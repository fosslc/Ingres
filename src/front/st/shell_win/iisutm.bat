@echo off 
if "%INSTALL_DEBUG%" == "DEBUG" echo on
if not "%II_PROD_DBMS%" == "" goto SUTM
if not "%II_PROD_NET%" == "" goto SUTM
goto EXIT

REM
@REM  Copyright (c) 1995, 1997 Ingres Corporation
REM
REM  Name: iisutm -- setup script for Ingres Intelligent DBMS
REM
REM  Usage:
REM	iisutm
REM
REM  Description:
REM	This script should only be called by the Ingres installation
REM	utility.  It sets up the Ingres Intelligent Database. 
REM
REM  Exit Status:
REM	0	setup procedure completed.
REM	1	setup procedure did not complete.
REM
REM Note:
REM	All lines beginning with uppercase REM will be stripped from
REM	the distributed version of this file.
REM
REM History:
REM	25-sep-1995 (reijo01)
REM	    Added line to check for the existence of dayfile.old
REM	24-apr-1996 (tutto01)
REM	    Cosmetic change.
REM	24-nov-1997 (canor01)
REM	    Quote pathnames to support embedded spaces.
REM	20-apr-98 (mcgem01)
REM	    Product name change to Ingres.
REM


:SUTM
cd "%II_SYSTEM%\ingres\files"

echo WH Setting up the Terminal Monitor...
echo.
echo WD Creating files...

cd "%II_SYSTEM%\ingres\files"
set exitstat=0
 
 
if not exist dayfile goto COPYDAY
if exist dayfile.old del dayfile.old
rename dayfile dayfile.old

:COPYDAY
if not exist dayfile.dst goto COPYSTART
copy dayfile.dst dayfile >NUL

:COPYSTART
if exist startup goto COPYSQL
if not exist startup.dst goto COPYSQL
	copy startup.dst startup >NUL

:COPYSQL
if exist startsql goto INST_COMPL
if not exist startsql.dst goto INST_COMPL
	copy startsql.dst startsql >NUL

:INST_COMPL
echo.
echo WD Terminal Monitor setup completed...

:EXIT
echo.
