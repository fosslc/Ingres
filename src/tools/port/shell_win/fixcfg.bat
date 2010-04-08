@echo off
setlocal
REM
REM  Name:
REM       fixcfg - Setup script for configuring Ingres Installations
REM                for development and runall testing.
REM  Usage:
REM       First you must choose your development environment.  Then... fixcfg
REM                  
REM  Description:
REM       This script will configure an installation with all of the necessary
REM       parameters for developing and runall testing.
REM
REM History:
REM	26-Jul-1996 (somsa01)
REM	    Created from UNIX version.
REM	03-apr-1998 (somsa01)
REM	    Added turning off the RMCMD server count.
REM	01-feb-1999 (somsa01)
REM	    Set the connect_limit to 16 rather than the default of 32.
REM	30-mar-2000 (somsa01)
REM	    Added turning off the JDBC server count.
REM

set HOST=%COMPUTERNAME%
set NECESS=FALSE

call ipset value iigetres "ii.%HOST%.dbms.*.sole_server"
echo ii.%HOST%.dbms.*.sole_server:  %value%
if "%value%"=="ON" (
	set NECESS=TRUE
	echo Setting to OFF...
	iisetres "ii.%HOST%.dbms.*.sole_server" OFF
	echo.
)

call ipset value iigetres "ii.%HOST%.rcp.lock.per_tx_limit"
echo ii.%HOST%.rcp.lock.per_tx_limit:  %value%
if not "%value%"=="500" (
	set NECESS=TRUE
	echo Setting limit to 500...
	iisetres "ii.%HOST%.rcp.lock.per_tx_limit" 500
	echo.
)

call ipset value iigetres "ii.%HOST%.rcp.lock.lock_limit"
echo ii.%HOST%.rcp.lock.lock_limit:  %value%
if not "%value%"=="36000" (
	set NECESS=TRUE
	echo Setting limit to 36000...
	iisetres "ii.%HOST%.rcp.lock.lock_limit" 36000
	echo.
)

call ipset value iigetres "ii.%HOST%.privileges.user.*"
call ipset value2 iigetres "ii.%HOST%.privileges.user.root"
echo ii.%HOST%.privileges.user.*:  %value%
if not "%value%"=="%value2%" (
	set NECESS=TRUE
	echo.
	echo ii.%HOST%.privileges.user.* being set to ...
	iigetres "ii.%HOST%.privileges.user.root"
	iisetres "ii.%HOST%.privileges.user.*" %value2%
	echo.
)

call ipset value iigetres "ii.%HOST%.dbms.nonames.name_service"
echo ii.%HOST%.dbms.nonames.name_service:  %value%
if not "%value%"=="OFF" (
	set NECESS=TRUE
	echo ii.%HOST%.dbms.nonames.name_service being set to OFF...
	iisetres "ii.%HOST%.dbms.nonames.name_service" OFF
	echo ii.%HOST%.dbms.nonames.define_address being set to OFF...
	iisetres "ii.%HOST%.dbms.nonames.define_address" OFF
	echo.
)

call ipset value iigetres "ii.%HOST%.ingstart.*.star"
echo ii.%HOST%.ingstart.*.star:  %value%
if not "%value%"=="0" (
	set NECESS=TRUE
	echo ii.%HOST%.ingstart.*.star being set to 0...
	iisetres "ii.%HOST%.ingstart.*.star" 0
	echo.
)

call ipset value iigetres "ii.%HOST%.ingstart.*.gcc"
echo ii.%HOST%.ingstart.*.gcc:  %value%
if not "%value%"=="0" (
	set NECESS=TRUE
	echo ii.%HOST%.ingstart.*.gcc being set to 0...
	iisetres "ii.%HOST%.ingstart.*.gcc" 0
	echo.
)

call ipset value iigetres "ii.%HOST%.ingstart.*.jdbc"
echo ii.%HOST%.ingstart.*.jdbc:  %value%
if not "%value%"=="0" (
	set NECESS=TRUE
	echo ii.%HOST%.ingstart.*.jdbc being set to 0...
	iisetres "ii.%HOST%.ingstart.*.jdbc" 0
	echo.
)

call ipset value iigetres "ii.%HOST%.ingstart.*.rmcmd"
echo ii.%HOST%.ingstart.*.rmcmd:  %value%
if not "%value%"=="0" (
	set NECESS=TRUE
	echo ii.%HOST%.ingstart.*.rmcmd being set to 0...
	iisetres "ii.%HOST%.ingstart.*.rmcmd" 0
	echo.
)

call ipset value iigetres "ii.%HOST%.dbms.*.connect_limit"
echo ii.%HOST%.dbms.*.connect_limit:  %value%
if not "%value%"=="16" (
	set NECESS=TRUE
	echo ii.%HOST%.dbms.*.connect_limit being set to 16...
	iisetres "ii.%HOST%.dbms.*.connect_limit" 16
	echo.
)

if "%NECESS%"=="FALSE" (
	echo.
	echo No changes were necessary!
	echo.
)

endlocal
