/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ermi.msg -	MISC Facility Message Definitions File.
**
** Description:
**	Defines the messages for the misc facility, class MI
**
** History:
**	05/02/90 (rdesmond)
**		first created
**	9/18/90 (pete)	Improve message text for DD0005 & DD0006.
**	7/30/91 (pete)	Add message E_DD0027_NOT_DBA and Change
**			"client" --> "product". That's how its documented
**      07-nov-2001 (stial01)
**          Added E_DD0028_BAD_PAGESIZE
**/

# define	_DD_CLASS	205
	
F_DD0000_INCOMPAT_PROMPT "Proceed with the installation (y/n):"

/*%
** Explanation:
**	This prompt is displayed when a dictionary upgrade will desupport
**	one or more dictionary products.
**
** System Status:
**	If the user confirms, the dictionary installation for the current
**	module will continue, otherwise it will terminate installation
**	will continue for the next user-specified product, if any.
**
** User Action:
**	Confirm or deny.
*/

/* following 3 messages have been removed from source code */
S_DD0001_CONNECTING_TO_DB "Connecting to database '%0c'..."

E_DD0002_CANT_CONNECT_TO_DB "Cannot connect to database '%0c'.\n
%1c was unable to connect to database '%0c'."

S_DD0003_CONNECTED_TO_DB "Connection to database '%0c' completed."

E_DD0004_ONLY_DBA_CAN_RUN "Only the DBA may run %0c."

E_DD0005_CANT_INIT_MODTABLE "Cannot initialize module table.\n
An error occurred while trying to read and process the *.cre files
in the 'dictfiles' subdirectory of the Ingres 'files' directory
(the *.cre files contain information about each dictionary module
and the SQL statements used to create each module).\n
Check that this directory exists and that it contains files
with a file extension 'cre'. Contact your Site Technical Contact
for assistance."

E_DD0006_CANT_INIT_CLITABLE  "Cannot initialize product table.\n
An error occurred while trying to read and process the *.cli files
in the 'dictfiles' subdirectory of the Ingres 'files' directory
(the *.cli files contain information about each dictionary product and
version, and which dictionary modules each product is composed of).\n
Check that this directory exists and that it contains files
with a file extension 'cli'. Contact your Site Technical Contact
for assistance."

E_DD0007_NO_CLIENT "There is no front-end product with the name '%0c'."

/*%
** Explanation:
**	There is no *.cli (i.e. the file suffix is "cli") file in the
**	'dictfiles' subdirectory of the ingres files directory for a
**	dictionary product with this name (the .cli files describe
**	each product and which dictionary modules it requires).
**	The first line of each .cli file gives the name of the dictionary
**	product described in the file.
**
** System Status:
**	Upgradefe will continue and process any other product names
**	on the command line
**
** User Action:
**	Make sure you have spelled the product name correctly on
**	the command line and if so, make sure that you have a
**	complete set of dictionary files in the directory mentioned
**	above in the Explanation. If you are missing any files, then
**	reload a fresh set from the Ingres distribution tape.
*/

E_DD001E_NO_CLIENT_VERS
  "There is no front-end product named '%0c', version %1d."

/*%
** Explanation:
**	There is no *.cli (i.e. the file suffix is "cli") file in the
**	'dictfiles' subdirectory of the ingres files directory for a
**	dictionary product with this name and version number (the .cli
**	files describe each product and which dictionary modules it requires).
**	The first line of each .cli file gives the name and version
**	number of the dictionary product described in the file.
**
** System Status:
**	Upgradefe will continue and process any other product names
**	on the command line
**
** User Action:
**	Make sure you have spelled the product name correctly on
**	the command line and if so, make sure that you have a
**	complete set of dictionary files in the directory mentioned
**	above in the Explanation. If you are missing any files, then
**	reload a fresh set from the Ingres distribution tape.
*/

E_DD0008_CLI_UNINSTALLABLE "Product %0c version %1d\nhas been made uninstallable by an incompatible dictionary upgrade."
E_DD0009_CANT_INSTALL_CLI "Cannot install product %0c version %1d."

S_DD000A_INSTALLING_CLI "  Installing product %0c version %1d."
S_DD000B_CLI_INSTALLED "  Product %0c version %1d installed."
S_DD000C_CLI_ALRDY_INSTALLED "  Product %0c version %1d is already installed."

E_DD000D_CANT_FIND_MOD_DEF "Cannot find module definition for %0c version %1d."
E_DD000E_CANT_CREATE_MOD "Cannot create module %0c version %1d.\n
An error occurred either CREATEing or MODIFYing the catalogs required for this
front-end dictionary module."
E_DD000F_CANT_REGISTER_MOD "Cannot register module %0c version %1d."
E_DD0010_CANT_MODIFY_MOD "Cannot modify module %0c version %1d."
E_DD0011_CANT_UPGRADE_MOD "Cannot upgrade to module %0c version %1d."

E_DD0026_CANT_UPGRADE_MOD2 "Cannot upgrade to module %0c version %1d.\n
Error %2d occurred trying to upgrade to this front-end catalog module.
Check if this error number is an operating system error. If not, then it
is an internal Ingres error. Contact your site technical contact for
assistance."

/*%
** Explanation:
**	An error occurred trying to upgrade a front-end dictionary module.
**	The error number displayed could be from the operating system, or
**	it could be an internal Ingres error number.
**
** System Status:
**	Upgradefe exits with an error. Front-end catalogs are not upgraded.
**
** User Action:
**	Check to see if the error number is an operating system error.
*/

S_DD0012_REQUIRES_UPGRADE "Module upgrade incompatible with -c flag.\n
Installation of module %0c version %1d\n
for product %2c version %3d\n
would require a module upgrade and module upgrades cannot be done\n
when the -c flag is specified. Installation of product %2c terminated.\n
If you REALLY want to install product %2c version %3d,\n
then don't use the '-c' flag."

/* the following 4 messages are informatory and must line up with those put
** out by Createdb and Sysmod.
*/
S_DD0013_MOD_ALRDY_INSTALLED "  (module %0c version %1d is already installed)"
S_DD0014_CREATING_MOD "  Creating front-end module %0c version %1d . . ."
S_DD0015_UPGRADING_MOD "  Upgrading to front-end module %0c version %1d . . ."
S_DD0016_MODIFYING_MOD "  Modifying front-end module %0c version %1d . . ."

S_DD0017_CLI_DESUPPORTED
"\nWarning: If product %0c, version %1d is installed, then the\n
following product(s) must be desupported (and any front-end\n
product that requires the desupported product will no longer run):\n"

S_DD0018_MOD_OR_CLI_NAME "\t%0c version %1d\n"

E_DD0019_DBMS_ERROR "DBMS error\n%0c\n"

E_DD001F_DBMS_ERROR2 "DBMS error:\n%0c\n occurred on the
SQL statement:\n\n%1c\n"

E_DD0020_NO_UPGRADE_FILE "Could not find module upgrade program.\n
In order to upgrade front-end dictionary module %0c to version %1d,
an executable program must be called.
This executable program should be in the ingres 'bin' directory
and its file name should begin with '%2cu'. This program was not found.
Check your installation and file permissions for files in the ingres
'bin' directory. Contact your Site Technical Contact for assistance."

E_DD0025_NO_UPGRADE_FILE "Could not find module upgrade program.\n
In order to upgrade front-end dictionary module %0c to version %1d,
an executable program must be called. The program
that could not be found is: '%2c'.
Check file permissions for files in the ingres
'bin' directory. Contact your Site Technical Contact for assistance.
(%3c)"

/*%
** Explanation:
**	Upgradefe was not able to find a required upgrade executable in the
**	Ingres Bin directory.
**
** System Status:
**	The upgrade is not performed.
**
** User Action:
**	Check file permissions, and check if the file listed in the
**	message exists.
*/

E_DD0027_NOT_DBA	"Only the DBA can %0c a database.\n
You, '%1c', cannot %0c a database whose DBA is '%2c'."

/*%
** Explanation:
**	Only the DBA for a database can upgrade or modify the database.
**	The system administrator may become the DBA by specifying the -u
**	flag.
**
** System Status:
**	The upgrade is not performed.
**
** User Action:
**	Log in as the DBA for the database you wish to upgrade or modify,
**	or contact the INGRES System Administrator and have them run
**	upgradefe or modifyfe for you.
*/

E_DD0028_BAD_PAGESIZE	"The page size '%0d' is invalid or the dbms is not configured for this page size."

E_DD001A_II_DICTFILES_NOT_DIR "The variable 'II_DICTFILES' does not contain the name of a directory."

E_DD001B_NO_DB_PRIV "Not authorized\n
You are not authorized to run program '%0c' on database '%1c'.
Contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This user is not the DBA for the database, and is also not
**	an Ingres super-user (this user is not allowed to use the -u flag to
**	impersonate other users). Only a database's DBA and Ingres super-users
**	are allowed to run this program on a database.
**
** System Status:
**	The catalogs in the database have not been affected. It is as if
**	this program was never run.
**
** User Action:
**	Contact the DBA for the database or an Ingres super-user and have
**	them run this program for you.
*/

E_DD001C_NO_CLIENTS_SPEC "No products have been specified.\n"
S_DD0021_NO_CLIENTS_PROMPT "Product?"

E_DD0022_TOO_MANY_STAR_STMTS	"Star: Too many CREATE, DROP, MODIFY stmts\n
Too many CREATE TABLE, DROP TABLE, and MODIFY statements were encountered for
a dictionary module (for Star, the maximum allowed for one module is %1d).
Unable to REGISTER (or REMOVE) table '%0c'. There is a problem in the .cre
and .mfy files for one module (these files are located in the 'dictfiles'
subdirectory of the ingres files directory): they contain too many statements.
Report this problem to your Site Technical Contact."

E_DD0024_CantModifyStar	"Unable to modify tables in distributed database\n
An error occurred while trying to MODIFY and then REGISTER tables in a
distributed database.
A preceeding error message should have given details about the error.
The MODIFY did not complete correctly."

E_DD0023_Cant_Run_On_Star_CDB	"Cannot run on Star Coordinator Database\n
Database '%0c' is an Ingres/Star Coordinator Database and '%1c' cannot
be run directly on a Star Coordinator Database.  The Ingres catalogs will
not be changed.  You must run '%1c' again, but specify database_name/star,
where 'database_name' is the Distributed Database with which '%0c' is
associated."

E_DD001D_CANT_GET_DB_TYPE "Cannot determine database or gateway type."

/*
**	FAST messages
*/
F_DD0001_DICTINSTALL	"DICTINSTALL"

F_DD0002_DICTMOD	"DICTMODE"

F_DD0003_TRY_AS_DBA "Would you like to attempt to execute as the DBA (y/n)[n]?"

/*
**	following are used as first argument to message E_DD0027_NOT_DBA.
*/
F_DD0004_UPGRADE	"upgrade"
F_DD0005_MODIFY		"modify"
