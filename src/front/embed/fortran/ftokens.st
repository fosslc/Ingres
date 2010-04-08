/*
** Copyright (c) 2004 Ingres Corporation
*/


/* %L merge <gtokens.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<gtokens.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"fortran.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"fortran.h"
/* %L includes end */

/**
+*  Name: ftokens.st - EQUEL/FORTRAN token tables.
**
**  Description:
**	Defines the token tables.
**
**  Defines:
**	None.
**
**  Notes:
**	Don't run ercheck on this file, as all strings are explicitly
**	considered to be ERx()ed.  Don't put the ERx()s in.
-*
**  History:
**	26-aug-1987	- Commented (mrw)
**	04-sep-1990 (kathryn)
**		Removed /* =VMS */ from tok_keytab for the following tokens:
**              [ byte, record, structure, map, union ] as structures are now
**              supported on SUN F77.
**/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"byte",			tFTYPE,
/* %L tokens */	"character",		tFCHAR,
/* %L tokens */	"complex",		tFCOMPLEX,
/* %L tokens */	"declare",		tFDECLARE,
		"define",		tDEFINE,
/* %L tokens */	"double",		tFDOUBLE,
		"end",			tEND,
/* %L tokens */	"external",		tFEXTERN,
		"formint",		tFORMINT,
		"formpin",		tFORMPIN,
		"formpout",		tFORMPOUT,
		"formstring",		tFORMSTRING,
		"formvar",		tFORMVAR,
/* %L tokens */	"function",		tFFUNC,
/* %L tokens */	"integer",		tFTYPE,
/* %L tokens */	"logical",		tFTYPE,
/* %L tokens */	"map",			tFMAP,
		"message",		tMESSAGE,
		"param",		tPARAM,
/* %L tokens */	"parameter",		tFPARAM,
/* %L tokens */	"precision",		tFPRECISION,
/* %L tokens */	"program",		tFPROGRAM,
		"queldot",		tQUELDOT,
		"quelint",		tQUELINT,
		"quelpin",		tQUELPIN,
		"quelpout",		tQUELPOUT,
		"quelstring",		tQUELSTRING,
		"quelvar",		tQUELVAR,
/* %L tokens */	"real",			tFTYPE,
/* %L tokens */	"record",		tFRECORD,
/* %L tokens */	"structure",		tFSTRUCT,
/* %L tokens */	"subroutine",		tFPROC,
/* %L tokens */	"union",		tFUNION
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"!",			tCOMMENT,
/* %L ops */	"\"",			tQUOTE,
		"#",			tDEREF,
/* %L ops */	"'",			tQUOTE,
		"(",			tLPAREN,
		")",			tRPAREN,
		"*",			tMULOP,
		",",			tCOMMA,
		".",			tPERIOD,
		"/",			tDIVOP,
		"/*",			tCOMMENT,
		"=",			tEQOP,
/* %L ops */	"{",			tLBRACE,
/* %L ops */	"}",			tRBRACE,
/* %L ops */	";",			tFSEMI
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
