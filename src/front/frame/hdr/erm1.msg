/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erm1.msg -	Form Index File Module Messages.
**
** Description:
**	Contains the message definitions for the Form Index file module.
**
** History:
**	87/08/14  peter
**		Initial revision.
**	88/04/01  wong
**		Revised for Form Index file as a class.
**	14-feb-90 (bruceb)
**		Changed ellipses as per new style guideline.
**/

# define	_M1_CLASS	179

/*
** FormIndex Command Messages.
*/

E_M10000_NoDB	"Error connecting to database `%0c'.\n"

/*%
** Explanation:
**	The FormIndex command could not successfully connect to the named
**	database most likely because the database does not exist, or possibly
**	because the commands specified using the ING_SET environment logical
**	are in error.
**
**	Occasionally, successful database connection may be precluded by
**	other errors (e.g., GCA errors, etc.) usually evidence of some sort
**	of installation problem or hardware or system failure.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Check that the database exists and create it if it does not.  If the
**	database name was specified on the command line, check that it was
**	specified correctly and re-enter the command line.  Also, check that
**	the commands specified using the ING_SET logical are correct.
*/

F_M10001_Usage	"index_file_name [-uuser] [-@database] [[-m] formname[.frm]
	[-d form_name] [-a formname[.frm] ] [-l]"

/*%
** Explanation:
**	The FormIndex command must have the Form Index file name and at least
**	one editing command specified on the command line.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Specify the required arguments.
*/

/*
** Form Index File Open Messages.
*/
S_M10010_Version	"   INGRES FormIndex Version %s"

S_M10011_Creating	"Creating form index file '%s' . . ."

S_M10012_Opening	"Opening index form file '%s' . . ."

E_M10013_CantCreate	"Form Index file creation error.\n
The Form Index file module could not create the Form Index file '%0c.'  Please
check that you have access rights to the directory where the file is to be
created.  Also, check that you have sufficient disk space to create files in
this directory."

/*%
** Explanation:
**	The Form Index file module could not create the file that will contain
**	a Form Index.  This could be for any number of reasons, but most likely
**	is because you do not have access rights to the directory where the file
**	is to be created or because you have exceeded the disk space available
**	to you in this directory.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Make sure that you have access rights to the directory where the Form
**	Index file is to be created and also that sufficient disk space is
**	available to you in that directory.  Also, check that the you correctly
**	specified the name of the Form Index file.
*/

E_M10014_CantOpen	"Form Index file open error.\n
Cannot open the file '%0c.'  Please check that this file exists and that you
have access rights to this file."

/*%
** Explanation:
**	The named Form Index file cannot be opened.  This could be because the
**	file does not exist in the installation for the language specified by
**	the II_LANGUAGE environment logical, or because the file does not have
**	the proper read access rights.  Another possibility is that the name
**	specified for the Form Index file using the II_FORMFILE environment
**	logical is incorrect.
**
**	Also possible is that the file could not be opened for writing because
**	another program has already opened it for either reading or writing,
**	which might occur on some operating systems when running the FormIndex
**	command.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Check with your system administrator that the language specified by the
**	environment logical II_LANGUAGE is correct and that the Form Index file
**	exists in the II_CONFIG installation for this language and has the
**	correct access rights.  Also, check that any file name specified using
**	the environment logical II_FORMFILE is correct.
*/

E_M10015_BadVersion	"Incompatible Form Index file version.\n
The version of the Form Index file '%1c', '%0c', is incompatible with the
current version supported by this installation, '%2c'."

/*%
** Explanation:
**	The version of the named Form Index file is not compatible with
**	the version that the current installation supports.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Check with your system administrator to verify that the installation is
**	correct, and that the Form Index file is compatible with this version.
*/

E_M10016_BadIndex	"Error reading index for Form Index file.\n
An error occurred while trying to read the index for the Form Index file,
'%0c'."

/*%
** Explanation:
**	The index table in the Form Index file could not be read, which might
**	be because the installation is incorrect.  Otherwise, this is because
**	of an internal programming error.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Report this problem to your technical support representative.
*/

/*
** General Form Index File Method Messages.
*/

S_M1001A_IndexInfo	"Form No.%3d\t%-32s\n"		/* DIAG */

E_M1001B_BadName	"Form name '%0c' is not a legal form name.\n
The form name '%0c' is not a legal form name, which must be a legal
Ingres name."

/*%
** Explanation:
**	The name of the form specified on the FormIndex command line is
**	not a legal name for a form.  Names specified for forms on the
**	command line are either legal Ingres form names or are a file name
**	with the extension ".frm" (which contains a CopyForm format of the
**	form.)
**
** System Status:
**	No action (replace, append, delete) will be performed on the form
**	in the Form Index file.
**
** User Action:
**	Make sure that the form names listed on the command line are correct.
*/

S_M1001C_Deleting	"Deleting form '%s' . . ."

E_M1001D_NoForm	"Non-existent form in Form Index.\n
The form '%0c' does not exist in the Form Index file, and hence, cannot be
deleted.  Check that the form name was specified correctly and retry the
delete operation."

/*%
** Explanation:
**	The index in the Form Index file does not list the named form.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Use correct form name.
*/

E_M1001E_AlreadyExists	"Form of name '%0c' already exists.\n
A form of the name '%0c' already exists in the form index file.
If you wish to replace it, use the replace (-m) flag."

/*%
** System Status:
**	This form will not be added to the file.
**
** User Action:
**	Use -m flag to replace a form.
*/

S_M1001F_Replacing	"Replacing form '%s' . . ."

/*
** Form Index File Insert Messages.
*/

S_M10020_Appending	"Appending form '%s' . . ."

E_M10021_IndexFull	"Form Index file index table full.\n
The number of forms inserted into this Form Index file cannot exceed '%0d'.  The
form '%1c' will not be inserted into the Form Index file."

/*%
** Explanation:
**	The internal number of indexes available in a Form Index file is set
**	at the time the file was created.  This number has now been exceeded.
**	Please report this problem to your RT Engineering contact so that this
**	number can be increased.
**
** System Status:
**	Program terminates.
**
** User Action:
**	The number of indexes available is a hard limit large enough for all
**	forms used by RT products.  Too many files have been placed in the
**	Form Index file:  Use fewer forms.
*/

E_M10022_BadRetrieve	"Encoded form fetch error.\n
An error occurred when fetching the form from the \"ii_encoded_forms\" table in
the database.  This could be caused by a permission error, or a deadlock, or any
other database error.  The form will not be replaced or appended in the Form
Index file."

/*%
** Explanation:
**	A DBMS error occurred when this form was fetched from the database.
**
** System Status:
**	Program terminates without replacing or appending the form in the
**	Form Index file.
**
** User Action:
**	Try again if a deadlock error occurred.  Otherwise, talk to
**	your system administrator.
*/

E_M10023_NoEncodedForm	"Missing encoded form.\n
No encoded form was found in the \"ii_encoded_forms\" system table for the
form '%0c.'  Please check that the form exists and is accessible to you in the
database.  (Forms are accessible if owned by you or the DBA of the database.)"

/*%
** Explanation:
**	No rows for the encoded form were found in the system table.
**
** System Status:
**	Program Exits.
**
** User Action:
**	Use correct form name, or check form ownership.
*/

E_M10024_BadEncodedForm	"Incomplete encoded form.\n
The encoded form found in the \"ii_encoded_forms\" system table for the form
'%0c' is incomplete.  Please report this problem to your technical
representative."

/*%
** Explanation:
**	Not all the rows for the encoded form could be fetched from the
**	database.  This is because Vifred or CopyForm failed to write all
**	the rows, or because the DBMS failed to write or fetch all the
**	rows.  This is an internal programming problem.
**
** System Status:
**	Program Exits.
**
** User Action:
**	Report this problem to your technical representative.
*/

/*
** Form Index File Read Messages.
*/

E_M10030_BadForm	"Error reading form from Form Index file.\n
An error occurred while trying to read the encoded form '%0c' from
the Form Index file, '%1c'."

/*%
** Explanation:
**	A read error occurred when an encoded form was being read in from the
**	Form Index file.  Most likely this is because of a system error or
**	disk error.  Less likely, this is because of some internal programming
**	error that occurred when the encoded form was inserted into the Form
**	Index file.
**
** System Status:
**	Since the form could not be read into the program, it will likely
**	terminate.
**
** User Action:
**	Report this problem to your technical representative.
*/
