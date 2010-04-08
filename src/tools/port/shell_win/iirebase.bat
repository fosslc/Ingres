@echo off
goto iirebase
REM
REM IIREBASE.BAT
REM
REM Description:
REM	A wrapper for the rebase.exe program.  Rebase.exe will not
REM	accept pathnames with spaces or pathnames with quotes.
REM	This batch file changes to the DLL directory to do the
REM	rebase locally, then switches back.
REM
:iirebase
pushd %II_SYSTEM%\ingres\bin
rebase %1 %2 *.dll
popd
