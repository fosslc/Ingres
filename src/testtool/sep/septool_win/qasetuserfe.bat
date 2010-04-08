@echo off
REM qasetuserfe.bat
REM
REM This is a wrapper script for qasetuser. It is invoked by SEP scripts
REM to run qasetuser on an FRS executable.
REM
REM History:
REM
REM	17-oct-1995 (somsa01)
REM		Created from qasetuserfe.sh .
REM

setlocal
:LOOP
if "%1"=="" goto ENDLOOP
set arg_line=%arg_line% %1
shift
goto LOOP

:ENDLOOP
qasetuser %arg_line%
endlocal
