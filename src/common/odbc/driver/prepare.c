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
#include <bt.h>
#include <cm.h>
#include <cv.h>
#include <me.h>
#include <st.h> 
#include <er.h>
#include <tr.h>
#include <erodbc.h>
#include <iiapi.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include <idmsocvt.h>               /* ODBC conversion routines */

/*
** Name: PREPARE.C
**
** Description:
**	Prepare statement routines for ODBC driver.
**
** History:
**	03-dec-1992 (rosda01)
**	    Initial coding
**	14-mar-1995 (rosda01)
**	    ODBC 2.0 upgrade...
**	10-jan-1997 (tenje01)
**	    Initialize stmtHandle
**	14-jan-1997 (tenje01)
**	    Modified SQLFreeStmt  
**	14-jan-1997 (tenje01)
**	    Conver ? to ~V 
**	16-jan-1997 (tenje01)
**	    put current pstmt in sqb
**	07-feb-1997 (tenje01)
**	    SetColVar ...
**	07-feb-1997 (tenje01)
**	    SetColVar, scale/precision not applicable to ingres date    
**	10-feb-1997 (tenje01)
**	    using sql data type in SetColOffset 
**	10-feb-1997 (tenje01)
**	    using sql data type in PrepareMaxLen
**	10-feb-1997 (tenje01)
**	    using api data type in PrepareSqlda
**	12-feb-1997 (tenje01)
**	    modified DBC_IDMS to Ingres specific
**	12-feb-1997 (thoda04)
**	    Added code for catalog char-->varchar convert    
**	13-feb-1997 (tenje01)
**	    PrepareMaxLen ... cfde is 0 in Ingres
**	13-feb-1997 (tenje01)
**	    Modified SetColVar and SetColOffset 
**	19-feb-1997 (thoda04)
**	    Added code for catalog int4-->int2 convert
**	25-feb-1997 (tenje01)
**	    Removed case TOK_ESC_TIMESTAMP in ParseConvert
**	27-feb-1997 (tenje01)
**	    Added code for calling a procedure
**	04-mar-1997 (thoda04)
**	    Bug fixes to DB2 SQLColumns support
**	04-mar-1997 (thoda04)
**	    Added SQLStatistics conversions support
**	11-mar-1997 (tenje01)
**	    calling a procedure using std odbc escape clause
**	20-mar-1997 (tenje01)
**	    ParseCommand ... looking for "where current of"
**	21-mar-1997 (thoda04)
**	    Bug fix for SQLPrepare passing SQL_NTS 
**	27-mar-1997 (thoda04)
**	    Removed CASQLSets baggage. 
**	09-apr-1997 (tenje01)
**	    free long data buffer if allocated
**	14-apr-1997 (thoda04)
**	    Convert int literals back to int from float.
**	22-apr-1997 (tenje01)
**	    ParseConvert ... to support scalar fn
**	02-may-1997 (tenje01)
**	    added support of outer joins
**	08-may-1997 (thoda04)
**	    Trap SET AUTOCOMMIT ON/OFF.
**	21-may-1997 (tenje01)
**	    Use dbmsinfo('username') instead of the constant user
**	22-may-1997 (tenje01)
**	    added workarounds in SQLBindParameter for gateway problems  
**	22-may-1997 (tenje01)
**	    added checks in ParseConvert for unsupported escape 
**	06-jun-1997 (tenje01)
**	    cvt decimal to float in SQLBindParameter since db2 gateway
**	    does not support decimal 
**	13-jun-1997 (tenje01)
**	    set sqllen to 19 if db2  
**	16-jun-1997 (tenje01)
**	    save cursor name of "update ... where current of c1"
**	17-jun-1997 (tenje01)
**	    remove my 6 changes. DB2 data type date and time are
**	    converted to char by the gateway. If app accesses a db2
**	    table (not created by the gateway) having a true db2 date
**	    data type, then the trailing zeroes added by our driver
**	    will cause error. 
**	19-jun-1997 (tenje01)
**	    db2 does not have db procedures
**	20-jun-1997 (tenje01)
**	    removed 6 changes
**	07-jul-1997 (tenje01)
**	    rc from SqlPrepare was lost after PrepareSqlda
**	28-jul-1997 (thoda04)
**	    Added support for cRowsetSize
**	31-jul-1997 (thoda04)
**	    Support for Desktop outer join
**	11-aug-1997 (tenje01)
**	    To issue a msg if "copy table" otherwise OpenAPI went into a loop
**	02-sep-1997 (tenje01)
**	    error in PrepareSqlda if number of columns described > 300 
**	08-sep-1997 (tenje01)
**	    do not alloc pstmt->pParmBuffer
**	17-sep-1997 (thoda04)
**	    Trap COMMIT or ROLLBACK and call SQLTransact instead
**	19-sep-1997 (tenje01)
**	    in char string was misinterpreted as parm marker
**	12-oct-1997 (thoda04)
**	    Bump up pcol ptr when deallocating ParmBuffers
**	31-oct-1997 (tenje01)
**	    Set command not processed correctly  
**	11-nov-1997 (thoda04)
**	    Documentation clean-up
**	25-nov-1997 (thoda04)
**	    Support of multiple cursors if AUTOCOMMIT=ON
**	04-dec-1997 (tenje01)
**	    conver C run-time functions to CL
**	12-dec-1997 (thoda04)
**	    Convert three-part colnames to two-part names
**	31-dec-1997 (thoda04)
**	    When scanning search condition, check all of "HAVING"
**	05-jan-1998 (thoda04)
**	    When ConvertParmMarkers, check the length 
**	14-jan-1998 (thoda04)
**	    Added RMS server class  
**	06-feb-1998 (thoda04)
**	    Fixed sizes from 16-bit to 32-bit for longchar, longbyte
**	17-feb-1998 (thoda04)
**	    Added cursor SQL_CONCURRENCY (fConcurrency)
**	27-feb-1998 (thoda04)
**	    Call GetTls for thread local storage
**	22-may-1998 (thoda04)
**	    SQLFreeStmt-support prepare/describecol/close/execute
**	08-jun-1998 (Dmitriy Bubis)
**	    Ignore milliseconds in TIME/TIMESTAMP 
**	09-jun-1998 (thoda04)
**	    Convert yyyy-mm-dd to yyyy_mm_dd in ParseConvert 
**	17-jul-1998 (Dmitriy Bubis)
**	    Blank out '9999-12-31' date value
**	21-jul-1998 (thoda04)
**	    Added cBindType, cKeysetSize, cRetrieveData.
**	06-oct-1998 (Dmitriy Bubis)
**	    Re-prepare on the second execute by turning off STMT_PREPARED,
**	    STMT_DESCRIBED, STMT_EXECUTE flags until we rewrite 
**	    prepare/execute.
**	23-oct-1998 (Dmitriy Bubis)
**	    When you have a decimal column with even precision don't return
**	    incremented column size to account for an extra sign digit of
**	    PACKED DECIMAL. (Bug 94000)
**	    Removed IIAPI_DEC_TYPE, JDT_NUM, JDT_UNUM, JDT_DEC,
**	    JDT_UDEC cases.
**	30-dec-1998 (Bobby Ward)
**	    SQLFreeStmt-> Don't do a commit for main connection; 
**	    SQLFreeStmt should free the statement but not commit the
**	    transaction. Bug 94064
**	05-jan-1998 (thoda04)
**	    Rewrite of SQLPrepare/SQLExecute sequence.
**	06-jan-1999 (thoda04)
**	    Moved sqldaDefault from ENV to DBC for thread-safeness.
**	07-jan-1999 (thoda04)
**	    Make sure the SQL control block anchor is initialized.
**	20-jan-1999 (thoda04)
**	    Auto-commit updates at SQLFreeStmt(SQL_CLOSE) if needed.
**	24-mar-1999 (thoda04)
**	    Port to UNIX.
**	22-apr-1999 (thoda04)
**	    Add prepare_or_describe parm to PrepareStmt.
**	04-jun-1999 (Bobby Ward)
**	    Fixed Bug 97193 SQLFreeSTMT (SQL_RESET_PARAMS)  
**	05-aug-1999 (thoda04)
**	    Convert nested escape sequences also (b98272).
**	01-sep-1999 (thoda04)
**	    When autocommit=on and closing last cursor, always commit
**	    to also release locks from from select statements.
**	01-oct-1999 (Bobby Ward)
**	    Fixed Bug#98156 Odbc Driver Crashes with long "QUERY" that
**	    was more than 500 chars in length. 
**	02-nov-1999 (thoda04)
**	    Raise limit of size of fetch buffer 
**	    to 2G (32K for WIN16). Removed unneeded PrepareMaxLen().
**	21-dec-1999 (thoda04)
**	    SET AUTOCOMMIT OFF was missing a break
**	13-mar-2000 (thoda04)
**	    Add support for Ingres Read Only.
**	20-apr-2000 (loera01)
**	    Bug 101292:  In ParseToken(), made table
**	    search case-insensitive, and removed
**	    TOK_ESC_CALLUPPER from the token table.
**	01-may-2000 (loera01)
**	    Bug 100747: Replaced check for "INGRES" in the registry with
**	    isServerClassINGRES macro, so that user-defined server classes
**	    are interpreted as Ingres classes.
**	08-may-2000 (thoda04)
**	    Initialize pdbc field in SQLCA
**	15-may-2000 (thoda04)
**	    Mark what stmts can be PREPAREd/EXECUTEd
**	07-jun-2000 (thoda04)
**	    Added ScanNumber
**	28-jul-2000 (loera01)
**	    Bug 102209: Added check for server version when preparing
**	    a statement with DECIMAL or NUMERIC datatypes, so that the
**	    datatype is treated as FLOAT if the server is 6.4.
**	11-aug-2000 (thoda04)
**	    Bump safety limit of buffer size to 2G
**	07-sep-2000 (thoda04)
**	    Remove SQL_API attribute from
**	    SQLAllocStmt_Common and SQLFreeStmt_Common
**	07-nov-2000 (thoda04)
**	    Convert nat's to i4
**	11-dec-2000 (weife01)
**	    Bug 102162: Added new internal routine CountQuestionMark()
**	    to assist in proper allocation of space for queries with
**	    parameters.                           
**	11-jan-2001 (thoda04)
**	    Integrate MTS/DTC support into 2.8
**	29-may-2001 (thoda04)
**	    Upgrade to 3.5 and remove use of thread local storage
**	03-jul-2001 (thoda04)
**	    Clear IRD in ResetStmt( )
**	12-jul-2001 (somsa01)
**	    Fixed up compiler warnings.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-dec-2001 (thoda04)
**	    Additional 64-bit changes for overloaded attributes.
**	11-mar-2002 (thoda04)
**	    Add support for row-producing procedures.
**	01-feb-2001 (thoda04)
**	    Wrap {ts ...} in date scalar function for gateways
**	    Leave {d ...} in yyyy-mm-dd format for gateways
**      05-jun-2002 (loera01) Bug 107949
**          In SQLBindParameter(), make sure the ParameterType field in
**          the Application Row Descriptor is set per the user request.
**     18-jul-2002 (loera01) Bug 108301
**          In  FreeStmtBuffers(), free space from any blob buffers in the
**          ARD or IRD, if allocated.  The long buffer structure in no
**          longer in the statement control block (tSTMT).
**     04-sep-2002 (loera01) Bug 108634
**          Ignore precision settings in SQLBindParameter if the ODBC
**          data type is SQL_TYPE_TIMESTAMP.
**      11-oct-2002 (loera01) Bug 108819
**          For FreeStmt(), clear the STMT_PREPARED flag in the statement
**          handle's statement block if an API cursor was closed. 
**      04-nov-2002 (loera01) Bug 108536
**          Second phase of support for SQL_C_NUMERIC.  Adjusted the treatment
**          of precision to allow more than 31 digits.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      02-mar-2003 (weife01) Bug 109746
**          Driver now scans for ODBC escape sequence in all queries unless
**          OPT_NOSCAN is set    
**      02-jun-03 (loera01) SIR 109643
**          Move ScanPastSpaces() template to idmseini.h.
**      06-may-2003 (weife01) Bug 109945, 110148
**          Reset date, time, timestamp to 19 for sqllen(109945)
**          reset sqltype(110148)
**     22-sep-2003 (loera01)
**          Added internal ODBC tracing.
**     26-sep-2003 (weife01) Bug 110930
**          Fix most of scalar function related to date in escape format.
**     17-oct-2003 (loera01)
**          Added new SqlToIngresApi() case SqlExecDirect.  Changed
**          "IIsyc_" routines to "odbc_".
**     20-oct-2003 (loera01)
**          Moved SqlPrepare() to SQLPrepare() instead of 
**          PrepareStmt().
**     05-nov-2003 (loera01)
**          Sending parameter descriptors is now invoked from
**          SQLExecute() or SQLExecDirect().
**     13-nov-2003 (loera01)
**          Added capability to send true segments.  A key change
**          is in PrepareParms(): if a parameter is found to be
**          an AT_EXEC (dynamic) parameter, memory allocation is 
**          deferred to SQLPutData().
**     20-nov-2003 (loera01)
**          Removed pParmBuffer from sTMT structure.
**    04-dec-2003 (loera01)
**          Changed "STMT_" command status to "CMD_".
**    18-dec-2003 (loera01)
**          Don't do an MEfree on plb->pLongData if the buffer was 
**          allocated by the application.
**    16-jan-2004  (loera01) Bug 111262
**          In FreeStmt(), do not invoke SqlClose if a result set was not
**          opened.
**    10-feb-2004 (loera01) SIR 111427
**          Added removeEndSemicolon().
**    24-mar-2004 (loera01)  Bug 112019
**          Removed redundant call to SetTransaction().
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**    12-may-2004 (loera01)
**          In GenerateCursor(), make sure cursor names are unique by
**          appending the hex value of the statement handle as a string.
**	11-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**     15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
**    18-nov-2004 (Ralph.Loen@ca.com) Bug 113503
**          In ParseCommand(), flag all DDL queries with STMT_CANBEPREPARED
**          flag.  Include rollback, commit, and suspend.
**    10-dec-2004 (Ralph.Loen@ca.com) SIR 113610
**          Added support for typeless nulls.
**    04-feb-2005 (weife01) SIR 113838
**          Added support for DCOM, DB2 and IDMS for outer
**          join escape sequence.
**    27-apr-2005 (loera01) Bug 114418
**          In FreeStmtBuffers(), don't clear the STMT_PREPARED flag.
**    15-apr-2005 (lunbr01,loera01) Bug 114319
**          Increase size of prepared statement id (PSID) cache from 32 
**          to 256 PSIDs; use CL facility BT to handle bit manipulation
**          since current logic would only work up to 32 bits. Also, if 
**          cache is full (all PSIDs used), increment PSIDs thereafter
**          based on connection high PSID rather than assigning duplicate
**          PSID based on statement cursor id.  Similar change for 
**          assignment of next cursor id.
**    08-jun-2005 (loera01) Bug 114631
**          In ParseCommand(), check for the TOK_MODIFY token and process
**          as for TOK_ALTER and TOK_DROP.
**    20-sep-2005 (weife01) Bug 115207
**          In convertDateFn and convertDateTs, no longer use work buffer    
**          MAX_STMT_LEN. Use actual length of the string plus 16 as safe
**          padding.
**    18-oct-2004 (loera01) Bug 115417
**          Expand ParseSpace() to optionally avoid stripping off <CR>, 
**          <LF>, or <TAB> "white" characters.
**    01-dec-2005 (loera01) Bug 115358
**          Added argument to SQLPrepare_InternalCall() so that the statement
**          handle buffers aren't freed when a statement is re-prepared.
**    20-jun-2006 (Ralph Loen) SIR 116275
**          In ParseConvert(), add support for CONVERT function scalars.
**    29-jun-2006 (Ralph Loen) SIR 116319
**          Add support for INTERVAL escape sequence syntax.  Since
**          INTERVAL is converted to the interval() scalar, only atomic
**          interval types are supported.
**   21-jul-06 (Ralph Loen)  SIR 116410
**          Add support for SQLColumnPrivileges().  In AllocStmt(), set
**          count of column privileges list to zero.  In FreeStmtBuffers(), 
**          empty the column privileges queue if present.
**   26-jul-2006 (Ralph Loen) SIR 116417
**          In AllocStmt(), set count of table privileges to zero.  In
**          FreeStmtBuffers(), empty the table privilege queue, if present.
**   10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase).
**   16-Aug-2006 (Ralph Loen) SIR 116477 (second phase)
**          In PutParm(), update verbose and concise types for default
**          C parameters.
**   28-Sep-2006 (Ralph Loen) SIR 116477
**          Adjusted treatment of ISO date/times according to new API/ADF rules.
**   03-Oct-2006 (Ralph Loen) SIR 116477
**          Dynamic parameters for dates and timestamps are now sent as binary
**          values for backward-compatibility with ingresdate.
**   23-Oct-2006 (Ralph Loen SIR 116477
**          In ConvertEscAndMarkers(), make sure query strings are terminated
**          properly.
**   10-Nov-2006 (Ralph Loen SIR 116477
**          In ParseConvert(), rewrite treatment of INTERVAL escape sequences
**          for literal parameters.  
**    21-Aug-2007 (Ralph Loen) Bug 118993
**          Remove references to unused "pcrowParm" field in the
**          statement handle.  
**   19-Oct-2007 (weife01) BUG 119274
**          Driver no longer converts decimal binded param if backend is 
**          gateway
**   09-Jan-2008 (Ralph Loen) SIR 119723
**          In AllocStmt(), add initilization for new scrollable cursor
**          attributes.
**   23-Jan-2008 (Ralph Loen) Bug 119800
**          Added F_OD0169_IDS_ERR_PREP_UCURS error for a more meaningful
**          error message if the application attempted to prepare an update
**          statement using an updatable cursor.
**   30-Jan-2008 (Ralph Loen) SIR 119723
**          Added tSTMT fields fCurrentCursPosition fCursorPosition,
**          and fCursLastResSetSize.
**   07-Feb-2008 (Ralph Loen) SIR 119723
**          Obsoleted cRowsetsize and cKeysetsize in tDBC. 
**   25-Feb-2007 (Ralph Loen) Bug 119975
**          In ConvertEscAndMarkers(), allow for embedded braces.  Retain 
**          braces if the escape sequence is not date/time or interval.
**   27-Feb-2007 (Ralph Loen) Bug 119730
**          In SQLBindParameter(), include SQL_C_WCHAR as a data type of
**          variable length.
**   08-May-2008 (Ralph Loen) SIR 120340
**          In SQLPrepare_InternalCall(), allocate szPSqlStr and set cbPSqlStr 
**          in pstmt if the statement cannot be prepared.  SQLExecute() will 
**          detect this and invoke the statement via 
**          SQLExecDirect_InternalCall().   Free szPSqlStr in FreeStmt().
**   12-May-2008 (Ralph Loen) Bug 120356
**          In SQLBindParameter(), treat SQL_C_DEFAULT as SQL_C_CHAR if
**          coercion to Unicode is specified.
**   21-May-2008 (Ralph Loen) Bug 120412
**          In SQLPrepare_InternalCall(), release the statement handle before 
**          returning if a non-preparable query is detected.
**   08-Aug-2008 (Ralph Loen) Bug 120755
**          In SQLBindParameter(), don't return F_OD0075_IDS_ERR_TYPE_NONE
**          if the ODBC type is SQL_C_LONGVARCHAR, SQL_C_LONGVARBINARY, or
**          SQL_C_WLONGVARCHAR.  Some gateways do support long data.
**   12-Aug-2008 (Ralph Loen) Bug 120534
**          In PrepareParms(), allow for length indicator settings of
**          NULL when determining whether to allocate pipd->DataPtr.
**   20-Aug-2008 (Ralph Loen) Bug 120795
**          In FreeStmtBufers(), reference only pointers to IPD 
**          structures that are known to be allocated.
**   06-Oct-2008 (Ralph Loen) SIR 121019
**          In ParseCommand(), set STMT_RETPROC if the syntax "{ ? = call"
**          is detected.  Clear STMT_RETPROC in ResetStmt().
**   14-Nov-2008 (Ralph Loen) SIR 121228
**         Add support for OPT_INGRESDATE configuration parameter.  If set,
**         ODBC date escape syntax is converted to ingresdate syntax, and
**         parameters converted to IIAPI_DATE_TYPE are converted instead to
**         IIAPI_DTE_TYPE.
**   17-Nov-2008 (Ralph Loen) SIR 121228
**         In ConvertEscAndMarkers(), the previous approach rewrote szSqlStr, 
**         which caused a segmentation violation on Linux.  The szAPISqlStr 
**         string is written instead.  The szAPISqlStr string is allocated
**         from the ODBC driver rather than the calling application.
**   19-Nov-2008 (Ralph Loen) SIR 121228
**         In some cases in ParseConvert, dateConnLevel was being examined for 
**         data conversion where fAPILevel was more appropriate, such as for
**         ANSI timestamps.  At present, the intent is to confine 
**         OPT_INGRESDATE support only to dates.
**   12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**   05-Jan-2009 (Ralph Loen) Bug 121358
**         In FreeStmt(), set icolFetch to zero.
**   26-Jan-2009 (Ralph Loen)  Bug 121556
**         In AllocStmt(), set fCursorDirection to SQL_FETCH_NEXT and 
**         fCursorSCrollable to SQL_SCROLLABLE.
**     12-Feb-2009 (Ralph Loen) bug 121649
**         Added boolean argument rePrepared to ParseCommand(), which
**         doesn't create a new copy of szSqlStr if set to TRUE.  In
**         ResetStmt(), moved invocation of FreeStmtBuffers(), 
**         ResetDescriptorFields(), and initialization of pIRD->Count to
**         general execution code instead of conditional upon the 
**         fFree boolean argument.
**  07-May-2009 (Ralph Loen) Bug 122038
**         In ConvertEscAndMarkers(), allow for single-quoted literals. 
**  20-May-2009 (Ralph Loen)  Bug 122095
**         In AllocStmt(), default of fCursorDirection is SQL_NONSCROLLABLE
**         for all connection levels.
**     03-Jun-2009 (Ralph Loen) Bug 122135
**         In SQLPrepare_internalCall(), added missing call to
**         UnlockStmt().
**   15-Sep-2009 (Ralph Loen) Bug 122592
**         In ResetStmt(), restore call of ResetDescriptorFields()
**         and initialization of pIRD->Count, coditional on autocommit
**         settings.  This is a regression introduced by the fix for 
**         bug 121649.
**     18-Sep-2009 (Ralph Loen, Dave Thole) Bug 122603
**         With the increase of the MAX_COLUMNS value
**         increase the safety margin of allocated SQL statement
**         buffer for possible expansion of the SQL statement
**         due to expansion of { fn ... } function to Ingres syntax.
**    22-Sep-2009 (hanje04) 
**	    Define null terminator, nt to pass to CMcpchar() instead of
**	    literal "\0" because it causes relocation errors when linking
**	    64bit intel OSX
**  15-Oct-2009 (Ralph Loen) Bug 122710 
**         The fix for bug 122038 failed to account for '?' literals 
**         following a date/time escape sequence. ConvertEscAndMarkers()
**         now includes a check for date/time escape sequences in order to
**         correctly mark the beginning of a single-quoted literal.
**  10-Nov-2009 (Ralph Loen) Bug 122866
**         Removed rePrepare arguments from ParseCommand() and 
**         SQLPrepare_InternalCall().  
**  19-Nov-2009 (Ralph Loen) Bug 122941
**         Re-instate rePrepare arguments from ParseCommand() and 
**         SQLPrepare_InternalCall().  Initialize reExecuteCount in
**         SQLPrepare_InternalCall().
**  29-Jan-2010 (Ralph Loen) Bug 123175
**         In ConvertEscAndMarkers(), add blank date support for timestamps.
**  25-Jun-2010 (Ralph Loen)    
**         In ConvertEscAndMarkers(), don't insert the INTERVAL keyword into
**         the string literal for interval escape sequences.        
**    14-Jul-2010 (Ralph Loen)  Bug 124079
**         In ParseConvert(), converted date/time scalars to use 
**         ISO syntax for targets supporting ISO syntax (IIAPI_LEVEL_3 or
**         later).  The exception is DAYNAME() for Vectorwise, which is 
**         supported via the non-ISO scalar dow(). 
**         Some of the ISO scalars are fairly large, which requires
**         bulletproofing, to make sure the converted scalar fits in the
**         query and does not overwrite existing query info.  Thus,
**         escape sequences are re-written so that only single spaces are
**         allowed between the brackets when spaces exist.  All queries now
**         use CountLiteralItems() to advise on required memory, instead of
**         a fudge factor of 1,000 bytes.  CountLiteralItems() was re-written
**         to allow for the largest scalar--currently 550 for DAYOFWEEK().
**         Added support for TIMESTAMPDIFF(), MONTHNAME() and EXTRACT().
**   19-Jul-2010 (Ralph Loen) Bug 124101
**         Added support for comment for slash-asterisk comment delimiters.
**   03-Aug-2010 (Ralph Loen) Bug 124079
**         Revised DAYOFYEAR() to include adjustments for leap years in the
**         Gregorian calendar.  Revised CountLiteralItems to look for
**         the DAYOFYEAR(), DAYOFMONTH() and MONTHNAME() scalars, and
**         calculate the buffer length depending on the scalar and the
**         length of the referenced column name.
**   03-Sep-2010 (Ralph Loen) Bug 124348
**         Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**         SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**         platforms.
*/

typedef enum
{
    TOK_VOID,
    TOK_ALL,
    TOK_AUTOCOMMIT,
    TOK_ALTER,
    TOK_BEGIN_COMMENT,
    TOK_COMMIT,
    TOK_COPY,
    TOK_CREATE,
    TOK_DELETE,
    TOK_DROP,
    TOK_ESC_DATE,
    TOK_ESC_ESCAPE,
    TOK_ESC_FUNCTION,
    TOK_ESC_INTERVAL,
    TOK_ESC_OJ,
    TOK_ESC_TIME,
    TOK_ESC_TIMESTAMP,
    TOK_ESC_BRACE,
    TOK_ESC_CALL,
    TOK_ESC_BRACECALL,
    TOK_ESC_TILDEV,
    TOK_END_COMMENT,
    TOK_GRANT,
    TOK_IDENTIFIER,
    TOK_IDMS,
    TOK_INDEX,
    TOK_INSERT,
    TOK_MODIFY,
    TOK_ODBC,
    TOK_OFF,
    TOK_ON,
    TOK_READ,
    TOK_ROLLBACK,
    TOK_SELECT,
    TOK_SESSION,
    TOK_SET,
    TOK_SUSPEND,
    TOK_TRACE,
    TOK_TRANSACTION,
    TOK_UPDATE,
    TOK_VIEW,
    TOK_WRITE
} TOKEN;

typedef struct
{
    SQLCHAR sqlType[25];
    SQLCHAR scalar[25];
}  SCALAR_TYPE;

SCALAR_TYPE sqlToScalarTbl[] = 
{
    { "SQL_BIGINT",	"int8" },
    { "SQL_BINARY",	"byte" },
    { "SQL_CHAR",	"char" },
    { "SQL_DECIMAL","decimal" },
    { "SQL_DOUBLE",	"float8" },
    { "SQL_FLOAT",	"float4" },
    { "SQL_INTEGER","int2" },
    { "SQL_LONGVARBINARY", "long_varbyte" },
    { "SQL_LONGVARCHAR", "long_varchar" },
    { "SQL_NUMERIC", "decimal" },
    { "SQL_REAL", "float4" },
    { "SQL_SMALLINT", "int2" },
    { "SQL_DATE", "date" },
    { "SQL_TIME", "date" },
    { "SQL_TIMESTAMP", "date" },
    { "SQL_TINYINT", "int1" },
    { "SQL_VARBINARY", "varbyte" },
    { "SQL_VARCHAR", "varchar" },
    { "SQL_WCHAR", "nchar" },
    { "SQL_WVARCHAR", "nvarchar" }
};

#define sqlToScalarTblMax sizeof(sqlToScalarTbl) / sizeof(sqlToScalarTbl[0])

#define SQLDA_FDE_LEN   98
#define DATE_PART_LEN   17
#define DATE_MAX_LEN    26
/*
**  Internal function prototypes:
*/
static  LPSTR       ParseEscapeStart (LPSTR szSqlStr);
static  LPSTR       ParseEscapeEnd   (LPSTR szSqlStr);

static VOID     ParseSpace    (LPSTR, SWORD);
RETCODE         ParseTrace    (LPSTMT);
static TOKEN    ParseToken    (LPSTR);
RETCODE         SetColVar     (LPSTMT, LPDESCFLD papd, LPDESCFLD pipd);

static const char ODBC_STD_ESC[] = "--(*VENDOR(Microsoft),PRODUCT(ODBC)";
static BOOL removeEndSemicolon(LPSTR szSqlStr);
static void editComment(char **pFrom, char **pTo, i4 *length);
static TOKEN  FetchToken( LPSTR  szToken);
static const char    nt[] = "\0";



/*
**  SQLAllocStmt
**
**  Allocate a statement (STMT) block.
**
**  On entry: pdbc  -->data base connection block.
**            phstmt-->location for statement handle.
**
**  On exit:  phstmt-->-->statement block.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/10/1997  Jean Teng           Initialize stmtHandle 
*/

RETCODE SQL_API SQLAllocStmt(
    SQLHDBC     hdbc,
    SQLHSTMT  * phstmt)
{
    LPDBC       pdbc  = (LPDBC)hdbc;
    RETCODE     rc;

    if (!LockDbc (pdbc))
         return SQL_INVALID_HANDLE;

    rc = AllocStmt(pdbc, (LPSTMT*)phstmt);

    UnlockDbc (pdbc);
    return rc;
}

RETCODE AllocStmt(
    LPDBC      pdbc,
    LPSTMT    *ppstmt)
{
    RETCODE rc;
    LPSTMT  pstmt;
    LPSTMT  pstmtPrior;

    ErrResetDbc (pdbc);        /* clear any errors on DBC */

    /*
    **  Allocate STMT:
    */
    pstmt = (LPSTMT)MEreqmem(0, sizeof(STMT), TRUE, NULL);
    *ppstmt = pstmt;   /* return the STMT block or NULL */

    if (pstmt == NULL)
    {
        rc = ErrState (SQL_HY001, pdbc, F_OD0014_IDS_STATEMENT);
        return rc;
    }

    STcopy ("STMT*", pstmt->szEyeCatcher);
    STcopy ("00000", pstmt->szSqlState);

    pstmt->pAPD =                       /* allocated automatic APD */
    pstmt->pAPD_automatic_allocated = 
                  AllocDesc_InternalCall(pdbc, SQL_DESC_ALLOC_AUTO,
                                         INITIAL_DESCFLD_ALLOCATION);
    pstmt->pAPD->Type_APD_ARD_IPD_IRD = Type_APD;

    pstmt->pARD =                       /* allocated automatic ARD */
    pstmt->pARD_automatic_allocated = 
                  AllocDesc_InternalCall(pdbc, SQL_DESC_ALLOC_AUTO,
                                         INITIAL_DESCFLD_ALLOCATION);
    pstmt->pARD->Type_APD_ARD_IPD_IRD = Type_ARD;

    pstmt->pIPD =                       /* allocated automatic IPD */
    pstmt->pIPD_automatic_allocated = 
                  AllocDesc_InternalCall(pdbc, SQL_DESC_ALLOC_AUTO,
                                         INITIAL_DESCFLD_ALLOCATION);
    pstmt->pIPD->Type_APD_ARD_IPD_IRD = Type_IPD;

    pstmt->pIRD =                       /* allocated automatic IRD */
    pstmt->pIRD_automatic_allocated = 
                  AllocDesc_InternalCall(pdbc, SQL_DESC_ALLOC_AUTO,
                                         INITIAL_DESCFLD_ALLOCATION);
    pstmt->pIRD->Type_APD_ARD_IPD_IRD = Type_IRD;

    /*
    **  Link STMT into DBC-->STMT chain and
    **  assign IDMS internal statement number (aka SECTION),
    **  which must be in the range of 1-32767:
    */
    pstmt->pdbcOwner = pdbc;

    TRACE_INTL(pstmt,"AllocStmt");

    if (pdbc->pstmtFirst == NULL)
    {
    pdbc->pstmtFirst = pstmt;
    }
    else
    {
    pstmtPrior = pdbc->pstmtFirst;
    while (pstmtPrior->pstmtNext != NULL)
        pstmtPrior = pstmtPrior->pstmtNext;
    pstmtPrior->pstmtNext = pstmt;
    }

    pstmt->iCursor = 0;

    /*
    **  Initialize with default options from connection:
    */
    pstmt->fOptions      = pdbc->fOptions;
    pstmt->cbValueMax    = pdbc->cbValueMax;
    pstmt->cMaxLength    = pdbc->cMaxLength;
    pstmt->crowMax       = pdbc->crowMax;
    pstmt->crowFetchMax  = pdbc->crowFetchMax;
    pstmt->cQueryTimeout = pdbc->cQueryTimeout;
    pstmt->cRetrieveData = pdbc->cRetrieveData;
    pstmt->cUseBookmarks = SQL_UB_DEFAULT;  /* SQL_UB_OFF */
    pstmt->fConcurrency  = pdbc->fConcurrency;
    pstmt->fMetadataID   = pdbc->fMetadataID;
    pstmt->fAsyncEnable  = pdbc->fAsyncEnable;
    pstmt->fAPILevel     = pdbc->fAPILevel;
    pstmt->dateConnLevel = pdbc->fOptions & OPT_INGRESDATE ? 
                               IIAPI_LEVEL_3 : pdbc->fAPILevel;

    pstmt->fCursorType   = pdbc->fCursorType;
    pstmt->fCursorDirection = SQL_FETCH_NEXT;
    pstmt->fCursorScrollable = SQL_NONSCROLLABLE;
    pstmt->fCursorOffset = 0;
    pstmt->fCursorPosition = 0;
    pstmt->fCurrentCursPosition = 0;
    pstmt->fCursLastResSetSize = 0;
    pstmt->stmtHandle    = NULL;
    pdbc->sqb.pStmt      = pstmt;

    MEcopy("SQLCA   ", 8, pstmt->sqlca.SQLCAID);
    pstmt->sqlca.pdbc =  pdbc;  /* init dbc owner of SQLCA */

    /*
    ** Inialize query sequence control block.
    */
    pstmt->qry_cb.pstmt = pstmt;
    pstmt->qry_cb.qry_buff = NULL;
    pstmt->qry_cb.serviceParms.pp_parmCount = 0;
    pstmt->qry_cb.serviceParms.pp_parmData = NULL;
    pstmt->moreData = FALSE;
    pstmt->tabpriv_count = 0;
    pstmt->colpriv_count = 0;

    ResetStmt (pstmt, TRUE);     /* clear STMT state and free buffers */

    return SQL_SUCCESS;
}


/*
**  SQLBindParameter
**
**  Bind parameters on a statement handle.
**
**  On entry: pstmt     -->statement block.
**            ipar       = parameter index number.
**            fParamType = parameter type.
**            fCType     = C data type.
**            fSqlType   = SQL data type.
**            cbColDef   = ColumnSize (in chars) or Precision.
**            ibScale    = scale.
**            rgbValue  -->parameter value at execution time.
**            cbValueMax = maximum length of rgbValue (in bytes).
**            pcbValue  -->length of parameter value (in bytes).
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Note: The following edits are described in the ODBC 2.0 doc as being
**  done in this routine, but are done elsewhere per the ODBC 2.01 release
**  notes.  They really could be done here for our data bases:
**
**  1. SQL STATE HY105: invalid parameter type.
**
**     We support only input parameters, not SQL_PARAM_OUTPUT.
**
**     Edit done in SQLExecute.
**     SQL STATE returned by SQLExecute,  or SQLExecDirect.
**
**  2. SQL STATE 07006: restricted data type attribute violation.
**
**     Check that C type can actually be converted to SQL type.
**
**     Edit done in PutParm.
**     SQL STATE returned by SQLExecute, SQLExecDirect, or SQLPutData
*/

RETCODE SQL_API SQLBindParameter(
    SQLHSTMT      hstmt,        /* StatementHandle  */
    UWORD         ipar,         /* ParameterNumber  */
    SWORD         fParamType,   /* InputOutputType  */
    SWORD         fCType,       /* ValueType        */
    SWORD         fSqlType,     /* ParameterType    */
    SQLULEN       cbColDef,     /* ColumnSize       */
    SWORD         ibScale,      /* DecimalDigits    */
    SQLPOINTER    rgbValue,     /* ParameterValuePtr*/
    SQLLEN        cbValueMax,   /* BufferLength     */
    SQLLEN      *pcbValue)    /* StrLen_or_IndPtr */
{
    LPSTMT        pstmt = (LPSTMT)hstmt;
    LPDBC         pdbc;
    UDWORD        cbColDefMax;
    BOOL          fString = FALSE;
    LPDESCFLD     papd;
    LPDESCFLD     pipd;
    LPSQLTYPEATTR ptypeC, ptypeSql;
    SWORD         VerboseCType;
    SWORD         ConciseCType   = fCType;
    SWORD         VerboseSqlType;
    SWORD         ConciseSqlType = fSqlType;
    SQLSMALLINT   CPrecision        = 0;
    SQLUINTEGER   SqlLength         = 0;
    SQLUINTEGER   SqlPrecision      = 0;
    SQLSMALLINT   SqlScale          = 0;

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
    TraceOdbc (SQL_API_SQLBINDPARAMETER, pstmt,
        ipar, fParamType, fCType, fSqlType, cbColDef, ibScale,
        rgbValue, cbValueMax, pcbValue);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    pdbc = pstmt->pdbcOwner;
    cbColDefMax = 0x7ffffff0;    /* no limit up to 2G */

    /* 
    ** Driver Manager is suppose to map but Merant 3.11 DM and the
    ** the ODBC CLI fail to map old date, time, timestamp values to 
    ** new values. 
    */
    if (fCType==SQL_C_DATE)
        fCType=ConciseCType=SQL_C_TYPE_DATE;
    else if (fCType==SQL_C_TIME)
        fCType=ConciseCType=SQL_C_TYPE_TIME;
    else if (fCType==SQL_C_TIMESTAMP)
        fCType=ConciseCType=SQL_C_TYPE_TIMESTAMP;

    if (fSqlType==SQL_DATE)
        fSqlType=ConciseSqlType=SQL_TYPE_DATE;
    else if (fSqlType==SQL_TIME)
        fSqlType=ConciseSqlType=SQL_TYPE_TIME;
    else if (fSqlType==SQL_TIMESTAMP)
        fSqlType=ConciseSqlType=SQL_TYPE_TIMESTAMP;

    /*
    **  Find default type attibutes for SQL type:
    */

    ptypeSql = CvtGetSqlType (fSqlType, pstmt->fAPILevel, pstmt->dateConnLevel);
    if (!ptypeSql) return ErrUnlockStmt (SQL_HY004, pstmt);
                                     /* invalid SQL data type */
    
    if (fCType == SQL_C_DEFAULT)
    {
        if (ptypeSql->fCType == SQL_C_WCHAR && (pdbc->fOptions & 
            OPT_DEFAULTTOCHAR))
            ConciseCType = SQL_C_CHAR;
        else
            ConciseCType = ptypeSql->fCType;
        VerboseCType = ptypeSql->fVerboseType;
        VerboseSqlType = ptypeSql->fVerboseType;
        ConciseSqlType = fSqlType;
    }
    else
    {
        ptypeC = CvtGetCType (fCType, pstmt->fAPILevel, pstmt->dateConnLevel);
        if (!ptypeC) return ErrUnlockStmt (SQL_HY004, pstmt);
                                     /* invalid SQL data type */
        VerboseCType = ptypeC->fVerboseType;
        VerboseSqlType = ptypeSql->fVerboseType;
        ConciseCType = fCType;
        ConciseSqlType = fSqlType;
    }
    
    /*
    **  Check precision:
    **
    **  Note dependence on underlying DBMS.
    */
    switch (fSqlType)
    {
    case SQL_CHAR:
    case SQL_WCHAR:
    case SQL_BINARY:

        fString = TRUE;
        SqlLength = cbColDef;  /* IPD's SQL_DESC_LENGTH */
        break;

    case SQL_LONGVARCHAR:
    case SQL_LONGVARBINARY:
    case SQL_WLONGVARCHAR:

        fString = TRUE;
        SqlLength = cbColDef;  /* IPD's SQL_DESC_LENGTH */

        if (cbValueMax != SQL_SETPARAM_VALUE_MAX)   /* only for ODBC 1.0 app */
            break;

    case SQL_VARCHAR:
    case SQL_VARBINARY:
    case SQL_WVARCHAR:

        fString = TRUE;
        SqlLength = cbColDef;  /* IPD's SQL_DESC_LENGTH */
        cbColDefMax -= 2;
        break;

    case SQL_DECIMAL:
    case SQL_NUMERIC:

            /* if not Ingres or is old Ingres then use float instead */ /* bug 119274 to exclude the EA server*/
        if (( !isServerClassINGRES(pdbc)  ||  isOld6xRelease(pdbc) ) && !isServerClassEA(pdbc) )
           {fSqlType = VerboseSqlType = ConciseSqlType = SQL_FLOAT;
            ibScale = 0;
           }

        cbColDefMax = DB_MAX_DECPREC;
        SqlPrecision = (SQLSMALLINT)cbColDef;  /* IPD's SQL_DESC_PRECISION */
        SqlScale     = (SQLSMALLINT)ibScale;   /* IPD's SQL_DESC_SCALE     */

        break;

    case SQL_TYPE_TIMESTAMP:

        cbColDefMax = 26;
        if (cbColDef < 19)
            cbColDef = 0;
        break;

    case SQL_FLOAT:
    case SQL_REAL:
    case SQL_DOUBLE:

        SqlPrecision = (SQLSMALLINT)cbColDef;  /* IPD's SQL_DESC_PRECISION */
        /* fall through */

    default:

        if (ptypeSql->cbPrec)  /* if data type has implied precision */
        {
            cbColDef    = ptypeSql->cbPrec;    /* ignore precision passed */
            cbColDefMax = 0;                /* and don't bother to check it */
        }
        break;
    }

    if (cbColDefMax && (cbColDef > cbColDefMax))
        cbColDef = cbColDefMax;

    /*
    **  Ensure that APD and IPD parameter descriptors are large enough:
    */
    if (ipar > 
	(UWORD)pstmt->pAPD->CountAllocated)  /* if need more col, re-allocate */
        if (ReallocDescriptor(pstmt->pAPD, (i2)ipar) == NULL)
           {
            RETCODE rc = ErrState (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
            UnlockStmt (pstmt);
            return rc;
           }

    if (ipar > 
	(UWORD)pstmt->pIPD->CountAllocated)  /* if need more col, re-allocate */
        if (ReallocDescriptor(pstmt->pIPD, (i2)ipar) == NULL)
           {
            RETCODE rc = ErrState (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
            UnlockStmt (pstmt);
            return rc;
           }

    /*
    **  Set parameter info into the APD and IPD descriptors:
    */
    if ((UWORD)pstmt->pAPD->Count < ipar)  /* bump count of APD fields w/data */
       {
        ResetDescriptorFields(pstmt->pAPD, pstmt->pAPD->Count+1, ipar);
        pstmt->pAPD->Count = ipar;
       }
    if ((UWORD)pstmt->pIPD->Count < ipar)  /* bump count of IPD fields w/data */
       {
        ResetDescriptorFields(pstmt->pIPD, pstmt->pIPD->Count+1, ipar);
        pstmt->pIPD->Count = ipar;
       }

    papd = pstmt->pAPD->pcol + ipar;  /* pard->application descriptor field */
    pipd = pstmt->pIPD->pcol + ipar;  /* pird->implementation descriptor field */

    if (fCType == SQL_C_DEFAULT)
    {
        if (ptypeSql->fCType  == SQL_C_WCHAR && (pdbc->fOptions
             & OPT_DEFAULTTOCHAR))
             pipd->cvtToWChar = TRUE;
    }        

    if (fCType == SQL_C_NUMERIC)
    {
        CPrecision = MAX_NUM_DIGITS;
        SqlPrecision = (SQLSMALLINT)cbColDef;  /* IPD's SQL_DESC_PRECISION */
        SqlScale     = (SQLSMALLINT)ibScale;   /* IPD's SQL_DESC_SCALE     */
    }
    papd->VerboseType   = VerboseCType;   /* APD SQL_DESC_TYPE           */
    papd->ConciseType   = ConciseCType;   /* APD SQL_DESC_CONCISE_TYPE   */
    papd->Precision     = CPrecision;     /* APD SQL_DESC_PRECISION      */
    papd->Scale         = 0;              /* APD SQL_DESC_SCALE          */
    papd->ParameterType = fParamType;     /* APD SQL_DESC_PARAMETER_TYPE */
    papd->DataPtr       = rgbValue;       /* APD SQL_DESC_DATA_PTR       */
    papd->OctetLength   = cbValueMax;     /* APD SQL_DESC_OCTET_LENGTH   */
    papd->OctetLengthPtr= pcbValue;       /* APD SQL_DESC_OCTET_LENGTH_PTR*/
    papd->IndicatorPtr  = pcbValue;       /* APD SQL_DESC_INDICATOR_PTR  */
    papd->fStatus      |= COL_BOUND;      /* indicate parameter is bound */

    pipd->VerboseType   = VerboseSqlType; /* IPD SQL_DESC_TYPE           */
    pipd->ConciseType   = ConciseSqlType; /* IPD SQL_DESC_CONCISE_TYPE   */
    pipd->Length        = SqlLength;      /* IPD SQL_DESC_LENGTH         */
    pipd->Precision     = SqlPrecision;   /* IPD SQL_DESC_PRECISION      */
    pipd->Scale         = SqlScale;       /* IPD SQL_DESC_SCALE          */
    pipd->ParameterType = fParamType;     /* IPD SQL_DESC_PARAMETER_TYPE */
                            /* from InputOutputType (SQL_PARAM_INPUT,...)*/
    pipd->fStatus      |= COL_BOUND;      /* indicate parameter is bound */

    /* 
    **  Since the ODBC SQLParamOptions API Help has a code example that 
    **  seems to (incorrectly) imply that cbColDef can be used instead of 
    **  cbValueMax with parameter arrays, use cbColDef if needed: 
    */ 
    /* not needed any more in ODBC 3.x 
    ** oh yes it is--loera01 6/4/02
    */
    if (cbValueMax == 0 &&
            (ConciseCType == SQL_C_CHAR ||
             ConciseCType == SQL_C_BINARY ||
            (ConciseCType == SQL_C_DEFAULT && fString)))
        papd->OctetLength = cbColDef + 1;

    /*
    **  Set flag if parm is fixed length, which means
    **  SQLPutData cannot be called multiple times:
    */
    if (!fString ||
           (fCType != SQL_C_CHAR &&
            fCType != SQL_C_WCHAR &&
            fCType != SQL_C_BINARY &&
            fCType != SQL_C_DEFAULT))
        papd->fStatus |= COL_FIXED;

    /*
    **  Remember if caller is an ODBC 1.0 application:
    */
    if (cbValueMax == SQL_SETPARAM_VALUE_MAX)
        papd->fStatus |= COL_ODBC10;      /* indicate parameter is 1.0 bound */

    UnlockStmt (pstmt);
    return SQL_SUCCESS;
}


/*
**  SQLCloseCursor
**
**  Closes a cursor that has been opened on a statement,
**  and discards pending results.
**
**  On entry:
**    StatementHandle [Input]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
**  Notes:
**    SQLCloseCursor returns SQLSTATE 24000 (Invalid cursor state) if no cursor
**    is open. Calling SQLCloseCursor is equivalent to calling SQLFreeStmt with
**    the SQL_CLOSE option, with the exception that SQLFreeStmt with SQL_CLOSE
**    has no effect on the application if no cursor is open on the statement,
**    while SQLCloseCursor returns SQLSTATE 24000 (Invalid cursor state).
** 
*/
SQLRETURN  SQL_API SQLCloseCursor (
    SQLHSTMT StatementHandle)
{
    LPSTMT   pstmt = (LPSTMT)StatementHandle;
    LPDBC    pdbc;
    RETCODE  rc;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLCLOSECURSOR,pstmt);
    /*
    **  Note that LockStmt really locks the DBC, not the statement.
    **  It does validate the pstmt, so that's why it is called.
    **  If this behavior is ever changed, it will be necessary
    **  to explicitly lock the DBC itself, which can get messy
    **  (see SQLFreeConnnect).
    */  

    if (!LockStmt (pstmt))          /* check for valid STMT and lock DBC */
       return SQL_INVALID_HANDLE;

    if (!(pstmt->fStatus & STMT_OPEN))            /* if cursor not open then */
        return ErrUnlockStmt (SQL_24000, pstmt);  /*  invalid cursor state */

    pdbc  = pstmt->pdbcOwner;       /* remember the DBC */

    rc = FreeStmt(pstmt, SQL_CLOSE);/* close the STMT */

    UnlockDbc (pdbc);               /* unlock DBC */
    return rc;
}


/*
**  SQLFreeStatement
**
**  Free all statement resources:
**
**  On entry: pstmt  -->statement control block.
**            fOption = free options
**
**  On exit:  All storage for statement is freed if fOption = SQL_DROP.
**
**  Returns:  SQL_SUCCESS
**            SQL_INVALID_HANDLE
**
**  Change History
**  --------------
**
**  date        programmer              description
**
**      01/14/1997  Jean Teng       called sqlclose if stmt has been executed
**  01/16/1997  Jean Teng       use pstmt->stmtHandle to decide if sqlclose needed   
**  12/30/1998  Bobby Ward          Don't do a commit for main connection; 
**                                  SQLFreeStmt should free the statement but
**                                  not commit the transaction. Bug 94064
*/

RETCODE SQL_API SQLFreeStmt(
    SQLHSTMT hstmt,
    UWORD   fOption)
{
    LPSTMT   pstmt = (LPSTMT)hstmt;
    LPDBC    pdbc;
    RETCODE  rc;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLFREESTMT,pstmt, fOption);
    /*
    /*
    **  Note that LockStmt really locks the DBC, not the statement.
    **  It does validate the pstmt, so that's why it is called.
    **  If this behavior is ever changed, it will be necessary
    **  to explicitly lock the DBC itself, which can get messy
    **  (see SQLFreeConnnect).
    */  
    if (!LockStmt (pstmt))          /* check for valid STMT and lock DBC */
       return SQL_INVALID_HANDLE;

    pdbc  = pstmt->pdbcOwner;       /* remember the DBC */

    if (pstmt->fStatus & STMT_OPEN)
       {
       }

    rc = FreeStmt(pstmt, fOption);  /* free the STMT */

    UnlockDbc (pdbc);               /* unlock DBC */
    return rc;
}

RETCODE FreeStmt(
    LPSTMT  pstmt,
    UWORD   fOption)
{
    LPDBC   pdbc;
    LPSTMT  pstmtPrior;
    LPSTMT  pstmtCurrent;
    LPSESS  psess;
    LPSESS  psessMain;
    RETCODE rc      = SQL_SUCCESS;
    BOOL    fCommit = 0;

    TRACE_INTL(pstmt, "FreeStmt");
    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    pdbc  = pstmt->pdbcOwner;
    psess = pstmt->psess;
    psessMain = &pdbc->sessMain;

    switch (fOption)
    {
    case SQL_DROP:
        /*
        **  Release everything associated with STMT:
        **  (note fall through...)
        */

    case SQL_CLOSE:
        /*
        **  Close cursor if open, commit if we can, and suspend,
        **  and reinitialize statement status info:
        */
        if (pstmt->fStatus & STMT_OPEN)
        {
            pstmt->fStatus &= ~STMT_OPEN;

            /*
            **  If the cursor is "open" for a constant or cached query (for
            **  some catalog function), there is nothing to do on the server.
            */
            if (pstmt->fStatus & (STMT_CONST | STMT_CACHE))
            {
            }
            /*
            **  If transaction was committed at end of fetch
            **  then there won't be anything to do on the server,
            **  and the session started flag will be off.
            **  If it's on, there are lots of things we can do:
            */
            else if (psess->fStatus & SESS_STARTED)  /* if TXN started */
            {
                pdbc->sqb.Options = SQB_SUSPEND;

                /*
                **  If cursor is "open" on catalog connection:
                **
                **  1. Set session anchor for catalog connection.
                **  2. If no other ODBC cursors open, we can commit. 
                **  3. If open, close cursor
                **  4. Commit (maybe).
                **  5. Restore session anchor.
                */
                if (pstmt->fStatus & STMT_CATALOG)
                {
                    CatSqlsid (pstmt, TRUE);
                    fCommit = !CursorOpen (pdbc, STMT_CATALOG);

                    /*
                    ** If not already closed by SQL Fetch and EOD...
                    */
                    if (!(pstmt->fStatus & STMT_CLOSED))
                    {                                  
                        pdbc->sqb.pSqlca = &pstmt->sqlca;
                        pdbc->sqb.pStmt = pstmt;
                        SqlClose (&pdbc->sqb);
                        rc = SQLCA_ERROR (pstmt, psess);
                    }

                    if (fCommit) /* if no other ODBC cursors */
                    {
                        pdbc->sqb.pSqlca = &pdbc->sqlca;
                        pdbc->sqb.pStmt = pstmt;
                        SqlCommit (&pdbc->sqb);
                        rc = SQLCA_ERROR (pdbc, psess);
                        if (rc == SQL_SUCCESS)
                            psess->fStatus |= SESS_SUSPENDED;
                    }
                    CatSqlsid (pstmt, FALSE);
                } /* endif (pstmt->fStatus & STMT_CATALOG)*/
                else
                {
                    /*  
                    **  If cursor is still open close it.
                    **
                    **  Note that session is already suspended if any
                    **  fetch was issued (when positioned updates are
                    **  implemented this will only be true for read only 
                    **  cursors).
                    */
                    /*
                    ** If not already closed by SQL Fetch and EOD...
                    */
                    if (!(pstmt->fStatus & STMT_CLOSED)) 
                    {                                   
                        pdbc->sqb.pSqlca = &pstmt->sqlca;
                        pdbc->sqb.pStmt = pstmt;
                        SqlClose (&pdbc->sqb);
                        rc = SQLCA_ERROR (pstmt, psess);
                    }
                }
                pdbc->sqb.Options = 0;
            } /* else if (psess->fStatus & SESS_STARTED)  */
        } /* if (pstmt->fStatus & STMT_OPEN) */
        
        /* 
        ** If a result set was not OPENed (STMT_OPEN) yet but an API query
        ** is open due to something like SQLPrepare, SQLDescribeCol sequence
        ** then close the API query only if DROPing the stmt.  Keep the API
        ** query active since the ODBC State Transition Table in Appendix B
        ** of the ODBC specification indicates that the query stays prepared. 
        ** This will handle SQLPrepare, SQLDescribeCol, SQLFreeStmt(SQL_CLOSE)
        ** SQLExecute sequence. 
        */
        if (pstmt->stmtHandle != NULL  && (fOption == SQL_DROP ))
        {
            pdbc->sqb.pStmt = pstmt;
            SqlClose (&pdbc->sqb);
        }
        
        /*
        **  If statement was issued by SQLExecuteDirect
        **  reset and free everything now, otherwise just reset flags
        **  and counters:
        */
        if (pstmt->fStatus & STMT_DIRECT) 
        {
            ResetStmt (pstmt, TRUE);     /* clear STMT state and free buffers */
            /*
            ** If this query was executed as a direct statement from
            ** SQLExecute(), ResetStmt() won't clear any status flags.
            ** Treat the same as a prepared query.
            */
            if (pstmt->fStatus & STMT_SQLEXECUTE)
		pstmt->fStatus &= ~(STMT_EOF | STMT_CLOSED | STMT_CONST | 
                STMT_CATALOG | STMT_API_CURSOR_OPENED);
        }
        else
        {
            pstmt->fStatus &= ~(STMT_EOF | STMT_CLOSED | STMT_CONST | 
                STMT_CATALOG | STMT_API_CURSOR_OPENED);
            ResetStmt (pstmt, FALSE);  /* clear STMT state and keep buffers */
        }

        /* 
        ** Simulated auto-commit is needed at close time.  If ODBC 
        ** auto-commit=ON and Ingres auto-commit=OFF and all Ingres 
        ** cursors are closed (to be consistent with Ingres 
        ** auto-commit whose "DBMS does not issue a commit until the close 
        ** cursor is executed, because cursors are logically a single 
        ** statement."  -- SQL Reference Guide - Committing Transactions 
        */

        /*
        ** If ODBC autocommit is on and API autocommit is off, and no XA
        ** transaction active and all API cursors closed...
        */
        if (  pdbc->fOptions & OPT_AUTOCOMMIT && 
            !(psessMain->fStatus & SESS_AUTOCOMMIT_ON_ISSUED) && 
            !(psessMain->XAtranHandle) &&  ! AnyIngresCursorOpen(pdbc)) 
        {
            /* 
            ** ...close the result sets, commit updates, and free select locks.
            */
            SQLTransact_InternalCall(NULL, pdbc, SQL_COMMIT); 
        }

        if (fOption == SQL_CLOSE)
            break;

    case SQL_RESET_PARAMS:
        /*
        **  Reset parm descriptions set by SQLSetParam for STMT:
        **
        */

        pstmt->crowParm = 0;
        ResetDescriptorFields(pstmt->pAPD, 0, pstmt->pAPD->Count);
        ResetDescriptorFields(pstmt->pIPD, 0, pstmt->pIPD->Count);
        pstmt->pAPD->Count = 0;
        pstmt->pIPD->Count = 0;
        if (fOption == SQL_RESET_PARAMS)
            break;

    case SQL_UNBIND:
        /*
        **  Unbind columns.
        */
        ResetDescriptorFields(pstmt->pARD, 0, pstmt->pARD->Count);
        pstmt->pARD->Count = 0;

        pstmt->icolPrev     = 0;  /* reset previous SQLGetData column */

        if (fOption == SQL_UNBIND)
            break;

        /*
        **  SQL_DROP falls through all cases to here:
        */
        /*
        **  Drop descriptors:
        */

        /*
        ** If query was saved during simulated autocommit of prepared
        ** statements, free the query memory and unset the flag indicating
        ** direct execution from SQLExecute().  This allows ResetStmt()
        ** to free pstmt->szSqlStr.
        */

        if (pstmt->fStatus & STMT_SQLEXECUTE)
        {
            pstmt->fStatus &= ~STMT_SQLEXECUTE;
            pstmt->fStatus &= ~STMT_DIRECT;
        }

        ResetStmt (pstmt, TRUE);     /* clear STMT state and free buffers */

        if (pstmt->iPreparedStmtID)    /* deallocate any prepared statement */
            FreePreparedStmtID (pstmt); /*    number from any PREPARE s00001 */
        if (pstmt->szPSqlStr)
            MEfree((PTR)pstmt->szPSqlStr);

        /*
        **  Drop automatically allocated descriptors.
        */
        FreeDescriptor(pstmt->pAPD_automatic_allocated);
        FreeDescriptor(pstmt->pARD_automatic_allocated);
        FreeDescriptor(pstmt->pIPD_automatic_allocated);
        FreeDescriptor(pstmt->pIRD_automatic_allocated);
        pstmt->pAPD_automatic_allocated = NULL;
        pstmt->pARD_automatic_allocated = NULL;
        pstmt->pIPD_automatic_allocated = NULL;
        pstmt->pIRD_automatic_allocated = NULL;

        /*
        **  Unlink STMT from DBC-->STMT chain and free it:
        */
	if (pdbc->pstmtFirst == pstmt)
	    pdbc->pstmtFirst = pstmt->pstmtNext;
	else
	{
	    pstmtPrior = pdbc->pstmtFirst;
	    while ((pstmtCurrent = pstmtPrior->pstmtNext) != pstmt)
	    {
		if (pstmtCurrent == NULL)
		   return SQL_INVALID_HANDLE; 
		pstmtPrior = pstmtCurrent;
	    }
	    pstmtPrior->pstmtNext = pstmt->pstmtNext;
	}
        pstmt->pdbcOwner->sqb.pStmt = NULL;
        MEfree ((PTR)pstmt);
	break;

    default:
        /*
        **  We've been asked to do the impossible:
        */
        rc = ErrState(SQL_HY092, pdbc);
    }

    return rc;
}


/*
**  SQLGetCursorName
**
**  Return the cursor name for a statement handle
**
**  On entry: pstmt      -->statment block.
**            szCursor   -->cursor name buffer.
**            cbCursorMax = max length of cursor name buffer.
**            pcbCursor  -->cursor name length.
**
**  On exit:  *szCursor   = cursor name.
**            *pcbCursor  = cursor name length.
**
**  Returns:  SQL_SUCCESS
*/

RETCODE SQL_API SQLGetCursorName(
    SQLHSTMT      hstmt,
    UCHAR       * szCursor,
    SWORD         cbCursorMax,
    SWORD       * pcbCursor)
{   return SQLGetCursorName_InternalCall(
                  hstmt,
                  szCursor,
                  cbCursorMax,
                  pcbCursor);
}

RETCODE SQL_API SQLGetCursorName_InternalCall(
    SQLHSTMT      hstmt,
    UCHAR       * szCursor,
    SWORD         cbCursorMax,
    SWORD       * pcbCursor)
{
    LPSTMT  pstmt = (LPSTMT)hstmt;
    RETCODE rc;

    TRACE_INTL(pstmt, "SQLGetCursorName_InternalCall");
    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
	TraceOdbc (SQL_API_SQLGETCURSORNAME, pstmt,
	    szCursor, cbCursorMax, pcbCursor);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */

    if (STlength(pstmt->szCursor) == 0)
        return ErrUnlockStmt (SQL_HY015, pstmt); /* no cursor name available */

    rc = GetChar (pstmt, pstmt->szCursor, (char*)szCursor, cbCursorMax, pcbCursor);
    UnlockStmt (pstmt);
    return rc; 
}


/*
**  SQLNativeSql
**
**  Return the SQL string as modified by the driver.
**  Does not execute the statement.
**
**  On entry: hdbc              = connection handle
**            szSqlStr         -->SQL text string to be translated
**            cbSqlStr          = SQL text string length or SQL_NTS
**            OutStatementText -->buffer to contain translated string
**            BufferLength      = length of *OutStatementText
**            pcbLength        -->integer to contain available bytes to return
**
**  On exit:  *OutStatementText = string
**            *pcbLength        = string length.
**
**  Returns:  SQL_SUCCESS
*/

RETCODE SQL_API SQLNativeSql(
    SQLHDBC     hdbc,
    SQLCHAR   * InStatementText,
    SQLINTEGER  TextLength1,
    SQLCHAR   * OutStatementText,
    SQLINTEGER  BufferLength,
    SQLINTEGER* pcbValue)
{
    return SQLNativeSql_InternalCall(
                hdbc,
                InStatementText,
                TextLength1,
                OutStatementText,
                BufferLength,
                pcbValue);
}

RETCODE SQL_API SQLNativeSql_InternalCall(
    SQLHDBC     hdbc,
    SQLCHAR   * InStatementText,
    SQLINTEGER  TextLength1,
    SQLCHAR   * OutStatementText,
    SQLINTEGER  BufferLength,
    SQLINTEGER* pcbValue)
{
    LPDBC       pdbc     = (LPDBC)hdbc;
    CHAR      * szSqlStr = (CHAR*)InStatementText;
    SQLINTEGER  cbSqlStr =        TextLength1;
    LPSTMT      pstmt;
    RETCODE     rc, rc2;
    CHAR      * szAPISqlStr;
    SDWORD      cbAPISqlStr;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    TRACE_INTL(pdbc,"SQLNativeSql_InternalCall");

    ErrResetDbc (pdbc);        /* clear ODBC error status on DBC */

    rc = AllocStmt(pdbc, &pstmt);  /* allocate a work statement */
    if (rc == SQL_ERROR)
    {
        UnlockDbc (pdbc);
        return rc; 
    }

    if (cbSqlStr == SQL_NTS)
        cbSqlStr = STlength (szSqlStr);

    /* alloc a buffer with a safety margin for escape substitutions */
    szAPISqlStr = MEreqmem(0, cbSqlStr + 
         CountLiteralItems(cbSqlStr, (CHAR*)szSqlStr, pstmt->dateConnLevel) 
             + 50, TRUE, NULL);
    if (szAPISqlStr == NULL)    /* allocate a work string */
       {UnlockDbc (pdbc);
        return ErrState (SQL_HY001, pdbc, F_OD0013_IDS_SQL_SAVE);
       }

    if (pstmt->fAPILevel > IIAPI_LEVEL_3)
        ConvertEscAndMarkers(szSqlStr, &cbSqlStr, szAPISqlStr, &cbAPISqlStr,
            pstmt->dateConnLevel);
    else
        ConvertParmMarkers(szSqlStr, &cbSqlStr, szAPISqlStr, &cbAPISqlStr);
            /* Convert parameter markers (?) to ~V */
    rc = ParseCommand (pstmt, szAPISqlStr, cbAPISqlStr, FALSE);
            /* Scan for escape sequences and translate to native DBMS syntax */
    MEfree (szAPISqlStr);             /* free the work string */

    if (rc != SQL_ERROR)
        rc = GetCharAndLength (pdbc, pstmt->szSqlStr,
                               (CHAR*)OutStatementText, BufferLength, pcbValue);

    rc2 = FreeStmt(pstmt, SQL_DROP);  /* drop the work STMT */

    UnlockDbc (pdbc);
    return rc; 
}


/*
**  SQLParamOptions
**
**  Set number of values for the set of parameters.
**
**  On entry: pstmt-->statement block.
**            crow  = number of values for each parameter.
**            pirow-->storage for current row number.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Most useful for bulk insert.  Note that changing the number of
**  sets of paramter values will force the statement to be prepared
**  again, in order to reallocate any buffers needed.
*/
RETCODE SQL_API SQLParamOptions(
    SQLHSTMT     hstmt,
    UDWORD       crow,
    UDWORD     * pirow)
{
    LPSTMT  pstmt = (LPSTMT)hstmt;
    LPDBC   pdbc;

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
	TraceOdbc (SQL_API_SQLPARAMOPTIONS, pstmt, crow, pirow);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */
    pdbc = pstmt->pdbcOwner;

    pstmt->crowParm  = crow;
    pstmt->irowParm  = 0;    /* initialize current parameter row number */
                             /* of a parameter array */
    pstmt->pirowParm = pirow;

    UnlockStmt (pstmt);
    return SQL_SUCCESS;
}



/*
**  SQLPrepare
**
**  Prepare an SQL statement.
**
**  IDMS R12.0 requires that all parameters be defined when an
**  SQL statement is prepared.  ODBC does not require this.
**  Since we can't be sure that SQLSetParam has called for all parameter
**  markers in the syntax we do the follwing:
**
**  1. Reset the STMT (dump any saved syntax and off all stmt flags).
**  2. Parse the syntax (minimally), get its length and SQL command
**     (this allocates a buffer and saves the syntax with certain
**     limited ODBC specific tokens converted to IDMS equivalents).
**  3. Return to the caller with success if it seems a valid command.
**
**  On entry: pstmt   -->statment block.
**            szSqlStr-->SQL statement text.
**            cbSqlStr = length of SQL statement text.
**
**  On exit:  Statement syntax is save in a buffer.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**  date        programmer          description
**
**  01/14/1997  Jean Teng                       convert ? to ~V   
**   08-May-2008 (Ralph Loen) SIR 120340
**          Allocate szPSqlStr and set cbPSqlStr in pstmt if the statement 
**          cannot be prepared.  SQLExecute() will detect this and invoke 
**          the statement via SQLExecDirect_InternalCall(). 
**   21-May-2008 (Ralph Loen) Bug 120412
**          Release the statement handle before returning if a non-preparable 
**          query is detected.
*/

RETCODE SQL_API SQLPrepare(
    SQLHSTMT      hstmt,
    UCHAR       * szSqlStr,
    SDWORD        cbSqlStr)
{
    return SQLPrepare_InternalCall(
                  hstmt,
                  szSqlStr,
                  cbSqlStr,
                  FALSE);
}

RETCODE SQL_API SQLPrepare_InternalCall(
    SQLHSTMT      hstmt,
    UCHAR       * szSqlStr,
    SDWORD        cbSqlStrParm,
    BOOL          rePrepared)
{
    LPSTMT        pstmt = (LPSTMT)hstmt;
    RETCODE       rc;
    CHAR     *    szAPISqlStr;
    SDWORD        cbAPISqlStr;
    SQLINTEGER        cbSqlStr = cbSqlStrParm;
    LPDBC pdbc = pstmt->pdbcOwner;
    SESS *psess = pdbc->sqb.pSession;
    LPSQB pSqb = &pdbc->sqb;

    TRACE_INTL(pstmt,"SQLPrepare_InternalCall");

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLPREPARE, pstmt, szSqlStr, cbSqlStr);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */

    if (pstmt->fStatus & STMT_OPEN) /* if already an open result set, error */
        return ErrUnlockStmt (SQL_24000, pstmt);

    /*
    ** If invoked for the first time (from SQLPrepare()), reset and clear
    ** all buffers.  If invoked from SQLExecute(), just reset flags and
    ** counters.
    */
    if (!rePrepared)
    {
        pstmt->reExecuteCount = 0;
        ResetStmt (pstmt, TRUE);
    }
    else
        ResetStmt (pstmt, FALSE);

    pstmt->irowParm = 0;         /* initialize current parameter row number */
                                 /* of a parameter array */


    if (cbSqlStr == SQL_NTS)
        cbSqlStr = (SDWORD)STlength ((char*)szSqlStr);

    szAPISqlStr = MEreqmem(0, cbSqlStr + 
         CountLiteralItems(cbSqlStr, (CHAR*)szSqlStr, pstmt->dateConnLevel) 
            + 50, TRUE, NULL);
    if (szAPISqlStr == NULL)
    {
        UnlockStmt (pstmt);
        return ErrState (SQL_HY001, pstmt, F_OD0013_IDS_SQL_SAVE);
    }

    if (pstmt->fAPILevel > IIAPI_LEVEL_3)
        ConvertEscAndMarkers((CHAR*)szSqlStr, &cbSqlStr, szAPISqlStr, 
            &cbAPISqlStr, pstmt->dateConnLevel);
    else
        ConvertParmMarkers((CHAR*)szSqlStr, &cbSqlStr, szAPISqlStr, 
            &cbAPISqlStr);
    rc = ParseCommand (pstmt, szAPISqlStr, cbAPISqlStr, rePrepared);
    MEfree (szAPISqlStr);

    if (rc == SQL_SUCCESS && pstmt->fCommand == CMD_EXT)
        rc = ErrState (SQL_HY000, pstmt, F_OD0043_IDS_ERR_EXTENSION);

    if (rc == SQL_SUCCESS && pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
    {
        UnlockStmt(pstmt);
        return ErrState (SQL_HY000, pstmt, 
            F_OD0169_IDS_ERR_PREP_UCURS );
    }

    /*
    **  Initialize the SQL control block anchor
    */
    pdbc->sqb.pSqlca = &pstmt->sqlca;
    pdbc->sqb.pStmt = pstmt;

    rc = SetTransaction (pdbc, psess);
    if (rc != SQL_SUCCESS)
    {
        UnlockStmt(pstmt);
        return rc;
    }
    /*
    ** Make sure that any transaction options that might have
    ** been reset by the last commit are in effect again.  Also
    ** indicate that we have started a transaction.
    */
    if (pstmt->fCommand == CMD_SELECT)
        odbc_AutoCommit(pSqb, FALSE);

    if (pstmt->fStatus & STMT_CANBEPREPARED)
        SqlPrepare (&pdbc->sqb, pstmt->szSqlStr); 
    else
    {
        pstmt->szPSqlStr = STalloc(szSqlStr); 
        pstmt->cbPSqlStr = STlength(szSqlStr);
        UnlockStmt(pstmt);
        return rc;
    }

    psess->fStatus &= ~SESS_SUSPENDED;
    if (pstmt->fCommand & CMD_START)
        psess->fStatus |= SESS_STARTED; /* mark transaction started */

    rc = SQLCA_ERROR (pstmt, psess);
    if (rc == SQL_ERROR)
    {
        ErrSetToken (pstmt, pstmt->szSqlStr, pstmt->cbSqlStr);
        UnlockStmt(pstmt);
        return rc;
    }

    UnlockStmt (pstmt);
    return rc;
}


/*
**  SQLSetCursorName
**
**  Set the cursor name on a statement handle
**
**  On entry: pstmt   -->statment block.
**            szCursor-->cursor name.
**            cbCursor = cursor name length.
**
**  On exit:  nothing special.
**
**  Returns:  SQL_SUCCESS
*/

RETCODE SQL_API SQLSetCursorName(
    SQLHSTMT      hstmt,
    UCHAR       * szCursorParameter,
    SWORD         cbCursor)
{   return SQLSetCursorName_InternalCall(hstmt, szCursorParameter, cbCursor);
}

RETCODE SQL_API SQLSetCursorName_InternalCall(
    SQLHSTMT      hstmt,
    UCHAR       * szCursorParameter,
    SWORD         cbCursor)
{
    LPSTMT  pstmt = (LPSTMT)hstmt;
    CHAR        * szCursor = (CHAR *)szCursorParameter;
    LPSTMT  pstmtCurrent;
    i4      cb;

    TRACE_INTL(pstmt,"SQLSetCursorName_InternalCall");
    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
	TraceOdbc (SQL_API_SQLSETCURSORNAME, pstmt, szCursor, cbCursor);

    ErrResetStmt (pstmt);        /* clear ODBC error status on STMT */

    if (pstmt->fStatus & STMT_OPEN) /* if already an open result set, error */
	return ErrUnlockStmt (SQL_24000, pstmt);

    cb = (cbCursor == SQL_NTS) ? (i4)STlength (szCursor) : (UWORD)cbCursor;
    if (cb > 18 || MEcmp (szCursor, CURSOR_PREFIX, min (cb, sizeof (CURSOR_PREFIX))) == 0)
        return ErrUnlockStmt (SQL_34000, pstmt);

    pstmtCurrent = pstmt->pdbcOwner->pstmtFirst;  /* scan other stmts, */
    while (pstmtCurrent != NULL)                  /*   make sure no duplicates */
    {
        if (STcompare (pstmtCurrent->szCursor, szCursor) == 0)
            return ErrUnlockStmt (SQL_3C000, pstmt);

	pstmtCurrent = pstmtCurrent->pstmtNext;
    }

    MEcopy(szCursor, cb, pstmt->szCursor);
    CMcpychar(nt,&pstmt->szCursor[cb]); 
    UnlockStmt (pstmt);
    return SQL_SUCCESS;
}


/*
**  AllocPreparedStmtID
**
**  Allocate an prepared statement id from the DBC bit map.
**  We do this to avoid chewing up prepared statment resources
**  in the gateways.  We try to use the same prepare stmt name.
**
**  On entry: pstmt->statement block.
**
**  On exit:  pstmt->iPreparedStmtID == nnnnn for prepare snnnnn.
**            pdbc->cPreparedStmtMap has bit map updated.
**
**    15-apr-2005 (lunbr01,loera01) Bug 114319
**          Increase size of prepared statement id (PSID) cache from 32 
**          to 256 PSIDs; use CL facility BT to handle bit manipulation
**          since current logic would only work up to 32 bits. Also, if 
**          cache is full (all PSIDs used), increment PSIDs thereafter
**          based on connection high PSID rather than assigning duplicate
**          PSID based on statement cursor id. 
*/

void    AllocPreparedStmtID (LPSTMT pstmt)
{
    LPDBC   pdbc   = pstmt->pdbcOwner;
    int     i;

    TRACE_INTL(pstmt, "AllocPreparedStmtID");
    for (i=0; i<MAX_PSID; i++)
	{
	  if (BTtest(i,pdbc->cPreparedStmtMap) == 0)
	     { BTset(i,pdbc->cPreparedStmtMap);
	       pstmt->iPreparedStmtID = i+1;
	       return;
	     } 
	}  /* end for loop */

    /* DBC's allocation bit map for prepared stmt numbers is full
       (there's room for MAX_PSID prepared statments) because the user is not
       SQLFree(SQL_DROP) his statements.  Fall back to incrementing 
       and using the maximum PSID for the connection; in this range,
       we won't keep track of when a PSID becomes unused and available
       for reuse. */
    pstmt->iPreparedStmtID = pstmt->iCursor + 32;
    if (pstmt->pdbcOwner->iPreparedStmtID == 0)  /* prime the first time */
       pstmt->pdbcOwner->iPreparedStmtID = MAX_PSID;
    pstmt->pdbcOwner->iPreparedStmtID++;
    pstmt->iPreparedStmtID = pstmt->pdbcOwner->iPreparedStmtID;
    return;
}


/*
**  FreePreparedStmtID
**
**  Free an prepared statement id back into the DBC bit map.
**
**  On entry: pstmt->statement block.
**
**  On exit:  pstmt->iPreparedStmtID == 0.
**            pdbc->cPreparedStmtMap has bit map updated.
**
**    15-apr-2005 (lunbr01,loera01) Bug 114319
**          Use BT to clear freed PSID in expanded cache.
*/

void    FreePreparedStmtID (LPSTMT pstmt)
{
    LPDBC   pdbc   = pstmt->pdbcOwner;
    int     i      = (int)(pstmt->iPreparedStmtID);

    TRACE_INTL(pstmt,"FreePreparedStmtID");
    if (i>0  &&  i<=MAX_PSID)  /* mapped id must range 1-MAX_PSID */ 
       { 
	 BTclear(i-1,pdbc->cPreparedStmtMap);
       }  /* end if */

    pstmt->iPreparedStmtID = 0;
    return;
}


/*
**  FreeSqlStr
**
**  Free saved SQL syntax string.
**
**  On entry: pstmt-->statement block.
**
**  On exit:  Save SQL syntax string is freed, anchor and length cleared.
**
**  Returns:  nothing.
*/

VOID FreeSqlStr(
    LPSTMT  pstmt)
{
    TRACE_INTL (pstmt, "FreeSqlStr");
    if (pstmt->szSqlStr)
        MEfree (pstmt->szSqlStr);
    
    pstmt->szSqlStr = NULL;
    pstmt->cbSqlStr = 0;
    return;
}


/*
**  FreeStmtBuffers
**
**  Free statement buffers and SQLDA's:
**
**  On entry: pstmt-->statement block.
**
**  On exit:  Parm and fetch buffers and descriptors are freed
**            and anchors cleared.
**
**  Returns:  nothing.
*/

VOID FreeStmtBuffers(
    LPSTMT  pstmt)
{
    LPDESC    pAPD = pstmt->pAPD;   /* Application Parameter Descriptor */
    LPDESC    pIPD = pstmt->pIPD;   /* Implementation Parameter Descriptor */
    LPDESC    pARD = pstmt->pARD;   /* Application Row Descriptor */
    LPDESCFLD papd;                 /* APD parm field */
    LPDESCFLD pipd;                 /* IPD parm field */
    LPDESCFLD pard;
    SDWORD    i;    
    TABLEPRIV_ROW *tabpriv_row;
    COLPRIV_ROW *colpriv_row;
    SDWORD    countAllocated = min( pIPD->CountAllocated, 
        pstmt->icolParmLast );

    TRACE_INTL (pstmt, "FreeStmtBuffers");

    if (pstmt->pFetchBuffer)
        MEfree ((PTR)pstmt->pFetchBuffer);
    pstmt->pFetchBuffer = NULL;
  
    pard = pARD->pcol + 1;

    papd = pAPD->pcol + 1;  /* papd->1st APD parameter field after bookmark */
    pipd = pIPD->pcol + 1;  /* papd->1st APD parameter field after bookmark */

    for (i = 0;
         (UWORD)i < pstmt->icolFetch;  /* i < count of fetched cols */
         i++, pard++)
    {
        if (pard->plb)
        {
            if (pard->plb->pLongData && pard->DataPtr != pard->plb->pLongData)
                MEfree ((PTR)pard->plb->pLongData); 
            MEfree ((PTR)pard->plb);
            pard->plb = NULL; 
        }
    }
    /*
    **  Look for allocated data pointers in the IPD.  Note that the
    **  "countAllocated" variable represents either the total number of
    **  parameters or the total number of allocated LPDESCFLD structures
    **  in the IPD, whichever is lesser. 
    **
    **  If parameters are specified in a query but not bound, the 
    **  number of allocated LPDESCFLD structures may be less than the 
    **  parameter count.  If all of the parameters specified in the 
    **  query have been bound, then the number of allocated descriptors 
    **  is equal to or greater than the number of parameters.
    **
    **  Data pointers may be allocated only for bound parameters, so
    **  only those are of interest.
    */
    for (i = 0;
         (UWORD)i < countAllocated;  /* count of allocated parm descriptors */
         i++, papd++, pipd++)
    {
        if (pipd->DataPtr && (pipd->fStatus & COL_DATAPTR_ALLOCED))
        {
            MEfree ((PTR)pipd->DataPtr);
            pipd->DataPtr = NULL;
            pipd->fStatus &= ~COL_DATAPTR_ALLOCED;
        }
    }

    /*
    ** Free the list of column and/or table privileges, if allocated.
    */
    for (i = 0; i < pstmt->tabpriv_count; i++)
    {
        tabpriv_row = (TABLEPRIV_ROW *)pstmt->baseTabpriv_q.q_prev;
        if (tabpriv_row->catalog)
            MEfree(tabpriv_row->catalog);
        if (tabpriv_row->ownerName)
            MEfree(tabpriv_row->ownerName);
        if (tabpriv_row->tableName)
            MEfree(tabpriv_row->tableName);
        if (tabpriv_row->grantor)
            MEfree(tabpriv_row->grantor);
        if (tabpriv_row->grantee)
            MEfree(tabpriv_row->grantee);
        if (tabpriv_row->privilege)
            MEfree(tabpriv_row->privilege);
        if (tabpriv_row->is_grantable)
            MEfree(tabpriv_row->is_grantable);
        QUremove((QUEUE *)tabpriv_row);
        MEfree((PTR)tabpriv_row);
    }
    pstmt->tabpriv_count = 0;

    for (i = 0; i < pstmt->colpriv_count; i++)
    {
        colpriv_row = (COLPRIV_ROW *)pstmt->baseColpriv_q.q_prev;
        if (colpriv_row->catalog)
            MEfree(colpriv_row->catalog);
        if (colpriv_row->ownerName)
            MEfree(colpriv_row->ownerName);
        if (colpriv_row->tableName)
            MEfree(colpriv_row->tableName);
        if (colpriv_row->grantor)
            MEfree(colpriv_row->grantor);
        if (colpriv_row->grantee)
            MEfree(colpriv_row->grantee);
        if (colpriv_row->privilege)
            MEfree(colpriv_row->privilege);
        if (colpriv_row->is_grantable)
            MEfree(colpriv_row->is_grantable);
        QUremove((QUEUE *)colpriv_row);
        MEfree((PTR)colpriv_row);
    }
    pstmt->colpriv_count = 0;
    return;
}


/*
**  GenerateCursor
**
**  Generate internal cursor name:
**
**  On entry: pstmt-->statment block.
**
**  On exit:  pstmt->szCursor contains a name generated from the cursor number.
**
**  Returns:  SQL_SUCCESS
**
**    15-apr-2005 (lunbr01,loera01) Bug 114319
**          Use connection-level, rather than statement-level, high cursor id
**          as the basis for assigning the next cursor id.  This will prevent
**          the problem of assigning duplicate ids.
*/
void GenerateCursor(
    LPSTMT pstmt)
{
    TRACE_INTL (pstmt, "GenerateCursor");
    if (STlength(pstmt->szCursor) == 0)
    {
        pstmt->pdbcOwner->iCursor++;
        pstmt->iCursor = pstmt->pdbcOwner->iCursor;
        STcopy (CURSOR_PREFIX, pstmt->szCursor);
        STprintf (
            pstmt->szCursor + sizeof(CURSOR_PREFIX) - 1,
            "%x_%05.5d",
            pstmt, pstmt->iCursor);
    }
}
/*
**  ConvertParmMarkers
**
**  Convert ? to ~V 
**
**  On entry: szSqlStr-->SQL statement text.
**            cbSqlStr = length of SQL statement text.
**            szAPISqlStr -->converted SQL stmt text.
**            cbAPISqlStr = length of converted SQL stmt text  
**
**  On exit:  Statement is converted in buffer pointed to by szAPISqlStr
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**  date        programmer          description
**
**  01/14/1997  Jean Teng                       convert ? to ~V   
*/

RETCODE ConvertParmMarkers(
    CHAR        * szSqlStr,
    SQLINTEGER      * cbSqlStr,
    CHAR        * szAPISqlStr,
    SDWORD      * cbAPISqlStr)
{
    CHAR     * pFrom;
    CHAR     * pTo;
    II_BOOL    fQuote = FALSE;
    i2         beginBracket = 0;
    i2         spaceCount = 0;
    CHAR       bQuote;
    SQLINTEGER     length=*cbSqlStr;
    BOOL       fComment = FALSE;
    i4         i;

    pFrom = szSqlStr;
    pTo       = szAPISqlStr;
    
    while (*pFrom != EOS  &&  length)
    {
         /*
         ** Comments are ignored, but a space is inserted after the beginning
         ** and end of the comment markers.  This is necessary because
         ** ParstToken() requires all tokens to separated by spaces.
         */
         if (!fComment && (length >= 4) && (!STbcompare(pFrom,2,"/*",2,TRUE)))
         {
             fComment = TRUE;
             CMcpychar(" ", pTo);
             CMnext(pTo);
             for (i = 0; i < 2; i++)
             {
                 CMcpychar(pFrom, pTo);
                 CMnext(pFrom);
                 CMnext(pTo);
                 length--;
             }
             continue;
         }
 
         if (fComment)
         {
             if (length >= 2 && (!STbcompare(pFrom,2,"*/",2,TRUE))) 
             {
                 fComment = FALSE;
                 for (i = 0; i < 2; i++)
                 {
                     CMcpychar(pFrom, pTo);
                     CMnext(pFrom);
                     CMnext(pTo);
                     length--;
                 }
                 CMcpychar(" ", pTo);
                 CMnext(pTo);
             }
             else
             {
                 CMcpychar(pFrom, pTo);
                 CMnext(pFrom);
                 CMnext(pTo);
                 length--;
             }
             continue;
         }
 

        if (fQuote)   /* ignore chars if in quote */
        {
            if (!CMcmpcase(pFrom,&bQuote))  /* end of literal value */ 
            {
                fQuote = FALSE;
                bQuote = 0;
            }
            CMcpychar(pFrom,pTo); 
            spaceCount = 0;
        }
        else
        {
            if (!CMcmpcase(pFrom,"'") || !CMcmpcase(pFrom,"\"") ) /* starting a char string */
            {
                fQuote = TRUE;
                CMcpychar(pFrom,&bQuote);
                CMcpychar(pFrom,pTo); 
                spaceCount = 0;
            }
            else
            {
                if (!CMcmpcase(pFrom,"?"))
                {
                    STcat(pTo," ~V ");
                    CMnext(pTo); 
                    CMnext(pTo);
                    CMnext(pTo);
                    spaceCount = 0;
                }
                else
                {
                    /*
                    ** Multiple spaces between left and right
                    ** brackets are converted to single spaces.  This
                    ** allows scalar conversions to be more predictable.
                    */
                    if (beginBracket && !CMcmpcase(pFrom, " "))
                        spaceCount++;
                    else
                        spaceCount = 0;
                    if (spaceCount < 2)
                        CMcpychar(pFrom,pTo);
                }
            }
            if (!CMcmpcase(pFrom,"{"))
                beginBracket++;
            else if (!CMcmpcase(pFrom,"}"))
                beginBracket--;
        }

        CMnext(pFrom);
        if (spaceCount < 2)
            CMnext(pTo);
        length--;
    }
    CMcpychar(nt,pTo); 
    
    if (*cbSqlStr == SQL_NTS)
    *cbAPISqlStr = SQL_NTS;
    else if (*cbSqlStr<0)  /* if length is bad, pass on the bad length */ 
        *cbAPISqlStr = (SDWORD)*cbSqlStr;  /* where is will be caught downstream */
    else
        *cbAPISqlStr = (SDWORD)STlength (szAPISqlStr);
    
    return(SQL_SUCCESS);
}

/*
**  ConvertEscAndMarkers
**
**  Convert time, interval, timestamp, and date ODBC escape sequences to
**  ISO format. Also convert "?" to "~V".
**
**  On entry: szSqlStr-->SQL statement text.
**            cbSqlStr = length of SQL statement text.
**            szAPISqlStr -->converted SQL stmt text.
**            cbAPISqlStr = length of converted SQL stmt text  
**
**  On exit:  Statement is converted in buffer pointed to by szAPISqlStr
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**  History:
**    23-Oct-2006 (Ralph Loen)  SIR 116477
**      Created.
**    25-Feb-2007 (Ralph Loen) Bug 119975
**      Allow for embedded braces.  Retain braces if the escape sequence
**      is not date/time or interval.
**   17-Nov-2008 (Ralph Loen) SIR 121228
**      The previous approach rewrote szSqlStr, which caused a segmentation 
**      violation on Linux.  The szAPISqlStr string is written instead.  
**      The szAPISqlStr string is allocated from the ODBC driver rather 
**      than the calling application.
**  07-May-2009 (Ralph Loen) Bug 122038
**      Allow for single-quoted literals. 
**  29-Jan-2010 (Ralph Loen) Bug 123175
**      Support blank spaces as 9999-12-31.
*/

RETCODE ConvertEscAndMarkers(
    CHAR        * szSqlStr,
    SQLINTEGER      * cbSqlStr,
    CHAR        * szAPISqlStr,
    SDWORD      * cbAPISqlStr,
    SDWORD       dateConnLevel)
{
    CHAR     * pFrom = szSqlStr;
    CHAR     * pTo = szAPISqlStr;
    CHAR     *p;
    BOOL     dateTime=FALSE;
    SQLINTEGER     length=*cbSqlStr;
    SQLINTEGER i;
    BOOL       isDate = FALSE;
    BOOL       isTimestamp = FALSE;
    SQLSMALLINT underscoreCount=0;
    BOOL    fQuote = FALSE;
    i2      beginBracket = 0;
    i2      spaceCount = 0;
    char    bQuote;
    UWORD   len = 0;
    BOOL    fComment = FALSE;
     
    while (*pFrom != EOS  && length)
    {
        /*
        ** Comments are ignored, but a space is inserted after the beginning
        ** and end of the comment markers.  This is necessary because
        ** ParstToken() requires all tokens to separated by spaces.
        */
        if (!fComment && (length >= 4) && (!STbcompare(pFrom,2,"/*",2,TRUE)))
        {
            fComment = TRUE;
            CMcpychar(" ", pTo);
            CMnext(pTo);
            for (i = 0; i < 2; i++)
            {
                CMcpychar(pFrom, pTo);
                CMnext(pFrom);
                CMnext(pTo);
                length--;
            }
            continue;
        }

        if (fComment)
        {
            if (length >= 2 && (!STbcompare(pFrom,2,"*/",2,TRUE)))
            {
                fComment = FALSE;
                for (i = 0; i < 2; i++)
                {
                    CMcpychar(pFrom, pTo);
                    CMnext(pFrom);
                    CMnext(pTo);
                    length--;
                }
                CMcpychar(" ", pTo);
                CMnext(pTo);
            }
            else
            {
                CMcpychar(pFrom, pTo);
                CMnext(pFrom);
                CMnext(pTo);
                length--;
            }
            continue;
        }

        if (fQuote)   /* ignore chars if in quote */
        {
            if (!CMcmpcase(pFrom,&bQuote))  /* end of literal value */ 
            {
                fQuote = FALSE;
                bQuote = 0;
            }
            CMcpychar(pFrom,pTo); 
        }
        else if ((!CMcmpcase(pFrom,"'") || !CMcmpcase(pFrom,"\""))
           && !dateTime ) /* starting a char string */
        {
                fQuote = TRUE;
                CMcpychar(pFrom,&bQuote);
                CMcpychar(pFrom,pTo); 
        }
        else if (!CMcmpcase(pFrom,"?"))
        {
            MEcopy(" ~V ", 4, pTo);
            CMnext(pFrom);
            length--;
            CMnext(pTo);
            CMnext(pTo);
            CMnext(pTo);
            CMnext(pTo);
        }
        else if (!CMcmpcase(pFrom,"{"))
        {
            /*
            ** Multiple spaces between left and right
            ** brackets are converted to single spaces.  This
            ** allows scalar conversions to be more predictable.
            */
            beginBracket++;
            p = pFrom;
            CMnext(p);
            while(CMwhite(p)) CMnext(p);
            if (!CMcmpnocase(p, "d"))
            {
                dateTime = TRUE;
                isDate = TRUE;
                CMnext(p);
                while (CMwhite(p)) CMnext(p);
        
                if (!CMcmpcase(p, "'"))
                {
                    if (dateConnLevel > IIAPI_LEVEL_3)
                    {
                        MEcopy("DATE", 4, pTo);
                        for (i = 0; i < 4; i++)
                            CMnext(pTo);
                    }
                    CMnext(pFrom);
                    length--;
                    while(CMwhite(pFrom))
                    {
                        CMnext(pFrom);
                        length--;
                    }
                    CMnext(pFrom);
                    length--;
                    while (CMwhite(pFrom))
                    {
                        CMnext(pFrom);
                        length--;
                    }
                }
            } /* if "{" */
            else
            {
                p = pFrom;
                CMnext(p);
                while(CMwhite(p)) CMnext(p);
                if (!STbcompare (p, 2, "ts", 2, TRUE)) 
                {
                    dateTime = TRUE;
                    isTimestamp = TRUE;
                    CMnext(p);
                    CMnext(p);
                    while (CMwhite(p)) CMnext(p);
                        
                    if (!CMcmpcase(p, "'"))
                    {
                        if (dateConnLevel > IIAPI_LEVEL_3)
                        {

                            MEcopy("TIMESTAMP", 9, pTo);
                            for (i = 0; i < 9; i++)
                                CMnext(pTo);
                        }
                        CMnext(pFrom);
                        length--;
                        while(CMwhite(pFrom))
                        {
                            CMnext(pFrom);
                            length--;
                        }
                        CMnext(pFrom); CMnext(pFrom);
                        length -= 2;
                        while (CMwhite(pFrom))
                        {   
                            CMnext(pFrom);
                            length--;
                        }
                    } /* if "'' */
                } /* if "ts" */
                else
                {
                    p = pFrom;
                    CMnext(p);
                    while(CMwhite(p)) CMnext(p);
                    if (!CMcmpnocase(p, "t"))
                    {
                        dateTime = TRUE;
                        CMnext(p);
                        while (CMwhite(p)) CMnext(p);
                
                        if (!CMcmpcase(p, "'"))
                        {
                            MEcopy("TIME", 4, pTo);
                            for (i = 0; i < 4; i++)
                                CMnext(pTo);
                            CMnext(pFrom);
                            length--;
                            while(CMwhite(pFrom))
                            {
                                CMnext(pFrom);
                                length--;
                            }
                            CMnext(pFrom);
                            length--;
                            while (CMwhite(pFrom))
                            {   
                                CMnext(pFrom);
                                length--;
                            }
                        }
                    }
                    else
                    {
                        p = pFrom;
                        CMnext(p);
                        while(CMwhite(p)) CMnext(p);
                        if (!STbcompare (p, 8, "interval", 8, TRUE)) 
                        {
                            dateTime = TRUE;
                            for (i = 0; i < 8; i++)
                                CMnext(p);
                            while(CMwhite(p)) CMnext(p);
                            if (!CMcmpcase(p, "'"))
                            {                    
                                CMnext(pFrom);
                                length--;
                                while(CMwhite(pFrom))
                                {
                                    CMnext(pFrom);
                                    length--;
                                }
                                for (i = 0; i < 8; i++)
                                {
                                    CMnext(pFrom);
                                    length --;
                                }
                                while (CMwhite(pFrom))
                                {   
                                    CMnext(pFrom);
                                    length--;
                                }
                           } /* if "'" */
                        } /* if { interval " */
                    } /* if "{ t" */
                } /* if "{ ts " */
            } /* if "{ d" */ 
        } /* if (!CMcmpcase(pFrom,"}")) */


        /*
        ** 9999-12-31 date is a special case for the Ingres ODBC.
        ** Ingres allows blank dates, but the ODBC escape sequence
        ** doesn't allow blank dates.  
        **
        ** The ODBC driver assumes all dates and timestamps with
        ** a date of 9999-12-31 represents a blank date, if
        ** Ingres syntax is enforced.  Ingres syntax rules are
        ** followed if the dateConnLevel is < IIAPI_LEVEL_4.
        **
        ** By default, the dateConnLevel is the same as the
        ** fAPILevel, but may be overridden by the "Coerce to
        ** Ingres date syntax" DSN attribute or the
        ** SendDateTimeAsIngresDate connection string attribute.
        */
        if ((isTimestamp || isDate) && 
            (dateConnLevel < IIAPI_LEVEL_4))
        {
            if ( !STbcompare(pFrom,4,"9999",4,TRUE) &&
                !STbcompare(pFrom+5,2,"12",2,TRUE) &&
                !STbcompare(pFrom+8,2,"31",2,TRUE) )
            {
                for (len = 0, p = pFrom; *p != '\'' && 
                    *p != '}' && *p != '\0' && 
                    len < DATE_MAX_LEN; len++, p++);
               
                MEfill(len, ' ', pTo );
                while (len > 0)
		{
		    CMnext(pFrom);
                    CMnext(pTo);
                    length--;
                    len--;
                }
            }
        }


        if ((isDate || isTimestamp) && (dateConnLevel < IIAPI_LEVEL_4) 
            && pFrom[0] == '-')
        {
            CMcpychar("_", pTo);
            CMnext(pTo);
            underscoreCount++;
            if (underscoreCount == 2)
            {
                isDate = FALSE;
                underscoreCount = 0;
            }
        }
        else if ((!dateTime) || (CMcmpcase(pFrom,"{") && CMcmpcase(pFrom,"}")))
        {
            if (beginBracket && !CMcmpcase(pFrom, " "))      
                spaceCount++;
            else
                spaceCount = 0;
            if (spaceCount < 2)
            {
                CMcpychar(pFrom,pTo); 
                CMnext(pTo);  
            }
        }

        if (!CMcmpcase(pFrom,"}"))
        {
            dateTime = FALSE;
            if (!fQuote)
                beginBracket--;
        }

        if (length)
        {
            CMnext(pFrom);
            length--;
        }
    } /* while (*pFrom != EOS  && length) */
    CMcpychar(nt,pTo); 
    
    if (*cbSqlStr == SQL_NTS)
    *cbAPISqlStr = SQL_NTS;
    else if (*cbSqlStr<0)  /* if length is bad, pass on the bad length */ 
        *cbAPISqlStr = (SDWORD)*cbSqlStr;  /* where is will be caught downstream */
    else
        *cbAPISqlStr = (SDWORD)STlength (szAPISqlStr);

    return(SQL_SUCCESS);

}


/*
**  ConvertThreePartNames
**
**  Convert "ownername"."tablename"."colname" to "tablename"."colname"
**       or  ownername.tablename.colname      to  tablename.colname
**
**  On entry: szSqlStr-->SQL statement text.
**            cbSqlStr = length of SQL statement text.
**            szAPISqlStr -->converted SQL stmt text.
**            cbAPISqlStr = length of converted SQL stmt text  
**
**  On exit:  Statement is converted in buffer pointed to by szSqlStr
**
**  Returns:  void
**  date        programmer          description
**
**  12/12/1997  Dave Thole                      creation   
*/

static void  ConvertThreePartNames(CHAR     * szSqlStr)
{
 CHAR     * p = szSqlStr;
 CHAR     * pOwnername;
 SQLINTEGER     cbOwnername;

 while (*p)
   { p = ScanPastSpaces(p);
     if (!(*p))   break;       /* stop scan if EOS hit */

     if (CMcmpcase(p, "\'")==0  ||  CMcmpcase(p, "\"")==0)
        { pOwnername=p;
          p=ScanQuotedString(p);
        }
     else if (CMalpha(p)  ||  CMcmpcase(p, "_")==0)
        { pOwnername=p;
          p=ScanRegularIdentifier(p);
        }
     else 
	{ p++;
	  continue;
	}

     if (!(*p))   break;       /* stop scan if EOS hit */

     p = ScanPastSpaces(p);
     if (*p != '.')            /* if not "owner." then start over */
	continue;

     p++;                      /* skip past period   */
     cbOwnername=p-pOwnername; /* length of "owner." */

     p = ScanPastSpaces(p);
     if (!(*p))   break;       /* stop scan if EOS hit */

                               /* scan for "owner.table" */
     if (CMcmpcase(p, "\'")==0  ||  CMcmpcase(p, "\"")==0)
          p=ScanQuotedString(p);
     else if (CMalpha(p)  ||  CMcmpcase(p, "_")==0)
          p=ScanRegularIdentifier(p);
     else 
	  continue;            /* if not "owner.table" then start over */

     p = ScanPastSpaces(p);
     if (!(*p))   break;       /* stop scan if EOS hit */

     if (*p=='.')              /* "owner.table." found! */
        MEfill (cbOwnername, ' ', pOwnername);
     
     continue;                 /* start over */
   }  /* end while (*p) */

return;
}


/*
**  ParseCommand
**
**  1. Parse first word of SQL syntax string for command.
**  2. Compute length if a null terminated string.
**  3. Scan for escape sequences and translate to native DBMS syntax.
**  4. Scan and count parameter markers.
**  5. Translate selected ODBC syntax into equivalents:
**     CREATE INDEX      remove owner on index name if any.
**     CREATE UPDATE     remove column list if any.
**
**  On entry: pstmt   -->statement block
**            szSqlStr-->SQL syntax
**            cbSqlStr = length of SQL syntax or SQL_NTS
**
**  On exit:  pstmt->fCommand = command flag:
**
**              CMD_SELECT
**              CMD_INSERT
**              CMD_UPDATE
**              CMD_DELETE
**              CMD_DDL
**              CMD_EXT (with fCatalog = CATLG_SETS)
**              0 if none of the above.
**
**            pstmt->icolParmLast = number of parameter markers
**
**            pstmt contains error info if an error.
**
**  Returns:  SQL_SUCCESS.
**            SQL_SUCCESS_WITH_INFO.
**            SQL_ERROR.
**  date        programmer          description
**
**  01/14/1997  Jean Teng                       use ~V for parameter markers  
**      10-feb-2004 (loera01) SIR 111427
**          Remove end semicolons, if present.
**    08-jun-2005 (loera01) Bug 114631
**          Check for the TOK_MODIFY token and process as for TOK_ALTER and 
**          TOK_DROP.
*/
RETCODE ParseCommand(
    LPSTMT      pstmt,
    CHAR     *  szSqlStr,
    SDWORD      cbSqlStr,
    BOOL        rePrepared)
{
    CHAR     *  p;
    CHAR     *  p1;
    CHAR     *  pSpace;
    UWORD       c       = 0;
    SDWORD      cb      = 0;
    SQLINTEGER      cbSpace = 0;
    BOOL        fView   = FALSE;
    CHAR     *  sz;
    /*UWORD len, i; */
    CHAR     *  pTemp; 
    TOKEN token;
    CHAR     *  pNext;

    TRACE_INTL (pstmt, "ParseCommand");


    /*
    **  Remove end semicolon, if any, and get actual length of syntax:
    */
    if (removeEndSemicolon(szSqlStr) || (cbSqlStr == SQL_NTS))
        cb = (UWORD)STlength (szSqlStr);
    else if (cbSqlStr > 0)
        cb = cbSqlStr;
    if (cb == 0)
        return ErrState (SQL_HY090, pstmt); /* invalid string length */

    if (pstmt->fStatus & STMT_SQLEXECUTE)
         FreeSqlStr(pstmt);

    /*
    **  Make a copy of the syntax to work with,
    **  and convert all "white space" (\n,\r,\t,etc),
    **  quoted literals, and quoted identifiers
    **  to space (' ') characters:
    */
    /* alloc a buffer with a safety margin for escape substitutions */
    if (!rePrepared)
    {
        /* alloc a buffer with a safety margin for escape substitutions */
         pstmt->szSqlStr = MEreqmem(0, cbSqlStr + 
         CountLiteralItems(cbSqlStr, (CHAR*)szSqlStr, pstmt->dateConnLevel) 
             + 50, TRUE, NULL);
    }

    if (pstmt->szSqlStr == NULL)
        return ErrState (SQL_HY001, pstmt, F_OD0013_IDS_SQL_SAVE);

    MEcopy (szSqlStr, cb, pstmt->szSqlStr);
    pstmt->cbSqlStr = cb;

    ParseSpace (pstmt->szSqlStr, TRUE);  
      /* Convert whitespace to spaces;  Uppercase the syntax */

    /*
    **  Get rid of leading parentheses:
    */
    p = pstmt->szSqlStr;
    while (CMspace(p) || !CMcmpcase(p,"(") )
    {
        CMcpychar(" ",p); 
        CMnext(p); 
    }

    /*
    **  Scan syntax and count parameter markers:
    */
    pstmt->icolParmLast = 0;        /* count of parameter markers = 0 */
    p = pstmt->szSqlStr;
    /*
    while (*p)
        if (*p++ == '?')
            pstmt->icolParmLast++;
    */
    while ( *p != EOS )
    {
        p1 = p;
        CMnext(p1); 
        if ( !CMcmpcase(p,"~") && !CMcmpcase(p1,"V") )
            pstmt->icolParmLast++;   /* increment count of parameter markers */
        CMnext(p);
    }

    pstmt->fStatus &= ~(STMT_WHERE_CURRENT_OF_CURSOR | /* make sure clear */
                        STMT_CANBEPREPARED);
    /*
    **  Find first token in syntax and
    **  set command flag for SQL DDL and DML commands:
    */
    p = strtok (pstmt->szSqlStr, " ");
    token = ParseToken(p); 
    switch (token)
    {
    case TOK_SELECT:

    pstmt->fCommand = CMD_SELECT;
    pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
    break;

    case TOK_INSERT:

    pstmt->fCommand = CMD_INSERT;
    pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */

        break;

    case TOK_UPDATE:
    case TOK_DELETE:

		if (token == TOK_UPDATE)
		   pstmt->fCommand = CMD_UPDATE;
		else
		   pstmt->fCommand = CMD_DELETE;
		pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */

		p += 7; 
		p = STstrindex(p," WHERE ", 0, FALSE);
		if (p)
		{
			pNext = p;  /* save addr to blank out the where phase later */
			p += 7;
		    while (CMspace(p)) CMnext(p); 
			
			if (p)
			{
			    if (!STbcompare(p,(i4)STlength(p),"CURRENT",6,TRUE))
				{
					p += 7;
		            while (CMspace(p)) CMnext(p); 

					if (p)
					{
						if (!STbcompare(p,(i4)STlength(p),"OF ",3,TRUE))
						{
							pstmt->fStatus |= STMT_WHERE_CURRENT_OF_CURSOR;
							/* save cursor name */
							p += 3;
							while (CMspace(p)) CMnext(p); 
					        if (p)
							{
								/* remove trailing blanks in cursor name */
								pTemp = p;
								while (!CMspace(pTemp) && *pTemp != EOS ) /* not space and not null */
									    CMnext(pTemp);
								CMcpychar(nt,pTemp);
								STcopy (p, pstmt->szCursor);
								/* blank out "where current of ..." */
								pstmt->cbSqlStr -= (SDWORD)STlength (pNext);
								CMcpychar(nt,pNext); 
							}
						}
					}
				}
			}
		}
	break;

	case TOK_ESC_BRACECALL:
		pstmt->fCommand = CMD_EXECPROC;
		break;

    case TOK_ESC_BRACE:
		p = strtok (NULL, " ");
		switch (ParseToken (p))
		{
		case TOK_ESC_CALL:
		    pstmt->fCommand = CMD_EXECPROC;
                    break;

		case TOK_ESC_TILDEV:
		    pstmt->fCommand = CMD_EXECPROC;
                    pstmt->fStatus |= STMT_RETPROC;       
		    break;
		}
		break;

    case TOK_CREATE:

	pstmt->fCommand = CMD_DDL;
        pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
	p = strtok (NULL, " ");

	switch (ParseToken (p))
	{
	case TOK_VIEW:

	    fView = TRUE;
	    break;

    case TOK_INDEX:
            /*
            **  Find owner name if specified on index:
            */
            if ((pSpace = strtok (NULL, " ")) != NULL)
                if ((p = STindex (pSpace, ".",0)) != NULL)
                    cbSpace = p - pSpace + 1;
            break;
        }
        break;

    case TOK_ALTER:
    case TOK_DROP:
    case TOK_MODIFY:

	pstmt->fCommand = CMD_DDL;
        pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
	break;

    case TOK_BEGIN_COMMENT:
        break;

    case TOK_END_COMMENT:
        break;


    case TOK_GRANT:
        /*
        **  Find column list in GRANT UPDATE for IDMS
        **  (we will blank it out):
        */
        pstmt->fCommand = CMD_DDL;
        pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
        break;


    case TOK_SET:

	p = strtok (NULL, " ");
	switch (ParseToken (p))
	{
	case TOK_SESSION:

	    pstmt->fCommand = CMD_SESSION;
	    p = strtok (NULL, " ");
	    if (ParseToken (p) == TOK_READ)
	    {   p = strtok (NULL, " ");
	        if (ParseToken (p) == TOK_WRITE)
	            pstmt->fCommand = CMD_SESSION_READ_WRITE;
	    }
	    break;

	case TOK_TRANSACTION:

	    pstmt->fCommand = CMD_TRANSACT_non_READ_WRITE;
	    p = strtok (NULL, " ");
	    if (ParseToken (p) == TOK_READ)
	    {   p = strtok (NULL, " ");
	        if (ParseToken (p) == TOK_WRITE)
	            pstmt->fCommand = CMD_TRANSACT_READ_WRITE;
	    }
	    break;

	case TOK_AUTOCOMMIT:
	    p = strtok (NULL, " ");
	    switch (ParseToken (p))
	    {
	    case TOK_OFF:
	    pstmt->fCommand = CMD_AUTOCOMMIT_OFF;
	    break;

	    case TOK_ON:
	    default:
	    pstmt->fCommand = CMD_AUTOCOMMIT_ON;
	    }
	    break;
	}
	break;

    case TOK_SUSPEND:
        p = strtok (NULL, " ");
        pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
        if (ParseToken (p) == TOK_SESSION)
            pstmt->fCommand = CMD_SUSPEND;
        break;

    case TOK_TRACE:

	return ParseTrace (pstmt);
	break;

    case TOK_COPY:
        return ErrState (SQL_HY000, pstmt, F_OD0054_IDS_ERR_NOT_CAPABLE);
		break;

    case TOK_COMMIT:
	pstmt->fCommand = CMD_COMMIT;
        pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
	break;

    case TOK_ROLLBACK:
	pstmt->fCommand = CMD_ROLLBACK;
        pstmt->fStatus |= STMT_CANBEPREPARED; /* can be PREPAREd/EXECUTEd */
	break;

	default:
        if (STbcompare (p, (i4)STlength(p),(char *) ODBC_STD_ESC, 4,TRUE) == 0)
        {   
            p1 = p;
            CMnext(p1); CMnext(p1); CMnext(p1); CMnext(p1);
            if ((sz = STindex (p1, ")",0)) != NULL)
            {
                CMnext(sz); 
                if ((sz = STindex (sz, ")", 0)) != NULL)
                {
                        /* len = sz + 1 - p; */
                        pTemp = MEreqmem(0, cb + 1, TRUE, NULL);
                        if (pTemp == NULL)
                                return ErrState (SQL_HY001, pstmt, F_OD0013_IDS_SQL_SAVE);
    /*
                        for (i = 0, sz = pTemp; i < len; i++)
                                if (p[i] != ' ') *sz++ = p[i];
                        *sz = EOS;
                        */
                        CMnext(sz);
                        CMcpychar(nt,sz);
                        STzapblank(p,pTemp);
                        if (STbcompare (pTemp,(i4)STlength(pTemp), (char *) ODBC_STD_ESC,sizeof(ODBC_STD_ESC),TRUE) == 0)
                                pstmt->fCommand = CMD_EXECPROC;
            MEfree(pTemp);
                }
            }
        }
		 break;
    }

    /*
    **  Restore saved syntax as it was passed,
    **  convert all white space chars to a space, except <CR>, <TAB>, 
    **  and <LF>.  Space out any unsupported syntax.
    */
    MEcopy (szSqlStr, pstmt->cbSqlStr, pstmt->szSqlStr);
    if (isServerClassINGRES(pstmt->pdbcOwner))
        /* 
        ** Convert whitespace to spaces, ignoring <CR>, <LF> or <TAB>.
        */
        ParseSpace (pstmt->szSqlStr, -1);  
    else
        /* 
        **  Convert whitespace to spaces only.
        */
        ParseSpace (pstmt->szSqlStr, FALSE);  

    if (cbSpace)
    {
        MEfill (cbSpace, ' ', pSpace);
    }

    /*
    **  Scan syntax for three-part qualified names
    **  of "ownername.tablename.columnname" generated
    **  by MS Visual Interdev and DBWEB and convert 
    **  them to "tablename.columnname".
    */
    if (pstmt->fOptions & OPT_CONVERT_3_PART_NAMES)
       ConvertThreePartNames(pstmt->szSqlStr);

    /*
    **  Scan syntax for ODBC escape sequences and translate.
    **  Note that this operates on the restored syntax, which
    **  can be in mixed case and contain quoted strings.
    */
    if (!(pstmt->fOptions & OPT_NOSCAN))
    {
        p = ParseEscape (pstmt, pstmt->szSqlStr);
        while (p)
        {
            cbSpace = 1;            /* flag as changed... */
            pNext = NULL;           /* assume dont need to expand */
            if (!ParseConvert (pstmt,p,&pNext,isaParameterFALSE, 
                pstmt->dateConnLevel))
            {
                /*
                **  Return invalid conversion as syntax error:
                */
                pstmt->sqlca.SQLCODE = -4;
                pstmt->sqlca.SQLERC  = 6001;
                pstmt->sqlca.SQLMCT  = 0;
                pstmt->sqlca.SQLSER = (II_INT4)(p - pstmt->szSqlStr);
                FreeSqlStr (pstmt);
                ErrSetToken (pstmt, szSqlStr, cbSqlStr);
                return ErrState (SQL_42000, pstmt, F_OD0041_IDS_ERR_ESCAPE);
            }

            p = ParseEscape(pstmt, NULL);
            if (p == NULL)  /* if done with this pass, do another for nested */
                p = ParseEscape (pstmt, pstmt->szSqlStr);    /* escapes */
        }  /* end while (p) loop */

        pstmt->cbSqlStr = (SDWORD)STlength (pstmt->szSqlStr);
    }

    return SQL_SUCCESS;
}


/*
**   ConvertDateFn 
**
**   convert ODBC time/date functions to Ingres specific
*/
static void ConvertDateFn (LPSTMT pstmt,LPSTR p, LPSTR pStart, UWORD cbEscape,
					LPSTR pDatePortion, CHAR *bNull)
{
    char * szTemp;	
    CHAR     * p1;

    while (CMwhite(p)) CMnext(p);
    if ( !CMcmpcase(p,"(") )
    {
        p1 = pStart + STlength(pStart);
        MEfill (cbEscape, ' ', p1 - cbEscape);
        CMnext(p);
        p1 = p +  STlength(p);
        CMcpychar(bNull,p1); /* Don't restore the saved character, again, zero */
        bNull[0] = '\0';    /* it, at the general restore it is checked for zero */
        szTemp = MEreqmem(0, STlength(pStart) + 8, TRUE, NULL);  /* alloc work area */
        STcopy(p, szTemp);
        MEcopy(" date_part('", 12, pStart);
        MEcopy(pDatePortion, 3, pStart+12);
        MEcopy("',", 2, pStart+15);
        STcopy(szTemp,pStart+17); 
        MEfree ((PTR)szTemp);                    /* free work area */
    }
    return;
}


/*
**   ConvertDateTs
**
**   convert ODBC timestamp to date('yyyy_mm_dd hh:mm:ss')
**
**   Notes:
**   The question arises for the {ts ...} timestamp case that if we are
**   emitting date(...) for the thin gateways then why not emit date(...)
**   for the {ts ...} for Ingres and the thick gateways also?
**   The reasoning for not making that change and leaving only the character
**   string of 'yyyy_mm_dd hh:mm:ss' for Ingres-type servers is in the
**   interest of upward compatibility.  If the target database field is
**   an Ingres date field then the character string is a valid value even
**   without the date function wrapper.
**
**   However, we worry about the case where the user is using a character
**   field (e.g. mycharfield) as to hold a timestamp and we suddenly start
**   emitting a date wrapper for {ts ...}.  Experiments with Ingres show
**   that if the expressions involving mycharfield and a date(  ) will
**   cause Ingres to try to implicitly cast mycharfield as a date field
**   and do a date-to-date operation with checking on a valid date value
**   in mycharfield.  This is different from the character operation today
**   with no validity checking.  The concern is that since this behavior
**   has been out there for many years with no complaints then we could
**   break existing applications that were counting on this idiosyncrasy
**   of the {ts ...} being treated as a character string and are now
**   failing on "'xyz' is not a valid date" error messages.

**   We're being conservative in the interest of upward compatibility
**   for existing applications.
*/
static void ConvertDateTs (LPSTMT pstmt,
                           LPSTR pStart,   /* ->  where "{ts" was */
                           UWORD cbEscape, /* length of "{" or "--(*" */
                           CHAR  *bNull)     /* bNull = saved character 
                                              after "}" that now has '\0' */
{
    CHAR * szTemp;
    CHAR * p;
    CHAR * p1;
    CHAR * szEscape;
    LPDBC pdbc = pstmt->pdbcOwner;

    p = ScanPastSpaces(pStart);   /* p->'yyyy-mm-dd hh:mm:ss'    */

    if (pstmt->dateConnLevel < IIAPI_LEVEL_4)
    {
        if (p[5]=='-'  &&  p[8]=='-') /* if yyyy-mm-dd format convert*/
                                      /* to yyyy_mm_dd format */
        {
            p[5]= '_';
            p[8]= '_';
        }
    }
    szEscape = pStart + STlength(pStart); /* szEscape->char just past "}" */
    szEscape = szEscape - cbEscape;       /* szEscape-> "}"               */
    memset (szEscape, ' ', cbEscape);     /* blank out "}"          */

    p1 = p + STlength(p);              /* p1->null term after yyyy_mm_dd ...*/
    *p1 = *bNull;                      /* restore the char after "}" */

    if (isServerClassAnIngresEngine(pdbc))  /* if Ingres or Ingres-based gateway*/
        return;                             /*     keep 'yyyy_mm_dd hh:mm:ss' */
    /* Move the date and whatever after it from pStart, so the date function */
    /* can be assembled there */
    szTemp = (CHAR*)MEreqmem(0, STlength(p) + 8, TRUE, NULL);
                                             /* alloc work area */
    *szEscape = ')';                         /* lay down the closing ')' of  */
                                             /* date( ) where the '}' use to be */
    strcpy(szTemp,p);                  /* save 'yyyy-mm-dd hh:mm:ss')...  */
    memcpy(pStart,"date(",5);          /* lay down "date("  */
    strcpy(pStart+5,szTemp);           /* lay down 'yyyy_mm...' and rest of string*/
    MEfree((PTR)szTemp);               /* free work area */ 

    return;
}

static void ConvertInterval (LPDBC pdbc,
                           LPSTR pStart,   /* ->  where "{ts" was */
                           UWORD cbEscape, /* length of "{" or "--(*" */
                           CHAR  *bNull)     /* bNull = saved character 
                                              after "}" that now has '\0' */
{
    CHAR * szTemp;
    CHAR * p;
    CHAR * p1;
	CHAR * p2;
    CHAR * szEscape;

    p = ScanPastSpaces(pStart);   /* p->'yyyy-mm-dd hh:mm:ss'    */

    if (p[5]=='-'  &&  p[8]=='-') /* if yyyy-mm-dd format convert*/
                                  /* to yyyy_mm_dd format */
       {p[5]= '_';
        p[8]= '_';
       }

    p2 = p;
    /*while (*p2)
    {
        if (*p2 == '\'')
            *p2 = ' ';
        CMnext(p2);
    } */
    szEscape = pStart + STlength(pStart); /* szEscape->char just past "}" */
    szEscape = szEscape - cbEscape;       /* szEscape-> "}"               */
    memset (szEscape, ' ', cbEscape);     /* blank out "}"          */

    p1 = p + STlength(p);              /* p1->null term after yyyy_mm_dd ...*/
    *p1 = *bNull;                      /* restore the char after "}" */

    if (!isServerClassAnIngresEngine(pdbc))  /* if Ingres or Ingres-based gateway*/
        return;                             /*     keep 'yyyy_mm_dd hh:mm:ss' */
    /* Move the date and whatever after it from pStart, so the date function */
    /* can be assembled there */
    szTemp = (CHAR*)MEreqmem(0, STlength(p) + 30, TRUE, NULL);
                                             /* alloc work area */
    *szEscape = ')';                         /* lay down the closing ')' of  */
                                             /* date( ) where the '}' use to be */
    strcpy(szTemp,p);                  /* save 'yyyy-mm-dd hh:mm:ss')...  */
    memcpy(pStart,"interval('seconds,",18);          /* lay down "interval("  */
    strcpy(pStart+18,szTemp);           /* lay down 'yyyy_mm...' and rest of string*/
    p = pStart + 18;
    while (*p)
    {
        CMnext(p);
        if (CMdigit(p))
        {
            while (*p)
            {
                CMnext(p);
                if (!CMdigit(p))
                {
                    *p = ' ';
                    break;
                }
            }
            break;
        }
    }
    CMnext(p);
    strcpy(p, "seconds')");
    MEfree((PTR)szTemp);               /* free work area */ 

    return;
}


/*
**  ParseConvert
**
**  Convert ODBC escape sequence into Ingres or OpenSQL syntax.
**  For most escapes seqences, this just removes the escape sequence.
**  Some are acutally converted.
**
**  On entry: szEscape-->ODBC escape sequence, null terminated.
**
**  On exit:  escape sequence is converted (in place) to Ingres syntax.
**
**  Returns:  TRUE  if success, FALSE if error
*/

BOOL ParseConvert (LPSTMT pstmt, LPSTR szEscape, LPSTR * pNextScan, BOOL isaParameter,
    UDWORD fAPILevel)
{
    UWORD       cbEscape = (UWORD)((!CMcmpcase(szEscape,"{")) ? 1 : 4);
    UWORD       cb = 1;
    CHAR     *  p;
    CHAR     *  p_temp;
    CHAR     *  q;
    CHAR        b[2];
    TOKEN       token;
    CHAR     *  pStart = (CHAR *) szEscape;
    i4          count;
    CHAR     *  p1;
    LPDBC       pdbc = pstmt->pdbcOwner;
    CHAR     *  bNull = pstmt->bNull;
    CHAR        buf[1000]; 
    i4         len;
    i4         i;
    i4         bufLen;

    typedef struct
    {
        SQLCHAR sqlType[25];
        SQLCHAR scalar[25];
    }  sqlToScalar;

    sqlToScalar sqlToScalarTbl[] = 
    {
        { "SQL_BIGINT",	"int8" },
        { "SQL_BINARY",	"byte" },
        { "SQL_CHAR",	"char" },
        { "SQL_DECIMAL","decimal" },
        { "SQL_DOUBLE",	"float8" },
        { "SQL_FLOAT",	"float4" },
        { "SQL_INTEGER","int2" },
        { "SQL_LONGVARBINARY", "long_varbyte" },
        { "SQL_LONGVARCHAR", "long_varchar" },
        { "SQL_NUMERIC", "decimal" },
        { "SQL_REAL", "float4" },
        { "SQL_SMALLINT", "int2" },
        { "SQL_DATE", "date" },
        { "SQL_TIME", "date" },
        { "SQL_TIMESTAMP", "date" },
        { "SQL_TINYINT", "int1" },
        { "SQL_VARBINARY", "varbyte" },
        { "SQL_VARCHAR", "varchar" },
        { "SQL_WCHAR", "nchar" },
        { "SQL_WVARCHAR", "nvarchar" },
        { "SQL_INTERVAL_YEAR", "interval" },
        { "SQL_INTERVAL_DAY", "interval" },
        { "SQL_INTERVAL_HOUR", "interval" },
        { "SQL_INTERVAL_MINUTE", "interval" },
        { "SQL_INTERVAL_SECOND", "interval" }
    };
#define sqlToScalarTblMax sizeof(sqlToScalarTbl) / sizeof(sqlToScalarTbl[0])

    TRACE_INTL(pstmt,"ParseConvert");
    /*
    **  Set ODBC escape initiator to spaces:
    */
    if (cbEscape == 4)  /* if "--(*vendor(Microsoft),product(ODBC) " format */
    {
        p = STindex (szEscape + 4, ")", 0);
        p = STindex (p + 1, ")", 0);
        cb = (UWORD)(p - szEscape + 1);
    }
    MEfill (cb, ' ', szEscape);  /* blank out "{" or "--(*ven..." prefix" */
    while (CMwhite(szEscape)) CMnext(szEscape);  /* scan past blanked prefix */

    /*
    **  Extract token containing type of literal:
    */
    for (p = szEscape, cb = 0; *p != EOS; CMnext(p), cb++)
    {
        if (!CMcmpcase(p,"'") || CMwhite(p) ) 
            break;
    }

    CMcpychar(p,b); 
    CMcpychar(nt,p);
    token = ParseToken (szEscape);
    CMcpychar(b,p); 

    /*
    **  Set escape type to spaces and
    **  convert escape sequence value:
    */
    MEfill (cb, ' ', szEscape);

    switch (token)
    {
    case TOK_ESC_DATE:
    case TOK_ESC_TIMESTAMP:
        p=ScanPastSpaces(p);
        if (*p=='\'')
            p++;
    
        /*
        ** 9999-12-31 date is a special case for the Ingres ODBC.
        ** Ingres allows blank dates, but the ODBC escape sequence
        ** doesn't allow blank dates.  
        **
        ** The ODBC driver assumes all dates and timestamps with
        ** a date of 9999-12-31 represents a blank date, if
        ** Ingres syntax is enforced.  Ingres syntax rules are
        ** followed if the dateConnLevel is < IIAPI_LEVEL_4.
        **
        ** By default, the dateConnLevel is the same as the
        ** fAPILevel, but may be overridden by the "Coerce to
        ** Ingres date syntax" DSN attribute or the
        ** SendDateTimeAsIngresDate connection string attribute.
        */
        if ( !STbcompare(p,4,"9999",4,TRUE) && 
             !STbcompare(p+5,2,"12",2,TRUE) &&
             !STbcompare(p+8,2,"31",2,TRUE) &&
             fAPILevel < IIAPI_LEVEL_4)
        {
            for (len = 0, p_temp = p; *p_temp != '\'' && 
                *p_temp != '}' && *p_temp != '\0' && len < DATE_MAX_LEN; 
                len++, p_temp++);

            MEfill(len, ' ', p );
            break;
        }
    
        /* 
        ** If the literal is in a parameter then convert to yyyy_mm_dd format
        ** if the date connection level warrants.
        **
        ** Otherwise, if literal is in a command string and the server is 
        ** Ingres or Ingres-based server (VSAM, IMS, RMS), convert to 
        ** yyyy_mm_dd format if the date connection level warrants.
        **  
        ** In all other cases, leave in yyyy-mm-dd format for now.
        */
        if (token == TOK_ESC_DATE && pstmt->dateConnLevel < IIAPI_LEVEL_4 && 
            (isaParameter || isServerClassAnIngresEngine(pdbc))) 
        {
            p[4]= '_';
            p[7]= '_';
        }
        else if (pstmt->fAPILevel < IIAPI_LEVEL_4 && (isaParameter || 
            isServerClassAnIngresEngine(pdbc)))
        {
            p[4]= '_';
            p[7]= '_';
        }
    
        if (token == TOK_ESC_DATE)
        /* 
        **  Parameters of Ingres class with a date connection level of
        **  IIAPI_LEVEL_3 or less are formatted as yyyy_mm_dd.
        ** 
        **  Otherwise, the format yyyy-mm-dd is used.
        */
        break;
    
        while (*p  && !CMspace(p))  /* search for the ' ' in "yyyy_mm_dd hh:mm:ss.ffffff" */
           p++;
    
    /* fall through for TOK_ESC_TIMESTAMP */
    
    case TOK_ESC_TIME:
        if (pstmt->fAPILevel < IIAPI_LEVEL_4)
        {
            while (*p != '\0')  /* scan for the "." in hh:mm:ss.fffffff  */
            {
                if (*p == '.')	/* milliseconds have to be ignored since
                                ** they are not valid for Ingres TIME
                                ** format in pre-ISO databases */
                {
                    while ( ('0' <= (*p) && (*p) <= '9') || (*p == '.') )
                    {
                        *p = ' ';    /* blank out the ".ffffff" */
                        p++;
                    }
                    break;
                }
                p++;
            }  /* end while loop */
        }
    
        /* If the {ts ..} literal is in a command string then 
        ** possibly build "date('yyyy_mm_dd hh:mm:ss')"
        */
    if (token==TOK_ESC_TIMESTAMP  &&  !isaParameter)
    {
        ConvertDateTs (pstmt,
                       pStart,   /* ->  where "{ts" was      */
                       cbEscape, /* length of "{" or "--(*"  */
                       bNull);   /* saved char after the "}" */
        *pNextScan = pStart;     /* where to resume escape scan */
        return TRUE;
    }
    break;
    
    case TOK_ESC_OJ:
    {
        static CHAR     * OJKeywordList[]=
                            {"LEFT","RIGHT","FULL","OUTER","JOIN","ON",NULL};
        CHAR     *  pOJ;
        CHAR     *  pEV;
    
        if (isServerClassAnIngresEngine(pdbc) || isServerClassDCOM(pdbc) || isServerClassDB2(pdbc) || isServerClassIDMS(pdbc))  /* if INGRES, VSAM, IMS, RMS */
            /* added DCOM, DB2 and IDMS for SIR11383 */
           {for (; *p != EOS; CMnext(p) )
            {
                if (STbcompare(p,(i4)STlength(p),"OUTER JOIN",10,TRUE) == 0  ||  
                    STbcompare(p,(i4)STlength(p),"OUTER  JOIN",11,TRUE) == 0)  /* give a little slack */
                {
                    MEfill(5,' ',p); /* blank out the word "OUTER" */
                    break;
                }
            }
        }  /* endif isServerClassAnIngresEngine */
        else if (isServerClassOPINGDT(pdbc)  ||  isServerClassORACLE(pdbc)) 
        {
            p=ScanPastSpaces(p);
            p=ScanSearchCondition(p, OJKeywordList);
            if (STbcompare(p,4,"LEFT",4,TRUE)==0)  /* blank out and skip over "LEFT" */
            { 
                MEfill(4,' ',p);
                p=ScanSearchCondition(p, OJKeywordList);
            }
            if (STbcompare(p,5,"OUTER",5,TRUE)!=0) /* "RIGHT", "FULL", or bad syntax */
               return FALSE;
            else MEfill(5,' ',p);        /* blank out "OUTER" */
            p=ScanSearchCondition(p, OJKeywordList);
            if (STbcompare(p,4,"JOIN",4,TRUE)!=0)  /* "RIGHT", "FULL", or bad syntax */
               return FALSE;
            else MEcopy(",   ",4,p);      /* replace "JOIN" with a comma */
            p=ScanSearchCondition(p, OJKeywordList);
            if (STbcompare(p,2,"ON",2,TRUE)!=0)    /* "ON" or bad syntax */
               return FALSE;
            else MEfill(2,' ',p);        /* blank out "ON" */
            p=ScanPastSpaces(p);          /* skip to search-condition after "ON"*/
            p1=ScanSearchCondition(p, NULL);  /* p1 should -> to "}" or "*)--" */
            /*
            count=p1-p;                   ** count = length of search-condition **
            while (count>0)               ** trim any trailing spaces **
               {if (*(p+count-1) == ' ') count--;
                else break;
               }
            */
            count = (i4)STtrmwhite(p);
            if (count<=0)  return FALSE;        /* if no ON clause, raise error*/
            pOJ = MEreqmem(0, count + 10, TRUE, NULL);  /* copy search-condition to memory */
            MEcopy(p,count,pOJ);
            
            p1 = pOJ+count;
            CMprev(p1,p);
            if (CMcmpcase(p1,")"))
            {
               CMnext(p1);
               MEcopy(" (+) \0",6,p1); /* add the Oracle-style OJ operator */
            }
            else                                 /* try to handle simple ON (a=b) case*/
               MEcopy(" (+)) \0",7,p1); /* add the Oracle-style OJ operator */
    
            MEfill(count,' ',p);          /* blank out the original search-condition*/
            szEscape += STlength (szEscape);  /* blank out the escape termination */
            MEfill (cbEscape, ' ', szEscape - cbEscape);
            p += STlength (p);              /* p->null termination after escape term */
            CMcpychar(bNull,p);             /* restore the original character */
    
            p=ScanPastSpaces(p);          /* skip to "WHERE"*/
            if (STbcompare(p,6,"WHERE ",6,TRUE)==0  ||
                STbcompare(p,6,"WHERE(",6,TRUE)==0)     /* "WHERE" clause present */
            {
                p+=5;              /* p->character after "WHERE" in original string*/
                count= (i4)STlength (p);   /* count = length of everything after "WHERE" */
                pEV = MEreqmem(0, count + 1, TRUE, NULL);  /* copy "everything" str to memory */
                MEcopy(p,count,pEV);       /* p1->post-WHERE "everything" string */
                p1 = pEV+count;
                CMcpychar(nt,p1);        /* make sure "everything" string terminated*/
                CMnext(p);
                CMcpychar(" ",p);          /* force a space after WHERE target string */
                p1=ScanPastSpaces(pEV);    /* skip past spaces on "everything" string */
                q=ScanSearchCondition(p1, NULL);  /* scan the WHERE clause */
                count=(i4)(q-p1);
                CMnext(p);
                CMcpychar("(",p);         /* WHERE (                      */
                if (count)                /* build up new WHERE           */
                   {MEcopy(p1,count,p);   /* WHERE (expression            */
                    p+=count;
                    MEcopy(") AND (",7,p); /*WHERE (expression) AND (     */
                    p+=7;
                   }
                CMcpychar(nt,p);   /* null terminated the target string     */
                STcat(p,pOJ);        /* WHERE (expression) AND (ojclause (+)  */
                STcat(p,") ");       /* WHERE (expression) AND (ojclause (+)) */
                STcat(p,q);          /* WHERE (expression) AND (ojclause (+)) ...*/
            }  /* end of WHERE present */
            else                              /* "WHERE" clause not present */
            {
                count=(i4)STlength(p);   /* count = length of everything after "WHERE" */
                pEV = MEreqmem(0, count + 1, TRUE, NULL);  /* copy "everything" str to memory */
                MEcopy(p,count,pEV);      /* p1->post-WHERE "everything" string */
                p1 = pEV+count;
                CMcpychar(nt,p1);       /* make sure "everything" string terminated*/
                MEcopy(" WHERE \0",8,p);  /* WHERE             */
                STcat(p,pOJ);            /* WHERE ojclause    */
                STcat(p,pEV);            /* WHERE ojclause ...*/
            }  /* end of WHERE not present */
            MEfree(pEV);
            MEfree(pOJ);
            return TRUE;
            break;
        } /* endif isServerClassOPINGDT*/
        else return FALSE;     /* OJ not supported for this server class */
            break;
    }
        
    case TOK_ESC_FUNCTION:
    {
        BOOL curTime = FALSE;
    
        /* if called by PutParm then cannot use scalar functions */
        if (!pNextScan)
            return FALSE;
    
        /* locate the first non-blank char after the token 'fn' */
        while (CMwhite(p) ) CMnext(p);
        
        /* max number of bytes we can safely go back(reuse) is 4.
        ** "{fn LCASE ..."
        */
        pStart = p - 4; 
        
        /* functions which need special processing */
    
        if (!STbcompare(p,7,"CONVERT",7,TRUE))
        {
            char *p1;
            char *p2;
            char *p3;
            char fmt[80] = "%s%s";
            char tmp[80];
            char tmp1[80];
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;

            MEfill (sizeof(tmp1),'\0', tmp1);
            MEfill (sizeof(tmp), '\0', tmp);

            for (i = 0; i < sqlToScalarTblMax; i++)
            {
                len = STlength(sqlToScalarTbl[i].sqlType);
                p1 = STstrindex(p, sqlToScalarTbl[i].sqlType, 0, FALSE);
                if (p1 && !STbcompare (p1, len, sqlToScalarTbl[i].sqlType, 
                    len, TRUE ))
                {
                    p1 = STindex(p, "(", 0);
                    p2 = STindex(p, ",", 0);
                    *p2 = ')';
                    p3 = STindex(p, "}", 0);
                    MEcopy(p1, (p2 - p1)+1, tmp);
                    STprintf(tmp1, fmt, sqlToScalarTbl[i].scalar, tmp);
                    MEcopy(tmp1, STlength(tmp1), p);
                    p1 = p + STlength(tmp1);
                    MEfill((p3 - p1)+1, ' ', p1);
                    p += STlength (p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0';    
                    return TRUE;
                }
            }
            if (i == sqlToScalarTblMax)
            {
                 InsertSqlcaMsg(&pstmt->sqlca,
                     "Invalid data type for CONVERT function scalar",
                     "HY000",TRUE);
                 return FALSE;
            }
            break;
        }
        if (!STbcompare(p,5,"LCASE",5,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            MEcopy("lowercase",9,pStart);
            if (pNextScan)
            {
                *pNextScan = p + 5;
                szEscape += STlength (szEscape);
                MEfill (cbEscape,' ',szEscape - cbEscape);
                p += STlength (p);
                /*
                ** Null terminate the string instead of restoring the
                ** saved character, since the revised query overwrites
                ** the original query string.
                */
                CMcpychar(bNull,p); 
                bNull[0] = '\0';    
                return TRUE;
            }
            break;
        }
        if (!STbcompare(p,5,"UCASE",5,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            MEcopy("uppercase",9,pStart);
            if (pNextScan)
            {
                *pNextScan = p + 5;
                szEscape += STlength (szEscape);
                MEfill (cbEscape, ' ',szEscape - cbEscape);
                p += STlength (p);
                /*
                ** Null terminate the string instead of restoring the
                ** saved character, since the revised query overwrites
                ** the original query string.
                */
                CMcpychar(bNull,p); 
                bNull[0] = '\0';    
                return TRUE;
            }
            break;
        }
        if (!STbcompare(p,5,"RTRIM",5,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            
            q = p;
            CMcpychar(" ",q);
            
            if (pNextScan)
            {
                *pNextScan = p + 5;
                szEscape += STlength (szEscape);
                MEfill (cbEscape,' ',szEscape - cbEscape);
                p += STlength (p);
                /*
                ** Null terminate the string instead of restoring the
                ** saved character, since the revised query overwrites
                ** the original query string.
                */
                CMcpychar(bNull,p);  
                bNull[0] = '\0';   
                return TRUE;
            }
            break;
        }
        if (!STbcompare(p,6,"LOCATE",6,TRUE))
        {
            break; /* NOT supported  */
        }
        if (!STbcompare(p,7,"CURDATE",7,TRUE))
        {
            p += 7;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"("))
            {
                CMcpychar(" ",p); 
                CMnext(p);
                while (CMwhite(p) ) CMnext(p);
                if (!CMcmpcase(p,")") )
                {
                    CMcpychar(" ",p); 
                    if (fAPILevel < IIAPI_LEVEL_4) 
                       MEcopy("date('today')",13,pStart);
                    else 
                       MEcopy("current_date",12,pStart);
                }
            }
            break; 
        }
        if (!STbcompare(p,12,"CURRENT_DATE",12, TRUE))
        {
            p += 12;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"("))
            {
                CMcpychar(" ",p); 
                CMnext(p);
                while (CMwhite(p) ) CMnext(p);
                if (!CMcmpcase(p,")") )
                {
                    CMcpychar(" ",p); 
                    if (fAPILevel < IIAPI_LEVEL_4) 
                       MEcopy("date('today')   ",16,pStart);
                    else 
                       MEcopy("current_date    ",16,pStart);
                }
            }
            break; 
        }
        if (!STbcompare(p,17,"CURRENT_TIMESTAMP",17,TRUE))
        {
            p += 17;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"(") )
            {
                CMcpychar(" ",p); 
                CMnext(p);
                while (CMwhite(p)) CMnext(p);
                if (!CMcmpcase(p,")") )
                {
                    p += STlength (p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0'; 
                    count = (i4)STlength (p);
                    if (count == 0)
                        count = 1;    /* to move the null terminator  */
                    /* 
                    ** pstmt->szNull is not updated, the inserted string 
                    ** doesn't contain escape character, it will be 
                    ** skipped anyway 
                    */
                    if (fAPILevel < IIAPI_LEVEL_4)
                    {
                        memmove(pStart+34,p,count);
                        MEcopy("date ( 'now' )                    ",
                            34,pStart);
                    }
                    else
                    {
                        memmove(pStart+34,p,count);
                        MEcopy("timestamp_wo_tz(current_timestamp)",
                            34,pStart);
                    }     

                    if (pNextScan)
                        *pNextScan = pStart + 34;

                    return TRUE;
                }
            }
            break; 
        }
        if (!STbcompare(p,7,"CURTIME",7,TRUE))
        {
            curTime = TRUE;
            p += 7;
        }
        else if (!STbcompare(p,12,"CURRENT_TIME",12,TRUE))
        {
            curTime = TRUE;
            p += 12;
        }
        if (curTime)
        {
            curTime = FALSE;
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"("))
            {
                CMcpychar(" ",p); 
                CMnext(p); 
                while (CMwhite((char *)p)) CMnext(p);
                if (!CMcmpcase(p,")"))
                {
                    p += STlength (p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0';   
                    count = (i4)STlength (p);
                    if (count == 0)
                        count = 1;
                    if (fAPILevel < IIAPI_LEVEL_4) 
                    {
                        memmove(pStart+29,p,count);
                        MEcopy(" right(char(date('now')),14) ",29,pStart);
                        if (pNextScan)
                            *pNextScan = pStart + 29;
                    
                    }
                    else
                    {
                        memmove(pStart+26,p,count);
                        MEcopy(" time_wo_tz(current_time) ",26,
                            pStart);
                        if (pNextScan)
                            *pNextScan = pStart + 26;
                    
                    }
                    /* pstmt->szNull is not updated, the inserted 
                    ** string doesn't contain
                    ** escape character, it will be skipped anyway 
                    */
                    return TRUE;
                }
            }
            break; 
        }
        if (!STbcompare(p,7,"DAYNAME",7,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
	    if (isServerClassVECTORWISE(pstmt->pdbcOwner))
		return FALSE;

            MEcopy("    dow",7,p);
            if (pNextScan)
            {
                /* set the next starting position for ParseEscape. 
                ** And must change the null terminator back to space here.
                */
                *pNextScan = p + 7;
                szEscape += STlength (szEscape);
                MEfill (cbEscape,' ',szEscape - cbEscape);
                p += STlength (p);
                CMcpychar(bNull,p); 
                /*
                ** Null terminate the string instead of restoring the
                ** saved character, since the revised query overwrites
                ** the original query string.
                */
                bNull[0] = '\0';    
                return TRUE;
            }
            break; 
        }
        if (!STbcompare(p,10,"DAYOFMONTH",10,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                p += 10;
                ConvertDateFn(pstmt, p,pStart,cbEscape,"day",bNull);
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                    return TRUE;
            }
            else
            {
                /*
                ** Convert to DAY().
                */
                MEfill (7, ' ', p+3);
                p += 10;
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
       
        }
        if (!STbcompare(p,9,"DAYOFWEEK",9,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            if (fAPILevel < IIAPI_LEVEL_4)
                return FALSE;

            p += 9;
            while (CMwhite(p)) CMnext(p);
            if ( !CMcmpcase(p,"(") )
            {
                p1 = CMnext(p); 
                count = 1;
                while (*p != EOS)
                {
                    if (!CMcmpcase(p,")"))
                        count--;
                    else
                    {
                        if (!CMcmpcase(p,"("))
                            count++;
                    }
                    if (count <= 0)
                        break;
                    CMnext(p);
                }
                if (!CMcmpcase(p,")"))
                {
                    CMcpychar(nt,p); 
                    STprintf(buf, " 1 + mod( 5 + (mod((day(%s) + " \
                        "((26*(coalesce(case when month(%s) <= 2 then " \
                        "month(%s) + 12 else month(%s) end) + 1)) / 10)" \
                        " + coalesce(case when month(%s) <= 2 then " \
                        "mod((year(%s) - 1), 100) else mod(year(%s), " \
                        "100) end) + coalesce(case when month(%s) <= 2 " \
                        "then mod((year(%s) - 1), 100) else mod(year(%s), " \
                        "100) end) / 4 + coalesce(case when month(%s) <= " \
                        "2 then (year(%s) - 1) / 100 else year(%s) / 100 " \
                        "end) / 4 + (5 * coalesce(case when month(%s) <= " \
                        "2 then (year(%s) - 1) / 100 else year(%s) / " \
                        "100 end))), 7)), 7)", p1, p1, p1, p1, p1, p1, p1, 
                            p1, p1, p1, p1, p1, p1, p1, p1, p1);
                    CMnext(p);
                    p += STlength (p);
                    CMcpychar(bNull,p); 
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    bNull[0] = '\0';    
                    count = (i4)STlength (p);
                    if (count == 0)
                        count = 1;
                    bufLen = STlength(buf);
                    memmove(pStart+bufLen,p,count);
                    MEcopy(buf,bufLen,pStart);
                    if (pNextScan)
                        *pNextScan = pStart + bufLen;
                    return TRUE;
                }
            }
            break; 
        }
        if (!STbcompare(p,9,"DAYOFYEAR",9,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;

            p += 9;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"("))
            {
                p1 = CMnext(p);
                count = 1;
                while (*p != EOS)
                {
                    if (!CMcmpcase(p,")"))
                        count--;
                    else
                    {
                        if (!CMcmpcase(p,"("))
                            count++;
                    }
                    if (count <= 0)
                        break;
                    CMnext(p);
                }
                if (!CMcmpcase(p,")"))
                {
                    CMcpychar(nt,p); 
                    if (fAPILevel < IIAPI_LEVEL_4)
                        STprintf(buf," int4(interval('days',%s-date_trunc" \
                            "('yr',%s)))+1",p1,p1);
                    else
                        STprintf(buf, "coalesce (case when (mod(year(%s),400)" \
                            " = 0 ) then 366-integer(left(varchar(ansidate(" \
                            "varchar(year(%s)) + '-12-31') - ansidate(%s)), " \
                            "locate(varchar(ansidate(varchar(year(%s)) + " \
                            "'-12-31') -ansidate(%s)), ' '))) end, " \
                            "case when (mod(year(%s), 100) = 0 ) then " \
                            "365-integer(left(varchar(ansidate(varchar(" \
                            "year(%s)) + '-12-31') - ansidate(%s)), " \
                            "locate(varchar(ansidate(varchar(year(%s)) + " \
                            "'-12-31') - ansidate(%s)), ' '))) end, " \
                            "case when (mod(year(%s), 4) = 0 ) then " \
                            "366-integer(left(varchar(ansidate(varchar(" \
                            "year(%s)) + '-12-31') - ansidate(%s)), " \
                            "locate(varchar(ansidate(varchar(year(%s)) + " \
                            "'-12-31') - ansidate(%s)), ' '))) " \
                            "else 365-integer(left(varchar(ansidate(" \
                            "varchar(year(%s)) + '-12-31') - ansidate(%s)), " \
                            "locate(varchar(ansidate(varchar(year(%s)) + " \
                            "'-12-31') - ansidate(%s)), ' '))) end)",
                            p1, p1, p1, p1, p1, p1, p1, p1, p1, p1, p1, 
                            p1, p1, p1, p1, p1, p1, p1, p1); 
                    CMnext(p);
                    p += STlength(p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0';    
                    count = (i4)STlength(p);
                    if (count == 0)
                        count = 1;
                    bufLen = STlength(buf);
                    memmove(pStart+bufLen,p,count);
                    MEcopy(buf,bufLen,pStart);
                    if (pNextScan)
                        *pNextScan = pStart + bufLen;
                    return TRUE;
                }
            }
            break; 
        }
        if (!STbcompare(p,9,"MONTHNAME",9,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            if (fAPILevel < IIAPI_LEVEL_4)
                return FALSE;
            p += 9;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"("))
            {
                p1 = CMnext(p);
                count = 1;
                while (*p != EOS)
                {
                    if (!CMcmpcase(p,")"))
                        count--;
                    else
                    {
                        if (!CMcmpcase(p,"("))
                            count++;
                    }
                    if (count <= 0)
                        break;
                    CMnext(p);
                }
                if (!CMcmpcase(p,")"))
                {
                    char *monthNameQuery = "coalesce(case when month(%s) = 1 " \
                        "then 'jan' end, case when month(%s) = 2 then " \
                        "'feb' end, case when month(%s) = 3 then 'mar' end, " \
                        "case when month(%s) = 4 then 'apr' end, case when " \
                        "month(%s) = 5 then 'may' end, case when month(%s) " \
                        "= 6 then 'jun' end, case when month(%s) = 7 then " \
                        "'jul' end, case when month(%s) = 8 then 'aug' end, " \
                        "case when month(%s) = 9 then 'sep' end, case when " \
                        "month(%s) = 10 then 'oct' end, case when month(%s) " \
                        "= 11 then 'nov' end, case when month(%s) = 12 then " \
                        "'dec' end)"; 
 
                    CMcpychar(nt,p);
                    STprintf(buf, monthNameQuery, p1, p1, p1, p1, p1, p1,
                        p1, p1, p1, p1, p1, p1);
		    CMnext(p);
                    p += STlength(p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0';   
                    count = (i4)STlength(p);
                    if (count == 0)
                        count = 1;
                    bufLen = STlength(buf);
                    memmove(pStart+bufLen,p,count);
                    MEcopy(buf,bufLen,pStart);
                    if (pNextScan)
                        *pNextScan = pStart + bufLen;
                    return TRUE;
                }
            }
            break;
        }
        if (!STbcompare(p,13,"TIMESTAMPDIFF",13,TRUE))
        {
            char *p2;
            char *diffStr = NULL;
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 13;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"("))
            {
                CMnext(p);
                count = 1;
                while (CMwhite(p)) CMnext(p);
                if (!STbcompare(p, 12,"SQL_TSI_YEAR",12, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4 (interval('years',date(" \
                            "%s) - date (%s)))";
                    else
                        diffStr = "int4(round((%s - %s)/interval " \
                            "'31556952' second, 0))";
                    p += 12;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                else if (!STbcompare(p, 13,"SQL_TSI_MONTH",13, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4(interval('months',date(" \
                            "%s) - date(%s)))";
                    else
                        diffStr = "int4(round(12.0*((%s - %s)/interval " \
                        "'31556952' second), 0))";
                    p += 13;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                else if (!STbcompare(p, 11,"SQL_TSI_DAY",11, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4(interval('days',date(" \
                            "%s) - date(%s)))";
                    else
                        diffStr = "int4(round(365.2425*((%s - %s)/interval " \
                        "'31556952' second), 0))";
                    p += 11;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                else if (!STbcompare(p, 12,"SQL_TSI_HOUR",12, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4(interval('hours',date(" \
                            "%s) - date(%s)))";
                    else
                        diffStr = "int4(round(365.2425*24*((%s - " \
                           "%s) / interval '31556952' second), 0))";
                    p += 12;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                else if (!STbcompare(p, 14,"SQL_TSI_MINUTE",14, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4(interval('minutes',date(" \
                            "%s) - date(%s)))";
                    else
                        diffStr = "int4(round(365.2425*24*60*((%s - " \
                            "%s)/interval '31556952' second), 0))";
                    p += 14;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                else if (!STbcompare(p, 14,"SQL_TSI_SECOND",14, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4(interval('seconds',date(" \
                            "%s) - date(%s)))";
                    else
                        diffStr = "int4(round(365.2425*24*60*60*((%s - " \
                            "%s)/interval '31556952' second), 0))";
                    p += 14;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                else if (!STbcompare(p, 15,"SQL_TSI_QUARTER",15, TRUE))
                {
                    if (fAPILevel < IIAPI_LEVEL_5)
                        diffStr = "int4(4.0*(interval('years',date(" \
                            "%s) - date(%s))))";
                    else
                        diffStr = "int4(round(4.0*((%s - %s)/interval " \
                        "'31556952' second), 0))";
                    p += 15;
                    p1 = STindex(p, ",", 0);
                    CMnext(p1);
                    p2 = STindex(p1, ",",0);
                    *p2 = '\0';
                    CMnext(p2);
                    p = STindex(p2, ")", 0);
                    *p = '\0';
                    CMnext(p);
                }
                
                if (diffStr == NULL)
                    return FALSE;
                
                STprintf(buf, diffStr, p2, p1);
                p += STlength(p);

                /*
                ** Null terminate the string instead of restoring the
                ** saved character, since the revised query overwrites
                ** the original query string.
                */
                CMcpychar(bNull,p); 
                bNull[0] = '\0';   
                count = (i4)STlength(p);
                if (count == 0)
                    count = 1;
                bufLen = STlength(buf);
                memmove(pStart+bufLen,p,count);
                MEcopy(buf,bufLen,pStart);
                if (pNextScan)
                    *pNextScan = pStart + bufLen;
                return TRUE;
            }
            break;
        }

        if (!STbcompare(p,4,"HOUR",4,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 4;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"hrs",bNull);
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {    
                /*
                ** Convert to HOUR() scalar.
                */
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,6,"MINUTE",6,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 6;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"min",bNull);
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                /*
                ** Convert to MINUTE() scalar.
                */
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,5,"MONTH",5,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 5;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"mos",bNull);
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                /*
                ** Convert to MONTH() scalar.
                */
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,3,"NOW",3,TRUE))
        {
            p += 3;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"(") )
            {
                CMcpychar(" ",p); 
                CMnext(p);
                while (CMwhite(p)) CMnext(p);
                if (!CMcmpcase(p,")") )
                {
                    p += STlength (p);
                    CMcpychar(bNull,p); 
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    bNull[0] = '\0';    
                    count = (i4)STlength (p);
                    if (count == 0)
                        count = 1;    /* to move the null terminator  */
                    /* 
                    ** pstmt->szNull is not updated, the inserted string 
                    ** doesn't contain escape character, it will be 
                    ** skipped anyway 
                    */
                    if (fAPILevel < IIAPI_LEVEL_4)
                    {  
                        memmove(pStart+11,p,count);
                        MEcopy("date('now')",11,pStart);
                        if (pNextScan)
                            *pNextScan = pStart + 11;
                    }
                    else
                    {
                        memmove(pStart+34,p,count);
                        MEcopy("timestamp_wo_tz(current_timestamp)",
                            34,pStart);
                        if (pNextScan)
                            *pNextScan = pStart + 34;
                    }

                    return TRUE;
                }
            }
            break; 
        }
        if (!STbcompare(p,7,"QUARTER",7,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 7;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"qtr",bNull);
                *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,6,"SECOND",6,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 6;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"sec",bNull);
                *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                /*
                ** Convert to MINUTE() scalar.
                */
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,4,"WEEK",4,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 4;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"wks",bNull);
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                /*
                ** Convert to WEEK() scalar.
                */
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,7,"EXTRACT",7,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                /*
                ** Re-constitute EXTRACT (XXX FROM yyy) into
                ** XXX(yyy) before calling ConvertDateFn().
                */
                MEfill (7, ' ', p);
                p += 7;
                while (CMwhite(p)) CMnext(p);
                CMcpychar(" ", p);
                if (p1 = STstrindex(p, "YEAR", 0, TRUE))
                {
                    p1 = STstrindex(p1, "FROM", 0, TRUE);
                    MEfill (4, ' ', p1);
                    CMcpychar("(",p1+3);
                    while (CMwhite(p)) CMnext(p);
                    p += 4;
                    ConvertDateFn(pstmt,p,pStart,cbEscape,"yrs",bNull);
                }
	        else if (p1 = STstrindex(p, "MONTH", 0, TRUE))
                {
                    p1 = STstrindex(p1, "FROM", 0, TRUE);
                    MEfill (4, ' ', p1);
                    CMcpychar("(",p1+3);
                    while (CMwhite(p)) CMnext(p);
                    p += 5;
                    ConvertDateFn(pstmt,p,pStart,cbEscape,"mos",bNull);
                }
                else if (p1 = STstrindex(p, "DAY", 0, TRUE))
                {         
                    p1 = STstrindex(p1, "FROM", 0, TRUE);
                    MEfill (4, ' ', p1);
                    CMcpychar("(",p1+3);
                    while (CMwhite(p)) CMnext(p);
                    p += 3;
                    ConvertDateFn(pstmt,p,pStart,cbEscape,"day",bNull);
                }
                else if (p1 = STstrindex(p, "HOUR", 0, TRUE))
                {                   
                    p1 = STstrindex(p1, "FROM", 0, TRUE);
                    MEfill (4, ' ', p1);
                    CMcpychar("(",p1+3);
                    while (CMwhite(p)) CMnext(p);
                    p += 4;
                    ConvertDateFn(pstmt,p,pStart,cbEscape,"hrs",bNull);
                }
                else if (p1 = STstrindex(p, "MINUTE", 0, TRUE))
                {                   
                    p1 = STstrindex(p1, "FROM", 0, TRUE);
                    MEfill (4, ' ', p1);
                    CMcpychar("(",p1+3);
                    while (CMwhite(p)) CMnext(p);
                    p += 6;
                    ConvertDateFn(pstmt,p,pStart,cbEscape,"min",bNull);
                }
                else if (p1 = STstrindex(p, "SECOND", 0, TRUE))
                {                   
                    p1 = STstrindex(p1, "FROM", 0, TRUE);
                    MEfill (4, ' ', p1);
                    CMcpychar("(",p1+3);
                    while (CMwhite(p)) CMnext(p);
                    p += 6;
                    ConvertDateFn(pstmt,p,pStart,cbEscape,"sec",bNull);
                }
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                /*
                ** Convert to EXTRACT() scalar.
                */
		p += 7;
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,4,"YEAR",4,TRUE))
        {
            if (!isServerClassINGRES(pstmt->pdbcOwner))
                return FALSE;
            p += 4;
            if (fAPILevel < IIAPI_LEVEL_4)
            {
                ConvertDateFn(pstmt,p,pStart,cbEscape,"yrs",bNull);
                *pNextScan = pStart + DATE_PART_LEN;
                return TRUE;
            }
            else
            {
                /*
                ** Convert to YEAR() scalar.
                */
                if (pNextScan)
                    *pNextScan = pStart + DATE_PART_LEN;
                break;
            }
        }
        if (!STbcompare(p,4,"USER",4,TRUE))
        {
            p += 4;
            while (CMwhite(p)) CMnext(p);
            if (!CMcmpcase(p,"(") )
            {
                CMcpychar(" ",p); 
                CMnext(p);
                while (CMwhite((char *)p)) CMnext(p);
                if (!CMcmpcase(p,")") )
                {
                    p += STlength (p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0';    
                    count = (i4)STlength(p);
                    if (count == 0)
                        count = 1;    /* to move the null terminator  */
                    memmove(pStart+20,p,count);
                    MEcopy("dbmsinfo('username')",20,pStart);
                    if (pNextScan)
                        *pNextScan = pStart + 20;
                    return TRUE;
                }
            }
            break;
        }
        if (!STbcompare(p,8,"DATABASE",8,TRUE))
        {
            p += 8;
            while (CMwhite((char *)p)) CMnext(p);
            if (!CMcmpcase(p,"(") )
            {
                CMcpychar(" ",p); 
                CMnext(p);
                while (CMwhite(p)) CMnext(p);
                if (!CMcmpcase(p,")") )
                {
                    p += STlength(p);
                    /*
                    ** Null terminate the string instead of restoring the
                    ** saved character, since the revised query overwrites
                    ** the original query string.
                    */
                    CMcpychar(bNull,p); 
                    bNull[0] = '\0';    
                    count = (i4)STlength(p);
                    if (count == 0)
                        count = 1;    /* to move the null terminator  */
                    /*MEmove(count,p,' ',count,pStart+20);  */
                    memmove(pStart+20,p,count);
                    MEcopy("dbmsinfo('database')",20,pStart);
                    if (pNextScan)
                        *pNextScan = pStart + 20;
                    return TRUE;
                }
            }
            break;
        }
        break;
    }
    case TOK_ESC_ESCAPE:
    
        MEcopy ("ESCAPE", 6, szEscape);
        break;
    
    case TOK_ESC_CALL:
    
    break;
    
    case TOK_ESC_TILDEV:
        
        /* 
        ** Scan for the 1st non blank char after '='. if it is 'call' 
        ** then OK. else error.
        */
        while (CMcmpcase(p,"=")) CMnext(p);
        CMcpychar(" ",p); 
        CMnext(p);
        while (CMwhite(p)) CMnext(p);
        
        szEscape = p;
    
        for (cb = 0; *p != EOS ; CMnext(p), cb++)
        {
             if (!CMcmpcase(p,"(") || CMwhite(p) )
                 break;
        }
    
        CMcpychar(p,b); 
        CMcpychar(nt,p); 
        token = ParseToken (szEscape);
        CMcpychar(b,p); 
    
        if (token == TOK_ESC_CALL)
        {
            MEfill (cb, ' ', szEscape);
            break;
        }
        return FALSE; 
    
    case TOK_ESC_INTERVAL:
    {
        char *p1;
    
        if (!isServerClassINGRES(pstmt->pdbcOwner))
            return FALSE;
        /*
        ** Intervals not currently supported for earlier versions of Ingres.
        */
        if (pstmt->fAPILevel < IIAPI_LEVEL_4)
            return FALSE;
    
      
        {
            p1 = STindex(p,"'", 0);
            CMnext(p1);
            p1 = STindex(p1, "'", 0);
            CMnext(p1);
            if (isaParameter)
            {
                CMcpychar(nt, p1);	
                return TRUE;
            }
            else 
            {
                p1 = STstrindex(p1, "YEAR", 0, TRUE);
                if (p1)
                {
                    MEcopy("    ", 4, p1);
                    p1 = STstrindex(p1, "TO", 0, TRUE);
                    if (p1)
                    {
                        MEcopy("  ", 2, p1);
                        p1 = STstrindex(p1, "MONTH", 0, TRUE);
                        if (p1)
                            MEcopy("     ", 5, p1);
                        else
                            return FALSE;
                    }
                }
                else
                {
                    p1 = STstrindex(p1, "DAY", 0, TRUE);
                    if (p1)
                    {
                        MEcopy("   ", 3, p1);
                        p1 = STstrindex(p1, "TO", 0, TRUE);
                        if (p1)
                        {
                            MEcopy("  ", 2, p1);
                            p1 = STstrindex(p1, "SECOND", 0, TRUE);
                            if (p1)
                                MEcopy("      ", 6, p1);
                            else
                                return FALSE;
                        }
                    }
                }
            } /* ! isaParameter */
            p1 = STindex (p1, "}", 0 );
            CMcpychar(" ", p1);
            return TRUE;
        }
    }
    default:
        return FALSE;
    }

    /*
    **  Set ODBC escape terminator to spaces:
    */
    szEscape += STlength(szEscape);              /* szEscape->end of string */
    MEfill(cbEscape, ' ', szEscape - cbEscape);  /* blank out "}" or "*)--" */
    return TRUE;
}


/*
**  ParseEscape
**
**  Scan SQL syntax string for ODBC escape sequences.
**
**  This works like strtok.
**
**  On entry: szSqlStr-->SQL syntax string (first time)
**                     = NULL (to continue)
**
**  On exit:  szSqlStr has a null character inserted to end the
**            escape sequence.  It will be restored to the original
**            character on the next call.
**
**  Returns:  -->ODBC escape sequence, null terminated, if found
**            NULL if none found.
*/
LPSTR ParseEscape(LPSTMT pstmt, LPSTR szSqlStr)
{
    CHAR     *  p;

    TRACE_INTL(pstmt,"ParseEscape");
    /*
    **  Find next part of string to scan, if any, and
    **  restore original character replaced by null terminator.
    **  These are stored between calls in STMT block.
    */
    if (szSqlStr)                   /* if new scan */
    {
        p = szSqlStr;
        pstmt->szNull  = NULL;          /* init ->inserted null terminator */
        pstmt->bNull[0]= '\0';          /* init original character value. */
        pstmt->bNull[1]= '\0';
    }
    else                            /* else resuming old scan */
    {
        p = pstmt->szNull;             /* p->where null term was inserted */
        if (pstmt->bNull[0] != '\0')   /* if none to restore, return NULL */
        {
           CMcpychar(pstmt->bNull, p);    /* return the original character */
        }
        CMnext(p);                     /* position to next char after escape */
    }

    /*
    **  Scan syntax for start of ODBC escape sequence,
    **  ignoring quoted literals and identifiers:
    */
    p = ParseEscapeStart(p);        /* p->'{' or '\0' */

    /*
    **  Find end of ODBC escape sequence
    **  save following character,
    **  and terminate with a null character.
    */
    pstmt->szNull = ParseEscapeEnd(p);   /* save -> to following char */
    if (pstmt->szNull)
    {
        CMcpychar(pstmt->szNull, pstmt->bNull); /* save following character */
        *pstmt->szNull = '\0';           /* null term the escape sequence */
    }

    return (pstmt->szNull) ? p : NULL;
}

/*
**  ParseEscapeStart
**
**  Scan SQL syntax string for start of an ODBC escape sequence.
**
**  On entry: szSqlStr-->where to start scanning in SQL syntax string
**
**  Returns:  -->start of ODBC escape sequence, if found,
**            -->null terminator if none found.
*/
static LPSTR ParseEscapeStart (LPSTR szSqlStr)
{
    CHAR *p      = szSqlStr;
    CHAR *p1;
    CHAR  bSave[2];
    CHAR  rgbTemp[LEN_TEMP]; /* work are for squeezed escape sequence string */

    /*
    **  Scan syntax for start of ODBC escape sequence,
    **  ignoring quoted literals and identifiers:
    */
    while (*p != EOS)
    {
        while (!CMcmpcase(p,"'") || !CMcmpcase(p,"\"") )
        {
            p = ScanQuotedString(p);   /* skip over quoted strings */
        }

        if (!CMcmpcase(p,"{")) break;   /* if left brace, we're done! */

        /*
        **  Look for MickeySoft "standard" escape sequence
        **  "--(*VENDOR(Microsoft),PRODUCT(ODBC)";
        **  (first compress any spaces out of it):
        */
        if (STbcompare (p,(i4)STlength(p), ODBC_STD_ESC, 4, TRUE) == 0)  /* "--(*" */
        {          /* if "--(*", peek ahead to see if escape sequence */
            p1 = p;      /* p1 -> "--(*VENDOR(Microsoft),PRODUCT(ODBC)" */
            CMnext(p1);
            CMnext(p1);
            CMnext(p1);
            CMnext(p1);  /* p1 ->     "VENDOR(Microsoft),PRODUCT(ODBC)" */
            if ((p1 = STindex (p1, ")", 0)) != NULL)
            {            /* p1 ->                     "),PRODUCT(ODBC)" */
                CMnext(p1);
                if ((p1 = STindex (p1, ")", 0)) != NULL)
                {          /* p1 ->                                 ")" */
                    CMnext(p1);
                    CMcpychar(p1,bSave);   /* save char after escape sequence */
                    CMcpychar(nt,p1);    /* null term the escape sequence */
                    STzapblank(p,rgbTemp); /* delete white space, put in rgbTemp */
                    CMcpychar(bSave,p1);   /* restore char after escape sequence */
                    if (STbcompare (rgbTemp, (i4)STlength(rgbTemp),
                                    ODBC_STD_ESC,sizeof(ODBC_STD_ESC),TRUE) == 0)
                        break;   /* if match on long escape sequence, we're done! */
                }
            }
        }
        CMnext(p);  /* no match on escape character, move on to next char */
    }  /* end while loop */

    return(p);
}


/*
**  ParseEscapeEnd
**
**  Scan SQL syntax string for end ODBC escape sequence.
**
**  On entry: szSqlStr-->start of escape sequence
**
**  Returns:  -->first character after end of ODBC escape sequence,
**               this may be the null terminator 
**            NULL if none found (indicates an invalid escape sequence).
*/
static LPSTR ParseEscapeEnd (LPSTR szSqlStr)
{
    CHAR  * p;
    CHAR  * sz;

    if (*szSqlStr == '\0')   /* if already at end of string, just return */
        return NULL;

    /*
    ** skip escape sequence beginning
    */
    if (!CMcmpcase(szSqlStr,"{"))   /* "{" escape form */
    {
        p = szSqlStr;
        CMnext(p);
    }
    else                    /* "--(*VENDOR(Microsoft),PRODUCT(ODBC)" form*/
    {
        p = STindex(szSqlStr + 4, ")", 0);
        if (p) p = STindex(p + 1, ")", 0);
        if (!p) return NULL;   /* invalid escape sequence beginning */
    }
    /* p->character after the start of the escape sequence */

    while (*p)
    {
        /*
        ** find start of next escape sequence, if any,
        ** in case it is nested in this one
        */
        sz = ParseEscapeStart(p);
    
        /*
        ** Scan syntax up to the start of the next escape sequence
        ** (if any, otherwise to the end) for the end of this one,
        ** ignoring quoted literals and identifiers:
        */
        for (; p < sz; p++)
        {
            while (!CMcmpcase(p,"'") || !CMcmpcase(p,"\"") )
            {
                p = ScanQuotedString(p);   /* skip over quoted strings */
            }

            if (!CMcmpcase(szSqlStr,"{"))  /* if started with "{", look for "}" */
            {
                if (!CMcmpcase(p,"}"))
                {
                    CMnext(p);
                    return p;       /* return ptr to char after escape end */
                }
            }
            else                /* else started with "--(*", look for "*)--" */
            {
                if (STbcompare (p, 4, "*)--", 4, FALSE) == 0)
                {
                    CMnext(p);
                    CMnext(p);
                    CMnext(p);
                    CMnext(p);
                    return p;       /* return ptr to char after escape end */
                }
            }
        }  /* end for loop */
        /*
        ** if we didn't find the end yet and there is another,
        ** find the end of the nested one and try again from there.
        */
        if (*sz) p = ParseEscapeEnd(sz);
    }

    /*
    ** No terminator found, indicate an error
    */
    return NULL;
}


/*
**  ParseSpace
**
**  Replace all "white space" characters (TAB, LF, FF, CR, etc)
**  in SQL syntax string with "real" spaces (' ') except inside
**  quoted literals and identifiers.  
**
**  Optionally replace the contents of quoted literals and identifiers
**  with spaces and convert all other characters to upper case.
**  to simplify scanning for other syntax elements.
**
**  On entry: szSqlStr-->SQL syntax string
**            fOption  = FALSE to avoid quoted literals and identifiers
**                     = TRUE to simplify scanning (unilateral)
**                     = -1 to avoid <LF>, <TAB>, and <CR>
**
**  On exit:  all spaces converted.
**
**  Returns:  nothing.
*/
static VOID  ParseSpace(
    LPSTR   szSqlStr,
    SWORD    fOption)
{
    BOOL    fQuote = FALSE;
    char    bQuote;

    while (*szSqlStr != EOS)
    {
        if (fQuote)
        {
            if (!CMcmpcase(szSqlStr,&bQuote))
            {
                fQuote = FALSE;
                bQuote = 0;
            }
            else
            {
                if (fOption > 0)
                    CMcpychar(" ",szSqlStr); 
            }
        }
        else
        {
            if (!CMcmpcase(szSqlStr,"'") || !CMcmpcase(szSqlStr,"\"") )
            {
                fQuote = TRUE;
                CMcpychar(szSqlStr,&bQuote);
            }
            else 
            {
                if (fOption >= 0)
                {
                   if (CMwhite(szSqlStr))
                       CMcpychar(" ",szSqlStr); 
                   else 
                       if (fOption)
                           CMtoupper(szSqlStr,szSqlStr);
                }
                else
                {
                    if (CMwhite(szSqlStr) && (CMcmpcase(szSqlStr,"\r"))
                         && (CMcmpcase(szSqlStr,"\n")) &&
                         (CMcmpcase(szSqlStr,"\t")))
                        CMcpychar(" ",szSqlStr); 
                }
            }
        }
        CMnext(szSqlStr);
    }
    return;
}


/*
**  ParseToken
**
**  Return id for a syntax token.
**
**  On entry: szToken-->SQL syntax token
**
**  Returns:  token id
*/
static TOKEN  ParseToken(
    LPSTR  szToken)
{
    TOKEN   token = TOK_VOID;
    char    *p = szToken;
    BOOL fComment = FALSE;

    token = FetchToken(szToken);
    
    /*
    ** If the token is not the beginning of a comment, we're done.
    */
    if (token != TOK_BEGIN_COMMENT)
        goto end;
    else
        fComment = TRUE;

    while (p)
    {
        p = strtok(NULL, " ");
        if (!p)
        {
            token = TOK_VOID;
            goto end;
        }

        /* 
        ** Swallow multiple "/*" markers.
        */
        token = FetchToken(p);
        if (token == TOK_BEGIN_COMMENT)
        {
            fComment = TRUE;
                continue;
        }

        if (fComment)
        {
            if (token == TOK_END_COMMENT)
                fComment = FALSE;            

            /*
            ** Keep looping through the query string until the end comment
            ** marker is found.
            */
            continue;
        }
        goto end;
    }

end:
    /*
    ** If no markers are found other than a comment marker, treat as
    ** invalid syntax.
    */
    if (token == TOK_BEGIN_COMMENT || 
        token == TOK_END_COMMENT)
        token = TOK_VOID;

    return token;
}

/*
**  FetchToken
**
**  Return id for a syntax token.
**
**  On entry: szToken-->SQL syntax token
**            fUpper  = TRUE if not case sensitive
**
**  Returns:  token id
*/
static TOKEN  FetchToken(
    LPSTR  szToken)
{
    SQLINTEGER  i;
    char    szTokenUpper[16];   /* work area for token in upper case */

    const static struct
    {
        TOKEN   token;
        LPSTR   sz;
    }
    TOKEN_TABLE[] =
    {
    TOK_AUTOCOMMIT,    "AUTOCOMMIT",
    TOK_ALL,           "ALL",
    TOK_ALTER,         "ALTER",
    TOK_BEGIN_COMMENT, "/*",
    TOK_COMMIT,        "COMMIT",
    TOK_COPY,          "COPY",
    TOK_CREATE,        "CREATE",
    TOK_DELETE,        "DELETE",
    TOK_DROP,          "DROP",
    TOK_END_COMMENT,   "*/",
    TOK_ESC_DATE,      "D",
    TOK_ESC_ESCAPE,    "ESCAPE",
    TOK_ESC_FUNCTION,  "FN",
    TOK_ESC_INTERVAL,  "INTERVAL",
    TOK_ESC_OJ,        "OJ",
    TOK_ESC_TIME,      "T",
    TOK_ESC_TIMESTAMP, "TS",
    TOK_ESC_BRACE,     "{",
    TOK_ESC_CALL,      "CALL",
    TOK_ESC_BRACECALL, "{CALL",
    TOK_ESC_TILDEV,    "~V",
    TOK_GRANT,         "GRANT",
    TOK_IDMS,          "IDMS",
    TOK_INDEX,         "INDEX",
    TOK_INSERT,        "INSERT",
    TOK_MODIFY,        "MODIFY",
    TOK_ODBC,          "ODBC",
    TOK_OFF,           "OFF",
    TOK_ON,            "ON",
    TOK_READ,          "READ",
    TOK_ROLLBACK,      "ROLLBACK",
    TOK_SELECT,        "SELECT",
    TOK_SESSION,       "SESSION",
    TOK_SET,           "SET",
    TOK_SUSPEND,       "SUSPEND",
    TOK_TRACE,         "TRACE",
    TOK_TRANSACTION,   "TRANSACTION",
    TOK_UPDATE,        "UPDATE",
    TOK_VIEW,          "VIEW",
    TOK_WRITE,         "WRITE",
    TOK_IDENTIFIER,    ""
    };

    if (!szToken || *szToken == EOS)
        return TOK_VOID;

    i = STlength(szToken);
    if (i > sizeof(szTokenUpper)-1)
        return TOK_VOID;
    STcopy(szToken, szTokenUpper);   /* copy token to upper case work area */
    CVupper(szTokenUpper);           /* convert token to upper case */

    for (i = 0; TOKEN_TABLE[i].token != TOK_IDENTIFIER; i++)
        if (STcompare(TOKEN_TABLE[i].sz, szTokenUpper) == 0)
            break;

    return TOKEN_TABLE[i].token;
}

/*
**  ParseTrace
**
**  Parse trace command and set options.
**
**  On entry: pstmt-->STMT block
**            strtok has been called on syntax string.
**
**  On exit:  Trace options set as specified.
**            Temporary SQL string is freed.
**
**  Returns:  SQL_SUCCESS_WITH INFO
**            SQL_ERROR
*/
RETCODE  ParseTrace(
    LPSTMT      pstmt)
{
    CHAR      * p;
    TOKEN   token;
    UINT    flag = 0;

    TRACE_INTL (pstmt, "ParseTrace");
    /*
    **  Parse TRACE command:
    */
    p = strtok (NULL, " \n\r\t" );
    token = ParseToken (p);
    switch (token)
    {
    case TOK_ODBC:
    case TOK_IDMS:
        /*
        **  Parse trace options:
        */
        p = strtok (NULL, " \n\r\t" );
        switch (ParseToken (p))
        {
        case TOK_OFF:
            flag = 0;
            break;
    
        case TOK_ON:
            flag = (token == TOK_ODBC) ? TRACE_ODBCCALL + SNAP_SQL : 1;
            break;
        
        case TOK_ALL:
            flag = 0xFFFF;
            break;
        
        default:
            flag = (UINT)strtoul (p, NULL, 0);
            break;
        }
    break;
    
    default:

        return ErrState (SQL_HY000, pstmt, F_OD0069_IDS_ERR_TRACE);
    } /* Switch (token) */

    /*
    **  Mark statement as unprepared, as if nothing happened:
    */
    FreeSqlStr (pstmt);
    return ErrState (SQL_01000, pstmt, F_OD0018_IDS_MSG_TRACE);
}


/*
**  PrepareConstant
**
**  Pretend to prepare a query that returns a constant result set:
**
**  1. Load default SQLDA from user defined resource.
**  2. Set statement flags: constant query, already "parsed".
**  3. Call PrepareSqlda to setup up SQLDA and COL arrays.
**
**
** On entry:  pstmt-->statement block
**
** On exit:  statement is prepared if successful.
**
** Returns:  SQL_SUCCESS
**           SQL_SUCCESS_WITH_INFO
**           SQL_ERROR
*/
RETCODE PrepareConstant(
    LPSTMT  pstmt)
{

    TRACE_INTL (pstmt, "PrepareConstant");

    pstmt->fStatus  = STMT_CONST | STMT_DIRECT;
    
    pstmt->fCommand = CMD_SELECT;

    return PrepareSqlda (pstmt);
}


/*
**  PrepareParms
**
**  Set up parameter SQLDA and buffer.
**
** On entry:  pstmt-->statement block
**
** Called by::
**           ExecuteStmt
**           PrepareStmt
**
** Returns:  SQL_SUCCESS
**           SQL_ERROR
*/
RETCODE PrepareParms (
    LPSTMT  pstmt)
{
    LPDESC    pAPD = pstmt->pAPD;   /* Application Parameter Descriptor */
    LPDESC    pIPD = pstmt->pIPD;   /* Implementation Parameter Descriptor */
    LPDESCFLD papd;                 /* APD parm field */
    LPDESCFLD pipd;                 /* IPD parm field */
    /*UDWORD  cbMax; */
    UWORD   i;
    RETCODE rc;
    UDWORD      cbParm;             /* Length of parm */
    SQLINTEGER     *pcbValue = NULL;
    BOOL      atExec = FALSE;

    TRACE_INTL (pstmt, "PrepareParms");

    FreeStmtBuffers (pstmt);

    if (pstmt->icolParmLast == 0)     /* if no parameter markers ("~V") */
       return SQL_SUCCESS;            /* then just return OK */

    if (pAPD->Count > pIPD->CountAllocated) /* allocated more columns if needed */
        ReallocDescriptor(pIPD, pAPD->Count);

    if (pAPD->Count > pIPD->Count)    /* bring IPD's count up to APD's count */
        pIPD->Count = pAPD->Count;


    if (pIPD->Count > pAPD->CountAllocated) /* allocated more columns if needed */
        ReallocDescriptor(pAPD, pIPD->Count);

    if (pIPD->Count > pAPD->Count)    /* bring APD's count up to IPD's count */
        pAPD->Count = pIPD->Count;

    if (pstmt->icolParmLast > GetDescBoundCount(pAPD))  /* if more markers than parms*/
       return ErrState (SQL_07001, pstmt);  /* "Wrong number of parameters" */



    /*
    **  Convert the COL array to an SQLVAR array in the SQLDA,
    **  and compute the offset of each parm in buffer, which
    **  also provides the size of the parm buffer:
    */
    cbParm = 0;

    papd = pAPD->pcol + 1;  /* papd->1st APD parameter field after bookmark */
    pipd = pIPD->pcol + 1;  /* papd->1st APD parameter field after bookmark */

    for (i = 0;
         i < pstmt->icolParmLast;  /* i < count of parameter markers */
         i++, papd++, pipd++)
    {
	rc = SetColVar (pstmt, papd, pipd);
	if (rc != SQL_SUCCESS)
	    return rc;

	cbParm = SetColOffset (0, pipd); 

	if (pipd->DataPtr  && (pipd->fStatus & COL_DATAPTR_ALLOCED))
	{
            MEfree(pipd->DataPtr);  /* free prior parm buffer */
	    pipd->DataPtr  = NULL;
	    pipd->fStatus &= ~COL_DATAPTR_ALLOCED;
	}
        GetBoundDataPtrs(pstmt, Type_APD, pstmt->pAPD, papd,
            NULL,
            &pcbValue,
            NULL);
        /*
        **  Don't allocate dynamic pointers at this point.  Leave
        **  this for SQLPutData().
        */
        if (pcbValue)
        {
	    if (*pcbValue == SQL_DATA_AT_EXEC || *pcbValue
                <= SQL_LEN_DATA_AT_EXEC_OFFSET)
                atExec = TRUE;
            else
                atExec = FALSE;
        }
        else
            atExec = FALSE;

        if (!atExec)
        {
            pipd->DataPtr = MEreqmem(0, cbParm, TRUE, NULL);
            pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr is allocated */
            if (pipd->DataPtr == NULL)
                return ErrState (SQL_HY001, pstmt, 
                    F_OD0008_IDS_PARM_BUFFER);
        }          
        else
        {
            pipd->fStatus &= ~COL_DATAPTR_ALLOCED;
            pipd->DataPtr = NULL;
        } 
    }  /* end for loop through parameters */

    return SQL_SUCCESS;
}


/*
**  PrepareSqlda
**
**  Set up fetch buffer SQLDA and buffer offsets.
**
** On entry:  pstmt-->statement block
**
** Returns:  SQL_SUCCESS
**           SQL_ERROR
*/
RETCODE PrepareSqlda(
    LPSTMT  pstmt)
{
    LPDBC     pdbc = pstmt->pdbcOwner;
    LPDESC    pIRD = pstmt->pIRD;   /* Implementation Row Descriptor */
    LPDESC    pARD = pstmt->pARD;   /* Application Row Descriptor */
    i2        IRDCount = pIRD->Count;
    LPDESCFLD pird;                 /* IRD row field */
    int      i;

    TRACE_INTL (pstmt, "PrepareSqlda");


    /*
    **  Ensure that ARD row descriptor is large enough:
    */
/*  should not be needed
    if (IRDCount > pARD->CountAllocated)   * if need more col, re-allocate * 
        if (ReallocDescriptor(pARD, pIRD->Count) == NULL)
            return ErrUnlockStmt (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
*/

    /*
    **  If preparing a catalog function, the SQLDA needs to
    **  be adjusted to make if conform to the way ODBC defines
    **  the result set:
    */
    if (pstmt->fCatalog && !(pstmt->fStatus & STMT_CONST))
        CatPrepareSqlda (pstmt);

    /*
    **  Compute offset of value and null indicator for each column
    **  and total length of one row:
    */
    pstmt->cbrow = 0;


    for (i = 0,
         pird = pIRD->pcol + 1;          /* pird -> 1st IRD (after bookmark) */
         i < IRDCount;
         i++, pird++)
    {

        switch (pird->fIngApiType)
        {
        /*
        **  Make back end truncate data if stmt attr SQL_ATTR_MAX_LENGTH was set:
        */
        case IIAPI_CHA_TYPE:
        case IIAPI_CHR_TYPE:
        case IIAPI_VCH_TYPE:
        case IIAPI_TXT_TYPE:
        case IIAPI_LVCH_TYPE:
        case IIAPI_LTXT_TYPE:
        case IIAPI_BYTE_TYPE:
        case IIAPI_VBYTE_TYPE:
        case IIAPI_LBYTE_TYPE:
# ifdef IIAPI_NCHA_TYPE
        case IIAPI_NCHA_TYPE:
        case IIAPI_NVCH_TYPE:
        case IIAPI_LNVCH_TYPE:
#endif
            if (pstmt->cbValueMax && pird->OctetLength > pstmt->cbValueMax)
                pird->OctetLength = pstmt->cbValueMax;
            break;
        /*
        **  Force all floats, and reals if they want, to double:
        **  This avoids conversion errors for single precision float.
        */
        case IIAPI_FLT_TYPE:
            if (!(pstmt->fOptions & OPT_FETCHDOUBLE))
                break;
            pird->OctetLength = 8;
            break;

        }   /* end switch */

        pstmt->cbrow = SetColOffset (pstmt->cbrow, pird);
    }   /* end for loop */

    return SQL_SUCCESS;
}


/*
**  PrepareSqldaAndBuffer
**
**  Set up fetch buffer SQLDA, buffer offsets, and allocate buffer.
**
** On entry:  pstmt-->statement block
**
** Returns:  SQL_SUCCESS
**           SQL_ERROR
*/
RETCODE PrepareSqldaAndBuffer(
    LPSTMT  pstmt, int prepare_or_describe)
{
    LPDBC   pdbc  = pstmt->pdbcOwner;
    LPSESS  psess = pstmt->psess;
    UDWORD  cb, cbMax;
    UINT    errnum, errstg;
    RETCODE rc = SQL_SUCCESS;

    TRACE_INTL(pstmt,"PrepareSqldaAndBuffer");
    switch (pstmt->fCommand)
    {
    case CMD_SELECT:
    case CMD_EXECPROC:  /* possibly row-producing procedure */

        /*
        **  Make sure the result set is completely described,
        **  and set up to allocate the fetch buffer:
        */
        rc = PrepareSqlda (pstmt);
        if (rc == SQL_ERROR || pstmt->fStatus & STMT_CONST)
            return rc;

        if (pstmt->fCommand == CMD_EXECPROC  &&  pstmt->pIRD->Count==0)
            return rc;  /* it's OK if procedure returns no result columns */

        pstmt->crowFetch = pstmt->crowFetchMax;
        if (pstmt->crowMax && pstmt->crowMax < pstmt->crowFetch)
            pstmt->crowFetch = (UWORD)pstmt->crowMax;

        errnum = F_OD0044_IDS_ERR_FETCH;    /* "Too many columns to FETCH" */
        errstg = F_OD0006_IDS_FETCH_BUFFER; /* "fetch buffer" */
        break;

    case CMD_INSERT:
    case CMD_UPDATE:
    case CMD_DELETE:
    default:

        return rc;
    }   /* end switch */


    /*
    **  Compute size of fetch buffer or bulk insert buffer:
    */
    cbMax = FETMAX;
    cb    = (UDWORD)pstmt->cbrow * (UDWORD)pstmt->crowFetch;
            /*   rowsize * number of rows to fetch at a time */

    /*
    **  Adjust size of buffer if too big for packet:
    */
    if (pstmt->crowFetch > 255 && cb > cbMax)
    {
        pstmt->crowFetch = 100;
    }

    if (cb > cbMax)
    {             /* if too big, drop down number of rows to fetch at a time */
       pstmt->crowFetch = (UWORD)(cbMax / pstmt->cbrow);
                  /*     max buffer size / rowsize */
       cb = (UDWORD)pstmt->cbrow * (UDWORD)pstmt->crowFetch;
                  /*     rowsize * number of rows to fetch at a time */
    }

    if (cb == 0) return ErrState (SQL_HY000, pstmt, errnum);

    if (prepare_or_describe == SQLDESCRIB)  /* if only getting descriptor */
        return rc;                          /*    then return now */
    
    /*
    **  Allocate fetch buffer or bulk insert buffer:
    */
    pstmt->pFetchBuffer = MEreqmem(0, (size_t)cb, TRUE, NULL);
    if (pstmt->pFetchBuffer == NULL)
        return ErrState (SQL_HY001, pstmt, errstg);

    return rc;
}

/*
**  PrepareStmt
**
**  Prepare a statement (really).
**
**  When called by SQLExecDirect, SQLExecute, or a catalog function,
**  everything needed to prepare the command should be available:
**
**  1. SQL command type.
**  2. SQL syntax length.
**  3. Parameter lengths.
**  4. Parameter values.
**
**  This routine can also be called by SQLNumResultCols, SQLDescribeCol,
**  and SQLColAttributes.  If the precision is not supplied for any
**  parameter other than SQL_DATA_AT_EXEC parameter a default is used.
**
**  On entry: pstmt-->statement block
**            prepare_or_describe 
**                = SQLPREPARE if to fully prepare stmt, parms, and descriptor
**                = SQLDESCRIB if to prepare just enough to get only descriptor
**
**  On exit:  statement is prepared if successful.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
**  Change History
**  --------------
**
**  date        programmer          description
** 01/16/1997   Jean Teng       put the correct pstmt before sqlprepare                         
**
*/
RETCODE PrepareStmt( LPSTMT  pstmt )
{
    LPDBC   pdbc  = pstmt->pdbcOwner;
    LPSESS  psess = pstmt->psess;
    RETCODE rc = SQL_SUCCESS;

    TRACE_INTL (pstmt, "PrepareStmt");

    /*
    **  If OPEN is piggybacked onto the PREPARE, the parameter SQLDA
    **  and buffer are all set to go.  Otherwise take care of it now.
    **  Note that PrepareParms frees any leftover buffers whether
    **  any parms are needed or not.
    */
    if (!(pdbc->sqb.Options & SQB_OPEN))
    {
        rc = PrepareParms (pstmt);
        if (rc != SQL_SUCCESS)
            return rc;
    }
    return rc;
}


/*
**  ResetStmt
**
**  Reset all flags and counters, and free any storage if needed
**  for a statement control block.
**
**  On entry: pstmt-->statement block
**            fFree = TRUE to dump syntax and buffers.
**
**  On exit:  statement is reset.
**
**  Returns:  nothing.
*/
VOID ResetStmt(
    LPSTMT  pstmt,
    BOOL    fFree)
{
    LPDESC    pIRD = pstmt->pIRD;   /* Implementation Row Descriptor */

    TRACE_INTL (pstmt, "ResetStmt");

    if (fFree && !(pstmt->fStatus & STMT_SQLEXECUTE))
    {
        pstmt->fStatus   = 0;
        ResetDescriptorFields(pIRD, 0, pIRD->Count); /* clear all desc fields */
        pIRD->Count = 0;
        pstmt->fCommand  = 0;
        FreeSqlStr (pstmt);
    }
    FreeStmtBuffers (pstmt);
    pstmt->psess       = &pstmt->pdbcOwner->sessMain;   /* default session */
    pstmt->fCatalog    = 0;
    pstmt->prowCurrent = NULL;
    pstmt->crowBuffer  = 0;
    pstmt->crow        = 0;
    pstmt->icolFetch   = 0;
    pstmt->icolPrev    = 0;
    pstmt->icolPut     = 0;
    pstmt->icolSent    = 0;
    pstmt->fCursorOffset = 0;
    pstmt->fCursorPosition = 0;
    pstmt->fCurrentCursPosition = 0;
    pstmt->fCursLastResSetSize = 0;

    *pstmt->szProcName  = EOS;
    *pstmt->szProcSchema= EOS;
    if (pstmt->fStatus & STMT_RETPROC)
        pstmt->fStatus &= ~STMT_RETPROC;

    return;
}


/*
**  SetColOffset
**
**  Compute offset of value and null indicator for a value in
**  the fetch or parm buffer.
**
**  On entry: cb      = offset of start of value.
**            pird   -->ird or ipd column field
**
**  On exit:  COL value and null offsets are set.
**
**  Returns:  Offset of next value.
**  Change History
**  --------------
**
**  date        programmer  description
**
**  02/10/1997  Jean Teng   use sql data type, instead of idms jdt... 
**  02/13/1997  Jean Teng   chged to use api data type
*/

#define ALIGN_OFFSET(offset, N)    ((offset + (N - 1)) & ~(N - 1))

UDWORD SetColOffset(
    UDWORD   cb,
    LPDESCFLD pird)     /* -> ird or ipd column field */
{
    i2   OrigfIngApiType = pird->fIngApiType;

    cb = ALIGN_OFFSET(cb, sizeof(ALIGN_RESTRICT));  /* make sure it's aligned */

# ifdef IIAPI_NVCH_TYPE
    if (pird->fIngApiType == IIAPI_NVCH_TYPE)       /* if WVARCHAR then align */
        cb += 2;             /* the wchars after 2 byte length to i4 boundary */
# endif

    pird->cbDataOffset = cb;

    if (pird->fIngApiType < IIAPI_xxx_to_xxx_TYPE)
        pird->cbDataOffsetAdjustment = 0;          /* if no conversions, 0 it */
    else
    if (pird->fIngApiType == IIAPI_CHA_to_VCH_TYPE) /* if catalog functions are */
       {pird->fIngApiType  = IIAPI_VCH_TYPE;        /* converting CHAR fields to*/
        pird->OctetLength  += 2;                     /* VARCHAR, then adjust for */ 
        pird->cbDataOffsetAdjustment = 2;          /* 2 byte len and tell      */
                                                   /* IIapiGetColumns to fetch */
       }                                           /* into cbDataOffset+2      */
    else
    if (pird->fIngApiType == IIAPI_VCH_to_INT2_TYPE || /* if catalog functions are*/
        pird->fIngApiType == IIAPI_VCH_to_CHR1_TYPE) /* converting VARCHAR fields*/
       {cb +=2;   /* bury the 2 byte length*/      /* to INT2 or CHR1, then first */
        pird->cbDataOffset = cb;                   /* convert to CHAR, then adjust*/ 
        pird->cbDataOffsetAdjustment = -2;         /* for 2 byte len and tell     */
        if (pird->fIngApiType == IIAPI_VCH_to_INT2_TYPE) /* IIapiGetColumns to */
            pird->fIngApiType =  IIAPI_CHR_to_INT2_TYPE;  /* fetch into         */  
        else pird->fIngApiType = IIAPI_CHR_to_CHR1_TYPE;  /* cbDataOffset-2     */
       }
    else
        pird->cbDataOffsetAdjustment = 0;

    if (pird->fIngApiType == IIAPI_VCH_TYPE    ||
        pird->fIngApiType == IIAPI_VBYTE_TYPE  ||
# ifdef IIAPI_NVCH_TYPE
        pird->fIngApiType == IIAPI_TXT_TYPE    ||
        pird->fIngApiType == IIAPI_NVCH_TYPE)
# else
        pird->fIngApiType == IIAPI_TXT_TYPE)
# endif
        cb += 2;
    else
    if (pird->fIngApiType == IIAPI_LVCH_TYPE   ||
        pird->fIngApiType == IIAPI_LBYTE_TYPE  ||
# ifdef IIAPI_LNVCH_TYPE
        pird->fIngApiType == IIAPI_LTXT_TYPE   ||
        pird->fIngApiType == IIAPI_LNVCH_TYPE)
# else
        pird->fIngApiType == IIAPI_LTXT_TYPE)
# endif
        cb += 4;

    cb += (UDWORD)pird->OctetLength;

    if (pird->Nullable)
       { cb = ALIGN_OFFSET(cb, sizeof(i4));  /* align the null indicator */
         if (cb == 0)    /* safety check in case data length is somehow 0 */
             cb  = 4;
         pird->cbNullOffset = cb;
         cb += 4;
       }
    else pird->cbNullOffset = 0;

    /* if catalog functions are converting INT4, CHAR, or FLT fields
       to SMALLINT or CHAR1, reset the sqlvar.type and sqlvar.length
       now that starting offsets have been safely computed */

    if (pird->fIngApiType > IIAPI_xxx_to_xxx_TYPE) /* if conversions */
       {
       if (pird->fIngApiType == IIAPI_INT4_to_INT2_TYPE)
          {pird->fIngApiType  = IIAPI_INT_TYPE;
           pird->OctetLength   = 2;
#ifdef BIG_ENDIAN_INT
           pird->cbDataOffset += 2;  
           pird->cbDataOffsetAdjustment = -2;
#endif
          }  /* end if(IIAPI_INT4_to_INT2_TYPE) */

       if (pird->fIngApiType == IIAPI_CHR_to_INT2_TYPE)
          {pird->fIngApiType  = IIAPI_INT_TYPE;
           pird->OctetLength   = 2; 
          }  /* end if(IIAPI_CHR_to_INT2_TYPE) */

       if (pird->fIngApiType == IIAPI_FLT_to_INT4_TYPE  ||
           pird->fIngApiType == IIAPI_FLT_to_INT2_TYPE)
          {pird->fStatus |= COL_CVTFLTINT;    /* remember FLT to INT needed */
           if (pird->fIngApiType == IIAPI_FLT_to_INT4_TYPE)
                pird->OctetLength   = 4;
           else pird->OctetLength   = 2;
           pird->fIngApiType  = IIAPI_INT_TYPE;
          }  /* end if(IIAPI_FLT_to_INTx_TYPE) */

       if (pird->fIngApiType == IIAPI_CHR_to_CHR1_TYPE)
          {pird->fIngApiType  = IIAPI_CHR_TYPE;
           pird->OctetLength   = 1; 
          }  /* end if(IIAPI_CHR_to_CHR1_TYPE) */
       }

    cb = ALIGN_OFFSET(cb+8, sizeof(ALIGN_RESTRICT));  /* align row-size + a safety*/

    if (OrigfIngApiType > IIAPI_xxx_to_xxx_TYPE) /* if there were conversions */
        SetDescDefaultsFromType(NULL, pird);   /* reset Type, Precision, etc. */

    return cb;
}


/*
**  SetColVar
**
**  Fill in an IPD descriptor from APD descriptor and data.
**
**  Called by:
**            PrepareParms
**
**  On entry: pstmt  -->STMT block.
**            papd-->application parameter descriptor
**            pipd-->implementation parameter descriptor
**
**  On exit:  SQLVAR is built describing parameter.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**  Change History
**  --------------
**
**  date        programmer  description
**
**  02/07/1997  Jean Teng       dont put idms data type in sqlvar->SQLTYPE 
**  02/10/1997  Jean Teng   scale/precision not applicable to ingres date
**  02/13/1997  Jean Teng   put api data type in sqlvar->SQLTYPE
*/
RETCODE  SetColVar(
    LPSTMT     pstmt,
    LPDESCFLD  papd,
    LPDESCFLD  pipd)
{
    LPSQLTYPEATTR ptype;

    TRACE_INTL (pstmt, "SetColVar");
    /*
    **  Error if parameter not bound:
    */
    if (!(papd->fStatus & COL_BOUND))
        return ErrState (SQL_07001, pstmt);

    ptype = CvtGetSqlType (pipd->ConciseType, pstmt->fAPILevel, 
        pstmt->dateConnLevel);
    
    if (ptype == NULL || !ptype->cbSupported)
        return ErrState (SQL_HY004, pstmt); /* SQL type unsupported */

    /*
    **  Map ODBC SQL type to Ingres API type:
    */
    pipd->fIngApiType = ptype->fIngApiType;  /* Ingres API data type */


    /*
    **  Set up null indicator (keyed by presence or absence of IndicatorPtr:
    */
    pipd->Nullable = papd->IndicatorPtr ? SQL_NULLABLE : SQL_NO_NULLS;

    /*
    **  Set up and length:
    */
    pipd->OctetLength     = ptype->cbLength;  /* default byte length */

    switch (pipd->ConciseType)
    {
    case SQL_DECIMAL:
    case SQL_NUMERIC:

        pipd->OctetLength = pipd->Precision/2+1;  /* length in bytes */
        /* Precision and Scale already filled in by SQLBindParameter */
        break;

    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:

        pipd->OctetLength = ParmLength (pstmt, papd, pipd);
        if (pipd->cvtToWChar && (pstmt->pdbcOwner->fOptions & 
            OPT_DEFAULTTOCHAR))
            pipd->OctetLength *= sizeof(SQLWCHAR);
        break;
    case SQL_TYPE_NULL:   /* special typeless null case */
        pipd->OctetLength = 2;
    }
    return SQL_SUCCESS;
}

/*
**  ScanPastSpaces
**
**  Scan past blanks if any.
**
**  On entry: p  -->where to start scan
**
**  On exit:  
**
**  Returns:  pointer to character just after the spaces. 
**
**  Notes:
**
**  Change History
**  --------------
**
**  date        programmer  description
**
**  07/31/1997  Dave Thole  Created 
*/
char * ScanPastSpaces(char * p)
{
   while (CMspace(p))  CMnext(p);       /* scan past blanks */
   return(p);
}

/*
**  ScanSearchCondition
**
**  Scan a <search-condition> clause until end of it.
**
**  On entry: p  -->where to start scan
**
**  On exit:  
**
**  Returns:  pointer to start of token just after 
**            the <search-condition>.
**
**  Change History
**  --------------
**
**  date        programmer  description
**
**  07/31/1997  Dave Thole  Created 
*/
CHAR     * ScanSearchCondition(CHAR * p, CHAR ** KeywordList)
{
 int parencount=0;
 int bracecount=0;
 SQLINTEGER  tokenlen;
 SQLINTEGER  keywordlen;
 int c=0,i;          /* c == *p */
 CHAR     * q;
 CHAR     * kp;

 for (;;)
 { 
   CMcpychar(p,(CHAR *)&c);     /* c == *p for efficient code generation */
   if (c == EOS )    break;     /* if hit null terminator, return */

   if (CMspace(&c))
      { 
	    while (CMspace(p))  CMnext(p);              /* scan past blanks */
        continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,"'")  || !CMcmpcase(&c,"\"") )
      { 
	    CMnext(p);                                 /* increment past the quote  */
        while ( (*p!=EOS)  &&  CMcmpcase(p,&c) )  CMnext(p);        /* scan past 'quoted string' */
        if (!CMcmpcase(p,&c))  CMnext(p);                   /* if ending quote and not \0, incr*/
        continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,"("))
      { parencount++;                        /* increment parenthesis nest */
        CMnext(p);
        continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,")"))
      { if (--parencount < 0)  break;        /* decrement parenthesis nest */
        CMnext(p);
        continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,"{"))
      { bracecount++;                        /* incr escape sequence brace nest */
        CMnext(p);
        continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,"-")  &&  STbcompare(p,10,(char *)ODBC_STD_ESC,10,TRUE)==0)   /* "--(*vendor" */
      { bracecount++;                        /* incr escape sequence brace nest */
	p+=10;
	continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,"}"))
      { if (--bracecount < 0)  break;        /* decr escape sequence brace nest */
        CMnext(p);
        continue;                            /* continue the main for loop */
      }

   if (!CMcmpcase(&c,"*")  &&  STbcompare(p,4,"*)--",4,TRUE)==0)
      { if (--bracecount < 0)  break;        /* decr escape sequence brace nest */
	p+=4;
	continue;                            /* continue the main for loop */
      }

   if (parencount || bracecount)             /* if inside a paren or brace nest */
      { 
        CMnext(p);
        continue;                            /* continue the main for loop */
      }

   if (!CMalpha(&c) && CMcmpcase(&c,"_"))              /* if NOT the start of a keyword */
      { 
        CMnext(p);
        continue;                            /* continue the main for loop */
      }
   q=p;
   CMnext(q);
   q=ScanRegularIdentifier(q);  /* scan the identifier */
   tokenlen = q-p;                /* q->char past the identifier */

   if (tokenlen == 5 &&
         (STbcompare(p,5,"WHERE",5,TRUE)==0  ||
          STbcompare(p,5,"ORDER",5,TRUE)==0  ||
          STbcompare(p,5,"GROUP",5,TRUE)==0  ||
          STbcompare(p,5,"UNION",5,TRUE)==0))    break;     /* return */
   if (tokenlen == 6 &&
         (STbcompare(p,6,"HAVING",6,TRUE)==0))   break;     /* return */

   if (KeywordList)   /* if also searching for specific keywords */
      for(kp=KeywordList[0],i=0; kp; kp=KeywordList[++i])
      {  keywordlen = STlength(kp);   /* kp->keyword;  keywordlen=keyword len */
         if (keywordlen != tokenlen) continue;
         if (STbcompare(p,(i4)tokenlen,kp,(i4)tokenlen,TRUE)==0)
            return(p);
      } /* end for(;;) loop through keyword list */

   p=q;    /* resume the scan following the identifier */
 } /* end for (;;) */

 return(p);
}

/*
**  ScanQuotedString
**
**  Scan a quoted literal or identifier until end of it.
**
**  On entry: p  -->where to start scan
**
**  On exit:  
**
**  Returns:  pointer to character just after 
**            the string.
**  Notes:
**
**  Change History
**  --------------
**
**  date        programmer  description
**
**  12/12/1997  Dave Thole  Created 
*/
CHAR     * ScanQuotedString(CHAR * p)
{
   CHAR bQuote =*p;

   for (p++; *p; p++)
     { if (*p == bQuote)
          {p++;
           if (*p != bQuote)
              return(p);      /* return if not quote,quote */
          }
     }

   return(p);
}

/*
**  ScanRegularIdentifier
**
**  Scan a regular identifier (not delimited) until end of it.
**
**  On entry: p  -->where to start scan
**
**  On exit:  
**
**  Returns:  pointer to character just after 
**            the <regular identifier>.
**  Notes:
**
**  Change History
**  --------------
**
**  date        programmer  description
**
**  07/31/1997  Dave Thole  Created 
*/
CHAR     * ScanRegularIdentifier(CHAR * p)
{
   /*int c =*p; */

   while (CMalpha(p)         ||  
          CMdigit(p)         || 
          !CMcmpcase(p,"_")  ||
          !CMcmpcase(p,"@")  ||
          !CMcmpcase(p,"#")  ||
          !CMcmpcase(p,"$")  )  
   { 
	   CMnext(p);
       /*c=*p;  */
   }

   return(p);
}

/*
**  ScanNumber
**
**  Scan a integer, decimal, or floating-point number
**
**  On entry: p            --> where to start scan
**            papiDataType --> where to put datatype
**
**  Returns:  pointer to character just after 
**            the string.
**  Notes:
**
**  Change History
**  --------------
**
**  06/07/2000  Dave Thole  Created 
*/
CHAR     * ScanNumber(CHAR * p, II_INT2 * papiDataType)
{
   II_BOOL fPeriod   = FALSE;
   II_BOOL fExponent = FALSE;

   if (!(CMdigit(p)  ||  *p == '-'  ||  *p == '+'))
      return(p);   /* we should never have been called */

   p++;  /* assume that we were called with starting
            p-> 'digit', '+', or '-'                 */

   while(CMdigit(p))         /* skip over leading digits */
        p++;                 /* -123                     */

   if (*p == '.')            /* if decimal point */
      { p++;
        fPeriod = TRUE;
        while(CMdigit(p))       /* skip over fraction digits */
           p++;                 /* -123.45                   */
      }

   if (*p == 'e'  ||  *p == 'E')  /* if exponent */
      { p++;
        fExponent = TRUE;
        if (*p == '-'  ||  *p == '+')
           p++;
        while(CMdigit(p)) /* skip over exponent digits */
           p++;                 /* -123.45e-01               */
      }

   *papiDataType = fExponent ? IIAPI_FLT_TYPE :
                   fPeriod   ? IIAPI_DEC_TYPE :
                               IIAPI_INT_TYPE;

   return(p);
}

/*
**  CountLiteralItems
**
**  count the question marks and left brackets in a query.
**
**  On entry: p-->SQL statement text.
**            cbSqlStr = length of SQL statement text.
**  Returns:  number of extra bytes to allocate, or 0 if stmt is a NULL pointer
**           
**  Change History
**  --------------
**
**  date        programmer  description
**
**  12/11/2000  Fei Wei	    created
**							 
*/

DWORD CountLiteralItems(SQLINTEGER cbSqlStr, CHAR * p, UDWORD fAPILevel)
{
    i2 count=0;
    i2 columnNameLength=0;
    i2 i;
    i2 varLen=0;
    i2 queryLen=0;

    if (!p)
         return count;

    while(cbSqlStr-- > 0  && CMcmpcase(p,nt))
    {
        if(!CMcmpcase(p,"?")) 
            count +=3;
        else if ( fAPILevel > IIAPI_LEVEL_3 && !CMcmpcase(p, "{"))
        {
            CMnext(p);
            cbSqlStr--;
            while (CMwhite(p)) 
            {
                cbSqlStr--;
                CMnext(p);
            }
 
            /*
            ** The function scalars DAYOFWEEK, DAYOFYEAR and MONTHNAME
            ** can range in length from 500 to 750 characters, and
            ** reiterate the column name multiple times.
            ** Discover the column name length and add it to the
            ** count, multiplied by the number of occurrences.
            ** Otherwise, 550 is a safe margin for converted scalars.
            */
            if (!STbcompare (p, 2, "fn", 2, TRUE))
            {
                CMnext(p);
                CMnext(p);
                cbSqlStr -= 2;

                while(CMwhite(p)) 
                {
                    CMnext(p);
                    cbSqlStr--;
                }
                if (!STbcompare(p,9,"DAYOFWEEK",9,TRUE))
                {
                    queryLen = 550;
                    varLen = 18;
                }
                else if (!STbcompare( p,9,"MONTHNAME",9,TRUE))
                {
                    queryLen = 500;
                    varLen = 14;
                }
                else if (!STbcompare(p,9,"DAYOFYEAR",9,TRUE))
                {
                    queryLen = 750;
                    varLen = 20;
                }
                if (varLen)
                {
                    for (i = 0; i < 9; i++)
                        CMnext(p);
                    cbSqlStr -= 9;
                    while(CMwhite(p)) 
                    {
                        cbSqlStr--;
                        CMnext(p);
                    }
                    CMnext(p);
                    cbSqlStr--;
                    while (*p && CMcmpcase(p,")")) 
                    {
                        columnNameLength++;
                        CMnext(p);
                        cbSqlStr--;
                    }
                    count += (varLen*columnNameLength) + queryLen;
                    varLen = 0;
                    columnNameLength = 0;
                    queryLen = 0;
                }
                else
                    count += 550;
            }
            else
                count += 30;
        }
        
        CMnext(p);
    }
    return count;
}
/*
**  removeEndSemicolon
**
**  On entry: szSqlStr -> query string.
**
**  Returns:  TRUE if a semicolon was found.
**
**  Notes:    Command string is searched for a terminating semicolon
**            and replaces it with a null character if found. 
*/
static BOOL removeEndSemicolon(LPSTR szSqlStr)
{
    CHAR   *p;
    SDWORD cb, i;

    if (!szSqlStr)
        return FALSE;

    if (!STlength(szSqlStr))
        return FALSE;

    cb = STlength(szSqlStr);
    p = &szSqlStr[cb]; 

    CMprev(p,szSqlStr); /* p points to the last char */

    for (i=1; i<=cb && CMwhite(p); CMprev(p,szSqlStr),++i)
        ;

    if ( i <= cb && *p == (CHAR)';')
    {
        /*
        ** 'i <= cb' prevents the usage of *p if the whole string is
        ** white spaces or empty.
        */
        *p = (CHAR)'\0';
        return TRUE;
    }
    return FALSE;
}
