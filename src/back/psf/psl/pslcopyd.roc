/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <cm.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <pslcopyd.h>

/**
**
**  Name: PSLCOPYD.C - Functions for handling delimiters for the "COPY" command
**
**  Description:
**      This file contains the functions for handling delimiters for the "COPY"
**	command.
**
**          psl_copydelim - Find the one character copy delimiter for a given
**		delimiter name (i.e. "comma" is ',').
**
**  History:    $Log-for RCS$
**      12-nov-86 (rogerk)
**          Written
**      05-nov-1992 (rog)
**          Include ulf.h and scf.h before qsf.h.
**      16-sep-93 (swm)
**          Added <cs.h> for CS_SID.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/


/*}
** Name: PSL_CPDELIMTAB - Table of copy delimiters and their names
**
** Description:
**      This structure is used for looking up copy delimiters given their
**	names.
**
** History:
**      12-nov-86 (rogerk)
**          Adapted from typespec.c in 4.0.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-mar-1996 (kch)
**	    Added uppercase copy delimiter names (NL, COMMA etc) to allow
**	    the correct look up of copy delimiters for dummy copy columns
**	    in an SQL-92 environment. This change fixes bug 73320.
**	9-Dec-2009 (kschendel) SIR 122974
**	    Add pseudo-delimiters csv, ssv.
*/
typedef struct _PSL_CPDELIMTAB
{
    char	*cp_delstring;		/* delimiter name */
    char	*cp_delchar;		/* delimiter character */
} PSL_CPDELIMTAB;

/*
**  Definition of static variables and forward static functions.
*/

static  const PSL_CPDELIMTAB Copy_delim_tab[] =   /* Table of copy delims */
{
    "nl",		"\n",
    "sp",		" ",
    "tab",		"\t",
    "nul",		"\0",
    "null",		"\0",
    "comma",		",",
    "colon",		":",
    "csv",		"\001",
    "dash",		"-",
    "lparen",		"(",
    "rparen",		")",
    "quote",		"\"",
    "ssv",		"\002",
    "NL",		"\n",
    "SP",		" ",
    "TAB",		"\t",
    "NUL",		"\0",
    "NULL",		"\0",
    "COMMA",		",",
    "COLON",		":",
    "CSV",		"\001",
    "DASH",		"-",
    "LPAREN",		"(",
    "RPAREN",		")",
    "QUOTE",		"\"",
    "SSV",		"\002",
    NULL
};

/*{
** Name: psl_copydelim	- Find the copy delimiter given its name
**
** Description:
**      This function takes a null-terminated string representing a copy
**	delimiter, and finds the actual delimiter.  If the string does
**	not represent a valid delimiter, then E_DB_ERROR is returned.
**
**	A valid copy delimiter is a single alphabetic character or a name
**	listed in the "Copy_delim_tab" table.
**
** Inputs:
**      delimstring                     Delimiter name
**	delimchar			Place to put delimiter (a char)
**
** Outputs:
**      delimchar                       Filled in with the delimiter
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Illegal delimiter name
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jul-86 (rogerk)
**          written
*/
DB_STATUS
psl_copydelim(
	char               *delimname,
	char		   **delimchar)
{
    i4		i;
    i4		len;

    len = STlength(delimname);

    if ((len == 1) || ((len ==2) && (CMdbl1st(delimname))))
    {
	if ( ! CMprint(delimname))
	    return (E_DB_ERROR);

	*delimchar = delimname;
	return (E_DB_OK);
    }
    else
    {
	/* Check if this is a legal delimiter name */

	for (i = 0; Copy_delim_tab[i].cp_delstring != NULL; i++)
	{
	    if (!STcompare(delimname, Copy_delim_tab[i].cp_delstring))
	    {
    		*delimchar = Copy_delim_tab[i].cp_delchar;
		return (E_DB_OK);
	    }
	}

	return (E_DB_ERROR);
    }
}
