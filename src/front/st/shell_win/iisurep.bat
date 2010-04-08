@ECHO OFF
@REM Copyright (c) 1996, 1997 Ingres Corporation
REM
REM Name:
REM     iisurep
REM
REM Usage:
REM     iisurep  (called from ingbuild, generally)
REM
REM Description:
REM     Configure the DD_RSERVERS location and set link file for REP
REM
REM Exit status:
REM     0       OK
REM     1       REP is not installed
REM
REM
REM History:
REM     10-Jun-97 (fanra01)
REM         Created.
REM	24-nov-1997 (canor01)
REM	    Quote directory names to support embedded spaces.
REM	14-dec-97 (mcgem01)
REM	    The default value for DD_RSERVERS on NT is %II_CONFIG%\rep
REM	10-apr-98 (mcgem01)
REM	    Product name change from OpenIngres to Ingres.
REM     08-May-98 (fanra01)
REM         Modified to include full paths to executables

IF "%INSTALL_DEBUG%" == "DEBUG" ECHO ON

SET REP_DEST=utility

ECHO WH Setting up the Ingres Replicator...

REM override value of II_CONFIG set by ingbuild
set II_CONFIG=%II_SYSTEM%\ingres\files

IF EXIST "%II_SYSTEM%\ingres\bin\repmgr.exe" GOTO repmgr
ECHO ERROR: Attempted to set up Replicator when repmgr is not installed
SET ERRORLEVEL=1
GOTO end

:repmgr
IF "%DD_RSERVERS%" == "" SET DD_RSERVERS=%II_SYSTEM%\ingres\files\rep
IF EXIST %DD_RSERVERS%\servers GOTO rep_ready
ECHO WD Creating RepServer Directories...
md "%DD_RSERVERS%"
md "%DD_RSERVERS%\servers"
for %%i in (1 2 3 4 5 6 7 8 9 10) DO MD "%DD_RSERVERS%\servers\server%%i"

:rep_ready

ECHO WD Configuring DBMS Replication parameters
IF "%CONFIG_HOST%" == "" GOTO no_config_host
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.dbms."*".rep_qman_threads 1
"%II_SYSTEM%\ingres\utility\iisetres" ii.%CONFIG_HOST%.dbms."*".rep_txq_size 50
GOTO complete

:no_config_host:
ECHO ERROR: Unable to configure DBMS Replication parameters CONFIG_HOST not set
ECHO WD Error detected during configuration. Please view the installation log.
SET ERRORLEVEL=1
GOTO end

:complete
ECHO WD Replicator setup complete.
SET ERRORLEVEL=0

:end
