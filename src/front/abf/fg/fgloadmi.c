/*
**	Copyright (c) 1991, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <er.h>
# include       <ug.h>
# include       <cm.h>
# include       <lo.h>
# include       <st.h>
# include       <si.h>
# include       <oocat.h>
# include       <abclass.h>
# include       <metafrm.h>
# include       <erfg.h>
# include       "framegen.h"
# include       <fglstr.h>

/**
** Name:	fgloadmi.c - Load Menuitems into tablefield
**
** Description:
**	This file defines:
**
**	IIFGglmGenLoadMenu - Load menuitems into tablefield
**
** History:
**	2/91 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
GLOBALREF FILE *outfp;
GLOBALREF METAFRAME *G_metaframe;

FUNC_EXTERN VOID IIFGdq_DoubleUpQuotes();
FUNC_EXTERN LSTR *IIFGals_allocLinkStr();
FUNC_EXTERN VOID IIFGols_outLinkStr();

/* static's */

/*{
** Name:	IIFGglmGenLoadMenu - Load menuitems into tablefield
**
** Description:
**	For each called frame, load the menuitem text and short remark into
**	the tablefield.
**
** Inputs:
**      i4      nmbr_words;     ** number of words in p_wordarray.
**      char    *p_wordarray[]; ** words on command line.
**      char    *p_wbuf;        ** whitespace to tack to front of generated code
**      char    *infn;          ** name of input file currently processing.
**      i4      line_cnt;       ** current line number in input file.
**
** Outputs:
**	none
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	2/91 (Mike S) Initial version
*/
STATUS
IIFGglmGenLoadMenu(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4      nmbr_words;     /* number of words in p_wordarray */
char    *p_wordarray[]; /* words on command line. */
char    *p_wbuf;        /* whitespace to tack to front of generated code */
char    *infn;          /* name of input file currently processing */
i4      line_cnt;       /* current line number in input file */
{
	LSTR *lstr;
	LSTR *lstr0;
	i4	i;

        if (nmbr_words > 0)
        {
            IIFGerr (E_FG0062_WrongNmbrArgs, FGCONTINUE, 2, (PTR) &line_cnt,
                        (PTR) infn);

            /* A warning; do not return FAIL -- processing can continue. */
        }

	/* 
	** Load tablefield. 
	*/
	for (i = 0; i < G_metaframe->nummenus; i++)
	{
		/* Buffers with enough room to double quote marks */
		char double_remark[(OOSHORTREMSIZE*2) +1]; 
		char double_text[FE_MAXNAME * 2 + 1];

		MFMENU *mptr = G_metaframe->menus[i];

		IIFGdq_DoubleUpQuotes(mptr->text, double_text);
		IIFGdq_DoubleUpQuotes(mptr->apobj->short_remark, double_remark);

		/* This string may be long, so allocate it in pieces */
		if ((lstr0 = IIFGals_allocLinkStr ((LSTR *)NULL, LSTR_NONE))
			== NULL)
		{
			return FAIL;
		}

		STprintf(lstr0->string, "LOADTABLE %s (", TFNAME);

		if ((lstr = IIFGals_allocLinkStr (lstr0, LSTR_NONE)) == NULL)
			return FAIL;
		STprintf(lstr->string, "command = '%s',", double_text);

		if ((lstr = IIFGals_allocLinkStr (lstr, LSTR_NONE)) == NULL)
			return FAIL;

		STprintf(lstr->string, "explanation = '%s');", double_remark);
		lstr->nl = TRUE;
		IIFGols_outLinkStr(lstr0, outfp, p_wbuf);
	}
	if (G_metaframe->nummenus > 0)
		SIfprintf(outfp, ERx("%sSCROLL %s TO 1;\n"), p_wbuf, TFNAME);
	
	return OK;
}
