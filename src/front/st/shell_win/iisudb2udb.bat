@echo off
REM
REM  Copyright (c) 1998 Ingres Corporation
REM 
REM  iisudb2udb - setup script for the DB2UDB Gateway.
REM
REM  History:
REM
REM	18-dec-2001 (mofja02)
REM	    Created - based on iisuodbc.bat
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up DB2UDB Gateway...
set CONFIG_HOST=%COMPUTERNAME%

set HOSTNAME=%CONFIG_HOST%

echo.
echo WD Generating default DB2UDB Gateway configuration...
REM Must cd to iiresutl so that Windows 95 setup can find it!
cd %II_SYSTEM%\ingres\utility
%II_SYSTEM%\ingres\utility\iigenres %CONFIG_HOST% %II_SYSTEM%\ingres\files\db2udb.rfm 

echo.
echo %II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.config.db2udb.%RELEASE_ID% complete
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.config.db2udb.%RELEASE_ID% complete

echo.
echo WD DB2UDB Gateway successfully set up...
echo.
REM echo You can now use the "ingstart" command to start your DB2UDB Gateway.
REM echo Refer to the Gateway Documentation for more information about
REM echo starting and using the DB2UDB Gateway.
goto EXIT

:EXIT
echo.
