@echo off
REM
REM  Name: mkgv.bat
REM
REM  Description:
REM	This batch file creates the gv.h header file in %ING_SRC%\cl\hdr\hdr.
REM
REM  History:
REM	13-dec-1999 (somsa01)
REM	    Created.
REM	29-dec-1999 (mcgem01)
REM	    Allow for embedded spaces in paths.
REM	14-jan-2000 (somsa01)
REM	    Corrected check for other products.
REM	09-feb-2001 (somsa01)
REM	    Added changes for NT_IA64.
REM	06-jun-2001 (somsa01)
REM	    Added creation of 'gv.rc', which will contain version information
REM	    for executables.
REM	17-dec-2001 (somsa01)
REM	    Changed "Whistler" to ".NET".
REM	11-nov-2002 (somsa01)
REM	    Changed "Microsoft Windows 2000" to "Microsoft Windows 32-bit",
REM	    and "Microsoft Windows .NET Server" to "Microsoft Windows 64-bit".
REM	    Also, modified setting of RELVER3 and RELVER4 in the gv.rc file.
REM	27-jan-2003 (somsa01)
REM	    Modified setting of RELVER3 and RELVER4 in the gv.rc file so that
REM	    we have a version which looks like "26.0.020705.0" (for example).
REM	29-jan-2003 (somsa01)
REM	    After much discussion, it seems that it would not be feasible to
REM	    have a patch number in the version string. Therefore, modified
REM	    setting of RELVER3 and RELVER4 in the gv.rc file so that we have
REM	    a version which looks like "2.60.0207.05" (for example).
REM	24-mar-2003 (somsa01)
REM	    When unaliasing BUILDNUM, refer also to "inc" (the increment)
REM	    which could be set in VERS.
REM	25-mar-2003 (somsa01)
REM	    If "inc" is not set, default to 00.
REM	09-may-2003 (abbjo03)
REM	    Remove GV_STAR_VER.
REM	02-oct-2003 (somsa01)
REM	    Ported to NT_AMD64.
REM 26-Jan-2004 (fanra01)
REM     SIR 111718
REM     Add initialization of the version structure to the automatic header
REM     generation.
REM	11-Jun-2004 (somsa01)
REM	    Cleaned up code for Open Source.
REM	14-Jun-2004 (drivi01)
REM	    Modified the script to work with new Unix Tools.
REM	02-Aug-2004 (kodse01)
REM	    Added the call to rc to create gv.res.
REM	23-Aug-2004 (drivi01)
REM	    Updated gv.h destination path to the new directory
REM	    structure, cl/hdr/hdr has been replaced with cl/hdr/hdr_win
REM	    and cl/hdr/hdr_unix_win.
REM	23-Aug-2004 (kodse01)
REM	    Replaced echo \c construct with printf, so that it is consistent 
REM	    with both MKS Toolkit and Cygwin.
REM	06-oct-2004 (somsa01)
REM	    Updated to account for change in versioning:
REM	    "II 3.0.1 (int.w32/00)".
REM 	01-Jun-2005 (fanra01)
REM         SIR 114614
REM         Add definition of GV_BLDLEVEL to gv.h definitions.
REM	26-Sep-2006 (drivi01)
REM	    FileVersion and ProductVersion are generated incorrectly
REM	    in gv.rc now that we changed version to II 9.1.0 (int.w32/00).
REM	    The product version we end up with is 9.10.0.
REM	20-Dec-2006 (drivi01)
REM	    Remove "32-bit" or "64-bit" strings from "Microsoft Windows Version".
REM	23-Jun-2010 (drivi01)
REM	    Automatically update copyright year in gv.rc.template,
REM	    so the copyright year always reflects current year.
REM
REM
call readvers

if "%MKSLOC%"=="" echo Please set MKSLOC to the location of your MKS Toolkit binaries!& goto DONE

REM create the header information for gv.h
"%MKSLOC%"\echo "/*" > %ING_SRC%\cl\hdr\hdr_win\gv.h
printf "** gv.h created on " >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h 
date /t >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
printf "** at " >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h 
time /t >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
"%MKSLOC%"\echo "** by the mkgv batch file" >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
"%MKSLOC%"\echo "*/" >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
"%MKSLOC%"\echo # include ^<gvcl.h^> >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h

REM Now, define the necessary stuff in gv.h
REM if "%CPU%" == "IA64" "%MKSLOC%"\echo "# define	GV_ENV \" Microsoft Windows 64-bit\"" >> %ING_SRC%"\cl\hdr\hdr_win\gv.h& goto GOTPLAT
REM if "%CPU%" == "AMD64" "%MKSLOC%"\echo "# define	GV_ENV \" Microsoft Windows 64-bit\"" >> %ING_SRC%"\cl\hdr\hdr_win\gv.h& goto GOTPLAT
"%MKSLOC%"\echo "# define     GV_ENV \" Microsoft Windows\"" >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h

:GOTPLAT
if not "%2"=="" echo Too many arguments& rm "%ING_SRC%"\cl\hdr\hdr_win\gv.h& goto DONE
if "%1"=="" set prod=INGRES& goto CONT1
if "%1"=="INGRES" set prod=%1& goto CONT1

echo Invalid product type "%1"
rm "%ING_SRC%"\cl\hdr\hdr_win\gv.h
goto DONE

:CONT1
printf "# define     GV_VER " >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
call genrelid %prod% >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h

echo.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo /*>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo **>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo ** Value definitions for components of the version string.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo */>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
ccpp -s ING_VER | sed -e "s/.* //g" -e "s,/,.,g" | %AWK_CMD% -F. "{printf \"# define GV_MAJOR %%d\n# define GV_MINOR %%d\n# define GV_GENLEVEL %%d\n# define GV_BLDLEVEL %%d\n\", $1, $2, $3, %BUILD%}">> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
if "%DOUBLEBYTE%"=="ON" echo # define GV_BYTE_TYPE 2 >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h&goto endvers 
echo # define GV_BYTE_TYPE 1 >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
:endvers
echo %CONFIG%| %AWK_CMD% -F_ "{print \"set ihw=\" $1 \"\nset ios=\"$2}" > varset.bat
call varset.bat
printf %ihw%|od -An -t x1 | %AWK_CMD% "{printf \"# define GV_HW 0x\" $1$2$3$4}"|sed -e "s/# define GV_HW 0x$//g">> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo. >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
printf %ios%|od -An -t x1 | %AWK_CMD% "{printf \"# define GV_OS 0x\" $1$2$3$4}"|sed -e "s/# define GV_OS 0x$//g">> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo. >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
if not "%inc%"=="" echo # define GV_BLDINC %inc%>>"%ING_SRC%"\cl\hdr\hdr_win\gv.h&goto bldtype
echo # define GV_BLDINC 00>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h

:bldtype

echo.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo GLOBALREF ING_VERSION  ii_ver; >> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h
echo.>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h

echo /* End of gv.h */>> "%ING_SRC%"\cl\hdr\hdr_win\gv.h

REM
REM Now, generate a gv.rc
REM
setlocal
call readvers
REM ccpp -s ING_VER | cut -d " " -f2 | sed -e "s/\//\\\//g" | %AWK_CMD% "{print \"set RELVER1=\" $0}" > varset.bat
ccpp -s ING_VER | cut -d " " -f2 | %AWK_CMD% "{print \"set RELVER1=\" $0}" > varset.bat
ccpp -s ING_VER | cut -d "." -f3 | %AWK_CMD% "{print \"set RELVER2=\" $0}" >> varset.bat
ccpp -s ING_VER | cut -d " " -f2 | cut -d "." -f1,2 | sed -e "s/\./, /g" | %AWK_CMD% "{print \"set RELVER3=\" $0}" >> varset.bat
ccpp -s ING_VER | cut -d " " -f2 | cut -d "." -f1,2 | %AWK_CMD% "{print \"set RELVER4=\" $0}" >> varset.bat

call varset.bat
rm varset.bat
if "%inc%"=="" set inc=00
sed -e "s:RELVER1:%RELVER1%:g" -e "s:RELVER2:%RELVER2%:g" -e "s:RELVER3:%RELVER3%:g" -e "s:RELVER4:%RELVER4%:g" -e "s:BUILDNUM:%BUILD%%INC%:g" -e "s:YEAR:%date:~10, 4%:g" "%ING_SRC%"\cl\clf\gv_win\gv.rc.template > "%ING_SRC%"\cl\clf\gv_win\gv.rc
endlocal

REM
REM Now, generate gv.res
REM
rc "%ING_SRC%"\cl\clf\gv_win\gv.rc

:DONE
