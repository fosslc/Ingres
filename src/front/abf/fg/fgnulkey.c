/*
** Copyright (c) 1990, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<er.h>
# include	<lo.h>
# include	<ug.h>
# include       <st.h>
# include	<ooclass.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	<si.h>
# include	"framegen.h"

/**
** Name:	fgnulkey.c -	Set values of Flag fields for NULLable keys.
**
** Description:
**
**	This file defines:
**
**	IIFGsf_SetNullKeyFlags
**	genSetNulKeys
**
** History:
**	3/16/90	(pete)	Initial Version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
GLOBALREF FILE *outfp;
GLOBALREF METAFRAME *G_metaframe;

FUNC_EXTERN bool  IIFGnkf_nullKeyFlags();
FUNC_EXTERN MFTAB *IIFGpt_pritbl();
FUNC_EXTERN i4    IIFGnkc_nullKeyCols();
FUNC_EXTERN MFCOL *IIFGmj_MasterJoinCol();
FUNC_EXTERN char *IIFGbuf_pad();


/* static's */
static VOID genSetNulKeys();
static const char _null[]            = ERx("");
static const char _dot[]             = ERx(".");

/*{
** Name:  IIFGsf_SetNullKeyFlags - Gen 4gl to set Nullable key Flag fields.
**
** Description:
**	Process ##GENERATE SET_NULL_KEY_FLAGS  [ MASTER | DETAIL ]  statement.
**
**	Create 4gl statements to set the values of the Flag fields that we
**	need to use when a table has NULLable keys and/or NULLable optimistic
**	locking columns.
**	Generated statements look like:
**
**		IF (<name of NULLable hidden key field or column> IS NULL) THEN
**		    iiNullKeyFlag1 = 1;
**		    <hidden field> = 0;	[this line only added in special case
**						- see below]
**		ELSE
**		    iiNullKeyFlag1 = 0;
**		ENDIF;
**		...
**
**	Special case for nullable columns which are being used for optimistic
**	locking and whose datatype is integer:
**
**	The set clause for updating an integer optimistic locking column is:
**		SET....<integer col> = <integer hidden field> + 1
**	This won't work if the value of the hidden field is null, so we set
**	the hidden field to 0 before doing the update.
**
** Inputs:
**	i4	nmbr_words;	** number of items in 'p_wordarray'
**	char	*p_wordarray[]; ** words in command line
**	char	*p_wbuf;	** left margin padding for output line
**	char	*infn;		** name of input file currently processing
**	i4	line_cnt;	** current line number in input file
**
** Outputs:
**	write generated declaration lines to 'outfp'.
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
**	3/16/90 (pete)	Initial Version.
**	16-feb-94 (blaise)
**		Updated code to handle nullable optimistic locking columns
**		as well as nullable key columns.
*/
STATUS
IIFGsf_SetNullKeyFlags(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	CVlower(p_wordarray[0]);

	if (nmbr_words <= 0)  /*##GENERATE SET_NULL_KEY_FLAGS already stripped*/
	{
	    IIFGerr (E_FG0059_MissingArgs, FGFAIL, 2,
				(PTR) &line_cnt, (PTR) infn);
	}
        else if (
		!STequal(p_wordarray[0], ERx("master"))
	     && !STequal(p_wordarray[0], ERx("detail"))
		)
        {
            IIFGerr (E_FG005B_UnknownArg, FGFAIL,
                        3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
        }
        else if (nmbr_words > 1)
        {
            /* note: this is a warning; processing continues */
            IIFGerr (E_FG005A_TooManyArgs, FGCONTINUE,
                        4, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0],
			(PTR) p_wordarray[1] );
        }


	/* if code to set nullKeyFlags required, then generate it */
	if (IIFGnkf_nullKeyFlags())
	    genSetNulKeys ( (STequal(p_wordarray[0], ERx("master")))
				 ? TAB_MASTER : TAB_DETAIL, p_wbuf);
	else
	    ;	/* generate nothing */

	return OK;
}

/*{
** Name:	genSetNulKeys - generate code to set iiNullKeyFlag fields.
**
** Description:
**
** Inputs:
**	i4	tabsect		table to process (TAB_MASTER or TAB_DETAIL;
**				see MFTAB.tabsect)
**	char	*p_wbuf		buffer to prepend on generated code.
**
** Outputs:
**	Writes to generated 4gl file
**
**	Returns:
**		VOID
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	3/16/90	(pete)	Initial Version.
**	16-feb-94 (blaise)
**		Add special case for columns which are being used for
**		optimistic locking and which have an integer datatype. See
**		description section of IIFGsf_SetNullKeyFlags() for details.
*/
static VOID
genSetNulKeys(tabsect, p_wbuf)
i4	tabsect;	/* process this table section */
char	*p_wbuf;	/* buffer of whitespace */
{
	MFTAB *p_tab;
	i4   colnum;
	i4   numkeycols;
	i4   keycols [DB_GW2_MAX_COLS];
	register MFCOL *p_col;
	MFCOL *p_col2;
        char *hidden;   /* hidden field name */
        bool hidintf;   /* TRUE if the hidden field is in a tablefield */
	char *p_wbuf4;  /* p_wbuf with 4 extra blanks added onto it */

	/* find primary table for this 'tabsect' */
	if ((p_tab = IIFGpt_pritbl(G_metaframe, tabsect)) == (MFTAB *)NULL)
	    return;

	/* find NULLable key columns for this table */
        if ((numkeycols = IIFGnkc_nullKeyCols(p_tab, keycols)) <= 0)
	    return;

	p_wbuf4 = IIFGbuf_pad (p_wbuf, PAD4);	/* p_wbuf + "    " */

	/* loop thru NULLable key columns for this table and
	** write out IF/ELSE statements to set iiNullKeyFlag fields
	*/
        for (colnum = 0; colnum < numkeycols; colnum++ )
        {
                p_col = p_tab->cols[keycols[colnum]];
                /*
                ** For the detail join field, the hidden field name is
                ** a simple field associated with the master join field,
		** and thus not in the tablefield.
                */
                if ( (p_tab->tabsect == TAB_DETAIL) &&
                     ((p_col->flags & COL_SUBJOIN) != 0) )
                {
		    /* detail join column. find joining master column */
                    if ((p_col2 = IIFGmj_MasterJoinCol(G_metaframe, p_tab,
					keycols[colnum]))
					== (MFCOL *) NULL)
	    		return;	/* Join not found; something is wrong.
				** ##GENERATE QUERY, which should follow this,
				** also catches this, and will report error.
				*/

                    hidden = (char *)p_col2->utility;
                    hidintf = FALSE;
                }
                else
                {
                    hidden = (char *)(p_col->utility);
                    hidintf =      tabsect == TAB_DETAIL
				|| (G_metaframe->mode & MFMODE) == MF_MASTAB;
                }

	        if (colnum == 0)
	        {
		    SIfprintf(outfp, ERx("%s%s\n"), p_wbuf,
			ERget(F_FG0038_SetNulKeyComnt));
	        }

	        SIfprintf (outfp, ERx("%sIF (%s%s%s IS NULL) THEN\n"),
	            p_wbuf,
		    hidintf ? TFNAME : _null,
		    hidintf ? _dot : _null,
		    hidden);

	        SIfprintf (outfp, ERx("%s%s%d = 1;\n"),
		    p_wbuf4, ERget(F_FG0037_FlagFieldName), colnum+1);

		/* special case for nullable integer optimistic locking cols */
		if ((p_col->flags & COL_OPTLOCK) &&
			(abs(p_col->type.db_datatype) == DB_INT_TYPE))
		{
			SIfprintf (outfp, ERx("%s%s%s%s = 0;\n"),
				p_wbuf4, 
				hidintf ? TFNAME : _null,
				hidintf ? _dot : _null,
				hidden);
		}

	        SIfprintf (outfp, ERx("%sELSE\n"), p_wbuf);

	        SIfprintf (outfp, ERx("%s%s%d = 0;\n"),
		    p_wbuf4, ERget(F_FG0037_FlagFieldName), colnum+1);

	        SIfprintf (outfp, ERx("%sENDIF;\n"), p_wbuf);
	}
}
