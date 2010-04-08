@echo off
REM
REM  Copyright (c) 1995 Ingres Corporation
REM
REM  Name:
REM 	iisuc2 -- set up C2 security auditing package 
REM 
REM  Usage:
REM	iisuc2
REM
REM  Description:
REM	This script should only be called by the Ingres installation
REM	utility.  It sets up the C2 auditing option.
REM
REM  Exit Status:
REM	0	setup procedure completed.
REM	1	setup procedure did not complete.
REM
REM  History:
REM	11-dec-95 (tutto01)
REM	    Created.	
REM     08-nov-1996 (canor01)
REM         Update older values for audit mechanism, ca_high, ca_low.
REM     27-nov-1996 (canor01)
REM         Restore the "@echo off" command at beginning of file.
REM     08-apr-97 (mcgem01)
REM         Changed CA-OpenIngres to OpenIngres.
REM	24-nov-1997 (canor01)
REM	    Quote pathnames to support embedded spaces.
REM	10-apr-98 (mcgem01)
REM	    Product name change from OpenIngres to Ingres.

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo.
echo WD Setting up Ingres security...

set CONFIG_HOST=%COMPUTERNAME%
set II_CONFIG=%II_SYSTEM%\ingres\files

call ipset SERVER_HOST iigetres ii."*".config.server_host

call ipset RELEASE_ID ingprenv RELEASE_ID

call ipset SETUP iigetres ii.%CONFIG_HOST%.config.c2.%RELEASE_ID%
if not "%SETUP%" == "complete" goto SETC2

echo.
echo WD Security already configured...
goto CLEANEXIT

:SETC2

rem check for older values
call ipset CA_HIGH iigetres ii.%CONFIG_HOST%.secure.ca_high
call ipset INGRES_HIGH iigetres ii.%CONFIG_HOST%.secure.ingres_high
call ipset CA_LOW iigetres ii.%CONFIG_HOST%.secure.ca_low
call ipset INGRES_LOW iigetres ii.%CONFIG_HOST%.secure.ingres_low
call ipset AUDIT_MECH iigetres ii."*".c2.audit_mechanism

if "%AUDIT_MECH%" == "INGRES" iisetres ii."*".c2.audit_mechanism CA

if not "%CA_LOW%" == "" goto SET0
if "%INGRES_LOW%" == "" goto SET0
    iisetres ii.%CONFIG_HOST%.secure.ca_low "%INGRES_LOW%"
    iiremres ii.%CONFIG_HOST%.secure.ingres_low

:SET0
if not "%CA_HIGH%" == "" goto GENC2
if "%INGRES_HIGH%" == "" goto GENC2
    iisetres ii.%CONFIG_HOST%.secure.ca_high "%INGRES_HIGH%"
    iiremres ii.%CONFIG_HOST%.secure.ingres_high

:GENC2

echo.
echo WD Generating default security configuration...

iigenres %CONFIG_HOST% "%II_SYSTEM%\ingres\files\secure.rfm"
if not errorlevel 0 goto BADDEF

echo.
echo WD Ingres security setup complete...
iisetres ii.%CONFIG_HOST%.config.c2.%RELEASE_ID% complete

goto EXIT



:BADDEF
echo.
echo WD An error occurred while generating the default configuration...

:CLEANEXIT
:EXIT
echo.
