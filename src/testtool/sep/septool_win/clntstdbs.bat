@echo off
REM Clean up databases left by SEP tests
REM
REM History:
REM	22-sep-1995 (somsa01)
REM	    Created from clntstdbs.sh .
REM	02-Feb-1996 (somsa01)
REM	    Replaced "c:\tmp" with "ingprenv II_TEMPORARY".
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
REM

REM check the args
setlocal
:LOOP
if "%1"=="" goto CONTINU
  if not "%1"=="-a" goto ELSE
    set aflag=true
    shift
    goto LOOP
:ELSE
  echo   Usage: clntstdbs [-a]
  goto DONE

:CONTINU
set db=iidbdb
set dbcat=iidatabase
set name=name                    
set delim=

call ipset tmpdir ingprenv II_TEMPORARY
if not "%aflag%"=="true" goto ELSE2
	echo Removing ALL testenv databases.
	ingres -s %db% << EOS
range of d is %dbcat% \g \script %tmpdir%\tstdbs
retrieve (d.name) where d.own="testenv" \g \q
EOS
goto CONTINU2
:ELSE
	echo Removing databases with names of the form "[a-z][a-z][a-z][0-9][0-9]*"
	ingres -s %db% << EOS
range of d is %dbcat% \g \script %tmpdir%\tstdbs
retrieve (d.name) where d.name="[a-z][a-z][a-z][0-9][0-9]*" \g \q
EOS

:CONTINU2
awk -F'|' '{print $2}' %tmpdir%\tstdbs > %tmpdir%\dbnames
call ipset count expr 0
:LOOP2
call ipset count expr %count% + 1
call ipset dbname PCread %tmpdir%\dbnames %count%
if "%dbname%"=="" goto DONE1
  if "%dbname%"=="" goto LOOP2
  if "%dbname%"=="%name%" goto LOOP2
  if "%dbname%"=="%delim%" goto LOOP2
    echo DB: %dbname%
    destroydb -s %dbname%
    goto LOOP2
:DONE1
del %tmpdir%\tstdbs
del %tmpdir%\dbnames
:DONE
endlocal
