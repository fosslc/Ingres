@echo off
REM	Remove all SEP and LISTEXEC temporary files from $ING_TST 
REM	and all log output files other than in $ING_TST/output.
REM
REM History:
REM	22-sep-1995 (somsa01)
REM		Created from clningtst.sh .
REM	18-Jan-1996 (somsa01)
REM		Replaced "rm -f" with "del".
REM

setlocal
call ipset olddir cd
cd %ING_TST%
echo %ING_TST% > tempfile
call ipset drive cut -c1 tempfile
del tempfile
%drive%:

echo Removing all SEP and LISTEXEC temporary files from %ING_TST% ...
for %%tfm in (*.stf *.syn in_*.sep out_*.sep *.dif) do PCfind . -type f -name %%tfm -print >> delfile
call ipset count expr 0
:BEGIN
call ipset count expr count + 1
call ipset tf PCread delfile %count%
  if "%tf%"=="" goto DONE
    echo Removing from %ING_TST%: %tf%
    del %tf%
    goto BEGIN

:DONE
del delfile
echo Removing all SEP and LISTEXEC log output files from %ING_TST%
echo 	except in %ING_TST%\output ...
for %%outm in (*.log *.out *.rpt) do PCfind . -type f -name %%outm -print >> delfile2
grep -v output delfile2 > delfile
del delfile2
call ipset count expr 0
:BEGIN2
call ipset count expr count + 1
call ipset out PCread delfile %count%
  if "%out%"=="" goto DONE2
    echo Removing from %ING_TST%: %out%
    del %out%
    goto BEGIN2

:DONE2
del delfile
cd %olddir%
echo %olddir% > tempfile
call ipset drive cut -c1 tempfile
del tempfile
%drive%:

endlocal
