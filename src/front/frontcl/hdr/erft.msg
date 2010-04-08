/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    erft.msg -	FT Interface Message File Definitions.
**
** Description:
**	This file defines the messages for class FT.  This message
**	file is for the "ft" directory in the "frontcl" facility.
**
**	Do NOT attempt to put message ids from the same file
**	together.  Even though message ids for the same file
**	may be spread over this file, the numbers in the file
**	are consecutive and makes compiling this file faster.
**	If it is necessary to look for all the message ids
**	for a particular file, simply search for the name of
**	the file.
**
** History:
**	18-aug-1987 (dave)
**		Created.
**	08/23/87 (dkh) - Added error numbers that used to be in 8000
**			 series and eliminated unnecessary underscores
**			 in some of the message ids.
**	11/12/87 (dkh) - Fixed typos.
**	12/12/87 (dkh) - Modified messages E_FT0002, E_FT0013,
**			 F_FT000F, E_FT0017, E_FT0018, E_FT001A,
**			 E_FT001B, E_FT001C and E_FT001D.
**	02/25/88 (dkh) - Fixed jup bug 1853; new message id E_FT003B.
**	01-apr-88 (bruceb)
**		New message id F_FT0010.
**	08-apr-88 (bruceb)
**		Modified messages S_FT000[CDE] and S_FT0010 to work better
**		for DG.
**	28-apr-88 (bruceb)
**		Modified message E_FT001A; mention ING_EDIT instead of
**		II_EDIT.
**	07/09/88 (dkh) - Added ids E_FT003C_NODISP, E_FT003D_NOWIN and
**			 E_FT003E_NOSWIN.
**	09/24/88 (dkh) - Fixed jup bug 1852.
**	01/12/90 (dkh) - Added [PRESS RETURN] to E_FT001E and E_FT002E.
**	14-feb-90 (bruceb)
**		Changed ellipses as per new style guideline.
**	27-feb-90 (bruceb)
**		Added F_FT0011_No_Keyfind as indicator that keyfind is
**		to be turned off.
**	16-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Technical Support Representative'
**		with 'Site Technical Contact'.
**/

#define	_FT_CLASS	61

/*
**	ftboxerr.c
*/
F_FT0001_Return		"Return"
F_FT0002_More		"More"
F_FT0003_MoreKey	"h"
/*
**	ftbrowse.c
*/
S_FT0001_Number_of_fields_to_s	"Number of fields to skip: "

E_FT0002_Bad_value_entered_for
"Bad value entered for SKIP FIELD command.\n
The input could not be converted to an integer.  Only
numeric characters or the '-' may be entered."

/*%
**  Description:
**	Non-numeric characters probably entered.
**
**  System Status:
**	The skip field command will be aborted.
**
**  User Action:
**	User needs to select the skip field command again and
**	enter a proper numeric value.
*/

F_FT0005_ERROR__Not_a_valid_co	
	"ERROR: Not a valid command in browse mode [PRESS RETURN]"
/*
**	ftbuild.c
*/
E_FT0003_FTbuild__passed_NULL_	"FTbuild: passed NULL pointer"
E_FT0004__rFIELD_INIT___s__s__	"\rFIELD INIT:(%s,%s) error in field window\r\n"
E_FT0005__rFIELD_INIT___s__s__	"\rFIELD INIT:(%s,%s) error in data window\r\n"
E_FT0006_TBL_INIT__can_t_alloc	"TBL INIT: can't allocate table field's window %s\n"
E_FT0007__rFIELD_INIT___s__s__	"\rFIELD INIT:(%s,%s) error in data window\r\n"
/*
**	ftgetmenu.c
*/
E_FT0008_Menuitem___d__does_no
"No menuitem at position `%0d' in the current menu.  You have pressed
a key that is mapped to a menuitem at a specific position.  But the
current menu does not contain a menuitem at that position.  Please
check that you have pressed the correct key and make another selection."
/*%
**  Description:
**	The user pressed a function/control key that is mapped
**	to a positional menuitem.  Unfortunately there is no
**	menuitem at that position.
**
**  System Status:
**	Once user acknowledges the message, the forms system will
**	continue.
**
**  User Action:
**	User acknowledges message and continues on.
*/

S_FT0009_Null_string_not_legal
"No input found for menu selection.  The current menu requires
that you type in a menuitem selection.  Please enter a menuitem
selection."
/*%
**  Description:
**	User has pressed the RETURN key with no characters in the
**	menu input area.  The current menu requires that
**	something be typed in.  This is really not an error
**	since all menus should now allow user to press the
**	RETURN key with no problems.
**
**  System Status:
**	Once user acknowledges the message, the forms system will
**	continue.
**
**  User Action:
**	User acknowledges message and continues on.
*/

/*
**	ftgtctl.c
*/
E_FT000A_FTgtctl_called_when_l	"FTgtctl called when loaded against non-graphics framelib"
/*
**	ftkfe.c
*/
/*
**	ftmaplbl.c
*/
F_FT000A_UP_ARROW	"UP-ARROW"
F_FT000B_DOWN_ARROW	"DOWN-ARROW"
F_FT000C_RIGHT_ARROW	"RIGHT-ARROW"
F_FT000D_LEFT_ARROW	"LEFT-ARROW"
/*
**	ftprnscr.c
*/
F_FT000E_printer	"printer"
S_FT000B_Enter_file_name_	"Enter file name: "
S_FT000C_Error_opening_file___	"Error opening '%s' file [PRESS RETURN]"
S_FT000D_Writing_screen_to_fil	"Writing screen to '%s' file . . ."
S_FT000E_Appending_screen_to_f	"Appending screen to '%s' file . . ."
E_FT000F_Error___Current_scree	"Error:  Current screen is empty."
S_FT0010_Error_closing_file___	"Error closing '%s' file [PRESS RETURN]"
S_FT0011_Error_attempting_to_p	
		"Error attempting to print screen. [PRESS RETURN]"
/*
**	ftputmenu.c
*/
E_FT0012_MENU	"MENU"
/*
**	ftrun.c
*/
E_FT0013_ERROR__bad_editing_mo
"Unknown editing mode encountered.\n  An unknown editing mode was
passed to the forms system while trying to run a display loop.
Please write down circumstances that led to the error and report
it to your Site Technical Contact.  Please retry operation
or restart the application."

/*%
**  Description:
**	Unknown mode passed to FTrun.  Probably something is corrupted.
**
**  System Status:
**	Returns control to runtime layer of forms system and continues
**	processing.
**
**  User Action:
**	User can retry the operation or restart the application.
*/

/*
**	insrt.c
*/
S_FT0014_Number_of_fields_to_s	"Number of fields to skip: "
S_FT0015_Bad_value_entered_for	"Bad value entered for SKIP FIELD command."
/*
**	strfld.c
*/
F_FT000F_ERROR__field__s_trunc	"Field \"%s\" truncated. [PRESS RETURN]"

/*
**	vi.c
*/

E_FT0017_Currently_unable_to_e
"Field is too wide to be edited.\n
Currently, only fields that are less than or equal to '%0d' bytes wide
may be edited by calling a system editor."

/*%
**  Description:
**	Field is too wide to be edited.
**
**  System Status:
**	Forms system will not call a system editor on the current
**	field.
**
**  User Action:
**	User must continue editing field within the forms system.
*/

E_FT0018_Could_not_open_tempor
"Could not open temporary edit file.\n
Please check that you have appropriate permissions and quotas to
call the system editor from the current directory."

/*%
**  Description:
**	Must be a permission or quota problem that prevents temp
**	file from being opened.
**
**  System Status:
**	Forms system will abort calling the system editor.
**
**  User Action:
**	User can continue editing within forms system.
*/

S_FT0019__nStarting_Editor____	"\nStarting Editor . . . \n"

E_FT001A_Could_not_start_Edito
"Could not start Editor.\n
The system editor could not be started.  Please check that ING_EDIT,
if defined, points to a valid system editor.  Also, make sure that
your process has appropriate quotas set."

/*%
**  Description:
**	ING_EDIT may not be set correctly or process has low quotas.
**
**  System Status:
**	Forms system will abort calling the system editor.
**
**  User Action:
**	User can continue editing within forms system.
*/

E_FT001B_Could_not_open_tempor
"Temporary editing file not found.\n
The forms system could not open the temporary editing file for
input.  Please check that process quotas."

/*%
**  Description:
**	Couldn't open editing file for input.
**
**  System Status:
**	Field will not be changed.
**
**  User Action:
**	Changes made via the system editor is lost.  User can
**	try calling the system editor again or remain in the
**	forms system.
*/

E_FT001C_Data_in_file_too_big_
"Data in file too big for field.\n
Changes made in the system editor can not all fit into the field.
Only data that fits will be placed back into field."

/*%
**  Description:
**	User entered too much data while in system editor.
**
**  System Status:
**	System will simply truncate the data.
**
**  User Action:
**	User should check field.
*/

E_FT001D_Data_in_file_too_big_
"Data in file too big for field.\n
Changes made in the system editor can not all fit into the field.
Only data that fits will be placed back into field."

/*%
**  Description:
**	User entered too much data while in system editor.
**
**  System Status:
**	System will simply truncate the data.
**
**  User Action:
**	User should check field.
*/

/*
**	keys30.c
*/
E_FT001E_CANTOPEN	
	"Can't open terminal specific mapping file.  [PRESS RETURN]"
E_FT001F_ERRSFND	"Error(s) in mapping file.  Error messages
in '%0c'.  '%1c'"

/*
**	funckeys.c
*/
E_FT0020_NUMCONV	"An error occurred while converting a key number
form its character representation."
E_FT0022_BSTART	"Unknown keyword at beginning of a mapping line."
E_FT0023_FRSCTRL	"No number specified for FRS/CONTROL key."
E_FT0024_FRSKEY	"May not specify a FRS key greater than `%0d'."
E_FT0025_UNKNOWN	"Unknown command specified."
E_FT0026_NOEQUAL	"Could not find the `=' character in a mapping line."
E_FT0027_PFKEY	"Missing number for PF key after the `=' character."
E_FT0028_BPFKEY	"PF key `%0d' does not exist for your terminal description."
E_FT0029_BADCMD	"Invalid command found after the `=' character."
E_FT002A_OFFIGN	"Ignoring excess characters after the 'OFF' keyword."
E_FT002B_BADLABEL	"A bad label was found in a mapping line."
E_FT002C_LBLIGN	"Ignoring excess characters after a 'LABEL'."
E_FT002D_CTRLPF	"No control or PF key specified in a mapping line."

/*
**	keys30.c
*/
E_FT002E_NODEFMAP	
	"Can't open default system mapping file.  [PRESS RETURN]"

/*
**	ftmapfile.c
*/
E_FT002F_NOMFILE	"Could not open mapping file `%0c'."

/*
**	mapping.c
*/
E_FT0030_INUSE	"'%0c' has already been mapped."
E_FT0031_BADLCMD	"Bad left hand size of `=' character passed to
FKdomap."	/* DIAG */
E_FT0032_OVRRIDE	"May not override mapping for `%0c'."
E_FT0033_BADRCMD	"Command on right hand size of `=' character
may not be assigned to command of left hand size of `=' character."

/*
**	funckeys.c
*/
E_FT0034_KEYZERO	"FRSkey/Menuitem numbers must start with one (1)."
E_FT0035_MAXMENU	"There is a current limit of `%0d' MENU positions
that may be specified."

/*
**	ftmaplbl.c
*/
E_FT0036_NOESC	"The ESCAPE key is not available for your terminal description."
E_FT0037_NOPFK	"Your terminal description does not support function keys."
/*
**	ftgetmenu.c
*/
E_FT0038_RTMULEN
"Can not display menuitems, it is longer than the width of the screen."
/*
**	ftsvinput.c
*/
E_FT0039_TSTBADOPN	"Can not open test file `%0c'."	/* DIAG */
/*
**	fttest.c
*/
E_FT003A_IONOREDIR	"Failed to redirect I/O for testing."	/* DIAG */

/*
**	ftbrowse.c
*/

E_FT003B_INVALID_COMMAND
"Not a valid command for BROWSE mode.\n  You are currently in BROWSE
mode.  This means that you may only move the cursor within and between
fields.  You may NOT modify or enter values in fields.  Your last
command either tried to change a field value or was a command not
allowed in BROWSE mode.  Please try again."

/*%
**  Description:
**	User tried to enter something into a field or entered a command
**	that is not supported by browse mode.
**
**  System Status:
**	System will just ignore the request and continue.
**
**  User Action:
**	User should make another request.
*/

/*
**	fldstr.c
*/
F_FT0010_STR_TRUNCATED	"Field \"%0c\" truncated."

/*%
**  Description:
**	Value entered by the user was too long for the underlying
**	data space.  This can occur with e.g. a c200.67 format which
**	contains screen space for 201 chars, but only accepts 200.
**
**  System Status:
**	String will be truncated; processing continues.
**
**  User Action:
**	User hits RETURN and continues.
*/

/*
**	ftsyncdsp.c
*/

E_FT003C_NODISP
"The forms system could not find desired form.\n  The forms system
was requested to display a form that it could not find in its
display list.  This is probably due to memory corruption.  Please
write down circumstances that led to this error and report it to
your Site Technical Contact.  This is a fatal error.  You
will need to restart your application."
/*%
**  Description:
**	The forms system tried to find a form in the list of display
**	structures but could not.  This probably means something
**	is corrupted and we can not continue.  This error is fatal.
**
**  System Status:
**	Once user acknowledges the error message, the application will
**	exit.
**
**  User Action:
**	User can do nothing since this is a fatal error.
*/

E_FT003D_NOWIN
"Structure allocation failed for forms system.\n  The forms system
could not allocate a structure needed to display a popup style
form.  This may be caused by an out of memory condition or by
the size or starting position of the popup style form has been
corrupted.  Please write down circumstances that led to this error
and report it to your Site Technical Contact.  This is a
fatal error.  You will need to restart your application."
/*%
**  Description:
**	The forms system tried to allocate a new window structure
**	for a new form to be displayed in a popup style but failed.  
**	This may be due to out of memory problem.  This error is fatal.
**
**  System Status:
**	Once user acknowledges the error message, the application will
**	exit.
**
**  User Action:
**	User can do nothing since this is a fatal error.
*/

E_FT003E_NOSWIN
"Form data structure setup failed.\n  The forms system failed
to setup data structures needed to display a popup style form.
This may be due to the starting position of the popup style
form being corrupted.  Please write down circumstances that led
to this error and report it to your Site Technical Contact.
This is a fatal error.  You will need to restart your application."
/*%
**  Description:
**	The forms system tried to create a subwindow for displaying
**	just the form area (excluding the border) for a popup style form
**	but failed.  This may be due to errors in coordinate specifications
**	and should never happen.
**	This error is fatal.
**
**  System Status:
**	Once user acknowledges the error message, the application will
**	exit.
**
**  User Action:
**	User can do nothing since this is a fatal error.
*/

E_FT003F_is_ambiguous
"Input \"%0c\" is an ambiguous prefix.  The current menu contains
more than one menuitem that begins with the string \"%0c\".  Please
enter more characters to uniquely identify a specific menuitem."
/*%
**  Description:
**	The user has entered a string that is not unique (i.e., more
**	than one menuitem start with the entered string.
**	User must type in more characters to make a selection.
**
**  System Status:
**	Once user acknowledges the message, the forms system will
**	continue.
**
**  User Action:
**	User acknowledges message and continues on.
*/

E_FT0040_is_not_prefix
"Input \"%0c\" is not a correct prefix.  The current menu contains
no menuitems that begin with the string \"%0c\".  Please check
what you have typed in and make any necessary corrections."
/*%
**  Description:
**	User has entered a string that does not match any of
**	the menuitems in the current menu.  User must correct
**	his input.
**
**  System Status:
**	Once user acknowledges the message, the forms system will
**	continue.
**
**  User Action:
**	User acknowledges message and continues on.
*/

F_FT0011_No_Keyfind	"false"
/*%
**  Description:
**	Set II_FRS_KEYFIND to this value to indicate that single keystroke
**	find (in table fields) is not desired.  Otherwise single keystroke
**	find will be turned on.
**
**  System Status:
**	If II_FRS_KEYFIND is set to this value, set global control block
**	to indicate keyfind is off.
**
**  User Action:
**	None.
*/
