@echo off
REM
REM  Copyright 2000 Ingres Corporation
REM
REM  Name: iiunloadfix.bat - replace system_maintained with sys_maintained
REM
REM  Usage:
REM	iiunloadfix
REM
REM  Description:
REM	Due to making system_maintained a reserved key word, the
REM	copy.in script produced from unloaddb will fail.  This
REM	script will updated the create table ii_atttype statement
REM	to create the correct column called sys_maintained.
REM
REM  History:
REM	28-nov-2000 (somsa01)
REM	    Created from UNIX version, iiunloadfix.sh .

sif copy.in "system_maintained" "sys_maintained"

