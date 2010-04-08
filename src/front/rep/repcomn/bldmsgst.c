/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <stdarg.h>
# include <compat.h>
# include <er.h>
# include <st.h>
# include <tm.h>

/**
** Name:	bldmsgst.c - build messageit string
**
** Description:
**	Defines
**		get_message_text	- get message text
**		build_messageit_string	- build message string
**		lkup_ermsg		- look up an error message
**
** FIXME:
**	This entire scheme should be replaced with ER routines.
**
** History:
**	03-jan-97 (joea)
**		Created based on bldmsgst.c in replicator library.
**	22-jan-97 (joea)
**		Update msg 1021 for IIRSrlt_ReleaseTrans.
**	21-feb-97 (joea)
**		Add message 1268 and fix 1258, 1260.
**	29-apr-97 (joea)
**		Remove messages referring to dd_transaction_id and the distrib/
**		distrib2 modules.  Change CA-Ingres to OpenIngres.
**	07-may-97 (joea)
**		Clarify various event() and eventit() messages.
**	08-sep-97 (joea)
**		Add message 103 for error from IIRSclq_CloseQueue.
**	23-oct-97 (joea) bug 86654
**		Provide all parameters for message 1226, error codes 2 and 3.
**	29-oct-97 (joea)
**		Remove obsolete messages, e.g., referring to the dd_dispatch
**		dbevent and the -DIS flag. Reinstate missing message 71. Ensure
**		that multiple error_code messages all have the same number and
**		type of parameters. Use consistent names for transaction key
**		elements.
**	14-nov-97 (joea)
**		Add missing error_code messages for 1528.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	21-apr-98 (joea)
**		Various changes and deletions related to converting RepServer
**		to OpenAPI.
**	11-jun-98 (abbjo03)
**		Add timeout error to text of message 1021/1.
**	08-jul-98 (padni01) # 89864
**		Cross-integration of change 2833: Re-phrased msg 1826
**	03-sep-98 (muhpa01)
**		Added message 1394 for mailing on HPUX - mailx supports '-s'
**		but mail does not.
**	30-sep-98 (abbjo03)
**		Remove messages used by ddba_messageit.
**	04-nov-98 (abbjo03)
**		Add message 106 to report statistics.
**	03-dec-98 (abbjo03)
**		Add messages 107-111. Replace 1394-1396 by a simplified 1394.
**	05-may-99 (abbjo03)
**		Add message 113.
**	08-sep-99 (nanpr01/abbjo03)
**		Add message 114 for optimistic collision resolution on insert.
**	22-nov-99 (abbjo03)
**		Remove CreateKeys messages.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Oct-2002 (padni01) bug 106498 INGREP 108
**	    Added message 1900.
**      23-Aug-2005 (inifa01)INGREP174/B114410
**	    Added message 115 and 1901.
**	16-May-2005 (inifa01) INGREP176 b114521
**	    Errors 1614 unkown error message and 1618 unkown error message
**	    get reported following error 1392.
**	    Message text was missing for both 1614 and 1618 where zero rows
**	    or multiple rows are returned.  
**/

# define DIM(a)		(sizeof(a) / sizeof(a[0]))


typedef struct _RP_ERMSG
{
	i4	msg_num;
	short	err_code;
	char	*msg_text;
} RP_ERMSG;


static char *Msg_Prefixes[] =
{
	ERx("Error"), ERx("Info 1"), ERx("Warning"),
	ERx("Info 2"), ERx("Debug")
};


/* Error messages and other strings */

static RP_ERMSG messages[] =
{
	{3, 0, "OPEN_DB:  DBMS error %d opening database '%s'.  %s."},
	{4, 0, "SET_FLAGS: -LGL!  Log level is %d."},
	{5, 0, "SET_FLAGS: -PTL!  Print level is %d"},
	{6, 0, "SET_FLAGS: -SVR!  Server Number is %d."},
	{7, 0, "SET_FLAGS: -CLQ!  Unquiet all databases and CDDS's."},
	{8, 0, "SET_FLAGS: -SVR!  Invalid server number - Aborting."},
	{9, 0, "SET_FLAGS: -EMX!  Maximum error count set to %d."},
	{15, 0, "SET_FLAGS: -IDB!  Local database set to '%s'."},
	{16, 0, "SET_FLAGS: -OWN!  Local database owner set to '%s'."},
	{17, 0, "SET_FLAGS: -MLE!  Send mail messages on error."},
	{18, 0, "SET_FLAGS: -NML!  Do not send mail messages on error."},
	{19, 0, "SET_FLAGS: -QIT!  Server mode set to quiet"},
	{20, 0, "SET_FLAGS: -NQT!  Server mode set to active (non-quiet)."},
	{27, 0, "SET_FLAGS: -MON!  Monitor events now being broadcast."},
	{28, 0, "SET_FLAGS: -NMO!  Monitor events silent."},
	{31, 0, "SET_FLAGS:  Unknown or invalid flag '%s' for this server."},
	{40, 0, "SET_FLAGS: -QBT!  Read queue try to break after %d."},
	{41, 0, "SET_FLAGS: -QBM!  Read queue must break after %d."},
	{42, 0, "SET_FLAGS: -SGL!  Single pass flag set."},
	{43, 0, "Ingres Replicator Server start."},
	{44, 0, "REPLICAT:  Replicator Server -- Normal shutdown."},
	{58, 0, "SET_FLAGS: -QEP!  Turn on QEP in the server."},
	{59, 0, "SET_FLAGS: -NEP!  Turn off QEP in the server."},
	{61, 0, "SET_FLAGS: -TOT!  Time out set to %d."},
	{62, 0, "SET_FLAGS: -ORT!  Open retry is %d."},
	{63, 0, "SET_FLAGS: -EVT!  DB event timeout is %d"},
	{64, 0, "D_ERROR:  Lost connection to local database -- ABORTING."},
	{65, 0, "FILE_FLAGS:  configuration file '%s' not found"},
	{66, 0, "PING:  The Server is Quiet, Error Count %d, Ping %d"},
	{67, 0, "PING:  The Server is Active, Error Count %d, Ping %d"},
	{68, 0, "Quiet, %d errors, Ping %d, db %d %s::%s"},
	{69, 0, "Active, %d errors, Ping %d, db %d %s::%s"},
	{71, 0, "Use of the Replicator is not authorized on this node."},
	{76, 0, "Checking of primary keys is disabled."},
	{77, 0, "Checking of primary keys is enabled"},
	{78, 0, "Server is already running.  Shut it down before starting \
another server by the same number."},
	{84, 0, "SET_FLAGS: -TPC!  Two Phase Commit is %d."},
	{85, 0, "SET_FLAGS: -CTO!  Connection timeout is %d."},
	{87, 0, "SET_FLAGS: -QDB!  Quiet database %d."},
	{88, 0, "SET_FLAGS: -UDB!  Unquiet database %d."},
	{89, 0, "SET_FLAGS: -QCD!  Quiet CDDS %d."},
	{90, 0, "SET_FLAGS: -UCD!  Unquiet CDDS %d."},
	{93, 0, "One or more required startup flags not set -- ABORTING."},
	{94, 0, "SHUTDOWN:  Replicator Server -- Shutdown due to error."},
	{96, 0, "OPEN_DB:  DBMS error %d selecting DBA name.  %s"},
	{97, 0, "OPEN_DB:  DBMS error %d selecting user name.  %s"},
	{98, 0, "OPEN_DB:  User '%s' is not the owner of database '%s'.  Use th\
e -OWN startup flag to specify user '%s' as the correct owner -- Aborting."},
	{99, 0, "QBM cannot be set lower than QBT. Setting QBM equal to QBT. \
Read queue must break after %d."},
	/* messages 100-102 are used in REP 1.0 */
	{103, 0, "TRANSMIT: Error in IIRSclq_CloseQueue with server %d."},
	{106, 0, "Closing connection to database %d '%s::%s'. Transmitted %d \
transactions in %d:%02d hours."},
	{107, 0, "Error initializing the registered tables cache"},
	{108, 0, "Error initializing statistics monitoring facility"},
	{109, 0, "Cannot open temporary error mail file. Disabling mail."},
	{110, 0, "Error preparing descriptor buffers for Source DB %d, \
Transaction ID %d, Sequence %d."},
	{111, 0, "Error fetching row data for Source DB %d, Transaction ID %d, \
Sequence %d."},
	{112, 0, "DBMS error %d turning off Change Recorder in database '%s'. \
%s."},
	{113, 0, "Error retrieving DB_NAME_CASE from iidbcapabilities."},
	{114, 0, "D_INSERT: Duplicate error inserting record Source DB %d, \
Transaction ID %d, Sequence %d into Target DB %d on table '%s'. \
This may be due to possible collision."},
	{115, 0, "SET_FLAGS: -NLR!  Turn on readlock=nolock in the server"},
	{1013, 0, "EVENTIT: Getting DB Event"},
	{1017, 0, "EVENTIT: '%s' dbevent received with text '%s'."},
	{1019, 0, "EVENTIT: Found dbevent '%s' with action %d."},
	{1020, 0, "EVENTIT: Event '%s' with action %d being processed."},
	{1021, 1, "IIRSrlt_ReleaseTrans:  Error deleting from distribution \
queue, Source DB %d, Transaction ID %d, Target DB %d.  If deadlock or timeout \
error, server will retry.  For all other errors, server will shutdown."},
	{1021, 2, "IIRSrlt_ReleaseTrans:  No rows deleted from distribution \
queue, Source DB %d, Transaction ID %d, Target DB %d. Server will shutdown"},
	{1025, 0, "EVENT: Getting DB Event."},
	{1034, 0, "Entering %s()."},
	{1044, 0, "ERROR_CHECK:  Too many errors -- aborting with error count o\
f %d."},
	{1064, 0, "OPEN_DB:  Local database '%s' is open."},
	{1069, 0, "OPEN_DB:  Selecting local database number."},
	{1073, 0, "OPEN_CONN:  Database '%s' is open."},
	{1074, 0, "DBMS error %d selecting local database name.  %s."},
	{1077, 0, "OPEN_DB:  Local database information not found -- Aborting"},
	{1080, 0, "TRANSMIT:  Unknown transaction type %d with Source DB %d, \
Transaction ID %d, Sequence %d."},
	{1081, 0, "TRANSMIT:  Committing Transaction ID %d"},
	{1126, 1, "D_INSERT: DBMS error occurred while selecting record Source \
DB %d, Transaction ID %d, Sequence %d, table '%s' from shadow table."},
	{1126, 2, "D_INSERT: Record Source DB %d, Transaction ID %d, Sequence \
 %d, table '%s' not found in shadow table."},
	{1126, 3, "D_INSERT: Too many records found in shadow table for record \
Source DB %d, Transaction ID %d, Sequence %d and table '%s'."},
	{1128, 1, "D_INSERT: DBMS error finding record Source DB %d, \
Transaction ID %d, Sequence %d in table '%s'."},
	{1128, 2, "D_INSERT: Record Source DB %d, Transaction ID %d, Sequence \
%d not found in table '%s'."},
	{1128, 3, "D_INSERT: Key for Record Source DB %d, Transaction ID %d, \
Sequence %d found more than once in table '%s'."},
	{1132, 1, "D_INSERT: DBMS error selecting record Source DB %d, \
Transaction ID %d, Sequence %d out of archive for table '%s'."},
	{1132, 2, "D_INSERT: Record Source DB %d, Transaction ID %d, Sequence \
%d, not found in archive for table '%s'."},
	{1132, 3, "D_INSERT: Too many records for Source DB %d, Transaction ID \
%d, Sequence %d, found in archive for table '%s'."},
	{1134, 0, "D_INSERT: DBMS error inserting record Source DB %d, \
Transaction ID %d, Sequence %d into Target DB %d on table '%s'."},
	{1134, 1, "D_INSERT: DBMS error inserting record Source DB %d, \
Transaction ID %d, Sequence %d into Target DB %d on table '%s'."},
	{1134, 2, "D_INSERT: No rows inserted while inserting record Source DB \
%d, Transaction ID %d, Sequence %d, into Target DB %d, on table '%s'."},
	{1134, 3, "D_INSERT: Too many rows inserted when inserting record \
Source DB %d, Transaction ID %d, Sequence %d, into Target DB %d, on table \
'%s'."},
	{1136, 0, "D_INSERT: DBMS error inserting record Source DB %d, \
Transaction ID %d, Sequence %d into Target DB %d, shadow table '%s'."},
	{1136, 1, "D_INSERT: DBMS error inserting record Source DB %d, \
Transaction ID %d, Sequence %d into Target DB %d, shadow table '%s'."},
	{1136, 2, "D_INSERT: Record Source DB %d, Transaction ID %d, Sequence \
%d not inserted into shadow table on Target DB %d shadow table '%s'."},
	{1136, 3, "D_INSERT: Too many record inserted for record Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d into shadow table '%s'."},
	{1142, 0, "D_INSERT: Inserting into table '%s'."},
	{1144, 0, "D_INSERT: Inserting into remote dd_input_queue."},
	{1148, 0, "D_UPDATE: Updating '%s' table in database %d."},
	{1158, 0, "CHECK_COMMIT_LOG:  Log entry for Source DB %d, Transaction \
ID %d is inconsistent -- missing entry lower than %d."},
	{1159, 0, "CHECK_COMMIT_LOG:  Two phase commit in progress for Source \
DB %d, Transaction ID %d, stage %d."},
	{1160, 0, "COMPRESS_ENTRIES:  Too many transactions to recover."},
	{1167, 0, "OPEN_COMMIT_LOG:  Cannot open two phase commit log file -- A\
BORTING."},
	{1212, 0, "%s:  DBMS error %d raising event.  %s."},
	{1214, 0, "COMMIT:  Error preparing to commit local session."},
	{1215, 0, "COMMIT:  Error committing remote session."},
	{1216, 0, "COMMIT:  Error rolling back local session."},
	{1220, 0, "D_DELETE:  For Source DB %d, Transaction ID %d, Sequence \
%d, Target DB %d, table '%s' -- error in archive append."},
	{1220, 1, "D_DELETE:  Error in archive append for Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1220, 2, "D_DELETE:  No rows appended to archive table for Source DB \
%d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1220, 3, "D_DELETE:  More than 1 row added to the archive table for \
Source DB %d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1221, 0, "D_DELETE:  For Source DB %d, Transaction ID %d, Sequence \
%d, Target %d, table '%s' -- error in repeated delete."},
	{1222, 0, "D_DELETE:  Error in archive update for Source DB %d, \
Transaction ID %d, Sequence %d Target DB %d table '%s'."},
	{1226, 0, "D_UPDATE: DBMS error in inserting into archive table (this \
is a possible collision) for Source DB %d, Transaction ID %d, Sequence %d, \
Target DB %d, table '%s'."},
	{1226, 2, "D_UPDATE: No rows inserted into archive table for Source DB \
%d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1226, 3, "D_UPDATE: Too many records inserted into archive table for \
Source DB %d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1227, 0, "D_UPDATE:  Error in first repeated update for Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1227, 1, "D_UPDATE:  Error in first repeated update for Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1227, 2, "D_UPDATE:  Row in target table is missing for Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1228, 0, "D_UPDATE:  Error updating in_archive in shadow table, \
Source DB %d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1228, 1, "D_UPDATE:  Error updating in_archive in shadow table, \
Source DB %d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1229, 0, "D_UPDATE:  Error in insert into shadow table Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1229, 1, "D_UPDATE:  Error in insert into shadow table Source DB %d, \
Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1230, 0, "D_INSERT:  Error in insert into remote dd_input_queue for \
Source DB %d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1234, 1, "GET_T_NO:  Error updating next transaction ID."},
	{1249, 0, "EVENT:  DBMS error %d selecting events -- aborting.  %s."},
	{1250, 0, "EVENT:  No events found -- aborting."},
	{1251, 0, "EVENT:  Registering dbevent '%s' with action %d."},
	{1252, 0, "EVENT:  DBMS error %d registering event '%s'.  %s."},
	{1253, 0, "EVENT:  '%s' dbevent received with text '%s'."},
	{1254, 0, "EVENT:  Found dbevent '%s' with action %d."},
	{1256, 0, "EVENT:  Processing event '%s' with action %d."},
	{1258, 0, "D_INSERT: Starting INSERT with Source DB %d, Transaction ID \
%d, Sequence %d, Target DB %d."},
	{1260, 0, "D_UPDATE: Starting UPDATE with Source DB %d, Transaction ID \
%d, Sequence %d, Target DB %d."},
	{1262, 0, "INIT_LIST: top %d, atop %d"},
	{1263, 0, "LIST_TOP: top %d, atop %d"},
	{1264, 0, "NEW_NODE-1: top %d, atop %d"},
	{1265, 0, "NEW_NODE-2: top %d, atop %d, mark %d"},
	{1266, 0, "Error allocating memory for record list: Aborting"},
	{1268, 0, "D_DELETE: Starting DELETE with Source DB %d, Transaction ID \
%d, Sequence %d, Target DB %d."},
	{1270, 0, "SHUTDOWN:  The server has shutdown."},
	{1275, 0, "READ_Q:  Nothing in distribution queue for active targets."},
	{1276, 0, "READ_Q: %d records read from distribution queue."},
	{1277, 0, "COMMIT: Prepare to commit.  The high value is %d, The low \
value is %d"},
	{1281, 0, "D_MONITOR:  Alerting monitor type %d on %s."},
	{1282, 0, "D_MONITOR:  Unknown monitor type %d"},
	{1289, 0, "MAILLIST:  Mailing '%s'."},
	{1343, 0, "CHECK_INSERT:  Error %d getting columns for '%s.%s'."},
	{1344, 0, "CHECK_INSERT:  No registered columns for %s."},
	{1349, 0, "CHECK_INSERT:  Integer execute immediate failed with error \
%d."},
	{1350, 0, "CHECK_INSERT:  Float execute immediate failed with error \
%d."},
	{1351, 0, "CHECK_INSERT:  Char execute immediate failed with error \
%d."},
	{1370, 0, "RECOVER_FROM_LOG:  Commit log file inconsistent."},
	{1377, 1, "PRE_CLEAN:  Error checking for benign collision for table \
'%s.%s', trans type %d, Source DB %d, Transaction ID %d, Sequence %d."},
	{1378, 0, "PRE_CLEAN:  Removing benign collision for table '%s.%s', \
trans type %d, Source DB %d, Transaction ID %d, Sequence %d."},
	{1379, 0, "PRE_CLEAN:  Invalid trans type %d for table '%s.%s', Source \
DB %d, Transaction ID %d, Sequence %d."},
	{1381, 0, "PRE_CLEAN: Benign collision detected for table '%s.%s', \
trans type %d, Source DB %d, Transaction ID %d, Sequence %d."},
	{1383, 1, "RESOLVE_BY_TIME:  execute immediate checking for primary key\
 failed."},
	{1384, 1, "RESOLVE_BY_TIME:  Error obtaining the local transaction \
time."},
	{1385, 1, "RESOLVE_BY_TIME:  Error obtaining difference between the \
transaction times"},
	{1386, 1, "RESOLVE_BY_TIME:  Error deleting remote record from table \
'%s.%s'."},
	{1389, 1, "RESOLVE_BY_TIME:  Error obtaining new trans_time."},
	{1391, 1, "RESOLVE_BY_TIME:  Error obtaining remote shadow record for \
table '%s.%s', Source DB %d, Transaction ID %d, Sequence %d."},
	{1392, 0, "%s collision found for table '%s.%s', Source DB %d, \
Transaction ID %d, Sequence %d."},
	{1394, 0, "Ingres Replicator error"},
	{1515, 0, "GET_COMMIT_LOG_ENTRIES:  Garbage %s in commit.log."},
	{1523, 0, "CHECK_UNIQUE_KEYS:  DBMS error %d looking for table '%s.%s\
' in database %s."},
	{1524, 0, "CHECK_UNIQUE_KEYS:  Error %d looking up columns for table '%\
s.%s' in database %s."},
	{1525, 0, "CHECK_INDEXES:  Fetch of index names for table '%s.%s' faile\
d with error %d."},
	{1527, 0, "CHECK_INDEXES:  Too many unique indexes for table '%s.%s'."},
	{1528, 1, "D_UPDATE:  Error getting old key values on update."},
	{1528, 2, "D_UPDATE:  No rows retrieved in getting old key values on \
update."},
	{1528, 3, "D_UPDATE:  Too many rows retrieved in getting old key \
values on update."},
	{1536, 0, "Table '%s.%s' does not have unique keys enforced in '%s'."},
	{1556, 1, "D_DELETE:  Target type 3 had an error deleting from '%s'."},
	{1556, 2, "D_DELETE:  Row in target table '%s' is missing."},
	{1559, 0, "Cannot resolve collisions for target types other than 1 or 2\
 (database number %d)."},
	{1560, 1, "D_UPDATE:  Inserting instead of updating for '%s' in type 3 \
database failed."},
	{1571, 1, "D_INSERT:  Error executing procedure '%s'."},
	{1572, 1, "D_UPDATE:  Error executing procedure '%s'."},
	{1573, 1, "D_DELETE:  Error executing procedure '%s'."},
	{1581, 0, "Server %d:  %s"},
	{1588, 1, "RESOLVE_INSERT:  Error updating shadow table '%s.%s'."},
	{1590, 1, "RESOLVE_INSERT:  Error inserting into archive table."},
	{1590, 3, "RESOLVE_INSERT:  Too many rows inserted into archive table."},
	{1614, 1, "RESOLVE_MISSING_UPDATE:  Error obtaining old replication key\
 from table '%s.%s' where %s"},
	{1614, 2, "RESOLVE_MISSING_UPDATE:  Error obtaining old replication \
key from table '%s.%s' where %s"},
	{1614, 3, "RESOLVE_MISSING_UPDATE:  Too many rows returned while \
obtaining old replication key from table '%s.%s' where %s"},
	{1615, 1, "RESOLVE_MISSING_UPDATE:  Error obtaining count in %s where %\
s."},
	{1616, 1, "RESOLVE_MISSING_UPDATE:  Error getting maximum transaction t\
ime from %s where %s"},
	{1617, 1, "RESOLVE_MISSING_UPDATE:  Error obtaining trans type from %s \
where %s"},
	{1618, 1, "RESOLVE_MISSING_UPDATE:  Error obtaining current replication\
 key from %s where old replication key = (%d, %d, %d)"},
	{1618, 2, "RESOLVE_MISSING_UPDATE:  No rows returned in obtaining current\
 replication key from %s where old replication key = (%d, %d, %d)"},
	{1618, 3, "RESOLVE_MISSING_UPDATE:  Too many rows returned in obtaining\
 current replication key from %s where old replication key = (%d, %d, %d)"},
	{1640, 0, "Error updating pid in dd_servers table."},
	{1652, 0, "READ_Q: DBMS error %d trying to read distribution queue -- \
%s."},
	{1655, 0, "D_ERROR:  Error communicating with database '%s'."},
	{1657, 1, "D_INSERT:  Error updating in_archive in shadow table, \
Source DB %d, Transaction ID %d, Sequence %d, Target DB %d, table '%s'."},
	{1695, 0, "QBM limit has been exceeded for Source DB %d, Transaction \
ID %d. Increase QBM and restart the Replicator Server."},
	{1705, 0, "CHECK_COMMIT_LOG:  Unrecoverable commit in progress Source \
DB %d, Transaction ID %d, stage %d."},
	{1710, 0, "INIT_CDDS_CACHE:  Error obtaining CDDS count."},
	{1711, 0, "INIT_CDDS_CACHE:  No CDDSs are defined."},
	{1712, 0, "INIT_CDDS_CACHE:  Error allocating CDDS cache."},
	{1713, 0, "INIT_CDDS_CACHE:  Error selecting CDDS information."},
	{1716, 0, "LKUP_CDDS:  Attempt to look up CDDS %d prior to cache initia\
lization."},
	{1718, 0, "QUIET_DB:  Setting target database %d to quiet."},
	{1719, 0, "QUIET_UPDATE:  Error %d updating dd_db_cdds.is_quiet. %s"},
	{1722, 0, "UNQUIET_DB:  Unquieting target database %d."},
	{1726, 0, "QUIET_CDDS:  Setting CDDS %d to quiet."},
	{1727, 0, "QUIET_CDDS:  Setting database %d CDDS %d to quiet."},
	{1731, 0, "UNQUIET_CDDS:  Unquieting CDDS %d."},
	{1732, 0, "UNQUIET_CDDS:  Unquieting database %d CDDS %d."},
	{1736, 0, "UNQUIET_ALL:  Unquieting all databases and CDDSs."},
	{1739, 0, "READ_Q:  Table %d unknown to this server -- row ignored."},
	{1740, 0, "READ_Q:  CDDS %d unknown to this server -- row ignored."},
	{1742, 0, "REPLICAT:  Error initializing CDDS cache -- ABORTING."},
	{1788, 0, "COLLISION:  Invalid collision mode of %d for table '%s.%s', \
Source DB %d, Transaction ID %d, Sequence %d."},
	{1789, 0, "D_MONITOR:  Unexpected database no %d."},
	{1791, 0, "CHECK_CONNECTIONS:  Closing inactive connection to %s::%s."},
	{1793, 0, "CLOSE_CON:  Attempt to close connection %d prior to initiali\
zation."},
	{1794, 0, "%s:  DBMS error %d counting number of targets.  %s"},
	{1795, 0, "%s:  Error allocating Connections array."},
	{1796, 0, "%s:  DBMS error %d selecting connection information.  %s."},
	{1798, 0, "%s:  DBMS error %d commiting.  %s."},
	{1799, 0, "LKUP_CON:  Attempt to look up connection for database %d pri\
or to connections initialization."},
	{1800, 0, "OPEN_CON:  Attempt to open connection %d prior to initializa\
tion."},
	{1801, 0, "OPEN_CON:  DBMS error %d connecting to database '%s' for \
session %d.  %s."},
	{1803, 0, "OPEN_CON:  DBMS error %d connecting to database '%s' ident\
ified by '%s' for session %d.  %s."},
	{1807, 0, "OPEN_CON:  DBMS error %d setting lockmode on database '%s'\
 identified by '%s' for session %d.  %s."},
	{1809, 0, "OPEN_RECOVER_CON:  Attempt to open recovery connection prior\
 to initialization."},
	{1810, 0, "OPEN_RECOVER_CON:  DBMS error %d connecting to database \
'%s' for recovery, session %d.  %s."},
	{1812, 0, "CHECK_FLAGS:  Required startup flag: -IDB not set."},
	{1813, 0, "CHECK_FLAGS:  Required startup flag: -OWN not set."},
	{1814, 0, "CHECK_FLAGS:  Required startup flag: -SVR not set or assigne\
d an invalid value."},
	{1816, 0, "%s:  Error generating object name for table '%s'."},
	{1845, 0, "RECOVER_FROM_LOG:  Error recovering log, case %d."},
	{1846, 0, "DO_LOCAL_COMMIT:  Error opening recovery connection."},
	{1847, 0, "DO_LOCAL_COMMIT:  Error %d doing commit."},
	{1848, 0, "DO_LOCAL_ROLLBACK:  Error opening recovery connection."},
	{1849, 0, "DO_LOCAL_ROLLBACK:  Error %d doing commit."},
	{1850, 0, "check_remote_db:  Cannot find recovery connection."},
	{1852, 0, "CHECK_REMOTE_DB:  Error opening remote recovery connection."},
	{1854, 0, "CHECK_REMOTE_DB:  Error %d executing query."},
	{1855, 0, "CHECK_REMOTE_DB:  Error %d executing commit or rollback."},
	{1868, 0, "READ_Q:  Lost connection to local database -- ABORTING."},
	{1900, 0, "Cannot allocate memory for temporary buffer -- ABORTING."},
	{1901, 0, "IIRSgrd_GetRowData: DBMS error %d setting shared lock on \
table %s.%s while retrieving row data. '%s'."},

};


static STATUS lkup_ermsg(i4 msg_num, short err_code, RP_ERMSG **ermsg_ptr);


/*{
** Name:	get_message_text - get message text
**
** Description:
**	Returns text of a "message" (see messageit) with parameters
**	filled in.
**
** Inputs:
**	msg_num		- the message number
**	err_code	- error code
**	...		- variable argument list corresponding to format
**			  control string conversion specifications
**
** Outputs:
**	msg_text	- the formatted text of the message
**
** Returns:
**	none
**
** Side effects:
**	msg_text is filled in.
*/
void
get_message_text(
char	*msg_text,
i4	msg_num,
short	err_code,
va_list	ap)
{
	RP_ERMSG	*ermsg_ptr;

	if (lkup_ermsg(msg_num, err_code, &ermsg_ptr) != OK)
		STprintf(msg_text, ERx("Unknown error message: %d"), msg_num);
	else
		vsprintf(msg_text, ermsg_ptr->msg_text, ap);
}


/*{
** Name:	build_messageit_string - build message string
**
** Description:
**		Formats a message string and also returns a message identifier,
**		and the current date and time of day.
**
** Inputs:
**		msg_num - the message number
**		msg_level - the level (error, warning, etc.)
**		ap - additional arguments to be formatted
**
** Outputs:
**		msg_prefix - the message prefix
**		timestamp - the current date and time
**		msg_text - the formatted text of the message
**
** Returns:
**		none
**
** Side effects:
**		msg_text is filled in.
*/
void
build_messageit_string(
char	*msg_prefix,
char	*timestamp,
char	*msg_text,
i4	msg_num,
i4	msg_level,
va_list	ap)
{
	SYSTIME	now;

	/* set the time stamp */
	TMnow(&now);
	TMstr(&now, timestamp);

	/* set the message prefix */
	if (msg_level < 1 || msg_level > DIM(Msg_Prefixes))
		msg_level = DIM(Msg_Prefixes);
	STcopy(Msg_Prefixes[--msg_level], msg_prefix);

	/* get the message text */
	get_message_text(msg_text, msg_num, 0, ap);
}


/*{
** Name:	lkup_ermsg - look up an error message in the messages array
**
** Description:
**	Looks up an error message in the messages array.  Each message has
**	a two part key consisting of msg_num and err_code.  A binary
**	search is used.
**
** Inputs:
**	msg_num		- the message number
**	err_code	- the error code
**
** Outputs:
**	ermsg_ptr	- pointer to a RP_ERMSG structure
**
** Returns:
**	OK	Function completed normally.
**	FAIL	No such message was found.
*/
static STATUS
lkup_ermsg(
i4		msg_num,
short		err_code,
RP_ERMSG	**ermsg_ptr)
{
	i4	low, mid, high, i;

	low = 0;
	high = DIM(messages) - 1;

	/* perform the look-up */
	while (low <= high)
	{
		mid = (low + high) / 2;
		i = msg_num - messages[mid].msg_num;
		if (i == 0)
			i = err_code - messages[mid].err_code;
		if (i < 0)
		{
			high = mid - 1;			/* too big */
		}
		else if (i > 0)
		{
			low = mid + 1;			/* too small */
		}
		else
		{
			*ermsg_ptr = &messages[mid];	/* found */
			return (OK);
		}
	}

	*ermsg_ptr = (RP_ERMSG *)NULL;			/* not found */
	return (FAIL);
}
