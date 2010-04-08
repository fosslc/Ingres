/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:	afcmplen.c -	Calculate Result Length of a Function.
**
** Description:
**	Contains a routine to calculate the result length of a function.
**	Defines:
**
**	iiafComputLen()	calculate result length of a function
**
** History:	
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

i4	adi_0calclen();

/*{
** Name:	iiafComputLen() -	Calculate result length of a function.
**
** Description:
**	This function calculates and returns the result length of a function
**	given a ADI_FI_DESC and the operand types and length.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	fdesc			A ADI_FI_DESC for the function.
**
**	ops			A properly filled in AFE_OPERS for the
**				operands.  Only the type and length are
**				used.
**
**	rlen			Must point to an i4.
**
** Outputs:
**	result	{DB_DATA_VALUE *}
**		.db_length {i4} Is given the result length of the function.
**
** Returns:
**	{DB_STATUS}	OK		if successful.
**			returns from:	'adi_0calclen()'
** History:	
**	Written	2/6/87	(dpr)
**	06/89 (jhw)  Modified to take result DBV and call 'adi_0calclen()'.
**	06/07/1999 (shero03)
**	    Support new parmater list for adi_0calclen.
*/
DB_STATUS
iiafComputeLen ( cb, fdesc, ops, result )
ADF_CB		*cb;
ADI_FI_DESC	*fdesc;
AFE_OPERS	*ops;
DB_DATA_VALUE	*result;
{

	/* 
	** What we shield from our clients is that 'adi_0calclen()' does all
	** the work.  It takes an ADI_LENSPEC, which is part of a function
	** descriptor.  Our clients simply load the AFE_OPERS structure with
	** the desired number of arguments and we do the rest.
	*/
	return adi_0calclen(cb, &(fdesc->adi_lenspec), ops->afe_opcnt,
		            ops->afe_ops, result);
}
