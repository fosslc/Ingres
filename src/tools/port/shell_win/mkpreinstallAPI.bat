@echo off
REM
REM	Name: mkpreinstallAPI.bat
REM
REM	Description:
REM	     This batch file creates preinstall directory and copies selected binaries 
REM	     there.
REM
REM	History:
REM	   12-Jan-2005 (drivi01)
REM		Created.
REM	   21-Jan-2005 (drivi01)
REM	        Removed error check from directory creation.
REM	   12-May-2006 (drivi01)
REM		Done was stripped out during integration by piccolo
REM		by mistake.
REM        03-Dec-2008 (drivi01)
REM           	Modify build script for rolling the image to sign
REM	        preinstall directory binaries as well.
REM           	Shared libraries are signed in II_SYSTEM\ingres\bin only
REM           	and this script is being modified to copy shard libraries
REM           	from II_SYSTEM\ingres\bin instead of lib.
REM	   09-Dec-2008 (drivi01)
REM		Fix iilibadf.dll source location.


echo.
echo Copying binaries....
echo.
if not exist %ING_ROOT%\preinstall mkdir %ING_ROOT%\preinstall  

cp -f  %II_SYSTEM%\ingres\bin\iilibadf.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibadfdata.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibcompat.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibcompatdata.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibembed.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibembeddata.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibq.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibqdata.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibframe.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibframedata.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibgcf.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibgcfdata.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibapi.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibcuf.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iilibutil.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\bin\iigetver.dll  %ING_ROOT%\preinstall   
if ERRORLEVEL 1 goto EXIT
cp -f  %II_SYSTEM%\ingres\files\charsets\inst1252\desc.chx  %ING_ROOT%\preinstall\inst1252.chx   
if ERRORLEVEL 1 goto EXIT
goto DONE


:EXIT
echo ERROR: There was error copying one or more binaries to preinstall directory.
endlocal

:DONE
