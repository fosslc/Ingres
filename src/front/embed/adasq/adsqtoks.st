/*
** Copyright (c) 2004 Ingres Corporation
*/

/* %L merge <esqgtoks.mt> */

/* the following are NOT copied to the output file, they are here for MING */
# include	<esqgtoks.mt>
# include	<compat.h>
# include	<eqscan.h>
# include	"adasq.h"

/* %L includes begin  -- here are the REAL includes. */
/* %L includes */ # include	<compat.h>
/* %L includes */ # include	<eqscan.h>
/* %L includes */ # include	"adasq.h"
/* %L includes end */

/**
+*  Name: adasqtoks.st - ESQL/Ada token tables.
**
**  Description:
**	Define the ESQL/Ada token tables.
**
**  Defines:
**	ESQL/Ada token tables.
**
**  Notes:
**	Do NOT put ERx() any strings in this file and do NOT run ercheck
**	on it.
-*
**  History:
**	27-aug-1987	- Commented (mrw)
**	25-aug-1989	- Removed 'when' ada keyword because B1 changes made
**			  'when' a master grammar keyword. (teresal)
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
**	02-oct-1992 (larrym)
**		added slave tokens tEND, tFOR, tIS, tNULL, tOF.   These are 
**		also in the master token table, and so were not merged when
**		there was only one table.  Now that they are separate, they
**		need to be included.
**	11-feb-1993 (lan)
**		Added keyword SQL_STANDARD.
**	01-jun-1995 (wolf) 
**		Add "raise" to the EXEC mode keywords, although it's already in
**		the DECL mode list.  This eliminates syntax errors on the stmt
**		"EXEC SQL WHENEVER SQLERROR RAISE stmt_error". (69004)
**	19-oct-95 (angusm)
**		Add "in" and "out" to the ADA slave keywords table. Their
**	    absence causes formal parameter declarations within
**		EXEC SQL BEGIN/END DECLARE SECTION blocks to fail with
**		E_60003, E_EQ0244.
**	27-Jul-98 consi01 bug 88213 problem INGEMB 16
**		Added "when" to ADA slave keywords table to eliminate a
**		syntax error when declaring a variant record structure.
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
/* %L tokens */	"raise",		tARAISE
/* %L tokens end */
};

/*
** put language specific DECL mode keywords here
*/
GLOBALDEF KEY_ELM	 tok_skeytab[] =	
{
		/* terminal */		/* token */
/* %L tokens begin */
/* %L tokens */	"access",		tAACCESS,
/* %L tokens */	"array",		tAARRAY,
/* %L tokens */	"case",			tACASE,
/* %L tokens */	"constant",		tACONST,
/* %L tokens */	"delta",		tADELTA,
/* %L tokens */	"digits",		tADIGITS,
/* %L tokens */	"end",			tEND,
/* %L tokens */	"for",			tFOR,
/* %L tokens */	"function",		tAFUNCTION,
/* %L tokens */ "in"			tIN,
/* %L tokens */	"is",			tIS,
/* %L tokens */	"limited",		tALIMITED,
/* %L tokens */	"new",			tANEW,
/* %L tokens */	"null",			tANULL,
/* %L tokens */	"of",			tOF,
/* %L tokens */	"others",		tAOTHERS,
/* %L tokens */ "out",			tOUT,
/* %L tokens */	"package",		tAPACKAGE,
/* %L tokens */	"private",		tAPRIVATE,
/* %L tokens */	"raise",		tARAISE,
/* %L tokens */	"range",		tARANGE,
/* %L tokens */	"record",		tARECORD,
/* %L tokens */	"renames",		tARENAMES,
/* %L tokens */	"return",		tARETURN,
/* %L tokens */	"subtype",		tASUBTYPE,
/* %L tokens */ "sql_standard",		tASQL_STANDARD,
/* %L tokens */	"type",			tATYPEDEF,
/* %L tokens */ "when",                 tWHEN,
/* %L tokens end */
};

GLOBALDEF KEY_ELM	 tok_optab[] =
{
		/* terminal */		/* token */
/* %L ops begin */
/* %L ops */	"\"",			tQUOTE,
/* %L ops */	"'",			tQUOTE,
/* %L ops */	"..",			tADOTDOT,
/* %L ops */	";",			tSEMICOL,
/* %L ops */	"|",			tAPIPE,
/* %L ops */	":=",			tAASSIGN
/* %L ops */	"=>",			tAARROW,
/* %L ops */	"--",			tCOMMENT,
/* %L ops end */
};

/* %T footer - Get rest of file for testing */
/*ERCHECK=ON*/
