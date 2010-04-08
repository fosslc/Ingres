/* Copyright (c) 2004 Ingres Corporation*/

/* %L merge <esqgtoks.mt> */

/* History:
**	13-dec-91 (swm)
**		Added missing tokens for "globalconstdef" and "globalconstref".
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
**	23-apr-93 (sandyd)
**		Added tCCONST and tCVOLATILE for ANSI C support.
**	15-sep-93 (sandyd)
**		Added tCVARBYTE (patterned after tCVARCHAR) for byte support.
**	07-mar-2001 (abbjo03)
**		Added wchar_t and nvarchar for Unicode support.
**      11-nov-2009 (joea)
**          Add tokens for C++'s (and C99) bool.
*/

/* the following are NOT copied to the output file, they are here for MING */
# include	<esqgtoks.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"csq.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"csq.h"
/* %L includes end */

/*
**	Do not use the 'ERx' macro on the quoted names in this file
**	because eqmerge (which merges this tokens file with the
**	ESQ tokens files) will not work.  Note that eqmerge reads
**	this file before the C preprocessor does.
*/
/* %T header - Get tokens header */

/*ERCHECK=OFF*/

/*
** put Language specific EXEC keywords here (see note 7 in mgram/esqgtoks.mt)
*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
{
		/* terminal */		/* token */
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
/* %L tokens */	"auto",			tCSTORAGE,
/* %L tokens */ "bool",                 tCTYPE_B,
/* %L tokens */	"char",			tCTYPE_C,
/* %L tokens */	"const",		tCCONST,
/* %L tokens */	"define",		tDEFINE,
/* %L tokens */	"double",		tCTYPE_D,
/* %L tokens */ "end",                  tEND,
/* %L tokens */	"enum",			tCENUM,
/* %L tokens */	"extern",		tCEXTERN,
/* %L tokens */	"float",		tCTYPE_F,
/* %L tokens */	"globaldef",		tCSTORAGE,	/* May need to ifdef */
/* %L tokens */	"globalref",		tCSTORAGE,
/* %L tokens */	"globalconstdef",	tCSTORAGE,
/* %L tokens */	"globalconstref",	tCSTORAGE,
/* %L tokens */ "include",              tINCLUDE,
/* %L tokens */	"int",			tCTYPE_I,
/* %L tokens */	"long",			tCTYPE_L,
/* %L tokens */	"nvarchar",		tCNVARCHAR,
/* %L tokens */ "register",		tREGISTER,
/* %L tokens */	"short",		tCTYPE_S,
/* %L tokens */	"static",		tCSTORAGE,
/* %L tokens */	"struct",		tCSTRUCT,
/* %L tokens */	"typedef",		tCTYPEDEF,
/* %L tokens */	"union",		tUNION,
/* %L tokens */	"unsigned",		tCUNSIGNED,
/* %L tokens */	"varbyte",		tCVARBYTE,
/* %L tokens */	"varchar",		tCVARCHAR,
/* %L tokens */	"volatile",		tCVOLATILE,
/* %L tokens */	"wchar_t",		tCTYPE_WC
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"\"",			tQUOTE,
/* %L ops */	"#",			tPOUND,
/* %L ops */	"'",			tQUOTE,
		"(",			tLPAREN,
/* %L ops */	"(*",			tCLPAR_STAR,
		")",			tRPAREN,
		"*",			tSTAR,
		"**",			tEXP,
		",",			tCOMMA,
/* %L ops */	"->",			tARROW,
		".",			tPERIOD,
		"/*",			tCOMMENT,
/* %L ops */	";",			tSEMICOL,
		"=",			tEQ,
/* %L ops */	"&",			tAMPERSAND,
/* %L ops */	"[",			tLBRKT,
/* %L ops */	"]",			tRBRKT,
/* %L ops */	"{",			tLBRACE,
/* %L ops */	"}",			tRBRACE
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
