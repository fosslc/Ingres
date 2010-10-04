@echo off
REM sepcc -- driver to allow sep to compile .c files.
REM
REM History:
REM
REM	05-oct-1995 (somsa01)
REM		Created from sepcc.sh .
REM	03-Jan-1996 (somsa01)
REM		Added /nologo and -DNT_GENERIC to cl options.
REM	30-sep-2001 (somsa01)
REM		Removed restriction of when to add %II_SYSTEM%\ingres\utility
REM		to the PATH.
REM	26-dec-2001 (somsa01)
REM		In the case of Win64, define NT_IA64.
REM	02-oct-2003 (somsa01)
REM		Ported to NT_AMD64.
REM	11-oct-2004 (somsa01)
REM		Removed some unecessary code, which was causing SEGV's when
REM		the PATH was too long.
REM	03-Mar-2006 (drivi01) 
REM		BUG 115811
REM		If cygwin tools used egrep binary isn't available. Replaced
REM		call to egrep with 'grep -E'.
REM	11-Mar-2007 (drivi01)
REM		grep command is being executed without cflag which causes
REM		hangs and errors, fix the condition for grep command as well
REM		as add correct routines for processing -d flag and add
REM		%II_SYSTEM%\ingres\files to include path.
REM	28-Oct-2008 (wanfr01)
REM		SIR 121146
REM		Include directory should always include 
REM             %II_SYSTEM%/ingres/files for API tests
REM	17-Dec-2008 (wanfr01)
REM	    	SIR 121407
REM		Add Zi to compile flags for debug mode
REM		Corrected argument handling
REM	17-Apr-2009 (hanal04) Bug 121951
REM	    	Correct LOOP4 to prevent a hang with grep waiting for CTRL-D
REM             from the terminal. Initialise cfilnam and filnam at the start 
REM             of each iteration.
REM	13-Sep-2010 (drivi01)
REM		Remove /Wp64 flag for x64 it's deprecated.

set CMD=sepcc

setlocal
if not "%1"=="" goto NEXT
echo.
PCecho "Usage: %CMD% <file1> [<file2> ... <filen>]"
echo.
PCecho "        -c[onditional]   Conditional Compilation. Compile only if"
PCecho "                         object file doesn't exist."
echo.
PCecho "        -d[ebug]         Debug code included."
echo.
PCecho "        -I<directory>    Directory to search for include files."
echo.
goto DONE

:NEXT
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM
REM Check local Ingres environment
REM
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

if not "%II_SYSTEM%"=="" goto CONTINUE
echo.
PCecho "|------------------------------------------------------------|"
PCecho "|                      E  R  R  O  R                         |"
PCecho "|------------------------------------------------------------|"
PCecho "|     The local Ingres environment must be setup and the     |"
PCecho "|     the installation running before continuing             |"
PCecho "|------------------------------------------------------------|"
echo.
goto DONE

:CONTINUE
call ipset tmpdir ingprenv II_TEMPORARY
set path=%II_SYSTEM%\ingres\utility;%path%

REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

set cflag=false
set nodbg_flag=true
set include_dirs=/I%II_SYSTEM%\\ingres\\files
if not "%ING_SRC%"=="" set include_dirs=/I%ING_SRC%\\gl\\hdr\\hdr /I%ING_SRC%\\cl\\hdr\\hdr /I%ING_SRC%\\cl\\clf\\hdr /I%ING_SRC%\\front\\hdr\\hdr /I%ING_SRC%\\front\\frontcl\\hdr /I%ING_SRC%\\common\\hdr\\hdr /I%II_SYSTEM%\\ingres\\files

if not "%CCFLAGS%"=="" set cc_sw=%CCFLAGS% 
if "%CPU%"=="IA64" set cc_sw=%cc_sw% -DNT_IA64 
if "%CPU%"=="AMD64" set cc_sw=%cc_sw% -DNT_AMD64 
if "%CC%"=="" set CC=CL

set line_args=%1
:LOOP
shift
if "%1"=="" goto ENDLOOP
set line_args=%line_args% %1
goto LOOP

:ENDLOOP
for %%i in (%line_args%) do echo %%i>> %tmpdir%\sepcc.tmp
call ipset count expr 0
:LOOP2
call ipset count expr %count% + 1
call ipset arg PCread %tmpdir%\sepcc.tmp %count%

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

  if "%arg%"=="-debug" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-debu" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-deb" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-de" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2
  if "%arg%"=="-d" set cc_sw=%cc_sw% /Yd /Zi&goto LOOP2

  echo %arg%| cut -c1,2 > %tmpdir%\sepcc2.tmp
  call ipset test_flag PCread %tmpdir%\sepcc2.tmp 1
  del %tmpdir%\sepcc2.tmp
  if "%test_flag%"=="-I" set include_dirs=%include_dirs% %arg%
  if "%test_flag%"=="-I" goto LOOP2

  echo %arg%| cut -c1 > %tmpdir%\sepcc2.tmp
  call ipset test_flag PCread %tmpdir%\sepcc2.tmp 1
  del %tmpdir%\sepcc2.tmp
  if "%test_flag%"=="-" echo bad flag %arg%
  goto LOOP2

:ENDLOOP2
del %tmpdir%\sepcc.tmp
set new_cc_sw=
set cc_sw_del=

touch %tmpdir%\sepcc.tmp
for %%i in (%cc_sw%) do echo %%i>> %tmpdir%\sepcc.tmp
call ipset count expr 0
:LOOP3
call ipset count expr %count% + 1
call ipset i PCread %tmpdir%\sepcc.tmp %count%
if "%i%"=="" goto ENDLOOP3
  set add_to_sw=true

  for %%j in (%new_cc_sw%) do if "%i%"=="%%j" set add_to_sw=false

  if "%add_to_sw%"=="true" set new_cc_sw=%new_cc_sw%%cc_sw_del%%i%
  set cc_sw_del= 
  goto LOOP3
:ENDLOOP3
del %tmpdir%\sepcc.tmp
set cc_sw=%new_cc_sw%

set copy_it=false
set cp_str=
for %%i in (%line_args%) do echo %%i>> %tmpdir%\sepcc.tmp
call ipset count expr 0
:LOOP4
call ipset count expr %count% + 1
call ipset arg PCread %tmpdir%\sepcc.tmp %count%

if "%arg%"=="" goto ENDLOOP4
  set do_it=false

  set cfilnam=
  set filnam=
  echo %arg%| cut -c1 > %tmpdir%\sepcc2.tmp
  call ipset test_flag PCread %tmpdir%\sepcc2.tmp 1
  del %tmpdir%\sepcc2.tmp
  if "%test_flag%"=="-" goto ENDCASE

  echo %arg%| cut -f 2 -d "." > %tmpdir%\sepcc2.tmp
  call ipset test_flag PCread %tmpdir%\sepcc2.tmp 1
  del %tmpdir%\sepcc2.tmp
  if not "%test_flag%"=="c" goto NEXTCASE
    set cfilnam=%arg%
    call ipset filnam basename %arg% .c
    set do_it=true
    goto ENDCASE

:NEXTCASE
  if not exist %arg%.c if exist %arg% goto COPYIT
  goto FINISH
:COPYIT
    set copy_it=true
    copy %arg% %arg%.c > %tmpdir%\sepcc3.tmp
    del %tmpdir%\sepcc3.tmp
    set cp_str=%cp_str% %arg%.c
:FINISH
  set cfilnam=%arg%.c
  set filnam=%arg%
  set do_it=true
  
:ENDCASE
if "%cfilnam%"=="" goto NEXT2
    grep -E "NO_OPTIM.*=.*all$|NO_OPTIM.*=.*int_wnt|NO_OPTIM.*=.*%CC%" %cfilnam% > %tmpdir%\sepcc3.tmp
    if errorlevel 1 set cc_sw=%cc_sw% /Od
    del %tmpdir%\sepcc3.tmp

:NEXT2
    if "%do_it%"=="true" if "%cflag%"=="true" if exist %filnam%.obj set do_it=false
    if not "%do_it%"=="true" goto LOOP4
      %CC% /c /w /MD /nologo -DNT_GENERIC %cc_sw% %include_dirs% %cfilnam%
    goto LOOP4

:ENDLOOP4
del %tmpdir%\sepcc.tmp
if not "%copy_it%"=="true" goto DONE
  del %cp_str%

:DONE
endlocal
