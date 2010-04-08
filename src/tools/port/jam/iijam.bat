@echo off

REM
REM Name: iijam.bat
REM
REM Description:
REM	Wrapper for iijam.sh shell script.
REM
REM History:
REM	22-Apr-2005 (drivi01)
REM	    Created.

if not exist %ING_SRC%\bin\iijam goto JAM1
goto JAM2

:JAM1
jam.exe %*
goto EXIT

:JAM2
sh %ING_SRC%\bin\iijam %*
goto EXIT


:EXIT