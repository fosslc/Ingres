@echo off
echo.
echo --------------------------------------------------------------------
echo.
echo Starting the auxiliary install script at:
PCdate
echo.
call ipset COMPUTERNAME iigetres "ii.*.config.server_host"
echo Writing dual_log_name resource to config.dat
echo.
call iisetres "ii.%COMPUTERNAME%.rcp.log.dual_log_name" "DUAL_LOG"
echo Auxiliary install script completed sucessfully.
echo.
ingsetenv II_INSTAUX_EXCODE 0
echo Ending at:
PCdate
