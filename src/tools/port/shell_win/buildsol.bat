@echo off
setlocal
REM
REM  Name: buildsol.bat
REM	This script is setup to create a SOL distributable on windows.
REM	It employs Patch Installer for install of SOL distribution.
REM	It uses patchid of 00000 which Patch Installer is programmed to recognize as SOL distribution.
REM
REM  Description:
REM
REM  History:
REM	16-Jun-2006 (drivi01)
REM		Created.


:CHECK1
if not x%II_SOL_DIR% == x goto CONT1
echo You must set II_SOL_DIR to desirable destination for SOL distributable!!
goto DONE

:CONT1
if not exist %II_SOL_DIR% mkdir %II_SOL_DIR%

:CHECK2
if not x%II_SYSTEM%==x goto CHECK3
echo You must set II_SYSTEM !!
goto DONE

:CHECK3
if not x%ING_SRC%==x goto CHECK4
echo You must set ING_SRC !!
goto DONE

:CHECK4
if not x%BLDROOT%==x goto CHECK5
echo You must set BLDROOT !!
goto DONE

:CHECK5
if not x%II_SOL_RELEASE_DIR%==x goto CHECK6
echo You must set II_SOL_RELEASE_DIR!!
goto DONE

:CHECK6
if not x%AWK_CMD%==x goto STEP1
echo You must set AWK_CMD to awk if using MKS or gawk if cygwin!!
goto DONE


:STEP1
rm %II_SOL_DIR%\*
call buildrel -p -s
cp %ING_BUILD%\picklist.dat %II_SOL_DIR%

:STEP2
cat %ING_BUILD%\version.rel |head -1> "%II_SOL_DIR%\version.rel"
echo 00000 >> %II_SOL_DIR%\version.rel

REM 
REM Check if the directory for files being copied from picklist_SOL.dat exist on the patchimage.
REM 
echo @echo off > %TMPDIR%\buildsol.$$1.bat
cat %II_SOL_DIR%\picklist.dat|sed s:.\\:\\:|sed s:/:\\:g|%AWK_CMD% '{print "dirname "$1}' >> %TMPDIR%\buildsol.$$1.bat
call %TMPDIR%\buildsol.$$1.bat > %TMPDIR%\buildsol.$$2.bat
cat  %TMPDIR%\buildsol.$$2.bat| sed s:/:\\:g |%AWK_CMD% '{print "if not exist %%II_SOL_DIR%%\\\\patch\\\\ingres"$1" mkdir %%II_SOL_DIR%%\\\\patch\\\\ingres"$1}' > %TMPDIR%\buildsol.$$3.bat
call %TMPDIR%\buildsol.$$3.bat
rm %TMPDIR%\buildsol.$$1.bat
rm %TMPDIR%\buildsol.$$2.bat
rm %TMPDIR%\buildsol.$$3.bat

REM
REM Checksum files that will be patched from %PICKLIST_SOL% in II_SYSTEM then save checksum to
REM patchfiles.lis in II_SOL_DIR with II_SOL_DIR path to files.
REM
echo %II_SYSTEM%|sed s:\\:\\\\:g|%AWK_CMD% '{print "@set buildsol_system="$0}'>"%TMPDIR%\buildsol.$$3.bat" 
call "%TMPDIR%\buildsol.$$3.bat" 
rm "%TMPDIR%\buildsol.$$3.bat" 
echo @echo off>"%TMPDIR%\buildsol.$$1.bat"
echo type "%TMPDIR%\buildsol.$$1.bat"
type "%TMPDIR%\buildsol.$$1.bat"
cat %II_SOL_DIR%\picklist.dat|%AWK_CMD% '{print "iicksum "$0}'|sed s:.\\:%%II_SYSTEM%%\\ingres\\: |sed 's:/:\\:g' >> "%TMPDIR%\buildsol.$$1.bat"
echo type "%TMPDIR%\buildsol.$$1.bat"
type "%TMPDIR%\buildsol.$$1.bat"
call "%TMPDIR%\buildsol.$$1.bat">"%TMPDIR%\buildsol.$$2"
echo type "%TMPDIR%\buildsol.$$2"
type "%TMPDIR%\buildsol.$$2"
if not %ERRORLEVEL%==0 goto ERROR1
cat "%TMPDIR%\buildsol.$$2"|sed -e s$'%buildsol_system%'$patch$g|sed -e s/\//\\/g>%II_SOL_DIR%\patchfiles.lis
echo type %II_SOL_DIR%\patchfiles.lis
type %II_SOL_DIR%\patchfiles.lis
rm "%TMPDIR%\buildsol.$$2"   "%TMPDIR%\buildsol.$$1.bat"

if not exist %II_SOL_DIR%\patch\ingres mkdir %II_SOL_DIR%\patch\ingres
if not exist %II_SOL_DIR%\patch\ingres\bin mkdir %II_SOL_DIR%\patch\ingres\bin
if not exist %II_SOL_DIR%\patch\ingres\lib mkdir %II_SOL_DIR%\patch\ingres\lib
if not exist %II_SOL_DIR%\patch\ingres\files mkdir %II_SOL_DIR%\patch\ingres\files
if not exist %II_SOL_DIR%\patch\ingres\utility mkdir %II_SOL_DIR%\patch\ingres\utility


:STEP3
:CONT1
echo.
"%MKSLOC%\echo" "Version: \c" 
cat %II_SYSTEM%/ingres/version.rel
echo SOL directory: %II_SOL_DIR%
echo.

:STEP4
echo Copying binaries on the list to %II_SOL_DIR% directory
cat %II_SOL_DIR%\picklist.dat|sed s:.\\:\\:|sed s:/:\\:g |%AWK_CMD% '{print "copy /y %%II_SYSTEM%%\\\\ingres"$1" %%II_SOL_DIR%%\\\\patch\\\\ingres"$1}' >  "%TMPDIR%\buildsol.$$1.bat"
call "%TMPDIR%\buildsol.$$1.bat"
if not %ERRORLEVEL% == 0 goto ERROR2_SOL
rm  "%TMPDIR%\buildsol.$$1.bat"

:STEP5
echo Building patch package
cd %ING_SRC%\front\st\patch_win
call chmod 777 PatchProject.ism
ISCmdBld.exe -x -p PatchProject.ism -b %II_SOL_RELEASE_DIR%\PatchProject
if errorlevel 1 goto ERROR3
chmod 444 PatchProject.ism

echo Copying files
cp -p -v %II_SYSTEM%\ingres\bin\PatchUpdate.exe %II_SOL_DIR%
copy /y "%II_SOL_RELEASE_DIR%\PatchProject\Product Configuration 1\Release 1\DiskImages\Disk1"\*.* %II_SOL_DIR%
cp -p -v %ING_BUILD%\picklist.dat %II_SOL_DIR%
rm -rf %II_SOL_DIR%\setup.*

:CONT5
echo DONE!!!
goto DONE


:ERROR1
echo buildsol.bat hit a bad error while trying to get checksums of files being rolled.  Aborting!!!
goto DONE

:ERROR2
rm  "%TMPDIR%\buildsol.$$1.bat"
echo buildsol.bat hit a bad error while copying files to %II_SOL_DIR%.  Aborting!!!
goto DONE

:ERROR3
echo buildsol.bat hit a bad error while running ISCmdBld.exe  on PatchProject ism.
goto DONE

:DONE
endlocal


