@echo off
REM
REM  Copyright (c) 1998 Ingres Corporation
REM 
REM  iisuodbc - setup script for the ODBC Gateway.
REM
REM  History:
REM
REM	01-apr-98 (mcgem01)
REM	    Created - based on iisubr.bat
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up ODBC Gateway...
set CONFIG_HOST=%COMPUTERNAME%

set HOSTNAME=%CONFIG_HOST%

echo.
echo WD Generating default ODBC Gateway configuration...
REM Must cd to iiresutl so that Windows 95 setup can find it!
cd %II_SYSTEM%\ingres\utility
%II_SYSTEM%\ingres\utility\iigenres %CONFIG_HOST% %II_SYSTEM%\ingres\files\odbc.rfm 

echo.
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.config.odbc.%RELEASE_ID% complete

echo.
echo WD ODBC Gateway successfully set up...
echo.
REM echo You can now use the "ingstart" command to start your ODBC Gateway.
REM echo Refer to the Gateway Documentation for more information about
REM echo starting and using the ODBC Gateway.
goto EXIT

:EXIT
echo.
