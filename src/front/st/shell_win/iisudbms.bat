@echo off
REM
@REM  Copyright (c) 1995, 1997 Ingres Corporation
REM
REM  Name: iisudbms -- setup script for OpenIngres Intelligent DBMS
REM
REM  Usage:
REM	iisudbms
REM
REM  Description:
REM	This script should only be called by the OpenIngres installation
REM	utility.  It sets up the OpenIngres Intelligent Database. 
REM
REM  Exit Status:
REM	0	setup procedure completed.
REM	1	setup procedure did not complete.
REM
REM Note:
REM	All lines beginning with uppercase REM will be stripped from
REM	the distributed version of this file.
REM
REM History:
REM	08-sep-1995 (canor01)
REM	    Get COMPUTERNAME from environment if it is not in symbol.tbl.
REM	    Default II_NUM_OF_PROCESSORS to 1 if not set.
REM	08-sep-1995 (canor01)
REM	    If II_INSTALLATION is already set, do not override with "II"
REM	11-sep-1995 (reijo01)
REM	    Get SERVER_HOST from the symbol table if it exists from a 
REM 	    previous installation.
REM	    Added comments, shorten labels, general cleanup.
REM	    Removed all help for locations and prompts for values that 
REM	    should be set before this script is run.
REM	    Removed references to II_ADMIN since they don't seem to be 
REM	    needed.
REM	12-sep-1995 (reijo01)
REM	    Remove the prompt for user input when a transaction log exists
REM	    already.
REM	    Added checks for temporary variable existence before setting
REM	    config.dat with iisetres. Done in order to allow iisudbms
REM	    to be run standalone.
REM     12-sep-1995 (lawst01)
REM         Only prompt for Upgrade... if symbol table var UPGRADE_DBS not
REM         set.
REM     18-sep-1995 (reijo01)
REM	    Removed a variable from a message in case the variable was not
REM 	    set.
REM     21-sep-1995 (reijo01)
REM	    Set the dual log name from dual_log to dual.log.
REM	17-nov-95 (tutto01)
REM	    Cleaned up output, and modified to work with new install
REM	    dialogs.
REM	16-dec-95 (tutto01)
REM	    Changed II_SYSTEM to II_DATABASE when looking for existing iidbdb.
REM	10-jan-96 (tutto01)
REM	    Moved setting of II_TEMPORARY to iisustrt.bat
REM	07-may-96 (emmag)
REM 	    If a 6.4 version of iimklog.exe exists in the bin, remove it.
REM	    Also call iimklog explicitly from %II_SYSTEM%\ingres\utility
REM	    directory - to be sure, to be sure!
REM     31-May-96 (fanra01)
REM         b76246 Added status check for upgradedb.
REM	31-Jul-96 (somsa01)
REM	    Took out service setup and moved it to iisustrt.bat; this should
REM	    get run if the install includes a DBMS or NET.
REM	11-mar-97 (mcgem01)
REM	    The buffer_count needs to be increased to 20 when upgrading a
REM	    1.2 installation.
REM	08-apr-97 (mcgem01)
REM	    Change product name from CA-OpenIngres to OpenIngres.
REM	13-jun-97 (mcgem01)
REM	    Add the creation of the imadb for VDBA.
REM	24-jun-97 (mcgem01)
REM	    Fix a typo.
REM	02-aug-97 (mcgem01)
REM	    Create and populate imadb when doing an upgrade installation.
REM	17-nov-1997 (canor01)
REM	    Quote pathnames to support embedded spaces. Log file size is in
REM	    kbytes now.
REM     30-apr-1998 (rigka01)
REM         Add rmcmd to config.dat for use with VDBA
REM	19-Nov-2010 (kiria01) SIR 124690
REM	    Default to UCS_BASIC collation if UTF8 chosen
REM


if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo.
echo WH Setting up the OpenIngres Intelligent DBMS...


REM	Set the host name based on the COMPUTERNAME variable.
REM
set CONFIG_HOST=%COMPUTERNAME%
set HOSTNAME=%COMPUTERNAME%
set ING_USER=INGRES


REM	Set privileges and the local vnode name now that we have the hostname.
REM
iisetres ii.%CONFIG_HOST%.privileges.user.root SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.%CONFIG_HOST%.privileges.user.%ING_USER% SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.%CONFIG_HOST%.gcn.local_vnode %HOSTNAME%


REM 	Check to see if SERVER_HOST has been placed in the symbol
REM	table from a previous installation.
REM
call ipset SERVER_HOST iigetres ii."*".config.server_host


REM	If we didn't get the host from the symbol table variable SERVER
REM	_HOST try another method to get the SERVER_HOST. Try the local
REM	vnode identifier.
REM
if not "%SERVER_HOST%"=="" goto GOTHOST
set SERVER_ID=%II_INSTALLATION%

if "%SERVER_ID%"=="" goto GOTHOST
call ipset SERVER_HOST ingprenv II_GCN%SERVER_ID%_LCL_VNODE


REM	See if the previous installation was on different host.
REM
:GOTHOST
if "%SERVER_HOST%"=="" goto  SAMEHOST 
if "%SERVER_HOST%"=="%CONFIG_HOST%" goto SAMEHOST
echo.
echo The OpenIngres Intelligent DBMS installation located at:
echo.
echo    %II_SYSTEM%
echo.
echo is already set up to run from:
echo.
echo    %SERVER_HOST%
echo.
echo Once the OpenIngres Intelligent DBMS has been set up to run on a 
echo particular system, it cannot be moved easily.
goto CLEANEXIT


REM	Check to see if the dbms installation has completed already.
REM
:SAMEHOST
call ipset RELEASE_ID ingprenv RELEASE_ID

call ipset SETUP iigetres ii.%CONFIG_HOST%.config.dbms.%RELEASE_ID%
if not "%SETUP%" == "complete" goto SETDBMS

echo The %RELEASE_ID% version of the OpenIngres Intelligent DBMS has
echo already been set up on the local host. 
goto CLEANEXIT


REM	Retrieve values set during the first stage of the install.
REM
:SETDBMS
ingsetenv II_DATABASE "%II_DATABASE%"
ingsetenv II_CHECKPOINT "%II_CHECKPOINT%"
ingsetenv II_JOURNAL "%II_JOURNAL%"
ingsetenv II_DUMP "%II_DUMP%"
ingsetenv II_WORK "%II_WORK%"

REM	Make sure these directory structures exist.
REM
echo WD Creating directory structures...
mkdir "%II_DATABASE%" 2>NUL
mkdir "%II_DATABASE%\ingres" 2>NUL
mkdir "%II_DATABASE%\ingres\data" 2>NUL
mkdir "%II_DATABASE%\ingres\data\default" 2>NUL

mkdir "%II_CHECKPOINT%" 2>NUL
mkdir "%II_CHECKPOINT%\ingres" 2>NUL
mkdir "%II_CHECKPOINT%\ingres\ckp" 2>NUL
mkdir "%II_CHECKPOINT%\ingres\ckp\default" 2>NUL

mkdir "%II_JOURNAL%" 2>NUL
mkdir "%II_JOURNAL%\ingres" 2>NUL
mkdir "%II_JOURNAL%\ingres\jnl" 2>NUL
mkdir "%II_JOURNAL%\ingres\jnl\default" 2>NUL

mkdir "%II_DUMP%" 2>NUL
mkdir "%II_DUMP%\ingres" 2>NUL
mkdir "%II_DUMP%\ingres\dmp" 2>NUL
mkdir "%II_DUMP%\ingres\dmp\default" 2>NUL

mkdir "%II_WORK%" 2>NUL
mkdir "%II_WORK%\ingres" 2>NUL
mkdir "%II_WORK%\ingres\work" 2>NUL
mkdir "%II_WORK%\ingres\work\default" 2>NUL


REM	Test to see if we have a completed parts of the setup before.
REM
:DEFCON
if "%SETUP%" == "defaults" goto DEFGEN
if "%SETUP%" == "users" goto DEFGEN

echo.
echo WD Generating default configuration...


REM	Populate the symbol table via the dbms rules file.
REM
iigenres %CONFIG_HOST% "%II_SYSTEM%\ingres\files\dbms.rfm"
if not errorlevel 0 goto BADDEF


REM	Set the connection limit in config.dat
REM
iisetres ii.%CONFIG_HOST%.dbms."*".connect_limit %II_CONC_USERS%


REM	Set the level of SQL.
REM
:SETSQL
if "%II_SQL92%" == "" goto SKIP_FIPS
iisetres ii.%CONFIG_HOST%.fixed_prefs.iso_entry_sql-92 %II_SQL92%
:SKIP_FIPS


REM	Set a marker in the config.dat file to indicate that we have 
REM	already generated a default configuration.
REM
:SETMARK
iisetres ii.%CONFIG_HOST%.config.dbms.%RELEASE_ID% defaults
iisetres ii.%CONFIG_HOST%.rcp.log.buffer_count 20
goto DEFGEN   

:BADDEF
echo.
echo An error occurred while generating the default configuration.
goto GRACEFUL_EXIT

:DEFGEN
REM echo.
REM echo Default configuration generated.
REM It worked, no need for extra verbiage.


:GOTINST
echo.
echo II_INSTALLATION is configured as %II_INSTALLATION%.
ingsetenv II_INSTALLATION %II_INSTALLATION%
ingsetenv II_GCN%II_INSTALLATION%_LCL_VNODE %HOSTNAME%

REM	Not sure that this one applies!  But it shouldn't hurt.
REM
ingsetenv II_MAX_SEM_LOOPS 2000


REM	Configure the time zone.
REM
:TIMEZONE
if "%II_TIMEZONE_NAME%"=="" goto BADTIME
echo.
echo II_TIMEZONE_NAME configured as %II_TIMEZONE_NAME%. 
goto CHARSET


:BADTIME
echo.
echo Error: Could not get II_TIMEZONE_NAME from symbol table.
goto GRACEFUL_EXIT


REM	Configure character set.
REM
:CHARSET
echo.
echo II_CHARSET%II_INSTALLATION% configured as %II_CHARSET%. 
ingsetenv II_CHARSET%II_INSTALLATION% %II_CHARSET%

if NOT "%II_CHARSET%"=="UTF8" goto QUELSQL
iisetres ii.%CONFIG_HOST%.dbms."*".default_collation UCS_BASIC
goto QUELSQL
   

:BADCHAR   
echo.
echo Error: Could not get IICHARSET%II_INSTALLATION% from symbol table.
echo goto GRACEFUL_EXIT
      

REM	Create quel and sql startup files if not found
REM
:QUELSQL
if not exist "%II_SYSTEM%\ingres\files\startup.dst" goto STARTSQL
if exist "%II_SYSTEM%\ingres\files\startup" goto STARTSQL
copy "%II_SYSTEM%\ingres\files\startup.dst" "%II_SYSTEM%\ingres\files\startup" >NUL
:STARTSQL
if not exist "%II_SYSTEM%\ingres\files\startsql.dst" goto DEFNM
if exist "%II_SYSTEM%\ingres\files\startsql" goto DEFNM
copy "%II_SYSTEM%\ingres\files\startsql.dst" "%II_SYSTEM%\ingres\files\startsql" >NUL
:DEFNM


:SETEDIT
ingsetenv ING_EDIT notepad.exe
goto TRANSLOG


:EDITERR
echo.
echo Error: %ING_EDIT% not defaulted in configuration rule base.
goto GRACEFUL_EXIT


REM	Configure the transaction logs.
REM
:TRANSLOG


REM	Verify the primary transaction log file name.
REM	If the name does not exist then default to ingres.log
REM
if not "%II_LOG_FILE_NAME%"=="" goto CHKDUAL
ingsetenv II_LOG_FILE_NAME ingres.log 


REM	Verify the dual transaction log file name.
REM
:CHKDUAL


REM	Check if the transaction log exists already.
REM
:LOGEXIST
set LOG="%II_LOG_FILE%\ingres\log\%II_LOG_FILE_NAME%"
if not exist %LOG% goto NOLOG


REM	Handle an existing transaction log.
REM
echo.
echo There is an existing transaction log.
echo.      
echo In order to recreate the transaction log an Ingstop
echo must be performed in order for the setup to continue.
echo.


REM	Shutting down the installation in order to erase the existing 
REM	transaction log.
REM
:SHUTTING
echo.
echo Running OpenIngres/Ingstop...
echo.
ingstop -force
echo.
echo.
echo WD Erasing existing OpenIngres transaction log...

ipcclean >NUL
echo.
del %LOG%
if not ERRORLEVEL 1 goto NOLOG
echo.
echo Unable to delete %LOG%.
echo Correct the above problem and re-run this setup program. 
goto GRACEFUL_EXIT


REM	Create the transaction log for the first time.
REM
:NOLOG
REM echo.
REM echo The primary transaction log will now be created. (buffered)
REM echo system file.
REM echo.

if not exist "%II_SYSTEM%\ingres\bin\iimklog.exe" goto NOOLDEXE
del /F "%II_SYSTEM%\ingres\bin\iimklog.exe"
:NOOLDEXE
ingsetenv II_LOG_FILE "%II_LOG_FILE%"
ingsetenv II_LOG_FILE_NAME %II_LOG_FILE_NAME%
iisetres ii.%CONFIG_HOST%.rcp.file.kbytes %II_LOG_FILE_KBYTES%
mkdir "%II_LOG_FILE%\ingres\log" 2>NUL
echo WD Now creating the primary transaction log...
"%II_SYSTEM%\ingres\utility\iimklog"
if not ERRORLEVEL 1 goto CRE8DUAL
echo.
echo.
echo The system utility "iimklog" failed to complete successfully.  You must
echo correct the problem described above and re-run this setup program. 
goto GRACEFUL_EXIT


REM	Check for the existence of a dual log.
REM
:CRE8DUAL
if "%II_DUAL_ENABLED%"=="ON" goto SKIPCLEAR
set II_DUAL_LOG=
set II_DUAL_LOG_NAME=

:SKIPCLEAR
if "%II_DUAL_ENABLED%"=="OFF" goto FORMPRIM
ingsetenv II_DUAL_LOG "%II_DUAL_LOG%"
ingsetenv II_DUAL_LOG_NAME %II_DUAL_LOG_NAME%
set DUAL_LOG="%II_DUAL_LOG%\ingres\log\%II_DUAL_LOG_NAME%"
mkdir "%II_DUAL_LOG%\ingres\log" 2>NUL
if not exist "%DUAL_LOG%" goto BACKTEXT   

echo.
echo A backup transaction log already exists.


REM	Erase the existing backup transaction log.
REM
echo.         
echo WD Erasing the existing OpenIngres transaction log...
echo.
del %DUAL_LOG%
if not ERRORLEVEL 1 goto BACKTEXT
echo.
echo.
echo Unable to delete %DUAL_LOG%.
echo Correct the above problem and re-run this setup program. 
goto GRACEFUL_EXIT


REM	Create the dual transaction log.
REM
:BACKTEXT
echo.
echo The backup transaction log will now be created as an ordinary (buffered)
echo system file.
"%II_SYSTEM%\ingres\utility\iimklog" -dual 
if not ERRORLEVEL 1 goto FORMPRIM
echo.
echo.
echo The system utility "iimklog" failed to complete successfully.  You must
echo correct the problem described above and re-run this setup program. 
goto GRACEFUL_EXIT


:FORMPRIM
echo.
echo.
echo WD Formatting the primary transaction log file...
ipcclean >NUL
goto INIT_LOG


:INIT_LOG
rcpconfig -init_log >NUL
if not ERRORLEVEL 1 goto CHKLOG
echo.
echo.
echo Unable to write the primary transaction log file header.  You must
echo correct the problem described above and re-run this setup program.
ipcclean >NUL
goto GRACEFUL_EXIT


:CHKLOG
if "%II_DUAL_ENABLED%"=="OFF" goto IPCCLEAN
echo.
echo WD Formatting the backup transaction log file...
rcpconfig -init_dual >NUL
if not ERRORLEVEL 1 goto IPCCLEAN
echo.
echo.
echo Unable to write the OpenIngres transaction log file header.  You must
echo correct the problem described above and re-run this setup program.
ipcclean >NUL
goto GRACEFUL_EXIT


:IPCCLEAN
ipcclean >NUL


if not exist "%II_DATABASE%\ingres\data\default\iidbdb" goto CRE8SYSC
  
if not "%UPGRADE_DBS%" == "" goto SETDBSY
set UPGRADE_DBS=YES
echo.
ipchoice Upgrade all databases in this installation now? 
if ERRORLEVEL 2 goto SETDBSNO
goto SETDBSY
:SETDBSNO
set UPGRADE_DBS=NO

:SETDBSY
echo.
if "%UPGRADE_DBS%" == "YES" goto STARTDBY
echo WD Starting OpenIngres to upgrade system catalogs...
echo.
goto STARTDB
:STARTDBY
echo WD Starting OpenIngres to upgrade system catalogs and databases...
:STARTDB
ipcclean
REM SET TEMP_VARIABLE=%II_INSTALLATION%
REM SET II_INSTALLATION=
ingstart -iigcn
ingstart -dmfrcp
ingstart -dmfacp
ingstart -iidbms
if not ERRORLEVEL 1 goto UPGRADE 
ingstop >NUL 
echo.
echo.
echo The OpenIngres server could not be started.  See the server error log
echo (%II_SYSTEM%\ingres\files\errlog.log) for a full description of the
echo problem. Once the problem has been corrected, you must re-run this
echo setup program before attempting to use the installation.
goto GRACEFUL_EXIT

:UPGRADE
if "%UPGRADE_DBS%" == "NO" goto UPGRADEI
   echo WD Upgrading system catalogs and databases...
   upgradedb -all >"%II_SYSTEM%\ingres\files\upgradedb.log"
   if errorlevel 0 goto SHUTDOWN
   echo A failure was reported during the upgrade of system catalogs and databases.
   echo Please review the %II_SYSTEM%\ingres\files\upgradedb.log file.
goto SHUTDOWN
:UPGRADEI
   echo WD Upgrading system catalogs...
   upgradedb iidbdb >"%II_SYSTEM%\ingres\files\upgradedb.log"
   if errorlevel 0 goto CR8IMA
   echo A failure was reported during the upgrade of system catalogs.
   echo Please review the %II_SYSTEM%\ingres\files\upgradedb.log file.
goto SHUTDOWN
:CR8IMA
if exist "%II_DATABASE%\ingres\data\default\imadb" goto SHUTDOWN
   echo WD Creating imadb for gathering performance statistics.
   createdb -u$ingres imadb>NUL
   echo.
   echo WD Populating imadb.
   sql -u$ingres imadb <"%II_SYSTEM%\ingres\vdba\makiman.sql">NUL
   if errorlevel 0 goto SHUTDOWN
   echo A failure was reported during the creation of the IMA database.
   echo Please review the %II_SYSTEM%\ingres\files\errlog.log file.
goto SHUTDOWN

:CRE8SYSC
echo.
echo.
echo WD Starting OpenIngres to initialize system catalogs...
ipcclean > NUL
echo.
ingstart -iigcn
ingstart -dmfrcp
ingstart -dmfacp
ingstart -iidbms
if not ERRORLEVEL 1 goto INIT_CAT
ingstop >NUL 
echo.
echo.
echo The OpenIngres server could not be started.  See the server error log
echo (%II_SYSTEM%\ingres\files\errlog.log) for a full description of the
echo problem. Once the problem has been corrected, you must re-run this
echo setup program before attempting to use the installation.
goto GRACEFUL_EXIT


:INIT_CAT
echo.
echo.
echo WD Initializing the OpenIngres system catalogs...
echo.
createdb -S iidbdb
if ERRORLEVEL 1 goto BADCREDB
echo.
echo WD Checkpointing OpenIngres system catalogs...
ckpdb +j iidbdb >"%II_SYSTEM%\ingres\files\ckpdb.log"
if not ERRORLEVEL 1 goto SHUTDOWN
echo.
echo An error occurred while checkpointing your system catalogs.  A log of
echo the attempted checkpoint can be found in:
echo.
echo        %II_SYSTEM%\ingres\files\ckpdb.log 
echo.
echo You should contact Ingres Corporation Technical Support to resolve
echo this problem. 


:SHUTDOWN

REM Setup Visual DBA tables...
REM
echo.
echo WD Setting up Visual DBA...
call "%II_SYSTEM%\ingres\vdba\rmcmdenv.exe" "%II_SYSTEM%\ingres\vdba"
call "%II_SYSTEM%\ingres\vdba\rmcmdgen.exe"
iisetres ii.%CONFIG_HOST%.ingstart.*.rmcmd 1

if exist "%II_DATABASE%\ingres\data\default\imadb" goto LOADIMA
echo.
echo WD Creating imadb for gathering performance statistics.
createdb -u$ingres imadb

:LOADIMA
echo.
echo WD Populating imadb.
sql -u$ingres imadb <"%II_SYSTEM%\ingres\vdba\makiman.sql">NUL

echo.
echo WD Shutting down the OpenIngres server...
ingstop -kill >NUL 
echo.
echo WD OpenIngres Intelligent DBMS setup complete...
echo.
REM echo Refer to the OpenIngres Installation Guide for information about
REM echo starting and using OpenIngres.
iisetres ii.%CONFIG_HOST%.config.dbms.%RELEASE_ID% complete
goto CLEANEXIT 


:BADCREDB
del /f /s /q "%II_DATABASE%\ingres\data\default\iidbdb\*.*"
rmdir "%II_DATABASE%\ingres\data\default\iidbdb"
del /f /s /q "%II_WORK%\ingres\work\default\iidbdb\*.*"
rmdir "%II_WORK%\ingres\work\default\iidbdb"
echo.
echo The system catalogs were not created successfully.   You must correct
echo the problem described above and re-run this setup program before this
echo installation can be used.
echo.
echo Shutting down the OpenIngres server...
echo.
ingstop -kill >NUL 
goto GRACEFUL_EXIT

:CLEANEXIT

REM 09-aug-95 (reijo01)
REM Added a call to setperm to set the file permissions for a few executables.
setperm > NUL

:GRACEFUL_EXIT
echo.
