/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adulcol.h>
#include    <aduucol.h>

/**
**
**  Name: ADTTUPCMP.C - Compare 2 tuples.
**
**  Description:
**	This file contains all of the routines necessary to perform the
**	external ADF call "adt_tupcmp()" which compares two tuples.
**
**      This file defines:
**
**          adt_tupcmp()  - Compare 2 tuples.
**	    adt_compare() - Generic function to compare two pieces
**			    of data.
**
**
**  History:
**      10-Feb-86 (thurston)
**          Initial creation.
**	03-apr-86 (thurston)
**	    Added two more return statuses (stati?) to adt_tupcmp():
**          E_AD2004_BAD_DTID and E_AD2005_BAD_DTLEN.
**      11-jun-86 (jennifer)
**          Fixed bug where compare result assigned to adt_cmp_result instead
**          of indirect through this pointer, i.e. *adt_cmp_result.
**      11-jun-86 (jennifer)
**	    Fixed bug where the first entry of attribute array (0)
**          was being used.  This entry is always garbage since 
**          attributes are numbered from 1.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	19-nov-86 (thurston)
**	    Fixed the comparison for text in adt_tupcmp().
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	22-sep-88 (thurston)
**	    In adt_tupcmp(), I added bit representation check as a first pass
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
**	13-May-2005 (thaju02)
**	    In adt_compare, for differing length vchar, return value 
**	    based on comparison of longer length vchar to blank. (B114514)
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

static i4
adt_utf8comp(
ADF_CB		    *adf_scb,
DB_ATTS		    *atr,
i4		    atr_bdt,
i2		    atr_len,
char		    *d1,
char		    *d2,
DB_STATUS	    *status);


/*{
** Name: adt_tupcmp() - Compare 2 tuples.
**
** Description:
**	This routine compares two tuples.  This will tell the caller if the
**	supplied tuples are equal, or if not, which one is "greater than" the
**	other.
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
**      10-Feb-86 (thurston)
**          Initial creation.
**	03-apr-86 (thurston)
**	    Initial coding.  Also, added two more return statuses (stati?):
**          E_AD2004_BAD_DTID and E_AD2005_BAD_DTLEN.
**      11-jun-86 (jennifer)
**          Fixed bug compare result asigned to adt_cmp_result instead
**          of indirect through this pointer, i.e. *adt_cmp_result.
**      11-jun-86 (jennifer)
**	    Fixed bug where the first entry of attribute array (0)
**          was being used.  This entry is always garbage since 
**          attributes are numbered from 1.
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
**	21-Apr-89 (anton)
**	    Added local collation support
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
**	    each attribute.
**	04-May-2005 (jenjo02)
**	    Move increment of "atr" out of adt_compare
**	    parm list, a problem on Linux.
*/

DB_STATUS
adt_tupcmp(
ADF_CB		    *adf_scb,
i4		    adt_natts,		/* # of attrs in each tuple. */
DB_ATTS 	    *adt_atts,		/* Ptr to vector of DB_ATTS,
					** 1 for each attr in tuple, and
					** in attr order.
					*/
char		    *adt_tup1,		/* Ptr to 1st tuple. */
char		    *adt_tup2,		/* Ptr to 2nd tuple. */
i4		    *adt_cmp_result)	/* Place to put cmp result. */

{
    DB_STATUS		status;
    DB_ATTS		*atr;		/* Ptr to current DB_ATTS */

    *adt_cmp_result = 0;
    status = E_DB_OK;

    /* Note we always skip 0th entry in DB_ATTS array */

    /* As long as there are attributes and they're equal... */
    while ( adt_natts-- > 0 && *adt_cmp_result == 0 && status == E_DB_OK )
    {
        atr = ++adt_atts;
        *adt_cmp_result = adt_compare(adf_scb,
				      atr,
				      adt_tup1 + atr->offset,
				      adt_tup2 + atr->offset,
				      &status);
    }
    return(status);
}

/*{
** Name: adt_compare() - Compare 2 data values.
**
** Description:
**	This routine compares two data values.  This will tell the caller if the
**	supplied values are equal, or if not, which one is "greater than" the
**	other.
**
**	This routine exists as a performance boost for DMF.  DMF is not allowed
**	to know how to compare data values for the various datatypes.
**	Therefore, if this routine did not exist, DMF would have to make a
**	function call to "adc_compare()" for every attribute in the tuples
**	being compared.  Even though tuples are constructs that seem a level
**	above what ADF logically handles, the performance improvement in this
**	case justifies this routine.
**
**	To avoid the overhead of an adc_compare call,
**	this routine has built into it the semantics for
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
**      atr                             Pointer to DB_ATTR.
**      d1                        	Pointer to first value.
**      d2                              Pointer to second value.
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
**      status                          Status returned from
**					adc_compare(), if called.
**	
**	    The following DB_STATUS codes may be returned by adc_compare():
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	    If a DB_STATUS code other than E_DB_OK is returned, the caller
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
** Returns:
**      i4  < 0   if 1st < 2nd
**          = 0   if 1st = 2nd
**          > 0   if 1st > 2nd
**
** Exceptions:
**       none
**
** Side Effects:
**       none
**
** History:
**      10-Feb-86 (thurston)
**          Initial creation.
**	03-apr-86 (thurston)
**	    Initial coding.  Also, added two more return statuses (stati?):
**          E_AD2004_BAD_DTID and E_AD2005_BAD_DTLEN.
**      11-jun-86 (jennifer)
**          Fixed bug compare result asigned to adt_cmp_result instead
**          of indirect through this pointer, i.e. *adt_cmp_result.
**      11-jun-86 (jennifer)
**	    Fixed bug where the first entry of attribute array (0)
**          was being used.  This entry is always garbage since 
**          attributes are numbered from 1.
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
**	21-Apr-89 (anton)
**	    Added local collation support
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
**	    Consolidated from the nine adt functions which duplicated
**	    this "compare" code. Runs the most optimal compare
**	    based on datatype for a single attribute.
**
**	    Optimized CHA/VCH compares to use MEcmp rather than
**	    CMcmpcase loop when possible.
**
**	    Replace giant switch statement with if-then-else
**	    ordered most likely to least likely, more or less.
**	    The switch was consuming 27% of the CPU used by this
**	    function.
**
**	    Added DB_STATUS *status as a function parameter so
**	    if adc_compare returns an error, it will be returned
**	    to the caller rather than as a bogus compare "result".
**	27-Apr-2005 (jenjo02 for stial01)
**	    Replaced computing compare value using arithmetic
**	    (d1 - d2) with logical compares ( d1 < d2...) as
**	    the arithmetic may over/underflow and return the
**	    wrong result.
**	13-May-2005 (thaju02)
**	    For differing length vchar, return value based on 
**	    comparison of longer length vchar to blank. (B114514)
**	19-Jun-2006 (gupsh01)
**	    Added support for new date/time types.
**	3-may-2007 (dougi)
**	    Tweak slightly to allow MEcmp() on collation/double byte
**	    if they're bytewise equal.
**	10-may-2007 (dougi)
**	    Add logic to handle ordering of c/text/char/varchar in UTF8
**	    server.
**	25-jul-2007 (gupsh01)	
**	    Although UTF8 is multibyte it is meant to be processed through
**	    UTF8 comparison code utilizing unicode comparison.
**	17-Aug-2007 (gupsh01)
**	    Fix the UTF8 handling.
*/
i4
adt_compare(
ADF_CB		    *adf_scb,
DB_ATTS		    *atr,		/* Attribute in question */
char		    *d1,		/* Ptr to 1st value */
char		    *d2,		/* Ptr to 2nd value */
DB_STATUS	    *status)		/* Status from adc_compare */

{
    i4			cur_cmp;	/* Result of latest attr cmp */
    i4			vi1, vi2;	/* Temp i4's used to cmp ints */
    i8			lli1, lli2;	/* Compare i8's */
    f8			vf1, vf2;	/* Temp f8's used to cmp flts */
    u_char	        *lc1, *lc2;
    u_char	        blank;
    i4			atr_bdt;	/* Base datatype of attr */
    i2			atr_len;	/* Length of cur attribute */
    i4			d1_isnull, d2_isnull;
    DB_DATA_VALUE	dv1, dv2;	/* Data value structs for call
					** to adc_compare(), if that is
					** necessary.
					*/
    u_char		*tc1, *tc2;	/* Temps used for string compare */
    u_char		*endtc1, *endtc2;
    UCS2		*tn1, *tn2;	/* Temps used for Nstring compare */
    UCS2		*endtn1, *endtn2;
	/*
	**  The following four lines provide temp storage to solve
	**  byte allignment problems on some archictectures.
	*/
    i2			i2_t1, i2_t2;
    i4			i4_t1, i4_t2;
    f4			f4_t1, f4_t2;
    f8			f8_t1, f8_t2;

#define	ADT_LT	(i4)-1
#define ADT_EQ	(i4)0
#define	ADT_GT	(i4)1

    *status = E_DB_OK;

    atr_len = atr->length;

    if ( (atr_bdt = atr->type) < 0 )
    {
	/* Nullable datatype */

	d1_isnull = (*((char *)d1 + atr_len - 1) & ADF_NVL_BIT);
	d2_isnull = (*((char *)d2 + atr_len - 1) & ADF_NVL_BIT);

	if ( !d1_isnull && !d2_isnull )
	{
	    /* Neither is the Null value, look at data */
	    atr_bdt = -atr_bdt;
	    atr_len--;
	}
	else if (d1_isnull && d2_isnull)
	    return(ADT_EQ);   /* both are Null values; 1st = 2nd */
	else if (d1_isnull)
	    return(ADT_GT);   /* 1st is Null value; 1st > 2nd */
	else
	    return(ADT_LT);  /* 2nd is Null value; 1st < 2nd */
    }

    /* "if then else" consumes ~27% less CPU than "switch" */

    if ( atr_bdt == DB_INT_TYPE )
    {
	/* If both operands aligned... */
	/* Extra casts to shut up compilers -- only care about low bits */
	if ( (((i4)(SCALARP)d1 | (i4)(SCALARP)d2) & atr_len-1) == 0 )
	{
	    if ( atr_len == 4 )
	    {
		if      (*(i4*)d1 < *(i4*)d2)	return(ADT_LT);
		else if (*(i4*)d1 > *(i4*)d2)	return(ADT_GT);
		else			  	return(ADT_EQ);
	    }
	    if ( atr_len == 8 )
	    {
		if      (*(i8*)d1 < *(i8*)d2)	return(ADT_LT);
		else if (*(i8*)d1 > *(i8*)d2)	return(ADT_GT);
		else			  	return(ADT_EQ);
	    }
	    if ( atr_len == 2 )
	    {
		if      (*(i2*)d1 < *(i2*)d2)	return(ADT_LT);
		else if (*(i2*)d1 > *(i2*)d2)	return(ADT_GT);
		else			  	return(ADT_EQ);
	    }
	
	    if      (*(i1*)d1 < *(i1*)d2)	return(ADT_LT);
	    else if (*(i1*)d1 > *(i1*)d2)	return(ADT_GT);
	    else			  	return(ADT_EQ);
	}
	/* One or both not aligned... */
	if (atr_len == 4)
	{
	    I4ASSIGN_MACRO(*d1, i4_t1);
	    I4ASSIGN_MACRO(*d2, i4_t2);
	    if      (i4_t1 < i4_t2)	return(ADT_LT);
	    else if (i4_t1 > i4_t2)	return(ADT_GT);
	    else			return(ADT_EQ);
	}
	if (atr_len == 8)
	{
	    I8ASSIGN_MACRO(*d1, lli1);
	    I8ASSIGN_MACRO(*d2, lli2);
	    if      (lli1 < lli2)	return(ADT_LT);
	    else if (lli1 > lli2)	return(ADT_GT);
	    else		  	return(ADT_EQ);
	}
	if (atr_len == 2)
	{
	    I2ASSIGN_MACRO(*d1, i2_t1);
	    I2ASSIGN_MACRO(*d2, i2_t2);
	    if      (i2_t1 < i2_t2)	return(ADT_LT);
	    else if (i2_t1 > i2_t2)	return(ADT_GT);
	    else			return(ADT_EQ);
	}
	if      (*(i1*)d1 < *(i1*)d2)	return(ADT_LT);
	else if (*(i1*)d1 > *(i1*)d2)	return(ADT_GT);
	else			  	return(ADT_EQ);
    }

    if ( atr_bdt == DB_FLT_TYPE || atr_bdt == DB_MNY_TYPE )
    {
	/* If both operands aligned... */
	/* Extra casts to shut up compilers, only care about low bits */
	if ( (((i4)(SCALARP)d1 | (i4)(SCALARP)d2) & atr_len-1) == 0 )
	{
	    if ( atr_len == 8 )
	    {
		if      (*(f8*)d1 < *(f8*)d2) return(ADT_LT);
		else if (*(f8*)d1 > *(f8*)d2) return(ADT_GT);
		else                  	  return(ADT_EQ);
	    }
	    if      (*(f4*)d1 < *(f4*)d2) return(ADT_LT);
	    else if (*(f4*)d1 > *(f4*)d2) return(ADT_GT);
	    else                  	  return(ADT_EQ);
	}

	/* One or both not aligned... */
	if ( atr_len == 8 )
	{
	    F8ASSIGN_MACRO(*d1, f8_t1);
	    F8ASSIGN_MACRO(*d2, f8_t2);
	    if      (f8_t1 < f8_t2) return(ADT_LT);
	    else if (f8_t1 > f8_t2) return(ADT_GT);
	    else   		    return(ADT_EQ);
	}
	F4ASSIGN_MACRO(*d1, f4_t1);
	F4ASSIGN_MACRO(*d2, f4_t2);
	if      (f4_t1 < f4_t2) return(ADT_LT);
	else if (f4_t1 > f4_t2) return(ADT_GT);
	else   		        return(ADT_EQ);
    }

    if ( atr_bdt == DB_BYTE_TYPE || atr_bdt == DB_TABKEY_TYPE ||
	      atr_bdt == DB_LOGKEY_TYPE )
    {
	return(MEcmp(d1, d2, atr_len));
    }

    if ( atr_bdt == DB_CHA_TYPE || atr_bdt == DB_VCH_TYPE )
    {
	if ( atr_bdt == DB_CHA_TYPE )
	{
	    /* Try the turbo compare. */
	    cur_cmp = MEcmp(d1, d2, atr_len);

	    /* If not equal and UTF8-enabled ... */
	    if (cur_cmp && (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
		return(adt_utf8comp(adf_scb, atr, atr_bdt, atr_len, 
							d1, d2, status));

	    /* If neither collation nor doublebyte or if byte-wise equal ... */
	    if ( !(adf_scb->adf_collation || 
		   Adf_globs->Adi_status & ADI_DBLBYTE) || cur_cmp == 0)
	    {
		return(cur_cmp);
	    }

	    tc1    = (u_char *) d1;
	    tc2    = (u_char *) d2;
	    endtc1 = tc1 + atr_len;
	    endtc2 = tc2 + atr_len;
	}
	else
	{
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)d1)->db_t_count, i2_t1);
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)d2)->db_t_count, i2_t2);
	    tc1 = (u_char *)d1 + DB_CNTSIZE;
	    tc2 = (u_char *)d2 + DB_CNTSIZE;

	    /* Try the turbo compare on the prefix. */
	    cur_cmp = MEcmp((char *)tc1, (char *)tc2, min(i2_t1, i2_t2));

	    /* If neither collation nor doublebyte... */
	    if ( !(adf_scb->adf_collation || 
		   Adf_globs->Adi_status & ADI_DBLBYTE) || 
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
	    {
		i2	diff = i2_t1 - i2_t2;
	    
		/* If not equal or lengths differ and UTF8-enabled ... */
		if ((diff || cur_cmp) && 
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
		    return(adt_utf8comp(adf_scb, atr, atr_bdt, atr_len, 
							d1, d2, status));

		/* If short compare produces inequality or lengths are
		** the same ... */
		if ( cur_cmp || diff == 0 )
		{
		    return(cur_cmp);
		}

		/*
		** Equal for shorter length.
		** If longer trails all blanks, they're equal
		*/
		if ( diff > 0 )
		{
		    /* tc1 is longer */
		    tc1 += i2_t2;
		    while ( *(tc1++) == MIN_CHAR && --diff > 0 );

		    if (!diff)
			return( ADT_EQ );
		    else
		    {
			tc1--;
			if (*tc1 > MIN_CHAR)
			    return(ADT_GT);
			else
			    return(ADT_LT);
		    }
		}
		else
		{
		    /* tc2 is longer */
		    tc2 += i2_t1;
		    while ( *(tc2++) == MIN_CHAR && ++diff < 0 );

		    if (!diff)
			return( ADT_EQ );
		    else
		    {
			tc2--;
			if (MIN_CHAR > *tc2)
			    return(ADT_GT);
			else 
			    return(ADT_LT);
		    }
		}

	    }
	    else if (i2_t1 == i2_t2 && cur_cmp == 0)
		return(cur_cmp);	/* byte wise compare returns "=" */

	    endtc1 = tc1 + i2_t1;
	    endtc2 = tc2 + i2_t2;
	}

	if (adf_scb->adf_collation)
	    return(adugcmp((ADULTABLE *)adf_scb->adf_collation,
			 ADUL_BLANKPAD, tc1, endtc1, tc2, endtc2));

	/* Doublebyte is rather more tedious */
	blank = (u_char)MIN_CHAR;
	cur_cmp = ADT_EQ;
	
	while ( cur_cmp == ADT_EQ )
	{
	    if (tc1 < endtc1)
	    {
		lc1 = tc1;
		CMnext(tc1);
	    }
	    else
	    {
		lc1 = &blank;
	    }
	    
	    if (tc2 < endtc2)
	    {
		lc2 = tc2;
		CMnext(tc2);
	    }
	    else
	    {
		lc2 = &blank;
	    }
	    
	    /* if both pointing to blank or we happen to be comparing
	    ** a string to itself then break
	    */
	    if (lc1 == lc2)
		break;
	    
	    cur_cmp = CMcmpcase(lc1, lc2);
	}
	return(cur_cmp);
    }

    if ( atr_bdt == DB_NCHR_TYPE || atr_bdt == DB_NVCHR_TYPE )
    {
	if ( atr_bdt == DB_NCHR_TYPE )
	{
	    tn1 = (UCS2 *)d1;
	    tn2 = (UCS2 *)d2;
	    endtn1 = tn1 + atr_len / sizeof(UCS2);
	    endtn2 = tn2 + atr_len / sizeof(UCS2);
	}
	else
	{
	    tn1 = ((DB_NVCHR_STRING *)d1)->element_array;
	    tn2 = ((DB_NVCHR_STRING *)d2)->element_array;
	    I2ASSIGN_MACRO(((DB_NVCHR_STRING *)d1)->count, i2_t1);
	    I2ASSIGN_MACRO(((DB_NVCHR_STRING *)d2)->count, i2_t2);
	    endtn1 = tn1 + i2_t1;
	    endtn2 = tn2 + i2_t2;
	}
      
	*status = aduucmp(adf_scb, ADUL_BLANKPAD,
	    tn1, endtn1, tn2, endtn2, &cur_cmp, atr->collID);
	return (cur_cmp);
    }

    if ( atr_bdt == DB_CHR_TYPE )
    {
	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    return(adt_utf8comp(adf_scb, atr, atr_bdt, atr_len, 
							d1, d2, status));

	if (adf_scb->adf_collation)
	    return(adugcmp((ADULTABLE *)adf_scb->adf_collation,
			     ADUL_SKIPBLANK, (u_char *)d1,
			     atr_len + (u_char *)d1, (u_char *)d2,
			     atr_len + (u_char *)d2));
	return(STscompare((char *)d1, atr_len, (char *)d2, atr_len));
    }

    if ( atr_bdt == DB_TXT_TYPE )
    {
	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    return(adt_utf8comp(adf_scb, atr, atr_bdt, atr_len, 
							d1, d2, status));

	I2ASSIGN_MACRO(((DB_TEXT_STRING *)d1)->db_t_count, i2_t1);
	I2ASSIGN_MACRO(((DB_TEXT_STRING *)d2)->db_t_count, i2_t2);
	tc1 = (u_char *)d1 + DB_CNTSIZE;
	tc2 = (u_char *)d2 + DB_CNTSIZE;
	endtc1 = tc1 + i2_t1;
	endtc2 = tc2 + i2_t2;

	if (adf_scb->adf_collation)
	    return(adugcmp((ADULTABLE *)adf_scb->adf_collation,
			     0, tc1, endtc1, tc2, endtc2));

	cur_cmp = ADT_EQ;
	while (cur_cmp == ADT_EQ && tc1 < endtc1  &&  tc2 < endtc2)
	{
	    if ( (cur_cmp = CMcmpcase(tc1, tc2)) == ADT_EQ )
	    {
		CMnext(tc1);
		CMnext(tc2);
	    }
	}
	
	/* If equal up to shorter, short strings < long */
	return( (cur_cmp) ? cur_cmp : (endtc1 - tc1) - (endtc2 - tc2) );
    }

    if ( atr_bdt == DB_DEC_TYPE )
    {
	/*  See if the bit representation is identical. */
	if ( MEcmp(d1, d2, atr_len) )
	{
	    i4	    pr;
	    i4	    sc;

	    pr = DB_P_DECODE_MACRO(atr->precision);
	    sc = DB_S_DECODE_MACRO(atr->precision);
	    return(MHpkcmp((PTR)d1, pr, sc, (PTR)d2, pr, sc));
	}
	return(ADT_EQ);
    }

    /* Bit-equivalent dates we can handle here */
    if (( atr_bdt == DB_DTE_TYPE || 
	 atr_bdt == DB_ADTE_TYPE ||
	 atr_bdt == DB_TMWO_TYPE ||
	 atr_bdt == DB_TMW_TYPE ||
	 atr_bdt == DB_TME_TYPE ||
	 atr_bdt == DB_TSWO_TYPE ||
	 atr_bdt == DB_TSW_TYPE ||
	 atr_bdt == DB_TSTMP_TYPE ||
	 atr_bdt == DB_INYM_TYPE ||
	 atr_bdt == DB_INDS_TYPE ) &&
	MEcmp(d1, d2, (u_i2)atr_len) == 0 )
    {
	return(ADT_EQ);
    }

    /* Will have to do it the slow way */
    dv1.db_datatype = dv2.db_datatype = (DB_DT_ID)atr_bdt;
    dv1.db_prec = dv2.db_prec = atr->precision;
    dv1.db_length = dv2.db_length = atr_len;
    dv1.db_collID = dv2.db_collID = atr->collID;
    dv1.db_data = d1;
    dv2.db_data = d2;
    *status = adc_compare(adf_scb, &dv1, &dv2, &cur_cmp);

    return(cur_cmp);
}

/*{
** Name: adt_utf8comp() - Compare 2 c/text/char/varchar values in UTF8 server
**
** Description:
**	This routine compares two string data values. Before they are compared,
**	they must be coerced to UCS2 and the comparison is done according to
**	the Unicode Collation Algorithm. This necessitates a call to 
**	adu_nvchr_utf8comp() to perform the setup and comparison. This function
**	presents the values as DB_DATA_VALUE structures, as expected by
**	adu_nvchr_utf8comp().
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      atr_bdt				Data type to be compared.
**	atr_len				Length of comparands.
**      d1                        	Pointer to first value.
**      d2                              Pointer to second value.
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
**      status                          Status returned from
**					adu_mvchr_utf8comp(), if called.
**	
**	    The following DB_STATUS codes may be returned by 
**	    adu_nvchr_utf8comp():
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	    If a DB_STATUS code other than E_DB_OK is returned, the caller
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
** Returns:
**      i4  < 0   if 1st < 2nd
**          = 0   if 1st = 2nd
**          > 0   if 1st > 2nd
**
** Exceptions:
**       none
**
** Side Effects:
**       none
**
** History:
**      10-may-2007 (dougi)
**	    Written for UTF8-enabled server.
**	05-nov-2007 (gupsh01)
**	    pass in default for quel pattern matching to 
**	    adu_nvchr_utf8comp.
**	19-mar-2008 (gupsh01)
**	    Use an aligned buffer for varchar and text types.
**	20-mar-2008 (gupsh01)
**	    Start with an aligned buffer to work with.
**	12-jul-2008 (gupsh01)
**	    Fix the size allocated for aligned buffer.
*/
static i4
adt_utf8comp(
ADF_CB		    *adf_scb,
DB_ATTS		    *atr,
i4		    atr_bdt,
i2		    atr_len,
char		    *d1,		/* Ptr to 1st value */
char		    *d2,		/* Ptr to 2nd value */
DB_STATUS	    *status)		/* Status from adc_compare */

{
    i4			cur_cmp;	/* Result of latest attr cmp */
    DB_DATA_VALUE	dv1;
    DB_DATA_VALUE	dv2;
    ALIGN_RESTRICT      temp_dv1[2048 / sizeof(ALIGN_RESTRICT)];
    ALIGN_RESTRICT      temp_dv2[2048 / sizeof(ALIGN_RESTRICT)];
    char		*dv1data = d1;
    char		*dv2data = d2;
    u_char		*tc1, *tc2, *dc1, *dc2;
    bool		getdv1mem = FALSE;
    bool		getdv2mem = FALSE;
    i4			reqlen;

    /* If varchar/text then use the aligned buffer */
    if ((abs(atr_bdt) == DB_VCH_TYPE) ||
        (abs(atr_bdt) == DB_TXT_TYPE))
    {
        STATUS		stat;
	/*
        ** Check for alignment
        */
	reqlen = atr_len + DB_CNTSIZE + sizeof(ALIGN_RESTRICT);
        if(ME_ALIGN_MACRO((PTR)d1, sizeof(ALIGN_RESTRICT)) !=d1)
	{
	  if (reqlen > sizeof(temp_dv1))
	  {
	    dv1data = (char *)MEreqmem(0, reqlen, FALSE, &stat);
	    if ((dv1data == NULL) || (stat != OK))
	      return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2, 0, 
			(i4) sizeof(stat), (i4 *)&stat));
            getdv1mem = TRUE;
	  }
	  else
	    dv1data = (char *)&temp_dv1[0];
	  
    	  I2ASSIGN_MACRO(((DB_TEXT_STRING *)d1)->db_t_count, 
			 ((DB_TEXT_STRING *)dv1data)->db_t_count); 
          tc1 = (u_char *)d1 + DB_CNTSIZE;
	  dc1 = (u_char *)dv1data + DB_CNTSIZE; 
	  MEcopy ( tc1, atr_len, dc1);
	}

        if(ME_ALIGN_MACRO((PTR)d2, sizeof(ALIGN_RESTRICT)) !=d2)
	{
	  if (reqlen > sizeof(temp_dv2))
	  {
	    dv2data = (char *)MEreqmem(0, reqlen , FALSE, &stat);
	    if ((dv2data == NULL) || (stat != OK))
	      return (adu_error(adf_scb, E_AD2042_MEMALLOC_FAIL, 2, 0, 
			(i4) sizeof(stat), (i4 *)&stat));
            getdv2mem = TRUE;
	  }
	  else
	    dv2data = (char *)&temp_dv2[0];

    	  I2ASSIGN_MACRO(((DB_TEXT_STRING *)d2)->db_t_count, 
			 ((DB_TEXT_STRING *)dv2data)->db_t_count); 
          tc2 = (u_char *)d2 + DB_CNTSIZE;
	  dc2 = (u_char *)dv2data + DB_CNTSIZE; 
	  MEcopy ( tc2, atr_len, dc2);
	}
    }

    /* Set up DB_DATA_VALUEs. */
    dv1.db_datatype = dv2.db_datatype = (DB_DT_ID)atr_bdt;
    dv1.db_prec = dv2.db_prec = atr->precision;
    dv1.db_length = dv2.db_length = atr_len;
    dv1.db_collID = dv2.db_collID = atr->collID;
    dv1.db_data = dv1data;
    dv2.db_data = dv2data;
    *status = adu_nvchr_utf8comp(adf_scb, 0, &dv1, &dv2, &cur_cmp);

    if (getdv1mem && dv1data)
	  MEfree (dv1data);

    if (getdv2mem && dv2data)
	  MEfree (dv2data);

    return(cur_cmp);
}
