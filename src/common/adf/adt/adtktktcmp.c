/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <sl.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adulcol.h>
#include    <aduucol.h>

/**
**
**  Name: ADTKTKTCMP.C - Compare keys in tuples.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adt_ktktcmp()" which compares a key in a tuple to a
**	key in a tuple.
**
**      This file defines:
**
**          adt_ktktcmp() - Compare keys in tuples.
**
**
**  History:
**      11-jun-86 (jennifer)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	19-nov-86 (thurston)
**	    Fixed the comparison for text in adt_tupcmp().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	22-sep-88 (thurston)
**	    In adt_ktktcmp(), I added bit representation check as a first pass
**	    on each attr.  Also, added code to deal with nullable types here
**	    ... this could be a big win, since all nullable types were going
**	    through adc_compare(), now they need not.  Also, added cases for
**	    CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	22-May-89 (anton)
**	    Local collation routines added
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	18-aug-89 (jrb)
**	    Copied bug fix from adu code to adt routines.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	28-oct-1992 (rickh)
**	    Replaced references to ADT_ATTS with DB_ATTS.  This compresses
**	    into one common structure the identical ADT_ATTS, DMP_ATTS, and
**	    GW_DMP_ATTS.
**      29-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-feb-96 (thoda04)
**	    Added mh.h to include function prototype for MKpkcmp().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	06-apr-2001 (abbjo03)
**	    Fix 3/28 change: include aduucol.h and correct first parameter to
**	    aduucmp().
**	01-may-2001 (abbjo03)
**	    Further correction to 3/28 change: fix calculation of DB_NCHR_TYPE
**	    endpoints.
**	10-aug-2001 (abbjo03)
**	    Yet another fix to 3/28/01 change. Correct the DB_NVCHR_TYPE start
**	    of data points.
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**	9-Jan-2004 (schka24)
**	    Add 8-byte int case.
**/


/*{
** Name: adt_ktktcmp()- Compare keys in tuples.
**
** Description:
**      This routine compares a key in a tuple with a key in another tuple.
**      This will tell the caller
**
**	This routine exists as a performance boost for DMF.  DMF is not allowed
**	to know how to compare data values for the various datatypes.
**	Therefore, if this routine did not exist, DMF would have to make a
**	function call to "adc_compare()" for every attribute in the tuples
**	being compared.  Even though tuples are constructs that seem a level
**	above what ADF logically handles, the performance improvement in this
**	case justifies this routine.
**
**	To avoid the overhead of a function call for each attribute in the
**	tuples to be compared, this routine has built into it the semantics for
**	comparing several of the most common datatypes supported by INGRES.
**
**	If the routine does run across a datatype that is not in this list, it
**	will call the general routine "adc_compare()", thereby guaranteeing
**	that it will function (perhaps slower), for ALL datatypes, even future
**	user defined ADTs.
**
**	When NULL values are involved, this routine will use the same semantics
**	that "adc_compare()" uses:  a NULL value will be considered greater
**	than any other value, and equal to another NULL value.
**
**	PLEASE NOTE:  As with "adc_compare()", no pattern matching for
**	-----------   the string datatypes is done within this routine.
**
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adt_nkatts                      Number of attributes that make
**                                      up the key.
**	adt_katts                       Pointer to vector of pointers.
**					Each pointer in this vector
**					will point to a DB_ATTS
**					corresponding to an attribute
**					in the tuple that helps make
**					up the key.  This vector will
**					be in "key" order.
**	adt_tup1			Pointer to the key.  This is
**					merely a byte string, with the
**					attributes in tuple format,
**					lined up in key order.
**      adt_tup2                        Pointer to the tuple data
**                                      record.
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
**      adt_cmp_result                  Result of comparison.  This is
**                                      guranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD2005_BAD_DTLEN          Internal length is illegal for
**					the given datatype.
**		(Others not yet defined)
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      11-jun-86 (jennifer)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	19-nov-86 (thurston)
**	    Fixed the comparison for text.
**	22-sep-88 (thurston)
**	    Added bit representation check as a first pass on each attr.  Also,
**	    added code to deal with nullable types here ... this could be a big
**	    win, since all nullable types were going through adc_compare(), now
**	    they need not.  Also, added cases for CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	22-May-89 (anton)
**	    Local collation routines added
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	08-aug-91 (seg)
**	    "d1" and "d2" are dereferenced too often with the assumption
**	    that (PTR) == (char *) to fix.  Made them (char *).
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	16-dec-04 (inkdo01)
**	    Add collation ID parm to aduucmp() calls.
**	02-Feb-2005 (jenjo02)
**	    Changed to call (new) adt_compare function for
**	    each attribute, return real status.
*/

DB_STATUS
adt_ktktcmp(
ADF_CB		    *adf_scb,
i4		    adt_nkatts,		/* Number attrs in key. */
DB_ATTS	            **adt_katts,	/* Ptr to vector of ptrs.
					** Each ptr in this vector
					** will point to a DB_ATTS
					** corresponding to an attribute
					** in the tuple that helps make
					** up the key.  This vector will
					** be in "key" order.
					*/
char		    *adt_tup1,		/* Ptr to the tuple. */
char		    *adt_tup2,		/* Ptr to the tuple. */
i4		    *adt_cmp_result)	/* Place to put cmp result. */

{
    DB_STATUS		status;
    DB_ATTS		*atr;

    *adt_cmp_result = 0;
    status = E_DB_OK;

    /* As long as there are equal attributes... */
    while ( adt_nkatts-- > 0 && status == E_DB_OK && *adt_cmp_result == 0 )
    {
	atr = *(adt_katts++);
	*adt_cmp_result = adt_compare(adf_scb,
				      atr,
				      adt_tup1 + atr->offset,
				      adt_tup2 + atr->offset,
				      &status);
    }

    return(status);
}
