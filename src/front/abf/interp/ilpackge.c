/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<si.h>
#include        <me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	<erit.h>

/**
** Name:	ilpackge.c -	4GL Interpreter Data Abstractions Module.
**
** Description:
**	Data abstractions.
**	Hides implementation of an operand.
**	Actually, information on the data type and length of a
**	particular operand are stored in the corresponding DB_DATA_VALUE.
**
** History:
**	Revision 5.1  86/04  arthur
**	Initial revision.
**
**	Revision 6.2  88/11/30  joe
**	Updated for 6.2.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		added an argument to IIOgvGetVal() to return additional
**		value information, which is sometimes needed to fully
**		construct a DBV (e.g. for decimal datatypes).
**	31-mar-93 (davel)
**		Fixed bug 50850 in IIOgvGetVal() - do not include
**		DB_TXT_TYPE as a type that can be easily converted to
**		a C string.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN 	char	*IIARtocToCstr();

/*{
** Name:	IIITvtnValToNat	- Convert a value to a i4.
**
** Description:
**	This routine takes an operand and converts it to a i4.
**	If the operand is a constant, then it is assumed to be
**	an INTEGER constant.
**	If the operand is NULL, then 0 is returned.
**	If the operand is a variable, then the type of the variable
**	must be integer or coercible to integer.  If the variable's
**	value is NULL, then the error E_IT001F_NULLINT is given using
**	the passed value as a parameter, and 0 is returned.
**
** Inputs:
**	val		The operand value to convert to a i4.
**	defval		The default value to return, if 'val' is null.
**	errmsg		The parameter for the E_IT001F_NULLINT error.
**
** History:
**	30-nov-1988 (Joe)
**		First Written
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL):
**		Return i4 instead of i4, because the value 
**		may represent an offset into IL.
*/
i4
IIITvtnValToNat(val, defval, errmsg)
IL	val;
i4	defval;
char	*errmsg;
{
    i4				i4var;	
    DB_DATA_VALUE		dbv;
    DB_DATA_VALUE		*from;

    if (ILNULLVAL(val))
	return (i4) defval;
    if (ILisConst(val))
	return (i4) *((i4 *) IIOgvGetVal(val, DB_INT_TYPE, (i4 *)NULL));
    /*
    ** In this case, it is a variable, if the type is NULLable check
    ** for the null value.  If it is NULL, then give an error, otherwise
    ** convert to an i4 and then convert that to a NAT.
    */
    from = IIOFdgDbdvGet(val);
    if (AFE_NULLABLE(from->db_datatype))
    {
	i4	itsnull;

	adc_isnull(FEadfcb(), from, &itsnull);
	if (itsnull)
	{
	    IIUGerr(E_IT001F_NULLINT, 0, 1, errmsg);
	    return (i4) defval;
	}
    }
    dbv.db_data = (PTR) &i4var;
    dbv.db_length = sizeof(i4);
    dbv.db_datatype = DB_INT_TYPE;
    afe_cvinto(FEadfcb(), from, &dbv);
    return (i4) i4var;
}

/*{
** Name:	IIOgvGetVal	-	Get the actual data of an operand
**
** Description:
**	Get the actual data of an operand.
**
** Inputs:
**	val	The operand.  If positive, this is the index of the
**		value's DB_DATA_VALUE in the base array.  If negative, this
**		is the offset of the value in the appropriate constants table.
**	type	The type of the operand:  DB_INT_TYPE, DB_FLT_TYPE, DB_CHR_TYPE,
**		DB_DEC_TYPE, or DB_QUE_TYPE.
**	valinfo Pointer to a i4  for additional value information - used for
**		decimal types to return precision/scale which is necessary
**		to compute the db_length in IIITctdConstToDbv().
**
** Outputs:
**	Returns:
**		valinfo - additional information associated with the value;
**			  for now this is limited to the precision/scale
**			  for decimal types.
**		Pointer to the actual data value.
**
*/
PTR
IIOgvGetVal(val, type, valinfo)
IL	val;
i4	type;
i4	*valinfo;
{
	PTR		rval = NULL;
	DB_DATA_VALUE	*base;

	if (val < 0)	/* argument is a constant */
	{
		_VOID_ IIORcqConstQuery(&il_irblk, - (val), type, 
					&rval, valinfo);
	}
	else
	{
		base = IIOFdgDbdvGet(val);
		if (base->db_datatype == DB_DYC_TYPE)
			rval = ((ABRTSSTR *) base->db_data)->abstrptr;
		else if (type == DB_DEC_TYPE && valinfo != NULL)
			*valinfo = (i4) base->db_prec;
		else if (type != DB_CHR_TYPE)
			rval = base->db_data;
		else
		{
			register char *dataptr = (char *)base->db_data;

			switch (base->db_datatype)
			{
			    case DB_CHA_TYPE:
			    case DB_CHR_TYPE:
				if (*(dataptr + base->db_length) == EOS)
				{
					/* We're null-terminated */
					rval = dataptr;
					break;
				}
				else
				{
					/* Fall through */
				}

			    default:
				/* Make a C string version */
				rval = IIARtocToCstr(base);
			}
		}
	}

	return (rval);
}


/*{
** Name:	IIOctdConstToDbv - Take a constant, a type and fill in a dbv.
**
** Description:
**	Given an IL operand which is a constant, and the expected
**	type of that constant, this routine will fill in a DBV to point
**	at that constant.
**
**	If the operand is not a constant, this routine will return FAIL,
**	but otherwise it is a NOOP.
**
** Implementation:
**	Depends on the fact that only 4 types of constants are supported:
**	
**	DB_INT_TYPE	is an i4 constant.
**	DB_FLT_TYPE	is an f8 constant.
**	DB_VCH_TYPE	is a DB_TEXT_STRING constant.
**	DB_CHA_TYPE	is a null-terminated character constant.
**	-DB_LTXT_TYPE	is the NULL constant.
**
**
** Inputs:
**	const_op	An IL operand which must be a constant.
**
**	type		The type of the constant
**	
** Outputs:
**	dbv		The DB_DATA_VALUE to fill in.  This must be
**			allocated by the caller.
**		.db_data
**			Will point to the data value for the constant.
**		.db_length
**			Will be filled in with the actual size of the
**			constant.
**		.db_prec
**			Set to zero for all types except for Decimal; set to
**			the actual precision of the constant for decimal.
**		.db_type
**			Will be filled in with the passed type.
**
**	Returns:
**		OK	If const_op is a constant and it found it.
**
**		FAIL	Otherwise
**
** History:
**	27-oct-1988 (Joe)
**		First Written
**	26-jun-92 (davel)
**		added support for decimal constants.
*/
STATUS
IIITctdConstToDbv(const_op, type, dbv)
IL		const_op;
DB_DT_ID	type;
DB_DATA_VALUE	*dbv;
{
    i4  valinfo;

    /*
    ** We take care of the NULL constant right away, since
    ** it is the only one that does not have a value in a constant
    ** table so the code for it is different from the other types
    ** of constants.
    */
    if (type == -DB_LTXT_TYPE)
    {
	IIARnliNullInit(dbv);
	return OK;
    }
    /*
    ** In all cases, we assign the address of the constant to
    ** the dbv's db_data, so do that first.
    */
    if ((dbv->db_data = IIOgvGetVal(const_op, type, &valinfo)) == NULL)
	return FAIL;;

    /*
    ** At this point, the dbv points at the constant.
    ** Now we have to set the length correctly.
    ** For character types, some real work has to get done.
    */
    switch (type)
    {
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      {
	DB_TEXT_STRING	*vch_const;

	vch_const = (DB_TEXT_STRING *) dbv->db_data;
	dbv->db_length = vch_const->db_t_count + DB_CNTSIZE;
	dbv->db_prec = 0;
	break;
      }

      case DB_CHA_TYPE:
	dbv->db_prec = 0;
	dbv->db_length = STlength( (char *) dbv->db_data);
	break;

      case DB_INT_TYPE:
	dbv->db_prec = 0;
	dbv->db_length = sizeof(i4);
	break;

      case DB_FLT_TYPE:
	dbv->db_prec = 0;
	dbv->db_length = sizeof(f8);
	break;
    
      case DB_DEC_TYPE:
	dbv->db_prec = (i2) valinfo;
	/* sure would be nice to have a DB_PS_TO_LEN_MACRO in dbms.h */
	dbv->db_length = DB_PREC_TO_LEN_MACRO(DB_P_DECODE_MACRO(dbv->db_prec));
	break;
    
      default:
	return FAIL;
    }
    return OK;
}

/*{
** Name:	IIOvtValType	-	Return the type of a VALUE
**
** Description:
**	Returns the type of a VALUE:  either DB_INT_TYPE, DB_FLT_TYPE,
**	or DB_CHR_TYPE.
**
** Inputs:
**	val	The VALUE.
**
** Outputs:
**	Returns:
**		The VALUE's type.
**
*/
i4
IIOvtValType(val)
IL	val;
{
	DB_DATA_VALUE	*base;

	base = IIOFdgDbdvGet(val);
	return (base->db_datatype);
}

/*{
** Name:        ILgetI4Operand    -       Return the value of an i4 operand.
**
** Description:
**      Returns the value of an i4 operand that spans 2 IL operands.
**
** History:
**      25-Mar-2008 (hanal04) Bug 118967
**          Created to avoid SIGBUS memory alignment issues seen on su9.
**
** Inputs:
**
**      IL      *stmt;  ** a pointer to the beginning of the IL statement **
**      i4      num;    ** the relative number of the operand **
**
** Returns:
**
**      an i4 value from the operands.
*/
i4
ILgetI4Operand(stmt, num)
IL      *stmt;
i4      num;
{
     IL   *offset = stmt + num;
     i4   res;

     MEcopy((PTR)offset, sizeof(res), (PTR)&res);
     return(res);
}
