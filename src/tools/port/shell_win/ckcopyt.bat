@echo off
REM
REM  usage: ckcopyt %N %D %3 [ %4 %5 ]
REM         where %N is the location number
REM               %D is the database directory
REM               %3 is the type of operation ("BACKUP" or "RESTORE")
REM               %4 is "PARTIAL" if doing table-level backup
REM               %5 is name of table file if table-level backup
REM
echo Mount tape %1 and press <return>.
pause
cd %2
REM
REM Note that the "/A" default option will append to tape so that
REM in the event we are checkpointing a multi-location database,
REM and a new tape is not inserted for each location, the
REM multi-location savesets will be appended to the same tape.
REM
if "%4"=="PARTIAL" goto partial
if "%3"=="BACKUP" ntbackup backup %2 /A /D "Ingres Checkpoint of Location %2" /T NORMAL
if "%3"=="RESTORE" ntbackup restore %2 /A /D "Ingres Checkpoint of Location %2" /T NORMAL
goto endback
:partial
if "%3"=="BACKUP" ntbackup backup %2\%5 /A /D "Ingres Checkpoint of Location %2" /T NORMAL
if "%3"=="RESTORE" ntbackup restore %2\%5 /A /D "Ingres Checkpoint of Location %2" /T NORMAL
:endback
