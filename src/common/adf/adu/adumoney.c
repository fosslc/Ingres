/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <ex.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <cv.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adumoney.h>
/*  [@#include@]...	*/

/*
NO_DBLOPTIM = hp8_us5
*/

/**
**
**  Name: ADUMONEY.C - implements ADF function instances for money datatype.
**
**  Description:
**	  This file contains the routines that implement the ADF function
**	instances for the money datatype.
**
**	This file defines:
**
**	    adu_11mny_round -	round internal money to two decimal places.
**	    adu_3mny_absu() -	get the absolute value of money.
**	    adu_4mny_addu() -	function instance to add two money values.
**	    adu_2mnytomny() -	convert money to money, for copying into
**				tuple buffers.
**	    adu_7mny_divf() -	function instance to divide number by money
**				value.
**	    adu_8mny_divu() -	function instance to divide money.
**	    adu_1mnytonum() -	convert money to a numeric type (f or dec)
**	    adu_9mnytostr() -	convert money to an Ingres string type.
**	    adu_5mnyhmap() -	get money's histogram value.
**	    adu_10mny_multu() -	function instance to multiply money.
**	    adu_numtomny() -	convert a number to internal money format.
**          adu_2strtomny() -	convert "money" string to internal money format.
**	    adu_12mny_subu() -	function instance to subtract two money values.
**	    adu_posmny() -	function instance to do unary "+" money value.
**	    adu_negmny() -	function instance to negate a money value.
**	    adu_cpmnytostr() -	convert money to an Ingres string type for COPY.
**
**  Function prototypes defined in ADUINT.H file except adu_11mny_round, which
**  is defined in ADUMONEY.H file.
**
**  History:
**      Revision 30.5  85/10/14  16:33:40  daveb
**      Fix adu_9mnytostr():  Eliminate complicated and broken
**      special case for 0.0;  Kill unneeded cnv_tbl[];
**      Sensibly do nothing on buffer overflow.
**      Revision 30.2  85/09/18  10:43:29  roger
**      Use AD_5MNY_OUTLENGTH and AD_6MNY_NUMLEN, not magic constants.
**      Add note in in adu_numtomny().  Add question in adu_7mny_divf()
**      about use of if statement.
**      (Replies to Greg, not to me!)
**      Revision 3.22.40.3  85/07/11  21:36:27  roger
**      Use I1_CHECK_MACRO to ensure preservation of i1 values
**      across assignments on machines w/o signed chars.
**      9-may-86 (ericj)    
**          Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-aug-86 (thurston)
**	    Added in #include's for <ulf.h> and <adftrace.h>.
**	20-nov-86 (thurston)
**	    Added the adu_posmny() and adu_negmny() routines.
**      25-feb-87 (thurston)
**          Made changes for using server CB instead of GLOBALDEF/REFs.
**	18-mar-87 (thurston)
**	    Fixed bug with several calls to adu_error():  missing `parameter
**	    count' parameter.
**	29-apr-87 (thurston)
**	    Do min/max money check at start of adu_11mny_round() routine, so
**	    that STprintf() doesn't get called with something it can not format.
**	10-jun-87 (thurston)
**	    Greatly simplified adu_numtomny() by using a call to
**	    adu_11mny_round().  Also fixed adu_7mny_divf() to return an f8;
**	    it had been returning a money.
**	03-aug-87 (thurston)
**	    Added the adu_cpmnytostr() routine for COPY.
**	05-aug-87 (thurston)
**	    Fixed adu_numtomny() so that it now catches max/min money overflow
**          on even the largest f8 values.  Previously, the largest f8 values
**          were being multiplied by 100.00 before this check, resulting in a
**          floating point overflow fault. 
**	26-aug-87 (thurston)
**	    Fixed bug in adu_cpmnytostr() with copying money to text/varchar ...
**	    last character was getting dropped.
**	31-aug-87 (thurston)
**	    Fixed adu_8mny_divu() to signal EXMNYDIV if the dividend was 0.0.
**	    Previously, this was not being done, so EXFLTDIV was occuring which
**	    is handled differently.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	19-aug-92 (stevet)
**	    Do not call STprintf when number is small.  This cause problem
**	    becasue F_FORMAT of CVfa() in some UNIX boxes return '******' if
**	    number underflow.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      14-sep-1993 (stevet)
**          Changed adu_8mny_divu() to support DB_DEC_TYPE.
**	01-sep-1994 (mikem)
**	    bug #64261
**	    Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug 
**	    #64261, in other files garbage return values from fabs would cause
**	    other problems.
**	28-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42:
**	    03-nov-93 (twai)
**	        Fix adu_2strtomny() to accept doublebyte trailing money unit.
**	        This is particularly obvious in sql-copy.  Replace the pre-
**	        and post- increment/decrement operators (++ & --) with
**		CM rotuines.
**	24-jul-95 (chimi03)
**	    For hp8_us5 doublebyte, added the NO_DBLOPTIM comment.
**          With optimization, adumoney.c rejects the $ as an invalid
**          character when entered as part of the data value, (i.e. $10.00).
**          When optimization is removed, the field is validated correctly.
**      13-feb-96 (thoda04)
**          Define return type of DB_STATUS for adu_2mnytomny().
**          Default of int is not good (especially 16 bit Window 3.1 systems).
**      25-jun-99 (schte01)
**          Changed assignment of data to MEcopy to avoid unaligned access.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-apr-04 (drivi01)
**		Added routines to handle money coercions to/from unicode in
**      functions adu_9mnytostr and adu_2strtomny.
**/

/*
[@forward_type_references@]
*/


/*
**  Definition of static variables and forward static functions.
*/

static	i4	    dum1;	/* Dummy to give to ult_check_macro() */
static	i4	    dum2;	/* Dummy to give to ult_check_macro() */

/* Convert numbers to a double */
static	STATUS	    ad0_makedbl(ADF_CB          *adf_scb,
                                DB_DATA_VALUE   *num_dv,
                                f8              *money);

/*
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/



/*
** Name: adu_11mny_round() - Round an internal money value to 2 decimal places.
**
** Description:
**	  This routine rounds an internal money value to the nearest cent.
**	If the fractional value of the money is >= 0.50, then the money is
**	rounded up to the nearest cent; otherwise it is truncated.
**
**	Given an integer large enough to hold the integer part of the
**	money (i.e. an i8), the simplest way to round is to convert to
**	integer (truncating);  compute val-intval, thus getting the fraction
**	part;  checking the fraction to see if it's >= 0.5, and if so,
**	incrementing the integer;  and converting the integer back to
**	float.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      a                               AD_MONEYNTRNL struct.
**	    .mny_cents			The money value to be rounded.
**
** Outputs
**	a
**	    .mny_cents			The resultant rounded money value.
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
**
**	Exceptions:
**	    none
**
** History:
**	18-may-86 (ericj)
**	    Converted for Jupiter.  It would be nice to rewrite this routine
**	    using MHfdint() instead of converting money to a string.
**	    Compatibility issues have to be looked into before this can be done.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	29-apr-87 (thurston)
**	    Do min/max money check at start of routine, so that STprintf()
**	    doesn't get called with something it can not format.
**	19-aug-92 (stevet)
**	    Do not call STprintf when number is small.  This cause problem
**	    becasue F_FORMAT of CVfa() in some UNIX boxes return '******' if
**	    number underflow.
**	28-apr-94 (vijay)
**	    change abs to fabs.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug 
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
**	16-Feb-2005 (schka24)
**	    Rounding by conversion to ascii is insane, if understandable
**	    in an i8-less world.  Revise to work via integer intermediates.
**	04-Oct-2005 (toumi01) BUG 115330
**	    Avoid rounding errors because of float approximation by adding or
**	    subtracting a fuzz factor.
*/

DB_STATUS
adu_11mny_round(
ADF_CB          *adf_scb,
AD_MONEYNTRNL	*a)
{
    f8 fval, orig_fval;		/* abs-floating version of value */
    i8 ival;			/* integer version of same */


#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
    {
        TRdisplay("round_money: internal money(actual X100) before\n");
        TRdisplay("         rounding=%26.8f\n", a->mny_cents);
    }
#   endif

    orig_fval = fval = (a->mny_cents < 0)
	? a->mny_cents-AD_MONEY_FUZZ : a->mny_cents+AD_MONEY_FUZZ;

    /* First, check for money overflow */
    if (fval >= (AD_1MNY_MAX_NTRNL + 0.5))
	return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
    if (fval <= (AD_3MNY_MIN_NTRNL - 0.5))
	return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));

    if (fval < 0) fval = -fval;		/* Take absolute value */
    ival = (i8) fval;
    fval = fval - (f8) ival;		/* Compute fractional part */
    if (fval >= 0.5) ++ival;
    fval = (f8) ival;			/* Rounded result */
    if (orig_fval < 0 && ival != 0) fval = -fval;  /* reapply sign */
    a->mny_cents = fval;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
    {
        TRdisplay("round_money: internal money(actual X100) after\n");
        TRdisplay("         rounding=%26.8f\n", a->mny_cents);
    }
#   endif

    return (E_DB_OK);
}



/*{
** Name: adu_3mny_absu()	- get the absolute value of money.
**
** Description:
**        This routine implements the function instance that will get the
**	absolute value of a money value.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	dv1				DB_DATA_VALUE describing the operand
**					data value.
**	    .db_data			Ptr to a MNYNTRNL struct.
**		.mny_cents		The money value to get the absolute
**					value of.
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
**      rdv                             DB_DATA_VALUE describing the result
**					data value.
**	    .db_data			Ptr to the result MNYNTRNL struct.
**		.mny_cents		The absolute value money result.
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
**	Exceptions:
**	    none
**
** History:
**	12-may-86 (ericj)
**          Converted for Jupiter.
[@history_line@]...
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
*/

DB_STATUS
adu_3mny_absu(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("abs_money: input money value=%24.6f\n", 
		  ((AD_MONEYNTRNL *) dv1->db_data)->mny_cents);
#   endif

    if (((AD_MONEYNTRNL *) dv1->db_data)->mny_cents < 0.0)
    {
	((AD_MONEYNTRNL *) rdv->db_data)->mny_cents =
			-((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;        
    }
    else
    {
	((AD_MONEYNTRNL *) rdv->db_data)->mny_cents =
		((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;
    }

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("abs_money: absolute money value=%24.6f\n",
		  ((AD_MONEYNTRNL *) rdv->db_data)->mny_cents);
#   endif

    return (E_DB_OK);
}


/*
** Name: adu_4mny_addu() -	add two money values.
**
** Description:
**	  This routine adds to money values, placing the result in a money
**	money value.
**
**  Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing one of the
**					additive money operands.
**	    .db_data			Ptr to internal money datatype.
**		.mny_cents		The actual money value.
**	dv2				DB_DATA_VALUE describing the other
**					additive money oprand.
**	    .db_data			Ptr to internal money datatype.
**		.mny_cents			The actual money value.
**
**  Outputs:
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
**	sum_dv				DB_DATA_VALUE which will describe the
**					sum result.
**	    .db_data			Ptr to internal money datatype.
**		.mny_cents		The money sum result.
**
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Completed successfully.
**
**  Exceptions:
**	none
**
**  History:
**	9-may-86 (ericj)
**	    Rewritten for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
*/

DB_STATUS
adu_4mny_addu(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*dv1,
DB_DATA_VALUE	*dv2,
DB_DATA_VALUE	*sum_dv)
{

    ((AD_MONEYNTRNL *) sum_dv->db_data)->mny_cents =
	((AD_MONEYNTRNL *) dv1->db_data)->mny_cents +
	((AD_MONEYNTRNL *) dv2->db_data)->mny_cents;

    /*
    ** Check for money overflow.  If an overflow has occured, return the proper
    ** error code.  (Note:  MONEY OVERFLOW no longer signals an exception.)
    */
    if (((AD_MONEYNTRNL *) sum_dv->db_data)->mny_cents > AD_1MNY_MAX_NTRNL)
	return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
    else if (((AD_MONEYNTRNL *) sum_dv->db_data)->mny_cents < AD_3MNY_MIN_NTRNL)
	return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));

    return (E_DB_OK);
}


/*{
** Name: adu_2mnytomny	- convert a money data value into a money data value.
**
** Description:
**        This routine converts a money data value into a money data value.
**	This is basically a copy of the data.  It will be used primarily
**	to copy a money data value into a tuple buffer.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	mny_dv1				DB_DATA_VALUE describing the source
**					money data value.
**	    .db_data			Ptr to a AD_MONEYNTRNL struct.
**		.mny_cents		The actual money data value.
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
**      mny_dv2                         DB_DATA_VALUE describing the destination
**					money data value.
**	    .db_data			Ptr to AD_MONEYNTRNL struct to be copied
**					into.
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
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-jul-86 (ericj)
**          Initial creation
[@history_line@]...
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
*/
DB_STATUS
adu_2mnytomny(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *mny_dv1,
DB_DATA_VALUE	    *mny_dv2)
{
    MEcopy( (PTR) mny_dv1->db_data, mny_dv2->db_length,
	    (PTR) mny_dv2->db_data);
    return (E_DB_OK);
}


/*
** Name: adu_7mny_divf() - ADF interface for dividing a number by a
**			   money; result is a number (f8).
**
** Description:
**	  This routine provides an ADF interface for dividing a number by a 
**	money value.  The result is an f8.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the number to
**					be divided.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to the number to be divided.
**	dv2				DB_DATA_VALUE describing the money
**					divisor.
**	    .db_data			AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value.
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
**	rdv				DB_DATA_VALUE describing the result.
**					It is assumed this is going to be of
**					type float with a length of 8.
**	    .db_data			Ptr to result data area.
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
**
**	Exceptions:
**	    EXMNYDIV			Attempt to divide by 0.
**
** History:
**	18-may-86 (ericj)
**	    Rewrote for Jupiter.  Eliminated call to support routine div_money()
**	    for clarity and performance.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	10-jun-87 (thurston)
**	    Fixed this routine to return an f8; it had been returning a money.
**      10-May-2010 (horda03) B123704
**          Changed ad0_makedbl() interface to handle strings being passed to
**          Mul/Div code.
*/

DB_STATUS
adu_7mny_divf(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    DB_STATUS       status;
    f8		    dd_num;
    f8		    dd_mny;


    status = ad0_makedbl(adf_scb, dv1, &dd_num);

    if (!status)
    {
       dd_mny = ((AD_MONEYNTRNL *) dv2->db_data)->mny_cents / 100.0;

       if (dd_mny == 0.0)
       {
	   EXsignal(EXMNYDIV, 0);
	   /* if the exception is returned from, a zero answer on divide by zero
	   ** seems reasonable.
	   */
	   *(f8 *)rdv->db_data = 0.0;
       }
       else
       {
	   *(f8 *) rdv->db_data = dd_num / dd_mny;
       }
    }
    
    return (status);
}


/*
** Name: adu_8mny_divu() - ADF function instance for dividing two money
**			   values or a money by a number; result is a money.
**
** Description:
**	  This routine implements the ADF function instance for dividing two
**	money data values or a money data value by a number.  The result is
**	money.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	dv1				DB_DATA_VALUE describing the money
**					dividend.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value.
**	dv2				DB_DATA_VALUE describing the
**					divisor.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to either a AD_MONEYNTRNL struct
**					or some numeric datatype.
**	        [.mny_cents]		The actual money value if datatype
**					is DB_MNY_TYPE.
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
**	rdv				DB_DATA_VALUE describing the result
**					It is assumed this is a money type.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value.
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
**	    E_AD0000_OK			    Completed successfully.
**
**	Exceptions:
**	    EXMNYDIV			    Attempt to divide money by 0.
**
** History:
**	16-may-86 (ericj)
**	    Rewritten, yet again, for Jupiter.  Updated so that all work
**	    done internally to this routine eliminating div_money().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	31-aug-87 (thurston)
**	    Fixed this routine to signal EXMNYDIV if the dividend was 0.0.
**	    Previously, this was not being done, so EXFLTDIV was occuring which
**	    is handled differently.
**      14-sep-1993 (stevet)
**          Added support for DB_DEC_TYPE.
**      10-May-2010 (horda03) B123704
**          Changed ad0_makedbl() interface to handle strings being passed to
**          Mul/Div code. Both parametrs could be non-MONEY types.
*/

DB_STATUS
adu_8mny_divu(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    DB_STATUS	    db_stat;
    f8		    mny_1;
    f8		    mny_2;


    if (dv1->db_datatype == DB_MNY_TYPE)
    {
        mny_1 = ((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;
    }
    else
    {
        db_stat = ad0_makedbl(adf_scb, dv1, &mny_1);

        if (db_stat)
        {
           return db_stat;
        }

        mny_1 *= 100.0;
    }

    if (dv2->db_datatype == DB_MNY_TYPE)
    {
	mny_2 = ((AD_MONEYNTRNL *) dv2->db_data)->mny_cents;
    }
    else
    {
        db_stat = ad0_makedbl(adf_scb, dv2, &mny_2);

        if (db_stat)
        {
           return db_stat;
        }

        mny_2 *= 100.0;
    }

    if (mny_2 == 0.0)
    {
	EXsignal(EXMNYDIV, 0);
	/* if the exception is returned from, a zero answer on divide by zero
	** seems reasonable.
	*/
	((AD_MONEYNTRNL *) rdv->db_data)->mny_cents = 0.0;
	db_stat = E_DB_OK;
    }
    else
    {
	((AD_MONEYNTRNL *) rdv->db_data)->mny_cents = mny_1 / mny_2 * 100.0;

	db_stat = adu_11mny_round(adf_scb, (AD_MONEYNTRNL *) rdv->db_data);
    }

    return (db_stat);
}



/*
** Name: adu_1mnytonum() -  convert internal money to numeric representation.
**
** Description:
**	  This routine converts an internal money representation to an
**	external money representation(dollars) and coerces the result
**	into an f4, f8, or decimal datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      mny_dv                          DB_DATA_VALUE describing money value
**					to be converted.
**	    .db_data			AD_MONEYNTRNL struct.
**		[.mny_cents]		The actual money value to be converted.
**	num_dv				DB_DATA_VALUE describing result.
**	    .db_prec			Precision/Scale of result
**	    .db_length			Length of result
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
**	num_dv
**	    .db_data			Ptr to area to store the result in.
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
**	    E_AD0000_OK			Successful completion.
**
** History:
**	17-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-jul-89 (jrb)
**	    Extended to work on decimal too (renamed from mnytoflt to mnytonum).
**      25-jun-99 (schte01)
**          Changed assignment of data to MEcopy to avoid unaligned access.
*/

DB_STATUS
adu_1mnytonum(
ADF_CB		*adf_scb,
DB_DATA_VALUE   *mny_dv,
DB_DATA_VALUE   *num_dv)
{
    switch (num_dv->db_datatype)
    {
      case DB_DEC_TYPE:
	if (CVfpk((f8)(((AD_MONEYNTRNL *)mny_dv->db_data)->mny_cents / 100.0),
		    (i4)DB_P_DECODE_MACRO(num_dv->db_prec),
		    (i4)DB_S_DECODE_MACRO(num_dv->db_prec),
		    (PTR)num_dv->db_data) == CV_OVERFLOW)
	{
	    EXsignal(EXDECOVF, 0);
	}
	break;
	
      case DB_FLT_TYPE:
	if (num_dv->db_length == 8)
	{
#ifdef LP64
         f8 i;
	 i = ((AD_MONEYNTRNL *)mny_dv->db_data)->mny_cents / 100.0;
         MEcopy(&i,sizeof(f8),num_dv->db_data);
#else
	    *(f8 *)num_dv->db_data =
			((AD_MONEYNTRNL *)mny_dv->db_data)->mny_cents / 100.0;
#endif

#	    ifdef xDEBUG
	    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_002_MNYFI_TRACE,
				&dum1, &dum2))
		TRdisplay("mnytonum: returned f8 value will=%24.8f\n",
			    *(f8 *)num_dv->db_data);
#	    endif
	}
	else
	{
#ifdef LP64
         f4 i;
	 i = ((AD_MONEYNTRNL *)mny_dv->db_data)->mny_cents / 100.0;
         MEcopy(&i,sizeof(f4),num_dv->db_data);
#else
	    *(f4 *)num_dv->db_data =
			((AD_MONEYNTRNL *)mny_dv->db_data)->mny_cents / 100.0;
#endif

					 

#	    ifdef xDEBUG
	    if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_002_MNYFI_TRACE,
				&dum1, &dum2))
		TRdisplay("mnytonum: returned f4 value will=%24.8f\n",
			    *(f4 *)num_dv->db_data);
#	    endif
	}
    }
    
    return (E_DB_OK);
}


/*
** Name: adu_9mnytostr() - Convert internal money to string representation for
**			   output.
**
** Description:
**	  Convert internal money to string representation for output.
**	Money is represented as a 20 character string with the format:
**	    " $sdddddddddddddd.dd"
**
**	where s is the sign ('+' is not printed for positive values)
**	      d is a digit between 0 - 9
**	      (Leading zeros are not printed).
**	      The '$' is printed just left of the first digit.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	dv1				DB_DATA_VALUE describing the money
**					value to be converted.
**	    .db_data			Ptr to a AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value.
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
**	str_dv				    DB_DATA_VALUE describing the result
**					    string.
**	    .db_datatype		    Its datatype, either "text" or
**					    "char".
**	    .db_length			    Its length.
**	    .db_data			    Either a ptr to an area to hold
**					    a "char" string or a ptr to a
**					    DB_TEXT_STRING struct.
**		[.db_t_count]		    If the result is a text string, this
**					    is its length.
**		[.db_t_text]		    And this ptr to the area which will
**					    hold the resulting string.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			    Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	    The datatype of str_dv was
**					    imcompatible for strings.
**
**	History:
**	    02/22/83 (lichtman)
**		Changed to work for both character type and new text type.
**	    15-may-86 (ericj)
**		Converted for Jupiter.
**
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring routine, 
**	    in order to allow for binary data to be copied untainted. Since the 
**	    character input to movestring was created in this routine itself 
**	    we pass in DB_NODT to this routine.
*/

DB_STATUS
adu_9mnytostr(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*mny_dv,
DB_DATA_VALUE	*str_dv)
{
	register AD_MONEYNTRNL	*m;
	i4			i,
				n1,
				prec;
	char			*fmt;
	char			temp[AD_5MNY_OUTLENGTH + 1];
	char			decimal = (adf_scb->adf_decimal.db_decspec
				    ? (char) adf_scb->adf_decimal.db_decimal
				    : '.');
	DB_DATA_VALUE	str2_dv;
	DB_STATUS	db_stat;
	//char data[ AD_5MNY_OUTLENGTH + 1 ];


      m = (AD_MONEYNTRNL *) mny_dv->db_data;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("mnytostr: internal money(actual X100)=%20.2f\n",
		  m->mny_cents);
#   endif


    /* determine output format for money string */

    prec = adf_scb->adf_mfmt.db_mny_prec;
  
    if (prec == 2)
        fmt = "%20.2f";
    else if (prec == 1)
        fmt = "%20.1f";
    else
        fmt = "%20.0f";

    /* 
    ** Elaborate special case for 0.0 zapped for simplicity.
    */
    STprintf(temp, fmt, (m->mny_cents == 0.0 ? 0.0 : m->mny_cents / 100.0),
             decimal);

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("mnytostr:after sprintf to temp, temp=%-25s\n", temp);
#   endif

    /* 
    ** Scan for 1st non-blank character, and place the money symbol ('$') 
    */

    for ( i = 0; temp[i] == ' '; i++)
        continue;

    n1 = STlength(adf_scb->adf_mfmt.db_mny_sym);
    if (i < n1)
    {
        /*
        ** There is no room for the currency symbol, this is wrong,
        ** but don't syserr *OR* place the symbol on top of a number.
	** This was not modified for Jupiter because of the compatibility
	** issues it might raise with users applications.
        */
    }
    else 
    {
        /*
        ** there is room for the currency symbol.
        */

        if (adf_scb->adf_mfmt.db_mny_lort == DB_LEAD_MONY)
        {
            MEcopy((PTR)adf_scb->adf_mfmt.db_mny_sym,
		   n1,
		   (PTR)&temp[i-n1]);
        }
        else
        {
	    /*
	    ** Trailing currency symbol, move number to make it fit.
	    ** NOTE: changed the number of chars to be moved from 
	    ** AD_5MNY_OUTLENGTH - n1 to AD_5MNY_OUTLENGTH - i.  The first
	    ** length worked because there was a do nothing buffer allocated
	    ** after temp which kept good data from being overwritten.
	    */
            MEcopy((PTR)&temp[i],
		   AD_5MNY_OUTLENGTH - i,
		   (PTR)&temp[i-n1]);
            MEcopy((PTR)adf_scb->adf_mfmt.db_mny_sym,
		   n1,
		   (PTR)&temp[AD_5MNY_OUTLENGTH-n1]);
        }
    }

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("mnytostr: external money value=%-20s\n", temp);
#   endif

    /* changed second param in movestring from STlength(temp) */
	switch (str_dv->db_datatype)
    {
	  case DB_NCHR_TYPE:
	  case DB_NVCHR_TYPE:
		if (str2_dv.db_datatype = DB_NVCHR_TYPE)
			str2_dv.db_length = (str_dv->db_length - DB_CNTSIZE)/sizeof(UCS2) + DB_CNTSIZE;
		else
			str2_dv.db_length = str_dv->db_length/sizeof(UCS2);
		str2_dv.db_data = (PTR)MEreqmem(0, str2_dv.db_length, TRUE, NULL);
		//str2_dv.db_data = &data;
		//MEfill(AD_5MNY_OUTLENGTH + 1, NULLCHAR, (PTR)&data);	
		if (str_dv->db_datatype == DB_NCHR_TYPE)
			str2_dv.db_datatype = DB_CHA_TYPE;	
		else if (str_dv->db_datatype == DB_NVCHR_TYPE)
			str2_dv.db_datatype = DB_VCH_TYPE;
		adu_movestring(adf_scb, (u_char *)temp, AD_5MNY_OUTLENGTH, DB_NODT, &str2_dv);
		db_stat = adu_nvchr_coerce(adf_scb,  &str2_dv, str_dv);
		MEfree((PTR)str2_dv.db_data);
		return db_stat;
		break;

	  default:
		return (adu_movestring(adf_scb, (u_char *)temp, AD_5MNY_OUTLENGTH, DB_NODT, str_dv));
	}
    //return (adu_movestring(adf_scb, (u_char *)temp, AD_5MNY_OUTLENGTH, str_dv));
}


/*
** Name: adu_5mnyhmap() - Builds a histogram element for a money data value.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	from_dv				DB_DATA_VALUE describing money data
**					to be used in making a histogram value.
**	    .db_data			Ptr to a AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value.
**	hg_dv				DB_DATA_VALUE describing the histogram
**					value to be made.  It is assumed that
**					this will be of type integer with length
**					of 4.
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
**	hg_dv
**	    *.db_data			An i4 histogram value approximately
**					equal to the money's dollar value.
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
**
** History:
**	18-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
*/

DB_STATUS
adu_5mnyhmap(
ADF_CB          *adf_scb,
DB_DATA_VALUE	*from_dv,
DB_DATA_VALUE	*hg_dv)
{
    AD_MONEYNTRNL  *mny_val;
    f8		    dollar_val;

    mny_val = (AD_MONEYNTRNL *) from_dv->db_data;

    /* convert to number of dollars. */
    dollar_val = mny_val->mny_cents / 100.0;
    if (dollar_val > MAXI4)
	*(i4 *) hg_dv->db_data = MAXI4;
    else if (dollar_val < MINI4)
        *(i4 *) hg_dv->db_data = MINI4;
    else
	*(i4 *) hg_dv->db_data = dollar_val;
    
    return (E_DB_OK);
}


/*
** Name: adu_10mny_multu() - ADF function instance for multiplying money.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing one of the
**					multiplicands.
**	    .db_datatype		Its type.
**	    .db_length			Its length.
**	    .db_data			Ptr to either a AD_MONEYNTRNL struct or
**					a number.
**		[.mny_cents]		The actual money value if dv1's type
**					is money.
**	dv2				DB_DATA_VALUE describing the other
**					mulitiplicand.  All of the fields
**					are as described above.
**	    .db_datatype
**	    .db_length
**	    .db_data
**		[.mny_cents]
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
**	rdv				DB_DATA_VALUE describing the result
**					data value.  It is assumed that this
**					is of type money.
**	    .db_data			AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value result.
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
**
**	Exceptions:
**	    EXMNYDIV			The result exceeded either the minimum
**					or maximum possible values for money.
**
** History:
**	18-may-86 (ericj)
**	    Rewrote for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**      10-May-2010 (horda03) B123704
**          Changed ad0_makedbl() interface to handle strings being passed to
**          Mul/Div code.
*/

DB_STATUS
adu_10mny_multu(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    DB_STATUS       status;
    f8		    dv1_f8;
    f8		    dv2_f8;


    /* if dv1 is not a money type, convert to a f8 before multiplying */
    if (dv1->db_datatype != DB_MNY_TYPE)
    {
        status = ad0_makedbl(adf_scb, dv1, &dv1_f8);

        if (status)
        {
            return status;
        }

        dv1_f8 *= 100.0;

#       ifdef xDEBUG
        if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_002_MNYFI_TRACE,
			    &dum1, &dum2))
            TRdisplay("multu_money: after ad0_makedbl for a, dbl_a=%24.6f\n",
			dv1_f8);
#       endif
    }
    else
    {
        /* dv1 is money type */
        dv1_f8 = ((AD_MONEYNTRNL *) dv1->db_data)->mny_cents;
    }

    /* if dv2 is not a money type, convert to a f8 before multiplying */
    if (dv2->db_datatype != DB_MNY_TYPE)
    {
        status = ad0_makedbl(adf_scb, dv2, &dv2_f8);

        if (status)
        {
            return status;
        }

        dv2_f8 *= 100.0;

#       ifdef xDEBUG
        if (ult_check_macro(&Adf_globs->Adf_trvect, ADF_002_MNYFI_TRACE,
			    &dum1, &dum2))
            TRdisplay("multu_money: after ad0_makedbl for b, dbl_b=%24.6f\n",
			dv2_f8);
#       endif
    }
    else
    {
        /* dv2 is money type */
	dv2_f8 = ((AD_MONEYNTRNL *) dv2->db_data)->mny_cents;
    }

    ((AD_MONEYNTRNL *) rdv->db_data)->mny_cents = dv1_f8 * dv2_f8 / 100.0;

    return (adu_11mny_round(adf_scb, (AD_MONEYNTRNL *) rdv->db_data));
}



/*
** Name: adu_numtomny() - Convert number to internal money representation.
**
**  Description:
**	All money fields are stored as doubles, rounded to two decimal places.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      num_dv                          DB_DATA_VALUE describing the number to
**					be converted to money.
**	    .db_datatype		Its datatype, some kind of number.
**	    .db_prec			Its precision/scale
**	    .db_length			Its length.
**	    .db_data			Ptr to the number.
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
**	mny_dv				DB_DATA_VALUE describing the money
**					result.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The converted money value.
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
**
**	Exceptions:
**	    none
**
** History:
**	19-may-86 (ericj)
**	    Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	10-jun-87 (thurston)
**	    Greatly simplified by using a call to adu_11mny_round().
**	05-aug-87 (thurston)
**	    Now catches max/min money overflow on even the largest f8 values.
**	    Previously, the largest f8 values were being multiplied by 100.00
**	    before this check, resulting in a floating point overflow fault.
**	14-jul-89 (jrb)
**	    Extended for decimal datatype.
**      10-May-2010 (horda03) B123704
**          Changed ad0_makedbl() interface to handle strings being passed to
**          Mul/Div code.
*/

DB_STATUS
adu_numtomny(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*num_dv,
DB_DATA_VALUE	*mny_dv)
{
    DB_STATUS       status;
    f8		    dollars;


    status = ad0_makedbl(adf_scb, num_dv, &dollars);

    if (status)
    {
        return status;
    }

    /*
    ** Now, check for money overflow ... do it here in case dollars * 100.00
    ** would cause floating point overflow, which we do not want to incur.
    */
    if (dollars >= (AD_2MNY_MAX_XTRNL + 0.5))
	return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
    if (dollars <= (AD_4MNY_MIN_XTRNL - 0.5))
	return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));

    ((AD_MONEYNTRNL *)mny_dv->db_data)->mny_cents = dollars * 100.0;

    return (adu_11mny_round(adf_scb, (AD_MONEYNTRNL *) mny_dv->db_data));

}


/*
** Name: adu_2strtomny() - This is the function used by the resovler to convert
**                         a "money" string into internal representation. It
**                         is a wrapper for adu_2strtomny_strict() and passes
**                         FALSE in the strict parameter.
**
** Description:
**	 Used by the resolver. Is a wrapper for adu_2strtomny_strict().
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      str_dv                          DB_DATA_VALUE describing the string
**					data value to be converted.
**	    .db_datatype		Its type, character or text string.
**	    .db_length			Its length.
**	    .db_data			If type is character, a ptr to the
**					character string; else type is text
**					and this is a ptr to a DB_TEXT_STRING.
**		[.db_t_count]		Length of a "text" string.
**		[.db_t_text]		Ptr to the "text" string.
**	adf_scb				ADF control block which contains
**					money format description.
**	    .adf_decimal		DB_DECIMAL struct.
**		.db_decspec		TRUE if decimal character is specified
**					in .db_decimal.  If FALSE, '.' will be
**					used as the decimal character.
**              .db_decimal             If .db_decspec is TRUE, then this is
**					the decimal character to use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_mfmt			DB_MONY_FMT struct.
**		.db_mny_sym		Ptr to the money symbol to be used.
**		.db_mny_lort		Is the money symbol suppossed to be
**					leading or trailing.
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
**	mny_dv				DB_DATA_VALUE describing the money
**					value to be assigned.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The money value result.
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
**	    E_AD5020_BADCH_MNY		An illegal character was found in a
**					money string or a legal money string
**					character was found in the wrong 
**					position in the string.
**	    E_AD5021_MNY_SIGN		A sign character in the money string
**					either occured in the wrong position
**					or occured twice.
**	    E_AD5022_DECPT_MNY		A decimal point character occured more
**					than once in the money string.
**
**	Exceptions:
**	    none
**
** History:
**	11-Aug-2010 (hanal04) Bug 124180
**	    Modified to become a wrapper for adu_2strtomny_strict()
*/

DB_STATUS
adu_2strtomny(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*str_dv,
DB_DATA_VALUE	*mny_dv)
{
    return(adu_2strtomny_strict(adf_scb, str_dv, mny_dv, FALSE));
}

/*
** Name: adu_2strtomny_strict() - Convert string to internal money 
**                         representation. All money fields are stored as 
**                         doubles, rounded to the nearest cent.
**                         
** WARNING: MUST NOT BE REFERENCED BY THE RESOLVER. NON-STANDARD ARGUMENTS.
**
** Description:
**	  This routine converts a "money" string to an internal money
**	representation.  All money fields are stored as doubles, rounded to
**	the nearest cent.  Valid string input formats are:
**
**	    "[MONEY_SYMBOL][sign]{blanks}{digits}{decimal_char}{digits}"
**
**	The resulting money value is rounded up if the third digit
**	after the "international decimal char" is greate than or equal to '5'.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      str_dv                          DB_DATA_VALUE describing the string
**					data value to be converted.
**	    .db_datatype		Its type, character or text string.
**	    .db_length			Its length.
**	    .db_data			If type is character, a ptr to the
**					character string; else type is text
**					and this is a ptr to a DB_TEXT_STRING.
**		[.db_t_count]		Length of a "text" string.
**		[.db_t_text]		Ptr to the "text" string.
**	adf_scb				ADF control block which contains
**					money format description.
**	    .adf_decimal		DB_DECIMAL struct.
**		.db_decspec		TRUE if decimal character is specified
**					in .db_decimal.  If FALSE, '.' will be
**					used as the decimal character.
**              .db_decimal             If .db_decspec is TRUE, then this is
**					the decimal character to use.
**					    ***                            ***
**					    *** NOTE THAT THIS IS A "nat", ***
**					    *** NOT A "char".              ***
**					    ***                            ***
**	    .adf_mfmt			DB_MONY_FMT struct.
**		.db_mny_sym		Ptr to the money symbol to be used.
**		.db_mny_lort		Is the money symbol suppossed to be
**					leading or trailing.
**	strict				Boolean which indicates whether a
**                                      strict money format is required.
**                                      (i.e. correct currency symbol in the
**                                      correct position).
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
**	mny_dv				DB_DATA_VALUE describing the money
**					value to be assigned.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The money value result.
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
**	    E_AD5020_BADCH_MNY		An illegal character was found in a
**					money string or a legal money string
**					character was found in the wrong 
**					position in the string.
**	    E_AD5021_MNY_SIGN		A sign character in the money string
**					either occured in the wrong position
**					or occured twice.
**	    E_AD5022_DECPT_MNY		A decimal point character occured more
**					than once in the money string.
**
**	Exceptions:
**	    none
**
** History:
**	19-may-86 (ericj)
**	    Converted for Jupiter.  Removed formatting setup code; this
**	    should already have been done by adg_init()
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	02-sep-2005 (gupsh01)
**	    Fixed money to unicode coercion case.
**      11-Aug-2010 (hanal04) Bug 124180
**          Copied from adu_2strtomny() with added paramter 'strict'.
**          If strict == TRUE we return a failure if this is not an
**          explicit money string.
*/

DB_STATUS
adu_2strtomny_strict(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*str_dv,
DB_DATA_VALUE	*mny_dv,
bool		strict)
{
    register char	*p;
    char		*ptemp;
    AD_MONEYNTRNL		mn;
    DB_STATUS           db_stat = E_DB_OK;
    i4			l;
    i4			innumber,
			indecimal;
    i4			places;
    i4			got_dollar,
			got_number,
			got_sign,
			got_decpt;
    char		insign;
    char		*currency_mask;
    char		decimal = (adf_scb->adf_decimal.db_decspec ?
				(char) adf_scb->adf_decimal.db_decimal : '.');
    DB_DATA_VALUE	str2_dv;
    DB_DATA_VALUE	data[ AD_5MNY_OUTLENGTH + 1 ];
    /*
    ** Parse an input string into a legal money value
    */
    if (str_dv->db_datatype == DB_NCHR_TYPE
	|| str_dv->db_datatype == DB_NVCHR_TYPE)
    {
	if (str_dv->db_datatype == DB_NCHR_TYPE)
	{
	  str2_dv.db_datatype = DB_CHA_TYPE;	
 	  str2_dv.db_length = str_dv->db_length/sizeof(UCS2);
	}
	else if (str_dv->db_datatype == DB_NVCHR_TYPE)
	{
	  str2_dv.db_datatype = DB_VCH_TYPE;	
	  str2_dv.db_length = 
		(str_dv->db_length - DB_CNTSIZE)/sizeof(UCS2) 
		+ DB_CNTSIZE;
	}
	MEfill(AD_5MNY_OUTLENGTH + 1, NULLCHAR, (PTR)&data);
	str2_dv.db_data = (PTR) &data;
	str2_dv.db_prec = str_dv->db_prec;

	adu_nvchr_coerce(adf_scb,  str_dv, &str2_dv);
        if ((db_stat = adu_lenaddr(adf_scb, &str2_dv, &l, &ptemp)) != E_DB_OK)
	   return (db_stat);
    }
    else if ((db_stat = adu_lenaddr(adf_scb, str_dv, &l, &ptemp)) != E_DB_OK)
       return (db_stat);

    p = ptemp;

    innumber    = 0;
    indecimal   = 0;
    insign      = '+';
    places      = 0;

    /* initialize flags for error checking */

    got_dollar  = 0;
    got_sign    = 0;
    got_decpt   = 0;
    got_number  = 0;

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("strtomoney: length of string passed=%d\n", l);
#   endif

    /* all blank or null strings will give zero dollars */

    mn.mny_cents  = 0.0;

    while (l)		/* while there're still money string chars to process */
    {
        if (CMdigit(p))
        {
            if (got_number)
		return (adu_error(adf_scb, E_AD5020_BADCH_MNY, 0));

            if (innumber)
            {
                if (indecimal)
                {
		    /* if 2 or less fractional digits have been processed */
                    if (++places <= 2)
                    {
                        mn.mny_cents = mn.mny_cents* 10.0 + (*p - '0');
                    }
                    else
                    {
                        if ( (places == 3) && (*p >='5') )
                        {
                            mn.mny_cents = mn.mny_cents + AD_7MNY_ROUND_VAL;
                        }
                    }
                }
                else
                {
                    mn.mny_cents = mn.mny_cents * 10.0 + (*p-'0');
                }
            }
            else
            {
                mn.mny_cents = *p - '0';

                if (indecimal)
                {
                    places++;
                }

                innumber = 1;
            }

            /* check if max value exceeded */

            if (mn.mny_cents > AD_1MNY_MAX_NTRNL)
            {
                if (insign == '+')
		    return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
                else
		    return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));
            }

        }
        else if (*p == ' ')
        {
            if (indecimal || innumber)
                got_number = 1;
        }
        else if ((*p == '+') || (*p == '-'))
        {
            if (got_sign)
            {
                return (adu_error(adf_scb, E_AD5021_MNY_SIGN, 0));
            }
            else
            {
                if (indecimal || innumber)
		{
                    return (adu_error(adf_scb, E_AD5021_MNY_SIGN, 0));
		}
                else
                {
                    got_sign    = 1;
                    insign      = *p;
                }
            }
        }
        else if (*p == decimal)
        {
            if (got_decpt)
            {
		return (adu_error(adf_scb, E_AD5022_DECPT_MNY, 0));
            }
            else
            {
                got_decpt = 1;
                indecimal = 1;
            }
        }
        else
        {
            /*
            ** Check for currency symbol or illegal character.
            ** Currency symbols may include any character except
            ** numbers, decimal indicator, plus, minus. Leading
            ** and trailing blanks are ignored.
            */
            if (innumber)
                got_number = 1;

            currency_mask = adf_scb->adf_mfmt.db_mny_sym;
            while (*currency_mask == ' ')
                currency_mask++;
            if (got_dollar)
                return (adu_error(adf_scb, E_AD5020_BADCH_MNY, 0));
            for (;;)
            {
		char	ctmp1;
		char	ctmp2;
		
		CMtolower(p, &ctmp1);
		CMtolower(currency_mask, &ctmp2);
                if (ctmp1 == ctmp2)
                {
		    CMnext(currency_mask);
		    while (CMspace(currency_mask))
			CMnext(currency_mask);
                    if (*currency_mask == '\0')
                    {
                        got_dollar = 1;
                        break;
                    }
		    CMbytedec(l, p);
		    CMnext(p);
                }
                else
                {
                    return (adu_error(adf_scb, E_AD5020_BADCH_MNY, 0));
                }
            }
        }
	CMbytedec(l, p);
	CMnext(p);
    }

    if(strict && !got_dollar)
        return (adu_error(adf_scb, E_AD5020_BADCH_MNY, 0));

    while (places < 2)
    {
        places++;

        mn.mny_cents = mn.mny_cents * 10.0;
    }

    if (insign == '-')
    {
        /* negative money value */

        mn.mny_cents =  -mn.mny_cents;
    }

    /* check if max or min values exceeded */


    if (mn.mny_cents > AD_1MNY_MAX_NTRNL)
	return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
    else if (mn.mny_cents < AD_3MNY_MIN_NTRNL)
	return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("strtomoney: internal money(actual X100)=%26.8f\n",
		    mn.mny_cents);
#   endif

    /* move converted money value to be returned */
    ((AD_MONEYNTRNL *) mny_dv->db_data)->mny_cents = mn.mny_cents;

    return (E_DB_OK);
}


/*
** Name: adu_12mny_subu() - ADF function instance for subtracting money.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the minuend
**					money operand.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The money value of the minuend.
**	dv2				DB_DATA_VALUE describing the subtrahend
**					money operand.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The money value of the subrahend.
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
**	rdv				DB_DATA_VALUE describing the money
**					result.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The resulting money value.
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
**
**	Exceptions:
**	    none
**
** History:
**	19-may-86
**	    Converted for Jupiter.  Put everything in-line and removed the
**	    routine sub_money().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
*/

DB_STATUS
adu_12mny_subu(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    ((AD_MONEYNTRNL *) rdv->db_data)->mny_cents =
	((AD_MONEYNTRNL *) dv1->db_data)->mny_cents -
	((AD_MONEYNTRNL *) dv2->db_data)->mny_cents;

    if (((AD_MONEYNTRNL *) rdv->db_data)->mny_cents > AD_1MNY_MAX_NTRNL)
	return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
    else if (((AD_MONEYNTRNL *) rdv->db_data)->mny_cents < AD_3MNY_MIN_NTRNL)
	return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));

    return (E_DB_OK);
}


/*
** Name: adu_posmny() - ADF function instance to do unary "+" money value.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the money
**					operand.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The money value.
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
**	rdv				DB_DATA_VALUE describing the money
**					result.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The resulting money value.
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
**
**	Exceptions:
**	    none
**
** History:
**      20-nov-86 (thurston)
**	    Initial creation.
*/

DB_STATUS
adu_posmny(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    ((AD_MONEYNTRNL *)rdv->db_data)->mny_cents =
				    ((AD_MONEYNTRNL *)dv1->db_data)->mny_cents;

    return (E_DB_OK);
}


/*
** Name: adu_negmny() - ADF function instance to negate a money value.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the money
**					operand to be negated.
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The money value.
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
**	rdv				DB_DATA_VALUE describing the negated
**					money value (i.e. the result).
**	    .db_data			Ptr to AD_MONEYNTRNL struct.
**		.mny_cents		The resulting money value.
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
**
**	Exceptions:
**	    none
**
** History:
**      20-nov-86 (thurston)
**	    Initial creation.
*/

DB_STATUS
adu_negmny(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    ((AD_MONEYNTRNL *)rdv->db_data)->mny_cents =
				    -((AD_MONEYNTRNL *)dv1->db_data)->mny_cents;

    return (E_DB_OK);
}


/*
** Name: adu_cpmnytostr() - convert money to an Ingres string type for COPY.
**
** Description:
**	  Convert internal money to string representation with RIGHT
**	justification for COPY.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	dv1				DB_DATA_VALUE describing the money
**					value to be converted.
**	    .db_data			Ptr to a AD_MONEYNTRNL struct.
**		.mny_cents		The actual money value.
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
**	str_dv				DB_DATA_VALUE describing the result
**					string.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Either a ptr to an area to hold
**					a "c" or "char" string or a ptr to a
**					DB_TEXT_STRING struct for "text",
**					"longtext" or "varchar".
**		[.db_t_count]		If the result is a DB_TEXT_STRING, this
**					is its length.
**		[.db_t_text]		And this ptr to the area which will
**					hold the resulting string.
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
**	    E_AD0000_OK			    Completed successfully.
**	    E_AD5001_BAD_STRING_TYPE	    The datatype of str_dv was
**					    imcompatible for strings.
**
**  History:
**      03-aug-87 (thurston)
**	    Created for COPY command.
**	26-aug-87 (thurston)
**	    Fixed bug with copying money to text/varchar ... last character was
**	    getting dropped.
*/

DB_STATUS
adu_cpmnytostr(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*mny_dv,
DB_DATA_VALUE	*str_dv)
{
    DB_STATUS		db_stat;
    DB_DATA_VALUE	tmp_dv;
    char		tbuf[AD_5MNY_OUTLENGTH];
    char		*tmp_ptr = &tbuf[0];
    i4			tmp_len = AD_5MNY_OUTLENGTH;
    char		*out_ptr;
    i4			out_len;


    tmp_dv.db_datatype = DB_CHR_TYPE;
    tmp_dv.db_length = AD_5MNY_OUTLENGTH;
    tmp_dv.db_data = (PTR) tmp_ptr;

    db_stat = adu_9mnytostr(adf_scb, mny_dv, &tmp_dv);
    if (DB_SUCCESS_MACRO(db_stat))
    {
	if (	str_dv->db_datatype == DB_VCH_TYPE
	    ||	str_dv->db_datatype == DB_TXT_TYPE
	    ||	str_dv->db_datatype == DB_LTXT_TYPE
	   )
	{
	    out_ptr = (char *) ((DB_TEXT_STRING *)str_dv->db_data)->db_t_text;
	    out_len = str_dv->db_length - DB_CNTSIZE;
	    ((DB_TEXT_STRING *)str_dv->db_data)->db_t_count = out_len;
	}
	else
	{
	    out_ptr = (char *) str_dv->db_data;
	    out_len = str_dv->db_length;
	}

	tmp_len = AD_5MNY_OUTLENGTH;
	while (*tmp_ptr == ' ')
	{
	    tmp_len--;
	    tmp_ptr++;
	}

	if (tmp_len < out_len)
	{
	    while (out_len > tmp_len)
	    {
		out_len--;
		*out_ptr++ = ' ';
	    }
	}
	else if (tmp_len > out_len)
	{
	    tmp_ptr += (tmp_len - out_len);
	}

	while (out_len--)
	    *out_ptr++ = *tmp_ptr++;
    }

    return (db_stat);
}


/*{
** Name: ad0_makedbl() - Convert number (string, f4, f8, dec, i1, i2, or i4) to a f8.
**
** Description:
**	  This routine converts a number, an f, dec, or an i, to an f8.  It
**	is used only as a support routine internally to adumoney.c.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      num_dv                          DB_DATA_VALUE describing the number to
**					be converted.
**	    .db_datatype		Its type.
**	    .db_prec			Its precision/scale
**	    .db_length			Its length.
**	    .db_data			Ptr to the data to be interpreted as
**					some kind of number dependent on its
**					type and length.
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
**      money                           Double coerced from input value.
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
** History:
**	20-may-86 (ericj)
**          Converted for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	14-jul-89 (jrb)
**	    Added decimal support.
**	12-apr-04 (inkdo01)
**	    Added support for BIGINT (i8).
**      10-May-2010 (horda03) b123704
**          Convert string data types.
**	02-Dec-2010 (gupsh01) SIR 124685
**	    Protype cleanup.
*/

static	DB_STATUS
ad0_makedbl(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *num_dv,
f8              *money)
{
    DB_STATUS status = E_DB_OK;

    if (num_dv->db_datatype == DB_FLT_TYPE)
    {
        if  (num_dv->db_length == 4)
        {
            *money = *(f4 *) num_dv->db_data;
        }
        else
        {
            *money = *(f8 *) num_dv->db_data;
        }
    }
    else if (num_dv->db_datatype == DB_DEC_TYPE)
    {
	CVpkf(	(PTR)num_dv->db_data,
		    (i4)DB_P_DECODE_MACRO(num_dv->db_prec),
		    (i4)DB_S_DECODE_MACRO(num_dv->db_prec),
		    money);
    }
    else if (num_dv->db_datatype == DB_INT_TYPE)
    {
        switch(num_dv->db_length)
        {
            case 1:
                *money = (i4) I1_CHECK_MACRO(*(i1 *) num_dv->db_data);
                break;
            case 2:
                *money = (i4) (*(i2 *) num_dv->db_data);
                break;
            case 4:
                *money = *(i4 *) num_dv->db_data;
                break;
            case 8:
                *money = *(i8 *) num_dv->db_data;
                break;
        }
    }
    else /* must be a string type */
    {
        DB_DATA_VALUE mny_dv;

        mny_dv.db_data = (PTR) money;

        if ( !(status = adu_2strtomny( adf_scb, num_dv, &mny_dv )) )
        {
           /* Value returned in cents, need it in whole currency units
           ** with fractional parts.
           */
           *money /= 100.0;
        }
    }

#   ifdef xDEBUG
    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_002_MNYFI_TRACE,&dum1,&dum2))
        TRdisplay("ad0_makedbl: converted to f8, d=%24.6f\n", *money);
#   endif

    return (status);
}
