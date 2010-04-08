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

/* NOTE NOTE NOTE:
** 2/14/90 (pete) I ended up not needing this (yet), so I dropped it for now.
** This file is NOT CURRENTLY USED by code generator. Also, this file
** was never completed (routine copyVisToHid was never worked on, except for
** the header comments).
*/

/**
** Name:	fgcpvtoh.c -	copy visible fields to corresponding hiddn flds.
**
** Description:
**	Copy contents of visible fields of a certain type
**	(for example Key fields) to the corresponding hidden fields.
**
**	This file Defines:
**
**	IIFGcvh_copyVisToHid	test for errors & call copyVisToHid.
**	copyVisToHid
**
** History:
**	2/90	(pete)	Initial Version.
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
static VOID copyVisToHid();

/*{
** Name:	IIFGcvh_copyVisToHid - Gen code to copy visible flds to hidden.
**
**	NOTE: THIS SHOULD PROBABLY BE BROKEN INTO TWO STATEMENTS -- ONE
**	FOR SIMPLE FIELDS, AND ONE FOR TABLE FIELDS. Also, EXPOSE THE
**	UNLOADTABLE STATEMENT IN THE TEMPLATE FILE.
**
** Description:
**	Generate 4gl code to copy values in visible fields to corresponding
**	hidden fields. For example,
**	For a Master-only UPDATE frame with keyfields named 'custno',
**	& 'div' generate (note that IIFGinit sets name of hidden
**	versions of join fields):
**
**		** reset hidden versions of key fields **
**		iih_custno = custno;
**		iih_div = div;
**
**	For a Master-Detail UPDATE frame with master keyfields named 'custno',
**	& 'div' and Detail key fields named ord_no and join field 'xno'
**	generate:
**		-- MISSING --
**
**	Example of template file statement:
**
**	## GENERATE COPY_VISIBLE_TO_HIDDEN KEY_FIELDS
**	Currently, the only valid third argument is "key_fields"
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
**	2/90	(pete)	Initial Version.
*/
STATUS
IIFGcvh_copyVisToHid(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4      nmbr_words;     /* number of words in p_wordarray */
char    *p_wordarray[]; /* words on command line. */
char    *p_wbuf;        /* whitespace to tack to front of generated code */
char    *infn;          /* name of input file currently processing */
i4      line_cnt;       /* current line number in input file */
{
        CVlower(p_wordarray[0]);

        if (nmbr_words <= 0)
        {
            /*Note: "##generate copy_visible_to_hidden" already stripped*/
            /* note that this is a warning, and processing continues */
            IIFGerr (E_FG0054_MissingArgs, FGCONTINUE, 2, (PTR) &line_cnt,
                        (PTR) infn);
        }
        else if (nmbr_words > 1)
        {
            /* note that this is a warning, and processing continues */
            IIFGerr (E_FG0055_TooManyArgs, FGCONTINUE,
                        2, (PTR) &line_cnt, (PTR) infn);
        }
        else if ( (nmbr_words == 1) &&
                  (!STequal(p_wordarray[0],ERx("key_fields")))
                )
        {
            /* note that this is a warning; processing continues */
            IIFGerr (E_FG0056_UnknownArg, FGCONTINUE,
                        3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
        }

	if (FALSE)
	     copyVisToHid (G_metaframe, p_wbuf);

	return OK;
}

/*{
** Name:	copyVisToHid - generate 4gl to copy visible to hidden.
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
**	2/90	(pete)	Initial Version.
*/
static VOID
copyVisToHid(mf, p_wbuf)
METAFRAME *mf;
char    *p_wbuf;        /* whitespace to tack to front of generated code */
{

/* NOTE: THIS CODE WAS STOLEN FROM ANOTHER ROUTINE AND NEVER
** CHANGED. THIS CODE MUST BE CUSTOMIZED FOR THIS APPLICATION
** BEFORE USE. It currently has nothing to do with the requirements
** of the routine described in the header!!
*/

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
