/*
** Copyright (c) 1992, 2010 Ingres Corporation
**
*/

/*
**  Name: ingstart.c -- a (more) portable startup program for OpenIngres
**
**  Usage:
**	ingstart [
**		-iigcn |
** 		-iigws |
**		-dmfrcp |
**		-dmfacp |
**              -rmcmd  |
**              -icesvr |
**              -iigcd |
**		-{iidbms|iigcc|iigcb|iistar|oracle|informix|mssql|sybase|odbc|rms|rdb|db2udb}[=name]
**	  	 ] 
**		 [ -rad=radid | -cluster | -node=nodename1... ]
**
**  Description:
**	ingstart first calls a program which checks for sufficient
**	system resources required to run OpenIngres as configured then
**	tries to start the entire installation based on the configuration
**	stored in the default configuration resource file (config.dat),
**	unless instructed to start an individual process by a command
**	line flag.
**
**	ingstart can also be configured to run any number of other programs
**	before and/or after the standard OpenIngres processes are launched.
**	config.dat entries of the form "ii.*.ingstart.%.begin" can be
**	used to specify programs that are to be run before the standard
**	processes.  Entries of the form "ii.*.ingstart.%.end" can be used
**	to specify programs that are to be run after the standard processes.
**	ingstart will exit with failure status if any program specified
**	in this manner exits with failure status.
**
**  Exit Status:
**	OK	installation startup succeeded.
**	FAIL	installation startup failed.	
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	18-jan-93 (tyler)
**		Ported to VMS.
**	16-feb-93 (tyler)
**		Fixed various bugs in VMS-specific code and added command 
**		line option to allow one DBMS, Net or Star server to be
**	 	started.	
**	1-mar-93 (tyler)
**		VMS logicals now get set in executive (not superviror)
**		mode.  Also moved sys$setprv() call before the first
**		PCcmdline() call to ensure that the sub-process created
**		by PCcmdline() has the new privilege set.
**	15-mar-93 (tyler)
**		Added command-line option to restart the archiver process.
**		Also fixed bug introduced in the last revision which
**		caused sys$setprv() to be called before the "privilege mask"
**		was set up. 
**	15-mar-93 (tyler)
**		Added correct VMS command line to start a Net server. 
**	25-mar-93 (tyler)
**		Removed call to ingsysdef.com and added code which deletes
**		II_SYSTEM from the JOB logical table before trying to start
**		anything.
**	26-apr-93 (tyler)
**		Added code to invert certain boolean flags.
**	24-may-93 (tyler)
**		Added cluster support and argument to PMmInit().
**	21-jun-93 (tyler)
**		Enhanced VMS cluster support and improved code which 
**		starts processes individually.  Switched to new interface
**		for multiple context PM functions.  Added code which
**		scans and passes DMF parameters.  Fixed broken passing
**		of "facility" parameters on VMS.  Removed CS and LG 
**		dependencies.	
**	23-jul-93 (tyler)
**		Added check for ii.$.prefs.syscheck_ingstart.  Fixed
**		incorrect testing of STbcompare() return status.
**		PMmInit() interface changed.  PMerror() is now passed
**		to PMmLoad() for better error reporting. 
**	26-jul-93 (tyler)
**		Removed dead code and fixed message formatting.
**	04-aug-93 (tyler)
**		Removed more dead code.  Resource names in warning message
**		regarding missing command line mappings is now displayed
**		in lower case.
**	20-sep-93 (tyler)
**		Removed /image flag from Star server command line on VMS.
**		Fixed bug which prevented supervisor-mode II_SYSTEM logical
**		from being deassigned before startup.  VMS logical
**		names are now forced to upper case before creation.
**		Fixed broken Star server "facility" parameter passing.
**	24-sep-93 (tyler)
**		Fixed bug which caused iirundbms command line on VMS
**		not to work with certain combinations of settings.
**	19-oct-93 (tyler)
**		Call PMhost() instead of GChostname().  Stopped
**		constructing old-style command line for iirundbms.
**		Define VMS logicals in EXECUTIVE_MODE.  Changed names
**		of resources which control syscheck behavior.
**	21-oct-93 (tyler)
**		Include the installation code on the iirundbms command
**		line so that it shows up in the process list.  Also
**		pass "(default)" instead of "*" for default servers
**		so that "*" is not interpreted as a wildcard by the
**		execution shell.
**	22-nov-93 (tyler)
**		Fixed output formatting.
**	17-dec-93 (joplin)
**		Put the value for II_INSTALLATION as the last parameter
**	05-jan-94 (tyler)
**		Display correct configuration name when starting Net
**		server.
**	31-jan-94 (tyler)
**		Don't bother supporting "/" option specifier on VMS. 
**		Also switched from "INGRES" to "OpenINGRES".
**	22-feb-94 (tyler)
**		Addressed SIR 58906.  ingstart will now launch programs
**		specified in config.dat as ii.*.ingstart.%.begin and
**		ii.*.ingstart.%.end, before and after OpenINGRES
**		processes are started, respectively.  Also, moved
**		messages to erst.msg where they belong.  Save a copy
**		of II_INSTALLATION value returned by NMgtAt().
**      08-Apr-94 (michael)
**              Changed references of II_AUTHORIZATION to II_AUTH_STRING where
**              appropriate.
**	23-may-94 (joplin)
**		Changed VMS startup of the gcc and gcn servers to use
**		the iirundbms utility.  This allows the gcc and gcn
**		servers to use the VMS quotas defined in the PM file.
**	31-Jan-1995 (canor01)
**		B63013: put maximum loop time on waiting for dmfcsp
**		to report startup in case it started and then died
**	20-mar-1995 (canor01)
**		B67637: for UNIX, check logstat to see if system running.
**      02-apl-1995 (chech02)
**              B67860: checks the auth string before starts the name string.
**	11-jul-1995 (sarjo01)
**		NT: Enabled ping_iigcn() to wait for gcn before starting
**		gcc. Replace exec of ipcclean, csinstall with deletion of
**		files in II_SYSTEM\ingres\files\memory.
**	13-jul-1995 (sarjo01)
**		NT: removed II_AUTH_STRING checking 
**	20-jul-1995 (sarjo01)
**		NT: Fixed broken #ifdef NT_GENERIC in start_logging()
**	26-jul-95 (tutto01)
**		Added additional sleep for NT logging system.
**	28-jul-95 (tutto01)
**		NT version now uses ipcclean.
**	03-aug-1995 (canor01)
**		NT version checks for return value from ipcclean to
**		verify that the installation is not running.
**	04-aug-1995 (canor01)
**		NT: call to ipcclean is OK, but csinstall is for 
**		Unix only.
**	09-aug-1995 (reijo01)
**		Added a sleep between polling iigcn before iigcc startup.
**		Also corrected a logic bug in ping_iigcn where if the first
**		request failed subsequent calls to ping_iigcn would always
**		fail.
**	23-aug-95 (emmag)
**		Changed OpenINGRES to OpenIngres.
**	05-sep-95 (emmag)
**		Print out successfull completion message on NT.  This
**		fixed message string is required by NT utilities.
**	19-oct-95 (emmag)
**	        Add a -client option for NT which will start just
**		a name server and comms server.  This is required
**		to enable us to start up a client service using a
**		single command.
**	06-dec-95 (emmag)
**		NT:  Issue an error message if the installation is
**		already running as is done on UNIX.
**	03-jan-1996 (canor01)
**		NT:  Delay after star server startup to allow 2-phase
**		commit to complete.
**	15-jan-1996 (sweeney)
**		Make Joe Reilly's 1.2 fix to ping_iigcn generic --
**		this code was clearly intended to work everywhere.
**		#include <gcnint.h> and <qu.h> to pick up GCN_RESTART_TIME
**		so we don't have to hard-code 10L as a manifest constant.
**		Be sure to take the companion change to mkming.sh to look
**		in common/gcf/hdr for include files.
**	17-jan-1996 (sweeney)
**		Part of the change above was lost in the X-integration
**		from ca-oping11->main->ca-oping12. Redo by hand.
**	19-jan-1996 (sweeney)
**		It isn't an error for Ingres to be up if we're explicitly
**		starting an individual server. Check argument before test
**		to see if Ingres is running.
**      19-jan-1996 (sweeney)
**		Add check_gcn flag, set to true if we're starting the
**		iigcn, either directly or as part of the whole installation.
**		Also, fix x-integration bodge. The ping_iigcn code was
**		#ifdef DOESNT_WORK in oping11 but #ifdef NT_GENERIC here,
**		and NT got both versions.
**	23-jan-1996 (sweeney)
**		Bah. Override short-circuit evaluation. Also, fix timeslip
**		above.
**	15-Feb-1996 (wonst02)
**		95:  Start a "local Gateway server" for OpenINGRES Desktop
**		if configured.
**	19-feb-1996 (emmag)
**		Fix previous submission, which didn't compile.
**	02-apr-1996 (emmag)
**		When starting a client installation on NT, give the
**		name server sufficient time to initialize before starting
**		the comms server.
**	16-may-96 (emmag)
**		When starting a client installation on a slow PC, wait to 
**		give the GCN time to initialize it's pipes before starting
**		the comms server. (BUG 76437)
**	12-jul-96 (hanch04)
**		Changed S_ST0529_iigws_failed to S_ST0531, conflict renamed.
**		Changed S_ST0530_starting_iigws to S_ST0532, conflict renamed.
**	06-sep-96 (mcgem01)
**		The starting of the gateway service should be a stealth
**		operation; remove the messages except in case of failure.
**	07-sep-96 (mcgem01)
**		OpenIngres Desktop is being ported to Windows NT and should
**		behave exactly as it does on '95.   
**	13-sep-96 (mcgem01)
**		More OpenIngres desktop changes to support a -desktop
**		flag in ingstart.
**      02-Apr-1996 (rajus01)
**              Added capability to start Protocol Bridge Server.
**	10-oct-1996 (canor01)
**		Replace hard-coded references to 'ii' with SystemCfgPrefix.
**	28-oct-1996 (hanch04)
**		Added missing tmp_buff
**	04-dec-1996 (reijo01)
**		Change to use generic system variables where applicable and
**		ifdefed JASMINE where it wasn't.
**	23-dec-1996 (reijo01)
**		Updated all ingstart to SystemExecName"start" and "ii" calls
**		in start_iigcn, start_iigcb, start_dbms to use SystemCatPrefix.
**		Changed starting message (OpenIngres/ingstart) to be generic.
**		i.e. (Jasmine/jasstart) for Jasmine and (CA-OpenIngres/ingstart)
**		for ingres.
**	07-jan-1997 (chech02)
**	        Failed to define logical names in VMS.
**	25-mar-97 (mcgem01)
**		On NT and '95 when the GUI's are used to start the
**		installation, the customer may think ingstart has
**		hung, because the output isn't being flushed.
**	09-may-97 (mcgem01)
**		Replaced hard-coded reference to OpenIngres with 
**		SystemProductName.
**	13-may-97 (mcgem01)
**		The use of scrapers to display the output of console
**		based applications in a GUI type environment on NT
**		requires that all output gets flushed.  
**	14-may-97 (funka01)
**		Updated to use no options for jasstart if compiled with
**		JASMINE flag.
**	04-jun-97 (mcgem01)
**		Replace hard-coded reference to ingstart with argv[0].
**		Also changed the usage message on NT when running
**		Jasmine, to match that on Solaris.
**	20-aug-97 (zhayu01)
**		B79148. Do not start iigws for Desktop if it is not installed.
**      01-Oct-97 (fanra01)
**              Add rmcmd for VDBA to the startup.
**	04-oct-97 (rosga02)
**		Fix timing problem for VAX VMS.
**		Increase waiting time for RCPSTAT to get fired first time
**		to make sure RCP has already created a shared memory segment
**		by itself, to prevent E_CL1221_ME_NO_SUCH_SEGMENT error in RCP.
**	23-nov-97 (mcgem01)
**		Print a blank line after starting iigcb on NT as well as UNIX.
**	09-mar-1998 (somsa01)
**		In start_dmfrcp(), if the user starting Ingres is not "ingres",
**		rcpstat will fail since the OpenIngres service is not started.
**		Therefore, wait a little longer and then proceed without
**		re-running rcpstat. If there was a problem, then the DBMS
**		server won't come up anyway.
**	24-mar-98 (mcgem01)
**		Add the gateways as startup options.
**	16-may-1998 (canor01)
**		Genericize ST0540 message.
**	04-jun-98 (mcgem01)
**		If running the Workgroup Edition of Ingres II, refuse
**		to start a second DBMS server.
**	09-jun-1998 (hanch04)
**		We don't use II_AUTH_STRING for Ingres II
**	29-Jun-1998 (thaal01)
**		For VMS we need a delay during RCP startup. Originally this was
**		for VAX only, but this has been experienced on Alpha also. 
**		I suppose it depends how much shared memory is needed ! 
**		Bug 86579.
**	05-aug-1998 (toumi01)
**		Do wait_iigcn before bringing up the recovery server, else
**		we might fail because gcn isn't ready yet.
**	26-Aug-1998 (hanch04)
**		Made rmcmd startup generic.
**	31-Aug-98 (mcgem01)
**		Check the license string before attempting to start the
**		logging system, to avoid the check II_SYSTEM etc. 
**		message appearing in Ingres II.
**      04-Sep-98 (fanra01)
**              Add starting of ice server.
**      01-sep-1998 (hweho01)
**              For DG/UX 88K r4.11MU04 (dg8_us5), in order to run the IIGCB
**              (Protocol Bridge Server) successfully, need a delay during
**              IIGCC startup.
**	16-sep-1998 (kinte01)
**		Add VMS specific info for starting RMCMD process
**	14-oct-98 (mcgem01)
**		Fix a typo in the usage message.
**      26-Nov-1998 (fanra01)
**              Changed icesvr usage to single instance only.
**	30-nov-1998 (mcgem01)
**              Increase the time allocated for name server initialization.
**      07-dec-1998 (chash01)
**              Start rmcmd with "iirundbms rmcmd" on vms.
**	29-dec-1998 (canor01)
**		Add siteid license validation check.
**	18-jan-1999 (cucjo01)
**              Fixed compiler warnings for unreferenced variables
**              Removed obsolete function ule_format that was not being called
**              Added log file 'ingstart.log' to ingres\files directory
**	08-feb-1999 (mcgem01)
**		Print a successful startup message for client installation
**		also, so that winstart recognises that the installation has
**		been started.
**	30-mar-1999 (somsa01)
**		Localized any strings which were left.
**	20-Sep-1999 (bonro01)
**		Modifications needed for Ingres clusters.
**	30-mar-1999 (mcgem01)
**		Replace SystemCatPrefix with SystemAltExecName as 
**		SystemCatPrefix is the prefix for the system catalogs and
**		not the prefix used for distinguishing binaries.
**	14-jun-1999 (somsa01)
**		On NT, if the Ingres service is started, then start whatever
**		pieces of Ingres we are told to start through the service
**		via PCexec_suid().
**	01-apr-1999 (rigka01) - bug #96164
**		ingstart -client changed so that it starts up as many iigcc
**		processes as in Startup Count in CBF
**      24-may-99 (chash01)
**          Add ifdef for starting VMS version oracle gateway.
**      04-jun-99 (chash01)
**          to start oracle gateway, call iigwstart first, which in turn calls
**          iirundbms (redundant?)
**      24-may-99 (chash01)
**          Add ifdef for starting VMS version oracle gateway.
**      04-jun-99 (chash01)
**          to start oracle gateway, call iigwstart first, which in turn calls
**          iirundbms (redundant?)
**	04-aug-99 (kinte01)
**	    add start of RMS Gateway (sir 98235) & RDB Gateway (SIR 98243)
**	03-sep-1999 (somsa01)
**	    In ping_iigcn(), we would not issue a GCA_TERMINATE to the Name
**	    server after a successful ping. This could cause subsequent
**	    calls to the Name server to not work.
**	09-sep-1999 (somsa01)
**	    Minor modification to last change.
**	29-sep-99 (i4jo01)
**	    Added fix to flush out symbol table to ensure we read gcn symbols
**	    in wait_iigcn. (b98648)
**	04-nov-1999 (toumi01)
**		Skip edit for Workgroup Edition of Ingres II start of second
**		server on Linux.  The function needed to implement this is
**		not available on this platform.
**	05-nov-99 (mcgem01)
**	    Move the successful startup message for a client installation
**	    to outside the for loop to avoid the message being duplicated.
**	22-jan-2000 (somsa01)
**	    On NT, we now always start Ingres via the service.
**	27-jan-2000 (somsa01)
**	    Added a "-service" option for NT, which is used if the service
**	    is running 'ingstart'. Also, when printing out
**	    S_ST0655_start_svc_fail, print out the OS error text.
**	28-jan-2000 (somsa01)
**	    Fix up printout when service is already started.
**	30-jan-2000 (mcgem01)
**	    Change the meaning of the service flag so that it can be used
**	    to start Ingres as a service from the command line.
**	10-feb-2000 (somsa01)
**	    Print out the success startup message on UNIX as well. Also,
**	    when starting just the dmfrcp, run csinstall as well. Also,
**	    print out more messages to the ingstart.log file.
**      22-feb-2000 (rajus01)
**              Added capability to start JDBC Server.
**	30-mar-2000 (somsa01)
**	    Changed S_ST0660_jdbc_failed to S_ST066A_jdbc_failed.
**	21-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products. Also, added usage printout for other products.
**      03-may-2000 (mcgem01)
**          Flush ingstart.log file to disk.
**	03-may-2000 (somsa01)
**	    In start_iigcc(), halved time to wait before starting a GCC
**	    on NT.
**	14-jun-2000 (mcgem01)
**	    The product specific service name is <product>_Client_<inst id>
**	17-Jul-2000 (hanje04)
**	    Skip WORKGROUP specific change to see if icesvr is running.
**	    Function (PCget_PidFromName) needed to do this is not supported
**	    on Linux
**	19-jul-2000 (somsa01)
**	    On NT, run 'ipcclean' first before starting anything. This is
**	    so that, if we start the NT service through here, 'ipcclean'
**	    will not fail due to the service installing its own shared
**	    memory.
**	24-jul-2000 (somsa01)
**	    Amended the previous change to only execute ipcclean if the
**	    logging system is not online.
**	26-jul-2000 (somsa01)
**	    When starting the GCC or GCB servers, use 'iirundbms'.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	16-aug-2000 (hanje04)
**	    Added ibm_lnx to platforms skipping WORKGROUP specific check for 
**	    ice server.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-sep-2000 (somsa01)
**	    When starting the JDBC server, use 'iirundbms'.
**	14-sep-2000 (somsa01)
**	    We only want to execute 'rcpstat' if the DBMS is installed.
**	22-dec-2000 (somsa01)
**	    In start_dmfrcp(), corrected printout of
**	    S_ST0540_cannot_run_rcpstat.
**      02-nov-1999 (kinte01)
**          With new definiton of F_ERROR macro with 18-jan-1999 (cucjo01)
**          change # 439924, buffer1 was not defined and the definition of
**          buffer was missing a semicolon.
**      28-Dec-2000 (horda03)
**          Modified thaal01's change for Bug 86579; it introduced a 60 second
**          delay. Now use appropriate function to determine when RCP server
**          is present and has initialised LGK memory.
**	06-mar-2001 (kinte01)
**	    Add VMS specific changes for starting the JDBC server
**	06-jun-2001 (somsa01)
**	    In start_iigcb(), slightly cleaned up printout.
**	13-sep-2001 (somsa01)
**	    Enabled the usage of 'ingstart -service -client' .
**	25-oct-2001 (somsa01)
**	    Changed "int_wnt" to "NT_GENERIC".
**	28-jun-2001 (devjo01)
**	    Replace fixed 60 second delay when starting in clustered
**	    mode with a polling loop checking for CSP on-line.  Enable
**	    ersatz_V|P for non-UNIX platforms.   s103715.
**	23-jul-2001 (devjo01)
**	    Initial changes for UNIX custer support (s103715).
**	01-nov-2001 (devjo01)
**	    If licensing not installed issue a warning.
**	    Add VMS specific changes for started the JDBC server
**	05-nov-2001 (kinte01)
**	    For ingstart.log use TR_F_APPEND on VMS as well to keep all
**	    ingstart information in a single file otherwise hundreds of
**	    one line ingstart.log files are created and they are essentially
**	    useless. (bug 106289)
**	06-nov-2001 (kinte01)
**	    Back on 10 feb 2000, somsa01 added a call to ipclean & csinstall
**	    before starting dmfrcp ifndef NT_GENERIC.  These routines do
**	    not exist on VMS so this needs to be ifdef'ed accordingly
**	14-jan-2002 (somsa01)
**	    Make sure we "break" after case's 2 and 3 for argc to prevent
**	    "ingstart -help" frm SEGV'ing.
**	17-jan-2002 (somsa01)
**	    Added a new flag, "-insvcmgr", to be used when being run via the
**	    Windows Service Manager. Also, if "-insvcmgr" is passed or the
**	    Ingres service is started, do not print
**	    S_ST0540_cannot_run_rcpstat when necessary.
**	31-jan-2002 (devjo01)
**	    Add -cluster, and -node options.  Rework parameter parsing to
**	    accomidate an arbitrary number of -node parameters.
**	22-feb-2002 (toumi01)
**	    For Unix, make sure before we do an ipcclean that there are
**	    no active servers already attached.  If this situation is found,
**	    abort the ingstart with error S_ST0672_shared_mem_in_use.
**	19-mar-2002 (kinte01)
**	    add ifndef around non-VMS header files
**	29-mar-2002 (somsa01)
**	    In Execute(), do not use PCexec_suid() for 'rcpstat' on Windows,
**	    as 'rcpstat' is itself set to use PCexec_suid() as appropriate.
**	15-apr-2002 (devjo01)
**	    Disable -cluster, and -node options, until problem with rsh
**	    hanging until remote daemon processes (in this case dbms servers)
**	    exit.
**	20-may-2002 (kinte01)
**	    ingstart -gateway does not work if change 439134 was applied.
**	    Change 439134 modified CBF to use a new format for the gateways
**	    ii.hostname.gateway.specific-gw.  The addition of the word
**	    gateway caused ingstart to generate the message "No definition
**	    found for specified configuration name." (b99420)
**	14-jun-2002 (mofja02)
**	    Changes to add DB2 UDB Gateway.
**	12-jul-2002 (mofja02)
**	    Changes to add DB2 UDB Gateway to individually start db2 udb
**		(bb99420).
**	26-sep-2002 (devjo01)
**	    Support for NUMA clusters.
**	18-feb-2003 (abbjo03)
**	    Make comparison in check_mem_name case-insensitive so that it will
**	    work on ODS-5 disks.
**      24-Feb-2003 (wansh01)
**          changes to support iigcd DAS server
**      14-Mar-03 (gordy)
**          Don't need (and shouldn't use) gcnint.h.
**	02-Apr-03 (gupsh01)
**	    Modified the usage message for ingstart. Added section for VMS.
**	    The gateway options rms and rdb are only valid on VMS. The odbc
**	    option is not supported on windows. Also the informix, sybase, 
**	    and DB2UDB options are not supported on VMS.
**      30-Apr-2003 (chash01)
**          The startup for iigcd for VMS was missing a quote
**	16-sep-2003 (abbjo03)
**	    Remove duplicate loop to define the VMS logicals using both
**	    wildcard (*.lnm) and node-specific syntax ($.lnm) since the latter
**	    covers both.
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	23-oct-2003 (devjo01)
**	    Fix conflict with 'mask' declaration conflict with identically named
**	    variable declared within VMS only scope.
**	05-nov-2003 (kinte01)
**	    Remove start_dmfcsp since DMFCSP is no longer a separate process
**	    due to the Portable Cluster Project 
**	25-nov-2003 (somsa01)
**	    On Windows, removed the sleep while waiting for the GCC to come up.
**	25-jan-2004 (devjo01)
**	    Correct "-service" and "-client" parameter processing broken by
**	    me back at rev 138.
**	08-Mar-2004 (hanje04)
**	    SIR 107679
**	    Improve syscheck functionality for Linux:
**	    Define more descriptive error codes so that ingstart can make a 
**	    more informed decision about whether to continue or not. (generic)
**	24-mar-2004 (somsa01)
**	    Corrected "-insvcmgr" parameter processing.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Safer env var handling (Windows).
**	23-Jun-2004 (drivi01)
**	    Replaced SystemProduct_name with SystemProductName.
**	13-Aug-2004 (hanje04)
**	    Stop root from starting installation. First part of full fix
**	    Will replace SIprintf() with correct error message. Need "quick
**	    fix" for CCS group.
**	19-Aug-2004 (hanje04)
**	    Allow root start if ii.$.ingstart.root.allowed = true.
**	    Add S_ST06B0_no_root_start for trying to run ingstart as root.
**	25-aug-2004 (penga03)
**	    Corrected "-client" parameter processing.
**	17-sep-2004 (somsa01)
**	    Updated dmfrcp starting on Windows: any administrator can now
**	    start Ingres.
**      17-sep-2004 (jammo01)bug#111941 prob#2785
**          Check to see if recovery server is already running before starting
**          a recovery server.Message if recovery server already running.
**	23-Sep-2004 (hanje04)
**	    Replace STcompare call with <NULL> test when checking root start.
**	    Add "new line" in NO_ROOT_START macro.
**	07-Oct-2004 (drivi01)
**	    Changed "rl.h" to <rl.h> reference and moved header into front/st/hdr.
**	04-Nov-2004 (hanch04)
**	    Reworked the wait_iigcn loop so that it won't hang forever.
**      18-Nov-2004 (ashco01)
**          Problem: INGNET148 (Bug: 113490).
**          Corrected call of 'ingstart' when starting IIGCC with named
**          configuration (instance) so that the correct IIGCC configuration
**          is read. 
**	27-Jan-2005 (hanje04)
** 	    Make sure rootok in "don't start as root" block is NULL before 
**	    we test it. DOH!
**	11-Feb-2005 (devjo01)
**	    Periodically double check that RCP hasn't crashed/exitted while
**	    polling for the CSP to come on-line.
**	11-Apr-2005 (devjo01)
**	    - Avoid calling iirun directly since iirundbms performs the
**	    useful task of redirecting standard in & standard out away
**	    from the terminal, allowing the terminal to be closed after
**	    performing an 'ingstart'.  
**	    - Re-enable -cluster, and -node options.
**	12-Apr-2005 (penga03)
**	    Added code to make "ingstart -service" cluster-aware.
**	02-May-2005 (drivi01)
**	    Fixed startup of iigcn and rmcmd servers.
**	02-May-2005 (fanch01)
**	    Adjust cx_nodes[] indexing to be zero based to agree with
**	    cx_nodes definition.
**	04-May-2005 (devjo01)
**	    - Tweaked drivi01's correction to my fix of 11-Apr, to restore
**	    use of iirundbms for gcn & rmcmd for UNIX.
**	24-may-2005 (devjo01)
**	    Replaced usage of PMm functions with PM equivalents.  This
**	    was required because calling PMmGet after PMmInit without
**	    calling PMinit prior to the PMmGet call will cause the default
**	    parameter context to be initialized as empty.  If other
**	    facilities (e.g. CX) query the default context, they will
**	    not find the expected data.  Flaw is really in PM, but was
**	    addressed here since there would be no benefit in maintaining
**	    two contexts.
**	02-Aug-2005(drivi01)
**	    Modified ipcclean call on windows, if ipcclean fails on the first
**	    try we'll retry 10 times, in case if ntrcpst call right before 
**	    ipcclean is taking longer than usual to cleanup.
**	26-oct-2005 (devjo01) b75599
**	    Address ancient problem where a failed ingstart on one node
**	    would hang ingstarts on all other nodes until someone manually
**	    removes the CLUSTER.LCK file.
**	17-nov-2005 (devjo01) b115591
**	    Shorten cluster startup time for VMS by allowing RCP/CSP
**	    processes to start in parallel on all the nodes.  The same
**	    mechanism that prevents multiple nodes from performing 
**	    incompatible operations for the Linux cluster will also
**	    prevent this on VMS.  Ersatz_V & Ersatz_P retained solely
**	    for any user scripts that may be registered.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**      13-Feb-2006 (loera01) Bug 115533
**          In start_iigcd(), added missing logic to allow startup of named
**          instances of the DAS server.
**      29-Aug-2006 (rajus01) Problem # INGNET 177, Bug #114791.
**	    Cross integrating change for Bug 114791 from ingres!26 codeline.
**	    Problem overview:
**	    When NET Server runs out of file descriptors (i.e. file descriptor
**	    hard limit is set to a low value compared to number of configured 
**	    NET sessions (using NET Server configuration parameters: 
**	    inbound_limit, outbound_limit ), it results in:
**	       a. Network listen (GCC TL layer) failure (Issue 14228541) thus
**	          causing hanging of remote connections.
**	          Error messages: E_GC2813_NTWK_LSN_FAIL(Network Listen Failed)
**	          ,  E_CLFE04_BS_ACCEPT_ERR (Unable to accept incoming 
**	          connection; System communication error: Bad file number.)
**	       b. GCA listen (GCC AL layer) failure (Issue 14433453) thus 
**	          filling up the errlog.log with the same error message that 
**	          eventually results in disk space issues.
**	          Repeated error: E_CLFE04_BS_ACCEPT_ERR (Unable to accept 
**	          incoming connection; System communication error: Bad file 
**	          number.)
**	    In both the cases above the NET server requires re-start. The 
**	    customer encountered the first problem. Later, when level 1 tested,
**	    the second problem was identified.
**	    There is no real good solution to this complex problem other than
**	    restarting the server.
**
**	    Changes made:
**	      Added file descriptor hard limit check for NET Server. A warning
**	      message is issued if file descriptor hard limit is less than the
**	      required limit which is calculated as (ib_limit+ob_limit)*2 + 5.
**	      Each NET session requires two file descriptors. The extra 5
**	      descriptors are added to handle logging, Name Server bed checking 
**	      etc., 
**
**	    Before finalizing on the above change, possibility of dynamically
**	    increasing the file descriptor hard limit while starting NET server 
**	    (just to make it transparent to the user) was investigated. 
**	    Even though it is feasible, the problem with this approach is that, 
**	    only the 'root' user can change the file descriptor hard limit. 
**
**	    Added write_to_errlog to log the warning message in the
**	    errlog.log as per the Alex's request. The format of the error
**	    the error message is: 
**          <HOSTNAME>::[INGSTART          , 00000000]:<ERROR_MESG>
**
**	    20-Jun-2006 (horda03) Problem # INGNET 177, Bug #114791.
**          Modified the above change as it used atol and 
**	    iiCL_get_fd_table_size(). iiCL_get_fd_table_size() is an internal
**	    function of the CL (in handy) and shouldn't be used in any other 
**	    facility. As this function isn't a defined interface of the CL, 
**	    it isn't available on VMS nor Windows.
**	    Temporary fix is to include the above change on UNIX only.
**	29-Aug-2006 (rajus01) Bug #114791
**	    Had to replace the PMm calls to PM specific ones for the above
**	    cross-integration fix for 114791.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	14-Feb-2007 (bonro01)
**	    Remove JDBC package.
**	25-Jul-2007 (drivi01)
**	    Updated ingstart to only run on Vista if it's executed by
**	    user with elevated privileges.  Force ingres to always
**	    start as a service on Vista.
**      28-Aug-2007 (ashco01) Bug #113490 & Bug #119021.
**          Corrected call of 'iirundbms' when starting GCB & GCD with
**          named configuration (instance) so that the correct configuration
**          is read and started.
**	04-Dec-2007 (drivi01)
**	    Updated previous change to show a warning on Vista asking user to
**	    start ingres as a service instead of starting it as a service
**	    automatically. This causes problems during fasti installation. 
**	07-jan-2008 (joea)
**	    Discontinue use of cluster nicknames.
**	04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    DBA Tools service was renamed to Ingres_DBATools_<instance id>
**	    and this file was updated to check for this service name
**	    if Ingres service isn't found.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      11-Dec-2008 (hanal04) Bug 121353
**          Add iigcd as a valid ingstart parameter on Windows.
**	01-May-2009 (drivi01)
**	    Move up clusapi.h header include to the top of the
**          header list to avoid errors due to undefine of a datatype.
**	    Cleanup warnings in efforts to port to Visual Studio
**          2008 compiler.
**	29-Sep-2009 (frima01)
**	    Add include of unistd.h to avoid warnings from gcc 4.3
**	02-Oct-2009 (bonro01) 122490
**	    unistd.h is not available on Windows.
**      23-nov-2009 (horda03) Bug 122555
**          On VMS, if a process (typically IPM) is connected to the LGLK shared
**          memory segment, then the INGSTART will hang following an attempt to
**          start the DMFRCP server. On VMS a Global (Shared) Memory section will
**          persist until the last processes attached to it disconnects, there is
**          no means to "blow away" the memory. Now, if INGSTART detects that there
**          are sessions connected to the LGLK memory (and it's starting the DMFRCP)
**          then by defining a logical GBL$II_<inst_id>_lglkdata.mem" to a unique
**          value will ensure that the new installation won't reference the old.
**	20-Jan-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	23-Sep-2010 (bonro01)
**	    Remove -icesvr from ingstart
*/ 

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# ifdef NT_GENERIC
# include <clusapi.h>
# endif
# ifndef NT_GENERIC
# include <unistd.h>
# endif /* NT_GENERIC */
# include <me.h>
# include <lo.h>
# include <pc.h>
# include <si.h>
# include <cv.h>
# include <nm.h>
# include <er.h>
# include <qu.h>
# include <gc.h>
# include <gcccl.h>
# include <gca.h>
# include <gcn.h>
# include <cs.h>
# include <ep.h>
# ifndef VMS
# include <clconfig.h>
# include <csev.h>
# include <cssminfo.h>
# endif
# include <cm.h>
# include <st.h>
# include <pm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
# include <erst.h>
# include <ci.h>
# include <id.h>
# include <tr.h>
# include <lk.h>
# include <cx.h>
# include <rl.h>

# ifdef VMS
# include <ssdef.h>
# include <descrip.h>
# include <efndef.h>
# include <iledef.h>
# include <iosbdef.h>
# include <lnmdef.h>
# include <psldef.h>
# include <starlet.h>
# include <jpidef.h>
# include <exhdef.h>
# endif /* VMS */

#if !defined(int_lnx) && !defined(int_rpl) && !defined(ibm_lnx)
#ifdef WORKGROUP_EDITION
FUNC_EXTERN PID PCget_PidFromName(PTR rmcmd);
# endif
# endif /* !int_lnx && !int_rpl && !ibm_lnx */

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

# ifdef WARNING
# undef WARNING
# endif
# define WARNING( MSG ) \
    { \
        char buffer [BIG_ENOUGH]; \
        STprintf( buffer, ERx( "\n%s\n\n" ), MSG); \
        SIfprintf( stderr, buffer ); \
        SIflush( stderr ); \
        write_to_log( buffer ); \
    }

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

# ifdef NO_ROOT_START
# undef NO_ROOT_START
# endif
# define NO_ROOT_START \
	{ \
	    SIprintf( ERget( S_ST06B0_no_root_start ) ) ; \
	    SIprintf( "\n" ) ; \
	    SIflush( stdout ); \
	    PCexit( FAIL ) ; \
	}

/*
PROGRAM =	(PROG2PRFX)start
**
NEEDLIBS =	UTILLIB GCFLIB COMPATLIB MALLOCLIB GLLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# ifndef E_CL2662_CI_BADSITEID
# define E_CL2662_CI_BADSITEID        (E_CL_MASK + 0x2662)
# endif

# ifdef VMS
static char *uic;	/* uic id used to launch VMS processes */
# endif /* VMS */

static char	ii_installation[16];	/* installation code */
static char	cmd[ BIG_ENOUGH ];	/* system command buffer */

static i4   log_file_len;                 /* log file length */
static char log_file_name[ MAX_LOC + 1 ]; /* log file name   */
static char log_file_path[ MAX_LOC + 1 ]; /* log file location */

static i4   Clustered = 0;		/* <> 0 if clustered Ingres */
static i4   NUMAclustered = 0;		/* <> 0 if local host NUMA clustered */
static i4   NUMAsupported = 0;		/* <> 0 if host supports NUMA */
static i4   RadID = 0;			/* Desired Resource Affinity Domain */
static i4   Have_Sysmem_Seg = 0;   /* <> 0 if parent instance of
					   ingstart already called csinstall. */
static char *RshUtil = NULL;		/* Remote shell utility to use. */
static char *ProgName;			/* Name of this util. */
static CX_CONFIGURATION *ClusterConfiguration;
static u_i4 NodeMask = 0, RemoteMask = 0; /* Bit mask of all/remote nodes
					   to be processed. */

/*
** If not configured for NUMA clusters, and NUMA is  not supported,
** "Affine_Always" will be left empty, and "Affine_Some" and
** "Affine_Clust" will point to empty strings.
** 
** If NUMA clusters are configured, "Affine_Always" will have the
** '-rad <radid>' value required to establish the RAD context,
** and "Affine_Clust" and "Affine_Some" will point to "Affine_Always".
** 
** If not configured for NUMA clusters, and OS/Ingres provides
** NUMA support, and if ingstart is not starting a single
** component, then "Affine_Always" is formatted with the affinity
** argument, but "Affine_Some" is left pointing to an empty string.
** Those components which benefit most from affinity take their
** affinity argument directly from "Affine_Always", while the others
** take it from "Affine_Some", so only some components will have 
** RAD affinity set.
** 
** If OS/Ingres provides NUMA support and only one component is
** being started, then "Affine_Always" is formatted for the requested
** RAD, and "Affine_Some" points to "Affine_Always", to make sure the
** users explicit wish is obeyed.
*/
static char	Affine_Always[16] = { '\0', }; /* NUMA RAD affinity arg. */
static char	*Affine_Some = "";
static char	*Affine_Clust = "";

/* Product component masks */
#define	PCM_DBMS	(1<<0)
#define PCM_GCC		(1<<1)
#define PCM_GCB		(1<<2)
#define PCM_JDBC	(1<<3)
#define PCM_GCN		(1<<4)
#define PCM_STAR	(1<<5)
#define PCM_RCP		(1<<6)
#define PCM_ACP		(1<<7)
#define PCM_ICE		(1<<8)
#define PCM_ORACLE	(1<<9)
#define PCM_INFORMIX	(1<<10)
#define PCM_SYBASE	(1<<11)
#define PCM_RMCMD	(1<<12)
#define PCM_RMS		(1<<13)
#define PCM_RDB		(1<<14)
#define PCM_DB2UDB	(1<<15)
#define PCM_MSSQL	(1<<16)
#define PCM_ODBC	(1<<17)
#define PCM_IIGWS	(1<<18)
#define PCM_GWS		(1<<19)
#define PCM_GCD		(1<<20)

#define PCM_ALL_RAD_SPECIFIC  (PCM_DBMS|PCM_RCP|PCM_ACP|PCM_STAR|PCM_ICE)

# ifdef VMS
/* Check if LGLK memory has been created */

int lgk_mem_ready = 0;                  /* true when RCP has initialised LGK memory */

STATUS
check_mem_name(PTR *arg_list, char *buf, CL_SYS_ERR *err_code)
{
   /* Is this for the LGLK memory segment ? */

   if (STbcompare("LGLKDATA.MEM", STlength("LGLKDATA.MEM"), buf, STlength(buf), TRUE))
      return OK;

   /* Indicate that the LGK memory segment has been created */

   lgk_mem_ready = 1;

   return ENDFILE;
}
#endif

# ifdef NT_GENERIC
static bool	ServiceStarted = FALSE;	/* If the NT service is started. */
static bool	in_svcmgr = FALSE; /* We're running via the Service Manager. */
# endif /* NT_GENERIC */

# ifdef UNIX
static i4 checkcsclean( void );
static i4 sysstat( void );
# endif /* UNIX */

/*
**	Forward declarations.
*/
static void add_to_node_mask( char *, char *, u_i4 *, u_i4 * );
STATUS	NMflushIng();
int	GVvista();


static STATUS
write_to_log( char *string)
{   STATUS status;
    i4  flag;
    CL_SYS_ERR sys_err;
    
    /* Open II_CONFIG!ingstart.log.  Append to the end of existing
    ** log file on UNIX & VMS.
    */
    flag = TR_F_APPEND;

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

static VOID
write_to_errlog ( char *msg )
{
 
 # define PROCNAME "INGSTART"
 # define SESSID	 0x0
 
     CL_ERR_DESC erdesc;
     char   msgbuf[ER_MAX_LEN];
     STprintf( msgbuf, "%-8.8s::[%-18s, %08.8x]: %s",
                 PMhost(), PROCNAME, SESSID, msg );
     TRdisplay("%s\n", msgbuf );
     ERlog(msgbuf, (i4)STlength( msgbuf ), &erdesc );
 
     return;
}

/*
**  Execute a command, blocking during its execution.
*/
static STATUS
execute( char *cmd )
{
	CL_ERR_DESC err_code;

#ifdef NT_GENERIC
	if (ServiceStarted && !STrstrindex(cmd, "rcpstat", 0, FALSE))
	    return ( PCexec_suid(cmd) );
	else
#endif
	return( PCcmdline( (LOCATION *) NULL, cmd, PC_WAIT,
		(LOCATION *) NULL, &err_code) );
}


/*
**  Remotely Execute a command, blocking during its execution.
*/
static STATUS
rpc_execute( i4 nodenum, char *cmd )
{
    CL_ERR_DESC		 err_code;
    char		*valp, *rhost;
    char		 buf[BIG_ENOUGH];
    char		 radspec[16];
    CX_NODE_INFO	*pni;

#ifdef NT_GENERIC
    return FAIL;
#endif
#ifdef VMS
    return FAIL;
#endif
    
    pni = ClusterConfiguration->cx_nodes + (nodenum - 1);

    if ( 0 == pni->cx_host_name_l )
	rhost = pni->cx_host_name;
    else
	rhost = pni->cx_node_name;

    /* Get network name of remote node */
    STprintf( buf, ERx("%s.%.*s.gcn.local_vnode"), SystemCfgPrefix, 
	      CX_MAX_NODE_NAME_LEN, rhost );
    if ( OK != PMget( buf, &valp ) || 
	 NULL == valp ||
	 '\0' == *valp )
    {
	/* Configuration failure */
	return FAIL;
    }

    radspec[0] = '\0';
    if ( pni->cx_rad_id )
    {
	STprintf( radspec, ERx(" -rad=%d"), pni->cx_rad_id );
    }

    if ( BIG_ENOUGH < 2 + STlength(RshUtil) + STlength(valp) +
         STlength(cmd) + STlength(radspec) )
    {
	STprintf( buf, ERget( S_ST05A8_command_line_too_long ), ProgName );
	WARNING(buf);
	return FAIL;
    }

    STprintf( buf, ERx("%s %s %s%s"), RshUtil, valp, cmd, radspec );
    return( PCcmdline( (LOCATION *) NULL, buf, PC_WAIT,
		(LOCATION *) NULL, &err_code) );
}


/*
**  Start the name server.
*/
static STATUS
start_iigcn( void )
{
	i4 time = 0;

	PRINT( ERget( S_ST0500_starting_gcn ) ); 
# if defined(VMS)
	PRINT( "\n" );
	STprintf( cmd, ERx( "iirundbms gcn" ) );
# elif defined(NT_GENERIC)
	STpolycat( 4, SystemAltExecName, ERx( "run " ), SystemAltExecName,
		   ERx( "gcn" ), cmd );
# else /* UNIX */
	STpolycat( 3, SystemAltExecName, ERx( "rundbms " ), 
		   ERx( "gcn" ), cmd );

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

# endif /* UNIX */
	if( execute( cmd ) != OK )
		return( FAIL );

	return( OK );
}


/*
** Ping name server to see if it's up.
*/
static STATUS 
ping_iigcn( void )
{
	GCA_PARMLIST	parms;
	STATUS		status;
	CL_ERR_DESC	cl_err;
	i4		assoc_no;
	STATUS		tmp_stat;
	char		gcn_id[ GCN_VAL_MAX_LEN ];

	MEfill( sizeof(parms), 0, (PTR) &parms );

	(void) IIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0,
		-1L, &status);
			
	if( status != OK || ( status = parms.gca_in_parms.gca_status ) != OK )
		return( FAIL );

	status = GCnsid( GC_FIND_NSID, gcn_id, GCN_VAL_MAX_LEN, &cl_err );

	if( status == OK )
	{
		/* make GCA_REQUEST call */
		MEfill( sizeof(parms), 0, (PTR) &parms);
		parms.gca_rq_parms.gca_partner_name = gcn_id;
		parms.gca_rq_parms.gca_modifiers = GCA_CS_BEDCHECK |
			GCA_NO_XLATE;
		(void) IIGCa_call( GCA_REQUEST, &parms, GCA_SYNC_FLAG, 
			0, GCN_RESTART_TIME, &status );
		if( status == OK )
			status = parms.gca_rq_parms.gca_status;

		/* make GCA_DISASSOC call */
		assoc_no = parms.gca_rq_parms.gca_assoc_id;
		MEfill( sizeof( parms ), 0, (PTR) &parms);
		parms.gca_da_parms.gca_association_id = assoc_no;
		(void) IIGCa_call( GCA_DISASSOC, &parms, GCA_SYNC_FLAG, 
			0, -1L, &tmp_stat );

		IIGCa_call( GCA_TERMINATE,
			    &parms,
			    (i4) GCA_SYNC_FLAG,    /* Synchronous */
			    (PTR) 0,                /* async id */
			    (i4) -1,           /* no timeout */
			    &tmp_stat);

		/* existing name servers answer OK, new ones NO_PEER */
		if( status == E_GC0000_OK || status == E_GC0032_NO_PEER )
			return( OK );
	}
 
        /*
        ** Terminate the connection because we will initiate it again on
        ** entry to this routine.
        */
        IIGCa_call(   GCA_TERMINATE,
                      &parms,
                      (i4) GCA_SYNC_FLAG,    /* Synchronous */
                      (PTR) 0,                /* async id */
                      (i4) -1,           /* no timeout */
                      &status);
	return( FAIL );
}


/*
**  Wait for the name server to accept connections. 
*/
static void
wait_iigcn( void )
{
    char     *dunsel;  /* dummy var to reinitialize sym table */
    i4 SleepTime = 30;
    i4 sleep;

    for (sleep=1 ; sleep <=SleepTime; sleep++)
    {
	NMflushIng(); /* make SURE symbol table reread */
	NMgtIngAt( ERx( "II_INSTALLATION" ), &dunsel);
	PCsleep( 1000 );
	if (ping_iigcn() == OK)
	    break;
    }
}

/*
**  Start the archiver processes.
*/
static STATUS
start_dmfacp( void )
{
	PRINT( ERget( S_ST0501_starting_dmfacp ) ); 

# ifdef VMS
	PRINT( "\n" );
	STprintf( cmd, ERx( "iirunacp %s %s" ), uic, ii_installation );
# else /* VMS */
	STprintf( cmd, ERx( "iirun%s dmfacp" ), Affine_Some );

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

# endif /* VMS */

	if( execute( cmd ) != OK )
		return( FAIL );

	return( OK );
}


/*
**  Start the recovery server.
*/
static STATUS
start_dmfrcp( void )
{
	i4 sleep;
	char tmp_buf[BIG_ENOUGH];
#ifdef VMS
        char chk_log [100];
        char *gbl_sec;
        char def_cmd [500];
        CL_SYS_ERR err_code;

        /* Check if RCP is running. lgk_mem_ready set if memory
        ** segment exists. This could happen following a bad
        ** shutdown of Ingres,
        */
        MEshow_pages(check_mem_name, 0, &err_code);

        if ( lgk_mem_ready && (execute( ERx( "rcpstat -silent -online" ) ) == OK))
        {
           PRINT("RCP server already started\n\n");

           return FAIL;
        }

#define LGLK_MEM "lglkdata.mem"

        if (lgk_mem_ready)
        {
           /* The LGK memory exists. But we're starting the system, so no memory
           ** should exist. So clean up.
           */
           MEsmdestroy(LGLK_MEM, &err_code);

           lgk_mem_ready = 0;
        }

        /* If the logical GBL$II_<inst_id>_lglkdata.mem exists, then the
        ** previous INGSTART detected that a process was connected to the default
        ** named lglkdata segment. We can remove the logical, this means we will
        ** verify that the default LGLK memory area doesn't exist. If it does
        ** exist, we will create a new logical which have a unique value (due
        ** to use of PID; even if the same session issues the INGSTART command).
        **
        ** Note the II_%s_%s must expand to the key name created in ME_makekey
        ** function. The first %s is the II_INSTALLATION id, if this is undefined,
        ** then an empty string.
        **
        **    Example GBL$II_E5_lglkdata.mem or GBL$II__lglkdata.mem
        */

        STprintf( chk_log, "GBL$II_%s_%s", ii_installation, LGLK_MEM);
        NMgtAt( chk_log, &gbl_sec);

        if (gbl_sec && *gbl_sec)
        {
           /* Logical exists, so deassign */
           STprintf( def_cmd, "DEASSIGN/SYSTEM %s", chk_log);

           if( execute(def_cmd) != OK )
           {
              F_PRINT("\nFailed deassigning logical: %s\n\n", def_cmd);

              return FAIL;
           }
        }

        if (execute( ERx( "rcpstat -silent -connected")) == OK)
        {
           PID pid;

           PCpid( &pid );

           STprintf( def_cmd, "DEFINE/SYSTEM %s \"II_%s_%s_%p\"", chk_log, ii_installation, LGLK_MEM, pid);

           if( execute(def_cmd) != OK )
           {
              F_PRINT("\nFailed Assigning logical: %s\n\n", def_cmd);

              return FAIL;
           }
        }
#endif
	PRINT( ERget( S_ST0502_starting_dmfrcp ) ); 
# ifdef VMS
	PRINT( "\n\n" );
# endif /* VMS */
	STprintf( cmd, ERx( "iirundbms%s recovery \\(dmfrcp\\) %s" ),
		Affine_Always, ii_installation );

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

	if( execute( cmd ) != OK )
		return( FAIL );

	/* wait for recovery server to go on-line */
#ifdef NT_GENERIC
	STprintf( tmp_buf, ERget( S_ST0503_dmfrcp_waiting ),
		  SystemProductName );
	PRINT( tmp_buf );
	for (sleep=1 ; sleep <=5; sleep++)
	{
	    PRINT( ERx(".") );
	    PCsleep( 1000 );
	}
	PRINT( ERx("\n") );
#else
#ifdef VMS 
	/* Wait for the LG LK shared memory segment to be created.
        ** But don't wait forever, 60 seconds should be more than
        ** enough time for the server to create the memory.
        */
        for(sleep = 1; sleep < 60; sleep++)
        {
           MEshow_pages(check_mem_name, 0, &err_code);

           if (lgk_mem_ready) break;

           PCsleep( 1000 );
        }

        if (sleep == 60) return FAIL; /* DMFRCP failed to start */
#else
	PCsleep( 1000 );
#endif
#endif
	STprintf( cmd, ERx( "rcpstat%s -silent -online" ), Affine_Clust );
	if( execute( cmd ) != OK )
	{
#ifndef NT_GENERIC
	        STprintf( tmp_buf, ERget( S_ST0503_dmfrcp_waiting ),
	 	          SystemProductName );
		PRINT( tmp_buf );
#endif

		while( execute( cmd ) != OK )
			PCsleep( 1000 );
	}

	if(Clustered)
	{
	    i4		i;

	    /* Replace fixed 60 second delay with a poll */
	    for ( i = 0; /*FOREVER*/ ; i++ )
	    {
		STprintf( cmd, ERx( "rcpstat%s -silent -csp_online" ),
			  Affine_Clust );
		if ( OK == execute( cmd ) )
		    break;

		if ( !( i % 10 ) )
		{
		    /* Every once in a while check to see if RCP has exitted */
		    STprintf( cmd, ERx( "rcpstat%s -silent -online" ),
		              Affine_Clust );
		    if ( OK != execute( cmd ) )
			return FAIL;

		    PRINT( ERget( S_ST0510_dmfcsp_waiting ) );
		}
		PCsleep( 1000 );
	    }
	}
	return( OK );
}

# ifdef VMS
# define CLUSTER_LCK_FILE	"cluster.lck;1"

static LOCATION mutex_loc;
static char mutex_locbuf[ MAX_LOC + 1 ];
static FILE *mutex_fp;
static	$EXHDEF     exit_block;
static i4   mutex_file_owned = 0;
# endif /* VMS */

# define MAX_CLUSTER_NODES	32

# define LOG_ENABLE_MASK	1
# define LOG_TRANSACTION_MASK	2
# define DUAL_ENABLE_MASK	4
# define DUAL_TRANSACTION_MASK  8	

typedef struct {
	char	*node;
	i4	info;	
} LOG_INFO;


/*
**  ersatz_P() semaphore operation to implement mutual exclusion between
**  processes.
*/
static void
ersatz_P( void )
{
# ifdef VMS
	sys$canexh( &exit_block );
	if ( mutex_fp ) SIclose( mutex_fp );
	if ( mutex_file_owned ) LOdelete( &mutex_loc );
# endif /* VMS */
}


/*
**  ersatz_V() semaphore operation to implement mutual exclusion between
**  processes.
*/
static void
ersatz_V( void )
{
# ifdef VMS
    PID		mypid, otherpid;
    STATUS	status;
    i4		count, len;
    char	buffer[32];
    char	imagename[LO_PATH_MAX];
    struct	{
	u_i2		 buflen;
	u_i2		 itmcod;
	i4      	*bufadr;
	i4		*lenadd;
    }	itmlist[2] = {
    	sizeof(imagename) - 1, JPI$_IMAGNAME, imagename, 0,
	0,
    };

    PCpid(&mypid);
    NMloc( FILES, FILENAME, ERx( CLUSTER_LCK_FILE ), &mutex_loc );
    LOcopy( &mutex_loc, mutex_locbuf, &mutex_loc );

    /*
    ** Try to assure file gets removed on image exit.
    */
    exit_block.exh$g_func = ersatz_P;
    exit_block.exh$l_argcount = 0;

    if ( (status = sys$dclexh(&exit_block)) != SS$_NORMAL)
    {
	/* Should never happen, but JIC it does, bail here. */
	STprintf( buffer, ERx("sys$dclexh failure = 0x%x\n"), status );
	PRINT( buffer );
	PCexit(status); 
    }

    for ( ; /* something to break out of */ ; )
    {
	/* Try to read cluster.lck */
	count = 0;
	while ( SIopen( &mutex_loc , ERx("r"), &mutex_fp ) == OK )
	{
	    /* Search for last PID added to file. */
	    otherpid = 0;
	    while ( OK == SIgetrec( buffer, sizeof(buffer), mutex_fp ) )
	    {
		/* Trim off the trailing new-line */
		len = STlength(buffer) - 1;
		if ( len > 0 && buffer[len] == '\012' )
		{
		    buffer[len] = '\0';
		}
		CVahxl(buffer, &otherpid);
	    }

	    if ( otherpid )
	    {
		IOSB iosb;

		if ( otherpid == mypid )
		{
		    /*
		    ** PID is available, but this is apparently a
		    ** retry of ingstart from the same process?!,
		    ** or a major coincidence. (file shouldn't be
		    ** here if ingstart is rerun from the same
		    ** process since exit handler should have removed
		    ** it if previous ingstart instance had abended
		    ** before calling ersatz_P.)
		    **
		    ** Either way, no need to create or update file, 
		    ** just exit.
		    */
		    SIclose( mutex_fp );
		    mutex_fp = NULL;
		    return;
		}

		/*
		** Find out if PID is still valid, and what's being run.
		*/
		status = sys$getjpiw(EFN$C_ENF, &otherpid, NULL, &itmlist,
				     &iosb, NULL, 0);
		if (status & 1)
		    status = iosb.iosb$w_status;
		if ( status & 0x1 )
		{
		    if ( NULL == STrstrindex( imagename, ERx("ingstart"),
					      sizeof(imagename), 1 ) )
		    {
			/*
			** Process is not running ingstart.
			** Break out of read loop, and try to claim
			** lock file.
			*/
			SIclose( mutex_fp );
			mutex_fp = NULL;
			break;
		    }
		}
		else
		{
		    /*
		    ** Process is no longer active.
		    */
		    SIclose( mutex_fp );
		    mutex_fp = NULL;
		    break;
		}
	    }
	    else if ( count > 0 )
	    {
		/*
		** Other process had plenty of time to write PID,
		** treat as invalid PID.
		*/
		SIclose( mutex_fp );
		mutex_fp = NULL;
		break;
	    }

	    SIclose( mutex_fp );
	    mutex_fp = NULL;

	    if ( 0 == count )
	    {
		PRINT( ERget( S_ST0504_remote_dmfcsp_waiting ) ); 
	    }
	    else
	    {
		SIprintf( "." );
		SIflush( stdout );
	    }
	    PCsleep(1000);
	    count++;
	}

	/*
	** Create, or append to lock file as circumstances dictate.
	** Normal case is file was removed by ersatz_P in the last
	** successful run of ingstart, or by the exit handler if
	** the ingstart abended somehow before ersatz_P was called.
	*/
	if( SIopen( &mutex_loc, ERx("a"), &mutex_fp ) == OK )
	{
	    /* Write out our PID */
	    STprintf( buffer, "%x", mypid );
	    SIputrec( buffer, mutex_fp );
	    SIflush( mutex_fp );
	    SIclose( mutex_fp );
	    mutex_fp = NULL;
	    mutex_file_owned = 1;

	    if ( count > 0 )
	    {
		PRINT( ERget( S_ST0505_remote_dmfcsp_online ) ); 
	    }
	    break;
	}
    }
# endif /* VMS */
}


/*
** Query the state of the logging system on a specified cluster node.
*/
static void
get_log_info( LOG_INFO *log_info )
{
	log_info->info = 0;
	STprintf( cmd, ERx( "rcpstat%s -silent -enable -node %s" ),
		Affine_Clust, log_info->node );
	if( execute( cmd ) == OK )
	{
		log_info->info |= LOG_ENABLE_MASK;
		STprintf( cmd,
			ERx( "rcpstat%s -silent -transactions -node %s" ),
			Affine_Clust, log_info->node );
		if( execute( cmd ) == OK )
			log_info->info |= LOG_TRANSACTION_MASK;
	}

	STprintf( cmd, ERx( "rcpstat%s -silent -exist -dual -node %s" ),
		Affine_Clust, log_info->node );
	if( execute( cmd ) == OK )
	{
	    STprintf( cmd, ERx( "rcpstat%s -silent -enable -dual -node %s" ),
		    Affine_Clust, log_info->node );
	    if( execute( cmd ) == OK )
	    {
		log_info->info |= DUAL_ENABLE_MASK;
		STprintf( cmd,
			ERx( "rcpstat%s -silent -transactions -dual -node %s" ),
			Affine_Clust, log_info->node );
		if( execute( cmd ) == OK )
			log_info->info |= DUAL_TRANSACTION_MASK;
	    }
	}
}


/*
** Report the state of the logging system on a specified VMS cluster node.
*/
static STATUS
check_log_info( LOG_INFO *log_info )
{
	i4 log1_ok = FALSE;
	i4 log2_ok = FALSE;

	PRINT( "\n" );
	if( log_info->info & LOG_ENABLE_MASK )
	{
		F_PRINT( ERget( S_ST0506_cluster_log1_on ), 
			log_info->node );
		log1_ok = TRUE;
		if( log_info->info & LOG_TRANSACTION_MASK )
		{
			F_PRINT( ERget( S_ST0507_cluster_log1_tx ), 
				log_info->node );
		}
	}
	else
	{
		F_PRINT( ERget( S_ST0508_cluster_log1_off ), 
			log_info->node );
	}

	if( log_info->info & DUAL_ENABLE_MASK )
	{
		F_PRINT( ERget( S_ST0509_cluster_log2_on ),
			log_info->node );
		log2_ok = TRUE;
		if( log_info->info & LOG_TRANSACTION_MASK )
		{
			F_PRINT( ERget( S_ST050A_cluster_log2_tx ), 
				log_info->node );
		}
	}
	else
	{
		F_PRINT( ERget( S_ST050B_cluster_log2_off ), 
			log_info->node );
	}

	if( !log1_ok && !log2_ok ) 
	{
		F_PRINT( ERget( S_ST050C_cluster_log_missing ), 
			log_info->node );
		return( FAIL );
	}

	return( OK );
}


/*
**  Start the cluster (if configured), recovery and archiver processes,
**  then wait for the logging system to come on-line before returning. 
*/
static void
start_logging( void )
{


# ifndef VMS
# ifndef NT_GENERIC
	char tmp_buf[BIG_ENOUGH];

	/* clean up old UNUSED shared memory possibly left lying around */ 
	if ( checkcsclean() )
		ERROR( ERget( S_ST0672_shared_mem_in_use ) ); 

	if ( !Have_Sysmem_Seg )
	{
	    STprintf( tmp_buf, ERx( "ipcclean%s" ), Affine_Clust );
	    (void) execute( tmp_buf );

	    PRINT( ERget( S_ST0511_csinstalling ) );
 
	    STprintf( tmp_buf, ERx( "csinstall%s -" ), Affine_Always );
	    if( execute( tmp_buf ) != OK )
		ERROR( ERget( S_ST0512_csinstall_failed ) );
	}
# endif /* NT_GENERIC */
# endif /* VMS */

	/* 
	** Jasmine does not use either the recovery server or the archiver.
	*/
	if( start_dmfrcp() != OK )
		ERROR( ERget( S_ST0513_dmfrcp_failed ) ); 
	if( start_dmfacp() != OK )
		ERROR( ERget( S_ST0514_dmfacp_failed ) ); 
}


/*
**  Start a DBMS server.
*/
static STATUS
start_iidbms( char *name )
{
	char *label;

#if !defined(int_lnx) && !defined(int_rpl) && !defined(ibm_lnx)
#ifdef WORKGROUP_EDITION

	if (PCget_PidFromName("iidbms") != 0)
	{
		F_PRINT( ERget( S_ST0570_dbms_already_running ), 
						SystemProductName );
		PCexit( FAIL );
	}

#endif /* WORKGROUP_EDITION */
# endif /* !int_lnx && !int_rpl && !ibm_lnx*/

	if( STcompare( ERx( "*" ), name ) == 0 )
	{
		label = ERx( "default" );
		name = ERx( "\\(default\\)" );
	}
	else
	{
		label = name;
	}
	STprintf( cmd, ERx( "%srundbms%s dbms %s %s" ),
		SystemAltExecName, Affine_Always, 
		name, ii_installation );

	F_PRINT( ERget( S_ST0515_starting_dbms ), label );
# ifdef VMS
	PRINT( "\n\n" );
# endif /* VMS */

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

	if( execute( cmd ) != OK )
		return( FAIL );

	return( OK );
}


/*
**  Start a Star server.
*/
static STATUS
start_iistar( char *name )
{
	char *label;
	i4  sleep;

	if( STcompare( ERx( "*" ), name ) == 0 )
	{
		label = ERx( "default" );
		name = ERx( "\\(default\\)" );
	}
	else
	{
		label = name;
	}
	STprintf( cmd, ERx( "iirundbms%s star %s %s" ),  Affine_Always, 
		    name, ii_installation );

	F_PRINT( ERget( S_ST0516_starting_star ), label );
# ifdef VMS
	PRINT( "\n\n" );
# endif /* VMS */

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

	if( execute( cmd ) != OK )
		return( FAIL );
# ifdef NT_GENERIC
	PRINT( ERget( S_ST0579_star_waiting ) );
	for (sleep = 0 ; sleep < 5; sleep++)
	{
		PRINT( ERx(".") );
		PCsleep( 2000 );
	}
	PRINT( ERx("\n") );
# endif
	
	return( OK );
}


/*
**  Start a Net server.
*/
STATUS
start_iigcc( name )
char *name; 
{
	char *label;

#ifdef UNIX
	/* This section has been made UNIX specific as the 
	** iiCL_get_fd_table_size() is
        ** not a CL supported interface.
        */
 	char *val = 0;
 	i4 ib_limit = 0;
 	i4 ob_limit = 0;
 	i4 hard_limit = 0;
 	i4 required = 0;
 	char buf[BIG_ENOUGH];
 
 	if( PMget( ERx( "$.$.gcc.$.inbound_limit" ), &val ) != OK )
        {
 	    PRINT( ERx("Config definition for inbound connections does not exist\n") );
        }
	else if( val && *val ) CVal(val, &ib_limit );
 	if( PMget( ERx("$.$.gcc.$.outbound_limit" ), &val ) != OK )
        {
 	    PRINT( ERx("Config definition for outbound connections does not exist\n") );
        }
        else if( val && *val ) CVal(val, &ob_limit );

        /* The calculation of required limit is based on 
        ** the assumption that NET Server is servicing
        ** both inbound and outbound connections
        */
 	required = ( ib_limit + ob_limit ) * 2 + 5;
 	hard_limit = iiCL_get_fd_table_size(); 
 	if( required > hard_limit ) 
 	{
 	    /* Issue a warning message and also log it in the errlog.log  */
 	    STprintf( buf, ERget(S_ST0684_increase_file_limit), ib_limit, ob_limit, required, hard_limit ); 
 	    SIfprintf(stderr, ERx(buf) );
 	    SIflush( stderr );
 	    write_to_errlog( buf );
 	}

#endif
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;

	F_PRINT( ERget( S_ST0517_starting_gcc ), label );

# ifdef VMS
	PRINT( ERx( "\n\n" ) );
        if( label == name )
                STprintf( cmd, ERx( "iirundbms gcc %s" ), name );
        else
                STprintf( cmd, ERx( "iirundbms gcc" ) );
# else /* VMS */

	if( label == name )
		STprintf( cmd, ERx( "%srundbms%s gcc -instance=%s" ), 
		SystemAltExecName, Affine_Some, name );
	else
		STprintf( cmd, ERx( "%srundbms%s gcc"), SystemAltExecName,
		 Affine_Some );

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

# endif /* VMS */

	if( execute( cmd ) != OK )
		return( FAIL );

#if defined(dg8_us5)
         PCsleep(500);
#endif /* dg8_us5  */

	return( OK );
}


/*
**  Start a DAS server.
*/
STATUS
start_iigcd( name )
char *name;
{
        char *label;

        if( STcompare( ERx( "*" ), name ) == 0 )
                label = ERx( "default" );
        else
            label = name;

        F_PRINT( ERget( S_ST0693_starting_iigcd ), label );

# ifdef VMS
        PRINT( ERx( "\n\n" ) );
        if( label == name )
            STprintf( cmd, ERx( "iirundbms gcd %s" ), name );
        else
            STprintf( cmd, ERx( "iirundbms gcd") );

# else /* VMS */
        if( label == name )
                STprintf( cmd, ERx( "%srundbms%s gcd -instance=%s" ),
                SystemAltExecName, Affine_Some, name );
        else
                STprintf( cmd, ERx( "%srundbms%s gcd"), SystemAltExecName,
		          Affine_Some );

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

# endif /* VMS */

        if( execute( cmd ) != OK )
                return( FAIL );

        PRINT( ERx( "\n" ) );
        return( OK );
}


# ifdef NT_GENERIC

/*
**  Start a Gateway server--Windows 95 only--for OpenINGRES Desktop.
*/
STATUS
start_iigws( )
{
    char	cmd[80];
	char *ingstart_iigws;

	if( PMget( ERx( "ii.$.ingstart.*.iigws" ),
		&ingstart_iigws ) != OK )
	{
		return( OK );
	}

    PRINT( ERget( S_ST0532_starting_gws ) );

    STcopy( ERx("iirun iigws"), cmd );

    if( execute( cmd ) != OK )
	return( FAIL );

    return( OK );
}
# endif

/*
**  Start VDBA remote command server
*/

STATUS
start_rmcmd()
{
char*   ingstart_rmcmd;

    if( PMget( ERx( "ii.$.ingstart.*.rmcmd" ),
        &ingstart_rmcmd ) != OK )
    {
        return( OK );
    }
    PRINT( ERget( S_ST0539_starting_rmcmd ) );

# if defined(VMS)
        PRINT( "\n" );
        STprintf( cmd, ERx( "iirundbms rmcmd" ));
# elif defined(NT_GENERIC)
    STprintf( cmd, ERx("%srun%s rmcmd"), SystemAltExecName, Affine_Some );
# else /* UNIX */
    STprintf( cmd, ERx("%srundbms%s rmcmd"), SystemAltExecName, Affine_Some );
# endif /* UNIX */

    if( execute( cmd ) != OK )
	return( FAIL );

    return( OK );
}


/*
**  Start a Bridge server.
*/
STATUS
start_iigcb( name )
char *name;
{
    char *label;
	 
    if( STcompare( ERx( "*" ), name ) == 0 )
	label = ERx( "default" );
    else
	label = name;
				  
    F_PRINT( ERget( S_ST052D_starting_gcb ), label );
								   
# ifdef VMS
	PRINT( ERx( "\n\n" ) );
        if( label == name )
            STprintf( cmd, ERx( "iirundbms gcb %s" ), name );
        else
            STprintf( cmd, ERx( "iirundbms gcb" ) );
						    
# else /* VMS */
			     
	if( label == name ) 
	    STprintf( cmd, ERx( "%srundbms%s gcb -instance=%s" ), 
	    SystemAltExecName, Affine_Some, name );
	else 
	    STprintf( cmd, ERx( "%srundbms%s gcb" ), SystemAltExecName,
	    Affine_Some );
	   
# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */
			    
# endif /* VMS */
	     
	if( execute( cmd ) != OK )
	     return( FAIL );
				      
	return( OK );
}

#if defined(conf_BUILD_ICE)
/*
**  Start an ice server.
*/
static STATUS
start_icesvr( char *name )
{
	char *label;

#if !defined(int_lnx) && !defined(int__rpl) && !defined(ibm_lnx)
#ifdef WORKGROUP_EDITION
	if (PCget_PidFromName("icesvr") != 0)
	{
		F_PRINT( ERget( S_ST0577_icesvr_already_running ),
						SystemProductName );
		PCexit( FAIL );
	}

#endif /* WORKGROUP_EDITION */
#endif /* !int_lnx && !int_rpl && !ibm_lnx */

	if( STcompare( ERx( "*" ), name ) == 0 )
	{
		label = ERx( "default" );
		name = ERx( "\\(default\\)" );
	}
	else
	{
		label = name;
	}
	STprintf( cmd, ERx( "%srunice%s icesvr %s %s" ),
		  SystemAltExecName, Affine_Some, name,
		  ii_installation);

	F_PRINT( ERget( S_ST0574_starting_icesvr ), label );
# ifdef VMS
	PRINT( "\n\n" );
# endif /* VMS */

# ifdef SOLARIS_PATCH
def_ii_gc_port();
# endif /* SOLARIS_PATCH */

	if( execute( cmd ) != OK )
		return( FAIL );

	return( OK );
}
#endif

/*
** Name: start_oracle
**
** Description: Start a named Oracle Gateway Server. 
**
** History:
**	24-mar-98 (mcgem01)
**	    Created.
**      24-may-99 (chash01)
**          Add ifdef for starting VMS version oracle gateway.
*/
static STATUS
start_oracle( name )
char *name;
{
	char *label;
	 
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0550_starting_oracle ), label );
								   
# ifdef VMS
        PRINT( "\n" );
        STprintf( cmd, ERx( "%sgwstart oracle" ),SystemCatPrefix);
# else /* VMS */
	if( label == name ) 
	    STprintf( cmd, ERx( "%srun%s %sgwstart oracle -instance=%s" ), 
	    SystemAltExecName, Affine_Some, SystemAltExecName, name );
	else 
	    STprintf( cmd, ERx( "%srun%s %sgwstart oracle" ),
	    SystemAltExecName, Affine_Some, SystemAltExecName );
#endif				      
	   
	if( execute( cmd ) != OK )
		return( FAIL );
# if defined (UNIX) || defined (NT_GENERIC)
	PRINT( ERx( "\n" ) );
# endif /* UNIX */
	return( OK );
}
/*
** Name: start_informix
**
** Description: Start a named Informix Gateway Server. 
**
** History:
**	24-mar-98 (mcgem01)
**	    Created.
*/
static STATUS
start_informix( name )
char *name;
{
	char *label;
	 
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0551_starting_informix ), label );
								   
	if( label == name ) 
	    STprintf( cmd, ERx( "%srun% %sgwstart informix -instance=%s" ), 
	    SystemAltExecName, Affine_Some, SystemAltExecName, name );
	else 
	    STprintf( cmd, ERx( "%srun%s %sgwstart informix" ), 
		SystemAltExecName, Affine_Some, SystemAltExecName );
	   
	if( execute( cmd ) != OK )
		return( FAIL );
				      
# if defined (UNIX) || defined (NT_GENERIC)
	PRINT( ERx( "\n" ) );
# endif /* UNIX */
	return( OK );
}
/*
** Name: start_mssql
**
** Description: Start a named Microsoft SQL Server Gateway Server. 
**
** History:
**	24-mar-98 (mcgem01)
**	    Created.
*/
static STATUS
start_mssql( name )
char *name;
{
	char *label;
	 
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0552_starting_mssql ), label );
								   
	if( label == name ) 
	    STprintf( cmd, ERx( "%srun%s %sgwstart mssql -instance=%s" ), 
	    SystemAltExecName, Affine_Some, SystemAltExecName, name );
	else 
	    STprintf( cmd, ERx( "%srun%s %sgwstart mssql" ),
	    SystemAltExecName, Affine_Some, SystemAltExecName );
	   
	if( execute( cmd ) != OK )
		return( FAIL );
				      
# if defined (UNIX) || defined (NT_GENERIC)
	PRINT( ERx( "\n" ) );
# endif /* UNIX */
	return( OK );
}
/*
** Name: start_odbc
**
** Description: Start a named ODBC Gateway Server. 
**
** History:
**	24-mar-98 (mcgem01)
**	    Created.
*/
static STATUS
start_odbc( name )
char *name;
{
	char *label;
	 
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0554_starting_odbc ), label );
								   
	if( label == name ) 
	    STprintf( cmd, ERx( "%srun%s %sgwstart odbc -instance=%s" ), 
	    SystemAltExecName, Affine_Some, SystemAltExecName, name );
	else 
	    STprintf( cmd, ERx( "%srun%s %sgwstart odbc" ),
	    SystemAltExecName, Affine_Some, SystemAltExecName );
	   
	if( execute( cmd ) != OK )
		return( FAIL );
				      
# if defined (UNIX) || defined (NT_GENERIC)
	PRINT( ERx( "\n" ) );
# endif /* UNIX */
	return( OK );
}
/*
** Name: start_sybase
**
** Description: Start a named Sybase Gateway Server. 
**
** History:
**	24-mar-98 (mcgem01)
**	    Created.
*/
static STATUS
start_sybase( name )
char *name;
{
	char *label;
	 
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0553_starting_sybase ), label );
								   
	if( label == name ) 
	    STprintf( cmd, ERx( "%srun%s %sgwstart sybase -instance=%s" ), 
	    SystemAltExecName, Affine_Some, SystemAltExecName, name );
	else 
	    STprintf( cmd, ERx( "%srun%s %sgwstart sybase" ),
	    SystemAltExecName, Affine_Some, SystemAltExecName );
	   
	if( execute( cmd ) != OK )
		return( FAIL );
				      
# if defined (UNIX) || defined (NT_GENERIC)
	PRINT( ERx( "\n" ) );
# endif /* UNIX */
	return( OK );
}

/*
** Name: start_rms
**
** Description: Start a named RMS Gateway Server. 
**
** History:
**	04-aug-1999 (kinte01)
**	    Created.
*/
static STATUS
start_rms( name )
char *name;
{
	char *label;
	 

	if( STcompare( ERx( "*" ), name ) == 0 )
	{
		label = ERx( "default" );
		name = ERx( "\\(default\\)" );
	}
	else
	{
		label = name;
	}
	STprintf( cmd, ERx( "%srundbms%s rms %s %s" ), SystemCatPrefix,
		 Affine_Some, name, ii_installation );

	F_PRINT( ERget( S_ST0640_starting_rms ), label );
# ifdef VMS
	PRINT( "\n\n" );
# endif /* VMS */

	if( execute( cmd ) != OK )
		return( FAIL );

	return( OK );
				  
}

/*
** Name: start_rdb
**
** Description: Start a named RDB Gateway Server. 
**
** History:
**	04-aug-1999 (kinte01)
**	    Created.
**	27-sep-2002 (devjo01)
**	    Made this a No-Op outside of VMS.  Previously conditional
**	    compilation was just excluding formatting "cmd"!
*/
static STATUS
start_rdb( name )
char *name;
{
	 
# ifdef VMS
	char *label;

	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0645_starting_rdb ), label );
								   
        PRINT( "\n" );
        STprintf( cmd, ERx( "%sgwstart rdb" ),SystemCatPrefix);
	   
	if( execute( cmd ) != OK )
		return( FAIL );
# endif				      
	return( OK );
}
/*
** Name: start_db2udb
**
** Description: Start a named db2udb Gateway Server. 
**
** History:
**	24-mar-98 (mcgem01)
**	    Created.
*/
static STATUS
start_db2udb( name )
char *name;
{
	char *label;
	 
	if( STcompare( ERx( "*" ), name ) == 0 )
		label = ERx( "default" );
	else
		label = name;
				  
	F_PRINT( ERget( S_ST0673_starting_db2udb ), label );
								   
	if( label == name ) 
	    STprintf( cmd, ERx( "%srun%s %sgwstart db2udb -instance=%s" ), 
	    SystemAltExecName, Affine_Some, SystemAltExecName, name );
	else 
	    STprintf( cmd, ERx( "%srun%s %sgwstart db2udb" ),
	    SystemAltExecName, Affine_Some, SystemAltExecName );
	   
	if( execute( cmd ) != OK )
		return( FAIL );
				      
# if defined (UNIX) || defined (NT_GENERIC)
	PRINT( ERx( "\n" ) );
# endif /* UNIX */
	return( OK );
}


/*
**  The following data structures are used for maintaining lists of DBMS,
**  Net, Star, and Gateway servers to be started. 
*/

typedef struct process {
	char *name;
	i4 count;
	struct process *next;
} PROCESS_LIST;

PROCESS_LIST *dbms_list;	/* DBMS process list */
PROCESS_LIST *net_list;		/* Net processlist */
PROCESS_LIST *bridge_list;	/* Bridge processlist */
PROCESS_LIST *das_list;		/* DAS processlist */
PROCESS_LIST *star_list;	/* Star process list */
#if defined(conf_BUILD_ICE)
PROCESS_LIST *icesvr_list;	/* Ice server process list */
#endif
PROCESS_LIST *oracle_list;	/* Oracle Gateway process list */
PROCESS_LIST *informix_list;	/* Informix Gateway process list */
PROCESS_LIST *mssql_list;	/* Microsoft SQL Server Gateway process list */
PROCESS_LIST *odbc_list;	/* ODBC Gateway process list */
PROCESS_LIST *sybase_list;	/* Sybase Gateway process list */
PROCESS_LIST *rms_list;		/* RMS Gateway process list */
PROCESS_LIST *rdb_list;		/* RDB Gateway process list */
PROCESS_LIST *db2udb_list;	/* DB2UDB Gateway process list */
PROCESS_LIST *rmcmd_list;       /* rmcmd process list */
# ifdef NT_GENERIC
PROCESS_LIST *gateway_list;	/* Local Gateway process list */
# endif


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

	regexp = PMexpToRegExp( expbuf );

	for(
		status = PMscan( regexp, &state, NULL, &name,
		&value ); status == OK; status = PMscan( NULL,
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

		rec->name = PMgetElem( 3, name ); 
		rec->count = n; 
		rec->next = NULL;
	}
}


/*
**  Counts total servers configured for startup in list. 
*/
static i4
startup_total( PROCESS_LIST *list )
{
	i4 total = 0;

	for( ; list != NULL; list = list->next )
		total += list->count;
	return( total );
}


/*
**  Format the node is up message.
*/
static void
report_node_is_up( i4 nodenum )
{
    char	tmp_buf1[BIG_ENOUGH], tmp_buf2[64], tmp_buf3[64];

    if ( 0 == nodenum )
    {
	STlcopy( ERget( S_ST05A9_local_node ), tmp_buf2,
	         sizeof(tmp_buf2) - 1);
	tmp_buf2[sizeof(tmp_buf2) - 1] = '\0';
	tmp_buf3[0] = '\0';
    }
    else
    {
	if ( (1 << nodenum) && RemoteMask )
	{
	    STprintf( tmp_buf3, ERx( " -node=%.*s" ), CX_MAX_NODE_NAME_LEN,
	     ClusterConfiguration->cx_nodes[nodenum-1].cx_node_name);
	}
	else
	{
	    /* This virtual node already appears to be up. */
	    STprintf( tmp_buf3, ERx( " -rad=%d" ), CX_MAX_NODE_NAME_LEN,
	     ClusterConfiguration->cx_nodes[nodenum-1].cx_rad_id );
	}
	STprintf( tmp_buf2, ERx( "%.*s" ), CX_MAX_NODE_NAME_LEN,
	 ClusterConfiguration->cx_nodes[nodenum-1].cx_node_name );
    }
    
    STprintf( tmp_buf1,
     ERget( S_ST05A6_system_running ), SystemProductName, tmp_buf2,
     SystemExecName, tmp_buf3 );
    WARNING( tmp_buf1 );
}


/*
**  Check if a virtual NUMA node appears to be up.
*/
static i4
cluster_node_up(i4 nodenum)
{
    char	tmp_buf1[256];

    STprintf(tmp_buf1,
      ERx("rcpstat%s -silent -csp_online -node %.*s"),
      Affine_Clust,
      CX_MAX_NODE_NAME_LEN,
      ClusterConfiguration->cx_nodes[nodenum-1].cx_node_name);

    if( OK == execute(tmp_buf1) )
    {
	report_node_is_up( nodenum );
	return TRUE;
    }
    return FALSE;
}

# ifdef VMS

typedef struct
{
	u_i2	len;	
	u_i2	item;	
	PTR	ptr;	
	u_i2	*retlen;	
} ITEM_LIST;

# endif /* VMS */

# ifdef SOLARIS_PATCH

static void
def_ii_gc_port( void )
{
	char cmd[ 1024 ];

	STprintf( buf, "%s_GC_PORT=%d", SystemVarPrefix, ii_gc_port++ );
	(void) putenv( cmd );
}

# endif /* SOLARIS_PATCH */


main(argc, argv)
int	argc;
char	*argv[];
{

# ifdef SOLARIS_PATCH

/* ### SOLARIS_PATCH BUG WORK-AROUND HACK */
char *ii_gc_port
i4  ii_gc_port_val;

# endif /* SOLARIS_PATCH */

	char *host;
	PROCESS_LIST *cur;
	i4 i, j;
	LOCATION loc;
	char *syscheck;
	int   scretval=OK;
	bool usage;
	bool havegcn = FALSE;
	char *p1, *p2, *temp;
	PM_SCAN_REC state;
	STATUS status;
	char *name, *value;
	bool Is_w95 = FALSE;
	char tmp_buf[BIG_ENOUGH];
	char tmp_buf1[BIG_ENOUGH];
	char tmp_buf2[BIG_ENOUGH];
	i4	parammix;/* Bit mask to check for bad parameter combinations */
# define PARAM_MASK_COMPONENT	(1<<0)	/* Component argument */
# define PARAM_MASK_CLUSTER	(1<<1)	/* -cluster flag */
# define PARAM_MASK_NODE	(1<<2)	/* -node argument */
# define PARAM_MASK_SERVICE	(1<<3)	/* -service flag */
# define PARAM_MASK_CLIENT	(1<<4)	/* -client */
# define PARAM_MASK_RAD		(1<<5)  /* -rad argument */
# define PARAM_MASK_INSVCMGR	(1<<5)  /* -insvcmgr argument */
	char *argp, **argh, *argvalp, *component_arg;
	i4    nodenum, rad;
	u_i4  mask;
	u_i4  startup_mask = ~0;		/* Assume start all */

# ifdef VMS
	char *privileges;
	char *lnm_table_name;
	struct dsc$descriptor lnm_table;
	char *regexp;
	i4 privmask[ 2 ];	/* 64 bit mask! */
	char *p, *privs[ BIG_ENOUGH ];
	i4 npriv;
	
	/*
	** The order of this array's contents is SACRED (i.e. don't
	** touch it) since it defines the bit ordering required to
	** construct the 64-bit mask argument to sys$setprv(). 
	*/
	char *order[ 2 ][ 32 ] = { 
		{
		"CMKRNL", "CMEXEC", "SYSNAM", "GRPNAM", "ALLSPOOL",
		"DETACH", "DIAGNOSE", "LOG_IO", "GROUP", "NOACNT",
		"PRMCEB", "PRMMBX", "PSWAPM", "SETPRI", "SETPRV",
		"TMPMBX", "WORLD", "MOUNT", "OPER", "EXQUOTA",
		"NETMBX", "VOLPRO", "PHY_IO", "BUGCHK", "PRMGBL",
		"SYSGBL", "PFNMAP", "SHMEM", "SYSPRV", "BYPASS",
		"SYSLCK", "SHARE"
		},{
		"UPGRADE", "DOWNGRADE", "GRPPRV", "READALL", "", "",
		"SECURITY", "", "", "", "", "", "", "", "", "", "",
		"ACNT", "", "", "", "ALTPRI"
		}
	};	
	i4 fill[] = { 32, 22 };
	long access_mode = PSL$C_EXEC;
	struct dsc$descriptor lnm_name;
# endif /* VMS */

# ifdef NT_GENERIC
# include <windows.h>
	OSVERSIONINFO		lpvers;
	CHAR			ServiceName[255];
	SERVICE_STATUS		ssServiceStatus;
	LPSERVICE_STATUS	lpssServiceStatus = &ssServiceStatus;
	SC_HANDLE		schSCManager, OpIngSvcHandle;
	bool			as_service = FALSE;
	int			OsError;
	char			*OsErrText;
	HCLUSTER	hCluster = NULL;
	HRESOURCE	hResource = NULL;
	WCHAR	lpwResourceName[256];

/* this define for windows95 is not in c++ 2.0 headers */

# define VER_PLATFORM_WIN32_WINDOWS 1

        lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
        GetVersionEx(&lpvers);
        Is_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)? TRUE: FALSE;

	/*
	** Check if we're on Vista and running with a stripped token
	*/
	ELEVATION_REQUIRED();
	/*
	** Figure out if the service is started.
	*/
	if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
	    != NULL)
	{
	    char        *ii_install;

            NMgtAt( ERx( "II_INSTALLATION" ), &ii_install );
            if( ii_install == NULL || *ii_install == EOS )
                ii_install = ERx( "" );
            else
                ii_install = STalloc( ii_install );

	    STlpolycat(3, sizeof(ServiceName)-1, SystemServiceName,
		"_Database_", ii_install, &ServiceName[0]);

	    OpIngSvcHandle = OpenService(schSCManager, ServiceName, SERVICE_QUERY_STATUS);
	    if (OpIngSvcHandle == NULL)
     	    {
		STlpolycat(3, sizeof(ServiceName)-1, SystemServiceName,
			"_DBATools_", ii_install, &ServiceName[0]);
		OpIngSvcHandle = OpenService(schSCManager, ServiceName, 
					SERVICE_QUERY_STATUS);
  	    }
	    if (OpIngSvcHandle != NULL)
	    {
		if (QueryServiceStatus(OpIngSvcHandle, lpssServiceStatus))
		{
		    if (ssServiceStatus.dwCurrentState == SERVICE_RUNNING)
			ServiceStarted = TRUE;
		}
		CloseServiceHandle(OpIngSvcHandle);
	    }
	    CloseServiceHandle(schSCManager);
	}
# endif

	ProgName = STrindex( argv[0], ERx("/"), 0 );
	if (NULL == ProgName)
	    ProgName = argv[0];
	else
	    ProgName ++;

	MEadvise( ME_INGRES_ALLOC );

	(void) PMinit();

	if( PMload( NULL, PMerror ) != OK )
	    PCexit( FAIL );

	host = PMhost();

#ifdef UNIX
	/*
	** If we're not explicitly allowed to start as root, 
	** bail out apropriately 
	*/
	{
	    char  *rootok = NULL ;

	    STprintf( tmp_buf, ERx( "%s.%s.%sstart.root.allowed" ), 
		SystemCfgPrefix, host, SystemExecName );

	    PMget( tmp_buf, &rootok ) ;
	    if ( getuid() == (uid_t)0 && ! rootok )
	        NO_ROOT_START ;
	}
#endif

	/*
	** The following three flags describe the basic architecture
	** of the Ingres instance on the current box.
	**
	** - Clustered will be true on all nodes of a Cluster or
	**   NUMA cluster, false for a stand-alone installation.
	**
	** - NUMAclustered will be true if current box is configured
	**   for one or more virtual nodes.  This flag may have
	**   different values on different machines within an Ingres
	**   cluster.  However if NUMAclustered is true, Clustered
	**   is true.
	**
	** - NUMAsupported will be true if current box has hardware
	**   that is organized into Resource Affinity Domains (RADs),
	**   and Ingres has been coded on this platform to exploit
	**   this.   Ingres need not be explicitly configured to
	**   be NUMA aware to use the -rad parameter if this flag
	**   is set.
	*/
	Clustered = CXcluster_configured();
	NUMAclustered = CXnuma_cluster_configured();
	NUMAsupported = CXnuma_support();

	STprintf(log_file_name, ERx("%sstart.log"), SystemExecName );
	if ( Clustered )
	{
	    CXcluster_nodes(NULL, &ClusterConfiguration);
	    CXdecorate_file_name(log_file_name,host);

	    /* Check to see if remote shell is configured */
	    STprintf( tmp_buf, ERx( "%s.%s.config.rsh_method" ), 
		SystemCfgPrefix, host, SystemExecName );

	    PMget( tmp_buf, &RshUtil ) ;
	}

	if ((status = NMloc(LOG, FILENAME, log_file_name, &loc)) != OK)
	{
	    SIfprintf( stderr, "Error while building path/filename to %s\n",
		       log_file_name);
	    SIflush( stderr );
	    PCexit( FAIL );
	}

	LOtos(&loc, &temp);
	STcopy(temp, log_file_path);
	log_file_len = STlength(log_file_path);
	LOdelete(&loc);

	/*
	** Set parameter defaults.
	*/
	p1 = p2 = NULL;

	usage = FALSE;
	parammix = 0;
	component_arg = ERx( "" );

	argh = argv;
	while ( --argc > 0 )
	{
	    argp = *(++argh);
	    if ( '-' != *argp )
	    {
		/* All parameters start with "-" */
		break;
	    }
	    (void)CMnext( argp );

	    if ( STbcompare( argp, 0, ERx( "cluster" ), 0, TRUE ) == 0 )
	    {
		if ( !Clustered || NULL == RshUtil )
		{
		    STprintf(tmp_buf, ERget( S_ST05A5_not_supported ),
			     argp);
		    ERROR(tmp_buf);
		}

		if ( parammix & (PARAM_MASK_SERVICE|PARAM_MASK_CLIENT|
		      PARAM_MASK_CLUSTER|PARAM_MASK_NODE|PARAM_MASK_RAD|
		      PARAM_MASK_INSVCMGR) )
		    break;

		/* generate node mask with ALL nodes */
		add_to_node_mask(host, NULL, &NodeMask, &RemoteMask);
		parammix |= PARAM_MASK_CLUSTER;
	    }
	    else if ( STbcompare( argp, 0, ERx( "csinstalled" ), 0, TRUE )
		       == 0 )
	    {
		Have_Sysmem_Seg = 1;
	    }
# if NT_GENERIC /*always start as service on vista */
	    else if ( STbcompare( argp, 0, ERx( "service" ), 0, TRUE ) == 0) 
	    {
		if ( parammix & (PARAM_MASK_COMPONENT|PARAM_MASK_SERVICE|
		      PARAM_MASK_CLUSTER|PARAM_MASK_NODE|PARAM_MASK_RAD) )
		    break;

		parammix |= PARAM_MASK_SERVICE;
		startup_mask |= PCM_GCN;
		as_service = TRUE;
	    }
	    else if ( STbcompare( argp, 0, ERx( "client" ), 0, TRUE ) == 0 )
	    {
		if ( parammix & (PARAM_MASK_COMPONENT|PARAM_MASK_CLIENT|
		      PARAM_MASK_CLUSTER|PARAM_MASK_NODE|PARAM_MASK_RAD) )
		    break;

		parammix |= PARAM_MASK_CLIENT;
		startup_mask |= PCM_GCN;
		p1 = ERx( "client" );
	    }
	    else if ( STbcompare( argp, 0, ERx( "insvcmgr" ), 0, TRUE ) == 0 )
	    {
		if ( parammix & (PARAM_MASK_COMPONENT|PARAM_MASK_INSVCMGR|
		      PARAM_MASK_CLUSTER|PARAM_MASK_NODE|PARAM_MASK_RAD) )
		    break;

		parammix |= PARAM_MASK_INSVCMGR;
		startup_mask |= PCM_GCN;
		in_svcmgr = TRUE;
	    }
# endif /*NT_GENERIC*/
	    else 
	    {
		/* other possible parameters may have '='value syntax */
		argvalp = STindex( argp, ERx( "=" ), 0 );	
		if( argvalp != NULL )
		{
		    *argvalp = EOS;
		    (void) CMnext( argvalp );
		    if( *argvalp == EOS )
		    {
			/* "-param=" is bad syntax */
			break;
		    }
		}

		if ( STbcompare( argp, 0, ERx( "node" ), 0, TRUE ) == 0 )
		{
		    if ( !Clustered || NULL == RshUtil )
		    {
			STprintf(tmp_buf, ERget( S_ST05A5_not_supported ),
				 argp);
			ERROR(tmp_buf);
		    }

		    if ( parammix & (PARAM_MASK_SERVICE|PARAM_MASK_CLIENT|
			  PARAM_MASK_INSVCMGR|PARAM_MASK_CLUSTER|
			  PARAM_MASK_RAD))
			break;

		    /*
		    ** nodename argument value mandatory, but allow
		    ** "-node <nodename>" as well as "-node=nodename"
		    */
		    if ( NULL == argvalp )
		    {
			if ( 1 >= argc )
			    break;
			argc--;
			argvalp = *(++argh);    
		    }

		    add_to_node_mask(host, argvalp, &NodeMask, &RemoteMask);

		    parammix |= PARAM_MASK_NODE;
		}
		else if ( STbcompare( argp, 0, ERx( "rad" ), 0, TRUE ) == 0 )
		{
		    if ( !( NUMAclustered || NUMAsupported ) )
		    {
			STprintf(tmp_buf, ERget( S_ST05A5_not_supported ),
				 argp);
			ERROR(tmp_buf);
		    }

		    if ( parammix & (PARAM_MASK_SERVICE|PARAM_MASK_CLIENT|
			  PARAM_MASK_INSVCMGR|PARAM_MASK_CLUSTER|
			  PARAM_MASK_RAD) )
			break;
		    
		    /*
		    ** rad argument value mandatory, but allow
		    ** "-rad <rad>" as well as "-rad=rad"
		    */
		    if ( NULL == argvalp )
		    {
			if ( 1 >= argc )
			    break;
			argc--;
			argvalp = *(++argh);    
		    }

		    if ( OK != CVan(argvalp, &rad) )
			break;
		    
		    if ( NUMAclustered )
		    {
			if ( !CXnuma_rad_configured(host, rad) )
			{
			    STprintf(tmp_buf, ERget( S_ST05A3_bad_rad ),
				     rad, host);
			    ERROR(tmp_buf);
			}

			STprintf(tmp_buf, ERx( "R%d_%s" ), rad, host);
		    
			add_to_node_mask(host, tmp_buf,
					 &NodeMask, &RemoteMask);
		    }
		    else if ( rad > CXnuma_rad_count() )
		    {
			STprintf(tmp_buf, ERget( S_ST05A3_bad_rad ),
				 rad, host);
			ERROR(tmp_buf);
		    }
		    else
		    {
			/* Only prevent multiple RAD parameters if not 
			   running NUMA clusters. */
			parammix |= PARAM_MASK_RAD;
			RadID = rad;
		    }
		}
		else
		{
		    /* All other possible parameters are components */
		    if ( parammix & (PARAM_MASK_SERVICE|PARAM_MASK_CLIENT|
			  PARAM_MASK_INSVCMGR|PARAM_MASK_COMPONENT) )
			break;
		    parammix |= PARAM_MASK_COMPONENT;
		    component_arg = *argh;

		    p2 = argvalp;

		    STcopy(SystemAltExecName,tmp_buf);
		    STcat(tmp_buf, ERx( "dbms" ));
		    if( STbcompare( argp, 0, tmp_buf, 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_DBMS;
			p1 = ERx( "dbms" );
			continue;
		    }
		    STcopy(SystemAltExecName,tmp_buf);
		    STcat(tmp_buf, ERx( "gcc" ));
		    if( STbcompare( argp, 0, tmp_buf, 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_GCC;
			p1 = ERx( "gcc" );
			continue;
		    }
		    STcopy(SystemAltExecName,tmp_buf);
		    STcat(tmp_buf, ERx( "gcb" ));
		    if( STbcompare( argp, 0, tmp_buf, 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_GCB;
			p1 = ERx( "gcb" );
			continue;
		    }
		    STcopy(SystemAltExecName,tmp_buf);
		    STcat(tmp_buf, ERx( "gcd" ));
		    if( STbcompare( argp, 0, tmp_buf, 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_GCD;
			p1 = ERx( "gcd" );
			continue;
		    }
		    STcopy(SystemAltExecName,tmp_buf);
		    STcat(tmp_buf, ERx( "gcn" ));
		    if( STbcompare( argp, 0, tmp_buf, 0, TRUE ) == 0 &&
			p2 == NULL )
		    {
			startup_mask = PCM_GCN;
			p1 = ERx( "gcn" );
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "iistar" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_STAR;
			p1 = ERx( "star" );
			continue;
		    }

		    /* Rest of companent names are unmodified */
		    p1 = argp;

		    if( STbcompare( argp, 0, ERx( "dmfrcp" ), 0, TRUE ) == 0 &&
			p2 == NULL )
		    {
			startup_mask = PCM_RCP;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "dmfacp" ), 0, TRUE ) == 0 &&
			p2 == NULL )
		    {
			startup_mask = PCM_ACP;
			continue;
		    }
#if defined(conf_BUILD_ICE)
		    if( STbcompare( argp, 0, ERx( "icesvr" ), 0, TRUE ) == 0 &&
			p2 == NULL )
		    {
			startup_mask = PCM_ICE;
			continue;
		    }
#endif
		    if( STbcompare( argp, 0, ERx( "oracle" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_ORACLE;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "informix" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_INFORMIX;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "sybase" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_SYBASE;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "rmcmd" ), 0, TRUE ) == 0 &&
			p2 == NULL )
		    {
			startup_mask = PCM_RMCMD;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "rms" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_RMS;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "rdb" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_RDB;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "db2udb" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_DB2UDB;
			continue;
		    }
# ifdef NT_GENERIC
		    if( STbcompare( argp, 0, ERx( "mssql" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_MSSQL;
			continue;
		    }
		    if( STbcompare( argp, 0, ERx( "odbc" ), 0, TRUE ) == 0 )
		    {
			startup_mask = PCM_ODBC;
			continue;
		    }
		    if( Is_w95 &&
			STbcompare( argp, 0, ERx( "iigws" ), 0, TRUE ) == 0 &&
			p2 == NULL )
		    {
			startup_mask = PCM_IIGWS;
			p1 = ERx( "gws" );
			continue;
		    }
# endif
		    /* Didn't match anything, break */
		    break;
		}
	    }
	} /* while */

	if ( 0 != argc )
	{
	    /* Bad parameter specified */
	    usage = TRUE;
	}

	if( usage )
	{
# ifdef NT_GENERIC
		if (Is_w95)
		{
			STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -iigws | -iigcc[=name] ]\n\n" ), argv[0] );
			SIfprintf( stderr, tmp_buf );
		}
		else
#if defined(conf_BUILD_ICE)
			STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -dmfrcp | -dmfacp | -client | -rmcmd | -icesvr |\n\t-{iidbms|iigcc|iigcb|iigcd|iistar|oracle|informix|mssql|sybase|\n\tdb2udb}[=name] ]\n\n"),argv[0] );
#else
			STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -dmfrcp | -dmfacp | -client | -rmcmd |\n\t-{iidbms|iigcc|iigcb|iigcd|iistar|oracle|informix|mssql|sybase|\n\tdb2udb}[=name] ]\n\n"),argv[0] );
#endif
			SIfprintf( stderr, tmp_buf);
# else
# ifdef VMS
#if defined(conf_BUILD_ICE)
                STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -dmfrcp | -dmfacp | -rmcmd | -icesvr | -{iidbms|iigcc|iigcb|iigcd|iistar|oracle|rms|rdb}[=name] ]\n\n"), argv[0]);
#else
                STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -dmfrcp | -dmfacp | -rmcmd | -{iidbms|iigcc|iigcb|iigcd|iistar|oracle|rms|rdb}[=name] ]\n\n"), argv[0]);
#endif
# else
#if defined(conf_BUILD_ICE)
                STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -dmfrcp | -dmfacp | -rmcmd | -icesvr | -{iidbms|iigcc|iigcb|iigcd|iistar|oracle|informix|sybase|db2udb}[=name] ]\n\n"), argv[0]);
#else
                STprintf( tmp_buf, ERx( "\nUsage: %s [ -iigcn | -dmfrcp | -dmfacp | -rmcmd | -{iidbms|iigcc|iigcb|iigcd|iistar|oracle|informix|sybase|db2udb}[=name] ]\n\n"), argv[0]);
#endif
# endif /* VMS */
		if ( RshUtil )
		{
		    if ( NUMAclustered )
			STcat( tmp_buf,
		 ERx("   [ -cluster | {-node=nodename | -rad=radid}... ]\n") );
		    else if ( Clustered )
			STcat( tmp_buf,
		 ERx("   [ -cluster | -node=nodename... ]\n") );
		}
                SIfprintf( stderr, ERx( "%s\n" ),  tmp_buf);
# endif
		SIflush( stderr );
		PCexit( FAIL );
	}
# ifdef NT_GENERIC
	if (GVvista() && !as_service && argc==0)
		PRINT( ERget( S_ST069E_not_recommended) );
# endif
	if ( NUMAclustered && !NodeMask && p1 != NULL &&
	     (startup_mask & PCM_ALL_RAD_SPECIFIC) )
	{
	    /*
	    ** If NUMA clusters, and starting a component which needs,
	    ** a rad context, we better specify a specific node(s) or rad(s).
	    **
	    ** Calling CXget_context, just to fill in the error buf. 
	    */
	    (void)CXget_context( &argc, argv, 0, tmp_buf, sizeof(tmp_buf) - 1 );
	    tmp_buf[ sizeof(tmp_buf) - 1 ] = '\0';
	    ERROR( tmp_buf );
	}

	if ( RemoteMask )
	{
	    /*
	    ** Request is directed in whole or part at a set of 
	    ** nodes.   ingstart is called recursively against the
	    ** target node(s), using RPC.
	    */
	    char	*iisystem;
	    char	*fmt;
# ifdef UNIX
	    i4		 removecsshm;

	    removecsshm = ( OK == execute( ERx("csinstall - >& /dev/null") ) );
# endif
	    fmt = ERx("%s/ingres/utility/iirsh %s %s");

	    if ( p2 )
	    {
		STprintf( tmp_buf, ERx("%s %s=%s"), ProgName, component_arg,
		          p2 );
	    }
	    else
	    {
		STprintf( tmp_buf, ERx("%s %s"), ProgName, component_arg );
	    }

	    NMgtAt( ERx("II_SYSTEM"), &iisystem );
	    if ( NULL == iisystem || '\0' == *iisystem )
	    {
		/* Major configuration failure */
		STprintf( tmp_buf, ERget( S_ST0580_iisystem_not_set ),
			  ERx("II_SYSTEM") );
		ERROR(tmp_buf);
	    }

	    if ( STlength(fmt) + 3 * STlength(iisystem) + STlength(tmp_buf) >
		 BIG_ENOUGH )
	    {
		/* II_SYSTEM value is too long for us to format command line */
		STprintf( tmp_buf,  ERget( S_ST05A0_value_too_long ),
			  ERx("II_SYSTEM") );
		ERROR(tmp_buf);
	    }
	    STprintf( tmp_buf1, fmt, iisystem, iisystem, tmp_buf );

	    for ( nodenum = 1, mask = RemoteMask;
		  0 != (mask >>= 1);
		  nodenum++ )
	    {
		if ( !(mask & 1) )
		{
		    /* Node unconfigured/unspecified */
		    continue;
		}

		if ( (p1 == NULL) && cluster_node_up(nodenum) )
		{
		    /* Node appears to be up */
		    continue;
		}

		/*
		** Operating against remote node.
		**
		** Need full path to iirsh, since remote environment
		** may not have correct PATH.  'iirsh' will set up
		** ingres environment based on II_SYSTEM value passed
		** as its first parameter.
		**
		** Need full path to ingstart as iirun parameter,
		** since otherwise iirun will construct a path to
		** 'bin' directory.
		*/
		status = rpc_execute( nodenum, tmp_buf1 );

		if ( OK != status )
		{
		    STprintf( tmp_buf2, ERx( "%.*s" ), CX_MAX_NODE_NAME_LEN,
		      ClusterConfiguration->cx_nodes[nodenum-1].cx_node_name);
		    STprintf( tmp_buf, ERget( S_ST05A1_fail_to_start_node ),
			      SystemExecName, tmp_buf2 );
		    WARNING(tmp_buf);
		}
	    } /* for */

	    /* Remove these bits from total set of nodes to process. */
	    NodeMask &= ~RemoteMask;

# ifdef UNIX
	    if ( removecsshm ) execute( ERx("ipcclean >& /dev/null") );
# endif
	    if ( !NodeMask )
	    {
		PCexit(OK);
	    }
	}

        STprintf(tmp_buf,ERx("%sstart"), SystemExecName );
	PMsetDefault( II_PM_IDX, SystemCfgPrefix );
	PMsetDefault( HOST_PM_IDX, host );
	PMsetDefault( OWNER_PM_IDX, tmp_buf );

	if( p1 != NULL && !(parammix & PARAM_MASK_CLIENT) && 
	    ( startup_mask & ( PCM_DBMS|PCM_GCC|PCM_GCD|PCM_GCB|
              PCM_ICE|PCM_ORACLE|PCM_INFORMIX|PCM_MSSQL|PCM_ODBC|PCM_SYBASE|
	      PCM_RMS|PCM_RDB|PCM_DB2UDB|PCM_STAR ) ) )
	{
		if( p2 == NULL )
			p2 =  ERx( "*" );

		if ( startup_mask & ( PCM_ORACLE|PCM_INFORMIX|PCM_MSSQL|
		      PCM_SYBASE|PCM_DB2UDB|PCM_RDB ) )
		{
		   STprintf( tmp_buf, ERx( "%s.%s.gateway.%s.%%" ),
		     SystemCfgPrefix, host, p1 );
		}
		else
		{
		   STprintf( tmp_buf, ERx( "%s.%s.%s.%s.%%" ), SystemCfgPrefix, 
			     host, p1, p2 );
		}
		temp = PMexpToRegExp( tmp_buf );
		state.last=0;
		if( PMscan( temp, &state, NULL, &temp, &temp ) != OK )
			ERROR( ERget( S_ST0518_no_definition ) ); 
	}

	/*
	** Check for the nameserver, IF ingstart is called w/o parameters,
	** or if we're explicitly starting the name server, or if we're
	** running with NUMA clusters or PARAM_MASK_CLIENT.
	*/
	if( ( NUMAclustered || (startup_mask & PCM_GCN) ) &&
	      ping_iigcn() == OK )
	{
	    havegcn = TRUE;
	    startup_mask &= ~PCM_GCN;
	    if ( (!RemoteMask &&
	         (!NUMAclustered || (NUMAclustered && !NodeMask))) )
	    {
		STprintf( tmp_buf, ERget( S_ST0519_system_running ),
			  SystemProductName, SystemExecName );
		ERROR( tmp_buf ); 
	    }
	    else
	    {
		report_node_is_up( 0 );
		PCexit( FAIL );
	    }
	}

	if ( p1 == NULL && NUMAclustered )
	{
	    if ( NodeMask )
	    {
		/* Turn off all but rad specific components & GCN */
		startup_mask &= PCM_ALL_RAD_SPECIFIC | PCM_GCN;
	    }
	    else
	    {
		/* 
		** If no nodes specified then generate a set of all local nodes.
		*/
		add_to_node_mask(host, NULL, &NodeMask, &RemoteMask);
		NodeMask &= ~RemoteMask;
	    }
	}

	/*
	** On Unix, check the logging system too
	*/
# ifdef UNIX
	/*
	** Despite the above comment, this is just checking the GCN again.
	*/
        if ( !havegcn && p1 == NULL && sysstat() != OK )
	{
		STprintf( tmp_buf, ERget( S_ST0519_system_running ),
			  SystemProductName, SystemExecName );
		ERROR( tmp_buf ); 
	}
# endif /* UNIX */

	if( p1 == NULL ) {
		STprintf(tmp_buf, "\n%s/%sstart\n", SystemProductName, SystemExecName );
		PRINT(tmp_buf);
	}
# ifdef SOLARIS_PATCH

NMgtAt( ERx( "II_GC_PORT" ), &ii_gc_port );

if( ii_gc_port && *ii_gc_port )
  CVan( ii_gc_port, &ii_gc_port_val );
else
  ii_gc_port = NULL;

# endif /* SOLARIS_PATCH */

	/*
	** NUMA Cluster processing.
	**
	** If running configured for virtual nodes on one NUMA
	** Archtecture box, the next section checks to see if
	** we have one target RAD, or multiple target RADs.
	**
	** If multiple target RADs, the GCN is started if required,
	** and 'ingstart' is recursively executed with arguments
	** specifying one single RAD.
	**
	** If only one RAD is specified, then we continue on past
	** this section, and perform all the actual non-GCN startups.
	*/
	if ( NUMAclustered && (startup_mask & PCM_ALL_RAD_SPECIFIC) )
	{
	    /*
	    ** Count local nodes being started, and note 1st nodenum.
	    */
	    mask = NodeMask;
	    j = nodenum = 0;
	    while ( mask )
	    {
		mask >>= 1;
		if ( 0 == j ) nodenum++;
		if ( mask & 1 ) j++;
	    }

	    if ( j > 1 )
	    {
		/*
		** All RAD specific startups will be done in the context
		** of a recursive call to ingstart.  The parent ingstart
		** then takes care of all the shared processes (except
		** iigcn, which is started by the parent prior to the
		** recursive instances).
		*/
		STprintf(tmp_buf, ERx("%sstart -rad=%%d"), SystemExecName);
		if ( p1 )
		{
		    STpolycat(2, " ", p1, tmp_buf);
		    if ( p2 ) STpolycat(2, "=", p2, tmp_buf);
		}

		if ( startup_mask & PCM_GCN )
		{
		    if( start_iigcn() != OK )
		    {
			STprintf( tmp_buf1,
				  ERget( S_ST0526_gcn_failed ),
				  SystemProductName,
				  SystemExecName,
				  SystemProductName );
			ERROR( tmp_buf1 );
		    }
		    startup_mask &= ~PCM_GCN;
		    havegcn = TRUE;
		}

		for ( nodenum = 1, mask = NodeMask;
		      0 != ( mask >>= 1 );
		      nodenum++ )
		{
		    if ( !(mask & 1) )
		    {
			/* Node unspecified */
			continue;
		    }
# ifndef VMS
# ifndef NT_GENERIC
		    if ( j )
		    {
			/*
			** Make sure we have at least one shared
			** memory segment available, so that 
			** 'cluster_node_up' can do its job.
			** 
			** If one already exists, no big deal.
			*/
			RadID =
			 ClusterConfiguration->cx_nodes[nodenum-1].cx_rad_id;
			STprintf(Affine_Always, ERx( " -rad=%d" ), RadID );
			Affine_Clust = Affine_Always;
			STprintf( tmp_buf1,
			   ERx( "csinstall %s - >/dev/null" ),
			   Affine_Always );
			(void)execute(tmp_buf1);
		    }
# endif /* NT_GENERIC */
# endif /* VMS */

		    if ( (NULL == p1) && cluster_node_up(nodenum) )
		    {
			continue;
		    }

		    STprintf(tmp_buf1, tmp_buf, 
		     ClusterConfiguration->cx_nodes[nodenum-1].cx_rad_id);
		    if ( j )
			STcat( tmp_buf1, ERx( " -csinstalled" ) );

		    if ( OK != execute(tmp_buf1) )
		    {
			STprintf( tmp_buf2, ERx( "%.*s" ), CX_MAX_NODE_NAME_LEN,
			  ClusterConfiguration->cx_nodes[nodenum-1].cx_node_name);
			STprintf( tmp_buf1,
			      ERget( S_ST05A1_fail_to_start_node ),
			      SystemExecName, tmp_buf2 );
			WARNING( tmp_buf1 );
		    }
		    j = 0;
		}

		/* Turn off all RAD specific components */
		startup_mask &= ~PCM_ALL_RAD_SPECIFIC;
		if ( !startup_mask )
		    PCexit(OK);
	    }
	    else if ( nodenum )
	    {
		RadID = ClusterConfiguration->cx_nodes[nodenum-1].cx_rad_id;
	    }
	    else
	    {
		/* How'd this happen?! Nothing to do anyway */
		PCexit(OK);
	    }
	}

	/*
	** From this point on, we are operating on one Ingres instance,
	** or starting the common components for a NUMA cluster host.
	*/
	if ( RadID )
	{
	    STprintf(Affine_Always, ERx( " -rad=%d" ), RadID);

	    if ( NUMAclustered )
	    {
		Affine_Clust = Affine_Some = Affine_Always;
	    }
	    else if ( p1 != NULL )
	    {
		/* Affine any component if specifically asked */
		Affine_Some = Affine_Always;
	    }

	    if ( !NUMAclustered || (startup_mask & PCM_ALL_RAD_SPECIFIC) )
	    {
		STprintf(tmp_buf, ERget( S_ST05A4_setting_affinity ),
		 SystemProductName, RadID );
		PRINT(tmp_buf);
	    }
	}

	if ( !RadID || !NUMAclustered ||
	     !(startup_mask & PCM_ALL_RAD_SPECIFIC) )
	{
	    (void)CXset_context(host, 0);
	}
	else
	{
	    (void)CXset_context(NULL, RadID);
	}
	
	if ( NUMAclustered && NULL == p1 &&
	     (startup_mask & PCM_ALL_RAD_SPECIFIC) ) 
	{
	    STprintf( tmp_buf1, ERx( "csinstall %s - >/dev/null" ),
		       Affine_Always );
	    if ( OK == execute(tmp_buf1) )
		Have_Sysmem_Seg = TRUE;
	    if ( cluster_node_up(nodenum) )
		PCexit(FAIL);
	}

	/* build DBMS server startup list */
	build_startup_list( &dbms_list, ERx( "dbms" ) );	

	/* build Net server startup list */
	build_startup_list( &net_list, ERx( "gcc" ) );	

	/* build Bridge server startup list */
	build_startup_list( &bridge_list, ERx( "gcb" ) );

	/* build Net server startup list */
	build_startup_list( &das_list, ERx( "gcd" ) );	

	/* build Star server startup list */
	build_startup_list( &star_list, ERx( "star" ) );	

#if defined(conf_BUILD_ICE)
	/* build ice server startup list */
	build_startup_list( &icesvr_list, ERx( "icesvr" ) );
#endif

	/* build Oracle Gateway startup list */
	build_startup_list( &oracle_list, ERx( "oracle" ) );	

	/* build Informix Gateway startup list */
	build_startup_list( &informix_list, ERx( "informix" ) );	

	/* build Sybase SQL Server Gateway startup list */
	build_startup_list( &sybase_list, ERx( "sybase" ) );	

	/* build RMS Gateway startup list */
	build_startup_list( &rms_list, ERx( "rms" ) );	

	/* build RDB Gateway startup list */
	build_startup_list( &rdb_list, ERx( "rdb" ) );	

	/* build rmcmd server startup list */
	build_startup_list( &rmcmd_list, ERx( "rmcmd" ) );

	/* build db2udb server startup list */
	build_startup_list( &db2udb_list, ERx( "db2udb" ) );

# ifdef NT_GENERIC
	/* build Microsoft SQL Server Gateway startup list */
	build_startup_list( &mssql_list, ERx( "mssql" ) );	

	/* build ODBC Gateway startup list */
	build_startup_list( &odbc_list, ERx( "odbc" ) );	

	/* build Gateway server startup list */
	if (Is_w95)
		build_startup_list( &gateway_list, ERx( "iigws" ) );
# endif

	if( p1 == NULL &&
		startup_total( dbms_list ) == 0 &&
		startup_total( star_list ) > 0 )
	{
		ERROR( ERget( S_ST051B_star_needs_dbms ) ); 
	}

#if defined(conf_BUILD_ICE)
	if( p1 == NULL &&
		startup_total( dbms_list ) == 0 &&
		startup_total( icesvr_list ) > 0 )
	{
		ERROR( ERget( S_ST0575_icesvr_needs_dbms ) );
	}
#endif

	if( p1 == NULL &&
		startup_total( dbms_list ) == 0 &&
# ifdef NT_GENERIC
		Is_w95 && 
		startup_total( gateway_list ) == 0 &&
# endif
		startup_total( net_list ) == 0 )
	{
		STprintf( tmp_buf, ERget( S_ST051C_nothing_configured ),
			  SystemProductName );
		ERROR( tmp_buf ); 
	}

# ifdef VMS
	/* Set privileges specified in config.dat */ 
	STprintf( tmp_buf, "%s.$.INGSTART.PRIVILEGES", SystemCfgPrefix );
	if( PMget( ERx( "!.privileges" ), &privileges ) != OK )
		F_ERROR( "%s not found.", PMexpandRequest( 
			 tmp_buf ) );

	/* parse privileges to build mask */
	for( p = name = STalloc( privileges ); *p != EOS; CMnext( p ) )
	{
		/* replace non-alpha with space for STgetwords() */
		if( !CMalpha( p ) )
			*p = ' ';
	}
	npriv = BIG_ENOUGH;
	STgetwords( name, &npriv, privs ); 

	PRINT( ERget( S_ST051D_setting_privileges ) ); 

	/* create 64-bit privilege mask for sys$setprv() */
	privmask[ 0 ] = 0; privmask[ 1 ] = 0;
	for( i = 0; i < npriv; i++ )
	{
		i4 j, k;
		bool set;

		/* force privilege name to upper case */
		CVupper(  privs[ i ] );

		set = FALSE;
		for( j = 0; j < 2; j++ )
		{
			/* repeat twice */
			for( k = 0; k < fill[ j ]; k++ )
			{
				/* assumes both are upper case! */
				if( STequal( order[ j ][ k ], privs[ i ] )
					!= 0 )
				{
					F_PRINT( "(\"%s\")\n",
						privs[ i ] );
					set = TRUE;
					privmask[ j ] |= 1L<< k; 
					break;
				}
			}
		}
		if( !set )
			F_PRINT( ERget( S_ST051E_unknown_privilege ), 
				privs[ i ] );
	}

	/* NOTE: VMS privileges must be set before any call to PCcmdline() */
	if( sys$setprv( 1, privmask, 1, NULL ) != SS$_NORMAL )
		ERROR( ERget( S_ST051F_privileges_failed ) ); 
# endif /* VMS */

	/* call system resource checker specified in config.dat */
	STprintf( tmp_buf, ERx( "%s.$.%sstart.syscheck_mode" ), 
		  SystemCfgPrefix, SystemExecName );
	STprintf( tmp_buf1, ERx( "%s.*.%sstart.syscheck_command" ),
		  SystemCfgPrefix, SystemExecName );
	if( p1 == NULL && 
	    PMget( tmp_buf, &syscheck ) == OK &&
	    ValueIsBoolTrue( syscheck ) && 
            PMget( tmp_buf1, &syscheck ) == OK )
	{
		/* check for sufficient system resources
		if syscheck can't read /dev/kmem don't fail */
		scretval=execute( syscheck );
		if ( scretval != OK && scretval != RL_KMEM_OPEN_FAIL )
			PCexit( FAIL );
	}
	else if( p1 == NULL )
		PRINT( "\n" );

	if( p1 == NULL )
	{
	    if ( Clustered )
		STprintf( tmp_buf, ERget( S_ST05A7_starting_system_on ),
			  SystemProductName, CXnode_name(NULL) );
	    else
		STprintf( tmp_buf, ERget( S_ST0520_starting_installation ),
			  SystemProductName );
	    PRINT( tmp_buf ); 
	}

# ifdef NT_GENERIC
    /* clean up old shared memory possibly left lying around */ 
    if (startup_total(dbms_list) > 0 &&
	execute(ERx("rcpstat -silent -online")) != OK)
    {
	i4 pcsleep;
	for (pcsleep=0; pcsleep <= 10;  pcsleep++)
	{
   	    if ( execute( ERx( "ipcclean.exe" ) ) )
	    {
		if (pcsleep == 10)
		{
	    	    STprintf( tmp_buf, ERget( S_ST0519_system_running ),
		          SystemProductName, SystemExecName ); 
	    	    ERROR( tmp_buf ); 
		}
		PCsleep(5000);
	    }
	    else
	    {
		break;
            }
        }
    }

    if (as_service)
    {
	/*
	** Figure out if the service is started.
	*/
	if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
	    != NULL)
	{
	    char	*ii_install;

	    F_PRINT( ERget( S_ST0654_start_service ), SystemProductName );
	    NMgtAt( ERx( "II_INSTALLATION" ), &ii_install );
	    if( ii_install == NULL || *ii_install == EOS )
		ii_install = ERx( "" );
	    else
		ii_install = STalloc( ii_install ); 

	    STlpolycat(3, sizeof(ServiceName)-1, SystemServiceName,
			"_Database_", ii_install, &ServiceName[0]);
	    
	    OpIngSvcHandle = OpenService(schSCManager, ServiceName,
					      SERVICE_QUERY_STATUS |
					      SERVICE_START);

	    if (OpIngSvcHandle == NULL)
	    {
		STlpolycat(3, sizeof(ServiceName)-1, SystemServiceName,
				"_DBATools_", ii_install, &ServiceName[0]);
		OpIngSvcHandle = OpenService(schSCManager, ServiceName,
						SERVICE_QUERY_STATUS |
						SERVICE_START);
	    }
	    if (OpIngSvcHandle != NULL)
	    {
		if (QueryServiceStatus(OpIngSvcHandle, lpssServiceStatus))
		{
		    if (ssServiceStatus.dwCurrentState == SERVICE_RUNNING)
		    {
			ServiceStarted = TRUE;
			PRINT("\n");
		    }
		    else
		    {
			char	*ServiceArgs[1], buf[256];
			DWORD	dwState;
			char	*ResourceName;

			hCluster = OpenCluster(NULL);
			if (hCluster)
			{
			    NMgtAt( ERx( "II_CLUSTER_RESOURCE" ), &ResourceName );
			    mbstowcs(lpwResourceName, ResourceName, 256);
			    hResource = OpenClusterResource(hCluster, lpwResourceName);
			    if (hResource)
			    {
				OnlineClusterResource(hResource);
				while (1)
				{
				    dwState=GetClusterResourceState(hResource, NULL, NULL, NULL, NULL);
				    if (dwState == ClusterResourceInitializing ||
					    dwState == ClusterResourcePending ||
					    dwState == ClusterResourceOnlinePending)
				    {
					PRINT(".");
					PCsleep(1000);
				    }
				    else if (dwState == ClusterResourceOnline)
				    {
					PRINT(ERget(S_ST06B6_online_successfully));
					PCexit(OK);
				    }
				    else
				    {
					PRINT(ERget(S_ST06B7_online_fail));
					PCexit(FAIL);
				    }
				}
			    }
			}

			/*
			** Let's start the service. However, tell it not to
			** run 'ingstart', since that is us.
			*/
			STcopy("NoIngstart", buf);
			ServiceArgs[0] = buf;
			if (!StartService(OpIngSvcHandle, 1,
					  (LPCTSTR *)&ServiceArgs))
			{
			    int	rc;

			    OsError = GetLastError();
			    rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
					     FORMAT_MESSAGE_ARGUMENT_ARRAY |
					     FORMAT_MESSAGE_ALLOCATE_BUFFER,
					     NULL,
					     OsError,
					     LANG_NEUTRAL,
					     (char *)&OsErrText,
					     0,
					     NULL);
			    if (rc)
			    {
				F_PRINT3( ERget( S_ST0655_start_svc_fail ),
					  SystemProductName, OsError,
					  OsErrText );
			    }
			    else
			    {
				F_PRINT3( ERget( S_ST0655_start_svc_fail ),
					  SystemProductName, OsError,
					  "unknown" );
			    }

			    CloseServiceHandle(OpIngSvcHandle);
			    CloseServiceHandle(schSCManager);
			    PCexit(FAIL);
			}
			else
			{
			    while (QueryServiceStatus(OpIngSvcHandle,
						      lpssServiceStatus) &&
				   ssServiceStatus.dwCurrentState !=
					SERVICE_RUNNING &&
				   ssServiceStatus.dwCurrentState !=
					SERVICE_STOPPED)
			    {
				PRINT(".");
				PCsleep(1000);
			    }
			    PRINT("\n");

			    if (ssServiceStatus.dwCurrentState ==
				SERVICE_STOPPED)
			    {
				int	rc;

				OsError = ssServiceStatus.dwWin32ExitCode;
				rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
						 FORMAT_MESSAGE_ARGUMENT_ARRAY |
						 FORMAT_MESSAGE_ALLOCATE_BUFFER,
						 NULL,
						 OsError,
						 LANG_NEUTRAL,
						 (char *)&OsErrText,
						 0,
						 NULL);
				if (rc)
				{
				    F_PRINT3( ERget( S_ST0655_start_svc_fail ),
					      SystemProductName, OsError,
					      OsErrText );
				}
				else
				{
				    F_PRINT3( ERget( S_ST0655_start_svc_fail ),
					      SystemProductName, OsError,
					      "unknown" );
				}

				CloseServiceHandle(OpIngSvcHandle);
				CloseServiceHandle(schSCManager);
				PCexit(FAIL);
			    }

			    ServiceStarted = TRUE;
			}
		    }
		}
		else
		{
		    int	rc;

		    OsError = GetLastError();
		    rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
				     FORMAT_MESSAGE_ARGUMENT_ARRAY |
				     FORMAT_MESSAGE_ALLOCATE_BUFFER,
				     NULL,
				     OsError,
				     LANG_NEUTRAL,
				     (char *)&OsErrText,
				     0,
				     NULL);
		    if (rc)
		    {
			F_PRINT3( ERget( S_ST0655_start_svc_fail ),
					 SystemProductName, OsError,
					 OsErrText );
		    }
		    else
		    {
			F_PRINT3( ERget( S_ST0655_start_svc_fail ),
				  SystemProductName, OsError,
				  "unknown" );
		    }

		    CloseServiceHandle(OpIngSvcHandle);
		    CloseServiceHandle(schSCManager);
		    PCexit(FAIL);
		}

		CloseServiceHandle(OpIngSvcHandle);
	    }
	    else
	    {
		int	rc;

		OsError = GetLastError();
		rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
				 FORMAT_MESSAGE_ARGUMENT_ARRAY |
				 FORMAT_MESSAGE_ALLOCATE_BUFFER,
				 NULL,
				 OsError,
				 LANG_NEUTRAL,
				 (char *)&OsErrText,
				 0,
				 NULL);
		if (rc)
		{
		    F_PRINT3( ERget( S_ST0655_start_svc_fail ),
				     SystemProductName, OsError, OsErrText );
		}
		else
		{
		    F_PRINT3( ERget( S_ST0655_start_svc_fail ),
			      SystemProductName, OsError, "unknown" );
		}

		CloseServiceHandle(schSCManager);
		PCexit(FAIL);
	    }

	    CloseServiceHandle(schSCManager);
	}
	else
	{
	    int	rc;

	    OsError = GetLastError();
	    rc=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			     FORMAT_MESSAGE_ARGUMENT_ARRAY |
			     FORMAT_MESSAGE_ALLOCATE_BUFFER,
			     NULL,
			     OsError,
			     LANG_NEUTRAL,
			     (char *)&OsErrText,
			     0,
			     NULL);
	    if (rc)
	    {
		F_PRINT3( ERget( S_ST0655_start_svc_fail ),
			  SystemProductName, OsError, OsErrText );
	    }
	    else
	    {
		F_PRINT3( ERget( S_ST0655_start_svc_fail ),
			  SystemProductName, OsError, "unknown" );
	    }

	    PCexit(FAIL);
	}
    }
# endif /* NT_GENERIC */

# ifdef VMS
	/* look up startup uic id */
	STprintf( tmp_buf, ERx( "%s.$.*.*.vms_uic" ), SystemCfgPrefix );
	if( PMget( tmp_buf, &uic ) != OK )
		F_ERROR( "%s not found in CONFIG.DAT",
			PMexpandRequest( ERx( "!.$.*.*.vms_uic" ) ) );

	/* compose string descriptor for job logical table */
	lnm_table.dsc$w_length = STlength( ERx( "LNM$JOB" ) );
	lnm_table.dsc$a_pointer = ERx( "LNM$JOB" ); 
	lnm_table.dsc$b_class = 0;
	lnm_table.dsc$b_dtype = 0;

	lnm_name.dsc$w_length = STlength( SystemLocationVariable );
	lnm_name.dsc$a_pointer = SystemLocationVariable;
	lnm_name.dsc$b_class = 0;
	lnm_name.dsc$b_dtype = 0;

        /* deassign supervisory-mode job logical II_SYSTEM */
        status = sys$dellnm( &lnm_table, &lnm_name, &access_mode );

        if( status != SS$_NORMAL && status != SS$_NOLOGNAM )
	{
		STprintf( tmp_buf, "%s %s.", ERget( S_ST057F_unable_deassign ),
			  SystemLocationVariable );
		ERROR( tmp_buf );
	}

	/* get logical table name */ 
	STprintf( tmp_buf, ERx( "%s.$.lnm.table.id" ), SystemCfgPrefix );
	if( PMget( tmp_buf, &lnm_table_name ) != OK )
	{
		STprintf( tmp_buf, ERx( "%s.$.lnm.table.id" ), 
			  SystemCfgPrefix );
		F_ERROR( "%s not found.", PMexpandRequest( tmp_buf ) );
	}

	/* compose string descriptor for system or group logical table */
	lnm_table.dsc$w_length = STlength( lnm_table_name );
	lnm_table.dsc$a_pointer = lnm_table_name; 
	lnm_table.dsc$b_class = 0;
	lnm_table.dsc$b_dtype = 0;

	F_PRINT( ERget( S_ST0521_defining_logicals ), lnm_table_name );

	STprintf( tmp_buf, ERx( "%s.$.lnm.%%" ), SystemCfgPrefix );
	regexp = PMexpToRegExp( tmp_buf );
	for(
		status = PMscan( regexp, &state, NULL, &name,
		&value ); status == OK; status = PMscan(
		NULL, &state, NULL, &name, &value ) )
	{
		ILE3 item_list[ 2 ];
		STATUS osstatus;

		name = PMgetElem( 3, name );

		/* force logical name to upper case */
		CVupper( name );

		lnm_name.dsc$w_length = STlength( name );
		lnm_name.dsc$a_pointer = name;
		lnm_name.dsc$b_class = 0;
		lnm_name.dsc$b_dtype = 0;

		item_list[ 0 ].ile3$w_length = STlength( value );
		item_list[ 0 ].ile3$w_code = LNM$_STRING; 
		item_list[ 0 ].ile3$ps_bufaddr = value; 
		item_list[ 0 ].ile3$ps_retlen_addr = NULL; 
		item_list[ 1 ].ile3$w_length = 0;
		item_list[ 1 ].ile3$w_code = 0;

		osstatus = sys$crelnm( NULL, &lnm_table, &lnm_name,
			&access_mode, item_list );

		if( osstatus != SS$_NORMAL && osstatus != SS$_SUPERSEDE )
		{
			SIfprintf( stderr, ERget( S_ST0522_crelnm_failed ), 
				name, value );
			PCexit( FAIL );
		}

		STprintf(tmp_buf, ERx( "\"%s\" = \"%s\"\n" ), name, value );
		PRINT(tmp_buf);
	}
# endif /* VMS */

	/* look up the installation code */
	NMgtAt( ERx( "II_INSTALLATION" ), &temp );
	if ( NUMAclustered && (startup_mask & PCM_ALL_RAD_SPECIFIC) )
	{
	    if ( NULL == temp || STlength(temp) != 2 )
	    {
		/* Fatal for NUMA clusters */
		STprintf( tmp_buf, ERget( S_ST0580_iisystem_not_set ),
			  ERx( "II_INSTALLATION" ) );
		ERROR( tmp_buf );
	    }
	    STprintf( ii_installation, "%s.%d", temp, RadID );
	}
	else if (temp)
	{
	    ii_installation[
	     STlcopy(temp, ii_installation, sizeof(ii_installation) - 1) ] =
	     '\0';
	}
	else
	{
	    ii_installation[0] = '\0';
	}
	    
	if (parammix & PARAM_MASK_CLIENT)
	{
		if( start_iigcn() != OK )
		{
			STprintf( tmp_buf,
				  ERget( S_ST0526_gcn_failed ),
				  SystemProductName,
				  SystemExecName,
				  SystemProductName );
			ERROR( tmp_buf );
		}
		PCsleep (3000);
		for( cur = net_list; cur != NULL; cur = cur->next )
		{
			for( i = 0; i < cur->count; i++ )
			{
			if( start_iigcc( cur->name ) != OK )
				ERROR( ERget( S_ST0524_gcc_failed ) ); 
			}
		}
		/* 
		** Please do not alter this string (mcgem01)
		*/
		STprintf (tmp_buf, "\n%s %s\n", SystemProductName,
		    ERget( S_ST057E_started_sucessfully ) );
		PRINT(tmp_buf);
		PCexit(OK);
	}
	else if( p1 != NULL )
	{
		if( startup_mask & PCM_DBMS )
		{
			if( start_iidbms( p2 ) != OK )
				ERROR( ERget( S_ST0523_dbms_failed ) ); 
		}
		else if( startup_mask & PCM_GCC )
		{
			if( start_iigcc( p2 ) != OK )
				ERROR( ERget( S_ST0524_gcc_failed ) ); 
		}
		else if( startup_mask & PCM_GCB )
		{
			if( start_iigcb( p2 ) != OK )
			ERROR( ERget( S_ST052E_gcb_failed ) );
		}
		else if( startup_mask & PCM_GCD )
		{
			if( start_iigcd( p2 ) != OK )
				ERROR( ERget( S_ST0694_das_failed ) ); 
		}
		else if( startup_mask & PCM_STAR )
		{
			if( start_iistar( p2 ) != OK )
				ERROR( ERget( S_ST0525_star_failed ) ); 
		}
		else if( startup_mask & PCM_ACP )
		{
			if( start_dmfacp() != OK )
				ERROR( ERget( S_ST0514_dmfacp_failed ) ); 
		}
		else if( startup_mask & PCM_RCP )
		{
#ifndef VMS
#ifndef NT_GENERIC 
#ifdef UNIX
			if (checkcsclean() && (execute( ERx( "rcpstat -silent -online" ) ) == OK))
			{
                                 PRINT("RCP server already started\n");
			}
			else
			{
				execute( ERx( "ipcclean" ) );
                        	PRINT( ERget( S_ST0511_csinstalling ) );
                        	if( execute( ERx( "csinstall -" ) ) != OK )
                               		ERROR( ERget( S_ST0512_csinstall_failed ) );
				if( start_dmfrcp() != OK )
                                	ERROR( ERget( S_ST0513_dmfrcp_failed ) );
			}
#else
		    if ( !Have_Sysmem_Seg )
		    {
			STprintf( tmp_buf, ERx( "ipcclean%s" ), Affine_Clust );
			(void) execute( tmp_buf );
			PRINT( ERget( S_ST0511_csinstalling ) );
			STprintf( tmp_buf, ERx( "csinstall%s -" ),
			   Affine_Always );
			if( execute( tmp_buf ) != OK )
				ERROR( ERget( S_ST0512_csinstall_failed ) );
		    }
#endif/*UNIX*/
#endif /* ~NT_GENERIC */
# endif /* VMS */
#ifndef UNIX
			if( start_dmfrcp() != OK )
				ERROR( ERget( S_ST0513_dmfrcp_failed ) );
#endif/*UNIX*/
		}
		else if( startup_mask & PCM_GCN )
		{
			if( start_iigcn() != OK )
			{
				STprintf( tmp_buf,
					  ERget( S_ST0526_gcn_failed ),
					  SystemProductName,
					  SystemExecName,
					  SystemProductName );
				ERROR( tmp_buf );
			}
		}
#if defined(conf_BUILD_ICE)
		else if( startup_mask & PCM_ICE )
		{
			if( start_icesvr( p2 ) != OK )
				ERROR( ERget( S_ST0576_icesvr_failed ) );
		}
#endif
		else if( startup_mask & PCM_ORACLE )
		{
			if( start_oracle( p2 ) != OK )
				ERROR( ERget( S_ST0555_oracle_failed ) );
		}
		else if( startup_mask & PCM_INFORMIX )
		{
			if( start_informix( p2 ) != OK )
				ERROR( ERget( S_ST0556_informix_failed ) );
		}
		else if( startup_mask & PCM_MSSQL )
		{
			if( start_mssql( p2 ) != OK )
				ERROR( ERget( S_ST0557_mssql_failed ) );
		}
		else if( startup_mask & PCM_ODBC )
		{
			if( start_odbc( p2 ) != OK )
				ERROR( ERget( S_ST0559_odbc_failed ) );
		}
		else if( startup_mask & PCM_SYBASE )
		{
			if( start_sybase( p2 ) != OK )
				ERROR( ERget( S_ST0558_sybase_failed ) );
		}
		else if( startup_mask & PCM_RMCMD )
                {
                    if( start_rmcmd() != OK )
                        ERROR( ERget( S_ST0538_rmcmd_failed ) );
                }
		else if( startup_mask & PCM_RMS )
		{
			if( start_rms( p2 ) != OK )
				ERROR( ERget( S_ST0641_rms_failed ) );
		}
		else if( startup_mask & PCM_RDB )
		{
			if( start_rdb( p2 ) != OK )
				ERROR( ERget( S_ST0646_rdb_failed ) );
		}
		else if( startup_mask & PCM_DB2UDB )
		{
			if( start_db2udb( p2 ) != OK )
				ERROR( ERget( S_ST0674_db2udb_failed ) );
		}
# ifdef NT_GENERIC
		else if( STbcompare( p1, 0, ERx( "client" ), 0, TRUE ) == 0 )
		{
			if( start_iigcn() != OK )
			{
				STprintf( tmp_buf,
					  ERget( S_ST0526_gcn_failed ),
					  SystemProductName,
					  SystemExecName,
					  SystemProductName );
				ERROR( tmp_buf );
			}
			PCsleep (3000);
			for( cur = net_list; cur != NULL; cur = cur->next )
			{
				for( i = 0; i < cur->count; i++ )
				{
				if( start_iigcc( cur->name ) != OK )
					ERROR( ERget( S_ST0524_gcc_failed ) ); 
				}
			}
			/* 
			** Please do not alter this string (mcgem01)
			*/
			STprintf (tmp_buf, "\n%s %s\n", SystemProductName,
			    ERget( S_ST057E_started_sucessfully ) );
			PRINT(tmp_buf);
		}
		else if( startup_mask & PCM_IIGWS )
		{
			if( start_iigws() != OK )
				ERROR( ERget( S_ST0531_gws_failed ) );
		}
# endif /* NT_GENERIC */
		PRINT( "\n" );
		PCexit( OK );
	}

	if ( Clustered )
	{
		STprintf( tmp_buf, ERget( S_ST0527_not_clustered ),
			  SystemProductName );
		PRINT( tmp_buf );

		/* block other invocations for critical sections */ 
		ersatz_V();
	}

	/*
	** Run program(s) specified in config.dat by "ii.*.ingstart.%.begin"
	** before launching standard OpenIngres processes.
	*/ 
	STprintf( tmp_buf, ERx( "%s.$.%sstart.%%.begin" ), SystemCfgPrefix,
		SystemExecName );
	for(
		status = PMscan( PMexpToRegExp( tmp_buf ), 
				  &state, NULL, &name, &value ); 
		status == OK; 
		status = PMscan( NULL, &state, NULL, &name, &value ) )
	{
		if( execute( value ) != OK )
			return( FAIL );
	}

	/* end criticial section */
	if( Clustered )
	    ersatz_P();

	if( !havegcn && start_iigcn() != OK )
	{
		STprintf( tmp_buf,
		  ERget( S_ST0526_gcn_failed ),
		  SystemProductName,
		  SystemExecName,
		  SystemProductName );
		ERROR( tmp_buf );
	}

	/* wait for name server to wake up */
	wait_iigcn();

	if( (startup_mask & PCM_DBMS) && startup_total( dbms_list ) > 0 )
		start_logging();

	if ( startup_mask & PCM_DBMS )
	{
	    for( cur = dbms_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_iidbms( cur->name ) != OK )
				    ERROR( ERget( S_ST0523_dbms_failed ) ); 
		    }
	    }
	}

	if ( startup_mask & PCM_GCC )
	{
	    for( cur = net_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_iigcc( cur->name ) != OK )
				    ERROR( ERget( S_ST0524_gcc_failed ) ); 
		    }
	    }
	}

	if ( startup_mask & PCM_GCB )
	{
	    for( cur = bridge_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_iigcb( cur->name ) != OK )
				    ERROR( ERget( S_ST052E_gcb_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_GCD )
	{
	    for( cur = das_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_iigcd( cur->name ) != OK )
				    ERROR( ERget( S_ST0694_das_failed ) );
		    }
	    }
	}

#if defined(conf_BUILD_ICE)
	if ( startup_mask & PCM_ICE )
	{
	    for( cur = icesvr_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_icesvr( cur->name ) != OK )
				    ERROR( ERget( S_ST0576_icesvr_failed ) );
		    }
	    }
	}
#endif

	if ( startup_mask & PCM_ORACLE )
	{
	    for( cur = oracle_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_oracle( cur->name ) != OK )
				    ERROR( ERget( S_ST0555_oracle_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_INFORMIX )
	{
	    for( cur = informix_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_informix( cur->name ) != OK )
				    ERROR( ERget( S_ST0556_informix_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_MSSQL )
	{
	    for( cur = mssql_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_mssql( cur->name ) != OK )
				    ERROR( ERget( S_ST0557_mssql_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_ODBC )
	{
	    for( cur = odbc_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_odbc( cur->name ) != OK )
				    ERROR( ERget( S_ST0559_odbc_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_SYBASE )
	{
	    for( cur = sybase_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_sybase( cur->name ) != OK )
				    ERROR( ERget( S_ST0558_sybase_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_STAR )
	{
	    for( cur = star_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_iistar( cur->name ) != OK )
				    ERROR( ERget( S_ST0525_star_failed ) ); 
		    }
	    }
	}

	if ( startup_mask & PCM_RMCMD )
	{
	    for( cur = rmcmd_list; cur != NULL; cur = cur->next )
	    {
		for( i = 0; i < cur->count; i++ )
		{
		    if( start_rmcmd( ) != OK )
			ERROR( ERget( S_ST0538_rmcmd_failed ) );
		}
	    }
	}

	if ( startup_mask & PCM_RMS )
	{
	    for( cur = rms_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_rms( cur->name ) != OK )
				    ERROR( ERget( S_ST0641_rms_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_RDB )
	{
	    for( cur = rdb_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_rdb( cur->name ) != OK )
				    ERROR( ERget( S_ST0646_rdb_failed ) );
		    }
	    }
	}

	if ( startup_mask & PCM_DB2UDB )
	{
	    for( cur = db2udb_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_db2udb( cur->name ) != OK )
				    ERROR( ERget( S_ST0674_db2udb_failed ) );
		    }
	    }
	}


# ifdef NT_GENERIC
	if ( startup_mask & PCM_IIGWS )
	{
	    for( cur = gateway_list; cur != NULL; cur = cur->next )
	    {
		    for( i = 0; i < cur->count; i++ )
		    {
			    if( start_iigws( ) != OK )
				    ERROR( ERget( S_ST0531_gws_failed ) ); 
		    }
	    }
	}
# endif
	PRINT( "\n" );
	/*
	** Run program(s) specified in config.dat by "ii.*.ingstart.%.end"
	** before exiting.
	*/ 
	STprintf( tmp_buf, ERx( "%s.$.%sstart.%%.end" ), SystemCfgPrefix,
		SystemExecName );
	for(
		status = PMscan( PMexpToRegExp( tmp_buf ), 
				  &state, NULL, &name, &value ); 
		status == OK; 
		status = PMscan( NULL, &state, NULL, &name, &value ) )
	{
		if( execute( value ) != OK )
			return( FAIL );
	}

	/* 
	** Please do not alter this string without my approval - emmag 
	*/
	STprintf (tmp_buf, "\n%s %s\n", SystemProductName,
		  ERget( S_ST057E_started_sucessfully ) );
	PRINT(tmp_buf);

	PCexit( OK );
}

#ifdef UNIX
/*
** checkcsclean -- Verify that shared memory is not already in use.
**
** Try to determine if the shared memory segment is already in use, so
** that we can, e.g., bail out of ingstart without doing an ipcclean
** if it appears that the server was not brought down cleanly.
**
** Inputs:
**	none
**
** Outputs:
**	FALSE	memory cannot be mapped or seems unused
**	TRUE	memory has >= 1 active servers registered
**
*/
static
i4
checkcsclean( void )
{
	CL_ERR_DESC	err_code;
	CS_SMCNTRL	*sysseg;
	CS_SERV_INFO	*svinfo;
	i4		i;
	
	if ( CS_map_sys_segment(&err_code) )
		return(FALSE);
	CS_get_cb_dbg(&sysseg);
	svinfo = sysseg->css_servinfo;
	for ( i = 0; i < sysseg->css_numservers; i++ )
	{
		if ( svinfo->csi_in_use )
			if ( svinfo->csi_pid )
				if ( PCis_alive( svinfo->csi_pid ) )
					return(TRUE);
		svinfo++;
	}
	return(FALSE);
}
#endif /* UNIX */

#ifdef UNIX
/*
** sysstat -- Verify that the system is not running.
**
** For now we do it like this: if $II_SYSTEM/ingres/bin/logstat doesn't
** exist or isn't readable, there's no system there at all.  If it exists,
** we run it; if it succeeds, the system's up, otherwise it's down.  This
** is pretty primitive.
*/
static i4
sysstat( void )
{
	CL_ERR_DESC cl_err;
	FILE *rfile;
	STATUS stat;
	char buf[MAX_LOC];
        STATUS rtrn;
	STpolycat( 4, SystemAltExecName, ERx( "gcfid " ), SystemAltExecName,
		   ERx( "gcn |grep . > /dev/null " ), buf );
	rtrn = PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err ) == OK ? 
		TRUE: FALSE ;
        return (rtrn );
}
#endif /* UNIX */

/*
** add_to_node_mask -- Build bitmasks of nodes to process.
**
** Utility routine used by main to process multiple -node & -rad parameters.
**
** Passed node name is verified, and if OK, node number for node is masked
** into the "nodemask", and if it is for a remote node, "remotemask" also.
**
** Inputs:
**	host	 - pointer to name of host (current node).
**	nodename - pointer to candidate node.
**	nodemask - pointer to bitmask of nodes to process.
**	remotemask - pointer to bitmask of nodes running on foreign boxes.
**
** Outputs:
**	none.   (Program Abends if a problem found)
*/
static void
add_to_node_mask( char *host, char *nodename, u_i4 *nodemask, u_i4 *remotemask )
{
    u_i4		i, result = FALSE;
    CX_NODE_INFO	*pni;
    char		 buf[128];

    for ( i = 1; i <= ClusterConfiguration->cx_node_cnt; i++ )
    {
	pni = ClusterConfiguration->cx_nodes + ClusterConfiguration->cx_xref[i];
	if ( ( NULL == nodename ) ||
	     ( 0 == STbcompare( nodename, 0,
			  pni->cx_node_name, pni->cx_node_name_l, TRUE ) ) )
	{
	    *nodemask |= 1 << pni->cx_node_number;
	    if ( 0 != STbcompare( host, 0, pni->cx_node_name,
				  pni->cx_node_name_l, TRUE ) &&
		 ( 0 == pni->cx_host_name_l ||
		   0 != STbcompare( host, 0, pni->cx_host_name,
				    pni->cx_host_name_l, TRUE ) ) )
		*remotemask |= 1 << pni->cx_node_number;
	    result = TRUE;
	    if ( NULL != nodename ) break;
	}
    }
    if ( !result )
    {
	if ( NULL == nodename ) nodename = "";
	STprintf( buf, ERget( S_ST05A2_bad_node ), nodename );
	ERROR( buf );
    }
} /* add_to_node_mask */
