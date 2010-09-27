/*
** Copyright (c) 1994, 2009 Ingres Corporation
*/

/*
** Name: IDMSODBC.H
**
** Description:
**	Header file for ODBC driver
**
** History:
**	24-nov-1992 (rosda01)
**	    Initial coding
**	10-jan-1997 (tenge01)
**	    Added stmtHandle to tSTMT
**	27-jan-1997 (tenje01)
**	    Added QueryHandle to tSTMT 
**	07-feb-1997 (thoda04)
**	    Moved connH/tranH to SESS
**	12-feb-1997 (tenje01)
**	    Added DBC_INGRES
**	13-feb-1997 (tenje02)
**	    Added COL_MORESEG and icolFetch
**	19-feb-1997 (thoda04)
**	    Added IIAPI_INT4_to_INT2_TYPE cat conversion
**	26-feb-1997 (tenje01)
**	    Added COL_BINDLONG
**	27-feb-1997 (tenje01)
**	    Added STMT_EXECPROC
**	04-mar-1997 (thoda04)
**	    Added IIAPI_CHAR_to_INT2_TYPE cat conversion
**	06-mar-1997 (tenje01)
**	    Modified DBC block to support SQLDriverConnect
**	10-mar-1997 (thoda04)
**	    Bumped up LEN_TEMP to 512
**	20-mar-1997 (tenje01)
**	    Added STMT_CURSOR 
**	09-apr-1997 (tenje01)
**	    renamed COL_MORESEG to COL_MULTISEGS
**	22-apr-1997 (tenge01)
**	    Added new parameters to ParseConvert and ParseEscape
**	08-may-1997 (thoda04)
**	    Added flags for SET AUTOCOMMIT mapping
**	22-may-1997 (tenje01)
**	    Added pstmt to ParseConvert
**	30-may-1997 (thoda04)
**	    Increased supported size of passwords to 32
**	07-jul-1997 (thoda04)
**	    Defined IIapi_xxx functions for LoadLibrary
**	25-jul-1997 (thoda04)
**	    Added cPacketSize
**	29-jul-1997 (tenje01)
**	    Added SESS_AUTOCOMMIT_ON_ISSUED
**	02-sep-1997 (tenje01)
**	    Bumped up NUMVAR from 50 to 301
**	08-sep-1997 (tenje01)
**	    Added pParmBuffer to COL block
**	12-sep-1997 (thoda04)
**	    Change serverclass DBD to STAR
**	17-sep-1997 (thoda04)
**	    Added flags for COMMIT and ROLLBACK mapping
**	12-oct-1997 (thoda04)
**	    Add debugging code for odbc_malloc
**	07-nov-1997 (tenje01)
**	    Added szRoleName and szRolePWD 
**	12-nov-1997 (thoda04)
**	    Added   STMT_API_CURSOR_OPENED
**	12-nov-1997 (thoda04)
**	    Renamed STMT_CURSOR to STMT_WHERE_CURRENT_OF_CURSOR
**	17-nov-1997 (thoda04)
**	    Added db_name_case to DBC
**	03-dec-1997 (thoda04)
**	    Added fActiveStatementsMax
**	04-dec-1997 (tenje01)
**	    Added CL defines 
**	15-dec-1997 (thoda04)
**	    Added OPT_CONVERT_3_PART_NAMES
**	18-dec-1997 (thoda04)
**	    Added db_table_name_length
**	14-jan-1998 (thoda04)
**	    Added RMS server class  
**	06-feb-1998 (thoda04)
**	    Fixed COL offset sizes from 16-bit to 32-bit
**	17-feb-1998 (thoda04)
**	    Added cursor SQL_CONCURRENCY (fConcurrency)
**	27-feb-1998 (thoda04)
**	    Thread local storage fix. GetTls().
**	13-apr-1998 (thoda04)
**	    Added GROUP support.
**	21-jul-1998 (thoda04)
**	    Added cBindType, cKeysetSize, cRetrieveData.
**	26-oct-1998 (thoda04)
**	    Added fields for MTS/DTC.
**	06-jan-1999 (thoda04)
**	    Moved sqldaDefault from ENV to DBC for thread-safeness.
**	13-jan-1999 (thoda04)
**	    Make CatGetTableNameLength a global function.
**	20-jan-1999 (thoda04)
**	    Auto-commit updates at SQLFreeStmt(SQL_CLOSE) if needed.
**	02-feb-1999 (thoda04)
**	    Clean-up unnecessary control blocks.
**	17-feb-1999 (thoda04)
**	    Port to UNIX.
**	22-apr-1999 (thoda04)
**	    Add prepare_or_describe parm to PrepareStmt.
**	30-apr-1999 (thoda04)
**	    Add ERlog, TMnow, TMstr.
**	17-may-1999 (loera01)
**	    Added OPT_SYSTABLES.  
**	09-jun-1999 (thoda04)
**	    Keep descriptor info across closing cursors (iODBC issue)
**	    since iODBC SQLExecute calls internally SQLNumResultCols
**	    which will always set pstmt->sqlca.SQLNRP=0, losing 
**	    the SQLExecute info on SQLNRP.
**	27-oct-1999 (thoda04)
**	    Add szUsername into DBC.
**	02-nov-1999 (thoda04)
**	    Bump up max fetched rowsize to 2G.
**	12-nov-1999 (thoda04)
**	    Added.OPT_BLANKDATEisNULL.
**	12-jan-2000 (thoda04)
**	    Added.OPT_DATE1582isNULL.
**	13-mar-2000 (thoda04)
**	    Add support for Ingres Read Only.
**	31-mar-2000 (thoda04)
**	    Add szDBAname, szSystemOwnername into DBC.
**	25-apr-2000 (thoda04)
**	    Added OPT_CATCONNECT.
**	15-may-2000 (thoda04)
**	    Added STMT_CANBEPREPARED.
**	05-jun-2000 (thoda04)
**	    Added OPT_NUMOVERFLOW (-numeric_overflow=ignore).
**	27-jun-2000 (thoda04)
**	    Bump sizeof col.cbColDef to i4
**	16-oct-2000 (loera01)
**	    Added support for II_DECIMAL (Bug 102666).
**	17-oct-2000 (thoda04)
**	    Added isOld6xRelease
**	30-oct-2000 (thoda04)
**	    Added OPT_CATSCHEMA_IS_NULL to return NULL for TABLE_SCHEM
**	11-dec-2000 (weife01)
**	    Added new internal routing:CountQuestionMark() 
**	    to assist in proper allocation of space for 
**	    queries with parameters. (Bug 102162)    
**	11-jan-2001 (thoda04)
**	    Integrate MTS/DTC support into 2.8
**	02-jul-2001 (loera01)
**	    Cast argument of odbc_malloc as u_i4.
**	12-jul-2001 (somsa01)
**	    Changed cbRolePWD to OFFSET_TYPE, and synced up types of
**	    various variables.
**	26-jul-2001 (thoda04)
**	    Cast argument of odbc_malloc to SIZE_TYPE
**	31-jul-2001 (thoda04)
**	    Add support for parameter offset binding.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-dec-2001 (thoda04)
**	    Additional 64-bit changes for overloaded attributes.
**	11-mar-2002 (thoda04)
**	    Add support for row-producing procedures.
**	16-jan-2002 (thoda04)
**	    Add support for DisableCatUnderscore search char.
**	04-28-2002  (weife01) Bug #106831
**          Add option in advanced tab of ODBC admin which allow the procedure 
**          update with odbc read only driver 
**      27-jun-2002 (weife01) Add DB2UDB to the supported server classes. 
**      18-jul-2002 (loera01) Bug 108301
**          Removed LONGBUFFER from tSTMT and placed it in tDESC. 
**      19-aug-2002 (loera01) Bug 108536
**          First phase of support for SQL_C_NUMERIC.
**      22-aug-2002 (loera01) Bug 108364
**          Added GetAttrLength().
**	30-jul-2002 (weife01) SIR #108411
**	    Made change to functions SQLGetInfo and SQLGetTypeInfo to support
**	    DBMS change to allow the VARCHAR limit to 32k; added internal
**	    function GetSQLMaxLen.
**      25-sep-2002 (loera01) Bug 108800
**          Added reference to IIapi_releaseEnv().  Added envHndl field to
**          tENV to save the environment handle from IIapi_initialize().
**      04-nov-2002 (loera01) Bug 108536
**          Added reference to IIapi_formatData() and IIapi_setEnvParam().
**          Removed dependency on API typedefs.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      02-jun-2003 (loera01) SIR 109643
**          Move ScanPastSpaces() template to idmseini.h.
**      06-may-2003 (weife01) Bug 109945, 110148
**          Reset date, time, timestamp to 19 for sqllen(109945)
**          correct definition for IIAPI_FLT_to_INT4_TYPE(110148)   
**   01-jul-2003 (weife01) SIR #110511
**          Add support for group ID in DSN.
**     22-sep-2003 (loera01)
**          Added bullet-proofing for internal ODBC tracing.
**     02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**     17-oct-2003 (loera01)
**          Changed "IIsyc_" routines to "odbc_".
**     05-nov-2003 (loera01)
**          Added ExecuteSelectStmt() and ExecuteDirect() to make
**          ExecuteStmt() a little easier to read.  Added
**          GetParamValues() so that odbc_putParms() could get
**          AT_EXEC parameter data.
**   13-nov-2003 (loera01)
**          Removed obsolete cbParm and iSection fields from
**          tSTMT structure.  Added fields to tSTMT and tDESCFLD
**          to support sending of true segments.
**   20-nov-2003 (loera01)
**          Remove obsolete parmBuffer from tSTMT structure.
**          Revised DEBUG_MALLOC to include MEreqmem().
**   26-nov-2003 (weife01) Bug 111354
**          Rename DRIVER_NAME from <product>ODBC.DLL to <product>OD35.DLL, so
**          SQLGetInfo for SQL_DRIVER_NAME will return <product>OD35.DLL 
**   04-dec-2003 (loera01)
**          Changed catalog status flags from "STMT_" to "CATLG_".
**          Changed command status flags from "STMT_" to "CMD_".
**   18-dec-2003 (loera01)
**          Rename internal routines to avoid naming conflicts with the
**          CAI/PT Driver Manager.
**   06-jan-2004 (loera01)
**          Provide a template for IIODscps_ScanPastSpaces() to quiet the 
**          compiler.
**   11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**   14-Sep-2004 (loera01) Bug 113042
**          Made odbc_query() external for use by SetTransaction().
**   15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
**   17-nov-2004 (Ralph.Loen@ca.com) Bug 113483
**          Add fReleaseRELEASE30 definition for r3.
**   01-dec-2004 (Ralph.Loen@ca.com) Bug 113553
**          Add cbDBMS_PWD and szDBMS_PWD in tDBC structure.
**   10-dec-2004 (Ralph.Loen@ca.com) SIR 113610
**          Added support for typeless nulls.  Added private definition of
**          SQL_C_TYPE_NULL.
**   15-apr-2005 (lunbr01,loera01) Bug 114319
**          Increase size of prepared statement id (PSID) cache from 32
**          to 256 PSIDs.  Also add high PSID and cursor id in connection
**          block.
**   27-sep-2005 (loera01) Bug 115051
**          Added prevOctetLength field to row descriptor (tDESC) struct.
**   01-dec-2005 (loera01) Bug 115358
**          Added argument to SQLPrepare_InternalCall() so that the statement
**          handle buffers aren't freed when a statement is re-prepared.
**   02-dec-2005 (loera01) Bug 115582
**          Added SQL_01S07 (fractional truncation) error.
**   10-Mar-2006 (Ralph Loen) Bug 115833
**          Added multibyteFillChar field in the DBC control block.
**   16-jun-2006 (Ralph Loen) SIR 116260
**          Added STMT_PARAMS_DESCRIBED to support SQLDescribeParam(). 
**   03-Jul-2006 (Ralph Loen) SIR 116326
**          Add template for SQLBrowseConnect_InternalCall().
**   14-Jul-2006 (Ralph Loen) SIR 116385
**          Add support for SQLTablePrivileges().
**   21-jul-06 (Ralph Loen)  SIR 116410
**          Add support for SQLColumnPrivileges().
**   26-jul-06 (Ralph Loen) SIR 116417
**          Added TABLEPRIV_ROW structure and table privileges queue in
**          tSTMT to support linked-list storage of SQLTablePrivileges() 
**          raw result set.
**   07-aug-06 (Ralph Loen) 
**          Made db_table_name_length signed in tSTMT signed to silence 
**          compiler warnings. 
**     10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase).
**          Adds fAPILevel field to connection and statement control blocks
**          to determine the GCA level of the server.
**     28-Sep-2006 (Ralph Loen) SIR 116477
**          Adjusted treatment of ISO date/times according to new API/ADF rules.
**     03-Oct-2006 (Ralph Loen) SIR 116477
**          Dynamic parameters for dates and timestamps are now sent as binary
**          values for backward-compatibility with ingresdate.
**     22-Nov-2006 (Ralph Loen) SIR 116477
**          Added IsoTimePrecision to tDESC (IRD and ARD) structure.
**     21-Aug-2007 (Ralph Loen) Bug 118993
**          Remove unused pcrowParm field from the statement handle.
**     20-Sep-2007 (weife01) SIR 119140
**          Added support for new DSN/connection string configuration
**          option:"Convert int8 from DBMS to int4".
**     19-Oct-2007 (weife01) BUG 119274
**          Driver no longer converts decimal binded param if backend is
**          gateway
**     01-apr-2008 (weife01) Bug 120177
**          Added DSN configuration option "Allow update when use pass through
**          query in MSAccess".
**   09-Jan-2008 (Ralph Loen) SIR 119723 
**          Add fields related to scrollable cursors in tSTMT. 
**   30-Jan-2008 (Ralph Loen) SIR 119723
**          Added sqlstate SQL_HY107 and tSTMT fields fCursorPosition, 
**          fCurrentCursPosition and fCursLastResSetSize.
**   07-Feb-2008 (Ralph Loen) Sir 119723
**          Obsoleted cRowsetSize, cBindType, and cKeysetSize from tDBC,
**          cBindType and cKeysetSize from tSTMT.
**   08-May-2008 (Ralph Loen) SIR 120340
**          Added szPSqlStr and cbPSqlStr to tSTMT to store queries executed
**          from SQLPrepare() that cannot be prepared.
**   12-May-2008 (Ralph Loen) Bug 120356
**          Added cvtToChar in tSTMT to treat SQL_C_DEFAULT as SQL_C_CHAR for
**          Unicode strings.
**     22-Aug-2008 (Ralph Loen) Bug 120814
**          Added blobLen field to tDESCFLD structure. 
**     02-Oct-2008 (Ralph Loen) SIR 117537
**          Added max_decprec to tDESC structure.
**     06-Oct-2008 (Ralph Loen) SIR 121019
**          Added STMT_RETPROC flag to tSTMT structure.
**     03-Nov-2008 (Ralph Loen) SIR 121152
**         Added support for II_DECIMAL in a connection string.
**         The field szDecimal in tDBC contains the value of II_DECIMAL,
**         and is assumed to be a comma or a period.
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Replace obsolete OPT_DBCS with OPT_INGRESDATE.  Add support for
**         date connection level to ConvertEscAndMarkers().
**     14-Dec-1008 (Ralph Loen) Bug 121384
**         Revised output length argument of GetSQLMaxLen() to DWORD instead
**         of UCHAR.
**     15-Dec-2008 (Ralph Loen) Bug 121393
**         Add sql_max_nchr_column_len and sql_max_nvchar_column_len
**         to tDBC.
**     11-Feb-2009 (Ralph Loen) Bug 121639
**         Obsolete the cRowsetSize field of tSTMT, which is intended for
**         Level 2 compliancy, with the ArraySize field of the ARD.
**     12-Feb-2009 (Ralph Loen) bug 121649
**         Added boolean argument rePrepared to ParseCommand().
**     28-Apr-2009 (Ralph Loen) SIR 122007
**         Replace obsolete configuration option OPT_NOCAID with
**         OPT_STRINGTRUNCATION.
**	24-Aug-2009 (kschendel) 121804
**	    Match declaration type for SQLGetPrivateProfileString() which
**	    is SQLINTEGER not int, for aux-info matching.
**     15-Oct-2009 (Ralph Loen) Bug 122675 
**         Add is_unicode_enabled to tDBC structure.
**     10-Nov-2009 (Ralph Loen) Bug 122866
**         Added STMT_SQLEXECUTE statement handle status.  Removed rePrepare
**         arguments from ParseCommand() and SQLPrepare_InternalCall().
**     19-Nov-2009 (Ralph Loen) Bug 122941
**         Re-instate rePrepared arguments to SQLPrepare_InternalCall() and
**         ParseCommand().  Add reExecuteCount field to tSTMT to track
**         executions of SQLExecDirect_Common() from SQLExecute().
**     14-Dec-2009 (Ralph Loen) Bug 123058
**         Cast the last argument of GetSQLMaxLen() an i4 instead of a
**         DWORD to silence compiler warnings on 64-bit Solaris.
**     05-Mar-2010 (Ralph Loen) SIR 123378
**         Changed CATALOG_NAME_SIZE from 32 to 257.  Modified static
**         schema, owner, table, column, procedure name bufferw with a
**         new OBJECT_NAME_SIZE.  Added max_owner_name_length and 
**         max_procedure_name_length to tDBC.
**     23-Mar-2010 (Ralph Loen) SIR 123465
**         Add function definition odbc_hexdump().
**     22-Apr-2010 (Ralph Loen) Bug 123614
**         Remove unused IIGetPrivateProfileString().
**     23-Apr-2010 (Ralph Loen) Bug 123629
**         Added tDBC attributes browseConnectCalled and bcOutStr.  Added
**         SQLSTATE SQL_01S00.
**     14-Jul-2010 (Ralph Loen) Bug 124079 
**         Add fServerClassVECTORWISE server class.  Note that a
**         Vectorwise server is assumed to have the capabilities of an 
**         Ingres server by default.
**    13-Aug-2010 (Ralph Loen) Bug 124235
**         Added ErrGetSqlcaMessageLen().
**     06-Sep-2010 (Ralph Loen) Bug 124348
**          Added version of SQLColAttribute_InternalCall() dependent on 
**          the _WIN64 macro for compatibility with MS implementation.
*/
#ifndef _INC_IDMSODBC
#define _INC_IDMSODBC

#ifdef __cplusplus
extern "C"{
#endif 

#include <cs.h>
#include <qu.h>
#include <iiapi.h>
/*
**  Driver DLL name for various strings:
*/
#ifdef _WIN32
#define IDMS_DRIVER_PGM "CAIDOD32"
#else
#define IDMS_DRIVER_PGM "CAIDODBC"
#endif

#if defined(WIN16)
#define DRIVER_NAME  "CAIIODBC.DLL"
# elif defined(WIN32)
#define DRIVER_NAME  "CAIIOD35.DLL"
# elif defined(UNIX)
#define DRIVER_NAME  "iiodbcdriver.1.so"
#else
#define DRIVER_NAME  " "
#endif

/* if 64bit memory access is supported */
# ifndef SIZE_TYPE
# if defined(size_t) || defined(_SIZE_T_DEFINED)
# define SIZE_TYPE      size_t
# else
# define SIZE_TYPE      unsigned int
# endif
# endif

/*
**  Internal constants:
*/
#ifdef  WIN16
#define FETMAX          0x7fff      /* Max size (32K) of fetch buffer */
#else
#define FETMAX          0x7fffffff  /* Max size (2G) of fetch buffer  */
#endif

#ifndef FETROW
#define FETROW          100         /* Default number rows to fetch */
#endif

#ifndef NUMVAR
#define NUMVAR          301         /* Default number of SQLVARs */
#endif

#define CURSOR_PREFIX  "CUR_$"      /* Generated cursor name prefix */
#define OBJECT_NAME_SIZE 257

#define IIAPI_xxx_to_xxx_TYPE   0X1000  /* catalog conversion of some kind  */
#define IIAPI_CHA_to_VCH_TYPE   0X1415  /* catalog char-->varchar conversion*/
#define IIAPI_INT4_to_INT2_TYPE 0X101E  /* catalog int4-->int2    conversion*/
#define IIAPI_INT2_to_INT4_TYPE 0X1E10  /* catalog int2-->int4    conversion*/
#define IIAPI_CHR_to_INT2_TYPE  0X141E  /* catalog char-->int2    conversion*/
#define IIAPI_CHR_to_CHR1_TYPE  0X1414  /* catalog char-->char1   conversion*/
#define IIAPI_VCH_to_CHR1_TYPE  0X1514  /* catalog vchr-->char1   conversion*/
#define IIAPI_VCH_to_INT2_TYPE  0X151E  /* catalog vchr-->int2    conversion*/
#define IIAPI_FLT_to_INT2_TYPE  0X1F1E  /* catalog float-->int2   conversion*/
#define IIAPI_FLT_to_INT4_TYPE  0X101F  /* catalog float-->int4   conversion*/

#ifdef _WIN32
typedef int   (__cdecl *INTPROC)();
typedef void* (__cdecl *PTRPROC)();
typedef void  (__cdecl *VOIDPROC)();
typedef long  (__cdecl *LONGPROC)();
#endif

#define SQL_WCHAR           (-8)
#define SQL_WVARCHAR        (-9)
#define SQL_WLONGVARCHAR    (-10)
#define SQL_C_WCHAR         SQL_WCHAR

#ifdef UNICODE
#define SQL_C_TCHAR		SQL_C_WCHAR
#else
#define SQL_C_TCHAR		SQL_C_CHAR
#endif 

#define SQL_C_TYPE_NULL     SQL_TYPE_NULL

#define SQL_SQLSTATE_SIZEW	10	/* size of SQLSTATE for unicode */

#ifdef _WIN32
#define INSTAPI __stdcall
#undef  EXPORT              /* because of stupid coding in <sqltypes.h> */
#define EXPORT
#else
#define INSTAPI EXPORT FAR PASCAL
#endif  /* endif WIN32 */

SQLINTEGER  INSTAPI SQLGetPrivateProfileString( LPCSTR, LPCSTR, LPCSTR, 
    LPSTR, int, LPCSTR);

/*
**  SQL STATE values returned by driver itself:
*/
#define SQL_01000        1
#define SQL_01004        2
#define SQL_01S02        3
#define SQL_01S05        4
#define SQL_07001        5
#define SQL_07006        6
#define SQL_22001        7
#define SQL_22002        8
#define SQL_22003        9
#define SQL_22008       10
#define SQL_22018       11
#define SQL_22026       12
#define SQL_23000       13
#define SQL_24000       14
#define SQL_25000       15
#define SQL_34000       16
#define SQL_3C000       17
#define SQL_42000       18
#define SQL_IM007       19
#define SQL_IM009       20
#define SQL_HY000       21
#define SQL_HY001       22
#define SQL_HY004       24  /* invalid SQL data type */
#define SQL_HY008       25  /* operation canceled */
#define SQL_HY009       26
#define SQL_HY010       27
#define SQL_HY011       28  /* attribute cannot be set now */
#define SQL_HY015       29  /* no cursor name available */
#define SQL_HY090       30  /* invalid string or buffer length */
#define SQL_HY091       31  /* invalid descriptor field identifier */
#define SQL_HY092       32
#define SQL_S1094       33  /* invalid scale (2.x only) */
#define SQL_HY096       34  /* invalid information type */
#define SQL_HY104       35  /* invalid precision or scale */
#define SQL_HY105       36  /* invalid parameter type */
#define SQL_HYC00       37
#define SQL_HYC00       37  /* optional feature not implemented */
#define SQL_S1DE0       38
#define SQL_IM001       39

#define SQL_HY021       40
#define SQL_HY024       41
#define SQL_HY106       42
#define SQL_07009       43  /* invalid descriptor index */
#define SQL_HY016       44
#define SQL_HY019       45
#define SQL_HY007       46
#define SQL_01S07       47
#define SQL_HY107       48
#define SQL_01S00       49

/*
**  Make return code type equivalent for ODBC 2.1 and 2.5
*/
/*#if (ODBCVER >= 0x0250)
typedef SQLRETURN RETCODE;
#else
typedef RETCODE SQLRETURN;
#endif
*/

/*
**  ODBC_CRITICAL_SECTION
*/
#define xDEBUG_ODBCCS
#if  defined(_WIN32)  &&  !defined(DEBUG_ODBCCS)
#define ODBC_CRITICAL_SECTION          CRITICAL_SECTION
#define ODBCInitializeCriticalSection  InitializeCriticalSection
#define ODBCDeleteCriticalSection      DeleteCriticalSection
#define ODBCEnterCriticalSection       EnterCriticalSection
#define ODBCLeaveCriticalSection       LeaveCriticalSection

#else  /* not _WIN32 */
#ifdef OS_THREADS_USED
typedef struct tODBC_CRITICAL_SECTION
   {
     CS_SYNCH      mutex;
     CS_THREAD_ID  threadid;
     int           usagecount;
   } ODBC_CRITICAL_SECTION;

#define ODBCInitializeCriticalSection  IIODcsin_ODBCInitializeCriticalSection
#define ODBCDeleteCriticalSection      IIODcsde_ODBCDeleteCriticalSection
#define ODBCEnterCriticalSection       IIODcsen_ODBCEnterCriticalSection
#define ODBCLeaveCriticalSection       IIODcslv_ODBCLeaveCriticalSection
void IIODcsin_ODBCInitializeCriticalSection(ODBC_CRITICAL_SECTION *sptr);
void IIODcsde_ODBCDeleteCriticalSection(ODBC_CRITICAL_SECTION *sptr);
void IIODcsen_ODBCEnterCriticalSection(ODBC_CRITICAL_SECTION *sptr);
void IIODcslv_ODBCLeaveCriticalSection(ODBC_CRITICAL_SECTION *sptr);

#else  /* not OS_THREADS_USED */
typedef int ODBC_CRITICAL_SECTION;
#define ODBCInitializeCriticalSection(cs)
#define ODBCDeleteCriticalSection(cs)
#define ODBCEnterCriticalSection(cs) 
#define ODBCLeaveCriticalSection(cs) 
#endif /* end if OS_THREADS_USED */
#endif /* end if defined(_WIN32)  &&  !defined(DEBUG_ODBCCS) */

/*
**  Memory leak detection (optional)...
*/
#ifdef DEBUG_MALLOC
#include <windowsX.h>
static LPVOID lpDebug;
#define MEreqmem(x, cb, x1, x2) UtMalloc(cb, 0)
#define MEfree(lp) UtFree(lp,0)
#define odbc_malloc(cb) UtMalloc(cb, 0) 
#define odbc_free(lp)   UtFree(lp, 0) 
#define UtMalloc(cb, h) \
        (lpDebug = (void     *)GlobalAllocPtr (GPTR, cb), \
         TRdisplay ("ODBC Debug: malloc at line %4d in %s:\t 0x%08lX  size=%u\r\n", \
                 __LINE__, __FILE__, (long int)lpDebug, (unsigned)cb), lpDebug)
#define UtFree(lp, h) \
        ((lp)?(TRdisplay ("ODBC Debug: free   at line %4d in %s:\t 0x%08lX\r\n", \
                  __LINE__, __FILE__, (long int)lp), \
              GlobalFreePtr (lp)) : 0)
#ifdef origDEBUG_MALLOC
        (TRdisplay ("ODBC Debug: free   at line %4d in %s:\t 0x%08lX\r\n", \
                  __LINE__, __FILE__, (long int)lp), \
          (lp) ? GlobalFreePtr (lp) : 0)
#endif  /* end origDEBUG_MALLOC */ 
#else
VOID *  odbc_malloc(SIZE_TYPE size);
VOID odbc_free  (VOID * buffer);
#define UtMalloc(cb, h) GlobalAllocPtr (GPTR, cb)
#define UtFree(lp, h)   if (lp) GlobalFreePtr (lp)
#endif

/*
**  ODBC DLL macros:
*/
#define MSGLEN          256  /* driver message buffer length */
#define SQLCA_ERROR(p,s) ErrSetSqlca(&(p->sqlca),p->szSqlState,&(p->pMessage),s)
#define UMIN(a,b)       (UDWORD)min((SDWORD)(a),(SDWORD)(b))
#ifndef TRACE_MSG
#define TRACE_MSG       "ODBC Trace: %s\n"
#define ERROR_MSG       "ODBC Error: %s\n"
#endif
# define ODBC_TRACE_LEVEL              \
        (IItrace.fTrace ? IItrace.fTrace : (i4)-1)
# define ODBC_TR_TRACE               0x0001
# define ODBC_EX_TRACE               0x0003
# define ODBC_IN_TRACE               0x0005

#define TRACE(o,m,f)    if (o) TRdisplay(m, f)
# define ODBC_EXEC_TRACE(level) if ( (level) <= ODBC_TRACE_LEVEL )  TRdisplay

#define TRACE_INTL(p,s) if ( (p) && (IItrace.fTrace >= ODBC_IN_TRACE) ) \
    TRdisplay ( TRACE_MSG, s )

/*
**  Define NOPSQL for limited local testing:
*/
#ifdef NOPSQL
#define SQL(sqb,com,p2,p3,p4,p5,p6,p7,p8)
#endif

#ifndef MAX_PATH
#define MAX_PATH    260 /* max. length of full pathname */
#endif

#ifdef NT_GENERIC
#define DEFAULT_PROMPTUIDPWD "Y"
#else
#define DEFAULT_PROMPTUIDPWD "N"   /* UNIX cannot prompt */
#endif

#ifndef NT_GENERIC    /* avoid global name conflicts under non-Windows environ
                         as linker/loader resolves names into Driver Manager */
#define SQLGetPrivateProfileString IIODBCGetPrivateProfileString
#define AllocConnect               IIODalcn_AllocConnect
#define AllocDescriptor            IIODalds_AllocDescriptor
#define AllocDesc_InternalCall     IIODaldi_AllocDesc_InternalCall
#define AllocEnv                   IIODalen_AllocEnv
#define AllocStmt                  IIODalst_AllocStmt
#define AllocPreparedStmtID        IIODalid_AllocPreparedStmtID
#define AnyIngresCursorOpen        IIODanop_AnyIngresCursorOpen
#define CacheFree                  IIODcahf_CacheFree
#define CacheGet                   IIODcahg_CacheGet
#define CachePut                   IIODcahp_CachePut
#define CatFetchBuffer             IIODcatf_CatFetchBuffer
#define CatGetDBcapabilities       IIODcatc_CatGetDBcapabilities
#define CatGetDBconstants          IIODcato_CatGetDBconstants
#define CatGetTableNameLength      IIODcatl_CatGetTableNameLength
#define CatPrepareSqlda            IIODcatp_CatPrepareSqlda
#define CatSqlsid                  IIODcati_CatSqlsid
#define CatToDB_NAME_CASE          IIODcatn_CatToDB_NAME_CASE
#define CheckConsistencyOfDescriptorField \
                                   IIODchdf_CheckConsistencyOfDescriptorField
#define CloseCursors               IIODclcu_CloseCursors
#define ConvertParmMarkers         IIODcopm_ConvertParmMarkers
#define ConvertEscAndMarkers             IIODcopm_ConvertEscAndMarkers
#define CountLiteralItems          IIODcoqm_CountLiteralItems
#define CursorOpen                 IIODcuop_CursorOpen
#define CvtBigFloat                IIODcvgf_CvtBigFloat
#define CvtBinChar                 IIODcvbc_CvtBinChar
#define CvtCharBig                 IIODcvcg_CvtCharBig
#define CvtCharBin                 IIODcvcb_CvtCharBin
#define CvtCharDate                IIODcvcd_CvtCharDate
#define CvtCharFloat               IIODcvcf_CvtCharFloat
#define CvtCharInt                 IIODcvci_CvtCharInt
#define CvtCharNum                 IIODcvcn_CvtCharNum
#define CvtCharNumStr              IIODcvcs_CvtCharNumStr
#define CvtCharSqlNumeric          IIODcvcn_CvtCharSqlNumeric
#define CvtCharTime                IIODcvct_CvtCharTime
#define ConvertCharToWChar         IIODcvcw_ConvertCharToWChar
#define CvtCharTimestamp           IIODcvcm_CvtCharTimestamp
#define CvtCurrStampDate           IIODcvsd_CvtCurrStampDate
#define CvtDateChar                IIODcvfc_CvtDateChar
#define CvtDoubleToChar            IIODcvdb_CvtDoubleToChar
#define CvtEditDate                IIODcved_CvtEditDate
#define CvtEditTime                IIODcvet_CvtEditTime
#define CvtEditTimestamp           IIODcvem_CvtEditTimestamp
#define CvtFloatNum                IIODcvfn_CvtFloatNum
#define CvtGetCType                IIODcvgy_CvtGetCType
#define CvtGetSqlType              IIODcvgs_CvtGetSqlType
#define CvtGetToken                IIODcvgt_CvtGetToken
#define CvtIntNum                  IIODcvin_CvtIntNum
#define CvtNumChar                 IIODcvnc_CvtNumChar
#define CvtNumNum                  IIODcvnn_CvtNumNum
#define CvtPack                    IIODcvpk_CvtPack
#define CvtSkipSpace               IIODcvss_CvtSkipSpace
#define CvtSqlNumericDec           IIODcvcn_CvtSqlNumericDec
#define CvtTimeChar                IIODcvtc_CvtTimeChar
#define CvtTimestampChar           IIODcvmc_CvtTimestampChar
#define CvtTimestampChar_DB2       IIODcvm2_CvtTimestampChar_DB2
#define ConvertUCS2ToUCS4          IIODcv24_ConvertUCS2ToUCS4
#define ConvertUCS4ToUCS2          IIODcv42_ConvertUCS4ToUCS2
#define CvtUltoa                   IIODcvua_CvtUltoa
#define CvtUnpack                  IIODcvup_CvtUnpack
#define ConvertWCharToChar         IIODcvwc_ConvertWCharToChar
#define ErrFlushErrorsToSQLCA      IIODerfe_ErrFlushErrorsToSQLCA
#define ErrResetDbc                IIODerrc_ErrResetDbc
#define ErrResetDesc               IIODerrd_ErrResetDesc
#define ErrResetEnv                IIODerre_ErrResetEnv
#define ErrResetStmt               IIODerrs_ErrResetStmt
#define ErrSetCvtMessage           IIODersm_ErrSetCvtMessage
#define ErrSetSqlca                IIODersc_ErrSetSqlca
#define ErrSetToken                IIODersk_ErrSetToken
#define ErrState                   IIODerst_ErrState
#define ErrUnlockDbc               IIODeruc_ErrUnlockDbc
#define ErrUnlockDesc              IIODerud_ErrUnlockDesc
#define ErrUnlockStmt              IIODerus_ErrUnlockStmt
#define ExecuteStmt                IIODexst_ExecuteStmt
#define FetchRow                   IIODfero_FetchRow
#define FetchRowset                IIODfers_FetchRowset
#define FindDescIRDField           IIODfiir_FindDescIRDField
#define FreeConnect                IIODfrcn_FreeConnect
#define FreeDescriptor             IIODfrds_FreeDescriptor
#define FreeEnv                    IIODfren_FreeEnv
#define FreeStmt                   IIODfrst_FreeStmt
#define FreePreparedStmtID         IIODfrid_FreePreparedStmtID
#define FreeSqlStr                 IIODfrsz_FreeSqlStr
#define FreeSqlcaMsgBuffers        IIODfrmb_FreeSqlcaMsgBuffers
#define FreeStmtBuffers            IIODfrsb_FreeStmtBuffers
#define GenerateCursor             IIODgncu_GenerateCursor
#define GetAttrLength              IIODgbdp_GetAttrLength
#define GetBoundDataPtrs           IIODgbdp_GetBoundDataPtrs
#define GetChar                    IIODgtch_GetChar
#define GetCharCharCommon          IIODgtcc_GetCharCharCommon
#define GetCharAndLength           IIODgtcl_GetCharAndLength
#define GetColumn                  IIODgtco_GetColumn
#define GetDescBoundCount          IIODgtdb_GetDescBoundCount
#define GetDescCaseSensitive       IIODgtdc_GetDescCaseSensitive
#define GetDescDisplaySize         IIODgtdz_GetDescDisplaySize
#define GetDescLiteralPrefix       IIODgtdp_GetDescLiteralPrefix
#define GetDescLiteralSuffix       IIODgtds_GetDescLiteralSuffix
#define GetDescRadix               IIODgtdx_GetDescRadix
#define GetDescSearchable          IIODgtd1_GetDescSearchable
#define GetDescTypeName            IIODgtdy_GetDescTypeName
#define GetDescUnsigned            IIODgtdu_GetDescUnsigned
#define GetInfoBit                 IIODgtib_GetInfoBit
#define GetProcedureReturn         IIODgtpr_GetProcedureReturn
#define GetServerInfo              IIODgtsi_GetServerInfo
#define GetTuplePtrs               IIODgtpt_GetTuplePtrs
#define GetWChar                   IIODgtwc_GetWChar
#define IIsyc_AutoCommit           IIODapac_IIsyc_AutoCommit
#define IIsyc_cancel               IIODapcn_IIsyc_cancel
#define IIsyc_close                IIODapcl_IIsyc_close
#define IIsyc_commitTran           IIODaptr_IIsyc_commitTran
#define IIsyc_execute              IIODapex_IIsyc_execute
#define IIsyc_free                 IIODapfr_IIsyc_free
#define IIsyc_getQInfo             IIODapgq_IIsyc_getQInfo
#define IIsyc_getResult            IIODapgr_IIsyc_getResult
#define IIsyc_malloc               IIODapme_IIsyc_malloc
#define IIsyc_rollbackTran         IIODaprb_IIsyc_rollbackTran
#define InsertSqlcaMsg             IIODinms_InsertSqlcaMsg
#define isApiCursorNeeded          IIODisan_isApiCursorNeeded
#define LockDbc                    IIODlkco_LockDbc
#define LockDbc_Trusted            IIODlkct_LockDbc_Trusted
#define LockDesc                   IIODlkds_LockDesc
#define LockEnv                    IIODlken_LockEnv
#define LockStmt                   IIODlkst_LockStmt
#define ParmLength                 IIODpmln_ParmLength
#define ParseCommand               IIODpacm_ParseCommand
#define ParseConvert               IIODpacv_ParseConvert
#define ParseEscape                IIODpaes_ParseEscape
#define ParseTrace                 IIODpatr_ParseTrace
#define PopSqlcaMsg                IIODpoms_PopSqlcaMsg
#define PrepareConstant            IIODprcn_PrepareConstant
#define PrepareParms               IIODprpa_PrepareParms
#define PrepareSqlda               IIODprda_PrepareSqlda
#define PrepareSqldaAndBuffer      IIODprdb_PrepareSqldaAndBuffer
#define PrepareStmt                IIODprst_PrepareStmt
#define PreparedStmt               IIODprds_PreparedStmt
#define ReallocDescriptor          IIODrads_ReallocDescriptor
#define ResetDbc                   IIODrscn_ResetDbc
#define ResetDescriptorFields      IIODrsdf_ResetDescriptorFields
#define ResetStmt                  IIODrsst_ResetStmt
#define SQLColAttribute_InternalCall \
                                   IIODsqca_SQLColAttribute_InternalCall
#define SQLColAttributes_InternalCall \
                                   IIODsqcb_SQLColAttributes_InternalCall
#define SQLColumns_InternalCall    IIODsqcl_SQLColumns_InternalCall
#define SQLColumnPrivileges_InternalCall \
                                   IIODsqcl_SQLColumnPrivileges_InternalCall
#define SQLConnect_InternalCall    IIODsqcn_SQLConnect_InternalCall
#define SQLDescribeCol_InternalCall \
                                   IIODsqdc_SQLDescribeCol_InternalCall
#define SQLBrowseConnect_InternalCall \
                                   IIODsqdr_SQLBrowseConnect_InternalCall
#define SQLDriverConnect_InternalCall \
                                   IIODsqdr_SQLDriverConnect_InternalCall
#define SQLError_InternalCall      IIODsqer_SQLError_InternalCall
#define SQLExecDirect_InternalCall IIODsqed_SQLExecDirect_InternalCall
#define SQLForeignKeys_InternalCall \
                                   IIODsqfk_SQLForeignKeys_InternalCall
#define SQLGetConnectAttr_InternalCall \
                                   IIODsqgc_SQLGetConnectAttr_InternalCall
#define SQLGetCursorName_InternalCall \
                                   IIODsqgu_SQLGetCursorName_InternalCall
#define SQLGetDescField_InternalCall \
                                   IIODsqg1_SQLGetDescField_InternalCall
#define SQLGetDescRec_InternalCall \
                                   IIODsqg2_SQLGetDescRec_InternalCall
#define SQLGetDiagField_InternalCall \
                                   IIODsqgf_SQLGetDiagField_InternalCall
#define SQLGetDiagRec_InternalCall \
                                   IIODsqgr_SQLGetDiagRec_InternalCall
#define SQLGetInfo_InternalCall    IIODsqgi_SQLGetInfo_InternalCall
#define SQLGetStmtAttr_InternalCall \
                                   IIODsqgs_SQLGetStmtAttr_InternalCall
#define SQLGetTypeInfo_InternalCall \
                                   IIODsqgt_SQLGetTypeInfo_InternalCall
#define SQLNativeSql_InternalCall  IIODsqnq_SQLNativeSql
#define SQLPrepare_InternalCall    IIODsqpr_SQLPrepare_InternalCall
#define SQLPrimaryKeys_InternalCall \
                                   IIODsqpk_SQLPrimaryKeys_InternalCall
#define SQLProcedures_InternalCall \
                                   IIODsqpc_SQLProcedures_InternalCall
#define SQLProcedureColumns_InternalCall \
                                   IIODsqpc_SQLProcedureColumns_InternalCall
#define SQLSetConnectAttr_InternalCall \
                                   IIODsqsa_SQLSetConnectAttr_InternalCall
#define SQLSetCursorName_InternalCall \
                                   IIODsqsc_SQLSetCursorName_InternalCall
#define SQLSetDescField_InternalCall \
                                   IIODsqsf_SQLSetDescField_InternalCall
#define SQLSetStmtAttr_InternalCall \
                                   IIODsqss_SQLSetStmtAttr_InternalCall
#define SQLSpecialColumns_InternalCall \
                                   IIODsqsp_SQLSpecialColumns_InternalCall
#define SQLStatistics_InternalCall IIODsqst_SQLStatistics_InternalCall
#define SQLTables_InternalCall     IIODsqtb_SQLTables_InternalCall
#define SQLTablePrivileges_InternalCall \
                                   IIODsqtb_SQLTablePrivileges_InternalCall
#define SQLTransact_InternalCall   IIODsqtr_SQLTransact_InternalCall
#define ScanNumber                 IIODscnm_ScanNumber
#define ScanPastSpaces             IIODscps_ScanPastSpaces
char *IIODscps_ScanPastSpaces ( char * );
#define ScanQuotedString           IIODscqs_ScanQuotedString
#define ScanRegularIdentifier      IIODscid_ScanRegularIdentifier
#define ScanSearchCondition        IIODscco_ScanSearchCondition
#define SetColOffset               IIODstco_SetColOffset
#define SetColVar                  IIODstcv_SetColVar
#define SetDescDefaultsFromType    IIODstdf_SetDescDefaultsFromType
#define SetServerClass             IIODstsc_SetServerClass
#define SetTransaction             IIODsttr_SetTransaction
#define SqlToIngresAPI             IIODsqap_SqlToIngresAPI
#define TraceOdbc                  IIODtrod_TraceOdbc
#define UnlockDbc                  IIODulco_UnlockDbc
#define UnlockEnv                  IIODulen_UnlockEnv
#define UtPrint                    IIODutpr_UtPrint
#define UtSnap                     IIODutsn_UtSnap

#define SQLColAttributesIsSupercededBySQLColAttributeButIsNeeded
                      /* needed by the Merant 3.11 DM else calls location 0x00*/
#endif  /* not NT_GENERIC */
/*
**  Global variables in ODLL or OD32:
*/
#ifdef _WIN32
GLOBALREF UWORD      fTrace;         /* Global trace options in Win32 only */
GLOBALREF OSVERSIONINFO os;          /* OS version information */
#endif

GLOBALREF HINSTANCE hModule;         /* module instance handle */

GLOBALREF char szIdmsHelp[MAX_PATH]; /* help file name */
GLOBALREF char INI_OPTIONS[];
GLOBALREF char INI_TRACE[];

#define DECLTEMP(t) char t;

#define LEN_TEMP 512            /* sizeof global temp work area */

/*
**  IDMS control block type definitions:
*/

#define SQLCA_FUDGE     32              /* SQLCA's are out of sync */

#define SQLCA           SQLCA_TYPE      /* These are easier to use... */

typedef SQLCA        *  LPSQLCA;

typedef HANDLE          HSQLCA;
typedef HANDLE          HSQLDA;
typedef HANDLE          HSQLVAR;

/*
**  Forward reference typedefs:
*/
typedef struct tDBC                * LPDBC;
typedef struct tSTMT               * LPSTMT;
typedef struct tCACHE              * LPCACHE;
typedef struct tDESC               * LPDESC;

/*
**  CACHE - SQLTables Cache Buffer Header
*/

typedef struct tCACHE
{
    LPCACHE pcacheNext;                 /* -->next cache buffer */
    UWORD   crow;                       /* Number of rows in buffer */
}
    CACHE;

/*
**  ENV Environment Control Block,
**      allocated by SQLAllocEnv.
*/
typedef struct tENV
{
    CHAR        szEyeCatcher[8];        /* "ENV*". */
    CHAR        szSqlState[8];          /* Environment SQLSTATE, padded. */
    i2          ODBCVersion;            /* ODBC 2.x or 3.x behavior
                    SQL_OV_ODBC2
                        The driver is to return and is to expect ODBC 2.x
                            codes for date, time, and timestamp data types.
                        The driver is to return ODBC 2.x SQLSTATE code when
                           SQLError, SQLGetDiagField, or SQLGetDiagRec is called.
                        The CatalogName argument in a call to SQLTables does not
                           accept a search pattern.
                     SQL_OV_ODBC3
                        The driver is to return and is to expect ODBC 3.x
                           codes for date, time, and timestamp data types.
                        The driver is to return ODBC 3.x SQLSTATE code when
                           SQLError, SQLGetDiagField, or SQLGetDiagRec is called.
                        The CatalogName argument in a call to SQLTables 
                           accepts a search pattern.  */
    i2          isMerant311;            /* is driver running under old Merant 3.11 DM */
    UWORD       cAlloc;                 /* Environment use count. */
    void     *  envHndl;                /* Ingres API environment  handle */
    UWORD       fStatus;                /* Environment status flags. */
    UWORD       fCommit;                /* Environment cursor commit option. */
    UWORD       crowFetchMax;           /* Default max number of rows per fetch. */
    LPDBC       pdbcFirst;              /* -->First connection. */
    CHAR        szMessage[MSGLEN];      /* Environment error message. */
    CHAR        rgbWork[300];           /* General work area. */
}
    ENV,     * LPENV;

typedef struct
{
    UWORD fTrace;
    char *odbc_trace_file;
}  IITRACE;
extern IITRACE IItrace;

/*
**  SESS Session Status Block
*/
typedef struct tSESS
{
    CHAR        szEyeCatcher[10];       /* "MAINSESS*" or "CATSESS*"   */
    UWORD       fStatus;                /* Connection status flags.    */
    void     *  connHandle;             /* Ingres API connection handle*/   
    void     *  tranHandle;             /* Ingres API transact. handle */
    VOID *      XAtranHandle;           /* XA unique transaction ID handle */
}
    SESS,     * LPSESS;
/*
**  SES status flags:
*/
#define SESS_CONNECTED       0x0001     /* Session connected. */
#define SESS_SUSPENDED       0x0002     /* Transaction needs to be committed. */
#define SESS_SESSION_SET     0x0004     /* Set session issued with trans options. */
#define SESS_AUTOCOMMIT_ON_ISSUED 0x0008 /*set autocommit on has been issued */
#define SESS_STARTED         0x0010     /* Transaction started. */
#define SESS_COMMIT_NEEDED   0x0020     /* Transaction needs to be committed. */

/*
**  DBC Data Base Connection Control Block,
**      allocated by SQLAllocConnect.
*/
typedef struct tDBC
{
    CHAR        szEyeCatcher[8];        /* "DBC*". */
    ODBC_CRITICAL_SECTION csDbc;        /* Single thread connection; */
#ifdef _WIN32
    HANDLE      hMSDTCTransactionCompleting;  /* event to block MTXDM connection 
                                           pooling thread from delisting from 
                                           MS DTC before DTC proxy thread has
                                           a chance to commit or rollback the 
                                           transaction */
#endif
    CHAR        szSqlState[8];          /* Connection SQLSTATE, padded. */
    void     *  envHndl;                /* Ingres API environment  handle */
    UWORD       fStatus;                /* Connection status flags. */
    UDWORD      fOptions;               /* Connection option bits, if set by */
    UDWORD      fOptionsSet;            /* SQLSetConnectOption, bit is set here too. */
    UWORD       fOptionsSet2;           /* Other options set by  SQLSetConnectOption. */
    UWORD       fCommit;                /* Connection cursor commit option. */
    UWORD       fConcurrency;           /* Cursor concurrency; default=SQL_CONCUR_READ_ONLY */
    UDWORD      fIsolation;             /* Connection transaction isolation mode. */
    UDWORD      fIsolationDBMS;         /* Default isolation mode for DBMS from RC file. */
    UDWORD      fIsolationDSN;          /* Default isolation mode for DSN from ini file. */
    SDWORD      fRelease;               /* Server release number. */
#define fReleaseRELEASE10 1000000             /* OI 1.00.0000 */
#define fReleaseRELEASE20 2000000             /* II 2.00.0000 */
#define fReleaseRELEASE25 2500000             /* II 2.50.0000 */
#define fReleaseRELEASE26 2600000             /* II 2.60.0000 */
#define fReleaseRELEASE30 3000000             /* r3 */
#define isOld6xRelease(pdbc)                  ((pdbc)->fRelease < fReleaseRELEASE10)
    UDWORD      fAPILevel;              /* Server API (GCA) level */
    UWORD       fDriver;                /* Driver type flag. */
    UWORD       fCatalog;               /* Catalog type flag. */
    UWORD       fInfo;                  /* GetInfo type flag. */
    UWORD       fInfoType;              /* GetTypeInfo type flag. */
    UWORD       fDlg;                   /* DriverConnect dialog box id */
    UWORD       fCursorType;            /* Cursor scroll type */
    HGLOBAL     hInfoInt;               /* GetInfo integer resource handle. */
    HGLOBAL     hInfoBit;               /* GetInfo bitmask resource handle. */
    HGLOBAL     hInfoStr;               /* GetInfo string resource handle. */
    HGLOBAL     hInfoType;              /* GetTypeInfo resource handle. */
    HINSTANCE   hTransDLL;              /* Translation DLL instance handle. */
    SQLHANDLE   hQuietMode;             /* Quiet mode window handle. */
    UDWORD      fTransOpt;              /* Translation DLL options. */
    SQLUINTEGER cConnectionTimeout;     /* Phony connect option support. */
    SQLUINTEGER cLoginTimeout;          /* Phony connect option support. */
    SQLUINTEGER cPacketSize;            /* Phony connect option support. */
    SQLINTEGER      cMaxLength;             /* SetStmtAttr(SQL_ATTR_MAX_LENGTH)
                                             as a possible 64-bit value */
    SQLUINTEGER     cQueryTimeout;          /* Query timeout. */
    UWORD       cRetrieveData;          /* Phoney statement option support. */
#define MAX_PSID    256 /* max. number of Prepared Stmt IDs in alloc bitmap */
    CHAR        cPreparedStmtMap[(MAX_PSID+7)/8];       /* Allocation bit map for prepared stmt numbers */
    SDWORD      iPreparedStmtID;        /* Max assigned PSID for conn if map full. */
    SDWORD      iCursor;                /* Max assigned cursor number for conn. */
    LPENV       penvOwner;              /* -->Owner environment block. */
    LPDBC       pdbcNext;               /* -->Next connection block. */
    LPSTMT      pstmtFirst;             /* -->First statement. */
    LPDESC      pdescFirst;             /* -->First descriptor. */
    LPSQB       psqb;                   /* -->IDMS SQL control blocks. */
    LPCACHE     pcache;                 /* -->list of cached fetch buffers. */
    LPSTR       pMessage;               /* -->next SQLCA message. */
    SDWORD      cbValueMax;             /* Max value length. */
    SQLUINTEGER     crowMax;                /* Max rows to return. */
    UWORD       crowFetchMax;           /* Default max number of rows per fetch. */
    UWORD       cbDSN;                  /* Connect data source name length. */
    UWORD       cbUID;                  /* Connect user id length. */
    UWORD       cbPWD;                  /* Connect password length. */
    UWORD       cbDBMS_PWD;             /* Connect DBMS password length. */
    UWORD       cbRoleName;             /* Connect role name length. */
    UWORD       cbRolePWD;              /* Connect role password length. */
    UWORD       cbGroup;
    SESS        sessMain;               /* Main SQL session block */
    SESS        sessCat;                /* Catalog SQL session block */
    SQB         sqb;                    /* IDMS control block anchor macro calls. */
    SQLCA       sqlca;                  /* SQL Communication area. */
    CHAR        reserved[SQLCA_FUDGE];  /* Allow for extra bytes from MF. */
    CHAR        szMessage[MSGLEN];      /* Connection error message. */
    CHAR        szCatTable[OBJECT_NAME_SIZE]; /* Catalog table name override. */
    CHAR        szDriver[LEN_DRIVER_NAME + 1]; /* Driver name. */
/*  CHAR        szTransDLL[129];           Translation DLL name. */
    CHAR        szCacheOwner[OBJECT_NAME_SIZE]; /* SQLTables cached szTableOwner parm. */
    CHAR        szCacheTable[OBJECT_NAME_SIZE]; /* SQLTables cached table parm. */
    UWORD       fCacheType;             /* SQLTables cached type parm. */
    CHAR        db_name_case;           /* iidbcapabilities' DB_NAME_CASE */
                                        /*    '/0'  unknown - need to build */
                                        /*    'L'   lower case */
                                        /*    'U'   upper case */
                                        /*    'M'   mixed case */
    CHAR        db_delimited_case;      /* iidbcapabilities' DB_DELIMITED_CASE */
                                        /*    '/0'  unknown - need to build */
                                        /*    'L'   lower case */
                                        /*    'U'   upper case */
                                        /*    'M'   mixed case */
    CHAR        szDSN[LEN_DSN + 1];     /* Connect data source name. */
    CHAR        szUID[OBJECT_NAME_SIZE]; /* Connect user id. */
    CHAR        szPWD[OBJECT_NAME_SIZE]; /* Connect password. */
    CHAR        szDBMS_PWD[OBJECT_NAME_SIZE]; /* Connect DBMS password. */
    CHAR        szVNODE[LEN_NODE + 1];  /* DriverConnect DRIVER vnode name. */
    CHAR        szDATABASE[LEN_DBNAME + 1];         /* Database name */
    CHAR        szSERVERTYPE[LEN_SERVERTYPE + 1] ;  /* Type of server */
    CHAR        szRoleName[LEN_ROLENAME+1]; /* Connect role name */
    CHAR        szRolePWD[LEN_ROLEPWD+1];   /* Connect role password. */
    CHAR        szGroup[33];            /* Group id (exec sql connect's -G option). */
    CHAR        szTIME[4];              /* DriverConnect DRIVER timeout value. */
    char        szUsername[33];         /* dbmsinfo('username') from server */
    char        szDBAname[33];          /* iidbconstants(dba_name) from server */
    char        szSystemOwnername[33];  /* iidbconstants(system_owner) from server */
    CHAR        multibyteFillChar;      /* fill char for Uni/MB code failure */
    UWORD       fIdentifierCase;        /* Case sensitivity of database object. */
                   /*    0                 unknown; get iidbcapabilities */
                   /*    SQL_IC_UPPER      1-idents stored UPPER case */
                   /*    SQL_IC_LOWER      2-idents stored LOWER case */
                   /*    SQL_IC_SENSITIVE  3-idents stored sensitive MIXED case */
                   /*    SQL_IC_MIXED      4-idents stored insensitive MIXED case */
    UWORD       fActiveStatementsMax;   /* Case sensitivity of database object. */
    UWORD       fMetadataID;            /* How to treat catalog string argument*/
                   /*    SQL_FALSE            not treated as identifier */
                   /*    SQL_TRUE             treated as identifier */
    UWORD       fAsyncEnable;           /* Enable asynchronous execution */
                   /*    SQL_ASYNC_ENABLE_OFF   synchronous execution */
                   /*    SQL_ASYNC_ENABLE_ON    asynchronous execution */
    SWORD       db_table_name_length;   /* length of iitables(table_name) field or -1 if varchar */
    UWORD       max_table_name_length;  /* SQL_MAX_TABLE_NAME_LEN */
    UWORD       max_owner_name_length; /* SQL_MAX_OWNER_NAME_LEN */
    UWORD       max_schema_name_length; /* SQL_MAX_SCHEMA_NAME_LEN */
    UWORD       max_procedure_name_length; /* SQL_MAX_PROCEDURE_LEN */
    UWORD       max_column_name_length; /* SQL_MAX_COLUMN_NAME_LEN */
    UWORD       max_columns_in_table;   /* SQL_MAX_COLUMNS_IN_TABLE */
    UWORD       max_row_length;         /* SQL_MAX_ROW_SIZE(LEN) */
    UWORD       max_char_column_length; /* SQL_MAX_CHAR_COLUMN_LEN */
    UWORD       max_nchr_column_length; /* SQL_MAX_NCHR_COLUMN_LEN */
    UWORD       max_vchr_column_length; /* SQL_MAX_VCHR_COLUMN_LEN */
    UWORD       max_nvchr_column_length; /* SQL_MAX_NVCHR_COLUMN_LEN */
    UWORD       max_byte_column_length; /* SQL_MAX_BYTE_COLUMN_LEN */
    UWORD       max_vbyt_column_length; /* SQL_MAX_VBYT_COLUMN_LEN */
    UDWORD      max_decprec;            /* Max decimal precision */

    BOOL        is_unicode_enabled;     /* If target is unicode_enabled */      

    UWORD       fServerClass;           /* ServerClass bits */
#define         fServerClassINGRES      0x0001
#define         fServerClassOPINGDT     0x0002
#define         fServerClassDCOM        0x0004
#define         fServerClassIDMS        0x0008
#define         fServerClassDB2         0x0010
#define         fServerClassIMS         0x0020
#define         fServerClassVSAM        0x0040
#define         fServerClassRDB         0x0080
#define         fServerClassSTAR        0x0100
#define         fServerClassORACLE      0x0200
#define         fServerClassINFORMIX    0x0400
#define         fServerClassSYBASE      0x0800
#define         fServerClassALLBASE     0x1000
#define         fServerClassRMS         0x2000
#define         fServerClassDB2UDB      0x4000
#define         fServerClassVECTORWISE  0x8000


#define         isServerClassINGRES(dbc)   ((dbc)->fServerClass \
                                           & (fServerClassINGRES+ \
	                                      fServerClassVECTORWISE))
#define         isServerClassOPINGDT(dbc)  ((dbc)->fServerClass & fServerClassOPINGDT) 
#define         isServerClassDCOM(dbc)     ((dbc)->fServerClass & fServerClassDCOM) 
#define         isServerClassIDMS(dbc)     ((dbc)->fServerClass & fServerClassIDMS) 
#define         isServerClassDB2(dbc)      ((dbc)->fServerClass & fServerClassDB2) 
#define         isServerClassIMS(dbc)      ((dbc)->fServerClass & fServerClassIMS) 
#define         isServerClassVSAM(dbc)     ((dbc)->fServerClass & fServerClassVSAM) 
#define         isServerClassRDB(dbc)      ((dbc)->fServerClass & fServerClassRDB) 
#define         isServerClassSTAR(dbc)     ((dbc)->fServerClass & fServerClassSTAR) 
#define         isServerClassORACLE(dbc)   ((dbc)->fServerClass & fServerClassORACLE) 
#define         isServerClassINFORMIX(dbc) ((dbc)->fServerClass & fServerClassINFORMIX) 
#define         isServerClassSYBASE(dbc)   ((dbc)->fServerClass & fServerClassSYBASE) 
#define         isServerClassALLBASE(dbc)  ((dbc)->fServerClass & fServerClassALLBASE) 
#define         isServerClassRMS(dbc)      ((dbc)->fServerClass & fServerClassRMS)
#define         isServerClassDB2UDB(dbc)   ((dbc)->fServerClass & fServerClassDB2UDB)
#define         isServerClassVECTORWISE(dbc)   ((dbc)->fServerClass & fServerClassVECTORWISE)
#define         isServerClassAnIngresEngine(dbc) ((dbc)->fServerClass \
                                                      & (fServerClassINGRES+ \
                                                         fServerClassIMS+ \
                                                         fServerClassVSAM+ \
                                                         fServerClassVECTORWISE+ \
	                                                 fServerClassRMS))
#define         isServerClassEA(dbc)       ((dbc)->fServerClass \
                                                      & (fServerClassORACLE+ \
                                                         fServerClassINFORMIX+ \
                                                         fServerClassSYBASE+ \
                                                         fServerClassRMS+ \
                                                         fServerClassDB2UDB))

    DWORD       dwDTCRMCookie;          /* MTS/DTC resource mgr cookie */
    CHAR        szDecimal[2];
    BOOL        bcConnectCalled;
    CHAR        *bcOutStr;
}
 DBC;

/* Not used yet for translation DLLs.
   Should be a typedef that used in DBC.

    BOOL (SQL_API * pfnSQLDriverToDataSource) (
                UDWORD       fOption,
                SWORD        fCType,
                PTR          rgbValueIn,
                SDWORD       cbValueIn,
                PTR          rgbValueOut,
                SDWORD       cbValueOutMax,
                SDWORD     * pcbValueOut,
                UCHAR      * szErrorMsg,
                SWORD        cbErrorMsgMax,
                SWORD      * pcbErrorMsg);
    BOOL (SQL_API * pfnSQLDataSourceToDriver) (
                UDWORD       fOption,
                SWORD        fCType,
                PTR          rgbValueIn,
                SDWORD       cbValueIn,
                PTR          rgbValueOut,
                SDWORD       cbValueOutMax,
                SDWORD     * pcbValueOut,
                UCHAR      * szErrorMsg,
                SWORD        cbErrorMsgMax,
                SWORD      * pcbErrorMsg);
*/

/*
**  DBC and STMT Option flags set in fOptions and fOptionsSet:
*/
#define OPT_FETCHDOUBLE     0x0001      /* Fetch real as double */
#define OPT_ACCESSIBLE      0x0002      /* Use accessible tables for catalog */
#define OPT_CACHE           0x0004      /* Cache SQLTables result set */
#define OPT_DEFAULTTOCHAR   0x0008      /* Treat SQL_C_DEFAULT as char */
#define OPT_AUTOCOMMIT      0x0010      /* Automatic commit enabled */
#define OPT_READONLY        0x0020      /* Set session read only */
#define OPT_TRANSLATE       0x0040      /* Translation DLL enabled */
#define OPT_INGRESDATE      0x0080      /* Support Ingresdate syntax */
#define OPT_ENABLE_ENSURE   0x0100      /* Enable ensure in SQLStatistics */
#define OPT_NOSCAN          0x0200      /* Don't scan for escape sequences */
#define OPT_CURSOR_LOOP     0x0400      /* Use cursor loops always, else try select loops */
#define OPT_CONVERT_3_PART_NAMES 0x0800 /* need to convert owner.table.colname  */
#define OPT_USESYSTABLES         0x1000 /* Use Sys Tables */
#define OPT_BLANKDATEisNULL      0x2000 /* Return blank Ingres date as NULL */
#define OPT_DATE1582isNULL       0x4000 /* Return Ingres date 1582-01-01 as NULL */
#define OPT_STRINGTRUNCATION     0x8000 /* Fail string truncations. */
#define OPT_CATCONNECT       0x00010000 /* Create separate Ingres session 
                                              for catalog functions (SQLTables, ...) */
#define OPT_NUMOVERFLOW      0x00020000 /* -numeric_overflow=ignore */
#define OPT_CATSCHEMA_IS_NULL 0x00040000 /* Catalog functions: ignore owner,
                                              return NULL always for TABLE_SCHEM */
#define OPT_DECIMALPT        0x00080000 /* II_DECIMAL value; ',' if set */
#define OPT_UNICODE          0x00100000 /* Unicode (wide char) is supported by server */
#define OPT_LONGDISPLAY      0x00200000 /* For EDS Mapper special, return longer
                                              SQL_COLUMN_DISPLAY_SIZE and default
                                              AUTOCOMMIT to OFF. Disables
                                              SQLTransact because DM won't call it. */
#define OPT_DISABLEUNDERSCORE 0x00400000 /* Disable catalog functions'
                                            underscore search character */
#define OPT_ALLOW_DBPROC_UPDATE 0x00800000
#define OPT_CONVERTINT8TOINT4   0x01000000 /*Convert i8 from DBMS to i4 for 
                                             application don't support i8*/
#define OPT_ALLOWUPDATE         0x02000000 /* Allow update using pass-through query */

/*
**  Additional flags for DBC options.
**
**  These flags are used when an option is set by SQLSetConnectOption,
**  the option is not a bit flag, and 0 is valid value.
*/
#define OPT_CAT_TABLE       0x0001      /* Accessible tables name from ConnectOption. */
#define OPT_COMMIT_BEHAVIOR 0x0002      /* Commit behavior set by SQLSetConnectOption. */

/*
**  DBC status flags:
*/
#define DBC_DRIVER          0x0001      /* SQLDriverConnect with DRIVER keyword. */
#define DBC_READ_ONLY       0x0002      /* Driver is read only.*/
#define DBC_DEAD            0x0004      /* For connection pooling, connection died */
#define DBC_CACHE           0x2000      /* SQLTables result set in cache. */
#define isDriverReadOnly(pdbc) (pdbc->fStatus & DBC_READ_ONLY)

/*
**  Driver types:
*/
#define DBC_IDMS        IDU_IDMS        /* CA-IDMS (Mainframe). */
#define DBC_IDMSUNIX    IDU_IDMSUNIX    /* CA-IDMS/UNIX. */
#define DBC_CADB        IDU_CADB        /* CA-DB (VAX). */
#define DBC_CADBUNIX    IDU_CADBUNIX    /* CA-DB/UNIX. */
#define DBC_DATACOM     IDU_DATACOM     /* CA-DATACOM/UNIX. */
#define DBC_INGRES      IDU_INGRES      /* CA-Ingres */

/*
**  LONGBUFFER structure. Used to fetch long data objects.
*/
typedef struct tagLongBuffer
{
    UDWORD      cbData;
    CHAR      * pLongData;
} LONGBUFFER;
typedef LONGBUFFER     * LPLONGBUFFER;

/*
**  Query sequence control block
*/
typedef struct 
{
   LPSTMT pstmt;
   IIAPI_QUERYTYPE apiQueryType;
   i2 sequence;
   i2 sequence_type;
   i2 options; 
   char *qry_buff;
   IIAPI_PUTPARMPARM serviceParms;
} QRY_CB;

/*
** Table privileges linked list member.
*/
typedef struct
{
    QUEUE tabpriv_q;
    char *catalog;
    char *ownerName;
    char *tableName;
    char *grantor;
    char *grantee;
    char *privilege;
    char *is_grantable;
}  TABLEPRIV_ROW;

/*
** Column privileges linked list member.
*/
typedef struct
{
    QUEUE colpriv_q;
    char *catalog;
    char *ownerName;
    char *tableName;
    char *columnName;
    char *grantor;
    char *grantee;
    char *privilege;
    char *is_grantable;
}  COLPRIV_ROW;

/*
**  STMT Statement Control Block,
**       allocated by SQLAllocStmt.
*/
typedef struct tSTMT
{
    CHAR        szEyeCatcher[8];        /* "STMT*". */
    CHAR        szSqlState[8];          /* Statement SQLSTATE, padded. */
    UDWORD      fStatus;                /* Statement status flags. */
    UDWORD      fOptions;               /* Statement option flags. */
    UDWORD      fCommand;               /* IDMS command type flag. */
    UWORD       fCatalog;               /* Catalog function type flag. */
    UWORD       fConcurrency;           /* Cursor concurrency; default=SQL_CONCUR_READ_ONLY */
    UWORD       fCursorSensitivity;     /* Cursor sensitivity; default=SQL_UNSPECIFIED */
    UWORD       fCursorType;            /* Cursor type; default=SQL_CURSOR_FORWARD_ONLY */
    UWORD       fCursorDirection;       /* Cursor type; default=SQL_FETCH_NEXT */
    SDWORD      fCursorOffset;          /* Row offset for relative scrolls */
    SDWORD      fCursorPosition;        /* Row position for SQLSetPos() */
    SDWORD      fCurrentCursPosition;   /* Current relative curs position */
    UWORD       fCursorScrollable;      /* Scrollability of the cursor */
    SDWORD      fCursLastResSetSize;    /* Result set size at EOD */
    LPDBC       pdbcOwner;              /* -->Owner connection block. */
    UDWORD      fAPILevel;              /* Server API (GCA) level */
    LPSTMT      pstmtNext;              /* -->Next statment block. */
    LPSESS      psess;                  /* SESS (IDMS SESsion Status block). */
    SDWORD      cbValueMax;             /* Max value length
                                             from SetStmtAttr(SQL_ATTR_MAX_LENGTH). */
    SQLINTEGER      cMaxLength;             /* SetStmtAttr(SQL_ATTR_MAX_LENGTH)
                                             as a possible 64-bit value */
    SQLUINTEGER     crowMax;                /* Max rows to return
                                             from SetStmtOption(SQL_MAX_ROWS). */
    SQLUINTEGER cQueryTimeout;          /* Query timeout */
    UWORD       cRetrieveData;          /* Phoney statement option support. */
    UWORD       cUseBookmarks;          /* Phoney statement option support. */
    UWORD       fMetadataID;            /* How to treat catalog string argument*/
                   /*    SQL_FALSE            not treated as identifier */
                   /*    SQL_TRUE             treated as identifier */
    UWORD       fAsyncEnable;           /* Enable asynchronous execution */
                   /*    SQL_ASYNC_ENABLE_OFF   synchronous execution (default)*/
                   /*    SQL_ASYNC_ENABLE_ON    asynchronous execution */
    CHAR       *pFetchBookmark;         /* ->binary bookmark to be used by
                                             SQLFetchScroll(SQL_FETCH_BOOKMARK) */
    SDWORD      iPreparedStmtID;        /* Prepared statement number for PREPARE s00001 */
    SDWORD      iCursor;                /* cursor number. */
    LPSTR       szSqlStr;               /* -->SQL saved syntax string. */
    SDWORD      cbSqlStr;               /* length of saved syntax string. */
    LPSTR       szPSqlStr;              /* -->prepared syntax string. */
    SDWORD      cbPSqlStr;              /* length of prepared syntax string. */
    LPCACHE     pcache;                 /* -->current cached fetch buffer. */
    LPDESC      pAPD;                   /* -->Application    Parameter Descriptor*/
    LPDESC      pIPD;                   /* -->Implementation Parameter Descriptor*/
    LPDESC      pARD;                   /* -->Application    Row Descriptor */
    LPDESC      pIRD;                   /* --.Implementation Row Descriptor */
    LPDESC      pAPD_automatic_allocated;/* -->Application    Parameter Descriptor*/
    LPDESC      pIPD_automatic_allocated;/* -->Implementation Parameter Descriptor*/
    LPDESC      pARD_automatic_allocated;/* -->Application    Row Descriptor */
    LPDESC      pIRD_automatic_allocated;/* -->Implementation Row Descriptor */
    CHAR      * pFetchBuffer;           /* -->fetch  buffer. */
    CHAR      * prowCurrent;            /* -->current row in FetBuf. */
    UWORD       icolParmLast;           /* Last parm column. #parameter markers */
    UWORD       icolSent;               /* Last parm column sent */
    UWORD       icolPrev;               /* Previous SQLGetData column. */
    UWORD       icolFetch;              /* Number of columns fetched  */
    UWORD       icolPut;                /* Current  SQLPutData parm. */
    UDWORD      cbrow;                  /* Length of row. */
    UWORD       crowFetch;              /* Number of rows per fetch. */
    UWORD       crowFetchMax;           /* Max number of rows per fetch. */
    UWORD       crowBuffer;             /* Number of rows in fetch buffer. */
    UDWORD      crow;                   /* Number of rows returned so far. */
    UDWORD     *pirowParm;              /* -->where to return current parm set. */
    UDWORD      crowParm;               /* Number of rows of parameter values. */
    UDWORD      irowParm;               /* Current row of parameter values. start at 0*/
    SQLUINTEGER     irowCurrent;            /* Current row of rowset for errors. start at 1*/
    UDWORD      icolCurrent;            /* Current column of rowset in error. start at 1*/
    UDWORD      fCvtError;              /* Column conversion error flags. */
    LPSTR       pMessage;               /* -->next SQLCA message. */
    CHAR        szMessage[MSGLEN];      /* Statement error message. */
    CHAR        szError[20];            /* Last syntax error token. */
    CHAR        szCursor[65];           /* Name for GET/SET cursor. */
    SQLCA       sqlca;                  /* SQL Communication area. */
    LPSTR       szNull;                 /* -->null byte for ParseEscape */
    CHAR        bNull[2];               /* previous value for Parse Escape */
    CHAR        szProcName  [OBJECT_NAME_SIZE];  /* call proc's name   */
    CHAR        szProcSchema[OBJECT_NAME_SIZE];  /* call proc's schema */
    CHAR        reserved[SQLCA_FUDGE];  /* Allow for extra bytes from MF. */
    VOID *      stmtHandle;             /* Ingres API stmt handle */
    VOID *      QueryHandle;            /* query handle of repeated query */
    QRY_CB      qry_cb;                 /* Query state machine control block */
    BOOL        moreData;
    UWORD       tabpriv_count;          /* Count of table privileges */
    QUEUE       baseTabpriv_q;          /* Linked list for table privs */
    UWORD       colpriv_count;          /* Count of column privileges */
    QUEUE       baseColpriv_q;          /* Linked list for column privs */
    BOOL        dateConnLevel;          /* Ingredate coercion */
    UWORD       reExecuteCount;
} STMT;

/*
**  Statement status flags:
*/
#define STMT_INTERNAL   0x0001          /*  Internal ODBC call. */
#define STMT_DIRECT     0x0002          /*  SQLExecDirect ODBC call. */
#define STMT_NEED_DATA  0x0004          /*  SQL_DATA_AT_EXEC in process. */
#define STMT_WHERE_CURRENT_OF_CURSOR \
                        0x0008          /*  stmt is cursor update/delete */
#define STMT_PREPARED   0x0010          /*  SQL statement has been prepared. */
#define STMT_DESCRIBED  0x0020          /*  SQL statement has been described. */
#define STMT_EXECUTE    0x0040          /*  SQL statement is being executed. */
#define STMT_API_CURSOR_OPENED \
                        0x0080          /*  Real Ingres cursor is opened. */
                                        /*     if on,  using cursor loop */
                                        /*     if off, using select loop */
#define STMT_OPEN       0x0100          /*  ODBC cursor is open, API cursor may be. */
#define STMT_EOF        0x0200          /*  IDMS cursor at end, may open or closed. */
#define STMT_CLOSED     0x0400          /*  Piggybacked close done, may be "open". */
#define STMT_CONST      0x0800          /*  Result set buffers are constants. */
#define STMT_CATALOG    0x1000          /*  Statement using catalog connection. */
#define STMT_CACHE_PUT  0x2000          /*  Put fetch buffer in cache. */
#define STMT_CACHE_GET  0x4000          /*  Get fetch buffer from cache. */
#define STMT_CACHE      (STMT_CACHE_PUT | STMT_CACHE_GET) /* cache in use. */
#define STMT_RESET      (STMT_NEED_DATA | STMT_HASBLOB)
                                        /*  Flags to keep in CloseCursors  */
                                        /*    Keep descriptor info across commit */
#define STMT_CANBEPREPARED  0x0008000  /*  Statement can be PREPAREd/EXECUTEd */
#define STMT_SEGINIT       0x00010000  /*  Initialize send segment */
#define STMT_HASBLOB       0x00020000  /* Fetch query has blob */
#define STMT_PARAMS_DESCRIBED 0x00040000  /* Parameters are described */
#define STMT_RETPROC       0x00080000  /* DB proc with return value */
#define STMT_SQLEXECUTE    0x00100000    /* Executed directly from SQLExecute */

/*  
** Statement command type 
*/
#define CMD_SELECT                  0x00000001 /* SELECT */
#define CMD_SESSION                 0x00000002 /* SET SESSION (non-READ WRITE) */
#define CMD_SUSPEND                 0x00000004 /* SUSPEND SESSION */
#define CMD_DDL                     0x00000008 /* DDL    */
#define CMD_INSERT                  0x00000010 /* INSERT */
#define CMD_BULK                    0x00000020 /* BULK INSERT */
#define CMD_UPDATE                  0x00000040 /* UPDATE */
#define CMD_DELETE                  0x00000080 /* DELETE */
#define CMD_TRANSACT_non_READ_WRITE 0x00000100 /* SET TRANSACTION (non-READ WRITE) */
#define CMD_EXT                     0x00000200 /* CA extensions */
#define CMD_EXECPROC                0x00000400 /* CALL Procedure */
#define CMD_AUTOCOMMIT_OFF          0x00000800 /* SET AUTOCOMMIT OFF */
#define CMD_AUTOCOMMIT_ON           0x00001000 /* SET AUTOCOMMIT ON */
#define CMD_COMMIT                  0x00002000 /* COMMIT */
#define CMD_ROLLBACK                0x00004000 /* ROLLBACK */
#define CMD_SESSION_READ_WRITE      0x00008000 /* SET SESSION READ WRITE */
#define CMD_TRANSACT_READ_WRITE     0x00010000 /* SET TRANSACTION READ WRITE */

#define CMD_DML        (CMD_INSERT | CMD_BULK | CMD_UPDATE | CMD_DELETE \
                                     | CMD_EXECPROC)
#define CMD_COMMITABLE (CMD_DDL | CMD_DML)
#define CMD_START      (CMD_COMMITABLE | CMD_SELECT | CMD_TRANSACT_non_READ_WRITE | \
                         CMD_TRANSACT_READ_WRITE | CMD_EXT)
#define CMD_READONLY_ALLOWABLE (CMD_SELECT | \
                                 CMD_SESSION | \
                                 CMD_TRANSACT_non_READ_WRITE | \
                                 CMD_AUTOCOMMIT_OFF | CMD_AUTOCOMMIT_ON | \
                                 CMD_COMMIT   | \
                                 CMD_ROLLBACK | \
                                 CMD_EXECPROC)

/* For a Read Only driver, allow SELECT, COMMIT, ROLLBACK, and some SET statments.
   Also allow calling of procedures if server is Ingres since this
   server supports the SET SESSION READ ONLY statement and we can trust the
   server to catch procedures calls that try to update.
*/
#define StatementAllowedForReadOnly(pstmt) \
        (pstmt->fCommand & CMD_READONLY_ALLOWABLE)

                                        /*  ODBC catalog command type: */
#define CATLG_COLUMNPRIV 1               /*  SQLColumnPrivileges. */
#define CATLG_COLUMNS    2               /*  SQLColumns. */
#define CATLG_FKEY       3               /*  SQLForeignKeys. */
#define CATLG_PKEY       4               /*  SQLPrimaryKeys. */
#define CATLG_PROCCOL    5               /*  SQLProcedureColumns. */
#define CATLG_PROC       6               /*  SQLProcedures. */
#define CATLG_SPECCOL    7               /*  SQLSpecialColumns. */
#define CATLG_TABLEPRIV  8               /*  SQLTablePrivileges. */
#define CATLG_STATS      9               /*  SQLStatistics. */
#define CATLG_TABLES     10              /*  SQLTables. */
#define CATLG_OWNERS     11              /*  SQLTables, owner list. */
#define CATLG_TYPES      12              /*  SQLTables, type list. */
#define CATLG_TYPEINFO   13              /*  SQLGetTypeInfo. */
#define CATLG_SETS       14              /*  CASQLSets. */
#define CATLG_CATALOGS   15              /*  SQLTables, catalog (qualifier) list. */

/*
**  DESC Descriptor Control Block,
**       allocated by AllocDescriptor.
*/
typedef struct tDESCFLD            * LPDESCFLD;

typedef struct tDESC
{                                   /* Note: an application can associate an 
                                       explicitly allocated descriptor with more
                                       one statment */
    char          szEyeCatcher[10]; /* "DESC*"      */
    SQLSMALLINT   AllocType;        /* Allocated automatically or by the user
                                         SQL_DESC_ALLOC_AUTO
                                         SQL_DESC_ALLOC_USER
                                       Note: an application can associate an 
                                       explicitly allocated descriptor with more 
                                       one statment 
                                       ARD-R,  APD-R,  IRD-R, IPD-R  */
    SQLUINTEGER       ArraySize;        /* Num of rows in rowset or of param values.
                                       Initially 1.  Enables multirow fetch.
                                       If > 1, DataPtr, IndicatorPtr, 
                                       and OctetLengthPtr point to arrays.
                                       ARD set by SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE).
                                       APD set by SQLSetStmtAttr(SQL_ATTR_PARAMSET_SIZE).
                                       ARD-RW, APD-RW                */
    SQLUSMALLINT* ArrayStatusPtr;   /* row status array (IRD),
                                       parameter status array (IPD),
                                       row operation array (ARD),
                                       parameter operation array (APD).
                                       Array initially populated by 
                                          SQLFetch, SQLFetchScroll.
                                       IRD set by SQLSetStmtAttr(SQL_ATTR_ROW_STATUS_PTR).
                                       IPD set by SQLSetStmtAttr(SQL_ATTR_PARAM_STATUS_PTR).
                                       ARD set by SQLSetStmtAttr(SQL_ATTR_ROW_OPERATION_PTR).
                                       APD set by SQLSetStmtAttr(SQL_ATTR_PARAM_OPERATION_PTR).
                                       ARD-RW, APD-RW, IRD-RW,IPD-RW */
    SQLUINTEGER     * BindOffsetPtr;    /* Pointer to binding offset
                                       ARD set by SQLSetStmtAttr(SQL_ATTR_ROW_BIND_OFFSET_PTR).
                                       APD set by SQLSetStmtAttr(SQL_ATTR_PARAM_BIND_OFFSET_PTR).
                                       ARD-RW, APD-RW                */
    SQLINTEGER    BindType;         /* Binding orientation for columns or parameters:
                                          SQL_BIND_BY_COLUMN for column-wise binding
                                          length of element for row-wise binding
                                       ARD set by SQLSetStmtAttr(SQL_ATTR_ROW_BIND_TYPE).
                                       APD set by SQLSetStmtAttr(SQL_ATTR_PARAM_BIND_TYPE).
                                       ARD-RW, APD-RW                */
    SQLSMALLINT   Count;            /* count of DESCFLDs beyond DESCFLD[0] that contain data
                                       ARD set = 0 by SQLFreeStmt(SQL_UNBIND)
                                       APD set = 0 by SQLFreeStmt(SQL_RESET_PARAMS)
                                       IRD set = 0 by SQLFreeStmt(SQL_UNBIND)
                                       IPD set = 0 by SQLFreeStmt(SQL_RESET_PARAMS)
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    SQLUINTEGER     * RowsProcessedPtr; /* Ptr to number of rows fetched (IRD) or
                                       number of sets of parameters processed (IPD).
                                       IRD set by SQLSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR).
                                       IPD set by SQLSetStmtAttr(SQL_ATTR_PARAMS_PROCESSED_PTR).
                                                       IRD-RW,IPD-RW */

    i2            CountAllocated;   /* count of user DESCFLDs allocated
                                       (not counting bookmark and safety) */
    i2            Type_APD_ARD_IPD_IRD;
#define Type_Unknown 0
#define Type_APD     1
#define Type_IPD     2
#define Type_IRD     3
#define Type_ARD     4
#define isaUnknown(p) ((p)->Type_APD_ARD_IPD_IRD==Type_Unknown)
#define isaAPD(p)     ((p)->Type_APD_ARD_IPD_IRD==Type_APD)
#define isaIPD(p)     ((p)->Type_APD_ARD_IPD_IRD==Type_IPD)
#define isaIRD(p)     ((p)->Type_APD_ARD_IPD_IRD==Type_IRD)
#define isaARD(p)     ((p)->Type_APD_ARD_IPD_IRD==Type_ARD)
    LPDBC         pdbc;             /* -->Owner connection block. */
    LPDESC        pdbcNext;         /* -->Next descriptor in DBC->DESC chain */
    LPSTMT        pstmt;            /* if IRD, -> stmt             */
    LPDESCFLD     pcol;             /* -> DESCFLDs descriptor fields */
#define INITIAL_DESCFLD_ALLOCATION   10  /* initial guess for num of flds to alloc*/
#define SECONDARY_DESCFLD_ALLOCATION 50 /* additional num of flds to alloc on Realloc*/
#ifdef _DEBUG
    LPDESCFLD     col1;             /* convenient debug pointers to elements */
    LPDESCFLD     col2;
    LPDESCFLD     col3;
    LPDESCFLD     col4;
#endif
    SQLCA       sqlca;              /* SQL Communication area to anchor error list */
    CHAR        szSqlState[8];      /* Descriptor SQLSTATE, workarea. */
    CHAR        szMessage[MSGLEN];  /* Descriptor error message workarea */
} DESC;

typedef struct tDESCFLD
    /* array of descriptor fields.  DESCFLD[0] is reserved for
       bookmark support in the future.  DESCFLD[1] is the
       first user descriptor field.  All fields that are
       marked as RW (Read/Write) can be set also by
       SQLSetDescField, SQLSetDescRec unless otherwise stated.

       Legend:
          ARD   Application Row Descriptor
          APD   Application Parameter Descriptor
          IRD   Implementation Row Descriptor
          IPD   Implementation Parameter Descriptor
          R     Read only
          RW    Read/Write

       Fields that are commented out are unneeded by the driver
       because constant values are involved or can be derived
       from other values in the block.
    */
{
    char          Name[OBJECT_NAME_SIZE];  /* Column alias (IRD)
                                              Parameter name if named procedure
                                              parameters are supported (IPD).
                                              IRD-R, IPD-RW */
    SQLSMALLINT   VerboseType;      /* SQL or C data type or verbose datetime type.
                                       Initally SQL_DEFAULT.
                                       APD set by SQLBindParameter(ValueType)
                                       IPD set by SQLBindParameter(ParameterType)
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    char *        DataPtr;          /* ptr to bound parameter (APD)
                                           or bound column (ARD).
                                       ARD set by SQLBindCol.
                                       APD set by SQLBindParameter(ParameterValuePtr).
                                       Setting this field triggers a consistency
                                       check.  IPD field can be set to force 
                                       a consistency check.
                                       ARD-RW, APD-RW  IRD-check only*/
/*  SQLINTEGER    AutoUniqueValue;   * TRUE if auto-incrementing column
                                                       IRD-R (R=read-only)*/
    CHAR *        BaseColumnName;   /* Result set's base column name or ""
                                                       IRD-R */
    CHAR *        BaseTableName;    /* Result set's base table name or ""
                                                       IRD-R */
/*  SQLINTEGER    CaseSensitive;     * TRUE if col or param is case-sensitive
                                       for collations and comparisons; otherwise
                                       FALSE if not case-sensitive or
                                       is non-character column.
                                                       IRD-R, IPD-R  */
/*  SQLCHAR *     CatalogName;       * Result set's table's catalog name or ""
                                                       IRD-R         */
    SQLSMALLINT   ConciseType;      /* SQL or C data type or concise datetime type
                                       Initally SQL_DEFAULT.
                                       APD set by SQLBindParameter(ValueType)
                                       IPD set by SQLBindParameter(ParameterType)
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    SQLSMALLINT   DatetimeIntervalCode;
                                    /* If ConcideType==SQL_TYPE_TIMESTAMP
                                         then   SQL_CODE_TIMESTAMP; else 0.
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    SQLINTEGER        DatetimeIntervalPrecision;
                                    /* For Ingres without intervals, always 0
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    SQLINTEGER    DisplaySize;      /* Max number of characters to display
                                                       IRD-R */
    SQLSMALLINT   FixedPrecScale;   /* TRUE if exact numeric with a
                                       fixed precision and non-zero scale
                                       (like MONEY).
                                                       IRD-R, IPD-R  */
    SQLINTEGER      * IndicatorPtr;     /* Ptr to NULL indicator
                                       ARD set by SQLBindCol.
                                       APD set by SQLBindParameter.
                                       ARD-RW, APD-RW */
    CHAR *        Label;            /* Column name or ""
                                                       IRD-R         */
    SQLUINTEGER       Length;           /* Max length of char string in chars,
                                       max length of binary data type in bytes,
                                       max length of fixed-length data type, or
                                       actual length of variable-length data type.
                                       ARD set by SQLBindCol.
                                       IPD set by SQLBindParameter(ColumnSize)
                                           if char or binary string.
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
/*  SQLCHAR *     LiteralPrefix;     * Literal prefix
                                                       IRD-R         */
/*  SQLCHAR *     LiteralSuffix;     * Literal suffix
                                                       IRD-R         */
/*  SQLCHAR *     LocalTypeName;     * Data type name as a displayable string
                                                       IRD-R, IPD-R  */
    SQLSMALLINT   Nullable;         /* SQL_NULLABLE if columns can have NULLs.
                                       SQL_NO_NULLS if columns has no NULLs.
                                       SQL_NULLABLE_UNKNOWN is not known.
                                       For IPD, always SQL_NULLABLE returned.
                                                       IRD-R, IPD-R  */
/*  SQLINTEGER    NumPrecRadix;      * 2 if approximate (floating) numeric,
                                       10 if exact numeric, else 0.
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    SQLINTEGER        OctetLength;      /* Length in bytes of char or binary data.
                                       Max length in bytes of varchar or varbin,
                                       or length of single char or binary element.
                                       APD set by SQLBindParameter(BufferLength)
                                           if C char or C binary string.
                                       ARD-RW, APD-RW, IRD-R, IPD-R(W?) */
    SQLINTEGER    * OctetLengthPtr;     /* Ptr to a total length in bytes.
                                       If APD, 
                                          ignored if not char and bin.
                                          if NULL or *OctetLengthPtr == SQL_NTS
                                              assume char and bin data null term.
                                          if SQL_DATA_AT_EXEC or 
                                             SQL_LEN_DATA_AT_EXEC(length)
                                               signals that data comes later.
                                       ARD set by SQLBindCol.
                                       APD set by SQLBindParameter(StrLen_or_IndPtr).
                                       ARD-RW, APD-RW                */
    SQLSMALLINT   ParameterType;    /* Parameter type
                                          SQL_PARAM_INPUT (default if IPD)
                                          SQL_PARAM_OUTPUT
                                          SQL_PARAM_INPUT_OUTPUT
                                       IPD set by SQLBindParameter(InputOutputType).
                                                              IPD-RW */
  /*SQLSMALLINT*/
    SQLINTEGER        Precision;        /* Number of digits for exact numeric.
                                       Number of bits in mantissa (binary prec).
                                       Number of digits in fractional seconds
                                          for TIMESTAMP.
                                       ARD set by SQLBindCol(TargetType).
                                       APD set by SQLBindParameter(default)
                                           if C datetime or C numeric.
                                       IPD set by SQLBindParameter(ColumnSize)
                                           if decimal, numeric, float, double.
                                       IPD set by SQLBindParameter(DecimalDigits)
                                           if datetime.
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
/*  SQLSMALLINT   Rowver;            * TRUE if column automatically modified
                                       For Ingres, always SQL_FALSE.
                                                       IRD-R, IPD-R  */
    SQLSMALLINT   Scale;            /* Scale for decimal and numeric data types.
                                       ARD set by SQLBindCol(TargetType).
                                       IRD set by SQLBindCol(DecimalDigits).
                                       IPD set by SQLBindParameter(DecimalDigits)
                                           if decimal, numeric.
                                       ARD-RW, APD-RW, IRD-R, IPD-RW */
    CHAR *        SchemaName;       /* Result set's base table's schema name or ""
                                                       IRD-R         */
/*  SQLSMALLINT   Searchable;        * Can column be in a WHERE clause.
                                       SQL_PRED_SEARCHABLE for all except
                                       SQL_PRED_NONE for long varchar and long byte.
                                                       IRD-R         */
    CHAR *        TableName;        /* Result set's base table name
                                                       IRD-R         */
/*  SQLCHAR *     TypeName;          * Data source-dependent type name
                                       (e.g "CHAR", "VARCHAR").
                                                       IRD-R, IPD-R  */
    SQLSMALLINT   Unnamed;          /* Indicates if SQL_DESC_NAME has a
                                       column-name (or n/a) (IRD) or 
                                       parameter name (IPD).
                                       Application can only set IPD to SQL_UNNAMED.
                                                       IRD-R, IPD-RW */
/*  SQLSMALLINT   Unsigned;          * TRUE if column type is unsigned or 
                                       non-numeric.
                                                       IRD-R, IPD-R  */
/*  SQLSMALLINT   Updatable;         * Result column is read-only, read-write,
                                       or unknown
                                                       IRD-R         */
    UWORD         fStatus;          /* Column status. */
#define COL_BOUND       0x0001          /* ARD column or APD parm has been bound. */
#define COL_RETURNED    0x0002          /* ARD column has been returned. */
#define COL_MULTISEGS   0x0004          /* IRD column has multi segs */
#define COL_PUTDATA     0x0008          /* APD SQLPutData called for parm. */
#define COL_PASSED      0x0010          /* APD parm has been passed to driver. */
#define COL_FIXED       0x0020          /* APD parm is fixed length */
#define COL_ODBC10      0x0040          /* APD parm is for an ODBC 1.0 app. */
#define COL_CVTFLTINT   0x0080          /* IRD catalog column requires FLT to INT conv */
#define COL_DATAPTR_ALLOCED 0x0100      /*     DataPtr is malloc'ed */

    i2            fIngApiType;      /* Ingres API data type */
    UDWORD       cbNullOffset;      /* IRD, IPD Offset of null  in row. */
    BOOL         Null;              /* replacement for cbNullOffset */
    UDWORD       cbDataOffset;      /* IRD, IPD Offset of value in row. */
    SQLUINTEGER      cbPartOffset;      /* ARD, APD Offset to next part to return. */
    SWORD        cbDataOffsetAdjustment;
                                    /* IRD, IPD Adjustment to cbDataOffset used
                                       by catalog to convert char-->varchar.
                                       Usually 0 or 2                 */
    SQLUINTEGER     * BindOffsetPtr;    /* Pointer to binding offset to override
                                       header's BindOffsetPtr.  Used internally
                                       by procedure parameter processing for 
                                       literal parameters */

    BOOL          cvtToWChar;           /* char to be converted to wchar */
    LPLONGBUFFER  plb;                  /* ptr to buffer to fetch long data */
    SQLINTEGER    SegmentLength;        /* SQLPutData segment length */
    SQLINTEGER    prevOctetLength;      /* SQLGetData prev octet length */
    SQLUSMALLINT  IsoTimePrecision;     /* Precision specifier for ISO times */
    SQLBIGINT     blobLen;              /* Total blob length */
}  DESCFLD;


/*
**  Internal global function prototypes:
*/
RETCODE  AllocConnect    (LPENV  penv, LPDBC  *ppdbc);
RETCODE  AllocDescriptor (LPDBC pdbc,  LPDESC *pdesc);
RETCODE  AllocEnv        (LPENV * ppenv);
RETCODE  AllocStmt       (LPDBC pdbc,  LPSTMT *pstmt);
LPDESC   AllocDesc_InternalCall(LPDBC pdbc, SQLSMALLINT AllocType, SQLSMALLINT count);
void    AllocPreparedStmtID (LPSTMT pstmt);
BOOL    AnyIngresCursorOpen (LPDBC pdbc);
VOID    CacheFree (LPDBC);
CHAR     * CacheGet (LPDBC, LPSTMT);
RETCODE CachePut (LPDBC, LPSTMT);
RETCODE CASQLSets (LPSTMT);
BOOL    CatGetDBcapabilities(LPDBC pdbc, LPSTMT pstmtparm);
BOOL    CatGetDBconstants   (LPDBC pdbc, LPSTMT pstmtparm);
VOID    CatFetchBuffer (LPSTMT);
VOID    CatPrepareSqlda (LPSTMT);
VOID    CatSqlsid (LPSTMT, BOOL fOn);
VOID    CatToDB_NAME_CASE(LPDBC pdbc, char     * s);
int     CatGetTableNameLength(LPDBC pdbc, LPSTMT pstmtparm);
RETCODE CheckConsistencyOfDescriptorField(LPDESC pdesc, LPDESCFLD papd);
VOID    CloseCursors (LPDBC);
RETCODE ConvertCharToWChar(LPVOID lpv, 
                           char     * szValue,      SQLINTEGER cbValue,
                           SQLWCHAR * rgbWideValue, SQLINTEGER cbWideValueMax,
                           SWORD    * pcbValue,     SDWORD   * pdwValue,
                           SQLINTEGER   * plenValue);
RETCODE ConvertWCharToChar(LPVOID     lpv,
                           SQLWCHAR * szWideValue,  SQLINTEGER     cbWideValue,
                           char     * rgbValue,     SQLINTEGER     cbValueMax,
                           SWORD    * pcbValue,     SDWORD   * pdwValue,
                           SQLINTEGER   * plenValue);
RETCODE ConvertUCS2ToUCS4(u_i2 * p2, u_i4 * p4, SQLINTEGER len);
RETCODE ConvertUCS4ToUCS2(u_i4 * p4, u_i2 * p2, SQLINTEGER len);
DWORD   CountLiteralItems(SQLINTEGER c, CHAR * p, UDWORD fAPILevel);
BOOL    CursorOpen (LPDBC, UWORD fCatalog);
BOOL    AnyIngresAPICursorOpen (LPDBC pdbc);
void    ErrFlushErrorsToSQLCA(LPVOID lpv);
void    ErrResetDbc (LPDBC);
void    ErrResetDesc(LPDESC);
void    ErrResetEnv (LPENV);
void    ErrResetStmt (LPSTMT);
UDWORD  ErrSetCvtMessage (LPSTMT);
RETCODE ErrSetSqlca (LPSQLCA, CHAR      * szSqlState, LPSTR FAR * pMessage, LPSESS);
VOID    ErrSetToken (LPSTMT pstmt, CHAR     * szSqlStr, SDWORD cbSqlStr);
RETCODE ErrState (UINT err, LPVOID lpv, ...);
RETCODE ExecuteStmt (LPSTMT pstmt);
RETCODE ExecuteSelectStmt (LPSTMT pstmt);
RETCODE ExecuteDirect (LPSTMT pstmt);
RETCODE FindDescIRDField(LPSTMT pstmt, UWORD icol, LPDESCFLD * ppird);
RETCODE FreeConnect(LPDBC pdbc);
RETCODE FreeDescriptor(LPDESC pdesc);
RETCODE FreeEnv(LPENV penv);
void    FreePreparedStmtID (LPSTMT pstmt);
void    FreeSqlStr (LPSTMT);
RETCODE FreeStmt(LPSTMT pstmt, UWORD fOption);
void    FreeStmtBuffers (LPSTMT);
void    FreeTransDLL (LPDBC);
void    GenerateCursor (LPSTMT);
SQLINTEGER GetAttrLength(UWORD);
VOID    GetBoundDataPtrs(LPSTMT, i2, LPDESC, LPDESCFLD,
                         char **, SQLINTEGER **, SQLINTEGER **);
RETCODE GetChar (LPVOID, CHAR     *, CHAR FAR *, SWORD, SWORD FAR *);
RETCODE GetCharAndLength(LPVOID, CHAR*,CHAR*,SQLINTEGER, SQLINTEGER*);
RETCODE GetColumn (LPSTMT, LPDESCFLD, LPDESCFLD, CHAR     *);
SQLSMALLINT GetDescCaseSensitive(LPDESCFLD pcol);
UWORD       GetDescBoundCount   (LPDESC    pdesc);
SQLINTEGER  GetDescDisplaySize  (LPDESCFLD pcol);
char *      GetDescLiteralPrefix(LPDESCFLD pcol);
char *      GetDescLiteralSuffix(LPDESCFLD pcol);
SQLSMALLINT GetDescRadix        (LPDESCFLD pcol);
SQLSMALLINT GetDescSearchable   (LPDESCFLD pcol);
char*       GetDescTypeName     (LPDESCFLD pcol);
SQLSMALLINT GetDescUnsigned     (LPDESCFLD pcol);
BOOL    GetInfoBit (LPDBC, UWORD, UDWORD     *);
RETCODE GetParmValues(LPSTMT, LPDESCFLD, LPDESCFLD );
void    GetSQLMaxLen(LPSTMT, SWORD fSqlType, i4     * p);
void    GetTuplePtrs(LPSTMT pstmt, char * tp, int colno, char ** dp, i4 **np);
VOID InsertSqlcaMsg(LPSQLCA psqlca, CHAR * pMsg,
                       CHAR * pSqlState, BOOL isaDriverMsg);
BOOL    isApiCursorNeeded(LPSTMT pstmt);
RETCODE LoadTransDLL (LPDBC);

void             LockEnv (LPENV);
BOOL             LockDbc (LPDBC);
BOOL             LockDbc_Trusted (LPDBC pdbc);
BOOL             LockDesc (LPDESC pdesc);
BOOL             LockStmt (LPSTMT);
void    FASTCALL UnlockEnv (LPENV);
void    FASTCALL UnlockDbc (LPDBC);
RETCODE FASTCALL ErrUnlockDbc (UINT, LPDBC);
RETCODE FASTCALL ErrUnlockDesc (UINT err, LPDESC pdesc);
RETCODE FASTCALL ErrUnlockStmt (UINT, LPSTMT);
WORD ErrGetSqlcaMessageLen( SQLSMALLINT, LPSQLCA);
#define UnlockDesc(p) UnlockDbc ((p)->pdbc)
#define UnlockStmt(p) UnlockDbc ((p)->pdbcOwner)

SQLINTEGER  ParmLength (LPSTMT, LPDESCFLD papd, LPDESCFLD pipd);
RETCODE ParseCommand (LPSTMT, CHAR     * szSqlStr, SDWORD cbSqlStr, BOOL 
    rePrepared);
RETCODE ConvertParmMarkers(CHAR     *, SQLINTEGER *, CHAR *, SDWORD *);
RETCODE ConvertEscAndMarkers(CHAR     *, SQLINTEGER *, CHAR *, SDWORD *, 
     SDWORD ); 
RETCODE GetProcedureReturn(LPSTMT);
BOOL    ParseConvert (LPSTMT,LPSTR,LPSTR *,BOOL, UDWORD fAPILevel);
#define    isaParameterTRUE  TRUE
#define    isaParameterFALSE FALSE
LPSTR   ParseEscape(LPSTMT pstmt, LPSTR szSqlStr);
VOID    PopSqlcaMsg(LPSQLCA psqlca);
RETCODE PrepareConstant (LPSTMT);
RETCODE PrepareParms (LPSTMT);
RETCODE PrepareSqlda (LPSTMT);
RETCODE PrepareSqldaAndBuffer (LPSTMT, int);
RETCODE PrepareStmt (LPSTMT pstmt);
BOOL    PreparedStmt (LPDBC pdbc);
LPDESC  ReallocDescriptor(LPDESC pdesc, i2 newcount);
void    ResetDescriptorFields(LPDESC pdesc, i4 startcount, i4 endcount);
VOID    ResetDbc (LPDBC pdbc);
VOID    ResetStmt (LPSTMT, BOOL fFree);
CHAR     * ScanQuotedString(CHAR FAR * p);
CHAR     * ScanRegularIdentifier(CHAR FAR * p);
CHAR     * ScanNumber(CHAR FAR * p, i2 * papiDataType);
CHAR     * ScanSearchCondition(CHAR FAR * p, CHAR FAR ** KeywordList);
UDWORD  SetColOffset (UDWORD cbOffset, LPDESCFLD);
RETCODE SetColVar(LPSTMT, LPDESCFLD papd, LPDESCFLD pipd);
void    SetDescDefaultsFromType(LPDBC pdbc, LPDESCFLD pird);
void    SetServerClass (LPDBC pdbc);
RETCODE SetTransaction (LPDBC, LPSESS);
#ifdef _WIN64
SQLRETURN  SQL_API SQLColAttribute_InternalCall (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength, SQLLEN *NumericAttribute);
#else
SQLRETURN  SQL_API SQLColAttribute_InternalCall (SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
           SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
           SQLSMALLINT *StringLength, SQLPOINTER NumericAttribute);
#endif
RETCODE SQL_API SQLColAttributes_InternalCall(  /* superceded by SQLColAttribute */
                           SQLHSTMT     hstmt,
                           UWORD        icol,
                           UWORD        fDescType,
                           SQLPOINTER   rgbDesc,
                           SWORD        cbDescMax,
                           SWORD      * pcbDesc,
                           SDWORD     * pfDesc);
RETCODE SQL_API SQLColumns_InternalCall(
                           SQLHSTMT    hstmt,
                           UCHAR     * szTableQualifier, SWORD cbTableQualifier,
                           UCHAR     * szTableOwner,     SWORD cbTableOwner,
                           UCHAR     * szTableName,      SWORD cbTableName,
                           UCHAR     * szColumnName,     SWORD cbColumnName);

RETCODE SQL_API SQLColumnPrivileges_InternalCall(
                           SQLHSTMT         hstmt,
                           SQLCHAR        *szCatalogName,
                           SQLSMALLINT      cbCatalogName,
                           SQLCHAR        *szSchemaName,
                           SQLSMALLINT      cbSchemaName,
                           SQLCHAR        *szTableName,
                           SQLSMALLINT      cbTableName,
                           SQLCHAR        *szColumnName,
                           SQLSMALLINT      cbColumnName);

RETCODE SQL_API SQLConnect_InternalCall(LPDBC pdbc, 
                           UCHAR     * szDSN, SWORD cbDSN,
                           UCHAR     * szUID, SWORD cbUID,
                           UCHAR     * szPWD, SWORD cbPWD);
RETCODE SQL_API SQLDescribeCol_InternalCall(
                           SQLHSTMT     hstmt,
                           UWORD        icol,
                           UCHAR      * szColName,
                           SWORD        cbColNameMax,
                           SWORD      * pcbColName,
                           SWORD      * pfSqlType,
                           SQLUINTEGER    * pcbColDef,
                           SWORD      * pibScale,
                           SWORD      * pfNullable);

SQLRETURN SQL_API SQLBrowseConnect_InternalCall(
                           SQLHDBC    hdbc,
                           SQLCHAR    * szConnStrIn,
                           SQLSMALLINT cbConnStrIn,
                           SQLCHAR    * szConnStrOut,
                           SQLSMALLINT cbConnStrOutMax,
                           SQLSMALLINT * pcbConnStrOut);

RETCODE SQL_API SQLDriverConnect_InternalCall(
                           SQLHDBC     hdbc,
                           SQLHWND     hwnd,
                           UCHAR     * szConnStrIn,
                           SWORD       cbConnStrIn,
                           UCHAR     * szConnStrOut,
                           SWORD       cbConnStrOutMax,
                           SWORD     * pcbConnStrOut,
                           UWORD       fDriverCompletion);
RETCODE SQL_API SQLError_InternalCall(
                           SQLHENV      henv,
                           SQLHDBC      hdbc,
                           SQLHSTMT     hstmt,
                           SQLCHAR     *szSqlStateParm,
                           SDWORD      *pfNativeError,
                           SQLCHAR     *szErrorMsgParm,
                           SWORD        cbErrorMsgMax,
                           SWORD      * pcbErrorMsg);
RETCODE SQL_API SQLExecDirect_InternalCall(
                           LPSTMT pstmt,
                           UCHAR     * szSqlStr,
                           SQLINTEGER cbSqlStr);
RETCODE SQL_API SQLForeignKeys_InternalCall(SQLHSTMT    hstmt,
                           UCHAR     * szPkTableQualifier, SWORD cbPkTableQualifier,
                           UCHAR     * szPkTableOwner,     SWORD cbPkTableOwner,
                           UCHAR     * szPkTableName,      SWORD cbPkTableName,
                           UCHAR     * szFkTableQualifier, SWORD cbFkTableQualifier,
                           UCHAR     * szFkTableOwner,     SWORD cbFkTableOwner,
                           UCHAR     * szFkTableName,      SWORD cbFkTableName);
RETCODE SQL_API SQLGetConnectAttr_InternalCall(
                           SQLHDBC     hdbc,
                           SQLINTEGER  fOption,
                           SQLPOINTER  pvParamParameter,
                           SQLINTEGER  BufferLength,
                           SQLINTEGER *pStringLength);
RETCODE SQL_API SQLGetCursorName_InternalCall(
                           SQLHSTMT      hstmt,
                           UCHAR       * szCursor,
                           SWORD         cbCursorMax,
                           SWORD       * pcbCursor);
SQLRETURN  SQL_API SQLGetDescField_InternalCall (
                           SQLHDESC    DescriptorHandle,
                           SQLSMALLINT RecNumber,
                           SQLSMALLINT FieldIdentifier,
                           SQLPOINTER  ValuePtr,
                           SQLINTEGER  BufferLength,
                           SQLINTEGER *StringLengthPtr);
SQLRETURN  SQL_API SQLGetDescRec_InternalCall (
                           SQLHDESC     DescriptorHandle,
                           SQLSMALLINT  RecNumber,
                           SQLCHAR     *Name,
                           SQLSMALLINT  BufferLength,
                           SQLSMALLINT *StringLengthPtr,
                           SQLSMALLINT *TypePtr,
                           SQLSMALLINT *SubTypePtr,
                           SQLLEN      *LengthPtr,
                           SQLSMALLINT *PrecisionPtr,
                           SQLSMALLINT *ScalePtr,
                           SQLSMALLINT *NullablePtr);
SQLRETURN  SQL_API SQLGetDiagField_InternalCall (
                           SQLSMALLINT  HandleType,
                           SQLHANDLE    Handle,
                           SQLSMALLINT  RecNumber,
                           SQLSMALLINT  DiagIdentifier,
                           SQLPOINTER   pDiagInfoParm,
                           SQLSMALLINT  BufferLength,
                           SQLSMALLINT *pStringLength);
SQLRETURN  SQL_API SQLGetDiagRec_InternalCall ( 
                           SQLSMALLINT  HandleType,
                           SQLHANDLE    Handle,
                           SQLSMALLINT  RecNumber,
                           SQLCHAR     *pSqlStateParm,
                           SQLINTEGER  *pNativeError,
                           SQLCHAR     *pMessageTextParm,
                           SQLSMALLINT  BufferLength,
                           SQLSMALLINT *pTextLength);
RETCODE SQL_API SQLGetInfo_InternalCall(LPDBC hdbc, UWORD fInfoType,
                           SQLPOINTER  rgbInfoValueParameter,
                           SWORD cbInfoValueMax, SWORD     *pcbInfoValue);
SQLRETURN  SQL_API SQLGetStmtAttr_InternalCall (
                           SQLHENV      StatementHandle,
                           SQLINTEGER   Attribute,
                           SQLPOINTER   ValuePtr,
                           SQLINTEGER   BufferLength,
                           SQLINTEGER  *pStringLength);
RETCODE SQL_API SQLGetTypeInfo_InternalCall(
                           SQLHSTMT     hstmt,
                           SWORD        fSqlType);
RETCODE SQL_API SQLNativeSql_InternalCall(
                           SQLHDBC     hdbc,
                           SQLCHAR   * InStatementText,
                           SQLINTEGER  TextLength1,
                           SQLCHAR   * OutStatementText,
                           SQLINTEGER  BufferLength,
                           SQLINTEGER* pcbValue);
RETCODE SQL_API SQLPrepare_InternalCall(
                           SQLHSTMT    hstmt,
                           UCHAR     * szSqlStr,
                           SDWORD      cbSqlStr,
                           BOOL        rePrepared);
RETCODE SQL_API SQLPrimaryKeys_InternalCall(
                           SQLHSTMT    hstmt,
                           UCHAR     * szTableQualifier, SWORD cbTableQualifier,
                           UCHAR     * szTableOwner,     SWORD cbTableOwner,
                           UCHAR     * szTableName,      SWORD cbTableName);
RETCODE SQL_API SQLProcedures_InternalCall(
                           SQLHSTMT    hstmt,
                           UCHAR     * szProcQualifier,  SWORD cbProcQualifier,
                           UCHAR     * szProcOwner,      SWORD cbProcOwner,
                           UCHAR     * szProcName,       SWORD cbProcName);
RETCODE SQL_API SQLProcedureColumns_Internal(
                           SQLHSTMT    hstmt,
                           UCHAR     * szProcQualifier,  SWORD cbProcQualifier,
                           UCHAR     * szProcOwner,      SWORD cbProcOwner,
                           UCHAR     * szProcName,       SWORD cbProcName,
                           UCHAR     * szColumnName,     SWORD cbColumnName);
RETCODE SQL_API SQLSetConnectAttr_InternalCall(
                           SQLHDBC     hdbc,
                           SQLINTEGER  fOption,
                           SQLPOINTER  pValue,
                           SQLINTEGER  StringLength);
RETCODE SQL_API SQLSetCursorName_InternalCall(
                           SQLHSTMT      hstmt,
                           UCHAR       * szCursorParameter,
                           SWORD         cbCursor);
SQLRETURN SQL_API SQLSetDescField_InternalCall (
                           SQLHDESC    DescriptorHandle,
                           SQLSMALLINT RecNumber,
                           SQLSMALLINT FieldIdentifier,
                           SQLPOINTER  ValuePtr,
                           SQLINTEGER  BufferLength);
RETCODE SQL_API SQLSpecialColumns_InternalCall(
                           SQLHSTMT    hstmt,
                           UWORD       fColType,
                           UCHAR     * szTableQualifier, SWORD cbTableQualifier,
                           UCHAR     * szTableOwner,     SWORD cbTableOwner,
                           UCHAR     * szTableName,      SWORD cbTableName,
                           UWORD       fScope,
                           UWORD       fNullable);
RETCODE SQL_API SQLSetStmtAttr_InternalCall(
                           SQLHSTMT   hstmt,
                           SQLINTEGER fOption,
                           SQLPOINTER ValuePtr,
                           SQLINTEGER StringLength);
RETCODE SQL_API SQLStatistics_InternalCall(
                           SQLHSTMT    hstmt,
                           UCHAR     * szTableQualifier, SWORD cbTableQualifier,
                           UCHAR     * szTableOwner,     SWORD cbTableOwner,
                           UCHAR     * szTableName,      SWORD cbTableName,
                           UWORD       fUnique,
                           UWORD       fAccuracy);
RETCODE SQL_API SQLTables_InternalCall(
                           SQLHSTMT    hstmt,
                           UCHAR     * szTableQualifier, SWORD cbTableQualifier,
                           UCHAR     * szTableOwner,     SWORD cbTableOwner,
                           UCHAR     * szTableName,      SWORD cbTableName,
                           UCHAR     * szTableType,      SWORD cbTableType);

RETCODE SQL_API SQLTablePrivileges_InternalCall(
                           SQLHSTMT         hstmt,
                           SQLCHAR        *szCatalogName,
                           SQLSMALLINT      cbCatalogName,
                           SQLCHAR        *szSchemaName,
                           SQLSMALLINT      cbSchemaName,
                           SQLCHAR        *szTableName,
                           SQLSMALLINT      cbTableName);
                           
RETCODE SQL_API SQLTransact_InternalCall(LPENV penv, LPDBC pdbc, UWORD fType);
void    TraceOdbc (int iApi, ...);
void    InitTrace();
void    TermTrace();

VOID odbc_cancel(LPSTMT pstmt);
BOOL odbc_close(LPSTMT pstmt);
RETCODE odbc_AutoCommit(LPSQB, BOOL);
BOOL odbc_commitTran  (VOID * *tranHandle, LPSQLCA psqlca);
BOOL odbc_rollbackTran(VOID * *tranHandle, LPSQLCA psqlca);
BOOL odbc_query(SESS * pSession, LPSTMT  pstmt, IIAPI_QUERYTYPE apiQueryType, LPSTR pQueryText);
VOID odbc_hexdump(char *, i4);

/*
**  The GET_WM_COMMAND_ID macro is supposed to allow the same source
**  to be used for 16 and 32 bit compiles, but the C 7.0 compiler
**  is too old to have it, so here is a the 16 bit equivalent:
*/
#if defined(WIN16) && !defined(GET_WM_COMMAND_ID)
#define GET_WM_COMMAND_ID(wParam,lParam) wParam
#endif

#ifdef WIN16
i2 IIInitIngres(HANDLE);
void    IITermIngres(void);
#endif

/* TEMPORARY!!!??? Remove when F_OD0151_IDS_ERR_NOT_READ_ONLY defined in erodbc.msg */
#ifndef F_OD0151_IDS_ERR_NOT_READ_ONLY
#define F_OD0151_IDS_ERR_NOT_READ_ONLY 0x10eb0151
#endif

/*
** The following section depends on the date/time definitions defined in
** adudate.h.  This must be kept in sync with adudate.h at all times.
*/
/*}
** Name: ODBC_DATENTRNL - defines the layout of an Ingres DATE value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard INGRESDATE data type as it appears in the data base.
**
** History:
**      02-oct-06 (Ralph Loen) SIR 116477
**          Written for the date/time enhancements.
*/

typedef struct _ODBC_DATENTRNL
{
    char	    date_spec;
#define ODBC_DATE_NULL     0x00   /* Ingres date is the empty date */
#define ODBC_TIME_INTERVAL 0x02   /* Ingres date is a time interval */
#define ODBC_YEARSPEC      0x04   /* Year specified */
#define ODBC_MONTHSPEC     0x08   /* Month specified */
#define ODBC_DAYSPEC       0x10   /* Day specified */
#define ODBC_TIMESPEC      0x20   /* Time specified */
    i1		    highday;	
    i2		    year;
    i2		    month;	
    u_i2	    lowday;
    i4		    field6;
}   ODBC_DATENTRNL;
typedef ODBC_DATENTRNL	ODBC_OLDDTNTRNL;

/*}
** Name: ODBC_ADATE	- defines the layout of an ANSI DATE value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard DATE data type as it appears in the data base.
**
** History:
**      02-oct-06 (Ralph Loen) SIR 116477
**          Written for the date/time enhancements.
*/

typedef	struct _ODBC_ADATE
{
    i2		year;	
    i1		month;	
    i1		day;	
}   ODBC_ADATE;

/*}
** Name: ODBC_TIME	- defines the layout of an ANSI TIME value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard TIME data type as it appears in the data base. NOTE,
**	the hours and minutes components of the time are converted to seconds
**	and added to the seconds of the time to produce the seconds 
**	component (0-86400).
**
** History:
**      02-oct-06 (Ralph Loen) SIR 116477
**          Written for the date/time enhancements.
*/

typedef	struct _ODBC_TIME
{
    i4		field1;	
    i4		field2;	
    i1		field3;	
    i1		field4;	
}   ODBC_TIME;

/*}
** Name: ODBC_TIMESTAMP	- defines the layout of an ANSI TIMESTAMP value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard TIMESTAMP data type as it appears in the data base.
**
** History:
**      02-oct-06 (Ralph Loen) SIR 116477
**          Written for the date/time enhancements.
*/

typedef	struct _ODBC_TIMESTAMP
{
    i2		year;	
    i1		month;
    i1		day;
    i4		hour;
    i4		minute;
    i1		second;
    i1		fraction;
}   ODBC_TIMESTAMP;

/*}
** Name: ODBC_INTYM	- defines the layout of an ANSI YEAR TO MONTH 
**	INTERVAL value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard YEAR TO MONTH INTERVAL data type as it appears in the 
**	data base.
**
** History:
**      02-oct-06 (Ralph Loen) SIR 116477
**          Written for the date/time enhancements.
*/

typedef	struct _ODBC_INTYM
{
    i2		year;	
    i1		month;
}   ODBC_INTYM;

/*}
** Name: ODBC_INTDS	- defines the layout of an ANSI DAY TO SECOND 
**	INTERVAL value
**
** Description:
**	This typedef describes the fields used to define an instance of the 
**	SQL standard DAY TO SECOND INTERVAL data type as it appears in the 
**	data base.
**
** History:
**	02-oct-06 (Ralph Loen) SIR 116477
**	    Written for the date/time enhancements.
*/

typedef	struct _ODBC_INTDS
{
    i4		hour;
    i4		minute;
    i4		second;	
    i4          fraction;            /* nanoseconds component */
}   ODBC_INTDS;

/*
** Date/time destination lengths for API char/date conversions.
*/

#define	    ODBC_DTE_LEN     	   sizeof(ODBC_DATENTRNL)
#define	    ODBC_DATE_LEN	    4
#define	    ODBC_TIME_LEN	   10	
#define	    ODBC_TS_LEN	           14
#define	    ODBC_INTYM_LEN          3	
#define	    ODBC_INTDS_LEN	   12	
#define	    ODBC_DTE_OUTLENGTH	   25 /* string length of ingresdate output */
#define     ODBC_DATE_OUTLENGTH    17 /* string length of ansidate output */
#define     ODBC_TMWO_OUTLENGTH    21 /* string length of TMWO output */
#define     ODBC_TMTZ_OUTLENGTH    31 /* string length of TMW output */
#define     ODBC_TSWO_OUTLENGTH    39 /* string length of TMWO output */
#define     ODBC_TSTZ_OUTLENGTH    49 /* string length of TSW output */
#define     ODBC_INTYM_OUTLENGTH   15 /* string length of INYM output */
#define     ODBC_INTDS_OUTLENGTH   45 /* string length of INDS output */

/*
** End of date/time section   
*/

#ifdef __cplusplus
}
#endif 

#endif  /* _INC_IDMSODBC */
