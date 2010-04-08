@echo off

REM
REM deldemo.bat - deletes files associated with demo
REM 
REM History:
REM	24-jul-95 (tutto01)
REM		Ported to NT.
REM     17-Aug-95 (fanra01)
REM             Modified the delete paths in line with createap.bat
REM

REM Maybe add option of deleting database here...
REM if not x%1==x goto DOIT
if "%II_SYSTEM%"=="" goto NOTSET
goto DOIT

REM :USAGE
REM echo.
REM echo Usage: %0 dbname
REM goto EXIT

:NOTSET
echo You must have both II_SYSTEM set in your environment.
goto EXIT

:DOIT
set ABFDEMODIR=%II_SYSTEM%\ingres\abf
echo.
echo WARNING! This program deletes all of the files in the ./%1
echo subdirectory of your abf directory.
echo.
echo You should destroy the database associated with this demo using
echo destroydb.
echo.
echo.
ipchoice Do you wish to continue?
if errorlevel 2 goto EXIT

echo Deleting %ABFDEMODIR%
cd %ABFDEMODIR%
del /Q *.*
rmdir /S .\%1

:EXIT
