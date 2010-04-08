/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<ex.h>
# include	<cv.h>
# include	<cm.h>
# include	<lo.h>
# include	<me.h>
# include	<si.h>
# include	<pc.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       <ugexpr.h>
# include	<ooclass.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"

/**
** Name:	framegen.c - generate 4gl from a template file.
**
** Description:
**	Process a Template file + Visual Query Meta-structure into a
**	4gl file that will have no syntactic or semantic errors!
**
**	This file defines:
**
** IIFGstart()	- startup procedure for generating 4gl code.
** IIFGscan()	- scan and process a template file. called recursively.
** fgsetloc()	- translate environment variable & set LOCATION struct.
** opentf()	- open a template file.
** tlstmt()	- process a parsed template language (##) statement.
** generate()	- call appropriate routines to generate 4gl in output file.
** savewhite()  - save leading white chars following ##.
** gettok()	- get next token from buffer; used for non-## lines.
** get_tltok()	- get next token; used for ##statements.
** rename_outfile()  rename output file from temp to correct name.
** IIFGbseBoolStmtEval	- evaluate a boolean statement (IF/WHILE)
** IIFGsfwSkipFirstWord	- skip over first word in statement.
** ParseAndSubTLStmt() - parse ##statement & substitute for $vars.
** save_stmt()	- save information about a ##statement.
**
** History:
**	4/1/89 (pete)	Initial Version.
**	11/15/89 (pete) Print error message in Handler if get Segmentation err.
**      08-mar-90 (sylviap)
**              Added a parameter to PCcmdline call to pass an CL_ERR_DESC.
**	22-mar-90 (pete)Set up LOCATION structure for files/language just
**			like Ingres Help.
**	25-apr-1990 (pete) Added support for environment variables ($$vars). 
**			along with ##ifdef
**	2/91 (Mike S) Add load_menuitems support
**	8-march-91 (blaise)
**	    Integrated PC porting changes.
**	16-apr-91 (pete)
**	    Fix bug 37025 (close open files following code gen error).
**	24-feb-92 (pete)
**	    Evaluate extensions to template language for Amethyst & 6.5.
**	    Changed return type of IIFGftFindToken(); also moved it
**	    to (new) file: fgvars.c.
**	16-jun-1992 (pete)
**	    Changed way statement parsing is done. Remove support for SET
**	    statement, added in 2/92.
**	13-aug-92 (blaise)
**	    Added "local_procedures" entry to cgenarray.
**	11/20/92 (dkh) - Changed opentf() to copy the input name to a
**			 buffer before using it in a call to LOfroms().
**			 This is needed since LOfroms() will want to
**			 update the buffer but the passed in name to
**			 opentf() may be pointing to a string literal
**			 that is not writeable and potentially not large
**			 enough.
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics in IIFGstart and opentf.
**	12-Feb-93 (fredb)
**	   Porting change for hp9_mpe in build_outfn.
**	22-Feb-93 (fredb)
**	   Porting change for hp9_mpe in fgfilesloc.
**	03-Mar-93 (fredb)
**	   Porting change for hp9_mpe in rename_outfile.
**	15-sep-93 (connie) Bug #50493
**	   Fix the problem of defining >=20 substitution variables in a
**	   template file
**	18-feb-94 (connie) Bug #54857
**	   Fix the problem of having syntax error when a DEFINE statement
**	   with no defined value is used in a template file which is
**	   included more than once. Also, the filename in the error
**	   message is corrected.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-oct-2001 (abbjo03)
**	    In this_tlstmt_continued(), change argument to STrchr to literal
**	    character instead of char array.
**    25-Oct-2005 (hanje04)
**        Add prototype for get_tltok() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

GLOBALREF METAFRAME *G_metaframe;

GLOBALREF IFINFO Ii_ifStack[MAXIF_NEST];
GLOBALREF i4	Ii_tos;	/* index for top of "Ii_ifStack" */

GLOBALREF LOCATION IIFGcl_configloc;	/* LOCATION for II_CONFIG (FILES) */
GLOBALREF char	IIFGcd_configdir[MAX_LOC+1];
GLOBALREF FILE	*outfp;		/* output FILE to put generated 4gl into */

/* Code generation functions */
FUNC_EXTERN STATUS	IIFGquery();	/* generate query */
FUNC_EXTERN STATUS	IIFGhidflds();	/* generate hidden_fields */
FUNC_EXTERN STATUS	IIFGusresc();	/* generate user_escape */
FUNC_EXTERN STATUS	IIFGglmGenLoadMenu();	/* generate load_menuitems */
FUNC_EXTERN STATUS	IIFGumUserMenu();  /* generate user_menuitems */
FUNC_EXTERN STATUS	IIFGlookup();	/* generate lookup */
FUNC_EXTERN STATUS	IIFGhelp();	/* generate help */
FUNC_EXTERN STATUS	IIFGchkchg();	/* generate check_change */
FUNC_EXTERN STATUS	IIFGundchg();	/* generate undo_change */
FUNC_EXTERN STATUS	IIFGflddefs();	/* set field default values */
FUNC_EXTERN STATUS	IIFGsf_SetNullKeyFlags();  /* set iiNullKeyFlag fields*/
FUNC_EXTERN STATUS	IIFGlocalprocs(); /* generate local procedure code */
FUNC_EXTERN IITOKINFO	*IIFGftFindToken(); /* find $variable (token) */
FUNC_EXTERN VOID        IIUGrefRegisterExpFns();
FUNC_EXTERN bool	IIFGbseBoolStmtEval();
FUNC_EXTERN PTR		IIFGgm_getmem();
FUNC_EXTERN bool	IIFGdseDefineStmtExec();
FUNC_EXTERN bool	IIFGuvUndefVariable();

/*
** build array of function info structures to be used to call functions
** when code needs to be generated.
** To add a new entry here: just add it to this list & declare the
** function above. If list gets long, we'll want to change this from
** a sequential list. Put the most commonly used statements first.
*/
static CGEN_FNS cgenarray[] =
{
	{ ERx("query"), IIFGquery },		/*
					** Generate a query. first argument
					** tells what type of query to generate
					** NOTE: IIFGlupval() generates the
					** lookup validation SELECT. There is
					** no language syntax for generating a
					** lookup val. select. IIFGlupval() is
					** called in fgusresc.c.
					*/
	{ ERx("user_escape"),  IIFGusresc },  /* insert a user escape */
	{ ERx("load_menuitems"),  IIFGglmGenLoadMenu },/* load mnuitms into tf*/
	{ ERx("user_menuitems"), IIFGumUserMenu }, /*insert all user menuitems*/
	{ ERx("hidden_fields"), IIFGhidflds},  /*declare hidden flds & columns*/
	{ ERx("lookup"), IIFGlookup },	/* insert code into Lookup menuitem */
	{ ERx("help"), IIFGhelp }, 	/*
					** generate help_forms statement &
					** copy help template file.
					*/
	{ ERx("set_default_values"), IIFGflddefs }, 	/*
					** set field default values. Append
					** frames only; visible master-table
					** fields only.
					*/
	{ ERx("check_change"), IIFGchkchg }, /*
					** inquire_forms for changes to a
					** type of field (e.g. join fields)
					*/
	{ ERx("copy_hidden_to_visible"), IIFGundchg },  /*
					** set visible fields = values
					** in corresponding hidden fields.
					** (done for a class of related fields,
					**  s.a. join fields).
					*/
	{ ERx("set_null_key_flags"), IIFGsf_SetNullKeyFlags },  /*
					** set iiNullKeyFlag hidden fields,
					** based on if key column fields
					** contain NULL.
					*/
	{ ERx("local_procedures"), IIFGlocalprocs } /*
					** insert local procedures
					*/
};
#define NMBRCGENFNS (sizeof(cgenarray)/sizeof(CGEN_FNS))

static	FG_STATEMENT stmts[] = {
	{ERx("DEFINE"),	FG_STMT_DEFINE,	2,		3,	NULL},
	{ERx("IF"),	FG_STMT_IF,	FG_NOWORD,	2,	ERx("THEN")},
	{ERx("ELSE"),	FG_STMT_ELSE,	FG_NOWORD,	FG_NOWORD, NULL},
	{ERx("ENDIF"),	FG_STMT_ENDIF,	FG_NOWORD,	FG_NOWORD, NULL},
	{ERx("IFDEF"),	FG_STMT_IFDEF,	2,		FG_NOWORD, NULL},
	{ERx("IFNDEF"),	FG_STMT_IFNDEF,	2,		FG_NOWORD, NULL},
	{ERx("GENERATE"),FG_STMT_GENERATE,FG_NOWORD,	FG_NOWORD, NULL},
	{ERx("INCLUDE"),FG_STMT_INCLUDE,FG_NOWORD,	FG_NOWORD, NULL},
	{ERx("DOC"),	FG_STMT_DOC,	FG_NOWORD,	FG_NOWORD, NULL},
	{ERx("UNDEF"),	FG_STMT_UNDEF,	2,		FG_NOWORD, NULL}
};
#define NMBR_STMTS (sizeof(stmts)/sizeof(FG_STATEMENT))

static char     Plus[]   = ERx("+"),
                Minus[]  = ERx("-"),
                Times[]  = ERx("*"),
                Divide[] = ERx("/"),
                Equals[] = ERx("="),
                Lparen[] = ERx("("),
                Rparen[] = ERx(")"),
                Expon[]  = ERx("^"),
                Quote[]  = ERx("'"),
		DQuote[] = ERx("\""),
                Exclaim[] = ERx("!"),
                Lt[] = ERx("<"),
                Gt[] = ERx(">"),
		Underscore[] = ERx("_"),
		Dollar[] = ERx("$"),
		Newline[] = ERx("\n");

static char *Delims[] = {Quote, Plus, Minus, Times, Divide,
                         Equals, Lparen, Rparen, Expon,
                         Exclaim, Lt, Gt, Dollar, DQuote
                        };
#define NMBR_DELIMS     (sizeof(Delims)/sizeof(char *))

/* extern's */
GLOBALREF char	*Tfname;	/* Set in fginit.c to name of template file.
				** Points to a static MAX_LOC+1 char buffer,
				** which is also allocated in fginit.c
				*/
/* static's */
static i4	Pgm_state = PROCESSING;	/* tells if template file records
					** should be processed (PROCESSING) or
					** flushed (FLUSHING).
					*/
static i4	chkdir = FALSE;		/* tells if location structures have
					** been built for places where template
					** files can live: II_TFDIR and
					** II_CONFIG.
					*/
static char	tfdir[MAX_LOC+1];
static LOCATION tfloc;		/* LOCATION for II_TFDIR */

static char	locbuf[MAX_LOC+1];  /*buffer to use for outfile LOCATION*/
static LOCATION outfile;    /* output file to put generated 4gl into */
static char	nm_scratch[MAX_LOC+1]; /* scratch buff for NMgtAt translations*/

/* "Tmp.." variables are related to current ##statement */
static	char	Tmp[TOKSIZE+1];	    /* current token (may have leading blanks)*/
static	char	Tmp_buf[RECSIZE+1];	/* ##stmt after substitution */
static	i4	Tmp_tok_type;		/* token type for current token */
static	char	Tmp_saved_tok[TOKSIZE+1]; /* saved token from current stmt.
					  ** Empty string if none exists.
					  */
static	FG_STATEMENT *Tmp_stmt_type;	/* details about current statement */
static	i4	Tmp_scan_mode;
static	i4	Tmp_expr_loc;		/* Relative position in substituted
					** buffer where expression begins.
					*/
#define	FG_EXPR_LOC_NOTSET	-1
static	FILE	*Filestack[MAXINCLUDE_NEST+1];
static	i4	Fs_top;	/* top of fileStack */

/*	functions */
static  i4 handler();	/* EXception handler */
static  i4	gettok();
static  char	*savewhite();
static  VOID	generate();
static  VOID	opentf();
static  VOID	tlstmt();
static  STATUS	exec_env();
static	VOID	build_outfn();
static	STATUS	fgsetloc();
static	VOID	rename_outfile();
static	VOID	IIFGscan();
static  VOID	fgfilesloc();
static	VOID	pushfile();	/* push file onto fileStack */
static	FILE	*popfile();     /* pop file off fileStack */
static	VOID	closefiles();	/* close all files on stack */
static	STATUS	striplastword();
static	FG_STATEMENT *getstmt_type();
static	char	*ParseAndSubTLStmt();
static	VOID	save_stmt();
static	bool	isdelim();
static	bool	is_in();
static	char	*stripbuf();
static	char	*cat_strings();
static	bool	this_tlstmt_continued();
static  i4	 get_tltok(
			char	**p_buf);

FUNC_EXTERN VOID  IIFGinit();
FUNC_EXTERN VOID    IIFGpushif();
FUNC_EXTERN IFINFO  *IIFGpopif();
FUNC_EXTERN bool  IIFGifdef_eval();
FUNC_EXTERN i4	  IIFGelse_eval();
FUNC_EXTERN char *UGcat_now();	/* date in catalog, GMT format. not in ug.h */
FUNC_EXTERN VOID IIFGrm_relmem();
FUNC_EXTERN VOID IIFGerr();
FUNC_EXTERN char *IIUGeceEvalCharExpr();

/*{
** Name:	IIFGstart()	- startup procedure for generating code.
**
** Description:
**	Open & process a template file. Other facilities should call this.
**
** Inputs:
**	METAFRAME *mf	Pointer to meta frame structure to generate code for.
**
** Outputs:
**	File of generated 4gl code. Deletes file is any error occurs.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		EXdeclare(handler)
**
** Side Effects:
**
** History:
**	4/1/89 (pete)	written
**	4/16/91 (pete)
**		Close all open files following error. Create an array to
**		hold stack of open files. Add routines pushfile(),
**		popfile(), closefiles(). Bug 37025.
**	31-mar-1992 (pete)
**		Call routine IIUGrefRegisterExpFns() to register callback
**		function for ugexpr.c.
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics.
*/
STATUS
IIFGstart (mf)
METAFRAME *mf;
{
	EX_CONTEXT	ex;
	char		*p_nm;	/* tells if METAFRAME should be dumped */

	G_metaframe = mf;

	/* establish an exception handler */
	if (EXdeclare(handler, &ex) != OK)
	{
	    /* cleanup & return FAIL */
	    if (outfp != NULL)
	        SIclose(outfp);
	    LOdelete(&outfile);
	    closefiles();	/* close all open input files on stack */

	    EXdelete();
	    IIFGrm_relmem();	/* deallocate memory */
	    return FAIL;
	}

	if (!chkdir)
	{
	    /* set up LOCATION structures for template file locations */

	    _VOID_ fgsetloc(ERx("II_TFDIR"), tfdir, &tfloc); 
	    fgfilesloc (IIFGcd_configdir, &IIFGcl_configloc);	/* FILES/Lang */

	    if ((tfdir[0] == EOS) && (IIFGcd_configdir[0] == EOS))
	    {
		IIFGerr(E_FG0005_No_TFdir, FGFAIL, 0);
	    }

	    chkdir = TRUE;
	}

	/*
	** Assertion: at least one of II_TFDIR or II_CONFIG is set
	** to a valid directory.
	*/
	
	/* build name of (temporary) output file */

	build_outfn (mf, &outfile, locbuf);

#ifdef hp9_mpe
	if (SIfopen(&outfile, ERx("w"), SI_TXT, 252, &outfp) != OK)
#else
	if (SIopen(&outfile, ERx("w"), &outfp) != OK)
#endif
	{
		char *p_fn;
		LOtos (&outfile, &p_fn);
		IIFGerr(E_FG0006_Open_Error, FGFAIL, 1, (PTR) p_fn);
	}

	/*
	** Initialize array of IItoken translations via meta structure
	**  & set Utility PTRs to names of hidden fields & correlation vars.
	*/
	IIFGinit(mf);

	/* Register callback functions with utils!ug!ugexpr.c. The callbacks
	** are then made from routine IIUGeceEvalCharExpr(), which is called
	** below to evaluate expressions in ##IF & ##DEFINE statements..
	*/
	IIUGrefRegisterExpFns(IIFGgm_getmem);

	Ii_tos = 0;	/* set top of "if" stack */
	Fs_top  = 0;	/* set (just before) top of fileStack */
	Pgm_state = PROCESSING;
	/*
	** Process main template file. IIFGscan is called recursively if
	** ##include statements are encountered.
	*/

	/* for runtime DEBUG */
	NMgtAt(ERx("II_DMPMF"), &p_nm);
	if ( (p_nm != NULL) && (*p_nm != EOS))
	{
	    /* II_DMPMF was set; dump the METAFRAME structure to a file */
	    IIFGdmp_DumpMF (mf, ERx("metaframe.dmp"));
	}

	IIFGscan (Tfname, ERx(""), 0);

	if (outfp != NULL)
	{
		SIclose(outfp);

		/* code generated ok; now rename file to correct name */
		rename_outfile(mf, &outfile, locbuf);
		
		/* Purge source directory to 3 versions of generated file */
		LOpurge (&outfile, (i4) 3);

	        /*if II_POST_4GLGEN is set, then translate and execute it */
	        _VOID_ exec_env(ERx("II_POST_4GLGEN"), &outfile);
	}

	IIFGrm_relmem();	/* Deallocate memory allocated
				** during code generation.
				*/
	EXdelete();
	return OK;
}

/*{
** Name:   fgfilesloc - Set LOCATION struct & char array for FILES/Language dir.
**
** Description:
**
** Inputs:
**	char pathname[MAX_LOC+1]   Write full pathname of FILES/Language here.
**	LOCATION *loc	           Write LOCATION struct for FILES/Lang here.
**
** Outputs:
**	Assignments to "pathname" & "loc".
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
**	3/22/90	(pete)	Initial Version. Code is similar to that in 'ughlpnm.c'
**			in front.utils.ug.
**	4/3/90  (pete)  LOcopy the LOCATION & buffer built by NMloc (bug).
**	22-Feb-93 (fredb) hp9_mpe
**		MPE/iX doesn't have enough directory levels to support a
**		language subdirectory under FILES, so don't try to add it
**		onto the location.
*/
static VOID
fgfilesloc (pathname, loc)
char *pathname;
LOCATION *loc;
{
        STATUS          clerror = OK;
        char            langstr[ER_MAX_LANGSTR + 1];
	LOCATION	tmploc;
        i4		langcode;

#ifndef hp9_mpe
        /* get language name to add to FILES directory */
        clerror = ERlangcode(NULL, &langcode);
#endif

        if (clerror == OK)
        {
#ifdef hp9_mpe
            clerror = NMloc(FILES, PATH, NULL, &tmploc);
#else
	    /* Create FILES/language directory spec.
	    ** (if ERlangcode works, ERlangstr will too)
	    */
            _VOID_ ERlangstr(langcode, langstr);

            clerror = NMloc(FILES, PATH, langstr, &tmploc);
#endif
	    if (clerror == OK)
	    {
		LOcopy (&tmploc, pathname, loc);	/* make safe copy */
	    }
        }

	if (clerror != OK)
	    IIFGerr(E_FG005C_BadFilesDir, FGFAIL, 1, (PTR) &clerror);

	if ( LOexist(loc) != OK)
	{
	    IIFGerr(E_FG005D_NoFilesDir, FGFAIL, 1, (PTR) pathname);
	}
}

/*{
** Name:	rename_outfile - rename output file name from temp to correct.
**
** Description:
**	Change the name of the output file that code was just generated into
**	from the current name (generated by LOuniq) to the correct source
**	file name indicated in ABF's USER_FRAME structure for this frame.
**
** Inputs:
**	METAFRAME	*mf
**	LOCATION	*outfile
**	char		*locbuf		buffer associated with outfile.
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
**	6/27/89 (pete)	Initial version.
**	03-Mar-93 (fredb) hp9_mpe
**		Porting Change: Due to MPE's unusual file system and the way
**		LO had to be implemented to accommodate it, we need to create
**		the source file name differently.
*/
static VOID
rename_outfile (mf, outfile, locbuf)
METAFRAME	*mf;
LOCATION	*outfile;
char		*locbuf;
{
	USER_FRAME *p_uf;
	APPL	   *p_ap;

	LOCATION   srcdirloc;
	char srcdirbuf[MAX_LOC+1];

	LOCATION   srcfileloc;
	char srcfilebuf[MAX_LOC+1];


	p_uf = (USER_FRAME *) mf->apobj ;
	p_ap = p_uf->appl ;

	/* set up a LOCATION structure for where the real source file is
	** supposed to be.
	*/

#ifdef hp9_mpe
	/*
	** Take the source file name, which is now in the format "f.g",
	** and add another delimiter ("."), then the source file directory
	** (account), giving us a fully qualified filename, "f.g.a".
	**
	** Use LOfroms to get a LOCATION setup and we're done.
	*/

	STpolycat (3, p_uf->source, ERx("."), p_ap->source, srcdirbuf);
	LOfroms (PATH & FILENAME, srcdirbuf, &srcdirloc);

#else
	/* make copy of application's source directory name */
	STcopy (p_ap->source, srcdirbuf);
	LOfroms (PATH, srcdirbuf, &srcdirloc);

	/* make copy of source filename for this frame */
	STcopy (p_uf->source, srcfilebuf);
	LOfroms (FILENAME, srcfilebuf, &srcfileloc);

	/* add the FILENAME to srcdirloc built above */
	LOstfile (&srcfileloc, &srcdirloc);
#endif

	/* rename outfile (a temp name from LOuniq) to real source file name.*/
	if (LOrename (outfile, &srcdirloc) != OK)
	{
	    IIFGerr (E_FG0057_RenameError, FGFAIL, 0);
	}

	/* Copy the new, correct file & path name to 'outfile', since that
	** is what the outside world expects the generated file to be.
	*/
	LOcopy (&srcdirloc, locbuf, outfile);
}

/*{
** Name:	build_outfn - build full filespec for name of output file.
**
** Description:
**	Build output file name via ABF App source code directory + frame
**	source file name.
**
** Inputs:
**	METAFRAME	*mf
**	LOCATION	*outfile
**	char		*buf		MAX_LOC buffer to use for PATH&FILENAME
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
**	5/22/89 (pete)	Initial version.
**	8/24/90 (pete)	Change file suffix from "fg" to "tmp" to make it easier
**			to identify a temporary file. Many other front-ends
**			also use "tmp".
**	12-Feb-93 (fredb)
**		Porting change for hp9_mpe: The source location contains only
**		the MPE account name and LOuniq generates only the file name,
**		leaving us without a group name.  We will assign the group
**		name 'tmp' here as part of the PATH.  Now the final location
**		will contain a proper MPE fully qualified filename in the
**		format 'filename.group.account'.
*/
static VOID
build_outfn (mf, outfile, buf)
METAFRAME	*mf;
LOCATION	*outfile;
char		*buf;
{
	USER_FRAME *p_uf;
	APPL	   *p_ap;

	if (mf == NULL)
	{
	    IIFGerr (E_FG0026_Null_Metaframe, FGFAIL | FGSETBIT, 0);
	}
	else if (mf->apobj == NULL)
	{
	    IIFGerr (E_FG0027_Null_ABF_Ptr, FGFAIL | FGSETBIT, 0);
	}

	p_uf = (USER_FRAME *) mf->apobj ;
	p_ap = p_uf->appl ;

	/* Note: Do not put this temp file in II_TEMPORARY, cause must be
	** in same directory as application source code directory, in
	** order for LOrename to work when all done.
	*/

	/*  build LOCATION path from application source directory name */
#ifdef hp9_mpe
	STprintf(buf, ERx("%s.%s"), ERx("tmp"), p_ap->source);
#else
	STcopy (p_ap->source, buf);
#endif
	LOfroms (PATH, buf, outfile);	/* The buffer 'buf' becomes associated
					** with the LOCATION 'outfile'.
					*/
	/* add unique temp file name in outfile */
	LOuniq(ERx("fg"), ERx("tmp"), outfile);
}

/*
**	Execute the translation of an environment variable concatenated with
**	a filename.
*/
static STATUS
exec_env(nm, outfile)
char *nm;
LOCATION *outfile;
{
	char        *p_nm;	/* pointer for NMgtAt to use */
	CL_ERR_DESC err_code;

	NMgtAt(nm, &p_nm);
	if ( (p_nm == NULL) || (*p_nm == EOS))
	    ;	 	/* environment variable not set */	
	else
	{
		/* env_var translated ok */
		char nm_tran[MAX_LOC+1];
		char *p_outf;

		STcopy (p_nm, nm_tran);
		LOtos(outfile, &p_outf);

		STpolycat (3, nm_tran, ERx(" "), p_outf, nm_tran);

		if (PCcmdline ( (LOCATION *)0, nm_tran, PC_WAIT, NULL, 
			&err_code) != OK)
		{
		    /* warning message -- processing continues */
		    IIFGerr (E_FG0058_Bad_Env_Var, FGCONTINUE, 1, (PTR) nm);
		}
	}
	return OK;
}
/*{
** Name:	handler - local exception handler.
**
** Description:
**	This routine is called when an exception occurs (Note that all errors
**	that occur during code generation raise an exception; other things
**	can raise an exception too).
**	We don't want to allow, for example, a user interrupt to leave a
**	half-generated 4gl file laying around, because ABF would later find it
**	and try to compile it).
**
**	Code generation should either produce a complete file, or no file
**	at all.
**
** Inputs:
**	EX_ARGS *ex_args
**
** Outputs:
**	NONE
**
**	Returns:
**		EXDECLARE or EXCONTINUES.
**
**	Exceptions:
**		NONE.
**
** Side Effects:
**
** History:
**	5/16/89 (pete) written.
**      17-apr-1991     (pete)
**              Fix invalid exceptions from EXsignal. Was signalling
**              EXCONTINUE and EXDECLARE.
**      18-Feb-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
*/
static i4
handler(ex_args)
EX_ARGS *ex_args;
{
	if (ex_args->exarg_num == EXVALJMP)
	{
	    return (EXCONTINUES);
	}
	else if ((ex_args->exarg_num == EXSEGVIO)
	     || (ex_args->exarg_num  == EXBUSERR))
	{
	    IIUGerr(E_FG004B_AV_SegViol, UG_ERR_ERROR, 0);
	}

	return ((i4) EXDECLARE);	/* any other exception: run 
					** declare section
					*/
}

/*{
** Name:	IIFGscan()	- scan and process a template file.
**
** Description:
**	scan and process the template file passed in as an argument.
**	CALLED RECURSIVELY.
**
** Inputs:
**	infn	- pointer to name of input file.
**	wbuf_in - buffer of whitespace to prepend onto each output line.
**	cnt	- recursion count to catch infinite loops.
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
**	4/1/89 (pete)	written
**	18-jun-1992 (pete)
**	    Change the way statement parsing and $var substitution is done.
**	    (some of this work started in feb-92).
*/
static VOID
IIFGscan(infn, wbuf_in, cnt)	/* p_meta, symtbl */
char	*infn;
char	*wbuf_in;	/* prepend generated lines with this whitespace */
i4	cnt;		/* recursion count to catch infinite loops */
{
    LOCATION	inloc;
    FILE	*infp;
    STATUS	stat = OK;
    char 	buf[RECSIZE+1];		/* read each line into here */
    char 	tlbuf[RECSIZE+1];	/* concatenate long ##lines into here */
    char 	buf_copy[RECSIZE+1];	/* some ## stmts need clean copy of
					** "buf" (STgetwords screws up "buf")
					*/
    char	*p_buf_copy = buf_copy;
    i4		len_buf_copy = RECSIZE;

    char	*p_buf = buf;
    char	*p_tlbuf = tlbuf;
    i4		buf_len = RECSIZE;
    char	token[TOKSIZE+1];
    i4	bytcnt;
    IITOKINFO	*p_iitok;
    i4		line_cnt=0;
    i4		tos_orig = Ii_tos;    /* top of stack at start of IIFGscan() */
    char	*(p_wordarray[MAXTFWORDS]);
    i4		nmbr_words;	/* nmbr words found on ## line */
    char	wbuf_line[MAXWHITE+1];
    char	wbuf_tmp[MAXWHITE+1];
    i4		len;

    bool last_tlstmt_continued = FALSE;	/* was previous piece of ##stmt
					** continued?
					*/
    /* see if > MAXINCLUDE_NEST levels deep in ##include file recursion */
    if (++cnt > MAXINCLUDE_NEST)
    {
	i4 maxnest = MAXINCLUDE_NEST ;
	IIFGerr(E_FG0007_IncludeStmt_Nesting, FGFAIL, 2, 
		(PTR) infn, (PTR) &maxnest);
    }
	
    /* open input template file on II_TFDIR or II_CONFIG */
    opentf(infn, &inloc, &infp);
    /*
    **	if/when implement WHILE: do SIftell() before each SIgetrec and save
    **  in local. (must pass as recursion argument).
    */
    while (SIgetrec( buf, RECSIZE, infp) != ENDFILE)
    {
	line_cnt++;

	if ((buf[0]=='#' && buf[1]=='#') || (last_tlstmt_continued == TRUE))
	{
	    /* template language statement */

	    if (last_tlstmt_continued)
	    {
		/* add stmt portion in "buf" to what we already have in tlbuf */
		p_tlbuf = cat_strings(p_tlbuf, &buf_len, buf);
	    }
	    else
		STcopy(buf, p_tlbuf);	/* beginning of new ##stmt */

	    if ((last_tlstmt_continued = this_tlstmt_continued(p_tlbuf)) ==TRUE)
	    {
	    	/* Current ##statement is continued on next line.
		** above routine trims "\nl", if present.
		*/
		continue;	/* get next portion */
	    }

	    /* ASSERTION: we have complete ##statement in p_tlbuf to process */

	    /* Accumulate whitespace chars for nested ##include stmts.
	    ** (return value from savewhite resets p_tlbuf beyond the "##")
	    */
	    p_tlbuf = savewhite(p_tlbuf, wbuf_line);
	    STpolycat(2, wbuf_in, wbuf_line, wbuf_tmp);

	    /*
	    ** Parse and substitute $variables in a ##stmt; Remove comments;
	    ** Determine statement type (Tmp_stmt_type).
	    */
	    p_tlbuf = ParseAndSubTLStmt(p_tlbuf);

	    /* save copy of the buffer; STgetwords will write into it! */
	    len = STlength(p_tlbuf);
	    if (len > len_buf_copy)
	    {
		/* not enuf room in "buf_copy"; create bigger one.*/

		/* following memory is freed when code generation ends */
		p_buf_copy = (char *)IIFGgm_getmem((u_i4)(len+1), FALSE);
		len_buf_copy = len;
	    }
	    STcopy(p_tlbuf, p_buf_copy);

	    nmbr_words = MAXTFWORDS ;

	    /* Parse substituted ## line into whitespace-delimited words.
	    ** (note that STgetwords alters the buffer)
	    */
	    STgetwords(p_tlbuf, &nmbr_words, p_wordarray);   /*sets nmbr_words*/

	    /* only process ## lines with something on them.
	    ** NULL ## lines are ignored.
	    */
	    if (nmbr_words > 0)
	    {
		/* Reposition to beginning of expression (if any) in p_buf_copy
		** and strip buffer of trailing word (if any) e.g. for
		** IF stmt, strip the word "THEN".
		*/
		p_buf_copy = stripbuf(p_buf_copy, Tmp_expr_loc,
					Tmp_stmt_type, infn, line_cnt);

		/* process substituted ## statement */
	        tlstmt(nmbr_words, p_wordarray, wbuf_tmp, cnt, infn,
			line_cnt, p_buf_copy,
			(Tmp_stmt_type != NULL) ? Tmp_stmt_type->type
					        : FG_TOK_NONE);

		/*XXX For ENDWHILE, must reset file position and branch
		** back to while above. What about a WHILE when
		** Pgm_state==FLUSHING? need BREAK or CONTINUE?
		**
		** NOTE: WHILE statements were deferred cause of no immediate
		** need and SI_RACC files were required to make it work
		** easily (the .tf files are not RACC files!). Ideas if done
		** later:
		** 1. Build abstraction on top of SI and do template file
		**	processing in memory (at least portion inside WHILE).
		**	Handle recursion.
		** 2. Copy template into RACC file and process that.
		*/
	    }
    	    p_tlbuf = tlbuf;	/* reset to beginning */
    	    p_buf_copy = buf_copy;
	}
	else if (Pgm_state == PROCESSING)	/* 4gl statement */
	{
	    p_buf = buf;

	    token[0] = EOS;
	    /*
	    ** Write out initial whitespace for stmt. needed by ##include
	    ** statements, which causes generated 4gl statements to be
	    ** indented appropriate amount, if in a ##include file.
	    */
	    if (SIwrite (STlength(wbuf_in), wbuf_in, &bytcnt, outfp) != OK)
	    {
		IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);
	    }
	    while (gettok(&p_buf,token) != EOS)	/*EOS marks endof "buf*/
	    {
		/*
		** Historical note: $variables used to be iivariables; changed
		** to avoid confusion early in project. Hence the name
		** Iitokarray.
		*/
		if ( token[0] == '$')
		{
		    if (token[1] != '$')
		    {
		        /* template language variable */

		        p_iitok = IIFGftFindToken(token);    /* lookup token */

		        if (p_iitok == (IITOKINFO *)NULL)
		        {
			    /* no such $token in Emerald. */
			    if (SIwrite (STlength(token),
			    		token, &bytcnt, outfp) != OK)
			    {
	    		        IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);
			    }
		        }
		        else
		        {
			    /* found it. write translation to output (may be
			    ** empty string if $var not defined)
			    */
			    if (SIwrite (STlength(p_iitok->toktran),
					p_iitok->toktran, &bytcnt, outfp) != OK)
			    {
	    		        IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);
			    }
		        }
		    }
		    else
		    {
			/* $$environment_variable */
		        char *tran;

		        NMgtAt (&(token[2]), &tran);
		        if ((tran == NULL) || (*tran == EOS))
		        {
			    /* it's not defined. write out untouched. */
			    if (SIwrite (STlength(token),
			    		token, &bytcnt, outfp) != OK)
			    {
	    		        IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);
			    }
		        }
		        else
		        {
			    /* it's defined */
			    _VOID_ STlcopy (tran, nm_scratch, MAX_LOC);
			    if (SIwrite (STlength(nm_scratch),
					nm_scratch, &bytcnt, outfp) != OK)
			    {
	    		        IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);
			    }
		        }
		    }
		}
		else
		{
		    /* not a '$' token */
	    	    if (SIwrite (STlength(token), token, &bytcnt, outfp) != OK)
		    {
	    		IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);
		    }
		}
	    }
	}
	else
	{
		/*
		** Pgm_state == FLUSHING & 4gl line; skip over all
		** 4gl lines when Pgm_state == FLUSHING
		*/
	}
    }

    if (stat == OK)
    {
	/* check for unclosed "if/endif" blocks or too many "endif"s in file */
	if (tos_orig == Ii_tos)
	    ;	/* Success: balanced number of if/endif's */
	else if (tos_orig > Ii_tos)
	{
	    /* extra "endif" found in file */
	    IIFGerr(E_FG0008_TooMany_Endifs, FGFAIL, 2,
		(PTR) &line_cnt, (PTR) infn);
	}
	else
	{
	    /* unclosed "##if" found in file */
	    IIFGerr(E_FG0009_Unclosed_IfStmt, FGFAIL, 1, (PTR) infn);
	}
    }

    if (infp != NULL)
    {
	SIclose(infp);
	_VOID_ popfile();
    }
}

/*{
** Name:	this_tlstmt_continued - check if current stmt continues.
**
** Description:
**	Check if current ##statement continues on next line. Checks to
**	see if statement ends in "\nl" (backslash, newline).
**
** Inputs:
**	buf	- statement buffer to check.
**
** Outputs:
**	Zaps the \nl by writing EOS to buf at position where \ was found.
**	Always returns TRUE when this occurs.
**
**	Returns:
**		TRUE if statement continues (and \nl was removed); FALSE
**		otherwise.
** History:
**	25-jun-1992 (pete)
**	    Initial entry.
**	30-jul-2001 (toumi01)
**	    Quiet the i64_aix compiler, which needs help converting char
**	    to int for the strrchr function.
**	30-oct-2001 (abbjo03)
**	    Undo above change and replace second argument to STrchr by literal
**	    single character backslash (instead of char array).
*/
static bool
this_tlstmt_continued(buf)
char	*buf;
{
	char *pos, *tmp;
	bool ret = FALSE;

	/* find last occurance of "\" in string */
	if ((pos = STrchr(buf, '\\')) != NULL)
	{
	    tmp = pos;	/* remember where it is */
	    CMnext(pos);
	    if (CMcmpcase(pos, Newline) == 0)
	    {
		ret = TRUE;	/* found \nl at end of string */
		*tmp = EOS;	/* null terminate at place where \ was */
	    }
	}

	return ret;
}

/*{
** Name:	ParseAndSubTLStmt - parse ##statement & substitute for $vars.
**
** Description:
**	Parse ##stmt and substitute values for any $vars or $$vars.
**	Also, remove --comments & determine statement type (set Tmp_stmt_type).
**
** Inputs:
**	p_buf		-- statement to parse (## already removed).
**
** Outputs:
**	
**	Returns:
**		pointer to char buffer with processed, substituted statement.
**
** History:
**	15-jun-1992 (pete)
**	    Initial version.
*/
static char *
ParseAndSubTLStmt(p_buf)
char	*p_buf;
{
#ifdef DEBUG
	char *in_buf_orig = p_buf;
#endif
	char *out_buf = Tmp_buf;
	char *out_string;
	i4 out_len = RECSIZE;
	i4 num_words = 0;

#ifdef DEBUG
	LOCATION dmploc;
	FILE  *dmpfile;
        char locbuf[MAX_LOC+1];
#endif

	Tmp_scan_mode = Tmp_tok_type = FG_TOK_NONE;
	Tmp_stmt_type = (FG_STATEMENT *)NULL;
	Tmp[0] = Tmp_saved_tok[0] = Tmp_buf[0] = EOS;
	Tmp_expr_loc = FG_EXPR_LOC_NOTSET;

	while (get_tltok(&p_buf) != EOS)
	{
	    out_string = Tmp;

	    if (Tmp_tok_type != FG_TOK_WHITESPACE)
	    {
		num_words++;
		save_stmt(num_words, Tmp);	/* save info about this stmt */
	    }

	    if (Tmp_tok_type == FG_TOK_WHITESPACE)
	    {
		/* whitespace characters */
	    }
	    else if (Tmp_tok_type == FG_TOK_WORD)
	    {
		/* unquoted token; string or number */
	    }
	    else if (Tmp_tok_type == FG_TOK_OTHER)
	    {
		/* probably quoted string with "--" (comment) in it. */
	    }
	    else if (Tmp_tok_type == FG_TOK_VARIABLE &&
		/* Should NOT translate the variable_name in its DEFINE stmt */
		(Tmp_stmt_type->type != FG_STMT_DEFINE || num_words > 2))
	    {
		/* $variable, translate it */
		IITOKINFO *p_tok;

		/* translate, allocate memory & write out translation */
                p_tok = IIFGftFindToken(Tmp);
                if (p_tok == (IITOKINFO *)NULL)
                {
                    /* no such $variable */
                }
                else
                {
		    /* found it. */
		    out_string = p_tok->toktran;
                }
	    }
	    else if (Tmp_tok_type == FG_TOK_ENV_VAR)
	    {
		/* $$environment_variable */
		char *tran;

		/* translate, allocate memory & write out translation */

		NMgtAt((Tmp + STlength(ERx("$$"))), &tran);
		if ((tran == NULL) || (*tran == EOS))
		{
                    /* not defined. write out untouched. */
		}
		else
		{
                    /* defined */
		    out_string = tran;
		}
	    }
	    else if (Tmp_tok_type == FG_TOK_QUOTE)
	    {
		/* track when we're inside a quoted string */
		Tmp_scan_mode = (Tmp_scan_mode == FG_TOK_NONE)
					? FG_TOK_QUOTED_STRING
					: FG_TOK_NONE;
	    }
	    else if (Tmp_tok_type == FG_TOK_COMMENT)
	    {
		STcat(out_buf, ERx("\n"));	/* write "\nEOS" */

		break;
	    }

	    out_buf = cat_strings(out_buf, &out_len, out_string);
	}
#ifdef DEBUG
        STcopy ("iifgparse.dmp", locbuf);
        LOfroms (FILENAME&PATH, locbuf, &dmploc);

	if (SIopen (&dmploc, ERx("a"), &dmpfile) != OK)
	{
	    SIprintf("Error: unable to open dump file iifgparse.dmps");
	}
	SIfprintf(dmpfile, "Input:\n%s\nOutput:\n%s\n", in_buf_orig, out_buf);

        if (dmpfile != NULL)
            SIclose(dmpfile);
#endif
	return out_buf;
}

/*{
** Name:	cat_strings - concatenate strings.
**
** Description:
**	Concatenate two strings, and allocate a new buffer if output
**	buffer is not large enough.
**
** Inputs:
**	in		- buffer to concatenate onto "out".
**
** Outputs:
**	out		- output buffer. concatenate onto here.
**	out_len		- declared length of "out" buffer. Written to
**			  if new buffer is allocated.
**
**	Returns:
**		Pointer to buffer with concatenated string.
**
**	Exceptions:
**
** Side Effects:
**	Allocates memory.
**
** History:
**	25-jun-1992 (pete)
**	    Initial version.
*/
static char *
cat_strings(out, out_len, in)
char	*out;
i4	*out_len;
char	*in;
{
	i4 len;

	len = STlength(out) + STlength(in);
	if (len > *out_len)
	{
	    /* must allocate bigger buffer */
	    char *tmp;

	    *out_len = len + *out_len;	/* set new value for out_len */

	    /* following memory is freed when code generation ends */
	    if ((tmp = (char *)IIFGgm_getmem((u_i4)(*out_len+1), FALSE))
			== (char *)NULL)
	    {
		IIFGerr(E_FG0030_OutOfMemory, FGFAIL, 0);
		/*NOTREACHED*/
	    }

	    STcopy(out, tmp);
	    out = tmp;	/* point "out" at new, larger buffer */
	}

	STcat(out, in);

	return out;
}

/*{
** Name:	save_stmt	- save statement info.
**
** Description: find & save statement type. Also, optionally save a
**		statement token.
**
** Inputs:
**	word		current word number on ##stmt
**	tok		the actual word
**
** Outputs:
**	Writes to Tmp_stmt_type and (optionally) to Tmp_saved_tok.
**
**	Returns:
**		VOID
**
** History:
**	17-jun-1992 (pete)
**	    Initial version.
*/
static VOID
save_stmt(word, tok)
i4	word;	/* current word number on ##stmt */
char	*tok;	/* the actual word */
{
	if (word == 1)
	{
	    /* first word on ##stmt gives type ("IF", "DEFINE", etc). */
	    Tmp_stmt_type = getstmt_type(tok);
	}

	if (Tmp_stmt_type != (FG_STATEMENT *)NULL)
	{
	    if (word == Tmp_stmt_type->save_input_word)
	    {
		/* Save current word. For example, on IFDEF stmt, need to
		** save name of $variable (second word), before translation.
		*/
		STcopy(tok, Tmp_saved_tok);
	    }

	    if (word == Tmp_stmt_type->expr_begin)
	    {
		/* Word is beginning of expression to be evaluated. Remember
		** this relative position in substituted statement.
		*/
		Tmp_expr_loc = STlength(Tmp_buf);
	    }
	}
}

/*{
** Name:	get_tltok	- get next token. Used for ##statements.
**
** Description: get next token from ## statement.
**
** Inputs:
**	p_buf		address of pointer to buffer where ##stmt resides.
**
** Outputs:
**	Writes to globals: Tmp & Tmp_tok_type.
**
**	Returns:
**		first character of Tmp.
**
** History:
**	17-jun-1992 (pete)
**	    Initial version.
*/
static i4
get_tltok(p_buf)
char	**p_buf;
{
	char *temp = Tmp;

	*temp = EOS;
	Tmp_tok_type = FG_TOK_OTHER;

	if (CMwhite(*p_buf))
	{
	    Tmp_tok_type = FG_TOK_WHITESPACE;

 	    while (CMwhite(*p_buf))
        	CMcpyinc(*p_buf, temp);
	}
	else if (CMcmpcase(*p_buf, Dollar) == 0)
	{
            CMcpyinc(*p_buf, temp);
            if (CMcmpcase(*p_buf, Dollar) == 0)
	    {
		Tmp_tok_type = FG_TOK_ENV_VAR;
            	CMcpyinc(*p_buf, temp);
	    }
	    else
	    {
		Tmp_tok_type = FG_TOK_VARIABLE;
	    }

	    /* collect rest of variable name */
	    while (CMalpha(*p_buf) || CMdigit(*p_buf) ||
	    	(CMcmpcase(*p_buf, Underscore) ==0))
                CMcpyinc(*p_buf, temp);
	}
        else if (CMcmpcase(*p_buf, Quote) == 0)
	{
	    Tmp_tok_type = FG_TOK_QUOTE;
            CMcpyinc(*p_buf, temp);
	}
        else if ((CMcmpcase(*p_buf, Minus) == 0) &&
		(Tmp_scan_mode != FG_TOK_QUOTED_STRING))
	{
            CMcpyinc(*p_buf, temp);
            if (CMcmpcase(*p_buf, Minus) == 0)
	    {
		Tmp_tok_type = FG_TOK_COMMENT;
            	CMcpyinc(*p_buf, temp);
	    }
	    else
	    {
		Tmp_tok_type = FG_TOK_OTHER;
	    }
	}
	else if (CMalpha(*p_buf) || CMdigit(*p_buf) ||
			(CMcmpcase(*p_buf, Underscore) ==0))
	{
	    Tmp_tok_type = FG_TOK_WORD;

            while (CMalpha(*p_buf) || CMdigit(*p_buf) ||
                (CMcmpcase(*p_buf, Underscore) ==0))
                CMcpyinc(*p_buf, temp);
	}
	else if (**p_buf == EOS)
	{
	    /* EOS could be preceeded by whitespace gathered above */
	}
        else
        {
	    Tmp_tok_type = FG_TOK_OTHER;
            CMcpyinc(*p_buf, temp);

            /* collect rest of token */
            while (!isdelim(*p_buf))
                CMcpyinc(*p_buf, temp);
        }

        *temp=EOS;      /* NULL terminate token */

	return (i4)*Tmp;
}

/*{
** Name:	isdelim	- check if character is a delimiter.
**
** Description: test if a character is in the Delims array, or is
**		white space (including \n) or EOS. Returns TRUE if it is.
** Inputs:
**	c	pointer to character string with character to test.
**
** Outputs:
**	Returns:
**		TRUE if it is a delimited; FALSE otherwise.
** History:
**	17-jun-1992 (pete)
**	    Initial version.
*/
static bool
isdelim(c)
char *c;
{
        if (is_in(c, Delims, NMBR_DELIMS) || CMwhite(c) || *c == EOS)
            return TRUE;

        return FALSE;
}

/*{
** Name:	is_in	- check if character is present in an array.
**
** Description: loop thru "s[]" array and check if "ch" is present.
**
** Inputs:
**	ch	character to check for.
**	s[]	array of character strings. check if "ch" is in here.
**	cnt	number of items in "s[]".
**
** Outputs:
**	Returns:
**		TRUE if it is in array; FALSE otherwise.
** History:
**	17-jun-1992 (pete)
**	    Initial version.
*/
static bool
is_in(ch, s, cnt)
char *ch;
char *s[];
i4   cnt;
{
        register i4  i;

	for (i=0; i < cnt; i++)
	    if (CMcmpcase(ch, s[i]) == 0)
		return TRUE;
	return FALSE;
}

/*{
** Name:	fgsetloc  - translate environment variable & set LOCATION struct
**
** Description:
**	translate an environment variable (s.a. II_CONFIG) and make
**	sure it points to a valid PATH; then set up a LOCATION structure
**	for this path. 
**
** Inputs:
**	env_var		environment variable.
**	char p_dirbuf[MAX_LOC+1]    buffer to translate environment var into.
**	p_loc		pointer to LOCATION structure.
**
** Outputs:
**	filled in LOCATION structure.
**
**	Returns:
**		OK if environment variable was set.
**		FAIL otherwise.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	sets p_dirbuf to EOS if environment variable not set.
**
** History:
**	4/13/89 (pete)	written
*/
static STATUS
fgsetloc(env_var, p_dirbuf, p_loc)
char	*env_var;
char	*p_dirbuf;	/* set to "env_var" translation */
LOCATION *p_loc;
{
	char *p_nm;	/* pointer for NMgtAt to use */
	STATUS stat = FAIL;
	i2 flag ;	/* change back to "nat" after LOisdir() bug fix */

	NMgtAt(env_var, &p_nm);
	if ( (p_nm == NULL) || (*p_nm == EOS))
	{
		/* environment variable not set */
		*p_dirbuf = EOS;
	}
	else
	{	/* env_var translated ok */
		STcopy (p_nm, p_dirbuf);
		LOfroms (PATH, p_dirbuf, p_loc);
		
		if ( (LOisdir (p_loc, &flag) != OK) || (p_loc == NULL) )
		{
	    	    IIFGerr(E_FG000A_Directory_ChkError, FGFAIL, 2,
			(PTR) env_var, (PTR) p_dirbuf);
		    *p_dirbuf = EOS;
		}
		else if (flag != ISDIR)
		{
	    	    IIFGerr(E_FG000B_Invalid_Directory, FGFAIL, 2,
			(PTR) env_var, (PTR) p_dirbuf);
		    *p_dirbuf = EOS;
		}
		else
		    stat = OK; /* LOCATION structure all set */
	}
	return stat;
}

/*{
** Name:	opentf	- open a template file.
**
** Description:
**	given a filename, determine where the template file with that name
**	exists (II_TFDIR checked first, then check II_CONFIG). Once found,
**	set up a LOCATION structure for the file and then open if.
**
** Inputs:
**	fn	template filename to open.
**	loc	pointer to location structure for this template file.
**	infp	pointer to FILE structure. SIopen will use this.
**
** Outputs:
**	assigns location structure & returns STATUS value.
**
**	Returns:
**		FAIL if no such file can be found, or if an error occurs.
**		OK if file was opened ok.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	no intentional ones, but this was written on day our stock dropped from
**	15 --> 10 !!
**
** History:
**	4/13/89 (pete)	written
**	17-apr-1991	(pete)
**		Fix error in argument to "pushfile()" to *infp (was infp).
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics in.
*/
static VOID
opentf(fn, loc, infp)
char *fn;
LOCATION *loc;
FILE	**infp;
{
	LOCATION tfnloc;	/* template file to process. */
	STATUS	tf_stat = FAIL;
	STATUS	config_stat = FAIL;
	char	locbuf[MAX_LOC + 1];

	/* Copy file name since it may be a string literal */
	STcopy(fn, locbuf);

	/* build LOCATION structure from "locbuf" for use below */
	if (LOfroms (FILENAME, locbuf, &tfnloc) != OK)
	{
	    IIFGerr(E_FG000C_Bad_Filename, FGFAIL | FGSETBIT, 1, (PTR) fn);
	}

	/* see if file exists in II_TFDIR */
	if (tfdir[0] != EOS)
	{
	    /* add filename to II_TFDIR's LOCATION struct */
	    LOstfile(&tfnloc, &tfloc);	/* tfloc is global */

	    if (LOexist(&tfloc) == OK)
	    {
		LOfroms (FILENAME&PATH, locbuf, loc);
		/* open it */
#ifdef hp9_mpe
		if ( (tf_stat = SIfopen(&tfloc, ERx("r"), SI_TXT, 0, 
					infp)) != OK)
#else
		if ( (tf_stat = SIopen(&tfloc, ERx("r"), infp)) != OK)
#endif
		{
		    /*
		    ** Don't issue error message, since there may be
		    ** one in II_CONFIG that SIopens ok.
		    */
		}
		else
		    pushfile(*infp);	/* push open file on stack */
	    }
	    else
		; /* not an error yet, cause may exist in II_CONFIG */
	}

	if ( (tf_stat == FAIL) && (IIFGcd_configdir[0] != EOS))
	{
		/* file not in II_TFDIR; see if exists in II_CONFIG */

		/* add filename to II_CONFIG's LOCATION struct */
	        LOstfile(&tfnloc, &IIFGcl_configloc);	/* IIFGcl_configloc
							** is global
							*/
	    	if (LOexist(&IIFGcl_configloc) == OK)
		{
		    /* open it */
		    LOfroms (FILENAME&PATH, locbuf, loc);
#ifdef hp9_mpe
		    if ((config_stat = SIfopen(&IIFGcl_configloc,
						ERx("r"), SI_TXT, 0, 
						infp))!= OK)
#else
		    if ((config_stat = SIopen(&IIFGcl_configloc,
						ERx("r"), infp))!= OK)
#endif
		    {
	    		IIFGerr(E_FG000D_Cant_Open_File, FGFAIL, 1, (PTR) fn);
		    }
		    else
		        pushfile(*infp);	/* push open file on stack */
		}
		else
		{
	    	    IIFGerr(E_FG000E_File_Not_Found, FGFAIL, 1, (PTR) fn);
		}
	}

	if ( (config_stat == FAIL) && (tf_stat == FAIL) )
	{
	    IIFGerr(E_FG000D_Cant_Open_File, FGFAIL, 1, (PTR) fn);
	}
	else
	    ;	/* all is ok */
}

/*{
** Name:	tlstmt() - process a parsed template language (##) statement
**
** Description:
**
**   "##if/##else/##endif" statements:
**   --------------------------
**   Program-level state (Pgm_state) can be either PROCESSING or FLUSHING,
**	& is determined by evaluating "##if" statements in the template file.
**
**   There is a stack of "if" statements (the stack handles nested "## if"
**	statements). A state is stored along with each "if" on the stack.
**	The stack is global to source file.
**
**   How program-level state is set:
**	  Program state begins as PROCESSING.
**	  When a "##if" evaluates to FALSE, that sets state to FLUSHING.
**	  When a "##if" evaluates to TRUE, that does not set program state; that
**	    "if" is pushed onto the stack with a state = the program state
**	    BEFORE the "if" was encountered.
**	  An "endif" pops an "if" from stack & that resets state to the
**	    state saved for that "if" on the stack.
**
**   If program-level state is "PROCESSING":
**	A "##endif" pops an "if" from the stack & Pgm_state is reset.
**	if a "## if" evaluates to TRUE, push it along with state PROCESSING.
**	if a "## if" evaluates to FALSE, set program-level state to FLUSHING
**	  and push "if" on stack with state PROCESSING (state prior to when
**	  this "if" was encountered).
**
**   If program-level state is "FLUSHING":
**	No need to scan non-## lines ("FLUSH" 4gl statements).
**	No need to fully evaluate ## lines, except for if/endif statements.
**	A "##endif" pops a "##if" from the stack & resets Pgm_state.
**
**   "##else" statements:
**   --------------------
**   These do not "popif()" anything or have any effect on what the "##endif"
**	statement does (described above). Neither does it have any effect on the
**	IFINFO stack. But "##else" can reset the Pgm_state,
**	based on the Ii_ifStack.iftest & Ii_ifStack.pgm_state of the matching
**	(most recent) "##if" statement. If the matching ##if evaluated to FALSE
**	& its Ii_ifStack.pgm_state is PROCESSING (the Pgm_state in effect
**	when the ##if was encountered), then a ##else
**	sets the global Pgm_state to PROCESSING. In all other cases the
**	##else sets the global Pgm_state to FLUSHING. ##else does not alter
**	the stack contents, it just reads the stack via a pop and then a
**	push.
**
**   "include" statement:
**   --------------------
**   These cause a recursive call to "IIFGscan" to process the input file,
**	unless the program state is FLUSHING, in which case these statements
**	are simply skipped over.
**
**   "generate" statement:
**   -------------------
**   These cause a call to "generate" to generate code, unless the program state
**	is FLUSHING, in which case these statements are simply skipped over.
**
** Inputs:
**	nmbr_words  - number of pointers in p_wordarray.
**	p_wordarray - array of pointers to words on the ## line. p_wordarray[0]
**		      contains text of statement type ("IF", etc).
**	p_wbuf      - array with whitespace to be used when generating code.
**	cnt	    - for infinite loop control in recursion.
**	infn	    - name of input file currently processing (for err msgs).
**	line_cnt    - current line number in input file (for error msgs).
**	buf	    - copy of original input buffer (not the one pointed to
**		      by p_wordarray).
**	stmt_type   - the ## statement type. (FG_STATEMENT.type).
**
** Outputs:
**	new program-level state. (sets global variable) + generated code.
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
**	4/1/89 (pete)	written
**	24-feb-1992 (pete)
**	    Change to allow conditions and expressions on ##IF stmts & to
**	    support ##DEFINE and ##WHILE statements. Some stmts use
**	    the parsed word array; others use the string buffer. Added arguments
**	    buf_altered, buf, and stmt_type.
*/
static VOID
tlstmt(nmbr_words, p_wordarray, p_wbuf, cnt, infn, line_cnt, buf, stmt_type)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
i4	cnt;
char	*infn;
i4	line_cnt;
char	*buf;
i4	stmt_type;
{
char	incl_name[MAX_LOC+1];

	if (stmt_type == FG_STMT_IF)
	{
		IFINFO if_res;

		if_res.iftest = IIFGbseBoolStmtEval(buf, infn, line_cnt);

		if_res.pgm_state = Pgm_state;	/* Pgm_state before ##if seen*/
		if_res.has_else  = FALSE ;	/* "else" not seen yet */

		/* Push result of if evaluation + pgm_state onto "if" stack */
		IIFGpushif(&if_res, infn, line_cnt);

		if (if_res.iftest == FALSE)
		{
		    Pgm_state = FLUSHING ;  /* reset global program state */
		}
	}
	else if (stmt_type == FG_STMT_ELSE)
	{
		Pgm_state = IIFGelse_eval(infn, line_cnt);
	}
	else if (stmt_type == FG_STMT_ENDIF)
	{
		IFINFO *if_res; 

		if_res = IIFGpopif(infn, line_cnt);	/* pop matching
							** "if" from stack
							*/
		/* Restore global program state */
		Pgm_state = if_res->pgm_state;
	}
	else if (stmt_type == FG_STMT_INCLUDE)
	{
	    if (Pgm_state != FLUSHING)
	    {
		/* p_wordarray[1] contains file name to open */
		/* Store it in a real buffer before calling  */
		/* recursively, otherwise pointer value may  */
		/* be lost                                   */
		STcopy(p_wordarray[1], incl_name); 
		IIFGscan(incl_name, p_wbuf, cnt);   /* recursion */
	    }
	}
	else if (stmt_type == FG_STMT_GENERATE)
	{
	    if (Pgm_state != FLUSHING)
	    {
		/*
		** Drop word 'generate'; p_wordarray[1] names type of
		** code to be generated ("query", "lookup", etc).
		*/
		generate (--nmbr_words, &p_wordarray[1], p_wbuf,
				infn, line_cnt);
	    }
	}
	else if (stmt_type == FG_STMT_DEFINE)
	{
	    if (Pgm_state != FLUSHING)
	    {
                _VOID_ IIFGdseDefineStmtExec(Tmp_saved_tok, buf, nmbr_words,
				infn, line_cnt);
	    }
	}
	else if (stmt_type == FG_STMT_DOC)
	{
		/* skip ##DOC statements for now */
	}
#if 0	/* WHILE never implemented */
	/* removed from stmts[] array above:
	/*
	** #define FG_STMT_WHILE
	** #define FG_STMT_ENDWHILE
	** {ERx("WHILE"),	FG_STMT_WHILE,	  FG_SUB_VARS_LATER}, 
	** {ERx("ENDWHILE"),	FG_STMT_ENDWHILE, FG_SUB_VARS_LATER},
	*/
	else if (stmt_type == FG_STMT_WHILE)
	{
	    IFINFO if_res;
	    if_res.iftest = IIFGbseBoolStmtEval(buf, infn, line_cnt);

	    if_res.pgm_state = Pgm_state;  /* Pgm_state before ##while seen*/
	    if_res.has_else  = FALSE;	   /* WHILE does not have "##else" */

	    /* Push result of WHILE evaluation + pgm_state onto "if" stack */
	    /*XXX
	    **	Must also save the buffer position and statement type
	    **	(stmt type used only for catching errors).
	    */
	    IIFGpushif(&if_res, infn, line_cnt);

	    if (if_res.iftest == FALSE)
	    {
		Pgm_state = FLUSHING ;  /* reset global program state */
	    }
	}
	else if (stmt_type == FG_STMT_ENDWHILE)
	{
	    /*XXX
	    **	pop off "if" stack.
	    **	should be a WHILE. (else error)
	    **	reset Pgm_state to the state when WHILE seen.
	    **		if state==PROCESSING then
	    **		    SIfseek() to file position saved with WHILE
	    **		    statement on "if" stack. (just before WHILE stmt).
	    **		else
	    **		    do nothing (continue flushing)
	    */
	}
#endif
	else if ((stmt_type == FG_STMT_IFDEF) || (stmt_type == FG_STMT_IFNDEF))
	{
		/* treat almost exactly like a ##if */
		/* push it like an "if", but write new ifdef_eval function */
		IFINFO if_res;
		bool tmp;

		tmp = IIFGifdef_eval(Tmp_saved_tok, infn, line_cnt);

		if_res.iftest = (stmt_type == FG_STMT_IFDEF) ? tmp : !tmp ;

		if_res.pgm_state = Pgm_state;	/* Pgm_state before ##if seen*/
		if_res.has_else  = FALSE ;	/* "else" not seen yet */

		/* Push result of if evaluation + pgm_state onto "if" stack */
		IIFGpushif(&if_res, infn, line_cnt);

		if (if_res.iftest == FALSE)
		{
		    Pgm_state = FLUSHING ;  /* reset global program state */
		}
	}
	else if (stmt_type == FG_STMT_UNDEF)
	{
	    /* ##UNDEF $variable */
	    if (Pgm_state != FLUSHING)
	    {
		if (!IIFGuvUndefVariable(Tmp_saved_tok))
		{
		    /* No such $variable, or attempt to UNDEF a builtin $var.
		    ** Give warning and continue.
		    */
		    IIFGerr(E_FG0068_No_Such_Variable, FGCONTINUE,
		    	3, (PTR) Tmp_saved_tok, (PTR) &line_cnt, (PTR)infn);
		}
	    }
	}
	else
	{
	    /* Note: p_wordarray[0] contains text of statement type
	    ** ("IF", etc)
	    */
	    IIFGerr(E_FG000F_Bad_TL_Statement, FGFAIL, 3, 
		(PTR) p_wordarray[0], (PTR) &line_cnt, (PTR) infn);
	}
}

/*{
** Name:	getstmt_type - get the statement type.
**
** Description:
**	Given the statement's keyword, find its FG_STATEMENT* type.
**
** Inputs:
**	p_word		- the statement keyword ("if", "generate", etc)
**
** Outputs:
**
**	Returns:
**		pointer to the statement info, or NULL if unknown statement.
**
** Side Effects:
**	NONE intended, but I wrote this after being told we are in
**	some financial trouble -- this (Amethyst) project may even be cut!
**
** History:
**	20-mar-1992 (pete)
**	    Initial version.
*/
static FG_STATEMENT *
getstmt_type(p_word)
char	*p_word;
{
	i4 i;
	char ttok[TOKSIZE+1];

	STcopy(p_word, ttok);
	CVupper(ttok);

	for (i=0; i < NMBR_STMTS; i++)
	{
	    if (STequal(ttok, stmts[i].stmt))
		return &(stmts[i]);
	}

	return (FG_STATEMENT *)NULL;
}

/*{
** Name:	generate  - call appropriate function to generate 4gl code.
**
** Description:
**
** Inputs:
**	nmbr_words	number of word pointers in p_wordarray.
**	p_wordarray	array of pointers to function name + its args. (the word
**			"generate" is NOT one of the arguments).
**	p_wbuf		pointer to buffer of white space to prepend on each
**			generated line of code.
**	infn		name of input file currently processing (for error msgs.
**	line_cnt	current line number in input file (for error msgs).
**
** Outputs:
**	generated code.
**
**	Returns:
**		STATUS value
**
**	Exceptions:
**		EXDECLARE
**
** Side Effects:
**
** History:
**	4/9/89 (pete)	written
*/
static VOID
generate (nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	int i;
	STATUS stat;

	if (nmbr_words == 0)
	{
	    /* ##generate statement has no arguments */
	    IIFGerr(E_FG0010_Missing_Args, FGFAIL, 2,
		(PTR) &line_cnt, (PTR) infn);
	}

	for (i=0; i<NMBRCGENFNS; i++)
	{
	    /* lookup function. if find, then call it. */
	    if (STbcompare(p_wordarray[0], 0,
	   		   cgenarray[i].cgen_type, 0, TRUE) == 0)
	    {
		/*
		** Found function; now call it.
		** Drop the code gen keyword "query", "hidden_fields", etc;
		** Second word in p_wordarray should be first argument
		** passed to function. Also, decrement nmbr_words, since
		** it currently also counts the word containing the
		** "cgen_type", matched above ("query", "hidden_fields", etc).
		*/
		stat = (*cgenarray[i].cgen_fn)(--nmbr_words, &p_wordarray[1],
			p_wbuf, infn, line_cnt);

		if (stat != OK)
		    EXsignal (EXDECLARE, 0);	/* & return FAIL to caller */
		else
		    break;
	    }
	}

	/* unknown type of ##generate statement */
	if (i == NMBRCGENFNS)
	{
	    IIFGerr(E_FG0011_Bad_Generate_Stmt, FGFAIL, 3,
		(PTR) p_wordarray[0], (PTR) &line_cnt, (PTR) infn);
	}
}

/*{
** Name:	savewhite()	- save leading white chars following ##
**
** Description:
**	save leading white space following ## on line; use later to indent
**	generated code. don't treat \n as whitespace (which CMwhite
**	likes to do).
**
** Inputs:
**	pointer to buffer where white space is to be read (p_buf).
**	pointer to buffer where white space is to be saved (p_wbuf).
**
**	p_buf is assumed to point to beginning # of a ## line.
**	p_wbuf is buffer to copy leading white space into.
**	MAXWHITE tells max size of whitespace buffer.
**
** Outputs:
**	moves whitespace into p_wbuf and null terminates it.
**
**	Returns:
**		pointer to first non-white char in read buffer (following ##)
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	4/1/89 (pete)	written
*/
static char
*savewhite(p_buf, p_wbuf)
register char *p_buf;
register char *p_wbuf;	/* copy white space from p_buf into here */
{
	i4	i;

	p_buf++; p_buf++;	/* move past "##" */
	if (*p_buf != '\n' && CMwhite(p_buf))
	{
		STcopy(PAD2, p_wbuf);	   /* 2 blanks to indent same as ## */
		p_wbuf++; p_wbuf++;	   /* move past two blanks */
	   	/*
		** Now walk through p_buf & copy whitespace into p_wbuf till get
		** to first non-white char in p_buf or to '\n', or till
		** white space buffer is full. Start counter at 2 cause
		** whitespace buff already has two blanks.
		*/
	    	for (i=2;  i<MAXWHITE && *p_buf != '\n' && CMwhite(p_buf);
					i++, CMcpyinc(p_buf,p_wbuf))
			;
		*p_wbuf = EOS;
	}
	else
	{
		/*
		** Non-white starts right after ##. That means don't
		** indent generated code
		*/

		*p_wbuf = EOS;
	}
	return p_buf ;
}

/*{
** Name:	gettok - get next token from buffer; used for non-## lines.
**
** Description:
**	Picks next token from input buffer and copies it to output buffer.
**	Returned token is terminated by EOS.
**	Token = any $variable name, or group of characters that would be valid
**          in a $variable body, or any single character that could not be
**          part of a $variable name.
**	$variable name ::=  $ { [a-z] | [0-9] | _ }
**
** Possible Performance Improvements:
**	Gather up all non-$variable characters at once, rather than 1 at a time.
**
** Inputs:
**	p_buf	- address of pointer to input buffer to get next token from.
**
** Outputs:
**	token	- copy next token here. terminate with EOS.
**
**	Returns:
**		first character of the output token, cast into a "nat".
** History:
**	4/1/89 (pete)	written
**	12-jun-1992 (pete)
**	    Changed definition of a "token" FROM "any single NON-CMnmchar
**	    character, or any group of CMnmchar characters" TO
**	    "any $variable name, or group of characters that would be valid
**	    in a $variable body, or any single character that could not be
**	    part of a $variable name". Reason: want to be able to substitute
**	    strings like "$var1$var2", which formerly would have been treated
**	    as a single token (all are valid CMnmchar characters).
*/
static i4
gettok(p_buf,token)
register char   **p_buf;
register char   *token;
{
    register i4  i =0;					/* 6-x_PC_80x86 */
    char *p_obuf = *p_buf;

    while ( ((CMalpha(*p_buf) || CMdigit(*p_buf) ||
	    (CMcmpcase(*p_buf, Underscore) ==0))
	     || (i == 0 && (CMcmpcase(*p_buf, Dollar) ==0))	  /* $var */
	     || ((i == 1 && (CMcmpcase(*p_buf, Dollar) == 0)) &&  /* $$var */
		 (CMcmpcase(p_obuf, Dollar) == 0)) )
	 && (i < TOKSIZE))
    {
	CMbyteinc(i, *p_buf);		/* i++ */
	CMcpyinc (*p_buf, token);
    }

    if (i >= TOKSIZE)
    {
	i4 tok = TOKSIZE ;
	IIFGerr(E_FG0012_Token_TooLong, FGFAIL, 2,
		(PTR) p_obuf, (PTR) &tok);
    }

    if (i==0)
    {
	CMcpyinc (*p_buf, token);
    }
    *token = EOS;
    return ((i4) *p_obuf);
}

/*{
** Name:        pushfile()  - push a file onto Filestack.
**
** Description:
**	Push a file pointer onto stack.
**
** Inputs:
**	FILE	*p_file;	file to push onto stack.
**
**      Returns:
**              VOID
**
** History:
**      4/16/91 (pete)
**		Initial version.
*/
static VOID
pushfile(p_file)
FILE *p_file;
{
        if (Fs_top < MAXINCLUDE_NEST)
	{
	    Filestack[Fs_top] = p_file;
	    Fs_top++;
	}
}

/*{
** Name:        popfile()     - pop a file from Filestack.
**
** Description:
**	pop a file from the Filestack.
**
** Inputs:
**
** Outputs:
**
**      Returns:
**		(FILE *)
**
** History:
**      4/16/91 (pete)
**		Initial version.
*/
static FILE
*popfile()
{
	Fs_top--;

	if (Fs_top < 0)
	    Fs_top = 0;

        return (Filestack[Fs_top]);
}

/*{
** Name:        closefiles()     - close all files on stack.
**
** Description:
**	close all files on Filestack.
**
** Inputs:
**
** Outputs:
**
**	Side Effects:
**
**		decrements the global: Fs_top.
**
**      Returns:
**		VOID
**
** History:
**      4/16/91 (pete)
**		Initial version.
*/
static VOID
closefiles()
{
	for (Fs_top--; Fs_top >= 0; Fs_top--)
	{
	    SIclose(Filestack[Fs_top]);
	}

	if (Fs_top < 0)
	    Fs_top = 0;
}

/*{
** Name:	stripbuf  - remove unneeded parts of the statement buffer.
**
** Description:	clean up the statement buffer.
**
** Inputs:
**	expr_loc	- location within "buf" of expression start.
**			  (else 
**	stmt		- struct of detailed info about this statement.
**	infn		- name of input file currently processing.
**	line_cnt	- current line number in input file.
**
** Outputs:
**	buf		- statement buffer. may write EOS at new position here.
**
**	Returns:
**		VOID
** History:
**	18-jun-1992 (pete)
**	    Initial version.
*/
static char *
stripbuf(buf, expr_loc, stmt, infn, line_cnt)
char		*buf;
i4		expr_loc;
FG_STATEMENT	*stmt;
char		*infn;
i4		line_cnt;
{
	if (stmt == (FG_STATEMENT *)NULL)	/* unknown statement type */
	    return buf;

	if (stmt->expr_begin != FG_NOWORD)
	{
	    /* statement may have an expression in it */
	    if (expr_loc != FG_EXPR_LOC_NOTSET)
	    {
		/* move to beginning of expression */
		buf += expr_loc;
	    }
	    else
	    {
		/* expression word never encountered. make buf an empty string*/
		buf += STlength(buf);
	    }
	}

	if (stmt->strip_last_word != (char *)NULL)
	{
	    /* This statement has a trailing word that must be stripped
	    ** before evaluating the statement. (at this time, only IF
	    ** has such a word: "THEN"). The trailing word is optional.
	    */
	    if (striplastword(buf, stmt->strip_last_word) != OK)
	    {
#if 0
		made "THEN" no longer required on IF stmts. (pete)
	    	IIFGerr(E_FG0067_Syntax_IF_WHILE, FGFAIL,
			4, (PTR)stmt->stmt, (PTR)&line_cnt, (PTR)infn,
			(PTR)stmt->strip_last_word);
	    	/*NOTREACHED*/
#endif
	    }
	}

	return buf;
}

/*{
** Name:	IIFGbseBoolStmtEval - evaluate a boolean condition.
**
** Description:
**	Evaluate a string that contains a boolean condition. This is intended
**	to work with the code generation products, for example Vision, but could
**	be adapted for other uses.
**
** Inputs:
**	stmtbuf		- char string buffer with condition to be evaluated.
**	infn		- name of input file currently processing.
**	line_cnt	- current line number in input file.
** Outputs:
**
**	Returns:
**		What condition evaluates to: TRUE or FALSE.
**
**	Exceptions:
**		IIUGrefRegisterExpFns() must be called first, before
**		this procedure, so procedures called by IIUGeceEvalCharExpr()
**		here-in will be known.
**
** Side Effects:
**
** History:
**	2-mar-1992 (pete)
**	    Initial Version.
*/
bool
IIFGbseBoolStmtEval(stmtbuf, infn, line_cnt)
char	*stmtbuf;
char	*infn;
i4	line_cnt;
{
	char *res;
	bool ret;

	/* ASSUMPTION: this routine only called for IF and WHILE statements */

	if ((res = IIUGeceEvalCharExpr(stmtbuf)) == (char *)NULL)
	{
	    /* error evaluating expression */
	    IIFGerr(E_FG0069_Bad_Expression, FGFAIL,
		2, (PTR)&line_cnt, (PTR)infn);
	    /*NOTREACHED*/
	    ret = FALSE;
	}
	else
	{
	    /* "0" is FALSE; everything else considered TRUE */
	    ret = (CMcmpcase(res, ERx("0")) != 0);
	}

	return ret;
}

/*{
** Name:	striplastword - strip the specified word from a buffer
**
** Description:
**	Strip the specified word from a buffer if it is the last word
**	in the buffer. Only the last-word occurance of it will be stripped.
**
** Inputs:
**	buf	- the buffer that needs to have last word stripped in.
**	word	- strip this word if it's the last one.
**
** Outputs:
**
**	Returns:
**		OK if all went ok, FAIL if couldn't find the specified
**		word as the last one.
**
** History:
**	5-mar-1992 (pete)
**	    Initial Version.
*/
static STATUS
striplastword(buf, word)
char *buf;
char *word;
{
	char *p_buf = buf;
	i4 b_len, w_len;

	if ((word == NULL) || (buf == NULL) || (*buf == EOS))
	    goto end;

	STtrmwhite(p_buf);      /* get rid of trailing '\n' */
	b_len = STlength(p_buf);
	w_len = STlength(word);

	/* point to beginning of "word" at end of string */
	if (w_len > b_len)
	    goto end;

	p_buf = p_buf + b_len - w_len;

	/* make sure we're synced on beginning of character in p_buf */
	CMprev(p_buf, buf);
	CMnext(p_buf);

	if (STbcompare(p_buf,0, word,0, TRUE) == 0) {
	    *p_buf = EOS;
	    STtrmwhite(buf);
	    return OK;
	}

end:
	return FAIL;
}
