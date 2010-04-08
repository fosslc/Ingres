@echo off
REM 
REM cksepenv --   checks to see if the environment is OK for SEP tests
REM
REM 	Flags available:
REM		-v (verbose)
REM		-s (silent)
REM		-c (continue, even if errors found)
REM
REM Exit status is 0 if environment seems okay.
REM 
REM Otherwise, error message is echoed and exit is non-zero.
REM
REM	exit	means
REM	----	-----
REM	   1	an environment variable not set or
REM	           an executable is not found
REM	   3	user is not 'testenv'
REM	   4	qasetuser is not working
REM	   5    user has umask other than 2
REM	   6	a test login is not in testenv's group
REM	   7    a test login can't create a file
REM	   8	wrong number of name servers up
REM	   9	no dbms servers up
REM       10    ingchkdate found II_AUTHORIZATION error
REM	  11	II_INSTALLATION isn't set
REM	  12	wrong or missing 'users' file entry
REM	  13    bad command line flag
REM	  14	missing or uncreatable output directory
REM

REM History:
REM
REM 22-sep-1995 (somsa01)
REM	Created from cksepenv.sh .
REM 03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems

REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Get command line args
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

setlocal
set Vecho=REM
set Eecho=echo
set Cexit=goto DONE

:LOOP
if "%1"=="" goto END
  if not "%1"=="-v" goto CASE2
    set Vecho=echo
    shift
    goto LOOP
:CASE2
  if not "%1"=="-s" goto CASE3
    set Eecho=REM
    shift
    goto LOOP
:CASE3
  if not "%1=="-c" goto DEFAULT
    set Cexit=echo Continuing instead of exiting with code
    set Vecho=echo
    shift
    goto LOOP
:DEFAULT
  %Eecho% Unknown flag '%1'
  call ipset exitcode expr 13
  %Cexit%

:END
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Porting environment variables are needed:
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking for porting environment variables...

if not "%ING_TST%"=="" goto CONTIN
  %Eecho% ING_TST environmant variable must be set
  call ipset exitcode expr 1
  %Cexit%
:CONTIN
if not "%ING_BUILD%"=="" goto CONTIN2
  %Eecho% ING_BUILD environment variable must be set
  call ipset exitcode expr 1
  %Cexit%

:CONTIN2
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Need an INGRES environment:
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking II_SYSTEM ...

if not "%II_SYSTEM%"=="" goto CONTIN3
  %Eecho% II_SYSTEM environment variable must be set
  call ipset exitcode expr 1
  %Cexit%

:CONTIN3
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Get installation code
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking for installation code...

call ipset II ingprenv II_INSTALLATION
if not "%II%"=="" goto CONTIN4
  %Eecho% II_INSTALLATION must be set in the INGRES symbol table
  call ipset exitcode expr 11
  %Cexit%

:CONTIN4
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM User running tests must be 'testenv'
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking effective userid....

if "%USERNAME%"=="testenv" goto CONTIN5
  %Eecho% You must be user 'testenv' to run tests
  call ipset exitcode expr 3
  %Cexit%

:CONTIN5
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Test environment variables must be set:
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking for test environment variables....

if not "%TST_TESTOOLS%"=="" goto NEXT
  %Eecho% TST_TESTOOLS must be set
  call ipset exitcode expr 1
  %Cexit%
:NEXT
if not "%TST_SEP%"=="" goto NEXT2
  %Eecho% TST_SEP must be set
  call ipset exitcode expr 1
  %Cexit%
:NEXT2
if not "%ING_EDIT%"=="" goto NEXT3
  %Eecho% ING_EDIT must be set
  call ipset exitcode expr 1
  %Cexit%
:NEXT3
if not "%PEDITOR%"=="" goto NEXT4
  %Eecho% PEDITOR must be set
  call ipset exitcode expr 1
  %Cexit%
:NEXT4
if not "%TST_TESTENV%"=="" goto NEXT5
  %Eecho% TST_TESTENV must be set
  call ipset exitcode expr 1
  %Cexit%
:NEXT5
if not "%TST_LISTEXEC%"=="" goto CONTIN6
  %Eecho% TST_LISTEXEC must be set
  call ipset exitcode expr 1
  %Cexit%

:CONTIN6
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Are the logins for the test users set up correctly?
REM (This also makes sure qasetuser works.)
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking test logins with 'qasetuser'...

call ipset tmpdir ingprenv II_TEMPORARY
echo testenv>> %tmpdir%\cksepenv.tmp
echo qatest>> %tmpdir%\cksepenv.tmp
echo ingres>> %tmpdir%\cksepenv.tmp
echo pvusr1>> %tmpdir%\cksepenv.tmp
echo pvusr2>> %tmpdir%\cksepenv.tmp
echo pvusr3>> %tmpdir%\cksepenv.tmp
echo pvusr4>> %tmpdir%\cksepenv.tmp

set ext=$$

call ipset count expr 0
:LOGINLOOP
call ipset count expr %count% + 1
call ipset tlogins PCread %tmpdir%\cksepenv.bat %count%
if "%tlogins%"=="" goto CONTIN7

  qasetuser %tlogins% touch %tlogins%.%ext% 2> %tmpdir%\cksepenv.tmp2
  call ipset errflag PCread %tmpdir%\cksepenv.tmp2 1
  del %tmpdir%\cksepenv.tmp2
  if "%errflag%"=="" goto LOGINNEXT
	%Eecho% 'qasetuser' failed for login %tlogins%
	call ipset exitcode expr 4
	%Cexit%

:LOGINNEXT
  if not exist %tlogins%.%ext% goto FAIL
	del %tlogins%.%ext% 2> %tmpdir%\cksepenv.tmp2
	call ipset errflag PCread %tmpdir%\cksepenv.tmp2 1
	del %tmpdir%\cksepenv.tmp2
	if "%errflag%"=="" goto LOGINLOOP
		%Eecho% testenv can't remove a file created by %tlogins%
		%Eecho% Maybe testenv and %tlogins% are in different groups?
		call ipset exitcode expr 6
		%Cexit%
:FAIL
	%Eecho% %tlogins% can't create a file in this directory
	call ipset exitcode expr 7
	%Cexit%

:CONTIN7
del %tmpdir%\cksepenv.tmp
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Are the output directories there? The main one is assumed to have
REM been put there by mkidir, and the subdirs get created now if they're
REM not already there.
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking for test output directories...

call ipset olddir cd

cd %ING_TST%\output 2> c:\logfile.tmp
call ipset status PCread c:\logfile.tmp 1
if "%status%"=="" goto CONTIN8
  del c:\logfile.tmp
  %Eecho% The %ING_TST%\output directory tree is missing
  call ipset exitcode expr 14
  goto DONE
:CONTIN8
del c:\logfile.tmp
cd %TST_OUTPUT% 2> c:\logfile.tmp
call ipset status PCread c:\logfile.tmp 1
if "%status%"=="" goto CONTIN9
  del c:\logfile.tmp
  %Eecho% The %TST_OUTPUT% directory tree is missing
  call ipset exitcode expr 14
  %Cexit%

:CONTIN9
del c:\logfile.tmp
cd %olddir%
echo be>> dirlist
echo fe>> dirlist
echo embed>> dirlist
echo star>> dirlist
echo init>> dirlist
echo audit>> dirlist
echo net>> dirlist
echo net\local>> dirlist
echo net\loopback>> dirlist
echo net\remote>> dirlist
call ipset count expr 0
:LOOP2
call ipset count expr %count% + 1
call ipset outdir PCread dirlist %count%
if "%outdir%"=="" goto CONTIN10
  cd %TST_OUTPUT%\%outdir% 2> c:\logfile.tmp
  call ipset status PCread c:\logfile.tmp 1
  if "%status%"=="" goto DIREXIST
    del c:\logfile.tmp
    md %TST_OUTPUT%\%outdir% 2> c:\logfile.tmp
    call ipset status PCread c:\logfile.tmp 1
    if not "%status%"=="" goto DIRERROR
      %Vecho% Created test output directory %TST_OUTPUT%\%outdir%
      goto LOOP2
:DIRERROR
    del c:\logfile.tmp
    %Eecho% Can't create output directory %TST_OUTPUT%\%outdir% 
    call ipset exitcode expr 14
    %Cexit%
    goto LOOP2

:DIREXIST
  if exist c:\logfile.tmp del c:\logfile.tmp
  cd %TST_OUTPUT%
  goto LOOP2

:CONTIN10
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Check for only one name server and at least one DBMS server. 
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking for an active name server in installation %II%...

echo show ingres | iinamu > c:\resfile.tmp
call ipset result grep "E_GC0021_NO_PARTNER" c:\\resfile.tmp
if "%result%"=="" goto CONTIN11
  %Eecho% There is no name server running in installation %II%
  del c:\resfile.tmp
  call ipset exitcode expr 8
  %Cexit%

:CONTIN11
%Vecho% Checking for active database server(s) in installation %II%...

grep "*" c:\\resfile.tmp | sed "s/IINAMU> //" > c:\\resfile2.tmp
del c:\resfile.tmp

call ipset status PCread c:\resfile2.tmp 1
if "%status%"=="" goto ERROR
  call ipset count expr 0
:LOOP3
  call ipset count expr %count% + 1
  call ipset line PCread c:\resfile2.tmp %count%
  if not "%line%"=="" goto LOOP3
  call ipset count expr %count% - 1
  %Vecho% Found %count% database server(s) running in installation %II%
  del c:\resfile2.tmp
  goto CONTIN12

:ERROR
  %Eecho% There are no database servers in installation %II%
  del c:\resfile2.tmp
  call ipset exitcode expr 9
  %Cexit%

:CONTIN12
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Does the iidbdb database contain the correct user entries?
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%Vecho% Checking the iidbdb database in installation %II%

echo select name, gid, mid, status from iiuser where name IN ('ingres', 'pvusr1', 'pvusr2', 'pvusr3', 'pvusr4', 'qatest', 'testenv')\g > c:\user.sql
call sql  -s iidbdb < c:\user.sql > c:\iidbdb.out
@echo off
del c:\user.sql

grep ingres c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out
grep pvusr1 c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out
grep pvusr2 c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out
grep pvusr3 c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out
grep pvusr4 c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out
grep qatest c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out
grep testenv c:\iidbdb.out | sed "s/ //g" | sed "s/|//" | sed "s/|//4" | sed "s/|/\!/g" >> c:\iidbdb2.out

del c:\iidbdb.out

echo ingres!0!0!125457>> c:\test.out
echo pvusr1!0!0!125457>> c:\test.out
echo pvusr2!0!0!2577>> c:\test.out
echo pvusr3!0!0!2577>> c:\test.out
echo pvusr4!0!0!2577>> c:\test.out
echo qatest!0!0!125457>> c:\test.out
echo testenv!0!0!125457>> c:\test.out

call ipset count expr 0
:LOOP4
call ipset count expr %count% + 1
call ipset line PCread c:\test.out %count%
call ipset line2 PCread c:\iidbdb2.out %count%
if "%line%"=="" goto CONTIN13
  if "%line%"=="%line2%" goto LOOP4
  echo %line% | sed "s/\!.*$//" > c:\user_is.tmp
  call ipset user_is PCread c:\user_is.tmp 1
  del c:\user_is.tmp
  %Eecho% INGRES user %user_is% is missing or has wrong privileges
  del c:\iidbdb2.out
  del c:\test.out
  call ipset exitcode expr 12
  %Cexit%

:CONTIN13
del c:\test.out
del c:\iidbdb2.out

REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
REM Looks like the tests may run okay....
REM++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

if "%Cexit%"=="exit" %Vecho% No problems were found in the test environment.
if not "%Cexit%"=="exit" %Vecho% Finished checking test environment.

call ipset exitcode expr 0
 
:DONE
if exist %tmpdir%\cksepenv.tmp del %tmpdir%\cksepenv.tmp
echo exit status %exitcode%
endlocal
