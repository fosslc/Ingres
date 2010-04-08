/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erui.msg -    Front-End DBMS Utilities Message Definitions File.
**
** Description:
**	Contains the message definitions for the front-end DBMS utilities,
**	class UI.
**
** History:
**  26-apr-95 (angusm)
**      Add prompt for ROLE id (bug 53689)
**
**	Revision 6.4  89/12  wong
**	Added role, user password prompts.
**	89/12  bobm  Added compatibility error messages.
**	89/12  kenl  Added DG read-only DB  error message.
**
**	Revision 6.0  87/07/28  peter
**	Initial revision.
**	23-oct-1990 (mgw)
**		Corrected some misspellings.
**	10/1/90 (pete) Changed E_UI0029_UPGRADE_DBA & E_UI002A_UPGRADE_NOTDBA
**		to reference upgradefe.
**	13-aug-1991 (pete)
**		Changed "client" to "product". That's how its documented now.
**		Also added 2 new messages for bug 39165.
**	24-jun-93 (blaise)
**		Added E_UI003A...3E: error messages for the 4gl
**		sequence_value() function, which has been moved from abf
**		to utils.
**	8/93 (Mike S) Add sequenced value errors.
**	18-feb-94 (sandyd)
**		Added E_UI003F... for DBI project.
**	4/94 (Mike S)
**		Add E_UI0040
**      11-mar-1995 (harpa06)
**              Added E_UI0041_StarTableErr for STAR users trying to use user
**		default attribute values in STAR tables that use catalogs below 
**		the OPING2.0 level. This is for Bug #67891.
**	13-nov-2008 (dougi)
**	    Added UI000A_Ident & UI000B_Seq for identity columns & sequence 
**	    defaults.
**/

# define	_UI_CLASS	34

E_UI0000_OK
"Successful return from UI"
E_UI0001_BLOCK_CLOSED	
"query block is currently closed"
E_UI0002_EOS
"no more records in set"
E_UI0003_TOO_MANY_TUP
"too many tuples returned for a single tuple qualification"
E_UI0004_INIT_ERR
"Error in accessing database capabilities"

E_UI0010_Transaction	"INGRES sub-system committing open transaction.\n
An INGRES sub-system was called with an open transaction still active on
the DBMS session.  Neither transactions nor cursors can remain open across
sub-system calls."

/*%
** Explanation:
**	INGRES sub-systems called through the OSL "call <sub-system> ..." or the
**	EQL "call <sub-system> (database = ... )" statements share the calling
**	application's DBMS session.  Since INGRES sub-systems have their own
**	transaction semantics (partly to avoid locking system tables,) it is a
**	mistake to call one while a transaction is still active on the DBMS
**	session.
**
** System Status:
**	The open transaction will be committed and any open cursors closed.
**
** User Action:
**	Commit all transactions before calling any INGRES sub-system.
*/

E_UI0011_Connect_err	"Error in query language specified.\n
The database connection ws created with an the query language
specified as QUEL.  This is not an option with this installation."

/*%
** Explanation:
**
** System Status:
**	A fatal error will be issued.
**
** User Action:
*/

/* Password Prompt Messages for User Password and Role Password */

S_UI0012_PasswordPrompt		"Please enter your password:\n\r"

S_UI0013_RolePasswordPrompt	"Please enter a password for role %0c:\r\n"

E_UI0016_ReadOnly_err   "Database accessed as read-only.\n
The attempted operation cannot be performed on a database that was opened for
reading only (read-only access.)"

/*%
** Explanation:
**	Data-General gateway, only.  A database was accessed using a read-only
**	connection, but you selected an operation (such as Save from VIFRED)
**	that would update the database.  The operation cannot be completed in
**	this case.
**
** System Status:
**	The operation will not be performed.
**
** User Action:
**	Only perform operations that do not require data to be added or modified
**	in the database.  Or, exit the program and re-execute it by connecting
**	to the database in update mode.
**	connection.
*/

E_UI0017_DictInstallErr   "Unable to upgrade FrontEnd dictionary.\n
An error occurred trying to upgrade the database to be compatible with this
INGRES application.  The internal routine 'UTexe()' returned status '%1d' while
calling '%2c' to install or upgrade the dictionary for front-end
product '%0c'.\n
To perform the dictionary installation, you will have to run the
'%2c' utility program directly, and give it the product name '%0c'
(program '%2c' is described in your INGRES/SQL Reference manual).
Contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	UTexe is an internal INGRES routine that can start other INGRES
**	programs. In this case it executed "upgradefe", and a return
**	code indicated that "upgradefe" did not work properly.
**
** System Status:
**	A fatal error will be issued. The program does not start.
**
** User Action:
**	Run "upgradefe" for the dictionary mentioned in the error message. 
**	See your INGRES/SQL reference manual for information on how to run
**	"upgradefe" from the command line.
*/

S_UI0018_UpgradingDict	"Upgrading FrontEnd dictionary . . ."

S_UI0020_Depend	"Cleaning up ABF dependency tables . . ."

S_UI0021_FidFix	"Conversion of old ABF dependencies (this may take a long time) . . ."

E_UI002F_MODULE_FILE_MISSING	"Error occurred creating front-end dictionary\n
An error occurred while reading the .cre and .cli files in the 'dictfiles'
subdirectory of the INGRES 'files' directory. The front-end dictionary module
'%0c', version '%1c' is referenced in file '%2c.cli', but the corresponding
.cre file for this module does not exist (the .cre file contains the SQL
statements that create the module; the first line in the missing .cre
file should contain the module name, %0c, followed by the version, %1c,
and then a comment).\n
This error in your dictionary files must be corrected before
the front-end dictionary can be created. Check if this installation has an
incomplete set of front-end dictionary files. Contact your Site Technical
Contact for assistance.  Exiting . . ."

/*%
** Explanation:
**	Upgradefe read a row from a .cli file that identified a
**	front-end dictionary module and version. However, the .cre
**	file for that module could not be found. The .cre file
**	gives the SQL statements required to create the module.
**
** System Status:
**	The front-end dictionary is not created or converted. Upgradefe
**	exits.
**
** User Action:
**	Check if a .cre file has been removed from your installation.
*/

S_UI002E_DepFix 
"Older versions of ABF have written to the catalogs.\n
Pre-INGRES 6.3/03 versions of ABF have written records to the system catalogs
whose format is incompatible with the format currently used.  We will convert
the records which are part of the current application.  Running SYSMOD will
convert all such records."

/*%
** Explanation:
**	Pre-6.3/03 versions of ABF use incompatible versions of the 
**	ii_abfdependencies records.  Some exist.  We will convert the ones
**	code indicated that "upgradefe" did not work properly.
**
** System Status:
**	Records will be converted.	
**
** User Action:
**	None required.
*/

S_UI0022_Table	"Cleaning up table %0d of %1d . . ."

/* Note: 0023, 0024 and 0025 are not currently used. */
S_UI0023_UpgradeTitle	"This program cannot run against this database\nbecause
the database is not compatible. Would\nyou like to attempt to upgrade the
database to\nbe compatible with this program?"
S_UI0024_YesExpl	"Attempt to upgrade the database."
S_UI0025_NoExpl		"Do not attempt an upgrade, exit the program."

S_UI0026_UpgradeTitle2
"\n\nThis program cannot run against this database because the database\n
is missing some of the required INGRES catalogs.\n\n"

S_UI0027_UpgradePrompt
"Do you want INGRES to create the missing catalogs (y/n)?"

E_UI0028_INSTALL_NOTDBA "Cannot start program; DBA must create catalogs\n
This program cannot run against this database
because the database is missing some of the
required INGRES catalogs. Only the Database
Administrator (user '%0c') for this database
can add the missing catalogs.\n\n
Contact user '%0c' and have them run the command:\n\n
    upgradefe %1c %2c\n\n"
/*%
** Explanation:
**      The front-end program has just checked if the correct versions
**      of the front-end catalogs it requires are present in this database,
**      and some are missing. Furthermore, this user is not the DBA, and
**      thus is not authorized to add the missing catalogs to the database.
**      Probably this is the first time anyone has tried to run this front-end
**      program on this database.
**
** System Status:
**      The database is untouched. It is as if the front-end program never
**      attempted to connect to this database.
**
** User Action:
**      Contact the database administrator for this database and have them
**      run the command described in the error text.
*/

E_UI0029_UPGRADE_DBA	"Cannot start program; must upgrade catalogs\n
This program cannot run against this database
because some of the INGRES catalogs need to be
upgraded.\n\n
To upgrade the catalogs, run the command:\n\n
    upgradefe %0c %1c\n\n"
/*%
** Explanation:
**      The front-end program has just checked if the correct versions
**      of the front-end catalogs it requires are present in this database,
**      and some are not of the correct version.
**
**      Probably you are trying to run a newer front-end program against an
**      older database, that has a different version of the catalogs this
**      front-end program requires.
**
** System Status:
**      The database is untouched. It is as if the front-end program never
**      attempted to connect to this database.
**
** User Action:
**      Upgrade the front-end catalogs in the database by running the
**      command described in the error text.
*/

E_UI002A_UPGRADE_NOTDBA	"Cannot start program; DBA must upgrade catalogs\n
This program cannot run against this database
because some of the INGRES catalogs need to be
upgraded. Only the Database Administrator (user '%0c')
for this database can upgrade the INGRES catalogs.\n\n
Contact user '%0c' and have them run the command:\n\n
    upgradefe %1c %2c\n\n"
/*%
** Explanation:
**      The front-end program has just checked if the correct versions
**      of the front-end catalogs it requires are present in this database,
**      and some are not of the correct version. Furthermore, you are not
**      the DBA for this database and so you cannot upgrade its front-end
**      catalogs.
**
**      Probably you are trying to run a newer front-end program against an
**      older database, that has a different version of the catalogs this
**      front-end program requires.
**
** System Status:
**      The database is untouched. It is as if the front-end program never
**      attempted to connect to this database.
**
** User Action:
**      Ask the DBA to upgrade the front-end catalogs in the database by
**      running the command described in the error text.
*/

E_UI002B_CANNOT_UPGRADE	"Cannot start program; Incompatible catalogs\n
This program cannot run against this database
because it is incompatible with some of the
database's INGRES catalogs.\n\n
The INGRES catalogs cannot be converted to remedy
this condition. You must
install a newer version of INGRES that is compatible
with the catalogs in database '%0c'. Contact your
Site Technical Contact for information on availability
of a newer INGRES version."
/*%
** Explanation:
**      The front-end program has just checked if the correct versions
**      of the front-end catalogs it requires are present in this database,
**      and some are not of the correct version. Furthermore, the catalogs
**      in this database cannot be converted to make them compatible with
**      this front-end program. Probably you are running an older front-end
**      program against a newer database, which has front-end catalogs
**      that are not compatible with this front-end program.
**
** System Status:
**      The database is untouched. It is as if the front-end program never
**      attempted to connect to this database.
**
** User Action:
**      You must acquire a newer version of this front-end program (one that
**      is compatible with the database you are trying to connect to). Usually
**      this is accomplished by installing a new version of INGRES.
*/

E_UI002C_CHECKDICT_ERR	"Cannot start program; Error checking catalogs\n
This program cannot run against this database. An error occurred
while checking whether the INGRES catalogs in this database (database '%0c')
are compatible with this program (CheckDictVer returned '%1d' when checking
compatibility with Client '%2c', Version '%3d').\n\n
This may be caused by missing files in the directory where INGRES
checks for dictionary information (the 'dictfiles' subdirectory
of the ingres 'files' directory) or by the database server getting an
error while processing the query that selects the catalog compatibility
information. Contact your Site Technical Contact for assistance."
/*%
** Explanation:
**      The front-end program has just tried to check if the correct versions
**      of the front-end catalogs it requires are present in this database.
**
**      During that process, it reads several files in the "dictfiles"
**      subdirectory of the "files" subdirectory of the "ingres" directory.
**      It also issues queries against the "ii_dict_modules" and
**      "ii_client_dep_mod" catalogs in your database.
**
**      An unexpected error occurred while reading these files or while
**      querying these tables in your database.
**
** System Status:
**      The database is untouched. It is as if the front-end program never
**      attempted to connect to this database.
**
** User Action:
**      First try to run "upgradefe -b" on this database. If that fails, then
**      check:
**              - That the catalogs "ii_dict_modules" and "ii_client_dep_mod"
**              both exist in the database.
**
**              - For the existence of the subdirectories of the "ingres"
**              directory mentioned above. Make sure you have read access
**              to that directory.
**
**              - Is the environment variable II_DICTFILES (an unsupported
**              environment variable) set? If set, it should point to
**              a directory that contains ALL the files normally found in the
**              subdirectory of the "ingres" directory mentioned above. Also,
**              you must have read access to that directory.
*/

E_UI0036_NO_SUCH_CLIENT	"Cannot start program; unknown product specified.\n
This program cannot run against this database. The front-end dictionary
product '%0c', version '%1d', specified by this program is not installed
in this database, and could not be found in the installation.\n\n
This may be caused by either a missing *.cli file
in the directory where INGRES checks for dictionary information
(the 'dictfiles' subdirectory
of the ingres 'files' directory),
or possibly you are trying to run a program
whose version differs from the rest of the installation.\n\n
Note that each front-end dictionary product and version
should have a corresponding .cli file in the 'dictfiles' directory
mentioned above; the first line of the .cli file lists
the product and version that the file describes. There is no .cli
file for product '%0c', version '%1d'.\n
Contact your Site Technical Contact for assistance."
/*%
** Explanation:
**	The product and version specified by the front-end program is not
**	installed in this dictionary, and the installation does not contain
**	the files necessary to install it. Something is inconsistent
**	with this installation compared to this front-end program.
**
** System Status:
**	The front-end program will not be allowed to start.
**
** User Action:
**	Check your installation. Particularly the dates, etc, on the
**	front-end program you tried to run, and check that against the
**	*.cli files in the 'dictfiles' subdirectory of the ingres
**	'files' directory.
**
**	The first line of each *.cli file identifies which product and
**	version the file describes. There is not one for the product
**	and version listed in the error message.
*/

E_UI0037_NO_SYSTEM_OWNER "Unable to find owner of INGRES catalogs.\n
Either an error occurred or no rows were found when
trying to select the owner of the INGRES system catalogs
(tried to select table_owner for table '%0c'). This program cannot run
without this information. Exiting . . ."
/*%
** Explanation:
**	The program tried to select iitables.table_owner for the system
**	table described in the message (probably catalog "ii_objects") and
**	either no rows were selected, or an error occurred.
**
**	It should only be possible for this error to happen on an
**	INGRES/Gateway, which has 6.3 or older standard catalogs and
**	NO front-end catalogs. 
**
** System Status:
**	Program cannot run without this information and must exit.
**	The front-end program will not be allowed to start.
**
** User Action:
**	Check that this database has the INGRES system catalog listed
**	in the error message. Contact your INGRES Technical Contact for
**	assistance. If the INGRES front-end system catalogs cannot be
**	created, then the standard catalogs must be upgraded to 6.4 level
**	or higher.
*/

E_UI0038_DUPLICATE_CLIENT "Found duplicate product information.\n
An error occurred while reading the product information in the .cli
files in the 'dictfiles' subdirectory of the ingres
'files' directory. File '%0c' contains information on product '%1c',
version %2d. Information on that product is also specified by another .cli
file in that directory. This installation problem must be corrected before
continuing."
/*%
** Explanation:
**	The program just tried to read all the *.cli files in the
**	ingres 'files', 'dictfiles' directory and found duplicate
**	information in two of the files.
**
** System Status:
**	Program will exit.
**
** User Action:
**	Check your installation. Check the directory mentioned in
**	the error message.
*/

E_UI0039_DUPLICATE_MODULE "Found duplicate module information.\n
An error occurred while reading the module information in the .cre
files in the 'dictfiles' subdirectory of the ingres
'files' directory. File '%0c' contains information on module '%1c',
version %2d. Information on that module is also specified by another .cre
file in that directory. This installation problem must be corrected before
continuing."
/*%
** Explanation:
**	The program just tried to read all the *.cre files in the
**	ingres 'files', 'dictfiles' directory and found duplicate
**	information in two of the files.
**
** System Status:
**	Program will exit.
**
** User Action:
**	Check your installation. Check the directory mentioned in
**	the error message.
*/

E_UI002D_FORMAT_ERROR "Error in dictionary installation file\n
While trying to install or upgrade the front-end dictionary, an
error was detected in the format of the following dictionary installation
file: '%0c', line '%1d'. Contact your Site Technical Contact
for assistance."

E_UI0030_UI_DBMS_ERROR "The above DBMS error occurred while %0c."

F_UI0001_SELECT_DICTMODULES "SELECTing from table ii_dict_modules"

F_UI0002_SELECT_CLIENT_DEPMOD "SELECTing from table ii_client_dep_mod"

F_UI0003_SELECT_CLNT_MOD_JOIN "SELECTing a join of tables ii_dict_modules & ii_client_dep_mod"

/*
** 	The following list of strings is what could be displayed when
**	showing the defaultability of a column. Current values are yes,
**	no, user, value, null, and n/a (for gateways). Each is formatted
**	with enough blanks to be nicely right justified if needed.
*/

F_UI0004_Yes	"   yes  "
F_UI0005_No	"    no  "
F_UI0006_Null	"  null  "
F_UI0007_User	"  user  "
F_UI0008_Value	" value  "
F_UI0009_N_A	"   n/a  "
F_UI000A_Ident  "identity"
F_UI000B_Seq	"sequence"


E_UI0031_MODULE_TABLE "Internal error occurred.\n
An error occurred while attempting to 
initialize an internal module table in routine
'init_moduletable'. This internal error should be reported to your
Site Technical Contact."

E_UI0032_LOWCARD "Unable to read '%0c' files.\n
An error occurred while attempting to read (via 'LOwcard') all the files with
suffix '%0c' in the 'dictfiles' subdirectory of the ingres
'files' directory. Check that files with this suffix exist in that directory
and that you have read access to them. Contact your Site Technical Contact for
assistance."

E_UI0033_LODETAIL "Unable to get information about a file.\n
An error occurred while attempting to obtain (via 'LOdetail' in
routine init_moduletable) information
about one the files whose name begins with '%0c' and whose suffix is '%1c'.
A file like this should exist in the 'dictfiles' subdirectory of the ingres
'files' directory and should be readable by you. Check that files with suffix
'%1c' exist in that directory and that you have read access to them.
Contact your Site Technical Contact for assistance."

E_UI0034_SIFOPEN "Unable to open a file.\n
An error occurred while attempting to open a file with suffix '%0c' in the
'dictfiles' subdirectory of the ingres
'files' directory (error occurred in routine '%1c').
Check that files with suffix
'%0c' exist in that directory and that you have read access to them.
Contact your Site Technical Contact for assistance."

E_UI0035_SIGETREC "Found truncated or empty '%0c' file.\n
One of the files with suffix '%0c' in the
'dictfiles' subdirectory of the ingres
'files' directory is either empty or has rows missing.
The file was opened correctly, but END-OF-FILE was reached
too soon when routine '%1c' attempted to read it.
Check the contents of '%0c' files in this directory and look for an empty
or truncated file.
Contact your Site Technical Contact for assistance."

/* ------------------------------------------------------------------- */
/* Error messages for 4gl sequence_value() function */

E_UI003A_BadSqvNumber "The '%0c' argument must be positive.\n
The '%0c' argument to the sequence_value procedure must be a positive number.
'%1d' was actually passed in.  No new value will be generated."

/*%
** Explanation:
**      Either the increment or start_value were passed in as a negative number
**	 or zero to the sequence_value procedure.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Specify valid values, or let them default to 1.
*/

E_UI003B_NoSuchTable	"Table '%0c' does not exist\n
Table '%0c' was passed to the sequence_value procedure.  No such table exists,
so no new value can be generated for it."
/*%
** Explanation:
**    The specified table does not exist.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**	Create the table, or correct the call to sequence_value.
*/

E_UI003C_Sqv_DBErr	"Database error(s) occurred in sequence_values\n
The previous error messages described errors which occurred in the 
sequence_values procedure while attempting to access the ii_sequence_values 
tables.  No new value will be generated."

/*%
** Explanation:
**      Database errors occurred while trying to access the ii_sequence_values
**	table.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was 
**      generated. 
** 
** User Action:  
**      Determine why the errors occurred, and eliminate them.
*/ 

E_UI003D_NotI4column "Column '%0c' is not a 4-byte integer\n
The column for which the sequence_value procedure generates a new value must be
a 4-byte integer.  Column '%0c' in table '%1c' is not.  The table definition
must be changed before sequence_value will work."

/*%
** Explanation:
**      The sequence_value procedure was called for a column which is not a 
**	4-byte integer.  
**
** System Status:
**      A value of zero will be returned, indicating that no new value was 
**	generated.
**
** User Action:
**      Either change the table definition or remove the call to sequence_value.
*/

E_UI003E_BadMaxGet "Errors occurred reading the current maximum value.\n
There is no record in ii_sequence_values for column '%0c' in table '%1c'.
This makes it necessary to determine the current maximum value for this column.
The previously displayed database errors make this impossible.  No new
value will be generated."

/*% 
** Explanation:
**	Database errors occurred trying to determine the maximum value of the
**	specified column.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was 
**      generated. 
**
** User Action:  
**     Determine the cause of the errors and eliminate them.
*/

/*=============================================================================
** Messages for DBI (database independent) Windows 4GL.
**
** Just reserving message number(s) in the mainline for now.  Details of text
** and parameters will change before the product ships.
*/
E_UI003F_DBI_INSTALL "Cannot start program; must create catalogs\n
This program cannot run against this database because the database 
is missing the required catalogs.  The database owner or administrator
must first run the DBI setup utility on this database."
/*%
** Explanation:
**      The front-end program has just checked if the correct versions
**      of the front-end catalogs it requires are present in this database,
**      and some are missing. 
**      Probably this is the first time anyone has tried to run this front-end
**      program on this database.
**
** System Status:
**      The database is untouched. It is as if the front-end program never
**      attempted to connect to this database.
**
** User Action:
**      Contact the database administrator for this database and have them
**      run the command described in the error text.
*/

E_UI0040_BadCatalaogs
"Cannot connect to database '%0c' because of a problem with the system
catalogs."
/*%
** Explanation:
**      Database catalog problems make it impossible for this program to
**      connect to the specified database.
**
** System Status:
**      The connection will fail.
**
** User Action:
**     Previous messages will indicate how to resolve the problem.
*/

E_UI0041_StarTableErr "Warning: User-defined default values are not supported.\n
User-defined default values are currently not supported in STAR tables. INGRES
default values will be assigned to attributes that are not assigned values by
the user."
 
/*%
** Explanation:
**      Currently, the STAR catalogs are at level 6.2, therefore, they do not
**      contain default attribute values for tables. This error message will
**      be in effect until the STAR catalogs are upgraded to OPING 2.0.
**
** System Status:
**      A warning message is issued to inform the user that the specified
**      defaults will be ignored.
**
** User Action:
**      Hit the ENTER key.
*/

/* Prompt Messages for ROLE id */
 
S_UI0042_RolePrompt "Please enter role :\r\n"
 

