/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include       <er.h>

/**
** Name:	afclinstd.c -	Call a function given instance id.
**
** Description:
**	This file contains afe_clinstd.  This file defines:
**
**	afclinstd	calls a funciton given its instance id.
**
** History:
**	Written	2/6/87	(dpr)
**	24-aug-1990	(teresal)
**			Added decimal support: pass db_prec.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need me.h to satisfy gcc 4.3.
**/

i4	adf_func();

/*{
** Name:	afe_clinstd() -	Call a function given instance ID.
**
** Description:
**	Call an ADF function given its instance ID, its operands and its result
**	type.
**
**	The type and length of the result must be known to use
**	this function.  If you don't know the result type and length,
**	call the function afe_fdesc to get this information.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	instd			The instance id for the function.
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
**					were supplied.
**		returns from:
**			adf_func
** History:
**	Written	2/6/87	(dpr)
**	25-oct-1991     (kirke/gautam)
**		Correct initialization error of 'adf_isescape' in 'fnblk'.
**	15-Oct-2008 (kiria01) b121061
**	    Extend the number of supported parameters to that in the dbms
**	    and access these in a more general manner.
**      26-May-2009 (hanal04) Bug 122105
**          Initialise fnblk (ADF_FN_BLK) to avoid spurious results.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK adf_dv_n member.
**	19-Aug-2010 (kschendel) b124282
**	    isescape deprecated, initialize pat-flags.
**	19-Nov-2010 (kiria01) SIR 124690
*	    Correct & symbolize collID checks.
*/
STATUS
afe_clinstd(cb, instd, ops, result)
ADF_CB		*cb;
ADI_FI_ID	instd;
AFE_OPERS	*ops;
DB_DATA_VALUE	*result;
{
	STATUS		rval;
	ADF_FN_BLK	fnblk;
	i4		i;
	VOID	adu_prdv();

	/* right number of args? */ 
	if (ops->afe_opcnt > ADI_MAX_OPERANDS-1 || ops->afe_opcnt < 0)
	{
		return afe_error(cb, E_AF6001_WRONG_NUMBER, 0);
	}

	/* This clears adf_fn_desc among others */
        MEfill(sizeof(fnblk), '\0', (PTR)&fnblk);

	/*
	**	LOAD THE FUNCTION INSTANCE ID
	*/
	fnblk.adf_fi_id = instd;	/* this is correct, but ICK */
	fnblk.adf_pat_flags = AD_PAT_DOESNT_APPLY; /* (better) not (be) LIKE */
	fnblk.adf_dv_n = ops->afe_opcnt;
	/* 	LOAD THE OPERAND DB_DATA_VALUES		*/ 
	for (i = 0; i <= ops->afe_opcnt; i++)
	{
	    if (i)
		fnblk.adf_dv[i] = *ops->afe_ops[i-1];
	    else
		fnblk.adf_dv[i] = *result;

	    /* Keep collID in bounds */
	    if ((u_i4)fnblk.adf_dv[i].db_collID >= DB_COLL_LIMIT)
		fnblk.adf_dv[i].db_collID = DB_UNSET_COLL;
	}

	/*	CALL THE FUNCTION	*/
	if ((rval = adf_func(cb, &fnblk)) != OK)
	{
	    return rval;
	}

	/*
	** should we if-def this out? what's the protocol?
	SIprintf("AFTER CALL, passed in result, fnblk result dbdvs follow\n");
	adu_prdv(result);
	adu_prdv(&(fnblk.adf_dv[0]));
	*/

	return OK;
}
