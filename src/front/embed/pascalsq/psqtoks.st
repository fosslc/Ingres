/*
** Copyright (c) 1987 - Ingres Corporation
*/


/* %L merge <esqgtoks.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<esqgtoks.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"pascalsq.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"pascalsq.h"
/* %L includes end */

/**
+*  Name: passqtoks.c - The ESQL/Pascal token tables.
**
**  Description:
**	ESQL/Pascal token tables.
**
**  Defines:
**	ESQL/Pascal token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	27-aug-1987	- Commented (mrw)
**      30-jul-92       - split tok_keytab into tok_keytab and tok_skeytab.
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
**      11-Apr-2001 (horda03) Bug 104463
**          The use of VAR <name> : [PACKED] SET OF CHAR := ...
**          was causing a syntax error. This is because SET isn't being
**          defined as a tok_skeytab token due to the "/* =MVS */" added to the
**          line.
**/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/

/*
** put language specific EXEC mode keywords here
*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"label",		tPLABEL,
/* %L tokens end */
};

/*
** put language specific DECL mode keywords here
*/
GLOBALDEF KEY_ELM	 tok_skeytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"array",		tPARRAY,
/* %L tokens */	"case",			tPCASE,
/* %L tokens */	"const",		tPCONST,
/* %L tokens */	"def",			tPVAR,		/* =MVS */
/* %L tokens */	"end",			tEND,
/* %L tokens */	"file",			tPFILE,
		"formint",		tFORMINT,
		"formpin",		tFORMPIN,
		"formpout",		tFORMPOUT,
		"formstring",		tFORMSTRING,
		"formvar",		tFORMVAR,
/* %L tokens */	"function",		tPFUNCTION,
/* %L tokens */	"label",		tPLABEL,
		"message",		tMESSAGE,
/* %L tokens */	"otherwise",		tPOTHERWISE,
/* %L tokens */	"of",			tOF,
/* %L tokens */	"packed",		tPPACKED,
		"param",		tPARAM,
		"queldot",		tQUELDOT,
		"quelint",		tQUELINT,
		"quelpin",		tQUELPIN,
		"quelpout",		tQUELPOUT,
		"quelstring",		tQUELSTRING,
		"quelvar",		tQUELVAR,
/* %L tokens */	"range",		tPRANGE,	/* =MVS */
/* %L tokens */	"record",		tPRECORD,
/* %L tokens */	"ref",			tPVAR,		/* =MVS */
/* %L tokens */	"set",			tSET,
/* %L tokens */	"static",		tPVAR,		/* =MVS */
/* %L tokens */	"type",			tPTYPEDEF,
/* %L tokens */	"var",			tPVAR,
/* %L tokens */	"varying",		tPVARYING
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"\"",			tQUOTE,
		"#",			tDEREF,
/* %L ops */	"'",			tQUOTE,
		"(",			tLPAREN,
		")",			tRPAREN,
		"*",			tMULOP,
		"**",			tEXPOP,
		",",			tCOMMA,
		".",			tPERIOD,
/* %L ops */	"..",			tPDOTDOT,
		"/*",			tCOMMENT,
/* %L ops */	"(*",			tCOMMENT,
/* %L ops */	"{",			tCOMMENT,
/* %L ops */	";",			tSEMICOL,
/* %L ops */	"[",			tLBRKT,
/* %L ops */	"]",			tRBRKT,
/* %L ops */	"^",			tPCARET,
/* %L ops */	"@",			tPCARET,	/* =MVS */
/* %L ops */	"::",			tPCAST,
/* %L ops */	"%",			tPPERCENT,
/* %L ops */	":=",			tPASSIGN
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
