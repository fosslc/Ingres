/*
** Copyright (c) 2004 Ingres Corporation
*/

/* the following are NOT copied to the output file, they are here for MING */
# include	<compat.h>
# include	<eqscan.h>
# include	"ada.h"

/* %L merge <gtokens.mt> */
#include <gtokens.mt>  /* fool MING. this line not copied to output. */

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"ada.h"
/* %L includes end */


/**
+*  Name: adatokens.st - EQUEL/Ada token tables.
**
**  Description:
**	Define the EQUEL/Ada token tables.
**
**  Defines:
**	EQUEL/Ada token tables.
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
/* %L tokens */	"access",		tAACCESS,
/* %L tokens */	"array",		tAARRAY,
/* %L tokens */	"begin",		tABEGIN,
/* %L tokens */	"body",			tABODY,
/* %L tokens */	"case",			tACASE,
/* %L tokens */	"constant",		tACONST,
/* %L tokens */	"declare",		tADECLARE,
/* %L tokens */	"delta",		tADELTA,
/* %L tokens */	"digits",		tADIGITS,
		"for",			tFOR,
/* %L tokens */	"function",		tAFUNCTION,
		"is",			tIS,
/* %L tokens */	"limited",		tALIMITED,
/* %L tokens */	"new",			tANEW,
		"null",			tNULL,
/* %L tokens */	"others",		tAOTHERS,
/* %L tokens */	"package",		tAPACKAGE,
/* %L tokens */	"private",		tAPRIVATE,
/* %L tokens */	"procedure",		tAPROCEDURE,
/* %L tokens */	"record",		tARECORD,
/* %L tokens */	"renames",		tARENAMES,
/* %L tokens */	"return",		tARETURN,
/* %L tokens */	"separate",		tASEPARATE,
/* %L tokens */	"subtype",		tASUBTYPE,
/* %L tokens */	"type",			tATYPEDEF,
/* %L tokens */	"when",			tAWHEN,
/* %L tokens */	"use",			tAUSE
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
		".",			tAERIOD,
/* %L ops */	"..",			tADOTDOT,
		"/*",			tCOMMENT,
/* %L ops */	"/=",			tNEQOP,
/* %L ops */	";",			tSEMICOL,
		"=",			tEQOP,
/* %L ops */	"|",			tAPIPE,
/* %L ops */	"{",			tLBRACE,
/* %L ops */	"}",			tRBRACE,
/* %L ops */	":=",			tAASSIGN
/* %L ops */	"=>",			tAARROW,
/* %L ops */	"--",			tCOMMENT,
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
