/*
** Copyright (c) 1987, 2001 Ingres Corporation
*/


/* %L merge <gtokens.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<gtokens.mt>  
# include	<compat.h>
# include	<eqscan.h>
# include	"basic.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"basic.h"
/* %L includes end */

/**
+*  Name: btokens.st - EQUEL/BASIC token tables.
**
**  Description:
**	Define the EQUEL/BASIC token tables.
**
**  Defines:
**	EQUEL/BASIC token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	28-aug-1987	- Commented (mrw)
**	24-feb-2000 (kinte01)
**		Add support for SFLOAT, TFLOAT for AlphaVMS
**		SINGLE & DOUBLE are not supported AlphaVMS. (SIR 100394)
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
{
		/* string */		/* yacctoken */
/* %L tokens begin */
		"by",			tBY,
/* %L tokens */	"byte",			tBBYTE,
/* %L tokens */ "case",			tBCASE,
/* %L tokens */	"com",			tBCOMMON,
/* %L tokens */	"common",		tBCOMMON,
/* %L tokens */	"constant",		tBCONSTANT,
/* %L tokens */	"decimal",		tBDECIMAL,
/* %L tokens */	"declare",		tBDECLARE,
/* %L tokens */	"def",			tBDEF,
/* %L tokens */	"dim",			tBDIM,
/* %L tokens */	"dimension",		tBDIM,
/* %L tokens */	"dynamic",		tBDYNAMIC,
		"end",			tEND,
/* %L tokens */	"external",		tBEXTERN,
/* %L tokens */	"fnend",		tBFNEND,
/* %L tokens */	"function",		tBFUNCTION,
/* %L tokens */	"functionend",		tBFUNCEND,
/* %L tokens */	"group",		tBGROUP,
		"ingres",		tINGRES,
/* %L tokens */	"integer",		tBINTEGER,
/* %L tokens */	"long",			tBLONG,
/* %L tokens */	"map",			tBMAP,
		"param",		tPARAM,
/* %L tokens */	"real",			tBREAL,
/* %L tokens */	"record",		tBRECORD,
/* %L tokens */	"string",		tBSTRING,
/* %L tokens */	"sub",			tBSUB,
/* %L tokens */	"subend",		tBSUBEND,
		"use",			tUSE,
/* %L tokens */	"variant",		tBVARIANT,
/* %L tokens */	"sfloat",		tBSFLOAT,
/* %L tokens */	"tfloat",		tBTFLOAT,
/* %L tokens */	"double",		tBDOUBLE,
/* %L tokens */	"single",		tBSINGLE,
/* %L tokens */	"word",			tBWORD
/* %L tokens end */
		
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"!",			tCOMMENT,
/* %L ops */	"\"",			tQUOTE,
/* %L ops */	"'",			tQUOTE,
		"(",			tLPAREN,
		")",			tRPAREN,
		"*",			tMULOP,
		"+",			tARUOP,
		",",			tCOMMA,
		"-",			tARUOP,
		"/*",			tCOMMENT,
/* %L ops */	"::",			tDOUBCOLON,
/* %L ops */	";",			tSEMICOL,
/* %L ops */	"{",			tLBRACE,
/* %L ops */	"}",			tRBRACE
/* %L ops end */
};

/* %T footer - Get rest of file for testing */

/* 
** The rest of this file has been extracted by hand from
** gtokens.h for local testing during development.  Nothing from 
** here on down gets built into the EQUEL/BASIC preprocessor; 
** instead the eqmerger takes the equivalent data structures from 
** gtokens.c.  Explanations and comments are found in gtokens, too.
*/

GLOBALDEF i4	tok_kynum = sizeof( tok_keytab ) / sizeof( KEY_ELM );
GLOBALDEF i4	tok_opnum = sizeof( tok_optab ) / sizeof( KEY_ELM );
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
};
GLOBALDEF TOK_AHEAD	tok_lookahead[] =
{
	0,		0,		(char *)0,	(char *)0
};
/*ERCHECK=ON*/
