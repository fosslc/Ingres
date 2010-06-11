/*
** Copyright (c) 2004, 2006 Ingres Corporation.
**
*/

/*
**
** Name: install.cpp - implementation file
**
** History:
**	04-aug-98 (mcgem01)
**	    Update release identifier for August release.
**	04-aug-98 (mcgem01)
**	    Components must be added in the same order that they appear
**	    in the isc file.
**	04-aug-98 (cucjo01)
**	    Changed SQL Gateway's selected=FALSE so components dialog
**	    works properly.
**	06-oct-98 (cucjo01)
**	    Added support for multiple log files
**	13-oct-98 (mcgem01)**	    On upgrade, migrate the log file variables from the symbol table to
**	    config.dat
**	16-oct-98 (mcgem01)
**	    Append the Installlation id to the II_CHARSET environment
**	    variable.
**	10-nov-1998 (mcgem01)
**	    Create the perfmon counters.
**	11-nov-1998 (cucjo01)
**	    Added /s for "silent" regedit command
**	    Added cazipxp unzipping of ICETutor after install completes
**	    Create ICE database
**	    Load ICE data into database
**	    Set rmcmd startup count to 0 since iidbdb doesnt exist
**	12-nov-1998 (cucjo01)
**	    Changed flag from CREATE_ALWAYS to OPEN_ALWAYS so config.dat is
**	    preserved on upgrade installs
**	20-nov-1998 (mcgem01)
**	    When ICE is installed, the 4K cache must be enabled as it uses
**	    row level locking to set up the tutorial.
**	23-nov-1998 (cucjo01)
**	    Add ICE configuration changes / updates
**	08-jan-1999 (cucjo01)
**	    Added Local_NMgtIngAt() which emulates NMgtIngAt() without
**	    requiring DLL's since they may not be available at installation
**	    time.
**	12-jan-1999 (cucjo01)
**	    Added unattended installation support using command line
**	    /R<response file> On upgrades, add .l01 extention to log file
**	    so it works with striped logging
**	20-jan-1999 (cucjo01)
**	    Moved upgrade code from finish.cpp to where other upgrade code
**	    resides
**	26-jan-1999 (mcgem01)
**	    Fix the syntax for initialization of the dual log file.
**	05-feb-1999 (somsa01)
**	    Added RetrieveHostname, which will issue a GetComputerName() and
**	    convert any odd characters to an underscore. Also, made appropriate
**	    changes for the installer to work with II_SYSTEM containing
**	    embedded spaces.
**	01-mar-1999 (somsa01)
**	    In SetConfigDat(), corrected usage of iisetres. Also, add the
**	    current user to config.dat properly through iisetres AFTER
**	    iigenres has been run. Also, in the case of TNG_EDITION, use
**	    the tng.rfm file.
**	15-mar-1999 (cucjo01)
**	    Updated upgrades to display error correctly when building
**	    transaction log. Changed message to "Updating system databases"
**	    on upgrades. Changed error message on wizard to display dialog
**	    box instead.
**	24-mar-99 (cucjo01)
**	    Removed INGRES4UNI precompile directives since response files
**	    now can replace it more gracefully.
**	    Removed function SetServiceStatus() for clarity
**	    Moved install log file to "files" directory with errlog.log
**	    Added response file option "serviceauto" for starting service
**	    automatically.
**	15-apr-99 (cucjo01)
**	    Added support for Service Start Password in response files
**	21-apr-99 (cucjo01)
**	    Added association of wordpad to log files if one didn't exist
**	    Unload performance counters before re-loading them (for
**	    re-installs)
**	23-apr-99 (cucjo01)
**	    Added function IsAcrobat301Installed() to check for previous 
**	    version of Acrobat before prompting user
**	30-apr-99 (cucjo01)
**	    Added better upgrade support and more diagnostics to the
**	    install.log.
**	    Show upgrade installation has been detected in log file.
**	    Added prompt to ask user if they want to automatically upgrade
**	    their DB's.
**	    Added UpgradeDatabases function which upgrades all user
**	    databases if desired.
**	    Transaction logs now are upgraded as follows:
**	    If an existing one is there, move it to file naming convention
**	    INGRES_LOG.L01.
**	    Added more messages on upgrades of the log file.
**	    Fixed bug where it displayed 'done.' without a task.
**	    Added function to remove start menu items (for Start Ingres
**	    and Stop Ingres,
**	    which have both been replaced by the Ingres Service Manager.)
**	    Moved upgrade of rmcmd catalogs to LoadIMA() for consolidation.
**	    Changed COMPLETE string to correct verision for config.dbms in
**	    config.dat.
**	13-may-99 (cucjo01)
**	    Added support to create a response file; if user passes /C, then
**	    the CreateResponseFile() function is called at the end of the
**	    wizard,
**	    which will prompt user for file name and create the response
**	    file based
**	    on the choices that were selected in the wizard.
**	14-may-99 (cucjo01)
**	    Display contents of response file inside the install.log file
**	    if it is invoked in response file mode.
**	19-may-99 (cucjo01)
**	    Added support for directories in the response file creation
**	    and usage.
**	18-aug-1999 (mcgem01)
**	    Update the Acrobat Reader to Version 4.0
**	18-aug-1999 (mcgem01)
**	    The Acrobat reader has been updated to 4.0, so we've renamed
**	    the check function to something more sensible.
**	15-nov-1999 (cucjo01)
**	    Moved silent.bat execution to function LaunchSilentBat() to ensure
**	    that it is only run once even if called multiple times.
**	    Added CheckValidLicense() to check license and log it to log file
**	    if the user installs the DBMS option.  
**	17-nov-1999 (mcgem01)
**	    Always install the Ingres service on NT.  If they've only installed
**	    the Ingres client piece, this will will only start the comms 
**	    server and net server anyway. 
**	24-jan-2000 (cucjo01)
**	    Set the password of the service named "Ingres_Database_XX",
**	    where XX is the installation identifier entered by the user.
**	    This is part of the new method of Ingres always starting as
**	    a service, and the service name changing to support multiple
**	    installations.
**	    The user name is now a member variable used once throughout
**	    the project.
**	    A function GetNTErrorText() has been added to retrieve the
**	    error text for a Windows error that may occur.
**	    The default for "Start Ingres service automatically" is now "TRUE".
**	26-jan-2000 (cucjo01)
**	    Set the file associations for ".vdbacfg" to execute VDBA.EXE
**	30-jan-2000 (cucjo01)
**	    Set up perfmon if Net is installed as well as if DBMS is installed.
**	30-jan-2000 (mcgem01)
**	    When Ingres DBMS is not installed, the Ingres service should
**	    be owned by the localsystem account and doesn't require a 
**	    password.   
**	30-jan-2000 (cucjo01)
**	    Changed 'COMPLETE' strings in config.dat file to reflect
**	    latest build.
**	31-jan-2000 (cucjo01)
**	    Once license is valid, don't check it again.
**	31-jan-2000 (cucjo01)
**	    Do not set up performance monitor if it is Windows 95/98.
**	31-jan-2000 (cucjo01)
**	    Set default m_bUserHasProperPrivileges to true, and add entry to 
**	    the install log if the user chose to continue with insufficient
**	    rights.
**	01-feb-2000 (cucjo01)
**	    Added error warning return code and text if user skipped
**	    password check.
**	02-feb-2000 (cucjo01)
**	    Added IVM to the startup group.
**	17-feb-2000 (cucjo01)
**	    Configure ColdFusion if user selected it on the ICE page.
**	    Add unattended installation support for the following rsp file
**	    items:
**	    - "installcoldfusionsupport" (YES | NO)
**	    - "iicoldfusiondir" (Directory Path Of ColdFusion)
**	23-feb-2000 (cucjo01)
**	    Added CreateSecurityDescriptor() to create and empty SD in order
**	    for Exec() to run properly since it was hanging when executing
**	    UpgradeDB. This Security Descriptor will allow output to be
**	    displayed. Before this, the output was prevented from being
**	    displayed and causing the application to hang after a certain
**	    buffer had been exceded. Display upgradedb output to install
**	    log file.
**	28-feb-2000 (cucjo01)
**	    Added three undocumented response file options:
**	    "LeaveIngresRunningInstallIncomplete", which will leave Ingres
**	    running at the end of the installation and leave the stopping
**	    Ingres as the responsibility of the caller. Also added
**	    "IngresPreStartupRunPrg" & "IngresPreStartupRunParams" which
**	    will execute this program and parameter as a command before
**	    starting Ingres for the first time. 
**	    Set the rmcmd startup count to 0 just prior to setting up
**	    iidbdb and back to 1 right after it has been created.
**	21-mar-2000 (cucjo01)
**	    Display the user environment inside the log file for
**	    diagnostic purposes.
**	22-mar-2000 (cucjo01)
**	    Removed redundant carriage returns in output to a generated
**	    response file which would show up as ^M in certain editors.
**	    When writing a string to a file, the newline character is
**	    written as a carriage return-linefeed pair.
**	10-may-2000 (cucjo01)
**	    Added ii.<machine name>.rcp.log.buffer_count = 20 on upgrades
**	    in order for the transaction log to perform properly on
**	    upgrades from OpenIngres 1.2 installations.
**	25-may-2000 (mcgem01)
**	    The parameter used to determine the size of a log file is
**	    now ii.$.rcp.file.kbytes.
**	02-aug-2000 (mcgem01)
**	    Rather than having two builds of the installer, the embedded 
**	    Ingres version will use a /embed flag at the command line to
**	    indicate that it's the embedded release.
**	03-aug-2000 (mcgem01)
**	    Added Visual DBA as a separately installable component.
**	09-aug-2000 (mcgem01)
**	    Add Embedded Ingres as a separately installable component.
**	04-aug-2000 (cucjo01)
**	    Moved icon creation from installer ".isc" file to the
**	    installation wizard post-installation. Also allowed for
**	    response file option "creaticons=YES|NO" to supress icon
**	    creation if the caller desires.
**	16-aug-2000 (cucjo01)
**	    Changed default value for VDBA in the response file; if there
**	    is no key for VDBA, the default is to leave VDBA on (unless
**	    embedded Ingres has been selected). This will allow users to
**	    specifically choose to install or not to install VDBA via the
**	    response file, while  installing it by default if they don't
**	    have an entry in the response file.
**	    A second parameter was also added to GetRSPValueBOOL() to
**	    allow for a default value in the case that the response file
**	    does not have a key for an install parameter. Previously, it
**	    always defaulted to FALSE.
**	17-aug-2000 (cucjo01)
**	    Changed license code check to 1H30 for Embedded Ingres
**	    installations.
**	17-aug-2000 (cucjo01)
**	    Crossed change #445389 (cucjo01) for bug #101018
**	    Added ODBC Registry and INI file setup for default and read
**	    only drivers based upon response file settings. Also added
**	    support to remove the driver if the response file option is
**	    chosen.  The four new response file values are:
**	    - setup_IngresODBC = TRUE | FALSE;             (Default: TRUE)
**	    - setup_IngresODBCReadOnly = TRUE | FALSE;     (Default: FALSE)
**	    - remove_IngresODBC = TRUE | FALSE;            (Default: FALSE)
**	    - remove_IngresODBCReadOnly = TRUE | FALSE;    (Default: FALSE)
**	    Changed SetRegistryEntry() to take REG_DWORD or REG_SZ as a
**	    parameter to make it more generic. Also write any registry
**	    updates to the install log.
**	21-aug-2000 (mcgem01)
**	    Set the installation ID for the Embedded Ingres release to WV
**	29-aug-00 (cucjo01)
**	    Add command line to the log file for better diagnostics.
**	30-aug-00 (cucjo01)
**	    Added response file option "hidewindows" to hide all windows
**	    during install. Default value is "FALSE". CA-Installer needs
**	    different flags based on this value.
**	29-sep-00 (noifr01)
**	    (bug 99242) cleanup for DBCS compliance
**	04-oct-2000 (mcgem01)
**	    When performing an upgrade, retain the old values for
**	    II_INSTALLATION, II_LANGUAGE, II_TIMEZONE_NAME, TERM_INGRES etc.
**	04-oct-2000 (mcgem01)
**	    Add a response file option which will allow the user to delete the 
**	    old transaction log file and create a new one at whatever size was
**	    specified.
**	04-oct-2000 (mcgem01)
**	    Add a response file option which will allow the user to
**	    automatically upgrade user databases during the installation
**	    process.
**	11-oct-2000 (mcgem01)
**	    On an upgrade, if II_LANGUAGE, II_TIMEZONE_NAME etc. etc.
**	    aren't found in the old symbol table use the default.
**	03-nov-2000 (mcgem01)
**	    Move the creation of the program group from after the creation and
**	    population of the system databases to immediately after the
**	    creation of the installation log file.
**	27-nov-2000 (mcgem01)
**	    Upgrade icedb if it exists, create it if it doesn't.
**	30-nov-2000 (mcgem01)
**	    Set the NT release identifier correctly.
**	01-dec-2000 (shust01)
**	    It is possible for a server to still be holding locks
**	    (for a short period of time) after the creation of iidbdb. If
**	    this happens, the ckpdb of iidbdb will fail.  That will cause
**	    an error code to be returned, and will skip the loading of the
**	    IMA tables. bug 103389, INGINS85.
**	01-dec-2000 (mcgem01)
**	    Need to set the working directory when setting up the program
**	    groups so that the help files can be located.
**	07-dec-2000 (mcgem01)
**	    Update the Adobe Acrobat Reader to 4.05
**	05-jan-2001 (mcgem01)
**	    Add JDBC Driver and JDBC Server as response file options.
**	16-feb-2001 (mcgem01)
**	    Set the correct 'Start in' directory so that the help files may be
**	    located.
**	30-mar-2001 (mcgem01)
**	    Remove extraneous status message from the log file.
**	31-mar-2001 (mcgem01)
**	    Set II_TEMPORARY in the symbol table rather than in the
**	    registry to keep things uncomplicated for multiple installation
**	    support. Remove old values from the system environment.
**	01-apr-2001 (mcgem01)
**	    Register the ActiveX controls required by the GUI tools.
**	    Also, set up the program menu items for IIA and IJA.
**	01-apr-2001 (mcgem01)
**	    Now that we've changed the Folder name to include the listen
**	    address we should remove the old folder in an upgrade scenario.
**	06-apr-2001 (rodjo04)
**	    Fixed retrieveing servicepassword and primarylogdir parameters.
**	10-apr-2001 (somsa01)
**	    Modified from Enterprise installer version to conform to what
**	    is only absolutely necessary for just post installation tasks.
**	11-apr-2001 (mcgem01)
**	    Add a file association with the Ingres Import Assistant for
**	    files of type *.iiimport.
**	    When creating the Ingres Import Assistant program menu,
**	    invoke it as "iia -loop"
**	18-apr-2001 (mcgem01)
**	    Set up an alternate ICE setup script for SQL 92 installations.
**	07-may-2001 (somsa01)
**	    When running "unlodctr", we do not care about the return code.
**	    Added setup for the ODBC driver.
**	21-may-2001 (somsa01)
**	    Re-implemented rodjo04's fix for bug 104417, which sets the
**	    appropriate 'complete' string for Double Byte.
**	23-may-2001 (somsa01)
**	    Added InstallAcrobat() and IsCurrentAcrobatAlreadyInstalled().
**	06-jun-2001 (somsa01)
**	    In LoadIMA(), run "rmcmdrmv -diidbdb" in the case of an
**	    upgrade.
**	06-jun-2001 (somsa01)
**	    In ServerPostInstallation(), removed the showing of
**	    IDS_UPGRADEMANUALLY.
**	06-jun-2001 (somsa01)
**	    Added Fortran.
**	07-jun-2001 (somsa01)
**	    In SetODBCEnvironment(), modified to use the new ODBC 3.5
**	    files.
**	12-jun-2001 (somsa01)
**	    The Release Notes file is now an HTML file.
**	13-jun-2001 (somsa01)
**	    Make sure we ALWAYS set the Ingres service password.
**	15-jun-2001 (somsa01)
**	    When bringing up the Release Notes, set up a wait cursor.
**	15-jun-2001 (penga03)
**	    Remove temporary MSI and Cabinet files.
**	18-jun-2001 (somsa01)
**	    Only run LoadPerfmon() if we're installing the DBMS. Also,
**	    repaired post installation of ICE if it is installed at a
**	    later date AFTER the DBMS.
**	23-July-2001 (penga03)
**	    When performing an upgrade, retain the old values for
**	    II_INSTALLATION, II_LANGUAGE, II_TIMEZONE_NAME, TERM_INGRES,
**	    II_ICE_WEB_HTML, II_ICE_COLDFUSION, II_DATABASE, II_CHECKPOINT,
**	    II_JOURNAL, II_WORK, II_DUMP and all the configuration values
**	    of transaction log files.
**	23-July-2001 (penga03)
**	    Delete function AlreadyThere.
**	23-July-2001 (penga03)
**	    Config transaction log file only if Ingres DBMS is installed.
**	25-jul-2001 (somsa01)
**	    In ThreadPostInstallation(), remove the old text release notes.
**	26-jul-2001 (somsa01)
**	    Moved the running of LoadPerfmon() to the end.
**	15-aug-2001 (somsa01)
**	    Added the removal of old-style shell links to IVM.
**	16-aug-2001 (somsa01)
**	    In RemoveOpenIngresFolders(), remove the old text release notes.
**	24-aug-2001 (somsa01)
**	    When retrieving registry values in a loop, make sure we
**	    properly set dwSize before using it in RegQueryValueEx().
**	24-aug-2001 (somsa01)
**	    More changes for the previous bug.
**	29-aug-2001 (penga03)
**	    Set association for .log with notepad and open install.log using 
**	    ShellExecute.
**	04-oct-2001 (penga03)
**	    Set the startup count to 1 for STAR if it was installed.
**	04-oct-2001 (somsa01)
**	    Slightly changed the name of the Cabinet file.
**	24-oct-2001 (penga03)
**	    Changed CreateProcessAsUser to CreateProcess.
**	26-oct-2001 (penga03)
**	    Set file association of .log to be txtfile. 
**	30-oct-2001 (penga03)
**	    Added DeleteObsoleteFileAssociations() to delete the obsolete file 
**	    associations.
**	    Modified SetFileAssociations() to embed installation identifier
**	    into file associations for IIA and VDBA.
**	05-oct-2001 (penga03)
**	    Remove the old Ingres program string from Add/Remove Prgrams panel.
**	10-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**	20-nov-2001 (somsa01)
**	    Redefined the way we figure out if the Online doc is installed.
**	    Also, we'll run the Acrobat install from II_TEMPORARY.
**	26-nov-2001 (somsa01)
**	    The name of the release notes is now "Readme_Ingres.htm".
**	04-dec-2001 (penga03)
**	    If embed install, or logon user is not "ingres", set the
**	    start name of Ingres service to be "LocalSystem".
**	05-dec-2001 (penga03)
**	    On Windows NT, start Ingres as a service.
**	27-dec-2001 (somsa01)
**	    Changed Ingres version to "ii260201".
**	28-jan-2002 (penga03)
**	    Moved the functionality to remove the temporary msi and cab
**	    files to 
**	    setup project.
**	30-jan-2002 (penga03)
**	    Changed Readme_Ingres.htm to readme.html.
**	30-jan-2002 (penga03)
**	    Changed the Ingres registry key from 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\IngresII\\" to 
**	    "HKEY_LOCAL_MACHINE\\Software\\ComputerAssociates\\Advantage
**	    Ingres\\".
**	06-mar-2002 (penga03)
**	    Added functions to build evaluation releases.
**	    Added Create_OtherDBUsers(), InstallPortal(), StartPortal(), 
**	    OpenPortal(), InstallForestAndTrees(), InstallOpenROAD(), and 
**	    UpdateFile(). Modified StartServer() and Execute().
**	21-mar-2002 (penga03)
**	    When Execute() creates a process with a new window, minimizes 
**	    the window.
**	26-mar-2002 (penga03)
**	    For evaluation releases, the name of the release note is 
**	    "readme_ingres_sdk.html".
**	27-mar-2002 (penga03)
**	    While opening the Portal Demo Page, bring the window to the 
**	    top of the Z order. Use a new Internet Explorer to open the 
**	    release note.
**	28-mar-2002 (somsa01)
**	    We now use Adobe Acrobat version 5.05.
**	02-apr-2002 (somsa01)
**	    We want to run SetServiceAuto() ONLY if it is not the
**	    Evaluation Release.
**	18-may-2002 (penga03)
**	    Added UpdateIngresBatchFiles().
**	06-jun-2002 (penga03)
**	    Utilize m_LeaveIngresRunningInstallIncomplete when just
**	    installing Ingres/Net as well.
**	25-jun-2002 (somsa01)
**	    Changed version to "260207".
**	16-jul-2002 (penga03)
**	    In IcePostInstallation(), for enterprise edition, if the 
**	    user chooses "Apache", we will first make sure the port 
**	    number used by ICE is available, then parse ice.conf 
**	    replacing [ICE_PORT], [II_SYSTEM] and [II_DOCUMENTROOT].
**	09-aug-2002 (penga03)
**	    Add a message at the end of the install.log when the post 
**	    installation process completes, unsuccessfully or 
**	    successfully.
**	22-aug-2002 (penga03)
**	    Wait until installing Adobe Acrobat completed, then delete 
**	    the executable from the Ingres temp directory.
**	09-sep-2002 (penga03)
**	    In SetFileAssociations(), added quotes around the command 
**	    parameters so that the OS passes to IIA and VDBA full file 
**	    name.
**	25-sep-2002 (penga03)
**	    Under some circumstances, install.exe hangs at DdeConnect 
**	    called by RemoveOpenIngresFolders(). This only happens on 
**	    Windows NT. Microsoft has confirmed that it is a bug, for 
**	    more details, please refer to Microsoft Knowledge Base 
**	    Article Q136218.
**	    To avoid this bug, instead of using DDE in 
**	    RemoveOpenIngresFolders(), we let IShellLink do the job.
**	02-dec-2002 (somsa01)
**	    Changed the Acrobat version and name to
**	    "AcroReader51_ENU_full.exe".
**	06-feb-2003 (penga03)
**	    Removed CheckValidLicense() and all its references.
**	20-may-2003 (penga03)
**	    Made changes in InstallPortal() so that Portal 4.51 can 
**	    be installed successfully. Also took away the comments on 
**	    OpenROAD.
**	30-may-2003 (penga03)
**	    Set II_LOG before installing OpenROAD. Added -f2 flag 
**	    in the call to OpenROAD’s installer.
**	02-jun-2003 (penga03)
**	    Made changes so that iipostinst will not wait until 
**	    installing acrobat is done and will not remove 
**	    "AcroReader51_ENU_full.exe".
**	06-aug-2003 (penga03)
**	    In RemoveOpenIngresFolders(), removed the code to remove 
**	    "Ingres II" and "Ingres II Online Documentation", which 
**	    are handled in the main setup. 
**	18-aug-2003 (penga03)
**	    In SetServiceAuto(), set the account name for Ingres 
**	    service to be "LocalSystem" if this is NOT an Enterprise 
**	    DBMS install.
**	    In InstallIngresService(), if ONLY install Ingres/Net, the 
**	    client flag should be used.
**	20-oct-2003 (penga03)
**	    Modified InstallIngresService() and SetServiceAuto().
**	    Now iipostinst is responsible for installing Ingres service. 
**	    And if Ingres service was already installed, do not install 
**	    it again. 
**	04-dec-2003 (somsa01)
**	    Added TCP_IP as a valid protocol.
**	26-Jan-2004 (penga03)
**	    Took out the creation of Administrator user.
**	28-Jan-2004 (penga03)
**	    Added the creation of Administrator user.
**	26-jan-2004 (penga03)
**	    Added code to set II_DATE_FORMAT and MONEY_FORMAT in 
**	    symbol.tbl.
**	28-jan-2004 (somsa01)
**	    In SetConfigDat(), take a stab at better handling of
**	    new/changed parameters. Avoid iigenres if it looks like some
**	    kind of valid DBMS configuration exists. Also remove
**	    old STAR config entries.
**	30-jan-2004 (penga03)
**	    Changed version to "300404".
**	30-jan-2004 (penga03)
**	    Removed the #ifdef DOUBLEBYTE around the CompleteString.
**	06-feb-2004 (penga03)
**	    We now use Adobe Acrobat version 6.0.
**	06-feb-2004 (penga03)
**	    Added ipm.ocx, sqlquery.ocx, vcda.ocx, vsda.ocx and 
**	    svrsqlas.dll in RegisterActiveXControls().
**	12-feb-2004 (penga03)
**	    Added installing Java runtime, and added II_JRE_HOME whose 
**	    value is %II_SYSTEM%\ingres\jre in symbol.tbl.
**	26-feb-2004 (penga03)
**	    Added SetODBCEnvironment() back in ThreadPostInstallation().
**	02-mar-2004 (penga03)
**	    Modified JDBCPostInstallation() to config Data Access 
**	    Server.
**	11-mar-2004 (somsa01)
**	    Make sure we use the "-l +w" options to get exclusive access
**	    to a database for checkpointing. Also, as on UNIX, checkpoint
**	    icesvr, icetutor, and icedb. Also, removed upgrade of imadb,
**	    icedb, icetutor, and icesvr, as they are now treated as system
**	    databases and will get upgraded automatically by an upgradedb on
**	    iidbdb. Also, if we are already upgrading all of the user
**	    databases, do not upgrade the "others". Also, in
**	    ServerPostInstallation(), do not turn on RMCMD, as it is turned
**	    on by LoadIMA() after INGSTART.
**	18-mar-2004 (somsa01)
**	    The RCP buffer_count now needs to be a minimum of 35.
**	18-mar-2004 (somsa01)
**	    Init value for dmf_tcb_limit if upgrading.
**	23-mar-2004 (penga03)
**	    Start Ingres at the end of post installation.
**	23-mar-2004 (penga03)
**	    Added a response file option: "startivm", which will 
**	    prevent the installer starting ivm if it is set to NO.
**	05-may-2004 (penga03)
**	    Removed the part that installs Java runtime and sets II_JRE_HOME.
**	05-may-2004 (penga03)
**	    In JDBCPostInstallation() made changes so that during upgrade, 
**	    if no jdbc is installed, das port will be %installation id%7 by 
**	    default, otherwise it will be the same as jdbc port.
**	18-may-2004 (penga03)
**	    Removed performance counters LoadPerfmon().
**	04-jun-2004 (gupsh04).
**	    Added LoadMDB().
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**  21-jun-2004 (gupsh01)
**      Added Grant_mdbimaadt().
**	23-jun-2004 (penga03)
**	    Added two response file parameters: installmdb, which allows user 
**	    to decide to install MDB or not;  mdbname, a configurable database 
**	    name for mdb. In addition, after the mdb creation has completed 
**	    the following config variable should be set using iisetres: 
**	    ii.<hostname>.mdb.mdb_dbname <mdbname>.
**	24-jun-2004 (penga03)
**	    In SetServiceAuto(), attached the domain name before the logon user 
**	    name.
**	09-jul-2004 (penga03)
**	    If no protocols is set to on, turn wintcp and tcp/ip on.
**	16-jul-2004 (penga03)
**	    Made some changes for mdb.
**	26-jul-2004 (penga03)
**	    Remove all references to "Advantage" and Ingres environment variables.
**	24-aug-2004 (penga03)
**	    Moved RegisterActiveXControls() after SetSymbolTbl() since 
**	    II_TIMEZONE_NAME is needed when registering ActiveX controls 
**	    required by the GUI tools.
**	25-aug-04 (penga03)
**	    Removed decnet protocol.
**	03-sep-2004 (somsa01)
**	    Updated for JAM build.
**	07-sep-2004 (penga03)
**	   Removed all references to service password since by default 
**	   Ingres service is installed using Local System account.
**	13-sep-2004 (penga03)
**	   Run rmcmd.exe from %ii_system%\ingres\utility.
**	14-sep-2004 (penga03)
**	   Replaced "Ingres Enterprise Edition" with "Ingres".
**	15-sep-2004 (penga03)
**	   Set up the system user user rights.
**	19-sep-2004 (penga03)
**	   Backed out the change made on 07-sep-2004 (471628) and the 
**	   change made on 15-sep-2004 (471900).
**	   Also start Ingres as service if the user wants to start Ingres
**	   automatically at startup time (that is m_serviceAuto is true);
**	   otherwise start Ingres without starting service.
**	20-sep-2004 (penga03)
**	   Removed creating ingres user.  
**	27-sep-2004 (penga03)
**	   Do not update the Ingres service if it is already installed.
**	14-oct-2004 (penga03)
**	   modified \ingres\vdba to \ingres\bin. 
**	20-oct-2004 (penga03)
**	   Fixed an allignment problem when creating ice databases.
**	21-oct-2002 (penga03)
**	   In ThreadPostInstallation() added RemoveObsoleteEnvironmentVariables()
**	   that will remove ingres\vdba from the PATH.
**	25-oct-2004 (penga03)
**	   The default size of log file is 32k.
**	25-oct-2004 (penga03)
**	   The environment variable for money format should be II_MONEY_FORMAT.
**	28-oct-2004 (somsa01)
**	    Removed DOUBLEBYTE define dependencies.
**	03-nov-2004 (penga03)
**	    Added InstallMDB().
**	03-nov-2004 (penga03)
**	    Modified SetServiceAuto() so that if the user wants to start 
**	    Ingres automatically at startup time but does not provide login 
**	    user id or password then Ingres service will use LocalSystem 
**	    account.
**	11-nov-2004 (penga03)
**	    Moved UpdateIngresBatchFiles() to ingres_installfinalize
**	    custom action in the main setup.
**	17-nov-2004 (penga03)
**	    Get the complete string from version.rel instead of 
**	    hard-coded.
**	18-nov-2004 (penga03)
**	    Set the startup counts to 0 for the connectivity servers (NET, JDBC,
**	    DAS) before configuring DBMS server, and restore the count when the 
**	    configuration completes.
**	07-dec-2004 (penga03)
**	    Changed the formulation to generate GUIDs since the old one does
**	    not work for id from A1 to A9.
**	08-dec-2004 (penga03)
**	    Shut down Ingres before restoring the startup counts for NET, JDBC
**	    and DAS.
**	15-dec-2004 (penga03)
**	    Backed out the change to generate ProductCode. If the installation
**	    id is between A0-A9, use the new formulation.
**	11-jan-2005 (penga03)
**	    Added a new symbol II_DECIMAL.
**	13-jan-2005 (penga03)
**	    Added passing two parameters, mdb size and debug, to setupmdb.bat.
**	14-jan-2005 (penga03)
**	    Use the localized administrator name.
**	18-jan-2005 (penga03)
**	    Added a new parameter II_MDB_PATH (%cdimage%\\mdb) for setupmdb.bat.
**	24-jan-2005 (penga03)
**	    Replaced strcmp() with _tcsicmp()in GetRegValue() and 
**	    GetRegValueBOOL().
**	28-jan-2005 (penga03)
**	    Removed the embedded part that affects the installation.
**	17-feb-2005 (penga03)
**	    Moved get the value of m_serviceAuto to Init().
**	03-march-2005 (penga03)
**	    Added Ingres cluster support.
**	07-march-2005 (penga03)
**	    Modified  RegisterClusterService() so that it accepts a list of shared 
**	    disks. In addition, corrected CreateDirectoryRecursively().
**	08-march-2005 (penga03)
**	    Get the directory containing the installation package from InstallSource
**	    instead of Cdimage.
**	08-march-2005 (penga03)
**	    Backed out the last change.
**	10-march-2005 (penga03)
**	    Fixed the iipostinst so that it handles the situation when II_DATABASE, 
**	    II_CHECKPOINT, etc. have already been there for a first time install.
**	11-march-2005 (penga03)
**	    Stop Ingres before registering cluster and do not run ingstart to start
**	    Ingres in a cluster situation. And corrected DefineDependencies().
**	14-march-2005 (penga03)
**	    For cluster install, set logon user for Ingres service.
**	15-march-2005 (penga03)
**	    Pass HAScluster to Ingres service to enable failover when one of the 
**	    Ingres services dies.
**	15-march-2005 (penga03)
**	    Added a check if the resource is already defined. 
**	18-march-2005 (penga03)
**	    The directory of upgrade.log should be 
**	    %ii_system%\ingres\files\upgradedb\%logon user's name%.
**	28-march-2005 (penga03)
**	    Open install.log, if it exists. Create a new install.log 
**	    if the file does not exist.
**	29-march-2005 (penga03)
**	    Corrected an error in RegisterClusterService(). 
**	29-march-2005 (penga03)
**	    The iipostinst will be failed if installing mdb fails.
**	31-march-2005 (penga03)
**	    If cluster, install Ingres service with LocalSystem account.
**	31-march-2005 (penga03)
**	    If cluster install, start Ingres as a service.
**	01-apr-2005 (penga03)
**	    For a cluster install, the startup type for the Ingres service
**	    should always be manual.
**	08-apr-2005 (penga03)
**	    In the context of the cluster, start Ingres by bringing the
**	    cluster resource online instead of running ingstart.
**	12-apr-2005 (penga03)
**	    Add II_CLUSTER_RESOURCE to symbol.tbl.
**	28-apr-2005 (drivi01) on behalf of penga03
**	    Added check to make sure that if user launches the isntall
**	    in a non-cluster context, we don't set the Ingres service to
**	    use the virtual node name.
**	15-may-2005 (penga03)
**	    Added iigcc to the Windows Firewall exceptions list.
**	15-may-2005 (penga03)
**	    Corrected the return code of InstallMDB().
**	20-may-2005 (penga03)
**	    Set the m_serviceAuto to its old value if service already installed.
**	23-may-2005 (penga03)
**	    Set II_HOSTNAME in the symbol table for cluster install and replaced
**	    GetComputerName with GetComputerNameEx so that m_computerName always 
**	    gets the physical node name of the local computer.
**	27-may-2005 (penga03)
**	    Set the default character set is WIN1252.
**	23-jun-2005 (penga03)
**	    Made changes to get the exit code from setupmdb.bat.
**	24-jun-2005 (penga03)
**	    Don't launch setupmdb.bat.
**	01-jul-2005 (penga03)
**	    Removed the misleading mdb message in the log file.
**	18-jul-2005 (penga03)
**	    Don't pop up the message to ask the user if he/she wants to upgrade
**	    user database if silent install.
**	28-jul-2005 (penga03)
**	    Add return code to the log file.
**	12-Aug-2005 (penga03)
**	    Corrected ThreadPostInstallation to set the right return code in the 
**	    install.log.
**	17-Aug-2005 (penga03)
**	    Corrected the error while calling Exec.
**	18-aug-2005 (penga03)
**	    Moved writting the return code to the install.log from 
**	    ThreadPostInstallation() to ExitInstance().
**	24-aug-2005 (penga03)
**	    Set dual_log_name even the user doesn't choose to install 
**	    dual log.
**	26-aug-2005 (penga03)
**	    Don't ask the user if he/she wants to upgrade the user databases
**	    if this is the HAO install on the second node.
**	30-aug-2005 (penga03)
**	    Keep the NET configuration during upgrade or modify.
**	19-Oct-2005 (gupsh01)
**	    Added new charactersets ISO 88597, Win1253 and PC737 in charsets
**	    table.
**	24-Oct-2005 (penga03)
**	    Corrected the minor error introduced by last change.
**	27-Oct-2005 (penga03)
**	    In SetSymbolTbl(), create symbol.tbl if it is not there.
**	27-Oct-2005 (penga03)
**	    Set II_HOSTNAME for a non-cluster install to fix the problem 
**	    happened in a client install in a cluster environment.
**	16-Nov-2005 (drivi01)
**	    Added routine for creating service owned by system on
**	    embeded install when serviceauto="NO".
**	5-Jan-2006 (drivi01)
**	    SIR 115615
**	    Modified registry Keys to be SOFTWARE\IngresCorporation\Ingres
**	    instead of SOFTWARE\ComputerAssociates\Ingres.
**	    Updated shortcuts to point to Start->Programs->Ingres->
**	    Ingres [II_INSTALLATION] and updated location to
**	    C:\Program Files\Ingres\Ingres [II_INSTALLATION].
**	23-Jan-2006 (drivi01)
**	    Modified routines that set II_LANGUAGE variable in symbol table
**	    to only set it to a language if corresponding directory with i18n
**	    files exists.
**	13-Feb-2006 (drivi01)
**	    Updating Adobe 6.0 to Adobe Acrobat 7.0.
**	15-Feb-2006 (drivi01)
**	    Updated binary that installs Acrobat Reader 7.0.
**	28-Jun-2006 (drivi01)
**	    SIR 116381
**	    iipostinst.exe is renamed to ingconfig.exe. Fix all calls to
**	    iipostinst.exe to call ingconfig.exe instead.
**	06-Sep-2006 (drivi01)
**          SIR 116599
**          As part of the initial changes for this SIR.  Modify Ingres
**          name to "Ingres II" instead of "Ingres [ II ]", Ingres location
**          to "C:\Program Files\Ingres\IngresII" instead of
**          C:\Program Files\Ingres\Ingres [ II ].
**  15-Nov-2006 (drivi01)
**          SIR 116599
**          Enhanced post-installer in effort to improve installer usability.
**	    Add new variables representing new features with functionality
**	    performed during configuration phase s.a. installdemodb. 
**	    expressinstall represents the mode in which install was executed
**	    to display consistent summary information at the end of 
**	    configuration.
**	    JDBC server is no longer configured due to the fact that
**	    support for it is being dropped.
**	    if installdemodb is set, then demodb is created and
**	    loaded with data.
**	    Added routines for openning adboe download page if
**	    adobe reader isn't detected.
**  04-Dec-2006 (drivi01)
**	    Added routines for retrieving new property value from registry and
**          using it to configure ingres.  New registry entry is ansidate.
** 	    For new installs and modified installs we allow user to set the
**	    date_type_alias in config.dat, for upgrades we set it to INGRES.
**  15-Jan-2007 (drivi01)
**	    For upgrade installations, remove all tables in imadb owned by user
**	    ingres, the tables should now be owned by $ingres.
**  14-Mar-2007 (drivi01)
**	    rmcmd count isn't restored to "1" after modify is performed.
**  12-Apr-2007 (drivi01)
**	    Modify routines for resetting rmcmd to 0 and then 1 to retrieve
**	    the real value of rmcmd first and then setting it to that value.
**	    This ensures that for Net only installations we don't set rmcmd
**	    to 1.
**  03-May-2007 (drivi01)
**	    Added m_attemptedUDBupgrade, this variable will be always set to
**	    FALSE unless iipostinst step into or near UpgradeDatabase code, in
**	    which case the variable will be set to TRUE to notify that this
**	    was a full iipostinst following valid upgrade.
**  25-May-2007 (drivi01)
**          Added new property to installer INGRES_WAIT4GUI which will force
**          setup.exe to wait for post installer to finish if this property
**          is set to 1. INGRES_WAIT4GUI which is referred to as 
**	    m_bWait4PostInstall within ingconfig.exe is set to TRUE if "/w" 
**	    flag is passed to setup.exe. If m_bbWait4PostInstall is set to 
**	    TRUE, ingconfig.exe will post messages to other windows instead 
**	    of sending them b/c setup.exe window will be frozen waiting for
**	    ingconfig.exe to return and this will result in a hang since
**	    setup.exe will be unable to process the message.
**  1-Jun-2007 (horda03) Bug 118464
**	    Create the directories for the Primary and Dual log files.
**  10-Jul-2007 (kibro01) b118702
**	    Move date_type_alias to config section
**  25-Jul-2007 (drivi01)
**	    Updated CInstallation::SetTemp() function to construct a 
**	    temporary directory path equal to the machine %TEMP%
**	    variable, instead of the II_SYSTEM\ingres\temp location.
**	    This is done for the port on Vista, b/c Vista can not write
**	    to Program Files location.
**  24-Aug-2007 (drivi01)
**	    Create configuration directory for visual tools configuration
**	    files on Vista at ALLUSERSPROFILE location.
**  20-Dec-2007 (drivi01)
**	    Added iijdbcprop executable to post installation for JDBC/DAS
**	    feature.  iijdbcprop should create iijdbc.properties file in
**	    II_SYSTEM/ingres/files upon installation.
**  04-Apr-2008 (drivi01)
**	    Adding functionality for DBA Tools installer to the Ingres code.
**	    DBA Tools installer is a simplified package of Ingres install
**	    containing only NET and Visual tools component.  
**	    Retrieves dbatools registry value if it is set and updates
**	    the dialog wording to reflect DBA Tools install.
**	    Adjusts service installation to let opingsvc know that we're
**	    installing DBA Tools.  Account for the fact that service
**	    was renamed for DBA Tools installation.
**  30-Apr-2008 (drivi01)
**	    Remove CreateConfigDir function.
**	    Update SetTemp function to leave temp directory in 
**	    II_SYSTEM\ingres\temp for non-Vista Windows, and 
**	    on Vista temp directory will be moved to %ALLUSERSPROFILE%.
**  10-Jun-2008 (drivi01)
**	    Add a few registry entries for the ODBC driver identifying
**	    the company and the read-only driver.  Update Ingres 3.0
**	    entry to create Ingres 2006 instead.
**  18-Sep-2008 (drivi01)
**	    Added routines for adding virtual nodes using gcnapi
**	    to the client installation. Post installer retrieves
**	    vnode_count stored by installer in the registry
**	    and then uses it to determine the amount of virtual
**	    nodes to create and retrieve the information about
**	    virtual nodes from the registry.
**  03-Nov-2008 (drivi01)
**	    Add routines to automate product version information
**	    polling.
**  17-Nov-2008 (drivi01)
**	    Replace SendMessage Environment system broadcasts 
**	    with PostMessage.  SendMessage is causing occasional
**	    hangs possibly due to tighter security and inability
**	    to send messages to higher integirty processes. 
**	    PostMessage will infiltrate environment changes to
**	    applications of lower or equal integirty level, will
**	    not cause hangs as our prpogram doesn't depend on 
**	    reply to the broadcast.
**  11-Feb-2009 (drivi01)
**	    Update temporary directory for DBA Tools package.
**  14-Jul-2009 (drivi01)
**		SIR 122309
**		Added new dialog to specify Configuration System for ingres instance.
**		The dialog contains 4 options which will be represented as
**		0 for Transactional System
**		1 for Traditional Ingres System
**		2 for Business Intelligence System
**		3 for Content Management System
**		Post-installer will read "ingconfigtype" registry entry
**		and depending on the configuration chosen set a custom
**		set of parameters for ingres instance.
**		The parameters and configuration type will be reflected in install.log.
**  06-Aug-2009 (drivi01)
**	    Update the cursor_limit in SetupBIConfig() to be 32 instead of 256.  
**	    256 is just way too high.
**  08-Aug-2009 (drivi01)
**	    Add pragma to remove deprecated POSIX functions warning 4996
**	    b/c it is a bug. Cleaned up warnings.
**  23-Sep-2009 (drivi01)
**	    Force Transactional, Business and Content Management configurations
**	    to set current directory before attempting to set any of the
**	    configurations to avoid failures during CreateProcess.
**	    This will improve the search pattern for DLLs that need
**	    to be loaded.
**  28-Sep-2009 (drivi01)
**	    Fix parameters for cursor_limit to match the rest in BI system.
**  29-Sep-2009 (drivi01)
**	    Set ii.<machine_name>.dbms.*.connect_limit for BI to 64.
**  23-Oct-2009 (drivi01)
**	    Update ii.%s.rcp.lock.lock_limit to CM to 325000.
**  04-Jun-2010 (drivi01)
**      Remove II_GCNapi_ModifyNode API for creating virtual nodes
**      for DBA package b/c II_GCNapi_ModifyNode is no longer supported
**      and stopped working due to long ids change.
**      This change replaces the obsolete API call with a range of
**      supported open API calls.
**      
**	    
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning (disable: 4996)

#include "stdafx.h"
#include <ddeml.h>
#include "shlobj.h"
#include "iipostinst.h"
#include <lm.h>
#include <clusapi.h> 
extern "C" {
#include <compat.h>
#include <gv.h>
}

void stop_logwatch_service();
void start_logwatch_service();

typedef BOOL (WINAPI *PSQLREMOVEDRIVERPROC)(LPCSTR, BOOL, LPDWORD);

BOOL CreateSecurityDescriptor(SECURITY_ATTRIBUTES *sa);
GLOBALREF ING_VERSION ii_ver;

typedef struct tagENTRY
{
	char key[128];
	char value[128];	
}ENTRY;

static ENTRY charsets[]=
{
    {"THAI", "874"},
    {"WTHAI", "874"},
    {"SHIFTJIS", "932"},
    {"KANJIEUC", "932"},
    {"CHINESES", "936"},
    {"CSGBK", "936"},
    {"CSGB2312", "936"},
    {"KOREAN", "949"},
    {"CHTBIG5", "950"},
    {"CHTEUC", "950"},
    {"CHTHP", "950"},
    {"UCS2", "1200"},
    {"UCS4", "1200"},
    {"UTF7", "1200"},
    {"UTF8", "1200"},
    {"IBMPC850", "1250"},
    {"WIN1250", "1250"},
    {"IBMPC866", "1251"},
    {"ALT", "1251"},
    {"CW", "1251"},
    {"WIN1252", "1252"},
    {"GREEK", "1253"},
    {"ELOT437", "1253"},
    {"PC857", "1254"},
    {"HEBREW", "1255"},
    {"WHEBREW", "1255"},
    {"PCHEBREW", "1255"},
    {"ARABIC", "1256"},
    {"WARABIC", "1256"},
    {"DOSASMO", "1256"},
    {"WIN1252", "1257"},
    {"WIN1253", "1253"},
    {"ISO88597", "28597"},
    {"PC737", "737"},
    {0,0}
};

#define SEPARATOR ";"

typedef struct tagING_ENV_VALUES
{
	char *key;
	char *value;
}ING_ENV_VALUES;

static ING_ENV_VALUES env_strings[]=
{
	{"PATH","%s\\ingres\\vdba"},
	{0,0}
};

static char *szEnvRegKey="SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";

CComponent *CInstallation::GetDBMS()
{
    CComponent	*c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELDBMSSERVER);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }

    return 0;
}

CComponent *CInstallation::GetNet()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELINGRESNET);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetStar()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELINGRESSTAR);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *) m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetESQLC()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELESQLC);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetESQLCOB()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELESQLCOBOL);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetTools()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELTOOLS);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetSQLServerGateway()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELSQLSERVERGATEWAY);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetOracleGateway()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELORACLEGATEWAY);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetSybaseGateway()
{
    CComponent *c;
    CString	s;
    int		i;

    s.LoadString(IDS_LABELSYBASEGATEWAY);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetInformixGateway()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELINFORMIXGATEWAY);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetVision()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELVISION);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c=(CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetReplicat()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELREPLICATOR);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetICE()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELICE);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetOpenROADDev()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELOPENROADDEV);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetOpenROADRun()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELOPENROADRUN);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetOnLineDoc()
{
    CComponent	*c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELDOC);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *) m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetVDBA()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELVDBA);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetEmbeddedComponents()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELEMBEDDEDCOMPONENTS);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

CComponent *CInstallation::GetJDBC()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELJDBC);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

static BOOL
SetServiceAuto(LPCSTR lpServiceName, BOOL AutoStart)
{
    BOOL	bRet = FALSE;
    SC_HANDLE	schSCManager;
    BOOL bLocalSystem = FALSE;
    CString cmd;	

    if (theInstall.m_bClusterInstall 
	|| (AutoStart && (theInstall.m_serviceloginid.IsEmpty() || theInstall.m_servicepassword.IsEmpty())))
	bLocalSystem = TRUE;

	if (!bLocalSystem && !AutoStart && theInstall.m_serviceloginid.Compare("LocalSystem") == 0)
		bLocalSystem = TRUE;

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schSCManager)
    {
	SC_HANDLE schService =
		OpenService(schSCManager, lpServiceName, SERVICE_CHANGE_CONFIG);

	if (schService)
	{
		if (bLocalSystem )
		{
			bRet = ChangeServiceConfig(schService, SERVICE_NO_CHANGE,
				AutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
				SERVICE_NO_CHANGE, 0, 0, 0, 0,
				"LocalSystem", "", 0);
			
			cmd.Format( "ii.%s.privileges.user.system SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED", 
				(LPCSTR)theInstall.m_computerName);
			theInstall.Exec(theInstall.m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
		}
		else
		{
			HANDLE hProcess, hAccessToken;
			char InfoBuffer[1000], szDomainName[1024], szAccountName[1024];
			PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
			DWORD dwInfoBufferSize, dwDomainSize=1024, dwAccountSize=1024;
			SID_NAME_USE snu;
			
			/*
			** Get the domain name that this user is logged on to. This could
			** also be the local machine name.
			*/
			hProcess = GetCurrentProcess();
			OpenProcessToken(hProcess, TOKEN_READ, &hAccessToken);
			GetTokenInformation(hAccessToken, TokenUser, InfoBuffer, 1000, &dwInfoBufferSize);
			LookupAccountSid(NULL, pTokenUser->User.Sid, szAccountName, &dwAccountSize, szDomainName, &dwDomainSize, &snu);
			theInstall.m_serviceloginid.Format("%s\\%s", szDomainName, szAccountName);

			bRet = ChangeServiceConfig(schService, SERVICE_NO_CHANGE,
				AutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
				SERVICE_NO_CHANGE, 0, 0, 0, 0,
				theInstall.m_serviceloginid,
				theInstall.m_servicepassword, 0);
		}
	    CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
    }
    return bRet;
}

BOOL
CInstallation::CopyOneFile(CString &src, CString &dst)
{
    BOOL bret = CopyFile(src, dst, FALSE);

    if (bret)
    {
	DWORD	att = GetFileAttributes(dst);
	if (att != 0xFFFFFFFF)
	    SetFileAttributes(dst, att & (~FILE_ATTRIBUTE_READONLY));
	else
	    bret = FALSE;
    }
    return bret;
}

CComponent::CComponent(LPCSTR name,double size, BOOL selected,BOOL visible,LPCSTR path) :
m_name(name), m_size(size), m_selected(selected), m_visible(visible) 
{ 
    if (path && *path) 
	m_installPath = path;
}

void
CInstallation::Halted()
{
    m_reboot = FALSE;
}

BOOL
CInstallation::SetRegistryEntry(HKEY hRootKey, LPCSTR lpKey, LPCSTR lpValue,
				DWORD dwType, void *lpData)
{		
    DWORD   dwDisposition = 0;
    BOOL    ret = FALSE;
    HKEY    Key = 0;	
    CString s;

    if ((dwType != REG_SZ) && (dwType != REG_DWORD))
        return FALSE;

    if ( RegCreateKeyEx(hRootKey, lpKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &Key, &dwDisposition) == ERROR_SUCCESS )
    { 
        if ((dwType == REG_SZ) && lpData)
        {
	    s.Format(IDS_WRITINGREGISTRYENTRY_SZ, lpKey, lpValue, lpData);
	    AppendToLog(s);
	    ret = (RegSetValueEx(Key, lpValue, 0, dwType, (CONST BYTE *)lpData,
		   (DWORD)strlen((char *)lpData)+1) == ERROR_SUCCESS);
        }
        else if (dwType == REG_DWORD)
        {
	    s.Format(IDS_WRITINGREGISTRYENTRY_DW, lpKey, lpValue, lpData);
	    AppendToLog(s);
	    ret = (RegSetValueEx(Key, lpValue, 0, dwType, (CONST BYTE *)&lpData,
		   sizeof(lpData)) == ERROR_SUCCESS);
        }

        if (!ret)
        {
	    s.Format(IDS_CANNOTWRITEREGISTRY, lpKey, lpValue, ret);
	    AppendToLog(s);
        }

        RegCloseKey(Key);
    }

    return ret;
}

BOOL
CInstallation::RemoveFile(LPCSTR lpFileName)
{
    return (FileExists(lpFileName) ?  DeleteFile(lpFileName) : TRUE);
}

void
CInstallation::ReadRNS()
{
    CWaitCursor wait;
    BOOL bRet=TRUE;
    char szBuf[MAX_PATH];
    HKEY hRegKey=0;
    CString sIE, sURL;

#ifdef EVALUATION_RELEASE
    sURL = m_installPath + "\\Readme_Ingres_SDK.html";
#else
    sURL = m_installPath + "\\readme.html";
#endif 

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
       "SOFTWARE\\Microsoft\\IE Setup\\Setup", 
       0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "Path", NULL, NULL, (BYTE *)szData, 
	                   &dwSize)==ERROR_SUCCESS)
	{
	    if (szData[strlen(szData)-1]=='\\') 
		szData[strlen(szData)-1]='\0';

	    sIE.Format("%s", szData);
	    if(sIE.Find("%programfiles%") != -1)
	    {
		GetEnvironmentVariable("programfiles", szBuf, sizeof(szBuf));
		sIE.Replace("%programfiles%", szBuf);
	    }
	}
	RegCloseKey(hRegKey);
    }

	sprintf(szBuf, "\"%s\\IEXPLORE.EXE\" \"%s\"", sIE, sURL);
	Execute(szBuf, FALSE);
}

void
CInstallation::ReadInstallLog()
{
	CWaitCursor	wait;
	CString	s = m_installPath + "\\ingres\\files\\install.log";
	
	ShellExecute(NULL, "open", s, 0, 0, SW_NORMAL);
}

void
CInstallation::SetDate()
{
    CString s;

    s = m_startTime.Format(IDS_DATEFORMAT);
    AppendToLog(s);

    s.Format(IDS_USERFORMAT, m_userName);
    AppendToLog(s);

    if (!theInstall.m_commandline.IsEmpty())
    {
	s.Format(IDS_COMMANDLINE, theInstall.m_commandline);
	AppendToLog(s);
    }
}

/*
**	History:
**	23-July-2001 (penga03)
**	    If this is an upgrade, get the language, timezone, terminal 
**	    and charset from symbol.tbl.
**	07-Sep-2001 (penga03)
**	    In Windows ME, the path returned by GetModuleFileName is uppercase.
**	14-Sep-2001 (penga03)
**	    After upgrade, delete careglog.log and inguninst.exe.
**	21-mar-2002 (penga03)
**	    Added m_InstallSource whose value is the InstallSource of 
**	    Advantage Ingres SDK.
**	21-jan-2004 (penga03)
**	    Remove inguninst.exe only if upgrade.
*/
void
CInstallation::Init()
{
    AddComponent(IDS_LABELDBMSSERVER, SIZE_DBMS, TRUE, FALSE);
    AddComponent(IDS_LABELINGRESNET, SIZE_NET, TRUE, TRUE);
    AddComponent(IDS_LABELODBC, SIZE_ODBC, TRUE, TRUE);
    AddComponent(IDS_LABELINGRESSTAR, SIZE_STAR, TRUE, TRUE);
    AddComponent(IDS_LABELESQLC,SIZE_ESQLC, TRUE, TRUE);
    AddComponent(IDS_LABELESQLCOBOL, SIZE_ESQLCOB, TRUE, TRUE);
    AddComponent(IDS_LABELESQLFORTRAN, SIZE_ESQLFORTRAN, TRUE, TRUE);
    AddComponent(IDS_LABELTOOLS, SIZE_TOOLS, TRUE, TRUE);
    AddComponent(IDS_LABELVISION, SIZE_VISION, TRUE, TRUE);
    AddComponent(IDS_LABELREPLICATOR,SIZE_REPLICAT, TRUE, TRUE);
    AddComponent(IDS_LABELICE, SIZE_ICE, TRUE, TRUE);
    AddComponent(IDS_LABELOPENROADDEV, SIZE_OPENROAD_DEV, TRUE, TRUE);
    AddComponent(IDS_LABELDOC, SIZE_DOC, TRUE, TRUE);
    AddComponent(IDS_LABELOPENROADRUN, SIZE_OPENROAD_RUN, FALSE, TRUE);
    AddComponent(IDS_LABELSQLSERVERGATEWAY, SIZE_SQLSERVER_GATEWAY, FALSE, TRUE);
    AddComponent(IDS_LABELORACLEGATEWAY, SIZE_ORACLE_GATEWAY, FALSE, TRUE);
    AddComponent(IDS_LABELINFORMIXGATEWAY, SIZE_INFORMIX_GATEWAY, FALSE, TRUE);
    AddComponent(IDS_LABELSYBASEGATEWAY, SIZE_SYBASE_GATEWAY, FALSE, TRUE);
    AddComponent(IDS_LABELVDBA, SIZE_VDBA, TRUE, TRUE);
    AddComponent(IDS_LABELEMBEDDEDCOMPONENTS, SIZE_EMBEDDED_COMPONENTS, TRUE, TRUE);
    AddComponent(IDS_LABELJDBC, SIZE_JDBC, TRUE, TRUE);

    char	ach[4096], *p;
    CString	szBuf;
    CString	s, strValue;
    CComponent	*dbms = GetDBMS();
    CComponent	*net = GetNet();
    CComponent	*odbc = GetODBC();
    CComponent	*star = GetStar();
    CComponent	*esqlc = GetESQLC();
    CComponent	*esqlcob = GetESQLCOB();
    CComponent	*esqlfortran = GetESQLFORTRAN();
    CComponent	*tools = GetTools();
    CComponent	*vision = GetVision();
    CComponent	*replicat = GetReplicat();
    CComponent	*ice = GetICE();
    CComponent	*openroaddev = GetOpenROADDev();
    CComponent	*openroadrun = GetOpenROADRun();
    CComponent	*onlinedoc = GetOnLineDoc();
    CComponent	*sqlserver = GetSQLServerGateway();
    CComponent	*oracle = GetOracleGateway();
    CComponent	*informix = GetInformixGateway();
    CComponent	*sybase = GetSybaseGateway();
    CComponent	*vdba = GetVDBA();
    CComponent	*embeddedcomponents = GetEmbeddedComponents();
    CComponent	*jdbc = GetJDBC();

    GetModuleFileName(AfxGetInstanceHandle(), ach, sizeof(ach));

    p = _tcsstr(ach, "\\ingres\\bin\\ingconfig.exe");
	if(p == NULL)
		p = _tcsstr(ach, "\\INGRES\\BIN\\INGCONFIG.EXE");  

    if (p!=NULL && *p)
	*p = 0;
    else
	*ach = 0;   /* '\\' may be a DBCS trailing byte: requires DBCS compliant function */

    m_installPath = ach;

    SetEnvironmentVariable("II_SYSTEM", m_installPath);

	char file2delete[MAX_PATH+1];
	BOOL ret;

	sprintf(file2delete, "%s\\CAREGLOG.LOG", m_installPath);
	ret = DeleteFile(file2delete);
	if (ret)
	{
		GetSystemDirectory(file2delete, sizeof(file2delete));
		strcat(file2delete, "\\inguninst.exe");
		DeleteFile(file2delete);
	}

    /*
    ** Let's get the essential values that we need from the registry.
    */
    m_installationcode = GetRegValue("installationcode");

	/* If this is a upgrade, grab those values from symbol.tbl first. */
	if(!Local_NMgtIngAt("II_LANGUAGE", m_language))
		m_language = GetRegValue("language");
	if(m_language.IsEmpty())
		m_language = "ENGLISH";
	
	if(!Local_NMgtIngAt("II_TIMEZONE_NAME", m_timezone))
		m_timezone = GetRegValue("timezone");
	if(m_timezone.IsEmpty())
		m_timezone = "NA-EASTERN";
	
	if(!Local_NMgtIngAt("TERM_INGRES", m_terminal))
		m_terminal = GetRegValue("terminal");
	if(m_terminal.IsEmpty())
		m_terminal = "IBMPC";
	
	szBuf.Format ("II_CHARSET%s", m_installationcode);
	if(!Local_NMgtIngAt("II_CHARSET", m_charset) && !Local_NMgtIngAt(szBuf, m_charset))
		m_charset = GetRegValue("charset");
	if(m_charset.IsEmpty())
		m_charset = "WIN1252";

	if(!Local_NMgtIngAt("II_DATE_FORMAT", m_dateformat))
		m_dateformat = GetRegValue("dateformat");
	if(m_dateformat.IsEmpty())
		m_dateformat = "US";

	if(!Local_NMgtIngAt("II_MONEY_FORMAT", m_moneyformat))
		m_moneyformat = GetRegValue("moneyformat");
	if(m_moneyformat.IsEmpty())
		m_moneyformat = "L:$";

	if(!Local_NMgtIngAt("II_DECIMAL", m_decimal))
		m_decimal = GetRegValue("decimal");
	if(m_decimal.IsEmpty())
		m_decimal = ".";

    m_serviceloginid = GetRegValue("serviceloginid");
    m_servicepassword = GetRegValue("servicepassword");
    m_serviceAuto = GetRegValueBOOL("serviceauto", FALSE);
	m_express = GetRegValueBOOL("expressinstall", TRUE);
	m_includeinpath = GetRegValueBOOL("includeinpath", TRUE);
	m_installdemodb = GetRegValueBOOL("installdemodb", TRUE);
	m_ansidate = GetRegValueBOOL("ansidate", FALSE);
	m_bWait4PostInstall = GetRegValueBOOL("waitforpostinstall", FALSE);
	m_DBATools = GetRegValueBOOL("dbatools", FALSE);
	m_vnodeCount=0; //initialize to 0 to avoid garbage values
	if (m_DBATools)
		m_vnodeCount = atoi(GetRegValue("vnode_count"));
	m_configType = atoi(GetRegValue("ingconfigtype"));

    char szServiceName[512];
    DWORD dwBytesNeeded;
    SC_HANDLE schSCManager, schService;
    LPQUERY_SERVICE_CONFIG pqsc;

    schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager)
    {
	sprintf(szServiceName, "Ingres_Database_%s", m_installationcode);
	schService=OpenService(schSCManager, szServiceName, SERVICE_QUERY_CONFIG);
	if (schService)
	{
	    QueryServiceConfig(schService, NULL, 0, &dwBytesNeeded);
	    pqsc=(LPQUERY_SERVICE_CONFIG)malloc(dwBytesNeeded);
	    QueryServiceConfig(schService, pqsc, dwBytesNeeded, &dwBytesNeeded);
	    if (pqsc->dwStartType == SERVICE_AUTO_START)
		m_serviceAuto = TRUE;
	    free(pqsc);
	    CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
    }

    m_bClusterInstall = GetRegValueBOOL("clusterinstall", FALSE);
    m_ClusterResource = GetRegValue("clusterresource");
    if (m_bClusterInstall) 
	m_serviceAuto = FALSE;
    m_bClusterInstallEx = 0;

    m_hidewindows = GetRegValueBOOL("hidewindows", FALSE);
    m_LeaveIngresRunningInstallIncomplete =
		GetRegValueBOOL("leaveingresrunninginstallincomplete", FALSE);
    m_StartIVM = GetRegValueBOOL("startivm", TRUE);
    m_IngresPreStartupRunPrg = GetRegValue("ingresprestartuprunprg");
    m_IngresPreStartupRunParams = GetRegValue("ingresprestartuprunparams");
//    m_checkpointdatabases = GetRegValueBOOL("checkpointdatabases", FALSE);
    m_ResponseFile = GetRegValue("ResponseFileLocation");
    m_UseResponseFile = !m_ResponseFile.IsEmpty();

    /*
    ** Install the Embedded Components if the installer is invoked with
    ** the /embed option.
    */
    m_bEmbeddedRelease = GetRegValueBOOL("EmbeddedRelease", FALSE);

    /*
    ** Find out which components are being installed.
    */
    if (dbms)
    {
	dbms->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELDBMSSERVER) ? strValue : "", FALSE);
    }
    if (net)
    {
	net->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELINGRESNET) ? strValue : "", FALSE);
    }
    if (odbc)
    {
	odbc->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELODBC) ? strValue : "", FALSE);
    }
    if (star)
    {
	star->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELINGRESSTAR) ? strValue : "", FALSE);
    }
    if (esqlc)
    {
	esqlc->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELESQLC) ? strValue : "", FALSE);
    }
    if (esqlcob)
    {
	esqlcob->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELESQLCOBOL) ? strValue : "", FALSE);
    }
    if (esqlfortran)
    {
	esqlfortran->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELESQLFORTRAN) ? strValue : "", FALSE);
    }
    if (tools)
    {
	tools->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELTOOLS) ? strValue : "", FALSE);
    }
    if (vision)
    {
	vision->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELVISION) ? strValue : "", FALSE);
    }
    if (replicat)
    {
	replicat->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELREPLICATOR) ? strValue : "", FALSE);
    }
    if (ice)
    {
	ice->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELICE) ? strValue : "", FALSE);
    }
    if (openroaddev)
    {
	openroaddev->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELOPENROADDEV) ? strValue : "", FALSE);
    }
    if (openroadrun)
    {
	openroadrun->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELOPENROADRUN) ? strValue : "", FALSE);
    }
    if (onlinedoc)
    {
	char	DocLoc[MAX_PATH];

	sprintf(DocLoc, "%s\\ingres\\files\\english\\gs.pdf", m_installPath);
	onlinedoc->m_selected = FileExists(DocLoc);
    }
    if (sqlserver)
    {
	sqlserver->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELSQLSERVERGATEWAY) ? strValue : "", FALSE);
    }
    if (oracle)
    {
	oracle->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELORACLEGATEWAY) ? strValue : "", FALSE);
    }
    if (informix)
    {
	informix->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELINFORMIXGATEWAY) ? strValue : "", FALSE);
    }
    if (sybase)
    {
	sybase->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELSYBASEGATEWAY) ? strValue : "", FALSE);
    }
    if (vdba)
    {
	vdba->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELVDBA) ? strValue : "", FALSE);
    }

    if (embeddedcomponents) 
	embeddedcomponents->m_selected = theInstall.m_bEmbeddedRelease;

    if (jdbc) 
    {
	jdbc->m_selected = GetRegValueBOOL(
			strValue.LoadString(IDS_LABELJDBC) ? strValue : "", FALSE);
    }

    /* get the InstallSource */
    HKEY hRegKey=0;
    char guid[128];
    int idx;
	
    idx = (toupper(m_installationcode[0]) - 'A') * 26 + toupper(m_installationcode[1]) - 'A';
    if (idx <= 0)
	idx = (toupper(m_installationcode[0]) - 'A') * 26 + toupper(m_installationcode[1]) - '0';

	sprintf (guid, "{A78D%04X-2979-11D5-BDFA-00B0D0AD4485}", idx);
    sprintf (ach, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", guid);
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, ach, 0, KEY_QUERY_VALUE, &hRegKey))
    {
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(!RegQueryValueEx(hRegKey, "Cdimage", NULL, NULL, (BYTE *)szData, &dwSize))
	{
	    if (szData[strlen(szData)-1]=='\\') 
		szData[strlen(szData)-1]='\0';
	    m_InstallSource.Format("%s", szData);
	}
	RegCloseKey(hRegKey);
    }

    /* Set up complete string.*/
    FILE *fp;
    char filename[MAX_PATH];
    int ch, i;

    for (i=0; i<MAX_INGVER_SIZE; i++)
	m_completestring[i]='\0';

    sprintf (filename, "%s\\ingres\\version.rel", m_installPath);
    fp=fopen(filename, "r");
    if (fp)
    {
	i=-1;
	ch=fgetc(fp);
	while (ch != EOF)
	{
	    if (isdigit(ch) || isalpha(ch))
	    {
		i++;
		m_completestring[i]=ch;
	    }
	    ch=fgetc(fp);
	}
	fclose(fp);
    }
    _strlwr(m_completestring);

	/* Remove the old Ingres program string from Add/Remove Prgrams panel. */
	char UninstKey[256];
	sprintf(UninstKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Ingres [ %s ]\\DisplayName", m_installationcode);
	RegDeleteKey(HKEY_LOCAL_MACHINE,UninstKey);
	sprintf(UninstKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Ingres [ %s ]\\UninstallString", m_installationcode);
	RegDeleteKey(HKEY_LOCAL_MACHINE,UninstKey);
	sprintf(UninstKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Ingres [ %s ]", m_installationcode);
	RegDeleteKey(HKEY_LOCAL_MACHINE,UninstKey);
}

BOOL
CInstallation::Execute(LPCSTR lpCmdLine, BOOL bWait/*=TRUE*/, BOOL bWindow/*=FALSE*/)
{
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;
    DWORD dwCreationFlags=CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS;

    memset((char*)&pi, 0, sizeof(pi)); 
    memset((char*)&si, 0, sizeof(si)); 
    si.cb = sizeof(si);

    if (bWindow)
    {
	si.dwFlags=STARTF_USESHOWWINDOW;
	si.wShowWindow=SW_MINIMIZE;
	dwCreationFlags=NORMAL_PRIORITY_CLASS;
    }

    if (CreateProcess(NULL, (LPSTR)lpCmdLine, NULL, NULL, TRUE, 
                      dwCreationFlags, NULL, NULL, &si, &pi))
    {
	if (bWait)
	{
	    DWORD   dw;

	    WaitForSingleObject(pi.hProcess, INFINITE);
	    if (GetExitCodeProcess(pi.hProcess, &dw))
		return (dw == 0);
	}
	else
	    return TRUE;
    }
    return FALSE;
}

/*
**	History:
**	23-July-2001 (penga03)
**	    Delete II_CHARSET from symbol.tbl and add II_ICE_COLDFUSION to 
**	    symbol.tbl.
**	23-July-2001 (penga03)
**	    If Ingres/Ice was already installed, get the value of 
**	    m_HTTP_ServerPath, as well as m_ColdFusionPath, from symbol.tbl.
**	06-oct-2001 (penga03)
**	    Set the defalut value of m_HTTP_ServerPath to be the install path of 
**	    the HTTP server installed locally.
*/
BOOL
CInstallation::SetSymbolTbl()
{
    BOOL    error = FALSE;
    CString s;
    HANDLE	File;
    
    s = m_installPath + "\\ingres\\files\\symbol.tbl";
    if (GetFileAttributes (s) == -1)
    {
	File = CreateFile(s, GENERIC_WRITE, 0, NULL, 
	                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (File == INVALID_HANDLE_VALUE)
	{
	    error = TRUE;
	}
	CloseHandle(File);
    }

    AppendComment(IDS_SETTINGENV);
    AppendToLog(IDS_SETTINGENV);

	s = m_installPath + "\\ingres\\files\\" + m_language;
	if (_access(s, 00) == 0 )
	{
        s.Format("II_LANGUAGE %s", (LPCSTR)m_language);		  
        if (Exec("ingsetenv.exe", s))
	    error = TRUE;
	}

    s.Format("II_TIMEZONE_NAME %s", (LPCSTR)m_timezone);		  
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

    s.Format("TERM_INGRES %s", (LPCSTR)m_terminal);	  
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

    s.Format("II_INSTALLATION %s", (LPCSTR)m_installationcode); 
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

    s.Format("II_CHARSET%s %s", (LPCSTR)m_installationcode,
	     (LPCSTR) m_charset);		  
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

	s="II_CHARSET";
	Exec("ingunset.exe", s);

    s.Format("II_DATE_FORMAT %s", (LPCSTR)m_dateformat);		  
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

    s.Format("II_MONEY_FORMAT %s", (LPCSTR)m_moneyformat);		  
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

    s.Format("II_DECIMAL %s", (LPCSTR)m_decimal);		  
    if (Exec("ingsetenv.exe", s))
	error = TRUE;

    s.Format("II_TEMPORARY \"%s\"", (LPCSTR) m_temp);	  
    if (Exec("ingsetenv.exe",s)) 
	error=TRUE;

    s.Format("II_CONFIG	\"%s\\ingres\\files\"", (LPCSTR)m_installPath);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_GCN%s_LCL_VNODE \"%s\"", (LPCSTR) m_installationcode,
	     (LPCSTR)m_computerName);  
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_HOSTNAME \"%s\"", (LPCSTR)m_computerName);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    if (m_bClusterInstall)
    {
	s.Format("II_CLUSTER_RESOURCE \"%s\"", (LPCSTR)m_ClusterResource);
	if (Exec("ingsetenv.exe", s))
	    error = TRUE;
    }

    CComponent *ice = theInstall.GetICE();
    if ((ice) && (ice->m_selected))
    { 	
	/* If this is a upgrade, grab those values from symbol.tbl first. */
	if(!Local_NMgtIngAt("II_ICE_WEB_HTML", m_HTTP_ServerPath))
	{
	    m_HTTP_Server = GetRegValue("iihttpserver");
	    m_HTTP_ServerPath = GetRegValue("iihttpserverdir");
	}
	if (m_HTTP_ServerPath.IsEmpty())
	{
	    if (!CheckMicrosoft(m_HTTP_ServerPath))
	    {
		if (!CheckNetscape(m_HTTP_ServerPath))
		{
		    if (!CheckApache(m_HTTP_ServerPath))
		    {
			m_HTTP_Server="Other";
			m_HTTP_ServerPath =m_installPath;
		    }
		}
	    }
	}
		
	if(!Local_NMgtIngAt("II_ICE_COLDFUSION", m_ColdFusionPath))
	    m_ColdFusionPath = GetRegValue("iicoldfusiondir");
	if (m_ColdFusionPath.IsEmpty())
	    m_bInstallColdFusion = FALSE;
	else
	    m_bInstallColdFusion = TRUE;
		
	s.Format("II_ICE_WEB_HTML \"%s\"", (LPCSTR)m_HTTP_ServerPath);
	if (Exec("ingsetenv.exe", s)) 
	    error = TRUE;
		
	s.Format("II_ICE_COLDFUSION \"%s\"", (LPCSTR)m_ColdFusionPath);
	if (Exec("ingsetenv.exe", s)) 
	    error = TRUE;
    }

    AppendComment(error ? IDS_FAILED : IDS_DONE);
    return (!error);
}

BOOL
CInstallation::SetServerSymbolTbl()
{
    BOOL    error=FALSE;
    CString s;

    s.Format("II_DATABASE \"%s\"", (LPCSTR)m_iidatabase);  
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_WORK \"%s\"", (LPCSTR)m_iiwork);  
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_DUMP \"%s\"", (LPCSTR)m_iidump);  
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_CHECKPOINT \"%s\"", (LPCSTR)m_iicheckpoint);	
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_JOURNAL \"%s\"", (LPCSTR)m_iijournal);	 
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    return (!error);
}

BOOL
CInstallation::CreateServerDirectories()
{
    BOOL    bret=TRUE;
    CString s;

    bret = CreateIIDATABASE();

    if (bret)
	bret = CreateIICHECKPOINT();

    if (bret)
	bret = CreateIIJOURNAL();

    if (bret)
	bret = CreateIIDUMP();

    if (bret)
	bret = CreateIIWORK();

    if (bret)
	bret = CreateIILOGFILE();

    return (bret);
}

BOOL
CInstallation::DatabaseExists(LPCSTR lpName)
{
    CString strDBPath; 
	
    strDBPath.Format("%s\\ingres\\data\\default\\%s", m_iidatabase, lpName);
    if (GetFileAttributes(strDBPath) == -1)
		return FALSE;
	else
		return TRUE;
}

BOOL
CInstallation::CreateOneDatabase(LPCSTR lpName)
{
    CString param;

    param = lpName;
    return (Exec("createdb.exe", param) == 0);
}

BOOL
CInstallation::CheckpointOneDatabase(LPCSTR lpName)
{
    CString param;

    param = lpName;
    return (Exec("ckpdb.exe", param) == 0);
}

/*
**	History:
**	23-July-2001 (penga03)
**	    Show right comments while upgrading Ingres/Ice.
**	06-mar-2002 (penga03)
**	    Created evaluation releases specific databases. 
**	21-mar-2002 (penga03)
**	    For evaluation releases, reload icetutor, portaldb, 
**	    fortunoff and infocadb after they are created. 
**	11-mar-2004 (somsa01)
**	    Removed upgrade of imadb, icedb, icetutor, and icesvr, as they
**	    are now treated as system databases and will get upgraded
**	    automatically by an upgradedb on iidbdb.
*/
BOOL
CInstallation::CreateDatabases()
{
    CComponent	*dbms = theInstall.GetDBMS();
    CComponent	*ice = theInstall.GetICE();
    BOOL	bret = TRUE;

    if (dbms && dbms->m_selected)
    {
	if (!m_DBMSupgrade)
	{
	    AppendComment(IDS_CREATESYSDB);
	    AppendToLog(IDS_CREATESYSDB);
	}
	else
	{
	    AppendComment(IDS_UPDATESYSDB);
	    AppendToLog(IDS_UPDATESYSDB);
	}

	if (DatabaseExists("iidbdb"))
	{
	    bret = (Exec("upgradedb", "iidbdb") == 0);
	}
	else
	{
	    if (!CreateOneDatabase("-S iidbdb"))
		bret = FALSE;
	}

	if (bret && !CheckpointOneDatabase("+j iidbdb -l +w"))
	    bret = FALSE;

	if (!DatabaseExists("imadb"))
	{
	    if (bret && !CreateOneDatabase("imadb -u$ingres -fnofeclients"))
		bret = FALSE;

#ifdef EVALUATION_RELEASE
    char theFile[MAX_PATH], tmpFile[MAX_PATH];
    char CurDir[MAX_PATH], NewDir[MAX_PATH];

	    if(bret)
	    {
		/*
		** modify [II_NT_SYSTEM] in
		** %II_SYSTEM%\ingres\temp\copy_imadb.in
		*/
		sprintf(theFile, "%s\\ingres\\temp\\copy_imadb.in", m_installPath);
		sprintf(tmpFile, "%s\\ingres\\temp\\copy_imadb.tmp", m_installPath);
		ReplaceString("[II_NT_SYSTEM]", m_installPath, theFile, tmpFile);
		CopyFile(tmpFile, theFile, FALSE );
		DeleteFile(tmpFile);
	    }
#endif /* EVALUATION_RELEASE */
	}

	//creates demodb database
	if (m_installdemodb)
	{
		if (!DatabaseExists("demodb"))
		{
			if (bret && !CreateOneDatabase("-i demodb"))
				bret = FALSE;
		}
	}

	
	AppendComment(bret ? IDS_DONE : IDS_FAILED);
	}

    if (ice && ice->m_selected)
    {
	BOOL bCreateICEDB=0;

	if (bret && !Create_icedbuser())
	    bret = FALSE;

	if (!DatabaseExists("icesvr"))
	{
	    bCreateICEDB=1;
	    AppendComment(IDS_CREATEICEDB);
	    AppendToLog(IDS_CREATEICEDB);

	    if (bret && !CreateOneDatabase("icesvr"))
		bret = FALSE;

#ifdef EVALUATION_RELEASE
	    if(bret)
	    {
		CString t(m_installPath); t.Replace("\\", "/");

		/*
		** modify [II_SYSTEM] in
		** %II_SYSTEM%\ingres\temp\icesdk.sql
		*/
		sprintf(theFile, "%s\\ingres\\temp\\icesdk.sql", m_installPath);
		sprintf(tmpFile, "%s\\ingres\\temp\\icesdk.tmp", m_installPath);
		ReplaceString("[II_SYSTEM]", t, theFile, tmpFile);
		CopyFile(tmpFile, theFile, FALSE );
		DeleteFile(tmpFile);
	    }
#endif /* EVALUATION_RELEASE */

	}

	if (!DatabaseExists("icetutor"))
	{
	    if (bret && !CreateOneDatabase("icetutor -uicedbuser"))
		bret = FALSE;

#ifdef EVALUATION_RELEASE
	    if(bret)
	    {
		/* populate icetutor */
		GetCurrentDirectory(sizeof(CurDir), CurDir);
		sprintf(NewDir, "%s\\DBupdate\\icetutor", m_InstallSource);
		SetCurrentDirectory(NewDir);
		Exec("reload.bat", 0, TRUE, FALSE);
		SetCurrentDirectory(CurDir);
	    }
#endif /* EVALUATION_RELEASE */
	}

	if (!DatabaseExists("icedb"))
	{
	    if (bret && !CreateOneDatabase("icedb -uicedbuser"))
		bret = FALSE;
	}

	if (bCreateICEDB)
	    AppendComment(bret ? IDS_DONE : IDS_FAILED);
    }

#ifdef EVALUATION_RELEASE

    if (!Create_OtherDBUsers())
	bret = FALSE;

    /* create and populate portaldb */
    if (DatabaseExists("portaldb") && !theInstall.m_upgradedatabases)
    {
	AppendComment(IDS_UPDATEOTHERDB);
	AppendToLog(IDS_UPDATEOTHERDB);

	Exec("upgradedb", "portaldb");
    }
    else
    {
	AppendComment(IDS_CREATEOTHERDB);
	AppendToLog(IDS_CREATEOTHERDB);

	if (!CreateOneDatabase("portaldb -uportaldba"))
	    bret = FALSE;
    }

    if (bret)
    {
	CString PortNum;
	char HostName[MAX_COMP_LEN];

	/* 
	** modify [II_NT_SYSTEM], [HOSTNAME], [ICE_PORT], [PORTAL_PORT] 
	** and [OR_PORT] in %II_SYSTEM%\ingres\temp\copy_portaldb.in 
	*/	
	sprintf(theFile, "%s\\ingres\\temp\\copy_portaldb.in", m_installPath);
	sprintf(tmpFile, "%s\\ingres\\temp\\copy_portaldb.tmp", m_installPath);

	ReplaceString("[II_NT_SYSTEM]", m_installPath, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	if(GetHostName(HostName))
	    ReplaceString("[HOSTNAME]", HostName, theFile, tmpFile);
	else
	    ReplaceString("[HOSTNAME]", m_computerName, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_IcePortNumber);
	ReplaceString("[ICE_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_PortalHostPort);
	ReplaceString("[PORTAL_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_OrPortNumber);
	ReplaceString("[OR_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	/* 
	** modify [HOSTNAME], [ICE_PORT], [PORTAL_PORT] and [OR_PORT] 
	** in %II_SYSTEM%\ingres\temp\updpath_portaldb.sql 
	*/
	sprintf(theFile, "%s\\ingres\\temp\\updpath_portaldb.sql", m_installPath);
	sprintf(tmpFile, "%s\\ingres\\temp\\updpath_portaldb.tmp", m_installPath);

	if(GetHostName(HostName))
	    ReplaceString("[HOSTNAME]", HostName, theFile, tmpFile);
	else
	    ReplaceString("[HOSTNAME]", m_computerName, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_IcePortNumber);
	ReplaceString("[ICE_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_PortalHostPort);
	ReplaceString("[PORTAL_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_OrPortNumber);
	ReplaceString("[OR_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	/* populate portaldb */
	GetCurrentDirectory(sizeof(CurDir), CurDir);
	sprintf(NewDir, "%s\\DBupdate\\portaldb", m_InstallSource);
	SetCurrentDirectory(NewDir);
	Exec("reload.bat", 0, TRUE, FALSE);
	SetCurrentDirectory(CurDir);
    }

    /* create and populate fortunoff */
    if (DatabaseExists("fortunoff") && !theInstall.m_upgradedatabases)
	Exec("upgradedb", "fortunoff"); 
    else
    {
	if (!CreateOneDatabase("fortunoff -uicedbuser"))
	    bret = FALSE;
    }

    if (bret)
    {
	/* populate fortunoff */
	char para[MAX_PATH];

	sprintf(para, "fortunoff \"%s\\ingres\\ice\\fortunoff\\cfg\" \"%s\\ingres\\ice\\fortunoff\\images\"", m_installPath, m_installPath);
	Exec(m_installPath + "\\ingres\\bin\\fortunoff.exe", para);
    }

    /* create and populate timeregs */
    if (DatabaseExists("timeregs") && !theInstall.m_upgradedatabases)
	Exec("upgradedb", "timeregs"); 
    else
    {
	if (!CreateOneDatabase("timeregs -utimregs"))
	    bret = FALSE;
    }

    if (bret)
    {
    }

    /* create and populate infocadb */
    if (DatabaseExists("infocadb") && !theInstall.m_upgradedatabases)
	Exec("upgradedb", "infocadb"); 
    else
    {
	if (!CreateOneDatabase("infocadb -n"))
	    bret = FALSE;
    }

    if (bret)
    {
	CString PortNum;
	char HostName[MAX_COMP_LEN];

	/*
	** modify [II_NT_SYSTEM] in 
	** %II_SYSTEM%\ingres\temp\copy_infocadb.in;
	*/	
	sprintf(theFile, "%s\\ingres\\temp\\copy_infocadb.in", m_installPath);
	sprintf(tmpFile, "%s\\ingres\\temp\\copy_infocadb.tmp", m_installPath);
	ReplaceString("[II_NT_SYSTEM]", m_installPath, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	/* 
	** modify [HOSTNAME], [ICE_PORT], [PORTAL_PORT] and [OR_PORT] 
	** in %II_SYSTEM%\ingres\temp\updpath_infocadb.sql 
	*/
	sprintf(theFile, "%s\\ingres\\temp\\updpath_infocadb.sql", m_installPath);
	sprintf(tmpFile, "%s\\ingres\\temp\\updpath_infocadb.tmp", m_installPath);

	if(GetHostName(HostName))
	    ReplaceString("[HOSTNAME]", HostName, theFile, tmpFile);
	else
	    ReplaceString("[HOSTNAME]", m_computerName, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_IcePortNumber);
	ReplaceString("[ICE_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_PortalHostPort);
	ReplaceString("[PORTAL_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	PortNum.Format("%d", m_OrPortNumber);
	ReplaceString("[OR_PORT]", PortNum, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	/* populate infocadb */
	GetCurrentDirectory(sizeof(CurDir), CurDir);
	sprintf(NewDir, "%s\\DBupdate\\infocadb", m_InstallSource);
	SetCurrentDirectory(NewDir);
	Exec("reload.bat", 0, TRUE, FALSE);
	SetCurrentDirectory(CurDir);
   }
	  
    AppendComment(bret ? IDS_DONE : IDS_FAILED);

#endif /* EVALUATION_RELEASE */

    return bret;
}

BOOL
CInstallation::UpgradeDatabases()
{
    BOOL bret;

    AppendComment(IDS_UPGRADEDATABASES);
    AppendToLog(IDS_UPGRADEDATABASES);

    bret = Exec(m_installPath + "\\ingres\\bin\\upgradedb.exe", "-all");
    
    if (bret)
    {
	CString	strUpgradeLog;

	strUpgradeLog.Format("%s\\ingres\\files\\upgradedb\\%s\\upgrade.log",
		m_installPath, m_userName);
	if (AskUserYN(IDS_ERRORUPGRADINGDATABASES, strUpgradeLog))
	{
	    CString s = "notepad.exe \"" + strUpgradeLog + "\"";

	    Execute(s, TRUE);
	}
    }

    AppendComment(bret ? IDS_FAILED : IDS_DONE );

    return (bret==0);
}

void
CInstallation::AddComponent(UINT uiStrID, double size, BOOL selected, BOOL visible, LPCSTR path)
{
    CString name;

    name.LoadString(uiStrID);
    CComponent *c = new CComponent(name, size, selected, visible, path);
    if (c)
	m_components.Add(c);
}

void
CInstallation::DeleteComponents()
{
    int		i;
    CComponent	*c;

    for (i=0;i<m_components.GetSize();i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c)
	    delete c;
    }
    m_components.RemoveAll();
}

CInstallation::~CInstallation()
{
    DeleteComponents();
}

CInstallation::CInstallation() 
{
    char    szBuffer[2048];
    time_t  t;
    DWORD   dwBufferSize;
    
    time(&t);
    m_startTime = t;
    m_win95 = IsWindows95();
    m_reboot = FALSE;
    m_halted = FALSE;
    m_logFile = 0;
    m_process = 0;
    m_postInstThread = 0;
    m_charCount = 0;
    m_attemptedUDBupgrade = FALSE;
    
    dwBufferSize = sizeof(szBuffer); 			
    GetUserName(szBuffer, &dwBufferSize);
    m_userName = szBuffer;

    RetrieveHostname(szBuffer);
    m_computerName = szBuffer;

    if(!GetLocalizedAdminName())
	m_adminName = "Administrator";

	/*
    m_temp = "C:\\";
    
    m_ingresAlreadyInstalled = AlreadyThere();
    
    if(GetEnvironmentVariable("II_SYSTEM", szBuffer, sizeof(szBuffer)))
	m_installPath = szBuffer;
    else
    {
	m_ingresAlreadyInstalled = FALSE;
	m_installPath = "C:\\IngresII";
    }
	*/
    m_postInstRet = TRUE;

    m_IcePortNumber=2552;
    m_OrPortNumber=7982;
    m_PortalHostPort=8080;
}

DWORD
CInstallation::ThreadAppendToLog()
{
    DWORD   dw;
    char    c;

    while (m_logFile)
    {
	if (ReadFile(m_hChildStdoutRd, &c, 1, &dw, NULL))
	{
	    WriteFile(m_logFile, &c, 1, &dw, NULL);
	    m_charCount++;
	}
    }
    return 0;
}

DWORD __stdcall
ThreadPostInstallationFunc(LPVOID param)
{
    theInstall.m_postInstRet = ((CInstallation *)param)->ThreadPostInstallation();
    theInstall.m_postInstThread = 0;
    return theInstall.m_postInstRet;
}

DWORD __stdcall
LogFunc(LPVOID param)
{
    return ((CInstallation *) param)->ThreadAppendToLog();
}

void
CInstallation::PostInstallation(CStatic *output)
{
    DWORD dw;

    m_output = output;
    m_postInstThread = CreateThread(NULL, 0, ::ThreadPostInstallationFunc,
				    (LPVOID)this, 0, &dw);
    SetThreadPriority(m_postInstThread, THREAD_PRIORITY_ABOVE_NORMAL);
}

/*
**  History:
**	07-sep-2001 (penga03)
**	    On Windows 98 and Windows ME, use CreateProcess.
**	20-Nov-2008 (drivi01)
**	    Update CreateProcess to return error number in exitCode
**	    variable if CreateProcess failed ot execute.  
**	    CreateProcess returns 0 indicating OK status if 
**	    CreateProcess failed to execute which i think can be misleading.
*/
int
CInstallation::Exec(LPCSTR cmdLine, LPCSTR param, BOOL appendToLog, BOOL changeDir)
{
    int			error = 0;
    DWORD		exitCode = 0;
    SECURITY_ATTRIBUTES saAttr;
    DWORD		dw = 0;
    HANDLE		logThread = 0;
    HANDLE		hChildStdoutWr = 0, hChildStdinRd = 0, hChildStdinWr = 0;
    CString		cmd;
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;
    char		achCurdir[1024];
    CString		Newdir;

    memset((char*)&pi, 0, sizeof(pi));
    memset((char*)&si, 0, sizeof(si));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    CreateSecurityDescriptor(&saAttr);

    GetCurrentDirectory(sizeof(achCurdir), achCurdir);

    if (changeDir)
    {
	Newdir.Format("%s\\ingres\\bin", (LPCSTR)m_installPath);
	SetCurrentDirectory(Newdir);
    }

    if (changeDir && !_tcsstr(cmdLine, (LPCSTR)m_installPath))
	cmd.Format("\"%s\\ingres\\bin\\%s\"", (LPCSTR) m_installPath, (LPCSTR) cmdLine);
    else
	cmd.Format("\"%s\"", (LPCSTR)cmdLine);

    if (param)
    {
	cmd += " " ;
	cmd += param;
    }

    AppendToLog(cmd);

    CreatePipe(&m_hChildStdoutRd, &hChildStdoutWr, &saAttr, 0);
    CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0);
    DuplicateHandle(GetCurrentProcess(), hChildStdinWr, GetCurrentProcess(),
		    &hChildStdinWr, 0, FALSE, DUPLICATE_SAME_ACCESS);

    si.cb = sizeof(si);  

    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = hChildStdinRd;
    si.hStdOutput = hChildStdoutWr;
    si.hStdError = hChildStdoutWr;

	/*
	if(m_win95)
	{
		if(!::CreateProcess(NULL, (LPSTR)(LPCSTR)cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
		{
			goto end;
		}
	}
	else
	{
    if (!::CreateProcessAsUser( m_phToken, NULL, (LPSTR)(LPCSTR)cmd, NULL,
				NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL,
				&si, &pi ))
    {
	goto end;
    }
	}
	*/
	
	if(!::CreateProcess(NULL, (LPSTR)(LPCSTR)cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		exitCode=GetLastError();
		goto end;
	}

    m_process = pi.hProcess;
    CloseHandle(hChildStdinRd);
    hChildStdinRd = 0; 
    CloseHandle(hChildStdoutWr);
    hChildStdoutWr = 0; 

    if (appendToLog)
	logThread = CreateThread(NULL, 0, LogFunc, (LPVOID)this, 0, &dw);
    
    WaitForSingleObject(pi.hProcess, INFINITE);

    if (appendToLog)
    {
	WaitForSingleObject(logThread, 500);
	if (GetExitCodeThread (logThread, &dw) && dw == STILL_ACTIVE)
	    TerminateThread(logThread, 0);
    }

    if (GetExitCodeProcess(pi.hProcess, &dw))
	exitCode = dw;
    
end:
    if (hChildStdinRd)
	CloseHandle(hChildStdinRd);
    if (hChildStdoutWr)
	CloseHandle(hChildStdoutWr);
    m_process = 0;
    SetCurrentDirectory(achCurdir);
    return exitCode;
}

void
CInstallation::AppendComment(LPCSTR p)
{
    CString s;

    if ((!p) || (!m_output))
	return;

    m_output->GetWindowText(s);
    s += p;
    m_output->SetWindowText(s);
    m_output->UpdateWindow();
}

void
CInstallation::AppendComment(UINT uiStrID)
{
    CString s;
	static bool tabremove=false;
    s.LoadString(uiStrID);
	if (theInstall.m_DBATools)
	{
		s.Replace("Ingres", "DBA Tools");
		//make sure output appears correct, align DONE|FAILED column
		if (uiStrID!=IDS_DONE && uiStrID!=IDS_FAILED )
		{
			int index=0;
			int count=0;
			while(s.GetLength()>=index && index>=0)
			{
				index = s.Find('\t', index+1);
				if (index>0)
					count++;
			}
			/* We'll assume maximum length of the characters that count is 35 
			** due to the fact that DONE or FAILED also contain \t and \n chars 
			** around them. Any string will get 2 tabs automatically to avoid
			** DONE sticking to its item in the list. For every character between 35 
			** and string length we will assume tab for every 5 characters and 1 more 
			** tab for leftover characters between 1 and 4.
			*/
			int length=0;
			if (s.GetLength()<35)
				length = 35-s.GetLength();
			int remainder = length % 5;
			int tabs = 2 + (length / 5);
			if (remainder)
				remainder = 1;
			tabs = tabs + remainder;
			while (count > tabs)
			{
				s.Remove('\t');
				count--;
			}
			while (count < tabs)
			{
				s.Append("\t");
				count++;
			}
		}
	}
    AppendComment(s);
}

void
CInstallation::AppendToLog(LPCSTR p)
{
    DWORD   dw;
    CString s;

    if (!m_logFile) 
	return;

    if (_tcsstr(p,"..."))
    {
	s.LoadString(IDS_SEPARATOR);
	WriteFile(m_logFile,s,s.GetLength(),&dw,NULL);
    }
    s = p;
    s += END_OF_LINE; 
    WriteFile(m_logFile, s,s.GetLength(), &dw, NULL);
}

void
CInstallation::AppendToLog(UINT uiStrID)
{
    CString s;
    
    s.LoadString(uiStrID);
	if (theInstall.m_DBATools)
		s.Replace("Ingres", "DBA Tools");
    AppendToLog(s);
}

BOOL
CInstallation::IsBusy()
{
    DWORD   dw;

    if (!m_postInstThread)
	return FALSE;

    if (!GetExitCodeThread(m_postInstThread,&dw))
	return FALSE;

    return (dw == STILL_ACTIVE);
}

BOOL
CInstallation::AppendToFile(CString &s, CString &value)
{
    BOOL    bret = FALSE;
    HANDLE File;
    
    File = CreateFile(s, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (File != INVALID_HANDLE_VALUE)
    {
	DWORD	dw=0;

	if (SetFilePointer(File, 0, (LONG*)&dw, FILE_END) != -1)
	{
	    value += END_OF_LINE;
	    bret = WriteFile(File, value, value.GetLength(), &dw, NULL);
	}
	CloseHandle(File);
    }

    if (!bret)
    {
	CString err;
	
	err.Format(IDS_CANNOTWRITE, (LPCSTR)value, (LPCSTR)s);
	AppendToLog(err);
    }
    return bret;
}

BOOL
CInstallation::AppendToFile(CString & file, LPSTR p)
{
    return AppendToFile(file, CString(p));
}

void
CInstallation::SetOneEnvironmentVariable(LPCSTR lpEnv, LPCSTR lpValue, BOOL bAdd)
{
    char    buf[12000];
	DWORD dwError;

    strcpy(buf, lpValue);
    
    if (bAdd)
    {
	char	oldEnv[8000];

	if (GetEnvironmentVariable(lpEnv, oldEnv, sizeof(oldEnv)))
	{
	    strcat(buf, ";");
	    strcat(buf, oldEnv);
	}
    }
    if (!SetEnvironmentVariable(lpEnv,buf))
		dwError=GetLastError();
    
    CString s = lpEnv;
    s += "=";
    s += buf;
    AppendToLog(s);
}

DWORD __stdcall
ThreadMsgFunc(LPVOID param)
{
	::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)"Environment");

    return 0;
}

void
CInstallation::SetFileAssociations()
{
  HKEY  hKey=0;	
  char szBuffer[MAX_PATH], szKey[255];
  
  AppendToLog(IDS_SETTINGFILEASSOCIATIONS);
  SetRegistryEntry(HKEY_CLASSES_ROOT, ".log", "", REG_SZ, "txtfile");

  // Set association for VDBA
  sprintf(szKey, "Ingres.VDBA.%s", m_installationcode); 
  SetRegistryEntry(HKEY_CLASSES_ROOT, szKey, "", REG_SZ, "Ingres VDBA Config File");

  SetRegistryEntry(HKEY_CLASSES_ROOT, ".vdbacfg", "", REG_SZ, szKey);

  sprintf(szKey, "Ingres.VDBA.%s\\shell\\open\\command", m_installationcode);
  sprintf(szBuffer, "\"%s\\ingres\\bin\\vdba.exe\" \"%%1\"", m_installPath);
  SetRegistryEntry(HKEY_CLASSES_ROOT, szKey, "", REG_SZ, szBuffer);

  // Set association for IIA
  sprintf(szKey, "Ingres.IIA.%s", m_installationcode);
  SetRegistryEntry(HKEY_CLASSES_ROOT, szKey, "", REG_SZ, "Ingres Import Assistant File");

  SetRegistryEntry(HKEY_CLASSES_ROOT, ".ii_imp", "", REG_SZ, szKey);

  sprintf(szKey, "Ingres.IIA.%s\\shell\\open\\command", m_installationcode);
  sprintf(szBuffer, "\"%s\\ingres\\bin\\iia.exe\" \"%%1\"", m_installPath);
  SetRegistryEntry(HKEY_CLASSES_ROOT, szKey, "", REG_SZ, szBuffer);
}

void CInstallation::DeleteObsoleteFileAssociations()
{
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.VDBA\\shell\\open\\command");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.VDBA\\shell\\open");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.VDBA\\shell");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.VDBA");

	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.IIA\\shell\\open\\command");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.IIA\\shell\\open");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.IIA\\shell");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres.IIA");

	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres_Database\\shell");
	RegDeleteKey(HKEY_CLASSES_ROOT, "Ingres_Database");
}

void CInstallation::SetEnvironmentVariables()
{
    CString s;
    char    tempbuf[1024];
    
    AppendToLog(IDS_SETENVIRONMENTVARIABLES);
    
    SetOneEnvironmentVariable("II_SYSTEM", m_installPath);
    sprintf(tempbuf, "II_SYSTEM=%s", m_installPath);
    putenv(tempbuf);
    
    SetOneEnvironmentVariable("II_TEMPORARY", m_temp);
    
	if (m_includeinpath)
	{
	    s.Format("%s\\ingres\\bin;%s\\ingres\\utility;",
		     (LPCSTR)m_installPath, (LPCSTR)m_installPath);
		SetOneEnvironmentVariable("PATH", s, TRUE);
	}
    
    s.Format("%s\\ingres\\lib", (LPCSTR)m_installPath);
    SetOneEnvironmentVariable("LIB", s, TRUE);
    
    s.Format("%s\\ingres\\files", (LPCSTR)m_installPath);
    SetOneEnvironmentVariable("INCLUDE", s, TRUE);
    
    DWORD dw;
    HANDLE hmsgthrd = CreateThread(NULL, 0, ::ThreadMsgFunc, 0, 0, &dw);
    WaitForSingleObject(hmsgthrd, 15000);
    if (GetExitCodeThread(hmsgthrd, &dw) && dw == STILL_ACTIVE)
	TerminateThread(hmsgthrd, 0);
}

BOOL
CInstallation::CleanSharedMemory()
{
    return (Exec("ipcclean.exe", 0) == 0);
}

/*
**	History:
**	23_July-2001 (penga03)
**	    Function DatabaseExists needs m_iidatabase.
**	23-July-2001 (penga03)
**	    If Ingres DBMS was already installed, get the information of 
**	    transaction log files from config.dat.
**	23-July-2001 (penga03)
**	    Config transaction log file only if Ingres DBMS is installed.
**	06-mar-2002 (penga03)
**	    Set cursor_limit to be 256 for evaluation releases.
**	21-mar-2002 (penga03)
**	    For evaluation releases, added OpenROAD user, orspo_runas, 
**	    in config.dat.
**	29-jan-2004 (somsa01)
**	    Take a stab at better handling of new/changed parameters.
**	    Avoid iigenres if it looks like some kind of valid DBMS
**	    configuration exists.
**	13-Aug-2009 (drivi01)
**	    Updated date_alias for new installs to rely on installer
**	    settings as opposed to dbms.crs settings.
**	08-Mar-2010 (thaju02)
**	    Remove max_tuple_length.
**	12-May-2010 (drivi01)
**          Add two new parameters on the upgrade offline_error_action
**          and online_error_action.
*/
BOOL
CInstallation::SetConfigDat()
{
    BOOL	error = FALSE;
    CString	s, sConfigDat;
    CString	ConfigKey, ConfigKeyValue, Host; 
    CString	cmd; 
    DWORD	dw = 0;
    HANDLE	File;
    CComponent	*dbms = theInstall.GetDBMS();
    CString	temp, BakCrsFile, SetupString;

    Host = m_computerName;
    Host.MakeLower();

    ConfigKey.Format("ii.%s.config.dbms.%s", Host, m_completestring);
    Local_PMget(ConfigKey, SetupString);

    m_logname = "INGRES_LOG";
    m_duallogname = "DUAL_LOG";

    /*
    ** If this is a DBMS upgrade, grab those values from symbol.tbl or
    ** config.dat.
    */
    if (!Local_NMgtIngAt("II_DATABASE", m_iidatabase))
	m_iidatabase = GetRegValue("iidatabasedir");
    if (m_iidatabase.IsEmpty())
	m_iidatabase = theInstall.m_installPath;
    if (DatabaseExists("iidbdb"))
    {
	m_DBMSupgrade = TRUE;
	s.Format("%s\\ingres\\files\\config.dat", m_installPath);
	if ((GetFileAttributes(s) == -1) && m_bClusterInstall)
	    m_bClusterInstallEx = 1;
    }

    AppendComment(IDS_SETTINGCONF);
    AppendToLog(IDS_SETTINGCONF);

	BOOL bLogFileExist;

	ConfigKey.Format("ii.%s.rcp.log.log_file_1", Host);
	if (Local_PMget(ConfigKey, m_iilogfile))
		bLogFileExist=TRUE;
	else
		bLogFileExist=FALSE;

    if (bLogFileExist)
    {
	CString	temp2, temp3, ConfigKey2;


	/* 
	** New parameters for release 10.0
	*/
	cmd.Format("offline_error_action \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("online_error_action \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);

	/*
	** Brand new parameters for this release.
	*/
	cmd.Format("cache_guideline \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("opf_pq_dop \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("opf_pq_threshold \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("qef_sorthash_memory \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("qsf_guideline \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);


	/*
	** Newly exposed parameters, might have an existing value.
	*/
	cmd.Format("-keep dmf_int_sort_size \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep gc_interval \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep gc_num_ticks \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep gc_threshold \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep hex_session_ids \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep opf_hash_join \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep pindex_bsize \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep pindex_nbuffers \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep psf_maxmemf \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep psort_bsize \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep psort_nthreads \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep psort_rows \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep qef_hash_mem \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep rep_dq_lockmode \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep rep_dt_maxlocks \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep rep_iq_lockmode \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep rep_sa_lockmode \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep rule_upd_prefetch \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep size_io_buf \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep ulm_chunk_size \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep archiver_refresh \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);

	/*
	** Special case for recovery database-limit, name is used
	** for other things we don't want to touch.
	*/
	ConfigKey.Format("ii.%s.recovery.*.database_limit", Host);
	if (Local_PMget(ConfigKey, temp))
	{
	    ConfigKey.Format("ii.%s.rcp.log.database_limit", Host);
	    Local_PMget(ConfigKey, temp);
	    cmd.Format( "ii.%s.recovery.*.database_limit %d", m_computerName,
			atoi(temp) );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}

	/*
	** Special case for recovery 2K cache, increase if it's the old
	** default value.
	*/
	ConfigKey.Format("ii.%s.rcp.dmf_cache_size", Host);
	if (Local_PMget(ConfigKey, temp) && atoi(temp) == 200)
	{
	    cmd.Format("ii.%s.rcp.dmf_cache_size 1000", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}

	/*
	** Brand new parameter for Ingres 2006 Release 2, for existing installation 
	** previous behavior should be preserved.
	** date_type should be set to INGRESDATE instead of ANSIDATE for upgrades
	** on modify follow the setting chosen in the dialog.
	*/
	if (m_ansidate)
	{
		cmd.Format("date_alias \"%s\\ingres\\files\\dbms.rfm",
		   (LPCSTR)m_installPath);
		Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	}
	else
	{
		ConfigKey.Format("ii.%s.config.date_alias INGRESDATE", m_computerName);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", ConfigKey, FALSE);
	}


	/*
	** Trade in old parameters for new.
	** Compute qef_dsh_memory from old qef_qep_mem.
	** Copy old rdf_tbl_cols to new rdf_col_defaults.
	** Compute cp_interval_mb from old cp_interval.
	** Compute dmf_tcb_limit from old dmf_hash_size.
	** The temporary rules have to live in II_CONFIG.
	*/
	temp.Format("%s\\ingres\\files\\postdbms.crs", (LPCSTR)m_installPath);
	RemoveFile(temp);
	AppendToFile(temp, "ii.$.dbms.$.dmf_tcb_limit:\t8 * ii.$.dbms.$.dmf_hash_size, MIN = 2048;\n");
	AppendToFile(temp, "ii.$.dbms.*.qef_dsh_memory:\tii.$.dbms.$.qef_qep_mem * ii.$.dbms.$.cursor_limit * ii.$.dbms.$.connect_limit;\n");
	AppendToFile(temp, "ii.$.dbms.$.rdf_col_defaults:\tii.$.dbms.$.rdf_tbl_cols;\n");
	AppendToFile(temp, "-- in case of no existing rdf_tbl_cols:\n");
	AppendToFile(temp, "ii.$.dbms.$.rdf_tbl_cols:	50;\n");
	AppendToFile(temp, "ii.$.rcp.log.cp_interval_mb:	(ii.$.rcp.file.kbytes * (ii.$.rcp.log.cp_interval / 100) + 512) / 1024, MIN = 1;\n");
	AppendToFile(temp, "ii.$.rcp.log.cp_interval:	5;\n");

	temp2.Format("%s\\ingres\\files\\dbms.rfm", (LPCSTR)m_installPath);
	temp3.Format("%s\\ingres\\temp\\rfmtemp", (LPCSTR)m_installPath);
	CopyFile(temp2, temp3, FALSE);
	AppendToFile(temp3, "rulefile.99: postdbms.crs\n");

	cmd.Format("-keep qef_dsh_memory \"%s\\ingres\\temp\\rfmtemp",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep rdf_col_defaults \"%s\\ingres\\temp\\rfmtemp",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep cp_interval_mb \"%s\\ingres\\temp\\rfmtemp",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);
	cmd.Format("-keep dmf_tcb_limit \"%s\\ingres\\temp\\rfmtemp",
		   (LPCSTR)m_installPath);
	Exec(m_installPath + "\\ingres\\utility\\iiinitres.exe", cmd, FALSE);

	RemoveFile(temp3);
	RemoveFile(temp);

	/*
	** An upgrade installation needs a larger buffer_count and
	** stack size of 128K for dbms and recovery servers.
	*/
	ConfigKey.Format("ii.%s.rcp.log.buffer_count", Host);
	if (Local_PMget(ConfigKey, temp) && atoi(temp) < 35)
	{
	    cmd.Format("ii.%s.rcp.log.buffer_count 35", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}

	ConfigKey.Format("ii.%s.dbms.*.stack_size", Host);
	ConfigKey2.Format("ii.%s.recovery.*.stack_size", Host);
	if (Local_PMget(ConfigKey, temp) && Local_PMget(ConfigKey2, temp2))
	{
	    if (atoi(temp) < 131072)
	    {
		cmd.Format("ii.%s.dbms.*.stack_size 131072", m_computerName);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd,
		     FALSE);
	    }

	    if (atoi(temp2) < 131072)
	    {
		cmd.Format("ii.%s.recovery.*.stack_size 131072",
			   m_computerName);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd,
		     FALSE);
	    }
	}

	ConfigKey.Format("ii.%s.rcp.file.kbytes", Host);
	if (!Local_PMget(ConfigKey, m_iilogfilesize))
	{
	    ConfigKey.Format("ii.%s.rcp.file.size", Host);
	    Local_PMget(ConfigKey, temp);
	    temp.Format("%d", atoi(temp)/1024);
	    m_iilogfilesize = temp;
	}

	ConfigKey.Format("ii.%s.rcp.log.log_file_1", Host);
	if (!Local_PMget(ConfigKey, m_iilogfile))
	    Local_NMgtIngAt("II_LOG_FILE", m_iilogfile);

	ConfigKey.Format("ii.%s.rcp.log.dual_log_1", Host);
	if (Local_PMget(ConfigKey, m_iiduallogfile) ||
	    Local_NMgtIngAt("II_DUAL_LOG", m_iiduallogfile))
	{
	    m_enableduallogging = TRUE;
	}
	else
	    m_enableduallogging = FALSE;

	ConfigKey.Format("ii.%s.fixed_prefs.iso_entry_sql-92", Host);
	Local_PMget(ConfigKey, ConfigKeyValue);
	if (!ConfigKeyValue.CompareNoCase("ON"))
	    m_sql92 = TRUE;
	else
	    m_sql92 = FALSE;

	/*
	** Temporarily rename dbms.crs so that it does not get invoked
	** during iigenres.
	*/
	temp.Format("%s\\ingres\\files\\dbms.crs", (LPCSTR)m_installPath);
	BakCrsFile.Format("%s\\ingres\\files\\dbmspostorig.crs",
		     (LPCSTR)m_installPath);
	MoveFileEx(temp, BakCrsFile, MOVEFILE_REPLACE_EXISTING);

	/*
	** Note that we've done Good Things for this release.
	*/
	cmd.Format("ii.%s.config.dbms.%s defaults", m_computerName,
		   m_completestring);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
    }
    else if (dbms && dbms->m_selected &&
	     SetupString.CompareNoCase("defaults") != 0)
    {
	/* Generate default configuration */
	temp.Format("%d", atoi(GetRegValue("logfilesize"))*1024);
	m_iilogfilesize = temp;

	m_iilogfile = GetRegValue("primarylogdir");
	
	m_enableduallogging = GetRegValueBOOL("enableduallogging", FALSE);
	m_iiduallogfile = GetRegValue("duallogdir");
	
	m_sql92 = GetRegValueBOOL("enablesql92", FALSE);
    }

    if (m_iilogfilesize.IsEmpty())
	m_iilogfilesize = "32768";
	
    if (m_iilogfile.IsEmpty())
	m_iilogfile = m_installPath;

    if (m_iiduallogfile.IsEmpty())
	m_iiduallogfile = m_installPath;

    for (;;)
    {
	/*
	** Create the config.dat file.
	*/
	sConfigDat = m_installPath + "\\ingres\\files\\config.dat";
	File = CreateFile(sConfigDat, GENERIC_WRITE, 0, NULL,
			  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (File == INVALID_HANDLE_VALUE)
	{
	    error = TRUE;
	    break;
	}
	CloseHandle(File);

	cmd.Format( "%s \"%s\\ingres\\files\\", (LPCSTR)m_computerName,
		    (LPCSTR)m_installPath );

	cmd += "default.rfm\"";

	if (Exec(m_installPath + "\\ingres\\utility\\iigenres.exe", cmd, FALSE))
	{
	    error = TRUE;
	    break;
	}

	if (bLogFileExist)
	{
	    /*
	    ** Rename dbms.crs back to what it was.
	    */
	    temp.Format("%s\\ingres\\files\\dbms.crs", (LPCSTR)m_installPath);
	    MoveFileEx(BakCrsFile, temp, MOVEFILE_REPLACE_EXISTING);
	}
	else if (dbms && dbms->m_selected)
	{
	    /* Configure the transaction log file */
	    cmd.Format( "ii.%s.rcp.file.kbytes %d", m_computerName,
			atoi(m_iilogfilesize) );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.rcp.log.log_file_name %s", m_computerName,
			m_logname );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.rcp.log.log_file_1 \"%s\"", m_computerName,
			m_iilogfile );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE); 

	    cmd.Format( "ii.%s.rcp.log.dual_log_name %s", m_computerName,
			m_duallogname );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

#ifdef EVALUATION_RELEASE
	    cmd.Format("ii.%s.dbms.*.cursor_limit 256", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	    cmd.Format( "ii.%s.icesvr.*.default_database fortunoff",
			m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	    cmd.Format( "ii.%s.privileges.user.orspo_runas SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED",
			m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
#endif /* EVALUATION_RELEASE */

	    if (m_enableduallogging)
	    {
		cmd.Format( "ii.%s.rcp.log.dual_log_1 \"%s\"",
			    m_computerName, m_iiduallogfile );
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe",
		     cmd, FALSE); 
	    }
		
	    if (m_sql92)
	    {
		cmd.Format( "ii.%s.fixed_prefs.iso_entry_sql-92 ON",
			    m_computerName );
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe",
		     cmd, FALSE); 
	    }

	    /*
	    ** Note that we've done Good Things for this release.
	    */
	    cmd.Format("ii.%s.config.dbms.%s defaults", m_computerName,
		   m_completestring);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	    /*
	    ** In Ingres 9.3 version date_alias scenario is to be
	    ** set by installer/response file settings.  dbms.crs contains
	    ** INGRESDATE as a default and we no longer want to rely on that
	    ** default.  
	    */
	    if (m_ansidate)	
		cmd.Format("ii.%s.config.date_alias ANSIDATE", m_computerName);
	    else
		cmd.Format("ii.%s.config.date_alias INGRESDATE", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
			

	}

	/*
	** Set up the Administrator, ingres, and current user User rights.
	*/
	cmd.Format( "ii.%s.privileges.user.%s SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED",
		   (LPCSTR)m_computerName, (LPCSTR)m_adminName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	cmd.Format( "ii.%s.privileges.user.%s SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED",
		    (LPCSTR)m_computerName, (LPCSTR)m_userName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	/*
	**Retrieve rmcmd count from config.dat before setting it to 0
	*/
	ConfigKey.Format("ii.%s.ingstart.*.rmcmd", (LPCSTR)m_computerName);
    if (Local_PMget(ConfigKey, ConfigKeyValue))
		m_rmcmdcount = atoi(ConfigKeyValue);
	/*
	** Set rmcmd startup count to 0 since iidbdb doesnt exist or is in
	** the process of being upgraded.
	*/
	cmd.Format("ii.%s.ingstart.*.rmcmd 0", (LPCSTR)m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	/*
	** Set ICE Server startup count to 0 since setup is not yet complete
	** Also, enable 4K pages, as ICE uses row level locking.
	*/
	CComponent *ice = theInstall.GetICE();
	if ((ice) && (ice->m_selected))
        { 
	    cmd.Format( "ii.%s.dbms.private.*.cache.p4k_status ON",
			(LPCSTR)m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE); 
	    cmd.Format("ii.%s.ingstart.*.icesvr 0", (LPCSTR)m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}

	CComponent *star = theInstall.GetStar();
	if ((star) && (star->m_selected))
	{
	    /* Remove obsolete resources */
	    cmd.Format( "ii.%s.star.*.rdf_cluster_nodes",
			(LPCSTR)m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iiremres.exe",cmd,FALSE);
	    cmd.Format( "ii.%s.star.*.rdf_netcosts_rows",
			(LPCSTR)m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iiremres.exe",cmd,FALSE);
	    cmd.Format( "ii.%s.star.*.rdf_nodecosts_rows",
			(LPCSTR)m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iiremres.exe",cmd,FALSE);

	    cmd.Format("ii.%s.ingstart.*.star 0",(LPCSTR) m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}

	if (m_bEmbeddedRelease)
	{ 
	    cmd.Format( "ii.%s.dbms.private.*.cache.p4k_status ON",
			(LPCSTR)m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    
		cmd.Format( "ii.%s.dbms.private.*.cache.p8k_status ON",
			(LPCSTR)m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    
		cmd.Format( "ii.%s.dbms.private.*.cache.p16k_status ON",
			(LPCSTR)m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    
		cmd.Format( "ii.%s.dbms.private.*.cache.p32k_status ON",
			(LPCSTR)m_computerName );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}

	break;
    }

    AppendComment(error ? IDS_FAILED : IDS_DONE);
    return(!error);
}

int
CInstallation::SetupBIConfig()
{
	int bret=0;
	CString cmd;
	AppendToLog(IDS_BI_CONFIG);
	AppendComment(IDS_BI_CONFIG);

	    
	cmd.Format("ii.%s.dbms.*.cache_dynamic ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;
	
	cmd.Format("ii.%s.dbms.*.cursor_limit 32", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.opf_joinop_timeout 100", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("+p ii.%s.dbms.*.opf_memory 50000000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.opf_timeout_factor 1", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;
	
	cmd.Format("ii.%s.dbms.*.blob_etab_page_size 16384", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.max_tuple_length 0", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.connect_limit 64", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.table_auto_structure ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_readlock nolock", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.log.buffer_count 50", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.dmf_group_size 32", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p8k.dmf_group_size 32", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p8k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.cache.p16k_status ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p16k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p32k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_isolation read_committed", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_lock_level row", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_maxlocks 500", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.lock.per_tx_limit 15000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.lock.lock_limit 325000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	if (m_ansidate)
		cmd.Format("ii.%s.config.date_alias ANSIDATE", (LPCSTR)m_computerName);
	else
		cmd.Format("ii.%s.config.date_alias INGRESDATE", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

end:
	AppendComment(bret ? IDS_FAILED : IDS_DONE);
	return bret;

}
int
CInstallation::SetupTXNConfig()
{
	CString cmd;
	int bret=0;
	AppendToLog(IDS_TXN_CONFIG);
	AppendComment(IDS_TXN_CONFIG);

	cmd.Format("ii.%s.dbms.*.opf_joinop_timeout 100", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("+p ii.%s.dbms.*.opf_memory 50000000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.opf_timeout_factor 1", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.blob_etab_page_size 16384", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.cursor_limit 128", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;
	
	cmd.Format("ii.%s.dbms.*.max_tuple_length 0", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.table_auto_structure ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_isolation read_committed", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_lock_level row", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.lock.per_tx_limit 4000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.log.buffer_count 100", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p8k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.cache.p16k_status ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p16k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p32k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_maxlocks 1500", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	if (m_ansidate)
		cmd.Format("ii.%s.config.date_alias ANSIDATE", (LPCSTR)m_computerName);
	else
		cmd.Format("ii.%s.config.date_alias INGRESDATE", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;


end:
	AppendComment(bret ? IDS_FAILED : IDS_DONE);
	return bret;

}
int
CInstallation::SetupCMConfig()
{
	CString cmd;
	int bret=0;

	AppendToLog(IDS_CM_CONFIG);
	AppendComment(IDS_CM_CONFIG);

	cmd.Format("+p ii.%s.dbms.*.opf_memory 23855104", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;


	cmd.Format("ii.%s.dbms.*.blob_etab_page_size 16384", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.cursor_limit 128", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;
	
	if (m_ansidate)
		cmd.Format("ii.%s.config.date_alias ANSIDATE", (LPCSTR)m_computerName);
	else
		cmd.Format("ii.%s.config.date_alias INGRESDATE", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;	
	
	cmd.Format("ii.%s.dbms.*.max_tuple_length 0", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.connect_limit 256", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_lock_level row", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_readlock nolock", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("+p ii.%s.dbms.*.opf_active_limit 51", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.log.buffer_count 200", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p8k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.cache.p16k_status ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p16k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.cache.p32k_status ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.private.*.p32k.dmf_separate ON", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.dbms.*.system_isolation read_committed", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.lock.per_tx_limit 15000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	cmd.Format("ii.%s.rcp.lock.lock_limit 325000", (LPCSTR)m_computerName);
	bret = Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	if (bret) goto end;

	
end:
	AppendComment(bret ? IDS_FAILED : IDS_DONE);
	return bret;
}

void
CInstallation::SetTemp()
{
    char temp_path[MAX_PATH];
    char product_name[MAX_PATH];
    OSVERSIONINFO VersionInfo;
    char *path;

    ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFO));
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    if (GetVersionEx((OSVERSIONINFO *)&VersionInfo) && 
	VersionInfo.dwMajorVersion == 6) //Vista specific case
    {
	path = getenv("ALLUSERSPROFILE");
	if (m_DBATools && strcmp(m_installationcode, "VT")==0)
		sprintf(product_name, "DBA Tools");
	else if (m_DBATools)
		sprintf(product_name, "DBA Tools %s", (LPCSTR)m_installationcode);
	else 
		sprintf(product_name, "Ingres%s", (LPCSTR)m_installationcode);
	if (path != NULL)
        {

    	     m_temp.Format("%s\\Ingres\\%s\\temp", (LPCSTR)path, (LPCSTR)product_name);
	     if (GetFileAttributes(m_temp) == -1)
		CreateDirectoryRecursively(m_temp.GetBuffer(MAX_PATH));
 	}
	else
	{
     	     GetTempPath(sizeof(temp_path), temp_path);
	     m_temp.Format("%s\\Ingres\\%s\\temp", (LPCSTR)temp_path, (LPCSTR)product_name);
	     CreateDirectoryRecursively(m_temp.GetBuffer(MAX_PATH));
	} 
    }
    else
    {
	m_temp.Format("%s\\ingres\\temp", (LPCSTR)m_installPath);
    }
}

BOOL
CInstallation::StartServer(BOOL Comment/*=TRUE*/)
{
    BOOL    error = FALSE;

    AppendToLog(IDS_STARTINGSERVER);
    if(Comment) AppendComment(IDS_STARTINGSERVER);

    /* Switch off the licensing service before starting the server */
    stop_logwatch_service();

    if (m_bClusterInstall)
    {
	if (!OnlineResource())
		error = TRUE;
    }
    else if (m_serviceAuto)
    {
	if (Exec(m_installPath + "\\ingres\\utility\\ingstart.exe", "-service")) 
	    error = TRUE;
    }
    else
    {
	if (Exec(m_installPath + "\\ingres\\utility\\ingstart.exe", 0)) 
	    error = TRUE;
    }

    if(Comment) AppendComment(error ? IDS_FAILED : IDS_DONE);
    return (!error);
}

BOOL
CInstallation::StopServer(BOOL echo)
{
    BOOL error = FALSE;

    AppendToLog(IDS_STOPPINGSERVER);
    AppendComment(IDS_STOPPINGSERVER);

    if (m_bClusterInstall)
    {
	if (!OfflineResource())
	    error = TRUE;
    }
    else
    {
	if (Exec(m_installPath + "\\ingres\\utility\\ingstop.exe", 0, echo)) 
	    error = TRUE;
    }

    /* Restart the licensing service now that we've shut down. */
    start_logwatch_service();
    
    AppendComment(error ? IDS_FAILED : IDS_DONE);
    return (!error);
}

BOOL
CInstallation::CreateLogFile()
{
    BOOL bret = FALSE;

    AppendComment(IDS_CREATELOGFILE);
    AppendToLog(IDS_CREATELOGFILE);
    
    /* Create the directory path to the Primary (and Dual) log file(s).
    ** This must be done here, as if the path includes non-existant
    ** directories, the iimklog will fail.
    */
    if (GetFileAttributes(m_iilogfile)==-1)
        CreateDirectoryRecursively(m_iilogfile.GetBuffer(MAX_PATH));

    bret = Exec(m_installPath + "\\ingres\\bin\\ipcclean.exe", 0);
    bret = Exec(m_installPath + "\\ingres\\utility\\iimklog.exe", 0);
    bret = Exec("rcpconfig.exe", "-force_init", 0);

    if (m_enableduallogging)
    {
        if (GetFileAttributes(m_iiduallogfile)==-1)
            CreateDirectoryRecursively(m_iiduallogfile.GetBuffer(MAX_PATH));

	bret = Exec(m_installPath + "\\ingres\\bin\\ipcclean.exe", 0);
	bret = Exec(m_installPath + "\\ingres\\utility\\iimklog.exe", "-dual", 0);
	bret = Exec("rcpconfig.exe", "-force_init_dual", 0);
    }

    AppendComment(bret ? IDS_FAILED : IDS_DONE);

    return bret;
}

BOOL
CInstallation::UpdateLogFile()
{   
    BOOL    bret = FALSE;
    CString s, cmd;
    CString strOrigName, strNewName;
    CString strLogFileDir, strLogFileName, strLogFile;
 
    if (Local_NMgtIngAt("II_LOG_FILE", strLogFileDir))
    {
	m_iilogfile = strLogFileDir;

	cmd.Format( "ii.%s.rcp.log.log_file_1 \"%s\"", (LPCSTR)m_computerName,
		    strLogFileDir.GetBuffer(255) );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	s = "II_LOG_FILE";
	bret = Exec("ingunset.exe", s);

	if (Local_NMgtIngAt("II_LOG_FILE_NAME", strLogFileName))
	{   
	    cmd.Format( "ii.%s.rcp.log.log_file_name %s", (LPCSTR)m_computerName,
			m_logname );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	    s = "II_LOG_FILE_NAME";
	    bret = Exec("ingunset.exe", s);

	    strOrigName.Format( "%s\\ingres\\log\\%s", strLogFileDir.GetBuffer(255),
				strLogFileName.GetBuffer(255) );
	    strNewName.Format( "%s\\ingres\\log\\%s.l01", strLogFileDir.GetBuffer(255),
				m_logname );
	    if (GetFileAttributes(strOrigName) != -1)
	    {
		MoveFile(strOrigName, strNewName);
		s.Format(IDS_MOVINGLOGFILE, strOrigName, strNewName);
		AppendToLog(s);
	    }
	}
    }

    if (Local_NMgtIngAt("II_DUAL_LOG", strLogFileDir))
    {
	cmd.Format( "ii.%s.rcp.log.dual_log_1 \"%s\"", (LPCSTR)m_computerName,
		    strLogFileDir.GetBuffer(255) );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

    s = "II_DUAL_LOG";
    bret = Exec("ingunset.exe", s);

    if (Local_NMgtIngAt("II_DUAL_LOG_NAME", strLogFileName))
	{
	    cmd.Format( "ii.%s.rcp.log.dual_log_name %s", (LPCSTR)m_computerName,
			m_iiduallogfile );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	    s = "II_DUAL_LOG_NAME";
	    bret = Exec("ingunset.exe", s);
	    
        strOrigName.Format( "%s\\ingres\\log\\%s", strLogFileDir.GetBuffer(255),
				strLogFileName.GetBuffer(255) );
        strNewName.Format( "%s\\ingres\\log\\%s.l01", strLogFileDir.GetBuffer(255),
				m_iiduallogfile );
        if (GetFileAttributes(strOrigName) != -1)
        {
		MoveFile(strOrigName, strNewName);
		s.Format(IDS_MOVINGLOGFILE, strOrigName, strNewName);
		AppendToLog(s);
        }
    }
    } 

    strLogFileName.Format("%s\\ingres\\log\\%s.l01", m_iilogfile, m_logname);
    if (GetFileAttributes(strLogFileName) == -1)
    {
	s.Format(IDS_CANNOTFINDLOGFILE, strLogFileName);
	AppendToLog(s);
	CreateLogFile();
    }
    else
    { 
	AppendToLog(IDS_UPDATELOGFILE);
	AppendComment(IDS_UPDATELOGFILE);

	if (theInstall.m_destroytxlogfile)
	{
	    RemoveFile(strNewName);
	    Exec(m_installPath + "\\ingres\\bin\\ipcclean.exe", 0);
	    Exec(m_installPath + "\\ingres\\utility\\iimklog.exe", 0);
	    Exec("rcpconfig.exe", "-force_init", 0);
	}
	else
	{
	    bret = Exec(m_installPath + "\\ingres\\bin\\ipcclean.exe", 0);
	    bret = Exec(m_installPath + "\\ingres\\utility\\iimklog.exe", "-erase", 0);
	    bret = Exec("rcpconfig.exe", "-force_init", 0);
	}

	if (m_enableduallogging)
	{ 
	    bret = Exec(m_installPath + "\\ingres\\bin\\ipcclean.exe", 0);
	    bret = Exec(m_installPath + "\\ingres\\utility\\iimklog.exe", "-dual -erase", 0);
	    bret = Exec("rcpconfig.exe", "-force_init_dual", 0);
	}
	AppendComment(bret ? IDS_FAILED : IDS_DONE);
    }

    return bret;
}

/*
**  History:
**	07-sep-2001 (penga03)
**	    On Windows 98 and Windows ME, user doesn't have to log on
**	    as 'ingres', so the Ingres service password doesn't have to
**	    be set. 
**	10-sep-2001 (penga03)
**	    On Windows 98 and Windows ME, delete the service specific
**	    registry keys since we don't run InstallIngresService().
**	16-nov-2001 (penga03)
**	    Since we let Windows Installer installs Ingres ODBC driver,
**	    we removed the code here, in post installation, that does the
**	    same work.
**	14-jan-2002 (penga03)
**	    Remove "SilentInstall" from the registry when the post installation 
**	    completes successfully.
**	06-mar-2002 (penga03)
**	    Install CleverPath Portal, Forest & Trees and OpenROAD for evaluation 
**	    evaluation.
**	21-mar-2002 (penga03)
**	    Create an operating system user "icedbuser" for evaluation 
**	    releases.
**	21-mar-2002 (penga03)
**	    For evaluation releases install Apache HTTP Server.
**	01-Apr-2008 (drivi01)
**	    If the install doesn't contain net package we shouldn't try to
**	    add iigcc to the firewall b/c iigcc.exe will not be there and
**	    this results in a failed message in install.log
**	23-Oct-2008 (drivi01)
**	    Reset start count of Net server to 0 only if net server was 
**	    selected for installation or net server start count is greater 
**	    than 1. This is to avoid gcc start count from being set to 
**	    0 when gcc server wasn't selected for installation and 
**	    resulting in Net server misleadingly appearing in IVM and/or 
**	    CBF and VCBF. When Net server count is being set back,
**	    we will check if net server was selected for installation
**	    and if count is supposed to be greater than 0.
**	    The same thing for DAS server, iigcd will only be reset to
** 	    0 at the beginning of post installation if ingstart count
**	    was previously set in config.dat, if it wasn't, it's possible
**	    DAS server isn't being installed (i.e. DBA Tools).  This
**	    will avoid DAS server from appearing in CBF/VCBF/IVM with
**	    count of 0.  Set DAS server count at the end only if the count
**	    is greater than 0.
**	23-Jan-2009 (bonro01)
**	    Restore the Net server startup count correctly after an 
**	    installation is modified. This corrects the bug introduced in
**	    change 494289 for b121120.
**	    
*/
DWORD
CInstallation::ThreadPostInstallation()
{
    CComponent	*dbms = GetDBMS();
    CComponent	*net = GetNet();
    CComponent	*odbc = GetODBC();
    CComponent	*star = GetStar();
    CComponent	*esqlc = GetESQLC();
    CComponent	*esqlcob = GetESQLCOB();
    CComponent	*esqlfortran = GetESQLFORTRAN();
    CComponent	*tools = GetTools();
    CComponent	*vision = GetVision();
    CComponent	*replicat = GetReplicat();
    CComponent	*ice = GetICE();
    CComponent	*openroaddev = GetOpenROADDev();
    CComponent	*openroadrun = GetOpenROADRun();
    CComponent	*jdbc = GetJDBC();
    BOOL	bret = TRUE;
    CString	cmd;

    m_charCount = 0;

    SetTemp();

    /* Remove the old text release notes. */
    RemoveFile(m_installPath + "\\ingres\\IngresRN.txt");

    /* Open log file  */
    CString s = m_installPath + "\\ingres\\files\\install.log";
    m_logFile = CreateFile( s, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			    OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0 );
    if (m_logFile == INVALID_HANDLE_VALUE)
    {
	m_logFile = 0;
	return FALSE;
    }
    else
    {
	SetFilePointer(m_logFile, 0, 0, FILE_END);
	SetDate();
    }

#ifdef EVALUATION_RELEASE
    AddOSUser("icedbuser", "icedbuser", "This user account is used by Ingres SDK");
#endif /* EVALUATION_RELEASE */

    DeleteRegValue("PostInstallationNeeded");

    /* put iigcc in the Windows Firewall exceptions list */
    if (net->m_selected)
    {
    	cmd.Format("firewall add allowedprogram \"%s\\ingres\\bin\\iigcc.exe\" iigcc enable", (LPCSTR)m_installPath);
	Exec("netsh.exe", cmd, TRUE, FALSE);
    	cmd.Format("firewall add allowedprogram \"%s\\ingres\\bin\\iigcd.exe\" iigcc enable", (LPCSTR)m_installPath);
	Exec("netsh.exe", cmd, TRUE, FALSE);
    }

    if (theInstall.m_UseResponseFile)
	WriteRSPContentsToLog();

    if (!m_win95)
	RemoveOldShellLinks();
    
    RemoveObsoleteEnvironmentVariables();
    SetEnvironmentVariables();
    SetODBCEnvironment();

    if (!m_win95)
	WriteUserEnvironmentToLog();

    if (dbms->m_selected || net->m_selected)
	SetFileAssociations();

    if (bret && !SetSymbolTbl())   
	bret = FALSE;

    if (bret && !SetConfigDat())   
	bret = FALSE;

    RegisterActiveXControls();

    if (bret && !m_win95)
	bret = InstallIngresService();

    /* Register cluster. */
    if (bret && m_bClusterInstall)
	bret = RegisterCluster();

#ifdef EVALUATION_RELEASE
    InstallApache();
    CheckPortNumbers();
#endif /* EVALUATION_RELEASE */

    if (!m_IngresPreStartupRunPrg.IsEmpty())
	RunPreStartupPrg();

    if (bret && net->m_selected)	
	bret = NetPostInstallation();

    if (bret && jdbc->m_selected)
	bret = JDBCPostInstallation();

    /*
    ** Set the startup counts for the connectivity servers (NET, 
    ** JDBC, DAS) zero.
    */
    CString	ConfigKey, ConfigKeyValue;
    int /*JdbcCount, */GccCount, GcdCount;

    /*JdbcCount=*/GccCount=GcdCount=0;
	/* Retrieve  Net server startup count and store it.  Reset value
	** to 0 temporarily to avoid it interfering with install.
	*/
    ConfigKey.Format("ii.%s.ingstart.*.gcc", (LPCSTR)m_computerName);
    if (Local_PMget(ConfigKey, ConfigKeyValue))
		GccCount = atoi(ConfigKeyValue);

	if ((net && net->m_selected) || GccCount)
	{
	    cmd.Format("ii.%s.ingstart.*.gcc 0", (LPCSTR)m_computerName);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	}
	/* Retrieve DAS server startup count and store it.  Reset value
	** to 0 temporarily to avoid it interfering with install.
	*/
    ConfigKey.Format("ii.%s.ingstart.*.gcd", (LPCSTR)m_computerName);
    if (Local_PMget(ConfigKey, ConfigKeyValue))
	{
		GcdCount = atoi(ConfigKeyValue);
		cmd.Format("ii.%s.ingstart.*.gcd 0", (LPCSTR)m_computerName);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	}

    if (bret && tools->m_selected)	
	bret = ToolsPostInstallation();

    if (bret && vision->m_selected) 	
	bret = VisionPostInstallation();

    if (bret && dbms->m_selected)	
	bret = ServerPostInstallation();

    if (bret && star->m_selected)	
	bret = StarPostInstallation();

    if (bret && replicat->m_selected)	
	bret = ReplicatPostInstallation();

    if (bret && ice->m_selected)	
	{
	if (!dbms->m_selected)
	{
	    /*
	    ** This means that the DBMS server is already installed.
	    */
	    if(!Local_NMgtIngAt("II_DATABASE", m_iidatabase))
		m_iidatabase = m_installPath;

	    StartServer();

	    bret = CreateDatabases();
	    if (bret)
		bret = LoadICE();
	}

	if (bret)
	    bret = IcePostInstallation();

	if (!dbms->m_selected)
	    StopServer(TRUE);
    }

#ifdef EVALUATION_RELEASE
    if(bret)
    {
	InstallForestAndTrees();
	InstallPortal();
	
	StartApache();
	StartPortal();
	InstallOpenROAD();
	
	OpenPortal();
    }
#endif /* EVALUATION_RELEASE */

    if (bret)
    {
	/*
	** Delete the appropriate registry values.
	*/
	DeleteRegValue("language");
	DeleteRegValue("timezone");
	DeleteRegValue("terminal");
	DeleteRegValue("charset");
	DeleteRegValue("dateformat");
	DeleteRegValue("moneyformat");
	DeleteRegValue("decimal");
	DeleteRegValue("hidewindows");
	DeleteRegValue("leaveingresrunninginstallincomplete");
	DeleteRegValue("startivm");
	DeleteRegValue("ingresprestartuprunprg");
	DeleteRegValue("ingresprestartuprunparams");
	DeleteRegValue("ResponseFileLocation");
	DeleteRegValue("EmbeddedRelease");
	DeleteRegValue("PostInstallationNeeded");
	DeleteRegValue("SilentInstall");
	DeleteRegValue("clusterinstall");
	DeleteRegValue("clusterresource");
	DeleteRegValue("serviceauto");
	DeleteRegValue("serviceloginid");
	DeleteRegValue("servicepassword");
	DeleteRegValue("expressinstall");
	DeleteRegValue("includeinpath");
	DeleteRegValue("installdemodb");
	DeleteRegValue("upgradeuserdatabases");
	DeleteRegValue("ansidate");
	DeleteRegValue("waitforpostinstall");
    }

    DeleteObsoleteFileAssociations();

    if (m_bMDBInstall)
    {
	DeleteRegValue("mdbinstall");
	DeleteRegValue("mdbname");
	DeleteRegValue("mdbsize");
	DeleteRegValue("mdbdebug");
    }

	if (m_DBATools)
		DeleteRegValue("vnode_count");

    if (m_bClusterInstall)
	OfflineResource();
    else
	Exec(m_installPath + "\\ingres\\utility\\ingstop.exe", 0);

    /*
    ** Restore startup counts for the connectivity servers (NET, 
    ** JDBC, DAS).
    */
	if ((net && net->m_selected) || GccCount)
	{
		cmd.Format("ii.%s.ingstart.*.gcc %d", (LPCSTR)m_computerName, GccCount);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	}
	if (GcdCount)
	{
	    cmd.Format("ii.%s.ingstart.*.gcd %d", (LPCSTR)m_computerName, GcdCount);
		Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	}
    /*cmd.Format("ii.%s.ingstart.*.jdbc %d", (LPCSTR)m_computerName, JdbcCount);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	*/
    /*
    ** Restore startup count of rmcmd if it isn't restored yet.
    */
	if (m_rmcmdcount)
	{
    cmd.Format("ii.%s.ingstart.*.rmcmd %d", (LPCSTR)m_computerName, m_rmcmdcount);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE); 
	}

    if (m_LeaveIngresRunningInstallIncomplete)
	StartServer();
	
    if (bret && m_DBATools && m_vnodeCount>0)
	bret = AddVirtualNodes();

    /* Report success or failure */
    AppendToLog(IDS_SEPARATOR);

    time_t t; time(&t);
    m_endTime =t;
    if (bret)
    {
	CString s(m_endTime.Format(IDS_DATEFORMATS));
	AppendToLog(s);
	AppendComment(IDS_SUCCEEDED2);
	//s.Format("RC=0");
	//AppendToLog(s);
    }
    else
    {
	CString s(m_endTime.Format(IDS_DATEFORMATF));
	AppendToLog(s);
	//s.Format("RC=1");
	//AppendToLog(s);
    }

    CloseHandle(m_logFile);
    m_logFile = 0;

    return bret;
}

static void
RemoveOneService(LPCSTR lpServiceName)
{
    SC_HANDLE		schSCManager, schService;
    
    schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    
    if (schSCManager)
    {
	schService=OpenService(schSCManager, lpServiceName,
	    SERVICE_ALL_ACCESS);
	if (schService)
	{
	    SERVICE_STATUS status;
	    ControlService(schService,SERVICE_CONTROL_STOP,&status);
	    DeleteService(schService);
	    CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
    }
}

int
RemoveFolder(char *szFolder, BOOL bRecursive)
{
    char	    szDirectory[MAX_PATH];
    char	    szFile[MAX_PATH];
    SHFILEOPSTRUCT  fos;
    WIN32_FIND_DATA FindData;
    HANDLE	    hFind;
    BOOL	    bFindFile = TRUE;
    int		    rc;
    DWORD	    dwAttributes;

    sprintf(szDirectory, "%s\\*.*", szFolder);

    ZeroMemory(&fos, sizeof(fos));
    fos.hwnd = NULL; 
    fos.wFunc = FO_DELETE;
    fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;   /* Send to the recycle bin */

    hFind = FindFirstFile(szDirectory, &FindData);

    while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
    {
	if (*(FindData.cFileName) != '.')
	{
	    sprintf(szFile, "%s\\%s", szFolder, FindData.cFileName);

	    /* add a second NULL because SHFileOperation is looking for this */
	    szFile[strlen(szFile)+1] = '\0';
	    dwAttributes = GetFileAttributes(szFile);

	    /* if it is a folder, Recursively delete it */
	    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
		if(bRecursive)
		    RemoveFolder(szFile, TRUE);
	    }
	    else
	    {
		fos.pFrom = szFile;	/* delete the file */
		SHFileOperation(&fos);
	    }
	}
    
	bFindFile = FindNextFile(hFind, &FindData);
    }

    FindClose(hFind);   

    rc = RemoveDirectory(szFolder);

    if (rc)
	SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szFolder, 0);

    return rc;
}

BOOL
RemoveStartMenuFolder(char *szFolder)
{
    char	    szBuffer[512], szPath[MAX_PATH];
    LPITEMIDLIST    pidlStartMenu;

    CoInitialize(NULL);

    SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlStartMenu);

    /* get the path for the folder */
    SHGetPathFromIDList(pidlStartMenu, szPath);

    sprintf(szBuffer, "%s\\%s", szPath, szFolder);
    RemoveFolder(szBuffer, TRUE);

    CoUninitialize();

    return (0);
}

HDDEDATA EXPENTRY
DdeCallBack(UINT u1,UINT u2,HCONV h1,HSZ h2,HSZ h3,HDDEDATA d,DWORD dw1,DWORD dwz2)
{
    return (HDDEDATA)0;
}

void 
RemoveOpenIngresFolders()
{
    LPITEMIDLIST pidlStartMenu;
    char szPath[MAX_PATH], szFolder[MAX_PATH];
    CString s;

    CoInitialize(NULL);  

    if (theInstall.m_win95)
	SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlStartMenu);
    else
	SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlStartMenu);
    SHGetPathFromIDList(pidlStartMenu, szPath);

    s.LoadString(IDS_DELETE20DATABASEFOLDER);
    sprintf(szFolder, "%s\\%s", szPath, s);
    theInstall.RemoveOneDirectory(szFolder);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

    s.LoadString(IDS_DELETE20NETWORKINGFOLDER);
    sprintf(szFolder, "%s\\%s", szPath, s);
    theInstall.RemoveOneDirectory(szFolder);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

    s.LoadString(IDS_DELETE20TOOLSFOLDER);
    sprintf(szFolder, "%s\\%s", szPath, s);
    theInstall.RemoveOneDirectory(szFolder);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

    s.LoadString(IDS_DELETE12DATABASEFOLDER);
    sprintf(szFolder, "%s\\%s", szPath, s);
    theInstall.RemoveOneDirectory(szFolder);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

    s.LoadString(IDS_DELETE12NETWORKINGFOLDER);
    sprintf(szFolder, "%s\\%s", szPath, s);
    theInstall.RemoveOneDirectory(szFolder);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

    s.LoadString(IDS_DELETE12TOOLSFOLDER);
    sprintf(szFolder, "%s\\%s", szPath, s);
    theInstall.RemoveOneDirectory(szFolder);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szFolder, 0);

    CoUninitialize();

    RemoveStartMenuFolder("CA-OpenROAD");
    RemoveStartMenuFolder("IngresII Workgroup Edition");
}

void 
RemoveStartMenuItem(char *szItemName)
{
    int rc;
    LPITEMIDLIST   pidlStartMenu;
    char szPath[MAX_PATH], szShortcut[MAX_PATH];
    
    CoInitialize(NULL);  
    
    // get the pidl for the start menu - this will be used to intialize the folder browser
    SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlStartMenu);
    
    // get the path for the folder
    SHGetPathFromIDList(pidlStartMenu, szPath);
    
    sprintf(szShortcut, "%s\\Ingres II\\%s.lnk", szPath, szItemName);
    
    rc = DeleteFile(szShortcut);
    
    if (rc)
	SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, szShortcut, 0);
    
    CoUninitialize();
    
}

BOOL
RemoveOldEnvVariable(char *pValue)
{
    int	    rc;
    HKEY    hKey;

    rc = RegOpenKeyEx(HKEY_CURRENT_USER,
		      "Environment",
		      (DWORD)0,
		      KEY_ALL_ACCESS,
		      &hKey);

    if (rc == ERROR_SUCCESS)
	rc = RegDeleteValue(hKey, pValue);

    if (hKey)
	RegCloseKey(hKey);

    return rc;
}

BOOL
RemoveOldSystemEnvVariable(char *pValue)
{
    int		rc;
    HKEY	hKey;

    rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
	    "System\\CurrentControlSet\\Control\\Session Manager\\Environment",
	    (DWORD)0,
	    KEY_ALL_ACCESS,
	    &hKey);

    if (rc == ERROR_SUCCESS)
	rc = RegDeleteValue(hKey, pValue);

    if (hKey)
	RegCloseKey(hKey);

    ::SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)"Environment");

    return rc;
}

BOOL
RemoveObsoleteRegEntries()
{
    int	    rc;
    HKEY    hKey;

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Computer Associates International Inc.",
		      0,
		      KEY_ALL_ACCESS,
		      &hKey);

    if (rc == ERROR_SUCCESS)
	rc = RegDeleteKey(hKey, "OpenIngres");

    if (hKey)
	RegCloseKey(hKey);

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      "SOFTWARE",
		      0,
		      KEY_ALL_ACCESS,
		      &hKey);

    if (rc == ERROR_SUCCESS)
	rc = RegDeleteKey(hKey, "Computer Associates International Inc.");

    if (hKey)
	RegCloseKey(hKey);

    return 0;
}

/*
**	History:
**	23-July-2001 (penga03)
**	    If this is an upgrade, get the directories of database files 
**	    from symbol.tbl.
**	10-Aug-2001 (penga03)
**	    Turn on the startup count at the beginning of server post installation 
**	    process.
**	20-Feb-2007 (drivi01)
**	    Remove prompt to upgrade user databases.  User is asked this 
**	    question in pre-installer now and doesn't need to be asked 
**	    the same question twice.
*/
BOOL
CInstallation::ServerPostInstallation()
{
    BOOL	bret = TRUE;
    CString	strBuffer;
    CComponent	*dbms = GetDBMS();
    CString cmd;
    BOOL silent;

    silent = GetRegValueBOOL("SilentInstall", FALSE);

    cmd.Format("ii.%s.ingstart.*.dbms 1", m_computerName);
    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, TRUE);

    /* If this is a upgrade, grab those values from symbol.tbl first. */
    if (!Local_NMgtIngAt("II_DATABASE", m_iidatabase))
	m_iidatabase = GetRegValue("iidatabasedir");
    if (m_iidatabase.IsEmpty())
	m_iidatabase = m_installPath;

	if (!Local_NMgtIngAt("II_CHECKPOINT", m_iicheckpoint))
	m_iicheckpoint = GetRegValue("iicheckpointdir");
    if (m_iicheckpoint.IsEmpty())
	m_iicheckpoint = m_installPath;

    if (!Local_NMgtIngAt("II_JOURNAL", m_iijournal))
	m_iijournal = GetRegValue("iijournaldir");
    if (m_iijournal.IsEmpty())
	m_iijournal = m_installPath;

    if (!Local_NMgtIngAt("II_DUMP", m_iidump))
	m_iidump = GetRegValue("iidumpdir");
    if (m_iidump.IsEmpty())
	m_iidump = m_installPath;

    if (!Local_NMgtIngAt("II_WORK", m_iiwork))
	m_iiwork = GetRegValue("iiworkdir");
    if (m_iiwork.IsEmpty())
	m_iiwork = m_installPath;
	
    m_destroytxlogfile = GetRegValueBOOL("destroytxlogfile", FALSE);
    m_upgradedatabases = GetRegValueBOOL("upgradeuserdatabases", FALSE);

    m_bMDBInstall = GetRegValueBOOL("mdbinstall", FALSE);
    m_MDBName = GetRegValue("mdbname");
    m_MDBSize = GetRegValue("mdbsize");
    m_bMDBDebug = GetRegValueBOOL("mdbdebug", FALSE);

    RemoveOneService("CA-OpenIngres_Database");
    RemoveOneService("OpenIngres_Database");

    RemoveOpenIngresFolders();

    RemoveStartMenuItem("Stop Ingres");
    RemoveStartMenuItem("Start Ingres");

    RemoveObsoleteRegEntries();

    /* Remove These Obsolete Env Vars From Old Replicator */
    RemoveOldEnvVariable("DD_RSERVERS");
    RemoveOldEnvVariable("DD_INGREP");
    RemoveOldEnvVariable("DD_TMP");
    RemoveOldEnvVariable("DD_REPBIN");

    /*
    ** Remove old II_TEMPORARY variable now that this is set in the
    ** symbol table.
    */
    RemoveOldSystemEnvVariable("II_TEMPORARY");

    /* Notify System Of Registry Changes */
    ::SendMessage(::FindWindow("ProgMan", NULL), WM_WININICHANGE, 0L, (LPARAM)"Environment");  

    SetServerSymbolTbl();

    CreateServerDirectories();

    if (!m_DBMSupgrade)
		CreateLogFile();
    else
        UpdateLogFile();

    if (bret && !StartServer())
	bret = FALSE;

    if (bret && !CreateDatabases())
	bret = FALSE;

    if (bret && !LoadIMA())
	bret = FALSE;

	if (bret && DatabaseExists("demodb") && !LoadDemodb())
	bret = FALSE;

    CComponent *ice = theInstall.GetICE();
    if ((ice) && (ice->m_selected))
    {
	if (bret && !LoadICE())
	    bret = FALSE;
    }

    if (m_DBMSupgrade)
    { 
	if (m_bClusterInstallEx)
	{
	    UpgradeDatabases();
	}
	else
	{
	    if (m_upgradedatabases)
		UpgradeDatabases();
	}
	m_attemptedUDBupgrade = TRUE;
    }

	StopServer(TRUE);

    if (bret)
    {

		/*
		** New feature is added for new installs which allows user to choose
		** configuration type. A new dialog is added in pre-installer with 4 
		** configurations.
		** 0 - Transactional System
		** 1 - Traditional Ingres System
		** 2 - Business Intelligence System
		** 3 - Content Management System
		**
		** Execute this for new installations only.  Upgrades should preserve
		** original settings.
		*/
		if (!m_DBMSupgrade)
		{
		if (m_configType == 0)
			SetupTXNConfig();
		else if (m_configType == 2)
			SetupBIConfig();
		else if (m_configType == 3)
			SetupCMConfig();
		}
	
	strBuffer.Format("ii.%s.config.dbms.%s COMPLETE", m_computerName,
			 m_completestring);
	Exec(m_installPath+"\\ingres\\utility\\iisetres.exe", strBuffer, FALSE);

	/*
	** Delete the appropriate registry values.
	*/
	DeleteRegValue("logfilesize");
	DeleteRegValue("enableduallogging");
	DeleteRegValue("enablesql92");
	DeleteRegValue("iidatabasedir");
	DeleteRegValue("iicheckpointdir");
	DeleteRegValue("iijournaldir");
	DeleteRegValue("iidumpdir");
	DeleteRegValue("iiworkdir");
	DeleteRegValue("primarylogdir");
	DeleteRegValue("duallogdir");
	DeleteRegValue("destroytxlogfile");
	DeleteRegValue(dbms->m_name);
	DeleteRegValue("ingconfigtype");
    }

    return bret;
}

/*
**	07-Aug-2001 (penga03)
**	    Turn the protocol off if the corresponding registry entry 
**	    is set to FALSE.
**	20-Nov-2008 (drivi01)
**	    Update wintcp settings to be always OFF, wintcp isn't 
**	    recommended for Windows installations anymore.
**	    Add routine to convert wintcp to tcp_ip for existing
**	    installations.
*/
BOOL
CInstallation::NetPostInstallation()
{
    BOOL	bret=TRUE;
    CString	cmd, strBuffer;
    CComponent	*net = GetNet();
    BOOL bTCPIP, bWINTCP, bNETBIOS, bSPX;
    CString host=m_computerName.MakeLower();

    m_enableTCPIP = GetRegValueBOOL("enableTCPIP", TRUE);
    m_enableNETBIOS = GetRegValueBOOL("enableNETBIOS", FALSE);
    m_enableSPX = GetRegValueBOOL("enableSPX", FALSE);
    m_enableDECNET = GetRegValueBOOL("enableDECNET", FALSE);
    if (!m_enableTCPIP && !m_enableNETBIOS && !m_enableSPX && !m_enableDECNET)
	m_enableTCPIP = TRUE;

    RemoveOpenIngresFolders();
    /*
    ** Remove old II_TEMPORARY variable now that this is set in the
    ** symbol table.
    */
    RemoveOldSystemEnvVariable("II_TEMPORARY");

    if (m_enableTCPIP)
    {
	bTCPIP = bWINTCP = 0;

	cmd.Format("ii.%s.gcc.*.tcp_ip.status", host);
	if (Local_PMget(cmd, strBuffer) && !_stricmp(strBuffer, "ON"))
	{
	    bTCPIP = 1;
	}

	cmd.Format("ii.%s.gcc.*.wintcp.status", host);
	if (Local_PMget(cmd, strBuffer) && !_stricmp(strBuffer, "ON"))
	{
	    bWINTCP = 1;
	}

	if (!bTCPIP && !bWINTCP)
	{
	    cmd.Format("ii.%s.gcc.*.wintcp.status OFF", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format("ii.%s.gcb.*.wintcp.status OFF", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcc.*.wintcp.port %s", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcb.*.wintcp.port %s", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	    cmd.Format("ii.%s.gcc.*.tcp_ip.status ON", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format("ii.%s.gcb.*.tcp_ip.status ON", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcc.*.tcp_ip.port %s1", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcb.*.tcp_ip.port %s1", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}
    }
    else
    {
	cmd.Format("ii.%s.gcc.*.wintcp.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format("ii.%s.gcb.*.wintcp.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcc.*.wintcp.port %s", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcb.*.wintcp.port %s", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	cmd.Format("ii.%s.gcc.*.tcp_ip.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format("ii.%s.gcb.*.tcp_ip.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcc.*.tcp_ip.port %s1", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcb.*.tcp_ip.port %s1", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
    }

    if (m_enableNETBIOS)
    {
	bNETBIOS = 0;

	cmd.Format("ii.%s.gcc.*.lanman.status", host);
	if (Local_PMget(cmd, strBuffer) && !_stricmp(strBuffer, "ON"))
	{
	    bNETBIOS = 1;
	}

	if (!bNETBIOS)
	{
	    cmd.Format("ii.%s.gcc.*.lanman.status ON", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format("ii.%s.gcb.*.lanman.status ON", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcc.*.lanman.port %s", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcb.*.lanman.port %s", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}
    }
    else
    {
	cmd.Format("ii.%s.gcc.*.lanman.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format("ii.%s.gcb.*.lanman.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcc.*.lanman.port %s", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcb.*.lanman.port %s", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
    }

    if (m_enableSPX)
    {
	bSPX = 0;

	cmd.Format("ii.%s.gcc.*.nvlspx.status", host);
	if (Local_PMget(cmd, strBuffer) && !_stricmp(strBuffer, "ON"))
	{
	    bSPX = 1;
	}

	if (!bSPX)
	{
	    cmd.Format("ii.%s.gcc.*.nvlspx.status ON", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format("ii.%s.gcb.*.nvlspx.status ON", m_computerName);
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcc.*.nvlspx.port %s", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	    cmd.Format( "ii.%s.gcb.*.nvlspx.port %s", m_computerName, m_installationcode );
	    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	}
    }
    else
    {
	cmd.Format("ii.%s.gcc.*.nvlspx.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format("ii.%s.gcb.*.nvlspx.status OFF", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcc.*.nvlspx.port %s", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
	cmd.Format( "ii.%s.gcb.*.nvlspx.port %s", m_computerName, m_installationcode );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);
    }

    if (bret)
    {
	cmd.Format("-action convert_wintcp");
	Exec(m_installPath + "\\ingres\\bin\\iicvtwintcp.exe", cmd, FALSE);

	cmd.Format( "ii.%s.config.net.%s COMPLETE", m_computerName,
		    m_completestring );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	/*
	** Delete the appropriate registry values.
	*/
 	DeleteRegValue("enableTCPIP");
	DeleteRegValue("enableNETBIOS");
	DeleteRegValue("enableSPX");
	DeleteRegValue("enableDECNET");
	DeleteRegValue(net->m_name);
    }

    return bret;
}

BOOL
CInstallation::StarPostInstallation()
{
    BOOL	bret = TRUE;
    CString	cmd;
    CComponent	*star = GetStar();

    /* Nothing to do for now! */
    if (bret)
    {
	cmd.Format( "ii.%s.config.star.%s COMPLETE", m_computerName,
		    m_completestring );
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE);

	cmd.Format("ii.%s.ingstart.*.star 1",(LPCSTR) m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe",cmd,FALSE);

	DeleteRegValue(star->m_name);
    }
    return bret;
}

BOOL
CInstallation::JDBCPostInstallation()
{
    BOOL	bret = TRUE;
    CString	cmd;
    CComponent	*jdbc = GetJDBC();
    CString	ConfigKey, ConfigKeyValue;
    int JdbcCount=0;

    cmd.Format("ii.%s.config.gcd.%s COMPLETE", m_computerName, m_completestring);
    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);

    Exec(m_installPath + "\\ingres\\bin\\iijdbcprop.exe", "-c");

    ConfigKey.Format("ii.%s.ingstart.*.jdbc", m_computerName);
    if (Local_PMget(ConfigKey, ConfigKeyValue))
	JdbcCount = atoi(ConfigKeyValue);
    if (JdbcCount >= 1)
    {
	cmd.Format("ii.%s", m_computerName);
	Exec(m_installPath + "\\ingres\\utility\\iicpydas.exe", cmd);
	
	cmd.Format("ii.%s.ingstart.*.jdbc %d", m_computerName, 0);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	
	cmd.Format("ii.%s.ingstart.*.gcd %d", m_computerName, JdbcCount);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
	
	ConfigKey.Format("ii.%s.jdbc.*.port", m_computerName);
	if (Local_PMget(ConfigKey, ConfigKeyValue))
	    cmd.Format("ii.%s.das.*.port %s", m_computerName, ConfigKeyValue);
	else
	    cmd.Format("ii.%s.das.*.port %s7", m_computerName, m_installationcode);
	Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd);
    }
	
    DeleteRegValue(jdbc->m_name);
    return bret;
}

BOOL
CInstallation::ToolsPostInstallation()
{
    BOOL bret = TRUE;

    /* Nothing to do for now! */
    return bret;
}

BOOL
CInstallation::VisionPostInstallation()
{
    BOOL    bret = TRUE;
    CString s;
    CComponent	*vision = GetVision();

    AppendToLog(IDS_ABF_SETUP);

    if (!m_DBMSupgrade)
    {
	s.Format("ING_ABFDIR \"%s\\ingres\\abf\"", (LPCSTR)m_installPath);
	Exec("ingsetenv.exe", s);

	AppendToLog(IDS_CREATING_ING_ABFDIR);
	s.Format("%s\\ingres\\abf", (LPCSTR)m_installPath);
	if (GetFileAttributes(s) == -1)
	    bret = CreateDirectory(s, NULL);
    }

    if (bret == TRUE)
    {
	AppendToLog(IDS_ABF_SETUP_COMPLETE);
	DeleteRegValue(vision->m_name);
    }
    else
	AppendToLog(IDS_ABF_SETUP_FAILED);

    return bret;
}

BOOL
CInstallation::ReplicatPostInstallation()
{
    BOOL	bret = TRUE;
    CComponent	*replicat = GetReplicat();

    CreateRepDirectories();

    DeleteRegValue(replicat->m_name);

    return bret;
}

BOOL
CInstallation::IcePostInstallation()
{
    BOOL	bret = TRUE;
    CString	s;
    CComponent	*ice = GetICE();

    if (GetFileAttributes(m_HTTP_ServerPath)==-1)
	CreateDirectoryRecursively(m_HTTP_ServerPath.GetBuffer(MAX_PATH));

    s.Format("-u -c\"%s\" \"%s\\ingres\\ice\\icetutor\\icetutor.caz\"", 
	     m_HTTP_ServerPath, m_installPath);

    if (Exec(m_installPath + "\\ingres\\bin\\cazipxp.exe", s)) 
	bret = FALSE;

    if (m_bInstallColdFusion)
    {
	CString	strParam, strTemp;
	
	strTemp = theInstall.m_ColdFusionPath + "\\Extensions";
	if (GetFileAttributes(strTemp)==-1)
		CreateDirectoryRecursively(strTemp.GetBuffer(MAX_PATH));

	s.Format("-u -c\"%s\\Extensions\" \"%s\\ingres\\ice\\coldfusion\\ingrescf.caz\"", 
		 m_ColdFusionPath, m_installPath);

	if (Exec(m_installPath + "\\ingres\\bin\\cazipxp.exe", s)) 
	    bret = FALSE;

	strParam.Format("\"%s\"", m_ColdFusionPath);
	s.Format("%s\\ingres\\bin\\instcf.exe", m_installPath);
	Exec(s, strParam);
    }

    char theFile[MAX_PATH], tmpFile[MAX_PATH];
#ifdef EVALUATION_RELEASE
    /* modify ice.conf */
    sprintf(theFile, "%s\\ingres\\ice\\bin\\apache\\ice.conf", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\ice\\bin\\apache\\ice.tmp", m_installPath);

    s.Format("%s", m_installPath); s.Replace("\\", "/");
    ReplaceString("[II_SYSTEM]", s, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    /* 
    ** modify [HOSTNAME] and [PORTAL_PORT] in 
    ** %II_SYSTEM%\ingres\ice\HTTPArea\AISDK\redirect\login.html
    */
    CString PortNum;
    char HostName[MAX_COMP_LEN];
    sprintf(theFile, "%s\\ingres\\ice\\HTTPArea\\AISDK\\redirect\\login.html", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\ice\\HTTPArea\\AISDK\\redirect\\login.tmp", m_installPath);

    if(GetHostName(HostName))
	ReplaceString("[HOSTNAME]", HostName, theFile, tmpFile);
    else
	ReplaceString("[HOSTNAME]", m_computerName, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    PortNum.Format("%d", m_PortalHostPort);
    ReplaceString("[PORTAL_PORT]", PortNum, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);
#else
    if (!m_HTTP_Server.CompareNoCase("Apache"))
    {
	/* Parse [ICE_PORT], [II_SYSTEM] and [II_DOCUMENTROOT] in ice.conf. */
    sprintf(theFile, "%s\\ingres\\ice\\bin\\apache\\ice.conf", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\ice\\bin\\apache\\ice.tmp", m_installPath);
	
	CheckSocket(&m_IcePortNumber);
	s.Format("Listen %d", m_IcePortNumber);
	ReplaceString("Listen [ICE_PORT]", s, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);
	
	s.Format("<VirtualHost *:%d>", m_IcePortNumber);
	ReplaceString("<VirtualHost *:[ICE_PORT]>", s, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	s.Format("%s", m_installPath); s.Replace("\\", "/");
	ReplaceString("[II_SYSTEM]", s, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);

	s.Format("%s", m_HTTP_ServerPath); s.Replace("\\", "/");
	ReplaceString("[II_DOCUMENTROOT]", s, theFile, tmpFile);
	CopyFile(tmpFile, theFile, FALSE );
	DeleteFile(tmpFile);
    }
#endif /* EVALUATION_RELEASE */

    if (bret)
    {
	DeleteRegValue("iihttpserver");
	DeleteRegValue("iihttpserverdir");
	DeleteRegValue("iicoldfusiondir");
	DeleteRegValue(ice->m_name);
    }

    return (bret);
}

BOOL
CInstallation::OpenROADPostInstallation()
{
    BOOL	error = FALSE;
    CString	s, strIconFile;
    CComponent	*openroaddev = GetOpenROADDev();
    CComponent	*openroadrun = GetOpenROADRun();

    m_use_256_colors = GetRegValue("openroad256colors");

    s.Format("II_W4GLAPPS_DIR \"%s\\ingres\\w4glapps\"",
	     (LPCSTR)m_installPath);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_FONT_FILE \"%s\\ingres\\files\\appedtt.ff\"",
	     (LPCSTR)m_installPath);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_COLORTABLE_FILE \"%s\\ingres\\files\\apped.ctb\"",
	     (LPCSTR)m_installPath);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("II_W4GL_USE_256_COLORS %s", (LPCSTR)m_use_256_colors);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    s.Format("PICTURE_DIR \"%s\\ingres\\files\\w4gldemo\"",
	     (LPCSTR)m_installPath);
    if (Exec("ingsetenv.exe", s)) 
	error = TRUE;

    if (!error)
    {
	DeleteRegValue("openroad256colors");
	DeleteRegValue(openroaddev->m_name);
	DeleteRegValue(openroadrun->m_name);
    }

    return (!error);
}

/*
**	History:
**	27-Oct-2006 (drivi01)
**	    When demonstration database is selected in the installer,
**	    this function will load data.
**	22-Feb-2007 (drivi01)
**	    Add a call to iisudemo.bat, if C# Frequent Flyer Demo is installed
**	    update instance id in the configuration file.
*/
BOOL 
CInstallation::LoadDemodb()
{
    BOOL		bret = TRUE;
    char		buf[1024], buf2[2048];				   
    char		in_buf[1024];
	char		CurDir[MAX_PATH+1], NewDir[MAX_PATH+1];
    CString		cmd;
    HANDLE		newstdin, SaveStdin, hFile;
    PROCESS_INFORMATION	pi;
    SECURITY_ATTRIBUTES handle_security;
    STARTUPINFO 	si;
	WIN32_FIND_DATA	wfd;
	char *scripts[] = {"drop.sql", "copy.in", '\0'};
	
    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;
    memset((char*)&pi , 0, sizeof(pi));
    memset((char*)&si, 0, sizeof(si));
    si.cb = sizeof(si);

    AppendComment(IDS_LOADDEMODB);
    AppendToLog  (IDS_LOADDEMODB);

	/*
	** If c# demo was installed, this portion will update xml file with installation id
	*/
	sprintf(buf, "%s\\ingres\\demo\\csharp\\travel\\app\\*.xml", m_installPath);
	if ((hFile = FindFirstFile(buf, &wfd)) != INVALID_HANDLE_VALUE)
	{
		Exec(m_installPath + "\\ingres\\utility\\iisudemo.bat", 0);
		FindClose(hFile);
	}

	GetCurrentDirectory(sizeof(CurDir), CurDir);
	sprintf(NewDir, "%s\\ingres\\demo\\data", m_installPath);
	SetCurrentDirectory(NewDir);
    for (int i = 0; scripts[i]!=NULL; i++)
    {	
    sprintf(in_buf, "%s\\ingres\\demo\\data\\%s", m_installPath, scripts[i]);

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
		    GetCurrentProcess(), &SaveStdin, 0, FALSE,
		    DUPLICATE_SAME_ACCESS);
    
    newstdin = CreateFile(in_buf, GENERIC_READ, FILE_SHARE_READ,
			  &handle_security, OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL, NULL);

    if ((newstdin == INVALID_HANDLE_VALUE) ||
	(SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE))
    {
	bret = FALSE;
    }

    si.dwFlags |= STARTF_USESTDHANDLES;   
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    sprintf(buf, "\"%s\\ingres\\bin\\tm.exe\" -qSQL demodb",
	    m_installPath);

    sprintf(buf2, "%s < %s", buf, in_buf);
    AppendToLog(buf2);
    if (CreateProcess(NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS, NULL,
			    NULL, &si, &pi))
    {
	DWORD	dw;

	WaitForSingleObject(pi.hProcess,INFINITE);
	if (GetExitCodeProcess(pi.hProcess, &dw))
	    bret = TRUE;
    }
    else
		bret = FALSE;

    SetStdHandle(STD_INPUT_HANDLE, SaveStdin);
    CloseHandle(newstdin);
    Sleep (1500);

	if (i == 0)
		if(!CheckpointOneDatabase("-j demodb"))
		{
			bret = FALSE;
			break;
		}
    }
	if (!CheckpointOneDatabase("+j demodb -l +w"))
    bret = FALSE;

	if (bret)
		AppendComment(IDS_DONE);
	else
		AppendComment(IDS_FAILED);

	SetCurrentDirectory(CurDir);

    return bret;
}

/*
**	History:
**	26-mar-2002 (penga03)
**	    For evaluation releases, reload database imadb.
*/
BOOL
CInstallation::LoadIMA()
{
    BOOL		bret = TRUE;
    char		buf[1024];				   
    char		in_buf[1024];
    CString		cmd;
    HANDLE		newstdin, SaveStdin;
    PROCESS_INFORMATION	pi;
    SECURITY_ATTRIBUTES handle_security;
    STARTUPINFO 	si;

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;
    memset((char*)&pi , 0, sizeof(pi));
    memset((char*)&si, 0, sizeof(si));
    si.cb = sizeof(si);

    AppendComment(IDS_LOADIMA);
    AppendToLog  (IDS_LOADIMA);

    sprintf(in_buf, "%s\\ingres\\bin\\makiman.sql", m_installPath);

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
		    GetCurrentProcess(), &SaveStdin, 0, FALSE,
		    DUPLICATE_SAME_ACCESS);
    
    newstdin = CreateFile(in_buf, GENERIC_READ, FILE_SHARE_READ,
			  &handle_security, OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL, NULL);

    if ((newstdin == INVALID_HANDLE_VALUE) ||
	(SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE))
    {
	bret = FALSE;
    }

    si.dwFlags |= STARTF_USESTDHANDLES;   
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    sprintf(buf, "\"%s\\ingres\\bin\\tm.exe\" -qSQL -u$ingres imadb",
	    m_installPath);

	/*
    if (CreateProcessAsUser(m_phToken, NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS, NULL,
			    NULL, &si, &pi))
	*/
    if (CreateProcess(NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS, NULL,
			    NULL, &si, &pi))
    {
	DWORD	dw;

	WaitForSingleObject(pi.hProcess,INFINITE);
	if (GetExitCodeProcess(pi.hProcess, &dw))
	    bret = TRUE;
    }
    else
	bret = FALSE;

    if (m_DBMSupgrade)    
    {
	Exec(m_installPath + "\\ingres\\utility\\rmcmdrmv.exe", "-diidbdb");
	Exec(m_installPath + "\\ingres\\utility\\rmcmdrmv.exe", "-uingres");
	Exec(m_installPath + "\\ingres\\utility\\rmcmdrmv.exe", 0);
    }

    Exec(m_installPath + "\\ingres\\utility\\rmcmdgen.exe", 0);

	if (m_rmcmdcount)
	{
    cmd.Format("ii.%s.ingstart.*.rmcmd %d", (LPCSTR)m_computerName, m_rmcmdcount);
    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE); 
	}

    SetStdHandle(STD_INPUT_HANDLE, SaveStdin);
    CloseHandle(newstdin);

#ifdef EVALUATION_RELEASE
    char CurDir[MAX_PATH], NewDir[MAX_PATH];
    /* populate imadb */
    GetCurrentDirectory(sizeof(CurDir), CurDir);
    sprintf(NewDir, "%s\\DBupdate\\imadb", m_InstallSource);
    SetCurrentDirectory(NewDir);
    Exec("reload.bat", 0, TRUE, FALSE);
    SetCurrentDirectory(CurDir);
#endif  /* EVALUATION_RELEASE */


	if (bret)
		AppendComment(IDS_DONE);
	else
		AppendComment(IDS_FAILED);

    return bret;
}

/*
**	History:
**	26-mar-2002 (penga03)
**	    For evaluation releases, reload database icesvr.
*/
BOOL
CInstallation::LoadICE()
{
    BOOL		bret = TRUE;
    char		buf[1024];				   
    char		in_buf[1024];		
    HANDLE		newstdin, SaveStdin;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES handle_security;
    STARTUPINFO 	si;
    CString		cmd;

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;
    memset((char*)&pi, 0, sizeof(pi));
    memset((char*)&si, 0, sizeof(si));
    si.cb = sizeof(si);

    AppendComment(IDS_LOADICE);
    AppendToLog(IDS_LOADICE);

    if (m_sql92)
    {
	sprintf(in_buf, "%s\\ingres\\ice\\scripts\\icinit92.sql",
		m_installPath);
    }
    else
    {
	sprintf(in_buf, "%s\\ingres\\ice\\scripts\\iceinit.sql",
		m_installPath);
    }

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
		    GetCurrentProcess(), &SaveStdin, 0, FALSE,
		    DUPLICATE_SAME_ACCESS);

    newstdin = CreateFile(in_buf, GENERIC_READ, FILE_SHARE_READ,
			  &handle_security, OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL, NULL);

    if ((newstdin == INVALID_HANDLE_VALUE) ||
	(SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE))
    {
	bret = FALSE;
    }

    si.dwFlags |= STARTF_USESTDHANDLES;   
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    sprintf(buf, "\"%s\\ingres\\bin\\tm.exe\" -qSQL icesvr",
	    m_installPath);
    
	/*
    if (CreateProcessAsUser(m_phToken, NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
			    NULL, NULL, &si, &pi))
	*/
	if (CreateProcess(NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
			    NULL, NULL, &si, &pi))
    {
	DWORD	dw;

	WaitForSingleObject(pi.hProcess, INFINITE);
	if (GetExitCodeProcess(pi.hProcess, &dw))
	    bret = TRUE;
    }
    else
	bret = FALSE;
    
    cmd.Format("ii.%s.ingstart.*.icesvr 1", (LPCSTR)m_computerName);
    Exec(m_installPath + "\\ingres\\utility\\iisetres.exe", cmd, FALSE); 

    SetStdHandle(STD_INPUT_HANDLE, SaveStdin);
    CloseHandle(newstdin);
    AppendComment(IDS_DONE);

    AppendComment(IDS_CONFIGURINGICE);
    AppendToLog(IDS_CONFIGURINGICE);

    if (Exec(m_installPath + "\\ingres\\bin\\iceinstall.bat", 0)) 
	bret = FALSE;

#ifdef EVALUATION_RELEASE
    char CurDir[MAX_PATH], NewDir[MAX_PATH];
    /* populate icesvr */
    GetCurrentDirectory(sizeof(CurDir), CurDir);
    sprintf(NewDir, "%s\\DBupdate\\icesvr", m_InstallSource);
    SetCurrentDirectory(NewDir);
    Exec("reload.bat", 0, TRUE, FALSE);
    SetCurrentDirectory(CurDir);
#endif  /* EVALUATION_RELEASE */

    if (!CheckpointOneDatabase("icesvr -l +w"))
	bret = FALSE;

    if (!CheckpointOneDatabase("-uicedbuser icetutor -l +w"))
	bret = FALSE;

    if (!CheckpointOneDatabase("-uicedbuser icedb -l +w"))
	bret = FALSE;

    AppendComment(IDS_DONE);

    return bret;
}

BOOL
CInstallation::LoadPerfmon()
{
    BOOL    bret=TRUE;
    CString cmd;
    char    szCurDir[1024], szNewDir[1024];
    
    AppendComment(IDS_LOADPERFMON);
    AppendToLog(IDS_LOADPERFMON);
    
    cmd.Format("UNLODCTR.EXE oipfctrs");
    Execute(cmd, TRUE);
    
    cmd.Format("/s \"%s\\ingres\\files\\OIPFCTRS.REG\"", (LPCSTR)m_installPath);
    Exec("REGEDIT.EXE", cmd, TRUE, FALSE); 
    
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    sprintf(szNewDir, "%s\\ingres\\files", (LPCSTR)m_installPath);
    SetCurrentDirectory(szNewDir);
    cmd.Format("\"%s\\ingres\\files\\OIPFCTRS.INI\"", (LPCSTR)m_installPath);
    Exec("LODCTR.EXE", cmd, TRUE, FALSE); 
    SetCurrentDirectory(szCurDir);
    
    AppendComment(IDS_DONE);
    return bret;
}

BOOL
CInstallation::CreateIIDATABASE()
{
    CString dir;
    BOOL    bret = TRUE;

    dir.Format("%s", (LPCSTR)m_iidatabase);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));

    if (bret)
    {
	dir.Format("%s\\ingres", (LPCSTR)m_iidatabase);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    if (bret)
    {
	dir.Format("%s\\ingres\\data", (LPCSTR)m_iidatabase);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    if (bret)
    {
	dir.Format("%s\\ingres\\data\\default", (LPCSTR)m_iidatabase);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    dir.ReleaseBuffer();
    return bret;
}


BOOL
CInstallation::CreateIIWORK()
{
    CString dir;
    BOOL    bret = TRUE;
    
    dir.Format("%s", (LPCSTR)m_iiwork);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));

    if (bret)
    {
	dir.Format("%s\\ingres", (LPCSTR)m_iiwork);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    if (bret)
    {
	dir.Format("%s\\ingres\\work", (LPCSTR)m_iiwork);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    if (bret)
    {
	dir.Format("%s\\ingres\\work\\default", (LPCSTR)m_iiwork);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    dir.ReleaseBuffer();
    return bret;
}

BOOL
CInstallation::CreateIIDUMP()
{
    CString dir;
    BOOL    bret = TRUE;

    dir.Format("%s", (LPCSTR)m_iidump);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));

    if (bret)
    {
	dir.Format("%s\\ingres", (LPCSTR)m_iidump);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    if (bret)
    {
	dir.Format("%s\\ingres\\dmp", (LPCSTR)m_iidump);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    if (bret)
    {
	dir.Format("%s\\ingres\\dmp\\default", (LPCSTR)m_iidump);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    dir.ReleaseBuffer();
    return bret;
}

BOOL
CInstallation::CreateIICHECKPOINT()
{
    CString dir;
    BOOL    bret = TRUE;

    dir.Format("%s", (LPCSTR)m_iicheckpoint);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));
    
    if (bret)
    {
	dir.Format("%s\\ingres", (LPCSTR)m_iicheckpoint);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    if (bret)
    {
	dir.Format("%s\\ingres\\ckp", (LPCSTR)m_iicheckpoint);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    if (bret)
    {
	dir.Format("%s\\ingres\\ckp\\default", (LPCSTR)m_iicheckpoint);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    dir.ReleaseBuffer();
    return bret;
}

BOOL
CInstallation::CreateIIJOURNAL()
{
    CString dir;
    BOOL    bret = TRUE;

    dir.Format("%s", (LPCSTR)m_iijournal);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));
    
    if (bret)
    {
	dir.Format("%s\\ingres", (LPCSTR)m_iijournal);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    if (bret)
    {
	dir.Format("%s\\ingres\\jnl", (LPCSTR)m_iijournal);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    if (bret)
    {
	dir.Format("%s\\ingres\\jnl\\default", (LPCSTR)m_iijournal);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }

    dir.ReleaseBuffer();
    return bret;
}

BOOL
CInstallation::CreateIILOGFILE()
{
    CString dir;
    BOOL    bret = TRUE;

    dir.Format("%s", (LPCSTR)m_iilogfile);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));

    if (bret)
    {
	dir.Format("%s\\ingres", (LPCSTR)m_iilogfile);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    if (bret)
    {
	dir.Format("%s\\ingres\\log", (LPCSTR)m_iilogfile);
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectory(dir, NULL);
    }
    dir.ReleaseBuffer();

    if (m_enableduallogging)
    {
	dir.Format("%s", (LPCSTR)m_iiduallogfile);
    
	if (GetFileAttributes(dir) == -1)
	    bret = CreateDirectoryRecursively(dir.GetBuffer(dir.GetLength()+1));

	if (bret)
	{
	    dir.Format("%s\\ingres", (LPCSTR)m_iiduallogfile);
	    if (GetFileAttributes(dir) == -1)
		bret = CreateDirectory(dir, NULL);
	}
	if (bret)
	{
	    dir.Format("%s\\ingres\\log", (LPCSTR)m_iiduallogfile);
	    if (GetFileAttributes(dir) == -1)
		bret = CreateDirectory(dir, NULL);
	}  
    }
    dir.ReleaseBuffer();
    return bret;  
}

BOOL
CInstallation::CreateRepDirectories()
{
    CString dir;
    BOOL    bret = TRUE;
    int	    i;

    dir.Format("%s\\ingres\\rep", (LPCSTR)m_installPath);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectory(dir, NULL);

    dir.Format("%s\\ingres\\rep\\servers", (LPCSTR)m_installPath);
    if (GetFileAttributes(dir) == -1)
	bret = CreateDirectory(dir, NULL);

    if (bret)
    {
	for (i = 1; i <= 10; i++)
	{
	    dir.Format( "%s\\ingres\\rep\\servers\\server%d",
			(LPCSTR)m_installPath, i );
	    if (GetFileAttributes(dir) == -1)
		bret = CreateDirectory(dir, NULL);
	}
    }

    return bret;
}

void 
stop_logwatch_service()
{
    BOOL		bResult = FALSE;
    SC_HANDLE		schSCManager; 
    SC_HANDLE		schService;
    SERVICE_STATUS	ServiceStatus;
    LPSERVICE_STATUS	lpServiceStatus = &ServiceStatus;
    
    schSCManager = OpenSCManager(
	NULL,				/* computer name, NULL for current one */
	SERVICES_ACTIVE_DATABASE,	/* open default servie control manager */
	SC_MANAGER_ALL_ACCESS		/* privlages to create a serevice */
	);

    if(schSCManager == NULL)
	return;

    schService = OpenService( 
	schSCManager,		/* handle to service control manager database  */
	TEXT(SERVICENAME),	/* pointer to name of service to start */
	SERVICE_STOP		/* type of access to service */
	);

    if(schService != NULL)
    {
	bResult = ControlService(
	    schService, 		/* handle to service */
	    SERVICE_CONTROL_STOP,	/* control code */
	    lpServiceStatus 		/* pointer to service status structure */
	    );
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

void 
start_logwatch_service()
{
    BOOL	bResult = FALSE;
    SC_HANDLE	schSCManager; 
    SC_HANDLE	schService;

    schSCManager = OpenSCManager (
	NULL,				/* computer name, NULL for current one */
	SERVICES_ACTIVE_DATABASE,	/* open default servie control manager */
	SC_MANAGER_CREATE_SERVICE	/* privlages to create a serevice */
	);

    if(schSCManager == NULL)
	return;

    /* see if the service is there */
    schService = OpenService( 
	schSCManager,		/* handle to service control manager database */
	TEXT(SERVICENAME),	/* pointer to name of service to start */
	SERVICE_START		/* type of access to service */
	);

    /* start the service */
    bResult = StartService(
	schService,	/* handle of service  */
	0,		/* number of arguments */
	NULL		/* array of argument string pointers */
	);

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

BOOL
CInstallation::Create_icedbuser()
{
    BOOL		bret = TRUE;
    char		buf[1024];				   
    char		in_buf[1024];		
    HANDLE		newstdin, SaveStdin;
    PROCESS_INFORMATION	pi;
    SECURITY_ATTRIBUTES handle_security;
    STARTUPINFO 	si;

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;
    memset((char*)&pi,0,sizeof(pi));
    memset((char*)&si,0,sizeof(si));
    si.cb = sizeof(si);

    AppendComment(IDS_CREATEICEDBUSER);
    AppendToLog(IDS_CREATEICEDBUSER);

    sprintf(in_buf, "%s\\ingres\\ice\\scripts\\icedbusr.sql",
	    m_installPath);

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
		    GetCurrentProcess(), &SaveStdin, 0, FALSE,
		    DUPLICATE_SAME_ACCESS);

    newstdin = CreateFile(in_buf, GENERIC_READ, FILE_SHARE_READ,
			  &handle_security, OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL, NULL);

    if ((newstdin == INVALID_HANDLE_VALUE) ||
	(SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE))
    {
	bret = FALSE;
    }

    si.dwFlags |= STARTF_USESTDHANDLES;   
    si.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle (STD_ERROR_HANDLE);

    sprintf(buf, "\"%s\\ingres\\bin\\tm.exe\" -qSQL iidbdb",
	    m_installPath);

	/*
    if (CreateProcessAsUser(m_phToken, NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
			    NULL, NULL, &si, &pi))
	*/
	if (CreateProcess(NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
			    NULL, NULL, &si, &pi))
    {
	DWORD dw;

	WaitForSingleObject(pi.hProcess,INFINITE);
	if (GetExitCodeProcess(pi.hProcess,&dw))
	    bret = TRUE;
    }
    else
    {
	bret = FALSE;
    }

    CloseHandle(newstdin);
    AppendComment(IDS_DONE);

    return bret;
}

/*
**
** Name: Local_NMgtIngAt - grab the value of a variable.
**
** Description:
**	This function will emulate NMgtIngAt() without requiring the DLL's
**	since they may not be available at installation time.
**	The environment is checked first, followed by the symbol.tbl file
**	If a value is successfully found, the return value will be 0
*/

int
CInstallation::Local_NMgtIngAt(CString strKey, CString &strRetValue)
{
    int		    iPos;
    CStdioFile	    theInputFile;
    CFileException  fileException;
    CString	    strInputString;
    CString	    strII_SYSTEM;
    char	    szBuffer[MAX_PATH];
    char	    szFileName[MAX_PATH];

    /* Check system environment variable */
    if (GetEnvironmentVariable(strKey.GetBuffer(255), szBuffer, sizeof(szBuffer)))
    {
	strRetValue=szBuffer;
	return (1);
    }

    /* Check symbol.tbl file */
    if (!GetEnvironmentVariable("II_SYSTEM", szBuffer, sizeof(szBuffer)))
	return (0);

    strII_SYSTEM = szBuffer;
    sprintf(szFileName, "%s\\ingres\\files\\symbol.tbl", strII_SYSTEM.GetBuffer(255));
    if (!theInputFile.Open(szFileName, CFile::modeRead | CFile::typeText,
	&fileException)) 
    {
	return (0);
    }

    while (theInputFile.ReadString(strInputString) != FALSE)
    {
	strInputString.TrimRight();

	iPos = strInputString.Find(strKey);
	if (iPos >= 0)
	{
	    /* should normally work under DBCS .... */
	    if ((strInputString.GetAt(iPos+strKey.GetLength()) == '\t') ||
		(strInputString.GetAt(iPos+strKey.GetLength()) == ' '))
	    {
		strInputString = strInputString.Right(strInputString.GetLength() -
				 strKey.GetLength()); /* -iPos ? */
		strInputString.TrimLeft();
		strRetValue = strInputString;
		theInputFile.Close();
		return(1);
	    }
	} 
    }

    theInputFile.Close();

    return (0);  /* Value not found */
}

BOOL
CInstallation::GetRegValueBOOL(CString strKey, BOOL DefaultValue)
{
    HKEY	    hKey;
    int		    ret_code = 0, i;
    TCHAR	    KeyName[MAX_PATH + 1];
    TCHAR	    tchValue[MAX_PATH];
    unsigned long   dwSize;
    DWORD	    dwType;
    CString	    strValue;

    if (!m_SoftwareKey)
    {
	/*
	** We have to figure out the right registry software key that
	** matches the install path.
	*/
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\IngresCorporation\\Ingres",
		     0, KEY_ALL_ACCESS, &hKey);
	for (i = 0; ret_code != ERROR_NO_MORE_ITEMS; i++)
	{
	    ret_code = RegEnumKey(hKey, i, KeyName, sizeof(KeyName));
	    if (!ret_code)
	    {
		RegOpenKeyEx(hKey, KeyName, 0, KEY_ALL_ACCESS, &m_SoftwareKey);
		dwSize = sizeof(tchValue);
		RegQueryValueEx(m_SoftwareKey, "II_SYSTEM", NULL, &dwType,
				(unsigned char *)&tchValue, &dwSize);

		if (!_tcsicmp(tchValue, m_installPath))
		    break;
		else
		    RegCloseKey(m_SoftwareKey);
	    }
	}
	RegCloseKey(hKey);
    }

    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(m_SoftwareKey, strKey, NULL, &dwType,
			(unsigned char *)&tchValue, &dwSize) == ERROR_SUCCESS)
    {
	strValue = tchValue;
	strValue.MakeLower();
	if (strValue.Compare("y")==0 || strValue.Compare("yes")==0 || 
	    strValue.Compare("1")==0 || strValue.Compare("true")==0)
	{
	    return TRUE;
	}
	else
	    return FALSE;
    }
    else
	return DefaultValue;
}

VOID
CInstallation::RetrieveHostname(CHAR *HostName)
{
    CHAR	ComputerName[256], *p;
    DWORD	dwSize = sizeof(ComputerName);
    
    GetComputerNameEx(ComputerNamePhysicalNetBIOS, ComputerName, &dwSize);
    for (p = ComputerName; *p != '\0'; p = _tcsinc(p)) /* because '\\' can be trailing byte, use appropriate pointer increment function*/
    {
	switch (*p)
	{
	case '.':
	case '&':
	case '"':
	case '\'':
	case '\\':
	case '$':
	case '#':
	case '!':
	case ' ':
	case '%':
	    *p = '_';
	}
    }
    
    strcpy(HostName, ComputerName);
}

BOOL
CInstallation::WriteRSPContentsToLog()
{
    CString	    s;
    CStdioFile	    theInputFile;
    CFileException  fileException;
    CString	    strBuffer;
    CString	    tempstrBuffer;
    bool	    checkforpassword = FALSE;

    s.Format(IDS_USINGRESPONSEFILE, theInstall.m_ResponseFile);
    AppendToLog(s);

    s.Format(IDS_READINGRESPONSEFILE, theInstall.m_ResponseFile);
    AppendToLog(s);

    if (!theInputFile.Open(theInstall.m_ResponseFile,
	CFile::modeRead  | CFile::typeText  |
	CFile::shareExclusive, &fileException))
    {
	DWORD	rc, dwRetCode;
	LPTSTR	lpszText;

	dwRetCode = GetLastError();
	rc = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
			    FORMAT_MESSAGE_ARGUMENT_ARRAY |
			    FORMAT_MESSAGE_ALLOCATE_BUFFER,
			    NULL,
			    dwRetCode,
			    LANG_NEUTRAL,
			    (LPTSTR)&lpszText,
			    0,
			    NULL);

	if (rc)
	{
	    strBuffer.Format("Could not open file '%s'; Error [%d] - %s",
			     theInstall.m_ResponseFile, dwRetCode, lpszText);
	}
	else
	{
	    strBuffer.Format("Error:  Could not open file '%s'",
			     theInstall.m_ResponseFile);
	}

	AfxMessageBox(strBuffer);
	return(FALSE);
    }
    else
    {
	if (theInstall.m_servicepassword.GetLength() > 0)
	    checkforpassword = TRUE;

	while (theInputFile.ReadString(strBuffer) != FALSE)
	{
	    if (checkforpassword)
	    {
		tempstrBuffer = strBuffer;
		tempstrBuffer.MakeLower();
		if (tempstrBuffer.Find( "servicepassword" ) != -1)
		{
		    AppendToLog("servicepassword = ********") ;
		    checkforpassword = FALSE;
		}
		else
		    AppendToLog(strBuffer);
	    }
	    else
		AppendToLog(strBuffer);
	}
    }

    theInputFile.Close();
    return(TRUE);
}

void
CInstallation::GetNTErrorText(int code, CString &strErrText)
{
    DWORD   dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			  FORMAT_MESSAGE_ARGUMENT_ARRAY,
			  NULL,
			  code,
			  LANG_NEUTRAL,
			  (LPTSTR)&lpszTemp,
			  0,
			  NULL);

    /* supplied buffer is not long enough */
    if (!dwRet)
        strErrText.Format("Cannot obtain error information for NT error #%i\r\n", code);
    else
    {
        lpszTemp[_tcslen(lpszTemp)-2] = _T('\0');  /* remove cr and newline character */
        strErrText.Format("%i - %s", code, lpszTemp);
    }

    if (lpszTemp)
        LocalFree((HLOCAL)lpszTemp);

    return;
}

/*
**
** Name: CreateSecurityDescriptor - Create a SECURITY_ATTRIBUTES structure
**       Taken from iimksec.c in cl!clf!cs.
**
** Description:
**  	This routine is used to set the security attributes of the 
**	SECURITY_ATTRIBUTES structure that is passed to it. In its
**	initial design we set this equal to a NULL Discretionary 
**	Access Control List (DACL). This allows implicit access to 
**	everyone.
**	It is called by routines which create objects (mutexes, processes, etc)
**	that need to be accessed by more than one user.
*/

BOOL
CreateSecurityDescriptor(SECURITY_ATTRIBUTES *sa)
{
    static PSECURITY_DESCRIPTOR	pSD = NULL;

    if (pSD == NULL)
    {
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL)
	    goto cleanup;

	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
	    goto cleanup;

	/* Initialize new ACL */
	if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
	    goto cleanup;
    }

    sa->nLength = sizeof(SECURITY_ATTRIBUTES);
    sa->lpSecurityDescriptor = pSD;
    sa->bInheritHandle = TRUE;
    return(FALSE);

cleanup:
    if (pSD != NULL)
	LocalFree((HLOCAL) pSD);
    return (TRUE);
}

BOOL
CInstallation::WriteUserEnvironmentToLog()
{
    AppendToLog(IDS_DISPLAYINGUSERENVIRONMENT);
    Exec("cmd", "/c set", TRUE, FALSE);
    return (TRUE);
}

int
CInstallation::CreateDirectoryRecursively(char *szDirectory)
{
    char    szBuffer[MAX_PATH] = "";
    char    szNewDirectory[MAX_PATH] = "";
    char    *p;
    int	    rc;

    if (!szDirectory[0])
	return(0);

    if (GetFileAttributes(szDirectory) != -1)
	return(1);

    sprintf(szNewDirectory, "%s", szDirectory);

    p = strtok(szNewDirectory, "\\");

    if (p)
	strcpy(szBuffer, p);

    while (p)
    {
	p = strtok(NULL, "\\");

	if (p)
	{
	    strcat(szBuffer, "\\");
	    strcat(szBuffer, p);
	    rc = CreateDirectory(szBuffer, NULL);
	}   
    }  /* while... */

    if (GetFileAttributes(szDirectory) != -1)
	return(1);
	else
	return(0);
}

void
CInstallation::RemoveOldDirectories()
{
    char szDirectoryPath[512];;
    
    AppendToLog(IDS_REMOVINGOLDFILES);
    
    sprintf(szDirectoryPath, "%s\\ingres\\bin", theInstall.m_installPath);
    RemoveOneDirectory(szDirectoryPath);
    sprintf(szDirectoryPath, "%s\\ingres\\utility", theInstall.m_installPath);
    RemoveOneDirectory(szDirectoryPath);
    sprintf(szDirectoryPath, "%s\\ingres\\vdba", theInstall.m_installPath);
    RemoveOneDirectory(szDirectoryPath);
    
    return;
}

BOOL
CInstallation::RemoveOneDirectory(char *szDirectoryPath)
{
    BOOL    status = TRUE;
    DWORD   attr;
    
    attr = GetFileAttributes(szDirectoryPath);
    
    if(attr & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE))
    {
	attr &= ~(FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|
	    FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE);
	SetFileAttributes(szDirectoryPath, attr);
    }
    
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
    {
	char		dirname[256];
	char		fname[256];
	WIN32_FIND_DATA	findbuf;
	HANDLE		hfind;

	strcpy(dirname, szDirectoryPath);
	strcat(dirname, "\\*.*");

	if((hfind = FindFirstFile(dirname, &findbuf)) != INVALID_HANDLE_VALUE)
	{
	    do
	    {
		if (findbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		    continue;
		
		if ((findbuf.dwFileAttributes & 
		    (FILE_ATTRIBUTE_READONLY |
		    FILE_ATTRIBUTE_HIDDEN |
		    FILE_ATTRIBUTE_SYSTEM)) &&
		    (SetFileAttributes(dirname, FILE_ATTRIBUTE_NORMAL) 
		    == FALSE))
		    break;
		strcpy(fname, szDirectoryPath);
		strcat(fname,"\\");
		strcat(fname, findbuf.cFileName);
		status = DeleteFile(fname);
		if(status == 0)
		    break;
	    } while (FindNextFile(hfind, &findbuf));
	    FindClose(hfind);
	}
	status = RemoveDirectory(szDirectoryPath);
    }
    else
	status = DeleteFile(szDirectoryPath);

    return status;
}

BOOL
CInstallation::RunPreStartupPrg()
{
    BOOL ret = TRUE;
    
    AppendToLog(IDS_PRESTARTUP);

    /*
    ** This program will execute if the response file value "IngresPreStartupRunPrg"
    ** is detected.  It will execute just prior to starting up Ingres for the first time.
    */
    Exec(m_IngresPreStartupRunPrg, m_IngresPreStartupRunParams, TRUE, FALSE);
    return ret;
}

BOOL
CInstallation::CheckAdminUser()
{
    BOOL			fAdmin;
    HANDLE			hThread;
    TOKEN_GROUPS		*ptg = NULL;
    DWORD			cbTokenGroups;
    DWORD			dwGroup;
    PSID			psidAdmin;
    SID_IDENTIFIER_AUTHORITY	SystemSidAuthority = SECURITY_NT_AUTHORITY;
   
    if (!theInstall.m_win95)	/* if on 95 skip check and return true. */
    {
	/* Check the Administrator User Right */
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread))
	{
	    if (GetLastError() == ERROR_NO_TOKEN)
	    {
		/*
		** If the thread does not have an access token, we'll examine the
		** access token associated with the process.
		*/
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThread))
		    return (FALSE);
	    }
	    else 
		return (FALSE);
	}
	
	/*
	** Then we must query the size of the group information associated with
	** the token. Note that we expect a FALSE result from GetTokenInformation
	** because we've given it a NULL buffer. On exit cbTokenGroups will tell
	** the size of the group information.
	*/
	if (GetTokenInformation (hThread, TokenGroups, NULL, 0, &cbTokenGroups))
	    return (FALSE);

	/*
	** Here we verify that GetTokenInformation failed for lack of a large
	** enough buffer.
	*/
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	    return (FALSE);
	
	/*
	** Now we allocate a buffer for the group information.
	** Since _alloca allocates on the stack, we don't have
	** to explicitly deallocate it. That happens automatically
	** when we exit this function.
	*/
	if (!(ptg = (struct _TOKEN_GROUPS *)malloc(cbTokenGroups))) 
	    return (FALSE);
	
	/*
	** Now we ask for the group information again.
	** This may fail if an administrator has added this account
	** to an additional group between our first call to
	** GetTokenInformation and this one.
	*/
	if (!GetTokenInformation(hThread, TokenGroups, ptg, cbTokenGroups,
				 &cbTokenGroups))
	{
	    return (FALSE);
	}
	
	/* Now we must create a System Identifier for the Admin group. */
	if (!AllocateAndInitializeSid(&SystemSidAuthority, 2, 
				      SECURITY_BUILTIN_DOMAIN_RID, 
				      DOMAIN_ALIAS_RID_ADMINS,
				      0, 0, 0, 0, 0, 0, &psidAdmin))
	{
	    return (FALSE);
	}
	
	/*
	** Finally we'll iterate through the list of groups for this access
	** token looking for a match against the SID we created above.
	*/
	fAdmin = FALSE;
	
	for (dwGroup = 0; dwGroup < ptg->GroupCount; dwGroup++)
	{
	    if (EqualSid(ptg->Groups[dwGroup].Sid, psidAdmin))
	    {
		fAdmin = TRUE;
		break;
	    }
	}

	/* Before we exit we must explicity deallocate the SID we created. */
	FreeSid (psidAdmin);
	return (fAdmin);
    }
    else
	return (TRUE);
}

BOOL
CInstallation::CheckpointDatabases(BOOL jnl_flag)
{
    BOOL	    status = TRUE;
    WIN32_FIND_DATA findbuf;
    HANDLE	    hfind;
    CString	    dirname;
    char	    ckp_options[50];

    Local_NMgtIngAt(CString("II_DATABASE"), dirname);
    dirname += CString("\\ingres\\data\\default\\*.*");

    if ((hfind = FindFirstFile(dirname, &findbuf)) != INVALID_HANDLE_VALUE)
    {
	do
	{
	    if (strcmp(findbuf.cFileName, ".") != 0 &&
		strcmp(findbuf.cFileName, "..") != 0 &&
		findbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    {
		if (jnl_flag)
		    strcpy(ckp_options, "ckpdb.exe +j ");
		else
		    strcpy(ckp_options, "ckpdb.exe -j ");

		strcat(ckp_options, findbuf.cFileName);
		strcat(ckp_options, " -l +w");
		if (!(status = Execute(ckp_options, TRUE)))
		{
		    if (status)
			status = FALSE;
		}
	    }
	} while (FindNextFile(hfind, &findbuf));

	FindClose(hfind);
    }
    else
	status = FALSE;

    return status;
}

CString
CInstallation::GetRegValue(CString strKey)
{
    HKEY	    hKey;
    int		    ret_code = 0, i;
    TCHAR	    KeyName[MAX_PATH + 1];
    TCHAR	    tchValue[MAX_PATH];
    unsigned long   dwSize;
    DWORD	    dwType;
    CString	    szBuf, strFile;

    if (!m_SoftwareKey)
    {
	/*
	** We have to figure out the right registry software key that
	** matches the install path.
	*/
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\IngresCorporation\\Ingres",
		     0, KEY_ALL_ACCESS, &hKey);
	for (i = 0; ret_code != ERROR_NO_MORE_ITEMS; i++)
	{
	    ret_code = RegEnumKey(hKey, i, KeyName, sizeof(KeyName));
	    if (!ret_code)
	    {
		RegOpenKeyEx(hKey, KeyName, 0, KEY_ALL_ACCESS, &m_SoftwareKey);
		dwSize = sizeof(tchValue);
		RegQueryValueEx(m_SoftwareKey, "II_SYSTEM", NULL, &dwType,
				(unsigned char *)&tchValue, &dwSize);

		if (!_tcsicmp(tchValue, m_installPath))
		    break;
		else
		    RegCloseKey(m_SoftwareKey);
	    }
	}
	RegCloseKey(hKey);
    }

    /*
    ** Now that we know where to look, let's get the value that we need
    ** from the registry.
    */
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(m_SoftwareKey, strKey, NULL, &dwType,
			(unsigned char *)&tchValue, &dwSize) == ERROR_SUCCESS)
    {
	return(CString(tchValue));
    }
    else
	return(CString(""));
}

BOOL
CInstallation::InstallIngresService()
{
    CComponent	*dbms = GetDBMS();
    char	szServiceName[512];
    BOOL	nret = FALSE;

    /*
    ** First, shut down the Ingres service.
    */
    if (m_bClusterInstall)
	OfflineResource();
    else
	Exec(m_installPath + "\\ingres\\utility\\ingstop.exe", 0);

    /*
    ** Then, check out if Ingres service was already installed.
    */
	if (m_DBATools)
		sprintf(szServiceName, "Ingres_DBATools_%s", m_installationcode);
	else
	    sprintf(szServiceName, "Ingres_Database_%s", m_installationcode);
    if (m_DBMSupgrade || !(dbms && dbms->m_selected))
    {
	SC_HANDLE schSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (schSCManager)
	{
	    SC_HANDLE schService=OpenService(schSCManager, szServiceName, SERVICE_QUERY_STATUS);
	    if (schService) 
		return TRUE;
	}
    }

    /*
    ** Install Ingres service.
    */
    AppendComment(IDS_SERVICE);
    AppendToLog(IDS_SERVICE);
    if (m_DBATools)
	Exec("opingsvc.exe", "remove DBATools", FALSE);
    else
	Exec("opingsvc.exe", "remove", FALSE);
    if ((dbms && dbms->m_selected))
	nret = (Exec("opingsvc.exe", "install", TRUE)==0);
    else
	{
		if (m_DBATools)
		nret = (Exec("opingsvc.exe", "install DBATools client", TRUE)==0);
		else
		nret = (Exec("opingsvc.exe", "install client", TRUE)==0);
	}

#ifndef EVALUATION_RELEASE
	if (nret)
    nret = SetServiceAuto(szServiceName, m_serviceAuto);
#endif /* EVALUATION_RELEASE */

    if (nret)
    {
	AppendComment(IDS_DONE);
	nret = TRUE;
    }
    else
    {
	AppendComment(IDS_FAILED);
	nret = FALSE;
    }

    return(nret);
}

void
CInstallation::SetODBCEnvironment()
{
    char	szSystemDir[512]; *szSystemDir = 0;
    char	szWindowsDir[512]; *szWindowsDir = 0;
    char	szODBCINIFileName[1024];
    char	szODBCDriverFileName[1024];
    char	szODBCSetupFileName[1024];
    char	szODBCReadOnly[2];
    char	szOdbcRegName[MAX_PATH];
    char	szOdbcDrvName[MAX_PATH];
    DWORD	dwDisposition, dwUsageCount, dwType, dwSize;
    HKEY	hKey;
    CComponent	*odbc = GetODBC();

    m_setup_IngresODBC = GetRegValueBOOL("setup_ingresodbc", FALSE);
    m_setup_IngresODBCReadOnly = GetRegValueBOOL("setup_ingresodbcreadonly", FALSE);
    if (!m_setup_IngresODBC) return;

    AppendToLog(IDS_SETTINGUPINGRESODBC);

    GetWindowsDirectory(szWindowsDir, sizeof(szWindowsDir));
    sprintf(szODBCINIFileName, "%s\\ODBCINST.INI", szWindowsDir);
    if (m_setup_IngresODBC && !m_setup_IngresODBCReadOnly)
    {
		sprintf(szODBCDriverFileName, "%s\\ingres\\bin\\caiiod35.dll", m_installPath);
		sprintf(szODBCReadOnly, "N");
    }
    if (m_setup_IngresODBC && m_setup_IngresODBCReadOnly)
    {
		sprintf(szODBCDriverFileName, "%s\\ingres\\bin\\caiiro35.dll", m_installPath);
		sprintf(szODBCReadOnly, "Y");
    }
    sprintf(szODBCSetupFileName,  "%s\\ingres\\bin\\caiioc35.dll", m_installPath);

    sprintf(szOdbcRegName, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres %d.%d", ii_ver.major, ii_ver.minor);
    if(!RegCreateKeyEx(HKEY_LOCAL_MACHINE, szOdbcRegName, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
    {
	RegSetValueEx(hKey, "Driver", 0, REG_SZ, (CONST BYTE *)szODBCDriverFileName, strlen(szODBCDriverFileName)+1);
	RegSetValueEx(hKey, "Setup", 0, REG_SZ, (CONST BYTE *)szODBCSetupFileName, strlen(szODBCSetupFileName)+1);
	RegSetValueEx(hKey, "APILevel", 0, REG_SZ, (CONST BYTE *)"1", strlen("1")+1);
	RegSetValueEx(hKey, "ConnectFunctions", 0, REG_SZ, (CONST BYTE *)"YYN", strlen("YYN")+1);
	RegSetValueEx(hKey, "DriverODBCVer", 0, REG_SZ, (CONST BYTE *)"03.50", strlen("03.50")+1);
	RegSetValueEx(hKey, "DriverReadOnly", 0, REG_SZ, (CONST BYTE *)szODBCReadOnly, strlen(szODBCReadOnly)+1);
	RegSetValueEx(hKey, "DriverType", 0, REG_SZ, (CONST BYTE *)"Ingres", strlen("Ingres")+1);
	RegSetValueEx(hKey, "FileUsage", 0, REG_SZ, (CONST BYTE *)"0", strlen("0")+1);
	RegSetValueEx(hKey, "SQLLevel", 0, REG_SZ, (CONST BYTE *)"0", strlen("0")+1);
	RegSetValueEx(hKey, "CPTimeout", 0, REG_SZ, (CONST BYTE *)"60", strlen("60")+1);

	dwSize = (DWORD) sizeof ( dwUsageCount);
	if (!RegQueryValueEx(hKey, "UsageCount", NULL, &dwType, (LPBYTE)&dwUsageCount, &dwSize))
	    dwUsageCount = dwUsageCount + 1;
	else 
	    dwUsageCount=1;
	RegSetValueEx(hKey, "UsageCount", 0, REG_DWORD, (CONST BYTE *)&dwUsageCount, sizeof(DWORD));
	RegSetValueEx(hKey, "Vendor", 0, REG_SZ, (CONST BYTE *)"Ingres Corporation", strlen("Ingres Corporation")+1);

	RegCloseKey(hKey);
    }

    if(!RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers", 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
    {
	sprintf(szOdbcDrvName, "Ingres %d.%d", ii_ver.major, ii_ver.minor);
	RegSetValueEx(hKey, szOdbcDrvName, 0, REG_SZ, (CONST BYTE *)"Installed", strlen("Installed")+1);
	RegCloseKey(hKey);
    }
    strcat(szOdbcDrvName, " (32 bit)");
    WritePrivateProfileString("ODBC 32 bit Drivers", szOdbcDrvName, "Installed", szODBCINIFileName);
    WritePrivateProfileString(szOdbcDrvName, "Driver", szODBCDriverFileName, szODBCINIFileName);
    WritePrivateProfileString(szOdbcDrvName, "Setup", szODBCSetupFileName, szODBCINIFileName);
    WritePrivateProfileString(szOdbcDrvName, "32Bit", "1", szODBCINIFileName);

    DeleteRegValue("setup_ingresodbc");
    DeleteRegValue("setup_ingresodbcreadonly");
    DeleteRegValue(odbc->m_name);
	
    if(!RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ODBC\\ODBCINST.INI\\Ingres", 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
    {
	RegSetValueEx(hKey, "Driver", 0, REG_SZ, (CONST BYTE *)szODBCDriverFileName, strlen(szODBCDriverFileName)+1);
	RegSetValueEx(hKey, "Setup", 0, REG_SZ, (CONST BYTE *)szODBCSetupFileName, strlen(szODBCSetupFileName)+1);
	RegSetValueEx(hKey, "APILevel", 0, REG_SZ, (CONST BYTE *)"1", strlen("1")+1);
	RegSetValueEx(hKey, "ConnectFunctions", 0, REG_SZ, (CONST BYTE *)"YYN", strlen("YYN")+1);
	RegSetValueEx(hKey, "DriverODBCVer", 0, REG_SZ, (CONST BYTE *)"03.50", strlen("03.50")+1);
	RegSetValueEx(hKey, "DriverReadOnly", 0, REG_SZ, (CONST BYTE *)szODBCReadOnly, strlen(szODBCReadOnly)+1);
	RegSetValueEx(hKey, "DriverType", 0, REG_SZ, (CONST BYTE *)"Ingres", strlen("Ingres")+1);

	RegSetValueEx(hKey, "FileUsage", 0, REG_SZ, (CONST BYTE *)"0", strlen("0")+1);
	RegSetValueEx(hKey, "SQLLevel", 0, REG_SZ, (CONST BYTE *)"0", strlen("0")+1);
	RegSetValueEx(hKey, "CPTimeout", 0, REG_SZ, (CONST BYTE *)"60", strlen("60")+1);

	dwSize = (DWORD) sizeof ( dwUsageCount);
	if (!RegQueryValueEx(hKey, "UsageCount", NULL, &dwType, (LPBYTE)&dwUsageCount, &dwSize))
	    dwUsageCount = dwUsageCount + 1;
	else 
	    dwUsageCount=1;
	RegSetValueEx(hKey, "UsageCount", 0, REG_DWORD, (CONST BYTE *)&dwUsageCount, sizeof(DWORD));
	RegSetValueEx(hKey, "Vendor", 0, REG_SZ, (CONST BYTE *)"Ingres Corporation", strlen("Ingres Corporation")+1);

	RegCloseKey(hKey);
    }

    if(!RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers", 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
    {
	RegSetValueEx(hKey, "Ingres", 0, REG_SZ, (CONST BYTE *)"Installed", strlen("Installed")+1);
	RegCloseKey(hKey);
    }

    WritePrivateProfileString("ODBC 32 bit Drivers", "Ingres (32 bit)", "Installed", szODBCINIFileName);
    WritePrivateProfileString("Ingres (32 bit)", "Driver", szODBCDriverFileName, szODBCINIFileName);
    WritePrivateProfileString("Ingres (32 bit)", "Setup", szODBCSetupFileName, szODBCINIFileName);
    WritePrivateProfileString("Ingres (32 bit)", "32Bit", "1", szODBCINIFileName);

    DeleteRegValue("setup_ingresodbc");
    DeleteRegValue("setup_ingresodbcreadonly");
    DeleteRegValue(odbc->m_name);


    return;
}

void
CInstallation::DeleteRegValue(CString strKey)
{
    HKEY	    hKey;
    int		    ret_code = 0, i;
    TCHAR	    KeyName[MAX_PATH + 1];
    TCHAR	    tchValue[MAX_PATH];
    unsigned long   dwSize;
    DWORD	    dwType;
    CString	    szBuf, strFile;

    if (!m_SoftwareKey)
    {
	/*
	** We have to figure out the right registry software key that
	** matches the install path.
	*/
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\IngresCorporation\\Ingres",
		     0, KEY_ALL_ACCESS, &hKey);
	for (i = 0; ret_code != ERROR_NO_MORE_ITEMS; i++)
	{
	    ret_code = RegEnumKey(hKey, i, KeyName, sizeof(KeyName));
		if(ret_code != ERROR_SUCCESS)
			break;

	    if (!ret_code)
	    {
		RegOpenKeyEx(hKey, KeyName, 0, KEY_ALL_ACCESS, &m_SoftwareKey);
		dwSize = sizeof(tchValue);
		RegQueryValueEx(m_SoftwareKey, "II_SYSTEM", NULL, &dwType,
				(unsigned char *)&tchValue, &dwSize);

		if (!strcmp(tchValue, m_installPath))
		    break;
		else
		    RegCloseKey(m_SoftwareKey);
	    }
	}
	RegCloseKey(hKey);
    }

    /*
    ** Now that we know where to look, let's delete the value from the registry.
    */
    RegDeleteValue(m_SoftwareKey, strKey);
}

CComponent *CInstallation::GetODBC()
{
    CComponent *c;
    CString	s;
    int		i;
    
    s.LoadString(IDS_LABELODBC);
    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);
	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

void
CInstallation::InstallAcrobat()
{
    CString	s;

    s.Format("%s\\ingres\\temp\\AdbeRdr707_DLM_en_US.exe",
	     (LPCSTR)m_installPath);
    Execute(s, FALSE);
    //DeleteFile(s);
}

void
CInstallation::OpenPageForAcrobat()
{
	 HKEY 	hKey = 0;
     int	rc;
     DWORD	dwType, dwSize;
     char	browser[MAX_PATH+1];
     char 	cmd [MAX_PATH+1];

     rc = RegOpenKeyEx(HKEY_CLASSES_ROOT, "HTTP\\shell\\open\\command", 0, KEY_QUERY_VALUE, &hKey);
     if (rc == ERROR_SUCCESS)
     {
	 dwSize=sizeof(browser);
	 if (RegQueryValueEx(hKey, NULL, NULL, &dwType, (BYTE *)&browser, &dwSize) == ERROR_SUCCESS)
	 {
		sprintf(cmd, "%s %s", browser, "http:\\\\www.adobe.com");
		Execute(cmd, FALSE);
		RegCloseKey(hKey);
	 }

     }

}

BOOL
CInstallation::IsCurrentAcrobatAlreadyInstalled()
{
    HKEY	hKey = 0;
    int		rc;

#ifdef WIN64
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Wow6432Node\\Adobe\\Acrobat Reader\\7.0", 0,
		      KEY_QUERY_VALUE, &hKey );
#else
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Adobe\\Acrobat Reader\\7.0", 0,
		      KEY_QUERY_VALUE, &hKey );
#endif  /* WIN64 */

    if (rc == ERROR_SUCCESS)
    {
	if (hKey)
	    RegCloseKey(hKey);
	return TRUE;
    }
    else
	return FALSE;
}

BOOL
CInstallation::RegisterActiveXControls()
{
    BOOL	bret=TRUE;
    CString	cmd;

    AppendToLog(IDS_REGISTERCONTROLS);

    cmd.Format("/s \"%s\\ingres\\bin\\IJACTRL.OCX\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    cmd.Format("/s \"%s\\ingres\\bin\\ipm.ocx\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    cmd.Format("/s \"%s\\ingres\\bin\\sqlquery.ocx\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    cmd.Format("/s \"%s\\ingres\\bin\\vcda.ocx\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    cmd.Format("/s \"%s\\ingres\\bin\\vsda.ocx\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    cmd.Format("/s \"%s\\ingres\\bin\\SVRIIA.DLL\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    cmd.Format("/s \"%s\\ingres\\bin\\svrsqlas.dll\"", (LPCSTR)m_installPath);
    bret = (Exec("REGSVR32.EXE", cmd, TRUE, FALSE)==0);

    return bret;
}

CComponent * CInstallation::GetESQLFORTRAN()
{
    CComponent *c;
    CString	s;
    int		i;

    s.LoadString(IDS_LABELESQLFORTRAN);

    for (i = 0; i < m_components.GetSize(); i++)
    {
	c = (CComponent *)m_components.GetAt(i);

	if (c && c->m_name == s)
	    return c;
    }
    return 0;
}

/*
**	History:
**	23-July-2001 (penga03)
**	    Check if there are MSI and Cabinet files in ingres temp directory, 
**	    if so, delete them.
*/
void CInstallation::RemoveTempMSIAndCab()
{
	char SubKey[128];
	char msiloc[MAX_PATH+1], msiname[MAX_PATH+1], cabname[MAX_PATH+1];
	DWORD nret, dwType;
	HKEY hkSubKey;
	CString ii_temp;

	sprintf(SubKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", m_installationcode);
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_ALL_ACCESS, &hkSubKey)==ERROR_SUCCESS)
	{
		nret=sizeof(msiloc);
		RegQueryValueEx(hkSubKey,"msilocation",0,&dwType,(BYTE *)msiloc,&nret);
		RegDeleteValue(hkSubKey, "msilocation");		
		RegCloseKey(hkSubKey);
	}
	if(!msiloc[0])
	{
		Local_NMgtIngAt("II_TEMPORARY", ii_temp);
		strcpy(msiloc, ii_temp.GetBuffer(ii_temp.GetLength()+1));
	}
	
	sprintf(msiname, "%s\\IngresII[%s].msi", msiloc, m_installationcode);
	sprintf(cabname, "%s\\IngresII[%s].cab", msiloc, m_installationcode);
	DeleteFile(msiname);
	DeleteFile(cabname);
}

/*
**	Name: Local_PMget
**
**	Description:
**	A version of PMget independent of Ingres CL functions. 
**
**	History:
**	23-July-2001 (penga03)
**	    Created.
*/
BOOL 
CInstallation::Local_PMget(CString strKey, CString &strRetValue)
{
	char ii_system[MAX_PATH+1], szFileName[MAX_PATH+1];
	CStdioFile theInputFile;
	int iPos;
	CString strInputString;

	strKey.MakeLower();
    if(!GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system)))
		return FALSE;

	sprintf(szFileName, "%s\\ingres\\files\\config.dat", ii_system);
	if(!theInputFile.Open(szFileName, CFile::modeRead | CFile::typeText, 0))
		return FALSE;
	while(theInputFile.ReadString(strInputString) != FALSE)
	{
		strInputString.TrimRight();
		iPos=strInputString.Find(strKey);
		if(iPos>=0)
		{
			if( (strInputString.GetAt(iPos+strKey.GetLength()) == ':') || 
				(strInputString.GetAt(iPos+strKey.GetLength()) == ' ') )
			{ 
				strInputString = strInputString.Right(strInputString.GetLength()-strKey.GetLength()-1);
				strInputString.TrimLeft();
				strInputString.TrimLeft("\'");
				strInputString.TrimRight("\'");
				strInputString.Replace("\\\\", "\\");
				strRetValue=strInputString;
				theInputFile.Close();
				return TRUE;
			}
		}
	}
	theInputFile.Close();
	return FALSE;
}

int
CInstallation::RemoveOldShellLinks()
{
    char		szOldFullyQDisplayName[MAX_PATH];
    char		szFullyQDisplayName[MAX_PATH];
    char		szFullyQExeFile[MAX_PATH];
    LPITEMIDLIST	pidlStartMenu;
    char		szPath[MAX_PATH];
    char		path[2048];*path=0;
    CString		csPath, strIconFile;
    HRESULT		hres;
    IShellLink		*pShellLink;
    char		OldTargetLocation[MAX_PATH], TargetLocation[MAX_PATH];

    *path=0;
  
    sprintf(szFullyQExeFile, "%s\\ingres\\vdba\\ivm.exe", m_installPath);
  
    if (GetFileAttributes(szFullyQExeFile) == 0xFFFFFFFF)
	return (-1);

    GetShortPathName(theInstall.m_installPath, path, sizeof(path));
    csPath = path;
    strIconFile.Format("%s\\ingres\\vdba\\ivm.exe",csPath );
    sprintf(szFullyQExeFile, "\"%s\\ingres\\vdba\\ivm.exe\"", m_installPath);
    sprintf(TargetLocation, "%s\\ingres\\vdba\\ivm.exe", m_installPath);
  
    CoInitialize(NULL);

    /*
    ** Get the CSIDL for the start menu - this will be used to intialize
    ** the folder browser.
    */
    SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlStartMenu);

    /* get the path for the folder */
    SHGetPathFromIDList(pidlStartMenu, szPath);
    lstrcat(szPath, "\\Startup");

    /* Get a pointer to the IShellLink interface. */
    hres = CoCreateInstance(CLSID_ShellLink, 
			    NULL, 
			    CLSCTX_INPROC_SERVER, 
			    IID_IShellLink, 
			    (LPVOID*)&pShellLink); 

    if (SUCCEEDED(hres))
    {
	IPersistFile	*pPersistFile; 

	sprintf(szFullyQDisplayName,
		"%s\\Ingres Visual Manager %s.lnk", szPath,
		m_installationcode);

	/*
	** Query IShellLink for the IPersistFile interface for saving the 
	** shotcut in persistent storage. 
	*/
	hres = pShellLink->QueryInterface(IID_IPersistFile,
					  (LPVOID*)&pPersistFile); 

	if (SUCCEEDED(hres)) 
	{ 
	    /*
	    ** Search for the old IVM shortcut. If it exists, and it's
	    ** from the installation that we've just upgraded, remove it.
	    */
	    sprintf(szOldFullyQDisplayName,
		    "%s\\Ingres Visual Manager.lnk", szPath);

	    if (FileExists(szOldFullyQDisplayName))
	    {
		WCHAR	wsz[MAX_PATH]; 

		/* Ensure that the string is ANSI. */
		MultiByteToWideChar(CP_ACP,
				    0,
				    szOldFullyQDisplayName,
				    -1,
				    wsz,
				    MAX_PATH);

		hres = pPersistFile->Load(wsz, STGM_READ);
		if (SUCCEEDED(hres))
		{
		    /* Resolve the link. */
		    hres = pShellLink->Resolve(AfxGetMainWnd()->m_hWnd, 0);
		    if (SUCCEEDED(hres))
		    {
			/* Get the path to the link target. */
			hres = pShellLink->GetPath( OldTargetLocation,
						    MAX_PATH,
						    (WIN32_FIND_DATA *)&wsz,
						    SLGP_SHORTPATH);
			if (SUCCEEDED(hres))
			{
			    pPersistFile->Release();
			    if (strcmp(TargetLocation, OldTargetLocation) == 0)
				DeleteFile(szOldFullyQDisplayName);
			}
		    }
		    else
			pPersistFile->Release();
		}
	    }
	}

	pShellLink->Release(); 
    }

    CoUninitialize();

    return(0);
}

/*
**  Name:Create_OtherDBUsers 
**
**  Description:
**	Create evaluation releases specific database users.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
**	21-mar-2002 (penga03)
**	    Created a user orspo_runas for OpenROAD.
*/
BOOL 
CInstallation::Create_OtherDBUsers()
{
    BOOL		bret = TRUE;
    char		buf[1024];				   
    char		in_buf[1024];		
    HANDLE		newstdin, SaveStdin;
    PROCESS_INFORMATION	pi;
    SECURITY_ATTRIBUTES handle_security;
    STARTUPINFO 	si;
    CStdioFile outFile;

    AppendToLog(IDS_CREATEOTHERDBUSERS);

    sprintf(in_buf, "%s\\ingres\\temp\\createdbusers.sql", m_installPath);
    if(!outFile.Open(in_buf, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText, NULL))
	bret=FALSE;

    outFile.WriteString("create user portaldba with\n");
    outFile.WriteString("  nogroup,\n");
    outFile.WriteString("  privileges=(createdb),\n");
    outFile.WriteString("  default_privileges=(createdb),\n");
    outFile.WriteString("  noexpire_date,\n");
    outFile.WriteString("  noprofile,\n");
    outFile.WriteString("  nosecurity_audit;\n");
    outFile.WriteString("\\p\\g\n");

    outFile.WriteString("create user timregs with\n");
    outFile.WriteString("  nogroup,\n");
    outFile.WriteString("  privileges=(createdb),\n");
    outFile.WriteString("  default_privileges=(createdb),\n");
    outFile.WriteString("  noexpire_date,\n");
    outFile.WriteString("  noprofile,\n");
    outFile.WriteString("  nosecurity_audit;\n");
    outFile.WriteString("\\p\\g\n");

    outFile.WriteString("create user orspo_runas with\n");
    outFile.WriteString("  nogroup,\n");
    outFile.WriteString("  privileges=(createdb, trace, security, maintain_locations, operator, auditor, maintain_audit, maintain_users),\n");
    outFile.WriteString("  default_privileges=(createdb, trace, security, maintain_locations, operator, auditor, maintain_audit, maintain_users),\n");
    outFile.WriteString("  noexpire_date,\n");
    outFile.WriteString("  noprofile,\n");
    outFile.WriteString("  nosecurity_audit;\n");
    outFile.WriteString("\\p\\g\n");

    outFile.Close();

    handle_security.nLength = sizeof(handle_security);
    handle_security.lpSecurityDescriptor = NULL;
    handle_security.bInheritHandle = TRUE;
    memset((char*)&pi,0,sizeof(pi));
    memset((char*)&si,0,sizeof(si));
    si.cb = sizeof(si);

    DuplicateHandle(GetCurrentProcess(), GetStdHandle(STD_INPUT_HANDLE),
		    GetCurrentProcess(), &SaveStdin, 0, FALSE,
		    DUPLICATE_SAME_ACCESS);

    newstdin = CreateFile(in_buf, GENERIC_READ, FILE_SHARE_READ,
			  &handle_security, OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL, NULL);

    if ((newstdin == INVALID_HANDLE_VALUE) ||
	(SetStdHandle(STD_INPUT_HANDLE, newstdin) == FALSE))
    {
	bret = FALSE;
    }

    si.dwFlags |= STARTF_USESTDHANDLES;   
    si.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle (STD_ERROR_HANDLE);

    sprintf(buf, "\"%s\\ingres\\bin\\tm.exe\" -qSQL iidbdb",
	    m_installPath);

    if (CreateProcess(NULL, buf, NULL, NULL, TRUE,
			    DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
			    NULL, NULL, &si, &pi))
    {
	DWORD dw;

	WaitForSingleObject(pi.hProcess,INFINITE);
	if (GetExitCodeProcess(pi.hProcess,&dw))
	    bret = TRUE;
    }
    else
    {
	bret = FALSE;
    }

    CloseHandle(newstdin);
    DeleteFile(in_buf);
    return bret;
}

/*
**  Name:InstallPortal 
**
**  Description:
**	Silently install and config CleverPath Portal.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
**	21-mar-2002 (penga03)
**	    Install Java runtime environment under the directory 
**	    %II_SYSTEM%\JRE\1.3.1.
**	    Made some changes so that Portal can be used as a regular 
**	    web site without any logon form.
**	26-mar-2002 (penga03)
**	    Create a shortcut for Portal Demonstration Page under the 
**	    Advantge Ingres SDK group.
**	27-mar-2002 (penga03)
**	    Add "logout.href=http://[HOSTNAME]:[ICE_PORT]/AISDK/redirect
**	    /login.html" at the end of local.properties.
**	10-sep-2004 (penga03)
**	    Removed MDB.
*/
BOOL 
CInstallation::InstallPortal()
{
    BOOL bret=TRUE;
    char theFile[MAX_PATH], tmpFile[MAX_PATH], szBuf[MAX_PATH];
    char CurDir[MAX_PATH], NewDir[MAX_PATH], JREDir[MAX_PATH];
    char HostName[MAX_COMP_LEN];

    AppendComment(IDS_INSTALLPORTAL);
    AppendToLog(IDS_INSTALLPORTAL);

    /* install jave runtime */
    sprintf(JREDir, "%s\\JRE\\1.3.1", m_installPath);
    sprintf(szBuf, "%s\\bin\\java.dll", JREDir);
    if (GetFileAttributes(szBuf)==-1)
    {
	CreateDirectoryRecursively(JREDir);
	sprintf(theFile, "%s\\ingres\\temp\\jre1_3_1.exe", m_installPath);
	sprintf(tmpFile, "%s\\jre1_3_1.exe", JREDir);
	CopyFile(theFile, tmpFile, FALSE);

	GetCurrentDirectory(sizeof(CurDir), CurDir);
	SetCurrentDirectory(JREDir);
	AppendToLog("jre1_3_1.exe");
	Execute("jre1_3_1.exe", TRUE);
	SetCurrentDirectory(CurDir);
    }

    /* prepare default.properties and portal.rsp */
    sprintf(theFile, "%s\\ingres\\temp\\default.properties", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\temp\\default.tmp", m_installPath);

    if (GetHostName(HostName))
	UpdateFile("%HOST_NAME%", HostName, theFile, tmpFile);
    else
	UpdateFile("%HOST_NAME%", m_computerName, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    if (GetHostName(HostName))
	UpdateFile("%MAIL_HOST%", HostName, theFile, tmpFile);
    else
	UpdateFile("%MAIL_HOST%", m_computerName, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    if (GetHostName(HostName))
	sprintf(szBuf, "portal@%s", HostName);
    else
	sprintf(szBuf, "portal@%s", m_computerName);
    UpdateFile("%SUPPORT_EMAIL%", szBuf, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    if (GetHostName(HostName))
    {
	sprintf(szBuf,
		"jdbc:edbc://%s:SD7/portaldb;auto=multi;cursor=readonly",
		HostName);
    }
    else
    {
	sprintf(szBuf,
		"jdbc:edbc://%s:SD7/portaldb;auto=multi;cursor=readonly",
		m_computerName);
    }
    UpdateFile("%JDBC_URL%", szBuf, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    sprintf(theFile, "%s\\ingres\\temp\\portal.rsp", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\temp\\portal.tmp", m_installPath);

    sprintf(szBuf, "\"%s\\PortalSDK\"", m_installPath);
    UpdateFile("-P PortalProduct.installLocation", szBuf, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    /* 
    ** run %CDIMAGE%\\CleverPath Portal\\portalsdk java  
    ** -Ddefault.properties=%II_SYSTEM\\ingres\\temp\\default.properties 
    ** -cp portalsetup.jar run -options %II_SYSTEM%\\ingres\\temp\\portal.rsp
    */
    GetCurrentDirectory(sizeof(CurDir), CurDir);
    sprintf(NewDir, "%s\\CleverPath Portal\\portal", m_InstallSource);
    SetCurrentDirectory(NewDir);
    sprintf(szBuf, "\"%s\\bin\\java\" -Ddefault.properties=\"%s\\ingres\\temp\\default.properties\" -cp setup.jar run -options \"%s\\ingres\\temp\\portal.rsp\"", JREDir, m_installPath, m_installPath);
    AppendToLog(szBuf);
    Execute(szBuf, TRUE);
    SetCurrentDirectory(CurDir);

    /* copy edbc.jar */
    sprintf(tmpFile, "%s\\ingres\\lib\\edbc.jar", m_installPath);
    sprintf(theFile, "%s\\PortalSDK\\jakarta-tomcat-3.3a\\lib\\common\\edbc.jar", m_installPath);
    CopyFile(tmpFile, theFile, FALSE);

    /* modify local.properties */
    sprintf(theFile, "%s\\PortalSDK\\properties\\local.properties",
	    m_installPath);
    sprintf(tmpFile, "%s\\PortalSDK\\properties\\local.tmp", m_installPath);
    UpdateFile("num.workplace.shortcut.pages", "5", theFile, tmpFile, "", TRUE);
    CopyFile(tmpFile, theFile, FALSE);
    DeleteFile(tmpFile);

    /* run portal.caz */
    CString s;
	
    s.Format("-u -c\"%s\\PortalSDK\" \"%s\\CleverPath Portal\\portal.caz\"", 
             m_installPath, theInstall.m_InstallSource);
    Exec(m_installPath + "\\ingres\\bin\\cazipxp.exe", s);

    /* 
    ** modify [HOSTNAME], [ICE_PORT] in frameset.acs and 
    ** [PORTAL_PORT] in server.xml
    */
    CString PortNum;

    sprintf(theFile, "%s\\PortalSDK\\content\\templates\\0\\frameset.acs", m_installPath);
    sprintf(tmpFile, "%s\\PortalSDK\\content\\templates\\0\\frameset.tmp", m_installPath);

    if(GetHostName(HostName))
	ReplaceString("[HOSTNAME]", HostName, theFile, tmpFile);
    else
	ReplaceString("[HOSTNAME]", m_computerName, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    PortNum.Format("%d", m_IcePortNumber);
    ReplaceString("[ICE_PORT]", PortNum, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    sprintf(theFile, "%s\\PortalSDK\\conf\\server.xml", m_installPath);
    sprintf(tmpFile, "%s\\PortalSDK\\conf\\server.tmp", m_installPath);

    PortNum.Format("%d", m_PortalHostPort);
    ReplaceString("[PORTAL_PORT]", PortNum, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    /* clean up */
    sprintf(szBuf, "%s\\ingres\\temp\\default.properties", m_installPath);
    DeleteFile(szBuf);
    sprintf(szBuf, "%s\\ingres\\temp\\portal.rsp", m_installPath);
    DeleteFile(szBuf);
    sprintf(szBuf, "%s\\ingres\\temp\\jre1_3_1.exe", m_installPath);
    DeleteFile(szBuf);

    /* create a shortcut for Portal demo page */
    CString csExe, csWorkDir, csParams;
    HKEY hRegKey=0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
       "SOFTWARE\\Microsoft\\IE Setup\\Setup", 
       0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "Path", NULL, NULL, (BYTE *)szData, 
	                   &dwSize)==ERROR_SUCCESS)
	{
	    if (szData[strlen(szData)-1]=='\\') 
		szData[strlen(szData)-1]='\0';

	    csWorkDir.Format("%s", szData);
	    if(csWorkDir.Find("%programfiles%") != -1)
	    {
		GetEnvironmentVariable("programfiles", szBuf, sizeof(szBuf));
		csWorkDir.Replace("%programfiles%", szBuf);
	    }	
	}
	RegCloseKey(hRegKey);
    }

    if(!csWorkDir.IsEmpty())
	csExe.Format("%s\\IEXPLORE.EXE", csWorkDir);
    else
	csExe.Format("IEXPLORE.EXE", csWorkDir);

    if (GetHostName(HostName))
   	csParams.Format("http://%s:%d/servlet/portal/?PORTAL_UN=guest&PORTAL_PW=guest", HostName, m_PortalHostPort);
    else
	csParams.Format("http://%s:%d/servlet/portal/?PORTAL_UN=guest&PORTAL_PW=guest", m_computerName, m_PortalHostPort);

    CreateOneShortcut("Computer Associates\\Ingres SDK", 
                      "CleverPath Portal Demonstration Page", 
                      csExe, csParams, csWorkDir);

    AppendComment(IDS_DONE);
    return bret;
}

/*
**  Name:StartPortal 
**
**  Description:
**	Start Portal.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
**	26-mar-2002 (penga03)
**	    After launch portal.bat, sleep 40 seconds to wait 
**	    for Portal starting up and importing data.
*/
BOOL 
CInstallation::StartPortal()
{
    BOOL bret=TRUE;
    char CurDir[MAX_PATH], NewDir[MAX_PATH];
    CString s;
    BOOL PortalStarted=FALSE;

    AppendComment(IDS_STARTPORTAL);
    AppendToLog(IDS_STARTPORTAL);

    if(StartServer(FALSE))
    {
	/* Start Portal for the first time, which takes the longest. */
	GetCurrentDirectory(sizeof(CurDir), CurDir);
	sprintf(NewDir, "%s\\PortalSDK", m_installPath);
	SetCurrentDirectory(NewDir);
	AppendToLog("portal.bat");
	Execute("portal.bat", FALSE, TRUE);
	SetCurrentDirectory(CurDir);

	Sleep(40000);
    }

    AppendComment(IDS_DONE);
    return bret;
}

/*
**  Name:InstallForestAndTrees 
**
**  Description:
**	Silently install Forest & Trees.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::InstallForestAndTrees()
{
    BOOL bret=TRUE;
    char RegKey[1024], issFile[MAX_PATH], szBuf[MAX_PATH];
    HKEY hRegKey;
    char theFile[MAX_PATH], tmpFile[MAX_PATH], HostName[MAX_COMP_LEN];
    CString PortNum;

    AppendComment(IDS_INSTALLFTREES);
    AppendToLog(IDS_INSTALLFTREES);
	
    /* prepare ftsetup.iss */	
    sprintf(issFile, "%s\\ingres\\temp\\ftsetup.iss", m_installPath);

    if(m_win95)
	strcpy(RegKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
    else
	strcpy(RegKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
	char szData[128];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "RegisteredOwner", NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	    WritePrivateProfileString("SdRegisterUser-0", "szName", szData, issFile);
	if(RegQueryValueEx(hRegKey, "RegisteredOrganization", NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	    WritePrivateProfileString("SdRegisterUser-0", "szCompany", szData, issFile);
    }

    sprintf(szBuf, "%s\\Forest And Trees RTL", m_installPath);
    WritePrivateProfileString("SdAskDestPath-0", "szDir", szBuf, issFile);

    /* 
    ** run %CDIMAGE%\\Forest And Trees RTL\\setup.exe /s /SMS 
    ** /f1"%II_SYSTEM%\\ingres\\temp\\ftsetup.iss 
    */
    sprintf(szBuf, "\"%s\\Forest And Trees RTL\\setup.exe\" /s /SMS /f1\"%s\"", m_InstallSource, issFile);
    AppendToLog(szBuf);
    Execute(szBuf, TRUE);
    
    /* 
    ** modify [HOSTNAME] and [ICE_PORT] in 
    ** %II_SYSTEM%\ingres\ice\HTTPArea\AISDK\forest-trees\forest-trees.html 
    */	
    sprintf(theFile, "%s\\ingres\\ice\\HTTPArea\\AISDK\\forest-trees\\forest-trees.html", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\ice\\HTTPArea\\AISDK\\forest-trees\\forest-trees.tmp", m_installPath);

    if(GetHostName(HostName))
	ReplaceString("[HOSTNAME]", HostName, theFile, tmpFile);
    else
	ReplaceString("[HOSTNAME]", m_computerName, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    PortNum.Format("%d", m_IcePortNumber);
    ReplaceString("[ICE_PORT]", PortNum, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    /* clean up */
    sprintf(szBuf, "%s\\ingres\\temp\\ftsetup.iss", m_installPath);
    DeleteFile(szBuf);

    AppendComment(IDS_DONE);
    return bret;
}

/*
**  Name:InstallOpenROAD 
**
**  Description:
**	Silently install OpenROAD.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::InstallOpenROAD()
{
    BOOL bret=TRUE;
    char szBuf[MAX_PATH];
    CString s;

    AppendComment(IDS_INSTALLOPENROAD);
    AppendToLog(IDS_INSTALLOPENROAD);

    s.Format("II_LOG \"%s\"", (LPCSTR) m_temp);
    Exec("ingsetenv.exe",s);

    /*
    ** run %CDIMAGE%\\OpenROAD\\Setup.exe -s 
    */
    sprintf(szBuf, "\"%s\\OpenROAD\\Setup.exe\" -s -f2\"%s\\orsetup.log\"", m_InstallSource, m_temp);
    Execute(szBuf, TRUE);

    AppendComment(IDS_DONE);
    return bret;
}

/*
**  Name:InstallApache 
**
**  Description:
**	Silently install Apache HTTP Server.
**
**  History:
**	12-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::InstallApache()
{
    char RegKey[1024];
    HKEY hRegKey;
    char ApachePath[MAX_PATH], szBuf[MAX_PATH], szBuf2[MAX_PATH];

    m_ApacheInstalled = FALSE;
    strcpy(RegKey, "SOFTWARE\\Apache Group\\Apache\\1.3.23");
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKey, 0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
    /* Apache was already installed as a service */
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "ServerRoot", NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	{
	    m_ApacheInstalled=TRUE;
	    sprintf(szBuf, "%sconf\\httpd.conf", szData);
	    sprintf(szBuf2, "%sconf\\httpd.conf.bak", szData);
	    CopyFile(szBuf, szBuf2, FALSE);
	    RegCloseKey(hRegKey);
	    return TRUE;
	}
	RegCloseKey(hRegKey);
    }

    if(RegOpenKeyEx(HKEY_CURRENT_USER, RegKey, 0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
    /* Apache was already installed, but only for the current user */
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "ServerRoot", NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	{
	    m_ApacheInstalled=TRUE;
	    sprintf(szBuf, "%sconf\\httpd.conf", szData);
	    sprintf(szBuf2, "%sconf\\httpd.conf.bak", szData);
	    CopyFile(szBuf, szBuf2, FALSE);
	    RegCloseKey(hRegKey);
	    return TRUE;
	}
	RegCloseKey(hRegKey);
    }

    /* 
    ** run MsiExec.exe /i %CDIMAGE%\\Apache HTTP Server\\apache_1.3.23-win32-x86-no_src.msi 
    ** /qn INSTALLDIR="%II_SYSTEM%" SERVERDOMAIN="localhost" SERVERNAME="localhost" 
    ** SERVERADMIN="www@localhost"
    */
    AppendComment(IDS_INSTALLAPACHE);
    AppendToLog(IDS_INSTALLAPACHE);
    sprintf(szBuf, "MsiExec.exe /i \"%s\\Apache HTTP Server\\apache_1.3.23-win32-x86-no_src.msi\" /qn INSTALLDIR=\"%s\" SERVERDOMAIN=\"localhost\" SERVERNAME=\"localhost\" SERVERADMIN=\"www@localhost\"", m_InstallSource, m_installPath);
    AppendToLog(szBuf);
    Execute(szBuf, TRUE);
    AppendComment(IDS_DONE);

    if(RegOpenKeyEx(HKEY_CURRENT_USER, RegKey, 0, KEY_ALL_ACCESS, &hRegKey)==ERROR_SUCCESS)
    {
	RegSetValueEx(hRegKey, "InstalledBy", 0, REG_SZ, (BYTE *)"CA-IngresSDK", strlen("CA-IngresSDK")+1);
	RegCloseKey(hRegKey);
    }
    sprintf(ApachePath, "%s\\Apache", m_installPath);

    /* modify httpd.conf */
    FILE *inFile;
    CString t(m_installPath); t.Replace("\\", "/");

    sprintf(szBuf, "\"%s\\Apache.exe\" -k stop", ApachePath);
    AppendToLog(szBuf);
    Execute(szBuf, TRUE);

    sprintf(szBuf, "%s\\conf\\httpd.conf", ApachePath);
    if( (inFile=fopen(szBuf, "a+" )) != NULL )
    {
	fprintf(inFile, "\nInclude \"%s/ingres/ice/bin/apache/ice.conf\"", t);
	fclose(inFile);
    }

    return TRUE;
}


/*
**  Name:StartApache 
**
**  Description:
**	Start Apache HTTP Server.
**
**  History:
**	12-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::StartApache()
{
    char RegKey[1024];
    HKEY hRegKey;
    char cmd[MAX_PATH];

    if (m_ApacheInstalled)
	return TRUE;

    AppendToLog(IDS_STARTAPACHE);
    strcpy(RegKey, "SOFTWARE\\Apache Group\\Apache\\1.3.23");
    if(RegOpenKeyEx(HKEY_CURRENT_USER, RegKey, 0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "ServerRoot", NULL, NULL, (BYTE *)szData, &dwSize)==ERROR_SUCCESS)
	{
	    if (szData[strlen(szData)-1]=='\\') 
		szData[strlen(szData)-1]='\0';
	    sprintf(cmd, "\"%s\\Apache.exe\" -k start", szData);
	    AppendToLog(cmd);
	    Execute(cmd, FALSE, TRUE);
	    RegCloseKey(hRegKey);
	    return TRUE;
	}
	RegCloseKey(hRegKey);
    }
    return FALSE;
}

/*
**  Name:UpdateFile 
**
**  Description:
**	Update the specified field of a file.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::UpdateFile(CString KeyName, CString Value, CString FileName, CString TempFile, CString Symbol/*=""*/, BOOL Append/*=FALSE*/)
{
    CStdioFile inFile, outFile;
    int iPos;
    CString in, out;
    BOOL Found=FALSE;

    if(!inFile.Open(FileName, CFile::modeRead|CFile::shareExclusive|CFile::typeText, NULL))
	return FALSE;
    if(!outFile.Open(TempFile, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText, NULL))
	return FALSE;

    while(inFile.ReadString(in) != FALSE)
    {
	iPos=in.Find(KeyName);
	if(iPos>=0 && in.GetAt(iPos+KeyName.GetLength())=='=' && !Found)
	{
	    Found=TRUE;
	    if(Symbol.IsEmpty())
		out=KeyName+"="+Value;
	    else
	    {
		in.TrimRight();
		out=in+Symbol+Value;
	    }
	    outFile.WriteString(out+"\n");
	}
	else 
	    outFile.WriteString(in+"\n");
    } /* end of while */

    if(Append)
    {
	out=KeyName+"="+Value;
	outFile.WriteString(out+"\n");
    }

    inFile.Close();
    outFile.Close();
    return TRUE;
}

/*
**  Name:ReplaceString
**
**  Description:
**	Replace all the occurrences of String1 with String2.
**
**  History:
**	12-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::ReplaceString(CString String1, CString String2, CString FileName, CString TempFile)
{
    CStdioFile inFile, outFile;
    int iPos;
    CString s;

    if(!inFile.Open(FileName, CFile::modeRead|CFile::shareExclusive|CFile::typeText, NULL))
	return FALSE;
    if(!outFile.Open(TempFile, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText, NULL))
	return FALSE;

    while(inFile.ReadString(s) != FALSE)
    {
	iPos=s.Find(String1);
	if (iPos>=0)
	{
	    s.Replace(String1, String2);
	}
	outFile.WriteString(s+"\n");
    } /* end of while */

    return TRUE;
}

/*
**  Name:OpenPortal 
**
**  Description:
**	Open the Portal Login page.
**
**  History:
**	06-mar-2002 (penga03)
**	    Created.
**	26-mar-2002 (penga03)
**	    Get the install path of Internet Explore from the 
**	    registry.
**	27-mar-2002 (penga03)
**	    While opening the Portal Demo Page, bring the window to the 
**	    top of the Z order.
*/
void 
CInstallation::OpenPortal()
{
    char szBuf[MAX_PATH];
    char HostName[MAX_COMP_LEN];
    HKEY hRegKey=0;
    CString s;
    PROCESS_INFORMATION	pi;
    STARTUPINFO		si;

    AppendComment(IDS_OPENPORTAL);
    AppendToLog(IDS_OPENPORTAL);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
       "SOFTWARE\\Microsoft\\IE Setup\\Setup", 
       0, KEY_QUERY_VALUE, &hRegKey)==ERROR_SUCCESS)
    {
	char szData[MAX_PATH];
	DWORD dwSize=sizeof(szData);

	if(RegQueryValueEx(hRegKey, "Path", NULL, NULL, (BYTE *)szData, 
	                   &dwSize)==ERROR_SUCCESS)
	{
	    if (szData[strlen(szData)-1]=='\\') 
		szData[strlen(szData)-1]='\0';

	    s.Format("%s", szData);
	    if(s.Find("%programfiles%") != -1)
	    {
		GetEnvironmentVariable("programfiles", szBuf, sizeof(szBuf));
		s.Replace("%programfiles%", szBuf);
	    }
	}
	RegCloseKey(hRegKey);
    }

    if (GetHostName(HostName))
	sprintf(szBuf, "\"%s\\IEXPLORE.EXE\" http://%s:%d/servlet/portal/?PORTAL_UN=guest&PORTAL_PW=guest", 
	        s, HostName, m_PortalHostPort);
    else
	sprintf(szBuf, "\"%s\\IEXPLORE.EXE\" http://%s:%d/servlet/portal/?PORTAL_UN=guest&PORTAL_PW=guest", 
	        s, m_computerName, m_PortalHostPort);

    memset((char*)&pi, 0, sizeof(pi)); 
    memset((char*)&si, 0, sizeof(si)); 
    si.cb = sizeof(si);
    si.dwFlags=STARTF_USESHOWWINDOW;
    si.wShowWindow=SW_SHOWMAXIMIZED;

    if(CreateProcess(NULL, szBuf, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, 
                     NULL, NULL, &si, &pi))
    {
	while (FindWindow(NULL, 
	       "Computer Associates - CleverPath Portal - Microsoft Internet Explorer"))
	    Sleep(1000);
    }

    AppendComment(IDS_DONE);
}

/*
**  Name:AddOSUser 
**
**  Description:
**	Creates a local user account with the given userid, password. If 
**	the account already exists, it will just reset the password.
**
**  History:
**	14-mar-2002 (penga03)
**	    Created.
*/
DWORD 
CInstallation::AddOSUser(char *Username, char *Password, char *Comment)
{
    WCHAR wUsername[MAX_PATH], wPassword[MAX_PATH], wComment[MAX_PATH];
    USER_INFO_1    ui = {0};
    DWORD          dwLevel = 1; // (dwLevel=1 means USER_INFO_1 is passed)
    NET_API_STATUS nStatus;
    CString        csMsg;

    csMsg.Format(IDS_ADDOSUER, Username, Password);
    AppendToLog(csMsg);

    /* maps a character string to a wide-character (Unicode) string */
    MultiByteToWideChar(CP_ACP, 0, Username, -1, wUsername, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, Password, -1, wPassword, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, Comment, -1, wComment, MAX_PATH);

    ui.usri1_name        = wUsername;
    ui.usri1_password    = wPassword;
    ui.usri1_priv        = USER_PRIV_USER;
    ui.usri1_home_dir    = NULL;
    ui.usri1_comment     = wComment;
    ui.usri1_flags       = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;
    ui.usri1_script_path = NULL;

    nStatus = NetUserAdd(NULL, dwLevel, (LPBYTE)&ui, NULL);

    if (nStatus == NERR_Success)
	return nStatus;

    if (nStatus == NERR_UserExists)
    {
	USER_INFO_1003 ui;
	DWORD          dwLevel = 1003; // (Now we pass USER_INFO_1003 struct)
	
	ui.usri1003_password = wPassword;
	nStatus = NetUserSetInfo(NULL, wUsername, dwLevel, (LPBYTE)&ui, NULL);
    }

    return nStatus;
}

/*
**  Name:CheckSocket 
**
**  History:
**	20-mar-2002 (somsa01)
**	    Created.
*/
VOID 
CInstallation::CheckSocket(u_short *port)
{
    WORD version;
    SOCKET sd;
    struct sockaddr_in ws_addr;
    WSADATA startup_data;

    version=MAKEWORD(1,1);
    if(WSAStartup(version, &startup_data)==0)
    {
	if((sd=socket(AF_INET, SOCK_STREAM, 0))!=INVALID_SOCKET)
	{
	    int i=1;

	    ws_addr.sin_family = AF_INET;
	    ws_addr.sin_addr.s_addr = INADDR_ANY;
	    ws_addr.sin_port = *port;

	    if(bind(sd, (struct sockaddr *)&ws_addr, sizeof(struct sockaddr_in))!=0)
	    {
		if(WSAGetLastError()==WSAEADDRINUSE)
		{
		    ws_addr.sin_port=0;
		    if(bind(sd, (struct sockaddr *)&ws_addr, sizeof(struct sockaddr_in))==0)
		    {
			i=sizeof(struct sockaddr_in);
			getsockname(sd, (struct sockaddr *)&ws_addr, &i);
			*port=ws_addr.sin_port ;
		    }
		}
	    }

	    i=1;
	    setsockopt(sd, SOL_SOCKET, SO_DONTLINGER, (const char *)&i, sizeof(i));
	    closesocket(sd);
	}

	WSACleanup();
    }
}

/*
**  Name:CheckOnePortNumber 
**
**  History:
**	20-mar-2002 (penga03)
**	    Created.
*/
VOID 
CInstallation::CheckOnePortNumber(CString KeyName, CString FileName, CString TempFile)
{
    CStdioFile inFile, outFile;
    int iPos;
    CString in, out;
    u_short port1, port2;

    if(!inFile.Open(FileName, CFile::modeRead|CFile::shareExclusive|CFile::typeText, NULL))
	return;
    if(!outFile.Open(TempFile, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeText, NULL))
	return;

    while((inFile.ReadString(in)!=FALSE))
    {
	in.TrimRight();
	iPos=in.Find(KeyName);
	if(iPos>=0 && 
	  (in.GetAt(iPos+KeyName.GetLength())=='=' || 
	   in.GetAt(iPos+KeyName.GetLength())==' '))
	{
	    char port[32];
	    int num1, num2;
		
	    num1=in.GetLength()-(iPos+KeyName.GetLength()+1);
	    sprintf(port, "%s", in.Right(num1));
	    port2=port1=(u_short)atoi(port);
	    CheckSocket(&port1);
	    if(port2!=port1)
	    {
		sprintf(port, "%d", port1);
		num2=in.GetLength()-num1;
		out=in.Left(num2)+CString(port);
		outFile.WriteString(out+"\n");
	    }
	    else
		outFile.WriteString(in+"\n");
	}
	else 
	    outFile.WriteString(in+"\n");
    } /* end of while */

    inFile.Close();
    outFile.Close();
}

/*
**  Name:GetPortNumber 
**
**  History:
**	20-mar-2002 (penga03)
**	    Created.
*/
BOOL 
CInstallation::GetPortNumber(CString KeyName, CString &PortNumber, CString FileName)
{
    CStdioFile inFile;
    int iPos;
    CString in;

    if(!inFile.Open(FileName, CFile::modeRead|CFile::shareExclusive|CFile::typeText, NULL))
	return FALSE;

    while((inFile.ReadString(in)!=FALSE))
    {
	in.TrimRight();
	iPos=in.Find(KeyName);
	if(iPos>=0 && 
	  (in.GetAt(iPos+KeyName.GetLength())=='=' || 
	   in.GetAt(iPos+KeyName.GetLength())==' '))
	{
	    int num1;
		
	    num1=in.GetLength()-(iPos+KeyName.GetLength()+1);
	    PortNumber=in.Right(num1);
	    return TRUE;
	}
    } /* end of while */

    inFile.Close();
    return FALSE;
}

/*
**  Name:CheckPortNumbers 
**
**  Description:
**  Check port numbers used by Ingres/ICE, OpenROAD and Portal.
**
**  History:
**	21-mar-2002 (penga03)
**	    Created.
*/
VOID 
CInstallation::CheckPortNumbers()
{
    char theFile[MAX_PATH], tmpFile[MAX_PATH];
    CString s;
    
    sprintf(theFile, "%s\\ingres\\ice\\bin\\apache\\ice.conf", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\ice\\bin\\apache\\ice.tmp", m_installPath);

    CheckSocket(&m_IcePortNumber);
    s.Format("Listen %d", m_IcePortNumber);
    ReplaceString("Listen [ICE_PORT]", s, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    s.Format("<VirtualHost *:%d>", m_IcePortNumber);
    ReplaceString("<VirtualHost *:[ICE_PORT]>", s, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    CheckSocket(&m_OrPortNumber);
    s.Format("Listen %d", m_OrPortNumber);
    ReplaceString("Listen [OR_PORT]", s, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    s.Format("<VirtualHost *:%d>", m_OrPortNumber);
    ReplaceString("<VirtualHost *:[OR_PORT]>", s, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    sprintf(theFile, "%s\\ingres\\temp\\default.properties", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\temp\\default.tmp", m_installPath);
    CheckOnePortNumber("_PORT%", theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    GetPortNumber("%HOST_PORT%", s, theFile);
    m_PortalHostPort=atoi(s);
}

/*
**  Name: GetHostName 
**
**  History:
**	21-mar-2002 (somsa01)
**	    Created.
*/
BOOL 
CInstallation::GetHostName(char *Hostname)
{
    BOOL		status = TRUE;
    WORD		wVersion = MAKEWORD(2, 0);
    WSADATA		wsadata;
    HOSTENT		*pHostbyname;
    HOSTENT		*pHostbyaddr;

    if (WSAStartup (wVersion, &wsadata) == 0)
    {
	if (gethostname (Hostname, MAX_COMP_LEN) == 0)
	{
	    if ((pHostbyname = gethostbyname(Hostname)) != NULL)
	    {
		char	*ptr;
		char	*pHost1 = NULL;
		char	*pHost2 = NULL;
		
		pHost1 = (char *)malloc(strlen(pHostbyname->h_name) + 1);
		strcpy(pHost1, pHostbyname->h_name);
		if ((ptr = strchr(pHost1, '.')) != NULL)
		{
		    *ptr = '\0';
		}
		if (!_stricmp(Hostname, pHost1))
		{
		    if ((pHostbyaddr = gethostbyaddr(*pHostbyname->h_addr_list, 
		                       pHostbyname->h_addrtype, 
		                       pHostbyname->h_length)) != NULL)
		    {
			pHost2 = (char *)malloc(strlen(pHostbyaddr->h_name) + 1);
			strcpy(pHost2, pHostbyaddr->h_name);
			if ((ptr = strchr(pHost2, '.')) != NULL)
			{
			    *ptr = '\0';
			}
			if (_stricmp(pHost1, pHost2))
			{
			    /*
			    ** Ambiguous host name! But, it is what will
			    ** work.
			    */
			    status = TRUE;
			}

			free(pHost2);
		    }
		}
		else
		{
		    /*
		    ** Ambiguous host name!
		    */
		    status = FALSE;
		}

		free(pHost1);
	    }
	}
	else
	{
	    status = FALSE;
	}
	WSACleanup();
    }

    return (status);
}

/*
**  Name:CreateOneShortcut 
**
**  History:
**	26-mar-2002 (penga03)
**	    Created.
*/
int 
CInstallation::CreateOneShortcut(CString Folder, CString DisplayName, 
               CString ExeFile, CString Params, CString WorkDir)
{

    LPITEMIDLIST pidlStartMenu;
    char szPath[MAX_PATH], szLinkFile[MAX_PATH];
    HRESULT hres; 
    IShellLink *pShellLink;

    CoInitialize(NULL);

    SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlStartMenu);
    SHGetPathFromIDList(pidlStartMenu, szPath);
    lstrcat(szPath, "\\");
    lstrcat(szPath, Folder);

    /*Get a pointer to the IShellLink interface.*/
    hres=CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                          IID_IShellLink, (LPVOID *)&pShellLink);

    if(SUCCEEDED(hres))
    {
	IPersistFile *pPersistFile;
	
	sprintf(szLinkFile, "%s\\%s.lnk", szPath, DisplayName);
	
	/*Set the path to the shortcut target.*/
	hres=pShellLink->SetPath(ExeFile);
	hres=pShellLink->SetArguments(Params);
	hres=pShellLink->SetWorkingDirectory(WorkDir);

	/*
	Query IShellLink for the IPersistFile interface for saving the 
	shortcut in persistent storage. 
	*/
	hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
	if(SUCCEEDED(hres))
	{
	    WCHAR wsz[MAX_PATH];
		
	    /*Ensure that the string is Unicode.*/
	    MultiByteToWideChar(CP_ACP, 0, szLinkFile, -1, wsz, MAX_PATH);
	    /*Save the link by calling IPersistFile::Save.*/
	    hres=pPersistFile->Save(wsz, TRUE);
	    pPersistFile->Release();
	}
	pShellLink->Release();
    }

    CoUninitialize();
    return (0);
}

/*
**  Name:UpdateIngresBatchFiles 
**
**  History:
**	18-mar-2002 (penga03)
**	    Created. Update \\ingres\\bin\\setingenvs.bat, which 
**	    sets II_SYSTEM, path, codepage and title.
*/
void 
CInstallation::UpdateIngresBatchFiles()
{
    char theFile[MAX_PATH], tmpFile[MAX_PATH], szBuf[256];
    int total, i;
    HKEY hKey;

    sprintf(theFile, "%s\\ingres\\bin\\setingenvs.bat", m_installPath);
    sprintf(tmpFile, "%s\\ingres\\bin\\setingenvs.tmp", m_installPath);

    UpdateFile("SET II_SYSTEM", m_installPath, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    sprintf(szBuf, "%s > C:\\NUL", "1252");
    total=sizeof(charsets)/sizeof(ENTRY);
    for(i=0; i<total; i++)
    {
	if (!_stricmp(m_charset, charsets[i].key))
	{
	    sprintf(szBuf, "%s > C:\\NUL", charsets[i].value);
	    break;
	}
    }
    UpdateFile("MODE CON CP SELECT", szBuf, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

#ifdef EVALUATION_RELEASE
    strcpy(szBuf, "TITLE Ingres SDK");
#else
    sprintf(szBuf, "TITLE Ingres %s", m_installationcode);
#endif
    ReplaceString("TITLE Ingres", szBuf, theFile, tmpFile);
    CopyFile(tmpFile, theFile, FALSE );
    DeleteFile(tmpFile);

    if(!RegCreateKeyEx (HKEY_CURRENT_USER, 
                       "Console\\Ingres Command Prompt", 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL, 
                        &hKey, 
                        NULL))
    {
	strcpy(szBuf, "Lucida Console");
	RegSetValueEx(hKey, "FaceName", 0, REG_SZ, (CONST BYTE *)szBuf, strlen(szBuf)+1);
	RegCloseKey(hKey);
    }

    return;
}


/*
**  Name:InstallJRE 
**
**  Description:
**	Silently install JRE.
**
**  History:
**	12-Feb-2004 (penga03)
**	    Created.
*/
int CInstallation::InstallJRE(void)
{
    char cmd[MAX_PATH];
    AppendComment(IDS_INSTALLJRE);
    AppendToLog(IDS_INSTALLJRE);

    /* install jave runtime */
    sprintf(cmd, "\"%s\\ingres\\temp\\j2re-1_4_2_02-windows-i586-p.exe\" /s /v\"/qn ADDLOCAL=ALL IEXPLORER=1 INSTALLDIR=\\\"%s\\ingres\\jre\\\"\"", m_installPath, m_installPath);
    Execute(cmd, TRUE);

    AppendComment(IDS_DONE);
    return 0;
}

BOOL 
NTGetEnvironmentVariable(LPCSTR pKeyValue,void *pData,DWORD sizeData)
{
    BOOL bret=FALSE;
    HKEY queryKey=0;	
    DWORD dwType;

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,szEnvRegKey,0,KEY_QUERY_VALUE, &queryKey ))
    {
	if (!RegQueryValueEx(queryKey, pKeyValue, NULL, &dwType, (BYTE*) pData, &sizeData )) 
	    bret=TRUE;
	RegCloseKey( queryKey );
    }
    return bret;
}

BOOL 
NTSetEnvironmentVariable(LPCSTR lpKey,LPCSTR lpValue)
{
    DWORD dwDisposition=0;
    HKEY hKeyItem = 0;
    BOOL bret=FALSE;

    if (!RegCreateKeyEx(HKEY_LOCAL_MACHINE,szEnvRegKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeyItem,&dwDisposition))
    {	
	if (lpValue && *lpValue)
	{
	    if (!RegSetValueEx(hKeyItem,lpKey,0,REG_EXPAND_SZ,(BYTE *)lpValue,lstrlen(lpValue)+1))
		bret=TRUE;
	}
	else
	{
	    if (!RegDeleteValue(hKeyItem,lpKey))
		bret=TRUE;
	}
	RegFlushKey(hKeyItem);
	RegCloseKey(hKeyItem);
    }
    return bret;
}

BOOL
RemoveValue(LPSTR oldValue,LPSTR newValue)
{
    char ach[4096];
    CStringList sl;
    BOOL removed = FALSE;
	
    *ach=0;
    while(oldValue)
    {
	LPSTR p=strstr(oldValue,SEPARATOR);
	if (p) *p=0;
	sl.AddTail(oldValue);
	oldValue=p ? p+1 : p;
    }
    for(POSITION pos=sl.GetHeadPosition();pos;)
    {
	CString &s=sl.GetNext(pos);
	if (s.CompareNoCase(newValue))
	{
	    strcat(ach,s);
	    strcat(ach,SEPARATOR);
	}
	else
	removed = TRUE;
    }
    if (*ach) 
	ach[strlen(ach)-1]=0;
    strcpy(newValue,ach);
    return (removed);
}

BOOL 
CInstallation::RemoveObsoleteEnvironmentVariables()
{
    for (int i=0;env_strings[i].key;i++)
    {
	char oldValue[4096];
	if (NTGetEnvironmentVariable(env_strings[i].key,oldValue,sizeof(oldValue)))
	{
	    char newValue[4096];
	    if (*(env_strings[i].value))
	    {
		wsprintf(newValue,env_strings[i].value, m_installPath);
		RemoveValue(oldValue,newValue);
		}
		else
		    *newValue=0;
		NTSetEnvironmentVariable(env_strings[i].key,newValue);
	}
    }
    ::SendMessage(::FindWindow("ProgMan", NULL), WM_WININICHANGE, 0L, (LPARAM)"Environment");
    return TRUE;
}

int 
CInstallation::InstallMDB()
{
    int bret=0;
    char szBuf[2048], CurDir[2048];
    CString csParams, csParam;
	
    GetCurrentDirectory(sizeof(CurDir), CurDir);
    SetCurrentDirectory(m_installPath);

    sprintf(szBuf, "%s\\mdb\\setupmdb.bat", m_InstallSource);

    csParams.Format ("-II_MDB_PATH=\"%s\\mdb\"", m_InstallSource);
    if (!m_MDBName.IsEmpty())
    {
	csParam.Format ("-II_MDB_NAME=%s", m_MDBName);
	csParams += " " + csParam;
    }
    if (!m_MDBSize.IsEmpty())
    {
	csParam.Format ("-II_MDB_SIZE=%s", m_MDBSize);
	csParams += " " + csParam;
    }
    if (m_bMDBDebug)
    {
	csParam.Format("-%s", "debug");
	csParams += " " + csParam;
    }
    //bret=Exec(szBuf, csParams, TRUE, FALSE);
    //CString cmd(szBuf);
    //AppendToLog(cmd+" "+csParams);


    SetCurrentDirectory(CurDir);
    return bret;
}

BOOL
LookupUserGroupFromRid(
					   LPWSTR TargetComputer,
					   DWORD Rid,
					   LPWSTR Name,
					   PDWORD cchName
					   )
{
	PUSER_MODALS_INFO_2 umi2;
	NET_API_STATUS nas;
	
	UCHAR SubAuthorityCount;
	PSID pSid;
	SID_NAME_USE snu;
	
	WCHAR DomainName[DNLEN+1];
	DWORD cchDomainName = DNLEN;
	BOOL bSuccess = FALSE; // assume failure
	
	nas = NetUserModalsGet(TargetComputer, 2, (LPBYTE *)&umi2);
	
	if(nas != NERR_Success) {
		SetLastError(nas);
		return FALSE;
	}
	
	SubAuthorityCount = *GetSidSubAuthorityCount
		(umi2->usrmod2_domain_id);
	

	pSid = (PSID)HeapAlloc(GetProcessHeap(), 0,
		GetSidLengthRequired((UCHAR)(SubAuthorityCount + 1)));
	
	if(pSid != NULL) {
		
		if(InitializeSid(
			pSid,
			GetSidIdentifierAuthority(umi2->usrmod2_domain_id),
			(BYTE)(SubAuthorityCount+1)
			)) {
			
			DWORD SubAuthIndex = 0;
			
		
			for( ; SubAuthIndex < SubAuthorityCount ; SubAuthIndex++) {
				*GetSidSubAuthority(pSid, SubAuthIndex) =
					*GetSidSubAuthority(umi2->usrmod2_domain_id,
					SubAuthIndex);
			}
			
			*GetSidSubAuthority(pSid, SubAuthorityCount) = Rid;
			
			bSuccess = LookupAccountSidW(
				TargetComputer,
				pSid,
				Name,
				cchName,
				DomainName,
				&cchDomainName,
				&snu
				);
		}
		
		HeapFree(GetProcessHeap(), 0, pSid);
	}
	
	NetApiBufferFree(umi2);
	
	return bSuccess;
}

BOOL 
CInstallation::GetLocalizedAdminName()
{
	WCHAR Name[UNLEN+1];
	DWORD cchName = UNLEN;
	
	if(!LookupUserGroupFromRid(
		NULL,
		DOMAIN_USER_RID_ADMIN,
		Name,
		&cchName
		)) {
		
		return FALSE;
	}
	m_adminName.Format("%ls", Name);
	return TRUE;
}

#define INGRESSVCNAME	L"Ingres_Database_"
#define DISKPROPNAME	L"Disk"
#define PHYSICALDISK	L"Physical Disk"
#define NETWORKNAME		L"Network Name"
#define HA_SUCCESS					0
#define HA_FAILURE					1
#define HA_INVALID_PARAMETER		2
#define HA_CLUSTER_NOT_RUNNING		3
#define HA_NO_DISK_RESOURCE_MATCH	4
#define HA_SERVICE_NOT_FOUND		5
BOOL AddedNetworkNameDep;
BOOL CheckServiceExists(LPSTR serviceName);
BOOL GetDriveLetterFromResource(HRESOURCE hResource, LPSTR driveLetter);
BOOL DefineProperties(HRESOURCE hResource, LPTSTR lpszResourceName);
BOOL DefineServiceParameters(HRESOURCE hResource, LPSTR lpszInstallCode, char *HostName);
BOOL DefineDependencies(HGROUP hGroup, HRESOURCE hIngresResource, LPSTR *sharedDiskList, INT sharedDiskCount);
BOOL SetupRegistryKeys(HRESOURCE hResource, LPSTR lpszInstallCode, BOOL bInstall);

unsigned int 
CInstallation::RegisterClusterService(LPSTR *sharedDiskList, INT sharedDiskCount, LPSTR lpszResourceName, LPSTR lpszInstallCode)
{
	HCLUSTER hCluster = NULL;
	HGROUP hGroup = NULL;
	HRESOURCE hResource = NULL;
	HCLUSENUM hClusEnum = NULL;
	HGROUPENUM hGroupEnum = NULL;
	BOOL bFoundDisk = FALSE;
	const int SIZE = 256;
	WCHAR lpwszGroupName[SIZE];
	WCHAR lpwszResourceType[SIZE];
	WCHAR lpwszResourceName[SIZE];
	WCHAR lpwszResName[SIZE];
	WCHAR lpwszNewResourceName[SIZE];
	CHAR lpszDiskName[SIZE];
	CHAR lpszSvcName[SIZE];
	CHAR HostName[MAX_COMP_LEN];
	
	DWORD dwIndex = 0;
	DWORD dwValueListSize = 0;
	DWORD dwGroupType = SIZE;
	DWORD dwGroupSize = SIZE;
	DWORD dwResourceType = SIZE;
	DWORD dwResourceSize = SIZE;
	unsigned int iError = HA_FAILURE;

	if(!sharedDiskList || !lpszInstallCode || strlen(lpszInstallCode)!=2)
		return HA_INVALID_PARAMETER;

	sprintf(lpszSvcName, "%S%s", INGRESSVCNAME, lpszInstallCode);
	if(!CheckServiceExists(lpszSvcName))
	{
		return HA_SERVICE_NOT_FOUND;
	}
	
	hCluster = OpenCluster(NULL);
	if(!hCluster)
		return HA_CLUSTER_NOT_RUNNING;
	
	memset(lpwszResName, '\0', sizeof(lpwszResName));
	mbstowcs(lpwszResName, lpszResourceName, sizeof(lpwszResName));
	hResource = OpenClusterResource(hCluster, lpwszResName);
	if (hResource)
	{
		CloseClusterResource(hResource);
		CloseCluster(hCluster);
		return HA_SUCCESS;
	}

	hClusEnum=ClusterOpenEnum(hCluster, CLUSTER_ENUM_RESOURCE);
	if(!hClusEnum)
	{
		CloseCluster(hCluster);
		return HA_FAILURE;
	}

	memset(lpwszGroupName, '\0', SIZE);
	memset(lpwszResourceName, '\0', SIZE);
	while(ERROR_SUCCESS == ClusterEnum(hClusEnum, dwIndex, &dwResourceType, lpwszResourceName, &dwResourceSize))
	{
		hResource = OpenClusterResource(hCluster, lpwszResourceName);
		if(hResource)
		{
			if(ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, NULL, NULL, lpwszResourceType, SIZE, NULL) == ERROR_SUCCESS)
			{
				if(wcscmp(lpwszResourceType, PHYSICALDISK) == 0)
				{
					if(GetDriveLetterFromResource(hResource, lpszDiskName))
					{
						if(_strnicmp(lpszDiskName, sharedDiskList[0], 2) == 0)
						{
							GetClusterResourceState(hResource, NULL, 0, lpwszGroupName, &dwGroupSize);
							break;
						}
					}
				}
			}
		}
		CloseClusterResource(hResource);
		memset(lpwszResourceName, '\0', SIZE);
		dwIndex++;
		dwResourceSize = SIZE;
	}

	ClusterCloseEnum(hClusEnum);

	if(wcslen(lpwszGroupName)>0 && wcslen(lpwszResourceName)>0)
	{
		hGroup = OpenClusterGroup(hCluster, lpwszGroupName);
		if(hGroup)
		{
			memset(lpwszNewResourceName, '\0', sizeof(lpwszNewResourceName));
			mbstowcs(lpwszNewResourceName, lpszResourceName, sizeof(lpwszNewResourceName));
			hResource = CreateClusterResource(hGroup, lpwszNewResourceName, L"Generic Service", 0);
			if(hResource)
			{
				AddedNetworkNameDep = FALSE;
				GetHostName(HostName);
				if (!DefineProperties(hResource, lpszResourceName) ||
					!DefineDependencies(hGroup, hResource, sharedDiskList, sharedDiskCount) ||
					!DefineServiceParameters(hResource, lpszInstallCode, HostName) ||
					!SetupRegistryKeys(hResource, lpszInstallCode, TRUE))
				{
					DeleteClusterResource(hResource);
				}
				else
				{
					iError = HA_SUCCESS;
				}
				CloseClusterResource(hResource);
			}
			CloseClusterGroup(hGroup);
		}
	}

	return iError;
}

LPVOID ClusDocEx_ResGetControlCodeOutput(HRESOURCE hResource,HNODE hNode,DWORD dwControlCode,LPDWORD lpcbResultSize)
{
    DWORD dwResult = ERROR_SUCCESS;
    DWORD cbBufSize = sizeof(DWORD);
    LPVOID lpOutBuffer = LocalAlloc(LPTR, cbBufSize);

    if(!lpOutBuffer)
    {
        dwResult = GetLastError();
        return NULL;
    }

	dwResult = ClusterResourceControl(hResource, hNode, dwControlCode, NULL, 0, lpOutBuffer, cbBufSize, lpcbResultSize);

	if(dwResult == ERROR_MORE_DATA)
    {
        LocalFree(lpOutBuffer);
        cbBufSize = *lpcbResultSize;
        lpOutBuffer = LocalAlloc(LPTR, cbBufSize);
        if(!lpOutBuffer)
    		return NULL;
    
		dwResult = ClusterResourceControl(hResource, hNode, dwControlCode, NULL, 0, lpOutBuffer, cbBufSize, lpcbResultSize);
    }

    if(dwResult != ERROR_SUCCESS)
    {
        LocalFree(lpOutBuffer);
        lpOutBuffer = NULL;
        *lpcbResultSize = 0;
    }

    return lpOutBuffer;
}

BOOL CheckServiceExists(LPSTR lpszServiceName)
{
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hCService = NULL;
	BOOL rc;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if(!hSCManager)	
		return FALSE;

	hCService = OpenService(hSCManager, lpszServiceName, GENERIC_READ);
	if(hCService)
	{
		rc = TRUE;
		CloseServiceHandle(hCService);
	}
	else
		rc = FALSE;

	CloseServiceHandle(hSCManager);

	return rc;
}

BOOL GetDriveLetterFromResource(HRESOURCE hResource, LPSTR driveLetter)
{
	LPVOID lpValueList = NULL;
	DWORD dwResult = 0;
	DWORD dwBytesReturned = 0;
	DWORD dwPosition = 0;
    DWORD dwOutBufferSize = 512;
	CLUSPROP_BUFFER_HELPER cbh;

	lpValueList = LocalAlloc(LPTR, dwOutBufferSize);
	if(!lpValueList)
		return FALSE;
	
	dwResult = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO, NULL, 0, lpValueList, dwOutBufferSize, &dwBytesReturned);
	if(dwResult == ERROR_MORE_DATA)
	{
		LocalFree(lpValueList);
		dwOutBufferSize = dwBytesReturned;
		lpValueList = LocalAlloc(LPTR, dwOutBufferSize);
		if(!lpValueList)
			return FALSE;
	
		dwResult = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO, NULL, 0, lpValueList, dwOutBufferSize, &dwBytesReturned);
	}
	
	if(dwResult != ERROR_SUCCESS)
		return FALSE;
	
	cbh.pb = (PBYTE)lpValueList;
	while(1)
	{
		if(cbh.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
		{
			if(wcslen(cbh.pPartitionInfoValue->szDeviceName) > 0)
			{
				memset(driveLetter, '\0', sizeof(driveLetter));
				wcstombs(driveLetter, cbh.pPartitionInfoValue->szDeviceName, sizeof(cbh.pPartitionInfoValue->szDeviceName));
			}
		}
		else if(cbh.pSyntax->dw == CLUSPROP_SYNTAX_ENDMARK)
			break;
	
		dwPosition = dwPosition + sizeof(CLUSPROP_VALUE) + ALIGN_CLUSPROP(cbh.pValue->cbLength);

		if((dwPosition + sizeof(DWORD)) > dwBytesReturned)
			break;
	
		cbh.pb = cbh.pb + sizeof(CLUSPROP_VALUE) + ALIGN_CLUSPROP(cbh.pValue->cbLength);
	}

	if(lpValueList)
		LocalFree(lpValueList);
	
	return TRUE;
}

BOOL DefineProperties(HRESOURCE hResource, LPTSTR lpszResourceName)
{
	DWORD dRet = ERROR_SUCCESS;
	WCHAR szPendingTimeout[] = L"PendingTimeout"; 

	struct _INGRES_PROPERTIES
	{
		DWORD nPropertyCount;
		CLUSPROP_PROPERTY_NAME_DECLARE(PendingTimeout, sizeof(szPendingTimeout) / sizeof(WCHAR));
		CLUSPROP_DWORD PendingTimeoutValue;
		CLUSPROP_SYNTAX EndPendingTimeout;
	} PropList;

	PropList.nPropertyCount = 1;
	PropList.PendingTimeout.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.PendingTimeout.cbLength = sizeof(szPendingTimeout);
	lstrcpyW(PropList.PendingTimeout.sz, szPendingTimeout);
	PropList.PendingTimeoutValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_DWORD;
	PropList.PendingTimeoutValue.cbLength = sizeof(DWORD);
	PropList.PendingTimeoutValue.dw = 300000;
	PropList.EndPendingTimeout.dw = CLUSPROP_SYNTAX_ENDMARK;

	LPVOID lpInBuffer = &PropList;
	DWORD cbInBufferSize = sizeof(PropList);
	
	dRet = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES, lpInBuffer, cbInBufferSize, NULL, 0, NULL);
	if(!(dRet == ERROR_SUCCESS || dRet == ERROR_RESOURCE_PROPERTIES_STORED))
		return FALSE;
	
	return TRUE;
}

BOOL DefineServiceParameters(HRESOURCE hResource, LPSTR lpszInstallCode, char *HostName)
{
	DWORD dRet = ERROR_SUCCESS;
	WCHAR szServiceName[] = L"ServiceName";
	WCHAR szServiceNameValue[] = L"Ingres_Database_II";
	WCHAR szStartupParameters[] = L"StartupParameters";
	WCHAR szStartupParametersValue[] = L"HAScluster";
	WCHAR szUseNetworkName[] = L"UseNetworkName";
	WCHAR wNetworkName[MAX_COMP_LEN];
	WCHAR szInstallCode[3];
	char cNetworkName[MAX_COMP_LEN];

	struct _INGRES_PROPERTIES
	{
		DWORD nPropertyCount;

		// ServiceName="Ingres_Database_II":string
        CLUSPROP_PROPERTY_NAME_DECLARE(ServiceName, sizeof(szServiceName) / sizeof(WCHAR));
		CLUSPROP_SZ_DECLARE(ServiceNameValue, sizeof(szServiceNameValue) / sizeof(WCHAR));
		CLUSPROP_SYNTAX EndServiceName;

		// StartupParameters="HAScluster":string 
		CLUSPROP_PROPERTY_NAME_DECLARE(StartupParameters, sizeof(szStartupParameters) / sizeof(WCHAR));
		CLUSPROP_SZ_DECLARE(StartupParametersValue, sizeof(szStartupParametersValue) / sizeof(WCHAR));
		CLUSPROP_SYNTAX EndStartupParameters;
		
		// UseNetworkName=1:DWORD
		CLUSPROP_PROPERTY_NAME_DECLARE(UseNetworkName, sizeof(szUseNetworkName) / sizeof(WCHAR));
		CLUSPROP_DWORD UseNetworkNameValue;
		CLUSPROP_SYNTAX EndUseNetworkName;
	} PropList;

	PropList.nPropertyCount = 2;
		
	if(AddedNetworkNameDep)
	{
		ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_GET_NETWORK_NAME, NULL, 0, &wNetworkName, sizeof(WCHAR)*MAX_COMP_LEN, NULL);
		memset(cNetworkName, 0, sizeof(char) * MAX_COMP_LEN);
		wcstombs(cNetworkName, wNetworkName, MAX_COMP_LEN);
		
		if(stricmp(HostName, cNetworkName)==0)
		{
			// UseNetworkName=1:DWORD
			PropList.UseNetworkName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
			PropList.UseNetworkName.cbLength = sizeof(szUseNetworkName);
			lstrcpyW(PropList.UseNetworkName.sz, szUseNetworkName);
			PropList.UseNetworkNameValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_DWORD;
			PropList.UseNetworkNameValue.cbLength = sizeof(DWORD);
			PropList.UseNetworkNameValue.dw = 1;
			PropList.EndUseNetworkName.dw = CLUSPROP_SYNTAX_ENDMARK;
			PropList.nPropertyCount = 3;
		}
	}

	// ServiceName="Ingres_Database_II":string
	lstrcpyW(szServiceNameValue, INGRESSVCNAME);
	mbstowcs(szInstallCode, lpszInstallCode, sizeof(lpszInstallCode));
	wcscat(szServiceNameValue, szInstallCode);
	PropList.ServiceName.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.ServiceName.cbLength = sizeof(szServiceName);
	lstrcpyW(PropList.ServiceName.sz, szServiceName);
	PropList.ServiceNameValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_SZ;
	PropList.ServiceNameValue.cbLength = sizeof(szServiceNameValue);
	lstrcpyW(PropList.ServiceNameValue.sz, szServiceNameValue);
	PropList.EndServiceName.dw = CLUSPROP_SYNTAX_ENDMARK;

	// StartupParameters="HAScluster":string 
	PropList.StartupParameters.Syntax.dw = CLUSPROP_SYNTAX_NAME;
	PropList.StartupParameters.cbLength = sizeof(szStartupParameters);
	lstrcpyW(PropList.StartupParameters.sz, szStartupParameters);
	PropList.StartupParametersValue.Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_SZ;
	PropList.StartupParametersValue.cbLength = sizeof(szStartupParametersValue);
	lstrcpyW(PropList.StartupParametersValue.sz, szStartupParametersValue);
	PropList.EndStartupParameters.dw = CLUSPROP_SYNTAX_ENDMARK;

	LPVOID	lpInBuffer = &PropList;
	DWORD	cbInBufferSize = sizeof(PropList);
	
	dRet = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES, lpInBuffer, cbInBufferSize, NULL, 0, NULL);

	if(!(dRet == ERROR_SUCCESS || dRet == ERROR_RESOURCE_PROPERTIES_STORED))
	{
		return FALSE;
	}
	
	return TRUE;
}

BOOL DefineDependencies(HGROUP hGroup, HRESOURCE hIngresResource, LPSTR *sharedDiskList, INT sharedDiskCount)
{
	HCLUSTER hCluster = NULL;
	HGROUPENUM hGroupEnum = NULL;
	HRESOURCE hResource = NULL;
	const int SIZE = 256;
	WCHAR lpwszResourceName[SIZE];
	WCHAR lpwszResourceType[SIZE];
	LPSTR lpszDiskName = new CHAR[SIZE];
	DWORD dwGroupType = SIZE;
	DWORD dwGroupSize = SIZE;
	DWORD dwResourceType = SIZE;
	DWORD dwResourceSize = SIZE;
	DWORD dwRet = 0;

	hCluster = OpenCluster(NULL);
	if(!hCluster)
	{
		return FALSE;
	}

	hGroupEnum = ClusterGroupOpenEnum(hGroup, CLUSTER_GROUP_ENUM_CONTAINS);
	if(!hGroupEnum)
	{
		return FALSE;
	}

	for(int resourceIndex=0; ; resourceIndex++)
	{
		memset(lpwszResourceName, '\0', sizeof(lpwszResourceName));

		if(ClusterGroupEnum(hGroupEnum, resourceIndex, &dwResourceType, lpwszResourceName, &dwResourceSize) == ERROR_NO_MORE_ITEMS)
			break;

		hResource = OpenClusterResource(hCluster, lpwszResourceName);
		if(!hResource)
		{
			continue;
		}

		if(ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, NULL, NULL, lpwszResourceType, SIZE, NULL) == ERROR_SUCCESS)
		{
			if(wcscmp(lpwszResourceType, PHYSICALDISK) == 0)
			{
				if(GetDriveLetterFromResource(hResource, lpszDiskName))
				{
					for(int SDindex=0; SDindex<sharedDiskCount; SDindex++)
					{
						if(_strnicmp(lpszDiskName, sharedDiskList[SDindex], 2) == 0)
						{
							dwRet = AddClusterResourceDependency(hIngresResource, hResource);
							if(!(dwRet==ERROR_SUCCESS || dwRet==ERROR_DEPENDENCY_ALREADY_EXISTS))
							{
								CloseClusterResource(hResource);
								ClusterGroupCloseEnum(hGroupEnum);
								return FALSE;
							}
							else
								break;
						}
					}
				}
			}
			else if(wcscmp(lpwszResourceType, NETWORKNAME) == 0)
			{
				dwRet = AddClusterResourceDependency(hIngresResource, hResource);
				if(!(dwRet==ERROR_SUCCESS || dwRet==ERROR_DEPENDENCY_ALREADY_EXISTS))
				{
					CloseClusterResource(hResource);
					ClusterGroupCloseEnum(hGroupEnum);
					return FALSE;
				}
				AddedNetworkNameDep = TRUE;
			}
		}

		CloseClusterResource(hResource);
		dwResourceSize = SIZE;
	}

	ClusterGroupCloseEnum(hGroupEnum);
	return TRUE;
}

BOOL SetupRegistryKeys(HRESOURCE hResource, LPSTR lpszInstallCode, BOOL bInstall)
{
	DWORD dRet = ERROR_SUCCESS;
    char SubKey[128];
	WCHAR szRegistryKey[128];
	LPVOID lpInBuffer = NULL;
	DWORD cbInBufferSize = 0;

	struct {
		char *szRegistryKey;
	} RegistryKeys[] = {
		"SOFTWARE\\Classes\\Ingres_Database_%s",
		"SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation"
	};

	for(int i=0; i<sizeof(RegistryKeys)/sizeof(RegistryKeys[0]); i++)
	{
		sprintf(SubKey, RegistryKeys[i].szRegistryKey, lpszInstallCode);
		mbstowcs(szRegistryKey, SubKey, sizeof(SubKey));
		lpInBuffer = (LPVOID)szRegistryKey;
		cbInBufferSize = (wcslen(szRegistryKey) + 1) * sizeof(WCHAR);

		if(bInstall)
		{
			dRet = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT, lpInBuffer, cbInBufferSize, NULL, 0, NULL);
			if (!(dRet == ERROR_SUCCESS || dRet == ERROR_ALREADY_EXISTS))
				return FALSE;
		}
		else
		{
			dRet = ClusterResourceControl(hResource, NULL, CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT, lpInBuffer, cbInBufferSize, NULL, 0, NULL);
			if (!(dRet == ERROR_SUCCESS || dRet == ERROR_FILE_NOT_FOUND))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL 
CInstallation::RegisterCluster()
{
	char szText[512];
	unsigned int uiRet;
	char **sharedDisks, sharedDisk[3];
	int sharedDiskCount, i, j;
	char *databaseName[] = {m_iidatabase.GetBuffer(256),
								m_iicheckpoint.GetBuffer(256),
								m_iijournal.GetBuffer(256),
								m_iidump.GetBuffer(256),
								m_iiwork.GetBuffer(256),
								m_iilogfile.GetBuffer(256),
								0};
	BOOL bFound;
	
	sharedDisks = (char**)malloc(sizeof(char*) * 256);
	sharedDisks[0] = (char*)malloc(sizeof(char) * 256 * 3);
	
	sharedDiskCount=0; i=0;
	while (databaseName[i])
	{
		strncpy(sharedDisk, databaseName[i], 2);
		sharedDisk[2]='\0';
		
		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
		
		i++;
	}
	m_iidatabase.ReleaseBuffer();
	m_iicheckpoint.ReleaseBuffer();
	m_iijournal.ReleaseBuffer();
	m_iidump.ReleaseBuffer();
	m_iiwork.ReleaseBuffer();
	m_iilogfile.ReleaseBuffer();
	
	if (m_enableduallogging)
	{
		strncpy(sharedDisk, m_iiduallogfile.GetBuffer(256), 2);
		sharedDisk[2]='\0';
		
		bFound=FALSE;
		for (j=0; j< sharedDiskCount; j++)
		{
			if (!_stricmp(sharedDisks[j], sharedDisk))
				bFound=TRUE;
		}
		if (!bFound){
			sharedDisks[sharedDiskCount]=&sharedDisks[0][sharedDiskCount*3];
			sprintf(sharedDisks[sharedDiskCount], "%s", sharedDisk);
			sharedDiskCount++;
		}
		m_iiduallogfile.ReleaseBuffer();
	}
	
	uiRet=RegisterClusterService(sharedDisks, sharedDiskCount, m_ClusterResource.GetBuffer(256), m_installationcode.GetBuffer(3));
	sprintf(szText, "Registering cluster resource '%s' returns: %d.", m_ClusterResource.GetBuffer(256), uiRet);
	AppendToLog(szText);
	free(sharedDisks[0]);
	free(sharedDisks);
	
	if (uiRet==HA_SUCCESS)
		return TRUE;
	else
		return FALSE;
}

BOOL 
CInstallation::OnlineResource()
{
	// Bring cluster online. 
	const int SIZE = 256;
	HCLUSTER hCluster = NULL;
	HRESOURCE hResource = NULL;
	WCHAR lpwResourceName[SIZE];
	DWORD dwState;
	BOOL bRet=FALSE;
	
	hCluster = OpenCluster(NULL);
	if (hCluster)
	{
		mbstowcs(lpwResourceName, m_ClusterResource.GetBuffer(SIZE), SIZE);
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
					Sleep (1000);
				}
				else if (dwState == ClusterResourceOnline)
				{
					bRet=TRUE;
					break;
				}
				else
				{
					bRet=FALSE;
					break;
				}
			}

			CloseClusterResource(hResource);
		}
		m_ClusterResource.ReleaseBuffer();
	}

	return bRet;
}

BOOL 
CInstallation::OfflineResource()
{
	// Bring cluster offline. 
	const int SIZE = 256;
	HCLUSTER hCluster = NULL;
	HRESOURCE hResource = NULL;
	WCHAR lpwResourceName[SIZE];
	DWORD dwState;
	BOOL bRet=FALSE;
	
	hCluster = OpenCluster(NULL);
	if (hCluster)
	{
		mbstowcs(lpwResourceName, m_ClusterResource.GetBuffer(SIZE), SIZE);
		hResource = OpenClusterResource(hCluster, lpwResourceName);
		if (hResource)
		{
			OfflineClusterResource(hResource);

			while (1)
			{
				dwState=GetClusterResourceState(hResource, NULL, NULL, NULL, NULL);
				if (dwState == ClusterResourceInitializing ||
					dwState == ClusterResourcePending ||
					dwState == ClusterResourceOfflinePending)
				{
					Sleep (1000);
				}
				else if (dwState == ClusterResourceOffline)
				{
					bRet=TRUE;
					break;
				}
				else
				{
					bRet=FALSE;
					break;
				}
			}

			CloseClusterResource(hResource);
		}
		m_ClusterResource.ReleaseBuffer();
	}

	return bRet;
}

BOOL CInstallation::AddVirtualNodes()
{
	BOOL bRet=FALSE;
	CString m_vhostname, m_protocol, m_listenaddress, m_vnodename, m_username, m_password;
	char hostname[32], protocol[32], listenaddress[32], vnodename[32], username[32], password[32];
	int count=0, res=0;

	AppendComment(IDS_ADDING_VNODES);
	AppendToLog(IDS_ADDING_VNODES);
	while ((count>0 && !m_vhostname.IsEmpty()) || count==0)
	{
		CString str;
		count++;
		sprintf(hostname, "VHOSTNAME%d", count);		
		sprintf(protocol, "PROTOCOL%d", count);
		sprintf(listenaddress, "LISTEN_ADDRESS%d", count);
		sprintf(vnodename, "VNODE_NAME%d", count);
		sprintf(username, "USERNAME%d", count);
		sprintf(password, "PASSWORD%d", count);
		m_vhostname = GetRegValue(hostname);
		m_protocol = GetRegValue(protocol);
		m_listenaddress = GetRegValue(listenaddress);
		m_vnodename = GetRegValue(vnodename);
		m_username = GetRegValue(username);
		m_password = GetRegValue(password);
		if (!m_vhostname.IsEmpty() && !m_protocol.IsEmpty() && !m_listenaddress.IsEmpty()
			&& !m_vnodename.IsEmpty() && !m_username.IsEmpty())
		{
			str.Format("Adding virtual node for %s with listen address %s", m_vhostname, m_listenaddress);
			AppendToLog(str);
			res = CreateVirtualNode(m_vnodename, m_vhostname, 
				m_protocol, m_listenaddress, m_username, 
				m_password);
			if (count>1 && res == 0 && bRet != FALSE)
				bRet = TRUE;
			else if (count == 1 && res == 0)
				bRet = TRUE;
			else
			{
				bRet = FALSE;
				str.Format("Virtual Node was not added because of errors.");
				AppendToLog(str);
			}
		}
		else
		{
			break;
		}
		DeleteRegValue(hostname);
		DeleteRegValue(protocol);
		DeleteRegValue(listenaddress);
		DeleteRegValue(vnodename);
		DeleteRegValue(username);
		DeleteRegValue(password);
	}
	AppendComment(bRet ? IDS_DONE : IDS_FAILED);

	return bRet;
}

void
CInstallation::CheckAPIError( IIAPI_GENPARM	*genParm)
{
    IIAPI_GETEINFOPARM	getErrParm; 
    char		type[33];
	CString errorMsg;
    
    /*
    ** Check API call status.
    */
	errorMsg.Format("\tgp_status = %s\n",
            (genParm->gp_status == IIAPI_ST_SUCCESS) ?  
      "IIAPI_ST_SUCCESS" :
            (genParm->gp_status == IIAPI_ST_MESSAGE) ?  
      "IIAPI_ST_MESSAGE" :
            (genParm->gp_status == IIAPI_ST_WARNING) ?  
      "IIAPI_ST_WARNING" :
            (genParm->gp_status == IIAPI_ST_NO_DATA) ?  
      "IIAPI_ST_NO_DATA" :
            (genParm->gp_status == IIAPI_ST_ERROR)   ?  
      "IIAPI_ST_ERROR"   :
            (genParm->gp_status == IIAPI_ST_FAILURE) ? 
      "IIAPI_ST_FAILURE" :
            (genParm->gp_status == IIAPI_ST_NOT_INITIALIZED) ?
      "IIAPI_ST_NOT_INITIALIZED" :
            (genParm->gp_status == IIAPI_ST_INVALID_HANDLE) ?
      "IIAPI_ST_INVALID_HANDLE"  :
            (genParm->gp_status == IIAPI_ST_OUT_OF_MEMORY) ?
      "IIAPI_ST_OUT_OF_MEMORY"   :
           "(unknown status)" );
   AppendToLog(errorMsg);
    
    /*
    ** Check for error information.
    */
    if ( ! genParm->gp_errorHandle )  return;
    getErrParm.ge_errorHandle = genParm->gp_errorHandle;
    
    do    
    { 
        /*
        ** Invoke API function call.
        */
        IIapi_getErrorInfo( &getErrParm );
    
        /*
        ** Break out of the loop if no data or failed.
        */
        if ( getErrParm.ge_status != IIAPI_ST_SUCCESS )
            break;
    
        /*
        ** Process result.
        */
        switch( getErrParm.ge_type ) 	{
            case  IIAPI_GE_ERROR	 : 
            strcpy( type, "ERROR" ); 	break;
            
            case  IIAPI_GE_WARNING :
            strcpy( type, "WARNING" ); 	break;
            
            case  IIAPI_GE_MESSAGE :
            strcpy(type, "USER MESSAGE");	break;
            
            default:
            sprintf( type, "unknown error type: %d", getErrParm.ge_type);
            break;
        }
    
		errorMsg.Format("Error Info: %s '%s' 0x%x: %s\n",
            type, getErrParm.ge_SQLSTATE, getErrParm.ge_errorCode,
            getErrParm.ge_message ? getErrParm.ge_message : "NULL" );   
		AppendToLog(errorMsg);
    } while( 1 );
    
    return;
}

int 
CInstallation::CreateVirtualNode(CString vnodename, CString hostname, CString protocol, 
								  CString listenaddress, CString username, CString password)
{

    II_PTR		connHandle = (II_PTR)NULL;
    II_PTR		tranHandle = (II_PTR)NULL;
    IIAPI_INITPARM	initParm;
    IIAPI_CONNPARM	connParm;
    IIAPI_AUTOPARM	autoParm;
    IIAPI_QUERYPARM	queryParm;
    IIAPI_GETQINFOPARM	getQInfoParm;
    IIAPI_CLOSEPARM	closeParm;
    IIAPI_DISCONNPARM	disconnParm;
    IIAPI_RELENVPARM	relEnvParm;
    IIAPI_TERMPARM	termParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    char connection[] = "create global connection %s %s %s %s";
    char login[] = "create global login %s %s %s";
    CString stmt;
    int ret = 0;

    /* Initialize API */
    initParm.in_version = IIAPI_VERSION_2;
    initParm.in_timeout = -1;
    IIapi_initialize( &initParm);

    /* Connect to local Name Server */
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target =  NULL;   
    connParm.co_type   =  IIAPI_CT_NS;   //Connect to name server
    connParm.co_connHandle = initParm.in_envHandle; 
    connParm.co_tranHandle = NULL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );

    while(connParm.co_genParm.gp_completed == FALSE)
      IIapi_wait( &waitParm );

    while(connParm.co_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&connParm.co_genParm);
		ret = connParm.co_genParm.gp_status;
		goto exit;
    }

    connHandle = connParm.co_connHandle;
    tranHandle = connParm.co_tranHandle;


    /* Enable autocommit */
    autoParm.ac_genParm.gp_callback = NULL;
    autoParm.ac_genParm.gp_closure  = NULL;
    autoParm.ac_connHandle = connHandle;
    autoParm.ac_tranHandle = NULL;

    IIapi_autocommit( &autoParm );

    while (autoParm.ac_genParm.gp_completed == FALSE )
		IIapi_wait( &waitParm );

    while(autoParm.ac_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&autoParm.ac_genParm);
		ret = autoParm.ac_genParm.gp_status;
		goto exit;
    }

    tranHandle = autoParm.ac_tranHandle;

    /* Add a connection for the vnode */
    stmt.Format(connection, vnodename, hostname, protocol, listenaddress);
    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = stmt.GetBuffer();
    queryParm.qy_parameters = FALSE;
    queryParm.qy_tranHandle = tranHandle;
    queryParm.qy_stmtHandle = NULL;
    queryParm.qy_flags = 0;

    IIapi_query( &queryParm );
  
    while( queryParm.qy_genParm.gp_completed == FALSE )
		IIapi_wait( &waitParm );

    while(queryParm.qy_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&queryParm.qy_genParm);
		ret = queryParm.qy_genParm.gp_status;
		goto exit;
    }

    /* Get query results. */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while (getQInfoParm.gq_genParm.gp_completed == FALSE )
		IIapi_wait( &waitParm );


	/* Close query */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
		IIapi_wait( &waitParm );

    while(closeParm.cl_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&closeParm.cl_genParm);
		ret = closeParm.cl_genParm.gp_status;
		goto exit;
    }

    /* Add login for the vnode */
    stmt.Format(login, vnodename, username, password);
    queryParm.qy_queryText = stmt.GetBuffer();

    IIapi_query ( &queryParm );

    while( queryParm.qy_genParm.gp_completed == FALSE )
      IIapi_wait( &waitParm );

    while(queryParm.qy_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&queryParm.qy_genParm);
		ret = queryParm.qy_genParm.gp_status;
		goto exit;
    }

    /* Get query results. */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    while (getQInfoParm.gq_genParm.gp_completed == FALSE )
		IIapi_wait( &waitParm );


	/* Close query */
    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_close( &closeParm );

    while( closeParm.cl_genParm.gp_completed == FALSE )
		IIapi_wait( &waitParm );

    while(closeParm.cl_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&closeParm.cl_genParm);
		ret = closeParm.cl_genParm.gp_status;
		goto exit;
    }

	/* Disable autocommit */
    autoParm.ac_connHandle = NULL;
    autoParm.ac_tranHandle = tranHandle;

    IIapi_autocommit( &autoParm );
    
    while( autoParm.ac_genParm.gp_completed == FALSE )
       IIapi_wait( &waitParm );

    while(autoParm.ac_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&autoParm.ac_genParm);
		ret = autoParm.ac_genParm.gp_status;
		goto exit;
    }

	/* Disconnect */
    disconnParm.dc_genParm.gp_callback = NULL;
    disconnParm.dc_genParm.gp_closure = NULL;
    disconnParm.dc_connHandle = connHandle;
    
    IIapi_disconnect( &disconnParm );
    
    while( disconnParm.dc_genParm.gp_completed == FALSE )
	 IIapi_wait( &waitParm );

    while(disconnParm.dc_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
		CheckAPIError(&disconnParm.dc_genParm);
		ret = disconnParm.dc_genParm.gp_status;
		goto exit;
    }

    connHandle = NULL;

	/* Release environment resources and terminate */

    relEnvParm.re_envHandle = initParm.in_envHandle;
    IIapi_releaseEnv(&relEnvParm);
    IIapi_terminate( &termParm );
    initParm.in_envHandle = NULL;


exit:
    return ret;

}




