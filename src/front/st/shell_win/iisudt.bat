@echo off
REM
@REM Copyright (c) 1996, 1997 Ingres Corporation
REM
REM  Name:  iisudt
REM
REM  Description:
REM	Setup for CA-OpenIngres Desktop.
REM
REM  History:
REM	26-feb-1996 (tutto01)
REM	    Added.  Will re-add call to mkopingdt when bugs are worked out.
REM	28-feb-1996 (tutto01)
REM	    Call post setup procedure to "submenu" the 95 taskbar.
REM	21-nov-1997 (canor01)
REM	    Quote pathnames for support of embedded spaces.
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up the Desktop Server ...

set CONFIG_HOST=%COMPUTERNAME%
set HOSTNAME=%CONFIG_HOST%

REM Set the default gateway startup count ...
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.ingstart."*".iigws 1

REM echo WD Initializing catalogs ...
REM call mkopingdt

REM Straighten out the taskbar
"%II_SYSTEM%\desktop\fixbar"

echo.
echo WD Desktop Server successfully set up ...
echo.
goto EXIT


:EXIT
echo.
