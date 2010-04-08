/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

/*
** Name: iirfapi.h
**
** Description:
** 
**	Public header file for response file generation API.
**
** History:
**
**    05-Sep-2006 (hanje04)
**	SIR 116907
**	Created.
** 
*/

/* typedefs */
typedef void * II_RFAPI_HANDLE;
typedef char * II_RFAPI_STRING;
typedef unsigned int II_RFAPI_OPTIONS;
typedef char * II_RFAPI_PVALUE;
typedef size_t II_RFAPI_SIZE;
typedef enum _II_RFAPI_PNAME {
		/* variables */
		II_SYSTEM,
		II_INSTALLATION,
		II_CHARSET,
		II_TIMEZONE_NAME,
		II_DATE_FORMAT,
		II_MONEY_FORMAT,
		/* locations */
		II_DATABASE,
		II_CHECKPOINT,
		II_JOURNAL,
		II_WORK,
		II_DUMP,
		II_LOG_FILE,
		II_DUAL_LOG,
		II_LOCATION_DOTNET,
		II_LOCATION_DOCUMENTATION,
		/* installation options */
		II_LOG_FILE_SIZE_MB,
		II_ENABLE_SQL92,
		II_DEMODB_CREATE,
		II_ENABLE_WINTCP,
		II_ENABLE_NETBIOS,
		II_ENABLE_SPX,
		II_ADD_TO_PATH,
		II_START_IVM_ON_COMPLETE,
		II_START_INGRES_ON_COMPLETE,
		II_INSTALL_ALL_ICONS,
		II_SETUP_ODBC,
		II_SERVICE_START_AUTO,
		II_SERVICE_START_USER,
		II_SERVICE_START_USERPASSWORD,
		II_USERID,
		II_GROUPID,
		II_START_ON_BOOT,
		II_INSTALL_DEMO,
		II_CREATE_FOLDER,
		/* Upgrade/Modify Options */
		II_UPGRADE_USER_DB,
		II_REMOVE_ALL_FILES,
		/* CA MDB Options */
		II_MDB_INSTALL,
		II_MDB_NAME,
		/* packages */
		II_COMPONENT_CORE,
		II_COMPONENT_DBMS,
		II_COMPONENT_NET,
		II_COMPONENT_ODBC,
		II_COMPONENT_STAR,
		II_COMPONENT_REPLICATOR,
		II_COMPONENT_FRONTTOOLS,
		II_COMPONENT_JDBC_CLIENT,
		II_COMPONENT_DOTNET,
		II_COMPONENT_DOCUMENTATION,
		NUM_PARAMS
		} II_RFAPI_PNAME;



/* Structures */
typedef struct _RFAPI_PARAM II_RFAPI_PARAM;
/* response file parameter */
struct _RFAPI_PARAM {
    II_RFAPI_PNAME name; /* parameter name */
    II_RFAPI_PVALUE value; /* value */
    II_RFAPI_SIZE vlen; /* size of value */
}; 

/* flags */
# define II_RF_OP_CLEANUP_AFTER_WRITE 0x00000002
# define II_RF_OP_ADD_ON_UPDATE 0x00000004
# define II_RF_OP_WRITE_DEFAULTS 0x00000008

/* supported platforms */
typedef char II_RFAPI_PLATFORMS;
# define II_RF_DP_NONE 0x0
# define II_RF_DP_WINDOWS 0x01
# define II_RF_DP_LINUX 0x02
# define II_RF_DP_ALL II_RF_DP_WINDOWS|II_RF_DP_LINUX

/* error codes */
typedef enum _II_RFAPI_STATUS {
	II_RF_ST_OK,
	II_RF_ST_FAIL,
	II_RF_ST_CREATE_FAIL,
	II_RF_ST_EMPTY_HANDLE,
	II_RF_ST_FILE_EXISTS,
	II_RF_ST_FILE_NOT_FOUND,
	II_RF_ST_INVALID_HANDLE,
	II_RF_ST_INVALID_LOCATION,
	II_RF_ST_INVALID_PARAMETER,
	II_RF_ST_INVALID_FORMAT,
	II_RF_ST_IO_ERROR,
	II_RF_ST_MEM_ERROR,
	II_RF_ST_NULL_ARG,
	II_RF_ST_PARAM_EXISTS,
	II_RF_ST_PARAM_NOT_FOUND,
	II_RF_ST_UNKNOWN_ERROR,
	} II_RFAPI_STATUS ;


/* function prototypes */
II_RFAPI_STATUS
IIrfapi_initNew		( II_RFAPI_HANDLE *handle,
			II_RFAPI_PLATFORMS format,
			II_RFAPI_STRING response_file,
			II_RFAPI_OPTIONS flags );

II_RFAPI_STATUS
IIrfapi_FromFile	( II_RFAPI_HANDLE *handle,
			II_RFAPI_STRING response_file,
			II_RFAPI_OPTIONS flags );

II_RFAPI_STATUS
IIrfapi_setOutFileLoc	( II_RFAPI_HANDLE	*handle,
			II_RFAPI_STRING newfileloc );

II_RFAPI_STATUS
IIrfapi_setPlatform	( II_RFAPI_HANDLE	*handle,
			II_RFAPI_PLATFORMS platform );

II_RFAPI_STATUS
IIrfapi_addParam	( II_RFAPI_HANDLE *handle,
			II_RFAPI_PARAM *param );

II_RFAPI_STATUS
IIrfapi_updateParam	( II_RFAPI_HANDLE handle,
			II_RFAPI_PARAM *param );

II_RFAPI_STATUS
IIrfapi_removeParam	( II_RFAPI_HANDLE *handle,
			II_RFAPI_PNAME pname );

II_RFAPI_STATUS
IIrfapi_writeRespFile	( II_RFAPI_HANDLE *handle );

II_RFAPI_STATUS
IIrfapi_cleanup( II_RFAPI_HANDLE *handle );

II_RFAPI_STRING
IIrfapi_errString( II_RFAPI_STATUS errcode );
