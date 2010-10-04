/*
** Copyright (c) 1992, 2010 Ingres Corporation
*/

#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <gl.h>
#include <iicommon.h>
#include <cm.h>
#include <cv.h>
#include <me.h>
#include <st.h> 
#include <tr.h>
#include <erodbc.h>
#include <clfloat.h>
#include <iiapi.h>
#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */

#include <idmsocvt.h>               /* ODBC driver conversion routines. */

#define ODBCfabs(x) (((x) < 0)? -(x) : (x))  /* floating point abs */
#define ADD_INTPTR_AND_OFFSET(ptr,offset) (SQLINTEGER*)((char*)(ptr) + (offset))

/*
** Name: EXECUTE.C
**
** Description:
**	Execute SQL command routines for ODBC driver.
**
** History:
**	10-dec-1992 (rosda01)
**	    Initial coding
**	14-mar-1995 (rosda01)
**	    ODBC 2.0 upgrade...
**	17-jan-1997 (tenge01)
**	    put current stmt handle in sqb
**	17-jan-1997 (tenge01)
**	    added SqlCancel
**	22-jan-1997 (tenge01)
**	    ExecuteStmt ...
**	07-feb-1997 (tenge01)
**	    PutCharChar ...  
**	07-feb-1997 (thoda04)
**	    Moved connH/tranH to SESS
**	10-feb-1997 (tenge01)
**	    PutNumChar didn't set length if sql_varchar
**	10-feb-1997 (tenge01)
**	    PutCharBin ... 
**	27-feb-1997 (tenge01)
**	    Added code for calling a procedure 
**	10-mar-1997 (tenge01)
**	    PutParm, SQL_DEFAULT_PARAM ...
**	20-mar-1997 (tenge01)
**	    Fixed a coding error in ExecuteStmt
**	21-mar-1997 (thoda04)
**	    Bug fix for SQLExecDirect passing SQL_NTS 
**	25-mar-1997 (thoda04)
**	    Bug fix for SQL_DECIMAL ParmLength 
**	    Bug fixes for {ts '1997-...'} 
**	27-mar-1997 (thoda04)
**	    Removed CASQLSets baggage. 
**	28-mar-1997 (tenge01)
**	    replaced SetAutoResult to SQLTransact 
**	28-mar-1997 (tenge01)
**	    PutParm, pcbValue was NULL when SQLBindParameter called by DM  
**	18-apr-1997 (tenge01)
**	    turned on SESS_COMMIT_NEEDED when necessary
**	22-apr-1997 (tenge01)
**	    added a new argument to ParseConvert 
**	24-apr-1997 (tenge01)
**	    fixed a bug in PutParm when stripping trainling quote 
**	30-apr-1997 (thoda04)
**	    Trap SET AUTOCOMMIT ON/OFF.
**	01-may-1997 (thoda04)
**	    Refresh cbPartOffset in SQLPutData. 
**	13-jun-1997 (tenge01)
**	    added trailing 0s for db2 sql_date type 
**	16-jun-1997 (tenge01)
**	    prepare/execute, instead of exec imm, if update cursor   
**	17-jun-19976 (tenge01)
**	    remove my 6/13 changes. DB2 data type date and time are
**	    converted to char by the gateway. If app accesses a db2
**	    table (not created by the gateway) having a true db2 date
**	    data type, then the trailing zeroes added by our driver
**	    will cause error. 
**	28-jul-1997 (tenge01)
**	    converted db2 date format to ingres us format if exec procedure
**	12-aug-1997 (tenge01)
**	    When using SQLParamOptions to insert multiple rows, db2 and
**	    dcom failed on the 2nd insert because the new autocommit code.
**	13-aug-1997 (tenge01)
**	    removed CvtTimestampChar_DB2 calls. For gateways, we will
**	    pass ingres internal date format to OpenAPI and let gateways
**	    take care of the timezone. 
**	08-sep-1997 (tenge01)
**	    using pcol->pParmBuffer instead of pstmt->pParmBuffer in PutParm
**	17-sep-1997 (thoda04)
**	    Trap COMMIT or ROLLBACK and call SQLTransact instead
**	02-oct-1997 (tenge01)
**	    added SQL_LONGVARBINARY to PutCharChar
**	11-nov-1997 (thoda04)
**	    Documentation clean-up
**	    Check for API_CURSOR_OPENED, not WHERE_CURRENT_CURSOR
**	25-nov-1997 (thoda04)
**	    Support of multiple cursors if AUTOCOMMIT=ON
**	05-dec-1997 (tenje01)
**	    conver C run-time functions to CL
**	31-dec-1997 (thoda04)
**	    If SQL_C_BINARY->(CHAR,VARCHAR,BINARY,VARBIN,etc) don't
**	    double length
**	06-feb-1998 (thoda04)
**	    Fixed sizes from 16-bit to 32-bit
**	24-feb-1998 (thoda04)
**	    Ignore change in parm buff size if IIAPI_DTE_TYPE.
**	19-may-1998 (thoda04)
**	    For SQL_TIMESTAMP, pass yyyy.mm.dd instead of yyyy-mm-dd
**	    to Ingres.
**	20-may-1998 (thoda04)
**	    Strip trailing '.' for float->char conversions
**	26-oct-1998 (thoda04)
**	    Changes for MS Distributed Transaction Coordinator
**	07-jan-1999 (thoda04)
**	    Make sure the SQL control block anchor is initialized.
**	20-jan-1999 (thoda04)
**	    Auto-commit updates at SQLFreeStmt(SQL_CLOSE) if needed.
**	08-mar-1999 (thoda04)
**	    Port to UNIX.
**	22-apr-1999 (thoda04)
**	    Add prepare_or_describe parm to PrepareStmt.
**	23-jun-1999 (thoda04)
**	    If procedure did a commit or rollback, no commit needed.
**	14-sep-1999 (Bobby Ward)
**	    Fixed bug# 98765 Error when updating Decimal
**	01-dec-1999 (thoda01)
**	    Send SQL_DATE parms in Ingres yyyy_mm_dd format.
**	13-mar-2000 (thoda04)
**	    Add support for Ingres Read Only.
**	01-may-2000 (loera01)
**	    Bug 100747: Replaced check for "INGRES" in the registry
**	    with isServerClassINGRES macro, so that user-defined server
**	    classes are interpreted as Ingres classes.
**	16-may-2000 (thoda04)
**	    Be forgiving of prepare/execute of COMMIT or ROLLBACK
**	05-sep-2000 (thoda04)
**	    For blobs, use actual len rather than buffer len
**	06-dec-2000 (loera01)
**	    Bug 103427: For SQLExecDirect(), execute a rollback via
**	    SQLTransact_InternalCall() if a deadlock error is returned
**	    from the dbms.
**	11-dec-2000 (feiwe01)
**	    Bug 102162: Added new internal routine CountQuestionMark()
**	    to assist in proper allocation of space for queries with
**	    parameters.
**	11-jan-2001 (thoda04)
**	    Integrate MTS/DTC support into 2.8
**	29-may-2001 (thoda04)
**	    Upgrade to 3.5
**	12-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	16-jul-2001 (thoda04)
**	    Add support for SQL_WLONGVARCHAR (Unicode blobs)
**	31-jul-2001 (thoda04)
**	    Add support for parameter offset binding.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-mar-2002 (thoda04)
**	    Add support for row-producing procedures.
**	01-nov-2001 (thoda04)
**	    ParmLength should not try to allocate more than a billion bytes
**	01-feb-2001 (thoda04)
**	    Wrap {ts ...} in date scalar function for gateways
**	21-mar-2002 (thoda04)
**	    Let SQLCancel cancel another thread's query before we lock on DBC
**      19-aug-2002 (loera01) Bug 108536
**          First phase of support for SQL_C_NUMERIC.
**      04-nov-2002 (loera01) Bug 108536
**          Second phase of support for SQL_C_NUMERIC.  Added coersions for all
**          supported datatypes besides SQL_NUMERIC and SQL_DECIMAL.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**	02-Apr-2003 (hanje04) Bug 109986
**	    Cast SQLINTEGER types with (i4) when comparing with SQL_NTS to prevent
**	    problems on 64bit platforms. (Possible compiler bug on AMD64 Linux)
**      06-may-2003 (weife01) Bug 109945
**          Reset date, time, timestamp to 19 for sqllen.
**      28-May-2003 (loera01)     
**          In PutParm() use CvtApiTypes() for converting from SQL_C_CHAR to
**          decimal.
**     22-sep-2003 (loera01)
**          Added internal ODBC tracing.
**     17-oct-2003 (loera01)
**         Added new SqlToIngresApi() cases SqlExecProc and SqlPutParms. 
**         Changed "IIsyc_" routines to "odbc_".  Removed pragma precompiler
**         defines, and removed references to bulk insert execution.
**     05-nov-2003 (loera01)
**          Sending parameter descriptors is now invoked from
**          SQLExecute() or SQLExecDirect(). 
**     13-nov-2003 (loera01)
**          Add capability to send true segments from SQLPutData().
**          New routine SegmentLength().
**     20-nov-2003 (loera01)
**          More cleanup on arguments sent to SqlToIngresAPI().
**          Rolled in database procedures to the state machine.
**          Added documentation for new routines and other
**          miscellaneous cleanup.
**    21-nov-2003 (loera01)
**          In PutParms(), make sure copy length is correctly cast
**          for MEcopy() and MEreqmem().  Added logic to
**          SQLPutData() so that segments are allocated only as
**          required.
**    04-dec-2003 (loera01)
**          Changed "STMT_" command flags to "CMD_".
**    24-mar-2004 (loera01)  Bug 112019
**          In SQLExecDirect_Common, added missing reference to
**          SetTransaction().
**    23-apr-2004 (loera01)
**          In PutParm() don't use a union as shorthand for comparing 
**          ODBC types, as this fails on Itanium HP.  Instead, compare
**          individual table elements. 
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**     27-apr-2004 (loera01)
**          First phase of support for SQL_BIGINT.
**     29-apr-2004 (loera01)
**          Second phase of support for SQL_BIGINT.
**     15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
**     19-nov-2004 (Ralph.Loen@ca.com) Bug 113474
**          In SQLExecDirect_Common(), invoke SetTransaction() before
**          executing queries, not after.
**     10-dec-2004 (Ralph.Loen@ca.com) SIR 113610
**          Added support for typeless nulls.
**     04-mar-2005 (Ralph.Loen@ca.com) Bug 114018
**          In ExecuteStmt() and GetBoundDataPtrs(), added logic to handle
**          column-wise row binding for bulk inserts.
**     27-apr-2005 (loera01) Bug 114418
**          In SQLExecute(), re-prepare the statement if the STMT_PREPARED
**          flag is clear.
**     06-may-2005 (weife01) Bug 113684
**          When send date to DB2, use format YYYY-MM-DD instead of 
**          YYYY_MM_DD.
**     26-may-2005 (loera01) Bug 114577
**          In GetBoundDataPtrs(), use the octet length in the ARD as the
**          length for character data--don't decrement by 1.
**     14-jul-2004 (loera01) Bug 114851
**          In SQLExecute(), initialize segment counter fields in the 
**          statement control block.
**     26-aug-2005 (loera01) Bug 115105
**          In SQLExecDirect_Common(), always invoke ExecuteStmt() for non- 
**          select queries that have dynamic parameters, not just update
**          queries.
**     01-dec-2005 (loera01) Bug 115358
**          Added argument to SQLPrepare_InternalCall() so that the statement
**          handle buffers aren't freed when a statement is re-prepared.
**    02-dec-2005 (loera01) Bug 115582
**          In PutParm(), return CVT_FRAC_TRUNC if a fractional truncation is
**          detected.   Check for this condition in SQLExecute() and return
**          SQL_SUCCESS_WITH_INFO if detected.
**    23-feb-2006 (loera01) Bug 115789
**          In PutParm(), allow the "or" indicator argument from 
**          SQLBindParameter() (pcbValue) to truncate the input string if
**          necessary.
**    29-mar-2006 (Ralph Loen) Bug 115908
**          In ParmLength(), when then length cannot be determined, 
**          return zero rather than 255.  In PutParm(), allow an extra
**          two bytes in the result when converting from Unicode to
**          multibyte via ConvertWCharToChar(), since the fix for bug
**          115517 returns the result as a varchar instead of a char.
**    19-may-2006 (Ralph Loen) Bug 116116
**          In GetBoundDataPtrs(), removed code that set the bind offset 
**          pointer to NULL if column-wise binding was specified.
**    16-june-2006 (Ralph Loen) SIR 116260 
**          Add SQLDescribeParam().
**    10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase).
**    16-Aug-2006 (Ralph Loen) SIR 116477 (second phase)
**          Add support for ISO timestamps and DS intervals.
**    28-Sep-2006 (Ralph Loen) SIR 116477
**          Adjusted treatment of ISO date/times according to new API/ADF rules.
**    03-Oct-2006 (Ralph Loen) SIR 116477
**          Dynamic parameters for dates and timestamps are now sent as binary
**          values for backward-compatibility with ingresdate.
**    10-oct-2006 (Fei Ge) bug 116827
**          In PutParm(), fixed DATE/TIMESTAMP/TIME conversions between
**          C types and SQL types.
**    23-oct-2006 (Ralph Loen) SIR 116477
**          In PutParm(), send binary versions of interval parameters.  
**          Convert timestamps to IIAPI_TSWO_TYPE instead of IIAPI_DTE_TYPE.
**    22-Nov-2006 (Ralph Loen) SIR 116477
**          In CvtCharIngresDateTime(), determine seconds precision from 
**          digits after the decimal point.
**    02-Mar-2006 (Ralph Loen) Bug 117825
**          Added support for SQL_TYPE_NULL in isTypeSupported().
**    20-Aug-2008 (Ralph Loen) Bug 117825
**          The actual check for SQL_TYPE_NULL was not done in 
**          isTypeSupported().
**    21-Aug-2007 (Ralph Loen) Bug 118993
**          Remove references to unused "pcrowParm" field in the
**          statement handle.  Initialize SQLNRP  (number of rows processed)
**          to zero before executing a statement.
**    29-Sep-2007 (weife01) Bug 119013
**          Remove conditional check of decrement octet length, set the
**          octet length in the ARD as the length for character data in
**          GetBoundDataPtrs(). 
**    25-Feb-2008 (Ralph Loen) Bug 119975
**          In SQLExecDirect_Common(), invoke ConvertEscAndMarkers() if
**          the API connection level is greater than 3().
**    27-Feb-2008 (Ralph Loen) Bug 119730
**          In SQLPutData(), allow for adjusted length for SQL_C_WCHAR
**          converted to char data.
**    29-Feb-2008 (Ralph Loen) Bug 119730
**          Reference to WCHAR typedef doesn't port on Linux.  Changed to
**          SQLWCHAR.
**    08-May-2008 (Ralph Loen) SIR 120340
**          In SQLExecute(), invoke SQLExecDirect_InternalCall() if
**          the statement status mask does not STMT_CANBEPREPARED set.
**    21-May-2008 (Ralph Loen) Bug 120412
**          Check for szPSqlstr and cbPSsqlstr instead of
**          szSqlStr and cbSqlStr to determine whether
**          SQLExecDirect_InternalCall() should be called.
**    29-May-2008 (Ralph Loen) Bug 120437
**          In PutParm(), adjust the octet length downward when converting
**          from SQL_C_WCHAR to SQL_CHAR or SQL_WCHAR and the parameter is
**          not a segment and has a specified length.
**     31-Jul-2008 (Ralph Loen) Bug 120718
**          In PutParm(), normalize decimal strings before converting to
**          packed decimal.  Set CVT_FRAC_TRUNC only if the scale is zero
**          only if a decimal point exists in the decimal string.
**    02-Oct-2008 (Ralph Loen) SIR 121019
**          In ExecuteStmt(), execute GetProcedureReturn() only if 
**          STMT_RETPROC is set.  Do not exit PutParms() if the 
**          parameter type is SQL_PARAM_OUTPUT.
**     03-Nov-2008 (Ralph Loen) SIR 121152
**         Added support for II_DECIMAL in a connection string.
**         The OPT_DECIMALPT bit now indicates that support for
**         II_DECIMAL has been specified in a DSN or connection string.
**         The value of II_DECIMAL is now stored in the connection
**         handle, and is assumed to be a comma or a period.  Affects
**         PutParm().
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Add support for OPT_INGRESDATE configuration parameter.  If set,
**         ODBC date escape syntax is converted to ingresdate syntax, and
**         parameters converted to IIAPI_DATE_TYPE are converted instead to
**         IIAPI_DTE_TYPE.
**     12-Feb-2009 (Ralph Loen) bug 121649
**         Added boolean argument rePrepared to ParseCommand().
**     13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**     03-Jun-2009 (Ralph Loen) Bug 122135
**         Added missing calls to UnlockStmt() at various exit points in 
**         SQLExecute(), SQLExecDirect_Common() and SQLPutData().
**     22-Jun-2009 (Ralph Loen) Bug 122222
**         In SQLDescribeParam(), anchor session control block before
**         invoking SqlDescribeInput().
**     09-Jul-2009 (Ralph Loen) Bug 122291
**         In SQLExecute(), if SQLPrepare_InternalCall() must be invoked,
**         don't unlock the statement handle before returning from
**         SQLExecute(); SQLPrepare_InternalCall() has already unocked
**         the statement handle.
**     01-Sep-2009 (Ralph Loen) Bug 122548
**         In SQLExecDirect_Common(), initialize sqlca.SQLNRP to zero
**         if a statement is to be executed with no parameters, since
**         bulk inserts are not possible. Thus, there is no need to
**         increment sqlca.SQLNRP for futher iterations of the update query.
**    22-Sep-2009 (hanje04) 
**	    Define null terminator, nt to pass to CMcpchar() instead of
**	    literal "\0" because it causes relocation errors when linking
**	    64bit intel OSX
**    10-Nov-2009 (Ralph Loen) Bug 122866
**         In SQLExecute(), replace the call to SQLPrepare_InternalCall()
**         with SQLExecDirect_Common().
**    19-Nov-2009 (Ralph Loen) Bug 122941
**         Amend the fix for bug 122866.  Invoke SQLPrepare_InternalCall()
**         instead of SQLExecDirect_Common() if SQLExecute() finds itself
**         with an un-prepared query, but do so after SQLExecute() has been
**         executed in this state five times.
**    03-Dec-2009 (Ralph Loen) Bug 123002
**         In SQLExecDirect_Common(), don't call FreeSqlStr() if
**         STMT_SQLEXECUTE is set in fStatus after SqlExecImm() has
**         been called.
**     10-Dec-2009 (Ralph Loen) Bug 123036
**         The fix for bug 120718 regressed the fix for bug 115789.  
**         Re-instate code in PutParm() that truncates the numeric string
**         when SQL_C_CHAR or SQL_C_WCHAR is bound to SQL_NUMERIC or
**         SQL_DECIMAL.
**     25-Jan-2010 (Ralph Loen) Bug 123175
**         Make timestamp conversion routines dependent on the date
**         connection level rather then the API connection level.
**     06-Feb-2010 (Ralph Loen) Bug 123231
**         In PutParm(), truncate numeric decimal strings if the
**         number of digits to the right of the decimal point is less
**         than pipd->scale, and set a fractional truncation warning. 
**     08-Feb-2010 (Ralph Loen) SIR 123266
**         Add support for SQL_BIT and SQL_C_BIT (boolean) in PutParm().
**     23-Mar-2010 (Ralph Loen) SIR 123465
**         Trace data input to PutParm() with new odbc_hexdump() function.
**     24-Mar-2010 (Ralph Loen) Bug 123470
**         In SQLDescribeParam(), subtract two from the returned column
**         size if the target column is a non-blob variable length
**         data type, to account for the length indicator.
**     24-Mar-2010 (Ralph Loen) Bug 123472
**         In PutParm(), set CVT_TRUNCATION and return SQL_ERROR if
**         the Length field of the IPD is less than the length referenced
**         in the OctetLengthPtr of the pipd.
**    14-Jul-1010 (Ralph Loen)  Bug 124079
**         In SQLExecDirect_InternalCall(), add 50 instead of 8 in
**         MEreqmem(), to allow "for readonly" to be potentially tacked on the 
**         end of a select query.
**     03-Sep-2010 (Ralph Loen) Bug 124348
**          Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**          SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**          platforms.
**     08-Sep-2010 (Ralph Loen) Bug 124307
**          Use new PutTinyInt() function for coercing datatypes into
**          SQL_BIT and SQL_TINYINT.
**     01-Oct-2010 (Ralph Loen) Bug 124517
**          In PutParm(), use tinyValue buffer for SQL_C_UTINYINT.  Group
**          treatment of SQL_C_STINYINT with SQL_C_TINYINT instead of
**          SQL_C_UTINYINT.
*/

/*
**  Internal functions:
*/
static UDWORD  PutCharBin  (LPDESCFLD, LPDESCFLD, CHAR*, CHAR*, SQLUINTEGER, UDWORD, BOOL);
static UDWORD  PutCharChar (LPDESCFLD, LPDESCFLD, CHAR*, CHAR*, SQLUINTEGER, UDWORD, CHAR, BOOL );
static UDWORD  PutTinyInt  (CHAR*, SCHAR, SWORD, BOOL);
static UDWORD  PutInt      (CHAR*, SDWORD, SWORD, BOOL);
static UDWORD  PutInt64      (CHAR*, ODBCINT64, SWORD, BOOL);
static UDWORD  PutNumChar  (LPDESCFLD, LPDESCFLD, CHAR*, CHAR*, UWORD);
static RETCODE PutParm     (LPSTMT, LPDESCFLD, LPDESCFLD, CHAR*, SQLINTEGER*, SQLINTEGER*, BOOL);
static RETCODE SQLExecDirect_Common(LPSTMT pstmt, CHAR     * szSqlStr, SQLINTEGER cbSqlStr);
static VOID         FASTCALL ExecErrCommit (LPSTMT);
static UWORD        FASTCALL SetAutoCommit (LPSTMT);
static VOID         FASTCALL SetAutoResult (LPSTMT, UWORD);
static UDWORD       FASTCALL ODBCDateToIngresUS (CHAR * rgbData);
static UDWORD       FASTCALL CvtCharToIngresDateTime(CHAR * rgbData, LPDESCFLD pipd,
    UDWORD apiLevel);
SQLINTEGER SegmentLength( LPSTMT, LPDESCFLD, LPDESCFLD, SQLPOINTER, 
    SQLINTEGER);
BOOL isTypeSupported(SWORD fCtype, SWORD fSqlType);
BOOL isStringType(SWORD type);
BOOL isNumberType(SWORD type);
BOOL isBinaryType(SWORD type) ;
BOOL isYearMonthIntervalType(SWORD type);
BOOL isCYearMonthIntervalType(SWORD type);
BOOL isDaySecondIntervalType(SWORD type);
BOOL isCDaySecondIntervalType(SWORD type);
BOOL isAtomicIntervalType(SWORD type);
BOOL isCAtomicIntervalType(SWORD type);
BOOL isCStringType(SWORD type);
BOOL isCNumberType(SWORD type);

#ifndef MAXI4
#define MAXI4 0x7fffffff
#endif

/*
**  SQLCancel
**
**  Cancel an SQL statement.
**
**  Since asynchronous operation is not currently supported for IDMS,
**  this just issues an SQLFreeStmt with the SQL_CLOSE option.
**
**  On entry: pstmt -->statement block.
**
**  On exit:  The deed is done, we hope.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/17/1997  Jean Teng           added SqlCancel
*/

RETCODE SQL_API SQLCancel(
    SQLHSTMT  hstmt)
{
    LPSTMT  pstmt = (LPSTMT)hstmt;
    RETCODE rc;
    LPDBC   pdbc    = pstmt->pdbcOwner;

    odbc_cancel(pstmt);    /* cancel the function outside of locking
                               since one thread of a connection may be calling
                               to cancel work on a different thread */

    if (!LockDbc_Trusted (pdbc))  /* assume the DBC is good */
        return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLCANCEL, pstmt);

    /*
    **  If processing SQL_NEED_DATA, cancel that:
    */
    if (pstmt->fStatus & STMT_NEED_DATA)
    {
        pstmt->fStatus &= ~STMT_NEED_DATA;
        pstmt->icolPut = 0;
        UnlockStmt (pstmt);
        return SQL_SUCCESS;
    }
    /*
    **  Otherwise just close cursor if open:
    */
    rc = FreeStmt (pstmt, SQL_CLOSE);

    /*
    **  According to the ODBC spec, we should let the driver manager
    **  know that we just did this by returning SQL_SUCCESS_WITH_INFO
    **  SQLSTATE 01S05. It is supposed to hide this from the caller,
    **  but the 16 bit DM did not appear to do so.  The 32 bit DM does
    **  appear to handle this, which it probably needs to do in a
    **  multi-threaded environment.
    */
    /* beginning with the 3.5 driver, just ignore the info of cancel open cursor
    if (rc != SQL_ERROR)
        rc = ErrState (SQL_01S05, pstmt);
    */

    UnlockDbc(pdbc);
    return rc;

}


/*
**  SQLExecute
**
**  Execute a prepared SQL statement
**
**  On entry: pstmt -->statement block.
**
**  On exit:  If a SELECT the cursor is open,
**            otherwise the command has been executed.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**            SQL_NEED_DATA
*/

RETCODE SQL_API SQLExecute(
    SQLHSTMT  hstmt)
{
    LPSTMT  pstmt = (LPSTMT)hstmt;
    RETCODE rc = SQL_SUCCESS;

    if (!(pstmt->fStatus & STMT_CANBEPREPARED) && pstmt->szPSqlStr &&
         pstmt->cbPSqlStr)
         return SQLExecDirect_InternalCall(hstmt, pstmt->szPSqlStr,
             pstmt->cbPSqlStr);

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLEXECUTE, pstmt);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    pstmt->crowBuffer  = 0;
    pstmt->crow        = 0;
    pstmt->icolPrev    = 0;
    pstmt->icolPut     = 0;
    pstmt->icolSent    = 0;

    if (!pstmt->szSqlStr)
    {
        UnlockStmt(pstmt);
        return ErrState (SQL_HY010, pstmt);  /* function sequence error*/
    }

    pstmt->irowParm = 0;    /* initialize current parameter row number */
                            /* of a parameter array */
    /*
    **  If a previous SQLFreeStmt() call invoked a commit, execute directly.
    */

    pstmt->fStatus &= ~STMT_SQLEXECUTE;
    pstmt->fStatus &= ~STMT_DIRECT;

    /*
    ** If SQLExecDirect_Common() has been called 5 times from 
    ** SQLExecute(), re-prepare the query.  Otherwise, execute directly.
    **
    ** The driver switches between these two execution modes as
    ** a compromise between two extremes related to performance.
    **
    ** A poorly-tuned application prepares once and calls commit after 
    ** every execute.  It is better in this case to execute queries directly.
    **
    ** A well-tuned application prepares once and commits after many
    ** executions.  It is better to re-prepare and keep executing the
    ** prepared query in this case.
    */
    if (!(pstmt->fStatus & STMT_PREPARED))
    {
        if (pstmt->reExecuteCount >= 5)
        {
            pstmt->fStatus &= ~STMT_SQLEXECUTE;
            pstmt->reExecuteCount = 0;

            /*
            ** Note that SQLPrepare_InternalCall() locks and unlocks the
            ** statement handle, resulting in a nested lock.  Windows allows
            ** recursive locks on the same semaphore object, but other platforms
            ** do not.  The ODBC simulates reference counts for semaphores
            ** in non-Windows environments.  See 
            ** IIODcsen_ODBCEnterCriticalSection() in lock.c.
            */
            rc = SQLPrepare_InternalCall(hstmt, pstmt->szSqlStr, 
                pstmt->cbSqlStr, TRUE);

            if (!SQL_SUCCEEDED(rc))
            {
                /*
                ** Decrement the reference count of the statement handle.
                */
                UnlockStmt(pstmt);
                return rc;
            }
        }
        else
        {
            pstmt->fStatus |= STMT_SQLEXECUTE;
            pstmt->reExecuteCount++;
            return ( SQLExecDirect_Common(hstmt,
                pstmt->szSqlStr, pstmt->cbSqlStr ) );
        }
    }

    /*
    **  Call internal execute routine:
    */
    if (pstmt->fCommand == CMD_SELECT)
        rc = ExecuteSelectStmt (pstmt);
    else 
        rc = ExecuteStmt (pstmt);
    UnlockStmt (pstmt);
    return rc;
}


/*
**  SQLExecDirect
**
**  Execute an SQL statement immediately.
**
**  Execute any statement other than a SELECT via the
**  IDMS EXECUTE IMMEDIATE command.
**
**  On entry: pstmt -->statement block.
**     
**  On exit:  If a SELECT the cursor is open,
**            otherwise the command has been executed.
**
**            The syntax passed is freed for INTERNAL calls.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**            SQL_NEED_DATA
*/

RETCODE SQL_API SQLExecDirect(
    SQLHSTMT    hstmt,
    UCHAR     * szSqlStr,
    SDWORD      cbSqlStr)
{
    if (!LockStmt ((LPSTMT)hstmt)) 
       return SQL_INVALID_HANDLE;

    return(SQLExecDirect_Common((LPSTMT)hstmt, (CHAR*)szSqlStr, cbSqlStr));
}

static RETCODE SQLExecDirect_Common(
    LPSTMT      pstmt,
    CHAR     *  szSqlStr,
    SQLINTEGER      cbSqlStr)
{
    RETCODE     rc;
    LPDBC       pdbc;
    UWORD       fCommit;
    CHAR     *  szAPISqlStr;
    SDWORD      cbAPISqlStr;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLEXECDIRECT, pstmt, szSqlStr, cbSqlStr);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    pdbc = pstmt->pdbcOwner;

    if (pstmt->fStatus & STMT_OPEN)  /* if already an active result set, error */
    {
        rc = ErrState (SQL_24000, pstmt);
        UnlockStmt (pstmt);
        return rc;
    }

    /*
    **  If this is an internal call, we've been called recursively
    **  for an ODBC catalog function, so we already know that the SQL
    **  SQL command is SELECT.  No need to parse the syntax.  Note
    **  that we will free the syntax passed to us instead of the syntax
    **  buffer that ParseCommand would allocate.
    */
    if (pstmt->fStatus & STMT_INTERNAL)
    {
        pstmt->szSqlStr = szSqlStr;
        pstmt->cbSqlStr = (SDWORD)STlength (szSqlStr);
        pstmt->icolParmLast = 0;        /* count of parameter markers = 0 */
    }
    /*
    **  Otherwise figure it out.
    */
    else if (pstmt->szSqlStr != szSqlStr)
    {
        if ((i4)cbSqlStr == SQL_NTS )
            cbSqlStr = (SDWORD)STlength (szSqlStr);

		ResetStmt (pstmt, TRUE);     /* clear STMT state and free buffers */
        szAPISqlStr = MEreqmem((u_i2)0, cbSqlStr +
           CountLiteralItems(cbSqlStr, szSqlStr, pstmt->dateConnLevel) + 50,
                              TRUE, NULL);

        if (szAPISqlStr == NULL)
        {
            UnlockStmt (pstmt);
            return ErrState (SQL_HY001, pstmt, F_OD0013_IDS_SQL_SAVE);
        }
        if (pstmt->fAPILevel > IIAPI_LEVEL_3)
            ConvertEscAndMarkers(szSqlStr, &cbSqlStr, szAPISqlStr, 
                &cbAPISqlStr, pstmt->dateConnLevel);
        else
            ConvertParmMarkers(szSqlStr, &cbSqlStr, szAPISqlStr, &cbAPISqlStr);
        rc = ParseCommand (pstmt, szAPISqlStr, cbAPISqlStr, FALSE);
        MEfree (szAPISqlStr);

        if (rc != SQL_SUCCESS)
        {
            UnlockStmt (pstmt);
            return rc;
        }
    }
    pstmt->irowParm = 0;    /* initialize current parameter row number */
                            /* of a parameter array */


    pstmt->fStatus |= STMT_DIRECT;

    if (isDriverReadOnly(pdbc))
       if (!StatementAllowedForReadOnly(pstmt))
          { rc = ErrState (SQL_HY000, pstmt, F_OD0151_IDS_ERR_NOT_READ_ONLY);
            UnlockStmt (pstmt);
            return rc;
          }

    /*
    **  Initialize the SQL control block anchor
    */
    pdbc->sqb.pSqlca = &pstmt->sqlca;
    pdbc->sqb.pStmt = pstmt;

    rc = SetTransaction (pdbc, pstmt->psess);
    if (rc != SQL_SUCCESS)
    {
        UnlockStmt (pstmt);
        return rc;
    }


    /*
    **  If a SELECT, call ExecuteSelectStmt to OPEN the cursor, then we're 
    **  done here (note that the syntax is not freed until SQLFreeStmt 
    **  (SQL_CLOSE) in case it needs to be prepared again for a change
    **  in the length of some character type parameter).
    */
    if (pstmt->fCommand == CMD_SELECT)
    {
        rc = ExecuteSelectStmt (pstmt);
        UnlockStmt (pstmt);
        return rc;
    }
    /*
    **  If the query has parameters, call ExecuteStmt to execute
    **  the command.
    */
    else if ((pstmt->icolParmLast > 0) ||
        (pstmt->fCommand & CMD_EXECPROC) || 
        (pstmt->fStatus  & STMT_WHERE_CURRENT_OF_CURSOR) )
    {
        rc = ExecuteStmt (pstmt);
        /*
        ** Don't free the query string if re-invoked from SQLExecute()
        ** as direct during simulated autocommit.  That will be done
        ** later.
        */
	if (!(pstmt->fStatus & STMT_SQLEXECUTE))
            FreeSqlStr (pstmt);
        UnlockStmt (pstmt);
        return rc;
    }
    /*
    **  If a COMMIT, ROLLBACK, or SET AUTOCOMMIT ON/OFF 
    **  don't execute it directly
    **  since OpenAPI doesn't like it undermining its 
    **  transaction management:
    */
    else if ((pstmt->fCommand & CMD_AUTOCOMMIT_OFF) ||
             (pstmt->fCommand & CMD_AUTOCOMMIT_ON)  ||
             (pstmt->fCommand & CMD_COMMIT)         ||
             (pstmt->fCommand & CMD_ROLLBACK)   )
    {
        FreeSqlStr (pstmt);      /* free up everything and call */
        UnlockStmt (pstmt);      /* the connect option instead  */
        if      ((pstmt->fCommand & CMD_AUTOCOMMIT_OFF) ||
                 (pstmt->fCommand & CMD_AUTOCOMMIT_ON))
           {rc = SQLSetConnectOption((SQLHDBC)pdbc, SQL_AUTOCOMMIT,
                  pstmt->fCommand & CMD_AUTOCOMMIT_ON ? TRUE : FALSE);
           }
        else   /* COMMIT or ROLLBACK */ 
           {rc = SQLTransact_InternalCall(SQL_NULL_HENV, pdbc, 
                  (UWORD)(pstmt->fCommand & CMD_COMMIT ? SQL_COMMIT : SQL_ROLLBACK));
           }
        return rc;
    }

    fCommit = SetAutoCommit (pstmt);
    pdbc->sqb.pSqlca = &pstmt->sqlca;
    pdbc->sqb.pStmt = pstmt;

    /* 
    ** if the external (user's point of view) autocommit=on AND
    ** current statement is commitable (e.g. INSERT, DELETE, etc.) AND
    ** autocommit is not blocked by an open API cursor
    ** THEN turn API autcommit back on if needed.  This will avoid
    ** issuing individual API commits simulating autocommit if the
    ** user closed his API cursors and is issuing INSERT, INSERT, etc. 
    */
    if (fCommit)
       odbc_AutoCommit(&pdbc->sqb, TRUE);

    pstmt->sqlca.SQLNRP = 0;

    SqlExecImm (&pdbc->sqb, pstmt->szSqlStr);

    if (pstmt->fCommand & CMD_START)  
        pstmt->psess->fStatus |= SESS_STARTED;  /* mark transaction started */

    rc = SQLCA_ERROR (pstmt, pstmt->psess);
    if (rc == SQL_ERROR)
    {
        ErrSetToken (pstmt, pstmt->szSqlStr, pstmt->cbSqlStr);
        /*
        **  If deadlock error, issue a rollback statement to burn up the
        **  leftover transaction handle in the API.  This is effectively
        **  a no-op statement as far as the dbms is concerned, because the
        **  transaction is already done. 
        */
        if (STequal(pdbc->sqb.pSqlca->SQLSTATE, SQST_SERIALIZATION_FAILURE))
            SQLTransact_InternalCall(SQL_NULL_HENV, pdbc, SQL_ROLLBACK);
    }
    else
    {
        SetAutoResult (pstmt, fCommit);
    }

    if (!(pstmt->fStatus & STMT_SQLEXECUTE))
        FreeSqlStr (pstmt);
    UnlockStmt (pstmt);
    return rc;
}

/*
**  SQLExecDirect_InternalCall
**
**  Lock the DBC without locking the ENV and then call the common code.
**
**  On entry: pstmt -->statement block.
**
**  On exit:  If a SELECT the cursor is open,
**            otherwise the command has been executed.
**
**            The syntax passed is freed for INTERNAL calls.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**            SQL_NEED_DATA
*/

RETCODE SQL_API SQLExecDirect_InternalCall(
    LPSTMT      pstmt,
    UCHAR     * szSqlStr,
    SQLINTEGER      cbSqlStr)
{
    if (!LockDbc_Trusted (pstmt->pdbcOwner)) 
       return SQL_INVALID_HANDLE;

    return(SQLExecDirect_Common(pstmt, (CHAR*)szSqlStr, cbSqlStr));
}


/*
**  SQLParamData
**
**  Supply parameter data at execution time.
**
**  Retrieve rgbValue for next SQL_DATA_AT_EXEC parm.
**  Complete execution of command when all parms passed.
**  Used in conjuction with SQLPutData.
**
**  On entry: pstmt    -->statement block.
**            prgbValue-->where to return rgbValue passed on SQLSetParam.
**
**            pstmt->icolPut = 0  no DATA_AT_EXEC started yet
**                           > 0  is current COL index relative to 1
**
**  Returns:  SQL_SUCCESS       Statement executed successfully.
**            SQL_NEED_DATA     Additional SQL_DATA_AT_EXEC parms remain.
**            SQL_ERROR         SQLParamData or SQLExecute error.
*/

RETCODE SQL_API SQLParamData(
    SQLHSTMT  hstmt,
    SQLPOINTER        *prgbValueParameter)
{
    LPSTMT    pstmt     = (LPSTMT)hstmt;
    PTR     * prgbValue = (PTR *)prgbValueParameter;
    RETCODE   rc = SQL_NEED_DATA;           /*  why else would we be here? */
    UWORD     icol;                         /* current column index */
    PTR       rgbValue;
    SQLINTEGER     * pcbValue = NULL;
    LPDESCFLD   papd;                       /* APD parm item */
    LPDESCFLD   pipd;                       /* IPD parm item */
    UDWORD      OctetLength;
    SQLSMALLINT fSqlType;
    SQLSMALLINT fCType;
    LPDBC  pdbc = pstmt->pdbcOwner;

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLPARAMDATA, pstmt, prgbValue);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    icol = pstmt->icolPut;
    papd =  pstmt->pAPD->pcol + 1;    /* papd->APD parm item after bookmark*/
    pipd =  pstmt->pIPD->pcol + 1;    /* pipd->IPD parm item after bookmark*/

    /*
    **  Find current DATA_AT_EXEC parm, if any.
    **
    **  1. If SQLPutData was not called for it, punt.
    **  2. If the caller did not fill a "long" field exactly, likewise.
    **  3. Mark the parameter as passed;
    **  4. Position on next parm.
    */
    if (icol)
    {
        papd =  pstmt->pAPD->pcol + icol;    /* papd->APD parm item */
        pipd =  pstmt->pIPD->pcol + icol;    /* pipd->IPD parm item */

        if (!(papd->fStatus & COL_PUTDATA))
        {
            return ErrUnlockStmt (SQL_HY010, pstmt); /* does DM do this? */
                                  /* function sequence error */
        }

        if (!(papd->fStatus & COL_PASSED))          /* not fixed length */
        {
            fCType   = papd->ConciseType;           /* source type */
            fSqlType = pipd->ConciseType;           /* target type */

            if (fSqlType == SQL_LONGVARCHAR   ||   /* if blob */
                fSqlType == SQL_LONGVARBINARY ||
                fSqlType == SQL_WLONGVARCHAR)
            {
                if (pstmt->fCommand != CMD_EXECPROC)  /* and not execute procedure */
                {
                    OctetLength = (UDWORD)pipd->OctetLength;
                    /* 
                    ** Adjust expected length for char to binary
                    ** or wchar to binary conversion.
                    */
                    if (fSqlType == SQL_LONGVARBINARY)
                    {
                        if (fCType == SQL_C_CHAR)       
                            OctetLength /= 2;           
                        else if (fCType == SQL_C_WCHAR) 
                            OctetLength /= sizeof(SQLWCHAR); 
                    }
                    /*
                    ** Adjust expected length for wchar to
                    ** char conversion.
                    */
                    if (fSqlType == SQL_LONGVARCHAR)
                    {
                        if (fCType == SQL_C_WCHAR) 
                            OctetLength /= sizeof(SQLWCHAR); 
                    }
                    /*  
                    ** Error - we're expecting more data.
                    */
                    if (papd->cbPartOffset < OctetLength)
                        return ErrUnlockStmt (SQL_22026, pstmt);
                }   /* endif pstmt->fCommand != CMD_EXECPROC */
            }  /* end if LONGVARCHAR or LONGVARBINARY or SQL_WLONGVARCHAR */
            papd->fStatus |= COL_PASSED;
        }   /* end if !(papd->fStatus & COL_PASSED) */
        papd++;
        pipd++;
    }
    icol++;

    /*
    **  Find next DATA_AT_EXEC parm, if any,
    **  set as current, and return its rgbValue:
    */
    while (rc == SQL_NEED_DATA)
    {
        while (icol <= pstmt->icolParmLast) /* icol <= #parameter markers */
        {
            GetBoundDataPtrs(pstmt, Type_APD, pstmt->pAPD, papd,
                             &rgbValue,  /* usually from papd->DataPtr */
                             &pcbValue,  /* usually from papd->OctetLengthPtr */
                             NULL);
               /*  Get DataPtr, OctetLengthPtr, and IndicatorPtr
                   after adjustment for bind offset, row number,
                   and row or column binding. */

            if (pcbValue &&
                (*pcbValue == SQL_DATA_AT_EXEC ||
                 *pcbValue <= SQL_LEN_DATA_AT_EXEC_OFFSET))
            {
                pstmt->icolPut = icol;
                if (prgbValue)
                   *prgbValue = rgbValue;
                UnlockStmt (pstmt);
                return SQL_NEED_DATA;
            }
            papd++;
            pipd++;
            icol++;
        }

        /*
        **  When no more DATA_AT_EXEC parms execute command:
        */
        pdbc->sqb.pSqlca = &pstmt->sqlca;
        pdbc->sqb.pStmt = pstmt;

        pstmt->icolPut = icol - 1;
        rc = SqlPutSegment (&pdbc->sqb);
        pstmt->fStatus &= ~STMT_NEED_DATA;
        pstmt->icolPut = 0;
    }
    UnlockStmt (pstmt);
    return rc;
}


/*
**  SQLPutData
**
**  Supply parameter data at execution time.
**
**  Used in conjuction with SQLParamData.
**
**  On entry: pstmt   -->statement block.
**            rgbValue-->parameter value.
**            cbValue  = length in bytes of parameter value.
**
**  Returns:  SQL_SUCCESS Parm value set.
**            SQL_ERROR   Some error setting parm value.
*/

RETCODE SQL_API SQLPutData(
    SQLHSTMT    hstmt,
    SQLPOINTER  DataPtr,
    SQLLEN      cbValue)
{
    LPSTMT  pstmt    = (LPSTMT)hstmt;
    char *rgbValue = DataPtr;
    RETCODE rc;
    UWORD   icol;                           /* current column index */
    LPDESC    pAPD;                         /* appl parm descriptor */
    LPDESC    pIPD;                         /* impl parm descriptor */
    LPDESCFLD papd;                         /* APD parm item */
    LPDESCFLD pipd;                         /* IPD parm item */
    SQLSMALLINT fSqlType;
    SQLSMALLINT fCType;
    LPDBC pdbc = pstmt->pdbcOwner;
    SQLINTEGER targetLen = 0,cbParm = 0;
    SQLINTEGER *pcbValue = NULL;
    static SQLINTEGER prevLen = 0;
    SQLINTEGER OctetLength;
    
    if (!LockStmt (pstmt)) 
        return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLPUTDATA, pstmt, rgbValue, cbValue);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    icol = (UWORD)(pstmt->icolPut);
    pAPD = pstmt->pAPD;          /* appl parm descriptor */
    pIPD = pstmt->pIPD;          /* impl parm descriptor */

    /*
    **  Find current DATA_AT_EXEC parm and corresponding SQLVAR:
    **
    **  We also need to check for some errors:
    **
    **  1. Not called twice for a fixed length parameter.
    **  2. Length passed by caller valid.
    */
    papd    = pAPD->pcol + icol;  /* papd-> current APD parm item */
    pipd    = pIPD->pcol + icol;  /* pipd-> current IPD parm item */

    if (papd->fStatus & COL_PASSED)
        return ErrUnlockStmt (SQL_HY019, pstmt);

    /* called more than once for non-char and non-binary data */

    if (rgbValue && 
        cbValue < 0 && 
        cbValue != SQL_NTS && 
        cbValue != SQL_NULL_DATA)
           return ErrUnlockStmt (SQL_HY090, pstmt);
        /* invalid string or buffer length */

    if ((!pipd->SegmentLength) || (prevLen != cbValue))
    {
        if (pipd->DataPtr && (pipd->fStatus & COL_DATAPTR_ALLOCED))
        {
            MEfree(pipd->DataPtr);  /* free prior parm buffer */
                pipd->DataPtr  = NULL;
            pipd->fStatus &= ~COL_DATAPTR_ALLOCED;
        } 
        pipd->SegmentLength = SegmentLength(pstmt, papd, pipd, 
            rgbValue, cbValue);
        GetBoundDataPtrs(pstmt, Type_APD, pstmt->pAPD, papd,
            NULL, &pcbValue, NULL);

        if (pcbValue && *pcbValue == SQL_DATA_AT_EXEC)
        {
            pipd->OctetLength = pipd->SegmentLength;
        }

        pipd->DataPtr = MEreqmem(0, (SIZE_TYPE)(pipd->SegmentLength + 6),
             TRUE, NULL);
        if (!pipd->DataPtr)
        {
            UnlockStmt(pstmt);
            return (ErrState (SQL_HY001, pstmt, F_OD0015_IDS_WORKAREA));
        }
    }

    prevLen = cbValue;

    pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr allocated */

    /*
    **  Pass parm data:
    */
    papd->fStatus |= COL_PUTDATA;  /* indicate we've been called */
    rc = PutParm (pstmt, papd, pipd, 
                  /* DataPtr        = */  rgbValue,
                  /* OctetLengthPtr = */  &cbValue,
                  /* IndicatorPtr   = */  &cbValue,
                                          FALSE);
    if (rc != SQL_SUCCESS)
    {
        UnlockStmt(pstmt);
        return rc;
    }

    /*
    **  Mark parameter as done if it is fixed length.
    */
    if (papd->fStatus & COL_FIXED)
        papd->fStatus |= COL_PASSED;

    pdbc->sqb.pSqlca = &pstmt->sqlca;
    pdbc->sqb.pStmt = pstmt;
    
    fSqlType = pipd->ConciseType;           /* target type */
    fCType = papd->ConciseType;             /* source type */
    OctetLength = pipd->OctetLength;
    
    switch (fSqlType)
    {
    case SQL_LONGVARCHAR:  
    case SQL_LONGVARBINARY:
    case SQL_CHAR:
    case SQL_VARCHAR:
        if (fCType == SQL_C_WCHAR)
            OctetLength /= sizeof(SQLWCHAR);
    /*
    ** Fall-through to Wide cases.
    */
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
        if ((papd->cbPartOffset) < (SQLUINTEGER)OctetLength)
            pstmt->moreData = TRUE;
        else
            pstmt->moreData = FALSE;
        break;
    default:
        break;
    }
    rc = SqlPutSegment (&pdbc->sqb);
    if (!pstmt->moreData) 
    {
        MEfree(pipd->DataPtr);  
        pipd->DataPtr = NULL;
        pipd->fStatus &= ~COL_DATAPTR_ALLOCED;
        pipd->SegmentLength = 0;
        prevLen = 0;
    }

    UnlockStmt (pstmt);
    return rc;
}


/*
**  ExecErrCommit
**
**  Execute AutoCommit after an error processing an array of parameters.
**
**  On entry: pstmt-->statement block
**
**  On exit:  If transacation is still active and statement updates the
**            data base transaction is committed, if possible, or it is
**            flagged as needing a commit.
**
**  Returns:  nothing
*/
static VOID      FASTCALL ExecErrCommit (
    LPSTMT  pstmt)
{
    RETCODE rc      = SQL_SUCCESS;
    LPDBC   pdbc    = pstmt->pdbcOwner;
    UWORD   fCommit = 0;

    TRACE_INTL (pstmt, "ExecErrCommit");
    if (pstmt->sqlca.SQLCODE > -5)  /* if we still have a transaction... */
    {
        fCommit = SetAutoCommit (pstmt);
        SetAutoResult (pstmt, fCommit);
        /*
        if (fCommit)
        {
            pdbc->sqb.pSqlca  = &pdbc->sqlca;   ** use SQLCA for connection **
            pdbc->sqb.Options = SQB_SUSPEND;    ** offs piggybacked commit **
            pdbc->sqb.pStmt = pstmt;

            if (fCommit == SQB_COMMIT)
                SqlCommit (&pdbc->sqb);
            else
                SqlCommitC (&pdbc->sqb);

            rc = SQLCA_ERROR (pdbc, pstmt->psess);
        }
        if (rc == SQL_SUCCESS)
            SetAutoResult (pstmt, fCommit);
        */
    }
    return;
}

/*
**  ExecuteStmt
**
**  Execute a prepared SQL statement
**
**  On entry: pstmt -->statement block.
**
**  On exit:  If a SELECT the cursor is open,
**            otherwise the command has been executed.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
**            SQL_NEED_DATA
**
**  ExecuteStmt() operates in three modes:
**
**    1.  For database procedures or queries without dynamic parameters, 
**        ExecuteStmt() executes and returns.
**    2.  For queries that have dynamic (SQL_AT_EXEC) parameters, ExecuteStmt()
**        returns SQL_NEED_DATA after executing the query.
**    3.  For insert, delete or modify queries that have more than one row
**        of parameters to send, queries are executed in a loop until all 
**        of the rows have been sent.
**
**  History
**
**  01/17/1997  Jean Teng   don't call PrepareParms if stmt is select and 
**	sql_data_at_exec in process.
**  17-nov-03 (loera01)
**      Overhauled to allow for sending true segments from SQLPutData().
**      Statements are now prepared in SQLPrepare().  Parameter data is
**      characterized from odbc_getDescriptor() and/or SQLPutData().
*/
RETCODE ExecuteStmt( LPSTMT  pstmt )
{
    RETCODE rc        = SQL_SUCCESS;
    RETCODE rc2;
    LPDBC   pdbc      = pstmt->pdbcOwner;
    BOOL    fNeedData = FALSE;
    UWORD   fCommit   = 0;
    LPDESC    pAPD     = pstmt->pAPD;       /* appl parm descriptor */
    LPDESC    pIPD     = pstmt->pIPD;       /* impl parm descriptor */
    LPDESCFLD papd;                         /* APD parm item */
    LPDESCFLD pipd;                         /* IPD parm item */
    SQLUINTEGER i;

    TRACE_INTL (pstmt, "ExecuteStmt");

    pstmt->fStatus |= STMT_EXECUTE;

    if (isDriverReadOnly(pdbc))
       if (!StatementAllowedForReadOnly(pstmt))
       { 
           rc = ErrState (SQL_HY000, pstmt, 
               F_OD0151_IDS_ERR_NOT_READ_ONLY);
           return rc;
       }

    /*
    **  Main loop through multiple sets of parameter values:
    */
    if (pAPD->ArraySize > 0)
    {
        pstmt->crowParm = pAPD->ArraySize;
        if (pIPD->ArrayStatusPtr)
        {
            for (i = 0; i < pAPD->ArraySize; i++)
                pIPD->ArrayStatusPtr[i] = SQL_PARAM_UNUSED;
        }
    }
    else if (pstmt->crowParm == 0)   /* if no parm row count */
        pstmt->crowParm = 1;    /* default to count=1 */

    pstmt->sqlca.SQLNRP = 0;
    while (pstmt->irowParm < pstmt->crowParm && rc != SQL_ERROR)
    {   /*  loop through parameter sets as long as no error */

        papd = pAPD->pcol + 1;
        pipd = pIPD->pcol + 1;

    /*
    **  If multiple sets of parameter values, return current index:
    **
    **  icolParmLast = count of parameter markers.
    **  pirowParm is set by SQLParamOption().
    **  Report the current parameter row number of a
    **  parameter array.
    */
    if (pstmt->pIPD->RowsProcessedPtr)
        pstmt->pirowParm = pstmt->pIPD->RowsProcessedPtr;

    if (pstmt->pirowParm && pstmt->icolParmLast)
        *pstmt->pirowParm = pstmt->irowParm + 1;
    /* 
    ** Forgive prepare/execute of commit or rollback.
    */
    else if (pstmt->fCommand == CMD_COMMIT  || 
        pstmt->fCommand == CMD_ROLLBACK) 
    {
        rc = SQLTransact_InternalCall(SQL_NULL_HENV, pdbc, 
           (UWORD)(pstmt->fCommand & CMD_COMMIT ? 
           SQL_COMMIT : SQL_ROLLBACK));
           return rc;
    }
    else 
    /* 
    ** pstmt->fCommand == CMD_INSERT or _UPDATE or _DELETE etc.
    */
    {
        fCommit = SetAutoCommit (pstmt);

        pdbc->sqb.pSqlca = &pstmt->sqlca;
	pdbc->sqb.pStmt = pstmt;

        /* 
        ** If the external autocommit=on AND current statement 
        ** is commitable (e.g. INSERT, DELETE, etc.) AND
        ** autocommit is not blocked by an open API cursor,
        ** turn API autcommit back on if needed.  This will 
        ** avoid issuing individual API commits simulating 
        ** autocommit if the user closed his API cursors and is 
        ** issuing INSERT, INSERT, etc. 
        */
        if (fCommit)
            odbc_AutoCommit(&pdbc->sqb, TRUE);

    } /* end for */

        if (rc == SQL_ERROR)
            return rc;

        /*
        **  Use SQLCA in STMT to EXECUTE or OPEN:
        */
            /*
            **  If command is anything else:
            **
            **  1. EXECUTE with parm buffer (if any).
            **  2. Save rows processed from SQLCA in array.
            **  3. Increment parm value set counter.
            **
            **  If insert multiple rows and have autocommit on, 
            **  DBMS will commit after each insert. Therefore, we have 
            **  to perform necessary processing here. Otherwise, we 
            **  would get Invalid Prepared Stmt Name message. 
    	    */
            pdbc->sqb.pSqlca = &pstmt->sqlca;
    	        pdbc->sqb.pStmt = pstmt;
    
            if (!(pstmt->fStatus & STMT_DIRECT) &&  /* not direct */
                pstmt->fStatus & STMT_CANBEPREPARED) /* preparable */
                rc = SqlExecute (&pdbc->sqb); 
            else if (pstmt->fStatus & STMT_DIRECT)
                rc = ExecuteDirect(pstmt); 
            else if (pstmt->fCommand == CMD_EXECPROC)
                rc = SqlExecProc (&pdbc->sqb); 
            if (rc == SQL_NEED_DATA)
                return SQL_NEED_DATA;
    
            if (pstmt->fCvtError & CVT_FRAC_TRUNC)
                pstmt->sqlca.SQLCODE = SQL_SUCCESS_WITH_INFO;

            rc = SQLCA_ERROR (pstmt, pstmt->psess);
            if (rc == SQL_ERROR)
            {
                if (pstmt->irowParm > 0)
                    ExecErrCommit (pstmt);
            }
            else
            {
                if (pstmt->fCommand & CMD_EXECPROC)
                {
                    if (pstmt->fStatus & STMT_RETPROC)
                    {
                        rc2 = GetProcedureReturn(pstmt);

                        if ((rc2 == SQL_ERROR) ||
                            (rc2 == SQL_SUCCESS_WITH_INFO && rc 
                            != SQL_ERROR))
                            rc = rc2;
                    }
                    /* 
                    ** If the procedure did a commit or rollback, 
                    ** no commit is needed.
                    */
                    if (pdbc->sessMain.tranHandle == NULL)
                        pstmt->fCommand &= ~CMD_COMMITABLE;
                    /*
                    ** If this is a row-procucing procedure,
                    ** return now and don't commit.
                    */
                    if (pstmt->fStatus & STMT_OPEN)
                        return rc;    
                }
                SetAutoResult (pstmt, fCommit);
            }

        /*
        ** Queries with result sets cannot have parameter
        ** sets (arrays).  Always return in this case.
        */
        if ((pstmt->fStatus & STMT_OPEN) && (pstmt->fStatus &
            CMD_EXECPROC))
            return rc;

        if (pIPD->ArrayStatusPtr && pAPD->ArraySize > 0)
        {
            switch (rc)
            {
                case SQL_SUCCESS: 
                    pIPD->ArrayStatusPtr[pstmt->irowParm] = 
                        SQL_PARAM_SUCCESS;
                    break;
                case SQL_ERROR: 
                    pIPD->ArrayStatusPtr[pstmt->irowParm] = 
                        SQL_PARAM_ERROR;
                    break;
                case SQL_SUCCESS_WITH_INFO: 
                    pIPD->ArrayStatusPtr[pstmt->irowParm] = 
                        SQL_PARAM_SUCCESS_WITH_INFO;
                    break;
                default:
                    pIPD->ArrayStatusPtr[pstmt->irowParm] = 
                        SQL_PARAM_DIAG_UNAVAILABLE;
                    break;
            }
        }

        if (rc == SQL_ERROR)
            return rc;

        pstmt->irowParm++;   /* increment row number */

    }   /* end loop through parameter sets */

    return rc;
}

/*
**  ExecuteSelectStmt
**
**  Execute a select statement.
**
**  On entry: pstmt-->statement block
**
**  On exit:  Implementation Row Descriptor array is updated.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
**
**  History:
**
**  17-nov-03 (loera01)
**     Extracted from ExecuteStmt().
**
*/

RETCODE ExecuteSelectStmt( LPSTMT  pstmt )
{
    RETCODE rc        = SQL_SUCCESS;
    LPDBC   pdbc      = pstmt->pdbcOwner;
    BOOL    fNeedData = FALSE;
    UWORD   fCommit   = 0;

    TRACE_INTL (pstmt, "ExecuteSelectStmt");

    pdbc->sqb.pSqlca = &pstmt->sqlca;
    pdbc->sqb.pStmt = pstmt;

    pstmt->sqlca.SQLNRP = 0;

    pstmt->fStatus |= STMT_EXECUTE;

    if (pstmt->fStatus & STMT_DIRECT)
    {
        pdbc->sqb.Options = 0; 

        rc = ExecuteDirect(pstmt);
        if (rc != SQL_ERROR)
            pstmt->fStatus |= STMT_OPEN;
        return rc;
    }
            
    pdbc->sqb.Options = SQB_OPEN; 

    /*
    **  If OPEN was not piggybacked onto the PREPARE,
    **  OPEN the cursor and flag STMT that cursor is open.
    **  If cursor was opened successfully and at doing first
    **  set of parms (the normal case), generate cursor name.
    */
    if (!(pstmt->fStatus & STMT_OPEN))
    {
        pdbc->sqb.pSqlca = &pstmt->sqlca;
        pdbc->sqb.pStmt = pstmt;
        rc = SqlOpen (&pdbc->sqb);
    
        if (rc != SQL_ERROR)
        {
            pstmt->fStatus |= STMT_OPEN; /* result set */
            pstmt->psess->fStatus &= ~SESS_SUSPENDED;
        }
    }
    return rc;
}

/*
**  ParmLength
**
**  Compute the length of a character or binary parameter in 
**  the parameter buffer.
**
**  We try the following to get the parameter length:
**
**  1. The result of the SQL_LEN_DATA_AT_EXEC macro, if appropriate.
**  2. The precision specified with SQLBindParameter.
**  3. The display size for any C type with an implicit size.
**  4. The actual C parameter length if available.
**  5. Take a guess and stuff in a default length.
**
**  Note that this can cause us to reprepare the command later on
**  if the actual length of the parameter in its C format changes.
**
**  On entry: pstmt  -->statement block.
**            papd   -->application parameter descriptor.
**            pipd   -->implementation parameter descriptor.
**
**  Returns:  Parameter length.
*/

SQLINTEGER ParmLength(
    LPSTMT   pstmt,
    LPDESCFLD papd,
    LPDESCFLD pipd)
{
    SQLINTEGER     * pcbValue = NULL;    /* ptr to total length in bytes */
    SQLINTEGER     * IndicatorPtr;/* ptr to indicator             */
    CHAR *       rgbValue;    /* ptrto bound parameter        */
    SWORD        fSqlType;    /* target data type             */
    SWORD        fCType;      /* source data type             */
    SQLINTEGER       cbValueMax;  /* length in bytes of char or bin data */
    SQLUINTEGER      cbColDef;    /* actual len of var data; max len of others */
    LPSQLTYPEATTR   ptype;
    SQLINTEGER       cb;

    TRACE_INTL (pstmt, "ParmLength");
                                       /* SQLBindParameter(...) */
    fSqlType   = pipd->ConciseType;    /*    fSqlType  (ParameterType)    */
    fCType     = papd->ConciseType;    /*    fCType    (ValueType)        */
    cbValueMax = papd->OctetLength;    /*    cbValueMax(BufferLength)     */
    cbColDef   = pipd->Length;         /*    cbColDef  (ColumnSize)       */

    /*  
    ** Get DataPtr, OctetLengthPtr, and IndicatorPtr
    ** after adjustment for bind offset, row number,
    ** and row or column binding. 
    */
    GetBoundDataPtrs(pstmt, Type_APD, pstmt->pAPD, papd,
                     &rgbValue,
                     &pcbValue,
                     &IndicatorPtr);

    switch (fSqlType)
    {
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:    
        if (pstmt->fStatus & STMT_EXECUTE && pcbValue)
	{
	    if (!(papd->fStatus & COL_ODBC10) && 
                *pcbValue <= SQL_LEN_DATA_AT_EXEC_OFFSET)
            {   /* 
                ** Get length from SQL_LEN_DATA_AT_EXEC(length)  
                */
                cb = -(*pcbValue - SQL_LEN_DATA_AT_EXEC_OFFSET);
                if (cb > 0  &&  cb < (MAXI4-1000))
                    return (fCType == SQL_C_CHAR  && 
                    fSqlType == SQL_WLONGVARCHAR) ? 
                    cb * sizeof(SQLWCHAR) : cb;
            }
            if (*pcbValue >= 0)  /* return dynamic length */
                return(*pcbValue);
        }
        break;

    default:
        break;
    }  
    ptype = CvtGetSqlType (fSqlType, pstmt->fAPILevel, pstmt->dateConnLevel);
    cb = ptype->cbLength;
    if (cb) return cb;

    /*
    **  Return length for decimal and string type column:
    */
    cb = cbColDef;        /* ColumnSize (chars) or precision
                             from SQLBindParameter */

    if (cb >= MAXI4/2) /* never return a parm length of 2147483647 */
        cb = 0;        /* or try to allocate more than a billion bytes */

    if ((fSqlType == SQL_DECIMAL || 
         fSqlType == SQL_NUMERIC)  &&  pipd->Precision)
        return ((pipd->Precision/2) + 1);
              /* if decimal/numeric, return (precision/2 + 1) */

    if (fSqlType == SQL_WCHAR       || /* if wide (Unicode) char */
        fSqlType == SQL_WVARCHAR    ||
        fSqlType == SQL_WLONGVARCHAR)
        cb = cb * sizeof(SQLWCHAR);
                        /* return ColumnSize length as number of bytes */

    if (cb) return cb;

    if (!(pstmt->fStatus & STMT_EXECUTE)) return 0;

    if (pcbValue == NULL || *pcbValue == SQL_NTS)
    {
        if (rgbValue)
        {
            if (fCType == SQL_C_WCHAR)     /* if source is wide (Unicode) */
                cb = (SDWORD)wcslen ((SQLWCHAR*)rgbValue);    /* length in char */
            else
                cb = (SDWORD)STlength (rgbValue);

            if (fSqlType == SQL_WCHAR       || /* if target is */
                fSqlType == SQL_WVARCHAR    || /* wide (Unicode) char */
                fSqlType == SQL_WLONGVARCHAR)
                cb = cb * sizeof(SQLWCHAR);
                                /* return length as number of bytes */
        }
        else  /* rgbValue == NULL */
        {
            cb = 0;
        }
    }
    else  /* pcbValue != NULL && *pcbValue != SQL_NTS */
    {
        cb = (*pcbValue > 0) ? *pcbValue : 0;
    }
    if (cb > 30000) cb = 30000;

    return cb;
}

/*
**  SegmentLength
**
**  Compute the length of a character or binary parameter in 
**  the parameter buffer.  If the data type has a standard length, use that.
**  Otherwise get the length provided from SQLPutData() and adjust for
**  data types as required.
**
**  On entry: pstmt  -->statement block.
**            papd   -->application parameter descriptor.
**            pipd   -->implementation parameter descriptor.
**
**  Returns:  Segment length.
**
**  History:
**     13-nov-03 (loera01)
**         Created.
*/

SQLINTEGER SegmentLength(
    LPSTMT   pstmt,
    LPDESCFLD papd,
    LPDESCFLD pipd,
    SQLPOINTER DataPtr, 
    SQLINTEGER    cbValue)
{
    CHAR *       rgbValue;    /* ptrto bound parameter        */
    SWORD        fSqlType;    /* target data type             */
    SWORD        fCType;      /* source data type             */
    SQLINTEGER       cbValueMax;  /* length in bytes of char or bin data */
    SQLUINTEGER      cbColDef;    /* actual len of var data; max len of others */
    LPSQLTYPEATTR   ptype;
    SQLINTEGER       cb;


    TRACE_INTL (pstmt, "SegmentLength");
                                       /* SQLBindParameter(...) */
    fSqlType   = pipd->ConciseType;    /*    fSqlType  (ParameterType)    */
    fCType     = papd->ConciseType;    /*    fCType    (ValueType)        */
    cbValueMax = papd->OctetLength;    /*    cbValueMax(BufferLength)     */
    cbColDef   = pipd->Length;         /*    cbColDef  (ColumnSize)       */
    rgbValue = DataPtr;

    ptype = CvtGetSqlType (fSqlType, pstmt->fAPILevel, pstmt->dateConnLevel);
    cb = ptype->cbLength;
    if (ptype->fVerboseType == SQL_DATETIME || 
        ptype->fVerboseType == SQL_INTERVAL)
        cb = pipd->OctetLength;

    if (cb) return cb;

    /*
    **  Return length for decimal and string type column:
    */
    if (cbValue == SQL_NTS)
    {
        if (rgbValue)
        {
            if (fCType == SQL_C_WCHAR)     /* if source is wide (Unicode) */
                cb = (SDWORD)wcslen ((SQLWCHAR*)rgbValue); /* len in chars */
            else
                cb = (SDWORD)STlength (rgbValue);

            if (fSqlType == SQL_WCHAR       || /* if target is */
                fSqlType == SQL_WVARCHAR    || /* wide (Unicode) char */
                fSqlType == SQL_WLONGVARCHAR)
                cb = cb * sizeof(SQLWCHAR);
            /* return length as number of bytes */
            return cb;
        }
    }

    cb = cbValue; 

    if (cb >= MAXI4/2) /* never return a parm length of 2147483647 */
        return (0);    /* or try to allocate more than a billion bytes */

    if ((fSqlType == SQL_DECIMAL || 
         fSqlType == SQL_NUMERIC)  &&  pipd->Precision)
        return ((pipd->Precision/2) + 1);
              /* if decimal/numeric, return (precision/2 + 1) */

    if (fSqlType == SQL_WCHAR       || /* if wide (Unicode) char */
        fSqlType == SQL_WVARCHAR    ||
        fSqlType == SQL_WLONGVARCHAR)
        cb = cb * sizeof(SQLWCHAR);
                        /* return ColumnSize length as number of bytes */

    return cb;
}


/*
**  PutCharBin
**
**  Set a character binary value as binary parm data:
**
**  On entry: pcol    -->parm column block.
**            rgbData -->parm in buffer.
**            rgbValue-->parm value.
**            cbData   = length of parm in buffer.
**            cbValue  = length of parm value.
**
**  On exit:  Data is converted from hex display and
**            copied to buffer, possibly truncated.
**            Bind column block is updated with length copied.
**
**  Returns:  CVT_SUCCESS           Converted successfully.
**            CVT_TRUNCATION        Data truncated.
**            CVT_NEED_DATA         Parm did not fill buffer.
**  Change History
**  --------------
**
**  date        programmer  description
**
**  02/10/1997  Jean Teng	set length if var data types
*/

static UDWORD PutCharBin(
    LPDESCFLD   papd,
    LPDESCFLD   pipd,
    CHAR     *  rgbData,
    CHAR     *  rgbValue,
    SQLUINTEGER     cbData,
    UDWORD      cbValue,
    BOOL        offsetData)
{
    if (offsetData)
    {
        rgbData += papd->cbPartOffset;
        cbData  -= papd->cbPartOffset;
    }
    cbValue /= 2;

    if (cbValue > cbData)
        return CVT_TRUNCATION;

    if (papd->cbPartOffset == 0)
        MEfill (cbData, 0, rgbData);

    if (offsetData)
        papd->cbPartOffset += (cbValue * 2);
    else
        papd->cbPartOffset = (cbValue * 2);

    if (pipd->ConciseType == SQL_VARBINARY)
    {
        *(UWORD *)rgbData = (UWORD)cbValue;
        rgbData += 2;
    }
    else
    if (pipd->ConciseType == SQL_LONGVARBINARY)
    {
        *(UDWORD *)rgbData = cbValue;
        rgbData += 4;
    }
    return CvtCharBin (rgbData, rgbValue, cbValue);

}


/*
**  PutCharChar
**
**  Set a character parm value from rgbValue to rgbData.
**
**  On entry: pcol    -->parm column block.
**            rgbData -->parm in target buffer.
**            rgbValue-->parm value in source buffer.
**            cbData   = length of parm in buffer.
**            cbValue  = length of parm value.
**            bPad     = Pad character.
**
**  On exit:  Value is copied to buffer, possibly truncated.
**            Bind column block is updated with length moved.
**
**  Returns:  CVT_SUCCESS           Copied successfully.
**            CVT_TRUNCATION        Data truncated.
**  Change History
**  --------------
**
**  date        programmer  description
**
**  02/07/1997  Jean Teng	added SQL_VARBINARY to fVarchar case
**  09/05/2000  Dave Thole  padding varchar is unnecessary
*/
static UDWORD PutCharChar(
    LPDESCFLD   papd,
    LPDESCFLD   pipd,
    CHAR      * rgbData,
    CHAR      * rgbValue,
    SQLUINTEGER     cbData,
    UDWORD      cbValue,
    CHAR        bPad,
    BOOL        offsetData)
{
    UWORD     * pcbData  = (UWORD *) rgbData;  /* -> 2 byte length prefix */
    UDWORD    * pcbDWData= (UDWORD *)rgbData;  /* -> 4 byte length prefix */
    SWORD       fSqlType  = pipd->ConciseType;
    BOOL        fBlob    = fSqlType == SQL_LONGVARCHAR   ||
                           fSqlType == SQL_LONGVARBINARY ||
                           fSqlType == SQL_WLONGVARCHAR;

    BOOL        fVarchar = fSqlType == SQL_VARCHAR     ||
                           fSqlType == SQL_VARBINARY   ||
                           fSqlType == SQL_WVARCHAR;

    BOOL        fUnicode = fSqlType == SQL_WCHAR       ||
                           fSqlType == SQL_WVARCHAR    ||
                           fSqlType == SQL_WLONGVARCHAR;
    UDWORD      i;

    if (fVarchar)        /* if VARCHAR or VARBINARY, skip 2-byte length prefix*/
        rgbData += 2;
    else if (fBlob)      /* if LONG CHAR or LONG BYTE, skip 4-byte length prefix*/
        rgbData += 4;

    if (offsetData)
    {
        rgbData += papd->cbPartOffset;
        cbData  -= papd->cbPartOffset;
    }
    if (cbValue > cbData)   /* if source length > target buffer length */
        return CVT_TRUNCATION;

    /*
    **  Pad with spaces or nulls.
    */
    if (papd->cbPartOffset == 0)
        {   SQLUINTEGER len=cbData-cbValue;
            if (len>0  &&  !fBlob  &&  !fVarchar)   /* pad if needed */
               if (fUnicode)
                   for (len /=sizeof(SQLWCHAR), i = 0; i < len; i++)
                      *((SQLWCHAR*)(rgbData+cbValue) + i) = bPad;
               else /* ANSI */
                   MEfill ((size_t)len, bPad, rgbData+cbValue);
        }

    papd->cbPartOffset += cbValue;

    if (offsetData)
	{
        if (fVarchar)
            *pcbData = (UWORD)(papd->cbPartOffset);  /* update length prefix */
        else if (fBlob)
            *pcbDWData = (UDWORD)(papd->cbPartOffset);  /* update length prefix */
    }
	else
    {
        if (fVarchar)
            *pcbData = (UWORD)cbValue;  /* update length prefix */
        else if (fBlob)
            *pcbDWData = (UDWORD)cbValue;  /* update length prefix */
    }

    MEcopy (rgbValue, (SIZE_TYPE)cbValue, rgbData);
    return CVT_SUCCESS;
}


/*
**  PutTinyInt
**
**  Put tiny value into the parameter buffer.
**
**  On entry: rgbData  -->where to set parm data.
**            tinyValue  = tiny value.
**            fSqlType  = SQL data type of parm.
**            fUnsigned = C data type is unsigned.
**
**  Returns:  CVT_SUCCESS           OK.
**            CVT_OUT_OF_RANGE      Won't fit.
**            CVT_INVALID           Wrong type.
*/
static UDWORD PutTinyInt(
    CHAR     *  rgbData,
    SCHAR       tinyValue,
    SWORD       fSqlType,
    BOOL        fUnsigned)
{
    switch (fSqlType)
    {
    case SQL_BIT:

        if (tinyValue != 0 && tinyValue != 1) return CVT_OUT_OF_RANGE;
        rgbData[0] = tinyValue;
        break;

    case SQL_TINYINT:

        *(SCHAR*)rgbData = tinyValue;
        break;

    default:

        return CVT_INVALID;
        break;

    }
    return CVT_SUCCESS;
}


/*
**  PutInt
**
**  Put integer value into the parameter buffer.
**
**  On entry: rgbData  -->where to set parm data.
**            intValue  = integer value.
**            fSqlType  = SQL data type of parm.
**            fUnsigned = C data type is unsigned.
**
**  Returns:  CVT_SUCCESS           OK.
**            CVT_OUT_OF_RANGE      Won't fit.
**            CVT_INVALID           Wrong type.
*/
static UDWORD PutInt(
    CHAR     *  rgbData,
    SDWORD      intValue,
    SWORD       fSqlType,
    BOOL        fUnsigned)
{
    switch (fSqlType)
    {
    case SQL_SMALLINT:

        if (fUnsigned)
        {
            if (intValue < 0 || intValue > 32767) return CVT_OUT_OF_RANGE;
        }
        else
        {
            if (intValue < -32768 || intValue > 32767) return CVT_OUT_OF_RANGE;
        }
        *(SWORD *)rgbData = (SWORD)intValue;
        break;

    case SQL_INTEGER:
	case SQL_BIGINT:

        if (fUnsigned && intValue < 0) return CVT_OUT_OF_RANGE;
        *(SDWORD *)rgbData = intValue;
        break;

    default:

        return CVT_INVALID;
        break;

    }
    return CVT_SUCCESS;
}

/*
**  PutInt64
**
**  Put 64-bit integer value into the parameter buffer.
**
**  On entry: rgbData  -->where to set parm data.
**            int64Value  = integer value.
**            fSqlType  = SQL data type of parm.
**            fUnsigned = C data type is unsigned.
**
**  Returns:  CVT_SUCCESS           OK.
**            CVT_OUT_OF_RANGE      Won't fit.
**            CVT_INVALID           Wrong type.
*/
static UDWORD PutInt64(
    CHAR     *  rgbData,
    ODBCINT64   int64Value,
    SWORD       fSqlType,
    BOOL        fUnsigned)
{

    if (fUnsigned && int64Value < 0) return CVT_OUT_OF_RANGE;
    *(ODBCINT64 *)rgbData = int64Value;
    return CVT_SUCCESS;
}


/*
**  PutNumChar
**
**  Set integer or floating point string as a character parm value.
**
**  On entry: pcol   -->parm column block.
**            rgbData-->parm in buffer.
**            szValue-->parm value.
**            cbData  = length of parm in buffer.
**
**  On exit:  Value is copied to buffer if it will fit.
**            Buffer is padded with spaces if needed.
**
**  Returns:  CVT_SUCCESS           Converted successfully.
**            CVT_OUT_OF_RANGE      Data truncated.
*/
static UDWORD PutNumChar(
    LPDESCFLD   papd,
    LPDESCFLD   pipd,
    CHAR     *  rgbData,
    CHAR     *  szValue,
    UWORD       cbData)
{
    CHAR     *  rgbCopy;
    UWORD       cbValue;
    SWORD       fSqlType = pipd->ConciseType;

    rgbCopy = rgbData;
    cbValue = (UWORD) STlength (szValue);
    if (cbValue > cbData)
        return CVT_OUT_OF_RANGE;

    if (fSqlType == SQL_VARCHAR  ||  fSqlType == SQL_WVARCHAR)
    {
        *(UWORD *)rgbData = cbValue;
        rgbCopy += 2;
    }
    else if (fSqlType == SQL_LONGVARCHAR  ||  fSqlType == SQL_WLONGVARCHAR)
    {
        *(UDWORD *)rgbData = cbValue;
        rgbCopy += 4;
    }

    MEfill (cbData, ' ', rgbCopy);
    MEcopy (szValue, cbValue, rgbCopy);

    return CVT_SUCCESS;
}


/*
**  PutParm
**
**  Get a parameter value from caller's storage, convert
**  it to SQL format, and stuff in parm buffer:
**
**  On entry: pstmt  -->statement block.
**            papd   -->application parameter descriptor.
**            pipd   -->implementation parameter descriptor.
**            rgbValue -->column value buffer.  (ParameterValuePtr)
**            pcbValue -->actual column value byte length (StrLen_or_IndPtr)
**                        or --> SQL_DEFAULT_PARAM
**                               SQL_DATA_AT_EXEC
**                               SQL_NTS
**            IndicatorPtr-->null indicator (SQL_NULL_DATA)
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_NEED_DATA (only if called by SQLExecute)
**            SQL_ERROR
*/

static RETCODE PutParm(
    LPSTMT   pstmt,
    LPDESCFLD papd,
    LPDESCFLD pipd,
    CHAR    * rgbValue,   /* -->column value buffer.  (ParameterValuePtr)*/
    SQLINTEGER  * pcbValue,   /* -->actual column value length (StrLen_or_IndPtr)
                          or --> SQL_DEFAULT_PARAM
                                 SQL_DATA_AT_EXEC
                                 SQL_NTS          */
    SQLINTEGER  * IndicatorPtr,  /* -->null indicator  (SQL_NULL_DATA) */
    BOOL    offsetData ) /* Whether to offset char data */
{
    UDWORD      rc = CVT_SUCCESS;   /* always look on the bright side of life */
    RETCODE     retcode;
    BOOL        fUnsigned = FALSE;
    BOOL        fSqlNts   = FALSE;
    SWORD       fCType    = papd->ConciseType; /* C data type */
    SWORD       fSqlType  = pipd->ConciseType; /* SQL data type */
    SQLSMALLINT fVerboseType = pipd->VerboseType; /* Verbose data type */
    UWORD       cbValue   = 0;       /* length of parm value */
    UDWORD      cbdwValue = 0;       /* length of parm value (UDWORD version)*/
    CHAR      * rgbData;             /* -->data in parm buffer */
    CHAR      * rgbNull;             /* -->null indicator in parm buffer */
    UWORD       cbData;              /* length of parm in buffer */
    UDWORD      cbdwData;            /* length of parm in buffer (UDWORD) */
    SDWORD      intValue;            /* conversion work buffers... */
    ODBCINT64   int64Value;
    UDWORD      uintValue;           /* conversion work buffers... */
    SCHAR       tinyValue;
    SDOUBLE     floatValue;
    DATE_STRUCT dateValue;
    ISO_TIME_STRUCT timeValue;
    TIMESTAMP_STRUCT stampValue;
    SQL_INTERVAL_STRUCT interval, *pInterval;
    CHAR       *rgbValueWk;
    LPSQLTYPEATTR ptype;
    CHAR       *pwork     = NULL;   /* work pointer for wide->char buffer*/
    CHAR        szValue[DB_MAX_DECPREC + 4];
    CHAR        szValueWChar[340]; /* intermediate result for
                                      SQL_C_WCHAR -> SQL_xxx conversions and
                                      SQL_C_CHAR -> SQL_Wxxx conversions */
    DECLTEMP (rgbTemp[LEN_TEMP]) /* Win32 only */
    CHAR      * lpChar;
    CHAR      * lpSave;
    BOOL      fractionalTrunc = FALSE;
    LPDBC     pdbc = pstmt->pdbcOwner;

    TRACE_INTL(pstmt,"PutParm");
    /*
    **  Check that we can convert convert C type to ODBC SQL type.
    */

    if (pipd->ParameterType == SQL_PARAM_OUTPUT)
    {
        if (pstmt->fCommand != CMD_EXECPROC)  /* error if not call proc */
            return ErrState (SQL_HY105, pstmt); /* invalid parameter type */
    }

 /* if a procedure call is to just use the default value of the parm, just return */
    if ((pcbValue) && (*pcbValue == SQL_DEFAULT_PARAM))
        return SQL_SUCCESS;

    if (!isTypeSupported(fCType, fSqlType))
    {
        return ErrState (SQL_07006, pstmt);
                  /* Restricted data type attribute violation */
    }
    ptype = CvtGetCType(fCType, pstmt->fAPILevel, pstmt->dateConnLevel);

    /*
    **  Decode pcbValue:
    **
    **  1. NULL parm values.
    **  2. Parm to be passed by SQLPutData.
    **  3. Null terminated strings.
    **  4. Actual length of parm if not implicit.
    */
    if (IndicatorPtr  &&  *IndicatorPtr == SQL_NULL_DATA)
    {
        if (pipd->Nullable == SQL_NO_NULLS)
            return ErrState (SQL_23000, pstmt);

        pipd->Null = TRUE;
        papd->fStatus |= COL_PASSED;

        /*
           if passing null date to gateways, we don't need to call 
           iiapi_convertdata, but we do need to set sqllen to 12.
        */
        if ( (!isServerClassINGRES(pstmt->pdbcOwner)) &&
             (pipd->ConciseType == SQL_TYPE_TIMESTAMP) )
        {
                pipd->OctetLength = 12;
        }
        return SQL_SUCCESS;
    }  /* end if (IndicatorPtr) */

    pipd->Null = FALSE;

    if (pcbValue)   /* SQLBindParameter's StrLen_or_IndPtr */
    {
        switch (*pcbValue)
        {
        case SQL_DATA_AT_EXEC:      /* only if not from SQLPutData... */

            return SQL_NEED_DATA;

        case SQL_NTS:

            fSqlNts = TRUE;
            break;

        default:

            if (!(papd->fStatus & COL_ODBC10) &&
                *pcbValue <= SQL_LEN_DATA_AT_EXEC_OFFSET)
                return SQL_NEED_DATA;


            if (fCType == SQL_C_CHAR   ||
                fCType == SQL_C_BINARY ||
                fCType == SQL_C_WCHAR)
            {
                cbdwValue = (UDWORD) *pcbValue;  /* value length (bytes) */
                cbValue   = (UWORD)cbdwValue;
            }
        }
    }
    else
    {
        fSqlNts = TRUE;
    }
    /*
    **
    **  Get value based on type:
    **
    */
    switch (fCType)
    {
    case SQL_C_CHAR:
    case SQL_C_BINARY:
    case SQL_C_WCHAR:

        if (rgbValue)
        {
            if (fSqlNts) 
               {
                if (fCType == SQL_C_WCHAR)  /* if wide (Unicode) string */
                    cbdwValue = (UDWORD)wcslen ((SQLWCHAR*)rgbValue)*sizeof(SQLWCHAR);
                else
                    cbdwValue = (UDWORD)STlength (rgbValue);
                cbValue = (UWORD)cbdwValue;   /* source length in bytes */
               }
        }
        else
        {
            if (cbValue) return ErrState (SQL_HY090, pstmt);
                                         /* invalid string or buffer length */
        }
        if (fCType == SQL_C_WCHAR && !fSqlNts && !pipd->SegmentLength)
        {
            if (fSqlType == SQL_VARCHAR || fSqlType == SQL_CHAR && 
                !pipd->SegmentLength)
                    pipd->OctetLength /= sizeof(SQLWCHAR);
        }

        break;

    case SQL_C_TYPE_NULL:  /* special case for typeless nulls */
        break;

    default:

        if (!rgbValue) return ErrState (SQL_HY090, pstmt);
                                         /* invalid string or buffer length */

        switch (fCType)
        {
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:

            tinyValue = *(SCHAR*)rgbValue;
            intValue = (SQLINTEGER)tinyValue;
            break;

        case SQL_C_UTINYINT:

            tinyValue = *(UCHAR*)rgbValue;
            uintValue = (SQLUINTEGER)tinyValue;
            break;

        case SQL_C_SBIGINT:
        case SQL_C_UBIGINT:

            int64Value = *(ODBCINT64 *)rgbValue;
            break;
 
        case SQL_C_SHORT:
        case SQL_C_SSHORT:

            intValue = *(SWORD*)rgbValue;
            break;

        case SQL_C_USHORT:

            uintValue = *(UWORD*)rgbValue;
            break;

        case SQL_C_LONG:
        case SQL_C_SLONG:

            intValue = *(SDWORD*)rgbValue;
            break;

        case SQL_C_ULONG:

            uintValue = *(UDWORD*)rgbValue;
            break;

        case SQL_C_FLOAT:

            floatValue = *(SFLOAT*)rgbValue;
            break;

        case SQL_C_DOUBLE:

            floatValue = *(SDOUBLE*)rgbValue;
            break;

        }
    }

    /*
    **  Set up SQL data:
    **
    **  1. Make type slightly easier to use.
    **  2. Set null indicator off if present.
    **  3. Find data destination in parm buffer.
    **  4. Find data length in parm buffer.
    */

    /*
    ** Octet length is adjusted bound length from SQLBindParameter().
    */

    if (pipd->SegmentLength)
        cbdwData = (UDWORD)pipd->SegmentLength;
    else
        cbdwData = (UDWORD)pipd->OctetLength; 
    cbData   = (UWORD)cbdwData;
    rgbData  = pipd->DataPtr;  /* target buffer */
    switch (ptype->fVerboseType)
    {
        case SQL_DATETIME:
        case SQL_INTERVAL: 
            retcode = SetColVar (pstmt, papd, pipd);
            if (retcode != SQL_SUCCESS)
               return retcode;
            break;

        default:
            break;
    }

    if (IItrace.fTrace >= ODBC_IN_TRACE)
    {
        if (rgbValue)
        {
            u_i4 blen = (pcbValue && *pcbValue > 0) ? *pcbValue : 0;
            u_i4 llen = (fCType == SQL_C_WCHAR) ? pipd->Length * 
                sizeof(SQLWCHAR) : pipd->Length;
            u_i4 wlen = (fCType == SQL_C_WCHAR) ? cbdwData * sizeof(SQLWCHAR) : 
                cbdwData;

            if (blen)
                 wlen = blen;
            else if (llen && (llen < wlen))
                 wlen = blen;
 
            if (wlen)
            {
                TRdisplay("PutParm() data hex dump C type %d SQL type %d " \
                    "len %d\n", fCType, fSqlType, wlen);
                odbc_hexdump(rgbValue, wlen);
            }
            else
                TRdisplay("PutParm() C type %d SQL type %d len 0\n", 
                    fCType, fSqlType);
        }
        else
            TRdisplay("PutParm() C type %d SQL type %d no parm buff\n", 
                fCType, fSqlType);
    }

    /*
    **  fCType    = source data type
    **  rgbValue -> source buffer
    **  cbValue   = source length
    **
    **  fSqlType  = target data type
    **  rgbData  -> target buffer
    **  cbData    = target length
    */

    /*
    **  Convert C data to SQL as requested:
    */
    switch (fCType)
    {
    case SQL_C_WCHAR:

        if (fSqlType == SQL_WCHAR         ||   /* target types is WCHAR*/
            fSqlType == SQL_WVARCHAR      ||
            fSqlType == SQL_WLONGVARCHAR)
           {    rc = PutCharChar (papd, pipd, rgbData, rgbValue, cbdwData, cbdwValue, ' ', offsetData);
                break;  /* all done * with WCHAR->WCHAR */
           }

        /*
        **  if source type is a wide (Unicode) string and
        **  target type is not WCHAR, WVARCHAR, nor WLONGVARCHAR (they handle
        **  the wide string themselves) then convert the wide string
        **  to a normal character string first and point rgbData to it.
        */
        cbdwValue=cbdwValue/sizeof(SQLWCHAR); /* min byte length of char buffer needed */

        if (cbdwValue < sizeof(szValueWChar))
             rgbValueWk = szValueWChar;   /* use the stack work buffer */
        else
            {
             rgbValueWk =  /* allocate work area for string (plus a safety) */
               pwork = MEreqmem(0, cbdwValue+4, TRUE, NULL);
             if (pwork == NULL)  /* no memory!? */
                {retcode = ErrState (SQL_HY001, pstmt, F_OD0015_IDS_WORKAREA);
                 return retcode;
                }
            }

        /* On entry: rgbData -> wide string
                cbdwValue  =  wide string length */

        if (!fSqlNts && (pipd->Length < cbdwValue))
        {
            pstmt->fCvtError |= CVT_TRUNCATION;
            return SQL_ERROR;
        }

        retcode = ConvertWCharToChar(pstmt,
                       (SQLWCHAR*)rgbValue, cbdwValue,     /* source */
                       rgbValueWk,          cbdwValue+3,   /* target */
                       (SWORD*)(&cbValue),
                       (SDWORD*)(&cbdwValue),  /* where to return target length */
                       NULL);
        if (retcode != SQL_SUCCESS  &&  retcode != SQL_SUCCESS_WITH_INFO)
           {if (pwork)
                MEfree((PTR)pwork);
            return(retcode);
           }
        rgbValue = rgbValueWk;  /* point rgbValue to ANSI string */
                        /* On exit:  rgbValue -> source character string
                                     cbValue  =  source character string length
                                     cbdwValue=  source character string length */

        /* treat as SQL_C_CHAR case and fall through */

    case SQL_C_CHAR:  /* and fall-through from SQL_C_WCHAR */

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

            rc = PutCharChar (papd, pipd, rgbData, rgbValue, cbdwData, cbdwValue, ' ', offsetData);
            break;

        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:

            if (fCType == SQL_C_CHAR)  /* if we need CHAR->WCHAR conversion */
               {
                UDWORD      cbdwBuff;
                              /* byte length of buffer needed to hold WCHAR string*/
                cbdwBuff=cbdwValue*sizeof(SQLWCHAR); /* min byte length of buffer needed */

                if (cbdwBuff < sizeof(szValueWChar)-4)
                     rgbValueWk = szValueWChar;   /* use the stack work buffer */
                else
                    {
                     rgbValueWk =  /* allocate work area for string (plus a safety) */
                       pwork = MEreqmem(0, cbdwBuff+4, TRUE, NULL);
                     if (pwork == NULL)  /* no memory!? */
                        {retcode = ErrState (SQL_HY001, pstmt, F_OD0015_IDS_WORKAREA);
                         return retcode;
                        }
                    }

                        /* On entry: rgbData    -> CHAR string
                                     cbdwValue  =  CHAR string length */
                retcode = ConvertCharToWChar(pstmt,
                               rgbValue,              cbdwValue,     /* source */
                               (SQLWCHAR*)rgbValueWk, cbdwBuff+4,   /* target */
                               (SWORD*)(&cbValue),
                               (SDWORD*)(&cbdwValue),  /* where to return target length */
                               NULL);
                if (retcode != SQL_SUCCESS  &&  retcode != SQL_SUCCESS_WITH_INFO)
                   {if (pwork)
                        MEfree((PTR)pwork);
                    return(retcode);
                   }

                rgbValue = rgbValueWk;  /* point rgbValue to WCHAR string */
                cbdwValue=cbdwValue*sizeof(SQLWCHAR); /* byte length of WCHAR string */
                        /* On exit:  rgbValue -> source character string
                                     cbValue  =  source character string length
                                     cbdwValue=  source character string length */

               }  /* end if (fCType == SQL_C_CHAR)   CHAR->WCHAR conversion */

            rc = PutCharChar (papd, pipd, rgbData, rgbValue, cbdwData, cbdwValue, ' ', offsetData);
            break;

        case SQL_NUMERIC:
        case SQL_DECIMAL:
        {
            char *p = NULL;
            char *szTemp = STalloc(rgbValue);
			UWORD strScale = 0;

            /*
            ** Truncate the string if the length indicator is less than
            ** the string length of the numeric string.  A separate
            ** buffer is used to edit the string, because rgbValue is passed
            ** directly from the application.
            */
            if (pcbValue && (*pcbValue > 0) &&
                (*pcbValue < (SQLINTEGER)STlength(szTemp)))
                    szTemp[*pcbValue] = '\0';

            /*
            **  Reject commas if II_DECIMAL is not so defined.
            */
            if (pdbc->szDecimal[0] == '.' && STindex(szTemp,",",0))
            {
                rc = CVT_OUT_OF_RANGE;
                goto numStrCvtError;
            }     

            /*
            ** Temporarily put a period in the decimal string
            ** if the decimal point is a comma and II_DECIMAL
            ** is set to a comma.
            ** CvtCharNumStr() does not "see" commas.
            */
            if (pdbc->szDecimal[0] == ',')
            {
                p = STindex(szTemp,",",0);
                if (p)
                     *p = '.';
            }
           
            /*
            ** If the scale from SQLBindParameter() is less than the number 
            ** of digits to the right of the decimal point, truncate the 
            ** string and set a fractional truncation warning.
            */
            lpChar = STindex(szTemp,".",0);
            if (lpChar)
            {
                CMnext(lpChar);
                while (CMdigit(lpChar))
                {
                    strScale++;
                    if (strScale > pipd->Scale)
                    {
                        *lpChar = '\0';
                        fractionalTrunc = TRUE;
                        break;
                    }
                    CMnext(lpChar);
                }
            }
         
            /*
            ** Normalize for leading or trailing zeros before converting to
            ** packed decimal.
            */
            rc = CvtCharNumStr (szValue, DB_MAX_DECPREC+4, szTemp, 
                (UWORD)STlength(szTemp), NULL, NULL);
            if (rc != CVT_SUCCESS)
                goto numStrCvtError;

            /*
            ** Put the comma back if present.  The API definitely
            ** "sees" commas in decimal strings.
            */
            if (p)
            {
                p = STindex(szValue,".",0);
                if (p)
                      *p = ',';
            }
            if (!CvtApiTypes( rgbData, (PTR)szValue, 
                pstmt->pdbcOwner->envHndl,
                    pipd, IIAPI_DEC_TYPE, IIAPI_CHA_TYPE))
              rc = CVT_OUT_OF_RANGE;
            else if (fractionalTrunc)
                rc = CVT_FRAC_TRUNC;

numStrCvtError:
            MEfree(szTemp);
            break;
        }
        case SQL_BIT:
        case SQL_TINYINT:

        rc = CvtCharInt (&intValue, rgbValue, cbValue);
            if (rc != CVT_SUCCESS)
                break;

            if (intValue < -128 || intValue > 255)
                return CVT_OUT_OF_RANGE;

            tinyValue = (SCHAR)intValue;
            rc = PutTinyInt (rgbData, tinyValue, fSqlType, FALSE);
            break;

		case SQL_SMALLINT:
        case SQL_INTEGER:

            rc = CvtCharInt (&intValue, rgbValue, cbValue);
            if (rc != CVT_SUCCESS)
                break;

            rc = PutInt (rgbData, intValue, fSqlType, FALSE);
            break;
        
        case SQL_BIGINT:

            rc = CvtCharInt64 (&int64Value, rgbValue, cbValue);
            if (rc != CVT_SUCCESS)
                break;
            rc = PutInt64 (rgbData, int64Value, fSqlType, FALSE);
            break;

        case SQL_REAL:

            rc = CvtCharFloat (&floatValue, rgbValue, cbValue);
            if (rc != CVT_SUCCESS)
                break;
            if (ODBCfabs(floatValue) > FLT_MAX)
            {
                rc = CVT_OUT_OF_RANGE;
                break;
            }
            *(SFLOAT *)rgbData = (SFLOAT)floatValue;
            break;

        case SQL_FLOAT:
        case SQL_DOUBLE:

            rc = CvtCharFloat ((SDOUBLE *)rgbData, rgbValue, cbValue);
            break;

        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:

            rc = PutCharBin (papd, pipd, rgbData, rgbValue, cbdwData, cbdwValue, offsetData);
            break;

        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP:
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            /*
            **  Extract date from ODBC escape sequence if needed:
            **
            **  1. Copy into temp buffer, strip leading spaces if too big
            **  2. Parse for escape sequence, note null terminated now
            **  3. If found, convert to Ingres format, strip quotes, and use it
            */
            while (cbValue > (UWORD)(LEN_TEMP - 1) && CMwhite(rgbValue))
            {
                CMnext(rgbValue);
                cbValue--;
            }
            if (cbValue > (UWORD)(LEN_TEMP - 1))
            {
                rc = CVT_DATE_OVERFLOW;
                break;
            }
            STlcopy (rgbValue, rgbTemp, cbValue);
            rgbNull = ParseEscape (pstmt, rgbTemp);
                          /* Convert ODBC escape sequence into Ingres syntax */
            if (rgbNull)
            {
                if (!ParseConvert (pstmt,rgbNull,NULL,isaParameterTRUE,
                   pstmt->dateConnLevel))
                {
                    rc = CVT_DATE_OVERFLOW;
                    break;
                }
                /* strip leading spaces */
                while (CMwhite(rgbNull)) CMnext(rgbNull);
                rgbValue = rgbNull; 
    
                /* strip leading quote */
                CMnext(rgbValue);
    
                /* strip trailing quote */
                lpChar = rgbValue; 
                lpSave = NULL;
                while(*lpChar != EOS)       
                {
                    if (! CMcmpcase(lpChar,"'") )
                        lpSave = lpChar;
                    CMnext(lpChar);
                }
                /* Trailing quote might not be the last char. There could be 
                some blanks after the trailing quote. */
                if (lpSave)
                {
                    const char nt[] = "\0";
                    cbValue = (UWORD) (lpSave - rgbValue);
                    CMcpychar(nt, lpSave);
                }
            }

            /*
            **  Convert data as specified per ODBC 2.0:
            */

            if (fSqlType == SQL_TYPE_DATE)
            {
                rc = CvtCharTimestamp (&stampValue, rgbValue, cbValue, 
                    pstmt->dateConnLevel);
                if (rc == CVT_SUCCESS)
                {
                    dateValue.year  = stampValue.year;
                    dateValue.month = stampValue.month;
                    dateValue.day   = stampValue.day;
                    rc = CvtDateChar (&dateValue, rgbData, cbData, 
                        pstmt->dateConnLevel);
                    if (rc == CVT_SUCCESS &&
                        stampValue.hour   || stampValue.minute ||
                        stampValue.second || stampValue.fraction)
                        rc = CVT_TRUNCATION;
                    break;
                }

                rc = CvtCharDate (&dateValue, rgbValue, cbValue);
                if (rc != CVT_SUCCESS)   break;
                rc = CvtDateChar (&dateValue, rgbData, cbData, 
                    pstmt->dateConnLevel);
                if (rc != CVT_SUCCESS)   break;

                if (pstmt->dateConnLevel < IIAPI_LEVEL_4)
                {
		    /* 
                    ** Convert to Ingres format only when
		    ** the server class is Ingres or VSAM which require 
                    ** internal Ingres format.
                    */
		    if (isServerClassINGRES(pstmt->pdbcOwner) || 
                        isServerClassVSAM(pstmt->pdbcOwner))
                        /* 
                        ** Convert ODBC date format (yyyy-mm-dd hh:mm:ss)
                        ** to Ingres US format (yyyy_mm_dd hh:mm:ss).
                        */ 
                        ODBCDateToIngresUS(rgbData);
                }
		else
                    rc = CvtCharToIngresDateTime(rgbData, pipd, 
                        pstmt->dateConnLevel);

                    
                break;
            }    /* endif fSqlType == SQL_TYPE_DATE */ 

            if (fSqlType == SQL_TYPE_TIME || fVerboseType == SQL_INTERVAL)
            {
                rc = CvtCharTimestamp (&stampValue, rgbValue, cbValue, 
                    pstmt->fAPILevel);
                if (rc == CVT_SUCCESS)
                {
                    timeValue.hour   = stampValue.hour;
                    timeValue.minute = stampValue.minute;
                    timeValue.second = stampValue.second;
                    timeValue.fraction = stampValue.fraction; 
		    rc = CvtTimeChar (&timeValue, rgbData, cbData,
                        pstmt->fAPILevel);
                    if (rc == CVT_SUCCESS && stampValue.fraction)
                        rc = CVT_TRUNCATION;

                    break;
                }
                rc = CvtCharTime (&timeValue, rgbValue, cbData, 
                    pstmt->fAPILevel);
                if (rc == CVT_SUCCESS)
                {
                    rc = CvtTimeChar (&timeValue, rgbData, cbData, 
                        pstmt->fAPILevel);
                    if (rc == CVT_SUCCESS && pstmt->fAPILevel 
                         > IIAPI_VERSION_3)
                    {
                        rc = CvtCharToIngresDateTime(rgbData, pipd, 
                            pstmt->fAPILevel);
                    } 
                }
                else if (fVerboseType == SQL_INTERVAL)
                {
                    rc = CvtCharInterval (rgbValue, &interval, fSqlType);
                    if (rc == CVT_SUCCESS)
                        rc = CvtIntervalChar(&interval, rgbData, fSqlType);
                            if (rc == CVT_SUCCESS)
                                rc = CvtCharToIngresDateTime(rgbData, pipd, 
                                    pstmt->fAPILevel);
                }
                break;
            }    /* endif fSqlType == SQL_TYPE_TIME */
            
            if (fSqlType == SQL_TYPE_TIMESTAMP)
            {
                rc = CvtCharTimestamp (&stampValue, rgbValue, cbValue, 
                    pstmt->dateConnLevel);
                if (rc == CVT_SUCCESS)
                {
                    rc = CvtTimestampChar ( &stampValue, rgbData, cbData, 
                        (UWORD)pipd->Precision, pstmt->dateConnLevel);
                    if (rc == CVT_SUCCESS)
                    {
                        if (pstmt->dateConnLevel < IIAPI_LEVEL_4)
                            ODBCDateToIngresUS(rgbData);
                        /* 
                        ** Convert ODBC date format (yyyy-mm-dd hh:mm:ss)
                        ** to Ingres US format (yyyy_mm_dd hh:mm:ss).
                        **
                        ** If not INGRES...
                        */
                        if (!isServerClassINGRES(pstmt->pdbcOwner)|| 
                            pstmt->dateConnLevel > IIAPI_LEVEL_3)
                            rc = CvtCharToIngresDateTime(rgbData, pipd, 
                                pstmt->fAPILevel);
                            /* convert to ts struct */
                    }
                }
                break;
            }    /* endif fSqlType == SQL_TYPE_TIMESTAMP */ 

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_NUMERIC:
    {
        SQL_NUMERIC_STRUCT numericValue;     
        void *envHndl = pstmt->pdbcOwner->envHndl;
        CHAR decimal_char = pdbc->szDecimal[0];
        CHAR *workBuff = MEreqmem(0, pipd->Precision, TRUE, NULL);
        CHAR *wb=&workBuff[0];
        numericValue = *((SQL_NUMERIC_STRUCT *)rgbValue);
        rc = CvtSqlNumericStr(workBuff, (UWORD)pipd->Precision, 
            pipd->Scale, numericValue);
        if (rc != CVT_SUCCESS)
            break;
        if ((wb = STindex(workBuff, ".", 0)) != NULL) 
            *wb = decimal_char;
        
        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

            rc = PutNumChar (papd, pipd, rgbData, workBuff, 
                (UWORD)pipd->Precision);
            break;

        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
        {
            UDWORD      cbdwBuff;
            cbdwValue = (UDWORD)pipd->Precision;
            /* byte length of buffer needed to hold WCHAR string*/
            cbdwBuff=cbdwValue*sizeof(SQLWCHAR); /* min len of buff */
            if (cbdwBuff < sizeof(szValueWChar)-4)
                rgbValueWk = szValueWChar;   /* use the stack work buffer */
            else
            {
                rgbValueWk =  /* work area for string (plus a safety) */
                pwork = MEreqmem(0, cbdwBuff+4, TRUE, NULL);
                if (pwork == NULL)  /* no memory!? */
                {retcode = ErrState (SQL_HY001, pstmt, F_OD0015_IDS_WORKAREA);
                         return retcode;
                }
            }
            retcode = ConvertCharToWChar(pstmt,
                workBuff,              cbdwValue,     /* source */
                (SQLWCHAR*)rgbValueWk, cbdwBuff+4,   /* target */
                NULL, NULL, NULL);
            if (retcode != SQL_SUCCESS  &&  retcode != SQL_SUCCESS_WITH_INFO)
                return(retcode);
            rgbValue = rgbValueWk;
            rc = PutCharChar (papd, pipd, rgbData, rgbValue, cbdwData, 
                cbdwBuff, ' ', offsetData);
            break;
        }
        case SQL_DECIMAL:
        case SQL_NUMERIC:

            if (!CvtApiTypes(rgbData, workBuff, envHndl, pipd, 
                IIAPI_DEC_TYPE, IIAPI_CHA_TYPE))
                rc = CVT_OUT_OF_RANGE;
            break;

        case SQL_REAL:

            rc = CvtCharFloat (&floatValue, workBuff, (UWORD)pipd->Precision);
            if (rc != CVT_SUCCESS)
                break;
            if (ODBCfabs(floatValue) > FLT_MAX)
            {
                rc = CVT_OUT_OF_RANGE;
                break;
            }
            *(SFLOAT *)rgbData = (SFLOAT)floatValue;
            break;

        case SQL_FLOAT:
        case SQL_DOUBLE:

            rc = CvtCharFloat ((SDOUBLE *)rgbData, workBuff, 
                (UWORD)pipd->Precision);
            break;

        case SQL_BIT:
        case SQL_TINYINT:
            
            if ((rc = CVal(workBuff, (i4 *)&intValue)) == OK)
            {
                if (intValue < -128 || intValue > 255)
                    return CVT_OUT_OF_RANGE;

                tinyValue = (SCHAR)intValue;
                rc = PutTinyInt (rgbData, tinyValue, fSqlType, FALSE);
            }
            break;

        case SQL_SMALLINT:
        case SQL_INTEGER:
        {
            /*
            ** Truncate the fraction if it exists.
            */
            if ((wb = STindex(workBuff, pdbc->szDecimal, 0)) != NULL) 
            { 
                char *t;
                t=wb;        /* points at decimal char in string */
                CMnext(wb);  /* c-> fraction string just past '.' */
                *t = EOS;    /* null term the integer part        */
            }
            if ((rc = CVal(workBuff, (i4 *)&intValue)) == OK)
                rc = PutInt (rgbData, intValue, fSqlType, FALSE);
            break;
        }
		case SQL_BIGINT:
        {
            /*
            ** Truncate the fraction if it exists.
            */
            if ((wb = STindex(workBuff, pdbc->szDecimal, 0)) != NULL) 
            { 
                char *t;
                t=wb;        /* points at decimal char in string */
                CMnext(wb);  /* c-> fraction string just past '.' */
                *t = EOS;    /* null term the integer part        */
            }
            if ((rc = CVal8(workBuff, (i8 *)&int64Value)) == OK)
                rc = PutInt64 (rgbData, int64Value, fSqlType, FALSE);
            break;
        }

        default:

            rc = CVT_INVALID;
            break;
        }

        MEfree(workBuff);
        break;
    }
    case SQL_C_USHORT:
    case SQL_C_ULONG:
    case SQL_C_UTINYINT:

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

            CvtUltoa(uintValue, szValue);
            rc = PutNumChar (papd, pipd, rgbData, szValue, cbData);
            break;

        case SQL_DECIMAL:
        case SQL_NUMERIC:
            rc = CvtIntNum (szValue, uintValue,
                    (UWORD)pipd->Precision, pipd->Scale, TRUE);

            if (rc != CVT_SUCCESS)
                break;

            CvtPack (rgbData, cbData, szValue, (UWORD)pipd->Precision);
            break;


        case SQL_BIT:
        case SQL_TINYINT:

            if (uintValue > 255)
                return CVT_OUT_OF_RANGE;

            rc = PutTinyInt (rgbData, (SDWORD)uintValue, fSqlType, TRUE);
            break;

        case SQL_SMALLINT:
        case SQL_INTEGER:

            rc = PutInt (rgbData, (SDWORD)uintValue, fSqlType, TRUE);
            break;

        case SQL_REAL:

            *(SFLOAT *)rgbData = (SFLOAT)uintValue;
            break;

        case SQL_FLOAT:
        case SQL_DOUBLE:

            *(SDOUBLE *)rgbData = uintValue;
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_BIT:
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
    case SQL_C_LONG:
    case SQL_C_SLONG:

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            
            CVla((i4)intValue, szValue);
            cbData = pipd->OctetLength = pipd->Length;
            rc = PutNumChar (papd, pipd, rgbData, szValue, cbData);
            break;

        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
        {
            UDWORD      cbdwBuff;

            CVla((i4)intValue, szValue);
            /* byte length of buffer needed to hold WCHAR string*/
			cbdwValue = STlength(szValue) + sizeof(CHAR);
            cbdwBuff = cbdwData = (cbdwValue * sizeof(SQLWCHAR));

            /* min byte length of buffer needed */

            rgbValueWk = szValueWChar;   /* use the stack work buffer */

            /* On entry: rgbData    -> CHAR string
               cbdwValue  =  CHAR string length */
            retcode = ConvertCharToWChar(pstmt,
                szValue,              cbdwValue,     /* source */
                (SQLWCHAR*)rgbValueWk, cbdwBuff+4,   /* target */
                (SWORD*)(&cbValue),
                (SDWORD*)(&cbdwValue),  /* target length */
                               NULL);
        
            if (retcode != SQL_SUCCESS  &&  retcode 
                != SQL_SUCCESS_WITH_INFO)
            {
                if (pwork)
                    MEfree((PTR)pwork);
                return(retcode);
            }
            rgbValue = rgbValueWk;  /* point rgbValue to WCHAR string */
            cbdwValue=cbdwValue*sizeof(SQLWCHAR); /* length of WCHAR  */
            pipd->OctetLength = cbdwValue;
            rc = PutCharChar (papd, pipd, rgbData, rgbValue, cbdwData, 
                cbdwValue, ' ', offsetData);
            break;
        }
        case SQL_DECIMAL:
        case SQL_NUMERIC:

            CVla((i4)intValue, szValue);
            if (!CvtApiTypes( rgbData, (PTR)szValue,
                pstmt->pdbcOwner->envHndl,
                    pipd, IIAPI_DEC_TYPE, IIAPI_CHA_TYPE))
                rc = CVT_OUT_OF_RANGE;

            break;


        case SQL_BIT:
        case SQL_TINYINT:
            if (intValue < -128 || intValue > 255)
                return CVT_OUT_OF_RANGE;

            tinyValue = (SCHAR)intValue;
            rc = PutTinyInt (rgbData, tinyValue, fSqlType, FALSE);
            break;

        case SQL_SMALLINT:
        case SQL_INTEGER:

            rc = PutInt (rgbData, intValue, fSqlType, FALSE);
            break;

        case SQL_BIGINT:

            int64Value = (SQLBIGINT)intValue;
            rc = PutInt64 (rgbData, intValue, fSqlType, FALSE);
            break;

        case SQL_REAL:

            *(SFLOAT *)rgbData = (SFLOAT)intValue;
            break;

        case SQL_FLOAT:
        case SQL_DOUBLE:

            *(SDOUBLE *)rgbData = intValue;
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_UBIGINT:
    case SQL_C_SBIGINT:

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

            CVla8(int64Value, szValue);
            rc = PutNumChar (papd, pipd, rgbData, szValue, cbData);
            break;

        case SQL_DECIMAL:
        case SQL_NUMERIC:
            rc = CvtInt64Num (szValue, int64Value,
                (UWORD)pipd->Precision, pipd->Scale, FALSE);

            if (rc != CVT_SUCCESS)
                break;

            CvtPack (rgbData, cbData, szValue, (UWORD)pipd->Precision);
            break;

        case SQL_BIT:
        case SQL_TINYINT:

            intValue = (UDWORD)int64Value;
            if (intValue < -128 || intValue > 255)
                return CVT_OUT_OF_RANGE;

            tinyValue = (SCHAR)intValue;
            rc = PutTinyInt (rgbData, tinyValue, fSqlType, FALSE);
            break;

        case SQL_SMALLINT:
        case SQL_INTEGER:
            intValue = (SDWORD)int64Value;
            rc = PutInt (rgbData, intValue, fSqlType, FALSE);
            break;

        case SQL_BIGINT:
            rc = PutInt64 (rgbData, int64Value, fSqlType, FALSE);
            break;

        case SQL_REAL:

            *(SFLOAT *)rgbData = (SFLOAT)int64Value;
            break;

        case SQL_FLOAT:
        case SQL_DOUBLE:

            *(SDOUBLE *)rgbData = (SDOUBLE)int64Value;
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_FLOAT:
    case SQL_C_DOUBLE:

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
           { size_t cbValue;
             i2     res_width;
             CHAR * sz;
/*          _gcvt (floatValue, 17, szValue);  */
            CVfa(floatValue, 22, 14, 'g', '.', szValue, &res_width);
            sz = szValue;
            if (*sz == ' ')    /* if sign character is a blank, squeeze if out */
                 sz++;
            cbValue = STlength (sz);
            while (cbValue  &&  sz[cbValue-1]==' ')
	            sz[--cbValue]='\0';   /* strip trailing blanks */
            if (cbValue  &&  sz[cbValue-1]=='.')
	            sz[--cbValue]='\0';   /* strip trailing '.' */
            rc = PutNumChar (papd, pipd, rgbData, sz, cbData);
            break;
           }

        case SQL_DECIMAL:
        case SQL_NUMERIC:
            rc = CvtFloatNum (szValue, floatValue,
                    (UWORD)pipd->Precision, pipd->Scale, FALSE);

            if (rc != CVT_SUCCESS)
                break;

            CvtPack (rgbData, cbData, szValue, (UWORD)pipd->Precision);
            break;

        case SQL_BIT:
        case SQL_TINYINT:

            intValue = (SDWORD)floatValue;
            
            if (intValue < -128 || intValue > 255)
                return CVT_OUT_OF_RANGE;
            
            tinyValue = (SCHAR)intValue;
            rc = PutTinyInt (rgbData, tinyValue, fSqlType, FALSE);
            break;

        case SQL_SMALLINT:
        case SQL_INTEGER:

            rc = PutInt (rgbData, (SDWORD)floatValue, fSqlType, FALSE);
            break;

        case SQL_BIGINT:

            rc = PutInt64 (rgbData, (ODBCINT64)floatValue, fSqlType, FALSE);
            break;

        case SQL_REAL:

            *(SFLOAT *)rgbData = (SFLOAT)floatValue;
            break;

        case SQL_FLOAT:
        case SQL_DOUBLE:

            *(SDOUBLE *)rgbData = floatValue;
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_BINARY:

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:

            rc = PutCharChar (papd, pipd, rgbData, rgbValue, cbdwData, cbdwValue, 0, offsetData);
            break;

        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_BIT:
        case SQL_TINYINT:
        case SQL_BIGINT:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP:

            if (cbValue == cbData)
                MEcopy (rgbValue, cbValue, rgbData);
            else
                rc = CVT_OUT_OF_RANGE;
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_TYPE_DATE:

        rc = CvtEditDate ((DATE_STRUCT     *)rgbValue);
        if (rc != CVT_SUCCESS) break;

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

            rc = CvtDateChar ((DATE_STRUCT     *)rgbValue, rgbData, cbData,
                pstmt->dateConnLevel);
            if (rc != CVT_SUCCESS)   break;

            /* Convert to Ingres format if the server is Ingres or VSAM */
	    if (isServerClassINGRES(pstmt->pdbcOwner) || isServerClassVSAM(pstmt->pdbcOwner))
                ODBCDateToIngresUS(rgbData);

            /* 
            ** For SQL_C_DATE to SQL_CHAR for non-Ingres server,
	    ** blank out the trailing part of pipd->DataPtr.
            */
            if (!isServerClassINGRES(pstmt->pdbcOwner) && cbData > 10)
	       MEfill(cbData - 10, ' ', &rgbData[10]);
            break;

        case SQL_TYPE_DATE:
            rc = CvtDateChar ((DATE_STRUCT     *)rgbValue, rgbData, cbData, pstmt->dateConnLevel);
            if (rc != CVT_SUCCESS)   break;

	    if (isServerClassINGRES(pstmt->pdbcOwner) || isServerClassVSAM(pstmt->pdbcOwner))
            {
                if (pstmt->dateConnLevel < IIAPI_LEVEL_4)
                    /* 
                    ** Convert ODBC date format (yyyy-mm-dd hh:mm:ss) to 
                    ** Ingres US format (yyyy_mm_dd hh:mm:ss). 
                    */
                    ODBCDateToIngresUS(rgbData);
                else
                    /*
                    ** Convert to a binary date structure.
                    */
                    rc = CvtCharToIngresDateTime(rgbData, pipd, 
                         pstmt->dateConnLevel);
            }

            break;

        case SQL_TYPE_TIMESTAMP:

            stampValue.year     = ((DATE_STRUCT     *)rgbValue)->year;
            stampValue.month    = ((DATE_STRUCT     *)rgbValue)->month;
            stampValue.day      = ((DATE_STRUCT     *)rgbValue)->day;
            stampValue.hour     = 0;
            stampValue.minute   = 0;
            stampValue.second   = 0;
            stampValue.fraction = 0;

            rc = CvtTimestampChar (
                    &stampValue, rgbData, cbData, (UWORD)pipd->Precision,
                    pstmt->fAPILevel);
            if (rc == CVT_SUCCESS)
               {
                if (pstmt->fAPILevel < IIAPI_LEVEL_4)
                    ODBCDateToIngresUS(rgbData);
                   /* convert ODBC date format (yyyy-mm-dd hh:mm:ss)
                           to Ingres US format (yyyy_mm_dd hh:mm:ss) */ 
                /* if not INGRES */
                if (!isServerClassINGRES(pstmt->pdbcOwner) || 
                      pstmt->fAPILevel > IIAPI_LEVEL_3)
                      rc = CvtCharToIngresDateTime(rgbData, pipd, 
                           pstmt->fAPILevel);
                                         /* convert to ts struct */
               }
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_TYPE_TIME:


        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

        rc = CvtEditTime ((ISO_TIME_STRUCT     *)rgbValue, 
             pstmt->fAPILevel);
        if (rc != CVT_SUCCESS) break;

            rc = CvtTimeChar ((ISO_TIME_STRUCT *)rgbValue, rgbData, cbData, 
                pstmt->fAPILevel);

	    if (!isServerClassINGRES(pstmt->pdbcOwner) && cbData > 8)
            {
               /* blank out the trailing part of pipd->DataPtr */
	       MEfill(cbData - 8, ' ', &rgbData[8]);
	    }

            break;

        case SQL_TYPE_TIME:
			((ISO_TIME_STRUCT *)rgbValue)->fraction = 0;
            rc = CvtEditTime ((ISO_TIME_STRUCT     *)rgbValue, 
                pstmt->fAPILevel);
            if (rc != CVT_SUCCESS) break;

            rc = CvtTimeChar ((ISO_TIME_STRUCT     *)rgbValue, rgbData, cbData,
                pstmt->fAPILevel);
            if (rc == SQL_SUCCESS && pstmt->fAPILevel > IIAPI_LEVEL_3)
                  rc = CvtCharToIngresDateTime(rgbData, pipd, 
                      pstmt->fAPILevel);

            break;

        case SQL_TYPE_TIMESTAMP:

            ((ISO_TIME_STRUCT *)rgbValue)->fraction = 0;
            rc = CvtEditTime ((ISO_TIME_STRUCT *)rgbValue, 
                pstmt->fAPILevel);
            if (rc != CVT_SUCCESS) break;

            CvtCurrStampDate (&stampValue, pstmt->fAPILevel);
            stampValue.hour     = ((ISO_TIME_STRUCT     *)rgbValue)->hour;
            stampValue.minute   = ((ISO_TIME_STRUCT     *)rgbValue)->minute;
            stampValue.second   = ((ISO_TIME_STRUCT     *)rgbValue)->second;
            if (pstmt->fAPILevel < IIAPI_LEVEL_4)
                stampValue.fraction = 0;
            else
                stampValue.fraction = ((ISO_TIME_STRUCT *)rgbValue)->fraction;
            rc = CvtTimestampChar ( &stampValue, rgbData, cbData, 
                (UWORD)pipd->Precision, pstmt->fAPILevel);
            if (rc == CVT_SUCCESS)
            {
                if (pstmt->fAPILevel < IIAPI_LEVEL_4)
                    ODBCDateToIngresUS(rgbData);
                /* 
                ** Convert ODBC date format (yyyy-mm-dd hh:mm:ss)
                ** to Ingres US format (yyyy_mm_dd hh:mm:ss).
                ** If not INGRES...
                */
                if (!isServerClassINGRES(pstmt->pdbcOwner) 
                    || pstmt->fAPILevel > IIAPI_LEVEL_3) 
                    rc = CvtCharToIngresDateTime(rgbData, pipd, 
                        pstmt->fAPILevel);
                           /* convert to ts struct */
            }
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_TYPE_TIMESTAMP:

        switch (fSqlType)
        {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:

            rc = CvtEditTimestamp ((TIMESTAMP_STRUCT     *)rgbValue,
                pstmt->fAPILevel);
            if (rc != CVT_SUCCESS) break;
            rc = CvtTimestampChar (
                    (TIMESTAMP_STRUCT     *)rgbValue,
                    rgbData, cbData, (UWORD)pipd->Precision, 
                    pstmt->fAPILevel);

	    /* Convert to Ingres format if the server is Ingres or VSAM */
            if (isServerClassINGRES(pstmt->pdbcOwner) || isServerClassVSAM(pstmt->pdbcOwner))
                ODBCDateToIngresUS(rgbData);

            /* convert to timestamp format yyyy-mm-dd-hh.mm.ss[.nnnnnn] */
	    if (isServerClassDB2(pstmt->pdbcOwner) ||
	        isServerClassDCOM(pstmt->pdbcOwner) ||
		isServerClassIDMS(pstmt->pdbcOwner))
	    {
             	rgbData[10] = '-';
		rgbData[13] = '.';
		rgbData[16] = '.';   
	    }

	    if (!isServerClassINGRES(pstmt->pdbcOwner))
            {
	        UDWORD i;

	        /* blank out the trailing part of pipd->DataPtr */

	        if (cbData > 26 && rgbData[19] == '.')
	           /* format yyyy-mm-dd hh.mm.ss.nnnnnn */
	           i = 26;
	        else if (cbData > 19)
                   /* format yyyy-mm-dd hh.mm.ss */
	           i = 19;

                MEfill(cbData - i, ' ', &rgbData[i]);
	    }

            break;

        case SQL_TYPE_DATE:
            dateValue.year  = ((TIMESTAMP_STRUCT     *)rgbValue)->year;
            dateValue.month = ((TIMESTAMP_STRUCT     *)rgbValue)->month;
            dateValue.day   = ((TIMESTAMP_STRUCT     *)rgbValue)->day;

            rc = CvtEditDate (&dateValue);
            if (rc != CVT_SUCCESS) break;

            if (((TIMESTAMP_STRUCT     *)rgbValue)->hour   ||
                ((TIMESTAMP_STRUCT     *)rgbValue)->minute ||
                ((TIMESTAMP_STRUCT     *)rgbValue)->second ||
                ((TIMESTAMP_STRUCT     *)rgbValue)->fraction)
                rc = CVT_TRUNCATION;

            rc = CvtDateChar (&dateValue, rgbData, cbData, 
                pstmt->dateConnLevel);
            if (rc != CVT_SUCCESS)   break;
                if (pstmt->dateConnLevel < IIAPI_LEVEL_4)
                    ODBCDateToIngresUS(rgbData);
                  /* convert ODBC date format (yyyy-mm-dd hh:mm:ss)
                       to Ingres US format (yyyy_mm_dd hh:mm:ss) */ 
                else
                    rc = CvtCharToIngresDateTime(rgbData, pipd, 
                        pstmt->dateConnLevel);
            break;

        case SQL_TYPE_TIME:

            timeValue.hour   = ((TIMESTAMP_STRUCT     *)rgbValue)->hour;
            timeValue.minute = ((TIMESTAMP_STRUCT     *)rgbValue)->minute;
            timeValue.second = ((TIMESTAMP_STRUCT     *)rgbValue)->second;
            timeValue.fraction = 0;
            rc = CvtEditTime (&timeValue, pstmt->fAPILevel);
            if (rc != CVT_SUCCESS) break;

            if (((TIMESTAMP_STRUCT     *)rgbValue)->fraction)
                rc = CVT_TRUNCATION;

            rc = CvtTimeChar (&timeValue, rgbData, cbData, 
                 pstmt->fAPILevel);
            break;

        case SQL_TYPE_TIMESTAMP:

            rc = CvtEditTimestamp ((TIMESTAMP_STRUCT     *)rgbValue, 
                pstmt->dateConnLevel);
            if (rc != CVT_SUCCESS) 
                break;

            rc = CvtTimestampChar (
                    (TIMESTAMP_STRUCT     *)rgbValue,
                    rgbData, cbData, (UWORD)pipd->Precision,
                    pstmt->dateConnLevel);
            if (rc == CVT_SUCCESS)
            {
                /* 
                ** Convert ODBC date format (yyyy-mm-dd hh:mm:ss)
                ** to Ingres US format (yyyy_mm_dd hh:mm:ss). 
                */
                if (pstmt->dateConnLevel < IIAPI_LEVEL_4)
					ODBCDateToIngresUS(rgbData);
                /*
                ** If not INGRES... 
                */
                if (!isServerClassINGRES(pstmt->pdbcOwner) || 
                    pstmt->dateConnLevel > IIAPI_LEVEL_3)
                    /*
                    ** Convert to binary date.
                    */
                    rc = CvtCharToIngresDateTime(rgbData, pipd, 
                        pstmt->dateConnLevel); 
            }
            break;

        default:

            rc = CVT_INVALID;
            break;
        }
        break;

    case SQL_C_TYPE_NULL:  /* special case for typeless nulls */
        break;

    case SQL_C_INTERVAL_YEAR:
    case SQL_C_INTERVAL_MONTH:
    case SQL_C_INTERVAL_YEAR_TO_MONTH:
    case SQL_C_INTERVAL_DAY:
    case SQL_C_INTERVAL_HOUR:
    case SQL_C_INTERVAL_MINUTE:
    case SQL_C_INTERVAL_SECOND:
    case SQL_C_INTERVAL_DAY_TO_HOUR:
    case SQL_C_INTERVAL_DAY_TO_MINUTE:
    case SQL_C_INTERVAL_DAY_TO_SECOND:
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
    case SQL_C_INTERVAL_MINUTE_TO_SECOND:
    {
        LPSQLTYPEATTR pt = CvtGetCType(fCType, pstmt->fAPILevel,
            pstmt->dateConnLevel);
        pInterval = (SQL_INTERVAL_STRUCT *)rgbValue;
        rc = CvtIntervalChar(pInterval, rgbData, pt->fSqlType);
        if (rc == SQL_SUCCESS)
            rc = CvtCharToIngresDateTime(rgbData, pipd, pstmt->fAPILevel);
        break;
    }
    default:

        rc = CVT_INVALID;
        break;
    }

    if (pwork)    /* free any allocated WCHAR->xxx or CHAR->WCHAR work buffer */
       MEfree((PTR)pwork);

    /*
    **  Return results:
    */
    switch (rc)
    {
    case CVT_FRAC_TRUNC:
        pstmt->fCvtError |= rc;
        return SQL_SUCCESS_WITH_INFO;

    case CVT_SUCCESS:

        return SQL_SUCCESS;

    case CVT_TRUNCATION:

        if (fSqlType == SQL_LONGVARCHAR   ||
            fSqlType == SQL_LONGVARBINARY ||
            fSqlType == SQL_WLONGVARCHAR)
            rc = CVT_STRING_TRUNC;

     default:

        pstmt->fCvtError |= rc;
        return SQL_ERROR;
    }
}


/*
**  SetAutoCommit
**
**  Set flags to cause autocommit.
**
**  On entry: pstmt-->statement block to be executed.
**
**  Returns:  TRUE if autocommit=on AND
**                    current statement is commitable (e.g. INSERT, DELETE, etc.) AND
**                    autocommit is not blocked by an open API cursor.
**            FALSE if autocommit=off OR
**                     current statement not commitable (e.g. SELECT) OR
**                     an API cursor is open.
*/
static UWORD      FASTCALL SetAutoCommit(
    LPSTMT  pstmt)
{
    LPDBC   pdbc = pstmt->pdbcOwner;
    int     fCommit = FALSE;

    TRACE_INTL(pstmt,"SetAutoCommit");
    pdbc->sqb.Options = 0;

    if (pdbc->fOptions & OPT_AUTOCOMMIT && pstmt->fCommand & CMD_COMMITABLE
                                        && pdbc->sessMain.XAtranHandle==NULL)
    {
        /* if autocommit is on and a real cursor is open, Ingres does not
           issue a commit until the close cursor stmt is executed */
        fCommit = AnyIngresCursorOpen(pdbc)?FALSE:TRUE;  /* is an Ingres cursor open? */
    }
    return((UWORD)fCommit);
}

/*
**  SetAutoResult
**
**  Set flags resulting from autocommit.
**
**  On entry: pstmt-->statement block.
**            fCommit = commit option selected.
**
**  Returns:  Piggyback commit option flag selected.
*/
static VOID      FASTCALL SetAutoResult(
    LPSTMT  pstmt,
    UWORD   fCommit)
{
    LPDBC   pdbc      = pstmt->pdbcOwner;
    LPSESS  psessCat  = &pdbc->sessCat;
    LPSESS  psessMain = &pdbc->sessMain;

    TRACE_INTL(pstmt,"SetAutoResult");

    if (fCommit)
        SQLTransact_InternalCall(NULL,pdbc,SQL_COMMIT);
    else
    {
        /* if stmt is insert, delete, etc., session needs commit */
        if ( (pstmt->fCommand & CMD_COMMITABLE)  &&
            !(psessMain->fStatus & SESS_AUTOCOMMIT_ON_ISSUED)) /* if API auto-commit=OFF */
                  psessMain->fStatus |= SESS_COMMIT_NEEDED;    /*   then commit needed */
    }
    return;
}

/*
** ODBCDateToIngresUS
**
** This funcitons converts ODBC date format (yyyy-mm-dd hh:mm:ss)
** to Ingres US format (yyyy_mm_dd hh:mm:ss).
**
*/
static UDWORD      FASTCALL ODBCDateToIngresUS(CHAR * rgbData)
{
    CHAR *spacer= "_";

    CMcpychar(spacer, (rgbData + 4));
    CMcpychar(spacer, (rgbData + 7));

    return (CVT_SUCCESS);
}
/*
**  CvtCharToIngresDateTime
**
**  Convert a date/time/timestamp/interval in character string format to 
**  Ingres internal date format.
*/	
static UDWORD FASTCALL CvtCharToIngresDateTime(CHAR * rgbData, LPDESCFLD pipd,
    UDWORD apiLevel)
{
    IIAPI_CONVERTPARM   cv;
    CHAR  szBuf[50]; 
    II_UINT2 dstLen;
    II_INT2  dstDataType;
    II_UINT2 precision=0;
    CHAR *p = NULL;
    
    STcopy(rgbData,szBuf);
    p  = STindex(szBuf, ".", 0);

    if (p)
    {
        CMnext(p);
        precision =  STlength(p);
    }
    switch(pipd->fIngApiType)
    {
    case IIAPI_TIME_TYPE:
    case IIAPI_TMTZ_TYPE:
    case IIAPI_TMWO_TYPE:
        dstDataType = pipd->fIngApiType;
        dstLen = ODBC_TIME_LEN;
        pipd->OctetLength = ODBC_TIME_LEN;
        pipd->Length = ODBC_TIME_LEN;
        break;

    case IIAPI_DTE_TYPE:
        dstDataType = IIAPI_DTE_TYPE;
        dstLen = ODBC_DTE_LEN;
        pipd->OctetLength = ODBC_DTE_LEN;
        pipd->Length = ODBC_DTE_LEN;
        break;
    
    case IIAPI_DATE_TYPE:
        dstDataType = IIAPI_DATE_TYPE;
        dstLen = ODBC_DATE_LEN;
        pipd->OctetLength = ODBC_DATE_LEN;
        pipd->Length = ODBC_DATE_LEN;
        break;
    
    case IIAPI_TS_TYPE:
    case IIAPI_TSTZ_TYPE:
    case IIAPI_TSWO_TYPE:
        dstDataType = pipd->fIngApiType;
        dstLen = ODBC_TS_LEN;
        pipd->OctetLength = ODBC_TS_LEN;
        pipd->Length = ODBC_TS_LEN;
        break;
    
    case IIAPI_INTYM_TYPE:
        dstDataType = IIAPI_INTYM_TYPE;
        dstLen = ODBC_INTYM_LEN;
        pipd->OctetLength = ODBC_INTYM_LEN;
        pipd->Length = ODBC_INTYM_LEN;
        break;

    case IIAPI_INTDS_TYPE:
        dstDataType = IIAPI_INTDS_TYPE;
        dstLen = ODBC_INTDS_LEN;
        pipd->OctetLength = ODBC_INTDS_LEN;
        pipd->Length = ODBC_INTDS_LEN;
        break;
    }
    cv.cv_srcDesc.ds_dataType   = IIAPI_CHA_TYPE; 
    cv.cv_srcDesc.ds_nullable   = FALSE;  
    cv.cv_srcDesc.ds_length     = (II_UINT2) STlength(szBuf);  
    cv.cv_srcDesc.ds_precision  = precision;
    cv.cv_srcDesc.ds_scale      = 0;
    cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
    cv.cv_srcDesc.ds_columnName = NULL; 
    
    cv.cv_srcValue.dv_null      = FALSE; 
    cv.cv_srcValue.dv_length    = (II_UINT2) STlength(szBuf);
    cv.cv_srcValue.dv_value     = szBuf; 
    
    cv.cv_dstDesc.ds_dataType   = dstDataType;
    cv.cv_dstDesc.ds_nullable   = FALSE;
    cv.cv_dstDesc.ds_length     = dstLen;  
    cv.cv_dstDesc.ds_precision  = precision;
    cv.cv_dstDesc.ds_scale      = 0;
    cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
    cv.cv_dstDesc.ds_columnName = NULL;
    
    cv.cv_dstValue.dv_null      = FALSE;
    cv.cv_dstValue.dv_length    = dstLen;
    cv.cv_dstValue.dv_value     = rgbData; 
    IIapi_convertData(&cv);
    
    if (cv.cv_status != IIAPI_ST_SUCCESS)
    {
        return(CVT_ERROR);
    }
    return(CVT_SUCCESS);
}

/*
**  GetBoundDataPtrs
**
**  Get DataPtr, OctetLengthPtr, and IndicatorPtr
**  after adjustment for bind offset, row number,
**  and row or column binding.
**
**  On entry: pstmt           -->statement block.
**            type             = Type_ARD (get column) or Type_APD (put parameter).
**            pard            -->ARD or APD descriptor.
**            pDataPtr        -->where to return bound DataPtr
**            pOctetLengthPtr -->where to return bound OctetLengthPtr
**            pIndicatorPtr   -->where to return bound IndicatorPtr
**
**  Returns:  DataPtr        in pDataPtr
**            OctetLengthPtr in pOctetLengthPtr
**            IndicatorPtr   in pIndicatorPtr
**
**  If multiple parameter values, papd->DataPtr points to an array
**  instead of a single value.  For non-numeric values (which
**  must have explicit lengths) this is an array of strings, which
**  may have explicit lengths or be null-terminated.  In either case
**  the length of the array element is given by cbValueMax (papd->OctetLength).
**  For numeric values (which have implicit lengths) this is an
**  an array of values.
**
*/

VOID GetBoundDataPtrs(
    LPSTMT        pstmt,
    i2            type,
    LPDESC        pARD,
    LPDESCFLD     pard,
    char       ** pDataPtr,
    SQLINTEGER     ** pOctetLengthPtr,
    SQLINTEGER     ** pIndicatorPtr)
{
    char        * DataPtr;
    SQLINTEGER      * OctetLengthPtr;
    SQLINTEGER      * IndicatorPtr;
    SQLUINTEGER       BindOffset;
    SQLUINTEGER       ElementSize;
    SQLUINTEGER       RowIndex;
    SQLUINTEGER     * BindOffsetPtr = pARD->BindOffsetPtr;
                                             /* Pointer to binding offset */

    TRACE_INTL(pstmt,"GetBoundDataPtrs");
    DataPtr        = pard->DataPtr;         /* or  = papd->DataPtr */
    OctetLengthPtr = pard->OctetLengthPtr;  /* or  = papd->OctetLengthPtr */
    IndicatorPtr   = pard->IndicatorPtr;    /* or  = papd->IndicatorPtr   */

    RowIndex  = (type == Type_ARD) ? pstmt->irowCurrent:  /* current get column */
                                     pstmt->irowParm;     /* current put parameter */

    if (pard->BindOffsetPtr)  /* if desc field is protecting a proc parm literal */
       {BindOffsetPtr = pard->BindOffsetPtr;   /* don't use application's array */
        RowIndex = 0;                          /* don't index into arrays       */
       }

    if (type == Type_ARD)                  /* if processing a "get column"    */
    {
        if (RowIndex > 0)    /* if row number is present */
            RowIndex--;      /*  turn row number to row index */
    }
                                    /* add row-wise bind offset */
    if ( BindOffsetPtr  &&          /* if SQL_ATTR_ROW_BIND_OFFSET_PTR */
        *BindOffsetPtr)             /* *SQL_ATTR_ROW_BIND_OFFSET_PTR!=0 */
       {
         BindOffset = *BindOffsetPtr;
                                    /* add SQL_ATTR_ROW_BIND_OFFSET_PTR */
         if (DataPtr)               /*  to SQL_DESC_DATA_PTR */
             DataPtr                       += BindOffset;

         if (OctetLengthPtr)        /*  to SQL_DESC_OCTET_LENGTH_PTR */
             OctetLengthPtr= ADD_INTPTR_AND_OFFSET(
                                              OctetLengthPtr,
                                              BindOffset);
         if (IndicatorPtr)          /*  to SQL_DESC_INDICATOR_PTR */
             IndicatorPtr  = ADD_INTPTR_AND_OFFSET(
                                              IndicatorPtr,
                                              BindOffset);
       }

    if (RowIndex)    /* if 2nd thru nth element in rowset */
       {
        if (pARD->BindType != SQL_BIND_BY_COLUMN) /* ROW-wise bind */
           {
            BindOffset = pARD->BindType * RowIndex;
                                    /* row size from SQL_ATTR_ROW_BIND_TYPE*/
            if (DataPtr)            /*  to SQL_DESC_DATA_PTR */
                DataPtr                       += BindOffset;

            if (OctetLengthPtr)     /*  to SQL_DESC_OCTET_LENGTH_PTR */
                OctetLengthPtr= ADD_INTPTR_AND_OFFSET(
                                                 OctetLengthPtr,
                                                 BindOffset);
            if (IndicatorPtr)       /*  to SQL_DESC_INDICATOR_PTR */
                IndicatorPtr  = ADD_INTPTR_AND_OFFSET(
                                                 IndicatorPtr,
                                                 BindOffset);
           }
        else                                   /* COLUMN-wise bind */
           {
            LPSQLTYPEATTR  ptypeC; /* -->TYPE_TABLE entry for C type */

            ptypeC = CvtGetCType (pard->ConciseType, 
                pstmt->fAPILevel, pstmt->dateConnLevel);
            if (ptypeC  &&  ptypeC->cbSize)
                ElementSize = ptypeC->cbSize;  /* fixed data type len*/
            else 
                ElementSize = pard->OctetLength;
            
            if (DataPtr)               /*  to SQL_DESC_DATA_PTR */
                DataPtr                       += ElementSize * RowIndex;
            if (OctetLengthPtr)        /*  to SQL_DESC_OCTET_LENGTH_PTR */
                OctetLengthPtr=       ADD_INTPTR_AND_OFFSET(
                                                 OctetLengthPtr,
                                                 sizeof(SQLINTEGER) * RowIndex);
            if (IndicatorPtr)          /*  to SQL_DESC_INDICATOR_PTR */
                IndicatorPtr  =       ADD_INTPTR_AND_OFFSET(
                                                 IndicatorPtr,
                                                 sizeof(SQLINTEGER) * RowIndex);
           }
       }  /* end if RowIndex */

    if (pDataPtr)
       *pDataPtr         = DataPtr;
    if (pOctetLengthPtr)
       *pOctetLengthPtr  = OctetLengthPtr;
    if (pIndicatorPtr)
       *pIndicatorPtr    = IndicatorPtr;
}

/*
**  ExecuteDirect
**
**  Execute a statement directly.
**
**  On entry: pstmt-->statement block
**
**  On exit:  Implementation Row Descriptor array is updated.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
**
**  History:
**
**  17-nov-03 (loera01)
**     Extracted from ExecuteStmt().
**
*/

RETCODE ExecuteDirect( LPSTMT pstmt )
{
    LPDBC   pdbc  = pstmt->pdbcOwner;
    LPSESS  psess = pstmt->psess;
    RETCODE rc = SQL_SUCCESS;

    TRACE_INTL (pstmt, "ExecuteDirect");

    pstmt->sqlca.SQLNRP = 0;

    /*
    ** If no query string, return a funcion sequence error.
    */
    if (!pstmt->szSqlStr)             
       return ErrState (SQL_HY010, pstmt); 

    if (pstmt->fCommand == CMD_EXECPROC)
        rc = SqlExecProc (&pdbc->sqb); 
    else
        rc = SqlExecDirect (&pdbc->sqb, pstmt->szSqlStr);

    /*
    ** Indicate that we have started a transaction.  
    */
    psess->fStatus &= ~SESS_SUSPENDED;
    if (pstmt->fCommand & CMD_START)
        psess->fStatus |= SESS_STARTED; /* transaction started */

    if (rc == SQL_ERROR)
    {
        ErrSetToken (pstmt, pstmt->szSqlStr, pstmt->cbSqlStr);
        return rc;
    }

    return rc;
}
/*
**  GetParamValues()
**
**  Get parameter values and descriptor data.
**
**  On entry: pstmt-->statement block
**
**  On exit:  Implementation Row Descriptor array is updated.  Application 
**  data is supplied if it is available.  AT_EXEC (dynamic) parameters will 
**  not yet have data.  Default column lengths are applied to dynamic 
**  parameters.  Other fields in pstmt may get updated.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
**
**  History:
**
**  17-nov-03 (loera01)
**     Extracted from ExecuteStmt().
**
*/

RETCODE GetParmValues (LPSTMT pstmt, LPDESCFLD papd, LPDESCFLD pipd)
{
    RETCODE rc        = SQL_SUCCESS;
    LPDBC   pdbc      = pstmt->pdbcOwner;
    BOOL    fNeedData = FALSE;
    UWORD   fCommit   = 0;
    UWORD   i;
    LPDESC    pAPD     = pstmt->pAPD;       /* appl parm descriptor */
    LPDESC    pIPD     = pstmt->pIPD;       /* impl parm descriptor */

    TRACE_INTL (pstmt, "GetParmValues");
    /*
    ** Loop through parameters (icolParmLast is the count)
    */
    for (i = 0; i < pstmt->icolParmLast; i++, papd++, pipd++)
    {
        char       * rgbValue;
        SQLINTEGER     * pcbValue = NULL;
        SQLINTEGER     * IndicatorPtr;
    
        papd->cbPartOffset = 0;
        papd->fStatus &= (~COL_PASSED & ~COL_PUTDATA); 
    
        /*  
        ** Get DataPtr, OctetLengthPtr, and IndicatorPtr
        ** after adjustment for bind offset, row number,
        ** and row or column binding.  The rgbValue variable
        ** points at papd->DataPtr (orginating data).  pcbValue
        ** points at papd->OctetLengthPtr (application length),
        ** and IndicatorPtr points at the applications "or"
        ** indicator.
        */
    
        GetBoundDataPtrs(pstmt, Type_APD, pAPD, papd,
            &rgbValue, &pcbValue, &IndicatorPtr);
    
        rc = PutParm (pstmt, papd, pipd, rgbValue,  pcbValue,
            IndicatorPtr, TRUE); 

        if (rc == SQL_ERROR)
            return SQL_ERROR;
    
        if (rc == SQL_NEED_DATA)
            pstmt->fStatus |= STMT_NEED_DATA;
    }  /* end for i < pstmt->icolParmLast */
    return rc;
}

BOOL isTypeSupported(SWORD fCType, SWORD fSqlType)
{
    TRACE_INTL(NULL,"isTypeSupported");

    /*
    ** SQL_GUID is unsupported in Ingres.
    */
    if (fSqlType == SQL_GUID || fCType == SQL_C_GUID)
        return FALSE;

    /*
    ** Special case SQL_TYPE_NULL (typeless null) is supported.
    */
    if (fSqlType == SQL_TYPE_NULL)
        return TRUE;

    /*
    ** Coercions supported on all string and binary types except SQL_GUID.
    */
    if ( isCStringType(fCType) )
        return TRUE;

    if (isStringType(fSqlType))
        return TRUE;

    if (isNumberType(fSqlType) && isCNumberType(fCType))
        return TRUE;
        
    if (fCType == SQL_C_BINARY)
        return TRUE;

    /*
    ** Special case coercions for date types.
    */
    switch (fCType)
    {
        case SQL_C_TYPE_DATE:
        case SQL_C_DATE:
            if ( fSqlType == SQL_TYPE_DATE || 
                fSqlType == SQL_DATE ||
                fSqlType == SQL_TYPE_TIMESTAMP_WITH_TIMEZONE ||
                fSqlType == SQL_TYPE_TIMESTAMP )
                return TRUE;
            break;
        case SQL_C_TYPE_TIME:
        case SQL_C_TIME:
            if ( fSqlType == SQL_TYPE_TIME || 
                fSqlType == SQL_TIME ||
                fSqlType == SQL_TYPE_TIME_WITH_TIMEZONE || 
                fSqlType == SQL_TYPE_TIMESTAMP )
                return TRUE;
        case SQL_C_TYPE_TIMESTAMP:
        case SQL_C_TIMESTAMP:
            if ( fSqlType == SQL_TYPE_TIME || 
                fSqlType == SQL_DATE ||
                fSqlType == SQL_TYPE_DATE ||
                fSqlType == SQL_TYPE_TIMESTAMP_WITH_TIMEZONE ||
                fSqlType == SQL_TYPE_TIMESTAMP )
               return TRUE;
        default:
            /*
            ** Fall-though to check for interval types.
            */
            break;
    }
 
    /*
    ** Interval types are compatible within their respective types only.
    */
    if (isYearMonthIntervalType(fSqlType)) 
    {
        if (isCYearMonthIntervalType(fCType))
            return TRUE;
        else if (!isCNumberType(fCType))
            return FALSE;
    }
    if (isDaySecondIntervalType(fSqlType))
    {
        if (isCDaySecondIntervalType(fCType))
            return TRUE;
        else if (!isCNumberType(fCType))
            return FALSE;
    }

    if (isAtomicIntervalType(fSqlType))
    {
        if (isCNumberType(fCType) && fCType != SQL_C_FLOAT &&
            fCType != SQL_DOUBLE)
            return TRUE;
        else
            return FALSE;
    }

    /*
    ** Interval types with only one field set are compatible with
    ** numeric types.  For some odd reason, floating-point numbers are
    ** not included.
    */
    if (isCAtomicIntervalType(fCType))
    {
        if (isNumberType(fSqlType) && fSqlType != SQL_FLOAT &&
            fSqlType != SQL_DOUBLE && fSqlType != SQL_REAL && 
            fSqlType != SQL_BIT)
            return TRUE;
        else
            return FALSE;
    }
    return FALSE;
}
/*
**  SQLDescribeParam
**
**  Describe dynamic parameters for a prepared query.
**
**  On entry: pstmt -->statement block.
**
**  On exit:  The IPD is filled with the parameter information.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/

SQLRETURN SQL_API SQLDescribeParam(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT        *pfSqlType,
    SQLUINTEGER        *pcbParamDef,
    SQLSMALLINT        *pibScale,
    SQLSMALLINT        *pfNullable)
{
    LPSTMT  pstmt = (LPSTMT)hstmt;
    LPDESC  pIPD = pstmt->pIPD;
    LPDESCFLD pipd;
    RETCODE rc = SQL_SUCCESS;
    LPDBC   pdbc      = pstmt->pdbcOwner;

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLDESCRIBEPARAM, pstmt);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    if (!pstmt->szSqlStr)
        return ErrState (SQL_HY010, pstmt);  /* function sequence error*/

    /*
    ** On first iteration, invoke DESCRIBE INPUT.  This will populate the 
    ** IPD with the description of the colums targeted by the prepared query.
    */
    if (!(pstmt->fStatus & STMT_PARAMS_DESCRIBED))
    {
        /*
        **  Initialize the SQL control block anchor
        */
        pdbc->sqb.pSqlca = &pstmt->sqlca;
        pdbc->sqb.pStmt = pstmt;

        rc = SqlDescribeInput (&pdbc->sqb);
        pstmt->fStatus |= STMT_PARAMS_DESCRIBED;
        if (!SQL_SUCCEEDED(rc))
            return rc;
    }

    pipd = pIPD->pcol + ipar;
    if (pfSqlType)
        *pfSqlType = pipd->ConciseType;

    if (pcbParamDef)
    {
        *pcbParamDef = pipd->OctetLength;

        switch (pipd->ConciseType)
        {
            case SQL_VARCHAR:
            case SQL_VARBINARY:
            case SQL_WVARCHAR:
                *pcbParamDef -= 2;
                break;

            default:
                break;
        }
    }

    if (pibScale)
        *pibScale = pipd->Scale;
    if (pfNullable)
        *pfNullable = pipd->Nullable;
    
    UnlockStmt (pstmt);
    return rc;
}

BOOL isStringType(SWORD type)
{
    switch(type)
    {
        case SQL_CHAR: 
        case SQL_VARCHAR: 
        case SQL_LONGVARCHAR: 
        case SQL_WCHAR: 
        case SQL_WVARCHAR: 
        case SQL_WLONGVARCHAR: 
            return TRUE; 
        default: 
            return FALSE; 
    }
}
BOOL isNumberType(SWORD type)
{
    switch(type)
    {
        case SQL_DECIMAL: 
        case SQL_BIT: 
        case SQL_NUMERIC: 
        case SQL_TINYINT: 
        case SQL_SMALLINT: 
        case SQL_INTEGER: 
        case SQL_BIGINT: 
        case SQL_REAL: 
        case SQL_FLOAT: 
        case SQL_DOUBLE: 
            return TRUE; 
        default: 
            return FALSE; 
     }
}       
BOOL isBinaryType(SWORD type) 
{
    switch(type) 
    { 
        case SQL_BINARY: 
        case SQL_VARBINARY: 
        case SQL_LONGVARBINARY: 
            return TRUE; 
        default: 
            return FALSE; 
    } 
}
BOOL isYearMonthIntervalType(SWORD type)
{
    switch(type)
    {
        case SQL_INTERVAL_YEAR_TO_MONTH: 
        case SQL_INTERVAL_YEAR: 
	case SQL_INTERVAL_MONTH: 
            return TRUE; 
        default: 
            return FALSE; 
    }
}
BOOL isCYearMonthIntervalType(SWORD type)
{
    switch(type)
    {
        case SQL_C_INTERVAL_YEAR_TO_MONTH: 
        case SQL_C_INTERVAL_YEAR: 
        case SQL_C_INTERVAL_MONTH: 
            return TRUE; 
        default: 
            return FALSE; 
     }
}
BOOL isDaySecondIntervalType(SWORD type)
{
    switch(type)
    {
        case SQL_INTERVAL_DAY_TO_SECOND: 
        case SQL_INTERVAL_DAY: 
        case SQL_INTERVAL_DAY_TO_HOUR: 
        case SQL_INTERVAL_DAY_TO_MINUTE: 
        case SQL_INTERVAL_HOUR: 
        case SQL_INTERVAL_HOUR_TO_MINUTE: 
        case SQL_INTERVAL_HOUR_TO_SECOND: 
        case SQL_INTERVAL_MINUTE: 
        case SQL_INTERVAL_MINUTE_TO_SECOND: 
        case SQL_INTERVAL_SECOND: 
            return TRUE; 
        default: 
            return FALSE; 
    }
}
BOOL isCDaySecondIntervalType(SWORD type)
{
    switch(type)
    {
        case SQL_C_INTERVAL_DAY_TO_SECOND: 
        case SQL_C_INTERVAL_DAY: 
        case SQL_C_INTERVAL_DAY_TO_HOUR: 
        case SQL_C_INTERVAL_DAY_TO_MINUTE: 
        case SQL_C_INTERVAL_HOUR: 
        case SQL_C_INTERVAL_HOUR_TO_MINUTE: 
        case SQL_C_INTERVAL_HOUR_TO_SECOND: 
        case SQL_C_INTERVAL_MINUTE: 
        case SQL_C_INTERVAL_MINUTE_TO_SECOND: 
        case SQL_C_INTERVAL_SECOND: 
            return TRUE; 
        default: 
            return FALSE; 
     }
}
BOOL isAtomicIntervalType(SWORD type)
{
    switch(type)
    {
        case SQL_INTERVAL_YEAR: 
        case SQL_INTERVAL_MONTH: 
        case SQL_INTERVAL_DAY: 
        case SQL_INTERVAL_MINUTE: 
        case SQL_INTERVAL_SECOND: 
            return TRUE; 
        default: 
            return FALSE; 
    }
}
BOOL isCAtomicIntervalType(SWORD type)
{
    switch(type)
    {
        case SQL_C_INTERVAL_YEAR: 
        case SQL_C_INTERVAL_MONTH: 
        case SQL_C_INTERVAL_DAY: 
        case SQL_C_INTERVAL_HOUR: 
        case SQL_C_INTERVAL_MINUTE: 
        case SQL_C_INTERVAL_SECOND: 
            return TRUE; 
        default: 
            return FALSE; 
     }
}
BOOL isCStringType(SWORD type)
{
    switch(type)
    {
        case SQL_C_CHAR: 
        case SQL_C_WCHAR: 
            return TRUE; 
        default: 
            return FALSE; 
    } 
}
BOOL isCNumberType(SWORD type)
{
    switch(type)
    {
        case SQL_C_NUMERIC: 
        case SQL_C_STINYINT: 
        case SQL_C_UTINYINT: 
        case SQL_C_TINYINT: 
        case SQL_C_SBIGINT: 
        case SQL_C_UBIGINT: 
        case SQL_C_SSHORT: 
        case SQL_C_USHORT: 
        case SQL_C_SHORT: 
        case SQL_C_SLONG: 
        case SQL_C_ULONG: 
        case SQL_C_LONG: 
        case SQL_C_FLOAT: 
        case SQL_C_DOUBLE: 
        case SQL_C_BIT: 
            return TRUE; 
        default: 
            return FALSE; 
    }
}

/*
** Name: odbc_hexdump
**
** Description:
**      Traces a buffer in hex and ascii.
**
** Input:
**      buff            Data buffer.
**      len             Data length.
**
** Output:
**      None.
**
** Returns:
**      VOID.
**
** History:
**      22-Mar-2010 (Ralph Loen) Bug 123458
**          Cloned from gcu_hexdump() as coded by Gordy.  
**
*/
VOID odbc_hexdump( char *buff, i4 len )
{
    char        *hexchar = "0123456789ABCDEF";
    char        hex[ 49 ];
    char        asc[ 17 ];
    u_i2        i;

    for( i = 0; len-- > 0; buff++ )
    {
        hex[ 3 * i ] = hexchar[ (*buff >> 4) & 0x0f ];
        hex[ 3 * i + 1 ] = hexchar[ *buff & 0x0f ];
        hex[ 3 * i + 2 ] =  ' ';
        asc[ i++ ] = (CMprint( buff )  &&  ! (*buff & 0x80)) ? *buff : '.' ;

        if ( i == 16  ||  ! len )
        {
            hex[ 3 * i ] = asc[ i ] = '\0';
            TRdisplay( "%48s    %s\n", hex, asc );
            i = 0;
        }
    }

    return;
}
