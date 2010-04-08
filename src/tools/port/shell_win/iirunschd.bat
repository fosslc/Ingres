@echo off
REM ***************************************************************************
REM  Name:
REM 	iirunschd
REM 
REM  Usage:
REM 	iirunschd
REM 
REM  Description:
REM 	iirunschd starts up the SQL Scheduler, a distributed database 
REM 	replicator.
REM 
REM  Exit status:
REM 	0 - OK
REM 	3 - Creation of iialarmdb failed
REM 	4 - User declined creation of iialarmdb
REM 	5 - Scheduler init failed
REM 	6 - Scheduler would not run
REM 
REM # History:
REM #	23-aug-1993 (dianeh)
REM #		Created (from ingres63p!unix!admin!install!shell!iirunschd.sh);
REM #		updated for 6.5 (call to chkins deleted, echolog to iiecholg,
REM #		ask to iiask4it, pointers to where things live, etc.)
REM #
REM # History:
REM #	18-jul-1994 (reijo01)
REM #		change to batch file for NT
REM ***************************************************************************

REM . iisysdep ????

set CMD=iirunschd

REM  --------------------------------------------------------------------------
REM  Start the scheduler
REM  Check if iialarmdb exists. Prompt to create it if it does not.

call infodb -uingres iialarmdb > NUL
if ERRORLEVEL 1 goto ERROR
goto LABEL1
:ERROR
	echo "The scheduler requires the database iialarmdb to operate."
	echo "The database iialarmdb does not exist yet."
	CHOICE "Do you want to create iialarmdb?" 
	if ERRORLEVEL 2 goto ERROR2
	echo "Creating iialarmdb ..."
	call createdb iialarmdb
	if ERRORLEVEL 1 goto ERROR3
	echo "iialarmdb created successfully."

:LABEL1
REM  See if iialarmdb needs to be initialized.
REM  Assume that if i_user_alarmed_dbs exists, then it has been initialized.

call sql -s iialarmdb < iirunschd.inp | find "No views were found"  > NUL
if ERRORLEVEL 1 goto LABEL2
	echo "Initializing iialarmdb ..."
	call    scheduler init master
	if ERRORLEVEL 1 goto ERROR4

:LABEL2
REM  Now run the scheduler.
set sched=%II_SYSTEM%\ingres\sig\star\scheduler
set logf=%II_SYSTEM%\ingres\files\iischeduler%II_INSTALLATION%.log

start /b %sched% %II_INSTALLATION% > %logf%  


goto EXIT

:ERROR2
echo " ----------------------------------------------------------------------"
echo "                           ***ERROR***                                 "
echo "The scheduler cannot operate without "iialarmdb"; scheduler not started"
echo " ----------------------------------------------------------------------"
goto EXIT

:ERROR3
echo " ----------------------------------------------------------------------"
echo "                            ***ERROR***                                "
echo " iialarmdb could not be created successfully.  Please check that the   "
echo " installation is up. You may need to destroy iialarmdb before retrying."
echo " Please rerun `iirunschd' later.                                       "
echo " ----------------------------------------------------------------------"
goto EXIT


:ERROR4
echo " ----------------------------------------------------------------------"
echo "                            ***ERROR***                                "
echo "            An error occured while initializing "iialarmdb" 	     " 
echo "            Please fix the error and re-initialize "iialarmdb".        "
echo " ----------------------------------------------------------------------"
goto EXIT

:EXIT
@echo on
