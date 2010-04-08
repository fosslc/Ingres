@echo off
REM This copies all of the working files from a build area and saves
REM them in a specified tar file along with data which can be used to
REM restore a build to the current build state.
REM The build is then restored to a label identified using the parameters.
REM
REM Requirements: Ingres installation with ipset, ipsetp, ipreadp, ipchoice
REM
REM History:
REM     26-Jan-2000 (hanal04 & fanra01)
REM         File created.
REM     23-Feb-2000 (hanal04)
REM         Prompt user for restoration of SEP and testtool files.
REM         Allow %ING_SRC% and %TEMP% to be any value for backup
REM         and restore on different machines.
REM	14-jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.	
REM
setlocal
pushd %ING_SRC%
if "%1" == "" goto usage
if "%2" == "" goto usage
if "%3" == "" goto usage
if "%4" == "" goto usage

if "%USER%" == "" set USER=ingres

p labels | grep -i %2 | grep -i %3 | grep -i %4 | %AWK_CMD% "{print $1}" > %TEMP%\label.lst
grep -n %2 %TEMP%\label.lst >NUL 2>&1
if errorlevel 1 echo "No matching label"& goto end
wc -l %TEMP%\label.lst | %AWK_CMD% "{print $1}" | ipsetp NUM_LABELS > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
expr %NUM_LABELS% = 1
if errorlevel 1 echo "Multiple matches found, please enter label number"&goto MULTI
if errorlevel 0 goto SINGLE

:MULTI
grep -n %2 %TEMP%\label.lst
grep -n %2 %TEMP%\label.lst > %TEMP%\label.tmp
ipreadp | ipsetp CHOICE > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
grep '^^%CHOICE%:' %TEMP%\label.tmp > %TEMP%\choice.tmp
if errorlevel 1 echo "Invalid Choice - please rety"& goto MULTI
%AWK_CMD% -F: "{print $2}" %TEMP%\choice.tmp | ipsetp BLD_LABEL > %TEMP%\tmp.bat
call %TEMP%\tmp.bat
echo .
echo Selected Build Label
echo "%BLD_LABEL%"
echo %BLD_LABEL% > %TEMP%\label.lst
goto BUILDINFO

:SINGLE
CALL ipset BLD_LABEL grep %2 %TEMP%\label.lst
echo .
echo Selected Build Label
echo "%BLD_LABEL%"
call ipchoice Do you want to continue? (y or n)
if errorlevel 2 goto end

:BUILDINFO

echo .
echo "Gathering build data"
echo %USER% > %TEMP%\user.lst
p label -p %BLD_LABEL% > %TEMP%\labelhave.lst
p here > %TEMP%\client.lst
p have > %TEMP%\have.lst
p opened > %TEMP%\opened.lst
p reserved > %TEMP%\reserved.lst
p working > %TEMP%\work.lst
grep "integrated to" %TEMP%\work.lst > %TEMP%\integrated.lst
p there -l %TEMP%\work.lst > %TEMP%\working.lst
echo %TEMP%\working.lst > %TEMP%\dlist.lst
echo %TEMP%\user.lst >> %TEMP%\dlist.lst
echo %TEMP%\client.lst >> %TEMP%\dlist.lst
echo %TEMP%\have.lst >> %TEMP%\dlist.lst
echo %TEMP%\reserved.lst >> %TEMP%\dlist.lst
echo %TEMP%\opened.lst >> %TEMP%\dlist.lst
echo %TEMP%\label.lst >> %TEMP%\dlist.lst
echo %TEMP%\labelhave.lst >> %TEMP%\dlist.lst
echo %TEMP%\integrated.lst >> %TEMP%\dlist.lst
cat %TEMP%\working.lst | %AWK_CMD% "{print $3\"\\\"$4}" > %TEMP%\slist.lst
echo .

REM Setpup %TEMP% and %ING_SRC% independant file lists
echo %TEMP%\| sed -e s/\\/\\\\/g | ipsetp BU_TEMP > %TEMP%\bu_temp.bat
call %TEMP%\bu_temp.bat
cat %TEMP%\dlist.lst | sed -e s/%BU_TEMP%//g > %TEMP%\gdlist.lst
echo %ING_SRC%\| sed -e s/\\/\\\\/g | ipsetp BU_SRC > %TEMP%\bu_src.bat
call %TEMP%\bu_src.bat
cat %TEMP%\slist.lst | sed -e s/%BU_SRC%//g > %TEMP%\gslist.lst

echo .
echo "Generating working tar file"
cat %TEMP%\gdlist.lst | tar cvf  %TEMP%\d.tar -C %TEMP% -

echo .
echo "Generating source tar file"
cat %TEMP%\gslist.lst | tar cvf  %TEMP%\s.tar -C %ING_SRC% -

echo d.tar > %TEMP%\tar.lst
echo s.tar >> %TEMP%\tar.lst
echo .
echo "Backing up source and working tar files"
cat %TEMP%\tar.lst | tar cvf %1 -C %TEMP% -
echo .
call ipchoice Do you want to restore the build to the label? (y or n)
if errorlevel 2 goto end
echo "Releasing working files under all user names"
p working |%AWK_CMD% "{ print \"echo \" $7 \"| ipsetp USER > %BU_TEMP%tmp.bat \ncall %BU_TEMP%tmp.bat\necho \" $1 \"| p there -l - | %AWK_CMD% ++{print \"pushd \" $2 }++ > %BU_TEMP%tmp.bat \ncall %BU_TEMP%tmp.bat\np release \" $2 \"\npopd\n\" }" | sed -e s/++/"'"/g > %TEMP%\relall.bat
call %TEMP%\relall.bat
cat %TEMP%\user.lst | ipsetp USER > %TEMP%\tmp.bat
call %TEMP%\tmp.bat

diff %TEMP%\have.lst %TEMP%\labelhave.lst | grep -v "^<" | grep -v "^---" | %AWK_CMD% "{print $2\" \"$3\" \"$4}" > %TEMP%\get.lst
echo .
call ipchoice Do you want to restore sep and test tools? (y or n)
if not errorlevel 2 goto DORESTORE
grep -v "^sep" %TEMP%\get.lst | grep -v "!ingtest" > %TEMP%\tmp.lst
del %TEMP%\get.lst >NUL 2>&1
move %TEMP%\tmp.lst %TEMP%\get.lst >NUL 2>&1
grep "!" %TEMP%\get.lst
if errorlevel 1 echo "Current have list matches label"& goto end
:DORESTORE
echo "Restoring build to label"
echo %BLD_LABEL%
p get -f -l %TEMP%\get.lst

goto end

:usage
echo Usage:
echo        %0 ^<tar filename^> ^<mark^> ^<platform^> ^<maintenance release^>
echo E.g.   %0 c:\patches\p6599\ingsrv909.tar m1030 wnt 9808
:end
popd
endlocal
