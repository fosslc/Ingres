/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<lo.h>
# include	<ooclass.h>
# include	<si.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"
# include       <fglstr.h>

/**
** Name:	fglookup.c -	generate Lookup block.
**
** Description:
**	Generate 4gl for a ##GENERATE LOOKUP statement.
**
**		Example template file code:
**
**		    'ListChoices' =
**		    BEGIN
**		        IIint = -1;	** look_up not run
**		    ##  GENERATE LOOKUP		-- sets IIint
**			IF (IIint < 0) THEN
**			    ** lookup not available for current field
**			    IIint = CALLPROC help_field();
**			ELSE
**			    ** Look_up IS available for current field.
**			    ** User chose a value in callframe lookup above, or
**			    ** lookup was displayed but user didn't choose value
**			ENDIF;
**
**			IF (IIint > 0) THEN
**			    ** value was selected from Lookup list above
**			    SET_FORMS FORM (CHANGE = 1);
**			ENDIF;
**		    END
**
**		Where the "##GENERATE LOOKUP" statement means
**		to generate the following code:
**
**		INQUIRE_FORMS FIELD '' (IIobjname = NAME, IIint2 = TABLE);
**		IF (IIint2 != 1) THEN	       ** cursor in simple field **
**		    IF (IIobjname = '1st_lookup_fldname') THEN
**		        IIint = CALLFRAME look_up (II_QUERY=SELECT * FROM...);
**		    ELSEIF (IIobjname = '2nd_lookup_fldname') THEN
**		        IIint = CALLFRAME look_up (II_QUERY=SELECT * FROM...);
**		    ELSEIF ...
**		    ENDIF;
**		ELSEIF (IIobjname='iitf') THEN ** cursor in table field **
**                  ** Skip look_up() call if table field is empty (get
**		    ** error if read from or assign to an empty table field)
**	            **
**                  INQUIRE_FORMS TABLE '' (IIint2 = DATAROWS('iitf'));
**            	    IF (IIint2 > 0) THEN
**		        INQUIRE_FORMS TABLE '' (IIobjname = COLUMN);
**		        IF (IIobjname = '1st_lookup_column_name') THEN
**		            IIint = CALLFRAME look_up
**				 (II_QUERY=SELECT * FROM...);
**		        ELSEIF (IIobjname = '2nd_lookup_column_name') THEN
**		            IIint = CALLFRAME look_up
**				 (II_QUERY=SELECT * FROM...);
**		        ELSEIF ...
**		        ENDIF;
**		    ENDIF;
**		ENDIF;
**
**	This file defines:
**
**	IIFGlookup
**
** History:
**	9/5/89	(pete)	Initial version.
**	2/12/90 (pete)	Skip look_up() call if table field in Update
**			of Query mode and has zero rows.
**	9/90 (Mike S)	Give a hidden field for primary lookup column
**	31-jul-90 (blaise)
**		In ifelseifWrite() convert the column name to lowercase before
**		comparing it with IIobjname. Column names in non-INGRES
**		backends may be in uppercase. Bug #37750.
**	15-nov-91 (leighb) DeskTop Porting Change: added include of <cv.h>.
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
FUNC_EXTERN char *IIFGbuf_pad();

/* static's */
static VOID ifelseifWrite();
static VOID lookupWrite();
static LSTR *queryWrite();
static LSTR *qualWrite();
static LSTR *fieldWrite();
static LSTR *titlesWrite();
static LSTR *fldtitlesWrite();
static LSTR *colnamesWrite();
static i4   setsortcols();

static char *P_wbuf4;  /* p_wbufn == p_wbuf with 'n' more spaces added on */
static char *P_wbuf8;
static char *P_wbuf12;

static char	*G_infn;	/* name of input file currently processing */
static i4	G_line_cnt;	/* current line number in input file */

STATUS
IIFGlookup(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;	/* number of words in p_wordarray */
char	*p_wordarray[]; /* words on command line. */
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{

	MFJOIN **p_mfjoin = G_metaframe->joins;

	MFTAB *p_primt;	/* primary table in the join */
	MFTAB *p_lookt;	/* lookup table in the join */
	MFCOL *p_primc;	/* primary column in join */
	/*	MFCOL *p_lookc;		lookup column in join */

	int i;
	bool simpfld_lookup = FALSE;
	bool tblfld_lookup = FALSE;

        P_wbuf4 = IIFGbuf_pad(p_wbuf, PAD4);
        P_wbuf8 = IIFGbuf_pad(p_wbuf, PAD8);
        P_wbuf12= IIFGbuf_pad(p_wbuf, PAD12);

	G_infn = infn;
	G_line_cnt = line_cnt;

	if (nmbr_words > 0)
	{
            /* Arguments found following word "LOOKUP".
	    ** Note: "##GENERATE LOOKUP" already stripped.
	    */

	    /* following is a warning, not an error; processing continues */
            IIFGerr(E_FG004A_LookupTooManyArgs, FGCONTINUE, 2,(PTR) &line_cnt,
                (PTR) infn);
	}

	/* the initial INQUIRE statement is needed no matter what, so write
	** it now, before the loop begins.
	*/
        SIfprintf(outfp,
	  ERx("%sINQUIRE_FORMS FIELD '' (IIobjname = NAME, IIint2 = TABLE);\n"),
	    p_wbuf);

	for (i=0; i < G_metaframe->numjoins; i++, p_mfjoin++)
	{
	    p_primt = G_metaframe->tabs[(*p_mfjoin)->tab_1];
	    p_lookt = G_metaframe->tabs[(*p_mfjoin)->tab_2];

	    p_primc = p_primt->cols[(*p_mfjoin)->col_1];
	/*	    p_lookc = (p_lookt->cols)[(*p_mfjoin)->col_2];	*/

	    /* Assertion: because MFJOIN is sorted by the order of the
	    ** table on the VQ display, all the
	    ** JOIN_MASLUP entries will come before the JOIN_DETLUP entries.
	    ** Following code assumes this order.
	    */

	    if (((*p_mfjoin)->type) == JOIN_MASLUP)
	    {
		/* master lookup */
		
		if ((G_metaframe->mode & MFMODE) == MF_MASTAB)
		{
			goto MASTER_IN_TABLEFIELD;
		}

		if (!simpfld_lookup)	/* first master lookup */
		{
		    /* generate first 2 IF statements */

            	    SIfprintf(outfp,
			ERx("%sIF (IIint2 != 1) THEN\t\t%s\n"),
			p_wbuf, ERget(F_FG0012_SimpleField));

		    ifelseifWrite (P_wbuf4, p_primc->alias, TRUE);

		    lookupWrite (P_wbuf8, (*p_mfjoin));

	    	    simpfld_lookup = TRUE;
		}
		else			/* second & subsequent master lookups */
		{
		    /* generate ELSEIF & CALLFRAME lookup statements */

		    ifelseifWrite (P_wbuf4, p_primc->alias, FALSE);

		    lookupWrite (P_wbuf8, (*p_mfjoin));
		}
	    }
	    else if ((*p_mfjoin)->type == JOIN_DETLUP)
	    {
		/* detail lookup.
		** First JOIN_DETLUP join signals end of JOIN_MASLUP joins
		** due to sort order of MFJOIN entries.
		*/

	    MASTER_IN_TABLEFIELD:

		if (!tblfld_lookup)	/* first detail lookup */
		{
		    if (simpfld_lookup)	/* master lookups were seen */
		    {
			/* Close off the above master block with an ENDIF.
			** Then, start detail block off with an "ELSEIF"
			*/

            	    	SIfprintf(outfp, ERx("%sENDIF;\n"), P_wbuf4);
            	    	SIfprintf(outfp,
			    ERx("%sELSEIF (IIobjname = '%s') THEN\t%s\n"),
			    p_wbuf, TFNAME, ERget(F_FG0013_TableField));
		    }
		    else	/* only detail lookups exist */
		    {
			/* No master IF block generated before, so nothing
			** to close off. Start detail block with an "IF".
			*/

            	    	SIfprintf(outfp,
			    ERx("%sIF (IIobjname = '%s') THEN\t%s\n"),
			    p_wbuf, TFNAME, ERget(F_FG0013_TableField));
		    }
		    /* generate INQUIRE_FORMS TABLE & first IF statements */

		    /* explanatory comment */
            	    SIfprintf(outfp, ERx("%s%s\n%s%s\n%s%s\n"),
			P_wbuf4, ERget(F_FG0033_SkipLookup1),
			P_wbuf4, ERget(F_FG0034_SkipLookup2),
			P_wbuf4, ERget(F_FG0035_EndOfComment));

            	    SIfprintf(outfp,
	    ERx("%sINQUIRE_FORMS TABLE '' (IIint2 = DATAROWS('%s'));\n"),
			P_wbuf4, TFNAME);

            	    SIfprintf(outfp, ERx("%sIF (IIint2 > 0) THEN\n"), P_wbuf4);

            	    SIfprintf(outfp,
			ERx("%sINQUIRE_FORMS TABLE '' (IIobjname = COLUMN);\n"),
			P_wbuf8);

	 	    ifelseifWrite (P_wbuf8, p_primc->alias, TRUE);  /* IF */

		    lookupWrite (P_wbuf12, (*p_mfjoin));

	    	    tblfld_lookup = TRUE;
		}
		else		/* second & subsequent detail lookups */
		{
		    /* generate ELSEIF & CALLFRAME look_up() statements */

	 	    ifelseifWrite (P_wbuf8, p_primc->alias, FALSE);  /* ELSEIF*/

		    lookupWrite (P_wbuf12, (*p_mfjoin));
		}
	    }
	}	/* for() */

	if (simpfld_lookup || tblfld_lookup)
	{
	    if (tblfld_lookup)
       	        SIfprintf(outfp, ERx("%sENDIF;\n"), P_wbuf8);	/* indented 8 */

	    /* Master &/or Detail table(s) have lookups. Close off
	    ** above with a pair of ENDIF's;
	    */
       	    SIfprintf(outfp, ERx("%sENDIF;\n"), P_wbuf4);	/* indented 4 */
       	    SIfprintf(outfp, ERx("%sENDIF;\n"), p_wbuf);
	}

	return OK;
}

/*{
** Name:	lookupWrite - generate a CALLFRAME LOOKUP statement.
**
** Description:
**	Generate a CALLFRAME LOOKUP statement like:
**
**	CALLFRAME look_up (
**	    II_ROWS = 10;
**	    II_QUERY = SELECT DISTINCT lookup_col1, lookup_col2, lookup_col3, ..
**		       FROM lookuptbl
**		       ORDER BY lookup_colN, ...;
**	    II_QUALIFY = '',		** leave out if no qualification screen
**	    II_FIELD1 = lookup_colM,	** tells order to display lookup data
**		...
**	    II_FIELDn = lookup_colP,
**	    II_TITLES = 1,		** default titles unless override below
**	    II_FIELD_TITLE1 = "custom title for field (entered by user in VQ)",
**	       ...
**	    II_FIELD_TITLEn = "custom title for field (entered by user in VQ)",
**	    lookup_colA = byref(primary_table_lookup_field),
**	    lookup_colB = byref(lookup_displayed_field),  ** or byref(tbl.col)
**		...
**	    lookup_colN = byref(lookup_displayed_field)
**	);
**
** Inputs:
**	char	*wbuf		whitespace buffer to prepend to generated line
**	MFJOIN	*p_mfjoin	create a CALLFRAME LOOKUP for this join
**
** Outputs:
**	Writes a statement to the generated 4GL file.
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
**	9/6/89	(pete)	Initial Version.
*/
static VOID
lookupWrite (wbuf, p_mfjoin)
char	*wbuf;		/* whitespace buffer to prepend to generated line */
MFJOIN	*p_mfjoin;	/* create a CALLFRAME LOOKUP for this Lookup join */
{
	MFTAB *p_primt;	/* primary table in the join */
	MFTAB *p_lookt;	/* lookup table in the join */
	MFCOL *p_primc;	/* primary column in join */
	MFCOL *p_lookc;	/* lookup column in join */

	LSTR  *p_lstr_head;	/* head of linked list of strings */
	LSTR  *p_lstr;		/* last entry in linked list of strings */
	char  *wbuf4;		/* pad in 4 add'l spaces from CALLFRAME LOOKUP*/

	MFCOL *sortcols[DB_MAX_COLS];	/* Lookup table sort columns,
					** if any, get loaded in here
					** in sequential order.
					*/
	i4   nmbrsortcols;		/* number of items loaded in sortcols */
	char  *tfname;
	char  *dot;

        wbuf4 = IIFGbuf_pad(wbuf, PAD4);

	p_primt = G_metaframe->tabs[p_mfjoin->tab_1];
	p_lookt = G_metaframe->tabs[p_mfjoin->tab_2];

	p_primc = p_primt->cols[p_mfjoin->col_1];
	p_lookc = p_lookt->cols[p_mfjoin->col_2];

	SIfprintf(outfp, ERx("%sIIint = CALLFRAME look_up (\n"), wbuf);

	/* set the table field to 10 rows. */ 
	SIfprintf(outfp, ERx("%sII_ROWS = 10;\n"), wbuf4);

	/* allocate first entry in linked list */
        if ((p_lstr_head = IIFGals_allocLinkStr((LSTR *)NULL,
		LSTR_NONE)) == NULL)
        {
            IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2, (PTR) &G_line_cnt,
                        (PTR) G_infn);
        }

	nmbrsortcols = setsortcols (p_lookt, DB_MAX_COLS, sortcols);

	p_lstr = queryWrite(p_lookt, p_lstr_head, nmbrsortcols,
					sortcols);	/* II_QUERY = */
	p_lstr = qualWrite(p_lookt, p_lstr);		/* II_QUALIFY = */
	p_lstr = fieldWrite(p_lookc, p_lstr, nmbrsortcols,
				sortcols);		/* II_FIELDn = */
	p_lstr = titlesWrite(p_lstr);			/* II_TITLES = 1 */
	p_lstr = fldtitlesWrite(p_lstr, nmbrsortcols,
				sortcols);		/* II_FIELD_TITLEn = */
	p_lstr = colnamesWrite(p_primt->tabsect, p_primc, p_lookc, p_lookt,
					p_lstr);	/* col = byref(fld) */

	p_lstr->follow = LSTR_NONE;	/* last item in linked list */

        /* write out the LSTR structure to the generated 4gl file. */
        IIFGols_outLinkStr(p_lstr_head, outfp, wbuf4);

	/* close the CALLFRAME look_up statement with ");" */
	SIfprintf (outfp, ERx("%s);\n"), wbuf);

	/* If it succeeded, copy the value to the form */
	if (  p_primt->tabsect == TAB_MASTER
	   && (G_metaframe->mode & MFMODE) != MF_MASTAB
	   )
	{
		tfname = dot = ERx("");
	}
	else
	{
		tfname = TFNAME;
		dot = ERx(".");
	}
	SIfprintf(outfp, 
		  ERx("%sIF (IIint > 0) THEN\n%s%s%s%s = %s%s%s;\n%sENDIF;\n"),
		  wbuf, 
		  wbuf4, tfname, dot, p_primc->alias, 
		         tfname, dot, p_lookc->utility,
		  wbuf);
}

/* Fill up an array with the lookup table sort columns, in sequential order.
** This code assumes there are no gaps in the sort sequence numbers (i.e.
** (1,2,3, but never 1,2,4, etc). Tom promised me this is so!
*/
static i4
setsortcols(p_tab, maxsort, sortcols)
MFTAB	*p_tab;		/* lookup table to check for sort columns in. */
i4	maxsort;	/* maximum number of entries in sortcols array. */
MFCOL	*sortcols[];	/* write sort columns here (in order). */
{
	MFCOL **p_col = p_tab->cols;
	i4 i, nmbr;

	for (i=0, nmbr=0; (i < maxsort) && (i < p_tab->numcols); i++, p_col++)
	{
	    if ( (*p_col)->corder > 0 ) {
		sortcols[(*p_col)->corder - 1] = *p_col;
		nmbr++;
	    }
	}

	return nmbr;	/* returns number of sort cols loaded into array */
}

/* Generate the II_QUERY.
** This routine should be first one called. It writes the first entries to
** the linked list of strings.
*/
static LSTR
*queryWrite (p_tab, p_lstr, nmbrsortcols, sortcols)
MFTAB	*p_tab;		/* lookup table to generate code for */
LSTR	*p_lstr;	/* First linked list entry. WRITE TO THIS */
i4	nmbrsortcols;	/* number of entries in sortcols array. */
MFCOL	*sortcols[];	/* list of sort columns (in order). */
{
	MFCOL **p_lookc = p_tab->cols;	/* lookup column in join */
	i4 i;

	STcopy(ERx("II_QUERY = SELECT DISTINCT"), p_lstr->string);

	/* target list of columns: col1, col2, ... */
	for (i=0; i < p_tab->numcols; i++, p_lookc++)
	{
	    /* Include the column in the SELECT target list if
	    ** the column is displayed on the form or in a variable 
	    ** (COL_USED|COL_VARIABLE), OR if it
	    ** is the join field (COL_SUBJOIN), OR if the user entered
	    ** an Order for the column in the VQ.
	    */
	    if  (    ((*p_lookc)->flags & (COL_USED|COL_VARIABLE))
		  || ((*p_lookc)->flags & COL_SUBJOIN)
		  || ((*p_lookc)->corder > 0)
		)
	    {
		/* column should be included in SELECT target list */

        	if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_COMMA)) == NULL)
        	{
            	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
			(PTR) &G_line_cnt, (PTR) G_infn);
        	}
		STcopy( (*p_lookc)->name, p_lstr->string);
	    }
	}
	p_lstr->follow = LSTR_NONE;	/* no COMMA following last column */
	p_lstr->nl = TRUE;		/* force \n */


	/* FROM clause */
	if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_NONE)) == NULL)
	{
	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
		(PTR) &G_line_cnt, (PTR) G_infn);
	}
	STprintf( p_lstr->string, ERx("%sFROM %s"), PAD11, p_tab->name );
	p_lstr->nl = TRUE;	/* force \n */


	if (nmbrsortcols > 0)
	{
	    /* generate an ORDER BY clause */

	    i4  i;

	    if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_NONE)) == NULL)
	    {
	    	IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
			(PTR) &G_line_cnt, (PTR) G_infn);
	    }
	    STpolycat(2, PAD11, ERx("ORDER BY"), p_lstr->string );

	    for (i=0; i < nmbrsortcols; i++, sortcols++)
	    {
	        if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_COMMA)) == NULL)
	        {
	    	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
			(PTR) &G_line_cnt, (PTR) G_infn);
	        }
		STcopy ((*sortcols)->name, p_lstr->string);
	    }
	    p_lstr->nl = TRUE;			/* force \n */
	}

	p_lstr->follow = LSTR_SEMICOLON;	/* terminate statement */

	return p_lstr;
}

/* Generate the II_QUALIFY */
static LSTR
*qualWrite(p_tab, p_lstr)
MFTAB	*p_tab;	/* lookup table to generate code for */
LSTR	*p_lstr;	/* tack new linked list entries onto this */
{
	if ( (p_tab->flags) & TAB_QUALLUP)
	{
	    /* User asked for a Lookup Qualification screen in VQ */

	    if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) == NULL)
	    {
	    	IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
		    (PTR) &G_line_cnt, (PTR) G_infn);
	    }
	    STcopy(ERx("II_QUALIFY = ''"), p_lstr->string);
	    p_lstr->nl = TRUE;		/* force \n */
	}
	return p_lstr;
}

/* Generate the list of II_FIELDn arguments */
static LSTR
*fieldWrite(p_lookc, p_lstr, nmbrsortcols, sortcols)
MFCOL	*p_lookc;		/* lookup column in join */
LSTR	*p_lstr;	/* tack new linked list entries onto this */
i4	nmbrsortcols;	/* number of entries in sortcols array. */
MFCOL	*sortcols[];	/* list of sort columns (in order). */
{
	i4 i;

	/* one "II_FIELD1 = col1," entry for every VQ Order column */
	for (i=0; i < nmbrsortcols; i++, sortcols++)
	{
	    if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) == NULL)
	    {
	    	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
			(PTR) &G_line_cnt, (PTR) G_infn);
	    }
	    STprintf(p_lstr->string, ERx("II_FIELD%d = '%s'"), i+1,
				(*sortcols)->name );
	    p_lstr->nl = TRUE;		/* force \n */
	}

	if (nmbrsortcols == 0)
	{
	    /* VQ Lookup table field was empty (no user entries), so
	    ** use the lookup join column as the II_FIELD1 arg.
	    ** No other II_FIELD args.
	    */
	    if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) == NULL)
	    {
	    	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
			(PTR) &G_line_cnt, (PTR) G_infn);
	    }
	    STprintf(p_lstr->string, ERx("II_FIELD1 = '%s'"),
			p_lookc->name );
	    p_lstr->nl = TRUE;		/* force \n */
	}

	return p_lstr;
}

/* Generate the II_TITLES argument cause we always want the default title
** for columns that user doesn't specify a title for in VQ.
*/
static LSTR
*titlesWrite (p_lstr)
LSTR	*p_lstr;	/* tack new linked list entries onto this */
{
	if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) == NULL)
	{
	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
		    (PTR) &G_line_cnt, (PTR) G_infn);
	}
	STcopy(ERx("II_TITLES = 1"), p_lstr->string);
	p_lstr->nl = TRUE;	/* force \n */

	return p_lstr;
}

/* Generate the list of II_FIELD_TITLEn arguments */
static LSTR
*fldtitlesWrite (p_lstr, nmbrsortcols, sortcols)
LSTR	*p_lstr;	/* tack new linked list entries onto this */
i4	nmbrsortcols;	/* number of entries in sortcols array. */
MFCOL	*sortcols[];	/* list of sort columns (in order). */
{
	i4 i;

	/* generate a "II_FIELD_TITLE1 = col1," entry for every VQ Order col
	** that has a column title filled in.
	*/
	for (i=0; i < nmbrsortcols; i++, sortcols++)
	{
	    if ( ((*sortcols)->info != NULL) && ( *((*sortcols)->info) != EOS))
	    {
		/* user entered a title for this column */
	        if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) 
			== NULL)
	        {
	    	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
			(PTR) &G_line_cnt, (PTR) G_infn);
	        }
	        STprintf(p_lstr->string, ERx("II_FIELD_TITLE%d = '%s'"), i+1,
				(*sortcols)->info );

		p_lstr->nl = TRUE;	/* force \n */
	    }
	}
	return p_lstr;
}

/* Generate the list of  "col = BYREF(displayed_field),"  args. */
static LSTR
*colnamesWrite (tabsect, p_pcol, p_lcol, p_tab, p_lstr)
i4	tabsect;	/* which section in VQ are these tables in? */
MFCOL	*p_pcol;	/* join column from Primary table */
MFCOL	*p_lcol;	/* join column from Lookup table */
MFTAB	*p_tab;		/* lookup table */
LSTR	*p_lstr;	/* tack new linked list entries onto this */
{
	MFCOL **p_col = p_tab->cols;	/* used to loop thru all Lookup MFCOLs*/
	i4 i;

	/* The join column always gets a "col=byref(fld)" entry.
	** note that, unlike other entries, this one assigns:
	** LOOKUPTableJoinColumnName = byref(hidden_field)
	*/
	if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) == NULL)
	{
	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
		    (PTR) &G_line_cnt, (PTR) G_infn);
	}

	if (  tabsect == TAB_MASTER
	   && (G_metaframe->mode & MFMODE) != MF_MASTAB
	   )
	{
	    STprintf(p_lstr->string, ERx("%s = BYREF(%s)"), p_lcol->name,
			p_lcol->utility);
	}
	else
	{
	    STprintf(p_lstr->string, ERx("%s = BYREF(%s.%s)"), p_lcol->name,
			TFNAME, p_lcol->utility);
	}

	p_lstr->nl = TRUE;		/* force \n */

	/* Now look thru the MFCOL entries for the Lookup table
	** and Write out an entry for every column which is displayed or
	** in a variable. Entries look like:
	** 	LookupTableColumnName = byref(LookupTableFieldName)
	** (don't make an entry if the column has MFCOL.flags = COL_SUBJOIN,
	** cause that was already handled above).
	*/
	for (i=0; i < p_tab->numcols; i++, p_col++)
	{
	    if  (    ((*p_col)->flags & (COL_USED|COL_VARIABLE))
		  && (((*p_col)->flags & COL_SUBJOIN) == 0)
		)
	    {
		/* this column needs to be added to the BYREF list */
		if ((p_lstr = IIFGals_allocLinkStr(p_lstr, LSTR_SEMICOLON)) 
				== NULL)
		{
	    	    IIFGerr(E_FG0022_Query_NoDynMem, FGFAIL, 2,
		    	(PTR) &G_line_cnt, (PTR) G_infn);
		}

		if (  tabsect == TAB_MASTER
	   	   && (G_metaframe->mode & MFMODE) != MF_MASTAB
	   	   )
		{
		    STprintf(p_lstr->string, ERx("%s = BYREF(%s)"),
			(*p_col)->name, (*p_col)->alias);
		}
		else
		{
		    STprintf(p_lstr->string, ERx("%s = BYREF(%s.%s)"),
			(*p_col)->name, TFNAME, (*p_col)->alias);
		}

		p_lstr->nl = TRUE;	/* force \n */
	    }
	}

	return p_lstr;
}

/* write out either an IF or an ELSEIF statement */
static VOID 
ifelseifWrite (wbuf, name, if_or_elseif)
char	*wbuf;		/* whitespace buffer to prefix generated line with */
char 	*name;		/* field/column name to write into IF statement */
bool	if_or_elseif;	/* TRUE = make it an IF; otherwise an ELSEIF */
{
	CVlower(name);
	SIfprintf(outfp,
	    ERx("%s%s (IIobjname = '%s') THEN\n"), wbuf,
	    (if_or_elseif ? ERx("IF") : ERx("ELSEIF")), name);
}
