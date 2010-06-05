/*
**  Copyright (c) 1995, 2010 Ingres Corporation
**
*/

/*
**  Name: ingstop.c
**
**  Usage:
**      ingstop [ -timeout=minutes ] [ -force | -immediate ] [ -kill ] [ -show | -check]
**
**
**  History:
**
**  27-jun-95 (sarjo01)
**      Module created based on UNIX ingstop shell script.
**  11-jul-95 (sarjo01)
**      Added recovery server detect w/ iinamu
**  13-jul-95 (emmag)
**	Replaced call to start_process with a call to system for shutting
**	down the DBMS server.
**  15-jul-95 (emmag)
**	Allocate a NULL DACL and use if for security to give other users
**	access to resources.
**  17-jul-95 (emmag)
**	Took code for cleaning up memory from ingstart.c
**  20-jul-95 (sarjo01)
**      Added detect, stop star servers. Fixed minor bug in stop_dbms()
**  28-jul-95 (tutto01, emmag, canor01)
**	Changed behaviour to match UNIX more closely.   We now support
**	a -kill option which will terminate the DBMS server even if
**	there are dead sessions lying around.  The user will be
**	prompted to issue an 'ingstop -kill' if ingstop fails to shut
**	down a DBMS server.  Also improved the warning messages.
**  20-jul-95 (reijo01)
**      Fixed a bug in the code that caused a check of the wrong process id
**	after a server had been shut down with iimonitor.
**  31-aug-1995 (canor01)
**	Fixed two typos.
**  05-sep-1995 (canor01)
**	Fixed one more typo.
**  05-sep-95 (emmag)
**	Cosmetic changes to informational messages.
**  06-sep-95 (emmag)
**	Check for II_SYSTEM in the environment before proceeding.
**	The wrapper for ingstop was getting all of the output
**	flushed on exit and nothing was appearing during the
**	processing of ingstop.   Added SIflush in certain
**	locations so that the display is updated periodically.
**  17-nov-1995 (canor01)
**	If the DBMS server has open sessions, don't continue
**	to shut down recovery and name servers.  If using the
**	"-kill" flag, also set "force" and "immediate."
**  11-dec-1995 (canor01)
**	If the rcp shutdown fails, don't shut down name server.
**	We will need it for any further attempt to shut down
**	recovery server.
**  14-dec-95 (emmag)
**	Add [-show] to the usage message.
**  17-dec-95 (emmag)
**	Add a [ -service ] option to ingstop which skips all of the
**	SIprintf's and SIflush's in an attempt to reduce the time
**	taken to run ingstop from the service control manager.
**	This operates the same way as ingstop -kill, which the
**	service was calling previously.
**  05-jan-1996 (canor01)
**	Change name displayed for STAR server from "iidbms" to "iistar"
**  16-Feb-1996 (wonst02)
** 	In Win 95, also shut down the gateway server, iigws.
**  05-mar-1996 (emmag)
**	Don't attempt to remove shared memory if this was a client
**	installation.  This will prevent "User does not have SERVER CONTROL
**	PRIVILEGES" messages appearing when shutting down a client
**	installation.
**  21-mar-1996 (canor01)
**	Even if we don't detect a recovery server, it may be lost to
**	the name server, so send it a shutdown message anyway.
**  17-may-96 (emmag)
**	On Windows '95, don't mention the dbms, rcp, acp or star servers.
**  21-may-96 (emmag)
**	Ingstop -service should operate like a regular ingstop.
**  24-jul-96 (emmag)
**      Guarantee the order of the messages printed when removing
**      shared memory, by flushing stdout before calling ipcclean.
**  31-jul-96 (emmag)
**	Since we won't have an rcp on '95, the call to rcpconfig
**	shouldn't be made, thus avoiding a plethora of messages in
**      the errlog.
**  06-sep-96 (mcgem01)
**      The shutting down of the gateway service should be a stealth
**      operation;  remove the gateway messages.
**  07-sep-96 (mcgem01)
**	OpenIngres Desktop is being ported to NT and should behave
**	exactly as it does on '95.   Add a -desktop option which
**	will cause us to mimic Windows '95 behaviour.
**  30-sep-96 (mcgem01)
**	Back out changes submitted by the Jasmine team in error.
**  30-oct-96 (somsa01)
**	Added a -sdk flag for OpenIngres SDK, which will be similar to
**	the -desktop flag.
**  13-nov-1996 (wilan06)
**      Users can set the "server_class" parameter in CBF for the DBMS
**      and for Star. This is "INGRES" or "STAR" by default, but can be
**      any string. Ingstop needs to query these parameters so it can
**      stop the DBMS and Star if the class name has been changed.
**  14-nov-96 (mcgem01)
**	A bug in propagate put <<<< at the end of this file.
**  09-jan-97 (mcgem01)
**	The five seconds allowed for the server to shutdown isn't always
**	sufficient, increase this to ten seconds and print dots on the
**	screen at one second intervals so the user doesn't think that
**	the shutdown has hung.
**  13-jan-97 (mcgem01)
**	Clean up message formatting.
**  20-feb-97 (mcgem01)
**	Remove the call to "rcpconfig -shutdown" when no RCP's are detected as
**	this is no longer required and can lead to misleading messages in
**	the error log.
**  04-apr-1997 (canor01)
**	Use generic system names from gv.c to allow program to be
**	used in both Jasmine and OpenIngres.
**  20-may-97 (mcgem01)
**	Clean up compiler warnings.
**  23-may-97 (mcgem01)
**	IPCCLEAN was not being called on NT when the shutdown was successful.
**	Some weird logic which caused this has been removed and instead we'll
**	always call ipcclean if there aren't any dbms/star/recovery/jasmine
**	servers running.
**  31-may-97 (mcgem01)
**      Even if we don't detect a recovery server, it may be lost to
**      the name server, so send it a shutdown message anyway.
**  01-jun-97 (mcgem01)
**	Forgot about Jasmine in previous submission - since Jasmine doesn't
**	have an rcp, we should not call rcpconfig if we're a jasmine
**	installation.
**  10-jun-97 (mcgem01)
**	Jasmine doesn't have an RCP or star server so don't bother
**	trying to shut them down.
**  17-jun-1997 (canor01)
**	Give the dbms server a chance to shutdown cleanly.  It may have to
**	do some cleanup first.
**  23-jun-1997 (canor01)
**	Do not ask the name server about Star or recovery servers in
**	a Jasmine installation.
**  3-jul-1997 (canor01)
**	Run ipcclean from installation directory.
**  11-aug-97 (rigka01)
**	check that directory specified in II_TEMPORARY exists; if it doesn't
**      use default II_TEMPORARY which is current directory
**  19-aug-1997 (chudu01)
**	Remove -force and -immediate options from Jasmine usage.
**  08-sep-1997 (chudu01)
**      renamed utilities like ipcclean to jasclean.
**  15-sep-1997 (chudu01)
**      Jasmine uses jasnetu not netu.
**  23-sep-1997 (canor01)
**	Genericize use of jasnetu and jasclean.  If not enough memory
**	to run jasnamu/iinamu, give error.
**  01-Oct-97 (fanra01)
**      Add the shutdown of rmcmd.
**  30-oct-1997 (chudu01)
**      Bug 85528 - jasmine as a service can never be stopped.
**      The jasproc executable calls jasstop with the -service
**      flag.  Since this flag was dissabled for Jasmine, the
**      jasstop command would return without any work being
**      done.  Re-enabled the -service flag.
**  15-nov-97 (mcgem01)
**	Only call rcpconfig, if there is a dmfrcp process running. (84917)
**  12-jan-1998 (reijo01)
**      Fix Bug 87000. Remove hard-coded ingres names, replace with generic
**      system contants.
**  18-feb-98 (mcgem01)
**	Set correct exit code.  Also, set the rcp_up flag correctly in
**	stop_rcp.   Previously, it was set to TRUE, if no RCP's were found
**	in the installation.
**  25-mar-1998 (somsa01)
**	If there are any other DBMS servers started in other server classes,
**	shut them down as well.  (Bug #89972)
**  26-mar-98 (mcgem01)
**	Add support for the Oracle, Sybase, Informix, SQL Server and ODBC
**	gateways.   We only report on these, if they've actually been
**	installed i.e. there is an ingstart count for the gateway in config.dat
**  12-apr-98 (mcgem01)
**	Gateway shutdown is done on a per-class basis and not via the listen
**	address.   Also fixed a typo in each of the gateway shutdown messages.
**  20-Aug-98 (fanra01)
**      Bugs 87664
**      Using a force would not stop the server with outstanding sessions.
**      If a normal shut of the DBMS does not work issue a stop server.
**  25-Aug-98 (fanra01)
**      Corrected typeo in condition for message.
**  07-Sep-98 (fanra01)
**      Add icesvr support.
**  23-Sep-98 (rajus01)
**    Added gcn_server_class.
**  11-dec-1998 (somsa01)
**      If the NET server does not come down after sending a Quiesce, then
**      bump the iigcc_count back up. Also, in the case of a normal shutdown
**      which is aborted due to the NET server still being up, do not run
**      ipcclean if all other DBMS and Star servers shut down. Also, in the
**      case of the DBMS, Star, or NET servers still being up, print out the
**      messages which are printed out on UNIX in these cases.  (Bug #94508)
**  31-dec-1998 (mcgem01)
**	Provide a mechanism for shutting down a protocol bridge - remarkably
**	similar to the gcc code.
**  05-feb-1999 (somsa01)
**	Paths may contain spaces. Therefore, put double quotes around paths
**	which are used to execute system commands.
**  11-feb-1999 (somsa01)
**	Dynamically allocate space for cmdbuf wherever ii_temp is involved,
**	so that we accurately use the correct path size all of the time.
**  08-Mar-99 (fanra01)
**      Change icesvr_key from ICE to ICESVR for consistency.
**  31-mar-1999 (somsa01)
**	Localized printable strings.
**  01-apr-1999 (somsa01)
**	In stop_gcc(), on Windows 95, use PCsearch_Pid() to see if a gcc is
**	running. OpenProcess() still brings back a valid handle to the gcc
**	process EVEN though it has exited!
**  19-apr-1999 (cucjo01)
**	Added newline after shutting down the ICE Server
**  27-apr-1999 (mcgem01)
**      Updated for the EDBC project. Also corrected the usage of
**      SystemCatPrefix which should not have been used here, replacing it
**      with SystemAltExecName.
**  14-jun-1999 (somsa01)
**	Added stopping of individual pieces (iigcn, iigcc, ...).
**  03-nov-1999 (somsa01)
**	If the user stopping Ingres is "system", then rcpstat will fail
**	since the service is shutting down. Print an appropriate message.
**  04-nov-1999 (mcgem01)
**	Message S_ST0542 was renumbered to avoid a conflict in the 2.0
**	codeline.
**  19-jan-2000 (mcgem01)
**	The EDBC version of ingstop should check EDBC_ROT.
**  22-jan-2000 (somsa01)
**	After all is done, delete any remaining temp files created from the
**	SETUID funtionality. If these files are created to capture output
**	from processes that were still running (such as the iigcn), the
**	temp files would not have been normally deleted as the spawned
**	processes still had the handles to those files opened. Also, if we
**	are not shutting down individual pieces, shut down the service too.
**  25-jan-2000 (mcgem01)
**	Send the ingstop messages to the ingstart.log file in the
**	%II_SYSTEM%\ingres\files directory.
**  28-jan-2000 (somsa01)
**	When printing out the OS error from the service shutdown, print
**	out the OS error text as well.
**  31-jan-2000 (mcgem01)
**	If we detect that Ingres is running as a service, stop the service.
**	Obviously this applies to NT and Windows 2000 only.
**  11-feb-2000 (somsa01)
**	Output more messages to the ingstart.log file.
**  30-mar-2000 (somsa01)
**	Added shutdown of the JDBC server. Also, use 'iigcstop' to shutdown
**	the NET and BRIDGE servers instead of 'netu'.
**  03-may-2000 (mcgem01)
**      Flush the ingstart.log file to disk after writing to it.
**  14-jun-2000 (mcgem01)
**	The EDBC service name is EDBC_Client_<inst id>
**  21-sep-2000 (mcgem01)
**      More nat to i4 changes.
**  20-oct-2000 (somsa01)
**	Replaced the usage of PCget_PidFromName() with PCsearch_Pid() when
**	obtaining the status of RMCMD.
**  22-dec-2000 (somsa01)
**	In stop_rcp(), corrected printout of S_ST0543_cannot_run_rcpstat2.
**  02-apr-2001 (rodjo04) SIR 104368
**	Added case for SERVICE_CONTROL_STOP_NOINGSTOP.
**  05-jun-2001 (rodjo04) Bug 104851
**	Ingstop is ignoring the parameters such as -kill. Added else statement
**	to conditional.
**  04-may-2001 (loera01) Bug 104646
**      If service boolean is set to TRUE via "ing[edbc]stop -service",
**      be sure to stop the service manager.
**  26-sep-2001 (somsa01)
**	In stop_jdbc(), on Windows 98 / ME, use PCsearch_Pid() to see if a
**	gcc is running. OpenProcess() still brings back a valid handle to
**	the gcc process EVEN though it has exited!
**  17-jan-2002 (somsa01)
**	If we're being run via the Service Manager, or the Ingres service
**	is currently started, do not print out S_ST0543_cannot_run_rcpstat2
**	when necessary.
**  08-may-2002 (mofja02)
**      Added support for db2udb.
**  02-dec-2002 (somsa01)
**	In the individual shutdown case, only run ipcclean if the recovery
**	server was being shut down (and it did shut down).
**  19-nov-2002 (mofja02)
**	I don't think ingstop -gateway has ever worked.  PMinit and PMload
**	were not being called before PMget.  Bug number is: 108875
**  26-nov-2002 (mofja02)
**	Removed code that prints out a message, as PMload puts out its own
**	message when it fails.  Bug number is still: 108875
**	Also modified the PMload call to pass a pointer to PMerror.
**	To do this I needed a prototype.  To get a prototype, I included
**	util.h.  When I include util.h, I needed to remove #define
**	BIG_ENOUGH because it is defined in util.h.
**  27-Feb-2003 (fanra01)
**      Bug 109284
**      Add flag to invocation of rmcmdstp.exe during a forced shutdown
**      from a service.
**      Also add a invocation of rmcmdstp.exe to execute a procedure in
**      imadb even if rmcmd is not running.
**	19-Mar-2003 (penga03)
**	    Addition fix for Bug 109284; in stop_rmcmd(), do not execute
**	    cmdbuf when it is not assigned to anything.
**  26-Mar-2003 (wansh01)
**	Added support for DAS.
**  26-Feb-2004 (wonst02)
** 	Added "-check" flag for Windows cluster High Availability support.
** 	Also fixed bug that shut down the service even if "-show" was specified.
**  14-jun-2004 (somsa01)
**	Cleaned up code for Open Source.
**	11-aug-2004 (penga03)
**	    Added an ending message when ingstop completes.
**	14-Sep-2004 (drivi01)
**	    Modified ipcclean to ipcclean.exe and modified code
**	    to search for it in II_SYSTEM/ingres/bin first and
**	    then in II_SYSTEM/ingres/utility.
**	17-sep-2004 (somsa01)
**	    ANy administrator can now shut down Ingres.
**	20-Sep-2004 (drivi01)
**	    In get_sys_dependencies added double quotes
**	    around a path to ipcclean.exe in case if
**	    II_SYSTEM has spaces in it.
**	5-Nov-2004 (drivi01)
**	    Added code for checking user privileges early in ingstop proccess
**	    instead of delaying "No SERVER_CONTROL privilege" message to the end.
**	15-Nov-2004 (drivi01)
**	    Moved string messages for change #473247 to erst.msg.
**	15-Jan-2005 (drivi01)
**	    Added routines for proccessing a new cbf value ii.$.prefs.ingstop.
**	    ii.$.prefs.ingstop can be set to -force or -f and -kill or -k, and if
**	    it's set ingstop will process it as one of its parameters, however
**	    whatever is passed at the command line to ingstop overwrites the cbf
**	    value.  Also added a new flag -try, which will disable cbf settings.
**	    The priority of proccessing the flags is specified in the following
**	    table:  (Assume O is no flag)
**
**		  CMD
**
**		   | O |try|-f | -i| -k|
**		___|___|___|___|___|___|___|___|
**		 O | O | O | -f| -i| -k|   |   |
**	CBF	___|___|___|___|___|___|___|___|
**		 -f| -f| O | -f| -i| -k|   |   |
**		___|___|___|___|___|___|___|___|
**		 -k| -k| O | -f| -i| -k|   |   |
**		___|___|___|___|___|___|___|___|
**
**	15-Jan-2005 (drivi01)
**	    Replaced stricmp with STcasecmp.
**	28-Mar-2005 (drivi01)
**	    Modified stop_dbms procedure to Terminate process in a
**	    similar fashion as kill if dbms server was stopped using
**	    "stop server" to avoid failure during force shut down if
**	    an sql session is open.
**	12-Apr-2005 (penga03)
**	    Added code to make "ingstop" cluster-aware.
**	05-Jul-2005 (drivi01)
**	    Modified stop_dbms and stop_gcc to better handle shut down
**	    of a client that was started by the system account.
**	27-Jul-2005 (drivi01)
**	    Increased the size of buffer in get_procs() function.
**	    The length of buffer we were allocating was too small
**	    to account for two temporary pathes with long II_SYSTEM
**	    path.
**  	08-Aug-2005 (lauri01)
**          Put OfflineClusterResource() call in a loop to handle
**          cases where the state of the Ingres resource may not be
**          ready to come offline yet so we keep trying until it
**          succeeds or the timeout is reached.
**	08-Aug-2005 (drivi01 on behalf of lauri01)
**	    Declared rc variable to store return code of
**	    OfflineClusterResource.
**	10-Aug-2005 (penga03 on behalf of lauri01)
**	    Increased the timeout.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**      08-08-2006 (huazh01)
**          Avoid shutting down duplicated dbms server Id shown in 'iinamu',
**          which is possible when using db list.
**          This fixes bug 116274.
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package.
**	30-Jul-2007 (smeke01) b118652
**	    Prevent shutdown message when we're doing -check or -show.
**	30-Aug-2007 (drivi01)
**	    On Vista, user requires elevated privileges to run this program.
**	    If user is running with reduced privileges, this program will
**	    exit.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    DBA Tools service was renamed to Ingres_DBATools_<instance id>
**	    and this file was updated to check for this service name
**	    if Ingres service isn't found.
**	07-May-2008 (whiro01)
**	    More reliable detection of dmfrcp termination so shared memory can
**	    reliably removed without causing essentially spurious errors.
**	    Some cleanup to use CL routines for string operations.
**	11-Jun-2008 (whiro01)
**	    Increased timeout for dmfrcp going away from 10 to 30 seconds.
**	07-Aug-2008 (bonro01)
**	    ingstop fails to shutdown rmcmd and iidbms when multiple iidbms
**	    are running and cache_sharing is OFF. The problem is that rmcmdstp
**	    is unable to send the shutdown command to the correct iidbms.
**	    The solution is to try to shutdown all iidbms first and ignore
**	    the first error which is caused by rmcmd still running, then
**	    shutdown rmcmd and the last iidbms.
**      11-Dec-2008 (hanal04) Bug 121353
**          Update usage output to make it consistent with documentation.
**	18-Feb-2009 (whiro01) Bug 121679
**	    When specifying a "connect_id" to shutdown specific instances
**	    of a server, check that the specific instance is actually running
**	    (this gives an indication of a common problem which is giving
**	    an instance name just like 'ingstart' instead of a connect_id).
**	18-Feb-2009 (drivi01)
**	    Add a routine to try to figure out if ingstop is running for
**	    DBA Tools package.  If it is, avoid recovery routines for
**	    stopping recovery server.
**	07-May-2009 (drivi01)
**	    Move clusapi.h up in a header list to clean up errors in
**	    effort to port to Visual Studio 2008.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    With II_GC_PROT=tcpip, ingstop could not find any running
**	    server processes.  Remove prefix containing installation
**	    id and backslash (\) from search criteria (looking at
**	    iinamu output).  With tcp_ip as the local IPC, the listen
**	    addresses are tcp port numbers and not pipe names and so
**	    are not in the format II\svrtype\....  Just use the server
**	    type, which is also displayed in the iinamu output, regardless
**	    of local protocol.
*/

/*
**
NEEDLIBS =  COMPATLIB UTILLIB

DEST = UTILITY
**
*/

# include <compat.h>
# include <gl.h>
# include <gv.h>
# include <sl.h>
# include <iicommon.h>
# include <clusapi.h>
# include <me.h>
# include <lo.h>
# include <pc.h>
# include <si.h>
# include <cv.h>
# include <id.h>
# include <nm.h>
# include <er.h>
# include <cs.h>
# include <cm.h>
# include <st.h>
# include <pm.h>
# include <pmutil.h>
# include <ci.h>
# include <tr.h>
# include <lo.h>
# include <ep.h>
# include <winbase.h>
# include <erst.h>
# include <util.h>
# include <gca.h>

static i4  log_file_len;                 /* log file length */
static char log_file_name[ MAX_LOC + 1 ]; /* log file name   */
static char log_file_path[ MAX_LOC + 1 ]; /* log file location */

static PM_CONTEXT *config;		/* configuration data PM context */

static STATUS ingstop_chk_priv( char *user_name, char *priv_name );
static STATUS write_to_log(char *string);
static STATUS find_process(char *process_name, DWORD pid);
static STATUS wait_for_process_death(char *process_name, DWORD pid, int wait, char *prompt);
static DWORD get_process_pid(char *name);
static void call_iimonitor(const char *cmd, const char *process);
static STATUS shutdown_service();
static STATUS create_command_file();
static STATUS append_to_command_file(const char *line);
static STATUS append_show_to_command_file(const char *cmd);
static STATUS delete_command_file();

# ifdef PRINT
# undef PRINT
# endif
# define PRINT( MSG ) \
    { \
	SIprintf( MSG ); \
	SIflush( stdout ); \
	write_to_log( MSG ); \
    }

# ifdef F_PRINT
# undef F_PRINT
# endif
# define F_PRINT( F_MSG, ARG) \
    { \
        char buffer [BIG_ENOUGH]; \
        STprintf( buffer, F_MSG, ARG ); \
        SIprintf( buffer ); \
        SIflush( stdout ); \
	write_to_log( buffer ); \
    }

# ifdef F_PRINT2
# undef F_PRINT2
# endif
# define F_PRINT2( F_MSG, ARG, ARG2) \
    { \
        char buffer [BIG_ENOUGH]; \
        STprintf( buffer, F_MSG, ARG, ARG2 ); \
        SIprintf( buffer ); \
        SIflush( stdout ); \
	write_to_log( buffer ); \
    }

# ifdef F_PRINT3
# undef F_PRINT3
# endif
# define F_PRINT3( F_MSG, ARG, ARG2, ARG3) \
    { \
        char buffer [BIG_ENOUGH]; \
        STprintf( buffer, F_MSG, ARG, ARG2, ARG3 ); \
        SIprintf( buffer ); \
        SIflush( stdout ); \
	write_to_log( buffer ); \
    }

/* Redo macros to include writing to the log file */
# ifdef ERROR
# undef ERROR
# endif
# define ERROR( MSG ) \
    { \
        char buffer [BIG_ENOUGH]; \
        STprintf( buffer, ERx( "\n%s\n\n" ), MSG); \
        SIfprintf( stderr, buffer ); \
        SIflush( stderr ); \
        write_to_log( buffer ); \
        PCexit( FAIL ); \
    }

# ifdef F_ERROR
# undef F_ERROR
# endif
# define F_ERROR( F_MSG, ARG ) \
    { \
        char buffer [BIG_ENOUGH]; \
	char buffer1 [BIG_ENOUGH]; \
        STprintf( buffer, F_MSG, ARG); \
        STprintf( buffer1, ERx( "\n%s\n\n" ), buffer); \
        SIfprintf( stderr, buffer1 ); \
        SIflush( stderr ); \
        write_to_log( buffer1 ); \
        PCexit( FAIL ); \
    }

/*
**  The following data structures are used for maintaining lists of DBMS,
**  Net, Star, and Gateway servers that should be started.
*/

typedef struct process {
	char *name;
	i4 count;
	struct process *next;
} PROCESS_LIST;

typedef struct {
	PROCESS_LIST *dbms_list;	/* DBMS process list */
	PROCESS_LIST *net_list;		/* Net process list */
	PROCESS_LIST *bridge_list;	/* Bridge process list */
	PROCESS_LIST *das_list;		/* DAS process list */
	PROCESS_LIST *star_list;	/* Star process list */
	PROCESS_LIST *icesvr_list;	/* Ice server process list */
	PROCESS_LIST *oracle_list;	/* Oracle Gateway process list */
	PROCESS_LIST *informix_list;	/* Informix Gateway process list */
	PROCESS_LIST *mssql_list;	/* Microsoft SQL Server Gateway */
	PROCESS_LIST *odbc_list;	/* ODBC Gateway process list */
	PROCESS_LIST *sybase_list;	/* Sybase Gateway process list */
	PROCESS_LIST *rms_list;		/* RMS Gateway process list */
	PROCESS_LIST *rdb_list;		/* RDB Gateway process list */
	PROCESS_LIST *db2udb_list;	/* DB2UDB Gateway process list */
	PROCESS_LIST *rmcmd_list;       /* rmcmd process list */
# ifdef NT_GENERIC
	PROCESS_LIST *gateway_list;	/* Local Gateway process list */
# endif
} SERVER_LIST;

static void stop_rcp( BOOL flag );
static void stop_gcn();
static void stop_gws();
static void stop_star( BOOL flag );
static void stop_dbms( BOOL flag );
static void open_dbms( );
static void stop_rmcmd( );
static void stop_icesvr( BOOL flag );
static void stop_oracle( );
static void stop_informix( );
static void stop_sybase( );
static void stop_mssql( );
static void stop_odbc( );
static void stop_db2udb( );
static void open_star( );
static void open_gcc( );
static void open_gcb( );
static void open_gcd( );
static void stop_gcc( BOOL flag );
static void stop_gcb( BOOL flag );
static void stop_gcd( BOOL flag );
static int  get_procs( );
static void find_procs( );
static char *find_key( char *buf, char *key, int *id_len );
static BOOL start_process( char *cmdbuf );
static STATUS execute( char *cmd );
static void get_server_classnames( void );
static void get_sys_dependencies( void );
static BOOL gateway_configured (char * gateway_class);
static void build_server_list( SERVER_LIST *list );
static int  check_server_list( SERVER_LIST *list );
FUNC_EXTERN PID PCget_PidFromName(PTR);
FUNC_EXTERN DWORD PCsearch_Pid(PTR, DWORD);
static BOOL check_cache_sharing (void);

static char *usage = "\nUsage:\n      %sstop [-iigcn|-dmfrcp|-dmfacp|-client|"
                     "-rmcmd|-icesvr|[-iidbms|-iigcc|\n\t-iigcb|-iigcd|-iistar|"
                     "-oracle|-informix|-mssql|-sybase|-db2udb]] "
                     "[-f]\n\t[ -timeout=minutes ] [ -kill ] [ -show | -check] "
                     "[ -force | -immediate ]\n";

# define USAGE() {F_PRINT(usage,SystemExecName);exit(1);}

# define MAX_SERVER_CLASSES 32

/*
** Various defaults for system names.  These can be
** overridden in get_sys_dependencies()
*/
static char iigcn_key[GL_MAXNAME]       = "IINMSVR";
static char gcn_server_class[GL_MAXNAME]= "NMSVR";
static char iigcc_key[GL_MAXNAME]       = "COMSVR";
static char iigcb_key[GL_MAXNAME]       = "BRIDGE";
static char iigcd_key[GL_MAXNAME]       = "DASVR";
static char iigws_key[GL_MAXNAME]       = "OPINGDT";
static int  num_dbms_classes;
static char iidbms_key[MAX_SERVER_CLASSES][GL_MAXNAME];
static char iirecovery_key[GL_MAXNAME]  = "IUSVR";
static int  num_star_classes;
static char iistar_key[MAX_SERVER_CLASSES][GL_MAXNAME];
static char icesvr_key[GL_MAXNAME]      = "ICESVR";
static char iirmcmd_key[GL_MAXNAME]     = "RMCMD";
static char oracle_key[GL_MAXNAME]      = "ORACLE";
static char informix_key[GL_MAXNAME]    = "INFORMIX";
static char mssql_key[GL_MAXNAME]       = "MSSQL";
static char sybase_key[GL_MAXNAME]      = "SYBASE";
static char odbc_key[GL_MAXNAME]        = "ODBC";
static char db2udb_key[GL_MAXNAME]      = "DB2UDB";
static char ingstop[GL_MAXNAME]         = "ingstop";
static char iinamu[GL_MAXNAME]          = "iinamu";
static char iimonitor[GL_MAXNAME]       = "iimonitor";
static char iigwstop[GL_MAXNAME]        = "iigwstop";
static char iigcn[GL_MAXNAME]           = "iigcn";
static char iigcc[GL_MAXNAME]           = "iigcc";
static char iigcb[GL_MAXNAME]           = "iigcb";
static char iigcd[GL_MAXNAME]           = "iigcd";
static char iidbms[GL_MAXNAME]          = "iidbms";
static char icesvr[GL_MAXNAME]          = "icesvr";
static char rmcmd[GL_MAXNAME]           = "rmcmd";
static char dmfrcp[GL_MAXNAME]          = "dmfrcp";
static char iiclean[MAX_LOC+1]          = "ipcclean.exe";
static char iinetu[GL_MAXNAME]          = "netu";
static char iigcstop[GL_MAXNAME]        = "iigcstop";
static char gwstop[GL_MAXNAME]          = "gwstop";
static char oracle[GL_MAXNAME]          = "iigworad";
static char informix[GL_MAXNAME]        = "iigwinfd";
static char mssql[GL_MAXNAME]           = "iigwmssd";
static char sybase[GL_MAXNAME]          = "iigwsybd";
static char odbc[GL_MAXNAME]            = "iigwodbd";
static char db2udb[GL_MAXNAME]          = "iigwudbd";

char        *ii_system;
char        *ii_install;
char        *ii_temp;
LOCATION     ii_temploc;
LOCATION     ii_cmndfile;
char         cmndfile_buf[MAX_LOC+1];
BOOL        dbms_up = FALSE;
BOOL        star_up = FALSE;
BOOL        rcp_up = FALSE;
int         iigcn_count = 0;
char        iigcn_id[64];
int         iigcc_count = 0;
char        iigcc_id[256];
int         iigcd_count = 0;
char        iigcd_id[256];
int         iigcb_count = 0;
char        iigcb_id[256];
int         iigws_count = 0;
char        iigws_id[256];
int         iidbms_count = 0;
char        iidbms_id[256];
int         iirecovery_count = 0;
char        iirecovery_id[256];
int         iistar_count = 0;
char        iistar_id[256];
int         iirmcmd_count = 0;
char        iirmcmd_id[256];
int         icesvr_count = 0;
char        icesvr_id[256];
int         oracle_count = 0;
char        oracle_id[256];
int         informix_count = 0;
char        informix_id[256];
int         mssql_count = 0;
char        mssql_id[256];
int         sybase_count = 0;
char        sybase_id[256];
int         odbc_count = 0;
char        odbc_id[256];
int         db2udb_count = 0;
char        db2udb_id[256];

BOOL        try_flag     = FALSE;
BOOL        service      = FALSE;
BOOL        showonly     = FALSE;
BOOL        checkonly    = FALSE;
BOOL        kill         = FALSE;
BOOL        force        = FALSE;
BOOL        immediate    = FALSE;
BOOL        Is_w95       = FALSE;
BOOL        Is_Jasmine   = FALSE;
BOOL	    Is_DBATools	 = FALSE;
BOOL        servicekill  = FALSE;
BOOL        Individual   = FALSE;
BOOL        shut_rmcmd   = FALSE;
BOOL        shut_oracle  = FALSE;
BOOL        shut_informix= FALSE;
BOOL        shut_sybase  = FALSE;
BOOL        shut_mssql   = FALSE;
BOOL        shut_odbc    = FALSE;
BOOL        shut_db2udb  = FALSE;
BOOL        shut_icesvr  = FALSE;
BOOL        shut_star    = FALSE;
BOOL        shut_dbms    = FALSE;
BOOL        shut_gcc     = FALSE;
BOOL        shut_gcd     = FALSE;
BOOL        shut_gcb     = FALSE;
BOOL        shut_dmfrcp  = FALSE;
BOOL        shut_gws     = FALSE;
BOOL        shut_gcn     = FALSE;
int         minutes      = 0;
char        readbuf[1024], dbms_id[20], star_id[20], gcc_id[20], gcb_id[20];
char        gcd_id[20];
char        tchII_INSTALLATION[3];
char        ServiceName[255];
char        *cmdbuf;
int         cmdbuflen;

BOOL	    bServiceStarted = FALSE;

main( int argc, char *argv[] )
{
    char        	*p;
    int         	i;
    BOOL        	abort_shutdown = FALSE;
    LOCATION 		loc;
    SC_HANDLE		schSCManager, OpIngSvcHandle;
    SERVICE_STATUS	ssServiceStatus;
    LPSERVICE_STATUS	lpssServiceStatus = &ssServiceStatus;
    STATUS status;
    char *temp;
    char tmp_buf[BIG_ENOUGH];
    char user_buf[256];
    char *user;
    OSVERSIONINFO           lpvers;
	char buffer[ 256 ];
	char *host = PMhost();
	char *start_val = NULL;
    static char *TIMEOUT = ERx("-timeout=");

    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);
    Is_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)? TRUE: FALSE;


    get_sys_dependencies();

    PMinit();
    if ((status = PMload (NULL, PMerror)) != OK)
    {
	/* there appears to be no need to output an error message here,
		because when the PMload call fails, it puts out an
		error message */
	PCexit( FAIL );
    }

    STprintf(log_file_name, ERx("%sstart.log"), SystemExecName );
    if ((status = NMloc(FILES, FILENAME, log_file_name, &loc)) != OK)
    {
	SIfprintf( stderr, "Error while building path/filename to %s\n", log_file_name);
	SIflush( stderr );
	PCexit( FAIL );
    }

    LOtos(&loc, &temp);
    STcopy(temp, log_file_path);
    log_file_len = STlength(log_file_path);

    for (i = 1; i < argc; i++)
    {
	if (STcasecmp( argv[i], ERx("-try") ) == 0)
	{
		try_flag = TRUE;
	}
        else if (STcasecmp( argv[i], ERx("-service") ) == 0)
        {
            service = TRUE;
		kill = FALSE;
		force = FALSE;
		immediate = TRUE;
        }
        else if ( (STcasecmp( argv[i], ERx("-show") ) == 0 ||
            STcasecmp( argv[i], ERx("-s") )    == 0 ) && !Is_Jasmine )
        {
            showonly = TRUE;
        }
        else if ( (STcasecmp( argv[i], ERx("-check") ) == 0 ||
            STcasecmp( argv[i], ERx("-c") )    == 0 ) && !Is_Jasmine )
        {
		checkonly = TRUE;	    // Check that all servers are up that are
            showonly = TRUE;	    // supposed to be. Do not shut down anything
        }
        else if ( (STcasecmp( argv[i], ERx("-force") ) == 0 ||
            STcasecmp( argv[i], ERx("-f") )          == 0) && !Is_Jasmine )
        {
            if (force == TRUE)
                USAGE();
            force = TRUE;
            immediate = FALSE;
        }
        else if (STcasecmp( argv[i], ERx("-kill") ) == 0 ||
            STcasecmp( argv[i], ERx("-k") )         == 0)
        {
            kill = TRUE;
	    force = TRUE;
	    immediate = TRUE;
        }
        else if ( (STcasecmp( argv[i], ERx("-immediate") ) == 0 ||
            STcasecmp( argv[i], ERx("-i") )              == 0) && !Is_Jasmine )
        {
            if (force == TRUE)
                USAGE();
            force = TRUE;
            immediate = TRUE;
        }
        else if (STncasecmp( argv[i], TIMEOUT, STlength(TIMEOUT)) == 0)
        {
            minutes = atoi(argv[i] + STlength(TIMEOUT));
            if (minutes == 0)
                minutes = 1;
        }
        else if ( ((STcasecmp( argv[i], ERx("-desktop") ) == 0) ||
		 (STcasecmp( argv[i], ERx("-sdk") ) == 0)) && !Is_Jasmine)
        {
            Is_w95 = TRUE;  /* Act like we're on Windows '95 */
        }
	else if (STcasecmp( argv[i], ERx("-servicekill") ) == 0)
        {
	    kill = TRUE;
	    force = TRUE;
	    immediate = TRUE;
	    servicekill = TRUE;
        }
	else if (STcasecmp( argv[i], ERx("-rmcmd") ) == 0)
	{
	    Individual = TRUE;
	    shut_rmcmd = TRUE;
	}
	else if ( (STcasecmp( argv[i], ERx("-oracle") ) == 0) &&
		  (gateway_configured (oracle_key)) )
	{
	    Individual = TRUE;
	    shut_oracle = TRUE;
	}
	else if ( (STcasecmp( argv[i], ERx("-informix") ) == 0) &&
		  (gateway_configured (informix_key)) )
	{
	    Individual = TRUE;
	    shut_informix = TRUE;
	}
	else if ( (STcasecmp( argv[i], ERx("-sybase") ) == 0) &&
		  (gateway_configured (sybase_key)) )
	{
	    Individual = TRUE;
	    shut_sybase = TRUE;
	}
	else if ( (STcasecmp( argv[i], ERx("-mssql") ) == 0) &&
		  (gateway_configured (mssql_key)) )
	{
	    Individual = TRUE;
	    shut_mssql = TRUE;
	}
	else if ( (STcasecmp( argv[i], ERx("-odbc") ) == 0) &&
		  (gateway_configured (odbc_key)) )
	{
	    Individual = TRUE;
	    shut_odbc = TRUE;
	}
	else if ( (STcasecmp( argv[i], ERx("-db2udb") ) == 0) &&
		  (gateway_configured (db2udb_key)) )
	{
	    Individual = TRUE;
	    shut_db2udb = TRUE;
	}
	else if (STcasecmp( argv[i], ERx("-icesvr") ) == 0)
	{
	    Individual = TRUE;
	    shut_icesvr = TRUE;
	}
	else if (STbcompare( argv[i], 7, ERx("-iistar"), 7, TRUE ) == 0)
	{
	    char	*p1;

	    Individual = TRUE;
	    shut_star = TRUE;
	    p1 = STindex( argv[i], ERx( "=" ), 0 );
	    if (p1 != NULL)
	    {
		CMnext(p1);
		STcopy(p1, star_id);
	    }
	    else
		STcopy("", star_id);
	}
	else if (STbcompare( argv[i], 7, ERx("-iidbms"), 7, TRUE ) == 0)
	{
	    char	*p1;

	    Individual = TRUE;
	    shut_dbms = TRUE;
	    p1 = STindex( argv[i], ERx( "=" ), 0 );
	    if (p1 != NULL)
	    {
		CMnext(p1);
		STcopy(p1, dbms_id);
	    }
	    else
		STcopy("", dbms_id);
	}
	else if (STbcompare( argv[i], 6, ERx("-iigcc"), 6, TRUE ) == 0)
	{
	    char	*p1;

	    Individual = TRUE;
	    shut_gcc = TRUE;
	    p1 = STindex( argv[i], ERx( "=" ), 0 );
	    if (p1 != NULL)
	    {
		CMnext(p1);
		STcopy(p1, gcc_id);
	    }
	    else
		STcopy("", gcc_id);
	}
	else if (STbcompare( argv[i], 6, ERx("-iigcd"), 6, TRUE ) == 0)
	{
	    char	*p1;

	    Individual = TRUE;
	    shut_gcd = TRUE;
	    p1 = STindex( argv[i], ERx( "=" ), 0 );
	    if (p1 != NULL)
	    {
		CMnext(p1);
		STcopy(p1, gcd_id);
	    }
	    else
		STcopy("", gcd_id);
	}
	else if (STbcompare( argv[i], 6, ERx("-iigcb"), 6, TRUE ) == 0)
	{
	    char	*p1;

	    Individual = TRUE;
	    shut_gcb = TRUE;
	    p1 = STindex( argv[i], ERx( "=" ), 0 );
	    if (p1 != NULL)
	    {
		CMnext(p1);
		STcopy(p1, gcb_id);
	    }
	    else
		STcopy("", gcb_id);
	}
	else if (STcasecmp( argv[i], ERx("-dmfrcp") ) == 0)
	{
	    Individual = TRUE;
	    shut_dmfrcp = TRUE;
	}
	else if (STcasecmp( argv[i], ERx("-iigws") ) == 0)
	{
	    Individual = TRUE;
	    shut_gws = TRUE;
	}
	else if (STcasecmp( argv[i], ERx("-iigcn") ) == 0)
	{
	    Individual = TRUE;
	    shut_gcn = TRUE;
	}
        else
            USAGE();
    }


	STprintf (buffer, ERx("%s.%s.prefs.%s"), SystemCfgPrefix, host,
		ERx("ingstop"));

	if ((PMget(buffer, &start_val) == OK) && start_val && *start_val)
	{
		if ( (STcasecmp( start_val, ERx("-force") ) == 0 ||
			STcasecmp( start_val, ERx("-f") )   == 0) )
		{
			if (force != TRUE && try_flag != TRUE)
			{
				force = TRUE;
				immediate = FALSE;
			}
		}
		else if (STcasecmp( start_val, ERx("-kill") ) == 0 ||
			STcasecmp( start_val, ERx("-k") )     == 0)
		{
			if (force != TRUE && kill != TRUE &&
				immediate != TRUE && try_flag != TRUE)
			{
				kill = TRUE;
				force = TRUE;
				immediate = TRUE;
			}
		}

	}

    /*
    ** If II_SYSTEM isn't set, then we should exit.
    */
    NMgtAt( SYSTEM_LOCATION_VARIABLE, &ii_system );
    if( ii_system == NULL || *ii_system == EOS )
    {
        F_PRINT( ERget( S_ST0580_iisystem_not_set ), SystemVarPrefix );
        PCexit (FAIL);
    }

    /*
    ** If II_INSTALLATION isn't set, then we should exit.
    */
    NMgtAt( ERx("II_INSTALLATION"), &ii_install );
    if( ii_install == NULL || *ii_install == EOS )
    {
        F_PRINT( ERget( S_ST0581_iiinstall_not_set ), SystemVarPrefix );
        PCexit (FAIL);
    }
    else
	STcopy(ii_install, tchII_INSTALLATION);

    NMloc(TEMP, PATH, NULL, &ii_temploc);
    LOtos(&ii_temploc, &ii_temp);

    /*
    ** Allocate space globally for the command buffer (maximum length ever used)
    */
    cmdbuflen = GL_MAXNAME + 2*STlength(ii_temp) + STlength(ingstop) + 64;
    if ((cmdbuf = MEreqmem(0, cmdbuflen, TRUE, NULL)) == NULL)
    {
	SIfprintf(stderr, ERget( S_ST0634_reqmem_fail ));
	PCexit(FAIL);
    }

    STprintf(ServiceName, ERx("%s_Database_%s"), SYSTEM_SERVICE_NAME, tchII_INSTALLATION);

    if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))!= NULL)
    {
	OpIngSvcHandle = OpenService(schSCManager, ServiceName, SERVICE_QUERY_STATUS);
	if (OpIngSvcHandle == NULL)
	{
		STprintf(ServiceName, ERx("%s_DBATools_%s"), SYSTEM_SERVICE_NAME, tchII_INSTALLATION);
		OpIngSvcHandle = OpenService(schSCManager, ServiceName, SERVICE_QUERY_STATUS);
		if (OpIngSvcHandle != NULL)
			Is_DBATools=TRUE;
	}
	if (OpIngSvcHandle != NULL)
	{
		if (QueryServiceStatus(OpIngSvcHandle, lpssServiceStatus))
		{
			if (ssServiceStatus.dwCurrentState == SERVICE_RUNNING)
				bServiceStarted = TRUE;
		}
		CloseServiceHandle(OpIngSvcHandle);
	}
	CloseServiceHandle(schSCManager);
    }

    PMsetDefault(0, SystemCfgPrefix);
    PMsetDefault(1, host);
    PMsetDefault(2, ERx("dbms"));
    PMsetDefault(3, ERx("*"));

    user = user_buf ;
    IDname(&user);
    if (ingstop_chk_priv(user, GCA_PRIV_SERVER_CONTROL) != OK)
    {
	PRINT( ERget ( S_ST06B1_stop_aborted ) );
	PRINT( ERget ( S_ST06B2_no_privilege ) );
	PCexit(FAIL);
    }
    /*
    ** Check if this is Vista, and if user has sufficient privileges
    ** to execute.
    */
    ELEVATION_REQUIRED();


    if (!service && !checkonly)  /* Need to reduce time to shutdown a service to < 30s */
    {
        if (force || kill)
        {
            if (minutes == 0)
	    {
                PRINT( ERget( S_ST0583_terminate_now ) );
	    }
            else
	    {
                F_PRINT2( ERget( S_ST0584_time_terminate ),
                    minutes, minutes == 1 ? "" : "s" );
	    }
        }

	if (Individual)
	{
	    F_PRINT2( ERget( S_ST063B_indv_proc_check ), SystemProductName,
		      ii_install );
	}
	else
	{
	    F_PRINT2( ERget( S_ST0585_proc_check ), SystemProductName,
		      ii_install );
	}
    }

    /* Query the server class names from config.dat */
    get_server_classnames();

    if ( get_procs() )
    {
	PRINT(ERget( S_ST0586_error_header ));
	F_PRINT( ERget( S_ST0587_bad_iinamu ), iinamu );
	PCexit(1);
    }

    find_procs();

    if (service || checkonly)
	goto skip_printfs;  /* Need ingstop < 30secs for a service!!! */

    /*
    ** Only print out this warning if we're running any DBMS servers.
    */

    if ((force || kill) && (iidbms_count > 0 || iistar_count > 0))
    {
	PRINT(ERget( S_ST0588_warn_header ));
	if (immediate)
	{
	    F_PRINT( ERget( S_ST0589_uncomm_trans ), SystemProductName );
	}
	else
	{
	    PRINT(ERget( S_ST058A_uncomm_trans2 ));
	}

	PRINT(ERget( S_ST058B_incons_dbs ));

    }

    if ( (!Individual) || (Individual && shut_gcn) )
    {
	/*
	** Report the number of Name Server's running in this installation.
	*/
	if (iigcn_count == 0)
	{
	    F_PRINT( ERget( S_ST058C_no_iigcn ), iigcn );
	}
	else if (iigcn_count == 1)
	{
	    F_PRINT2( ERget( S_ST058D_iigcn_running ), iigcn, iigcn_id );
	}
	else
	{
	    F_PRINT( ERget( S_ST058E_too_many_gcn ), iigcn );
	}
    }

    if ( (!Individual) || (Individual && shut_gcc) )
    {
	/*
	** Report the number of Net Server's running in this installation.
	*/
	if (iigcc_count > 0)
	{
	    if (iigcc_count == 1)
	    {
		F_PRINT( ERget( S_ST058F_one_iigcc ), iigcc );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0590_many_iigcc ), iigcc_count, iigcc );
	    }
	    for (i = 0, p = iigcc_id; i < iigcc_count; p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else
	{
	    F_PRINT( ERget( S_ST0591_no_iigcc ), iigcc );
	}
    }

    if ( (!Individual) || (Individual && shut_gcd) )
    {
	/*
	** Report the number of DAS Servers running in this installation.
	*/
	if (iigcd_count > 0)
	{
	    if (iigcd_count == 1)
	    {
		F_PRINT( ERget( S_ST0695_one_iigcd ), iigcd );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0696_many_iigcd ), iigcd_count, iigcd );
	    }
	    for (i = 0, p = iigcd_id; i < iigcd_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else
	{
	    F_PRINT( ERget( S_ST0697_no_iigcd ), iigcd );
	}
    }

    if ( (!Individual) || (Individual && shut_gcb) )
    {
	/*
	** Report the number of Bridge Server's running in this installation.
	*/
	if (iigcb_count > 0)
	{
	    if (iigcb_count == 1)
	    {
		F_PRINT( ERget( S_ST0592_one_iigcb ), iigcb );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0593_many_iigcb ), iigcb_count, iigcb );
	    }
	    for (i = 0, p = iigcb_id; i < iigcb_count; p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else
	{
	    F_PRINT( ERget( S_ST0594_no_iigcb ), iigcb );
	}
    }

    if ( (!Individual) || (Individual && shut_oracle) )
    {
	/*
	** Report the number of Oracle Gateways running in this installation.
	*/
	if (oracle_count > 0)
	{
	    if (oracle_count == 1)
	    {
		F_PRINT( ERget( S_ST0595_one_ora ), oracle );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0596_many_ora ), oracle_count, oracle );
	    }
	    for (i = 0, p = oracle_id; i < oracle_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (gateway_configured (oracle_key))
	{
	    F_PRINT( ERget( S_ST0597_no_ora ), oracle );
	}
    }

    if ( (!Individual) || (Individual && shut_informix) )
    {
	/*
	** Report the number of Informix Gateways running in this installation.
	*/
	if (informix_count > 0)
	{
	    if (informix_count == 1)
	    {
		F_PRINT( ERget( S_ST0598_one_informix ), informix );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0599_many_informix ), informix_count,
			  informix );
	    }
	    for (i = 0, p = informix_id; i < informix_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (gateway_configured (informix_key))
	{
	    F_PRINT( ERget( S_ST059A_no_informix ), informix );
	}
    }

    if ( (!Individual) || (Individual && shut_mssql) )
    {
	/*
	** Report the number of MSSQL Gateway running in this installation.
	*/
	if (mssql_count > 0)
	{
	    if (mssql_count == 1)
	    {
		F_PRINT( ERget( S_ST059B_one_mssql ), mssql );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST059C_many_mssql ), mssql_count, mssql );
	    }
	    for (i = 0, p = mssql_id; i < mssql_count; p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (gateway_configured (mssql_key))
	{
	    F_PRINT( ERget( S_ST059D_no_mssql ), mssql );
	}
    }

    if ( (!Individual) || (Individual && shut_sybase) )
    {
	/*
	** Report the number of Sybase Gateways running in this installation.
	*/
	if (sybase_count > 0)
	{
	    if (sybase_count == 1)
	    {
		F_PRINT( ERget( S_ST059E_one_syb ), sybase );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST059F_many_syb ), sybase_count, sybase );
	    }
	    for (i = 0, p = sybase_id; i < sybase_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (gateway_configured (sybase_key))
	{
	    F_PRINT( ERget( S_ST0600_no_syb ), sybase );
	}
    }

    if ( (!Individual) || (Individual && shut_odbc) )
    {
	/*
	** Report the number of ODBC Gateways running in this installation.
	*/
	if (odbc_count > 0)
	{
	    if (odbc_count == 1)
	    {
		F_PRINT( ERget( S_ST0601_one_odbc ), odbc );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0602_many_odbc ), odbc_count, odbc );
	    }
	    for (i = 0, p = odbc_id; i < odbc_count; p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (gateway_configured (odbc_key))
	{
	    F_PRINT( ERget( S_ST0603_no_odbc ), odbc );
	}
    }

    if ( (!Individual) || (Individual && shut_db2udb) )
    {
	/*
	** Report the number of db2udb Gateways running in this installation.
	*/
	if (db2udb_count > 0)
	{
	    if (db2udb_count == 1)
	    {
		F_PRINT( ERget( S_ST0678_one_db2udb ), db2udb );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0679_many_db2udb ), db2udb_count, db2udb );
	    }
	    for (i = 0, p = db2udb_id; i < db2udb_count; p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (gateway_configured (db2udb_key))
	{
	    F_PRINT( ERget( S_ST0680_no_db2udb ), db2udb );
	}
    }

    if ( (!Individual) || (Individual && shut_rmcmd) )
    {
	/*
	** Report rmcmd servers running
	*/
	if (iirmcmd_count > 0)
	{
	    F_PRINT( ERget( S_ST0604_one_rmcmd ), rmcmd );
	}
	else
	{
	    F_PRINT( ERget( S_ST0605_no_rmcmd ), rmcmd );
	}
    }

    if ( (!Individual) || (Individual && shut_dbms) )
    {
	if (iidbms_count > 0)
	{
	    if (iidbms_count == 1)
	    {
		F_PRINT( ERget( S_ST0606_one_dbms ), iidbms );
	    }
	    else
	    {
		F_PRINT2( ERget( S_ST0607_many_dbms ), iidbms_count, iidbms );
	    }
	    for (i = 0, p = iidbms_id; i < iidbms_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (!Is_w95)
	{
	    F_PRINT( ERget( S_ST0608_no_dbms ), iidbms );
	}
    }

    if ( (!Individual) || (Individual && shut_icesvr) )
    {
	if (icesvr_count > 0)
	{
	    if (icesvr_count == 1)
	    {
		PRINT( ERget( S_ST0609_one_ice ) );
	    }
	    else
	    {
		F_PRINT( ERget( S_ST060A_many_ice ), icesvr_count );
	    }
	    for (i = 0, p = icesvr_id; i < icesvr_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (!Is_w95 && !Is_Jasmine)
	{
	    PRINT( ERget( S_ST060B_no_ice ) );
	}
    }

    if ( (!Individual) || (Individual && shut_star) )
    {
	if (iistar_count > 0)
	{
	    if (iistar_count == 1)
	    {
		PRINT( ERget( S_ST060C_one_star ) );
	    }
	    else
	    {
		F_PRINT( ERget( S_ST060D_many_star ), iistar_count );
	    }
	    for (i = 0, p = iistar_id; i < iistar_count;
		 p += (STlength(p)+1), i++)
		F_PRINT( "\t%s\n", p);
	    PRINT( "\n" );
	}
	else if (!Is_w95 && !Is_Jasmine)
	{
	    PRINT( ERget( S_ST060E_no_star ) );
	}
    }

    if ( (!Individual) || (Individual && shut_dmfrcp) )
    {
	if (iirecovery_count > 0)
	{
	    F_PRINT( ERget( S_ST060F_one_recovery ), iirecovery_id );
	}
	else if (!Is_w95 && !Is_Jasmine)
	{
	    PRINT( ERget( S_ST0610_no_recovery ) );
	}
    }

    if ( (!Individual) || (Individual && shut_gcn) ||
	 (Individual && shut_gcc) || (Individual && shut_gcb) ||
	 (Individual && shut_gcd) ||
	 (Individual && shut_dbms) ||
	 (Individual && shut_star) || (Individual && shut_dmfrcp) )
    {
	if ((iigcn_count < 1) && (iigcc_count <1 ) &&
	(iigcd_count < 1) &&
	(iigcb_count < 1) && (iidbms_count < 1) &&
	(iistar_count < 1) && (iirecovery_count < 1))
	{
	    F_PRINT2( ERget( S_ST0611_nothing_running ), SystemProductName,
		      ii_install);

	    if (shut_dmfrcp)
	    {
		PRINT( ERget( S_ST0612_remove_shmem ) );
		execute( iiclean );
	    }

	    if (bServiceStarted && !Individual)
	    {
		/*
		** Shut down the service too.
		*/
		PCexit(shutdown_service());
	    }

	    PCexit (OK);
	}
    }


skip_printfs:

    /*
    **  Shut down processes
    */

    if (Individual)
    {
	/*
	** Shut down the selected pieces of Ingres.
	*/
	if (shut_rmcmd)
	    stop_rmcmd();
	if (shut_oracle)
	    stop_oracle();
	if (shut_informix)
	    stop_informix();
	if (shut_sybase)
	    stop_sybase();
	if (shut_mssql)
	    stop_mssql();
	if (shut_odbc)
	    stop_odbc();
	if (shut_db2udb)
	    stop_db2udb();
	if (shut_icesvr)
	    stop_icesvr(TRUE);
	if (shut_star)
	    stop_star(TRUE);
	if (shut_dbms)
	    stop_dbms(TRUE);
	if (shut_gcc)
	    stop_gcc(TRUE);
	if (shut_gcd)
	    stop_gcd(TRUE);
	if (shut_gcb)
	    stop_gcb(TRUE);
        if (minutes > 0)
        {
            F_PRINT2( ERget( S_ST0613_wait_sessions ), minutes,
		      (minutes == 1) ? "" : "s" );
            PCsleep( minutes * 60 * 1000); /* sleep is milli seconds */
        }
	if (iidbms_count > 0 || iigcc_count > 0 || iistar_count > 0 ||
	    iigcd_count > 0 ||
	    icesvr_count > 0 || iigcb_count > 0)
	{
	    get_procs();
	    find_procs();
	    if (shut_dbms)
		open_dbms();
	    if (shut_star)
		open_star();
	    if (shut_gcc)
		open_gcc();
	    if (shut_gcb)
		open_gcb();
            if (force)
            {
		if (shut_rmcmd)
		    stop_rmcmd();
		if (shut_oracle)
		    stop_oracle();
		if (shut_informix)
		    stop_informix();
		if (shut_sybase)
		    stop_sybase();
		if (shut_mssql)
		    stop_mssql();
		if (shut_odbc)
		    stop_odbc();
		if (shut_db2udb)
		    stop_db2udb();
		if (shut_icesvr)
		    stop_icesvr(FALSE);
		if (shut_star)
		    stop_star(FALSE);
		if (shut_dbms)
		    stop_dbms(FALSE);
		if (shut_gcc)
		    stop_gcc(FALSE);
		if (shut_gcd)
		    stop_gcd(FALSE);
		if (shut_gcb)
		    stop_gcb(FALSE);
		if (shut_dmfrcp)
		    stop_rcp(immediate);
	    }
	}
	else
	{
	    if (shut_dmfrcp)
		stop_rcp(FALSE);
	}

	if (shut_gws)
	    stop_gws();
	if (shut_gcn)
	    stop_gcn();

	if (dbms_up || star_up)
	    PRINT( ERget( S_ST0614_abort_shutdown ) );
	if (iigcc_count)
	    PRINT( ERget( S_ST0615_reopen_net ) );
	if (iigcd_count)
	    PRINT( ERget( S_ST0698_reopen_das ) );
	if (iigcb_count)
	    PRINT( ERget( S_ST0616_reopen_bridge ) );
    }
    else if (showonly == FALSE)
    {
	if( check_cache_sharing() == TRUE || iidbms_count == 1 )
            stop_rmcmd();
        stop_oracle();
        stop_informix();
        stop_sybase();
        stop_mssql();
        stop_odbc();
        stop_db2udb();

	if (servicekill)
	{
            stop_icesvr( FALSE );
            stop_star( FALSE );
            stop_dbms( FALSE );
	    if( check_cache_sharing() != TRUE && iidbms_count > 0 )
	    {
		stop_rmcmd();
                stop_dbms( FALSE );
	    }
	}
	else
	{
       	    stop_icesvr( TRUE );
            stop_star( TRUE );
            stop_dbms( TRUE );
	    if( check_cache_sharing() != TRUE && iidbms_count > 0 )
	    {
		stop_rmcmd();
                stop_dbms( TRUE );
	    }
	}
        stop_gcc( TRUE );
        stop_gcd( TRUE );
        stop_gcb( TRUE );
        if (minutes > 0)
        {
            F_PRINT2( ERget( S_ST0613_wait_sessions ), minutes,
		      (minutes == 1) ? "" : "s" );
            PCsleep( minutes * 60 * 1000); /* sleep is milli seconds */
        }
        if (iidbms_count > 0 || iigcc_count > 0 || iistar_count > 0 ||
            iigcd_count > 0 ||
            icesvr_count > 0 || iigcb_count > 0)
        {
            get_procs();
            find_procs();
            open_dbms();
            open_star();
            open_gcc();
            open_gcd();
            open_gcb();
            if (force)
            {
                stop_rmcmd ();
                stop_oracle ();
                stop_informix ();
                stop_sybase ();
                stop_mssql ();
                stop_odbc ();
                stop_db2udb ();
                stop_gcc( FALSE );
                stop_gcd( FALSE );
                stop_gcb( FALSE );
                stop_icesvr( FALSE );
                stop_star( FALSE );
                stop_dbms( FALSE );
                stop_rcp( immediate );
	        stop_gws( );
                stop_gcn();
            }
        }
        else
        {
	    if (servicekill)
                stop_rcp( TRUE );
	    else
                stop_rcp( FALSE );
	      stop_oracle ();
            stop_informix ();
            stop_sybase ();
            stop_mssql ();
            stop_odbc ();
            stop_db2udb ();
            stop_rmcmd ();
	    stop_gws( );
            stop_gcn();
        }

        /*
        **  Clean up shared memory files if none of the servers are up.
        */

	if ( !(dbms_up || star_up || rcp_up || iigcc_count || iigcb_count ||
	     iigcd_count) && !Is_w95 )
	{
            SIflush (stdout);
            execute( iiclean );
	}

	if (dbms_up || star_up)
	    PRINT( ERget( S_ST0614_abort_shutdown ) );

	if (iigcc_count)
	    PRINT( ERget( S_ST0615_reopen_net ) );

	if (iigcd_count)
	    PRINT( ERget( S_ST0698_reopen_das ) );

	if (iigcb_count)
	    PRINT( ERget( S_ST0616_reopen_bridge ) );
    }
/*
**  Cleanup temp files
*/
    STprintf( cmdbuf, ERx("del \"%s\\%s.$$*\""), ii_temp, ingstop );
    system( cmdbuf );
    if (!Is_w95)
    {
        STprintf( cmdbuf, ERx("del \"%s\\*stdin.tmp\" > nul 2>&1"), ii_temp );
        system( cmdbuf );
        STprintf( cmdbuf, ERx("del \"%s\\*stdout.tmp\" > nul 2>&1"), ii_temp );
        system( cmdbuf );
    }

    if (checkonly)
    {
    	/*
	** When "-check" is specified, check to make sure that everything that
	** is supposed to be running IS running. (Added for cluster support)
	*/
    	SERVER_LIST servers;	    // Lists of servers that should be started
    	build_server_list( &servers );
	if ( check_server_list( &servers ) )
	    PCexit (FAIL);
    }
    else if (dbms_up || star_up || rcp_up || iigcc_count || iigcb_count ||
	iigcd_count)
	PCexit (FAIL);
    else if (bServiceStarted && !service && servicekill)
    {
	STATUS status;
	if ((status = shutdown_service()) == FAIL)
	    PCexit(status);

	if (!Individual)
	{
	    STprintf(tmp_buf, ERx("\n%s %s\n"), SystemProductName, ERget(S_ST06AF_shutdown_sucessfully));
	    PRINT(tmp_buf);
	}
	PCexit (OK);
    }
    else if (bServiceStarted && !Individual && !showonly)
    {
	/*
	** Shut down the service too, unless "-show" was specified.
	*/
	STATUS status;
	if ((status = shutdown_service()) == FAIL)
	    PCexit(status);
    }

    if (!Individual && !checkonly)
    {
	STprintf(tmp_buf, ERx("\n%s %s\n"), SystemProductName, ERget(S_ST06AF_shutdown_sucessfully));
	PRINT(tmp_buf);
    }
    PCexit (OK);
}


/******************************************************************************
**
** Name: find_procs - Find the requested server processes that are running
**
** Description:
**   This routine identifies the listen ids of all running server processes
**   for the requested server types.  This info is obtained from the output
**   of the iinamu command.
** 
** Inputs:
**     no input parms
**     globals:
**        readbuf           Pointer to iinamu output
**        iixxx_key         String constant of server xxx name
** Outputs:
**     no input parms
**     globals:
**        iixxx_id          Listen id of server xxx
**
** Returns:
**     none
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Changed to search for the real server name in the iinamu
**	    output (column 1) and then return the server's listen id
**	    in column 3.  Previously, the pipe name prefix (which included
**	    the server name embedded in it) was being used as the search
**	    key; that approach doesn't work for tcp_ip which uses numeric
**	    ports as the listen_id.
**
******************************************************************************/
static
void
find_procs( )
{
    char    *kp;
    char    *p = readbuf;
    int     i, id_len;
    char    prevId[64] = {0};
    i4      len;
    bool    duplicate;

    iigcn_count = 0;
    iigcc_count = 0;
    iigcd_count = 0;
    iigcb_count = 0;
    iigws_count = 0;
    iidbms_count = 0;
    iirecovery_count = 0;
    iistar_count = 0;
    oracle_count = 0;
    sybase_count = 0;
    informix_count = 0;
    mssql_count = 0;
    odbc_count = 0;
    db2udb_count = 0;
    
    kp = iigcn_id;
    if (p = find_key( p, iigcn_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        iigcn_count++;
    }
    else if (!Individual)
        return;

    kp = iigcc_id;
    p = readbuf;
    while (p = find_key( p, iigcc_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        iigcc_count++;
    }

    kp = iigcd_id;
    p = readbuf;
    while (p = find_key( p, iigcd_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        iigcd_count++;
    }

    kp = iigcb_id;
    p = readbuf;
    while (p = find_key( p, iigcb_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        iigcb_count++;
    }

    if (Is_w95)
    {
        kp = iigws_id;
    	p = readbuf;
    	while (p = find_key( p, iigws_key, &id_len ))
    	{
            STlcopy( p, kp, id_len );
            p  += id_len;
            kp += id_len + 1;
            iigws_count++;
    	}
    }

    kp = iirmcmd_id;
    p = readbuf;
    while (p = find_key(p, iirmcmd_key, &id_len))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        iirmcmd_count++;
    }

    kp = iidbms_id;
    for (i = 0; i <= num_dbms_classes; i++)
    {
	p = readbuf;
	len = 0;
	duplicate = FALSE;

	while (p = find_key( p, iidbms_key[i], &id_len ))
	{
	    if (iidbms_count)
	    {
	       duplicate = !STscompare(prevId, len, p, id_len);
	    }

	    if (!duplicate)
	    {
                STlcopy( p, kp, id_len );
                kp += id_len + 1;
	        iidbms_count++;
	    }
            STlcopy( p, prevId, id_len );
            len = id_len;
            p  += id_len;
	}
    }

    kp = iirecovery_id;
    p = readbuf;
    while (p = find_key( p, iirecovery_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        iirecovery_count++;
    }

    kp = iistar_id;
    for (i = 0; i <= num_star_classes; i++)
    {
	p = readbuf;
	while (p = find_key( p, iistar_key[i], &id_len ))
	{
            STlcopy( p, kp, id_len );
            p  += id_len;
            kp += id_len + 1;
	    iistar_count++;
	}
    }

    kp = icesvr_id;
    p = readbuf;
    while (p = find_key( p, icesvr_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        icesvr_count++;
    }

    kp = oracle_id;
    p = readbuf;
    while (p = find_key( p, oracle_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        oracle_count++;
    }

    kp = informix_id;
    p = readbuf;
    while (p = find_key( p, informix_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        informix_count++;
    }

    kp = sybase_id;
    p = readbuf;
    while (p = find_key( p, sybase_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        sybase_count++;
    }

    kp = mssql_id;
    p = readbuf;
    while (p = find_key( p, mssql_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        mssql_count++;
    }

    kp = odbc_id;
    p = readbuf;
    while (p = find_key( p, odbc_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        odbc_count++;
    }

    kp = db2udb_id;
    p = readbuf;
    while (p = find_key( p, db2udb_key, &id_len ))
    {
        STlcopy( p, kp, id_len );
        p  += id_len;
        kp += id_len + 1;
        db2udb_count++;
    }

}

/******************************************************************************
**
** Name: find_key - Find the listen id for the input key (server name)
**
** Description:
**   This routine scans the input buffer of iinamu output for the listen
**   id of the first entry of a matching server name (specifed as the input
**   key). 
** 
**  iinamu lines are in the format of:
**     servername  classes     listen_id
**  eg:  COMSVR       *       II\COMSVR\cd4   (named pipes)
**       COMSVR       *       1234            (tcp_ip)
**
** Inputs:
**   buf                    Pointer to some place in the iinamu output
**   key                    Server name key (eg, IINMSVR, COMSVR, etc)
**   id_len                 Pointer to len field where the length of the
**                            listen id should be stored.
** Outputs:
**   id_len                 Length of the listen_id of the server
**                            =0 if server not found
**
** Returns:
**     Pointer to the listen id of the server (NULL if server not found).
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Changed to search for the real server name in the iinamu
**	    output (column 1) and then return the server's listen id
**	    in column 3.  Previously, the pipe name prefix (which included
**	    the server name embedded in it) was being used as the search
**	    key; that approach doesn't work for tcp_ip which uses numeric
**	    ports as the listen_id.
**
******************************************************************************/
static
char *
find_key( char *buf, char *key , int *id_len)
{
    int     len = STlength( key );
    char    *id = NULL;
    *id_len = 0;

    while (*buf)
    {
        if ( (!memcmp( buf, key, len ))
        &&   ( *(buf+len) == ' ' ) )
        {
            buf = buf + len + 1;  /* pt to 1st byte after "key+space" */
            break;  /* Match on server name */
        }
        buf++;
    }
    if (!(*buf)) return NULL;

    /*
    ** A match has been found on the server key.  Now find and
    ** return a pointer to the listen_id (3rd field).
    */

    while (isspace( *buf )) {buf++;};  /* skip 1st set of spaces */
    if (!(*buf)) return NULL;
    while (!isspace( *buf ) && (*buf)) {buf++;};  /* skip classes field */
    if (!(*buf)) return NULL;
    while (isspace( *buf )) {buf++;};  /* skip 2nd set of spaces */
    if (!(*buf)) return NULL;
    id = buf;
    while (!CMwhite( buf ) && (*buf)) {buf++;};  /* find end of listen id */
    if (!(*buf)) return NULL;
    *id_len = buf - id;
    return (id);
}

static
int
get_procs( )
{
    FILE        *desc;
    int         bytesread, i;
    LOCATION    loc;
    int         ret;

    create_command_file();

    if ( (!Individual) || (Individual && shut_gcn) )
    {
	append_show_to_command_file(iigcn_key);
    }

    if ( (!Individual) || (Individual && shut_gcc) )
    {
	append_show_to_command_file(iigcc_key);
    }

    if ( (!Individual) || (Individual && shut_gcd) )
    {
	append_show_to_command_file(iigcd_key);
    }

    if ( (!Individual) || (Individual && shut_gcb) )
    {
	append_show_to_command_file(iigcb_key);
    }

    if ( (!Individual) || (Individual && shut_oracle) )
    {
	append_show_to_command_file(oracle_key);
    }

    if ( (!Individual) || (Individual && shut_informix) )
    {
	append_show_to_command_file(informix_key);
    }

    if ( (!Individual) || (Individual && shut_mssql) )
    {
	append_show_to_command_file(mssql_key);
    }

    if ( (!Individual) || (Individual && shut_sybase) )
    {
	append_show_to_command_file(sybase_key);
    }

    if ( (!Individual) || (Individual && shut_odbc) )
    {
	append_show_to_command_file(odbc_key);
    }

    if ( (!Individual) || (Individual && shut_db2udb) )
    {
	append_show_to_command_file(db2udb_key);
    }

    if ( (!Individual) || (Individual && shut_icesvr) )
    {
	append_show_to_command_file(icesvr_key);
    }

    if ( (!Individual) || (Individual && shut_rmcmd) )
    {
	append_show_to_command_file(iirmcmd_key);
    }

    /* Jasmine has no Star or recovery servers */
    if ( !Is_Jasmine )
    {
	if ( (!Individual) || (Individual && shut_star) )
	{
	    for (i = 0; i <= num_star_classes; i++)
	    {
		append_show_to_command_file(iistar_key[i]);
	    }
	}

	if ( (!Individual) || (Individual && shut_dmfrcp) )
	{
	    append_show_to_command_file(iirecovery_key);
	}
    }

    if ( (!Individual) || (Individual && shut_dbms) )
    {
	for (i = 0; i <= num_dbms_classes; i++)
	{
	    append_show_to_command_file(iidbms_key[i]);
	}
    }

    if ( (Is_w95) && ((!Individual) || (Individual && shut_gws)) )
    {
	append_show_to_command_file(iigws_key);
    }

    append_to_command_file(ERx("quit"));

    STprintf( cmdbuf, ERx("%s <\"%s\" >\"%s\\%s.$$2\""),
             iinamu, cmndfile_buf, ii_temp, ingstop );
    ret = system( cmdbuf );

    delete_command_file();

    if ( ret != 0 && ret != 1 )
	return ( ret );

    STprintf( cmdbuf, ERx("%s\\%s.$$2"), ii_temp, ingstop );
    loc.string = cmdbuf;
    SIopen( &loc, "r", &desc );
    SIread( desc, sizeof(readbuf), &bytesread, readbuf);
    SIclose( desc );
    readbuf[bytesread] = '\0';

    return ( OK );
}

/*
** Name: check_running_servers
**
** Description:
**      Compare the user-entered connect_id value against the
**      actual list of running servers of the given type.
**
** Inputs:
**      check_id        = value given as the -iixxx= value
**      running_ids     = list of running servers from "find_procs"
**      count           = number of entries in "running_ids" list
**
** Outputs:
**      If the "check_id" value is found, it is copied to the
**      "running_ids" buffer.
**
** Returns:
**      1 if the value is found, and "running_ids" is updated
**      0 if the value is not found, an error message is printed
**
** Side effects:
**      "running_ids" can be updated, or an error message may be printed
**
** History:
**      18-Feb-2009 (whiro01)
**          Created.
*/
static
int
check_running_servers( const char *check_id, char * running_ids, int count)
{
	int i;
	const char *p;

	for (i = 0, p = running_ids; i < count; p += STlength(p)+1, i++)
	{
	    if (STcasecmp(check_id, p) == 0)
	    {
		STcopy(check_id, running_ids);
		return 1;
	    }
	}
	F_PRINT( ERget( S_ST06C2_unknown_connect_id ), check_id );
	return 0;
}

static
void
stop_gcc( BOOL shut )
{
    char    *p;
    int     i, gcc_count;
    char    *cmd = shut ? ERx("-q") : ERx("-s");
    char    *cmdmsg = shut ? "Shutting down" : "Killing";
    DWORD   pid;

    STATUS  dead;

    if (dbms_up && shut) return;

    if ( Individual && (STcompare(gcc_id, "") != 0) )
    {
	/*
	** Then we're only shutting down the GCC server id given.
	** Make sure it is in the "iigcc_id" list first.
	*/
	iigcc_count = check_running_servers(gcc_id, iigcc_id, iigcc_count);
    }

    if (iigcc_count > 0)
    {
        gcc_count = iigcc_count;
        for (i = 0, p = iigcc_id; i < gcc_count; p += (STlength(p)+1), i++)
        {
	    iigcc_count--;
            F_PRINT2( ERget( S_ST0617_net_procs ), cmdmsg, p );

	    STprintf( cmdbuf, ERx("%s %s %s"), iigcstop, cmd, p );
	    system( cmdbuf );

	    /*
	    ** Give the NET server some time to shut down.
	    */
	    pid = get_process_pid(p);
	    if (kill)
		dead = wait_for_process_death(NULL, pid, 0, NULL);
	    else
		dead = wait_for_process_death(NULL, pid, 5, ".");
	    PRINT("\n\n");

	    if (dead == FAIL)
		iigcc_count++;
        }

	if (iigcc_count)
	    PRINT("\n");
    }
}


static
void
stop_gcd( BOOL shut )
{
    char    *p;
    int     i, gcd_count;
    char    *cmd = shut ? ERx("-q") : ERx("-s");
    char    *cmdmsg = shut ? "Shutting down" : "Killing";
    DWORD   pid;
    STATUS  status;

    if (dbms_up && shut) return;

    if ( Individual && (STcompare(gcd_id, "") != 0) )
    {
	/*
	** Then we're only shutting down the DAS server id given.
	** Make sure it is in the "iigcd_id" list first.
	*/
	iigcd_count = check_running_servers(gcd_id, iigcd_id, iigcd_count);
    }

    if (iigcd_count > 0)
    {
        gcd_count = iigcd_count;
        for (i = 0, p = iigcd_id; i < gcd_count; p += (STlength(p)+1), i++)
        {
	    iigcd_count--;
            F_PRINT2( ERget( S_ST0699_das_procs ), cmdmsg, p );

	    STprintf( cmdbuf, ERx("%s %s %s"), iigcstop, cmd, p );
	    system( cmdbuf );

	    /*
	    ** Give the DAS server some time to shut down.
	    */
	    pid = get_process_pid(p);
	    status = wait_for_process_death(iigcd, pid, 5, ".");
	    PRINT("\n\n");

	    if (status == FAIL)
		iigcd_count++;
        }

	if (iigcd_count)
	    PRINT("\n");
    }
}

static
void
stop_gcb( BOOL shut )
{
    char    *p;
    int     i, gcb_count;
    char    *cmd = shut ? ERx("-q") : ERx("-s");
    char    *cmdmsg = shut ? "Shutting down" : "Killing";
    DWORD   pid;
    STATUS  dead;

    if (dbms_up && shut) return;

    if ( Individual && (STcompare(gcb_id, "") != 0) )
    {
	/*
	** Then we're only shutting down the GCB server id given.
	** Make sure it is in the "iigcb_id" list first.
	*/
	iigcb_count = check_running_servers(gcb_id, iigcb_id, iigcb_count);
    }

    if (iigcb_count > 0)
    {
        gcb_count = iigcb_count;
        for (i = 0, p = iigcb_id; i < gcb_count; p += (STlength(p)+1), i++)
        {
	    iigcb_count--;
            F_PRINT2( ERget( S_ST0618_bridge_procs ), cmdmsg, p );

	    STprintf( cmdbuf, ERx("%s %s %s"), iigcstop, cmd, p );
	    system( cmdbuf );

	    /*
	    ** Give the Bridge server some time to shut down.
	    */
	    pid = get_process_pid(p);
	    if (kill)
		dead = wait_for_process_death(NULL, pid, 0, NULL);
	    else
		dead = wait_for_process_death(NULL, pid, 5, ".");
	    PRINT("\n\n");

	    if (dead == FAIL)
		iigcb_count++;
        }

	if (iigcb_count)
	    PRINT("\n");
    }
}


static
void
stop_gws( )
{
    char    *p;
    int     i, gws_count;

    if (iigws_count > 0)
    {
        gws_count = iigws_count;
        for (i = 0, p = iigws_id; i < gws_count; p += (STlength(p)+1), i++)
        {
	    iigws_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), iigwstop, p );
            system( cmdbuf );
        }
    }
}


static void
stop_oracle( )
{
    char    *p;
    int     i, count;

    if (oracle_count > 0)
    {
        F_PRINT( ERget( S_ST0619_stop_ora ), oracle );
        count = oracle_count;
        for (i = 0, p = oracle_id; i < count; p += (STlength(p)+1), i++)
        {
	    oracle_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), gwstop, oracle_key );
            system( cmdbuf );
        }
    }
}

static void
stop_informix( )
{
    char    *p;
    int     i, count;

    if (informix_count > 0)
    {
        F_PRINT( ERget( S_ST061A_stop_informix ), informix );
        count = informix_count;
        for (i = 0, p = informix_id; i < count; p += (STlength(p)+1), i++)
        {
	    informix_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), gwstop, informix_key );
            system( cmdbuf );
        }
    }
}

static void
stop_sybase( )
{
    char    *p;
    int     i, count;

    if (sybase_count > 0)
    {
        F_PRINT( ERget( S_ST061B_stop_syb ), sybase );
        count = sybase_count;
        for (i = 0, p = sybase_id; i < count; p += (STlength(p)+1), i++)
        {
	    sybase_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), gwstop, sybase_key );
            system( cmdbuf );
        }
    }
}

static void
stop_mssql( )
{
    char    *p;
    int     i, count;

    if (mssql_count > 0)
    {
        F_PRINT( ERget( S_ST061C_stop_mssql ), mssql );
        count = mssql_count;
        for (i = 0, p = mssql_id; i < count; p += (STlength(p)+1), i++)
        {
	    mssql_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), gwstop, mssql_key );
            system( cmdbuf );
        }
    }
}

static void
stop_odbc( )
{
    char    *p;
    int     i, count;

    if (odbc_count > 0)
    {
        F_PRINT( ERget( S_ST061D_stop_odbc ), odbc );
        count = odbc_count;
        for (i = 0, p = odbc_id; i < count; p += (STlength(p)+1), i++)
        {
	    odbc_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), gwstop, odbc_key );
            system( cmdbuf );
        }
    }
}

static void
stop_db2udb( )
{
    char    *p;
    int     i, count;

    if (db2udb_count > 0)
    {
        F_PRINT( ERget( S_ST0681_stop_db2udb ), db2udb );
        count = db2udb_count;
        for (i = 0, p = db2udb_id; i < count; p += (STlength(p)+1), i++)
        {
	    db2udb_count--;
            STprintf( cmdbuf, ERx("%s %s >nul"), gwstop, db2udb_key );
            system( cmdbuf );
        }
    }
}

static
void
stop_rmcmd( )
{
    int     i;
    int     rmcmd_count;
    char   *cmd = ERx("rmcmdstp");

    if (iirmcmd_count > 0)
    {
        F_PRINT( ERget( S_ST061E_stop_rmcmd ), rmcmd );
        rmcmd_count = iirmcmd_count;
        for (i = 0; i < rmcmd_count; i++)
        {
	    iirmcmd_count--;
            /*
            ** Add a force flag to rmcmd if the options show
            ** that we're in a service and are forcing the
            ** installation down.
            */
            if ((service == TRUE) && (force == TRUE))
            {
                STprintf( cmdbuf, ERx("%s -force >nul"), cmd );
            }
            else
            {
                STprintf( cmdbuf, ERx("%s >nul"), cmd );
            }
            system( cmdbuf );
        }
    }
    else
    {
        /*
        ** If rmcmd isn't started then we need to stop dbms sessions
        ** in a service if the installation is being forced down.
        */
        if ((service == TRUE) && (force == TRUE))
        {
            STprintf( cmdbuf, ERx("%s -drop_only >nul"), cmd );
            system( cmdbuf );
        }
    }
}


static
void
stop_star( BOOL shut )
{
    char    *p, sname[20];
    int     i, star_count;
    DWORD   pid;
    HANDLE  hProcess;
    STATUS  dead;
    char    *cmd = shut ? ERx("set server shut") : ERx("stop server");

    if (iistar_count > 0)
    {
	if ( Individual && (STcompare(star_id, "") != 0) )
	{
	    /*
	    ** Then we're only shutting down the STAR server id given.
	    ** Make sure it is in the "iistar_id" list first.
	    */
	    iistar_count = check_running_servers(star_id, iistar_id, iistar_count);
	}
	star_count = iistar_count;
        for (i = 0, p = iistar_id; i < star_count; p += (STlength(p)+1), i++)
        {
	    iistar_count--;
            F_PRINT( ERget( S_ST061F_star_procs ), p );
            call_iimonitor(cmd, p);
	    SIflush( stdout );

	    /*
	    ** Put complete server name in sname.
	    */
            STscanf(p, "%s", sname);

	    /*
	    ** Now just get the process id which is the xx part of II/STAR/xx
	    */
	    pid = get_process_pid(p);

	    /* wait for the server to die */
	    dead = wait_for_process_death(NULL, pid, 10, ERx("."));

            PRINT ("\n\n");

	    /*
	    ** Verify that the process has been shutdown.
	    */
            if ( dead == FAIL )
            {
                if (!kill)
                {
                    star_up = TRUE;
		    F_PRINT2( ERget( S_ST0620_fail_star ), iimonitor, sname );
		    F_PRINT( ERget( S_ST0621_fail_try ), ingstop );
		    iistar_count++;
                }
                else
                {
                    hProcess = OpenProcess( PROCESS_TERMINATE,
                                            FALSE,
                                            pid );
                    if (hProcess != NULL)
                    {
                        TerminateProcess( hProcess, 0 );
                        CloseHandle( hProcess );
                    }
                }
	    }
	}
    }
}

static
void
stop_dbms( BOOL shut )
{
    char    *p, sname[20];
    int     i, dbms_count;
    DWORD   pid;
    char    *cmd = shut ? ERx("set server shut") : ERx("stop server");
    STATUS  dead;
    CL_ERR_DESC	err_code;
    i4      display_error=TRUE;


    if (star_up) return;

    dbms_up = FALSE;

    /*
    ** Suppress display of first dbms shutdown error when multiple dbms
    ** are running and cache_sharing is OFF.
    */
    if( check_cache_sharing() != TRUE && iidbms_count > 1 )
        display_error=FALSE;
 
    if (iidbms_count > 0)
    {
	if ( Individual && (STcompare(dbms_id, "") != 0) )
	{
	    /*
	    ** Then we're only shutting down the DBMS server id given.
	    ** Make sure it is in the "iidbms_id" list first.
	    */
	    iidbms_count = check_running_servers(dbms_id, iidbms_id, iidbms_count);
	}
	dbms_count = iidbms_count;

        for (i = 0, p = iidbms_id; i < dbms_count; p += (STlength(p)+1), i++)
        {
	    iidbms_count--;
	    if (force)
	    {
                F_PRINT2( ERget( S_ST0622_dbms_procs ),
                          ((shut != FALSE) ? "Shutting down" : "Stopping"),p );
	    }
	    else
	    {
                F_PRINT( ERget( S_ST0623_close_dbms ), p );
	    }

	    call_iimonitor(cmd, p);
	    SIflush( stdout );

	    /*
	    ** Put complete server name in sname.
	    */
	    STscanf(p, "%s", sname);

	    /*
	    ** Now just get the process id which is the xx part of II/INGRES/xx
	    */
	    pid = get_process_pid(p);

	    /* Give the server time to cleanly shutdown */
	    if (kill)
		dead = wait_for_process_death(NULL, pid, 0, NULL);
	    else
		dead = wait_for_process_death(NULL, pid, 30, ".");

            PRINT ("\n\n");

	    if ( dead == FAIL )
	    {
		/* In other words, we didn't kill the server yet */
		if (!kill)
		{
		    dbms_up = TRUE;
		    if (!force && display_error)
		    {
			PRINT(ERget( S_ST0624_fatal_err ));
			F_PRINT2( ERget( S_ST0625_fail_dbms_conn ), iimonitor,
				sname );
			F_PRINT( ERget( S_ST0626_alt_dbms_conn ), ingstop );
		    }
		    display_error=TRUE;
		    iidbms_count++;
		}
		else
		{
		    if (PCforce_exit( pid, &err_code ) && find_process( NULL, pid ) == OK)
		    {
			dbms_up = TRUE;
			iidbms_count++;
		    }
		}
	    }
	}

    }
}



static
void
stop_icesvr( BOOL shut )
{
    char    *p, sname[20];
    int     i, ice_count;
    DWORD   pid;
    HANDLE  hProcess;
    STATUS  dead;
    char    *cmd = shut ? ERx("set server shut") : ERx("stop server");

    if (icesvr_count > 0)
    {
	ice_count = icesvr_count;

        for (i = 0, p = icesvr_id; i < ice_count; p += (STlength(p)+1), i++)
        {
	    icesvr_count--;
	    if (force)
	    {
                F_PRINT2( ERget( S_ST0627_ice_procs ),
                         ((shut != FALSE) ? "Shutting down" : "Stopping"),p );
	    }
	    else
	    {
                F_PRINT( ERget( S_ST0628_close_ice ), p );
	    }

	    SIflush (stdout);
	    call_iimonitor(cmd, p);
	    SIflush( stdout );

	    /*
	    ** Put complete server name in sname.
	    */
	    STscanf(p, "%s", sname);

	    /*
	    ** Now just get the process id which is the xx part of II/INGRES/xx
	    */
	    pid = get_process_pid(p);

	    /* Give the server time to cleanly shutdown */
	    if (kill)
		dead = wait_for_process_death(NULL, pid, 10, NULL);
	    else
		dead = wait_for_process_death(NULL, pid, 15, ".");

            PRINT ("\n\n");

	    if ( dead == FAIL )
	    {
	        if (!kill)
	        {
                    if (!force)
                    {
		        PRINT(ERget( S_ST0624_fatal_err ));
		        F_PRINT2( ERget( S_ST0629_fail_ice_conn ), iimonitor,
			          sname );
		        F_PRINT( ERget( S_ST0626_alt_dbms_conn ), ingstop );
                    }
		    icesvr_count++;
	        }
	        else
                {
                    hProcess = OpenProcess( PROCESS_TERMINATE,
                                            FALSE,
                                            pid );
		    TerminateProcess( hProcess, 0 );
		    CloseHandle( hProcess );
                }
            }
        }

    }
}

static
void
open_star( )
{
    char    *p;
    int     i;

    if ( Individual && (STcompare(star_id, "") != 0) )
    {
	/*
	** Then we're only reopening the STAR server id given, if it is
	** still up.
	*/
	iistar_count = check_running_servers(star_id, iistar_id, iistar_count);
    }

    for (i = 0, p = iistar_id; i < iistar_count; p += (STlength(p)+1), i++)
    {
	F_PRINT( ERget( S_ST062A_star_active ), p );
	call_iimonitor(ERx("set server open"), p);
    }

}

static
void
open_dbms( )
{
    char    *p;
    int     i;

    if ( Individual && (STcompare(dbms_id, "") != 0) )
    {
	/*
	** Then we're only reopening the DBMS server id given, if it is
	** still up.
	*/
	iidbms_count = check_running_servers(dbms_id, iidbms_id, iidbms_count);
    }

    for (i = 0, p = iidbms_id; i < iidbms_count; p += (STlength(p)+1), i++)
    {
	F_PRINT( ERget( S_ST062B_dbms_active ), p );
	call_iimonitor(ERx("set server open"), p);
    }

}

static
void
open_gcc( )
{
    char    *p;
    int     i;

    if ( Individual && (STcompare(gcc_id, "") != 0) )
    {
	/*
	** Then we're only displaying the GCC server id given, if it is
	** still up.
	*/
	iigcc_count = check_running_servers(gcc_id, iigcc_id, iigcc_count);
    }

    for (i = 0, p = iigcc_id; i < iigcc_count; p += (STlength(p)+1), i++)
    {
	F_PRINT( ERget( S_ST062C_net_active ), p );
    }
}


static
void
open_gcd( )
{
    char    *p;
    int     i;

    if ( Individual && (STcompare(gcd_id, "") != 0) )
    {
	/*
	** Then we're only displaying the DAS server id given, if it is
	** still up.
	*/
	iigcd_count = check_running_servers(gcd_id, iigcd_id, iigcd_count);
    }

    for (i = 0, p = iigcd_id; i < iigcd_count; p += (STlength(p)+1), i++)
    {
	F_PRINT( ERget( S_ST069A_das_active ), p );
    }
}

static
void
open_gcb( )
{
    char    *p;
    int     i;

    if ( Individual && (STcompare(gcb_id, "") != 0) )
    {
	/*
	** Then we're only displaying the GCB server id given, if it is
	** still up.
	*/
	iigcb_count = check_running_servers(gcb_id, iigcb_id, iigcb_count);
    }

    for (i = 0, p = iigcb_id; i < iigcb_count; p += (STlength(p)+1), i++)
    {
	F_PRINT( ERget( S_ST062D_bridge_active ), p );
    }
}


static
void
stop_gcn()
{
    if ((dbms_up || iirecovery_count > 0) && (!Individual)) return;

    if (iigcn_count > 0)
    {
	iigcn_count--;
	F_PRINT( ERget( S_ST062E_stop_gcn ), iigcn );

	create_command_file();
	append_to_command_file(ERx("stop"));
	append_to_command_file(ERx("yes"));
	append_to_command_file(ERx("quit"));
	
        STprintf( cmdbuf, ERx("%s <\"%s\" >nul"),
	         iinamu, cmndfile_buf );
        system( cmdbuf );

        delete_command_file();
    }
}

static
void
stop_rcp( BOOL immediate )
{
    char    *cmdopt = immediate ? ERx("-imm_shutdown") : ERx("-shutdown");
    DWORD   pid;
    HANDLE  hProcess = NULL;
    i4      i;

    rcp_up = TRUE;

    if (iirecovery_count > 0)
    {
	/*
	** Now just get the process id which is the xx part of II/IUSVR/xx
	*/
	pid = get_process_pid(iirecovery_id);

	if (kill)
	{
            hProcess = OpenProcess( PROCESS_TERMINATE,
                                    FALSE,
                                    pid );

	}
        PRINT( ERget( S_ST062F_stop_rcp ) );
        STprintf( cmdbuf, ERx( "rcpconfig %s -silent" ), cmdopt );
        system( cmdbuf );
	if (!dbms_up)
	{
            PRINT( ERget( S_ST0630_log_wait ) );
	    /* limit the time to try this */
            for (i = 0; i < 7 && rcp_up; i++)
            {
            	/* exit code "0" means the RCP is online still */
		if (start_process( ERx( "rcpstat -online -silent" ) ) == 0)
		{
                    PCsleep(3000);
		    PRINT(".");
		}
		else
		    rcp_up = FALSE;
            }
	    PRINT("\n\n");
	}
	/* We know from observation that even though 'rcpstat' indicates
	** logging is off-line, dmfrcp continues to run for a little longer
	** to clean up things
	*/
	rcp_up = (wait_for_process_death(dmfrcp, pid, 30, NULL) == FAIL) ? TRUE : FALSE;

	if (rcp_up && kill)
        {
            if (hProcess != NULL)
	    {
		TerminateProcess( hProcess, 0 );
	    }
        }
        /* Regardless whether rcp_up is true, the process handle MUST be
        ** closed or the process won't really go away
        */
        if (hProcess != NULL)
	{
	    CloseHandle( hProcess );
	    hProcess = NULL;
	}
	if (rcp_up && !kill)
	{
	    PRINT(ERget( S_ST0624_fatal_err ));
	    F_PRINT( ERget( S_ST0631_fail_rcp_stop ), iirecovery_id );
	    if ((find_process(iidbms, 0) == OK) ||
		(find_process("iistar", 0) == OK))
	    {
		F_PRINT( ERget( S_ST0632_manual_stop ), ingstop );
	    }
	    else
	    {
		F_PRINT( ERget( S_ST0626_alt_dbms_conn ), ingstop );
	    }
	}
	if (!rcp_up)
	{
            PRINT( ERget( S_ST0633_rcp_exited ) );
	    iirecovery_count = 0;
	}
    }
    else
    {
	rcp_up = FALSE; /* No recovery servers found */
        if (!Is_w95 && !Is_Jasmine && !Is_DBATools)
        {
    	    if (find_process(dmfrcp, 0) == OK)
	    {
                STprintf( cmdbuf, ERx( "rcpconfig %s -silent" ), cmdopt );
                system( cmdbuf );
	    }
        }
    }
}

static
BOOL
start_process( char *cmd )
{
    BOOL                    status;
    STATUS                  wait_status;
    PROCESS_INFORMATION     cmd_info;
    STARTUPINFO             startup;
    SECURITY_ATTRIBUTES	    handle_security, child_security;

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;

    child_security.nLength = sizeof(child_security);
    child_security.lpSecurityDescriptor = NULL;
    child_security.bInheritHandle = TRUE;

    MEfill(sizeof(STARTUPINFO), '\0', &startup);
    startup.cb = sizeof(STARTUPINFO);

    status = CreateProcess( NULL,
                            cmd,
                            &handle_security,
                            &child_security,
                            FALSE,
                            DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                            NULL,
                            NULL,
                            &startup,
                            &cmd_info );
    if (status == TRUE)
    {
        wait_status = WaitForSingleObject( cmd_info.hProcess, INFINITE );
        status = GetExitCodeProcess( cmd_info.hProcess, &wait_status );
        CloseHandle( cmd_info.hProcess );
        CloseHandle( cmd_info.hThread );
    }
    return wait_status;
}


/*
**  Execute a command, blocking during its execution.
*/
static STATUS
execute( char *cmd )
{
        CL_ERR_DESC err_code;

        return( PCcmdline( (LOCATION *) NULL, cmd, PC_WAIT,
                (LOCATION *) NULL, &err_code) );
}

/*
** Name: get_server_classnames
**
** Description:
**
**      Query config.dat to find the server class names for the DBMS and Star.
**      These are user configurable parameters, so we cannot depend upon the
**      default values of "STAR" and "INGRES".
**
** Inputs:
**      none
**
** Outputs:
**      If the values are successfully
**      queried they are pointed at by the static variables iidbms_key and
**      iistar_key on return. These variables are not updated if the PMget
**      call fails.
**
** Returns:
**      void
**
** History:
**      13-nov-96 (wilan06)
**          Created
**	25-mar-1998 (somsa01)
**	    Modified so that we can get multiple DBMS and STAR server
**	    classes.  (Bug #89972)
*/
static void get_server_classnames (void)
{
    char        buffer[256];
    char*       className = NULL;
    char*       host = PMhost();
    char	*regexp, *name;
    int		i;
    STATUS	status;
    PM_SCAN_REC	state1;
    BOOL	ClassExists;

    num_dbms_classes = -1;
    STprintf (buffer, ERx( "%s.%s.dbms.%%.server_class" ), SystemCfgPrefix, host);
    regexp = PMexpToRegExp (buffer);
    for (
	    status = PMscan( regexp, &state1, NULL, &name, &className );
	    status == OK;
	    status = PMscan( NULL, &state1, NULL, &name, &className ) )
    {
	ClassExists = FALSE;
	if (num_dbms_classes != -1)
	{
	    for (i = 0; i <= num_dbms_classes; i++)
		if (!STcompare(iidbms_key[i], className))
		{
		    ClassExists = TRUE;
		    break;
		}
	}

	if (!ClassExists)
	    STcopy( className, iidbms_key[++num_dbms_classes] );
    }

    num_star_classes = -1;
    STprintf (buffer, ERx( "%s.%s.star.%%.server_class" ), SystemCfgPrefix, host);
    regexp = PMexpToRegExp (buffer);
    for (
	    status = PMscan( regexp, &state1, NULL, &name, &className );
	    status == OK;
	    status = PMscan( NULL, &state1, NULL, &name, &className ) )
    {
	ClassExists = FALSE;
	if (num_star_classes != -1)
	{
	    for (i = 0; i <= num_star_classes; i++)
		if (!STcompare(iistar_key[i], className))
		{
		    ClassExists = TRUE;
		    break;
		}
	}

	if (!ClassExists)
	    STcopy( className, iistar_key[++num_star_classes] );
    }
}


/*
** Name: gateway_configured
**
** Description:
**
**      Query config.dat to find the startup count for the specifed gateway
**	class.   If one is found, then the gateway had been installed.
**
** Inputs:
**      gateway_class - Specifies the gateway server class.
**
** Outputs:
**      None
**
** Returns:
**      TRUE  - Gateway server class was configured.
**	FALSE - Gateway wasn't configured.
**
** History:
**	02-apr-98 (mcgem01)
**		Created.
**	19-nov-02 (mofja02)
**		added ERx around literal string.
*/
static BOOL
gateway_configured (char * gateway_class)
{
	char buffer[ 256 ];
	char *host = PMhost();
	char *start_val = NULL;

        /*
	** scan for ii.$.ingstart.%.<gateway_class>
	*/
	STprintf (buffer, ERx( "%s.%s.ingstart.*.%s" ), SystemCfgPrefix, host,
		gateway_class);

        if ((PMget (buffer, &start_val) == OK) && start_val && *start_val)
		return TRUE;
	else
		return FALSE;
}

/*
** Name: get_sys_dependencies -   Set up global variables
**
** Description:
**      Replace system-dependent names with the appropriate
**      values from gv.c.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Side effects:
**      initializes global variables
**
** History:
**      04-apr-1997 (canor01)
**          Created.
*/
static void
get_sys_dependencies()
{
    OSVERSIONINFO lpvers;
    LOCATION      sysloc;
    char          pathbuf[MAX_LOC+1];
    char          *str;

/* this define for windows95 was not in c++ 2.0 headers */

# ifndef VER_PLATFORM_WIN32_WINDOWS
# define VER_PLATFORM_WIN32_WINDOWS 1
# endif /* VER_PLATFORM_WIN32_WINDOWS */

    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);
    Is_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)? TRUE: FALSE;

    Is_Jasmine = (STcompare("jas", SystemExecName) == 0);

    STprintf( ingstop, "%sstop", SystemExecName );
    STprintf( iinamu, "%snamu", SystemAltExecName );
    STprintf( iimonitor, "%smonitor", SystemAltExecName );
    STprintf( iigcn, "%sgcn", SystemAltExecName );
    STprintf( iigcc, "%sgcc", SystemAltExecName );
    STprintf( iigcd, "%sgcd", SystemAltExecName );
    STprintf( iigcb, "%sgcb", SystemAltExecName );
    STprintf( iidbms, "%sdbms", SystemAltExecName );
    STprintf( iigcstop, "%sgcstop", SystemAltExecName );
    STprintf( iigcn_key, "%sNMSVR", SystemVarPrefix );

    STprintf( oracle, "%sgworad", SystemAltExecName );
    STprintf( informix, "%sgwinfd", SystemAltExecName );
    STprintf( mssql, "%sgwmssd", SystemAltExecName );
    STprintf( sybase, "%sgwsybd", SystemAltExecName );
    STprintf( odbc, "%sgwodbd", SystemAltExecName );
    STprintf( db2udb, "%sgwudbd", SystemAltExecName );
    STprintf( gwstop, "%sgwstop", SystemAltExecName );

    if ( Is_Jasmine )
    {
	STcopy( "jasclean", iiclean );
	STcopy( "jasnetu", iinetu );
    }

    /* get the correct version of iiclean with full path */
    NMgtAt( SystemLocationVariable, &str );
    if ( str != NULL && *str != EOS )
    {
	STpolycat(3, "\"", str, "\"", &pathbuf);
	LOfroms( PATH, pathbuf, &sysloc );
	LOfaddpath( &sysloc, SystemLocationSubdirectory, &sysloc );
	LOfaddpath( &sysloc, "bin", &sysloc );
        LOfstfile( iiclean, &sysloc );
	if ( LOexist( &sysloc ) == OK )
	{
	    LOtos( &sysloc, &str );
	    STcopy( str, iiclean );
	}
	else
	{
	    STpolycat(3, "\"", str, "\"", &pathbuf);
	    LOfroms( PATH, pathbuf, &sysloc );
	    LOfaddpath( &sysloc, SystemLocationSubdirectory, &sysloc );
	    LOfaddpath( &sysloc, "utility", &sysloc );
	    LOfstfile( iiclean, &sysloc );
	    if ( LOexist( &sysloc ) == OK )
	    {
		LOtos( &sysloc, &str );
		STcopy( str, iiclean );
	    }
	}
    }
}

static STATUS
write_to_log( char *string)
{   STATUS status;
    i4 flag;
    CL_SYS_ERR sys_err;

    /* Open II_CONFIG!ingstart.log.  Append to the end of existing
    ** log file on UNIX, have VMS versioning to do the equivalent thing.
    */
#ifdef VMS
    flag = TR_F_OPEN;
#else
    flag = TR_F_APPEND;
#endif

    if ((status = TRset_file(flag, log_file_path, log_file_len, &sys_err)) != OK)
    {
        SIfprintf( stderr, "Error while opening %s\n", log_file_name);
        SIflush( stderr );
        return ( FAIL );
    }

    TRdisplay(string);
    TRflush();

    if ((status = TRset_file(TR_F_CLOSE, log_file_path, log_file_len, &sys_err)) != OK)
    {
        SIfprintf( stderr, "Error while closing %s\n", log_file_name);
        SIflush( stderr );
        return ( FAIL );
    }

    return( OK );
}


/*
**  Build a list of the specified server type to be started.
*/
static void
build_startup_list( PROCESS_LIST **list, char *type )
{
    STATUS status;
    PM_SCAN_REC state;
    PROCESS_LIST *rec, *cur;
    char expbuf[ BIG_ENOUGH ];
    char *regexp, *name, *value;
    i4 n;

    cur = *list = NULL;	/* initialize list */

    /* scan for ii.$.ingstart.%.<type> */
    STprintf( expbuf, ERx( "%s.$.%sstart.%%.%s" ), SystemCfgPrefix,
	    SystemExecName, type );

    regexp = PMmExpToRegExp( config, expbuf );

    for(
	    status = PMmScan( config, regexp, &state, NULL, &name,
	    &value ); status == OK; status = PMmScan( config, NULL,
	    &state, NULL, &name, &value ) )
    {
	    /* ignore non numeric data */
	    if( ! ValueIsInteger( value ) )
		    continue;

	    CVan( value, &n );

	    /* skip negative numbers */
	    if( n < 0 )
		    continue;

	    rec = (PROCESS_LIST *)  MEreqmem( 0, sizeof( PROCESS_LIST ),
		    FALSE, NULL );
	    if( rec == NULL )
		    ERROR( "MEreqmem() failed." );

	    if( *list == NULL )
		    cur = *list = rec;
	    else
	    {
		    cur->next = rec;
		    cur = rec;
	    }

	    rec->name = PMmGetElem( config, 3, name );
	    rec->count = n;
	    rec->next = NULL;
    }
}

/*
**  Build a list of servers to be started for each of the server types.
*/
static void
build_server_list( SERVER_LIST *list )
{
    char *host;
    char tmp_buf[BIG_ENOUGH];

    (void) PMmInit( &config );

    PMmLowerOn( config );

    if( PMmLoad( config, NULL, PMerror ) != OK )
	PCexit( FAIL );

    host = PMmHost( config );
    STprintf(tmp_buf,ERx("%sstart"), SystemExecName );
    PMmSetDefault( config, II_PM_IDX, SystemCfgPrefix );
    PMmSetDefault( config, HOST_PM_IDX, host );
    PMmSetDefault( config, OWNER_PM_IDX, tmp_buf );

    /* build DBMS server startup list */
    build_startup_list( &(list->dbms_list), ERx( "dbms" ) );

    /* build Net server startup list */
    build_startup_list( &(list->net_list), ERx( "gcc" ) );

    /* build Bridge server startup list */
    build_startup_list( &(list->bridge_list), ERx( "gcb" ) );

    /* build Data Access Server startup list */
    build_startup_list( &(list->das_list), ERx( "gcd" ) );

    /* build Star server startup list */
    build_startup_list( &(list->star_list), ERx( "star" ) );

/*
** NOTE: From other code in ingstop, it looks like the servers that need to be
**       checked are: gcn (name), rcp (recovery), gcc (net), gcb (bridge),
**       gcd (DAS), dbms (INGRES), and star (Star).
**       In the meantime, just leave these others out...
*/
# ifdef _TEMP_NOTE_
    /* build ice server startup list */
    build_startup_list( &(list->icesvr_list), ERx( "icesvr" ) );

    /* build Oracle Gateway startup list */
    build_startup_list( &(list->oracle_list), ERx( "oracle" ) );

    /* build Informix Gateway startup list */
    build_startup_list( &(list->informix_list), ERx( "informix" ) );

    /* build Sybase SQL Server Gateway startup list */
    build_startup_list( &(list->sybase_list), ERx( "sybase" ) );

    /* build RMS Gateway startup list */
    build_startup_list( &(list->rms_list), ERx( "rms" ) );

    /* build RDB Gateway startup list */
    build_startup_list( &(list->rdb_list), ERx( "rdb" ) );

    /* build rmcmd server startup list */
    build_startup_list( &(list->rmcmd_list), ERx( "rmcmd" ) );

    /* build db2udb server startup list */
    build_startup_list( &(list->db2udb_list), ERx( "db2udb" ) );

# ifdef NT_GENERIC
    /* build Microsoft SQL Server Gateway startup list */
    build_startup_list( &(list->mssql_list), ERx( "mssql" ) );

    /* build ODBC Gateway startup list */
    build_startup_list( &(list->odbc_list), ERx( "odbc" ) );

    /* build Gateway server startup list */
    if (Is_w95)
	    build_startup_list( &(list->gateway_list), ERx( "iigws" ) );
# endif
# endif
}

/*
**  Check list of servers against those we found active.
*/
static int
check_server_list( SERVER_LIST *list )
{
    i4	rc = 0;

    /* check name server */
    if (iigcn_count == 0)
    {
	F_PRINT( ERget(S_ST06BD_one_server_running), iigcn );
	++rc;
    }
    /* check recovery server */
    if (iirecovery_count == 0)
    {
	F_PRINT( ERget(S_ST06BD_one_server_running), dmfrcp );
	++rc;
    }
    /* check DBMS server startup list */
    if (list->dbms_list && list->dbms_list->count != iidbms_count)
    {
	if (list->dbms_list->count == 1) {
	    F_PRINT( ERget(S_ST06BD_one_server_running), iidbms );
	}
	else {
	    F_PRINT2( ERget(S_ST06BE_many_servers_running),
 	            list->dbms_list->count, iidbms );
 	}
	++rc;
    }
    /* check Net server startup list */
    if (list->net_list && list->net_list->count != iigcc_count)
    {
	if (list->net_list->count == 1) {
	    F_PRINT( ERget(S_ST06BD_one_server_running), iigcc );
	}
	else {
	    F_PRINT2( ERget(S_ST06BE_many_servers_running),
		list->net_list->count, iigcc );
	}
	++rc;
    }
    /* check Bridge server startup list */
    if (list->bridge_list && list->bridge_list->count != iigcb_count)
    {
	if (list->bridge_list->count == 1) {
	    F_PRINT( ERget(S_ST06BD_one_server_running), iigcb );
	}
	else {
	    F_PRINT2( ERget(S_ST06BE_many_servers_running),
		list->bridge_list->count, iigcb );
	}
	++rc;
    }
    /* check Data Access Server startup list */
    if (list->das_list && list->das_list->count != iigcd_count)
    {
	if (list->das_list->count == 1) {
	    F_PRINT( ERget(S_ST06BD_one_server_running), iigcd );
	}
	else {
	    F_PRINT2( ERget(S_ST06BE_many_servers_running),
		list->das_list->count, iigcd );
	}
	++rc;
    }
    /* check Star server startup list */
    if (list->star_list && list->star_list->count != iistar_count)
    {
	if (list->star_list->count == 1) {
	    F_PRINT( ERget(S_ST06BD_one_server_running), ERx("star") );
	}
	else {
	    F_PRINT2( ERget(S_ST06BE_many_servers_running),
		list->star_list->count, "star" );
	}
	++rc;
    }
/*
** NOTE: From other code in ingstop, it looks like the servers that need to be
**       checked are: gcn (name), rcp (recovery), gcc (net), gcb (bridge),
**       gcd (DAS), dbms (INGRES), and star (Star).
**       In the meantime, just leave these others out...
*/
# ifdef _SEE_NOTE_ABOVE_
    /* check ice server startup list */
    if (list->icesvr_list && list->icesvr_list->count != icesvr_count)
	++rc;
    /* check Oracle Gateway startup list */
    if (list->oracle_list && list->oracle_list->count != oracle_count)
	++rc;
    /* check Informix Gateway startup list */
    if (list->informix_list && list->informix_list->count != informix_count)
	++rc;
    /* check Sybase SQL Server Gateway startup list */
    if (list->sybase_list && list->sybase_list->count != sybase_count)
	++rc;
    /* check RMS Gateway startup list */
    if (list->rms_list && list->rms_list->count != rms__count)
	++rc;
    /* check RDB Gateway startup list */
    if (list->rdb_list && list->rdb_list->count != rdb_count)
	++rc;
    /* check rmcmd server startup list */
    if (list->rmcmd_list && list->rmcmd_list->count != iirmcmd_count)
	++rc;
    /* check db2udb server startup list */
    if (list->db2udb_list && list->db2udb_list->count != db2udb_count)
	++rc;
# ifdef NT_GENERIC
    /* check Microsoft SQL Server Gateway startup list */
    if (list->mssql_list && list->mssql_list->count != mssql_count)
	++rc;
    /* check ODBC Gateway startup list */
    if (list->odbc_list && list->odbc_list->count != odbc_count)
	++rc;
    /* check Gateway server startup list */
    if (Is_w95)
	if (list->gateway_listlist && gateway_list->count != iigws_count)
	    ++rc;
# endif
# endif
    return rc;
}

static STATUS
ingstop_chk_priv( char *user_name, char *priv_name )
{
	char	pmsym[128], *value, *valueptr ;
	char	*strbuf = 0;
	int	priv_len;

        /*
        ** privileges entries are of the form
        **   ii.<host>.privileges.<user>:   SERVER_CONTROL, NET_ADMIN ...
        */

	STpolycat(2, ERx( "$.$.privileges.user." ), user_name, pmsym);

	/* check to see if entry for given user */
	/* Assumes PMinit() and PMload() have already been called */

	if( PMget(pmsym, &value) != OK )
	    return E_GC003F_PM_NOPERM;

	valueptr = value ;
	priv_len = STlength(priv_name) ;

	/*
	** STindex the PM value string and compare each individual string
	** with priv_name
	*/
	while ( *valueptr != EOS && (strbuf=STchr(valueptr, ',')))
	{
	    if (!STscompare(valueptr, priv_len, priv_name, priv_len))
		return OK ;

	    /* skip blank characters after the ','*/
	    valueptr = STskipblank(strbuf+1, 10);
	}

	/* we're now at the last or only (or NULL) word in the string */
	if ( *valueptr != EOS  &&
              !STscompare(valueptr, priv_len, priv_name, priv_len))
		return OK ;

	return E_GC003F_PM_NOPERM;
}

/*
** Name: check_cache_sharing
**
** Description:
**
**      Query config.dat to find the cache_sharing setting.
**
** Inputs:
**      NONE
**
** Outputs:
**      None
**
** Returns:
**      TRUE  - Cache Sharing turned ON
**	FALSE - Cache Sharing turned OFF
**
** History:
**	15-Jul-08 (bonro01)
**		Created.
*/
static BOOL 
check_cache_sharing (void)
{
	char buffer[ 256 ];	
	char *host = PMhost();
	char *value = NULL;

        /* 
	** scan for ii.$.dbms.%.cache_sharing
	*/
	STprintf (buffer, ERx("%s.%s.dbms.*.cache_sharing"), SystemCfgPrefix, host);

	/* PMget should not normally fail, but if it does, treat it as OFF */
        if ((PMget (buffer, &value) != OK) )
	    return FALSE;	

	if ( STcasecmp( value, "ON" ) == 0 )
		return TRUE;
	else
		return FALSE;
}

static void
call_iimonitor(const char *cmd, const char *process)
{
	create_command_file();
	append_to_command_file(cmd);
	append_to_command_file(ERx("quit"));

	STprintf( cmdbuf, ERx("%s %s <\"%s\" >nul"),
		 iimonitor, process, cmndfile_buf );
	system ( cmdbuf );

	delete_command_file();
}

/*
** Name: find_process
**
** Description:
**      Find a running process by name and/or id.
**
** Inputs:
**      process_name    = partial name of process to wait for
**      pid             = process ID (0 if only search by name)
**
** Outputs:
**      none
**
** Returns:
**      OK if process exists
**      FAIL if process has gone away
**
** Side effects:
**      none.
**
** History:
**      07-May-2008 (whiro01)
**          Created.
*/
static STATUS
find_process(char *process_name, DWORD pid)
{
	if (pid == 0)
	{
	    if (process_name != NULL && *process_name != '\0')
		return ((PCget_PidFromName(process_name) != 0) ? OK : FAIL);
	    else
		/* Can't find anything given no information!! */
		return FAIL;
	}
	else
	{
	    if (process_name != NULL && *process_name != '\0')
		return ((PCsearch_Pid(process_name, pid) != 0) ? OK : FAIL);
	    else
		return (PCis_alive(pid) ? OK : FAIL);
	}
}

/*
** Name: wait_for_process_death
**
** Description:
**      Detect and/or wait for a process to complete.
**
** Inputs:
**      process_name    = partial name of process to wait for
**      pid             = process ID (0 if only search for name)
**      wait            = wait flag (0 = detect, don't wait)
**                        (otherwise number of seconds to wait)
**
** Outputs:
**      none
**
** Returns:
**      OK if process finishes in time (or is not found on "detect")
**      FAIL if process is still running when we finish
**
** Side effects:
**      none.
**
** History:
**      07-May-2008 (whiro01)
**          Created.
*/
static STATUS
wait_for_process_death(char *process_name, DWORD pid, int wait, char *prompt)
{
	STATUS status = OK;
	int secs;

	if (wait > 0)
	{
	    for (secs = 0; secs < wait; secs++)
	    {
		if (find_process(process_name, pid) == FAIL)
		{
		    /* That's a good sign that we didn't find it running */
		    status = OK;
		    break;
		}
		else
		{
		    /* It's still hanging around, so wait another second for it to die */
		    status = FAIL;
		    if (prompt != NULL && *prompt != '\0')
			PRINT(prompt);
		    PCsleep(1000);
		}
	    }
	}
	else
	{
	    /* Reverse the sense of the return value from 'find_process' */
	    status = (find_process(process_name, pid) == FAIL) ? OK : FAIL;
	}

	return status;
}

static DWORD
get_process_pid(char *name)
{
	char *s;
	DWORD pid;
	for (s = name + STlength(name) - 1; *s != '\\'; s--)
		;
	s++;
	STscanf(s,"%x", &pid);
	return pid;
}

static void
print_service_os_error()
{
	int OsError = GetLastError();
	char *OsErrText;

	if (OsError != ERROR_SERVICE_NOT_ACTIVE)
	{
		int	rc;
		rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_ARGUMENT_ARRAY |
			FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			OsError,
			LANG_NEUTRAL,
			(char *)&OsErrText,
			0,
			NULL);
		if (rc == 0)
		    STcopy( ERx("unknown"), OsErrText );

		F_PRINT3( ERget( S_ST0653_stop_svc_fail ),
			  SystemProductName, OsError, OsErrText );
	}
}

static STATUS
shutdown_service()
{
	SC_HANDLE	schSCManager, OpIngSvcHandle;
	SERVICE_STATUS	ssServiceStatus;
	LPSERVICE_STATUS lpssServiceStatus = &ssServiceStatus;
	HCLUSTER	hCluster = NULL;
	HRESOURCE	hResource = NULL;
	WCHAR	lpwResourceName[256];
	char	*ResourceName;
	CLUSTER_RESOURCE_STATE 	dwState;
	DWORD   rc;
	int     count;
	int     timeout;

	F_PRINT( ERget( S_ST0652_stop_service ), SystemProductName );

	hCluster = OpenCluster(NULL);
	if (hCluster)
	{
	    NMgtAt( ERx( "II_CLUSTER_RESOURCE" ), &ResourceName );
	    mbstowcs(lpwResourceName, ResourceName, 256);
	    hResource = OpenClusterResource(hCluster, lpwResourceName);
	    if (hResource)
	    {
		dwState=GetClusterResourceState(hResource, NULL, NULL, NULL, NULL);
		if(dwState == ClusterResourceOffline)
		{
		    PRINT("Cluster resource is already offline.\n");
		    return OK;
		}

		count = 1;
		timeout = 60;
		while(1)
		{
		    rc = OfflineClusterResource(hResource);
		    if (rc != ERROR_SUCCESS)
		    {
			if(timeout-- == 0)
		        {
		            PRINT("Timeout for cluster offline exceeded.  Exiting\n");
		            return FAIL;
		        }

			if(rc==997 || rc==5023)
			{
			    F_PRINT("%d: Cluster resource not in the correct state.  Trying again.\n", count++);
			    PCsleep(1000);
			    continue;
			}
			else
			{
			    F_PRINT("OfflineClusterResource failed with error %d\n", rc);
			    return FAIL;
			}
		    }
		    else
			break;
		}

		timeout = 60;
		while(1)
		{
		    dwState=GetClusterResourceState(hResource, NULL, NULL, NULL, NULL);
		    if (dwState == ClusterResourceInitializing ||
			dwState == ClusterResourcePending ||
			dwState == ClusterResourceOfflinePending)
		    {
			PRINT(".");
			PCsleep(1000);
		    }
		    else if (dwState == ClusterResourceOffline)
		    {
			PRINT(ERget(S_ST06B8_offline_successfully));
			return OK;
		    }
		    else if(dwState == ClusterResourceStateUnknown)
		    {
			F_PRINT2(ERget(S_ST06B9_offline_fail), "Unknown state: ", GetLastError());
			return FAIL;
		    }
		    else
		    {
			F_PRINT2(ERget(S_ST06B9_offline_fail), "Unexpected state: ", dwState);
			return FAIL;
		    }

		    if(timeout-- == 0)
		    {
			PRINT("Timed out waiting for Ingres resource to come online.  Exiting\n");
			return FAIL;
		    }
		}
	    }
	}

	if ( (schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)) )
	{
	    if ( (OpIngSvcHandle = OpenService( schSCManager, ServiceName,
						SERVICE_USER_DEFINED_CONTROL |
						SERVICE_STOP )) )
	    {
		if (!ControlService(OpIngSvcHandle,
			SERVICE_CONTROL_STOP_NOINGSTOP,
			lpssServiceStatus))
		{
		    print_service_os_error();
		}
		else
		{
		    while (QueryServiceStatus(OpIngSvcHandle,
					      lpssServiceStatus) &&
				ssServiceStatus.dwCurrentState != SERVICE_STOPPED)
			;
		}
		CloseServiceHandle(OpIngSvcHandle);
	    }
	    else
	    {
		print_service_os_error();
	    }

	    CloseServiceHandle(schSCManager);
	}
	else
	{
	    print_service_os_error();
	}
	return OK;
}

static STATUS
create_command_file()
{
	char fname[200], *s;

	STprintf(cmndfile_buf, ERx("%s"), ii_temp);
	LOfroms(PATH, cmndfile_buf, &ii_cmndfile);

	/* append "ingstop.$$1" to ii_temp location */
	STprintf(fname, ERx("%s.$$1"), ingstop);
	LOfstfile(fname, &ii_cmndfile);
	LOtos(&ii_cmndfile, &s);

	return LOcreate(&ii_cmndfile);
}

static STATUS
append_to_command_file(const char *line)
{
	FILE *fdesc;
	STATUS status;
	if ((status = SIopen(&ii_cmndfile, "a", &fdesc)) == OK)
	{
		SIfprintf(fdesc, ERx("%s\n"), line);
		SIclose(fdesc);
	}
	return status;
}

static STATUS
append_show_to_command_file(const char *cmd)
{
	char buf[256];
	STprintf(buf, ERx("show %s"), cmd);
	return append_to_command_file(buf);
}

static STATUS
delete_command_file()
{
	return LOdelete(&ii_cmndfile);
}
