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
# include	<fglstr.h>


/**
** Name:	fgchkchg.c -	code to check for changes to join fields.
**
** Description:
**	Check for changes to join fields.
**
**	This file defines:
**
**	IIFGchkchg
**	gen_chkchg
**
** History:
**	7/5/89 (pete)	Initial Version.
**	22-oct-1990 (pete)
**		Fix bug 34043 where bad code was generated in MDupdate frame
**		with multiple join columns, where some but not all the join
**		columns are displayed.
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
LSTR *IIFGals_allocLinkStr();
VOID IIFGols_outLinkStr();
FUNC_EXTERN VOID IIFGerr();

/* static's */
static STATUS gen_chkchg();

/*{
** Name:	IIFGchkchg - generate code to check for join field change.
**
** Description:
**	Generate 4gl code to check for a change to a join field. For example,
**	for an UPDATE or MODIFY VQ with displayed joinfields named 'cust',
**	'div' and a variable-generating joinfiled named 'ord' generate:
**
**		** Check for change to join field(s) **
**		IIupdrules = 0;
**		IF (cust != ii_cust) OR (div != ii_div) THEN
**			IIupdrules = 2;	** a join field was changed **
**		ENDIF;
**		IF (ord != ii_ord) THEN
**			IIupdrules = IIupdrules + 1;	** a join field was changed **
**		ENDIF;
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
*/
STATUS
IIFGchkchg(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4      nmbr_words;     /* number of words in p_wordarray */
char    *p_wordarray[]; /* words on command line. */
char    *p_wbuf;        /* whitespace to tack to front of generated code */
char    *infn;          /* name of input file currently processing */
i4      line_cnt;       /* current line number in input file */
{
        if (nmbr_words <= 0)
        {
            /*Note: "##generate check_change" already stripped*/

            IIFGerr (E_FG0032_MissingArgs, FGFAIL, 2,(PTR) &line_cnt,
	    	(PTR) infn);
        }

        CVlower(p_wordarray[0]);

        if (STequal(p_wordarray[0],ERx("join_fields")))
	{
	     if (gen_chkchg (G_metaframe, p_wbuf) != OK)
		return FAIL;
	}
        else
        {
            /* unknown argument on ##generate check_change statement */
            IIFGerr (E_FG0033_UnknownArg, FGFAIL,
                        3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
        }

	return OK;
}

/*{
** Name:	gen_chkchg - generate 4gl to check for join field changes.
**
** Description:
**
** Inputs:
**	METAFRAME *mf		pointer to METAFRAME.
**	char    *p_wbuf		whitespace to tack to front of generated code
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
**	7/5/89	(pete)	Initial Version.
*/
static STATUS
gen_chkchg(mf, p_wbuf)
METAFRAME *mf;
char    *p_wbuf;        /* whitespace to tack to front of generated code */
{
	MFTAB **p_mftab = mf->tabs;
	register MFJOIN **p_mfjoin;
	register i4  i;
	i4 num_disp=0;   /* number of displayed columns with hidden copies */
	i4 num_var=0;   /* number of variable columns with hidden copies */
	STATUS stat=OK;

        LSTR *p_lstr_head;      /* for working with linked list of strings*/
        LSTR *p_lstr;
	char indent4[80];	/* Indent 4 past p_wbuf */

	STcopy(p_wbuf, indent4);
	STcat(indent4, PAD4);

	/* for every join */
	for (i=0, p_mfjoin = mf->joins; i < mf->numjoins; i++, p_mfjoin++)
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
		    && ((*tmp_mfcol)->flags & COL_USED)
		   )
		{
		    /* Field is displayed & has a hidden field version,
		    ** so generate (part of) a 4gl 'if' statement to test
		    ** for changes.
                    ** Since the generated 'if' statement can be quite long,
		    ** use Mike's list string allocation & print routines.
		    ** (these LinkStr routines manage their own memory tag).
                    */

		    if (num_disp++ == 0)
		    {
			/* first one */

                        if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)NULL,
                             LSTR_NONE)) == NULL)
                        {
                            stat = FAIL;
                    	    break;
                        }

                        p_lstr_head = p_lstr;   /* save head of list */

                        STprintf (p_lstr->string, ERx("IF (%s != %s)"),
				(*tmp_mfcol)->alias,
				(char *)(*tmp_mfcol)->utility );
		    }
		    else
		    {
                        if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
                            LSTR_NONE)) == NULL)
                        {
                            stat = FAIL;
                            break;
                        }

                        STprintf (p_lstr->string,ERx("OR (%s != %s)"),
				(*tmp_mfcol)->alias,
				(char *)(*tmp_mfcol)->utility );
		    }

		}
	    }
	}
	
	if (stat != OK)
	    return stat;

	if (num_disp > 0)
	{
	    /* need to write out some 4gl code */

	    /* terminate the 'if' statement with a 'then' */
            if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
                        LSTR_NONE)) == NULL)
            {
                return FAIL;
            }
	    STcopy (ERx(" THEN"), p_lstr->string);
	    p_lstr->nl = TRUE;

            SIfprintf (outfp, ERx("\n%s%s\n"), p_wbuf,
			ERget(F_FG000E_CheckChngComnt));

            SIfprintf (outfp, ERx("%sIIupdrules = 0;\n"), p_wbuf);

            /* Write out the 'if' stmt in the LSTR structure
	    ** built above.
	    */
            IIFGols_outLinkStr (p_lstr_head, outfp, p_wbuf);

            SIfprintf (outfp, ERx("%s%sIIupdrules = 1;\t%s\n"),
			p_wbuf, PAD4,
			ERget(F_FG000F_JoinFldChanged));

	    SIfprintf (outfp, ERx("%sENDIF;\n"), p_wbuf);
	}

	/* Now check for variable-generating join fields with hidden copies */
	for (i=0, p_mfjoin = mf->joins; i < mf->numjoins; i++, p_mfjoin++)
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
		    && (((*tmp_mfcol)->flags & (COL_USED|COL_VARIABLE))
			  == COL_VARIABLE)
		   )
		{
		    /* Field generates a variable & has a hidden field version,
		    ** so generate (part of) a 4gl 'if' statement to test
		    ** for changes.
                    ** Since the generated 'if' statement can be quite long,
		    ** use Mike's list string allocation & print routines.
		    ** (these LinkStr routines manage their own memory tag).
                    */

		    if (num_var++ == 0)
		    {
			/* first one */

                        if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)NULL,
                             LSTR_NONE)) == NULL)
                        {
                            stat = FAIL;
                    	    break;
                        }

                        p_lstr_head = p_lstr;   /* save head of list */

                        STprintf (p_lstr->string, 
				ERx("IF (%s != %s)"),
				(*tmp_mfcol)->alias,
				(char *)(*tmp_mfcol)->utility );
		    }
		    else
		    {
                        if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
                            LSTR_NONE)) == NULL)
                        {
                            stat = FAIL;
                            break;
                        }

                        STprintf (p_lstr->string,ERx("OR (%s != %s)"),
				(*tmp_mfcol)->alias,
				(char *)(*tmp_mfcol)->utility );
		    }

		}
	    }
	}
	
	if (stat != OK)
	    return stat;

	if (num_var > 0)
	{
	    /* need to write out some 4gl code */

	    /* terminate the 'if' statement with a 'then' */
            if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
                        LSTR_NONE)) == NULL)
            {
                return FAIL;
            }
	    STcopy (ERx(" THEN"), p_lstr->string);
	    p_lstr->nl = TRUE;

	    if (num_disp == 0)
	    {
		SIfprintf (outfp, ERx("\n%s%s\n"), p_wbuf,
			    ERget(F_FG000E_CheckChngComnt));

		SIfprintf (outfp, ERx("%sIIupdrules = 0;\n"), p_wbuf);
	    }

            /* Write out the 'if' stmt in the LSTR structure
	    ** built above.
	    */
	    SIfprintf (outfp, ERx("%sIF IIupdrules = 0 THEN\n"), p_wbuf);
            IIFGols_outLinkStr (p_lstr_head, outfp, indent4);

            SIfprintf (outfp, ERx("%s%sIIupdrules = 2;\t%s\n"),
			indent4, PAD4,
			ERget(F_FG000F_JoinFldChanged));

	    SIfprintf (outfp, ERx("%sENDIF;\n"), indent4);
	    SIfprintf (outfp, ERx("%sENDIF;\n"), p_wbuf);
	}

	return (stat);
}
