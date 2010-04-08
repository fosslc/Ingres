@echo off
REM 
REM  ccpp.bat
REM 
REM  History:
REM 	13-jul-95 (tutto01)
REM	    Created.
REM	23-jul-95 (tutto01)
REM	    Turned off echo.
REM	13-dec-1999 (somsa01)
REM	    Added option to print out a specific symbol (-s).
REM	23-Aug-2004 (kodse01_
REM	    Used sed "/^$/d" instead of grep -v "^$". Grep when used with
REM	    "$" behaves differently in Cygwin compared to MKS Toolkit. 
REM	18-Apr-2005 (drivi01)
REM	    SIR: 114339
REM	    Windows crsfiles are merged with generic. Added -DNT_GENERIC
REM	    define to compiler to be able to process #ifdef NT_GENERIC 
REM	    directives in crsfiles.
REM	08-May-2006 (fanra01)
REM	    Build
REM	    Add script to remove empty lines.  When using cygwin the expression to
REM	    remove empty lines only seems to work with Unix file formats.
REM	21-Nov-2007 (kiria01) b118421
REM	    Cater for passthrough of persistant comments and preprocessor lines
REM	    escaped with %
REM	11-Mar-2009 (smeke01) b120071
REM	    If using Cygwin, output from sed needs to be explicitly made 
REM	    DOS format. 

if not "%1"=="" goto CONT1
echo.
echo Usage:  ccpp filename
echo         ccpp -s [symbol name]
echo.
goto EXIT


:CONT1
REM Get the version string
REM
call readvers

if not "%1"=="-s" goto CONT2
if not "%2"=="" goto SYMB_GET
echo.
echo Usage:  ccpp filename
echo         ccpp -s [symbol name]
echo.
goto EXIT

:CONT2
set FILENAME=%1

REM The following line commented out because our bzarch has  stuff in it
REM that is way more than the original ccpp expected.  It looks like some
REM includes were thrown in there that should hopefully be taken out.
REM
REM set BZARCH=%ING_SRC%\cl\hdr\hdr\bzarch.h

REM The following lines will build an intermediate file, and precompile it.
REM The "grep" merely removes empty lines.  The REM'd out version of the cat
REM command reflects the fact that bzarch is "not right" on Windows NT.
REM
REM cat %BZARCH% %ING_SRC%\tools\port\conf\CONFIG %FILENAME% > %FILENAME%.tmp
    cat          %ING_SRC%\tools\port\conf\CONFIG %FILENAME% > %FILENAME%.tmp
REM call cl %FILENAME%.tmp /EP /DNT_GENERIC /DVERS=%CONFIG% /D%CONFIG% | grep -v "^$"
if "%USE_CYGWIN%"=="TRUE" goto CYGWIN
call cl %FILENAME%.tmp /EP /DNT_GENERIC /DVERS=%CONFIG% /D%CONFIG% | sed -e "/^[	 ]*$/d" -e "s:/%%:/*:g" -e "s:%%/:*/:g" -e "s:^%%::" 
goto EXIT

:CYGWIN
call cl %FILENAME%.tmp /EP /DNT_GENERIC /DVERS=%CONFIG% /D%CONFIG% | sed -e "/^[	 ]*$/d" -e "s:/%%:/*:g" -e "s:%%/:*/:g" -e "s:^%%::" -e "s/$/\r/"
goto EXIT

:SYMB_GET
echo %2| cat %ING_SRC%\tools\port\conf\CONFIG - > %2.tmp
call cl %2.tmp /EP /nologo /DNT_GENERIC /DVERS=%CONFIG% /D%CONFIG% > %2.tmp2 2>&1
REM grep -v "^$" %2.tmp2 | grep -v %2
sed -e "s/\r//g" -e "/^$/d" %2.tmp2 | grep -v %2
rm %2.tmp %2.tmp2

:EXIT
