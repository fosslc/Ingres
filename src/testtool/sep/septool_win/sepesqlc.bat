@echo off
REM
REM sepesqlc -- Preprocesses an Esql/C or Equel/C script.
REM
REM History:
REM	04-oct-1995 (somsa01)
REM		Created from sepesqlc.sh .
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
setlocal
if not "%1"=="" goto CONTINUE
    echo.
    PCecho "Usage: sepesqlc <file1> [<file2> ... <filen>]"
    echo.
    echo.
    PCecho "        -c[onditional]   Conditional Processing. Preprocess only if"
    PCecho "                         source file does not exist."
    echo.
    goto DONE

:CONTINUE
set cflag=false

call ipset tmpdir ingprenv II_TEMPORARY
set line_args=%1
:LOOP
shift
if "%1"=="" goto ENDLOOP
set line_args=%lineargs% %1
goto LOOP

:ENDLOOP
for %%i in (%line_args%) do echo %%i>> %tmpdir%\sepesqlc.tmp
call ipset count expr 0
:LOOP2
call ipset count expr %count% + 1
call ipset arg PCread %tmpdir%\sepesqlc.tmp %count%
if "%arg%"=="" goto ENDLOOP2
  if "%arg%"=="-conditional" set cflag=true
  if "%arg%"=="-conditiona" set cflag=true
  if "%arg%"=="-condition" set cflag=true
  if "%arg%"=="-conditio" set cflag=true
  if "%arg%"=="-conditi" set cflag=true
  if "%arg%"=="-condit" set cflag=true
  if "%arg%"=="-condi" set cflag=true
  if "%arg%"=="-cond" set cflag=true
  if "%arg%"=="-con" set cflag=true
  if "%arg%"=="-co" set cflag=true
  if "%arg%"=="-c" set cflag=true
  if "%cflag%"=="true" goto LOOP2

echo %arg%| cut -c1 > %tmpdir%\sepesqlc2.tmp
call ipset test_flag PCread %tmpdir%\sepesqlc2.tmp 1
del %tmpdir%\sepesqlc2.tmp
if "%test_flag%"=="-" echo bad flag %arg%
goto LOOP2

:ENDLOOP2
call ipset count expr 0
:LOOP3
call ipset count expr %count% + 1
call ipset arg PCread %tmpdir%\sepesqlc.tmp %count%
if "%arg%"=="" goto DONE
  set do_it=false
  set pcccmd=esqlc
  
  echo %arg%| cut -c1 > %tmpdir%\sepesqlc2.tmp
  call ipset test_flag PCread %tmpdir%\sepesqlc2.tmp 1
  del %tmpdir%\sepesqlc2.tmp
  if "%test_flag%"=="-" goto ENDCASE

  echo %arg%| cut -f 2 -d "." > %tmpdir%\sepesqlc2.tmp
  call ipset test_flag PCread %tmpdir%\sepesqlc2.tmp 1
  del %tmpdir%\sepesqlc2.tmp
  if not "%test_flag%"=="sc" goto NEXTCAS
    set esfilnam=%arg%
    call ipset filnam basename %arg% .sc
    set cfilnam=%filnam%.c
    set do_it=true
    set pcccmd=esqlc
    goto ENDCASE

:NEXTCAS
  echo %arg%| cut -f 2 -d "." > %tmpdir%\sepesqlc2.tmp
  call ipset test_flag PCread %tmpdir%\sepesqlc2.tmp 1
  del %tmpdir%\sepesqlc2.tmp
  if not "%test_flag%"=="qc" goto NEXTCAS2
    set esfilnam=%arg%
    call ipset filnam basename %arg% .qc
    set cfilnam=%filnam%.c
    set do_it=true
    set pcccmd=eqc
    goto ENDCASE
  
:NEXTCAS2
  set filnam=%arg%
  call ipset esfilnam basename %arg%.sc
  if not exist %esfilnam% goto ELSE
    set do_it=true
    set cfilnam=%arg%.c
    set pcccmd=esqlc
    goto ENDCASE
:ELSE
  call ipset esfilnam basename %arg%.qc
  if not exist %esfilnam% goto ELSE2
    set do_it=true
    set cfilnam=%arg%.c
    set pcccmd=eqc
    goto ENDCASE
:ELSE2
  call ipset esfilnam basename %arg%
  set do_it=true
  set pcccmd=esqlc

:ENDCASE
  if not "%do_it%"=="true" goto NEXT
  if exist %esfilnam% goto NEXT
    PCecho " file %esfilnam% not found..."
    set do_it=false

:NEXT
  if "%do_it%"=="true" if "%cflag%"=="true" if exist %cfilnam% set do_it=false

  if "%do_it%"=="true" %pcccmd% -f%filnam%.c %esfilnam%

goto LOOP3

:DONE
if exist %tmpdir%\sepesqlc.tmp del %tmpdir%\sepesqlc.tmp
endlocal
