/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/* # include's */
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include <adf.h>
# include <afe.h>
# include       <er.h>

/**
** Name:	afclfunc.c -	Call a function.
**
** Description:
**	This file contains afe_clfunc.  This file defines:
**
**	afe_clfunc()	routine to call some function (sin, cos, etc). 
**
** History:
**	Written	2/6/87	(dpr)
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */


/* extern's */

FUNC_EXTERN	i4	 afe_opid();
FUNC_EXTERN	i4	 afe_fdesc();
FUNC_EXTERN	i4	 afe_clinstd();

/* static's */

/*{
** Name:	afe_clfunc	-	Call a function
**
** Description:
**	Call an ADF function given the function name, its
**	operands and its result type.
**
**	The type and length of the result must be known to use
**	this function.  If you don't know the result type and length,
**	call the functions afe_fdesc followed by afe_clinstd.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	name			A null terminated string containing the name
**				of the function to call.
**
**	ops			The operands for the function.
**
**		.afe_opcnt	The number of operands.
**
**		.afe_ops	The array of operands.
**
**	result
**		.db_datatype	The datatype of the result.
**
**		.db_length	The length of the result.
**
** Outputs:
**	result
**		.db_data	Set to the result of the function.
**
**	Returns:
**		OK			If it successfully called the function.
**
**		E_AF6001_WRONG_NUMBER	If the wrong number of arguments 
**					are supplied.
**
**		returns from:
**			afe_opid
**			afe_fdesc
**			afe_clinstd
**
*/
i4
afe_clfunc(cb, name, ops, result)
ADF_CB		*cb;
char		*name;
AFE_OPERS	*ops;
DB_DATA_VALUE	*result;
{
	i4		rval;
	ADI_FI_ID	fid;
	ADI_OP_ID	opid;		/* The operator id for the conversion */
	ADI_FI_DESC	fdesc;

	if (ops->afe_opcnt > 2 || ops->afe_opcnt < 0)
		return afe_error(cb, E_AF6001_WRONG_NUMBER, 0);
	if ((rval = afe_opid(cb, name, &opid)) != OK)
		return rval;
	if ((rval = afe_fdesc(cb, opid, ops,(AFE_OPERS *) NULL, 
				result, &fdesc)) != OK)
	{
		return rval;
	}
	fid = fdesc.adi_finstid;
	if ((rval = afe_clinstd(cb, fid, ops, result)) != OK)
	{
		return rval;
	}

	/*
	** should we if-def this out? what's the protocol?
	SIprintf("AFTER CALL, passed in result, dbdv follows\n");
	adu_prdv(result);
	*/

	return OK;
}
