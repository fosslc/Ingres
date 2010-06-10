@echo off
REM
REM  Name: buildenv.bat
REM
REM  Description:
REM	Similar to UNIX's buildenv, this batch file sets up a build
REM	environment. It should be run within a Command Prompt whereby the
REM	compiler environment is set up beforehand, as well as the UNIX tools.
REM
REM  History:
REM	08-jul-2004 (somsa01)
REM	    Created.
REM	02-Aug-2004 (kodse01)
REM	    Modified to support Cygwin.
REM	02-Sep-2004 (somsa01)
REM	    More updates for Open Source.
REM	23-sep-2004 (somsa01)
REM	    Added vdba into the PATH. Also, added making of VERS file.
REM	06-oct-2004 (somsa01)
REM	    ing_tools is now just "tools", located in ING_ROOT. Also, added
REM	    new variables used by buildrel.
REM	14-oct-2004 (somsa01)
REM	    Updated Xerces look and feel, to match binary drop.
REM	19-oct-2004 (somsa01)
REM	    Make sure we set DOUBLEBYTE to ON.
REM	29-oct-2004 (somsa01)
REM	    Updated name of dotnet directory.
REM	29-oct-2004 (somsa01)
REM	    Place the MKSLOC path in front of the existing path.
REM	31-oct-2004 (somsa01)
REM	    Replace VISUALC setting in Jamdefs.int_w32 if it is not the
REM	    expected location.
REM     20-apr-2005 (loera01) SIR 114358
REM         Added check for KRB5HDR.
REM	26-May-2005 (drivi01)
REM	     Slightly modified creation of ING_TOOLS directories.
REM	03-Jan-2007 (drivi01)
REM	     Added VS2005 and VS2005SDK variables to point to Visual Studio 2005 and SDK.
REM	24-Jan-2007 (drivi01)
REM	     Added another variable to be set called MT that points to MS Manifest Tool.
REM     04-Jan-2008 (clach04)
REM          Added check for mt.exe (%MT%) existence, if it is not there unset MT
REM          This is needed for build systems that do not have Visual Studio 2005
REM          installed (e.g. do not want a Vista or .NET Data Provider built).
REM	22-Feb-2008 (smeke01)
REM	     Don't prompt for II_SYSTEM, DEBUG, XERCES or KRB5HDR if these are 
REM	     already set in the environment. Handle MT better - put out an 
REM	     informative message if mt.exe not found. Allow caller to specify 
REM	     Cygwin use, so that bldenv doesn't have to check for it.
REM      7-Mar-2008 (hanal04) Bug 119979
REM          Double quote if exist of %MT%, otherwise we get a failure
REM          when the executable does exist.
REM	12-Mar-2008 (drivi01)
REM	     Add MSREDIST and CAZIPXP variables to the script.
REM     14-Mar-2008 (hanal04)
REM          When an invalid directory is entered for MSREDIST and CAZIPXP
REM          we go into a loop reporting that the entered directory does not
REM          exist. The problem also exists for other variables.
REM      1-May-2008 (lunbr01)  bug 120417
REM          Directories with embedded space(s) cause errors, so enclose in
REM          double quotes ("") as needed. Enclose paths following 
REM          "if [not] exists", to prevent error running bldenv.bat:
REM          "The system cannot find the path specified.".
REM	09-Jun-2008 (drivi01)
REM	     Update XERCES path to the path of version
REM	     2.7 of the xerces.
REM	27-Oct-2008 (whiro01)
REM	     Update the default location of MSREDIST to match the location in Piccolo.
REM	04-May-2009 (drivi01)
REM	     Update the setup file with new compiler information in efforts
REM	     to port to Visual Studio 2008 compiler.
REM	25-Jan-2010 (kschendel) b123194
REM	    Changes to make Windows build against VS 2008 + Cygwin, with
REM	    no MKS and no old compiler leftovers.
REM	    - Add default for DEBUG (ie OFF).
REM	    - Change cazipxp default to where it really is in the Ingres
REM	    download, ie %ING_ROOT%\cazipxp.
REM	    - Remove sed stuff related to msvcdir and jamdefs.int_w32, no
REM	    longer relevant.
REM	    - Remove unreachable lines re PSDK, VS2005.
REM	    - Define MT if it's out there in the compiler directories.
REM	    - The default XERCES dir didn't match anything, neither what
REM	    was in the (current) 10.0 download, nor the apache.org download.
REM	    Make the default match the 3.0.1 apache.org download.
REM	    - Use windows NUL: rather than MK C:/nul.
REM	    - Only set II_JDK_HOME stuff in include/lib paths if it's set
REM	    (by the outside, it's not normally set).
REM	    - .NET stuff is now in dotnet2_win, fix here.
REM 22-Mar-2010 (troal01)
REM      Add GEOSROOT to point to where the GEOS binaries and headers are
REM	07-Apr-2010 (drivi01)
REM	     Add routines for setting up environment on x64.
REM	     Use cygwin shell for the build instead of MKS shell.  MKS shell
REM	     gets an error.
REM     14-may-2010 (maspa05)
REM          New versions of flex (>=2.5.31) require the --nounistd flag to 
REM          work correctly on Windows. Earlier versions (typically 2.5.4) will 
REM          fail with an unrecognised flag. Store the flag in %FLEX_FLAG%
REM          which is then appended to FLEX by Jamdefs.
REM          If FLEX_FLAG is already set then use that value, otherwise set
REM          it based on the detected version of flex.
REM

:TRUNK_LOOP
if "%ING_ROOT%"=="" SET /P ING_ROOT=Root Ingres location (default is %CD:\src\tools\port\jam=%): 
if "%ING_ROOT%"=="" SET ING_ROOT=%CD:\src\tools\port\jam=%
if not exist "%ING_ROOT%" echo Directory %ING_ROOT% does not exist& SET ING_ROOT=& goto TRUNK_LOOP

SET ING_SRC=%ING_ROOT%\src
if not exist "%ING_SRC%\bin" mkdir %ING_SRC%\bin
if not exist "%ING_SRC%\lib" mkdir %ING_SRC%\lib

if "%II_SYSTEM%"=="" SET /P II_SYSTEM=Root build location (default is %ING_ROOT%\build): 
if "%II_SYSTEM%"=="" SET II_SYSTEM=%ING_ROOT%\build
if not exist "%II_SYSTEM%" mkdir %II_SYSTEM%
SET ING_BUILD=%II_SYSTEM%\ingres

:DEBUG_SET
if "%DEBUG%"=="" SET /P DEBUG=DEBUG build status (ON or OFF, default OFF):
if "%DEBUG%"=="" SET DEBUG=OFF
if "%DEBUG%"=="OFF" SET DEBUG=& goto MSVC_LOOP
if "%DEBUG%"=="ON" goto MSVC_LOOP
echo Invalid setting for DEBUG.
goto DEBUG_SET

:MSVC_LOOP
if "%MSVCDir%"=="" SET INCLUDE=
if "%MSVCDir%"=="" SET LIB=
if "%MSVCDir%"=="" SET /P MSVCLoc=Root location of the Microsoft Visual Studio 2008 compiler (default is C:\Program Files\Microsoft Visual Studio 9.0): 
if "%MSVCLoc%"=="" SET MSVCLoc=C:\Program Files\Microsoft Visual Studio 9.0
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" if not exist "%MSVCLoc%\VC\vcvarsall.bat" echo Invalid compiler path specified. & goto MSVC_LOOP
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" if "%MSVCDir%"=="" call "%MSVCLoc%\VC\vcvarsall.bat" amd64 & set MSVCLoc=
if "%PROCESSOR_ARCHITECTURE%"=="x86" if not exist "%MSVCLoc%\Common7\Tools\vsvars32.bat" echo Invalid compiler path specified. & goto MSVC_LOOP
if "%PROCESSOR_ARCHITECTURE%"=="x86" if "%MSVCDir%"=="" call "%MSVCLoc%\Common7\Tools\vsvars32.bat"& set MSVCLoc=
@echo on
if NOT "%WindowsSdkDir%"=="" if x%MT%==x SET MT="%WindowsSdkDir%\bin\mt.exe" & set PATH=%WindowsSdkDir%\bin;%PATH%
@echo off
goto OTHERS

:OTHERS
set MSVCLoc=
SET JAM_TOOLSET=VISUALC
SET ING_SRCRULES=%ING_SRC%\tools\port\jam\Jamrules

:XERCES_LOOP
if "%XERCESCROOT%"=="" SET /P XERCESCROOT=Root location of Xerces source (default is %ING_ROOT%\xerces-c-3.0.1-x86-windows-vc-9.0):
if "%XERCESCROOT%"=="" SET XERCESCROOT=%ING_ROOT%\xerces-c-3.0.1-x86-windows-vc-9.0
if not exist "%XERCESCROOT%" echo %XERCESCROOT% does not exist& SET XERCESCROOT=& goto XERCES_LOOP
SET XERCESLOC=%XERCESCROOT%\lib

REM Jamdefs default xercvers is 2_7, if you downloaded xerces you probably
REM have a newer one.  Look in %XERCESCROOT%\bin and the XERCVERS should match
REM the xerces-c_X_Y.dll name, e.g. xerces-c_2_8.dll -> XERCVERS = 2_8.
if exist "%XERCESCROOT%\bin\xerces-c_2_7.dll" set XERCVERS=2_7
if exist "%XERCESCROOT%\bin\xerces-c_2_8.dll" set XERCVERS=2_8
if exist "%XERCESCROOT%\bin\xerces-c_3_0.dll" set XERCVERS=3_0
if "%XERCVERS"=="" echo XERCVERS could not be determined
echo XERCVERS=  %XERCVERS%

:GEOS_LOOP
if "%GEOSROOT%"=="" SET /P GEOSROOT=Root location of GEOS source and lib (default is %ING_ROOT%\geos):
if "%GEOSROOT%"=="" SET GEOSROOT=%ING_ROOT%\geos
if not exist "%GEOSROOT%" echo %GEOSROOT% does not exist& SET GEOSROOT=& goto GEOS_LOOP
SET GEOS_LOC=%GEOSROOT%\lib
SET GEOS_INC=%GEOSROOT%\inc
SET PATH=%GEOS_LOC%;%PATH%

:KERBEROS_LOOP
if "%KRB5HDR%"=="" SET /P KRB5HDR=Location of Kerberos header files (default is %ING_ROOT%\Kerberos5):
if "%KRB5HDR%"=="" SET KRB5HDR=%ING_ROOT%\Kerberos5
if not exist "%KRB5HDR%" echo %KRB5HDR% does not exist& SET KRB5HDR=& goto KERBEROS_LOOP
goto CAZIPXP

:CAZIPXP
if "%CAZIPXP%"=="" SET /P CAZIPXP=Location of cazipxp (default is %ING_ROOT%\cazipxp):
if "%CAZIPXP%"=="" SET CAZIPXP=%ING_ROOT%\cazipxp
if not exist "%CAZIPXP%" echo %CAZIPXP% does not exist& SET CAZIPXP=& goto CAZIPXP

REM Check if Cygwin is installed
if "%USE_CYGWIN%"=="TRUE" goto FLEX_SET
SET USE_CYGWIN=
which cygpath.exe > nul: 2>&1
if errorlevel 1 goto CHK_UNXLOC
SET USE_CYGWIN=TRUE

:CHK_UNXLOC
which ls.exe > nul: 2>&1
if errorlevel 1 goto GET_UNXLOC
printf "SET MKSLOC=" > settmp.bat
if "%USE_CYGWIN%"=="" which ls.exe | sed "s:/:\\:g" | sed "s:\\ls.exe::g" >> settmp.bat& goto CALLSETTMP
which ls.exe | cygpath -am -f- | sed "s:/:\\:g" | sed "s:\\ls.exe::g" >> settmp.bat

:CALLSETTMP
call settmp.bat
rm settmp.bat
goto FLEX_SET

:GET_UNXLOC
SET /P MKSLOC=Root location of the UNIX tools: 
if not exist "%MKSLOC%\ls.exe" echo This location does not contain the appropriate set of UNIX tools.& goto GET_UNXLOC
SET PATH=%WindowsSdkDir%\bin;%MKSLOC%;%PATH%

:FLEX_SET
REM if FLEX_FLAG is not set then set according to version of flex 
if NOT "%FLEX_FLAG%"=="" goto SHELL_SET
SET FLEX_FLAG=--nounistd
printf "SET FLEXVER=" > settmp.bat
flex --version | cut -d. -f3 >> settmp.bat
call settmp.bat
rm settmp.bat
REM note the following line end in FLEX_FLAG={space} - this is important as it
REM sets an empty string rather than unset the variable
if %FLEXVER% LSS 31 SET FLEX_FLAG= 
SET FLEXVER=

:SHELL_SET
@echo on
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" if "%USE_CYGWIN%"=="" SET /P SHELL=Full path to the cygwin shell (default C:\cygwin\bin\sh.exe):
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" if "%USE_CYGWIN%"=="" if "%SHELL%"=="" set SHELL=c:\cygwin\bin\sh.exe & goto AWK_SET
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" if "%USE_CYGWIN%"=="" if NOT "%SHELL%"=="" goto AWK_SET
if exist "%MKSLOC%\sh.exe" SET SHELL=%MKSLOC%\sh.exe& goto AWK_SET
if exist "%MKSLOC%\zsh.exe" SET SHELL=%MKSLOC%\zsh.exe

:AWK_SET
@echo off
REM prefer gawk, as awk.exe on Cygwin is a symlink and cmd doesn't like it.
if exist "%MKSLOC%\gawk.exe" SET AWK_CMD=gawk& goto MISC
if exist "%MKSLOC%\awk.exe" SET AWK_CMD=awk

:MISC
SET DD_RSERVERS=%II_SYSTEM%\ingres\rep
SET INCLUDE=%II_SYSTEM%\ingres\files;%include%
SET LIB=%II_SYSTEM%\ingres\lib;%lib%
if not "%II_JDK_HOME%"=="" SET INCLUDE=%INCLUDE%;%II_JDK_HOME%\include;%II_JDK_HOME%\include\win32
if not "%II_JDK_HOME%"=="" SET LIB=%LIB%;%II_JDK_HOME%\lib
SET HOSTNAME=%COMPUTERNAME%
SET ING_DOC=%ING_SRC%\tools\techpub\pdf
SET DBCS=DOUBLEBYTE
SET DOUBLEBYTE=ON
SET II_RELEASE_DIR=%ING_ROOT%\release
SET II_CDIMAGE_DIR=%ING_ROOT%\cdimage
SET II_MM_DIR=%ING_ROOT%\mergemodules
if not exist "%II_RELEASE_DIR%" mkdir %II_RELEASE_DIR%
if not exist "%II_CDIMAGE_DIR%" mkdir %II_CDIMAGE_DIR%
if not exist "%II_MM_DIR%" mkdir %II_MM_DIR%

:MISC2
SET CPU=%PROCESSOR_ARCHITECTURE%
if "%CPU%"=="x86" SET CPU=i386
if "%CPU%"=="i386" SET config_string=int_w32
if "%CPU%"=="IA64" SET config_string=i64_win
if "%CPU%"=="AMD64" SET config_string=a64_win

if exist "%ING_SRC%\tools\port\conf\VERS" goto VERS_EXIST
cp %ING_SRC%\tools\port\conf\VERS.%config_string% %ING_SRC%\tools\port\conf\VERS

:VERS_EXIST
SET TESTTOOLS=%ING_ROOT%\tools
if not exist "%TESTTOOLS%" mkdir %TESTTOOLS%\bin& mkdir %TESTTOOLS%\files& mkdir %TESTTOOLS%\man1
if not exist "%TESTTOOLS%\bin" mkdir %TESTTOOLS%\bin 
if not exist "%TESTTOOLS%\files" mkdir %TESTTOOLS%\files 
if not exist "%TESTTOOLS%\man1" mkdir %TESTTOOLS%\man1

SET PATH=%ING_SRC%\bin;%II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility;%II_SYSTEM%\ingres\vdba;%PATH%

title Ingres Build Window - ING_ROOT=%ING_ROOT%

REM
REM This stuff is for SEP
REM

set ING_TST=%ING_ROOT%\tst
set TECHDOC=
set ING_REL=2.5
set TST_OUTPUT=%II_SYSTEM%\output
set ING_TOOLS=%ING_ROOT%\tools
set TERM_INGRES=ibmpc
set TST_ROOT_OUTPUT=%TST_OUTPUT%

REM This variable should be set to the ingres user's password
SET QASETUSER_INGRESPW=

REM This variable controls how long SEP delays before diff'ing canons
SET SEP_DIFF_SLEEP=10

REM
REM The rest of this file should not be modified!!
REM

if "%ING_TOOLS%"=="" goto SETTOOLS2
set TOOLS_DIR=%ING_TOOLS%
set TST_SEP=%ING_TOOLS%\files
set PEDITOR=%ING_TOOLS%\bin\peditor.exe
set TST_TOOLS=%ING_TOOLS%\bin
goto CONTINUE

:SETTOOLS2
if "%ING_SRC%"=="" goto CONTINUE
set TST_SEP=%ING_SRC%\testtool\sep\files
set PEDITOR=%ING_SRC%\bin\peditor.exe

:CONTINUE
if not "%ING_TST%"=="" set SEPPARAM_SYSTEM=%OS%
if "%ING_EDIT%"=="" set ING_EDIT=%WINDIR%\system32\notepad.exe

if not "%TST_OUTPUT%"=="" goto CONTINUE2
if not "%ING_TST%"=="" set TST_OUTPUT=%ING_TST%\output

:CONTINUE2
if "%ING_TST%"=="" goto CONTINUE3
set TST_TESTENV=%ING_TST%
set TST_TESTOOLS=%ING_TST%\testtool
set TST_CFG=%ING_TST%\suites\handoffqa
set TST_SHELL=%ING_TST%\suites\bat
set TST_LISTEXEC=%ING_TST%\suites\handoffqa
set TST_DCL=%TST_SHELL%
set TST_INIT=%ING_TST%\basis\init
set path=%path%;%TST_SHELL%

:CONTINUE3
REM Regression directories
if "%ING_TST%"=="" goto CONTINUE4
set TST_FE=%ING_TST%\fe
set TST_BE=%ING_TST%\be
set TST_ACCESS=%ING_TST%\be\access
set TST_ACCNTL=%ING_TST%\be\accntl
set TST_DATATYPES=%ING_TST%\be\datatypes
set TST_QRYPROC=%ING_TST%\be\qryproc
set TST_GCF=%ING_TST%\gcf
set TST_NETTEST=%TST_GCF%\gcc

REM Stress Directories
set TST_STRESS=%ING_TST%\stress
set TST_POOL=%TST_STRESS%\pool
set TST_PROPHECY=%TST_STRESS%\prophecy
set TST_MAYO=%TST_STRESS%\mayo
set TST_FIDO=%TST_STRESS%\fido
set TST_REGRESSION=%TST_STRESS%\regression
set TST_GATEWAY=%TST_STRESS%\gateway
set TST_CALLT=%TST_STRESS%\callt
set TST_BUGS=%TST_STRESS%\bugs
set TST_ACTEST=%TST_STRESS%\actest
set TST_ITB=%TST_STRESS%\itb

:CONTINUE4
REM These are also needed.
set ING_BUILD=%II_SYSTEM%\ingres
set II_CONFIG=%II_SYSTEM%\ingres\files

set II_ABF_RUNOPT=nondynamic
if not exist "%II_CONFIG%\termcap" goto ELSE
set II_TERMCAP_FILE=%II_CONFIG%\termcap
goto CONTINUE5

:ELSE
set II_TERMCAP_FILE=

:CONTINUE5
REM if not "%ING_SRC%"=="" set path=%path%;%ING_SRC%\bin
if not "%ING_TOOLS%"=="" set path=%path%;%ING_TOOLS%\bin

REM Replicator
set TST_REP=%ING_TST%
set REP_TST=%TST_REP%

set lib=%lib%;%II_SYSTEM%\ingres\lib

REM Show what we have...
echo.
which cazipxp.exe > nul: 2>&1
if errorlevel 1 echo Warning: Please place cazipxp.exe in your path, or obtain it from the CA Open& echo          Source website.
if not exist "%ING_SRC%\common\dotnet2_win\specials\keypair.snk" echo Warning: Please follow the README instructions to create the strong name key& echo          file, required in building the .NET Data Provider, located in& echo          %ING_SRC%\common\dotnet2_win\specials.
which IsCmdBld.exe > nul: 2>&1
if errorlevel 1 echo Warning: The InstallShield Developer command-line packaging tool (IsCmdBld.exe)& echo          does not seem to be in the path.
echo.
echo 	Welcome to %COMPUTERNAME%/%OS% Ingres Build

echo.
echo II_SYSTEM    = %II_SYSTEM%
if not "%ING_SRC%"=="" echo ING_SRC      = %ING_SRC%
if not "%ING_TST%"=="" echo ING_TST      = %ING_TST%
if not "%ING_TOOLS%"=="" echo ING_TOOLS    = %ING_TOOLS%
echo.
