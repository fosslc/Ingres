/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	eric.msg -	Message file for class IC
**
** Description:
**	This file defines the messages for class IC.
**
** History:
**	14-aug-1987 (daver)
**		Created.
**	03-nov-1988 (kathryn) Bug# 3296
**		Added message S_IC000C 'Cannot_Use_location'.
**	18-may-89 (mgw)
**		Added some messages for TERMINATOR:
**		S_IC0062_User_Not_In_Group
**		E_IC0063_No_Such_Group
**		E_IC0024_A_grp_already_exists
**		E_IC002F_A_role_already_exists
**	23-oct-1990 (mgw)
**		Corrected some misspellings.
**	8-dec-1992 (jonb)
**		Messages for ML sort support changes.
**	14-jul-1993 (kellyp)
**		Added listchoice menuitems.
**	20-sep-93 (robf)
**              Added E_IC0071_A_profile_exists and
**		E_IC0070_Profile_not_found
**	        F_IC0039_ProfileType
**	21-jul-94 (robf)
**              Added:
**		E_IC0072_Password_mismatch
**	        E_IC0073_Password_invalid
**		S_IC0074_User_priv_title
**	28-nov-94 (nick)
**		Added E_IC005E_Cannot_remove_super
**      7-dec-1994 (juliet)
**              Modified:
**              E_IC0042_Location_already_exis
**	16-aug-95 (emmag)
**		Illegal location area specified, error message added.
**      16-nov-95 (pchang)
**		Added:
**		E_IC0125_Ingres_Path_Not_Dir
**		E_IC0126_Ingres_Path_No_Write
**		E_IC0127_Ingres_Path_Not_Exist
**		8-may-1996 (angusm)
**		Add:
**		E_IC005F_cant_alter (must have at least 1 WORK location)
**		(bug 75683)
** 	18-jun-1997 (wonst02)
** 		Changed E_IC0127_Ingres_Path_No_Write to 
** 		S_IC0127_Ingres_Path_No_Write; it is no longer a fatal error.
**      20-Jul-1998 (wanya01)
**              Added:
**              E_IC0128_Ingres_Path_Not_SubD             
**	02-jul-1999 (hanch04)
**		Added:
**		E_IC0160_Ingres_Path_Cant_Crea
**		E_IC0161_Invalid_Usage
**		E_IC0162_Loc_Work_And_Work_Aux
**		E_IC0163_Cant_Extend_Exist_DB
**		E_IC0164_Alter_Flag_Needed
**		E_IC0165_Alter_With_DBname
**		E_IC0166_bad_user_id
**		E_IC0167_delete_nonexist_loc
**	17-mar-1999 (mcgem01)
**		Replace references to the now obsolete I&O Guide.
**	11-May-2001 (jenjo02)
**	        Changed E_IC0168, added E_IC0129_Area_Access_Denied.
**	23-May-2001 (jenjo02)
**	        Changed E_IC0125 text, fixed E_IC0167 to include
**	        "alter" as well as "delete", fixed text of E_IC0168,
**	        added E_IC0171_Cant_Make_Raw_Area.
**	15-Oct-2001 (jenjo02)
**	        Area directories now made in server during CREATE/ALTER
**	        LOCATION. Removed unneeded messages related to that.
**	27-Aug-2002 (inifa01) Sir 108589
**		Added new extenddb error message E_IC0171_Invalid_Usage 
** 29-Apr-2004 (gorvi01)
**		Added new unextenddb error message E_IC0172_Invalid_Usage 
**	11-Nov-2004 (gorvi01)
**		Added new error message of E_IC004C for unextenddb utility. 
**	28-Dec-2004 (jenjo02)
**		Tweaked E_IC004C text.
**      17-Jun-2010 (coomi01) b123913
**              Add new error message E_IC0052 for unextenddb utility.
**/

# define	_IC_CLASS	83
/*
**	accessdb.c
*/
/*
**	Keyword displayed in table fields of database names.
*/
F_IC0027_AllDatabases		"all databases"
/*
**	addlmenu.qc
*/
S_IC0004_Add_Locations_Form	"Add Locations Form"
S_IC0114_Edit_Locations_Form	"Edit Locations Form"
S_IC0115_Exam_Locations_Form	"Location Information Form"
S_IC0008_Location_Appended	"Location Appended."
S_IC0072_Location_Updated	"Location Updated."
S_IC000C_Cannot_use_location	
"\"No\" has been specified for all\nusage options; therefore,
the\nlocation cannot be used.  Do you\nwant to save it anyway?"

S_IC0003_save_anyway		"Save anyway"
S_IC0005_dont_save		"Do not save"

E_IC0120_Loc_Already_Exists	"A location with that name already exists."

/*
**	exdbmain.sc
*/

E_IC0161_Invalid_Usage		"The usage '%0c' is invalid."
E_IC0162_Loc_Work_And_Work_Aux	"Work and Auxiliary work cannot both be specified for a location."
E_IC0163_Cant_Extend_Exist_DB	"An existing database cannot be extended to new checkpoint, journal, or dump locations."
E_IC0164_Alter_Flag_Needed	"The -alter flag must be passed to modify the usage of a location."
E_IC0165_Alter_With_DBname	"The -alter flag cannot be used with a database."
E_IC0166_bad_user_id		"Incorrect user: %0c\nYou must run this utility as: '%1c'."
E_IC0167_Nonexist_Loc		"Cannot alter or delete location.\nThe location '%0c' does not exist."
E_IC0168_Bad_Rawpct		"Rawpct must be between 1 and 100, not %0d."
E_IC0170_Raw_Extend		"A raw location can only be extended to one database."
E_IC0171_Invalid_Usage		"Location already exists - cannot specify '-aarea_dir' or '-rraw_pct'."
/*
**	catalogdb.c
*/
/*
**	credbloc.qc
*/

E_IC000E_Full_pathname_too_big	"Pathname too long.\n
Full pathname '%0c'\n
for location '%1c' exceeds max length of %2d\n"
S_IC000F_Starting_Target_DBMS	"Starting up Target Database . . ."
E_IC0010_Error_opening_target	"Error opening database '%0c' for extending"
E_IC0014_define_loc_error	"Error(s) executing the DEFINE LOCATION command"

/*
**	ctrl.qc
*/
S_IC0015_INGRES_CONTROL_PROGRA	"ACCESSDB"
/*
**	Menu item "Databases"
*/
F_IC0001_Databases	"Databases"
F_IC0002_Acc_Database_Catalog	"ACCESSDB - Database Catalog"
F_IC0003_Cat_Database_Catalog	"CATALOGDB - Database Catalog"

S_IC0080_Accessdb_Db_Info	"ACCESSDB - Database Information"
S_IC0081_Catalogdb_Db_Info	"CATALOGDB - Database Information"
/*
**	Menu item "LocationNames"
*/
F_IC0005_Locations	"Locations"
/*
**	Menu item "Users"
*/
F_IC0006_Users	"Users"

/*
**      Menu item "Alter"
*/
F_IC0026_Alter  "Alter"
 

/*
**	Menu item Extend
*/
F_IC0007_ExtendDB	"Extend"
/*
**	Menu item "LocationName"
*/
S_IC0084_Accessdb_Loc_Info	"ACCESSDB - Location Information"
S_IC0085_Catalogdb_Loc_Info	"CATALOGDB - Location Information"
/*
**	Menu item "User"
*/
F_IC0009_User	"User"
F_IC0082_Accessdb_User_Info	"ACCESSDB - User Information"
F_IC0083_Catalogdb_User_Info "CATALOGDB - User Information"
F_IC0111_Accessdb_Create_User	"ACCESSDB - Create a User"
S_IC001B_Manager_Menu_Form	"Manager Menu Form"
F_IC000A_CATALOGDB_title	"CATALOGDB"
/*
**	DbExtensions menu item
*/
F_IC000B_DbExtensions	"Extensions"
S_IC001C_User_Menu_Form	"CATALOGDB Database Catalog Form"
/*
**	Menu item "AccessGrants"
*/
F_IC0024_AccessGrants		"Access"
F_IC0025_Installation		"Installation"
S_IC007A_AccessGrantsHelp	"Installation-Wide Access Grants & Databases With Public Access"
S_IC0079_UpdateInstAccGrnts	"Updating Installation-Wide Access Grants . . ."
/*
**	accgrnts.qsc
*/
S_IC0074_InstallationAccess	"Installation-Wide Access Grants"
S_IC0075_DBPublicAccess		"Databases With Public Access"
S_IC0076_Retrieving_InsAcc	"Retrieving Installation-Wide Access Grants . . ."
S_IC0077_Retrieving_DBPubAcc	"Retrieving Databases with Public Access . . ."
S_IC0078_UpdatePubAccGrnts	"Updating Public Access Grants . . ."
/*
**	db.qsc
*/
S_IC001D_db_form	"Database Information Form"
S_IC011D_dbacc_form	"Database Access Form"
S_IC001E_Retrieving_db_info	"Retrieving database information . . ."
E_IC0001_Cannot_use_DBA	"It does not make sense to give/revoke access to/from the DBA."
E_IC0020_Database_doesnt_exist	"Database '%0c' does not exist."
E_IC006E_AuthId_DoesntExist	"Authorization Identifier does not exist.\n
There is no User, Group or Role named '%0c'."

E_IC0068_Retrieve_Error		"Unable to retrieve info on database '%0c'\n
An error occurred while trying to retrieve information about database
'%0c'. The previous error message should have described the problem.
Contact your Site Technical Contact for assistance."

E_IC006F_Retrieve_AuthError	"Unable to retrieve info on Authorization Id\n
An error occurred while trying to retrieve information about Authorization
Identifier '%0c'. The previous error message should have described the problem.
Contact your Site Technical Contact for assistance."

S_IC0102_Acc_Dbaccess_Title	"ACCESSDB - Database Access"
S_IC0103_Cat_Dbaccess_Title	"CATALOGDB - Database Access"

E_IC006A_BadArea "Location/Area does not exist; unable to create.\n
Location '%0c' corresponds to area '%1c',
but this area does not exist. Accessdb was not able to create
this area and give it the permissions needed for a database area.
Database '%2c' cannot be extended to location '%0c' until the problem
with area '%1c' is fixed. Check your Ingres Database Administrators Guide 
for information on creating Locations and Areas."

S_IC0023_Updating_database	"Updating database . . ."

E_IC0064_Err_getting_CDBinfo	"Error selecting coordinator database info\n
An error occurred while selecting information
about the coordinator database for distributed database '%0c'. The error
occurred while attempting to select from the 'iistar_cdbs' catalog."

E_IC0065_No_CDB_info_found	"No CDB info found for Distributed Database\n
No coordinator information found for
distributed database. There is an integrity problem in your 'iidbdb' system
catalogs: this distributed database does not have a corresponding row in the
'iistar_cdbs' catalog to describe its Coordinator Database information."

E_IC0067_BadIngresVersion	"Can not run against this INGRES version\n
This version of Accessdb/Catalogdb can only run against version
'%0c' INGRES iidbdb databases. Accessdb/Catalog just attempted to connect with a
version '%1c' INGRES iidbdb database. This indicates an installation
problem: the copy of Accessdb/Catalogdb you are attempting to run is
incompatible with the databases in this installation.
Please notify your Site Technical Contact."

E_IC0130_Yes_or_No	"Invalid value for column. \n
Valid values are 'y', 'yes', 'n', or 'no'."

/*
**	dbtable.qsc
*/
S_IC0025_Database_table	"Database Catalog"
S_IC0026_Retrieving_db_names	"Retrieving database names . . ."
/*
** Following are database types, that appear in database catalog table field.
*/
F_IC0010_Distributed	"distributed"
F_IC0011_Local		"local"
/*
** following is only accessed in db.qsc, since CDBs are suppressed 
** from the database catalog in dbtable.qsc. This is another Database type,
** like the above 2.
*/
F_IC0012_Star_CDB	"Star-CDB"

/*
**	util.c
*/
/*
** following two strings used to fill in table field in form DBTBLFRM and
** field in form DBFRM. ALSO: these are used to map what user enters in
** field "access" on DBFRM into whether to set the database to public
** or private in iidatabase.access. I'm allowing 24 bytes for each of these
** in the program (internal buffer).
*/
F_IC000C_public "public"
F_IC000D_private "private"

S_IC0090_select_default_access
"Select the default access for the '%s' database:"
S_IC0091_pub_exp "Available to all but selected users."
S_IC0092_priv_exp "Access denied to all but selected users."
S_IC0093_defacc_help_title "Choosing the Default Access"

S_IC009A_defacc_list "public;Available to all but selected users.\n
private;Access denied to all but selected users."
 
/*
**	futil.qc
*/
E_IC002D_bad_username	"'%0c' is an invalid username or a reserved word.\n
Legal characters are alphanumerics, '@', '#', '$', and '_'. 
The first character must be alphabetic.  Maximum length is %1d characters."

E_IC002E_bad_name	"'%0c' is not a valid name.\n
Name must begin with an alphanumeric character or '_'. The remaining characters
of the name must be alphanumeric and/or the characters '@', '#', '$' or '_'.
It can be at most %1d characters long."
/*
**	ingcntrl.qc
*/
E_IC0030_Cannot_open_Dump_file	"Cannot open -D file %0c.\n"
E_IC0031_Cannot_do_Z_test	"Cannot do -Z test on %0c.\n"
E_IC0032_Bad_flag	"Bad flag: %0c\n"
E_IC0033_Bad_argument	"Bad argument: %0c\n"
S_IC0034_accessdb_catalogdb	"accessdb/catalogdb"
E_IC0035_Cannot_start_INGRES	"Cannot start up INGRES.\n"
E_IC0036_no_authority	"You do not have proper authority or privilege to run Accessdb.\n"
E_IC0060_Not_INGRES_DBMS	"INGRES database required\n
This program may only be run on an INGRES database. The 'iidbcapabilities' catalog in your 'iidbdb' database does not contain a row indicating that it is an INGRES database. Exiting..."

E_IC007B_Insufficient_Privs   "No permission to run Accessdb.\n
You (user '%0c') do not have sufficient user permissions to run Accessdb.
Try running Catalogdb."

E_IC006C_NotOnTableField	"Your cursor must be on a table field.\n
This menuitem is only active when your cursor is in a table field."

E_IC006D_NotOnAccessTableField	"Cursor must be on table field.\n
This menuitem is only active when your cursor is in the 'Authorized
Databases' table field."

S_IC0038_Accessing_Databases	"Accessing Databases . . ."
S_IC0039_Exiting_Accessdb	"Exiting Accessdb . . ."
S_IC003A_Exiting_Catalogdb	"Exiting Catalogdb . . ."
S_IC003B_Usage_statement	"Usage: %0c [-m] [-uname] [-vnode=vnode]\n
Usage: %0c [-m] [-uname], where -m means run as INGRES manager, -uname
means run as the specified user name, and vnode specifies a remote
connection."

E_IC0073_GetPrivsError		"Error checking user permissions.\n
Accessdb/Catalogdb cannot start because an error occurred checking
your user permissions. The preceding message should describe the error
that occurred."

F_IC0090_Listpick_Title		"Select a %0c:"
F_IC0091_Location		"Location"
E_IC0110_ListPick_Empty		"No %0cs are in the database.\n
You have requested a list of %0cs available to you in the database.
There are none at this time."
E_IC0111_Format_Username_Error	"Unable to normalize user name '%c'"

S_IC0121_ListPick	"ListChoices"

/*
**	loclist.qc
*/
S_IC003D_Location_Listing	"Location Listing"
E_IC0011_cant_delete_loc
"Cannot delete location.\nThe location '%0c' is in use by one or more
databases; it cannot be deleted."

/*
**	locmenu.qc
*/
S_IC003E_Extenddb_Form	"Database Extensions Form"
S_IC003F_Looking_up_Locations	"Looking up Location Names . . ."

S_IC0041_Validating_DB_extens	"Validating Database Extensions . . ."
E_IC0042_Location_already_exis
"Location '%0c' with Usage '%2c' already exists for Database '%1c'."
S_IC0043_Attempting_Re_Extend	"Attempting to Re-Extend the Database . . ."
S_IC0044_Creating_the_New_Exte	"Creating the New Extension(s) . . ."
E_IC0045_Error_retrieving_area	"Error(s) retrieving area data for
location '%0c'"
E_IC0046_Errors_extending_db	
"Error(s) occurred extending database '%0c' to location '%1c'."
E_IC003C_cant_alter            
"Error(s) occurred altering the extension usage."
E_IC0047_Missing_data	"Missing data in table field row.
You must enter both a Database Name and New Location Name in each
row before you Save."
E_IC0048_db_not_belong_to_dba	"Invalid database name.\n
Data base either does not exist or is not owned by this DBA"
E_IC0049_Location_doesnt_exist	"Location name '%0c' does not exist.\n
You must create a new location before you can extend databases to it.
The location '%0c' does not exist."
E_IC004A_Location_is_not_avail	"Location '%0c' not available for databases.\n
Databases and database tables may not be created on location '%0c'."
E_IC0154_Usage_required	"Missing Usage for a Location.
You must enter a Usage type for each Location before you Save."
E_IC004C_Errors_unextending_db	
"Error(s) occurred unextending database '%0c' from location '%1c'."
E_IC0052_Error_last_db_workloc
"Must have at least one \"Work\" location for database '%0c',\n
unextenddb will not proceed on any request(s) made."
S_IC005A_select_extension_type
"Select which type of extension will apply to\n
the '%s' database:"
 
S_IC005B_ext_data       "For use by data tables"
S_IC005C_ext_work       "For use by sort and modify operations"
 
F_IC0030_ext_data       "Database"
F_IC0032_ext_work_aux   "Auxiliary work"
F_IC0033_ext_work       "Work"
E_IC005D_cant_alter	
"\"Alter\" may only be used to change a location's usage from \"Work\"
to \"Auxiliary work\", or vice versa."
E_IC005F_cant_alter	
"Must have at least one \"Work\" location."
 
S_IC0094_Loc_Info_Title		"CATALOGDB - Information About a Location"
S_IC0095_Loc_Info_Title		"ACCESSDB - Information About a Location"
S_IC0096_Loc_Create_Title	"ACCESSDB - Create a Location"
S_IC0097_Loc_Edit_Title		"ACCESSDB - Edit a Location"

S_IC0100_Acc_Extend_Title	"ACCESSDB - Database Extensions"
S_IC0101_Cat_Extend_Title	"CATALOGDB - Database Extensions"
/*
**	user.qc
*/
S_IC004B_User_form	"User form"
S_IC004D_Working_dot_dot_dot	"Working . . ."
E_IC004E_A_user_already_exists	"A user already exists with that name."
E_IC004F_Cannot_add_user	"Cannot add user '%0c'."
E_IC0051_cant_change_ingres	"User '$ingres' cannot be changed"
E_IC006B_CannotDeleteUser	"User '%0c' cannot be deleted."
E_IC0055_User_is_a_DBA_no_del	"User is a DBA.  Delete not performed."
E_IC0056_User_does_not_exist	"User does not exist; cannot delete"
E_IC005E_Cannot_remove_super	"Cannot remove super user status from '%0c'"
F_IC0013_ListUsers		"ListUsers"
F_IC0014_ListGroups		"ListGroups"
F_IC0015_ListRoles		"ListRoles"
F_IC0016_RowDelete		"RowDelete"

/* following messages are menuitem explanations */
F_IC0018_RowDeleteExpl		"Delete row from table field"
F_IC002A_AlterExpl              "Alter the usage of a work location"

/* following are used as the object type names in popups you see if try to end
** after making changes, but without saving.
*/
F_IC0019_User			"user"
F_IC001A_Database		"database"
F_IC0021_Location		"location"
F_IC0022_InsAccGrants		"Installation-Wide Access Grants"
F_IC0023_DBPublicAcc		"Databases With Public Access"

/*
** following are the "Authorization Identifier" types on the
** Information About an INGRES User table field.
*/
F_IC001B_UserType		"User"
F_IC001C_GroupType		"Group"
F_IC001D_RoleType		"Role"
F_IC001E_PublicType		"Public"
F_IC001F_UnknownType		"??????"
F_IC0020_AccessType		"Default access type"
F_IC0039_ProfileType            "Profile"



/*
**	usertbl.qc
*/
S_IC0057_User_table	"User Catalog Form"
S_IC0058_Retrieving_user_names	"Retrieving user names . . ."
S_IC0059_Get_Form_Error	"An Error occurred while Initializing Forms.\n
This is a Fatal Error, as the forms necessary for the execution of 
this program cannot be initialized."
E_IC0061_Other_startup_err	"Error starting INGRES.\n
Error '%0x' occurred when Accessdb/Catalogdb tried to connect
to the database. The preceding error messages should explain
what type of error occurred. If not, then report this problem to
your Site Technical Contact."

S_IC0062_Writing_Script	"Writing SQL commands to file"
E_IC0066_Script_Fail	"Can't write SQL commands to file '%0c'"
E_IC0069_Script_Success	"SQL 'CREATE USER' commands written\n
to file '%0c'"

F_IC0070_User_Script	"SQLscript"

/*
**	Afterthoughts
*/

/*%
** Parameters:
**	1st %s	User name to be updated in system catalogs
**	2nd %s	Group name to be updated in system catalogs
**	3rd %s	User name to be updated in system catalogs
**	4th %s	Group name to be updated in system catalogs
**
** Explanation:
**	This string is to be used as input to an STprintf call which will
**	make the proper substitutions to produce a string suitable for input
**	to a prompt popup. Hence the %s's. User has specified a default group
**	to which (s)he is not a member.
**
** System Status:
**	The user will be added as a member of the named group or the update
**	will be canceled, depending on the user's response.
**
** User Action:
**	Answer yes or no.
*/

E_IC0063_No_Such_Group	"Group '%0c' does not exist\n
Default group must have been created with an SQL 'CREATE GROUP' command."

/*%
** Parameters:
**	%0	Group name user is trying to make the default group.
**
** Explanation:
**	User has specified a default group which does not exist.
**
** System Status:
**	No update will be performed. Frame is resumed.
**
** User Action:
**	Specify a proper default group or none.
*/

E_IC0024_A_grp_already_exists	"A group with that name already exists.\n
You cannot create a user with the same name as an existing group."

/*%
** Parameters:
**	None
**
** Explanation:
**	User has specified a user name which is identical to an already
**	existing group name. User name - group name collisions are not allowed.
**
** System Status:
**	Control reverts back to the main ctrlmenu frame.
**
** User Action:
**	Select User frame again and specify a different user name.
*/

E_IC002F_A_role_already_exists	"A role already exists with that name.\n
User name/role name collisions are not allowed."

/*%
** Parameters:
**	None
**
** Explanation:
**	User has specified a user name which is identical to an already
**	existing role name. User name - role name collisions are not allowed.
**
** System Status:
**	Control reverts back to the main ctrlmenu frame.
**
** User Action:
**	Select User frame again and specify a different user name.
*/

E_IC0070_Profile_not_found	"Profile not found.\n
No profile with this name was found."
/*%
** Parameters:
**	None
**
** Explanation:
**	User has specified a profile name which could not be found.
**
** System Status:
**	Control reverts back to the main ctrlmenu frame.
**
** User Action:
**	Select User frame again and specify a different profile name.
*/

E_IC0071_A_profile_exists	"A profile already exists with that name.\n
User name/profile name collisions are not allowed."

S_IC0150_User_priv_title "Define Privileges for User"
S_IC0151_User_pwd_title "Change Password for User"
/*%
** Parameters:
**	None
**
** Explanation:
**	User has specified a user name which is identical to an already
**	existing profile name. User name - profile name collisions are not 
**	allowed.
**
** System Status:
**	Control reverts back to the main ctrlmenu frame.
**
** User Action:
**	Select User frame again and specify a different user name.
*/

E_IC0152_Password_mismatch "The passwords do not match."

/*%
** Parameters:
**	None
**
** Explanation:
**	The passwords entered by the user do not match. This is probably
**      due to a typing error in entering one or both of the passwords.
**
** System Status:
**	Password update is aborted.
**
** User Action:
**	Reenter the passwords and ensure they match.
*/

E_IC0153_Password_invalid "The specified password is invalid."
/*%
** Parameters:
**	None
**
** Explanation:
**	The password entered by the user is invalid, for example  it
**	contains an illegal character or is an illegal length.
**
** System Status:
**	Password update is aborted.
**
** User Action:
**	Enter a legal password.
*/


/*
**      extenddb.c
*/
F_IC0071_EXTENDDB
"EXTENDDB"

/*
**      unextenddb.c
*/
F_IC0072_UNEXTENDDB
"UNEXTENDDB"


