/*
** Copyright (c) 2004 Ingres Corporation
*/


/* %L merge <esqgtoks.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<esqgtoks.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"cobolsq.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"cobolsq.h"
/* %L includes end */

/**
+*  Name: cobsqtoks.st - ESQL/COBOL token tables.
**
**  Description:
**	Define the ESQL/COBOL token tables.
**
**  Defines:
**	ESQL/COBOL token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	27-aug-1987	- Commented (mrw)
**      28-aug-1989     - 'WHEN' COBOL keyword became a master grammar keyword
**			  because of B1 changes. (teresal)
**	18-jul-1990	- Add 'PACKED-DECIMAL' as valid usage. (teresal)
**	02-jul-1992	- Cobol keyword 'GLOBAL' became a master grammar
**			  keyword.  The change is needed for adding the
**			  "DECLARE GLOBAL TEMPORARY TABLE" statement. (lan)
**      30-jul-1992	- split tok_keytab into tok_keytab and tok_skeytab.
**                        tok_keytab is for master (or EXEC mode) keywords,
**                        and tok_skeytab is for slave (or DECL mode) keywords.
**                        This change was to facilitate usage of reserved
**                        words in host identifiers.  Some words are duplicated
**                        in both tables.  Both tables are still "merged" with
**                        the "mgram" tokens file.  The scanner now checks
**                        dml->dm_exec to determine the table againts which to
**                        match; IT IS IMPORTANT that you be in the correct
**                        mode.  If you start getting syntax errors on your
**                        reserved words, this is the first place at which to
**                        look. (larrym)
**	26-aug-1992	- Cobol keyword 'KEY' became a master grammar
**			  keyword. (lan)
<<<< THEIR VERSION
<<<< THEIR VERSION
**	4-dec-1995 (angusm)
**		LEFT and RIGHT omitted from slave grammar after split. (bug 72792)
**	7-dec-1995 (angusm)
**		BINARY (synonym for COMP) needed for SQL-92 compliance (bug 72980)
====
**	22-aug-1995	- Added source listing format control keywords of
**			  EJECT, SKIP1, SKIP2, SKIP3 (thoda04)
>>>> YOUR VERSION
====
>>>> YOUR VERSION
**/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/
/*
** put language specific EXEC mode keywords here
*/
GLOBALDEF KEY_ELM        tok_keytab[] =
{
                /* terminal */          /* token */
/* %L tokens begin */
/* %L tokens end */
};

/*
** put language specific DECL mode keywords here
*/
GLOBALDEF KEY_ELM	 tok_skeytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"ASCENDING",		tCBDIREC,
/* %L tokens */ "BINARY", 		tCBTYPE,
/* %L tokens */	"BLANK",		tCBBLANK,
/* %L tokens */	"BY",			tBY,		
/* %L tokens */	"CHARACTER",		tCBCHARACTER,
/* %L tokens */	"COMP",			tCBTYPE,
/* %L tokens */	"COMP-1",		tCBTYPE,
/* %L tokens */	"COMP-2",		tCBTYPE,
/* %L tokens */	"COMP-3",		tCBTYPE,
/* %L tokens */	"COMP-4",		tCBTYPE,
/* %L tokens */	"COMP-5",		tCBTYPE,
/* %L tokens */	"COMP-6",		tCBTYPE,
/* %L tokens */	"COMPUTATIONAL",	tCBTYPE,
/* %L tokens */ "COMPUTATIONAL-1",	tCBTYPE,
/* %L tokens */	"COMPUTATIONAL-2",	tCBTYPE,
/* %L tokens */	"COMPUTATIONAL-3",	tCBTYPE,
/* %L tokens */	"COMPUTATIONAL-4",	tCBTYPE,
/* %L tokens */	"COMPUTATIONAL-5",	tCBTYPE,
/* %L tokens */	"COMPUTATIONAL-6",	tCBTYPE,
/* %L tokens */	"DEPENDING",		tCBDEPENDING,
/* %L tokens */	"DESCENDING",		tCBDIREC,
/* %L tokens */	"DISPLAY",		tDISPLAY,	
/* %L tokens */	"END-EXEC",		tEND_EXEC,	
/* %L tokens */	"EXTERNAL",		tCBEXTERNAL,
/* %L tokens */	"FILLER",		tCBFILLER,		
/* %L tokens */	"GLOBAL",		tGLOBAL,		
/* %L tokens */	"IN",			tIN,		
/* %L tokens */	"INDEX",		tINDEX,		
/* %L tokens */	"INDEXED",		tCBINDEXED,	
/* %L tokens */	"IS",			tIS,		
/* %L tokens */	"JUST",			tCBJUSTIFIED,		
/* %L tokens */	"JUSTIFIED",		tCBJUSTIFIED,		
/* %L tokens */	"KEY",			tKEY,		
/* %L tokens */	"LEADING",		tCBLEAD_TRAIL,	
/* %L tokens */ "LEFT",			tLEFT,
/* %L tokens */	"OCCURS",		tCBOCCURS,		
/* %L tokens */	"OF",			tOF,		
/* %L tokens */	"ON",			tON,		
/* %L tokens */	"PACKED-DECIMAL",	tCBTYPE,
/* %L tokens */	"PIC",			tCBPICTURE,		
/* %L tokens */	"PICTURE",		tCBPICTURE,		
/* %L tokens */	"POINTER",		tCBBADTYPE,		
/* %L tokens */	"REDEFINES",		tCBREDEFINES,	
/* %L tokens */	"REFERENCE",		tCBREFERENCE,
/* %L tokens */ "RIGHT",			tRIGHT,
/* %L tokens */	"SEPARATE",		tCBSEPARATE,	
/* %L tokens */	"SIGN",			tCBSIGN,		
/* %L tokens */	"SYNC",			tCBSYNC,		
/* %L tokens */	"SYNCHRONIZED",		tCBSYNC,		
/* %L tokens */	"TIMES",		tCBTIMES,		
/* %L tokens */	"TO",			tTO,		
/* %L tokens */	"TRAILING",		tCBLEAD_TRAIL,	
/* %L tokens */	"USAGE",		tCBUSAGE,		
/* %L tokens */	"VALUE",		tCBVALUE,		
/* %L tokens */ "WHEN",                 tWHEN,
/* %L tokens */	"ZERO",			tCBZERO,		
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"\"",			tQUOTE,
/* %L ops */	"'",			tQUOTE,
		"(",			tLPAREN,
		")",			tRPAREN,
		",",			tCOMMA,
		".",			tPERIOD,
		"/*",			tCOMMENT,
/* %L ops */	";",			tSEMICOL,
		"=",			tIS,
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
