/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
#include	<afe.h>

/**
** Name:	afcanco.c -	Check Type Coercibility Module.
**
** Description:
**	Contains the interface to the ADF module that determines whether
**	the types of two data values are coercible.
**
**  Defines:
**	afe_cancoerce()	ADF-style check of type compatibility.
**	afe_tycoerce()	Boolean check of type compatibility.
**
** History:
**	Revision 6.2  89/05  wong
**	Implement DEFAULT case for 'afe_tycoerce()'.
**	06/10/89 (dkh) - Fixed bug 6436.
**
**	Revision 6.1  88/10  dave
**	10/21/88 (dkh) - Speeded up implementation.
**	10/31/88 (ncg) - Hard-coded afe_tycoerce.
**
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**	24-Aug-2009 (kschendel) 121804
**	    include afe.h for bool definitions.
**/


/*{
**  Name:	afe_cancoerce() -	Can we coerce these two types?
**
**  Description:
**	Determines whether the types of the two DB_DATA_VALUEs passed 
**	are compatable and can be coerced.  For example, integers and floats
**	are compatable with each other and can be coerced into float
**	or integer fields, respectively.  However, character-type fields
**	are not compatable with integer and float values. 
**
**  Inputs:
**	cb	{ADF_CB *}	A reference to the current ADF control block.
**
**	atype	{DB_DATA_VALUE *}  	A reference to a DB_DATA_VALUE with
**					one of the types in question.
**		
**	btype	{DB_DATA_VALUE *}	A reference to a DB_DATA_VALUE with 
**					the other type in question.
**
**  Outputs:
**	result	{bool}		A reference to a boolean that should contain the
**				coercion result.  Value is TRUE if the types 
**				can be coerced, FALSE otherwise.
**
**  Returns:
**	{STATUS}  OK		Could determine coercion but may not be
**				compatible.
**  Errors:
**	E_AD2009_NOCOERCION - No type compatibility.  No error message is put
**			      into the ADF error block.
**
**  Side Effects:
**	None
**	
**  History:
**	Written	2/6/87	(dpr)
**	11/01/88 (ncg) - Modified to use afe_tycoerce.
*/
DB_STATUS
afe_cancoerce (cb, atype, btype, result)
ADF_CB		*cb;
DB_DATA_VALUE	*atype;
DB_DATA_VALUE	*btype;
bool		*result;
{
	*result = afe_tycoerce(cb, atype->db_datatype, btype->db_datatype);
	return OK;
}


/*{
**  Name:	afe_tycoerce() -	Fast Type Compatibility Checker.
**
** Description:
**	This routine is used instead of adi_ficoerce and adi_tycoerce in order
**	to provide type checks.  
**
**	The routine confirms that the non-nullable type ids are compatible.
**	The LTXT type is always compatible.
**
** Inputs:
**	acb			Pointer to the current ADF_CB control block.
**	itype			Input data type id.
**	otype			Output data type id.
**
** Outputs:
**	acb.adf_errcb		ADF error data (if errroneous conversion):
**	   .ad_errcode		Error code if failure.  See numbers below.
**	   .ad_emsglen		If an error occured this is set to zero as
**				no message is looked up.
**
** Returns:
**	{bool} 	TRUE		If compatible types.
**		FALSE		If not compatible.
** Errors:
**	E_AD2009_NOCOERCION - No type compatibility.
**
**  History:
**	2/6/87	 (dpr)	- Written.
**	10/31/88 (ncg)	- Rewritten.
**	05/89 (jhw) - Added DEFAULT case.
**	06/10/89 (dkh) - Fixed bug 6436.  Added calls to adi_tycoerce
**			 to ask ADF for unusual conversion cases.
**	08/89 (jhw) - Removed DB_MNY_TYPE fast check (which will now fall
**		through to the DEFAULT case and call ADF.)  ADF does not
**		support money ==> other integer conversions.
**	21-oct-1992 (rdrane)
**		The ANSI compiler complained about not reaching the lines
**		after the switch statement, and it was right - you can't
**		get there from here.  So, any error detection and reporting
**		will be ADF's responsibility.  The lines are being retained
**		but commented out in case future modifications allow exit from
**		the switch statement.
**	30-aug-2006 (toumi01) SIR 116208
**		Return FALSE for ADF coercions that are now supported between
**		numeric and character datatypes but which the front end
**		programs have not historically expected and for which TRUE
**		results here will confuse them.
**	07-sep-2006 (gupsh01)
**	    Added support for ANSI Date/time types.
*/

bool
afe_tycoerce ( ADF_CB *acb, DB_DT_ID itype, DB_DT_ID otype )
{
    ADI_FI_ID	cvid;
    DB_DT_ID	it = itype > 0 ? itype : -itype;
    DB_DT_ID	ot = otype > 0 ? otype : -otype;

    switch (it)
    {
      case DB_INT_TYPE:
      case DB_FLT_TYPE:
      case DB_DEC_TYPE:
	switch (ot)
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_MNY_TYPE:
	  case DB_DEC_TYPE:
	  case DB_LTXT_TYPE:
	    return TRUE;
	  case DB_CHA_TYPE:
	  case DB_CHR_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	    return FALSE;

	  default:
	    /*
	    **  Call ADF to do final check for possible coercions.
	    */
	    return (bool)( adi_ficoerce(acb, itype, otype, &cvid) == OK );
	    break;
	}
	break;
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_DTE_TYPE:
      case DB_ADTE_TYPE:
      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
	switch (ot)
	{
	  case DB_CHA_TYPE:
	  case DB_CHR_TYPE:
	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
	  case DB_DTE_TYPE:
	  case DB_LTXT_TYPE:
          case DB_ADTE_TYPE:
          case DB_TMWO_TYPE:
          case DB_TMW_TYPE:
          case DB_TME_TYPE:
          case DB_TSWO_TYPE:
          case DB_TSW_TYPE:
          case DB_TSTMP_TYPE:
          case DB_INYM_TYPE:
          case DB_INDS_TYPE:
	    return TRUE;
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	    return FALSE;

	  default:
	    /*
	    **  Call ADF to do final check for possible coercions.
	    */
	    return (bool)( adi_ficoerce(acb, itype, otype, &cvid) == OK );
	    break;
	}
	break;
      case DB_LTXT_TYPE:
	return TRUE;
	break;
      default:
      {
	ADI_FI_ID	junk;

	return (bool)( adi_ficoerce(acb, itype, otype, &junk) == OK );
	break;
      }
    } /* end switch */


/****************************************************************************
**	Restore these lines if we ever come up with a case which will NOT
**	return from inside the above switch statement.
** 
**
**  acb->adf_errcb.ad_errcode = E_AD2009_NOCOERCION;
**  acb->adf_errcb.ad_emsglen = 0;
**
**  return FALSE;
**
**
****************************************************************************/

}
