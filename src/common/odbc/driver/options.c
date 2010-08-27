/*
** Copyright (c) 1993, 2010 Ingres Corporation
*/

#include <compat.h>
#include <systypes.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>
#include <me.h>
#include <st.h> 
#include <tr.h>
#include <erodbc.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */

/*
** Name: OPTIONS.C
**
** Description:
**	Connect and statement attribute/option routines for ODBC driver.
**
** History:
**	25-mar-1993 (rosda01)
**	    Initial coding
**	14-mar-1995 (rosda01)
**	    ODBC 2.0 upgrade...
**	15-apr-1997 (tenje01)
**	    made SQL_CURRENT_QUALIFIER an error in GetConnectOption 
**	24-jul-1997 (tenje01)
**	    calling OpenApi to set autocommit on OPT_AUTOCOMMIT  
**	25-jul-1997 (thoda04)
**	    Handle PacketSize and RowsetSize quietly.
**	05-dec-1997 (tenje01)
**	    conver C run-time functions to CL
**	17-feb-1998 (thoda04)
**	    Added cursor SQL_CONCURRENCY (fConcurrency)
**	26-feb-1998 (thoda04)
**	    Hold off on autocommit=on for SetTransaction later
**	21-jul-1998 (thoda04)
**	    Handle SQL_RETRIEVE_DATA, SQL_BIND_TYPE 
**	    and SQL_KEYSET_SIZE quietly.
**	26-oct-1998 (thoda04)
**	    Added support for SQL_ATTR_ENLIST_IN_DTC
**	30-oct-1998 (Dmitriy Bubis)
**	    Before changing autocommit modes, we need to close active
**	    stmt. (Bug 94027) 
**	10-mar-1999 (thoda04)
**	    Port to UNIX.
**	03-aug-1999 (Bobby Ward)
**	    Before changing Access MODE (EX.from READONLY to READ_WRITE)
**	    you must end the Session that is running (Bug # 97853).
**	09-nov-1999 (thoda04)
**	    Allow Set READONLY, SELECT, Set READWRITE
**	    sequence of 97853 if autocommit=on.
**	13-mar-2000 (thoda04)
**	    Add support for Ingres Read Only.
**	23-mar-2000 (thoda04)
**	    If autocommmit not on, don't close resultsets.
**	03-apr-2000 (loera01)
**	    Bug 101087 
**	    For SQLSetConnectOption(), removed READONLY restriction if
**	    SQL_TXN_ISOLATION was set with SQL_TXN_READ_UNCOMMITTED
**	    (allow dirty reads).
**	11-jan-2001 (thoda04)
**	    Integrate MTS/DTC support into 2.8
**	29-may-2001 (thoda04)
**	    Upgrade to 3.5 specifications
**	12-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	13-aug-2001 (thoda04)
**	    Add support for LONGDISPLAY option.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-dec-2001 (thoda04)
**	    Additional 64-bit changes for overloaded attributes.
**	04-jan-2002 (thoda04)
**	    Return empty string for SQL_ATTR_CURRENT_CATALOG.
**      06-apr-2002 (loera01) Bug 107948
**          Return an error when the requested transaction isolation
**          fails to match the set of supported transaction levels--
**          not the other way around.
**      22-aug-2002 (loera01) Bug 108364
**          Make sure a length is provided to SQLGetConnectAttr_InternalCall()
**          if invoked from SQLGetConnectOption().
**      05-nov-2002 (loera01)  
**          Remove unnecessary dependency on API data types.       
**      07-feb-2003 (weife01)
**          Fix bug 109593 for ODBC rejects user setting transaction isolation
**          level in the middle of a session.
**      12-jan-2003 (loera01) Bug 109585
**          In SQLSetConnectAttr_InternalCall(), don't return "invalid
**          argument" for unsupported TXN isolations if the host is 1.2/01 
**          or earlier.  Instead, return SQL_SUCCESS_WITH_INFO.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**     22-sep-2003 (loera01)
**          Added internal ODBC tracing.
**     17-oct-2003 (loera01)
**          Changed "IIsyc_" routines to "odbc_".
**     18-dec-2003 (loera01)
**          In SQLSetEnvAttr() return an error if the application
**          supplied a null environment handle.
**     01-apr-2008 (weife01) Bug 120177
**          Added DSN configuration option "Allow update when use pass through
**          query in MSAccess".
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      09-Jan-2008 (Ralph Loen) SIR 119723
**          Add support for static scrollable cursors in SQLSetStmtAttr()
**          and SQLGetStmtAttr().
**      30-Jan-2008 (Ralph Loen) SIR 119723
**          Add support for keyset-driven scrollable cursors in 
**          SQLSetStmtAttr() and SQLGetStmtAttr().
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Replace obsolete OPT_DBCS with OPT_INGRESDATE
**     11-Feb-2009 (Ralph Loen) Bug 121639
**         Obsolete the cRowsetSize field of tSTMT, which is intended for
**         Level 2 compliancy, with the ArraySize field of the ARD.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**     20-May-2009 (Ralph Loen)  Bug 121556
**         In SQLSetStmtAttr(), if the cursor type is set to
**         SQL_CURSOR_FORWARD_ONLY, set the scrollability to SQL_NONSCROLLABLE.
**         If the cursor type is set to SQL_CURSOR_STATIC or
**         SQL_CURSOR_KEYSET_DRIVEN, set the scrollability to SQL_SCROLLABLE.
**     07-aug-2009 (Ralph Loen) Bug 122425
**         In SQLSetStmtAttr(), implicitly set the cursor type to 
**         SQL_CURSOR_STATIC if the cursor scrollability is specified as 
**         SQL_SCROLLABLE; otherwise, the cursor type is 
**         SQL_CURSOR_FORWARD_ONLY.  If the cursor sensitivity is set to
**         SQL_INSENSITIVE, set the cursor type to SQL_CURSOR_STATIC; 
**         otherwise, the cursor type is SQL_CURSOR_FORWARD_ONLY;
**     24-Nov-2009 (Ralph Loen) Bug 122937
**         In SQLSetStmtAttr(), implicitly set cursor type to
**         SQL_CURSOR_STATIC if SQL_CURSOR_DYNAMIC is requested, instead of
**         defaulting to SQL_CURSOR_FORWARD_ONLY.
**     21-Jul-2010 (Ralph Loen) Bug 124112
**         If the rowset or array size is set via SQLSetStmtAttr(), 
**         zero out pstmt->crowFetchMax and set pstmt->crowFetch to
**         the length specifier.
**     27-Jul-2010 (Ralph Loen) Bug 124131
**         Add support for SQL_ROWSET_SIZE in SQLSetConnectAttr() and    
**         SQLGetConnectAttr().  Addd support for SQL_MAX_ROWS in 
**         SQLGetConnectAttr().  Deprecate support for statement-level
**         attributes if version is SQL_OV_ODBC3 or later.
*/

RETCODE EnlistInDTC(LPDBC pdbc, VOID * pITransaction);
BOOL isL2StatementAttr(SQLINTEGER attr);

/*
**  SQLGetConnectAttr
**
**  SQLGetConnectAttr returns the current setting of a connection attribute.
**
**  On entry:
**    pdbc    [Input] -->connection block
**    fOption [Input]  = type of option
**    pvParam [Output] -->where to return option setting
**      A pointer to memory in which to return the current value of the
**      attribute specified by fOption.
**    BufferLength [Input] = length of buffer if pvParam -> char/bin buffer
**                           (ignored if *pvParam is an integer)
**    pStringLength [Output] --> where to return length if 
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
*/
RETCODE SQL_API SQLGetConnectAttr(
    SQLHDBC     hdbc,
    SQLINTEGER  fOption,
    SQLPOINTER  pvParamParameter,
    SQLINTEGER  BufferLength,
    SQLINTEGER *pStringLength)
{
    return SQLGetConnectAttr_InternalCall(
                hdbc,
                fOption,
                pvParamParameter,
                BufferLength,
                pStringLength);
}

RETCODE SQL_API SQLGetConnectAttr_InternalCall(
    SQLHDBC     hdbc,
    SQLINTEGER  fOption,
    SQLPOINTER  pvParamParameter,
    SQLINTEGER  BufferLength,
    SQLINTEGER  *pStringLength)
{
    LPDBC       pdbc         = (LPDBC)hdbc;
    SQLUINTEGER vParam = 0;
    SQLHANDLE   Handle;
    SQLINTEGER  StringLength = sizeof(SQLUINTEGER);  /* default Stringlength */
    RETCODE     rc;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);        /* clear any errors on DBC */

    if (pvParamParameter == NULL) /* if no option buffer, just return OK */
    {
        UnlockDbc (pdbc);
        return SQL_SUCCESS;
    }

    /*
    ** Statement-level attributes are deprecated for ODBC 3.x and
    ** later.
    */
    if (isL2StatementAttr(fOption) && (pdbc->penvOwner->ODBCVersion 
        >= SQL_OV_ODBC3))
        return ErrUnlockDbc(SQL_HY092, pdbc);
        
    switch (fOption)    /* get connection attributes */
    {
    case SQL_MAX_ROWS:
        vParam = pdbc->crowMax;
        break;

    case SQL_ROWSET_SIZE:
        vParam = pdbc->crowFetchMax;
        break;

    case SQL_ATTR_ACCESS_MODE:
        vParam = (SQLUINTEGER) (pdbc->fOptions & OPT_READONLY) ? SQL_MODE_READ_ONLY
                                                   : SQL_MODE_READ_WRITE;
        break;

    case SQL_ATTR_ASYNC_ENABLE:
        vParam = (SQLUINTEGER) pdbc->fAsyncEnable;
        break;

    case SQL_ATTR_AUTOCOMMIT:
        vParam = (SQLUINTEGER) (pdbc->fOptions & OPT_AUTOCOMMIT) ? TRUE : FALSE;
        break;

    case SQL_ATTR_AUTO_IPD:   /* automatic population of IPD after SQLPrepare */
        vParam = (SQLUINTEGER) SQL_FALSE;
        break;

    case SQL_ATTR_CONNECTION_DEAD:
              /* Is connection dead for pool? */
              /* Did API hit E_AP0001_CONNECTION_ABORTED? */
              /* Called by ODBC DM on logical disconnect. */
        vParam = (SQLUINTEGER) (pdbc->fStatus & DBC_DEAD)?
                       SQL_CD_TRUE:  /* Connection is closed/dead */
                       SQL_CD_FALSE; /* Connection is open/available */
        if (pdbc->fOptions & OPT_LONGDISPLAY) /* if EDS Mapper, autocommit DM confusion */
           vParam = (SQLUINTEGER) SQL_CD_TRUE;  /* makes connection closed/dead to the pool*/
        break;

    case SQL_ATTR_CONNECTION_TIMEOUT:
        vParam = (SQLUINTEGER) pdbc->cConnectionTimeout;
        break;

    case SQL_ATTR_CURRENT_CATALOG:
        rc = GetCharAndLength (pdbc, "", (CHAR*)pvParamParameter,
                               BufferLength, pStringLength);
        UnlockDbc (pdbc);
        return rc;
   case SQL_ATTR_LOGIN_TIMEOUT:
        vParam = (SQLUINTEGER) pdbc->cLoginTimeout;
        break;

    case SQL_ATTR_METADATA_ID:
        vParam = (SQLUINTEGER) pdbc->fMetadataID; /* How to treat catalog string argument*/
                   /*    SQL_FALSE            not treated as identifier */
                   /*    SQL_TRUE             treated as identifier */
        break;

    case SQL_ATTR_ODBC_CURSORS:
            /* implemented by Driver Manager */
        break;

    case SQL_ATTR_PACKET_SIZE:

        vParam = (SQLUINTEGER) pdbc->cPacketSize;
        break;

    case SQL_ATTR_QUIET_MODE:
        Handle = pdbc->hQuietMode;
        break;

    case SQL_ATTR_TRANSLATE_LIB:        
    case SQL_ATTR_TRANSLATE_OPTION:    
        /* optional feature not implemented */
        return ErrUnlockDbc (SQL_HYC00, pdbc); 

    case SQL_ATTR_TXN_ISOLATION:
        vParam = (SQLUINTEGER) pdbc->fIsolation;
        break;

    default:
        return ErrUnlockDbc (SQL_HY092, pdbc);
                             /* Invalid option/attribute identifier */
    }

    if (pStringLength)  /* if want length of returned string */
       *pStringLength = (SQLINTEGER)StringLength;

    if (fOption == SQL_ATTR_QUIET_MODE)
        MEcopy((PTR)&Handle, sizeof(SQLHANDLE), (PTR)pvParamParameter);
    else
        MEcopy((PTR)&vParam, sizeof(SQLUINTEGER), (PTR)pvParamParameter);

    UnlockDbc (pdbc);

    return SQL_SUCCESS;
}


/*
**  SQLGetConnectOption
**
**  Return the current setting of a connection option.
**  associated with a connection.
**
**  On entry: pdbc   -->connection block.
**            fOption = type of option.
**            pvParam -->where to return option setting.
**
**  Returns:  SQL_SUCCESS
**            SQL_NO_DATA_FOUND
**            SQL_ERROR
*/
RETCODE SQL_API SQLGetConnectOption(
    SQLHDBC    hdbc,
    UWORD      fOption,
    SQLPOINTER pvParam)
{
    SQLINTEGER StringLength;
    SQLINTEGER BufferLength=GetAttrLength(fOption);
   
    return(SQLGetConnectAttr_InternalCall(hdbc, fOption, pvParam, 
        BufferLength, &StringLength));
}

/*
**  SQLGetEnvAttr
**
**  SQLGetEnvAttr returns the current setting of an environment attribute.
**
**  On entry:
**    EnvironmentHandle[Input]
**    Attribute        [Input]
**    ValuePtr         [Output]
**    BufferLength     [Input]
**   *StringLength     [Output]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLGetEnvAttr (
    SQLHENV     EnvironmentHandle,
    SQLINTEGER  Attribute,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength,
    SQLINTEGER *pStringLength)
{
    i4 *        pi4      = (i4 *)ValuePtr;
    LPENV       penv     = (LPENV)EnvironmentHandle;
    SQLINTEGER  StringLength = sizeof(i4);  /* default Stringlength */

    /* Don't need to lock ENV */

    ErrResetEnv (penv);        /* clear any errors on ENV */

    if (ValuePtr == NULL)      /* if no return option buffer, just return OK */
        return SQL_SUCCESS;

    switch (Attribute)
    {
    case SQL_ATTR_CONNECTION_POOLING:
        break;                         /* DM does the processing */

    case SQL_ATTR_CP_MATCH:
        break;                         /* DM does the processing */

    case SQL_ATTR_ODBC_VERSION:
/*          SQL_OV_ODBC2
               The driver is to return and is to expect ODBC 2.x codes for
                  date, time, and timestamp data types.
               The driver is to return ODBC 2.x SQLSTATE code when 
                  SQLError, SQLGetDiagField, or SQLGetDiagRec is called.
               The CatalogName argument in a call to SQLTables does not 
                  accept a search pattern.
            SQL_OV_ODBC3
               The driver is to return and is to expect ODBC 3.x codes for
                  date, time, and timestamp data types.
               The driver is to return ODBC 3.x SQLSTATE code when 
                  SQLError, SQLGetDiagField, or SQLGetDiagRec is called.
               The CatalogName argument in a call to SQLTables 
                  accepts a search pattern. */
        *pi4 = penv->ODBCVersion;
        break;

    case SQL_ATTR_OUTPUT_NTS:
        *pi4 = SQL_TRUE;
         /* Always return SQL_TRUE - 
            the driver always returns string data null-terminated */
        break;

    default:
        return ErrState (SQL_HY092, penv);
                             /* Invalid option/attribute identifier */
    }

    if (pStringLength)  /* if want length of returned string */
       *pStringLength = StringLength;

    return(SQL_SUCCESS);
}

/*
** Name:        SQLSetScrollOptions - Set cursor scroll options.
**
** Description:
**              SQLSetScrollOptions() specifies the cursor type and
**              concurrency for SQLFetchScroll() and SQLFetchExtended().
**
** Inputs:
**             hstmt - Statement handle.
**             fConcurrency - Cursor direction.
**             crowKeyset - Key set size.
**             crowRowset - Row set size.
**
** Outputs:
**             None.
**
** Returns:
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions:
**
**              N/A
**
** Side Effects:
**
**              None.
**
** History:
**    07-Feb-2008 (Ralph Loen) SIR 119723
**      Created.
*/

SQLRETURN SQL_API SQLSetScrollOptions(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       fConcurrency,
    SQLINTEGER         crowKeyset,
    SQLUSMALLINT       crowRowset)
{
    LPSTMT       pstmt  = (LPSTMT)hstmt;
    RETCODE      rc     = SQL_SUCCESS;

    TRACE_INTL(pstmt,"SQLSetScrollOptions");
    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    ErrResetStmt (pstmt);        /* clear any errors on STMT */

    pstmt->pARD->ArraySize = crowRowset;

    /*
    ** If crowKeyset is greater than zero, the key buffer size
    ** was specified.  Ignore this.
    */
    if (crowKeyset > 0)
        goto scrollopt_return;
    
    if (pstmt->fAPILevel >= IIAPI_LEVEL_5)
    {
        switch (crowKeyset)
        {
        case SQL_SCROLL_FORWARD_ONLY:
            pstmt->fConcurrency = fConcurrency == SQL_CONCUR_READ_ONLY ? 
                fConcurrency : SQL_CONCUR_VALUES;
            pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
            break;
        
        case SQL_SCROLL_STATIC:
            pstmt->fConcurrency = SQL_CONCUR_READ_ONLY;
            pstmt->fCursorType = SQL_CURSOR_STATIC;
            break;
        
        case SQL_SCROLL_KEYSET_DRIVEN: 
            pstmt->fConcurrency = SQL_CONCUR_VALUES;
            pstmt->fCursorType = SQL_CURSOR_KEYSET_DRIVEN;
            break;
    
        default:
            pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
            return ErrUnlockStmt (SQL_01S02, pstmt);
            break;
        }
    }
    else if (crowKeyset != SQL_SCROLL_FORWARD_ONLY)
    {
        pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
        return ErrUnlockStmt(SQL_01S02, pstmt);
    }
    else 
    {
        pstmt->fConcurrency = fConcurrency == SQL_CONCUR_READ_ONLY ? 
            fConcurrency : SQL_CONCUR_VALUES;
        pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
    }
 
scrollopt_return:
    if (SQL_SUCCEEDED(rc))
		pstmt->pARD->ArraySize = crowRowset > 1 ? crowRowset : 1;
    
    UnlockStmt (pstmt);
    return rc;
}
/*
**  SQLGetStmtAttr
**
**  SQLGetStmtAttr returns the current setting of a statement attribute.
**
**  On entry:
**    StatementHandle  [Input]
**    Attribute        [Input]
**    ValuePtr         [Output]
**    BufferLength     [Input]
**   *StringLength     [Output]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLGetStmtAttr (
    SQLHENV      StatementHandle,
    SQLINTEGER   Attribute,
    SQLPOINTER   ValuePtr,
    SQLINTEGER   BufferLength,
    SQLINTEGER  *pStringLength)
{
    return SQLGetStmtAttr_InternalCall (
                 StatementHandle,
                 Attribute,
                 ValuePtr,
                 BufferLength,
                 pStringLength);
}

SQLRETURN  SQL_API SQLGetStmtAttr_InternalCall (
    SQLHENV      StatementHandle,
    SQLINTEGER   Attribute,
    SQLPOINTER   ValuePtr,
    SQLINTEGER   BufferLength,
    SQLINTEGER  *pStringLength)
{
    LPSTMT       pstmt        = (LPSTMT)StatementHandle;
    SQLUINTEGER     *pvParamulen  = (SQLUINTEGER *)ValuePtr;
    LPDESC      *pvParamDesc  = (LPDESC *)ValuePtr;
    CHAR       **pvParamChar  = (CHAR**)ValuePtr;
    SQLUSMALLINT**pvParampui2 = (SQLUSMALLINT**)ValuePtr;
    SQLUINTEGER *pvParamuint  = (SQLUINTEGER *)ValuePtr;
    SQLUINTEGER    **pvParampulen = (SQLUINTEGER**)ValuePtr;
    SQLINTEGER   StringLength = sizeof(i4);  /* default Stringlength */
    RETCODE      rc     = SQL_SUCCESS;

    TRACE_INTL(pstmt,"SQL_API SQLGetStmtAttr_InternalCall");

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    ErrResetStmt (pstmt);        /* clear any errors on STMT */

    if (ValuePtr == NULL)        /* just return if no return buffer */
    {
        UnlockStmt (pstmt);
        return SQL_SUCCESS;
    }

    switch (Attribute)
    {
    case SQL_ATTR_APP_PARAM_DESC:

            /* Handle to the APD to be used for subsequent
               calls to SQLExecute and SQLExecDirect */
        *pvParamDesc = pstmt->pAPD;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_APP_ROW_DESC:

            /* Handle to the ARD to be used for subsequent fetches */
        *pvParamDesc = pstmt->pARD;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_ASYNC_ENABLE:

            /* SQL_ASYNC_ENABLE_OFF - execute statements synchronously (default)
               SQL_ASYNC_ENABLE_ON  - execute statements asynchronously */
        *pvParamuint = pstmt->fAsyncEnable;
        break;

    case SQL_ATTR_CONCURRENCY:

            /* SQL_CONCUR_READ_ONLY - cursor is read-only. no updates (default)
               SQL_CONCUR_LOCK      - cursor uses lowest level of locking
               SQL_CONCUR_ROWVER    - cursor uses optimistic control, 
                                      comparing row versions
               SQL_CONCUR_VALUES    - cursor uses optimistic control, 
                                      comparing values */
        *pvParamuint = pstmt->fConcurrency;
        break;

    case SQL_ATTR_CURSOR_SCROLLABLE:

            /* SQL_NONSCROLLABLE - scrollable cursors are not required (default)
               SQL_SCROLLABLE    - scrollable cursors are required */
        if (pstmt->fAPILevel >= IIAPI_LEVEL_5)
            *pvParamuint = pstmt->fCursorScrollable;
        else
            *pvParamuint = SQL_NONSCROLLABLE;
        break;

    case SQL_ATTR_CURSOR_SENSITIVITY:

            /* SQL_UNSPECIFIED - unspecified whether changes visible (default)
               SQL_INSENSITIVE - cursors don't reflect changes made by another
               SQL_SENSITIVE   - cursors reflect changes made by another cursor
               Only support SQL_UNSPECIFIED since Ingres can't set sensitivity
               at the cursor level. */
        *pvParamuint = pstmt->fCursorSensitivity;
        break;

    case SQL_ATTR_CURSOR_TYPE:

            /* SQL_CURSOR_FORWARD_ONLY - cursor only scrolls forward (default)
               SQL_CURSOR_KEYSET_DRIVEN- data in result set static
               SQL_CURSOR_DYNAMIC      - driver saves/uses only keys of rowset
               SQL_CURSOR_STATIC       - result set data static  */
        if (pstmt->fAPILevel >= IIAPI_LEVEL_5 && ((pstmt->fCursorType ==
            SQL_CURSOR_STATIC) || (pstmt->fCursorType == 
            SQL_CURSOR_KEYSET_DRIVEN)))
            *pvParamuint = pstmt->fCursorType;
        else
            *pvParamuint = SQL_CURSOR_FORWARD_ONLY;
        break;

    case SQL_ATTR_ENABLE_AUTO_IPD:

            /* SQL_FALSE - auto population of IPD after SQLPrepare is off (default)
               SQL_TRUE  - auto population of IPD after SQLPrepare is on  */
        *pvParamuint = SQL_FALSE;
        break;

    case SQL_ATTR_FETCH_BOOKMARK_PTR:

            /* Pointer to a binary bookmark value 
               used by SQLFetchScroll(SQL_FETCH_BOOKMARK) */
        *pvParamChar = pstmt->pFetchBookmark;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_IMP_PARAM_DESC:

            /* The handle of the stmt's initially allocated IPD descriptor */
        *pvParamDesc = pstmt->pIPD_automatic_allocated;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_IMP_ROW_DESC:

            /* The handle of the stmt's initially allocated IRD descriptor */
        *pvParamDesc = pstmt->pIRD_automatic_allocated;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_KEYSET_SIZE:

        /* Number of rows in the keyset buffer for a keyset-driven 
        ** cursor. Ignored.
        */
        *pvParamulen = 0;   /* SQLExtendedFetch option ignored */
        break;

    case SQL_ATTR_MAX_LENGTH:

            /* Maximum length of character/binary data driver returns */
        *pvParamulen = pstmt->cMaxLength;
        break;

/*    case SQL_ATTR_MAX_ROWS same as SQL_MAX_ROWS */ 
    case SQL_MAX_ROWS:

            /* Maximum number of rows to return to an application
               for a SELECT statement.  If 0, return all rows */
        *pvParamulen = pstmt->crowMax;
        break;

    case SQL_ATTR_METADATA_ID:

            /* SQL_FALSE - don't treat as identifiers; case is significant; 
                           may contain search pattern characters. (default)
               SQL_TRUE  - string argument of catalog functions treated as
                           identifiers case not significant; remove trailing
                           spaces; fold to uppercase. */
        *pvParamuint = pstmt->fMetadataID;
        break;

    case SQL_NOSCAN:

            /* SQL_NOSCAN_OFF - driver scans SQL strings for escape sequences (default)
               SQL_NOSCAN_ON  - Don't scan; send stmt directly to data source  */
        *pvParamuint = (pstmt->fOptions & OPT_NOSCAN) ? SQL_NOSCAN_ON
                                                  : SQL_NOSCAN_OFF;
        break;

    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:

            /* offset added to pointer to change binding of dynamic parameters.
               always added directly to:
                   SQL_DESC_DATA_PTR
                   SQL_DESC_INDICATOR_PTR
                   SQL_DESC_OCTET_LENGTH_PTR */
        *pvParampulen = pstmt->pAPD->BindOffsetPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_PARAM_BIND_TYPE:

            /* SQL_PARAM_BIND_BY_COLUMN (default) or 
               length of structure or instance buffer (bind by row) */
        *pvParamuint = (SQLUINTEGER)pstmt->pAPD->BindType;
        StringLength = sizeof(SQLUINTEGER);
        break;

    case SQL_ATTR_PARAM_OPERATION_PTR:

            /* array of SQL_PARAM_PROCEED or SQL_PARAM_IGNORE or NULL */
        *pvParampui2 = pstmt->pAPD->ArrayStatusPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_PARAM_STATUS_PTR:

            /* array of statuses */
        *pvParampui2 = pstmt->pIPD->ArrayStatusPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_PARAMS_PROCESSED_PTR:
            /* array of sets of parameter values */
        *pvParampulen = pstmt->pIPD->RowsProcessedPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_PARAMSET_SIZE:

            /* number of sets of parameter values */
        *pvParamuint = (SQLUINTEGER)pstmt->pAPD->ArraySize;
        break;

    case SQL_ATTR_QUERY_TIMEOUT:

            /* number of to wait for SQL statement to execute before timeout */
        *pvParamuint = (SQLUINTEGER)pstmt->cQueryTimeout;
        break;

    case SQL_ATTR_RETRIEVE_DATA:        /* kludge for ADODB.Recordset */

            /* SQL_RD_ON  - fetch data after it positions the cursor (default)
               SQL_RD_OFF - don't fetch data after it positions the cursor  */
        *pvParamuint = pstmt->cRetrieveData;
        break;

    case SQL_ATTR_ROW_ARRAY_SIZE:
            /* number of rows returned by SQLFetch or SQLFetchScroll.
               also number of rows in a bookmark array in 
               bulk bookmark SQLBulkOperations.  Default rowset size is 1 */
        *pvParamulen = pstmt->pARD->ArraySize;
        break;

    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
            /* offset added to pointer to change binding of column data.
               always added directly to:
                  SQL_DESC_DATA_PTR
                  SQL_DESC_INDICATOR_PTR
                  SQL_DESC_OCTET_LENGTH_PTR */
        *pvParampulen = pstmt->pARD->BindOffsetPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_ROW_BIND_TYPE:
            /* SQL_BIND_BY_COLUMN (default) or length of structure or 
                  instance buffer (bind by row)
               this field was part of ODBC 1.0 but now we use ARD.  */
        *pvParamuint = (SQLUINTEGER)pstmt->pARD->BindType;
        break;

    case SQL_ATTR_ROW_NUMBER:
            /* number of the current row in the entire result set */
        *pvParamulen = 0;  /* 0 == unknown */
        break;

    case SQL_ATTR_ROW_OPERATION_PTR:
            /* array of values used to ignore a row during bulk SQLSetPos
                  SQL_ROW_PROCEED
                  SQL_ROW_IGNORE */
        *pvParampui2 = pstmt->pARD->ArrayStatusPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_ROW_STATUS_PTR:
            /* array of values containing row status values
               after SQLFetch or SQLFetchScroll */
        *pvParampui2 = pstmt->pIRD->ArrayStatusPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_ROWS_FETCHED_PTR:
            /* number of rows fetched after SQLFetch or SQLFetchScroll */
        *pvParampulen = pstmt->pIRD->RowsProcessedPtr;
        StringLength = sizeof(SQLPOINTER);
        break;

    case SQL_ATTR_USE_BOOKMARKS:        /* quietly process the unused option */

            /* SQL_UB_OFF      - don't use bookmarks (default)
               SQL_UB_VARIABLE - application will use bookmarks  */
        *pvParamuint = pstmt->cUseBookmarks;
        break;

    case SQL_ATTR_SIMULATE_CURSOR:

        return ErrUnlockStmt(SQL_HYC00, pstmt); /* optional feature not implemented */

    case SQL_ROWSET_SIZE:

        /* number of rows in 2.x SQLExtendedFetch rowset (default=1) */
        *pvParamuint = (SQLUINTEGER)pstmt->pARD->ArraySize;  
        break;

    default:

        return ErrUnlockStmt (SQL_HY092, pstmt);
    }
    UnlockStmt (pstmt);
    return SQL_SUCCESS;
}


/*
**  SQLGetStmtOption
**
**  Return the current setting of a statement option.
**
**  On entry: pstmt  -->statement block.
**            fOption = type of option.
**            pvParam -->where to return option setting.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/
RETCODE SQL_API SQLGetStmtOption(
    SQLHSTMT     hstmt,
    UWORD        fOption,
    SQLPOINTER   pvParamParameter)
{
    SQLINTEGER StringLength;

    return(SQLGetStmtAttr_InternalCall(hstmt, fOption, pvParamParameter, 0, &StringLength));
}


/*
**  SQLSetConnectAttr
**
**  Set connection option or default statement option.
**  Default statement option is set for all allocated statements.
**
**  On entry: pdbc   -->connection block.
**            fOption = option.
**            vParam  = option setting or -->option setting
**            StringLength = length or char string if needed.
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
*/
RETCODE SQL_API SQLSetConnectAttr(
    SQLHDBC     hdbc,
    SQLINTEGER  fOption,
    SQLPOINTER  pValue,
    SQLINTEGER  StringLength)
{
    return SQLSetConnectAttr_InternalCall(
                hdbc,
                fOption,
                pValue,
                StringLength);
}

RETCODE SQL_API SQLSetConnectAttr_InternalCall(
    SQLHDBC     hdbc,
    SQLINTEGER  fOption,
    SQLPOINTER  pValue,
    SQLINTEGER  StringLength)
{
    LPDBC         pdbc   = (LPDBC)hdbc;
    SQLUINTEGER       vParamulen = (SQLUINTEGER) (SCALARP)pValue;
    SQLUINTEGER   vParamuint = (SQLUINTEGER)vParamulen;
    LPSTMT        pstmt;
    UDWORD        dw;
    RETCODE       rc = SQL_SUCCESS;           
    RETCODE       rc1;           
    LPSESS        psess;

    TRACE_INTL(pdbc,"SQLSetConnectAttr_InternalCall");
    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);        /* clear any errors on DBC */

    /*
    ** Statement-level attributes are deprecated for ODBC 3.x and
    ** later.
    */
    if (isL2StatementAttr(fOption) && (pdbc->penvOwner->ODBCVersion 
        >= SQL_OV_ODBC3))
         return ErrUnlockDbc(SQL_HY092, pdbc);
    
    switch (fOption)    /* set connection attributes */
    {
    case SQL_ATTR_ACCESS_MODE:
        if ((vParamuint == SQL_MODE_READ_ONLY  &&  (pdbc->fOptions & OPT_READONLY)) ||
            (vParamuint == SQL_MODE_READ_WRITE && !(pdbc->fOptions & OPT_READONLY)))
        {
            pdbc->fOptionsSet |= OPT_READONLY;  /* Remember option was set by
                                                   user and not defaulted */
            break;  /* just return OK because there's no change of state */
        }
        
        rc1 = SQLTransact_InternalCall (NULL, pdbc, SQL_COMMIT);

        switch (vParamuint)
        {
        case SQL_MODE_READ_ONLY:

            if(!(pdbc->fOptions & OPT_ALLOWUPDATE))
            {
                pdbc->fOptions |= OPT_READONLY;
                break;
            }

        case SQL_MODE_READ_WRITE:

            if (isDriverReadOnly(pdbc))  /* if Read Only driver, ignore it */
            { 
                rc = ErrState (SQL_01000, pdbc, F_OD0151_IDS_ERR_NOT_READ_ONLY);
                UnlockDbc (pdbc);
                return rc;
            }

            pdbc->fOptions &= ~OPT_READONLY;
            break;

        default:

            return ErrUnlockDbc (SQL_HY024, pdbc);
        }  /* end switch (vParam) */

        pdbc->sessMain.fStatus &= ~SESS_SESSION_SET;    /* Enable SetTransaction */
        pdbc->fOptionsSet |= OPT_READONLY;              /* Remember for next SQLConnect */
        break;

    case SQL_ATTR_ASYNC_ENABLE:
        pdbc->fAsyncEnable = (UWORD)vParamuint;
        break;

    case SQL_ATTR_AUTOCOMMIT:

        pdbc->sqb.pSqlca   = &pdbc->sqlca;
        pdbc->sqb.pSession =
        psess              = &pdbc->sessMain;
        pdbc->sqb.pDbc = pdbc;

        /* if already in the right mode just skip the change, and don't close */
        if ((vParamuint==SQL_AUTOCOMMIT_ON  &&  !(pdbc->fOptions & OPT_AUTOCOMMIT)) ||
            (vParamuint==SQL_AUTOCOMMIT_OFF &&   (pdbc->fOptions & OPT_AUTOCOMMIT)))
        {
            if (vParamuint==SQL_AUTOCOMMIT_ON)
            {
                /*  
                **  Switching from manual-commit mode to auto-commit mode.
                **
                **  Turning autocommit on commits the transaction if started.
                **  Note that we would return SQL_SUCCESS_WITH_INFO and a
                **  message when it actually does a commit, if only the ODBC
                **  Driver Manager worked correctly.  But the stupid DM seems to
                **  think that implies an error, and then does not reissue the
                **  connect option if the DBC is reused.
                **
                **  As per Programmer's Reference, changing from manual-commit
                **  mode to auto-commit mode commits any open transactions on
                **  the connection.
                */
                if ( (psess->tranHandle) &&
                    (!(psess->fStatus & SESS_AUTOCOMMIT_ON_ISSUED)) )
                    rc = SQLTransact_InternalCall (NULL, pdbc, SQL_COMMIT);
                
                pdbc->fOptions |= OPT_AUTOCOMMIT;
                /* if SetTransaction had a chance to set transaction isolation 
                   then go ahead and set autocommit=on */
                if (pdbc->psqb->pSession->fStatus & (SESS_STARTED | SESS_SESSION_SET))
                {
                    /*  before changing autocommit mode we need to close
                        a statement */
                    for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
                    {
                        if (pstmt->stmtHandle)  /* close it */
                        {
                            pstmt->fStatus &= ~(STMT_OPEN | STMT_CLOSED |
                                                STMT_EOF | STMT_CONST |
                                                STMT_CATALOG | STMT_API_CURSOR_OPENED);
                            odbc_close(pstmt);
                        }
                    }
                    rc = odbc_AutoCommit(pdbc->psqb,TRUE); 
                }
            } /*  end if switching from manual-commit mode to auto-commit mode */
            else
            { /*  switching from auto-commit mode to manual-commit mode */
                pdbc->fOptions &= ~OPT_AUTOCOMMIT;
                /* if API autocommit on, then we need to change it and close resultsets
                else API autocommit is off (due to simulated autocommit (due to 
                open cursors)) then leave MS Access resultsets open */
                if (psess->fStatus & SESS_AUTOCOMMIT_ON_ISSUED)
                {
                /*  before changing autocommit mode we need to close
                   open resultsets; otherwise, API returns SQLSTATE 25000
                   and msg "The requested operation cannot be performed
                   with active queries." */
                    for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
                    {
                        if (pstmt->stmtHandle)  /* close it */
                        {
                            pstmt->fStatus &= ~(STMT_OPEN | STMT_CLOSED |
                                           STMT_EOF | STMT_CONST |
                                           STMT_CATALOG | STMT_API_CURSOR_OPENED);
                            odbc_close(pstmt);
                        }
                    }
                    rc = odbc_AutoCommit(pdbc->psqb,FALSE);
                }  /* end if API autocommit on */
            } /*  end else switching from auto-commit mode to manual-commit mode */
        } /* end if ((vParamuint==SQL_AUTOCOMMIT_ON  &&  
          ** !(pdbc->fOptions & OPT_AUTOCOMMIT)) ...
          */
        pdbc->fOptionsSet |= OPT_AUTOCOMMIT;  /* indicate user set the option */
        break;

/*  case SQL_ATTR_AUTO_IPD:       */    /* can only be get, not set */

/*  case SQL_ATTR_CONNECTION_DEAD:*/    /* can only be get, not set */

    case SQL_ATTR_CONNECTION_TIMEOUT:
        pdbc->cConnectionTimeout = vParamuint;
        break;

    case SQL_ATTR_CURRENT_CATALOG:
        break;                          /* just ignore it... */

    case SQL_ATTR_ENLIST_IN_DTC:  /* MTS's Distributed Transaction Coordinator*/
#ifdef _WIN32
        rc = EnlistInDTC(pdbc, pValue /*pITransaction*/);
             /* Call EnlistInDTC() in xa.cpp to
                  enlist in MS Distributed Transaction Coordinator */
        break;
#else
        return ErrUnlockDbc(SQL_HYC00, pdbc); /* optional feature not implemented */
#endif

    case SQL_ATTR_LOGIN_TIMEOUT:             /* kludge for ACCESS, per DL... */
        pdbc->cLoginTimeout = vParamuint;   /* ignored... */
        UnlockDbc (pdbc);
        return SQL_SUCCESS;

    case SQL_ATTR_METADATA_ID:
        pdbc->fMetadataID = (UWORD)vParamuint; /* How to treat catalog string argument*/
                   /*    SQL_FALSE (default)     not treated as identifier */
                   /*    SQL_TRUE                treated as identifier */
        break;

    case SQL_ATTR_ODBC_CURSORS:         /* normally this is intercepted */
                                        /* and handled by Driver Manager but */
        break;                          /* for static link test, just ignore */

    case SQL_ATTR_PACKET_SIZE:          /* handle PacketSize quietly for Interdev */
        pdbc->cPacketSize = vParamuint;
        break;

    case SQL_ATTR_QUIET_MODE:
        pdbc->hQuietMode = (SQLHANDLE)pValue;
        break;

    case SQL_TXN_ISOLATION:
        /*
        **  This is invalid if a transaction is started.
        */
        if (vParamuint != pdbc->fIsolation)
        {
            /*
            **  If we are already connected, get valid isolation modes for
            **  the driver we are impersonating.  If not, just accept anything
            **  that could possibly be valid and hope for the best.  Note that
            **  SQL_TXN_READ_UNCOMMITTED implies SQL_MODE_READ_ONLY.
            */
            if (pdbc->sessMain.fStatus & SESS_CONNECTED)
            {
                SQLGetInfo_InternalCall (pdbc, SQL_TXN_ISOLATION_OPTION, 
                                          &dw, sizeof (dw), NULL);
                if (!(dw &= vParamuint))
                {
                    if (pdbc->fDriver == DBC_INGRES && pdbc->fRelease
                        >= fReleaseRELEASE20)
                        return ErrUnlockDbc (SQL_HY024, pdbc);
                    else
                        /*  
                        **  Since we really don't support transaction isolation 
                        **  prior to Ingres 2.0, default to serializable if the
                        **  user asked for REPEATABLE_READ or READ_COMMITTED.
                        **  Return SQL_SUCCESS_WITH_INFO if this is the case. 
                        */
                        dw |= SQL_TXN_SERIALIZABLE;
                    
                    if (vParamuint == SQL_TXN_REPEATABLE_READ || 
                        vParamuint == SQL_TXN_READ_COMMITTED)
                        rc = ErrState (SQL_01S02, pdbc, 
                            F_OD0057_IDS_ERR_OPTION_CHANGE); 
                }
            }
            else
            {
                /*
                ** If not connected, all we can do is a sanity check against
                ** the supplied isolation level.
                */
                dw = SQL_TXN_READ_UNCOMMITTED |
                     SQL_TXN_READ_COMMITTED   |
                     SQL_TXN_REPEATABLE_READ  |
                     SQL_TXN_SERIALIZABLE;
                if (!(dw &= vParamuint))
                    return ErrUnlockDbc (SQL_HY024, pdbc);
            }

            pdbc->sessMain.fStatus &= ~SESS_SESSION_SET;  /* Enable SetTransaction */
            pdbc->fIsolation = dw;
        }
        break;

        /*
        ** begin STATEMENT options set at connection level
        */
    case SQL_MAX_LENGTH:
        /*
        **  Don't allow setting truncation longer than we can ever get:
        */
        pdbc->cbValueMax = (SDWORD)vParamuint;
        if (pdbc->cbValueMax > FETMAX)
        {
            pdbc->cbValueMax = FETMAX;
            rc = ErrState (SQL_01000, pdbc, F_OD0017_IDS_MSG_MAXLENGTH);
        }
        pdbc->cMaxLength = pdbc->cbValueMax;
        break;

    case SQL_MAX_ROWS:
        pdbc->crowMax = vParamulen;
        break;

    case SQL_NOSCAN:
        if (vParamuint)
            pdbc->fOptions |= OPT_NOSCAN;
        else
            pdbc->fOptions &= ~OPT_NOSCAN;

        pdbc->fOptionsSet |= OPT_NOSCAN;
        break;

    case SQL_QUERY_TIMEOUT:             
        pdbc->cQueryTimeout = vParamuint;   
        break;

    case SQL_ROWSET_SIZE:     
        pdbc->crowFetchMax = (UWORD) vParamuint;  
        pdbc->crowMax = 0;
        break;

    case SQL_RETRIEVE_DATA:            
        pdbc->cRetrieveData = (UWORD)vParamuint;
        break;
        
    case SQL_BIND_TYPE:     /* SQLExtendedFetch option  */
        pstmt->pARD->BindType = vParamuint;
        break;

    case SQL_KEYSET_SIZE:   /* SQLExtendedFetch option ignored */
        break;

    case SQL_CONCURRENCY:
        if (vParamuint == SQL_CONCUR_READ_ONLY  ||
            vParamuint == SQL_CONCUR_LOCK       ||
            vParamuint == SQL_CONCUR_VALUES)
            pdbc->fConcurrency = (UWORD)vParamuint;
        else if (vParamuint == SQL_CONCUR_ROWVER)
        {
            pdbc->fConcurrency = SQL_CONCUR_VALUES;
            return ErrUnlockDbc (SQL_01S02, pdbc);
        }
        break;

    case SQL_CURSOR_TYPE:
        if (pstmt->fAPILevel >= IIAPI_LEVEL_5)
        {
            if (vParamuint == SQL_CURSOR_FORWARD_ONLY || vParamuint ==
                SQL_CURSOR_STATIC || vParamuint== SQL_CURSOR_KEYSET_DRIVEN)
                pstmt->fCursorType = (UWORD)vParamuint;
            break;
        }
        pdbc->fCursorType = SQL_CURSOR_FORWARD_ONLY;
        return ErrUnlockDbc (SQL_01S02, pdbc);
        break;

    /* unsupported STMT options */
    case SQL_SIMULATE_CURSOR:
    case SQL_TRANSLATE_DLL:     
    case SQL_TRANSLATE_OPTION:   
    case SQL_USE_BOOKMARKS:

        return ErrUnlockDbc(SQL_HYC00, pdbc); 
        /* optional feature not implemented */

    default:

        return ErrUnlockDbc(SQL_HY092, pdbc);
    }    /* end switch (fOption) for connection attributes */

    /*
    **  Statement options get set in all allocated STMT's.
    */
    switch (fOption)    /* statement options */
    {
    case SQL_MAX_LENGTH:                /* Stmt options */
    case SQL_MAX_ROWS:
    case SQL_ROWSET_SIZE:
    case SQL_NOSCAN:
    case SQL_QUERY_TIMEOUT:         
    case SQL_CURSOR_TYPE:
    case SQL_CONCURRENCY:

        /* loop through all statements */
        for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
        {
            rc = SQLSetStmtAttr_InternalCall((SQLHSTMT)pstmt, fOption, 
                (SQLPOINTER)(SCALARP)vParamulen, SQL_NTS);
            if (rc == SQL_ERROR)
            {
                UnlockDbc (pdbc);
                return rc;
            } 
        }  /* end for loop through all statements */
        break;
    }  /* end switch (fOption) for statement options */

    UnlockDbc (pdbc);
    return rc;
}


/*
**  SQLSetConnectOption
**
**  Return the current setting of a connection option.
**  associated with a connection.
**
**  On entry: pdbc   -->connection block.
**            fOption = type of option.
**            pvParam -->where to return option setting.
**
**  Returns:  SQL_SUCCESS
**            SQL_NO_DATA_FOUND
**            SQL_ERROR
*/
RETCODE SQL_API SQLSetConnectOption(
    SQLHDBC    hdbc,
    UWORD      fOption,
    SQLUINTEGER vParam)
{
    SQLINTEGER StringLength = SQL_NTS;

    return(SQLSetConnectAttr_InternalCall(hdbc, fOption, 
                 (SQLPOINTER)(SCALARP)vParam, StringLength));
}

/*
**  SQLSetEnvAttr
**
**  SQLSetEnvAttr sets attributes that govern aspects of environments.
**
**  On entry:
**    EnvironmentHandle[Input]
**    Attribute        [Input]
**    ValuePtr         [Input]
**   *StringLength     [Input]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLSetEnvAttr (
    SQLHENV    EnvironmentHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
    LPENV      penv = (LPENV)EnvironmentHandle;

    if (!penv)
        return SQL_INVALID_HANDLE;

    LockEnv (penv);            /* lock the ENV block */

    ErrResetEnv (penv);        /* clear any errors on ENV */

    switch (Attribute)
    {
    case SQL_ATTR_CONNECTION_POOLING:
        break;                         /* DM does the processing */

    case SQL_ATTR_CP_MATCH:
        break;                         /* DM does the processing */

    case SQL_ATTR_ODBC_VERSION:
/*          SQL_OV_ODBC2
               The driver is to return and is to expect ODBC 2.x codes for
                  date, time, and timestamp data types.
               The driver is to return ODBC 2.x SQLSTATE code when 
                  SQLError, SQLGetDiagField, or SQLGetDiagRec is called.
               The CatalogName argument in a call to SQLTables does not 
                  accept a search pattern.
            SQL_OV_ODBC3
               The driver is to return and is to expect ODBC 3.x codes for
                  date, time, and timestamp data types.
               The driver is to return ODBC 3.x SQLSTATE code when 
                  SQLError, SQLGetDiagField, or SQLGetDiagRec is called.
               The CatalogName argument in a call to SQLTables 
                  accepts a search pattern. */
        penv->ODBCVersion = (i2)(SCALARP)ValuePtr;
        break;

    case SQL_ATTR_OUTPUT_NTS:
         /* Always return SQL_TRUE - 
            the driver always returns string data null-terminated */
        break;
    }

    UnlockEnv (penv);
    return(SQL_SUCCESS);
}


/*
**  SQLSetStmtAttr
**
**  SQLSetStmtAttr sets attributes related to a statement.
**
**  On entry:
**    StatementHandle  [Input]
**    Attribute        [Input]
**    ValuePtr         [Input]
**   *StringLength     [Input]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
**  Notes:
**    Neither an IPD nor IRD can be specified in this function.
**    SQL_ATTR_IMP_PARAM_DESC,
**    SQL_ATTR_IMP_ROW_DESC, and
**    SQL_ATTR_ROW_NUMBER are read-only and may not be set by SQLSetStmtAttr.
**
*/
RETCODE SQL_API SQLSetStmtAttr(
    SQLHSTMT   hstmt,
    SQLINTEGER fOption,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
    return SQLSetStmtAttr_InternalCall(
               hstmt,
               fOption,
               ValuePtr,
               StringLength);
}

RETCODE SQL_API SQLSetStmtAttr_InternalCall(
    SQLHSTMT   hstmt,
    SQLINTEGER fOption,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
    LPSTMT       pstmt  = (LPSTMT)hstmt;
    SQLUINTEGER      vParamulen = (SQLUINTEGER)(SCALARP)ValuePtr;
    RETCODE      rc     = SQL_SUCCESS;
    LPDESC       pdesc;

    TRACE_INTL(pstmt,"SQLSetStmtAttr_InternalCall");
    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    ErrResetStmt (pstmt);        /* clear any errors on STMT */

    switch (fOption)
    {
    case SQL_ATTR_APP_PARAM_DESC:

            /* ValuePtr has handle to the APD to be used for subsequent
               calls to SQLExecute and SQLExecDirect */
        pdesc = (LPDESC)ValuePtr;
        if (pdesc != NULL  &&
            pdesc != pstmt->pAPD_automatic_allocated)
           {
            /* check that descriptor handle is valid and same pdbc;
               else errHY024;
               check that descriptor handle was explicitly allocated */
            if (!STequal("DESC*", pdesc->szEyeCatcher)  ||
                 pdesc->pdbc != pstmt->pdbcOwner)
                     rc = ErrState (SQL_HY024, pstmt);
            else
                 pstmt->pAPD = pdesc;
           }
        else  /* pdesc == SQL_NULL_DESC or == original pAPD_automatic_allocated*/
            pstmt->pAPD = pdesc;
        break;

    case SQL_ATTR_APP_ROW_DESC:

            /* ValuePtr has handle to the ARD to be used for subsequent fetches */
        pdesc = (LPDESC)ValuePtr;
        if (pdesc != NULL  &&
            pdesc != pstmt->pARD_automatic_allocated)
           {
            /* check that descriptor handle is valid and same pdbc;
                  else errHY024;
               check that descriptor handle was explicitly allocated */
            if (!STequal("DESC*", pdesc->szEyeCatcher)  ||
                 pdesc->pdbc != pstmt->pdbcOwner)
                     rc = ErrState (SQL_HY024, pstmt);
            else
                 pstmt->pARD = pdesc;
           }
        else  /* pdesc == SQL_NULL_DESC or == original pARD_automatic_allocated*/
            pstmt->pAPD = pdesc;
        break;

    case SQL_ATTR_ASYNC_ENABLE:

            /* SQL_ASYNC_ENABLE_OFF - execute statements synchronously (default)
               SQL_ASYNC_ENABLE_ON  - execute statements asynchronously */
        pstmt->fAsyncEnable = (UWORD)vParamulen;
        break;

    case SQL_ATTR_CONCURRENCY:

            /* SQL_CONCUR_READ_ONLY - cursor is read-only. no updates (default)
               SQL_CONCUR_LOCK      - cursor uses lowest level of locking
               SQL_CONCUR_ROWVER    - cursor uses optimistic control, 
                                      comparing row versions
               SQL_CONCUR_VALUES    - cursor uses optimistic control, 
                                      comparing values */
        if (vParamulen == SQL_CONCUR_READ_ONLY)
        {
             pstmt->fCursorSensitivity = SQL_INSENSITIVE;
             pstmt->fConcurrency = (UWORD)vParamulen;
        }
        else if ( vParamulen == SQL_CONCUR_LOCK       ||
            vParamulen == SQL_CONCUR_VALUES)
        {
            pstmt->fCursorSensitivity = SQL_UNSPECIFIED;
            pstmt->fConcurrency = (UWORD)vParamulen;
        }
        else
        if (vParamulen == SQL_CONCUR_ROWVER)
           {
            pstmt->fConcurrency = SQL_CONCUR_VALUES;
            pstmt->fCursorSensitivity = SQL_UNSPECIFIED;
            return ErrUnlockStmt (SQL_01S02, pstmt);
           }
        break;

    case SQL_ATTR_CURSOR_SCROLLABLE:
            /* SQL_NONSCROLLABLE - scrollable cursors are not required (default)
               SQL_SCROLLABLE    - scrollable cursors are required */
        if (pstmt->fAPILevel >= IIAPI_LEVEL_5)
        { 
            if (vParamulen == SQL_SCROLLABLE)
                pstmt->fCursorType = SQL_CURSOR_STATIC;
            else
                pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
        }
        else if (vParamulen != SQL_NONSCROLLABLE)
        {
            pstmt->fCursorScrollable = SQL_NONSCROLLABLE;
            return ErrUnlockStmt (SQL_01S02, pstmt);
        }

        pstmt->fCursorScrollable = (UWORD)vParamulen;

        break;

    case SQL_ATTR_CURSOR_SENSITIVITY:

            /* SQL_UNSPECIFIED - unspecified whether changes visible (default)
               SQL_INSENSITIVE - cursors don't reflect changes made by another
               SQL_SENSITIVE   - cursors reflect changes made by another cursor
               Only support SQL_UNSPECIFIED since Ingres can't set sensitivity
               at the cursor level. */
        if (vParamulen == SQL_SENSITIVE)
            return ErrUnlockStmt (SQL_HYC00, pstmt);
        if (pstmt->fAPILevel >= IIAPI_LEVEL_5)
        {
            if (vParamulen == SQL_INSENSITIVE)
            {
                pstmt->fConcurrency = SQL_CONCUR_READ_ONLY;
                pstmt->fCursorType = SQL_CURSOR_STATIC;
            }
            else 
            {
                pstmt->fConcurrency = SQL_CONCUR_VALUES;
                pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
            }

            pstmt->fCursorSensitivity = (UWORD)vParamulen;
        }
        break;

    case SQL_ATTR_CURSOR_TYPE:

            /* SQL_CURSOR_FORWARD_ONLY - cursor only scrolls forward (default)
               SQL_CURSOR_KEYSET_DRIVEN- data in result set static
               SQL_CURSOR_DYNAMIC      - driver saves/uses only keys of rowset
               SQL_CURSOR_STATIC       -result set data static  */

        if (pstmt->fAPILevel >= IIAPI_LEVEL_5)
        {
            switch (vParamulen)
            {
            case SQL_CURSOR_FORWARD_ONLY:
                pstmt->fCursorScrollable = SQL_NONSCROLLABLE;
                break;

            case SQL_CURSOR_STATIC:
                pstmt->fCursorScrollable = SQL_SCROLLABLE;
                if (pstmt->fConcurrency == SQL_CONCUR_READ_ONLY)
                    pstmt->fCursorSensitivity = SQL_INSENSITIVE;
                else
                    pstmt->fCursorSensitivity = SQL_UNSPECIFIED;
                break;

            case SQL_CURSOR_KEYSET_DRIVEN:
                pstmt->fCursorScrollable = SQL_SCROLLABLE;
                pstmt->fCursorSensitivity = SQL_UNSPECIFIED;
                break;

            case SQL_CURSOR_DYNAMIC:
                pstmt->fCursorScrollable = SQL_SCROLLABLE;
                pstmt->fCursorType = SQL_CURSOR_STATIC;
                if (pstmt->fConcurrency == SQL_CONCUR_READ_ONLY)
                    pstmt->fCursorSensitivity = SQL_INSENSITIVE;
                else
                    pstmt->fCursorSensitivity = SQL_UNSPECIFIED;
                return ErrUnlockStmt (SQL_01S02, pstmt);
                break;

            default:
                pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
                return ErrUnlockStmt (SQL_01S02, pstmt);
                break;
            }
        }
        else if (vParamulen != SQL_CURSOR_FORWARD_ONLY)
        {
            pstmt->fCursorType = SQL_CURSOR_FORWARD_ONLY;
            return ErrUnlockStmt (SQL_01S02, pstmt);
        }

        pstmt->fCursorType = (UWORD) vParamulen;

        break;

    case SQL_ATTR_ENABLE_AUTO_IPD:

            /* SQL_FALSE - auto population of IPD after SQLPrepare is off (default)
               SQL_TRUE  - auto population of IPD after SQLPrepare is on  */
        if (vParamulen == SQL_TRUE)
            return ErrUnlockStmt (SQL_HYC00, pstmt);
        break;

    case SQL_ATTR_FETCH_BOOKMARK_PTR:

            /* Pointer to a binary bookmark value 
               used by SQLFetchScroll(SQL_FETCH_BOOKMARK) */
        pstmt->pFetchBookmark = ValuePtr;
        break;

/*  case SQL_ATTR_IMP_PARAM_DESC:           cannot set, only get */

            /* The handle of the stmt's initially allocated IPD descriptor */


/*  case SQL_ATTR_IMP_ROW_DESC:             cannot set, only get */

            /* The handle of the stmt's initially allocated IRD descriptor */


    case SQL_ATTR_KEYSET_SIZE:

        /* Number of rows in the keyset buffer for a keyset-driven
        ** cursor.  Ignored. 
        */
        break;

    case SQL_ATTR_MAX_LENGTH:

            /* Maximum length of character/binary data driver returns */
        /*
        **  Don't allow setting truncation longer than we can ever get:
        */
        pstmt->cMaxLength = vParamulen;
        pstmt->cbValueMax = (SDWORD)(vParamulen <= MAXI4 ? vParamulen : MAXI4);
        if (pstmt->cbValueMax > FETMAX)
        {
            pstmt->cbValueMax = FETMAX;
            rc = ErrState (SQL_01000, pstmt, F_OD0017_IDS_MSG_MAXLENGTH);
            UnlockStmt (pstmt);
            return rc; 
        }
        break;

    case SQL_MAX_ROWS:

            /* Maximum number of rows to return to an application
               for a SELECT statement.  If 0, return all rows */
        pstmt->crowMax = vParamulen;
        break;

    case SQL_ATTR_METADATA_ID:

            /* SQL_FALSE - don't treat as identifiers; case is significant; 
                           may contain search pattern characters. (default)
               SQL_TRUE  - string argument of catalog functions treated as
                           identifiers case not significant; remove trailing
                           spaces; fold to uppercase. */
        pstmt->fMetadataID = (UWORD)vParamulen;
        break;

    case SQL_NOSCAN:

            /* SQL_NOSCAN_OFF - driver scans SQL strings for escape sequences (default)
               SQL_NOSCAN_ON  - Don't scan; send stmt directly to data source  */
        if (vParamulen)
            pstmt->fOptions |= OPT_NOSCAN;
        else
            pstmt->fOptions &= ~OPT_NOSCAN;
        break;

    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:

            /* offset added to pointer to change binding of dynamic parameters.
               always added directly to:
                   SQL_DESC_DATA_PTR
                   SQL_DESC_INDICATOR_PTR
                   SQL_DESC_OCTET_LENGTH_PTR */
        pstmt->pAPD->BindOffsetPtr = ValuePtr;
        break;

    case SQL_ATTR_PARAM_BIND_TYPE:

            /* SQL_PARAM_BIND_BY_COLUMN (default) or 
               length of structure or instance buffer (bind by row) */
        pstmt->pAPD->BindType = (SQLINTEGER)vParamulen;
        break;

    case SQL_ATTR_PARAM_OPERATION_PTR:

            /* array of SQL_PARAM_PROCEED or SQL_PARAM_IGNORE or NULL */
        pstmt->pAPD->ArrayStatusPtr = ValuePtr;
        break;

    case SQL_ATTR_PARAM_STATUS_PTR:

            /* array of statuses */
        pstmt->pIPD->ArrayStatusPtr = ValuePtr;
        break;

    case SQL_ATTR_PARAMS_PROCESSED_PTR:
            /* array of sets of parameter values */
        pstmt->pIPD->RowsProcessedPtr = ValuePtr;
        break;

    case SQL_ATTR_PARAMSET_SIZE:

            /* number of sets of parameter values */
        pstmt->pAPD->ArraySize = vParamulen;
        break;

    case SQL_ATTR_QUERY_TIMEOUT:

            /* number of to wait for SQL statement to execute before timeout */
        pstmt->cQueryTimeout = vParamulen;
        break;

    case SQL_ATTR_RETRIEVE_DATA:        /* kludge for ADODB.Recordset */

            /* SQL_RD_ON  - fetch data after it positions the cursor (default)
               SQL_RD_OFF - don't fetch data after it positions the cursor  */
        pstmt->cRetrieveData = (UWORD)vParamulen;
        break;

    case SQL_ATTR_ROW_ARRAY_SIZE:
        /* 
        ** Number of rows returned by SQLFetch or SQLFetchScroll.
        ** Default rowset size is 1. 
        */
        pstmt->crowFetchMax = (UWORD)pstmt->pARD->ArraySize = (UWORD)vParamulen;
        pstmt->crowMax = 0;
        break;

    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
            /* offset added to pointer to change binding of column data.
               always added directly to:
                  SQL_DESC_DATA_PTR
                  SQL_DESC_INDICATOR_PTR
                  SQL_DESC_OCTET_LENGTH_PTR */
        pstmt->pARD->BindOffsetPtr = ValuePtr;
        break;

    case SQL_ATTR_ROW_BIND_TYPE:
            /* SQL_BIND_BY_COLUMN (default) or length of structure or 
                  instance buffer (bind by row)
               this field was part of ODBC 1.0 but now we use ARD.  */
        pstmt->pARD->BindType = (SQLINTEGER)vParamulen;
        break;

    case SQL_ATTR_ROW_OPERATION_PTR:
            /* array of values used to ignore a row during bulk SQLSetPos
                  SQL_ROW_PROCEED
                  SQL_ROW_IGNORE */
        pstmt->pARD->ArrayStatusPtr = ValuePtr;
        break;

    case SQL_ATTR_ROW_STATUS_PTR:
            /* array of values containing row status values
               after SQLFetch or SQLFetchScroll */
        pstmt->pIRD->ArrayStatusPtr = ValuePtr;
        break;

    case SQL_ATTR_ROWS_FETCHED_PTR:
            /* number of rows fetched after SQLFetch or SQLFetchScroll */
        pstmt->pIRD->RowsProcessedPtr = ValuePtr;
        break;

    case SQL_ATTR_USE_BOOKMARKS:        /* quietly process the unused option */

            /* SQL_UB_OFF      - don't use bookmarks (default)
               SQL_UB_VARIABLE - application will use bookmarks  */
        pstmt->cUseBookmarks = (UWORD)vParamulen;
        break;

    case SQL_ATTR_SIMULATE_CURSOR:

        return ErrUnlockStmt(SQL_HYC00, pstmt); /* optional feature not implemented */

    case SQL_ROWSET_SIZE:

        /* number of rows in 2.x SQLExtendedFetch rowset (default=1) */
        pstmt->crowFetchMax = (UWORD)pstmt->pARD->ArraySize = (UWORD)vParamulen;
        pstmt->crowMax = 0;
        break;

    default:

        return ErrUnlockStmt (SQL_HY092, pstmt);
    }
    UnlockStmt (pstmt);
    return SQL_SUCCESS;
}


/*
**  SQLSetStmtOption
**
**  Set statement option.
**
**  On entry: pstmt  -->statment block.
**            fOption = option.
**            vParam -->option setting.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/
RETCODE SQL_API SQLSetStmtOption(
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLUINTEGER  vParam)
{
    SQLINTEGER StringLength = SQL_NTS;

    return(SQLSetStmtAttr_InternalCall(hstmt, fOption, 
                 (SQLPOINTER)(SCALARP)vParam, StringLength));
}


/*
**  GetAttrLength
**
**  Get length of ODBC version 2 connection attribute.
**
**  On entry: fOption = option.
**
**  Returns:  Attribute length, if fOption is a valid attribute type, 
**            else zero.
*/

SQLINTEGER GetAttrLength( UWORD fOption )
{
   
    /*
    ** Get appropriate lengths for connection options (attributes).  Note
    ** that the version 3 attributes listed below are assumed to map
    ** to their version 2 equivalents, i.e., SQL_ATTR_AUTOCOMMIT maps to
    ** SQL_AUTOCOMMIT.  Version 2 strings should never be longer than
    ** SQL_MAX_OPTION_STRING_LENGTH (256) characters, excluding the null 
    ** terminator.
    */
    switch ( fOption )
    {
    case SQL_ATTR_ACCESS_MODE:
    case SQL_ATTR_ASYNC_ENABLE:
    case SQL_ATTR_AUTOCOMMIT:
    case SQL_ATTR_AUTO_IPD:   
    case SQL_ATTR_CONNECTION_DEAD:
    case SQL_ATTR_CONNECTION_TIMEOUT:
    case SQL_ATTR_LOGIN_TIMEOUT:
    case SQL_ATTR_METADATA_ID:
    case SQL_ATTR_ODBC_CURSORS:
    case SQL_ATTR_PACKET_SIZE:
    case SQL_ATTR_QUIET_MODE:
    case SQL_ATTR_TRANSLATE_OPTION:
    case SQL_ATTR_TXN_ISOLATION:

        return ( sizeof(i4) );  /* Currently ignored by SQLGetConnectAttr() */  

    case SQL_ATTR_CURRENT_CATALOG:
    case SQL_ATTR_TRANSLATE_LIB: 

        return ( SQL_MAX_OPTION_STRING_LENGTH + 1 );

    default:   /* Implies invalid attribute was specified */

        return ( 0 );
    }
}

#if 0                               /* maybe someday... */

static const char SQL_DRIVER_TO_DATASOURCE[] = "SQLDriverToDataSource";
static const char SQL_DATASOURCE_TO_DRIVER[] = "SQLDataSourceToDriver";

/*
**  FreeTransDLL
**
**  Free translation DLL.
**
**  On entry: pdbc -->connection block.
**
**  On exit:  DLL is freed, and all flags cleared.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/
VOID FreeTransDLL(
    LPDBC   pdbc)
{
    LPSTMT  pstmt;

    TRACE_INTL (pdbc, "FreeTransDLL");

    if (pdbc->hTransDLL > HINSTANCE_ERROR)
        FreeLibrary (pdbc->hTransDLL);

    pdbc->fOptions &= ~OPT_TRANSLATE;

    pstmt = pdbc->pstmtFirst;
    while (pstmt != NULL)
    {
        pstmt->fOptions &= ~OPT_TRANSLATE;
        pstmt = pstmt->pstmtNext;
    }
    return;
}


/*
**  LoadTransDLL
**
**  Load translation DLL.
**
**  On entry: pdbc-->connection block.
**
**  On exit:  DLL is loaded, function addresses found, and flag set.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/
RETCODE LoadTransDLL(
    LPDBC       pdbc)
{
    LPSTMT      pstmt;
    HINSTANCE   hinst;
    UCHAR       szParm[20];

    TRACE_INTL (pdbc, "LoadTransDLL");
    /*
    **  Disable translation, if on already:
    */
    FreeTransDLL (pdbc);

    /*
    **  Load library and find translation function addresses:
    */
    pdbc->hTransDLL = LoadLibrary (pdbc->szTransDLL);
    if (pdbc->hTransDLL <= HINSTANCE_ERROR)
        return ErrState (SQL_IM009, pdbc, pdbc->szTransDLL, pdbc->hTransDLL);

    szParm[0] = 0;

    (   PROC)pdbc->pfnSQLDriverToDataSource =
        GetProcAddress (hinst, SQL_DRIVER_TO_DATASOURCE);
    if (pdbc->pfnSQLDriverToDataSource == NULL)
        return ErrState (SQL_IM009, pdbc, SQL_DRIVER_TO_DATASOURCE, 0);

    (   PROC)pdbc->pfnSQLDataSourceToDriver =
        GetProcAddress (hinst, SQL_DATASOURCE_TO_DRIVER);
    if (pdbc->pfnSQLDataSourceToDriver == NULL)
        return ErrState (SQL_IM009, pdbc, SQL_DATASOURCE_TO_DRIVER, 0);

    /*
    **  Set flags on in DBC and all STMT's.
    */
    pdbc->fOptions |= OPT_TRANSLATE;

    pstmt = pdbc->pstmtFirst;
    while (pstmt != NULL)
    {
        pstmt->fOptions |= OPT_TRANSLATE;
        pstmt = pstmt->pstmtNext;
    }

    return SQL_SUCCESS;
}
#endif

/*
** Name: isL2StatementAttr
**
** Description:
**      Determines whether attribute is a Level 2 statement-level attribute.
**
** Input:
**      attr            Attribute.
**
** Output:
**      None.
**
** Returns:
**      TRUE if Level 2 statement-level attribute, otherwise FALSE.
**
** History:
**      29-Jul-2010 (Ralph Loen) Bug 124131
**         Created.
**
*/
BOOL isL2StatementAttr(SQLINTEGER attr)
{
    switch(attr)
    {
    case SQL_QUERY_TIMEOUT:
    case SQL_MAX_ROWS:
    case SQL_NOSCAN:
    case SQL_MAX_LENGTH:
    case SQL_ASYNC_ENABLE:
    case SQL_BIND_TYPE:
    case SQL_CURSOR_TYPE:
    case SQL_CONCURRENCY:
    case SQL_KEYSET_SIZE:
    case SQL_ROWSET_SIZE:
    case SQL_SIMULATE_CURSOR:
    case SQL_RETRIEVE_DATA:
    case SQL_USE_BOOKMARKS:
    case SQL_GET_BOOKMARK:
    case SQL_ROW_NUMBER:
        return TRUE;
    }
    return FALSE;
}
