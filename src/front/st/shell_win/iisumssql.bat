@echo off
REM
REM  Copyright (c) 2006 Ingres Corporation.  All Rights Reserved.
REM 
REM  iisumssql - setup script for the Microsoft SQL Server Gateway.
REM
REM  History:
REM
REM	01-apr-98 (mcgem01)
REM	    Created - based on iisubr.bat
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up Microsoft SQL Server Gateway...
set CONFIG_HOST=%COMPUTERNAME%

set HOSTNAME=%CONFIG_HOST%

echo.
echo WD Generating default Microsoft SQL Server Gateway configuration...
REM Must cd to iiresutl so that Windows 95 setup can find it!
cd %II_SYSTEM%\ingres\utility
%II_SYSTEM%\ingres\utility\iigenres %CONFIG_HOST% %II_SYSTEM%\ingres\files\mssql.rfm 

echo.
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.config.mssql.%RELEASE_ID% complete

echo.
echo WD Microsoft SQL Server Gateway successfully set up...
echo.
REM echo You can now use the "ingstart" command to start your Microsoft 
REM echo SQL Server Gateway.  Refer to the Gateway Documentation for more 
REM echo information about starting and using the Microsoft SQL Server 
REM echo Gateway.
goto EXIT

:EXIT
echo.
