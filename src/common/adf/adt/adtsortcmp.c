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
#include    <aduint.h>

/*
** NO_OPTIM = nc4_us5
*/

/**
**
**  Name: ADTSORTCMP.C - Compare 2 sort records.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adt_sortcmp()" which compares two sort records.
**
**      This file defines:
**
**          adt_sortcmp() - Compare 2 sort records.
**
**
**  History:
**      01-jul-87 (Jennifer)
**          Initial creation.
**	22-sep-88 (thurston)
**	    In adt_sortcmp(), I added bit representation check as a first pass
**	    on each attr.  Also, added code to deal with nullable types here
**	    ... this could be a big win, since all nullable types were going
**	    through adc_compare(), now they need not.  Also, added cases for
**	    CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	21-Apr-89 (anton)
**	    Added local collation support
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	18-aug-89 (jrb)
**	    Copied bug fix from adu code to adt routines.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	28-oct-92 (rickh)
**	    Replaced references to ADT_CMP_LIST with DB_CMP_LIST so that we
**	    could collapse two identical structures (ADT_CMP_LIST and
**	    DM_CMP_LIST) into one.
**      29-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-feb-96 (thoda04)
**	    Added mh.h to include function prototype for MKpkcmp().
**	    Added aduint.h to include function prototype for adu_1seclbl_cmp().
**	13-jul-00 (hayke02)
**	    Added NO_OPTIM for NCR (nc4_us5). This change fixes bug 102080.
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
**      13-jul-2001 (stial01)
**          case DB_NVCHR_TYPE, don't MEreqmem for zero bytes (B105240)
**	23-jul-2001 (gupsh01)
**	    use I2_ASSIGN_MACRO for DB_NVCHR_TYPE case. Always align the 
**	    memory even when the input data is zero bytes.
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**	9-Jan-2004 (schka24)
**	    Add 8-byte int case.
**/


/*{
** Name: adt_sortcmp() - Compare 2 sort records.
**
** Description:
**	This routine compares two sort records.  It will tell the caller if the
**	supplied records are equal, or if not, which one is "greater than" the
**	other.  It also tells the caller which attribute caused the sort
**	difference if the two records are not equal.
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
**	INGRES now supports some datatypes which are inherently unsortable.
**	For these datatypes, adt_sortcmp() returns a whatever the caller wishes.
**	This allows the caller to request the appropriate value to keep the
**	sort stable.  The "normal" behavior for unsortable objects is to leave
**	them in their original order.  However, returning "equal" all the time
**	is less than useful because it will "accidentally" remove duplicates.
**	Therefore, the return value is caller's choice.
**
**	The sorter itself should deal with that appropriately.
**
**	This is done on purpose.  ADF could, conceivably, return that anything
**	unsortable is either "plain" greater or less than.  However, this is
**	likely to cause havoc in some sort algorithms if/when if finds that
**	a > b AND b > a.  Therefore, ADF returns a requested indication, and
**	leaves it up to the sorter to maintain sanity & stability.
**
**	PLEASE NOTE:  As with "adc_compare()", no pattern matching for
**	-----------   the string datatypes is done within this routine.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	adt_atts			Pointer to vector of  DB_CMP_LIST, 
**					which describes each attributes.
**					in attr order.
**      adt_natts                       Number of attributes in the
**					DB_CMP_LIST.  (i.e. Number of attrs
**					to use in compare.)
**      adt_tup1                        Pointer to the 1st tuple data
**                                      record.
**      adt_tup2                        Pointer to the 2nd tuple data
**                                      record.
**	dif_ret_value			Value to return in the event that a
**					field of type DB_DIF_TYPE is
**					encountered.
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
**
**      Returns:
**	     0	    Means that the tuples are considered `equal' in sort order.
**
**	    -n	    Means that tuple 1 is `less than' tuple 2, and `n' will
**		    be the sequence number (in the sort compare list) of the
**		    first attribute they differed on.
**
**	    +n	    Means that tuple 1 is `greater than' tuple 2, and `n' will
**		    be the sequence number (in the sort compare list) of the
**		    first attribute they differed on.
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
**	12-nov-1987 (Derek)
**	    Change return value to also indicate the attribute number that
**	    caused a not equal comparison.  This is indicated by the absolute
**	    number of the return value.  Attributes are number from one.
**	22-sep-88 (thurston)
**	    Added bit representation check as a first pass on each attr.  Also,
**	    added code to deal with nullable types here ... this could be a big
**	    win, since all nullable types were going through adc_compare(), now
**	    they need not.  Also, added cases for CHAR and VARCHAR.
**	21-oct-88 (thurston)
**	    Got rid of all of the `#ifdef BYTE_ALIGN' stuff by using the
**	    appropriate [I2,I4,F4,F8]ASSIGN_MACROs.  This also avoids a bunch
**	    of MEcopy() calls on BYTE_ALIGN machines.
**	21-Apr-89 (anton)
**	    Added local collation support
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	25-jul-89 (jrb)
**	    Added support for decimal datatype.
**	03-nov-89 (fred)
**	    Added support for unsortable datatypes.
**	02-jan-90 (jrb)
**	    Fix alignment problem.
**	08-aug-91 (seg)
**	    "d1" and "d2" are dereferenced too often with the assumption
**	    that (PTR) == (char *) to fix.  Made them (char *).
**	17-oct-97 (inkdo01)
**	    Tiny change to avoid re-evaluation of compare on some
**	    char/varchar/text fields.
**	23-oct-97 (inkdo01)
**	    Tinier change to only apply above heuristic on fixed length 
**	    char, since garbage may appear in vch/text after the actual
**	    value length.
**      12-nov-98 (gygro01)
**          Removed the above change on the fixed length char, because
**          of bug 92675. Problem is the usage of special characters
**          (European 8bit) without a collation sequence defined.
**          The change make us rely on the return of MEcmp. On platforms,
**          where we do not use the compare function of the OS, we
**          compare actually with signed chars.
**          This causes wrong sorting and queries may return incorrect
**          results using special characters without a collation sequence
**          defined.   
**	28-mar-2001 (abbjo03)
**	    Add support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	25-may-2001 (stephenb)
**	    Allocte memory for nchar/nvarchar strings because they need to
**	    be alligned to prevent bus errors
**	16-dec-04 (inkdo01)
**	    Add collation ID parm to aduucmp() calls.
**	02-Feb-2005 (jenjo02)
**	    Changed to call (new) adt_compare function for
**	    each attribute.
*/

i4
adt_sortcmp(
ADF_CB		    *adf_scb,
DB_CMP_LIST	    *adt_atts,		/* Ptr to vector of DB_CMP_LIST,
					** 1 for each attr in tuple, and
					** in attr order.
					*/
i4		    adt_natts,		/* # of attrs in each tuple. */
char		    *adt_tup1,		/* Ptr to 1st tuple. */
char		    *adt_tup2,		/* Ptr to 2nd tuple. */
i4		    dif_ret_value)	/*
					** Value to return if difference found
					** is of type DB_DIF_TYPE
					*/

{
    i4			adt_cmp_result;	/* Place to put cmp result. */
    DB_STATUS		status;		/* Used to catch return status
					** from the adc_compare() call,
					** if the call is made.
					*/
    i4			cur_cmp;	/* Result of latest attr cmp */
    DB_CMP_LIST		*atr;		/* Ptr to current atts list */
					/********************************/
    char		*d1;		/* Ptrs to data in data records */
    char		*d2;		/* for current attributes       */
					/********************************/
    i4			atr_bdt;	/* Base datatype of attr */
    i4			atr_len;	/* Length of cur attribute */
    i4			natts = adt_natts;
    DB_ATTS		Datt;


    for (atr = adt_atts; --natts >= 0; atr++)
    {
	d1 = (char *)adt_tup1 + atr->cmp_offset;
	d2 = (char *)adt_tup2 + atr->cmp_offset;
	atr_len = atr->cmp_length;

	if ( (atr_bdt = atr->cmp_type) < 0)
	{
	    /* Nullable datatype */

	    i4	    d1_isnull = (*((char *)d1 + atr_len - 1) & ADF_NVL_BIT);
	    i4	    d2_isnull = (*((char *)d2 + atr_len - 1) & ADF_NVL_BIT);

	    if ( !d1_isnull && !d2_isnull )
	    {
		/* Neither is the Null value, look at data */
		atr_bdt = -atr_bdt;
		atr_len--;
	    }
	    else if (d1_isnull && d2_isnull)
	    {
		continue;   /* both are Null values; 1st = 2nd */
	    }
	    else if (d1_isnull)
	    {
		/* 1st is Null value; 1st > 2nd */
		adt_cmp_result = adt_natts - natts;
		if (atr->cmp_direction)
		    return (-adt_cmp_result);
		return (adt_cmp_result);
	    }
	    else if (d2_isnull)
	    {
		/* 2nd is Null value; 1st < 2nd */
		adt_cmp_result = natts - adt_natts;
		if (atr->cmp_direction)
		    return (-adt_cmp_result);
		return (adt_cmp_result);
	    }
	}
	
	if ( atr_bdt == DB_DIF_TYPE )
	{
	    if ( dif_ret_value )
		return(dif_ret_value);
	    continue;
	}

	/* Construct a (non-null) DB_ATTS for adt_compare */
	Datt.type = atr_bdt;
	Datt.length = atr_len; 
	Datt.precision = atr->cmp_precision;
	Datt.collID = atr->cmp_collID;

	if ( cur_cmp = adt_compare(adf_scb,
				   &Datt,
				   d1,
				   d2,
				   &status) )
	{
	    adt_cmp_result = adt_natts - natts;
	    if (cur_cmp < 0) 
		adt_cmp_result = -adt_cmp_result;
	    if (atr->cmp_direction)
		return (-adt_cmp_result);
	    return (adt_cmp_result);
	}
    }
    return (0);
}
