@echo off
REM 
REM Name: setenv.bat
REM
REM Description:
REM    Implement UNIX command, setenv
REM
REM History:
REM    05-Nov-2002 (rigka01)
REM     created for purpose of patch creation procedures

REM Check for correct number of parameters
if "%1"=="" echo setenv: Error_1: you must specify two parameters& goto DONE
if "%2"=="" echo setenv: Error_2: you must specify two parameters& goto DONE 
:CONT
SET %1=%2
echo pb_setenv: Info: %1 has been set to %2
:SETUSER
if "%1"=="PMFKEY" pb_setenv USER %2
:DONE
