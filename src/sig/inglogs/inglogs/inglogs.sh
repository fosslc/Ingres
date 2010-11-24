:
INGLOGS_VERSION="1.28"
#
#usage: inglogs [-noparallel] [-internalthreads] ... [-h|-help] [--help] [-v|-version|--version]
#(run "inglogs -help" or "inglogs --help to get a short or detailed description)
#
#The following logs and configuration are collected(if exist):
#syscheck (-v), ulimit -a, ulimit -aH,uname -a
#ps, ipcs, netstat
#config.dat,config.log,protect.dat(6.4: rundbms.opt)
#ingprenv, env, version.rel, version.dat
#errlog.log, iircp.log, iiacp.log(6.4: II_RCP.LOG, II_ACP.LOG)
#II_DBMS_LOG's (if exist)
#logstat
#logstat -verbose(OI/II)
#lockstat
#lockstat -dirty(OI/II)
#csreport
#iinamu output 
#iimonitor:show (all) sessions, show (all) sessions formatted, stack, show mutex
#depending on the platform some additional information
#run "inglogs --help" to get actual list of collected info
#Comment:
# Starting with version 1.26 you may see /bin/bash used on
# Linux instead of /bin/sh (check the very first line). This is because
# inglogs is now part of the distribution/patch (in sig/inglogs) and we
# use bash as standard shell on Linux. This shouldn't be any problem unless
# such an inglogs version is copied to a machine that doesn't have the bash,
# in this case you would need change the first line.
#####################################################################
##history
## 09-jul-2003	Kristoff Picard		initial version		0.9
##     mutex part not tested, but should work. (based on the old mutex script)
##     basic tests done on Solaris, Linux, AIX, OSF and HP
##
## 08-aug-2003   Kristoff Picard          version 0.91
##     corrected wrong filename for lockstat -dirty output
## 12-sep-2003   Kristoff Picard          version 0.92
##     on Linux do an additional "ps auxm" (if the "m" flag is supported,
##      this depends on the Linux version). With this flag we will see the
##      threads too(depending on the Linux version the threads might be
##      unvisible otherwise, seen in RedHat 9)
## 12-may-2004   Kristoff Picard          version 0.92a
##     temporary changes because of problems with 64 bit version
##     don't call stacks in iimonitor. For the future we could
##     distinct between 32 and 64 bit versions and also determine
##     whether system threads are used(were the stack command isn't useful 
##     anyway, i.e on most sytems)
## 17-jun-2004   Kristoff Picard          version  0.93
##     put $II_SYSTEM/ingres/bin and $II_SYSTEM/ingres/bin/utility
##     at the beginning of PATH, to avoid conflicts with other commands
##     (for example the Solaris lockstat command)
## 02-dec-2004   Kristoff Picard          version  0.94
##    added possibility to run the functions in parallel, this can be helpful
##    when a function is hanging. A function might still hang within a single
##    command, but the other functions can continue. To use this feature call
##    "inglogs -parallel".
##    Added "netstat -an" command, useful if "netstat -a" should hang because
##    of problems with name resolution.
##    Querying the name server is now the last command in function ingres_state
## 22-dec-2004   Kristoff Picard          version  0.95
##    parallel mode is now the default (use -noparallel to avoid this mode)
##    most of the ingres commands are now done in parallel mode too,
##    especially the iimonitor is now done for each server in parallel
##    Some system commands are executed in parallel mode also.
##    These changes required some additional sub function, such as do_iimonitor
## 03-jun-2005   Kristoff Picard          version  0.96
##    added "show server" to the iimonitor output
##    allow to run the "stacks" command in iimonitor, for this you need to call
##    inglogs with the "-internalthreads" command. Don't use this flag when
##    running system threads, especially in 64 bit installations, this can
##    crash the server. We don't rely on II_THREAD_TYPE here, since there
##    are versions where this variable is ignored.
##    changed the logic to get the dbms logs, the old method sometimes failed
##    and was clumsy.
##    on AIX print OS version using "oslevel"
##    on TRU64 print OS version using "sizer -v"
##    print the used command to inglogs_version.txt
## 19-Aug-2005   Kristoff Picard          version  1.0
##    removed os_specificics, the various commands are now executed in other 
##    modules.
##    When running in parallel mode(default), wait for the background processes
##    to be completed. You may have to interrupt the script should a background 
##    processes hang. Removed some progress messages which were not always
##    correct and are no longer necessary.
##    Added "-help" and "--help" flag to get a brief or detailed description
##    Added additional flags to switch off a "module":
##    -nosystemconfig -nosystemstate -nologfiles -noingresconfig -noingresstate
##    This gives you some control which information will be collected.
##    Assume that the installation owner is running the script when
##    $II_SYSTEM/ingres is writeable, removed the check whether
##    the script is running as "ingres".
## 25-Jan-2006   Kristoff Picard          version  1.1 
##    Changed the way to determine mutex numbers.
##    Sessions in mutex are sometimes listed in a different format, 
##    the script failed to identify the mutex numbers in this second format.
## 16-Feb-2006   Kristoff Picard          version  1.11
##    redirect stderr for some commands to &1
## 13-Jul-2006   Kristoff Picard          version  1.12
##    added /sbin and /usr/sbin to the end of the PATH
##    on OSF run "ksh ulimit", in the standard sh we don't get all results
##    added --version flag to print out the version of inglogs
## 06-nov-2006   Kristoff Picard          version  1.13
##    added support for a64_sol 
##    check for symbol.log and if exist copy it too
##    try to copy protect.dat only if exists
## 10-Nov-2006   Kristoff Picard          version  1.14
##    when the script is interrupted(^C) check $ports, if empty
##    try to collect iimonitor output again
##    (this is useful when ports=`csreport ....` hung).
##    Trying to cleanup (kill background processes)
##    To really interrupt the script it might be necessary to
##    interrupt twice.
## 08-dec-2006   Kristoff Picard          version  1.15
##    All output of iimonitor is now collected twice with a 10-seconds pause
##    between the first and secon collect. This allows to see changes happening
##    in the system.
## 21-may-2007   Kristoff Picard          version  1.16
##    Changed PSCMD_2 for AIX to get the kernel thread id
##    Added PSCMD_2 for Solaris to get the LWP id
##    flags: allow -h (same as -help), allow -v and -version (same as --version)
##    exit when an invalid flag is given, show usage before
## 21-may-2007   Kristoff Picard          version  1.17
##    copy install.log if exists
## 22-sep-2007   Kristoff Picard          version  1.18
##    replaced all "cp" copy commands by "cp -p" to keep the timestamps
##    use existing csreport output if possible, ideally we now call csreport
##    only once.(give csreport 5 seconds to complete, assume output is okay,
##    if it contains a string "server 31:")
##    For AIX try to run procstack, even though it isn't so helpful as the
##    Solaris pstack command.
##    "-noparallel" mode didn't work on OSF
##    todo: on OSF signal handler doesn't work for whatever reason
## 01-oct-2007   Kristoff Picard          version  1.19
##    On Solaris and AIX get the stacks for the other Ingres processes (not 
##    attached to the logging system, iigcn,iigcc,iigcj,iigcd,iigcb) too.
## 22-nov-2007   Kristoff Picard          version  1.20
##    Add PSCMD_3 command to get the virtual size of the processes
##    PSCMD_2 was not set on a64_lnx and usl_us5
## 31-mar-2008   Kristoff Picard          version  1.21
##    added a64_lnx to the linux versions (amd_lnx is outdated)
##    iinamu is now executed in background too
##    added some commands collected by "ingdiags": 
##    vmstat, uptime 
##    some additional netstat commands 
##    top on Solaris, HP and Linux
##    df -k (bdf on HP)
##    ls -lR bin utility lib files
##    infodb
##    "show mutex x" is now executed 2x5 times in iimonitor
##    psrinfo and prtconf on some platforms
## 25-aug-2008   Kristoff Picard          version  1.22
##    added pstack on Linux (doesn't exist in all Linux versions)
##    added debug scripts to get stacktraces, using gdb on Linux and HP,
##     ladebug on Tru64 and dbx on AIX. (executed with +stackdebug flag only)
##    pstack, procstack and the debugscripts are now executed twice
##    list open files for all Ingres processes(Solaris, Linux, AIX)
##    copy all DBMS logs, not only the active ones
##    i64_lnx and int_rpl added to the list of Linux versions
##    wait for the background processes of "ingres_state" before returning to 
##     main(easier to see whether the script has completed)
## 16-sep-2008   Kristoff Picard/Daryl Monge   version 1.23 (not released)
##    added support for Mac OS/X (thanks to Daryl)
##    added "show smstats" to iimonitor (iimon_server...)
##    If pstack is not found on Linux, try gstack
##    added repstat output and rcpconfig.log
##    added ls -l II_WORK output
##    made get_exec and other_pids to work on HP
##    on HP look for /opt/langtools/bin/gdb when using gdb(the official supported
##     one by HP, have seen strange things with other versions
##    redirect stderr was missing for some commands
##    ps was put in background twice
## !!!   +stackdebug may not work at the moment on osx
## 07-nov-2008   Kristoff Picard       version 1.24
##    redirected top output on OSX
##    collect iicrash.log if requested (+iicrashlog)
##    iimonitor for iigcc and iigcd (9.1 and above)
## 18-mar-2009   Kristoff Picard       version 1.25
##    corrected an error in get_exec(lp64 was added to the wrong place)
##    don't call iimonitor against gcc and iigcd where it is not supported
## 01-jul-2009   Kristoff Picard       version 1.26
##    look for iiexcept.opt (ingresconfig) and iivdb.log (ingreslogfiles)
##    if INGLOGSDIR is set in environment, create inglogdirectory within that 
##     directory
##    Look for flags defined in $INGLOGFLAGS and append to the given flags
##    report INGLOGSDIR and INGLOGFLAGS into inglogs_version.txt
##    for each file in files/memory call iimemkey file (output in memkey.out)
##    On Solaris collect isainfo -b output (32/64 bit mode)
##    Add timestamps to the iimonitor and pstack output(begin/end)
## 14-May-2010 (hanal04) Bug 123291
##    Ensure LOGDIR is unique to the inglogs process to avoid conflict
##    with concurrent instances (and associated hang).
## 27-oct-2010   Kristoff Picard       version 1.28 
##    bug 124651
##    Corrected a problem with do_iimonitor_other: sometimes a '*' is picked
##    up from the iinamu output instead of the port of the gcc or gcd  which 
##    is expanded to each existing file. This is caused by unexpected iinamu 
##    output. Fix is to change the sed command in "other_ports()" making sure 
##    that every "IINAMU>" is deleted (and not just the first one)
##    On Solaris, replaced "top 50" with "prstat -n50 1 1"
##    On Solaris, collect "prstat -L -n100000 1 3" output (shows statistics for 
##    each light-weight process).
##    On Solaris add another ps command, /usr/ucb/ps -auxwww (useful if the 
##    complete command is very long)
##    Added a description of INGLOGSDIR and INGLOGSFLAGS to the help text.
##    (note that the history 01-jul-2009 comment wrongly says INGLOGFLAGS 
##    instead of INGLOGSFLAGS)
##    Wrote a timestamp to inglogs_completed.txt when inglogs completed. This
##    allows other scripts to check whether inglogs has completed.
##    Copy symbol.bak if exists
##    fixed a problem with gdb on Mac OSX
##    On Linux collect lsb_release -a output
##    history comments are now using ##, so will no longer appear in the 
##    compiled inglogs script, removed one obsolete comment.
#####################################################################
#
show_usage()
{
   echo "Usage: `basename $0` [-nosystemconfig] [-nosystemstate]"
   echo "               [-nologfiles] [-noingresconfig] [-noingresstate] [+stackdebug]"
   echo "               [+iicrashlog]"
   echo "               [-internalthreads] [-noparallel]"
   echo "       `basename $0` [-h|-help]"
   echo "       `basename $0` [--help]"
   echo "       `basename $0` [-v|-version|--version]"
}

show_help()
{
   cat << ENDE | more
`show_usage`

The inglogs script collects some information from your system which
might be helpful for analysis. By default it collects as much information
as possible, but you can switch off some "modules" if wanted.
By default, most of the commands are running in the background, i.e in
parallel. This is especially useful in cases where a particular command
hangs - otherwise it would block all following commands.
The script waits until all processes are ready, so it might not finish.
In this case use ^C to interupt (or kill -INT pid), you may need 
to interupt again, since the script is trying to collect the iimonitor
output if not collected yet.
Output is collected in directory inglogs_yyyymmdd_hhmmss_pid$$ which is by 
default created in the current working directory. Use INGLOGSDIR defined in the
UNIX environment, to specify another directory where inglogs creates its output 
directory
Use INGLOGSFLAGS defined in the UNIX environment to specify the flags 
(described below) to specify the flags inglogs should pickup automatically.


The following information(at least) is collected, the exact command may differ
from platform to platform:
module systemconfig:
  ulimit -a, ulimit -aH, uname -a
  /lib/libc.so.6 (libc version, Linux only)
  oslevel (AIX only)
  sizer -v (OS version, tru64)
  psrinfo, prtconf (SunOS, UnixWare, UNIX_SV)
module systemstate:
  ps 
  ipcs
  netstat
  vmstat
  uptime
  top (Solaris, Linux, HP)
  df
  ls -lR bin utility lib files
module ingconfig:
  config.dat, config.log, protect.dat,symbol.tbl, symbol.log
  ingprenv
  env
  version.rel, version.dat
  iiexcept.opt
module inglogfiles:
  errlog.log, iircp.log, iiacp.log
  dbms logs
  iivdb.log
module ingstate:
  pstack (Solaris/Linux), procstack (AIX)
  stacktraces using debugger (see +stackdebug flag)
  list of open files(Solaris, Linux, AIX)
  logstat, logstat -verbose
  lockstat, lockstat -dirty
  csreport
  iimonitor: show all sessions, show all sessions formatted
  iimonitor: stacks (ONLY if explicitly requested, dangerous, see below)
  iimonitor: show mutex x
  iinamu
  iimemkey
  
Flags:
-no<modulename>
  (-nosystemconfig -nosystemstate -nologfiles -noingresconfig -noingresstate)
  By default all the above modules are executed, you can switch off
  one or multiple modules by using -no<modulename> ... 

+stackdebug
  If set, try to get the stack traces by using an appropriate debugger
  not needed on Solaris because of pstack 
  you may use it on AIX, because procstack output is incomplete if OS <5.3
  Ignored if -noingresstate is used
  Ignored on some platforms

+iicrashlog
  If set, look for iicrash.log and copy if exist. (can be huge)
  This flag is ignored when -nologfiles is used

-internalthreads
  When using the flag "-internalthreads" the command "stacks" in iimonitor
  is executed. This command is useful only when running with Ingres internal
  threads - and it is dangerous on installations running with 
  system/posix threads, at least in 64bit versions this crashes the dbms server.

-noparallel
  Disables the parallel mode, i.e. no process is started in the background

-help (-h)
  Prints out a short usage message and exits

--help
  Prints out this message and exits

--version (-v, -version)
  Prints out the version of inglogs and exits


ENDE
}

add_timestamp()
{
   [ "$1" = END ] && echo ""
   echo "$1:" `date`
}

get_ingres_version()
{
  [ -r $II_SYSTEM/ingres/version.rel ] && head -1 $II_SYSTEM/ingres/version.rel || echo unknown
}
inglogs_init()
{
  echo "init ..."
  [ -n "$INGLOGSFLAGS" ] && {
     echo "Warning: INGLOGSFLAGS is set to $INGLOGSFLAGS"
     echo "these flags will be appended to the flags inglogs is called with"
  }

#  SCRIPTUSER=`id | sed -e 's/).*//' -e 's/.*(//'`
#  if [ $SCRIPTUSER != ingres ] 
#  then
#    echo "Please run as user ingres"
#    exit 1
#  fi
  
  [ -z "$II_SYSTEM" ] && {
    echo "Please set II_SYSTEM before running this script"
    exit 2
  }
#if $II_SYSTEM/ingres is writeable assume we are the installtion
#owner - might be wrong
  [ -w $II_SYSTEM/ingres ] || {
    echo "Please run as installation owner.($II_SYSTEM/ingres not writeable)"
    exit 3
  }
  F=$II_SYSTEM/ingres/files
  PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH:/sbin:/usr/sbin 
  export PATH
  [ -x $II_SYSTEM/ingres/utility/iigcfid ] && {
  #This is an OI/II installation
    ING_RELEASE=II
    IINAMUCMD=iigcfid
    IIMONALL=all
    IIRCP=$F/iircp.log
    IIACP=$F/iiacp.log
    CONFIG="$F/config.dat $F/config.log" 
    [ -r $F/protect.dat ] && CONFIG="$CONFIG $F/protect.dat"
    [ -r $F/symbol.log ] && CONFIG="$CONFIG $F/symbol.log"
    [ -r $F/symbol.bak ] && CONFIG="$CONFIG $F/symbol.bak"
    [ -r $F/symbol.tbl ] && CONFIG="$CONFIG $F/symbol.tbl"
    [ -r $F/install.log ] && CONFIG="$CONFIG $F/install.log"
    [ -r $F/iiexcept.opt ] && CONFIG="$CONFIG $F/iiexcept.opt"
    INSTCODE=`ingprenv II_INSTALLATION`
    DBMSLOG=`ingprenv II_DBMS_LOG`
    SYSCHECKFLAG="-v"
    LP64=`ingprenv II_LP64_ENABLED`
  } || {
  #assuming 6.4
    ING_RELEASE=64
    IINAMUCMD=runiinamu
    IIMONALL=""
    IIRCP=$F/II_RCP.LOG
    IIACP=$F/II_ACP.LOG
    CONFIG="$F/rundbms.opt"
    INSTCODE=`ingprenv1 II_INSTALLATION`
    DBMSLOG=`ingprenv1 II_DBMS_LOG`
    SYSCHECKFLAG=""
  }
  
  . $II_SYSTEM/ingres/utility/iisysdep  
  AWK=awk
  DF="df -k"
  PSCMD_2=""
  PSCMD_3=""
  GDB="gdb"
  TOP=""
  case "$VERS" in
      su4_us5|su9_us5|a64_sol)
         AWK=nawk
         PSCMD_2="ps -eLf"
         PSCMD_3="ps -e -opid,vsz,comm"
         PSCMD_4="/usr/ucb/ps -auxwww"
##         TOP="top 50 >top.out"
         TOP="prstat -n50 1 1 >top.out"
         FILESCMD=/usr/proc/bin/pfiles
         STACKCMD=/usr/proc/bin/pstack
         [ -x "$STACKCMD" ] || unset STACKCMD
         ;;
      axp_osf)
         PSCMD_2="ps auxm"
         PSCMD_3="ps -e -opid,vsz,command"
         ULIMITSH=ksh
         if [ "$stackdebug" = 1 ]
         then
            STACKDEBUGCMD=get_pstack_ladebug
            ladebug -V >/dev/null || unset STACKDEBUGCMD
         fi
         ;;
      rs4_us5|r64_us5)
         PSCMD_2="env COLUMNS=200 ps -em -oTHREAD"
         PSCMD_3="ps -e -opid,vsz,command"
         FILESCMD="/usr/bin/procfiles -n"
         STACKCMD=/usr/bin/procstack
         [ -x "$STACKCMD" ] || unset STACKCMD
         if [ "$stackdebug" = 1 ]
         then
            STACKDEBUGCMD=get_pstack_dbxaix
#there is no -v flag or similar, so we look for dbx here:
            [ -x /usr/bin/dbx -o -x /usr/ccs/bin/dbx ] || unset STACKDEBUGCMD
         fi
         ;;
      int_lnx|amd_lnx|a64_lnx|i64_lnx|int_rpl)
         #check if "m" flag is supported
         ps -m >/dev/null 2>&1
         [ $? = 0 ] &&  PSCMD_2="ps auxm"
         PSCMD_3="ps -e -opid,vsz,command"
         TOP="top -n 1 -b >top.out"
         FILESCMD="lsof -p"
#I haven't seen this on many linuxes, but let us try
         [ -x /usr/bin/pstack ] && STACKCMD=/usr/bin/pstack || {
            [ -x /usr/bin/gstack ]  && STACKCMD=/usr/bin/gstack || unset STACKCMD
         }
         if [ "$stackdebug" = 1 ]
         then
            STACKDEBUGCMD=get_pstack_gdb
            $GDB -v >/dev/null || unset STACKDEBUGCMD
         fi
         ;;
      hpb_us5|hp2_us5|i64_hpu)
         PSCMD_3="env UNIX95=1 ps -e -opid,vsz,comm"
         DF=bdf
         TOP="top -n 50 -d 1 -f top.out"
         if [ "$stackdebug" = 1 ]
         then
           STACKDEBUGCMD=get_pstack_gdb
           GDB=/opt/langtools/bin/gdb
           $GDB -v >/dev/null || unset STACKDEBUGCMD
         fi
         ;;
      usl_us5)
         PSCMD_2="ps -eLf"
         PSCMD_3="ps -e -opid,vsz,comm"
         ;;
      *_osx)
         PSCMD_2="ps -eMf" 
         PSCMD_3="ps -ev"
         FILESCMD="lsof -p"
         TOP="top -l1 -n50 >top.out"
         STACKDEBUGCMD=get_pstack_gdb
         $GDB -v >/dev/null || unset STACKDEBUGCMD
         ;;
  esac
  LOGDIR1=`date +inglogs_%Y%m%d_%H%M%S`
  [ -n "$INGLOGSDIR" ] && LOGDIR=$INGLOGSDIR/$LOGDIR1 || LOGDIR=$LOGDIR1
  LOGDIR=${LOGDIR}_pid$$
  mkdir $LOGDIR && cd $LOGDIR || {
    echo "can't create LOGDIR: $LOGDIR"
    exit 1
  }
  ingres_version=`get_ingres_version`
#try to find out whether iimonitor against the iigcn is supported
# by looking for releases that doen't support it
  othermon=0
  case $ingres_version in
       OI*|II\ 2*|II\ 3*|II\ 9.0*|unknown)
         othermon=0
         ;;
       *)
         othermon=1
         ;;
  esac
  [ "$ING_RELEASE" = 64 ] && othermon=0
  echo "$INGLOGS_VERSION" >inglogs_version.txt
  echo "command used: $0 $flags" >>inglogs_version.txt
  echo "INGLOGSFLAGS: $INGLOGSFLAGS" >>inglogs_version.txt
  echo "INGLOGSDIR: $INGLOGSDIR" >>inglogs_version.txt
}


ingres_ports()
{
  #check if we we can use existing csreport output
  grep -s "server 31:" csreport.out >/dev/null && {
    echo "ingres_ports: csreport ok" >>.csreport.debug
    cat  csreport.out |  $AWK -F"[ ,]" '/inuse [^0]/ {print $9}'
  } || {
    csreport |  $AWK -F"[ ,]" '/inuse [^0]/ {print $9}'
    echo "ingres_ports: csreport not ok" >>.csreport.debug
  }
}

other_ports()
{
  [ -z "$1" ] && return
  [ -f iinamu.out ] || return
  sed -e 's/IINAMU> *//g' iinamu.out | awk "/$1/ {print \$3}"
}

ingres_pids()
{
  #check if we we can use existing csreport output
  grep -s "server 31:" csreport.out >/dev/null  && {
    echo "ingres_pids: csreport ok" >>.csreport.debug
    cat  csreport.out | $AWK -F"[ ,]" '/inuse [^0]/ {print $5}'
  } || {
    csreport | $AWK -F"[ ,]" '/inuse [^0]/ {print $5}' 
    echo "ingres_pids: csreport not ok" >>.csreport.debug
  }
}

other_pids()
{
#UNIX95 is needed on HP, on the other platforms it doesn't harm
   env UNIX95=1 ps -e -opid,args | grep "$II_SYSTEM/ingres/bin/" | grep -v "grep " | egrep "$1" | awk '{print $1}'
}

get_exec()
{
#fails if whitespace is in $II_SYSTEM
  [ -z "$1" ] && return
  case "$VERS" in
      hpb_us5|hp2_us5|i64_hpu)
          export UNIX95=1 && tmpcmd=`ps -oargs -p $1 | tail -1 | awk {'print $1'}`
          unset UNIX95
          ;;
      *_osx)
          tmpcmd=`ps -ocomm -p $1 | tail -1 | awk {'print $1'}`
          ;;
      *)
          tmpcmd=`ps -oargs -p $1 | tail -1 | awk {'print $1'}`
          ;;
  esac
  if [ "$LP64" = TRUE ]
  then
#if this is a hybrid version running in 64bit mode, we may have to add lp64 to the path
    tmpcmd2=`echo $tmpcmd | sed -e "s#$II_SYSTEM/ingres/bin/#$II_SYSTEM/ingres/bin/lp64#"`
#but on AIX it might be already there, so we check whether tmpcmd2 exists
    [ -x "$tmpcmd2" ] && tmpcmd=$tmpcmd2
  fi
  echo $tmpcmd
}

ingres_config()
{
  echo "get ingres configuration ..."
  cp -p $CONFIG .
  ingprenv >ingprenv.out 2>&1
  env >env.out 2>&1
  set >set.out 2>&1
  [ -r $II_SYSTEM/ingres/version.rel ] && cp -p $II_SYSTEM/ingres/version.rel .
  [ -r $II_SYSTEM/ingres/version.dat ] && cp -p $II_SYSTEM/ingres/version.dat .
  (cd $II_SYSTEM/ingres && ls -lR bin utility lib files) >ls.out 2>&1
}


copy_dbmslog()
{
[ -z "$1" ] && {
  return
} || {
  dbmslog=`echo $1 | sed -e 's/%p/*/'`
}
cp -p $dbmslog .
}
  

ingres_logfiles()
{
  echo "get ingres logfiles ..."
  cp -p $F/errlog.log $IIRCP $IIACP $F/rcpconfig.log .
  copy_dbmslog $DBMSLOG
  [ "$iicrashlog" = 1 -a -r $F/iicrash.log ] && cp -p $F/iicrash.log .
  [ -r $F/iivdb.log ] && cp -p $F/iivdb.log .
}

system_config()
{
  echo "get system configuration ..."
  $ULIMITSH ulimit -a >ulimit-a.out 2>&1
  $ULIMITSH ulimit -aH >ulimit-aH.out 2>&1
  uname -a >uname-a.out 2>&1
  case "$VERS" in
    int_lnx|amd_lnx|a64_lnx)
      /lib/libc.so.6 >libc_version.txt 2>&1
      lsb_release -a >lsb_release.txt 2>&1
      ;;
    rs4_us5|r64_us5)
      oslevel >osversion.txt 2>&1
      ;;
    axp_osf)
      /usr/sbin/sizer -v >osversion.txt 2>&1
      ;;
    su4_us5|su9_us5|a64_sol)
      /usr/bin/isainfo -b >isainfo-b.out
      ;;
  esac
  syscheck $SYSCHECKFLAG >syscheck.out 2>&1
  os_flavour=`uname -s`
  case "$os_flavour" in
    SunOS|UnixWare|UNIX_SV)
      /usr/sbin/psrinfo -v > processor.txt 2>&1
      /usr/sbin/prtconf > hardware.txt 2>&1
      ;;
    Darwin)
      system_profiler >hardware.txt 2>&1
      system_profiler -xml >hardware.spx 2>&1
      ;;
  esac


}

do_ps()
{
  date >ps.out.$1 2>&1
  $PSCMD >> ps.out.$1 2>&1
  [ -n "$PSCMD_2" ] && $PSCMD_2 >ps2.out.$1 2>&1
  [ -n "$PSCMD_3" ] && $PSCMD_3 >ps3.out.$1 2>&1
  [ -n "$PSCMD_4" ] && $PSCMD_4 >ps4.out.$1 2>&1
}

system_state()
{
  echo "get system state ..."
  echo "   ps ..."
  eval do_ps 1 $bg
  echo $! >.bg_systemstate
  echo "   ipcs ..."
  eval ipcs -a >ipcs-a.out 2>&1 $bg
  echo $! >>.bg_systemstate
  echo "   netstat  ..."
  eval netstat -an >netstat-an.out 2>&1 $bg
  echo $! >>.bg_systemstate
  eval netstat -a >netstat-a.out 2>&1 $bg
  echo $! >>.bg_systemstate
  eval netstat -i > netstat-i.out 2>&1 $bg
  echo $! >>.bg_systemstate
  eval netstat -nr > netstat-nr.out 2>&1 $bg
  echo $! >>.bg_systemstate
  eval netstat -s > netstat-s.out 2>&1 $bg
  echo $! >>.bg_systemstate

  case "$VERS" in
    int_lnx|amd_lnx|a64_lnx)
      echo "   netstat -ap ..."
      eval netstat -ap >netstat-ap.out 2>/dev/null $bg
      echo $! >>.bg_systemstate
      ;;
    su4_us5|su9_us5|a64_sol)
      eval prstat -L -n100000 1 3 >prstat.log 2>&1 $bg
      echo $! >>.bg_systemstate
      ;;
  esac;
  eval $DF >df.out 2>&1 $bg
  echo $! >>.bg_systemstate
  eval vmstat 2 5 > vmstat.out 2>&1 $bg
  echo $! >>.bg_systemstate
  [ -n "$TOP" ] && {
     eval $TOP $bg
     echo $! >>.bg_systemstate
  }
  uptime > uptime.out 2>&1

}

do_logstat()
{
  echo "   logstat ..."
  logstat >logstat.out 2>&1
  [ $ING_RELEASE = II ] && {
     echo "   logstat -verbose ..."
     logstat -verbose >logstat_verbose.out 2>&1
  }
}


do_lockstat()
{
  [ $ING_RELEASE = II ] && {
     echo "   lockstat -dirty ..."
     lockstat -dirty >lockstat_dirty.out 2>&1
  }
  echo "   lockstat ..."
  lockstat >lockstat.out 2>&1
}

do_iimonitor1()
{
    port="$1"
    count="$2"
    namepart=`basename $port`
    echo "   iimonitor $port(show $IIMONALL sessions) ..."
    add_timestamp "START" >iimon_ses_$namepart.out.$count 2>&1
    echo "show $IIMONALL sessions" | iimonitor $port >>iimon_ses_$namepart.out.$count 2>&1
    add_timestamp "END" >>iimon_ses_$namepart.out.$count 2>&1
    echo "   iimonitor $port(show $IIMONALL sessions formatted) ..."
    add_timestamp "START" >iimon_fmt_$namepart.out.$count 2>&1
    echo "show $IIMONALL sessions formatted" | iimonitor $port >>iimon_fmt_$namepart.out.$count 2>&1
    add_timestamp "END" >>iimon_fmt_$namepart.out.$count 2>&1
    [ "$ingstacks" = 1 ] && {
      echo "   iimonitor $port(stacks) ..."
      add_timestamp "START" >iimon_stacks_$namepart.out.$count 2>&1 
      echo stacks | iimonitor $port >>iimon_stacks_$namepart.out.$count 2>&1
      add_timestamp "END" >>iimon_stacks_$namepart.out.$count 2>&1 
    }
    echo "   iimonitor $port(looking for mutex) ..."
    for i in 1 2 3 4 5
    do 
      echo "$i ------------------------------" >>mutexes_$namepart.out.$count 2>&1
      add_timestamp START >> mutexes_$namepart.out.$count 2>&1
      grep -i 'mutex[: ].*((x)' iimon_fmt_$namepart.out.$count \
           | sed -e 's/.*((x) \(.*\)).*/show mutex \1/' \
           | sort -u \
           | iimonitor $port >>mutexes_$namepart.out.$count 2>&1
      add_timestamp END >> mutexes_$namepart.out.$count 2>&1
    done
    echo "   iimonitor $port(show server) ..."
    add_timestamp "START" >iimon_server_$namepart.out.$count 2>&1
    (echo show server;echo show smstats) | iimonitor $port >>iimon_server_$namepart.out.$count 2>&1
    add_timestamp "END" >>iimon_server_$namepart.out.$count 2>&1
}

do_iimonitor()
{
   do_iimonitor1 $1 1
   sleep 10
   do_iimonitor1 $1 2
}

do_iimonitor1_other()
{
    port="$1"
    count="$2"
    namepart=`basename $port`
    echo "   iimonitor $port (gcc/das) "
    add_timestamp START >iimon_other_$namepart.out.$count 2>&1
    iimonitor $port >>iimon_other_$namepart.out.$count 2>&1 <<IIMONOTHER
show server
show all sessions formatted
IIMONOTHER
    add_timestamp END >>iimon_other_$namepart.out.$count 2>&1
}

do_iimonitor_other()
{
   do_iimonitor1_other $1 1
   sleep 10
   do_iimonitor1_other $1 2
}

do_iinamu()
{
  echo "   querying nameserver ..."
  [ -f "$F/name/svrclass.nam" ] && {
#   i.e. >= II 2.5
    sed -e '/^#/ d' -e 's/[	 ].*//'  -e 's/^/show /' \
      $F/name/svrclass.nam \
      | iinamu >iinamu.out 2>&1
  } || {
    grep 'transient$' $F/name/iiname.all \
     | sed -e 's/[ 	].*//' -e 's/^/show /' \
     | iinamu  >iinamu.out 2>&1
  }
}

get_pstack_gdb()
{
  [ -z "$1" ] && return || xpid=$1
  xcmd=`get_exec $xpid`
  [ -z "$xcmd" ] && return 
  echo $xcmd $xpid
#on OSX gdb does not work with a here document, so we use cat and pipe to gdb
#we do this only for OSX, since this solution doesn't work om Linux
  case "$VERS" in
    *_osx)
       cat  <<GDBEND | $GDB $xcmd $xpid
info threads
thread apply all bt
detach
quit
GDBEND
       ;;
    *)
       $GDB $xcmd $xpid <<GDBEND
info threads
thread apply all bt
detach
quit
GDBEND
       ;;
  esac
}

get_pstack_ladebug()
{
  [ -z "$1" ] && return || xpid=$1
  xcmd=`get_exec $xpid`
  [ -z "$xcmd" ] && return
  echo $xcmd $xpid
  ladebug <<LADEBUGEND
set \$stoponattach=1
attach $xpid $xcmd
where thread all
detach
quit
LADEBUGEND
}

get_pstack_dbxaix()
{
  [ -z "$1" ] && return || xpid=$1
  xcmd=`get_exec $xpid`
  [ -z "$xcmd" ] && return
  echo $xcmd $xpid
  dbx -a $xpid $xcmd <<DBXAIXEND
thread >! thread_info_short.$xpid.$2.txt
thread - >! thread_info_long.$xpid.$2.txt
sh  grep '^.\\$' thread_info_short.$xpid.$2.txt | sed -e 's/[ >]\$t//' -e 's/ .*//' -e 's/.*/thread current &;thread info &;where/' > show_threads_tmp.$2.$xpid
source show_threads_tmp.$2.$xpid
detach
DBXAIXEND
}

get_pstacks_pfiles()
{
#if pstack (or similar) command is available use that, otherwise use debugger
#it may make sense to use both(on AIX 5.2 and below, procstack doesn't work
#very well)
  [ -z "$STACKCMD" -a -z "$STACKDEBUGCMD" -a -z "$FILESCMD" ] && return
  ipids=`ingres_pids`
  otheripids=`other_pids "iigcn|iigcc|iigcb|iijdbc|iigcd"`
  for i in 1 2
  do
    for pid in $ipids $otheripids
    do
        [ -n "$STACKCMD" ] && {
              add_timestamp START >pstack_$pid.$i.out 2>&1
              $STACKCMD $pid >>pstack_$pid.$i.out 2>&1
              add_timestamp END >>pstack_$pid.$i.out 2>&1
        }
        [ -n "$STACKDEBUGCMD" ] && {
            echo "   Debug $pid"
            add_timestamp START >pstackdebug_$pid.$i.out 2>&1
            $STACKDEBUGCMD $pid $i >>pstackdebug_$pid.$i.out 2>&1
            add_timestamp END >>pstackdebug_$pid.$i.out 2>&1
        }
    done
    sleep 1
  done
#get the open files, this may conflict with getting stacks, so we don't do this 
#in parallel
  [ -z "$FILESCMD" ] && return
  for pid in $ipids $otheripids
  do
     $FILESCMD $pid >pfiles_$pid.out 2>&1
  done
}

inglogs_showmemkey()
{
   for mem in *
   do
     echo $mem: `iimemkey $mem`
   done
}
ingres_state()
{
  echo "collecting ingres state ..."
  eval csreport >csreport.out  2>&1 $bg
  echo $! >.bg_ingresstate
# wait a few seconds, hoping that csreport has completed
  sleep 5
  eval get_pstacks_pfiles $bg
  echo $! >>.bg_ingresstate
  eval do_logstat $bg
  echo $! >>.bg_ingresstate
  eval do_lockstat $bg
  echo $! >>.bg_ingresstate
# Get the connectid's from csreport instead from the name server
# fails when using UNIX sockets and II_GC_PORT contains a comma
  ports=`ingres_ports`
  echo $ports >.ports
  for p in $ports 
  do
    eval do_iimonitor "$p" $bg
    echo $! >>.bg_ingresstate
  done
  eval $II_SYSTEM/ingres/bin/infodb > infodb.log 2>&1 $bg
  echo $! >>.bg_ingresstate
  eval do_iinamu $bg
  echo $! >>.bg_ingresstate
  eval repstat >repstat.log 2>&1 $bg
  echo $! >>.bg_ingresstate
  (cd `ingprenv II_WORK`/ingres/work/default && ls -lR) >ls_iiwork.out 2>&1
  (cd $II_SYSTEM/ingres/files/memory && inglogs_showmemkey ) > memkey.out 2>&1
# sleep 5 seconds hoping that this is enough for iinamu
# then try iimonitor against the iigcd and iigcc processes
# fails if release <9.1, so don't do it in this case.
  if [ "$othermon" = 1 ]
  then
    sleep 5
    otherports=`other_ports DASVR`
    otherports2=`other_ports COMSVR`
    otherports="$otherports $otherports2"
    for p in $otherports
    do
      eval do_iimonitor_other "$p" $bg
      echo $! >>.bg_ingresstate
    done
  fi
##### 
  [ "$bg" = "&" ] && wait
}

clean_up()
{
echo clean_up
   BG2=`cat .bg_systemstate`
   BG3=`cat .bg_ingresstate`
   BGALL=`echo $BG1 $BG2 $BG3`
   for i in $BGALL 
   do
      kill $i 2>/dev/null
   done
}

clean_up2()
{
echo clean_up2
   BG4=`cat .bg_iimon 2>/dev/null`
   for i in $BG4 
   do
      kill $i 2>/dev/null
   done
   exit
}

check_iimonitor()
{
#if $ports is not empty, assume that iimonitor worked and exit
echo check_iimonitor...

  ports=`cat .ports`
  if [ "$ingstate" = 1  -a -z "$ports" ] 
  then
     echo "iimonitor output not collected yet. Trying again"
     echo "if you don't want to do this, interrupt again".
     ports=`cat csreport.out | $AWK -F"[ ,]" '/inuse [^0]/ {print $9}'` 
     [ -z "$ports" ] && {
         echo "no ports found. Trying to get info from nameserver"
#ignoring recovery and non /ingres servers for the moment
#will adress this later
         ports=`$IINAMUCMD iidbms`
     }
     for p in $ports
     do
        eval do_iimonitor "$p" $bg
        echo $! >>.bg_iimon
     done
  fi
}

sig()
{
  sig=1
  echo "interrupting..."
}


inglogs_set_flags()
{
while [ -n "$1" ]
do
   case "$1" in
     -help|-h)
         show_usage
         exit
         ;;
     --help)
         show_help
         exit
         ;;
     --version|-version|-v)
         echo $INGLOGS_VERSION
         exit
         ;;
     -noparallel)  
         bg=""
         ;;
     -nosystemconfig)
         systemconfig=0
         ;;
     -nosystemstate)
         systemstate=0
         ;;
     -nologfiles)
         inglogfiles=0
         ;;        
     -noingresconfig)
         ingconfig=0
         ;;
     -noingresstate)
         ingstate=0
         ;;
     -internalthreads)  
         ingstacks=1
         ;;
     +stackdebug)
         stackdebug=1
         ;;
     +iicrashlog)
         iicrashlog=1
         ;;
     *)
         echo "Invalid flag: $1"
         show_usage
         exit
         ;;
   esac
   shift
done
}

###START
flags="$*"
#defaults:
bg="&"
systemconfig=1
systemstate=1
inglogfiles=1
ingconfig=1
ingstate=1
ingstacks=0
stackdebug=0
iicrashlog=0
sig=0

#we now set the flags in inglogs_set_flags, making it easy to add $INGLOGSFLAGS
inglogs_set_flags $* $INGLOGSFLAGS
inglogs_init
echo "LOGDIR: $LOGDIR"
[ -n "$bg" ] && trap sig INT
[ "$systemconfig" = 1 ] && {
   eval system_config $bg
   BG1=$!
}
[ "$systemstate" = 1 ] && {
   eval system_state $bg
   BG1="$BG $!"
}
[ "$ingconfig" = 1 ] && {
   eval ingres_config $bg
   BG1="$BG $!"
}
[ "$ingstate" = 1 ] && {
   eval ingres_state $bg
   BG1="$BG $!"
}
[ "$inglogfiles" = 1 ] && {
   eval ingres_logfiles $bg
   BG1="$BG $!"
}

[ "$systemstate" = 1 ] && {
   echo sleeping 10 seconds ...
   sleep 10
   do_ps 2
}

[ "$bg" = "&" ] && {
   sleep 2
   echo "WAITING for the background processes to be completed ...."
   wait
   if [ "$sig" = 1 ] 
   then
      sig=0
      clean_up
      check_iimonitor
      wait
      [ "$sig" = 1 ] && clean_up2
   fi
   sleep 2
   echo "DONE."
####   echo "you may have to hit return to see that the script has completed."
####   sleep 5
}
echo "LOGDIR: $LOGDIR"
date +%Y%m%d_%H%M%S >inglogs_completed.txt
