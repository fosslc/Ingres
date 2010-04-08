@echo off
REM 
REM iisustar.bat - setup script for star.
REM History:
REM	08-apr-97 (mcgem01)
REM		Changed product name to OpenIngres.
REM	10-apr-98 (mcgem01)
REM		Product name change from OpenIngres to Ingres.
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

set HOSTNAME=%COMPUTERNAME%

echo WH Setting up Ingres Star Distributed DBMS...
echo.

set II_CONFIG=%II_SYSTEM%\ingres\files
ingsetenv II_CONFIG %II_CONFIG%

call ipset RELEASE_ID ingprenv RELEASE_ID

set CONFIGHOST=iipmhost
call ipset CONFIG_HOST %CONFIGHOST%
set CONFIGHOST=

call ipset SETUP iigetres ii.%CONFIG_HOST%.config.star.%RELEASE_ID%
if not "%SETUP%"=="complete" goto CHECK_DBMS
echo.
echo The %RELEASE_ID% version of Ingres Star Distributed DBMS has 
echo already been set up on "%HOSTNAME%".
goto GRACEFUL_EXIT


:CHECK_DBMS
call ipset DBMS_SETUP iigetres ii.%CONFIG_HOST%.config.dbms.%RELEASE_ID%
if not "%DBMS_SETUP%"=="complete" goto NOT_COMPLETE

call ipset NET_SETUP iigetres ii.%CONFIG_HOST%.config.net.%RELEASE_ID%

if not "%NET_SETUP%"=="complete" goto NOT_COMPLETE
goto SET_UP_STAR

:NOT_COMPLETE
echo.
echo The setup procedures for the following version of Ingres
echo Intelligent DBMS and Ingres Networking:
echo.
echo     %RELEASE_ID%
echo.
echo must be completed before Ingres Star Distributed DBMS can be
echo set up on host:
echo.
echo     %HOSTNAME%
goto GRACEFUL_EXIT


:SET_UP_STAR
set II_INSTALLATION=%II_INSTALLATION%

echo WD Generating default Star configuration...
iiremres ii.%CONFIG_HOST%.syscheck.star_swap
iigenres %CONFIG_HOST%
if ERRORLEVEL 0 goto SET_RELEASE_ID
echo.
echo An error has occurred while generating the default configuration.
goto GRACEFUL_EXIT


:SET_RELEASE_ID
iisetres ii.%CONFIG_HOST%.config.star.%RELEASE_ID% complete
echo.
echo WD Ingres Star setup completed...


:GRACEFUL_EXIT
echo.
