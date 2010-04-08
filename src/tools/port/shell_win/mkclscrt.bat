@echo off
REM
REM mkclscrt.bat - Windows NT version of mkclscrt.
REM 
REM History:
REM	1-apr-97 (clate01)
REM	    Created.
REM

echo.
echo Now running mkclscrt...


if "%CPU%" == "I386" goto CLINTEL
if "%CPU%" == "i386" goto CLINTEL
if "%CPU%" == "I486" goto CLINTEL
if "%CPU%" == "i486" goto CLINTEL
if "%CPU%" == "ALPHA" goto CLALPHA

echo.
echo The environment variable "CPU" has not been properly defined.  It should
echo be either "I386", "I486" or "ALPHA".
goto END

:CLALPHA
echo Copying clsecret.h for Windows NT running on ALPHA...
sed -e s/int_wnt/axp_wnt/g clsecret.h > %ING_SRC%\cl\hdr\hdr\clsecret.h
goto END

:CLINTEL
echo Copying clsecret.h for Windows NT running on INTEL...
copy clsecret.h %ING_SRC%\cl\hdr\hdr
goto END

:END
echo.
@echo on
