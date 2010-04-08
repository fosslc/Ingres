/*
**	Copyright (c) 1989 Ingres Corporation
*/

/**
** Name:	erwc.msg	- Message file for wcxt and wcmotif/wcolit directories.
**
** Description:
**	This is the message file for the wcxt and wcmotif/wcolit directories.
**	Messages for other window system directories may be placed here or in
**	their own message file.
**
** History:
**	11/14/91 (dkh) - Initial version.
**	01-may-92 (bruceb)
**		Added strings for wcolit.
**    06-oct-92 (bruceb)
**            Added FilePopup strings for wcolit.
**    18-nov-92 (kirke)
**            Spelling correction.
**    15-apr-94 (leighb)
**            Added user accelerator table error messages for bexcutor.c.
**    15-apr-94 (leighb)
**            Improve above messages.
**		15-aug-95 (hansm)
**						Added warnings for opening file with systemcursor definitions.
**/

# define	_WC_CLASS	220

/*
**	utxedit.c
*/

S_WC0001_TOO_MANY_EDITORS	"Can not start another editor.  Only %d editors may be active simultaneously.\n"

S_WC0002_COULDNT_EXEC	"Could not execute the '%s' command.\n"

S_WC0003_INTERNAL_ERR	"INGRES internal error '%x'.\n"

S_WC0004_COULDNT_FIND_CMD	"Could not find the '%s' command in your search path (errno = %d).\n"

S_WC0005_CHILD_DYING	"ChildDying: AST got bad parameter (%d).\n"

/*
**	xbitview.c
*/

S_WC0006_NOPIXEL_REALLOC	"Could not reallocate Pixel %d (%i2,%i2,%2) while cloning %c(%p) from %c(%p).\n"

S_WC0007_NOT_ENOUGH_COLORS	"Warning: only %d of %d colors were correctly allocated.  The rest are approximations.\n"

/*
**	xexcutor.c
*/
S_WC0008_NON_FATAL_X_ERROR	"=== Non-fatal Error Detected and Ignored ===\n"

S_WC0009_OLIT_WARNING	"OPEN LOOK Toolkit Warning: '%s'\n"

E_WC000A_SECOND_INIT	"Trying to initialize a second Executor.  Exiting.\n"

/*%
** Explanation:
**	W4gl detected an attempt to start a second executor.  This is
**	not allowed.
**
** System Status:
**	W4gl will exit.  User is back at the system prompt.
**
** User Action:
**	None.
*/

E_WC000B_NO_X_CONNECT	"Unable to open connection to X (DISPLAY = %c).  Exiting.\n"

/*%
** Explanation:
**	W4gl was not able to establish a connection with the requested
**	X server.
**
** System Status:
**	W4gl will exit leaving the user at the system prompt.
**
** User Action:
**	User should to make sure that they have correctly specified the
**	X server to connect to or that the network is reachable.
*/

S_WC000C_FONT_FILE_OPEN_ERR	"Warning: Couldn't open II_FONT_FILE ('%c').\n"

S_WC000D_BAD_FONT_FILE_NAME	"Warning: II_FONT_FILE is set to an invalid file name ('%c') and will be ignored.\n"


S_WC000E_FONT_FILE_EOF		"Unexpected EOF while reading '%c'.\n"

E_WC000F_NO_DEF_FONT_LOAD	"Couldn't load default font.  Exiting.\n"

/*%
** Explanation:
**	W4gl was unable to load the default font file.  Without this font
**	information, W4gl can not start up.
**
** System Status:
**	W4gl will exit leaving the user at the system prompt.
**
** User Action:
**	User needs to check to make sure that the font files are set up
**	correctly.
*/

S_WC0010_NO_SYS_DEF_FONT	"Warning: Couldn't load default SYSTEM font.  Substituting font 'fixed'.\n"

S_WC0011_II_KEYBD_OPEN_ERR	"Warning: Couldn't open II_KEYBOARD_FILE ('%c').\n"

S_WC0012_BAD_II_KEYBD	"Warning: II_KEYBOARD_FILE is set to an invalid file name ('%c') and will be ignored.\n"

S_WC0013_DEF_KYBD_OPEN_ERR	"Warning: Couldn't open II_KEYBOARD file '%c' in the INGRES 'files' directory.  Using "

S_WC0014_BUILTIN	"builtin defaults.\n"

S_WC0015_SPEEDKEY_LINE_IGN	"Record %d of Speedkey file ignored.\n"

S_WC0016_COLOR_OPEN_ERR	"Warning: Couldn't open II_COLOR_TABLE_FILE ('%c').\n"

S_WC0017_BAD_COLOR_NAME	"Warning: II_COLORTABLE_FILE is set to an invalid name ('%c') and will be ignored.\n"

S_WC0018_DEF_COLOR_OPEN	"Warning: Couldn't open II_COLORTABLE file '%c' in the INGRES 'files' directory.  Using builtin depth8 defaults.\n"

S_WC0019_COLOR_LINE_IGN	"Record %d of ColorTable file ignored.\n"

S_WC001A_LESS_COLORS	"Warning: only %d of %d standard colors were correctly allocated.  The rest are approximations.\n"

/*
**	xfont.c
*/

S_WC001B_CANT_LOAD_FONT	"Unable to load font '%c' by name.\n"

/*
**	xoption.c
*/

S_WC001C_OPTIONFLD_OUT_OF_SYNC	"OPTIONFIELD out of sync with CHOICELIST.  May need to execute the UpdChoiceList method for the OPTIONFIELD.\n"

/*
**	xutils.c
*/

S_WC001D_CANT_FIND_IN_HASH	"Couldn't find relative of %p is hash tables.\n"

E_WC001E_CANT_REG_FOR_EVENT	"Registration of UserEvent '%c' failed.  Couldn't create Atom.\n"

/*%
** Explanation:
**	W4gl was unable to create the necessary Atom in order to register
**	for receiving UserEvents.
**
** System Status:
**	W4gl continues processing.
**
** User Action:
**	None.
*/

E_WC001F_TOO_MANY_U_EVENTS	"Registration of UserEvent '%c' failed due to exceeding the maximum number of receivers (16).\n"

/*%
** Explanation:
**	W4gl could not register for receiving another UserEvent since
**	there are already 16 applications registered for the particular
**	userevent.
**
** System Status:
**	W4gl continues processing.
**
** User Action:
**	Reduce the number of UserEvents.
*/

E_WC0020_UNREG_EVENT_FAILED	"Unregistration of UserEvent '%c' failed.  Could not find the Atom.\n"

/*%
** Explanation:
**	W4gl could not find the necessary handle (Atom) to unregister for
**	the userevent.
**
** System Status:
**	W4gl continues processing.
**
** User Action:
**	None.
*/

E_WC0021_UNREG_E2_FAILED	"Unregistration of UserEvent '%c' failed.  Could get Property.\n"

/*%
** Explanation:
**	W4gl could not get the list of applications registered for the
**	userevent.  Probably some problems with X.
**
** System Status:
**	W4gl continues processing.
**
** User Action:
**	None.
*/

S_WC0022_TOO_FEW_COLORS	"Warning: insufficient colors.  Defining new colormap.\n"

S_WC0023_PIXEL_REALLOC	"Couldn't reallocate Pixel: %d (%i2,%i2,%i2).\n"

/*
**	xwindow.c
*/

S_WC0024_WIN_SET_SIZE_ERROR	"Warning: MainWindow size couldn't be properly set after %d attempts.  Window manager may be slow, broken or missing.\n"

/*
**	xwindow1.c
*/

E_WC0025_NO_CLIENT_MSG	"No ClientMessage event found for Window(%p) after
flush.  This is an internal error. Please report this problem to your Site
Technical Contact.  It is safest at this point to save any current work 
and then exit Windows4GL.\n"

/*%
** Explanation:
**	W4gl did not found a ClientMessage event waiting in the input queue.
**	Probably X did not queue it correctly or lost it somewhere.
**
** System Status:
**	W4gl will continue, but some frame(s) may be blocked forever.
**
** User Action:
**	Save any current work, then exit when possible.
*/

/*
**	mimgfld.c
*/

F_WC0001_IMAGE	"Image"

/*
**	mlistfld.c
*/

S_WC0026_LISTFLD_OUT_OF_SYNC	"LISTFIELD out of sync with CHOICELIST.  May need to execute the UpdChoiceList method for the LISTFIELD.\n"

/*
**	mmenubtn.c
*/

F_WC0002_MENU_BUTTON	"Menu Button"

F_WC0003_ELLIPSES	"..."

/*
**  Note that the ellipses have been put here so that it may be
**  changed to something else for indicating pull right menus.
*/

/*
**	mmenugrp.c
*/

F_WC0004_MENU_GROUP	"Menu Group"

/*
**	mmenulst.c
*/

S_WC0027_MENULIST_OUT_OF_SYNC	"MENULIST out of sync with CHOICELIST.  May need to execute the UpdChoiceList method of the MENULIST.\n"

/*
**	mpalette.c
*/

S_WC0028_PALETTE_OUT_OF_SYNC	"PALETTEFIELD out of sync with CHOICELIST.  May need to execute the UpdChoiceList method of the PALETTEFIELD.\n"

/*
**	mpopcnfr.c
*/

F_WC0005_MSGCONF	"Confirmation PopUp"

/*
**	mpopfile.c
*/

F_WC0006_FILE_POPUP	"File Selection PopUp"

/*
**	mpopinfo.c
*/

F_WC0007_MSG_POPUP	"Message PopUp"

/*
**	mpoprply.c
*/

F_WC0008_PROMPT_POPUP	"Prompt PopUp"

/*
**	mradio.c
*/


S_WC0029_RADIO_OUT_OF_SYNC	"RADIOFIELD out of sync with CHOICELIST.  May need to execute the UpdChoiceList method of the RADIOFIELD.\n"

/*
**	mtblfld.c
*/

F_WC0009_TF_INSERT	"Insert Before Current Row"

F_WC000A_TF_DEL_CUR	"Delete Current Row"

F_WC000B_TF_DEL_ALL	"Delete All Rows"

/*
**	mtoggle.c
*/

F_WC000C_TOGGLE	"Toggle"

/*
**	xexcutor.c
*/
S_WC002A_FONT_CNV_OPEN_ERR	"Warning: Couldn't open II_FONT_CONVENTION file '%c' in the INGRES 'files' directory.  Using builtin convention of 'x11r4'.\n"

/*
**	oimgfld.c
*/
E_WC002B_NON_EXPOSE_EVENT	"Expose handler for %c(%p) called with an non-expose event\n"

/*
**	opop*.c
*/
F_WC000D_OK	"OK"

F_WC000E_Cancel	"Cancel"

/*
**	opopinfo.c
*/
F_WC000F_Warning_Message "Warning Message"

F_WC0010_Error_Message "Error Message"

F_WC0011_Information_Message "Information Message"


/*
**    opopfile.c
*/
F_WC0012_Filter       "Filter"
F_WC0013_Previous     "Previous"
F_WC0014_Filter       "Filter:"
F_WC0015_File "File:"

E_WC002C_Invalid_File "Invalid File Selection"
E_WC002D_Invalid_Search_Mask  "Invalid File Search Mask Specification"
E_WC002E_Invalid_Directory    "Invalid Directory Specification"

/*
** xexcutor.c
*/
E_WC002F_Invalid_LineWidths   "Attempt to override the LineWidth values has failed.  Using default values.\n"


/*
** bexcutor.c
*/
E_WC0030_Invalid_Key		"Invalid Key: %c"
E_WC0031_No_Open_W4GLACC	"Unable to open W4GLACC.DLL\n"
E_WC0032_No_Create_W4GLACCX	"Unable to create W4GLACCX.DLL\n"
E_WC0033_Bad_ReadWrite		"W4GLACC Template DLL is too small OR out of disk space\n"
E_WC0034_Out_Of_Disk_Space	"Out of disk space while building user accelerator table\n"
E_WC0035_Invalid_SK_Command	"Invalid Speed Key Command: %c\n"
E_WC0036_No_DLL_Load		"Unable to load DLL %c\n"

/*%
** Explanation:
**	OpenRoad was not able to open the system cursor definition file.
**
** System Status:
**	It will give the following warning and use its internal defaults.
**
** User Action:
**	Read warnings and decide what to do.
*/

S_WC0037_CURSOR_FILE_OPEN_ERR	"Warning: Couldn't open II_CURSORTABLE_FILE ('%c').\n"

S_WC0038_BAD_CURSOR_FILE_NAME	"Warning: II_CURSORTABLE_FILE is set to an invalid file name ('%c') and will be ignored.\n"


S_WC0039_CURSOR_CNV_OPEN_ERR	"Warning: Couldn't open II_CURSORTABLE file '%c' in the INGRES 'files' directory.\n"

S_WC003A_CURSOR_FILE_EOF		"Unexpected EOF while reading '%c'.\n"

