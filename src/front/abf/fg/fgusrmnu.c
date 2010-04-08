/*
**  Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>
# include	<lo.h>
# include	<st.h>
# include	<si.h>
# include       <oocat.h>
# include	<ooclass.h>
# include	<abclass.h>

# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"
# include	<fglstr.h>


/**
** Name:	fgusrmnu.c -	Process a "generate user_menuitems" statement.
**
**	This file defines:
**	IIFGumUserMenu
**	fnd_menuescs	Find an escape in MFESC array.
**	gen_callfr	Generate code for CALLFRAME or CALLPROC statement.
**	frm_orProc	Return string "callframe" or "callproc".
**
** History:
**	6/5/89 (pete)	Written.
**	11/15/89 (pete)	Add "EXPLANATION =" to menuitem.
**	1/3/90 (pete)	Add check of IIretval after CALLFRAME.
**	19-aug-91 (blaise)
**	    Added missing #include <cv.h>.
**      07/14/99 (hweho01)
**          Added IIFGgm_getmem() function prototype. Without the 
**          explicit declaration, the default int return value for  
**          a function will truncate a 64-bit address on ris_u64. 
**          platform. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for fnd_escape() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */

GLOBALREF FILE *outfp;
GLOBALREF METAFRAME *G_metaframe;
FUNC_EXTERN char *IIFGgdrGetDefRetVal();
FUNC_EXTERN LSTR *IIFGals_allocLinkStr();
FUNC_EXTERN VOID IIFGols_outLinkStr();
FUNC_EXTERN VOID IIFGdq_DoubleUpQuotes();
FUNC_EXTERN PTR          IIFGgm_getmem();

/* static's */
static i4	fnd_menuescs();
static STATUS	gen_callfr();
static STATUS	gen_callchild();
static char	*frm_orProc();
static VOID	check_retval();
static i4	fnd_escape (
			i4  	numescs,
			MFESC	**p_escs,
			char	*text,
			i4	type);

static const char Frm[] = ERx("CALLFRAME");
static const char Proc[] = ERx("CALLPROC");

/*{
** Name:	IIFGumUserMenu - Process a "generate user_menuitems" statement.
**
** Description:
**	Process a "##generate user_menuitems" statement. Generate code like:
**
**		'user-specified menuitem' =
**		BEGIN
**		    user code for menu-start (if any)
**		    generated callframe(optional arguments) statement
**		    user code for menu-end   (if any)
**		    <test/set IIretval>
**		END
**
**	    where "<test/set IIretval>" looks like:
**		<when the called frame/proc's type != QBF, Report, Graph,
**		  DB Proc, or 3GL Proc>:
**
**		IF ((IIretval = 2) AND
**		    (ii_frame_name('current') <> ii_frame_name('entry'))) THEN
**		    ** Return to top (this is not the start frame)
**		    RETURN <default_return_value>;
**		ELSE
**		    IIretval = 1;	** restore default value
**		ENDIF;
**
**	Method: for every user defined menuitem (MFMENU entry),
**	search the user escapes in the MFESC array for a Start and
**	End escape for that menuitem.
**
** Inputs:
**	i4	nmbr_words;
**	char	*p_wordarray[];
**	char	*p_wbuf;
**	char	*infn;		** name of input file currently processing 
**	i4	line_cnt;	** current line number in input file
**
** Outputs:
**	Writes generated code to FILE *outfp
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
**	6/5/89 (pete)	Written.
**	8/6/90 (pete)	Changed generated code to use new 'ii_frame_name'
**			built-in 4GL procedure and to generate same
**			code regardless of whether current_frame == StartFrame.
**	2/91 (Mike S)	Add Listchoices-style menu code generation
*/
STATUS
IIFGumUserMenu(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	MFMENU **p_menus = G_metaframe->menus;
	STATUS stat = OK;
	i4 i;		/* counter */
	bool lcs;	/* ListChoices style ? */
	char double_text[FE_MAXNAME * 2 + 1]; /* buffer with room to double
					      ** single quotes in menu text */

	lcs = FALSE;	/* Assume classic menu */
	if (nmbr_words > 0)
	{
		CVlower(p_wordarray[0]);
		if (nmbr_words == 1 && 
		    STequal(p_wordarray[0], ERx("listchoices"))
		   )
		    lcs = TRUE;
		else
		   IIFGerr (E_FG002B_WrongNmbrArgs, FGCONTINUE, 2, 
			    (PTR) &line_cnt, (PTR) infn);
	   /* A warning; do not return FAIL -- processing can continue. */
	}

	/* Walk thru user-defined menuitems in MFMENU array. */
	if (!lcs)
	{
	    char padplus4[80];

	    STcopy(p_wbuf, padplus4);
	    STcat(padplus4, PAD4);

	    for ( i=0; i < G_metaframe->nummenus; i++, p_menus++ )
	    {
		char double_remark[(OOSHORTREMSIZE*2) +1]; 
					    /* Buffer with enough
					    ** room to double any single
					    ** quotes in remark text.
					    */
		IIFGdq_DoubleUpQuotes( 
		      ((USER_FRAME *)(*p_menus)->apobj)->short_remark,
		      double_remark);
		IIFGdq_DoubleUpQuotes( (*p_menus)->text, double_text);

		SIfprintf (outfp, ERx("\n%s'%s' (EXPLANATION = '%s') =\n"),
		    	   p_wbuf, double_text, double_remark);

		SIfprintf (outfp, ERx("%sBEGIN\n"), p_wbuf);

		/* Generate callframe/callproc, with any escape code */
		stat = gen_callchild(*p_menus, p_wbuf, FALSE);
		if (stat != OK)
			return stat;
		/* Generate code to check the IIretval global variable */
		check_retval((USER_FRAME *)G_metaframe->apobj,
			     (USER_FRAME *)((*p_menus)->apobj),
			     padplus4);

		SIfprintf (outfp, ERx("%sEND\n"), p_wbuf);	
							/* close of menuitem */
	    }
	}
	else
	{
	    /* Generate IF THEN ELSE ENDIF with all the child frames */
	    bool first = TRUE;

	    for (i = 0; i < G_metaframe->nummenus; i++, p_menus++ )
	    {
		IIFGdq_DoubleUpQuotes( (*p_menus)->text, double_text);
		SIfprintf(outfp, ERx("%s%s IIchoice = '%s' THEN\n"), 
			p_wbuf, first ? ERx("IF") : ERx("ELSEIF"), 
			double_text);
		first = FALSE;
		
		/* Generate callframe/callproc, with any escape code */
		stat = gen_callchild(*p_menus, p_wbuf, TRUE);
		if (stat != OK)
			return stat;
	    }
	    if (i > 0)
		SIfprintf(outfp, ERx("%sENDIF;\n"), p_wbuf);
	}
	return stat;
}

	
/*{
** Name:	gen_callchild - Generate the call to a child frame or proc
**
** Description:
**	Genrate the escape code and callframe for a call to a child frame or
**	procedure.  Optionally, generate 
**
**		IIfound = 1
**
**	so that a ListChoices-style menu frame knows it found the command.
**
** Inputs:
**	p_menu		MFMENU *	pointer to call descriptor
**	p_wbuf		char *		base tabbing offset
**	found		bool		whether IIfound it
**
** Outputs:
**
**	Returns:
**		STATUS
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	3/91 (Mike S) Initial Version
*/
static STATUS
gen_callchild(p_menu, p_wbuf, found)
MFMENU 	*p_menu;
char	*p_wbuf;
{
	i4 esc_index;

	/* search for optional MENU_START escape */
	if ((esc_index = fnd_escape(G_metaframe->numescs, 
				    G_metaframe->escs,
				    p_menu->text, ESC_MENU_START)) != -1)
	{
	    /* Menuitem has a MENU_START escape at offset 'esc_index'.
	    ** Write out the escape code.
	    */
	    IIFGbsBeginSection (outfp, ERget(F_FG0025_Menu_Start),
				p_menu->text, TRUE);

	    if (IIFGwe_writeEscCode (G_metaframe->escs[esc_index]->text,
			    p_wbuf, PAD4, outfp) != OK)
	    {
		return FAIL;
	    }

	    IIFGesEndSection (outfp, ERget(F_FG0025_Menu_Start),
				      p_menu->text, TRUE);
	}

	/* Generate code for CALLFRAME stmt (every menuitem must have
	** a callframe).
	*/
	if (gen_callfr ( p_menu, p_wbuf, PAD4 ) != OK)
		    IIFGerr (E_FG0030_OutOfMemory, FGFAIL, 0);/*returns FAIL*/

	if (found)
		SIfprintf(outfp, "%s%sIIfound = 1;\n", p_wbuf, PAD4);

	/* search for optional MENU_END escape */
	if ((esc_index = fnd_escape(G_metaframe->numescs, 
			    G_metaframe->escs,
			    p_menu->text, ESC_MENU_END)) != -1)
	{
	    /* Menuitem has a MENU_END escape at offset 'esc_index'.
	    ** Write out the escape code.
	    */
	    IIFGbsBeginSection (outfp, ERget(F_FG0026_Menu_End),
				p_menu->text, TRUE);

	    if (IIFGwe_writeEscCode (G_metaframe->escs[esc_index]->text,
			    p_wbuf, PAD4, outfp) != OK)
	    {
		return FAIL;
	    }

	    IIFGesEndSection (outfp, ERget(F_FG0026_Menu_End),
			      p_menu->text, TRUE);
	}
	return OK;
}

/*{
** Name:  check_retval - Generate code to check IIretval global after callframe.
**
** Description: Generate code to check the IIretval global variable after a
**	CALLFRAME or CALLPROC.
**
** Inputs:
**	USER_FRAME *p_thisFrame;	info about current frame.
**	USER_FRAME *p_calledFrame;	info about called frame or proc.
**					Ignored if NULL
**	char	*p_wbuf;		whitespace to prepend to generated code.
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
**	Writes to output file.
**
** History:
**	3-jan-1990	(pete)	Initial Version.
*/
static VOID
check_retval(p_thisFrame, p_calledFrame, p_wbuf)
USER_FRAME *p_thisFrame;
USER_FRAME *p_calledFrame;
char	*p_wbuf;
{
	if ( (p_calledFrame != NULL) &&
	    ( (p_calledFrame->class == OC_RWFRAME)
	   || (p_calledFrame->class == OC_QBFFRAME)
	   || (p_calledFrame->class == OC_GRFRAME)
	   || (p_calledFrame->class == OC_DBPROC)
	   || (p_calledFrame->class == OC_HLPROC)
	    )
	   )
	{
	    /* no need to generate code to check IIretval after calls to
	    ** these types, cause none of them can set it.
	    */
	    return;
	}
	else
	{
	    SIfprintf (outfp,
		ERx("%sIF ((IIretval = 2) AND\n"), p_wbuf);

	    SIfprintf (outfp,
        ERx("%s%s(ii_frame_name('current') <> ii_frame_name('entry'))) THEN\n"),
			p_wbuf, PAD4);

	    SIfprintf (outfp, ERx("%s%s/* %s */\n"), p_wbuf, PAD4,
			ERget(F_FG0039_ReturnToTop));
    
	    SIfprintf (outfp, ERx("%s%sRETURN %s;\n%sELSE\n"),
			p_wbuf, PAD4, IIFGgdrGetDefRetVal(G_metaframe), p_wbuf);

	    SIfprintf (outfp,
			ERx("%s%sIIretval = 1;\t/* %s */\n"),
			p_wbuf, PAD4, ERget(F_FG003A_RestoreDefVal));

	    SIfprintf (outfp, ERx("%sENDIF;\n"), p_wbuf);
	}
}

/*{
** Name:	gen_callfr - Generate code for a CALLFRAME statement.
**
** Description: Generate code for CALLFRAME stmt.
**
** Inputs:
**	MFMENU	*p_menu;
**	char	*p_wbuf1;	left margin
**	char	*p_wbuf2;	extra left margin
**
** Outputs:
**	writes code to output file.
**
**	Returns:
**		OK or FAIL
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	lstr routines allocate memory.
**
** History:
**	6/89	(Pete)	Initial Version.
*/
static STATUS
gen_callfr(p_menu, p_wbuf1, p_wbuf2)
MFMENU	*p_menu;
char	*p_wbuf1;	/* left margin */
char	*p_wbuf2;	/* extra left margin */
{
	MFCFARG **p_args = p_menu->args;
	LSTR *p_lstr_head;
	LSTR *p_lstr;
	OOID class = p_menu->apobj->class;
	i4 sep;	/* indicates what to separate arguments with */
	char *p_wbuf;
	i4 i;

	/*
	**   for Frames, generate:
	**	CALLFRAME framename (arg1=expr; arg2=expr[;...]);\n
	**   for 4gl procedures & Dbase procedures:
	**	CALLPROC procname (arg1=expr, arg2=expr[,...]);\n
	**   for 3gl procedures:
	**	CALLPROC procname (expr, expr[,...]);\n
	*/
	if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)NULL, LSTR_NONE)) == NULL)
	    return FAIL;

	p_lstr_head = p_lstr;	/* save head of list */

	STprintf (p_lstr->string, ERx("%s %s ("), 
		frm_orProc(class),
		((USER_FRAME *)(p_menu->apobj))->name);

	for (i=0; i< p_menu->numargs	; i++, p_args++)
	{
	    /* Note: IIFGals_alocLinkStr allocates 256 byte buffer that
	    ** I'm not using here.
	    */

	    /* For frames, 4gl procs & Dbase procs, syntax is:
	    **		(arg=expr; arg=expr; ..)
	    ** For 3gl procs, syntax is just (expr; expr; ..)
	    */
	    if (class != OC_HLPROC)	/* != 3gl proc */
	    {
	        if ((p_lstr=IIFGals_allocLinkStr (p_lstr, LSTR_EQUALS)) == NULL)
		    return FAIL;
	    	p_lstr->string = (*p_args)->var;
	    }

	    /* callframe args separated by ';', callproc args separated by ','*/
	    sep = ( (class == OC_HLPROC) || (class == OC_OSLPROC) ||
		(class == OC_DBPROC) ) ? LSTR_COMMA : LSTR_SEMICOLON ;

	    if ((p_lstr = IIFGals_allocLinkStr (p_lstr, sep))== NULL)
		return FAIL;
	    p_lstr->string = (*p_args)->expr;
	}

	/* 4gl won't allow a semicolon or comma following the last argument.
	** e.g. "callframe fr1 (arg1=expr;arg2=expr;);"  is bad syntax.
	** so Zap it!				   ^
	*/
	if (i>0)
	    p_lstr->follow = LSTR_NONE ;

	if ((p_lstr = IIFGals_allocLinkStr (p_lstr, LSTR_SEMICOLON)) == NULL)
	    return FAIL;
	STcopy (ERx(")"), p_lstr->string);
	p_lstr->nl = TRUE;

	/* Fix up indented white space with extra padding. */
	p_wbuf = (char *)IIFGgm_getmem
			( (STlength(p_wbuf1) + STlength(p_wbuf2) + 1), FALSE);

	STpolycat (2, p_wbuf1, p_wbuf2, p_wbuf);

#ifdef DEBUG
	/* dump out the LSTR linked list to a file */
	IIFGdls_dumpLinkStr(ERx("lstr.dmp"), p_lstr_head);
#endif

	/* write out the LSTR structure to the generated 4gl file. */

        IIFGbsBeginSection (outfp, ERget(F_FG0015_Call), 
			    p_menu->text, FALSE);

	IIFGols_outLinkStr (p_lstr_head, outfp, p_wbuf);

        IIFGesEndSection (outfp, ERget(F_FG0015_Call), 
			  p_menu->text, FALSE);

	return OK;
}

/*{
** Name:  fnd_escape -Find an escape of a particular type for an item in MFESC.
**
** Description:
**	Search the MFESC array for an escape of a particular type and item name.
**
** Inputs:
**	i4 	numescs		number of entries in MFESC array.
**	MFESC	**p_escs	array of MFESCs.
**	char	*text		menuitem name to search for.
**	i4	type		escape type to search for.
**
** Outputs:
**
**	Returns:
**		the index number in the MFESCs where a match occurred,
**		or -1 if no match was found.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/15/89	(pete)	Written.
*/
static i4
fnd_escape (numescs, p_escs, text, type)
i4  	numescs;
MFESC	**p_escs;
char	*text;
i4	type;
{
	i4 i;

	/*
	** The MFESC array is sorted by type, so stop searching when
	** the array types are larger than the one we are searching for.
	*/
	for (i=0; (i < numescs) && (type >= (*p_escs)->type); i++, p_escs++)
	{
	    if  ((type == (*p_escs)->type) && STequal( text, (*p_escs)->item))
	    {
		return i;
	    }
	}
	return -1;
}

static char
*frm_orProc(class)
OOID class;
{
	switch (class)
	{
	    case OC_HLPROC:
	    case OC_OSLPROC:
	    case OC_DBPROC:
	    case OC_UNPROC:
		return Proc;	/* "callproc" */
	    default:
		return Frm;	/* "callframe" */
	}
}
