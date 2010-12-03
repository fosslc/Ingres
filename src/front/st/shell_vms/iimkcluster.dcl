$ goto _START_IIMKCLUSTER
$!  Copyright (c) 2002, 2008 Ingres Corporation
$! 
$! 
$!  $Header $
$!
$!  Name: iimkcluster - Convert a non-cluster Ingres instance to 1st node
$!		       of a Clustered Ingres instance.
$!
$!  Usage:
$!	iimkcluster
$!
$!  Description:
$!	Converts where supported a non-clustered Ingres instance
$!	to the first node of an Ingres cluster.
$!
$!	This basically consists of "decorating" the log file names,
$!	Modifying certain config.dat and symbol.tbl values, and
$!	creating a number of node specific subdirectories.
$!
$!	This script must be run as 'ingres', and while instance is
$!	down.
$!
$! Exit status:
$!	0	OK - conversion to Clustered Ingres successful.
$!	1	User decided not to proceed.
$!	2	Environment does not support clustering.
$!	3	Instance fails basic setup sanity checks.
$!	4	Ingres is apparently not set up.
$!	5	Ingres is already set up for clusters.
$!	6	II_CLUSTER indicates instance is already
$!		part of a cluster.
$!	7	Ingres must be shutdown before conversion.
$!	8	Quorum manager was not up.
$!	9	Distributed Lock Manager was not up.
$!	10	Must be "ingres" to run script.
$!	11	Process failed or was interrupted during convert.
$!	12	Configuration file is locked.
$!	14	Unable to backup an existing file.
$!
$!! History
$!!	13-may-2005 (bolke01)
$!!		Initial version.
$!!		Based on the UNIX version iimkcluster.sh
$!!	15-may-2005 (bolke01)
$!!		Chnaged location of iishlib.com to UTILITY
$!!	07-jun-2005 (bolke01)
$!!		Comment (!) erroneously deleted has been replaced
$!!	13-jun-2005 (bolke01/devjo01)
$!!             - Changed setting of arch_name
$!!             - Deleted unused symbols 
$!!             - Changed Completion Message
$!!		- Various cleanups to silence DCL check script warnings.
$!!		- Add logic to set VMS enqueue limit.
$!!	        - Add +p to iisetres lock_limit to protect value.
$!!		- Replace iiappend_first_class with iiappend_class as the
$!!		  two functions have merged.
$!!	19-sep-2005 (devjo01)
$!!		- Break some text which is too long for the 7.3 DCL into
$!!		two boxes.
$!!		- Bail if config.dat is absent rather than creating an
$!!		empty config.dat.
$!!		- Handle case where protect.dat is absent. (14334102;01)
$!!		- Remove extraneous "$" from STAR message. (14334102;01)
$!!	27-sep-2005 (devjo01) b115278
$!!		- Reduce CLUSTER_MAX_NODE from 16 to 15.
$!!		- Be consistent in case used for CLUSTER_MAX_NODE var.
$!!		- Correct message passed to add_install_history.
$!!	05-Jan-2007 (bonro01)
$!!		- Change Getting Started Guide to Installation Guide.
$!!	28-feb-2007 (upake01) b107385
$!!             - Check if II_SYSTEM logical exists in the GROUP table.
$!!               If it does not, check if it exists in the SYSTEM table.
$!!               If it does, skip checking contents of II_INSTALLATION
$!!               as it is not required for SYSTEM level installation.
$!!	15-Mar-2007 (upake01) b114136
$!!             - removed "$" sign near line 371 which was a 
$!!               continuation from the previous "nobox -" line.
$!!	19-Apr-2007 (upake01) b118387
$!!             - "IIMKCLUSTER -NODENUMBER 1" fails with a usage message.
$!!               Modify the "else" part of 
$!!                      if p1 .eqs. "-TEST" .or. P3 .eqs. "-DEBUG"
$!!               to check for "-NODENUMBER" in P1.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to do a selective purge by retaining only the latest
$!!             version of CONFIG.DAT file among all versions generated.
$!!	06-Sep-2007 (upake01)
$!!         BUG 119079 - Fix leading space problem with date for dates 1 to 9 of the month.
$!!             Lexical function "f$time()" returns date and timestamp as
$!!             " 6-SEP-2007 12:34:56.78".  Use f$cvtime(f$time(),"ABSOLUTE").
$!!	02-jan-2008 (joea)
$!!	    Disable CNA features.
$!!	02-jan-2008 (joea)
$!!	    Discontinue use of cluster nicknames.
$!!     12-Feb-2008 (upake01)
$!!         BUG 118387 - Modify to allow users to run "IIMKCLUSTER  -NODENUMBER x" where
$!!             "x" is a node number user wants to use.  This functionality never worked
$!!             due to not setting the user supplied node number as a logical.
$!!     28-Feb-2008 (upake01)
$!!         SIR 120019 - Remove lines that set "100,000" to "rcp.lock.lock_limit" allowing
$!!             the value to get calculated using the formula in DBMS.CRS.
$!!	11-Nov-2008 (upake01)
$!!         SIR 118402 - Skip running "Selective Purge" routine when the command procedure
$!!             terminates with errors.
$!!     24-Jun-2010 (horda03) Bug 122555
$!!             Call check_lglk_mem to ensure Ingres will connect to a
$!!             new LG/LK shared memory section.
$!!     16-Nov-2010 (loera01)
$!!         Invoke iicvtgcn to merge node-specific GCN info into the
$!!             clustered GCN database.
$!!
$!  DEST = utility
$! ------------------------------------------------------------------------
$!
$_START_IIMKCLUSTER:
$!
$  self         = "IIMKCLUSTER"
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
$! Set up shared DCL routibes.
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
$
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
$    ENDIF
$ ENDIF
$!
$!
$! Display copyright, and set 'PID', 'VERSION', and 'RELEASE_ID'
$!
$ copyright_banner 
$ if iishlib_err .ne. ii_status_ok then EXIT
$
$! ------------------------------------------------------------------------
$!
$  'self'IDSLST   		== ""
$  'self'NODESLST   		== ""
$!
$!#
$!#   Set up some global variables
$!#
$    'self'_subStatus  == 0
$    false              = 0
$    true               = 1
$    say 		:= type sys$input
$    echo 		:= write sys$output 
$! 
$!
$    cna_enabled = 0
$    IIAffinityClass    = "iicna"
$    IIAffinitySvr      = "CN"
$!
$    CLUSTER_MAX_NODE   = 15
$!
$!   Set up error codes
$    STS_NO_MATCHES = %X08D78053
$    SS$_ABORT = 44
$!
$    set_ingres_symbols "quiet"
$!
$!  Ingres tools:
$!
$   iisulock :== $II_SYSTEM:[INGRES.UTILITY]iisulock
$!
$ IF F$TRNLNM("II_CONFIG") .NES. "" .AND. -
     F$SEARCH("II_CONFIG:config.dat") .NES. ""
$ THEN
$    IF "''IISHLIB_INFO'" .NES. ""
$    THEN
$       nobox -
	"Environment II_CONFIG is defined and contains a config.dat file" -
	" "
$    ENDIF
$ ELSE
$    nobox -
	"The Ingres logical II_CONFIG was not found, using II_SYSTEM" -
	" "
$!
$    DEFINE/JOB II_CONFIG II_SYSTEM:[ingres.files]
$ ENDIF
$!
$!   Start of MAINLINE
$!
$!   Set up defaults and useful constants.
$!
$ set noon
$! 
$ test 		 = 0
$ status 	 = 1
$ subStatus 	 = 0
$ nodeId 	 = 0
$ iisulocked 	 = 0
$ resp 		 = ""
$ fileext 	 = "_" + f$cvtime() - "-" - "-" - " " - ":" - ":" - "."
$ config_file 	 = ""
$ CONFIG_HOST 	 = f$getsyi( "NODENAME" )
$ ii_system 	 = ""
$ ii_installation = f$trnlnm("II_INSTALLATION")
$ num_parts 	 = 0
$ warnings 	 = 0
$ have_protect_dat = 0
$!
$ user_uic 	 = f$user() 
$ tmpfile 	 = "ii_system:[ingres]iimklog.''CONFIG_HOST'_tmp"
$ clusterid 	 = ""
$!
$!- subroutine call symbols
$!
$ usage 		:= call _usage
$ cleanup_iimkcluster 	:= call _cleanup_iimkcluster
$!
$!   Clear any unwanted environment variables.
$!
$ if p1 .eqs. "-NODENUMBER" .and. p2 .nes. ""
$ then
$     nodeId = f$integer(p2)
$     validate_node_number 'nodeId'
$     if iishlib_err .eq. ii_status_error
$     then
$         usage "''self'" "''p1'" "''p2'" "''p3'"
$         goto _EXIT_FAIL
$     else
$         DEFINE/NOLOG/proc iimkcluster_ii_cluster_id 'nodeId'
$     endif
$ endif
$!
$ 'self'BATCH			== 0
$ 'self'READ_RESPONSE		== 0
$ 'self'WRITE_RESPONSE		== 0
$ 'self'RESINFILE		== ""
$!
$ if p1 .eqs. "-BATCH" .or. P3 .eqs. "-BATCH"
$ then
$    info_box "Batch: ON"
$    'self'BATCH 		==1
$ endif
$!
$ $DEBUGIT =="PIPE ss = 1 ! "
$ FAKEIT   == 0
$!
$ if p1 .eqs. "-TEST" .or. P3 .eqs. "-DEBUG"
$ then
$    if P1 .eqs. "-TEST" 
$    then
$       test = 1
$    endif
$    if P3 .eqs. "-DEBUG" 
$    then
$       $DEBUGIT =="PIPE ss = 1 ; "
$    endif
$    info_box "Debug and Test mode on" " "
$    box "using the following external variable to increase debugging" -
	 "      SHELL_DEBUG    = ''SHELL_DEBUG'" -
	 "      IISHLIB_DEBUG  = ''IISHLIB_DEBUG'" -
	 "      IISHLIB_VERIFY = ''IISHLIB_VERIFY'"
$ else
$    if (p1 .nes. "-NODENUMBER") .and. (p1 .nes. "")
$    then
$        usage "''self'" "''p1'" "''p2'" "''p3'"
$        goto _EXIT_FAIL
$    endif
$ endif
$! 
$!
$!# Check II_SYSTEM has been defined and exists
$!# At the same time check other values that are essential and define and 
$!# warn of all exceptions
$!#
$!  Sanity Check
$ check_sanity
$ IF iishlib_err .NE. ii_status_ok
$ THEN
$     error_box "II_SYSTEM must be set to the root of the Ingres directory tree before you may run ''self'.  Please see the Installation Guide for instructions on setting up the Ingres environment."  
$     exit 1
$ ENDIF
$!
$ check_path "bin"
$ if iishlib_err .eq. ii_status_error then goto _EXIT_FAIL
$ check_path "utility"
$ if iishlib_err .eq. ii_status_error then goto _EXIT_FAIL
$!
$ if  f$search( "ii_system:[ingres.files]config.dat" ) .eqs. ""
$ then
$     error_box "Cannot find ii_system:[ingres.files]config.dat." " " -
$	        "Please check your Ingres environment."
$     exit 1
$ endif
$!
$!
$!#   Get "ingres" user since user no longer needs to literally be 'ingres'.
$!
$   getressym 'self' "''CONFIG_HOST'" setup.owner.user
$   INGUSER = iishlib_rv1
$!
$   IF "''INGUSER'" .EQS. "" 
$   THEN 
$      INGUSER="ingres"
$   ENDIF
$!
$   check_user
$!
$ if test .eq. 0 
$ then
$   IF iishlib_err .ne. ii_status_ok
$   THEN 
$      GOTO _EXIT_FAIL
$   ENDIF
$ else
$    info_box "TEST MODE DIS-ABLE USER CHECKING"
$ endif
$!
$! Check the transaction log configuration.
$!
$  getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_parts 
$  NUM_PARTS = F$INTEGER(iishlib_rv1)
$ if NUM_PARTS .eq. 0 
$ then
$    error_box  "Ingres transaction log improperly configured. " - 
		" " -
		"Cannot determine the number of log segments from ii.''CONFIG_HOST'.rcp.log.log_file_parts. "
$    goto _EXIT_FAIL
$ endif
$!
$!   Check if already clustered.
$!
$ if test .eq. 1 
$ then 
$    iisetres ii.'CONFIG_HOST.config.cluster_mode  "OFF"
$ endif
$!
$ getressym 'self' "''CONFIG_HOST'" config.cluster_mode
$ VALUE = iishlib_rv1
$ if VALUE .eqs. "on"
$ then
$    error_box "Ingres is already set up in a cluster configuration."
$    goto _exit_fail
$ endif
$!
$!   Check if instance is up.
$!
$ if test .eq. 0 
$ then
$    check_lglk_mem "''ii_installation'"
$    if iishlib_err .eq. ii_status_warning
$    then 
$      error_box -
	"Ingres must be shutdown before converting to a cluster configuration." -
	" " -
	"Please use """"ingstop"""" to stop your installation before running ''self' again."
$      goto _EXIT_FAIL
$    else
$       info_box "Ingres is not online - OK to continue"
$    endif
$ else
$    info_box "TEST RUN rcpstat -online -silent NOT CHECKED"
$ endif
$!
$!
$! logging_offline: Logging system is off line
$!
$!   Prompt for user confirmation.
$!
$ echo " "
$ IF (ii_installation_must_exist .eqs. "TRUE")
$ THEN
$      nobox " "  - 
       "iimkcluster will convert Ingres instance """"''ii_installation'"""" on ''CONFIG_HOST' into the first node of an Clustered Ingres instance."
$ ELSE
$      nobox " "  - 
       "iimkcluster will convert Ingres instance of SYSTEM LEVEL on ''CONFIG_HOST' into the first node of an Clustered Ingres instance."
$ ENDIF
$!
$ IF 'self'BATCH .EQ. false
$ THEN
$    PIPE confirm_intent
$    IF iishlib_rv1 .NES. "y"
$    THEN
$       goto _EXIT_OK
$    ENDIF
$ ELSE
$    $DEBUGIT  box "Looks OK - lets move on" 
$ ENDIF
$!
$! Check if STAR is configured.
$!
$ getressym 'self' "''CONFIG_HOST'" ingstart.*.star
$ VALUE =  f$integer(iishlib_rv1)
$ reset_star_start = 0
$ if VALUE .ne. 0
$ then
$    warn_box -
   "The Ingres STAR distributed database server uses two phase commit (2PC), and therefore is not currently suppported by Clustered Ingres." -
   " " -
   "Your installation is currently configured for one or more STAR servers." 
$!
$   nobox " " -
   "If you decide to continue, the startup count will be set to zero, and no STAR servers will be configured to start." -
   " "
$!
$    IF 'self'BATCH .EQ. false
$    THEN
$       PIPE confirm_intent
$       if iishlib_rv1 .nes. "y"
$       THEN
$          goto _EXIT_OK
$      ENDIF
$    ELSE
$       $DEBUGIT  box "Looks OK - lets move on" 
$    ENDIF
$    reset_star_start = 1
$ endif
$!
$!   Lock config.dat and backup files.  We do this before getting the node #
$!   because sadly these routines update the config.dat file.
$!
$! Lock the configuration file. 
$!
$    lock_config_dat "''self'" "''iisulocked'"
$    iisulocked = F$INTEGER(iishlib_rv1)
$!
$!  We must be able to lock the config.dat file.
$!
$    IF iishlib_err .ne. ii_status_ok
$    THEN
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    backup_file config.dat "''fileext'"
$!
$    $DEBUGIT dir II_CONFIG:config.dat*/sin
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    IF F$Search( "II_SYSTEM:[ingres.files]protect.dat" ) .NES. ""
$    THEN
$	 have_protect_dat = 1
$        backup_file protect.dat "''fileext'"
$        $DEBUGIT dir II_CONFIG:protect.dat*/sin
$        IF iishlib_err .NE. ii_status_ok
$        THEN
$           GOTO _EXIT_FAIL 
$        ENDIF
$    ENDIF
$!    
$! backup_ok: continue with processing
$  have_id = f$TRNLNM("iimkcluster_ii_cluster_id")
$  have_id = F$INTEGER(have_id) 
$!
$ if have_id .lt. 1 .or. have_id .gt. CLUSTER_MAX_NODE
$ then 
$   nobox -
	"Please specify a unique integer node number in the range 1 through ''CLUSTER_MAX_NODE'." -
	" "
$   nobox "The surviving node with the lowest node number in a partially failed cluster is responsible for recovery." " "
$   nobox "Therefore the lowest number to be assigned should be assigned to the most powerful machine planned for inclusion in the Ingres cluster." " "
$!
$!ENTER_NODE_ID: Prompt user if interactive mode.
$!    #
$!    #  Get node number
$!    #
$    gen_nodes_lists
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       GOTO _EXIT_FAIL 
$    ENDIF
$ endif
$!
$!   Requires/set the following
$!   CONFIG_HOST   - required
$!   CLUSTER_ID    sets
$!
$!
$ if have_id .lt. 1 .or. have_id .gt. CLUSTER_MAX_NODE
$ then
$    get_node_number "''self'" "" "''CONFIG_HOST'"
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       GOTO _EXIT_FAIL
$    ENDIF
$    DEFINE/NOLOG ii_cluster_id 'II_CLUSTER_ID'
$ else
$    II_CLUSTER_ID = have_id
$    DEFINE/NOLOG ii_cluster_id 'have_id'
$ endif
$!
$    ii_clu_id = F$INTEGER(II_CLUSTER_ID)
$    II_CLUSTER_ID = ii_clu_id
$    IF (ii_installation_must_exist .eqs. "TRUE")
$    THEN
$       message_box " " "Existing ''VERSION' Ingres instance """"''ii_installation'"""" on host ''CONFIG_HOST' will be converted to the initial node of an Ingres Cluster using node number ''II_CLUSTER_ID'." " "
$    ELSE
$       message_box " " "Existing ''VERSION' Ingres SYSTEL LEVEL instance on host ''CONFIG_HOST' will be converted to the initial node of an Ingres Cluster using node number ''II_CLUSTER_ID'." " "
$    ENDIF
$    IF 'self'BATCH .EQ. false
$    THEN
$       PIPE confirm_intent
$       if iishlib_rv1 .nes. "y"
$       THEN
$          goto _EXIT_OK
$       ENDIF
$    ELSE
$       $DEBUGIT box "Ingres installation looks suitable to convert to cluster installation"  
$    ENDIF
$!------------------------------------------------------------------------
$! The Rubicon is crossed. - Start updates
$!------------------------------------------------------------------------
$!
$ nobox " "  -
       "Converting installation:  please wait ..." -
       " "
$    log_config_change_banner  "BEGIN CONVERT TO CLUSTER" "''CONFIG_HOST'" "''self'" -
		"''II_CLUSTER_ID'" "''nname'" 18 
$    $DEBUGIT dir II_CONFIG:config.log*/sin
$!
$!   Create new sub-directories.
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "files" "memory"
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "files" "memory" "''CONFIG_HOST'"
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "admin"
$ iicreate_dir "s:rwe,o:rwe,g:re,w:e" "admin" 'CONFIG_HOST'
$!
$! - |check Directory 
$!
$  if iishlib_err .eq. ii_status_error
$  then
$     error_box "Failed to successfully create the Cluster Node specific directories"
$     goto _EXIT_FAIL
$  endif 
$!  
$!   Rename transaction log files.
$!
$  rename_file :== rename/nolog
$  oktogo = true
$  getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_name 
$  primary_log_name = iishlib_rv1
$!
$ if primary_log_name .eqs. "" 
$ then 
$    primary_log_name = "ingres_log"
$    warn_box "Unable to determine TX logfile name using ''primary_log_name'"
$ else
$    info_box "Identified TX logfile name : ''primary_log_name'"
$ endif
$!
$  getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_name 
$  dual_log_name = iishlib_rv1
$!
$ if dual_log_name .eqs. "" 
$ then 
$    warn_box "Unable to determine TX DUAL logfile name, skipping all DUAL log setup."
$ else
$    info_box "Identified DUAL TX logfile name : ''dual_log_name'"
$ endif
$!
$ if dual_log_name .eqs. primary_log_name 
$ then 
$    warn_box "Both the Primary and Dual log files have the same name" -
	" " -
	"If the Directory names resolve to the same value this will cause an access confilct." -
	" " -
	"Dual log file name and directories must be different"
$ endif
$!
$ part = 0
$!
$_GET_CONFIG_INFO:
$!
$ if part .ge. num_parts 
$ then 
$    goto _get_config_done 
$ endif
$!
$ part = part + 1
$! 
$ getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_'PART' 
$ DIRSPEC = iishlib_rv1
$!
$ IF DIRSPEC .EQS. ""
$ THEN 
$    info_box "Cannot find configuration information for ""iigetres""" -
	      "ii.''CONFIG_HOST'.rcp.log.log_file_''part'"
$    oktogo = false
$    warnings = 1
$ ELSE
$!
$    gen_log_name 'PRIMARY_LOG_NAME' 'PART' " " 'SHELL_DEBUG'
$    OLD_NAME = iishlib_rv1 
$!
$    gen_log_name 'PRIMARY_LOG_NAME' 'PART' 'CONFIG_HOST' 'SHELL_DEBUG'
$    CLU_NAME = iishlib_rv1 
$!
$    IF F$LOCATE(":",DIRSPEC) .EQ. F$LENGTH(DIRSPEC) -1
$    THEN
$       DIRSPEC = DIRSPEC + "[" 
$    ELSE
$       DIRSPEC = DIRSPEC - "]" -".]" + "."
$    ENDIF
$!
$    SPEC = "''DIRSPEC'ingres.log]''OLD_NAME'"
$    IF F$SEARCH(SPEC) .EQS. ""
$    THEN
$       error_box "Cannot locate a portion of the transaction log:" -
	" ''SPEC'"
$       warnings = 1
$	oktogo = false
$    ENDIF
$    SPECCLU = "''DIRSPEC'ingres.log]''CLU_NAME'"
$    IF F$SEARCH("''DIRSPEC'ingres.log]''CLU_NAME'") .NES. "" 
$    THEN
$       error_box "A file already exists with the new name of this portion of the transaction log:" -
	" ''SPECCLU'"
$       warnings = 1
$	oktogo = false
$    ENDIF
$!
$    IF oktogo .eq. true
$    THEN
$       rename_file 'SPEC' 'SPECCLU'
$!
$       IF F$SEARCH(SPECCLU) .EQS. ""
$       THEN
$          error_box "Rename of transaction file ''SPEC' to ''SPECCLU' failed!"
$          warnings = 1
$	   oktogo = false
$       ENDIF
$    ENDIF
$ ENDIF
$! 
$ IF dual_log_name .eqs. "" 
$ THEN
$    GOTO _GET_CONFIG_INFO
$ ELSE
$!
$    getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_'PART' 
$    DIRSPEC = iishlib_rv1
$!
$    IF DIRSPEC .EQS. ""
$    THEN 
$       info_box "Cannot find configuration information for ""iigetres""" -
	      "ii.''CONFIG_HOST'.rcp.log.dual_log_''part'" " "	-
	     "Assuming no DUAL logging enabled."
$    ELSE
$!
$       gen_log_name "''DUAL_LOG_NAME'" "''PART'" " " 'SHELL_DEBUG'
$       OLD_NAME = iishlib_rv1 
$!
$       gen_log_name "''DUAL_LOG_NAME'" "''PART'" "''CONFIG_HOST'" 'SHELL_DEBUG'
$       CLU_NAME = iishlib_rv1 
$!
$       IF F$LOCATE(":",DIRSPEC) .EQ. F$LENGTH(DIRSPEC) -1
$       THEN
$          DIRSPEC = DIRSPEC + "[" 
$       ELSE
$          DIRSPEC = DIRSPEC - "]" -".]" + "."
$       ENDIF
$!
$       SPEC    = F$SEARCH("''DIRSPEC'ingres.log]''OLD_NAME'")
$       SPECCLU = F$SEARCH("''DIRSPEC'ingres.log]''CLU_NAME'")
$!
$       IF SPECCLU .NES. "" 
$       THEN
$          error_box "EXIST: Log file found:" "''SPECCLU'" -
		  "Aborting ''self' processing after checking all log files"
$          warnings = 1
$	   oktogo = false
$          rename_file :== "RENAME !"
$       ENDIF
$!
$       IF SPEC .NES. "" .AND. CLU_NAME .NES. ""
$       THEN
$          rename_file 'SPEC' 'DIRSPEC'ingres.log]'CLU_NAME'
$!
$          SPEC = F$SEARCH("''DIRSPEC'ingres.log]''CLU_NAME'")
$          IF SPEC .EQS. ""
$          THEN
$             warn_box "MVfile FAILED: ''DIRSPEC'ingres.log]''CLU_NAME' not found"
$             warnings = 1
$	   ELSE
$	      if oktogo .eq. false
$ 	      then
$                warn_box "MVfile ABORTED: ''DIRSPEC'ingres.log]''CLU_NAME' exists."
$             endif
$          ENDIF
$       ELSE
$          box "Nofile: Cannot find ''DIRSPEC'ingres.log]''OLD_NAME' - missing. " -
  	    "or the new name was blank : ''CLU_NAME'"
$          WARNINGS=1
$       ENDIF
$    ENDIF
$ ENDIF
$!
$ goto _GET_CONFIG_INFO
$!
$_GET_CONFIG_DONE:
$!
$ if oktogo .eq. true .and.  warnings .ne. 0
$ then
$    warn_box "TX logfiles have beeen renamed. There were some problems identified" -
      	      " " "Please refer to the previous output for details"
$ else
$    if oktogo .eq. true 
$    then
$       info_box "TX logfiles have been renamed successfully"
$    else
$       error_box "TX logfiles have NOT been renamed. There were some problems identified" -
      	      " " "Please refer to the previous output for details"
$       goto _EXIT_FAIL
$    endif
$ endif
$ if oktogo .eq. false
$ then 
$   goto _EXIT_FAIL
$ endif
$!
$!   Modify config.dat
$ if reset_star_start .ne. 0
$ then
$     iisetres ii.'CONFIG_HOST.ingstart.*.star "0"
$ endif
$ clusterid = f$trnlnm("II_CLUSTER_ID")
$ clusterid2 = F$FAO("!2ZL",'clusterid')
$ cfu_host  = F$EDIT(CONFIG_HOST,"UPCASE")
$!
$ iisetres ii.'CONFIG_HOST.config.cluster.id "''clusterid'"
$ iisetres ii.'CONFIG_HOST.lnm.ii_cluster_id "''clusterid'"
$ iisetres ii.'CONFIG_HOST.config.cluster_mode  "ON"
$ iisetres "ii.''CONFIG_HOST'.lnm.ii_gcn''II_INSTALLATION'_lcl_vnode" -
	"''cfu_host'"
$ iisetres "ii.''CONFIG_HOST'.gcn.local_vnode"   "''cfu_host'"
$!
$! Convert GCN files to cluster format.
$!
$ iicvtgcn -c
$!
$! - permisions
$    getressym 'self' "''CONFIG_HOST'" recovery.*.vms_privileges 
$    VMSPRIV = iishlib_rv1 - ")" - ",tmpmbx" - ",sysnam" + ",tmpmbx" + ",sysnam" + ")"
$    iisetres "ii.''CONFIG_HOST'.recovery.*.vms_privileges"   "''VMSPRIV'"
$!
$    getressym 'self' "''CONFIG_HOST'" dbms.*.vms_privileges 
$    VMSPRIV = iishlib_rv1 - ")" - ",tmpmbx" - ",sysnam" + ",tmpmbx" + ",sysnam" + ")"
$    iisetres "ii.''CONFIG_HOST'.dbms.*.vms_privileges"   "''VMSPRIV'"
$!
$!   Set enqueue limit to infinity for all server classes.
$    iisetres "ii.''CONFIG_HOST'.recovery.*.vms_enqueue_limit" "32767"
$    iisetres "ii.''CONFIG_HOST'.dbms.*.vms_enqueue_limit" "32767"
$    dbmsclass="*"
$    OPEN IICFG II_CONFIG:config.dat /READ/ERROR=_ERR_OPEN_CFG
$ _READ_CFG_LOOP:
$    READ IICFG iicfgrec /ERROR=_ERR_READ_CFG /END_OF_FILE=_EOF_READ_CFG
$!
$!   Parse parameter line looking for a different dbms class.
$    iicfgrec = F$Element(0,":",iicfgrec)
$    IF F$Element(2,".",iicfgrec) .EQS. "dbms" .and. -
        F$Element(4,".",iicfgrec) .EQS. "active_limit" .and. -
        F$Element(3,".",iicfgrec) .NES. "''dbmsclass'"
$    THEN
$        dbmsclass=F$Element(3,".",iicfgrec)
$        iisetres "ii.''CONFIG_HOST'.dbms.''dbmsclass'.vms_enqueue_limit" -
          "32767"
$    ENDIF
$    GOTO _READ_CFG_LOOP
$ _ERR_READ_CFG:
$    CLOSE IICFG
$    error_box "Error reading II_CONFIG:config.dat, status=''$STATUS'"
$    GOTO _DONE_READ_CFG
$ _ERR_OPEN_CFG:
$    error_box "Could not open II_CONFIG:config.dat, status=''$STATUS'"
$    GOTO _DONE_READ_CFG
$ _EOF_READ_CFG:
$    CLOSE IICFG
$ _DONE_READ_CFG:
$!
$ info_box "Updated configuration parameters"
$!
$!   
$! Add default Node affinity class.
$!
$ if cna_enabled
$ then
$ CNA  		= "''IIAffinityClass'''clusterid2'"
$ CNA_server 	= "''IIAffinitySvr'''clusterid2'" 
$!
$ iiappend_class "''self'" "''CONFIG_HOST'" "config.dat"   "''CONFIG_HOST'"  "''CNA'" ""
$ IF have_protect_dat
$ THEN
$     iiappend_class "''self'" "''CONFIG_HOST'" "protect.dat"  -
       "''CONFIG_HOST'"  "''CNA'" ""
$ ENDIF
$ iisetres ii.'CONFIG_HOST'.ingstart.'CNA'.dbms 	    "0"
$ iisetres ii.'CONFIG_HOST'.dbms.'CNA'.cache_name 	    "''CNA'"
$ iisetres ii.'CONFIG_HOST'.dbms.'CNA'.cache_sharing 	    "OFF"
$ iisetres ii.'CONFIG_HOST'.dbms.'CNA'.class_node_affinity  "ON"
$ iisetres ii.'CONFIG_HOST'.dbms.'CNA'.server_class 	    "''CNA_server'"
$!
$   info_box "Affinity class added to configuration files:" -
		"HOST            = ''CONFIG_HOST'" -
        	"Server Name     = ''CNA'"  	   -
		"Server Class    = ''CNA_server'"
$ endif
$!
$!  Force DMCM to "ON" for all classes - JIC
$!  Remove from protect.dat file.
$!
$ IF have_protect_dat
$ THEN
$     PIPE SEARCH/NOWARN/NOLOG/MATCH=NOR II_CONFIG:protect.dat -
        ".dmcm" -
        > II_CONFIG:'self'_protect.dat
$     rename_file II_CONFIG:'self'_protect.dat II_CONFIG:protect.dat 
$ ENDIF
$!
$   PIPE SEARCH/NOWARN/NOLOG/MATCH=AND II_CONFIG:config.dat -
        ".dmcm", "off" -
        > II_CONFIG:'self'_config.dat
$   OPEN/READ dmcmin II_CONFIG:'self'_config.dat
$!
$_DMCM_NEXT_0:
$   READ/END_OF_FILE=_DMCM_LAST_0 dmcmin irec
$   node  = F$ELEMENT(0,":",irec)
$   value = F$ELEMENT(1,":",irec)
$   iisetres "''node'" "ON"
$   GOTO _DMCM_NEXT_0
$_DMCM_LAST_0:
$!
$! tidy up from DMCM processing
$   CLOSE dmcmin
$   DELETE/NOLOG/NOCONF II_CONFIG:'self'_config.dat;*
$!
$!
$   info_box """""''self'"""" has forced DMCM to """"ON"""" for all server classes." -
		" " "It is mandatory for all active DBMS servers to run with DMCM" 	-
		"in a Clustered Ingres installation."
$!
$ if warnings .eq. 1
$ then
$    warn_box   -
	"Caution, during conversion, ''self' detected one or more configuration inconsistencies.  Please resolve the detected problems before using Ingres in the Cluster configuration."
$ endif
$ nobox "Ingres has now been converted to an Ingres Cluster configuration." -
	" " -
	"You may now use """"iisunode"""" to set up additional Ingres Cluster nodes." -
	" "
$ nobox "Note: Certain text log files found in : """"II_SYSTEM:[ingres.files]"""", such as IIACP.LOG and IIRCP.LOG will remain in place, but new entries will be written to log files with names *decorated* with the associated Ingres Cluster node."
$ nobox	"(e.g. IIACP.LOG_''CONFIG_HOST', IIRCP.LOG_''CONFIG_HOST')"
$!
$ IF (ii_installation_must_exist .eqs. "TRUE")
$ THEN
$    add_install_history "''VERSION'" "Converted to first node of Ingres cluster ''II_INSTALLATION' using node number ''II_CLUSTER_ID'."
$ ELSE
$    add_install_history "''VERSION'" "Converted to first node of Ingres cluster SYSTEM LEVEL using node number ''II_CLUSTER_ID'."
$ ENDIF
$ goto _EXIT_OK
$!
$!  
$!-------------------------------------------------------------------
$! Exit labels.
$!
$_EXIT_FAIL:
$ complete_message = "Failed at"
$ restore_file "config.dat" 'fileext'
$ IF have_protect_dat
$ THEN
$     restore_file "protect.dat" 'fileext'
$ ENDIF
$!
$ status = SS$_ABORT
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
$_EXIT_OK_2:
$   unlock_config_dat "''self'" "''iisulocked'"
$!
$   cleanup "''self'"   
$!
$  cleanup_iimkcluster
$!
$!
$!This is the only clean exit point in the script
$ end_time = F$TIME()
$ nobox "''self' ''complete_message' ''end_time'"
$!
$ EXIT 
$!
$!
$!
$!------------------------------------------------------------------------
$!
$!   Complain about bad input.
$!
$!      $1 = bad parameter 
$!
$ _usage: subroutine
$ echo "invalid parameter ''p1' ''p2' ''p3'."
$ echo "Usage: ''p1' [ -nodenumber <node_number> ] "
$ exit
$ endsubroutine
$!
$!   Cleanup 
$!
$ _cleanup_iimkcluster: subroutine
$!
$!  Delete symbols
$!
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
$  II_CLUSTER_ID       == ""
$  DELETE/SYM/GLOBAL   II_CLUSTER_ID
$ exit
$ endsubroutine
