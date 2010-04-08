@echo off
REM
REM mkbzarch.bat - Windows NT version of mkbzarch.
REM 
REM History:
REM	18-oct-95 (tutto01)
REM	    Created.
REM	09-feb-2001 (somsa01)
REM	    Added changes for NT_IA64.
REM	02-oct-2003 (somsa01)
REM	    Ported to NT_AMD64.
REM 05-feb-2004 (noifr01)
REM     ensure DOUBLEBYTE is defined fir the GUI tools (if _MBCS is defined)
REM     if it is set in the environment
REM	12-Aug-2004 (kodse01)
REM	    Added absolute path for source bzarch.h file at 
REM	    %ING_SRC%\tools\port\bat
REM	25-Aug-2004 (drivi01)
REM	    Modified absolute pathes to some directories to the new
REM	    structure.  (i.e. replaced tools\port\bat with tools\port\bat_win)
REM	27-Oct-2004 (drivi01)
REM	    bat directories on windows were replaced with shell_win,
REM	    this script was updated to reflect that.
REM

echo.
echo Now running mkbzarch...

if "%CPU%" == "I386" goto BZI386
if "%CPU%" == "i386" goto BZI386
if "%CPU%" == "ALPHA" goto BZALPHA
if "%CPU%" == "IA64" goto BZI386
if "%CPU%" == "AMD64" goto BZI386

echo.
echo The environment variable "CPU" has not been properly defined.  It should
echo be either "I386" or "ALPHA".
goto END

:BZALPHA
echo Copying bzarch.h for Windows NT running on ALPHA...
sed -e s/int_wnt/axp_wnt/g %ING_SRC%\tools\port\shell_win\bzarch.h > %ING_SRC%\cl\hdr\hdr_win\bzarch.h
if not "%DOUBLEBYTE%" == "ON" goto END
goto INCDBL

:BZI386
echo Copying bzarch.h for Windows NT running on I386/IA64/AMD64...
copy %ING_SRC%\tools\port\shell_win\bzarch.h %ING_SRC%\cl\hdr\hdr_win
if not "%DOUBLEBYTE%" == "ON" goto END
goto INCDBL

:INCDBL
echo #ifdef _MBCS             >> %ING_SRC%\cl\hdr\hdr_win\bzarch.h
echo #ifndef DOUBLEBYTE >> %ING_SRC%\cl\hdr\hdr_win\bzarch.h
echo #define DOUBLEBYTE >> %ING_SRC%\cl\hdr\hdr_win\bzarch.h
echo #endif            >> %ING_SRC%\cl\hdr\hdr_win\bzarch.h
echo #endif            >> %ING_SRC%\cl\hdr\hdr_win\bzarch.h
goto END

:END
echo.
@echo on
