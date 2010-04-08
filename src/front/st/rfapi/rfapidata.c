/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

# include <compat.h>
# include <gl.h>
# include <lo.h>
# include <rfapi.h>
# include "rfapilocal.h"

/*
** Name: rfapidata.c
**
** Description:
**
**      Data module for the Response File C API. Contains static
**	and global structures used in API.
**
** History:
**
**    05-Sep-2006 (hanje04)
**	SIR 116907
**      Created.
**    23-Oct-2006 (hanje04)
**	Correct variable names for II_DOTNET_LOCATION and
**	II_DOCUMENTATION_LOCATION.
**	Add entries for:
**	    II_LANGUAGE
**	    II_TERMINAL
**	    II_ADD_REMOVE_PROGRAMS
**	    II_DUAL_LOG_ENABLED
**	    II_DESTROY_TXLOG
**	13-Nov-2006 (hanje04)
**	    SIR 116907
**	    Add section headers to make response file more user readable.
**	    Format used is Windows .ini file and lines are "commented" on 
**	    Linux and UNIX.
**	20-Nov-2006 (hanje04)
**	    SIR 116148
**	    Add entries for II_DATE_TYPE_ALIAS.
**      08-Jul-2009 (hanje04)
**          SIR 122309
**          Add new response file variable II_CONFIG_TYPE which 
**          controls which configuration settings are applied to an
**          instance at install time.
**
*/


/* Location Variables */
static RFAPI_VAR ii_system = {
			II_RF_SYSTEM,
			"II_SYSTEM",
			"Installation location",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};

static RFAPI_VAR ii_database = {
			II_RF_DATABASE,
			"II_DATABASE",
			"Database",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};
static RFAPI_VAR ii_checkpoint = {
			II_RF_CHECKPOINT,
			"II_CHECKPOINT",
			"Backup",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};
static RFAPI_VAR ii_journal = {
			II_RF_JOURNAL,
			"II_JOURNAL",
			"Journal",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};
static RFAPI_VAR ii_work = {
			II_RF_WORK,
			"II_WORK",
			"Temporary",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};
static RFAPI_VAR ii_dump = {
			II_RF_DUMP,
			"II_DUMP",
			"Dump",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};
static RFAPI_VAR ii_log_file = {
			II_RF_LOG_FILE,
			"II_LOG_FILE",
			"Trasaction log location",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_INST_LOC,
			RFAPI_WIN_DEFAULT_INST_LOC,
			};
static RFAPI_VAR ii_dual_log = {
			II_RF_DUAL_LOG,
			"II_DUAL_LOG",
			"Dual log location",
			II_RF_DP_ALL,
			II_RF_DP_NONE,
			NULL,
			NULL,
			};
static RFAPI_VAR ii_location_dotnet = {
			II_LOCATION_DOTNET,
			"II_LOCATION_DOTNET",
			".Net Data Provider Location",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			RFAPI_WIN_DEFAULT_DOTNET_LOC
			};
static RFAPI_VAR ii_location_documentation = {
			II_LOCATION_DOCUMENTATION,
			"II_LOCATION_DOCUMENTATION",
			"Documentation Location",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			RFAPI_WIN_DEFAULT_DOCUMENTATION_LOC
			};

GLOBALDEF RFAPI_VAR *loc_info[] = {
		&ii_system,
		&ii_database,
		&ii_checkpoint,
		&ii_journal,
		&ii_work,
		&ii_dump,
		&ii_log_file,
		&ii_dual_log,
		&ii_location_dotnet,
		&ii_location_documentation,
		NULL };

/* System Variables */
static RFAPI_VAR ii_installation = {
			II_INSTALLATION,
			"II_INSTALLATION",
			"Instance Identifier",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"II",
			"II"
			};

static RFAPI_VAR ii_charset = {
			II_CHARSET,
			"II_CHARSET",
			"Character Set",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_LNX_DEFAULT_CHARSET,
			RFAPI_WIN_DEFAULT_CHARSET
			};

static RFAPI_VAR ii_timezone_name = {
			II_TIMEZONE_NAME,
			"II_TIMEZONE_NAME",
			"Timezone",
			II_RF_DP_ALL,
			II_RF_DP_LINUX,
			RFAPI_LNX_DEFAULT_TIMEZONE,
			NULL
			};

static RFAPI_VAR ii_date_format = {
			II_DATE_FORMAT,
			"II_DATE_FORAMT",
			"Date Format",
			II_RF_DP_ALL,
			II_RF_DP_NONE,
			NULL,
			NULL,
			};

static RFAPI_VAR ii_money_format = {
			II_MONEY_FORMAT,
			"II_MONEY_FORMAT",
			"Money Format",
			II_RF_DP_ALL,
			II_RF_DP_NONE,
			NULL,
			NULL,
			};

static RFAPI_VAR ii_date_type_alias = {
			II_DATE_TYPE_ALIAS,
			"II_DATE_TYPE_ALIAS",
			"Date Type Alias",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"ansidate",
			"ansidate",
			};

static RFAPI_VAR ii_language = {
			II_LANGUAGE,
			"II_LANGUAGE",
			"Interface Language",
			II_RF_DP_ALL,
			II_RF_DP_NONE,
			NULL,
			NULL,
			};

static RFAPI_VAR ii_terminal = {
			II_TERMINAL,
			"II_TERMINAL",
			"Terminal Type",
			II_RF_DP_ALL,
			II_RF_DP_NONE,
			NULL,
			NULL,
			};


GLOBALDEF RFAPI_VAR *sys_var[] = {
		&ii_installation,
		&ii_charset,
		&ii_timezone_name,
		&ii_date_format,
		&ii_money_format,
		&ii_date_type_alias,
		&ii_language,
		&ii_terminal,
		NULL };

/* Installation Options */
static RFAPI_VAR ii_log_file_size_mb = {
			II_LOG_FILE_SIZE_MB,
			"II_LOG_FILE_SIZE_MB",
			"Transaction Log Size (MB)",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			RFAPI_DEFAULT_TXLOG_SIZE_MB,
			RFAPI_DEFAULT_TXLOG_SIZE_MB
			};

static RFAPI_VAR ii_enable_sql92 = {
			II_ENABLE_SQL92,
			"II_ENABLE_SQL92",
			"Enable ISO/ANSI SQL-92 Compliance",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"NO",
			"NO"
			};

static RFAPI_VAR ii_demodb_create = {
			II_DEMODB_CREATE,
			"II_DEMODB_CREATE",
			"Create Demo Database",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_add_to_path = {
			II_ADD_TO_PATH,
			"II_ADD_TO_PATH",
			"Add Ingres Executables to Path",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"NO"
			};


static RFAPI_VAR ii_start_ivm_on_complete = {
			II_START_IVM_ON_COMPLETE,
			"II_START_IVM_ON_COMPLETE",
			"Start IVM after installation completes",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

static RFAPI_VAR ii_start_ingres_on_complete = {
			II_START_INGRES_ON_COMPLETE,
			"II_START_INGRES_ON_COMPLETE",
			"Start Ingres after installation completes",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"NO"
			};

static RFAPI_VAR ii_install_all_icons = {
			II_INSTALL_ALL_ICONS,
			"II_INSTALL_ALL_ICONS",
			"Install Desktop Icons",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"NO"
			};

static RFAPI_VAR ii_setup_odbc = {
			II_SETUP_ODBC,
			"II_SETUP_ODBC",
			"Setup Ingres ODBC Driver",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

static RFAPI_VAR ii_service_start_auto = {
			II_SERVICE_START_AUTO,
			"II_SERVICE_START_AUTO",
			"Start Ingres service with system",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

static RFAPI_VAR ii_service_start_user = {
			II_SERVICE_START_USER,
			"II_SERVICE_START_USER",
			"User to run Ingres Service as",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

static RFAPI_VAR ii_service_start_userpassword = {
			II_SERVICE_START_USERPASSWORD,
			"II_SERVICE_START_USERPASSWORD",
			"Password for Ingres Service user",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};



static RFAPI_VAR ii_userid = {
			II_USERID,
			"II_USERID",
			"User to install Ingres as",
			II_RF_DP_LINUX,
			II_RF_DP_LINUX,
			"ingres",
			NULL
			};

static RFAPI_VAR ii_groupid = {
			II_GROUPID,
			"II_GROUPID",
			"Group for II_USERID",
			II_RF_DP_LINUX,
			II_RF_DP_LINUX,
			"ingres",
			NULL
			};

static RFAPI_VAR ii_start_on_boot = {
			II_START_ON_BOOT,
			"II_START_ON_BOOT",
			"Start Ingres with system",
			II_RF_DP_LINUX,
			II_RF_DP_LINUX,
			"Yes",
			NULL
			};

static RFAPI_VAR ii_install_demo = {
			II_INSTALL_DEMO,
			"II_INSTALL_DEMO",
			"Install Ingres demo DB and apps",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"Yes",
			"Yes"
			};

static RFAPI_VAR ii_create_folder = {
			II_CREATE_FOLDER,
			"II_CREATE_FOLDER",
			"Create Ingres folder on desktop",
			II_RF_DP_LINUX,
			II_RF_DP_LINUX,
			"Yes",
			NULL
			};

static RFAPI_VAR ii_config_type = {
			II_CONFIG_TYPE,
			"II_CONFIG_TYPE",
			"Configuration to be appliance to instance",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"TXN",
			"TXN"
			};

GLOBALDEF RFAPI_VAR *inst_ops[] = {
		&ii_log_file_size_mb,
		&ii_enable_sql92,
		&ii_demodb_create,
		&ii_add_to_path,
		&ii_start_ivm_on_complete,
		&ii_start_ingres_on_complete,
		&ii_install_all_icons,
		&ii_setup_odbc,
		&ii_service_start_auto,
		&ii_service_start_user,
		&ii_service_start_userpassword,
		&ii_userid,
		&ii_groupid,
		&ii_start_on_boot,
		&ii_config_type,
		&ii_install_demo,
		&ii_create_folder,
		NULL };

/* Windows connectivity options */
static RFAPI_VAR ii_enable_wintcp = {
			II_ENABLE_WINTCP,
			"II_ENABLE_WINTCP",
			"Enable WINTCP Net Protocol",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"Yes"
			};

static RFAPI_VAR ii_enable_netbios = {
			II_ENABLE_NETBIOS,
			"II_ENABLE_NETBIOS",
			"Enable NETBIOS Net Protocol",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"NO"
			};

static RFAPI_VAR ii_enable_spx = {
			II_ENABLE_SPX,
			"II_ENABLE_SPX",
			"Enable SPX Net Protocol",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"NO"
			};

GLOBALDEF RFAPI_VAR *wincon_ops[] = {
		&ii_enable_wintcp,
		&ii_enable_netbios,
		&ii_enable_spx,
		NULL };

/* Upgrade/Modify Options */
static RFAPI_VAR ii_upgrade_user_db = {
			II_UPGRADE_USER_DB,
			"II_UPGRADE_USER_DB",
			"Upgrade User Databases",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_remove_all_files = {
			II_REMOVE_ALL_FILES,
			"II_REMOVE_ALL_FILES",
			"Remove DBs and other files during uninstall",
			II_RF_DP_LINUX,
			II_RF_DP_LINUX,
			"NO",
			NULL,
			};

GLOBALDEF RFAPI_VAR *upg_ops[] = {
		&ii_upgrade_user_db,
		NULL };

/* Packages */
static RFAPI_VAR ii_component_core = {
			II_COMPONENT_CORE,
			"II_COMPONENT_CORE",
			"Ingres Core Package",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_component_dbms = {
			II_COMPONENT_DBMS,
			"II_COMPONENT_DBMS",
			"Ingres DBMS Server",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_component_net = {
			II_COMPONENT_NET,
			"II_COMPONENT_NET",
			"Ingres/Net",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_component_odbc = {
			II_COMPONENT_ODBC,
			"II_COMPONENT_ODBC",
			"Ingres ODBC Driver",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_component_star = {
			II_COMPONENT_STAR,
			"II_COMPONENT_STAR",
			"Ingres Federated Database Option",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_component_replicator = {
			II_COMPONENT_REPLICATOR,
			"II_COMPONENT_REPLICATOR",
			"Ingres/Replicator",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"NO",
			"NO"
			};

static RFAPI_VAR ii_component_fronttools = {
			II_COMPONENT_FRONTTOOLS,
			"II_COMPONENT_FRONTTOOLS",
			"Character Based Front End Tools",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

static RFAPI_VAR ii_component_jdbc_client = {
			II_COMPONENT_JDBC_CLIENT,
			"II_COMPONENT_JDBC_CLIENT",
			"Ingres JDBC Driver",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

static RFAPI_VAR ii_component_dotnet = {
			II_COMPONENT_DOTNET,
			"II_COMPONENT_DOTNET",
			"Ingres .NET Data Provider",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

static RFAPI_VAR ii_component_doc = {
			II_COMPONENT_DOCUMENTATION,
			"II_COMPONENT_DOCUMENTATION",
			"Ingres Documentation",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"YES",
			"YES"
			};

GLOBALDEF RFAPI_VAR *pkg_info[] = {
		&ii_component_dbms,
		&ii_component_net,
		&ii_component_odbc,
		&ii_component_star,
		&ii_component_replicator,
		&ii_component_fronttools,
		&ii_component_jdbc_client,
		&ii_component_dotnet,
		&ii_component_doc,
		NULL };

/* CA MDB Options */
static RFAPI_VAR ii_mdb_install = {
			II_MDB_INSTALL,
			"II_MDB_INSTALL",
			"Install CA MDB",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"NO",
			"NO"
			};

static RFAPI_VAR ii_mdb_name = {
			II_MDB_NAME,
			"II_MDB_NAME",
			"CA MDB Name",
			II_RF_DP_ALL,
			II_RF_DP_ALL,
			"mdb",
			"mdb"
			};

static RFAPI_VAR ii_destroy_txlog = {
			II_DESTROY_TXLOG,
			"II_DESTROY_TXLOG",
			"Destroy Transaction log on upgrade",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"NO"
			};

static RFAPI_VAR ii_add_remove_programs = {
			II_ADD_REMOVE_PROGRAMS,
			"II_ADD_REMOVE_PROGRAMS",
			"Add Ingres to Add/Remove programs",
			II_RF_DP_WINDOWS,
			II_RF_DP_WINDOWS,
			NULL,
			"YES"
			};

GLOBALDEF RFAPI_VAR *camdb_ops[] = {
		&ii_mdb_install,
		&ii_mdb_name,
		&ii_destroy_txlog,
		&ii_add_remove_programs,
		NULL };


/* Error Code String */
GLOBALDEF const II_RFAPI_STRING rfapierrs [] = {
	"II_RFAPI_OK",
	"II_RFAPI_FAIL",
	"II_RFAPI_CREATE_FAIL",
	"II_RFAPI_EMPTY_HANDLE",
	"II_RFAPI_FILE_EXISTS",
	"II_RFAPI_FILE_NOT_FOUND",
	"II_RFAPI_INVALID_HANDLE",
	"II_RFAPI_INVALID_LOCATION",
	"II_RFAPI_INVALID_PARAMETER",
	"II_RFAPI_INVALID_FORMAT",
	"II_RFAPI_IO_ERROR",
	"II_RFAPI_MEM_ERROR",
	"II_RFAPI_NULL_ARG",
	"II_RFAPI_PARAM_EXISTS",
	"II_RFAPI_PARAM_NOT_FOUND",
	"II_RFAPI_UNKNOWN_ERROR",
	} ;
