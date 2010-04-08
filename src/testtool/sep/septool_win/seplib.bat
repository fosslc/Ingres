@echo off
REM
REM seplib -- Create a library for SEP to use.
REM
REM History:
REM	16-oct-1995 (somsa01)
REM		Created from seplib.sh .
REM	29-Jan-1996 (somsa01)
REM		Added /nologo to lib options.
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
set CMD=seplnk

setlocal
if not "%1"=="" goto NEXT
echo.     
PCecho "Usage: seplib <libname> [<ofile1> ... <ofilen>]"
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
echo %1|cut -f 2 -d "." > %tmpdir%\seplib.tmp
call ipset check_flag PCread %tmpdir%\seplib.tmp 1
del %tmpdir%\seplib.tmp
if not "%check_flag%"=="lib" goto NEXTCAS
  set library=%1
  goto ENDCASE

:NEXTCAS
call ipset check_flag basename %1
if exist %check_flag% set library=%check_flag%
if not exist %check_flag% call ipset library basename %1.lib

:ENDCASE
shift

:LOOP
if "%1"=="" goto LOOPEND
echo %1>> %tmpdir%\seplib.tmp
shift
goto LOOP

:LOOPEND
call ipset count expr 0

:FLOOP
call ipset count expr %count% + 1
call ipset i PCread %tmpdir%\seplib.tmp %count%
if "%i%"=="" goto FLOOPEND
echo %i%> %tmpdir%\seplib2.tmp
call ipset check_flag cut -f 2 -d "." %tmpdir%\seplib2.tmp
if not "%check_flag%"=="obj" goto NEXTCAS2
  if exist %i% set objlist=%objlist% %i%
  goto FLOOP

:NEXTCAS2
call ipset check_flag basename %i%.obj
if exist %check_flag% set objfile=%check_flag%
if not exist %check_flag% call ipset objfile basename %i%
if exist %objfile% set objlist=%objlist% %objfile%
goto FLOOP

:FLOOPEND
if exist %library% lib /out:%library% /nologo %library% %objlist%
if not exist %library% lib /out:%library% /nologo %objlist%

:DONE
endlocal
