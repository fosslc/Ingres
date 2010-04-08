@echo off
REM
REM  Copyright (c) 1997 Ingres Corporation
REM 
REM  iisubr - setup script for the Ingres Protocol Bridge
REM
REM  History:
REM
REM	13-dec-97 (mcgem01)
REM	    Created - based on iisunet.bat
REM	10-apr-98 (mcgem01)
REM	    Product name change from OpenIngres to Ingres.
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up Ingres Protocol Bridge...
set CONFIG_HOST=%COMPUTERNAME%

set HOSTNAME=%CONFIG_HOST%

echo.
echo WD Generating default Net configuration...
REM Must cd to iiresutl so that Windows 95 setup can find it!
cd %II_SYSTEM%\ingres\utility
%II_SYSTEM%\ingres\utility\iigenres %CONFIG_HOST% %II_SYSTEM%\ingres\files\bridge.rfm 

echo.
echo WD Configuring Bridge server listen addresses...

%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".wintcp.port %II_INSTALLATION%
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".lanman.port %II_INSTALLATION%
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".nvlspx.port %II_INSTALLATION%
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".decnet.port II_GCC%II_INSTALLATION%_0


%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.config.bridge.%RELEASE_ID% complete

echo.
echo WD Configuring bridge protocols...

%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".wintcp.status %II_WINTCP%
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".lanman.status %II_LANMAN%
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".nvlspx.status %II_NVLSPX%
%II_SYSTEM%\ingres\utility\iisetres ii.%CONFIG_HOST%.gcb."*".decnet.status %II_DECNET%


echo.
echo WD Ingres Protocol Bridge successfully set up...
echo.
REM echo You can now use the "ingstart" command to start your Ingres server.
REM echo Refer to the Ingres Installation Guide for more information about
REM echo starting and using Ingres.
goto EXIT



:EXIT
echo.
