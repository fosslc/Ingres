/*
** Copyright (c) 2004 Ingres Corporation
*/

/* %L includes begin	-- get the .h's, they are host-language dependent. */
/* %L includes end */

/* %T header begin - Include this comment for language test */
/*
+* Filename:	esqgtoks.c
** Purpose:	Operator and Keyword tables for main ESQL Grammar
**
** Defines:
**		tok_keytab	- Master Keyword table. (see note 7)
**		tok_kynum	- Number of master keywords (for binary search)
**		tok_skeytab	- Slave Keyword table.
**		tok_skynum	- Number of slave keywords (for binary search)
**		tok_optab	- Operator table.
**		tok_opnum	- Number of operators (for binary search).
**		tok_special	- Special operators to pass back to yylex.
**		tok_sql2key	- ANSI SQL2 keywords not in keyword table.
-*		tok_sql2num	- Number of SQL2 keywords (for binary search)
** Notes::
** 0. Please read the comment at the start of the Eqmerge tool for the
**    embedded directives.
** 1. The tables tok_keytab and tok_optab must be in alphabetical order for
**    a binary search (ignoring case).  As we port to machines that do not
**    run under the Ascii set, the Eqmerge tool will sort the tables, but
**    for readability try to list the tokens in alphabetical order.
** 2. All EQUEL reserved strings should be lowercase. Language specific strings
**    may be in either upper or lowercase, as the language requires.  If a L
**    specific string must be upper case and conflicts with a G string then the
**    L should take precedence. (The Cobol "IS" string, for example.) The 
**    Eqmerge tool makes this decision.
** 3. The tables tok_1st_doubk and tok_2nd_doubk are used for scanning
**    double keywords.  Tok_1st_doubk must be in alphabetical order because
**    the linear search of this table stops when the word searched for is
**    less than the current item in the table (see scan_doub()).  The order
**    of tok_2nd_doubk depends on the entries in tok_1st_doubk (indices into 
**    tok_2nd_doubk).  However, the range of second words for each first word 
**    must be in alphabetical order.  Tok_1st_doubk can also be searched using 
**    a binary search, (which also requires the alphabetic ordering).
** 4. Operators must be at most 2 characters long.
** 5. Comments within the %L object begin and %L object end must begin at the
**    beginning of the line for the Eqmerge tool.
** 6. Do not use the 'ERx' macro on the quoted token names in this file
**    because eqmerge will not work.  Note that eqmerge reads this file before 
**    the C preprocessor does.
** 7. The tokens table tok_keytab was split into two tables for ESQL;
**    tok_keytab for master keywords, and tok_skeytab for slave keywords.
**    EQUEL uses only tok_keytab for its keywords, however, tok_skeytab and
**    tok_skynum still need to be defined here as sc_word references them.
** 8. The table tok_sql2key must also be in alphabetical order for a binary
**    search (ignoring case).
** 9. IMPORTANT: when adding a new SQL keyword, first check to see if it is
**    listed in the tok_sql2key table.  If it is, remove it before adding it
**    as an ESQL keyword.
** 
** History:
**		14-feb-1985	Written for EQUEL (ncg)
**		20-aug-1985	Rewritten for ESQL (ncg)
**		26-feb-1987	Converted for ESQL 6.0 (mrw)
**		23-aug-1989	Added keywords for B1 security. (teresal)
**		04-oct-1989	Removed AFTER as a keyword (barbara)
**		29-nov-1989	Added ALTER TABLE double keyword. (teresal)
**		30-nov-1989	Added keywords for outer join syntax. (teresal)
**		11-dec-1989	Added keywords for Phoenix/Alerters. (barbara).
**		21-dec-1989	Updated with Ultrix porting change. (bjb)
**		13-jun-1990	Added decimal token. (teresal) 
**		16-oct-1990	Added COMMENT ON as double keyword (kathryn)
**		19-jul-1991	Change EVENT to DBEVENT. (teresal) 
**		15-jun-1992	Added keywords RESTRICT, CASCADE, and double
**				keyword GRANT OPTION. (lan)
**		15-jun-1992	Removed GRANT OPTION as a double keyword. (lan)
**		02-jul-1992	Added keywords GLOBAL, TEMPORARY, SESSION, ROWS,
**				PRESERVE, and double keyword ONCOMMIT. (lan)
**		07-jul-1992	Added double keywords CRE_SYNONYM and
**				DROP_SYNONYM. (lan)
**		30-jul-1992	Split tok_keytab into tok_keytab and 
**				tok_skeytab.  Basically made the slave and 
**				master keyword lookup tables seperate thingees.
**				"Split" tok_kynum into tok_kynum and 
**				tok_skynum.  Moved merge directives from 
**				tok_keytab to tok_skeytab.  tok_skeytab is now
**				"merged" entirely from the slave tokens file. 
**				(larrym) 
**		30-jul-1992	Added two new keywords, AUTHORIZATION and
**				SCHEMA. (lan)
**		04-aug-1992	Added double keyword ALT_SEC_AUDIT. (lan)
**		26-aug-1992	Added keywords CHECK, CONSTRAINT, DEFAULT
**				FOREIGN, KEY, PRIMARY, REFERENCES, USER. (lan)
**		24-sep-1992	Add support for flagging SQL2 keyword ids.
**				Added new table containing ANSI SQL2 keywords 
**				not yet reserved by ESQL. To avoid grammar
**				ambiguity, also added new keyword PUBLIC.
**				Note: PUBLIC was already reserved by the
**				server parser.(teresal)
**	14-oct-1992 (larrym)
**		Fixed typo in tok_sql2key.  Also updated copyright to new
**		format.
**		14-oct-1992	Fixed another typo in tok_sql2key - removed
**				"scroll" as this is already a Ingres
**				reserved word. (teresal).
**	01-nov-1992 (kathryn)
**		Added GET_DATA and PUT_DATA as double keywords (large objects).
**	09-nov-1992 (lan)
**		Added keyword DATAHANDLER.
**	11/92 (Mike S) Add keywords for EXEC 4GL
**	12/92 (Mike S) Add keyword for SEND USEREVENT
**	09-dec-1992 (lan)
**		Added keywords SYSTEM_USER, SESSION_USER, CURRENT_USER.
**	14-dec-1992 (lan)
**		Added tDELIMID to the tok_special table.
**	15-dec-1992 (larrym)
**	    Added keyword BYREF for db procedure BYREF parameters.
**	14-jan-1993 (lan)
**		Added keyword INITIAL_USER.
**	09-feb-1993 (lan)
**		Removed current_user, session_user and system_user from table
**		tok_sql2key since they are already Ingres reserved words.
**	11-feb-1993 (lan)
**		Removed "schema" and "preserve" from table tok_sql2key since
**		they are already Ingres reserved words.
**	09-mar-1993 (larrym)
**	    added SET CONNECTION as a double key word.
**	13-jul-1993 (kathryn)
**	    Added "$" as valid operator to tok_optab for $DBA and $INGRES.
**	22-jul-1993 (seiwald)
**		Added tREM_SYS_USER, tREM_SYS_PASS double keywords.
**	07/28/93 (dkh) - Added purgetable as a new token to support the
**			 PURGETABLE statement.
**	10-aug-1993 (kathryn)
**	    Added EXCLUDING as new reserved word. This is needed for the new
**	    EXCLUDING clause of the GRANT statement.
**	18-aug-1993 (kathryn) Bug 53945
**	    Put the tok_1st_doubk back to alphabetical order.  This table
**	    must be kept in alphabetical order, or double keyword lookup will
**	    fail!.
**	14-sep-93 (robf)
**	    Added CREATE/ALTER/DROP PROFILE as double keywords
**	9-nov-93 (robf)
**          Added DBMS PASSWORD as double keyword
**	9-dec-93 (robf)
**          Changed DBMS PASSWORD to single keyword DBMS_PASSWORD per LRC
**	    Added IF as keyword, needed to disambiguate.
**	    Added ON DATABASE as double keyword.
**	14-apr-1994 (teresal)
**	    Add support for ESQL/C++. Add tSLASH token to tok_special[] 
**	    array - needed to determine a C++ comment in the scanner routines.
**	16-feb-1995 (canor01)
**	    tok_1st_doubk had bad value for last token in tok_2nd_doubk,
**	    so "create user" was not being parsed.
**	27-Feb-1997 (jenjo02)
**	  - Extended <set session> statement to support <session access mode>
**	    (READ ONLY | READ WRITE).
**	    Added 2 new double tokens, tREAD_ONLY and tREAD_WRITE.
**	  - Added grammar for SET TRANSACTION <isolation level> | <access mode>.
**	    Added new tokens, tSET_TRANS, tISO_LEVEL, tUNCOMMITTED, tCOMMITTED,
**	    tSERIALIZABLE, tREAD.
**	7-may-97 (inkdo01)
**	    Added tEACH, tROW, tSTATEMENT for statement level rule support
**	    in fe grammars.
**	8-may-97 (inkdo01)
**	    Damn! tSTATEMENT causes reserved word error. Change "each row", 
**	    "each statement" to doubles.
**	14-sep-99 (inkdo01)
**	    Add tCASE, tELSE and tTHEN for case expressions.
**	25-feb-00 (inkdo01)
**	    Add tENDEXECUTE, tRESULT, tROW for row producing procedures.
**	27-mar-00 (inkdo01)
**	    Added tSYSTEM_MAINTAINED (believe it or not) to disambiguate
**	    constraint index option syntax.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-sep-00 (inkdo01)
**	    Added tFIRST for "select first n ...".
**	20-mar-02 (inkdo01)
**	    Added reswds for sequence support.
**	24-apr-03 (inkdo01)
**	    Added "no xxx" doublewords for "no cache", "no cycle". ...
**	    options of sequences.
**	5-mar-04 (inkdo01)
**	    Added CAST for new ANSI CAST syntax.
**	29-Apr-2004 (schka24)
**	    Added PARTITION for partitioned table create, modify.
**	    Since the ESQL grammar is doing minimal true error checking,
**	    we can get away without the other partition-y reserved words.
**	6-jan-05 (inkdo01)
**	    Added ASYMMETRIC and SYMMETRIC for BETWEEN predicate.
**	6-jan-05 (inkdo01)
**	    Damn - forgot to add "collate" for column level collation.
**	31-aug-05 (inkdo01)
**	    Add TOP as synonym for FIRST.
**	8-dec-05 (inkdo01)
**	    Add tUCONST for Unicode literal support.
**	16-jan-06 (dougi)
**	    Add "create/drop trigger" as synonyms for "create/drop rule".
**	6-Jun-2006 (kschendel)
**	    Add DESCRIBE INPUT pair, USING [SQL] DESCRIPTOR.
**	5-feb-2007 (dougi)
**	    Add KEYSET, ABSOLUTE, LAST, PRIOR and RELATIVE options for 
**	    scrollable cursors.
**	05-apr-2007 (toumi01)
**	    Add support for the Ada ' tick operator.
**	13-aug-2007 (dougi)
**	    Add double kwds "with time", "with local", "without time" for
**	    date/time column type declarations.
**	11-dec-2007 (dougi)
**	    Add "offset", "only" to support standard "first n" syntax.
**	26-march-2008 (dougi)
**	    Add "cross" for "cross join" and long missing "outer".
**	2-june-2008 (dougi)
**	    Add "always", "generated" and "identity" for identity columns.
**	21-Jul-2008 (kiria01) b120473
**	    Added keywords WITHOUT CASE, WITH CASE and added the two word
**	    forms for the LIKE family.
**	30-Oct-2008 (kiria01) b120569
**	    Add || concatenator operator support.
**	21-feb-2009 (dougi)
**	    Add autoincrement for identity columns.
**	5-March-2009 (dougi)
**	    Add overriding, system and value for identity columns.
**      10-nov-2009 (joea)
**          Add tokens for FALSE, TRUE and UNKNOWN.
**	12-Apr-2010 (gupsh01)
**	    Add tokens for RENAME to support RENAME table/column syntax.
**	26-Aug-2010 (miket) SIR 122403 SD 146503
**	    Add tENCRYPT.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Added COLLATION.
*/
/* %T header end */

/*ERCHECK=OFF*/
/*  ESQL master keyword table (see note 7) */
GLOBALDEF KEY_ELM	 tok_keytab[] =
{
/* %L tokens begin - Include L master tokens */
	/* terminal */		/* token */
	"abort",		tABORT,
	"absolute",		tABSOLUTE,
	"activate",		tACTIVATE,
	"add",			tADD,
	"addform",		tADDFORM,
	"all",			tALL,
	"always",		tALWAYS,
	"and",			tAND,
	"any",			tANY,
	"as",			tAS,
	"asymmetric",		tASYMMETRIC,
	"at",			tAT,
	"authorization",	tAUTHORIZATION,
	"autoincrement",	tAUTOINCREMENT,
	"avg",			tAVG,
	"avgu",			tAVG,
	"begin",		tBEGIN,
	"beginning",		tBEGINNING,
	"between",		tBETWEEN,
	"breakdisplay",		tBREAKDISPLAY,
	"by",			tBY,
	"byref",		tBYREF,
	"cache",		tCACHE,
	"call",			tCALL,
	"cascade",		tCASCADE,
	"case",			tCASE,
	"cast",			tCAST,
	"check",		tCHECK,
	"clear",		tCLEAR,
	"clearrow",		tCLEARROW,
	"close",		tCLOSE,
	"collate",		tCOLLATE,
	"collation",		tCOLLATION,
	"column",		tCOLUMN,
	"command",		tCOMMAND,
	"commit",		tCOMMIT,
	"committed",		tCOMMITTED,
	"connect",		tCONNECT,
	"constraint",		tCONSTRAINT,
	"containing",		tCONTAINING,
	"continue",		tCONTINUE,
	"copy",			tCOPY,
	"count",		tCOUNT,
	"countu",		tCOUNT,
	"create",		tCREATE,
	"cross",		tCROSS,
	"current",		tCURRENT,
	"current_user",		tCURRENT_USER,
	"currval",		tCURRVAL,
	"cursor",		tCURSOR,
	"cycle",		tCYCLE,
	"datahandler",		tDATAHANDLER,
	"dbms_password",	tDBMS_PASSWORD,
	"declare",		tDECLARE,
	"default",		tDEFAULT,
	"delete",		tDELETE,
	"deleterow",		tDELETEROW,
	"describe",		tDESCRIBE,
	"descriptor",		tDESCRIPTOR,
	"disconnect",		tDISCONNECT,
	"display",		tDISPLAY,
	"distinct",		tDISTINCT,
	"down",			tDOWN,
	"drop",			tDROP,
	"else",			tELSE,
	"encrypt",		tENCRYPT,
	"end",			tEND,
	"end-exec",		tEND_EXEC,
	"enddata",		tENDDATA,
	"enddisplay",		tENDDISPLAY,
	"endexecute",		tENDEXECUTE,
	"endforms",		tENDFORMS,
	"ending",		tENDING,
	"endloop",		tENDLOOP,
	"endselect",		tENDSELECT,
	"escape",		tESCAPE,
	"excluding",		tEXCLUDING,
	"execute",		tEXECUTE,
	"exists",		tEXISTS,
        "false",                tFALSE,
	"fetch",		tFETCH,
	"field",		tFIELD,
	"finalize",		tFINALIZE,
	"first",		tFIRST,
	"for",			tFOR,
	"foreign",		tFOREIGN,
	"formdata",		tFORMDATA,
	"forminit",		tFORMINIT,
	"forms",		tFORMS,
	"from",			tFROM,
	"full",			tFULL,
	"generated",		tGENERATED,
	"getform",		tGETFORM,
	"getoper",		tGETOPER,
	"getrow",		tGETROW,
	"global",		tGLOBAL,
	"goto",			tGOTO,
	"grant",		tGRANT,
	"group",		tGROUP,
	"having",		tHAVING,
	"help",			tHELP,
	"help_frs",		tHELP_FRS,
	"helpfile",		tHELPFILE,
	"identified",		tIDENTIFIED,
	"identity",		tIDENTITY,
	"if",			tIF,
	"iimessage",		tIIMESSAGE,
	"iiprintf",		tIIPRINTF,
	"iiprompt",		tIIPROMPT,
	"immediate",		tIMMED,
	"in",			tIN,
	"include",		tINCLUDE,
	"increment",		tINCREMENT,
	"index",		tINDEX,
	"indicator",		tINDICATOR,
	"initial_user",		tINITIAL_USER,
	"initialize",		tINITIALIZE,
	"inittable",		tINITTABLE,
	"inner",		tINNER,
	"inquire_frs", 		tINQ_FRS,
	"inquire_ingres",	tINQ_INGRES,
	"inquire_sql",		tINQ_INGRES,
	"insert",		tINSERT,
	"insertrow",		tINSERTROW,
	"integrity",		tINTEGRITY,
	"interval",		tINTERVAL,
	"into",			tINTO,
	"is",			tIS,
	"join",			tJOIN,
	"key",			tKEY,
	"keyset",		tKEYSET,
	"last",			tLAST,
	"left",			tLEFT,
	"like",			tLIKE,
	"loadtable",		tLOADTABLE,
	"max",			tMAX,
	"maxvalue",		tMAXVALUE,
	"menuitem",		tMENUITEM,
	"message",		tMESSAGE,
	"min",			tMIN,
	"minvalue",		tMINVALUE,
	"modify",		tMODIFY,
	"natural",		tNATURAL,
	"next",			tNEXT,
	"nextval",		tNEXTVAL,
	"nocache",		tNOCACHE,
	"nocycle",		tNOCYCLE,
	"nomaxvalue",		tNOMAXVALUE,
	"nominvalue",		tNOMINVALUE,
	"noorder",		tNOORDER,
	"not",			tNOT,
	"notrim",		tNOTRIM,
	"null",			tNULL,
	"of",			tOF,
	"offset",		tOFFSET,
	"on",			tON,
	"only",			tONLY,
	"open",			tOPEN,
	"or",			tOR,
	"order",		tORDER,
	"out",			tOUT,
	"outer",		tOUTER,
	"overriding",		tOVERRIDING,
	"partition",		tPARTITION,
	"permit",		tPERMIT,
	"prepare",		tPREPARE,
	"preserve",		tPRESERVE,
	"primary",		tPRIMARY,
	"print",		tPRINT,
	"printscreen",		tPRINTSCREEN,
	"prior",		tPRIOR,
	"procedure",		tPROCEDURE,
	"prompt",		tPROMPT,
	"public",		tPUBLIC,
	"purgetable",           tPURGETBL,
	"putform",		tPUTFORM,
	"putoper",		tPUTOPER,
	"putrow",		tPUTROW,
	"read",			tREAD,
	"redisplay",		tREDISPLAY,
	"references",		tREFERENCES,
	"register",		tREGISTER,
	"relative",		tRELATIVE,
	"relocate",		tRELOCATE,
	"remove",		tREMOVE,
	"rename",		tRENAME,
	"repeat",		tREPEAT,
	"repeatable",		tREPEATABLE,
	"repeated",		tREPEAT,
	"restart",		tRESTART,
	"restrict",		tRESTRICT,
	"result",		tRESULT,
	"resume",		tRESUME,
	"revoke",		tREVOKE,
	"right",		tRIGHT,
	"rollback",		tROLLBACK,
	"row",			tROW,
	"rows",			tROWS,
	"save",			tSAVE,
	"savepoint",		tSAVEPOINT,
	"schema",		tSCHEMA,
	"screen",		tSCREEN,
	"scroll",		tSCROLL,
	"scrolldown",		tSCROLLDOWN,
	"scrollup",		tSCROLLUP,
	"section",		tSECTION,
	"select",		tSELECT,
	"serializable",		tSERIALIZABLE,
	"session",		tSESSION,
	"session_user",		tSESSION_USER,
	"set",			tSET,
	"set_frs",		tSET_FRS,
	"set_ingres",		tSET_INGRES,
	"set_sql",		tSET_INGRES,
	"similar",		tSIMILAR,
	"sleep",		tSLEEP,
	"some",			tANY,
	"sql",			tSQL,
	"start",		tSTART,
	"stop",			tSTOP,
	"submenu",		tSUBMENU,
	"substring",		tSUBSTRING,
	"sum",			tSUM,
	"sumu",			tSUM,
	"symmetric",		tSYMMETRIC,
	"system",		tSYSTEM,
	"system_maintained",	tSYSTEM_MAINTAINED,
	"system_user",		tSYSTEM_USER,
	"table",		tTABLE,
	"tabledata",		tTABLEDATA,
	"temporary",		tTEMPORARY,
	"then",			tTHEN,
	"to",			tTO,
	"top",			tFIRST,
        "true",                 tTRUE,
	"uncommitted",		tUNCOMMITTED,
	"union",		tUNION,
	"unique",		tUNIQUE,
        "unknown",              tUNKNOWN,
	"unloadtable",		tUNLOADTABLE,
	"until",		tUNTIL,
	"up",			tUP,
	"update",		tUPDATE,
	"user",			tUSER,
	"using",		tUSING,
	"validate",		tVALIDATE,
	"validrow",		tVALIDROW,
	"value",		tVALUE,
	"values",		tVALUES,
	"view",			tVIEW,
	"when",			tWHEN,
	"whenever",		tWHENEVER,
	"where",		tWHERE,
	"with",			tWITH
/* %L tokens end */
};

/* EXEC 4GL-specific keywords */
GLOBALDEF KEY_ELM	 tok_4glkeytab[] =
{
	"callframe",		tCALLFRAME,
	"callproc",		tCALLPROC,
	"describe",		tDESCRIBE_4GL,
	"getrow",		tGETROW_4GL,
	"inquire_4gl",		tINQ_4GL,
	"insertrow",		tINSERTROW_4GL,
	"removerow",		tREMOVEROW,
	"set_4gl",		tSET_4GL,
	"setrow",		tSETROW
};

/*  ESQL slave keyword table (see note 7) */
GLOBALDEF KEY_ELM	 tok_skeytab[] =
{
        /* terminal */          /* token */
/* %L tokens begin - Include L slave tokens */
/* %L tokens end */
};


GLOBALDEF KEY_ELM	 tok_optab[] =
{
	/* terminal */		/* token */
/* %L ops begin - Include L operators */
	"!=",			tNE,
	"$",			tDOLLAR,
	"(",			tLPAREN,
	")",			tRPAREN,
	"*",			tSTAR,
	"**",			tEXP,
	"+",			tPLUS,
	",",			tCOMMA,
	"-",			tMINUS,
	"--",			tCOMMENT,
	".",			tPERIOD,
	"/",			tSLASH,
	"/*",			tCOMMENT,
	":",			tCOLON,
	"<",			tLT,
	"<=",			tLE,
	"<>",			tNE,
	"=",			tEQOP,
	">",			tGT,
	">=",			tGE,
	"^=",			tNE,
	"||",			tCONCAT,
/* %L ops end */
};

/* %T footer begin - Include token globals and structures for language test */

/* tok_kynum is used for a binary search of tok_keytab[] */

GLOBALDEF i4	tok_kynum = sizeof( tok_keytab ) / sizeof( KEY_ELM );

/* tok_4glkynum is used for a binary search of tok_4glkeytab[] */

GLOBALDEF i4	tok_4glkynum = sizeof( tok_4glkeytab ) / sizeof( KEY_ELM );

/* tok_skynum is used for a binary search of tok_skeytab[] */

GLOBALDEF i4    tok_skynum = sizeof( tok_skeytab ) / sizeof( KEY_ELM );

/* tok_opnum is used for a binary search of tok_optab[] */

GLOBALDEF i4	tok_opnum = sizeof( tok_optab ) / sizeof( KEY_ELM );

/* 
** tok_special is filled with special token values that can be returned to yylex
** but need some value that the grammar has defined.
*/

GLOBALDEF i4	tok_special[] =
{
		tNAME,
		tSCONST,
		tINTCONST,
		tFLTCONST,
		tQUOTE,
		tCOMMENT,
		tHOSTCODE,
		tTERMINATE,
		tINCLUDE,
		tNAME,		/* No DEREF for ESQL - leave as simple name */
		tEOFINC,
		tHEXCONST,
		tWHITESPACE,
		tDECCONST,
		tDELIMID,
		tSLASH,
		tUCONST,
		tATICK,
		tDTCONST,
		0
};

/* 
** tok_1st_doubk and tok_2nd_doubk are used together to scan double
** keys which are not reserved words.  tok_1st_doubk is filled with the 
** first words of all double keywords and indices into tok_2nd_doubk.
** tok_2nd_doubk contains the second words of the doubke keywords and
** the appropriate token.  See scan_doub in scword.c for use of these tables.
** scan_doub() expects both tables to have an alphabetic ordering:
** tok_1st_doubk is in alphabetical order as a whole; tok_2nd_doubk is
** in alphabetical order within each range of second words.
** Read notes at top of file before modifying these tables.
*/
GLOBALDEF ESC_1DBLKEY tok_1st_doubk[] = {
    /* first wd */	/* low ind in tok_2nd_doubk */	/* upper ind */
	"alter",		0,			    8,
	"begin",		9,			    11,
	"clear",		69,			    69,
	"comment",		62,			    62,
	"create",		12,			    23,
	"current",		75,			    75,
	"describe",		24,			    26,
	"direct",		27,			    29,
	"disable",		30,			    30,
	"drop",			31,			    42,
	"each",			73,			    74,
	"enable",		43,			    43,
	"end", 			44,			    45,
	"get",			46,			    49,
	"help",			50,			    53,
	"isolation",		54,			    54,
	"next",			76,			    76,
	"no",			77,			    81,
	"not",			87,			    91,
	"on",			55,			    56,
	"put",			63,			    63,
	"raise",		57,			    57,
	"read", 		58,			    59,
	"register",		60,			    60,
	"remote",		71,			    72,
	"remove",		61,			    61,
	"rename",		92,			    92,
	"send",			70,			    70,
	"set",			64,			    67,
	"setrow",		68,			    68,
	"with",			82,			    84,
	"without",		85,			    86,
	(char *)0,		0,			    0
};

GLOBALDEF ESC_2DBLKEY tok_2nd_doubk[] = {
/* index */	/* second wd */	/* key */	/* dblname */
/*  0 */	"default",	tALT_DEFAULT,	"alter default",
/*  1 */	"group",	tALT_GROUP,	"alter group",
/*  2 */	"location",	tALT_LOCATION,	"alter location",
/*  3 */        "profile",	tALT_PROFILE,	"alter profile",
/*  4 */	"role",		tALT_ROLE,	"alter role",
/*  5 */	"security_audit",tALT_SEC_AUDIT,"alter security_audit",
/*  6 */	"sequence",	tALT_SEQUENCE,	"alter sequence",
/*  7 */	"table",	tALT_TABLE,	"alter table",
/*  8 */	"user",		tALT_USER,	"alter user",
/*  9 */	"declare",	tBEG_DECLARE,	"begin declare",
/* 10 */	"exclude",	tBEG_EXCLUDE,	"begin exclude",
/* 11 */	"transaction",	tBEG_XACT,	"begin transaction",
/* 12 */	"dbevent",	tCRE_DBEVENT,	"create dbevent",
/* 13 */	"domain",	tCRE_DOM,	"create domain",
/* 14 */	"link",		tCRE_LINK,	"create link",
/* 15 */	"location",	tCRE_LOCATION,	"create location",
/* 16 */        "profile",      tCRE_PROFILE,   "create profile",
/* 17 */	"role",		tCRE_ROLE,	"create role",
/* 18 */	"rule",		tCRE_RULE,	"create rule",
/* 19 */	"security_alarm",tCRE_SEC_ALARM,"create security_alarm",
/* 20 */	"sequence",	tCRE_SEQUENCE,	"create sequence",
/* 21 */	"synonym",	tCRE_SYNONYM,	"create synonym",
/* 22 */	"trigger",	tCRE_RULE,	"create trigger",
/* 23 */	"user",		tCRE_USER,	"create user",
/* 24 */	"form",		tDESC_FORM,	"describe form",
/* 25 */	"input",	tDESC_INPUT,	"describe input",
/* 26 */	"output",	tDESCRIBE,	"describe output",
/* 27 */	"connect",	tDIR_CONNECT,	"direct connect",
/* 28 */	"disconnect",	tDIR_DISCONNECT,"direct disconnect",
/* 29 */	"execute",	tDIR_EXECUTE,	"direct execute",
/* 30 */	"security_audit",tDIS_SEC_AUDIT,"disable security_audit",
/* 31 */	"dbevent",	tDROP_DBEVENT,	"drop dbevent",
/* 32 */	"domain",	tDROP_DOM,	"drop domain",
/* 33 */	"link",		tDROP_LINK,	"drop link",
/* 34 */	"location",	tDROP_LOCATION,	"drop location",
/* 35 */        "profile",      tDROP_PROFILE,  "drop profile",
/* 36 */	"role",		tDROP_ROLE,	"drop role",
/* 37 */	"rule",		tDROP_RULE,	"drop rule",
/* 38 */	"security_alarm",tDROP_SEC_ALARM,"drop security_alarm",
/* 39 */	"sequence",	tDROP_SEQUENCE,	"drop sequence",
/* 40 */	"synonym",	tDROP_SYNONYM,	"drop synonym",
/* 41 */	"trigger",	tDROP_RULE,	"drop trigger",
/* 42 */	"user",		tDROP_USER,	"drop user",
/* 43 */	"security_audit",tENA_SEC_AUDIT,"enable security_audit",
/* 44 */	"exclude",	tEND_EXCLUDE,	"end exclude",
/* 45 */	"transaction",	tEND_XACT,	"end transaction",
/* 46 */	"attribute",	tGET_ATTR,	"get attribute",
/* 47 */	"data",		tGET_DATA,	"get data",
/* 48 */	"dbevent",	tGET_DBEVENT,	"get dbevent",
/* 49 */	"global",	tGET_GLOBAL,	"get global",
/* 50 */	"all",		tHLP_ALL,	"help all",
/* 51 */	"integrity",	tHLP_INTEGRITY,	"help integrity",
/* 52 */	"permit",	tHLP_PERMIT,	"help permit",
/* 53 */	"view",		tHLP_VIEW,	"help view",
/* 54 */	"level",	tISO_LEVEL,	"isolation level",
/* 55 */	"commit",	tONCOMMIT,	"on commit",
/* 56 */        "database",	tONDATABASE,    "on database",
/* 57 */	"dbevent",	tRAISE_DBEVENT,	"raise dbevent",
/* 58 */	"only",		tREAD_ONLY,	"read only",
/* 59 */	"write",	tREAD_WRITE,	"read write",
/* 60 */	"dbevent",	tREG_DBEVENT,	"register dbevent",
/* 61 */	"dbevent",	tREM_DBEVENT,	"remove dbevent",
/* 62 */	"on",		tCOM_ON,	"comment on",
/* 63 */	"data",		tPUT_DATA,	"put data",
/* 64 */	"attribute",	tSET_ATTR,	"set attribute",
/* 65 */	"connection",	tSET_CONNECT,	"set connection",
/* 66 */	"global",	tSET_GLOBAL,	"set global",
/* 67 */	"transaction",	tSET_TRANS,	"set transaction",
/* 68 */	"deleted",	tSETROW_DEL,	"setrow deleted",
/* 69 */	"array",	tCLEAR_ARRAY,	"clear array",
/* 70 */	"userevent",	tSEND_USEREVENT,"send userevent",
/* 71 */	"system_password",tREM_SYS_PASS,"remote system_password",
/* 72 */	"system_user",	tREM_SYS_USER,	"remote system_user",
/* 73 */	"row",		tEACH_ROW,	"each row",
/* 74 */	"statement",	tEACH_STATEMENT,"each statement",
/* 75 */	"value",	tCURRVAL,	"current value",
/* 76 */	"value",	tNEXTVAL,	"next value",
/* 77 */	"cache",	tNOCACHE,	"no cache",
/* 78 */	"cycle",	tNOCYCLE,	"no cycle",
/* 79 */	"maxvalue",	tNOMAXVALUE,	"no maxvalue",
/* 80 */	"minvalue",	tNOMINVALUE,	"no minvalue",
/* 81 */	"order",	tNOORDER,	"no order",
/* 82 */	"case",		tWITHCASE,	"with case",
/* 83 */	"time",		tWTIME,		"with time",
/* 84 */	"local",	tWLOCAL,	"with local",
/* 85 */	"case",		tWOCASE,	"without case",
/* 86 */	"time",		tWOTIME,	"without time",
/* 87 */	"beginning",	tNOTBEGINNING,	"not beginning",
/* 88 */	"containing",	tNOTCONTAINING,	"not containing",
/* 89 */	"ending",	tNOTENDING,	"not ending",
/* 90 */	"like",		tNOTLIKE,	"not like",
/* 91 */	"similar",	tSIMILAR,	"not similar",
/* 92 */	"table",	tREN_TABLE,	"rename table",
};

/*
** List of ANSI SQL2 keywords NOT reserved by ESQL. For FIPS compliance,
** we must flag usage of these keywords with the FIPS flagger. It is a
** vendor extension to allow these keywords to be used in SQL statements
** as identifiers.
*/
GLOBALDEF char	 *tok_sql2key[] =
{
	/* ANSI SQL2 (X3H2 Draft March 1992) keywords */
	"absolute",
	"action",
	"allocate",
	"alter",
	"are",
	"asc",
	"assertion",
	"bit",
	"bit_length",
	"both",
	"cascaded",
	"case",
	"cast",
	"catalog",
	"char",
	"character",
	"char_length",
	"character_length",
	"coalesce",
	"collate",
	"collation",
	"connection",
	"constraints",
	"convert",
	"corresponding",
	"cross",
	"current_date",
	"current_time",
	"current_timestamp",
	"date",
	"day",
	"deallocate",
	"dec",
	"decimal",
	"deferrable",
	"deferred",
	"desc",
	"diagnostics",
	"domain",
	"double",
	"else",
	"except",
	"exception",
	"exec",
	"external",
	"extract",
	"false",
	"float",
	"found",
	"get",
	"go",
	"hour",
	"identity",
	"initially",
	"input",
	"insensitive",
	"int",
	"integer",
	"intersects",
	"interval",
	"isolation",
	"language",
	"last",
	"leading",
	"level",
	"local",
	"lower",
	"match",
	"minute",
	"module",
	"month",
	"names",
	"national",
	"nchar",
	"no",
	"nullif",
	"numeric",
	"octet_length",
	"only",
	"option",
	"outer",
	"output",
	"overlaps",
	"pad",
	"partial",
	"position",
	"precision",
	"prior",
	"privileges",
	"read",
	"real",
	"relative",
	"second",
	"size",
	"smallint",
	"space",
	"sql",
	"sqlcode",
	"sqlerror",
	"sqlstate",
	"substring",
	"then",
	"time",
	"timestamp",
	"timezone_hour",
	"timezone_minute",
	"trailing",
	"transaction",
	"translate",
	"translation",
	"trim",
	"true",
	"unknown",
	"upper",
	"usage",
	"value",
	"varchar",
	"varying",
	"work",
	"write",
	"year",
	"zone"
};

/* tok_sql2num is used for a binary search of tok_sql2key[] */

GLOBALDEF i4	tok_sql2num = sizeof( tok_sql2key ) / sizeof( char * );

/* %T footer end */
/*ERCHECK=ON*/
