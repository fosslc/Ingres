/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<erg4.h>
# include	"g4globs.h"


/*
** Name:	g4globs.c
**
** Description:
**	Global data used by the EXEC 4GL interface
**
** History:
**	16-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	22-jan-92 (sandyd)
**		Added comma after "SET_4GL" initializer in iiAG4routineNames[]
**		to make VMS compiler happy.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* Global error state */
GLOBALDEF ER_MSGID	iiAG4errno;
GLOBALDEF char		iiAG4errtext[ER_MAX_LEN];

/* Saved, validated object for set and get attribute */
GLOBALDEF PTR iiAG4savedObject = (PTR)NULL;
GLOBALDEF i4	 iiAG4savedAccess;

/* Array of routine names, indexed by caller IDs */
GLOBALDEF  char *iiAG4routineNames[] =
{
	"",
	"GET ATTRIBUTE",
	"SET ATTRIBUTE",
	"CLEAR ARRAY",
	"SETROW DELETED",
	"GETROW",
	"INSERTROW",
	"REMOVEROW",
	"SETROW",
	"CALLPROC",
	"CALLFRAME",
	"GET GLOBAL CONSTANT",
	"GET GLOBAL VARIABLE",
	"SET GLOBAL VARIABLE",
	"DESCRIBE",
	"INQUIRE_4GL",
	"SET_4GL",
	"SEND USEREVENT"
};

/* Array of INQUIRE_4GL and SET_4GL type names */
GLOBALDEF char *iiAG4inqtypes[] =
{
    ERx("errortext"),
    ERx("errorno"),
    ERx("allrows"),
    ERx("lasrrow"),
    ERx("firstrow"),
    ERx("isarray"),
    ERx("classname")
};

GLOBALDEF char *iiAG4settypes[] =
{	
    ERx("messages")
};

/* Current message reporting status */
GLOBALDEF bool iiAG4showMessages = TRUE;
