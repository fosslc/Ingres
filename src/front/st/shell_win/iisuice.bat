@echo off
REM
REM
@REM  Copyright (c) 1996, 1997 Ingres Corporation
REM
REM  Name: iisuice -- setup script for Ingres Internet Communication
REM
REM  Usage:
REM	iisuice
REM
REM  Description:
REM	This script should only be called by the Ingres installation
REM	utility.  It sets up the default parameters that Ingres/ICE will
REM	use to connect to an Ingres database.
REM
REM  Note:
REM	All lines beginning with uppercase REM will be stripped from
REM	the distributed version of this file.
REM
REM
REM  History:
REM
REM	28-feb-96 (harpa06)
REM	    Created.
REM	08-may-96 (harpa06)
REM	    Modified setup so it runs like the other setup scripts.
REM	08-apr-97 (mcgem01)
REM	    Change product name from CA-OpenIngres to OpenIngres.
REM	04-aug-97 (mcgem01)
REM	    Change CA-openIngres to OpenIngres.
REM	21-nov-1997 (canor01)
REM	    Quote pathnames for support of embedded spaces.
REM	10-apr-98 (mcgem01)
REM	    Product name change from OpenIngres to Ingres.
REM     08-May-98 (fanra01)
REM         Modified to include full paths to executables
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WD Setting up Ingres/ICE...
set CONFIG_HOST=%COMPUTERNAME%


set HOSTNAME=%CONFIG_HOST%
set ING_USER=INGRES

call "%II_SYSTEM%\ingres\bin\ipset" SERVER_HOST "%II_SYSTEM%\ingres\utility\iigetres" ii."*".config.server_host

call "%II_SYSTEM%\ingres\bin\ipset" RELEASE_ID "%II_SYSTEM%\ingres\bin\ingprenv" RELEASE_ID

call "%II_SYSTEM%\ingres\bin\ipset" SETUP "%II_SYSTEM%\ingres\utility\iigetres" ii.%CONFIG_HOST%.config.ice.%RELEASE_ID%
if not "%SETUP%" == "complete" goto SETICE

echo.
echo WD Ingres/ICE has already been set up...
goto CLEANEXIT

:SETICE

echo.
echo WD Generating default internet communication configuration...

"%II_SYSTEM%\ingres\utility\iigenres" %CONFIG_HOST% "%II_SYSTEM%\ingres\files\ice.rfm"
if not errorlevel 0 goto BADDEF

echo.
echo WD Ingres/ICE setup complete...
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.config.ice.%RELEASE_ID% complete

goto EXIT



:BADDEF
echo.
echo WD An error occurred while generating the default configuration...

:CLEANEXIT
:EXIT
echo.
echo.
