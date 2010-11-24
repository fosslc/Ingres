$ GOTO _START_IIUNCLUSTER
$!  Copyright (c) 2002, 2009 Ingres Corporation
$! 
$! 
$!  $Header $
$!
$!  Name: iiuncluster - Convert Ingres from Clustered to non-Clustered.
$!
$!  Usage:
$!	iiuncluster [-force [<datestamp>]
$!
$!  Description:
$!	Converts the Ingres instance running on the current host machine
$!	to a non-clustered Ingres instance.  Ingres will be disabled
$!	on all other nodes for this instance.
$!
$!	This script must be run as ingres, and while installation is
$!	down.
$!
$! Parameters:
$!	-force	: if present, makes iiuncluster run without prompts.
$!		  this parameter should only be used by iimkcluster
$!		  to undo a partial conversion.
$!
$! Exit status:
$!	0	OK - conversion to non-clustered Ingres successful.
$!	1	User decided not to procede.
$!	3	Ingres instance fails basic setup sanity checks.
$!	4	Ingres is apparently not set up.
$!	5	Ingres is not set up for clusters.
$!	6	Ingres must be shutdown before conversion.
$!	7	Recoverable transactions must be scavanged before convert.
$!	10	Must be "ingres" to run script.
$!	11	Process failed or was interrupted during convert.
$!	12	Configuration file is locked.
$!
$!! History
$!!	15-may-2005 (bolke01)
$!!		Initial version.
$!!             Based on the UNIX version iiuncluster.sh
$!!	13-jun-2005 (bolke01)
$!!	  	Changed position of some variable definitions.
$!!		Replaced Alpha with lexical function
$!!		Removed call to ingsysdef.
$!!		Moved iisulock symbol definition to iishlib
$!!		Implementented a common completion message format
$!!	08-aug-2005 (devjo01)
$!!		Cleanup.
$!!	19-sep-2005 (devjo01)
$!!		- Correct problem with iiuncluster destroying log files
$!!		it should have renamed.
$!!	        - Script was not handling correctly transaction log
$!!		locations that were not simple opaque devices.
$!!		- Script did not remove parameter entries for other
$!!		nodes in cofig.dat if there were more than two nodes.
$!!	27-sep-2005 (devjo01) b115278
$!!		Remove cluster_max_node.
$!!     26-oct-2005 (devjo01) b115463
$!!		Remove MEMORY and ADMIN node specific sub-directories,
$!!		and "decorated" files from FILES.NAME.
$!!     07-aug-2006 (abbjo03) b115463
$!!	    Correct 26-oct-2005 change. The MEMORY files are actually in
$!!	    FILES.MEMORY.
$!!	05-Jan-2007 (bonro01)
$!!	    Change Startup Guide to Installation Guide.
$!!	28-feb-2007 (upake01) b107385
$!!             - Check if II_SYSTEM logical exists in the GROUP table.
$!!               If it does not, check if it exists in the SYSTEM table.
$!!               If it does, skip checking contents of II_INSTALLATION
$!!               as it is not required for SYSTEM level installation.
$!!	19-apr-2007 (upake01) b118388
$!!             - Remove "iicna" and "nickname" entries in CONFIG.DAT
$!!               in "_clean_config"
$!!             - When the job is run with parameters, display message
$!!               whether restoring the configuration file from the current
$!!               config file or saved config file.  Hold on displaying this
$!!               message until after the check is done whether the Ingres
$!!               instance is up.  If Ingres is up, the job needs to abort
$!!               and no point in displaying the informational message about
$!!               what configuration file will be restored.
$!!             - Added missing "$ error_condition '$severity' " line after
$!!               "$ rcpstat -any_csp_online -silent" when checking to see
$!!               ingres is up on other nodes in the cluster.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to do a selective purge by retaining only the latest
$!!             version of CONFIG.DAT file among all versions generated.
$!!     06-Sep-2007 (upake01)
$!!         BUG 119079 - Fix leading space problem with date for dates 1 to 9 of the month.
$!!             Lexical function "f$time()" returns date and timestamp as
$!!             " 6-SEP-2007 12:34:56.78".  Use f$cvtime(f$time(),"ABSOLUTE").
$!!	07-jan-2008 (joea)
$!!	    Discontinue use of cluster nicknames.
$!!     30-Apr-2008 (upake01) Bug 120319
$!!             Handle case where protect.dat is absent.
$!!     06-Jun-2008 (upake01) Bug 120222
$!!             Remove "-mkresponse" and "-exresponse" options and change
$!!             text for usage accordingly.
$!!     11-Nov-2008 (upake01)
$!!         SIR 118402 - Skip running "Selective Purge" routine when the command procedure
$!!             terminates with errors.
$!!	30-apr-2009 (joea)
$!!	    Remove undocumented or incompletely implemented options.
$!!     16-Nov-2010 (loera01)
$!!         Invoke iicvtgcn to extract the clustered GCN database into
$!!         a database normalized for an unclustered environment.
$!!
$!!  DEST = utility
$!! ------------------------------------------------------------------------
$_START_IIUNCLUSTER:
$!#
$!#   Set up process specific variables
$!#
$  self         = "IIUNCLUSTER"
$!
$! Get the start time of the job.  It will be used later to do a selective purge
$ start_time = f$cvtime(f$time(),"ABSOLUTE")
$! Get position of space between YYYY & HH
$ space_pos = f$locate(" ",start_time)
$! Change " " to ":"
$ start_time[space_pos*8,8] = 58
$! start time is now in format "23-MAY-2007:15:30:45.90"
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
$! Set up shared DCL routines.
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
$!
$!------------------------------------------------------------------
$! Ingres tools
$!
$  iisulock :== $II_SYSTEM:[INGRES.UTILITY]iisulock
$!------------------------------------------------------------------
$!   
$  complete_message = "Completed at"
$!------------------------------------------------------------------
$!
$  if F$TRNLNM("inself")  .nes. "" then CLOSE inself
$  if F$TRNLNM("infiles")  .nes. "" then CLOSE infiles
$  if F$TRNLNM("outfiles") .nes. "" then CLOSE outfiles
$!
$!#
$!#   Set up some global variables
$!#
$    false              = 0
$    true               = 1
$    say                := type sys$input
$    echo               := write sys$output 
$    pid		= F$GETJPI("","PID")
$! 
$! 
$  'self'IDSLST        == ""
$  'self'NODESLST      == ""
$!
$!   Set up error codes
$    STS_NO_MATCHES = %X08D78053
$    SS$_ABORT = 44
$!
$!
$ IF F$TRNLNM("II_CONFIG") .NES. "" .AND. -
     F$SEARCH("II_CONFIG:config.dat") .NES. ""
$ THEN
$	info_box  -
	"Environment II_CONFIG is defined and contains a config.dat file" -
	" "
$ ELSE
$	nobox -
	"The Ingres logical II_CONFIG was not found, using II_SYSTEM" -
	" "
$   DEFINE/JOB II_CONFIG II_SYSTEM:[ingres.files]
$ ENDIF
$!
$ remove_file  		:== call _remove_file
$ rename_file  		:== call _rename_file
$ usage 		:== call _usage_'self'
$ cleanup_'self'	:== call _cleanup_'self'
$ clean_config		:== call _clean_config
$!
$!#
$!#   Start of MAINLINE
$!#
$!
$!#   Set up defaults and useful constants.
$ set noon
$! 
$ test           = 0
$ status         = 1
$!
$ subStatus      = 0
$ nodeId         = 0
$ iisulocked     = 0
$ resp           = ""
$ 'self'_subStatus  == 0
$ fileext        = "_" + f$cvtime() - "-" - "-" - " " - ":" - ":" - "."
$ filepfx        = F$FAO("!8AS_",self) 
$ fileunq        = filepfx + fileext
$ unqext 	 = fileext
$ config_file    = ""
$ CONFIG_HOST    = f$getsyi( "NODENAME" )
$ tmpsch         = F$TRNLNM("SYS$SCRATCH")
$ tmpfile        = "''tmpsch'''fileunq'.''config_host'_tmp"
$ selffile       = "''tmpsch'''fileunq'.''config_host'_self"
$ wrtfile        = "''tmpsch'''fileunq'.''config_host'_wrt"
$ user_uic       = f$user() 
$ ii_system      = ""
$ FAKEIT	 = false
$ $DEBUGIT       = "PIPE ss = 1 ! "
$ have_protect_dat = 0
$!
$ crtself 	:== CREATE 'selffile''pid'
$ crtfiles 	:== CREATE 'wrtfile''pid'
$ wrtself 	:== WRITE inself 
$ wrtfiles 	:== WRITE infiles 
$ rdself 	:== READ inself 
$ rdfiles 	:== READ infiles 
$ opAself	:== "OPEN/APPEND inself  ''selffile'''pid'"
$ opAfiles	:== "OPEN/APPEND infiles  ''wrtfile'''pid'"
$ clsself	:== "PIPE if F$TRNLNM(""inself"")  .nes. """" then close inself"
$ clsfiles	:== "PIPE if F$TRNLNM(""infiles"") .nes. """" then close infiles"
$ rmvself 	:== "DELETE/NOLOG/NOCONFIRM  ''selffile'''pid';*"
$ rmvfiles 	:== "DELETE/NOLOG/NOCONFIRM  ''wrtfile'''pid';*"
$!
$!#   Parse parameters
$!#
$!  global symbols - cleaned up on EXIT
$!
$    'self'NODESLST          == ""
$    'self'IDSLST            == ""
$    'self'BATCH             == false
$    'self'CONFIRM           == true
$    'self'RESINFILE         == ""
$    'self'WRITE_RESPONSE    == false
$    'self'READ_RESPONSE     == false
$!
$! - local symbols
$    BATCH             	      = false
$    CONFIRM                  = true
$    RESINFILE                = ""
$    WRITE_RESPONSE           = false
$    READ_RESPONSE            = false
$!
$    verify_cur = F$VERIFY('SHELL_DEBUG')
$!
$    disp_conf_restore_msg = 0
$!   Valid values for above field are - 
$!      0 ===> no parameters specified
$!      1 ===> Store configuration file from current config file
$!      2 ===> Store configuration file from saved config file with
$!             time-stamp in P2
$!      Display informational message for values 1 & 2
$!
$   id=0
$_CHECK_PARM:
$   id = id + 1
$   parm =  p'id'
$   IF "''parm'" .NES. "" .AND. "''parm'" - "-" .NES. "''parm'"  
$   THEN
$       parm = F$EDIT(parm,"LOWERCASE")
$       IF "''parm'" - "-force" .NES. "''parm'"
$       THEN
$          BATCH           = true
$          CONFIRM         = false
$          GOTO _CHECK_PARM
$       ENDIF
$       IF parm - "-test" .NES. parm
$       THEN
$!          # This prevents actual execution of most commands, while
$!          # config.dat & symbol.tbl values are appended to the trace
$!          # file, prior to restoring the existing copies (if any)
$!          #
$	    iicreate_dirs tmp
$           DEBUG_LOG_FILE="II_SYSTEM:[ingres.tmp]''filepfx'test.output_''fileext'"
$           catlog    :== WRITE SYS$INPUT /OUT='DEBUG_LOG_FILE'
$	    FAKEIT	= true
$           id2         = id+1
$           parm        = p'id2'
$!
$!          # Check the second parameter for -test immediately
$           IF "''parm'" - "new" .NES. parm
$           THEN
$                   DEBUG_FAKE_NEW = true
$                   DEBUG_DRY_RUN  = true
$                   id             = id+1
$           ELSE 
$              IF "''parm'" - "dryrun" .NES. parm
$              THEN
$                   DEBUG_DRY_RUN  = true
$                   id             = id+1
$              ENDIF
$           ENDIF
$!
$           PIPE echo "Logging ''self' run results to ''DEBUG_LOG_FILE'"
$           PIPE echo "DEBUG_DRY_RUN  =''DEBUG_DRY_RUN'"                
$           PIPE echo "DEBUG_FAKE_NEW =''DEBUG_FAKE_NEW'"       
$!
$           PIPE echo "Starting ''self' run on ''fileext'"      > 'DEBUG_LOG_FILE'
$           PIPE echo "DEBUG_DRY_RUN  =''DEBUG_DRY_RUN'"        > 'DEBUG_LOG_FILE'
$           PIPE echo "DEBUG_FAKE_NEW =''DEBUG_FAKE_NEW'"       > 'DEBUG_LOG_FILE'
$           GOTO _CHECK_PARM
$!
$       ENDIF
$       IF parm .NES. ""
$       THEN
$           warn_box "Invalid parameter [''parm']."
$           usage "''self'" "''P1'" "''P2'"
$           goto _EXIT_OK
$       ENDIF
$       id = id+1
$       GOTO _CHECK_PARM
$   ELSE
$     IF F$TYPE(parm) .nes. "INTEGER" .and. parm .NES. ""
$     THEN
$        usage "''self'" "''P1'" "''P2'"
$        goto _EXIT_OK
$     ELSE
$        IF READ_RESPONSE .EQ. true .OR. -
            WRITE_RESPONSE .EQ. true 
$        THEN
$           RESINFILE = parm 
$	    'self'READ_RESPONSE  == READ_RESPONSE
$	    'self'WRITE_RESPONSE == WRITE_RESPONSE
$	    'self'RESINFILE 	 == RESINFILE
$           box "READ_RESPONSE==''READ_RESPONSE'" -
                "WRITE_RESPONSE==''WRITE_RESPONSE'" -
                "RESINFILE==''RESINFILE'"
$        ELSE
$	    old_cfg = F$SEARCH("II_CONFIG:config.dat_''P2'")
$	    if (P2 .nes. "0" .and. P2 .nes. "" .and. old_cfg .eqs. "")
$           then
$              error_box "II_CONFIG:config.dat_''P2' does not exist.  Please re-run IIUNCLUSTER with correct date-time stamp for saved configuration."  
$              goto _EXIT_FAIL
$           endif
$	    if P2 .eqs. "0" .or. (P2 .nes. "0" .and. P2 .nes. "" .and. old_cfg .eqs. "" )
$	    then
$              disp_conf_restore_msg = 1
$	       old_cfg = ""
$	    endif
$	    if old_cfg .nes. "" 
$	    then
$              disp_conf_restore_msg = 2
$              unqext = "''P2'"
$	    endif
$        ENDIF 
$     ENDIF 
$   ENDIF
$_NOPARMS:
$!
$! Set global confirmatoiion/batch controls
$!
$ 'self'BATCH == BATCH
$ 'self'CONFIRM == CONFIRM
$!#
$!#   Validate environment
$!#
$   oktogo=false
$!
$!# Check II_SYSTEM has been defined and exists
$!# At the same time check other values that are essential and define and 
$!# warn of all exceptions
$!#
$!  Sanity Check
$   check_sanity
$!
$   IF iishlib_err .EQ. ii_status_ok
$   THEN
$      oktogo=true
$   ELSE
$      error_box "II_SYSTEM must be set to the root of the Ingres directory tree before you may run ''self'.  Please see the Installation Guide for instructions on setting up the Ingres environment."  
$      goto _EXIT_OK
$   ENDIF
$!
$ check_path "bin"
$ if iishlib_err .eq. ii_status_error then goto _EXIT_FAIL
$ check_path "utility"
$ if iishlib_err .eq. ii_status_error then goto _EXIT_FAIL
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
$!
$! Check the transaction log configuration.
$!
$  getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_parts 
$  NUM_PARTS = F$INTEGER(iishlib_rv1)
$ info_box "NUM_PARTS = ''NUM_PARTS'"
$ if NUM_PARTS .eq. 0 
$ then
$    error_box  "Ingres transaction log improperly configured. " - 
		" "
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
$!
$ getressym 'self' "''CONFIG_HOST'" config.cluster_mode
$ VALUE = iishlib_rv1
$ if VALUE .eqs. "on"
$ then
$    info_box "Ingres is set up in a cluster configuration."
$ else
$    error_box "Ingres does not appear to be set up in a cluster configuration."
$    goto _EXIT_FAIL
$ endif
$!
$!
$ getressym 'self' "''CONFIG_HOST'" config.cluster.id 0 'SHELL_DEBUG'
$ RESVAL = iishlib_rv1
$!
$ if "''RESVAL'" .NES. "" .and.  "''RESVAL'" .NES. "0"
$ then 
$    info_box "''CONFIG_HOST' has been set up for clustering."
$ endif
$!
$!
$!   Check if instance is up.
$!
$ if test .eq. 0 
$ then 
$    rcpstat -online -silent
$! 
$    error_condition '$severity'
$    if iishlib_err .ne. ii_status_ok
$    then
$       rcpstat -any_csp_online -silent
$       error_condition '$severity'
$    endif
$    if iishlib_err .eq. ii_status_ok
$    then 
$       error_box "Ingres must be shutdown before converting to a non-cluster configuration." -
	" " - 
	"Please use """"ingstop"""" on each node to stop your installation before running ''self' again."
$       goto _EXIT_FAIL
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
$    ii_installation = f$trnlnm("II_INSTALLATION")
$    gen_nodes_lists "''self'"
$    IF iishlib_err .NE. ii_status_ok
$    THEN
$       GOTO _EXIT_FAIL 
$    ENDIF
$!
$    info_box "Check TX log for active transactions - all nodes"
$!
$!#   Make a list of other nodes.
$    ALLNODES='self'NODESLST
$!
$!#   Check if recoverable transactions exist.
$    OTHERNODES=""
$    PLURAL=""
$!
$!    # Confirm no transactions remain in this node's logs.
$     if oktogo .eq. true
$     then
$        set noon
$	 ele_id = 0
$_ALLNODES_NEXT_NODE:
$        cur_node = F$ELEMENT(ele_id," ",ALLNODES)
$	 if cur_node .eqs. " " then GOTO _ALLNODES_DONE
$!
$        if test .eq. 0 
$        then 
$	    define /user SYS$ERROR NL:
$           PIPE rcpstat -transactions -silent -node 'cur_node' ;    STS = $SEVERITY > NL:
$        else
$           info_box "SINGLE BOX TEST rcpstat -transactions -silent ; STS = $SEVERITY > NL: "
$        endif
$        info_box "rcpstat -transactions -silent -node ''cur_node'  (status = ''STS')"
$        if STS .ne. 4 
$        then 
$	    warn_box -
		"There are recoverable transactions for node ''cur_node'." 
$!
$           oktogo=false
$        endif
$	 if "''cur_node'" .nes. F$EDIT("''CONFIG_HOST'","LOWERCASE")
$	 then 
$	    if "''OTHERNODES'" .eqs. ""
$	    then 
$	       PLURAL="s"
$	    endif
$	    OTHERNODES="''OTHERNODES' ''cur_node'"
$	 endif
$!
$	 ele_id = ele_id + 1
$	 GOTO _ALLNODES_NEXT_NODE
$     endif
$!
$_ALLNODES_DONE:
$   info_box "OTHERNODES    =''OTHERNODES'" -
                "CONFIG_HOST   = ''CONFIG_HOST'"
$   if oktogo .eq. true 
$   then
$      info_box "Note: There are no transactions in the logging system - proceding" 
$   endif
$!    
$!#   Prompt for user confirmation.
$ IF (ii_installation_must_exist .eqs. "TRUE")
$ THEN
$    nobox " "  -
        """""''self'"""" will convert Ingres instance """"''ii_installation'"""" on ''CONFIG_HOST' into a non-clustered Ingres instance." -
	" " 
$    if F$EDIT(OTHERNODES,"COLLAPSE") .nes. "" 
$    then 
$	message_box -
	     "Ingres instance """"''II_INSTALLATION'"""" will be disabled from running on the following node''PLURAL': " -
	     "    ''OTHERNODES'." -
	     " " -
	     "Empty transaction logs for the disabled node''PLURAL' will be destroyed."
$    endif
$ ELSE
$    nobox " "  -
        """""''self'"""" will convert Ingres instance of SYSTEM LEVEL on ''CONFIG_HOST' into a non-clustered Ingres instance." -
	" " 
$    if F$EDIT(OTHERNODES,"COLLAPSE") .nes. "" 
$    then 
$	message_box -
	     "Ingres instance of SYSTEM LEVEL will be disabled from running on the following node''PLURAL': " -
	     "    ''OTHERNODES'." -
	     " " -
	     "Empty transaction logs for the disabled node''PLURAL' will be destroyed."
$    endif
$ ENDIF
$!
$!
$ if (disp_conf_restore_msg .eq. 1)
$ then
$    message_box "Using current configuration to restore configuration:" 
$ else
$    if (disp_conf_restore_msg .eq. 2)
$    then
$       message_box "Using saved configuration to restore configuration:" " " -
		    "''old_cfg'" " "
$    endif
$ endif
$!
$!
$ IF BATCH .EQ. false
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
$!#
$!#   The Rubicon is crossed.
$!#
$!
$    nobox "Proceeding with conversion." " " -
		    "Existing config.dat saved as """"config.dat''fileext'""""."
$    log_config_change_banner "BEGIN UNCLUSTER" "''CONFIG_HOST'" "''self'" 25
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
$    if f$search( "II_SYSTEM:[ingres.files]protect.dat" ) .NES. ""
$    then
$       have_protect_dat = 1
$       backup_file protect.dat "''fileext'"
$       $DEBUGIT dir II_CONFIG:protect.dat*/sin
$       IF iishlib_err .NE. ii_status_ok
$       THEN
$          GOTO _EXIT_FAIL 
$       ENDIF
$    endif
$!    
$! backup_ok: ontinue with processing
$!
$!#   Mark cluster specific directories for death.
$!
$    'clsself'
$    'clsfiles'
$    'crtself' 
$    'crtfiles' 
$    'opAfiles'
$!
$     'wrtfiles' "$ SET NOON"
$     'wrtfiles' "$ SET FILE /PROT=(O:RWED) II_SYSTEM:[ingres.admin...]*.*;*"
$     'wrtfiles' "$ SET FILE /PROT=(O:RWED) II_SYSTEM:[ingres.files.memory...]*.*;*"
$     'wrtfiles' "$ DELETE/NOCONFIRM/NOLOG II_SYSTEM:[ingres.files.memory...]*.*;* /EXC=*.dir;*"
$     'wrtfiles' "$ DELETE/NOCONFIRM/NOLOG II_SYSTEM:[ingres.files.memory]*.DIR;"
$     'wrtfiles' "$ DELETE/NOCONFIRM/NOLOG II_SYSTEM:[INGRES.ADMIN...]*.*;* /EXC=*.dir;*"
$     'wrtfiles' "$ DELETE/NOCONFIRM/NOLOG II_SYSTEM:[INGRES.ADMIN]*.DIR;"
$     'wrtfiles' "$ SET ON"
$!
$!#   Rename transaction log files.
$!    
$     getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_name 
$     PRIMARY_LOG_NAME = iishlib_rv1
$     info_box  "REMOVE_NODE for ''CONFIG_HOST' " " (log_log_name=''iishlib_rv1')"
$     IF "PRIMARY_LOG_NAME" .EQS. ""
$     THEN
$        PRIMARY_LOG_NAME='ingres_log'
$     ENDIF
$     getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_name 
$     DUAL_LOG_NAME = iishlib_rv1
$     info_box  "REMOVE_NODE for ''CONFIG_HOST' " " (dual_log_name=''iishlib_rv1')"
$!
$     WARNINGS=""
$     PART=0
$_PART_NEXT:
$     IF  PART .GE. NUM_PARTS .or. PART .GT. 16
$     THEN
$	GOTO _PART_DONE
$     ENDIF
$     PART=PART + 1
$     getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_'PART' 
$     DIRSPEC = iishlib_rv1
$!
$     IF DIRSPEC .EQS. "" 
$     THEN 
$        GOTO _PART_NEXT
$     ENDIF
$!
$     gen_log_name 'PRIMARY_LOG_NAME' 'PART' 'CONFIG_HOST' 'SHELL_DEBUG'
$     OLD_NAME = iishlib_rv1 
$     gen_log_spec "''DIRSPEC'" "''OLD_NAME'"
$     SPEC = F$SEARCH("''iishlib_rv1'")
$!
$     IF SPEC .NES. ""
$     THEN
$        info_box "Renfile:''SPEC'"
$        gen_log_name 'PRIMARY_LOG_NAME' 'PART' "" 'SHELL_DEBUG'
$        gen_log_spec "''DIRSPEC'" "''iishlib_rv1'"
$        'wrtfiles' "$ RENAME ''SPEC' ''iishlib_rv1'"
$     ELSE
$        box "Nofile: ''iishlib_rv1'"
$     ENDIF
$!
$     IF "''DUAL_LOG_NAME'" .EQS. ""
$     THEN
$        GOTO _PART_NEXT
$     ENDIF
$!
$     getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_'PART' 
$     DIRSPEC = iishlib_rv1
$     IF DIRSPEC .EQS. "" 
$     THEN 
$        GOTO _PART_NEXT
$     ENDIF
$!          
$     gen_log_name 'DUAL_LOG_NAME' 'PART' 'CONFIG_HOST' 'SHELL_DEBUG'
$     OLD_NAME = iishlib_rv1 
$     gen_log_spec "''DIRSPEC'" "''OLD_NAME'"
$     SPEC = F$SEARCH("''iishlib_rv1'")
$     IF SPEC .NES. ""
$     THEN
$        info_box "Renfile:''SPEC'"
$	 gen_log_name 'DUAL_LOG_NAME' 'PART' "" 'SHELL_DEBUG'
$        gen_log_spec "''DIRSPEC'" "''iishlib_rv1'"
$        'wrtfiles' "$ RENAME ''SPEC' ''iishlib_rv1'"
$     ELSE
$        box "Nofile: ''iishlib_rv1'"
$     ENDIF
$     GOTO _PART_NEXT
$!
$_PART_DONE:
$!
$!#   Create list of files from other nodes to destroy
$     if OTHERNODES .nes. ""
$     then
$        ele_id = 0
$_OTHERNODES_DEL_LOG_NEXT_NODE:
$        if ele_id .gt. 16 
$	 then 
$	    GOTO _OTHERNODES_DEL_LOG_DONE
$	 endif
$        cur_node = F$ELEMENT(ele_id," ",OTHERNODES)
$!
$        if cur_node .eqs. " " 
$	 then 
$	    GOTO _OTHERNODES_DEL_LOG_DONE
$	 endif
$        if cur_node .eqs. CONFIG_HOST
$	 then 
$	    GOTO _OTHERNODES_DEL_LOG_NEXT_NODE
$	 endif
$!
$        info_box "CURRENT_NODE=''cur_node'"
$        getressym 'self' "''cur_node'" rcp.log.log_file_parts 
$        NUM_PARTS = iishlib_rv1
$	 if NUM_PARTS .eq. 0
$	 then
$	    ele_id = ele_id + 1
$	    GOTO _OTHERNODES_DEL_LOG_NEXT_NODE
$	 endif
$!
$        getressym 'self' "''CONFIG_HOST'" rcp.log.log_file_name 
$        PRIMARY_LOG_NAME = iishlib_rv1
$        info_box  "REMOVE_NODE for ''CONFIG_HOST' " " (log_file_name=''iishlib_rv1')"
$!
$        IF "PRIMARY_LOG_NAME" .EQS. ""
$        THEN
$           PRIMARY_LOG_NAME="ingres_log"
$        ENDIF
$!
$        getressym 'self' "''CONFIG_HOST'" rcp.log.dual_log_name 
$        DUAL_LOG_NAME = iishlib_rv1
$        info_box  "REMOVE_NODE for ''CONFIG_HOST' " " (dual_log_name=''iishlib_rv1')"
$!
$        WARNINGS=""
$        PART=0
$_OTHERNODE_DEL_PART_NEXT:
$        IF  PART .LT. NUM_PARTS 
$        THEN
$           PART=PART + 1
$           getressym 'self' "''cur_node'" rcp.log.log_file_'PART' 
$           DIRSPEC = iishlib_rv1
$           info_box  "REMOVE_NODE for ''cur_node' " "(log_file_''PART'=''iishlib_rv1')"
$!
$           IF DIRSPEC .EQS. "" 
$           THEN 
$              GOTO _OTHERNODE_DEL_PART_NEXT
$           ENDIF
$!
$           gen_log_name 'PRIMARY_LOG_NAME' 'PART' 'cur_node' 'SHELL_DEBUG'
$           OLD_NAME = iishlib_rv1 
$           info_box  "REMOVE_NODE for ''cur_node' " "(gen_log-name=''OLD_NAME')"
$           gen_log_spec "''DIRSPEC'" "''OLD_NAME'"
$           SPEC = F$SEARCH("''iishlib_rv1'")
$!
$           IF SPEC .NES. ""
$           THEN
$              info_box "RMfile:''SPEC'"
$  	       'wrtfiles' "$ DELETE ''SPEC'"
$           ELSE
$              box "Nofile: ''iishlib_rv1'"
$           ENDIF
$!
$           IF "''DUAL_LOG_NAME'" .EQS. ""
$           THEN
$              GOTO _OTHERNODE_DEL_PART_NEXT
$           ENDIF
$!>
$           getressym 'self' "''cur_node'" rcp.log.dual_log_'PART' 
$           DIRSPEC = iishlib_rv1
$           info_box  "REMOVE_NODE for ''cur_node' " "(dual_log_''PART'=''iishlib_rv1')"
$           IF DIRSPEC .EQS. "" 
$           THEN 
$              GOTO _OTHERNODE_DEL_PART_NEXT
$           ENDIF
$!          
$           gen_log_name 'DUAL_LOG_NAME' 'PART' 'cur_node' 'SHELL_DEBUG'
$           OLD_NAME = iishlib_rv1 
$           info_box  "REMOVE_NODE for ''cur_node' " "(gen_log-name=DUAL=''OLD_NAME')"
$           gen_log_spec "''DIRSPEC'" "''OLD_NAME'"
$           SPEC = F$SEARCH("''iishlib_rv1'")
$           IF SPEC .NES. ""
$           THEN
$             info_box "RMfile:''SPEC'"
$   	       'wrtfiles' "$ DELETE ''SPEC'"
$           ELSE
$              box "Nofile: ''iishlib_rv1'"
$           ENDIF
$           GOTO _OTHERNODE_DEL_PART_NEXT
$        ENDIF     
$!
$!#   Strip out all reference to other nodes from configuration.
$!
$	 ele_id = ele_id + 1
$        GOTO _OTHERNODES_DEL_LOG_NEXT_NODE
$     ENDIF
$_OTHERNODES_DEL_LOG_DONE:
$!
$  if F$TRNLNM("II_GCN''II_INSTALLATION'_LCL_VNODE") then -
	DEASSIGN II_GCN'II_INSTALLATION'_LCL_VNODE 
$!

$!   Remove configuration entries for other nodes.
$    clean_config "config.dat" "''CONFIG_HOST'" "''tmpfile'"
$    if have_protect_dat
$    then
$       clean_config "protect.dat" "''CONFIG_HOST'" "''tmpfile'"
$    endif
$!
$!#   Reset certain entries for surviving node.
$    cfu_host = F$EDIT(CONFIG_HOST,"UPCASE")
$    iisetres "ii.''CONFIG_HOST'.config.cluster_mode" "OFF"
$    iisetres "ii.*.config.server_host" "''cfu_host'"
$!
$! Convert clustered GCN files to unclustered (node-specific) format.
$!
$ iicvtgcn -u
$!
$!  Forced DMCM to "OFF" for all classes - JIC
$!
$   PIPE SEARCH/NOWARN/NOLOG/MATCH=AND II_CONFIG:config.dat -
        ".dmcm", "on" -
        > II_CONFIG:'self'_config.dat
$   OPEN/READ dmcmin II_CONFIG:'self'_config.dat
$!
$_DMCM_NEXT_0:
$   READ/END_OF_FILE=_DMCM_LAST_0 dmcmin irec
$   node  = F$ELEMENT(0,":",irec)
$   value = F$ELEMENT(1,":",irec)
$   iisetres "''node'" "OFF"
$   GOTO _DMCM_NEXT_0
$_DMCM_LAST_0:
$!
$! tidy up from DMCM processing
$   irec = ""
$   if F$TRNLNM("dmcmin") .nes. "" then CLOSE dmcmin
$   DELETE/NOLOG II_CONFIG:'self'_config.dat.*
$!
$!
$   info_box "''self' has forced DMCM to """"OFF"""" for all server classes."         -
                " " "It is mandatory for all active DBMS servers to run without DMCM"      -
                "in a Non-clustered Ingres installation."
$!
$!
$!#   Ingres configuration change is now complete.  Any failure
$!#   from here on will NOT abort the conversion.
$     info_box "Ingres configuration change is now complete.  Any failure from here on will NOT abort the conversion."
$!
$!# Perform file rename/deletion.
$   'clsfiles'
$   @'wrtfile''pid'
$   $DEBUGIT TYPE 'wrtfile''pid'
$!
$   cleanup
$!
$   if warnings .ne. 0
$   then
$      warn_box - 
	"During conversion, ''SELF' detected one or more configuration inconsistencies." " " -
	"Please resolve the reported problems before using Ingres."
$   endif
$   message_box -
	"Ingres has now been converted to a non-cluster configuration."
$!
$   nobox " " "Note: Certain text log files found in : """"II_SYSTEM:[INGRES.FILES]"""", such as IIACP.LOG_''CONFIG_HOST', and IIRCP.LOG_''CONFIG_HOST' will remain in place, but all new information will be written to log files with normal names. "
$   nobox " "       -
	   "(e.g. IIACP.LOG, IIRCP.LOG)" " "
$!
$   add_install_history "''VERSION'" "Converted from cluster to unclustered configuration"
$!
$ goto _EXIT_OK
$!
$!  
$!-------------------------------------------------------------------
$! Exit labels.
$!
$_EXIT_FAIL:
$ complete_message = "Failed at"
$ restore_file "config.dat" 'fileext'
$ if have_protect_dat
$ then
$    restore_file "protect.dat" 'fileext'
$ endif
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
$   cleanup_'self' 
$!  
$   unlock_config_dat "''self'" "''iisulocked'"
$!  
$   cleanup "''self'"   
$!
$!This is the only clean exit point in the script
$!
$ end_time = F$TIME()
$ nobox "''self' ''complete_message' ''end_time'"
$!
$ EXIT
$!
$!   Complain about bad input.
$!
$!      $1 = bad parameter 
$!
$ _usage_iiuncluster: subroutine
$!
$ echo "invalid parameters specified. ''P1' ''P2' ''P3'"
$ echo "Usage: ''self' [ -force <datestamp> ]"
$ exit
$ endsubroutine


$!#
$!#   rename wrapper to allow suppression of rename.
$!#
$!#	$1 = target.
$!#	$2 = new name
$!#
$ _rename_file: SUBROUTINE
$ src_file =  F$SEARCH("''P1'")
$ tgt_file =  F$SEARCH("''P2'")
$ if src_file .eqs. ""
$ then
$   warn_box "Source file missing" -
	    "''P2'"
$ endif
$ if tgt_file .nes. ""
$ then
$   info_box "Target file exists" -
	    "''P2'"
$ endif
$ if src_file .nes. ""
$ then
$    if FAKEIT .ne. 0
$    then    
$       box  "Skipped mv of: ''P1'" -
	     "           to: ''P2'"
$     else
$!
$	 ren_file = F$ELEMENT(0,";",src_file)
$	 dst_file = F$ELEMENT(0,";",P2)
$	 PURGE 'ren_file'
$	 RENAME/NOCONF  'ren_file';0 'dst_file'
$	 PURGE 'dst_file'
$     endif
$ else
$     box "NOfile: ''P1'"
$ endif
$ EXIT
$!
$ ENDSUBROUTIMNE

$!#
$!#   delete wrapper to allow suppression of file deletion.
$!#
$!#	$1 = target.
$!#
$ _remove_file: SUBROUTINE
$ if F$SEARCH("''P1'") .nes. ""
$ then
$    if FAKEIT .ne. 0
$    then    
$       box  "Skipped mv of: ''P1' " -
	     "           to: ''P2'"
$    else
$	   del_file = F$ELEMENT(0,";",P1)
$	   PURGE 'del_file'
$	   DELETE/NOCONFIRM/NOLOG  'del_file';* 
$    endif
$ else
$    xbox "NOfile: ''P1'"
$ endif
$!
$ EXIT
$!
$ ENDSUBROUTINE


$ _clean_config: SUBROUTINE 
$!
$   cf_file  	= F$EDIT("''P1'","LOWERCASE")
$   cf_host  	= F$EDIT("''P2'","LOWERCASE")
$   scratchfile	= "''P3'"
$!
$   cc_status = 0
$   OPEN /READ/SHARE/ERROR=_cc_ropen_err cc_infile II_CONFIG:'cf_file' 
$   OPEN /WRITE/ERROR=_cc_wopen_err cc_outfile 'scratchfile' 
$ _cc_loop:
$   READ /END_OF_FILE=_cc_done/ERROR=_cc_read_err cc_infile cc_buffer
$!  Pass any element not starting with "ii."
$   IF F$Element(0,".",cc_buffer) .NES. "ii" THEN GOTO _cc_write
$   cc_host = F$Element(1,".",cc_buffer)
$!  Pass all "shared" parameters.
$   IF "''cc_host'" .EQS. "*" THEN GOTO _cc_write
$!  Reject params for all other node names.
$   IF "''cc_host'" .NES. "''cf_host'" THEN GOTO _cc_loop
$!  Discard "cluster.id, ii_cluster_id"
$   IF F$Locate( ".config.cluster.id:", cc_buffer ) .NE. F$Length(cc_buffer) -
       THEN GOTO _cc_loop
$   IF F$Locate( ".lnm.ii_cluster_id:", cc_buffer ) .NE. F$Length(cc_buffer) -
       THEN GOTO _cc_loop
$!
$   IF F$Locate( ".config.nickname", cc_buffer ) .NE. F$Length(cc_buffer) -
       THEN GOTO _cc_loop
$   IF F$Locate( ".dbms.iicna", cc_buffer ) .NE. F$Length(cc_buffer) -
       THEN GOTO _cc_loop
$   IF F$Locate( ".dbms.private.iicna", cc_buffer ) .NE. F$Length(cc_buffer) -
       THEN GOTO _cc_loop
$   IF F$Locate( ".ingstart.iicna", cc_buffer ) .NE. F$Length(cc_buffer) -
       THEN GOTO _cc_loop
$!
$ _cc_write:
$   WRITE /ERROR=_cc_read_err cc_outfile cc_buffer
$   GOTO _cc_loop
$ _cc_done:
$   cc_status = 1
$ _cc_read_err:
$   CLOSE cc_outfile
$ _cc_wopen_err:
$   CLOSE cc_infile
$ _cc_ropen_err:
$
$    IF cc_status
$    THEN
$        COPY 'scratchfile' II_CONFIG:'cf_file'
$        DELETE/NOCONFIRM/NOLOG 'scratchfile'; 
$    ENDIF
$    EXIT cc_status
$!
$ ENDSUBROUTINE

$!
$!   Cleanup 
$!
$!#
$ _cleanup_iiuncluster: SUBROUTINE
$    remove_file "''selffile'''pid'"
$    remove_file "''wrtfile'''pid'"
$!
$    if FAKEIT .eq. true
$    then
$       message_box -
	"Restoring original config.dat and protect.dat. " " " - 
	" Modified configuration files saved with extention .dat_test"
$!
$       backup_file config.dat "_test"
$       if have_protect_dat
$       then
$          backup_file protect.dat "_test"
$       endif
$	restore_file  config.dat  'fileext' 
$       if have_protect_dat
$       then
$	   restore_file  protect.dat 'fileext'
$       endif
$    endif
$!
$!  Delete symbols
$!
$  DELETE/SYM/GLOBAL   'self'BATCH
$  DELETE/SYM/GLOBAL   'self'READ_RESPONSE
$  DELETE/SYM/GLOBAL   'self'WRITE_RESPONSE
$  DELETE/SYM/GLOBAL   'self'RESINFILE
$  DELETE/SYM/GLOBAL   'self'NODESLST
$  DELETE/SYM/GLOBAL   'self'IDSLST
$  DELETE/SYM/GLOBAL   'self'CONFIRM 
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
