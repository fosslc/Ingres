/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:	afficonv.c -	Return Function Instance ID for Conversion.
**
** Description:
**	Contains a routine that returns a function instance ID and sets the
**	result length for a conversion.  Defines:
**
** 	IIAFfifConv	return function instance id for forced conversion.
**
** History:
**	Revision 8.0  89/08/08  wong
**	Initial revision.
**/

/*{
** Name:	IIAFfifConv() -	Return Function Instance ID
**					for Forced Conversion.
** Description:
**	Returns the ADF function instance ID for converting one data value
**	into another as well as setting the result length.  The conversion
**	is `forced', i.e., the function instance will convert the input value
**	whether the types are compatibile or not.  (Compatible types are
**	implicitly coercible.)  The data value to be converted is input using
**	an AFE operand list along with the result data value.
**
** Input:
**	cb	{ADF_CB *}  The ADF control block.
**	ops	{AFE_OPERS *}  The operand to be converted.
**	result	{DB_DATA_VALUE *}  The result.
**
** Output:
**	result	{DB_DATA_VALUE *}  The result.
**			.db_length	{longnat}	The result length.
**
** Returns:
**	{ADI_FI_ID}  The conversion function instance ID;
**		     otherwise ADI_NOFI on error.
**
** History:
**	08/89 (jhw) -- Written.
*/

static char	*numeric();

ADI_FI_ID
IIAFfifConv ( cb, ops, result )
ADF_CB		*cb;
AFE_OPERS	*ops;
DB_DATA_VALUE	*result;
{
	ADI_OP_ID	cvopid;
	DB_USER_TYPE	cvfunc;
	ADI_FI_DESC	fdesc;

	/* assert(ops->afe_opcnt == 1) */

	if ( afe_vtyoutput(cb, result, &cvfunc) != OK
		|| ( afe_opid(cb, cvfunc.dbut_name, &cvopid) != OK
			&& ( ( AFE_DATATYPE(result) != DB_INT_TYPE
					&& AFE_DATATYPE(result) != DB_FLT_TYPE )
			    || cb->adf_errcb.ad_errcode != E_AD2001_BAD_OPNAME
			    || afe_opid(cb, numeric(result), &cvopid) != OK ) )
			|| afe_fdesc(cb, cvopid, ops, (AFE_OPERS *)NULL,
					result, &fdesc) != OK )
	{
		return ADI_NOFI;
	}
	return fdesc.adi_finstid;
}

/* Return conversion function for integer or float numerics */

static char	buf[FE_MAXNAME+1] = {EOS};

static char *
numeric ( dbv )
register DB_DATA_VALUE	*dbv;
{
	return STprintf(buf,
		(AFE_DATATYPE(dbv) == DB_INT_TYPE ? "int%d" : "float%d"),
		dbv->db_length - (AFE_NULLABLE(dbv->db_datatype) ? 1 : 0)
	);
}
