/*
** Copyright (c) 1987, 2001 Ingres Corporation
*/


/* %L merge <esqgtoks.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<esqgtoks.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"basicsq.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"basicsq.h"
/* %L includes end */

/**
+*  Name: bassqtoks.st - ESQL/BASIC token tables.
**
**  Description:
**	Define the ESQL/BASIC token tables.
**
**  Defines:
**	ESQL/BASIC token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	28-aug-1987	- Commented (mrw)
**      30-jul-92 	- split tok_keytab into tok_keytab and tok_skeytab.  
**			  tok_keytab is for master (or EXEC mode) keywords, 
**			  and tok_skeytab is for slave (or DECL mode) keywords.
**			  This change was to facilitate usage of reserved 
**			  words in host identifiers.  Some words are duplicated
**			  in both tables.  Both tables are still "merged" with
**			  the "mgram" tokens file.  The scanner now checks
**			  dml->dm_exec to determine the table againts which to 
**			  match; IT IS IMPORTANT that you be in the correct 
**			  mode.  If you start getting syntax errors on your 
**			  reserved words, this is the first place at which to 
**			  look. (larrym)
**	 5-aug-92	- Added keywords DECLARE, END, and GROUP to slave toks.
**			  (larrym)
**	24-feb-2000 (kinte01)
**		Add support for SFLOAT, TFLOAT for AlphaVMS
**		SINGLE & DOUBLE are not supported AlphaVMS. (SIR 100394)
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
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
		/* string */		/* yacctoken */
/* %L tokens begin */
/* %L tokens */	"byte",			tBBYTE,
/* %L tokens */ "case",			tBCASE,
/* %L tokens */	"com",			tBCOMMON,
/* %L tokens */	"common",		tBCOMMON,
/* %L tokens */	"constant",		tBCONSTANT,
/* %L tokens */	"decimal",		tBDECIMAL,
/* %L tokens */	"declare",		tDECLARE,
/* %L tokens */	"dim",			tBDIM,
/* %L tokens */	"dimension",		tBDIM,
/* %L tokens */	"dynamic",		tBDYNAMIC,
/* %L tokens */	"end",			tEND,
/* %L tokens */	"external",		tBEXTERN,
/* %L tokens */	"group",		tGROUP,
/* %L tokens */	"integer",		tBINTEGER,
/* %L tokens */	"long",			tBLONG,
/* %L tokens */	"map",			tBMAP,
/* %L tokens */	"real",			tBREAL,
/* %L tokens */	"record",		tBRECORD,
/* %L tokens */	"string",		tBSTRING,
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
		",",			tCOMMA,
		"/*",			tCOMMENT,
/* %L ops */	"::",			tBDOUBCOLON,
/* %L ops */	";",			tTERMINATE,
		";",			tBSEMI
/* %L ops end */
};

/* %T footer - Get rest of file for testing */

/* 
** The rest of this file has been extracted by hand from
** gtokens.h for local testing during development.  Nothing from 
** here on down gets built into the ESQL/BASIC preprocessor; 
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
};

GLOBALDEF TOK_AHEAD	tok_lookahead[] =
{
	0,		0,		(char *)0,	(char *)0
};

/* %T footer end */
/*ERCHECK=ON*/
