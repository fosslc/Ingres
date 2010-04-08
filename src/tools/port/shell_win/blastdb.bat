@echo off
REM 
REM blastdb.bat - removes all files associated with a database.
REM
REM History:
REM
REM	27-jul-95 (tutto01)
REM	    Created.
REM

if "%1"=="" goto SHOW

rm -rf %II_SYSTEM%\ingres\*\default\%1


goto EXIT

:SHOW
echo.
echo The following database files exist...
echo.
ls %II_SYSTEM%\ingres\*\default
echo.
echo.
echo Type blastdb "dbname" to remove files...
echo.
echo.


:EXIT
