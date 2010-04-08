/*
** Copyright (c) 2004 Ingres Corporation
*/


/* %L merge <esqgtoks.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<esqgtoks.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"forsq.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"forsq.h"
/* %L includes end */

/**
+*  Name: forsqtoks.c - FORTRAN/SQL token tables.
**
**  Description:
**	Defines the token tables.
**
**  Defines:
**	None.
**
**  Notes:
**	Don't run ercheck on this, and don't ERx() the strings --
**	they all implicitly are ERx()ed.
-*
**  History:
**	26-aug-1987	- Commented (mrw)
**	04-sep-1990 (kathryn)
**		Removed /* = VMS */ front tok_keytab for the following tokens:
**		[ byte, record, structure, map ] as structures are now 
**		supported on SUN F77.
**      30-jul-1992	 - split tok_keytab into tok_keytab and tok_skeytab.
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
**	22-Jun-1999 (andbr08)
**		Added the union token so that the fortran union/structure declaration
**		is parsed correctly.
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
 		"begin",		tBEGIN,
/* %L tokens */	"byte",			tFBYTE,
/* %L tokens */	"character",		tFCHAR,
/* %L tokens */	"complex",		tFCOMPLEX,
/* %L tokens */	"double",		tFDOUBLE,
/* %L tokens */	"end",			tEND,
/* %L tokens */	"include",		tINCLUDE,
/* %L tokens */	"integer",		tFINTEGER,
/* %L tokens */	"logical",		tFLOGICAL,
/* %L tokens */	"map",			tFMAP,
/* %L tokens */ "parameter",		tFPARAM,
/* %L tokens */	"precision",		tFPRECISION,
/* %L tokens */	"real",			tFREAL,
/* %L tokens */	"record",		tFRECORD,
/* %L tokens */	"section",		tSECTION,
/* %L tokens */	"structure",		tFSTRUCT,
		"table",		tTABLE,
/* %L tokens */	"union",		tUNION,
               	"use",			tFUSE
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"!",			tCOMMENT,
/* %L ops */	"\"",			tQUOTE,
/* %L ops */	"%",			tFPERCENT,
/* %L ops */	"'",			tQUOTE,
		"(",			tLPAREN,
		")",			tRPAREN,
		",",			tCOMMA,
		"/",			tDIVOP,
		"/*",			tCOMMENT,
		":",			tCOLON,
/* %L ops */	";",			tTERMINATE,
/* %L ops */	"<",			tLT
/* %L ops end */
};

/* %T footer - Get rest of file for testing */


/*ERCHECK=ON*/
