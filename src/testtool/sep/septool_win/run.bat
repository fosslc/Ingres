@echo off
REM run : brainlessly pass things through to the os from sep
REM
REM	05-oct-1995 (somsa01)
REM		Created from run.sh .
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems.  Also changed cmdline
REM 		to cmdlin.  This is a system environment variable in Win95.
REM	26-feb-1999 (somsa01)
REM	    Refined the way the command line is set, as we could have more
REM	    than 10 incoming parameters.
REM	19-may-1999 (somsa01)
REM	    Set "cmdlin" to the arguments of the command.
REM	06-jun-2001 (somsa01)
REM	    Account for embedded spaces in program name path.
REM	29-sep-2001 (somsa01)
REM	    Removed restriction on setting/not setting
REM	    %II_SYSTEM%\ingres\utility in the PATH variable.

set CMD=run

setlocal
if not "%1"=="" goto NEXT
echo Usage: %CMD% {executable_filespec}[args ...]
goto DONE

:NEXT
REM+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM
REM Check local Ingres environment
REM
REM+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

if not "%II_SYSTEM%" =="" goto CONTINU
PCecho "|------------------------------------------------------------|"
PCecho "|                      E  R  R  O  R                         |"
PCecho "|------------------------------------------------------------|"
PCecho "|     The local Ingres environment must be setup and the     |"
PCecho "|     the installation running before continuing             |"
PCecho "|------------------------------------------------------------|"

goto DONE

:CONTINU
call ipset tmpdir ingprenv II_TEMPORARY
if "%DEBUG%"=="" set debugit=false
if not "%DEBUG%"=="" set debugit=true

echo %PATH% > %tmpdir%\tempfile.tmp
call ipset tmppath sed -e "s/;/ /g" %tmpdir%\tempfile.tmp
del %tmpdir%\tempfile.tmp
set path=%II_SYSTEM%\ingres\utility;%path%

set prog_name=%1
shift
set cmdlin=%1
shift
:LOOP
if "%1"=="" goto ENDLOOP
set cmdlin=%cmdlin% %1
shift
goto LOOP

:ENDLOOP
call ipset dir_name dirname %prog_name%

if "%debugit%"=="false" goto CONTINU2
echo.>>%tmpdir%\run.tmp
PCecho "|------------------------------------------------------------|">>%tmpdir%\run.tmp
PCecho "|	args      = %cmdlin%   ">>%tmpdir%\run.tmp
PCecho "|	prog_name = %prog_name%">>%tmpdir%\run.tmp
PCecho "|	dir_name  = %dir_name% ">>%tmpdir%\run.tmp
PCecho "|------------------------------------------------------------|">>%tmpdir%\run.tmp
echo.>>%tmpdir%\run.tmp
type %tmpdir%\run.tmp
if "%LOG_FD%"=="" goto CONTINU2
type %tmpdir%\run.tmp > %LOG_FD%

REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

:CONTINU2
if exist %tmpdir%\run.tmp del %tmpdir%\run.tmp
if exist %prog_name% goto CONTINU3
     call ipset which_prog PCwhich %prog_name%
     if "%which_prog%"=="" goto ELSE
          shift
          set prog_name="%which_prog%"
          set cmdlin=%prog_name% %cmdlin%
          call ipset dir_name dirname %prog_name%
          goto CONTINU3
:ELSE
echo.>>%tmpdir%\run.tmp
PCecho "|------------------------------------------------------------|">>%tmpdir%\run.tmp
PCecho "|                      E  R  R  O  R                         |">>%tmpdir%\run.tmp
PCecho "|------------------------------------------------------------|">>%tmpdir%\run.tmp
PCecho "|                    Can't find %prog_name%">>%tmpdir%\run.tmp
PCecho "|------------------------------------------------------------|">>%tmpdir%\run.tmp
echo.>>%tmpdir%\run.tmp
type %tmpdir%\run.tmp
if "%LOG_FD%"=="" goto DONE
type %tmpdir%\run.tmp > %LOG_FD%
goto DONE

:CONTINU3
if not "%dir_name%"=="." goto CONTINU4
  echo %prog_name% > %tmpdir%\run.tmp
  call ipset tempvar cut -c1 %tmpdir%\run.tmp
  if "%tempvar%"=="." goto CONTINU4
  if "%tempvar%"=="\" goto CONTINU4 
  del %tmpdir%\run.tmp
  set cmdlin=.\%prog_name% %cmdlin%

:CONTINU4
%cmdlin%
:DONE
if exist %tmpdir%\run.tmp del %tmpdir%\run.tmp
endlocal
