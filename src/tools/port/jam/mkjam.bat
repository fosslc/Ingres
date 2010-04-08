@echo off

REM
REM Name: mkjam.bat
REM
REM Description:
REM	Wrapper for mkjam.sh shell script.
REM
REM History:
REM	20-jul-2004 (somsa01)
REM	    Created.
REM     29-Jul-2004 (kodse01)
REM         Passing the arguments to shell script.
REM

sh %ING_SRC%\bin\mkjam %*
