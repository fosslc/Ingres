/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>

/**
** Name:	affitype.c -	Returns Type of Function.
**
** Description:
**	Contains the routines used to get the class of an ADF function.
**	Defines:
**
**	afe_fitype	returns the class of a function.
**	IIAFfiType()	return operator ID and class for an ADF function.
**
** History:
**	Revision 6.5  1991/05/31  wong
**	Added 'IIAFfiType()' function.
**
**	Revision 6.0  1987/03/23  daver
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

DB_STATUS	IIAFfiType();
DB_STATUS	afe_opid();
DB_STATUS	adi_op_info();

/*{
** Name:	afe_fitype() -	Returns class of function.
**
** Description:
**	Given a function name, this routine returns the its function class,
**	i.e., it's an aggregate function, an operator, a normal function,
**	a coercion function, or a comparison operator.
**
** Inputs:
**	cb	A pointer to the current ADF_CB control block.
**
**	name	A NULL-terminated string containing the function name.
**
**	type	Must point to a nat.
**
** Outputs:
**	type	Set to the type of the function:
**			ADI_COERCION		Coercion function
**			ADI_OPERATOR		Operator
**			ADI_NORM_FUNC		Normal function
**			ADI_AGG_FUNC		Aggregate function
**			ADI_COMPARISON		Comparison operator
**			
** Returns:
**	{DB_STATUS}	OK
**			returns from:  IIAFfiType()
** History:
**	23-mar-1987 (daver) First Written.
**	31-may-1991 (jhw) Moved functionality to 'IIAFfiType()'.
*/

DB_STATUS
afe_fitype ( cb, name, type )
ADF_CB	*cb;
char	*name;
i4	*type;
{
    ADI_OP_ID	opid;

    return IIAFfiType(cb, name, &opid, type);
}

/*{
** Name:	IIAFfiType() -	Returns Operator ID and Class for ADF Function.
**
** Description:
**	Given a ADF function name, returns the both its ADF operator ID and its
**	function class, i.e., whether it's an aggregate function, an operator,
**	a normal function, a coercion function, or a comparison operator.
**
** Inputs:
**	cb	{ADF_CB}  The ADF_CB control block.
**	name	{char *}  The ADF function name.
**	opid	{ADI_OP_ID *}  The result operator ID address.
**	type	{nat *}  The result function class address.
**
** Outputs:
**	opid	{ADI_OP_ID *}  The ADF operator ID for the function.
**	type	{nat *}  Set to the type of the function:
**			ADI_COERCION		Coercion function
**			ADI_OPERATOR		Operator
**			ADI_NORM_FUNC		Normal function
**			ADI_AGG_FUNC		Aggregate function
**			ADI_COMPARISON		Comparison operator
**			
** Returns:
**	{DB_STATUS}	OK
**			returns from:
**				afe_opid()
**				adi_op_info()
** History:
**	31-may-1991 (jhw) Modified original 'afe_fitype()' to set new passed
**				operator ID and to optimally use 'adi_op_info()'.
*/

DB_STATUS
IIAFfiType ( cb, name, opid, type )
ADF_CB		*cb;
char		*name;
ADI_OP_ID	*opid;
i4		*type;
{
    DB_STATUS	rval;
    ADI_OPINFO	opinfo;

    if ( (rval = afe_opid(cb, name, opid)) != OK ||
		(rval = adi_op_info(cb, *opid, &opinfo)) != OK )
    {
	return rval;
    }

    *type = opinfo.adi_optype;

    return OK;
}
