/*
** Name:	g4globs.h 
**
** Description:
**	Global data used by the EXEC 4GL interface
**
** History:
**	16-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include <g4funcs.h>

# define CLEAR_ERROR (iiAG4errno = 0, *iiAG4errtext = EOS)

/* Global error state */
GLOBALREF ER_MSGID      iiAG4errno;
GLOBALREF char		iiAG4errtext[ER_MAX_LEN];

/* Saved, validated object for set and get attribute */
GLOBALREF PTR		iiAG4savedObject;
GLOBALREF i4		 iiAG4savedAccess;

/* array of routine names, indexed by caller ID */
GLOBALREF  char *iiAG4routineNames[];

/* array of INQUIRE_4GL and SET_4GL types */
GLOBALREF char *iiAG4inqtypes[];
GLOBALREF char *iiAG4settypes[];

/* Current message reporting status */
GLOBALREF bool iiAG4showMessages;

