@echo off
REM
REM gd (go directory) - Magically go to the directory of choice!
REM
REM History:
REM     13-jun-95 (tutto01)
REM         Created.
REM     15-aug-95 (tutto01)
REM         Now works with either slash.
REM	11-oct-95 (canor01)
REM	    Standardized to work on various platforms.
REM	16-oct-95 (canor01)
REM	    Modified so that sed is only called once.  Much faster now.
REM	11-jan-2000 (mcgem01)
REM	    Add other product support.
REM	11-Jun-2004 (somsa01)
REM	    Cleaned up code for Open Source.
REM	23-Aug-2004 (kodse01)
REM	    Changed sed statements to work for both Cygwin and MKS Toolkit.
REM

if "%USE_CYGWIN%"=="TRUE" goto USE_CYGWIN
echo %1| sed -e "s:/:\\:g"  -e "s:\\:[\\]:g" -e "s:^\(.*\)$:[\\]\1$:" > %ING_SRC%\bin\patt%USER%.exp
grep -i -f %ING_SRC%\bin\patt%USER%.exp %ING_SRC%\bin\dirlist.lst > %ING_SRC%\bin\cd%USER%.bat
goto RUN_BAT

:USE_CYGWIN
echo %1| sed -e "s:/:\\:g" -e "s:\\:[\\]:g" -e "s:^\(.*\)$:/[\\]\1$/p:" > %ING_SRC%\bin\patt%USER%.exp
REM In Cygwin gd becomes case-sensitive for pathnames.
sed -n -f %ING_SRC%\bin\patt%USER%.exp %ING_SRC%\bin\dirlist.lst > %ING_SRC%\bin\cd%USER%.bat

:RUN_BAT
call %ING_SRC%\bin\cd%USER%.bat
@echo on
