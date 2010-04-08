@echo off
REM
@REM Copyright (c) 1995, 1997 Ingres Corporation
REM
REM  iisunet.bat
REM
REM  History:
REM
REM	16-nov-95 (tutto01)
REM	    Cleaned up script.  Modified to work with new install dialogs.
REM     31-jan-96 (tutto01)
REM         Added full path to executables for Windows 95 install, which
REM         did not "see" modified path setting.
REM	24-apr-96 (tutto01)
REM	    Added default configuration for the DECnet protocol.
REM	08-apr-97 (mcgem01)
REM	    Changed product name from CA-OpenIngres to OpenIngres.
REM	21-nov-1997 (canor01)
REM	    Quote pathnames for support of embedded spaces.
REM	10-apr-98 (mcgem01)
REM	    Product name change from OpenIngres to Ingres.
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up Ingres Networking...
set CONFIG_HOST=%COMPUTERNAME%


set HOSTNAME=%CONFIG_HOST%
set ING_USER=INGRES


REM Set privileges...
REM
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.privileges.user.root SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.privileges.user.%ING_USER% SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED


echo.
echo WD Generating default Net configuration...
REM Must cd to iiresutl so that Windows 95 setup can find it!
cd "%II_SYSTEM%\ingres\utility"
"%II_SYSTEM%\ingres\utility\iigenres" %CONFIG_HOST% "%II_SYSTEM%\ingres\files\net.rfm"

"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcn.local_vnode %HOSTNAME%


echo.
echo WD Configuring Net server listen addresses...

"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".wintcp.port %II_INSTALLATION%
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".lanman.port %II_INSTALLATION%
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".nvlspx.port %II_INSTALLATION%
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".decnet.port II_GCC%II_INSTALLATION%_0


"%II_SYSTEM%\ingres\bin\ingsetenv" II_GCA_PIPE_SIZE 4096
"%II_SYSTEM%\ingres\bin\ingsetenv" II_HOSTNAME %CONFIG_HOST%

"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.config.net.%RELEASE_ID% complete

echo.
echo WD Configuring networking protocols...

"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".wintcp.status %II_WINTCP%
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".lanman.status %II_LANMAN%
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".nvlspx.status %II_NVLSPX%
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.gcc."*".decnet.status %II_DECNET%


echo.
echo WD Ingres Networking successfully set up...
echo.
REM echo You can now use the "ingstart" command to start your Ingres server.
REM echo Refer to the Ingres Installation Guide for more information about
REM echo starting and using Ingres.
goto EXIT



:EXIT
echo.
