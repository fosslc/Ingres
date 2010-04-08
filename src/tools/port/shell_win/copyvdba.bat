@echo off
REM This copies all of the necessary executables for vdba 
REM from %II_SYSTEM%\INGRES\BIN to ING_ROOT\vdba and tar it 
REM so it can be delivered as separate file for open source.
REM
REM
REM History:
REM	05-jan-07 (karye01)
REM	    File created.
REM	18-jan-07 (karye01)
REM	    Changed tar to zip as script will be used on Windows only and MKS 7.5 and above 
REM         provides zip utility. 
REM
setlocal
REM create directory vdba under ING_ROOT to store all distributable files;
if not exist "%ING_ROOT%\vdba" mkdir "%ING_ROOT%\vdba"
if exist %ING_ROOT%\vdba\vdbafiles.tar del %ING_ROOT%\vdba\vdbafiles.tar
REM copy all vdba-related files into created dir;
@echo on
copy "%II_SYSTEM%\ingres\bin\catolbox.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\catolist.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\dataldr.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\iea.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\iea.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\iia.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\iia.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ija.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ija.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ijactrl.ocx"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ingnet.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ingnet.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ingr3cnt.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ingr3spn.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ingrswin.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ipm.ocx"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ivm.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\ivm.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\lbtreelp.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\makimadb.bat"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR 
copy "%II_SYSTEM%\ingres\bin\sqlassis.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\sqlquery.ocx"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\svriia.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\svrsqlas.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\tkanimat.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\tksplash.dll"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vcbf.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vcbf.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vcda.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vcda.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vcda.ocx"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdba.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdba.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdbamon.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdbamon.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdbasql.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdbasql.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdda.chm"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vdda.exe"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
copy "%II_SYSTEM%\ingres\bin\vsda.ocx"   	"%ING_ROOT%\vdba\"
if ERRORLEVEL 1 goto ERROR
@echo off
cd %ING_ROOT%\vdba

for /F "tokens=2,4 delims=()/\ " %%i in (%ING_BUILD%\version.rel) do set TMPVER=ingres2006-%%i-%%j-win-x86-vdba
zip -qr %ING_ROOT%\vdba\%TMPVER%.zip *
if exist %ING_ROOT%\distribution copy "%ING_ROOT%\vdba\%TMPVER%.zip" %ING_ROOT%\distribution\
if exist %ING_ROOT%\distribution del /Q "%ING_ROOT%\vdba\*"
goto END

:ERROR
echo.
echo *** Copy operation failed     ***
echo *** Please check errors above ***
goto END2

:END
echo.
echo Copy operation completed successfully.
echo.

:END2
endlocal
