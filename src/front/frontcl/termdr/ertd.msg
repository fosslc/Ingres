/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ertd.msg -	Message file for class TD
**
** Description:
**	This file defines the messages for class TD, the "termdr"
**	directory in the "frontcl" facility.
**
**	Do NOT move the error numbers around in this file.  The
**	numbers have been set up to be in numerical order for
**	faster compiling.  This is worth the cost of having
**	error numbers for the same file being split up in
**	this file.  One can always search for the name of the
**	file to gather all error numbers for a particular file.
**
** History:
**	18-aug-1987 (dave)
**		Created.
**	08/28/87 (dkh) - Added error numbers that used to be in the 8000 series.
**	11/12/87 (dkh) - Fixed typos.
**	12/17/87 (dkh) - Fixed jup bug 1632.
**	13-apr-88 (bruceb)
**		Fixed typo: termap => termcap in E_TD001D.
**	28-apr-88 (bruceb)
**		Added E_TD0021 as DG alternative to E_TD001D.
**	01/12/90 (dkh) - Added message E_TD0022.
**	23-oct-1990 (mgw)
**		Corrected a misspelling in a message.
**	02-may-97 (mcgem01)
**		Removed hard-coded references to TERM_INGRES.
**/

# define	_TD_CLASS	66

/*
**	itin.c
*/
E_TD0001_Buffer_overflow	"Buffer overflow on push back"	/* DIAG */
/*
**	keys.c
*/
F_TD0001_Error_occured_in_line	"Error occurred in line '%d'"
F_TD0002_Line_was	"Line was \"%s\""
E_TD0002_Cyclic_key_definition	"Cyclic key definitions"	/* DIAG */
/*
**	termcap.c - reads the termcap file.
*/
E_TD0003_error_opening	"TERMCAP: error(%d) in opening %s\n" /* DIAG */
E_TD0004_Termcap_entry_long	"Termcap entry too long\n" /* DIAG */
E_TD0005_Bad_termcap_entry	"Bad termcap entry\n" /* DIAG */
E_TD0006_Infinite_tc_loop	"Infinite tc= loop\n" /* DIAG */
/*
**	keys.c - parses mapping files, etc.
*/
E_TD0007_OPEN_ERR	"Can't open file to output function key
definition error messages."
E_TD0008_BADHAT	"Invalid character after the `^' character."
E_TD0009_BADBSQ	"Back backslash ('\') sequence."
E_TD000A_AFBSQ	"No character found after a backslash ('\')."
E_TD000B_NONUM	"No digits following a `#' character."
E_TD000C_BADNUM	"A function key number must be identified by a
2 digit sequence."
E_TD000D_NUMCONV	"An error occurred while converting a key number
from its character representation."
E_TD000E_NOKEY	"Function key `%0d' does not exist for the
terminal you are using."
E_TD000F_NOALW	"Function key `%0d' may not be use in a definition statement."
E_TD0010_KEYSELF	"May not define a function key that references itself."
E_TD0011_NOPFK	"Your terminal description does not support function keys."
E_TD0012_FIRSTCH	"First character on a line must be the `#' character."
E_TD0013_DIGITS	"Function keys must be identified by a 1 or 2 digit
character sequence."
E_TD0014_EQUAL	"You must use the equal character ('=') to assign a
definition to a function key."
E_TD0015_KEYRES	"Function keys `%0d' and `%1d' are reserved and definitions
be not be assigned to them."
E_TD0016_INSOVFL	"Buffer overflow when installing expanded definition."
E_TD0017_BNUMEX	"An unknown function key number was encountered on
macro expansion."
E_TD0018_OVFL	"Buffer overflow during definition expansion."
E_TD0019_REFDEF	"Definition references a function key with no definition."
E_TD001A_NULLEXP	"Result of a definition expansion was the null string."
E_TD001B_AFHAT	"No character found after the `^' character."

/*
**	initscr.c
*/
E_TD001C_NOTERM
"Cannot determine user's terminal type.\n  Either %0c
is not defined or it is defined to nothing.  Please
check that %1c is defined correctly and restart."

/*%
**  Description:
**	Can't find terminal definition or it is defined to be empty string.
**
**  System Status:
**	Forms system will not start up.
**
**  User Action:
**	Needs to check terminal definition and restart.
*/

E_TD001D_UNKNOWN
"An entry for terminal type `%0c' cannot be found in the termcap
file.\n  Please make sure %1c is properly defined.  If the
terminal you have does not match any existing entries in the
termcap file, you will need to write a termcap entry for your
specific terminal.  Please refer to the manual Using
Querying And Reporting Tools for directions."

/*%
**  Description:
**	Terminal type not found in termcap file.
**
**  System Status:
**	Forms system will not start up.
**
**  User Action:
**	User needs to make sure they defined their terminal type correctly
**	or write a termcap entry for a new terminal type.
*/

E_TD001E_NOTTERM
"Cannot open interactive communications.\n  Forms based applications
cannot be run in batch mode.  Please make sure application is
running from an interactive terminal."

/*%
**  Description:
**	TEopen failed.  Can't run forms in background or batch.
**
**  System Status:
**	Forms system will not start up.
**
**  User Action:
**	User needs to run a forms based application from an
**	interactive terminal.
*/

E_TD001F_CURINIT
"Error occurred while initializing current screen." /* DIAG */

E_TD0020_MSGINIT
"Error occurred while initializing standard message screen."	/* DIAG */

E_TD0021_UNKNOWN
"Terminal name `%0c' is not understood.\n  Please make sure
%1c is properly defined.  Refer to the manual Using
Querying And Reporting Tools for a list of accepted terminals."

/*%
**  Description:
**	Terminal type not found in termcap file.
**
**  System Status:
**	Forms system will not start up.
**
**  User Action:
**	User needs to make sure they defined their terminal type correctly.
*/

E_TD0022_too_small
"The forms system can't start up because the terminal size
appears to be too small (`%0d' rows by `%1d' columns).
Minimum size needed by the forms system is 5 rows by 25
columns.  Please check your %2c definition.  If you
are running from a terminal emulator within a windowing
system, please check the size of your emulator window."

/*%
**  Description:
**	The forms system couldn't startup because the terminal
**	size was determined to be too small.  Terminal size
**	must at least be 5 rows by 25 columns.
**
**  System Status:
**	The forms system will not be initialized and no forms
**	statements can be executed.
**
**  User Action:
**	User needs to check the terminal type definition or the window
**	size if running from a window system.
*/
