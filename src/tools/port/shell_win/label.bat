@echo off
REM This restores a build to a label identified by the parameters.
REM All working files will be released and changes lost if the
REM user chooses to continue.
REM
REM Requirements: Ingres installation with ipset, ipsetp, ipreadp, ipchoice 
REM
REM History:
REM     26-Jan-2000 (hanal04 & fanra01)
REM         File created.
REM     23-Feb-2000 (hanal04)
REM         Prompt user for restoration of SEP and testtool files.
REM         Release working files for ALL users.
REM	14-Jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.
REM
setlocal
pushd %ING_SRC%
if "%1" == "" goto usage
if "%2" == "" goto usage
if "%3" == "" goto usage

if "%USER%" == "" set USER=ingres

echo %TEMP%\| sed -e s/\\/\\\\/g | ipsetp BU_TEMP > %TEMP%\bu_temp.bat
call %TEMP%\bu_temp.bat

p working > %TEMP%\working.lst
grep "!" %TEMP%\working.lst
if errorlevel 1 goto RELEASED
echo "There are open/reserved files. CHANGES WILL BE LOST IF YOU CONTINUE"
call ipchoice Do you want to continue? (y or n)
if errorlevel 2 goto end
echo "Releasing all working files under all user names"
echo %USER% | ipsetp USER > %TEMP%\olduser.bat
p working |%AWK_CMD% "{ print \"echo \" $7 \"| ipsetp USER > %BU_TEMP%tmp.bat \ncall %BU_TEMP%tmp.bat\necho \" $1 \"| p there -l - | %AWK_CMD% ++{print \"pushd \" $2 }++ > %BU_TEMP%tmp.bat \ncall %BU_TEMP%tmp.bat\np release \" $2 \"\npopd\n" }" | sed -e s/++/"'"/g > %TEMP%\relall.bat
call %TEMP%\relall.bat
call %TEMP%\olduser.bat

:RELEASED

p have > %TEMP%\have.lst
p labels | grep -i %1 | grep -i %2 | grep -i %3 | %AWK_CMD% "{print $1}" > %TEMP%\label.lst
grep -n %1 %TEMP%\label.lst >NUL 2>&1
if errorlevel 1 echo "No matching label"& goto end
wc -l %TEMP%\label.lst | %AWK_CMD% "{print $1}" | ipsetp NUM_LABELS > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
expr %NUM_LABELS% = 1
if errorlevel 1 echo "Multiple matches found, please enter label number"&goto MULTI
if errorlevel 0 goto SINGLE

:MULTI
grep -n %1 %TEMP%\label.lst
grep -n %1 %TEMP%\label.lst > %TEMP%\label.tmp
ipreadp | ipsetp CHOICE > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
grep '^^%CHOICE%:' %TEMP%\label.tmp > %TEMP%\choice.tmp
if errorlevel 1 echo "Invalid Choice - please rety"& goto MULTI
%AWK_CMD% -F: "{print $2}" %TEMP%\choice.tmp | ipsetp BLD_LABEL > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
echo Selected Build Label
echo "%BLD_LABEL%"
goto RESTORE

:SINGLE
CALL ipset BLD_LABEL grep %1 %TEMP%\label.lst
echo Selected Build Label
echo "%BLD_LABEL%"
call ipchoice Do you want to continue? (y or n)
if errorlevel 2 goto end

:RESTORE
p label -p %BLD_LABEL% >%TEMP%\labelhave.lst
diff %TEMP%\have.lst %TEMP%\labelhave.lst > %TEMP%\dif.lst
if not errorlevel 1 echo "Have list matches label"&cat %TEMP%\label.lst&goto NEXT
grep -v "^<" %TEMP%\dif.lst | grep -v "^---" | %AWK_CMD% "{print $2\" \"$3\" \"$4}" > %TEMP%\get.lst
grep "!" %TEMP%\get.lst
if errorlevel 1 echo "No files to get"& echo "WARNING YOU'RE SUBSCRIBED TO DIRECTORIES THAT ARE NOT IN THE LABEL"&goto NEXT
call ipchoice Do you want to restore sep and test tools? (y or n)
if not errorlevel 2 goto DORESTORE
grep -v "^sep" %TEMP%\get.lst | grep -v "!ingtest" > %TEMP%\tmp.lst
del %TEMP%\get.lst >NUL 2>&1
move %TEMP%\tmp.lst %TEMP%\get.lst >NUL 2>&1
grep "!" %TEMP%\get.lst
if errorlevel 1 echo "Current have list matches label"& goto NEXT
:DORESTORE
echo "Restoring have list to that of label"
echo %BLD_LABEL%
p get -f -l %TEMP%\get.lst

:NEXT
goto end

:usage
echo Usage:
echo       %0 ^<mark^> ^<platform^> ^<maintenance release^>
echo E.g.  %0 m1030 wnt 9808

:end
popd
endlocal
