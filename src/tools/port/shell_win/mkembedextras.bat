@echo off
REM
REM	Name: mkembedextras.bat
REM
REM	Description:
REM	     This batch file creates incomplete embedded extras directory.  The directory
REM	     will be missing generated pdf files since they're not submitted to repository.
REM
REM	History:
REM	     21-Mar-2005 (drivi01)
REM		Created.
REM


echo.
echo Creating embedded_extras directory.
echo.
if not exist %ING_ROOT%\embedded_extras mkdir %ING_ROOT%\embedded_extras

echo.
echo Copying header files iff.h and tngapi.h...
echo.
copy /Y %ING_SRC%\front\st\tngapi\iff.h  %ING_ROOT%\embedded_extras
if ERRORLEVEL 1 goto EXIT
copy /Y %ING_SRC%\front\st\tngapi\tngapi.h  %ING_ROOT%\embedded_extras
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying documentation available...
echo.
copy /Y %ING_SRC%\front\st\tngapi\oiutil.doc  %ING_ROOT%\embedded_extras
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying shared library...
echo.
copy /Y %II_SYSTEM%\ingres\bin\iilibutil.dll  %ING_ROOT%\embedded_extras
if ERRORLEVEL 1 goto EXIT
goto DONE

 

:EXIT
echo ERROR: There was one or more errors copying files to embedded_extras.
endlocal

:DONE
echo.
echo embedded_extras completed successfully!!!
echo.