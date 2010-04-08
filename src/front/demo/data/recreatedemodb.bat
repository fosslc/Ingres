@ECHO OFF
REM
REM   Copyright (c) 2006 Ingres Corporation
REM
REM   Name:         recreatedemodb.bat
REM
REM   Description:  script will recreate and load Ingres Demo database demodb.
REM                 To save script output use: recreatedemodb.bat > output.log
REM
REM    

IF "%II_SYSTEM%"=="" goto SetupError
echo.
echo  __%date%__%time%________________________________________________
echo.
echo   Creating Ingres Demo Database demodb ...
echo.

destroydb demodb
createdb -i demodb
IF %ERRORLEVEL% EQU 0 GOTO CREATED_OK
echo.
echo  ______________________________________________________________________________
echo.
echo   ERROR:  Creation of database demodb failed. Please check above errors.       
echo           Exiting...                                                           
echo  ______________________________________________________________________________
GOTO END

:CREATED_OK
echo.
echo   %time%   Loading demodb ...
echo.
sql demodb < copy.in
IF %ERRORLEVEL% EQU 0 GOTO LOADED_OK
echo.
echo  ______________________________________________________________________________
echo.
echo   ERROR:  Failed to load data into demodb database. Please check above errors.  
echo           Exiting ...                                                           
echo  ______________________________________________________________________________
GOTO END

:LOADED_OK
optimizedb demodb
sysmod demodb
ckpdb +w +j demodb
echo.
echo __%time%______ Database demodb is loaded successfully.________________
echo.
GOTO END


:SetupError
echo.
echo  _____________________________________________________________________________
echo.                                                                               
echo  ERROR: The environment variable, II_SYSTEM, is not set in your environment.  
echo         This may mean that Ingres has not been installed,or if                
echo         Ingres has been installed, it is not configured correctly.            
echo  _____________________________________________________________________________
echo.

:END
echo.