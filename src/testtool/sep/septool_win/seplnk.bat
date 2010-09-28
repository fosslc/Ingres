@echo off
REM seplnk : Link EQUEL/ESQL object file with various INGRES libraries
REM History:
REM	17-oct-1995 (somsa01)
REM		Created from seplnk.sh .
REM	03-Jan-1996 (somsa01)
REM		Added /nologo and -DNT_GENERIC to cl options.
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems, also added iiapi.lib
REM		to the linked libraries list.
REM	13-Nov-1998 (somsa01)
REM		Changed "iiapi.lib" to "oiapi.lib".
REM	30-sep-2001 (somsa01)                                         
REM		Removed restriction of when to add %II_SYSTEM%\ingres\utility   
REM		to the PATH.
REM	19-dec-2001 (somsa01)
REM		Use ingres64.lib on Win64.
REM	02-oct-2003 (somsa01)
REM		Ported to NT_AMD64.
REM	18-aug-2004 (drivi01)
REM		Updated link libraries to new jam libraries.
REM     11-oct-2004 (drivi01)
REM             Removed some unecessary code, which was causing SEGV's when
REM             the PATH was too long.
REM	21-oct-2004 (drivi01)
REM		Backing out change from 11-oct-2004, b/c it isn't needed.
REM	17-May-2005 (lakvi01)
REM		Link with bufferoverflowu.lib on WIN64 (i64_win and a64_win).
REM	28-Oct-2008 (wanfr01)
REM		SIR 121146
REM		Add -a option to sync seplnk options across platforms
REM	17-Dec-2008 (wanfr01)
REM		SIR 121407
REM		Add -d flag to enable compiling in debug
REM		Corrected argument handling
REM	04-May-2009 (drivi01)
REM		Update this script to build manifest files into binaries.
REM		After the manifest files are built into binaries they
REM		will be removed by this script.
REM	28-Jul-2009 (drivi01)
REM		Remove bufferoverflowu.lib dependency as the library was 
REM		removed in Visual Studio 2008 compiler and also for pure 
REM		64-bit use libingres.lib instead of ingres64.lib or 
REM		libapi64.lib.
REM	8-Jul-2010 (drivi01)
REM		Add libcmt.lib to the link line.  It is needed to resolve 
REM		security_cookie symbol that is now built into binaries 
REM		with /GS compiler flag.
REM
set CMD=seplnk
setlocal

if not "%1"=="" goto CONTINU
echo.
PCecho "Usage: seplnk <file1> [<file2> ... <filen>] [flags]"
echo.
echo.
PCecho "        -a               VMS Only:  Include API option files
echo.
PCecho "        -c[onditional]   Conditional Linking. Only link if"
PCecho "                         image file does not exist."
echo.
PCecho "        -d[ebug]         Debug code included."
echo.
goto DONE
 
:CONTINU
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM+
REM+ Check local Ingres environment
REM+
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

if not "%II_SYSTEM%"=="" goto CONTINU2
echo.
PCecho "|------------------------------------------------------------|"
PCecho "|                      E  R  R  O  R                         |"
PCecho "|------------------------------------------------------------|"
PCecho "|     The local Ingres environment must be setup and the     |"
PCecho "|     the installation running before continuing             |"
PCecho "|------------------------------------------------------------|"
echo.
goto DONE

:CONTINU2
call ipset tmpdir ingprenv II_TEMPORARY
echo %PATH%> %tmpdir%\seplnk.tmp
call ipset tmpvar sed -e "s/;/ /g" %tmpdir%\seplnk.tmp
del %tmpdir%\seplnk.tmp
set path=%II_SYSTEM%\ingres\utility;%path%

REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

if "%CC%"=="" set CC=CL
set imageflg=true

REM conditional
set cflag=false
set cc_sw=

set line_args=%1
:LOOP
shift
if "%1"=="" goto ENDLOOP
set line_args=%line_args% %1
goto LOOP

:ENDLOOP
for %%i in (%line_args%) do echo %%i>> %tmpdir%\seplnk.tmp
call ipset count expr 0
:LOOP2
call ipset count expr %count% + 1
call ipset arg PCread %tmpdir%\seplnk.tmp %count%
if "%arg%"=="" goto ENDLOOP2
  if "%arg%"=="-conditional" set cflag=true&goto LOOP2
  if "%arg%"=="-conditiona" set cflag=true&goto LOOP2
  if "%arg%"=="-condition" set cflag=true&goto LOOP2
  if "%arg%"=="-conditio" set cflag=true&goto LOOP2
  if "%arg%"=="-conditi" set cflag=true&goto LOOP2
  if "%arg%"=="-condit" set cflag=true&goto LOOP2
  if "%arg%"=="-condi" set cflag=true&goto LOOP2
  if "%arg%"=="-cond" set cflag=true&goto LOOP2
  if "%arg%"=="-con" set cflag=true&goto LOOP2
  if "%arg%"=="-co" set cflag=true&goto LOOP2
  if "%arg%"=="-c" set cflag=true&goto LOOP2

  if "%arg%"=="-a" goto LOOP2

  if "%arg%"=="-debug" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-debu" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-deb" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-de" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-d" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2

  echo %arg%| cut -c1 > %tmpdir%\seplnk2.tmp
  call ipset test_flag PCread %tmpdir%\seplnk2.tmp 1
  del %tmpdir%\seplnk2.tmp
  if "%test_flag%"=="-" echo bad flag %arg%
  goto LOOP2

:ENDLOOP2
set do_it=false
call ipset count expr 0
:LOOP3
call ipset count expr %count% + 1
call ipset arg PCread %tmpdir%\seplnk.tmp %count%
if "%arg%"=="" goto ENDLOOP3
  echo %arg%| cut -c1 > %tmpdir%\seplnk2.tmp
  call ipset test_flag PCread %tmpdir%\seplnk2.tmp 1
  del %tmpdir%\seplnk2.tmp
  if "%test_flag%"=="-" goto LOOP3

  echo %arg% | cut -f 2 -d "." > %tmpdir%\seplnk2.tmp
  call ipset test_flag PCread %tmpdir%\seplnk2.tmp 1
  del %tmpdir%\seplnk2.tmp
  if not "%test_flag%"=="obj" goto NEXTCAS
    set cfilnam=%cfilnam% %arg%
    call ipset tfilnam basename %arg% .obj
    goto ENDCASE

:NEXTCAS
  echo %arg% | cut -f 2 -d "." > %tmpdir%\seplnk2.tmp
  call ipset test_flag PCread %tmpdir%\seplnk2.tmp 1
  del %tmpdir%\seplnk2.tmp
  if not "%test_flag%"=="lib" goto NEXTCAS2
    set cfilnam=%cfilnam% %arg%
    call ipset tfilnam basename %arg% .lib
    goto ENDCASE

:NEXTCAS2
  if not exist %arg%.lib goto NEXTIF
    set ofilnam=%arg%.lib
    set cfilnam=%cfilnam% %ofilnam%
    call ipset tfilnam basename %ofilnam% .lib
    goto ENDCASE
:NEXTIF
  if not exist %arg%.obj goto NEXTIF2
    set ofilnam=%arg%.obj
    set cfilnam=%cfilnam% %ofilnam%
    call ipset tfilnam basename %ofilnam% .obj
    goto ENDCASE
:NEXTIF2
  set cfilnam=%cfilnam% %arg%
  call ipset tfilnam basename %arg%

:ENDCASE
  if "%imageflg%"=="false" goto LOOP3
    set filnam=%tfilnam%
    set imageflg=false
    set do_it=true
    goto LOOP3

:ENDLOOP3
if "%do_it%"=="true" if "%cflag%"=="true" if exist %filnam%.exe set do_it=false

if not "%do_it%"=="true" goto DONE
set libingfiles=%II_SYSTEM%\ingres\lib\libingres.lib %II_SYSTEM%\ingres\lib\iilibapi.lib

REM :RUNCC
  %CC% /nologo -DNT_GENERIC %cc_sw% -Fe%filnam%.exe %cfilnam% %libingfiles% msvcrt.lib kernel32.lib user32.lib advapi32.lib libcmt.lib

  if not exist %filnam%.exe.manifest goto DONE
  if not "%WindowsSdkDir%"=="" if x%MT%==x set MT="%WindowsSdkDir:\\=\%bin\mt.exe"
  if not x%MT%==x if exist %filnam%.exe %MT% -nologo -manifest %filnam%.exe.manifest -outputresource:%filnam%.exe;#1
  if not ERRORLEVEL 1 if not x%MT%==x del %filnam%.exe.manifest

:DONE
endlocal
