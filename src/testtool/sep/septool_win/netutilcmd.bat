@echo off
REM netutilcmd.bat
REM
REM This is a wrapper script for netutil. It is invoked by SEP scripts
REM to run netutil with command line arguments.
REM
REM History:
REM
REM	22-sep-1995 (somsa01)
REM		Created from netutilcmd.sh .
REM

setlocal
set ii_netutil=%1
:loop
shift
if "%1"=="" goto end
set ii_netutil=%ii_netutil% %1
goto loop
:end
echo on
@netutil %ii_netutil%
@endlocal
