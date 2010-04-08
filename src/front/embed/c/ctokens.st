/* Copyright (c) 2004 Ingres Corporation*/
/* %L merge <gtokens.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<gtokens.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"c.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"c.h"
/* %L includes end */

/*
**	Do not use the 'ERx' macro on the quoted names in this file
**	because eqmerge (which merges this tokens file with the
**	EQ tokens files) will not work.  Note that eqmerge reads
**	this file before the C preprocessor does.
*/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"auto",			tCSTORAGE,
/* %L tokens */	"char",			tCTYPE_C,
/* %L tokens */	"define",		tDEFINE,
/* %L tokens */	"double",		tCTYPE_D,
/* %L tokens */	"enum",			tCENUM,
/* %L tokens */	"extern",		tCEXTERN,
/* %L tokens */	"float",		tCTYPE_F,
		"formint",		tFORMINT,
		"formpin",		tFORMPIN,
		"formpout",		tFORMPOUT,
		"formstring",		tFORMSTRING,
		"formvar",		tFORMVAR,

/* %L tokens */	"globaldef",		tCSTORAGE,	/* May need to ifdef */
/* %L tokens */	"globalref",		tCSTORAGE,
/* %L tokens */	"globalconstdef",	tCSTORAGE,
/* %L tokens */	"globalconstref",	tCSTORAGE,

/* %L tokens */	"int",			tCTYPE_I,
/* %L tokens */	"long",			tCTYPE_L,
		"message",		tMESSAGE,
		"param",		tPARAM,
		"queldot",		tQUELDOT,
		"quelint",		tQUELINT,
		"quelpin",		tQUELPIN,
		"quelpout",		tQUELPOUT,
		"quelstring",		tQUELSTRING,
		"quelvar",		tQUELVAR,
		"register",		tREGISTER
/* %L tokens */	"short",		tCTYPE_S,
/* %L tokens */	"static",		tCSTORAGE,
/* %L tokens */	"struct",		tCSTRUCT,
/* %L tokens */	"typedef",		tCTYPEDEF,
/* %L tokens */	"union",		tCSTRUCT,
/* %L tokens */	"unsigned",		tCUNSIGNED,
/* %L tokens */ "varchar",		tCVARCHAR
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
/* %L ops */	"(*",			tCLPAR_STAR,
		")",			tRPAREN,
		"*",			tMULOP,
		"**",			tEXPOP,
		",",			tCOMMA,
/* %L ops */	"->",			tARROW,
		".",			tPERIOD,
		"/*",			tCOMMENT,
/* %L ops */	";",			tSEMICOL,
		"=",			tEQOP,
/* %L ops */	"[",			tLBRKT,
/* %L ops */	"]",			tRBRKT,
/* %L ops */	"{",			tLBRACE,
/* %L ops */	"}",			tRBRACE
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
