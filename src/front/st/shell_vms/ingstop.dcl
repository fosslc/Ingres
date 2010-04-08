$!========================================================================
$!
$!  Copyright (c) 2004 Ingres Corporation
$! 
$!  Name:  
$!      iisdall.com -- shut down all Ingres processes 
$! 
$!  Usage: 
$!      @iisdall
$! 
$!  Description:
$!
$!      This program is invoked as "ingstop". 
$!
$!!  History:
$!!
$!!	10-9-1998  (carsu07)
$!!		Added the -force and -f flags so that ingstop can now force
$!!              the DBMS servers to shutdown via iimonitor "stop server".  
$!!	05-Oct-1998 (kinte01)
$!!			Shutdown RMCMD server
$!!	17-Feb-1999 (kinte01)
$!!			The check for the RMCMD server process id
$!!			was looking for the wrong number of "-". This
$!!			would cause problems when stopping a System level
$!!			installation if a system level and a group level
$!!			installation of Ingres were both running a
$!!			RMCMD server (bug 95375).
$!!	12-May-1999 (chash01)
$!!		Add stopping gateways
$!!	01-Jul-1999 (kinte01)
$!!		Add above comment and modify the gateway shutdown so it
$!!		is only executed if a Gateway process is running otherwise
$!!		there will be errors.
$!!	09-Aug-1999 (kinte01)
$!!		Add shutdown for RMS gateway (98235).
$!!	23-Mar-2000 (devjo01)
$!!		Add node name to scratch file for simultaneous shutdown
$!!		of multiple clustered Ingres nodes.
$!!      14-Aug-2000 (horda03)
$!!              Stop DBMS servers with server classes other than Ingres
$!!              (102064)
$!!      26-Sep-2000 (horda03)
$!!              Fix typo with above bug fix F$FAO directives are uppercase.
$!!      10-Nov-2000 (horda03) Bug 103169
$!!              For some reason, we never tried stopping the GCB servers.
$!!      28-Dec-2000 (horda03) Bug 86579
$!!              Remove the Shared memory files following successful shutdown
$!!              of Ingres.
$!!      29-mar-2001 (chash01)
$!!              Shutting down relational gateways returns error message from
$!!              DCL "conext frozen".  The cause may well be in iigwstop 
$!!              program.  Before we can fix it, we can avoid this error
$!!              message by changing ctx to ctx1 in STOP_RELGW_SERVERS
$!!      17-aug-2001 (horda03)
$!!              Remove installed images. Bug 105541
$!!	13-Mar-2001 (kinte01)
$!!		Add shutdown for JDBC servers
$!!		Use iigcstop to shutdown net, bridge, & jdbc servers
$!!	05-Nov-2001 (kinte01)
$!!		Increase minimum VMS version -- now 7.3
$!!		Remove code for VAX specific version checking.  VAX no longer
$!!		supported
$!!	05-nov-2003 (kinte01)
$!!	    Remove dmfcsp as there is no separate DMFCSP process with 
$!!	    the Portable Clustered Ingres project
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	05-apr-2005 (ashco01) INGSRV3234, Bug 114235.
$!!	    Allow this release to be run on ALPHA OpenVMS versions 7
$!!	    or greater (actually, 7.3 or greater).
$!!     17-May-2005 (nansa02)
$!!             Added support for Data Access Server(DAS).   
$!!     20-May-2005 (bolke01)
$!!             Added the detection of Node specific memory files.
$!!	31-may-2005 (devjo01)
$!!		Remove debug display of memory files.
$!!	23-sep-2005 (devjo01) Bug 115259
$!!		Correct misspelling of "Access" as "Acess" in informational
$!!	        message kicked out when shutting down DAS (iigcn) b
$!!	08-nov-2005 (devjo01) b101019
$!!		Scratch file classes.tmp needs to have its name made
$!!		unique within the cluster.
$!!	12-Feb-2007 (bonro01)
$!!		Remove JDBC package.
$!!	26-Feb-2008 (upake01) b120023
$!!		If "rmcmdstp" does not stop RMCMD process, kill the
$!!             process using "KILL_PROCESS" subroutine.  Normally
$!!             this happens when there are no DBMS processes running and
$!!             "rmdcmdstp" can't stop RMCMD process since it cannot access
$!!             the database.  This means time to kill the process.
$!!             Before kiliing the "rmcmd" process, make sure that the "DBMS" 
$!!             server is down.  We want to kill "rmcmd" only if the "dbms" is down.
$!!     07-Mar-2008 (horda03) Bug 120047
$!!             Wait for the RCP server process to terminate before attempting to
$!!             stop the GCN server. After the RCP has turned the Logging system
$!!             offline, it still has to de-register from the Name server. There is
$!!             a window where the RCP server tries to deregister from the GCN
$!!             while the GCN is terminated; this leaves the RCP in a hung state.
$!!     12-Mar-2008 (upake01) Bug 120047
$!!             For the RCP server process, call "DEATH_WATCH_LOOP" only if the
$!!             pid symbol has a value - confirming that the process is running and
$!!             we need to wait until it exits.
$!!             When checking for existence of the DBMS server before killing RMCMD,
$!!             check for the local node only as it is okay to have the ingres instance
$!!             up and running on other nodes in the cluster.
$!!     13-Mar-2008 (upake01) Bug 120047
$!!             EXIT from "Death_Watch_Loop" if the process does not exist (i.e.
$!!             symbol pid contains nothing).  Just a safeguard in case we end up
$!!             getting to "Death Watch" for a non-existent process.
$!!     18-Mar-2008 (upake01) Bug 120123
$!!             Modify logic in "F$CONTEXT" section of the code to
$!!             (a) Add "_" when checking for a process for group level
$!!                 installations as they are nameed "II_ppp_KK_xxxx", where
$!!                 "ppp" is the process abbreviation, "KK" is the group iunstallation 
$!!                 code and "xxxx" is the last four hexadecimal digits of the process
$!!                 id with leading zeroes removed (e.g. II_DBMS_KU_401 or II_IUSV_KU_DEF0).
$!!             (b) Doing "f$context" for a system-level installation may
$!!                 return processes for other group level installation.
$!!                 Add logic to go through the process IDs and skip all
$!!                 group installations by checking the process name.
$!!     25-Mar-2008 (upake01 & horda03) Bug 120047
$!!             INGSTOP hangs (with waiting for the recover server to exit)
$!!             when run simultaneously from two different sessions.
$!!             INGSTOP from session 1 issues "rcpstat -online" and
$!!             sees that the logging system is "online", so issues the
$!!             "rcpconfig -shutdown".
$!!             INGSTOP from session 2 issues "rcpstat -online" and
$!!             sees that the logging system is "offline", so checks
$!!             and stops the GCN.
$!!             If rcpstat fails, don't assume there is no RCP server.
$!!             Indicate the INGSTOP did not shutdown the RCP, then
$!!             verify that the IUSV server does not exist.
$!!             When IUSV server (for the installation) has expired, output
$!!             the "The recovery server and archiver process have exited"
$!!             message only if the ingstop command was the one that issued 
$!!             the rcp shutdown instruction.
$!!             Tidy up 2-3 "if-endif" in the same block that existed one
$!!             after the other for the same condition into one "if-endif".
$!=========================================================================
$!
$! Check VMS version
$!
$ vermajor = f$extract( 1, 1, f$getsyi( "version" ) )
$ verminor = f$extract( 3, 1, f$getsyi( "version" ) )
$!
$! Check VMS Architecture against version
$!
$!    Process Alpha Open VMS
$ if (vermajor .eq. 7) .and. (verminor .ge. 3) then goto VERSION_OK
$ if vermajor .ge. 8 then goto VERSION_OK
$ type sys$input
 --------------------------------------------------------------------------------
|                      ************* ERROR *************                         |
| You must be running Alpha OpenVMS 7.3 or higher to use this shutdown procedure |
|                      *********************************                         |
 --------------------------------------------------------------------------------
$ goto NORMAL_EXIT
$!
$ VERSION_OK:
$!
$! Define local symbols
$!
$ echo		:= write sys$output
$ say		:= type sys$input
$ space		:= " "	! this is wierd 
$ silent_watch  = 0
$!
$ saved_message_format	= f$environment( "MESSAGE" ) ! save message format
$ on error then goto GENERIC_ERROR_HANDLER
$ askuser="FALSE" ! ### REMOVE askuser LATER
$!
$! Initialize active process counters. 
$!
$ rmcmd_count = 0
$ star_count = 0
$ rms_count = 0
$ gcc_count  = 0
$ gcb_count  = 0
$ dbms_count = 0
$ gcn_count  = 0
$ gcd_count  = 0
$!
$ right_one = "Y"	! In case II_INSTALLATION is defined
$!			  This gets changed if user answers N later...
$!
$ all_gone := "%SYSTEM-W-NONEXPR, nonexistent process"
$!			error message you get when looking for
$!			a process that has gone away...
$!
$ on warning then continue
$!
$! Define required symbols
$!
$ iigetres	:= $ii_system:[ingres.utility]iigetres.exe 
$ iimonitor	:= $ii_system:[ingres.bin]iimonitor.exe 
$ iinamu	:= $ii_system:[ingres.bin]iinamu.exe
$ iishare       := @ii_system:[ingres.utility]iishare.com
$ iigcstop	:= $ii_system:[ingres.bin]iigcstop.exe
$ rcpconfig 	:= $ii_system:[ingres.bin]rcpconfig.exe
$ rcpstat	:= $ii_system:[ingres.bin]rcpstat.exe
$ rmcmdstp      := $ii_system:[ingres.bin]rmcmdstp.exe
$!
$! Names of temporary files
$!
$! Need to be unique per node, so decorate with node name.
$ node_name = f$getsyi( "NODENAME" )
$ iimonitor_in  := "ii_system:[ingres]iimonitor.''node_name'_in"
$ iimonitor_frc  := "ii_system:[ingres]iimonitor.''node_name'_frc"
$ ingstop_out   := "ii_system:[ingres]ingstop.''node_name'_out"
$ classes_tmp := "ii_system:[ingres]classes.''node_name'_tmp"
$!
$! Check for necessary privileges.
$!
$ iigetres ii.'node_name.ingstop.privileges ingstop_privs
$ ingstop_privs = f$trnlnm( "ingstop_privs" )
$ deassign "ingstop_privs"
$ if( f$privilege( "''ingstop_privs'" ) ) then goto START
$ setpriv = f$setprv( "''ingstop_privs'" )
$ if( .not. f$privilege( "''ingstop_privs'" ) )
$ then
$    echo "" 
$    echo "You must have the following privileges to shut down Ingres:
$    echo "" 
$    echo "''ingstop_privs'"
$    echo ""
$    goto NORMAL_EXIT
$ endif
$!
$ START:
$!
$! If II_INSTALLATION is not defined, it's a system-level installation
$! so look for process names alone; if defined, find process names which
$! include the installation code. 
$!
$ ii_installation = f$trnlnm( "II_INSTALLATION" )
$ echo ""
$!
$ gosub INIT_FILES
$!
$ gosub STOP_RMCMD_SERVERS
$!
$ gosub STOP_RELGW_SERVERS
$!
$ gosub STOP_STAR_SERVERS
$!
$ gosub STOP_RMS_SERVERS
$!
$ gosub STOP_DBMS_SERVERS
$!
$ gosub STOP_NET_SERVERS
$!
$ gosub STOP_BRIDGE_SERVERS
$!
$ gosub STOP_DAS_SERVER
$!
$ gosub STOP_LOGGING
$!
$ gosub STOP_NAME_SERVER
$!
$!! When runing in CLuster mode, memory files are placed in
$!! Node specific Direectories. Once created these deirecories remain.
$!
$ CONFIG_HOST    = f$getsyi( "NODENAME" )
$ iigetres ii.'CONFIG_HOST'.config.cluster_mode LCL_CLUSTER_MODE
$ cf_cluster_mode = f$edit(F$TRNLNM("LCL_CLUSTER_MODE"),"LOWERCASE")
$!
$ if cf_cluster_mode .eqs. "on"
$ then 
$    iigetres ii.'CONFIG_HOST'.lnm.ii_gcn'II_INSTALLATION'_lcl_vnode LCL_NODE
$    cf_host = F$TRNLNM("LCL_NODE")
$    cf_host = ".''cf_host'"
$ else
$    cf_host = ""
$ endif
$!
$ say
Cleaning up shared memory.

$ if f$search("ii_system:[ingres.files.memory''cf_host']*.*") .NES. "" then -
              delete/noconfirm ii_system:[ingres.files.memory'cf_host']*.*;*/exc=*.dir
$!
$ iishare delete
$!
$ goto NORMAL_EXIT 
$!
$ STOP_LOGGING:
$!
$ on error then goto STOP_LOGGING_EXIT
$ done_rcp_shutdown=0
$ rcpstat -online -silent
$ on error then goto GENERIC_ERROR_HANDLER
$ say
Shutting down the recovery server and archiver process...

$ define/user sys$output 'ingstop_out
$ rcpconfig -shutdown
$ done_rcp_shutdown=1
$ say
Waiting for processes to exit...
$ !
$ STOP_LOGGING_WAIT:
$!
$ wait 00:00:05
$ on error then goto STOP_LOGGING_EXIT
$ rcpstat -online -silent
$ goto STOP_LOGGING_WAIT
$!
$ STOP_LOGGING_EXIT:
$ on error then goto GENERIC_ERROR_HANDLER
$
$ if ii_installation .nes "" 
$ then
$    dummy = f$context( "PROCESS", ctx_iusv, "PRCNAM", -
        "II_IUSV_''ii_installation'_*", "EQL" )
$ else
$    dummy = f$context( "PROCESS", ctx_iusv, "PRCNAM", "II_IUSV_*", "EQL" )
$ endif
$
$ STOP_LOGGING_LOOP:
$ pid = f$pid( ctx_iusv )
$
$ if (pid .NES. "")
$ then
$    if ii_installation .eqs. ""
$    then
$       proc_nm = f$getjpi(pid, "PRCNAM")
$       if proc_nm .eqs. "" then goto STOP_LOGGING_DONE
$       if ( f$element( 3, "_", proc_nm ) .nes. "_" ) 
$       then
$          goto STOP_LOGGING_LOOP
$       endif
$    endif
$    silent_watch = 1
$    GOSUB DEATH_WATCH_LOOP
$    silent_watch = 0
$ endif
$
$ if done_rcp_shutdown .eq. 1
$ then
$    say

The recovery server and archiver process have exited.

$ endif
$ goto STOP_LOGGING_DONE
$!
$ LOGGING_OFF_LINE:
$!
$ say
There is no recovery server (dmfrcp) running.

There is no archiver process (dmfacp) running.

$!
$ STOP_LOGGING_DONE:
$!
$ return
$!
$ DEATH_WATCH:
$!
$ echo "Waiting for process to exit..."
$!
$ DEATH_WATCH_LOOP:
$!
$ if (pid .eqs. "")
$ then
$    return
$ endif
$ wait 00:00:01
$ on warning then goto DEATH_RATTLE
$ set message/notext/nosev/nofac/noid
$ proc_state := 'f$getjpi( pid, "STATE" )
$ set message 'saved_message_format'
$ goto DEATH_WATCH_LOOP
$!
$ DEATH_RATTLE: 
$!
$ if f$message( '$status' ) .nes. all_gone then -
     goto GENERIC_ERROR_HANDLER
$
$ if silent_watch then return
$
$ echo ""
$ echo "Process ''procname' has exited."
$ echo ""
$ return
$!
$ KILL_PROCESS:
$!
$ echo "Killing process... "
$ !set message/fac/sever/id/text
$ stop 'procname
$ set message 'saved_message_format'
$ echo ""
$ return
$!
$ INIT_FILES:
$!
$!  Create input file used by iimonitor to shutdown servers
$!
$ open/write tfile 'iimonitor_in
$ write tfile "show sessions"
$ write tfile "set server shut"
$ write tfile "quit"
$ close tfile
$!
$ open/write tfile 'iimonitor_frc
$ write tfile "show sessions"
$ write tfile "stop server"
$ write tfile "quit"
$ close tfile
$ return
$!
$ STOP_RMCMD_SERVERS:
$!
$ if ii_installation .nes. ""
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "RMCMD_''ii_installation'_*", "EQL" )
$ else
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", "RMCMD_*", "EQL" )
$ endif
$!
$ RMCMD_LOOP:
$!
$ pid = f$pid( ctx )
$!      Get the process-id from the context
$ if pid .eqs. "" then -
     goto END_RMCMD_LOOP
$ procname = f$getjpi( pid, "PRCNAM" )
$!
$! If the system-level installation is being shut down, avoid PIDs
$! associated with group-level installations...
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 2, "_", procname ) .nes. "_" ) )
$ then
     goto RMCMD_LOOP
$ endif
$ rmcmd_count = rmcmd_count + 1
$ echo "Shutting down RMCMD server ", procname, "..."
$ on error then goto RMCMD_ERROR_HANDLER
$!
$ rmcmdstp
$ echo ""
$!
$!   If DBMS is down, kill the "rmcmd" process
$ dbms_is_down = "TRUE"
$ dctx = ""
$ if (ii_installation .NES. "")
$ then
$    dummy_ctx = F$CONTEXT("PROCESS", dctx, "PRCNAM", "II_DBMS_''ii_installation'_*", "EQL")
$ else
$    dummy_ctx = F$CONTEXT("PROCESS", dctx, "PRCNAM", "II_DBMS_*", "EQL")
$ endif
$
$ DBMS_PROCESS_LOOP:
$ proc_pid = f$pid(dctx)
$ if (proc_pid .nes. "")
$ then
$    if ii_installation .eqs. ""
$    then
$       proc_nm = f$getjpi(pid, "PRCNAM")
$       if proc_nm .eqs. "" then goto DBMS_PROCESS_LOOP
$       if ( f$element( 3, "_", proc_nm ) .eqs. "_" ) 
$       then
$          dbms_is_down = "FALSE"
$       else
$          goto DBMS_PROCESS_LOOP
$       endif
$    else
$       dbms_is_down = "FALSE"
$    endif
$ endif
$
$ if (dbms_is_down .eqs. "TRUE")
$ then
$    echo "DBMS Server is down.  Killing RMCMD process ", procname, "..."
$    gosub KILL_PROCESS
$ endif
$ goto RMCMD_LOOP
$!
$!
$!
$ END_RMCMD_LOOP:
$!
$ if rmcmd_count .eq. 0
$ then
$    say
There are no RMCMD servers (rmcmd) running.

$ endif
$ goto STOP_RMCMD_DONE
$!
$ RMCMD_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ echo ""
$ echo "Unable to shutdown RMCMD server ''procname'"
$ echo ""
$ goto FAILURE_EXIT
$!
$ STOP_RMCMD_DONE:
$!
$ return
$!
$! stop all relational gateway listener
$!
$ STOP_RELGW_SERVERS:
$!
$ if ii_installation .nes ""
$ then
$    dummy = f$context( "PROCESS", ctx1, "PRCNAM", -
        "II_ORAC_''ii_installation'*", "EQL" )
$    dummy2 = f$context( "PROCESS", ctx2, "PRCNAM", -
        "II_RDB_''ii_installation'*", "EQL" )
$ else
$    dummy = f$context( "PROCESS", ctx1, "PRCNAM", "II_ORAC_*", "EQL" )
$    dummy2 = f$context( "PROCESS", ctx2, "PRCNAM", "II_RDB_*", "EQL" )
$ endif
$ RELGW_LOOP:
$!
$ pid = f$pid( ctx1 )
$ pid2 = f$pid( ctx2 )
$!      Get the process-id from the context
$ if pid .eqs. "" .and. pid2 .eqs. "" then -
     goto END_RELGW_LOOP
$ iigwstop
$ END_RELGW_LOOP:
$ return
$!
$ STOP_STAR_SERVERS:
$!
$ if ii_installation .nes "" 
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "II_STAR_''ii_installation'_*", "EQL" )
$ else
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", "II_STAR_*", "EQL" )
$ endif
$!
$ STAR_LOOP:
$!
$ pid = f$pid( ctx )
$!	Get the process-id from the context
$ if pid .eqs. "" then -
     goto END_STAR_LOOP
$ procname = f$getjpi( pid, "PRCNAM" )
$!
$! If the system-level installation is being shut down, avoid PIDs
$! associated with group-level installations...
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 3, "_", procname ) .nes. "_" ) )
$ then
     goto STAR_LOOP
$ endif
$ star_count = star_count + 1
$ echo "Shutting down Star server ", procname, "..."
$ on error then goto STAR_ERROR_HANDLER
$ define/user sys$output 'ingstop_out 
$ define/user sys$input 'iimonitor_in
$ iimonitor "''procname'"
$ echo ""
$ gosub DEATH_WATCH
$ goto STAR_LOOP
$!
$ END_STAR_LOOP:
$!
$ if star_count .eq. 0
$ then
$    say
There are no Star servers (iistar) running.

$ endif
$ goto STOP_STAR_DONE
$!
$ STAR_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ echo ""
$ type 'ingstop_out
$ echo ""
$ echo "Unable to shutdown Star server ''procname'"
$ echo ""
$ goto FAILURE_EXIT 
$!
$ STOP_STAR_DONE:
$!
$ return
$!
$ STOP_RMS_SERVERS:
$!
$ if ii_installation .nes "" 
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "II_RMS_''ii_installation'_*", "EQL" )
$ else
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", "II_RMS_*", "EQL" )
$ endif
$!
$ RMS_LOOP:
$!
$ pid = f$pid( ctx )
$ if pid .eqs. "" then -
     goto END_RMS_LOOP
$ procname = f$getjpi( pid, "PRCNAM" )
$!
$! Skip PIDs for group-level installation process if stopping the
$! system-level installation.
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 3, "_", procname ) .nes. "_" ) )
$ then
$       goto RMS_LOOP
$ endif
$ RMS_count = RMS_count + 1
$ echo "Shutting down RMS server ", procname, "..."
$ on error then goto RMS_ERROR_HANDLER
$ define/user sys$output 'ingstop_out
$ if ( p1 .eqs. "-FORCE" .or. p1 .eqs. "-F")
$ then
$       define/user sys$input 'iimonitor_frc
$ else
$       define/user sys$input 'iimonitor_in
$ endif
$ iimonitor "''procname'"
$ echo ""
$!
$ gosub DEATH_WATCH
$!
$ goto RMS_LOOP
$!
$ END_RMS_LOOP:
$!
$ if RMS_count .eq. 0
$ then
$    say
There are no RMS servers (iiRMS) running.

$ endif
$!
$ goto STOP_RMS_EXIT
$!
$ RMS_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ echo ""
$ type 'ingstop_out
$ echo ""
$ echo "Unable to shutdown RMS server ''procname'"
$ echo ""
$ goto FAILURE_EXIT
$!
$ STOP_RMS_EXIT:
$!
$ return
$!
$ STOP_DBMS_SERVERS:
$
$! Determine the server classes
$
$ search/nowarn/out='classes_tmp'/match=and ii_config:config.dat dbms,server_class
$
$ class_list = "INGRES"
$
$ open/read/error=CLASSES_FIN classes 'classes_tmp'
$
$CLASSES_LOOP:
$ read/end_of_file=CLASSES_FIN /error=CLASSES_FIN classes class
$
$ new_class = f$edit(f$element(1, ":", class),"compress,trim,upcase")
$
$ if f$locate(new_class,class_list) .ne. f$length(class_list) then goto classes_loop
$
$ class_list = f$fao("!AS,!AS",class_list, new_class)
$
$ goto classes_loop
$
$CLASSES_FIN:
$ close classes
$
$ delete/noconfirm 'classes_tmp';*
$!
$ class_no = 0
$
$DBMS_CLASS_LOOP:
$
$ class = f$element(class_no, ",", class_list)
$
$ class_no = class_no + 1
$
$ if class .eqs. "," then goto END_DBMS_LOOP
$
$ if class .eqs. "INGRES" then class = "DBMS"
$
$ IF F$LENGTH( class ) .GT. 4 THEN class = F$EXTRACT(0, 4, class)
$
$ if ii_installation .nes "" 
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "II_''class'_''ii_installation'*", "EQL" )
$ else
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", "II_''class'_*", "EQL" )
$ endif
$!
$ DBMS_LOOP:
$!
$ pid = f$pid( ctx )
$ if pid .eqs. "" then -
     goto DBMS_CLASS_LOOP
$ procname = f$getjpi( pid, "PRCNAM" )
$!
$! Skip PIDs for group-level installation process if stopping the
$! system-level installation.
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 3, "_", procname ) .nes. "_" ) )
$ then
$       goto DBMS_LOOP
$ endif
$ dbms_count = dbms_count + 1
$ echo "Shutting down DBMS server ", procname, "..."
$ on error then goto DBMS_ERROR_HANDLER
$ define/user sys$output 'ingstop_out
$ if ( p1 .eqs. "-FORCE" .or. p1 .eqs. "-F")
$ then
$       define/user sys$input 'iimonitor_frc
$ else
$       define/user sys$input 'iimonitor_in
$ endif
$ iimonitor "''procname'"
$ echo ""
$!
$ gosub DEATH_WATCH
$!
$ goto DBMS_LOOP
$!
$ END_DBMS_LOOP:
$!
$ if dbms_count .eq. 0
$ then
$    say
There are no DBMS servers (iidbms) running.

$ endif
$!
$ goto STOP_DBMS_EXIT
$!
$ DBMS_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ echo ""
$ type 'ingstop_out
$ echo ""
$ echo "Unable to shutdown DBMS server ''procname'"
$ echo ""
$ goto FAILURE_EXIT
$!
$ STOP_DBMS_EXIT:
$!
$ return
$!
$ STOP_NET_SERVERS:
$!
$ if ii_installation .nes "" 
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "II_GCC_''ii_installation'_*", "EQL")
$ else
$    dummy = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_GCC_*", "EQL")
$ endif
$!
$ NET_LOOP:
$!
$ pid = f$pid(ctx)
$ if( pid .eqs. "" ) then goto END_NET_LOOP
$ procname = f$getjpi(pid,"PRCNAM")
$!
$! Skip PIDs for group-level installation process if stopping the
$! system-level installation.
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 3, "_", procname ) .nes. "_" ) )
$ then
$    goto NET_LOOP
$ endif
$ gcc_count = gcc_count + 1
$ echo "Shutting down Net server ", procname, "..."
$ iigcstop -q 'procname
$ echo ""
$!
$ gosub DEATH_WATCH
$!
$ goto NET_LOOP
$!
$ END_NET_LOOP:
$!
$ goto STOP_NET_EXIT
$!
$ NET_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ type 'ingstop_out
$ echo ""
$ echo "Unable to shutdown Net server ''procname'"
$ echo ""
$ goto FAILURE_EXIT
$!
$ STOP_NET_EXIT:
$!
$ if gcc_count .eq. 0
$ then
$    say
There are no Net servers (iigcc) running.

$ endif
$!
$ STOP_NET_EXIT:
$!
$ return
$!
$ STOP_BRIDGE_SERVERS:
$!
$ if ii_installation .nes ""
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "II_GCB_''ii_installation'_*", "EQL")
$ else
$    dummy = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_GCB_*", "EQL")
$ endif
$!
$ BRIDGE_LOOP:
$!
$ pid = f$pid(ctx)
$ if( pid .eqs. "" ) then goto END_BRIDGE_LOOP
$ procname = f$getjpi(pid,"PRCNAM")
$!
$! Skip PIDs for group-level installation process if stopping the
$! system-level installation.
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 3, "_", procname ) .nes. "_" ) )
$ then
$    goto BRIDGE_LOOP
$ endif
$ gcb_count = gcb_count + 1
$ echo "Shutting down Bridge server ", procname, "..."
$ iigcstop -q 'procname
$ echo ""
$!
$ gosub DEATH_WATCH
$!
$ goto BRIDGE_LOOP
$!
$ END_BRIDGE_LOOP:
$!
$ goto STOP_BRIDGE_EXIT
$!
$ BRIDGE_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ type 'ingstop_out
$ echo ""
$ echo "Unable to shutdown Bridge server ''procname'"
$ echo ""
$ delete/noconfirm netutil.cmd;*
$ goto FAILURE_EXIT
$!
$ STOP_BRIDGE_EXIT:
$!
$ if gcb_count .eq. 0
$ then
$    say
There are no Bridge servers (iigcb) running.

$ endif
$!
$ STOP_BRIDGE_EXIT:
$!
$ return
$!
$ STOP_DAS_SERVER:
$!
$ if ii_installation .nes ""
$ then
$    dummy = f$context( "PROCESS", ctx, "PRCNAM", -
        "II_DASV_''ii_installation'_*", "EQL")
$ else
$    dummy = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_DASV_*", "EQL")
$ endif
$!
$ DAS_LOOP:
$!
$ pid = f$pid(ctx)
$ if( pid .eqs. "" ) then goto END_DAS_LOOP
$ procname = f$getjpi(pid,"PRCNAM")
$!
$! Skip PIDs for group-level installation process if stopping the
$! system-level installation.
$!
$ if( ( ii_installation .eqs. "" ) .and. -
     ( f$element( 3, "_", procname ) .nes. "_" ) )
$ then
$    goto DAS_LOOP
$ endif
$ gcd_count = gcd_count + 1
$ echo "Shutting down Data Access server ", procname, "..."
$ iigcstop 'procname IIR
$ echo ""
$!
$ gosub DEATH_WATCH
$!
$ goto DAS_LOOP
$!
$ END_DAS_LOOP:
$!
$ goto STOP_DAS_EXIT
$!
$ DAS_ERROR_HANDLER:
$!
$ on error then goto GENERIC_ERROR_HANDLER
$ type 'ingstop_out
$ echo ""
$ echo "Unable to shutdown Data Access server ''procname'"
$ echo ""
$ delete/noconfirm netutil.cmd;*
$ goto FAILURE_EXIT
$!
$ STOP_DAS_EXIT:
$!
$ if gcd_count .eq. 0
$ then
$    say
There are no Data Access servers (iigcd) running.

$ endif
$!
$ STOP_DAS_EXIT:
$!
$ return
$!
$ STOP_NAME_SERVER:
$!
$ if ii_installation .nes "" 
$ then
$    dummy = F$CONTEXT("PROCESS", ctx, "PRCNAM", -
        "II_GCN_''ii_installation'", "EQL")
$ else
$    dummy = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_GCN", "EQL")
$ endif
$!
$ NAME_LOOP:
$!
$ pid = f$pid(ctx)
$ if pid .eqs. "" then -
$    goto END_NAME_LOOP
$ procname = f$getjpi( pid, "PRCNAM" )
$ gcn_count = gcn_count + 1
$ echo "Shutting down name server ", procname,"..."
$ define/user sys$output 'ingstop_out
$ iinamu
stop
yes
$ echo ""
$!
$ END_NAME_LOOP:
$!
$ if gcn_count .eq. 0
$ then
$    say
There is no name server (iigcn) running.

$ endif
$!
$ STOP_NAME_EXIT:
$!
$return
$!
$ CLEAN_UP_FILES:
$!
$ set message/notext/noid/nosev/nofac
$ delete/noconfirm 'ingstop_out;*
$ delete/noconfirm 'iimonitor_in;*
$ delete/noconfirm 'iimonitor_frc;*
$ delete/noconfirm 'netutil_in;*
$ set message 'saved_message_format'
$ return
$!
$ GENERIC_ERROR_HANDLER:
$!
$ echo f$message( $severity ), space, f$message( $status )
$ echo ""
$!
$ FAILURE_EXIT:
$!
$ gosub CLEAN_UP_FILES
$ exit
$ !exit 0
$!
$ NORMAL_EXIT:
$!
$ gosub CLEAN_UP_FILES
$ exit
$ !exit 1
