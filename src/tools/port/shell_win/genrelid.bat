@echo off
setlocal
REM
REM  Name: genrelid.bat
REM
REM  Description:
REM	This batch file generates the Product Release id.
REM
REM  History:
REM	13-dec-1999 (somsa01)
REM	    Created.
REM	29-dec-1999 (mcgem01)
REM	    Add support for other products
REM	24-mar-2003 (somsa01)
REM	    If "inc" is defined then make it part of the version id.
REM	    Also, if "-use_inc" is not passed, do not use "inc".
REM	09-may-2003 (abbjo03)
REM	    Remove ING_STAR_VER.
REM	11-Jun-2004 (somsa01)
REM	    Cleaned up code for Open Source.
REM	14-jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.   
REM     07-jun-2005 (clach04)
REM         Added EA_VER.

call readvers

REM Check for product
if not "%3"=="" echo Too many arguments& goto DONE
if "%1"=="" set prod=INGRES& goto CONT1
if "%1"=="INGRES" set prod=%1& goto CONT1
if "%1"=="W4GL" set prod=%1& goto CONT1
if "%1"=="EA" set prod=%1& goto CONT1

echo Invalid product type "%1"
goto DONE

:CONT1
REM Set up the suffix
if "%WORKGROUP_EDITION%"=="ON" set suffix=W

REM Output the release id
if "%prod%"=="INGRES" ccpp -s ING_VER | %AWK_CMD% "{print \"set VER=\" $0}" > varset.bat
if "%prod%"=="W4GL" ccpp -s ING_W4GL_VER | %AWK_CMD% "{print \"set VER=\" $0}" > varset.bat
if "%prod%"=="EA" ccpp -s EA_VER | awk '{print "set VER=" $0}' > varset.bat
call varset.bat
echo %CONFIG%| tr "_" "." | %AWK_CMD% "{print \"set dotvers=\" $0}" > varset.bat
call varset.bat
rm varset.bat

if "%2"=="-use_inc" echo "%VER% (%dotvers%/%BUILD%%INC%)%suffix%"& goto DONE
echo "%VER% (%dotvers%/%BUILD%)%suffix%"

:DONE
endlocal
