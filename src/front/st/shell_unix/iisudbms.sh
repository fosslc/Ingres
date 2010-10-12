:
#
# Copyright (c) 1992, 2010 Ingres Corporation
#
#
#
#  Name: iisudbms -- setup script for Ingres Intelligent DBMS
#
#  Usage:
#	iisudbms
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility.  It sets up the Ingres Intelligent Database.
#
#  Exit Status:
#	0       setup procedure completed.
#	1       setup procedure did not complete.
#
#  DEST = utility
## History:
##	18-jan-93 (tyler)
##		Created.	
##	16-feb-93 (tyler)
##		Vastly improved.
##	18-feb-93 (tyler)
##		Added check to verify that /dev/kmem is readable before
##		attempting setup.  Fixed bug #49640.
##      15-mar-93 (tyler)
##		Changed if/then and while loop coding style.  Added error
##		checking for symbol.tbl and config.dat creation.  Added
##		optional installation code argument to batch mode. 
##		Removed unnecessary prompts previously displayed during
##		upgrades.
##	17-mar-93 (tyler)
##		Added code which validates installation code specified 
##		on the command line in batch mode.  Added check for syntax
##		errors in configuration files.
##	19-mar-93 (dianeh)
##		Added missing OR to symbol.tbl creation.
##	22-mar-93 (tyler)
##		Beefed up warning to user not to set II_DATABASE to point
##		to the II_SYSTEM device and fixed preceding fix by dianeh
##		which would cause setup to fail whenever symbol.tbl didn't
##		exist on entry, even if touch succeeded. 
##	21-jun-93 (tyler)
##		Added "Press RETURN to continue:" prompt after errors.
##		Removed defunct logging system setup code and createdb
##		bug workaround.  Added code which handles upgrades of
##		installations containing raw transaction logs.  Added
##		confirmation after the character set prompt.  Replaced
##		in-line scripts invocations with calls to function calls.
##		An interrupts now causes the program to exit with an error
##		status. 
##	25-jun-93 (tyler)
##		Removed definition of II_TEMPLATE which is now defunct.
##		Increased the size of the transaction log created during 
##		batch setup to 33554432 bytes to accomodate regression
##		testing.
##	23-jul-93 (tyler)
##		Removed dead code.  Removed -installation flag from
##		iimaninf call.
##	26-jul-93 (tyler)
##		Set PM resources required to give root and owner permission
##		to start the server.  Added call to remove_temp_resources()
##		before final (completion) exits to strip temporary resources
##		from config.dat.  Added call to iisulock to lock config.dat
##		during setup.
##	04-aug-93 (tyler)
##		Removed unnecessary message.  Replaced calls to pause()
##		before failure exits by calls to pause_message().
##	05-aug-93 (tyler)
##		Fixed message which incorrectly displayed the number
##		of concurrent users configured by previous invocation.
##	20-sep-93 (tyler)
##		Make sure II_CONFIG is set before it is referenced.
##		Replaced some pause_message() calls with calls to pause().
##		Allow II_INSTALLATION and II_TIMEZONE_NAME to be defined
##		in Unix environment before setup.  Changed default
##		II_TIMEZONE_NAME value to NA-PACIFIC.  Set local_vnode
##		resource explicitly.  Allow user to change default
##		transaction log size.  Stop creating the now defunct
##		users file.
##	19-oct-93 (tyler)
##		Set default values for II_DUAL_LOG and II_DUAL_LOG_NAME.
##		Run "ckpdb +j" after creating and upgrading the iidbdb.
##		Added prompt whether to make the iidbdb FIPS compliant.
##		Removed unnecessary call to syscheck.  Don't remove
##		iilink script.  Allow user to abort setup procedure
##		when syscheck fails.  Removed "Defining symbols..."
##		message.  Removed reference to the "Running INGRES"
##		chapter of the Installation Guide.  Removed redundant
##		message which reported syntax error in config.dat or
##		protect.dat.  Don't get II_TIMEZONE_NAME from the
##		environment because it ingbuild sets it to IIGMT.
##		Stopped clearing the screen at the beginning of the
##		script.  Added default responses to (y/n) questions.
##		Removed calls to pause_message().  Handle existing
##		raw transaction logs correctly.  Explicitly test
##		for character special transaction log files, since -f
##		doesn't evaluate to true (for special character files)
##		on all platforms.  Prompt for concurrent users last.
##	22-nov-93 (tyler)
##		Beefed up transaction log help message.  Added trap
##		to remove config.lck on exit and removed multiple
##		calls which removed it explicitly (at the suggestion
##		of daveb).  Improved code which removes defunct
##		files to require only one system call .  Added
##		dual log configuration support.  Replaced several
##		system calls with calls to shell procedures.  Replaced
##		iimaninf call with ingbuild call.
##	29-nov-93 (tyler)
##		iiechonn reinstated.
##	30-nov-93 (tyler)
##		ingbuild must be invoked by its full path name.
##	16-dec-93 (tyler)
##		Use CONFIG_HOST instead of HOSTNAME to compose
##		configuration resource names.  Consolidate privilege
##		resources using PM wildcard.
##	05-jan-94 (tyler)
##		Fixed incorrect documentation reference.
##	31-jan-94 (tyler)
##		Updated privilege string syntax.	
##	4-Feb-1994 (fredv)
##		As the result of a recent change in rcpconfig, rcpconfig -init
##		now requires the shared memory to be re-initialized each time
##		it is invoked. This change is to put in a temporary workaround 
##		so that we can proceed to complete the installation with this
##		integration and move forward to testing. Without this
##		workaround, installation with dual logging will fail.
##		This change can be removed when the long term solution comes
##		through.
##	22-feb-94 (tyler)
##		Fixed BUG 59367: corrected syntax error in assignment of
##		"oldfiles" variable used to remove defunct files. 
##	31-mar-94 (tomm)
##		Remove remains of /dev/kmem checks from iisudbms.  syscheck
##		now does this right thing with unreadable /dev/kmem's so
##		we don't need the extra check in here.
##      08-Apr-94 (michael)
##              Changed references of II_AUTHORIZATION to II_AUTH_STRING where
##              appropriate.
##	8-Nov-94 (stephenb)
##		when running upgradedb to upgrade from 6.4 we must re-set
##		II_LG_MEMSIZE, since the 6.4 value may be too small, set it
##		for the upgrade and then remove it. Exit 1 when upgradedb
##		fails to prevent future problems (60345).
##      08-Nov-94 (forky01)
##              Fixed BUG 63476: Rename ii.$CONFIG_HOST.prefs.iso_entry_sql-92
##              to ii.$CONFIG_HOST.fixed_prefs.iso_entry_sql-92 so that 
##              SQL-92 mode cannot be switched once set.  CBF reads all
##              preference variables by scanning for ".prefs.".  This
##              change would keep this variable from showing up in the
##              CBF preferences menu.
##	01-Mar-95 (hanch04)
##		added set II_GCNxx_LCL_VNODE to hostname
##      09-Mar-95 (tutto01)
##              Clarified message concerning backup transcation log.
##      02-Mar-1995 (canor01)
##              Don't delete ingchkdate.
##      09-Mar-1995 (canor01)
##              If there is a default star server and user specifies
##              upgrade all databases, disable star server while 
##              upgradedb is run on iidbdb, then restart it for
##              running upgradedb on all databases.  Otherwise, star
##              complains about non-upgraded iidbdb. (67384)
##      20-Mar-95 (tutto01) BUG 67540
##              Changed text of installation message.               
##      20-Mar-1995 (canor01)
##              Remove existing transaction log files on upgrade. (67499)
##              Clean up shared memory before creating log files. (67637)
##      03-Apr-1995 (canor01)
##              Traps cannot be canceled in a function in AIX 4.1, so 
##              add explicit "trap 0" before clean_exit.
## 	30-May-1995 (johna)
##		Added call to get_num_processors and set up 
##		II_NUM_OF_PROCESSORS and II_MAX_SEM_LOOPS accordingly
##      15-jun-95 (popri01)
##              nc4_us5 complained about VERSION sed string. Escape the '/'.
##	22-nov-95 (hanch04)
##		add log of install
##	08-dev-95 (hanch04)
##		do not pause on error in batch mode
##	08-jan-1996 (nick)
##		Convert vestigal hostname from a 6.4 installation to
##		PM format. #73222
## 	17-Oct-1995 (allst01)
##		Revised creation of iidbdb for B1/ES
## 	08-Feb-1996 (smiba01)
##		Above change caused installation problems on non-B1 platforms
##		Removed!
##	08-Oct-1996 (hanch04)
##		Added iisetres for rcp log buffer_count.  When upgrading,
##		parameter did not increase to 20.
##	28-jan-1997 (hanch04)
##		Moved order of ingstart and upgradedb for star.
##	11-Nov-1997 (fucch01)
##		Changed 'ckpdb +j iidbdb' to 'ckpdb +j iidbdb -l +w' so
##		ckpdb will wait for exclusive hold on iidbdb.  ts2_us5
##		was experiencing E_DM110D_CPP_DB_BUSY db lock conflict
##		when attempting initial checkpoint of iidbdb during ingbuild
##      11-nov-1997 (canor01)
##		Changed KBYTES for BYTES for log file > 2 gig
##	09-jun-1998 (hanch04)
##		We don't use II_AUTH_STRING for Ingres II
##	12-feb-1998 (kinpa04)
##		Had to add the +w(wait) parameter to checkpointing
##		the catalog because of error: E_DM110D_CPP_DB_BUSY
##	25-Aug-1998 (hanch04)
##		When upgrading, remove II_LOG_FILE and enter config.dat.
##	26-aug-1998 (hanch04)
##		Added setup for rmcmd and imadb
##	08-oct-1998 (hanch04)
##		Add missing bracket.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##	30-nov-98 (toumi01)
##		Make rmcmd and vdba setup conditional on the product
##		being present as part of the install (it is not in the
##		free Linux version, for example).
##		Use the late, great OEMSTRING capability to set up
##		II_AUTH_STRING for the free Linux version, which uses
##		that facility to expire the product but does not do any
##		other license checking of any kind.
##	15-jan-98 (toumi01)
##		Correct check for rmcmdgen to $II_SYSTEM/ingres/utility
##		and amend previous history to take credit [sic] for the
##		change that contained the error.
##	25-mar-99 (toumi01)
##		Supply full path for createdb command so that we don't
##		accidentally invoke a foreign createdb program (e.g. the
##		postgres program on Linux).
##	16-Aug-1999 (hanch04)
##		upgradedb will be run for express installations.
##	19-Aug-99 (bonro01)
##		Ingres 6.4 uses only II_LOG_FILE so II_LOG_FILE_NAME must
##		default to "ingres_log" explicitly to allow upgrade to
##		succeed.
##      29-oct-1999 (hanch04)
##              Use correct KBYTES durring an upgrade install.
##		Make sure the DUAL log get created on initial install.
##	27-apr-2000 (hanch04)
##		Moved trap out of do_iisudbms so that the correct return code
##		would be passed back to inbuild.
##      14-Feb-2000 (linke01)
##              during Ingres installation,after Initializing imadb for vdba, 
##              Ingres Intelligent DBMS setup complete, sometimes Ingres was
##              not shutdown well and installation hangs. Force Ingres
##              Ingres shutdown completely. Also, $LOG_BYTES should have 
##              double quote.
##      31-aug-2000 (hanch04)
##          If this is an embeded install use the tng.rfm
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based
##              installer on Linux.
##	06-Feb-2001 (hanje04)
##		As part of fix for SIR 103876 iilink must be run during
##		installation to allow checkpoints etc. to be run by non
##		Ingres users. If USE_SHARED_SVR_LIBS=true in iisysdep
##		iilink -noudts will be run after dbms setup has completed.
##	22-mar-2001 (somsa01)
##		Now that the RMCMD tables live in imadb, we must make sure
##		that it is created BEFORE running 'rmcmdgen'.
##	15-May-2001 (hanje04)
##		II 2.6 requires stack size of 128K, this must be changed 
##		for the dbms, recovery and star servers when upgrading.
##	24-May-2001 (hanje04)
##		When upgrading from II 2.5 to II 2.6 VDBA catalogs must be 
##		removed from iidbdb.
##	06-Jun-2001 (hanje04)
##	 	1) When upgrading from II 2.5 to II 2.6 we should NOT try
##		to remove VDBA catalogs from imadb
##		2) To stop the icesvr failing on startup during upgrade,
##		set startup count to 0 and upgrade ICE databases after imadb.
##	15-Jun-2001 (hanje04)
##		Correct typos from upgrade change 450435.
##	05-Jul-2001 (hanje04)
##		Upgrade COMMAND3 doesn't get set if startup count of ICESVR
##		is 0. Change condition so that it is.
##      20-jul-2001 (devjo01)
##              Add setup of machine nicknames.
##	20-Aug-2001 (toumi01)
##		FIXME - change dbms stack to 256K for i64_aix - dbms.crs
##		value ignored ???  but backed this out for now as it is
##		not platform-specific and i64_aix is mothballed
##		< iisetres "ii.$CONFIG_HOST.dbms.*.stack_size" 131072
##		> iisetres "ii.$CONFIG_HOST.dbms.*.stack_size" 262144
##	17-Sep-2001 (somsa01)
##		When upgrading from II 2.0 to II 2.6 we should NOT try to
##		remove VDBA catalogs from imadb.
##      20-Sep-2001 (gupsh01)
##              Added new flag -mkresponse and -exresponse.
##      20-Sep-2001 (gupsh01)
##              Corrected error with correctly referencing ENV_VAR, and 
##		incorrect setting of WRITE_RESPONSE.
##	25-Sep-2001 (hanje04)
##	 	During upgrade, if is ICE installed, set ICESVR startup count to
##		0 before starting installation and back to orig value (which 
##		could also be 0) after ICE databases have been upgraded.
##	19-Oct-2001 (gupsh01)
##		Added code for supporting -exresponse flag. Indented propery for
##		code readability.
##	28-Oct-2001 (devjo01)
##		- Hand merged all the cluster stuff.
##		- Finished indentation rationalization.
##		- Backup symbol.tbl & config.dat to simplify guarding against 
##		  unwanted updates during mkresponse.
##		- Added local clean_up routine to restore backed up files.
##		- Removed unused/redundant IGNORE_RESPONSE, NOTBATCH.
##		- Added usage message.
##		- Allowed for <default> tag to be written to response files,
##		  for the cases where we need to distinguish between
##		  a specific value, and a "take local default" specification.
##		- Move iilink into a "real set up only" section past the
##		  "do you want to perform set up" prompts.
##	30-jan-2002 (devjo01)
##		Allow user to respecify log file size just in case user
##		cannot specify a location on a device with sufficient space,
##		and needs to reduce his log size request.
##	04-Apr-2002 (somsa01)
##		Set "ii.*.setup.add_on64_enable" to ON if we install the
##		64-bit add-on package.
##	21-May-2002 (hanje04)
##		Contditionalize setting stack_size for dbms to 128K for 
##	        upgrades. If it's alreadly larger than this, leave it alone.
##      18-Jul-2002 (hanch04)
##              Changed Installation Guide to Getting Started Guide.
##      04-Sep-2002 (hweho01)
##              During the upgrade,  if the primary transaction log exists,   
##              preserve the value in rcp.file.kbytes. Star 12116613.  
##	20-Sep-2002 (devjo01)
##		Put in NUMA cluster configuration questions.
##		As part of this:
##		- Placed some major functional subsections into separate
##		  functions for clarity, and to allow iterative execution.
##		- Moved "installation running" check much further back
##		  in the processing.
##		- Created a number of utility routines for commomly
##		  performed actions.
##		- Added "-test" option to allow dry-runs of this
##		  script to speed debugging/QA.
##	10-Apr-2003 (hanje04)
##		Only invoke iilink for shared library server if C compiler
##	        is installed.
##	21-Jul-2003 (devjo01)
##		Make sure rmcmdrmv is always called before rmcmdgen
##		if imadb is not newly created. (b110593).
##	12-Sep-2003 (hanje04)
##		Ammend fix (hweho01) to preserve size of transaction log
##		during upgrade. [ "$LOG_BYTES" -gt 0 ] errors on Linux
##		because "$LOG_BYTES" is a string and -gt expects an integer.
##	21-May-2003 (bonro01)
##		Increased server stack size for HP Itanium (i64_hpu)
##		Made stack size a variable so that it can be different for
##		each platform.  The DBMS_STACKSIZE variable is defined in
##		Minglocs.mk
##	15-Oct-2003 (hanch04)
##		SP1 installed the files/zoneinfo with the wrong permissions.
##		chmod them if they exist.
##	4-Dec-2003 (schka24)
##		Take a stab at better handling of new/changed parameters.
##		Avoid iigenres if it looks like some kind of valid DBMS
##		configuration exists.
##	6-Jan-2004 (schka24)
##		It appears that some old cluster stuff that finally got
##		integrated uses "grep -q", which is only supported by
##		xpg4 grep on Solaris -- use /dev/null redirect instead.
##	20-Jan-2004 (bonro01)
##		Remove ingbuild call for PIF installations.
##    30-Jan-2004 (hanje04)
##        Changes for SIR 111617 changed the default behavior of ingprenv
##        such that it now returns an error if the symbol table doesn't
##        exists. Unfortunately most of the iisu setup scripts depend on
##        it failing silently if there isn't a symbol table.
##        Added -f flag to revert to old behavior.
##      21-Jan-2004 (hanje04)
##              SIR 110765
##		Define -rpm flag so that when packaged using RPM the
##		version is detected using RPM and put parameter checking
##		in while loop so that -rpm (and others) can be used with
##		-exresponse.
##		Also, in batch mode DUAL_LOG should be disabled by default
##	03-feb-2004 (somsa01)
##		Removed symbol.tbl change for now.
##	8-Mar-2004 (schka24)
##		Init value for dmf_tcb_limit if upgrading.
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##	14-mar-2004 (devjo01)
##		Validate nickname, rename CLUSTER_ID to II_CLUSTER_ID.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	24-Mar-2004 (hanje04)
##	    Removed quotes from around variable in "$x" -ge 2048 test as 
##	    it requires 2 integers and "$x" is a string.
##	23-Mar-2004 (hanje04)
##	    Quote ("") $GETRES when checking for dual_log in readresponse mode
##	    otherwise [ -n $GETRES ] is always true.
##	24-Mar-2004 (hanje04)
##	    Removed quotes from around variable in "$x" -ge 2048 test as 
##	    it requires 2 integers and "$x" is a string.
##	27-mar-2004 (devjo01)
##	    Do not setup "admin" subdirectories for UNIX.
##	02-Apr-2004 (hanje04)
##	    When checking is default_page size is > 2048, initialise
##	    x to 0 it's not set by iigetres. This stops $x -ge 2048 complaining
##	    when $x resolved to nothing.
##	04-Apr-2004 (hanje04)
##	    If II_EMBED_INSTALL is set to ON, make sure II_INSTALLATION only
##	    defaults to WV. Don't let it override user specified values
##	12-apr-2004 (devjo01)
##	    Move get_node_number, get_nickname, create_cluster_dirs to
##	    iishlib.sh so they can be available to iisunode.sh.
##	30-apr-2004 (sheco02)
##	    Allow user set the number of concurrent users even if syscheck fails
##	    with return code 1(failed to open /dev/kmem file). 
##	17-May-2004 (hanje04)
##	    Changed default value for II_NUM_PROCESSORS to 2 as it doesn't
##	    hurt uni-processor machines and it does benefit SMP machines.
##	04-Jun-2004 (hanje04)
##	    Added temporary hack to setup mdb database for EMBEDDED installs.
##	21-Jun-2004 (gupsh01)
##	    Added mdbimaadt.sql for mdb version 005 update.
##	23-Jun-2004 (hanje04)
##	    Make mdb setup conditional on II_MDB_INSTALL being set. 
##	    Make it possible to change the name of MDB by setting II_MDB_DBNAME
##	    Change default installation ID for embedded installs to be EI
##	    Checkpoint MDB after initialization.
##	17-Jul-2004 (hanje04)
##	    Move creation and setup of MDB to loadmdb script which is now
##	    in $II_SYSTEM/ingres/files/mdb/mdb.tar.gz with all the mdb data
##	    and load scripts.
##	26-Jul-2004 (hanje04)
##	    Increase default log size for II_MDB_INSTALL=TRUE to 128Mb
##	31-Aug-2004 (hanje04)
##	    BUG 112963 - Iss# 1368605
##	    Increase GCN bedcheck interval and socket timeout so that it 
##	    doesn't occur during mdb load. 
##	    Temporary workaoround, to be removed.
##	1-Sep-2004 (schka24)
##	    Various fixes for upgrade installations.  Make sure that
##	    upgrading from 6.4 works.  Revise all the log file stuff
##	    such that if the current log config is in config.dat, leave
##	    it alone.  Cluster install restart still probably needs
##	    some work, haven't abused it enough yet.
##	21-Sep-2004 (hanje04)
##	    SIR 113101
##	    Remove all trace of EMBEDDED or MDB setup. Moving to separate 
##	    package.
##	21-Sep-2004 (hanje04)
##	    Remove increase in bedcheck interval. BUG 112963 has been 
##	    properly addressed.
##	    Use iipmhost to set SERVER_HOST not II_GCNxx_LOCAL_VNODE, as 
##	    CONFIG_HOST is always lower case but II_GCNxx_LOCAL_VNODE isn't.
##	25-Sep-2004 (schka24)
##	    Fix typos in above: iisetres should be iigetres, LOG_HOST should
##	    be LOGHOST.  Fix lost reading of ice server count pre-upgrade.
##	    Fix 1.2 updating, genres doesn't reset some existing params
##	    and some are too small (log buffers, stack size).
##	    Turn off Bridge server around initial upgrades.
##	06-Oct-2004 (hanch04/hanje04)
##	    Turn off all non essential servers for upgrade.
##	    Remove "" from -ne operations as Linux doesn't like strings in 
##	    integer calls.
##	20-Oct-2004 (schka24)
##	    Fix typo in above (JDBC_SERVERS -> SERVER).
##	    Make sure we have a RMCMD_SERVERS setting, might not be
##	    set if restarting setup.  Restore rmcmd startup count for
##	    install, not just upgrade.
##      04-Nov-2004 (bonro01)
##          Set the SETUID bits in the install scripts because the pax
##          program does not set the bits when run as a normal user.
##	09-Nov-2004 (bonro01)
##	    Write II_INSTALLATION to symbol.tbl during batch install.
##	23-Nov-2004 (hanje04)
##	    Look for II_CHARSET in response file not II_CHARSET$II_INSTALLATION
##	15-Nov-2004 (hanje04)
##	    On upgrade, if ii.$CONFIG_HOST.dbms.*.opf_stats_nostats_factor
##	    is set, save it off and set it again by hand. iigenres is setting
##	    it to '0' if it's set to 1.
##	    The reasons for this are unclear, so setting it by hand for now.
##	    FIXME!!!
##      22-Dec-2004 (nansa02)
##          Changed all trap 0 to trap : 0 for bug (113670).
##
##	10-Jan-2005 (hanje04)
##	    BUG 113719
##	    Move upgrade code which saveset off communication server startup
##	    counts to new function quite_comsvrs() and code that does the 
##	    inverse to restore_comsvrs().
##	    Use these new functions to the same effect in regular installation
##	    mode to stop said com servers starting up during the initial
##	    install of iidbms.
##	    Also add 1 second sleep before sysmod and ckpdb to stop
##	    failure of either due to lock release race contition
##	11-Jan-2005 (schka24)
##	    Don't do update-param stuff if installation is already at r3.
##	27-jan-2005 (devjo01)
##	    Move grep_search_pattern from here to iishlib (its useful!)
##	8-Feb-2005 (schka24)
##	    Caseless compare of server-host and config-host.  With r3,
##	    iipmhost always (?) returns lowercase.
##	11-Feb-2005 (bonro01)
##	    Move quiet_comsvrs() and restore_comsvrs() to iishlib
##      14-Feb-2005 (stial01)
##          Check for new r3 param system_lock_level
##	28-Feb-2005 (bonro01)
##	    Issue a warning when upgrading from an installation that
##	    had Enhanced Security installed.
##	10-Mar-2005 (hweho01)
##          In a 2.5 to r3 upgrade, imadb is left unupgraded if Star  
##          server wasn't installed in the previous release and all  
##          databases are selected to upgrade. 
##          To fix the error, need to always call upgradedb with iidbdb    
##          as the parameter before upgrade all the user database.  
##      07-Apr-2005 (hanje04)
##          Bug 114248
##          When asking users to re-run scripts it's a good idea to tell
##          them which script needs re-running.
##	07-Apr-2005 (bonro01)
##	    The response file should contain II_CHARSET not II_CHARSETxx
##	    Allow II_CHARSET and II_TIMEZONE_NAME to be set in environment.
##	    Change II_TIMEZONE_NAME default back to NA-PACIFIC as documented.
##	07-Jul-2005 (bonro01)
##	    Fix detection method for old Enhanced Security.
##	17-Aug-2005 (bonro01)
##	    Add IMP application to installation
##	28-Nov-2005 (kschendel)
##	    Add result_structure and di_zero_bufsize to upgrade.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	23-Jan-2006 (kschendel)
##	    Upgrade audit-mechanism to "INGRES" if it was "CA".
##	15-Mar-2006
##	    Remove IMP application
##	26-Jul-2006 (bonro01)
##	    Explicitly link 64bit iimerge for II_LP64_ENABLED Hybrid platforms
##	    to eliminate iilink prompt for iimerge type.
##	18-Sep-2006 (gupsh01)
##	    Added get_date_type_alias() to configure whether the alias 
##	    datatype date refers to ingresdate or ansidate.
##	27-Sep-2006 (gupsh01)
##	    Fixed setting of date_type_alias parameter in config file.
##	11-Nov-2006 (gupsh01)
##	    Fixed reading date_type_alias parameter from response file.
##	12-Dec-2006 (gupsh01)
##		Modify the default for II_DATE_TYPE_ALIAS to ingresdate.
##	18-Dec-2006 (hanje04)
##	    BUG 117353
##	    SD 113656
##	    Make script aware of II_LOG_FILE_SIZE_MB which is new response
##	    file variable.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##       7-Feb-2006 (hanal04) Bug 116943
##          Correct the handling of -mkresponse filename.rsp. After a
##          shift $1 is the filename not $2.
##	26-Feb-2007 (hanje04)
##	    SIR 117784
##	    Create and populate demodb database
##      12-Mar-2007 (hanal04) Bug 117893
##          LOG location details skipped when run with -mkresponse.
##	14-Mar-2007 (hweho01)
##	    Modify the test operator for demodb handling, because "==" 
##          is invalid on Unix platforms. Fix the codes to accept user        
##          selection whether demodb should be created.         
##	22-Mar-2007 (hweho01)
##	    Correct the spelling error in the demodb prompt.  
##	16-Mar-2007 (bonro01)
##	    Don't call restore_comsvrs() when writing a response file.
##	04-Apr-2007 (hanje04)
##	    BUG 118028
##	    Make script aware of new response file parameters for SQL 92
##	    support.
##	25-Apr-2007 (hanje04)
##	    SIR 118202
##	    Increase default Tx log size to 256Mb
##	21-Jun-2007 (hanje04)
##	    SIR 118202
##	    Missed increasing default Tx log size for -mkresponse option.
##	    Increase to 256Mb.
##      22-Jun-2007 (hanal04) Bug 118548
##          Connect limit skipped when run with -mkresponse.
##	10-Jul-2007 (kibro01) b118702
##	    Move date_type_alias to config section
##	17-Apr-2007 (hanje04) 
##	    Bug 117044
##	    Skip "update_parameters()" stuff on int_rpl (rPath) as it causes
##	    problems when we pre-load config.
##	18-Sep-2007 (hweho01)
##	    Need to export II_CHARSETxx, so createdb can pick up the setting. 
##	17-Oct-2007 (hweho01)
##	    Amend the above change, if II_CHARSETxx is set to null, unset it. 
##	    Also added message to log the II_CHARSETxx setting during batch 
##	    installtion. 
##	26-Oct-2007 (kschendel) SIR 122757
##	    direct_io is now an installation (config) parameter; translate
##	    dbms.*.direct_io to the new config.direct_io upon updating.
##	20-Mar-2008 (bonor01/abbjo03)
##	    Discontinue use of cluster nicknames.
##	11-Apr-2008 (hweho01)
##	    Remove the chmod call for directories in files/zoneinfo. The
##	    correct permissions will be set by iizic now, no matter whether
##	    iisudbms is run or not.
##	30-Sep-2008 (bonro01)
##	    Upgrade fails for demodb because of error creating a primary key
##	    contraint on a non-journaled table in a journaled database.
##	    Turn off db journaling before reloading demodb.
##	20-Jun-2009 (kschendel) SIR 122138
##	    Use DBMS_UDT symbol which is transformed into either -noudt
##	    or site / build specific OME library string.
##	23-Jun-2009 (hanje04)
##	    Bug 122227
##	    SD 137277
##	    Check for II_UPGRADE_USER_DB in response file when running in
##	    batch mode as it's used by the GUI installer during upgrade.
##	09-Jul-2009 (hanje04)
##	    SIR 122309
##	    Add II_CONFIG_TYPE as response file parameter to allow one
##	    on of 3 configuration types to be applied after the rest of
##	    the setup is complete. New configuration types are:
##		Transactional - TXN
##		Business Intelligence - BI
##	        Content Management - CM
##	    II_CONFIG_TYPE can also be set to "TRAD" which just leaves
##	    the defaults as they are. Actually configuration is applied 
##	    using a separate script.
##	29-Jul-2009 (rajus01) Issue:137354, Bug:122277
##          During upgrade gcn.local_vnode gets modified. The setup scripts
##          should not set/modify this value. My research indicates that
##          the Ingres setup scripts started to set this parameter as per the
##          change 405090 (21-Sep-1993). Later came along the change 420349
##          (bug 70999) in Sep 1995 which defined this parameter for Unix
##          platforms in crs grammar (all.crs). So, the Unix scripts during
##          fresh install or upgrade should not set this parameter 
##          explicitly. My change removes the definition from the setup 
##	    scripts.
##	7-Aug-2009 (kschendel) SIR 122512
##	    Install new hash-related parameters if needed.  Make "if param
##	    not defined, define it" into a loop to make it easier to read
##	    and add new parameters to.
##	21-Sep-2009 (bonro01)
##	    Add code to block upgrade of Ingres releases before 9.2
##	    if the character set on those installations is UTF8.
##	19-Nov-2009 (kschendel) SIR 122890
##	    Raise dmf_tcb_limit if it's the old default value of 5000.
##	11-Dec-2009 (smeke01) b123049
##	    Add in missing 'shift' statement, to fix -test flag.
##	10-Feb-2010 (kschendel) SIR 122757
##	    Add new fallocate option to parameter upgrades.
##	25-Feb-2010 (jonj)
##	    SIR 121619 MVCC: Add rcp.log.readbackward_blocks, readforward_blocks
##    15-Dec-2009 (hanje04)
##        Bug 123097
##        Remove references to II_GCNXX_LCL_VNODE as it's obsolete.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	09-Mar-2010 (bonro01)
##	    Fix typo and change == to =
##	05-Apr-2010 (hanje04)
##	    SIR 123296
##	    Fix typo in demodb section for LSB builds
##	09-Apr-2010 (frima01) SIR 123296
##	    Change demodir to the data directory where the demodb scripts are.
##      21-Apr-2010 (hanje04)
##          SIR 123296
##          Call createdb explicity as /usr/bin/createdb had to be removed
##          to prevent conflicts for LSB builds.
##	07-May-2010 (stial01) SIR 122205
##          Added offline_error_action, online_error_action
##	23-Jun-2010 (frima01) SIR 123296
##	    Move defining createdb to the init section so it will be present
##	    everywhere in this script.
##	08-Jul-2010 (hanje04)
##	    SD 144213
##	    BUG 124055
##	    Make II_CONFIG_TYPE=TRAD a no-op as it just refers to the default
##	    Ingres configuration and causes an error in install.log
##	20-Jul-2010 (kschendel) SIR 124104
##	    Add create_compression to the upgrade list.
##	3-Aug-2010 (kschendel) SIR 122757
##	    Change that added config.direct_io should include direct_io_log.
##	03-Aug-2010 (miket) SIR 124154
##	    Add dmf_crypt_maxkeys to the upgrade list.
##	25-Aug-2010 (bonro01) BUG 124305
##	    Remove confusing Independent Storage device prompt.
##	07-Oct-2010 (frima01) SIR 124560
##	    On upgrade always upgrade demodb if existent.
##	    
#----------------------------------------------------------------------------
. iisysdep
. iishlib
# override value of II_CONFIG set by ingbuild
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    II_CONFIG=$II_SYSTEM/ingres/files
    export II_CONFIG
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc

if [ "$1" = "-rmpkg" ] ; then
   cat << !
  Ingres DBMS has been removed

!
exit 0
fi

SELF=`basename $0`

WRITE_RESPONSE=false
READ_RESPONSE=false
BATCH=false
DEBUG_DRY_RUN=false
DEBUG_FAKE_NEW=false
DOIT=""
DIX=""
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp   
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
RPM=false



# check parameters
while [ $# != 0 ]
do
    if [ "$1" = "-batch" ] ; then
        BATCH=true
        INSTLOG="2>&1 | cat >> $II_LOG/install.log"
        shift
    elif [ "$1" = "-vbatch" ] ; then
        BATCH=true
        shift
    elif [ "$1" = "-rpm" ] ; then
        BATCH=true
        RPM=true
        INSTLOG="2>&1 | cat >> $II_LOG/install.log"
        shift
    elif [ "$1" = "-mkresponse" ] ; then
        WRITE_RESPONSE=true
        shift
        [ "$1" ] && { RESOUTFILE="$1" ; shift ; }
    elif [ "$1" = "-exresponse" ] ; then
        READ_RESPONSE=true
        BATCH=true	#run as if running -batch flag.
        shift
        [ "$1" ] && { RESINFILE="$1" ; shift ; }
    elif [ "$1" = "-test" ] ; then
        #
        # This prevents actual execution of most commands, while
        # logging commands being executed to iisudbms.dryrun.$$.
        # config.dat & symbol.tbl values are appended to this
        # file, prior to restoring the existing copies (if any)
        #
        DEBUG_LOG_FILE="/tmp/${SELF}_test_output_`date +'%Y%m%d_%H%M'`"
        DOIT=do_it
        DIX="\\"
        shift
        while [ "$1" ]
        do
	    case "$1" in
	    new)
	        DEBUG_FAKE_NEW=true
	        DEBUG_DRY_RUN=true
	        ;;
	    dryrun) 
	        DEBUG_DRY_RUN=true
	        ;;
	    esac
	    shift
        done
	    
        cat << !
Logging $SELF run results to $DEBUG_LOG_FILE

DEBUG_DRY_RUN=$DEBUG_DRY_RUN  DEBUG_FAKE_NEW=$DEBUG_FAKE_NEW
!
        cat << ! >> $DEBUG_LOG_FILE
Starting $SELF run on `date`
DEBUG_DRY_RUN=$DEBUG_DRY_RUN  DEBUG_FAKE_NEW=$DEBUG_FAKE_NEW

!
    elif [ "$1" ] ; then
    cat << !

    ERROR: invalid parameter "{$1}".

    Usage $SELF [ -batch [<inst_id>] | -vbatch [<inst_id>] | \
    	   -mkresponse [ <resfile> ] | -exresponse [ <resfile> ] ]
!
        exit 1
    fi
done

export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE 
export RESINFILE 

#
# Do a basic sanity check (II_SYSTEM set to a directory).
#
[ -z $II_SYSTEM -o ! -d $II_SYSTEM ] &&
{
    cat << !

II_SYSTEM must be set to the Ingres root directory before running $SELF.

Please correct your environment before running $SELF again.

!
    exit 1
}

createdb=$II_SYSTEM/ingres/bin/createdb

check_response_file 	# Validate response file (if any)

#=================================================================
#
#	Utility routine to prompt user if he wants to abort set up
#
confirm_abort()
{
   abort=$BATCH
   $BATCH ||
   {
      prompt "Do you want to abort DBMS setup?" n && abort=true
   }
   $abort &&
   {
      cat << !

Aborting DBMS set up.

!
      exit 1
   }
}

#=[ pluralize ]===================================================
#
#	Echo either singular or plural form of passed noun.
#	based on the quantity parameter.  
#
#	Params:	$1 - singular form of noun
#		$2 - quantity of object
#		$3 - plural form of noun, if absent, just tack on 's'
#
#	Like this whole module, this badly need localization.
#	
#=================================================================
pluralize()
{
    if [ $2 -eq 1 ]
    then
	echo "$1"
    elif [ -z "$3" ]
    then
	echo "${1}s"
    else
	echo "$3"
    fi
}

#=[ kbytes_to_mbytes ]============================================
#
#	Echo passed Kilobyte amount as a descriptive string in
#	megabytes.
#
#	Params:	$1 - value to format.
#
kbytes_to_mbytes()
{
    approx=""
    mbs=`expr $1 / 1024`
    if [ $1 -ne `expr $mbs \* 1024` ] ; then
       approx="approximately "
       [ 512 -le `expr $1 - $mbs \* 1024` ] && \
	mbs=`expr $mbs + 1`
    fi
    echo "$approx ${mbs}M bytes"
}

#=[ get_log_size ]================================================
#
#	Utility routine to prompt for log size
#
#	Params: $1 - Rad number, or NULL
#
get_log_size()
{
   prompt "Do you want to change the default transaction log size?" n &&
   {
      echo ""
      iiechonn "Please enter the desired transaction log size (in Megabytes): "
      read LOG_MBYTES junk
      while [ -z "$LOG_MBYTES" ] || validate_resource \
	 "ii.*.setup.rcp.file.mbytes" $LOG_MBYTES
      do
	 iiechonn "Please enter the desired size transaction log size (in Megabytes): "
	 read LOG_MBYTES junk
      done
      LOG_KBYTES=`expr $LOG_MBYTES \* 1024`

      if $WRITE_RESPONSE
      then
	  check_response_write LOG_KBYTES${1} $LOG_KBYTES replace
      else
	  $DOIT iisetres ii.${LOGHOST}.rcp.file.kbytes $LOG_KBYTES
      fi
      return 0
   }
   return 1
}

#=[ node_number_instuctions ]=====================================
#
#	Print instuctions for selecting a node number
#
node_number_instuctions()
{
    cat << !

Each node (member) of an Ingres cluster must be assigned a unique
node number which identifies it with respect to other cluster members.

Surviving node with the lowest node number will perform recovery on
the behalf of failing node(s), so lower numbers should be assigned
to nodes running on fast machines within the cluster.

!
}

#=[ show_available_rads ]=========================================
#
#	Display information about the RADs to configure.
#
#	Indicate via return value if none are available.
#
show_available_rads()
{
    cat << !
Available RADs for setup are:
!
    i=0
    j=1
    while [ $i -lt $NUM_RADS ]
    do
	i=`expr $i + 1`
	in_set "$EXISTING_RAD_IDS" $i && continue
	j=0
	iinumainfo -radinfo $i
    done
    [ $j -eq 1 ] && cat << !
  <none>

You cannot setup additional virtual nodes.
!
    return $j
}


#=[ get_rad_number ]==============================================
#
#	Utility routine to prompt for a unique RAD number in
#	the range 1 .. $NUM_RADS
#
#	Data:	NUM_RADS - high bound of input
#		NEW_RAD_IDS - RAD ids already selected.
#		EXISTING_RAD_IDS - RAD ids from previous runs.
#
#	Outputs: GETRES - valid candidate RAD # or "".
#
get_rad_number()
{
    gen_ok_set "$EXISTING_RAD_IDS $NEW_RAD_IDS" $NUM_RADS
    [ $okset ] || return 1		# Nothing to select

    while :
    do
	iiechonn \
	 "Please select a RAD identifier from the set (${okset}): "
	read GETRES junk
	[ "$GETRES" ] || \
	    return 1			# Exit empty
	[ -z "`echo $GETRES | sed 's/[0-9]//g'`" ] ||
	{
	    cat << !
Bad input ($GETRES).   Please try again.

!
	    continue
	}
	[ $GETRES -gt 0 -a $GETRES -le $NUM_RADS ] ||
	{
	    cat << !
$GETRES is out of range.

!
	    continue
	}
	in_set "$NEW_RAD_IDS $EXISTING_RAD_IDS" $GETRES || \
	    return 0			# Got a valid RAD
	cat << !
RAD number $GETRES is already in use.

!
    done
}

#=[ set_cluster_defaults ]========================================
#
#	Set certain per "CONFIG_HOST" options.
#
set_cluster_defaults()
{
    if $WRITE_RESPONSE
    then
	# Do nothing, we're just generating a response file
	:
    else
	[ "$VERS" = "axp_osf" ] && $DOIT ingsetenv II_THREAD_TYPE INTERNAL
	$DOIT ingsetenv II_CLUSTER 1
    fi
}

#=[ gen_log_ext ]=================================================
#
#	Put logic for generating the log extension in one place.
#	Log extension is echoed to stdout.  A non-null extension is
#	only generated for clustered Ingres.
#
#	Params:	$1	RAD id or nothing
#
#	Data:	II_CLUSTER_ID	- if not NULL, then Ingres is clustered.
#
gen_log_ext()
{
    if [ "$II_CLUSTER_ID" != "" ]
    then  
	echo "_`node_name $1 | tr '[a-z]' '[A-Z]'`"
    else
	echo ""
    fi
}

#=[ truncate_file_name ]==========================================
#
#	Echo passed filename, truncated to 36 chars.  This
#	prevents possible problems with generated filenames
#	and the CL DI facility, which cannot handle extra
#	long names.
#
#	params:	$1 - filename
#
truncate_file_name()
{
    echo "$1"  | cut -c 1-36 
}

#=[ gen_log_file_path ]============================================
#
#	Echo a full log path from the following
#
#	params:	$1 - directory root
#		$2 - base log file name
#		$3 - log_segment #
#		$4 - Clustered Ingres log extension or empty
#
#	Log file path is constructed as follows:
#	<directory root>/ingres/log/<base log file name>.l<log_segment #><ext>
#
#	<log_segment #> is zero padded on the left to 2 digits.
#	Total file name length is truncated  to 36 characters to
#	keep CL DI facility happy.
#
gen_log_file_path()
{
    echo "${1}/ingres/log/`truncate_file_name \"${2}.l\`echo \"0${3}\" \
     | sed 's/0\([0-9][0-9]\)/\1/'\`${4}\"`"
}

#=[ configure_logs ]==============================================
#
#	Encapsulate the configure log logic, to allow it
#	to be called iteratively.
#
#	params:	$1 - RAD being configured or NULL if not NUMA cluster.
#
configure_logs()
{
    LOGHOST=`node_name $1`

    $WRITE_RESPONSE ||
    {
	$DOIT iisetres ii.$LOGHOST.rcp.log.log_file_name ingres_log
	$DOIT iisetres ii.$LOGHOST.rcp.log.dual_log_name dual_log
    }

    # Find out if we want a backup transaction log
    II_DUAL_LOG_CONFIG=false
    $BATCH ||
    {
        prompt "Do you want to disable the backup transaction log?" n ||
	  II_DUAL_LOG_CONFIG=true
    }

    if $LOCATION_HELP
    then 
        cat << !

You must verify that the location(s) you intend to select for the Ingres
transaction log(s) have sufficient disk space.  The default transaction
log size is:

	${LOG_KBYTES}K bytes  (`kbytes_to_mbytes $LOG_KBYTES`)

This setup procedure will create the Ingres transaction log(s) as
ordinary (operating system buffered) files, rather than unbuffered
("raw") device files.  For information on using the "mkrawlog" utility
to create raw transaction log files after this setup procedure has been
completed, please refer to the Ingres Installation Guide.

!
    else
	$BATCH ||
	{
	     cat << ! 

The default size for the Ingres transaction log is: 

	   ${LOG_KBYTES}K bytes (`kbytes_to_mbytes $LOG_KBYTES`)

!
	}
    fi

    if $BATCH
    then
	$READ_RESPONSE && 
	{
	  II_LOG_FILE_SIZE_MB="`iiread_response II_LOG_FILE_SIZE_MB $RESINFILE`"
	  if [ "$II_LOG_FILE_SIZE_MB" ] ; then
      	    LOG_KBYTES=`expr $II_LOG_FILE_SIZE_MB \* 1024`
	  else
	    LOG_KBYTES="`iiread_response LOG_KBYTES${1} $RESINFILE`"
	  fi
	}
	[ "$LOG_KBYTES" ] || LOG_KBYTES=262144
	$DOIT iisetres ii.${LOGHOST}.rcp.file.kbytes $LOG_KBYTES
    else
	# allow user to change default transaction log size
	get_log_size ${1}
    fi
   
    $WRITE_RESPONSE || \
	$DOIT iisetres ii.${LOGHOST}.rcp.log.log_file_parts 1

    while true
    do
        config_location "II_LOG_FILE" "Ingres transaction log" ${1} && break;
        get_log_size ${1} || confirm_abort
    done

    $II_DUAL_LOG_CONFIG &&
    {
	while true
	do
	    config_location \
		  "II_DUAL_LOG" "backup Ingres transaction log" ${1} && break
	    get_log_size ${1} || confirm_abort
        done
    }

    # In read response mode we are running in batch mode but it is possible 
    # we have set a value in the response file. 
    #
    if $READ_RESPONSE
    then
	GETRES=`iiread_response II_DUAL_LOG${1} $RESINFILE`
	[ -n "${GETRES}" ] &&
	{
	    config_location \
	     "II_DUAL_LOG" "backup Ingres transaction log" ${1} || \
	      confirm_abort
	}
    fi
}

#=[ setup_one_log ]===============================================
#
#	Centralizes code common to primary & dual log set up
#
#	params: $1 - log name to use in messages (primary|backup)
#		$2 - additional parameter to iimklog (""|"-dual")
#
#	data:	LOGHOST	- equals CONFIG_HOST, except in NUMA clusters
#		LOGEXT 	- "", or "_${LOGHOST}"
#		LOG	- full rooted path of target log.
#		RAD_ARG - "", or "-rad=<rad>" if NUMA clusters
#		RAW_LOG_KBYTES - "" or size of existing log.
#
setup_one_log()
{
    dual_arg=${2}
    if [ -c "$LOG" ]
    then
	# Existing transaction log is raw
	cat << !

A raw ${1} transaction log already exists.
!
	[ -z "$RAW_LOG_KBYTES" ] &&
	{
	    # use dd to determine size of raw transaction log
	    cat << !

Determining size of raw transaction log...
!
	    dd if=$LOG of=/dev/null bs=32k >/tmp/mkraw.$$ 2>&1
	    if fgrep "+" /tmp/mkraw.$$ > /tmp/mkrawout.$$ 2>&1
	    then
		RAW_LOG_BLOCKS=`sed -e 's/+.*//' -e '2,$d' /tmp/mkrawout.$$`
		RAW_LOG_KBYTES=`expr $RAW_LOG_BLOCKS \* 32`
		echo $RAW_LOG_KBYTES > \
		  $II_LOG_FILE/ingres/log/rawlog.size${LOGEXT}
		    
		$DOIT rm -f /tmp/mkraw.$$
		$DOIT rm -r /tmp/mkrawout.$$
		$DOIT iisetres ii.$LOGHOST.rcp.file.kbytes $RAW_LOG_KBYTES 
	    else
		cat << !

Unable to determine size of raw transaction log file.

Run mkrawlog as root to create a new raw transaction log,
then re-run $SELF.

!
		$DOIT rm -f /tmp/mkraw.$$
		$DOIT rm -r /tmp/mkrawout.$$
		exit 1
	    fi
	} 
	$DOIT iimklog $dual_arg $RAD_ARG -erase ||
	{
	    cat << !
Correct the above problem and re-run $SELF. 

!
	    exit 1
	}
    elif [ -f "$LOG" ]
    then
	# Have existing non-raw transaction log
	# In this case we can't be sure about the file.kbytes resource,
	# tell iimklog to reset it properly.
	cat << !

Erasing existing Ingres ${1} transaction log.
!
	$DOIT ipcclean $RAD_ARG
	setres='-setres'
	if [ -n "$dual_arg" ] ; then
	    setres=
	fi
	$DOIT iimklog $dual_arg $RAD_ARG -erase $setres ||
	{
	    cat << !
Error erasing the existing ${1} transaction log at:
$LOG
Correct the above problem and re-run $SELF. 

!
	    exit 1
	}
    else
        cat << !

The ${1} transaction log will now be created as an ordinary (buffered)
system file.  For information on how to create a "raw", or unbuffered,
transaction log, please refer to the Ingres Installation Guide, after
completing this setup procedure.
!
        $DOIT iimklog $dual_arg $RAD_ARG ||
        {
	    cat << !
The system utility "iimklog" failed to complete successfully.  You must
correct the problem described above and re-run $SELF. 

!
	    exit 1
        }
    fi
}

#=[ setup_transaction_logs ]======================================
#
#	Centralizes the code for creating and formatting transaction logs.
#
#	Params:	$1 - target RAD or nothing
#
setup_transaction_logs()
{
    LOGHOST=`node_name $1`
    LOGEXT=`gen_log_ext $1`
    RAD_ARG=""
    [ -n "${1}" ] && RAD_ARG="-rad=$1"

    #
    # create/erase the primary transaction log
    #
    II_LOG_FILE=`iigetres ii.$LOGHOST.rcp.log.log_file_1`
    LOG="`gen_log_file_path \"${II_LOG_FILE}\" \"${II_LOG_FILE_NAME}\" "1" \"${LOGEXT}\"`"

    setup_one_log "primary"

    II_DUAL_LOG=`iigetres ii.$LOGHOST.rcp.log.dual_log_1`
    if [ $II_DUAL_LOG ]
    then
	#
	# create/erase the backup transaction log
	#
	LOG="`gen_log_file_path \"${II_DUAL_LOG}\" \"${II_DUAL_LOG_NAME}\" "1" \"${LOGEXT}\"`"
	setup_one_log "backup" "-dual"
    fi

    #
    # format the primary transaction log
    #
    cat << !

Formatting the primary transaction log file...
!
    $DOIT ipcclean $RAD_ARG
    $DOIT csinstall $RAD_ARG - ||
    {
       cat << !

Unable to install shared memory.  You must correct the problem described above
and re-run $SELF.

!
       $DOIT ipcclean $RAD_ARG
       exit 1
    }
    $DOIT rcpconfig $RAD_ARG -init_log >/dev/null ||
    {
       cat << !

Unable to write the primary transaction log file header.  You must correct
the problem described above and re-run $SELF.

!
       $DOIT ipcclean $RAD_ARG
       exit 1
    }
    [ "$II_DUAL_LOG" ] &&
    {
       cat << !

Formatting the backup transaction log file...
!
       $DOIT rcpconfig $RAD_ARG -init_dual >/dev/null ||
       {
	  cat << !

Unable to write the Ingres transaction log file header.  You must correct
the problem described above and re-run $SELF.

!
	  $DOIT ipcclean $RAD_ARG
	  exit 1
       }
    }
    $DOIT ipcclean $RAD_ARG
}

#=[ required_param_changes ]====================================
#
#	iigenres does not update all parameters, just new and derived
#	ones.  If we are upgrading from a version that had an existing
#	config.dat, make sure that certain parameters have values that
#	are high enough to run the current version of Ingres.
#
##	This function probably needs to be edited / added to for each
##	release of Ingres.
#
required_param_changes()
{
        # an upgrade installation needs a larger buffer_count and
        # stack size of (DBMS_STACKSIZE) for dbms and recovery servers
	LOGBUFS=`iigetres "ii.$CONFIG_HOST.rcp.log.buffer_count"`
	if [ $LOGBUFS -lt 35 ] ; then
		$DOIT iisetres ii.$CONFIG_HOST.rcp.log.buffer_count 35
	fi
	DBMS_STACK=`iigetres "ii.$CONFIG_HOST.dbms.*.stack_size"`
	RCP_STACK=`iigetres "ii.$CONFIG_HOST.recovery.*.stack_size"`
	[ (DBMS_STACKSIZE) -lt "$DBMS_STACK" ] || \
		$DOIT iisetres "ii.$CONFIG_HOST.dbms.*.stack_size" \
		 (DBMS_STACKSIZE)
	[ (DBMS_STACKSIZE) -lt "$RCP_STACK" ] || \
        	$DOIT iisetres "ii.$CONFIG_HOST.recovery.*.stack_size" \
		 (DBMS_STACKSIZE)
}


#=[ update_parameters ]=========================================
#
#	If it appears that there is a reasonably valid configuration
#	already, avoid iigenres (which wipes out existing values).
#	Instead, simply iiinitres the new ones.  Parameters that
#	have existed but are newly become "official" (exposed in the
#	rules file) should be inited with -keep in case a knowledgeable
#	user had edited the parameter into config.dat by hand.
#
#	Some tricky stuff goes on for parameters that are being
#	renamed or replaced.  A special rule is set up to define
#	the new parameter in terms of the old one instead of just
#	taking the rule default.
#
##	This section would seem to require editing with each new release,
##	as the definition of "brand new" for a parameter clearly changes
##	with time.  At some point we will probably want these things
##	in a file with a release number tag.
##
##	The current state of this section is good for ->release 3 updating.
#
update_parameters()
{
    ## Parameters added since the start of r3
    for param in system_lock_level di_zero_bufsize fd_affinity dmf_build_pages \
		fallocate opf_new_enum opf_greedy_factor \
		qef_hash_rbsize qef_hash_wbsize qef_hash_cmp_threshold \
		qef_hashjoin_min qef_hashjoin_max create_compression \
		dmf_crypt_maxkeys
    do
	x=`iigetres "ii.$CONFIG_HOST.dbms.*.$param"`
	if [ -z "$x" ] ; then
	    $DOIT iiinitres $param $II_CONFIG/dbms.rfm
	fi
    done

    ## rcp.log parameters added at 10.0 for MVCC
    for param in readbackward_blocks readforward_blocks
    do
	x=`iigetres "ii.$CONFIG_HOST.rcp.log.$param"`
	if [ -z "$x" ] ; then
	    $DOIT iiinitres $param $II_CONFIG/dbms.rfm
	fi
    done

    ## offline_error_action, online_error_action added at 10.0 for SIR 122205
    for param in offline_error_action online_error_action
    do
	x=`iigetres "ii.$CONFIG_HOST.recovery.*.$param"`
	if [ -z "$x" ] ; then
	    $DOIT iiinitres $param $II_CONFIG/dbms.rfm
	fi
    done

    ## was dbms.*.direct_io, now is config.direct_io.  If not there at
    ## all, add the new one;  else translate old to new.
    ## Note: due to unfortunate peculiarities of iiinitres, we'll
    ## probably end up with both ii.*.config.direct_io AND
    ## ii.hostname.config.direct_io.  The second is the one the
    ## server uses.  No good way to get rid of the * versions.
    x=`iigetres "ii.$CONFIG_HOST.config.direct_io"`
    if [ -z "$x" ] ; then
	x=`iigetres "ii.$CONFIG_HOST.dbms.*.direct_io"`
	$DOIT iiinitres direct_io $II_CONFIG/dbms.rfm
	$DOIT iiinitres direct_io_log $II_CONFIG/dbms.rfm
	$DOIT iiinitres direct_io_load $II_CONFIG/dbms.rfm
	if [ -n "$x" ] ; then
	    $DOIT iisetres -v "ii.$CONFIG_HOST.config.direct_io" "$x"
	    $DOIT iisetres -v "ii.$CONFIG_HOST.config.direct_io_log" "$x"
	    ## Leave direct_io_load OFF, has to be set explicitly.
	fi
    fi

    ## 10.x counts partition TCB's towards tcb_limit.  If it looks like
    ## the limit value was the old default, increase it.
    x=`iigetres "ii.$CONFIG_HOST.dbms.*.dmf_tcb_limit"`
    if [ -n "$x" ] ; then
	if [ $x -eq 5000 ] ; then
	    $DOIT iisetres "ii.$CONFIG_HOST.dbms.*.dmf_tcb_limit" 10000
	fi
    fi

    ## New default for result_structure is heap, but keep cheap for
    ## existing installations for compatibility.
    x=`iigetres "ii.$CONFIG_HOST.dbms.*.result_structure"`
    if [ -z "$x" ] ; then
	$DOIT iiinitres result_structure $II_CONFIG/dbms.rfm
	$DOIT iisetres -v "ii.$CONFIG_HOST.dbms.*.result_structure" 'cheap'
    fi
    ## Ingres 2006 change:  revert audit_mechanism back to "INGRES"
    ## if it was "CA".  For whatever reason, this parm is normally
    ## seen as '*' rather than $CONFIG_HOST.
    ## Note: also done in iisuc2.sh but out of paranoia I'm leaving this here.
    x=`iigetres 'ii.*.c2.audit_mechanism'`
    if [ "$x" = 'CA' ] ; then
	$DOIT iisetres -v 'ii.*.c2.audit_mechanism' INGRES
    fi
    ## See if it looks like we're already at r3, if so don't do
    ## anything.
    x=`iigetres "ii.$CONFIG_HOST.dbms.*.qsf_guideline"`
    if [ -z "$x" ] ; then
	## Here for 2.6 or earlier to r3 or later.
	cat << !
  
Updating existing configuration ...
!

	## FIXME. Save ii.host.dbms.*.opf_stats_nostats_factor
	## and reset after doing everything else, value is being trashed 
	## somewhere in this funtion but it is unclear where.
	opfsav=`iigetres "ii.$CONFIG_HOST.dbms.*.opf_stats_nostats_factor"`

	## FIXME no point in deleting unused params like rdf_tbl_idxs
	## until we have an iiremres that can do wildcarding
	## remove dbms.$.rdf_tbl_idxs

	# Brand new parameters for this release
	$DOIT iiinitres cache_guideline $II_CONFIG/dbms.rfm
	$DOIT iiinitres opf_pq_dop $II_CONFIG/dbms.rfm
	$DOIT iiinitres opf_pq_threshold $II_CONFIG/dbms.rfm
	$DOIT iiinitres qef_sorthash_memory $II_CONFIG/dbms.rfm
	$DOIT iiinitres qsf_guideline $II_CONFIG/dbms.rfm

	# Newly exposed parameters, might have an existing value
	$DOIT iiinitres -keep dmf_int_sort_size $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep gc_interval $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep gc_num_ticks $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep gc_threshold $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep hex_session_ids $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep opf_hash_join $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep opf_stats_nostats_max $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep opf_stats_nostats_factor $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep pindex_bsize $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep pindex_nbuffers $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep psf_maxmemf $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep psf_vch_prec $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep psort_bsize $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep psort_nthreads $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep psort_rows $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep qef_hash_mem $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep rep_dq_lockmode $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep rep_dt_maxlocks $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep rep_iq_lockmode $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep rep_sa_lockmode $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep rule_upd_prefetch $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep size_io_buf $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep ulm_chunk_size $II_CONFIG/dbms.rfm
	$DOIT iiinitres -keep archiver_refresh $II_CONFIG/dbms.rfm

	# Special case for recovery database-limit, name is used
	# for other things we don't want to touch
	oldval=`iigetres "ii.$CONFIG_HOST.recovery.*.database_limit"`
	if [ -z "$oldval" ] ; then
	    $DOIT iisetres "ii.$CONFIG_HOST.recovery.*.database_limit" `iigetres "ii.$CONFIG_HOST.rcp.log.database_limit"`
	fi

	# Special case for recovery 2K cache, increase if it's the old
	# default value
	oldval=`iigetres ii.$CONFIG_HOST.rcp.dmf_cache_size`
	if [ "$oldval" -eq 200 ] ; then
	    $DOIT iisetres ii.$CONFIG_HOST.rcp.dmf_cache_size 1000
	fi

	# Trade in old parameters for new.
	# Compute qef_dsh_memory from old qef_qep_mem (generously).
	# Copy old rdf_tbl_cols to new rdf_col_defaults.
	# Compute cp_interval_mb from old cp_interval.
	# Compute dmf_tcb_limit from old dmf_hash_size.
	# The temporary rules have to live in II_CONFIG.

	rm -f ${cfgloc}/sudbms$$.crs
	cat >${cfgloc}/sudbms$$.crs <<'!EOF!'
ii.$.dbms.$.dmf_tcb_limit:	8 * ii.$.dbms.$.dmf_hash_size, MIN = 2048;
ii.$.dbms.$.qef_dsh_memory:	ii.$.dbms.$.qef_qep_mem *
				ii.$.dbms.$.cursor_limit *
				ii.$.dbms.$.connect_limit,
				MIN = 10M, MAX=750M;
ii.$.dbms.$.rdf_col_defaults:	ii.$.dbms.$.rdf_tbl_cols;
-- in case of no existing rdf_tbl_cols:
ii.$.dbms.$.rdf_tbl_cols:	50;
ii.$.rcp.log.cp_interval_mb:	(ii.$.rcp.file.kbytes * (ii.$.rcp.log.cp_interval / 100) + 512) / 1024,
				MIN = 1;
ii.$.rcp.log.cp_interval:	5;
!EOF!
	rm -f /tmp/rfm$$
	cat $II_CONFIG/dbms.rfm >/tmp/rfm$$
	echo rulefile.99: sudbms$$.crs >>/tmp/rfm$$
	$DOIT iiinitres -keep qef_dsh_memory /tmp/rfm$$
	$DOIT iiinitres -keep rdf_col_defaults /tmp/rfm$$
	$DOIT iiinitres -keep cp_interval_mb /tmp/rfm$$
	$DOIT iiinitres -keep dmf_tcb_limit /tmp/rfm$$
	rm -f /tmp/rfm$$ ${cfgloc}/sudbms$$.crs

	# Make sure that parameters are large enough
	required_param_changes

	## Reset opf_stats_nostats_factor if we need to 
	[ "$opfsav" ] && \
	iisetres ii.$CONFIG_HOST.dbms.*.opf_stats_nostats_factor $opfsav

    fi

    # Note that we've done Good Things for this release
    $DOIT iisetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID defaults

}  # update_parameters


#=[ generate_parameters ]=========================================
#
#	If there doesn't appear to be any useful DBMS configuration,
#	come here to generate one.
##
## It's probably too late in the game to make use of FROM64,
## the effort expended translating parameters isn't worth the number
## of 6.4 upgrades left by now.  (schka24 Sep 2004)

generate_parameters()
{

	cat << !
  
Generating default configuration...
!
	DBMS_RFM_FILE=$II_CONFIG/dbms.rfm
  
	## Never smash log-file-size regardless of "new" config or not
	## Don't smash ANSI/ISO conformance if we might have known it
	## (eg upgrade from 1.x)
	logkb=`iigetres ii.$CONFIG_HOST.rcp.file.kbytes`
	ansi=
	if [ -z "$FROM64" ] ; then
	    ansi=`iigetres ii.$CONFIG_HOST.fixed_prefs.iso_entry_sql-92`
	fi
	if $DOIT iigenres $CONFIG_HOST $DBMS_RFM_FILE; then
	    if [ -n "$logkb" ] ; then
		$DOIT iisetres ii.$CONFIG_HOST.rcp.file.kbytes $logkb
	    fi
	    if [ -n "$ansi" ] ; then
		$DOIT iisetres ii.$CONFIG_HOST.fixed_prefs.iso_entry_sql-92 $ansi
	    fi
	    required_param_changes
	    $DOIT iisetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID defaults
	else
	    cat << !
An error occurred while generating the default configuration.
  
!
	    exit 1
	fi
} # generate_parameters


#=[ abort_if_up ]===========================================
#
#	Check to see if it appears Ingres is running.
#	If it is, put out a warning, then abort.
#
#	params:	$1 - rad number or nothing
#
abort_if_up()
{
    r=""
    [ -n "$1" ] && r="-rad $1"

    i=`csreport $r | grep "inuse 1" | wc -l`
    if [ $i -ne 0 ]
    then
	cat << !

There seems to be an installation already running.
Run ingstop to stop it and re-run $SELF.

!
	exit 1
    fi
}

#=[ clean_up ]====================================================
#
#	Cleanup routine
#
clean_up()
{
    trap : 0

    if [ $DEBUG_LOG_FILE ]
    then
	cat << ! >> $DEBUG_LOG_FILE

--- symbol table after dry run ---

!
	ingprenv >> $DEBUG_LOG_FILE
	cat << ! >> $DEBUG_LOG_FILE

--- config.dat generated by dry run ---
!
	[ -f ${cfgloc}/config.dat ] && \
	 cat ${cfgloc}/config.dat  >> $DEBUG_LOG_FILE
	cat << ! >> $DEBUG_LOG_FILE

--- protect.dat generated by dry run ---
!
	[ -f ${cfgloc}/protect.dat ] && \
	 cat ${cfgloc}/protect.dat  >> $DEBUG_LOG_FILE

	echo "Closing $DEBUG_LOG_FILE"
    fi

    if $WRITE_RESPONSE || $DEBUG_DRY_RUN
    then
	for f in config.dat symbol.tbl
	do
	    if [ -f /tmp/$f.$$ ] ; then
		$DOIT rm -f ${cfgloc}/$f
		$DOIT cp /tmp/$f.$$ ${cfgloc}/$f
		[ -s ${cfgloc}/$f ] || rm -f ${cfgloc}/$f >/dev/null 2>&1
	    fi
	done
    fi

    $DOIT rm -f ${cfgloc}/config.lck /tmp/*.$$ >/dev/null 2>&1
    $DOIT rm -f $II_CONFIG/sudbms$$.crs >/dev/null 2>&1
}

#=[ MAIN ]========================================================
#
#	Routine containing main body of DBMS setup
#
do_iisudbms()
{
echo "Setting up the Ingres Intelligent DBMS..."

trap "clean_up ; exit 1" 0 1 2 3 15

#
# Set the SETUID bit for all necessary files extracted from
# the install media.
#
set_setuid

#
#  Get basic info on NUMA aspects of current box (if any)
#
NUM_RADS=""		# Assume non-NUMA architecture for box.
GETRES="`iinumainfo -radcount`"
if [ -n "$GETRES" -a -z "`echo \"$GETRES\" | sed 's/[0-9]*//'`" ]
then
    NUM_RADS=$GETRES
fi

#
#  If faking it (generating response file or debug testing)
#  backup any real config.dat, and symbol.tbl files, so we
#  can play with temp versions of them.
#
#  Otherwise, check if Ingres is running.  If so, stop
#  right here.   
#
#  Note: it is probably a bad idea to generate a response
#  file, or do debug testing with an active installation
#  up, since Ingres may query these files in an incomplete
#  or inconsistent state.
#
if $WRITE_RESPONSE || $DEBUG_DRY_RUN
then	# Set up for creating response file
    # Backup config.dat, and symbol.tbl
    for f in config.dat symbol.tbl
    do
	[ -f ${cfgloc}/$f ] &&
	{
	    cp ${cfgloc}/$f /tmp/$f.$$ >/dev/null 2>&1 || \
	    {
		cat << !
ERROR: unable to backup files to /tmp.  Check write permission,
and disk space availability.
!
		exit 1
	    }
	}
    done
    if $DEBUG_DRY_RUN
    then
	$DEBUG_FAKE_NEW && \
	 rm ${cfgloc}/config.dat ${cfgloc}/symbol.tbl
    else
	mkresponse_msg
    fi
else
    abort_if_up		# Check for regular installs this II_SYSTEM.
    if [ -n "$NUM_RADS" ]
    then
	rad=0
	while [ $rad -lt "$NUM_RADS" ]
	do
	    rad=`expr $rad + 1`
	    abort_if_up $rad
	done
    fi
fi

# Before touching config.dat, see if this looks like it might be an
# upgrade from 6.4 (or any pre-1.x) -- we can be a little nicer.
## The usefulness of FROM64 is much decreased at this late date,
## but we can still do a couple things with it.

FROM64=
if [ ! -f ${cfgloc}/config.dat -a -f $II_CONFIG/rundbms.opt ] ; then
    FROM64=Y
fi
export FROM64

touch_files # make sure symbol.tbl and config.dat exist

# Detect upgrade of pre-9.2 installation with UTF8 to 9.3 or above
II_DATABASE=`ingprenv II_DATABASE`
if [ -d "$II_DATABASE" -a -d $II_DATABASE/ingres/data/default/iidbdb ] ; then
  TMP_II_INSTALLATION=`ingprenv II_INSTALLATION`
  TMP_II_CHARSET=`ingprenv II_CHARSET${TMP_II_INSTALLATION}`
  if [ "$TMP_II_CHARSET" = "UTF8" ]; then
    TMP_VERSION=`head -1 $II_SYSTEM/ingres/version.rel | sed -e 's/^.*(//' -e 's#/.*$##' -e 's/[.]//' `
    TMP_RELEASE=`grep config.dbms.ii ${cfgloc}/config.dat | sed -e s/^.*config.dbms.ii// -e s/${TMP_VERSION}.*$//`
    if [ $TMP_RELEASE -lt 920 ]; then
      cat << !

Unable to upgrade.  UTF8 installation prior to Ingres 9.2 cannot be upgraded.

!
      $BATCH || pause
      exit 1
    fi
  fi
fi

# 
#	Initialize certain global data items
#
II_CLUSTER_ID=""	# Assume non-Cluster 
NUMA_CLUSTER=false	# NUMA cluster is NUMA plus II_CLUSTER_ID
NEW_RADS=0		# Number of RADs added (Always 0 for non-NUMA).
NEW_RAD_IDS=""		# List of RADs to be configured. (Normally empty).
FIRST_RAD=""		# First New RAD configured IF NUMA & clustered.
EXISTING_RAD_IDS=""	# RADs already in use on this box.

#-----------------------------------------------------------------------
#		     NUMA specific configuration
#
# Check if NUMA support available/useful.  Only prompt
# for NUMA configuration if # RADs > 1.
#
[ -n "$NUM_RADS" ] &&
{
    [ $NUM_RADS -le 1 ] && NUM_RADS=""
}

if [ -n "$NUM_RADS" ]
then
    #
    # Useful NUMA support available.
    #
    GETRES=`iigetres ii.$CONFIG_HOST.config.numa | tr '[a-z]' '[A-Z]'`
    if [ "$GETRES" = "ON" ]
    then
	#
	# This node is already set up for NUMA.
	#

	# Have virtual nodes already been setup on this box?
	GETRES=`iigetres ii.$CONFIG_HOST.config.cluster.numa | tr '[a-z]' '[A-Z]'`
	if [ "$GETRES" = "ON" ]
	then
	    NUMA_CLUSTER=true
	    HOST_GREP=`grep_search_pattern ${CONFIG_HOST}`
	    EXISTING_RAD_IDS=`grep -i "R[1-9][0-9]*_${HOST_GREP}\.config\.numa\.rad" ${cfgloc}/config.dat 2>/dev/null | \
	     cut -d':' -f2`
	    EXISTING_RAD_IDS=`echo $EXISTING_RAD_IDS`
	else
	    EXISTING_RAD_IDS=`iigetres ii.$CONFIG_HOST.config.numa.rad`
	    [ -n "$EXISTING_RAD_IDS" -a -n "`echo $EXISTING_RAD_IDS | sed 's/[1-9][0-9]*//'`" ] && EXISTING_RAD_IDS=""
	fi
    elif $BATCH
    then
	if $READ_RESPONSE
	then
	    #Can't setup NUMA cluster in batch w/o response file
	    GETRES=`iiread_response NUMA_SETUP $RESINFILE`
	    [ "$GETRES" = "no" ] && NUM_RADS=""
	fi
    else
	cat << !

Target machine $CONFIG_HOST has a Non-Uniform Memory Access (NUMA)
architecture, supporting $NUM_RADS Resource Affinity Domains (RADs).

Ingres may be configured to be aware of the NUMA nature of your machine
by giving preference to memory allocation and process execution to
one RAD.

!
	$CLUSTERSUPPORT && cat << !
You may also set up multiple cooperating Ingres instances on 
separate RADs within $CONFIG_HOST, each functioning as one "virtual"
node of an Ingres Cluster.

By setting up multiple Ingres instances, you may exploit additional
CPUs and memory as local resources, at the expense of the added
complexity and limitations inherent to Clustered Ingres.

If properly licensed, these virtual nodes may also be part of an
Ingres cluster spanning additional machines.

!
	cat << !
You do not have to make Ingres NUMA aware, and Ingres will perform
correctly.  However, there may be a performance benefit to setting
up Ingres in a NUMA aware configuration.

!
	$BATCH || pause
	prompt "Do you want to set up Ingres for NUMA?" y || NUM_RADS=""
	echo ""
	if $WRITE_RESPONSE 
	then
	    if [ -n "$NUM_RADS" ]
	    then
		check_response_write NUMA_SETUP yes replace 
	    else
		check_response_write NUMA_SETUP no replace 
	    fi
	fi
    fi
    [ -n "$NUM_RADS" ] && $DOIT iisetres ii.$CONFIG_HOST.config.numa ON
fi

#---------------------------------------------------------------------------
#			Cluster specific configuration
if $CLUSTERSUPPORT
then	# Only execute if supported by OS/hardware.
    #
    # Determine if user wants to set up for Clustered Ingres.
    #
    II_CLUSTER=`iigetenv II_CLUSTER`
    CLUSTER_ID_PROMPT=false	# Default to non-clustered install

    # Have virtual nodes already been setup on this box?
    if $NUMA_CLUSTER
    then
	CLUSTER_ID_PROMPT=true
    else
	# Examine value of ii.(node).config.cluster_id
	GETRES=`iigetres ii.$CONFIG_HOST.config.cluster.id`
	if [ "$GETRES" != "" ]
	then	# Node number already set, keep previous setting.
	    [ "$GETRES" != "0" -a "$GETRES" != "no" -a "$GETRES" != "NO" ] && \
	     II_CLUSTER_ID="$GETRES"
	elif [ "$II_CLUSTER" != "" ]
	then
	    # use value of II_CLUSTER
	    [ $II_CLUSTER -ne 0 ] && CLUSTER_ID_PROMPT=true
	elif $BATCH
	then	# batch setup
	    if $READ_RESPONSE 
	    then # Get from response file.
		 # Can't setup cluster in batch w/o response file.
		if [ -n "$NUM_RADS" ]
		then
		    GETRES=`iiread_response NUMA_CLUSTER_SETUP $RESINFILE`
		    [ "$GETRES" = "yes" ] && NUMA_CLUSTER=true
		fi
		if $NUMA_CLUSTER
		then
		    CLUSTER_ID_PROMPT=true
		else
		    GETRES=`iiread_response CLUSTER_SETUP $RESINFILE`
		    [ "$GETRES" = "yes" ] && CLUSTER_ID_PROMPT=true
		fi
	    fi
	fi

	if $BATCH || [ $II_CLUSTER_ID ]
	then
	    :
	else
	    # interactive setup
	
	    if [ -n "$NUM_RADS" ]
	    then
		prompt \
		 "Do you wish to set up multiple cluster nodes on this host?" n && \
		    NUMA_CLUSTER=true
	    fi
	    if $NUMA_CLUSTER
	    then
		CLUSTER_ID_PROMPT=true
	    else
		cat << !

$CONFIG_HOST may function as a node in a cluster, you have the
option of setting up the Ingres DBMS as part of an Ingres cluster.

If you do not intend to set up an Ingres cluster this with other
machines in your hardware cluster, then do not set up Ingres
for clusters.

!
		prompt "Do you want to set up Ingres for clusters?" n && 
		  CLUSTER_ID_PROMPT=true
		if $WRITE_RESPONSE
		then
		    if $CLUSTER_ID_PROMPT
		    then
			check_response_write CLUSTER_SETUP yes replace 
		    else
			check_response_write CLUSTER_SETUP no replace 
		    fi
		fi
	    fi
	fi
    fi

    $NUMA_CLUSTER &&
    {
	# Turn NUMA cluster indication ON, turn node wide
	# RAD value off.   Each "virtual" node will have
	# it's own RAD assignment.
	$DOIT iisetres ii.$CONFIG_HOST.config.cluster.numa ON
	$DOIT iisetres ii.$CONFIG_HOST.config.numa.rad 0
    }

    #
    # Get existing node information if setting up new node for Ingres clusters
    #
    if $CLUSTER_ID_PROMPT
    then
	# Get list of existing nodes and node numbers.
	gen_nodes_lists
	NUM_OLD_NODES=0
	for i in $IDSLST
	do
	    NUM_OLD_NODES=`expr $NUM_OLD_NODES + 1`
	done
	FIRSTNODE="`echo $NODESLST | cut -d' ' -f1`"
	GETRES="`iigetres ii.$FIRSTNODE.config.host`"
	[ -n "$GETRES" ] && FIRSTNODE="$GETRES"
    fi

    if $NUMA_CLUSTER
    then
	cat << !

Setting up the Ingres DBMS on "$CONFIG_HOST" as NUMA Clustered Ingres.

!
	show_available_rads || abort

	while :
	do
	    cat << !

Select RADs to configure as virtual nodes using the Ingres RAD identifier
(leftmost column).   Just hit enter when finishing entering RADs.

!
	    [ "$IDSLST" ] || cat << !
Transaction log files left over from any previous version of Ingres,
will be assigned to the first RAD you select.

!
	    while [ $NEW_RADS -lt $NUM_RADS ]
	    do
		if get_rad_number
		then
		    NEW_RAD_IDS="${NEW_RAD_IDS} ${GETRES}"
		    NEW_RADS=`expr $NEW_RADS + 1`
		    [ -z "$FIRST_RAD" ] && FIRST_RAD="${GETRES}"
		else
		    [ $NEW_RADS -ge 1 ] && break
		    cat << !

ERROR: You must select at least one RAD when configuring for NUMA.
!
		    while :
		    do
			prompt "Continue selecting RADs?" y && break
			confirm_abort
		    done
		fi
	    done

	    # Display RADs and confirm.
	    cat << !
Following $NEW_RADS `pluralize RAD $NEW_RADS` selected:
!
	    for i in $NEW_RAD_IDS
	    do
		iinumainfo -radinfo $i
	    done
	    prompt \
	      "Continue setup with selected `pluralize RAD $NEW_RADS`?" y && \
	      break
	    cat << !
Existing selected RAD set has been cleared.

!
	    NEW_RAD_IDS=""
	    NEW_RADS=0
	done 

	#
	#   Get nodes for each RAD
	#
	node_number_instuctions
        
	$BATCH || pause
	for rad in $NEW_RAD_IDS
	do
	    cat << !

Setting up virtual node configuration for RAD $rad.

!
	    $DOIT iisetres "ii.`node_name $rad`.config.numa.rad" $rad
	    $DOIT iisetres "ii.`node_name $rad`.config.host" "$CONFIG_HOST"
	    get_node_number $rad
	    create_node_dirs `node_name $rad`
	done
	create_node_dirs "$CONFIG_HOST"
	set_cluster_defaults

    else
	[ $CLUSTER_ID_PROMPT = "true" -o -n "$II_CLUSTER_ID" ] && cat << !

Setting up the Ingres DBMS on node "$CONFIG_HOST" for Clustered Ingres.
!
	if $CLUSTER_ID_PROMPT
	then
	    node_number_instuctions
	    get_node_number
	    create_node_dirs $CONFIG_HOST
	    set_cluster_defaults
	fi
    fi

    if [ -z "$II_CLUSTER_ID" ]
    then
	cat << !

This procedure will set up the Ingres DBMS in stand-alone mode.

!
	$BATCH || pause
    fi
fi # end of cluster node id setup

#
# If NUMA, and not NUMA clusters, select HOME RAD.
#
$NUMA_CLUSTER ||
{
    if [ -n "$NUM_RADS" -a -z "$EXISTING_RAD_IDS" ]
    then
	#
	# Setup "simple" NUMA environment (either stand-alone Ingres,
	# or NUMA box is only functioning as one node in the Ingres
	# cluster.
	#
	cat << !
Please select a home RAD for your Ingres installation.  Ideally,
as many as possible of the storage devices to be used by Ingres
should be "local" to the selected RAD, and the RAD should have
sufficient CPU and memory.   Note, in a NUMA configuration, Ingres
asks the OS to give preference for process execution and memory
allocation on the home RAD, but the OS is allowed to choose
otherwise.   

!
	show_available_rads
	while :
	do
	    get_rad_number &&
	    {
		$DOIT iisetres "ii.$CONFIG_HOST.config.numa.rad" $GETRES
		break;
	    }
	    cat << !

You must choose a home RAD if you wish to take advantage of the NUMA
aware features of Ingres.

!
	    prompt "Do you wish to configure Ingres for NUMA?" y ||
	    {
		# Guess not
		$DOIT iisetres "ii.$CONFIG_HOST.config.numa" off
		$DOIT iisetres "ii.$CONFIG_HOST.config.numa.rad" 0
		NUM_RADS=""
		break
	    }
	done
    fi	
}

#---------------------------------------------------------------------------
SETUP=""
if $WRITE_RESPONSE ; then
   :	# Do nothing if setting up a response file.
else
   # override local II_CONFIG symbol.tbl setting
   if [ -z "`ingprenv II_CONFIG`" ] ; then
       $DOIT ingsetenv II_CONFIG $II_CONFIG
   fi
   
   SERVER_HOST=`iigetres ii."*".config.server_host` || exit 1
   
   # grant owner and root all privileges 
   $DOIT iisetres ii.$CONFIG_HOST.privileges.user.root \
      SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
   $DOIT iisetres ii.$CONFIG_HOST.privileges.user.$ING_USER \
      SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

   # Turn on 64-bit usage
   [ -f $II_SYSTEM/ingres/lib/lp64/iimerge.o ] &&
      $DOIT iisetres "ii.*.setup.add_on64_enable" ON
   
#
# Don't set gcn local_vnode resource explicitly. Use the default local_vnode
# set in crs grammar (all.crs ). This file is included in all of the
# "*.rfm" files.
   
   [ -z "$SERVER_HOST" ]  &&
   {
      # check for II_GCNxx_LCL_VNODE
      SERVER_ID=`eval II_ADMIN=${symbloc} ingprenv II_INSTALLATION`
      [ -n "$SERVER_ID" ] && 
      {
         # convert FQDNs using iipmhost
         SERVER_HOST=`eval II_ADMIN=${symbloc} iipmhost`
      }
   }

   ## Server-host can be uppercase if it was entered by hand via a former
   ## cluster logical-name setup, or if something else fishy went on.
   LC_SERVER_HOST=`echo $SERVER_HOST | tr '[A-Z]' '[a-z]' `
   [ -n "$SERVER_HOST" ] && \
   [ "$SERVER_HOST" != "$CONFIG_HOST" -a "$LC_SERVER_HOST" != "$CONFIG_HOST" ] && \
   [ -z "$II_CLUSTER_ID" ] &&
   { 
      cat << !
   
The Ingres Intelligent DBMS installation located at:
   
	$II_SYSTEM
   
is already set up to run from:
   
	$SERVER_HOST
   
Once the Ingres Intelligent DBMS has been set up to run on a particular
system, it cannot be moved easily.
   
!
      $BATCH || pause
      trap : 0
      clean_exit 
   }
   
   
   if [ -f $II_SYSTEM/ingres/install/release.dat ] ; then
       VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
       {
           cat << !
   
$VERSION

!
          exit 1
       }
   elif [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      VERSION=`head -1 /usr/share/ingres/version.rel` ||
   {
       cat << !

Missing file /usr/share/ingres/version.rel

!
      exit 1
   }
   else
       VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
       {
           cat << !
   
Missing file $II_SYSTEM/ingres/version.rel
   
!
          exit 1
       }
   fi
   
   RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`
   
   SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`
   if [ "$SETUP" = "complete" ]
   then
	# If testing, then pretend we're not set up, otherwise scram.
	$DEBUG_DRY_RUN ||
	{
	    cat << !
   
The $VERSION version of the Ingres Intelligent DBMS has
already been set up on local host "$HOSTNAME".
   
!
	    $BATCH || pause
	    clean_up
	    exit 0
	}
	SETUP=""
   fi

   cat << ! 
   
This procedure will set up the following release of the Ingres
Intelligent DBMS:
   
	$VERSION
   
to run on local host:
   
	$HOSTNAME
   
This Ingres instance will be owned by:
   
	$ING_USER
!
    if $NUMA_CLUSTER
    then
	cat << !

With virtual `pluralize node $NEW_RADS`:

RAD    Node Name                Node#
!
	j=0
	k=$NUM_OLD_NODES
	for rad in $NEW_RAD_IDS
	do
	    j=`expr $j + 1`
	    k=`expr $k + 1`
	    echo \
"$rad `nth_element \"$NODESLST\" $k` `nth_element \"$IDSLST\" $k`" | \
	    awk '{ printf( "%3d    %-24.24s %8d\n", $1, $2, $3 ) }'
	done
    else
	[ -n "$II_CLUSTER_ID" ] && cat << !

Ingres cluster membership ID:

	$II_CLUSTER_ID
!
	echo ""
    fi
   
   DATA=`eval II_ADMIN=${symbloc} ingprenv II_DATABASE`
fi #end of WRITE_RESPONSE mode ****

[ -z "$SETUP" ] &&
{
   $BATCH ||
   {
      if [ -z "$DATA" ] ; then
         echo ""
         prompt "Do you want to continue this setup procedure?" y || exit 1
      else
         echo ""
      fi
   }
}

if $WRITE_RESPONSE ; then
    :	# Do nothing if just setting up a response file.
else
   if $USE_SHARED_SVR_LIBS
   then
       echo "Re-linking iimerge..."
       if [ -f "`which ${CC}`" ]
       then
	    ADD_ONHB=`ingprenv II_LP64_ENABLED`
	    if [ "$ADD_ONHB" = "TRUE" ] ; then
		lphb="-lp64"
	    fi
	    $DOIT iilink $lphb (DBMS_UDT)
	    lnk_ok=$?
       else
	    echo "ERROR: C compiler not found"	
	    lnk_ok=1
       fi

       [ $lnk_ok -ne 0 ] &&  cat << !

WARNING:
Ingres Server executable could not be re-linked.
This may cause utilities such as ckpdb and rollforwarddb to fail for 
users other than ingres.
Use the following command to do this by hand:

	iilink $lphb (DBMS_UDT)

!
   fi

    # Try to decide if this is an upgrade, at least as far as configuration
    # is concerned.  Look for the "default page size" parameter which was
    # introduced in Ingres II 2.0.  That version included all the multiple
    # cache size stuff, and configurations older than 2.0 should probably
    # be regenerated.
    # Skip for int_rpl (Appliance). As we oftern preload config and it
    # triggers upgrade behavior. As int_rpl is Ingres 2006 onwards we
    # don't need it anyway

    if [ "$VERS" = "int_rpl" ]; then
	x=0
    else
	x=`iigetres "ii.$CONFIG_HOST.dbms.*.default_page_size"`
    fi

    [ -z "$x" ] && x=0
    if [ -z "$SETUP" -a $x -ge 2048 ] ; then
	# Do a configuration update
	update_parameters
	SETUP=defaults
    elif [ -z "$SETUP" ] ; then
	generate_parameters
	SETUP=defaults
    else
	cat << !
Existing configuration remains unchanged...
!
    fi

fi #end WRITE_RESPONSE mode

#
# get II_INSTALLATION 
#

[ -z "$II_INSTALLATION" ] && II_INSTALLATION=`ingprenv II_INSTALLATION`
if [ "$II_INSTALLATION" ] ; then 
   cat << !

II_INSTALLATION configured as $II_INSTALLATION. 
!
   if $WRITE_RESPONSE ; then
        check_response_write II_INSTALLATION $II_INSTALLATION replace 
   else
        $DOIT ingsetenv II_INSTALLATION $II_INSTALLATION
   fi
else
   if $BATCH ; then
      if $READ_RESPONSE ; then
	 II_INSTALLATION=`iiread_response II_INSTALLATION $RESINFILE`
      else
	 II_INSTALLATION="$2"
      fi
      [ -z "$II_INSTALLATION" ] && II_INSTALLATION=II
      case "$II_INSTALLATION" in 
	[a-zA-Z][a-zA-Z0-9])
        ;;
      *)
        cat << !

The installation code you have specified ($II_INSTALLATION) is invalid. 
The first character code must be a letter; the second may be a 
letter or a number.

!
	exit 1
        ;;
      esac

      cat << !

II_INSTALLATION configured as $II_INSTALLATION. 
!
     $DOIT ingsetenv II_INSTALLATION $II_INSTALLATION
   else
      get_installation_code

      #Check write to the response file 
      if $WRITE_RESPONSE ; then
         check_response_write II_INSTALLATION $II_INSTALLATION replace
      else 
         $DOIT ingsetenv II_INSTALLATION $II_INSTALLATION
      fi
   fi
fi

LOCATION_HELP=false

# skip location instructions prompt if possible
# ("DATA" set earlier on.)
CKP=`eval II_ADMIN=${symbloc} ingprenv II_CHECKPOINT`
JNL=`eval II_ADMIN=${symbloc} ingprenv II_JOURNAL`
DMP=`eval II_ADMIN=${symbloc} ingprenv II_DUMP`
WORK=`eval II_ADMIN=${symbloc} ingprenv II_WORK`
[ -z "$DATA" ] || [ -z "$CKP" ] || [ -z "$JNL" ] || [ -z "$DMP" ] || \
   [ -z "$WORK" ] &&
{
   $BATCH ||
   {
      cat << !

This program prompts you for the default physical storage locations you
want to use (if not already configured) for the database, backup, and
temporary work files maintained by the Ingres Intelligent DBMS.
Before you respond to these prompts, you have the option of reading detailed
instructions on configuring the default Ingres storage locations.

!
   }

   $BATCH || prompt \
      "Do you want instructions on configuring your storage locations?" y &&
   {
      LOCATION_HELP=true
      cat << !

Ingres Location Variables
-------------------------
The Ingres storage locations and the corresponding Ingres variables
are listed below:

    Ingres Variable       Description
    ---------------       -----------
    II_DATABASE           Default location for database files
    II_CHECKPOINT         Default location for full backup snapshots
    II_JOURNAL            Default location for journals (backup audit trails)
    II_DUMP               Default location for on-line backup change logs
    II_WORK               Default location for temporary files

!
      $BATCH || pause

      [ "$II_CLUSTER_ID" ] && cat << !
All Ingres storage locations except for II_WORK, must be accessible
from all nodes in the Ingres cluster.
!
      cat << !

Important Note
--------------
You cannot change the values you assign to the default Ingres
storage location variables once this setup procedure has been completed.
However you can create additional storage locations and assign
individual databases to them.  For information on creating additional
storage locations, refer to the Ingres Database Administrator's Guide.

!
      $BATCH || pause
   }
}
# location configuration help ends here

#
# get locations
#
for i in 1 2 3 4 5
do
   case "$i" in 

      1)
         $LOCATION_HELP &&
         {
           cat << !

The Default Database Location
-----------------------------
The Ingres variable II_DATABASE stores the default location used to 
store Ingres database files.

Once a value has been assigned to II_DATABASE, you can change it only
by removing and re-installing Ingres. Refer to the Ingres Database
Administrator's Guide for information on moving the default database
file location.

!
	    $BATCH || pause
         }
         LOCATION="II_DATABASE"
         LOCATION_TEXT="Ingres Database Files"
         ;;
 
      2)
         $LOCATION_HELP &&
         {
            cat << !

The Default Checkpoint, Journal and Dump Locations
--------------------------------------------------
The Ingres variables II_CHECKPOINT, II_JOURNAL, and II_DUMP are
used to store the default locations used for full and incremental
on-line backup files.  It is essential that II_CHECKPOINT, II_JOURNAL,
and II_DUMP not be assigned to the same physical device as
II_DATABASE.  If this restriction is ignored, you will be unable to
recover lost data in the event that the II_DATABASE location fails.

Once assigned, the values of II_CHECKPOINT, II_JOURNAL, and II_DUMP
can only be changed by removing and re-installing Ingres.  
Refer to the Ingres Database Administrator's Guide for
information on moving the default backup locations.

Refer to the chapter on "Backup and Recovery" in the Ingres
Database Administrator's Guide for instructions on backup and recovery
of Ingres databases.

!
	    $BATCH || pause
         }
         LOCATION="II_CHECKPOINT"
         LOCATION_TEXT="Ingres Checkpoint Files"
         ;;   

      3)
         LOCATION="II_JOURNAL"
         LOCATION_TEXT="Ingres Journal Files"
         ;;  

      4)
         LOCATION="II_DUMP"
         LOCATION_TEXT="Ingres Dump Files"
         ;;  

      5)
         $LOCATION_HELP &&
         {
            cat << !

The Default Work Location
-------------------------
The Ingres variable II_WORK stores the default location used for
all temporary files created during external sorts and other DBMS server
operations that require large amounts of temporary file space.
Ingres lets you designate a separate storage device as the default
location for these files.

When choosing the II_WORK location, be aware that temporary files tend
to increase fragmentation on storage devices, which can lead to
decreased performance.  For this reason, you should assign II_WORK to a
physical device which does not contain database, checkpoint, journal,
or dump files, if possible.  Your choice of the default work location
does not affect your ability to recover lost data in the event of a
storage device failure, although it may affect performance.

Once assigned, the value of II_WORK can only be changed by removing
and re-installing Ingres. Refer to the Ingres Database
Administrator's Guide for instructions on moving the default work location
in this manner.

!
	    $BATCH || pause
         }
         LOCATION="II_WORK"
         LOCATION_TEXT="Ingres Work Files"
         ;;  

   esac 
   while true
   do
      config_location "$LOCATION" "$LOCATION_TEXT" && break
      confirm_abort
   done
done  

#
# Reconfigure existing Ingres transaction log(s)
# If we've been through this already, not much happens.
#
LOGHOST="`node_name $FIRST_RAD`"
if $WRITE_RESPONSE ; then
    LOG_EXISTS=false	# Must set LOG_EXISTS to run through associated prompts.
else
    # Maybe we have a 2.5+ style log spec already
    II_LOG_FILE=`iigetres ii.$LOGHOST.rcp.log.log_file_1`
    II_LOG_FILE_NAME=`iigetres ii.$LOGHOST.rcp.log.log_file_name`
    II_DUAL_LOG=`iigetres ii.$LOGHOST.rcp.log.dual_log_1`
    II_DUAL_LOG_NAME=`iigetres ii.$LOGHOST.rcp.log.dual_log_name`
    LOG_STYLE=new
    RAW_LOG_KBYTES=
    export RAW_LOG_KBYTES
    if [ -z "$II_LOG_FILE" ] ; then

	# Look for old style ingprenv variables
	II_LOG_FILE=`ingprenv II_LOG_FILE`
	II_LOG_FILE_NAME=`ingprenv II_LOG_FILE_NAME`

	# Ingres 6.4 uses only II_LOG_FILE so II_LOG_FILE_NAME must
	# default to "ingres_log" explicitly.
	if [ -n "$II_LOG_FILE" -a -z "$II_LOG_FILE_NAME" ] ; then
	    II_LOG_FILE_NAME="ingres_log"
	fi

	if [ -n "$II_LOG_FILE" ] ; then
	    LOG_STYLE=old
	fi
  
	II_DUAL_LOG=`ingprenv II_DUAL_LOG`
	II_DUAL_LOG_NAME=`ingprenv II_DUAL_LOG_NAME`
    fi
  
    # Cluster log files are "decorated" with upper case node name
    LOGEXT="`gen_log_ext $FIRST_RAD`"

    if [ "$LOG_STYLE" = 'old' ] ; then
	# Get rid of old style logicals, stuff into config.dat
	# Rename the old style log file to the new style name.
	$DOIT iisetres ii.$LOGHOST.rcp.log.log_file_1 $II_LOG_FILE
	$DOIT iisetres ii.$LOGHOST.rcp.log.log_file_name $II_LOG_FILE_NAME
	$DOIT mv ${II_LOG_FILE}/ingres/log/${II_LOG_FILE_NAME} \
	  `gen_log_file_path "${II_LOG_FILE}" "${II_LOG_FILE_NAME}" 1 "$LOGEXT"` >/dev/null 2>&1
	$DOIT ingunset II_LOG_FILE_NAME
	$DOIT ingunset II_LOG_FILE
	if [ "$II_DUAL_LOG" ] && [ "$II_DUAL_LOG_NAME" ] ; then
	    $DOIT iisetres ii.$LOGHOST.rcp.log.dual_log_1 $II_DUAL_LOG
	    $DOIT iisetres ii.$LOGHOST.rcp.log.dual_log_name $II_DUAL_LOG_NAME
	    $DOIT mv ${II_DUAL_LOG}/ingres/log/${II_DUAL_LOG_NAME} \
	      `gen_log_file_path "${II_DUAL_LOG}" "${II_DUAL_LOG_NAME}" 1 "$LOGEXT"` >/dev/null 2>&1
	    $DOIT ingunset II_DUAL_LOG_NAME
	    $DOIT ingunset II_DUAL_LOG
	fi
    fi

    LOG_EXISTS=false
  
    # move rawlog.size to new location if it exists 
    sizefilename=`truncate_file_name "rawlog.size$LOGEXT"`
    [ -f $II_CONFIG/rawlog.size ] &&
    {
       $DOIT mv $II_CONFIG/rawlog.size $II_LOG_FILE/ingres/log/$sizefilename
    }

    if [ "$II_LOG_FILE" ] && [ "$II_LOG_FILE_NAME" ]
    then
	logfilename=`gen_log_file_path "$II_LOG_FILE" "$II_LOG_FILE_NAME" 1 "$LOGEXT"`
	if [ -f $logfilename -o -c $logfilename ]
	then
	   # primary transaction log exists - figure out its size
	   LOG_EXISTS=true
	   if [ -c $logfilename ] ; then
	      # log is a raw device
	      if [ -f $II_LOG_FILE/ingres/log/$sizefilename ] ; then 
		 # use size stored in rawlog.size
		 ## Not only is this faster than dd, but it preserves total
		 ## size knowledge for multiple log file partitions.
		 RAW_LOG_KBYTES=`cat $II_LOG_FILE/ingres/log/$sizefilename`
		 $DOIT iisetres ii.$LOGHOST.rcp.file.kbytes $RAW_LOG_KBYTES
	      fi
	      # use dd (later) to determine raw log size if RAW_LOG_KBYTES
	      # is undefined.
	   else
	      # log is not a raw device
	      [ -f $II_LOG_FILE/ingres/log/$sizefilename ] &&
	      {
		 # remove "decorated" $II_LOG_FILE/ingres/log/rawlog.size
		 $DOIT rm -f $II_LOG_FILE/ingres/log/$sizefilename
	      }
	      [ -f $II_LOG_FILE/ingres/log/rawlog.size ] &&
	      {
		 # remove $II_LOG_FILE/ingres/log/rawlog.size
		 $DOIT rm -f $II_LOG_FILE/ingres/log/rawlog.size
	      }
	   fi
	fi
    fi

fi #end WRITE_RESPONSE mode 

# use kbytes for log files
LOG_KBYTES=`iigetres ii.$CONFIG_HOST.rcp.file.kbytes`
LOG_BYTES=`iigetres ii.$CONFIG_HOST.rcp.file.size`
if [ "$LOG_BYTES" ]
then
    # Translate old file.size if log file has old style name, and if
    # there's no kbytes resource.  Ignore old file.size if there's a
    # kbytes resource around (could be rawlog size, or MDB setting.)
    if [ ! -f "$II_LOG_FILE/ingres/log/$II_LOG_FILE_NAME.l01" ] && \
         ( [ "$LOG_KBYTES" -le 0 ] || [ "$LOG_KBYTES" -eq "" ] )
    then
	if [ "$LOG_BYTES" -gt 0 ]
	then
	    LOG_BYTES=`expr $LOG_BYTES + 1023`
	    LOG_KBYTES=`expr $LOG_BYTES / 1024`
	    $DOIT iisetres ii.$CONFIG_HOST.rcp.file.kbytes $LOG_KBYTES
	    [ "$LOGHOST" = "$CONFIG_HOST" ] || \
	     $DOIT iisetres ii.$LOGHOST.rcp.file.kbytes $LOG_KBYTES
	fi
    fi
    $DOIT iiremres ii.$CONFIG_HOST.rcp.file.size
fi

if [ "$WRITE_RESPONSE" = "true" ] ; then
    # in Write response mode the config.dat may 
    # not exist so we prompt something to the user here.
    [ -z "$LOG_KBYTES" ] && LOG_KBYTES=262144
fi

#
# If logs not already set up by conversion of previous logs
#
if [ "$LOG_EXISTS" = "false" -o  $NEW_RADS -gt 1 ]
then
    if $LOCATION_HELP ; then 
	cat << !

Ingres Transaction Log Locations
--------------------------------
The Ingres Intelligent DBMS uses a transaction log file to store
uncommitted transactions and buffer committed transactions before they
are written to the database file location(s).

In order to guarantee that no committed transactions can be lost in
the event of a single storage device failure, a backup of the Ingres
transaction log should be maintained on a storage device which is
independent of the primary transaction log file device.

By default, Ingres will maintain a backup of the primary transaction
log file on a storage location which you select.

You have the option of disabling the backup transaction log file,
although you should only do so if the location you intend to use
for your primary transaction log file has built-in fault tolerance
(e.g. a fault-tolerant disk array or "mirrored" disk sub-system).

!
	$BATCH || pause
    fi

    #
    # Get information for configuring the transaction logs
    #
    if $NUMA_CLUSTER
    then
	#
	# Loop over each virtual node
	#
	for rad in $NEW_RAD_IDS
	do
	    if $LOG_EXISTS && [ "$rad" = "$FIRST_RAD" ]
	    then
		cat << !

Configuring RAD $rad to use existing transaction log files.

!
		CREATE=true
		export CREATE
		check_directory $II_LOG_FILE/ingres/log 700
		[ -n "$II_DUAL_LOG" ] && \
		 check_directory $II_DUAL_LOG/ingres/log 700
		continue
	    fi
	    cat << !

Configuring transaction logs for RAD $rad.

	${LOG_KBYTES}K bytes  (${approx}${LOG_MBYTES}M bytes)

!
	    configure_logs $rad
	    LOCATION_HELP=false
	done

	#
	# Display what was done, and confirm
	#
	cat << !

    Transaction log configuration specified

!
	for rad in $NEW_RAD_IDS
	do
	    LOGHOST=`node_name $rad`

	    II_LOG_FILE_NAME="`iigetres ii.$LOGHOST.rcp.log.log_file_name`"
	    LOGEXT="`gen_log_ext $rad`"

	    GETRES="`iigetres ii.$LOGHOST.rcp.file.kbytes`"
	    echo "RAD $rad\tlog size = `kbytes_to_mbytes $GETRES`"
		
	    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
	    do
		GETRES="`iigetres ii.$LOGHOST.rcp.log.log_file_${i}`"
		[ "$GETRES" ] || break;
		[ $i -eq 1 ] && echo "Primary log:"
		echo \
		"`gen_log_file_path $GETRES $II_LOG_FILE_NAME $i $LOGEXT`"
	    done

	    II_LOG_FILE_NAME="`iigetres ii.$LOGHOST.rcp.log.dual_log_name`"
	    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
	    do
		GETRES="`iigetres ii.$LOGHOST.rcp.log.dual_log_${i}`"
		[ "$GETRES" ] || break;
		[ $i -eq 1 ] && echo "Dual (Backup) log:"
		echo \
		"`gen_log_file_path $GETRES $II_LOG_FILE_NAME $i $LOGEXT`"
	    done
	    echo ""
	done

	while :
	do
	    prompt "Continue setup with above configuration?" y && break;
	    confirm_abort
	done
    else
	#
	# Just process one log
	#
	echo ""
	configure_logs
    fi
else
    CREATE=true
    export CREATE
    check_directory $II_LOG_FILE/ingres/log 700
fi

#
# get II_NUM_OF_PROCESSORS value
#
II_NUM_OF_PROCESSORS=`ingprenv II_NUM_OF_PROCESSORS`
if [ "$II_NUM_OF_PROCESSORS" ] ; then 
   cat << !

II_NUM_OF_PROCESSORS configured as $II_NUM_OF_PROCESSORS. 
!
else
   if $BATCH ; then
      if $READ_RESPONSE ; then
         II_NUM_OF_PROCESSORS=`iiread_response II_NUM_OF_PROCESSORS $RESINFILE`
      fi
      [ -z "$II_NUM_OF_PROCESSORS" ] && II_NUM_OF_PROCESSORS="2"
   else
      get_num_processors
   fi
fi

#Check write to the response file
if $WRITE_RESPONSE ; then
    check_response_write II_NUM_OF_PROCESSORS $II_NUM_OF_PROCESSORS replace
else
    $DOIT ingsetenv II_NUM_OF_PROCESSORS $II_NUM_OF_PROCESSORS 

    if [ "$II_NUM_OF_PROCESSORS" != "1" ] ; then
	$DOIT ingsetenv II_MAX_SEM_LOOPS 2000
    fi
fi

#
# get II_TIMEZONE_NAME value
#
[ -z "$II_TIMEZONE_NAME" ] && II_TIMEZONE_NAME=`ingprenv II_TIMEZONE_NAME`
if [ "$II_TIMEZONE_NAME" ] ; then 
   cat << !

II_TIMEZONE_NAME configured as $II_TIMEZONE_NAME. 
!
else
   TZ_RULE_MAP=$II_CONFIG/dbms.rfm
   if $BATCH ; then
      if $READ_RESPONSE ; then
         II_TIMEZONE_NAME=`iiread_response II_TIMEZONE_NAME $RESINFILE`
      fi
      [ -z "$II_TIMEZONE_NAME" ] && II_TIMEZONE_NAME="NA-PACIFIC"
   else
      get_timezone
   fi
fi

#Check write to the response file
if $WRITE_RESPONSE ; then
    check_response_write II_TIMEZONE_NAME $II_TIMEZONE_NAME replace
else
    $DOIT ingsetenv II_TIMEZONE_NAME $II_TIMEZONE_NAME
fi 

#
# get II_CHARSETXX value
#
if [ -z "$II_CHARSET" ] ; then
	VALUE=`ingprenv II_CHARSET''$II_INSTALLATION''`
else
	VALUE=$II_CHARSET
fi
if [ "$VALUE" ] ; then 
   cat << !

II_CHARSET$II_INSTALLATION configured as $VALUE. 
!
else
   if $BATCH ; then
      # batch mode default
      VALUE=ISO88591
      if $READ_RESPONSE ; then 
	 RSP_VALUE=`iiread_response II_CHARSET $RESINFILE`
         if [ ! -z "$RSP_VALUE" ] ; then
            if validate_resource ii."*".setup.ii_charset $RSP_VALUE
            then
               cat << !

  Characterset supplied ( $RSP_VALUE ) is illegal.
  Character set will be set to default [ $VALUE ]

!
            else
               VALUE=$RSP_VALUE
            fi
         fi
         cat << !

II_CHARSET$II_INSTALLATION configured as $VALUE. 
!
      fi
   else
      cat << !

Ingres supports different character sets.  You must now enter the
character set you want to use with this Ingres installation.

IMPORTANT NOTE: You will be unable to change character sets once you
make your selection.  If you are unsure of which character set to use,
exit this program and refer to the Ingres Installation Guide.
!
      # display valid entries
      iivalres -v ii."*".setup.ii_charset BOGUS_CHARSET
      DEFAULT="ISO88591"
      iiechonn "Please enter a valid character set [$DEFAULT] "
      read VALUE junk
      [ -z "$VALUE" ] && VALUE=$DEFAULT
      NOT_DONE=true
      while $NOT_DONE ; do
         if [ -z "$VALUE" ] ; then
            VALUE=$DEFAULT
         elif validate_resource ii."*".setup.ii_charset $VALUE
         then
            # reprompt
            iiechonn "Please enter a valid character set [$DEFAULT] "
            read VALUE junk
         else
            CHARSET_TEXT=`iigetres ii."*".setup.charset.$VALUE`
            cat << !

The character set you have selected is:

	$VALUE ($CHARSET_TEXT)

!
            if prompt "Is this the character set you want to use?" y
            then 
               NOT_DONE=false 
            else
               cat << !

Please select another character set.
!
               iivalres -v ii."*".setup.ii_charset BOGUS_CHARSET
               iiechonn "Please enter a valid character set [$DEFAULT] "
               read VALUE junk
            fi 
         fi
      done
   fi
fi

# If II_CHARSETxx is set to null, need to unset it.
if env | grep  "^II_CHARSET$II_INSTALLATION" > /dev/null ; then
eval "ii_charsetxx_value"="\$II_CHARSET$II_INSTALLATION"
[ -z "$ii_charsetxx_value" ] && eval unset "II_CHARSET$II_INSTALLATION"
fi  

#Check write to the response file 
if $WRITE_RESPONSE ; then
   check_response_write II_CHARSET $VALUE replace
   SETUP=defaults
else
   $DOIT ingsetenv II_CHARSET$II_INSTALLATION $VALUE
fi

if $WRITE_RESPONSE ; then 
   : # Do nothing if just creating the response files. 
else
   #
   # create quel and sql startup files if not found
   #
   [ -f ${cfgloc}/startup.dst ] &&
      [ ! -r ${cfgloc}/startup ] &&
   { 
      $DOIT cp ${cfgloc}/startup.dst ${cfgloc}/startup
   }
   [ -f ${cfgloc}/startsql.dst ] &&
      [ ! -r ${cfgloc}/startsql ] &&
   {
      $DOIT cp ${cfgloc}/startsql.dst ${cfgloc}/startsql
   }
       
   #
   # make and set permissions on shared memory directory
   #
   [ ! -d ${cfgloc}/memory ] &&
   { 
      $DOIT mkdir ${cfgloc}/memory
      $DOIT chmod 755 ${cfgloc}/memory
   }
       
   #
   # define NM symbols if defaulted but not defined in symbol.tbl 
   #
   for SYMBOL in II_TEMPORARY ING_EDIT 
   do
      DEFAULT=`iigetres ii."*".setup.$SYMBOL`
      VALUE=`ingprenv $SYMBOL`
      [ -z "$VALUE" ] &&
      {
	 if [ "$DEFAULT" ] ; then 
	    DEFAULT=`eval echo $DEFAULT`
	    $DOIT ingsetenv $SYMBOL $DEFAULT
	 else
	    cat << !
   
Error: $SYMBOL not defaulted in configuration rule base.
       
!
	    exit 1
	 fi
      }
   done
fi #End of check for WRITE_RESPONSE 

#
# set the number of concurrent users 
#
DEFAULT=`iigetres ii.$CONFIG_HOST.dbms."*".connect_limit`
if [ -n "$FROM64" ] ; then
    nn=`grep connected_sessions $II_CONFIG/rundbms.opt | $AWK '{print $2}'`
    if [ -n "$nn" -a "$nn" -gt 0 ] ; then
	DEFAULT=$nn
    fi
fi
if $BATCH ; then
    VALUE=$DEFAULT
    if $READ_RESPONSE ; then
	NEWVALUE=`iiread_response CONNECT_LIMIT $RESINFILE`
	if [ ! -z "$NEWVALUE" ] ; then
	    # 
	    # if syscheck fails we need to reconfigure unix kernal.
	    # we can abort setup or continue with defaults.
	    #
	    if validate_resource "ii.$CONFIG_HOST.dbms.\"*\".connect_limit" \
		$VALUE ; then
		cat << !
The number of concurrent users is not valid. The value for
concurrent users will be set to $DEFAULT
!
	    else
		$DOIT iisetres ii.$CONFIG_HOST.dbms."*".connect_limit $NEWVALUE
		if syscheck 1>/tmp/syscheck.$$ 2>/tmp/syscheck.$$ ; then
		    $DOIT rm -f /tmp/syscheck.$$
		    VALUE=$NEWVALUE
		else
		    cat << !
Warning!!..  The supplied number of concurrent users [ $NEWVALUE ] can not be set.
You should reconfigure your Unix kernel to setup the required number
of concurrent users before running setup. Number of users will be 
set to default [ $DEFAULT ].
!
		    $DOIT iisetres ii.$CONFIG_HOST.dbms."*".connect_limit $ VALUE
		fi
	    fi #end validation
	fi
    fi #end read response
else
    if [ "$SETUP" = "defaults" ] ; then 
	[ -z "$DEFAULT" ] && DEFAULT=32
	echo ""
	iiechonn "How many concurrent users do you want to support? [$DEFAULT] "
	VALUE=""
	read VALUE junk
	[ -z "$VALUE" ] && VALUE=$DEFAULT

	if $WRITE_RESPONSE ; then
	    check_response_write CONNECT_LIMIT $VALUE replace
	else
	    NOT_DONE=true
	    SYSCHECK_FAIL=false
	    while $NOT_DONE
	    do
		while $SYSCHECK_FAIL ||
		    validate_resource \
		     "ii.$CONFIG_HOST.dbms.\"*\".connect_limit"  $VALUE
		do
		    SYSCHECK_FAIL=false
		    iiechonn "Please enter another number of concurrent users: "
		    read VALUE junk
		    [ -z "$VALUE" ] && VALUE=$DEFAULT
		done

		cat << !

Updating configuration...
!
		$DOIT iisetres ii.$CONFIG_HOST.dbms."*".connect_limit $VALUE

		syscheck 1>/tmp/syscheck.$$ 2>/tmp/syscheck.$$
		if [ $? -lt 2 ] ; then
		    $DOIT rm -f /tmp/syscheck.$$
		    NOT_DONE=false
		    SYSCHECK_FAIL=false
		else
		    cat /tmp/syscheck.$$
		    $DOIT rm -f /tmp/syscheck.$$
		    SYSCHECK_FAIL=true
		    cat << !
If Ingres is configured for the minimum number of concurrent users,
you should abort this procedure in order to reconfigure your Unix kernel
before attempting to complete this setup procedure.

If you choose not to enter another number of concurrent users, the setup
procedure will be aborted.

!
		    if prompt \
		    "Do you want to enter another number of concurrent users?" \
		      y ; then 
			echo ""
		    else
			exit 1
		    fi
		fi
	    done
	fi #end WRITE_RESPONSE mode
	DEFAULT=$VALUE
    else
	VALUE=`iigetres ii.$CONFIG_HOST.dbms."*".connect_limit`
    fi
fi

cat << !

Ingres is configured to support $VALUE concurrent users.
!

if $WRITE_RESPONSE ; then
    :	# Do nothing if setting up a response file. 
else
    if [ "$SETUP" = 'defaults' ] ; then
	# Remember that we completed #-of-users assignment
	$DOIT iisetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID users 
	SETUP=users
    fi
   
    if [ "$SETUP" = 'users' ] ; then
	#
	# Create/erase the transaction logs
	#
	if $NUMA_CLUSTER
	then
	    #
	    # Loop over each virtual node
	    #
	    for rad in $NEW_RAD_IDS
	    do
		cat << !

Setting up transaction log(s) for RAD $rad.
!
		setup_transaction_logs $rad
	    done
	else
	    #
	    # Just process one set of logs for box.
	    #
	    setup_transaction_logs
	fi

	# Turn off remote-command servers until we can get it set up
	# or db's upgraded
	RMCMD_SERVERS=`iigetres ii.$CONFIG_HOST.ingstart."*".rmcmd`
	[ -z "$RMCMD_SERVERS" ] && RMCMD_SERVERS=0
	if [ $RMCMD_SERVERS -ne 0 ] ; then
	    $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".rmcmd 0
	fi
	$DOIT iisetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID txlogs 
	SETUP=txlogs
    fi
fi #end of WRITE_RESPONSE mode   

#
# get date_type_alias value
#
if [ "$II_DATE_TYPE_ALIAS" ] ; then
   cat << !

II_DATE_TYPE_ALIAS configured as $II_DATE_TYPE_ALIAS.
!
else
   if $BATCH ; then
      if $READ_RESPONSE ; then
         II_DATE_TYPE_ALIAS=`iiread_response II_DATE_TYPE_ALIAS $RESINFILE`
      fi
      [ -z "$II_DATE_TYPE_ALIAS" ] && II_DATE_TYPE_ALIAS="ingresdate"
   else
      get_date_type_alias
   fi
fi

#Check write to the response file
if $WRITE_RESPONSE ; then
    check_response_write II_DATE_TYPE_ALIAS $II_DATE_TYPE_ALIAS replace
else
    $DOIT iisetres ii.$CONFIG_HOST.config.date_alias $II_DATE_TYPE_ALIAS
fi

#
# If this is a cluster install, and this is not the first node,
# and we are not generating a response file, then we are done.
#
if $WRITE_RESPONSE ; then
    :	# Do nothing if setting up a response file. 
else
    if [ -n "$FIRSTNODE" ] ; then
	GETRES=`iigetres ii.$FIRSTNODE.config.dbms.$RELEASE_ID`
	if [ "$GETRES" = "complete" ] ; then
	    $DOIT iisetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID "complete"
	    cat << !
Ingres DBMS setup complete for this cluster node.

Refer to the Ingres Installation Guide for information about
starting and using Ingres.
!
	    clean_up
	    exit 0
	fi
    fi
fi

RAD_ARG=""
$NUMA_CLUSTER && 
{
    #
    # Set default RAD for all downstream operations
    #
    II_DEFAULT_RAD=$FIRST_RAD
    export II_DEFAULT_RAD
    RAD_ARG="-rad=$FIRST_RAD"
}

# determine whether this is an upgrade
NONFATAL_ERRORS=
IMADB_ERROR=
II_DATABASE=`ingprenv II_DATABASE`
if [ -d $II_DATABASE/ingres/data/default/iidbdb ] ; then
    # Looks like an upgrade
    if $WRITE_RESPONSE ; then
	:	# Do nothing if setting up a response file. 
    else
        if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	    # skip this for LSB builds, not relavent
	    # and problematic
	    :
	else
	    cat << !
   
Removing defunct utilities and files...
!
	    # Check for upgrade from pre r3 system that had enhanced security
   	    if [ -f $II_SYSTEM/ingres/utility/slutil ] ; then
		cat << ! | warning_box
You are upgrading an Ingres installation that had Ingres Enhanced
Security installed.  The current version of Ingres does not have
Enhanced Security so this feature will be removed.
!
	    fi

	    # remove defunct utilities and files
	    oldfiles="\
	 ask chk5to6 chkcaps chkingdir chkins chkins.orig chklocation \
	 clientchk configabf configdblocs configdbms diskfree diskinuse dlist \
	 echolog echon fixmode getdescrip getlocation getprime iiarchive \
         iiauthadd iiauthbase iibuild iiclient iimakelog iirungcc iirungcn \
         iishutdown iistartup.orig ilist ingprenv1 mkclientdir mkclientdir.b \
         mkmakelog mkrcpconfig mkrundbms mkrungcc mkrunstar plist rcheck \
         readdist rmsrvr runiinamu runrundbms seting showrcp sockcheck \
         sysdepvar ingchkdate iisues iisetlab slutil"
	    cd $II_SYSTEM/ingres/utility
	    $DOIT rm -f $oldfiles
	    $DOIT ingunset II_AUTHORIZATION
	    $DOIT ingunset II_AUTH_CODE
	    $DOIT ingunset II_RUN
	    $DOIT ingunset II_TEMPLATE 
        fi 
	# upgrade the system catalogs and optionally all databases 
   
	$BATCH || cat << !
  
Before you can access the existing databases in this Ingres instance, they
must be upgraded to support the new release of the Ingres server which
you have installed.  You have the option of upgrading all the databases
contained in this Ingres instance now.  Alternately, you can upgrade them
later by hand using the "upgradedb" utility.
 
Be aware that the time required to upgrade all local databases will
depend on their size, number, and complexity; therefore it is not possible
to accurately predict how long it will take.  Also note that the Ingres
master database (iidbdb) will be upgraded at this time whether or not
you choose to upgrade your databases.
 
If you are installing a Service Pack or release that does not require
database upgrading, you may answer "no" to this question.  The upgrade
utility will still be run against the master database to verify its
release level.

!
	UPGRADE_DBS=false
	def=y
	if [ -n "$FROM64" ] ; then
	    # Default to NO for 6.4, because if there are star db's, we
	    # need to go thru Star setup and we haven't done that yet!
	    def=n
	fi

        if $READ_RESPONSE ; then
	    upgrade=`iiread_response II_UPGRADE_USER_DB $RESINFILE`
	    case $upgrade in
		[Yy][Ee][Se]|\
		[Tt][Rr][Uu][Ee]) UPGRADE_DBS=true
			;;
	    esac
	fi

	$BATCH ||
	{
	    if prompt "Upgrade all databases in this Ingres instance now?" $def
	    then
		UPGRADE_DBS=true
	    else
		UPGRADE_DBS=false
	    fi
	}
   
	# Note stdout append here since "command2" might run first
	rm -f $II_LOG/upgradedb.log
	if $UPGRADE_DBS ; then
	    COMMAND="upgradedb -all >>$II_LOG/upgradedb.log 2>&1"
	    COMMAND2="upgradedb iidbdb >>$II_LOG/upgradedb.log 2>&1"
	else
	    COMMAND="upgradedb iidbdb >>$II_LOG/upgradedb.log 2>&1"
	fi

	# See if there's already an imadb.  "Real" imadb's are in the
	# default data location.
	CREATE_IMADB=Y
	if [ -z "$FROM64" -a -d $II_DATABASE/ingres/data/default/imadb ] ; then
	    CREATE_IMADB=
	fi

	# Set startup count for all "com" server to zero for the duration
	# of the upgrade and save current values
	quiet_comsvrs


	wostar=
	if [  $STAR_SERVERS -ne 0 ] ; then
		wostar=' without Star'
        fi
	if [ "$UPGRADE_DBS" = 'true' ] ; then
	    cat << ! 

Starting the Ingres server$wostar to upgrade all system and user databases...
!
	else
	    cat << ! 

Starting the Ingres server$wostar to upgrade system databases...
!
	fi

	memsize=`iishowres`

	$DOIT ingsetenv II_LG_MEMSIZE $memsize
	$DOIT ingstart $RAD_ARG ||
	{
	    $DOIT ingstop >/dev/null 2>&1
	    cat << ! | error_box
The Ingres server could not be started.  See the server error log ($II_LOG/errlog.log) for a full description of the problem. Once the problem has been corrected, you must re-run $SELF before attempting to use the Ingres instance.
!
	     $DOIT ingunset II_LG_MEMSIZE
	     exit 1
	}

	if [ "$UPGRADE_DBS" = "true" ]
	then
	    echo "Upgrading Ingres master and system databases..."
	    if $DOIT eval $COMMAND2 ; then
              if [ $STAR_SERVERS -ne 0 ] ; then
	 	cat << !

Shutting down the Ingres server...
!
		$DOIT ingstop >/dev/null 2>&1
		$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star $STAR_SERVERS
		cat << ! 

Starting the Ingres servers including Star to upgrade user databases...
!
		$DOIT ingstart $RAD_ARG ||
		{
		    $DOIT ingstop >/dev/null 2>&1
		    cat << ! | error_box
The Ingres server could not be started.  See the server error log ($II_LOG/errlog.log) for a full description of the problem. Once the problem has been corrected, you must re-run $SELF before attempting to use the Ingres instance.
!
		    $DOIT ingunset II_LG_MEMSIZE
		    exit 1
		}
              fi 
	    else
		# Error upgrading iidbdb pre-all
		cat << ! | error_box
A system database could not be upgraded.  A log of the attempted upgrade can be found in:

	$II_LOG/upgradedb.log

The problem must be corrected before the installation can be used.

Contact Ingres Corporation Technical Support for assistance.
!
		$DOIT ingstop >/dev/null 2>&1
                if [ $STAR_SERVERS -ne 0 ] ; then
	    	  $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star $STAR_SERVERS
                fi  
		$DOIT ingunset II_LG_MEMSIZE
		exit 1
	    fi
	else
          if [ $STAR_SERVERS -ne 0 ]
          then
              $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star $STAR_SERVERS
          fi
        fi

	if $UPGRADE_DBS ; then
	    echo "Upgrading all system and user databases..."
	else
	    echo "Upgrading Ingres master and system databases..."
	fi
	if $DOIT eval $COMMAND ; then
	    # FIXME - sleep to stop DB lock race condition
	    sleep 1
	    $DOIT sysmod +w iidbdb
	    cat << !

Checkpointing Ingres master database...
!
	    # FIXME - sleep to stop DB lock race condition
	    sleep 1
	    $DOIT ckpdb +j iidbdb -l +w > \
	      $II_LOG/ckpdb.log 2>&1 ||
	    {
		cat << ! | warning_box
An error occurred while checkpointing your master database.  A log of the attempted checkpoint can be found in:

$II_LOG/ckpdb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
		NONFATAL_ERROR=Y
	    } 

	    # Create imadb if needed.
	    # upgradedb is now kind enough to upgrade imadb when iidbdb
	    # is upgraded.
	    if [ -n "$CREATE_IMADB" ] ; then
	        cat << !

Creating imadb...
!
		if $DOIT $createdb -fnofeclients \
		   -u${DIX}\$ingres imadb ; then
		    : ok
		else
		    cat << ! | warning_box
An error occurred creating the imadb database.  Standard IMA registrations and Visual DBA Remote Command services will be unavailable until the imadb is created.  The installation process will continue.

Contact Ingres Corporation Technical Support for assistance.
!
		    IMADB_ERROR=Y
		    NONFATAL_ERROR=Y
		fi
	    fi
	    # Remove VDBA catalogs from iidbdb if they might
	    # have been there (2.0, 2.5).
	    # Remove old VDBA catalogs from imadb if we upgraded it.
	    # Note: I'm not entirely happy about the latter, but it's
	    # something of a no-win situation.  If the user had fancy
	    # non-ingres privs on the rmcmd stuff, those privs will
	    # have to be re-done after the upgrade.
	    # We could skip the rmcmdrmv/gen if and only if we know
	    # that rmcmd didn't change.  And I don't know that. (schka24)
	    if [ -x $II_SYSTEM/ingres/utility/rmcmdrmv ] ; then
		UPG25=`grep 'config.dbms.ii25' ${cfgloc}/config.dat \
			| awk '{print $2}'`
		UPG20=`grep 'config.dbms.ii20' ${cfgloc}/config.dat \
			| awk '{print $2}'`
		if [ -n "$UPG25" -o -n "$UPG20" ] ; then
		   cat << !

Removing VDBA catalogs from iidbdb...
!
		   $DOIT rmcmdrmv -diidbdb 
		fi
		# Remove old VDBA catalogs from imadb
		if [ -z "$IMADB_ERROR" -a -z "$CREATE_IMADB" ] ; then
		    $DOIT rmcmdrmv
		fi
	    fi
	    # re-registration of imadb stuff done below

	    ## upgradedb is now kind enough to auto upgrade icesvr and
	    ## other ice installation db's when iidbdb is upgraded.

	else
	    # upgradedb on iidbdb or on all failed
	    UPGRADE_ERROR=Y
	    NONFATAL_ERROR=Y
	    if $UPGRADE_DBS ; then
		cat << ! | error_box
An error occurred while upgrading your system databases or one of the user databases in this Ingres instance.  A log of the attempted upgrade can be found in:

$II_LOG/upgradedb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
	     else
		cat << ! | error_box
An error occurred while upgrading your system databases.  A log of the attempted upgrade can be found in: 

$II_LOG/upgradedb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
	    fi
	fi

	# Set startup counts back to original values
	restore_comsvrs

    fi #end WRITE_RESPONSE mode
else
    $BATCH ||
    {
	# prompt whether to enable ANSI/ISO Entry SQL-92 compliance 
	# Since this is apparently an install, not upgrade, just go ahead
	# and ask even if it's a retry.
	cat << !

The Ingres DBMS can be configured to be strictly compliant with the ANSI/ISO
entry-level SQL-92 standard.  If you need information about this standard,
please refer to the Ingres SQL Reference Manual.

Strict ANSI/ISO SQL-92 compliance causes some Ingres features to
operate in a non-default mode, such as uppercase identifiers and
subquery semantics changes.  Strict compliance should only be
selected if you are sure that you require it.

!
	prompt "Do you need strict compliance to the ANSI/ISO standard?" n &&
	{
	    if $WRITE_RESPONSE ; then
		check_response_write SQL92 ON replace
	    else
		$DOIT iisetres ii.$CONFIG_HOST.fixed_prefs.iso_entry_sql-92 ON
	    fi
	}
    }

    if $READ_RESPONSE ; then
	SQL92resp=`iiread_response SQL92 $RESINFILE`
        # check for II_ENABLE_SQL92 to
	[ -z "$SQL92resp" ] && \
		SQL92resp=`iiread_response II_ENABLE_SQL92 $RESINFILE`
	if [ "$SQL92resp" = "ON" ] || [ "$SQL92resp" = "YES" ] ; then 
	    $DOIT iisetres ii.$CONFIG_HOST.fixed_prefs.iso_entry_sql-92 ON
	fi
    fi

    if $WRITE_RESPONSE ; then
	:	# Do nothing if setting up a response file. 
    else
	# Make sure we don't start any "com" servers, but store existing
	# values
	quiet_comsvrs

	#
	# create the system catalogs
	cat << ! 
      
Starting the Ingres server to initialize the master database...
!
	$DOIT ingstart $RAD_ARG ||
	{
	    $DOIT ingstop >/dev/null 2>&1
	    cat << ! | error_box
The Ingres server could not be started.  See the server error log ($II_LOG/errlog.log) for a full description of the problem. Once the problem has been corrected, you must re-run $SELF before attempting to use the Ingres instance.
!
	    exit 1
	}

	cat << !

Initializing the Ingres master database, iidbdb...

!
	if $DOIT $createdb -S iidbdb ; then
	    # FIXME - sleep to stop DB lock race condition
	    sleep 1
	    $DOIT sysmod +w iidbdb
	    echo "Checkpointing the Ingres master database..."

	    # FIXME - sleep to stop DB lock race condition
	    sleep 1
	    $DOIT ckpdb +j iidbdb -l +w > \
	      $II_LOG/ckpdb.log 2>&1 ||
	    {
		cat << ! | warning_box
An error occurred while checkpointing your master database.  A log of the attempted checkpoint can be found in:

$II_LOG/ckpdb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
		NONFATAL_ERROR=Y
            } 

	    cat << !

Creating imadb...
!
	    if $DOIT $createdb -fnofeclients \
		   -u${DIX}\$ingres imadb ; then
		: ok
	    else
		IMADB_ERROR=Y
		NONFATAL_ERROR=Y
	    fi

	else  #createdb -S
	    II_CHECKPOINT=`ingprenv II_CHECKPOINT`
	    II_DUMP=`ingprenv II_DUMP`
	    II_JOURNAL=`ingprenv II_JOURNAL`
	    II_WORK=`ingprenv II_WORK`
	    $DOIT rm -rf $II_DATABASE/ingres/data/default/iidbdb
	    $DOIT rm -rf $II_CHECKPOINT/ingres/data/default/iidbdb
	    $DOIT rm -rf $II_DUMP/ingres/data/default/iidbdb
	    $DOIT rm -rf $II_JOURNAL/ingres/data/default/iidbdb
	    $DOIT rm -rf $II_WORK/ingres/work/default/iidbdb
	    cat << ! | error_box
The master database was not created successfully.   You must correct this before this Ingres instance can be used.
!
	    cat << !
Shutting down the Ingres server...

!
	    $DOIT ingstop >/dev/null 2>&1
	    exit 1 
	fi
	# Restore "com" server startup counts
	restore_comsvrs
    fi #end of WRITE_RESPONSE mode 
fi

if [ "$WRITE_RESPONSE" = 'false' ] ; then
    # Not writing response file, finish up by doing stuff common to
    # upgrade and install: init imadb.

    if [ -z "$IMADB_ERROR" ] ; then
	cat << !

Initializing imadb for IMA and rmcmd...
!
	if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	    vdbadir=/usr/share/ingres/vdba
	    vdbalogdir=$II_LOG
	else
	    vdbadir=$II_SYSTEM/ingres/vdba
	    vdbalogdir=$vdbadir
	fi
	$DOIT sql -u${DIX}\$ingres imadb < \
		  ${vdbadir}/makimau.sql \
		  > ${vdbalogdir}/mkimau.out 2>&1

	if [ -f $II_SYSTEM/ingres/utility/rmcmdgen ] ; then
	    $DOIT rmcmdgen
	fi
	# Restore remote command server (turned off for setup)
        [ -z "$RMCMD_SERVERS" ] && RMCMD_SERVERS=0
        if [ $RMCMD_SERVERS -ne 0 ] ; then
	    $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".rmcmd 1
        fi
	# FIXME - sleep to stop DB lock race condition
	sleep 1
	sysmod +w imadb
    fi

fi

# Create and load demo databases if we need to
CREATE_DEMODB=NO
DEMODB_ERROR=''
$READ_RESPONSE && \
    CREATE_DEMODB=`iiread_response II_DEMODB_CREATE $RESINFILE`

if [ -d "$II_DATABASE/ingres/data/default/demodb" ] && [ "$UPGRADE_DBS" = "false" ] ; then
    cat << !
Upgrading  demodb...

!
    upgradedb demodb
    CREATE_DEMODB="YES"
else
    $BATCH ||
    {
        if [ -d "$II_DATABASE/ingres/data/default/demodb" ]; then
            CREATE_DEMODB="YES"
        else
	    cat << !
This setup process can create and populate a demonstration database (demodb)
which will be used by the Ingres demonstration applications.

!

            prompt "Do you want demodb to be created?" y  &&
                CREATE_DEMODB="YES"
        fi
        $WRITE_RESPONSE &&
            check_response_write II_DEMODB_CREATE $CREATE_DEMODB replace
    }

fi

if [ "$WRITE_RESPONSE" = 'false' ] ; then
    # create demodb if it doesn't exist
    if [ "$CREATE_DEMODB" = "YES" ] ; then
        if [ -d "$II_SYSTEM/ingres/data/default/demodb" ] ; then
            # turn off journaling before data is reloaded in existing db
	    $DOIT ckpdb -w -j demodb
	    echo
        else
	    cat << !
Creating demodb...

!
	    $DOIT $createdb -i demodb ||
	    {
	        DEMODB_ERROR=Y
	        NONFATAL_ERROR=Y
	        cat << ! | error_box
An error occurred creating database 'demodb'. As this database is used by some of the Ingres demonstration applications they may not function correctly until this issue has been resolved.

Contact Ingres Corporation Technical Support for assistance.
!
	    }
        fi
    fi

    # load data
    if [ "$CREATE_DEMODB" = "YES" ] && [ -z "$DEMODB_ERROR" ] ; then
	cat << !

Populating demodb...

!
	if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	    demodir=/usr/share/ingres/demo/data
	    demologdir=/var/log/ingres/demo
	else
	    demodir=$II_SYSTEM/ingres/demo/data
	    demologdir=$demodir
	fi
	[ -f "${demodir}/drop.sql" ] &&
	    [ -f "${demodir}/copy.in" ] &&
	    ( $DOIT cd ${demodir} ;
		$DOIT sql demodb < ${demodir}/drop.sql >> \
				${demologdir}/copyin.log ;
		$DOIT sql demodb < ${demodir}/copy.in >> \
				${demologdir}/copyin.log ) ||
		{
		    DEMODB_ERROR=Y
		    NONFATAL_ERROR=Y
		    cat << ! | error_box
An error occurred loading 'demodb'. As this database is used by some of the Ingres demonstration applications they may not function correctly until this issue has been resolved.

Contact Ingres Corporation Technical Support for assistance.
!
		}
    fi
	
    # now sysmod and checkpoint demodb and we're done
    if [ "$CREATE_DEMODB" = "YES" ] && [ -z "$DEMODB_ERROR" ] ; then
	sleep 1
	$DOIT sysmod -w demodb ||
	{
	    DEMODB_ERROR=Y
	    NONFATAL_ERROR=Y
	}
    fi
 
    if [ "$CREATE_DEMODB" = "YES" ] && [ -z "$DEMODB_ERROR" ] ; then
	sleep 1
	$DOIT ckpdb -w +j demodb ||
	{
	    DEMODB_ERROR=Y
	    NONFATAL_ERROR=Y
	}
    fi
# Now we're done with everything we can shutdown the server
    cat << !

Shutting down the Ingres server...
!
    $DOIT ingstop >/dev/null 2>&1

    # Check to see if we've beed passed any config to supply
    # For now this is only intented to be used by the GUI installer
    # so we don't need to worry about write_response.
    CONFIG_TYPE=''
    $READ_RESPONSE && \
	CONFIG_TYPE=`iiread_response II_CONFIG_TYPE $RESINFILE`
    if [ "$CONFIG_TYPE" -a "$CONFIG_TYPE" != "TRAD" ] ; then
	$DOIT iiapplcfg -t $CONFIG_TYPE ||
	{
	    NONFATAL_ERROR=Y
	}
        # Check and report on any potential resource issues after
	# new config has been applied
	syscheck 1>/tmp/syscheck.$$ 2>/tmp/syscheck.$$
	if [ $? -gt 1 ] ; then
	    cat /tmp/syscheck.$$
	    NONFATAL_ERROR=Y
	fi
    fi

    if [ -z "$NONFATAL_ERRORS" ] ; then
	cat << !

Ingres Intelligent DBMS setup complete.
!
    else
	cat << !

Ingres Intelligent DBMS setup completed, with errors.

Some errors were encountered creating, checkpointing, or upgrading
one or more databases.  The installation will be marked "complete",
but the errors should be reviewed and corrected before attempting to
use this Ingres instance.  
!
    fi
    $DOIT ingstop -f >/dev/null 2>&1
    cat << !

Refer to the Ingres Installation Guide for information about
starting and using Ingres.
      
!
    $DOIT iisetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID complete
    remove_temp_resources
    $BATCH || pause
fi # write-response
   
clean_up 
exit 0
}


trap "clean_up ; exit 1" 0 1 2 3 15

$DOIT iisulock "Ingres Intelligent DBMS setup" || exit 1

eval do_iisudbms $INSTLOG
trap : 0
