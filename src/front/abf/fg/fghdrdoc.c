/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>
# include	<lo.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	<si.h>
# include	"framegen.h"


/**
** Name:	fghdrdoc.c -	code to check for changes to join fields.
**
** Description:
**	Generate documentation for the top of a generated 4gl file.
**	The "## generate header_doc" should be inside a comment
**	in the template file.
**
**	This file defines:
**	IIFGhdrdoc
**
** History:
**	6/2/89 (pete)	Written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN char *IIFGft_frametype();
FUNC_EXTERN char *UGcat_now();  /* date in catalog, GMT format. not in ug.h */

GLOBALREF FILE *outfp;
GLOBALREF METAFRAME *G_metaframe;

/* static's */

STATUS
IIFGhdrdoc(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	USER_FRAME *p_uf = (USER_FRAME *) G_metaframe->apobj;

	if (nmbr_words != 0)    /* "##generate header_doc" already stripped*/
	{
	   IIFGerr (E_FG001F_WrongNmbrArgs, FGCONTINUE, 2, (PTR) &line_cnt,
			(PTR) infn);
	   /* A warning; do not return FAIL -- processing can continue. */
	}

	/* No guarantee that these SIfprintf's are working, cause
	** SIfprintf doesn't return an error STATUS!
	*/
	SIfprintf (outfp, ERget(F_FG0007_FrameName), p_wbuf, p_uf->name);

	SIfprintf (outfp, ERget(F_FG0008_FormName), p_wbuf, p_uf->form->name);

	SIfprintf (outfp, ERget(F_FG0009_SourceFile), p_wbuf, p_uf->source);

	SIfprintf (outfp, ERget(F_FG000A_FrameType), 
		p_wbuf, IIFGft_frametype(p_uf->class)) ;

	SIfprintf (outfp, ERget(F_FG000D_Date), p_wbuf, UGcat_now()) ;

	return OK;
}
