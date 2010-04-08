
/* %L includes begin	-- get the .h's, they are host-language dependent. */
/* %L includes end */

/* %T header begin - Include this comment for language test */
/*
** TOKENS.C - Operator and Keyword tables for main Grammar
**
** Defines:
**		tok_keytab	- Keyword table.
**		tok_kynum	- Number of keywords (for binary search).
**		tok_optab	- Operator table.
**		tok_opnum	- Number of operators (for binary search).
**		tok_special	- Special operators to pass back to yylex.
**		tok_skeytab	- Dummy slave Keyword table (needed for ESQL).
**		tok_skynum	- Dummy Num of slave keywords (needed for ESQL).
**		tok_sql2key	- Dummy ANSI SQL2 table (needed for ESQL).
**		tok_sql2num	- Dummy num of SQL2 keywrds (needed for ESQL).
** Notes::
** 0. Please read the comment at the start of the Eqmerge tool for the
**    embedded directives.
** 1. The tables tok_keytab and tok_optab must be in alphabetical order for
**    a binary search (ignoring case).  As we port to machines that do not
**    run under the Ascii set, the Eqmerge tool will sort the tables, but
**    for readability try to list the tokens in alphabetical order.
** 3. The tables tok_1st_doubk and tok_2nd_doubk are used for scanning
**    double keywords.  Tok_1st_doubk must be in alphabetical order because
**    the linear search of this table stops when the word searched for is
**    less than the current item in the table (see scan_doub()).  The order
**    of tok_2nd_doubk depends on the entries in tok_1st_doubk (indices into 
**    tok_2nd_doubk).  This table can also be searched using a binary search,
**    which also requires the alphabetic ordering.
** 3. All Equel reserved strings should be lowercase. Language specific strings
**    may be in either upper or lowercase, as the language requires.  If a L
**    specific string must be upper case and conflicts with a G string then the
**    L should take precedence. (The Cobol "IS" string, for example.) The 
**    Eqmerge tool makes this decision.
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
** 
** History:
**		14-feb-1985	Written (ncg)
**		13-jun-1990	Added decimal token. (teresal)
**		10-aug-1992	Added dummy declaration of tok_skeytab and
**				tok_skynum (see note 7) (larrym)
**		09-oct-1992	Added dummy declaractions of ANSI SQL2
**				table to prevent undefined symbols on VMS
**				at link time (teresal)
**	07/28/93 (dkh) - Added purgetable as a new token to support the
**			 PURGETABLE statement.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Mar-2001 (toumi01)
**	    Add support for 'set transaction' statement to [E]QUEL. b94351.
**	    This will allow db utilities written in quel to issue e.g.
**	    SET TRANSACTION READ WRITE to override an installation level
**	    isolation level setting of READ UNCOMMITTED.
**	26-Mar-2001 (toumi01)
**	    Modify 'set transaction' change to avoid making READ a keyword
**	    and hence breaking other aspects of the QUEL grammar.
**	27-Mar-2001 (toumi01)
**	    Okay, I give up.  The esqlc 'set transaction' grammar will not
**	    work with the very loose validation provided by eqc's grammar.
**	    To avoid nasty problems by introducing new key words (especially
**	    'read') into the quel grammar, and the massive changes that
**	    that would trigger, simply continue the practice of using the
**	    'set' rule with very loose validation.  Note that this allows:
**		SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED (etc.)
**	    and also
**		SET TRANSACTION ISOLATION CHOCOLATE CREAM PIE
**	    Of course, the latter will be flagged at run-time.
**	    This change reverts this file to r7 (but with these comments).
*/
/* %T header end */

/*ERCHECK=OFF*/
GLOBALDEF KEY_ELM	 tok_keytab[] =
{
	/* terminal */		/* token */
/* %L tokens begin - Include L tokens and keywords */
	"abort",		tABORT,
	"activate",		tACTIVATE,
	"addform",		tADDFORM,
	"all",			tALL,
	"and",			tLOGBOP,
	"any",			tAGOP,
	"append",		tAPPEND,
	"as",			tAS,
	"at",			tAT,
	"avg",			tAGOP,
	"avgu",			tAGOP,
	"breakdisplay",		tBREAKDISPLAY,
	"by",			tBY,
	"call",			tCALL,
	"clear",		tCLEAR,
	"clearrow",		tCLEARROW,
	"column",		tCOLUMN,
	"command",		tCOMMAND,
	"copy",			tCOPY,
	"count",		tAGOP,
	"countu",		tAGOP,
	"create",		tCREATE,
	"delete",		tDELETE,
	"deleterow",		tDELETEROW,
	"destroy",		tDESTROY,
	"display",		tDISPLAY,
	"down",			tDOWN,
	"end",			tEND,
	"enddata",		tENDDATA,
	"enddisplay",		tENDDISPLAY,
	"endforms",		tENDFORMS,
	"endloop",		tENDLOOP,
	"endretrieve",		tENDRETRIEVE,
	"exit",			tEXIT,
	"field",		tFIELD,
	"finalize",		tFINALIZE,
	"for",			tFOR,
	"formdata",		tFORMDATA,
	"forminit",		tFORMINIT,
	"forms",		tFORMS,
	"from",			tFROM,
	"getform",		tGETFORM,
	"getoper",		tGETOPER,
	"getrow",		tGETROW,
	"help",			tHELP,
	"help_frs",		tHELP_FRS,
	"helpfile",		tHELPFILE,
	"iimessage",		tIIMESSAGE,
	"iiprintf",		tIIPRINTF,
	"iiprompt",		tIIPROMPT,
	"iistatement",		tIISTATEMENT,
	"in",			tIN,
	"index",		tINDEX,
	"ingres",		tINGRES,
	"initialize",		tINITIALIZE,
	"inittable",		tINITTABLE,
	"inquire_equel",	tINQ_EQUEL,
	"inquire_frs", 		tINQ_FRS,
	"inquire_ingres",	tINQ_EQUEL,
	"insertrow",		tINSERTROW,
	"into",			tINTO,
	"is",			tIS,
	"loadtable",		tLOADTABLE,
	"max",			tAGOP,
	"menuitem",		tMENUITEM,
	"message",		tMESSAGE,
	"min",			tAGOP,
	"modify",		tMODIFY,
	"next",			tNEXT,
	"not",			tLOGUOP,
	"notrim",		tNOTRIM,
	"null",			tNULL,
	"of",			tOF,
	"on",			tON,
	"or",			tLOGBOP,
	"order",		tSORT,
	"out",			tOUT,
	"param",		tPARAM,
	"print",		tPRINT,
	"printscreen",		tPRINTSCREEN,
	"prompt",		tPROMPT,
	"purgetable",		tPURGETBL,
	"putform",		tPUTFORM,
	"putoper",		tPUTOPER,
	"putrow",		tPUTROW,
	"range",		tRANGE,
	"redisplay",		tREDISPLAY,
	"register",		tREGISTER,
	"relocate",		tRELOCATE,
	"remove",		tREMOVE,
	"repeat",		tREPEAT,
	"replace",		tREPLACE,
	"resume",		tRESUME,
	"retrieve",		tRETRIEVE,
	"save",			tSAVE,
	"savepoint",		tSAVEPOINT,
	"screen",		tSCREEN,
	"scroll",		tSCROLL,
	"scrolldown",		tSCROLLDOWN,
	"scrollup",		tSCROLLUP,
	"set",			tSET,
	"set_equel",		tSET_EQUEL,
	"set_frs",		tSET_FRS,
	"set_ingres",		tSET_EQUEL,
	"sleep",		tSLEEP,
	"sort",			tSORT,
	"submenu",		tSUBMENU,
	"sum",			tAGOP,
	"sumu",			tAGOP,
	"tabledata",		tTABLEDATA,
	"to",			tTO,
	"unique",		tUNIQUE,
	"unloadtable",		tUNLOADTABLE,
	"until",		tUNTIL,
	"up",			tUP,
	"validate",		tVALIDATE,
	"validrow",		tVALIDROW,
	"where",		tWHERE,
	"with",			tWITH,
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
	/* terminal */		/* token */
/* %L ops begin - Include L operators */
	"!=",			tNEQOP,
	"#",			tDEREF,
	"(",			tLPAREN,
	")",			tRPAREN,
	"*",			tMULOP,
	"**",			tEXPOP,
	"+",			tARUOP,
	",",			tCOMMA,
	"-",			tARUOP,
	".",			tPERIOD,
	"/",			tDIVOP,
	"/*",			tCOMMENT,
	":",			tCOLON,
	"<",			tBNDOP,
	"<=",			tBNDOP,
	"=",			tEQOP,
	">",			tBNDOP,
	">=",			tBNDOP,
	"@",			tATSIGN,
/* %L ops end */
};

/* DUMMY 4GL token table.  Only present to avoid a link error */
GLOBALDEF KEY_ELM        tok_4glkeytab[] =
{
	"Invalid keyword",	tHELP
};

/* DUMMY slave token table, used by ESQL (see note 7) 
** have to put something in it so that tok_skynum will be > 0
*/
GLOBALDEF KEY_ELM        tok_skeytab[] =
{
        "help",                 tHELP
};

/* %T footer begin - Include token globals and structures for language test */

/* tok_kynum is used for a binary search of tok_keytab[] */

GLOBALDEF i4	tok_kynum = sizeof( tok_keytab ) / sizeof( KEY_ELM );

/* tok_opnum is used for a binary search of tok_optab[] */

GLOBALDEF i4	tok_opnum = sizeof( tok_optab ) / sizeof( KEY_ELM );

/* 
** tok_4glkynum is used for a ESQL binary searches of tok_4glkeytab[]
** It is not used by EQUEL, but it needs to be declared here to aviod undefs
** in sc_word.
*/

GLOBALDEF i4	tok_4glkynum = sizeof( tok_4glkeytab ) / sizeof( KEY_ELM );

/* 
** tok_skynum is used for a ESQL binary searches of tok_skeytab[]
** It is not used by EQUEL, but it needs to be declared here to aviod undefs
** in sc_word.
*/

GLOBALDEF i4	tok_skynum = sizeof( tok_skeytab ) / sizeof( KEY_ELM );

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
		tDEREF,
		0,		/* No tEOFINC for EQUEL - leave as zero */
		tHEXCONST,
		tWHITESPACE,
		tDECCONST,
		0
};

/* 
** tok_1st_doubk and tok_2nd_doubk are used together to scan double
** keys which are not reserved words.  tok_1st_doubk is filled with the 
** first words of all double keywords and indices into tok_2nd_doubk.
** tok_2nd_doubk contains the second words of the doubke keywords and
** the appropriate token.  See scan_doub in scword.c for use of these tables.
** Read notes at top of file before modifying these tables.
*/
GLOBALDEF ESC_1DBLKEY tok_1st_doubk[] = {
    /* first wd */	/* low ind in tok_2nd_doubk */	/* upper ind */
	"begin",		0,			    0,
	"close",		1,			    1,
	"declare",		2,			    2,
	"define",		3,			    6,
	"delete", 		7,		   	    7,
	"destroy",		8,			    11,
	"direct",		12,			    13,
	"end", 			14,			    14,
	"help",			15,			    17,
	"open",			18,			    18,
	"replace",		19,			    19,
	"retrieve",		20,			    20,
	(char *)0,		0,			    0
};

GLOBALDEF ESC_2DBLKEY tok_2nd_doubk[] = {
/* index */    /* second wd */	/* key */	/* dblname */
/* 0 */		"transaction",	tBEG_XACT,	"begin transaction",
/* 1 */		"cursor",	tCLS_CURSOR,	"close cursor",
/* 2 */		"cursor",	tDEC_CURSOR,	"declare cursor",
/* 3 */		"integrity",	tDEF_INTEGRITY,	"define integrity",
/* 4 */		"link",		tDEF_LINK,	"define link",
/* 5 */		"permit",	tDEF_PERMIT,	"define permit",
/* 6 */		"view",		tDEF_VIEW,	"define view",
/* 7 */		"cursor",	tDEL_CURSOR,	"delete cursor",
/* 8 */		"integrity",	tDES_INTEGRITY,	"destroy integrity",
/* 9 */		"link",		tDES_LINK,	"destroy link",
/* 10 */	"permit",	tDES_PERMIT,	"destroy permit",
/* 11 */	"table",	tDES_TABLE,	"destroy table",
/* 12 */	"connect",	tDIR_CONNECT,	"direct connect",
/* 13 */	"disconnect",	tDIR_DISCONNECT,"direct disconnect",
/* 14 */	"transaction",	tEND_XACT,	"end transaction",
/* 15 */	"integrity",	tHLP_INTEGRITY,	"help integrity",
/* 16 */	"permit",	tHLP_PERMIT,	"help permit",
/* 17 */	"view",		tHLP_VIEW,	"help view",
/* 18 */	"cursor",	tOPN_CURSOR,	"open cursor",
/* 19 */	"cursor",	tREP_CURSOR,	"replace cursor",
/* 20 */	"cursor",	tRET_CURSOR,	"retrieve cursor"
};

/* Dummy declaraction of the ANSI SQL2 keyword table - Only used
** by ESQL. VMS pulls in these declaractions because these are
** declared in eqscan.h.
*/
GLOBALDEF char   *tok_sql2key[1];
GLOBALDEF i4	 tok_sql2num;

/* %T footer end */
/*ERCHECK=ON*/
