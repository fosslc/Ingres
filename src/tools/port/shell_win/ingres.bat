@echo off
REM Name: ingres.bat
REM
REM Description:
REM 	Wraper script for invoking Ingres terminal monitor
REM
REM History:
REM      26-Apr-2010 (hanje04)
REM          SIR 123608
REM          Invoke SQL session instead of QUEL
set ii_sql=%1
:loop
shift
if "%1"=="" goto end
set ii_sql=%ii_sql% %1
goto loop
:end
echo on
@tm -qSQL %ii_sql%
@set ii_sql=
