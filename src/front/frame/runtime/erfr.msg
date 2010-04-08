/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    erfr.msg -	Frame Run-Time Message Definitions File.
**
** Description:
**	Contains the definitions for the messages of the frame run-time module.
**
** History:
**	14-aug-1987 (dave)
**		Created.
**	09/12/87 (dkh) - Added S_FR004C_ETXT.
**	11/12/87 (dkh) - Fixed typos.
**	11/21/87 (dkh) - Fixed jup bug 1218.
**	02/27/88 (dkh) - Added definition of F_FR0048_Nitm & F_FR0049_Nitm_Desc.
**	05/31/88 (ncg) - Added dynamic FRS errors
**	09/24/88 (dkh) - Fixed jup bug 1852.
**	20-dec-88 (bruceb)
**		Added S_FR0159_RDONLY and S_FR015A_INVIS.
**	24-apr-89 (bruceb)
**		Added F_FR004C_Shell, F_FR004D_Shell_Desc, S_FR015B_SHELL
**		and F_FR004E_Func_disabled.
**	27-sep-89 (bruceb)
**		Added S_FR0160_EDIT.
**	10/06/89 (dkh) - Added S_FR0160_DRVED and S_FR0161_DRVST.
**	29-nov-89 (bruceb)
**		Added F_FRxxx
**	12/07/89 (dkh) - Added support for inquiring on size information
**			 of a decimal datatype.
**	07-feb-90 (bruceb)
**		Added support for Getmessage display/supression.
**		Added support for single character find feature.
**	23-oct-1990 (mgw)
**		Corrected a misspelling in a message.
**	08/28/92 (dkh) - Added DEFINITION of S_FR0171_EXISTS to support
**			 inquiring on existence of a FRS object.
**	09/01/92 (dkh) - Added S_FR0172_MURNME, S_FR0173_MUONOF and
**			 S_FR0174_ODMSG for 6.5.
**	01/20/93 (dkh) - Added support for inputmasking toggle.
**/

#define	_FR_CLASS	57

/*
**	iitest.c
*/
E_FR0001_IIFlag_D	"IIFlag -D: Cannot open <%s>\n"		/* DIAG */
E_FR0002_IIFlag_Z	"IIFlag -Z: Cannot open <%s>\n"		/* DIAG */
E_FR0003_Illegal_II_FRSFLAG	"Illegal II_FRSFLAG Flag -%c\n"	/* DIAG */
/*
**	iisetio.c
*/
/*
**	iimenu.c
*/
F_FR0002_SUBMENU	"SUBMENU"
/*
**	ii40stiq.c
*/
S_FR0004_TERM	"TERMINAL"			/* DIAG */
S_FR0005_ERRN	"ERRORNO"			/* DIAG */
S_FR0006_VNXT	"VALIDATE ON NEXT FIELD"	/* DIAG */
S_FR0007_VPRV	"VALIDATE ON PREVIOUS FIELD"	/* DIAG */
S_FR0008_VMKY	"VALIDATE ON MENU KEY"		/* DIAG */
S_FR0009_VKYS	"VALIDATE ON FRSKEY"		/* DIAG */
S_FR000A_VMNU	"VALIDATE ON MENU ITEM"		/* DIAG */
S_FR000B_MUMP	"MENUMAP"			/* DIAG */
S_FR000C_MAP	"MAP"				/* DIAG */
S_FR000D_LABL	"LABEL"				/* DIAG */
S_FR000E_MPFL	"MAPFILE"			/* DIAG */
S_FR000F_PCST	"PCSTYLE"			/* DIAG */
S_FR0010_STLN	"STATUS LINE"			/* DIAG */
S_FR0011_VFRK	"FRSKEY VALIDATION"		/* DIAG */
S_FR0012_NORM	"NORMAL"			/* DIAG */
S_FR0013_RVVI	"REVERSE"			/* DIAG */
S_FR0014_BLNK	"BLINK"				/* DIAG */
S_FR0015_UNLN	"UNDERLINE"			/* DIAG */
S_FR0016_INTS	"INTENSITY"			/* DIAG */
S_FR0017_NAME	"NAME"				/* DIAG */
S_FR0018_CHNG	"CHANGE"			/* DIAG */
S_FR0019_MODE	"MODE"				/* DIAG */
S_FR001A_FILD	"FIELD"				/* DIAG */
S_FR001B_NUMB	"NUMBER"			/* DIAG */
S_FR001C_LEN	"LENGTH"			/* DIAG */
S_FR001D_TYPE	"TYPE"				/* DIAG */
S_FR001E_FMT	"FORMAT"			/* DIAG */
S_FR001F_VALD	"VALID"				/* DIAG */
S_FR0020_TBL	"TABLE"				/* DIAG */
S_FR0021_RWNO	"ROWNO"				/* DIAG */
S_FR0022_MXRW	"MAXROW"			/* DIAG */
S_FR0023_LROW	"LASTROW"			/* DIAG */
S_FR0024_DROW	"DATAROWS"			/* DIAG */
S_FR0025_MCOL	"MAXCOL"			/* DIAG */
S_FR0026_COLN	"COLUMN"			/* DIAG */
S_FR0027_COLR	"COLOR"				/* DIAG */
S_FR0028_ANXT	"ACTIVATE ON NEXT FIELD"	/* DIAG */
S_FR0029_APRV	"ACTIVATE ON PREVIOUS FIELD"	/* DIAG */
S_FR002A_LCMD	"COMMAND"			/* DIAG */
S_FR002B_AMKY	"ACTIVATE ON MENU KEY"		/* DIAG */
S_FR002C_AMUI	"ACTIVATE ON MENU ITEM"		/* DIAG */
S_FR002D_AKYS	"ACTIVATE ON FRSKEY"		/* DIAG */
S_FR002E_DTYP	"DATATYPE"			/* DIAG */
/*
**	fetabfnd.qc
*/
S_FR002F_Enter_string_to_find	"Enter string to find: "
S_FR0030_Cannot_find	"Cannot find '%0c'"
/*
**	fehvals.qc
*/
E_FR0031_Can_not_access_form	"Can not access form \"helpfrom\""
S_FR0032_HELP_Values_for	"HELP -- Values for "
S_FR0033_Column	"Column "
S_FR0034_Field	"Field "
F_FR0003_Select	"Select"
S_FR0035_HELP_Field_Values	"HELP -- Field Values Frame"
E_FR0036_Error_formatting	"Error formatting validation string"
S_FR0037_Nullable	"Nullable"
S_FR0038_Datatype	"Datatype: %s %s"
S_FR0039_Format	"Format: %s"
S_FR003A_No_validation_check	"No validation check in effect."
S_FR003B_Validation_Check_is	"Validation Check is:"
S_FR003C_Validations	" -- Validations"
S_FR003D_Error_filling	" -- Error(s) filling values list."
S_FR003E_Value_must_be_in	" -- Value must be in list shown below..."
/*
**	fehelp.qc
*/
F_FR0004_WhatToDo	"WhatToDo"
S_FR003F_HELP_sub	"HELP -- %s"
F_FR0005_Keys		"Keys"
S_FR0040_HELP_Command_Key	"HELP -- Command-Key Mapping for %s"
S_FR0041_HELP_Help_Submenu	"HELP -- Help Submenu"
E_FR0042_Unexpected_graphics	"Unexpected graphics flag (vfrfiq = %d)"
/*
**	fehframe.qc
*/
F_FR0050_SubTopics		"SubTopics"
F_FR0051_PrevTopic		"PrevTopic"
F_FR0052_ChooseTopic		"Choose a SubTopic"
F_FR0053_ChooseTopicHelp	"Choosing a SubTopic"
F_FR0054_SubTopicToken		"SubTopic"

E_FR0043_BadForm		"Cannot access form \"%0c\"."
S_FR0044_Sorry_No_help_avail	"Sorry - No help available on \"%s\""
S_FR0045_Sorry_Cannot_edit	"Sorry - Cannot edit file \'%s\': Status %d"
S_FR0046_HELP_Main_Help		"HELP -- Main Help Frame"
/*
**	fehkeys.qc
*/
F_FR0006_Menu	"Menu"
F_FR0007_Menu_Desc	"Go to or scroll through the menu line"
F_FR0008_Scup	"Scrollup"
F_FR0009_Scup_Desc	"Go to the next page of table or form"
F_FR000A_Scdn	"Scrolldown"
F_FR000B_Scdn_Desc	"Go to previous page of table or form"
F_FR000C_Nfld	"Nextfield"
F_FR000D_Nfld_Desc	"Go to the next field"
F_FR000E_Next	"Next"
F_FR000F_Next_Desc	"Go to the next graphical component"
F_FR0010_Pfld	"Previousfield"
F_FR0011_Pfld_Desc	"Go to the previous field"
F_FR0012_Prev	"Previous"
F_FR0013_Prev_Desc	"Go to the previous graphical component"
F_FR0014_Sclt	"Scrollleft"
F_FR0015_Sclt_Desc	"Scroll form to the left"
F_FR0016_Scrt	"Scrollright"
F_FR0017_Scrt_Desc	"Scroll form to the right"
F_FR0018_Ledg	"Leftedge"
F_FR0019_Ledg_Desc	"Go to left edge of field or form"
F_FR001A_Ledg	"Leftedge"
F_FR001B_Ledg_Desc	"Go to left edge of component or graph"
F_FR001C_Tedg	"Topedge"
F_FR001D_Tedg_Desc	"Go to top edge of component or graph"
F_FR001E_Redg	"Rightedge"
F_FR001F_Redg_Desc	"Go to right edge of field or form"
F_FR0020_Redg	"Rightedge"
F_FR0021_Redg_Desc	"Go to right edge of component or graph"
F_FR0022_Bedg	"Bottomedge"
F_FR0023_Bedg_Desc	"Go to lower edge of component or graph"
F_FR0024_Lchr	"Leftchar"
F_FR0025_Lchr_Desc	"Move left one space"
F_FR0026_Rchr	"Rightchar"
F_FR0027_Rchr_Desc	"Move right one space"
F_FR0028_Dnln	"Downline"
F_FR0029_Dnln_Desc	"Move down one line"
F_FR002A_Upln	"Upline"
F_FR002B_Upln_Desc	"Move up one line"
F_FR002C_Gcur	"Graphcursor"
F_FR002D_Gcur_Desc	"Activate graphics pointer on terminal"
F_FR002E_Lerr	"Lasterror"
F_FR002F_Lerr_Desc	"Display last error message"
F_FR0030_Nxwd	"Nextword"
F_FR0031_Nxwd_Desc	"Move forward one word"
F_FR0032_Pvwd	"Previousword"
F_FR0033_Pvwd_Desc	"Move backward one word"
F_FR0034_Mode	"Mode"
F_FR0035_Mode_Desc	"Switch edit mode - insert/overstrike"
F_FR0036_Redr	"Redraw"
F_FR0037_Redr_Desc	"Redraw the screen "
F_FR0038_Dchr	"Deletechar"
F_FR0039_Dchr_Desc	"Delete character under cursor"
F_FR003A_Rubout	"Rubout"
F_FR003B_Rubout_Desc	"Delete character to left of cursor"
F_FR003C_Edit	"Editor"
F_FR003D_Edit_Desc	"Start system editor on field"
F_FR003E_Nrow	"Newrow"
F_FR003F_Nrow_Desc	"Move to first column on next row"
F_FR0040_Clear	"Clear"
F_FR0041_Clear_Desc	"Clear field or menu input"
F_FR0042_Clst	"Clearrest"
F_FR0043_Clst_Desc	"Clear field from cursor to end"
F_FR0044_Dup	"Duplicate"
F_FR0045_Dup_Desc	"Duplicate last value entered"
F_FR0046_Pscr	"Printscreen"
F_FR0047_Pscr_Desc	"Write current screen to file"
F_FR0048_Nitm	"Nextitem"
F_FR0049_Nitm_Desc	"Move to next item in form"
E_FR004A_Can_not_access_form	"Can not access form \"keysform\""
S_FR004B_HELP_Command_Key	"HELP -- Command-Key Mapping"
/*
**	ii40stiq.c
*/
S_FR004C_ETXT	"Error Text"	/* DIAG */

/*
**	fetabfnd.qc
*/
E_FR004D_Not_on_tf	"Cursor not on table field.\n
The \"Find\" function can only search a table field column for a desired string.
Either the cursor is not on a table field, or the specified field to be searched
is not a table field.  Please move the cursor to a table field and select
\"Find\" again."

/*
**  Description:
**	User selecting the "Find" menu item without having the cursor on
**	a table field.
**
**  System Status:
**	System will simply ignore request.
**
**  User Action:
**	Move the cursor to a table field and select "Find" again.
*/

E_FR004E_TF_is_empty	"The table field is empty.\n
The cursor is on an empty table field, or an empty table field was specified
for the \"Find\" operation.  There is nothing to find."

/*
**  Description:
**	User selected the "Find" menuitem with the cursor on a empty table
**	field, or specified a table field that was empty.
**
**  System Status:
**	System will simply ignore request.
**
**  User Action:
**	User needs to move cursor to a table field with data in it.
*/

E_FR004F_NotComparable	"Search value cannot be compared against column value.\n
The data type of the search value is not comparable with that of the column
values.  The search value for the \"Find\" menu item or 'find_record()' function
must be comparable to the data type of the column to be searched."


/*
**  Description:
**	User has cursor on a column that is not compatible with
**	one of the character datatypes when the "Find" menuitem
**	was selected.
**
**  System Status:
**	System will simply ignore request.
**
**  User Action:
**	User needs to move cursor to a column that contains that
**	that is compatible with one of the character datatypes.
*/

/*
** 50 - 59 are for logic checks in IIFRgp* routines.  They should not
** happen, and indicate software bugs rather than user errors
*/
E_FR0050_NegAlt
"Bad parameters in IIFRgpinit (negative alt. value,
or no default settings)" /* DIAG */

E_FR0051_NegPid		"Bad parameters in IIFRgpinit (negative pid)" /* DIAG */

E_FR0052_BadType
"Bad parameters in IIFRgpinit (Set entry %0d is unknown type %1d)" /* DIAG */

E_FR0053_BadPid		"Illegal PID, %0d, IIFRgpset" /* DIAG */

E_FR0054_BadType	"Bad data type in parameter array, IIFRgpset" /* DIAG */

E_FR0055_ConvFail	"Failed conversion in IIFRgpset" /* DIAG */

E_FR0056_BadOpen	"Already open error, IIFRgpcontrol" /* DIAG */

E_FR0057_BadAlt		"Bad parameter set, %0d, IIFRgpcontrol"	/* DIAG */

E_FR0058_BadClose	"Already closed error, IIFRgpcontrol" /* DIAG */

E_FR0059_BadState	"Bad state, %0d, IIFRgpcontrol" /* DIAG */

/*
**	ii40stiq.c
*/
S_FR005A_HGHT	"Height (rows)" /* DIAG */
S_FR005B_WIDTH	"Width (columns)" /* DIAG */
S_FR005C_SROW	"Starting row" /* DIAG */
S_FR005D_SCOL	"Starting column" /* DIAG */
S_FR005E_CROW	"Current row" /* DIAG */
S_FR005F_CCOL	"Current column" /* DIAG */
S_FR0060_TMOUT	"Timeout" /* DIAG */
S_FR0061_EACT	"Entry activation" /* DIAG */
S_FR0062_DROW	"Delete row" /* DIAG */

/* Errors E_FR0100 through E_FR0120 are used for Dynamic SQL/FRS */

E_FR0100_SQLDA_NULL
"The address of the SQLDA in forms statement is a null pointer."

/*
**  Description:
**	In a forms DESCRIBE statement, or forms USING clause with an SQLDA
**	the SQLDA object is pointing at a zero address.
**
**  System Status:
**	Run-time system will not execute the statement.
**
**  User Action:
**	Check the declaration and use of the SQLDA.  The preprocessor does
**	not confirm that this is a valid SQLDA object.
*/

E_FR0101_SQ_FORM_TABLE_BAD
"Invalid or null form or table name in forms DESCRIBE statement."

/*
**  Description:
**	In a forms DESCRIBE statement, the form or table name was either
**	null or invalid.
**
**  System Status:
**	Run-time system will not execute the statement.
**
**  User Action:
**	Confirm that you have initialized the form into the run-time system
**	before describing it.
*/

E_FR0102_SQ_MODE_BAD
"Unknown mode specification '%0c' in forms DESCRIBE statement."

/*
**  Description:
**	In a forms DESCRIBE statement the specified mode was not one of
**	QUERY, READ or ALL.
**
**  System Status:
**	Run-time system will not execute the statement.
**
**  User Action:
**	Use the correct mode when describing a form.
*/

E_FR0103_SQ_NAME_BAD
"Invalid syntax for name '%0c' in forms SQLDA SQLNAME element %1d."

/*
**  Description:
**	When using a SQLDA to execute a forms statement the name specified
**	in the SQLNAME field was invalid and could not be processed.
**
**  System Status:
**	Run-time system will ignore this field/column and continue processing
**	the statement.
**
**  User Action:
*/

E_FR0104_SQ_NULL_IND
"Indicator variable expected in forms SQLDA SQLIND element %0d."

/*
**  Description:
**	When using a SQLDA to execute a forms statement the data type 
** 	(SQLTYPE) was negative, but no indicator variable (SQLIND) was
**	provided.
**
**  System Status:
**	Run-time system will ignore this field/column and continue processing
**	the statement.
**
**  User Action:
*/

E_FR0105_SQ_NULL_DATA
"Data variable expected in forms SQLDA SQLDATA element %0d."

/*
**  Description:
**	When using a SQLDA to execute a forms statement the data address
**	(SQLDATA) was zero.  You must provide a data address even if the
**	value being assigned is null, as specified in the null indicator.
**
**  System Status:
**	Run-time system will ignore this field/column and continue processing
**	the statement.
**
**  User Action:
*/

S_FR0150_cnoval_null	"Column '%0c', data type nullable %1c, display
format %2c, has no validation check"

S_FR0151_cnoval	"Column '%0c', data type %1c, display
format %2c, has no validation check"

S_FR0152_chasval_null	"Column '%0c', data type nullable %1c, display
format %2c, has validation check '%3c'"

S_FR0153_chasval	"Column '%0c', data type %1c, display
format %2c, has validation check '%3c'"

S_FR0154_fnoval_null	"Field '%0c', data type nullable %1c, display
format %2c, has no validation check"

S_FR0155_fnoval	"Field '%0c', data type %1c, display
format %2c, has no validation check"

S_FR0156_fhasval_null	"Field '%0c', data type nullable %1c, display
format %2c, has validation check '%3c'"

S_FR0157_fhasval	"Field '%0c', data type %1c, display
format %2c, has validation check '%3c'"

/*
**  Description (150-157):
**	Messages used for Help/Field.  %0c argument is the name of the
**	field or column. %1c is the data type string as it would appear
**	in the Data Type field of vifred's attribute screen, %2 is the
**	display format as seen in vifred, and %3 the actual validation
**	check text as entered in the vifred attribute screen.
**
*/

F_FR004A_column	"column"
F_FR004B_field	"field"
E_FR0158_Menuitem_does_not
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

S_FR0159_RDONLY "Displayonly"	/* DIAG */
S_FR015A_INVIS "Invisible"	/* DIAG */
S_FR015B_SHELL "Shell"	/* DIAG */

F_FR004C_Shell	"Shell"
F_FR004D_Shell_Desc	"Enter command line interpreter"
F_FR004E_disabled	" (Currently disabled)"

/*%
**  Description:
**	Line entries in the 'help keys' table.  (fehkeys.qsc)
**	The explanation for the shell key will include F_FR004E if the
**	shell key is not enabled in the application.
**
**  System Status:
**	None.
**
**  User Action:
**	None.
*/

E_FR015C_BadEXSet	"Could not set exception handler"	/* DIAG */
E_FR015D_BadListParm	"Bad dynamic list size (%d rows, %d columns)" /* DIAG */
E_FR015E_BadMagic	"Bad magic number on dynamic list"	/* DIAG */

/*%
**  Description:
**	Internal list pick errors which are not supposed to happen.
**
**  System Status:
**	Treated as "Forget".
**
**  User action:
**	Call tech support.
*/

S_FR015F_LFRSK	"LAST FRSKEY"		/* DIAG */

S_FR0160_EDIT	"Editor"		/* DIAG */

S_FR0161_DRVED	"DERIVED"		/* DIAG */

S_FR0162_DRVST	"DERIVATION STRING"	/* DIAG */

F_FR004F_frskey_expl	"User defined activation."
/*%
**  Description:
**	String to display as the 'explanatory text' for an frskey
**	in Help Keys when the particular frskey is active for the current
**	display loop, but has no explanation specified.
**
**  System Status:
**	None.
**
**  User action:
**	None.
*/

E_FR016B_Alloc		"Memory allocation failure in prompt"

/*%
**  Description:
**	Memory allocation failure while processing popup prompt.
**
**  System Status:
**	"Cancel" returned.
**
**  User action:
**	Retry operation from a "shallower" level of interaction, if
**	possible, or in a new session.
*/

S_FR016C_DPREC	"DECIMAL PRECISION"	/* DIAG */

S_FR016D_DSCL	"DECIMAL SCALE"		/* DIAG */

S_FR016E_DSZE	"DECIMAL SIZE"		/* DIAG */

S_FR016F_GMSGS	"GETMESSAGE"		/* DIAG */

S_FR0170_KEYFND	"KEYFIND"		/* DIAG */

S_FR0171_EXISTS	"EXISTS"		/* DIAG */

S_FR0172_MURNME "MENU RENAME"		/* DIAG */

S_FR0173_MUONOF "MENU ACTIVE"		/* DIAG */

S_FR0174_ODMSG	"OUT OF DATA MESSAGE"	/* DIAG */

S_FR0175_EMASK	"INPUTMASKING"		/* DIAG */
