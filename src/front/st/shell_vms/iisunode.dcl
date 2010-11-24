$ test = 0
$ set NOON
$ GOTO _START_IISUNODE
$!
$!  Copyright (c) 2002, 2009 Ingres Corporation 
$! 
$! 
$!
$!  Name: iisunode -- Secondary Cluster Node setup script for Ingres DBMS
$!
$!  Usage:
$!	iisunode
$!
$!  Description:
$!
$!	This script is currently used in lieu of ingbuild when setting
$!	secondary nodes in an Ingres cluster.
$!
$!	Most of the heavy lifting was completed when the first node
$!	was set up, so in most cases all that the set up scripts
$!	invoked here will do, is copy parameters to the current node.
$!
$!  Pre-requisites
$!	
$!	II_SYSTEM	must be defined
$!	
$!	User Account	Must be the user that owns the ingres installation
$!	
$!	The installation must be able to support clustering.  I.e. the
$!	machine must be part of a cluster.
$!	
$!	
$!  Exit Status:
$!	0       setup procedure completed.
$!	<>0     setup procedure did not complete.
$!
$!  DEST = utility
$!! History:
$!!	12-Feb-2002 (bolke01)
$!!		Created.	
$!!             Initial version.
$!!             Based on the UNIX version iimkcluster.sh
$!!	08-jun-2005 (abbjo03)
$!!	    Add temporary defines for II_COMPATLIB and II_LIBQLIB so that we
$!!	    may run prior to being fully configured.
$!!	09-jun-2005 (bolke01/devjo01)
$!!	    - Changed setting of arch_name
$!!	    - Deleted unused symbols 
$!! 	    - Changed Completion Message
$!!	    - Only call ingsysdep at end of "add" case.
$!!	    - Remove bad env. check for remove case.
$!!	    - Correct problem where subroutines would implicitly & prematurely
$!!	      exit if a called routine (e.g. rcpstat) returns a failure.
$!!	    - Correct calculation of new 1st node if original node removed.
$!!	    - Remove memory directory when node is removed.
$!!	14-jun-2005 (bolke01)
$!!	    - Removed part of change to new_first_node and added comment
$!!	08-aug-2005 (devjo01)
$!!	    - Add remove node support.
$!!	    - Log action taken to ingres_installation.dat history file.
$!!	    - Clean up output.  Exploit new ability of box routines to
$!!	      format text so it wraps properly.
$!!	    - Remove a number of UNIX residuals.
$!!	27-sep-2005 (devjo01) b115278
$!!	    - Reduce CLUSTER_MAX_NODE from 16 to 15.
$!!	    - Be consistent in the case used for CLUSTER_MAX_NODE var.
$!!	05-Jan-2007 (bonro01)
$!!	    - Change Startup Guide to Installation Guide.
$!!	28-feb-2007 (upake01) b107385
$!!             - Check if II_SYSTEM logical exists in the GROUP table.
$!!               If it does not, check if it exists in the SYSTEM table.
$!!               If it does, skip checking contents of II_INSTALLATION
$!!               as it is not required for SYSTEM level installation.
$!!     19-Apr-2007 (upake01) b118389
$!!             - Check if "Ingres" is up on other nodes.  If it is,
$!!               issue an error message and stop processing
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to do a selective purge by retaining only the latest
$!!             version of CONFIG.DAT file among all versions generated.
$!!     06-Sep-2007 (upake01)
$!!         BUG 119079 - Fix leading space problem with date for dates 1 to 9 of the month.
$!!             Lexical function "f$time()" returns date and timestamp as
$!!             " 6-SEP-2007 12:34:56.78".  Use f$cvtime(f$time(),"ABSOLUTE").
$!!	02-jan-2008 (joea)
$!!	    Disable CNA features.
$!!	07-jan-2008 (joea)
$!!	    Discontinue use of cluster nicknames.
$!!     12-Feb-2008 (upake01)
$!!         SIR 118402
$!!             In Cleanup Procedure - Remove the line that purges CONFIG.DAT file.
$!!                Split the PURGE line before that as it purges CONFIG.DAT.
$!!                Above change is required as we now retain all versions of CONFIG.DAT.
$!!             Fix the line that sets WRITE_RESPONSE to 'self'READ_RESPONSE i.e. Change
$!!                WRITE_RESPONSE = 'self'READ_RESPONSE   to   WRITE_RESPONSE = 'self'WRITE_RESPONSE
$!!                This will create the response file if user runs IISuNode using a
$!!                "-mkresponse filename" parameter.
$!!     12-Feb-2008 (upake01)
$!!         SIR 118402
$!!             Initialize symbol "II_INSTALLATION" as it gives an "undefined symbol" error
$!!             for system level installation.
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
$!!     30-Apr-2008 (upake01) Bug 120319
$!!             Handle case where protect.dat is absent.
$!!     06-Jun-2008 (upake01) Bug 120222
$!!             Remove "-mkresponse" and "-exresponse" options and change 
$!!             text for usage accordingly.
$!!     11-Nov-2008 (upake01)
$!!         SIR 118402 - Skip running "Selective Purge" routine when the command procedure
$!!             terminates with errors.
$!!	30-apr-2009 (joea)
$!!	    Remove undocumented and incompletely implemented options.
$!!	    Clean up commented out code.
$!!     24-Jun-2010 (horda03) Bug 122555
$!!             Add check_lglk_mem to ensure Ingres will connect to a
$!!             new LG/LK shared memory section.
$!!     16-Nov-2010 (loera01)
$!!         Invoke iicvtgcn to merge node-specific GCN info into the
$!!             clustered GCN database.
$!!
$!----------------------------------------------------------------------------
$_START_IISUNODE:
$!   
$  self		= "iisunode"
$!
$! Get the start time of the job.  It will be used later to do a selective purge
$ start_time = f$cvtime(f$time(),"ABSOLUTE")
$! Get position of space between YYYY & HH
$ space_pos = f$locate(" ",start_time)
$! Change " " to ":"
$ start_time[space_pos*8,8] = 58
$! start time is now in format "23-MAY-2007:15:30:45.90"
$!
$!
$! Check for II_SYSTEM
$!
$ IF F$TrnLnm("II_SYSTEM") .EQS. ""
$ THEN
$DECK

ERROR: Logical II_SYSTEM must be set to run this script.

$EOD
$    EXIT
$ ENDIF
$!
$!  Setup the shared function library.
$!  
$!  Uses:
$!	.	cut_and_append
$!	.	in_set
$!	.	check_path
$!	.	check_user
$!	.	substiture_string
$!	.	validate_
$!	.	iishlib_init
$!	.	clean
$!	.	box
$!	.	error_box
$!	.	nobox
$!	.	message_box
$!	.	warn_box
$!	.	get_node
$!	.	
$!	
$!
$ IF F$SEARCH("II_SYSTEM:[ingres.utility]iishlib.COM") .NES. ""
$ THEN 
$    @II_SYSTEM:[ingres.utility]iishlib _iishlib_init "''self'"
$    cleanup	
$ ELSE
$    TYPE SYS$INPUT
$DECK

ERROR: A shared component of the ingres procedure code could not be located
   
      II_SYSTEM:[ingres.utility]iishlib
   
$EOD
$    exit
$ ENDIF
$!
$!
$ ii_installation_must_exist = "TRUE"
$ IF f$trnlnm("II_SYSTEM","LNM$GROUP") .eqs. ""
$ THEN
$    IF f$trnlnm("II_SYSTEM","LNM$SYSTEM") .eqs. ""
$    THEN
$       error_box "II_SYSTEM is not defined.  The II_SYSTEM logical must be defined at the appropriate (GROUP or SYSTEM) level prior to running this procedure"
$       goto _EXIT_FAIL
$    ELSE
$       ii_installation_must_exist = "FALSE"
$
$       ii_installation = F$TRNLNM( "II_INSTALLATION", "LNM$SYSTEM" )
$    ENDIF
$ ELSE
$    ii_installation = F$TRNLNM( "II_INSTALLATION", "LNM$GROUP" )
$ ENDIF
$!
$!
$! Display copyright, and set 'PID', 'VERSION', and 'RELEASE_ID'
$!
$  copyright_banner
$  if iishlib_err .ne. ii_status_ok then EXIT
$!
$!------------------------------------------------------------------
$!
$! utility macros
$    say := type sys$input
$    echo := write sys$output 
$!------------------------------------------------------------------
$!
$ verify_save = F$VERIFY(F$INTEGER("''SHELL_DEBUG'"))
$!
$!#
$!#   Set up some global variables
$!#
$    complete_message = "Completed at"
$    'self'_subStatus  == 0
$    false		= 0
$    true 		= 1
$!
$    CLUSTER_MAX_NODE 	= 15
$!
$!   Set up error codes
$    STS_NO_MATCHES = %X08D78053
$    SS$_ABORT = 44
$    sts = 0
$    svs = 0
$!
$!   Global variables relating to current nodes
$    'self'NODESLST      == ""
$    'self'IDSLST     	 == ""
$!
$ IF F$TRNLNM("II_SYSTEM") .EQS. "" 
$ THEN
$    error_box -     
"The Ingres logical II_SYSTEM must be defined prior to running this script."
$    exit
$ ENDIF
$!
$ IF F$TRNLNM("II_CONFIG") .NES. "" .AND. -
     F$SEARCH("II_CONFIG:config.dat") .NES. ""
$ THEN
$    IF "''IISHLIB_INFO'" .EQS. "VERBOSE"
$    THEN
$	nobox -
	"Environment II_CONFIG is defined and contains a config.dat file" -
	" "
$!
$    ENDIF
$ ELSE
$	nobox -
	"The Ingres logical II_CONFIG was not found, using II_SYSTEM" -
	" "
$!
$   DEFINE/JOB II_CONFIG II_SYSTEM:[ingres.files]
$ ENDIF
$!
$!  symols and variables used to call utility subroutines
$!
$    $DEBUGIT	= "PIPE ss = 0 ! "
$    FAKEIT 	= 0
$!    
$!
$ IF verify_save  .EQ. 1 THEN SET VERIFY
$!
$!------------------------------------------------------------------
$! aliases and macros for functions and subroutines
$!
$ cancel_add		:=  call _cancel_add 
$ clean_config		:=  call _clean_config 
$ convert_configuration :=  call _convert_configuration 
$ create_node_dirs 	:=  call _create_node_dirs
$ location_check	:=  call _location_check "''self'" 
$ remove_node		:=  call _remove_node
$ setup_log		:=  call _setup_log 
$ status_quo_ante 	:=  call _status_quo_ante  
$ usage			:== call _usage
$ new_first_node        :== call _new_first_node
$ cleanup_iisunode	:=  call _cleanup_iisunode
$!
$!------------------------------------------------------------------
$! Ingres tools
$!
$   iisulock :== $II_SYSTEM:[INGRES.UTILITY]iisulock
$   iiresutl == "$II_SYSTEM:[INGRES.UTILITY]IIRESUTL.EXE"
$   iigetres == "$II_SYSTEM:[INGRES.UTILITY]IIGETRES.EXE"
$   iisetres == "$II_SYSTEM:[INGRES.UTILITY]IISETRES.EXE"
$   iircpseg == "@II_SYSTEM:[INGRES.UTILITY]IIRCPSEG.COM"
$!
$! default values (local to main)
$!
$ clusterid 		= ""
$ config_file 		= ""
$!
$ CONFIG_HOST == f$getsyi( "NODENAME" )
$ info_box " "  "CONFIG_HOST = ''CONFIG_HOST'" " "
$!
$ fileext 	= "_" + f$cvtime() - "-" - "-" - " " - ":" - ":" - "."
$ filepfx 	= F$FAO("!8AS_",self) 
$ fileunq 	= filepfx + fileext
$ ii_system 	= ""
$ iisulocked 	= 0
$ nodeId	= 0
$ resp		= ""
$ require_cancel = 0
$ have_protect_dat = 0
$! 
$ status 	= 1
$ IF F$SEARCH("ii_system:[ingres]tmp.dir") .EQS. "" 
$ THEN
$    CREATE/DIRECTORY ii_system:[ingres.tmp]
$ ENDIF 
$ tmpfile 	= "ii_system:[ingres.tmp]''fileunq'.''config_host'_tmp"
$ user_uic 	= f$user() 
$!
$!------------------------------------------------------------------
$! error handling action
$!
$ on error   then goto exit_fail
$ on warning then goto exit_fail
$!
$ set noon
$!------------------------------------------------------------------
$!#   Parse parameters
$!#
$    'self'BATCH	       == false
$    'self'RESINFILE	       == ""
$    'self'WRITE_RESPONSE      == false
$    'self'READ_RESPONSE       == false
$    REMOVE_CONFIG	= false
$!
$    verify_cur = F$VERIFY('SHELL_DEBUG')
$!
$   id	    =0
$_CHECK_PARM:
$   id = id + 1
$   parm =  p'id'
$   IF "''parm'" .NES. "" .AND. "''parm'" - "-" .NES. "''parm'"  
$   THEN
$       parm = F$EDIT(parm,"LOWERCASE")
$	IF parm - "-remove" .NES. parm
$       THEN
$	   REMOVE_CONFIG  	 = true
$          GOTO _CHECK_PARM
$       ENDIF
$	IF parm - "-test" .NES. parm
$       THEN
$!	    # This prevents actual execution of most commands, while
$!	    # config.dat & symbol.tbl values are appended to the trace
$!	    # file, prior to restoring the existing copies (if any)
$!	    #
$	    DEBUG_LOG_FILE="II_SYSTEM:[ingres.tmp]''filepfx'test.output_''fileext'"
$           catlog    :== WRITE/APPEND SYS$INPUT /OUT='DEBUG_LOG_FILE'
$	    test        = 1
$	    id2 	= id+1
$	    parm 	= p'id2'
$!
$!          # Check the second parameter for -test immediately
$	    IF "''parm'" - "new" .NES. parm
$	    THEN
$		    DEBUG_FAKE_NEW = true
$		    DEBUG_DRY_RUN  = true
$		    id		   = id+1
$	    ELSE 
$              IF "''parm'" - "dryrun" .NES. parm
$	       THEN
$		    DEBUG_DRY_RUN  = true
$		    id		   = id+1
$	       ENDIF
$	    ENDIF
$!
$           PIPE echo "Logging ''self' run results to ''DEBUG_LOG_FILE'"
$	    PIPE echo "DEBUG_DRY_RUN  =''DEBUG_DRY_RUN'" 		
$	    PIPE echo "DEBUG_FAKE_NEW =''DEBUG_FAKE_NEW'" 	
$!
$           PIPE echo "Starting ''self' run on ''fileext'" 	> 'DEBUG_LOG_FILE'
$	    PIPE echo "DEBUG_DRY_RUN  =''DEBUG_DRY_RUN'" 	> 'DEBUG_LOG_FILE'
$	    PIPE echo "DEBUG_FAKE_NEW =''DEBUG_FAKE_NEW'" 	> 'DEBUG_LOG_FILE'
$           GOTO _CHECK_PARM
$!
$       ENDIF
$	IF parm .NES. ""
$       THEN
$	    warn_box "ERROR: invalid parameter P''id' [''parm']."
$           usage "''self'"
$	    exit
$	ENDIF
$	id = id+1
$       GOTO _CHECK_PARM
$   ELSE
$     IF F$TYPE(parm) .EQS. "INTEGER" .OR. -
         ( F$LENGTH("''parm'") .NE. 2 .AND. -
           'self'RESINFILE .EQS. "" )
$     THEN
$        IF "''parm'" .nes. ""
$        THEN
$           usage "''self'"
$	    exit 
$        ENDIF
$     ELSE
$        IF 'self'READ_RESPONSE .EQ. true .OR. -
            'self'WRITE_RESPONSE .EQ. true 
$        THEN
$	   READ_RESPONSE = 'self'READ_RESPONSE
$	   WRITE_RESPONSE = 'self'WRITE_RESPONSE
$          'self'RESINFILE == parm 
$	   RESINFILE = 'self'RESINFILE
$           box "READ_RESPONSE=''READ_RESPONSE'" -
  		"WRITE_RESPONSE=''WRITE_RESPONSE'" -
		"RESINFILE=''RESINFILE'"
$        ENDIF 
$     ENDIF 
$   ENDIF
$   BATCH = false
$!
$!MAIN_PROCESS starts here
$!#
$!# Check II_SYSTEM has been defined and exists
$!# At the same time check other values that are essential and define and 
$!# warn of all exceptions
$!#
$!  Sanity Check
$   check_sanity
$!
$   IF iishlib_err .NE. ii_status_ok 
$   THEN
$	error_box "II_SYSTEM must be set to the root of the Ingres directory tree before you may run ''self'.  Please see the Installation Guide for instructions on setting up the Ingres environment."
$	GOTO _EXIT_OK
$   ENDIF
$!
$! Make a temporary definition of II_COMPATLIB and II_LIBQLIB
$!
$ if f$trnlnm("II_COMPATLIB") .eqs. "" then define II_COMPATLIB -
	'f$parse("II_SYSTEM:[ingres.library]clfelib.exe",,,,"no_conceal")'
$ if f$trnlnm("II_LIBQLIB") .eqs. "" then define II_LIBQLIB -
	'f$parse("II_SYSTEM:[ingres.library]libqfelib.exe",,,,"no_conceal")'
$!
$!  # Obtain identity of 1st node. 
$!
$   oktogo=false
$!
$   SPEC = F$SEARCH("II_CONFIG:config.dat")
$   IF SPEC .NES. ""
$   THEN 
$!
$       getressym 'self' "*" config.server_host 
$       first_node = iishlib_rv1
$	IF "''first_node'" .NES. ""
$	THEN
$          info_box "FIRST_NODE=''first_node'"
$          getressym 'self' "''first_node'" config.dbms.'RELEASE_ID' 
$          RESVAL = iishlib_rv1
$	   IF RESVAL .EQS. "complete" 
$	   THEN
$	      oktogo=true
$          ENDIF
$       ENDIF
$   ENDIF
$!
$   IF oktogo .NE. true
$   THEN
$   	error_box "Ingres must be installed and set up for the primary node before ''self' is run for the secondary nodes."
$	GOTO _EXIT_OK
$   ENDIF
$!
$!
$ IF (ii_installation_must_exist .eqs. "TRUE")
$ THEN
$    iigetres "ii.''first_node'.lnm.ii_installation" II_INSTALLATION
$    ii_installation = f$edit(f$trnlnm("II_INSTALLATION"),"UPCASE")
$    IF ii_installation .EQS. ""
$    THEN 
$       error_box "Could not determine installation identifier. Check if Ingres is configured correctly."
$	GOTO _EXIT_FAIL
$    ENDIF
$ ENDIF
$!
$!  Only the installation owner may run this script.
$!
$   INGUSER = iishlib_rv1
$   IF "''INGUSER'" .EQS. "" 
$   THEN 
$      INGUSER="ingres"
$   ENDIF
$!
$   check_user
$!
$   if test .eq. 0 
$   then
$     IF iishlib_err .ne. ii_status_ok
$     THEN 
$        GOTO _EXIT_FAIL
$     ENDIF
$   endif
$!
$ if 'REMOVE_CONFIG' .eq. true
$ then
$!
$!  # Handle node removal
$!
$   set_ingres_symbols quiet
$!
$   call _remove_node_sub
$   if 'self'_subStatus .eq. 1
$   then
$      goto _EXIT_OK
$   else
$      goto _EXIT_FAIL
$   endif
$ endif
$!
$!  # Continue checks for adding a node.
$!
$    PIPE SEARCH/NOWARN/NOLOG/NOOUT II_CONFIG:config.dat  "ii.''CONFIG_HOST'" ; STS = $STATUS > NL:
$    IF STS .NE. STS_NO_MATCHES 
$    then getressym 'self' "''CONFIG_HOST'" config.cluster_mode
$	  if iishlib_rv1 .eqs. "on"
$	  then warn_box "Ingres on ''CONFIG_HOST' has already been set up in a clustered configuration."
$	  else warn_box "Ingres on ''CONFIG_HOST' has already been set up.  If you want to convert this to a clustered configuration you should run iimkcluster."
$	  endif
$	  goto _exit_ok
$    ENDIF
$!
$!
$!
$   getressym 'self' "''first_node'" config.cluster.id 0 'SHELL_DEBUG'
$   RESVAL = iishlib_rv1
$   IF RESVAL .GT. 0 .AND. RESVAL .LE. 16
$   THEN
$!     # Setup for cluster
$      info_box "''first_node' setup for cluster: ''RESVAL'"
$   ELSE
$!     # Not setup for cluster, check NUMA cluster
$      IF (ii_installation_must_exist .eqs. "TRUE")
$      THEN
$         error_box "Target Ingres instance (''II_INSTALLATION') on ''first_node' was not set up for Clustered Ingres. Ingres instance on ''first_node' must be converted to a cluster configuration before you can run ''self'."
$      ELSE
$         error_box "Target Ingres instance of SYSTEM LEVEL on ''first_node' was not set up for Clustered Ingres. Ingres instance on ''first_node' must be converted to a cluster configuration before you can run ''self'."
$      ENDIF
$      GOTO _EXIT_FAIL
$   ENDIF
$!
$!    # Make sure primary locations are accessible as a sanity check
$!    # Unfortunately without access to iidbdb we can't check ALL locations.
$!
$    save_status = 1
$!
$    location_check "II_DATABASE" "data"
$    if 'self'_subStatus .ne. 1 then save_status = 'self'_subStatus 
$!
$    location_check "II_JOURNAL" "jnl"
$    if 'self'_subStatus .ne. 1 then save_status = 'self'_subStatus 
$!
$    location_check "II_CHECKPOINT" "ckp"
$    if 'self'_subStatus .ne. 1 then save_status = 'self'_subStatus 
$!
$    location_check "II_DUMP" "dmp"
$    if 'self'_subStatus .ne. 1 then save_status = 'self'_subStatus 
$!
$    location_check "II_WORK" "work"
$    if 'self'_subStatus .ne. 1 then save_status = 'self'_subStatus 
$
$    if save_status .NE. 1 then GOTO _EXIT_FAIL
$!
$!   Make sure that Ingres is down on all nodes.
$    ing_instance_is_up = "FALSE"
$    ctx = ""
$    if (ii_installation .NES. "")
$    then
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "NODENAME", "*", "EQL")
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_GCN_''ii_installation'", "EQL")
$    else
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "NODENAME", "*", "EQL")
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_GCN", "EQL")
$    endif
$    proc_pid = f$pid(ctx)
$    if F$TYPE(ctx) .eqs. "PROCESS_CONTEXT" then ctx_release = F$CONTEXT("PROCESS", ctx, "CANCEL")
$    if proc_pid .NES. ""
$    then 
$       ing_instance_is_up = "TRUE"
$       GOTO CHECK_INSTANCE_EXIT
$    endif
$!
$!
$    ctx = ""
$    if (ii_installation .NES. "")
$    then
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "NODENAME", "*", "EQL")
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_DBMS_''ii_installation'_*", "EQL")
$    else
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "NODENAME", "*", "EQL")
$       dummy_ctx = F$CONTEXT("PROCESS", ctx, "PRCNAM", "II_DBMS_*", "EQL")
$    endif
$
$ CHECK_INSTANCE_LOOP:
$    proc_pid = f$pid(ctx)
$    if (proc_pid .NES. "")
$    then
$       if (ii_installation .NES. "")
$       then
$          ing_instance_is_up = "TRUE"
$       else
$          proc_name = f$getjpi(proc_pid, "PRCNAM")
$          if (f$element(3, "_", proc_name) .EQS. "_") 
$          then 
$             ing_instance_is_up = "TRUE"
$          else
$             goto CHECK_INSTANCE_LOOP
$          endif
$       endif
$    endif
$!
$ CHECK_INSTANCE_EXIT:
$    if (ing_instance_is_up .eqs. "TRUE")
$    then 
$       error_box "Ingres must be shutdown on all nodes in the cluster prior to running this procedure. " -
        " " -
        "Please use """"ingstop"""" on each node before running ''self' again."
$       goto _EXIT_FAIL
$    endif
$!
$!
$    IF (ii_installation_must_exist .eqs. "TRUE")
$    THEN
$    nobox -
           """""''self'"""" will set up Ingres on ''CONFIG_HOST' as a node in the Clustered Ingres instance """"''II_INSTALLATION'"""" using the same configuration as ''VERSION' components already setup on """"''first_node'""""." -
           " "
$    ELSE
$    nobox -
           """""''self'"""" will set up Ingres on ''CONFIG_HOST' as a node in the Clustered Ingres instance of SYSTEM LEVEL using the same configuration as ''VERSION' components already setup on """"''first_node'""""." -
           " "
$    ENDIF
$!
$    IF BATCH .EQ. false
$    THEN
$	PIPE confirm_intent
$       if iishlib_rv1 .nes. "y"
$       THEN
$          goto _EXIT_OK
$       ENDIF
$    ELSE
$       $DEBUGIT  box "Looks OK - lets move on"	
$    ENDIF
$!
$    lock_config_dat "''self'" "''iisulocked'"
$    iisulocked = F$INTEGER(iishlib_rv1)
$!
$!  We must be able to lock the config.dat file.
$!
$    IF iishlib_err .ne. ii_status_ok
$    THEN
$       unlock_config_dat "''self'" "''iisulocked'"
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    backup_file config.dat "''fileext'"
$!
$    $DEBUGIT dir II_CONFIG:config.dat*/sin
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       unlock_config_dat "''self'" "''iisulocked'"
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    if f$search( "II_SYSTEM:[ingres.files]protect.dat" ) .NES. ""
$    then
$       have_protect_dat = 1
$       backup_file protect.dat "''fileext'"
$       $DEBUGIT dir II_CONFIG:protect.dat*/sin
$       IF iishlib_err .NE. ii_status_ok
$       THEN
$          unlock_config_dat "''self'" "''iisulocked'"
$          GOTO _EXIT_FAIL 
$       ENDIF
$    endif
$!    
$    log_config_change_banner  "BEGIN NODE ADD" "''CONFIG_HOST'" "''self'" 24
$    $DEBUGIT dir II_CONFIG:config.log*/sin
$!
$!   From this point onwards the cancel_add function needs to be called.
$    require_cancel = 1
$!    #
$!    #  Get node number
$!    #
$    'self'NODESLST   == ""
$    'self'IDSLST     == ""
$    gen_nodes_lists "''self'"
$    nlst = 'self'NODESLST
$    ilst = 'self'IDSLST
$    'self'NODESLST == iishlib_rv1
$    'self'IDSLST == iishlib_rv2
$    info_box "''self'NODESLST = ''nlst'" -
	   	 "''self'IDSLST   = ''ilst'" 
$    nlst = 'self'NODESLST
$    ilst = 'self'IDSLST
$    info_box "''self'NODESLST = ''nlst'" -
	   	 "''self'IDSLST   = ''ilst'" 
$!  
$    nlst = 'self'NODESLST
$    ilst = 'self'IDSLST
$    info_box "NODESLST = ''nlst'" -
	   	 "IDSLST   = ''ilst'" 
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       unlock_config_dat "''self'" "''iisulocked'"
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    get_node_number  "''self'"
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       unlock_config_dat "''self'" "''iisulocked'"
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    II_CLUSTER_ID = F$INTEGER(II_CLUSTER_ID)
$    message_box "''CONFIG_HOST' will be set up as node ''II_CLUSTER_ID'." 
$!
$    define II_CLUSTER_ID "''II_CLUSTER_ID'"
$!
$    IF BATCH .EQ. false
$    THEN
$	PIPE confirm_intent
$       if iishlib_rv1 .nes. "y"
$       THEN
$          goto _EXIT_OK
$       ENDIF
$    ELSE
$       $DEBUGIT box "Looks OK - lets move on"	
$    ENDIF
$!
$    nobox "Adding node to cluster, please wait ..."
$!  #
$!  #	Create an admin area for this NODE. logical names are held in the 
$!  #   config.dat file in the lnm space
$!  #
$    if "''CONFIG_HOST'" .eqs. ""
$    then
$       error_box "Inconsistent condition detected """"CONFIG_HOST"""" is not declared"
$       goto _EXIT_FAIL
$    endif
$!
$    clusterid = F$TRNLNM("II_CLUSTER_ID")
$    clusterid = F$INTEGER(clusterid)
$    clusterid2 = F$FAO("!2ZL",clusterid)
$    cfu_host = F$EDIT(CONFIG_HOST,"UPCASE")
$!
$    create_node_dirs "''CONFIG_HOST'"
$    info_box "Node directories for """"''CONFIG_HOST'"""" have been created or existed"
$! 
$!   # Add new node to config.dat as clone of the old node.
$!
$    nobox "Adding configuration for node ''CONFIG_HOST'"
$    convert_configuration "config.dat" "''filepfx'" "''fileunq'"
$    if have_protect_dat
$    then
$       convert_configuration "protect.dat" "''filepfx'" "''fileunq'"
$    endif
$    set_ingres_symbols 
$!   
$!   # Update parameters specific to the new node in config.dat
$    iisetres "ii.''CONFIG_HOST'.config.cluster_mode"    "ON"
$    iisetres "ii.''CONFIG_HOST'.config.cluster.id" "''clusterid'"
$    iisetres "ii.''CONFIG_HOST'.lnm.ii_cluster_id" "''clusterid'"
$    iisetres "ii.''CONFIG_HOST'.lnm.ii_gcn''II_INSTALLATION'_lcl_vnode" -
	"''cfu_host'"
$    iisetres "ii.''CONFIG_HOST'.gcn.local_vnode"   "''cfu_host'"
$    echo ""
$!
$! Convert GCN files to cluster format.  Merge this node's GCN database
$! with the GCN cluster database.
$!
$ iicvtgcn -c
$!
$!   # clear the II_GCN logicals
$!   
$    if F$TRNLNM("II_GCN''ii_installation'_LCL_VNODE","LNM$PROCESS_TABLE")
$    then
$       DEASSIGN "II_GCN''ii_installation'_LCL_VNODE" 
$    endif
$!
$    if F$TRNLNM("II_GCN''II_INSTALLATION'_''first_node'_PORT","LNM$PROCESS_TABLE")
$    then
$       DEASSIGN "II_GCN''II_INSTALLATION'_''first_node'_PORT" 
$    endif
$! 
$!  # Initialize transaction logs for new node.
$!
$   setup_log "primary" "" "-init_log"
$!
$   getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_1
$   RESVAL = iishlib_rv1
$   IF "''RESVAL'" .NES. ""
$   THEN
$      setup_log "dual" "-dual" "-init_dual"
$   ENDIF
$!
$!  Add to Install history file
$   IF (ii_installation_must_exist .EQS. "TRUE")
$   THEN
$       add_install_history "''VERSION'" -
	    "Added as node ''II_CLUSTER_ID' to clustered installation ''II_INSTALLATION'"
$   ELSE
$       add_install_history "''VERSION'" -
	    "Added as node ''II_CLUSTER_ID' to clustered installation of SYSTEM LEVEL"
$   ENDIF
$!
$   nobox "Node ''CONFIG_HOST' set up is complete."
$!
$   goto _EXIT_OK
$!
$!
$!-------------------------------------------------------------------
$! Exit labels.
$!
$_EXIT_FAIL:
$ complete_message = "Failed at"
$ if  require_cancel .eq. 1
$ then
$    cancel_add  "''fileext'" "''filepfx'" "''fileunq'"
$ endif
$!
$ status = SS$_ABORT
$ warn_box "The script ''self' failed, check log for further information"
$!
$ goto _EXIT_OK_2
$!
$_EXIT_OK:
$!
$!! Purge config.dat files since the start of the job keeping only the
$!! latest version.  Rename the most recent version to previous version
$!! number plus 1.  Delete the most recent version if the last two versions
$!! are same.  All this will be done by calling "selective_purge"
$ selective_purge  'start_time'
$ if iishlib_err .ne. ii_status_ok
$    then
$    message_box "''self' - Error doing Selective Purge on CONFIG.DAT.  The job has completed but all versions of CONFIG.DAT have been retained"
$ endif
$!
$!
$_EXIT_OK_2:
$   unlock_config_dat "''self'" "''iisulocked'"
$!
$   cleanup "''self'"	
$!
$   cleanup_iisunode
$!
$!This is the only clean exit point in the script
$!
$ end_time = F$TIME()
$ nobox "''self' ''complete_message': ''end_time'"
$!
$ EXIT
$!
$!-------------------------------------------------------------------
$!   SUBROUTINES
$!	0. 	_clean_config 
$!	1.	_convert_configuration 
$!	2.	_gen_log_name
$!	3.	_setup_log 
$!	4.	_location_check
$!	5.	_backup_file
$!	6.	_restore_file 
$!	7.	_append_node
$!	8.	_create_node_dirs
$!	9.	_confirm_intent
$!	10.	_log_config_change_banner
$!	11.	_unlock_config_dat
$!	12.	_lock_config_dat
$!	13.	_check_path 
$!	14.	_cancel_add
$!	15.	_status_quo_ante  
$!	16.	_status_quo_ante2 - called only by _status_quo_ante
$!	17.	_validate_node_number
$!	18.	_check_user
$!	19.	_check_sanity
$!	20.	_remove_node_dirs
$!   UTILITIES
$!	1.	_nobox
$!	2.	_message_box
$!	3.	_warn_box
$!	4.	_info_box
$!	5.	_box
$!	6.	_getressym
$!	7.	
$!	8.	
$!-------------------------------------------------------------------
$!
$!
$ _create_node_dirs: SUBROUTINE
$!
$ node = "''P1'"
$!
$!   Create new sub-directories.
$!
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "files" "memory"
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "files" "memory" 'node'
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "admin"
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "admin" 'node'
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$ _clean_config: SUBROUTINE 
$!
$   cf_file  = "''P1'"
$   cf_host  = "''P2'"
$   pfx  = "''P3'"
$   if pfx .eqs. ""
$   then
$      'self'_subStatus == 0
$      exit
$   endif
$!
$ if F$SEARCH("II_CONFIG:''cf_file'") .nes. ""
$ then
$   PIPE SEARCH/NOWARN/NOLOG/NOEXACT/MATCH=NOR/OUTPUT=II_CONFIG:'pfx''cf_file'  -
	II_CONFIG:'cf_file' -
         "ii.''cf_host'." > NL:
$   'self'_subStatus == 1
$ else
$   'self'_subStatus == 0
$   warn_box "Backup of ''cf_file' with extension ''fileext' was not found, it may not yet have been created"  
$ endif
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!
$!   Generate configuration for this node based on that of 1st node.
$!
$! 	P1 = file name (config.dat | protect.dat)
$!
$ _convert_configuration: SUBROUTINE 
$!
$   cf_file = P1
$   if cf_file .eqs. ""
$   then
$      'self'_subStatus == 0
$      exit
$   endif
$!
$   pfx = "''P2'"
$   if pfx .eqs. ""
$   then
$      pfx = "tmp_"
$   endif
$!
$   unq = "''P3'"
$   if unq .eqs. ""
$   then
$      unq = "new_"
$   endif
$!
$!--- Ensure that all existing records are removed
$!
$   clean_config "''cf_file'" "''CONFIG_HOST'" "''unq'"
$!
$!--- existing records are copied and replaced by new NODE
$!
$   PIPE SEARCH/NOWARN/NOLOG/NOEXACT/OUTPUT=II_CONFIG:'pfx''P1' -
	 II_CONFIG:'P1'              -
         "ii.''first_node'."  ; -
         substitute_string "''self'" "ii.''first_node'." "ii.''CONFIG_HOST'."  -
         "II_CONFIG:''pfx'''P1'" 
$   TYPE/NOHEAD/OUTPUT=II_CONFIG:'unq''P1'_tmp II_CONFIG:'unq''P1', II_CONFIG:'pfx''P1'
$!
$   PURGE/NOLOG/KEEP=1  II_CONFIG:*'unq'*.*
$   PURGE/NOLOG/KEEP=1  II_CONFIG:*'pfx'*.*
$   RENAME  		 II_CONFIG:'unq''P1'_tmp II_CONFIG:'P1'
$   DELETE  		 II_CONFIG:'pfx''P1'.0
$!
$   EXIT
$!
$ ENDSUBROUTINE
$!
$!
$!#
$!#   Create transaction logs for new node
$!#
$!#	$1 = "primary" | "dual"
$!#	$2 = "" | "-dual"
$!#	$3 = "-init_log" | "-init_dual"
$!#
$ _setup_log: SUBROUTINE 
$!
$   log_name 	= "''P1'"
$   log_flag 	= "''P2'"
$   config_flag = "''P3'"
$!
$   if test .eq. 0
$   then
$     iimklog 'log_flag' 
$     echo " "
$     ss = $severity
$   else
$     ss = 1
$     info_box "SINGLE MACHINE TEST : iimklog ''log_flag'" 
$   endif
$   IF ss .NE. 1
$   THEN
$     error_box "Fail to create ''log_name' transaction log for ''CONFIG_HOST'." 
$     GOTO _SETUP_LOG_EXIT_FAIL
$   ENDIF
$!    
$   if test .eq. 0
$   then
$     rcpconfig "''config_flag'" 
$     ss = $severity
$   else
$     ss = 1
$     info_box "SINGLE MACHINE TEST : rcpconfig ''config_flag'" 
$   endif
$   echo " "
$   IF ss .NE. 1
$   THEN
$     error_box "Fail to format ''log_name' transaction log for ''CONFIG_HOST'."
$!
$     GOTO _SETUP_LOG_EXIT_FAIL
$   ENDIF
$!
$  GOTO _SETUP_LOG_EXIT
$!  
$_SETUP_LOG_EXIT_FAIL:
$  message_box "Aborting setting up the transaction log for new node ''CONFIG_HOST'" -
        " " -
	"Check log file for errors, and if required create using """"iimklog"""" or """"cbf""""."
$! 
$_SETUP_LOG_EXIT:
$ exit
$!
$ ENDSUBROUTINE
$!
$!#
$!#
$!# Location check to ensure the directory exists
$!#
$!#       P1 = self
$!#       P2 = Ingres environment variable
$!#       P3 = Content subdirectory name.
$!#
$ _location_check: SUBROUTINE
$!
$    self           = P1
$    iigetres "ii.*.lnm.''P2'" "''P2'"
$    root_location  = F$TRNLNM("''P2'")
$    subdir	    = "''P3'"
$!
$    SPEC = F$SEARCH("''root_location'[INGRES]''subdir'.dir")
$    IF SPEC .EQS. ""   
$    THEN
$       IF root_location .EQS. ""  
$       THEN
$          root_location = "Undefined"
$       ENDIF  
$       error_box "Cannot access content directory for location defined for Ingres environment variable ''P2' (''root_location'[INGRES.''subdir'])."
$       message_box  "Please check that this directory is accessible from ''CONFIG_HOST'."
$       'self'_subStatus == 0
$    ELSE
$       $DEBUGIT box "''P1' = ''root_location'''SPEC'"
$       'self'_subStatus == 1
$    ENDIF
$! 
$ RETURN
$!
$ ENDSUBROUTINE
$! 
$!
$!#
$!# cancel_add - backout adding a node
$!#
$ _cancel_add: SUBROUTINE
$!
$ ext = "''P1'"
$ pfx = "''P2'"
$ unq = "''P3'"
$!
$ remove_node  "''ext'" "''pfx'" "''unq'"
$ if 'self'_subStatus .ne. 1
$ then
$    warn_box "Node removal was partially incomplete. See logfile for details of problem"
$ endif
$ restore_file config.dat "''ext'"
$ if have_protect_dat
$ then
$    restore_file protect.dat "''ext'"
$ endif
$ log_config_change_banner  "NODE ADD FAILED/CANCELED" -
	"''CONFIG_HOST'" "''self'" 30
$ unlock_config_dat "''self'" "''iisulocked'"
$!
$ exit
$!
$ ENDSUBROUTINE
$! 
$!#
$!# Undo changes to config files, saving modified files for post-mort
$!#
$ _status_quo_ante: SUBROUTINE
$!
$!
$ ext = "''P1'"
$ pfx = "''P2'"
$ unq = "''P3'"
$!
$   call _status_quo_ante2  "config.dat"   "''ext'" "''pfx'" "''unq'"
$   if have_protect_dat
$   then
$      call _status_quo_ante2  "protect.dat"  "''ext'" "''pfx'" "''unq'"
$   endif
$!
$ return
$!
$ ENDSUBROUTINE
$! 
$!#
$!# Undo changes to config files, saving modified files for post-mort
$!#
$ _status_quo_ante2: SUBROUTINE
$!
$ config_file = "''P1'"
$ ext = "''P2'"
$ pfx = "''P3'"
$ unq = "''P4'"
$!
$    SPEC = F$SEARCH("II_CONFIG:config.dat")
$    IF SPEC .NES. ""
$    THEN 
$	APPEND II_CONFIG:'config_file'  'DEBUG_LOG_FILE'
$       restore_file "''config_file'" "''ext'"
$    ENDIF	
$!
$ return
$!
$ ENDSUBROUTINE
$!
$!
$!
$!--------------------------------------------------------------------
$! UTILITY SUBROUTINES
$! -------------------
$!--------------------------------------------------------------------
$!
$ _new_first_node: SUBROUTINE
$!
$!    # Get existing node with lowest node number, assign to first_node
$!
$   gen_nodes_lists "''self'"
$   nlst = 'self'NODESLST
$   ilst = 'self'IDSLST
$   info_box "Current list of nodes" "NODESLST=''nlst'"  "IDSLST=''ilst'" 
$!
$   id = 0
$   min_id = 0
$   current_id = -1
$   min_first = CLUSTER_MAX_NODE + 1
$_FN_NEXT:
$!  When current_id is zero all nodes have been reviewed
$   if id .ge. CLUSTER_MAX_NODE .or. current_id .eq. 0
$   then 
$      goto _FN_LAST
$   endif
$!  When no more elements current_id is set to zero
$   current_id = F$INTEGER(F$ELEMENT(id," ","''ilst'"))
$   IF current_id .ne. 0 .and. current_id .LT. min_first 
$   THEN 
$      min_id = id
$      min_first = current_id
$   ENDIF
$   id = id + 1
$   GOTO _FN_NEXT
$!
$_FN_LAST:
$   nlst = 'self'NODESLST
$   node_name = F$ELEMENT(min_id," ","''nlst'")
$   first_node==node_name
$   message_box "Lowest cluster ID = ''min_first'" -
 		"       Node name  = ''node_name'"
$!  
$  if min_first .gt. CLUSTER_MAX_NODE
$  then
$     'self'_subStatus == 0
$  else
$      'self'_subStatus == 1
$  endif
$!
$ exit
$ ENDSUBROUTINE
$!
$!
$!#
$!# remove_node - do the actual removal of a node.
$!#
$ _remove_node: SUBROUTINE
$!
$   ext = "''P1'"
$   if ext .eqs. ""
$   then
$      ext = "tmp_"
$   endif
$!
$   pfx = "''P2'"
$   if pfx .eqs. ""
$   then
$      pfx = "tmp_"
$   endif
$!
$   unq = "''P3'"
$   if unq .eqs. ""
$   then
$      unq = "new_"
$   endif
$!
$! # Blast log files
$  SET NOON
$  getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_parts 
$  NUM_PARTS = F$INTEGER(iishlib_rv1)
$  info_box  "REMOVE_NODE for ''CONFIG_HOST' - START (log_file_parts=''iishlib_rv1')"
$  IF NUM_PARTS .EQ. 0 .OR.  (NUM_PARTS .LT. 1 .OR. NUM_PARTS .GT. 16) 
$  THEN
$     warn_box "Could not determine transaction log configuration." -
	   " " -
	   "Transaction logs for node ''CONFIG_HOST' must be manually removed."
$  ELSE
$     getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_name 
$     PRIMARY_LOG_NAME = iishlib_rv1
$     info_box  "REMOVE_NODE for ''CONFIG_HOST' " " (log_file_name=''iishlib_rv1')"
$     IF "PRIMARY_LOG_NAME" .EQS. ""
$     THEN
$        PRIMARY_LOG_NAME='ingres_log'
$     ENDIF
$     getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_name 
$     DUAL_LOG_NAME = iishlib_rv1
$     info_box  "REMOVE_NODE for ''CONFIG_HOST' " " (dual_file_name=''iishlib_rv1')"
$!
$     PART=0
$_PART_NEXT:
$     IF  PART .LT. NUM_PARTS 
$     THEN
$ 	 PART=PART + 1
$        getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_'PART' 
$        DIRSPEC = iishlib_rv1
$!
$	 IF DIRSPEC .EQS. "" 
$	 THEN 
$	    GOTO _PART_NEXT
$	 ENDIF
$!
$	 gen_log_name 'PRIMARY_LOG_NAME' 'PART' 'CONFIG_HOST' 'SHELL_DEBUG'
$	 OLD_NAME = iishlib_rv1 
$        info_box  "REMOVE_NODE for ''CONFIG_HOST' " "(gen_log-name=''OLD_NAME')"
$	 SPEC = F$SEARCH("''DIRSPEC'[ingres.log]''OLD_NAME'")
$!
$	 IF SPEC .NES. ""
$	 THEN
$ 	    remove_file 'SPEC'
$	 ELSE
$           warning_box "Cannot find primary transaction log file ''DIRSPEC'[ingres.log]''OLD_NAME'"
$	 ENDIF
$!
$	 IF "''DUAL_LOG_NAME'" .EQS. ""
$	 THEN
$           GOTO _PART_NEXT
$	 ENDIF
$!>
$        getressym 'self' "''CONFIG_HOST'" rcp.log.dual_file_'PART' 
$        DIRSPEC = iishlib_rv1
$	 IF DIRSPEC .EQS. "" .OR. PART .EQ. 1
$	 THEN 
$	    GOTO _PART_NEXT
$	 ENDIF
$!	    
$	 gen_log_name 'DUAL_LOG_NAME' 'PART' 'CONFIG_HOST' 'SHELL_DEBUG'
$	 OLD_NAME = iishlib_rv1 
$        info_box  "REMOVE_NODE for ''CONFIG_HOST' " "(gen_log_name=DUAL=''OLD_NAME')"
$	 SPEC = F$SEARCH("''DIRSPEC'[ingres.log]''OLD_NAME'")
$	 IF SPEC .NES. ""
$	 THEN
$	    remove_file 'SPEC'
$	 ELSE
$           warning_box "Cannot find dual transaction log file ''DIRSPEC'[ingres.log]''OLD_NAME'"
$	 ENDIF
$	 GOTO _PART_NEXT
$     ENDIF	
$  ENDIF
$!
$! # Remove memory directory (don't fail if have touble).
$  PIPE DELETE /NOCONF II_SYSTEM:[INGRES.FILES.MEMORY.'CONFIG_HOST']*.*;* > NL:
$  PIPE SET FILE /PROT=(O:RWED) -
     II_SYSTEM:[INGRES.FILES.MEMORY]'CONFIG_HOST'.DIR;* > NL:
$  PIPE DELETE /NOCONF II_SYSTEM:[INGRES.FILES.MEMORY]'CONFIG_HOST'.DIR;* > NL:
$!
$! # Strip out config.dat entries for removed node
$  clean_config "config.dat" "''CONFIG_HOST'"  "''unq'" ""
$!
$  if 'self'_subStatus .ne. 0
$  then
$     RENAME II_CONFIG:'unq'config.dat II_CONFIG:config.dat 
$  endif
$!
$  if have_protect_dat
$  then
$     if 'self'_subStatus .ne. 0
$     then
$        clean_config "protect.dat" "''CONFIG_HOST'" "''unq'" ""
$!
$        if 'self'_subStatus .ne. 0
$        then
$           RENAME II_CONFIG:'unq'protect.dat II_CONFIG:protect.dat 
$        endif
$     endif
$  endif
$!
$  getressym 'self' "*" config.server_host 
$  first_node = iishlib_rv1
$  info_box  "REMOVE_NODE for ''CONFIG_HOST' " "(config.server_host=''iishlib_rv1')"
$!
$  IF first_node .EQS. F$EDIT(CONFIG_HOST,"LOWERCASE") 
$  THEN
$!
$!	 Need to reset server_host
$     new_first_node 
$     if  'self'_subStatus .eq. 0
$     then
$        error_box "Unable to configure a new primary node server host"
$	 exit
$     endif
$!
$     iisetres "ii.*.config.server_host" "''first_node'"
$  ENDIF
$!
$ return
$!
$ ENDSUBROUTINE
$!
$ _remove_node_sub: SUBROUTINE
$!
$     SET NOON
$!
$!    # Confirm this node is clustered.
$     getressym 'self' "''CONFIG_HOST'" config.cluster_mode
$     RESVAL = iishlib_rv1
$     info_box "''CONFIG_HOST' config.cluster_mode = ''iishlib_rv1'"
$     if "''RESVAL'" .nes. "on" 
$     then
$        error_box "Current node (''CONFIG_HOST') does not appear to be part of a cluster."
$        goto _REMNODE_EXIT_FAIL
$     endif
$!
$!     # Do not remove the last node.
$     PIPE SEARCH/NOWARN/NOLOG/MATCH=AND/OUTPUT=II_CONFIG:'filepfx'config.dat -
	II_CONFIG:config.dat "ii.",".config.cluster_mode", "on"  > NL:
$     chop_and_count "II_CONFIG:''filepfx'config.dat" ":"
$     RESVAL = F$INTEGER(iishlib_rv1)
$     if 'RESVAL' .le. 1
$     then
$        error_box "You cannot remove the last node in a cluster using """"''self'"""". " -
	 	  " " "Please use """"iiuncluster"""" to convert Ingres instance to a non-cluster configuration."
$        goto _REMNODE_EXIT_FAIL
$     endif
$!
$!    # Confirm that all nodes are down
$     PIPE rcpstat -any_csp_online -silent ; STS = $SEVERITY > NL:
$     info_box "rcpstat -any_csp_online -silent = (status ''STS')"
$     if STS .ne. 4 
$     then 
$        error_box "All nodes in cluster must be shutdown prior to changing the static configuration of the cluster."
$        goto _REMNODE_EXIT_FAIL
$     endif
$!
$!    # Confirm no transactions remain in this node's logs.
$     PIPE rcpstat -transactions -silent ; STS = $SEVERITY > NL:
$     info_box "rcpstat -transactions -silent  (status = ''STS')"
$     if STS .ne. 4 
$     then 
$         error_box "There are still transactions in the transaction log for this node.  You may not remove a node from a cluster before these transactions have been resolved."
$!
$         message_box "Please restart this node and then stop it cleanly so these transactions will be removed from the log."
$!
$        goto _REMNODE_EXIT_FAIL
$     endif
$
$     ! Make certain there isn't an IPM session using the Logging/Locking shared memory.
$
$     check_lglk_mem "''ii_installation'"
$!
$     getressym 'self' "''CONFIG_HOST'" config.cluster.id
$     II_CLUSTER_ID = iishlib_rv1
$     IF (ii_installation_must_exist .eqs. "TRUE")
$     THEN
$     nobox " " -
      "This procedure will remove the current machine (''CONFIG_HOST') configured as node ''II_CLUSTER_ID' from Ingres cluster ''ii_installation'." -
      " "
$     ELSE
$     nobox " " -
      "This procedure will remove the current machine (''CONFIG_HOST') configured as node ''II_CLUSTER_ID' from Ingres cluster of SYSTEM LEVEL." -
      " "
$     ENDIF
$     nobox "All the transaction logs for this node have been confirmed empty and will be destroyed as part of this procedure."
$!
$     confirm_intent 
$     if iishlib_rv1 .nes. "y"
$     then
$       goto _REMNODE_EXIT_OK
$     endif
$!
$     lock_config_dat "''self'" "''iisulocked'"
$     iisulocked = F$INTEGER(iishlib_rv1)
$!
$     IF iishlib_err .ne. ii_status_ok
$     THEN
$        unlock_config_dat "''self'" "''iisulocked'"
$        GOTO _REMNODE_EXIT_FAIL 
$     ENDIF
$!
$     backup_file config.dat "''fileext'"
$     $DEBUGIT dir II_CONFIG:config.dat*/sin
$     IF iishlib_err .NE. ii_status_ok
$     THEN
$        unlock_config_dat "''self'" "''iisulocked'"
$        GOTO _REMNODE_EXIT_FAIL 
$     ENDIF
$!
$     if have_protect_dat
$     then
$        backup_file protect.dat "''fileext'"
$        $DEBUGIT dir II_CONFIG:protect.dat*/sin
$        IF iishlib_err .NE. ii_status_ok
$        THEN
$           unlock_config_dat "''self'" "''iisulocked'"
$           GOTO _REMNODE_EXIT_FAIL 
$        ENDIF
$     endif
$!
$     log_config_change_banner  "BEGIN NODE REMOVE" "''CONFIG_HOST'" "''self'" 27
$     $DEBUGIT dir II_CONFIG:config.log*/sin
$!
$     remove_node  "''fileext'" "''filepfx'" "''fileunq'"
$     if 'self'_subStatus .ne. 1
$     then
$        warn_box "Node removal was partially incomplete.  See logfile for details of problem."
$       'self'_subStatus == 0
$     endif
$!
$     unlock_config_dat "''self'" "''iisulocked'"
$     IF iishlib_err .ne. ii_status_ok
$     then
$        warn_box " " "Unable to unlock CONFIG.DAT"
$     endif
$!
$     if 'self'_subStatus .eq. 1
$     then
$        nobox " " "Node ''CONFIG_HOST' has been removed from the cluster." -
             "Old configuration file has been saved as " -
             "config.dat''fileext' for documentation."
$     else
$        warn_box -
	     "Errors occured during the removal of the node. Old configuration file has been saved as config.dat''fileext' for documentation."
$    endif
$!
$   IF (ii_installation_must_exist .eqs. "TRUE")
$   THEN
$      add_install_history "''VERSION'" -
	   "Removed node ''II_CLUSTER_ID' from clustered installation ''II_INSTALLATION'"
$   ELSE
$      add_install_history "''VERSION'" -
	   "Removed node ''II_CLUSTER_ID' from clustered installation of SYSTEM LEVEL"
$   ENDIF
$!
$    goto _REMNODE_EXIT_OK
$!
$ _REMNODE_EXIT_FAIL:
$ 'self'_subStatus == 0
$ exit
$!
$ _REMNODE_EXIT_OK:
$ 'self'_subStatus == 1
$ exit
$!
$ ENDSUBROUTINE
$!
$ _usage: SUBROUTINE
$!
$	    nobox "Usage ''self' [ -remove ] "
$ exit
$!
$ ENDSUBROUTINE
$!
$!
$!   Cleanup 
$!
$ _cleanup_iisunode: subroutine
$!
$!  Delete symbols
$!
$ if f$trnlnm("II_COMPATLIB", "LNM$PROCESS") .nes. "" then deassign II_COMPATLIB
$ if f$trnlnm("II_LIBQLIB", "LNM$PROCESS") .nes. "" then deassign II_LIBQLIB
$  DELETE/SYM/GLOBAL   'self'BATCH
$  DELETE/SYM/GLOBAL   'self'READ_RESPONSE
$  DELETE/SYM/GLOBAL   'self'WRITE_RESPONSE
$  DELETE/SYM/GLOBAL   'self'RESINFILE
$  DELETE/SYM/GLOBAL   'self'NODESLST
$  DELETE/SYM/GLOBAL   'self'IDSLST
$!
$! Some symbols may not have been defined, create and delete to 
$! ensure no error.  alternative is to turn off messages
$!
$  'self'_SUBSTATUS    == 1
$  DELETE/SYM/GLOBAL   'self'_SUBSTATUS
$  IISHLIB_'self'_LTIME == 0
$  DELETE/SYM/GLOBAL   IISHLIB_'self'_LTIME
$  II_CLUSTER_ID       == ""
$  DELETE/SYM/GLOBAL   II_CLUSTER_ID
$ exit
$ endsubroutine
$!
$!-----------------------END SUBROUTINES ---------------------
$!
