/*
** Copyright (c) 1999, 2005 Ingres Corporation
**
**
*/

/**
**  Name: tngapi.h
**
**  Description:
**  Function prototypes file for TNG API functions.
**
##  History:
##      04-may-1999 (mcgem01)
##          Created.
##      02-aug-2000 (mcgem01)
##          Added a prototype for II_ServiceStarted
##      17-aug-2000 (mcgem01)
##          Add functions to start and stop the Ingres service.
##      08-aug-2001 (rodjo04) Bug 105063
##          Added missing function II_IngresVersion(). Also added
##          the return codes for this function. This will have to be updated as
##          new versions are created.
##      9-Nov-2001 (rodjo04)
##          Added ERROR CODES for inguninst. Please note that these codes
##          must be in sync with uninstClass.h
##      27-dec-2001 (somsa01)
##          Added prototype for II_IngresServiceName().
##      27-dec-2001 (somsa01)
##          Added define of II_INGRES_260201.
##      11-jan-2002 (somsa01)
##          Added prototype for II_StartServiceSync().
##      14-jan-2002 (somsa01)
##          Added prototype for II_IngresVersionEx().
##      10-apr-2002 (abbjo03)
##          Add prototype for II_PrecheckInstallation().
##      26-Jun-2002 (fanra01)
##          Sir 108122
##          Add defines for module name and max error message length.
##          Add extra error returns for testing user and hostname.
##      09-Sep-2002 (somsa01)
##          Merged contents of oigcnapi.h and gcnapi.h into here for
##          completeness.
##      23-sep-2002 (somsa01)
##          Added define of II_INGRES_260207.
##      25-Apr-2003 (fanra01)
##          Bug 110152
##          Add prototype for II_PingServers
##          Add structure definitions for instance server information.
##      20-aug-2003 (somsa01)
##          Added define of II_INGRES_260305.
##      12-Feb-2004 (hweho01)
##          Added supports for Unix platforms.
##      18-Mar-2004 (hweho01)
##          1) avoid compiler warning, modified the typedef statement   
##             for IISVRINFO. 
##          2) defined bool to char type in UNIX platforms. 
##          3) Added prototype GetPrivateProfileString() for UNIX.
##      29-Apr-2004 (hweho01)
##          Defined BOOL to int for Unix platforms.
##      07-Sep-2004 (fanra01)
##          SIR 112988
##          Add prototype and definitions for II_PrecheckInstallationEx function
##          to allow specification of individual tests.
##      24-Sep-2004 (fanra01)
##          Sir 113152
##          Add exposed function typedefs.
##          Reordered definitions.
##      03-dev-2004 (penga03)
##          Updated the sizes of Ingres products. And added a new API 
##          II_GetIngresInstallSizeEx().
##      14-Dec-2004 (drivi01 on behalf of fanra01)
##          Changed oiutil.dll to iilibutil.dll to reflect new dll names.
##      06-Jan-2005 (fanra01)
##          Reinstate changes 473716, 473774 and 473900, to this file, lost
##          from cross integration of change 472311.
##      14-Jan-2005 (fanra01)
##          Sir 113776
##          Add an architecture parameter to the II_PrecheckInstallationEx
##          function and values.
##          Add function type for II_GetIngresInstallSizeEx.
##          Sir 113777
##          Add function type for II_GetIngresMessage.
##      19-Jan-2005 (drivi01)
##          Added back II_GetIngresInstallSizeEx b/c it was accidentally 
##          removed by one of the previous submissions.
##      07-Feb-2005 (fanra01)
##          Sir 113881
##          Merge Windows and UNIX sources.
##      14-Feb-2005 (fanra01)
##          Sir 113888
##          Add a synchronous service stop function.
##          Sir 113891
##          Add II_CHK_USERNAME as a valid flag.
##      17-Oct-2005 (fanra01)
##          Sir 115396
##          Add II_CHK_INSTCODE as a flag.
##      12-Dec-2006 (hweho01)
##          Added hardware code for Unix, update version number for releases 301,
##          302 and 303. 
**/
# ifndef __TNGAPI_INCLUDED
# define __TNGAPI_INCLUDED
# include <iiapi.h>

# define IIUTIL_CALL

#if defined(UNIX)
# define MAX_COMPUTERNAME_LENGTH  256                
# define UNLEN     256                    
# define II_MODULENAME "liboiutil.1.so"
#else
# define II_MODULENAME  "iilibutil.dll"
#endif

# define MAX_IIERR_LEN  1024

/*
** Opcode values for II_GCNapi_ModifyNode()
*/
#define ADD_NODE                  1
#define DELETE_NODE               2

/*
** Entry type (Private, Global) for II_GCNapi_ModifyNode()
*/
#define GCN_PRIVATE_FLAG          0x0000
#define GCN_GLOBAL_FLAG           0x0001

#define II_INGRES_NOT_RUNNING    -4
#define II_CONFIG_BAD_PARAM      -3
#define II_CONFIG_NOT_FOUND      -2
#define II_INGRES_NOT_FOUND      -1
#define II_INGRES_1200            0
#define II_INGRES_1201            1
#define II_INGRES_209712          2
#define II_INGRES_209808          3
#define II_INGRES_209905          4
#define II_INGRES_200001          5
#define II_INGRES_250006          6
#define II_INGRES_250011          7
#define II_INGRES_260106          8
#define II_INGRES_260201          9
#define II_INGRES_260207          10
#define II_INGRES_260305          11
#define II_INGRES_300404          12
#define II_INGRES_301             13
#define II_INGRES_302             14
#define II_INGRES_303             15

/*
** ERROR CODES for inguninst. NOTE these codes must be in sync with
** uninstClass.h
*/
#define II_SERVER_RUNNING         3   /* A server is running */
#define II_FILES_MARKED_DELETE    1   /* Some files were marked for
                                      ** deletion upon reboot
                                      */
#define II_UNSUCCESSFUL          -1   /* The Uninstall was unsuccessful.
                                      ** Generic error
                                      */
#define II_SUCCESSFUL             0   /* Successful uninstallation */

/*
**  Current sizes for the different products, based upon the Merge
**  Modules for Ingres. Note that these values MUST be updated for
**  every re-packageing of Ingres.
*/
#define SIZE_DBMS                       (15.264)
#define SIZE_DBMS_ICE                   (2.432)
#define SIZE_DBMS_NET                   (35.836)
#define SIZE_DBMS_NET_ICE               (0.100)
#define SIZE_DBMS_NET_TOOLS             (0.373)
#define SIZE_DBMS_NET_VISION            (0.026)
#define SIZE_DBMS_REPLICATOR            (0.022)
#define SIZE_DBMS_TOOLS                 (0.004)
#define SIZE_DBMS_VDBA                  (0.199)
#define SIZE_DBMS_VISION                (0.111)
#define SIZE_DOC                        (56.009)
#define SIZE_ESQLC                      (0.540)
#define SIZE_ESQLC_ESQLCOB              (0.027)
#define SIZE_ESQLCOB                    (0.540)
#define SIZE_ESQLFORTRAN                (0.509)
#define SIZE_ICE                        (3.149)
#define SIZE_JDBC                       (0.705)
#define SIZE_NET                        (0.266)
#define SIZE_NET_TOOLS                  (1.628)
#define SIZE_ODBC                       (0.967)
#define SIZE_REPLICATOR                 (1.091)
#define SIZE_STAR                       (0.211)
#define SIZE_TOOLS                      (1.628)
#define SIZE_TOOLS_VISION               (0.022)
#define SIZE_VDBA                       (13.401)
#define SIZE_VISION                     (1.986)
#define SIZE_DOTNETDP                   (1.578)
#define SIZE_DBMS_EXTRAS                (12.867)
#define SIZE_ICE_EXTRAS                 (14.700)
#define SIZE_TRANSACTION_LOG_DEFAULT    (32.00)
#define SIZE_MDB                        (2048)

/*
** Function flags for II_PrecheckInstallationEx
*/
# define II_CHK_HOSTNAME          0x0001
# define II_CHK_OSVERSION         0x0002
# define II_CHK_PATHCHAR          0x0004
# define II_CHK_PATHDIR           0x0008
# define II_CHK_PATHPERM          0x0010
# define II_CHK_PATHLEN           0x0020
# define II_CHK_USERNAME          0x0040
# define II_CHK_INSTCODE          0x0080
# define II_CHK_ALL             (II_CHK_HOSTNAME | II_CHK_OSVERSION | \
          II_CHK_PATHCHAR | II_CHK_PATHDIR | II_CHK_PATHPERM | \
          II_CHK_PATHLEN | II_CHK_USERNAME | II_CHK_INSTCODE)

/*
** Architecture values for II_PrecheckInstallationEx
*/
# define II_DEFARCH               0x000000  /* Use Ingres internal */
# define II_IA32                  0x696E74  /* 32-bit */
# define II_IA64                  0x693634  /* Intel 64-bit */
# define II_AMD64                 0x613634  /* AMD 64-bit */
# define II_SPARC                 0x737534  /* SUN SPARC  */
# define II_PARISC                0x687062  /* HP  PA-RISC */
# define II_POWERPC               0x727334  /* IBM POWER   */

/* error codes for II_PrecheckInstallation */
#define II_NULL_PARAM             1   /* bad (NULL) parameter */
#define II_NO_INSTALL_PATH        2   /* II_SYSTEM not in response file */
#define II_OS_NOT_MIN_VERSION     3   /* OS version below that required */
#define II_BAD_PATH               4   /* cannot be parsed or does not exist */
#define II_PATH_NOT_DIR           5   /* path not a directory */
#define II_PATH_CANNOT_WRITE      6   /* cannot write to path */
#define II_INVAL_CHARS_IN_PATH    7   /* disallowed characters in path */

#define II_NO_CHAR_MAP            8   /* charmap field not found in rsp */
#define II_CHARMAP_ERROR          9   /* error reading char desc file */
#define II_GET_COMPUTER_FAIL     10   /* unable to get computer name */
#define II_GET_HOST_FAIL         11   /* unable to get host name */
#define II_UNMATCHED_NAME        12   /* computer name doesn't match host */
#define II_INVALID_HOST          13   /* computer name not supported */
#define II_GET_USER_FAIL         14   /* unable to get user name */
#define II_INVALID_USER          15   /* user name unsupported */
#define II_PATH_EXCEEDED         16   /* path exceeds max allowed length */
#define II_INSUFFICIENT_BUFFER   17   /* message does not fit in the area */ 
#define II_GET_MESSAGE_ERROR     18   /* error occured retrieving the msg */
#define II_INVALID_INSTANCECODE  19   /* invalid chars in instance code */
#define II_INSTANCE_ERROR        20   /* error reading instance code */

#ifdef __cplusplus
extern "C" {
#endif

/*
** Name: IISVRINFO          - list of registered servers
**
** Description:
**  Structure is used to return a list of active server
**  processes in a running Ingres instance.
**
**(E
**  size          Size of IISVRINFO
**  servers     Bit mask of active servers
**)E
**
** History:
**        24-Sep-2004 (fanra01)
**            Commented.
*/
typedef struct _svrinfo IISVRINFO;

typedef struct _svrinfo
{
    int  size;
    int  servers;
    /*
    **  Bit masks for servers
    */
# define II_NMSVR                 0x0001
# define II_IUSVR                 0x0002
# define II_INGRES                0x0004
# define II_COMSVR                0x0008
# define II_ICESVR                0x0010
};

extern
int II_IngresVersion( void );

typedef
int
(IIUTIL_CALL *EP_II_INGRESVERSION)( void );

extern
int II_IngresVersionEx( char * );

typedef
int
(IIUTIL_CALL *EP_II_INGRESVERSIONEX)( char * );

extern
II_BOOL II_PingGCN( void );

typedef
II_BOOL
(IIUTIL_CALL *EP_II_PINGGCN)( void );

extern
int  II_PingServers( IISVRINFO* svrinfo );

typedef
int
(IIUTIL_CALL *EP_II_PINGSERVERS)( IISVRINFO* svrinfo );

extern
II_BOOL II_TNG_Version( void );

typedef
II_BOOL
(IIUTIL_CALL *EP_II_TNG_VERSION)( void );

extern
II_BOOL II_ServiceStarted( void );

typedef
II_BOOL
(IIUTIL_CALL *EP_II_SERVICESTARTED)( void );

extern
int II_StopService( void );

typedef
int
(IIUTIL_CALL *EP_II_STOPSERVICE)( void );

extern
int II_StartService( void );

typedef
int
(IIUTIL_CALL *EP_II_STARTSERVICE)( void );

extern
int II_StartServiceSync( void );

typedef
int
(IIUTIL_CALL *EP_II_STARTSERVICESYNC)( void );

extern
int II_StopServiceSync( void );

typedef
int
(IIUTIL_CALL *EP_II_STOPSERVICESYNC)( void );

extern
void II_IngresServiceName( char * );

typedef
void
(IIUTIL_CALL *EP_II_INGRESSERVICENAME)( char * );

extern
int II_GetResource( char *ResName, char *ResValue );

typedef
int
(IIUTIL_CALL *EP_II_GETRESOURCE)( char *ResName, char *ResValue );

extern
void II_GetEnvVariable( char *EnvName, char  **EnvValue );

typedef
void
(IIUTIL_CALL *EP_II_GETENVVARIABLE)( char *EnvName, char  **EnvValue );

extern
int II_PrecheckInstallation( char *response_file, char *error_msg );

typedef
int
(IIUTIL_CALL *EP_II_PRECHECKINSTALLATION)( char *response_file, char *error_msg );

extern
int II_PrecheckInstallationEx(
    int eflags,
    int architecture,
    char* response_file,
    char* error_msg
);

typedef
int
(IIUTIL_CALL *EP_II_PRECHECKINSTALLATIONEX)(
    int eflags, 
    int architecture,
    char* response_file,
    char* error_msg 
);

extern
int II_GCNapi_ModifyNode(
    int     opcode,
    int     entryType,
    char    *virtualNode,
    char    *netAddress,
    char    *protocol,
    char    *listenAddress,
    char    *username,
    char    *password
);

typedef
int
(IIUTIL_CALL *EP_II_GCNAPI_MODIFYNODE)(
    int     opcode,
    int     entryType,
    char    *virtualNode,
    char    *netAddress,
    char    *protocol,
    char    *listenAddress,
    char    *username,
    char    *password
);

extern
int II_GCNapi_TestNode( char *virtualNode );

typedef
int
(IIUTIL_CALL *EP_II_GCNAPI_TESTNODE)( char *virtualNode );

extern
II_BOOL II_GCNapi_Node_Exists( char *virtualNode );

typedef
II_BOOL
(IIUTIL_CALL *EP_II_GCNAPI_NODE_EXISTS)( char *virtualNode );

extern
void II_GetIngresInstallSize ( char *ResponseFile, double *InstallSize );

typedef
void
(IIUTIL_CALL *EP_II_GETINGRESINSTALLSIZE )( char *ResponseFile, double *InstallSize );

extern
void II_GetIngresInstallSizeEx ( char *ResponseFile, char *MSIPath, double *InstallSize );

typedef
void
(IIUTIL_CALL *EP_II_GETINGRESINSTALLSIZEEX )(
    char *ResponseFile,
    char *MSIPath,
    double *InstallSize
);

extern
void II_GetErrorMessage( II_UINT4 status, char* errmsg );

typedef
void
(IIUTIL_CALL *EP_II_GETERRORMESSAGE)( II_UINT4 status, char* errmsg );

extern int
II_GetIngresMessage(
    unsigned int ingres_status,
    int* message_length,
    char* message
);

typedef
int
(IIUTIL_CALL *EP_II_GETINGRESMESSAGE)(
    unsigned int ingres_status,
    int* message_length,
    char* message
);

#if defined(UNIX)
int
GetPrivateProfileString(
    char *app_str,
    char *key_name, 
    char *def_str,
    char *buf_ptr,
    int buf_size,
    char *res_file
);
#endif

#ifdef __cplusplus
}
#endif

# endif /* __TNGAPI_INCLUDED */
