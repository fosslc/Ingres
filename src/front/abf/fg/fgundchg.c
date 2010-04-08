/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include       <ug.h>
# include	<lo.h>
# include	<st.h>
# include	<ooclass.h>
# include	<si.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"


/**
** Name:	fgundchg.c -	code to check for changes to join fields.
**
** Description:
**	Undo changes to join fields.
**
**	This file Defines:
**
**	IIFGundchg
**	gen_undchg
**
** History:
**	7/5/89 (pete)	Initial Version.
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

/* static's */
static VOID gen_undchg();

/*{
** Name:	IIFGundchg - generate code to check for join field change.
**
** Description:
**	Generate 4gl code to copy values in hidden fields to corresponding
**	visible fields. For example,
**	for an UPDATE or MODIFY VQ with joinfields named 'cust', 'div' & 'ord'
**	generate (note that fginit sets name of hidden versions of join fields):
**
**		** undo changes made to join field(s) **
**		cust = ii_cust;
**		div = ii_div;
**		ord = ii_ord;
**
** Inputs:
**      i4      nmbr_words;     ** number of words in p_wordarray.
**      char    *p_wordarray[]; ** words on command line.
**      char    *p_wbuf;        ** whitespace to tack to front of generated code
**      char    *infn;          ** name of input file currently processing.
**      i4      line_cnt;       ** current line number in input file.
**
** Outputs:
**      Writes generated code to FILE *outfp
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	7/3/89 (pete)	Initial Version.
**	2/91 (Mike S)	Only displayed fields
*/
STATUS
IIFGundchg(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4      nmbr_words;     /* number of words in p_wordarray */
char    *p_wordarray[]; /* words on command line. */
char    *p_wbuf;        /* whitespace to tack to front of generated code */
char    *infn;          /* name of input file currently processing */
i4      line_cnt;       /* current line number in input file */
{
        if (nmbr_words <= 0)
        {
            /*Note: "##generate copy_hidden_to_visible" already stripped*/

            IIFGerr (E_FG0034_MissingArgs, FGFAIL, 2, (PTR) &line_cnt,
			(PTR) infn);
        }

        CVlower(p_wordarray[0]);

        if (STequal(p_wordarray[0],ERx("join_fields")))
	{
	     gen_undchg (G_metaframe, p_wbuf);
	}
        else
        {
            /* unknown argument on ##generate undo_change statement */
            IIFGerr (E_FG0035_UnknownArg, FGFAIL,
                        3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
        }

	return OK;
}

/*{
** Name:	gen_undchg - generate 4gl to undo join field changes.
**
** Description:
**
** Inputs:
**	METAFRAME *mf		pointer to METAFRAME.
**	char      *p_wbuf	whitespace to tack to front of generated code
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	7/6/89	(pete)	Initial Version.
*/
static VOID
gen_undchg(mf, p_wbuf)
METAFRAME *mf;
char    *p_wbuf;        /* whitespace to tack to front of generated code */
{
	MFTAB **p_mftab = mf->tabs;
	register MFJOIN **p_mfjoin = mf->joins;
	register i4  i;
	i4 num_tests=0;   /* number of assignments generated so far */

	/* for every join */
	for (i=0; i < mf->numjoins; i++, p_mfjoin++)
	{
	    /*if join is master-detail type (we don't care about lookup joins)*/
	    if ( (*p_mfjoin)->type == JOIN_MASDET)
	    {
		/* Locate the master table.column involved in the join.
		** If the field has a hidden field version, then generate code
		** to check for changes to this field.
		** (Currently only TAB_UPDATE tables will have a hidden field
		** version of the join field)
		*/

		MFTAB *tmp_mftab;
		MFCOL **tmp_mfcol;

		/* Set pointer to master table in join
		** (MFJOIN.tab_1 is for master).
		*/
		tmp_mftab = *(p_mftab + (*p_mfjoin)->tab_1);

		/* Set pointer to MFCOLS for above master join table */
		tmp_mfcol = tmp_mftab->cols + (*p_mfjoin)->col_1 ;

		if (   ((*tmp_mfcol)->utility != NULL)
		    && ((*tmp_mfcol)->utility[0] != EOS)
		    && (((*tmp_mfcol)->flags & COL_USED) != 0)
		   )
		{
		    /* Join field has a hidden field version, so generate
		    ** a 4gl assignment statement to set visible join
		    ** field to same value as hidden version of same field.
                    */
		    if (num_tests++ == 0)    /*write comment before first one*/
                        SIfprintf (outfp, ERx("\n%s%s\n"), p_wbuf,
			    ERget(F_FG0010_UndoChngComment));

                    SIfprintf (outfp, ERx("%s%s = %s;\n"), p_wbuf,
			(*tmp_mfcol)->alias, (char *)(*tmp_mfcol)->utility );
		}
	    }
	}
}
