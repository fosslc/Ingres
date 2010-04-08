/*
**	Copyright (c) 1992, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>
# include	<cm.h>
# include	<lo.h>
# include	<st.h>
# include	<si.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"

/*
** Name:	fgprocs.c - process a "generate local_procedures" statement
**
**	This file defines:
**	IIFGlocalprocs
**
** History:
**	13-aug-92 (blaise)
**		Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* #defines */

/* externs */
FUNC_EXTERN VOID	IIFGbsBeginSection();
FUNC_EXTERN VOID	IIFGesEndSection();

GLOBALREF FILE		*outfp;
GLOBALREF METAFRAME	*G_metaframe;
/*{
** Name:	IIFGlocalprocs - insert local procedure code.
**
** Description:
**
** Inputs:
**	i4	nmbr_words;	number of items in p_wordarray
**	char	*p_wordarray[];	words on command line
**	char	*p_wbuf;	left margin padding for output line
**	char	*infn;		name of input file currently processsing
**	i4	line_cnt;	current line number in input file
**
** Outputs:
**	write local procedure code to outfp.
**
** Returns:
**	STATUS
**
** History:
**	13-aug-92 (blaise)
**	    initial version.
**	16-sep-93 (connie) Bug #53978
**	    Put in the BEGIN LocalProcs & END LocalProcs comments as long as
**	    there is local procedure declaration. This helps to identify the 
**	    location of error for no source code.
**	21-apr-94 (connie) Bug #62286
**	    Process the "generate local_procedures declare" statement
**	    so that DECLARE section is added only if local procedures
**	    exist.
*/
STATUS
IIFGlocalprocs(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
char	*infn;
i4	line_cnt;
{
	i4	i, lproc_cnt = 0, lvarcount = 0;
	MFESC	**p_esc;
	bool	first = TRUE;	/* first segment of local procedure code? */
	register MFVAR **p_mfvar = G_metaframe->vars;
	bool	do_declare = FALSE;  /*TRUE will do the declaration statements*/

	if (nmbr_words == 1)	/* "## generate local_procedures" already
							stripped */
	{
		CVlower(p_wordarray[0]);
		if (STequal(p_wordarray[0],ERx("declare"))) do_declare = TRUE;
	}

	for (i = 0; i < G_metaframe->numvars 
		&& (*p_mfvar)->vartype == VAR_LVAR; i++, p_mfvar++)
	{ /* loop through all local variables */
	}

	/* Count the number of local procedures & write out declarations */
	lvarcount = i;
	for (i=lvarcount; i < G_metaframe->numvars &&
		(*p_mfvar)->vartype == VAR_LPROC; i++, p_mfvar++)
	{
		if (lproc_cnt == 0 )
		{
			if (do_declare) SIfprintf( outfp, ERx("DECLARE\n" ) );

			/* precede first local procedure with comment */
			IIFGbsBeginSection (outfp, ERget(F_FG0045_LocalProcs),
				(char *)NULL, FALSE);
		}

		if (do_declare)
		{ /* write out a 4gl local procedure declaration statement. */
	    		SIfprintf(outfp, 
			ERx("%s%s = PROCEDURE RETURNING %s, \t/* %s */\n"), 
			p_wbuf, (*p_mfvar)->name, (*p_mfvar)->dtype, 
			(*p_mfvar)->comment );
		}

		lproc_cnt++;
	}

	if (!do_declare)
	{
		for (i = 0, p_esc = G_metaframe->escs; i < G_metaframe->numescs;
			i++, p_esc++)
		{
			if ((*p_esc)->type == ESC_LOC_PROC)
			{
				if (first)
				{
					first = FALSE;
				}
				else
				{
				/* write a blank line between procedures */
					SIfprintf(outfp, ERx("\n"));
				}

				if (IIFGwe_writeEscCode((*p_esc)->text, 
					p_wbuf, PAD0, outfp) != OK)
				{
					return FAIL;
				}
			}
		}
	}

	if ( lproc_cnt > 0 )
	{ /* follow last local procedure with Section End */
		IIFGesEndSection (outfp, ERget(F_FG0045_LocalProcs),
			(char *)NULL, FALSE);
	}
	return OK;
}

