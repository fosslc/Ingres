@echo off
setlocal
REM
REM  Name: create_patch
REM
REM  Description:
REM	This batch file sets up a patch cdimage. It requires that a VERS
REM	file be present in this directory.
REM
REM  History:
REM	28-jan-1999 (somsa01)
REM	    Created. Remember that, what needs to reside in the
REM	    II_RELEASE_DIR directory (besides the ingres tree and what
REM	    gets copied from front!st!patch over there) is: CAINW32.EXE,
REM	    CALM_W32.DLL, CTL3D95.DLL, CTL3DNT.DLL, and Msvcrt.dll .
REM	17-feb-1999 (somsa01)
REM	    Added Doublebyte stuff.
REM	12-Mar-1999 (andbr08)
REM	    Added Console redirection of files for windows 95.  Now copy:
REM	    PCdate.exe and redirdos.exe to II_RELEASE_DIR
REM	29-Sep-1999 (andbr08)
REM	    Copy files listed in VERS file automatically from II_SYSTEM to
REM	    II_RELEASE_DIR
REM	12-jun-2000 (kitch01)
REM	    Changed MAS file names to reflect genlevel being built.
REM	    Do not copy DLLs to utility directory unless version is 9712
REM	29-may-2002 (somsa01)
REM	    Made changes for 2.6 patches.
REM	27-jun-2002 (somsa01)
REM	    CHanged II_RELEASE_DIR to II_PATCH_DIR.
REM	23-Aug-2002 (inifa01)
REM	    Updated to add appropriate references to 2.6/0207 version.
REM	28-Jul-2003 (penga03)
REM	    Updated to add appropriate references to 2.6/0305 version.
REM     29-apr-2004 (rigka01)
REM         wanfr01's performance improvements
REM     11-nov-2004 (rigka01)
REM         changes due to use of mkpatchhtml for 2.6 
REM

if not exist "VERS" echo Please create a VERS file before running create_patch& goto END

if "%II_PATCH_DIR%"=="" echo Please set II_PATCH_DIR to the location of the cdimage& goto END

if not exist %II_PATCH_DIR% echo Please set II_PATCH_DIR to a valid cdimage location& goto END

grep IngVersion VERS > nul
if errorlevel 1 echo IngVersion must be set to the version you are patching inside the VERS file& goto END

grep PATCHNO VERS > nul
if errorlevel 1 echo PATCHNO must be set to the patch number inside the VERS file& goto END

which PCread > nul
if errorlevel 1 echo Please go to testtool\tools and run nmake& goto END

which PCdate > nul
if errorlevel 1 echo Please go to testtool\tools and run nmake& goto END

grep FILES_CHANGED_BEGIN VERS > nul
if errorlevel 1 echo Please add a FILES_CHANGED_BEGIN / FILES_CHANGED_END section inside the VERS file describing the modified / new patch files& goto END
grep FILES_CHANGED_END VERS > nul
if errorlevel 1 echo Please add a FILES_CHANGED_BEGIN / FILES_CHANGED_END section inside the VERS file describing the modified / new patch files& goto END

call ipset ingvers grep IngVersion VERS
call ipset patchnum grep PATCHNO VERS

set %ingvers%
set %patchnum%
echo %IngVersion% | grep "2\.6\/0305" > nul
if not errorlevel 1 set masname=ingres_260305.mas& goto NEXT
echo %IngVersion% | grep "2\.6\/0207" > nul
if not errorlevel 1 set masname=ingres_260207.mas& goto NEXT
echo %IngVersion% | grep "2\.6\/0201" > nul
if not errorlevel 1 set masname=ingres_260201.mas& goto NEXT
echo %IngVersion% | grep "2\.5\/0011" > nul
if not errorlevel 1 set masname=ingres_0011.mas& goto NEXT
if not "%INGRESII%"=="" set masname=ingres_9808.mas& goto NEXT
if not "%DOUBLEBYTE%"=="" set masname=ingresntdbl.mas& goto NEXT
echo %IngVersion% | grep "int\.wnt" > nul
if errorlevel 1 set masname=ingresw95.mas& goto NEXT
set masname=ingres_9712.mas

:NEXT
echo Using %masname% ...
REM echo Is this correct? (Press any key to continue or CTRL+C to abort)
REM pause
REM Create the version.rel file in the cdimage location
echo Creating version.rel ...
echo %IngVersion%> %II_PATCH_DIR%\ingres\version.rel
echo %PATCHNO%>> %II_PATCH_DIR%\ingres\version.rel

REM Copy over the appropriate patch.txt
REM if "%masname%"=="ingres_260305.mas" cp patch_260305.html %II_PATCH_DIR%\ingres\patch.html
REM For 2.6, patch.html now directly created by mkpatchhtml and the templates procedures 
if "%masname%"=="ingres_260207.mas" cp patch_260207.html %II_PATCH_DIR%\ingres\patch.html
if "%masname%"=="ingres_0011.mas" cp patch_0011.txt %II_PATCH_DIR%\ingres\patch.txt& goto NEXT2
if "%masname%"=="ingres_9808.mas" cp patch_9808.txt %II_PATCH_DIR%\ingres\patch.txt& goto NEXT2
if "%masname%"=="ingresntdbl.mas" cp patch_dbl.txt %II_PATCH_DIR%\ingres\patch.txt& goto NEXT2
if "%masname%"=="ingresw95.mas" cp patch_w95.txt %II_PATCH_DIR%\ingres\patch.txt& goto NEXT2
if "%masname%"=="ingres_9712.mas" cp patch_9712.txt %II_PATCH_DIR%\ingres\patch.txt

:NEXT2
REM Examine the list of user files changed. If any of them are DLLs which
REM live in the utility directory, then copy them there in II_PATCH_DIR.
REM Also, take them and shove them in the "filelist" file.
echo Compiling list of files in patch ...
if exist filelist rm filelist
set count=0
:LOOP1
set /a count=%count%+1
PCread VERS %count% | grep FILES_CHANGED_BEGIN > nul
if errorlevel 1 goto LOOP1

if exist copyfile.bat rm copyfile.bat
:LOOP2
set /a count=%count%+1
PCread VERS %count% | grep FILES_CHANGED_END > nul
if not errorlevel 1 goto DONE

PCread VERS %count% >> filelist
PCread VERS %count% | cut -d " " -f 2 > tmpfile
PCread VERS %count% | awk '{print "cp -f %II_SYSTEM%\\" $1 $2 " %II_PATCH_DIR%\\" $1 $2}' >> copyfile.bat
call copyfile.bat
REM rm copyfile.bat

REM do not copy DLLs to utility if not 9712
if NOT "%masname%"=="ingres_9712.mas" goto LOOP2

call ipset filename PCread tmpfile 1
if "%filename%"=="oiadfdata.dll" cp %II_PATCH_DIR%\ingres\bin\oiadfdata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiadfnt.dll" cp %II_PATCH_DIR%\ingres\bin\oiadfnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiclfdata.dll" cp %II_PATCH_DIR%\ingres\bin\oiclfdata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiclfnt.dll" cp %II_PATCH_DIR%\ingres\bin\oiclfnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oicufnt.dll" cp %II_PATCH_DIR%\ingres\bin\oicufnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiembddata.dll" cp %II_PATCH_DIR%\ingres\bin\oiembddata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiembdnt.dll" cp %II_PATCH_DIR%\ingres\bin\oiembdnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oifrsdata.dll" cp %II_PATCH_DIR%\ingres\bin\oifrsdata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oifrsnt.dll" cp %II_PATCH_DIR%\ingres\bin\oifrsnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oigcfdata.dll" cp %II_PATCH_DIR%\ingres\bin\oigcfdata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oigcfnt.dll" cp %II_PATCH_DIR%\ingres\bin\oigcfnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oigcndata.dll" cp %II_PATCH_DIR%\ingres\bin\oigcndata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oigcnnt.dll" cp %II_PATCH_DIR%\ingres\bin\oigcnnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiglfdata.dll" cp %II_PATCH_DIR%\ingres\bin\oiglfdata.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
if "%filename%"=="oiglfnt.dll" cp %II_PATCH_DIR%\ingres\bin\oiglfnt.dll %II_PATCH_DIR%\ingres\utility& echo \ingres\utility\ %filename% ^0>> filelist
goto LOOP2

:DONE
REM Copy the lines from the masname into the filelist
set count=0
:LOOP3
set /a count=%count%+1
PCread %masname% %count% | grep "^:" > nul
if not errorlevel 1 goto LOOP3

:LOOP4
PCread %masname% %count% > nul
if errorlevel 2 goto DONE2
PCread %masname% %count% >> filelist
set /a count=%count%+1
goto LOOP4

:DONE2
REM Now, Uniquely sort the filelist
sort -u filelist > filelist2
rm filelist
ren filelist2 filelist

REM Run nmake; this will create installer.exe and the plist
echo Creating install program and plist ...
nmake

if errorlevel 1 goto END

REM Copy the results over to the II_PATCH_DIR area
cp installer.exe %II_PATCH_DIR%\install.exe
cp pinggcn.exe %II_PATCH_DIR%\pinggcn.exe
cp instaux.bat %II_PATCH_DIR%\instaux.bat
cp redirdos.exe %II_PATCH_DIR%\redirdos.exe
cp plist %II_PATCH_DIR%\ingres\plist
cp %ING_SRC%\ing_tools\bin\PCdate.exe %II_PATCH_DIR%\PCdate.exe
echo Done!
echo Make sure you add your release note(s) for your bugs to the patch.html
echo file, located in %II_PATCH_DIR%\ingres.
time /t
:END
endlocal
