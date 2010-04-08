@echo off

REM
REM Name: mkjams.bat
REM
REM Description:
REM	Wrapper for mkjams.sh shell script.
REM
REM History:
REM	19-jul-2004 (somsa01)
REM	    Created.
REM	29-Jul-2004 (kodse01)
REM	    Passing the arguments to shell script.
REM

sh %ING_SRC%\bin\mkjams %*
