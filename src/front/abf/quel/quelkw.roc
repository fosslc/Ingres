/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<iicommon.h>
#include	<oslconf.h>
#include	<oslkw.h>
#include	<oskeyw.h>
#include	"quel.h"

/**
** Name:    quelkw.roc -	OSL/QUEL Parser Keyword Tables.
**
** Description:
**	Contains data tables used to map keywords to tokens as well as other DML
**	dependent data for the OSL/QUEL parser.  Defines:
**
**	osldml		OSL DML constant.
**	osQuote		OSL DML quote character.
**	kwtab		OSL/QUEL keyword table.
**	kwtab_size	size of keyword table (no. of entries.)
**	osscankw	OSL/QUEL token map table.
**
** History:
**	Revision 4.0  85/07/29  11:11:56  joe
**	base version  (85/05  joe)
**
**	Revision 4.1  85/10/10  11:31:58  arthur
**	Integration of work on expressions, argument passing....
**	Added new keyword 'printscreen'.
**	Added keywords 'key,' 'not' and 'byref'.
**	Changes 'message,' 'prompt' and 'helpfile' from OSBUFFER to
**	OSDONTCARE.
**	Added 'while,' 'do' and 'endwhile' to keyword table.
**	Changed 'kwscanmode' in keyword table to OSBUFFER for 'if' statements.
**	Change 'kwscanmode' element of keyword table to 'OSBUFFER'
**	so that output for quel-like statements is buffered up.
**
**	Revision 5.1  86/11/04  14:55:35  wong
**	Added double keywords.  Added size of 'kwtab'.
**	Sorted and added keyword/operator string definitions.
**
**	Revision 6.0  87/06  wong
**	Added keywords for NULL support and pattern-matching (LIKE).
**	Removed DBMS keywords not needed to be recognized by the parser.
**	Moved keyword/operator strings to "osl/oskeyw.roc".
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added double keyword PROCEDURE RETURNING.
**
**	Revision 6.4/02
**	09/30/91 (emerson)
**		Added DECLARE keyword (bug 39722).
**	09/30/91 (emerson)
**		Added DISPLAY SUBMENU and RUN SUBMENU double keywords
**		(bug 40086).
**
**	Revision 6.5
**	11-feb-93 (davel)
**		Added tokens for INQUIRE_4GL and SET_4GL, and added OFF.
**	08-mar-93 (davel)
**		Part of fix for bug 49878 - add ON DBEVENT as double keyword.
**	15-mar-93 (davel)
**		Add SET CONNECTION as a double keyword.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	28-nov-95 (abowler)
**		Fix for 70504: Added DCONST to osscankw token map as per SQL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Add specialised SET command for RANDOM_SEED.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-Aug-2009 (kschendel) 121804
**	    Need iicommon.h to satisfy gcc 4.3.
**/

/*
** Name:    osldml -	OSL Data Manipulation Language Constant.
**
** Description:
**	Defines the data manipulation language for the OSL/QUEL
**	translator parser.
*/

GLOBALCONSTDEF i4	osldml = OSLQUEL;

/*
** Name:    osQuote -	OSL Data Manipulation Language Quote String.
**
** Description:
**	Defines the quote character (in a string) that is to be used
**	for the OSL/QUEL translator parser.
*/

GLOBALCONSTDEF char	osQuote[] = ERx("\"");

/*
** Name:    kwtab -		OSL/QUEL Keyword Table.
**
** Description:
**	Contains the keywords recognized by OSL/QUEL.  This table (and its
**	double keyword sub-tables) must be sorted in alphabetically ascending
**	order on the keyword name field so that a binary search algorithm can be
**	used on them.  They also must be NULL-terminated for linear search.
*/

/* DEFINE Double Keyword Table

static const KW	defktab[] = {
	{_Integrity,	INTEGRITY,	TRUE,	NULL},
	{_Permit,	PERMIT,		TRUE,	NULL},
	{_View,		VIEW,		TRUE,	NULL},
	NULL
};
*/

/* DESTROY Double Keyword Tables

static const KW	desktab[] = {
	{_Integrity,	DESTROY_INTEG,	TRUE,	NULL},
	{_Permit,	DESTROY_PERMIT,	TRUE,	NULL},
	{_View,		DESTROY_VIEW,	TRUE,	NULL},
	NULL
};
*/


/* DIRECT Double Keyword Table */

static const KW	dirktab[] = {	/* DDB */
	{ ERx("connect"),	DIR_CONNECT,	TRUE,	NULL},
	{ ERx("disconnect"),	DIR_DISCONNECT,	TRUE,	NULL},
	{ ERx("execute"),	DIR_EXECUTE,	TRUE,	NULL},
	NULL
};

/* TRANSACTION Double Keyword Tables */

static const char	_Transaction[] =	ERx("transaction");

static const KW	begktab[] = {
	{_Transaction,	BEGINTRANSACTION, TRUE,	NULL},
	NULL
};

static const KW	endktab[] = {
	{_Transaction,	ENDTRANSACTION,	TRUE,	NULL},
	NULL
};

/* NOT Double Keyword Tables */

static const KW	notktab[] = {
	{_Like,		NOTLIKE,	TRUE,	NULL},
	NULL
};

/* BEFORE | AFTER FIELD Double Keyword Tables */

static const char	_Field[] =	ERx("field");

static const KW	befktab[] = {
	{_Field,	FIELD_ENTRY,	FALSE,	NULL},
	NULL
};

static const KW	aftktab[] = {
	{_Field,	FIELD_EXIT,	FALSE,	NULL},
	NULL
};

/* DISPLAY SUBMENU */
static const KW	dispktab[] = {
	{ERx("submenu"),DISPLAY_MENU,	FALSE,	NULL},
	NULL
};

/* RUN SUBMENU */
static const KW	runktab[] = {
	{ERx("submenu"),RUN_MENU,	FALSE,	NULL},
	NULL
};

/* RESUME Double Keyword Table */
static const KW	resktab[] = 
{
    {_Entry,			RESENTRY,	TRUE,	NULL},
    {_iiNext,			RESNEXT,	TRUE,	NULL},
    {_Menu,			RESMENU,	TRUE,	NULL},
    {ERx("nextfield"),		RESNFLD,	TRUE,	NULL},
    {ERx("previousfield"),	RESPFLD,	TRUE,	NULL},
    NULL
};

/* REGISTER Double Keyword Table */

static const KW	rgtktab[] = {
	{ ERx("table"),		REG_TABLE,	FALSE,	NULL},
	{ ERx("view"),		REG_VIEW,	FALSE,	NULL},
	NULL
};

/* REMOVE Double Keyword Table */

static const KW	rmtktab[] = {
	{ ERx("table"),		REM_TABLE,	FALSE,	NULL},
	{ ERx("view"),		REM_VIEW,	FALSE,	NULL},
	NULL
};

/* PROCEDURE Double Keyword Table */

static const KW 	procktab[] = {
	{ERx("returning"), PROCEDURE_RETURNING,	FALSE,	NULL},
	NULL
};

/* ON Double Keyword Table */
static const KW	onktab[] = {
	{ERx("dbevent"),      ON_DBEVENT,    FALSE,  NULL},
	NULL
};

/* SET Double Keyword Table */
static const KW	setktab[] = {
	{ERx("connection"),	SET_CONNECTION, FALSE,  NULL},
	{ERx("random_seed"),	SET_RANDOM_SEED,FALSE,  NULL},
	NULL
};

/* General OSL Keywords */

GLOBALCONSTDEF KW	kwtab[] = {
	{ERx("abort"),		ABORT,		TRUE,	NULL},
	{ERx("after"),		0,		FALSE,	aftktab},
	{_All,			ALL,		TRUE,	NULL},
	{_And,			AND,		TRUE,	NULL},
	{ERx("append"),		APPEND,		TRUE,	NULL},
	{ERx("as"),		AS,		TRUE,	NULL},
	{ERx("at"),		AT,		TRUE,	NULL},
	{ERx("before"),		0,		FALSE,	befktab},
	{ERx("begin"),		BEGIN,		FALSE,	begktab},
	{ERx("by"),		BY,		TRUE,	NULL},
	{ERx("byref"),		BYREF,		FALSE,	NULL},
	{ERx("call"),		CALL,		FALSE,	NULL},
	{ERx("callframe"),	CALLF,		FALSE,	NULL},
	{ERx("callproc"),	CALLP,		FALSE,	NULL},
	{ERx("clear"),		CLEAR,		FALSE,	NULL},
	{ERx("clearrow"),	CLEARROW,	FALSE,	NULL},
	{ERx("copy"),		COPY,		TRUE,	NULL},
	{ERx("create"),		CREATE,		TRUE,	NULL},
	{ERx("declare"),	DECLARE,	FALSE,	NULL},
	{ERx("default"),	DEFAULT,	TRUE,	NULL},
	{ERx("define"),		DEFINE,		TRUE,	NULL},
	{ERx("delete"),		DELETE,		TRUE,	NULL},
	{ERx("deleterow"),	DELETEROW,	FALSE,	NULL},
	{ERx("destroy"),	DESTROY,	TRUE,	NULL},
	{ERx("direct"),		0,		TRUE,	dirktab}, /* DDB */
	{ERx("display"),	0,		FALSE,	dispktab},
	{ERx("do"),		DO,		FALSE,	NULL},
	{ERx("else"),		ELSE,		FALSE,	NULL},
	{ERx("elseif"),		ELSEIF,		FALSE,	NULL},
	{ERx("end"),		END,		TRUE,	endktab},
	{ERx("endif"),		ENDIF,		FALSE,	NULL},
	{ERx("endloop"),	ENDLOOP,	FALSE,	NULL},
	{ERx("endwhile"),	ENDWHILE,	FALSE,	NULL},
	{ERx("exit"),		EXIT,		FALSE,	NULL},
	{_Field,		FIELD,		FALSE,	NULL},
	{ERx("from"),		FROM,		TRUE,	NULL},
	{ERx("help_forms"),	HELP_FORMS,	FALSE,	NULL},
	{ERx("helpfile"),	HELPFILE,	FALSE,	NULL},
	{ERx("if"),		IF,		FALSE,	NULL},
	{ERx("immediate"),	IMMEDIATE,	TRUE,	NULL},
	{ERx("index"),		INDEX,		TRUE,	NULL},
	{ERx("initialize"),	INITIALIZE,	FALSE,	NULL},
	{ERx("inittable"),	INITTABLE,	FALSE,	NULL},
	{ERx("inquire_4gl"),	INQ_4GL,	FALSE,	NULL},
	{ERx("inquire_forms"),	INQ_FORMS,	FALSE,	NULL},
	{ERx("inquire_ingres"), INQ_INGRES,	FALSE,	NULL},
	{ERx("insertrow"),	INSERTROW,	FALSE,	NULL},
	{ERx("integrity"),	INTEGRITY,	TRUE,	NULL},
	{ERx("into"),		INTO,		TRUE,	NULL},
	{ERx("is"),		IS,		TRUE,	NULL},
	{ERx("key"),		KEY,		FALSE,	NULL},
	{_Like,			LIKE,		FALSE,	NULL},
	{ERx("loadtable"),	LOADTABLE,	FALSE,	NULL},
	{ERx("message"),	MESSAGE,	FALSE,	NULL},
	{ERx("mode"),		MODE,		FALSE,	NULL},
	{ERx("modify"),		MODIFY,		TRUE,	NULL},
	{_iiNext,		NEXT,		FALSE,	NULL},
	{ERx("noecho"),		NOECHO,		FALSE,	NULL},
	{_Not,			NOT,		TRUE,	notktab},
	{ERx("null"),		NULLK,		TRUE,	NULL},
	{ERx("of"),		OF,		TRUE,	NULL},
	{ERx("off"),		OFF,		TRUE,	NULL},
	{ERx("on"),		ON,		TRUE,	onktab},
	{ERx("only"),		ONLY,		TRUE,	NULL},
	{_Or,			OR,		TRUE,	NULL},
	{ERx("order"),		ORDER,		TRUE,	NULL},
	{ERx("permit"),		PERMIT,		TRUE,	NULL},
	{ERx("printscreen"),	PRINTSCREEN,	FALSE,	NULL},
	{ERx("procedure"),	PROCEDURE,	FALSE,	procktab},
	{ERx("prompt"),		PROMPT,		FALSE,	NULL},
	{ERx("purgetable"),	PURGETBL,	FALSE,	NULL},
	{ERx("qualification"),	QUALIFICATION,	FALSE,	NULL},
	{ERx("range"),		RANGE,		TRUE,	NULL},
	{ERx("redisplay"),	REDISPLAY,	FALSE,	NULL},
	{ERx("register"),	REGISTER,	FALSE,	rgtktab}, /* DDB */
	{ERx("relocate"),	RELOCATE,	TRUE,	NULL},	  /* DDB */
	{ERx("remove"),		REMOVE,		FALSE,	rmtktab},
	{ERx("repeat"),		REPEAT,		TRUE,	NULL},
	{ERx("replace"),	REPLACE,	TRUE,	NULL},
	{ERx("resume"),		RESUME,		FALSE,	resktab},
	{ERx("retrieve"),	RETRIEVE,	TRUE,	NULL},
	{ERx("return"),		RETURN,		FALSE,	NULL},
	{ERx("run"),		0,		FALSE,	runktab},
	{ERx("save"),		SAVE,		TRUE,	NULL},
	{ERx("savepoint"),	SAVEPOINT,	TRUE,	NULL},
	{ERx("screen"),		SCREEN,		FALSE,	NULL},
	{ERx("scroll"),		SCROLL,		FALSE,	NULL},
	{ERx("set"),		SET,		TRUE,	setktab},
	{ERx("set_4gl"),	SET_4GL,	FALSE,	NULL},
	{ERx("set_forms"),	SET_FORMS,	FALSE,	NULL},
	{ERx("set_ingres"),	SET_INGRES,	TRUE,	NULL},
	{ERx("sleep"),		SLEEP,		FALSE,	NULL},
	{ERx("sort"),		SORT,		TRUE,	NULL},
	{ERx("system"),		SYSTEM,		FALSE,	NULL},
	{ERx("then"),		THEN,		FALSE,	NULL},
	{ERx("to"),		TO,		TRUE,	NULL},
	{ERx("unique"),		UNIQUE,		TRUE,	NULL},
	{ERx("unloadtable"),	UNLOADTABLE,	FALSE,	NULL},
	{ERx("until"),		UNTIL,		TRUE,	NULL},
	{ERx("validate"),	VALIDATE,	FALSE,	NULL},
	{ERx("validrow"),	VALIDROW,	FALSE,	NULL},
	{ERx("view"),		VIEW,		TRUE,	NULL},
	{ERx("where"),		WHERE,		TRUE,	NULL},
	{ERx("while"),		WHILE,		FALSE,	NULL},
	{ERx("with"),		WITH,		TRUE,	NULL},
	NULL
};

GLOBALCONSTDEF i4	kwtab_size = sizeof(kwtab)/sizeof(KW) - 1;

/*
** Name:    osscankw -	OSL/QUEL Token Map.
**
** Description:
**	This table is used by the scanner to map from some constants
**	declared in "oslkw.h" to actual token values.  This permits the
**	two different grammars for SQL and QUEL to have different values
**	for the same token.  Although the two tables are identical they
**	need to be in separate files since the values for the tokens are
**	different in the two places.
*/

GLOBALCONSTDEF
    i4	osscankw[] = {
     /*  Values 	   Constant */

	ID, 		/* OSIDENT */
	ICONST, 	/* OSICONST */
	FCONST, 	/* OSFCONST */
	SCONST, 	/* OSSCONST */
	DEREFERENCE, 	/* OSDEREFERENCE */
	COLEQ, 		/* OSCOLEQ */
	NOTEQ, 		/* OSNOTEQ */
	LTE, 		/* OSLTE */
	GTE, 		/* OSGTE */
	EXP, 		/* OSEXP */
	COLID,		/* OSCOLID */
	LSQBRK,		/* OSLSQBRK */
	RSQBRK,		/* OSRSQBRK */
	XCONST,		/* OSXCONST */
	OWNER_INGRES,	/* OSINGRES */
	BEGIN,		/* OSBEGIN */
	END,		/* OSEND */
	DCONST          /* OSDCONST */
};
