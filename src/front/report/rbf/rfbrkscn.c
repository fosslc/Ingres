/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfbrkscn.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFBRK_SCN - scan the .HEADER/.FOOTER blocks for the page and
**	for the breaks in order to fill in fields in the COPTIONS
**	form.  The fields filled in are:
**		copt->copt_whenprint	determined by .TFORMAT commands
**					in the headers.
**		copt->copt_newpage	determined by .NEWPAGE commands in 
**					the headers.
**
**	Parameters:
**		none.
**
**	Returns:
**		TRUE	if all went well.
**		FALSE	if not, and will cause a SYSERR in calling routine.
**	
**	Called by:
**		rFdisplay.
**
**	Side Effects:
**		Updates the COPT structures for break attributes.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		200.
**
**	History:
**		9/12/82 (ps)	written.
**		8/24/83 (gac)	bug 1496 fixed -- now 1st page is not blank if newpage option is chosen.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
*/



bool
rFbrk_scn()
{
	/* internal declarations */

	COPT		*copt;		/* COPT for attribute */
	BRK		*brk;		/* break structure */

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0))
	{
		SIprintf(ERx("rFbrk_scn: entry.\r\n"));
	}
#	endif

	/* First check the page HEADER for .TFORMAT commands */

	for(Cact_tcmd=Ptr_pag_brk->brk_header; Cact_tcmd!=NULL; 
			Cact_tcmd=Cact_tcmd->tcmd_below)
	{	/* only looking for TFORMAT commands */
		switch(Cact_tcmd->tcmd_code)
		{
			case(P_TFORMAT):
				copt = rFgt_copt(Cact_tcmd->tcmd_val.t_v_ap->ap_ordinal);
				if (copt == NULL)
					return (FALSE);
				copt->copt_whenprint = 'p';
				break;

			default:
				break;
		}
	}

	/* Now check the HEADER/FOOTERs for breaks  */

	for (brk=Ptr_brk_top->brk_below; brk!=NULL; brk=brk->brk_below)
	{	/* next brk with header or footer */
		copt = rFgt_copt(brk->brk_attribute);
		if (copt->copt_break != 'y')
		{	/* skip this one */
			continue;
		}

		/* scan header for TFORMAT or NEWPAGE */

		for (Cact_tcmd = brk->brk_header; Cact_tcmd != NULL; Cact_tcmd = Cact_tcmd->tcmd_below)
		{
			if (Cact_tcmd->tcmd_code == P_TFORMAT)
			{
				copt->copt_whenprint = (copt->copt_whenprint=='p') ? 't' : 'b';
			}
		}

		/* scan footer for NEWPAGE */

		for (Cact_tcmd = brk->brk_footer; Cact_tcmd != NULL; Cact_tcmd = Cact_tcmd->tcmd_below)
		{
			switch(Cact_tcmd->tcmd_code)
			{
				case(P_NPAGE):
					copt->copt_newpage = TRUE;
					break;

				default:
					break;
			}
		}
	}

	return(TRUE);
}
