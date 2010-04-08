@echo off
REM  Online Checkpoint Test
REM
REM  This is called by SEP to test online checkpoint.
REM
REM  Outline:
REM
REM	  - Reload data in the database
REM	  - Turn off journaling for database.
REM	  - Start test programs as subprocesses.
REM	  - Start Checkpoint process.
REM	  - Wait for subprocesses to complete.
REM	  - Check test results for correctness.
REM	  - Roll database back to checkpoint.
REM	  - Check data for consistency.
REM
REM  Usage:
REM	
REM	ckpdbtst dbname journaled id
REM    or
REM	ckpdbtst dbname notjournaled id
REM
REM       where:
REM		- dbname is the name of a database
REM
REM		- "journaled" or "notjournaled" is a required keyword
REM
REM		- id is a unique character or short string that will 
REM		  be used to create distinct filenames
REM
REM  History:
REM
REM	21-sep-1995 (somsa01)
REM		Created from ckpdbtst.sh .
REM	01-nov-1999 (mcgem01)
REM		Add CONTINUE label which is referenced a couple of times
REM		but didn't exist.
REM
REM Get commandline args 
REM

setlocal
set dbname=%1

if not "%2"=="journaled" goto ELSE
set journaled=TRUE
goto CONTINUE
:ELSE
if not "%2"=="notjournaled" goto EXIT
set journaled=FALSE
goto CONTINUE
:EXIT
echo Second argument must be "journaled" or "notjournaled"
goto DONE

:CONTINUE
REM $df is used to build distinct filenames, so this script can be run
REM by more than one concurrent process. Each SEP script that calls the
REM "ckpdbtst" command should pass a unique string as the 3rd arg.

set df=%3

REM
REM Set up run variables
REM
call ipset abort_loops expr 25
call ipset delapp_loops expr 600
call ipset append_loops expr 300
set test_success=TRUE
ingprenv II_CHECKPOINT > tempfile
call ipset ckploc sed -e "s#\\#\\\\#g" outfile
del tempfile
set cpdbmask=%ING_TST%\\be\\lar\\data\\ckpdbmasks

REM
REM Number of seconds to delay while doing checkpoint.  This stalls the
REM checkpoint to allow the system to do work which will need to be copied
REM to the DUMP directory.
REM THIS IS NOT CURRENTLY USED BY CHECKPOINT - when the stall flag is passed
REM to checkpoint it always stalls for 2 minutes, this cannot currently be
REM changed other than by recompiling the checkpoint code.
REM
    call ipset ckpdb_stall expr 120

REM
REM Turn off Journaling
REM 
if not "%journaled%"=="FALSE" goto CONTINUE2
    echo Turning off journaling on %dbname%
    ckpdb -l -j %dbname% | sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%

:CONTINUE2
REM
REM Reload database
REM 
    echo Reloading %dbname% database
    ckpdb1_load %dbname%
REM
REM Turn on journaling 
REM
if not "%journaled%"=="TRUE" goto CONTINUE3
    echo Turning on journaling on %dbname%
    ckpdb +j %dbname% |sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%

:CONTINUE3
REM
REM Start Tests in background
REM 
    echo Starting Stress Tests
    start /b lck1.bat %dbname% %abort_loops% %cpdbmask% %df%
    start /b lck2.bat %dbname% %append_loops% %cpdbmask% %df%
    start /b lck3.bat %dbname% %delapp_loops% %cpdbmask% %df%
REM 
REM Wait for tests to get going - instead of building in handshaking between
REM driver and test scripts, we just wait for a few seconds.
REM 
    echo Waiting to start Checkpoint
    sleep 30
REM 
REM Start Checkpoint
REM Pass checkpoint flag to slow down the checkpoint to get better testing.
REM 
if not "%journaled%"=="TRUE" goto ELSE2
    echo Starting Online Checkpoint of %dbname%
    ckpdb -d '#w' %dbname% |sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%
    goto CONTINUE4
:ELSE2
    echo Starting Online Checkpoint of %dbname%
    ckpdb '#w' %dbname% |sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%

:CONTINUE4
REM 
REM Wait for subprocesses to complete
REM 
    echo "Waiting for Stress Tests to complete"
:IFEXIST
    if not exist CKPDB%df%_LCK1 goto OR1
      sleep 15
      goto IFEXIST
:OR1
    if not exist CKPDB%df%_LCK2 goto OR2
      sleep 15
      goto IFEXIST
:OR2
    if not exist CKPDB%df%_LCK3 goto NOTEXIST
      sleep 15
      goto IFEXIST
:NOTEXIST
REM 
REM Copy result tables out.
REM 
    echo Checking results of stress tests
    ingres -s %dbname% <ckp%df%qry1 |sed -f %cpdbmask% >%dbname%.out1
    ingres -s %dbname% <ckp%df%qry2 |sed -f %cpdbmask% >%dbname%.out2
    ingres -s %dbname% <ckp%df%qry3 |sed -f %cpdbmask% >%dbname%.out3
    ingres -s %dbname% <ckp%df%qry4 |sed -f %cpdbmask% >%dbname%.out4
    ingres -s %dbname% <ckp%df%qry5 |sed -f %cpdbmask% >%dbname%.out5
    ingres -s %dbname% <ckp%df%qry6 |sed -f %cpdbmask% >%dbname%.out6

if not "%journaled%"=="TRUE" goto ELSE3
REM
REM Roll back database to consistency point and check data for consistency.
REM
    echo Restoring database %dbname% to checkpoint
    rollforwarddb -j %dbname% |sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%
REM
    echo Checking results of checkpoint
    ckpdb1_check %dbname%
    if errorlevel 1 goto FAIL
        echo.
        echo Online Checkpoint Test - Phase 2 Part 1 Successful
        echo.
	goto CONTINUE5
:FAIL
        echo.
        echo Online Checkpoint Test Check Failure
        echo Phase2 Part 1, rollforward journaled DB back to CP
        echo Online Checkpoint Test - Phase 2 Part 1 Failed
        echo. 
	goto CONTINUE5

:ELSE3
else
REM
REM Roll back database to consistency point and check data.
REM Try both with '-j' flag and without.  Both should actually be the
REM same since the db is not journaled.
REM
    echo Restoring database %dbname% to checkpoint
    rollforwarddb -j %dbname%    |sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%
    rollforwarddb -c +j %dbname% |sed -e "s#%ckploc%#CKPDBLOC#g" -f $cpdbmask%
    rollforwarddb %dbname%       |sed -e "s#%ckploc%#CKPDBLOC#g" -f $cpdbmask%
REM
    echo Checking results of checkpoint
    ckpdb1_check %dbname%
    if errorlevel 1 goto FAIL2
        echo "Online Checkpoint Test - Phase 1 Successfull"
	goto CONTINUE5
:FAIL2
        echo.
        echo Online Checkpoint Test Check Failure
        echo Phase1, rollforward non-journaled DB
        echo Online Checkpoint Test - Phase 1 Failed
        echo. 
	goto CONTINUE5

:CONTINUE5
if not "%journaled%"=="TRUE" goto DONE
REM
REM Roll forward and check again that the data is correct.
REM
    echo Restoring database $dbname using journals
    rollforwarddb %dbname% |sed -e "s#%ckploc%#CKPDBLOC#g" -f %cpdbmask%
REM
REM Copy result tables out.
REM
    echo Checking results of rollforwarddb
    ingres -s %dbname% <ckp%df%qry1 |sed -f %cpdbmask% >%dbname%.out7
    ingres -s %dbname% <ckp%df%qry2 |sed -f %cpdbmask% >%dbname%.out8
    ingres -s %dbname% <ckp%df%qry3 |sed -f %cpdbmask% >%dbname%.out9
    ingres -s %dbname% <ckp%df%qry4 |sed -f %cpdbmask% >%dbname%.out10
    ingres -s %dbname% <ckp%df%qry5 |sed -f %cpdbmask% >%dbname%.out11
    ingres -s %dbname% <ckp%df%qry6 |sed -f %cpdbmask% >%dbname%.out12
REM
REM Clean up space used up by test.
REM
REM    ckpdb -l -d %dbname%

:DONE
endlocal
