/*
** Copyright (c) 2004 Ingres Corporation
*/


/* %L merge <gtokens.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<gtokens.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"cobol.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"cobol.h"
/* %L includes end */

/**
+*  Name: cobtokens.c - EQUEL/COBOL token tables.
**
**  Description:
**	Define the EQUEL/COBOL token tables.
**
**  Defines:
**	EQUEL/COBOL token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	27-aug-1987	- Commented (mrw)
**	18-jul-1990	- Add 'PACKED-DECIMAL' as a valid usage. (teresal) 
**	4-dec-95 (angusm)
**		Add LEFT, RIGHT to slave grammar (bug 72792)
**	7-dec-1995 (angusm)
**		Add BINARY - synonym for COMP (bug 72980)
**	9-dec-1995 (hanch04)
**		LEFT, RIGHT not valid for QUEL
**/

/* %T header - Get tokens header */

/*ERCHECK=OFF*/
GLOBALDEF KEY_ELM	 tok_keytab[] =	
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
/* %L tokens */	"declare",		tCBDECLARE,
/* %L tokens */	"DEPENDING",		tCBDEPENDING,
/* %L tokens */	"DESCENDING",		tCBDIREC,
/* %L tokens */	"DISPLAY",		tDISPLAY,	
/* %L tokens */	"EXTERNAL",		tCBEXTERNAL,
/* %L tokens */	"FILLER",		tCBFILLER,		
		"formint",		tFORMINT,
		"formstring",		tFORMSTRING,
		"formvar",		tFORMVAR,
/* %L tokens */	"GLOBAL",		tCBGLOBAL,		
/* %L tokens */	"IN",			tIN,		
/* %L tokens */	"INDEX",		tINDEX,		
/* %L tokens */	"INDEXED",		tCBINDEXED,	
/* %L tokens */	"IS",			tIS,		
/* %L tokens */	"JUST",			tCBJUSTIFIED,		
/* %L tokens */	"JUSTIFIED",		tCBJUSTIFIED,		
/* %L tokens */	"KEY",			tCBKEY,		
/* %L tokens */	"LEADING",		tCBLEAD_TRAIL,	
/* %L tokens */	"OCCURS",		tCBOCCURS,		
/* %L tokens */	"OF",			tOF,		
/* %L tokens */	"ON",			tON,		
/* %L tokens */	"PACKED-DECIMAL",	tCBTYPE,
		"param",		tPARAM,
/* %L tokens */	"PIC",			tCBPICTURE,		
/* %L tokens */	"PICTURE",		tCBPICTURE,		
/* %L tokens */	"POINTER",		tCBBADTYPE,		
		"queldot",		tQUELDOT,
		"quelint",		tQUELINT,
		"quelstring",		tQUELSTRING,
		"quelvar",		tQUELVAR,
/* %L tokens */	"REDEFINES",		tCBREDEFINES,	
/* %L tokens */	"REFERENCE",		tCBREFERENCE,
/* %L tokens */	"SEPARATE",		tCBSEPARATE,	
/* %L tokens */	"SIGN",			tCBSIGN,		
/* %L tokens */	"SYNC",			tCBSYNC,		
/* %L tokens */	"SYNCHRONIZED",		tCBSYNC,		
/* %L tokens */	"TIMES",		tCBTIMES,		
/* %L tokens */	"TO",			tTO,		
/* %L tokens */	"TRAILING",		tCBLEAD_TRAIL,	
/* %L tokens */	"USAGE",		tCBUSAGE,		
		"useinparam",		tUSEINPARAM,
		"useoutparam",		tUSEOUTPARAM,
/* %L tokens */	"VALUE",		tCBVALUE,		
/* %L tokens */	"WHEN",			tCBWHEN,		
/* %L tokens */	"ZERO",			tCBZERO,		
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
		",",			tCOMMA,
		".",			tPERIOD,
		"/*",			tCOMMENT,
		"=",			tIS,
/* %L ops */	"{",			tLBRACE,
/* %L ops */	"}",			tRBRACE
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
