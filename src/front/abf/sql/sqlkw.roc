/*
** Copyright (c) 1985, 2008, 2009 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
#include	<iicommon.h>
#include	<oslconf.h>
#include	<oslkw.h>
#include	<oskeyw.h>
#include	"sql.h"

/**
** Name:    sqlkw.roc -	OSL/SQL Parser Keyword Tables.
**
** Description:
**	Contains data tables used to map keywords to tokens as well as other DML
**	dependent data for the OSL/SQL parser.  Defines:
**
**	osldml		OSL DML constant.
**	osQuote		OSL DML quote character.
**	kwtab		OSL/SQL keyword table.
**	kwtab_size	size of keyword table (no. of entries.)
**	osscankw	OSL/SQL token map table.
**
** History:
**	Revision 4.0  86/01/06 joe
**	Initial version.  85/06  joe
**
**	Revision 6.0  87/06  wong
**	Added DML configuration constants.  Added hexadecimal string constant
**	token.  Added new 6.0 SQL keywords.  Modified to use double keywords
**	and keyword/operator strings.
**
**	Revision 6.4
**	03/12/91 (emerson)
**		Put the entries for crktab, drpktab, callktab, and resktab
**		into collating sequence.  (No reported bug, but the comments
**		say they're searched by a binary search).
**		Split the CALL/EXECUTE double keyword table into two separate
**		tables and added the EXECUTE IMMEDIATE double keyword.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added double keyword PROCEDURE RETURNING.
**	04/22/91 (emerson)
**		Added 6 new double keywords (CREATE EVENT, DROP EVENT,
**		REGISTER EVENT, REMOVE EVENT, RAISE EVENT and GET EVENT)
**		for alerter support.
**	07/26/91 (emerson)
**		Change EVENT to DBEVENT (per LRC 7-3-91).
**
**	Revision 6.5 	92/04	davel
**	29-apr-92 (davel)
**		Removed TO and FROM double keywords (not needed).
**		Standardized the names of double keywords for DROP,
**		CREATE, and REMOVE.
**	25-aug-92 (davel)
**		Change token for EXECUTE PROCEDURE from CALLP to EXE_PROC.
**	03-sep-92 (davel)
**		Added tokens for Outer Join support. Also added ON COMMIT
**		as a double keyword to prevent shift/reduce conflict with
**		outer join ON <DMLcondition> clause.
**	10-sep-92 (davel)
**		Added tokens for Table Constraints/Defaults support.
**	14-sep-92 (davel)
**		Added tokens for Table and Column Comments support and 
**		security_alaram and security_audit support. Also added USER
**		as a new keyword.
**	24-sep-92 (davel)
**		Added tokens for multi-session support.
**	10-feb-93 (davel)
**		Added tokens for INQUIRE_4GL and SET_4GL, and for OFF.
**	09-mar-93 (davel)
**		Added double keyword SET CONNECTION.
**	24-mar-93 (davel)
**		Removed double keyword EXECUTE_ON (part of fix for bug 50675).
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	19-nov-93 (robf)
**               Add support for secure 2.0 changes:
**		 - CREATE/ALTER/DROP PROFILE double keywords
**		 - DBMS PASSWORD double keyword
**	9-dec-93 (robf)
**               Make DBMS_PASSWORD single keyword per LRC
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Add specialised SET command for RANDOM_SEED.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	07-Jun-2009 (kiria01) b122185
**	    Add CASE/IF expression support for SQL
**	26-Aug-2009 (kschendel) 121804
**	    Need iicommon.h to satisfy gcc 4.3.
*/

/*
** Name:    osldml -	OSL Data Manipulation Language Constant.
**
** Description:
**	Defines the data manipulation language for the OSL/SQL
**	translator parser.
*/

GLOBALCONSTDEF i4	osldml = OSLSQL;

/*
** Name:    osQuote -	OSL Data Manipulation Language Quote String.
**
** Description:
**	Defines the quote character (in a string) that is to be used
**	for the OSL/SQL translator parser.
*/

GLOBALCONSTDEF char	osQuote[] = ERx("'");

/*
** Name:    kwtab -		OSL/SQL Keyword Table.
**
** Description:
**	Contains the keywords recognized by OSL/SQL.  This table (and its
**	double keyword sub-tables) must be sorted in alphabetically ascending
**	order on the keyword name field so that a binary search algorithm can be
**	used on them.  They also must be NULL-terminated for linear search.
*/
static const char	s_Add[]		= ERx("add");		/* RTI/SQL */
static const char	s_All[]		= ERx("all");		/* RTI/SQL */
static const char	s_And[]		= ERx("and");		/* RTI/SQL */
static const char	s_As[]		= ERx("as");		/* RTI/SQL */
static const char	s_By[]		= ERx("by");		/* RTI/SQL */
static const char	s_Commit[]	= ERx("commit");	/* RTI/SQL */
static const char	s_Connect[]	= ERx("connect");	/* RTI/SQL */
static const char	s_Current[]	= ERx("current");	/* RTI/SQL */
static const char	s_Dbevent[]	= ERx("dbevent");	/* RTI/SQL */
static const char	s_Dbms_Passwd[]	= ERx("dbms_password"); /* RTI/SQL */
static const char	s_Default[]	= ERx("default");	/* RTI/SQL */
static const char	s_Disconnect[]	= ERx("disconnect");	/* RTI/SQL */
static const char	s_Entry[]	= ERx("entry");
static const char	s_Escape[]	= ERx("escape");
static const char	s_Execute[]	= ERx("execute");
static const char	s_Group[]	= ERx("group");		/* RTI/SQL */
static const char	s_Integrity[]	= ERx("integrity");	/* RTI/SQL */
static const char	s_Join[]	= ERx("join");		/* RTI/SQL */
static const char	s_Key[]		= ERx("key");		/* RTI/SQL */
static const char	s_Like[]	= ERx("like");		/* RTI/SQL */
static const char	s_Link[]	= ERx("link");		/* RTI/SQL */
static const char	s_Location[]	= ERx("location");	/* RTI/SQL */
static const char	s_Menu[]	= ERx("menu");		/* RTI/SQL */
static const char	s_Next[]	= ERx("next");		/* RTI/SQL */
static const char	s_Nextfield[]	= ERx("nextfield");
static const char	s_Not[]		= ERx("not");		/* RTI/SQL */
static const char	s_On[]		= ERx("on");		/* RTI/SQL */
static const char	s_Or[]		= ERx("or");		/* RTI/SQL */
static const char	s_Password[]	= ERx("password");	/* RTI/SQL */
static const char	s_Permit[]	= ERx("permit");	/* RTI/SQL */
static const char	s_Prevfield[]	= ERx("previousfield");
static const char	s_Privileges[]	= ERx("privileges");	/* RTI/SQL */
static const char	s_Procedure[]	= ERx("procedure");	/* RTI/SQL */
static const char	s_Profile[]	= ERx("profile");	/* RTI/SQL */
static const char	s_Public[]	= ERx("public");	/* RTI/SQL */
static const char	s_Role[]	= ERx("role");		/* RTI/SQL */
static const char	s_Rule[]	= ERx("rule");		/* RTI/SQL */
static const char	s_SecAlarm[]	= ERx("security_alarm"); /* RTI/SQL */
static const char	s_SecAudit[]	= ERx("security_audit"); /* RTI/SQL */
static const char	s_Session[]	= ERx("session"); 	/* RTI/SQL */
static const char	s_Synonym[]	= ERx("synonym");	/* RTI/SQL */
static const char	s_Table[]	= ERx("table");		/* RTI/SQL */
static const char	s_To[]		= ERx("to");		/* RTI/SQL */
static const char	s_Users[]	= ERx("users");		/* RTI/SQL */
static const char	s_User[]	= ERx("user");		/* RTI/SQL */
static const char	s_View[]	= ERx("view");

/* CREATE Double Keyword Table */
static const KW	crktab[] = {
	{s_Dbevent,	CRE_DBEVENT,	FALSE,	NULL},
	{s_Group,	CRE_GROUP,	TRUE,	NULL},
	{s_Integrity,	CRE_INTEGRITY,	TRUE,	NULL},
	{s_Location,	CRE_LOCATION,	TRUE,	NULL},
	{s_Permit,	CRE_PERMIT,	TRUE,	NULL},
	{s_Procedure,	PROCEDURE,	TRUE,	NULL},
	{s_Profile,	CRE_PROFILE,	TRUE,   NULL},
	{s_Role,	CRE_ROLE,	TRUE,	NULL},
	{s_Rule,	CRE_RULE,	TRUE,	NULL},
	{s_SecAlarm,	CRE_SECALARM,	TRUE,	NULL},
	{s_Synonym,	CRE_SYNONYM,	TRUE,	NULL},
	{s_User,	CRE_USER,	TRUE,	NULL},
	{s_View,	CRE_VIEW,	TRUE,	NULL},
	NULL
};


/* ADD Double Keyword Table */

static KW	addtab[] = {
	{s_Privileges,	ADD_PRIVILEGES,	TRUE,	NULL},
	NULL
};
/* CURRENT Double Keyword Table */

static KW	curtab[] = {
	{ERx("installation"),	CURRENT_INST,	TRUE,	NULL},
	NULL
};


/* DROP Double Keyword Table */
static const KW	drpktab[] = {
	{s_Dbevent,	DRP_DBEVENT,	FALSE,	NULL},
	{s_Group,	DRP_GROUP,	TRUE,	NULL},
	{s_Integrity,	DRP_INTEGRITY,	TRUE,	NULL},
	{s_Link,	DRP_LINK,	TRUE,	NULL},
	{s_Location,	DRP_LOCATION,	TRUE,	NULL},
	{s_Permit,	DRP_PERMIT,	TRUE,	NULL},
	{s_Privileges,	DROP_PRIVILEGES,TRUE,	NULL},
	{s_Procedure,	DRP_PROC,	TRUE,	NULL},
	{s_Profile,	DRP_PROFILE,	TRUE,	NULL},
	{s_Role,	DRP_ROLE,	TRUE,	NULL},
	{s_Rule,	DRP_RULE,	TRUE,	NULL},
	{s_SecAlarm,	DRP_SECALARM,	TRUE,	NULL},
	{s_Synonym,	DRP_SYNONYM,	TRUE,	NULL},
	{s_User,	DRP_USER,	TRUE,	NULL},
	{s_View,	DRP_VIEW,	TRUE,	NULL},
	NULL
};

/* DIRECT CONNECT/DISCONNECT Double Keyword Table */
static const KW	dirktab[] = {
	{ s_Connect,	DIR_CONNECT,	TRUE,	NULL},
	{ s_Disconnect,	DIR_DISCONNECT,	TRUE,	NULL},
	{ s_Execute,	DIR_EXECUTE,	TRUE,	NULL},
	NULL
};

/* CALL Double Keyword Table */
static const KW	callktab[] = {
	{s_Procedure,		CALLP,		TRUE,	NULL},
	NULL
};

/* EXECUTE Double Keyword Table */
static const KW	execktab[] = {
	{ERx("immediate"), 	EXECUTE_IMMED,	FALSE,	NULL},
	{s_Procedure,		EXE_PROC,	TRUE,	NULL},
	NULL
};

/* GET Double Keyword Table */
static const KW	getktab[] = 
{
	{s_Dbevent,	GET_DBEVENT,	FALSE,	NULL},
	NULL
};

/* RAISE Double Keyword Table */
static const KW	rasktab[] = 
{
	{s_Dbevent,	RAISE_DBEVENT,	FALSE,	NULL},
	NULL
};

/* RESUME Double Keyword Table */
static const KW	resktab[] = 
{
	{s_Entry,	RESENTRY,	TRUE,	NULL},
	{s_Menu,	RESMENU,	TRUE,	NULL},
	{s_Next,	RESNEXT,	TRUE,	NULL},
	{s_Nextfield,	RESNFLD,	TRUE,	NULL},
	{s_Prevfield,	RESPFLD,	TRUE,	NULL},
	NULL
};

/* REGISTER/REMOVE Double Keyword Tables */
static const KW	regktab[] = {
	{s_Dbevent,		REGISTER_DBEVENT,	FALSE,	NULL},
	{s_View,		REG_VIEW,		TRUE,	NULL},
	NULL
};
static const KW	remktab[] = {
	{s_Dbevent,		REMOVE_DBEVENT,	FALSE,	NULL},
	{s_View,		REM_VIEW,	TRUE,	NULL},
	NULL
};

/* TRANSACTION Double Keyword Tables */
static const char	s_Transaction[] =	ERx("transaction");
static const KW	begktab[] = {
	{s_Transaction,	BEGINTRANSACTION, TRUE,	NULL},
	NULL
};

static const KW	endktab[] = {
	{s_Transaction,	ENDTRANSACTION,	TRUE,	NULL},
	NULL
};

/* COPY Double Keyword Table */
static const KW	cpktab[] = {
	{s_Table,	COPY,		TRUE,	NULL},
	NULL
};

/* MODIFY Double Keyword Table */
static const KW	modktab[] = {
	{s_Table,	MODIFY,		TRUE,	NULL},
	NULL
};

/* NOT Double Keyword Table */
static const KW	notktab[] = {
	{s_Like,		NOTLIKE,	TRUE,	NULL},
	NULL
};

/* BEFORE | AFTER FIELD Double Keyword Tables */
static const char	s_Field[] =	ERx("field");

static const KW	befktab[] = {
	{s_Field,	FIELD_ENTRY,	FALSE,	NULL},
	NULL
};
static const KW	aftktab[] = {
	{s_Field,	FIELD_EXIT,	FALSE,	NULL},
	NULL
};

/* DISPLAY MENU */
static const KW	dispktab[] = {
	{ERx("submenu"), DISPLAY_MENU,	FALSE,	NULL},
	NULL
};

/* RUN MENU */
static const KW	runktab[] = {
	{ERx("submenu"), RUN_MENU,	FALSE,	NULL},
	NULL
};

/* ARRAY OF */
static const KW	arrayktab[] = {
	{ERx("of"),	ARRAY_OF,	FALSE,	NULL},
	NULL
};

/* CLASS OF */
static const KW	classktab[] = {
	{ERx("of"),	CLASS_OF,	FALSE,	NULL},
	NULL
};

/* PROCEDURE RETURNING */
static const KW 	procktab[] = {
	{ERx("returning"), PROCEDURE_RETURNING,	FALSE,	NULL},
	NULL
};

/* TERMINATOR I double keywords */

static const KW 	altktab[] = {
	{s_Default,	ALTER_DEFAULT,	TRUE,	NULL},
	{s_Group,	ALTER_GROUP,	TRUE,	NULL},
	{s_Location,	ALTER_LOCATION,	TRUE,	NULL},
	{s_Profile,	ALTER_PROFILE,	TRUE,	NULL},
	{s_Role,	ALTER_ROLE,	TRUE,	NULL},
	{s_SecAudit,	ALTER_SECAUDIT,	TRUE,	NULL},
	{s_Table,	ALTER_TABLE,	TRUE,	NULL},
	{s_User,	ALTER_USER,	TRUE,	NULL},
	NULL
};

static const KW 	onktab[] = {
	{ERx("commit"), ON_COMMIT,	FALSE,	NULL},
	{ERx("database"), ON_DATABASE,	FALSE,	NULL},
	{s_Dbevent,	 ON_DBEVENT,	FALSE,	NULL},
	{s_Location,	 ON_LOCATION,	FALSE,	NULL},
	NULL
};

/* 6.5 double keywords */

/* USER AUTHORIZATION */
static const KW 	userktab[] = {
	{ERx("authorization"), USER_AUTH,	TRUE,	NULL},
	NULL
};

/* SYSTEM USER */
static const KW 	systemktab[] = {
	{s_User, 	SYSTEM_USER,	TRUE,	NULL},
	NULL
};

/* TO/FROM for GRANT/REVOKE */
static const KW 	fromktab[] = {
	{s_Group, 	FROM_GROUP,	TRUE,	NULL},
	{s_Role, 	FROM_ROLE,	TRUE,	NULL},
	{s_User, 	FROM_USER,	TRUE,	NULL},
	NULL
};
static const KW 	toktab[] = {
	{s_Group, 	TO_GROUP,	TRUE,	NULL},
	{s_Role, 	TO_ROLE,	TRUE,	NULL},
	{s_User, 	TO_USER,	TRUE,	NULL},
	NULL
};

/* SESSION USER/GROUP/ROLE */
static const KW 	sessionktab[] = {
	{s_Group, 	SESSION_GROUP,	TRUE,	NULL},
	{s_Role, 	SESSION_ROLE,	TRUE,	NULL},
	{s_User, 	SESSION_USER,	TRUE,	NULL},
	NULL
};

/* GLOBAL TEMPORARY */
static const KW 	globalktab[] = {
	{ERx("temporary"), 	GLOB_TEMP,	TRUE,	NULL},
	NULL
};

/* Outer Join double keywords: */
static const KW 	innerktab[] = {
	{s_Join, INNER_JOIN,	TRUE,	NULL},
	NULL
};

static const KW 	fullktab[] = {
	{s_Join, FULL_JOIN,	TRUE,	NULL},
	NULL
};

static const KW 	leftktab[] = {
	{s_Join, LEFT_JOIN,	TRUE,	NULL},
	NULL
};

static const KW 	rightktab[] = {
	{s_Join, RIGHT_JOIN,	TRUE,	NULL},
	NULL
};

/* Table constraints/defaults */
static const KW      primaryktab[] = {
	{s_Key, PRIMARY_KEY,    TRUE,   NULL},
	NULL
};

static const KW      foreignktab[] = {
	{s_Key, FOREIGN_KEY,    TRUE,   NULL},
	NULL
};

/* Table and Column Comments */
static const KW 	commentktab[] = {
	{s_On,		COMMENT_ON,	FALSE,	NULL},
	NULL
};

/* Enable/Disable security_audit  */
static const KW 	enablektab[] = {
	{s_SecAudit,	ENABLE_SECAUDIT,	FALSE,	NULL},
	NULL
};
static const KW 	disablektab[] = {
	{s_SecAudit,	DISABLE_SECAUDIT,	FALSE,	NULL},
	NULL
};
static const KW 	byktab[] = {
	{s_Group, 	BY_GROUP,	FALSE,	NULL},
	{s_Role, 	BY_ROLE,	FALSE,	NULL},
	{s_User,	BY_USER,	FALSE,	NULL},
	NULL
};
/* Multi-session support */
static const KW 	connectktab[] = {
	{s_To,	CONNECT,	FALSE,	NULL},
	NULL
};
static const KW 	disconnectktab[] = {
	{s_Current,	DISCONNECT,		FALSE,	NULL},
	NULL
};
static const KW 	identktab[] = {
	{s_By,	IDENTIFIED_BY,	FALSE,	NULL},
	NULL
};
static const KW 	setktab[] = {
	{ERx("connection"),	SET_CONNECTION,	FALSE,	NULL},
	{ERx("random_seed"),	SET_RANDOM_SEED,FALSE,	NULL},
	NULL
};


/* General OSL Keywords */

GLOBALCONSTDEF KW	kwtab[] = {
	{ERx("abort"),		ABORT,		TRUE,	NULL},
	{s_Add,			ADD,		TRUE,	addtab},
	{ERx("after"),		0,		FALSE,	aftktab},
	{s_All,			ALL,		TRUE,	NULL},
	{ERx("alter"),		0,		FALSE,	altktab},
	{s_And,			AND,		TRUE,	NULL},
	{ERx("any"),		ANY,		TRUE,	NULL},
	{ERx("array"),		0,		TRUE,	arrayktab},
	{s_As,			AS,		TRUE,	NULL},
	{ERx("asc"),		ASC,		TRUE,	NULL},
	{ERx("at"),		AT,		TRUE,	NULL},
	{ERx("avg"),		AVG,		TRUE,	NULL},
	{ERx("before"),		0,		FALSE,	befktab},
	{ERx("begin"),		BEGIN,		TRUE,	begktab},
	{ERx("bell"),		BELL_TOK,	FALSE,	NULL},
	{ERx("between"),	BETWEEN,	TRUE,	NULL},
	{s_By,			BY,		TRUE,	byktab},
	{ERx("byref"),		BYREF,		FALSE,	NULL},
	{ERx("call"), 		CALL,		FALSE,	callktab},
	{ERx("callframe"),	CALLF,		FALSE,	NULL},
	{ERx("callproc"),	CALLP,		FALSE,	NULL},
	{ERx("case"),		CASE,		FALSE,	NULL},
	{ERx("check"),		CHECK,		FALSE,	NULL},
	{ERx("clear"),		CLEAR,		FALSE,	NULL},
	{ERx("clearrow"),	CLEARROW,	FALSE,	NULL},
	{ERx("comment"),	0,		FALSE,	commentktab},
	{s_Commit,		COMMIT,		TRUE,	NULL},
	{s_Connect,		CONNECT,	TRUE,	connectktab},
	{ERx("constraint"),	CONSTRAINT,	FALSE,	NULL},
	{ERx("copy"),		COPY,		TRUE,	cpktab},/* RTI/SQL */
	{ERx("count"),		COUNT,		TRUE,	NULL},
	{ERx("create"),		CREATE,		TRUE,	crktab},
	{s_Current,		0,		TRUE,	curtab},
	{s_Dbms_Passwd,		DBMS_PASSWORD,	TRUE,	NULL},
	{ERx("declare"),	DECLARE,	FALSE,	NULL},
	{s_Default,		DEFAULT,	TRUE,	NULL},
	{ERx("delete"),		DELETE,		TRUE,	NULL},
	{ERx("deleterow"),	DELETEROW,	FALSE,	NULL},
	{ERx("desc"),		DESC,		TRUE,	NULL},
	{ERx("direct"),		0,		TRUE,	dirktab},
	{ERx("disable"),	0,		FALSE,	disablektab},
	{s_Disconnect,		DISCONNECT,	FALSE,	disconnectktab},
	{ERx("display"),	0,		FALSE,	dispktab},
	{ERx("distinct"),	DISTINCT,	TRUE,	NULL},
	{ERx("do"),		DO,		FALSE,	NULL},
	{ERx("drop"),		DROP,		TRUE,	drpktab},
	{ERx("else"),		ELSE,		FALSE,	NULL},
	{ERx("elseif"),		ELSEIF,		FALSE,	NULL},
	{ERx("enable"),		0,		FALSE,	enablektab},
	{ERx("end"),		END,		TRUE,	endktab},
	{ERx("endif"),		ENDIF,		FALSE,	NULL},
	{ERx("endloop"),	ENDLOOP,	FALSE,	NULL},
	{ERx("endwhile"),	ENDWHILE,	FALSE,	NULL},
	{s_Escape,		ESCAPE,		TRUE,	NULL},
	{s_Execute,		0,		TRUE,	execktab},
	{ERx("exists"),		EXISTS,		TRUE,	NULL},
	{ERx("exit"),		EXIT,		FALSE,	NULL},
	{s_Field,		FIELD,		FALSE,	NULL},
	{ERx("for"),		FOR,		TRUE,	NULL},
	{ERx("foreign"),	0,		TRUE,	foreignktab},
	{ERx("from"),		FROM,		TRUE,	fromktab},
	{ERx("full"),		0,		TRUE,	fullktab},
	{ERx("get"),		0,		FALSE,	getktab},
	{ERx("global"),		0,		TRUE,	globalktab},
	{ERx("grant"),		GRANT,		TRUE,	NULL},
	{ERx("group"),		GROUP,		TRUE,	NULL},
	{ERx("having"),		HAVING,		TRUE,	NULL},
	{ERx("help_forms"),	HELP_FORMS,	FALSE,	NULL},
	{ERx("helpfile"),	HELPFILE,	FALSE,	NULL},
	{ERx("identified"),	0,		FALSE,	identktab},
	{ERx("if"),		IF,		FALSE,	NULL},
	{ERx("immediate"), 	IMMEDIATE,	FALSE,	NULL},
	{ERx("in"),		IN,		TRUE,	NULL},
	{ERx("index"),		INDEX,		TRUE,	NULL},
	{ERx("initialize"), 	INITIALIZE,	FALSE,	NULL},
	{ERx("inittable"),	INITTABLE,	FALSE,	NULL},
	{ERx("inner"),		0,		TRUE,	innerktab},
	{ERx("inquire_4gl"),	INQ_4GL,	FALSE,	NULL},
	{ERx("inquire_forms"),	INQ_FORMS,	FALSE,	NULL},
	{ERx("inquire_ingres"), INQ_INGRES,	FALSE,	NULL},
	{ERx("inquire_sql"),	INQ_INGRES,	FALSE,	NULL},
	{ERx("insert"),		INSERT,		TRUE,	NULL},
	{ERx("insertrow"),	INSERTROW,	FALSE,	NULL},
	{ERx("into"),		INTO,		TRUE,	NULL},
	{ERx("is"),		IS,		TRUE,	NULL},
	{s_Join,		JOIN,		TRUE,	NULL},
	{s_Key,			KEY,		FALSE,	NULL},
	{ERx("left"),		0,		TRUE,	leftktab},
	{s_Like,		LIKE,		TRUE,	NULL},
	{ERx("loadtable"),	LOADTABLE,	FALSE,	NULL},
	{ERx("max"),		MAX,		TRUE,	NULL},
	{ERx("message"),	MESSAGE,	FALSE,	NULL},
	{ERx("min"),		MIN,		TRUE,	NULL},
	{ERx("mode"),		MODE,		FALSE,	NULL},
	{ERx("modify"),		MODIFY,		TRUE,	modktab}, /* RTI/SQL */
	{s_Next,		NEXT,		FALSE,	NULL},
	{ERx("noecho"),		NOECHO,		FALSE,	NULL},
	{s_Not,			NOT,		TRUE,	notktab},
	{ERx("null"),		NULLK,		TRUE,	NULL},
	{ERx("of"),		OF,		FALSE,	NULL},
	{ERx("off"),		OFF,		FALSE,	NULL},
	{s_On,			ON,		FALSE,	onktab},
	{s_Or,			OR,		TRUE,	NULL},
	{ERx("order"),		ORDER,		TRUE,	NULL},
	{ERx("primary"),	0,		TRUE,	primaryktab},
	{ERx("printscreen"),	PRINTSCREEN,	FALSE,	NULL},
	{s_Procedure,		PROCEDURE,	FALSE,	procktab},
	{ERx("prompt"),		PROMPT,		FALSE,	NULL},
	{ERx("purgetable"),	PURGETBL,	FALSE,	NULL},
	{ERx("qualification"),	QUALIFICATION,	FALSE,	NULL},
	{ERx("raise"),		0,		FALSE,	rasktab},
	{ERx("redisplay"),	REDISPLAY,	FALSE,	NULL},
	{ERx("references"),	REFERENCES,	FALSE,	NULL},
	{ERx("referencing"),	REFERENCING,	FALSE,	NULL},
	{ERx("register"),	REGISTER,	FALSE,	regktab},
	{ERx("relocate"),	RELOCATE,	TRUE,	NULL},	/* RTI/SQL */
	{ERx("remove"),		REMOVE,		FALSE,	remktab},
	{ERx("repeat"),		REPEAT,		FALSE,	NULL},
	{ERx("repeated"),	REPEAT,		FALSE,	NULL},
	{ERx("resume"),		RESUME,		FALSE,	resktab},
	{ERx("return"),		RETURN,		FALSE,	NULL},
	{ERx("revoke"),		REVOKE,		FALSE,	NULL},
	{ERx("right"),		0,		TRUE,	rightktab},
	{ERx("rollback"),	ROLLBACK,	FALSE,	NULL},
	{ERx("run"),		0,		FALSE,	runktab},
	{ERx("save"),		SAVE,		TRUE,	NULL},	/* RTI/SQL */
	{ERx("savepoint"),	SAVEPOINT,	TRUE,	NULL},	/* RTI/SQL */
	{ERx("screen"),		SCREEN,		FALSE,	NULL},
	{ERx("scroll"),		SCROLL,		FALSE,	NULL},
	{ERx("select"),		SELECT,		TRUE,	NULL},
	{s_Session,		SESSION,	TRUE,	sessionktab},
	{ERx("set"),		SET,		TRUE,	setktab},
	{ERx("set_4gl"),	SET_4GL,	FALSE,	NULL},
	{ERx("set_forms"),	SET_FORMS,	FALSE,	NULL},
	{ERx("set_ingres"),	SET_INGRES,	TRUE,	NULL},
	{ERx("set_sql"),	SET_INGRES,	TRUE,	NULL},
	{ERx("sleep"),		SLEEP,		FALSE,	NULL},
	{ERx("some"),		SOME,		TRUE,	NULL},
	{ERx("sum"),		SUM,		TRUE,	NULL},
	{ERx("system"),		SYSTEM,		FALSE,	systemktab},
	{s_Table,		TABLE,		TRUE,	NULL},
	{ERx("then"),		THEN,		FALSE,	NULL},
	{s_To,			TO,		TRUE,	toktab},
	{ERx("type"),		0,		FALSE,	classktab},
	{ERx("union"),		UNION,		TRUE,	NULL},
	{ERx("unique"),		UNIQUE,		TRUE,	NULL},
	{ERx("unloadtable"),	UNLOADTABLE,	FALSE,	NULL},
	{ERx("until"), 		UNTIL,		TRUE,	NULL},	/* RTI/SQL */
	{ERx("update"),		UPDATE,		TRUE,	NULL},
	{ERx("user"),		USER,		FALSE,	userktab},
	{ERx("validate"),	VALIDATE,	FALSE,	NULL},
	{ERx("validrow"),	VALIDROW,	FALSE,	NULL},
	{ERx("values"),		VALUES,		TRUE,	NULL},
	{ERx("when"), 		WHEN,		FALSE,	NULL},
	{ERx("where"), 		WHERE,		TRUE,	NULL},
	{ERx("while"),		WHILE,		FALSE,	NULL},
	{ERx("with"),		WITH,		TRUE,	NULL},
	{ERx("work"),		WORK,		TRUE,	NULL},
	NULL
};

GLOBALCONSTDEF i4	kwtab_size = sizeof(kwtab)/sizeof(KW) - 1;

/*
** Name:    osscankw -	OSL/SQL Token Map.
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
	DCONST		/* OSDCONST */
};

