@ECHO OFF

REM Name: setupmdb.bat 
REM
REM Description: 
REM	install mdb.
REM 
REM Syntax:
REM 	setupmdb [-II_SYSTEM=xxxx] [-II_MDB_NAME=xxxx^|-reinitcfg]
REM History:
REM     26-oct-2004 (penga03)
REM	    Created.
REM	02-nov-2004 (hanch04)
REM	    Combined the LoadMDB.bat into setupmdb.bat

REM Start localization of environment variables in a batch file.
SETLOCAL

REM set DEBUGECHO=@ECHO ON
set DEBUGECHO=@ECHO OFF
%DEBUGECHO%
for /f %%a in ('chdir') do set CURDIR=%%a

REM Set the default parameters.
REM set II_SYSTEM=
set REINITCFGONLY=
set II_MDB_ADMIN=mdbadmin
set IIDBDB=iidbdb
set IMADB=imadb
set II_MDB_NAME=mdb


REM Parse the command line parameters.
:PARSE_ARGS
IF "%~1"=="" GOTO NO_MORE_ARGS

IF NOT "%~1"=="-II_SYSTEM" GOTO NEXT_ARG1
SHIFT
set II_SYSTEM=%~1

:NEXT_ARG1
IF NOT "%~1"=="-II_MDB_NAME" GOTO NEXT_ARG2
SHIFT
set II_MDBNAME=%~1

:NEXT_ARG2
IF NOT "%~1"=="-reinitcfg" GOTO NEXT_ARG3
set REINITCFGONLY=TRUE

:NEXT_ARG3
SHIFT
GOTO PARSE_ARGS

:NO_MORE_ARGS

REM Couple of pre-install checks.
IF "%II_SYSTEM%" == "" GOTO USAGE

REM Set the install log file.
set LOGFILE="%II_SYSTEM%\ingres\files\install_%II_MDB_NAME%.log"

REM Create the mdb directory.
IF EXIST "%II_SYSTEM%\ingres\files\mdb" GOTO MDB_DIR_EXISTS
mkdir "%II_SYSTEM%\ingres\files\mdb" >> %LOGFILE% 2>&1
:MDB_DIR_EXISTS

date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
echo Installed the mdb. >> %LOGFILE% 2>&1

REM Add the Ingres installation into the path.
set PATH=%II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility;%PATH%

REM Make sure config.dat exists.
IF NOT EXIST "%II_SYSTEM%\ingres\files\config.dat" GOTO ERR_NOINSTALLATION

REM The DBMS is required for the mdb install.
CALL ipset II_DATABASE ingprenv II_DATABASE
%DEBUGECHO%
IF NOT EXIST "%II_DATABASE%\ingres\data\default\iidbdb" GOTO ERR_NODBMS

CALL ipset HOST iipmhost
%DEBUGECHO%
CALL ipset II_TEMPORARY ingprenv II_TEMPORARY
%DEBUGECHO%

IF "%REINITCFGONLY%"=="TRUE" GOTO CONFIG

REM Step1: Copy and Unzip mdb.caz.

IF NOT EXIST mdb.caz GOTO ERR_NOMDBCAZ

REM Copy and Unzip the mdb.caz.

echo Copying the mdb.caz file. >> %LOGFILE% 2>&1
copy mdb.caz "%II_SYSTEM%\ingres\files\mdb\mdb.caz" >> %LOGFILE% 2>&1

IF NOT EXIST "%II_TEMPORARY%\mdb" GOTO MDB_TMPDIR_EXISTS
echo Removing the temporary mdb directory. >> %LOGFILE% 2>&1
rmdir /s /q "%II_TEMPORARY%\mdb" >> %LOGFILE% 2>&1
:MDB_TMPDIR_EXISTS

echo Making the temporary mdb directory. >> %LOGFILE% 2>&1
mkdir "%II_TEMPORARY%\mdb" >> %LOGFILE% 2>&1

echo Unziping the mdb.caz file. >> %LOGFILE% 2>&1
cazipxp -u -c"%II_TEMPORARY%\mdb" "%II_SYSTEM%\ingres\files\mdb\mdb.caz"  >> %LOGFILE% 2>&1

REM Step2: Configure the DBMS Server for MDB.

:CONFIG

REM Ingres must be shut down before we can install the mdb.
REM Try to shut down nicely.
echo Stopping Ingres. >> %LOGFILE% 2>&1
ingstop > NUL 2>&1
REM Force a shut down.
ingstop -kill > NUL 2>&1

IF NOT ERRORLEVEL 0 GOTO ERR_INGSTOP

echo Configuring server for MDB. >> %LOGFILE% 2>&1

REM Set the registry server.
iisetres ii.%HOST%.gcc.*.registry_type peer
iisetres ii.%HOST%.gcn.registry_type peer
iisetres ii.%HOST%.registry.tcp_ip.status ON

REM Set the dbms cache paramters.

iisetres.exe ii.%HOST%.dbms.private.*.cache.p4k_status ON
iisetres.exe ii.%HOST%.dbms.private.*.cache.p8k_status ON
iisetres.exe ii.%HOST%.dbms.private.*.cache.p16k_status ON
iisetres.exe ii.%HOST%.dbms.private.*.cache.p32k_status ON
iisetres.exe ii.%HOST%.dbms.private.*.cache.p64k_status ON

iisetres.exe ii.%HOST%.dbms.shared.*.cache.p4k_status ON
iisetres.exe ii.%HOST%.dbms.shared.*.cache.p8k_status ON
iisetres.exe ii.%HOST%.dbms.shared.*.cache.p16k_status ON
iisetres.exe ii.%HOST%.dbms.shared.*.cache.p32k_status ON
iisetres.exe ii.%HOST%.dbms.shared.*.cache.p64k_status ON

iisetres ii.%HOST%.dbms.private.*.cache_guideline medium
iisetres ii.%HOST%.dbms.shared.*.cache_guideline medium
REM iisetres ii.%HOST%.dbms.private.*.dmf_cache_size 10000
REM iisetres ii.%HOST%.dbms.shared.*.dmf_cache_size 10000
REM iisetres ii.%HOST%.dbms.private.*.dmf_group_count 1500
REM iisetres ii.%HOST%.dbms.shared.*.dmf_group_count 1500

iisetres ii.%HOST%.dbms.private.*.p4k.cache_guideline medium
iisetres ii.%HOST%.dbms.shared.*.p4k.cache_guideline medium
REM iisetres ii.%HOST%.dbms.private.p4k.*.dmf_cache_size 7500
REM iisetres ii.%HOST%.dbms.shared.p4k.*.dmf_cache_size 7500
REM iisetres ii.%HOST%.dbms.private.p4k.*.dmf_group_count 900
REM iisetres ii.%HOST%.dbms.shared.p4k.*.dmf_group_count 900

iisetres ii.%HOST%.dbms.private.*.p8k.cache_guideline medium
iisetres ii.%HOST%.dbms.shared.*.p8k.cache_guideline medium
REM iisetres ii.%HOST%.dbms.private.p8k.*.dmf_cache_size 2000
REM iisetres ii.%HOST%.dbms.shared.p8k.*.dmf_cache_size 2000
REM iisetres ii.%HOST%.dbms.private.p8k.*.dmf_group_count 250
REM iisetres ii.%HOST%.dbms.shared.p8k.*.dmf_group_count 250

iisetres ii.%HOST%.dbms.private.*.p16k.cache_guideline medium
iisetres ii.%HOST%.dbms.shared.*.p16k.cache_guideline medium
REM iisetres ii.%HOST%.dbms.private.p16k.*.dmf_cache_size 3000
REM iisetres ii.%HOST%.dbms.shared.p16k.*.dmf_cache_size 3000
REM iisetres ii.%HOST%.dbms.private.p16k.*.dmf_group_count 375
REM iisetres ii.%HOST%.dbms.shared.p16k.*.dmf_group_count 375

iisetres ii.%HOST%.dbms.private.*.p32k.cache_guideline medium
iisetres ii.%HOST%.dbms.shared.*.p32k.cache_guideline medium
REM iisetres ii.%HOST%.dbms.private.p32k.*.dmf_cache_size 375
REM iisetres ii.%HOST%.dbms.shared.p32k.*.dmf_cache_size 375
REM iisetres ii.%HOST%.dbms.private.p32k.*.dmf_group_count 45
REM iisetres ii.%HOST%.dbms.shared.p32k.*.dmf_group_count 45

iisetres ii.%HOST%.dbms.private.*.p64k.cache_guideline medium
iisetres ii.%HOST%.dbms.shared.*.p64k.cache_guideline medium
REM iisetres ii.%HOST%.dbms.private.p64k.*.dmf_cache_size 175
REM iisetres ii.%HOST%.dbms.shared.p64k.*.dmf_cache_size 175
REM iisetres ii.%HOST%.dbms.private.p64k.*.dmf_group_count 20
REM iisetres ii.%HOST%.dbms.shared.p64k.*.dmf_group_count 20

iisetres ii.%HOST%.max_tuple_length 0

CALL ipset LOG_KBYTES iigetres ii.%HOST%.rcp.file.kbytes
%DEBUGECHO%
IF LOG_KBYETS LSS 131072 GOTO ERR_INVALIDLOGFILESIZE

IF "%REINITCFGONLY%" == "TRUE" GOTO DONE

echo Starting Ingres. >> %LOGFILE% 2>&1
ingstart > NUL 2>&1
IF NOT ERRORLEVEL 0 GOTO ERR_INGSTART

chdir /D "%II_TEMPORARY%\mdb" >> %LOGFILE% 2>&1

:CREATEUSERS
REM Create the mdb users.
date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
echo Creating the mdb users. >> %LOGFILE% 2>&1
sql %IIDBDB% < .\ddl\users.sql >> %LOGFILE% 2>&1
sql %IMADB%  < .\ddl\ima_adt.sql >> %LOGFILE% 2>&1

:CHECKMDBEXISTS
IF NOT EXIST "%II_DATABASE%\ingres\data\default\%II_MDB_NAME%" GOTO CREATEMDB
REM The mdb exists.  It needs to be upgraded.

REM :UPGRADEMDB
:DESTROYMDB
REM There is no upgrade path yet.  Destroy it.

echo Destroy the %II_MDB_NAME% database. >> %LOGFILE% 2>&1
destroydb %II_MDB_NAME% -u%II_MDB_ADMIN% >> %LOGFILE% 2>&1
REM Goto OPTIMIZEMDB after the upgrade.

:CREATEMDB
REM Create the mdb database.
echo Creating the %II_MDB_NAME% database. >> %LOGFILE% 2>&1
createdb %II_MDB_NAME% -n -u%II_MDB_ADMIN% >> %LOGFILE% 2>&1
iisetres.exe ii.%HOST%.mdb.mdb_dbname %II_MDB_NAME%

:LOADMDB
REM Load the mdb database.
REM grant db_admin on the mdb to group tngadmin.
date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
echo Loading the %II_MDB_NAME% database. >> %LOGFILE% 2>&1
echo grant db_admin on database %II_MDB_NAME% to group tngadmin\p\g > .\DDL\temp.sql
sql %IIDBDB% < .\ddl\temp.sql >> %LOGFILE% 2>&1
REM Run the script to create and load the tables, procedures, grants, etc...
sql -u%II_MDB_ADMIN% %II_MDB_NAME% <.\ddl\mdbadmin.sql >> %LOGFILE% 2>&1

:OPTIMIZEMDB
REM Optimize the mdb database.
date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
echo Optimizing the %II_MDB_NAME% database. >> %LOGFILE% 2>&1
optimizedb -zk %II_MDB_NAME% >> %LOGFILE% 2>&1

:SYSMODMDB
REM Modify the system catalogs in the mdb database.
date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
sysmod +w %II_MDB_NAME% >> %LOGFILE% 2>&1

:CKPMDB
REM Checkpoint the mdb database.
date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
ckpdb +j %II_MDB_NAME% -l +w >> %LOGFILE% 2>&1

:SETCOMPLETE
REM SET MDBVERS=014
REM iisetres.exe ii.%HOST%.config.mdb.%MDBVERS% complete
iisetres.exe ii.%HOST%.mdb.mdb_dbname %II_MDB_NAME%

REM Go back to the starting directory.
chdir /D "%CURDIR%" >> %LOGFILE% 2>&1

echo Removing the temporary mdb directory. >> %LOGFILE% 2>&1
rmdir /s /q "%II_TEMPORARY%\mdb" >> %LOGFILE% 2>&1

REM Shutdown Ingres when the install is done.
REM Try to shut down nicely.
echo Stopping Ingres. >> %LOGFILE% 2>&1
ingstop > NUL 2>&1
REM Force a shut down.
ingstop -kill > NUL 2>&1
IF NOT ERRORLEVEL 0 GOTO ERR_INGSTOP
GOTO COMPLETED

:USAGE
echo Usage: setupmdb [-II_SYSTEM=xxxx] [-II_MDB_NAME=xxxx^|-reinitcfg]
GOTO DONE

:ERR_INGSTART
echo Ingres failed to start.
GOTO DONE

:ERR_INGSTOP
echo Ingres failed to stop.
GOTO DONE

:ERR_NOINSTALLATION
echo There is not a valid Ingres installation under the directory "%II_SYSTEM%".
GOTO DONE

:ERR_NOMDBCAZ
echo The mdb.caz file cannot be found under the directory "%CURDIR%".
GOTO DONE

:ERR_INVALIDLOGFILESIZE
echo Transaction log file must be at least 131073Kb for setup to continue.
GOTO DONE

:ERR_NODBMS
echo The  Ingres Intelligent DBMS must be installed before MDB can be set up on the host %HOST%.
GOTO DONE

:COMPLETED
date /t >> %LOGFILE% 2>&1
time /t >> %LOGFILE% 2>&1
echo Installing mdb completed. >> %LOGFILE% 2>&1
GOTO DONE

:DONE
ENDLOCAL
