REM @echo off
REM 
REM  readvers.bat
REM 
REM  History:
REM 	13-jul-95 (tutto01)
REM	    Created.
REM	13-dec-1999 (somsa01)
REM	    Set all variables that are in the VERS file.
REM 	14-Jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.
REM	24-Aug-2009 (bonro01)
REM	    Make the VERS override file optional.
REM	10-Sep-2009 (bonro01)
REM	    Fix previous change to make VERSFILE a global variable.


REM Locate the VERS file
REM
set VERSDIR=%ING_SRC%\tools\port\conf
if exist %VERSDIR%\VERS (
    set VERSFILE=%VERSDIR%\VERS
) else if exist %VERSDIR%\VERS.%config_string% (
    set VERSFILE=%VERSDIR%\VERS.%config_string%
) else (
    echo.
    echo There is no VERS or VERS.%config_string% file!!!!!!
    echo.
    goto EXIT
)


REM Transform the VERS file into an executable batch file.
REM Only deal with lines that start with "#set:", as other
REM lines are for jam (or are comments).
REM Do this for now so that VERS files looks just like other systems.
REM
%AWK_CMD% "$1 == \"#set:\" {print \"set \" $2 \"=\" $3}" %VERSFILE% > %VERSDIR%\VERS.bat
call %VERSDIR%\VERS.bat


:EXIT
