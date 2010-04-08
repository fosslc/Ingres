@echo off
REM
REM  usage: ckcopyd %D %A %3 [ %4 %5 ]
REM         if %3 is BACKUP
REM               %1 is the data location to archive and
REM               %2 is the checkpoint directory to create.
REM         if %3 is RESTORE
REM               %1 is the data location to restore
REM               %2 is the checkpoint directory to restore from.
REM	    if %4 is PARTIAL
REM		  %5 is the table file to restore
REM

if not "%3" == "BACKUP" goto notbackup
if not "%4" == "PARTIAL" goto notbpartial

REM Perform a partial backup ...
mkdir %2
set CKP_SRC=%1
set CKP_DEST=%2
:loop1
xcopy %CKP_SRC%\%5 %CKP_DEST%
shift
if not "%5"=="" goto loop1
goto endcopy

REM Perform a full backup ...
:notbpartial
mkdir %2
xcopy %1 %2
goto endcopy

:notbackup
if not "%3" == "RESTORE" goto notrestore
if not "%4" == "PARTIAL" goto notrpartial

REM Perform a partial restore ...
set CKP_SRC=%2
set CKP_DEST=%1
:loop2
xcopy %CKP_SRC%\%5 %CKP_DEST%
shift
if not "%5"=="" goto loop2
goto endcopy

REM Perform a full restore ...
:notrpartial
xcopy %2 %1
goto endcopy

:notrestore
:endcopy
