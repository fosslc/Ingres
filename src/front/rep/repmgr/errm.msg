/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	errm.msg - message file for Replicator Manager
**
** Description:
**	This file defines the messages for the Replicator Manager.
**
** History:
**	26-sep-1996 (joea)
**		Created.
**	14-jan-97 (joea)
**		Fix gap in fast message numbers.  Add target type descriptions.
**	30-apr-97 (joea)
**		Change CA-Ingres to OpenIngres.
**	14-nov-97 (joea)
**		Add No_valid_choices.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	04-may-98 (joea) bug 90732
**		Change E_RM0008 to allow initial digits on vnode names.
**	01-oct-98 (abbjo03)
**		Add messages to replace ddba_messageit.
**	19-oct-98 (abbjo03)
**		Add E_RM0122_Catalogs_exist.
**	30-nov-98 (abbjo03)
**		Modify text of E_RM0028 to allow zero as a server number.
**	30-apr-99 (abbjo03)
**		Change Dist_queue_empty message to an error.
**	05-Apr-2004 (gupsh01)
**		Added error E_RM0135_Err_alloc_ucode_key for memory 
**		allocation failure during creating keys for unicode
**		columns. 
**/

# define	_RM_CLASS	232

/*
**	repmgr.qsc
*/

E_RM0001_Unsupp_Ingres_version	"Unsupported Ingres version.\n
Ingres Replicator will only run on Ingres version 6.4/04 or later
or OpenIngres 1.1/04 or later."

E_RM0002_Must_be_DBA		"You must be the DBA to run this program.\n
In order to run this you need to connect as the DBA of the target database."
/*%
** Explanation:
**	Ingres Replicator Manager and most other Replicator utilities
**	can only be executed by the DBA of a target database, or by someone
**	connected as the DBA.
**
** System Status:
**	The program will not run.
**
** User Action:
**	Invoke the program as the DBA.
*/

E_RM0003_No_rep_catalogs	"Missing or outdated Replicator catalogs.\n
Unable to find Replicator catalogs in the database or the catalogs have
not been upgraded.  Please run 'repcat' to install the catalogs or 'convrep'
to upgrade them."
/*%
** Explanation:
**	Replicator did not find the catalogs required by it.
**
** System Status:
**	Replicator will not run.
**
** User Action:
**	If the Replicator catalogs are missing from the database, run the
**	'repcat' utility to create them.  If the catalogs are present but
**	are from an older version, run the 'convrep' utility to upgrade
**	them.
*/

E_RM0004_Error_get_local_db	"Error getting local database number.\n
A database error(s) occurred while selecting from 'dd_databases' to
determine the local database number.  The previous message should have
given details about what went wrong."
/*%
** Explanation:
**	A database error occurred.
**
** System Status:
**	Same status as shown in the previous error.
**
** User Action:
**	Take action from previous error.
*/

E_RM0005_Too_many_local_db	"More than one local database.\n
There is more than one row in 'dd_databases' marked as the local database.
This is a configuration error.  Please correct before continuing."
/*%
** Explanation:
**	The 'dd_databases' catalog has more than one row with the 'local_db'
**	column set to 1.
**
** System Status:
**	Replicator cannot determine which is the correct local database.
**
** User Action:
**	Review the 'dd_databases' catalog and ensure only one row is marked
**	as the local database.
*/

F_RM0001_Configuration		"Configuration"
F_RM0002_Configuration_expl	"Configure Replicator"

F_RM0003_Monitor		"Monitor"
F_RM0004_Monitor_expl		"Monitor Replication in this Database"

F_RM0005_Reports		"Resolver"
F_RM0006_Reports_expl		"Resolver and Other Reports"

F_RM0007_Repmgr_main_menu	"Replicator Manager Main Menu"

F_RM0008_Quit_expl		"Exit to the operating system"

E_RM0006_Get_form_failed	"Unable to read from form index file.\n
RepMgr got an error trying to read form '%0c' from your form index file.
Check the setting of the environment variables II_CONFIG and II_LANGUAGE.
Make sure you have read access to all files in the II_LANGUAGE
subdirectory of II_CONFIG, which have a file extension of 'fnx' (formindex
files have an extension 'fnx'). If a 'fnx' file exists, and you have read
access to it, then report this problem to your Site Technical Contact."
/*%
** Explanation:
**	This is caused by a bad installation.  The file "rtiforms.fnx"
**	in the language files contains the forms.  The Ingres names
**	II_TEXT, II_LANGUAGE and II_FORMFILE can affect this.  See
**	the Ingres System Reference Guide for details.
**
** System Status:
**	RepMgr may terminate or may not proceed beyond the point of error.
**
** User Action:
**	Check the installation.
*/

E_RM0007_Invalid_db_no		"Database number must be between 1 and 32767."

E_RM0008_Invalid_vnode_name	"Invalid vnode name.\n
A vnode must begin with an alphanumeric character or an underscore and
must not contain any blanks."

F_RM0009_Saving			"Saving . . ."

E_RM0009_Error_saving		"An error occurred while saving changes.\n
Details about the error were described by the message immediately preceding
this one. The \"Save\" operation was not performed. Please correct the
error and select \"Save\" again."

E_RM000A_Error_committing	"An error occurred while saving this data.\n
Details about the error were described by the message immediately preceding
this one.  The \"Save\" operation was not performed.  Please correct the
error and select \"Save\" again."

F_RM000A_Clear			"Clear"
F_RM000B_Clear_expl		"Clear all fields"

F_RM000C_Define_local_db	"Define Local Database"

S_RM000B_Clear_form		"Do you wish to clear the form?"

S_RM000C_Yes_clear		"Clear form and lose changes"
S_RM000D_No_clear		"Don't clear the form"

S_RM000E_End_no_save		"Do you wish to leave without\nsaving changes?"

S_RM000F_Yes_end		"End and lose changes"
S_RM0010_No_end			"Return to the form"

S_RM0011_Save_local_info	"If you don't save your local database\n
information, you cannot run Replicator Manager.  Select \"End\" again if
you really want to exit."

/* cfgmenu.qc */

F_RM000D_Databases		"Databases"
F_RM000E_Databases_expl		"Define Databases in the Replication Environment"

F_RM000F_CDDS			"CDDS"
F_RM0010_CDDS_expl		"Define Consistent Distributed Data Sets"

F_RM0011_Tables			"Tables"
F_RM0012_Tables_expl		"Register Tables for Replication"

F_RM0013_BuildServer		"BuildServer"
F_RM0014_BuildServer_expl	"Build a Replication Server"

F_RM0015_MoveConfig		"MoveConfig"
F_RM0016_MoveConfig_expl	"Move Configuration Information Between Databases"

F_RM0017_Activate		"Activate"
F_RM0018_Activate_expl		"Activate or Deactivate Change Recording"

F_RM0019_MailList		"MailList"
F_RM001A_MailList_expl		"Define List of Users to Receive Mail on Error"

F_RM001B_Config_Rep		"Configure Replicator"

/* dbsumm.qsc */

F_RM001C_Selecting_data		"Selecting data . . ."

E_RM0012_Error_selecting	"Error occurred SELECTing data.\n
Details about the error were described by the message immediately preceding
this one. Returning to previous frame . . ."

E_RM0013_No_data_found		"No data found, returning to previous frame . . ."

F_RM001D_Add			"Add"
F_RM001E_Add_db_expl		"Add a new database definition"

F_RM001F_Delete			"Delete"
F_RM0020_Delete_db_expl		"Delete current database definition"

E_RM0014_Cant_del_local_db	"You cannot delete the local database."

F_RM0021_database		"database"

F_RM0022_Deleting		"Deleting . . ."

E_RM0015_Error_deleting
"An error occurred while deleting the table field data.\n
Details about the error were described by the message immediately preceding
this one. This data was not deleted. Correct the error and select \"Delete\"
again. The cursor will be placed on the row where the error occurred."

F_RM0023_Edit			"Edit"
F_RM0024_Edit_db_expl		"Edit existing database definition"

F_RM0025_Db_summary		"Replicated Database Summary"

/* dbadd.qsc, dbedit.qsc */

E_RM0016_Db_no_in_use		"Database number %0d already in use."

E_RM0017_Invalid_db_name	"Invalid database name.\n
A database name must begin with an alphabetic character or an underscore
and must not contain any blanks."

E_RM0018_Invalid_dbms_type	"Invalid DBMS type.\n
Select ListChoices for help."

E_RM0019_Error_check_vnode_db
"Error checking uniqueness of vnode::database."

E_RM001A_Db_already_defined
"'%0c' has already been defined as database %1d."

F_RM0026_Add_replic_db		"Add a Replicated Database"
F_RM0027_Edit_replic_db		"Edit a Replicated Database"

/* cddssumm.qsc */

F_RM0028_Add_cdds_expl		"Add a new CDDS definition"
F_RM0029_Delete_cdds_expl	"Delete current CDDS definition"

E_RM001B_Cant_del_default_cdds
"This is the default CDDS.  It cannot be deleted."

E_RM001C_Error_check_cdds_tbls
"Error checking if there are tables registered under this CDDS."

S_RM001D_Cdds_has_tables	"There are tables registered under this CDDS.\n
They will be reassigned to CDDS 0.  Do you want to continue?"

S_RM001E_Yes_reassign		"Reassign tables to CDDS 0"
S_RM001F_No_reassign		"Don't delete CDDS"

F_RM002A_Edit_cdds_expl		"Edit existing CDDS definition"

F_RM002B_Databases_expl
"Specify CDDS database locations and assign servers"

F_RM002C_Paths			"Paths"
F_RM002D_Paths_expl		"Specify Data Propagation Paths"

F_RM002E_Tables_expl		"Display Table List for current CDDS"

F_RM002F_Cdds_summary		"CDDS Summary"

E_RM0020_Invalid_cdds_no	"CDDS number must be between 0 and 32767."

E_RM0021_Cdds_no_in_use		"CDDS number %0d already in use."

E_RM0022_Cdds_name_in_use	"CDDS name already in use."

E_RM0023_Invalid_coll_mode	"Invalid collision mode.\n
Select ListChoices for help."

E_RM0024_Invalid_err_mode	"Invalid error mode.\n
Select ListChoices for help."

F_RM0030_Add_cdds		"Add CDDS Definition"
F_RM0031_Edit_cdds		"Edit CDDS Definition"

/* cddsdb.qsc */

E_RM0025_Error_check_db_no	"Error validating database number."
E_RM0026_Invalid_db_no		"Invalid database number.\n
Select ListChoices for help."

E_RM0027_Invalid_target_type	"Invalid target type.\n
Select ListChoices for help."

E_RM0028_Invalid_server_no	"Server number must be between 1 and 999.\n
The server number must be zero, indicating no server is required, or a
positive integer between 1 and 999."
E_RM0029_No_changes_to_save	"No changes to save."

E_RM002A_Error_saving_tf
"An error occurred while saving changes to the tablefield data.\n
Details about the error were described by the message immediately preceding
this one. The \"Save\" operation was not performed. Please correct the
error and select \"Save\" again. The cursor will be placed on the row where
the error occurred."

F_RM0032_RowDelete		"RowDelete"
F_RM0033_RowDelete_expl		"Delete current row"
F_RM0034_Define_cdds_dbs	"Define CDDS Databases and Servers"

/* cddspath.qsc */

E_RM002B_Orig_db_eq_target_db
"The originator database cannot be the same as the target database."

F_RM0035_None			"None"

E_RM002C_Not_valid_value	"%0d is not a valid value for this field.\n
Select ListChoices for help."

E_RM002D_Orig_local_full_peer
"Originator and Local Database can only accept Full Peer databases."

E_RM002E_Local_db_eq_target_db
"The local database cannot be the same as the target database."

E_RM002F_Target_db_eq_others
"The target database cannot be the same as the originator database or the
local database."

F_RM0036_Insert_expl		"Insert row of data onto screen"
F_RM0037_Update_paths		"Update Data Propagation Paths"

/* cddstbls.qsc */

F_RM0038_yes			"yes"

F_RM0039_Cdds_tables		"View CDDS Table Registration"

/* tblsumm.qsc */

E_RM0030_Err_retrieve_db_name	"Error retrieving local database name."

F_RM003A_Register		"Register"
F_RM003B_Register_expl		"Register all tables or a selected table"

F_RM003C_ThisTable		"ThisTable"
F_RM003D_ThisTable_reg_expl	"Registration for current table"

E_RM0031_Tbl_already_regist	"This table has already been registered.\n
Please use Edit to change registration specifications."

F_RM003E_err			"err"

F_RM003F_AllTables		"AllTables"
F_RM0040_AllTables_reg_expl	"Default registration for all tables"

F_RM0041_Deregister		"Deregister"
F_RM0042_Deregister_expl
"Remove all registration data and support objects"

F_RM0043_ThisTable_dereg_expl	"Deregister the selected table"

F_RM0044_Deregistering		"Deregistering '%0c.%1c' . . ."

F_RM0045_AllTables_dereg_expl	"Deregister all tables"

E_RM0032_Err_updt_last_tbl_no	"Error updating last table number."

F_RM0046_Edit_tbl_expl		"Custom registration of a table"

F_RM0047_Support		"Support"
F_RM0048_Support_expl		"Create replication support objects"

F_RM0049_ThisTable_supp_expl	"Object generation for current table"

E_RM0033_Must_reg_before_supp	"Columns have not yet been registered\n
You must register columns before creating support objects."

E_RM0034_Correct_reg_errors
"Please correct registration error(s) before creating support objects."

F_RM004A_AllTables_supp_expl	"Object generation for all tables"

F_RM004B_Activation		"Activation"
F_RM004C_Activation_expl	"Activate or deactivate Change Recording"

E_RM0035_Reg_supp_before_activ	"You cannot change activation status\n
until a table has been registered and its support procedures created."

F_RM004D_DeActivate		"DeActivate"

F_RM004E_CreateKeys		"CreateKeys"
F_RM004F_CreateKeys_expl	"Create shadow and queue keys for existing data"

F_RM0050_ShadowTableOnly	"ShadowTableOnly"
F_RM0051_BothQueue_ShadowTable	"BothQueue&ShadowTable"

F_RM0052_Table_Registration	"Table Registration for Replication"

E_RM0036_Err_processing_tbl
"Error %0d encountered while processing '%1c.%2c'."

E_RM0037_Found_regist_errors	"Found %0d registration errors."

/* tbledit.qsc */

F_RM0053_Selecting_column_reg
"Selecting column registration for '%0c.%1c' . . ."

S_RM0038_No_table_regist_yet	"There has been no table registration. Do you\n
want to accept the default registration?"

S_RM0039_Yes_register		"Register the table"
S_RM003A_No_register		"Don't register"

E_RM003B_Err_check_uniq_index	"Error occurred checking for unique indexes.\n
Details about the error were described by the message immediately preceding
this one."

E_RM003C_No_unique_indexes	"No unique indexes found.\n
You must define an index to complete registration."

E_RM003D_Error_check_cdds	"Error occurred while validating the CDDS.\n
Details about the error were described by the message immediately preceding
this one."

E_RM003E_Error_check_index	"Error occurred while validating the index used.\n
Details about the error were described by the message immediately preceding
this one."

E_RM003F_Not_valid_value_str	"'%0s' is not a valid value for this field.\n
Select ListChoices for help."

E_RM0040_Error_check_lookup	"Error occurred while validating the lookup table.\n
Details about the error were described by the message immediately preceding
this one."

E_RM0041_Key_col_must_regist
"All key columns MUST be registered for replication."

E_RM0042_Not_null_must_regist
"Columns which are NOT NULL NOT DEFAULT must be registered for replication."

E_RM0043_Col_regist_y_or_n	"Column Registration only takes 'y' or 'n'"

S_RM0044_Drop_existing_objects
"There are existing support objects for this table.\nSaving this table
registration requires that they be dropped.\nDo you want to do that now?"

S_RM0045_Yes_drop		"Drop existing support objects"
S_RM0046_No_drop		"Don't drop support objects"

E_RM0047_No_keys_found		"No keys found."

E_RM0048_Cursor_in_tf_for_top
"Your cursor must be in a table field before selecting \"Top\"."
E_RM0049_Cursor_in_tf_for_bot
"Your cursor must be in a table field before selecting \"Bottom\"."

F_RM0054_Register_columns
"Register Key and Replicated Columns for Replication"

E_RM004A_Error_select_keys	"Error occurred SELECTing key sequence data.\n
Details about the error were described by the message immediately preceding
this one."

S_RM004B_Unique_keys		"Unique Keys"
S_RM004C_Cdds_lookup_tables	"CDDS Lookup Tables"
S_RM004D_Prio_lookup_tables	"Priority Lookup Tables"

/* tblsave.qsc */

E_RM004E_Error_del_regist_col	"Error deleting registered columns."
E_RM004F_Error_ins_regist_col	"Error inserting registered columns."

E_RM0050_Error_updt_regist_tbl
"Error occurred updating table registration data."

F_RM0055_Build			"Build"
F_RM0056_Build_expl		"Build server using existing object files"

F_RM0057_Rebuild		"Rebuild"
F_RM0058_Rebuild_expl		"Rebuild Replicator Server"

E_RM0051_Server_not_built	"The Replicator Server was NOT built."
S_RM0052_Server_built		"The Replicator Server was successfully built."

F_RM0059_Build_replic_server	"Build a Replication Server"

/* movecfg.qsc */

F_RM005A_ThisDatabase		"ThisDatabase"
F_RM005B_ThisDatabase_expl	"Move to specified database"

E_RM0053_Error_moving_config	"Error moving configuration to '%0c::%1c'."

F_RM005C_AllDatabases		"AllDatabases"
F_RM005D_AllDatabases_expl	"Move to all remote databases"

F_RM005E_Move_config_data	"Move Configuration Data"

/* activate.qsc */

F_RM005F_ThisRow		"ThisRow"
F_RM0060_ThisRow_expl		"Current CDDS within current database"

F_RM0061_Activ_cdds_db_expl	"Activate current CDDS in database"

F_RM0062_done			"done"

F_RM0063_Deactiv_cdds_db_expl	"De-activate current CDDS in database"

F_RM0064_Cdds_expl		"Current CDDS"
F_RM0065_Activ_cdds_expl	"Activate current CDDS"
F_RM0066_Deactiv_cdds_expl	"De-activate current CDDS"

F_RM0067_Database		"Database"
F_RM0068_Database_expl		"Current database"
F_RM0069_Activ_db_expl		"Activate current database"
F_RM006A_Deactiv_db_expl	"De-activate current Database"

F_RM006B_All			"All"
F_RM006C_All_expl		"All combinations of databases and CDDSs"
F_RM006D_Activ_all_expl		"Activate all CDDSs and Databases"
F_RM006E_Deactiv_all_expl	"De-activate all CDDSs and Databases"

F_RM006F_Activ_change_rec	"Activate Change Recording"

E_RM0054_Error_deactiv_cdds_db
"Error in deactivating CDDS '%0c' in database '%1c'."

E_RM0055_Error_activ_cdds_db
"Error in activating CDDS '%0c' in database '%1c'."

/* maillist.qsc */

F_RM0070_Changes_saved		"Changes saved."
F_RM0071_Update_error_mail	"Update Error Mail List"

/* monitor.qsc */

F_RM0072_Server			"Server"
F_RM0073_Server_expl		"Server startup, shutdown and configuration"

E_RM0056_Must_be_on_server_tf
"You must be on the Server Status tablefield to act on a server."

F_RM0075_Start			"Start"
F_RM0076_Start_expl		"Start a Replicator Server"

E_RM0057_No_config_file		"No configuration file exists for this server."

E_RM0058_Svr_already_running
"You can not start a server that is already running."

F_RM0077_Starting_rep_server	"Starting Replicator Server"

F_RM0078_Stop			"Stop"
F_RM0079_Stop_expl		"Shutdown a Replicator Server"

S_RM0059_Server_not_running	"This server does not appear to be running.\n
Continue?"

S_RM005A_Yes_stop		"Attempt to stop the server"
S_RM005B_No_stop		"Return to previous frame"

F_RM007A_Stopping_rep_server	"Shutting down the Replicator Server"

F_RM007B_EditConfig		"EditConfig"
F_RM007C_EditConfig_expl	"Edit the server's configuration file"

F_RM007D_ShowAssign		"ShowAssign"
F_RM007E_ShowAssign_expl	"Show the server CDDS and database assignments"

F_RM007F_Ping			"Ping"
F_RM0080_Ping_expl		"Issue a ping event to all servers"

F_RM0081_Queues			"Queues"
F_RM0082_Queues_expl		"Refresh the queue statistics"

F_RM0083_Events			"Events"
F_RM0084_Events_expl		"Raise dbevents to the running servers"

F_RM0085_Replication_monitor	"Replication Monitor"

E_RM005C_No_server_assignments	"There are no servers with assigned targets."

/* editcfg.qsc */

E_RM005D_Rservers_not_defined
"The DD_RSERVERS environment variable has not been defined."

E_RM005E_Error_open_file	"Unable to open file %0c."
E_RM005F_Error_open_file_write	"Unable to open file %0c for writing."

F_RM0086_Default		"Default"
F_RM0087_Default_expl		"Reset configuration file to default values"

E_RM0060_Error_get_local_db	"Error accessing local database.\n
Please set the -IDB and -OWN flags manually."

F_RM0088_unknown		"**UNKNOWN**"

F_RM0089_SelectFlag		"SelectFlag"
F_RM008A_SelectFlag_expl
"Select the current flag into the configuration file"

E_RM0061_Not_on_svr_flags_tf	"You must be on the Server Flags tablefield\n
to use this menu option."

E_RM0062_Flag_already_used
"The server flag '%0c' is already in the runrepl.opt file."

F_RM008B_DescribeFlag		"DescribeFlag"
F_RM008C_DescribeFlag_expl	"Display long description of current flag"

S_RM0063_Flag_has_no_int_val	"This flag has no associated integer value."
S_RM0064_Flag_uses_pos_int	"This flag uses any positive integer."

F_RM008D_Edit_config_file	"Edit Configuration File"

/* asgnshow.qsc */

F_RM008E_Show_svr_assign	"Show Server Assignments"

/* evtlist.qsc */

F_RM008F_Send			"Send"
F_RM0090_Send_expl		"Send this event to a server(s)"

F_RM0091_Description		"Description"
F_RM0092_Description_expl	"Display complete description of event"

F_RM0093_Send_events		"Send Events to Servers"

S_RM0065_Enter_db_no		"Enter the Database number: "
S_RM0066_Enter_cdds_no		"Enter the CDDS number: "

/* evtsend.qsc */

F_RM0094_Send_one_expl		"Raise the event to the selected server"
F_RM0095_Send_all_expl		"Raise the event to all servers"

F_RM0096_Select_svr_to_send	"Select Server to Send Event to"

E_RM0067_Error_issuing_event	"Event not issued."
S_RM0068_Event_issued		"Event issued."

/* reports.qc */

F_RM0097_Collision		"Collision"
F_RM0098_Collision_expl		"Queue Collision Report"

F_RM0099_Integrity		"Integrity"
F_RM009A_Integrity_expl		"Table Integrity Report"

F_RM009B_DistribConfig		"Distrib Config"
F_RM009C_DistribConfig_expl	"Distributed Configuration Checker"

F_RM009D_Databases_rpt_expl	"Databases Report"
F_RM009E_Cdds_rpt_expl		"CDDS and Data Propagation Paths Report"
F_RM009F_Tables_rpt_expl	"Replicated Tables and Columns Report"

F_RM00A0_Resolver_other_rpts	"Resolver and Other Reports"

F_RM00A1_Compar_distrib_queue
"Comparing local distribution queue with targets . . ."

E_RM0069_Error_collision_rpt	"Error while running the Collision Report."
E_RM006A_Error_tbl_integ_rpt
"Error while running the Table Integrity Report."

E_RM006B_Error_cfg_check_rpt
"Error while running the Configuration Checker report."

E_RM006C_Error_dbs_rpt		"Error while running the Databases report."
E_RM006D_Error_paths_rpt	"Error while running the CDDS Paths report."
E_RM006E_Error_tables_rpt	"Error while running the Tables report."

/* tblintgt.qsc */

E_RM006F_Error_check_tbl_reg	"Error verifying table registration for '%0c'."
E_RM0070_Tbl_not_registered	"Table is not registered for replication."
E_RM0071_Tbl_not_exist		"Table does not exist."

F_RM00A2_Table_Owner		"Table Owner"

E_RM0072_Invalid_cdds_no	"Invalid CDDS number."

F_RM00A3_Go_tbl_integ		"Run table integrity report"

F_RM00A4_Working		"Working . . ."

E_RM0073_No_valid_cdds		"No valid CDDS in lookup table."

F_RM00A5_Tbl_integrity_rpt	"Table Integrity Report"

E_RM0074_Error_open_file_read	"Cannot open file '%0c' for reading."

S_RM0075_Dereplic_db
"Dropping Replicator objects from database '%0c' . . .\n"
E_RM0076_Error_dereplic_db
"Error dropping Replicator objects from database '%0c'.\n"
S_RM0077_Dereplic_success
"Replicator objects dropped successfully from database '%0c'.\n"

S_RM0078_Create_rep_cat_db
"Creating Replicator catalogs on database '%0c' . . .\n"
E_RM0079_Error_rep_cat_db
"Error creating Replicator catalogs on database '%0c'.\n"
S_RM007A_Rep_cat_success
"Replicator catalogs for database '%0c' created successfully.\n"

S_RM007B_Modify_rep_cat_db
"Modifying Replicator catalogs on database '%0c' . . .\n"
E_RM007C_Error_mod_cat_db
"Error modifying Replicator catalogs on database '%0c'.\n"
S_RM007D_Rep_mod_success
"Replicator catalogs for database '%0c' modified successfully.\n"

/* monitor.qsc */

S_RM007E_Server_couldnt_start	"The Server could not be started"
S_RM007F_Server_shutdown	"The Server has shutdown"

/* targtype.c */

F_RM0074_Unknown_type		"**UNKNOWN TYPE**"
F_RM00A6_Full_peer		"Full Peer"
F_RM00A7_Prot_read		"Protected Read"
F_RM00A8_Unprot_read		"Unprotected Read"

E_RM0080_No_valid_choices	"No valid choices available."

E_RM0081_Invalid_key_type	"Invalid datatype for key.\n
FLOAT, MONEY and LONG datatypes cannot be used as part of the replicated
table primary key."

/* cddsacti.sc */

E_RM0082_Error_rtrv_tables
"Error retrieving registered tables information."

E_RM0083_Error_alloc_tables
"Error allocating memory for registered table numbers."

E_RM0084_Error_connecting	"Error connecting to database '%0c'."

/* tblrules.sc */

E_RM0085_No_cols_registered	"No columns registered.\n
No columns have been registered for table '%0c.%1c'. Register the table before
creating the support objects."

E_RM0086_No_supp_objs		"No supports objects exist.\n
Support objects have not been created for table '%0c.%1c'. Register the table
and create the support objects before activating the table."

/* ckdstcfg.sc */
E_RM0087_Err_open_report	"Error opening report file '%0c'."

E_RM0088_Err_rtrv_db_name	"Error retrieving database name."

F_RM00A9_Prod_name		"Ingres Replicator"

F_RM00AA_Distrib_Cfg_Checker	"Distributed Configuration Checker Report"

F_RM00AB_From_local_db		"From local database '%s'"

E_RM0089_Err_check_paths	"Error checking Data Propagation Paths catalog."

E_RM008A_No_paths
"No Data Propagation Paths found. Data will not be replicated from this
database."

E_RM008B_Err_open_dbinfo_curs
"Error opening local cursor for retrieving database information."
E_RM008C_Err_fetch_dbinfo_curs
"Error fetching database information from local cursor."
E_RM008D_Err_close_dbinfo_curs
"Error closing local cursor for retrieving database information."

E_RM008E_Err_local_rollback	"Error on local rollback."
E_RM008F_Err_local_commit	"Error on local commit."

F_RM00AC_Report_on_remote_db	"Report on remote database '%s' (%d)"

E_RM0090_No_further_check
"\nError prohibits further checking on database number %d\n"

F_RM00AD_No_problems		"     No problems found."

E_RM0091_Err_rtrv_dba		"Error retrieving DBA name from database '%0c'."
E_RM0092_Err_ck_catalogs
"Error verifying existence of Replicator catalogs in database '%0c'."
E_RM0093_Miss_rep_catalogs
"Replicator catalogs not found in database '%0c'."
E_RM0094_Err_open_db_curs
"Error opening database information cursor in database '%0c'."
E_RM0095_Err_fetch_db_curs
"Error fetching from database information cursor in database '%0c'."
E_RM0096_Err_rtrv_db_info
"Error retrieving database %0d information from database '%1c'."
E_RM0097_Dd_databases_err	"    DD_DATABASES ERROR:"
E_RM0098_Db_not_found
"        Record for database number %0d, database name '%1c'\n
       not found in database '%2c'."
E_RM0099_Db_name_diff
"        Database number %0d, has name '%1c' in database '%2c'\n
       but name '%3c' in database '%4c'."
E_RM009A_Vnode_name_diff
"        Database number %0d, has vnode name '%1c' in database '%2c'\n
       but vnode name '%3c' in database '%4c'."
E_RM009B_Dbms_type_diff
"        Database number %0d, has DBMS type '%1c' in database '%2c'\n
       but DBMS type '%3c' in database '%4c'."
E_RM009C_Err_close_db_curs
"Error closing database information cursor in database '%0c'."

E_RM009D_Err_open_tbl_curs
"Error opening cursor for retrieving replicated table information in database
'%0c'."
E_RM009E_Err_fetch_tbl_curs
"Error fetching from replicated table information cursor in database '%0c'."
E_RM009F_Err_rtrv_tbl_info
"Error retrieving table %0d information from database '%1c'."
E_RM00A0_Dd_regist_tbls_err	"    DD_REGIST_TABLES ERROR:"
E_RM00A1_Tbl_not_found
"        Record for table number %0d, table name '%1c'\n
       not found in database '%2c'."
E_RM00A2_Supp_not_created
"        Support objects for table number %0d, table name '%1c'\n
       owner '%2c', not created in database '%3c'."
E_RM00A3_Cols_not_regist
"        Columns for table number %0d, table name '%1c'\n
       owner '%2c', not registered in database '%3c'."
E_RM00A4_Tbl_name_diff
"        Table number %0d, has name '%1c' in database '%2c'\n
       but name '%3c' in database '%4c'."
E_RM00A5_Tbl_owner_diff
"        Table number %0d, has owner '%1c' in database '%2c'\n
       but owner '%3c' in database '%4c'."
E_RM00A6_Tbl_cdds_diff
"        Table number %0d, is assigned to CDDS %1d in database '%2c'\n
       but CDDS %3d in database '%4c'."
E_RM00A7_Err_close_tbl_curs
"Error closing cursor for retrieving replicated table information in database
'%0c'."

E_RM00A8_Err_open_col_curs
"Error opening cursor for retrieving replicated column information in database
'%0c'."
E_RM00A9_Err_fetch_col_curs
"Error fetching from replicated column information cursor in database '%0c'."
E_RM00AA_Err_rtrv_col_info
"Error retrieving table %0d column '%1c' information from database '%2c'."
E_RM00AB_Dd_regist_cols_err	"    DD_REGIST_COLUMNS ERROR:"
E_RM00AC_Col_not_found
"        Record for table number %0d, column name '%1c'\n
       not found in database '%2c'."
E_RM00AD_Col_key_seq_diff
"        Table number %0d, column name '%1c' has key sequence %2d\n
       in database '%3c' but key sequence %4d in database '%5c'."
E_RM00AE_Col_col_seq_diff
"        Table number %0d, column name '%1c' has column sequence %2d\n
       in database '%3c' but column sequence %4d in database '%5c'."
E_RM00AF_Err_close_col_curs
"Error closing cursor for retrieving replicated column information in database
'%0c'."

E_RM00B0_Err_open_cdds_curs
"Error opening cursor for retrieving CDDS information in database '%0c'."
E_RM00B1_Err_fetch_cdds_curs
"Error fetching from CDDS information cursor in database '%0c'."
E_RM00B2_Err_rtrv_cdds_info
"Error retrieving CDDS %0d information from database '%1c'."
E_RM00B3_Dd_cdds_err		"    DD_CDDS ERROR:"
E_RM00B4_Cdds_not_found
"        Record for CDDS number %0d not found in database '%1c'."
E_RM00B5_Cdds_name_diff
"        CDDS number %0d, has name '%1c' in database '%2c'\n
       but name '%3c' in database '%4c'."
E_RM00B6_Cdds_coll_mode_diff
"        CDDS number %0d has collision mode %1d in database '%2c'\n
       but collision mode %3d in database '%4c'."
E_RM00B7_Cdds_err_mode_diff
"        CDDS number %0d has error mode %1d in database '%2c'\n
       but error mode %3d in database '%4c'."
E_RM00B8_Err_close_cdds_curs
"Error closing cursor for retrieving CDDS information in database '%0c'."

E_RM00B9_Err_open_dbcdds_curs
"Error opening cursor for retrieving CDDS Database information in database
'%0c'."
E_RM00BA_Err_fetch_dbcdds_curs
"Error fetching from CDDS Database information cursor in database '%0c'."
E_RM00BB_Err_rtrv_dbcdds_info
"Error retrieving CDDS %0d Database %1d information from database '%2c'."
E_RM00BC_Dd_db_cdds_err		"    DD_DB_CDDS ERROR:"
E_RM00BD_Dbcdds_not_found
"        Record for CDDS %0d Database %1d not found in database '%2c'."
E_RM00BE_Dbcdds_targtype_diff
"        Database number %0d in CDDS number %1d has target type %2d\n
       in database '%3c' but target type %4d in database '%5c'."
E_RM00BF_Err_close_dbcdds_curs
"Error closing cursor for retrieving CDDS Database information in database
'%0c'."

E_RM00C0_Err_open_path_curs
"Error opening cursor for retrieving Data Propagation Path information in
database '%0c'."
E_RM00C1_Err_fetch_path_curs
"Error fetching from Data Propagation Path information cursor in database
'%0c'."
E_RM00C2_Err_rtrv_path_info
"Error retrieving CDDS %0d Source Database %1d Local Database %2d Target
Database %3d information from database '%4c'."
E_RM00C3_Dd_paths_err		"    DD_PATHS ERROR:"
E_RM00C4_Path_not_found
"        Record for CDDS %0d Source Database %1d Local Database %2d\n
       Target Database %3d not found in database '%4c'."
E_RM00C5_Path_target_diff
"        CDDS %0d Source Database %1d Local Database %2d Target Database %3d\n
       has final target %4d in database '%5c' but final target %6d\n
       in database '%7c'."
E_RM00C6_Err_close_path_curs
"Error closing cursor for retrieving Data Propagation Path information in
database '%0c'."
E_RM00C7_Err_set_session	"Error switching to session number %0d." 

/* dbrpt.sc */
F_RM00AE_Replic_dbs_rpt		"Replicated Databases Report"
F_RM00AF_Db_no			"Database #"
F_RM00B0_Vnode_db_name		"Virtual Node Name / Database Name"
F_RM00B1_Dbms_type		"DBMS Type"
F_RM00B2_Db_owner_remarks	"Database Owner / Remarks"

/* pathrpt.sc */
F_RM00B3_Cdds_paths_rpt		"CDDS Data Propagation Paths Report"
F_RM00B4_Cdds_no		"CDDS No:"
F_RM00B5_Orig_db		"Originating Database"
F_RM00B6_Local_db		"Local Database"
F_RM00B7_Targ_db		"Target Database"

/* tablerpt.sc */
F_RM00B8_Regist_tbls_rpt	"Registered Tables Report"
F_RM00B9_Table			"Table"
F_RM00BA_C_S_A			"C S A"
F_RM00BB_Prio_lkup_tbl		"Priority Lookup Table"
F_RM00BC_Cdds			" CDDS"
F_RM00BD_Cdds_lkup_tbl		"CDDS Lookup Table"
F_RM00BE_Column			"  Column"
F_RM00BF_Datatype		"Datatype"
F_RM00C0_D_K_R			"D  K  R"
F_RM00C1_Cols_regist		"C - Columns Registered"
F_RM00C2_Tbl_key_seq		"D - Table Key Sequence"
F_RM00C3_Supp_objs		"S - Support Objects Created"
F_RM00C4_Rep_key_col		"K - Replicated Key Column"
F_RM00C5_Chg_rec_activ		"A - Change Recording Activated"
F_RM00C6_Rep_col		"R - Replicated Column"
F_RM00C7_K			"K"
F_RM00C8_R			"R"

/* tblinteg.sc */
E_RM00C8_Err_rtrv_db_name
"Error retrieving database name for database number %0d."
E_RM00C9_Db_not_found		"Database number %0d not found."
E_RM00CA_Err_rtrv_targ_type
"Error retrieving target type for CDDS %0d Database %1d."
E_RM00CB_No_URO_integ_rpt	"Cannot run integrity report on URO target.\n
Database '%0c' in CDDS %1d is an Unprotected Read Only target. The Integrity
Report can only be run on Full Peer and Protected Read Only targets."
F_RM00C9_Tbl_integ_rpt		"Table Integrity Report"
F_RM00CA_For_table		"For table '%s.%s' in '%s'"
F_RM00CB_And_table		"And table '%s.%s' in '%s'"
E_RM00CC_Keys_not_unique
"      Registered key columns do not force the table to be\n
     unique in '%s'."
F_RM00CC_Unbounded_datatype	"**Unbounded Data Type**"
E_RM00CD_No_replic_cols
"No replicated columns found for table '%0c.%1c'."
E_RM00CE_No_key_cols
"No key columns found for table '%0c.%1c'."
E_RM00CF_Prep_failed
"PREPARE statement failed for table '%0c.%1c' in database '%2c'."
E_RM00D0_Descr_failed
"DESCRIBE statement failed for table '%0c.%1c' in database '%2c'."
E_RM00D1_Err_open_integ_curs
"Error opening cursor for retrieving table integrity information in
database '%0c'."
F_RM00CD_Comparing		"Comparing contents in '%0c' with '%1c' ..."
E_RM00D2_Err_fetch_integ_curs
"Error fetching from table integrity cursor in database '%0c'."
E_RM00D3_Err_exec_immed
"Error in EXECUTE IMMEDIATE retrieval in database '%0c'."
E_RM00D4_In_dbx_not_in_dby	"\nRow only in '%s', not in '%s'\n"
E_RM00D5_Src_txn_seq
"Source DB:  %d, Transaction ID:  %d, Sequence no:  %d.\n"
E_RM00D6_Tbl_mismatch
"Table definition mismatch between '%0c' and '%1c'."
E_RM00D7_Row_diff_dbx_vs_dby	"\nRow is different in '%s' than in '%s'\n"
F_RM00CE_No_diffs		"No differences found."

/* loaddict.sc */
E_RM00D8_Err_creating_cat	"Error creating Replicator catalog '%0c'."
E_RM00D9_Err_loading_cat	"Error loading Replicator catalog '%0c'."

/* dropobj.sc */
E_RM00DA_Err_drop_obj		"Error dropping database object '%0c'. %1c"

/* tblexist.sc */
E_RM00DB_Err_ck_exist		"Error checking existence of table '%0c.%1c'."

/* tblregst.sc */
E_RM00DC_Err_ck_null_cols
"Error checking for nullable columns in table '%0c.%1c'."
E_RM00DD_Tbl_null_keys		"Table has nullable key columns.\n
Replicator does not support nullable columns as part of the primary key, so
table '%0c.%1c' cannot be registered."
E_RM00DE_Err_add_regist_cols
"Error adding registered column information for table '%0c.%1c'."
E_RM00DF_Err_updt_regist_cols
"Error updating registered column information for table '%0c.%1c'."
E_RM00E0_No_unique_key		"Table does not have unique keys.\n
Table '%0c.%1c' cannot be registered for replication because it does not
have a unique key. Tables can only be replicated if they have a PRIMARY KEY
or UNIQUE constraint, they use a UNIQUE storage structure or they have a
UNIQUE secondary index."
E_RM00E1_Err_get_index
"Error getting index information for table '%0c.%1c'."
E_RM00E2_Err_asgn_tbl_no
"Error assigning table number for table '%0c.%1c'."

/* dbcfgchg.sc */
E_RM00E3_Err_updt_cfg_chg
"Error updating the configuration change timestamp."

/* ddgensup.sc */
E_RM00E4_Supp_obj_create_fail
"Creation of support objects for table '%0c.%1c' failed."
E_RM00E5_Proc_up_to_date
"Procedures for table '%0c.%1c' are up to date . . ."
E_RM00E6_Dbproc_create_fail
"Creation of database procedures for table '%0c.%1c' failed."
F_RM00CF_Supp_obj_create_succ
"Creation of support objects for table '%0c.%1c' successful.\n"

/* ddgenrul.sc */
E_RM00E7_Activ_tbl_fail		"Activation of table '%0c.%1c' failed."
F_RM00D0_Activ_tbl_succ		"Table '%0c.%1c' activated successfully.\n"

/* issvrrun.sc */
E_RM00E8_Err_rtrv_pid		"Error retrieving process information.\n
Unable to determine if Replicator Server is running already."

/* printcol.sc */
E_RM00E9_Err_rtrv_col_info
"Error retrieving information for column '%0c' of table '%1c.%2c'."
E_RM00EA_Col_info_miss
"Column information for '%s.%s' is missing.  Record deleted?\n"

/* expfunc.sc */
E_RM00EB_Err_gen_obj_name
"Error generating object name for table '%0c.%1c'."

/* deregtbl.sc */
E_RM00EC_Err_del_reg_tbl
"Error deleting registered table information for table number %0d."
E_RM00ED_Cant_dereg_tbl
"You cannot deregister a table with outstanding queue entries."

/* sqlgen.sc */
E_RM00EE_Null_empty_arg
"Internal error in routine '%0c':  Null or empty argument."
E_RM00EF_Err_rtrv_node
"Cannot determine local vnode name. Unable to run generated script. %0c"

/* exptoken.c */
E_RM00F0_Exec_str_too_long
"String too long for EXECUTE IMMEDIATE processing."

/* genopen.c */
E_RM00F1_Err_open_template	"Error opening template file '%0c'."
E_RM00F2_Err_open_outfile	"Error opening output file '%0c'."

/* svgenlog.c */
E_RM00F3_Err_open_logfile	"Error opening error log file '%0c'."

/* tblprocs.sc */
E_RM00F4_No_URO_procs		"URO target doesn't need procedures.\n
The current table is an Unprotected Read Only target. It is not necessary to
create database procedures for it."
E_RM00F5_Err_rtrv_dba		"Error retrieving DBA name."
E_RM00F6_Err_rtrv_proc_time
"Error retrieving procedure timestamp for table '%0c.%1c'."
E_RM00F7_No_grant_permit	"No GRANT or GRANT compatible permit."
E_RM00F8_Err_creat_proc		"Error creating procedure '%0c'.\n
If the following screen indicates insufficient system/server resources, you
may have more columns in this table than your system configuration will allow
you to replicate."
F_RM00D1_Drop_proc		"Dropping procedure '%0c'."
E_RM00F9_Err_grant_proc
"Error granting EXECUTE privilege on database procedure '%0c'."

/* tblfetch.sc */
E_RM00FA_Tbl_not_found
"Table number %0d not found in registered tables catalog."
E_RM00FB_No_valid_cdds		"No valid CDDS in lookup table.\n
The lookup table for table '%0c.%1c' doesn't have a valid CDDS for this
database."
E_RM00FC_Miss_cdds_defn		"Missing CDDS definition.\n
The relationship between CDDS number %0d and this database has not been
established."
E_RM00FD_No_regist_cols		"No registered columns.\n
No columns of table '%0c.%1c' have been registered for replication. A table
cannot be processed for replication without first registering it."
E_RM00FE_Err_alloc_col
"Error allocating memory for replicated column information."
E_RM00FF_Err_col_count
"Error retrieving replicated column information for table '%0c.%1c':
Inconsistent count of columns."

/* crsuptbl.sc */
E_RM0100_Tbl_not_found		"Table '%0c.%1c' not found."
E_RM0101_Err_rtrv_shad_info
"Error retrieving shadow table information for table '%0c.%1c'."
E_RM0102_Tbl_create_fail	"Table create failed for '%0c'."
E_RM0103_Err_create_shad	"Error creating shadow table for '%0c.%1c'."
E_RM0104_Err_modify_shad	"Error modifying shadow table '%0c'."
E_RM0105_Err_index_shad		"Error creating index on shadow table '%0c'."
E_RM0106_Err_ins_sha_sup
"Error adding shadow table '%0c' to support tables catalog."
E_RM0107_Err_create_arch	"Error creating archive table for '%0c.%1c'."
E_RM0108_Err_modify_arch	"Error modifying archive table '%0c'."
E_RM0109_Err_ins_arc_sup
"Error adding archive table '%0c' to support tables catalog."
E_RM010A_Err_grant_tbl
"Error granting privileges on table '%0c.%1c'."
F_RM00D2_Proc_tbl		"Processing table '%s.%s' . . .\n\r"
F_RM00D3_Gen_sup_tbl
"Generating support tables for '%s.%s' . . . \n\r"
E_RM010B_Sup_up_to_date
"Support tables for '%s.%s' are up to date . . .\n\r"

/* move1cfg.sc */
E_RM010C_Err_copy_out		"Error copying out catalog '%0c'."
E_RM010D_Err_clear_cat		"Error clearing catalog '%0c'."
E_RM010E_Err_copy_in		"Error copying in catalog '%0c'."
E_RM010F_Err_adj_owner
"Error adjusting table ownership in the registered tables catalog."
E_RM0110_Err_adj_localdb
"Error adjusting the local database flag in the databases catalog."
E_RM0111_Err_rtrv_type		"Error retrieving DBMS type of target database."
E_RM0112_Err_open_tbl_curs
"Error opening cursor for retrieving replicated table information."
E_RM0113_Err_fetch_tbl_curs
"Error fetching from replicated table information cursor."
F_RM00D4_Moving_cats
"Moving contents of Replicator catalogs to '%s' . . .\n\r"

/* collrpt.sc */
F_RM00D5_Queue_coll_rpt		"Queue Collision Report"
F_RM00D6_Insert_coll		"INSERT collision for table"
F_RM00D7_Srcdb_txn_seq
"Source DB:  %d    Transaction ID:  %d    Sequence No:  %d\n"
E_RM0114_Err_rtrv_rem_db
"Error retrieving remote database name for database number %0d."
F_RM00D8_Not_found		"NOT FOUND"
F_RM00D9_Remote_db
"Remote Database Number: %d, Node:  %s, Name:  %s\n"
F_RM00DA_Update_coll		"UPDATE collision for table"
F_RM00DB_Delete_coll		"DELETE collision for table"
E_RM0115_Err_exec_immed
"Error in EXECUTE IMMEDIATE retrieval from '%0c'."
F_RM00DC_Colls_found		"\n%d collision(s) found.\n"

/* colllist.sc */
E_RM0116_Err_open_queue_curs
"Error opening cursor for retrieving distribution queue information."
E_RM0117_Err_fetch_queue_curs
"Error fetching distribution queue information from cursor."
E_RM0118_Err_ck_targ_row
"Error checking if row exists in target base table."
E_RM0119_Err_rtrv_shad_row
"Error retrieving replicated transaction key from shadow table."
E_RM011A_Err_ck_targ_shad
"Error checking target shadow table for unarchived transactions."
E_RM011B_Err_ck_src_shad
"Error checking source shadow table for previous transaction."
E_RM011C_Err_cnt_conns		"Error counting potential connections."
E_RM011D_Err_alloc_sess
"Error allocating memory for session information."
E_RM011E_Err_open_sess_curs
"Error opening cursor to retrieve session information."
E_RM011F_Err_fetch_sess_curs
"Error fetching from target database information cursor."

/* move1cfg.sc */
E_RM0120_Incomplete_cfg
"Incomplete configuration. No tables are registered."

/* pathrpt.sc */
E_RM0121_Err_rtrv_paths
"Error retrieving Data Propagation Path information."

/* repcat.sc */
E_RM0122_Catalogs_exist		"Replicator catalogs already installed."

E_RM0123_Dist_queue_empty
"There are no entries in the distribution queue."

/* crrepkey.sc */
F_RM00DD_Creating_keys		"Creating replication keys for '%0c.%1c' . . ."
F_RM00DE_Rows_completed
"Completed %0d rows (of approximately %1d) . . ."
E_RM0124_Tbl_not_exist		"Table '%0c.%1c' does not exist."
E_RM0125_Shadow_not_exist	"Shadow for table '%0c.%1c' does not exist."
E_RM0126_Err_rtrv_num_rows	"Error retrieving number of rows in '%0c.%1c.'"
E_RM0127_Err_set_nojournal	"Error disabling journaling on '%0c.%1c.'"
E_RM0128_Err_dropping		"Error dropping '%c'."
E_RM0129_Err_modifying		"Error modifying '%c'."
E_RM012A_Err_creat_tmp_tbl	"Error creating temporary table '%c'."
E_RM012B_Unsupp_datatype	"Unsupported datatype '%0c' for column '%1c'."
E_RM012C_Err_prep_stmt		"Error preparing SELECT from base table."
E_RM012D_Err_descr_stmt		"Error describing SELECT from base table."
E_RM012E_Err_open_base_curs
"Error opening cursor for SELECT from base table."
E_RM012F_Err_get_trans_id	"Error getting transaction ID."
E_RM0130_Err_fetch_base		"Error retrieving data from base table."
E_RM0131_Err_copy_from		"Error copying into '%0c' from '%1c'."
E_RM0132_Err_rtrv_col_name	"Error retrieving lookup column name."
E_RM0133_Err_lookup_value	"Error looking up '%0c' in '%1c'."
E_RM0134_Err_set_journal	"Error enabling journaling on '%0c.%1c.'"
E_RM0135_Err_alloc_ucode_key	"Error allocating memory for keys for unicode column."
