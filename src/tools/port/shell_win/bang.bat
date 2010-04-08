@echo off
REM
REM  Name:
REM	bang.bat
REM
REM  Description:
REM	This batch file builds EVERYTHING.  When done, every file needed
REM	for Ingres should be created.  The best use of this is
REM	following the use of "purge.bat" to wipe out all existing objects,
REM	libraries, etc.
REM
REM  Usage:
REM	Wipe clean your entire client.  "purge.bat" for example.  Wipe
REM	out your existing installation located at "%II_SYSTEM%."  To run
REM	this batch file, simply type "bang > bang.out" to log activity.
REM
REM  History:
REM	25-jan-96 (tutto01)
REM	    Created.
REM	15-may-1996 (fanra01)
REM	    Updated to work with a "DLL" build.
REM	27-sep-97 (mcgem01)
REM	    Add relocation of build files and change DLL names to OI.
REM	01-Apr-98 (rajus01)
REM	    Added "nmake -a" for libq, "call gd embed\libqgca ; nmake -a" ,
REM	    "call gd embed\sqlca; nmake -a", "call gd front; nmake -a" to
REM 	    make the complete newbuild go through fine.
REM	05-may-98 (mcgem01)
REM	    Add the ddf directories.
REM	05-may-1998 (canor01)
REM	    Quote pathnames to support embedded spaces.
REM     21-Jul-98 (fanra01)
REM         Add g4xfer build before oiembdnt as it is now included.
REM         Moved oiembdnt.lib as oifrsnt.dll requires it.
REM         Make deletion of precompilers depend on environment variable
REM         NEW_TOOLS
REM	15-sep-98 (mcgem01)
REM	    Clean up to remove unneeded steps such as copying icon files
REM	    to the bin, generating an auth string etc.
REM	15-sep-98 (mcgem01)
REM	    Allow for the bang script to build from a label report on
REM	    build options before proceeding with the build.
REM	18-sep-98 (mcgem01)
REM	    Need oiadfnt.dll to run aducompile
REM     23-sep-1998 (canor01)
REM         Add "-p" for "partial" build--do not get files from piccolo, just
REM         build what's on the machine.
REM	25-sep-98 (mcgem01)
REM	    In future, when cutting a label I will use the same label
REM	    name to for the treasure library so that we can keep the two
REM	    in sync.
REM     01-oct-1998 (canor01)
REM	    Move test for "-p" flag up so it will be checked.
REM     18-dec-1998 (cucjo01)
REM	    Make II_TEMPORARY directory.
REM	04-jan-1999 (mcgem01)
REM	    You *MUST* specifiy either, -p, a label or head_rev
REM	03-feb-1999 (somsa01)
REM	    Before going to common\gcf, make sure we make
REM	    front\frontcl\specials and front\frontcl\files, since we need
REM	    the header files.
REM	05-feb-1999 (somsa01)
REM	    After all is done, build the testtool stuff.
REM     10-Feb-99 (fanra01)
REM         Add full path for ntboot.
REM	28-jun-1999 (somsa01)
REM	    Added building of sig!dp!dp .
REM	30-aug-1999 (cucjo01)
REM	    Changed copysrc, copymake, copydirlist bat file calls to
REM	    nmake from the respective directories.
REM	19-nov-1999 (cucjo01)
REM	    Display time information in log file
REM	31-jan-2000 (mcgem01)
REM	    Build winstart after we've built the ingres.lib
REM	24-feb-2000 (somsa01)
REM	    Build ice!plays!src after building ingres.lib .
REM	25-feb-2000 (cucjo01)
REM	    Stop Ingres and IVM before starting the build.
REM	01-may-2000 (abbjo03)
REM	    Added building of sig!openapi!syncapiw.
REM	23-aug-2000 (cucjo01)
REM	    Updated ice and coldfusion cazip files
REM	28-nov-2000 (mcgem01)
REM	    Since the cazip utility embeds paths in the zip file, make sure
REM	    that we run zip from the local directory.
REM     04-Apr-2001 (peeje01)
REM         SIR 103810
REM         Added build of front!ice!icetranslate, depends on oiiceapi.lib
REM	13-apr-2001 (mcgem01)
REM	    There isn't anything to build in front\vdba\rmsvc any more.
REM	19-apr-2001 (somsa01)
REM	    Added making of tools/port/resource, front/st/perfwiz,
REM	    front/st/iilink, and front/st/iipostinst.
REM	17-may-2001 (somsa01)
REM	    Corrected CAZIPXP runs.
REM	05-jun-2001 (somsa01)
REM	    Added building of VCBF and adbtofst.exe from the vdba directory.
REM	    Also added proper making of sig!tz!tz.
REM	07-jun-2001 (somsa01)
REM	    Added making of sig!rep!rpclean.
REM	10-jun-2001 (somsa01)
REM	    Added making of front!vdba!vnodemgr.
REM	13-jun-2001 (somas01)
REM	    Added building of flex.exe after iyacc.exe, as it is needed by
REM	    ICE.
REM	05-jul-2001 (somsa01)
REM	    Do not build flex.exe for NT_IA64.
REM	07-dec-2001 (somsa01)
REM	    Win64 libraries have LIBSUFFIX in their names now.
REM	12-feb-2002 (somsa01)
REM	    Corrected building of the GUI tools.
REM     02-Jul-2002 (fanra01)
REM         Sir 108159
REM         Add building of imp in the sig directory.
REM	22-jul-2003 (somsa01)
REM	    Re-added making flex.exe on Windows 64-bit.
REM	02-oct-2003 (somsa01)
REM	    Ported to NT_AMD64.
REM	06-oct-2003 (penga03)
REM	    Add re-building oiapi.dll, oigcfnt.lib and common\bin.
REM	17-oct-2003 (somsa01)
REM	    We can now make ICETranslate on Windows 64-bit.
REM     31-dec-2003 (rigka01)
REM 	    add nmake -a gv.res 
REM	30-jan-2004 (somsa01)
REM	    unialscompile.exe and unimapcompile.exe are now prerequisites
REM	    for building common.
REM	14-may-2004 (penga03)
REM	    Re-build back after building dbutil.
REM	15-jun-2004 (somsa01)
REM	    Added wincluster.
REM	15-jun-2004 (drivi01)
REM	    Removed commands for building flex, will use the one from new
REM	    unix tools package.
REM	16-jun-2004 (somsa01)
REM	    Modified script to tolerate different lib prefixes.
REM	27-Oct-2004 (drivi01)
REM	    bat directories on windows were replaced with shell_win,
REM	    this script was updated to reflect that.
REM

if not "%II_SYSTEM%" == "" goto start
echo The environment variable II_SYSTEM is not set.
goto finished

:start

set LIBSUFFIX=
if "%CPU%" == "IA64" set LIBSUFFIX=64
if "%CPU%" == "AMD64" set LIBSUFFIX=64

set LIBPREFIX=ii

REM Stop Ingres and IVM before the build starts.
if exist "%II_SYSTEM%\ingres\utility\ingstop.exe" call "%II_SYSTEM%\ingres\utility\ingstop.exe" -k
if exist "%II_SYSTEM%\ingres\vdba\ivm.exe" call %II_SYSTEM%\ingres\vdba\ivm -stop


REM Report on the build configuration before starting.
@echo off
cls
echo.
echo Building Ingres II for Microsoft Windows NT Release 2.5
@echo.
@echo Build Start Time:
@time /t&date /t
@echo.

if not "%II_TEMPORARY%" == "" md "%II_TEMPORARY%"
if DEBUG == "" goto NOT_BUILDING_FOR_DEBUG
echo.
echo Debug:		ON
echo Licensing:	OFF
goto HEAD_REVS_OR_NOT
:NOT_BUILDING_FOR_DEBUG
echo.
echo Debug:		OFF
echo Licensing:	ON
:HEAD_REVS_OR_NOT
if "%1" == "-p" goto SOURCE_UP_TO_DATE
if not "%1" == "" goto BANGING_FROM_LABEL
echo Label:		Head Revision 
goto DONE_WITH_INFO
:BANGING_FROM_LABEL
echo Label:		%1
:DONE_WITH_INFO


@echo on

REM Update the client here, remembering the treasure directories.     

if "%1" == "" goto ESPECIALLY_FOR_STEVE

if not "%1" == "head_rev" goto PULL_FROM_LABEL
p need | p get -l -
call gd treasure
p need | p get -l -
goto SOURCE_UP_TO_DATE

:PULL_FROM_LABEL
p need -w%1 | p get -c -l -
call gd treasure
p need -w%1 | p get -c -l -

:SOURCE_UP_TO_DATE

@echo off
@echo.
@echo Files Updated From Piccolo At Time:
@time /t&date /t
@echo.
@echo on

REM LIB_REBUILD says whether or not to rebuild all libraries before
REM linking an executable.  This is a waste of time for a build which
REM is setting out to rebuild everything.  Not to mention that rebuilding
REM all libraries is often impossible due to cyclic dependencies that exist
REM when building from scratch.

set LIB_REBUILD=OFF

REM The following turns out to be a bad idea when the dll does not match
REM the version of the compiler that Ingres is being compiled with.
REM copy %SYSTEMROOT%\system32\MSVCRT20.DLL "%II_SYSTEM%\ingres\bin"

REM errorlog.log must exist for the install process to associate notepad
REM with it and create an icon for it.

REM --- "Touching" an errorlog.log ...
touch "%II_SYSTEM%\ingres\files\errlog.log"

REM (Not sure how or why to build the following)
touch "%II_SYSTEM%\ingres\files\utld.def"
touch "%II_SYSTEM%\ingres\files\utldcob.def"
touch "%II_SYSTEM%\ingres\files\alarwkp.def"

REM --- Call ntboot to get things underway.
call %ING_SRC%\tools\port\shell_win\ntboot

REM --- Copy makefiles, srclists, dirlists etc. into place.
call gd tools\port\make
call nmake
call gd tools\port\srclist
call nmake
call gd tools\port\dirlist
call nmake

call gd defs
nmake -a 
REM --- Copying all help files ...
call copyhlp

REM --- Creating abfdemo ...
call gd abfdemo
nmake -a

REM --- Copying batch files for install/setup purposes ...
call gd st\shell_win
nmake -a

REM --- Creating Ingres web demo ...
call gd webdemo
nmake -a

REM --- Copying NT specific files for Ingres ...
call gd files_nt
nmake -a

REM --- Creating esql demos ...
call gd esqldemo
nmake -a
call gd esqlcobdemo
nmake -a

REM --- Creating crs configuration files ...
call gd crsfiles
nmake -a

REM --- Building messages.txt ...
call gd erxtract
call buildh.bat

REM --- Need iiapidep.h
call gd common\aif\hdr
nmake -a

REM --- Making gv.res ...
call gd cl\clf\gv
nmake -a gv.res

REM --- Starting compile with gl...
REM (First aim is to build ercomp which needs oiglfnt.lib)
REM (This will die in gl\bin)
call gd gl
nmake -a

REM --- Compiling cl ...
REM (This goes through in one shot!  bin and all.)
call gd cl
nmake -a

REM --- Creating "special" files ...
call gd cl\clf\specials
nmake -a

REM --- Finishing compile of gl ...
call gd gl\bin
nmake

REM --- Recompile cl, since it uses stuff from gl
call gd cl\bin
nmake

REM --- Setup the environment as the utilities require variables to find
REM --- messages and an authorisation string.
if exist "%II_SYSTEM%\ingres\files\symbol.tbl" goto skipsym
call gd bat
call setenv.bat
:skipsym

REM --- Building eqc and esqlc ...
REM --- If a previous installation exists, delete eqc.exe and esqlc.exe from it
if NOT "%NEW_TOOLS%" == "" goto skip_tools
if exist "%II_SYSTEM%\ingres\bin\eqc.exe" del "%II_SYSTEM%\ingres\bin\eqc.exe"
if exist "%II_SYSTEM%\ingres\bin\esqlc.exe" del "%II_SYSTEM%\ingres\bin\esqlc.exe"
:skip_tools

call gd common\hdr\hdr
nmake -a
call gd front\hdr\hdr
REM (This fails on eqc, but by then we've got what we need!)
nmake
call gd afe
nmake
call gd fmt
nmake
call gd ug
nmake
call gd embed\hdr
nmake
call gd equel
nmake
call gd eqmerge
nmake

REM --- yapp required for character sets
call gd yapp
nmake
call gd front\bin
nmake -a yapp.exe

call gd front\bin
nmake -a eqmerge.exe
call gd yacc
nmake
call gd front\bin
nmake -a iyacc.exe

REM call gd testtool\tools
REM nmake -a flex

call gd yycase
nmake
call gd front\bin
nmake -a yycase.exe
call gd embed\c
nmake -a
call gd embed\libqgca
nmake -a
call gd embed\sqlca
nmake -a
call gd embed\copy
nmake
call gd libq
nmake -a
call gd csq
nmake
call gd front\bin
nmake -a %ING_SRC%\front\lib\embedc.lib
nmake -a eqc.exe
nmake -a esqlc.exe

REM --- Need to build the message files before trying to compile with eqc or
REM --- esqlc.

call gd common\hdr\hdr
nmake -a msg_files

REM --- Creating character set translation files ...
call gd gcf\files
nmake -a
call gd front\misc\hdr
nmake ermc.h
call gd chinst
nmake
call gd front\bin
nmake -a chinst.exe

REM --- Building byacc ...
call gd psf\yacc
nmake
call gd back\bin
nmake -a byacc.exe

REM --- Building aducompile ...
call gd common\hdr\hdr
nmake -a
call gd adt
nmake
call gd adu
nmake
call gd adx
nmake
call gd cuf
nmake -a
call gd ddf
nmake -a
call gd front\frontcl\specials
nmake -a
call gd front\frontcl\files
nmake -a
call gd common\gcf
nmake -a
REM (Some of the above not necessary because EXES dependency to be removed)
REM (from common\bin\makefile)
REM
call gd common\adf\adb
nmake
call gd common\adf\adc
nmake
call gd common\adf\ade
nmake
call gd common\adf\adf
nmake
call gd common\adf\adg
nmake
call gd common\adf\adi
nmake
call gd common\adf\adt
nmake
call gd common\adf\adu
nmake
call gd common\adf\adx
nmake
call gd common\aif\hdr
nmake -a
call gd ult
nmake -a
REM (Finally got what aducompile needs...)
call gd common\bin
nmake -a %ING_SRC%\common\lib\%LIBPREFIX%adfnt%LIBSUFFIX%.lib
nmake -a %LIBPREFIX%adfdata%LIBSUFFIX%.dll
nmake -a %LIBPREFIX%adfnt%LIBSUFFIX%.dll
nmake -a aducompile.exe

REM --- Building what we need for unialscompile.exe and unimapcompile.exe
call gd adl
nmake -a

call gd common\bin
nmake -a unialscompile.exe unimapcompile.exe

REM --- Building all of common ...
call gd common
nmake -a 

call gd common\bin
nmake -a %ING_SRC%\common\lib\%LIBPREFIX%gcfnt%LIBSUFFIX%.lib
nmake -a %LIBPREFIX%api%LIBSUFFIX%.dll

REM --- Re-Building all of common ...
call gd common
nmake -a 

REM --- Building 1st pass front ...
call gd frontcl\files
nmake -a
call gd frontcl\specials
nmake -a
call gd front\hdr\hdr
nmake -a
call gd front\utils
nmake -a
call gd front\embed
nmake -a
call gd frontcl
nmake -a
call gd front\frame
nmake -a
call gd abf\hdr
nmake -a
call gd iaom
nmake -a

REM --- Build abfrt g4xfer.obj is required by oiembddata.lib
call gd abf\abfrt
nmake -a

REM --- Now build the front DLLs
call gd front\bin

REM --- Build the static libraries and import libraries required for frs
nmake -a %ING_SRC%\front\lib\%LIBPREFIX%frsnt%LIBSUFFIX%.lib
nmake -a %ING_SRC%\front\lib\%LIBPREFIX%embdnt%LIBSUFFIX%.lib

nmake -a %LIBPREFIX%frsdata%LIBSUFFIX%.dll
nmake -a %LIBPREFIX%frsnt%LIBSUFFIX%.dll
nmake -a %ING_SRC%\front\lib\%LIBPREFIX%embddata%LIBSUFFIX%.lib
nmake -a %LIBPREFIX%embddata%LIBSUFFIX%.dll
nmake -a %LIBPREFIX%embdnt%LIBSUFFIX%.dll

REM --- Link compform
call gd front\bin
nmake -a compform.exe

REM --- Building back ...
call gd back
nmake -a

REM --- Build vdba directories required for front\bin completion
call gd front
cd vdba\rmcmd
nmake

REM --- Now go back and finish remaining front (incl. bin)
REM --- Ipm needs oidmfnt.lib so back must be built before.
call gd front
nmake -a

REM --- Build winstart now that we have ingres.lib
call gd winstart
nmake -a

REM --- Build ice!plays!src now that we have ingres.lib
call gd src
nmake -a

REM --- Build icetranslate now that we have oiiceapi.lib
call gd icetranslate
nmake -a

REM --- Build front!st!iilink now that we have ingres.lib
call gd iilink
nmake -a

REM --- Build front!st!iipostinst now that we have ingres.lib
call gd iipostinst
nmake -a

REM --- Build front!st!perfwiz now that we have ingres.lib
call gd perfwiz
nmake -a

REM --- Build front!st!wincluster now that we have ingres.lib
call gd wincluster
nmake -a

REM --- Build the GUI tools now that we have ingres.lib
call gd front\vdba
nmake -a

REM --- Update Icetutor cazip file
md %II_SYSTEM%\icezip
set II_HTML_ROOT=%II_SYSTEM%\icezip
call icepublic
cd /d "%II_HTML_ROOT%"
cazipxp -r -w *.* "%II_SYSTEM%\ingres\ice\icetutor\icetutor.caz"

REM --- Update Coldfusion cazip file
cd /d "%II_SYSTEM%\ingres\ice\coldfusion"
cazipxp -r -w *.* "%II_SYSTEM%\ingres\ice\coldfusion\ingrescf.caz"

REM --- Building dbutil ...
call gd dbutil
nmake -a

REM --- Re-Building back ...
call gd back
nmake -a

REM --- Creating mnx message files ...
call gd bat
call mkfnx

REM --- Building sig ...
call gd dp
nmake -a
call gd errhelp
nmake -a
call gd star
nmake -a
call gd syncapiw
nmake -a
call gd rpclean
nmake -a
call gd tz
nmake -a
call gd imp
nmake -a
call gd sig\bin
nmake -a

REM Building tools ...
call gd tools\port\others
nmake
call gd tools\port\resource
nmake
call gd tools\port\sm
nmake
call gd tools\bin
nmake -a

REM Building testtool ...
call gd testtool\make
call mv_make.bat
call gd testtool
nmake -a

@echo off
@echo.
@echo Build Finish Time:
@time /t&date /t
@echo.

echo Build has completed!
cd ..
goto END

:ESPECIALLY_FOR_STEVE
@echo off
echo.
echo Syntax:	bang -p ^| label_name ^| head_rev

:END
