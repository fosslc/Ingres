/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

# ifndef _RFAPI_H
# define _RFAPI_H

# include <compat.h>
# include <gl.h>
# include <lo.h>

/*
** Name: rfapi.h
**
** Description:
** 
**	Header file for response file generation API. Contains typedefs,
**	structures, defines, errors and function prototypes used in 
**	rfapi.
**
** History:
**
**    05-Sep-2006 (hanje04)
**	SIR 116907
**	Created.
**    23-Oct-2006 (hanje04)
**	Add entries for:
**	    II_LANGUAGE
**	    II_TERMINAL
**	    II_ADD_REMOVE_PROGRAMS
**	    II_DESTROY_TXLOG
**	13-Nov-2006 (hanje04)
**	    SIR 116907
**	    Add section headers to make response file more user readable.
**	    Format used is Windows .ini file and lines are "commented" on 
**	    Linux and UNIX.
**	20-Nov-2006 (hanje04)
**	    SIR 116148
**	    Add entries for II_DATE_TYPE_ALIAS.
**	25-Apr-2007 (hanje04)
**	    SIR 118202
**	    Up default Tx log size to 256MB
**	21-May-2007 (hanje04)
**	    SIR 118202
**	    RFAPI_DEFAULT_TXLOG_SIZE_MB is an integer NOT a char string.
**	20-July-2007 (rapma01)
**          changed RFAPI_DEFAULT_TXLOG_SIZE_MB back to char string
**          because it's used as a string in RFAPI_VAR 
**	15-Aug-2007 (hanje04)
**	    BUG 118959
**	    SD 120837
**	    Define RFAPI_DEFAULT_TXLOG_SIZE_MB_INT to be the integer
**	    version of RFAPI_DEFAULT_TXLOG_SIZE_MB. All RFAPI values
**	    are strings but log files size is stored as an integer
**	    in the GUI installer and it is useful to have the integer
**	    value available also.
**	08-Jul-2009 (hanje04)
**	    SIR 122309
**	    Add new response file variable II_CONFIG_TYPE which 
**	    controls which configuration settings are applied to an
**	    instance at install time.
**	20-May-2010 (hanje04)
**	    SIR 123791
**	    Add new parameters and defaults for Vectorwise configuration
**	    - II_VWDATA: Vectorwise data location
**	    - IIVWCFG_MAX_MEMORY: Processing memory for VW server
**	    - IIVWCFG_BUFFERPOOL: Buffer memory for VW server
**	    - IIVWCFG_COLUMNSPACE: Max size for VW database
**	    - IIVWCFG_BLOCK_SIZE: Block size for VW data
**	    - IIVWCFG_GROUP_SIZE: Block group size for VW data
** 
*/

/*
** Typedefs
**
** II_RFAPI_STRING - String pointer
** II_RFAPI_PLATFORMS - Platform bitmap. (Windows and Linux)
** II_RFAPI_OPTIONS - Response file create options bitmap
** II_RFAPI_PNAME - Response file parameter name
** II_RFAPI_PVALUE - Response file parameter value
** II_RFAPI_SIZE - Size type
** II_RFAPI_STATUS - Size type
**
*/
typedef char * II_RFAPI_STRING;
typedef char II_RFAPI_PLATFORMS;
typedef u_i4 II_RFAPI_OPTIONS;
typedef char * II_RFAPI_PVALUE;
typedef size_t II_RFAPI_SIZE;
typedef enum _II_RFAPI_PNAME {
		/* variables */
# define RFAPI_SEC_CONFIG "Ingres Configuration"
		II_INSTALLATION,
		II_CHARSET,
		II_TIMEZONE_NAME,
		II_DATE_FORMAT,
		II_MONEY_FORMAT,
		II_DATE_TYPE_ALIAS,
		II_LANGUAGE,
		II_TERMINAL,
		/* locations */
# define RFAPI_SEC_LOCATIONS "Ingres Locations"
		II_RF_SYSTEM,
		II_RF_DATABASE,
		II_RF_CHECKPOINT,
		II_RF_JOURNAL,
		II_RF_WORK,
		II_RF_DUMP,
		II_RF_VWDATA,
		II_RF_LOG_FILE,
		II_RF_DUAL_LOG,
		II_LOCATION_DOTNET,
		II_LOCATION_DOCUMENTATION,
		/* windows net options */
# define RFAPI_SEC_WIN_NET_OPTIONS "Ingres Connectivity"
		II_ENABLE_WINTCP,
		II_ENABLE_NETBIOS,
		II_ENABLE_SPX,
		II_ADD_TO_PATH,
		/* installation options */
# define RFAPI_SEC_INSTALL_OPTIONS "Ingres Installation Options"
		II_LOG_FILE_SIZE_MB,
		II_ENABLE_SQL92,
		II_DEMODB_CREATE,
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
		II_CONFIG_TYPE,
		/* Upgrade/Modify Options */
# define RFAPI_SEC_UPGRADE_OPTIONS "Ingres Upgrade Options"
		II_UPGRADE_USER_DB,
		II_REMOVE_ALL_FILES,
		/* Ingre Vectorwise Config */
# define RFAPI_SEC_IVW_CONFIG "Vectorwise Configuration"
		II_VWCFG_MAX_MEMORY,
		II_VWCFG_BUFFERPOOL,
		II_VWCFG_COLUMNSPACE,
		II_VWCFG_BLOCK_SIZE,
		II_VWCFG_GROUP_SIZE,
		/* CA MDB Options */
# define RFAPI_SEC_MDB_OPTIONS "CA MDB Options"
		II_MDB_INSTALL,
		II_MDB_NAME,
		II_DESTROY_TXLOG,
		II_ADD_REMOVE_PROGRAMS,
		/* packages */
# define RFAPI_SEC_COMPONENTS "Ingres Components"
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
	RFAPI_STATUS_MAX
        } II_RFAPI_STATUS ;


/*
** Structures
*/

/*
** Name: II_RFAPI_PARAM - Repsonse file parameter
**
** Description:
**	Stores the name value of a parameter to be added to the 
**	response file.
**
**	name - Parameter name
**	value - Parameter value
**	vlen - Length of value
** 	
*/
typedef struct _II_RFAPI_PARAM II_RFAPI_PARAM;

struct _II_RFAPI_PARAM {
    II_RFAPI_PNAME name; /* parameter name */
    II_RFAPI_PVALUE value; /* value */
    II_RFAPI_SIZE vlen; /* size of value */
};

/*
** Name: RFAPI_PARAMS - Parameter list element
**
** Description:
**	Element in bi-directional linked list used to store all the
**	parameters to be added to the response file.
**
**	prev - Pointer to previous element (NULL for first)
**	next - Pointer to next element (NULL for last)
**	param - Response file parameter info
** 	
*/
typedef struct _RFAPI_PARAMS RFAPI_PARAMS;

struct _RFAPI_PARAMS {
    RFAPI_PARAMS *prev;
    RFAPI_PARAMS *next;
    II_RFAPI_PARAM param;
};

/*
** Name: RFAPI_LIST
**
** Description:
**	Master structure for response file generation.
**	
**	response_file_loc	- LO LOCATION used to store name and path to
**			     	  response file.
**	locbuf 			- Pointer to buffer used to store response file
**				  location as string.
**	output_format 		- Format for response file to be created
**				  (Windows or Linux)
**	param_list		- Pointer to start of link list of parameters
**	memtag			- ME tag used for all memory requests associated
**				  with structure.
*/

typedef struct _RFAPI_LIST RFAPI_LIST;

struct _RFAPI_LIST {
    LOCATION response_file_loc; /* location and name for response file */
    II_RFAPI_STRING locbuf;
    II_RFAPI_PLATFORMS output_format; /* Format for response file */
# define II_RF_DP_NONE 0x0	
# define II_RF_DP_WINDOWS 0x01
# define II_RF_DP_LINUX 0x02
# define II_RF_DP_ALL (II_RF_DP_WINDOWS|II_RF_DP_LINUX)
    RFAPI_PARAMS *param_list; /* Pointer to link list of response
				 file parameters */
    II_RFAPI_OPTIONS flags; /* Options for response file creation */
# define II_RF_OP_INITIALIZED 0x80000000
# define II_RF_OP_CLEANUP_AFTER_WRITE 0x00000002
# define II_RF_OP_ADD_ON_UPDATE 0x00000004
# define II_RF_OP_WRITE_DEFAULTS 0x00000008
# define II_RF_OP_INCLUDE_MDB 0x00000010
    u_i2 memtag;	/* ME tag used for memory allocation */
};

typedef RFAPI_LIST * II_RFAPI_HANDLE;

typedef struct _RFAPI_VAR RFAPI_VAR;
struct _RFAPI_VAR {
	II_RFAPI_PNAME pname;
	char	*vname;
	char	*description;
	II_RFAPI_PLATFORMS	valid_on;
	II_RFAPI_PLATFORMS	default_on;
	char	*default_lnx;
	char	*default_win;
	};

/* Default values */
# define RFAPI_DEFAULT_INST_ID "II"
# define RFAPI_DEFAULT_TXLOG_SIZE_MB "256"
# define RFAPI_DEFAULT_TXLOG_SIZE_MB_INT 256
# define RFAPI_WIN_DEFAULT_INST_LOC "C:\\Program Files\\Ingres\\Ingres"RFAPI_DEFAULT_INST_ID
# define RFAPI_LNX_DEFAULT_INST_LOC "/opt/Ingres/Ingres"RFAPI_DEFAULT_INST_ID

# define RFAPI_WIN_DEFAULT_CHARSET "WIN1252"
# define RFAPI_LNX_DEFAULT_CHARSET "ISO88591"
# define RFAPI_LNX_DEFAULT_TIMEZONE "NA-PACIFIC"
# define RFAPI_WIN_DEFAULT_DOTNET_LOC RFAPI_WIN_DEFAULT_INST_LOC"\\Ingres .Net Data Provider" 
# define RFAPI_WIN_DEFAULT_DOCUMENTATION_LOC RFAPI_WIN_DEFAULT_INST_LOC"\\Ingres Documentation"
# define RFAPI_DEFAULT_VWCFG_MAX_MEMORY_GB_INT 1
# define RFAPI_DEFAULT_VWCFG_BUFFERPOOL_GB_INT 2
# define RFAPI_DEFAULT_VWCFG_COLUMNSPACE_GB_INT 512
# define RFAPI_DEFAULT_VWCFG_BLOCK_SIZE_MB_INT 512
# define RFAPI_DEFAULT_VWCFG_GROUP_SIZE_INT 8

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
			II_RFAPI_PLATFORMS newformat );
II_RFAPI_STATUS
IIrfapi_addParam	( II_RFAPI_HANDLE *handle,
			II_RFAPI_PARAM *param );

II_RFAPI_STATUS
IIrfapi_updateParam	( II_RFAPI_HANDLE *handle,
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

# endif /* _RFAPI_H */
