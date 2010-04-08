@echo off
REM 
REM History:
REM 
REM 20-oct-2004 (emma.mcgrattan@ca.com)
REM     Created 
REM

if not "%1"=="" goto CONT1
echo.
echo Usage: "SetIngresLanguage [fra|deu|esn|ita|ptb|sch or jpn]"
echo.
goto EXIT

:CONT1
echo.
echo Setting the Ingres Language Environment to %1
echo.
if not "%II_SYSTEM%" == "" goto IISYS
echo.
echo.
echo Error: The II_SYSTEM environment variable has not been set!
echo.
echo.
goto EXIT
:IISYS

if exist "%II_SYSTEM%\ingres\files\%1" goto SKIPDIRCREATE
mkdir "%II_SYSTEM%\ingres\files\%1"
:SKIPDIRCREATE
copy %1\*.* "%II_SYSTEM%\ingres\files\%1"
ingsetenv II_LANGUAGE %1

:EXIT
