@echo off
if "%INSTALL_DEBUG%" == "DEBUG" echo on
REM 
@REM   Copyright (c) 1995, 1997 Ingres Corporation
REM 
REM   Name:
REM 	iisustrt
REM 
REM   Usage:
REM 	iisustrt - called from the CA-installer.
REM 
REM   Description:
REM 	Start the setup batch files vi ipsetup.bat and start setperm.exe.
REM 
REM 
REM 
REM  History:
REM 	21-jul-95 (reijo01)
REM 		Created
REM 	09-aug-95 (reijo01)
REM 		Remove the call to setperm. Now called from iisudbms.bat.
REM	05-sep-1995 (canor01)
REM		Remove debugging statement
REM	11-sep-1995 (bocpa01)
REM		II_SYSTEM is not inherited from Installer and needs to be 
REM		set explicitly.
REM	05-sep-1995 (canor01)
REM		Change the move to a copy and delete.
REM	25-sep-1995 (reijo01)
REM		Added a path statement to fix a problem when Vision and
REM		ESQL/C is installed after Net had been installed.
REM	15-nov-95 (tutto01)
REM		Redirect the output of the copy to NUL.  Eliminates
REM		"1 file(s) copied" message.
REM	02-dec-95 (tutto01)
REM		Rewritten for use with new installer.
REM	22-dec-95 (tutto01)
REM		Restore temporary names of api files to originals.
REM	10-jan-96 (tutto01)
REM		Set II_TEMPORARY here instead of iisudbms.
REM     31-jan-96 (tutto01)
REM         Added full path to executables for Windows 95 install, which
REM         did not "see" modified path setting.  Also added "slack" variable
REM	    clearing for II_TEMP.
REM	01-feb-96 (tutto01)
REM	    Added II_NUM_PROCESSORS.
REM	06-feb-1996 (tutto01)
REM	    Removed garbage left from previous integration.
REM	25-feb-1996 (tutto01)
REM	    Fixed TIMEZONE typo, and make sure that symbol.tbl gets default
REM	    settings.  Also ensure that II_SYSTEM is set.
REM	27-feb-1996 (tutto01)
REM	    Added setup for Desktop.
REM	01-apr-1996 (tutto01)
REM	    Added call for ICE setup.
REM	02-apr-1996 (tutto01)
REM	    Removed lines which were setting dual log variables inappropriately
REM	    during an upgrade install.
REM	24-apr-1996 (tutto01)
REM	    Allow for post-installation setup of the dbms and net.
REM	24-jul-1996 (somsa01)
REM	    Put back call to iisuc2.bat; C2 security on NT has been fixed.
REM	25-jul-96 (emmag)
REM	    Set release_id to correct value.  1201intwnt00
REM	31-Jul-96 (somsa01)
REM	    Added service setup if DBMS or NET is installed, and if OS is NT.
REM	18-Nov-96 (somsa01)
REM	    Bug #78737; in the setting of the character set, an "ingprenv" was
REM	    being done on "II_CHARSET". It should be
REM	    "II_CHARSET%II_INSTALLATION%".
REM	19-Nov-96 (somsa01)
REM	    The original log file size should be taken from config.dat, as well
REM	    as the number of concurrent users and the network protocols. Also,
REM	    added II_DECNET and properly set the value for II_DUAL_ENABLED.
REM	19-Nov-96 (somsa01)
REM	    If the install is a client installation, then use opingclient to
REM	    install the client service, rather than opingsvc (for DBMS).
REM	06-Jan-97 (somsa01)
REM	    Set RELEASE_ID to oping1201intwnt01.
REM	04-Feb-97 (somsa01)
REM	    Added running of OpenROAD setup.
REM	21-Feb-97 (mcgem01)
REM	    Same service code applies for both DBMS and Net.
REM	11-Mar-97 (mcgem01)
REM	    Set RELEASE_ID to oping2000intwnt00
REM	09-apr-97 (mcgem01)
REM	    Change product name to OpenIngres.
REM	15-jun-97 (mcgem01)
REM	    Remove the 1.2 VDBA and OpenIngres services.
REM	16-jun-97 (mcgem01)
REM	    Set up the replicator if installed.
REM	05-nov-97 (mcgem01)
REM	    Update the release id for the MR.
REM	15-nov-1997 (canor01)
REM	    Log file size is now in kbytes.  Quote pathnames for support of
REM	    embedded spaces.
REM	15-dec-97 (mcgem01)
REM	    Add the protocol bridge.
REM	10-apr-98 (mcgem01)
REM	    Product name change from OpenIngres to Ingres.
REM


REM Make sure that II_SYSTEM is set...
REM 
if not "%II_SYSTEM%" == "" goto II_SYS_OK
echo.
echo The environment variable II_SYSTEM must be set to the location of the
echo Ingres installation.
echo.
echo ie.  set II_SYSTEM=c:\oping
echo.
goto EXIT
:II_SYS_OK

REM Check for post installation setup ...
if "%1" == "" goto DOSETUP
if "%1" == "-DBMS" goto SETDBMS
if "%1" == "-dbms" goto SETDBMS
if "%1" == "-NET" goto SETNET
if "%1" == "-net" goto SETNET

echo IISUSTRT: Invalid argument
goto EXIT

:SETDBMS
set II_PROD_DBMS=INSTALLED
goto DOSETUP

:SETNET
set II_PROD_NET=INSTALLED
goto DOSETUP


:DOSETUP
@echo Setting up installed components...
@echo.

REM Clear the setting of II_TEMP.  This is how we are freeing environment
REM space for the install on Windows 95, whose environment we can't seem
REM to expand during an install.  So what happens now is, the parent process
REM of the install sets II_TEMP to be a fairly large variable, and clearing
REM it here gives us the room we need!

set II_TEMP=

REM The following needed to find xcopy for ckpdb
REM
path %II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility;%path%;%systemroot%\system32


REM Make sure all environment variables are set that should be...
REM
if "%II_SETTINGS%" == "NEW" goto SKIP_SET

call "%II_SYSTEM%\ingres\bin\ipset" II_TIMEZONE_NAME "%II_SYSTEM%\ingres\bin\ingprenv" II_TIMEZONE_NAME
if not "%II_TIMEZONE_NAME%" == "" goto SET1
set II_TIMEZONE_NAME=NA-EASTERN
"%II_SYSTEM%\ingres\bin\ingsetenv" II_TIMEZONE_NAME %II_TIMEZONE_NAME%
:SET1

call "%II_SYSTEM%\ingres\bin\ipset" II_LANGUAGE "%II_SYSTEM%\ingres\bin\ingprenv" II_LANGUAGE
if not "%II_LANGUAGE%" == "" goto SET2
set II_LANGUAGE=ENGLISH
"%II_SYSTEM%\ingres\bin\ingsetenv" II_LANGUAGE %II_LANGUAGE%
:SET2

call "%II_SYSTEM%\ingres\bin\ipset" II_INSTALLATION "%II_SYSTEM%\ingres\bin\ingprenv" II_INSTALLATION
if not "%II_INSTALLATION%" == "" goto SET3
set II_INSTALLATION=II
"%II_SYSTEM%\ingres\bin\ingsetenv" II_INSTALLATION %II_INSTALLATION%
:SET3

call "%II_SYSTEM%\ingres\bin\ipset" II_CHARSET "%II_SYSTEM%\ingres\bin\ingprenv" II_CHARSET%II_INSTALLATION%
if not "%II_CHARSET%" == "" goto SET4
set II_CHARSET=IBMPC850
"%II_SYSTEM%\ingres\bin\ingsetenv" II_CHARSET %II_CHARSET%
:SET4

call "%II_SYSTEM%\ingres\bin\ipset" TERM_INGRES "%II_SYSTEM%\ingres\bin\ingprenv" TERM_INGRES
if not "%TERM_INGRES%" == "" goto SET5
set TERM_INGRES=IBMPC
"%II_SYSTEM%\ingres\bin\ingsetenv" TERM_INGRES %TERM_INGRES%
:SET5

call "%II_SYSTEM%\ingres\bin\ipset" II_CONC_USERS "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.dbms."*".connect_limit
if not "%II_CONC_USERS%" == "" goto SET6
set II_CONC_USERS=16
"%II_SYSTEM%\ingres\bin\ingsetenv" II_CONC_USERS %II_CONC_USERS%
:SET6

call "%II_SYSTEM%\ingres\bin\ipset" dual_check "%II_SYSTEM%\ingres\bin\ingprenv" II_DUAL_LOG
if not "%dual_check%" == "" set II_DUAL_ENABLED=ON& goto SET7
set II_DUAL_ENABLED=OFF
:SET7

set dual_check=
call "%II_SYSTEM%\ingres\bin\ipset" II_DUAL_LOG "%II_SYSTEM%\ingres\bin\ingprenv" II_DUAL_LOG
if not "%II_DUAL_LOG%" == "" goto SET8
set II_DUAL_LOG=%II_SYSTEM%
:SET8

call "%II_SYSTEM%\ingres\bin\ipset" II_DUAL_LOG_NAME "%II_SYSTEM%\ingres\bin\ingprenv" II_DUAL_LOG_NAME
if not "%II_DUAL_LOG_NAME%" == "" goto SET9
set II_DUAL_LOG_NAME=dual.log
:SET9

REM OOPS, forgot SET10

call "%II_SYSTEM%\ingres\bin\ipset" II_LOG_FILE "%II_SYSTEM%\ingres\bin\ingprenv" II_LOG_FILE
if not "%II_LOG_FILE%" == "" goto SET11
set II_LOG_FILE=%II_SYSTEM%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_LOG_FILE "%II_LOG_FILE%"
:SET11

call "%II_SYSTEM%\ingres\bin\ipset" II_LOG_FILE_NAME "%II_SYSTEM%\ingres\bin\ingprenv" II_LOG_FILE_NAME
if not "%II_LOG_FILE_NAME%" == "" goto SET12
set II_LOG_FILE_NAME=ingres.log
"%II_SYSTEM%\ingres\bin\ingsetenv" II_LOG_FILE_NAME %II_LOG_FILE_NAME%
:SET12

call "%II_SYSTEM%\ingres\bin\ipset" II_LOG_FILE_KBYTES "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.rcp.file.kbytes
if not "%II_LOG_FILE_KBYTES%" == "" goto SET13
call "%II_SYSTEM%\ingres\bin\ipset" II_LOG_FILE_SIZE "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.rcp.file.size
if "%II_LOG_FILE_SIZE%" == "" goto SET12a
set /A II_LOG_FILE_KBYTES=%II_LOG_FILE_SIZE%/1024
"%II_SYSTEM%\ingres\bin\ingsetenv" II_LOG_FILE_KBYTES %II_LOG_FILE_KBYTES%
goto SET13

:SET12a
set II_LOG_FILE_KBYTES=16384
"%II_SYSTEM%\ingres\bin\ingsetenv" II_LOG_FILE_KBYTES %II_LOG_FILE_KBYTES%

:SET13

call "%II_SYSTEM%\ingres\bin\ipset" II_DATABASE "%II_SYSTEM%\ingres\bin\ingprenv" II_DATABASE
if not "%II_DATABASE%" == "" goto SET14
set II_DATABASE=%II_SYSTEM%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_DATABASE "%II_DATABASE%"
:SET14

call "%II_SYSTEM%\ingres\bin\ipset" II_CHECKPOINT "%II_SYSTEM%\ingres\bin\ingprenv" II_CHECKPOINT
if not "%II_CHECKPOINT%" == "" goto SET15
set II_CHECKPOINT=%II_SYSTEM%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_CHECKPOINT "%II_CHECKPOINT%"
:SET15

call "%II_SYSTEM%\ingres\bin\ipset" II_JOURNAL "%II_SYSTEM%\ingres\bin\ingprenv" II_JOURNAL
if not "%II_JOURNAL%" == "" goto SET16
set II_JOURNAL=%II_SYSTEM%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_JOURNAL "%II_JOURNAL%"
:SET16

call "%II_SYSTEM%\ingres\bin\ipset" II_WORK "%II_SYSTEM%\ingres\bin\ingprenv" II_WORK
if not "%II_WORK%" == "" goto SET17
set II_WORK=%II_SYSTEM%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_WORK "%II_WORK%"
:SET17

call "%II_SYSTEM%\ingres\bin\ipset" II_DUMP "%II_SYSTEM%\ingres\bin\ingprenv" II_DUMP
if not "%II_DUMP%" == "" goto SET18
set II_DUMP=%II_SYSTEM%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_DUMP "%II_DUMP%"
:SET18

call "%II_SYSTEM%\ingres\bin\ipset" II_WINTCP "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.gcc."*".wintcp.status
if not "%II_WINTCP%" == "" goto SET19
set II_WINTCP=ON
"%II_SYSTEM%\ingres\bin\ingsetenv" II_WINTCP %II_WINTCP%
:SET19

call "%II_SYSTEM%\ingres\bin\ipset" II_NVLSPX "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.gcc."*".nvlspx.status
if not "%II_NVLSPX%" == "" goto SET20
set II_NVLSPX=OFF
"%II_SYSTEM%\ingres\bin\ingsetenv" II_NVLSPX %II_NVLSPX%
:SET20

call "%II_SYSTEM%\ingres\bin\ipset" II_LANMAN "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.gcc."*".lanman.status
if not "%II_LANMAN%" == "" goto SET21
set II_LANMAN=OFF
"%II_SYSTEM%\ingres\bin\ingsetenv" II_LANMAN %II_LANMAN%
:SET21

call "%II_SYSTEM%\ingres\bin\ipset" II_DECNET "%II_SYSTEM%\ingres\utility\iigetres" ii.%COMPUTERNAME%.gcc."*".decnet.status
if not "%II_DECNET%" == "" goto SET22
set II_DECNET=OFF
"%II_SYSTEM%\ingres\bin\ingsetenv" II_DECNET %II_DECNET%
:SET22

call "%II_SYSTEM%\ingres\bin\ipset" II_NUM_OF_PROCESSORS "%II_SYSTEM%\ingres\bin\ingprenv" II_NUM_OF_PROCESSORS
if not "%II_NUM_OF_PROCESSORS%" == "" goto SET23
set II_NUM_OF_PROCESSORS=1
"%II_SYSTEM%\ingres\bin\ingsetenv" II_NUM_OF_PROCESSORS %II_NUM_OF_PROCESSORS%
:SET23

call "%II_SYSTEM%\ingres\bin\ipset" II_TEMPORARY "%II_SYSTEM%\ingres\bin\ingprenv" II_TEMPORARY
if not "%II_TEMPORARY%" == "" goto SET24
set II_TEMPORARY=%II_SYSTEM%\ingres\temp
"%II_SYSTEM%\ingres\bin\ingsetenv" II_TEMPORARY "%II_TEMPORARY%"
:SET24
if not exist "%II_TEMPORARY%" mkdir "%II_TEMPORARY%"

:SKIP_SET


REM Can now count on these variables being in your environment, so set them
REM in symbol.tbl
REM
"%II_SYSTEM%\ingres\bin\ingsetenv" II_INSTALLATION %II_INSTALLATION%
"%II_SYSTEM%\ingres\bin\ingsetenv" TERM_INGRES %TERM_INGRES%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_TIMEZONE_NAME %II_TIMEZONE_NAME%
"%II_SYSTEM%\ingres\bin\ingsetenv" II_NUM_OF_PROCESSORS %II_NUM_OF_PROCESSORS%

if not "%II_TEMPORARY%" == "" goto TEMPSET
set II_TEMPORARY=%II_SYSTEM%\ingres\temp
:TEMPSET
"%II_SYSTEM%\ingres\bin\ingsetenv" II_TEMPORARY "%II_TEMPORARY%"
if not exist "%II_TEMPORARY%" mkdir "%II_TEMPORARY%"


REM Set up version info...
REM
set RELEASE_ID=oping2001intwnt00
"%II_SYSTEM%\ingres\bin\ingsetenv" RELEASE_ID %RELEASE_ID%

REM set authorization string
REM
"%II_SYSTEM%\ingres\bin\ingsetenv" II_AUTH_STRING "%II_AUTH_STRING%"

REM rename the api header files if installed
REM 
if "%II_PROD_DBMS%" == "INSTALLED" goto FIXHEADERS
if "%II_PROD_NET%"  == "INSTALLED" goto FIXHEADERS
goto RUNSCRIPTS

:FIXHEADERS
if not exist "%II_SYSTEM%\ingres\demo\api\asc\asc.hh" goto HEADERSDONE
copy "%II_SYSTEM%\ingres\demo\api\asc\*.hh" "%II_SYSTEM%\ingres\demo\api\asc\*.h" >NUL
del  "%II_SYSTEM%\ingres\demo\api\asc\*.hh" >NUL
copy "%II_SYSTEM%\ingres\demo\api\syc\*.hh" "%II_SYSTEM%\ingres\demo\api\syc\*.h" >NUL
del  "%II_SYSTEM%\ingres\demo\api\syc\*.hh" >NUL
:HEADERSDONE

:RUNSCRIPTS
REM Run appropriate scripts
REM
if not "%II_PROD_DBMS%" == "INSTALLED" if not "%II_PROD_NET%" == "INSTALLED" goto SKIPSVCE
if not "%II_PLATFORM%" == "II_WINNT" goto SKIPSVCE
@echo WD Setting up the Ingres service...
call opingclient remove 1>NUL
call rmcser12 remove 1>NUL
call oi12svc remove 1>NUL
call opingsvc remove 1>NUL
call opingsvc install 1>NUL
@echo.

:SKIPSVCE
if not "%II_PROD_DBMS%" == "INSTALLED" goto SKIPDBMS
call "%II_SYSTEM%\ingres\utility\iisudbms.bat"

call "%II_SYSTEM%\ingres\utility\iisuc2.bat"

:SKIPDBMS
if not "%II_PROD_OIDT%" == "INSTALLED" goto SKIPOIDT
call "%II_SYSTEM%\ingres\utility\iisudt.bat"

:SKIPOIDT
if not "%II_PROD_NET%" == "INSTALLED" goto SKIPNET
call "%II_SYSTEM%\ingres\utility\iisunet.bat"
call "%II_SYSTEM%\ingres\utility\iisubr.bat"

:SKIPNET
if not "%II_PROD_ICE%" == "INSTALLED" goto SKIPICE
call "%II_SYSTEM%\ingres\utility\iisuice.bat"

:SKIPICE
if not "%II_PROD_JAS%" == "INSTALLED" goto SKIPJAS
call "%II_SYSTEM%\ingres\utility\iisujas.bat"

:SKIPJAS
if not "%II_PROD_STAR%" == "INSTALLED" goto SKIPSTAR
call "%II_SYSTEM%\ingres\utility\iisustar.bat"

:SKIPSTAR
if "%II_PROD_JAS%" == "INSTALLED" goto SKIPTM
call "%II_SYSTEM%\ingres\utility\iisutm.bat"

:SKIPTM
if not "%II_PROD_VISION%" == "INSTALLED" goto SKIPVISION
call "%II_SYSTEM%\ingres\utility\iisuabf.bat"

:SKIPVISION
if not "%II_PROD_OR%" == "INSTALLED" goto SKIPOR
call "%II_SYSTEM%\ingres\utility\iisuor.bat"

:SKIPOR
if not "%II_PROD_REP%" == "INSTALLED" goto SKIPREP
call "%II_SYSTEM%\ingres\utility\iisurep.bat"

:SKIPREP

echo.
echo Installation completed!
echo.

:EXIT
@echo on
