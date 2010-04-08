@echo off
REM This restores all of the working files from bucode.bat saveset. The
REM build is restored to the original label and have list at the time
REM bucode.bat was run.
REM
REM Requirements: Ingres installation with ipchoice
REM
REM History:
REM     26-Jan-2000 (hanal04 & fanra01)
REM         File created.
REM     23-Feb-2000 (hanal04)
REM         Prompt user for restoration of SEP and testtool files.
REM         Allow %ING_SRC% and %TEMP% to be any value for backup 
REM         and restore on different machines.
REM         Release working files for ALL users.
REM	14-Jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.
REM
setlocal
pushd %ING_SRC%
if "%1" == "" goto usage

if "%USER%" == "" set USER=ingres

p working > %TEMP%\working.lst
grep "!" %TEMP%\working.lst
if errorlevel 1 goto RELEASED
echo .
echo "There are open/reserved files. CHANGES WILL BE LOST IF YOU CONTINUE"
call ipchoice Do you want to continue? (y or n)
if errorlevel 2 goto end
echo .
echo "Releasing all working files under all user names"
echo %TEMP%\| sed -e s/\\/\\\\/g | ipsetp BU_TEMP > %TEMP%\bu_temp.bat
call %TEMP%\bu_temp.bat
p working |%AWK_CMD% "{ print \"echo \" $7 \"| ipsetp USER > %BU_TEMP%tmp.bat \ncall %BU_TEMP%tmp.bat\necho \" $1 \"| p there -l - | %AWK_CMD% ++{print \"pushd \" $2 }++ > %BU_TEMP%tmp.bat \ncall %BU_TEMP%tmp.bat\np release \" $2 \"\npopd\n\" }" | sed -e s/++/"'"/g > %TEMP%\relall.bat
call %TEMP%\relall.bat

:RELEASED

echo .
echo "Unpacking build information files"
tar -xvf %1 -C %TEMP% >NUL 2>&1
tar -xvf %TEMP%\d.tar -C %TEMP% >NUL 2>&1

cat %TEMP%\user.lst | ipsetp USER > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
if "%USER%" == "" set USER=ingres
echo.
echo User ID set to %USER% (internal to batch job)

p have > %TEMP%\havenow.lst
diff %TEMP%\havenow.lst %TEMP%\labelhave.lst > %TEMP%\dif.lst
if not errorlevel 1 echo "Have list matches label"&cat %TEMP%\label.lst&goto GETNEXT
echo .
echo "Restoring have list to label"
grep -v "^<" %TEMP%\dif.lst | grep -v "^---" | %AWK_CMD% "{print $2\" \"$3\" \"$4}" > %TEMP%\get.lst
cat %TEMP%\label.lst
call ipchoice Do you want to restore sep and test tools? (y or n)
if not errorlevel 2 goto DORESTORE
grep -v "^sep" %TEMP%\get.lst | grep -v "!ingtest" > %TEMP%\tmp.lst
del %TEMP%\get.lst >NUL 2>&1
move %TEMP%\tmp.lst %TEMP%\get.lst >NUL 2>&1
grep "!" %TEMP%\get.lst
if errorlevel 1 echo "Current have list matches label"& goto GETNEXT
:DORESTORE
p get -f -l %TEMP%\get.lst

:GETNEXT

diff %TEMP%\labelhave.lst %TEMP%\have.lst > %TEMP%\dif.lst
if not errorlevel 1 echo "Have list matches backup"&goto INTEGRATED
echo .
echo "Getting required files revision which exceeded the label"
grep -v "^<" %TEMP%\dif.lst | grep -v "^---" | %AWK_CMD% "{print $2\" \"$3\" \"$4}" > %TEMP%\get.lst
p get -f -l %TEMP%\get.lst

:INTEGRATED

grep integrated %TEMP%\integrated.lst >NUL 2>&1
if errorlevel 1 echo "No integrated files"&goto OPEN
echo .
echo "Pulling files to integrated levels"
%AWK_CMD% "{print $1\" \"$2\" \"$8}" %TEMP%\integrated.lst | sed -e s/^)//g > %TEMP%\get.lst
p get -f -l %TEMP%\get.lst

:OPEN

grep "!" %TEMP%\opened.lst >NUL 2>&1
if errorlevel 1 echo "No opened files to open"&goto OPENRES
echo .
echo "Opening previously opened files"
p open -l %TEMP%\opened.lst

:OPENRES

grep "!" %TEMP%\reserved.lst >NUL 2>&1
if errorlevel 1 goto RESTORE
echo .
echo "Reserving previously reserved files"
p reserve -l %TEMP%\reserved.lst

:RESTORE

echo .
echo "Restoring source files"
tar -xvf %TEMP%\s.tar -C %ING_SRC% >NUL 2>&1

echo .
echo "Restoration of %1 is now complete"
goto end

:usage
echo Usage:
echo        %0 ^<tar filename^>
echo E.g.   %0 c:\patches\p6599\ingsrv909.tar
:end
popd
endlocal
