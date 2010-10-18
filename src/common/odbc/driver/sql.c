/*
** Copyright (c) 1992, 2009 Ingres Corporation
*/

#include <compat.h> 
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <cm.h>
#include <cv.h> 
#include <lo.h> 
#include <me.h>
#include <nm.h> 
#include <er.h>
#include <si.h> 
#include <st.h> 
#include <tr.h>
#include <gl.h>
#include <iicommon.h>
#include <adf.h>
#include <iiapi.h>
#include <erodbc.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"               /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include <idmsucry.h>
#include <idmsocvt.h>               /* ODBC conversion routines */

/*
** Name: sql.c
**
** Description:
**	Basic Ingres API functions for Ingres SQL.
**
** History:
**	18-dec-1991 (rosda01)
**	    Initial coding.
**	19-nov-1992 (rosda01)
**	    Unbundled from SQI.
**	16-mar-1993 (rosda01)
**	    Spiffed up tracing.
**	22-feb-1996 (rosda01)
**	    Port to Win32.
**	18-sep-1996 (rosda01)
**	    Piggybacked commands.
**	09-jan-1997 (tenje01)
**	    API connect/disconnect
**	03-feb-1997 (tenje01)
**	    psqlvar->SQLTYPE contains API data type
**	05-feb-1997 (tenje01)
**	    odbc_getDesc ...
**	06-feb-1997 (tenje01)
**	    odbc_cancel, odbc_setDescr, odbc_putParms,
**	    odbc_printErrors ...
**	07-feb-1997 (thoda04)
**	    Converted malloc to UtMalloc calls 
**	07-feb-1997 (thoda04)
**	    Moved connH/tranH to SESS
**	07-feb-1997 (tenje01)
**	    odbc_putParms ...
**	07-feb-1997 (tenje01)
**	    Chgd SQL_DATE/TIME/TIMESTAMP in SqlToApiDataType to
**	    IIAPI_CHA_TYPE 
**	10-feb-1997 (tenje01)
**	    Added SetConnectParam
**	10-feb-1997 (tenje01)
**	    When bind sql_c_date to sql_varchar, CvtDateChar did not
**	    put the length field required by api 
**	12-feb-1997 (thoda04)
**	    Added use of cbDataOffsetAdjustment for CH->VCH convert
**	    field required by api
**	13-feb-1997 (tenje01)
**	    supporting long data ...
**	13-feb-1997 (tenje01)
**	    SpecialApiDataType ...  
**	14-feb-1997 (tenje01)
**	    skipping unwanted segs w/o destroying the 1st seg in buffer
**	20-feb-1997 (tenje01)
**	    fixed the following problems:
**	    1.insert with paramoptions
**	    2.use/put null indicator when insert/select null values 
**	24-feb96 (tenje01)
**	    Added code for default in case SQLPREPARE
**	25-feb-1997 (tenje01)
**	    fixed fetching bound long data column
**	27-feb-1997 (tenje01)
**	    Added code for calling a procedure
**	03-mar-1997 (tenje01)
**	    supporting SQLDriverConnect
**	05-mar-1997 (tenje01)
**	    odbc_getColumns should update icolFetch   
**	11-mar-1997 (tenje01)
**	    Added code for calling db procedures 
**	20-mar-1997 (tenje01)
**	    reprepare if cursor update/delete 
**	21-mar-1997 (thoda04)
**	    Clear the null indicator if not null value 
**	25-mar-1997 (tenje01)
**	    commit if any transaction pending at disconnect  
**	28-mar-1997 (tenje01)
**	    odbc_putParms, donot special case SQL_CHAR 
**	01-apr-1997 (tenje01)
**	    using SQL_LOGIN_TIMEOUT value at connect time. 
**	02-apr-1997 (tenje01)
**	    Removed my changes of 3-1997 since it's ingres spec
**	    not odbc spec 
**	04-apr-1997 (tenje01)
**	    calling procedures with literal parameters
**	07-apr-1997 (tenje01)
**	    checking form null in getcolumnslong
**	14-apr-1997 (thoda04)
**	    Added "(local)" support
**	15-apr-1997 (tenje01)
**	    getting server release number after connected
**	28-apr-1997 (tenje01)
**	    added query timeout support
**	28-apr-1997 (thoda04)
**	    Cleaned up warnings for 16-bit
**	29-apr-1997 (tenje01)
**	    fixed release number
**	04-may-1997 (thoda04)
**	    SetLockMode since ING_SET not honored.
**	13-may-1997 (tenje01)
**	    fixed length in SetDescr and PutParms for SQL_TIMESTAMP
**	15-may-1997 (tenje01)
**	    use API's sqlstate as msg if no msg provided by api
**	16-may-1997 (thoda04)
**	    Minimum descriptor length is 1.
**	16-may-1997 (tenje01)
**	    added ING_SET support
**	20-may-1997 (thoda04)
**	    Removed the SetLockMode code now that ING_SET does it.
**	09-jun-1997 (thoda04)
**	    Added procedure parameter support for the gateways.
**	16-jun-1997 (tenje01)
**	    used IIAPI_QT_CURSOR_UPDATE to handle cursor update
**	20-jun-1997 (tenje01)
**	    not include schema in proc name
**	23-jun-1997 (thoda04)
**	    Added gateway "WITH parameter" support.
**	30-jun-1997 (tenje01)
**	    1.made IIAPI_GE_MESSAGE return SQL_SUCCESS_WITH_INFO instead
**	      of SQL_ERROR
**	    2.added schema in GetProcName
**	    3.when set descriptor, SQLNULL==0 means desc. is nullable
**	      (see SetColVar in prepare.c)
**	    4. added proc owner to the query in GetProcParamNames
**	    5. added a link list for error msgs           
**	08-jul-1997 (thoda04)
**	    Restored SQLNULL==0 means no null indicator provided.
**	08-jul-1997 (thoda04)
**	    Restored missing "else".
**	08-jul-1997 (tenje01)
**	    using pcol->pcbValue, instead of SQLNULL to set ds_nullable 
**	    in SetDescr and ExecProcSetDescr
**	09-jul-1997 (tenje01)
**	    applied 5-13-97 changes to ExecProcSetDescr and ExecProcPutParms   
**	22-jul-1997 (tenje01)
**	    set precision of mny data type in getdescriptor so that
**	    iiapi_convertData in CvtIngresMnyToDec will work correctly
**	    for large mny values
**	24-jul-1997 (thoda04)
**	    Prevent runaway on release number scan.
**	24-jul-1997 (tenje01)
**	    iiapi_autocommit after connection/prior to disconnection if
**	    autocommit is on
**	28-jul-1997 (tenje01)
**	    added code to process autocommit in ProcessIngSet
**	29-jul-1997 (tenje01)
**	    a prepared insert stmt could not be reexecuted by just
**	    calling SQLExecute in manual commit mode. 
**	05-aug-1997 (thoda04)
**	    Bug fix to SQLCOMMIT autocommit code.
**	13-aug-1997 (tenje01)
**	    modified odbc_setDescr,odbc_putParms,
**	    odbc_ExecProcSetDescr,odbc_ExecProcPutParms
**	    to pass Ingres internal date format to gateways
**	08-sep-1997 (tenje01)
**	    using pcol->pParmBuffer instead of pstmt->pParmBuffer in PutParm
**	17-sep-1997 (thoda04)
**	    Skip ROLLBACK if autocommit on (OpenApi gets upset).
**	19-sep-1997 (thoda04)
**	    Wait for result of iiapi_close.
**	23-sep-1997 (tenje01)
**	    GPF when connect to db name > 17 chars
**	11-oct-1997 (thoda04)
**	    Add error messages for odbc_malloc
**	12-oct-1997 (thoda04)
**	    Add debugging code for odbc_malloc
**	22-oct-1997 (tenje01)
**	    Fix bug 86144 
**	24-oct-1997 (tenje01)
**	    my previous fix didnot work if no SQLBindCol was issued  
**	03-nov-1997 (tenje01)
**	    copy SQLSTATE from IIAPI_GETEINFOPARM to sqlca
**	10-nov-1997 (tenje01)
**	    added support of role name and pwd
**	11-nov-1997 (thoda04)
**	    Documentation clean-up
**	17-nov-1997 (thoda04)
**	    DB_NAME_CASE from iicapabilities support
**	    Fixed a memory leak in GetServerReleaseNumber
**	21-nov-1997 (thoda04)
**	    Add support for piggyback CLOSE if fetch sqlcode=100
**	    Multiple cursor support if AUTOCOMMIT=ON
**	06-dec-1997 (tenje01)
**	    convert C run-time functions to CL functions 
**	22-jan-1998 (tenje01)
**	    odbc_query, if readonly transaction and using cursors
**	    for select, then add "for readonly"
**	10-feb-1998 (tenje01)
**	    always convert szSERVERTYPE to upper case
**	12-dec-1997 (thoda04)
**	    Added opt to convert three-part colnames to two-part names
**	06-feb-1998 (thoda04)
**	    Added support for blob > 32K
**	17-feb-1998 (thoda04)
**	    Added cursor SQL_CONCURRENCY=SQL_CONCUR_READ_ONLY
**	26-feb-1998 (thoda04)
**	    Don't turn on autocommit yet after connect.
**	13-apr-1998 (thoda04)
**	    Added GROUP support.
**	21-apr-1998 (Dmitriy Bubis)
**	    Added II_DATE_FORMAT support.
**	22-may-1998 (thoda04)
**	    Report "invalid API handle" errors.
**	27-jul-1998 (thoda04)
**	    Default server_type to "INGRES" if no supplied.
**	30-jul-1998 (Dmitriy Bubis)
**	    Don't add "for readonly" clause if it is already
**	    present in a stmt (Bug #92236)
**	03-aug-1998 (thoda04)
**	    Fix odbc_getColumnsLong to use correct pcol. Added comments.
**	09-sep-1998 (Dmitriy Bubis)
**	    Turn on OPT_CONVERT_3_PART_NAMES flag when talking to 6.x DBMS
**	21-oct-1998 (Dmitriy Bubis)
**	    Don't overwrite the native error code SQLERC with gp_status.
**	    (Bug #93966)
**	26-oct-1998 (thoda04)
**	    Added support for SQL_ATTR_ENLIST_IN_DTC
**	30-dec-1998 (padni01)
**	    Don't overwrite the native SQLSTATE with HY000 (Bug 94286)
**	06-jan-1999 (lunbr01)
**	    Depending on the user id and password combination, the
**	    ODBC driver does not pass in the password to Ingres Net.
**	    When checking for no password entered, the code was 
**	    chkg the 1st byte of the encrypted PWD; i.e., checking
**	    for Null string. However, the encrypted PWD is not a
**	    valid C string. Changed code to check the length field for
**	    for PWD, which is accurate.  (Bug #93967)
**	07-jan-1999 (thoda04)
**	    Do a real PREPARE on SQLPrepare to avoid expensive EXECUTE.
**	20-jan-1999 (thoda04)
**	    Clear "session commit needed" flag after a commit.
**	03-feb-1999 (thoda04)
**	    Remove unnecessary SQLCIB, SQLPIB, SQLSID
**	17-feb-1999 (thoda04)
**	    Ported to UNIX.
**	08-feb-1999 (thoda04)
**	    Always do a IIapi_getQueryInfo before IIapi_close.
**	09-feb-1999 (thoda04)
**	    IIapi_cancel the data stream before IIapi_close.
**	10-feb-1999 (thoda04)
**	    Always NULL out transaction pointer after commit/rollback,
**	    even on errors.
**	23-jun-1999 (thoda04)
**	    Clean up transaction handle after execution of a database
**	    procedure that contains a commit/rollback.
**	07-jul-1999 (Bobby Ward)
**	    Fixed Bug 94747 Ing_Set for AUTOCOMMIT problem.			
**	24-oct-1999 (loera01)
**	    Bug 91557: For odbc_ExecProcSetDescr() and
**	    odbc_ExecProcPutParms(), added a second parameter
**	    (the first one is always the procedure name) if the
**	    procedure was qualified with a schema.
**	24-oct-1999 (thoda04)
**	    Bug 99265 fix for unqualified procs w/ parms.
**	    Use dbmsinfo('username') as default proc owner. 
**	    Rename GetServerReleaseNumber to GetServerInfo.
**	05-nov-1999 (thoda04)
**	    Allow insert of zero length strings into blobs.
**	12-nov-1999 (loera01)
**	    Bug 99363: Honor II_DATE_CENTURY_BOUNDARY
**	    in ODBC.  This code is redundant to the API
**	    and should be eliminated eventually.
**	15-nov-1999 (thoda04)
**	    Added option to return blank date as NULL.
**	17-dec-1999 (thoda04)
**	    Break dbmsinfo(_version and username) into 2 queries
**	    for gateways.
**	29-dec-1999 (thoda04)
**	    Recover from trying to close incomplete query
**	12-jan-2000 (thoda04)
**	    Added option to return date 1582-01-01 as NULL.
**	31-mar-2000 (thoda04)
**	    Match procedure schema name against 
**	    szDBAname, szSystemOwnername.
**	25-apr-2000 (thoda04)
**	    Make separate catalog session optional
**	01-may-2000 (loera01)
**	    Bug 100747: Replaced check for "INGRES" in
**	    the registry with isServerClassINGRES 
**	    macro, so that user-defined server classes 
**	    are interpreted as Ingres classes.
**	08-may-2000 (thoda04)
**	    Trap and remember dead connections 
**	    for SQL_ATTR_CONNECTION_DEAD support
**	15-may-2000 (thoda04)
**	    Check what stmts can be PREPAREd/EXECUTEd
**	24-may-2000 (thoda04)
**	    Additional isServerClassINGRES conversions
**	    for user-defined server class support
**	05-jun-2000 (thoda04)
**	    Added support for numeric_overflow=ignore.
**	08-jun-2000 (thoda04)
**	    Added support of global temp table proc parm
**	20-jul-2000 (thoda04)
**	    Add bullet-proofing when checking SQL_DEFAULT_PARAM
**	03-oct-2000 (thoda04)
**	    Set descriptor for SQL_VARCHAR parameter
**	    to be actual length instead of ColumnSize.
**	16-oct-2000 (loera01)
**	    Added support for II_DECIMAL (Bug 102666).
**	17-oct-2000 (thoda04)
**	    Read the cached servertype parms
**	30-oct-2000 (thoda04)
**	    Added CatSchemaNULL for NULL TABLE_SCHEM
**	11-jan-2001 (thoda04)
**	    Integrate MTS/DTC support into 2.8
**	02-jul-2001 (loera01)
**	    Cast argument of odbc_malloc to u_i4.
**	13-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	16-jul-2001 (thoda04)
**	    Add support for SQL_WLONGVARCHAR (Unicode blobs)
**	26-jul-2001 (thoda04)
**	    Cast argument of odbc_malloc to SIZE_TYPE.
**	31-jul-2001 (somsa01)
**	    Add support for parameter offset binding.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-mar-2002 (thoda04)
**	    Add support for row-producing procedures.
**	16-jan-2002 (thoda04)
**	    Add support for DisableCatUnderscore search char.
**	21-mar-2002 (thoda04)
**	    Break out SqlCancel and odbc_cancel into primitives.
**	18-apr-2002 (weife01) Bug #106831
**          Add option in advanced tab of ODBC admin which allow the 
**          procedure update with odbc read only driver 
**      05-jun-2002 (loera01) Bug 107949
**          In ScanProcParmsAndBuildDescriptor(), make sure all of the 
**          relevant fields in pprocapd and pprocid are copied from papd 
**          and pipd.
**      22-jun-2002 (weife01) Bug #108044: added case when II_DECIMAL 
**          is set to "," on client, and "Support II_DECIMAL" is not 
**          checked in ODBC Admin, ODBC driver allows inserting "." as 
**          decimal point.
**      27-jun-2002 (loera01) Bug 108136: checked for IIapi_wait() status
**         to prevent infinite loops.  Made all  references to cancelParm
**         structure dynamic.  Set gp_closure argument to NULL in
**         cancelParm; IIapi_cancel() doesn't use it.
**      27-jun-2002 (loera01) Bug 108138: removed default invocation of
**         odbc_cancel() for SQL_CLOSE operations. Always invoke
**         odbc_close() first.
**      18-jul-2002 (loera01) Bug 108301  
**         Removed resursive call to odbc_getColumnsLong when bound colunms
**         are present.  Utilized new LONGBUFFER in the application row
**         descriptor.
**      30-jul-2002 (weife01) SIR #108411
**         Made change to functions SQLGetInfo and SQLGetTypeInfo to support 
**         DBMS change to allow the VARCHAR limit to 32k; added internal 
**         function GetSQLMaxLen.
**      04-nov-2002 (loera01) Bug 108536
**         Second phase of support for SQL_C_NUMERIC.  Added support for
**         II_DECIMAL configuration in the ODBC administrator.
**      12-sep-2002 (ashco01) Bug 108217
**         introduced call to IIapi_setConnectParam() in SQLconnect() to set 
**         IIAPI_CP_EFFECTIVE_USER if the user supplied an alternate username without a
**         password. This ensures that only users with ingres privilege to do so can 
**         connect as an alternate user via a vnode utilising an installation password.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      07-mar-2003 (loera01)
**          Use a common definition for ODBC_INI.
**      08-mar-2003 (loera01) SIR 109643
**          Conditionally compile IIAPI_SETENVPRMPARM according to the
**          presence of the macro definition IIAPI_VERSION_2 instead of
**          IIAPI_SETENVPRMPARM.
**      27-May-2003 (loera01) Bug 110318
**          Cleaned up the treatment of the decimal character in
**          ProcessII_DECIMAL() and accompanying logic.
**     22-sep-2003 (loera01)
**          Added internal ODBC tracing.
**     17-oct-2003 (loera01)
**          Added query state machine.  Changed "IIsyc_" routines to
**          "odbc_".  Separated cases SQLEXEC_DIRECT, SQL_PUTPARMS,
**          and SQL_EXECPROC from SQLPREPARE and SQLEXECUTE.
**     20-oct-2003 (loera01)
**          Completed SQLPREPARE sequencing and added sequence for
**          SQL_DESCRIBE.
**     05-nov-2003 (loera01)
**          Sending parameter descriptors is now invoked from
**          SQLExecute() or SQLExecDirect().
**     10-nov-2003 (weife01) Bug 111261
**         Add IIsyc_disconn() in SQLCONNECT when SQLConnect() returns SQL_ERROR
**         to clean up the handles in API.
**     13-nov-2003 (loera01)
**          Add capability to send true segments from SQLPutData().
**          New routines SqlPutSegment() and odbc_putSegment().
**          Removed obsolete arguments from SqlToIngresAPI().
**     20-nov-2003 (loera01)
**          More cleanup on arguments sent to SqlToIngresAPI().
**          Rolled in database procedures to the state machine.
**     21-nov-2003 (loera01)
**          In odbc_query_sm(), made sure odbc_close() is always
**          called after a select query and added odbc_getQInfo().
**          In odbc_putSegment(), restored blob data only after 
**          calling odbc_getResult().  In odbc_query_sm(), if
**          odbc_getDescr() returns no descriptor, don't call
**          PrepareSqldaAndBuffer().
**    04-dec-2003 (loera01) SIR 111408
**          Don't allocate internal memory for bound blob columns.
**          Just use the application bound buffer.
**    04-dec-2003 (loera01) SIR 111409
**          Added the capability to fetch "true" segments from SQLGetData().
**          Previously all the blob data had been fetched before SQLGetData()
**          was ever invoked.
**    18-dec-2003 (loera01) Bug 111492
**          In odbc_getColumnsLong(), if a blob is unbound, check the 
**          remainder of the result set for bound columns.  In odbc_query_sm(),
**          return an error if odbc_getQInfo() finds an error.  In
**          odbc_getColumnsLong(), make sure the last colunm in the
**          PIRD is checked for bound/unbound status.
**    12-dec- 2004  Fei Wei  Bug 111262
**          Improve the performance by canceling fetching data if 
**          application did not request data exclusivly when using Select Loop.
**    16-jan-2004  (loera01) Bug 111262
**          The prior fix did not allow for when SQLSetCursorName() overrode
**          select loop configuration and if a non-select query was issued.
**    16-jan-2004 (loera01) Bug 111262
**          Added a check for STMT_EXECUTE.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**    29-apr-2004 (loera01)
**          Before SetConnectParam() is invoked in the SQLCONNECT case,
**          initialize the connection handle with the API environment
**          handle.
**    16-sep-2004 (Ralph.Loen@ca.com) Bug 113063
**          In odbc_setDescriptor(), set a minimum length in the descriptor
**          if the target column is a byte, char, or nchar, and the data
**          has a length of zero.  Otherwise, the API will return
**          "Ingres API out of memory error".
**    15-oct-2004 (Ralph.Loen@ca.com) Bug 113241
**          In odbc_setDescr(), make sure schema qualifiers for database
**          procedures get set up in IIapi_setDescriptor(). 
**    01-dec-2004 (Ralph.Loen@ca.com) Bug 113533
**          Add support for DBMS passwords.
**    10-dec-2004 (Ralph.Loen@ca.com) SIR 113610
**          Added support for typeless nulls.
**    30-dec-2004 (Ralph.Loen@ca.com) Bug 113691
**          In odbc_conn(), make sure the co_type field in the API connection 
**          handle is initialized with IIAPI_CT_SQL.
**    26-Jan-2005 (Ralph.Loen@ca.com) Bug 113798
**          In odbc_getSegment(), get more API blob segments if more to get and
**          the fetched length is less than or equal to the requested segment
**          length--not if only less than the requested length.
**    26-Apr-2005 (Ralph.Loen@ca.com) Bug 114418
**          In SqlToIngresAPI(), set all statement status fields to zero in the
**          SQLCOMMIT and SQLROLLBACK cases.
**    06-may 2005 (weife01) Bug 113668
**          When passing date, time for DB2, IDMC and DCOM, send as character 
**          instead of binary.
**    26-May-2005 (Fei.Wei@ca.com) Bug 114099
**          ODBC will strip off double quote around procedure name, so it'll
**          not take it as part of the procedure name.
**    13-jun-2005 (loera01) Bug 114672
**          Part of the fix for bug 113668 introduced regression when Ingres
**          was the back end.  In SpecialApiDateType(), check for an Ingres
**          backend before coercing SQL_TYPE_TIMESTAMP to IIAPI_CHA_TYPE. 
**    03-Aug-2005 (loera01) Bug 114985
**          In SqlToIngresAPI(), clear STMT_PREPARED bit from all statement 
**          status fields in the SQLCOMMIT and SQLROLLBACK cases.  Setting to 
**          zero can nullify valid flags.
**    26-Aug-2005 (loera01) Bug 115111
**          In SqlToIngresAPI(), do not set the query type to QST_PREPARE if
**          doing direct execution of a query with multiple rows.  Always
**          execute as QST_GENERIC.
**     27-sep-2005 (loera01) Bug 115051
**          In odbc_getSegmtnt(), use new prevOctetLength field in the row
**          descriptor to track previous octet lengths.  Reallocate if the
**          current octet length is greater than the previous length.
**     05-Jan-2005 (loera01) Bug 115621
**          In odbc_getSegment(), check for more segments after          
**          IIapi_getColumn() is called.  Set the COL_MULTISEGS flag and
**          call GetSegment() if more segments are detected.
**     10-Mar-2006 (Ralph Loen) Bug 115831
**          In odbc_conn(), check the stored length of the password
**          to determine a non-null string, rather than looking in the
**          first byte of the encrypted password.
**     10-Mar-2006 (Ralph Loen) Bug 115833
**          In SqlToIngresApi(), fetch the multibyte fill character value
**          from the registry, if present.
**     16-June-2006 (Ralph Loen) SIR 116260
**          Added odbc_getInput() and SQLDESCRIBE_INPUT case to 
**          SqlToIngresAPI() to support SQLDescribeParam().
**      03-Jul-2006 (Ralph Loen) SIR 116326
**          In SQLToIngresApi(). in SQLRELEASE case, don't disconnect
**          if the session control block has no connection handle.
**     16-Aug-2006 (Ralph Loen) SIR 116477 (second phase)
**          In odbc_getInput(), add support for ISO dates, times and
**          intervals.
**     28-Sep-2006 (Ralph Loen) SIR 116477
**          Adjusted treatment of ISO date/times according to new API/ADF rules.
**     03-Oct-2006 (Ralph Loen) SIR 116477
**          Dynamic parameters for dates and timestamps are now sent as binary
**          values for backward-compatibility with ingresdate.
**     10-oct-2006 (Fei Ge) Bug 116827
**          In odbc_setDescr(), don't do SpecialApiDataType() if
**          data redturned from non-Ingres server is SQL_TYPE_TIMESTAMP.
**     17-Oct-2006 (Ralph Loen) SIR 116477
**          In SpecialApiDataType(), removed invalid II_UINT2 casts from
**          pipd->OctetLength.
**     10-Nov-2006 (Ralph Loen) SIR 116477
**          In SpecialApiDataTypes(), revise treatment of binary length
**          for ISO dates/times/intervals.
**     22-Nov-2006 (Ralph Loen) SIR 116477
**          In odbc_putSegment(), set the parameter length of time-
**          only if the API connection level is less than IIAPI_LEVEL_4.	
**     21-Aug-2007 (Ralph Loen) Bug 118993
**          In odbc_getQInfo(), accumulate SQLNRP (number of rows processed)
**          from the returned row count, and don't set SQLNRP to zero if
**          no rows were found.  Remove unused odbc_def_repeat_query() and
**          odbc_exec_repeat_query().
**     20-Sep-2007 (weife01) SIR 119140
**          Added support for new DSN/connection string configuration
**          option:"Convert int8 from DBMS to int4".
**     04-Dec-2007 (Ralph Loen) Bug 119565 
**          In odbc_query_sm(), change state to QRY_CANCEL instead of
**          QRY_CLOSE if an error occurs in an intermediate stage of a
**          query.
**     06-Dec-2007 (Ralph Loen) Bug 119565 
**          Code cleanup: In odbc_describe(), don't invoke 
**          IIapi_getDescriptor() if the previous call to IIapi_query() failed.
**     09-Jan-2008 (Ralph Loen) SIR 119723
**          Add new odbc_scroll() routine to support static scrollable
**          cursors.  Modify qy_flag in IIAPIQUERY structure to allow for
**          new API interface.
**     23-jan-2008 (Ralph Loen) Bug 119800
**          Resurrect support for updatable forward-only cursors.
**     09-Jan-2008 (Ralph Loen) SIR 119723
**          Add new odbc_setPos() to support SQLSetPos().  Add new
**          sequence QST_SETPOS.
**     09-Jan-2008 (Ralph Loen) SIR 119723
**          In odbc_scrollColumns(), account for updatable cursors.  Since
**          IIapi_position() returns only one row, invoke IIapi_position()
**          with an reference of IIAPI_POS_CURRENT and an offset of 1 for
**          all rows after the first fetch.
**     07-Feb-2008 (Ralph Loen) SIR 119723
**          Enable support for prepared statements with scrollable cursors.
**     05-May-2008 (Ralph Loen) Bug 120339
**          In odbc_query_sm(), queue DBMS messages on the 
**          error stack in the QRY_CLOSE state.
**     12-May-2008 (Ralph Loen) Bug 120356
**          In SqlToIngresAPI(), add OPT_DEFAULTTOCHAR as a new DSN config
**          option to treat SQL_C_DEFAULT as SQL_C_CHAR for SQLWCHAR types
**     31-Jul-2008 (Ralph Loen) Bug 120718
**          In odbc_query_sm(), don't invoke SQLCA_ERROR if a conversion
**          error has been set for the QRY_CLOSE state.  Dump obsolete
**          #ifdef IIAPI_VERSION_2 macro.
**     15-Aug-2008 (Ralph Loen) Bug 120777
**          Add support for qy_flags argument in IIAPI_QUERYPARM. 
**     18-Aug-2008 (Ralph Loen) Bug 120790
**          In odbc_getInput(), revised treatment of ReallocDescriptor()
**          for the IPD to reflect the IRD.        
**     22-Aug-2008 (Ralph Loen) Bug 120814
**          In odbc_getSegment() and odbc, getColumnsLong(), get the total
**          blob length via IIapi_getColumnInfo() and store it in
**          the new "blobLen" field of the IRD.  This is done only for
**          the first segmented fetch.
**     06-Oct-2008 (Ralph Loen) Bug 121019
**          Removed obsolete ExecProcSetDescriptor() and ExecProcPutParms().
**          In ScanProcParmsAndBuildDescriptor(), check for STMT_EXECPROC
**          flag before skipping over the output parameter.   
**     28-Oct-2008 (Ralph Loen) Bug 121149
**         Change on 31-Jul-2008 caused a regression on bug 108044 in
**         a section of code containing the #ifdef IIAPI_VERSION_2 macro.
**         Restored code that explicitly sends IIapi_setConnectParam()
**         with a period if the decimal char is II_DECIMAL is not
**         supported.
**     03-Nov-2008 (Ralph Loen) SIR 121152
**         Added support for II_DECIMAL in a connection string.  
**         The OPT_DECIMALPT bit now indicates that support for
**         II_DECIMAL has been specified in a DSN or connection string.
**         The value of II_DECIMAL is now stored in the connection
**         handle, and is assumed to be a comma or a period.
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Add support for OPT_INGRESDATE configuration parameter.  If set,
**         ODBC date escape syntax is converted to ingresdate syntax, and
**         parameters converted to IIAPI_DATE_TYPE are converted instead to 
**         IIAPI_DTE_TYPE.
**     19-Nov-2008 (Ralph Loen) SIR 121228
**         In some cases, dateConnLevel was being examined for data
**         conversion where fAPILevel was more appropriate, such as for
**         ANSI dates and ANSI intervals.  At present, the intent is
**         to confine OPT_INGRESDATE support only to dates.
**     27-Dec-2008 (Ralph Loen) Bug 121406
**         In odbc_getInput(), reset the IRD after it has been copied to
**         the IPD, since the IRD is no longer required.
**     05-Jan-2009 (Ralph Loen) Bug 121358
**         Added odbc_getUnboundColumnsLong(), QST_FETCHUNBOUND, and
**         SQL_FETCHUNBOUND to handle case in which SQLFetch() is invoked
**         with remaining segments to be sent from the DBMS.
**         Added "ignoreBlob" boolean argument to GetAllSegments().  If
**         TRUE, "ignoreBlob" fetches remaining blob segments without
**         allocating additional memory.   Allowed for case in GetAllSegments()
**         in which the blob column is bound but segments remain to be fetched. 
**     07-Jan-2009 (Ralph Loen) Bug 121406
**         Reworked the fix so that invocation of SQLDescribeParam()
**         never fills the IRD.  Odbc_getDescr() includes a boolean
**         argument to inticate that it has been called by 
**         odbc_describe_input().   If this is the case, odbc_getDescr()
**         fills the IPD rather than the IRD.  Odbc_getInput(), which
**         copied the IRD to the IPD, is no longer required and has been
**         removed.
**    23-Jan-2009 (Ralph Loen) Bug 121358
**         In the case of a blob bound with insufficient length, copy into
**         the application buffer (pard->DataPtr) according to the specified
**         length, rather than the number of bytes fetched.
**    10-Jan-2009 (Ralph Loen) Bug 121639
**          In odbc_scrollColumns(), account for cases in which the cursor
**          has reach EOD with a row size less than the ArraySize in the
**          ARD.
**     28-Apr-2009 (Ralph Loen) SIR 122007
**         Add support for "StringTruncation" connection string attribute.
**         If set to Y, string truncation errors are reported.  If set to
**         N (default), strings are truncated silently.
**     18-Sep-2009 (Ralph Loen, Dave Thole) Bug 122603
**         With the increase of the MAX_COLUMNS value,
**         use int4 indexes into column descriptors instead of int2.
**    22-Sep-2009 (hanje04) 
**	    Define null terminator, nt to pass to CMcpchar() instead of
**	    literal "\0" because it causes relocation errors when linking
**	    64bit intel OSX
**    20-Nov-2009 (Ralph Loen)
**          For the SQLPREPARE case in SqlToIngresApi(), do not execute
**          odbc_query_sm() with the QST_DESCRIBE query type.  Instead,
**          add syntax "into sqlda" when a select query is prepared in
**          odbc_prepare(), and fetch the tuple descriptor.  The 
**          QST_DESCRIBE query type is now obsolete.  This avoids
**          sending an extra describe statement to the DBMS.
**          Added a new function named odbc_print_qry() to trace the ODBC 
**          query state machine for debugging.
**    03-Dec-2009 (Ralph Loen) Bug 123001
**          In odbc_setDescr(), set SQLCODE in the sqlca error block if
**          PrepareParms() is unsuccessful.  Otherwise, errors will not be
**          propagated to the application when SQLCA_ERROR is invoked.
**    23-Mar-2010 (Ralph Loen) SIR 123465
**          Trace data send to database to in odbc_putParms() with new 
**          odbc_hexdump() function.
**    29-Apr-2010 (Ralph Loen) SIR 123661
**          In ScanProcParamsAndBuildDescriptor(), make the call to
**          GetProcParamNames() contingent on IIAPI_LEVEL_6, since
**          databases at this level don't require the parameters to be
**          named.
**    14-Jul-1010 (Ralph Loen)  Bug 124079
**          In ScanProcParamNamesAndBuildDescriptor(), don't skip
**          GetProcParamNames() if the target db is Vectorwise.  In
**          GetServerInfo(), set the release version to the same as Ingres 
**          10.0 for Vectorwise servers, and disable OPT_INGRESDATE and
**          OPT_BLANKDATEisNULL for Vectorwise servers.`
**    17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs or NBR type as BYTE.
**    12-Oct-2010 (Ralph Loen) Bug 124581
**          Removal of QST_DESCRIBE resulted in a memory leak for
**          prepared select queries using select loops.
**          In the QRY_SETD case of odbc_query_sm(), invoke 
**          PrepareSqldaAndBuffer() with the prepare_or_describe argument set 
**          to SQLDESCRIB if the select query is prepared: in this case, only 
**          the sqlda needs to be allocated, not the fetch buffer.
*/

/*
** Sequences for ODBC state machine
*/

#define QRY_QUERY       1       /* Send query */
#define QRY_SETD        2       /* Send parameter descriptor */
#define QRY_PUTP        3       /* Send parameters */
#define QRY_GETD        4       /* Receive result descriptor */
#define QRY_GETQI       5       /* Get query Info */
#define QRY_DONE        6       /* Finalize query processing */
                                /* Optionally get tuples  */

#define QRY_CANCEL      96      /* Cancel query */
#define QRY_CLOSE       97      /* Close query */
#define QRY_EXIT        98      /* Respond to client */

/* 
** Sequence types for ODBC state machine 
*/
#define QST_GENERIC      0      /* No special processing */
#define QST_QUERY        1      /* Send a simple query */
#define QST_EXECUTE      2      /* Perform "execute" query */
#define QST_PREPARE      3      /* Prepare a statement */
#define QST_FETCH        4      /* Fetch rows */
#define QST_PROC         5      /* Call a DB procedure */
#define QST_PUTSEG       6      /* Put a segment */
#define QST_GETSEG       7      /* Get a segment */
#define QST_DESCRIBE_INPUT 8    /* Describe parameters */
#define QST_GETUNBOUND   9      /* Get unbound columns */
#define QST_SETPOS       10     /* Scroll cursor for update */

#define SEGMENT_SIZE     2008
#define LEN_GATEWAY_OPTION 255
#define putParmsSEGMENTSIZE 65280       /* 0xFF00 = 64K minus a little slack */
#define SQL_PARAM_TYPE_PROCGTTPARM   15 /* dummy parameter type to describe GTT parm */
#define SQL_PARAM_TYPE_PROCNAME      16 /* dummy parameter type to describe GTT parm */

static BOOL ProcessIngSet(SESS*, LPSQLCA, SQLUINTEGER, CHAR     *,LPDBC); 
static i4 ProcessII_DATE_FORMAT(void);
static i4 ProcessII_DATE_CENTURY_BOUNDARY(void);
static CHAR *ProcessII_DECIMAL(void);
static BOOL ProcessIncludeFile(SESS*, CHAR    *,LPSQLCA,SQLUINTEGER,LPDBC);
static BOOL isCursorReadOnly(LPSTMT pstmt);
static void GetCapabilitiesFromDSN(char * pDSN, LPDBC pdbc);
static BOOL GetProcParamNames(SESS * pSession, LPSTMT pstmt, LPDESC pLitIPD,
                                 char *szProcName, char *szSchema);

RETCODE odbc_AutoCommit(LPSQB pSqb, BOOL bFlag);
RETCODE odbc_query_sm( QRY_CB *qry_cb );
RETCODE exitPutSeg(RETCODE, IIAPI_PUTPARMPARM *, IIAPI_PUTPARMPARM *);
BOOL    stripOffQuotes( CHAR *);
void odbc_print_qry( i2, i2 );

static const char nt[] = "\0"; 
#define MAGIC_1980  1980
#define MAX_LIT_PARAMS 128  /* max number of literal procedure parms */
#define FOUR_K    1024*4

typedef struct
{
    i2 value;
    char str[30];
} QRY_SEQ_TABLE;

QRY_SEQ_TABLE qry_state[] =
{
    QRY_QUERY, "QRY_QUERY",
    QRY_SETD,   "QRY_SETD",
    QRY_PUTP,   "QRY_PUTP",
    QRY_GETD,   "QRY_GETD",
    QRY_GETQI,  "QRY_GETQI",
    QRY_DONE,   "QRY_DONE",
    QRY_CANCEL, "QRY_CANCEL",
    QRY_CLOSE, "QRY_CLOSE",
    QRY_EXIT,  "QRY_EXIT",
    -1,        "UNKNOWN"
};

#define QRY_SEQ_TABLE_SIZE sizeof(qry_state) / sizeof(qry_state[0])

QRY_SEQ_TABLE qry_type[] = 
{
    QST_GENERIC, "QST_GENERIC",
    QST_QUERY, "QST_QUERY",
    QST_EXECUTE, "QST_EXECUTE",
    QST_PREPARE, "QST_PREPARE",
    QST_FETCH, "QST_FETCH",
    QST_PROC, "QST_PROC",
    QST_PUTSEG, "QST_PUTSEG",
    QST_GETSEG, "QST_GETSEG",
    QST_DESCRIBE_INPUT, "QST_DESCRIBE_INPUT",
    QST_GETUNBOUND, "QST_GETUNBOUND",
    QST_SETPOS, "QST_SETPOS",
    -1,        "UNKNOWN"
};

#define QRY_SEQ_TYPE_TABLE_SIZE sizeof(qry_type) / sizeof(qry_type[0])

static  SQLUINTEGER sqlulength0 = 0;  /* a convenient const ulen of zero */

/*------------------------------------------------------------------*/
#ifndef DEBUG_MALLOC
VOID * odbc_malloc( SIZE_TYPE size )
{
    VOID * 	    retValue;
    char        s[80];
    STATUS      status;
    
    if (size==0)   /* if size=0, don't worry about it */
       return(0);

    retValue = (VOID *)MEreqmem((u_i2)0,size,TRUE,&status);
    if (!retValue)
    {    STprintf(s, 
                  "odbc_malloc - Insufficient memory to allocate %d bytes",
                  (int)size);
#ifdef WINDOWS
         MessageBox(NULL, s, ERget(F_OD0110_OPING_DRIVER_ERROR), MB_ICONSTOP | MB_OK);
#endif
         return(0);
    } 
    return(retValue );
}
/*------------------------------------------------------------------*/
II_VOID odbc_free( VOID *	buffer )
{
    STATUS      status;

    status = MEfree( (PTR)buffer );
    return;
}
#endif  /* endif DEBUG_MALLOC */
/*------------------------------------------------------------------*/
static char* odbc_stalloc( char	*src )
{
    char	    *dest;

    dest = odbc_malloc( STlength (src) + 1 );
    STcopy( src, dest );
    return	( dest );
}
/*------------------------------------------------------------------*/
static void SpecialApiDataType(IIAPI_DESCRIPTOR *psd, SWORD fCType, BOOL isIngres,
     UDWORD fAPILevel, LPDESCFLD pipd)
{   
    if (isIngres && psd->ds_dataType != IIAPI_VCH_TYPE)
    {
        switch (psd->ds_dataType)
        {
        case IIAPI_DTE_TYPE:
            pipd->OctetLength = ODBC_DTE_OUTLENGTH+1;
            psd->ds_length = (II_UINT2)pipd->OctetLength;
            psd->ds_dataType = IIAPI_CHA_TYPE;
            break;

        case IIAPI_TMWO_TYPE:
        case IIAPI_TIME_TYPE:
        case IIAPI_TMTZ_TYPE:
            pipd->OctetLength = ODBC_TIME_LEN;
            psd->ds_length = (II_UINT2)pipd->OctetLength;
            psd->ds_dataType = IIAPI_TMWO_TYPE;
            break;

        case IIAPI_TSWO_TYPE:
        case IIAPI_TS_TYPE:
        case IIAPI_TSTZ_TYPE:
            pipd->OctetLength = ODBC_TS_LEN;
            psd->ds_length = (II_UINT2)pipd->OctetLength;
            psd->ds_dataType = IIAPI_TSWO_TYPE;
            break;

        case IIAPI_INTYM_TYPE:
            pipd->OctetLength = ODBC_INTYM_LEN;
            psd->ds_length = (II_UINT2)pipd->OctetLength;
            psd->ds_dataType = IIAPI_INTYM_TYPE;
            break;        

        case IIAPI_INTDS_TYPE:
            pipd->OctetLength = ODBC_INTDS_LEN;
            psd->ds_length = (II_UINT2)pipd->OctetLength;
            psd->ds_dataType = IIAPI_INTDS_TYPE;
            break;

        case IIAPI_DATE_TYPE:
            pipd->OctetLength = ODBC_DATE_LEN;
            psd->ds_length = (II_UINT2)pipd->OctetLength;
            psd->ds_dataType = IIAPI_DATE_TYPE;
            break;
    
        default:
            break;        
        } /* switch (psd->ds_dataType) */
     }  /* if (isIngres psd->ds_dataType != IIAPI_VCH_TYPE)) */
    else if (fCType != SQL_C_TYPE_TIMESTAMP && 
        psd->ds_dataType == IIAPI_DTE_TYPE)
        psd->ds_dataType = IIAPI_CHA_TYPE;
    else if (psd->ds_dataType == IIAPI_VCH_TYPE)
    {
        if ((fCType == SQL_C_TYPE_DATE) ||
            (fCType == SQL_C_TYPE_TIME) ||
            (fCType == SQL_C_TYPE_TIMESTAMP))
            psd->ds_dataType = IIAPI_CHA_TYPE;
    }        
    return;
}


/*------------------------------------------------------------------*/
static II_VOID odbc_printErrors( VOID * handle, LPSQLCA psqlca )
{
IIAPI_GETEINFOPARM  * gete;
LPDBC                 pdbc;   /* owner dbc of the SQLCA */

    gete = (VOID *) MEreqmem(0, sizeof(IIAPI_GETEINFOPARM), TRUE, NULL);
    if (!gete) 
        return; 
   
    gete->ge_errorHandle = (VOID *)handle;
   
    for(;;)
    { 
        IIapi_getErrorInfo(gete);
        if (gete->ge_status != IIAPI_ST_SUCCESS )
             break;

        if (!psqlca)
            continue;   /* continue iterating the loop if errors are ignored */

        pdbc = psqlca->pdbc;  /* owner dbc of the SQLCA */

        switch( gete->ge_type )
        {
			case IIAPI_GE_ERROR:	
			case IIAPI_GE_MESSAGE:
				if (gete->ge_type == IIAPI_GE_ERROR)
				{
					psqlca->SQLCODE = SQL_ERROR;
					psqlca->SQLERC = gete->ge_errorCode;
					if (gete->ge_errorCode == E_AP0001_CONNECTION_ABORTED)
					   {strcpy(gete->ge_SQLSTATE,"08S01"); 
					     /* connection pooling may look for "comm link failure"*/
					    if (pdbc)
					        pdbc->fStatus |= DBC_DEAD;  /* SQL_ATTR_CONNECTION_DEAD */
					   }
				    MEcopy(gete->ge_SQLSTATE, sizeof(psqlca->SQLSTATE), psqlca->SQLSTATE);
				}
				else
				{
					if (psqlca->SQLCODE != SQL_ERROR)
					{
						psqlca->SQLCODE = SQL_SUCCESS_WITH_INFO;
						psqlca->SQLERC = gete->ge_errorCode;
						MEcopy(gete->ge_SQLSTATE, sizeof(psqlca->SQLSTATE), psqlca->SQLSTATE);
					}
				}
				InsertSqlcaMsg(psqlca,gete->ge_message,gete->ge_SQLSTATE,FALSE);
                break;

			case IIAPI_GE_WARNING : 
                break;
           default:
                break;
        }

    };  /* end for loop */
    
    MEfree((PTR) gete);
    return;
}

/*
** Name: odbc_getResult - get result of the last API function  
**
** Description:
**      This function prints out the result of the last API function.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
BOOL odbc_getResult( IIAPI_GENPARM  *genParm, LPSQLCA psqlca, SQLUINTEGER cQueryTimeout)
{
    IIAPI_WAITPARM waitParm = { -1 };
	
	if (cQueryTimeout > 0)
		waitParm.wt_timeout = (i4)(cQueryTimeout * 1000);
	
    while( genParm->gp_completed == FALSE )
    {
        IIapi_wait( &waitParm );
        if ( waitParm.wt_status != IIAPI_ST_SUCCESS )
        {
            genParm->gp_status = waitParm.wt_status;
            break;
        }
    }
    
    if (genParm->gp_errorHandle)
        odbc_printErrors( genParm->gp_errorHandle, psqlca );
    
    /*
    ** Return TRUE if succeeded.
    */
    if (
         genParm->gp_status == IIAPI_ST_SUCCESS ||
         genParm->gp_status == IIAPI_ST_WARNING ||
         genParm->gp_status == IIAPI_ST_MESSAGE ||
         genParm->gp_status == IIAPI_ST_NO_DATA
       )
        return( TRUE );

    if (psqlca)
        psqlca->SQLCODE = SQL_ERROR;

    if (psqlca  &&  psqlca->SQLMCT==0)  /* if catastrophe and no error messages */
        {psqlca->SQLERC = genParm->gp_status;
         if      (genParm->gp_status == IIAPI_ST_INVALID_HANDLE)
              InsertSqlcaMsg(psqlca,"Ingres API invalid handle error","HY000",TRUE);
         else if (genParm->gp_status == IIAPI_ST_ERROR)
              InsertSqlcaMsg(psqlca,"Ingres API error probably to due to\
 transaction already abnormally terminated","HY000",TRUE);
         else if (genParm->gp_status == IIAPI_ST_FAILURE)
              InsertSqlcaMsg(psqlca,"Ingres API failure probably to due to\
 transaction already abnormally terminated","HY000",TRUE);
         else if (genParm->gp_status == IIAPI_ST_NOT_INITIALIZED)
              InsertSqlcaMsg(psqlca,"Ingres API not initialized error","HY000",TRUE);
         else if (genParm->gp_status == IIAPI_ST_OUT_OF_MEMORY)
              InsertSqlcaMsg(psqlca,"Ingres API out of memory error","HY000",TRUE);
         else
              InsertSqlcaMsg(psqlca,"Ingres API error of unknown nature","HY000",TRUE);
        }

   return( FALSE );
}
/*
** Name: odbc_close  
**
** Description:
**      This function ends a SQL statement 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
BOOL odbc_close( LPSTMT pstmt )
{
    IIAPI_CLOSEPARM     closeParm;
    BOOL             rc = TRUE;

    if ((pstmt->stmtHandle) == NULL)
         return(TRUE);

    TRACE_INTL(pstmt,"odbc_close");
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = pstmt->stmtHandle;

    IIapi_close( &closeParm );

    if (!odbc_getResult( &closeParm.cl_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
        rc = FALSE;

    pstmt->stmtHandle = NULL;

    return(rc);
}

/*
** Name: odbc_cancel  
**
** Description:
**      Cancel an active SQL statement 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
II_VOID odbc_cancel( LPSTMT pstmt )
{
IIAPI_CANCELPARM		*cancelParm;	  
IIAPI_WAITPARM waitParm = { -1 };

	if ((pstmt->stmtHandle) == NULL)
		return; 

    TRACE_INTL(pstmt,"odbc_cancel");
	cancelParm = (IIAPI_CANCELPARM *)odbc_malloc(sizeof(IIAPI_CANCELPARM));
    cancelParm->cn_genParm.gp_callback = NULL;
    cancelParm->cn_genParm.gp_closure = NULL;
	cancelParm->cn_genParm.gp_completed = FALSE;
    cancelParm->cn_stmtHandle = pstmt->stmtHandle;

    IIapi_cancel( cancelParm );

	if (pstmt->cQueryTimeout > 0)
		waitParm.wt_timeout = (i4)(pstmt->cQueryTimeout * 1000);

    while( cancelParm->cn_genParm.gp_completed == FALSE )
    {
        IIapi_wait( &waitParm );
        if ( waitParm.wt_status != IIAPI_ST_SUCCESS ) 
        {
            break;
        }
    }
	
    odbc_free( cancelParm );
    return; 
}
/*
** Name: GetCursorSelectStmtHandle 
**
** Description:
**      loop thru stmt chains to locate the "select for update" stmt
**      handle associated with the "update ... where current of" stmt  
**
*/
static void GetCursorSelectStmtHandle(LPSTMT pstmt)
{
	LPSTMT pNextStmt = pstmt->pdbcOwner->pstmtFirst; 

        TRACE_INTL(pstmt,"GetCursorSelectStmtHandle");
	pstmt->QueryHandle = NULL;
	while (pNextStmt)
	{
		if ( (pNextStmt->fCommand == CMD_SELECT) &&
			 (!STbcompare(pNextStmt->szCursor, (i4)STlength(pNextStmt->szCursor),
			              pstmt->szCursor, (i4)STlength(pstmt->szCursor), TRUE)) )
		{
			 pstmt->QueryHandle = pNextStmt->stmtHandle;
			 break;
		}
		pNextStmt = pNextStmt->pstmtNext;
	}
	return; 
}
/*
** Name: GetProcName 
**
** Description:
**      Get procedure name from pstmt->szSqlStr
**
*/
static void GetProcName(LPSTMT pstmt, char * pProcName, char * pSchemaName)
{
	CHAR  *p;
	CHAR  *p1;
	LPDBC  pdbc = pstmt->pdbcOwner;
	CHAR   wkname[2*OBJECT_NAME_SIZE+2]=""; /* wkarea for "schemaname.procname" */

        TRACE_INTL(pstmt,"GetProcName");
	*pProcName = *pSchemaName = EOS;

    /*  Get rid of leading spaces */
    p = pstmt->szSqlStr;
    while ( CMspace(p) ) CMnext(p);

	p1  = STindex(p, "(", 0);  /* find start of parm list */
	
	if (!p1)   /* if no parm list, find 1st ' ' after schema.procname */
		p1 = STindex(p, " ", 0); 

	if (p1)
	{
		SIZE_TYPE len = p1-p;               /* length of schemaname.procname */

		if (len > sizeof(wkname)-1)         /* safety check not to overflow */
		    len = sizeof(wkname)-1;
		MEcopy (p, len, wkname);            /* copy to name work area */
		STtrmwhite(wkname);                 /* trim any spaces after name */
	}

	/* separate SchemaName (if present) from ProcName */
	p  = STindex(wkname, ".", 0);  /* find '.' between schemaname and procname */
	if (p)
		{
		    p1 = p;                /* p1->'.' (the end of schemaname) */
		    CMnext(p);             /* p ->start of procname */
		    CMcpychar(nt,p1);    /* null term the schemaname) */
		    STlcopy (wkname, pSchemaName, OBJECT_NAME_SIZE);
		    STlcopy (p,      pProcName,   OBJECT_NAME_SIZE);
		}
	else
		STlcopy (wkname, pProcName, OBJECT_NAME_SIZE); /* no schema name */

	CatToDB_NAME_CASE(pdbc, pSchemaName); /* convert name to right case */
	CatToDB_NAME_CASE(pdbc, pProcName);   /* convert name to right case */
	return;
}


/*
**  ConvertChar 
*/
static BOOL ConvertChar(CHAR     * pSource, VOID * pTarget, 
                           LPDESCFLD pipd, IIAPI_DT_ID apiDataType)
{
	IIAPI_CONVERTPARM   cv;

	cv.cv_srcDesc.ds_dataType   = IIAPI_CHA_TYPE; 
	cv.cv_srcDesc.ds_nullable   = FALSE;  
	cv.cv_srcDesc.ds_length     = (II_UINT2) pipd->OctetLength;
	cv.cv_srcDesc.ds_precision  = 0;
	cv.cv_srcDesc.ds_scale      = 0;
	cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_srcDesc.ds_columnName = NULL;  
  
	cv.cv_srcValue.dv_null      = FALSE; 
	cv.cv_srcValue.dv_length    = (II_UINT2) pipd->OctetLength;
	cv.cv_srcValue.dv_value     = pSource; 
      
	cv.cv_dstDesc.ds_dataType   = apiDataType; 
	cv.cv_dstDesc.ds_nullable   = FALSE;
	cv.cv_dstDesc.ds_length     = (II_UINT2) pipd->OctetLength;
	cv.cv_dstDesc.ds_precision  = (II_UINT2) pipd->Precision;
	cv.cv_dstDesc.ds_scale      = (II_UINT2) pipd->Scale;
	cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_dstDesc.ds_columnName = NULL;

	cv.cv_dstValue.dv_null      = FALSE;
	cv.cv_dstValue.dv_length    = (II_UINT2) pipd->OctetLength;
    cv.cv_dstValue.dv_value     = pTarget; 
    IIapi_convertData(&cv);

	if (cv.cv_status != IIAPI_ST_SUCCESS)
	{
		return(FALSE);
	}
	return(TRUE); 
}
/*
** Name: PutProcLiteralParm 
**
*/
static BOOL PutProcLiteralParm(LPDESCFLD pipd,
                   CHAR * ParameterName, IIAPI_DT_ID apiDataType, 
                   CHAR * pstart, CHAR * pend)

{
	SDWORD   intValue;
	UDWORD   rc;
	UWORD    cbData;
	UWORD    i;

	cbData = (UWORD)(pend - pstart);

	pipd->fIngApiType = apiDataType;
	pipd->Nullable    = FALSE;
	memcpy(pipd->Name, ParameterName, sizeof(pipd->Name));

	switch(apiDataType)
	{
	case IIAPI_INT_TYPE:
		pipd->OctetLength = sizeof(intValue);
		rc = CvtCharInt(&intValue, pstart, cbData); 
		if (rc != CVT_SUCCESS)
			return(FALSE);
		pipd->DataPtr = odbc_malloc((size_t)(pipd->OctetLength));
		pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr is allocated */
		* (SDWORD*) (pipd->DataPtr) = intValue;
		break;

	case IIAPI_DEC_TYPE:
		pipd->OctetLength =  cbData;
		pipd->Precision   =  cbData - 1;

		for (i=0; i<cbData; i++)
		{
		    if (pstart[i] == '.') 
		        break;
		}
		pipd->Scale   =  (SQLSMALLINT)(pipd->Precision - i);

		pipd->DataPtr = odbc_malloc((size_t)(pipd->OctetLength));
		pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr is allocated */
		if (!ConvertChar(pstart, pipd->DataPtr, pipd, apiDataType))
			return(FALSE);   /* convert from CHAR to DECIMAL */
		break;
	
	case IIAPI_FLT_TYPE:
		pipd->OctetLength = sizeof(double);
		pipd->DataPtr = odbc_malloc((size_t)(pipd->OctetLength));
		pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr is allocated */
		rc = CvtCharFloat((SDOUBLE*)pipd->DataPtr, pstart, cbData);
		if (rc != CVT_SUCCESS)
			return(FALSE);
		break;

	case IIAPI_BYTE_TYPE:
	case IIAPI_NBR_TYPE:
		cbData = cbData / 2;
		pipd->OctetLength = cbData; 
		pipd->DataPtr = odbc_malloc((size_t)(pipd->OctetLength));
		pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr is allocated */
		rc = CvtCharBin(pipd->DataPtr, pstart, cbData);
		if (rc != CVT_SUCCESS)
			return(FALSE);
		break;

	case IIAPI_VCH_TYPE:
# ifdef IIAPI_NVCH_TYPE
	case IIAPI_NVCH_TYPE:
# endif
		pipd->OctetLength = cbData + 2; 
		pipd->DataPtr = odbc_malloc((size_t)(pipd->OctetLength));
		pipd->fStatus |= COL_DATAPTR_ALLOCED;  /* DataPtr is allocated */
		memcpy(pipd->DataPtr+2, pstart, cbData);
		*((II_INT2*)(pipd->DataPtr)) = cbData;
		break;

	default:
		pipd->OctetLength = cbData; 
		pipd->DataPtr = pstart;
		break;
	}
	return(TRUE);
}

/*
** Name: ScanProcParmsAndBuildDescriptor
**
** Description:
**      Build a new APD and IPD based on procedure parameters
**
** On exit:
**
*/
static
BOOL ScanProcParmsAndBuildDescriptor(
    SESS      *pSession,
    LPSTMT     pstmt,
    LPDESC     pProcAPD,
    LPDESC     pProcIPD,
    char      *szProcName,
    char      *szSchema)
{
    CHAR     * p;
    CHAR     * pstart;
    CHAR     * pend;
    BOOL    fFoundParam  = FALSE;
    BOOL    fFoundLit    = FALSE;
    BOOL    fFoundSession= FALSE;
    BOOL    fBinary      = FALSE;
    BOOL    rc;
    LPDESCFLD  papd         = pstmt->pAPD->pcol + 1;
    LPDESCFLD  pipd         = pstmt->pIPD->pcol + 1;
    LPDESCFLD  pprocapd;
    LPDESCFLD  pprocipd;
    BOOL    isIngres =  isServerClassAnIngresEngine(pstmt->pdbcOwner)?TRUE:FALSE;
    II_INT2    apiDataType;
    char       ParameterName[OBJECT_NAME_SIZE+1] = "";
    SIZE_TYPE  len;
    char       msgOnlyOneParmAllowed[] = "Only one parameter allowed";
    LPDBC      pdbc = pstmt->pdbcOwner;

    /*
    ** Skip over first output parameter if a procedure return value.
    */
    if ((pipd->ParameterType == SQL_PARAM_OUTPUT))
    {
        if ((pstmt->fStatus & STMT_RETPROC))
        {
            pstmt->icolParmLast--;
            papd++;
            pipd++;
        }
    }

    TRACE_INTL(pstmt,"ScanProcParmsAndBuildDescriptor");
    p = pstmt->szSqlStr;  /* p -> call statement and its parms */

    p = STindex (p, "(", STlength(p) );
                          /* locate first left parenthesis */
    if (p)
        CMnext(p);   /* skip of the '(' */
    else  /* missing parens format in {call procname} */
       {  /* it's OK and legal, just add it */
        STcat(pstmt->szSqlStr, "()");
        return(TRUE);
       }

    while (p && (*p != '\0') )  /* loop through parameters */
    {
     p = ScanPastSpaces(p);

     if ((*p == 'x'  ||  *p == 'X')  &&   /* if X'binary string' */
         (*(p+1) == '\''  ||  *(p+1) == '\"'))
        {p++;                   /* skip past 'x' or 'X' */
         fBinary = TRUE;
         /* fall through to quoted string */
        }

     if (*p == '\''  ||  *p == '\"')  /* quoted string or X'binary string' */
        {
         pstart = p + 1;
         p = ScanQuotedString(p);
         pend = p - 1;
         fFoundLit = TRUE;
         apiDataType = fBinary ? IIAPI_BYTE_TYPE : IIAPI_CHA_TYPE;
         fBinary = FALSE;
         continue;          /* iterate the loop through parameters */
        }

     if (CMdigit(p)  ||  *p == '-'  ||  *p == '+')  /* number */
        {
         pstart = p;
         p = ScanNumber(p,&apiDataType);  /* scan integer, decimal, or float*/
         pend = p;
         fFoundLit = TRUE;
         continue;          /* iterate the loop through parameters */
        }

     if (*p == '~'  &&  *(p+1) == 'V')  /* parameter marker ('?') translated */
        {
         p += 2;
         fFoundParam = TRUE;
         continue;          /* iterate the loop through parameters */
        }

     if (CMnmstart(p))  /* parmname = session.GTTtablename*/
        {
         pstart = p;
         p = ScanRegularIdentifier(p);  /* scan parameter name */
         len = p - pstart;              /* length of parameter name */
         if (len > (sizeof(ParameterName) - 1))
             goto errexit;
         memcpy(ParameterName, pstart, len);  /* save "parmname" */
         ParameterName[len] = '\0';
         p = ScanPastSpaces(p);

         if (*p != '=')                 /* parmname =  */
             goto errexit;
         p = ScanPastSpaces(p+1);

         if (*p != 's'  &&  *p != 'S')  /* parmname = session */
             goto errexit;
         pstart = p;
         p = ScanRegularIdentifier(p);  /* scan "session" */
         len = p - pstart;              /* length of "session" keyword */
         if (len != 7  ||  STbcompare(pstart,7,"session",7,TRUE) !=0)
             goto errexit;
         p = ScanPastSpaces(p);

         if (*p != '.')                 /* parmname = session. */
             goto errexit;
         p = ScanPastSpaces(p+1);

         if (!CMnmstart(p))             /* parmname = session.GTTtablename*/
             goto errexit;
         pstart = p;
         p = ScanRegularIdentifier(p);  /* scan parameter value */
         pend = p;
         apiDataType = IIAPI_VCH_TYPE;  /* session.GTT must be varchar */
         fFoundSession = TRUE;
         continue;          /* iterate the loop through parameters */
        }   /* end if scan of "parmname = session.GTTtablename" */

     if (*p != ','  &&  *p != ')')  /* if not a parameter separator */
         goto errexit;              /* error - unknown item to scan */


     /* allocated a new field in the new procedure APD descriptor */
     if (pProcAPD->Count+1 > pProcAPD->CountAllocated)
           /* if need more col, re-allocate */
         if (ReallocDescriptor(pProcAPD, (i2)(pProcAPD->Count+1)) == NULL)
            {ErrState (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
             return FALSE;
            }
     pProcAPD->Count++;
     ResetDescriptorFields(pProcAPD, pProcAPD->Count, pProcAPD->Count);

     /* allocated a new field in the new procedure IPD descriptor */
     if (pProcIPD->Count+1 > pProcIPD->CountAllocated)
           /* if need more col, re-allocate */
         if (ReallocDescriptor(pProcIPD, (i2)(pProcIPD->Count+1)) == NULL)
            {ErrState (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
             return FALSE;
            }
     pProcIPD->Count++;
     ResetDescriptorFields(pProcIPD, pProcIPD->Count, pProcIPD->Count);

     pprocapd = pProcAPD->pcol + pProcAPD->Count;  /* pprocapd->descriptor column */
     pprocipd = pProcIPD->pcol + pProcIPD->Count;  /* pprocipd->descriptor column */

     if (fFoundParam)
        {               /* parameter marker found */
         pprocapd->fStatus |= COL_BOUND;

         pprocapd->ConciseType   = papd->ConciseType;
         pprocapd->VerboseType   = papd->VerboseType;
         pprocapd->OctetLength   = papd->OctetLength;
         pprocapd->OctetLengthPtr= papd->OctetLengthPtr;
         pprocapd->IndicatorPtr  = papd->IndicatorPtr;
         pprocapd->Length        = papd->Length;
         pprocapd->Precision     = papd->Precision;
         pprocapd->Scale         = papd->Scale;
         pprocapd->Length        = papd->Length;
         pprocapd->ParameterType = papd->ParameterType;
         pprocapd->fIngApiType   = papd->fIngApiType;
         pprocapd->DataPtr       = papd->DataPtr;
         pprocapd->Nullable      = papd->Nullable;
         pprocapd->cbDataOffset  = papd->cbDataOffset;
         pprocapd->cbNullOffset  = papd->cbNullOffset;
         pprocapd->cbPartOffset  = papd->cbPartOffset;
         pprocapd->cbDataOffsetAdjustment
                                 = papd->cbDataOffsetAdjustment; 
         pprocipd->ConciseType   = pipd->ConciseType;
         pprocipd->VerboseType   = pipd->VerboseType;
         pprocipd->OctetLength   = pipd->OctetLength;
         pprocipd->OctetLengthPtr= pipd->OctetLengthPtr;
         pprocipd->IndicatorPtr  = pipd->IndicatorPtr;
         pprocipd->Length        = pipd->Length;
         pprocipd->Precision     = pipd->Precision;
         pprocipd->Scale         = pipd->Scale;
         pprocipd->Length        = pipd->Length;
         pprocipd->ParameterType = pipd->ParameterType;
         pprocipd->fIngApiType   = pipd->fIngApiType;
         pprocipd->DataPtr       = pipd->DataPtr;
         pprocipd->Nullable      = pipd->Nullable;
         pprocipd->cbDataOffset  = pipd->cbDataOffset;
         pprocipd->cbNullOffset  = pipd->cbNullOffset;
         pprocipd->cbPartOffset  = pipd->cbPartOffset;
         pprocipd->cbDataOffsetAdjustment
                                 = pipd->cbDataOffsetAdjustment;

         if (papd->OctetLengthPtr  &&  *papd->OctetLengthPtr==SQL_DEFAULT_PARAM)
             pprocipd->ParameterType = SQL_PARAM_TYPE_UNKNOWN;
             /* if default (empty) parameter, keep its slot in the Proc IRD
                to keep the order correct for the procedure parameter names */
         papd++;
         pipd++;
        }  /* end if parameter marker */
     else
     if (fFoundLit  ||  fFoundSession)
        {               /* literal or session name found */
         if (!PutProcLiteralParm(pprocipd, ParameterName, apiDataType, pstart, pend))
             goto errexit;

         pprocapd->BindOffsetPtr = &sqlulength0;
                /* overide possible parm array ptr pAPD->BindOffsetPtr and
                   force offset to 0 to protect proc param literals by
                   disabling array */

         SetDescDefaultsFromType(pdbc, pprocipd);
         if (fFoundSession)
              pprocipd->ParameterType = SQL_PARAM_TYPE_PROCGTTPARM;
         else pprocipd->ParameterType = SQL_PARAM_INPUT; /* treat literal as INPUT */
        }  /* end if literal or session name */
     else   /*  default (empty) parameter found, keep its slot in the Proc IRD
                to keep the order correct for the procedure parameter names.
                odbc_SetDescr and odbc_PutDescr will skip it later */
         pprocipd->ParameterType = SQL_PARAM_TYPE_UNKNOWN;


     if (*p == ')')  /* break if end of parm list */
        break;
     else 
        { p++;       /* skip over ',' separator */
          if (fFoundSession) /* gtt session parm must be only parm */
              goto errexit_oneparm;
        }

     *ParameterName = '\0';   /* clear the slate for another parm */
     fFoundParam = fFoundLit = fFoundSession = FALSE;

     continue;          /* continue onto the next parameter */

    }  /* end while loop through parameters */

    if (fFoundSession)   /* if found GTT session then all done, because */
        return(TRUE);    /* the parameter names is already filled in */


    /*
    ** Ingres 10 and later supports positional DB procedure parameters,
    ** so there is no need to get the names of the parameters.  Vectorwise
    ** currently does not support positional parameters.
    */
    if (pdbc->fAPILevel < IIAPI_LEVEL_6 || isServerClassVECTORWISE(pdbc) )
    {
        rc = GetProcParamNames(pSession, pstmt, pProcIPD, szProcName, szSchema);
                 /* Search and read the catalog for the procedure column names,
                    and fill in the proc IRD with the names since the names 
                    are required by the API. */
    }
    return(rc);

errexit:
    p = ERget(F_OD0111_INVLID_LIT_PARM);
    goto errexitcommon;

errexit_oneparm:
    p = &msgOnlyOneParmAllowed[0];
    goto errexitcommon;

errexitcommon:
    pstmt->sqlca.SQLERC = -1; 
    pstmt->sqlca.SQLCODE = SQL_ERROR;
    InsertSqlcaMsg(&pstmt->sqlca, p, SQST_GENERAL_ERROR, TRUE);
    return(FALSE);
}

/*
** Name: hasLongDataColumn
**
** Description:
**       check to see if long varchar/long byte columns 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL hasLongDataColumn(LPSTMT  pstmt)
{
    LPDESC    pIRD = pstmt->pIRD;   /* Implementation Row Descriptor */
    LPDESCFLD pird;                 /* IRD row field */
    i2        i;
    SQLSMALLINT IRDCount = pIRD->Count;
                                   /* count of DESCFLDs */

    TRACE_INTL(pstmt,"hasLongDataColumn");
    for (i = 0, pird = pIRD->pcol + 1;  /* pird -> 1st IRD (after bookmark) */
         i < IRDCount;  i++, pird++)    /* loop through the result columns */
    {
        if (pird->fIngApiType == IIAPI_LVCH_TYPE   ||
            pird->fIngApiType == IIAPI_GEOM_TYPE   ||
            pird->fIngApiType == IIAPI_POINT_TYPE  ||
            pird->fIngApiType == IIAPI_MPOINT_TYPE ||
            pird->fIngApiType == IIAPI_LINE_TYPE   ||
            pird->fIngApiType == IIAPI_MLINE_TYPE  ||
            pird->fIngApiType == IIAPI_POLY_TYPE   ||
            pird->fIngApiType == IIAPI_MPOLY_TYPE  ||
            pird->fIngApiType == IIAPI_GEOMC_TYPE  ||
# ifdef IIAPI_LNVCH_TYPE
            pird->fIngApiType == IIAPI_LBYTE_TYPE  ||
            pird->fIngApiType == IIAPI_LNVCH_TYPE)
# else
            pird->fIngApiType == IIAPI_LBYTE_TYPE)
# endif
           return(TRUE); 
    }  /* end for loop through the result columns */

    return(FALSE);
}

/*
** Name: odbc_getDescr 
**
** Description:
**       get descriptor information 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
** History:
**
**     07-Jan-2009 (Ralph Loen) Bug 121406
**         Added descInput argument to inticate that this routine has been 
**         called by odbc_describe_input().   If this is the case, 
**         odbc_getDescr() fills the IPD rather than the IRD. 
*/
static BOOL odbc_getDescr( LPSTMT pstmt, BOOL bDescriptionWanted, 
II_INT2 *descrCount, BOOL descInput)
{
    IIAPI_GETDESCRPARM getDescrParm;
    IIAPI_DESCRIPTOR  *ds;
    II_INT2  i;
    LPDESC    pIDesc = descInput ? pstmt->pIPD : pstmt->pIRD;
    LPDESC    pADesc = descInput ? pstmt->pAPD : pstmt->pARD; 
    LPDESCFLD pdescfld;  /* pointer to IRD or IPD */
    LPDBC     pdbc = pstmt->pdbcOwner;
    II_INT2   dsCount;

    TRACE_INTL (pstmt, "odbc_getDescr");

    /*
    ** Provide input parameters for IIapi_getDescriptor().
    */
    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = pstmt->stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;

    /*
    ** Invoke API function call.
    */
    IIapi_getDescriptor( &getDescrParm );
    
    if (!odbc_getResult( &getDescrParm.gd_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
        return(FALSE);
    if (!bDescriptionWanted)     /* return if description not wanted */
        return(TRUE);

    dsCount = getDescrParm.gd_descriptorCount;
    if (descrCount)
        *descrCount = dsCount;

    if (!dsCount)
        return (TRUE);
    /*
    ** Move descriptor info into the Implementation Descriptor
    */
    ResetDescriptorFields(pIDesc, 0, pIDesc->Count);  /* clear desc fields */
    /*
    **  Ensure that IRD row descriptor is large enough.
    **  If too few then re-allocate.
    */
    if (dsCount > pIDesc->CountAllocated)
        if (ReallocDescriptor(pIDesc, dsCount) == NULL)
           { ErrState (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
             return FALSE;
           }
    pIDesc->Count = dsCount;

    if (dsCount > pADesc->CountAllocated)
        if (ReallocDescriptor(pADesc, dsCount) == NULL)
           { ErrState (SQL_HY001, pstmt, F_OD0001_IDS_BIND_ARRAY);
             return FALSE;
           }

    pdescfld = pIDesc->pcol + 1;   
    ds = getDescrParm.gd_descriptor;  /* ds->API's data descriptor */

    for (i=0; i < dsCount;  i++, pdescfld++, ds++)
    {
        II_UINT2  ds_length = ds->ds_length;

# ifdef IIAPI_NCHA_TYPE
        if (ds->ds_dataType == IIAPI_NCHA_TYPE  &&  sizeof(SQLWCHAR)==4)
            ds_length = ds_length*2;
                                  /* adjust UCS2 length to UCS4 length */
       else
        if (ds->ds_dataType == IIAPI_NVCH_TYPE  &&  sizeof(SQLWCHAR)==4)
            ds_length = (ds_length - 2)*2 + 2;
                                  /* adjust UCS2 length to UCS4 length */
# endif
        if (ds->ds_columnName)
            STlcopy (ds->ds_columnName, pdescfld->Name, 
                sizeof(pdescfld->Name)-1);
        else
           {
            STcopy ("col", pdescfld->Name);
            STprintf (pdescfld->Name+3, "%d", (i4)(i+1));
           }
        pdescfld->Nullable    = ds->ds_nullable?SQL_NULLABLE:SQL_NO_NULLS;
        pdescfld->fIngApiType = ds->ds_dataType;
        pdescfld->Length      =  /* data length plus prefix length in bytes */
        pdescfld->OctetLength =     ds_length;
        pdescfld->Precision   = ds->ds_precision;
        pdescfld->Scale       = ds->ds_scale;
        SetDescDefaultsFromType(pdbc, pdescfld);
    }   /* end for loop through descriptors */

    return(TRUE);
}

/*
** Name: odbc_setDescr  
**
** Description:
**      This function sends information to the DBMS server about the 
**      format of data to be provided in subsequent calls to IIapi_putParm. 
**
** On entry: 
**
** On exit:
**
** Returns:
** Change History
** --------------
**
**  date        programmer  description
**  02/06/1997  Jean Teng   use pipd->OctetLength for ds_length to fix SQL_VARCHAR bug.
**    15-oct-2004 (Ralph.Loen@ca.com) Bug 113241
**          Make sure schema qualifiers for database procedures get set up 
**          in IIapi_setDescriptor().
**  10/10/06 Fei Ge Bug 116827
**           In odbc_setDescr(), don't do SpecialApiDataType() if
**           data redturned from non-Ingres server is SQL_TYPE_TIMESTAMP. 
**      
*/
static BOOL odbc_setDescr( LPSTMT pstmt, IIAPI_QUERYTYPE apiQueryType )
{
    IIAPI_SETDESCRPARM setDescrParm = {0};
    IIAPI_DESCRIPTOR  *psd;
    LPDESC    pAPD = pstmt->pAPD;   /* Application Parameter Descriptor */
    LPDESC    pIPD = pstmt->pIPD;   /* Implementation Parameter Descriptor */
    LPDESCFLD papd;                 /* APD parm field */
    LPDESCFLD pipd;                 /* IPD parm field */
    II_INT2   i=0;
    II_INT2   parmCount;
    BOOL   rc = TRUE;
    RETCODE ret = SQL_SUCCESS;
    CHAR  *   rgbData;              /* -->data in parm buffer */
    BOOL   isIngres = (isServerClassINGRES(pstmt->pdbcOwner))?TRUE:FALSE;
    i2        fSqlType;
    SQLINTEGER    OctetLength;         /* Length in bytes of char or binary data*/

    TRACE_INTL (pstmt, "odbc_setDescr");

     ret = PrepareParms(pstmt);
     if (ret != SQL_SUCCESS)
     {
         pstmt->sqlca.SQLCODE  = ret;
         return FALSE;
     }

    if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE && pstmt->fAPILevel 
        >= IIAPI_LEVEL_5 && pstmt->fCursorType == SQL_CURSOR_STATIC)
    {
        ErrState(SQL_HY000,pstmt, F_OD0168_IDS_ERR_SCRL_PROC);
        return FALSE;
    }
    
    setDescrParm.sd_genParm.gp_callback = NULL;
    setDescrParm.sd_genParm.gp_closure = NULL;
    setDescrParm.sd_stmtHandle = pstmt->stmtHandle;

    if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
        parmCount = pIPD->Count;         /* count of procedure parameters */
    else
        parmCount = pstmt->icolParmLast; /* count of parameter markers */

    if (parmCount < 0)                   /* bulletproofing */
        parmCount = 0;

    if (apiQueryType == IIAPI_QT_OPEN              ||
        apiQueryType == IIAPI_QT_EXEC_REPEAT_QUERY ||
        apiQueryType == IIAPI_QT_CURSOR_UPDATE     ||
        apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
    {
        setDescrParm.sd_descriptorCount++;  /* account for service parameter */

        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE  && *pstmt->szProcSchema)
        {
            setDescrParm.sd_descriptorCount++;
        }
    }

    if (setDescrParm.sd_descriptorCount == 0  &&  parmCount == 0)
        return(rc);  /* return if no parameters for anyone */

    papd = pAPD->pcol + 1;
    pipd = pIPD->pcol + 1;

    if  (GetParmValues(pstmt, papd, pipd) == SQL_ERROR)
        return FALSE;

    psd = setDescrParm.sd_descriptor = (IIAPI_DESCRIPTOR*)
        odbc_malloc( (parmCount+2) * sizeof(IIAPI_DESCRIPTOR ) );

    /*
    ** Move descriptor info from ODBC descriptors to API area 
    */
    switch (apiQueryType)
    {
    case IIAPI_QT_OPEN:
        psd->ds_nullable   = FALSE;
        psd->ds_length     = (II_UINT2)STlength (pstmt->szCursor);
        psd->ds_dataType   = IIAPI_CHA_TYPE;
        psd->ds_precision  = 0;
        psd->ds_scale      = 0;
        psd->ds_columnType = IIAPI_COL_SVCPARM;
        psd->ds_columnName = NULL;
        psd++;
        break;

    case IIAPI_QT_EXEC_REPEAT_QUERY:
    case IIAPI_QT_CURSOR_UPDATE:
        psd->ds_dataType   = IIAPI_HNDL_TYPE;
        psd->ds_nullable   = FALSE;
        psd->ds_length     = sizeof( VOID * );
        psd->ds_precision  = 0;
        psd->ds_scale      = 0;
        psd->ds_columnType = IIAPI_COL_SVCPARM;
        psd->ds_columnName = NULL;
        psd++;
        break;

    case IIAPI_QT_EXEC_PROCEDURE:
        psd->ds_dataType   = IIAPI_CHA_TYPE;
        psd->ds_nullable   = FALSE;
        psd->ds_length     = (II_UINT2)STlength (pstmt->szProcName);
        psd->ds_precision  = 0;
        psd->ds_scale      = 0;
        psd->ds_columnType = IIAPI_COL_SVCPARM;
        psd->ds_columnName = NULL;
        psd++;

        if (*pstmt->szProcSchema == EOS)  /* break if no schema qualifier */
            break;                        /*   for procedure name */

        psd->ds_dataType   = IIAPI_CHA_TYPE;
        psd->ds_nullable   = FALSE;
        psd->ds_length     = (II_UINT2)STlength (pstmt->szProcSchema);
        psd->ds_precision  = 0;
        psd->ds_scale      = 0;
        psd->ds_columnType = IIAPI_COL_SVCPARM;
        psd->ds_columnName = NULL;
        psd++;
        break;

    default:
        break;
    }


    papd = pAPD->pcol + 1;  /* papd->application parm descriptor field */
    pipd = pIPD->pcol + 1;  /* pipd->implementation parm descriptor field */
    
    for (i=0;  i<parmCount;  i++, psd++, papd++, pipd++)
    {
        SQLINTEGER  * IndicatorPtr;       /* Ptr to NULL indicator */
    
        fSqlType = pipd->ConciseType;     /* target SQL type */
    
        setDescrParm.sd_descriptorCount++;  /* start a new IIAPI_DESCRIPTOR*/
    
        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)  /* if procedure parameters */
        {
            SQLSMALLINT  ParameterType = pipd->ParameterType;
    
            if (ParameterType == SQL_PARAM_TYPE_UNKNOWN)
            {   /* skip over SQL_DEFAULT parameters */
                setDescrParm.sd_descriptorCount--;
                psd--;  /* step back so we walk back into IIAPI_DESCRIPTOR*/
                continue;  /* continue to next parm in for loop */
            }
            else if (ParameterType == SQL_PARAM_INPUT_OUTPUT)
                psd->ds_columnType = IIAPI_COL_PROCBYREFPARM;
            else if (ParameterType == SQL_PARAM_OUTPUT)
            {
                 psd->ds_columnType = IIAPI_COL_PROCBYREFPARM;
                 psd->ds_dataType = IIAPI_LTXT_TYPE;
                 psd->ds_nullable = TRUE;
                 psd->ds_length = 2; 
                 psd->ds_precision = 0;
                 psd->ds_scale = 0; 
                 psd->ds_columnName = (*pipd->Name)? pipd->Name : NULL;
                 continue;
            }
            else if (ParameterType == SQL_PARAM_TYPE_PROCGTTPARM)
                psd->ds_columnType = IIAPI_COL_PROCGTTPARM;
            /*  else if (ParameterType == SQL_PARAM_OUTPUT) was removed from
                       descriptor by ScanProcParmsAndBuildDescriptor */
            else /* (ParameterType == SQL_PARAM_INPUT) */
                psd->ds_columnType = IIAPI_COL_PROCPARM;
    
            psd->ds_columnName = (*pipd->Name)? pipd->Name : NULL;
                                             /* procedure parameter name */
        }
        else  /* normal query parameters*/
        {
            psd->ds_columnType = IIAPI_COL_QPARM;
            psd->ds_columnName = NULL;
        }
        psd->ds_dataType   = pipd->fIngApiType;
    
        /* 
        ** Change date to char.
        ** Don't change date to char if non-Ingres server and SQL_TIMESTAMP.
        */
        if (!(!isIngres && fSqlType == SQL_TYPE_TIMESTAMP))
             SpecialApiDataType(psd, papd->ConciseType, isIngres,
             pstmt->fAPILevel, pipd);

        /*  
        ** Get IndicatorPtr after adjustment for bind offset, row number,
        ** and row or column binding. 
        */
        GetBoundDataPtrs(pstmt, Type_APD, pAPD, papd,
                         NULL,
                         NULL,
                         &IndicatorPtr);
    
        if (IndicatorPtr  &&  *IndicatorPtr == SQL_NULL_DATA /* -1 */)
            psd->ds_nullable = TRUE;
        else if (psd->ds_dataType == IIAPI_LTXT_TYPE)
                        psd->ds_nullable = TRUE;
        else
            psd->ds_nullable = FALSE;
    
        OctetLength = pipd->OctetLength;
    
        /*
        ** The API doesn't like zero-length lengths in its descriptor 
        ** for chars, byte arrays, or nchars, so allow for at least one 
        ** character if the data is a zero-length string.
        */
        if (OctetLength == 0)
        {
            switch (psd->ds_dataType)
            {
            case IIAPI_CHA_TYPE:
            case IIAPI_BYTE_TYPE:
            case IIAPI_NBR_TYPE:
                OctetLength = sizeof(SQLCHAR);
                break;

            case IIAPI_NCHA_TYPE:
                OctetLength = sizeof(SQLWCHAR);
                break;

            default:
                break;
            }
        }
    
        if (fSqlType == SQL_WCHAR  &&  sizeof(SQLWCHAR) == 4)
            /* if UCS4 Unicode, adjust length down */
            OctetLength /= 2;        /* to UCS2 length */
    
            /* start with byte length of the parameter */
        psd->ds_length = (II_UINT2) OctetLength;
    
        if (fSqlType == SQL_VARCHAR     ||
            fSqlType == SQL_VARBINARY   ||
            fSqlType == SQL_WVARCHAR)
        {
            if (pipd->ParameterType == SQL_PARAM_INPUT       ||
                pipd->ParameterType == SQL_PARAM_INPUT_OUTPUT)
            {  
                rgbData = pipd->DataPtr + pipd->cbDataOffset;
                if (fSqlType == SQL_WVARCHAR  &&  
                                sizeof(SQLWCHAR) == 4)
                    /* if WVARCHAR && UCS4, use UCS2 len*/
                    psd->ds_length = (II_UINT2)(OctetLength/2+2);
                else
                    psd->ds_length += 2;
            }
            else
                psd->ds_length += 2;
        }
        else
        if (fSqlType == SQL_LONGVARCHAR || fSqlType == 
                    SQL_LONGVARBINARY || fSqlType == SQL_WLONGVARCHAR)
        {
            if (pipd->OctetLength > putParmsSEGMENTSIZE)
                psd->ds_length = (II_UINT2)(pipd->OctetLength + 2);
            else
                psd->ds_length = putParmsSEGMENTSIZE+2;
        }
    
        if (pipd->fIngApiType == IIAPI_DEC_TYPE  ||
            pipd->fIngApiType == IIAPI_MNY_TYPE)
               psd->ds_precision = (II_INT2) pipd->Precision;
        else   
            psd->ds_precision = 0;
    
        if (pipd->fIngApiType == IIAPI_DEC_TYPE)
               psd->ds_scale     = pipd->Scale;
        else   
            psd->ds_scale     = 0;
    
    }  /* end for loop building parameters */
	
    IIapi_setDescriptor( &setDescrParm );
    
    if (!odbc_getResult( &setDescrParm.sd_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
        rc = FALSE;
    
    odbc_free( (VOID *)setDescrParm.sd_descriptor );
    
    return(rc);
}

/*-------------------------------------------------------------*/
BOOL odbc_getQInfo( LPSTMT pstmt )
{
    IIAPI_GETQINFOPARM  getQInfoParm ZERO_FILL;
    BOOL rc=FALSE;

    TRACE_INTL(pstmt,"odbc_getQInfo");
    /*
    ** Provide input parameters for IIapi_getQueryInfo().
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = pstmt->stmtHandle;

    /*
    ** Invoke API function call.
    */
    IIapi_getQueryInfo( &getQInfoParm );

    rc=odbc_getResult( &getQInfoParm.gq_genParm, &pstmt->sqlca, pstmt->cQueryTimeout );

    if ((getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT) && 
        (getQInfoParm.gq_rowCount) )
    {
        /*
        ** If this is an insert or update query with multiple parameters,
        ** increment the number of rows processed (NRP) since the current
        ** implementation of bulk inserts executes a series of queries on
        ** invididual arrays, rather than one query on the with the entire
        ** array as the parameter.
        */
        if (pstmt->crowParm > 1)
            pstmt->sqlca.SQLNRP += getQInfoParm.gq_rowCount;
        else
            pstmt->sqlca.SQLNRP = getQInfoParm.gq_rowCount;
    }

    if (getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_RET)
    {
        pstmt->sqlca.SQLRS2 = getQInfoParm.gq_procedureReturn;
        pstmt->sqlca.SQLRS3 = 1;
    }
    else
        pstmt->sqlca.SQLRS3 = 0;

    return(rc);
}
/*
** Name: odbc_rollbackTran  
**
** Description:
**      This function rolls back a transaction and frees the transaction
**      handle. 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
BOOL odbc_rollbackTran(VOID *	*tranHandle, LPSQLCA psqlca )
{
    IIAPI_ROLLBACKPARM		rollbackParm;
    BOOL                 rc = TRUE;

    if ((*tranHandle) == NULL)
         return(TRUE);
    /*
    ** Provide parameters for IIapi_rollback().
    */
    rollbackParm.rb_genParm.gp_callback = NULL;
    rollbackParm.rb_genParm.gp_closure = NULL;
    rollbackParm.rb_tranHandle = *tranHandle;
    rollbackParm.rb_savePointHandle = NULL;

    IIapi_rollback( &rollbackParm );

    if (!odbc_getResult( &rollbackParm.rb_genParm, psqlca, 0 ))
		rc = FALSE;

	*tranHandle = ( VOID * )NULL;
    return (rc);
}
/*
** Name: odbc_commitTran  
**
** Description:
**      This function commits a transaction and frees the transaction
**      handle. 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
BOOL odbc_commitTran(VOID *	*tranHandle, LPSQLCA psqlca ) 
{
    IIAPI_COMMITPARM  commitParm;
    BOOL           rc = TRUE;
    
    if ((*tranHandle) == NULL)
         return(TRUE);
    /*
    ** provide input parameters for IIapi_commit().
    */
    commitParm.cm_genParm.gp_callback = NULL;
    commitParm.cm_genParm.gp_closure = NULL;
    commitParm.cm_tranHandle = *tranHandle;

    IIapi_commit( &commitParm );

    if (!odbc_getResult( &commitParm.cm_genParm, psqlca, 0 ))
		rc = FALSE;

    *tranHandle = NULL;

    return(rc);
}
/*
** Name: odbc_conn 
**
** Description:
**      This function establishes connection with the specific target.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_conn(LPDBC pdbc, SESS * pSession, LPSQLCA psqlca)
{
IIAPI_SETCONPRMPARM connSetParm;
IIAPI_CONNPARM      connParm;
char				szTarget[LEN_NODE+LEN_DBNAME+LEN_SERVERTYPE+10];
BOOL bRC = TRUE;
    

	if ( (STlength(pdbc->szVNODE) > 0) &&  (STbcompare(pdbc->szVNODE,7,"(local)",7,TRUE) != 0) )
		STprintf(szTarget,"%s::%s",pdbc->szVNODE,pdbc->szDATABASE);
	else
		STcopy (pdbc->szDATABASE, szTarget);  /* no vnode or vnode = "(local)" */


    if (STlength(pdbc->szSERVERTYPE) > 0 )
	{
		STcat(szTarget,"/");
		STcat(szTarget,pdbc->szSERVERTYPE);
	} 

    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target = odbc_stalloc( szTarget );
    connParm.co_type = IIAPI_CT_SQL;
    connParm.co_connHandle = pSession->connHandle;
    connParm.co_tranHandle = pSession->XAtranHandle;  /* usually NULL */
    connParm.co_username = NULL;
    connParm.co_password = NULL;

	/* pdbc->szUID contains the supplied effective-user (like sql -u flag), If 
	** pdbc->szPWD also supplied, connect using effective user as actual user.
	** If no password supplied, modify connection to set II_CP_EFFECTIVE_USER 
	** and connect as actual-user (NULL). This ensures that connections
	** utilising netu installation passwords only succeed if process owner
	** has ingres privilege to do so.- bug 108217*/

	if (pdbc->cbUID)
	{
		/* effective username supplied - so check for password */
		if (pdbc->cbPWD)
		{
			/* username & password supplied, so set 
			actual user to 'effective user' (old behaviour) */
			connParm.co_username = odbc_stalloc(pdbc->szUID);
		}
		else
		{		
			/* Connect as process owner with effective user and installation password */
			/* This prevents unauthorised access via installation passwords */
			connSetParm.sc_genParm = connParm.co_genParm;
			connSetParm.sc_connHandle = connParm.co_connHandle;
			connSetParm.sc_paramID = IIAPI_CP_EFFECTIVE_USER;
			connSetParm.sc_paramValue = pdbc->szUID;
			IIapi_setConnectParam( &connSetParm );
			connParm.co_username = NULL;
		}
	}
	else
	{
		/* connect as process owner */
		connParm.co_username = NULL;
	}

	if (pdbc->cbPWD)
	{
		pwcrypt(pdbc->szUID, pdbc->szPWD); /* decrypts the pw */
		connParm.co_password = odbc_stalloc(pdbc->szPWD);
		pwcrypt(pdbc->szUID, pdbc->szPWD); /* encrypts the pw */
	}
    else
		connParm.co_password = NULL;
 
    if (pdbc->cLoginTimeout > 0)
		connParm.co_timeout = (i4)(pdbc->cLoginTimeout * 1000);
    else
	    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    
    if (! odbc_getResult( &connParm.co_genParm, psqlca, pdbc->cLoginTimeout ))
    {
        bRC = FALSE;
    }

    pSession->connHandle = connParm.co_connHandle;
    pSession->tranHandle = connParm.co_tranHandle;
    pdbc->fAPILevel = connParm.co_apiLevel;

    if ((VOID * )connParm.co_target != NULL)
        odbc_free((VOID *)connParm.co_target );

    if (( VOID * )connParm.co_username != NULL)
        odbc_free((VOID *)connParm.co_username );

    if (( VOID * )connParm.co_password != NULL)
        odbc_free((VOID *)connParm.co_password );

    return(bRC);
}

/*
** Name: odbc_disconn - Perform connection release operation
**
** Description:
**      This function releases connection with the specific target.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL  odbc_disconn( SESS *pSession, LPSQLCA psqlca )
{
	IIAPI_DISCONNPARM disconnParm;
    
    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = pSession->connHandle;
    
    IIapi_disconnect( &disconnParm );
    
    if (! odbc_getResult( &disconnParm.dc_genParm, psqlca, 0 ))
    {
        return (FALSE);
    }
    
    pSession->connHandle = NULL;

    return(TRUE);
}

/*
** Name: odbc_query 
**
** Description:
**      This function begins a SQL stmt and allocates a stmt handle.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
BOOL odbc_query(SESS * pSession,
					LPSTMT  pstmt, IIAPI_QUERYTYPE apiQueryType, LPSTR pQueryText)
{
    IIAPI_QUERYPARM   	queryParm;
    BOOL rc = TRUE;

    TRACE_INTL (pstmt, "odbc_query");
    /*
    ** provide input parameters for IIapi_query().
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = apiQueryType;  

	if ( (pstmt->icolParmLast > 0) ||       /* parameter count > 0 */
		 (apiQueryType == IIAPI_QT_OPEN) ||
		 (apiQueryType == IIAPI_QT_CURSOR_UPDATE) ||
		 (apiQueryType == IIAPI_QT_EXEC_PROCEDURE) )
        queryParm.qy_parameters = TRUE; 
	else
		queryParm.qy_parameters = FALSE;

	if (apiQueryType != IIAPI_QT_EXEC_PROCEDURE)
	{
		if (!pQueryText)
			queryParm.qy_queryText = odbc_stalloc( pstmt->szSqlStr );
		else
		{
			queryParm.qy_parameters = FALSE;
			queryParm.qy_queryText = odbc_stalloc( pQueryText );
		}
	}
	else
		queryParm.qy_queryText = NULL; 

	
    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_stmtHandle = NULL;
    
    /*
    **  Set the API scrolling flag if the cursor type is static
    **  or keyset-driven.
    */
    /*
    **  Set the API scrolling flag if the cursor type is static
    **  or keyset-driven and the query type is IIAPI_QT_OPEN.
    */
    if ((pstmt->fAPILevel >= IIAPI_LEVEL_5) && 
        (pstmt->fCursorType != SQL_CURSOR_FORWARD_ONLY))
    {
        
        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
        {
            ErrState(SQL_HY000,pstmt, F_OD0168_IDS_ERR_SCRL_PROC);
            return FALSE;
        }

        if (apiQueryType == IIAPI_QT_OPEN)
            queryParm.qy_flags = IIAPI_QF_SCROLL;
        else
            queryParm.qy_flags = 0;
    }
    else
        queryParm.qy_flags = 0;

    /*
    ** Invoke API function call.
    */
    IIapi_query( &queryParm );

    pSession->tranHandle = queryParm.qy_tranHandle;
    pstmt->stmtHandle = queryParm.qy_stmtHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
		rc = FALSE;

	if (queryParm.qy_queryText != NULL)
	    odbc_free( ( VOID * )queryParm.qy_queryText );

	return (rc);
}

/*
** Name: odbc_setPos 
**
** Description:
**      This function positions an updatable cursor within a result set
**      returned from SQLFetchScroll().
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_setPos(SESS *pSession, LPSTMT pstmt)
{
    IIAPI_GETCOLPARM    getColParm;
    IIAPI_SCROLLPARM    scrollParm;
    II_INT2             i,j;
    II_INT4             idx;
    CHAR               *prowCurrent;
    i4                 *rgbNull;
    char               *rgbData;
    u_i2               *rgbDataui2;
    LPDESC              pIRD = pstmt->pIRD;
    LPDESCFLD           pird;
    BOOL                rc = TRUE, scroll_cursor = FALSE;

    TRACE_INTL(pstmt,"odbc_setPos");

    getColParm.gc_columnData = NULL;

    scrollParm.sl_genParm.gp_callback = NULL;
    scrollParm.sl_genParm.gp_closure = NULL;
    scrollParm.sl_stmtHandle = pstmt->stmtHandle;
        scrollParm.sl_offset = (pstmt->fCursorPosition - 
            pstmt->fCurrentCursPosition);

    pstmt->fCurrentCursPosition = pstmt->fCursorPosition;
    scrollParm.sl_orientation = IIAPI_SCROLL_RELATIVE;    
    IIapi_scroll( &scrollParm );
    
    if (!odbc_getResult( &scrollParm.sl_genParm, 
        &pstmt->sqlca, pstmt->cQueryTimeout ))
    {
        rc = FALSE;
        goto pos_return;
    }

    getColParm.gc_stmtHandle = pstmt->stmtHandle;
    getColParm.gc_moreSegments = 0;  
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = pIRD->Count;
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * )
        odbc_malloc( sizeof( IIAPI_DATAVALUE ) *
                    getColParm.gc_rowCount * getColParm.gc_columnCount );
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    
    for( j = 0, idx = 0; j < getColParm.gc_rowCount; j++ )
    {
        pird = pIRD->pcol + 1; /* pird->1st impl row descriptor after bookmark */
        prowCurrent =  pstmt->pFetchBuffer + (j * pstmt->cbrow);
        for( i = 0; i < getColParm.gc_columnCount; i++, idx++, pird++ )
        {
            getColParm.gc_columnData[idx].dv_value = 
                prowCurrent + pird->cbDataOffset 
                    + pird->cbDataOffsetAdjustment;
        }  /* end for loop through gc_columnCount */
    }  /* end for loop through gc_rowCount */    

    IIapi_getColumns( &getColParm );

    if (!odbc_getResult( &getColParm.gc_genParm, &pstmt->sqlca, 
        pstmt->cQueryTimeout ))
    {
        rc = FALSE;
        goto pos_return;
    }
    /* see if any null value returned */
    for( j = 0, idx = 0; j < getColParm.gc_rowsReturned; j++ )
    {
        pird = pIRD->pcol + 1; /* pird->1st impl row descriptor after bookmark */
        for( i = 0; i < getColParm.gc_columnCount; i++, idx++, pird++)
        {
            if (pird->cbNullOffset > 0)  /* if null ind in buffer, set or reset it */
            {
                rgbNull = (i4*)(pstmt->pFetchBuffer + 
                                 (j * pstmt->cbrow) + pird->cbNullOffset);
                *rgbNull = (getColParm.gc_columnData[idx].dv_null) ? 
                            SQL_NULL_DATA /* -1 */ : 0;
            }

            rgbData = pstmt->pFetchBuffer + 
                             (j * pstmt->cbrow) + pird->cbDataOffset;

            if (pird->fIngApiType == IIAPI_NCHA_TYPE)  /* if NCHAR */
            {
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)rgbData,
                                             (u_i4)(pird->OctetLength/4));
            }
            else
            if (pird->fIngApiType == IIAPI_NVCH_TYPE)  /* if NVARCHAR */
            {
                rgbDataui2 = (u_i2*)rgbData;
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4(rgbDataui2+1, (u_i4*)(rgbDataui2+1),
                                              *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
                         /* convert NVARCHAR character length to byte length */
            }
        }
    }

pos_return:  

    if (getColParm.gc_columnData)
        odbc_free((VOID *) getColParm.gc_columnData);

    if (rc)
    {
        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
            pstmt->sqlca.SQLCODE = SQL_NO_DATA_FOUND;
        pstmt->sqlca.SQLNRP = getColParm.gc_rowsReturned;
        pstmt->icolFetch = getColParm.gc_columnCount;
    }
    else
    {
        pstmt->sqlca.SQLNRP = 0;
        pstmt->icolFetch = 0;
    }
    return(rc);
}
/*
** Name: odbc_scrollColumns 
**
** Description:
**      This function scrolls and returns columns pertaining to a previously 
**      invoked SQL statement.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_scrollColumns(SESS *pSession, LPSTMT pstmt)
{
    IIAPI_GETCOLPARM    getColParm;
    IIAPI_POSPARM       posParm;
    IIAPI_SCROLLPARM    scrollParm;
    II_INT2             i,j;
    II_INT4             idx;
    CHAR               *prowCurrent;
    i4                 *rgbNull;
    char               *rgbData;
#ifdef IIAPI_NVCH_TYPE
    u_i2               *rgbDataui2;
#endif
    LPDESC              pIRD = pstmt->pIRD;
    LPDESCFLD           pird;
    BOOL                rc = TRUE, scroll_cursor = FALSE;
    i4                  rowCount;

    TRACE_INTL(pstmt,"odbc_scrollColumns");

    getColParm.gc_rowCount = rowCount = pstmt->pARD->ArraySize; 
    getColParm.gc_columnCount = pIRD->Count;
    getColParm.gc_columnData = NULL;
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * )
        odbc_malloc( sizeof( IIAPI_DATAVALUE ) *
                    getColParm.gc_rowCount * getColParm.gc_columnCount );

    /*
    ** Set up arguments for IIapi_position().  
    */
    switch (pstmt->fCursorDirection)
    {
    case SQL_FETCH_NEXT:
        posParm.po_reference = IIAPI_POS_CURRENT;
        posParm.po_offset = 1;
        scroll_cursor = FALSE;
        break;

    case SQL_FETCH_FIRST:
        posParm.po_reference = IIAPI_POS_BEGIN;
        posParm.po_offset = 1;
        scroll_cursor = FALSE;
        break;

    case SQL_FETCH_LAST:
        posParm.po_reference = IIAPI_POS_END;
        posParm.po_offset = -(pstmt->pARD->ArraySize);
        scroll_cursor = FALSE;
        break;

    case SQL_FETCH_PRIOR:
        posParm.po_reference = IIAPI_POS_CURRENT;
        if (pstmt->fCursLastResSetSize)
        {
            posParm.po_offset =  -(pstmt->fCursLastResSetSize + 
                pstmt->pARD->ArraySize);
        }
        else
            posParm.po_offset = -(2 * pstmt->pARD->ArraySize) + 1;
        scroll_cursor = FALSE;
        break;

    case SQL_FETCH_ABSOLUTE:
        scrollParm.sl_orientation = IIAPI_SCROLL_ABSOLUTE;
        scrollParm.sl_offset = pstmt->fCursorOffset - 1;
        posParm.po_reference = IIAPI_POS_CURRENT;
        posParm.po_offset = 1;
        scroll_cursor = TRUE;
        break;

    case SQL_FETCH_RELATIVE:
        posParm.po_reference = IIAPI_POS_CURRENT;
        if (pstmt->fCursLastResSetSize)
            posParm.po_offset = (pstmt->fCursorOffset - 
                pstmt->fCursLastResSetSize);
        else
            posParm.po_offset = (pstmt->fCursorOffset - 
                pstmt->pARD->ArraySize) + 1;
        scroll_cursor = FALSE;
        break;

    default:
        scrollParm.sl_orientation = IIAPI_SCROLL_NEXT;
        scrollParm.sl_offset = 0;
        break;    
    }

    if (pstmt->irowCurrent > 1 && !isCursorReadOnly(pstmt))
    {
        posParm.po_reference = IIAPI_POS_CURRENT;
        posParm.po_offset = 1;
        scroll_cursor = FALSE;
    }
    
    if (scroll_cursor)
    {
        scrollParm.sl_genParm.gp_callback = NULL;
        scrollParm.sl_genParm.gp_closure = NULL;
        scrollParm.sl_stmtHandle = pstmt->stmtHandle;
    
        IIapi_scroll( &scrollParm );
    
        if (!odbc_getResult( &scrollParm.sl_genParm, 
            &pstmt->sqlca, pstmt->cQueryTimeout ))
        {
            rc = FALSE;
            goto scroll_return;
        }

        /*
        ** Discard the row just returned, we'll get it again with
        ** IIapi_position().
        */
        if (!odbc_getQInfo(pstmt))
        {
            rc = FALSE;
            goto scroll_return;
        }

    } /* if scroll_cursor == TRUE */

 
    posParm.po_genParm.gp_callback = NULL;
    posParm.po_genParm.gp_closure = NULL;
    posParm.po_stmtHandle = pstmt->stmtHandle;

    posParm.po_rowCount = rowCount;

    IIapi_position( &posParm );

    if (!odbc_getResult( &posParm.po_genParm, 
        &pstmt->sqlca, pstmt->cQueryTimeout ))
    {
        rc = FALSE;
        goto scroll_return;
    }

    getColParm.gc_stmtHandle = pstmt->stmtHandle;
    getColParm.gc_moreSegments = 0;  
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_rowCount = rowCount;
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    
    for( j = 0, idx = 0; j < getColParm.gc_rowCount; j++ )
    {
        pird = pIRD->pcol + 1; /* pird->1st impl row descriptor after bookmark */
        prowCurrent =  pstmt->pFetchBuffer + (j * pstmt->cbrow);
        for( i = 0; i < getColParm.gc_columnCount; i++, idx++, pird++ )
        {
            getColParm.gc_columnData[idx].dv_value = 
                prowCurrent + pird->cbDataOffset 
                    + pird->cbDataOffsetAdjustment;
        }  /* end for loop through gc_columnCount */
    }  /* end for loop through gc_rowCount */    

    IIapi_getColumns( &getColParm );

    if (!odbc_getResult( &getColParm.gc_genParm, &pstmt->sqlca, 
        pstmt->cQueryTimeout ))
    {
        rc = FALSE;
        goto scroll_return;
    }
    /* see if any null value returned */
    for( j = 0, idx = 0; j < getColParm.gc_rowsReturned; j++ )
    {
        pird = pIRD->pcol + 1; /* pird->1st impl row descriptor after bookmark */
        for( i = 0; i < getColParm.gc_columnCount; i++, idx++, pird++)
        {
            if (pird->cbNullOffset > 0)  /* if null ind in buffer, set or reset it */
            {
                rgbNull = (i4*)(pstmt->pFetchBuffer + 
                                 (j * pstmt->cbrow) + pird->cbNullOffset);
                *rgbNull = (getColParm.gc_columnData[idx].dv_null) ? 
                            SQL_NULL_DATA /* -1 */ : 0;
            }

            rgbData = pstmt->pFetchBuffer + 
                             (j * pstmt->cbrow) + pird->cbDataOffset;

            if (pird->fIngApiType == IIAPI_NCHA_TYPE)  /* if NCHAR */
            {
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)rgbData,
                                             (u_i4)(pird->OctetLength/4));
            }
            else
            if (pird->fIngApiType == IIAPI_NVCH_TYPE)  /* if NVARCHAR */
            {
                rgbDataui2 = (u_i2*)rgbData;
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4(rgbDataui2+1, (u_i4*)(rgbDataui2+1),
                                              *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
                         /* convert NVARCHAR character length to byte length */
            }
        }
    }

scroll_return:  

    if (rc)
    {
        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
        {

            pstmt->sqlca.SQLCODE = SQL_NO_DATA_FOUND;
            pstmt->fCursLastResSetSize = pstmt->fCurrentCursPosition;
            pstmt->fCurrentCursPosition++;
        }

        pstmt->sqlca.SQLNRP = getColParm.gc_rowsReturned;
        pstmt->icolFetch = getColParm.gc_columnCount;
        pstmt->fCurrentCursPosition += getColParm.gc_rowsReturned; 
    }
    else
    {
        pstmt->sqlca.SQLNRP = 0;
        pstmt->icolFetch = 0;
    }

    if (getColParm.gc_columnData)
        odbc_free((VOID *) getColParm.gc_columnData);
    return(rc);
}

/*
** Name: odbc_getColumns 
**
** Description:
**      This function returns columns pertaining to a previously 
**      invoked SQL statement.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
** History:
**     23-jan-2008 (Ralph Loen) Bug 119800
**        Set the row count to 1 if the cursor is updatable.
*/
static BOOL odbc_getColumns(SESS *pSession, LPSTMT pstmt)
{
    IIAPI_GETCOLPARM    getColParm;
    II_INT2             i,j;
    II_INT4             idx;
    CHAR               *prowCurrent;
    i4                 *rgbNull;
    char               *rgbData;
#ifdef IIAPI_NVCH_TYPE
    u_i2               *rgbDataui2;
#endif
    LPDESC              pIRD = pstmt->pIRD;
    LPDESCFLD           pird;
    BOOL             rc = TRUE;
    /*char msg[100]; */

    TRACE_INTL(pstmt,"odbc_getColumns");
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;

    if ((pstmt->fStatus & STMT_API_CURSOR_OPENED) &&
        !isCursorReadOnly(pstmt))
        getColParm.gc_rowCount = 1;
    else
        getColParm.gc_rowCount = pstmt->crowFetch; 
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_columnCount = pIRD->Count;
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * )
        odbc_malloc( sizeof( IIAPI_DATAVALUE ) *
                      getColParm.gc_rowCount *
                      getColParm.gc_columnCount );


    for( j = 0, idx = 0; j < getColParm.gc_rowCount; j++ )
       {
        pird = pIRD->pcol + 1; /* pird->1st impl row descriptor after bookmark */
        prowCurrent =  pstmt->pFetchBuffer + (j * pstmt->cbrow);
        for( i = 0; i < getColParm.gc_columnCount; i++, idx++, pird++ )
           {
              getColParm.gc_columnData[idx].dv_value = 
                      prowCurrent + pird->cbDataOffset 
                                  + pird->cbDataOffsetAdjustment;
           }  /* end for loop through gc_columnCount */
       }  /* end for loop through gc_rowCount */
    getColParm.gc_stmtHandle = pstmt->stmtHandle;
    getColParm.gc_moreSegments = 0;  

    IIapi_getColumns( &getColParm );

    if (!odbc_getResult( &getColParm.gc_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
    {
        rc = FALSE;
    }

    /* see if any null value returned */
    for( j = 0, idx = 0; j < getColParm.gc_rowsReturned; j++ )
    {
        pird = pIRD->pcol + 1; /* pird->1st impl row descriptor after bookmark */
        for( i = 0; i < getColParm.gc_columnCount; i++, idx++, pird++)
        {
            if (pird->cbNullOffset > 0)  /* if null ind in buffer, set or reset it */
            {
                rgbNull = (i4*)(pstmt->pFetchBuffer + 
                                 (j * pstmt->cbrow) + pird->cbNullOffset);
                *rgbNull = (getColParm.gc_columnData[idx].dv_null) ? 
                            SQL_NULL_DATA /* -1 */ : 0;
            }

            rgbData = pstmt->pFetchBuffer + 
                             (j * pstmt->cbrow) + pird->cbDataOffset;

# ifdef IIAPI_NCHA_TYPE

            if (pird->fIngApiType == IIAPI_NCHA_TYPE)  /* if NCHAR */
            {
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)rgbData,
                                             (u_i4)(pird->OctetLength/4));
            }
            else
            if (pird->fIngApiType == IIAPI_NVCH_TYPE)  /* if NVARCHAR */
            {
                rgbDataui2 = (u_i2*)rgbData;
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4(rgbDataui2+1, (u_i4*)(rgbDataui2+1),
                                              *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
                         /* convert NVARCHAR character length to byte length */
            }
# endif

        }  /* end for loop through columns */
    }  /* end for loop through rows */

/*    for( i = 0; i <= getColParm.gc_columnCount; i++)
    {
        odbc_free((VOID *) getColParm.gc_columnData[i].dv_value); 
    }
*/

    odbc_free((VOID *) getColParm.gc_columnData);

    if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
    {
        pstmt->sqlca.SQLCODE = 100;
    }
    pstmt->sqlca.SQLNRP = getColParm.gc_rowsReturned;
    /*STprintf(msg,"GetColumn %ld ",pstmt->sqlca.SQLNRP);  */
        /*TRACE_INTL (pstmt, msg); */
    pstmt->icolFetch = getColParm.gc_columnCount;
    return(rc);
}
/*
** Name: MoveCharChar 
**
** Description:
**      copy data from fetch buffer to application buffer
**
*/
static UDWORD MoveCharChar(LPSTMT pstmt,
    LPDESCFLD   pard,         /* --> application row descriptor column */
    CHAR      * rgbData,      /* --> data to copy from */
    UWORD       cbData)       /*  =  length of data remaining to be copied */
{
    CHAR      * rgbFrom;
    CHAR      * rgbTo;
    SWORD       cbCopy, cbLeft, cbMax;

    TRACE_INTL(pstmt,"MoveCharChar");
    rgbFrom = rgbData;
    cbLeft  = cbData;
    cbMax   = (SWORD)(pard->OctetLength  -    /* SQLBindCol(cbValueMax) */
                      pard->cbPartOffset - 1);
    cbCopy  = (SWORD)min(cbLeft, cbMax);

    if (cbCopy < 0)
        return(CVT_TRUNCATION);

    rgbTo   = pard->DataPtr + pard->cbPartOffset;

    pard->cbPartOffset += cbCopy;

    if ( pard->OctetLengthPtr)
        *pard->OctetLengthPtr = (SDWORD)(pard->cbPartOffset);

    MEcopy (rgbFrom, cbCopy, rgbTo);

    if (cbLeft - cbCopy > 0) 
        return (CVT_TRUNCATION);

    return (CVT_SUCCESS);
}
/*
** Name: MoveCharBin 
**
** Description:
**      copy data from fetch buffer to application buffer
**
*/
static UDWORD MoveCharBin(LPSTMT pstmt,
    LPDESCFLD   pard,         /* --> application row descriptor column */
    CHAR      * rgbData,      /* --> data to copy from */
    UWORD       cbData)       /*  =  length of data remaining to be copied */
{
    CHAR      * rgbFrom;
    CHAR      * rgbTo;
    SWORD       cbCopy, cbLeft, cbMax;

    TRACE_INTL(pstmt,"MoveCharBin");
    rgbFrom = rgbData;
    cbLeft  = cbData;
    cbMax   = (SWORD)(pard->OctetLength  -    /* SQLBindCol(cbValueMax) */
                      pard->cbPartOffset - 1);
    cbCopy  = (SWORD)min(cbLeft, cbMax);

    if (cbCopy < 0)
        return(CVT_TRUNCATION);

    rgbTo   = pard->DataPtr + pard->cbPartOffset;

    pard->cbPartOffset += cbCopy;

    if ( pard->OctetLengthPtr)
        *pard->OctetLengthPtr = (SDWORD)(pard->cbPartOffset);

    MEcopy (rgbFrom, cbCopy, rgbTo);

    if (cbLeft - cbCopy > 0) 
        return (CVT_TRUNCATION);

    return (CVT_SUCCESS);
}
/*
** Name: MoveBinChar 
**
** Description:
**      copy data from fetch buffer to application buffer
**
*/
static UDWORD MoveBinChar(LPSTMT pstmt,
    LPDESCFLD   pard,         /* --> application row descriptor column */
    CHAR      * rgbData,      /* --> data to copy from */
    UWORD       cbData)       /*  =  length of data remaining to be copied */
{
    CHAR      * rgbFrom;
    CHAR      * rgbTo;
    SWORD       cbCopy, cbLeft, cbMax;

    TRACE_INTL(pstmt,"MoveBinChar");
    rgbFrom = rgbData; 
    cbLeft  = cbData; 
    cbMax   = (SWORD)((pard->OctetLength  -    /* SQLBindCol(cbValueMax) */
                       pard->cbPartOffset - 1) / 2);
    cbCopy  = (SWORD)min(cbLeft, cbMax);

    if (cbCopy < 0)
        return(CVT_TRUNCATION);

    rgbTo   = pard->DataPtr + pard->cbPartOffset;

    pard->cbPartOffset += cbCopy;

    if ( pard->OctetLengthPtr)
        *pard->OctetLengthPtr = (SDWORD)(pard->cbPartOffset*2);

    CvtBinChar (rgbTo, rgbFrom, cbCopy);

    if (cbLeft - cbCopy > 0) 
        return (CVT_TRUNCATION);

    return (CVT_SUCCESS);
}


static void MoveData(LPSTMT pstmt, LPDESCFLD pard, LPDESCFLD pird)
{
    CHAR      * rgbData;
    UWORD       cbData; 
    UDWORD      rc2 = 0;

    TRACE_INTL(pstmt,"MoveData");
    rgbData = pstmt->pFetchBuffer + pird->cbDataOffset;
    cbData = * (UWORD *) rgbData;
    rgbData += 2;

    switch (pard->ConciseType)    /* SQLBindCol(fCType)  TargetType  */
    {
    case SQL_C_CHAR:
        switch (pird->fIngApiType)
        {
        case IIAPI_LVCH_TYPE:
                rc2 = MoveCharChar(pstmt, pard, rgbData, cbData); 
                break;

        case IIAPI_LBYTE_TYPE:
        case IIAPI_GEOM_TYPE :
        case IIAPI_POINT_TYPE :
        case IIAPI_MPOINT_TYPE :
        case IIAPI_LINE_TYPE :
        case IIAPI_MLINE_TYPE :
        case IIAPI_POLY_TYPE :
        case IIAPI_MPOLY_TYPE :
        case IIAPI_GEOMC_TYPE :
                rc2 = MoveBinChar(pstmt, pard, rgbData, cbData); 
                break;

        default:
                rc2 = CVT_INVALID;
                break;
        }
        break;

    case SQL_C_BINARY:
        switch (pird->fIngApiType)
        {
        case IIAPI_LVCH_TYPE:
        case IIAPI_LBYTE_TYPE:
        case IIAPI_GEOM_TYPE :
        case IIAPI_POINT_TYPE :
        case IIAPI_MPOINT_TYPE :
        case IIAPI_LINE_TYPE :
        case IIAPI_MLINE_TYPE :
        case IIAPI_POLY_TYPE :
        case IIAPI_MPOLY_TYPE :
        case IIAPI_GEOMC_TYPE :
# ifdef IIAPI_LNVCH_TYPE
        case IIAPI_LNVCH_TYPE:
# endif
                rc2 = MoveCharBin(pstmt, pard, rgbData, cbData); 
                break;

        default:
                rc2 = CVT_INVALID;
                break;
        }
        break;

    case SQL_C_FLOAT:
    case SQL_C_DOUBLE:
    case SQL_C_LONG:
    case SQL_C_SLONG:
    case SQL_C_ULONG:
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
    case SQL_C_USHORT:
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
    case SQL_C_UTINYINT:
    case SQL_C_BIT:
    case SQL_C_TYPE_DATE:
    case SQL_C_TYPE_TIME:
    case SQL_C_TYPE_TIMESTAMP:

        rc2 = CVT_OUT_OF_RANGE;
        break;

    default:
        rc2 = CVT_INVALID; 
        break;
    }

    pstmt->fCvtError |= rc2;
    return; 
}


/*
** Name: GetAllSegments 
**
** Description:
**      Retrieve all segments of current long varchar, long byte, 
**      or long nvarchar column.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL GetAllSegments(LPSTMT pstmt,
                              LPDESCFLD pard, LPDESCFLD pird,
                              LPLONGBUFFER plb, BOOL ignoreBlob)
{
    IIAPI_GETCOLPARM  getColParm;
    BOOL           rc = TRUE;
    IIAPI_DATAVALUE   dv = { FALSE, 0, NULL };
    UWORD             segLength;
    UDWORD            cbTotal;
    CHAR            * pNewBuf;
    char             *rgbData;
    u_i2             *rgbDataui2;
    BOOL             igb = ignoreBlob;

    TRACE_INTL(pstmt,"GetAllSegments");
    rgbData    = pstmt->pFetchBuffer + pird->cbDataOffset;  /* data buffer */
    rgbDataui2 = (u_i2*)rgbData;
    *rgbDataui2 = 0;

    if (pard->fStatus & COL_BOUND)
        cbTotal = pard->OctetLength;
    else
        cbTotal = FOUR_K;

    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1; 
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_stmtHandle = pstmt->stmtHandle;
    getColParm.gc_columnCount = 1; 
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * ) &dv;
    getColParm.gc_moreSegments = FALSE;  
    getColParm.gc_columnData[0].dv_value = rgbData;

    /* 
    ** Move 1st segment data to appl bound buffer.
    */
    if (!plb)
        MoveData(pstmt, pard, pird); 
	
    for(;;)
    {
        *rgbDataui2 = 0;   /* init character length prefix to 0 */
        IIapi_getColumns( &getColParm ); /* get next segment of long data */

        if (!odbc_getResult(&getColParm.gc_genParm, 
            &pstmt->sqlca, pstmt->cQueryTimeout))
        {
            rc = FALSE;
            break;
        }

        /* 
        ** First two bytes has char length, followed by data.
        ** rgbDataui2-> character length 
        */

        if (pird->fIngApiType == IIAPI_LNVCH_TYPE)  /* if LONG NVARCHAR */
        {
            /* 
            ** Convert LONG NVARCHAR character length to byte length.
            */
            if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                ConvertUCS2ToUCS4(rgbDataui2+1, (u_i4*)(rgbDataui2+1),
                    *rgbDataui2);
            *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
        }
        /* 
        ** Move data to appl bound buffer.
        */
        if (!plb)
            MoveData(pstmt, pard, pird);  
        else if (!igb)
        {
            /* 
            ** Else move data to long buffer (plb->pLongData)
            */
            segLength = * (UWORD *) (getColParm.gc_columnData[0].dv_value);
            if ((plb->cbData + segLength) > cbTotal) 
            {
                /*
                **  If insufficient space was allocated for the blob,
                **  add a truncation warning to the error message list.
                **  If more segments remain, fetch them, but ignore the
                **  data.  If no more segments, return.
                */
                if (pard->fStatus & COL_BOUND)
                {
                    ErrState(SQL_01004, pstmt, F_OD0072_IDS_ERR_TRUNCATION);
                    if (getColParm.gc_moreSegments == TRUE) 
                    {
                        igb = TRUE;
                        continue;
                    }
                    else
                        break;
                }
                if (cbTotal > 0x100000)
                    cbTotal += 0x100000;
                else
                    cbTotal *= 2;
                pNewBuf = odbc_malloc((size_t)cbTotal);
                if (pNewBuf)
                {
                    MEcopy(plb->pLongData, (size_t)plb->cbData,pNewBuf);
                    MEfree((PTR)(plb->pLongData));
                    plb->pLongData = pNewBuf;
                }
                else
                {
                    ErrState(SQL_HY001,pstmt, F_OD0006_IDS_FETCH_BUFFER);
                    break;
                }
            }
            MEcopy ((CHAR *)getColParm.gc_columnData[0].dv_value+2,
                segLength,(plb->pLongData + plb->cbData));
            plb->cbData += (UDWORD) segLength;
       }
       if (getColParm.gc_moreSegments == FALSE) 
           break;
    }  /* end for loop */

    return(rc);
}


/*
** Name: GetSegment 
**
** Description:
**      Retrieve a segment of current long varchar, long byte, 
**      or long nvarchar column.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL GetSegment(LPSTMT pstmt,
                              LPDESCFLD pard, LPDESCFLD pird,
                              LPLONGBUFFER plb)
{
    IIAPI_GETCOLPARM  getColParm;
    BOOL           rc = TRUE;
    IIAPI_DATAVALUE   dv;
    UWORD             segLength;
    UDWORD            cbTotal;
    char             *rgbData;
    u_i2             *rgbDataui2;

    TRACE_INTL(pstmt,"GetSegment");
    rgbData    = pstmt->pFetchBuffer + pird->cbDataOffset;  /* data buffer */
    rgbDataui2 = (u_i2*)rgbData;
    *rgbDataui2 = 0;

    if (pard->OctetLength > FOUR_K)
		cbTotal = pard->OctetLength;
    else
        cbTotal = FOUR_K;

    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1; 
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_stmtHandle = pstmt->stmtHandle;
    getColParm.gc_columnCount = 1; 
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * ) &dv;
    getColParm.gc_moreSegments = FALSE;  
    getColParm.gc_columnData[0].dv_value = rgbData;

    for(;;)
    {
        *rgbDataui2 = 0;   /* init character length prefix to 0 */
        IIapi_getColumns( &getColParm ); /* get next segment of long data */

        if (!odbc_getResult(&getColParm.gc_genParm, 
            &pstmt->sqlca, pstmt->cQueryTimeout))
        {
            return FALSE;
        }

        /* 
        ** First two bytes has char length, followed by data.
        ** rgbDataui2-> character length 
        */

#ifdef IIAPI_LNVCH_TYPE
        if (pird->fIngApiType == IIAPI_LNVCH_TYPE)  /* if LONG NVARCHAR */
        {
            /* 
            ** Convert LONG NVARCHAR character length to byte length.
            */
            if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                ConvertUCS2ToUCS4(rgbDataui2+1, (u_i4*)(rgbDataui2+1),
                    *rgbDataui2);
            *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
        }
#endif
        /* 
        ** Move data to appl bound buffer.
        */
        segLength = * (UWORD *) (getColParm.gc_columnData[0].dv_value);
        MEcopy ((CHAR *)getColParm.gc_columnData[0].dv_value+2,
            segLength,(plb->pLongData + plb->cbData));
        plb->cbData += (UDWORD) segLength;
       
        if (getColParm.gc_moreSegments == FALSE)
        {
            pstmt->icolFetch++;
            break;
        }

        if (plb->cbData > (UDWORD)pard->OctetLength)
            break; 
    }  /* end for loop */

    return(rc);
}

/*
** Name: odbc_getColumnsLong 
**
** Description:
**      Same as odbc_getColumns except that this one includes columns
**      which are long varchar/long byte.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_getColumnsLong(SESS * pSession, LPSTMT pstmt)
	{
    IIAPI_GETCOLPARM    getColParm;
    UWORD               icolFirstInGroup;      /* no. of 1st col in group */
    II_INT2             i;
    II_UINT2            column_count, column_next;
    CHAR              * prowCurrent;
    i4                * rgbNull;
    char              * rgbData;
    u_i2              * rgbDataui2;
    BOOL                rc = TRUE;
    LPDESCFLD           pard;        /* -> application row descriptor */
    LPDESCFLD           pird;        /* -> implementation row descriptor */
    LPDESCFLD           pardSave, pirdSave;
    LPLONGBUFFER        plb=NULL;
    IIAPI_GETCOLINFOPARM getColInfoParm;
    
    TRACE_INTL(pstmt,"odbc_getColumnsLong");
    column_count = 0;
    column_next = 0;

    getColParm.gc_columnData = NULL;

    getColInfoParm.gi_stmtHandle = pstmt->stmtHandle;
    getColInfoParm.gi_mask = IIAPI_GI_LOB_LENGTH;
    getColInfoParm.gi_columnNumber = 0;
    getColInfoParm.gi_lobLength = 0;

    while ((SWORD)(pstmt->icolFetch) < pstmt->pIRD->Count)
    {
        getColParm.gc_genParm.gp_callback = NULL;
        getColParm.gc_genParm.gp_closure  = NULL;
        getColParm.gc_rowCount            = 1; 
        getColParm.gc_rowsReturned        = 0;
        getColParm.gc_stmtHandle          = pstmt->stmtHandle;
    
        icolFirstInGroup = pstmt->icolFetch; 
        pard = pstmt->pARD->pcol + icolFirstInGroup + 1;
        pird = pstmt->pIRD->pcol + icolFirstInGroup + 1;
    
        /*
        ** Count and fetch all columns up to and including a blob.
        */
        pirdSave = pird;
        pardSave = pard;

        for ( column_next = icolFirstInGroup + 1; column_next <= 
            (II_UINT2)pstmt->pIRD->Count; column_next++,pird++,pard++) 
        { 
            pirdSave = pird;
            pardSave = pard;
            if (pird->fIngApiType == IIAPI_LVCH_TYPE   ||  /* break on blob */
                pird->fIngApiType == IIAPI_GEOM_TYPE   ||
                pird->fIngApiType == IIAPI_POINT_TYPE  ||
                pird->fIngApiType == IIAPI_MPOINT_TYPE ||
                pird->fIngApiType == IIAPI_LINE_TYPE   ||
                pird->fIngApiType == IIAPI_MLINE_TYPE  ||
                pird->fIngApiType == IIAPI_POLY_TYPE   ||
                pird->fIngApiType == IIAPI_MPOLY_TYPE  ||
                pird->fIngApiType == IIAPI_GEOMC_TYPE  ||
# ifdef IIAPI_LNVCH_TYPE
                pird->fIngApiType == IIAPI_LBYTE_TYPE  ||
                pird->fIngApiType == IIAPI_LNVCH_TYPE)
# else
                pird->fIngApiType == IIAPI_LBYTE_TYPE)
# endif
            {
                getColInfoParm.gi_columnNumber = column_next;
                break;
            }
        }
        if (column_next > pstmt->pIRD->Count)
        {
            column_next--;
            pird--;
            pard--;
        }
        column_count =             /* count of columns to fetch */
           (II_UINT2)(column_next - pstmt->icolFetch);
        pstmt->icolFetch = column_next;  /* icolFetch=last col fetched */
    
        if (!column_count)
            break;

        getColParm.gc_columnCount = column_count; 
    
        if (getColParm.gc_columnData)
            odbc_free((VOID *) getColParm.gc_columnData);

        getColParm.gc_columnData  = ( IIAPI_DATAVALUE * )
            odbc_malloc(sizeof(IIAPI_DATAVALUE) * 
            getColParm.gc_columnCount);
    
        prowCurrent =  pstmt->pFetchBuffer; 
        /*
        ** Fill here where to read.
        */
        pard = pstmt->pARD->pcol + icolFirstInGroup + 1;
        pird = pstmt->pIRD->pcol + icolFirstInGroup + 1;
    
        for( i = 0; i < getColParm.gc_columnCount; i++, pird++) 
        {
            getColParm.gc_columnData[i].dv_value = 
                prowCurrent + pird->cbDataOffset;
        }
        getColParm.gc_moreSegments = FALSE;  /* No blob segments yet */  
        
        IIapi_getColumns( &getColParm );     /* get the columns */
    
        if (!odbc_getResult( &getColParm.gc_genParm, 
            &pstmt->sqlca, pstmt->cQueryTimeout ))
        {
            rc = FALSE;
            break;
        }

        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
        {
            pstmt->sqlca.SQLCODE = 100;
            break;
        }

        if (getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
        {
            rc = FALSE;
            break;
        }

        /*
        ** Point row descriptors back at the start.
        */
        pard = pstmt->pARD->pcol + icolFirstInGroup + 1;
        pird = pstmt->pIRD->pcol + icolFirstInGroup + 1;
    
        /* 
        ** See if any null value returned.
        */
        for( i = 0; i < getColParm.gc_columnCount; i++, pard++, pird++)
        {
            /* 
            ** If null ind in buffer, set or reset it.
            */
            if (pird->cbNullOffset > 0)  
            {
                rgbNull = (i4*)(pstmt->pFetchBuffer + 
                    pird->cbNullOffset);
                *rgbNull = (getColParm.gc_columnData[i].dv_null) ? 
                    SQL_NULL_DATA /* -1 */ : 0;
            }
    
            rgbData = pstmt->pFetchBuffer + pird->cbDataOffset;
            rgbDataui2 = (u_i2*)rgbData;
    
# ifdef IIAPI_NCHA_TYPE
            /*
            ** If this is a National Character data type,
            ** and this is a UCS4 platform, convert as required.
            */
            if (pird->fIngApiType == IIAPI_NCHA_TYPE) 
            {
                if (sizeof(SQLWCHAR) == 4) /* UCS2->UCS4 conversion? */
                    ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)(rgbData),
                        (u_i4)(pird->OctetLength/4));
            }
            else if (pird->fIngApiType == IIAPI_NVCH_TYPE  || 
                pird->fIngApiType == IIAPI_LNVCH_TYPE) 
            {
                if (sizeof(SQLWCHAR) == 4) 
                    ConvertUCS2ToUCS4(rgbDataui2+1, 
                        (u_i4*)(rgbDataui2+1), *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
            }
# endif
        }
    
        pird = pirdSave;  /* restore pird->last column (possibly blob) */
        pard = pardSave;  /* restore pard */
        pird->fStatus &= ~COL_MULTISEGS;  /* clear multi-segment flag */
        /*
        ** If there are more segments to get, store them in the 
        ** column's "long" data buffer.
        */
        if (getColParm.gc_moreSegments == TRUE) 
        {
            pird->fStatus |= COL_MULTISEGS;

            /*
            **  If the blob is not bound, and the columns after the
            **  blob are not bound, let SQLGetData() get the
            **  rest of the row.  Fetch the total blob length.
            */
            if (!(pard->fStatus & COL_BOUND))
            {
                 LPDESCFLD tmppird=pird, tmppard=pard;
                 II_UINT2 tmpColNext;
                 BOOL hasBoundCol=FALSE;
             
                 IIapi_getColumnInfo( &getColInfoParm );
                 if ( getColInfoParm.gi_status != IIAPI_ST_SUCCESS )
                 {
                     rc = FALSE;
                     break;
                 }
                 else
                     pird->blobLen = getColInfoParm.gi_lobLength;

                 for ( tmpColNext = column_next; tmpColNext < 
                     (II_UINT2)pstmt->pIRD->Count; tmpColNext++)
                 {
                     tmppird++;
                     tmppard++;
                     if (tmppard->fStatus & COL_BOUND)
                     {
                         hasBoundCol = TRUE;
                         break;
                     }
                 }

                 if (!hasBoundCol)
		 {
		    pstmt->icolFetch--; 
                    break;
                 }
            }

            /* 
            ** Allocate long buffer header if not allocated yet.
            */
            if (!plb)  
            {

                plb = odbc_malloc(sizeof(LONGBUFFER));
                if (plb)
                {
                    /*
                    ** Now allocate the long buffer itself if required.
                    */
                    if (pard->fStatus & COL_BOUND)
                        plb->pLongData = pard->DataPtr;
                    else
                        plb->pLongData = odbc_malloc(FOUR_K + 8); 
                    pard->plb = plb;
                }
            }  /* end long buffer header allocation */
            /* 
            ** Get the whole blob into the long buffer.
            */
            if (plb && (plb->pLongData)) 
            {
                plb->cbData =
                    (UDWORD)*(II_INT2*)getColParm.gc_columnData[getColParm.gc_columnCount-1].dv_value;
    
                if (pard->fStatus & COL_BOUND  &&
                    (SQLINTEGER)plb->cbData > pard->OctetLength)
                    MEcopy((CHAR *)getColParm.gc_columnData[getColParm.gc_columnCount-1].dv_value+2, 
                        (size_t)(pard->OctetLength), plb->pLongData);
                else
                    MEcopy((CHAR *)getColParm.gc_columnData[getColParm.gc_columnCount-1].dv_value+2, 
                        (size_t)(plb->cbData), plb->pLongData);
                GetAllSegments(pstmt, pard, pird, plb, FALSE);
                /* 
                ** Get all segments into the long buffer.  
                */
                pstmt->icolPrev = (UWORD)(pstmt->icolFetch + 1); 
            }
            else
            {
                ErrState(SQL_HY001,pstmt,F_OD0006_IDS_FETCH_BUFFER);
                rc = FALSE;
                break;
            }
        }   /* end additional blob segments to get */

        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
        {
            pstmt->sqlca.SQLCODE = 100;
            break;
		}

        if (getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
        {
            rc = FALSE;
            break;
        }
    } /* End while ((SWORD)(pstmt->icolFetch) < pstmt->pIRD->Count) */

    if (getColParm.gc_columnData)
        odbc_free((VOID *) getColParm.gc_columnData);

    pstmt->sqlca.SQLNRP = getColParm.gc_rowsReturned;

    return(rc);
}

/*
** Name: odbc_getUnboundColumnsLong 
**
** Description:
**      Same as odbc_getColumnsLong except that this one fetches columns
**      which are long varchar/long byte without exiting the state machine.
**
** On entry: 
**
** On exit:  May modify SQLCA error block.
**
** Returns:  TRUE if successful, FALSE if not.
**      
*/
static BOOL odbc_getUnboundColumnsLong(SESS * pSession, LPSTMT pstmt)
{
    IIAPI_GETCOLPARM    getColParm;
    UWORD               icolFirstInGroup;      /* no. of 1st col in group */
    II_INT2             i;
    II_UINT2            column_count, column_next;
    CHAR              * prowCurrent;
    i4                * rgbNull;
    char              * rgbData;
    u_i2              * rgbDataui2;
    BOOL                rc = TRUE;
    LPDESCFLD           pard;        /* -> application row descriptor */
    LPDESCFLD           pird;        /* -> implementation row descriptor */
    LPDESCFLD           pardSave, pirdSave;
    LPLONGBUFFER        plb=NULL;
    
    TRACE_INTL(pstmt,"odbc_getUnboundColumnsLong");
    column_count = 0;
    column_next = 0;
    getColParm.gc_columnData = NULL;
    while ((SWORD)(pstmt->icolFetch) < pstmt->pIRD->Count)
    {
        getColParm.gc_genParm.gp_callback = NULL;
        getColParm.gc_genParm.gp_closure  = NULL;
        getColParm.gc_rowCount            = 1; 
        getColParm.gc_rowsReturned        = 0;
        getColParm.gc_stmtHandle          = pstmt->stmtHandle;
    
        icolFirstInGroup = pstmt->icolFetch; 
        pard = pstmt->pARD->pcol + icolFirstInGroup + 1;
        pird = pstmt->pIRD->pcol + icolFirstInGroup + 1;
    
        /*
        ** Count and fetch all columns up to and including a blob.
        */
        pirdSave = pird;
        pardSave = pard;

        for ( column_next = icolFirstInGroup + 1; column_next <= 
            (II_UINT2)pstmt->pIRD->Count; column_next++,pird++,pard++) 
        { 
            pirdSave = pird;
            pardSave = pard;
            if (pird->fIngApiType == IIAPI_LVCH_TYPE   ||  /* break on blob */
                pird->fIngApiType == IIAPI_LBYTE_TYPE  ||
                pird->fIngApiType == IIAPI_GEOM_TYPE   ||
                pird->fIngApiType == IIAPI_POINT_TYPE  ||
                pird->fIngApiType == IIAPI_MPOINT_TYPE ||
                pird->fIngApiType == IIAPI_LINE_TYPE   ||
                pird->fIngApiType == IIAPI_MLINE_TYPE  ||
                pird->fIngApiType == IIAPI_POLY_TYPE   ||
                pird->fIngApiType == IIAPI_MPOLY_TYPE  ||
                pird->fIngApiType == IIAPI_GEOMC_TYPE  ||
                pird->fIngApiType == IIAPI_LNVCH_TYPE)
            {
                break;
            }
        }
        if (column_next > pstmt->pIRD->Count)
        {
            column_next--;
            pird--;
            pard--;
        }
        column_count =             /* count of columns to fetch */
           (II_UINT2)(column_next - pstmt->icolFetch);
        pstmt->icolFetch = column_next;  /* icolFetch=last col fetched */
    
        if (!column_count)
            break;

        getColParm.gc_columnCount = column_count; 
    
        if (getColParm.gc_columnData)
            odbc_free((VOID *) getColParm.gc_columnData);

        getColParm.gc_columnData  = ( IIAPI_DATAVALUE * )
            odbc_malloc(sizeof(IIAPI_DATAVALUE) * 
            getColParm.gc_columnCount);
    
        prowCurrent =  pstmt->pFetchBuffer; 
        /*
        ** Fill here where to read.
        */
        pard = pstmt->pARD->pcol + icolFirstInGroup + 1;
        pird = pstmt->pIRD->pcol + icolFirstInGroup + 1;
    
        for( i = 0; i < getColParm.gc_columnCount; i++, pird++) 
        {
            getColParm.gc_columnData[i].dv_value = 
                prowCurrent + pird->cbDataOffset;
        }
        getColParm.gc_moreSegments = FALSE;  /* No blob segments yet */  
        
        IIapi_getColumns( &getColParm );     /* get the columns */
    
        if (!odbc_getResult( &getColParm.gc_genParm, 
            &pstmt->sqlca, pstmt->cQueryTimeout ))
        {
            rc = FALSE;
            break;
        }

        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
        {
            pstmt->sqlca.SQLCODE = 100;
            break;
        }

        if (getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
        {
            rc = FALSE;
            break;
        }
    
        /*
        ** Point row descriptors back at the start.
        */
        pard = pstmt->pARD->pcol + icolFirstInGroup + 1;
        pird = pstmt->pIRD->pcol + icolFirstInGroup + 1;
    
        /* 
        ** See if any null value returned.
        */
        for( i = 0; i < getColParm.gc_columnCount; i++, pard++, pird++)
        {
            /* 
            ** If null ind in buffer, set or reset it.
            */
            if (pird->cbNullOffset > 0)  
            {
                rgbNull = (i4*)(pstmt->pFetchBuffer + 
                    pird->cbNullOffset);
                *rgbNull = (getColParm.gc_columnData[i].dv_null) ? 
                    SQL_NULL_DATA /* -1 */ : 0;
            }
    
            rgbData = pstmt->pFetchBuffer + pird->cbDataOffset;
            rgbDataui2 = (u_i2*)rgbData;
    
            /*
            ** If this is a National Character data type,
            ** and this is a UCS4 platform, convert as required.
            */
            if (pird->fIngApiType == IIAPI_NCHA_TYPE) 
            {
                if (sizeof(SQLWCHAR) == 4) /* UCS2->UCS4 conversion? */
                    ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)(rgbData),
                        (u_i4)(pird->OctetLength/4));
            }
            else if (pird->fIngApiType == IIAPI_NVCH_TYPE  || 
                pird->fIngApiType == IIAPI_LNVCH_TYPE) 
            {
                if (sizeof(SQLWCHAR) == 4) 
                    ConvertUCS2ToUCS4(rgbDataui2+1, 
                        (u_i4*)(rgbDataui2+1), *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
            }
        }
    
        pird = pirdSave;  /* restore pird->last column (possibly blob) */
        pard = pardSave;  /* restore pard */
        pird->fStatus &= ~COL_MULTISEGS;  /* clear multi-segment flag */
        /*
        ** If there are more segments to get, store them in the 
        ** column's "long" data buffer.
        */
        if (getColParm.gc_moreSegments == TRUE) 
        {
            pird->fStatus |= COL_MULTISEGS;

            /* 
            ** Allocate long buffer header if not allocated yet.
            */
            if (!plb)  
            {
                plb = odbc_malloc(sizeof(LONGBUFFER));
                if (plb)
                {
                    /*
                    ** Now allocate the long buffer itself if required.
                    */
                    if (pard->fStatus & COL_BOUND)
                        plb->pLongData = pard->DataPtr;
                    else
                        plb->pLongData = odbc_malloc(FOUR_K + 8); 
                    pard->plb = plb;
                }
            }  /* end long buffer header allocation */
            /* 
            ** Get the whole blob into the long buffer.
            */
            if (plb && (plb->pLongData)) 
            {
                plb->cbData =
                    (UDWORD)*(II_INT2*)getColParm.gc_columnData[getColParm.gc_columnCount-1].dv_value;
    
                    MEcopy((CHAR *)getColParm.gc_columnData[getColParm.gc_columnCount-1].dv_value+2, (size_t)(plb->cbData), plb->pLongData);
                    GetAllSegments(pstmt, pard, pird, plb, TRUE);
                    /* 
                    ** Get all segments into the long buffer.  
                    */
                    pstmt->icolPrev = (UWORD)(pstmt->icolFetch + 1); 
            }
            else
            {
                ErrState(SQL_HY001,pstmt,F_OD0006_IDS_FETCH_BUFFER);
                rc = FALSE;
                break;
            }
        }   /* end additional blob segments to get */

        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
        {
            pstmt->sqlca.SQLCODE = 100;
            break;
        }

        if (getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
        {
            rc = FALSE;
            break;
        }
    } /* End while ((SWORD)(pstmt->icolFetch) < pstmt->pIRD->Count) */

    if (getColParm.gc_columnData)
        odbc_free((VOID *) getColParm.gc_columnData);

    pstmt->sqlca.SQLNRP = getColParm.gc_rowsReturned;

    return(rc);
}

/*
** Name: odbc_getSegment 
**
** Description:
**      Get a single column or partial long column.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_getSegment(SESS * pSession, LPSTMT pstmt)
{
    IIAPI_GETCOLPARM    getColParm;
    i4                * rgbNull;
    char              * rgbData;
    u_i2              * rgbDataui2;
    BOOL                rc = TRUE;
    LPDESCFLD           pard;        /* -> application row descriptor */
    LPDESCFLD           pird;        /* -> implementation row descriptor */
    LPLONGBUFFER        plb=NULL;
    IIAPI_DATAVALUE     dv;
    PTR tmpPtr = NULL;
    IIAPI_GETCOLINFOPARM getColInfoParm;
    
    TRACE_INTL(pstmt,"odbc_getSegment");

    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure  = NULL;
    getColParm.gc_rowCount            = 1; 
    getColParm.gc_rowsReturned        = 0;
    getColParm.gc_stmtHandle          = pstmt->stmtHandle;
    getColParm.gc_columnCount = 1;     
    getColParm.gc_columnData  = &dv; 
    getColParm.gc_moreSegments = FALSE;  /* No blob segments yet */
		
    pard = pstmt->pARD->pcol + pstmt->icolFetch + 1;
    pird = pstmt->pIRD->pcol + pstmt->icolFetch + 1;

    plb = pard->plb;
    /*
    ** If there are more segments to get, store them in the 
    ** column's "long" data buffer.
    */
    if (pird->fStatus & COL_MULTISEGS) 
    {
        /* 
        ** Allocate long buffer header if not allocated yet.
        */
        if (!plb)  
        {
            plb = odbc_malloc(sizeof(LONGBUFFER));
            if (plb)
            {
                /*
                ** Now allocate the long buffer itself if required.
                */
                if (pard->OctetLength > FOUR_K)
                    plb->pLongData = odbc_malloc((pard->OctetLength * 2) + 16);
                else
                    plb->pLongData = odbc_malloc((FOUR_K * 2) + 16);
                pird->prevOctetLength = pard->OctetLength;
            }
        }  /* end long buffer header allocation */

        /* 
        ** Try to get the whole blob into the long buffer.
        */
        if (plb && plb->pLongData) 
        {
            if (!pard->plb)
            {
                plb->cbData = (UDWORD)*(II_INT2*)(pstmt->prowCurrent + 
					pird->cbDataOffset);
                MEcopy((PTR)(pstmt->prowCurrent + pird->cbDataOffset+2), 
                    (size_t)(plb->cbData), plb->pLongData);
            }
            pard->plb = plb;

            if ((plb->cbData - pard->cbPartOffset) > (UDWORD)pard->OctetLength)
                return TRUE;
            else
            {
                if (pird->prevOctetLength < pard->OctetLength)
                {
                    tmpPtr = plb->pLongData;
                    plb->pLongData = odbc_malloc((pard->OctetLength * 2) + 16);
                    MEcopy((PTR)tmpPtr, (size_t)plb->cbData, 
                        (PTR)plb->pLongData);
                        MEfree((PTR)tmpPtr);
                    pird->prevOctetLength = pard->OctetLength;
                }
                if (pard->cbPartOffset > 0)
                {
                    plb->cbData -= pard->cbPartOffset;

                    MEcopy((PTR)(plb->pLongData + pard->cbPartOffset),
                        (size_t)plb->cbData, (PTR)plb->pLongData);
                    pard->cbPartOffset = 0;
                }
                return GetSegment(pstmt, pard, pird, plb);
	    }
        }
        else
        {
            ErrState(SQL_HY001,pstmt,F_OD0006_IDS_FETCH_BUFFER);
            return FALSE;
        }
    }   /* end if COL_MULTISEGS */ 

    getColParm.gc_columnData->dv_value =
        pstmt->pFetchBuffer + pird->cbDataOffset;

    IIapi_getColumns( &getColParm );     /* get the columns */
    
    if (!odbc_getResult( &getColParm.gc_genParm, 
        &pstmt->sqlca, pstmt->cQueryTimeout ))
        return FALSE;

    if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
    {
        pstmt->sqlca.SQLCODE = 100;
        return TRUE;
    }

    if (getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
        return FALSE;

    /*
    ** If the next column must be retrieved in segments, set the flag
    ** and give the same treatment as the beginning of this routine.
    ** Get the total blob length.
    */   
    if (getColParm.gc_moreSegments == TRUE)
    {
        pird->fStatus |= COL_MULTISEGS;
        getColInfoParm.gi_stmtHandle = pstmt->stmtHandle;
        getColInfoParm.gi_mask = IIAPI_GI_LOB_LENGTH;
        getColInfoParm.gi_columnNumber = pstmt->icolFetch + 1;
        getColInfoParm.gi_lobLength = 0;
        IIapi_getColumnInfo( &getColInfoParm );
        if ( getColInfoParm.gi_status != IIAPI_ST_SUCCESS )
            return  FALSE;
        else
            pird->blobLen = getColInfoParm.gi_lobLength;
    }
    else 
    {
        pird->fStatus &= ~COL_MULTISEGS;
        pstmt->icolFetch++;  /* icolFetch=last col fetched */
    }
    /*
    ** If there are more segments to get, store them in the 
    ** column's "long" data buffer.
    */
    if (pird->fStatus & COL_MULTISEGS) 
    {
        /* 
        ** Allocate long buffer header if not allocated yet.
        */
        if (!plb)  
        {
            plb = odbc_malloc(sizeof(LONGBUFFER));
            if (plb)
            {
                /*
                ** Now allocate the long buffer itself if required.
                */
                if (pard->OctetLength > FOUR_K)
                    plb->pLongData = odbc_malloc((pard->OctetLength * 2) + 16);
                else
                    plb->pLongData = odbc_malloc((FOUR_K * 2) + 16);
                pird->prevOctetLength = pard->OctetLength;
            }
        }  /* end long buffer header allocation */

        /* 
        ** Try to get the whole blob into the long buffer.
        */
        if (plb && plb->pLongData) 
        {
            if (!pard->plb)
            {
                plb->cbData = (UDWORD)*(II_INT2*)(pstmt->prowCurrent + 
					pird->cbDataOffset);
                MEcopy((PTR)(pstmt->prowCurrent + pird->cbDataOffset+2), 
                    (size_t)(plb->cbData), plb->pLongData);
            }
            pard->plb = plb;

            if ((plb->cbData - pard->cbPartOffset) > (UDWORD)pard->OctetLength)
                return TRUE;
            else
            {
                if (pird->prevOctetLength < pard->OctetLength)
                {
                    tmpPtr = plb->pLongData;
                    plb->pLongData = odbc_malloc((pard->OctetLength * 2) + 16);
                    MEcopy((PTR)tmpPtr, (size_t)plb->cbData, 
                        (PTR)plb->pLongData);
                        MEfree((PTR)tmpPtr);
                    pird->prevOctetLength = pard->OctetLength;
                }
                if (pard->cbPartOffset > 0)
                {
                    plb->cbData -= pard->cbPartOffset;

                    MEcopy((PTR)(plb->pLongData + pard->cbPartOffset),
                        (size_t)plb->cbData, (PTR)plb->pLongData);
                    pard->cbPartOffset = 0;
                }
                return GetSegment(pstmt, pard, pird, plb);
	    }
        }
        else
        {
            ErrState(SQL_HY001,pstmt,F_OD0006_IDS_FETCH_BUFFER);
            return FALSE;
        }
    }   /* end if COL_MULTISEGS */ 
    /* 
    ** See if any null value returned.
    ** If null ind in buffer, set or reset it.
    */
    if (pird->cbNullOffset > 0)  
    {
        rgbNull = (i4*)(pstmt->pFetchBuffer + pird->cbNullOffset);
        *rgbNull = (getColParm.gc_columnData->dv_null) ? 
            SQL_NULL_DATA /* -1 */ : 0;
    }
    
    rgbData = pstmt->pFetchBuffer + pird->cbDataOffset;
    rgbDataui2 = (u_i2*)rgbData;
    
# ifdef IIAPI_NCHA_TYPE
    /*
    ** If this is a National Character data type,
    ** and this is a UCS4 platform, convert as required.
    */
    if (pird->fIngApiType == IIAPI_NCHA_TYPE) 
    {
        if (sizeof(SQLWCHAR) == 4) /* UCS2->UCS4 conversion? */
            ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)(rgbData),
                (u_i4)(pird->OctetLength/4));
    }
    else if (pird->fIngApiType == IIAPI_NVCH_TYPE  || 
        pird->fIngApiType == IIAPI_LNVCH_TYPE) 
    {
        if (sizeof(SQLWCHAR) == 4) 
            ConvertUCS2ToUCS4(rgbDataui2+1, 
                (u_i4*)(rgbDataui2+1), *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
    }
# endif
    
    if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
    {
        pstmt->sqlca.SQLCODE = 100;
        return TRUE;
    }

   if (getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
       return FALSE;

    pstmt->sqlca.SQLNRP = getColParm.gc_rowsReturned;

    return(rc);
}


/*
** Name: odbc_getSvcParms 
**
** Description:
**      This function sends SQL statement parameter values to DBMS server 
**
** On entry: 
**
** On exit:
**
** Returns: 
** 
** Change History
** --------------
**
**  23-oct-03  (loera01)
**    Created.
*/
static i2 odbc_getSvcParms( LPSTMT pstmt, QRY_CB *qry_cb, 
    IIAPI_QUERYTYPE apiQueryType)
{
	IIAPI_DATAVALUE   *ppd;

        IIAPI_PUTPARMPARM *serviceParms = &qry_cb->serviceParms;

	TRACE_INTL (pstmt, "odbc_getSvcParms");

	serviceParms->pp_genParm.gp_callback = NULL;
	serviceParms->pp_genParm.gp_closure = NULL;
	serviceParms->pp_stmtHandle = pstmt->stmtHandle;
	serviceParms->pp_parmCount  = 0;
	serviceParms->pp_moreSegments = 0;

	if (apiQueryType == IIAPI_QT_OPEN              ||
	    apiQueryType == IIAPI_QT_EXEC_REPEAT_QUERY ||
	    apiQueryType == IIAPI_QT_CURSOR_UPDATE     ||
	    apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
	{
	    serviceParms->pp_parmCount++; 

	    if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE && 
                *pstmt->szProcSchema)
	        serviceParms->pp_parmCount++;
	}

	if (serviceParms->pp_parmCount == 0)
	    return(0);

	ppd = serviceParms->pp_parmData = ( IIAPI_DATAVALUE * )
	    odbc_malloc( serviceParms->pp_parmCount * 
                sizeof( IIAPI_DATAVALUE ) );

	/*
	** provide parameters for API function
	*/
	switch (apiQueryType)  /* cursor or query handle info */
	{
	case IIAPI_QT_OPEN:
		ppd->dv_null   = FALSE;
                ppd->dv_length = (II_UINT2)STlength(pstmt->szCursor);
                ppd->dv_value  = (char *) &(pstmt->szCursor);
		ppd++;
		break;

	case IIAPI_QT_EXEC_REPEAT_QUERY:
		ppd->dv_null   = FALSE;
		ppd->dv_length = sizeof( VOID * );
		ppd->dv_value  = &(pstmt->QueryHandle);
		ppd++;
		break;

	case IIAPI_QT_CURSOR_UPDATE:
		ppd->dv_null   = FALSE;
		ppd->dv_length = sizeof( VOID * );
		GetCursorSelectStmtHandle(pstmt);
		ppd->dv_value  = &(pstmt->QueryHandle);
		ppd++;
		break;

	case IIAPI_QT_EXEC_PROCEDURE:
		ppd->dv_null   = FALSE;
		ppd->dv_length = (II_UINT2)STlength(pstmt->szProcName);
		ppd->dv_value  = (char *) &(pstmt->szProcName);
		ppd++;

                /*
                ** If the procedure has no schema (owner name), 
                ** break.
                */
		if (*pstmt->szProcSchema == EOS) 
		    break;                      

		ppd->dv_null   = FALSE;
		ppd->dv_length = (II_UINT2)STlength(pstmt->szProcSchema);
		ppd->dv_value  = (char *) &(pstmt->szProcSchema);
		ppd++;
		break;

	default: 
		break;

	}  /* end switch (apiQueryType) */

	return(serviceParms->pp_parmCount);
}

/*
** Name: odbc_putParms 
**
** Description:
**      This function sends SQL statement parameter values to DBMS server 
**
** On entry: 
**
** On exit:
**
** Returns: 
** 
** Change History
** --------------
**
**  date        programmer    description
**
**  02/07/1997  Jean Teng     length was not set correctly for decimal 
*/
static BOOL odbc_putParms(LPSTMT pstmt, IIAPI_QUERYTYPE apiQueryType,
    IIAPI_PUTPARMPARM *serviceParms)
{
    IIAPI_PUTPARMPARM    putParmParm = {0};
    IIAPI_DATAVALUE     *ppd, *svcp;
    LPDESC      pAPD = pstmt->pAPD;   /* Application Parameter Descriptor */
    LPDESC      pIPD = pstmt->pIPD;   /* Implementation Parameter Descriptor */
    LPDESCFLD   papd;                 /* APD parm field */
    LPDESCFLD   pipd;                 /* IPD parm field */
    II_INT2     i=0;
    II_INT2     parmCount=0;
    II_UINT4    length;
    II_UINT2    segmentlengthmax;     /* for LONGWCHAR maximum byte segment length */
    u_i2        charlength;           /* length of CHAR or UCS2 char (1 or 2) */
    BOOL     rc = TRUE;
    CHAR     *  rgbData;                    /* -->data in parm buffer */
    u_i2     *  rgbDataui2;                 /* -->data as u_i2 in parm buffer */
    u_i4     *  rgbDataui4;                 /* -->data as u_i4 in parm buffer */
    BOOL     isIngres = isServerClassINGRES(pstmt->pdbcOwner)?TRUE:FALSE;
    SQLSMALLINT fCType;
    SQLSMALLINT fSqlType;
    SQLINTEGER      OctetLength;          /* Length in bytes of char or binary data*/
    
    TRACE_INTL (pstmt, "odbc_putParms");

        papd = pAPD->pcol + 1;
        pipd = pIPD->pcol + 1;

    putParmParm.pp_genParm.gp_callback = NULL;
    putParmParm.pp_genParm.gp_closure = NULL;
    putParmParm.pp_stmtHandle = pstmt->stmtHandle;
    putParmParm.pp_parmCount  = 0;
    putParmParm.pp_moreSegments = 0;
    
    if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
        parmCount = pIPD->Count;         /* count of procedure parameters */
    else
        parmCount = pstmt->icolParmLast; /* count of parameter markers */
    
    if (parmCount < 0)                   /* bulletproofing */
        parmCount = 0;

    /*
    ** Add service parameters, if any, to count.
    */
    putParmParm.pp_parmCount += serviceParms->pp_parmCount;  
    
    if (putParmParm.pp_parmCount == 0  &&  parmCount == 0)
        return(rc);
    
    ppd = putParmParm.pp_parmData = ( IIAPI_DATAVALUE * )
        odbc_malloc( (parmCount+2) * sizeof( IIAPI_DATAVALUE ) );
    
    /*
    ** Provide parameters for API function.  Add service
    ** parameters first, then application data.
    */
    if (serviceParms->pp_parmCount)
        svcp = serviceParms->pp_parmData;

    for (i = 0; i < serviceParms->pp_parmCount; i++)
    {
        ppd->dv_null   = svcp->dv_null;
        ppd->dv_length = svcp->dv_length;
        ppd->dv_value  = svcp->dv_value;
        ppd++;
        svcp++;
    }

    papd = pAPD->pcol + 1;  /* pard->application descriptor field */
    pipd = pIPD->pcol + 1;  /* pird->implementation descriptor field */
    
    for (i=0;  i<parmCount;  i++, ppd++, papd++, pipd++)
    {
        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)  /* if procedure parameters */
        {
            SQLSMALLINT  ParameterType = pipd->ParameterType;
    
            if (ParameterType == SQL_PARAM_TYPE_UNKNOWN)
               {   /* skip over SQL_DEFAULT parameters */
                ppd--;  /* step back so we walk back into IIAPI_PUTPARMPARM again*/
                continue;  /* continue to next parm in for loop */
               }
        }
    
        putParmParm.pp_parmCount++;  /* start a new IIAPI_PUTPARMPARM */
    
        ppd->dv_null = pipd->Null;
    
        rgbData = pipd->DataPtr;   /* rgbData--> user data */
        rgbDataui2 = (u_i2 *)rgbData;/* rgbDataui2-->data as u_i2 in user data */
    
        OctetLength = pipd->OctetLength;
        fCType      = papd->ConciseType;
        fSqlType    = pipd->ConciseType;
    
        if (fSqlType == SQL_WCHAR  &&  sizeof(SQLWCHAR) == 4)
        {    
			/* 
			** if UCS4 Unicode, adjust byte length down.
			*/
            OctetLength /= 2;        /* to UCS2 length in bytes */
            ConvertUCS4ToUCS2((u_i4*)rgbData, (u_i2*)rgbData, OctetLength/2);
                                               /*convert using char length */
        }
        if (pipd->ParameterType == SQL_PARAM_OUTPUT)
            fSqlType = SQL_TYPE_NULL;
		
        ppd->dv_length = (II_UINT2)OctetLength; /* byte length of the parameter */
        ppd->dv_value = rgbData;
	
        switch (fSqlType)        /* special case overrides of dv_length */
        {                        /*                       and dv_value  */
        case SQL_VARCHAR:
        case SQL_VARBINARY:
        case SQL_LONGVARCHAR:
        case SQL_LONGVARBINARY:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
            if (fCType == SQL_C_TYPE_DATE  ||
                fCType == SQL_C_TYPE_TIME  ||
                fCType == SQL_C_TYPE_TIMESTAMP)
               break;  /* leave ppd->dv_length = pipd->OctetLength */

            if (fSqlType == SQL_VARCHAR  ||
                fSqlType == SQL_VARBINARY||
                fSqlType == SQL_WVARCHAR)
            {                         /* var length in 1st two bytes */
                if (fSqlType == SQL_WVARCHAR  &&  sizeof(SQLWCHAR) == 4)
                          /* if WVARCHAR && UCS4, use UCS2 len*/
                    ppd->dv_length = *rgbDataui2/2+2;
                else
                    ppd->dv_length = *rgbDataui2+2;

                if (fSqlType == SQL_WVARCHAR)       /* NVCHAR prefix has */
                {
                    *rgbDataui2 = *rgbDataui2 / sizeof(SQLWCHAR); /* char length */
                    if (sizeof(SQLWCHAR) == 4)
                        ConvertUCS4ToUCS2((u_i4*)(rgbDataui2+1),
                                          (u_i2*)(rgbDataui2+1),
                                          *rgbDataui2);
                }
                break;
            }

            if (ppd->dv_null == TRUE)  /* skip if NULL parm */
               break;
    
            putParmParm.pp_parmCount--;    /* flush out parms before the blob */
    
            if (putParmParm.pp_parmCount)  /* check if initial non-blob parms exist */
            {
                IIapi_putParms( &putParmParm ); /* put the initial non-blob parms out */
    
                if (!odbc_getResult( &putParmParm.pp_genParm, 
                                     &pstmt->sqlca, pstmt->cQueryTimeout ))
                {
                    rc = FALSE;
                    break;
                }
            }

            rgbDataui4 = (u_i4 *)rgbData;    /* rgbDataui4-->data as u_i4 */

            if (fSqlType == SQL_WLONGVARCHAR)       /* NVCHAR prefix has */
            {
                *rgbDataui4 = *rgbDataui4 / sizeof(SQLWCHAR); /* char length */
                if (sizeof(SQLWCHAR) == 4)
                    ConvertUCS4ToUCS2((u_i4*)(rgbDataui4+1),
                                      (u_i2*)(rgbDataui4+1),
                                      *rgbDataui4);  /* char length */
                charlength       = sizeof(u_i2);  /* now it's UCS2 data */
                segmentlengthmax = putParmsSEGMENTSIZE / sizeof(u_i2);
                                       /* max segment length in UCS2 chars */
            }
            else /* fSqlType == SQL_LONGVARCHAR or SQL_LONGVARBINARY */
            {
                charlength       = 1;
                segmentlengthmax = putParmsSEGMENTSIZE;
            }

            putParmParm.pp_parmCount  = 1;   /* putting out one UCS2 blob segment at a time*/
            putParmParm.pp_moreSegments = 1; /* assume there will be more segments */
            ppd = putParmParm.pp_parmData;   /* reset ppd -> 1st IIAPI_DATAVALUE */

            length = *rgbDataui4;            /* 4-byte prefix has char length of data  */
            rgbData += 2;                    /* rdbData->2 bytes before real data */
            rgbDataui2 = (u_i2 *)rgbData;    /* rgbDataui2-->data as u_i2 */

            while(rc == TRUE  &&  putParmParm.pp_moreSegments)
            {
                CHAR  savech0, savech1;
                II_UINT2 segmentlength;

                if (length <= segmentlengthmax)         /* bite off a segment */
                    segmentlength = (II_UINT2)length;
                else 
                    segmentlength = segmentlengthmax;
                length -= segmentlength;                /* decr length remaining */
                if (length==0)                          /* if no more segments */ 
                   putParmParm.pp_moreSegments = 0;
                    
                savech0=rgbData[0]; /* save the original data that is about */
                savech1=rgbData[1]; /* to be overlayed with the 2-byte len  */
                ppd->dv_null   = FALSE;
                ppd->dv_length = (II_UINT2)(segmentlength*charlength+2);
                ppd->dv_value  = rgbData;
                *rgbDataui2    = segmentlength;

                IIapi_putParms( &putParmParm );   /* put the segment out */

                if (!odbc_getResult( &putParmParm.pp_genParm, 
                                      &pstmt->sqlca, pstmt->cQueryTimeout ))
                   rc = FALSE;
               rgbData[0]=savech0;  /* restore the original data that was */
                rgbData[1]=savech1;  /* overlayed with the 2-byte len      */
                rgbData += segmentlength * charlength;
                                     /* move past the bytes of the segment */
                rgbDataui2 = (u_i2 *)rgbData;/* rgbDataui2-->data as u_i2 */
            }  /* end while (putParmParm.pp_moreSegments) */

            putParmParm.pp_parmCount    = 0;  /* restart API count back to 0 */
            putParmParm.pp_moreSegments = 0;
            ppd = putParmParm.pp_parmData-1;   /* reset ppd -> 1st IIAPI_DATAVALUE - 1
                                                  so for loop will walk into 1st */
            break;
            
        case SQL_TYPE_NULL: 
            ppd->dv_null   = TRUE;
            ppd->dv_length = 2;
            *rgbDataui2  = 0;
            break;
        
        case SQL_TYPE_TIME:
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            if (isIngres && pstmt->fAPILevel < IIAPI_LEVEL_4)
                ppd->dv_length = (II_UINT2)STlength(rgbData);
            break;

        default:
            /*  
            ** ...else leave ppd->dv_length at pipd->OctetLength         
            */
            break;
        }   /* end switch (fSqlType) */
    }  /* end for loop through user parameters */


	if (rc == TRUE  &&  putParmParm.pp_parmCount)  /* check if parms already put out */
	{
	    IIapi_putParms( &putParmParm );            /* put the last non-blob parms out */

	    if (!odbc_getResult( &putParmParm.pp_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
	        rc = FALSE;
	}


    if (IItrace.fTrace >= ODBC_IN_TRACE)
    {
        i4 pp;
        ppd = putParmParm.pp_parmData;
        for (pp = 0; pp < putParmParm.pp_parmCount; pp++)
        {
            TRdisplay("odbc_putParm data hex dump for parm %d len %d " \
                "null %d\n", pp+1, ppd->dv_length, ppd->dv_null);
            odbc_hexdump(ppd->dv_value, ppd->dv_length);
            ppd++;
        }
    }

    odbc_free((VOID *)putParmParm.pp_parmData );
    if (serviceParms->pp_parmData)
    {
        odbc_free ((VOID *)serviceParms->pp_parmData);
        serviceParms->pp_parmData = NULL; 
        serviceParms->pp_parmCount = 0;
    }

 
	return(rc);
}

/*
** Name: odbc_putSegment 
**
** Description:
**      This function sends SQL statement segments to the DBMS server.
**
** On entry: 
**
** On exit:
**
** Returns: 
** 
** Change History
** --------------
**
**  05-nov-2003 (loera01)
**      Created.
*/
static RETCODE odbc_putSegment(LPSTMT pstmt, IIAPI_QUERYTYPE 
    apiQueryType, IIAPI_PUTPARMPARM *serviceParms, BOOL moreData)
{
	IIAPI_PUTPARMPARM    putParmParm = {0};
	IIAPI_DATAVALUE     *ppd;
	LPDESC      pAPD = pstmt->pAPD;   /* Appl Param Descrip */
	LPDESC      pIPD = pstmt->pIPD;   /* Impl Param Descrip */
	LPDESCFLD   papd;                 /* APD parm field */
	LPDESCFLD   pipd;                 /* IPD parm field */
	II_INT2     i=0,k=0;
	II_INT2     parmCount=0;
	II_UINT4    length;
	UWORD       icol = (UWORD)pstmt->icolPut;
	II_UINT2    segmentlengthmax;     /* LONGWCHAR max segment len */
	u_i2        charlength;           /* len CHAR or UCS2 (1 or 2) */
	RETCODE     rc = SQL_NEED_DATA;
	CHAR     *  rgbData;              /* -->data in parm buffer */
	u_i2     *  rgbDataui2;           /* -->data as u_i2 in parm buffer */
	u_i4     *  rgbDataui4;           /* -->data as u_i4 in parm buffer */
	BOOL     isIngres = isServerClassINGRES(pstmt->pdbcOwner)?TRUE:FALSE;
	SQLSMALLINT fCType;
	SQLSMALLINT fSqlType;
	SQLINTEGER      OctetLength;   /* byte length of char or binary data */

	TRACE_INTL (pstmt, "odbc_putSegment");
        
        if (pstmt->icolSent == pstmt->icolParmLast)
            return SQL_NO_DATA;

	putParmParm.pp_genParm.gp_callback = NULL;
	putParmParm.pp_genParm.gp_closure = NULL;
	putParmParm.pp_stmtHandle = pstmt->stmtHandle;
	putParmParm.pp_parmCount  = 0;
	putParmParm.pp_moreSegments = 0;

	if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
	    parmCount = pIPD->Count;         /* procedure param count */
	else
	    parmCount = pstmt->icolParmLast - pstmt->icolSent; /* parameter count */

	if (parmCount < 0)                   /* bulletproofing */
	    parmCount = 0;

        /*
        ** For first parameter, add service parameters, if any, to count.
        */
	if (!(pstmt->fStatus & STMT_SEGINIT))
	{
	    putParmParm.pp_parmCount += serviceParms->pp_parmCount;  
	}

	if (putParmParm.pp_parmCount == 0  &&  parmCount == 0)
	    return(SQL_NO_DATA);

	ppd = putParmParm.pp_parmData = ( IIAPI_DATAVALUE * )
	    odbc_malloc( (parmCount+2) * sizeof( IIAPI_DATAVALUE ) );

	/*
	** Provide parameters for API function.  Add service
        ** parameters first, then application data.
	*/
	if (!(pstmt->fStatus & STMT_SEGINIT))
	{
            for (i = 0; i < serviceParms->pp_parmCount; i++)
            {
                ppd->dv_null   = serviceParms->pp_parmData->dv_null;
                ppd->dv_length = serviceParms->pp_parmData->dv_length;
                ppd->dv_value  = serviceParms->pp_parmData->dv_value;
                ppd++;
            }
        }

	pstmt->fStatus |= STMT_SEGINIT;

	papd = pAPD->pcol + 1;  /* pard->application descriptor field */
	pipd = pIPD->pcol + 1;  /* pird->implementation descriptor field */

	pipd += pstmt->icolSent;
	papd += pstmt->icolSent;

	for (i = pstmt->icolSent; i < icol; i++, ppd++, pipd++, papd++)
	{
		if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)  
		{
		    SQLSMALLINT  ParameterType = pipd->ParameterType;

                    /*
                    ** Unknown parameters cannot be sent.  Skip over.
                    */
		    if (ParameterType == SQL_PARAM_TYPE_UNKNOWN)
		    {
		        ppd--; 
		        continue;  /* continue to next parm in for loop */
		    }
		}

            putParmParm.pp_parmCount++;  /* start a new IIAPI_PUTPARMPARM */

            ppd->dv_null = pipd->Null;

                /*
                ** Point the data buffer at the IPD's pre-allocated buffer.
                ** Save embedded lengths in case this is a varying type.
                */
		rgbData = pipd->DataPtr;
		rgbDataui2 = (u_i2 *)rgbData;

		/*
		** Fixed length parameters will not have a segment length.  In those
		** cases, use the default octet length.  Otherwise, use the segment 
		** length set in SQLPutData().
		*/
        if (pipd->SegmentLength)
		    OctetLength = pipd->SegmentLength;
        else
			OctetLength = pipd->OctetLength;

		fCType      = papd->ConciseType;
		fSqlType    = pipd->ConciseType;

                /* 
                ** Adjust for UCS4 platforms. 
                */
		if (fSqlType == SQL_WCHAR  &&  sizeof(SQLWCHAR) == 4)
		{
		    OctetLength /= 2;  
		    ConvertUCS4ToUCS2((u_i4*)rgbData, 
                        (u_i2*)rgbData, OctetLength/2);
		}

		ppd->dv_length = (II_UINT2)OctetLength; /* param length */
		ppd->dv_value = rgbData;

                /*
                ** Obtain the lengths of blobs or varying data types.
                */
		switch (fSqlType)       
		{                      
		case SQL_VARCHAR:
		case SQL_VARBINARY:
		case SQL_LONGVARCHAR:
		case SQL_LONGVARBINARY:
		case SQL_WVARCHAR:
		case SQL_WLONGVARCHAR:
                    if (fCType == SQL_C_TYPE_DATE  ||
                        fCType == SQL_C_TYPE_TIME  ||
                        fCType == SQL_C_TYPE_TIMESTAMP)
                        break;  /* leave ppd->dv_length = pipd->OctetLength */

                    /*
                    ** Get the embedded two-byte length.
                    */
                    if (fSqlType == SQL_VARCHAR  ||
                        fSqlType == SQL_VARBINARY||
                        fSqlType == SQL_WVARCHAR)
                    {  
                        if (fSqlType == SQL_WVARCHAR  &&  sizeof(SQLWCHAR) == 4)
                        /* if WVARCHAR && UCS4, use UCS2 len*/
                            ppd->dv_length = *rgbDataui2/2+2;
                        else
                            ppd->dv_length = *rgbDataui2+2;

                        if (fSqlType == SQL_WVARCHAR)       
                        {
                            *rgbDataui2 = *rgbDataui2 / sizeof(SQLWCHAR); 
                            if (sizeof(SQLWCHAR) == 4)
                                ConvertUCS4ToUCS2((u_i4*)(rgbDataui2+1),
                                    (u_i2*)(rgbDataui2+1), *rgbDataui2);
                         }
                         break;
                     }

                     if (ppd->dv_null == TRUE)  /* skip if NULL parm */
                         break;

                     /*
                     ** Flush out non-blob parameters.
                     */
                     putParmParm.pp_parmCount--;  

                     /*
                     ** If non-blob parameters before this one, send them
                     ** out first.
                     */
                     if (putParmParm.pp_parmCount)  
                     {
                         putParmParm.pp_moreSegments = 0;

                         IIapi_putParms( &putParmParm ); 
                         if (!odbc_getResult( &putParmParm.pp_genParm, 
                             &pstmt->sqlca, pstmt->cQueryTimeout ))
                             return exitPutSeg(SQL_ERROR, 
                                 &putParmParm, serviceParms);
                                 pstmt->icolSent = i;
                     }
                     /*
                     ** Get the embedded four-byte length.
                     */
                     rgbDataui4 = (u_i4 *)rgbData;

                     if (fSqlType == SQL_WLONGVARCHAR) 
                     {
                         *rgbDataui4 = *rgbDataui4 / sizeof(SQLWCHAR);
                         if (sizeof(SQLWCHAR) == 4)
                             ConvertUCS4ToUCS2((u_i4*)(rgbDataui4+1),
                                 (u_i2*)(rgbDataui4+1),
                                 *rgbDataui4);  /* char length */

                          charlength = sizeof(u_i2);  /* now it's UCS2 data */

                          /* max segment length in UCS2 chars */
                          segmentlengthmax = putParmsSEGMENTSIZE/sizeof(u_i2);
                     }
                     else /* fSqlType == SQL_LONGVARCHAR or SQL_LONGVARBINARY */
                     {
                         charlength       = 1;
                         segmentlengthmax = putParmsSEGMENTSIZE;
                     }
                     /* 
                     ** Send one UCS2 blob segment at a time.
                     */
                     putParmParm.pp_parmCount = 1;

                     /*
                     ** Four-byte length is ready for use.
                     */
                     length = (II_UINT4)*rgbDataui4;
                     /*
                     ** Each segment has only a two-byte length prefix.
                     ** Save the original pointer for the length, and advance
                     ** the pointer for filling with the actual data.
                     */
                     rgbData += 2; 
                     rgbDataui2 = (u_i2 *)rgbData;  
                 
                    /*
                    ** Assume more segments.
		    */
                    putParmParm.pp_moreSegments = 1;

                    while(putParmParm.pp_moreSegments)
                    {
                        CHAR  savech0, savech1;
                        II_UINT2 segmentlength;

                        ppd = putParmParm.pp_parmData;
                        if (length <= segmentlengthmax) 
                             segmentlength = (II_UINT2)length;
                        else 
                             segmentlength = segmentlengthmax;

                    /*
                    ** Determine whether there are more segments to send.
                    */
                    length -= segmentlength;   
                    if (length == 0)
                    {
                        if (pstmt->moreData)
                            putParmParm.pp_moreSegments = 1;
                        else
                            putParmParm.pp_moreSegments = 0;
                    }
                        /*
                        **  We don't want to corrupt the original data
                        **  from the user, so save it for now.
                        */
                        savech0=rgbData[0]; 
                        savech1=rgbData[1];
                        
                        ppd->dv_null   = FALSE;
                        ppd->dv_length = (II_UINT2)(segmentlength*charlength+2);
                        ppd->dv_value  = rgbData;
                        *rgbDataui2  = segmentlength;

                        IIapi_putParms( &putParmParm );   /* Send blob seg */
                        if (!odbc_getResult( &putParmParm.pp_genParm, 
                            &pstmt->sqlca, pstmt->cQueryTimeout ))
                            return exitPutSeg(SQL_ERROR, &putParmParm, 
                                serviceParms);

                        if (putParmParm.pp_moreSegments == 0)
			                 pstmt->icolSent = i + 1;

                        /*
                        ** Restore the original data.
                        */
                        rgbData[0]=savech0; 
                        rgbData[1]=savech1; 
                        
                        rgbData += segmentlength * charlength;
                        /* 
                        ** Advance the pointer for the next iteration of
                        ** the loop.
                        */
                        rgbDataui2 = (u_i2 *)rgbData;

			if (pstmt->moreData && length == 0)
			    break;

                    }  /* end while (putParmParm.pp_moreSegments) */

                    /*
                    ** If more blob data is to be obtained from 
                    ** SQLPutData(), go back and get it.
                    */
                    if (pstmt->moreData && (papd->fStatus & COL_PUTDATA))
                        return exitPutSeg(SQL_NEED_DATA, &putParmParm,
                            serviceParms);
                    
                    /*
                    ** Since parameters have been sent, reset the 
                    ** parameter data buffer.
                    */
                    putParmParm.pp_parmCount = 0;  
                    putParmParm.pp_moreSegments = 0;
                                                  
                    break;

                case SQL_TYPE_TIMESTAMP:
                    if (isIngres && pstmt->fAPILevel < IIAPI_LEVEL_4)
                        ppd->dv_length = (II_UINT2)STlength(rgbData);
                    break;

                default:
                    break;
		}   /* end switch (fSqlType) */

           /*
            ** If the AT_EXEC column index points at the
            ** current column, send the current segment and exit.
            */
            if (papd->fStatus & COL_PUTDATA)
                break;

	}  /* end for loop through user parameters */

        if (rc != SQL_ERROR && putParmParm.pp_parmCount)
        {
            putParmParm.pp_moreSegments = 0;

            IIapi_putParms(&putParmParm);
            if (!odbc_getResult( &putParmParm.pp_genParm, 
                &pstmt->sqlca , pstmt->cQueryTimeout ))
            rc = SQL_ERROR;
            pstmt->icolSent = i + 1;
        }
        if ((pstmt->fCommand != CMD_SELECT) && (pstmt->fCommand != 
            CMD_EXECPROC) && (pstmt->icolSent >= pstmt->icolParmLast))
             odbc_getQInfo(pstmt);
        return exitPutSeg(rc, &putParmParm, serviceParms);
}

RETCODE exitPutSeg(RETCODE rc, IIAPI_PUTPARMPARM
    *putParmParm, IIAPI_PUTPARMPARM *serviceParms)
{
	odbc_free((VOID *)putParmParm->pp_parmData );
        if (serviceParms->pp_parmData)
        {
            odbc_free ((VOID *)serviceParms->pp_parmData);
            serviceParms->pp_parmData = NULL; 
            serviceParms->pp_parmCount = 0;
        }

	return(rc);
}
/*
** Name: odbc_prepare 
**
** Description:
**      This function prepares a SQL stmt 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_prepare(SESS* pSession, LPSTMT pstmt)
{
    IIAPI_QUERYPARM   	queryParm;
    BOOL rc = TRUE;
	char	szPrepareStr[50];
	char *  pQueryStr;
	char *  pTildeV;

    TRACE_INTL(pstmt,"odbc_prepare");
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_flags = 0;
	
	/* construct the query text */
    if (pstmt->iPreparedStmtID == 0)
       AllocPreparedStmtID(pstmt);

    if (pstmt->fCommand & CMD_SELECT)
       STprintf (szPrepareStr,"Prepare s%05.5d into sqlda from ",
           pstmt->iPreparedStmtID);
       else
       STprintf (szPrepareStr,"Prepare s%05.5d from ",
           pstmt->iPreparedStmtID);

    pQueryStr = odbc_malloc( STlength(pstmt->szSqlStr) + STlength(szPrepareStr) + 1);
    STcopy (szPrepareStr, pQueryStr);
    STcat  (pQueryStr,pstmt->szSqlStr);
	queryParm.qy_queryText = (VOID *) pQueryStr; 
	
	/* must chg ~V to ? if any */
	pTildeV = pQueryStr;
	while (pTildeV != NULL)
	{
		pTildeV = STstrindex(pTildeV," ~V ", 0, FALSE); 
		if (pTildeV)
			MEcopy("  ?", 3, pTildeV);
	}
	queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_stmtHandle = ( VOID * )NULL;
    
    IIapi_query( &queryParm );

    pSession->tranHandle = queryParm.qy_tranHandle;
    pstmt->stmtHandle = queryParm.qy_stmtHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, &pstmt->sqlca,pstmt->cQueryTimeout ))
		rc = FALSE;

	odbc_free( ( VOID * ) pQueryStr );

	return (rc);
}

/*
** Name: odbc_describe 
**
** Description:
**      This function describes a SQL stmt 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_describe(SESS* pSession, LPSTMT pstmt)
{
    IIAPI_QUERYPARM   	queryParm;
    BOOL rc = TRUE;
	char	szDescribeStr[50];

    TRACE_INTL(pstmt,"odbc_describe");
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_flags = 0;
	
	/* construct the query text */
	STprintf (szDescribeStr,"Describe s%05.5d ", pstmt->iPreparedStmtID);
	queryParm.qy_queryText = (VOID *) szDescribeStr; 
	queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = pSession->tranHandle;
    queryParm.qy_stmtHandle = ( VOID * )NULL;
    
    IIapi_query( &queryParm );

    pSession->tranHandle = queryParm.qy_tranHandle;
    pstmt->stmtHandle = queryParm.qy_stmtHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, 
        &pstmt->sqlca,pstmt->cQueryTimeout ))
        rc = FALSE;

    /* 
    ** Get the API descriptor and description.
    */
    if (rc)
        if (!odbc_getDescr(pstmt, TRUE, 0, FALSE))
            rc = FALSE;
    return (rc);
}
/*
** Name: odbc_describe_input
**
** Description:
**      This function describes parameters of a prepared query.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL odbc_describe_input(SESS* pSession, LPSTMT pstmt)
{
    IIAPI_QUERYPARM   	queryParm;
    BOOL rc = TRUE;
	char	szDescribeStr[50];

    TRACE_INTL(pstmt,"odbc_describe_input");
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_flags = 0;
	
    /* construct the query text */
    STprintf (szDescribeStr,"Describe input s%05.5d ", 
        pstmt->iPreparedStmtID);
    queryParm.qy_queryText = (VOID *) szDescribeStr; 
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = pSession->tranHandle;
    queryParm.qy_stmtHandle = ( VOID * )NULL;
    
    IIapi_query( &queryParm );

    pSession->tranHandle = queryParm.qy_tranHandle;
    pstmt->stmtHandle = queryParm.qy_stmtHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, 
        &pstmt->sqlca,pstmt->cQueryTimeout ))
        rc = FALSE;
    /* 
    ** Get the API descriptor and description.
    */
    if (rc)
        if (!odbc_getDescr(pstmt, TRUE, 0, TRUE))
            rc = FALSE;

    return (rc);
}
/*
** Name: odbc_execute 
**
** Description:
**      This function executes a prepared stmt.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
BOOL odbc_execute(SESS* pSession,
					LPSTMT  pstmt, IIAPI_QUERYTYPE apiQueryType)
{
    IIAPI_QUERYPARM   	queryParm;
    BOOL rc = TRUE;
	char	szExecute[50]; 

    TRACE_INTL(pstmt,"odbc_execute");
    /*
    ** provide input parameters for IIapi_query().
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = apiQueryType;
    
    /*
    **  Set the API scrolling flag if the cursor type is static
    **  or keyset-driven and the query type is IIAPI_QT_OPEN.
    */
    if ((pstmt->fAPILevel >= IIAPI_LEVEL_5) && 
        (pstmt->fCursorType != SQL_CURSOR_FORWARD_ONLY))
    {
        
        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
        {
            ErrState(SQL_HY000,pstmt, F_OD0168_IDS_ERR_SCRL_PROC);
            return FALSE;
        }

        if (apiQueryType == IIAPI_QT_OPEN)
            queryParm.qy_flags = IIAPI_QF_SCROLL;
        else
            queryParm.qy_flags = 0;
    }
    else
        queryParm.qy_flags = 0;

    if (apiQueryType == IIAPI_QT_EXEC)
	     STprintf (szExecute, "Execute s%05.5d", pstmt->iPreparedStmtID);
    else /* apiQueryType == IIAPI_QT_OPEN) */
         if (isCursorReadOnly(pstmt))
              STprintf (szExecute, "s%05.5d for readonly", pstmt->iPreparedStmtID);
         else STprintf (szExecute, "s%05.5d",              pstmt->iPreparedStmtID);
 
    queryParm.qy_queryText = &szExecute[0]; 

    if ( (pstmt->icolParmLast > 0) ||      /* count of parameter markers > 0 */
        (apiQueryType == IIAPI_QT_OPEN) ||
        (apiQueryType == IIAPI_QT_CURSOR_UPDATE) )
        queryParm.qy_parameters = TRUE; 
    else
        queryParm.qy_parameters = FALSE;

    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_stmtHandle = NULL;
    
    /*
    ** Invoke API function call.
    */
    IIapi_query( &queryParm );

    pSession->tranHandle = queryParm.qy_tranHandle;
    pstmt->stmtHandle = queryParm.qy_stmtHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, 
        &pstmt->sqlca,pstmt->cQueryTimeout ))
        return(FALSE); 

    return (rc);
}

/*
** Name: SetConnectParam 
**
** Description:
**      This function assigns a connection parameter and value to a 
**      connection 
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
static BOOL SetConnectParam(LPSQB pSqb, i4 paramID, VOID * paramValue)
{
    BOOL rc = TRUE;
	IIAPI_SETCONPRMPARM scp;
	
    scp.sc_genParm.gp_callback = NULL;
    scp.sc_genParm.gp_closure = NULL;
    scp.sc_connHandle = pSqb->pSession->connHandle; 
    scp.sc_paramID    = paramID; 
    scp.sc_paramValue = paramValue; 
    IIapi_setConnectParam(&scp);
         
    if (!odbc_getResult(&scp.sc_genParm, pSqb->pSqlca, 0))
		 rc = FALSE;

    if (scp.sc_genParm.gp_status == IIAPI_ST_SUCCESS)
	{ 
	     pSqb->pSession->connHandle = scp.sc_connHandle;	      
	}    
	 
	return(rc); 
}
/*
** Name: GetProcParamNames 
**
** Description:
**      Search and read the catalog for the procedure column names,
**      and fill in the proc IRD with the names.
**
*/
static BOOL GetProcParamNames(
    SESS               *pSession,
    LPSTMT              pstmt,
    LPDESC              pprocIPD,
    char               *szProcName,
    char               *szSchema)
{
    BOOL             rc = TRUE;
    BOOL             collecting = TRUE;
    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_GETCOLPARM    getColParm;
    LPDBC               pdbc = pstmt->pdbcOwner;
    LPDESCFLD           pprocipd;
    int                 precedence = 10;

    char szQuery[500];

    char szSql_Ingres[]="select trim(m.pp_name), p.dbp_owner, m.pp_number\
 from iiprocedure_parameter m, iiprocedure p\
 where (p.dbp_id=m.pp_procid1) and (p.dbp_idx=m.pp_procid2)\
 and (p.dbp_name='%s') order by p.dbp_owner, m.pp_number";

    char szSql_Ingres_Schema[]="select trim(m.pp_name), p.dbp_owner, m.pp_number\
 from iiprocedure_parameter m, iiprocedure p\
 where (p.dbp_id=m.pp_procid1) and (p.dbp_idx=m.pp_procid2)\
 and (p.dbp_name='%s') and (p.dbp_owner='%s') order by m.pp_number";

    char szSql_Gateway[]="select param_name, proc_owner, param_sequence\
 from iigwprocparams \
 where (proc_name='%s') order by param_sequence";

    char szSql_Gateway_Schema[]="select param_name, proc_owner, param_sequence\
 from iigwprocparams \
 where (proc_name='%s') and (proc_owner='%s') order by param_sequence";

    VOID * stmtSave;
    char * szSql;
    char * p;
    char * q;
    int    len;
    BOOL isGateway;
    short i;
    short  Count;

    stripOffQuotes(szProcName);

    TRACE_INTL(pstmt,"GetProcParamNames");
    /* Different tables for Ingres and Gateways */
    if (isServerClassAnIngresEngine(pstmt->pdbcOwner))
       {isGateway = FALSE;
        szSql     = (*szSchema) ? szSql_Ingres_Schema  : szSql_Ingres;
       }
    else
       {isGateway = TRUE;
        szSql     = (*szSchema) ? szSql_Gateway_Schema : szSql_Gateway;
       }

    if (*szSchema)  /* if schemaname explicitly specified in call, use it */
    {
        STprintf(szQuery,szSql,szProcName,szSchema);
    }
    else
    {   /* no explicit schema name provided */
        STprintf(szQuery,szSql,szProcName);
        CatGetDBconstants(pstmt->pdbcOwner, NULL);
       /* get dba_name and system_owner for resolving best match among
          duplicate proc names later */
    }
    /*
    ** provide input parameters for IIapi_query().
    */
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = (VOID *) szQuery; 
    queryParm.qy_parameters = FALSE;  
    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_flags = 0;
    queryParm.qy_stmtHandle = ( VOID * )NULL;
    
    IIapi_query( &queryParm );
    pSession->tranHandle = queryParm.qy_tranHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
        return(FALSE);

    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = queryParm.qy_stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;

    IIapi_getDescriptor( &getDescrParm );
    
    if (!odbc_getResult( &getDescrParm.gd_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
        return(FALSE);

    /*
    ** provide input parameters for IIapi_getColumns().
    */
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1;
    getColParm.gc_columnCount = 3;
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * )
        odbc_malloc( sizeof( IIAPI_DATAVALUE ) * getColParm.gc_columnCount);

    getColParm.gc_columnData[0].dv_value = (VOID *)odbc_malloc( OBJECT_NAME_SIZE + 3 );
    getColParm.gc_columnData[1].dv_value = (VOID *)odbc_malloc( OBJECT_NAME_SIZE + 3 );
    getColParm.gc_columnData[2].dv_value = (VOID *)odbc_malloc( sizeof (II_INT2) );
    
    getColParm.gc_stmtHandle = queryParm.qy_stmtHandle;
    getColParm.gc_moreSegments = 0;

    pprocipd = pprocIPD->pcol;    /* pprocipd->procedure IPD descriptor field */
    Count = 0;                    /* count of names read from catalog */

    for(;;)  /* convenient forever loop to break out of */
    {
        IIapi_getColumns( &getColParm );
        if (!odbc_getResult( &getColParm.gc_genParm, &pstmt->sqlca, pstmt->cQueryTimeout))
        {
            rc = FALSE;
            break;
        }

        if ( getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA )
             break;

        if (!(*szSchema)  &&  *(II_INT2*)(getColParm.gc_columnData[2].dv_value)==1)
           { /* no schemaname explicitly specified and 1st parm, search for best match */

             p   = getColParm.gc_columnData[1].dv_value;  /* p->schema name */
             len = getColParm.gc_columnData[1].dv_length;
             if (getDescrParm.gd_descriptor[1].ds_dataType == IIAPI_VCH_TYPE)
                {len = *((II_INT2*)p);
                 p=p+2;                    /* if VARCHAR, skip past 2-byte length */
                }
             for (q=p + len - 1; len && (*q-- == ' '); len--); /* back trim spaces */
             if (len > 32)  len = 32;  /* safety check */
             p[len] = '\0';  /* p -> null-termed schema name in parameter name row */

             collecting = FALSE;  /* stop collecting the parms */
             /* order of precedence (with 0 being the highest precedence):
                   0  explicit schema specified
                   1  username
                   2  dbaname that owns the database
                   3  system owner name (usually '$ingres')
                   4  first schema name found among the procedures */
             if      (STequal(p, pdbc->szUsername)           &&  precedence > 1)
                      { collecting = TRUE;                       precedence = 1;}
             else if (STequal(p, pdbc->szDBAname)            &&  precedence > 2)
                      { collecting = TRUE;                       precedence = 2;}
             else if (STequal(p, pdbc->szSystemOwnername)    &&  precedence > 3)
                      { collecting = TRUE;                       precedence = 3;}
             else if ( /* 1st anything found */                  precedence > 4)
                      { collecting = TRUE;                       precedence = 4;}

             pprocipd = pprocIPD->pcol; /* pprocipd->procedure IPD descriptor field */
             Count = 0;                 /* reset count of names read from catalog */

           } /* end if searching for best schema */

        if (!collecting)  /* iterate to next row if discarding the rows */
             continue;    /*   and not collecting the best parms */

        if (isGateway)  /* if gateway, need to convert char to varchar */
           {char s[OBJECT_NAME_SIZE+1];
            i2   cb =         getColParm.gc_columnData[0].dv_length;
            char *p = (char *)getColParm.gc_columnData[0].dv_value;
            STcopy (p, s);
            /* trim trailing blanks */
            cb = (i2) STtrmwhite(s); 
            MEcopy(s,   cb, (char *)getColParm.gc_columnData[0].dv_value+2);
            MEcopy((PTR)(&cb),  2,
                            (char *)getColParm.gc_columnData[0].dv_value+0);
           }

        do {
            pprocipd++;  /* increment to next field in descriptor fields */
            Count++;
           }  while (Count <= pprocIPD->Count  &&  /* skip the schema.procname */
                     pprocipd->ParameterType == SQL_PARAM_TYPE_PROCNAME);

        if (Count <= pprocIPD->Count)
           {
            i = * (II_INT2 *) (getColParm.gc_columnData[0].dv_value);
            STlcopy ((char *)getColParm.gc_columnData[0].dv_value+2, pprocipd->Name, i);
            pprocipd->Name[i] = '\0';
           }
    }  /* end convenient for loop */
    
    for( i = 0; i <= getColParm.gc_columnCount; i++)
    {
        odbc_free((VOID *) getColParm.gc_columnData[i].dv_value); 
    }

    odbc_free( ( VOID * )getColParm.gc_columnData );

    stmtSave = pstmt->stmtHandle;
	pstmt->stmtHandle = queryParm.qy_stmtHandle;
	
	if (!odbc_getQInfo( pstmt ))
		rc = FALSE;

    odbc_close( pstmt );
    pstmt->stmtHandle = stmtSave; 

	return(rc);
}
/*
** Name: odbc_retrieveByRefParms 
**
** Description:
**      Retrieves descriptors and values of BYREF parameter 
**
*/
static BOOL odbc_retrieveByRefParms(LPSTMT  pstmt)
{
    BOOL            rc = TRUE;
    IIAPI_GETCOLPARM   getColParm;
    II_INT2            i, j;
    DESCFLD            apd = {0}; /* application parameter descriptor for target */
    DESCFLD            ird = {0}; /* implementation row descriptor for source*/
    LPDESCFLD          papd;
    LPDESCFLD          pird = &ird;
    LPDBC              pdbc = pstmt->pdbcOwner;
    SQLSMALLINT        APDCount = pstmt->pAPD->Count;
    SQLSMALLINT        IRDCount = pstmt->pIRD->Count;
    char               *rgbData;
#ifdef IIAPI_NVCH_TYPE
    u_i2               *rgbDataui2;
#endif

    TRACE_INTL(pstmt,"odbc_retrieveByRefParms");
    if (IRDCount == 0)       /* return if no result columns */
        return(rc);

    papd = pstmt->pAPD->pcol + 1;    /* pard->1st APD user field after bookmark */
    for( i = 0; i < APDCount; i++, papd++ )
    {
        if  (papd->ParameterType != SQL_PARAM_INPUT_OUTPUT && papd->ParameterType 
            != SQL_PARAM_OUTPUT)
            continue;   /* skip to next column if not in/out */
        break;          /* in/out found!  break out of loop  */
    }    /* end for loop */

    if (i >= APDCount)  /* return if no in/out parameters */
       {pstmt->fStatus |= STMT_OPEN; /* result set created */
        return(rc);                  /* since there were result columns */
       }                             /* without in/out parameter columns */

    /* get BYREF parameters values */
    getColParm.gc_genParm.gp_callback = NULL;
    getColParm.gc_genParm.gp_closure = NULL;
    getColParm.gc_rowCount = 1; 
    getColParm.gc_rowsReturned = 0;
    getColParm.gc_columnCount = IRDCount;
    getColParm.gc_columnData = ( IIAPI_DATAVALUE * )
        odbc_malloc( sizeof( IIAPI_DATAVALUE ) *
                      getColParm.gc_rowCount * getColParm.gc_columnCount );

    /* loop thru and allocate memory for the result columns going into the in/out parms*/
    pird = pstmt->pIRD->pcol + 1;    /* pard->1st IRD field after bookmark */
    for( i = 0; i < IRDCount; i++, pird++)
    {
        getColParm.gc_columnData[i].dv_value = (VOID *)
            odbc_malloc(pird->OctetLength);
    }

    getColParm.gc_stmtHandle = pstmt->stmtHandle;
    getColParm.gc_moreSegments = 0;  
    
    IIapi_getColumns( &getColParm );  /* get the in/out data */

    if (!odbc_getResult( &getColParm.gc_genParm, &pstmt->sqlca, pstmt->cQueryTimeout ))
    {
        rc = FALSE;
    }

    /* see if any null value returned */
    papd = pstmt->pAPD->pcol + 1;    /* pard->1st APD user field after bookmark */
    pird = pstmt->pIRD->pcol + 1;    /* pard->1st IRD field after bookmark */

    for( i=0, j=0; i < APDCount  &&  j < IRDCount; i++, papd++ )
    {
        if  (papd->ParameterType != SQL_PARAM_INPUT_OUTPUT &&
             papd->ParameterType != SQL_PARAM_OUTPUT)
            continue;   /* skip to next column if not in/out */

        if (getColParm.gc_columnData[j].dv_null)
        {
            SQLINTEGER  * IndicatorPtr;     /* Ptr to NULL indicator */

            GetBoundDataPtrs(pstmt, Type_APD, pstmt->pAPD, papd,
                             NULL,
                             NULL,
                             &IndicatorPtr);
                /*  Get IndicatorPtr
                    after adjustment for bind offset, row number,
                    and row or column binding. */
            if (IndicatorPtr)
               *IndicatorPtr = SQL_NULL_DATA;
        }
        else
        {
            rgbData = getColParm.gc_columnData[j].dv_value;
# ifdef IIAPI_NCHA_TYPE
            if (pird->fIngApiType == IIAPI_NCHA_TYPE)  /* if NCHAR */
            {
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4((u_i2*)rgbData, (u_i4*)rgbData,
                                             (u_i4)(pird->OctetLength/4));
            }
            else
            if (pird->fIngApiType == IIAPI_NVCH_TYPE)  /* if NVARCHAR */
            {
                rgbDataui2 = (u_i2*)rgbData;
                if (sizeof(SQLWCHAR) == 4)  /* need UCS2->UCS4 conversion? */
                   ConvertUCS2ToUCS4(rgbDataui2+1, (u_i4*)(rgbDataui2+1),
                                              *rgbDataui2);
                *rgbDataui2 = *rgbDataui2 * sizeof(SQLWCHAR);
                         /* convert NVARCHAR character length to byte length */
            }
# endif
            /* use info from get descriptor to construct a temp descriptor
               to move the byref data back to the parameter (desc by APD)  */
            /* move data to papd->DataPtr (SQLBindParameter(rgbValue)) */
            MEcopy ((PTR)papd, sizeof(apd), (PTR)&apd );
            apd.cbPartOffset = 0;

            GetBoundDataPtrs(pstmt, Type_APD, pstmt->pAPD, papd,
                             &apd.DataPtr,
                             &apd.OctetLengthPtr,
                             &apd.IndicatorPtr);
                /*  Get DataPtr, OctetLengthPtr, and IndicatorPtr
                    after adjustment for bind offset, row number,
                    and row or column binding. */

            rc = GetColumn(pstmt, &apd, pird, rgbData);
        }

        /* move on to next result column for the in/out parm */
        j++;
        pird++;

    } /* end for loop */

    for( i = 0; i < getColParm.gc_columnCount; i++ )
    {
        odbc_free(getColParm.gc_columnData[i].dv_value);
    }

    odbc_free((VOID *) getColParm.gc_columnData);

    return(rc);
}

/*
** GetServerQuery
**
**    Query to get information about capabilities of the server.
**
*/
static BOOL GetServerQuery(SESS * pSession, LPSQLCA psqlca, LPDBC pdbc,
                               IIAPI_GETCOLPARM * pgetColParm, 
                               char * szQuery)
{
    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CLOSEPARM     closeParm;
    IIAPI_GETDESCRPARM  getDescrParm;

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = (VOID *) szQuery; 
    queryParm.qy_parameters = FALSE;  
    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_stmtHandle = ( VOID * )NULL;
    queryParm.qy_flags = 0;
 
    IIapi_query( &queryParm );
    pSession->tranHandle = queryParm.qy_tranHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, psqlca, pdbc->cQueryTimeout ))
        return(FALSE);

    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = queryParm.qy_stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;
    IIapi_getDescriptor( &getDescrParm );
    
    if (!odbc_getResult( &getDescrParm.gd_genParm, psqlca, pdbc->cQueryTimeout ))
        return(FALSE);

    /*
    ** provide input parameters for IIapi_getColumns().
    */
    pgetColParm->gc_genParm.gp_callback = NULL;
    pgetColParm->gc_genParm.gp_closure = NULL;
    pgetColParm->gc_rowCount = 1;
    pgetColParm->gc_stmtHandle = queryParm.qy_stmtHandle;
    pgetColParm->gc_moreSegments = 0;
    
    IIapi_getColumns( pgetColParm );
    odbc_getResult( &pgetColParm->gc_genParm, psqlca, pdbc->cQueryTimeout);

    /*
    ** Provide input parameters for IIapi_getQueryInfo().
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    odbc_getResult( &getQInfoParm.gq_genParm, psqlca, pdbc->cQueryTimeout);

    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;
    IIapi_close( &closeParm );
    if (!odbc_getResult( &closeParm.cl_genParm, psqlca, pdbc->cQueryTimeout))
        return(FALSE);

    return(TRUE);
}

/*
** GetServerInfo
**
**    Get information about capabilities of the server.
**
**    On output: pdbc->fRelease = 640000, 1200000, or 2500000, etc. 
**                              for 6.4, 1.2, 2.5, etc.
**               pdbc->fOptions has OPT_CONVERT_3_PART_NAMES set if 6.4
**               pdbc->szUsername has dbmsinfo('username') if INGRES
*/
BOOL GetServerInfo(SESS * pSession, LPSQLCA psqlca, LPDBC pdbc)
{
    IIAPI_GETCOLPARM    getColParm;
    IIAPI_DATAVALUE     datavalues[10];
    CHAR     *  p;
    SIZE_TYPE   i;
    CHAR        szBuf[100];
    char        part1[5] = "0";  /* part before decimal place */
    char        part2[5] = "0";  /* part between decimal and slash */
    char        part3[5] = "0";  /* part after slash */
    char      * part     = &part1[0];
    BOOL        notrelease6x = TRUE;
    BOOL        isVectorwise = FALSE;
    SIZE_TYPE   digitlimit=2;

    char szRelease[100];
    char szUsername[64] = "";
    char szUnicode [8]  = "";

    memset(szRelease,0,sizeof(szRelease));
    memset(szUsername,0,sizeof(szUsername));

    if (isServerClassINGRES(pdbc))
       {
        getColParm.gc_columnCount = 3;
        getColParm.gc_columnData = &datavalues[0];
        getColParm.gc_columnData[0].dv_value = szRelease;
        getColParm.gc_columnData[1].dv_value = szUsername;
        getColParm.gc_columnData[2].dv_value = szUnicode;
        GetServerQuery(pSession, psqlca, pdbc, &getColParm, 
"select dbmsinfo('_version'), dbmsinfo('username'), dbmsinfo('unicode_level')");
       }
    else   /* gateway needs two separater queries */
       {
        getColParm.gc_columnCount = 1;
        getColParm.gc_columnData = &datavalues[0];
        getColParm.gc_columnData[0].dv_value = szRelease;
        GetServerQuery(pSession, psqlca, pdbc, &getColParm, 
                   "select dbmsinfo('_version')");
      /*  delay getting username until GetProcParamNames 
        getColParm.gc_columnCount = 1;
        getColParm.gc_columnData = &datavalues[0];
        getColParm.gc_columnData[0].dv_value = szUsername;
        GetServerQuery(pSession, psqlca, pdbc, &getColParm, 
                   "select dbmsinfo('username')");
      */
       }

	pdbc->fRelease = 0;  /* init to 0 in case we are confused by the string*/
	p = &szRelease[2];   /* p points past 2-byte length field to release string */
                         /* e.g. "OI 1.2/01 (su4.us5/78)"
                                 "6.2/01 (mvs.mxa/09)"
                                 "II 2.5/0006 (su4.us5/39)"
                                 "EC DB2 2.2/9904"  */
	if (*p == 'V')
            isVectorwise = TRUE;

    if (*p=='6')
       {
			/*	Since Ingres 6.x does not allow 3 level qualifier
				for a table name, we should always convert 
				owner.tablename.columnname to tablename.columnname */
			pdbc->fOptions |= OPT_CONVERT_3_PART_NAMES;
		
		/* change "6.2/01" to 6.2/1 "  so that 621 < Ingres release numbers */
			*(p+4) = *(p+5);
			*(p+5) = ' ';
		notrelease6x = FALSE;
		part = part2;               /* 00.62.0001 */
       }
    else
	   while (*p)    /* skip leading spaces and find 1st real digit */
	   {
	    while(*p  &&  *p==' ')   /* leading spaces */
	        p++;

	    if (CMdigit(p))          /* if found digit, all done! */
	        break;

	    while(*p  &&  *p!=' ')   /* skip over "DB2" */
	        p++;

	   }  /* end while (*p) */

	i = 0;
	while ( (*p != ' ') && (*p != '(') && (*p !='\0') )  /* stop on '(' or ' ' */
	{
		if (*p=='.'  &&  notrelease6x)  /* if decimal-point */
		   { digitlimit=2;
		     part = part2;
		     i=0;
		   }
		else
		if (*p=='/')                    /* maintenance level */
		   { digitlimit=4;
		     part = part3;
		     i=0;
		   }

		if (CMdigit (p))    /* build up first 4 digits in szBuf */
		{
		    if (i < digitlimit)
		       { part[i] = *p;
		         i++;
		       }
		}
		CMnext(p);
	}

	STcopy("00000000",szBuf);

	i = STlength(part1);
	memcpy(szBuf+2-i, part1, i);   /* release number */

	i = STlength(part2);
	memcpy(szBuf+2,   part2, i);   /* point release number */

	i = STlength(part3);
	memcpy(szBuf+8-i, part3, i);   /* maintenance level */

	pdbc->fRelease = atoi(szBuf);

	p = &szUsername[2];   /* p points past 2-byte length field to username */
	STcopy(p, pdbc->szUsername);  /* save dbmsinfo('username') */

	p = &szUnicode[2];   /* p points past 2-byte length field to unicode level */
	if (*p == '1')
	    pdbc->fOptions |= OPT_UNICODE;  /* server supports NCHAR */

        /*
        ** If Vectorwise, make the release level the same as Ingres 10.0.
        ** Ignore "Send Date/Time as Ingres Date" and "Return empty string 
        ** DATE values as NULL configuration settings.
        */
        if (isVectorwise)
        {
            pdbc->fRelease = 10000000;
            pdbc->fOptions &= ~OPT_INGRESDATE;
            pdbc->fOptions &= ~OPT_BLANKDATEisNULL;
        }

	return(TRUE);
}


/*
** odbc_AutoCommit 
*/
RETCODE odbc_AutoCommit(LPSQB pSqb, BOOL bFlag)
{
	IIAPI_AUTOPARM	autoParm;
    SESS * pSession = pSqb->pSession;

	if (pSession->connHandle == NULL)
		return(SQL_SUCCESS);

	if (pSession->XAtranHandle != NULL)   /* don't autocommit if we're */ 
		return(SQL_SUCCESS);              /*    enlisted in DTC XA transaction */

	if (bFlag)  
	{
		/* 
		   set autocommit on. If api autocommit has been issued already,
		   then just return. Api would turn autocommit off if we call it
		   again. 
		*/
		if (pSession->fStatus & SESS_AUTOCOMMIT_ON_ISSUED)
                {
			return(SQL_SUCCESS);
                }

        if (pSession->tranHandle)  /* if transaction open, commit it */
        {
            odbc_commitTran(&(pSession->tranHandle), pSqb->pSqlca);
            pSession->fStatus &= ~SESS_COMMIT_NEEDED;
        }

		autoParm.ac_connHandle = (VOID *) (pSession->connHandle); 
	}
	else		
	{
		/*
		   set auto commit off. If autocommit is off then don't call api.
		   Otherwise, api would get upset.
	    */
		if (!(pSession->fStatus & SESS_AUTOCOMMIT_ON_ISSUED))
			return(SQL_SUCCESS);
		autoParm.ac_connHandle = NULL; 
	}

    autoParm.ac_tranHandle          = pSession->tranHandle;
    autoParm.ac_genParm.gp_callback = NULL;
    autoParm.ac_genParm.gp_closure  = NULL;

    IIapi_autocommit( &autoParm );

    if (!odbc_getResult( &autoParm.ac_genParm, pSqb->pSqlca, ((LPDBC)(pSqb->pDbc))->cQueryTimeout ))
		return(SQL_ERROR);

    pSession->tranHandle = autoParm.ac_tranHandle;
	if (bFlag)  
		pSession->fStatus |= SESS_AUTOCOMMIT_ON_ISSUED; 
	else
		pSession->fStatus &= ~SESS_AUTOCOMMIT_ON_ISSUED; 
	return(SQL_SUCCESS);
}
/*
*   Function:    SQL
*
*   Description: Set up parm list and issue SQL call to IDMSQCLI.
*
*   Input:       1. Pointer to QCLI parmlist structure
*                   (SQLCA pointer)
*                2. IDMS SQL command code, required (sqlcmd.h).
*                3. Section number or 0.
*                4. Cursor number (FETCH only) or 0.
*                5. Pointer to fetch buffer, parm buffer,
*                   syntax, dbname, or NULL.
*                6. Number of rows to fetch,
*                   length of syntax, or NULL.
*
*   Returns:     SQLCODE value.
*
*   Change History
*   --------------
*
*   date        programmer          description
*
*   01/09/1997  Jean Teng           API connect/disconnect
**     23-Jan-2008 (Ralph Loen) Bug 119800
**         In the SQLEXEC_DIRECT case, invoke odbc_getSvcParms() if
**         the statement is DELETE or UPDATE and for an updatable cursor.
*
*/
RETCODE SqlToIngresAPI
(
   LPSQB          pSqb,               /* -->QCLI blocks          */
   i2             lCmd,               /* IDMS SQL command        */
   LPSTR          pBuf                /* -->buffer/syntax        */
)
{
    II_INT2    fOptions;
	LPSTMT     pstmt; 
	LPDBC      pdbc;
	IIAPI_QUERYTYPE apiQueryType;
	BOOL    rc;
	i4    templong;
	CHAR    szWithOption[LEN_GATEWAY_OPTION+1];
	CHAR    *szRole;
	CHAR    szSupportIIDECIMAL[LEN_IIDECIMAL+1] = "N";
	CHAR       szIngSetName[50];
	char       *pfor_readonly_added=NULL;
	LPSTR      pDSN;               /* -->DSN name if connect */
	char       dsnVNODE[LEN_NODE + 1];
	char       dsnDATABASE[LEN_DBNAME + 1];
	char       dsnSERVERTYPE[LEN_SERVERTYPE + 1];
        QRY_CB     *qry_cb;
        IIAPI_SETENVPRMPARM se;

    
    pstmt = (LPSTMT) pSqb->pStmt;
	if (pstmt)
    {
        qry_cb = &pstmt->qry_cb;
    }
    if (lCmd == SQLCANCEL)   /* make CANCEL very shallow to avoid collisions */
       {                     /* with the function that is being cancelled */
        odbc_cancel(pstmt);
        return SQL_SUCCESS;
       }

    fOptions = pSqb->Options;      /* Save and reset to let   */ 
    pSqb->Options = 0;             /* ODBC know we got here...*/
    pSqb->pSqlca->SQLCODE = SQL_SUCCESS;

    /*
    **  Convert SQL function arguments into DSI parameters:
    */
    switch (lCmd) 
    {
    case SQLCONNECT:

        pdbc = (LPDBC) pSqb->pDbc;
        pDSN = pBuf;                        /* -> DSN name */

        /* process II_DATE_FORMAT */
        templong=ProcessII_DATE_FORMAT();
        pSqb->pSession->connHandle = pdbc->envHndl;
        rc=SetConnectParam(pSqb, IIAPI_CP_DATE_FORMAT, 
                               &templong);           

        if (pdbc->fOptions & OPT_CURSOR_LOOP)  /* if using cursors, default */
            pdbc->fActiveStatementsMax=0;   /*  to unlimited active statements*/ 
        else
            pdbc->fActiveStatementsMax=1;   /* else to 1 active statement */

        if (pDSN)  /* if DSN=, not DRIVER= */
           {
             SQLGetPrivateProfileString (pDSN,"WithOption","",szWithOption, 
                 	               LEN_GATEWAY_OPTION + 1,ODBC_INI);
             if (*szWithOption)   /* if Gateway WITH parameters, send them */
                { rc=SetConnectParam(pSqb, IIAPI_CP_GATEWAY_PARM, szWithOption);
                }

             if (!(pdbc->fOptionsSet & OPT_CURSOR_LOOP)) /* if not already set */
                 SQLGetPrivateProfileString (pDSN,"SelectLoops","Y",szWithOption, 
                                            LEN_SELECTLOOPS + 1,ODBC_INI);
             else /* cursor loop or select loop option set by connect string*/
                if (pdbc->fOptions & OPT_CURSOR_LOOP)
                     *szWithOption='N';  /* slave off the connection string */
                else *szWithOption='Y';

             if (*szWithOption=='N') /* if not using select loops, then */
                {pdbc->fOptions |= OPT_CURSOR_LOOP; /* use cursor loops */
                 pdbc->fActiveStatementsMax=0;      /* unlimited active stmts */
                }
             else         /* else using select loop (normally just 1 active) */
                {SQLGetPrivateProfileString(pDSN,"GetInfoActiveStatements","1",
                          szWithOption,2,ODBC_INI);  /* check if .ini override */
                 if (*szWithOption=='1')
                    pdbc->fActiveStatementsMax=1; /* 1 active statement */
                 else
                    pdbc->fActiveStatementsMax=0; /* unlimited active statements */
                }

             SQLGetPrivateProfileString (pDSN,"ConvertThreePartNames","N",szWithOption, 
                                            6,ODBC_INI);
             if (*szWithOption=='Y') /* if need to convert owner.table.colname */
                {pdbc->fOptions |= OPT_CONVERT_3_PART_NAMES; /* convert 3-part names*/
                }
             SQLGetPrivateProfileString (pDSN,"UseSysTables","N",szWithOption, 
                                            6,ODBC_INI);
             if (*szWithOption=='Y') /* if need to convert owner.table.colname */
                {pdbc->fOptions |= OPT_USESYSTABLES; /* Include tables beginning with SYS% */
                }

             if (!(pdbc->fOptionsSet & OPT_BLANKDATEisNULL)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"BlankDate","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='N'  ||  *szWithOption=='n')  /* BlankDate=NULL */
                    {pdbc->fOptions |= OPT_BLANKDATEisNULL; /* return blank dates as NULL*/
                    }
                 pdbc->fOptionsSet |= OPT_BLANKDATEisNULL; /* indicate option set*/
                }

             if (!(pdbc->fOptionsSet & OPT_DATE1582isNULL)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"Date1582","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='N'  ||  *szWithOption=='n')  /* Date1582=NULL */
                    {pdbc->fOptions |= OPT_DATE1582isNULL; /* return blank dates as NULL*/
                    }
                 pdbc->fOptionsSet |= OPT_DATE1582isNULL; /* indicate option set*/
                }

             if (!(pdbc->fOptionsSet & OPT_CATCONNECT)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"CatConnect","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='Y'  ||  *szWithOption=='y')  /* CatConnect=Y */
                    {pdbc->fOptions |= OPT_CATCONNECT; /* force separate sess for catalog*/
                    }
                 pdbc->fOptionsSet |= OPT_CATCONNECT; /* indicate option set*/
                }

             if (!(pdbc->fOptionsSet & OPT_NUMOVERFLOW)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"Numeric_overflow","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='I'  ||  *szWithOption=='i')
                    {pdbc->fOptions |= OPT_NUMOVERFLOW; /* Numeric_overflow=ignore */
                    }
                 pdbc->fOptionsSet |= OPT_NUMOVERFLOW; /* indicate option set*/
                }
                if (!(pdbc->fOptionsSet & OPT_STRINGTRUNCATION)) /* if not already set */
                {
                    SQLGetPrivateProfileString (pDSN,"StringTruncation","",szWithOption, 
                        LEN_GATEWAY_OPTION,ODBC_INI);
                    if (*szWithOption=='Y'  ||  *szWithOption=='y')
                    {
                        pdbc->fOptions |= OPT_STRINGTRUNCATION; /* StringTruncation=Y */
                    }
                    pdbc->fOptionsSet |= OPT_STRINGTRUNCATION; /* indicate option set*/
                }

             if (!(pdbc->fOptionsSet & OPT_CATSCHEMA_IS_NULL)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"CatSchemaNULL","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='Y'  ||  *szWithOption=='y')  /* CatSchemaNULL=Y */
                    {pdbc->fOptions |= OPT_CATSCHEMA_IS_NULL; /* NULL TABLE_SCHEM */
                    }
                 pdbc->fOptionsSet |= OPT_CATSCHEMA_IS_NULL; /* indicate option set*/
                }

             if (!(pdbc->fOptionsSet & OPT_DISABLEUNDERSCORE)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"DisableCatUnderscore","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='Y'  ||  *szWithOption=='y')
                    {pdbc->fOptions |= OPT_DISABLEUNDERSCORE; /* set the option */
                    }
                 pdbc->fOptionsSet |= OPT_DISABLEUNDERSCORE; /* indicate option set*/
                }
             if (!(pdbc->fOptionsSet & OPT_ALLOW_DBPROC_UPDATE)) /* if not already set */
                {SQLGetPrivateProfileString (pDSN,"AllowProcedureUpdate","",szWithOption, 
                                LEN_GATEWAY_OPTION,ODBC_INI);
                 if (*szWithOption=='Y'  ||  *szWithOption=='y')  
                    {pdbc->fOptions |= OPT_ALLOW_DBPROC_UPDATE; 
                    }
                 pdbc->fOptionsSet |= OPT_ALLOW_DBPROC_UPDATE; /* indicate option set*/
                }

                if (!pdbc->multibyteFillChar)
                {
                     SQLGetPrivateProfileString (pDSN,"MultibyteFillChar","",
                         szWithOption, 3, ODBC_INI);

                     if (*szWithOption)
                         pdbc->multibyteFillChar = *szWithOption;
                     else
                         pdbc->multibyteFillChar = ' ';
                }
                if (!(pdbc->fOptionsSet & OPT_CONVERTINT8TOINT4))
                {
                    SQLGetPrivateProfileString (pDSN,"ConvertInt8ToInt4","",
                        szWithOption, 6,ODBC_INI);
                    if (*szWithOption=='Y' ||  *szWithOption=='y') 
                        pdbc->fOptions |= OPT_CONVERTINT8TOINT4; 
                }

                if (!(pdbc->fOptionsSet & OPT_ALLOWUPDATE)) 
                {
                    SQLGetPrivateProfileString (pDSN,"AllowUpdate","",
                        szWithOption, 6,ODBC_INI);
                    if (*szWithOption=='Y' ||  *szWithOption=='y') 
                        pdbc->fOptions |= OPT_ALLOWUPDATE; 
                }
                if (!(pdbc->fOptionsSet & OPT_DEFAULTTOCHAR)) 
                {
                    SQLGetPrivateProfileString (pDSN,"DefaultToChar","",
                        szWithOption, 6,ODBC_INI);
                    if (*szWithOption=='Y' ||  *szWithOption=='y') 
                        pdbc->fOptions |= OPT_DEFAULTTOCHAR; 
                }
                if (!(pdbc->fOptionsSet & OPT_INGRESDATE)) 
                {
                    SQLGetPrivateProfileString (pDSN,"IngresDate","",
                        szWithOption, 10,ODBC_INI);
                    if (*szWithOption=='Y' ||  *szWithOption=='y') 
                        pdbc->fOptions |= OPT_INGRESDATE; 
                }

             if (!(pdbc->fOptionsSet & OPT_DECIMALPT))
             {
                 SQLGetPrivateProfileString (pDSN,"SupportIIDECIMAL","N",
                     szSupportIIDECIMAL, LEN_IIDECIMAL + 1, ODBC_INI);
                 if (*szSupportIIDECIMAL =='Y' ||  *szSupportIIDECIMAL =='y') 
                     pdbc->fOptions |= OPT_DECIMALPT; 
             }

             SQLGetPrivateProfileString (pDSN,"Server","",dsnVNODE,
                               sizeof(dsnVNODE),ODBC_INI);
             SQLGetPrivateProfileString (pDSN,"Database","",dsnDATABASE,
                               sizeof(dsnDATABASE),ODBC_INI);
             SQLGetPrivateProfileString (pDSN,"ServerType","INGRES",dsnSERVERTYPE,
                               sizeof(dsnSERVERTYPE),ODBC_INI);

                   /* if not filled in from user's connection string then
                      fill in from DSN definition */
             if (*pdbc->szVNODE      == '\0')
                STcopy(dsnVNODE,      pdbc->szVNODE);
             if (*pdbc->szDATABASE   == '\0')
                STcopy(dsnDATABASE,   pdbc->szDATABASE);
             if (*pdbc->szSERVERTYPE == '\0')
                STcopy(dsnSERVERTYPE, pdbc->szSERVERTYPE);

           }  /* end if (pDSN) */

        if (*pdbc->szSERVERTYPE == '\0')          /* if still no server type, */
            STcopy("INGRES", pdbc->szSERVERTYPE); /*   default to "INGRES" */
		
		CVupper(pdbc->szSERVERTYPE);

		/* any role name/pwd specified ? */
		if (STlength(pdbc->szRoleName) > 0) 
		{
			szRole=MEreqmem(0, LEN_ROLENAME + LEN_ROLEPWD + 2, TRUE, NULL);
			if (szRole)
			{
				STcopy (pdbc->szRoleName, szRole);
				if (pdbc->cbRolePWD)
				{
					STcat(szRole,"/");
					pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /* decrpts the pw */  
					STcat(szRole,pdbc->szRolePWD); 
					pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /* encrypts the pw */
				}
				rc=SetConnectParam(pSqb, IIAPI_CP_APP_ID, szRole);
				MEfree((PTR)szRole);
			}
		}

		templong=ProcessII_DATE_CENTURY_BOUNDARY();
		rc=SetConnectParam(pSqb, IIAPI_CP_CENTURY_BOUNDARY,
		                  &templong);
 		if (pdbc->fOptions & OPT_DECIMALPT)
		    STcopy(ProcessII_DECIMAL(), pdbc->szDecimal);
                se.se_envHandle = pdbc->envHndl;
                se.se_paramID = IIAPI_EP_DECIMAL_CHAR;
                se.se_paramValue = pdbc->szDecimal;
                IIapi_setEnvParam(&se);

		if (*pdbc->szGroup != '\0')    /* user's Group ID? */
		{
		    rc=SetConnectParam(pSqb, IIAPI_CP_GROUP_ID, pdbc->szGroup);
		}

		if (pdbc->fOptions & OPT_NUMOVERFLOW)    /* numeric_overflow=ignore? */
		{
		    rc=SetConnectParam(pSqb, IIAPI_CP_MATH_EXCP, IIAPI_CPV_RET_IGNORE);
		}
                if (pdbc->fOptions & OPT_STRINGTRUNCATION)    /* numeric_overflow=ignore? */
		{
		    rc=SetConnectParam(pSqb, IIAPI_CP_STRING_TRUNC, IIAPI_CPV_RET_FATAL);
		}
                if (*pdbc->szDBMS_PWD != '\0')
                {        
                    rc=SetConnectParam(pSqb, IIAPI_CP_DBMS_PASSWORD, 
                        pdbc->szDBMS_PWD);
                }
		if (odbc_conn(pdbc,pSqb->pSession,pSqb->pSqlca))
		{
			pSqb->pSqlca->SQLCODE = SQL_SUCCESS;

			SetServerClass(pdbc);             /* Set up fServerClass in DBC */

			/* process ING_SYSTEM_SET */
			STcopy ("ING_SYSTEM_SET", szIngSetName);
			ProcessIngSet(pSqb->pSession,pSqb->pSqlca,
					pdbc->cQueryTimeout,szIngSetName,pdbc);
			
			/* process ING_SET_DBNAME */
		    STprintf(szIngSetName,"ING_SET_%s",pdbc->szDATABASE);
			ProcessIngSet(pSqb->pSession,pSqb->pSqlca,
					pdbc->cQueryTimeout,szIngSetName,pdbc);

			/* process ING_SET */
			STcopy ("ING_SET", szIngSetName);
			ProcessIngSet(pSqb->pSession,pSqb->pSqlca,
					pdbc->cQueryTimeout,szIngSetName,pdbc);
			

            GetCapabilitiesFromDSN(pDSN, pdbc);
                   /* If a DSN name and gateway, use cache.  In order to
                      improve performance at connection time, the ODBC DSN
                      configuration DLL will cache certain servertype information
                      to avoid issuing connection queries to get keep getting
                      the same answers. */

            if (!pdbc->fRelease)
               {GetServerInfo(pSqb->pSession, pSqb->pSqlca, pdbc);
               }

            CatGetDBcapabilities(pdbc, NULL); /* get DB_NAME_CASE and maybe name lens from 
                                               iidbcapabilities */
            CatGetTableNameLength(pdbc, NULL); /* get any unresolved name lengths now */

            odbc_commitTran(&(pSqb->pSession->tranHandle),pSqb->pSqlca);
            pSqb->pSession->fStatus &= ~SESS_COMMIT_NEEDED;

  /*        don't turn autocommit on right now because it will
            start a transaction handle that will block SetTransaction().

			** set auto commit if necessary **
            if (pdbc->fOptions & OPT_AUTOCOMMIT)
				pSqb->pSqlca->SQLCODE = odbc_AutoCommit(pSqb,TRUE);
  */ 
		}
	    else
                {
			pSqb->pSqlca->SQLCODE = SQL_ERROR;
                        odbc_disconn(pSqb->pSession, pSqb->pSqlca);
                }
        break;

    case SQLSUSPEND:

        break;

    case SQLEXECUTE:

	pstmt->QueryHandle = NULL;

        if (pstmt->fCommand == CMD_SELECT && !isApiCursorNeeded(pstmt))
        {
	    qry_cb->apiQueryType = IIAPI_QT_QUERY;
            qry_cb->sequence = QRY_QUERY;
            qry_cb->sequence_type = QST_GENERIC;
            
            pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        }
        else
        {
	    if (pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
            {
	        qry_cb->apiQueryType = IIAPI_QT_CURSOR_UPDATE; 
                odbc_getSvcParms(pstmt, qry_cb, 
                    IIAPI_QT_CURSOR_UPDATE);
            }
            else
	        qry_cb->apiQueryType = IIAPI_QT_EXEC;
            qry_cb->sequence = QRY_QUERY;
            qry_cb->sequence_type = QST_EXECUTE;
            
            pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        }
        
	break;

     case SQLEXECPROC:
     {
        CHAR     szParamSpecified[MAX_COLUMNS_INGRES];
        LPSQLCA  psqlca = pSqb->pSqlca;
        SESS    *pSession = pSqb->pSession;
        LPDESC   pprocAPD = NULL;
        LPDESC   pprocIPD = NULL;
        LPDESC   savepAPD = NULL;
        LPDESC   savepIPD = NULL;
        BOOL     moreParms = TRUE;

        char    *szProcName = pstmt->szProcName;
        char    *szSchema   = pstmt->szProcSchema;

        pdbc = (LPDBC) pSqb->pDbc;
        MEfill (sizeof(szParamSpecified), 0, szParamSpecified);
            pprocAPD = AllocDesc_InternalCall(pdbc,
            SQL_DESC_ALLOC_AUTO,
            INITIAL_DESCFLD_ALLOCATION);

        pprocAPD->BindOffsetPtr = pstmt->pAPD->BindOffsetPtr;

        /* copy the pointer to binding offset */
        pprocAPD->BindType      = pstmt->pAPD->BindType;

        /* copy the binding orientation for parameters */
        pprocIPD = AllocDesc_InternalCall(pdbc,
            SQL_DESC_ALLOC_AUTO,
            INITIAL_DESCFLD_ALLOCATION);

        GetProcName(pstmt,szProcName,szSchema);

        odbc_getSvcParms(pstmt, qry_cb, IIAPI_QT_EXEC_PROCEDURE);

        ScanProcParmsAndBuildDescriptor(pSession, 
            pstmt, pprocAPD, pprocIPD, szProcName, szSchema);

        /*
        ** Save the default pAPD and pIPD temporarily.
        */
        savepAPD = pstmt->pAPD;
        savepIPD = pstmt->pIPD;
        pstmt->pAPD = pprocAPD; 
        pstmt->pIPD = pprocIPD;  
        apiQueryType = IIAPI_QT_EXEC_PROCEDURE;

        qry_cb->apiQueryType = apiQueryType;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_GENERIC;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

        if (psqlca->SQLERC == E_AP0002_TRANSACTION_ABORTED)
        { 
        /* 
        ** If procedure issued a commit or rollback then 
        ** burn away the transaction handle by internal 
        ** rollback.
        */
            SQLTransact_InternalCall(NULL, pdbc, 
            SQL_ROLLBACK);
            PopSqlcaMsg(psqlca); 
            ErrResetDbc(pdbc);  
        }

        /*
        ** Restore the default pAPD and pIPD.
        */
        if (savepAPD)
        {
            pstmt->pAPD = savepAPD; 
            pstmt->pIPD = savepIPD;
        }
        if (pprocIPD)
            FreeDescriptor(pprocIPD); 
        if (pprocAPD)
            FreeDescriptor(pprocAPD);
    
        break;
    }
    case SQLPUTPARMS:		 

	if (pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
	    apiQueryType = IIAPI_QT_CURSOR_UPDATE;
        else
	    apiQueryType = IIAPI_QT_QUERY;

        qry_cb->sequence = QRY_PUTP;
        qry_cb->sequence_type = QST_GENERIC;
        qry_cb->apiQueryType = apiQueryType;
	pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

        pstmt->fStatus &= ~STMT_NEED_DATA;
        pstmt->icolPut = 0;

	break;

    case SQLPUTSEGMENT:		 

	if (pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
	    apiQueryType = IIAPI_QT_CURSOR_UPDATE;
        else
	    apiQueryType = IIAPI_QT_QUERY;

        qry_cb->sequence = QRY_PUTP;
        qry_cb->sequence_type = QST_PUTSEG;
        qry_cb->apiQueryType = apiQueryType;
	pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

	break;

    case SQLOPEN:           /* called only by ExecuteStmt('SELECT...') */

        pdbc = pstmt->pdbcOwner;
	if (!(pstmt->fStatus & STMT_DIRECT))  /* if not of SQLExecDirect */
        {                                  /*    then do a real OPEN s00001. */
	    pstmt->QueryHandle = NULL;

	    if (isApiCursorNeeded(pstmt)) /* if def'ed cursor name or force */
	    {
	        GenerateCursor(pstmt);    /* then open the prepared stmt */

		pstmt->fStatus |=  STMT_API_CURSOR_OPENED; /* cursor loop */
                odbc_getSvcParms(pstmt, qry_cb, IIAPI_QT_OPEN);
                qry_cb->apiQueryType = IIAPI_QT_OPEN;  
                qry_cb->sequence = QRY_QUERY;
                qry_cb->sequence_type = QST_EXECUTE;
                pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

		break;
	    }
	    else  /* use select loops */
	    { 
                LPSESS  psess = pstmt->psess;

                qry_cb->apiQueryType = IIAPI_QT_QUERY;  
                qry_cb->sequence = QRY_QUERY;
                qry_cb->sequence_type = QST_GENERIC;
                pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
		
		psess->fStatus &= ~SESS_SUSPENDED;
		psess->fStatus |= SESS_STARTED; /* mark transaction started */

		rc = SQLCA_ERROR (pstmt, psess);
		if (rc == SQL_ERROR)
		{
		    ErrSetToken (pstmt, pstmt->szSqlStr, pstmt->cbSqlStr);
		    break;
		}
		    break;
            }
        }  /* end if within context of SQLExecute */
         break;

    case SQLPREPARE:

	apiQueryType = IIAPI_QT_QUERY;

        pstmt->QueryHandle = NULL;

	qry_cb->apiQueryType = apiQueryType;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_PREPARE;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

        if (pSqb->pSqlca->SQLCODE == SQL_SUCCESS)
	    pstmt->pdbcOwner->sqb.Options |= (fOptions & SQB_OPEN);

        if (pSqb->pSqlca->SQLCODE != SQL_ERROR)
             pstmt->fStatus |= STMT_PREPARED;

        break;

    case SQLDESCRIBE_INPUT:

	apiQueryType = IIAPI_QT_QUERY;

        pstmt->QueryHandle = NULL;

	qry_cb->apiQueryType = apiQueryType;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_DESCRIBE_INPUT;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

        break;

    case SQLSETPOS:

        apiQueryType = IIAPI_QT_QUERY;

        pstmt->QueryHandle = NULL;

        qry_cb->apiQueryType = apiQueryType;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_SETPOS;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

        break;

    case SQLEXEC_DIRECT:

	apiQueryType = IIAPI_QT_QUERY;

	switch(pstmt->fCommand)
	{
	case CMD_SELECT:

	    if (isApiCursorNeeded(pstmt)) /* if cursor name or force */
	    {
                apiQueryType = IIAPI_QT_OPEN;
                odbc_getSvcParms(pstmt, qry_cb, apiQueryType); 
		pstmt->fStatus |= STMT_API_CURSOR_OPENED; /* cursor loop */
                if (isCursorReadOnly(pstmt))
                { 
                /* 
                ** Add "for readonly" clause at the end of a stmt.
                ** After the query, we'll undo the damage to the string
                ** so that we'll leave things as we found them.
                */
                pfor_readonly_added = pstmt->szSqlStr + pstmt->cbSqlStr;
                STcopy(" for readonly\0",pfor_readonly_added);
                pstmt->cbSqlStr += 13;
            }
            odbc_AutoCommit(pSqb, FALSE);
            /* 
            ** make sure autocommit=off since Ingres server can't 
            ** handle more than one cursor open at a time when 
            ** autocommit=on.  Suspend the real API autocommit 
            ** (which is done by the server anyway when one cursor is 
            ** open) and issue a COMMIT when the statement is closed 
            ** in SQLFreeStmt. It will stay off for any other SELECT,
            **  and we'll turn API autocommit back on if the user 
            ** starts issuing INSERT, UPDATE, or DELETE. 
            */
	}
	else
	{
            /* Using select loop. */
            pstmt->fStatus &= ~STMT_API_CURSOR_OPENED;

            /*
            ** Generate a cursor name in case SQLGetCursorName() 
            ** is called.  
            */
	    GenerateCursor(pstmt);
	}
	/*
	**  invoke API function call
	*/
	pstmt->QueryHandle = (VOID *) NULL;
	qry_cb->apiQueryType = apiQueryType;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_GENERIC;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);

        /*
        ** Strip off the "for readonly" that was added to
        ** pstmt->szSqlStr previously.
        */
        if (pfor_readonly_added) 
        {                   
            *pfor_readonly_added = '\0';  /* restore the string */
            pfor_readonly_added = NULL;
            pstmt->cbSqlStr -= 13;     /* restore the string length */
        }
        break;

    case CMD_INSERT:
    case CMD_UPDATE: 
    case CMD_DELETE: 
			
	if (pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
        {
	    qry_cb->apiQueryType = IIAPI_QT_CURSOR_UPDATE; 
            odbc_getSvcParms(pstmt, qry_cb, IIAPI_QT_CURSOR_UPDATE);
        }
	    else
            qry_cb->apiQueryType = apiQueryType;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_GENERIC;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        
        break;

        case CMD_EXECPROC:
        /*
        ** Should never reach here.  SQLEXECPROC case should always handle.
        */
        break;

    default:
        if ((pstmt->crowParm <= 1)) /* one row parm */ 
        {
            qry_cb->apiQueryType = apiQueryType;
            qry_cb->sequence = QRY_QUERY;
            qry_cb->sequence_type = QST_GENERIC;
            pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        }
        else
        {
            qry_cb->apiQueryType = apiQueryType;
            qry_cb->sequence = QRY_QUERY;
            qry_cb->sequence_type = QST_PREPARE;
            pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        }
        break;
    }
    break;

    case SQLEXECUTI:

        qry_cb->apiQueryType = IIAPI_QT_QUERY;
        qry_cb->sequence = QRY_QUERY;
        qry_cb->sequence_type = QST_GENERIC;
        qry_cb->qry_buff = pBuf;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        qry_cb->qry_buff = NULL;

        break;

    case SQLFETCH:

        qry_cb->apiQueryType = IIAPI_QT_EXEC;
        qry_cb->sequence = QRY_DONE;
        qry_cb->sequence_type = QST_FETCH;
        qry_cb->qry_buff = pBuf;
        odbc_query_sm(qry_cb);
        qry_cb->qry_buff = NULL;

        break;

    case SQLFETCHSEG:

        qry_cb->apiQueryType = IIAPI_QT_EXEC;
        qry_cb->sequence = QRY_DONE;
        qry_cb->sequence_type = QST_GETSEG;
        qry_cb->qry_buff = pBuf;
        odbc_query_sm(qry_cb);
        qry_cb->qry_buff = NULL;

        break;

    case SQLFETCHUNBOUND:

        qry_cb->apiQueryType = IIAPI_QT_EXEC;
        qry_cb->sequence = QRY_DONE;
        qry_cb->sequence_type = QST_GETUNBOUND;
        qry_cb->qry_buff = pBuf;
        odbc_query_sm(qry_cb);
        qry_cb->qry_buff = NULL;

        break;

    case SQLCLOSE:
    {
        VOID * stmtHandle = pstmt->stmtHandle; /* save in case */
        if (!(pstmt->fStatus & STMT_API_CURSOR_OPENED) &&
             (pstmt->fStatus & STMT_EXECUTE) &&
             (pstmt->fCommand & CMD_SELECT) &&
             (pstmt->sqlca.SQLCODE != SQL_NO_DATA ))
             qry_cb->sequence = QRY_CANCEL;
        else
             qry_cb->sequence = QRY_CLOSE;

        qry_cb->apiQueryType = IIAPI_QT_QUERY;
        qry_cb->sequence_type = QST_GENERIC;
        pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        /* 
        ** Recover from trying to close incomplete query 
        */
        if (pstmt->sqlca.SQLERC==E_AP0007_INCOMPLETE_QUERY)
        { 
            PopSqlcaMsg(&pstmt->sqlca);      /* pop the AP0007 off */
            pstmt->stmtHandle = stmtHandle;  /* restore query handle */
            qry_cb->sequence = QRY_CANCEL;
            qry_cb->sequence_type = QST_GENERIC;
            pSqb->pSqlca->SQLCODE = odbc_query_sm(qry_cb);
        }
    }
    break;

    case SQLRELEASE:
        pdbc = (LPDBC) pSqb->pDbc;
        if (pdbc) /* If no session, nothing to disconnect */
        {
            if (pdbc->fOptions & OPT_AUTOCOMMIT)
                pSqb->pSqlca->SQLCODE = odbc_AutoCommit(pSqb,FALSE); 
            odbc_disconn(pSqb->pSession, pSqb->pSqlca);
        }
        break;

    case SQLCOMMIT:
        pdbc = (LPDBC) pSqb->pDbc;
        if (!(pSqb->pSession->fStatus & SESS_AUTOCOMMIT_ON_ISSUED))  
        /* skip if API's autocommit on */
        {
            odbc_commitTran(&(pSqb->pSession->tranHandle), pSqb->pSqlca);
            pSqb->pSession->fStatus &= ~SESS_COMMIT_NEEDED;
            if (pdbc)
                for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
                    pstmt->fStatus &= ~STMT_PREPARED; 
        }
        break;
	
    case SQLROLLBCK:
        pdbc = (LPDBC) pSqb->pDbc;
        if (!(pSqb->pSession->fStatus & SESS_AUTOCOMMIT_ON_ISSUED))  
        /* skip if API's autocommit on */
        {
            odbc_rollbackTran(&(pSqb->pSession->tranHandle),pSqb->pSqlca);
            pSqb->pSession->fStatus &= ~SESS_COMMIT_NEEDED;
            if (pdbc)
                for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
                    pstmt->fStatus &= ~STMT_PREPARED; 
        }
break; 

    default:
         /*
         *   Unsupported command:
         *
         *   SQLLNO will contain command code on return.
         */
         pSqb->pSqlca->SQLCODE = -4;       /* severity = statement fail */
         pSqb->pSqlca->SQLERC  = 1006;     /* reason = invalid command  */
         InsertSqlcaMsg(pSqb->pSqlca,ERget(F_OD0112_INVLID_COMMAND),
                         SQST_GENERAL_ERROR,TRUE);
         return -4;
    }
    
    return pSqb->pSqlca->SQLCODE;

}

/*
** SetStmt 
**
*/
static BOOL SetStmt(SESS * pSession,
		               CHAR     * pSetText,LPSQLCA psqlca,SQLUINTEGER cQueryTimeout)
{
    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CLOSEPARM     closeParm;
    IIAPI_CANCELPARM    *cancelParm;
    IIAPI_WAITPARM      waitParm = { -1 };
    BOOL             rc;

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_flags = 0;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = (VOID *) pSetText;
    queryParm.qy_parameters = FALSE;  
    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_stmtHandle = NULL;
    queryParm.qy_flags = 0;

    IIapi_query( &queryParm );

    pSession->tranHandle = queryParm.qy_tranHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, psqlca, cQueryTimeout ))
    {
	cancelParm = (IIAPI_CANCELPARM *)odbc_malloc(sizeof(IIAPI_CANCELPARM));
        cancelParm->cn_genParm.gp_callback = NULL;
        cancelParm->cn_genParm.gp_closure = NULL;
        cancelParm->cn_genParm.gp_completed = FALSE;
        cancelParm->cn_stmtHandle = queryParm.qy_stmtHandle;
        IIapi_cancel( cancelParm );

        if (cQueryTimeout > 0)
            waitParm.wt_timeout = (i4)(cQueryTimeout * 1000);

        while( cancelParm->cn_genParm.gp_completed == FALSE )
        {
            IIapi_wait( &waitParm );
            if ( waitParm.wt_status != IIAPI_ST_SUCCESS ) 
            {
                break;
            }
        }
        odbc_free( cancelParm );
    }

    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    rc=odbc_getResult( &getQInfoParm.gq_genParm, psqlca, cQueryTimeout);

    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;
    IIapi_close( &closeParm );
    if (!odbc_getResult( &closeParm.cl_genParm, psqlca, cQueryTimeout ))
        return(FALSE);

    return(TRUE);
}
/*
**
** ProcessAutoCommit
**
*/
static void ProcessAutoCommit(CHAR     * lpAutoCommit, LPDBC pdbc)
{
	CHAR     * lpChar;

	lpChar = lpAutoCommit + 11; /* skip AUTOCOMMIT */
	while (CMwhite(lpChar)) CMnext(lpChar);
	if (!STbcompare(lpChar,2,"ON",2,TRUE))
			pdbc->fOptions |= OPT_AUTOCOMMIT;
	else
            pdbc->fOptions &= ~OPT_AUTOCOMMIT;
	return;
}
/*
** ProcessIncludeFile  
**
*/
static BOOL ProcessIncludeFile(SESS * pSession,
		               CHAR     * szPath,LPSQLCA psqlca,SQLUINTEGER cQueryTimeout,
					   LPDBC pdbc)
{
	FILE *     pFile = NULL;
	LOCATION   loc;
	STATUS     status;
	char       locbuf[MAX_LOC + 1];
	CHAR       buff[512];
	CHAR     * token;
	CHAR     * lpChar;
	BOOL    rc = FALSE;
	CHAR       szSep[]=";\n";

	if (*szPath == '\0')
		return(FALSE);

	STcopy(szPath, locbuf);    /* set up LOCATION block for file */
	if (LOfroms(PATH & FILENAME, locbuf, &loc) != OK)
	   return(FALSE);

	if ((status = SIfopen(&loc, ERx("r"), (i4)SI_TXT, 256, &pFile)) != OK)
		return(FALSE);

	rc = TRUE;
	
	while ((status = SIgetrec (buff, (i4)sizeof(buff), pFile)) == OK)
	{
		/* process set stmts */
		token = strtok(buff,szSep);
		while (token)
		{
			while (CMwhite(token)) CMnext(token);  /* skip whilespace*/
			if (!STbcompare(token,4,"SET ",4,TRUE))
			{
				lpChar = token + 4;
				while (CMwhite(lpChar)) CMnext(lpChar);
				if (STbcompare(lpChar,11,"AUTOCOMMIT ",11,TRUE))
				    SetStmt(pSession,token,psqlca,cQueryTimeout);
				else
					ProcessAutoCommit(lpChar,pdbc);
			}
			token = strtok(NULL,szSep);
		}
    }   /* end while loop reading records */

	SIclose(pFile);
	return(rc);
}
/*
** ProcessIngSet 
**
**      Needed because OpenAPI does not honor ING_SET env vars 
*/
static BOOL ProcessIngSet(SESS * pSession,
		                    LPSQLCA psqlca,
		                    SQLUINTEGER cQueryTimeout,CHAR     * pEnvVar,
							LPDBC pdbc)
{
#define MAX_SET 64	/* max length of the _SET enviroment variables */
	CHAR     * lpIngSet;
	CHAR     * token;
	CHAR     * lpChar;
	CHAR       szSep[]=";\"";
	CHAR szIngSet[MAX_SET+1];

	if (!isServerClassINGRES(pdbc))  /* return if not INGRES server */
	   return(TRUE);

	NMgtAt(pEnvVar,&lpIngSet);
	if (lpIngSet != NULL && *lpIngSet)
	{
		MEfill (sizeof(szIngSet), 0, szIngSet );
		STlcopy (lpIngSet, szIngSet, MAX_SET);
		lpIngSet = szIngSet;
	}
	else
		return(TRUE);

	/* locate the first non-blank char after quotation mark */
	while ((CMwhite(lpIngSet)) || (*lpIngSet == '\"'))
		    CMnext(lpIngSet);
    
	token = strtok(lpIngSet,szSep);
	while (token)
	{
	    while (CMwhite(token)) CMnext(token);
		if (!STbcompare(token,4,"SET ",4,TRUE))
		{
			lpChar = token + 4;
			while (CMwhite(lpChar)) CMnext(lpChar);
			if (STbcompare(lpChar,11,"AUTOCOMMIT ",11,TRUE))
				SetStmt(pSession,token,psqlca,cQueryTimeout);
			else
				ProcessAutoCommit(lpChar,pdbc);
		    token = strtok(NULL,szSep);
		}
		else
		{
			if (!STbcompare(token,8,"INCLUDE ",8,TRUE))
			{
				lpChar = token + 8; 
			    while (CMwhite(lpChar)) CMnext(lpChar);
				ProcessIncludeFile(pSession,lpChar,psqlca,cQueryTimeout,pdbc); 
				token = NULL;
			}
			else
            {
			    token = strtok(NULL,szSep);
            }
		}
	}
	return(TRUE);
}

/*
** ProcessII_DATE_FORMAT 
**
**      Needed because OpenAPI does not honor II_DATE_FORMAT env vars 
*/
static i4 ProcessII_DATE_FORMAT(void)
{
	CHAR     * lpEnvVal;
	CHAR     * token;
	CHAR       szSep[]=";\"";
	CHAR szEnvVal[MAX_LOC+1];

	NMgtAt("II_DATE_FORMAT",&lpEnvVal);
	if (lpEnvVal != NULL && *lpEnvVal)
	{
		MEfill (sizeof(szEnvVal) ,0, szEnvVal);
		STlcopy(lpEnvVal,szEnvVal,MAX_LOC);
		lpEnvVal = szEnvVal;
	}
	else
		return(IIAPI_CPV_DFRMT_US);

	/* locate the fisrt non-blank char after quotation mark */
	while ((CMspace (lpEnvVal)) || (*lpEnvVal == '\"'))
		lpEnvVal++;
    
	token = strtok(lpEnvVal,szSep);

	while (CMspace (token)) token++;
	if       (!STbcompare(token,6,"SWEDEN",6,TRUE) || 
	          !STbcompare(token,7,"FINLAND",7,TRUE))  
	   return(IIAPI_CPV_DFRMT_FINNISH);
	else if (!STbcompare(token,14,"MULTINATIONAL4",14,TRUE))  
	   return(IIAPI_CPV_DFRMT_MLT4);
  	else if (!STbcompare(token,13,"MULTINATIONAL",13,TRUE))
	   return(IIAPI_CPV_DFRMT_MULTI);
  	else if (!STbcompare(token,3,"ISO",3,TRUE))
	   return(IIAPI_CPV_DFRMT_ISO);
  	else if (!STbcompare(token,6,"GERMAN",6,TRUE))
	   return(IIAPI_CPV_DFRMT_GERMAN);
  	else if (!STbcompare(token,3,"YMD",3,TRUE))
	   return(IIAPI_CPV_DFRMT_YMD);
  	else if (!STbcompare(token,3,"DMY",3,TRUE))
	   return(IIAPI_CPV_DFRMT_DMY);
  	else if (!STbcompare(token,3,"MDY",3,TRUE))
	   return(IIAPI_CPV_DFRMT_MDY);

	return(IIAPI_CPV_DFRMT_US);
}

/*
** ProcessII_DATE_CENTURY_BOUNDARY 
**
**      Needed because OpenAPI does not honor II_DATE_CENTURY_BOUNDARY env vars 
*/
static i4 ProcessII_DATE_CENTURY_BOUNDARY(void)
{
# define DEF_YEAR_CUTOFF     0
# define MAX_YEAR_CUTOFF     100

        CHAR      * lpEnvVal;

        static  BOOL year_cutoff_set = FALSE;
        static  SDWORD date_century = DEF_YEAR_CUTOFF;

        if (!year_cutoff_set)
        {
            NMgtAt("II_DATE_CENTURY_BOUNDARY", &lpEnvVal);
            if (lpEnvVal != NULL && *lpEnvVal)
            {
                if (CvtCharInt(&date_century, lpEnvVal, 
                    (UWORD) strlen(lpEnvVal)) == CVT_SUCCESS)
                {
                    if ((date_century > DEF_YEAR_CUTOFF) && 
                        (date_century < MAX_YEAR_CUTOFF))
                    {
                        year_cutoff_set = TRUE;
                        return(date_century);
                    }
                }
            }
        }
        else
        {
            return(date_century);
        }
        return(DEF_YEAR_CUTOFF);
}

/*
** ProcessII_DECIMAL 
**
*/
static CHAR *ProcessII_DECIMAL(void)
{
    static CHAR     *lpEnvVal;
    static BOOL decimal_set = FALSE;

    if (!decimal_set)
    {
        decimal_set = TRUE;
        NMgtAt( "II_DECIMAL", &lpEnvVal );
        if ( lpEnvVal != NULL && *lpEnvVal != '\0' )
        {
            if (!STbcompare(lpEnvVal,0,",\0",0,TRUE)) 
                lpEnvVal =  STalloc(",");
            else
                lpEnvVal = STalloc(".");
        }
        else
            lpEnvVal = STalloc(".");
    }
    return(lpEnvVal);
}

/*
** Name: isApiCursorNeeded 
**
** Description:
**      Returns TRUE if need to OPEN an API cursor and use
**      a cursor loop instead of a select loop.  
**
*/
BOOL isApiCursorNeeded(LPSTMT pstmt)
{
    size_t len;

    TRACE_INTL(pstmt,"isApiCursorNeeded");
    if (STlength(pstmt->szCursor) > 0 )  /* if user defined cursor name, return TRUE*/
       { if (pstmt->pdbcOwner->fOptions & OPT_CURSOR_LOOP)
            return(TRUE);  /* if always using cursor loops, return TRUE */
         len = STlength(CURSOR_PREFIX);
         if (MEcmp(pstmt->szCursor, CURSOR_PREFIX, len)==0)
            return(FALSE);  /* if system gen'ed name, try select loop */
         return(TRUE);
       }

    if (!(pstmt->pdbcOwner->fOptions & OPT_CURSOR_LOOP))
         return(FALSE);  /* if using select loops, return FALSE */

    /* szCursor is needed */
    GenerateCursor(pstmt);
    return(TRUE);
}

/*
** Name: isCursorReadOnly 
**
** Description:
**      Returns TRUE if SQL_CONCURRENCY for the stmt's
**      cursor is read-only.  
**
*/
static BOOL isCursorReadOnly(LPSTMT pstmt)
{
    TRACE_INTL(pstmt,"isCursorReadOnly");
    if (pstmt->fConcurrency == SQL_CONCUR_READ_ONLY)
       return(TRUE);
    return(FALSE);
}

/*
**  GetCapabilitiesFromDSN
**
**  On entry: szDSN      -> Data Source Name
**
**  Returns:  nothing
**
**  Notes:    Get the namecase, tablename length, etc., into DBC from DSN
**            before we start editting in changes to servername and servertype.
*/
static void GetCapabilitiesFromDSN(char * pDSN, LPDBC pdbc)
{
    char   dsnValue[64];

    if (pDSN == NULL  ||  *pDSN == '\0')
       return;    /* return if no DSN */

          /* if a DSN name and gateway, use cache.  In order to
             improve performance at connection time, the ODBC DSN
             configuration DLL will cache certain servertype information
             to avoid issuing connection queries to get keep getting
             the same answers. */

    if (isServerClassINGRES(pdbc))  /* if Ingres, always do the '_version' */
       return;                      /* and case queries */

          /* use DSN cache for server capabilities rather than issuing
             queries for the same answer every time. */
    SQLGetPrivateProfileString (pDSN,"ServerVersion",
                          "",dsnValue,sizeof(dsnValue),ODBC_INI);

    if (*dsnValue == '\0')  /* Don't bother getting the other Serverxxx */
        return;             /* entries if ServerVersion is not there.   */

    pdbc->fRelease = atoi(dsnValue+0) * 1000000 +
                     atoi(dsnValue+3) * 10000  +
                     atoi(dsnValue+6);

    SQLGetPrivateProfileString (pDSN,"ServerNameCase",
                      "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server name case */
        pdbc->db_name_case = *dsnValue;

    SQLGetPrivateProfileString (pDSN,"ServerDelimitedCase",
                      "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server delimited name case */
        pdbc->db_delimited_case = *dsnValue;

    SQLGetPrivateProfileString (pDSN,"ServerMaxColumns",
                      "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max columns in table */
        pdbc->max_columns_in_table = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxSchemaNameLen",
                      "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max schema name len */
        pdbc->max_schema_name_length = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxTableNameLen",
                      "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max table name len */
       {pdbc->max_table_name_length = 
        pdbc->db_table_name_length  = atoi(dsnValue);
       }

    SQLGetPrivateProfileString (pDSN,"ServerMaxColumnNameLen",
                      "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max column name len */
        pdbc->max_column_name_length = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxRowLen",
                  "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max column size */
        pdbc->max_row_length = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxCharColumnLen",
                  "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max char column size */
        pdbc->max_char_column_length = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxVchrColumnLen",
                  "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max varchar column size */
        pdbc->max_vchr_column_length = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxByteColumnLen",
                  "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max binary column size */
        pdbc->max_byte_column_length = atoi(dsnValue);

    SQLGetPrivateProfileString (pDSN,"ServerMaxVbytColumnLen",
                  "",dsnValue,sizeof(dsnValue),ODBC_INI);
    if (*dsnValue)  /* server max varbinary column size */
        pdbc->max_vbyt_column_length = atoi(dsnValue);
    
    return;
}


/*
** Name: odbc_query_sm
**
** Description:
**	Query processing sequencer.  Callback function for API
**	query functions.
**
** Input:
**	arg	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**      04-Dec-2007 (Ralph Loen) Bug 119565 
**          Change state to QRY_CANCEL instead of QRY_CLOSE if an error occurs 
**          in an intermediate stage of a query.
**     23-Jan-2008 (Ralph Loen) Bug 119800
**          Don't set pstmt->QueryHandle to NULL at the top of the
**          state machine.           
**     05-May-2008 (Ralph Loen) Bug 120339
**          In odbc_query_sm(), queue DBMS messages on the 
**          error stack in the QRY_CLOSE state.
**     31-Jul-2008 (Ralph Loen) Bug 120718
**          Don't invoke SQLCA_ERROR if a conversion error has been set for 
**          the QRY_CLOSE state.
**    20-Nov-2009 (Ralph Loen)
**          The QST_DESCRIBE query type is now obsolete.  Instead,
**          the QST_PREPARE query type sets the sequence to QRY_GETD.
**    12-Oct-2010 (Ralph Loen)  Bug 124581
**          Removal of QST_DESCRIBE resulted in a memory leak for
**          prepared select queries using select loops.  
**          In the QRY_SETD case invoke PrepareSqldaAndBuffer() with the 
**          prepare_or_describe argument set to SQLDESCRIB if the
**          select query is prepared: in this case, only the sqlda needs 
**          to be allocated, not the fetch buffer.
*/

RETCODE odbc_query_sm( QRY_CB *qry_cb )
{

    LPSTMT pstmt = qry_cb->pstmt;
    i2 sequence = qry_cb->sequence;
    i2 sequence_type = qry_cb->sequence_type;
    IIAPI_QUERYTYPE apiQueryType = qry_cb->apiQueryType; 
    i2 options = qry_cb->options; 
    char *qry_buff = qry_cb->qry_buff;
    LPDBC pdbc = pstmt->pdbcOwner;
    IIAPI_PUTPARMPARM *serviceParms = &qry_cb->serviceParms;
    SESS *pSession = pdbc->sqb.pSession;
    LPSQB pSqb = &pdbc->sqb;
    BOOL descriptorWanted = TRUE;
    RETCODE rc = SQL_SUCCESS, ret;
    II_INT2 dsCount;

    top:

    if (IItrace.fTrace >= ODBC_IN_TRACE)
        odbc_print_qry(sequence, sequence_type);

    switch( sequence++ )
    {

    case QRY_QUERY :	

        if (pstmt->stmtHandle && sequence_type != QST_SETPOS)
            odbc_close(pstmt);

        if (sequence_type == QST_PREPARE)
        { 
            if (!odbc_prepare(pSession, pstmt))
            {
               /*
               ** Setting the state to QRY_CANCEL results in a call to
               ** odbc_cancel->IIapi_cancel->GCA_PURGE.  This should be done
               ** only if the client is waiting for the DBMS to send
               ** something other than GCA_RESPONSE.  So the state machine
               ** tries to be conservative and send the cancel only when
               ** strictly necessary, i.e., when parameter data is queued.
               */
               rc = SQL_ERROR;
               if (pstmt->icolParmLast > 0)    /* parameter count > 0 */
                   sequence = QRY_CANCEL;
               else
                   sequence = QRY_CLOSE;
               break;
            }
            if (pstmt->fCommand & CMD_SELECT)
                sequence = QRY_GETD;
            else
                sequence = QRY_GETQI;
            break;
        }
        if (sequence_type == QST_DESCRIBE_INPUT)
        { 
            if (!odbc_describe_input(pSession, pstmt))
            {
                rc = SQL_ERROR;
                sequence = QRY_CANCEL;
                break;
            }
            sequence = QRY_GETQI;
            break;
        }
        if (sequence_type == QST_SETPOS)
        {
            if (!odbc_setPos(pSession,pstmt))
            {
               rc = SQL_ERROR;
               sequence = QRY_CANCEL;
               break;
            }
            sequence = QRY_GETQI;
            break;
        }
        if (sequence_type == QST_EXECUTE)
        { 
            if (!odbc_execute(pSession, pstmt, apiQueryType))
            {
               /*
               ** Setting the state to QRY_CANCEL results in a call to
               ** odbc_cancel->IIapi_cancel->GCA_PURGE.  This should be done
               ** only if the client is waiting for the DBMS to send
               ** something other than GCA_RESPONSE.  So the state machine
               ** tries to be conservative and send the cancel only when
               ** strictly necessary, i.e., when parameter data is queued.
               */
               rc = SQL_ERROR;
               if (pstmt->icolParmLast > 0)    /* parameter count > 0 */
                   sequence = QRY_CANCEL;
               else
                   sequence = QRY_CLOSE;
               break;
            }
        }
        else if (sequence_type == QST_QUERY)
        {
            if (!odbc_query(pSession,pstmt,apiQueryType,
                qry_buff))
            {
                /*
                ** Setting the state to QRY_CANCEL results in a call to
                ** odbc_cancel->IIapi_cancel->GCA_PURGE.  This should be done
                ** only if the client is waiting for the DBMS to send
                ** something other than GCA_RESPONSE.  So the state machine
                ** tries to be conservative and send the cancel only when
                ** strictly necessary, i.e., when parameter data is queued.
                */
                rc = SQL_ERROR;
                if (pstmt->icolParmLast > 0)    /* parameter count > 0 */
                    sequence = QRY_CANCEL;
                else
                    sequence = QRY_CLOSE;
                break;
            }
            sequence = QRY_GETQI;
            break;
        }
        else 
        {
            if (!odbc_query(pSession,pstmt,apiQueryType,
                qry_buff))
            {
                rc = SQL_ERROR;
                sequence = QRY_CLOSE;
                break;
            }
        }

	if ( (apiQueryType == IIAPI_QT_OPEN) ||
	    (apiQueryType == IIAPI_QT_CURSOR_UPDATE) ||
            (apiQueryType == IIAPI_QT_EXEC_PROCEDURE) )
            break;

        if (pstmt->icolParmLast > 0)    /* parameter count > 0 */
            break;

	sequence = QRY_GETD;
        break;
    
    case QRY_SETD :

        if (!odbc_setDescr( pstmt, apiQueryType ))
        {
            rc = SQL_ERROR;
            sequence = QRY_CANCEL; 
            break;
        }

        if (pstmt->fStatus & STMT_NEED_DATA)
        {
            rc = SQL_NEED_DATA;
            sequence = QRY_EXIT;
            break;
        }

	break;

    case QRY_PUTP :

        if (sequence_type == QST_PUTSEG)
        {
            ret = odbc_putSegment(pstmt,apiQueryType, serviceParms,
                pstmt->moreData);
            if (ret == SQL_NEED_DATA)
                sequence = QRY_EXIT;
            else if (ret == SQL_ERROR)
            {
                rc = ret;
                sequence = QRY_EXIT;
            }
        }
        else
	    if (!odbc_putParms(pstmt,apiQueryType, serviceParms))
                rc = SQL_ERROR;
      
	break;

    case QRY_GETD :	

        if ((pstmt->fCommand != CMD_SELECT) && (pstmt->fCommand !=
            CMD_EXECPROC))
            break;
      
	if (!odbc_getDescr(pstmt, TRUE, &dsCount, FALSE))
        {
            rc = SQL_ERROR;
            sequence = QRY_CANCEL;
            break;
        }

        if (!dsCount)
            break;

	/*  
        ** Set up fetch buffer SQLDA, buffer offsets, and 
        ** allocate the buffer.
        */
        rc = PrepareSqldaAndBuffer (pstmt, sequence_type == QST_PREPARE ?
             SQLDESCRIB : SQLPREPARE);
	if (rc == SQL_ERROR)
	{
            sequence = QRY_CANCEL; 
	    break;
	}

        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
        {
           odbc_retrieveByRefParms(pstmt);
           if (pstmt->fStatus & STMT_OPEN)
               sequence = QRY_EXIT;
        }

	if (pstmt->fCommand & CMD_SELECT)
        {
            sequence = QRY_EXIT;
	    break;
        }

	break;

    case QRY_GETQI :	

	/*
	** If there is no result set, then the
	** current statement simply needs to be
	** closed.  
	**
	** Query info required for non-select 
	** statements and cursors but not for 
	** tuple streams since the call would 
	** close the stream.
	*/

        if (apiQueryType == IIAPI_QT_EXEC_PROCEDURE)
        {
           odbc_getQInfo(pstmt);
           sequence = QRY_CLOSE;
           break;
        }

        if (sequence_type == QST_PREPARE)
        {
           if (!(pstmt->fCommand & CMD_SELECT))
           {
               if (!odbc_getQInfo(pstmt))
                   rc = SQL_ERROR;
           }

           sequence = QRY_EXIT;

           break;
        }

        if ((pstmt->fStatus & STMT_API_CURSOR_OPENED)
            && (pSqb->pSqlca->SQLCODE != SQL_NO_DATA))
        {
           if (!odbc_getQInfo(pstmt))
           {
               rc = SQL_ERROR;
               sequence = QRY_EXIT;
           }
           if (sequence_type == QST_SETPOS)
           {
               sequence = QRY_EXIT;
               break;
		   }
           break;
        }

        if (pstmt->fCommand & CMD_SELECT && sequence_type != QST_DESCRIBE_INPUT)
            break;
           
        /*
        ** All non-select queries fall through to QRY_CLOSE.
        */
        if (!odbc_getQInfo(pstmt))
        {
            rc = SQL_ERROR;
            sequence = QRY_EXIT;
        }
        else
            sequence = QRY_CLOSE;

        break; 

    case QRY_DONE :	

	/*
	** Cursor and tuple stream statements are kept
	** open for future reference.  Close statements
	** which are not are to be kept open.
	*/

        if (sequence_type == QST_GETSEG)
        {
            if (!odbc_getSegment(pSession,pstmt))
            {
                rc = SQL_ERROR;
                sequence = QRY_CANCEL;
                break;
            }
        }
        else if (sequence_type == QST_GETUNBOUND)
        {
            if (!odbc_getUnboundColumnsLong(pSession,pstmt))
            {
                rc = SQL_ERROR;
                sequence = QRY_CANCEL;
                break;
            }
        }
        else if (pstmt->fCursorType != SQL_CURSOR_FORWARD_ONLY &&
            pstmt->fAPILevel >= IIAPI_LEVEL_5)
        {
            if (!hasLongDataColumn(pstmt))
            {
                if (!odbc_scrollColumns(pSession,pstmt))
                {
                    rc = SQL_ERROR;
                    sequence = QRY_CANCEL;
                    break;
                }
            }
            else
            {
                /*
                ** Presently scrollable result sets cannot contain LOB's
                ** or locators.
                */
                InsertSqlcaMsg(&pstmt->sqlca,"LOB's or locators are not \
supported in scrollable result sets","HY000",TRUE);
                rc = SQL_ERROR;
                sequence = QRY_CANCEL;
                break;
            }
        }
        else if (!hasLongDataColumn(pstmt))
        {
            if (!odbc_getColumns(pSession,pstmt))
            {
                rc = SQL_ERROR;
                sequence = QRY_CANCEL;
            }
        }
        else 
        {
            if (!odbc_getColumnsLong(pSession,pstmt) )
            {
                rc = SQL_ERROR;
                sequence = QRY_CANCEL;
            }
        }

        /*
        ** For tuple streams, if the data stream is still active
        ** on the connection, make sure we keep the statement
        ** active locally.  Cursors are dormant after being opened
        ** and in between fetches.  Scrollable cursors cannot be 
        ** closed at EOD because the cursor may be still be           
        ** re-positioned.
        */
        if (pstmt->fAPILevel < IIAPI_LEVEL_5)
        {
            if (pSqb->pSqlca->SQLCODE == SQL_NO_DATA)
            {
                sequence = QRY_CLOSE;
                break;
            }
        }
        else if (pstmt->fCursorType == SQL_CURSOR_FORWARD_ONLY)
        {
            if (pSqb->pSqlca->SQLCODE == SQL_NO_DATA)
            {
                sequence = QRY_CLOSE;
                break;
            }
    
        }
        sequence = QRY_EXIT;
        break;

    case QRY_CANCEL :		/* Cancel the current query */

	odbc_cancel(pstmt);
	break;

    case QRY_CLOSE :		/* Close current query */

        /*
        ** Catch any DBMS messages.  Conversion errors are caught
        ** later on.                
        */
        if ( ! pstmt->fCvtError )
            rc = SQLCA_ERROR(pstmt,pSqb->pSession);

	odbc_close(pstmt);

        if (sequence_type == QST_FETCH)
        {
            /*
            ** If the close was successful, restore the
            ** EOD fetch status.
            */
            if (rc == SQL_SUCCESS)
                pSqb->pSqlca->SQLCODE = SQL_NO_DATA;
        }
	break;

    case QRY_EXIT :		/* Query completed. */

	return rc;

    }

    goto top;
}


BOOL
stripOffQuotes( char *s)
{
  BOOL ret = FALSE;
  char *p, *q=s;
 
  if ( *s == '"' ) 
  { /* It starts with quote */
    ret = TRUE;
    /* Shift the string one position to left */
    for ( p=s; *p = *CMnext(s); ) 
    {  /* *(++s) */
        p = s;
    }
    /* *p is point to the closing zero */
    if ( (*(char *)(CMprev(p,q))) == '"' ) /* *(--p) */
    {
      *p = '\0';
    }
  }
  return ret;
}


/*
** Name: odbc_print_qry
**
** Description:
**	Write ODBC state machine information to the trace log.
**
** Input:
**	sequence	Query Sequence.
**      sequence_type   Query Sequence Type.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 20-Nov-2009 (Ralph Loen) Bug 122945
**	    Created.
*/
void
odbc_print_qry( i2 sequence, i2 sequence_type )
{
    i2 seq;
    i2 type;

    for ( seq = 0; seq < QRY_SEQ_TABLE_SIZE; seq++)
    {
        if (qry_state[seq].value == sequence)
           break;
    }   

    for ( type = 0; type < QRY_SEQ_TYPE_TABLE_SIZE; type++)
    {
        if (qry_type[type].value == sequence_type)
           break;
    }   

    TRdisplay("ODBC Trace: Query sequence %s query type %s\n", 
        qry_state[seq].str, qry_type[type].str);
}
