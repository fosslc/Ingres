:
#  Copyright (c) 1992, 2010 Ingres Corporation
#
#  Name:  
#	ingstop -- shuts down running Ingres processes.
#
#  Usage: 
#	ingstop [ -timeout=seconds ] [ -force | -immediate ] [ -kill ] [ -show ]
#		[ -component[=idlist] ...]
#
#		/* additional options for clustered Ingres */
#
#		[ -c[luster] | [-n[ode]=]{ nodename | nickname } ... ]
#
#  Description: 
#	This program shuts down a running Ingres instance (installation)
#	in whole or in part.
#
#	With neither -force nor -kill options, if there are active
#	sessions, the installation will not shut down.
#
#	With the addition of -force, servers will be forcibly
#	closed (using normal force-termination commands), even if
#	there are active sessions remaining.  ingstop will wait
#	as long as needed for any recovery rollback to occur.
#
#	With the further addition of -kill, (dbms) servers that
#	don't stop are force-terminated with kill -9.  Also,
#	we only wait a predefined time (100 seconds) for recovery
#	to finish before killing the recovery server as well.
#	(-kill by itself is not especially useful.)
#
#	The -immediate option implies -force, but eliminates some
#	of the "nice shutdown" delays that -force allows.  Typically,
#	-immediate would be used with -kill.
#
#	Summary:
#	To shutdown only if idle: ingstop
#	To shutdown as cleanly as possible, idle or not: ingstop -force
#	To shutdown even if ingres is stuck/broken/looping: ingstop -f -kill
#	To shutdown as fast as possible without being clean: -imm -kill
#
#  Exit Status:
#	0	the requested operation succeeded.
#	1	the requested operation failed.
#
## History:
##	16-feb-93 (tyler)
##		Adapted from iishutdown.	
##	15-mar-93 (tyler)
##		Cosmetic changes.
##	24-may-93 (tyler)
##		Added code which waits for the recovery process to exit
##		before continuing.
##	21-jun-93 (tyler)
##		Moved code which kills Net servers before other murderous 
##		sections as per suggestion of IUA attendee who observed
##		that killing them last resulted in remote sessions hanging 
##		around indefinitely.  Changed -kill option to -f.
##	19-oct-93 (tyler)
##		Changes to support shutdown of recovery server now named
##		"iidbms recovery" in the Unix process list.  Improved
##		shutdown messages.
##	21-oct-93 (tyler)
##		Fixed bug which caused Star servers to be counted among
##		unregistered DBMS servers, due to process name change.
##	22-nov-93 (tyler)
##		Modified to ignore distinction between registered
##		and private DBMS servers.  Replaced system calls with
##		shell procedure invocations.
##	31-jan-93 (tomm)
##		Added sleep between cscleanup and ipcclean.  Without
##		this HP will leave shared memory segments laying around.
##		I have no idea why.
##	01-beb-94 (fredv)
##		Backed out Tom's previous change. Will look for a better
##		answer to this bug.
##	22-feb-94 (tyler)
##		Merged contents of iisdall, which isn't needed.  Fixed
##		BUG 59325: make sure exit code is correct.  Addressed
##		SIR 59339.
##	18-apr-94 (peterk) bug 62547
##		test for dbms server processes fails if dbms config flavor
##		contains strings "star" or "recovery"; make it smarter.
##	28-Dec-1994 (canor01)
##		when passing null input to 'uniq' on AIX 4.1, an error
##		is written to standard output. passed it to /dev/null.
##		Also add change 'timeout' to '-timeout' in Usage.
##	19-mar-1996 (sweeney)
##		Missed comment for 12-mar-1996 change; here it is.
##		Added trap to catch programmatic exit of script on
##		error.
##	10-apr-96 (nick)
##		If a server failed to shutdown because it had active 
##		sessions yet it did terminate before we had a chance to
##	        mark it open again, catch this rather than complaining that
##		we couldn't reconnect and then aborting. #62361
##	21-Feb-96 (rajus01)
##		Added support for stopping Protocol Bridge.
##	27-march-1997(angusm)
##		For NFS client, don't clean up shared memory. (bug 54496)
##		For -force option, ensure that shared memory is 
##		cleared up. (bug 81350)
##	25-jul-97(angusm)
##		bug 79539: code that checks if process is still up
##		can get a false positive.
##	6-feb-1998(angusm)
##		back out change 424826 w.r. this file:
##		'trap 0' command prevents error status passing back
##		to shell
##	25-mar-1998 (canor01)
##		Add gateways.
##	14-jul-1998 (hanch04)
##		replace iigwshut with iigwstop
##	08-apr-1998 (canor01)
##		Fix typo in previous submission. Change OpenIngres to Ingres.
##	17-aug-98 (mcgem01)
##		Fix grammatical errors in the gateway shutdown messages where
##		there is a single gateway running in the installation.
##	26-Aug-1998 (hanch04)
##		Added shutdown for rmcmd.
##      11-Mar-1999 (peeje01)
##              Add shutdown for icesvr.
##	21-apr-1999 (walro03)
##		Typo in msg about uncommitted transactions next time Ingres
##		starts up.
##	11-may-1999 (popri01)
##		Some platforms need a moment after "set server shut" to
##		allow the server to quiesce. Otherwise, ingstop may encounter
##		"sessions still active" and even occasionally abort. To
##		accomplish this, the script now checks "seconds" instead of 
##		"minutes" when determining whether to issue a sleep and 
##		"seconds" is initialized to 10 seconds. However,
##		we will not issue a "waiting" message for the sleep.
##		Platforms included in this change are Unixware (usl_us5),
##		Alpha (axp_osf), Solaris/Intel (sui_us5), HP (hp8/hpb_us5).
##	21-may-1999 (walro03)
##		Add Sequent (sqs_ptx) to the 11-may-1999 popri01 change.
##	25-Jun-1999 (ahaal01)
##		Add AIX (rs4_us5) to the 11-may-1999 popri01 change.
##      01-Aug-1999 (linke01)
##              Added SGI (sgi_us5) to the 11-may-1999 propri01 change.
##	24-jan-2000 (marro11)
##		Drop in logic that allows user to pick object to be stopped.
##	10-Feb-2000 (somsa01, marro11)
##		Added writing to the ingstart.log file.
##	07-mar-2000 (gupsh01)
##              Modified usage to include [ -rmcmd ].
##	24-Feb-2000 (rajus01)
##		Added shutdown for JDBC server. Use iigcstop instead of
##		netutil to stop net, bridge severs.
##	21-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products. Also, circumvent certain checks in the case of
##		other products.
##      02-June-2000 (gupsh01)
##              Modified the usage message to indicate shutting down of 
##		individual servers.
##	06-June-2000 (gupsh01)
##		Modified the usage message as shutting down the individual 
##		servers is only allowed for internal ingres processes. 
##    14-Jul-2000 (hanje04)
##            Added missing $ to icesvr_count which caused ingstop to fail
##            on Linux (and probably other's although no confirmed), and
##            added int_lnx to to list of platforms waiting after 
##            "set server shut"
##	11-Jul-2000 (clach04) 101879
##		Simplified gateway shutdown calls
##		Gateway shutdown failure aborts ingres installation shutdown
##		Added capability to -force and -kill gateway
##      14-Aug-2000 (hweho01)
##             Add ris_u64 port to the 11-may-1999 popri01 change.
##	16-aug-2000 (somsa01)
##		Added ibm_lnx to LINUX-specific changes.
##	08-May-2001 (lavcl02)
##              When the hostname contains mixed case characters , Ingstop
##              doesn't execute ipcclean. This happens because in config.dat
##              all the hostnames are stored in lower case. Changed grep for
##              hostname matching in grep -i. (B86559/INGSRV175)
##	11-jul-01(carsu07)
##		When using ingstop -force, recheck for running servers every
##		time a server is killed. This is to prevent errors when 2 or
##		more servers share a cache. The first server is stopped with
##		iimonitor which automatically stops the second server. Then
##		when you try to stop the second server with iimonitor you get
##		an error.  (Bug #86961)
##	11-jul-01(carsu07)
##		When iimonitor cannot stop a server this could be caused by a
##		shared cache server in the middle of shutting down. Issues a
##		warning message to this effect.
##	11-jul-01(carsu07)
##		When the -kill flag is used and there are 2 or more shared
##		cache  servers with active sessions, ingstop will first try
##		to shut down the servers cleanly, if that fails try to force
##		the servers to stop. Finally use kill -9 to kill a server.
##		(Bug #85222)
##	11-jul-01 (bobmart)
##		Merged duplicate code sections.  Addressed timing issue
##		seen when "set server shut" actually brings server down.
##	11-jul-01 (bobmart)
##		Modified logic introduced in previous change so that it
##		works for unix domain sockets (II_GC_PROT=UNIX).  Fix typo.
##	08-Sep-2000 (hanje04)
##		Added axp_lnx (Alpha Linux) to list of platforms waiting after
##		"set server shut"	
##	07-jan-2002 (devjo01)
##		Added options to allow ingstop to operate on an entire
##		cluster, or a set of remote nodes.
##	02-oct-2002 (devjo01)
##		Add options to allow ingstop to operate on NUMA clusters.
##      08-Oct-2002 (hanje04)
##          As part of AMD x86-64 Linux port, replace individual Linux
##          defines with generic _lnx define where apropriate
##	25-oct-02 (mofja02)
##		Added support for DB2 UDB.
##      21-Oct-04 (ashco01) INGNET152, b113279
##              Amended call to iigcstop to include '-s' flag when running
##              ingstop in -force or -kill modes to ensure active IIGCCs are
##              shutdown.
##      13-Feb-2003 (wansh01)
##              Added shutdown for DAS server.
##      15-Apr-2003 (wansh01)
##              Changed "DAS" to "Data Access Server".
##	05-Jan-2004 (hanje04)
##		Remove line break on gcd shutdown
##	20-feb-2004 (devjo01)
##		Remove (again?) line break on gcd shutdown.
##	20-May-2004 (hanje04)
##		Remove (yes, again) line break on gcd shutdown.
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##	15-Feb-2005 (drivi01)
##	    Added routines for proccessing a new cbf value ii.$.prefs.ingstop.
##	    ii.$.prefs.ingstop can be set to -force or -f and -kill or -k, and
##	    if it's set ingstop will process it as one of its parameters, 
##	    however whatever is passed at the command line to ingstop over-
##	    writes the cbf value. Also added a new flag -try, which will 
##	    disable cbf settings.
##	    The priority of proccessing the flags is specified in the following
##	    table:  (Assume O is no flag)
##		     
##		  CMD	
##	
##		   | O |try|-f | -i| -k|
##		___|___|___|___|___|___|___|___|
##		 O | O | O | -f| -i| -k|   |   |
##	CBF	___|___|___|___|___|___|___|___|
##		 -f| -f| O | -f| -i| -k|   |   |
##		___|___|___|___|___|___|___|___|
##		 -k| -k| O | -f| -i| -k|   |   |
##		___|___|___|___|___|___|___|___|
##
##	15-Feb-2005 (sheco02)
##	    Minor fix for the previous change 475137 for ingstop.
##
##	15-Mar-2005 (mutma03)
##	    Minor fix to display node names correctly. Used `uname -n` 
##	instead of iipmhost -local for displaying node name.
##      16-Mar-2005 (ashco01) INGSRV3197 b110276
##              Added '-show' option to Unix version as documentation 
##              suggests that this functionality is available on all 
##              platforms. 
##      07-Apr-2005 (wansh01)
##	   for -force and -kill option and gcf servers, use -s for ingcstop 	
##	   instead -q. 
##	20-Apr-2005 (mutma03)
##         Fixed ingstop -cluster, also added sanity check to prevent the
##         execution of ingstop -cluster if remote shell is not defined.
##	06-May-2005 (mutma03)
##	   Minor fix which was missed out for the previous change 476650.   
##	28-May-2005 (hanje04)
##	    Add Mac OS to sleep on shutdown.
##	14-Jun-2005 (mutma03)
##	   Fixed the help menu. Fixed the issue, the ingstop -cluster does not
##	   shutdown by reissueing the shutdown command to rmcmd server on
##	   ingstop as the messages get lost on simultaneous execution of
##	   ingstop on all nodes with ingstop -cluster.
##	17-Jun-2005 (hanje04)
##	   BUG 114702
##	   If we're doing a full shutdown and gcn_count is 0 start one
##	   up to ensure we get a clean shutdown
##       6-Nov-2006 (hanal04) SIR 117044
##         Add int.rpl for Intel Rpath Linux build.
##	12-Feb-2006 (bonro01)
##	   Remove JDCB package.
##	12-Feb-2008 (hanje04)
##	    SIR S119978
##	    Replace mg5_osx with generic OSX
##      14-May-2008 (hanal04) Bug 120371
##         Prevent double shift during parameter parsing when -show is used.
##         Do not start the gcn if -show was specified.
##	07-Aug-2008 (bonro01)
##	   ingstop fails to shutdown rmcmd and iidbms when multiple iidbms
##	   are running and cache_sharing is OFF. The problem is that rmcmdstp
##	   is unable to send the shutdown command to the correct iidbms.
##	   The solution is to try to shutdown all iidbms first and ignore
##	   the first error which is caused by rmcmd still running, then
##	   shutdown rmcmd and the last iidbms.
##      18-Aug-2008 (horda03) Bug 120787
##         An ingstop -force -iidbms causes the DBMS servers to be shutdown. However,
##         this doesn't remove the shared cache memory (if exists). Thus the next
##         server to connect to the memory may hang if a buffer is mutexed or has a
##         IO flag set. Remove the shared cache when the DBMS servers are shutdown.
##      20-Aug-2008 (horda03) Bug 120787
##         Fix on 18th Aug, didn't work on all unix platforms, as ps -fe returns
##         different results.
##	29-Oct-2008 (hweho01)
##	   Add double quotes around string variable CACHE, avoid test error. 
##      11-Feb-2009 (hanal04) Bug 112398
##         rmcmdstp may fail if we have SOLE Server on and more than one
##         DBMS running. Treat the same as the Shared CACHE case, i.e.
##         quiesce DBMSs first then try to shutdown the rmcmd. Don't call
##         rmcmd a second time if the quiesced DBMS has exited because
##         the first rmcmdstp succeeded.
##	15-May-2009 (clach04) Bug 122078
##	   db2udb was not being checked for.
##	   Restored original code from change 461858.
##	22-Jun-2009 (kschendel) SIR 122138
##	   Check for r64_us5 as well as rs4..
##	15-Nov-2009 (kschendel) SIR 122890
##	    Integrate datallegro changes/fixes:
##	    Make "ingstop -force -kill" work hard to make sure that all
##	    ingres processes are stopped.  Kill -9 anything left lying
##	    around.  Rework id-lists into shell vars instead of temp files,
##	    which had a bad tendency to be left lying around if the
##	    ingstop hung up somewhere and got killed with kill -9.
##	    Fix some causes of junk files in /tmp.  Clean up some (not all)
##	    bogus indentation.
##	    Wrap commands that might hang indefinitely, like iimonitor and
##	    csreport, so that we can abort them and move on if necessary.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	5-May-2010 (kschendel) SIR 122890
##	    Some of the timers put in for 122890 were too short, especially
##	    for slower Solaris (SPARC) boxes.  (Solaris seems to do an awful
##	    lot in iisysdep, including compiling a small trial C program.)
##	    Increase some exec-with-timeout timers.
##
#  PROGRAM = (PROG2PRFX)stop
#
#  DEST = utility
#----------------------------------------------------------------------------
(LSBENV)

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib

usage1()
{
   wrap_lines 76 << ! | wmsg

Only use -force or -immediate, not both.

!
   exit 1
}

#
#	cleanup()
#
#	Remove temporary work files on exit
#
cleanup()
{
    [ -z "$debugging" ] && rm -f /tmp/*.$$ 2>/dev/null
}

#
# Checks if server $1 is still running, getting its pid from the csreport list.
# Returns 0 if running, non-zero otherwise.
#
server_active()
{
   scan_id=$1

   pid=`grep "connect id $scan_id" /tmp/csreport.$$ | awk '{ print $4 }'`
   [ -z "$pid" ] && return 1
   if kill -0 $pid >/dev/null 2>&1 ; then
	return 0
   fi
   return 1
}

#
# Waits no more than $2 seconds for server $1 to exit.  Escalates time spent
# napping between server checks (Fibonacci series).
# Return 0 if server process has exited; non-zero otherwise.
#
wait_on_server()
{
   scan_id=$1
   max_wait_on_server=`expr $2 + 1`

   cur_time=0
   cum_time=1
   new_time=1
   server_up=1
   while [ $cum_time -le $max_wait_on_server ]
   do
      sleep $cur_time
 
      server_active $scan_id ||
      {
         server_up=0
         break
      }
 
      cur_time=$new_time
      new_time=$cum_time
      cum_time=`expr $cum_time + $cur_time`
   done
 
   return $server_up
}

#  Writes messages to both stdout and "ingstart.log"
#
#  Supply
#    $*		: message(s) to write; if no args, read from stdin
#
#  Return:
#    0		: OK
#
wmsg()
{
    if [ $# -eq 0 ] ; then
	cat -     | sed 's/^/!/' | tee -a $lfile | sed 's/^.//'
    else
	echo "$*" | sed 's/^/!/' | tee -a $lfile | sed 's/^.//'
    fi
    return 0
}

#  Scans for requested list of object ids in supplied space-separated list.
#  List should be empty if there are no active objects
# 
#  Supply:
#    $1		: Name of variable containing active objects list
#		: (updated with selected object list)
#    $2		: Name of variable to receive number of matches
#    $3		: object type/description
#    $4-n	: list of objects to scan
#
#  Return:
#    $?		: status of call; 0 OK, != 0 fail
#    <stdout>	: diagnostics if some specified object isn't there
#
#
scan_oid()
{

    listvar=$1
    countvar=$2
    oidpres=`eval echo '$'$listvar`
    eval "$countvar=0"
    oiddesc="$3"
    [ -z "$oidpres" ] && return 0
    oidpres=' '"$oidpres"' '

    oidlist=""
    oidmiss=""
    if [ "$4" = "all" ] ; then
	oidlist="$oidpres"
    else
	shift
	shift
	shift
	for oid in $* ; do
	    oidx='.* '"$oid"' '
	    i=`expr "$oidpres" \: "$oidx"`
	    if [ $i -eq 0 ] ; then
		oidmiss="$oidmiss $oid"
	    else
		oidlist="$oidlist $oid"
	    fi
	done
	oidmiss=`echo "$oidmiss" | sed 's/^  *//'`
    fi
    if [ -n "$oidmiss" ] ; then
	oidcurr=`echo $oidpres`
	echo "An inputted $oiddesc id(s) is not currently active."
	echo "   Ignoring:  $oidmiss"
	echo "     Active:  $oidcurr"
	echo "  Targeting:  ${oidlist:-<none>}"
	echo ""
	eval "$listvar='$oidlist'"
    fi
    # clean out blanks before counting:
    oidlist=$oidlist
    if [ -z "$oidlist" ] ; then
	oidcount=0
    else
	oidcount=`echo $oidlist | tr ' ' '\012' | sort -u | wc -l | tr -d ' '`
    fi
    eval "$countvar=$oidcount"

    return 0
}

#  Reports status of supplied object with beautified / pluralized message.
# 
#  Supply:
#    $1		: count
#    $2		: max allowed
#    $3		: product installed (1 = true, 0 = false)
#    $4		: object type/description; example "name server (iigcn)"
#
#  Return:
#    $?		: status of call; 0 OK, != 0 fail
#
report_nicely()
{
(
    oid_cnt="$1"
    max_cnt="$2"
    instald="$3"
    oiddesc="$4"

    pluralize="sed -e 's/server/servers/' -e 's/process/processes/'"

    if [ $oid_cnt -eq 1 ] ; then
	# echo "The name server ((PROG1PRFX)gcn) is running." 
	# echo "There is one ICE server (icesvr) running." 
	if [ "$max_cnt" -eq 1 ] ; then
	    wmsg "The $oiddesc is running."
	else
	    wmsg "There is one $oiddesc running."
	fi

    elif [ $oid_cnt -gt 1 ] ; then
	# echo "There are too many ($oid_cnt) name servers ((PROG1PRFX)gcn) running." 
	# echo "There are $icesvr_count ICE servers (icesvr) running." 
	rptmsg0="$oid_cnt"
	[ "$oid_cnt" -gt "$max_cnt" ] && rptmsg0="too many ($oid_cnt)"
	echo "There are $rptmsg0 $oiddesc running." | eval "$pluralize" | wmsg

    elif [ $oid_cnt -eq 0 -a $instald -eq 1 ] ; then
	# echo "There is no name server ((PROG1PRFX)gcn) running." 
	# echo "There are no Oracle Gateway servers running."
	if [ "$max_cnt" -eq 1 ] ; then
	    wmsg "There is no $oiddesc running."
	else
	    echo "There are no $oiddesc running." | eval "$pluralize" | wmsg
	fi
    fi

    return 0
)
}


#
#	exec_with_timeout
#
#	Issue a command in the background and kill it if it takes too long.
#	Intended for use with commands like iimonitor that aren't expected
#	to take any great length of time (although the timeout can be
#	anything you like.)  If the command takes longer than specified,
#	it's killed with kill -9.
##
##	Initially, it's mainly used for things that talk to the DBMS
##	server, since that seems to be the main culprit.  Also, used
##	for the initial csreport, which can CPU-loop if the lock bit in
##	the system control segment is stuck on.
#
#	$1 = the max time to allow, in seconds
#	$2 = the command (quote it if there are arguments)
#
# 	Returns with $? set to exit status of the command.
#
##	Put any redirection, pipelines, etc on the exec_with_timeout
##	invocation, not the command itself.  For example:
##	Correct: exec_with_timeout 10 "foo -bar" | grep something
##	Wrong: exec_with_timeout 10 "foo -bar | grep something"
##	Also, stdin redirection may not always work depending on how the
##	called program does things.
##
##	This used to use kill -0 to test for the existence of the spawned
##	command process.  That doesn't work on Solaris, because for some
##	unknown reason, kill -0 zeros out the wait return status!
#

## Make sure we use ingstop's $$ and not some sub-shell that might exit...

cv=/tmp/iicv$$

exec_with_timeout()
{
    maxtime=$1
    rm -f $cv
    touch $cv

    # Run the command.  Don't use () or anything similar, we need the
    # pid of the command and not an executing shell.
    # $! is pid of last command executed in the background

    $2 & bgpid=$!

    # Spawn a watchdog subshell.  Normally it will just wake up, see
    # nothing, and exit.  The interlock file $cv ensures that the
    # watchdog is testing the command process, and not some other one
    # that was spawned perhaps later.
    (elapsed=0;
     while true ; do
	sleep 2;
	elapsed=`expr $elapsed + 2`
	if [ ! -r $cv ] ; then
	    break
	fi
	if [ $elapsed -ge $maxtime ] ; then
	    # It's still running, take it out the hard way
	    kill -9 $bgpid >/dev/null 2>&1
	    break
	fi
     done
    ) &
    wait $bgpid >/dev/null 2>&1
    ret=$?
    rm -f $cv
    return $ret
}


#
#	cleanup_shared_mem
#
#
#	Wait for any background processing to complete and collect logs.
#
report_remote_results()
{
    iteration=0
    while [ -n "${STOPPIDS}" ]
    do
	$PSCMD > /tmp/ps.$$
	for rinfo in $STOPPIDS
	do
	    stoppid="`echo $rinfo | sed 's/^.*\.//'`"
	    nodename="`echo $rinfo | sed 's/\..*$//'`"
	    mask="\$2==${stoppid} { print 1 }"
	    if [ -z "`awk \"$mask\" /tmp/ps.$$`" ]
	    then
		# Display results for remote ingstop (don't log it)
		cat << ! | center_text 76 | box 76 1 0 | wmsg
(PROD1NAME)/(PROG2PRFX)stop results for $nodename
!
		cat /tmp/$self_$nodename.$$

		# remove it from list
		mask="s/ `echo $rinfo | sed 's/[^ _A-Za-z0-9()]/\\\\&/g'` / /"
		STOPPIDS="`echo \"${STOPPIDS} \" | \
		 sed -e \"${mask}\" -e 's/ \$//'`"
		[ -z "$STOPPIDS" ] && break 2
	    elif [ $iteration -eq 0 ]
	    then
		# Whine about delay
		wrap_lines 76 << ! | wmsg

Waiting for $nodename to shutdown ...
!
	    fi
	done
	iteration=`expr $iteration + 1`
	[ $iteration -ge 10 ] && iteration=0
	sleep 6
    done
    return 0
}

#
#	exec_locals
#
#	Run local sub-processes to shut down other NUMA nodes on host.
#
#	in:	components - list of components to shut down.
#		targets - list of NUMA nodes this host.
#		STOPPIDS - list of pids for sub-processes.
#
#	out:	STOPPIDS - argumented with additional sub-process pids.
#
exec_locals()
{
    # Check if no need for action against other nodes this host.
    [ -n "$components" ] && 
    {
	notfirst=false
	for node in $targets
	do
	    if $notfirst
	    then
		$self -node $node $components $options \
		 > /tmp/$self_$node.$$  2>&1 &
		STOPPIDS="${STOPPIDS} ${node}.$!"
	    fi
	    notfirst=true
	done
    }
}

#
#	cluster_opt_check - check if cluster only option enabled.
#
cluster_opt_check()
{
    $CLUSTERSUPPORT ||
    {
	wrap_lines 76 << ! | wmsg

$self: $1 parameter is only supported in a clustered configuration.

!
	exit 1
    }
}
#
#   sanity check to ensure rsh is setup correctly.
#
check_rsh_configured()
{
    if [ "X$rsh_method" = "X" -o "$rsh_method" = "none" ]; then
	wrap_lines 76 << ! | wmsg

$self: You cannot use the "$1" parameter unless you've configured (PROD2NAME) to use a remote shell.

!
        exit 1
    fi
}
#
#	append_node - add node to list if not a duplicate.
#
#	$1	- node name
#	$2	- value node was derived from.
#	$3	- value type.
#
append_node()
{
    [ -z "$1" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: $3 "$2" is not configured in the cluster.

!
	exit 1
    }
    if in_set "$targets" "$1" 
    then
	# duplicate, don't want it
	:
    else
	targets="$targets $1"
    fi
    return 0
}

#
#	add_host - Add all nodes for a host to target set
#
add_host()
{
    in_set "$target_hosts" $1 ||
    {
	# Add all vnodes this host to target_sets
	target_hosts="$target_hosts $1"
	target_sets="$target_sets *"
    }
    return 0
}

#
#	add_node - Evaluate argument as a potential target node.
#
add_node()
{
    [ -z "$1" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: $badarg argument value missing.

!
	exit 1
    }
    node="`(PROG1PRFX)pmhost -node=$1`"
    [ -z "$node" ] && 
    {
	in_set "$host_names" $1 &&
	{
	    add_host $1
	    return 0
	}
    }
    append_node "$node" "$1" "node name"
    return 0
}

#
#	add_rad - Evaluate argument as a target RAD spec.
#
add_rad()
{
    cluster_opt_check $badarg

    [ -z "$1" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: $badarg argument value missing.

!
	exit 1
    }

    [ -n "`echo $1 | sed 's/[0-9]*//'`" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: bad syntax for RAD value ($1).

!
	exit 1
    }
    append_node "`(PROG1PRFX)pmhost -rad=$1`" "$1" "RAD"
    return 0
}
help_menu()
{ 
    wrap_lines 76 << ! | wmsg

Usage: (PROG2PRFX)stop [ -timeout=minutes ] [ -force | -immediate ] [ -kill ]
!
    wrap_lines 76 << ! | wmsg 
 [ -(PROG1PRFX)gcn ] [ -dmfrcp ] [ - dmfacp ] [ -rmcmd ] [ -(PROG1PRFX)dbms ]
!
    wrap_lines 76 << ! | wmsg 
 [ -(PROG1PRFX)star ] [ -(PROG1PRFX)gcc ] [ -(PROG1PRFX)gcd ] [ -(PROG1PRFX)gcb ] [ -oracle ] [ -sybase]
!
    wrap_lines 76 << ! | wmsg
 [ -informix] [ -db2udb ] 
!

    $CLUSTERSUPPORT && 
    {
        if [ "X$rsh_method" != "X" -a "$rsh_method" != "none" ]; then
	 
	    wrap_lines 76 << ! | wmsg
 [ -c[luster] ] [ [ [-n[ode]] node_name | nick_name ] | -r[ad] radid ... ] 
!
        fi
    } 
    wmsg ""
}
#
#	MAIN
#

#
# Start initialization
#
rm -f /tmp/*.$$ 2>/dev/null
FORCE=false
KILL=false
IMMEDIATE=false
SHOWONLY=false
TRY=false
STOPPIDS=""
need_dbms_cfg=
self=`basename $0`
lfile="$(PROG3PRFX)_LOG/(PROG2PRFX)start.log"
hostname=`(PROG1PRFX)pmhost`
if [ x"${conf_LSB_BUILD}" = x"TRUE" ] ; then
    config_dat="$(PROG3PRFX)_ADMIN/config.dat"
else
    config_dat="$(PROG3PRFX)_CONFIG/config.dat"
fi

#
# See if we have to worry about cluster stuff.
#
if $CLUSTERSUPPORT
then
    # Only enable cluster options if supportable and configured.
    [ "1" = "`ingprenv II_CLUSTER`" ] || CLUSTERSUPPORT=false
fi

if $CLUSTERSUPPORT
then
    #
    # Allow -cluster, -node, -rad & -local options.  Get data
    # needed to validate input for cluster options.  Redirect log.
    #
    lfile="${lfile}_`echo $hostname | tr '[a-z]' '[A-Z]'`"

    host_names=""
    numa_nodes="`grep -i \
	'^(PROG1PRFX)\.[^.]*\.config.numa.rad:[ 	]*[1-9][0-9]*$' \
	$config_dat | awk -F. '{ printf \" %s\",\$2 }'`"
    if [ -n "$numa_nodes" ]
    then
	# We have at least one NUMA configured node.
	# Collect Host names for these puppies.
	for node in $numa_nodes
	do
	    host=`(PROG1PRFX)getres "(PROG1PRFX).$node.config.host"`
	    [ -z "$host" ] &&
	    {
		error_box 76 << ! | wmsg
Configuration shows node "$node" configured as a NUMA virtual node, but no host name for this node was found.
!
		exit 1
	    }
	    host_names="$host_names $host"
	done

	# Don't allow Implicit RAD context.
	II_DEFAULT_RAD=
	export II_DEFAULT_RAD
    fi
fi

minutes=0
case "$VERS" in

      usl_us5|axp_osf|hp8_us5|hpb_us5|sui_us5|sqs_ptx|rs4_us5|r64_us5|sgi_us5|\
      *_lnx|*_osx|int_rpl)
	 seconds=10;
         ;;

      *)
	 seconds=0;
         ;;

esac

##
##  Allow user to specify which object to shutdown; no real attempt is
##  made to handle interdependencies between objects (such as the dbms
##  and dmfrcp).  The rules that are employed go as follows:
##
##  dmfrcp -> dmfacp -> iidbms,iistar,icesvr,rmcmd (cscleanup/ipcclean)
##
##  Objects to date (2.5) 	var_suffix	identify
##  ---------------------	----------	--------
##	iigcn			gcn		(PROG1PRFX)gcfid
##	dmfrcp			rcp		ps
##	dmfacp			acp		ps
##	iidbms[=<idlist>]	dbms		ps,csreport
##	iistar[=<idlist>]	star		(PROG1PRFX)gcfid
##	icesvr[=<idlist>]	ice		(PROG1PRFX)gcfid
##	rmcmd			rmcmd		ps
##	iigcc[=<idlist>]	gcc		(PROG1PRFX)gcfid
##	iigcd[=<idlist>]	gcd		(PROG1PRFX)gcfid
##	iigcb[=<idlist>]	gcb		(PROG1PRFX)gcfid
##	oracle[=<idlist>]	ora		(PROG1PRFX)gcfid
##	sybase[=<idlist>]	syb		(PROG1PRFX)gcfid
##	informix[=<idlist>]	inf		(PROG1PRFX)gcfid
##	db2udb[=<idlist>]	udb		(PROG1PRFX)gcfid
##
##
##  Objects to date (2.65)      var_suffix      identify
##  ---------------------       ----------      --------
##      iigcd[=<idlist>]        gcd             (PROG1PRFX)gcfid

shutids_gcn=""
shutids_rcp=""
shutids_acp=""
shutids_rmcmd=""
shutids_ice=""
shutids_dbms=""
shutids_gcc=""
shutids_gcb=""
shutids_gcd=""
shutids_star=""
shutids_ora=""
shutids_syb=""
shutids_inf=""
shutids_udb=""

# Set option & target sets to empty
options=""
targets=""
nontargets=""
target_hosts=""
target_sets=""
components=""
debugging=""
rsh_method=`(PROG1PRFX)getres "(PROG1PRFX).$hostname.config.rsh_method"`

while [ -n "$1" ]
do
    badarg="$1"
    case "$1" in

	-try)
	    # defeats the ingstop pref in config.dat
	    TRY=true;
	    ;;

	-force|-f)
	    $FORCE && usage1
	    FORCE=true;
	    IMMEDIATE=false;
	    options="$options $1"
	    ;;

	-immediate|-i)
	    $FORCE && usage1
	    FORCE=true;
	    IMMEDIATE=true;
	    options="$options $1"
	    ;;

	-kill|-k)
	    KILL=true;
	    options="$options $1"
	    ;;

        -show)
            SHOWONLY=true;
            ;;

	-cluster|-c)
	    # Act against all nodes
	    cluster_opt_check $1
	    check_rsh_configured $1
	    targets="`grep -i \
	'^(PROG1PRFX)\.[^.]*\.config.cluster.id:[ 	]*[1-9][0-9]*$' \
	$config_dat | awk -F. '{ printf \" %s\",\$2 }'`"
	    ;;

	-local)
	    cluster_opt_check $1
	    add_host $hostname
	    ;;

	-node|-n)
	    check_rsh_configured $1
	    shift
	    add_node "$1"
	    ;;

	-node=*|-n=*)
	    add_node "`echo $1 | sed 's/-n[ode]*=\(.*\)/\1/'`"
	    ;;

	[A-Za-z][A-Za-z_0-9]*)
	    # Node name
	    add_node "$1"
	    ;;

	-rad|-r)
	    shift
	    add_rad "$1"
	    ;;

	-rad=*|-r=*)
	    add_rad "`echo $1 | sed 's/-r[ad]*=\(.*\)/\1/'`"
	    ;;

	-timeout=[0-9]*|-t=[0-9]*)
	    options="$options $1"
	    minutes=`echo $1 | awk -F= ' { print \$2 } '`
	    seconds=`expr 60 \* $minutes`
	    ;;

	-(PROG1PRFX)gcn|-dmfrcp|-dmfacp|-rmcmd)
	    components="$components $1"
	    partial=true
	    case $1 in
		-(PROG1PRFX)gcn)		shutids_gcn="all" ;;
		-dmfrcp)	shutids_rcp="all" ;;
		-dmfacp)	shutids_acp="all" ;;
		-rmcmd)		shutids_rmcmd="all" ;;
	    esac
	    ;;

	-icesvr*|-(PROG1PRFX)dbms*|-(PROG1PRFX)gcc*|-(PROG1PRFX)gcd*|-(PROG1PRFX)gcb*|-(PROG1PRFX)star*|\
	-oracle*|-sybase*|-informix*|-db2udb*)
	    components="$components $1"
	    partial=true
	    case $1 in
		-(PROG1PRFX)dbms*)	sufx="dbms"; need_dbms_cfg=y ;;
		-(PROG1PRFX)star*)	sufx="star" ;;
		-icesvr*)	sufx="ice" ;;
		-(PROG1PRFX)gcc*)	sufx="gcc" ;;
                -(PROG1PRFX)gcd*)       sufx="gcd" ;;
		-(PROG1PRFX)gcb*)	sufx="gcb" ;;
		-oracle*)	sufx="ora" ;;
		-sybase*)	sufx="syb" ;;
		-informix*)	sufx="inf" ;;
		-db2udb*)	sufx="udb" ;;
	    esac

	    if [ `expr "$1" : ".*=."` -gt 0 ] &&
	       [ `expr "$1" : ".*[=,]all"` -eq 0 ] ; then
		oids=`echo "$1" | sed 's/^[^=]*=//' | tr ',' ' '`
		eval "shutids_${sufx}=\"\$shutids_${sufx} $oids\""
	    else
		eval "shutids_${sufx}='all'"
	    fi
	    ;;
	-debug)
	    options="$options $1"
	    debugging="Y"
	    set -x
	    ;;
	-help)
	    help_menu
	    exit 1
	    ;;
	*)
	    help_menu
	    exit 1
	    ;;
    esac
    shift
done

ingstop_opt=`iigetres ii.$hostname.prefs.ingstop`
case "$ingstop_opt" in
        -force|-f)
            if [ "$FORCE" != "true" -a "$TRY" != "true" ]
            then
            if [ "$KILL" != "true" ]
            then
                    FORCE=true
                    IMMEDIATE=false
                    options="$options $ingstop_opt"
            fi
            fi
            ;;

        -kill|-k)
            if [ "$FORCE" != "true"  -a "$KILL" != "true" ]
            then
            if [ "$IMMEDIATE" != "true" -a "$TRY" != "true" ]
            then
                    KILL=true
		    FORCE=true
                    options="$options $ingstop_opt"
            fi
            fi
                ;;
esac

if [ -z "$targets" ]
then
    # Normal case.  No nodes were specified, just do a full stop on local host. 
    wrap_lines 76 << ! | wmsg

(PROD1NAME)/(PROG2PRFX)stop
!
    foreground="$hostname"
    target_hosts="$hostname"
    target_sets="."
fi

if [ -n "$targets" -o -z "`(PROG1PRFX)pmhost -node $hostname`" ] 
then
    #
    # Cluster case, and one or more target nodes was listed,
    # or local node is a NUMA node.
    #
    # Group target nodes into sets by host
    #
    targn=0
    for node in $targets
    do
	targn=`expr $targn + 1`
	if in_set "$numa_nodes" $node
	then
	    #
	    # For NUMA nodes, see if all virtual nodes for a host
	    # have been specified.
	    #
	    nodehost=`nth_element "$host_names" \
	      \`element_index "$numa_nodes" $node\``
	    in_set "$target_hosts" $nodehost && continue

	    hostn=0
	    nodeset=".${node}"
	    allhits=true 
	    for host in $host_names
	    do
		hostn=`expr $hostn + 1`
		if [ "$host" = "$nodehost" ]
		then
		    numanode=`nth_element "$numa_nodes" $hostn`
		    [ "$numanode" = "$node" ] && continue
		    in_set "$targets" $numanode &&
		    {
			nodeset="${nodeset}.${numanode}"
			continue;
		    }
		    allhits=false
		fi
	    done
	    
	    target_hosts="$target_hosts $nodehost"
	    if $allhits
	    then
		target_sets="$target_sets ."
	    else
		target_sets="$target_sets $nodeset"
	    fi
	else
	    #
	    # Regular cluster node.  host = node in this case
	    #
	    in_set "$target_hosts" $node && continue
	    target_hosts="$target_hosts $node"
	    target_sets="$target_sets ."
	fi
    done

    #
    # If current host is in the set of target_hosts, it is the foreground
    #
    foreground=""
    in_set "$target_hosts" "$hostname" && foreground="$hostname"
    if [ -z "$foreground" ]
    then
	for host in $target_hosts
	do
	    foreground="$host"
	    break
	done
    fi

    #
    # Launch any background processing
    #
    targets=""		# 'targets' being reused as set of local NUMA nodes.
    hostn=0
    for host in $target_hosts
    do
	hostn=`expr $hostn + 1`

	# Retreive set of nodes for this host
	nodeset=`nth_element "$target_sets" $hostn`
	if [ "$nodeset" = "." ]
	then
	    nodeset=""
	else
	    nodeset="`echo \"$nodeset\" | sed 's/\./-n /g'`"
	fi

	[ "$host" = "$hostname" ] &&
	{
	    targets="`echo $nodeset | sed 's/\./ /'`"
	    continue
	}

	# Resolve machine name for remote host
	machid=`(PROG1PRFX)getres \
	       "(PROG1PRFX).${host}.gcn.local_vnode"`
	[ -z "$machid" ] && 
	{
	    warning_box 76 << ! | wmsg
$self: Cannot resolve network address for host "${host}"
!
	    continue
	}
	if [ "$host" = "$foreground" ]
	then
	  $rsh_method "$machid" $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)rsh \
	     $(PRODLOC) $self $nodeset $components $options
	else
	  $rsh_method "$machid" $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)rsh \
	     $(PRODLOC) $self $nodeset $components $options \
	     > /tmp/$self_$host.$$  2>&1 &
	  STOPPIDS="${STOPPIDS} ${host}.$!"
	fi
    done

    if [ "$foreground" = "$hostname" ]
    then
	if in_set "$host_names" "$hostname"
	then
	    # Flesh out set of local target nodes, and
	    # create a set of spared local nodes
	    
	    hostn=0
	    for host in $host_names
	    do
		hostn=`expr $hostn + 1`
		if [ "$host" = "$hostname" ]
		then
		    node=`nth_element "$numa_nodes" $hostn`
		    in_set "$targets" $node || nontargets="$nontargets $node"
		fi
	    done
	    if [ -z "$targets" ]
	    then
		targets="$nontargets"
		nontargets=""
	    fi

	    numa_parts="-dmfrcp -dmfacp -(PROG1PRFX)dbms -(PROG1PRFX)star -icesvr"
	    if [ -z "$components" ]
	    then
		components="$numa_parts"
	    else
		oldcomp="$components"
		components=""
		for part in $oldcomp
		do
		    in_set "$numa_parts" $part && \
		     components="$components $part"
		done
	    fi

	    for node in $targets
	    do
		# Use 1st NUMA node for operations on common processes.
		rad=`(PROG1PRFX)getres "(PROG1PRFX).$node.config.numa.rad"`
		[ -z "$rad" ] &&
		{
		    fatal_box 76 << ! | wmsg
Configuration error.  Connot find RAD id for NUMA node "$node".
!
		    exit 1
		}
		II_DEFAULT_RAD="$rad"
		export II_DEFAULT_RAD

		# Process other nodes (if any) later.
		break
	    done
	fi
    fi
    node=`(PROG1PRFX)pmhost -local`
    wrap_lines 76 << ! | wmsg

Executing (PROG2PRFX)stop against $node.
!
    if [ -n "$STOPPIDS" ]
    then
	wrap_lines 76 << ! | wmsg

Processing for the other node(s) specified will go on in the background, \
with the results reported when foreground processing is complete.
!
    fi
fi

[ "$foreground" = "$hostname" ] ||
{
    #
    # No local operations, just wait & report results
    #
    report_remote_results

    [ -z "$debugging" ] && rm -f /tmp/*.$$ 2>/dev/null
    exit 0
}

#
# From this point on, all operations are against one node,
# with the NUMA context if any set by II_DEFAULT_RAD.
#
full_shutdown=0
if [ -z "$shutids_gcn$shutids_rcp$shutids_acp$shutids_rmcmd" ] &&
   [ -z "$shutids_dbms$shutids_star$shutids_ice$shutids_gcc" ] &&
   [ -z "$shutids_gcb$shutids_ora$shutids_syb" ] &&
   [ -z "$shutids_gcd$shutids_inf$shutids_udb" ]
then
    if [ -z "$nontargets" ]
    then
	full_shutdown=1
	shutids_gcn="all"
	shutids_rmcmd="all"
	shutids_gcc="all"
	shutids_gcd="all"
	shutids_gcb="all"
	shutids_ora="all"
	shutids_syb="all"
	shutids_inf="all"
	shutids_udb="all"
    fi
    shutids_rcp="all"
    shutids_acp="all"
    shutids_dbms="all"
    shutids_star="all"
    shutids_ice="all"
fi

if [ "$shutids_rcp" = "all" ] ; then
    shutids_rcp="all"
    shutids_acp="all"
    shutids_dbms="all"
    shutids_star="all"
    shutids_ice="all"
    [ -n "$nontargets" ] || shutids_rmcmd="all"
fi

if [ $minutes -eq 1 ] ; then
   minute_or_minutes="minute"
else
   minute_or_minutes="minutes"
fi

# set trap on exit to clean up temporary files
trap "cleanup ; exit 1" 1 2 3 15

if grep -i "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
   server_host=true
else
   server_host=false
fi    

( $FORCE || $KILL ) && 
[ -n "$shutids_dbms" -o -n "$shutids_star" ] &&
{
    if [ "$shutids_dbms" = "all" ] ; then
	shutscope="this installation"
    else
	shutscope="the selected DBMS's"
    fi

    warning_box 76 1 -1 << ! | wmsg
All connections held in $shutscope will be terminated in $minutes $minute_or_minutes!
!

   $server_host &&
   { 
      if $IMMEDIATE ; then
         box 76 -1 -1 << ! | wmsg

All uncommitted transactions will be rolled back the next time (PROD1NAME) starts up.
!
      else
	 ifpos=""
	 if [ "$KILL" = 'true' ] ; then
	    ifpos=', if possible,'
	 fi
         box 76 -1 -1 << ! | wmsg

All uncommitted transactions will be rolled back$ifpos before shutdown is complete.
!
      fi
      box 76 -1 -1 << ! | wmsg

NOTE: this operation may leave database files on disk in $shutscope in a physically inconsistent state until Ingres is restarted, and recovery from the transaction log is completed.
!
   }
   echo "" | box 76 -1 | wmsg
}

(PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`

icesvr_count=0 # number of ice server processes
dbms_count=0   # number of dbms server processes
rcp_count=0    # number of recovery processes
acp_count=0    # number of archiver processes
star_count=0   # number of star server processes
gcc_count=0    # number of comm server processes
gcd_count=0    # number of DAS server processes
gcb_count=0    # number of bridge server processes
gcn_count=0    # number of name server processes
ora_count=0    # number of oracle gateway server processes
syb_count=0    # number of sybase gateway server processes
inf_count=0    # number of informix gateway server processes
udb_count=0    # number of db2udb gateway server processes
rmcmd_count=0  # number of rmcmd server processes
shut_proc=0    # number of processes that are actually shutdown

# Lists of server IDs, init to none
ice_ids=""
dbms_ids=""
star_ids=""
gcc_ids=""
gcd_ids=""
gcb_ids=""
ora_ids=""
syb_ids=""
inf_ids=""
udb_ids=""
acp_pids=""
rcp_pids=""
dbms_pids=""

wrap_lines 76 << ! | wmsg

Checking for active (PROD1NAME) processes in installation $(PROG3PRFX)_INSTALLATION ...

!

$PSCMD > /tmp/ps.$$

WSRE='[	 ][	]*'
SEDTRIM="s/^$WSRE//;s/$WSRE$//"
IIGCFID_FILTER="sed -e 's/ *(.*)//' -e '$SEDTRIM' | sort -u 2> /dev/null | tr '\012' ' '"

if [ -n "$shutids_gcn" ] ; then

    # get name server status, report count
    #
    gcn_count=`eval "(PROG1PRFX)gcfid (PROG1PRFX)gcn | $IIGCFID_FILTER" | wc -w | tr -d ' '`
    report_nicely $gcn_count 1 1 "name server ((PROG1PRFX)gcn)"
fi

cat /dev/null >/tmp/csreport.$$
cat /dev/null >/tmp/psinst.$$

if [ -n "$shutids_rcp" -o -n "$shutids_dbms" -o -n "$shutids_ice" ] ||
   [ -n "$shutids_acp" -o -n "$shutids_star" -o -n "$shutids_rmcmd" ] ; then

    # we need a name server to shut down cleanly so
    # so start one if we need to
    [ $SHOWONLY = false ] && [ $gcn_count = 0 ] && [ "$shutids_gcn" ] &&
    {
	wmsg "Active name server is required for clean shutdown..."
        ingstart -iigcn
	gcn_count=1
    }

    # get status of Ingres processes from csreport
    ## Send to file first to get exec-with-timeout status, not tr status!
    ## Return status > 128 implies spin, csreport killed.
    exec_with_timeout 20 "(PROG0PRFX)csreport" >/tmp/xcsreport.$$
    if [ $? -gt 128 ] ; then
	## FIXME here, and in similar places, if we are force & kill we could
	## ps for ii and dmf processes with the right installation ID, kill
	## them all, ipcrm the system segment (by magic), and ipcclean.
	## Or, run some new utility to force the spin-lock off, because
	## it's probably stuck, and then ipcclean.
	fatal_box 76 << ! | wmsg
Unable to issue csreport command.  The installation will have to be stopped manually.
!
	cleanup
	exit 2
    fi
    grep 'inuse 1' /tmp/xcsreport.$$ | tr ',' ' ' | tr -s ' ' >/tmp/csreport.$$
    rm -f /tmp/xcsreport.$$
    for pid in `awk '{ print $4 }' </tmp/csreport.$$`
    do
       grep $pid /tmp/ps.$$ >> /tmp/psinst.$$
    done
fi

if [ -n "$shutids_rcp" ] ; then

    # get status of recovery server, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
	recovery_installed=1
    else
	recovery_installed=0
    fi
    rcp_pids=`grep recovery /tmp/psinst.$$ | awk '{print $2}' | tr '\012' ' '`
    rcp_count=`echo $rcp_pids | wc -w | tr -d ' '`
    report_nicely $rcp_count 1 $recovery_installed "recovery server (dmfrcp)"
fi

if [ -n "$shutids_ice" ] ; then

    # get list of ICE servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.icesvr" $config_dat >/dev/null ; then
	icesvr_installed=1
    else
	icesvr_installed=0
    fi
    ois="ICE server (icesvr)"
    icesvr_ids=`eval "(PROG1PRFX)gcfid icesvr | $IIGCFID_FILTER"`
    icesvr_count=`echo $icesvr_ids | wc -w | tr -d ' '`
    report_nicely $icesvr_count 100 $icesvr_installed "$ois"

    if [ $icesvr_count -ne 0 ] ; then
	scan_oid icesvr_ids icesvr_count "$ois" $shutids_ice
    fi
fi

if [ -n "$shutids_dbms" ] ; then

    # get list of DBMS servers, report count
    #
    cat /dev/null >/tmp/(PROG1PRFX)dbms.$$
    if [ -n "$need_dbms_cfg" ] ; then
	cat /dev/null >/tmp/dbms_cfg.$$
	cat /dev/null >/tmp/dbms_pid_cfg.$$
	cfg_field=-1
    fi
    grep (PROG1PRFX)dbms /tmp/psinst.$$ | tr -s ' ' | grep -v "/(PROG1PRFX)dbms recovery" | \
	grep -v "/(PROG1PRFX)star star" | while read line
    do
	pid=`echo $line | awk '{ print $2 }'`
	grep "pid $pid " /tmp/csreport.$$ | awk '{ print $7 }' >> /tmp/(PROG1PRFX)dbms.$$
	if [ -n "$need_dbms_cfg" ] ; then
	    # Also gather up all configuration-id's from the command line.
	    if [ $cfg_field -lt 0 ] ; then
		# configuration-id is the one after the word "dbms".
		cfg_field=1
		val=
		while [ "$val" != 'dbms' -a $cfg_field -lt 200 ]
		do
		    val=`echo $line | cut -f$cfg_field -d' '`
		    cfg_field=`expr $cfg_field + 1`
		done
		if [ $cfg_field -ge 200 ] ; then
		    # can't figure it out, give up on this
		    need_dbms_cfg=
		    rm -f /tmp/dbms_cfg.$$
		fi
	    fi
	    cfg_id=`echo $line | cut -f$cfg_field -d' '`
	    echo $pid $cfg_id >>/tmp/dbms_pid_cfg.$$
	fi
    done

    # sort list of DBMS servers 
    #
    dbms_ids=`sort /tmp/(PROG1PRFX)dbms.$$ | sed "$SEDTRIM" | tr '\012' ' '`

    if grep "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
	dbms_installed=1
    else
	dbms_installed=0
    fi
    ois="DBMS server ((PROG1PRFX)dbms)"
    dbms_count=`echo $dbms_ids | wc -w | tr -d ' '`
    report_nicely $dbms_count 100 $dbms_installed "$ois"

    if [ $dbms_count -ne 0 ] ; then
	scan_oid dbms_ids dbms_count "$ois" $shutids_dbms
	if [ $dbms_count -ne 0 ] ; then
	    for server_id in $dbms_ids ; do
		pid=`grep "connect id $server_id" /tmp/csreport.$$ | awk '{ print $4 }'`
		dbms_pids="$dbms_pids $pid"
		if [ -n "$need_dbms_cfg" ] ; then
		    grep "^$pid " /tmp/dbms_pid_cfg.$$ | awk '{print $2}' >>/tmp/dbms_cfg.$$
		fi
	    done
	fi
    fi
    rm -f /tmp/(PROG1PRFX)dbms.$$ /tmp/dbms_pid_cfg.$$
fi

if [ -n "$shutids_star" ] ; then

    # get list of Star servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*star" $config_dat >/dev/null ; then
	star_installed=1
    else
	star_installed=0
    fi
    ois="Star server ((PROG1PRFX)star)"
    star_ids=`eval "(PROG1PRFX)gcfid (PROG1PRFX)star | $IIGCFID_FILTER"`
    star_count=`echo $star_ids | wc -w | tr -d ' '`
    report_nicely $star_count 100 $star_installed "$ois"

    if [ $star_count -ne 0 ] ; then
	scan_oid star_ids star_count "$ois" $shutids_star
    fi
fi

if [ -n "$shutids_gcc" ] ; then

    # get list of Net servers, report count
    #
    ois="Net server ((PROG1PRFX)gcc)"
    gcc_ids=`eval "(PROG1PRFX)gcfid (PROG1PRFX)gcc | $IIGCFID_FILTER"`
    gcc_count=`echo $gcc_ids | wc -w | tr -d ' '`
    report_nicely $gcc_count 100 1 "$ois"

    if [ $gcc_count -ne 0 ] ; then
	scan_oid gcc_ids gcc_count "$ois" $shutids_gcc
    fi
fi

if [ -n "$shutids_gcd" ] ; then

    # get list of DAS servers, report count
    #
    ois="Data Access server ((PROG1PRFX)gcd)"
    gcd_ids=`eval "(PROG1PRFX)gcfid (PROG1PRFX)gcd | $IIGCFID_FILTER"`
    gcd_count=`echo $gcd_ids | wc -w | tr -d ' '`
    report_nicely $gcd_count 100 1 "$ois"

    if [ $gcd_count -ne 0 ] ; then
        scan_oid gcd_ids gcd_count "$ois" $shutids_gcd
    fi
fi

if [ -n "$shutids_gcb" ] ; then

    # get list of Bridge servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*gcb" $config_dat >/dev/null ; then
	gcb_installed=1
    else
	gcb_installed=0
    fi
    ois="Bridge server ((PROG1PRFX)gcb)"
    gcb_ids=`eval "(PROG1PRFX)gcfid (PROG1PRFX)gcb | $IIGCFID_FILTER"`
    gcb_count=`echo $gcb_ids | wc -w | tr -d ' '`
    report_nicely $gcb_count 100 $gcb_installed "$ois"

    if [ $gcb_count -ne 0 ] ; then
	scan_oid gcb_ids gcb_count "$ois" $shutids_gcb
    fi
fi

if [ -n "$shutids_rmcmd" ] ; then

    # get status of RMCMD process, report count
    #
    if grep "$hostname.(PROG2PRFX)start.*.rmcmd" $config_dat >/dev/null ; then
	rmcmd_installed=1
    else
	rmcmd_installed=0
    fi
    grep "rmcmd $(PROG3PRFX)_INSTALLATION" /tmp/ps.$$ > /tmp/rmcmd.$$
    rmcmd_count=`wc -l < /tmp/rmcmd.$$ | tr -d ' '`
    report_nicely $rmcmd_count 1 $rmcmd_installed "remote command process (rmcmd)"
fi

if [ -n "$shutids_acp" ] ; then

    # get status of archiver process, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
	archiver_installed=1
    else
	archiver_installed=0
    fi
    acp_pids=`grep dmfacp /tmp/psinst.$$ | awk '{print $2}' | tr '\012' ' '`
    acp_count=`echo $acp_pids | wc -w | tr -d ' '`
    report_nicely $acp_count 1 $archiver_installed "archiver process (dmfacp)"
fi

if [ -n "$shutids_ora" ] ; then

    # get list of Oracle Gateway servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*oracle" $config_dat >/dev/null ; then
       oracle_installed=1
    else
       oracle_installed=0
    fi
    ois="Oracle Gateway server"
    ora_ids=`eval "(PROG1PRFX)gcfid oracle | $IIGCFID_FILTER"`
    ora_count=`echo $ora_ids | wc -w | tr -d ' '`
    report_nicely $ora_count 100 $oracle_installed "$ois"

    if [ $ora_count -ne 0 ] ; then
	scan_oid ora_ids ora_count "$ois" $shutids_ora
    fi
fi

if [ -n "$shutids_syb" ] ; then

    # get list of Sybase Gateway servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*sybase" $config_dat >/dev/null ; then
       sybase_installed=1
    else
       sybase_installed=0
    fi
    ois="Sybase Gateway server"
    syb_ids=`eval "(PROG1PRFX)gcfid sybase | $IIGCFID_FILTER"`
    syb_count=`echo $syb_ids | wc -w | tr -d ' '`
    report_nicely $syb_count 100 $sybase_installed "$ois"

    if [ $syb_count -ne 0 ] ; then
	scan_oid syb_ids syb_count "$ois" $shutids_syb
    fi
fi

if [ -n "$shutids_inf" ] ; then

    # get list of Informix Gateway servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*informix" $config_dat >/dev/null ; then
       informix_installed=1
    else
       informix_installed=0
    fi
    ois="Informix Gateway server"
    inf_ids=`eval "(PROG1PRFX)gcfid informix | $IIGCFID_FILTER"`
    inf_count=`echo $inf_ids | wc -w | tr -d ' '`
    report_nicely $inf_count 100 $informix_installed "$ois"

    if [ $inf_count -ne 0 ] ; then
	scan_oid inf_ids inf_count "$ois" $shutids_inf
    fi
fi

if [ -n "$shutids_udb" ] ; then

    # get list of DB2 UDB Gateway servers, report count
    #
    if grep "$pmhost.*(PROG2PRFX)start.*db2udb" $config_dat >/dev/null ; then
       db2udb_installed=1
    else
       db2udb_installed=0
    fi
    ois="DB2 UDB Gateway server"
    udb_ids=`eval "(PROG1PRFX)gcfid db2udb | $IIGCFID_FILTER"`
    udb_count=`echo $udb_ids | wc -w | tr -d ' '`
    report_nicely $udb_count 100 $db2udb_installed "$ois"

    if [ $udb_count -ne 0 ] ; then
	scan_oid udb_ids udb_count "$ois" $shutids_udb
    fi
fi

## Clean files no longer needed now, so less chance of them being left around
## We still need csreport output.
rm -f /tmp/rmcmd.$$ /tmp/ps.$$ /tmp/psinst.$$

# exit if no servers running
[     $dbms_count -ge 1 -o $star_count -ge 1 -o $icesvr_count -ge 1 \
   -o $gcc_count  -ge 1 -o $gcd_count -ge 1 -o $gcb_count  -ge 1 \
   -o $gcn_count  -ge 1 \
   -o $rcp_count  -ge 1 -o $acp_count  -ge 1 -o $rmcmd_count -ge 1 \
   -o $ora_count  -ge 1 -o $syb_count  -ge 1 -o $inf_count   -ge 1 \
   -o $udb_count  -ge 1 ] ||
{
   if $server_host && [ -n "$shutids_rcp" ]
   then
	cscleanup >/dev/null 2>&1
	sleep 1
	ipcclean >/dev/null 2>&1
   fi
   if [ "$full_shutdown" -ne 0 ] ; then
	shutscope="in"
   else
	shutscope="matching your request in"
   fi
   wrap_lines 76 << ! | wmsg

There are no active (PROD1NAME) processes $shutscope installation $(PROG3PRFX)_INSTALLATION.

!

    [ -n "$targets" ] && exec_locals

    if [ -n "$STOPPIDS" ]
    then
        cat << !
Processing of ingstop on remote nodes will continue
!
	report_remote_results
    fi
    [ -z "$debugging" ] && rm -f /tmp/*.$$ 2>/dev/null
    exit 0
}

#  Use of the -show flag will always prevent shutdown even if combined with 
#  other operation flags in the command tail, so exit now. 
if $SHOWONLY ; then
    wmsg "" 
    cleanup
    exit 0
fi

CACHE=`iigetres ii.$CONFIG_HOST.dbms.*.cache_sharing`
SOLE=`iigetres ii.$CONFIG_HOST.dbms.*.sole_server`
#
#  Issue the stop command to rmcmd and wait
#  rmcmdstp will only work consistently when cache_sharing is ON or if
#  there is only one dbms server. For other cases the extra dbms servers
#  need to be shutdown first.  Go ahead and give it a whirl anyway.
## (The sole-server thing is understandable, but it's unclear why multiple
## private caches should defeat rmcmdstp as long as sole-server is OFF...???)
#
[ $rmcmd_count -ge 1 ] &&
{
    wmsg ""
    wmsg "Shutting down the remote command server..."

    exec_with_timeout 10 rmcmdstp
    sleep 2
    rmcmd_count=`$PSCMD | grep "rmcmd $(PROG3PRFX)_INSTALLATION" | wc -l | tr -d ' '`
#
#  Some times the messages get lost and the remote server does not shut itself
#  down when there are commands issued simultaneously on all nodes. Reissue the
#  command. Ideally this should be fixed in rmcmd server.
## The remote command server shutdown mechanism really needs to be
## reworked so that the stopper can work properly.  Possibly have
## rmcmdstp forego the queue entry, and simply blast out dbevents
## until the rmcmd goes away (as seen by e.g. a keepalive dbevent,
## or a name server query, or something).
## Don't whack rmcmdstp again if there are likely to be sole-server issues...
## we'll take care of it later on.
#
    if [ $rmcmd_count -ge 1 -a \( $dbms_count -eq 1 -o "$SOLE" = 'OFF' \) ] ; then
	exec_with_timeout 10 rmcmdstp
	sleep 2
	rmcmd_count=`$PSCMD | grep "rmcmd $(PROG3PRFX)_INSTALLATION" | wc -l | tr -d ' '`
    fi
}

# Start closing network stuff before DBMS servers.
## The message distinction is historical, we do the same thing
## (quiesce) regardless of force or not.

#
# disallow new connections to Net server(s)
#
[ $gcc_count -ge 1 ] &&
{
   for server_id in  $gcc_ids ; do

      wmsg ""
      if $FORCE || [ $minutes -eq 0 ] ; then
         wmsg "Shutting down Net server $server_id ..."
      else
         wmsg "Closing Net server $server_id to new connections ..."
      fi

      (PROG1PRFX)gcstop -q $server_id >/dev/null ||
      {
         fatal_box 76 << ! | wmsg
Unable to issue shutdown command.
!
	cleanup
     	exit 1

      }
      done
}


#
# disallow new connections to DAS server(s)
#
[ $gcd_count -ge 1 ] &&
{
   for server_id in  $gcd_ids ; do

      wmsg ""
      if $FORCE || [ $minutes -eq 0 ] ; then
         wmsg "Shutting down Data Access Server $server_id ..."
      else
         wmsg "Closing Data Access Server $server_id to new connections ..."
      fi

      (PROG1PRFX)gcstop -q $server_id >/dev/null ||
      {
         fatal_box 76 << ! | wmsg
Unable to issue shutdown command.
!
	cleanup
        exit 1

      }
      done
}

#
# disallow new connections to Bridge server(s)
#
[ $gcb_count -ge 1 ] &&
{
   for server_id in  $gcb_ids ; do

      wmsg ""
      if $FORCE || [ $minutes -eq 0 ] ; then
         wmsg "Shutting down Bridge server $server_id ..."
      else
         wmsg "Closing Bridge server $server_id to new connections ..."
      fi

      (PROG1PRFX)gcstop -q $server_id >/dev/null ||
      {
         fatal_box 76 << ! | wmsg
Unable to issue shutdown command.
!
	cleanup
     	exit 1

      }
   done
}

#
#  Issue "set server shut" command to DBMS and Star servers
#
[ $icesvr_count -ge 1 -o $dbms_count -ge 1 -o $star_count -ge 1 ] &&
{
   #
   # disallow new connections to ICE, DBMS and Star server(s)
   #
   # Ensure ICE is closed first
   for server_type in icesvr star dbms ; do

      case $server_type in

         icesvr)  
            SERVER_TYPE="ICESVR"
            oidlist="$ice_ids"
            ;;
         star)  
            SERVER_TYPE="Star"
	    oidlist="$star_ids"
            ;;
                
         dbms)
            SERVER_TYPE="DBMS"
	    oidlist="$dbms_ids"
            ;;
      esac

      count_name=${server_type}_count

      server_count=`eval echo '$'$count_name`

      [ $server_count -ge 1 ] &&
      {
         for server_id in $oidlist ; do

	    ## Again, the message distinction here is mostly historical,
	    ## we're going to start with a "set server shut" attempt.
            wmsg ""
            if $FORCE || [ $minutes -eq 0 ] ; then
               wmsg "Shutting down $SERVER_TYPE server $server_id ..."
            else
               wmsg "Closing $SERVER_TYPE server $server_id to new connections ..."
            fi

	    cat >/tmp/(PROG1PRFX)monin.$$ << !
set server shut
quit
!
	    echo "(PROG1PRFX)monitor $server_id </tmp/(PROG1PRFX)monin.$$" >/tmp/monex.$$
	    ## Much patience this first time in case server is super busy
	    exec_with_timeout 45 "sh /tmp/monex.$$" >/tmp/(PROG1PRFX)monit.$$
	    rm -f /tmp/monex.$$
            if [ $? -eq 0 ]
	    then
		# save connection count
		cons=`grep 'sessions remaining' /tmp/iimonit.$$ \
		    | sed 's/^[^0-9]*\([0-9][0-9]*\).*$/\1/'`
            else
		warning_box 76 << ! | wmsg
(PROG1PRFX)monitor connection to $SERVER_TYPE server $server_id failed. 
!
		cons=0
	    fi
	    sid=`echo "$server_id" | awk -F. '{print $NF}'`
	    eval cons_${sid}="$cons"
         done
	 rm -f /tmp/(PROG1PRFX)monit.$$ /tmp/(PROG1PRFX)monin.$$
      }
   done
}

#
#  For cases where cache_sharing was OFF and there were more than
#  one dbms server, the extra dbms servers need to be shutdown
#  first.  At this point all the servers were sent the server shut
#  command.  Wait a few seconds to allow the extra dbms severs to exit,
#  then issue rmcmdstp which will shutdown the rmcmd server and allow
#  the final dbms server to exit.
## (Not sure why this is needed, other than stupidity in rmcmdstp.)
#
[ $rmcmd_count -ge 1 -a "$CACHE" = "OFF" -a $dbms_count -gt 1 ] &&
{
    wmsg ""
    wmsg "Shutting down the remote command server..."

    sleep 2
    exec_with_timeout 10 rmcmdstp
    sleep 2
    rmcmd_count=`$PSCMD | grep "rmcmd $(PROG3PRFX)_INSTALLATION" | wc -l | tr -d ' '`

    # Again with the double hit on rmcmdstp, but avoid it if it looks like
    # no dbms servers are running any more.
    if [ $rmcmd_count -ge 1 ] ; then
	dbms_running=0
	for server_id in $dbms_ids ; do
	    if server_active $server_id ps
	      then
		dbms_running=1
		break
	    fi
	done
	if [ $dbms_running -eq 1 ] ; then
	    exec_with_timeout 10 rmcmdstp
	    sleep 2
	fi
    fi
}

#
# Start shutdown of other local NUMA nodes (if any).
#
[ -n "$targets" ] && exec_locals

#
# Wait timeout period for connections to terminate
#

[ $gcc_count -gt 0 ] || [ $dbms_count -gt 0 ] || [ $star_count -gt 0 ] \
   || [ $gcb_count -gt 0 ] || [ $gcd_count -gt 0 ] \
   && [ $seconds -gt 0 ] &&
{
   [ $minutes -gt 0 ] && wrap_lines 76 << ! | wmsg

Waiting $minutes $minute_or_minutes for active sessions to terminate ...
!
   sleep $seconds
}

#
# Check whether DAS, Star, and Net servers, Bridge servers have exited.
## Check networkish servers first, then check the DBMS server.
#
abort_shutdown=false
active_iidbms=""
active_iigcc=""
active_iigcb=""
if [ $star_count -gt 0 -o $gcc_count -gt 0 -o $gcb_count -gt 0 ] ; then

   #
   # Check on Net servers.
   #
   [ $gcc_count -ge 1 ] &&
   {
      for server_id in  $gcc_ids ; do

         if (PROG1PRFX)namu >/tmp/(PROG1PRFX)namu.$$ << !
show comsvr
quit
!
         then :
         else
	     fatal_box 76 << ! | wmsg
(PROG1PRFX)namu failed to connect to the name server.
!
	    cleanup
            exit 1 
         fi

         if grep $server_id /tmp/(PROG1PRFX)namu.$$ | grep -v Copyright >/dev/null
         then
            abort_shutdown=true
            active_iigcc="$active_iigcc $server_id"
            wrap_lines 76 << ! | wmsg

Net server $server_id still has active session(s).
!
         fi
      done 
   }

   #
   # Check on DAS servers.
   #
   [ $gcd_count -ge 1 ] &&
   {
      for server_id in  $gcd_ids ; do

         if (PROG1PRFX)namu >/tmp/(PROG1PRFX)namu.$$ << !
show gcd
quit
!
         then :
         else
            fatal_box 76 << ! | wmsg
(PROG1PRFX)namu failed to connect to the name server.
!
	    cleanup
            exit 1
         fi

         if grep $server_id /tmp/(PROG1PRFX)namu.$$ | grep -v Copyright >/dev/null
         then
            abort_shutdown=true
            active_iigcd="$active_iigcd $server_id"
            wmsg << !

Data Access Server $server_id still has active session(s).
!
         fi
      done
   }

   #
   # Check on Bridge servers.
   #
   [ $gcb_count -ge 1 ] &&
   {
      for server_id in  $gcb_ids ; do

         if (PROG1PRFX)namu >/tmp/(PROG1PRFX)namu.$$ << !
show bridge
quit
!
         then :
         else
	     fatal_box 76 << ! | wmsg
(PROG1PRFX)namu failed to connect to the name server.
!
	    cleanup
            exit 1 
         fi

         if grep $server_id /tmp/(PROG1PRFX)namu.$$ | grep -v Copyright >/dev/null
         then
            abort_shutdown=true
            active_iigcb="$active_iigcb $server_id"
            wrap_lines 76 << ! | wmsg

Bridge server $server_id still has active session(s).
!
         fi
      done 
   }
fi
rm -f /tmp/(PROG1PRFX)namu.$$

[ $dbms_count -gt 0 ] &&
{
   #
   # Now check on DBMS and Star servers.
   #
   for server_type in star dbms ; do

      case $server_type in

         star)  
            SERVER_TYPE="Star"
	    oidlist=$star_ids
            ;;
                
         dbms)
            SERVER_TYPE="DBMS"
	    oidlist=$dbms_ids
            ;;
      esac

      count_name=${server_type}_count

      server_count=`eval echo '$'$count_name`

      [ $server_count -ge 1 ] &&
      {
         for server_id in $oidlist ; do

	    # If this server only had one session, it could still be in image
	    # run-down mode; if ! IMMEDIATE, and the iimonitor worked above,
	    # and we haven't waited minutes already, do so now.
	    #
	    sid=`echo "$server_id" | awk -F. '{print $NF}'`
	    eval cons=\$cons_${sid}
	    [ -z "$cons" ] && cons=0
	    if [ $cons -gt 0 ] ; then
		if [ $cons -eq 1 -a $minutes -eq 0 -a "$IMMEDIATE" != "true" ] ; then
		   wait_secs=20
		else
		   wait_secs=0
		fi
		wait_on_server $server_id $wait_secs && continue

		active_iidbms="$active_iidbms $server_id"
		wrap_lines 76 << ! | wmsg

$SERVER_TYPE server $server_id still has active session(s).
!
	    fi
	    if [ "$FORCE" = 'false' -a "$KILL" = 'false' ] ; then
		# Try to reopen server unless forcing down
		cat >/tmp/(PROG1PRFX)monin.$$ << !
set server open
quit
!
		echo "(PROG1PRFX)monitor $server_id </tmp/(PROG1PRFX)monin.$$" >/tmp/monex.$$
		exec_with_timeout 10 "sh /tmp/monex.$$" >/tmp/(PROG1PRFX)monit.$$
		rm -f /tmp/monex.$$
		if [ $? -eq 0 ]
		then
		    # reopened server so don't continue with the shutdown
		    abort_shutdown=true
		elif server_active $server_id ps ; then
		    # Server still there-- given that the connection failed there
		    # is possibly something amiss, but abort the shutdown anyway
		    # Don't message if the first iimonitor failed as well.
		    #
		    abort_shutdown=true
		    if [ $cons -gt 0 ] ; then
			warning_box 76 << ! | wmsg
(PROG1PRFX)monitor connection to $SERVER_TYPE server $server_id failed. 

Unable to reopen server.
!
		    fi
		fi
	    fi
         done
      }
   done
}

if [ "$FORCE" = 'true' -o "$KILL" = 'true' ] ; then
   if [ "$full_shutdown" -ne 0 ] ; then
	shutscope="active"
   else
	shutscope="chosen, active"
   fi

   if [ $gcn_count -ge 1 -o $acp_count -ge 1 -o $rcp_count -ge 1 -o \
        $ora_count -ge 1 -o $inf_count -ge 1 -o $syb_count -ge 1 -o \
        $udb_count -ge 1 ] ||
      [ "$active_iidbms" ] || [ "$active_iigcb" ] || [ "$active_iigcd" ] || \
      [ "$active_iigcc" ]; then

       wrap_lines 76 << ! | wmsg

Forcing $shutscope servers in this installation to shut down ...
!
   fi

   #
   # Terminate active Net servers.
   #
    [ "$active_iigcc" ] &&
    {
	for server_id in $active_iigcc ; do

	    wmsg ""
	    wmsg "Terminating active Net server $server_id ..."

	    (PROG1PRFX)gcstop -s $server_id >/dev/null ||
	    {
		fatal_box 76 << ! | wmsg
Unable to issue shutdown command.
!
		cleanup
     		exit 1

	    }
	done
    }

   #
   # Terminate active DAS servers.
   #
    [ "$active_iigcd" ] &&
    {
	for server_id in $active_iigcd ; do

	    wmsg ""
	    wmsg "Terminating active Data Access Server $server_id ..."

	    (PROG1PRFX)gcstop -s $server_id >/dev/null ||
	    {
		fatal_box 76 << ! | wmsg
Unable to issue shutdown command.
!
		cleanup
		exit 1
	    }
	done
    }

   #
   # Terminate active Bridge servers.
   #
   [ "$active_iigcb" ] &&
   {
      for server_id in $active_iigcb ; do

         wmsg ""
         wmsg "Terminating active Bridge server $server_id ..."

      	 (PROG1PRFX)gcstop -s $server_id >/dev/null ||
         {
	     fatal_box 76 << ! | wmsg
Unable to issue shutdown command.
!
	    cleanup
     	    exit 1

         }
         done
    }

   #
   # Terminate active DBMS and Star servers.
   #
   [ "$active_iidbms" ] &&
   {
	for server_id in $active_iidbms ; do

	    wmsg ""
	    wmsg "Stopping active server $server_id ..."

	    cat >/tmp/(PROG1PRFX)monin.$$ << !
stop server
quit
!
	    ## less patience this time
	    echo "(PROG1PRFX)monitor $server_id </tmp/(PROG1PRFX)monin.$$" >/tmp/monex.$$
	    exec_with_timeout 10 "sh /tmp/monex.$$" >/tmp/(PROG1PRFX)monit.$$
	    rm -f /tmp/monex.$$
	    if [ $? -ne 0 ]
	    then
		# If it won't talk to iimonitor, try taking it out with -QUIT.
		# (do this whether forcing or killing or both.)
		pid=`grep "connect id $server_id" /tmp/csreport.$$ | awk '{ print $4 }'`
		if [ -n "$pid" ] ; then
		    wmsg "(PROG1PRFX)monitor failed, sending -QUIT to server process $pid ..."
		    kill -QUIT $pid >/dev/null 2>&1
		fi
	    fi
	    wait_secs=20
	    if [ "$IMMEDIATE" = 'true' ] ; then
		wait_secs=3
	    fi
	    wait_on_server $server_id $wait_secs && continue

	    if [ "$FORCE" = 'true' -a "$KILL" = 'false' ]
	    then
		warning_box 76 << ! | wmsg
(PROG1PRFX)monitor command to DBMS server $server_id failed to stop the server.

If this is a shared cache server, then this server could still be in the process of shutting down.  Verify that this server has shutdown when ingstop is complete.
!
	    fi
	done
	if [ "$KILL" = 'true' ] ; then
	    # If killing as well as forcing, run thru the dbms PID list and make
	    # sure they are all down.  Kill -9 anything that's left.
	    new_list=
	    for pid in $dbms_pids ; do
		if kill -0 $pid >/dev/null 2>&1 ; then
		    new_list="$new_list $pid"
		fi
	    done
	    if [ -n "$new_list" ] ; then
		wmsg "Some DBMS server(s) failed to stop."
		wmsg "Killing remaining active DBMS servers ..."
		for pid in $new_list ; do
		    kill -9 $pid >/dev/null 2>&1
		done
	    fi
	fi
   }

elif $abort_shutdown ; then

   if $server_host ; then

      if [ "$shutids_dbms" = "all" -a "$shutids_star" = "all" ] ; then
	  shutscope1="active"
	  shutscope2="All"
      else
	  shutscope1="the selected"
	  shutscope2="These"
      fi

      wrap_lines 76 << ! | wmsg

Shutdown of $shutscope1 DBMS and Star servers has been aborted.
$shutscope2 active DBMS and Star servers have been re-opened for connections.

!
   else
      wrap_lines 76 << ! | wmsg

Full shutdown of this (PROD1NAME) client installation has been aborted.

!
   fi

   [ $gcc_count -gt 0 ] &&
   {
      wrap_lines 76 << ! | wmsg
Active Net servers which have been closed to new connections cannot be \
re-opened.  To enable new network connections to or from this installation, \
you must start another Net server.

!
   }

   [ $gcd_count -gt 0 ] &&
   {
      wrap_lines 76 << ! | wmsg
Active Data Access servers which have been closed to new connections cannot be \
re-opened.  To enable new Data Access Server connections to or from this \
installation, you must start another Data Access Server.

!
   }

   [ $gcb_count -gt 0 ] &&
   {
      wrap_lines 76 << ! | wmsg
Active Bridge servers which have been closed to new connections cannot be \
re-opened.  To enable new network connections to or from this installation, \
you must start another Bridge server.

!
   }

   # Refresh Name Server list

   iinamu << EOF > /dev/null
show servers
EOF

   cleanup
   exit 1

fi

#
# Terminate the recovery server and archiver process
#
dmfdown=0
[ $rcp_count -gt 0 ] &&
{
    shutok='y'
    dmfdown=1
    wmsg ""
    msg="the recovery and archiver processes ..."
    if $IMMEDIATE ; then
	wmsg "Performing an immediate shutdown on $msg"
	cmd="rcpconfig -imm_shutdown"
	maxtries=1
    else
	wmsg "Shutting down $msg"
	cmd="rcpconfig -shutdown"
	maxtries=25
    fi

    exec_with_timeout 20 "$cmd" >/tmp/rcpconf.$$
    if [ $? -ne 0 ] ; then
	fatal_box 76 1 -1 << ! | wmsg
Unable to deliver shutdown message to the logging system.  Please review the following error message for help in diagnosing the problem:

!
	cat /tmp/rcpconf.$$ | box 76 -1 | wmsg
	if [ "$KILL" = 'false' ] ; then
	    cleanup
	    exit 1
	fi
	wmsg "The recovery server will be forcibly killed."
	## avoid rcpstat, and arrange for kill -9 of the recovery server:
	shutok=""
	dmfdown=0
    fi
    sleep 2
    indef=' indefinitely'
    if [ "$KILL" = 'true' ] ; then
	indef=' up to 100 seconds'
    fi
    [ -n "$shutok" ] && exec_with_timeout 10 "rcpstat -online -silent" &&
    {
	if [ "$IMMEDIATE" = 'false' ] ; then
	    wmsg ""
	    wmsg "Waiting$indef for the logging system to shut down ..."
	fi;
	tries=0
	while exec_with_timeout 10 "rcpstat -online -silent"
	do
	    sleep 4
	    tries=`expr $tries + 1`
	    if [ "$KILL" = 'true' -a $tries -ge $maxtries ] ; then
		wmsg "The logging system has not shut down in time."
		wmsg "The recovery and archiver processes will be killed."
		dmfdown=0
		break
	    fi
	done
    }
    if [ $dmfdown -eq 1 ] ; then
	wmsg ""
	wmsg "The recovery and archiver processes have exited."
	acp_count=0
    elif [ "$KILL" = 'true' ] ; then
	# Take out the rcp by force
	for pid in $rcp_pids ; do
	    kill -9 $pid >/dev/null 2>&1
	done
	## Ask for ipcclean, below.
	dmfdown=1
    fi
}

#
# Stop the archiver process, if not done above
## Might as well kill -9 it, should have gone away with the rcp
## if it were clean.
## Note that acp-count is cleared above if the logging system
## shut down normally.
#
[ $acp_count -gt 0 ] &&
{
   wmsg ""
   wmsg "Shutting down the archiver process..."
   for pid in $acp_pids
   do
	kill -9 $pid 1> /dev/null 2>&1
   done
}


#
# Clean up shared memory and semaphores.
#
if [ $dmfdown -eq 1 ] ; then

   wrap_lines 76 << ! | wmsg

Cleaning up shared memory and semaphores...
!
   if $server_host && [ -n "$shutids_rcp" ]
   then
	## FIXME would be useful to have a utility that forces the
	## spinlock in the system segment to OFF, so that these two will
	## always work.
	exec_with_timeout 10 cscleanup >/dev/null 2>&1
	sleep 1
	exec_with_timeout 10 ipcclean >/dev/null 2>&1
	if [ $? -ne 0 ] ; then
	    wmsg "ipcclean could not be run.  Use ipcs -m and ipcrm to"
	    wmsg "remove idle Ingres shared-memory segments (nattch = 0)"
	fi
   fi
else
   if [ "$FORCE" = 'true' -o "$KILL" = 'true' ] ; then
	if [ -n "$need_dbms_cfg" ] ; then
	    # Probably ingstop -iidbms=xxx, clean up any shared caches
	    # used by the dbms's listed on the command line.  Made trickier
	    # since the command line is server ID, while we need the config
	    # ID for config.dat.  Only needed if force/kill since a normal
	    # dbms shutdown cleans up shared cache segments.

	    sort </tmp/dbms_cfg.$$ | uniq | while read config_id
	    do
		if [ "$config_id" = '(default)' ] ; then
		    config_id='\*'
		fi
		grep "\.$config_id\.cache_name" $config_dat | awk '{ printf("%s.\n", $2)}' | sort -u | while read cache_name
		do
		    cnt=`grep "\.$config_id\.cache_sharing" $config_dat | grep -i ":[ 	]*ON" | wc -l`
		    if [ $cnt -gt 0 ] ; then
			echo $cache_name >> /tmp/cache_names.$$
		    fi
		done
	    done

	    cat /tmp/cache_names.$$ | sort -u | while read cache_name
	    do
		ipcclean -cache=$cache_name
	    done

	    rm -f /tmp/cache_names.$$
	fi
   fi

   # Refresh Name Server list

   iinamu << EOF > /dev/null
show servers
EOF

fi

#
# Terminate all Gateways
#

#
# Utility functions for gateway shutdown
#

tolowercase1()
{
  sed -e 'y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/'
}

tolowercase2()
{
  echo $* | tolowercase1
}

gatewayname_to_short()
{
  name_to_check=`tolowercase2 $1`

  case "$name_to_check" in
  oracle)
     echo ora
     ;;
  sybase)
     echo syb
     ;;
  informix)
     echo inf
     ;;
  db2udb)
     echo udb
     ;;
  esac
}

abort_shutdown=false

if ( $FORCE )
then
  GATEWAY_SHUT_MSG="Forcing down"
  GATEWAY_SHUT_PARM="-force"
elif ( $KILL )
then
  GATEWAY_SHUT_MSG="Killing"
  GATEWAY_SHUT_PARM="-kill"
else
  GATEWAY_SHUT_MSG="Shutting down"
  GATEWAY_SHUT_PARM=""
fi

for currentgateway_name in Oracle Sybase Informix DB2UDB
do
  # Determine which gateway we are now looking at
  # find out how many are running
  short_gatewayname=`gatewayname_to_short $currentgateway_name`
  eval currentgateway_count=`eval echo '$'${short_gatewayname}_count`

  # If gateway running, issue shutdown request
  [ $currentgateway_count -ge 1 ] &&
  {
     wmsg ""
     wmsg "${GATEWAY_SHUT_MSG} the ${currentgateway_name} Gateway server..."
     sh $(PRODLOC)/(PROD2NAME)/utility/iigwstop $GATEWAY_SHUT_PARM \
         `tolowercase2 ${currentgateway_name}` || abort_shutdown=true
  }

  # If gateway shutdown request failed, abort shutting down any more processes
  if $abort_shutdown
  then
    wrap_lines 76 << ! | wmsg

Full shutdown of this Ingres installation has been aborted.

!
    cleanup
    exit 1
  fi
done # Gateway shutdown

#
# Terminate the name server
#
[ $gcn_count -ge 1 ] &&
{
   wmsg ""
   wmsg "Shutting down the name server..."
   (PROG1PRFX)namu << ! > /tmp/shuttmp.$$ 2>&1
stop
y
!
}

wmsg ""

## FIXME, if FORCE and KILL on all, probably should re-do ps looking for ii's
## and dmf's with the current installation ID.  If found, kill -9 'em
## and do a safety ipcclean.

#
#	END LOCAL PROCESSING
#

report_remote_results

cleanup
exit 0
