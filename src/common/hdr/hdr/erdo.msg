/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:    erdo.msg -  Sapphire Data Dictionary Message File
**
** Description:
**      Contains the errors for the Windows4gl Data Dictionary.
**
** History:
**	MEvans	4/19/90
**		MEvans
**	26-jun-1990 (Joe)
**	    Added compile warning error.
**	11/20/91 (dkh) - ER changes for internationalization.
**	04/25/92 (emerson)
**		Added messages E_DO0069 and E_DO006A, which supersede E_DO0060.
**
**	W4GL III
**	5-feb-1993 (emerson)
**		Added messages E_DO0070 and E_DO0071, for the removal of
**		the old ILRF.
**	6-feb-1995 (donc) Bugs63315, 52492, 50279
**		Missing 0 in E_DO0067 was causing ERlookup errors.
**	18-may-95 (leighb) Bug 68453
**		Added E_DO0085_cant_open_img_file.
**	22-aug-95 (spepa01)
**		add E_DO0086 for ner_us5 port
**	17-nov-95 (lan)
**		Added E_DO0087_PARENT_UNDEFINED.
**	20-jan-97 (trevor)
**              Added E_DO007A_CANNOT_WRITE_TEMPFILE
**	21-jan-98 (phach01)
**		Added E_DO007B_NO3GL_LIB
**	25-dec-1999 (donc) Bug 95006
**		Added E_DO0089_INVALID_IMAGE_VERSION.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**              Ingres message file cleanup.
**	14-apr-2008 (zhayu01) SIR 120626
**		Added E_DO0090_INVALID_UTF8_FILE.
**/

#define _DO_CLASS       191

E_DO0000_unknown_error "An unknown error condition has been generated.\n"

E_DO0001_BE_error "An unexpected DBMS error has been generated.\n"

E_DO0002_object_locked "The source object is locked.\n"

E_DO0003_exclusive_lock "The object is in the process of being checked in.\n"

E_DO0004_no_lock "The source object is not locked.\n"

E_DO0005_no_object "The source object does not exist.\n"

E_DO0006_cannot_lock "The source object cannot be locked.\n"

E_DO0007_cant_get_dict "No dictionary defined.\n"

E_DO0008_no_storagetype "The object has no storage type.\n"

E_DO0009_encoded_flush_fail "The object encodings could not be flushed.\n"

E_DO0010_expanded_flush_fail "The expanded definition could not be flushed.\n"

E_DO0011_locked_by_user_difses "You have a lock in another session.\n"

E_DO0012_already_in_memory "The object is already in memory.\n"

E_DO0013_encoded_delete_fail "The encoded definition could not be deleted.\n"

E_DO0014_expanded_delete_fail "The expanded definition could not be deleted.\n"

E_DO0015_component_in_app "The component is in the application.\n"

E_DO0016_compile_error "There was a compilation error.\n"

E_DO0017_cant_open_menu_file "Cannot open menu file.\n"

E_DO0018_cant_create_base "Cannot create base entry.\n"

E_DO0019_cant_create_working "Cannot create working copy.\n"

E_DO0020_already_exists_in_db "Object already exists in the database.\n"

E_DO0021_component_in_use "The component is in use.\n"

E_DO0022_comp_in_cache "The component is in the cache.\n"

E_DO0023_comp_not_in_cache "The component is not in the cache.\n"

E_DO0024_same_as_last_version "The component is the same as the last version.\n"

E_DO0025_cycle_in_incl_applist "The included application list cycles.\n
Somewhere the chain of included applications defined for %0c closes in on
itself, resulting in an infinite cycle of links that cannot be resolved.
Check the included application chain, and remove any links that cause cycling."

E_DO0026_cant_open_file "Cannot open file '%0c'.\n"

E_DO0027_bad_image_appname
"The file '%0c' is supposed to contain the application '%1c'.  Instead it
contains '%2c'."

E_DO0028_bad_image_appversion
"The image file '%0c' is supposed to contain application version '%1c'.\n
It contains version '%2c' instead.\n
Because application components in the image file may have changed between versions, any reference to them could result in unexpected behavior.\n"

E_DO0029_bad_file_type "File '%0c' is not a valid image file.\n"

E_DO0030_dict_not_installed "The dictionary is not installed.\n"

E_DO0031_dict_client_unusable "The dictionary client is not usable.\n"

E_DO0032_appname_not_specified "The application name is not specified.\n"

E_DO0033_file_not_spec "The image file name is not specified.\n"

E_DO0034_nameexists_in_db "The object name already exists.\n"

E_DO0035_OPENDB_BAD    "OpenDatabase set to incorrect value.\n
The attribute OpenDatabase of an AppSource was set to an illegal value.
The only legal values are DB_NODB, DB_DEFAULT and DB_STRING."

/*%
** Explanation:
** 	The attribute OpenDatabase was set to an incorrect value.  It
** 	can only take one of the values DB_NODB, DB_DEFAULT or DB_STRING.
**
** System Status:
** 	The attribute's value will not be changed.
**
** User Action:
**	Change the assignment so that a good value is given.
*/

E_DO0036_COMPILE_WARNING	"There were compilation warnings.\n"

E_DO0037_BAD_ENTITYID	"The object has an invalid entity id.\n
The DB Catalogue identification number of the object is invalid. Please
contact Ingres technical support for assistance."

E_DO0038_BADIMAGE "The image file does not contain all application components.\n
The image file %0c encountered a problem while attempting to load component %1c,
and therefore does not contain it.  If the problem cannot be addressed, caution
should be used when executing the image."

E_DO0039_CANNOT_OPENFILE "The image file %0c cannot be opened.\n"

E_DO0040_NOIMAGE "The image file could not be created.\n
A non-recoverable error occurred during the creation of the application image
file %0c, so the file could not be created."

E_DO0041_APPRES		"Error fetching component information.\n
There was some kind of error fetching information for some of the
components of the application '%0c'.  Subsequent errors should give
exact details on the errors encountered.  These errors should be corrected
before the application is run."

/*%
** Parameters:
**	0		The name of the application that had problems.
**
** Explanation:
**	While fetching the information about the components of an application
**	an error was encountered.  This message simply tells that the
**	error occurred in a certain application.  Subsequent errors should
**	give more details.
**
** System Status:
**	The system will continue running, but problems may be encountered
**	later.
**
** User Action:
**	Fix the problems described in the subsequent errors.
*/

E_DO0042_PROCRES	"Error fetching frame or procedure information.\n
There was some kind of error fetching information for some of the
components of the frame or procedure '%0c' that is contained in the
application '%1c'.  Subsequent errors should give exact details on the
errors encountered.  These errors must be corrected before the component
is run."

/*%
** Parameters:
**	0		The name of the frame or procedure that had problems.
**
**	1		The name of the application containing the errors.
**
** Explanation:
**	While fetching the information about a frame or procedure, an
**	error was encountered.   This message simply tells that the
**	error occurred in a certain frame or procedure.  Subsequent errors
**	should give more details.
**
** System Status:
**	The system will continue running, but this procedure will
**	not be runnable.
**
** User Action:
**	Fix the problems described in the subsequent errors.
*/

E_DO0043_CANNOT_INCLAPP	"Error including application %0c.\n
A non-recoverable error occurred while attempting to load the included
application %0c.  Any references to its components by the host application
will not be able to be resolved.\n\
Subsequent errors should give more details."

E_DO0044_NO3GL	"User 3GL library does not contain procedure '%0c'."

F_DO0000_SRCFOR	"%0c for '%1c'"

F_DO0001_SRCNULL "%0c for NULL name"


/*
**	mappsrc.qsc
*/
S_DO0045_SETTING_APP_ICON	"Setting application icon.\n"
/*
**	mcompsrc.qsc
*/

S_DO0046_COMPONENT_EXISTS	"\nWARNING: Component %c already exists in database.\n"

S_DO0047_WANT_TO_ABORT	"Do you wish to abort? "

S_DO0048_WANT_TO_VERSION	"Do you wish to create a new version to replace the existing component? "

S_DO0049_WANT_TO_REPLACE	"Do you wish to replace the existing component? "

S_DO004A_NO_OVERRIDE	"Component %c will not be overridden since it already exists.\n"

S_DO004B_LOADING_MSG	"Loading %c into database . . . "

/*
**	mdbcache.qsc
*/

E_DO004C_DBCACHE_ALLOC_ERR	"Windows4gl could not allocate a hash table for the database object cache.\nProcessing can not continue.\n"

/*%
** Explanation:
**	Windows4gl was not able to allocate a hash table needed to
**	handle DBcache.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None.  User should check that there are adequate process and
**	system limits to run Windows4gl.
*/

F_DO0002_FETCHING_FROM_IMAGE	"Fetching %c from image . . . "

F_DO0003_FETCHING_DONE	"done.\n"

F_DO0004_FETCHING_FROM_DB	"Fetching %c from database . . . "

F_DO0005_FILE_OPENED_READ	"\nFile %c opened for read.\n"

F_DO0006_OBJECT_FETCHED	"\nObject %c fetched from %c.\n"

F_DO0007_FILE_CLOSED	"\nFile %c closed.\n"

/*
**	mimapsrc.c
*/

E_DO004D_IMGAPPSOURCE_VERSION	"The IMGAPPSOURCE object (%x) has version# %d, a non-supported version.\n  Processing can not continue\n"

/*%
** Explanation:
**	Windows4gl detected an attempt to use an image app source object
**	which has a version number that is not supported.  This is a
**	fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	User should check to make sure that the image file has not been
**	altered and that the versions of the image and sphmerge are
**	compatible.
*/

E_DO004E_FILE_INCOMPATIBLE	"ERROR: File format is incompatible with the current release.\n"

/*%
** Explanation:
**	The file format passed to windows4gl can not be handled by the
**	current release.  That is, the major version number does not
**	match or the minor version number is too old.
**
** System Status:
**	Windows4gl exits gracefully.
**
** User Action:
**	User should check the origin of the file passed to windows4gl.
*/

S_DO004F_APP_EXISTS	"\nWARNING: Application %c already exists in the database.\n"

S_DO0050_MERGE_APP	"Do you wish to merge with the existing application? "

S_DO0051_VERSION_APP	"Do you wish to create a new version of the application? "

S_DO0052_REPLACE_APP	"Do you wish to replace the existing application? "

S_DO0053_ABORT_APP	"Do you wish to abort? "

S_DO0054_VERSIONING_APP	"\nVersioning application %c . . . "

S_DO0055_OVERRIDE_APP	"\nOverriding application %c . . . \n\n"

S_DO0056_NO_APP_OVERRIDE	"Application %c will not be overridden since it already exists.\n"

F_DO0008_OBJECT_FETCHED	"\nObject %c fetched from %c.\n"

S_DO0057_COMPILING_COMPS	"Compiling application components . . . \n\n"

S_DO0058_APP_LOADED	"Application %c loaded into database.\n"

/*
**	msrcobj.qsc
*/

S_DO0059_CANT_OPEN_FILE	"Cannot open file %c\n"

S_DO005A_FILE_OPEN_WRITE	"\nFile %c opened for write.\n\n"

E_DO005B_IN_USE	"Old version of frame '%c' is in use,\n
and cannot be recompiled at this time.  Breakpoints and linenumber information
for field scripts will not behave properly until frame is recompiled."

/*%
** Explanation:
**	User is using the debugger, and has encountered an old version
**	compiled frame in the database.  Line numbering information for
**	field scripts is incorrect, although the frame script will be OK.
**
** System Status:
**	Debugger will behave incorrectly when stepping through or trying to
**	set breaks in field scripts.
**
** User Action:
**	If users frame doesn't contain any field scripts, it doesn't matter.
**	Otherwise, this problem will go away the next time the frame compiles.
*/

E_DO005C_IN_USE	"Frame '%c' is in use,\n
and cannot be recompiled at this time.  Breakpoints and linenumber information
may not properly reflect the current script until frame is recompiled."

/*%
** Explanation:
**	User is using the debugger, and has encountered a stale frame
**	which is in use.  Breakpoint and line numbering information may
**	be out of synch.
**
** System Status:
**	Debugger may behave incorrectly when stepping or trying to
**	set breakpoints.
**
** User Action:
**	If script is not very different from last compilation, user may
**	not notice anything.  Problem will go away when frame recompiles.
*/

/*
**	mappsrc.qsc
*/

F_DO0009_CHECKIN_COMPONENT	"Checking in component '%c'.\n"

F_DO000A_VERS_COMP_CREATED	"Version %d of component '%c' created.\n"

F_DO000B_USING_VERS_COMP	"Using version %d of component '%c'.\n"

S_DO005D_NUMBER_OF_COMPS	"\nThere are %d components in application %c.\n\n"

F_DO000C_WRITING_TO	"Writing %c to %c . . . "

F_DO0045_WRITING_APP_TO	"\nWriting application %c to %c . . . \n\n"

F_DO000D_DONE_WITH_IT	"done (%d of %d).\n\n"

F_DO000E_DESTROYED_VERS_COMP	"Destroyed version %d of component '%c'.\n"

F_DO000F_GENERATING_REP	"Generating report for %c . . . "

F_DO0010_COMPILING_IT	"Compiling %c . . . "

F_DO0011_COMP_WARNING	"WARNING: Compile warnings in component %c.\n"

F_DO0012_COMP_ERRORS	"ERROR: Compile errors in component %c.\n"

F_DO0013_COMP_FLUSHED	"Component %c flushed to database.\n\n"

S_DO005E_APP_LOADED	"Application %c loaded into %c.\n\n"

E_DO005F_ERR_DESTROY 	"Error destroying version %d of component '%c'."

E_DO0060_INV_IMGAPP_VERS
"The included application %0c has the illegal version number %1c.  The
application cannot be included."

E_DO0061_COMP_LOCKED_YOU "Component %0c is locked by you."
E_DO0062_COMP_LOCKED "Component %0c is locked by user %1c."
E_DO0063_COMP_CHECKEDOUT_YOU "Component %0c has a private version owned by you."
E_DO0064_COMP_CHECKEDOUT "Component %0c has a private version owned by user %1c."
E_DO0065_CANNOTFETCH	"Unable to fetch component %0c."
E_DO0066_CANNOTFLUSHWORKING	
"Unable to check in your private copy of component %0c."
E_DO0067_NOVERSIONS
"Component %0c has no numbered versions."

/*
**	Miscellaneous source files.
*/
E_DO0068_COMP_ERRORS	"ERROR: Compile errors in component %0c.\n"

E_DO0069_bad_image_type "File '%0c' is invalid.\n
It does not appear to be an image file created by Windows/4GL."

E_DO006A_bad_image_version "File '%0c' is unusable.\n
It appears to be an image file created by a future, incompatible release of
Windows/4GL.  You may wish to recreate it using the makeimage utility
of the currently-installed release of Windows/4GL."

S_DO006B_3gl_loaded	"3GL procedure '%0c' loaded at hex address %1x."

E_DO006C_COMPONENT_EXECUTING
"Invalid change to  ParentApplication.\n
You have attempted to change the ParentApplication attribute of component
'%0c' while it is executing.  This is not allowed.  ParentApplication will 
keep its current value."
/*%
** Explanation:
**	Code has attempted to set the ParentApplication attribute of a
**	component which is currently executing.  This is not allowed.
**
** System Status:
**	The ParentApplication attribute will not be changed.  If it was
**	being changed to NULL, this will result in the component not being
**	removed from the cache.
**
** User Action:
**	None required in most cases.  If it is important to change the
**	ParentApplication attribute; wait until the component is no longer
**	executing to do so.
*/

E_DO0073_CANNOT_OPEN_TEMPFILE
"An intermediate storage file could not be opened in the temporary file area.\n
Make sure the temporary file area exists, has room, and is available to you."

/*%
** Explanation:
**	The system tried to open a file in the temporary file area for
**	intermediate storage, and couldn't.
**
** System Status:
**	Execution cannot continue because the intermediate storage file
**	is required for whatever action is taking place.
**
** User Action:
**	The file couldn't be opened for some reason; the user has to find
**	out what it is.  Either it doesn't exist, is full, or is inaccessible
**	to the user.
*/

E_DO006D_FRAME_IN_BATCH
"Frame instantiation in batch mode.\n
Attempt to call frame '%0c' while in batch mode.  In batch mode, only
ghostframes may be called."

/*%
** Explanation:
**	An image running in batch mode has attempted to instantiate a frame
**	with a window.
**
** System Status:
**	Frame will not execute.
**
** User Action:
**	Do not run this application in batch, or modify it to run in batch mode.
*/

E_DO006E_CANNOT_CREATE_FROM
"Component %0c of type %1c cannot be created from type %2c."

/*%
** Explanation:
**	An attempt was made to create a new component type from a source
**	object that was incompatible with the new type.
**
** System Status:
**	The new component will not be created.
**
** User Action:
**	Use compatible source types when creating from existing source.
*/

S_DO006F_OVERRIDING_INCLAPP
"The existing included application definition '%0c %1c %2c' has been overridden by the new definition '%3c %4c %5c'."

/*%
** Explanation:
**	An included application definition is being superseded by another
**  definition for the application from an override file.
**
** System Status:
**	The new included application will be loaded instead of the defined
**  one.
**
** User Action:
**	This is not an error.
*/

E_DO0070_BADOLDFRAME	"Error fetching frame or procedure from old image.\n
An error occurred fetching frame or procedure '%0c' from an image file
for application '%1c', created using Windows/4GL release 1 or 2.
You may wish to recreate it using the makeimage utility
of the currently-installed release of Windows/4GL."

/*%
** Parameters:
**	0		The name of the frame or procedure that had problems.
**
**	1		The name of the application containing the errors.
**
** Explanation:
**	While fetching a frame or procedure in an image file that was
**	created using Windows/4GL release 1 or 2, an error occurred.
**
** System Status:
**	The system will continue running, but this procedure will
**	not be runnable.
**
** User Action:
**	Contact Technical Support.  You may wish to work around the problem
**	by recreating the image using the makeimage utility
**	of the currently-installed release of Windows/4GL.
*/

E_DO0071_BADOLDAPP	"Error fetching application from old image.\n
An error occurred fetching global information from an image file
for application '%0c', created using Windows/4GL release 1 or 2.
You may wish to recreate it using the makeimage utility
of the currently-installed release of Windows/4GL."

/*%
** Parameters:
**	0		The name of the application containing the errors.
**
** Explanation:
**	While fetching global information from an image file that was
**	created using Windows/4GL release 1 or 2, an error occurred.
**
** System Status:
**	The application will not be run.
**
** User Action:
**	Contact Technical Support.  You may wish to work around the problem
**	by recreating the image using the makeimage utility
**	of the currently-installed release of Windows/4GL.
*/

E_DO0072_BADLIBIMAGE "Error compiling component in library image.\n
While building library image file %0c, an error was generated while compiling
component %1c.  Although the library image file will still contain the
component, exercise caution when using it." 

/*%
** Parameters:
**      0               The name of the application containing the errors.
**
** Explanation:
**      While fetching global information from an image file that was
**      created using Windows/4GL release 1 or 2, an error occurred.
**
** System Status:
**      The application will not be run.
**
** User Action:
**      Contact Technical Support.  You may wish to work around the problem
**      by recreating the image using the makeimage utility
**      of the currently-installed release of Windows/4GL.
*/

E_DO0074_IMGFILE_BUILD_EXCEPTI
"A fatal error has occurred while trying to create the image file.  The image
file will be deleted."

/*%
** Explanation:
**	An unhandled exception occurred during the build of the image file.
**
** System Status:
**	The partially built image file is deleted.
**
** User Action:
**      Contact Technical Support. 
*/

E_DO0075_PARENTAPP_UNDEFINED
"The parent application of the class %0c has not been defined."

/*%
** Explanation:
**      The user has not set the ParentApplication attribute of their
**  user class source object.
**
** System Status:
**      The user class object cannot be instantiated.
**
** User Action:
**      Set the ParentApplication attribute of the CLASSSOURCE object
**      to be the current application.
*/

E_DO0076_CANNOTFETCH    "Unable to fetch application %0c."

/*%
** Explanation:
**      An attempt was made to load an application that doesn't exist.
**
** System Status:
**      unknown
**
** User Action:
**      Check to see if application name is correct and expected.
*/

E_DO0077_CANNOTPARENT  "Unable to parent unnamed component.\n
An application component must be named before it can be parented."

/*%
** Explanation:
**      An attempt was made to load an application that doesn't exist.
**
** System Status:
**      unknown
**
** User Action:
**      Check to see if application name is correct and expected.
*/

E_DO0078_CANNOTDELETEAPP "Unable to delete application %0c.\n
The application cannot be deleted because component %1c is locked by %2c."

/*%
** Explanation:
**      An attempt was made to delete an application that has components
**      that are in the process of being edited.
**
** System Status:
**      unknown
**
** User Action:
**      Check to see if application name is correct and expected.
*/


E_DO0079_CANNOTCREATECLASS "Unable to create class %0c.\n
The class cannot be created because objects of its type are currently instantiated.\n"

/*%
** Explanation:
**      An attempt was made to invoke the 'createclass' method on a
**      CLASSSOURCE object which already has been created, and has objects
**      of its type currently instantiated.
**
** System Status:
**      unknown
**
** User Action:
**      All instances of the class must be set to NULL before the class
**      can be created again.
*/

E_DO007A_CANNOT_WRITE_TEMPFILE "Unable to write object to temporary file.\n
This may be caused by lack of disk space in the II_TEMPORARY area\n"

/*%
** Explanation:
**      An attempt was made write an object to a temporary file
**      prior to flushing to the database. The write failed,
**      possibly, due to lack of disk space
**
** System Status:
**      unknown
**
** User Action:
**      Check disk space on the partition used by II_TEMPORARY.
**      Make room and retry.
*/

/*%
** Explanation:
**	Cannot find the 3GL procedure in the specified library.
**
** System Status:
**	The 3GL procedure cannot be instantiated.
**
** User Action:
**	Check if the specified library exists, and actually contains the
**	procedure in question.
*/
E_DO007B_NO3GL_LIB	"User 3GL library '%0c' does not contain procedure '%1c'."


E_DO0080_CANNOTPARENTCOMP "Unable to parent component %0c.\n
Only application components created dynamically can have their parent applications assigned at runtime.\n"

/*%
** Explanation:
**      An attempt was made to invoke the 'createclass' method on a
**      CLASSSOURCE object which already has been created, and has objects
**      of its type currently instantiated.
**
** System Status:
**      unknown
**
** User Action:
**      All instances of the class must be set to NULL before the class
**      can be created again.
*/

E_DO0081_BADINCLAPPS	
"Cannot run application with invalid included applications.\n\
There was a serious error loading the included application for application
'%0c'.  Please correct the included applications list and try again."

/*%
** Explanation:
**      An attempt was made to run an application with a serious problem
**	with its included applications, probably a cycle.
**
** System Status:
**      The application cannot be run.
**
** User Action:
**      Fix the problem in the included applications list and try again.
*/

E_DO0082_DIFFVERSIONS
"Warning: Application '%0c' has version '%1c'; Expected version is '%2c'."

/*%
** Explanation:
**      An attempt was made to run an application with different
**      versions of the same included application.
**
** System Status:
**      The application will run.
**
** User Action:
**      User may check whether the versions are correct.
*/

E_DO0083_INVALID_COMBINATION
"An imaged application cannot include an application from the database.\n\
Imaged application '%0c' is attempting to include application '%1c' from the database. This is not allowed."

/*%
** Explanation:
**      An attempt was made to run an application with different
**      versions of the same included application.
**
** System Status:
**      The application will not run.
**
** User Action:
**      Fix the problem specifying a correct included applications list 
*/

E_DO0084_OLDER_IISYSTEM
"Warning: application '%0c' expects version %1d of iisystem.\n\
Version %2d is installed."

/*%
** Explanation:
**      An imaged application was built against a newer version of iisystem
**	than what is currently installed.
**
** System Status:
**      The application will not run, but may act strangely.  In particular,
**	it may not find components it expects to be in iisystem.
**
** User Action:
**      Install a nwere version of iisystem or reimage the application.
*/

E_DO0085_cant_open_img_file
"Cannot open the image file in the directories indicated by\n\
II_W4GLAPPS_SYS and II_W4GLAPPS_DIR and cannot open file '%0c'.\n"

/*%
** Explanation:
**	An image file is searched for first in the directory indicated by
**	the II_W4GLAPPS_SYS environment variable, if it is set.  If not found
**	it is searched for in the directory indicated by the II_W4GLAPPS_DIR 
**	environment variable, if it is set.  If still not found, it is 
**	searched for in $II_SYSTEM\ingres\w4glapps.  This message indicates
**	it could not be located in any of the 3 directories.
**
** System Status:
**      The application will not run.
**
** User Action:
**	If the image name is wrong, use the right name.  If the name is
**	correct, put the image file in one of the 3 indicated places.
**	If one of the II_W4GLAPPS_ environment variables points to the
**	wrong location, correct it.
*/

E_DO0086_CANNOT_OPEN_TEMPFILE
"An intermediate storage file could not be opened in the temporary file area.\n
Make sure the temporary file area exists, has room, and is available to you."

/*%
** Explanation:
**      The system tried to open a file in the temporary file area for
**      intermediate storage, and couldn't.
**
** System Status:
**      Execution cannot continue because the intermediate storage file
**      is required for whatever action is taking place.
**
** User Action:
**      The file couldn't be opened for some reason; the user has to find
**      out what it is.  Either it doesn't exist, is full, or is inaccessible
**      to the user.
*/
E_DO0087_PARENT_UNDEFINED
"Entity of type %0c has not been assigned a parent."

/*%
** Explanation:
**	The entity of type COMPSOURCE or FRAMESOURCE has not been assigned a
**	parent.
**
** System Status:
**	The entity cannot be instantiated.
**
** User Action:
**	Assigned a parent to the entity.  A parent can be the current frame
**	or component.
*/

E_DO0088_CANNOT_DEP
"Dependency information cannot be generated for component %c."

/*%
** Explanation:
**	There was an error entering dependency information for a component.
**
** System Status:
**
**
** User Action:
*/

E_DO0089_INVALID_IMAGE_VERSION
"Error: Application image for '%0c' was created with
OpenROAD version %1c. The application needs to be
recompiled, and a new image made, using OpenROAD %2c."

/*%
** Explanation:
**      An attempt was made to run an application created under an earlier 
**      version of OpenROAD. 
**
** System Status:
**      The application will fail.
**
** User Action:
**      User needs to create the image file with a newer OpenROAD version.
*/

E_DO0090_INVALID_UTF8_FILE
"An error occurred while reading the file: '%0c'.  
Make sure that the file '%0c' dose not contain any invalid UTF8 characters."

/*%
** Explanation:
**      The export file contains invalid UTF8 characters. 
**
** System Status:
**      The application or component will not be imported.
**
** User Action:
**      User needs to get rid of invalid UTF8 characters.
*/
