@echo off
REM
REM  Compare an "original" .sep file with one derived from it, but which
REM  contains altcanons.  The first parameter is the name of the original
REM  file, the second is the name of the derived file.
REM
REM  Note: This script depends on the existence of the "tac" program.  That's
REM  "cat" spelled backwards, and it does what you might expect, namely writes
REM  out its input in line-reversed order.  The source for tac should be
REM  distributed with this program.  
REM
REM  History:
REM
REM	21-sep-1995 (somsa01)
REM		Created from altcompare.sh .
REM	18-Jan-1996 (somsa01)
REM		Replaced "rm -rf" with "del" and "rd".
REM	02-Feb-1996 (somsa01)
REM		Replaced "c:\tmp" with "ingprenv II_TEMPORARY".
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
REM
REM

REM
REM  Make sure the params are ok...
REM
if not "%1"=="" goto CONTINU
echo Usage: altcompare origfile derivedfile
echo   ...where: origfile = an original .sep file
echo      and derivedfile = a file derived from origfile which contains altcanons
echo. 
echo or:    altcompare {filename}
echo   ...in which case {filename}.sep is used for the original file, and
echo      {filename}.upd is used for the derived file.
goto DONE2

if "%3"=="" goto CONTINU
echo Usage: altcompare origfile derivedfile
echo   ...where: origfile = an original .sep file
echo      and derivedfile = a file derived from origfile which contains altcanons
echo.
echo or:    altcompare {filename}
echo   ...in which case {filename}.sep is used for the original file, and
echo      {filename}.upd is used for the derived file.
goto DONE2

:CONTINU
REM
REM  Useful things for displays...
REM
setlocal
set dashes=---------------------------------------
set dashes=%dashes%-%dashes%

REM
REM  Create a directory to keep temporary files in, and make sure it goes
REM  away if we exit for any reason.
REM
call ipset tmpdir ingprenv II_TEMPORARY
set tmpdir=%tmpdir%\altc.$$
md %tmpdir%
REM trap '/bin/rm -rf $tmpdir; exit' 0
REM trap 'exit' 2

REM
REM  Some file names in the temp directory...
REM

set orig=%tmpdir%\orig
set derived=%tmpdir%\derived
set alt=%tmpdir%\altcanon
set main=%tmpdir%\maincanon

REM
REM  Generate full path names for the input files.
REM
if not "%2"=="" goto ELSE
  echo %1 > %tmpdir%\tempfile
  call ipset tempvar cut -c2 %tmpdir%\tempfile
  del %tmpdir%\tempfile
  if "%tempvar%"==":" goto YESPATH
  call ipset f cd
  set f=%f%\%1
  goto SETFILE
:YESPATH
  set f=%1
  goto SETFILE
:NOPATH

:SETFILE
  sepfile=%f%.sep
  updfile=%f%.upd
  goto CONTINU2
:ELSE
  echo %1> %tmpdir%\tempfile
  call ipset tempvar cut -c2 %tmpdir%\tempfile
  del %tmpdir%\tempfile
  if "%tempvar%"==":" goto YESPATH2
  call ipset sepfile cd
  set sepfile=%sepfile%\%1
  call ipset updfile cd
  set updfile=%updfile%\%2
  goto CONTINU2
:YESPATH2
  set sepfile=%1
  set updfile=%2

:CONTINU2
REM
REM  Preprocess the input files.  First we'll get rid of leading pound
REM  signs, because they make the C preprocessor unhappy.  Then we use
REM  the preprocessor to get rid of comments in the sep files.
REM

call ipset olddir cd
c:
cd %tmpdir%

sed "s/^#/\<pound\>/" %sepfile% > temp.c
cl /P temp.c
sed "/^$/d" temp.i > %orig%

sed "s/^#/\<pound\>/" %updfile% > temp.c
cl /P temp.c
sed "/^$/d" temp.i > %derived%

del temp.*

REM
REM  Diff the two input files, and grab all lines that indicate that a block
REM  of something has been added to the derived file.  We'll just blindly
REM  believe that it's an altcanon...
REM

diff %orig% %derived% > tempfile
grep "^[0-9]*a[0-9]*,[0-9]" tempfile > tempfile2
call ipset count expr 0

:BEGIN
call ipset count expr count + 1
call ipset line PCread tempfile2 %count%
   if "%line%"=="" goto DONE
       echo %line% > tempfile
       cut -f2 -da tempfile > tempfile3
       call ipset linenum cut -f1 -d, tempfile3
       del tempfile3
       call ipset linenum1 expr %linenum% - 1
       numtoexit %linenum1%
       if not errorlevel 1 goto BEGIN
       del %alt% %main%
REM
REM  Get the altcanon...
REM
       sed -e "1,%linenum1%d"  -e "/^>>$/,$d" %derived% > %alt%
       PCecho ">>" >> %alt%
REM
REM  Get the main canon..
REM
:WHILE
	 set sedcmd=%linenum%,\$d
         sed "%sedcmd%" %derived% | tac | sed -e "/^<<$/=" -e "d" | sed "2,$d" > tempfile
         call ipset linenum1 PCread tempfile 1
         del tempfile
	 call ipset linenum expr %linenum% - %linenum1%
	 call ipset linenum1 expr %linenum% - 1
	 call ipset command sed -e "%linenum1%p" -e "d" %derived%
	 if not "%command%"==">>" goto ENDWHILE
         goto WHILE
:ENDWHILE
       sed -e "1,%linenum1%d" -e "/^>>/,$d" %derived% > %main%
       PCecho ">>" >> %main%
REM
REM  Diff them...
REM
       echo %dashes%
       echo "%command%"
       echo %dashes%
       diff %main% %alt%
       echo %dashes%
       echo.
       echo.
       echo.
       goto BEGIN
:DONE
cd ..
for %%i in (%tmpdir%\*.*) do del %%i
rd %tmpdir%
cd %olddir%
echo %olddir% > tempfile
call ipset olddir cut -c1 tempfile
del tempfile
%olddir%:
:DONE2
endlocal
