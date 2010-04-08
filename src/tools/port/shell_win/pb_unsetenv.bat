@echo off
REM 
REM Name: unsetenv.bat
REM
REM Description:
REM    Implement UNIX command, unsetenv
REM
REM History:
REM    05-Nov-2002 (rigka01)
REM     created for purpose of patch creation procedures

REM Check for correct number of parameters
if "%1"=="" echo unsetenv: Error_1: you must specify two parameters& goto DONE
:CONT
SET %1=
echo unsetenv: Info: %1 has been unset
if "%1"=="PMFKEY" unsetenv USER
:DONE
