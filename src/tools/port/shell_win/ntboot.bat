REM
REM ntboot.bat - bootstrapping program which readies the nt build.
REM
REM History:
REM
REM 06-jun-95 (tutto01)
REM 	Created.
REM 11-dec-95 (emmag)
REM     Added ingchkdate.bat ingsetauth.bat
REM 17-feb-96 (tutto01)
REM	Removed some obsolete lines.
REM 04-apr-96 (tutto01)
REM	Cleaned up.  Make sure II_SYSTEM is set.  Added creation of
REM	installation directory structure.  Copy some new commands.
REM 10-jun-96 (emmag)
REM	Remove some batch files which have been replaced by executables.
REM 11-jun-96 (somsa01)
REM     Added copying of fasti.bat to %ING_SRC%\bin directory.
REM 26-jul-96 (somsa01)
REM	Added copying of fixcfg.bat to %ING_SRC%\bin directory.
REM 29-sep-97 (mcgem01)
REM	sql.bat, quel.bat and abf.bat have been replaced by exe's
REM 28-jan-1998 (canor01)
REM	Quote II_SYSTEM for use with embedded spaces in directory names.
REM 08-may-1998 (canor01)
REM	Copy CA License Check API files from treasure directories into
REM	build tree.
REM 17-may-1998 (canor01)
REM     lic98.lib has been renamed to lic98mtdll.lib.
REM 22-jul-1998 (fanra01)
REM	Corrected syntax error
REM 14-sep-98 (mcgem01)
REM	Catalogdb and accessdb are now built as executables.
REM 18-sep-98 (mcgem01)
REM	Stop the Log watch service, to be sure that the DLL gets updated.
REM 24-sep-1998 (mcgem01)
REM     Added copying of users.sql to %ING_SRC%\bin directory for setting
REM	up the runall environment.
REM 01-oct-1998 (canor01)
REM     Fix typo in copying of describe.bat.
REM 15-feb-1999 (mcgem01)
REM	Use the CPU environment variable to determine which version of the
REM	license DLL gets copied into the cl\lib directory.
REM 13-dec-1999 (somsa01)
REM	Added the dynamic making of gv.h .
REM 14-jan-2000 (somsa01)
REM	When calling mkgv, pass the product as an argument so that we
REM	properly generate the correct gv.h file.
REM 09-feb-2001 (somsa01)
REM	Added changes for port to i64_win (NT_IA64).
REM 19-apr-2001 (somsa01)
REM	Added creation of version.rel .
REM 04-may-2001 (somsa01)
REM	Copy cazipxp.exe from int_w32!utils to II_SYSTEM\ingres\bin.
REM 04-may-2001 (somsa01)
REM	The redirection of 'sed' does not work unless used through the
REM	Korn shell command interpreter.
REM 07-may-2001 (somsa01)
REM	Moved "setlocal" and "endlocal" such that it is only used when
REM	making the version.rel file.
REM 05-jun-2001 (mcgem01)
REM	Copy the contents of the %II_SYSTEM%\ingres\sig\auditmgr directory
REM	into place.
REM 11-jun-2001 (mcgem01)
REM	Assemble the release notes in HTML format.
REM 16-jun-2001 (somsa01)
REM	Added copying of buildrel.bat .
REM 30-jul-2001 (somsa01)
REM	Added copying of SAX and Xerces stuff from the treasure area.
REM 20-oct-2001 (somsa01)
REM	Fixed typo from previous submission.
REM 10-nov-2001 (somsa01)
REM	Added copyover of 64-bit libraries.
REM 26-nov-2001 (somsa01)
REM	Changed "ingresrn.htm" to "Readme_Ingres.htm".
REM 30-nov-2001 (somsa01)
REM	Modified the relnote creation to just copy over the platform-specific
REM	one as the proper name.
REM 28-jan-2002 (somsa01)
REM	Changed "Readme_Ingres.htm" to "readme.html".
REM 11-feb-2002 (somsa01)
REM	Also copy over lic98.lib for GUI tool builds.
REM 22-feb-2002 (somsa01)
REM	Added creation of %ING_SRC%\front\vdba\App and %ING_SRC%\front\vdba\Lib.
REM 27-jan-2003 (somsa01)
REM	Removed ingsetauth.bat.
REM 25-mar-2003 (somsa01)
REM	Use "-use_inc" flag to genrelid when making version.rel.
REM 02-oct-2003 (somsa01)
REM	Ported to NT_AMD64.
REM 29-mar-2004 (gupsh01)
REM	Added Xerces 2.3. 
REM 11-Jun-2004 (somsa01)
REM	Cleaned up code for Open Source.
REM 15-jun-2004 (drivi01)
REM	Modified sh command to be a variable.
REM 24-Jun-2004 (drivi01)
REM	Removed licensing.
REM 01-Jul-2004 (somsa01)
REM	Updated for Xerces 2.5.
REM 31-aug-2004 (somsa01)
REM	More updates for Xerces 2.5.
REM 27-Oct-2004 (drivi01)
REM     bat directories on windows were replaced with shell_win,
REM     this script was updated to reflect that
REM 29-aug-2005 (rigka01)
REM     Add creation of PTOOLSROOT\bin directory

@if not "%II_SYSTEM%" == "" goto IISYS
@echo.
@echo.
@echo II_SYSTEM not set!!!!!
@echo.
@echo.
@goto EXIT
:IISYS

@if not "%ING_SRC%" == "" goto INGSRC
@echo.
@echo.
@echo ING_SRC not set!!!!!
@echo.
@echo.
@goto EXIT
:INGSRC

REM Locate the VERS file
REM
if exist %ING_SRC%\tools\port\conf\VERS goto VERSFILE
if exist %ING_SRC%\VERS copy %ING_SRC%\VERS %ING_SRC%\tools\port\conf\VERS
if exist %ING_SRC%\tools\port\conf\VERS goto VERSFILE
echo.
echo There is no VERS file!!!!!!
echo.
goto EXIT
:VERSFILE

cd %ING_SRC%\tools\port\shell_win

@echo.
@echo Creating bin and lib directories...

mkdir %ING_SRC%\bin

mkdir %ING_SRC%\admin\bin
mkdir %ING_SRC%\admin\lib

mkdir %ING_SRC%\back\bin
mkdir %ING_SRC%\back\lib

mkdir %ING_SRC%\cl\bin
mkdir %ING_SRC%\cl\lib

mkdir %ING_SRC%\common\bin
mkdir %ING_SRC%\common\lib

mkdir %ING_SRC%\dbutil\bin
mkdir %ING_SRC%\dbutil\lib

mkdir %ING_SRC%\front\bin
mkdir %ING_SRC%\front\lib

mkdir %ING_SRC%\gl\bin
mkdir %ING_SRC%\gl\lib

mkdir %ING_SRC%\sig\bin
mkdir %ING_SRC%\sig\lib

mkdir %ING_SRC%\testtool\bin
mkdir %ING_SRC%\testtool\lib

mkdir %ING_SRC%\tools\bin
mkdir %ING_SRC%\tools\lib

@echo.
@echo Now creating Ingres directory structure at %II_SYSTEM% ...
call mkidir

@echo.
@echo Now copying clsecret.h and bzarch.h ...

copy clsecret.h %ING_SRC%\cl\clf\hdr
call mkbzarch.bat

@echo.
@echo Now creating gv.h ...
@echo on
@setlocal

call mkgv.bat
call genrelid.bat INGRES -use_inc > %TEMP%\verstmp.rel
cd /d %TEMP%
%SHELL% -c "sed 's/\"//g' verstmp.rel > version.rel"
mv -f version.rel %II_SYSTEM%\ingres\version.rel

@endlocal
echo on
@echo.
@echo Copying iyacc.par and byacc.par

mkdir %ING_SRC%\lib
copy  %ING_SRC%\front\tools\yacc\iyacc.par %ING_SRC%\lib
copy  %ING_SRC%\back\psf\yacc\byacc.par    %ING_SRC%\lib

@echo.
@echo Copying Piccolo aliases ...

copy describe.bat     %ING_SRC%\bin\describe.bat
copy filelog.bat      %ING_SRC%\bin\filelog.bat
copy get.bat          %ING_SRC%\bin\get.bat
copy have.bat         %ING_SRC%\bin\have.bat
copy working.bat      %ING_SRC%\bin\working.bat
copy here.bat         %ING_SRC%\bin\here.bat
copy integrate.bat    %ING_SRC%\bin\integrate.bat
copy need.bat         %ING_SRC%\bin\need.bat
copy open.bat         %ING_SRC%\bin\open.bat
copy opened.bat       %ING_SRC%\bin\opened.bat
copy rcompare.bat     %ING_SRC%\bin\rcompare.bat
copy rcompare.bat     %ING_SRC%\bin\rcomapre.bat
copy release.bat      %ING_SRC%\bin\release.bat
copy reserve.bat      %ING_SRC%\bin\reserve.bat
copy reserved.bat     %ING_SRC%\bin\reserved.bat
copy sc.bat           %ING_SRC%\bin\sc.bat
copy subscribe.bat    %ING_SRC%\bin\subscribe.bat
copy subscribed.bat   %ING_SRC%\bin\subscribed.bat
copy there.bat        %ING_SRC%\bin\there.bat
copy unsubscribe.bat  %ING_SRC%\bin\unsubscribe.bat

echo.
echo Copying utilities...

copy bang.bat         %ING_SRC%\bin\bang.bat
copy buildrel.bat     %ING_SRC%\bin\buildrel.bat
copy purge.bat        %ING_SRC%\bin\purge.bat
copy copymake.bat     %ING_SRC%\bin\copymake.bat
copy copysrc.bat      %ING_SRC%\bin\copysrc.bat
copy copydirlist.bat  %ING_SRC%\bin\copydirlist.bat
copy genrelid.bat     %ING_SRC%\bin\genrelid.bat
copy getmake.bat      %ING_SRC%\bin\getmake.bat
copy getsrc.bat       %ING_SRC%\bin\getsrc.bat 
copy getdirlist.bat   %ING_SRC%\bin\getdirlist.bat
copy mkgv.bat         %ING_SRC%\bin\mkgv.bat
copy putmake.bat      %ING_SRC%\bin\putmake.bat 
copy putsrc.bat       %ING_SRC%\bin\putsrc.bat 
copy putdirlist.bat   %ING_SRC%\bin\putdirlist.bat
copy diffmake.bat     %ING_SRC%\bin\diffmake.bat
copy diffsrc.bat      %ING_SRC%\bin\diffsrc.bat
copy diffdirlist.bat  %ING_SRC%\bin\diffdirlist.bat
copy ccpp.bat	      %ING_SRC%\bin\ccpp.bat
copy readvers.bat     %ING_SRC%\bin\readvers.bat
copy grepall.bat      %ING_SRC%\bin\grepall.bat
copy grepall2.bat     %ING_SRC%\bin\grepall2.bat
copy ts.bat           %ING_SRC%\bin\ts.bat
copy where.bat        %ING_SRC%\bin\where.bat
copy mkwherelist.bat  %ING_SRC%\bin\mkwherelist.bat
copy blastdb.bat      %ING_SRC%\bin\blastdb.bat
copy copyhlp.bat      %ING_SRC%\bin\copyhlp.bat
copy gd.bat           %ING_SRC%\bin\gd.bat
copy namke.bat        %ING_SRC%\bin\namke.bat
copy mkidir.bat       %ING_SRC%\bin\mkidir.bat
copy fasti.bat        %ING_SRC%\bin\fasti.bat
copy fixcfg.bat       %ING_SRC%\bin\fixcfg.bat
copy iirebase.bat     %ING_SRC%\bin\iirebase.bat
copy users.sql        %ING_SRC%\bin\users.sql

copy dirlist.lst %ING_SRC%\bin\dirlist.lst

echo.
echo Copying installation batch files...

copy ingres.bat       "%II_SYSTEM%\ingres\bin\ingres.bat"
copy ingchkdate.bat   "%II_SYSTEM%\ingres\utility"
copy makestdcat.bat   "%II_SYSTEM%\ingres\sig\star"
copy makestdcat.inp   "%II_SYSTEM%\ingres\sig\star"
copy iirunschd.bat    "%II_SYSTEM%\ingres\sig\star"
copy iirunschd.inp    "%II_SYSTEM%\ingres\sig\star"
copy abfimage.bat     "%II_SYSTEM%\ingres\bin\abfimage.bat"
copy mkingusr.bat     "%II_SYSTEM%\ingres\bin\mkingusr.bat"
copy query.bat        "%II_SYSTEM%\ingres\bin\query.bat"
copy ckcopyd.bat      "%II_SYSTEM%\ingres\bin"
copy ckcopyt.bat      "%II_SYSTEM%\ingres\bin"
copy deltutor.bat     "%II_SYSTEM%\ingres\utility"
copy deltutor.bat     "%II_SYSTEM%\ingres\utility"
copy visiontutor.bat  "%II_SYSTEM%\ingres\utility"
copy abfdemo.bat      "%II_SYSTEM%\ingres\bin"
copy deldemo.bat      "%II_SYSTEM%\ingres\bin"
copy imageapp.bat     "%II_SYSTEM%\ingres\bin"

copy %ING_SRC%\front\frontcl\files_unix\vt100f.map "%II_SYSTEM%\ingres\files\vt100f.map"
copy %ING_SRC%\front\frontcl\files_unix\vt100i.map "%II_SYSTEM%\ingres\files\vt100i.map"
copy %ING_SRC%\front\frontcl\files_unix\vt100nk.map "%II_SYSTEM%\ingres\files\vt100nk.map"
copy %ING_SRC%\front\frontcl\files_unix\vt200i.map "%II_SYSTEM%\ingres\files\vt200i.map"
copy %ING_SRC%\front\frontcl\files_unix\vt220.map "%II_SYSTEM%\ingres\files\vt220.map"

copy %ING_SRC%\front\abf\visitut\book_lis.doc "%II_SYSTEM%\ingres\files\tutorial\book_lis.doc"
copy %ING_SRC%\front\abf\visitut\book_ord.doc "%II_SYSTEM%\ingres\files\tutorial\book_ord.doc"
copy %ING_SRC%\front\abf\visitut\copy.in "%II_SYSTEM%\ingres\files\tutorial\copy.in"
copy %ING_SRC%\front\abf\visitut\cust_inf.doc "%II_SYSTEM%\ingres\files\tutorial\cust_inf.doc"
copy %ING_SRC%\front\abf\visitut\cust_ord.doc "%II_SYSTEM%\ingres\files\tutorial\cust_ord.doc"
copy %ING_SRC%\common\adf\adm\multi.dsc "%II_SYSTEM%\ingres\files\collation\multi.dsc"
copy %ING_SRC%\common\adf\adm\spanish.dsc "%II_SYSTEM%\ingres\files\collation\spanish.dsc"

copy %ING_SRC%\sig\ima\ima\iomon.form "%II_SYSTEM%\ingres\sig\ima\"
copy %ING_SRC%\sig\ima\ima\iomon.sc   "%II_SYSTEM%\ingres\sig\ima\"
copy %ING_SRC%\sig\ima\ima\iomon.sql  "%II_SYSTEM%\ingres\sig\ima\"
copy %ING_SRC%\sig\ima\ima\Makefile   "%II_SYSTEM%\ingres\sig\ima\makefile"
copy %ING_SRC%\sig\ima\ima\README.DOC "%II_SYSTEM%\ingres\sig\ima\readme.doc"

copy %ING_SRC%\sig\secure\auditmgr\README "%II_SYSTEM%\ingres\sig\auditmgr\readme.txt"
copy %ING_SRC%\sig\secure\auditmgr\*.osq "%II_SYSTEM%\ingres\sig\auditmgr"
copy %ING_SRC%\sig\secure\auditmgr\*.tmp "%II_SYSTEM%\ingres\sig\auditmgr"

copy %ING_SRC%\front\tools\erxtract\messages.doc "%II_SYSTEM%\ingres\files\english\messages\messages.doc"

copy %ING_SRC%\tools\techpub\genrn\readme.html "%II_SYSTEM%\readme.html"

REM
REM Copy cazipxp.exe from int_w32!utils over to %II_SYSTEM%\ingres\bin
REM
@echo Copying cazipxp.exe to %II_SYSTEM%\ingres\bin
copy %ING_SRC%\treasure\int_w32\utils\cazipxp.exe %II_SYSTEM%\ingres\bin

REM
REM Copy the Xerces and SAX stuff from the treasure area
REM
@echo Copying Xerces and SAX stuff...
if "%CPU%" == "IA64" goto 64bitXerces
if "%CPU%" == "AMD64" goto AMD64bitXerces
copy %ING_SRC%\treasure\Xerces\2.5\int_w32\bin\xerces*.dll %II_SYSTEM%\ingres\bin
copy %ING_SRC%\treasure\Xerces\2.5\int_w32\lib\xerces*.lib %ING_SRC%\front\lib
goto XercesDone

:64bitXerces
copy %ING_SRC%\treasure\Xerces\2.5\i64_win\bin\xerces*.dll %II_SYSTEM%\ingres\bin
copy %ING_SRC%\treasure\Xerces\2.5\i64_win\lib\xerces*.lib %ING_SRC%\front\lib
goto XercesDone

:AMD64bitXerces
copy %ING_SRC%\treasure\Xerces\1.4\a64_win\lib\icesax* %II_SYSTEM%\ingres\bin
copy %ING_SRC%\treasure\Xerces\1.4\a64_win\lib\xerces* %ING_SRC%\front\lib

:XercesDone

if not "%BUILD_VDBA%" == "ON" goto EXIT
if not exist %ING_SRC%\front\vdba\App mkdir %ING_SRC%\front\vdba\App
if not exist %ING_SRC%\front\vdba\Lib mkdir %ING_SRC%\front\vdba\Lib

:Patchtools

@if not "%PTOOLSROOT%" == "" goto PTOOLS
@echo Warning: PTOOLSROOT is not set
goto EXIT
:PTOOLS 
mkdir %PTOOLSROOT%\bin
cp %ING_SRC%\patchtools\tools\shell\kit\* %PTOOLSROOT%\bin\.

:EXIT
