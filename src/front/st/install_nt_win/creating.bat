@echo off
REM
REM creating.bat - "Create Ingres" batch file.  This file generates output
REM                and batch files for the creation of the Ingres CD.
REM
REM History:
REM
REM	31-oct-95 (tutto01)
REM	    Modified from original to be more user friendly.
REM	29-jun-97 (mcgem01)
REM	    Updated to move with the times.
REM

if "%INST_DRV%"=="" goto NOINSTDRV

if not EXIST %ING_SRC%\bin\rs.exe goto SKIPRS
rs
:SKIPRS

echo.
echo.
echo This program will create output and batch files for creating an
echo OpenIngres CD...
echo.
echo.
echo Please note the following settings in ingresnt.mas ...
echo.
head -21 ingresnt.mas | tail -4
echo.
echo ____________________________________________________________________


echo.
echo Erasing old files...
echo.
echo on
ERASE .\OUTPUT\COMPRESS.BAT
ERASE .\OUTPUT\DISK*.BAT
ERASE .\OUTPUT\CDROMFIN.BAT
@echo off
echo ____________________________________________________________________


echo.
echo Generating new output and batch files...
echo.
echo on 
GENINGNT > %INST_DRV%\INGRESWK\OUTPUT\OUTPUT.TXT
@echo off
echo ____________________________________________________________________


echo.
echo Changing current directory to output directory...
cd .\output
pwd
echo.
echo ____________________________________________________________________


echo.
echo The compress.bat file should be run if it contains commands...
echo.
dir compress.bat | grep -i compress.bat
echo.
echo ____________________________________________________________________


echo.
echo Summary of output...
echo.
egrep "Total # of Ingres Files|Total # of Files needing Compression|Total # of Read Errors|Total Uncompressed size|Total Compressed size|Total # of disks needed" output.txt | grep -v Prod
echo ____________________________________________________________________

echo.
echo Type "compress" to compress files (if necessary) 
echo                      or
echo Type "cdromfin" to create the OpenIngres CD.
echo.

goto END

:NOINSTDRV
The INST_DRV variable is not set.
goto END

:END
