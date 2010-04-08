echo off
REM 
REM inglogs.bat - mutex gathering script for Windows
REM           *should* in theory work on anything we support
REM           though not all info is available pre-XP Pro
REM
REM           Assumes Ingres environment and path etc are set
REM
REM Known Limitations: 
REM
REM  * Uses 'systeminfo' and 'tasklist' which are only
REM     available in XP Pro & Windows 2003
REM
REM  * Custom server classes must be added manually to this
REM    script. Any servers of a class not added will be 
REM    ignored. Search for 'CUSTOM' below
REM
REM 11-May-2006 Paul Mason
REM   v1 initial version
REM   
REM   v2 changed code to generate date-time formatted
REM      directory name to something more generic
REM
REM 12-May-2006 Paul Mason
REM   v3 using PIDs from logstat to identify possible
REM      non-registered servers
REM
REM      also tidied up messages and added comments
REM
REM 30-Jul-2007 Paul Mason
REM   v4 prefix FIND with %WINDIR%\system32 so as not to
REM      conflict with e.g. cygwin find
REM
REM      also copy mut.bat to results directory so we can
REM      see the exact version that was used.
REM
REM 30-Jul-2008 Paul Mason
REM   v5 add ALL to show sessions commands
REM
REM 23-Oct-2009 Paul Mason
REM   inglogs v1.00 (= mut.bat v6)
REM   Renamed to inglogs.bat in preparation for adding to
REM   Ingres SIG directory.
REM
REM 23-Oct-2009 Paul Mason
REM   v1.01
REM   Also added show help and flags to run individual modules
REM   similar to the unix inglogs
REM   v1.02
REM   Amended echo statements for blank lines


set INGLOGS_VERSION=1.02_WIN

set HELP=0
set USAGE=0
set VERSION=0
set SYSCONFIG=1
set SYSSTATE=1
set INGLOGFILES=1
set INGCONFIG=1
set INGSTATE=1
set LOGDUMP=0

REM
REM check command line flags
REM should really only be a maximum of 6 but let's check upto 10
REM

call :check_arg %1
call :check_arg %2
call :check_arg %3
call :check_arg %4
call :check_arg %5
call :check_arg %6
call :check_arg %7
call :check_arg %8
call :check_arg %9
call :check_arg %10


if  %HELP% == 1 (goto :show_help)
if %USAGE% == 1 (goto :show_usage)
if %VERSION% == 1 (goto :show_version)



REM Friendly message
echo ********************************************************
echo Running INGLOGS - Ingres information gathering script
echo ---
echo note:
echo      if the script does not respond for a long time then
echo      it's possible that it has hung - in which case 
echo      ctrl-c to quit
echo ********************************************************
echo.

REM set date and time variables - using a method which
REM works in *most* locales and in Win2000

if "%date%A" LSS "A" (set toks=1-3) else (set toks=2-4)
for /f "skip=1 tokens=2-4 delims=(-)" %%a in ('echo:^|date') do (
  for /f "tokens=%toks% delims=.-/ " %%d in ('date/t') do (
    set %%a=%%d
    set %%b=%%e
    set %%c=%%f
    set toks=
  )
)

for /f "tokens=5-7 delims=:., " %%a in ('echo:^|time') do (
  set hh=%%a
  set mn=%%b
  set ss=%%c
)
if %hh% LSS 10 set hh=0%hh%

REM
REM and the purpose of this - to generate a directory name
REM based on a date-time stamp
REM

SET DNAME=INGLOGS_%yy%%mm%%dd%_%hh%%mn%%ss%

echo Creating directory %DNAME% for logs
echo.

md %DNAME%
cd %DNAME%

REM
REM MFIND is our version of FIND, with full path
REM

SET MFIND=%WINDIR%\system32\find

REM
REM copy inglogs itself so we can see what version was used 
REM

copy %0% . > dontcare

if %SYSCONFIG% == 1 (call :system_config)

if %SYSSTATE% == 1 (call :system_state)

if %INGCONFIG% == 1 (call :ingconfig)

if %INGLOGFILES% == 1 (call :inglogfiles)

if %INGSTATE% == 1 (call :ingstate)

if %LOGDUMP% == 1 (call :logdump)

REM
REM Cleanup
REM

SET DONE=
SET EXT=
SET SVRCLASSES=
SET SVRDONE=
SET dd=
SET mm=
SET yy=
SET hh=
SET ss=
SET mn=
SET INST_ID=
SET PRVDONE=
SET PRVSVR=

DEL done 2> dontcare2
DEL dontcare 2> dontcare2
DEL iinamu.tmp.a 2> dontcare2
DEL iinamu.tmp.b 2> dontcare2
DEL svrpids1 2> dontcare2
DEL svrpids2 2> dontcare2
DEL svrpids3 2> dontcare2
DEL svrpids.vbs 2> dontcare2
DEL tmp.iinamu 2> dontcare2
DEL mutexlist.* 2> dontcare2
DEL dontcare2 

if EXIST tmp.iimon (del tmp.iimon)

cd ..

echo.
echo ********************************************************
echo INGLOGS completed - output in %DNAME%
echo ********************************************************

goto:eof

:system_config

REM
REM SYSTEM_CONFIG - collect system (OS) config info
REM

echo Gathering Windows config information 
echo. 

ver > ver.txt
set > set.txt

REM
REM systeminfo only available in XP Pro and Up
REM re-direct stderr so as to not produce any messages on W2K
REM

systeminfo > sysinfo.txt 2>dontcare

goto :eof

:system_state

echo Gathering Windows state information 
echo. 

REM
REM tasklist only available in XP Pro and Up
REM re-direct stderr so as to not produce any messages on W2K
REM

tasklist /v > tasklist.txt 2> dontcare
tasklist /svc >> tasklist.txt 2> dontcare

net start > services.txt
netstat -a > netstat.txt

goto :eof

:inglogfiles

REM
REM INGLOGFILES - collect ingres log files
REM

echo Gathering Ingres logs 
echo. 

REM
REM quotes required as II_SYSTEM could be C:\Program Files\... etc
REM

copy "%II_SYSTEM%\ingres\files\errlog.log" . > dontcare 2> dontcare2
copy "%II_SYSTEM%\ingres\files\iiacp.log" . > dontcare 2>dontcare2
copy "%II_SYSTEM%\ingres\files\iircp.log" . > dontcare 2>dontcare2
copy "%II_SYSTEM%\ingres\files\*dbms*.log" . > dontcare 2> dontcare2

goto :eof

:ingconfig

REM
REM INGCONFIG - collect ingres configuration files
REM

echo Gathering Ingres configuration files
echo. 

copy "%II_SYSTEM%\ingres\files\config.log" . > dontcare 2>dontcare2
copy "%II_SYSTEM%\ingres\files\config.dat" .> dontcare 2>dontcare2
copy "%II_SYSTEM%\ingres\files\protect.dat" . > dontcare 2> dontcare2
copy "%II_SYSTEM%\ingres\version.rel" . > dontcare 2>dontcare2
copy "%II_SYSTEM%\ingres\version.dat" . > dontcare 2>dontcare2
copy "%II_SYSTEM%\ingres\symbol.*" . > dontcare 2>dontcare2

ingprenv > ingprenv.txt
syscheck -v > syscheck.txt

goto :eof

:ingstate

REM
REM INGSTATE - collect ingres state info, iimonitor, mutexes etc
REM 


REM
REM IMPORTANT: Put any custom server classes in the list below
REM

SET SVRCLASSES=INGRES IUSVR CUSTOM

REM
REM build a list of servers registered with iinamu
REM

echo Getting a list of Servers from IINAMU 

FOR %%A in (%SVRCLASSES%) DO echo show %%A >>tmp.iinamu
echo quit >> tmp.iinamu

iinamu < tmp.iinamu > iinamu.txt

type iinamu.txt | %MFIND% "IINAMU" > iinamu.tmp.a
type iinamu.txt | %MFIND% /V "IINAMU" > iinamu.tmp.b

FOR %%A in (%SVRCLASSES%) DO call :svrclass %%A

REM 
REM build a list of servers based on PIDs in logstat
REM these are the servers registered with the logging system
REM

echo. 
echo Getting a list of servers attached to the logging system 

logstat > logstat.txt
logstat -verbose > logstat_verbose.txt

REM
REM we need the Installation ID to generate the connect id
REM

for /f %%A in ('ingprenv II_INSTALLATION') do set INST_ID=%%A

REM
REM grab the lines from logstat - MASTER=RCP, FCT=Fast Commit server, 
REM  SLAVE=non-FC server
REM

type logstat.txt | %MFIND% "MASTER" > svrpids1
type logstat.txt | %MFIND% "FCT" >> svrpids1
type logstat.txt | %MFIND% "SLAVE" >> svrpids1

echo Server PIDs from logstat > svr_msgs.txt
type svrpids1 >> svr_msgs.txt
echo. >> svr_msgs.txt

REM 
REM tiny bit of VBscript to do decimal -> Hex conversion
REM

for /F "tokens=2" %%P in (svrpids1) do echo Wscript.Echo "PID:",Hex(%%P) >> svrpids.vbs

cscript svrpids.vbs > svrpids2

type svrpids2 | %MFIND% "PID" > svrpids3

echo Server PIDs in Hex: >> svr_msgs.txt
type svrpids3 >> svr_msgs.txt

echo. >> svr_msgs.txt
echo Messages when checking for private servers: >> svr_msgs.txt

for /f "tokens=2" %%P in (svrpids3) do call :prvsvr %%P

REM
REM Now we have two lists registered and unregistered servers
REM let's get sessions on both of them
REM

echo. 
echo Gathering Session and Mutex info for each Server 

echo dummy > done
for /F %%S in (reg_svrs.txt) do call :getsvrinfo %%S 
if EXIST prv_svrs.txt call :prvsvr_exist

echo. 
echo Gathering Locking information 

lockstat -dirty > lockstat_dirty.txt
lockstat > lockstat.txt

goto :eof

:prvsvr_exist
REM
REM PRSVR_exist - do getsvrinfo on private servers
REM   called if prv_svrs.txt exists (avoids a file not found
REM   error)
REM

for /F %%S in (prv_svrs.txt) do call :getsvrinfo %%S 

goto :eof

:prvsvr
REM
REM PRVSVR - check to see if %1 is a private server
REM

for /F "tokens=3" %%A in ('%MFIND% /c /i "%1" reg_svrs.txt') do SET PRVDONE=%%A

if "%PRVDONE%" == "0" (

  echo   %1 looks like a de-registered/private server >> svr_msgs.txt
  echo   %1 looks like a de-registered/private server 


  FOR %%T in (%SVRCLASSES%) do call :chkprvtype %1 %%T
) else (
  echo   %1 is registered in IINAMU >> svr_msgs.txt
  echo   %1 is registered in IINAMU 
)


goto:eof

:logdump

REM
REM logdump is commented out, since logdump output is large
REM uncomment if needed 
REM

echo Gathering Transaction Logfile info - this may take a few minutes
echo.
logdump -verbose > logdump.txt

goto :eof

:chkprvtype
REM
REM CHKPRVTYPE - check if server %1 is of type %2
REM

REM
REM generate connect id
REM

set PRVSVR=%INST_ID%\%2\%1 


REM
REM if we can connect then it is of type %2
REM

echo HELP | iimonitor %PRVSVR% > dontcare
if %ERRORLEVEL% == 0 (
	echo %PRVSVR% >> prv_svrs.txt  
	echo     %1 seems to be a %2 server, will connect using %PRVSVR% >> svr_msgs.txt
	echo     %1 seems to be a %2 server, will connect using %PRVSVR% 
		)
goto:eof

:svrclass
REM
REM SVRCLASS - get a list (from iinamu) of all servers of type %1
REM

echo   Getting list of Ingres Servers of type %1 

REM
REM The output from IINAMU will contain at least one line like
REM this:
REM    IINAMU> INGRES    * ...
REM so we look for both IINAMU line and not
REM The value we're looking for is item 3 on most lines, 4 on IINAMU lines
REM

for /F "tokens=4" %%S in ('%MFIND% "%1" iinamu.tmp.a') do echo %%S >>reg_svrs.txt
for /F "tokens=3" %%S in ('%MFIND% "%1" iinamu.tmp.b') do echo %%S >>reg_svrs.txt

goto:eof

:getsvrinfo

REM
REM GETSVRINFO - get Session and mutex info for server %1
REM

REM
REM If database lists are in use then we can get duplicates so
REM check to see if we've done this server before
REM

for /F "tokens=3" %%A in ('%MFIND% /c "%1" done') do SET SVRDONE=%%A

REM
REM Need to set the EXT variable for the per-server filenames
REM because of the way DOS works need to do this outside the
REM compound statement in the IF
REM

for /f "tokens=3 delims=\" %%A in ('echo %1') do SET EXT=%%A

if "%SVRDONE%"=="0" (

  echo   for Server %1 : 
  echo     gathering Session information 

  echo SHOW ALL SESSIONS | iimonitor %1 > sessions.%EXT%.txt
  echo SHOW ALL SESSIONS FORMATTED |iimonitor %1 > formatted.%EXT%.txt

  echo     checking for mutexes  

  type sessions.%EXT%.txt | %MFIND% "CS_MUTEX" > mutexlist.%EXT%
  type mutexlist.%EXT% >> mutexlist.all

  if EXIST tmp.iimon (del tmp.iimon)
  for /f "tokens=3 delims=)" %%M in (mutexlist.%EXT%) do echo SHOW MUTEX %%M >> tmp.iimon
  if EXIST tmp.iimon (iimonitor %1 < tmp.iimon > mutex.%EXT%.txt)
  echo %1 >> done
  
)

goto :eof

:check_arg

REM
REM CHECK_ARG - check a command line argument
REM

set LATEST_ARG=%1

if "%LATEST_ARG%" EQU "-v" (set VERSION=1)
if "%LATEST_ARG%" EQU "-version" (set VERSION=1)
if "%LATEST_ARG%" EQU "--version" (set VERSION=1)
if "%LATEST_ARG%" EQU "--help" (set HELP=1)
if "%LATEST_ARG%" EQU "-help" (set USAGE=1)
if "%LATEST_ARG%" EQU "-h" (set USAGE=1)
if "%LATEST_ARG%" EQU "-nosystemconfig" (set SYSCONFIG=0)
if "%LATEST_ARG%" EQU "-nosystemstate" (set SYSSTATE=0)
if "%LATEST_ARG%" EQU "-nologfiles" (set INGLOGFILES=0)
if "%LATEST_ARG%" EQU "-noingresconfig" (set INGCONFIG=0)
if "%LATEST_ARG%" EQU "-noingresstate" (set INGSTATE=0)
if "%LATEST_ARG%" EQU "-logdump" (set LOGDUMP=1)

goto :eof


:show_usage

REM
REM SHOW_USAGE - show the usage message
REM

echo Usage: inglogs [-nosystemconfig] [-nosystemstate] [-nologfiles] 
echo                [-noingresconfig] [-noingresstate] [-logdump]
echo        inglogs [-h^|-help]
echo        inglogs [--help]
echo        inglogs [-v^|-version^|--version]
   
goto :eof

:show_version

REM
REM SHOW_VERSION - show the version
REM

echo INGLOGS version %INGLOGS_VERSION%

goto :eof


:show_help

REM
REM SHOW_HELP - show usage information 
REM

call :show_version > help.tmp
echo. >> help.tmp

call :show_usage >> help.tmp

echo. >> help.tmp
echo The inglogs script collects some information from your system which >> help.tmp
echo might be helpful for analysis. By default it collects as much information >> help.tmp
echo as possible, but you can switch off some "modules" if wanted. >> help.tmp
echo. >> help.tmp
echo The script connects to all DBMS via iimonitor, which may not  >> help.tmp
echo respond if there is a problem.In this case use ^C to interrupt. >> help.tmp
echo. >> help.tmp
echo To report on the state of custom DBMS server classes edit inglogs.bat >> help.tmp
echo and look for the line with CUSTOM. Replace CUSTOM with your server >> help.tmp
echo class name >> help.tmp
echo. >> help.tmp
echo The following information is collected: >> help.tmp
echo. >> help.tmp
echo module systemconfig: >> help.tmp
echo   set >> help.tmp
echo   ver >> help.tmp
echo   systeminfo  >> help.tmp
echo module systemstate: >> help.tmp
echo   tasklist /v  >> help.tmp
echo   tasklist /svc  >> help.tmp
echo   net start  >> help.tmp
echo   netstat -a >> help.tmp
echo module ingconfig: >> help.tmp
echo   config.dat, config.log, protect.dat,symbol.tbl, symbol.log >> help.tmp
echo   ingprenv >> help.tmp
echo   version.rel, version.dat >> help.tmp
echo   syscheck -v >> help.tmp
echo module inglogfiles: >> help.tmp
echo   errlog.log, iircp.log, iiacp.log >> help.tmp
echo   dbms logs >> help.tmp
echo module ingstate: >> help.tmp
echo   logstat, logstat -verbose >> help.tmp
echo   lockstat, lockstat -dirty >> help.tmp
echo   iimonitor: show all sessions, show all sessions formatted >> help.tmp
echo   iimonitor: show mutex x >> help.tmp
echo   iinamu >> help.tmp
echo. >> help.tmp
echo Flags: >> help.tmp
echo -no^<modulename^> >> help.tmp
echo   (-nosystemconfig -nosystemstate -nologfiles -noingresconfig -noingresstate) >> help.tmp
echo   By default all the above modules are executed, you can switch off >> help.tmp
echo   one or multiple modules by using -no^<modulename^> ...  >> help.tmp
echo. >> help.tmp
echo -logdump >> help.tmp
echo   By default inglogs does not perform a logdump as the output can be very >> help.tmp
echo   large. You can tell it to do a logdump with -logdump >> help.tmp
echo. >> help.tmp
echo -help (-h) >> help.tmp
echo   Prints out a short usage message and exits >> help.tmp
echo. >> help.tmp
echo --help >> help.tmp
echo   Prints out this message and exits >> help.tmp
echo. >> help.tmp
echo --version (-v, -version) >> help.tmp
echo   Prints out the version of inglogs and exits >> help.tmp
echo. >> help.tmp
more help.tmp
DEL help.tmp
