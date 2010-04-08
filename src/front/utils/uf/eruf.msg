/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	eruf.msg -  Front-End Utility UF Message/Error Definitions File.
**
** Description:
**	Contains the message/error definitions for the FRS dependent
**	front-end utilities.
**
** History:
**	Revision 6.0  87/08/11  wong
**	Initial revision.
**	09/02/87 (dkh) - Added error message id "E_UF0007_NOFRS".
**	Revision 6.4  89/10/12  sylviap
**		Added messages for the output browser.
**	Revision 6.3  89/11  marian
**	01-nov-89 (marian)  Removed references to RTI.
**	02-nov-89 (marian)  Replaced "Technical Support Representative" with
**				"Site Technical Contact".
**	02-nov-89 (bruceb)
**		Added messages for listobj routine.
**	20-nov-89 (bruceb)
**		Added messages for code moved here from frs.  The messages
**		reference dyn.c, frmfmt.c, listpck.qsc and stdprmpt.qsc.
**	11-dec-89 (bruceb)
**		Added messages for Help code, moved here from frs.
**	16-jan-90 (bruceb)
**		Help Keys text now reads 'overtype' instead of 'overstrike'.
**	01-may-90 (sylviap)
**		Added E_UF0053 - E_UF0060 for scrollable output popups.
**	22-may-90 (sylviap)
**		Changed scrollable output msgs.  Result is E_UF0053 - E_UF0059.
**	29-aug-90 (kathryn)
**		Add Key explanation messages for Output Browser submenus.
**		ExplPrintOK, ExplPrintPartial, ExplFileOK, ExplFilePartial
**	23-oct-1990 (mgw)
**		Corrected some misspellings.
**	15-mar-91 (sylviap) #32101
**		Added E_UF005A_PrintFailed for bug #32101. 
**	06/20/93 (dkh) - Fixed bug 49638.  Added messages E_UF005B and E_UF005C.
**	15-nov-93 (robf)
**              Add F_UF007F_seclabel for security_label type
**	20-mar-2000 (kitch01)
**		Bug 99136. Add E_UF0080->83 messages for new browsefile read
**		functions.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	07-sep-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**	18-Jul-2007 (kibro01) b118777
**	    Changed "A ANSI..." to "An ANSI"
**	19-May-2009 (kschendel) b122041
**	    Typo in UF0009 found by inspection (%d0 should be %0d).
**/

#define		_UF_CLASS	136

/*
**	Restore Terminal Prompt ("uf/ufrtm.qc".)
*/
F_UF0000	"Press RETURN to clear the screen"
#define	_ClearReturn	ERget(F_UF0000)

/*
**	Edit Field/Column Messages ("uf/fefedit.qc".)
*/
S_UF0001	"Entering editor on field %s."
#define	_EnterEditor	ERget(S_UF0001)
S_UF0002	"Edit file '%s' is empty; field/column value will be unchanged."
#define _EmptyFile	ERget(S_UF0002)

/*
** Name:    IIUF_ERROR -	Front-End UF Error Messages.
*/

E_UF0003_Bad_Create	"Could not create temporary edit file.\n
A temporary file used to edit a field or column value could not be created or
opened.  The field or column value will remain unchanged.  Please check your
temporary directory access rights and disk quota.\nThe system status was %0d."

/*%
** Explanation:
**	A temporary file used to edit the contents of a field or column could
**	not be created and opened.  This could be for any number of reasons,
**	but is most likely a quota or access problem.
**
** System Status:
**	The contents of the field or column will not be changed.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory
**	and sufficient disk quota to create files.
*/

E_UF0004_Bad_Close	"Could not close temporary edit file.\n
The temporary edit file `%0c' could not be closed prior to calling the editor.\ 
The field or column value will remain unchanged.  Please check your temporary
directory access rights and disk quota before reporting this as an internal
problem to your technical representative.\nThe system status was %1d."

/*%
** Explanation:
**	A temporary file used to edit the contents of a field or column could
**	not be closed after being created but before starting the system editor.
**	This is an unusual error that could be a quota or access problem but
**	could also be an internal programming error.
**
** System Status:
**	The contents of the field or column will not be changed.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory
**	and sufficient disk quota to create files before reporting this as
**	an internal problem to your technical representative.
*/

E_UF0005_Bad_Open	"Could not open temporary edit file for reading.\n
The temporary edit file `%0c' could not be opened for input after it had been
edited.  The field or column value will remain unchanged.  Please check your
temporary directory access rights and disk quota before reporting this as an
internal problem to your technical representative.\nThe system status was %1d."

/*%
** Explanation:
**	A temporary file used to edit the contents of a field or column could
**	not be re-opened for reading.  This could be for any number of reasons,
**	but is most likely a quota or access problem.
**
** System Status:
**	The contents of the field or column will not be changed.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory
**	and sufficient disk quota to create files before reporting this as
**	an internal problem to your technical representative.
*/

E_UF0006_Bad_Delete	"Unable to close or delete temporary edit file.\n
The temporary edit file `%0c' could not be closed or deleted, but the field or
column value was already changed.  Please check your temporary directory access
rights and disk quota before reporting this as an internal problem to your
technical representative.\nThe system status was %1d."

/*%
** Explanation:
**	A temporary file used to edit the contents of a field or column could
**	not be closed or deleted after the field or column value(s) had been
**	read.  This is an unusual error that could be a quota or access problem
**	but could also be an internal programming error.
**
** System Status:
**	The contents of the field or column were already changed.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory
**	and sufficient disk quota to create files before reporting this as
**	an internal problem to your technical representative.
*/

/*
**	uffindex.c - forms library access routines.
*/

E_UF0007_NOFRS	"Unable to get form from form index file.\n
The forms system has not been started yet, so obtaining a form from
the form index file may not be done."

/*%
** Explanation:
**	Before getting (and initializing) a form from the form index file the
**	Forms System must already have started up.  Using forms requires that
**	the Forms System be running.  The form index file is ONLY used by INGRES
**	products and this condition is an internal error in that product.
**
** System Status:
**	The requested form is not available for use.  Features that
**	use the requested form may not be accessed.
**
** User Action:
**	Since the form index file is only used by INGRES products, this should
**	be reported as a problem to your Site Technical Contact.
*/

S_UF0008_EndNoSavePrompt	"Do you wish to End without saving changes? "

/*%
** Explanation:
**	Changes have been made to the current form that have not been saved.
**	You must confirm that you wish to end without saving the changes by
**	answering "yes" or "no".
*/

/*
** tfedit.qsc
*/
E_UF0009_Unable_open_temp_edit	"Unable to open temporary edit file (rc=%0d)"
E_UF000A_Unable_to_close_file	"Unable to close file '%0c' (rc=%1d)"
S_UF000B_File_empty_table_unch	"File '%s' is empty; input table is unchanged"
E_UF000C_cant_open_f_tbl_unchg	"Unable to open file '%0c' for input (rc=%1d); input table is unchanged"
E_UF000D_cant_read_temp_file	"Unable to read temporary file '%0c'"
S_UF000E_Enter_filenam_to_read  "Enter name of file to read: "
E_UF000F_File_does_not_exist    "File '%0c' does not exist"
E_UF0010_cant_open_file_input   "Unable to open file '%0c' for input (rc=%1d)"
E_UF0011_Unable_to_read_file    "Unable to read file '%0c'"
S_UF0012_Enter_filename_write   "Enter name of file to be written: "
E_UF0013_cant_open_file_output  "Unable to open file '%0c' for output (rc=%1d)"
E_UF0014_Unable_to_write_file   "Unable to write file '%0c'(rc=%1d)"
E_UF0015_Line_too_long   	"The joined line would be too long"
 
/*
** Output Browser
**      These next few fast strings are loaded into a table of
**      menu items and their explanations. These strings are the
**      explanations for the menu items in the order of:
**              top, bottom, print, file, help, end.
**
**	And, submenu items of:
**	    file menu  :   OK , FilePartial
**	    print menu :   OK , PrintPartial
*/
F_UF0001_Go_to_top_of_output    "Go to top of output"
F_UF0002_Go_to_bottom_of_outpu  "Go to bottom of output"
F_UF0003_Print_this_output      "Print this output"
F_UF0004_Save_this_output       "Save this output"
F_UF0005_Help_on_the_form       "Help on the form"
F_UF0006_End_this_display       "End this display"
F_UF0007_Output 		 "Output"
F_UF007B_ExplFileOK	     "Complete, then save results to file specified."
F_UF007C_ExplFilePartial     "Save the results seen thus far to file specified."
F_UF007D_ExplPrintOK	     
"Complete, write results to temporary file, then send file to printer specified."
F_UF007E_ExplPrintPartial    "Write results seen thus far to temporary file, send file to printer specified."
 

E_UF0017_FileOpenRead	"Could not open file '%0c.'
\nThe file could not be opened for reading.  Please check that you have access
rights to the file before reporting this as an internal problem to your Site
Technical Contact.  (status=%1d)."

/*%
** Explanation:
**	The program failed to read from the browser file.  Since it was able
**	to open it, it is possible the file was deleted from underneath it.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your site technical contact.
*/

S_UF0018_RunToCompletion	"Run the request to completion"

/*%
** Explanation:
**	Status message.
**
** System Status:
**	Program is running the complete query.
**
** User Action:
**	None.
*/

E_UF0019_CloseFile	"Unable to close file '%0c' (rc=%1d)"

/*%
** Explanation:
**	The program failed to close the browser file.  Since it was able
**	to open it, it is possible the file was deleted from underneath it.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your site technical contact.
*/

E_UF001A_EOFonFile	"Unexpected EOF on file '%0c' (code=%1d)"

/*%
** Explanation:
**	The program was trying to read another record from the browser file,
**	but it unexpectedly reached the end of the file.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your site technical contact.
*/

E_UF001B_FileOpenAppend	"Could not open file '%0c'.
\nThe file could not be opened for appending.  Please check that you have access
rights and sufficient disk space available for the file before reporting this as
an internal problem to your Site Technical Contact.  (status=%1d)."

/*%
** Explanation:
**	The program failed to open the browser file to append more data to it.
**	Since it was able to open it, it is possible the file was deleted from 
**	underneath it.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your site technical contact.
*/

S_UF001C_Terminating_msg	"Program Terminating - %s\n\n"

/*%
** Explanation:
**	A fatal error occurred.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your INGRES representative.
*/

S_UF001D_Enter_file_name	"Enter file name: "

/*%
** Explanation:
**	The program is prompting for a filename where write the output. 
**
** System Status:
**	The program is waiting for the user response.
**
** User Action:
**	Type in the name of the desired filename and press RETURN.
*/

E_UF001E_FileOpen	"Could not open file '%0c.'
\nThe file could not be opened for writing.  Please check that you have access
rights and sufficient disk space available for the file before reporting this as
an internal problem to your Site Technical Contact.  (status=%1d)."

/*%
** Explanation:
**	The program failed to open the file for writing.  It is possible the
**	user does not have write access for that directory.
**
** System Status:
**	The program continues.
**
** User Action:
**	Check for write access.
*/

S_UF001F_WritingScreen		"Writing screen to file '%0c' . . ."

S_UF0020_AppendingScreen	"Appending screen to file '%0c' . . ."

E_UF0021_PrintError		"Error attempting to print screen."

/*%
** Explanation:
**	Program failed sending file to the printer.
**
** System Status:
**	Program continues.
**
** User Action:
**	Check the status of the printer/printer queue.
*/

S_UF0022_PrintingOutput	"Printing output . . ."

S_UF0023_OverwritePrompt	"File already exists.  Do you want to overwrite
file? "

/*%
** Explanation:
**	The user entered a filename that already exists.  The program is
**	asking for verification to overwrite the file.
**
** System Status:
**	Waiting fo a user response.
**
** User Action:
**	Answer the question, 'Y' or 'N'.
*/

E_UF0024_TempFile	"Could not open temporary file.\n
Unable to open temporary browse file.  Please check that you have access rights
and sufficient disk space available in the temporary directory before reporting
this as an internal problem to your Site Technical Contact.  (status=%0d)"

/*%
** Explanation:
**	The program fails trying to open the browser file.  It is possible
**	the user does not have write access in the directory or the directory
**	does not exist.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Check the above situations. 
*/

E_UF0025_No_such_objects
"No %0c are in the database.\n
You have requested a list of %0c available to you in the database.
There are none at this time."
/*%
** Explanation:
**	User requested a selection list of a type of front end objects.
**	The user has access to none of that type of object.
**
** System Status:
**	Return without displaying the (non-existent) list.
**
** User Action:
**	Do something else.
*/

E_UF0026_BadForm		"Cannot access form \"%0c\".\n
The form could not be loaded from the form index file for the installation.\ 
Please check that your installation is correct."

/*%
** Explanation:
**	The named form could not be loaded from the form index file for the
**	current language (specified through II_LANGUAGE) in this installation.
**
** System Status:
**	Exit program.
**
** User Action:
**	Check that the II_LANGUAGE is specified correctly and that the
**	form index file in the language directory for this installation
**	is correct.
*/

E_UF0027_BadEXSet	"Could not set exception handler" /* DIAG */
E_UF0028_BadListParm    "Bad dynamic list size (%d rows, %d columns)" /* DIAG */
/*%
** Explanation:
**	Diagnostic error messages used in listpick code.
**
** System Status:
**	Terminate listpick.
**
** User Action:
**	None.
*/


S_UF0029_Destroy	"\nReally destroy %0c%1c?\n"
S_UF002A_Ydest		"Destroy %0c"
S_UF002B_Ndest		"Do not destroy %0c"
S_UF002C_Save		"\nSave changes to %0c%1c?\n"
S_UF002D_Ysave		"Save changes, then %0c"
S_UF002E_Nsave		"%0c without saving changes"
/*%
**  Description:
**	Strings used in standard prompt utility.
**
**  System Status:
**	None.
**
**  User action:
**	None.
*/

E_UF002F_BadType	"Bad type passed to IIUFccConfirmChoice()" /* DIAG */
E_UF0030_Alloc		"Memory allocation failure in prompt"
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

E_UF0031_Unexpected	"Unexpected failure of %0c in make_frame()" /* DIAG */
/*%
**  Description:
**	Internal errors are not supposed to happen.
**
**  System Status:
**	"Cancel" returned.
**
**  User action:
**	Call tech support.
*/

E_UF0032_Bad_Format_Params	"Internal program error.  Bad parameters when
generating field format.\n  Invalid arguments passed to 'IIFDffFormatField()'
for field '%0c'.  Please report this problem to your Site Technical Contact."

/*%
** Explanation:
**	An internal error has occurred.  An attempt to format a
**	dynamic field used invalid arguments.
**
** System Status:
**	Attempt to create a dynamic field has failed.
**
** User Action:
**	Report this.
*/

E_UF0033_Cannot_Wrap "Internal program error.  Cannot wrap non-character field.
\nAttempt to wrap a non-character field, '%0c' of datatype '%1d'. Please report
this problem to your Site Technical Contact."

/*%
** Explanation:
**	An internal error has occurred.  An attempt to wrap a
**	non-character field has been made
**
** System Status:
**	Attempt to create a dynamic field has failed.
**
** User Action:
**	Report this.
*/

E_UF0034_Cannot_get_wrap	"Internal program error:  Cannot get wrap value.
\nError from fmt_deffmt returned for field '%0c'.  Please report this problem to
your Site Technical Contact."

/*%
** Explanation:
**	An internal error has occurred.  An attempt to get the wrapped
**	default format for a dynamic field has failed.
**
** System Status:
**	Attempt to create a dynamic field has failed.
**
** User Action:
**	Report this.
*/

E_UF0035_Cannot_get_default	"Internal program error:  Cannot format field.
\nError from 'fmt_sdeffmt()' returned for field '%0c'.  Please report this
problem to your Site Technical Contact."

/*%
** Explanation:
**	An internal error has occurred.  An attempt to get the default
**	format for a dynamic field has failed.
**
** System Status:
**	Attempt to create a dynamic field has failed.
**
** User Action:
**	Report this.
*/

E_UF0036_Cannot_check_datatype	"Internal program error:  Cannot check datatype.
\nError from 'afe_cancoerce()' returned for field '%0c'.  Please reports this
problem to your Site Technical Contact."

/*%
** Explanation:
**	An internal error has occurred.  An attempt to check the
**	datatype for a dynamic field has failed.
**
** System Status:
**	Attempt to create a dynamic field has failed.
**
** User Action:
**	Report this.
*/

S_UF0037_Delete		"\nReally delete %0c%1c?\n"
S_UF0038_Ydel		"Delete %0c"
S_UF0039_Ndel		"Do not delete %0c"
S_UF003A_Remove		"\nReally remove %0c%1c?\n"
S_UF003B_Yrem		"Remove %0c"
S_UF003C_Nrem		"Do not remove %0c"

/*
** Messages in the ranges
**	E_UF003E -- E_UF0050
**	F_UF000B -- F_UF0059
** are all added because of moving the Help code from runtime to uf.
*/
S_UF003E_HELP_Field_Values	"HELP -- Field Values Frame"

S_UF003F_HELP_sub	"HELP -- %s"
S_UF0040_HELP_Command_Key	"HELP -- Command-Key Mapping for %s"
S_UF0041_HELP_Help_Submenu	"HELP -- Help Submenu"
E_UF0042_Unexpected_graphics	"Unexpected graphics flag (vfrfiq = %d)"

S_UF0044_Sorry_No_help_avail	"Sorry - No help available on \"%s\""
S_UF0045_Sorry_Cannot_edit	"Sorry - Cannot edit file \'%s\': Status %d"
S_UF0046_HELP_Main_Help		"HELP -- Main Help Frame"

S_UF0048_HELP_Command_Key	"HELP -- Command-Key Mapping"

F_UF0008_NoColChoices	"Column has no selection choices available."

S_UF0049_cnoval_null	"%3c\n\nColumn '%0c' is of data type Nullable `%1c',
display format `%2c', and has no validation check."

S_UF004A_cnoval	"%3c\n\nColumn '%0c' is of data type `%1c', display format `%2c',
and has no validation check."

S_UF004B_chasval_null	"%4c\n\nColumn '%0c' is of data type Nullable `%1c',
display format `%2c', and has validation check:\n`%3c'."

S_UF004C_chasval	"%4c\n\nColumn '%0c' is of data type `%1c', display format
`%2c', and has validation check:\n`%3c'"


F_UF0009_NoFldChoices	"Field has no selection choices available."

S_UF004D_fnoval_null	"%3c\n\nField '%0c' is of data type Nullable `%1c',
display format `%2c', and has no validation check."

S_UF004E_fnoval	"%3c\n\nField '%0c' is of data type `%1c', display format `%2c',
and has no validation check."

S_UF004F_fhasval_null	"%4c\n\nField '%0c' is of data type Nullable `%1c',
display format `%2c', and has validation check:\n`%3c'."

S_UF0050_fhasval	"%4c\n\nField '%0c' is of data type `%1c', display format
`%2c', and has validation check:\n`%3c'."

/*
**  Description (49-50):
**	Messages used for Help/Field.  %0c argument is the name of the
**	field or column. %1c is the data type string as it would appear
**	in the Data Type field of vifred's attribute screen, %2 is the
**	display format as seen in vifred, and %3 the actual validation
**	check text as entered in the vifred attribute screen.
**
*/

F_UF000B_Select	"Select"
F_UF000C_WhatToDo	"WhatToDo"
F_UF000D_Keys		"Keys"
F_UF000E_Menu	"Menu"
F_UF000F_Menu_Desc	"Go to or scroll through the menu line"
F_UF0010_Scup	"Scrollup"
F_UF0011_Scup_Desc	"Go to the next page of table or form"
F_UF0012_Scdn	"Scrolldown"
F_UF0013_Scdn_Desc	"Go to previous page of table or form"
F_UF0014_Nfld	"Nextfield"
F_UF0015_Nfld_Desc	"Go to the next field"
F_UF0016_Next	"Next"
F_UF0017_Next_Desc	"Go to the next graphical component"
F_UF0018_Pfld	"Previousfield"
F_UF0019_Pfld_Desc	"Go to the previous field"
F_UF001A_Prev	"Previous"
F_UF001B_Prev_Desc	"Go to the previous graphical component"
F_UF001C_Sclt	"Scrollleft"
F_UF001D_Sclt_Desc	"Scroll form to the left"
F_UF001E_Scrt	"Scrollright"
F_UF001F_Scrt_Desc	"Scroll form to the right"
F_UF0020_Ledg	"Leftedge"
F_UF0021_Ledg_Desc	"Go to left edge of field or form"
F_UF0022_Ledg	"Leftedge"
F_UF0023_Ledg_Desc	"Go to left edge of component or graph"
F_UF0024_Tedg	"Topedge"
F_UF0025_Tedg_Desc	"Go to top edge of component or graph"
F_UF0026_Redg	"Rightedge"
F_UF0027_Redg_Desc	"Go to right edge of field or form"
F_UF0028_Redg	"Rightedge"
F_UF0029_Redg_Desc	"Go to right edge of component or graph"
F_UF002A_Bedg	"Bottomedge"
F_UF002B_Bedg_Desc	"Go to lower edge of component or graph"
F_UF002C_Lchr	"Leftchar"
F_UF002D_Lchr_Desc	"Move left one space"
F_UF002E_Rchr	"Rightchar"
F_UF002F_Rchr_Desc	"Move right one space"
F_UF0030_Dnln	"Downline"
F_UF0031_Dnln_Desc	"Move down one line"
F_UF0032_Upln	"Upline"
F_UF0033_Upln_Desc	"Move up one line"
F_UF0034_Gcur	"Graphcursor"
F_UF0035_Gcur_Desc	"Activate graphics pointer on terminal"
F_UF0036_Lerr	"Lasterror"
F_UF0037_Lerr_Desc	"Display last error message"
F_UF0038_Nxwd	"Nextword"
F_UF0039_Nxwd_Desc	"Move forward one word"
F_UF003A_Pvwd	"Previousword"
F_UF003B_Pvwd_Desc	"Move backward one word"
F_UF003C_Mode	"Mode"
F_UF003D_Mode_Desc	"Switch edit mode - insert/overtype"
F_UF003E_Redr	"Redraw"
F_UF003F_Redr_Desc	"Redraw the screen "
F_UF0040_Dchr	"Deletechar"
F_UF0041_Dchr_Desc	"Delete character under cursor"
F_UF0042_Rubout	"Rubout"
F_UF0043_Rubout_Desc	"Delete character to left of cursor"
F_UF0044_Edit	"Editor"
F_UF0045_Edit_Desc	"Start system editor on field"
F_UF0046_Nrow	"Newrow"
F_UF0047_Nrow_Desc	"Move to first column on next row"
F_UF0048_Clear	"Clear"
F_UF0049_Clear_Desc	"Clear field or menu input"
F_UF004A_Clst	"Clearrest"
F_UF004B_Clst_Desc	"Clear field from cursor to end"
F_UF004C_Dup	"Duplicate"
F_UF004D_Dup_Desc	"Duplicate last value entered"
F_UF004E_Pscr	"Printscreen"
F_UF004F_Pscr_Desc	"Write current screen to file"
F_UF0050_Nitm	"Nextitem"
F_UF0051_Nitm_Desc	"Move to next item in form"
F_UF0052_Shell	"Shell"
F_UF0053_Shell_Desc	"Enter command line interpreter"
F_UF0054_disabled	" (Currently disabled)"
/*%
**  Description:
**	Line entries in the 'help keys' table.  (fehkeys.qsc)
**	The explanation for the shell key will include F_UF0054 if the
**	shell key is not enabled in the application.
**
**  System Status:
**	None.
**
**  User Action:
**	None.
*/

F_UF0055_SubTopics		"SubTopics"
F_UF0056_PrevTopic		"PrevTopic"
F_UF0057_ChooseTopic		"Choose a SubTopic"
F_UF0058_ChooseTopicHelp	"Choosing a SubTopic"
F_UF0059_SubTopicToken		"SubTopic"


E_UF0051_LoadForm	"Cannot initialize form.\n
The form `%0c' could not be loaded from the Form Index file.  This is an
installation problem.  Please report this problem to your Site Technical
Contact."

/*%
** Explanation:
**	A form used by the INGRES Front-End program could not be loaded from
**	a file containing all the forms for the INGRES installation.
**
** System Status:
**	The program may terminate or may not execute certain operations that
**	depend on the form.
**
** User Action:
**	Check that the environment logical II_LANGUAGE has been set correctly
**	before reporting this problem to your Site Technical Contact.
*/

/*
** Help on Data Type	("ufhlptyp.qc")
*/

F_UF0060_DT_Instr	"Data Type Selection.\n\nChoose one of the data types
listed below;\nyou will be prompted for a length if one\nis required."

F_UF0061_TypeTitle	"Data Type"
F_UF0062_LenTitle	"Length"
F_UF0063_PrecTitle	"Precision"
F_UF0064_ScaleTitle	"Scale"
F_UF0065_DT_Title	"Data Type Selection."

F_UF0070_char		"A fixed length character string."
F_UF0071_date		"An INGRES date or interval."
F_UF0072_decimal	"A packed decimal number."
F_UF0073_int		"A large integer."
F_UF0074_float		"A floating point number."
F_UF0075_money		"An INGRES money value."
F_UF0076_none		"No return value."
F_UF0077_logical	"A system-wide logical key."
F_UF0078_short		"A small integer, -32768 to 32767."
F_UF0079_tab_key	"A logical key for a table."
F_UF007A_varchar	"A variable length character string."
F_UF007F_seclabel	"A security label."
F_UF0080_ansidate	"An ANSI date."
F_UF0081_ansitmwo	"An ANSI time without time zone."
F_UF0082_ansitmw	"An ANSI time with time zone."
F_UF0083_ansitme	"An ANSI time with local time zone."
F_UF0084_ansitswo	"An ANSI timestamp without time zone."
F_UF0085_ansitsw	"An ANSI timestamp with time zone."
F_UF0086_ansitstmp	"An ANSI timestamp with local time zone."
F_UF0087_ansiinym	"An ANSI interval year to month."
F_UF0088_ansiinds	"An ANSI interval day to second."
   

E_UF0053_no_copy
"You did not specify the number of copies to be printed.\nSetting the default
number of copies to 1."
  
/*%
** Explanation:
**      The number of copies was not entered.  User is prompted to enter 
**	the number of copies of the output results to be printed.
**
** System Status:
**      Displays an error message and returns to prompt for number of copies.
**
** User Action:
**      Enter number of copies or choose to cancel this operation.
*/

S_UF0054_file_partial     
"FilePartial"

/*%
** Explanation:
**      Text for scrollable output browser popup.
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

S_UF0055_print_partial     
"PrintPartial"

/*%
** Explanation:
**      Text for scrollable output browser popup.
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

E_UF0056_no_filename
"A file name is required.  Please enter a valid file name."
 
/*%
** Explanation:
**      Prompting for the user to enter a file name to contain the query output.
**
** System Status:
**      Returns user to the file name prompt.
**
** User Action:
**      Enter a valid file name or cancel the current operation.
*/

E_UF0057_bad_filename
"An invalid file name has been encountered.\nPlease check your input for a 
valid file name."

/*%
** Explanation:
**      The user is prompted for a filename to send the query results.  An
**      invalid filename was encountered.
**
** System Status:
**      Displays an error message and returns to prompt for the error log name.
**
** User Action:
**      Enter a valid file name or choose to run the report interactively.
*/
 
  
 
E_UF0058_cannot_open_file
"Unable to write the file in the specified directory.\nPlease enter another
valid file name."
  
/*%
** Explanation:
**      The user entered a filename in a directory that the user does not have
**      write privileges.
**
** System Status:
**      Displays an error message and returns to prompt for another filename.
**
** User Action:
**      Enter a valid file name or choose to cancel this operation.
*/

 
E_UF0059_no_printer
"Please enter a printer name to print out your results.\nEnter 'default' to use
the system default printer."
  
/*%
** Explanation:
**      No printer name was entered.  User is prompted to enter a printer to
**	send the output results.
**
** System Status:
**      Displays an error message and returns to prompt for another printer.
**
** User Action:
**      Enter a valid printer name or choose to cancel this operation.
*/

E_UF005A_PrintFailed		
"An error occurred printing the output.\nThe error may be caused by an invalid 
printer name or a system error sending the output to the printer.  Check your
printer and all your parameters.\nYour file, '%0c' containing the output is not
deleted."


/*%
** Explanation:
**	Program failed sending file to the printer.
**
** System Status:
**	Program continues.  Temporary file containing the output is not
**	deleted.  
**
** User Action:
**	Check the status of the printer/printer queue.
*/

E_UF005B_NO_DEC_PREC
"Bad or no precision for the decimal datatype entered.\n  Either no
precision value was entered or the entered value is illegal.  Legal
values for the precision of a decimal datatype is between
1 and %0d."

/*%
** Explanation:
**	The user did not enter a precision value or the value is
**	illegal.
**
** System Status:
**	System will wait for user to enter a legal value.
**
** User Action:
**	User must entered a legal precision value.
*/

E_UF005C_NO_DEC_SCALE
"Bad or no scale for the decimal datatype entered.\n  Either no
scale value was entered or the entered value is illegal.  Legal
values for the scale of a decimal datatype is between 0 and %0d
and must be less than or equal to the precision value."

/*%
** Explanation:
**	The user did not enter a scale value or the value is
**	illegal.
**
** System Status:
**	System will wait for user to enter a legal value.
**
** User Action:
**	User must entered a legal scale value.
*/

E_UF0080_BrowseFileSeek	"Could not locate a record '%0d.'
\nThe indicated record could not be located in the browser file.
(status=%1d)."

/*%
** Explanation:
**	The program failed to locate a record in the browser file. Since it was able
**	to open it, it is possible the file was deleted from underneath it.
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your site technical contact.
*/

E_UF0081_BrowseFileRead	"Unable to read record '%0d' from the browser file'
\nUnexpected error whilst reading the browser file '%1c'.\n(status=%2d)."

/*%
** Explanation:
**	The program failed to read a record from the browser file. 
**
** System Status:
**	The program terminates.
**
** User Action:
**	Please contact your site technical contact.
*/

E_UF0082_BrowseFileTell	"Unable to obtain browser file current position'
\nUnexpected error whilst locating browser file '%0c'\ncurrent position. (status=%1d)."

/*%
** Explanation:
**	The program failed to find the browser files current position
**
** System Status:
**	The program terminates.
**
** User Action:
**	Please contact your site technical contact.
*/

E_UF0083_BrowseFileWrite "Unable to write record '%0d' from the browser file'
\nUnexpected error whilst writing to the browser file '%1c'.\nPlease check that you have
access rights and sufficient disk space available for the file before reporting this as
an internal problem to your Site Technical Contact.  (status=%2d)."

/*%
** Explanation:
**	The program failed to write a record from the browser file. This may be
**	because there is insufficient disk space available. 
**
** System Status:
**	The program terminates.
**
** User Action:
**	Try to execute the program.  If the error is duplicated, please contact
**	your site technical contact.
*/
