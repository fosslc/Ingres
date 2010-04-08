/*
**  Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<lo.h>
# include	<cv.h>
# include	<ex.h>
# include	<st.h>
# include	<si.h>
# include	<ooclass.h>
# include	<metafrm.h>
# include	<abclass.h>
# include	"framegen.h"
# include	<fglstr.h>
# include	<erfg.h>

/**
** Name:	fgquery.c -	generate query in output 4gl file.
**
** Description:
**	  These routines produce the four kinds of queries generated from 
**	  a template file:
**		delete		(master or detail)
**		insert		(master or detail)
**		select		(master, detail, or master_detail)
**		update		(master or detail)
**	
**	This file defines:
**
**	IIFGquery		Generate a 4GL query
**
** History:
**	15 may 1989 (Mike S)	Initial version
**	11 jul 1989 (pete)	Added support for new 'NOTERM' argument.
**	12 jul 1989 (pete)	Changed so modify & display frames use
**				qualification function.
**	6-dec-92 (blaise)
**		Added support for optimistic locking
**	20-apr-93 (blaise)
**		Check that the frame uses optimistic locking before adding
**		optimistic locking columns to the where clause.
**	09-sep-93 (cmr) Get rid of superfluous optimistic locking code (above)
**		that was causing an incorrect where clause to be generated
**		for updates and deletes.  Also changed numcols to numkcols
**		to avoid confusion.
**	15-jan-94 (blaise)
**		Reinstate code which was deleted in previous change. It
**		wasn't superfluous! (bug #57546)
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-Aug-2009 (kschendel) b121804
**	    Fix void * type mistakes.
**/

/* # define's */

/*	Sections for query */
# define SEC_BAD	-1
# define SEC_MASTER	0
# define SEC_DETAIL	1
# define SEC_MD		2

/* GLOBALREF's */
GLOBALREF i4  G_fgqline;
GLOBALREF char *G_fgqfile;

/* extern's */
FUNC_EXTERN	MFTAB   *IIFGpt_pritbl();
FUNC_EXTERN	i4	IIFGkc_keycols();
FUNC_EXTERN	LSTR	*IIFGwc_whereClause();
FUNC_EXTERN	LSTR	*IIFGfc_fromClause();
FUNC_EXTERN	LSTR	*IIFGac_assignClause();
FUNC_EXTERN	LSTR	*IIFGrc_restrictClause();
FUNC_EXTERN	LSTR	*IIFGic_intoClause();
FUNC_EXTERN	LSTR	*IIFGjc_joinClause();
FUNC_EXTERN	LSTR	*IIFGqc_qualClause();
FUNC_EXTERN	LSTR	*IIFGoc_orderClause();
FUNC_EXTERN	LSTR	*IIFGsc_setClause();
FUNC_EXTERN	LSTR	*IIFGvc_valuesClause();
FUNC_EXTERN	VOID	IIFGbsBeginSection();
FUNC_EXTERN	VOID	IIFGesEndSection();
FUNC_EXTERN	VOID	IIFGti_tblindx();

extern FILE *outfp;
extern METAFRAME *G_metaframe;

/* static's */
static VOID fgqdelete();
static VOID fgqinsert();
static VOID fgqselect();
static VOID fgqmselect();
static VOID fgqdselect();
static VOID fgqupdate();
static VOID check_options();
static i4   check_section();

static const char _and[] 		= ERx("AND");
static const char _delete[] 		= ERx("delete");
static const char _detail[] 		= ERx("detail");
static const char _insert[] 		= ERx("insert");
static const char _select[] 		= ERx("select");
static const char _master[] 		= ERx("master");
static const char _master_detail[] 	= ERx("master_detail");
static const char _null[]   		= ERx("");
static const char _repeat[] 		= ERx("repeated");
static const char _noterm[] 		= ERx("noterm");
static const char _repeated[] 	= ERx("REPEATED ");
static const char _update[] 		= ERx("update");

static char *fgpad;
static i4   sectdx[TAB_DETAIL+1][2];

/*{
** Name:	IIFGquery	- Generate a 4GL Query
**
** Description:
**	Receive a line from the template file, consisting of
**		query_type section [repeat]
**	along with the left-hand margin string and the current file and line
**	(for error messages).  Generate the query and write it to the output
**	file.
**
** Inputs:
**	nmbr_words	i4		number of words in the following array
**	p_wordarray	char *[]	words in the query line
**	p_wbuf		char *		left-margin padding
**	infn		char *		template file name
**	line_cnt	i4		current line in template file
**
** Outputs:
**	none
**
**	Returns:
**		STATUS			OK	if query was produced
**					FAIL	otherwise
**
**	Exceptions:
**		none
**
** Side Effects:
**		Query is written to output file
**
** History:
**	15 may 1989 (Mike S)	Initial version
**	22 jun 1989 (Pete)	Change _repeat from "repeat" to "repeated".
**	11 jul 1989 (Pete)	Add support for new optional param: "NOTERM".
**	24-feb-91 (seg)
**	    EXdeclare takes (STATUS (*)()), not (EX (*)())
*/
STATUS
IIFGquery(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;	/* Number of words in command */
char	*p_wordarray[]; /* Command line */
char	*p_wbuf;	/* Left-margin padding */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	register char 	*query_type = p_wordarray[0];
	VOID 		(*qry_rtn)();
	STATUS 		FEjmp_handler();
	EX_CONTEXT	context;
	OOID		frame_type;

	/* Set up global variables */
	G_fgqline = line_cnt;
	G_fgqfile = infn;
	fgpad = p_wbuf;

	/* establish exception handler */
	if (EXdeclare(FEjmp_handler, &context) != OK)
	{
		/* Clean up and return failure */
		EXdelete();
		IIFGfls_freeLinkStr();
		return(FAIL);		
	}
	if (nmbr_words <= 1)
	{
		IIFGqerror(E_FG0013_Query_InsfArgs, FALSE, 0);
	}

	/* Call indicated query routine */
	CVlower(query_type);
	if ( !STcompare(query_type, _select) )
		qry_rtn = fgqselect;
	else if ( !STcompare(query_type, _update) )
		qry_rtn = fgqupdate;
	else if ( !STcompare(query_type, _insert) )
		qry_rtn = fgqinsert;
	else if ( !STcompare(query_type, _delete) )
		qry_rtn = fgqdelete;
	else
		IIFGqerror(E_FG0015_Query_InvType, FALSE, 1, query_type);

	frame_type = ((USER_FRAME *)(G_metaframe->apobj))->class;
	(*qry_rtn)(frame_type, nmbr_words-1, p_wordarray+1);

	/* 
	** If we returned to here, no error occured.  Clean up and 
	** return success.
	*/
	EXdelete();
	return(OK);
}

/*
**	Produce a DELETE
*/
static VOID
fgqdelete(frame_type, nmbr_words, p_wordarray)
OOID	frame_type;	/* Frame type */
i4	nmbr_words;	/* Number of words in command */
char	*p_wordarray[]; /* Command line */
{
	MFTAB 	*table;
	bool 	repeat = FALSE;
	bool 	terminate = TRUE;
	i4 	keycols[DB_GW2_MAX_COLS];
	i4	numkcols;
	bool	intblfld;
	i4	section;

	/* Check number of arguments */
	if (nmbr_words < 1 )
		IIFGqerror(E_FG0013_Query_InsfArgs, FALSE, 0);
	if (nmbr_words > 3 )
		IIFGqerror(E_FG0014_Query_TooManyArgs, FALSE, 0);

	/* Check that the section is valid and contains a primary table */
	section = check_section(p_wordarray[0]);
	if ( (section != SEC_MASTER) && (section != SEC_DETAIL) )
		IIFGqerror(E_FG0020_Query_BadSect, FALSE, 1, p_wordarray[0]);
	if ( (table = IIFGpt_pritbl(G_metaframe, section)) == NULL)
		IIFGqerror(E_FG0021_Query_NoPriTbl, TRUE, 1, p_wordarray[0]);

	/* Check validity of frame type */
	if (frame_type != OC_UPDFRAME)
		IIFGqerror(E_FG0018_Query_FrameType, TRUE, 2, _delete, 
			   (PTR)&frame_type);

	/* Check that the primary table contains a unique key */
	if ( (numkcols = IIFGkc_keycols(table, keycols)) == 0)
		IIFGqerror(E_FG0019_Query_NoKey, TRUE, 1, p_wordarray[0]);

	/*
	** If we're using optimistic locking, then any optimistic locking
	** columns need to go into the where clause too
	*/
	if (G_metaframe->mode & MF_OPTIM)
		numkcols += IIFGolc_optLockCols(table, keycols, numkcols);

	/* Check for a repeat query & whether to terminate query with ';' */
	if (nmbr_words > 1)
		check_options(p_wordarray+1, nmbr_words-1, &repeat, &terminate); 

	/* See if we're in a tablefield */
	intblfld = table->tabsect > TAB_MASTER 	
		|| (G_metaframe->mode & MFMODE) == MF_MASTAB;
	/* 
	** Good enough.  Output the "DELETE FROM" line and the where clause.
	*/
	SIfprintf(outfp, ERx("%s%sDELETE FROM %s\n"), 
		  fgpad, (repeat ? _repeated : _null ), table->name);
	IIFGols_outLinkStr(
		IIFGwc_whereClause(table, numkcols, keycols, intblfld,terminate),
		outfp, fgpad);
}

/*
**	Produce an INSERT
** History:
**	3-dec-92 (blaise)
**		If we're using optimistic locking, add any optimistic locking
**		columns to the list passed to IIFGwc_whereClause
*/
static VOID
fgqinsert(frame_type, nmbr_words, p_wordarray)
OOID	frame_type;	/* Frame type */
i4	nmbr_words;	/* Number of words in command */
char	*p_wordarray[]; /* Command line */
{
	MFTAB 	*table;
	bool 	repeat = FALSE;
	bool 	terminate = TRUE;
	bool	intblfld;
	bool	assign;
	i4	section;
	char	*secname;

        /* Check number of arguments */
	if (nmbr_words < 1 )
		IIFGqerror(E_FG0013_Query_InsfArgs, FALSE, 0);
	if (nmbr_words > 3 )
		IIFGqerror(E_FG0014_Query_TooManyArgs, FALSE, 0);

        /* Check that the section is valid and contains a primary table */
	section = check_section(p_wordarray[0]);
	if ( (section != SEC_MASTER) && (section != SEC_DETAIL) )
		IIFGqerror(E_FG0020_Query_BadSect, FALSE, 1, p_wordarray[0]);
	secname = ERget((section == SEC_MASTER) ? 
					F_FG0018_Master : F_FG0019_Detail);
	if ( (table = IIFGpt_pritbl(G_metaframe, section)) == NULL)
		IIFGqerror(E_FG0021_Query_NoPriTbl, TRUE, 1, p_wordarray[0]);

        /* 
	** Check validity of frame type.  For an append frame, we can use the
	** assignment strings from the metaframe 
	*/
	if (frame_type == OC_APPFRAME)
		assign = TRUE;
	else if (frame_type == OC_UPDFRAME)
		assign = FALSE;
	else
		IIFGqerror(E_FG0018_Query_FrameType, TRUE, 2, _insert, 
			   (PTR)&frame_type);

	/* Check for a repeat query & whether to terminate query with ';' */
	if (nmbr_words > 1)
		check_options(p_wordarray+1, nmbr_words-1, &repeat, &terminate); 

        /* See if we're in a tablefield */
	intblfld = table->tabsect > TAB_MASTER
		|| (G_metaframe->mode & MFMODE) == MF_MASTAB;

        /*
	** Good enough.  Output the "INSERT" line and the into and values 
	** clauses.
	*/
	IIFGbsBeginSection(outfp, ERget(F_FG0016_Insert), secname, FALSE);
	SIfprintf(outfp, ERx("%s%sINSERT\n"), fgpad, repeat ? _repeated : _null);
	IIFGols_outLinkStr(IIFGic_intoClause(table), outfp, fgpad);
	IIFGols_outLinkStr(IIFGvc_valuesClause(table,intblfld,assign,terminate),
			   outfp, fgpad);
	IIFGesEndSection(outfp, ERget(F_FG0016_Insert), secname, FALSE);
}

/*
**	Produce an UPDATE 
** History:
**	3-dec-92 (blaise)
**		If we're using optimistic locking, add any optimistic locking
**		columns to the list passed to IIFGwc_whereClause
*/
static VOID
fgqupdate(frame_type, nmbr_words, p_wordarray)
OOID	frame_type;	/* Frame type */
i4	nmbr_words;	/* Number of words in command */
char	*p_wordarray[]; /* Command line */
{
	MFTAB 	*table;
	bool 	repeat = FALSE;
	bool 	terminate = TRUE;
	bool	intblfld;
	i4	section;
	i4 	keycols[DB_GW2_MAX_COLS];
	i4	numkcols;


        /* Check number of arguments */
	if (nmbr_words < 1 )
		IIFGqerror(E_FG0013_Query_InsfArgs, FALSE, 0);
	if (nmbr_words > 3 )
		IIFGqerror(E_FG0014_Query_TooManyArgs, FALSE, 0);

        /* Check that the section is valid and contains a primary table */
	section = check_section(p_wordarray[0]);
	if ( (section != SEC_MASTER) && (section != SEC_DETAIL) )
		IIFGqerror(E_FG0020_Query_BadSect, FALSE, 1, p_wordarray[0]);
	if ( (table = IIFGpt_pritbl(G_metaframe, section)) == NULL)
		IIFGqerror(E_FG0021_Query_NoPriTbl, TRUE, 1, p_wordarray[0]);

        /* Check validity of frame type */
	if (frame_type != OC_UPDFRAME)
		IIFGqerror(E_FG0018_Query_FrameType, TRUE, 2, _update, 
			   (PTR)&frame_type);

	/* Check for a repeat query & whether to terminate query with ';' */
	if (nmbr_words > 1)
		check_options(p_wordarray+1, nmbr_words-1, &repeat, &terminate); 

        /* Check that the primary table contains a unique key */
	if ( (numkcols = IIFGkc_keycols(table, keycols)) == 0)
		IIFGqerror(E_FG0019_Query_NoKey, TRUE, 1, p_wordarray[0]);

	/*
	** If we're using optimistic locking, then any optimistic locking
	** columns need to go into the where clause too
	*/
	if (G_metaframe->mode & MF_OPTIM)
		numkcols += IIFGolc_optLockCols(table, keycols, numkcols);

        /* See if we're in a tablefield */
	intblfld = table->tabsect > TAB_MASTER
		|| (G_metaframe->mode & MFMODE) == MF_MASTAB;

        /*
        ** Good enough.  Output the "UPDATE" line and the set and where clauses.
        */
	SIfprintf(outfp, ERx("%s%sUPDATE %s\n"), 
		  		  fgpad, (repeat ? _repeated : _null),
				  table->name); 
	IIFGols_outLinkStr(IIFGsc_setClause(table, intblfld), outfp, fgpad);
	IIFGols_outLinkStr(
		IIFGwc_whereClause(table, numkcols, keycols, intblfld,terminate),
	   	outfp, fgpad);
}

/*
**	Produce a SELECT 
*/
static VOID
fgqselect(frame_type, nmbr_words, p_wordarray)
OOID	frame_type;	/* Frame type */
i4	nmbr_words;	/* Number of words in command */
char	*p_wordarray[]; /* Command line */
{
	bool 	repeat = FALSE;
	bool 	terminate = TRUE;
	bool	qualified;
	i4	section;
	char	*form;


        /* Check number of arguments, and check for a repeat query */
	if (nmbr_words < 1 )
		IIFGqerror(E_FG0013_Query_InsfArgs, FALSE, 0);
	else if (nmbr_words > 3 )
		IIFGqerror(E_FG0014_Query_TooManyArgs, FALSE, 0);
	else if (nmbr_words > 1)
	        /* Statement has optional arguments. Check them. */
		check_options(p_wordarray+1, nmbr_words-1, &repeat, &terminate); 
	else
		;	/* no optional args -- fall thru */

	/*
	**	Check frame type.  Update and Browse frames
	**	use run-time qualification on the select.
	*/
	switch (frame_type)
	{
	    case OC_UPDFRAME:
	    case OC_BRWFRAME:
		qualified = TRUE;
		break;

	    default:
		IIFGqerror(E_FG0018_Query_FrameType, TRUE, 2, _select, 
			   (PTR)&frame_type);
	}

	/* Check section type for validity */
	section = check_section(p_wordarray[0]);
	if (section == SEC_BAD)
	{
		IIFGqerror(E_FG0020_Query_BadSect, FALSE, 1, p_wordarray[0]);
	}

	/* Get form name */
	form = ((USER_FRAME *)(G_metaframe->apobj))->form->name;

	/* Produce master select, detail select, or both */
	if ((section == SEC_MASTER) || (section == SEC_MD))
		fgqmselect(form, section,
			   repeat && (section == SEC_MASTER) && !qualified,
			   qualified, terminate );
	if ((section == SEC_DETAIL) || (section == SEC_MD))
		fgqdselect(form, repeat, terminate);
}

/*
**	Produce a master SELECT
*/
static VOID 
fgqmselect( form, section, repeat, qualified, terminate )
char	*form;		/* Form name */
i4	section;	/* section. don't terminate master portion of MD */
bool 	repeat;		/* TRUE for a repeat query */
bool 	qualified;	/* TRUE if qualifications are allowed */
bool	terminate;	/* TRUE if should terminate statement with ';' */
{
	bool 		intblfld;
	MFTAB 		*table;
	bool		select = FALSE;
	LSTR		*clause;
	bool		where_yet = FALSE;

	/* Get pointer to primary table for master section */
	IIFGti_tblindx(G_metaframe, TAB_MASTER, &sectdx[TAB_MASTER][0],
		       &sectdx[TAB_MASTER][1]);
	if (sectdx[TAB_MASTER][0] < 0)
		IIFGqerror(E_FG0021_Query_NoPriTbl, TRUE, 1, _master);

	/* Output "form[.tablefield] = SELECT" */
	IIFGbsBeginSection(outfp, ERget(F_FG0017_Select), 
		           ERget(F_FG0018_Master), FALSE);
	SIfprintf(outfp, ERx("%s%s"), fgpad, form);
	intblfld = ((G_metaframe->mode & MFMODE) == MF_MASTAB);
	if (intblfld)
		SIfprintf(outfp, ERx(".%s"), TFNAME);
	SIfprintf(outfp, ERx(" := %sSELECT\n"), 
		  repeat ? _repeated : _null);

	/* Output assignemnts and from clause */
	IIFGols_outLinkStr(
		IIFGac_assignClause(
		    sectdx[TAB_MASTER][0], sectdx[TAB_MASTER][1]),
		outfp, fgpad);
	IIFGols_outLinkStr(
		IIFGfc_fromClause(sectdx[TAB_MASTER][0], sectdx[TAB_MASTER][1]),
		outfp, fgpad);


	/* Output developer's restrictions, if any */
	table = G_metaframe->tabs[sectdx[TAB_MASTER][0]]; 
	clause = IIFGrc_restrictClause(table);
	if (clause != NULL)
	{
		if (where_yet == FALSE)
		{
			SIfprintf(outfp,ERx("%sWHERE\n"), fgpad);
			where_yet = TRUE;
		}

		IIFGols_outLinkStr(clause, outfp, fgpad);
		select = TRUE;
	}

	/* Output lookup joins, if any */
	clause = IIFGjc_joinClause(sectdx, TAB_MASTER, TAB_MASTER);
	if (clause != NULL)
	{
		if (where_yet == FALSE)
		{
			SIfprintf(outfp,ERx("%sWHERE\n"), fgpad);
			where_yet = TRUE;
		}

		if (select) SIfprintf(outfp, "%s%s\n", fgpad, _and);
	 	IIFGols_outLinkStr(clause, outfp, fgpad);
		select = TRUE;
	}

	/* Output run-time qualifications, if frame type uses them */
	if (qualified)
	{
		clause = IIFGqc_qualClause(sectdx[TAB_MASTER][0], 
					   sectdx[TAB_MASTER][1]);
		if (where_yet == FALSE)
		{
			SIfprintf(outfp,ERx("%sWHERE\n"), fgpad);
			where_yet = TRUE;
		}

		if (clause != NULL)
		{
			if (select) SIfprintf(outfp, "%s%s\n", fgpad, _and);
		 	IIFGols_outLinkStr(clause, outfp, fgpad);
			select = TRUE;
		}
	}

	/* Output order-by clause */
	IIFGols_outLinkStr(IIFGoc_orderClause(table), outfp, fgpad);

	/* don't terminate master section of a Master-Detail query */
	if ( (terminate) && (section != SEC_MD) )
		SIfprintf(outfp, "%s;", fgpad);
	SIputc('\n', outfp);
	IIFGesEndSection(outfp, ERget(F_FG0017_Select), 
			 ERget(F_FG0018_Master), FALSE);
}

/*
**	Produce a detail select/
*/
static VOID 
fgqdselect(form, repeat, terminate)
char	*form;		/* form name */	
bool	repeat;		/* TRUE for a repeat query */
bool	terminate;	/* tells if should terminate statement with ';' */
{
	MFTAB 	*table;
	bool	select = FALSE;
	LSTR	*clause;


        /* Get pointer to primary table for detail section */
	IIFGti_tblindx(G_metaframe, TAB_DETAIL, &sectdx[TAB_DETAIL][0],
		       &sectdx[TAB_DETAIL][1]);

        /* Output "form.tablefield = SELECT" */
	IIFGbsBeginSection(outfp, ERget(F_FG0017_Select), 
			   ERget(F_FG0019_Detail), FALSE);
	if (sectdx[TAB_DETAIL][0] < 0)
		IIFGqerror(E_FG0021_Query_NoPriTbl, TRUE, 1, _detail);
	SIfprintf(outfp, ERx("%s%s.%s := %sSELECT\n"), fgpad,
		  form, TFNAME, (repeat ? _repeated : _null));

        /* Output assignemnts and from clause */
	IIFGols_outLinkStr(
		IIFGac_assignClause(sectdx[TAB_DETAIL][0], 
				    sectdx[TAB_DETAIL][1]), 
		outfp, fgpad);
	IIFGols_outLinkStr(
		IIFGfc_fromClause(sectdx[TAB_DETAIL][0], 
			     	  sectdx[TAB_DETAIL][1]), 
				  outfp, fgpad);

        /* Output "WHERE" string */
	SIfprintf(outfp,ERx("%sWHERE\n"), fgpad);

        /* Output developer's restrictions, if any */
	table = G_metaframe->tabs[sectdx[TAB_DETAIL][0]]; 
	clause = IIFGrc_restrictClause(table);
	if (clause != NULL)
	{
		IIFGols_outLinkStr(clause, outfp, fgpad);
		select = TRUE;
	}

        /* Output lookup joins, if any */ 
	clause = IIFGjc_joinClause(sectdx, TAB_DETAIL, TAB_DETAIL);
	if (clause != NULL)
	{
		if (select) SIfprintf(outfp, "%s%s\n", fgpad, _and);
	 	IIFGols_outLinkStr(clause, outfp, fgpad);
		select = TRUE;
	}

        /* Output master-detail joins, if any */ 
	clause = IIFGjc_joinClause(sectdx, TAB_MASTER, TAB_DETAIL);
	if (clause != NULL)
	{
		if (select) SIfprintf(outfp, "%s%s\n", fgpad, _and);
	 	IIFGols_outLinkStr(clause, outfp, fgpad);
		select = TRUE;
	}

        /* Output order-by clause */
	IIFGols_outLinkStr(IIFGoc_orderClause(table), outfp, fgpad);

	if (terminate)
		SIfprintf(outfp, "%s;", fgpad);
	SIputc('\n', outfp);
	IIFGesEndSection(outfp, ERget(F_FG0017_Select), 
			 ERget(F_FG0019_Detail), FALSE);
}

/*
**      Check query options.
**      The only option currently defined is "_repeat" for a repeated query.
**	7/11/89 (pete)	Added support for an additional optional
**			parameter:  "NOTERM".
*/
static VOID
check_options( words, numwords, repeat, terminate )
char	*words[];
i4	numwords;
bool	*repeat;
bool	*terminate;
{
	register i4  i;

	/* Check optional arguments for words "repeat" or "noterm" */
	for (i=0; i<numwords; i++)
	{
	    CVlower(words[i]);
	    if (STequal(_repeat, words[i]))
		*repeat = TRUE;
	    else if (STequal(_noterm, words[i]))
		*terminate = FALSE;
	    else
		IIFGqerror(E_FG0016_Query_InvOption, FALSE, 1, words[0]);
	}
}

/*
**	Check section names.
**	Valid ones are:
**		master
**		detail
**		master_detail
*/
static i4  
check_section( section )
char 	*section;
{
	CVlower(section);
	if (!STcompare(_master, section))
		return (SEC_MASTER);
	else if (!STcompare(_detail, section))
		return (SEC_DETAIL);
	else if (!STcompare(_master_detail, section))
		return (SEC_MD);
	else
		return (SEC_BAD);
}
