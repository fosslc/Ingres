/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ercf.msg -	CopyForm Message Definitions File.
**
** Description:
**	Defines the messages for the CopyForm program, class CF.
**
** History:
**	Revision 6.1  88/06/16  wong
**	Removed numbers 22 and 23, which were not language dependent.
**
**	Revision 6.0  87/07/31  rdesmond
**	Created.
**	12/11/87 (dkh) - Changed "Compiling" to "Encoding" in
**			 message S_CF001E_Compiling_forms_into_.
**	07/18/89 (dkh) - Added error message for IICFsave();
**	07/06/90 (dkh) - Added message E_CF0024_EMPTY to handle
**			 empty input file.
**	07/11/90 (dkh) - Added message E_CF0025_BADFILE to handle
**			 cases where passed file is not in copyform format.
**	04/08/91 (dkh) - Fixed bug 36573.
**	02/20/93 (dkh) - Added message E_CF0027_OPENFAILED.
**	07/25/07 (drivi01) - Updated E_CF0000_CANNOT_OPEN_FILE  message
**			     to provided clearer message on Vista.
**/

# define	_CF_CLASS	79
	
F_CF0000_OVERWRITE_PROMPT "REPLACE qbfnames, joindefs and forms which exist (y, n)[n]"

/*%
** Explanation:
**	This prompt is displayed when at least one qbfname, form or joindef in
**	the intermediate file has the same name as one in the destination
**	database.
**
** System Status:
**	If the user confirms, all existing objects in the database are 
**	overwritten, else the program terminates.
**
** User Action:
**	Confirm or deny.
*/

E_CF0000_CANNOT_OPEN_FILE "COPYFORM: Cannot open file '%0c'.\nVerify that you have sufficient privileges to write to this location."

/*%
** Explanation:
**	The intermediate file cannot be opened.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Contact system administrator.
*/

E_CF0001_CANNOT_WRITE_TO_FILE "COPYFORM: Cannot write to file '%0c'."

/*%
** Explanation:
**	For Copyform OUT, the intermediate file cannot be written to.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Contact system administrator.
*/

S_CF0002_WROTE_QBFNAME "Wrote copy of qbfname '%s' to file '%s'."
S_CF0003_WROTE_JOINDEF "Wrote copy of joindef '%s' to file '%s'."
S_CF0004_WROTE_FORM "Wrote copy of form '%s' to file '%s'."

E_CF0005_CANNOT_FIND_QBFNAME "COPYFORM: Cannot find qbfname '%0c'."

/*%
** Explanation:
**	For Copyform OUT, the given qbfname specified in the input list is 
**	not in the database.
**
** System Status:
**	Program continues.
**
** User Action:
**	Specify qbfnames which are in the database.
*/

E_CF0006_CANNOT_FIND_JOINDEF "COPYFORM: Cannot find joindef '%0c'."

/*%
** Explanation:
**	For Copyform OUT, the given joindef specified in the input list is 
**	not in the database.
**
** System Status:
**	Program continues.
**
** User Action:
**	Specify joindefs which are in the database.
*/

E_CF0007_CANNOT_FIND_FORM "COPYFORM: Cannot find form '%0c'."

/*%
** Explanation:
**	For Copyform OUT, the given form specified in the input list is 
**	not in the database.
**
** System Status:
**	Program continues.
**
** User Action:
**	Specify forms which are in the database.
*/

E_CF0008_CANNOT_CONVERT_FILE "COPYFORM: Cannot convert file '%0c' to 6.0 format."

/*%
** Explanation:
**	For Copyform IN, the intermediate file is neither in the 5.0 or 6.0
**	format.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Make sure intermediate file is either in 5.0 or 6.0 format.
*/

S_CF0009_COPIED_QBFNAME "Copied qbfname '%s' to database."
S_CF0010_COPIED_JOINDEF "Copied joindef '%s' to database."
S_CF0011_COPIED_FORM "Copied form '%s' to database."

E_CF0012_CANNOT_COPY_QBFNAME "Cannot copy qbfname '%0c' to database."

/*%
** Explanation:
**	For Copyform IN, the given qbfname cannot be copied to the database.
**
** System Status:
**	Program continues.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF0013_CANNOT_COPY_JOINDEF "Cannot copy joindef '%0c' to database."

/*%
** Explanation:
**	For Copyform IN, the given joindef cannot be copied to the database.
**
** System Status:
**	Program continues.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF0014_CANNOT_COPY_FORM "Cannot copy form '%0c' to database."

/*%
** Explanation:
**	For Copyform IN, the given form cannot be copied to the database.
**
** System Status:
**	Program continues.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF0015_BAD_FLAG "COPYFORM:  Bad flag '%0c' specified.	 Program stops."

/*%
** Explanation:
**	User specified an invalid flag on the command line.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Specify only valid flags.
*/

E_CF0016_ILLEGAL_QBFNAME "COPYFORM: Illegal qbfname '%0c'."

/*%
** Explanation:
**	User specified an invalid name for a qbfname to be copied.
**
** System Status:
**	Program continues.
**
** User Action:
**	Specify valid names for objects to copy.
*/

E_CF0017_ILLEGAL_FORMNAME "COPYFORM: Illegal form name '%0c'."

/*%
** Explanation:
**	User specified an invalid name for a form to be copied.
**
** System Status:
**	Program continues.
**
** User Action:
**	Specify valid names for objects to copy.
*/

E_CF0018_ILLEGAL_JDEFNAME "COPYFORM: Illegal joindef name '%0c'."

/*%
** Explanation:
**	User specified an invalid name for a joindef to be copied.
**
** System Status:
**	Program continues.
**
** User Action:
**	Specify valid names for objects to copy.
*/

/*
**	cfaddobj.qc
*/

/*
**	cfcompile.qc
*/

E_CF001A_ERROR__Unable_to_relo	"COPYFORM: Unable to relocate pointers for compiled form"

/*%
** Explanation:
**	Cannot relocate pointers during encoding of forms.
**
** System Status:
**	Program continues.  Form being worked on is not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF001B_ERROR__Unable_to_conv	"COPYFORM: Unable to convert filename to LOCATION"

/*%
** Explanation:
**	Cannot convert filename to LOCATION during encoding of forms.
**
** System Status:
**	Program continues.  Form being worked on is not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF001C_ERROR__Unable_to_open	"COPYFORM: Unable to open intermediate file"

/*%
** Explanation:
**	Cannot open intermediate file during encoding of forms.
**
** System Status:
**	Program continues.  Form being worked on is not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF001D_ERROR__Unable_to_re	"COPYFORM: Unable to read intermediate file"

/*%
** Explanation:
**	Cannot read intermediate file during encoding of forms.
**
** System Status:
**	Program continues.  Form being worked on is not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

S_CF001E_Compiling_forms_into_	"Encoding forms into database . . ."

E_CF001F_Could_not_open_temp_f	"COPYFORM: Could not open temp file for compiling forms."

/*%
** Explanation:
**	Cannot open intermediate file during encoding of forms.
**
** System Status:
**	Program continues.  Forms are not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF0020_Could_not_find_form__	"COPYFORM: Could not find form \"%0c\" to compile."

/*%
** Explanation:
**	Could not find given form during encoding of forms.
**
** System Status:
**	Program continues.  Specified Form is not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

E_CF0021_Errors_while_accessin	"COPYFORM: Errors while accessing form \"%0c\" for compilation."

/*%
** Explanation:
**	Non-specific error during encoding of given form.
**
** System Status:
**	Program continues.  Specified Form is not encoded.
**
** User Action:
**	Report to INGRES technical support.
*/

/*
**	copyform.qc
*/
F_CF0001_COPYFORM	"COPYFORM"

F_CF0002_FIELDS		"II_FIELDS"

F_CF0003_TRIM		"II_TRIM"

F_CF0004_FORMS		"II_FORMS"

F_CF0005_ENCODED	"II_ENCODED_FORMS"

E_CF0022_SAVE_ERROR
"Error saving form into database.\n
An error occurred while saving form `%0c' into database
catalog `%1c'."

E_CF0023_OBJ_SAVE
"Error creating form object in database.\n
An error occurred while trying to register form `%0c' into the database."

E_CF0024_EMPTY
"Input file `%0c' appears to be empty.  Please check that you have
specified the correct filename."

E_CF0025_BADFILE
"Input file `%0c' does not appear to be in COPYFORM format.  Please
check that you have specified the correct filename."

E_CF0026_CORRUPTED_FILE
"Copyform could not copy your form into the database since the file
passed to copyform appears to have bad data in it.  The bad data
appears to start in the `%0c' section of the file.\n  Copyform
is exiting."

E_CF0027_OPENFAILED
"Could not open file `%0c' or it does not exists.  Please check the
spelling of the file name and file permissions."

/*%
** Explanation:
**	The specified could not be opened or that is does not exist.
**	Probably due to spelling error or permissions problems.
**
** System Status:
**	The clients of the routine that caused this error will most
**	likely exit.  The routine that references this error is an
**	internal use only routine.
**
** User Action:
**	Check spelling and permission of the file and try again.
*/
