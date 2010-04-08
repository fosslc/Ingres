@echo off
REM
@REM Copyright 1996, 1997 Ingres Corporation
REM
REM  iisuor.bat
REM
REM  History:
REM
REM	04-feb-96 (somsa01)
REM	    Created from OI Desktop Version.
REM	08-apr-97 (mcgem01)
REM	    The product name is OpenROAD.
REM	04-aug-97 (mcgem01)
REM	    Replace Ca-OpenIngres/OpenROAD with plain old CA-OpenROAD.
REM	21-nov-1997 (canor01)
REM	    Quote pathnames for support of embedded spaces.
REM

if "%INSTALL_DEBUG%" == "DEBUG" echo on

echo WH Setting up OpenROAD...

echo II_CONFIG=%II_SYSTEM%\ingres\files>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_MSGDIR=%II_SYSTEM%\ingres\files\english>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_INSTALLATION=%II_INSTALLATION%>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_GCC%II_INSTALLATION%_PROTOCOLS=netbios,wintcp>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_GCN%II_INSTALLATION%_LVL_VNODE=>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_TIMEZONE=%II_TIMEZONE_NAME%>>"%II_SYSTEM%\ingres\files\config.ing"
call "%II_SYSTEM%\ingres\bin\ipset" II_LANGUAGE "%II_SYSTEM%\ingres\bin\ingprenv" II_LANGUAGE
echo II_LANGUAGE=%II_LANGUAGE%>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_DML_DEF=SQL>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_DATE_FORMAT=US>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_MONEY_FORMAT=L:$>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_DECIMAL=.>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_MONEY_PREC=2 >>"%II_SYSTEM%\ingres\files\config.ing"
echo II_W4GLAPPS_DIR=%II_SYSTEM%\ingres\w4glapps>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_LIBU3GL=>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_FONT_FILE=%II_SYSTEM%\ingres\files\appedtt.ff>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_COLORTABLE_FILE=%II_SYSTEM%\ingres\files\apped.ctb>>"%II_SYSTEM%\ingres\files\config.ing"
echo ING_EDIT=notepad>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_SCREEN_HEIGHT_INCHES=>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_SCREEN_WIDTH_INCHES=>>"%II_SYSTEM%\ingres\files\config.ing"
echo II_W4GL_DEMO=%II_SYSTEM%\ingres\files\w4gldemo>>"%II_SYSTEM%\ingres\files\config.ing"
echo PICTURE_DIR=%II_SYSTEM%\ingres\files\w4gldemo>>"%II_SYSTEM%\ingres\files\config.ing"

if "%II_PLATFORM%"=="II_WINNT" COPY "%II_SYSTEM%\INGRES\BIN\OR_NT\GCA32_1.DLL" "%II_SYSTEM%\INGRES\BIN" >NUL
if "%II_PLATFORM%"=="II_WIN95" COPY "%II_SYSTEM%\INGRES\BIN\OR_W95\GCA32_1.DLL" "%II_SYSTEM%\INGRES\BIN" >NUL

echo.
echo WD CA-OpenROAD successfully set up...
echo.
goto EXIT


:EXIT
echo.
