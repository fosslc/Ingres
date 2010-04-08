/*
** Copyright (c) 2004, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adudate.h>
#include    "adutime.h"
/*  [@#include@]... */

/**
**
**  Name: ADUEXTDT.C - Externally visible date functions.
**
**  Description:
**	Functions available to external callers relating to dates.
**
**	This file defines:
**	    adu_datenow()    -	Get an INGRES internal date for now
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**	24-oct-89 (jrb)
**	    Created.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	13-feb-1998 (walro03)
**	    aduadtdate.h and adudate.h are redundant.  aduadtdate.h deleted and
**	    references (such as here) changed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Feb-2008 (kiria01) b120004
**	    Consolidate timezone handling
**/


/*{
** Name: adu_datenow	- Retrieve current time and date
**
** Description:
**	This routine will fill the result with the INGRES internal date
**	for the current date and time.
**	
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adu_dv				Pointer to the data value to be filled:
**		.db_datatype		Must be DB_DTE_TYPE (non-nullable DATE).
**		.db_prec		Ignored (should be zero).
**		.db_length  		Must be sizeof(DB_DATE)
**		.db_data    		Pointer to result location to fill.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      adu_dv				Data value to be filled:
**	        .db_data    		INGRES date of "now".
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	    E_AD2004_BAD_DTID		Datatype passed in was not valid
**					(must be non-nullable DATE)
**	    E_AD2005_BAD_DTLEN		Length passed in was not valid
**					(must be sizeof(DB_DATE))
**					
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	24-oct-89 (jrb)
**	    Created.
**	19-jun-2006 (gupsh01)
**	    Added support for new datetime datatypes.
**	01-aug-2006 (gupsh01)
**	    Supply nanosecond parameter in adu_cvtime() 
**	    call.
**	29-aug-2006 (gupsh01)
**	    Removed support for ANSI datetime datatypes.
**	    date(now) is only used for ingresdate types.
**	07-nov-2006 (gupsh01)
**	    Fixed the second calculations.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL. Also ensure that the dn structure
**	    is fully iniialised.
*/
DB_STATUS
adu_datenow(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *adu_dv)
{
    i4			bintim_secs;
    DB_DATA_VALUE	dv_tmp;
    DB_STATUS		db_stat;
    struct timevect	tv;
    AD_NEWDTNTRNL	dn;

    /* Check validity of inputs */
    if (adu_dv->db_datatype != DB_DTE_TYPE)
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    if (adu_dv->db_length != ADF_DTE_LEN)
	return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0));


    dv_tmp.db_datatype = DB_INT_TYPE;
    dv_tmp.db_prec = 0;
    dv_tmp.db_length = 4;
    dv_tmp.db_data = (PTR) &bintim_secs;

    if (db_stat = adu_dbconst(adf_scb, &Adf_globs->Adk_bintim_map, &dv_tmp))
	return (db_stat);

    adu_cvtime((i4) bintim_secs, 0, &tv);

    dn.dn_year	= tv.tm_year + AD_TV_NORMYR_BASE;
    dn.dn_month    = tv.tm_mon + AD_TV_NORMMON;
    dn.dn_day   = tv.tm_mday;
    dn.dn_seconds = tv.tm_hour * AD_39DTE_ISECPERHOUR +
		      tv.tm_min * AD_10DTE_ISECPERMIN +
		      tv.tm_sec;
    dn.dn_nsecond = 0;
    AD_TZ_SETNEW(&dn, 0);
    dn.dn_status = AD_DN_ABSOLUTE | AD_DN_TIMESPEC;
    dn.dn_dttype = adu_dv->db_datatype;
    return (adu_7from_dtntrnl (adf_scb, adu_dv, &dn));
}
