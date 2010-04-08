@echo off
setlocal
REM
REM
REM  Name: iisign.bat
REM 
REM  Description:
REM	Digitally signs all binaries in the distribution list which it takes as a parameter(s).
REM     
REM  History:
REM	30-Jun-2008 (drivi01)
REM	     Created.
REM	28-Jul-2008 (drivi01)
REM	     Save accessed and modified timestamps of the file before 
REM	     it is signed. After the file is signed restore the timestamps, 
REM	     otherwise Jam dependencies break b/c all binaries end up 
REM	     timestamp with almost the same time.
REM	11-Mar-2009 (drivi01)
REM	     Updated authenticode certificate because the old one expired.
REM
REM

echo.
echo Digitally signing the binaries
echo.
if x%1 == x goto NOPARAMS
if %1 == -? goto USAGE
if %1 == /? goto USAGE
if %1 == -h goto USAGE
if %1 == -help goto USAGE
if %1 == -l goto PROCESS_LIST

:PROCESS_FILE
set TIMEFILE=%TEMP%\timestamp.$$%RANDOM%
touch -r %1 "%TIMEFILE%"
echo SignCode.exe -sha1 "63a642f197c8c316f1ece767f5b4244dbce3ba25"  -n %1 -i http://www.ingres.com/ -t http://timestamp.verisign.com/scripts/timstamp.dll %1  
SignCode.exe -sha1 "63a642f197c8c316f1ece767f5b4244dbce3ba25"  -n %1 -i http://www.ingres.com/ -t http://timestamp.verisign.com/scripts/timstamp.dll %1  
if NOT ERRORLEVEL 0 goto ERROR
touch -r "%TIMEFILE%" %1
rm "%TIMEFILE%"
goto DONE

:PROCESS_LIST
if not exist "%2" goto ERROR2
set TIMEFILE=%TEMP%\timestamp.$$%RANDOM%
for /F %%I in (%2) do (
touch -r %%I "%TIMEFILE%"
echo SignCode.exe -sha1 "63a642f197c8c316f1ece767f5b4244dbce3ba25"  -n %%I -i http://www.ingres.com/ -t http://timestamp.verisign.com/scripts/timstamp.dll %%I
SignCode.exe -sha1 "63a642f197c8c316f1ece767f5b4244dbce3ba25"  -n %%I -i http://www.ingres.com/ -t http://timestamp.verisign.com/scripts/timstamp.dll %%I    
if NOT ERRORLEVEL 0 goto ERROR
touch -r "%TIMEFILE%" %%I
)
rm "%TIMEFILE%"

REM type "%2"|awk '{print "echo SignCode.exe -sha1 \"63a642f197c8c316f1ece767f5b4244dbce3ba25\"  -n \"Ingres System File\" -i http://www.ingres.com/ -t http://timestamp.verisign.com/scripts/timstamp.dll \""$1"\"""\r\nSignCode.exe -sha1 \"63a642f197c8c316f1ece767f5b4244dbce3ba25\"  -n \"Ingres System File\" -i http://www.ingres.com/ -t http://timestamp.verisign.com/scripts/timstamp.dll \""$1"\""}'> %TEMP%\iisign$$1.bat
REM call %TEMP%\iisign$$1.bat
REM if NOT ERRORLEVEL 0 goto ERROR
REM rm %TEMP%\iisign$$1.bat
goto DONE

:USAGE
echo.
echo iisign [<file> | -l <list of files>]
echo.
goto DONE

:NOPARAMS
echo.
echo No parameters were specified, parameters didn't provide files to digitally sign!
echo.
goto DONE

:ERROR
echo.
echo iisign failed to sign one or more files, exiting...
echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED!!!
echo.
goto DONE

:ERROR2
echo.
echo iisgin was unable to find the list of files you provided...
echo WARNING!!! YOUR IMAGE IS NOT DIGITALLY SIGNED!!!
echo.
goto DONE


:DONE