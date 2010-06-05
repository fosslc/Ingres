/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
**
*/

/*
**  Name:  erst.msg    Message definition file for software tools facility
**
**  Description:
**
**      Contains all messages for the Support Tools facility, including:
**      
**		cbf
**              netutil
**              ingbuild
**		setup scripts	
**
**  History:
**      17-jul-1992 (jonb)
**      	Created
**	17-feb-1993 (jonb)
**		Added messages for the listpick that lets the user blow
**		away the setup phase if a setup program fails.
**	11-mar-1993 (kellyp)
**		Added 2 new messages to be displayed during the installation
**      	of support packages. Also, renamed component -> package.
**	30-mar-93 (jonb)
**		Change decimal number to hex in netutil error message.
**	26-apr-93 (tyler)
**		Improved messages used by cbf.
**	12-may-93 (kellyp)
**		Indented header displayed by "iimaninf -packages".
**	13-may-93 (kellyp)
**		Updated S_ST0100_version_id.  Brought iimaninf usage
**		message up to date.
**	21-may-93 (kellyp)
**		Updated S_ST0100_version_id.  Made minor change
**		to an ingbuild error message.
**	21-jun-93 (tyler)
**		Minor changes to cbf messages.  Changed all instances
**		of "Ingres" (the product) to "INGRES".  Added History
**		comments for previous three changes by kellyp.  Updated
**		S_ST0100_version_id for this integration.
**	13-jul-93 (tyler)
**		Changes made during portable manifest file project. 
**	14-jul-93 (tyler)
**		Changed message to indicate 'ln' failure and removed
**		'mkdir' failure message. 
**	16-jul-93 (tyler)
**		Added failure message for Unix "chmod" command.
**	23-jul-93 (tyler)
**		More/better messages for the server support tools.
**		Incremented S_ST0100_version_id message for upcoming
**		integration.
**	04-aug-93 (tyler)
**		Added/improved various messages.
**	05-aug-93 (tyler)
**		More message improvements.
**	20-sep-93 (tyler)
**		Fixed broken message.
**	19-oct-93 (tyler)
**		More message and improvements.  Also enhancements by
**		joplin to netutil messages.
**	11-nov-93 (tyler)
**		Removed unused message.
**	22-nov-93 (tyler)
**		Still more message additions/changes.
**	30-nov-93 (tyler)
**		Yet another message.
**	01-dec-93 (tyler)
**		Fixed messages relating to errors resulting from lack 
**		of write permission on INGRES configuration files.
**	05-jan-94 (tyler)
**		Changed INGRES to OpenINGRES and improved install
**		popup used to report disk space requirements.
**	3-feb-94 (robf)
**              Add messages for new secure options
**	22-feb-94 (tyler)
**		Added ingstart messages.  Added various cbf messages.
**		Fixed BUG 59511: Removed -output from ingbuild usage
**		message.  Fixed BUG 58907: ingstart should explain that
**		failure to start the name server is probably due to 
**		a running name server and that ingstop should be used
**		to shut down.
**	24-mar-94 (joplin)
**		Updated usage message for iimaninf.
**	08-Apr-94 (michael)
**		Changed references of II_AUTHORIZATION to II_AUTH_STRING where
**              appropriate.
**	21-apr-94 (joplin)
**		Changed ingbuild install_failed message to include space
**		warning on the target device.
**	10-jan-95 (sweeney)
**		Change all references to ASK/Ingres to CA-Ingres, to
**		reflect the proud new owner.
**	05-feb-95 (carly01) b62921
**		Changed msg E_ST0108_install_failed to reflect requirement of
**		ingres as user during install.
**      06-mar-95 (tutto01)  BUG #67316
**              Changed S_ST0185 to E_ST0185 to comply with message naming
**              format.
**      04-apr-95 (harpa06) Bug #67859:
**              Changed S_ST0525_iistar_failed to read "Unable to start STAR 
**		DBMS server."
**	30-mar-95 (haanch04) BUG # 67735
**		revised ingbuild -help message
**      14-apr-95 (chech02) Bug #67860
**              Added msg 'S_ST0528_invalid_auth_string' for ingstart.
**	07-jul-95 (harpa06) Bug 69560
**		Modified messages S_ST039C, S_ST039E. Modified S_ST0420 and
**		S_ST0421
**	31-aug-95 (tutto01)
**		Changed OpenINGRES to OpenIngres.
**	11-Nov-95 (hanch04)
**		Added menuitems for ingbuild,  F_ST010C, F_ST010D.
**	29-Nov-95 (hanch04)
**		Added message for ingbuild,  F_ST0207 F_ST0208 F_ST010E
**	10-jan-1996 (nick)
**		Changed S_ST037E and S_ST037F ( were reserved ).
**	16-jan-96 (dougb)
**		Modified message E_ST017F to conform to updated iimaninf 
**		interface.
**	15-Feb-96 (wonst02)
**		Added messages for ingstart: S_ST0529_iigws_failed,
**		S_ST0530_starting_iigws.
**	26-feb-1996 (canor01)
**		Added messages S_ST042C_no_directory, S_ST042D_create_dir,
**		and S_ST042E_no_create_dir for cbf directory checking.
**	13-feb-96 (harpa06)
**		Added S_ST02FF_internet_comm, 
**		S_ST042F_config_internet_comm,
**		S_ST0430_internet_parameters for internet communication.
**	10-apr-96 (nick)
**	    Added E_ST020B to fix #62921.
**      02-apr-1996 (rajus01)
**              Added S_ST0529_configure_bridge, S_ST052A_bridge_parameters,
**              S_ST052B_bridge_server, S_ST052C_bridge_protocols_menu,
**              S_ST052D_starting_iigcb, S_ST052E_iigcb_failed for Protocol
**              Bridge server.
**	06-jun-1996 (thaju02)
**		Added S_ST0431_dmf_cache_2k, S_ST0432_dmf_cache_4k,
**		S_ST0433_dmf_cache_8k, S_ST0434_dmf_cache_16k, 
**		S_ST0435_dmf_cache_32k, S_ST0436_dmf_cache_64k, 
**		S_ST0437_p4k, S_ST0438_p8k, S_ST0439_p16k, S_ST0440_p32k,
**		S_ST0441_p64k for variable page size support.
**	30-jul-1996 (thaju02)
**	        Added S_ST0442_config_caches, S_ST0443_dbms_caches
**	10-apr-96 (nick)
**	    Added E_ST020B to fix #62921.
**	06-sep-96 (mcgem01)
**		The CA-OpenIngres Desktop gateway server has been renamed
**		to desktop server.
**	16-oct-96 (mcgem01)
**		Removed hard-coded ingres references.
**	28-Oct-96 (hanch04)
**		Changed S_ST010C_installation_is_still_up to
**		S_ST010C_installation_still_up, message was to long.
**	31-jan-97 (thaju02)
**		Added S_ST0444_dmcm_warning.
**	25-feb-97 (funka01)
**		Added S_ST0445_p2k for generic.sc for jasmine.
**		Update ii* ,II_INSTALLATION, II_SYSTEM to be generic for
**		JASMINE
**	16-apr-97 (funka01)
**		Updated to %s instead of %s_SYSTEM for S_ST0312
**	02-jun-97 (nanpr01)
**		Added message to reformat the log if log.block_size is 
**		changed.
**	21-Aug-97 (rajus01)
**		Added messages related to specifying attribute
**	  	information to the vnode.
**      02-Oct-97 (fanra01)
**              Add messages for remote command server.
**      03-Oct-97 (fanra01)
**              Add comment for previous submission.
**              Add message for failed read of restore parameters.
**	17-Oct-97 (mcgem01)
**		Change E_ST0535 to E_ST053B as ST0535 was already in use.
**      05-nov-1997 (canor01)
**              Add E_ST053C_nopriv.
**	23-Jan-98 (rajus01)
**		Added S_ST053D_choose_login_type. Changed maessage for
**		S_ST0020_choose_vnode_type.
**	09-mar-1998 (somsa01)
**		Added S_ST0540_cannot_run_rcpstat.
**	29-apr-1998 (rigka01) - bug 90660
**		Add S_ST02FE_rmcmd and S_ST0448_startupcnt_zero_one to
**		support Remote Command startup through CBF on Windows NT
**	24-mar-98 (mcgem01)
**		Added startup configuration and failure messages for 
**		the Informix, Sybase, Oracle, SQL Server and ODBC gateways.
**	08-apr-98 (mcgem01)
**		Remove duplicate message.
**	16-may-1998 (canor01)
**		Genericize ST0540 message.
**	27-may-1998 (hanch04)
**		Removed duplicate ST0540 message.
**	01-jun-1998 (hanch04)
**		Added E_ST056A_no_calicense.
**	03-jun-98 (mcgem01)
**		Add message reporting that Workgroup is limited to a single
**		DBMS server.
**	09-jun-1998 (hanch04)
**		Added S_ST056B_invalid_calicense
**	16-Jun-1998 (thaal01)
**		Added S_ST0541_derived_name, as GCN now needs a derived 
**		parameter menu.  Part of bug 91342. 
**	26-Jul-98 (gordy)
**		Added strings for GCF/GCS security configuration.
**	20-jul-1998 (nanpr01)
**		Added message to edit the striped location.
**	31-aug-98 (mcgem01)
** 		Add missing %s (product name) to S_ST056B_invalid_calicense.
**      03-Sep-98 (fanra01)
**              Add messages for ice server startup
**	 1-Oct-98 (gordy)
**		Add menu item for installation registry protocols.
**	11-mar-1999 (hanch04)
**		Add ignore flag to ingbuild help.
**	17-mar-1999 (mcgem01)
**		Change references to I&O Guide to Getting Started.
**	30-mar-1999 (somsa01)
**		Added more localization strings.
**	19-apr-1999 (cucjo01)
**		Made Starting and Stopping messages consistent.
**	20-apr-1999 (somsa01)
**		Changed S_ST0580_iisystem_not_set to take out "_SYSTEM".
**	03-jun-1999 (mcgem01)
**		The VCBF tree view requires plural component names.
**	09-jun-1999 (somsa01)
**		Added S_ST063A_bad_srv_class.
**	14-jun-1999 (somsa01)
**		Added S_ST063B_indv_proc_check.
**	22-jun-1999 (mcgem01)
**		Clean up clashing message identifiers in previous submission.
**      16-Mar-99 (hanal04)
**              Added S_ST0449_no_cache_pg_default to warn user if the DMF
**              cache has not been enabled for the default_page_size. b95887.
**	02-aug-1999 (cucjo01)
**		Added VCBF tree view plural component for Bridge Servers
**	04-aug-99 (kinte01)
**		Added startup configuration and failure messages for 
**		the RMS (98235) and RDB (98243) gateways.
**  12-Aug-1999 (rodjo04) bug 98353
**      Added S_ST0542_derived_recovery for the derived recovery server
**      title screen.
**	03-nov-1999 (somsa01)
**		Added S_ST0542_cannot_run_rcpstat2 for ingstop on NT.
**      04-nov-1999 (mcgem01)
**              Renumbered E_ST0542 to avoid a conflict with the oping20 
**		code-line.
**	20-jan-2000 (hanch04)
**		Updated S_ST039C_log_size_prompt and S_ST039E_log_too_small
**		to include number of partitions and size.
**	22-jan-2000 (somsa01)
**		Added messages for starting/stopping the Ingres service.
**	27-jan-2000 (somsa01)
**		With S_ST0655_start_svc_fail, print out the OS message text.
**	28-jan-2000 (somsa01)
**		With S_ST0653_stop_svc_fail, print out the OS message text.
**	02-feb-2000 (somsa01)
**		Changed wording of S_ST0539_starting_rmcmd.
**      22-feb-2000 (rajus01)
**		Added S_ST0656_jdbc_server, S_ST0657_configure_jdbc
**		S_ST0658_jdbc_parameters, S_ST0659_starting_iijdbc,
**		S_ST0660_jdbc_failed.
**	30-mar-2000 (somsa01)
**		Added ingstop-specific messages for the JDBC Server.
**	16-may-2000 (cucjo01)
**		Added VCBF tree view plural component for JDBC Servers.
**	07-jun-2000 (somsa01)
**		Modified E_ST056A, E_ST0177, E_ST017E for multiple
**		products.
**	27-dec-2000 (somsa01)
**		Slightly modified S_ST0543_cannot_run_rcpstat2.
**      12-Jan-2001 (hanal04) SIR 102190
**              Added E_ST0187_usage_1 and E_ST0188_usage_2 for iisubset.
**	22-Jan-2001 (horda03) Bug 103736
**		Added S_ST044A_init_resource and S_ST044B_added_resource
**		for IIINITRES application; used to add new resources to
**		CONFIG.DAT.
**	12-Sep-2001 (gupsh01) 
**		Added error E_ST0189_response_write.
**	19-Oct-2001 (gupsh01)
**		Added error E_ST0190_bad_resp_op.
**	01-Nov-2001 (gupsh01)
**		Modified the error message E_ST0187_usage_1. The -exresponse and
**		-mkresponse options were incorrectly added they should be added to 
**		E_ST017E_usage_2 error message. 
**	11-Dec-2001 (gupsh01)
**		Added new error message E_ST0191_response_exists. Formatted the 
**		usage message for ingbuild.
**	22-Feb-2002 (toumi01)
**		Added new error message S_ST0672_shared_mem_in_use; used to
**		warn of ingstart attempt with running server processes.
**      17-Mar-2002 (wansh01) 
**              Added S_ST044C_configure_net_prot,
**              S_ST044D_net_protocols_parm,S_ST044E_registry_proto_parm,
**              S_ST041A_add_vnode_menu,S_ST041B_del_vnode_menu, 
**              S_ST041C_bridge_vnode_menu,S_ST041D_configure_bridge_prot,
**              S_ST041E_bridge_protocols_parm. 
**	29-May-2002 (xu$we01)
**		Modified the error message E_ST0177_usage_1 and
**		E_ST017E_usage_2 for undisplayed variables by changing
**		"%0cbuild" to "%sbuild".
**	13-Jun-2002 (mofja02) 
**		Added S_ST0673_starting_db2udb to S_ST0681_stop_db2udb
**		for the new DB2 UDB Gateway.
**      14-nov-2002 (chash01)
**       add followings for rms gateway confguration.
**          S_ST0544_derived_rms            "Derived RMS Server Parameters"
**          S_ST0545_config_rms_caches      "CBF - Configure RMS Server Caches"
**          S_ST0546_rms_caches             "RMS Server Caches"
**	21-Feb-2003 (wansh01)
**		Added following for das server configuration. 
**		S_ST_0690_das_server thru S_ST069D_das_protocols_parm. 
**	12-Mar-2003 (hanje04)
**	    Reformat S_ST0533 so that it fits properly in VCBF message box.
**	15-Apr-2003 (wansh01)
**		Changed "DAS" to "Data Access Server". 
**	22-May-2003 (drivi01)
**	    Added S_ST0682_not_derived_resource error for DAR #110244 when
**	    a parameter that is not derived can not be protected.
**      28-Oct-2003 (hanal04) Bug 111182 INGCBT500
**          Added S_ST0683_menu_item_invalid.
**	12-Feb-2004 (hweho01)
**	    Added error messages for remote API.
**	10-aug-2004 (penga03)
**	    Added an error message S_ST057E_shutdown_sucessfully.
**	11-aug-2004 (penga03)
**	    Corrected message S_ST057E_shutdown_sucessfully.
**	13-aug-2004 (penga03)
**	    Corrected message S_ST0519_system_running.
**	19-Aug-2004 (hanje04)
**	    Add S_ST06B0_no_root_start for trying to run ingstart as root.
**	02-nov-2004 (devjo01)
**	    Correct spelling error in E_ST0177_usage_1 (encounted).
**	04-Nov-2004 (bonro01)
**	    Update messages E_ST0107_tar_failed & E_ST0201_tar_failed for pax.
**	28-Sept-2004 (lakvi01)
**	    Added messages for inconsistent log file sizes in CBF.
**	02-nov-2004 (devjo01)
**	    Correct spelling error in E_ST0177_usage_1 (encounted).
**	04-Nov-2004 (bonro01)
**	    Update messages E_ST0107_tar_failed & E_ST0201_tar_failed for pax.
**	15-Nov-2004 (drivi01)
**	    Added messages S_ST06B6_online_successfully, S_ST06B7_online_fail
**	    S_ST06B8_offline_successfully and S_ST06B9_offline_fail for 
**	    ingstart and ingstop.
**	15-Apr-2005 (bonro01)
**	    Min and Max values are now i8 variables.
**	07-Feb-2005 (fanra01)
**	    Sir 113881
**	    Merge Windows and UNIX sources.
**	    Define UNIX equivalent error messages.
**	11-Apr-2005 (devjo01)
**	    Added S_ST05A8_command_line_too_long, S_ST05A9_local_node.
**	15-Apr-2005 (bonro01)
**	    Min and Max values are now i8 variables.
**	31-may-2005 (abbjo03)
**	    Remove Licensed? column from E_ST0179.
**	13-Jun-2005 (bonro01)
**	    Update ingbuild usage messages E_ST0177 and E_ST017E
**	13-Jul-2005 (drivi01)
**	    Added a new error S_ST06BA_gcn_wait_fail.
**	02-aug-2005 (devjo01) b114975
**	    Change "Cluster Server process" to "Cluster Support", as we
**	    no longer have a separate CSP.
**	08-aug-2005 (penga03)
**	    Update S_ST06B9_offline_fail to return error code.
**  	17-Oct-2005 (fanra01)
**          Sir 115396
**      Add S_ST06BB_invalid_instance S_ST06BC_instance_read
**	20-nov-2005 (devjo01) b115591
**	    Change text for S_ST0504_remote_dmfcsp_waiting, and
**	    S_ST0505_remote_dmfcsp_online.
**	27-Mar-2006 (bonro01)
**	    Add -acceptlicense to ingbuild syntax. 
**	10-Apr-2006 (hweho01)
**	    Add error msg E_ST0210_no_maching_dbms for spatial package  
**	    installation.  
**      11-Dec-2006 (bonro01)
**          Disable the license prompt yet again. (third time).
**	    Remove -acceptlicense from ingbuild syntax.
**	05-Jan-2007 (bonro01)
**	    Change Getting Started Guide to Installation Guide.
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package messages.
**      9-May-2007 (hanal04) Bug 117075
**          Added E_ST0192_response_not_found.
**	19-Jul-2007 (bonro01) Bug 118202
**	    Changed minimum transaction log size to 32MB
**	25-Jul-2007 (drivi01)
**	    Updated a few error messages to provide clearer
**	    error message on Vista.
**	04-Dec-2007 (drivi01)
**	    Add a warning for ingstart to be shown on Vista
** 	    when Ingres is being started without service flag.
**	11-Feb-2008 (bonro01)
**	    Remove separate Spatial object install package.
**	    Remove error E_ST0210_no_matching_dbms
**	19-May-2008 (whiro01)
**	    Added S_ST06BD_one_server_running and S_ST06BE_many_servers_running
**	    for "ingstop -check" report.
**      16-Oct-2008 (Ralph Loen) SIR 120457
**          Bumped up S_ST06BD, S_ST06BE and S_ST06BF to S_ST06BF, S_ST06C0 and
**          S_ST06C1, resectively, to resolve potential cross-integration
**          issues from the ingres2006r3 code line.
**	18-Feb-2009 (whiro01) Bug 121679
**	    Added S_ST06C2 to report invalid connect_id strings in ingstop.
**      08-Nov-2008 (Ralph Loen) Bug 120471 
**          In S_ST0684_increase_file_limit, change the term "hard limit" to
**          "soft limit". 
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added S_ST06C3_mustlogDB_menu item to DBMS config.
**	16-Oct-2009 (smeke01) b122732
**	    Added S_ST045B_protected_cpi_prompt & S_ST045C_protected_cpi_small.
**	10-Dec-2009 (bonro01)
**	    Enable the license prompt yet again (fourth time).
**	02-Apr-2010 (thaju02) Bug 123528
**	    Added S_ST0408_invalid_cache_name.
*/

# define        _ST_CLASS       212

/*
****************  Messages for netutil *****************
*/

F_ST0001_list_choices   "ListChoices"

/*
**  Creating a new vnode (numain.sc)
*/

E_ST0001_vnode_exists   "Vnode '%0c' is already defined.
Choose 'Edit' to modify the definition."
S_ST0002_enter_vnode    "Enter new virtual node name: "

/*
**  The words "private" and "global" are used all over the place, for
**  example in tables.  The first letter of each word is used as an
**  internal flag by the code, so they _must_ begin with different letters.
*/

S_ST0003_global         "Global"
S_ST0004_private        "Private"

/*
**  Entries in the "scope" field (numain.sc)
*/

S_ST0005_any_user       "Any user on %s"
S_ST0006_user_only      "User %s only"


E_ST0007_cannot_delete 
"Unable to remove '%0c'.\n\nThis product is needed by '%1c'."

/*
**  Error messages during edits (nuedit.sc)
*/

E_ST000B_p_or_g         
"Please enter 'P' for private, or 'G' for global."

E_ST000C_net_address_req        "A network address is required."
E_ST000D_protocol_req           "A network protocol name is required."
E_ST000E_listen_req             "A listen address is required."

E_ST000F_duplicate_conn "Duplicate connection entry.\nTo modify
the definition, use 'Edit'."

/*
**  Messages in confirmation prompt box for "destroy" (numain.sc)
*/

S_ST0010_all_data       "all data for vnode"
S_ST0011_conn_entry     "connection entry"
S_ST0012_auth_entry     "login/password entry"
S_ST0013_inst_pwd       "installation password"

/*
**  Informational headers for connection edit/entry form (nuedit.sc)
*/

S_ST0014_define_new_conn        "Enter new connection"
S_ST0015_edit_conn              "Edit connection entry"

/*
**  We have both a global and a private authorization already (numain.sc)
*/

E_ST0016_cannot_create  
"No more login/password entries may be created for vnode '%0c'.\nChoose
'Edit' to modify existing entries."

/*
**  Trying to delete an entry that isn't there???  Shouldn't happen (numain.sc)
*/

E_ST0017_no_conn        "Can't locate connection entry."

/*
**  Not enough rows to do whatever you're trying to do (numain.sc)
*/

E_ST0018_no_vnode_rows  "There are no vnode names defined."
E_ST0019_no_conn_rows   "There are no connection entries defined."
E_ST001A_no_auth_rows   "There are no login/password entries defined."

/*
**  Informational headers for authorization edit/entry form (nuedit.sc)
*/

S_ST001B_define_new_auth "Enter new login/password"
S_ST001C_edit_auth       "Edit login and password"

/*
** Renaming a vnode (numain.sc)
*/

S_ST001D_new_name_text   "Enter the new name for vnode '%s'"
S_ST001E_new_name_prompt "New name:"

/*
**  Error on startup (numain.sc)
*/

E_ST001F_cant_connect   "Cannot connect to specified vnode: '%0c'"
E_ST003E_cant_connect   "Cannot connect to name server on local host: '%0c'"

/*
**  Network manager can create either private or global vnode (numain.sc)
*/

S_ST0020_choose_vnode_type      "Choose type of vnode/login to be created"

/*
**  Assorted error messages (numain.sc)
*/

E_ST0024_cant_update_db         
"Unable to update name server database.\nReturn
code: %1x\nLocation indicator: %0d\n\nPlease report this problem to
Ingres Corporation Technical Support."

E_ST0025_cant_destroy_vnode
"Network manager only.\nThe virtual node definition for '%0c' contains
global entries.  It may only be destroyed by a %s network manager."

E_ST0026_cant_destroy_global
"Only an %s network manager may destroy a global entry."

E_ST0027_cant_edit_global
"Only an %s network manager may edit a global entry."

E_ST0028_cant_edit_vnode
"Network manager only.\nThe virtual node definition for '%0c' contains
global entries.  It may only be edited by an %s network manager."

/*
**  Entering and confirming passwords (nuedit.sc)
*/

S_ST0029_confirm_pw             "Confirm password by re-entering."
E_ST002C_must_enter_pw          "You must enter a password."

E_ST002D_must_confirm_pw        
"You must confirm the password by re-entering it."

E_ST002F_password_mismatch
"Password does not match confirmation.\nPlease re-enter and re-confirm."

S_ST0030_enter_pw       "Enter %s password"
S_ST0031_enter_inst_pw  "Enter %s installation password"

E_ST0032_cant_create_global
"Only an %s network manager may create a global entry.\nChoose
'Edit' to modify the existing private entry."

S_ST0033_password_wont_echo  "(The password will not echo)"

/*
**  Messages from the "test" function (numain.sc)
*/

S_ST0034_connection_ok   "Connection to name server on '%s' was successful."
E_ST0035_connection_failed
"The connection to the name server on '%s' has failed.\n\n"
S_ST0036_reason_unknown      "Unable to look up error message; status = %x"

/*
**  Tags for userinfo field on main form (numain.sc)
*/

S_ST0037_user_tag            "Local user"
S_ST0038_netmgr_tag          "Network manager"

/*
**  Error messages returned from GCA requests (nuaccess.c)
*/

E_ST0050_gca_error           "GCA error returned by server"
/*
**  Command file parsing/processing (nufile.c)
*/

E_ST0021_create_login_failed "Failed trying to create login entry\n"
E_ST0022_create_conn_failed  "Failed trying to create connection entry\n"
E_ST0023_destroy_login_failed "Failed trying to destroy login entry\n"
E_ST002A_destroy_conn_failed "Failed trying to destroy connection entry\n"
E_ST0039_cant_open_file      "Cannot open file: %s\n"
E_ST003A_general_syntax      "General syntax error"
E_ST003B_unrecognized_token  "Unrecognized word: %s"
E_ST003C_insuff_data         "Insufficient data"
E_ST003D_too_much_data       "Too many data items"

/*
**  Command line error (temporary until we start using utexe.def)...
*/

E_ST0040_cmdline_usage  
"Correct syntax:\n    netutil [-uuser] [-vnode vnode] [-file file,file...]"

/*
**  Protocol listpick prompt:
*/

S_ST0041_protocol_prompt
"Select a protocol from this list.  If the\ndesired protocol does not
appear in the list,\nyou may type the protocol name directly into\nthe
field."

/*
**  Menu items for interactive control form...
*/

F_ST0042_control_menuitem       "Control"
F_ST0043_quiesce_menuitem       "Quiesce"
F_ST0044_quiesce_all_menuitem   "QuiesceAll"
F_ST0045_stop_menuitem          "Stop"
F_ST0046_stop_all_menuitem      "StopAll"
E_ST0047_no_net_servers         "No network servers active."
S_ST0048_really_do_it           "Really %s network server %s?"
S_ST0049_really_do_all          "Really %s all network servers?"
S_ST004A_dont_do_it             "Don't "
S_ST004B_do_it                  "%s"
F_ST004C_quiesce_word           "quiesce"
F_ST004D_stop_word              "stop"
E_ST004E_shutdown_err           "Unable to %0c network server %1c."

/*
****************  Messages for ingbuild *****************
*/

F_ST0100_install_menuitem       "Install"
F_ST0101_current_menuitem       "Current"
F_ST0102_setup_menuitem         "Setup"
F_ST0103_yes                    "Yes"
F_ST0104_no                     "No"
F_ST0105_setup_all_menuitem     "SetupAll"
F_ST0106_describe_menuitem      "GetInfo"
S_ST0106_xxx			""
F_ST0107_verify_menuitem        "VerifyAll"
F_ST0108_help_menuitem          "Help"
F_ST0109_end_menuitem           "End"
F_ST010A_quit_menuitem          "Quit"
F_ST010B_delete_menuitem        "Remove"
F_ST010C_custom_menuitem        "CustomInstall"
F_ST010D_express_menuitem       "ExpressInstall"
F_ST010E_package_menuitem       "PackageInstall"

E_ST0101_device_required        "A distribution medium is required."

E_ST0104_no_pkgs_selected
"No products were selected.\n\n
To install a product, type 'y' in the 'Install?' column and select
'Install'."

S_ST0105_installing
"Reading from distribution medium... (This may take a few minutes)"

E_ST0107_tar_failed		"pax failed with exit code %d\n"

E_ST0108_install_failed
"An attempt to install from the distribution medium failed.\n\n
Please confirm that the distribution device has been mounted correctly,
the device name is valid, that there is adequate space on the target
device, and that you are user %0c prior to running this step.
If you are unable to resolve the problem, please contact
Ingres Corporation Technical Support."

E_ST0178_installation_is_up
"The %0c installation at\n\n  %1c\nis currently active.  Installation
cannot proceed."

S_ST0109_installation_is_up
"Warning: the %s installation at\n\n  %s\n\nis currently active.
Installing new executables\nover a running system is extremely
dangerous, and can have\nunpredictable results.\n\nYou may continue
with the next phase of the installation, in\nwhich the distribution
files wil be read from the distribution\nmedium, but not copied into
place.  You will receive another\nwarning similar to this one before
any files are overwritten\nin the installation area."

S_ST010A_continue       "Continue"
S_ST010B_abort          "Abort"

S_ST010C_installation_still_up
"Warning: the %s installation at\n\n  %s\n\nis still
active.  Installing new executables\nover a running system
is extremely dangerous, and can result\nin loss of transaction
integrity.\n\nPlease shut %s down and continue, or
abort\nthe installation."

S_ST010D_installation_is_down
"Advisory: the %s system at\n\n  %s\n\nappears to be shut
down.  Before continuing, please confirm\nthat no executables are
running in this installation."

S_ST010E_moving_files           "Moving files for product: %s..."

E_ST0110_parse_failed		"\nSyntax error at line %s of %s\n\n"

E_ST0111_cant_create_dir
"Trouble creating directory %s.  Return code: %d"

E_ST0112_cant_delete_dir
"Unable to delete directory.  Return code: %d"

S_ST0113_for_help_key           "For help, press the key labeled '%s'."

S_ST0114_for_help_menu
"For help, press the key labeled '%s', and then type 'help'."

S_ST0115_verifying_file         "Verifying: %s . . ."

E_ST0116_cant_verify_file
"Unable to verify: %0c\n
   Feature name: %1c\n
      Directory: %2c\n
           File: %0c\n
\nA file could not be verified.  The file either has incorrect 
permissions, or it is missing."

E_ST0117_license_required
"II_AUTH_STRING must be defined in order to complete the requested
operation."

S_ST0118_many_verify_errors
"There are many verification errors.\n\nDo you want to
verify any more files?"

S_ST0119_continue_inst          "Install all selected products"
S_ST011A_abort_inst             "Cancel requested installation"

S_ST011B_continue_ver           "Verify the remaining files"
S_ST011C_abort_ver              "Do not verify any more files"

E_ST011D_ver_errors
"Verification errors were detected.\nOne or more files in your installation
failed to pass verification.  Please retry installing from the
same distribution medium.  If this does not the problem, please
contact Ingres Corporation Technical Support."

E_ST0120_no_such_device
"Device does not exist.\n\nThe specified distribution device (%0c) does
not exist.  Please enter either a device name or the full pathname of
a disk archive containing the distribution."

S_ST0121_no_install_dir
"Warning: the specified installation directory\n\n  %s\n\ndoes not
exist.  Shall I attempt to create it?"

S_ST0122_yes_create     "Create the installation directory"
S_ST0123_dont_create    "Do not create the directory"

E_ST0124_cant_create_instdir
"Unable to create installation directory.\nAn attempt to create
installation directory '%0c' failed.  This is probably due
to incorrectly set permissions.  Please make sure that the path it is
located in exists, and is writable by user '%1c'.  Rerun this program
when the directory has been created.\n\nIf you are unable to resolve
this problem, please contact Ingres Corporation Technical Support."

S_ST0125_continue_setup_prompt
"A setup program failed to complete successfully.\n\n
You have the option of continuing to execute the remaining setup\n
program(s), although note that this may have unexpected results\n
due to dependencies which may exist between installed products.\n\n
Do you want to continue executing setup programs?"

E_ST0126_cant_create_dir
"Unable to create directory: (%0c).  This is probably due to incorrectly
set permissions.  Please be sure that the installation area is writeable
by user '%1c'.\n\nIf you are unable to resolve this problem, please contact
Ingres Corporation Technical Support."

S_ST0127_ver_ok_prompt
"All requested products have been successfully installed.\n
One or more of these products requires setup.\n\n
Do you want to set up these products now?"

F_ST0128_do_setup       "Execute setup programs now"
F_ST0129_dont_do_setup  "Execute setup programs later"

S_ST012A_no_setup_now
"You have chosen not to execute the setup programs for some product(s)\n
you have installed.  Note that these products must be set up before\n
they can be used."

S_ST012B_setup_ok
"All installed products are now available for use."

S_ST011E_setup_ok
"All installed products are now available for use.\n\n 
Once you resolve the problem(s) which caused the failure of the\n
pre-transfer validation program(s), try reinstalling the affected\n
product(s).  If you are unable to resolve these problems, please\n
contact Ingres Corporation Technical Support."

E_ST012C_setup_failed
"One or more setup programs failed to complete successfully.\n\n
The error messages displayed when the error(s) occurred should have
indicated the nature of the problem(s).  Once you have resolved any 
problems that occurred, you will need to re-run the setup program(s)
by choosing 'Setup' or 'SetupAll' from the menu.\n\n
If you are unable to resolve any setup problems, please contact 
Ingres Corporation Technical Support."

E_ST012D_no_setup_program
"Can't find setup program.\n\n
 Program: %0c\n     Part: %1c\n  Product: %2c\n\nA
setup program cannot be found.  Please contact Ingres Corporation 
Technical Support."

E_ST012E_setup_program_error
"A product setup program failed to complete.\n\n
 Program: %0c\n     Part: %1c\n  Product: %2c\n\nThis program terminated
prematurely - either because you chose to abort or because of an
error.  If an error occurred and you are unable to resolve the problem,
please contact Ingres Corporation Technical Support."

S_ST012F_ver_error_prompt
"One or more images failed to pass verification.  The next phase\nof the
installation process will involve extracting files from these\nimages;
therefore, it is strongly recommended that you resolve\nthe verification
problems before continuing with the next phase.\nTry re-installing from
the same distribution medium, and if this\nfails to solve the problem,
contact your Ingres Corporation sales representative.\n\nIf you want to 
attempt to extract files from the images despite the\nverification errors, 
you may choose to proceed with the installation.\nDo you want to do this?"

E_ST0130_enter_y_or_n   "Please enter 'Yes' or 'No' in this field."

S_ST0131_preload_failed
"A pre-transfer validation program failed for:\n\n  %s\n\nIt is
highly advisable to correct the condition that\ncaused the validation
failure.  However, you may choose\nto ignore the failure and transfer
the product into the\ninstallation directory.  Would you like to do this?"

S_ST0132_do_transfer    "Transfer the product"
S_ST0133_dont_transfer  "Do not transfer the product"

E_ST0134_no_preload_prog
"Can't find pre-transfer validation program.\n\n  Program:
%0c\n     Part: %1c\n\nA pre-transfer validation program cannot be 
found.  Please contact Ingres Corporation Technical Support."

E_ST0135_preload_prog_err
"Error detected by pre-transfer validation program.\n\n  Program:
%0c\n     Part: %1c\n\nA pre-transfer validation program has detected
a fatal error condition.  If you are unable to resolve this problem,
please contact Ingres Corporation Technical Support."

F_ST0136_installed_tag          "Ready"
F_ST0137_no_setup_tag           "Not Set Up"
F_ST0138_expired_tag            "Expired"

S_ST0139_writing_pkl            "Writing installation descriptor file..."

E_ST013A_write_error
"Can't write installation descriptor.\n\nAn error has occurred while writing
your installation descriptor files.  This is probably due to a lack of
disk space on the device where the installation directory is located."

E_ST013B_no_such_host
"Host is unknown or unreachable.\n\nThe specified remote host (%0c)
cannot be reached from this system.  Please make sure you have entered
the remote host name correctly.  If you have, confirm that user '%1c'
has trusted status when accessing host '%0c' from host '%2c'.  If you are
unsure of how to establish trusted status, please contact your network
or system administrator."

E_ST013C_no_such_remote_dev
"Remote device does not exist.\n\nThe specified distribution device (%0c)
does not exist on host '%1c'.  Please enter either a valid device name or the
full pathname of a disk archive containing the distribution."

S_ST013D_confirming_host        "Checking access to remote host..."
S_ST013E_confirming_dev         "Checking access to remote device..."

E_ST013F_cant_transfer_pkg
"Unable to transfer product.
\n\nAn error occurred while attempting to transfer '%0c'
to the installation area.  This is probably due to insufficient disk
space.\n\nTry reinstalling when sufficient disk space is available."

E_ST0140_no_manifest
"The %0c file cannot be found.  The file should be located
in the same directory as this installation utility.  Please attempt to
resolve this problem by re-reading the installation utility and its
associated files from the distribution medium; see the Installation
Guide for the exact format of the archive command you will need
to use.  If you are unable to resolve this problem, please contact your
Ingres Corporation sales representative."

E_ST0141_bad_search_path
"Your Unix command search path is not set correctly.  Please refer to
the Installation Guide for instructions on how to set your Unix
command search path before re-running %sbuild."


E_ST0142_no_inst_directory
"Installation directory does not exist.\nThe specified installation
directory\n\n  %s\n\ndoes not exist.  To specify a different installation
directory, exit from this utility, and change the value of the environment
variable %s."

E_ST0143_no_pkgs_req_setup
"No products require setup.\n\nAll installed products in this installation
have been set up successfully."

S_ST0144_distribution		"distribution"

S_ST0145_installation		"installation"

S_ST0146_no_setup_reqd          "'%s' does not require setup."

S_ST0147_really_delete
"Do you really want to remove all files associated \n'%s' from your system?"

F_ST0148_dont_delete            "Do not delete the product."
F_ST0149_delete                 "Delete all files in the product."

S_ST014A_press_return           "Press RETURN to continue:"

S_ST014B_installation_is_up
"Warning: the %s server at\n\n  %s\n\nis currently active.  It is
highly inadvisable to delete\nfiles from a running system.  Unless you
are extremely sure\nthat the files which will be deleted are not in
current use\nby the system, please do not proceed with this operation
until\nyou have taken the system down.\n\n Do you really want to delete
all files associated with \n'%s'?"

S_ST014C_deleting_package	"Removing selected product..." 

E_ST014D_delete_errs
"Errors detected during product removal.\n\nThe attempt to delete all files
associated with '%0c' was unsuccessful.  This is probably
due to incorrect permissions.  Please be sure that user '%1c' has write
permission on the '%2c' directory.\n\nThis product
is now in an undefined state; it is unlikely to be usable in production,
but some of its files may remain on the installation area."

S_ST014E_undefined_tag          "Undefined"

E_ST014F_no_delete_prog
"Can't find delete utility program.\n\n  Program: %0c\n     Part: %1c\n\nA
delete utility program cannot be found.  The product cannot be deleted
without this program.  Please contact Ingres Corporation Technical Support."

E_ST0150_delete_prog_err
"Error detected by delete utility program.\n\n  Program: %0c\n     Part:
%1c\n\nA delete utility program has detected a fatal error condition.
The product cannot be deleted.  If you are unable to resolve this problem,
please contact Ingres Corporation Technical Support."

E_ST0152_rename_error
"Rename error on descriptor file.\nThe temporary installation descriptor
file (T%0c) could not be renamed as '%0c'.  The file is located in the
installation directory (%1c).  You will need to do the rename manually
before you can use the 'Current' screen in this program."

E_ST0153_write_error
"Write error on descriptor file.\nA write error occurred while trying to
write a temporary version of the installation descriptor file (T%0c).
This is probably due to a lack of disk space.  The former descriptor
file (%0c) in directory '%1c' has not been altered."

E_ST0154_mem_alloc_error
"Memory allocation error.\nMemory allocation failed with return code %0d.
Please report this condition to Ingres Corporation Technical Support."

E_ST0155_file_open_error
"Cannot open file.\nUnable to open file '%0c' in directory '%1c'."

E_ST0156_install_aborted
"Installation of selected products aborted."

F_ST0157_verifying_pkg          "Verifying product: %s..."

F_ST0151_verifying_supp_pkg     "Verifying support module: %s..."

E_ST0158_cant_unload
"Cannot unload product.\nAn attempt to unload '%0c' into a
temporary directory was unsuccessful.  The verification phase is aborted."

E_ST0159_bad_user_id
"Incorrect user: %0c\nYou must run this utility as: '%1c'."

S_ST015A_moving_supp_files	"Moving files for support module: %s..."

E_ST015B_verification_error
"Verification error on file: %0c\n\n
      Directory: %1c\n\n
       Checksum: %2d should be: %3d\n\n
     Byte count: %4d should be: %5d\n
\nThis may indicate a defect in the distribution medium.  This can also occur 
if you specified the wrong %6c distribution."

S_ST015C_verification_failed
"One or more files failed to pass verification."

S_ST015D_verification_ok
"All installed files have passed verification."

S_ST015E_ver_err_prompt
"An image verification error has occurred.\n\nWhat would you like to do?"

S_ST015F_pkg_is_installed
"The following product is already installed on your system:\n\n
    Product: %s\n
   Revision: %s\n\nDo you want to re-install it?"

S_ST0160_do_install     "Install the product"
S_ST0161_dont_install   "Do not install the product"

S_ST0162_dep_prompt
"'%s' requires '%s',\n
but it has not been selected for installation.\n\n
Please select one of the following options:"

S_ST0163_select         "Install '%s'"
S_ST0164_deselect       "Cancel installation of '%s'"

S_ST0165_no_more_pkgs
"There are no more products in this installation.\n\n
Returning to main menu."

S_ST0166_no_description
"No description available for '%s'."

S_ST0167_install_popup_1
"Products selected: %2d     Disk space required: %s\n"

S_ST0168_install_popup_2
" Support modules : %2d                          %s\n
                  --     Temporary storage  : %s\n
                                               ---- \n
           Total: %2d                          %s\n"

S_ST0169_install_popup_3
"\nDo you want to proceed with the installation?"

S_ST016A_term_explanation
"Please identify the type of terminal (or terminal
emulation software)\nyou are using by entering its %s
termcap name (e.g. 'vt100f' for a\nVT100 with a function
key pad).  If you do not know the termcap name for\nyour
terminal, press return to see a list of the available
termcap names.\n"

S_ST016B_enter_term     "\nEnter termcap name, or press return for a list: "

S_ST016C_menu_prompt_a  "To select a menu item, "
S_ST016D_menu_prompt_b  "press the key whose label appears in parenthesis"
S_ST016E_menu_prompt_c  "after the name, or "
S_ST016F_menu_prompt_d  "press %s and type the name of the menu item."

S_ST0170_term_list_prompt
"\nPress return again for a short list of the most common
termcap\nnames, or type  ?  followed by return for a complete list: "

E_ST0171_no_term_list
"Unable to display termcap list -- couldn't open termcap file.\n"

S_ST0172_pause_prompt
"Press return to continue, or type  s  and return to stop list: "

E_ST0173_authstr_required
"An authorization string is required.\n\nPlease enter the authorization
string you received with your distribution.  You must have a valid
authorization string in order to install %0c and its related
products.  If you do not have a valid authorization string, please
contact your Ingres Corporation sales representative."

E_ST0174_authstr_bad
"This authorization string is invalid.\n\nThe authorization string
you entered is unreadable.  Carefully re-enter the authorization
string you received with your product distribution.  You must have a
valid authorization string in order to install %0c and its
related products.  If you do not have a valid authorization string,
please contact your Ingres Corporation sales representative."

E_ST0175_dependency_recursion
"Dependency recursion error.\nExcessive dependency recursion was detected
in the %0c file.  Please report this problem to Ingres Corporation Technical
Support.  Installation cannot proceed."

S_ST0176_dep_prompt
"'%s' has been selected\n
for installation.  This product requires that\n
a newer version of '%s'\n
than the one you have installed (%s) 
be installed.\n\n
Please select one of the following options:"

E_ST0177_usage_1
"\n
Usage:  %sbuild [ flags ] [ <distribution-medium> ]\n\n
Flags: -all              Install all products.\n
       -install=<list>   Install all products in the comma-separated\n
                         list of product names.\n
       -products         Display a list of the available products.\n
       -version[=<list>] Display the version number of the installation\n
                         or of each product in the comma-separated list.\n
       -express          Run setup programs using default values.\n
       -ignore           Ignore any errors encountered during file copy.\n
       -nosetup          Don't run setup programs.\n
       -acceptlicense    Accept Ingres License, bypass License prompt.\n"

S_ST0179_pkg_list_header
"	Feature name      Product\n
	------------      ------------------------------\n"

E_ST017A_bad_featname   "Unrecognized feature name: %0c"

E_ST017B_not_present    
"Not present on this installation: %0c"

S_ST017C_dep_prompt
"\n'%s', which is required by \n'%s',
will be installed automatically.\n"

E_ST017D_cant_open_file "Can't open file for output: %0c"

E_ST017E_usage_2
"	-mkresponse       Create a response file, containing installation\n
                         parameters, without actually installing.\n
       -exresponse       Read installation parameters from a \n
                         response file.\n
       -file=<filename>  Specify filename for -mkresponse and -exresponse.\n
                         The default filename is ingrsp.rsp\n\n
If only -mkresponse, -exreponse or no command line arguments are specified,\n
%sbuild will run in interactive forms mode.\n
If a distribution medium is specified, the install will run in batch mode.\n
A distribution medium is required with -all, -install, -express, -ignore, and\n
-nosetup; if a distribution medium is provided with no flag, -all is assumed.\n\n"

E_ST017F_usage_1
"\n
Usage: iimaninf [ flags ]\n\n
Flags: -all                     List all packages described in the file\n
       -custom                  List custom packages described in the file\n
       -packages                List express packages described in the file\n
       -version[=<list>]        Display versions of all pkgs in the list, or\n
                                version ID of the rel if no list specified.\n
       -output=<file>           Direct output to the named file.\n
       -describe=<list>         Display descriptions of pkgs in the list.\n
       -resolve=<list>          Display all files which need to be installed\n
                                for the specified list of pkgs.\n
       -setup=<list>            Display all setup programs which need to be\n
                                run for the specified list of pkgs.\n
       -preload=<list>          Display all pre-transfer progs which need to\n
                                be run for the specified list of pkgs.\n"

E_ST0180_usage_2
"        -installation            Read the installation descriptor file.\n
       -location=<path>         Specify the location of the file to be parsed.\n
       -authorization=<authstr> Validate specified authorization string.\n
       -directories=<list>      Display a list of all dirs which will be used\n
                                by files associated with specified products.\n
       -unique=<list>           Display all files which need to be installed\n
                                for the specified list of packages.\n
       -permission=<list>       Displays directory, file, and file protection.\n\n
Command-line flags may be abbreviated to the smallest unique left\n
substring.  In the above descriptions, <list> represents a comma-separated\n
list of product featurenames, with no embedded whitespace.\n\n"

E_ST0181_preserve_failed
"Unable to preserve existing version(s) of user customizable file(s).
This installation procedure will be aborted."

S_ST0182_preserving_files	"Preserving user customizable files..."

S_ST0183_continue_setup       	"Execute remaining setup programs"
S_ST0184_abort_setup		"Abort further setup"

E_ST0185_not_absolute_path
"You must specify an absolute pathname for the %0c distribution
medium."

S_ST0186_xxx			""

E_ST0187_usage_1
"\n
Usage: iisubset [ flags ]\n\n
Flags: -all                     List all packages described in the file\n
       -custom                  List custom packages described in the file\n
       -packages                List express packages described in the file\n
       -version[=<list>]        Display versions of all pkgs in the list, or\n
                                version ID of the rel if no list specified.\n
       -output=<file>           Direct output to the named file.\n
       -describe=<list>         Display descriptions of pkgs in the list.\n
       -resolve=<list>          Display all packages and files which need to\n
                                be installed for the specified list of pkgs.\n
                                Required and optional packages will be\n
                                displayed.\n
       -nolinks                 Stops -resolve from listing link files.\n"

E_ST0188_usage_2
"\n
Command-line flags may be abbreviated to the smallest unique left\n
substring.  In the above descriptions, <list> represents a comma-separated\n
list of product featurenames, with no embedded whitespace.\n\n"

E_ST0189_response_write
"\n
Response file write has failed. Please check if you permissions to access the\n
directories and the drive is not full. \n\n"

E_ST0190_bad_resp_op
"\n
Only one of the flags mkresponse and exresponse flags should be specified. \n\n"

E_ST0191_response_exists
"\n
The specified response file '%0c' already exists, please specify another name 
for the response file. \n\n" 

E_ST0192_response_not_found
"\n
The specified response file '%0c' does not exist, please specify the correct name for the response file. \n\n"

E_ST0201_tar_failed     
"\n\nArchive creation failed due to 'pax' error!\n"

E_ST0202_cant_chmod_file
"Unable to change mode of file.  Return code: %d"

E_ST0203_cp_failed
"\n\nCopy failed due to 'cp' error.  Return code: %d"

E_ST0204_ln_failed
"\n\nCopy failed due to 'ln' error.  Return code: %d"

E_ST0205_chmod_failed
"\n\nCopy failed due to 'chmod' error.  Return code: %d"

S_ST0206_xxx			""

F_ST0207_setting_up_pkg          "Setting up product: %s..."

F_ST0208_setting_up_supp_pkg     "Setting up support module: %s..."

S_ST0209_overlay_size_checksum	"Replacing SIZE/CHECKSUM values for %s"

S_ST020A_overlay_not_found		
"File %s not found to get current SIZE/CHECKSUM"

E_ST020B_tmp_create_chmod
"Unable to create temporary directory: (%0c).\nThis is probably due to
incorrectly set permissions.\n\nIf you are unable to resolve this problem, 
please contact Ingres Corporation Technical Support."

/*%
E_ST0210_no_matching_dbms
"No dbms server version %0c is found in the\ninstallation, spatial package
version %1c \ncan not be installed."

** Use of this message has been discontinued since a separate spatial
** package has been removed.
*/


/*
** cbf messages
*/

S_ST02FE_rmcmd			"Remote Command"
S_ST02FF_internet_comm		"Internet Communication"
S_ST0300_name_server            "Name Server"
S_ST0301_recovery_server        "Recovery Server"
S_ST0302_archiver_process       "Archiver Process"
S_ST0303_logging_system         "Logging System"
S_ST0304_locking_system         "Locking System"
S_ST0305_transaction_log        "Transaction Log"
S_ST0306_xxx                    ""
S_ST0307_dbms_server            "DBMS Server"
S_ST0308_net_server             "Net Server"
S_ST0309_star_server            "Star Server"
S_ST030A_not_configurable       "(not configurable)"
S_ST030B_required               "(required)"
S_ST030C_xxx			""
S_ST030D_xxx			""
S_ST030E_xxx			""
S_ST030F_default_settings       "(default)"

S_ST0310_start_count_prompt
"Please enter the number of copies of the selected component you want
to execute at startup: "

S_ST0311_recalculating          "Recalculating derived parameters..."

S_ST0312_no_ii_system
"%s must be set before the configuration utility is run."

S_ST0313_no_ii_installation
"%s_INSTALLATION must be set before the configuration utility is run."

S_ST0314_bad_config_write
"Unable to write config.dat in the \"files\" directory.\nVerify that you have write privileges to that location."

S_ST0315_bad_config_read
"Unable to read config.dat in the \"files\" directory."

S_ST0316_cbf_syscheck_message       "Checking system resources..."

S_ST0317_syscheck_failure
"Your system does not have sufficient resources to support the change you
just entered.  Do you want to undo the change?"

S_ST0318_fatal_error            "Exiting due to fatal error: %0c"

S_ST0319_unable_to_load
"Unable to load %s"

S_ST031A_form_not_found         "fatal error: form '%0c' not found."

S_ST031B_restoring_settings     "Restoring settings..."

S_ST031C_add_db_prompt
"Please enter the name of the database you want to add: "

S_ST031D_edit_listen_prompt
"Please enter a listen address for the selected protocol: "

S_ST031E_edit_status_menu       "Edit"

S_ST031F_edit_protocol_prompt
"Please enter a configured protocol: "

S_ST0320_bad_protect_read
"Unable to read protect.dat in the \"files\" directory."

S_ST0321_no_independent
"According to the current configuration rules, all parameters for the
selected component are derived from other sources.  You can view or edit
the derived parameters for this component on the following form, although
be advised that directly editing derived parameters is not recommended."

S_ST0322_missing_cache
"The 'cache_name' and/or 'cache_share' resources are missing from this
DBMS server definition.  Unable to open cache definition."

S_ST0323_unable_to_open
"Unable to open %0c for writing. Verify that the file\nexists and you have privileges to write to this file."

S_ST0324_validating             "Validating..."

S_ST0325_constraint_violation
"Configuration Rule Constraint Violation"

S_ST0326_backup_failed
"Internal backup failed - unable to proceed. You may be out of disk space."

S_ST0327_syscheck_disabled
"System resource checking has been disabled since you don't have sufficient
resources to start %s currently."

S_ST0328_value_prompt
"Please enter a new value for the selected resource: "

S_ST0329_reset_prompt
"Do you want to restore '%0c' to its default setting? "

S_ST032A_recalculate_prompt
"Do you want to recalculate the derived parameter '%0c'? "

S_ST032B_resource_protected
"You must turn off protection for this resource before recalculating its
value." 

S_ST032C_undefined_symbol
"The configuration rule symbol for this resource or one it depends on
is undefined."

S_ST032D_restoring_settings     "Restoring settings..."

S_ST032E_create_prompt
"Please enter a unique name (alphanumeric characters and underbars are
allowed - whitespace will be stripped) for the duplicate component
definition: "

S_ST032F_rename_prompt
"Please enter a new name (alpha-numeric characters and underbars are
allowed - whitespace will be stripped) for the selected component
definition: "

S_ST0330_yes                    "yes"
S_ST0331_no                     "no"
S_ST0332_shared                 "shared"
S_ST0333_private                "private"
S_ST0334_owner                  "owner"

S_ST0340_destroy_prompt
"Do you want to destroy the selected component definition?"

S_ST0341_duplicating            "Duplicating component..."

S_ST0342_renaming               "Renaming component..."

S_ST0343_destroying             "Destroying component..."

S_ST0344_selecting_parameters   "Selecting parameters..."      

S_ST0345_selecting_derived      "Selecting derived parameters..."      

S_ST0346_no_help_available	"No help available."

S_ST0347_change_log_title	"Change Log"

S_ST0348_no_change_log		"No change log found."

S_ST0349_page_up_menu		"PageUp"
S_ST034A_page_down_menu		"PageDown"
S_ST034B_top_menu		"Top"
S_ST034C_bottom_menu		"Bottom"
S_ST034D_preferences_menu	"Preferences"
S_ST034E_security_menu		"Security"
S_ST034F_xxx			""

S_ST0350_configure_menu         "Configure"
S_ST0351_duplicate_menu         "Duplicate"
S_ST0352_derived_menu           "Derived"
S_ST0353_reset_menu             "Restore"
S_ST0354_databases_menu         "Databases"
S_ST0355_edit_count_menu        "EditCount"
S_ST0356_cache_menu             "Cache"
S_ST0357_protocols_menu         "Protocols"
S_ST0358_protect_menu           "Protect"
S_ST0359_reformat_menu          "Reformat"
S_ST035A_erase_menu             "Erase"
S_ST035B_disable_menu           "Disable"
S_ST035C_recalculate_menu       "Recalculate"
S_ST035D_scroll_up_menu         "ScrollUp"
S_ST035E_scroll_down_menu       "ScrollDown"
S_ST035F_change_log_menu        "ChangeLog"

S_ST0360_configure_dbms         "CBF - Configure DBMS Server Definition"
S_ST0361_dbms_parameters        "DBMS Server Parameters"
S_ST0362_configure_cache        "CBF - Configure DBMS Cache Definition"
S_ST0363_cache_parameters       "DBMS Cache Parameters"
S_ST0364_configure_net          "CBF - Configure Net Server Definition"
S_ST0365_net_parameters         "Net Server Parameters"
S_ST0366_configure_star         "CBF - Configure Star Server Definition"
S_ST0367_star_parameters        "Star Server Parameters"
S_ST0368_configure_logging      "CBF - Configure Logging System"
S_ST0369_logging_parameters     "Logging System Parameters"
S_ST036A_configure_locking      "CBF - Configure Locking System"
S_ST036B_locking_parameters     "Locking System Parameters"
S_ST036C_derived_dbms           "Derived DBMS Server Parameters"
S_ST036D_derived_cache          "Derived DBMS Cache Parameters"
S_ST036E_derived_net            "Derived Net Server Parameters"
S_ST036F_derived_star           "Derived Star Server Parameters"
S_ST0370_derived_logging        "Derived Logging System Parameters"
S_ST0371_derived_locking        "Derived Locking System Parameters"
S_ST0372_configure_preferences	"CBF - Configure Preferences"
S_ST0373_system_preferences	"Configuration System Preferences"

S_ST0374_startup_not_zero
"The startup count for a component must be 0 in order to destroy it."

S_ST0375_configure_name_server	"CBF - Configure Name Server"
S_ST0376_iigcn_parameters	"Name Server Parameters"
S_ST0377_configure_security	"CBF - Configure Security Auditing System"
S_ST0378_security_parameters	"Security Auditing Parameters"
S_ST0379_derived_security	"Derived Security Auditing Parameters"
S_ST037A_security_params	"Security Parameters"
S_ST037B_security_param_deriv	"Derived Security Parameters"
S_ST037C_configure_sec_params	"CBF - Configure Security Parameters"
S_ST037D_auditing_menu		"Auditing"
S_ST037E_configure_recovery	"CBF - Configure Recovery Server"
S_ST037F_recovery_parameters	"Recovery Server Parameters"

S_ST0380_recalc_parameter       "Recalculate derived parameter"
S_ST0381_no_recalc_parameter    "Don't recalculate parameter"
S_ST0382_reset_parameter        "Restore default value"
S_ST0383_no_reset_parameter     "Don't restore default" 
S_ST0384_destroy_component	"Destroy component"
S_ST0385_no_destroy_component	"Don't destroy component"

S_ST0386_reformat_prompt
"Do you want to throw away any uncommitted transactions contained in the
selected transaction log by reformatting? "
S_ST0387_reformat_log		"Reformat the selected transaction log"
S_ST0388_no_reformat_log	"Don't reformat log"

S_ST0389_transaction_warning
"WARNING: The selected transaction contains uncommitted transactions!
Reformatting or destroying a transaction log which contains uncommitted
transactions will cause those transactions to be lost."

S_ST038A_examining_logs		"Examining transaction log(s)..."

S_ST038B_no_primary_size
"Unable to determine size of primary transaction log."

S_ST038C_no_secondary_size
"Unable to determine size of backup transaction log."

S_ST038D_log_creation_failed	"Creation of transaction log failed."

S_ST038E_formatting_log		"Formatting transaction log..."

S_ST038F_log_format_failed	"Unable to format transaction log."

S_ST0390_cant_disable_log
"You cannot disable the only active transaction log."

S_ST0391_disabling_log		"Disabling transaction log..."

S_ST0392_log_disable_failed	"Unable to disable transaction log."

S_ST0393_other_log_enabled
"The other transaction log has been enabled, since one of the two
logs must be active."

S_ST0394_log_erase_failed	"Unable to erase transaction log."

S_ST0395_cleaning_up_memory	"Cleaning up shared memory..."

S_ST0396_destroy_log_prompt
"Do you want to destroy the selected transaction log?"

S_ST0397_destroy_log		"Destroy transaction log"
S_ST0398_no_destroy_log		"Don't destroy log"

S_ST0399_log_destroy_failed	"Unable to destroy transaction log."

S_ST039A_log_ops_disabled
"Since %s is on-line, operations on the transaction log(s) have been
disabled."

S_ST039B_destroying_log		"Destroying transaction log..."

S_ST039C_log_size_prompt
"Please enter the size (in megabytes) of the transaction log you want
to create.  The minimum size of a transaction log with %d partition(s)
is %d megabytes."

S_ST039D_integer_expected	"An integer value is expected."

S_ST039E_log_too_small
"The minimum size for a transaction log partition is 32 megabytes.
The minimum size for a transaction log with %d partition(s)
is %d megabytes."

S_ST039F_csinstall_failed_1
"\nUnable to install shared memory (csinstall failed).\n\n"

S_ST0400_mimimum_value
"\nThe minimum value for this setting is %ld.\n\n"

S_ST0401_maximum_value
"\nThe maximum value for this setting is %ld.\n\n"

S_ST0402_valid_values
"\nThis setting must be assigned one of the following values:\n\n"

S_ST0403_no_rule_found
"\nNo configuration rule was found for the specified resource.\n\n"

S_ST0404_value_not_prime
"\nThe value of this seting must be prime.\n\nThe two closest
primes to the number you entered are:\n\n"

S_ST0405_value_not_power2
"\nThe value of this setting must be a power of 2.\n\n"

S_ST0406_value_not_boolean
"\nThe value of this setting must be ON or OFF.\n\n"

S_ST0407_edit_listen_menu	"EditListen"

S_ST0408_invalid_cache_name
"\n The cache_name value specified is invalid.\n The cache_name value:\n    - can not contain operators (eg. ! @ # $)\n    - can not contain whitespace\n    - first character must be alphabetic\n\n"

S_ST0410_crs_syntax_error
"\nSyntax error at line %d of %s\n\n"

S_ST0411_generating_defaults
"\nGenerating default configuration for host \"%s\"...\n"

S_ST0412_sorting_resources
"\nSorting resources...\n"

S_ST0413_computing_resources
"\nComputing resources...\n\n"

S_ST0414_undefined_resource
"\n%s or something it depends on is undefined.\n\n"

S_ST0415_defaults_generated
"\nDefault configuration generated for host \"%s\".\n\n"

S_ST0416_rule_limit_exceeded
"\nrule-base symbol limit (%d) exceeded.\n\n"

S_ST0417_recursive_definition
"\nrecursive definition for %s detected.\n\n"

S_ST0418_no_rules_found
"\nno configuration rules found.\n\n"

S_ST0419_invalid_name
"The name you entered contains non-alphanumeric characters.  The requested
operation was aborted."

S_ST0420_creating_log
"Creating a %ld byte transaction log file..."

S_ST0421_erasing_log
"Erasing the %ld byte transaction log file..."

S_ST0422_finishing_up
"\nFinishing up...\n\n"

S_ST0423_must_be_defined
"%s must be defined."

S_ST0424_on_line		"On-Line"

S_ST0425_off_line		"Off-Line"

S_ST0426_off_line_transactions	"Off-Line / Transactions-Pending"

S_ST0427_cbf_usage		"Usage: cbf [ host ]"

S_ST0428_log_size_warning
"This operation will create a transaction log of the same size as the\n
existing (backup) log.  If you want to change the transaction log size\n
for this installation, you must first destroy the other\n
transaction log file.\n\nDo you want to continue this operation?"

S_ST0429_log_size_ok		"Use the existing transaction log size"
S_ST042A_log_size_not_ok	"Don't use the existing transaction log size"


S_ST043A_inconsistent_log            
"Actual primary log file size is %0c kbytes, but config.dat rcp.file.kbytes\n 
resources says %1c kbytes. Is the actual primary log file size correct?"
S_ST043B_correct_config "Correct the config.dat value and proceed."
S_ST043C_no_correction "Stop the secondary log file size creation."

S_ST042B_xxx			""

S_ST042C_no_directory		"Directory not found. Do you want to create it?"
S_ST042D_create_dir		"Create directory."
S_ST042E_no_create_dir		"Don't create directory."
S_ST042F_config_internet_comm   "CBF - Configure Internet Communication"
S_ST0430_internet_parameters	"Internet Communication Parameters"
S_ST0431_dmf_cache_2k   	"DMF Cache 2k"
S_ST0432_dmf_cache_4k   	"DMF Cache 4k"
S_ST0433_dmf_cache_8k   	"DMF Cache 8k"
S_ST0434_dmf_cache_16k  	"DMF Cache 16k"
S_ST0435_dmf_cache_32k  	"DMF Cache 32k"
S_ST0436_dmf_cache_64k   	"DMF Cache 64k"
S_ST0437_p4k			".p4k"
S_ST0438_p8k			".p8k"
S_ST0439_p16k			".p16k"
S_ST0440_p32k			".p32k"
S_ST0441_p64k			".p64k"
S_ST0442_config_caches          "CBF - Configure DBMS Caches"
S_ST0443_dbms_caches            "DBMS Caches"
S_ST0444_dmcm_warning		"Both dmcm and cache_sharing can NOT be ON"
S_ST0445_p2k			".p2k"
S_ST0446_creating_log
"Creating a %ldK byte transaction log file..."

S_ST0447_erasing_log
"Erasing the %ldK byte transaction log file..."

S_ST0448_startupcnt_zero_one
"The startup count for this system component must be 0 or 1."

/* b95887 */
S_ST0449_no_cache_pg_default
"The DMF cache for the default page size is not enabled. Page size\n
will default to 2K unless the appropriate DMF cache is enabled."

S_ST044A_init_resource
"\nAdding default configuration for resource \"%s\"...\n"

S_ST044B_added_resource
"\nDefault configuration added for resource \"%s\".\n"

S_ST044C_configure_net_prot     "CBF - Configure Net Server Protocols"
S_ST044D_net_protocols_parm     "Net Protocols Parameters"
S_ST044E_registry_proto_parm    "Registry Protocols Parameters"

S_ST0450_auth_system_menu	"System"
S_ST0451_configure_auth_system	"CBF - Configure System Security"
S_ST0452_auth_system_parameter	"System Security Parameters"
S_ST0453_mechanisms_menu	"Mechanisms"
S_ST0454_configure_mechanism	"CBF - Configure Security Mechanism"
S_ST0455_mechanism_parameter	"Security Mechanism Parameters"
S_ST0456_select_mechanism	"Select Mechanism"

S_ST045A_registry_menu		"Registry"

S_ST045B_protected_cpi_prompt
"Please enter the size (in megabytes) of the transaction log you want to create. 
The derived configuration parameter cp_interval_mb is set to %d and is protected. The minimum size for this %d-partition transaction log is therefore %d megabytes."
S_ST045C_protected_cpi_small
"The derived configuration parameter cp_interval_mb has the value %d and is protected.
The minimum size for this %d-partition transaction log is therefore %d megabytes."

/*
** ingstart messages 
*/

S_ST0500_starting_gcn		"\nStarting the Name Server...\n"	

S_ST0501_starting_dmfacp	"\nStarting the Archiver Process...\n"

S_ST0502_starting_dmfrcp	"\nStarting the Recovery Server..."

S_ST0503_dmfrcp_waiting
"\nThe %s Logging System is active.\n\nWaiting for the Logging System to go online..."

S_ST0504_remote_dmfcsp_waiting
"\nIngres on another node is in the process of starting up.\n\nWaiting for any user ingstart scripts to complete..."

S_ST0505_remote_dmfcsp_online
"\nAll user ingstart scripts on remote nodes have completed.\n"

S_ST0506_cluster_log1_on
"The Primary Transaction Log for node %s is enabled.\n"

S_ST0507_cluster_log1_tx
"The Primary Transaction Log for node %s contains transactions.\n"

S_ST0508_cluster_log1_off
"The Primary Transaction Log for node %s is not enabled.\n"

S_ST0509_cluster_log2_on
"The Backup Transaction Log for node %s is enabled.\n"

S_ST050A_cluster_log2_tx
"The Backup Transaction Log for node %s contains transactions.\n"

S_ST050B_cluster_log2_off
"The Backup Transaction Log for node %s is not enabled.\n"

S_ST050C_cluster_log_missing
"Cluster node %s has no Transaction Log!\n"

S_ST050D_verifying_logs
"\nVerifying cluster member Transaction Logs...\n"

S_ST050E_dmfcsp_failed
"Cluster Support failed to start."

S_ST050F_starting_dmfcsp
"\nStarting Cluster Support...\n\n"

S_ST0510_dmfcsp_waiting
"\nCluster Support is active.\n\nWaiting for Cluster Support to go ONLINE...\n"

S_ST0511_csinstalling
"\nAllocating shared memory for Logging and Locking Systems...\n"

S_ST0512_csinstall_failed
"Unable to allocate shared memory."

S_ST0513_dmfrcp_failed
"The Recovery Server failed to start."

S_ST0514_dmfacp_failed
"The Archiver Process failed to start."

S_ST0515_starting_dbms
"\nStarting DBMS Server (%s)..."

S_ST0516_starting_star
"\nStarting Star Server (%s)..."

S_ST0517_starting_gcc
"\nStarting Net Server (%s)..."

S_ST0518_no_definition
"No definition found for specified configuration name."

S_ST0519_system_running
"%s is already running.  Use %sstop to shut down Ingres."

S_ST051A_no_license
"The license string (%s_AUTH_STRING) has not been defined."

S_ST051B_star_needs_dbms
"At least one DBMS Server is required to run Star."

S_ST051C_nothing_configured
"No %s servers have been configured to start up."

S_ST051D_setting_privileges
"\nSetting privileges...\n\n"

S_ST051E_unknown_privilege
"Ignoring unknown privilege. (%s)\n"

S_ST051F_privileges_failed
"Unable to set all required privileges."

S_ST0520_starting_installation
"Starting your %s installation...\n"

S_ST0521_defining_logicals
"\nDefining logicals...\n\n(%s)\n\n"

S_ST0522_crelnm_failed
"Unable to set %s to \"%s\".\n\n"

S_ST0523_dbms_failed
"Unable to start DBMS Server."

S_ST0524_gcc_failed
"Unable to start Net Server."

S_ST0525_star_failed
"Unable to start Star DBMS Server."

S_ST0526_gcn_failed
"The name server failed to start.  This may indicate that %s is\n
already running.  Use %sstop to shut down a running %s installation."

S_ST0527_not_clustered
"\nThis node is configured as part of an %s Cluster.\n"

S_ST0528_invalid_auth_string
"The license string (%s_AUTH_STRING) is invalid.\n"

S_ST0529_configure_bridge          "CBF - Configure Bridge Server Definition"
S_ST052A_bridge_parameters         "Bridge Server Parameters"
S_ST052B_bridge_server             "Bridge Server"
S_ST052D_starting_gcb
"\nStarting Bridge server (%s)..."
S_ST052E_gcb_failed
"Unable to start Bridge server."
S_ST0530_vnode_prompt
"Please enter a vnode name: "
S_ST041A_add_vnode_menu            "Create"
S_ST041B_del_vnode_menu            "Destroy"
S_ST041C_bridge_vnode_menu         "Vnodes"
S_ST041D_configure_bridge_prot     "CBF - Configure Bridge Server Protocols"
S_ST041E_bridge_protocols_parm     "Bridge Protocols Parameters"

S_ST0531_gws_failed
"Unable to start the Desktop server."

S_ST0532_starting_gws
"\nStarting Desktop server..."

S_ST0533_reformat_log
"\nWarning : Please reformat the transaction log file if this value \n
is changed. Please make sure that there is no transaction pending \n
before you reformat the log file. You must shutdown the installation \n
to reformat the log file."

E_ST0136_no_attrib_rows
"There are no attribute name/value entries defined."
S_ST0535_define_new_attr        "Enter new attribute"
S_ST0536_edit_attr              "Edit attribute entry"
E_ST0137_attribname_req 	"Attribute name is required."
E_ST0138_attribval_req 		"Attribute value is required."
E_ST020C_duplicate_attr 	"Duplicate attribute entry.\nTo modify
the definition, use 'Edit'."
S_ST0537_attr_entry     	"attribute entry"
E_ST020D_destroy_attr_failed 	"Failed trying to destroy attribute entry\n"
E_ST020E_no_attr        	"Can't locate attribute entry."
E_ST020F_create_attr_failed  	"Failed trying to create attribute entry\n"
E_ST0534_usage_3
"\n
Usage:  %sbuild [ flags ] [ <distribution-medium> ]\n\n
Flags: list of feature names.\n
       -products         Display a list of the available products.\n
       -version[=<list>] Display the version number of the installation\n
                         or of each product in the comma-separated list.\n
  Only one flag may be specified.  If no command line arguments are\n
specified, %sbuild will run in interactive forms mode.\n"

S_ST0538_rmcmd_failed
"Unable to start Remote Command Server."

S_ST0539_starting_rmcmd
"\nStarting the Visual DBA Remote Command Server..."

S_ST053A_bad_restore_read
"Unable to read saved values."

E_ST053B_nospace
"\nInstalling the selected products requires %s disk space. Please
confirm that there is sufficient disk space before continuing
with the installation.\n"

E_ST053C_nopriv
"\nYou do not have sufficient privilege to run this program.
\nOnly the owner of the installation can run %s.\n"

S_ST053D_choose_login_type      "Choose type of login to be created"

S_ST0540_cannot_run_rcpstat
"\nNOTE:	Since you are not user '%s', and the %s service\n
	has not been started, %sstart cannot accurately detect\n
	whether the logging system is online prior to starting the\n
	DBMS server(s).\n"

S_ST0541_derived_name "Derived Name Server Parameters"

S_ST0542_derived_recovery "Derived Recovery Server Parameters"

S_ST0543_cannot_run_rcpstat2
"NOTE:	Since you are user '%s', and the %s service is\n
	either shutting down or not started, %sstop cannot accurately\n
	detect whether the logging system has properly shut down.\n\n"

S_ST0544_derived_rms            "Derived RMS Server Parameters"

S_ST0545_config_rms_caches      "CBF - Configure RMS Server Caches"

S_ST0546_rms_caches             "RMS Server Caches"

S_ST0550_starting_oracle	"\nStarting Oracle Gateway (%s)...\n"	

S_ST0551_starting_informix	"\nStarting Informix Gateway (%s)...\n"	

S_ST0552_starting_mssql		"\nStarting Microsoft SQL Server Gateway (%s)...\n"	

S_ST0553_starting_sybase	"\nStarting Sybase Gateway (%s)...\n"	

S_ST0554_starting_odbc		"\nStarting ODBC Gateway (%s)...\n"	

S_ST0555_oracle_failed		"The Oracle Gateway failed to start."	

S_ST0556_informix_failed	"The Informix Gateway failed to start."	

S_ST0557_mssql_failed		"The Microsoft SQL Server Gateway failed to start."	

S_ST0558_sybase_failed		"The Sybase Gateway failed to start."	

S_ST0559_odbc_failed		"The ODBC Gateway failed to start."	


S_ST055A_configure_oracle       "CBF - Configure Oracle Gateway Definition"
S_ST055B_oracle_parameters      "Oracle Gateway Parameters"
S_ST055C_oracle_gateway         "Oracle Gateway"
S_ST055D_configure_informix     "CBF - Configure Informix Gateway Definition"
S_ST055E_informix_parameters    "Informix Gateway Parameters"
S_ST055F_informix_gateway       "Informix Gateway"
S_ST0561_configure_sybase       "CBF - Configure Sybase Gateway Definition"
S_ST0562_sybase_parameters      "Sybase Gateway Parameters"
S_ST0563_sybase_gateway         "Sybase Gateway"
S_ST0564_configure_mssql        "CBF - Configure SQL Server Gateway Definition"
S_ST0565_mssql_parameters       "SQL Server Gateway Parameters"
S_ST0566_mssql_gateway          "SQL Server Gateway"
S_ST0567_configure_odbc         "CBF - Configure ODBC Gateway Definition"
S_ST0568_odbc_parameters        "ODBC Gateway Parameters"
S_ST0569_odbc_gateway           "ODBC Gateway"
E_ST056A_no_calicense		   "\n%0cbuild has detected that no license files have been installed.\nPlease run install/%1cmklicense as root before re-running %2cbuild.\n"
S_ST056B_invalid_calicense	   "The %s license is invalid.\n"

S_ST0570_dbms_already_running      "The Workgroup Edition of %s is limited to a single DBMS server.\n"
S_ST0571_log_location_prompt "Please enter the log file location:\n"
S_ST0572_same_no_log_partition "You must have same number of log partitions for primary and dual log.\n"
S_ST0573_mkdir_failed		"Unable to create location:\n"

S_ST0574_starting_icesvr
"\nStarting ICE Server (%s)..."

S_ST0575_icesvr_needs_dbms
"At least one DBMS Server is required to run Ice."

S_ST0576_icesvr_failed
"Unable to start ICE Server."

S_ST0577_icesvr_already_running
"The Workgroup Edition of %s is limited to a single ICE Server.\n"

S_ST0578_ice_server             "ICE  Server"

S_ST0579_star_waiting
"\nWaiting for Star Server to initialize."

S_ST057A_security_label
"Unable to get process security label\n"

S_ST057B_ingres_high_value
"Unable to get value of INGRES_HIGH label.\n"

S_ST057C_convert_ingres_high
"Unable to convert INGRES_HIGH to internal label\n"

S_ST057D_ingres_high_execute
"%sstart must be executed at INGRES_HIGH.\n"

S_ST057E_started_sucessfully
"installation successfully started."

S_ST057F_unable_deassign
"Unable to deassign/job/super"

S_ST0580_iisystem_not_set
"%s has not been set in your environment.\n"

S_ST0581_iiinstall_not_set
"%s_INSTALLATION has not been set in your environment.\n"

S_ST0582_bad_iitemp
"The directory specified by II_TEMPORARY cannot be accessed.\nThe current
directory will be used.\n"

S_ST0583_terminate_now
"All connections held in this installation will be terminated immediately.\n\n"

S_ST0584_time_terminate
"All connections held in this installation will be terminated in %d
minute%s!\n\n"

S_ST0585_proc_check
"\nChecking for active %s processes in installation %s...\n\n"

S_ST0586_error_header
"\t\t\t***************\n\t\t\t***  ERROR  ***\n\t\t\t***************\n\n"

S_ST0587_bad_iinamu
"Fatal error attempting to run %s.\nSystem may be low on memory or other
resources.\n"

S_ST0588_warn_header
"\t\t\t***************\n\t\t\t*** WARNING ***\n\t\t\t***************\n\n"

S_ST0589_uncomm_trans
"All uncommitted transactions will be rolled back the next time %s\nstarts
up.\n"

S_ST058A_uncomm_trans2
"All uncommitted transactions will be rolled back before shutdown is
complete.\n\n"

S_ST058B_incons_dbs
"NOTE:  This operation may leave databases accessed by this installation
in\nan inconsistent state.\n\n"

S_ST058C_no_iigcn
"The Name Server (%s) process is not running in this installation.\n\n"

S_ST058D_iigcn_running
"The Name Server (%s) process %s is running.\n\n"

S_ST058E_too_many_gcn
"Too many Name Server (%s) processes running.\n\n"

S_ST058F_one_iigcc
"There is 1 Net Server (%s) process running:\n\n"

S_ST0590_many_iigcc
"There are %d Net Server (%s) processes running:\n\n"

S_ST0591_no_iigcc
"No Net Server processes (%s) could be found in this installation.\n\n"

S_ST0592_one_iigcb
"There is 1 Bridge Server (%s) process running:\n\n"

S_ST0593_many_iigcb
"There are %d Bridge Server (%s) processes running:\n\n"

S_ST0594_no_iigcb
"No Bridge Server processes (%s) could be found in this installation.\n\n"

S_ST0595_one_ora
"There is 1 Oracle Gateway (%s) process running:\n\n"

S_ST0596_many_ora
"There are %d Oracle Gateway (%s) processes running:\n\n"

S_ST0597_no_ora
"No Oracle Gateway processes (%s) could be found in this installation.\n\n"

S_ST0598_one_informix
"There is 1 Informix Gateway (%s) process running:\n\n"

S_ST0599_many_informix
"There are %d Informix Gateway (%s) processes running:\n\n"

S_ST059A_no_informix
"No Informix Gateway processes (%s) could be found in this installation.\n\n"

S_ST059B_one_mssql
"There is 1 SQL Server Gateway (%s) process running:\n\n"

S_ST059C_many_mssql
"There are %d SQL Server Gateway (%s) processes running:\n\n"

S_ST059D_no_mssql
"No SQL Server Gateway processes (%s) could be found in this installation.\n\n"

S_ST059E_one_syb
"There is 1 Sybase Gateway (%s) process running:\n\n"

S_ST059F_many_syb
"There are %d Sybase Gateway (%s) processes running:\n\n"

S_ST05A0_value_too_long
"The value for %s is too long."

S_ST05A1_fail_to_start_node
"One or more %sstart operations on node %s failed."

S_ST05A2_bad_node
"Specified node %s is not configured as part of this cluster."

S_ST05A3_bad_rad
"RAD id %d is not valid for host %s."

S_ST05A4_setting_affinity
"Core %s processes will be started with affinity to RAD %d."

S_ST05A5_not_supported
"Parameter -%s is not supported in this configuration."

S_ST05A6_system_running
"%s is already running on %s.\n
Use %sstop%s to shut down the system."

S_ST05A7_starting_system_on
"Starting your %s installation on node %s...\n"

S_ST05A8_command_line_too_long
"Command line for remote %s call is too long."

S_ST05A9_local_node
"the local node"

S_ST0600_no_syb
"No Sybase Gateway processes (%s) could be found in this installation.\n\n"

S_ST0601_one_odbc
"There is 1 ODBC Gateway (%s) process running:\n\n"

S_ST0602_many_odbc
"There are %d ODBC Gateway (%s) processes running:\n\n"

S_ST0603_no_odbc
"No ODBC Gateway processes (%s) could be found in this installation.\n\n"

S_ST0604_one_rmcmd
"There is 1 Remote Command Server (%s) process running:\n\n"

S_ST0605_no_rmcmd
"No Remote Command Server processes (%s) could be found in this
installation.\n\n"

S_ST0606_one_dbms
"There is 1 DBMS Server (%s) process running:\n\n"

S_ST0607_many_dbms
"There are %d DBMS Server (%s) processes running:\n\n"

S_ST0608_no_dbms
"No DBMS Server processes (%s) could be found in this installation.\n\n"

S_ST0609_one_ice
"There is 1 ICE Server (icesvr) process running:\n\n"

S_ST060A_many_ice
"There are %d ICE Server (icesvr) processes running:\n\n"

S_ST060B_no_ice
"No ICE Server processes (icesvr) could be found in this installation.\n\n"

S_ST060C_one_star
"There is 1 Star Server (iistar) process running:\n\n"

S_ST060D_many_star
"There are %d Star Server (iistar) processes running:\n\n"

S_ST060E_no_star
"No Star Server processes (iistar) could be found in this installation.\n\n"

S_ST060F_one_recovery
"The Recovery Server %s is running.\n\n"

S_ST0610_no_recovery
"No Recovery Server process could be found in this installation.\n\n"

S_ST0611_nothing_running
"There are no active %s processes running in installation %s.\n\n"

S_ST0612_remove_shmem
"Removing shared memory files.\n\n"

S_ST0613_wait_sessions
"Waiting %d minute%s for active sessions to terminate...\n\n"

S_ST0614_abort_shutdown
"Shutdown of active Ingres DBMS and Star servers has been aborted.\nAll
active DBMS and Star servers have been re-opened for connections.\n\n"

S_ST0615_reopen_net
"Active Net servers which have been closed to new connections cannot
be\nre-opened.  To enable new network connections to or from this
installation,\nyou must start another Net server.\n\n"

S_ST0616_reopen_bridge
"Active Bridge servers which have been closed to new connections cannot
be\nre-opened.  To enable new network connections to or from this
installation,\nyou must start another Bridge server.\n\n"

S_ST0617_net_procs
"%s Net Server process %s..."

S_ST0618_bridge_procs
"%s Bridge Server process %s..."

S_ST0619_stop_ora
"Stopping Oracle Gateway process %s...\n\n"

S_ST061A_stop_informix
"Stopping Informix Gateway process %s...\n\n"

S_ST061B_stop_syb
"Stopping Sybase Gateway process %s...\n\n"

S_ST061C_stop_mssql
"Stopping SQL Server Gateway process %s...\n\n"

S_ST061D_stop_odbc
"Stopping ODBC Gateway process %s...\n\n"

S_ST061E_stop_rmcmd
"Stopping Remote Command Server process %s...\n\n"

S_ST061F_star_procs
"Shutting down Star Server process %s..."

S_ST0620_fail_star
"The %s connection to the Star Server process %s failed.\n"

S_ST0621_fail_try
"To force the server to shut down use \"%s -kill\"\n\n"

S_ST0622_dbms_procs
"%s DBMS Server %s..."

S_ST0623_close_dbms
"Closing DBMS Server %s to new connections..."

S_ST0624_fatal_err
"\t\t\t*******************\n\t\t\t*** FATAL ERROR ***\n\t\t\t*******************\n"

S_ST0625_fail_dbms_conn
"The %s connection to the DBMS Server process %s failed.\n"

S_ST0626_alt_dbms_conn
"You may reissue the %s command with an additional \"-kill\" parameter.\n
This is a last-resort measure.\n\n"

S_ST0627_ice_procs
"%s ICE Server %s..."

S_ST0628_close_ice
"Closing ICE Server %s to new connections..."

S_ST0629_fail_ice_conn
"The %s connection to the ICE Server process %s failed.\n"

S_ST062A_star_active
"Star Server %s still has active sessions.\n\n"

S_ST062B_dbms_active
"DBMS Server %s still has active sessions.\n\n"

S_ST062C_net_active
"Net Server %s still has active sessions.\n\n"

S_ST062D_bridge_active
"Bridge Server %s still has active sessions.\n\n"

S_ST062E_stop_gcn
"Stopping Name Server process %s...\n"

S_ST062F_stop_rcp
"Stopping Recovery and Archiver Server processes...\n\n"

S_ST0630_log_wait
"Waiting for the Logging System to shut down..."

S_ST0631_fail_rcp_stop
"Attempt to shut down recovery server %s failed.\n"

S_ST0632_manual_stop
"Please manually shut down any private servers which are not registered\n
with the Name Server (i.e., the server's configuration has the\nname_service
parameter set to OFF) via the iimonitor command. For example:\n\n    echo
set server shut| iimonitor <Server ID>\n\nThen reissue the %s command.\n"

S_ST0633_rcp_exited
"The Recovery processes have exited...\n\n"

S_ST0634_reqmem_fail
"Request for memory failed."

S_ST0635_reqmem_fail2
"MEreqmem() failure.\n"

S_ST0636_ughash_fail
"\n\nIIUGhiHtabInit() failed.\n\n"

S_ST0637_compute_fail
"\n\ncompute(): unrecognized node type.\n\n"

S_ST0638_PMnumelem_over
"\nHard limit on number of PM symbol components exceeded.\n"

S_ST0639_max_tuple_warn
"Reducing DBMS Server maximum tuple length parameter, may negatively effect
star server.  Please reduce Star Server maximum tuple length parameter as
necessary."

S_ST063A_bad_srv_class
"The server class name '%s' is the same as that for a
%s Server configuration. Please choose another server class name."

S_ST063B_indv_proc_check
"\nChecking for selected %s processes in installation %s...\n\n"

S_ST063C_dbms_servers           "DBMS Servers"
S_ST063D_net_servers            "Net Servers"
S_ST063E_star_servers           "Star Servers"
S_ST063F_bridge_servers         "Bridge Servers"


S_ST0640_starting_rms
"\nStarting RMS Gateway (%s)...\n"	

S_ST0641_rms_failed
"The RMS Gateway failed to start."	

S_ST0642_configure_rms
"CBF - Configure RMS Gateway Definition"

S_ST0643_rms_parameters
"RMS Gateway Parameters"

S_ST0644_rms_gateway
"RMS Gateway"

S_ST0645_starting_rdb
"\nStarting RDB Gateway (%s)...\n"	

S_ST0646_rdb_failed
"The RDB Gateway failed to start."	

S_ST0647_configure_rdb
"CBF - Configure RDB Gateway Definition"

S_ST0648_rdb_parameters
"RDB Gateway Parameters"

S_ST0649_rdb_gateway
"RDB Gateway"

S_ST064A_one_rms
"There is 1 RMS Gateway (%s) process running:\n\n"

S_ST064B_many_rms
"There are %d RMS Gateway (%s) processes running:\n\n"

S_ST064C_no_rms
"No RMS Gateway processes (%s) could be found in this installation.\n\n"

S_ST064D_one_rdb
"There is 1 RDB Gateway (%s) process running:\n\n"

S_ST064E_many_rdb
"There are %d RDB Gateway (%s) processes running:\n\n"

S_ST064F_no_rdb
"No RDB Gateway processes (%s) could be found in this installation.\n\n"

S_ST0650_stop_rms
"Stopping RMS Gateway process %s...\n\n"

S_ST0651_stop_rdb
"Stopping RDB Gateway process %s...\n\n"

S_ST0652_stop_service
"\nShutting down the %s service...\n\n"

S_ST0653_stop_svc_fail
"Error shutting down the %s service, Error [%d] - %sPlease see the Event Log
for any further information.\n\n"

S_ST0654_start_service
"\nStarting the %s service..."

S_ST0655_start_svc_fail
"\n\nError starting the %s service, Error [%d] - %sPlease see the Event Log
for any further information.\n\n"

/*
** INGRES/EDBC JDBC Server Configuration Error Strings
**
*/
S_ST0656_jdbc_server		"JDBC Server"
S_ST0657_configure_jdbc		"CBF - Configure JDBC Server Definition"
S_ST0658_jdbc_parameters	"JDBC Server Parameters"

S_ST0672_shared_mem_in_use
"Active servers seem already to be running.  Please make sure that the
server\nis fully shut down before restarting it."

S_ST0673_starting_db2udb
"\nStarting DB2 UDB Gateway (%s)...\n"	

S_ST0674_db2udb_failed
"The DB2 UDB Gateway failed to start."	

S_ST0675_configure_db2udb
"CBF - Configure DB2 UDB Gateway Definition"

S_ST0676_db2udb_parameters
"DB2 UDB Gateway Parameters"

S_ST0677_db2udb_gateway
"DB2 UDB Gateway"

S_ST0678_one_db2udb
"There is 1 DB2 UDB Gateway (%s) process running:\n\n"

S_ST0679_many_db2udb
"There are %d DB2 UDB Gateway (%s) processes running:\n\n"

S_ST0680_no_db2udb
"No DB2 UDB Gateway processes (%s) could be found in this installation.\n\n"

S_ST0681_stop_db2udb
"Stopping DB2 UDB Gateway process %s...\n\n"

S_ST0682_not_derived_resource
"\nParameter %s you are attempting to protect is not\n
a derived server parameter. Only derived parameters can be protected.\n\n"

S_ST0683_menu_item_invalid
"Sorry, invalid menu item. Please reselect."

S_ST0684_increase_file_limit
"WARNING!!! The recommended file descriptor soft limit for the NET Server to service %d inbound, and %d outbound connections is %d. The current file descriptor soft limit is set to %d."
 
/*
** INGRES DAS Server Configuration Error Strings
**
*/
S_ST0690_das_server		"Data Access Server"
S_ST0691_configure_das		"CBF - Configure Data Access Server Definition"
S_ST0692_das_parameters	"Data Access Server Parameters"
S_ST0693_starting_iigcd	"\nStarting Data Access Server (%s)..."
S_ST0694_das_failed		"Unable to start Data Access Server."

S_ST0695_one_iigcd
"There is 1 Data Access Server (%s) process running:\n\n"

S_ST0696_many_iigcd
"There are %d Data Access Server (%s) processes running:\n\n"

S_ST0697_no_iigcd
"No Data Access Server processes (%s) could be found in this installation.\n\n"

S_ST0698_reopen_das
"Active Data Access servers which have been closed to new connections cannot
be\nre-opened.  To enable new Data Access Server connections to or from this
installation,\nyou must start another Data Access Server.\n\n"

S_ST0699_das_procs
"%s Data Access Server process %s..."

S_ST069A_das_active
"Data Access Server %s still has active sessions.\n\n"

S_ST069B_das_servers		"Data Access Servers"
S_ST069C_configure_das_prot     "CBF - Configure Data Access Server Protocols"
S_ST069D_das_protocols_parm     "Data Access Server Protocols Parameters"
S_ST069E_not_recommended     "WARNING! RUNNING INGSTART WITHOUT THE -SERVICE FLAG ON WINDOWS VISTA\nIS NOT RECOMMENDED. INGRES MAY NOT FUNCTION PROPERLY."

/*
** Remote API Error Strings
**
*/
S_ST06A0_invalid_parameter        "Invalid (NULL) parameter passed to II_PrecheckInstallation."
S_ST06A1_iisystem_not_in_res      "II_SYSTEM not defined in the response file."
S_ST06A2_os_not_min_version       "The operating system does not meet the minimum requirements."
S_ST06A3_bad_path                 "The installation path cannot be found." 
S_ST06A4_path_not_dir             "The installation path is not a directory."
S_ST06A5_path_connot_write        "The user does not have write permission on the installation path."
S_ST06A6_inval_chars_in_path      "The installation path contains characters that are not supported."
S_ST06A7_no_char_map              "charmap field is not defined in the response file."
S_ST06A8_charmap_error            "Error accessing the requested character map."
S_ST06A9_get_computername_fail    "Unable to retrieve computer name."
S_ST06AA_get_host_fail            "Unable to retrieve host name."
S_ST06AB_unmatched_name           "Configured computer name does not match the host name."
S_ST06AC_invalid_host             "The configured computer name contains characters that are not supported."
S_ST06AD_get_user_fail            "Unable to retrieve user name."
S_ST06AE_invalid_user             "The configured user name contains characters that are not supported."
S_ST06AF_shutdown_sucessfully
"instance successfully shut down."
S_ST06B0_no_root_start		      "Starting Ingres as root is not permitted."
S_ST06B1_stop_aborted		      "Ingstop aborted . . .\n"
S_ST06B2_no_privilege		      "User does not have the SERVER_CONTROL privilege.\n"
S_ST06B3_path_length_exceeded     "The length of the installation path has exceeded the permitted maximum.\n"
S_ST06B4_insufficient_buffer      "An insufficient message area has been provided for an Ingres status message.\n"
S_ST06B5_get_message_error        "An error occurred during the retrieval of an Ingres status message.\n"

S_ST06B6_online_successfully        "\nTake the cluster resource online successfully.\n"
S_ST06B7_online_fail        "\nFailed to take the cluster resource online.\n"
S_ST06B8_offline_successfully        "\nTake the cluster resource offline successfully.\n"
S_ST06B9_offline_fail        "\nFailed to take the cluster resource offline. %s %d\n"
S_ST06BA_gcn_wait_fail       "\nFailed to ping Name Server successfully.\n"

S_ST06BB_invalid_instance       "An invalid character or sequence of characters has been provided as an instance code."
S_ST06BC_instance_read          "An error occurred reading the instance code."

S_ST06BD_one_server_running     "There should be one %s server running.\n\n"
S_ST06BE_many_servers_running   "There should be %d %s servers running.\n\n"
/*
** DAS CBF messages
*/
S_ST06BF_multiple_das           "The numeric port '%0c' will increment by one\neach time a new instance of the server\nis started.  Do you wish to allow this?"
S_ST06C0_allow_multiple         "Multiple startup counts allowed"
S_ST06C1_disallow_multiple      "Multiple startup counts not allowed"

S_ST06C2_unknown_connect_id     "The connect_id '%s' does not match any running server processes.\n"
S_ST06C3_mustlogDB_menu         "MustLogDBs"
