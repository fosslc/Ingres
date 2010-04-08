/*
**	Copyright (c) 1989 Ingres Corporation
*/

/**
** Name:    erse.msg -	OpenROAD executables Message file
**
** Description:
**	Contains the errors for the sapphire executables.
**
** History:
**	7-nov-1989 (Joe)
**	    First Written
**	21-jun-1990 (Joe)
**	    Changed value of E_SE0008.
**	11/20/91 (dkh) - ER changes for internationalization.
**	13-mar-1994 (emerson)
**	    Added S_SE0036_BAD_ARRAY.
**	5-aug-94 (mevans)
**		Added S_SE0037_IMAGE_FILE_BUILT
**	17-jul-98 (dansa02)
**		Added E_SE0038_NO_RUN_LICENSE and E_SE0039_NO_DEV_LICENSE.
**	21-aug-98 (dansa02) Bugs 90780, 90783
**		Added E_SE003A_APP_ALREADY_EXISTS, E_SE003B_CANT_FETCH_APP_VER,
**		E_SE003C_CANT_FETCH_COMP_VER and E_SE003D_BADAPPVERFROMDB.  
**		NOTE that only E_SE003D has the true ER parameter format (with
**		the parmeter number between the % and the format specifier).
**		That's because many of the messages from this file are output
**		using the sesblk.dump formatting method instead of real ER 
**		formatting.  Yuk!
**	22-apr-2003 (crido01) SIR 110063
**	    Add messages pertaining to new makeimage options, also replace
**	    Windows4GL references with OpenROAD and change 'can not' to
**	    cannot. 
**	02-oct-2003 (crido01) SIR 110155
**	    Add messages pertaining to new backupapp options.
**/

#define	_SE_CLASS	199

E_SE0000_TIMEDOUT	"This field test software is no longer authorized.\n
Please contact Ingres Corporation for renewal."

/*%
** Explanation:
**	The program being invoked is field test software that has reached
**	its expiration date.  It is no longer runnable.
**
** System Status:
**	The program will exit and will not be runnable until the software
**	is renewed.
**
** User Action:
**	Contact your Ingres Corporation representative for renewal.
*/

E_SE0001_NOSTARTINGOBJ	"No starting object was specified.\n
To run an application the name of an object (either a frame or a procedure)
must be given.  In this run of the application no starting object was given
on the command line, and no default starting object was specified for
the application.  Either specify a default starting object for the application
or specify a starting object on the command line using the -c flag."

/*%
** Explanation:
**	For an application to run it must be given the name of a frame
**	or procedure to start running.  In this case, there was no
**	default starting object specified for the application and the
**	user did not specify a starting object on the command line.
**
** System Status:
**	The program will exit.
**
** User Action:
**	Specify a starting object using the -c flag.
*/

E_SE0002_CANTSTART	"The application '%0c' could not be started.\n
An error occurred while trying to start the application '%0c' on the
object '%1c'"

/*%
** Explanation:
**	The application could not be started and thus cannot be
**	run.  A previous error should explain what is wrong.
**
** System Status:
**	The program will exit.
**
** User Action:
**	The more specific errors should give the information needed to
**	get this running.
*/

E_SE0003_BADIMAGEFILE	"The image file '%0c' does not exist or is unreadable.\n
Make sure the image file given on the command line is a valid file name, 
that the file exists, and that the file is accessible to you."

/*%
** Explanation:
**	The image file given by the user could not be opened.
**
** System Status:
**	The program will exit.
**
** User Action:
**	Correct the command line so that a valid image file name is given.
*/

E_SE0004_NOAPPIMAGE	"The application could not be extracted.\n
The application contained in the image file '%0c' could not be extracted from
the image file.  Make sure the file given on the command line is a valid
image file name and that the file is accessible to you."

/*%
** Explanation:
**	The image file given by the user exists, but an application
**	could not be gotten from it.
**	Other errors may give more information on what is wrong.
**
** System Status:
**	The program will exit.
**
** User Action:
**	Provide a valid image file name, or follow the directions of
**	the other errors.
*/

E_SE0005_IMGNEWER	"Version Mismatch: The image file is too new.\n
The image file '%0c' was created under a newer version of OpenROAD than
this program.  This program cannot run this image file.  Check your
installation and make sure that makeimage, rundbapp, runimage have
all been installed at the same time.  The image's version number
is (%1d, %2d) while the runtime system's version number is (%3d, %4d)."

/*%
** Explanation:
**	OpenROAD image files are given a version number when created.
**	This version number consists of two parts, a major version and
**	a minor version.  For an image file to be compatible with
**	a OpenROAD executable, the major version number of the image
**	file must be the same as the executable's major version number,
**	and the minor version number of the image file must be less than
** 	or equal to the executable's minor version number.
**
**	In this case, the image's major version number is greater than
**	the executable's major version number which means the image was
**	created with a newer version of OpenROAD than the executable which
**	is being used to run the image file.
**
** System Status:
**	The image will not be run, and the program will exit.
**
** User Action:
**	Either run the image on a newer version of the executable
**	or create an image with an older version of OpenROAD.
*/

E_SE0006_IMGOLDER	"Version Mismatch: The image file is too old.\n
The image file '%0c' was created under an older version of OpenROAD than
this program.  This program cannot run this image file.  Check your
installation and make sure that makeimage, rundbapp, runimage have
all been installed at the same time.  The image's version number
is (%1d, %2d) while the runtime system's version number is (%3d, %4d)."

/*%
** Explanation:
**	OpenROAD image files are given a version number when created.
**	This version number consists of two parts, a major version and
**	a minor version.  For an image file to be compatible with
**	a OpenROAD executable, the major version number of the image
**	file must be the same as the executable's major version number,
**	and the minor version number of the image file must be less than
** 	or equal to the executable's minor version number.
**
**	In this case, the image's major version number is less than
**	the executable's major version number which means the image was
**	created with an older version of OpenROAD than the executable which
**	is being used to run the image file.
**
** System Status:
**	The image will not be run, and the program will exit.
**
** User Action:
**	Either run the image on an older version of the executable
**	or create an image with a newer version of OpenROAD.
*/

E_SE0007_MINORMISMATCH	"Version Mismatch: The image file is too new.\n
The image file '%0c' was created under a newer version of OpenROAD than
this program.  This program cannot run this image file.  Check your
installation and make sure that makeimage, rundbapp, runimage have
all been installed at the same time.  The image's version number
is (%1d, %2d) while the runtime system's version number is (%3d, %4d)."

/*%
** Explanation:
**	OpenROAD image files are given a version number when created.
**	This version number consists of two parts, a major version and
**	a minor version.  For an image file to be compatible with
**	a OpenROAD executable, the major version number of the image
**	file must be the same as the executable's major version number,
**	and the minor version number of the image file must be less than
** 	or equal to the executable's minor version number.
**
**	In this case, the image's minor version number is greater than
**	the executable's minor version number which means the image was
**	created with a newer version of OpenROAD than the executable which
**	is being used to run the image file.
**
** System Status:
**	The image will not be run, and the program will exit.
**
** User Action:
**	Either run the image on a newer version of the executable
**	or create an image with an older version of OpenROAD.
*/

E_SE0008_NODATABASE    "Database name given to application that doesn't use database.\n
The application is declared as not using a database, yet the -d flag was used
to specify a database to use.  The application will run, but the -d flag will
be ignored."

/*%
** Explanation:
**	The application was specified not to use a database, but
**	the -d flag was given.
**
** System Status:
**	The application will be run but the -d flag will be ignored.
**
** User Action:
**	Don't specify the -d flag with this database.
*/

E_SE0009_BADAPPFROMDB	"Could not retrieve the application '%0c'.\n
The application could not be gotten from the database.  Either the
application does not exist or there were errors retrieving the
application.  Check to see if the application exists."

/*%
** Explanation:
**	The application could not be retrieved from the database.
**	Either the application does not exist, or there were
**	errors trying to retrieve it.
**	Other errors may give more information on what is wrong.
**
** System Status:
**	The program will exit.
**
** User Action:
**	Check that the application exists, or follow the directions of
**	the other errors.
*/

E_SE000A_NO_CURRENT_DB	"No database name found for application.\n
The application's database property is set to use the current database,
however no current database exists.  This represents a problem when building
the image file for this application.   To run this application, you will
need to specify the -d flag and explicitly give the database name.  Report
this problem to your Ingres representative."

/*%
** Explanation:
**	The database property for the application was marked to use the
**	current database.  However, the image file does not contain
**	a database name.  This is a problem in Windows/4GL, and it
**	probably occurred when the image was created.
**
** System Status:
**	The application will not be run.
**
** User Action:
**	Explicitly specify the -d flag, and report the problem to
**	your Ingres representative.
*/

/*
**	backupapp.c
*/

S_SE000B_CONNECTING_TO_DB	"Connecting to database %c . . . "

E_SE000C_CANT_CONNECT_DB	"\nERROR: Cannot connect to database %c.\n"

/*%
** Explanation:
**	OpenROAD was not able to connect to the database that the
**	user specified.  The reason why the connect failed should
**	already have been specified in a preceding message.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check that the database name is spelled corrected and
**	that the desired INGRES installation is running.
**
*/

S_SE000D_DONE	"done.\n"

E_SE000E_CANT_FETCH_APP	"ERROR: Cannot fetch application %c from database %c.\n"

/*%
** Explanation:
**	OpenROAD could not fetch the specified application from the
**	database.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the spelling of both the application and database
**	names.
*/

E_SE000F_CANT_FETCH_COMP	"ERROR: Cannot fetch component %c from database.\n"

/*%
** Explanation:
**	OpenROAD was not able to retrieve the component of the database.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check that the database is in a consistent state.
*/

S_SE0010_DISCONNECTING	"Disconnecting from database %c . . . "

E_SE0011_UNKNOWN_FLAG	"ERROR: Unknown command flag \"-n%c\".\n"

/*%
** Explanation:
**	OpenROAD/backupapp was passed a command flag that it does
**	not support.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the command line used to invoke backuapp.
*/

E_SE0012_COMPNAME_WITH_NO_FLAG	"ERROR: Component name must be supplied with '-c' option.\n"

/*%
** Explanation:
**	OpenROAD/backuapp found a component name on the command line that
**	was not specified with the -c flag.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the command line used to invoke backuapp.
*/

E_SE0013_WRONG_MODE	"ERROR: Copy mode must be \"in\" or \"out\".\n"

/*%
** Explanation:
**	OpenROAD/backupapp was passed an unknown copy mode.  The only
**	modes supported at this time is "in" or "out".
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the command line used to invoke backuapp.
*/

/*
**	compileapp.c
*/

E_SE0014_APP_CANT_FETCH	"ERROR: Application %c cannot be fetched from database.\n"

/*%
** Explanation:
**	OpenROAD could not fetch the specified application from the
**	database.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the spelling of both the application and database
**	names.
*/

S_SE0015_COMPILING	"Compiling %c . . . "

S_SE0016_COMP_WARNINGS	"WARNING: Compile warnings in component %c.\n"

E_SE0017_COMP_ERRORS	"ERROR: Compile errors in component %c.\n"

/*%
** Explanation:
**	OpenROAD/compileapp encountered errors while compiling the
**	specified component.
**
** System Status:
**	OpenROAD/compileapp will exit
**
** User Action:
**	User should reviewed the component script for errors.
*/

S_SE0018_COMP_FLUSHED	"Component %c flushed to database.\n\n"

S_SE0019_FINISHED_COMP	"Finished compiling components.\n"

/*
**	make3gltab.qsc
*/

E_SE001A_CANT_OPEN_SOURCE	"ERROR: Cannot open source file.\n"

/*%
** Explanation:
**	OpenROAD cannot open the expected source file.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check permissions for creating files, disk quotas
**	and available free space.
*/

E_SE001B_APP_FETCH_FAILED	"ERROR: Fetch of application %c failed.\n"

/*%
** Explanation:
**	OpenROAD could not fetch the application.  Previous error messages
**	should provide the explanation.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	Retry the command.
*/

E_SE001C_COMP_FETCH_FAILED	"ERROR: Fetch of component %c failed.\n"

/*%
** Explanation:
**	OpenROAD could not fetch the specified component out of the
**	cache.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check that the the resource limits for the process
**	are large enough.
*/

E_SE001D_NO_3GL_PROC_REGIS	"ERROR: No 3GL procedures are registered with %c.\n"

/*%
** Explanation:
**	OpenROAD could not find any 3GL procedures registered for the
**	specified application.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check that the correct application was specified.
*/

E_SE001E_CANT_COMPILE_3GL	"ERROR: Unable to compile source file.\n"

/*%
** Explanation:
**	OpenROAD could not compile a 3GL source file or there were
**	compilation errors.
**
** System Status:
**	OpenROAD will continue processing of possible.
**
** User Action:
**	User should review the 3GL source for errors.
*/

/*
**	sphmerge.c
*/
E_SE001F_UNKNOWN_EXEC_TO_RUN	"\"%s\" executable cannot be run.\n"

/*%
** Explanation:
**	OpenROAD was given a program named that is not in its
**	lookup table.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check that the various windows4gl scripts and/or symbols
**	have not been tampered with.  Also, user should be running from
**	the various scripts that have been provided instead of directly
**	executing the sphmerge executable.
*/

/*
**	versionapp.c
*/

E_SE0020_BAD_P_FLAG	"ERROR: Invalid value for -p flag.  Use '-p{IGNORE|ABORT|VERSION}'.\n"

/*%
** Explanation:
**	OpenROAD detected an invalid value for the -p flag to versionapp.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the command line that is executing versionapp.
*/

E_SE0021_BAD_W_FLAG	"ERROR: Invalid value for -w flag.  Use '-w{IGNORE|VERSION}'.\n"

/*%
** Explanation:
**	OpenROAD detected an invalid value for the -w flag to versionapp.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the command line that is executing versionapp.
*/

E_SE0022_NO_NEW_APP_VERSION	
"New version of application %c not created.\nPlease correct the problems
shown in the previous messages and try again.\n\n"

/*%
** Explanation:
**	OpenROAD was not able to create a new version of the application.
**	Messages preceding this one should provide the detailed explanation.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should proceed based on the preceding messages.
*/

S_SE0023_CREATED_NEW_VERSION	"Created version %d of application %c.\n"

/*
**	createapp.c
*/

S_SE0024_CREATED_APP	"Application %c created.\n"

E_SE0025_APP_NOT_CREATED	"ERROR: Application %c NOT created.\n"


/*%
** Explanation:
**	OpenROAD could not create a new application.  Preceding messages
**	should provide more detailed information.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	Address the problem described in the messages before restarting.
*/

/*
**	destroyapp.c
*/

E_SE0032_APPLVER_CANT_FETCH
"ERROR: Version %0d of application %1c cannot be fetched from database.\n"

/*%
** Explanation:
**	OpenROAD could not fetch the specified application version from the
**	database.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check the spelling of both the application and database
**	names, as well as the specified version number.
*/


S_SE0026_APPL_DELETED	"Application %c deleted.\n"
S_SE0033_APPLVER_DELETED	"Version %0d of application %1c deleted.\n"

E_SE0027_APP_NOT_DELETED	"ERROR: Application %c NOT deleted.\n"
E_SE0034_APPLVER_NOT_DELETED	
"ERROR: Version %0d of application %1c NOT deleted.\n"

/*%
** Explanation:
**	OpenROAD could not destroy the application.  Preceding messages
**	should provide more detailed information.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	Address the problem described in the messages before restarting.
*/

/*
**	documentapp.c
*/

E_SE0028_CANT_OPEN_REP_FILE	"ERROR: Cannot open Application Report file %c.\n"

/*%
** Explanation:
**	OpenROAD was not able to output the file to write the application
**	report to.  Probably due to typo in name or permissions problem.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should check that the file name is spelled correctly, has
**	permission to create the file and there is enough space for the
**	file.
*/

S_SE0029_GENERATING_DOC	"Generating document for %c . . . "

S_SE002A_DOC_WAS_BUILT	"\nThe document file %c has been built successfully.\n"

/*
**	makeimage.c
*/

S_SE002B_IMAGE_FILE_BUILT	"\nThe image file %c has been built successfully.\n"

/*
**	purgeapp.c
*/

S_SE002C_APP_PURGED	"Application %c purged.\n"

E_SE002D_APP_NOT_PURGED	"ERROR: Application %c NOT purged.\n"

/*%
** Explanation:
**	OpenROAD was not able to purge the application.  Preceding
**	messages should provide more detailed information.
**
** System Status:
**	OpenROAD will exit.
**
** User Action:
**	User should address the problems described in the message before
**	restarting.
*/


E_SE0031_BAD_INCLAPP_FILE   "The included application override file '%0c' does not exist or is unreadable.\n
Make sure the file given on the command line is a valid file name, that the file exists, and that the file is accessible to you."

/*%
** Explanation:
**      The included app override file given by the user could not be opened.
**
** System Status:
**      The program will exit.
**
** User Action:
**      Correct the command line so that a valid file name is given.
*/

E_SE002F_BAD_INCLAPP_FORMAT "The included application definition '%0c %1c %2c' in override file %3c is invalid.\n
Refer to documentation for valid definition formats."


/*%
** Explanation:
**      The included application definition was invalid.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Edit the included application override file so that all included
**  app definitions are valid as per the documentation.
*/

E_SE0030_NO_INCLAPP_DEFS "The included application definition file %0c either was empty, or had no valid included application definitions."


/*%
** Explanation:
**      The included application override file was either empty, or none of
**  its included application definitions were in a valid format.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Edit the included application override file so that all included
**  app definitions are valid as per the documentation.
*/

S_SE002E_MemoryCorrupt	"Detection of memory corruption."

E_SE0035_OPENINGIMG "Opening image file %c . . . \n\n"

/*%
** Explanation:
**       An informational message to inform the user an image file
**  is being opened.
**
*/

S_SE0036_BAD_ARRAY
"Unable to allocate an array without elements straddling segment boundaries."

/*%
** Explanation:
**      Running on Windows, W4GL tried to allocate an array.
**	Given the array's element size and number of elements,
**	at least one element will straddle a segment boundary,
**	no matter how the array is positioned in memory.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Please report this internal error to your Technical Support
**	representative.
*/

S_SE0037_IMAGE_FILE_BUILT	"\nThe image file %c has been built, but does not contain all application components.\nPlease refer to the output log of this operation to identify the specific components missing from the image file.\n"

/*%
** Explanation:
**    When building an image file, if an application component cannot be
**  written to the file, the file is built anyway with the component missing.
**
** User Action:
**    Review the output log of the image file building process to see which
**  specific application components could not be written to the image, and
**  why.
*/

E_SE0038_NO_RUN_LICENSE
"License check failed.  No valid OpenROAD Runtime license for this installation.\n"

/*%
** Explanation:
**      OpenROAD was unable to verify a valid OpenROAD Runtime license 
**	for this installation.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Check your Ingres Corporation Licensing setup.
*/

E_SE0039_NO_DEV_LICENSE
"License check failed.  No valid OpenROAD Development license for this installation.\n"

/*%
** Explanation:
**      OpenROAD was unable to verify a valid OpenROAD Development license 
**	for this installation.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Check your Ingres Corporation Licensing setup.
*/

E_SE003A_APP_ALREADY_EXISTS
"ERROR: The application could not be created because an application named '%c' already exists in this database.\n"

/*%
** Explanation:
**      The createapp utility was asked to create an application that already
**	exists.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Try a new application name.
*/

E_SE003B_CANT_FETCH_APP_VER
"ERROR: Version %d of application %c could not be fetched from database.\n"

/*%
** Explanation:
**      The utility was unable to fetch the specified version of the
**	specified application.  It could be because the application doesn't
**	exist at all, or because the specified version of the application 
**	doesn't exist.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Check that the application name was spelled correctly, and that the
**	version number was specified correctly.
*/

E_SE003C_CANT_FETCH_COMP_VER
"ERROR: Version %d of component %c could not be fetched from database.\n"

/*%
** Explanation:
**      The utility was unable to fetch the specified version of the
**	specified component.  It could be because the component doesn't
**	exist at all, or because the specified version of the component 
**	doesn't exist.
**
** System Status:
**      The program will exit.
**
** User Action:
**	Check that the component name was spelled correctly, and that the
**	version number was specified correctly.
*/

E_SE003D_BADAPPVERFROMDB
"Could not retrieve version %0d of the application '%1c' from the database.\n"

/*%
** Explanation:
**	The application version specified could not be retrieved 
**	from the database.  Either the application version does 
**	not exist, or there were errors trying to retrieve it.
**	Other errors may give more information on what is wrong.
**
** System Status:
**	The program will exit.
**
** User Action:
**	Check that the application exists, or follow the directions of
**	the other errors.
*/

E_SE003E_BAD_FLAG_COMBINATION
"Either the -s (scpgen) or -m (metadatagen) flag was specified in addition to the -v (version) flag.  The -s or -m flag will be ignored.\n"

E_SE003F_BAD_FLAG_COMBINATION
"The -r (interfaceversion) or -n (interfacename) flag was specified without specifying the -m (metadatagen) flag.\nThe -r or -n flag will be ignored.\n"

E_SE004A_BAD_FLAG_COMBINATION
"Both the -l (componentlist) and the -c (component) flags were specified. The -c flag will be ignored.\n"

E_SE004B_BAD_COMPONENT_LIST
"The component list file contains an invalid component name, component file pair '%c'\n"
