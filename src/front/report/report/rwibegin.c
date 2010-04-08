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
**   R_WI_BEGIN - set up the flags used in the .WITHIN block
**	for use in scanning the TCMD lists.  This allows
**	the use of the r_group routines without calling the
**	r_p_... routines.
**
**	Parameters:
**		tcmd	first .WITHIN tcmd structure.
**
**	Returns:
**		tcmd address of last of the .WITHIN structures.
**		The next tcmd will be St_ccoms.
**
**	Called by:
**		r_op_add, r_scn_tcmd.
**
**	Side Effects:
**		pushes the BE stack, sets St_cwithin,St_ccoms.
**
**	Trace Flags:
**		130, 131.
**
**	Error messages:
**		Syserr: already in .WI block.
**
**	History:
**		1/12/82 (ps)	written.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
*/



TCMD	*
r_wi_begin(tcmd)
register TCMD	*tcmd;
{
	/* internal declarations */


	/* start of routine */



	if ((St_cwithin!=NULL) || (r_pop_be(P_WITHIN)!=NULL) || (tcmd==NULL))
	{
		r_syserr(E_RW004A_r_wi_begin_Bad_WITHIN);
	}

	r_psh_be(tcmd);

	St_cwithin = tcmd;

	/* find first formatting command after the P_WITHIN commands */

	for(; (tcmd->tcmd_below!=NULL) &&
		((tcmd->tcmd_below)->tcmd_code==P_WITHIN);
			tcmd=tcmd->tcmd_below)
		;

	St_ccoms = tcmd->tcmd_below;


	return(tcmd);
}
