/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/


/**
** Name:	erca.msg -	Copy Application Utility Message Definitions.
**
** Description:
**	Defines messages for the copy application utility, COPYAPP.
**
** History:
**	Revision 6.0  10-aug-1987 (Joe)
**	Initial Version.
**	02-nov-89 (marian)
**		Replace "Technical Support Representative" with "Site
**		Technical Contact".
**	23-oct-1990 (mgw)
**		Corrected some misspellings. All are in comments, not messages,
**		but these comments get picked up for the messages.txt file via
**		erxtract and are thus in a deliverable.
**	14-nov-1990 (blaise)
**		Change text of E_CA000C_BADFLAG, removing usage message; add
**		a new message S_CA0026_Correct_syntax to be displayed after
**		E_CA000C_BADFLAG or E_CA000A_DIRECTION. (bug 34312).
**	26-jul-93 (blaise)
**		Added F_CA0005_Application, to replace literal string in
**		caout.sc.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**              Ingres message file cleanup.
**/

# define	_CA_CLASS	44

/*
**	caconvert.c
*/
/*
**	caconvert.c
*/
/*
**	caerror.h
*/
/*
**	caerror.qc
*/
/*
**	COPYAPPl.h
*/
/*
**	COPYAPPl.qc
*/
/*
**	Prompt for direction of copy.
*/
S_CA0001_Direction_of_copy_	"Direction of copy: "
/*
**	Response for S_CA0001.  This is the out direction from db
**	to file.
*/
F_CA0001_out	"out"
/*
**	Response to prompt S_CA0001.  This is the in direction, from 
**	a file to the database.
*/
F_CA0002_in	"in"
/*
**	Prompting for a missing database name.
*/
S_CA0002_Database_name_	"Database name: "
/*
**	Prompting for the name of a file containing the
**	objects to copy in.
*/
S_CA0003_Name_of_intermediate_	"Name of intermediate file: "
/*
**	Prompt to get the name of the application.
*/
S_CA0004_Application_name_	"Application name: "
/*
**	The class of the object is unknown.
*/
F_CA0003_UNKNOWN	"UNKNOWN"
F_CA0004_APPLICATION	"APPLICATION"
/*
**	caout.sc
*/
F_CA0005_Application	"Application"
/*
**	main.c
*/
E_CA0005_EXCEPTION	"COPYAPP: could not set exception handler.\n
An internal error in COPYAPP occurred.  COPYAPP could not set an exception
handler that it uses.  Retry running COPYAPP and if the problem persists
notify your Site Technical Contact."

/*%
** Explanation:
**	An internal error in COPYAPP occurred.  This problem should
**	not usually happen.  Retry running COPYAPP to see if the
**	problem will go alway.
**
** System Status:
**	COPYAPP will terminate with a fatal status.
**
** User Action:
**	Try running COPYAPP again.  If the problem persists, call
**	your Site Technical Contact.
*/
/*
**	insrcs.qc
*/
S_CA0006_Copying_source_files_	"Copying source files\r\n"

E_CA0007_DUPOBJNAME "A %1c named `%0c' already exists in the database.\n"
/*%
** Explanation:
**     	COPYAPP found that an object with the same name as
**    	an object being copied in already exists in the database.
**	This is an information message.
** System Status:
**	The action that COPYAPP takes depends on the setting
**	of various options.  It will either replace the object,
**	leave the object alone or quit.
** User Action:
**	If the action COPYAPP takes is not what you intended, the
**	flags -r or -q can be used to change the action COPYAPP takes.
*/
/*
** The first argument is the name of the object.  The second argument
** is the type of object it is (e.g. application, form...)
*/

E_CA0008_REPLACE "The %1c `%0c' in the database will be replaced.\n"
/*%
** Explanation:
**	COPYAPP found that an object with the same name as
**	an object being copied in already exists in the database.
**	COPYAPP is going to replace the object in the database
**	with the one in the file.
** System Status:
**	COPYAPP will carry out the replace.
** User Action:
**	If you do not want COPYAPP to do a replace then the
**	-r flag should not be used.
*/
/*
** The first argument is the name of the object that is being replace.
** The second argument is the type of object (e.g. application, form...)
*/

E_CA0009_NOCHANGE "The %1c `%0c' in the database will NOT be changed."
/*%
** Explanation:
**	COPYAPP found that an object with the same name as
**	an object being copied in already exists in the database.
**	COPYAPP is not going to change the object in the database.
** User Action:
**	If you want COPYAPP to do a replace then the -r flag should be used.
*/
/*
** The first argument is the name of the object that is being replace.
** The second argument is the type of object (e.g. application, form...)
*/

E_CA000A_DIRECTION "Direction of copying must be specified."
/*%
** Explanation:
**	COPYAPP must be given a direction to do the copy.
** System Status:
**	COPYAPP will terminate with a fatal status.
** User Action:
**	Run COPYAPP again specifying a direction.
*/

E_CA000B_FLAGARG   "No argument provided for the flag `%0c'.\n
The flag `%0c' expects an argument to follow immediately afterward.
Please specify the argument."
/*
** The argument is a command line flag like "-t"
*/

E_CA000C_BADFLAG  	"`%0c': Bad argument.\n"
/*%
** Explanation:
**	The user entered an invalid command line argument.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Run COPYAPP with a valid command line argument.
*/

E_CA000D_FILEOPEN	"Unable to open the file `%0c'.\n
COPYAPP could not open the file `%0c'.  Make sure the file
exists and you have permission to read it."
/*
** The argument is the name of the file.
*/

E_CA0028_FILECRE	"Unable to create the file `%0c'.\n
COPYAPP could not create the file `%0c'.  Make sure that the file name is valid
and that you have permission to create the file."
/*
** The argument is the name of the file.
*/

E_CA000E_DIREXIST "Directory `%0c' does not exist and/or cannot be created.\n"
/*
** The argument is the directory name.
*/

E_CA000F_EXITMSG	"Processing stops.\n
No changes will be made to the database."
/*%
** Explanation:
**	COPYAPP got a fatal error. The exact error that occurred will
**	have already generated an error message.
** System Status:
**	COPYAPP will back out any changes that have been made and
**	terminate.
** User Action:
**	Correct the previous error.
*/

E_CA0010_TOOLONGNAME 	"The name `%0c is too long for an application.\n
It has been truncated to 9 characters."
/*
** ???
*/

E_CA0011_CANOSRCDIR	" No source file directory was specified.\n
The application in the intermediate file does not have a source file
directory. Therefore, no source files can be copied."
/*%
** Explanation:
**	The application being copied in does not have a source file
**	directory.  No source files can be copied.
** System Status:
**	COPYAPP will not copy the source files and will terminate.
** User Action:
**	Use ABF to add a source file directory to the application.
*/

E_CA0012_FLAGCNFLT	"You cannot specify both the -r and -q flags.\n"
/*%
** Explanation:
**	The -r and -q flags are incompatible since they request
**	conflicting actions.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Only specify one of -r or -q.
*/

E_CA0013_NOSUCHAPP  "The application `%0c' does not exist in the database\n."
/*%
** Explanation:
**	The application was not found in the database.
** System Status;
**	COPYAPP will terminate.
** User Action:
**	Run COPYAPP with a valid database name.
*/

E_CA0014_GETOBJS	"Cannot get objects for application `%0c'.\n
The objects for the application `%0c' could not be retrieved.  The
module that retrieves them returned a value of `%1x'."
/*%
** Explanation:
**	There was an error retrieving the objects for an application.
**	This is normally an internal problem in COPYAPP, but could
**	be because of problems in the database.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Make sure the application name is correct.  Try running
**	ABF on the application.  Finally, report the error to
**	your Site Technical Contact.  Make sure you
**	report value given in the error.
*/

E_CA0015_NOOUTFUNC	"INTERNAL ERROR: No output function for object\n.
The object `%0c' of class `%1d' does not have an output function defined
for it.  Processing will continue, but this object will not be written
to the file. This is an internal problem in COPYAPP and should be reported
to your Site Technical Contact."
/*%
** Explanation:
**	There is an internal error in COPYAPP so that it can not
**	copy out an object.
** System Status:
**	COPYAPP will simply ignore this object and continue to
**	write the output file.
** User Action:
**	Report this to your Site Technical Contact.
*/

S_CA0016_COPYOUT	"Copying %1c `%0c' to the file."
/*
** The first argument is the name of the object, and the second
** is the type of object being copied.
*/

E_CA0017_NO_APPL_REC	"Cannot get application record.\n
While trying to copy the source files, COPYAPP could not retrieve
the application record for the application.  This usually means the
ABF catalogs have been corrupted.  Try running COPYAPP again."
/*%
** Explanation:
**	After copying in the application, COPYAPP tried to retrieve
**	the application's record from the ABF catalogs.  It could
**	not retrieve the record.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Try running COPYAPP again.  If it still fails,
**	report this to your Site Technical Contact.
*/

E_CA0018_ErrLongRemark	"Cannot read long remarks for object.\n
The long remarks for the object `%1c,' which is part of the application,
'%0c,' could not be read from the catalogs.  Please report this problem
to your Site Technical Contact."

S_CA0019_NoCopy	"The source directory specified in the intermediate
file is identical to the target directory specified on the command line.  No
copy needs to take place."

E_CA0020_NoForm	"Could not copy form `%0c' to the file"

E_CA0021_4GLExists	"File `%0c' already exists.
The 4GL source for frame `%1c', which should be automatically generated,
already exists.  This source file may or may not be appropriate for the
frame.  The frame will now be considered CUSTOM"

E_CA0022_GSCNFLT	"You cannot specify both the -a and -s flags.\n"
/*%
** Explanation:
**	The -a and -s flags are incompatible since they request
**	conflicting actions.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Only specify one of -a or -s.
*/

E_CA0023_BadChar	"Unexpected characters, '%0c', in source file section."
/*%
** Explanation:
**	Bad syntax has been discovered in the temp file when copying source
**	files from it.
** System Status:
**	Copyapp will ignore the line.
** User action:
**	Make sure the temp file did not get accidentally modified prior
**	to the copyapp in.
*/

E_CA0024_EOF	"Unexpected end in source file section."
/*%
** Explanation:
**	An EOF has been reached, or read failure has occurred in reading
**	the source files from the temporary file before an expected
**	terminator line.
** System Status:
**	Copyapp will stop copying source files.
** User action:
**	Make sure the temp file did not get accidentally truncated.
*/

S_CA0025_No_a_or_s	"Warning: Neither -a nor -s specified.\n
A Vision application has been copied without use of either the -a
or -s flags.  This may create an undesired situation if 4GL source files for
automatically generated frames exist, or are copied into the application source
directory.  If such files are more recent than they were at time of
the \"copyapp out\" operation, they will become \"custom\" the first time
they are compiled, as it will appear as if they have been edited."
/*%
** Explanation:
**	There are metaframe types in this application, and neither of the
**	two options which allow the generation dates to be handled have
**	been provided.  Generation dates will be copied as they were in
**	the source database.
** System Status:
**	This is a warning message.  Copyapp actually worked, except
**	that the generation dates are questionable.
** User action:
**	If they wish to avoid the problem, they may redo the copyapp
**	specifying either -a or -s.
*/

S_CA0026_Correct_syntax		"Correct Syntax:\n\
copyapp out [-ddir_name] [-llistfile] [-tfilename] [-uusername] dbname\n\
	applname\n\
copyapp in [-adirname] [-c] [-ddirname] [-llistfile] [-nnewapplname]\n\
	[-p] [-q] [-r] [-sdirname] [-uusername] newdbname intfilename"
/*%
** Explanation:
**	Display the correct syntax of copyapp, following E_CA000C_BADFLAG
**	 or E_CA000A_DIRECTION.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Run COPYAPP with a valid command line argument.
*/

S_CA0027_IllegalName
"'%0c' is an illegal name for an application.\n\
The '-n' flag on the command line specified an illegal name for an application.
Please re-enter the 'copyapp' command specifying a legal name."
/*%
** Explanation:
**	The -n flag on the command line specified an illegal name.
** System Status:
**	COPYAPP will terminate.
** User Action:
**	Run COPYAPP specifying a valid application name.
*/
