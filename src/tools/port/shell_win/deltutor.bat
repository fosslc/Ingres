@echo off

REM
REM deltutor.bat - deletes files associated with tutorial
REM 
REM History:
REM	24-jul-95 (tutto01)
REM		Ported to NT.
REM 

if not exist *.osq echo This MUST be run in the directory where the VisionTutor was run!
if not exist *.osq goto USAGE
if not x%1==x goto DOIT
:USAGE
echo.
echo Usage is %0 dbname
echo Usage is NOT %0 node::dbname
echo.
echo Use dbname = "none", if you do not want to cleanup
echo -		%II_SYSTEM%\ingres\abf\dbname\phone_or\*.*
echo You must also run destroydb dbname
echo -		on the host system to remove the database
goto EXIT

:DOIT
echo.
echo The following files will be removed...
echo.
dir /w
echo.
echo Do you wish to continue?
choice
if errorlevel 2 goto EXIT
del *.*
if %1==none goto EXIT
if %1=="none" goto EXIT
if %1==NONE goto EXIT
if %1=="NONE" goto EXIT
cd %II_SYSTEM%\ingres\abf
cd %1
cd phone_or
if exist phone_or.c dir /w
if exist phone_or.c del *.*
cd ..
rmdir phone_or
cd ..
rmdir %1
cd \
cd
:EXIT
