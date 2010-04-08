setlocal
REM
REM  Name: copy_build_to_release <masname>
REM
REM  Description:
REM	This batch file copies all files needed for a patch from a given
REM	mas file from II_SYSTEM\ingres (ING_BUILD) into
REM	II_PATCH_DIR/ingres.
REM
REM  History:
REM	02-feb-1999 (somsa01)
REM	    Created.
REM	08-feb-1999 (somsa01)
REM	    Make the II_RELEASE_DIR directories.
REM	18-jul-2000 (kitch01)
REM	    Do not copy DLLs to utility directory unless version is 9712
REM	27-jun-2002 (somsa01)
REM	    Changed II_RELEASE_DIR to II_PATCH_DIR.
REM     13-nov-2003 (rigka01)
REM         provide "quick" option; skip checking/creating directories
REM     29-apr-2004 (rigka01)
REM         wanfr01's performance improvements
REM

if "%II_PATCH_DIR%"=="" echo Please set II_PATCH_DIR to the location of the cdimage& goto END

if not exist %II_PATCH_DIR% echo Please set II_PATCH_DIR to a valid cdimage location& goto END

which PCread > nul
if errorlevel 1 echo Please go to testtool\tools and run nmake& goto END

if "%1"=="" echo Usage: copy_build_to_release {masname}& goto END
if not exist %1 echo Please provide a valid mas filename& goto END

if not "%1" == "ingres_9712.mas" goto NOCOPYDLL
REM Copy the appropriate DLLs from %II_SYSTEM%\ingres\bin to
REM %II_SYSTEM%\ingres\utility
echo Duplicating appropriate DLLs from bin to utility ...
cp -f %II_SYSTEM%\ingres\bin\oifrsnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oifrsdata.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiembdnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiembddata.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiclfnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiclfdata.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiglfnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiglfdata.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiadfnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oiadfdata.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oicufnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oigcfnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oigcfdata.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oigcnnt.dll %II_SYSTEM%\ingres\utility
cp -f %II_SYSTEM%\ingres\bin\oigcndata.dll %II_SYSTEM%\ingres\utility

:NOCOPYDLL
if "%2"=="quick" goto :COPYFILES
REM Make any II_PATCH_DIR directories that do not exist
echo.
echo Creating any II_PATCH_DIR directories that do not exist ...
set count=0
:PRELOOP1
set /a count=%count%+1
PCread %1 %count% | grep "^:" > nul
if not errorlevel 1 goto PRELOOP1

:PRELOOP2
PCread %1 %count% > nul
if errorlevel 2 goto CREATEDIRS
PCread %1 %count% | awk '{print "mkdir -p %II_PATCH_DIR%\\" $1 " >nul 2>&1"}' >> create_dirs.bat
set /a count=%count%+1
goto PRELOOP2

:CREATEDIRS
if exist create_dirs.bat sort -u create_dirs.bat > create_dirs2.bat
if exist create_dirs2.bat call create_dirs2.bat
echo rm -f create_dirs.bat create_dirs2.bat

:COPYFILES
REM Now, copy the appropriate files from %II_SYSTEM% to %II_PATCH_DIR%
echo.
echo Copying appropriate files from build area to patch area ...
if exist copyfiles.bat rm copyfiles.bat
set count=0
:LOOP1
set /a count=%count%+1
PCread %1 %count% | grep "^:" > nul
if not errorlevel 1 goto LOOP1

:LOOP2
PCread %1 %count% > nul
if errorlevel 2 goto DONE
PCread %1 %count% | awk '{print "cp -f %II_SYSTEM%\\" $1 $2 " %II_PATCH_DIR%\\" $1 $2}' >> copyfiles.bat
set /a count=%count%+1
goto LOOP2

:DONE
call copyfiles.bat
echo DONE!

:END
endlocal
