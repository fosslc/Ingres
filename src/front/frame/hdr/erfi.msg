/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erfi.msg -	Message file for class FI
**
** Description:
**	This file defines the messages for class FI.  This is the
**	error message file for the 8000 series error numbers that
**	users can inquire about.  These numbers span across
**	multiple directories of the forms system.  Essentially,
**	these are the error numbers for the forms system facility.
**	All numbers here are defined in the 8000 range.
**
**	When adding new error numbers, please put it in numerical
**	order so that compiling of this message file will go
**	quickly.
**
** History:
**	22-aug-1987 (dave)
**		Created.
**	09/03/87 (dkh) - Fixed error id E_FI1FB5_8117.
**	09/05/87 (dkh) - Added error ids for "valid" directory.
**	09/05/87 (dkh) - Deleted unused error ids.
**	09/16/87 (dkh) - Integrated table field change bit.
**	10/16/87 (dkh) - Added following error ids - E_FI2198_8600,
**			 E_FI2199_8601, E_FI21CA_8650, E_FI21CB_8651,
**			 E_FI21CC_8652, E_FI21CD_8653, E_FI21CE_8654
**			 and E_FI21CF_8655.
**	11/12/87 (dkh) - Fixed typos.
**	11/28/87 (dkh) - Added error id E_FI21D0_8656.
**	12/07/87 (dkh) - Fixed jup bug 1451.
**	05-apr-88 (bruceb)
**		Fixed sentence in message E_FI201E.
**	02-may-88 (bruceb)
**		Added that value might be out of range for msgs
**		E_FI21CB and E_FI21CC.
**	04-aug-88 (bruceb)
**		The above messages slightly changed.
**	29-nov-88 (bruceb)
**		Added messages E_FI1FE7 and E_FI2022 (multiple activate
**		timeout blocks per context).
**	21-dec-88 (bruceb)
**		Added message E_FI1FE8 (set/inquire displayonly on a table
**		field.)
**	31-jan-89 (bruceb)
**		Added message E_FI1FE9 (set/inquire displayonly or invisible
**		on an old form.)
**	03-feb-89 (bruceb)
**		Added E_FI1FEA (format unsuitable for datatype) and
**		E_FI1FEB (format wrong size for the field).
**	06-feb-89 (bruceb)
**		Added E_FI1FEC (set/inquire format on a table field.)
**	07-feb-89 (bruceb)
**		Added E_FI1FED (set format on reverse field) and
**		E_FI1FEE (set reverse format on a field.)
**	13-feb-89 (bruceb)
**		Added E_FI1FEF (set format on field containing bad data.)
**	22-feb-89 (bruceb)
**		Added E_FI1FF0 (set justified format on scrolling field.)
**	03/23/89 (dkh) - Fixed venus bug 4556.
**	04/07/89 (dkh) - Fixed error E_FI1F45_8005 and added
**			 error E_FI1F52_8018.
**	14-apr-89 (bruceb)
**		Added E_FI2264, E_FI2265 and E_FI2266 for user setting of
**		the _STATE value.
**	06/27/89 (dkh) - Adding error messages for derivation string parser.
**	29-jun-89 (bruceb)
**		Added error message for attempt to set on/off displayonly
**		attribute for a derived field.
**	12-jul-89 (bruceb)
**		Added error message for attempt to directly modify a
**		derived field.
**	08/20/89 (dkh) - Put in more error message for derived fields.
**	26-sep-89 (bruceb)
**		Added messages telling the runtime user that activation has
**		been requested for derived fields or columns.
**	03-oct-89 (bruceb)
**		Fixed a number of typos in the derivation error messages.
**	12-oct-89 (bruceb)
**		Added messages related to dynamically changing the mode
**		of a table field.
**	01/12/90 (dkh) - Changed text of E_FI2263 message to indicate
**			 that a popup will be displayed as a fullscreen
**			 if it is too large.
**	19-jan-90 (bruceb)
**		Changed 'many' of the derivation error messages to not
**		include quote marks around the form name--the parameter
**		itself will contain the quote marks when 'necessary'.
**	16-apr-90 (bruceb)
**		Minor modification to E_FI21CA (mandatory field msg), and
**		creation of E_FI226E as new msg for mandatory/clearrest.
**	03-jul-90 (bruceb)	Fix for 31018.
**		Parameterize msgs 8600-8601, 8650-8655, and 8814 to state
**		a field's title and internal name, internal name, or
**		'current field', depending on what's available.  OK to do
**		this because 6.4 uses a new message file format.  NOTE:
**		the '%0c' in these msgs refers to strings in the erfd.msg
**		file.  Common usage is "... the %0c", where %0c is replaced
**		by "current field", "field (title: Address, internal name:
**		home_address)", or "field (internal name: _xtitle)".
**	17-jul-90 (bruceb)
**		Changed a number of the messages so that the short version
**		would completely fit on the screen.
**	07/28/90 (dkh) - Added messages for table field cell highlighting.
**	11/26/90 (dkh) - Added messages to handle divide by zero and
**			 overflow exceptions for money and decimal types.
**	09/01/92 (dkh) - Added support for inquiring on existence of
**			 frs objects.
**	09/02/92 (dkh) - Added error messages for dynamic menus.
**	10/02/92 (dkh) - Added new messages to handle unsupported datatypes.
**	07/18/93 (dkh) - Added new messages to support purgetable.
**	07/21/93 (dkh) - Added error message E_FI226F_8815_BAD_PUTROW_STATE
**			 to support the ability to set the _STATE value
**			 at runtime with the putrow statement.
**	07/24/93 (dkh) - Added new messages that used to be hardcoded
**			 in the code.
**	17-dec-1996 (angusm)
**		add E_FI2270_8816_BadDecimal for bug 79824
**/

# define	_FI_CLASS	181



E_FI1F41_8001 "No form named '%0c' found in the forms catalogs."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F42_8002
"There are %0d forms named '%1c' in the forms catalogs.
Only one is allowed."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F43_8003
"Error in allocating form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F45_8005
"Field '%1c' in form '%0c' has an invalid sequence number."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F46_8006
"There are more display-only fields allocated in '%0c' than specified
for this form."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F47_8007
"Error in allocating field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F48_8008
"Error in specifying format string for field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F49_8009
"Error in specifying validation string for field '%1c' in form '%0c'."

E_FI1F4A_8010
"Error in specifying derivation string for field `%1c' in form `%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F4B_8011
"Error retrieving columns for table field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F4C_8012
"Error allocating table field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F4D_8013
"Error allocating memory for table field columns in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F4E_8014
"Too few fields in form '%0c'.  Number present only '%1d'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F4F_8015
"Too few display only fields in form '%0c'.  Number present only '%1d'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F50_8016
"Too few columns for table field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F51_8017
"The column '%2c' does not fit in the table field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1F52_8018
"An error occurred while setting up formatting information for field/column
'%1c' in form '%0c'."

/*
** Explanation:
**	The forms system could not create fmt structures for the
**	field/column.  This could be due to an invalid format
**	string or out of memory or something bad with ADF.
**
** System Status:
**	The form will not be initialized.
**
** User Action:
**	User needs to fix up the form before it can be used.
*/

E_FI1F53_8019
"Unsupported datatype found in a form.\n
Field '%0c' has unsupported datatype '%1d'.  The form
can not be initialized for use."

/*
** Explanation:
**	The forms system found an unsupported datatype for a simple
**	field.  This is probably due to user's playing with the
**	forms catalogs.
**
** System Status:
**	The form will not be initialized.
**
** User Action:
**	User needs to fix up the form before it can be used.
*/

E_FI1F54_801A
"Unsupported datatype found in a form.\n
Column '%1c' in table field '%0c' has unsupported
datatype '%2d'.  The form can not be initialized for use."

/*
** Explanation:
**	The forms system found an unsupported datatype for a table
**	field column.  This is probably due to users playing with
**	the forms catalogs.
**
** System Status:
**	The form will not be initialized.
**
** User Action:
**	User needs to fix up the form before it can be used.
*/

E_FI1FA4_8100
"Could not initialize terminal."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FA5_8101
"Forms statements may not be run without issuing the
FORMS statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FA6_8102
"No form is currently active to run the forms statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FA7_8103
"Form '%0c' has not been initialized by a FORMINIT statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FA8_8104
"Bad display mode '%0c' for form '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FA9_8105
"Field '%1c' not found in form '%0c' for ACTIVATE FIELD statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FAA_8106
"Field '%1c' not found in form '%0c' for GETFORM/GETOPER statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FAB_8107
"Equal sign not found in INGRES/FORMS PARAM format for form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FAC_8108
"Percent sign not found in INGRES/FORMS PARAM format for form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FAF_8111
"Bad datatype format in INGRES/FORMS PARAM format for form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FB0_8112
"Bad pointer in argument vector for INGRES/FORMS PARAM format
for field '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FB1_8113
"Field '%1c' not found in form '%0c' for CLEAR FIELD statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FB2_8114
"Form '%0c' is currently being displayed.  Display statements of the same
form may not be nested."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FB5_8117
"No more than %0d menuitems are allowed in DISPLAY of form '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FB8_8120
"Cannot find frame for table field."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FBA_8122
"Could not set exception handler."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FBF_8127
"Field '%0c', floating point number conversion error."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC0_8128
"Form '%0c' has not been initialized."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC1_8129
"Bad input mode for form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC2_8130
"A datatype conversion error occurred while getting a value 
from form %0c field %1c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC3_8131
"A datatype conversion error occurred while putting a value 
into form %0c field %1c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC4_8132
"A datatype conversion error occurred while getting a 
query operator from form %0c field %1c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC5_8133
"An error occurred in converting the prompt response to
a user variable."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FC6_8134
"A datatype conversion error occurred in a SET/INQUIRE
statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FD6_8150
"Object '%0c' not found for SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FD7_8151
"Insufficient number of arguments for object '%0c' 
in SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FD8_8152
"Form '%0c' not found for SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FD9_8153
"Table field '%0c' not found for SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FDA_8154
"Field '%0c' not found for SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FDB_8155
"Column '%0c' not found for SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FDC_8156
"Attribute '%0c' not found for SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FDD_8157
"Illegal datatype for numeric attribute '%0c' 
in SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FDE_8158
"Illegal datatype for character string attribute '%0c' 
in SET / INQUIRE statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FDF_8159
"Not that many rows in table field for SET/INQUIRE ROW statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE0_8160
"Column '%0c' does not exist for SET/INQUIRE ROW_COLUMN statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE1_8161
"May not set NORMAL display attribute to OFF."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE2_8162
"Unknown command found for SET/INQUIRE MAP or LABEL statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE3_8163
"Unknown control or PF key found for SET/INQUIRE MAP or LABEL statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE4_8164
"Can not SET/INQUIRE on Display Attributes for a Table Field."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE5_8165
"The color value in a SET statement must be between 0 and 7.  Value
specified was '%0d'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE6_8166
"Cannot use '%0c' in a SET_FRS MAP statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI1FE7_8167
"Timeout seconds not in valid range.\n
The number of seconds specified for timeout must be in
the range 0 to %0d.  The timeout value has been set to '%1d'."

/*
** Explanation:
**	User set the number of seconds until timeout to be either too large
**	(> 32767) or negative.
**
** System Status:
**	Timeout value will instead be set to the extreme value allowable
**	at the appropriate end of the range, 0 or 32767.
**
** User Action:
**	None.
*/

E_FI1FE8_8168
"DISPLAYONLY not allowed for Table Fields.\n
The DISPLAYONLY attribute may not be SET or INQUIRED about for table fields.
Only simple fields and table field columns may have the DISPLAYONLY attribute."

/*
** Explanation:
**	User attempted to set or inquire about the displayonly attribute on
**	a table field.  Only simple fields or table field columns may have
**	this attribute.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	None.
*/

E_FI1FE9_8169
"Using new attribute with old form.\n
The use of SET and INQUIRE of %0c against an old form is prohibited.
Forms with version number less than 7 need to be resaved through VIFRED
in order to be used with these statements."

/*
** Explanation:
**	User attempted to set or inquire on the displayonly or
**	invisible attributes on an unconverted form (one with version
**	number less than 7.)  Such old forms must be updated (resaved)
**	through VIFRED in order to use these set or inquire statements.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	Resave the form in VIFRED.
*/

E_FI1FEA_8170
"New format unsuitable for datatype.\n
The new format (%0c) is unsuitable for field (column) %1c's
%2c datatype."

/*
** Explanation:
**	The new format specified by the user for this field is
**	not compatible with the field's datatype.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	Change the application code.
*/

E_FI1FEB_8171
"The new format is too big.\n
The area covered by the new format (%0c) for
field (column) %1c is too big.  The number of rows or columns for
this format exceeds that of the original field (column).
The original format covered %2d row(s) and %3d column(s)."

/*
** Explanation:
**	The new format specified by the user for this field is
**	a bad size for the field.  It's too large (larger than
**	the original size of the field.)
**
** System Status:
**	Ignore this request.
**
** User Action:
**	Change the application code.
*/

E_FI1FEC_8172
"FORMAT not allowed for Table Fields.\n
A display format may not be SET or INQUIREd about for table fields.
Users may only SET or INQUIRE about a display format for
simple fields and table field columns."

/*
** Explanation:
**	User attempted to set or inquire about a display format for
**	a table field.  Only simple fields or table field columns may
**	do this.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	None.
*/

E_FI1FED_8173
"Cannot change FORMAT for reverse field.\n
A new display format may not be SET for reverse format fields (columns)
such as %0c.  Reverse format fields (columns) are those character fields
(columns) whose contents are entered and displayed right-to-left."

/*
** Explanation:
**	User attempted to set a display format for a reverse field.
**	This is not allowed.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	None.
*/

E_FI1FEE_8174
"Cannot set field to a reverse format.\n
Field (column) %0c may not have its display format reset so that
data is entered and displayed in the reverse (right-to-left) direction.
This means that the 'r' indicator may not be included in the format
specification."

/*
** Explanation:
**	User attempted to set a reverse display format for a field.
**	This is not allowed.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	None.
*/

E_FI1FEF_8175
"Display format will not be changed.\n
Data in field (column) %0c failed validation.
Until this is corrected the field (column) will not be reformatted."

/*
** Explanation:
**	User attempted to set a display format for a field for
**	which the data failed validation.  Until the data passes
**	validation, this reformatting will not be allowed.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	Fix the data and try again.
*/

E_FI1FF0_8176
"Cannot set scroll field to justified format.\n
Field (column) %0c is a scrolling field, and such fields (columns)
may not use justified display formats.  This includes right justified (+),
left justified (-) and centered (*) displays."

/*
** Explanation:
**	User attempted to set a justified format for a scrolling field.
**	This is not allowed.
**
** System Status:
**	Ignore this request.
**
** User Action:
**	None.
*/

E_FI1FF1_8177
"A form name must be specified when inquiring on the existence of
a form."

/*
** Explanation:
**	User code attempted to inquire on the existence of a form
**	without passing down a name to use.  This is a back stop
**	in case pre-processors didn't catch it or user pass an
**	empty name in a host variable.
**
** System Status:
**	Ignore this request.
**
**  User Action:
**	None for end user.  Programmer must fix code.
*/

E_FI1FF2_8178
"A field name must be specified when inquiring on the existence of
a field."

/*
** Explanation:
**	User code attempted to inquire on the existence of a field
**	without passing down a name to use.  This is a back stop
**	in case pre-processors didn't catch it or user pass an
**	empty name in a host variable.
**
** System Status:
**	Ignore this request.
**
**  User Action:
**	None for end user.  Programmer must fix code.
*/

E_FI1FF3_8179
"A column name must be specified when inquiring on the existence of
a table field column."

/*
** Explanation:
**	User code attempted to inquire on the existence of a column
**	without passing down a name to use.  This is a back stop
**	in case pre-processors didn't catch it or user pass an
**	empty name in a host variable.
**
** System Status:
**	Ignore this request.
**
**  User Action:
**	None for end user.  Programmer must fix code.
*/

E_FI1FF4_8180
"There are no menuitems to operate on for form '%0c'."

/*
** Explanation:
**	User code specified a form for a set/inquire menu statement
**	that have no defined menuitems.
**
** System Status:
**	Ignore this request.
**
**  User Action:
**	Noe for end user.  Programmer must fix code.
*/

E_FI1FF5_8181
"Menuitem '%0c' does not exist for form '%1c'."

/*
** Explanation:
**	User code specified a menuitem name for a set/inquire menu statement
**	that does not exist for the specified form.
**
** System Status:
**	Ignore this request.
**
**  User Action:
**	Noe for end user.  Programmer must fix code.
*/

E_FI1FF6_8182
"Form '%0c' is not currently active.  Thus there are no menuitems
to SET/INQUIRE on."

/*
** Explanation:
**	User code specified a form for a set/inquire menu statement
**	that is not currently active.  That is, the form is not
**	in any active display loop.
**
** System Status:
**	Ignore this request.
**
**  User Action:
**	Noe for end user.  Programmer must fix code.
*/

E_FI1FF7_8183
"Unknown attribute (%0d) passed in a SET / INQUIRE statement."

/*
** Explanation:
**	The forms system was passed an invalid attribute for the
**	set/inquire setup statement.
**
** System Status:
**	The forms system will stop executing the set/inquire statement
**	and continue execution with the next statement.
**
** User Action:
**	Programmer needs to fix the code.
*/

E_FI1FF8_8184
"Unknown object type (%0d) passed to a SET / INQUIRE statement."

/*
** Explanation:
**	The forms system was passed an unknown object type for the
**	set/inquire statement.
**
** System Status:
**	The forms system will stop executing the set/inquire statement
**	and continue execution with the next statement.
**
** User Action:
**	Programmer needs to fix the code.
*/

E_FI2008_8200
"Field '%0c' not found for GETFORM statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI200C_8204
"Table field '%0c' used in a GETFORM statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI200D_8205
"Invalid conversion from or to COBOL packed decimal."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2012_8210
"Field '%0c' not found for PUTFORM statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2017_8215
"Field '%0c' not found for RESUME FIELD statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2018_8216
"Column '%1c' in table field '%0c' not found for 
RESUME COLUMN statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2019_8217
"FRSKEY '%0d' has already been activated on in current display loop."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI201B_8219
"Control character(s) found in menuitem, converted to blank(s)."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI201C_8220
"Ignoring RESUME MENU statement since there is no menu to resume to."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI201D_8221
"Could not allocate memory for forms system control block."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI201E_8222
"Program variable does not have null indicator.\n
Field \"%1c\" in form \"%0c\" contains a NULL value.  However,
the program variable where the field value is to be placed
does not have an attached null indicator.  It will not be
possible to place the NULL value in field \"%1c\" into the program
variable."

/*
** Explanation:
**	A getform tried to put a NULL value into a program
**	variable that does not have an attached null indicator.
**
** System Status:
**	The getform statement will be skipped.
**
** User Action:
**	Need to change the code or the field.
*/

E_FI201F_8223
"Can not place a NULL value into field \"%1c\" of form \"%0c\".  The datatype
for the field is non-nullable."

/*
** Explanation:
**	A putform tried to put a NULL value into a field that is
**	non-nullable.
**
** System Status:
**	The putform statement will be skipped.
**
** User Action:
**	Need to change the code or the field.
*/

E_FI2020_8224
"Specifying an explicit row number is not allowed.\n  You may not
specify an explicit row number for the following statements:\n
inquire_frs row \"form\" \"table\" (var = change(column)) or\n
set_frs row \"form\" \"table\" (change(column) = var)"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2021_8225
"Value may only be zero(0) or one(1).\n  The value for setting a
change bit may on be zero or one.  Please check your code."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2022_8226
"Multiple TIMEOUT blocks not allowed.\n
Only one ACTIVATE TIMEOUT block is allowed in a single 'display',
'display submenu' or 'submenu' statement.  Only the first ACTIVATE
TIMEOUT block will be used."

/*
** Explanation:
**	User specified more than one activate timeout block for a given
**	display, display submenu or submenu context.  Only one per
**	such context is allowed.
**
** System Status:
**	Ignore all but the first activate timeout block in the given
**	context.
**
** User Action:
**	None.
*/

E_FI2023_8227
"Row number not allowed.\n  You may not specify an explicit row number
when setting or inquiring on dataset attributes from within an
unloadtable loop."

/*
** Explanation:
**	Application specified an explicit row number in a set/inquire
**	statement on attributes.  This is not allowed when executed
**	in the context of an unloadtable loop.
**
** System Status:
**	The FRS will ignore the request and continue processing.
**
** User Action:
**	None.
*/

E_FI2024_8228
"Can't store attributes in a bare table field.\n  The attributes
specified with the 'insertrow' statement have been ignored since
the target table field is a bare table field."

/*
** Explanation:
**	The application executed an insertrow with attribute settings.
**	Unfortunately, the target table field is a bare table field
**	so there is no dataset to store the attributes in.
**
** System Status:
**	The FRS will ignore the request and continue processing.
**
** User Action:
**	None.
*/

E_FI203A_8250
"Cannot use LOADTABLE/UNLOADTABLE without 
a previous INITTABLE '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI203B_8251
"Table field '%0c' does not exist in form '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI203C_8252
"No table name given in table field statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI203D_8253
"Cursor must be on a specific row of table field '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI203E_8254
"Row number '%0d' out of range in table field '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI203F_8255
"No column name given for table field statement on '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2041_8257
"Column '%1c' not found in table field '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2042_8258
"Illegal SCROLL mode '%0c' given for scrolling table field '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2043_8259
"Table field '%0c' is empty; cannot SCROLL."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2044_8260
"Table field '%0c' is empty; cannot DELETE ROW."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2045_8261
"Table field '%0c' is empty; cannot GET ROW."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2046_8262
"Cannot initialize table field '%1c' with hidden column '%0c' 
that already exists."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2047_8263
"Cannot initialize table field '%0c' with column type '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2048_8264
"Cannot nest UNLOADTABLE statement's with table field '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2049_8265
"Table field '%0c' is linked to a data set; cannot use IN list
with DELETE ROW statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI204A_8266
"Table field '%0c' is linked to a data set; cannot use IN list
with SCROLL statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI204B_8267
"Illegal initializing mode '%1c' used with INITTABLE '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI204C_8268
"Cannot request '%1c' without a previous INITTABLE '%0c.'"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI204D_8269
"Record/row number '%0d' out of range in table field '%1c'
in SCROLL TO statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI204E_8270
"Table field '%0c' must be in QUERY mode for requesting operator."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2050_8272
"Illegal request for QUERY operator for hidden column '%1c'
in table field '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2053_8275
"A datatype conversion error occurred while putting a value 
into tablefield %0c column %1c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2054_8276
"A datatype conversion error occurred while getting a value
from tablefield %0c column %1c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2055_8277
"A datatype conversion error occurred while putting a query
operator value into tablefield %0c column %1c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2057_8279
"Can not place NULL value from column \"%1c\" of tablefield \"%0c\"
into program variable.  Program variable not specified with
a null indicator."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2058_8280
"Can not place a NULL value into column \"%1c\" of tablefield \"%0c\".
The datatype for the column is non-nullable."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2059_8281
"Can not use PURGETABLE on a bare tablefield\n.  Tablefield \"%0c\"
is a bare table field.  The PURGETABLE statement may not be used
on bare tablefields.  Please initialize the tablefield (with the
INITTABLE statement) or check that the correct tablefield name has been
specified for the PURGETABLE statement."

/*
** Explanation:
**	The user specified a bare table field in a purgetable statement.
**	This is illegal.  Purgetable may only be used on tablefields that
**	have datasets.
**
** System Status:
**	The forms system will stop processing the purgetable statement.
**	The next statement in user code will be executed.
**
** User Action:
**	User must pass an initialized table field to purgetable or
**	initialize the tablefield before calling purgetable.
*/


E_FI205A_8282
"PURGETABLE may not be used while unloadding the tablefield.\n  The
PURGETABLE statement may not be used on tablefield \"%0c\" since it
is currently being unloadded with the UNLOADTABLE statement.  Please update
the code so that PURGETABLE is executed before the UNLOADTABLE
statement begins or after the UNLOADTABLE statement completes."

/*
** Explanation:
**	The user tried to execute a purgetable statement on
**	a tablefield that is being unloaded via the unloadtable
**	statement.
**
** System Status:
**	Processing of the purgetable statement will stop.  The next
**	statement in the user's code will be executed.
**
** User Action:
**	User must change the code so that purgetable is executed before the
**	unloadtable begins or after the it completes
*/

E_FI206C_8300
"Parser error occurred in module \"%0c\" for field \"%2c\" in form \"%1c\".
'%3c' Last token read in is '%4c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI206D_8301
"Validation error in form '%0c', field '%1c'.
Control character found in validation string.
Character was '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI206E_8302
"Validation error in form '%0c', field '%1c'.
Unknown character found in validation string.
Last token read in is '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2070_8304
"Validation error in form '%0c', field '%1c'.
Floating point number conversion error in validation string.
Last token read in is '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2071_8305
"Validation error in form '%0c', field '%1c'.
Name too long in validation string.
Last token read in is '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2072_8306
"Validation error in form '%0c', field '%1c'.
Unknown operator found in validation string.
Last token read in is '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2073_8307
"Validation error in form '%0c', field '%1c'.
Non-terminated string in validation string.
Last token read in is '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2075_8309
"Validation error in form '%0c', field '%1c'.
Bad character found in validation string.
Character was '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2077_8311
"There is no field named '%1c' in form '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2078_8312
"Form field has a validation string that refers to a table
in an unopened database (e.g., field1 in table.column).
Program must contain and execute the ## ingres statement
before such a validation string can be processed."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2080_8320
"Name too long.\n
While parsing the derivation string for field/column `%1c' in form `%0c',
the parser found a name that was too long.  Last token read was
`%2c'."

E_FI2081_8321
"Floating point number conversion error.\n
While parsing the derivation string for field/column `%1c' in form `%0c',
the parser could not convert a string to a floating point number.
Last token read was `%2c'."

E_FI2082_8322
"Invalid operator found.\n
An invalid operator was encountered in the input while parsing
the derivation string for field/column `%1c' in form `%0c'.  Last
token read was `%2c'."

E_FI2083_8323
"Ran out of input.\n
The derivation parser ran out of input while parsing the derivation
string for field `%1c' in form `%2c'.  This is probably due to
an unterminated string.  Last token read was `%2c'."

E_FI208A_8330
"Internal struct alloc error for derivation.\n
The forms system could not allocate certain internal structures
necessary to derivation execution of field `%1c' in form %0c.
This error makes the form unusable."

E_FI208B_8331
"Internal struct setup error for derivation.\n
The forms system could not initialize internal structures
necessary to derivation execution of field `%1c' in form %0c.
This error makes the form unusable."

E_FI208C_8332
"Error completing derivation execution setup.\n
The forms system failed to complete the setup of
internal structures necessary to derivation execution of field `%1c'
in form %0c.  This error makes the form unusable."

E_FI208D_8333
"Error filling derivation execution structs.\n
The forms system encountered an error while filling in internal
structures necessary for derivation execution.  This occurred
for field `%1c' in form %0c.  The form is not usable
at this time."

E_FI208E_8334
"Error alloc'ing derivation aggregate structs.\n
The forms system could not allocate structures needed for executing
derivation aggregates.  This occurred for field `%1c'
in form %0c.  The form is not usable at this time."

E_FI208F_8335
"Error initializing aggregate structures.\n
The forms system could not initialize the structures needed for
derivation aggregate execution.  This occurred for field `%1c'
in form %0c.  The form is not usable at this time."

E_FI2090_8336
"Error filling aggregate structures.\n
The forms system could not fill in structures used for
derivation aggregate execution.  This occurred for field `%1c'
in form %0c.  The form is not usable at this time."

E_FI2091_8337
"Error completing aggregate execution setup.\n
The forms system could not complete the initialization of
structures used in aggregate execution.  This occurred for
field `%1c' in form %0c.  The forms is not usable
at this time."

E_FI2094_8340
"Internal struct alloc error for derivation.\n
The forms system could not allocate certain internal structures
necessary to derivation execution of column `%1c' for
table field `%2c' in form %0c.  This error makes the form unusable."

E_FI2095_8341
"Internal struct setup error for derivation.\n
The forms system could not initialize internal structures
necessary to derivation execution of column `%1c' for
table field `%2c' in form %0c.  This error makes the form unusable."

E_FI2096_8342
"Error completing derivation execution setup.\n
The forms system failed to complete the setup of
internal structures necessary to derivation execution of column `%1c'
for table field `%2c' in form %0c.  This error makes the form unusable."

E_FI2097_8343
"Error filling derivation execution structs.\n
The forms system encountered an error while filling in internal
structures necessary for derivation execution.  This occurred
for column `%1c' of table field `%2c' in form %0c.  The form
is not usable at this time."

E_FI209E_8350
"Invalid char found in derivation string.\n
An invalid character `%2c' has been found while parsing
the derivation string for field `%1c' in form %0c."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI209F_8351
"Unknown field specified in derivation string.\n
The derivation string for field `%1c' in form %0c
references non-existent field `%2c'."

E_FI20A0_8352
"Unknown field specified in derivation string.\n
The derivation string for column `%1c' of table field `%2c'
in form %0c references non-existent field `%3c'."

E_FI20A1_8353
"Derivation may not reference a table field.\n
The derivation string for field `%1c' in form %0c
may not reference a table field (`%2c').  Derivations
for a simple field may only depend on constants,
other simple fields or the aggregate value of a
table field column.  For a table field column, it
may depend on another column in the same table field
or a constant value."

E_FI20A2_8354
"Derivation may not reference a table field.\n
The derivation string for column `%1c' of
table field `%2c' in form %0c
may not reference a table field (`%3c').  Derivations
for a simple field may only depend on constants,
other simple fields or the aggregate value of a
table field column.  For a table field column, it
may depend on another column in the same table field
or a constant value."

E_FI20A3_8355
"A column may not depend on a simple field.\n
The derivation string for column `%1c' of table field `%2c'
in form %0c references simple field `%3c'.  This is
illegal.  A table field column may only depend on another
column in the same table field or a constant value."

E_FI20A4_8356
"A field may not depend on itself.\n
The derivation string for field `%1c' in form %0c
references itself.  This is illegal.  A derived field may
never depend on itself."

E_FI20A5_8357
"Unknown table field in derivation string.\n
Derivation string for field `%1c' in form %0c
references non-existent table field `%2c'."

E_FI20A6_8358
"Unknown table field in derivation string.\n
Derivation string for column `%1c' of table field `%2c'
in form %0c references non-existent table field `%3c'."

E_FI20A7_8359
"Using a simple field as a table field.\n
Derivation string for field `%1c' in form %0c
uses simple field `%2c' where a table field
is required."

E_FI20A8_8360
"Using a simple field as a table field.\n
Derivation string for column `%1c' of table field `%2c'
in form %0c uses simple field `%3c' where a table field
is required."

E_FI20A9_8361
"A simple field may not depend on a column.\n
Derivation string for field `%1c' in form %0c references
a table field column (`%2c.%3c') directly.  This is illegal.
A simple field
may only depend on constants, other simple fields or the
aggregate value of a table field column."

E_FI20AA_8362
"A column may not depend on another table field.\n
The derivation string for column `%1c' of table field `%2c'
in form %0c references column `%4c' of table field `%3c'.
This is illegal.  A derived column may only depend on constants
or another column in the SAME table field."

E_FI20AB_8363
"Unknown column in derivation string.\n
The derivation string for field `%1c' in form %0c
references column `%3c' which does not exist in table
field `%2c'."

E_FI20AC_8364
"Unknown column in derivation string.\n
The derivation string for column `%1c' of table field
`%2c' in form %0c references column `%4c' which does
not exist in table field `%3c'."

E_FI20AD_8365
"A column may not depend on itself.\n
The derivation string for column `%1c' of table field `%2c'
in form %0c references itself.  This is illegal.  A derived
column may never depend on itself."

E_FI20AE_8366
"Internal derivation parser error.\n
The derivation parser was requested to create an unknown
node type (`%2d') while parsing the derivation string for
field `%1c' in form %0c."

E_FI20AF_8367
"Internal derivation parser error.\n
The derivation parser was requested to create an unknown
node type (`%3d') while parsing the derivation string for
column `%1c' of table field `%2c' in form %0c."

E_FI20B0_8368
"Internal derivation parser error.\n
The derivation parser was requested to process a node
of an unknown math operator type (`%2d') while parsing the derivation
string for field `%1c' in form %0c."

E_FI20B1_8369
"Internal derivation parser error.\n
The derivation parser was requested to process a node
of an unknown math operator type (`%3d') while parsing the derivation
string for column `%1c' of table field `%2c' in form %0c."

E_FI20B2_8370
"Internal derivation parser error.\n
The derivation parser encountered an error while processing
the `%2c' operator for field `%1c' in form %0c."

E_FI20B3_8371
"Internal derivation parser error.\n
The derivation parser encountered an error while processing
the `%3c' operator for column `%1c' of table field `%2c' in form %0c."

E_FI20B4_8372
"Internal derivation parser error.\n
The derivation parser encountered an error while setting up
execution information for the `%2c' operator.  This happened
while parsing the derivation string for field `%1c' in form %0c.
The error could be the result of 1) incompatibility of the
datatypes on either side of the operator or 2) the
operator is not compatible or not allowed for the datatypes."

E_FI20B5_8373
"Internal derivation parser error.\n
The derivation parser encountered an error while setting up
execution information for the `%3c' operator.  This happened
while parsing the derivation string for
column `%1c' of table field `%2c' in form %0c.
The error could be the result of 1) incompatibility of the
datatypes on either side of the operator or 2) the
operator is not compatible or not allowed for the datatypes."

E_FI20B6_8374
"Unknown table field referenced in aggregate.\n
The derivation string for field `%1c' in form %0c references
table field `%2c' in an aggregate expression.  Table field `%2c'
does not exist."

E_FI20B7_8375
"Unknown table field referenced in aggregate.\n
The derivation string for column `%1c' of table field `%2c'
in form %0c references table field `%3c' in an aggregate
expression.  Table field `%3c' does not exist."

E_FI20B8_8376
"Can't aggregate a simple field.\n
The derivation string for field `%1c' in form %0c uses
simple field `%2c' in an aggregate expression.  This is illegal.
Aggregate expressions must be applied to a table field column."

E_FI20B9_8377
"Can't aggregate a simple field.\n
The derivation string for column `%1c' of table field `%2c' in
form %0c uses simple field `%3c' in an aggregate expression.  This
is illegal.  Aggregates expressions must be applied to a table field
column."

E_FI20BA_8378
"Unknown column in an aggregate.\n
The derivation string for field `%1c' in form %0c specifies
an aggregate expression for table field column (`%2c.%3c').
However, column `%3c' does not exist in table field `%2c'."

E_FI20BB_8379
"Unknown column in an aggregate.\n
The derivation string for column `%1c' of table field `%2c' in
form %0c specifies an aggregate expression for table field
column (`%3c.%4c').  However, column `%4c' does not exist in
table field `%3c'."

E_FI20BC_8380
"A column can not depend on an aggregate.\n
The derivation string for column `%1c' of table field `%2c' in form %0c
specifies an aggregate.  This is illegal.  A table field column
may only depend on constants or another column in the SAME table
field."

E_FI20BD_8381
"Internal derivation parser error.\n
The derivation parser was given an unknown aggregate
type (`%2d') to process.  Parsing of derivation string for field
`%1c' in form %0c will terminate."

E_FI20BE_8382
"Internal derivation parser error.\n
The derivation parser could not process aggregate `%2c'.
Parsing of the derivation string for field `%1c' in form `%0c'
will terminate."

E_FI20BF_8383
"Internal derivation parser error.\n
The derivation parser could not set up execution information
for aggregate `%2c'.  Parsing of the derivation string for field `%1c'
in form `%0c' will terminate."

E_FI20C0_8384
"Internal derivation parser error.\n
The derivation parser encountered an error while trying
to determine the datatype compatibility
between the derivation string and field `%1c' in form %0c.
Parsing of the derivation string will terminate."

E_FI20C1_8385
"Internal derivation parser error.\n
The derivation parser encountered an error while trying
to determine the datatype compatibility
between the derivation string and column `%1c' of table field
`%2c' in form %0c.  Parsing of the derivation string will
terminate."

E_FI20C2_8386
"Incompatible datatypes for field and derivation string.\n
The datatypes for field `%1c' in form %0c and its
derivation string are incompatible.  Datatype for the field
is `%2c' while the datatype for the derivation string is `%3c'."

E_FI20C3_8387
"Incompatible datatypes for field and derivation string.\n
The datatypes for column `%1c' of table field `%2c' in form %0c
and its derivation string are incompatible.  Datatype for the
column is `%3c' while the datatype for the derivation string is `%4c'."

E_FI20C4_8388
"Datatype of Field must be NULLable.\n
The result datatype of the derivation string for field `%1c' in form %0c
is NULLable which means the datatype of the field must also be made
NULLable.  Datatype of the field is currently not NULLable."

E_FI20C5_8389
"Datatype of Column must be NULLable.\n
The result datatype of the derivation string for column `%1c' of table
field `%2c' in form %0c is NULLable which means the datatype
of the column must also be made NULLable.  Datatype of the column
is currently not NULLable."

E_FI20C6_8390
"Failed to convert derivation constant.\n
An error occurred while trying to convert `%2c' into a `%3c' value.
This happened for field `%1c' in form %0c.  Please check that the
specified constant is compatible with the datatype of the field."

E_FI20C7_8391
"Failed to convert derivation constant.\n
An error occurred while trying to convert `%3c' into a `%4c' value.
This happened for column `%1c' of table field `%2c' in form %0c.
Please check that the specified constant is compatible with the
datatype of the column."

E_FI20C8_8392
"Numeric overflow has occurred.\n
Numeric overflow occurred while evaluating a derivation formula.
The offending field will be marked as invalid."

E_FI20C9_8393
"Numeric underflow has occurred.\n
Numeric underflow occurred while evaluating a derivation formula.
The offending field will be marked as invalid."

E_FI20CA_8394
"Integer divide by zero has occurred.\n
An integer divide by zero error has occurred while evaluating a
derivation formula.  The offending field will be marked as invalid."

E_FI20CB_8395
"Floating point divide by zero error.\n
A floating point divide by zero error has occurred while evaluating
a derivation formula.  The offending field will be marked as invalid."

E_FI20CC_8396
"Aggregate setup error.\n
An internal error has occurred while setting up aggregate `%2c'
for field `%1c' in form %0c.  Please check that the datatype
for the field is valid for the specified aggregate."

E_FI20CD_8397
"Aggregate processing initialization error.\n
Could not initialize data structures for processing aggregate
`%0c' for field `%1c'.  The error was\n
`%2c'\n"

E_FI20CE_8398
"Aggregate calculation error.\n
An error occurred while calculating aggregate `%0c' for field
`%1c'.  The error was\n
`%2c'\n"

E_FI20CF_8399
"Aggregate result error.\n
An error occurred while extracting the `%0c' aggregate value for
field `%1c'.  The error was\n
`%2c'\n"

E_FI20D0_8400
"Circular reference in derivation string.\n
Derivation string for field `%1c' in form `%0c' contains a circular
reference."

E_FI20D1_8401
"Circular reference in derivation string.\n
Derivation string for column `%1c' of table field `%2c' in form
`%0c' contains a circular reference."

E_FI20D2_8402
"Error parsing a derivation string.\n
A syntax error probably exists in the derivation string
for field/column `%1c' in form %0c.
Please check the derivation string for correct syntax."

E_FI20D3_8403
"Error putting derived value in field/column.\n
An error occurred while trying to place a derived value into
field/column `%0c'.  The error was\n
`%1c'."

E_FI20D4_8404
"Aggregate processing initialization error.\n
Could not initialize compiled data structures for processing aggregate
`%0c' on column `%1c'.  The error was\n
`%2c'\n"

E_FI20D5_8405
"Aggregate calculation error.\n
An error occurred while calculating aggregate `%0c' on column
`%1c'.  The error was\n
`%2c'\n"

E_FI20D6_8406
"Aggregate result error.\n
An error occurred while extracting the `%0c' aggregate value on
column `%1c'.  The error was\n
`%2c'\n"

E_FI20D7_8407
"Derivation calculation error.\n
An error occurred while calculating a derived value for
field/column `%0c'.  The error was\n
`%1c'."

E_FI20D8_8408
"Money divide by zero has occurred.\n
A money divide by zero error has occurred while evaluating a
derivation formula.  The offending field will be marked as invalid."

E_FI20D9_8409
"Decimal divide by zero has occurred.\n
A decimal divide by zero error has occurred while evaluating a
derivation formula.  The offending field will be marked as invalid."

E_FI20DA_8410
"Money overflow has occurred.\n
Money overflow occurred while evaluating a derivation formula.
The offending field will be marked as invalid."

E_FI20DB_8411
"Decimal overflow has occurred.\n
Decimal overflow occurred while evaluating a derivation formula.
The offending field will be marked as invalid."

E_FI2102_8450
"Could not convert form \"%0c\" to 6.0 format."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2198_8600
"Value in field failed the validation check.\n
Please check that you have entered a correct value into
the %0c."

/*%
**  Explanation:
**	You have entered a value that did not pass the validation
**	check that exists for the field.  You must correct
**	value for the field.
**
**  System Status:
**	The forms system has rejected the value for the field.
**	Processing will continue after user has cleared the error
**	message.
**
**  User Action:
**	Correct value in the field.
*/

E_FI2199_8601
"%1c\n
Value in field failed the validation check.
Please check that you have entered a correct value into
the %0c."

/*%
**  Explanation:
**	You have entered a value that did not pass the validation
**	check that exists for the field.  You must correct
**	value for the field.  This is essentially the
**	same as error 8600 except that we print out user's error
**	message first.
**
**  System Status:
**	The forms system has rejected the value for the field.
**	Processing will continue after user has cleared the error
**	message.
**
**  User Action:
**	Correct value in the field.
*/

E_FI21CA_8650
"No value found for the field.\n
The %0c is a mandatory field and is not allowed
to contain only blanks or the NULL value.
This error can be triggered by the application trying to get
a value from the field, by trying to move the cursor off the field
or by entering the clearrest key.
Please check that the field has a non-NULL non-blank value."

/*%
**  Explanation:
**	The field is a mandatory field.  This means that the
**	field may not be left entirely blank and that the NULL value
**	is also invalid for the field.  The user needs to enter
**	a non-NULL and non-blank value into the field.
**
**  System Status:
**	The forms system has rejected the blanks or NULL value in
**	the current field.
**
**  User Action:
**	Please enter a non-NULL or non-blank value into the
**	field.
*/

E_FI21CB_8651
"The field only accepts a valid integer.\n
Character data is not allowed in the %0c; please enter an integer.
Check that the value entered into this field is not too large
for this integer.  An integer number is normally composed
of the digits zero through nine (0-9) with an optional leading
'+' or '-'.  Please check what you have entered into the
field and remove any alphabetic and punctuation characters."

/*%
**  Explanation:
**	The forms system has encountered invalid characters that have
**	been entered into the field.  Alternatively, the number
**	entered is too large for the associated integer type.
**	Since the field
**	is an integer field, the types of characters that may
**	be entered is limited to the digits 0 through 9 and a leading
**	"-" or "+" character.
**
**  System Status:
**	The forms system has rejected what has been entered into the
**	field.  User needs to correct the data in the field.
**
**  User Action:
**	Please check what has been entered into the field.
**	Remove any alphabetic and punctuation characters that are
**	in the field.
*/

E_FI21CC_8652
"Field only accepts a floating point number.\n
Character data is not allowed in the %0c; please enter a
floating point number.
Check that the value entered into this field is not too large for this float.
A floating pointer number is normally composed
of the digits zero through nine (0-9) with an optional decimal point.
Please check what you have entered into the field and remove
any alphabetic and punctuation characters."

/*%
**  Explanation:
**	The forms system has encountered invalid characters that have
**	been entered into the field.  Alternatively, the number
**	entered is too large for the associated floating point type.
**	Since the field
**	is a floating point field, the types of characters that may
**	be entered is limited to the digits 0 through 9, the decimal
**	point, the comma, the "-" and "+" sign and the exponential
**	"e" (if the format for the current field is the exponential "e"
**	format).
**
**  System Status:
**	The forms system has rejected what has been entered into the
**	field.  User needs to correct the data in the field.
**
**  User Action:
**	Please check what has been entered into the field.
**	Remove any alphabetic and punctuation characters that are
**	in the field.  A floating point number may be entered
**	and displayed in many ways, please consult the VIFRED and
**	REPORT WRITER manuals for the various possible formats.
*/

E_FI21CD_8653
"Can not convert data entered in the field.\n
Please check and correct any errors in the %0c.  The usual source
of errors is mistyping data into the field.  Another possibility is that
the entered data does not conform to the format for the field.  This
may be true for fields that have numeric and date templates as formats.
If the current application is one that is supplied by Ingres
Corporation, one can see the format for a given field by
selecting the \"ListChoices\" menuitem (if it exists) to display either a
list of valid values or various information (including the format) related to
the field containing the cursor."

/*%
**  Explanation:
**	The data entered into the field could not be converted
**	into an internal value.  Reasons may be invalid characters
**	entered into the field, but more probably is that the data
**	does not conform to the format for the field.  The
**	latter is a possibility if the field has numeric or date
**	templates as the format specification.
**
**  System Status:
**	The forms system has rejected what has been entered into the
**	field.  User needs to correct the data in the field.
**
**  User Action:
**	Please check and correct any typos in the field.  Common errors are
**	transpositions of characters, entering punctuation where they
**	are not allowed and entering data that does not conform to
**	the format for the field.  The user may see the format for
**	the field by selecting the "ListChoices" menuitem,
**	if that facility is available.
*/

E_FI21CE_8654
"The value entered is too large.\n
You have entered a number (floating point or integer) that is too
large for the %0c.  Please enter a smaller number."

/*%
**  Explanation:
**	The field holds either an integer or floating point
**	number.  The user has entered a value that is too large
**	for the field.  Please note that a one byte integer may
**	only hold values from -128 to +127, a two byte integer may
**	only hold values form -32,768 to +32,767 and a four byte
**	integer may only hold values from -2,147,483,648 to
**	+2,147,483,647.  For a four byte integer, the approximate
**	range of values is (+ or -) 0.29 X 10**-38 to 1.7 X 10**38 with
**	7 digits of precision.  For an eight byte integer, the
**	approximate range of values is (+ or 1) 0.56 X 10**-308 to
**	0.9 X 10**308 with 15 digits of precision.  The data entered
**	into the field exceeded these limits.
**
**  System Status:
**	The forms system has rejected the data in the field and
**	will continue with processing.
**
**  User Action:
**	Please enter a smaller number into the numeric field.
*/

E_FI21CF_8655
"The value entered is too small.\n
You have entered a floating point number that is too small for the
%0c.  Please enter a larger number."

/*%
**  Explanation:
**	The field holds a floating point number.  The user has
**	entered a value that is too small for the field.  Please remember
**	that a four byte floating point number only has 7 digits of
**	precision and an eight byte floating point number only has
**	15 digits of precision.  The data in the field has specified
**	a precision greater than the specified datatype for the field.
**
**  System Status:
**	The forms system has rejected the data in the field and
**	will continue with processing.
**
**  User Action:
**	Please enter a larger number into the floating point field.
*/

E_FI21D0_8656
"Default value truncated.\n  The default value for field \"%0c\"
was truncated to fit into the field."

/*%
**  Explanation:
**	The default value for the specified field could not all
**	fit into the field.  The value was truncated.
**
**  System Status:
**	The forms system will continue processing.
**
**  User Action:
**	End user should notify the support person for the application
**	they are using.  An application builder should check to length
**	of a field to make sure that the default value can fit.
*/

E_FI21FC_8700
"A NULL function pointer was passed to FDrngchk"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI21FD_8701
"The source data value for FDrngchk is not long text or has a null pointer"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_FI2260_PopBox
"Too small a number of rows or columns for a pop up %0c.   Popup
must be specified with %1d or more rows (or default), and %2d or more
columns (or default).  The %0c will not be displayed."

/*
**  Description:
**	Application code has specified a popup box for a display item
**	which is too small.
**
**  System Status:
**	System will simply ignore request.  If a prompt, an empty
**	response buffer will be returned to the application.
**
**  User Action:
**	Application designer should specify a larger box.
*/

E_FI2261_PopStart
"Illegal starting location for a pop up %0c.   Popup
starting location must be a positive row number (or default), and a positive
column number (or default).  The %0c will not be displayed."

/*
**  Description:
**	Application code has specified a popup box for a display item
**	which starts at an illegal screen location.
**
**  System Status:
**	System will simply ignore request.  If a prompt, an empty
**	response buffer will be returned to the application.
**
**  User Action:
**	Application designer should specify a legal start location.
*/

F_FI0001_Message	"message"
F_FI0002_Prompt		"prompt"

/*
** The nouns used for %0c arguments of E_FR005A_PopBox and E_FR005B_PopStart
*/

E_FI2262_PopMove
"WARNING: Popup form '%0c' would not fit at position originally specified.
Form will be displayed at an adjusted position which will allow
it to fit on the screen."

/*
**  Description:
**	Application code has attempted to display a popup form at a location
**	which is partially or completely off-screen.
**
**  System Status:
**	Form will be shifted to an on-screen location.
**
**  User Action:
**	Application designer should place form so it fits on the screen.
*/

E_FI2263_PopTooBig
"Form '%0c' is too large to display as a popup.  It will be displayed
as a fullscreen form."

/*
**  Description:
**	Application code has attempted to display a popup form which
**	is too large for the screen.
**
**  System Status:
**	Display will not take place.
**
**  User Action:
**	Application designer should define a form which fits on the screen.
*/

E_FI2264_8804_no_STATE
"Not allowed to set _STATE value.\n
Attempt to set a row's _STATE value in the '%0c' table field will be ignored.
_STATE may only be set in a LOADTABLE statement and in an INSERTROW
statement to an initialized table field."

/*
**  Description:
**	Attempt made to set _STATE in a statement for which this is not
**	allowed.  This includes any statement other that LOADTABLE and
**	INSERTROW to a table field with a data set.
**
**  System Status:
**	_STATE will not be modified.
**
**  User Action:
**	None.
*/

E_FI2265_8805_bad_STATE_val
"Attempt to set _STATE to a bad value.\n
_STATE may only be set to UNDEFINED (0), NEW (1),
UNCHANGED (2) or CHANGED (3).
The row value of _STATE in the '%0c' table field will not be modified."

/*
**  Description:
**	Attempt made to set _STATE to a value other than 0, 1, 2 or 3.
**
**  System Status:
**	_STATE will not be modified.
**
**  User Action:
**	None.
*/

E_FI2266_8806_data_in_undef
"Can't set UNDEFINED _STATE with other data.\n
_STATE should NOT be set to a value of UNDEFINED (0) when
other visible columns are having values set in this row of the '%0c' table
field.  This warning should be considered when unloading the table field
and checking row states."

/*
**  Description:
**	Attempt made to set _STATE to a value of 0 while also setting
**	the values of other visible columns in the row.  This is discouraged
**	though not prohibited.
**
**  System Status:
**	None.
**
**  User Action:
**	None.
*/

E_FI2267_8807_SetDerived
"Can not directly modify a DERIVED field.\n
Derived fields and columns can not be cleared, or have their value set, by
application code.  The only way that the contents of a derived field
or column can be modified is by modifying the fields
or columns referenced by the derivation formula for that field."

/*%
**  Description:
**	Attempt made to clear a derived field or column or to set the
**	value of such a field.  This is not allowed.
**
**  System Status:
**	Ignore the request.
**
**  User action:
**	None.
*/

E_FI2268_8808_DispDerived
"Can't use DISPLAYONLY with DERIVED fields.\n
Derived fields and columns can not be moved to by the user.  This inability
is separate from the DISPLAYONLY attribute which may not be used on
derived fields or columns.  The DISPLAYONLY status of '%0c' will not
be changed."

/*%
**  Description:
**	Attempt to modify the DISPLAYONLY status of a DERIVED field
**	is not allowed and will be ignored.
**
**  System Status:
**	Ignore the request.
**
**  User action:
**	None.
*/

E_FI2269_8809_ActDerivedFld
"Activation requested on a DERIVED field.\n
Field %0c is a derived field.  Derived fields cannot be moved to by
the user, so an activation on such a field will never be triggered."

/*%
**  Description:
**	The user requested an entry or exit activation on the named
**	field.  Since users can never land on derived fields, this is
**	essentially a worthless operation.  Warning the user in case
**	this was done by mistake.
**
**  System Status:
**	This is only a warning.
**
**  User action:
**	None.
*/

E_FI226A_8810_ActDerivedCol
"Activation requested on a DERIVED column.\n
Column %0c is a derived column.  Generally, derived columns cannot be moved
to by the user, so an activation on such a column will not be triggered.
The exception to this statement is that if all columns in a table field
are unreachable (DISPLAYONLY, INVISIBLE, DERIVED, or QUERYONLY with the table
field in READ or UPDATE mode) and %0c is the first such column in the table
field, then activations can still occur on that column."

/*%
**  Description:
**	The user requested an entry or exit activation on the named
**	column.  Since users generally can not land on derived columns, this
**	is usually a worthless operation.  Warning the user in case this was
**	done by mistake.
**
**  System Status:
**	This is only a warning.
**
**  User action:
**	None.
*/

E_FI226B_8811_ModeBareTF
"Can't set mode on uninitialized table fields.\n
The SET mode statement may only be used on the '%0c' table field after
it has first been initialized using an INITTABLE statement.  Dynamically
setting a table field mode is not allowed on non-initialized table fields."

/*%
**  Description:
**	The user attempted to dynamically set the mode of a bare
**	table field using the set_frs/set_forms statement.  This is
**	not allowed.
**
**  System Status:
**	The attempt to set the mode failed.
**
**  User action:
**	Use an inittable statement if desired.
*/

E_FI226C_8812_BadTFMode
"Illegal mode '%1c' used in SET statement.\n
An attempt was made to set table field '%0c' to '%1c', an illegal mode.
Legal modes are 'fill', 'query', 'update' and 'read'.
The table field mode will remain unchanged."

/*%
**  Description:
**	The user attempted to dynamically set the mode of a table field
**	to a bad mode using the set_frs/set_forms statement.  The request
**	will be ignored.
**
**  System Status:
**	The attempt to set the mode failed.
**
**  User action:
**	Use a legal mode if so desired.
*/

E_FI226D_8813_TFModeUnchgd
"Invalid data so table field mode unchanged.\n
Setting the mode of table field '%0c' to 'read' from either 'fill' or 'update'
failed because the table field contained invalid data.  Correct the data
before attempting to change the table field mode to 'read'."

/*%
**  Description:
**	The user requested a change to read mode for the table field.
**	This attempt failed because the table field contained invalid data.
**
**  System Status:
**	Deny the request.
**
**  User action:
**	Fix any such data before again attempting to set the mode to 'read'.
*/

E_FI226E_8814_ClrMandFld
"No value found for mandatory field.\n
The %0c is a mandatory field and must contain a non-blank,
non-NULL value.
This error was caused by entering the clearrest key at the start of
the field.  The previous contents of the field will be restored."

/*%
**  Explanation:
**	The field is a mandatory field.  This means that the
**	field may not be left entirely blank and that the NULL value
**	is also invalid for the field.  The user cannot clear the field.
**
**  System Status:
**	The forms system has rejected the blanks in the field.
**
**  User Action:
**	Please enter a non-NULL or non-blank value into the field.
*/

E_FI226F_8815_BAD_PUTROW_STATE
"Bad value for _STATE in a putrow statement.\n  The new value
for _STATE depends on the current value of _STATE.  If the
current value is NEW (1), the new value may be NEW (1), UNCHANGED (2)
or CHANGED(3).  The new value may only be UNCHANGED (2) or CHANGED (3)
if the current value is UNCHANGED (2) or CHANGED (3).  Any other
combination is not currently allowed.  The _STATE value for
table field '%0c' will not be changed."

/*%
**  Explanation:
**	The application requested an unsupported change in _STATE using
**	a putrow statement.  The allowed changes are:  if the current
**	value of _STATE is NEW, then the new value may be NEW,
**	UNCHANGED or CHANGED; if the current value is UNCHANGED or
**	CHANGED then the new value may be UNCHANGED or CHANGED.
**	No other transitions are currently supported.
**
**  System Status:
**	The forms system will not change the _STATE value in the
**	table field.  Execution continues with the next statement.
**
**  User Action:
**	Application writer needs to modify code to follow the rules
**	stated above.
*/
E_FI2270_8816_BadDecimal     
"A bad value was given in II_APP_DECIMAL.\n
The character `%0c' specified by the logical variable II_APP_DECIMAL \
is not an acceptable decimal point character. The character `.' or valid \
value from II_DECIMAL will be used instead. Set the INGRES logical variable \
II_APP_DECIMAL to a valid decimal point character."
 
 /*%
 ** Explanation:
 **      The character given in II_APP_DECIMAL is not a valid decimal point
 **      character.  This is the character you have used in field validation 
 **      statements to specify numeric literals that can have a fractional part,
 **      such as money values or real numbers.  Valid decimal point characters
 **      are `.' or `,'.  Important note: this logical symbol is provided
 **      only for backward compatibility.  It is recommended that you use
 **      a decimal point character of `.' in your field validation code,
 **      and leave II_APP_DECIMAL unset (it will default to `.').
 **
 ** System Status:
 **      The ABF compiler will assume that your field validation statements 
 **	will use a decimal point character of `.' to specify the fractional 
 **	parts of numeric literals.  If your field validation statements in 
 **	fact use some other character, a syntax error will be reported.
 **
 ** User Action:
 **      Set a valid single-character string for the II_APP_DECIMAL name.
 */

