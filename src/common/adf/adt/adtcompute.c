/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <mh.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adulcol.h>
#include    <aduucol.h>
#include    <aduint.h>

/**
**
**  Name: ADTCOMPUTE.C - Compare 2 tuples and determines if values changed.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adt_comput_change()" which compares two tuples and
**	determines if their values have changed.  It will determine if it was
**	just a bit or representation change or if a key or non-key attributes
**	actually changed values.  A bit map is used to determine which
**	attributes have been changed.
**
**      This file defines:
**
**          adt_compute_change() - Compare 2 tuples to see if values changed.
**
**
**  History:
**      01-jul-87 (Jennifer)
**          Initial creation.
**	22-sep-88 (thurston)
**	    In adt_compute_change(), I added code to deal with nullable types
**	    here ... this could be a big win, since all nullable types were
**	    going through adc_compare(), now they need not.  Also, added cases
**	    for CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	21-Apr-89 (anton)
**	    added local collation support
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	18-aug-89 (jrb)
**	    Copied bug fix from adu code to adt routines.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	04-sep-1992 (fred)
**	    Aded DB_LVCH_TYPE support
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
**      28-Sep-1993 (fred)
**          Corrected support for generic large object types.
**      26-feb-96 (thoda04)
**          Added mh.h to include function prototype for MKpkcmp().
**          Added aduint.h to include function prototype for adu_1seclbl_cmp().
**      12-Jul-1999 (hanal04)
**          Added ex.h to allow exception handling within adt_compute_change().
**          Added adt_handler() to deal with the exception. b85041.
**      22-Jul-1999 (hanal04)
**          Modify the above change to fix build problems. b98034.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**      03-apr-2001 (gupsh01)
**          Add support for DB_LNVCHR_TYPE.
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
**      3-aug-2005 (jenjo02)
**          Check for representation change if d1_isnull && d2_isnull (b114887)
**/


/*{
** Name: adt_comput_change() - Compare 2 tuples to see if values changed.
**
** Description:
**	This routine compares two tuples and determines if their values have
**	changed.  It will determine if it was just a bit or representation
**	change or if key or non-key attributes actually changed values.
**	A bit map is used to determine which attributes have been changed.
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
**      adt_natts                       Number of attributes in each
**					tuple.
**	adt_atts			Pointer to vector of DB_ATTS, 1
**					for each attribute in tuple, and
**					in attr order.
**      adt_tup1                        Pointer to the 1st tuple data
**                                      record.
**      adt_tup2                        Pointer to the 2nd tuple data
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
**      adt_change_set                  Pointer to an array containing
**                                      bit map for change set.
**                                      Assume zero filled.
**      adt_value_set                   Pointer to an array containing
**                                      bit map for value changes.
**                                      Assume zero filled.
**      adt_key_change                  Varaiable used to indicate key changed.
**      adt_nkey_change                 Variable used to indicate non-key 
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
**      01-jul-87 (Jennifer)
**          Initial creation.
**	22-sep-88 (thurston)
**	    Added code to deal with nullable types here ... this could be a big
**	    win, since all nullable types were going through adc_compare(), now
**	    they need not.  Also, added cases for CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	21-Apr-89 (anton)
**	    Added local collation support
**	17-Jun-89 (anton)
**	    Moved local collation support routines to mainline from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	08-aug-91 (seg)
**	    "d1" and "d2" are dereferenced too often with the assumption
**	    that (PTR) == (char *) to fix.  Made them (char *).
**	04-sep-1992 (fred)
**	    Added DB_LVCH_TYPE support.  At present, long varchar's don't
**	    compare, and are therefore always different.
**      28-Sep-1993 (fred)
**          Corrected support for generic large object types.  Added 
**          support for DB_LBYTE_TYPE.
*	23-jul-1996 (ramra01)
**	    Skip over dropped columns, avoid comparing them.
**      12-Jul-1999 (hanal04)
**          Trap and deal with SIGFPEs that can be caused by uninitialised
**          floating point variables from the user application. b85041.
**      22-Jul-1999 (hanal04)
**          Modify the above change to fix build problems caused by
**          call to ule_format in ADT. Instead use adu_error() and
**          check the ADF_CB in DMF in order to log the appropriate
**          error message. b98034.
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**      03-apr-2001 (gupsh01)
**          Add support for DB_LNVCHR_TYPE.
**	16-dec-04 (inkdo01)
**	    Add collation ID parm to aduccmp() calls.
**	02-Feb-2005 (jenjo02)
**	    Changed to call (new) adt_compare function for
**	    each attribute, return real status.
**	    Check NULL attributes first, avoid double compares
**	    (bit vs logical) when not needed.
*/

DB_STATUS
adt_compute_change(
ADF_CB		    *adf_scb,
i4		    adt_natts,		/* # of attrs in each tuple. */
DB_ATTS	    	    *adt_atts,		/* Ptr to vector of DB_ATTS,
					** 1 for each attr in tuple, and
					** in attr order.
					*/
char		    *adt_tup1,		/* Ptr to 1st tuple. */
char		    *adt_tup2,		/* Ptr to 2nd tuple. */
i4		    adt_change_set[(DB_MAX_COLS + 31) / 32],
i4		    adt_value_set[(DB_MAX_COLS + 31) / 32],
i4		    *adt_key_change,	/* Place to put cmp result. */
i4		    *adt_nkey_change)	/* Place to put cmp result. */

{
    DB_ATTS		*atr;		/* Ptr to current DB_ATTS */
					/********************************/
    char		*d1;		/* Ptrs to data in data records */
    char		*d2;		/* for current attributes       */
					/********************************/
    i2			atr_len;	/* Length of cur attribute */
    i4			atr_bdt;
    i4			i;
    i4			d1_isnull, d2_isnull;
    DB_STATUS		status = E_DB_OK;

    for (i = 1; i <= adt_natts && status == E_DB_OK; i++)
    {
	atr = ++adt_atts;

	if (atr->ver_dropped)
	   continue;

	atr_len = atr->length;

	d1 = (char *)adt_tup1 + atr->offset;
	d2 = (char *)adt_tup2 + atr->offset;

	if ( (atr_bdt = atr->type) < 0 )
	{
	    /* Nullable. Check to see if the nullability changed. */

	    d1_isnull = (*((char *)d1 + atr_len - 1) & ADF_NVL_BIT);
	    d2_isnull = (*((char *)d2 + atr_len - 1) & ADF_NVL_BIT);

	    if ( !d1_isnull && !d2_isnull )
	    {
		/* Neither is the Null value, look at data */
		atr_len--;
		atr_bdt = -atr_bdt;
	    }
	    else if (d1_isnull && d2_isnull)
	    {
		/* both are Null values, check if representation changed */
		if ( MEcmp((PTR)d1, (PTR)d2, atr_len-1) )
		{
		    adt_change_set[i >> 5] |= (1 << (i & 0x1f));
		    if ( atr->key )
			*adt_key_change |= ADT_REPRESENTATION;
		    else
			*adt_nkey_change |= ADT_REPRESENTATION;
		}
		continue;
	    }
	    else
	    {
		/* One is Null, the other not; values differ */
		if (atr->key)
		    *adt_key_change = ADT_VALUE;
		else
		    *adt_nkey_change = ADT_VALUE;
		adt_change_set[i >> 5] |= (1 << (i & 0x1f));
		adt_value_set[i >> 5] |= (1 << (i & 0x1f));
		continue;
	    }
	}

	/*  See if the bit representation has changed. */
	if ( MEcmp((PTR)d1, (PTR)d2, atr_len) )
	{
	    /* The bit representation has changed */
	    adt_change_set[i >> 5] |= (1 << (i & 0x1f));

	    switch ( atr_bdt )
	    {
	      case DB_INT_TYPE:
	      case DB_FLT_TYPE:
	      case DB_MNY_TYPE:
	      case DB_CHA_TYPE:
	      case DB_LVCH_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_LNVCHR_TYPE:
	      case DB_BYTE_TYPE:
	      case DB_TABKEY_TYPE:
	      case DB_LOGKEY_TYPE:

		/* For these types, MEcmp is all we need */
		break;
	    
	      default:
		if ( Adf_globs->Adi_dtptrs[
		  ADI_DT_MAP_MACRO(atr_bdt)]->adi_dtstat_bits
		                               & AD_NOSORT)
		    break;

		/* Compare the logical values */
		if ( adt_compare(adf_scb,
				 atr,
				 d1,
				 d2,
				 &status) )
		    break;
		/*
		** The bits have changed but the
		** logical value has not.
		*/
		if (atr->key)
		    *adt_key_change |= ADT_REPRESENTATION;
		else
		    *adt_nkey_change |= ADT_REPRESENTATION;
		continue;
	    }

	    /* The value has changed */
	    if (atr->key)
		*adt_key_change = ADT_VALUE;
	    else
		*adt_nkey_change = ADT_VALUE;
	    adt_value_set[i >> 5] |= (1 << (i & 0x1f));
	}
    }
    return (status);
}
