/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<si.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	<er.h>

/*
**   S_PR_REN - print out a REN structure, in debugging.
**
**	Parameters:
**		ren	ptr to REN structure.
**
**	Returns:
**		none.
**
**	History:
**		12/28/81 (ps)	written.
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
*/


s_pr_ren(ren)
register REN	*ren;
{

	SIprintf(ERx("   REN address:%p\n"),ren);
	if (ren->ren_report)
		SIprintf(ERx("	ren_report:%s\n"),ren->ren_report);
	if (ren->ren_owner != NULL)
		SIprintf(ERx("	ren_report:%s\n"),ren->ren_owner);
	SIprintf(ERx("	ren_created:%d\n"),ren->ren_created);
	SIprintf(ERx("	ren_modified:%d\n"),ren->ren_modified);
	if (ren->ren_type)
		SIprintf(ERx("	ren_type:%s\n"),ren->ren_type);
	SIprintf(ERx("	ren_acount:%d\n"),ren->ren_acount);
	SIprintf(ERx("	ren_scount:%d\n"),ren->ren_scount);
	SIprintf(ERx("	ren_qcount:%d\n"),ren->ren_qcount);
	SIprintf(ERx("	add ren_rsotop:%p\n"),ren->ren_rsotop);

	return;
}
