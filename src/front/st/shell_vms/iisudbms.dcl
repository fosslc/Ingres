$! -----------------------------------------------------------------------
$!
$!  Copyright (c) 1994, 2006 Ingres Corporation
$!
$!  Name: iisudbms.com -- IVP (setup) script for Ingres/DBMS 
$!
$!  Usage:
$!	@iisudbms
$!
$!  Description:
$!	This script is called by VMSINSTAL to set up the Ingres DBMS.
$!	It assumes that II_SYSTEM has been set in the JOB, GROUP or 
$!	SYSTEM logical name table.
$!
$!!  History:
$!!       11-apr-95 (ROSGA02)
$!!		Added logic to override default size of the
$!!		transaction log file(s).  (59018)
$!!       12-apr-95 (wolf)
$!!		Added the following changes from iisudbms.sh:
$!!		08-nov-94 (forky01) Fix bug 63476--rename
$!!		 ii.$CONFIG_HOST.prefs.iso_entry_sql-92
$!!		 to ii.$CONFIG_HOST.fixed_prefs.iso_entry_sql-92
$!!		01-Mar-95 (hanch04) Fix bug 67103--add setting of
$!!		 II_GCNxx_LCL_VNODE.
$!!		09-Mar-95 (tutto01) Clarified message concerning backup
$!!		 transcation log.
$!!		20-Mar-95 (tutto01) Fix bug 67540--drop reference to 
$!!		 "ingstart" in intermediate messages.
$!!       19-apr-95 (ROSGA02)
$!!		Remove existing transaction log file(s) on upgrade
$!!              if new size is selected.  (67499)
$!!       08-may-95 (wolf)
$!!		More bug fixes.
$!!		If there are Star servers to start at this time, disable
$!!		them while upgradedb is run on the IIDBDB.  Restart Star
$!!		if all databases are to be upgraded.  (67384)
$!!		Set II_GCN_LCL_VNODE (not II_GCNxx...) for a production
$!!		installation.  (68555)
$!!		Re-phrase the prompts about changing log file size; warn
$!!		users that upgrades will erase/re-format the logs. (68604)
$!!	 01-jun-95 (dougb)
$!!		We are now delivering a IIJOBDEL.COM file -- don't delete it.
$!!       06-jun-95 (wolf) 
$!!		VMSINSTAL may be run by another account on behalf of the
$!!		INGRES owner.  In that case, assign OpenINGRES privileges
$!!		to the IVP user too. (68776)
$!!		In a cluster installation, neither II_LOG_FILE_NAME nor
$!!		II_DUAL_LOG_NAME includes 'node_name' at the end.   Build
$!!		temporary names and logicals correctly. (69139)
$!!       07-jun-95 (wolf) 
$!!		If the transaction log size is not changed, iisetres it anyway.
$!!		Otherwise, a 16M dual log would be created regardless of the
$!!		size of a pre-existing 6.4 tranaction log. (69187)
$!!		For a cluster, set sole_server to OFF. (69127)
$!!       15-jun-95 (wolf) 
$!!		Upper limit on cluster ids is 15.  (69350)
$!!		Remove 69127 fix, use dbms.crs to set sole_server based on
$!!		ii.$.config.cluster_mode.
$!!		Create config.dat with w:re file protections
$!!	 22-jun-95 (albany, for wolf)
$!!		Fix typo.
$!!       26-jun-95 (wolf) 
$!!		If ii.$.config.cluster_mode is already set, don't turn it OFF.
$!!	 17-jul-95 (wagner)
$!!		Added installation code to dmfjsp executable.  
$!!		Copying all installation exe's with w:re file protections. 
$!!	 18-jul-95 (duursma) 
$!!		Bug 69827: if group installation, don't set II_DUAL_LOG or
$!!		II_DUAL_LOG_NAME.  Instead, set II_DUAL??_LOG and 
$!!		II_DUAL_LOG??_NAME where ?? is the installation code.
$!!	 15-aug-95 (dougb)
$!!		Correct placement of ckpdb symbol definition.
$!!       19-sep-95 (nfletcher)
$!!		Added a  definition for ii_installation after it
$!!		has been entered correctly be user - this is required within
$!!		'iimklog' as a consequence of change for bug 69827 above.
$!!	 22-mar-96 (duursma)
$!!		Added Express Setup features.
$!!	 26-jul-96 (rosga02)
$!!		Added 5 seconds wait between createdb and ckpdb to prevent
$!!              E_DM110D_CPP_DB_BUSY error.
$!!	29-jul-96 (kamal03)
$!!		Deleted "if EXPRESS" logic to bypass Cluster vs. Standalone
$!!		type of installation
$!!	22-oct-96 (boama01)
$!!		De-install any previous shared images used by setup before
$!!		assigning job logicals for the new ones w. same names.
$!!		This prevents image ID mismatch from old images.
$!!	21-mar-96 (rosga02)
$!!		Check if slutil.exe is installed and if so skip createdb iidbdb
$!!	12-iun-96 (rosga02)
$!!		Check if slutil.exe is installed and if so skip upgrade dbs
$!!      1-nov-97 (kinte01)
$!!              Added iisetres for rcp log buffer_count.  When upgrading,
$!!              parameter did not increase to 20.
$!!      17-feb-98 (kinte01)
$!! 		Add changes for renaming of rcp.file.size to rcp.file.kbytes
$!!	28-feb-98 (kinte01)
$!!		Reapply change 422591
$!!              11-jan-96 (loera01)
$!!                 Added API shared library.
$!!    20-apr-98 (mcgem01)
$!!            Product name change to Ingres.
$!!	20-apr-98 (kinte01)
$!!		Correct problem with log size calculations resulting from 
$!!		previous changes
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!      10-aug-1998 (kinte01)
$!!	    Remove references to II_AUTH_STRING
$!!	20-aug-1998 (kinte01)
$!!	    Be consistent in value entered for log file size. Enter MB here
$!!	    just like in CBF. Ensure minimum log file size is 16M.
$!!	04-sep-1998 (kinte01)
$!!	    Add setup for rmcmd and imadb
$!!	29-sep-1998 (kinte01)
$!!	    Missing prompt for confirmation of log size
$!!	06-oct-1998 (kinte01)
$!!	    Bug 87347: For installations defined using System Logicals
$!!	    don't try to set ii_installation as it will not be defined.
$!!	12-Oct-1998 (kinte01)
$!!	    Rename the shared images even if it is found that this version
$!!	    has already been installed. This accounts for what will need to
$!!	    updated if this is update from a BETA version of 2.0
$!!	17-Feb-1999 (kinte01)
$!!	    Add stricter checking to ensure that this script is not being
$!!	    executed from the system account. Also move this check to the
$!!	    beginning to not waste time attempting to execute the script
$!!	    (bug 95373)
$!!	18-Feb-1999 (kinte01)
$!!	    Change all references to VAXcluster to VMScluster as this
$!!	    script is executed on both VAX & Alpha machines
$!!	18-Feb-1999 (kinte01)
$!!	    Change the logfile calculation to be entered using VMS blocks
$!!	    instead of megabytes (bug 95414)
$!!	11-feb-2000 (kinte01)
$!!	    Change the way the logical is defined for ii_log_file & ii_dual_log
$!!	    so in a clustered Ingres installation each node in the cluster
$!!	    can maintain its own logfile on a local disk
$!!      24-mar-2000 (devjo01)
$!!          Make gen of scratch file names safe for Clustered Ingres.
$!!	23-apr-2001 (kinte01)
$!!	    Add express selection for II_WORK as you are currently prompted
$!!	    for II_WORK regardless (104540)
$!!      27-apr-2001 (horda03)
$!!          Ensure RMCMD programs connect to the DB as the Ingres Installation
$!!          owner. (Bug 100039)
$!!	15-May-2001 (hanje04)
$!!	    II 2.6 requires stack size of 128K, this must be changed 
$!!	    for the dbms, recovery servers when upgrading.
$!!	24-may-2001 (kinte01)
$!!	    RMCMD startup count should be changed if greater than & equal 0 
$!!	    and not just greater 0
$!!      18-Jun-2001 (horda03)
$!!          Only allow Ingres System Administrator of the VMSINSTAL script
$!!          to invoke the IVP scripts.
$!!	25-oct-2001 (kinte01)
$!!	    Add symbol definition for iiingloc as it is utilized in the
$!!	    all.crs file
$!!	13-aug-2002 (abbjo03)
$!!	    Fix references to ii_log_file_name/ii_dual_log_name.
$!!	12-dec-2002 (abbjo03)
$!!	    Correct rmcmd processing to reflect usage of imadb as repository.
$!!	20-dec-2002 (abbjo03)
$!!	    Correct change of 13-aug: rcp.dual.dual_file should have been
$!!	    rcp.log.dual_file.
$!!	22-Jul-2002 (kinte01)
$!!	    Add definition for iigwstart and iirungw in case you are 
$!!	    upgrading a DBMS server installation that also contains the
$!!	    gateways. (108360)
$!!	19-Aug-2002 (kinte01)
$!!	    iigwstop is in utility not in bin.  I'm not sure why iigwstart
$!!	    is in bin but iigwstop is in utility. (108360)
$!!	23-dec-2002 (abbjo03)
$!!	    Temporarily disable cluster installation.
$!!	12-feb-2003 (abbjo03)
$!!	    Further corrections to dual log logic.
$!!	05-nov-2003 (kinte01)
$!!	    Remove iiruncsp as there is no separate DMFCSP process with 
$!!	    the Portable Clustered Ingres project
$!!	30-jan-2004 (abbjo03)
$!!	    Change log size calculations to use kbytes and use consistent
$!!	    variable names.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	08-mar-2005 (abbjo03)
$!!	    iisdall.com has been renamed as ingstop.
$!!	24-may-2005 (abbjo03)
$!!	    Need to set rcp.log.dual_log_name even if the users choose not to
$!!	    configure dual logging, to prevent problems if they add it later.
$!!	25-may-2005 (bolke01)
$!!	    Removed all references to performing a clustered Ingres 
$!!	    install.   Replaced this with a text block to display
$!!	23-sep-2005 (abbjo03)
$!!	    Do not invoke rmcmdgen/rmcmdrmv with -u since now all rmcmd objects
$!!	    are owned by $ingres.
$!!	17-may-2006 (abbjo03)
$!!	    Remove double quotes from II_CONFIG process level definition.
$!!	26-apr-2007 (upake01)
$!!	    SIR 118202 - Change text 
$!!             "The minimum logfile size is 32768 Blocks (16MB). " to
$!!             "The minimum logfile size is 65536 Blocks (32MB). "
$!!         Bug# 118214 - When upgrading to next Ingres release, 
$!!             the procedure asks for 
$!!                 "Do you want to change transaction log size? (y/n) "
$!!             twice.  For such installs, disabled the first prompt
$!!             as the 2nd prompt is more appropriate as it also lets user
$!!             know that for existing transaction log file - "if you change
$!!             the transaction log size, the existing log file(s) will be
$!!             deleted before the new one(s) are created".
$!!	22-jun-2007 (upake01)
$!!	    Answering "N"o to "Do you wish to continue this setup procedure? (y/n"
$!!         results in a "%SYSTEM-F-ABORT, abort" message.  Answering "N"o should
$!!         not be treated as an error as user just wishes to not install the
$!!         component.  Instead, display a message "*** The above component will
$!!         not be installed ***" and exit gracefully.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Modify to take out "Purge" and "Set File/version"
$!!             statement(s) for CONFIG.DAT.
$!!     15-Aug-2007 (Ralph Loen) SIR 118898
$!!         Added symbol definition of iinethost.
$!!     06-Nov-2007 (upake01)
$!!         SIR 116148 - Added "date_type_alias" to prompt for alias datatype date 
$!!             (default to ingresdate).  Ported this change for VMS.
$!!     20-Nov-2007 (upake01)
$!!         Remove "/nolog" from "deassign" statement for "default_date_alias"
$!!     11-Jan-2007 (upake01)
$!!         Change  if cluster_id .nes. "" then logfile = logfile + node_name
$!!         to      if cluster_id .nes. "" then logfile = logfile + "_" + node_name 
$!!         Add the missing underscore ("_") between logfile and node_name in
$!!            when building the log file name.  Missing "_" was causing the
$!!            procedure to abort or skip the functionality.
$!!     12-feb-2008 (horda03) Bug 119901
$!!         Ensure only RMCMD tables in imadb are owned by $ingres. If another
$!!         user has RMCMD tables, then INGSTOP may fail to shutdown the
$!!         RMCMD server.
$!!     08-may-2008 (upake01) SIR 117648
$!!         In prompts for "ingres date / ansi date", change "yes" and "no"
$!!         to "y" and "n" so as to be consistent with rest of the code.
$!!         On an upgrade, procedure prompts
$!!               "Do you want to change transaction log size? (y/n) "
$!!         two times.  Modify to prompt only once.
$!!     12-may-2008 (upake01) SIR 117648
$!!         Change display text for "HSC-based devices" and
$!!         "Local Area VAXclusters".
$!!     16-Jul-2009 (horda03) Bug 122285
$!!         gcn_lcl_vnode is defined on SYSTEM level installations.
$!!         ii_gcn_lcl_vnode should be defined.
$!!     07-Jan-2010 (horda03) Bug 119379
$!!         Correct the grammar of the ingresdate/ansidate date alias
$!!         setup.
$!!     14-Jan-2010 (horda03) Bug 123153
$!!         Upgrade all nodes in a clustered environment.
$!!     24-Jun-2010 (horda03) Bug 122555
$!!             Add check_lglk_mem to ensure Ingres will connect to a
$!!             new LG/LK shared memory section.
$!!     08-Jul-2010 (horda03) Bug 124060
$!!         Correct dual_log_name config symbol name.
$!!    
$!------------------------------------------------------------------------	
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$!
$! Define symbols for programs called locally.
$!
$ createdb	:== $ii_system:[ingres.bin]createdb.exe
$ iigenres	:== $ii_system:[ingres.utility]iigenres.exe
$ iigetres	:== $ii_system:[ingres.utility]iigetres.exe
$ iigetsyi	:== @ii_system:[ingres.utility]iigetsyi.com
$ iigwstart	:== $ii_system:[ingres.bin]iigwstart.exe
$ iigwstop	:== $ii_system:[ingres.utility]iigwstop.exe
$ iiinitres     :== $ii_system:[ingres.utility]iiinitres.exe
$ iiingloc      :== $ii_system:[ingres.utility]iiingloc.exe
$ iipmhost      :== $ii_system:[ingres.utility]iipmhost.exe
$ iinethost     :== $ii_system:[ingres.utility]iinethost.exe
$ iimaninf	:== $ii_system:[ingres.utility]iimaninf.exe
$ iimklog	:== $ii_system:[ingres.utility]iimklog.exe
$ iircpseg	:== @ii_system:[ingres.utility]iircpseg.com
$ iiremres      :== $ii_system:[ingres.utility]iiremres.exe
$ iiresutl	:== $ii_system:[ingres.utility]iiresutl.exe
$ iirunacp	:== @ii_system:[ingres.utility]iirunacp.com
$ iirundbms	:== $ii_system:[ingres.utility]iirundbms.exe
$ iirungcc	:== @ii_system:[ingres.utility]iirungcc.com
$ iirungcn	:== @ii_system:[ingres.utility]iirungcn.com
$ iirungw	:== $ii_system:[ingres.utility]iirungw.exe
$ iishowres	:== $ii_system:[ingres.bin]iishowres.exe
$ iisetres	:== $ii_system:[ingres.utility]iisetres.exe
$ iisulock	:== $ii_system:[ingres.utility]iisulock.exe
$ iisyschk	:== @ii_system:[ingres.utility]iisyschk.com
$ iivalres	:== $ii_system:[ingres.utility]iivalres.exe
$ ingstart	:== $ii_system:[ingres.utility]ingstart.exe
$ ingstop	:== @ii_system:[ingres.utility]ingstop.com
$ rcpconfig	:== $ii_system:[ingres.bin]rcpconfig.exe
$ rcpstat	:== $ii_system:[ingres.bin]rcpstat.exe
$ rmcmdgen	:== $ii_system:[ingres.utility]rmcmdgen
$ rmcmdrmv	:== $ii_system:[ingres.utility]rmcmdrmv
$ sql		:== $ii_system:[ingres.bin]tm.exe -qSQL
$ upgradedb	:== $ii_system:[ingres.bin]upgradedb.exe
$!
$! Other variables, macros and temporary files
$!
$ cluster_id         = ""                              !User provided node Cluster ID
$ cluster_id_prompt  = ""                              !Prompt for cluster id?
$ cluster_nodelist   = ""                              !List of nodes already in cluster
$ cluster_idlist     = ""                              !List of ids already in cluster
$ cluster_sequence   = 0                               !Number of existing cluster nodes
$ iiqueue            = "SYS$BATCH"                     !Default queue for startup script
$ iisulocked         = 0                               !Do we hold the setup lock?
$ ii_compatlib_def   = f$trnlnm("ii_compatlib","LNM$JOB") ! Current values
$ ii_libqlib_def     = f$trnlnm("ii_libqlib","LNM$JOB")   !  for shared
$ ii_framelib_def    = f$trnlnm("ii_framelib","LNM$JOB")  !   library definitions
$ node_name	     = f$edit(f$getsyi("NODENAME"), "lowercase")
$ tmpfile            = "ii_system:[ingres]iisudbms.''node_name'_tmp"  !Temporary file
$ saved_message      = f$environment( "MESSAGE" )
$ set_message_off    = "set message/nofacility/noidentification/noseverity/notext"
$ set_message_on     = "set message''saved_message'"
$ SS$_NORMAL	     = 1
$ SS$_ABORT	     = 44
$ status	     = SS$_NORMAL
$ upgrade_warning   == 0
$ do_abort == 0
$ default_log_file   = "ingres_log"
$ default_dual_log   = "dual_log"
$!
$ say		:= type sys$input
$ echo		:= write sys$output 
$!
$ set noverify
$ clustered	= f$getsyi( "CLUSTER_MEMBER" ) !Is this node clustered?
$ user_uic	= f$user() !For RUN command (detached processes)
$ user_name	= f$trnlnm( "II_SUINGRES_USER","LNM$JOB" ) !Installation owner
$
$ script_id = "IISUDBMS"
$ @II_SYSTEM:[ingres.utility]iishlib _iishlib_init "''script_id'"
$!
$ IF "''user_name'".EQS.""
$ THEN
$       user_name= f$getjpi("","USERNAME") !User running this procedure
$!
$! Verify not SYSTEM account.
$!
$      if ( (f$process() .eqs. "SYSTEM") .or. (f$user() .eqs. "[SYSTEM]"))
$      then
$         type sys$input

  --------------------------------------------------------------------
 | This setup procedure must not be run from the SYSTEM account.      |
 | Please execute this procedure from the Ingres System Administrator |
 | account.                                                           |
  --------------------------------------------------------------------
$         goto EXIT_FAIL
$      endif
$
$      isa_uic = f$file_attributes( "II_SYSTEM:[000000]INGRES.DIR", "UIC")
$
$      if isa_uic .nes. user_uic
$      then
$         type sys$input

     ---------------------------------------------------------
    | This setup procedure must be run from the Ingres System |
    | Administrator account.                                  |
     ---------------------------------------------------------
$         goto EXIT_FAIL
$      endif
$ endif
$! 
$! VMSINSTAL is not necessarily run by the Ingres system owner/administrator.
$! For that reason, save the "IVP" userid for later.
$!
$ ivp_name= f$getjpi("","USERNAME") 
$ ivp_user = f$edit( ivp_name, "TRIM,LOWERCASE" ) 
$!
$ ing_user = f$edit( user_name, "TRIM,LOWERCASE" ) !Ingres version of username
$!
$! Make temporary definitions for shared library logicals to 
$! be the unconcealed path of the library directory within ii_system
$! 
$ define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$ define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ if f$search("ii_system:[ingres]iistartup.del") .nes. "" then @ii_system:[ingres]iistartup.del
$!
$! See if user wants express setup
$!
$ echo ""
$ echo "This procedure will set up the Ingres DBMS."
$ echo "In the course of this process you will be asked many questions"
$ echo "that help configure the installation."
$ echo ""
$ echo "Alternatively, you may choose the EXPRESS option which will result"
$ echo "in as few questions as possible; where feasible, defaults will be chosen."
$ echo ""
$ CONFIRM_EXPRESS:
$ inquire/nopunc answer "Do you wish to choose the EXPRESS option? (y/n) "
$ if answer .EQS. "" then goto CONFIRM_EXPRESS
$ if answer
$ then 
$ 	EXPRESS = 1
$ else
$	EXPRESS = 0
$ endif
$
$!
$! Define ii_config for the installation configuration files
$!
$ define/nolog/process II_CONFIG    II_SYSTEM:[ingres.files]
$!
$ echo ""
$ echo "Setting up the Ingres DBMS..."
$!
$! Create ii_system:[ingres.files]config.dat if non-existent. 
$!
$ if( f$search( "ii_system:[ingres.files]config.dat" ) .eqs. "" )
$ then
$    copy nl: ii_system:[ingres.files]config.dat -
	/prot=(s:rwed,o:rwed,g:rwed,w:re)
$ endif
$!
$! Lock the configuration file. 
$!
$ on error then goto EXIT_FAIL
$ iisulock "Ingres DBMS setup"
$ iisulocked = 1
$ set noon
$!
$! Get compressed release ID.
$!
$ on error then goto EXIT_FAIL
$ iimaninf -version=dbms -output='tmpfile
$ set noon
$!
$ set_message_off
$ deassign /job ii_timezone_name
$ set_message_on
$ open tmp 'tmpfile
$ read tmp version_string
$ close tmp
$ delete 'tmpfile;*
$ release_id = f$edit( version_string, "collapse, trim" )
$!
$ STRIPPER_1:
$!
$ offset = f$locate( ".", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPER_2
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_1
$!
$ STRIPPER_2:
$!
$ offset = f$locate( "/", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPER_3
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_2
$!
$ STRIPPER_3:
$!
$ offset = f$locate( "(", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPER_4
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_3
$!
$ STRIPPER_4:
$!
$ offset = f$locate( ")", release_id )
$ length = f$length( release_id )
$ if( offset .eq. length ) then goto STRIPPERS_DONE
$ sub1 = f$extract( 0, offset, release_id )
$ sub2 = f$extract( offset + 1, length - offset - 1, release_id )
$ release_id = sub1 + sub2
$ goto STRIPPER_1
$!
$ STRIPPERS_DONE:
$!
$! Check whether Ingres DBMS is already set up.
$!
$ iigetres ii.'node_name.config.dbms.'release_id dbms_setup_status
$ set_message_off
$ dbms_setup_status = f$trnlnm( "dbms_setup_status" )
$ deassign "dbms_setup_status"
$ set_message_on
$ if( ( dbms_setup_status .nes. "" ) .and. ( dbms_setup_status .eqs. "COMPLETE" ) )
$ then
$    echo ""
$    echo "The ''version_string' version of Ingres DBMS has already been set up"
$    echo f$fao( "on local node !AS.", f$edit(node_name, "upcase"))
$    echo ""
$
$    goto EXIT_OK 
$ endif
$!
$! Determine whether to configure as an Ingres cluster member
$!
$ get_nodes 'node_name
$ if iishlib_err .ne. ii_status_ok then goto EXIT_FAIL
$ nodes = iishlib_rv1
$ cluster_mode = iishlib_rv2
$
$ if( cluster_mode .eqs. "" ) then -
	iisetres ii.'node_name.config.cluster_mode "OFF"
$
$ if ( cluster_mode .eqs. "ON")
$ then
$    iigetres ii.'node_name'.config.cluster_mode mode
$    iigetres ii.'node_name'.config.cluster.id   cluster_id_log
$
$    cluster_id = f$trnlnm( "cluster_id_log" )
$
$    if f$trnlnm( "mode" ) .eqs. "" .or. cluster_id .eqs. ""
$    then
$       echo ""
$       echo f$fao("This node (!AS) is not part of the Clustered Ingres Installation.", f$edit( node_name, "upcase"))
$       echo "To add this node to the Clustered Installation, use the utility IISUNODE"
$       echo ""
$       echo "If this utility is being run as part of an Installation upgrade, run the"
$       echo "utility via the Ingres account on one of the nodes that forms the cluster."
$       echo f$fao("IISUNODE can then be run (on this node) to add !AS to the Clustered", f$edit( node_name, "upcase"))
$       echo "Ingres Installation.
$       echo ""
$       echo "Nodes in the cluster are;"
$       echo ""
$       display_nodes "''nodes'"
$       echo ""
$
$       goto EXIT_OK
$    endif
$
$    deassign "mode"
$    deassign "cluster_id_log"
$
$ endif
$ 
$!
$! Run IIJOBDEF.COM if it exists. 
$!
$ set_message_off
$ if( f$search( "ii_system:[ingres]iijobdef.com" ) .nes. "" )
$ then
$    set_message_on
$    say

Loading existing configuration...
$    ! Save settings thay may be lost
$    @ii_system:[ingres]iijobdef
$    !
$    ! Restore modified settings used by setup scripts
$    define/nolog/job ii_compatlib 'f$parse("ii_system:[ingres.library]clfelib.exe",,,,"NO_CONCEAL")
$    define/nolog/job ii_libqlib   'f$parse("ii_system:[ingres.library]libqfelib.exe",,,,"NO_CONCEAL")
$    define/nolog/job ii_framelib  'f$parse("ii_system:[ingres.library]framefelib.exe",,,,"NO_CONCEAL")
$ endif
$ set_message_on
$!
$! Look up II_DATABASE. 
$!
$ iigetres ii.*.lnm.ii_database ii_database_value
$ set_message_off
$ ii_database = f$trnlnm( "ii_database_value" )
$ deassign "ii_database_value"
$ if( ii_database .eqs. "" ) then -
     ii_database = f$trnlnm( "II_DATABASE", "LNM$JOB" )
$ set_message_on
$!
$! Look up II_CHECKPOINT. 
$!
$ iigetres ii.*.lnm.ii_checkpoint ii_checkpoint_value
$ set_message_off
$ ii_checkpoint = f$trnlnm( "ii_checkpoint_value" )
$ deassign "ii_checkpoint_value"
$ if( ii_checkpoint .eqs. "" ) then -
     ii_checkpoint = f$trnlnm( "II_CHECKPOINT", "LNM$JOB" )
$ set_message_on
$!
$! Look up II_JOURNAL. 
$!
$ iigetres ii.*.lnm.ii_journal ii_journal_value
$ set_message_off
$ ii_journal = f$trnlnm( "ii_journal_value" )
$ deassign "ii_journal_value"
$ if( ii_journal .eqs. "" ) then -
     ii_journal = f$trnlnm( "II_JOURNAL", "LNM$JOB" )
$ set_message_on
$!
$! Look up II_DUMP. 
$!
$ iigetres ii.*.lnm.ii_dump ii_dump_value
$ set_message_off
$ ii_dump = f$trnlnm( "ii_dump_value" )
$ deassign "ii_dump_value"
$ if( ii_dump .eqs. "" ) then -
     ii_dump = f$trnlnm( "II_DUMP", "LNM$JOB" )
$ set_message_on
$!
$! Look up II_WORK. 
$!
$ iigetres ii.*.lnm.ii_work ii_work_value
$ set_message_off
$ ii_work = f$trnlnm( "ii_work_value" )
$ deassign "ii_work_value"
$ if( ii_work .eqs. "" ) then -
     ii_work = f$trnlnm( "II_WORK", "LNM$JOB" )
$ set_message_on
$!
$! Look up II_LOG_FILE and II_DUAL_LOG. 
$!
$ iigetres ii.*.lnm.ii_log_file ii_log_file_value_old
$ iigetres ii.*.lnm.ii_dual_log ii_dual_log_value_old
$!
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".lnm.ii_log_file"       "ii_log_file"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".lnm.ii_log_file_name"  "ii_log_file_name"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".rcp.log.log_file_1"    "log_file_1"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".rcp.log.log_file_name" "log_file_name"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".lnm.ii_dual_log"       "ii_dual_log"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".lnm.ii_dual_log_name"  "ii_dual_log_name"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".rcp.log.dual_log_1"    "dual_log_1"
$ pan_cluster_iigetres "''script_id'" "''nodes'" "ii." ".rcp.log.dual_log_name" "dual_log_name"
$
$ set_message_off
$ ii_log_file_old = f$trnlnm( "ii_log_file_value_old" )
$ ii_dual_log_old = f$trnlnm( "ii_dual_log_value_old" ) 
$ deassign "ii_log_file_value_old"
$ deassign "ii_dual_log_value_old"
$ set_message_on
$
$ ! Get rid of the old LOG file entries
$ if (ii_log_file_old .nes. "")
$ then
$    iiremres ii.*.lnm.ii_log_file
$ else
$    ii_log_file_old = f$trnlnm( "II_LOG_FILE", "LNM$JOB")
$ endif
$
$ if (ii_dual_log_old .nes. "")
$ then
$    iiremres ii.*.lnm.ii_dual_log
$ else
$    ii_dual_log_old = f$trnlnm( "II_DUAL_LOG", "LNM$JOB")
$ endif
$
$ entry = 0
$
$PURGE_OLD_LOG_DATA:
$ c_node = f$element( entry, ",", nodes)
$
$ if (c_node .nes. ",")
$ then
$    entry = entry + 1
$
$    lg_file = ii_log_file_'c_node
$
$    if (lg_file .nes. "")
$    then
$       iiremres ii.'c_node.lnm.ii_log_file
$
$       if (log_file_1_'c_node .eqs. "")
$       then
$          log_file_1_'c_node = lg_file
$
$          iisetres ii.'c_node.rcp.log.log_file_1 "''lg_file'"
$       endif
$    else
$       if (ii_log_file_old .nes. "")
$       then
$          if (log_file_1_'c_node .eqs. "")
$          then
$             log_file_1_'c_node = ii_log_file_old
$
$             iisetres ii.'c_node.rcp.log.log_file_1 "''ii_log_file_old'"
$          endif
$       endif
$    endif
$
$    lg_file_name = ii_log_file_name_'c_node
$
$    if (lg_file_name .nes. "")
$    then
$       iiremres ii.'c_node.lnm.ii_log_file_name
$    endif
$
$    if (log_file_name_'c_node .eqs. "")
$    then
$       log_file_name_'c_node = default_log_file
$
$       iisetres ii.'c_node.rcp.log.log_file_name "''default_log_file'"
$    endif
$
$    lg_file = ii_dual_log_'c_node
$
$    if (lg_file .nes. "")
$    then
$       iiremres ii.'c_node.lnm.ii_dual_log
$
$       if (dual_file_1_'c_node .eqs. "")
$       then
$          dual_log_1_'c_node = lg_file
$
$          iisetres ii.'c_node.rcp.log.dual_log_1 "''lg_file'"
$       endif
$    else
$       if (ii_dual_log_old .nes. "")
$       then
$          if (dual_log_1_'c_node .eqs. "")
$          then
$             dual_log_1_'c_node = ii_dual_log_old
$
$             iisetres ii.'c_node.rcp.log.dual_log_1 "''ii_dual_log_old'"
$          endif
$       endif
$    endif
$
$    lg_file_name = ii_dual_log_name_'c_node
$
$    if (lg_file_name .nes. "")
$    then
$       iiremres ii.'c_node.lnm.ii_dual_log_name
$    endif
$
$    if (dual_log_name_'c_node .eqs. "")
$    then
$       dual_log_name_'c_node = default_dual_log
$
$       iisetres ii.'c_node'.rcp.log.dual_log_name "''default_dual_log'"
$    endif
$
$    goto PURGE_OLD_LOG_DATA
$ endif
$
$!
$! Confirm setup. 
$!
$ if( clustered )
$ then
$    vmscluster = "VMScluster "
$ else
$    vmscluster = ""
$ endif
$ say 

This procedure will set up the following release of the Ingres DBMS:

$ echo "	''version_string'"
$ echo ""
$ if (node_name .nes. nodes)
$ then
$    echo "to run on ''vmscluster'nodes:"
$    echo ""
$    display_nodes "''nodes'"
$ else
$    echo "to run on local ''vmscluster'node:"
$    echo ""
$    echo f$fao("	!AS", f$edit(node_name, "upcase"))
$ endif
$ echo ""
$ echo "This installation will be owned by:"
$ echo ""
$ echo "	''user_name' ''user_uic'" 
$ if( ( ii_database .eqs. "" ) .and.( dbms_setup_status .eqs. "" ) )
$ then
$    !
$     CONFIRM_SETUP_1:
$    !
$    echo ""
$    if EXPRESS then goto SKIP_INQ_1
$    inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_SETUP_1
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$ SKIP_INQ_1:
$    say

Ingres has the capability to guarantee integrity of all committed
database transactions in the event of a system software failure or
single storage device failure.  IMPORTANT NOTE: if a second storage
device fails before recovery from an initial failure has taken place,
committed transactions may be lost.

If you intend to take full advantage of this capability, you need to
have at least two independent storage devices available (one in
addition to the one Ingres is being installed on).  If you do not
have two independent storage devices available, but would like to take
advantage of this capability, you should not complete this setup
procedure at this time.

$    !
$     CONFIRM_SETUP_2:
$    !
$    if EXPRESS then goto SKIP_INQ_2
$    inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_SETUP_2
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$ SKIP_INQ_2:
$ endif
$!
$! Confirm logical name table.
$!
$ if( dbms_setup_status .nes. "" )
$ then
$    iigetres ii.'node_name.lnm.table.id lnm_table 
$    lnm_table = f$trnlnm( "lnm_table" )
$    deassign "lnm_table"
$    goto LNM_TABLE_CONFIRMED
$ endif
$ set_message_off
$ if( f$trnlnm( "II_SYSTEM", "LNM$JOB" ) .nes. "" )
$ then
$    job_ii_system = "TRUE" 
$ else
$    job_ii_system = "FALSE" 
$ endif
$ if( job_ii_system .or. f$trnlnm( "II_SYSTEM", "LNM$GROUP" ) .eqs. "" )
$ then
$    if( job_ii_system .or. f$trnlnm( "II_SYSTEM", "LNM$SYSTEM" ) .eqs. "" )
$    then
$       say 

You have the option of setting up this installation using GROUP
or SYSTEM logical names to store configuration data.

You may only use SYSTEM logicals if no existing INGRES installation
on this machine already uses them.  If there is an existing 
installation on this machine which uses SYSTEM logicals, then
this installation must be set up using GROUP logicals.

Unlike SYSTEM logicals, GROUP logicals can be used to set up multiple
INGRES installations on the same machine.  Before you set up this
installation using GROUP logicals however, be sure that no existing
INGRES installation on this machine is set up to use SYSTEM logicals
under an account which has the same GROUP UIC as the account being
used for this installation.  Therefore, it is permissible to set up
this installation using GROUP logicals if there is a SYSTEM level
installation set up under an account with a different GROUP UIC.
$       !
$        CONFIRM_SETUP_3:
$       !
$    	if EXPRESS then goto CONFIRM_LNM_TABLE
$       inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$       if( answer .eqs. "" ) then goto CONFIRM_SETUP_3
$       if( .not. answer )
$       then 
$          say
*** The above component will not be installed ***
$!
$          goto EXIT_OK
$       endif
$       !
$        CONFIRM_LNM_TABLE:
$       !
$       echo ""
$       inquire/nopunc answer "Do you want to set up this installation using GROUP logicals? (y/n) "
$       if( answer .eqs. "" ) then goto CONFIRM_LNM_TABLE
$       if( answer )
$       then
$          say

You have chosen to set up this installation using GROUP logicals.

If there is another INGRES installation on this machine which is set up
under an account with the same GROUP UIC as this account, do not continue
this setup procedure.

$          lnm_table = "LNM$GROUP"
$       else
$          say

You have chosen to set up this installation using SYSTEM logicals.

If there is another INGRES installation on this system which uses SYSTEM
level logical names, you must not continue this setup procedure. 

$          lnm_table = "LNM$SYSTEM"
$       endif
$       !
$        CONFIRM_SETUP_4:
$       !
$	if EXPRESS 
$	then 
$	   answer = 1
$	else
$          inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$	endif
$       if( answer .eqs. "" ) then goto CONFIRM_SETUP_4
$       if( answer )
$       then
$          pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.table.id ""''lnm_table'"""
$       else
$          say
*** The above component will not be installed ***
$          goto EXIT_OK
$       endif
$    else
$       set_message_on
$       say 

Since II_SYSTEM is defined in the SYSTEM logical name table, this setup
procedure will configure Ingres using SYSTEM level logicals.

If there is another INGRES installation on this system which uses SYSTEM
level logical names, do not continue this setup procedure. 

$       !
$        CONFIRM_SYSTEM_SETUP:
$       !
$       inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$       if( answer .eqs. "" ) then goto CONFIRM_SYSTEM_SETUP
$       if( answer )
$       then
$          pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.table.id ""LNM$SYSTEM"""
$       else
$          say
*** The above component will not be installed ***
$          goto EXIT_OK
$       endif
$       lnm_table = "LNM$SYSTEM" 
$    endif
$ else
$    say 

Since II_SYSTEM is defined in the GROUP logical name table, this setup
procedure will configure Ingres using GROUP level logicals.

If there is an INGRES installation on this machine which uses SYSTEM
level logicals, you must set up this installation using an account
with a different GROUP UIC than the account used to install the
SYSTEM level installation.

$    !
$     CONFIRM_GROUP_SETUP:
$    !
$    if EXPRESS 
$    then
$	answer = "TRUE"
$    else
$       inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto CONFIRM_GROUP_SETUP
$    if( answer )
$    then
$       pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.table.id ""LNM$GROUP"""
$    else
$       say
*** The above component will not be installed ***
$       goto EXIT_OK
$    endif 
$    lnm_table = "LNM$GROUP"
$ endif
$!
$ LNM_TABLE_CONFIRMED:
$!
$! Define II_SYSTEM in the correct logical table.
$!
$ set_message_off
$ ii_system_value = f$trnlnm( "ii_system" )
$ deassign /proc ii_system
$ deassign /proc/exec ii_system
$ deassign /job ii_system
$ deassign /job/exec ii_system
$ define/exec/table='lnm_table/trans=concealed ii_system "''ii_system_value'"
$ deassign "ii_system_value"
$ set_message_on
$
$ if (clustered) .and. ( cluster_mode .nes. "ON" )
$ then
$    !
$    say
$DECK

Since this node is in a VMScluster, you have the option of setting up
the Ingres DBMS for VMSclusters.

This is a post installation process that can be accomplished in stages.

Stage 1.

  Run the iimkcluster script on the primary cluster node, normally the
  most capable machine in the VMS cluster.

Stage 2. 

  Run the iisunode script to add each additional node as required.
 
  Note: this script also has the option to remove a node from the 
        Clustered Ingres Installation.

Reversal of the combined effects of both IIMKCLUSTER and IISUNODE
can be accomplished by running the IIUNCLUSTER script that will
restore the configuration of the node upon which it is run, to a
stand-alone Ingres installation.

$EOD
$          !
$       say
$DECK
This procedure will set up the Ingres DBMS in stand-alone mode. 

$EOD
$       cluster_id_prompt = "FALSE"
$ endif
$
$ gosub get_cluster_details
$! 
$! Determine the correct batch queue to submit the startup procedure
$!
$ entry=0
$
$BATCH_QUEUES:
$ c_node = F$ELEMENT( entry, ",", nodes)
$
$ if (c_node .nes. ",")
$ then
$    entry = entry + 1
$
$    gosub Get_Not_Batch_Queue
$
$    goto BATCH_QUEUES
$ endif
$!
$! Generate IISTARTUP.COM and IISTARTUP_NODENAME.COM
$!
$ open/write iistartup ii_system:[ingres]iistartup.com
$ write iistartup "$!"
$ write iistartup "$! This script can be run from any account which has CMKRNL privilege to"
$ write iistartup "$! start the installation."
$ write iistartup "$! "
$ write iistartup "$ define/job/exec/trans=concealed/nolog ii_system ""''ii_system_value'"" "
$ write iistartup "$ iistartup1 = ""@ii_system:[ingres]iistartup_"" + f$getsyi(""NODENAME"") + "".com"""
$ write iistartup "$ iistartup1"
$ close iistartup
$ purge ii_system:[ingres]iistartup.com
$!
$ entry=0
$
$STARTUP_SCRIPT:
$ c_node = F$ELEMENT( entry, ",", nodes)
$
$ if (c_node .nes. ",")
$ then
$    entry = entry + 1
$
$    gosub Create_iistartup
$    goto STARTUP_SCRIPT
$ endif
$!
$! Generate default DBMS configuration.
$!
$ if( dbms_setup_status .nes. "DEFAULTS" )
$ then
$    say

Generating default configuration...
$    on error then goto IIGENRES_FAILED
$    pan_cluster_cmd "''nodes'" iigenres "" " ii_system:[ingres.files]dbms.rfm"
$    set noon
$    pan_cluster_cmd "''nodes'" iisetres "ii." ".config.dbms.''release_id' ""DEFAULTS"""
$!   an upgrade installation needs a larger buffer_count and 
$!   stack size for dbms and recovery servers
$    call Perform_setres_min_CMD "ii." ".dbms.*.stack_size" 131072
$    call Perform_setres_min_CMD "ii." ".dbms.*.vms_stack_size" 131072
$    call Perform_setres_min_CMD "ii." ".dbms.*.vms_page_file" 2000000000
$    call Perform_setres_min_CMD "ii." ".dbms.*.vms_working_set" 2048
$    call Perform_setres_min_CMD "ii." ".rcp.log.buffer_count" 35
$    call Perform_setres_min_CMD "ii." ".recovery.*.stack_size" 131072
$    call Perform_setres_min_CMD "ii." ".recovery.*.vms_page_file" 2000000000
$    call Perform_setres_min_CMD "ii." ".recovery.*.vms_stack_size" 131072
$    call Perform_setres_min_CMD "ii." ".recovery.*.vms_working_set" 2048
$    goto IIGENRES_DONE
$!
$ IIGENRES_FAILED:
$!
$    set noon
$    say
An error occured while generating the default configuration.

$    goto EXIT_FAIL 
$ else
$    say

Default configuration generated.
$ endif
$!
$ IIGENRES_DONE:
$!
$! Set necessary privileges.
$!
$ iigetres ii.'node_name.ingstart.privileges privileges
$ set_message_off
$ privileges = f$trnlnm( "privileges" )
$ deassign "privileges"
$ set_message_on
$ on error then goto NOT_PRIVILEGED
$ on warning then goto NOT_PRIVILEGED
$ set proc/priv=('privileges)
$ goto PRIVILEGED
$!
$ NOT_PRIVILEGED:
$!
$ set noon
$ say

The following privileges are required to complete this procedure:

$ echo "(''privileges')"
$ echo ""
$ goto EXIT_FAIL
$!
$ PRIVILEGED:
$!
$ set noon
$
$!
$! Prompt for II_INSTALLATION if using GROUP level logicals .
$!
$ if( lnm_table .eqs. "LNM$SYSTEM" ) then goto II_INSTALLATION_OK 
$!
$ iigetres ii.'node_name.lnm.ii_installation ii_installation_value
$ set_message_off
$ ii_installation = f$trnlnm( "ii_installation_value" )
$ deassign "ii_installation_value"
$ if( ii_installation .eqs. "" ) then -
     ii_installation = f$trnlnm( "II_INSTALLATION", "LNM$JOB" )
$ set_message_on
$ if( ( lnm_table .eqs. "LNM$GROUP" ) -
     .and. ( ii_installation .eqs. "" ) )
$ then
$    !
$    ! If the installation code has been setup on other members of
$    ! a VMScluster, then use that value.
$    !
$    if (cluster_id .nes. "")
$    then
$       !
$       tmp = 0
$       !
$       II_INSTALLATION_CLUSTER_LOOP:
$       !
$       sub1 = f$element(tmp, ",", cluster_nodelist)
$       if ( sub1 .nes. ",")
$       then
$          iigetres ii.'sub1.lnm.ii_installation ii_installation_value
$          ii_installation = f$trnlnm( "ii_installation_value" )
$          if( ii_installation .nes. "" )
$          then
$             deassign "ii_installation_value"
$             pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.ii_installation ""''ii_installation'"""
$             echo "" 
$             echo "II_INSTALLATION configured as ""''ii_installation'"""
$             goto II_INSTALLATION_OK
$          endif
$          tmp = tmp + 1
$          goto II_INSTALLATION_CLUSTER_LOOP
$       endif
$       !
$       II_INSTALLATION_CLUSTER_LOOP_END:
$       !
$    endif
$    say 

To differentiate between multiple INGRES installations set up to run on
the same machine using GROUP level logical names, you must assign each
installation a two-letter code that is unique across the machine.

The first character of the installation code must be a letter; the
second character may be a letter or a number.  You may use any code which
satisfies these conditions except for the following:

"AA" may not be used since it is the installation code used for SYSTEM
level installations. 

"TT" may not be used since it translates to the VMS terminal identifier.

The installation code you select will be assigned to the GROUP level
logical II_INSTALLATION when you start Ingres. Before you choose an
installation code, make certain that it is not already in use by another
(GROUP level) installation on this machine.

$!
$ II_INSTALLATION_PROMPT:
$!
$    inquire ii_installation "Please enter a valid installation code" 
$    if( f$length( ii_installation ) .ne. 2 ) then goto II_INSTALLATION_PROMPT
$    char1 = f$extract( 0, 1, ii_installation )
$    char2 = f$extract( 1, 1, ii_installation )
$    if( ( "''char1'" .lts. "A" ) .or. ( "''char1'" .gts. "Z" ) -
        .or. ( ( ( "''char2'" .lts. "A" ) .or. ( "''char2'" .gts. "Z" ) ) -
        .and. ( ( "''char2'" .lts. "0" ) .or. ( "''char2'" .gts. "9" ) ) ) -
        .or. ( ii_installation .eqs "AA" ) -
        .or. ( ii_installation .eqs. "TT" ) )
$    then
$       say

You have entered an invalid installation code.

$       goto II_INSTALLATION_PROMPT
$    endif
$ else
$    echo "" 
$    echo "II_INSTALLATION configured as ""''ii_installation'"""
$ endif
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.ii_installation ""''ii_installation'"""
$!
$ II_INSTALLATION_OK:
$!
$! Now set up group logical defining the installation as this will be 
$! required within iimklog
$!
$ if( lnm_table .nes. "LNM$SYSTEM" ) 
$ then 
$   define/group/exec/nolog ii_installation  "''ii_installation'"
$ endif
$!
$! After determining the installation code, we can establish
$! the names of the shared libraries and their logicals.
$!
$ if ("''ii_installation'".nes."")
$ then
$     copy ii_system:[ingres.library]clfelib.exe -
           ii_system:[ingres.library]clfelib'ii_installation.exe -
	   /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     pan_cluster_cmd "''nodes'" iisetres -
                       "ii." ".lnm.ii_compatlib ""ii_system:[ingres.library]clfelib''ii_installation'.exe"""
$     purge/nolog ii_system:[ingres.library]clfelib'ii_installation.exe
$!
$     copy ii_system:[ingres.library]framefelib.exe -
           ii_system:[ingres.library]framefelib'ii_installation.exe -
	   /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     pan_cluster_cmd "''nodes'" iisetres -
                       "ii." ".lnm.ii_framelib ""ii_system:[ingres.library]framefelib''ii_installation'.exe"""
$     purge/nolog ii_system:[ingres.library]framefelib'ii_installation.exe
$!
$     copy ii_system:[ingres.library]interpfelib.exe -
           ii_system:[ingres.library]interpfelib'ii_installation.exe -
	   /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     pan_cluster_cmd "''nodes'" iisetres -
                       "ii." ".lnm.ii_interplib ""ii_system:[ingres.library]interpfelib''ii_installation'.exe"""
$     purge/nolog ii_system:[ingres.library]interpfelib'ii_installation.exe
$!
$     copy ii_system:[ingres.library]libqfelib.exe -
           ii_system:[ingres.library]libqfelib'ii_installation.exe -
	   /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     pan_cluster_cmd "''nodes'" iisetres -
                       "ii." ".lnm.ii_libqlib ""ii_system:[ingres.library]libqfelib''ii_installation'.exe"""
$     purge/nolog ii_system:[ingres.library]libqfelib'ii_installation.exe
$!
$     copy ii_system:[ingres.library]apifelib.exe -
            ii_system:[ingres.library]apifelib'ii_installation.exe -
          /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     pan_cluster_cmd "''nodes'" iisetres -
                       "ii." ".lnm.ii_apilib ""ii_system:[ingres.library]apifelib''ii_installation'.exe"""
$     purge/nolog ii_system:[ingres.library]apifelib'ii_installation.exe
$!
$     copy ii_system:[ingres.library]iiuseradt.exe -
           ii_system:[ingres.library]iiuseradt'ii_installation.exe -
	   /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     iisetres ii.*.lnm.ii_useradt "ii_system:[ingres.library]iiuseradt''ii_installation'.exe" 
$     purge/nolog ii_system:[ingres.library]iiuseradt'ii_installation.exe
$!
$     copy ii_system:[ingres.bin]dmfjsp.exe -
           ii_system:[ingres.bin]dmfjsp'ii_installation.exe -
	   /prot=(s:rwed,o:rwed,g:rwed,w:re)
$     purge/nolog ii_system:[ingres.bin]dmfjsp'ii_installation.exe
$ endif
$!
$! II_GCNxx_LCL_VNODE must be defined at startup time, insert appropriate
$! entry.
$ if ("''ii_installation'".nes."")
$ then
$     temp_str = "ii_gcn" + ii_installation + "_lcl_vnode"
$     pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.''temp_str'" NODENAME
$ else
$     pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.ii_gcn_lcl_vnode" NODENAME
$ endif
$!
$! Before we instigate use of new images, make sure any installed images of the
$! same name are de-installed.  We may or may not have purged the old images
$! above, but if they are installed they may still be loaded in global space.
$! This will force the use of the new, uninstalled images.
$!
$ set_message_off
$ sub1 = f$environment("ON_SEVERITY")
$ set noon
$ install remove ii_system:[ingres.library]clfelib'ii_installation.exe
$ install remove ii_system:[ingres.library]libqfelib'ii_installation.exe
$ install remove ii_system:[ingres.library]framefelib'ii_installation.exe
$ if (sub1.nes."NONE") then set on
$ set_message_on
$!
$! Define shared library logicals and make sure these are
$! not confused with any installed images.
$!
$ define/nolog/job ii_compatlib ii_system:[ingres.library]clfelib'ii_installation.exe
$ define/nolog/job ii_libqlib ii_system:[ingres.library]libqfelib'ii_installation.exe
$ define/nolog/job ii_framelib ii_system:[ingres.library]framefelib'ii_installation.exe
$!
$! Don't prompt for LOCATION help if not needed
$!
$ location_help = "FALSE"
$ if (cluster_id.nes."" .and. cluster_sequence.gt.0) 
$ then 
$     goto II_DATABASE_PROMPT1
$ endif
$!
$ if( ( ii_database .eqs. "" ) .or. ( ii_checkpoint .eqs. "" ) -
     .or. ( ii_journal .eqs. "" ) .or. ( ii_dump .eqs. "" ) -
     .or. ( ii_work .eqs. "" ) .or. ( log_file_1_'node_name' .eqs. "" ) -
     .or. ( dual_log_1_'node_name' .eqs. "" ) )
$ then
$    say

This program prompts you for the default physical storage locations you
want to use (if not already configured) for the database, backup, temporary,
and transaction log files maintained by the Ingres DBMS.  Before you
respond to these prompts, you have the option of reading detailed
instructions on configuring the default Ingres storage locations.

$!
$ LOCATION_HELP_PROMPT:
$!
$    if EXPRESS
$    then
$	location_help = "FALSE"
$    else
$       inquire/nopunc location_help "Do you want instructions on configuring Ingres storage locations? (y/n) "
$    endif
$    if( location_help .eqs. "" ) then goto LOCATION_HELP_PROMPT 
$ endif
$ if( location_help )
$ then
$    say

Ingres Location Logicals
------------------------
The Ingres storage locations and their corresponding VMS logicals
are listed below:

    VMS Logical       Description
    -----------       -----------
    II_DATABASE       Default location for database files
    II_CHECKPOINT     Default location for full backup files
    II_JOURNAL        Default location for incremental backup files
    II_DUMP           Default location used to store transactions during
                       on-line backups
    II_WORK           Default location for temporary files
    II_LOG_FILE       Location for the Ingres transaction log file
    II_DUAL_LOG       Location for the backup Ingres transaction log

$    inquire/nopunc junk "Press RETURN to continue:" 
$    say

Assigning Locations
-------------------
The values you assign to the default Ingres location logicals must
be device names or concealed device logical names (i.e. "$DISK1:",
"INGDEV:").  If you do not have at least two devices or concealed
device logical names defined at this time, you should not continue
this setup procedure.

NOTE: You can change a default Ingres storage location under VMS
simply by redefining the appopriate logical(s) and relocating the
directory associated with the location.  You can also create additional
storage locations and assign individual databases to them at any
point after the setup procedure has been completed.  For information
on creating additional storage locations, refer to the Ingres Database
Administrator's Guide.

$    !
$     CONFIRM_SETUP_5:
$    !
$    inquire/nopunc answer "Do you want to continue this procedure? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_SETUP_5
$    if( .not. answer )
$    then 
$       say
*** The above component will not be installed ***
$!
$       goto EXIT_OK
$    endif
$ endif
$!
$! Configure II_DATABASE 
$!
$ location_ok = "TRUE"
$!
$ II_DATABASE_PROMPT1:
$!
$ if( ii_database .eqs. "" )
$ then
$    ! This is a workaround for an apparent DCL bug!
$    ii_database = ii_database
$ endif
$ if( ii_database .eqs. "" )
$ then
$    if( location_help .and. location_ok )
$    then
$       say

The Default Database Location
-----------------------------
The Ingres variable II_DATABASE stores the default location used for
all Ingres database files.  If the physical device corresponding to
the II_DATABASE location fails, all databases stored on it may be lost.

$	if EXPRESS then goto II_DATABASE_PROMPT2
$       inquire/nopunc junk "Press RETURN to continue:" 
$    endif
$   !
$    II_DATABASE_PROMPT2:
$   !
$    if( location_ok ) then echo ""
$    echo "Please enter the default location for your Ingres database files."
$    inquire location_root "II_DATABASE"
$    if( location_root .eqs. "" ) then goto II_DATABASE_PROMPT2
$ else
$    echo "" 
$    echo "II_DATABASE configured as ""''ii_database'"""
$    location_root = ii_database
$ endif
$ location_dir = "data"
$ gosub VERIFY_LOCATION
$ if( .not. location_ok ) then goto II_DATABASE_PROMPT1
$ ii_database = location_root
$ iisetres ii.*.lnm.ii_database "''ii_database'"
$!
$! Display II_CHECKPOINT, II_JOUNRNAL, and II_DUMP help if needed
$!
$ if( location_help .and. ( ( ii_checkpoint .eqs. "" ) .or. -
     ( ii_journal .eqs. "" ) .or. ( ii_dump .eqs. "" ) ) ) 
$ then
$    say

The Default Checkpoint, Journal and Dump Locations
--------------------------------------------------
The Ingres variables II_CHECKPOINT, II_JOURNAL, and II_DUMP are used
to store the default locations used for off-line, on-line, and incremental
on-line backup files.  It is essential that II_CHECKPOINT, II_JOURNAL,
and II_DUMP not be assigned to the same physical device as II_DATABASE.
If this restriction is ignored, you will be unable to recover lost data
in the event an II_DATABASE storage device failure.

Refer to the chapter on "Backup and Recovery" in the Ingres Database
Administrator's Guide for instructions on backup and recovery of
Ingres databases.

$    inquire/nopunc junk "Press RETURN to continue:" 
$ endif
$!
$! Configure II_CHECKPOINT
$!
$ II_CHECKPOINT_PROMPT1:
$!
$ if( ii_checkpoint .eqs. "" )
$ then
$   !
$    II_CHECKPOINT_PROMPT2:
$   !
$    if( location_ok ) then echo ""
$    echo "Please enter the default location for your Ingres checkpoint files."
$    inquire location_root "II_CHECKPOINT"
$    if( location_root .eqs. "" ) then goto II_CHECKPOINT_PROMPT2
$ else
$    echo ""
$    echo "II_CHECKPOINT configured as ""''ii_checkpoint'"""
$    location_root = ii_checkpoint
$ endif
$ location_dir = "ckp"
$ gosub VERIFY_LOCATION
$ if( .not. location_ok ) then goto II_CHECKPOINT_PROMPT1 
$ ii_checkpoint = location_root
$ iisetres ii.*.lnm.ii_checkpoint "''ii_checkpoint'"
$!
$! Configure II_JOURNAL
$!
$ II_JOURNAL_PROMPT1:
$!
$ if( ii_journal .eqs. "" )
$ then
$   !
$    II_JOURNAL_PROMPT2:
$   !
$    if( location_ok ) then echo ""
$    echo "Please enter the default location for your Ingres journal files."
$    inquire location_root "II_JOURNAL"
$    if( location_root .eqs. "" ) then goto II_JOURNAL_PROMPT2
$ else
$    echo ""
$    echo "II_JOURNAL configured as ""''ii_journal'"""
$    location_root = ii_journal
$ endif
$ location_dir = "jnl"
$ gosub VERIFY_LOCATION
$ if( .not. location_ok ) then goto II_JOURNAL_PROMPT1 
$ ii_journal = location_root
$ iisetres ii.*.lnm.ii_journal "''ii_journal'"
$!
$! Configure II_DUMP
$!
$ II_DUMP_PROMPT1:
$!
$ if( ii_dump .eqs. "" )
$ then
$   !
$    II_DUMP_PROMPT2:
$   !
$    if( location_ok ) then echo ""
$    echo "Please enter the default location for your Ingres dump files."
$    inquire location_root "II_DUMP"
$    if( location_root .eqs. "" ) then goto II_DUMP_PROMPT2
$ else
$    echo ""
$    echo "II_DUMP configured as ""''ii_dump'"""
$    location_root = ii_dump
$ endif
$ location_dir = "dmp"
$ gosub VERIFY_LOCATION
$ if( .not. location_ok ) then goto II_DUMP_PROMPT1 
$ ii_dump = location_root
$ iisetres ii.*.lnm.ii_dump "''ii_dump'"
$!
$! Configure II_WORK
$!
$ II_WORK_PROMPT1:
$!
$ if ( ii_work .eqs. "" )
$ then
$    ii_work = ii_work
$ endif
$ if( ii_work .eqs. "" )
$ then
$    if( location_help .and. location_ok )
$    then
$       say

The Default Work Location
-------------------------
The Ingres variable II_WORK stores the default location used for all
temporary files created during external sorts and other DBMS server
operations that require large amounts of temporary file space.  Ingres
lets you designate a separate storage device as the default location
for these files.

When choosing the II_WORK location, be aware that temporary files tend
to increase fragmentation on storage devices, which can lead to
decreased performance.  For this reason, you should assign II_WORK to a
physical device which does not contain database, checkpoint, journal,
or dump files, if possible.  Your choice of the default work location
does not affect your ability to recover lost data in the event of a
storage device failure, although it may affect performance.

$       if EXPRESS then goto II_WORK_PROMPT2
$       inquire/nopunc junk "Press RETURN to continue:" 
$    endif
$   !
$    II_WORK_PROMPT2:
$   !
$    if( location_ok ) then echo ""
$    echo "Please enter the default location for your Ingres work files."
$    inquire location_root "II_WORK"
$    if( location_root .eqs. "" ) then goto II_WORK_PROMPT2
$ else
$    echo ""
$    echo "II_WORK configured as ""''ii_work'"""
$    location_root = ii_work
$ endif
$ location_dir = "work"
$ gosub VERIFY_LOCATION
$ if( .not. location_ok ) then goto II_WORK_PROMPT1
$ ii_work = location_root
$ iisetres ii.*.lnm.ii_work "''ii_work'"
$!
$! Transaction log file configuration. 
$!
$ tx_log_entry = 0
$ disp_tx_log_help = 1
$
$TX_CONF:
$ tx_node = f$element( tx_log_entry, ",", nodes)
$
$ if tx_node .nes. ","
$ then
$    tx_log_entry = tx_log_entry + 1
$
$    disable_dual_log_'tx_node = "FALSE"
$
$    if( ( log_file_1_'tx_node' .eqs. "" ) .or. ( dual_log_1_'tx_node' .eqs. "" ) )
$    then
$       if( location_help .and. location_ok .and. disp_tx_log_help)
$       then
$          disp_tx_log_help = 0
$          say

Ingres Transaction Log Locations
--------------------------------
The Ingres DBMS uses a transaction log file to store uncommitted
transactions and buffer committed transactions before they are written
to the database file location(s).

In order to guarantee that no committed transacions can be lost in
the event of a single storage device failure, a backup of the Ingres
transaction log must be maintained on a storage device which is
independent of the primary transaction log file device.

By default, Ingres will maintain a backup of the primary transaction
log file on a storage location which you select.

You have the option of disabling the backup transaction log file,
although you should only do so if the location you intend to use
for your primary transaction log file has built-in fault tolerance
(e.g. a fault-tolerant disk array or "mirrored" disk sub-system).

$          inquire/nopunc junk "Press RETURN to continue:" 
$       endif
$   !
$       CONFIRM_DUAL_LOGGING:
$   !
$      echo ""
$      if EXPRESS 
$      then
$         disable_dual_log_'tx_node' = 1
$         goto TX_CONF
$      endif
$      if (cluster_id .nes. "")
$      then
$         add_text = f$fao(" on !AS ", f$edit(tx_node, "upcase"))
$      else
$         add_text = ""
$      endif
$      inquire/nopunc disable_dual_log "Do you want to disable the backup transaction log ''add_text'? (y/n) "
$      if( disable_dual_log .eqs. "" ) then goto CONFIRM_DUAL_LOGGING 
$      disable_dual_log_'tx_node' = disable_dual_log
$    endif
$
$    goto TX_CONF
$ endif
$
$ tx_log_entry = 0
$ display_tx_def_size = 1
$
$TX_SIZE:
$ tx_node = f$element( tx_log_entry, ",", nodes)
$
$ if tx_node .nes. ","
$ then
$    tx_log_entry = tx_log_entry + 1
$
$    if ( log_file_1_'tx_node' .eqs. "" )
$    then
$    !
$    ! Display transaction log size.
$    !
$       iigetres ii.'tx_node.rcp.file.kbytes log_kbytes_value 
$	set_message_off
$	log_kbytes = f$trnlnm( "log_kbytes_value" )
$	deassign "log_kbytes_value"
$	set_message_on
$       log_mb = (log_kbytes/1024)
$       log_blocks == log_mb * 2048
$!
$       log_file_name = log_file_name_'tx_node
$       log_path = log_file_1_'tx_node
$       if( log_file_name .eqs. "" ) then log_file_name = "''default_log_file'"
$       logfile = "''log_path'[ingres.log]''log_file_name'.l01"
$       if cluster_id .nes. ""
$       then
$          logfile = logfile + "_" + node_name
$       endif
$
$       if( f$search( "''logfile'" ) .eqs. "" )
$ !        LOG file does not exist - display the log file message 
$ !        and prompt for size change
$       then 
$          set_message_on
$          if display_tx_def_size
$          then
$             display_tx_def_size = 0
$             say

You must verify that the location(s) you intend to select for the 
Ingres transaction log(s) have sufficient disk space.  The default 
transaction log size for Ingres is:

$             echo "   ''log_blocks' blocks (''log_mb'MB)"
$          endif
$	   if EXPRESS then goto TX_SIZE
$          call II_SELECT_LOG_SIZE "''tx_node'" 'log_kbytes
$          if (do_abort) then goto EXIT_FAIL
$       endif
$       set_message_on
$    endif
$
$    goto TX_SIZE
$ endif
$!
$! Configure II_LOG_FILE
$!
$ II_LOG_FILE_PROMPT1:
$!
$ tx_log_entry = 0
$ display_cluster_msg = 1
$
$TX_PRIM_LOC:
$ tx_node = f$element( tx_log_entry, ",", nodes)
$
$ if tx_node .nes. ","
$ then
$    tx_log_entry = tx_log_entry + 1
$
$    II_BAD_LOG_DEFINED:
$
$    if( log_file_1_'tx_node .eqs. "" )
$    then
$       if( cluster_id .nes. "" ) 
$       then
$          if (display_cluster_msg)
$          then
$             display_cluster_msg = 0
$             say

Since you are creating an Ingres DBMS installation for VMS clusters,
the location(s) you specify for the Ingres transaction log(s) must be
on a cluster-mounted device.
$          endif
$
$          add_text = f$fao(" on !AS", f$edit(tx_node, "upcase"))
$       else
$          add_text = ""
$       endif
$   !
$       II_LOG_FILE_PROMPT2:
$   !
$       if( location_ok ) then echo ""
$       echo "Please enter a location for the primary Ingres transaction log''add_text'."
$       inquire location_root "II_LOG_FILE"
$       if( location_root .eqs. "" ) then goto II_LOG_FILE_PROMPT2
$
$       location_dir = "log"
$       gosub VERIFY_LOCATION
$       if( .not. location_ok ) then goto II_LOG_FILE_PROMPT2
$       ii_log_file = location_root
$       iisetres ii.'tx_node.rcp.log.log_file_1 "''ii_log_file'"
$       iisetres ii.'tx_node.rcp.log.log_file_name 'default_log_file'
$    else
$       echo "" 
$
$       if ( cluster_id .eqs. "" )
$       then
$          location_root = log_file_1_'tx_node
$          echo "II_LOG_FILE configured as ""''location_root'"""
$       else
$          location_root = log_file_1_'tx_node
$          echo "II_LOG_FILE (''tx_node') configured as ""''location_root'"""
$       endif
$       location_dir = "log"
$       gosub VERIFY_LOCATION
$       if( .not. location_ok )
$       then
$          log_file_1_'tx_node = ""
$
$          goto II_BAD_LOG_DEFINED
$       endif
$    endif
$
$    goto TX_PRIM_LOC
$ endif
$!
$! Configure II_DUAL_LOG
$!
$ II_DUAL_LOG_PROMPT1:
$!
$ tx_log_entry = 0
$
$TX_DUAL_LOC:
$ tx_node = f$element( tx_log_entry, ",", nodes)
$
$ if tx_node .nes. ","
$ then
$    tx_log_entry = tx_log_entry + 1
$
$    if( ( dual_log_1_'tx_node .eqs. "" ) .and. ( .not. disable_dual_log_'tx_node' ) )
$    then
$       if( cluster_id .nes. "" ) 
$       then
$          if (display_cluster_msg)
$          then
$             display_cluster_msg = 0
$             say

Since you are creating an Ingres DBMS installation for VMS clusters,
the location(s) you specify for the Ingres transaction log(s) must be
on a cluster-mounted device.
$          endif
$
$          add_text = f$fao(" on !AS", f$edit(tx_node, "upcase"))
$       else
$          add_text = ""
$       endif
$       !
$       II_DUAL_LOG_PROMPT2:
$       !
$       if( location_ok ) then echo ""
$       echo "Please enter a location for the backup transaction log''add_text'."
$       inquire location_root "II_DUAL_LOG"
$       if( location_root .eqs. "" ) then goto II_DUAL_LOG_PROMPT2
$       gosub VERIFY_LOCATION
$       if( .not. location_ok ) then goto II_LOG_FILE_PROMPT2
$       ii_log_file = location_root
$       iisetres ii.'tx_node.rcp.log.dual_log_1 "''ii_log_file'"
$    else
$       if (.not. disable_dual_log_'tx_node' )
$       then
$          echo "" 
$
$          if ( cluster_id .eqs. "" )
$          then
$             location_root = dual_log_1_'tx_node
$             echo "II_DUAL_LOG configured as ""''location_root'"""
$          else
$             location_root = log_file_1_'tx_node
$             echo "II_DUAL_LOG (''tx_node') configured as ""''location_root'"""
$          endif
$       endif
$    endif
$
$    goto TX_DUAL_LOC
$ endif
$ goto LOCATIONS_DONE
$!
$ VERIFY_LOCATION: ! sub-routine
$!
$ if( ( f$locate( ":", location_root ) .ne. -
     f$length( location_root ) - 1 ) .or. -
     f$locate( "[", location_root ) .ne. -
     f$length( location_root ) )
$ then
$    echo ""
$    echo "The location you have entered is invalid.
$    say

The value assigned to an Ingres location logical must be the name of a
device or a concealed device logical (i.e. "$DISK1:", "INGDEV:").
$    goto VERIFY_LOCATION_FAILED
$ endif
$!
$ if( f$getdvi("''location_root'","EXISTS") .eqs. "FALSE")
$ then
$    say

The location's device does not exist.  Please enter another name 
for the device or concealed device logical.
$    goto VERIFY_LOCATION_FAILED
$ endif
$!
$ if( f$getdvi("''location_root'","MNT") .eqs. "FALSE")
$ then
$    say

The location's device is not mounted.  Please mount the device
or enter another name for the device or concealed device logical.
$    goto VERIFY_LOCATION_FAILED
$ endif
$!
$ set_message_off
$ if( f$search( "''location_root'[000000]ingres.dir" ) .eqs. "" )
$ then
$    set_message_on
$    echo "" 
$    echo "Creating ''location_root'[ingres] ..."
$    on error then goto VERIFY_LOCATION_FAILED
$    create/directory 'location_root[ingres]
$    set noon
$    set_message_off
$    set prot=(s:rwed,o:rwed) 'location_root:[000000]ingres.dir
$    set_message_on
$ endif
$ set_message_off
$ if( f$search( "''location_root'[ingres]''location_dir'.dir" ) .eqs. "" )
$ then
$    set_message_on
$    echo "" 
$    echo "Creating ''location_root'[ingres.''location_dir'] ..."
$    on error then goto VERIFY_LOCATION_FAILED
$    create/directory 'location_root[ingres.'location_dir]
$    set noon
$    set_message_off
$    set prot=(s:rwed,o:rwed) 'location_root[ingres]'location_dir.dir
$    set_message_on
$ endif
$ set_message_on
$ location_ok = "TRUE"
$ set noon
$ return
$!
$ VERIFY_LOCATION_FAILED:
$!
$ set noon
$ echo ""
$ location_ok = "FALSE"
$ return
$!
$ LOCATIONS_DONE:
$!
$! Issue warning about USERS. file being obsolete
$!
$ set_message_off
$ if( f$search( "ii_system:[ingres.files]users." ) .nes. "" )
$ then
$    say

WARNING: "II_SYSTEM:[INGRES.FILES]USERS." IS NOW OBSOLETE.
$ endif
$ set_message_on
$!
$! Give SYSTEM and installation owner all Ingres privileges 
$!
$ entry = 0
$
$INGRES_PRIVS:
$ c_node = f$element(entry, ",", nodes)
$
$ if (c_node .nes. ",")
$ then
$    c_user = ing_user_'c_node
$
$    gosub Set_User_Priv
$
$    entry = entry + 1
$
$    goto INGRES_PRIVS
$ endif
$!
$! Configure II_TIMEZONE_NAME
$!
$ iigetres ii.'node_name.lnm.ii_timezone_name ii_timezone_name_value
$ set_message_off
$ ii_timezone_name = f$trnlnm( "ii_timezone_name_value" )
$ deassign "ii_timezone_name_value"
$ if( ii_timezone_name .eqs. "" ) then -
     ii_timezone_name = f$trnlnm( "ii_timezone_name", "''lnm_table'" )
$ set_message_on
$ if( ii_timezone_name .eqs. "" )
$ then
$    !
$    ! If the Timezone name has been setup on other members of
$    ! a VMScluster, then look for that value.
$    !
$    if (cluster_id.nes."" .and. cluster_sequence.gt.0) 
$    then
$       !
$       tmp = 0
$       !
$       II_TIMEZONE_CLUSTER_LOOP:
$       !
$       sub1 = f$element(tmp, ",", cluster_nodelist)
$       if (sub1 .nes. ",")
$       then
$          iigetres ii.'sub1.lnm.ii_timezone_name ii_timezone_name_value
$          ii_timezone_name = f$trnlnm( "ii_timezone_name_value" )
$          if( ii_timezone_name .nes. "" )
$          then
$              deassign "ii_timezone_name_value"
$              echo "" 
$              echo "II_TIMEZONE_NAME configured as ''ii_timezone_name'."
$              goto II_TIMEZONE_OK
$          endif
$          tmp = tmp + 1
$          goto II_TIMEZONE_CLUSTER_LOOP
$       endif
$       !
$       II_TIMEZONE_CLUSTER_LOOP_END:
$       !
$    endif
$    say

You must now specify the time zone this installation is located in.
To specify a time zone, you must first select the region of the world
this installation is located in.  You will then be prompted with a
list of the time zones in that region.
$!
$ REGION_PROMPT:
$!
$    set noon
$    iivalres -v ii.*.setup.region BOGUS_REGION -
        ii_system:[ingres.files]dbms.rfm
$!
$ REGION_PROMPT_2:
$!
$    set noon
$    inquire/nopunc region "Please enter one of the named regions: "
$    if( region .eqs. "" ) then goto REGION_PROMPT_2
$    on error then goto REGION_PROMPT_2
$    iivalres -v ii.*.setup.region 'region ii_system:[ingres.files]dbms.rfm
$    set noon
$    say

If you have selected the wrong region, press RETURN for another chance.
$    set noon
$    iivalres -v ii.*.setup.'region.tz BOGUS_TIMEZONE -
        ii_system:[ingres.files]dbms.rfm
$!
$ TIMEZONE_PROMPT:
$!
$    set noon
$    inquire/nopunc ii_timezone_name "Please enter a valid time zone: " 
$    if( ii_timezone_name .eqs. "" )
$    then 
$       say

Returning to region prompt...
$       goto REGION_PROMPT
$    endif
$    on error then goto TIMEZONE_PROMPT
$    iivalres -v ii.*.setup.'region.tz 'ii_timezone_name -
        ii_system:[ingres.files]dbms.rfm
$    set noon
$    iigetres ii.*.setup.'region.'ii_timezone_name tz_text
$    tz_text = f$trnlnm( "tz_text" )
$    deassign "tz_text"
$    say

The time zone you have selected is:

$    echo "	''ii_timezone_name' (''tz_text')"
$    if EXPRESS then goto II_TIMEZONE_OK
$    say

If this is not the correct time zone, you will be given the opportunity to
select another region.

$    !
$     CONFIRM_TIMEZONE:
$    !
$    inquire/nopunc answer "Is this time zone correct? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_TIMEZONE
$    if( .not. answer )
$    then
$       say

Please select another time zone region.
$       goto REGION_PROMPT
$    endif
$ else
$    echo "" 
$    echo "II_TIMEZONE_NAME configured as ''ii_timezone_name'."
$ endif
$ II_TIMEZONE_OK:
$
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.ii_timezone_name ""''ii_timezone_name'"""
$!
$! Configure II_CHARSETxx
$!
$ iigetres ii.'node_name.lnm.ii_charset'ii_installation ii_charset_value
$ set_message_off
$ ii_charset = f$trnlnm( "ii_charset_value" )
$ deassign "ii_charset_value"
$ if( ii_charset .eqs. "" ) then -
     ii_charset = f$trnlnm( "II_CHARSET''ii_installation'", "LNM$JOB" )
$ set_message_on
$ if( ii_charset .eqs. "" )
$ then
$    !
$    ! If the Charset has been setup on other members of
$    ! a VMScluster, then look for its value.
$    !
$    if (cluster_id.nes."" .and. cluster_sequence.gt.0) 
$    then
$       !
$       tmp = 0
$       !
$       II_CHARSET_CLUSTER_LOOP:
$       !
$       sub1 = f$element(tmp, ",", cluster_nodelist)
$       if (sub1 .nes. ",")
$       then
$          iigetres ii.'sub1.lnm.ii_charset'ii_installation ii_charset_value
$          ii_charset = f$trnlnm( "ii_charset_value" )
$          if( ii_charset .nes. "" )
$          then
$             deassign "ii_charset_value"
$             echo "" 
$             echo "II_CHARSET''ii_installation' configured as ''ii_charset'."
$             goto II_CHARSET_OK
$          endif
$          tmp = tmp + 1
$          goto II_CHARSET_CLUSTER_LOOP
$       endif
$       !
$       II_CHARSET_CLUSTER_LOOP_END:
$       !
$    endif
$    if EXPRESS then goto CHARSET_PROMPT
$    say

Ingres supports different character sets.  You must now enter the
character set you want to use with this Ingres installation.

IMPORTANT NOTE: You will be unable to change character sets once you
make your selection.  If you are unsure of which character set to use,
exit this program and refer to the Ingres Installation Guide.
$    iivalres -v ii.*.setup.ii_charset BOGUS_CHARSET
$!
$ CHARSET_PROMPT:
$!
$    set noon
$    iigetres ii.'node_name.setup.ii_charset default_charset
$    default_charset = f$trnlnm( "default_charset" )
$    deassign "default_charset"
$    if EXPRESS 
$    then
$	ii_charset = default_charset
$	goto SKIP_INQ_D
$    endif
$    inquire/nopunc ii_charset "Please enter a valid character set [''default_charset'] " 
$    if( ii_charset .eqs. "" ) then -
        ii_charset = default_charset 
$    on error then goto CHARSET_PROMPT
$ SKIP_INQ_D:
$    iivalres -v ii.*.setup.ii_charset 'ii_charset 
$    set noon
$    iigetres ii.*.setup.charset.'ii_charset charset_text
$    charset_text = f$trnlnm( "charset_text" )
$    deassign "charset_text"
$    if EXPRESS then goto II_CHARSET_OK
$    say

The character set you have selected is:

$    echo "	''ii_charset' (''charset_text')"
$    echo "" 
$    !
$     CONFIRM_CHARSET:
$    !
$    if EXPRESS then goto II_CHARSET_OK
$    inquire/nopunc answer "Is this the character set you want to use? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_CHARSET
$    if( .not. answer )
$    then
$       iivalres -v ii.*.setup.ii_charset BOGUS_CHARSET
$       goto CHARSET_PROMPT
$    endif
$ else
$    echo "" 
$    echo "II_CHARSET''ii_installation' configured as ''ii_charset'."
$ endif
$ II_CHARSET_OK:
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".lnm.ii_charset''ii_installation' ""''ii_charset'"""
$!
$! Create memory directory if it doesn't exist. 
$!
$ set_message_off
$ if( f$search( "ii_system:[ingres.files]memory.dir") .eqs. "" ) then -
$    create/directory ii_system:[ingres.files.memory]
$ if (cluster_id .nes. "")
$ then
$    entry = 0
$
$CREATE_FILES_MEM:
$    c_node = f$element( entry, ",", nodes)
$
$    if (c_node .nes. ",")
$    then
$       entry = entry + 1
$
$       if (f$search( "ii_system:[ingres.files.memory]''c_node'.dir") .eqs. "")
$       then
$          ! Need to create the cluster node's memory area.
$
$          create/dir ii_system:[ingres.files.memory.'c_node]
$       endif
$
$       goto CREATE_FILES_MEM
$    endif
$ endif
$
$ set_message_on
$!
$! Prompt for concurrent users. 
$!
$ iigetres ii.'node_name.dbms.*.connect_limit default_users 
$ default_users = f$trnlnm( "default_users" )
$ deassign "default_users"
$ echo ""
$!
$ if EXPRESS 
$ then 
$ 	user_limit = 'default_users
$	goto SKIP_INQ_9
$ endif
$ CONCURRENT_USERS_PROMPT:
$ set_message_on
$ inquire/nopunc user_limit "How many concurrent users do you want to support? [''default_users'] "
$ if( user_limit .eqs. "" ) then user_limit = 'default_users
$ on error then goto CONCURRENT_USERS_PROMPT
$ SKIP_INQ_9:
$ iivalres -v ii.'node_name.dbms.*.connect_limit 'user_limit 
$ set noon
$ say

Updating configuration...
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".dbms.*.connect_limit ''user_limit'"
$ default_users = 32
$ on error then goto CONCURRENT_USERS_PROMPT
$
$!###
$!### HANDLE SYSCHECK FAILURE
$!###
$ iisyschk
$ set_message_on
$ set noon
$!
$! Make sure ingstart invokes image installation
$!
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".ingstart.*.begin ""@ii_system:[ingres.utility]iishare"""
$!
$! Create or erase the primary transaction log.
$!
$! First verify that there isn't an IPM process still using
$! the Locking/Logging shared memory.
$!
$ check_lglk_mem "''ii_installation'"
$ entry = 0
$
$Primary_TX_log:
$ c_node = f$element(entry, ",", nodes)
$
$ if (c_node .nes. ",")
$ then
$    call SET_UP_TX_LOG_FILES 'c_node'
$
$    if do_abort then goto EXIT_FAIL
$
$    entry = entry + 1
$
$    goto Primary_TX_log
$ endif
$!
$!
$ DATE_TYPE_ALIAS:
$ set_message_off
$ iigetres ii.'node_name'.config.date_alias    default_date_alias
$ default_date_alias = f$trnlnm("default_date_alias")
$ deassign   "default_date_alias"
$ if "''default_date_alias'" .nes. "" 
$ then
$     echo " "
$     echo "For current configuration, the date data type associates to ''default_date_alias' "
$     echo " "
$ else
$     default_date_alias = "ingresdate"
$ endif
$ set_message_on
$!
$ DATE_ALIAS_LOOP:
$ ii_date_alias = ""
$ say

  You can configure 'date' keyword in SQL to refer to ingresdate or
  ansidate data type.

  Ingresdate is a composite data type and stores date, time,
  timestamp or interval data in the same column.
  Before the introduction of ansidate, date referred to ingresdate.

  Ansidate refers to ANSI specified Date type and can only store a
  date value, in the format YYYY-MM-DD.

  The value you specify will be used to set the date_type_alias
  configuration parameter. If you choose no for associating
  ingresdate to date it will be set to ansidate.

$!
$ inquire/nopunc answer "Do you wish the date data type to be associated with ingresdate (y/n) [y] ? "
$ if (answer .eqs. "") then answer = "y"
$ answer = F$EDIT(answer, "TRIM,LOWERCASE")
$ if (answer .eqs. "y") .or. (answer .eqs. "yes")
$    then 
$    say

        Date refers to ingresdate data type

$    ii_date_alias = "ingresdate"
$    goto DATE_ALIAS_EXIT
$ endif
$!
$ if (answer .eqs. "n") .or. (answer .eqs. "no")
$    then 
$    say

        Date refers to ansidate data type

$    ii_date_alias = "ansidate"
$    goto DATE_ALIAS_EXIT
$ endif
$!
$ say 

    Please enter [y]es or [n]o

$ goto DATE_ALIAS_LOOP
$!
$ DATE_ALIAS_EXIT:
$ pan_cluster_cmd "''nodes'" iisetres "ii." ".dbms.*.date_type_alias   ''ii_date_alias'" 
$ goto RCPCONFIG_DONE
$!
$!
$ RCPCONFIG_DONE:
$!
$! We're all done when this is an additional cluster node
$!
$!
$    iigetres ii.'node_name.ingstart.*.rmcmd rmcmd_servers
$    set_message_off
$    rmcmd_servers = f$trnlnm( "rmcmd_servers" )
$    deassign "rmcmd_servers"
$    set_message_on
$    if (( rmcmd_servers .nes. "" )  .and. ( rmcmd_servers .gt. 0))
$    then
$       rmcmd_setup = "TRUE"
$	iisetres ii.'node_name.ingstart.*.rmcmd 0
$    else
$       rmcmd_setup = "FALSE"
$    endif

$! Create or upgrade system catalogs. 
$!
$ ckpdb		:= $ii_system:[ingres.bin]dmfjsp'ii_installation.exe ckpdb
$ set_message_off
$ if( f$search( "''ii_database'[ingres.data]iidbdb.dir" ) .nes. "" )
$ then
$    upgrade_dbs = "TRUE"
$    say
Removing defunct utilities and files...
$    delete/noconfirm ii_system:[ingres]acp_start.com;* 
$    delete/noconfirm ii_system:[ingres]acp_start.log;* 
$    delete/noconfirm ii_system:[ingres]iibuild.com;* 
$    delete/noconfirm ii_system:[ingres]iigcc.dmp;* 
$    delete/noconfirm ii_system:[ingres]iijobdef.com;* 
$    delete/noconfirm ii_system:[ingres]iircpconfig.com_'node_name';* 
$    delete/noconfirm ii_system:[ingres]iistartup.del;* 
$    delete/noconfirm ii_system:[ingres]iistartup1.log_'node_name;* 
$    delete/noconfirm ii_system:[ingres]iistartupinit.com;* 
$    delete/noconfirm ii_system:[ingres]iistartupinit1.com;* 
$    delete/noconfirm ii_system:[ingres]lgmemsz.dat;* 
$    delete/noconfirm ii_system:[ingres]runbe.com;* 
$    delete/noconfirm ii_system:[ingres]runsv.com;* 
$    delete/noconfirm ii_system:[ingres.files]rcpconfig.dat;* 
$    delete/noconfirm ii_system:[ingres.utility]iisymboldef.com;* 
$    set_message_on
$
$    ! As this is an upgrade need to ensure the correct privileges are defined
$    PIPE IIINITRES vms_privileges >nla0:
$    PIPE IIINITRES privileges >nla0:
$!
$    if f$search("ii_system:[ingres.utility]slutil.exe") .nes. "" 
$    ! If slutil.exe is installed then this installation is B1,
$    ! so don't upgrade now, do it later in iisues.com
$    then
$        say
This is a B1 installation. II_SYSTEM:[ingres.utility]iisues.com must be run
to upgrade existing databases.

$	 goto FINISH_SETUP
$    endif
$    say

Before you can access the existing databases in this installation, they
must be upgraded to support the new release of the Ingres server which
you have installed.  You have the option of upgrading all the databases
contained in this installation now.  Alternately, you can upgrade them
later by hand using the "upgradedb" utility.

Be aware that the time required to upgrade all local databases will
depend on their size, number, and complexity; therefore it is not possible
to accurately predict how long this will take.  Also note that the Ingres
system catalogs (iidbdb) will be upgraded at this time whether or not
you choose to upgrade your databases.

$!
$ UPGRADE_ALL_PROMPT:
$!
$    if EXPRESS
$    then
$	upgrade_all = "FALSE"
$    else
$    	inquire/nopunc answer "Upgrade all databases in this installation now? (y/n) "
$    endif
$    if( answer .eqs. "" ) then goto UPGRADE_ALL_PROMPT
$    if( answer )
$    then
$       upgrade_all = "TRUE"
$       echo ""
$       echo "Starting the Ingres server to upgrade system catalogs and databases..."
$    else
SKIP_INQ_A:
$       upgrade_all = "FALSE"
$       echo ""
$       echo "Starting the Ingres server to upgrade system catalogs..."
$    endif 
$!
$! If Star servers have previously been setup, disable them while upgrading
$! IIDBDB.  Restart Star if all databases are to be upgraded (67384)
$!
$    iigetres ii.'node_name.ingstart.*.star star_servers
$    iigetres ii.'node_name.ingstart.*.rms  rms_servers
$    set_message_off
$    star_servers = f$trnlnm( "star_servers" )
$    deassign "star_servers"
$    rms_servers = f$trnlnm( "rms_servers" )
$    deassign "rms_servers"
$    set_message_on
$    if (( star_servers .nes. "" )  .and. ( star_servers .gt. 0))
$    then
$       star_setup = "TRUE"
$	iisetres ii.'node_name.ingstart.*.star 0
$    else
$       star_setup = "FALSE"
$    endif
$    if (( rms_servers .nes. "" )  .and. ( rms_servers .gt. 0))
$    then
$       rms_setup = "TRUE"
$       iisetres ii.'node_name.ingstart.*.rms 0
$    else
$       rms_setup = "FALSE"
$    endif
$    on error then goto INGSTART_FAILED
$    ingstart
$    on error then goto UPGRADEDB_FAILED
$
$    if rms_setup
$    then
$       iisetres ii.'node_name.ingstart.*.rms 'rms_servers
$    endif
$    if ( .not. star_setup )
$    then
$	if( upgrade_all )
$	then
$	    say
Upgrading system catalogs and databases...
$	    upgradedb -all
$	else
$	    say
Upgrading system catalogs...
$	    upgradedb iidbdb 
$	endif
$    else
$! Star server was configured, upgrade iidbdb separately even if user specified
$! "all"
$	say
Upgrading system catalogs...
$	upgradedb iidbdb 
$	iisetres ii.'node_name.ingstart.*.star 'star_servers 
$	if( upgrade_all )
$	then
$! Restart Star, wait a little for it to complete its processing of IIDBDB
$! tables.  IIDBDB was just upgraded, now a Star server can read it.  
$	    on error then goto INGSTART_STAR_FAILED
$	    ingstart -iistar
$	    wait 00:01
$	    on error then goto UPGRADEDB_FAILED
$	    say
Now upgrading databases...
$	    upgradedb -all
$	endif
$    endif
$    goto UPGRADEDB_DONE
$!
$ UPGRADEDB_FAILED:
$!
$    echo "Shutting down the Ingres server..."
$    define/user sys$output 'tmpfile
$    ingstop
$    delete/noconfirm 'tmpfile;*
$    if( upgrade_all )
$    then
$       say
An error occurred while upgrading your system catalogs or one of the
databases in this installation.  You should contact Ingres Corporation
Technical Support to resolve this problem.

$    else
$       say
An error occurred while upgrading your system catalogs.  You should
contact Ingres Corporation Technical Support to resolve this problem.

$    endif
$    goto EXIT_FAIL
$!
$ INGSTART_FAILED:
$!
$    echo "Shutting down the Ingres server..."
$    define/user sys$output 'tmpfile
$    ingstop
$    delete/noconfirm 'tmpfile;*
$    say

The Ingres server could not be started.  See the server error log
(II_SYSTEM:[INGRES.FILES]ERRLOG.LOG) for a full description of the
problem. Once the problem has been corrected, you must re-run this
setup program before attempting to use the installation.

$    goto EXIT_FAIL
$!
$ INGSTART_STAR_FAILED:
$!
$    echo "Shutting down the Ingres servers..."
$    define/user sys$output 'tmpfile
$    ingstop
$    delete/noconfirm 'tmpfile;*
$    say

A previously-configured Ingres Star server could not be started.  
See the server error log (II_SYSTEM:[INGRES.FILES]ERRLOG.LOG) for a full
description of the problem.  Once the problem has been corrected, you 
must re-run this setup program before attempting to use the installation.

$    goto EXIT_FAIL
$!
$ UPGRADEDB_DONE:
$!
$! Checkpoint the system catalogs.
$!
$    echo "Checkpointing the Ingres system catalogs..."
$    on error then goto UPGRADE_CHECKPOINT_FAILED
$    ckpdb "+j" iidbdb
$    goto UPGRADE_CHECKPOINT_DONE
$   !
$    UPGRADE_CHECKPOINT_FAILED:
$   !
$    say

An error occurred while checkpointing your system catalogs.
You should contact Ingres Corporation Technical Support to
resolve this problem.

$    goto SHUTDOWN_SERVER1
$!
$ UPGRADE_CHECKPOINT_DONE:
$    on error then continue
$!
$    create_imadb = 0
$    if f$search("II_SYSTEM:[ingres.utility]rmcmdgen.exe") .nes. ""
$    then
$         say

Upgrading imadb...

$	upgradedb imadb
$
$      if .not. upgrade_all
$      then
$	  UPG20 = ""
$         set_message_off
$	  search II_CONFIG:config.dat "config.dbms.ii20" /exact /out='tmpfile
$	  set_message_on
$	  open tmp 'tmpfile
$	  read /end=NO_UPG20 tmp line
$	  line = f$edit(line, "collapse")
$	  UPG20 = f$extract(f$locate(":", line), 255, line)
$NO_UPG20:
$	  close tmp
$	  delete 'tmpfile;*
$	  if UPG20 .nes. ""
$	  then
$	     say

Removing VDBA catalogs from iidbdb...

$	     rmcmdrmv "-u''ing_user'" -diidbdb
$	  else
$	     rmcmdrmv
$	  endif
$      endif
$      if (f$search( "''ii_database'[ingres.data]imadb.dir") .eqs. "")
$      then
$         createdb -fnofeclients "-u$ingres" imadb
$	  create_imadb = 1
$      endif
$      sql "-u$ingres" imadb <II_SYSTEM:[ingres.vdba]makimav.sql >II_SYSTEM:[ingres.vdba]makimav.out
$      say

Initializing imadb for rmcmd...

$      if .not. create_imadb
$      then rmcmdrmv
$      endif
$
$      type sys$input /out=ii_system:[ingres]rmcmdcmd.get_owner
declare global temporary table session.remotecmd_owner as
select relowner
from iirelation
where relowner not in ('$ingres','$INGRES') AND
      relid in ('remotecmd', 'REMOTECMD')
on commit preserve rows with norecovery;

copy session.remotecmd_owner(relowner = char(50))
into 'ii_system:[ingres]rmcmdcmd.owner'\g
$      sql -s imadb <ii_system:[ingres]rmcmdcmd.get_owner >nla0:
$
$      open/read tmp ii_system:[ingres]rmcmdcmd.owner
$RMCMD_OWNER:
$      read/end=NOMORE_OWNER tmp line
$
$      owner = F$EDIT(line, "TRIM")
$      rmcmdrmv "-u''owner'"
$      goto RMCMD_OWNER

$NOMORE_OWNER:
$      close tmp
$      delete ii_system:[ingres]rmcmdcmd.get_owner;*,ii_system:[ingres]rmcmdcmd.owner;*
$
$      rmcmdgen
$    endif
$!
$ SHUTDOWN_SERVER1:
$    echo "Shutting down the Ingres server..."
$    define/user sys$output 'tmpfile
$    ingstop
$    delete/noconfirm 'tmpfile;* 
$    say
Ingres DBMS setup complete.

Refer to the Ingres Installation Guide for information about 
starting and using Ingres.

$    if (( rmcmd_servers .nes. "" )  .and. ( rmcmd_servers .gt. 0))
$    then
$	iisetres ii.'node_name.ingstart.*.rmcmd 1
$    endif
$    pan_cluster_cmd "''nodes'" iisetres "ii." ".config.dbms.''release_id' ""COMPLETE""" 
$ else
$    ! prompt whether to support ANSI/ISO entry-level SQL-92
$    say
The Ingres DBMS can be configured to be compliant with the ANSI/ISO
entry-level SQL-92 standard.  If you need information about this standard,
please refer to the SQL Reference Manual.

$   !
$    CONFIRM_SQL_92_SUPPORT:
$   !
$    if EXPRESS then goto SKIP_INQ_B
$    inquire/nopunc answer "Do you want Ingres to comply with this standard? (y/n) "
$    if( answer .eqs. "" ) then goto CONFIRM_SQL_92_SUPPORT
$    if( answer )
$    then
$       iisetres ii.'node_name.fixed_prefs.iso_entry_sql-92 ON
$    endif
$    set_message_on
$ SKIP_INQ_B:
$    if f$search("ii_system:[ingres.utility]slutil.exe") .nes. "" 
$    ! If slutil.exe is installed then this installation is B1,
$    ! so don't create iidbdb now, do it later in iisues.com
$	then
$    say
This is a B1 installation. II_SYSTEM:[ingres.utility]iisues.com must be run.

$	 goto FINISH_SETUP
$      endif
$!
$    ! Createdb iidbdb
$    say

Starting the Ingres server to initialize system catalogs..
$    on error then goto INGSTART_FAILED
$    ingstart
$    set noon
$    say
Initializing the Ingres system catalogs (creating database 'iidbdb')...

$    on error then goto CATALOG_CREATION_FAILED
$    createdb "-u''ing_user'" "-S" iidbdb 
$    set noon
$!
$! Checkpoint the system catalogs.
$!
$    echo "Checkpointing the Ingres system catalogs..."
$    on error then goto CREATE_CHECKPOINT_FAILED
$    wait 00:00:05
$    ckpdb "+j" iidbdb
$    set noon
$    goto CREATE_CHECKPOINT_DONE
$   !
$    CREATE_CHECKPOINT_FAILED:
$   !
$    set noon
$    say

An error occurred while checkpointing your system catalogs.
You should contact Ingres Corporation Technical Support to
resolve this problem.
$    goto SHUTDOWN_SERVER
$!
$ CREATE_CHECKPOINT_DONE:
$ on error then continue
$!
$ if f$search("II_SYSTEM:[ingres.utility]rmcmdgen.exe") .nes. ""
$ then
$    on error then goto SHUTDOWN_SERVER
$    createdb -fnofeclients "-u$ingres" imadb
$    say

Initializing imadb for vdba...

$    sql "-u$ingres" imadb <II_SYSTEM:[ingres.vdba]makimav.sql >II_SYSTEM:[ingres.vdba]makimav.out
$    say

Initializing imadb for rmcmd...

$    rmcmdgen
$  endif
$!
$ SHUTDOWN_SERVER:
$    say

Shutting down the Ingres server...
$    define/user sys$output 'tmpfile 
$    ingstop
$    delete/noconfirm 'tmpfile;* 
$    say
Ingres DBMS setup complete.

Refer to the Ingres Installation Guide for information about 
starting and using Ingres.

$ FINISH_SETUP:
$    if (( rmcmd_servers .nes. "" )  .and. ( rmcmd_servers .gt. 0))
$    then
$	iisetres ii.'node_name.ingstart.*.rmcmd 1
$    endif
$    pan_cluster_cmd "''nodes'" iisetres "ii." ".config.dbms.''release_id' ""COMPLETE"""
$ endif 
$ goto EXIT_OK
$!
$ CATALOG_CREATION_FAILED:
$!
$ say

Cleaning up...
$ set_message_off
$ delete/noconfirm ii_database:[ingres.data...]*.*;*
$ delete/noconfirm ii_database:[ingres.data...]*.*;*
$ delete/noconfirm ii_work:[ingres.work...]*.*;*
$ delete/noconfirm ii_work:[ingres.work...]*.*;*
$ set_message_on
$ echo ""
$ echo "Shutting down the Ingres server..."
$ define/user sys$output 'tmpfile 
$ ingstop
$ delete/noconfirm 'tmpfile;* 
$ say

The system catalogs were not created successfully.   You must correct
the problem described above and re-run this setup program before this
installation can be used.

$ goto EXIT_FAIL 
$!
$ INGSTART_FAILED:
$!
$ define/user sys$output 'tmpfile 
$ ingstop
$ delete/noconfirm 'tmpfile;* 
$ say
The Ingres server could not be started.  See the server error log
(II_SYSTEM:[INGRES.FILES]ERRLOG.LOG) for a full description of the
problem. Once the problem has been corrected, you must re-run this
setup program before attempting to use the installation.

$!
$ EXIT_OK:
$ status = SS$_NORMAL
$ goto exit_ok1
$!
$ EXIT_FAIL:
$! There is a small window where an abort can leave ii_system undefined
$ if "''ii_system_value'".eqs."''f$trnlnm( "ii_system_value" )'"
$ then
$     if "''f$trnlnm("II_SYSTEM")'".eqs.""
$     then
$         define/job ii_system "''ii_system_value'"
$     endif
$ endif
$ status = SS$_ABORT
$exit_ok1:
$!
$ on control_c then goto EXIT_FAIL
$ on control_y then goto EXIT_FAIL
$!
$! Restore previous values of shared library definitions
$!
$ set_message_off
$ entry = 0
$LOG_PROMPT_LOG:
$ node = f$element( entry, ",", nodes)
$
$ if node .nes. ","
$ then
$    entry = entry + 1
$
$    if f$trnlnm( "log_size_prompted_''node'") .nes. ""
$    then
$       deassign "log_size_prompted_''node'"
$    endif
$
$    goto LOG_PROMPT_LOG
$ endif
$ if "''ii_compatlib_def'".eqs."" 
$ then
$     deassign/job ii_compatlib
$ else
$     define/job/nolog ii_compatlib "''ii_compatlib_def'"
$ endif
$ if "''ii_libqlib_def'".eqs."" 
$ then
$     deassign/job ii_libqlib
$ else
$     define/job/nolog ii_libqlib "''ii_libqlib_def'"
$ endif
$ if "''ii_framelib_def'".eqs."" 
$ then
$     deassign/job ii_framelib
$ else
$     define/job/nolog ii_framelib "''ii_framelib_def'"
$ endif
$
$ cleanup "''script_id'"
$
$ delete/sym/global upgrade_warning
$ delete/sym/global do_abort
$
$ set_message_on
$!
$ if f$search( "''tmpfile';*" .nes."" ) then -
	delete/noconfirm 'tmpfile;* 
$ if iisulocked  .and. f$search("ii_system:[ingres.files]config.lck;*").nes."" then -
	delete/noconfirm ii_system:[ingres.files]config.lck;*
$!
$ set on
$ exit 'status'
$!   
$ II_SELECT_LOG_SIZE: subroutine
$
$  node       = P1
$  log_kbytes = 'P2
$
$  log_node = f$edit( node, "lowercase" )
$!
$  if f$trnlnm( "log_size_prompted_''log_node'") .nes. "" then exit
$  if EXPRESS 
$  then
$     answer = "FALSE"
$  else
$     inquire/nopunc answer "Do you want to change transaction log size? (y/n) "
$     echo ""
$  endif
$  if( answer .eqs. "" ) then goto II_SELECT_LOG_SIZE
$  define/nolog "log_size_prompted_''log_node'" 1
$  if( .not. answer )
$  then
$     new_log_kbytes_'node == log_kbytes
$     ! If the answer is 'n', save the size anyway, since it could represent
$     ! a pre-existing transaction log
$     iisetres ii.'node.rcp.file.kbytes 'log_kbytes
$     EXIT
$  endif 
$
$  min_blocks = 65536
$  min_mb = (min_blocks /2) / 1024  !VMS block = 512 bytes
$
$ II_LOG_FILE_SIZE1:
$  log_blocks == 0
$
$  echo ""
$  echo "Please enter transaction log(s) size on ''node' in VMS blocks for this "
$  inquire blocks "installation.  The minimum logfile size is ''min_blocks' Blocks (''min_mb'MB). "
$  if (f$type(blocks).nes."INTEGER") then goto II_LOG_FILE_SIZE1
$  if 'blocks .lt. min_blocks then goto II_LOG_FILE_SIZE1
$  log_kbytes = blocks / 2
$  log_mb = log_kbytes / 1024
$  tempi = log_mb /8
$  if ((tempi * 8) .ne. log_mb)
$  then
$    log_mb = tempi * 8
$    log_kbytes = log_mb * 1024
$    blocks = log_kbytes * 2
$  endif
$  new_log_kbytes_'node == log_kbytes
$  log_blocks == blocks
$  echo ""
$  echo "The new transaction log size will be:"
$  echo "	''log_blocks' blocks (''log_mb'MB)"
$  inquire log_answ "Is this correct? (y/n) "
$  if (log_answ .eqs. "n") .or. (log_answ .eqs. "N") then goto II_LOG_FILE_SIZE1
$  iisetres ii.'node.rcp.file.kbytes 'log_kbytes
$endsubroutine
$  ! end of sub-routine

$Get_Not_Batch_Queue:
$
$ iigetres ii.'c_node.config.start_queue use_queue
$ use_queue = f$trnlnm( "use_queue" )
$ set_message_off
$ deassign "use_queue"
$ set_message_on
$ if (use_queue .nes. "")
$ then
$    iiqueue_'c_node' = use_queue
$    return
$ endif
$
$ tque = f$getqui("DISPLAY_QUEUE","GENERIC_TARGET",iiqueue,"BATCH")
$ if (clustered) .and. ("''tque'" .nes. "")
$ then
$    tmp = 0
$    answer = ""
$    QUEUE_FIND:
$       iiqueue = F$ELEMENT(tmp,",",tque)
$       if (("''iiqueue'".eqs."") .or. ("''iiqueue'".eqs.","))
$       then 
$           iiqueue = ""
$           goto QUEUE_PROMPT
$       endif
$       tnode = f$getqui("DISPLAY_QUEUE","SCSNODE_NAME",iiqueue,"BATCH")
$       if "''tnode'" .eqs. "''c_node'" then goto QUEUE_PROMPT
$       tmp = tmp + 1
$       goto QUEUE_FIND
$! 
$    QUEUE_PROMPT:
$    say

A command procedure (IISTARTUP) is created for the automated start up of the
installation at boot time.  Please specify the name of a batch execution queue
to submit the startup script.  NOTE:   The execution queue must not be a 
generic queue, and it must be located on the local node.

$    tque = iiqueue
$    if tque .eqs. "" then tque = "NO DEFAULT" 
$    if EXPRESS then goto SKIP_INQ_5
$    if c_node .eqs. node_name
$    then
$       inquire answer "Please enter the name of a local batch queue [''tque']"
$    else
$       inquire answer "Please enter the name of a local batch queue on ''c_node' [''tque']"
$    endif
$    if ("''answer'".eqs."") .and. ("''tque'".eqs."NO DEFAULT") -
     then goto QUEUE_PROMPT
$ SKIP_INQ_5:
$    if ("''answer'".eqs."") then answer = tque
$    if ("''answer'".eqs."NO DEFAULT" then answer = "SYS$BATCH"
$    iiqueue = answer
$ endif
$
$ iisetres ii.'c_node.config.start_queue "''iiqueue'"
$
$ iiqueue_'c_node = iiqueue
$
$ return

$Create_iistartup:
$
$ iiqueue = iiqueue_'c_node'
$ s_user  = ing_user_'c_node
$
$ open/write iistartup1 ii_system:[ingres]iistartup_'c_node.com
$ write iistartup1 "$!"
$ write iistartup1 "$! When this script is called interactively (probably by iistartup.com) "
$ write iistartup1 "$!     it submits itself for batch execution"
$ write iistartup1 "$! Otherwise if it is a batch execution"
$ write iistartup1 "$!     define logicals, symbols and then start the server processes"
$ write iistartup1 "$!"
$ write iistartup1 "$ if f$mode() .eqs. ""INTERACTIVE"" "
$ write iistartup1 "$ then"
$ write iistartup1 "$     logname = f$environment(""DEFAULT"")"
$ write iistartup1 "$     submit/notify/noprint/nodelete/log='logname -"
$ write iistartup1 "            /user=''s_user' /queue=''iiqueue' -"
$ write iistartup1 "            ii_system:[ingres]iistartup_''c_node'.com"
$ write iistartup1 "$ else"
$ write iistartup1 "$     set process/privileges=(SYSNAM,GRPNAM)"
$ if "''lnm_table'".eqs."LNM$SYSTEM"
$ then
$    write iistartup1 "$     define/system/exec/trans=concealed ii_system ""''ii_system_value'"" "
$ else
$    write iistartup1 "$     define/group/exec/trans=concealed ii_system ""''ii_system_value'"" "
$ endif
$ write iistartup1 "$     @ii_system:[ingres]ingsysdef.com"
$ write iistartup1 "$     ingstart"
$ write iistartup1 "$ endif"
$ close iistartup1
$ purge ii_system:[ingres]iistartup_'c_node.com
$
$ return

$Perform_setres_min_CMD: subroutine
$
$   pre_node  = P1
$   post_node = P2
$   min_value = P3
$
$   entry = 0
$
$CMD_LOOP:
$   c_node = f$element(entry, ",", nodes)
$
$   if (c_node .nes. ",")
$   then
$      iigetres 'pre_node''c_node''post_node' res
$      cur_val = f$trnlnm( "res" )
$      set_message_off
$      deassign "res"
$      set_message_on
$      if (cur_val .eqs. "")
$      then
$         set_val = 1
$      else
$         if ('cur_val .lt. 'min_value)
$         then
$            set_val = 1
$         else
$            set_val = 0
$         endif
$      endif
$
$      if (set_val .eq. 1)
$      then
$         iisetres 'pre_node''c_node''post_node' 'min_value
$      endif
$
$      entry = entry + 1
$
$      goto CMD_LOOP
$   endif
$
$endsubroutine

$get_cluster_details:
$
$ entry = 0
$
$CLUSTER_DETAILS:
$ c_node = f$element(entry, ",", nodes)
$
$ if (c_node .nes. ",")
$ then
$    iigetres ii.'c_node.*.*.vms_uic vms_uic
$    set_message_off
$    vms_uic = f$trnlnm( "vms_uic" ) - "[" - "]"
$    deassign "vms_uic"
$    set_message_on
$    if (vms_uic .eqs. "")
$    then
$       s_user = ing_user
$    else
$       s_user = f$element( 1, ",", vms_uic)
$       if (s_user .eqs. ",") then s_user = vms_uic
$    endif
$    ing_user_'c_node = s_user
$
$    entry = entry + 1
$
$    goto CLUSTER_DETAILS
$ endif
$
$ RETURN

$ Set_User_Priv:
$
$ iisetres ii.'c_node.privileges.user.'c_user -
     "SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED"
$ iisetres ii.'c_node.privileges.user.system -
     "SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED"
$ if ("''ing_user'" .nes. "''ivp_user'") then -
	iisetres ii.'c_node.privileges.user.'ivp_user - 
	"SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED"
$
$ return


$SET_UP_TX_LOG_FILES: subroutine
$
$ use_node = P1
$
$ log_path = log_file_1_'use_node
$ iigetres ii.'use_node'.rcp.log.log_file_name log_file_name
$ set_message_off
$ log_file_name = f$trnlnm( "log_file_name" )
$ deassign "log_file_name"
$ set_message_on
$
$ if ii_log_file_name_'use_node .nes. ""
$ THEN
$    if ii_log_file_name_'use_node .nes. log_file_name
$    then
$       ! If the old log file exists and it has an
$       ! old format name, must delete the file now
$       ! as there's no guarantee that the file name
$       ! can have the log part number added.
$
$       lg_file = ii_log_file_name_'use_node
$       lg_path = ii_log_file_'use_node
$
$       call Delete_File "''ii_log_file_old'[ingres.log]*.system*"
$       call Delete_File "''ii_log_file_old'[ingres.log]''lg_file'"
$       call Delete_File "''lg_path'[ingres.log]*.system*"
$       call Delete_File "''lg_path'[ingres.log]''lg_file'"
$    endif
$ endif
$
$ logfile = "''log_path'[ingres.log]''log_file_name'.l01"
$ if cluster_id .nes. ""
$ then
$    call Delete_Non_Cluster_Logs ii.'use_node.rcp.log.log_file_ 'log_file_name'
$
$    logfile = logfile + "_" + use_node
$    node_param = "-node ''use_node'"
$ else
$    node_param = ""
$ endif
$ delete_bkup_log = 0
$ set_message_off
$ if( f$search( "''logfile'" ) .nes. "" )
$ then 
$       set_message_on
$       echo "A primary transaction log already exists for ''use_node'"
$       if( "''p1'" .eqs. "-NOERASE" .or. "''p1'" .eqs. "-noerase" )
$       then
$          echo ""
$          echo "The primary transaction log will not be erased, at your request."
$          echo ""
$          goto II_CREATE_BACKUP_LOG
$       else
$          iigetres ii.'use_node'.rcp.log.log_file_parts log_file_parts
$          log_file_parts = f$trnlnm( "log_file_parts")
$          set_message_off
$          deassign "log_file_parts"
$          set_message_on
$          log_blocks == f$file_attributes( "''logfile'", "ALQ" ) * log_file_parts
$          log_kbytes = log_blocks / 2
$	   log_mb = log_kbytes / 1024
$ 	   echo ""
$          if (cluster_id .nes. "")
$          then
$             echo "The existing transaction log size on ''use_node' is:"
$          else
$             echo "The existing transaction log size is:"
$          endif
$          echo "	''log_blocks' blocks (''log_mb'MB) "
$          echo " "
$          if .not. upgrade_warning
$          then
$             echo "Please note, if you change the transaction log size, the existing log"
$             echo "file(s) will be deleted before the new one(s) are created."
$          endif
$          call II_SELECT_LOG_SIZE "''use_node'" 'log_kbytes
$          if (do_abort) then exit
$          if (new_log_kbytes_'use_node .EQ. log_kbytes)
$          then 
$		 if( f$search( "''ii_database'[ingres.data]iidbdb.dir" ) .nes. "" )
$		 then
$                   if .not. upgrade_warning
$                   then
$                      upgrade_warning == 1
$		       say

Because you are upgrading your installation, the transaction log records 
will be erased and the log file(s) will be re-formatted for this version.

$                   endif
$		 endif
$                on error then goto IIMKLOG_FAILED
$                iimklog -erase 'node_param
$                set noon
$                goto II_CREATE_BACKUP_LOG
$          else
$                delete_bkup_log = 1
$                log_kbytes = new_log_kbytes_'use_node 
$                delfile = logfile + ";*"
$                DELETE 'delfile
$                echo ""
$                echo "The existing primary transaction log is deleted"
$                echo ""
$                iisetres ii.'use_node'.rcp.file.kbytes 'log_kbytes
$          endif
$       endif
$ endif
$
$ set_message_on
$ echo "The new Ingres transaction log will now be created."
$
$ on error then goto IIMKLOG_FAILED
$ iimklog 'node_param
$ set noon
$!
$! Create or erase the backup primary transaction log.
$!
$ II_CREATE_BACKUP_LOG:
$ iigetres ii.'use_node'.rcp.log.dual_log_1 log_path
$ set_message_off
$ log_path = f$trnlnm( "log_path" )
$ deassign "log_path"
$ set_message_on
$ if( log_path .eqs. "" ) then goto RCPCONFIG
$ iigetres ii.'use_node'.rcp.log.dual_log_name log_file_name
$ set_message_off
$ log_file_name = f$trnlnm( "log_file_name" )
$ deassign "log_file_name"
$ set_message_on
$
$ if ii_dual_log_name_'use_node .nes. ""
$ THEN
$    if ii_dual_log_name_'use_node .nes. log_file_name
$    then
$       ! If the old log file exists and it has an
$       ! old format name, must delete the file now
$       ! as there's no guarantee that the file name
$       ! can have the log part number added.
$
$       lg_file = ii_dual_log_name_'use_node
$       lg_path = ii_dual_log_'use_node
$
$       call Delete_File "''ii_dual_log_old'[ingres.log]*.system*"
$       call Delete_File "''ii_dual_log_old'[ingres.log]''lg_file'"
$       call Delete_File "''lg_path'[ingres.log]*.system*"
$       call Delete_File "''lg_path'[ingres.log]''lg_file'""
$    endif
$ endif
$
$ logfile = "''log_path'[ingres.log]''log_file_name'.l01"
$ if cluster_id .nes. ""
$ then
$    call Delete_Non_Cluster_Logs ii.'use_node.rcp.log.dual_log_ 'log_file_name'
$
$    logfile = logfile + "_" + use_node
$ endif
$ set_message_off
$ if( f$search( "''logfile'" ) .nes. "" )
$ then 
$    set_message_on
$    echo "A backup transaction log already exists for ''use_node'"
$    if( "''p1'" .eqs. "-NOERASE" .or. "''p1'" .eqs. "-noerase" )
$    then
$       echo ""
$       echo "The backup transaction log will not be erased, at your request."
$       echo ""
$       goto RCPCONFIG
$    else
$          if ( .NOT. delete_bkup_log)
$          then 
$                on error then goto IIMKLOG_FAILED
$	         iimklog -erase -dual 'node_param
$                set noon
$                goto RCPCONFIG
$          else 
$                delfile = logfile + ";*"
$                DELETE 'delfile
$                echo ""
$                echo "The existing backup transaction log is deleted"
$                echo ""
$          endif
$    endif
$ endif
$
$ set_message_on
$ echo "The new backup transaction log will now be created."
$ on error then goto IIMKLOG_FAILED
$ iimklog -dual 'node_param
$ set noon
$ goto RCPCONFIG
$!
$ IIMKLOG_FAILED:
$!
$ say
Correct the problem described above and re-run this program.

$!
$ do_abort == 1
$ exit
$!
$ RCPCONFIG:
$!
$ echo "Formatting transaction log file(s) on ''use_node'..."
$ echo ""
$ on error then goto RCPCONFIG_FAILED
$ define /user ii_dbms_config ii_system:[ingres.files]
$ define /user sys$output 'tmpfile
$ rcpconfig -init 'node_param
$ set noon
$ delete/noconfirm 'tmpfile;*
$ exit
$!
$ RCPCONFIG_FAILED:
$!
$ set noon
$ type 'tmpfile 
$ delete/noconfirm 'tmpfile;* 
$ say

Unable to format the transaction log file.  You must correct the problem
and re-run this set up procedure.

$ do_abort == 1
$
$ exit
$
$endsubroutine

$Delete_File: subroutine
$
$   del_file = P1
$
$   if f$search( "''del_file'") .nes. ""
$   then
$      delete/nolog 'del_file';*
$   endif
$
$endsubroutine

$Delete_Non_Cluster_Logs: subroutine
$
$ loc_string = P1
$ log_name   = P2
$
$ loc_no = 1
$
$DELETE_LOOP:
$ iigetres 'loc_string''loc_no' log_area
$
$ if f$trnlnm( "log_area" ) .nes. ""
$ then
$    log_file = f$fao( "log_area:[ingres.log]!AS.l!2ZL", log_name, loc_no)
$
$    if f$search( "''log_file'") .nes. "" then delete 'log_file';
$
$    loc_no = loc_no + 1
$
$    deassign "log_area"
$
$    goto DELETE_LOOP
$ endif
$
$ endsubroutine
