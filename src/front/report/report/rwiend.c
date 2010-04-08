/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_WI_END - turn off the flags used in the .WITHIN block
**	for use in scanning the TCMD lists.  This ends what
**	was started by r_wi_begin.
**
**	Parameters:
**		none.
**
**	Returns:
**		tcmd address of first tcmd structure after
**		the .ENDWITHIN structures.
**
**	Called by:
**		r_op_add, r_scn_tcmd.
**
**	Side Effects:
**		pops the BE stack, sets St_cwithin, St_ccoms.
**
**	Trace Flags:
**		130, 131.
**
**	Error messages:
**		Syserr: not in .WI block.
**
**	History:
**		1/12/82 (ps)	written.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
*/



TCMD	*
r_wi_end()
{
	/* internal declarations */

	register TCMD	*tcmd;			/* fast ptr to find end of
						** .WI block */

	/* start of routine */


	if ((St_cwithin==NULL) || (r_pop_be(P_WITHIN)==NULL) || (St_ccoms==NULL))
	{
		r_syserr(E_RW004B_r_wi_end_Bad_WITHIN);
	}

	St_cwithin = NULL;

	/* find the .ENDWITHIN statement */

	for(tcmd=St_ccoms; (tcmd!=NULL); tcmd=tcmd->tcmd_below)
	{
		if ((tcmd->tcmd_code == P_END) &&
			(tcmd->tcmd_val.t_v_long == P_WITHIN))
		{	/* that's it !! */
			break;
		}
	}

	if (tcmd == NULL)
	{
		r_syserr(E_RW004B_r_wi_end_Bad_WITHIN);
	}

	St_ccoms = NULL;

	return(tcmd);
}
