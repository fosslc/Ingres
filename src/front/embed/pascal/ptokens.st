/*
** Copyright (c) 1987 - Ingres Corporation
*/


/* %L merge <gtokens.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<gtokens.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"pascal.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"pascal.h"
/* %L includes end */

/**
+*  Name: ptokens.st - EQUEL/Pascal token tables.
**
**  Description:
**	Define the EQUEL/Pascal token tables.
**
**  Defines:
**	EQUEL/Pascal token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	27-aug-1987	- Commented (mrw)
**/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"array",		tPARRAY,
/* %L tokens */	"begin",		tBEGIN,
/* %L tokens */	"case",			tPCASE,
/* %L tokens */	"const",		tPCONST,
/* %L tokens */	"declare",		tPDECLARE,
/* %L tokens */	"def",			tPVAR,		/* =MVS */
/* %L tokens */	"do",			tPDO,
/* %L tokens */	"extern",		tPEXTERN,
/* %L tokens */	"external",		tPEXTERN,
/* %L tokens */	"file",			tPFILE,
		"formint",		tFORMINT,
		"formpin",		tFORMPIN,
		"formpout",		tFORMPOUT,
		"formstring",		tFORMSTRING,
		"formvar",		tFORMVAR,
/* %L tokens */	"fortran",		tPFORTRAN,
/* %L tokens */	"forward",		tPFORWARD,
/* %L tokens */	"function",		tPFUNCTION,
/* %L tokens */	"label",		tPLABEL,
/* %L tokens */	"main",			tPFORWARD,	/* =MVS */
		"message",		tMESSAGE,
/* %L tokens */	"module",		tPMODULE,
/* %L tokens */	"otherwise",		tPOTHERWISE,
/* %L tokens */	"packed",		tPPACKED,
		"param",		tPARAM,
/* %L tokens */	"procedure",		tPPROCEDURE,
/* %L tokens */	"program",		tPPROGRAM,
		"queldot",		tQUELDOT,
		"quelint",		tQUELINT,
		"quelpin",		tQUELPIN,
		"quelpout",		tQUELPOUT,
		"quelstring",		tQUELSTRING,
		"quelvar",		tQUELVAR,
/* %L tokens */	"record",		tPRECORD,
/* %L tokens */	"reentrant",		tPFORWARD,	/* =MVS */
/* %L tokens */	"ref",			tPVAR,		/* =MVS */
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
		"=",			tEQOP,
/* %L ops */	"[",			tLBRKT,
/* %L ops */	"]",			tRBRKT,
/* %L ops */	"^",			tPCARET,
/* %L ops */	"::",			tPCAST,
/* %L ops */	"%",			tPPERCENT,
/* %L ops */	":=",			tPASSIGN
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
