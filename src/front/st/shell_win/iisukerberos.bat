@echo off   
setlocal
REM
@REM  Copyright (c) 2007 Ingres Corporation
REM
REM  Name: iisukerberos -- setup script for Ingres Kerberos Driver
REM  Usage:
REM       iisukerberos
REM
REM  Description:
REM       This script sets up the Ingres Kerberos Driver.
REM
REM History:
REM      30-Jul-2007 (Ralph Loen) SIR 118898
REM          Created.
REM      12-Aug-2007 (Ralph Loen) SIR 118898
REM          Removed iigenres, since Kerberos info was moved to all.crs.
REM          Prompting for domain is no longer necessary, since the
REM          iinethost utility should enter the correct host name into
REM          config.dat.  Iisukerberos now prompts for three main
REM          configurations.
REM      15-Aug-2007 (Ralph Loen) SIR 118898
REM          Wrong label referenced for removal of client configuration.
REM          Should be RMCLIENT1, not RMCLIENT2.
REM      29-Nov-2007 (Ralph Loen) Bug 119358
REM          "Server-level" (gcf.mech.user_authentication) has been deprecated.
REM          Menu option 3 becomes "standard" Kerberos authentication, i.e.,
REM          Kerberos listed in the general mechanism list and as a remote
REM          mechanism for the Name Server.
REM      14-May-2008 (Ralph Loen) Bug 117737
REM          Added calls to iiinitres to set DBMS-type mechanism values to
REM          "none" in case of an upgrade.
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

set CONFIG_HOST=%COMPUTERNAME%
call ipset IINETHOST iinethost

echo This procedure will configure Kerberos for the Ingres installation
echo located at:
echo.
echo             %II_SYSTEM%
echo.
echo to run on local host:
echo.
echo             %IINETHOST%
echo.
:PROMPT0
set /p YorN=Do you want to continue this setup procedure? (y/n) [y]:
if "%YorN%" == "" goto STARTIT
if "%YorN%" == "y" goto STARTIT
if "%YorN%" == "Y" goto STARTIT
if "%YorN%" == "n" goto END
if "%YorN%" == "N" goto END
echo Please enter 'y' or 'n'.
pause
goto PROMPT0
REM
REM Initialize DBMS server mechanisms to "none" if not already initialized
REM in iigenres.
REM
:STARTIT
%II_SYSTEM%\ingres\utility\iiinitres -keep mechanisms %II_SYSTEM%\ingres\files\dbms.rfm >NUL
%II_SYSTEM%\ingres\utility\iiinitres -keep mechanisms %II_SYSTEM%\ingres\files\star.rfm >NUL
%II_SYSTEM%\ingres\utility\iiinitres -keep mechanisms %II_SYSTEM%\ingres\files\rms.rfm >NUL
:MAINDISP
set OPTION=0
echo.
echo Basic Kerberos Configuration Options
echo.
echo 1.  Client Kerberos configuration
echo 2.  Name Server Kerberos configuration
echo 3.  Standard Kerberos configuration (both 1 and 2)
echo 0.  Exit
echo.
set /p OPTION=select from [0 - 3] above [0]:
if "%OPTION%" == "" goto END
if "%OPTION%" == "0" goto END
if "%OPTION%" == "1" goto CLIENT
if "%OPTION%" == "2" goto NS
if "%OPTION%" == "3" goto STANDARD
echo Entry must be in range 0 to 3.
pause
goto MAINDISP
:CLIENT
echo Client configuration enables this installation to use Kerberos to authenticate
echo against a remote installation that has been configured to use Kerberos for
authentication.  This is the mimimum level of Kerberos authentication.
:PROMPT1
echo.
echo Select 'a' to add client-level Kerberos authentication,
set /p AorR='r' to remove client-level Kerberos authentication:
if "%AorR%" == "a" goto ADDCLIENT
if "%AorR%" == "A" goto ADDCLIENT
if "%AorR%" == "r" goto RMCLIENT
if "%AorR%" == "R" goto RMCLIENT
echo Entry must be 'a' or 'r'
pause
goto PROMPT1
:ADDCLIENT
iisetres ii.%CONFIG_HOST%.gcf.mechanisms "kerberos"
echo.
echo Client Kerberos configuration added.
echo.
echo You must add the "authentication_mechanism" attribute in netutil or
echo VDBA for each remote node you wish to authenticate using Kerberos.  The
echo "authentication_mechanism" attribute must be set to "kerberos".  There
echo is no need to define users or passwords for vnodes using Kerberos
echo authentication.
echo.
pause
goto MAINDISP
:RMCLIENT
iisetres ii.%CONFIG_HOST%.gcf.mechanisms ""
echo.
echo Client Kerberos configuration removed.
pause
goto MAINDISP
:NS
echo Name Server Kerberos authentication allows the local Name Server to
echo authenticate using Kerberos.
echo.
:PROMPT2
echo.
echo Select 'a' to add Kerberos authentication for the local Name Server,
set /p AorR='r' to remove Name Server Kerberos authentication:
if "%AorR%" == "a" goto ADDNS
if "%AorR%" == "A" goto ADDNS
if "%AorR%" == "r" goto RMNS
if "%AorR%" == "R" goto RMNS
echo Entry must be 'a' or 'r'
pause
goto PROMPT2
:ADDNS
iisetres ii.%CONFIG_HOST%.gcn.mechanisms "kerberos"
iisetres ii.%CONFIG_HOST%.gcn.remote_mechanism "kerberos"
echo.
echo Kerberos authentication has been added to the Name Server 
echo on %IINETHOST%.
echo.
pause
goto MAINDISP
:RMNS
iisetres ii.%CONFIG_HOST%.gcn.mechanisms ""
iisetres ii.%CONFIG_HOST%.gcn.remote_mechanism "none"
echo.
echo Kerberos authentication has been removed from the Name Server 
echo on %IINETHOST%.
pause
goto MAINDISP
:STANDARD
echo.
echo Standard Kerberos authentication specifies Kerberos as the remote
echo authentication mechanism for the Name Server, and allows clients to specify
echo Kerberos for remote servers that authenticate with Kerberos.
echo.
:PROMPT3
echo.
echo Select 'a' to add standard Kerberos authentication,
set /p AorR='r' to remove Standard Kerberos authentication:
if "%AorR%" == "a" goto ADDSTANDARD
if "%AorR%" == "A" goto ADDSTANDARD
if "%AorR%" == "r" goto RMSTANDARD
if "%AorR%" == "R" goto RMSTANDARD
echo Entry must be 'a' or 'r'
pause
goto PROMPT3
:ADDSTANDARD
iisetres ii.%CONFIG_HOST%.gcf.mechanisms "kerberos"
iisetres ii.%CONFIG_HOST%.gcn.mechanisms "kerberos"
iisetres ii.%CONFIG_HOST%.gcn.remote_mechanism "kerberos"
echo.
echo Standard Kerberos authentication has been added.
echo.
pause
goto MAINDISP
:RMSTANDARD
iisetres ii.%CONFIG_HOST%.gcf.mechanisms ""
iisetres ii.%CONFIG_HOST%.gcn.mechanisms ""
iisetres ii.%CONFIG_HOST%.gcn.remote_mechanism "none"
echo.
echo Standard Kerberos authentication has been removed.
goto MAINDISP
pause
echo.
:END
echo Kerberos configuration complete.  Be sure the GSS API library, 
echo GSSAPI32.DLL, resides in your execution path, as defined by the 
echo environment variable PATH.
echo.
echo You may use the cbf utility to fine-tune Kerberos security options.
endlocal
