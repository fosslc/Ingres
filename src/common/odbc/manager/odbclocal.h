/* 
** Copyright (c) 2004, 2007 Ingres Corporation 
*/ 
/** 
** Name: odbclocal.h - Ingres ODBC CLI private data definitions.
** 
** Description: 
**   This file describes data used by the ODBC CLI module only.
** 
*/

# include <odbccfg.h>
# include <tm.h>

/*
** Name: IIODBC_HDR - CLI header info.
**
** Description:
**   Used by all CLI handles to map common information.  
**
** History:
**   07-Jul-04 (loera01)
**      Created.
**   17-Feb-05 (Ralph.Loen@ca.com)  SIR 113913
**      Added installer structure for SQLInstallerError(), and added
**      internal SQL_INSTALLER_HANDLE type.
**   14-JUL-2005 (hanje04)
**	Move all II_ODBCDRIVER_FN defines to tracefh.h as GLOBALREFs
**	as mg5_osx doesn't allow multiply defined symbols.
**	GLOBALDEF's are now in common!odbc!manager manager.c.
**   26-Feb-07 (Ralph Loen) SIR 117786
**      Added support for ODBC connection pooling.
**   13-Mar-07 (Ralph Loen) SIR 117786
**      Added templates for IIodbc_timeOutThread() and IIodbc_createThread().
**      Removed "dead" connection attribute.  Removed WideCharToMultiByte()
**      and MultiByteToWideChar().
**   23-Jul-07 (Ralph Loen) SIR 117786
**      Added include of tm.h so that VMS can compile the SYSTIME structure.
*/
typedef struct
{
    void * driverHandle;
    i2 type;
    i2 state;
    BOOL driverError;
    MU_SEMAPHORE sem;
} IIODBC_HDR;

/*
** Name: IIODBC_ERROR - CLI error message structure.
**
** Description:
**    Stores error messages and associated SQLSTATE for error processing.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    char      sqlState[6];
    char      messageText[512];
}  IIODBC_ERROR;

#define IIODBC_MAX_ERROR_ROWS 10

#define SQL_HANDLE_INSTALLER -1
#define HOSTNAME_LEN 64
#define CLI_MASK "CLI"

/*
** Name: IIODBC_ERROR_HDR - CLI error header.
**
** Description:
**    A fixed table for storing error information.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    RETCODE rc;
    i4 error_count;
    IIODBC_ERROR err[IIODBC_MAX_ERROR_ROWS];
}  IIODBC_ERROR_HDR;

/*
** Name: IISETENV_PARAM - SQLSetEnvAttr parmeters.
**
** Description:
**    The CLI stores SQLSetEnvAttr parameters in this structure if making
**    a deferred call to SQLSetEnvAttr().
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    QUEUE setEnvAttr_q;
    SQLHENV    EnvironmentHandle;
    SQLINTEGER Attribute;
    SQLPOINTER ValuePtr;
    SQLINTEGER StringLength;
}  IISETENVATTR_PARAM;

/*
** Name: IISETCONNECATTR_PARAM - SQLSetConnectAttr parmeters.
**
** Description:
**    The CLI stores SQLSetConnectAttr parameters in this structure if making
**    a deferred call to SQLSetConnectAttr().
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    QUEUE setConnectOption_q;
    SQLHDBC ConnectionHandle;
    SQLINTEGER Attribute; 
    SQLPOINTER Value;
    SQLINTEGER StringLength;
}  IISETCONNECTATTR_PARAM;


/*
** Name: IISETCONNECTATTRW - SQLSetConnectAttrW parmeters.
**
** Description:
**    The CLI stores SQLSetConnectAttrW parameters in this structure if making
**    a deferred call to SQLSetConnectAttrW().
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/

typedef struct
{
    QUEUE setConnectAttrW_q;
    SQLHDBC          hdbc;
    SQLINTEGER       fAttribute;
    SQLPOINTER       rgbValue;
    SQLINTEGER       cbValue;      
}  IISETCONNECTATTRW_PARAM;

/*
** Name: IISETCONNECTOPTION_PARAM - SQLSetConnectOption parmeters.
**
** Description:
**    The CLI stores SQLSetConnectOption parameters in this structure if making
**    a deferred call to SQLSetConnectOption().
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    QUEUE setConnectOption_q;
    SQLHDBC ConnectionHandle;
    SQLUSMALLINT Option; 
    SQLUINTEGER Value;
}  IISETCONNECTOPTION_PARAM;


/*
** Name: IISETCONNECTOPTIONW_PARAN - SQLSetConnectOptionW parameters.
**
** Description:
**    The CLI stores SQLSetConnectOptionW parameters in this structure if making
**    a deferred call to SQLSetConnectOptionW().
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    QUEUE setConnectOptionW_q;
    SQLHDBC ConnectionHandle;
    SQLUSMALLINT Option; 
    SQLUINTEGER Value;
}  IISETCONNECTOPTIONW_PARAM;

/*
** Name: IIODBC_DSN_LIST - List of DSN's
**
** Description:
**    Cache for data source list retrieved from SQLDataSources().
**
** History:
**   04-Feb-04 (Ralph.Loen@ca.com)
**      Created.
*/

typedef struct 
{
    char dsnName[OCFG_MAX_STRLEN];
    char dsnDescription[OCFG_MAX_STRLEN];
} IIODBC_DSN_LIST;

/*
** Name: IIODBC_POOL - ODBC connection pool entry
**
** Description:
**    Defines fields used for ODBC driver and environment connection pools.
**
** History:
**   26-Jan-07 (Ralph Loen)  SIR 117786
**      Created.
*/

typedef struct
{
    QUEUE pool_q;
    PTR pdbc;
}  IIODBC_POOL, *pPOOL;

/*
** Name: IIODBC_ENV - CLI environment handle
**
** Description:
**    Defines fields used for CLI environment handles.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
**   26-Feb-07 (Ralph Loen) SIR 117786
**      Added entries for connection pooling.
*/

typedef struct 
{
    QUEUE env_q;
    IIODBC_HDR hdr;
    i4 setEnvAttr_count;
    QUEUE setEnvAttr_q;
    SQLPOINTER envAttrValue;
    i4 usrDsnCount;
    i4 usrDsnCurCount;
    IIODBC_DSN_LIST *usrDsnList;    
    i4 sysDsnCount;
    i4 sysDsnCurCount;
    IIODBC_DSN_LIST *sysDsnList;    
    i4 dbc_count;
    QUEUE dbc_q;
    i4 pool_count;
    QUEUE pool_q;
    IIODBC_ERROR_HDR errHdr;
    BOOL relaxed_match;
    BOOL pooling;
} IIODBC_ENV, *pENV;

/*
** Name: IIODBC_DBC - CLI connection handle
**
** Description:
**    Defines fields used for CLI connection handles.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
**   26-Feb-07 (Ralph Loen) SIR 117786
**      Added fields to store connection attributes.  Added "dead" and
**      "DriverConnect" fields.
*/

typedef struct
{
    QUEUE dbc_q;
    IIODBC_HDR hdr;
    i4 setConnectAttr_count;
    QUEUE setConnectAttr_q;
    i4 setConnectAttrW_count;
    QUEUE getConnectAttr_q;
    i4 setConnectOption_count;
    QUEUE setConnectOption_q;
    i4 getConnectAttr_count;
    QUEUE setConnectAttrW_q;
    i4 getConnectOption_count;
    QUEUE getConnectOption_q;
    i4 setConnectOptionW_count;
    QUEUE setConnectOptionW_q;
    pENV penv;
    i4 stmt_count;
    QUEUE stmt_q;
    IIODBC_ERROR_HDR errHdr;
    BOOL autocommit;
    char *dsn;
    char *uid;
    char *pwd;
    char *database;
    char *server_type;
    char *vnode;
    char *driver;
    char *with_option;
    char *role_name;
    char *role_pwd;
    char *dbms_pwd;
    char *group;
    BOOL blank_date;
    BOOL date_1582;
    BOOL cat_connect;
    BOOL numeric_overflow;
    BOOL cat_schema_null;
    BOOL select_loops;
    SYSTIME expire_time;
    BOOL driverConnect;
} IIODBC_DBC, *pDBC;

/*
** Name: IIODBC_STMT - CLI statement handle
**
** Description:
**    Defines fields used for CLI statement handles.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/

typedef struct
{
    QUEUE stmt_q;
    IIODBC_HDR hdr;
    pDBC pdbc;
    IIODBC_ERROR_HDR errHdr;
    BOOL prepared;
    BOOL select;
    BOOL cursor_open;
    BOOL execproc;
} IIODBC_STMT, *pSTMT;

/*
** Name: IIODBC_DESC - CLI descriptor handle
**
** Description:
**    Defines fields used for CLI descriptor handles.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/

typedef struct
{
    QUEUE desc_q;
    IIODBC_HDR hdr;
    pDBC pdbc;
    IIODBC_ERROR_HDR errHdr;
}  IIODBC_DESC, *pDESC;

typedef struct
{
    IIODBC_ERROR_HDR errHdr;
}  IIODBC_INSTALLER, *pINST;

/*
** Name: IIODBC_CB - CLI control block.
**
** Description:
**   Root control block used by all CLI routines and handles.
**
** History:
**   07-Jul-04 (loera01)
**      Created.
*/
typedef struct
{
    i4 env_count;
    QUEUE env_q;
    i4 desc_count;
    QUEUE desc_q;
    MU_SEMAPHORE sem;
    void * drvConfigHandle;
    i4 odbc_trace_level;
    char *odbc_trace_file;
    IIODBC_INSTALLER inst;
    BOOL pooling;
    i4 pool_count;
    QUEUE pool_q;
    char hostname[64];
    i4 timeout;
}  IIODBC_CB;

#define NO_POOL     0
#define DRIVER_POOL 1
#define HENV_POOL   2

GLOBALREF IIODBC_CB IIodbc_cb;
GLOBALREF BOOL block_init;

SQLINTEGER SQL_API SQLGetPrivateProfileString(
    const char *  szSection, const char *szEntry,
    const char *szDefault, char *szBuffer,
    SQLINTEGER cbBuffer, const char *szFile);
   
RETCODE IIodbc_allocEnv(pENV *);
RETCODE IIodbc_allocDbc(pENV, pDBC *);
RETCODE IIodbc_allocStmt(pDBC, pSTMT *);
RETCODE IIodbc_allocDesc(pDBC, SQLHDESC *);
RETCODE IIodbc_connectPhase1(pDBC);
RETCODE IIodbc_connectPhase2(pDBC);
BOOL IIodbc_freeHandle( SQLSMALLINT, void * );
BOOL isSelectStmt(char *);
BOOL insertErrorMsg(SQLHANDLE, i2, i2, RETCODE, char *, STATUS);
BOOL insertDriverError(SQLHANDLE handle, i2 handleType, RETCODE rc);
BOOL resetErrorBuff(SQLHANDLE, i2);
RETCODE validHandle(SQLHANDLE handle, i2 handleType );
pDESC getDescHandle( SQLHANDLE );
BOOL initLock( i2, SQLHANDLE );
BOOL applyLock( i2, SQLHANDLE );
BOOL releaseLock( i2, SQLHANDLE );
BOOL deleteLock( i2, SQLHANDLE );
void IIodbc_initTrace();
void IIodbc_termTrace();
BOOL isCLI();
void IIodbc_initControlBlock();
BOOL IIodbc_fetchPoolElement (pDBC pdbc, const i2 pool_type, pDBC 
    *pdbcMatch);
BOOL IIodbc_freeEnvHandles ();
BOOL IIodbc_freePoolHandle (pDBC pdbc);
BOOL IIodbc_connStringMatches(char *str1, char *str2);
BOOL IIodbc_formatConnHandle(char *szConnStrIn, pDBC pdbc);
i4 IIodbc_encode( char *str, char *key, char *buff );
i4 IIodbc_decode( char *str, char *key, char *buff );
void IIodbc_timeOutThread();
BOOL IIodbc_createPoolThread();
