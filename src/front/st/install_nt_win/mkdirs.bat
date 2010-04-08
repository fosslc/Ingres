@echo off
REM 
REM mkdirs.bat - makes all of the directories necessary for compressing
REM 		  ingres files, and generating a "cd", as well as some
REM		  other stuff.
REM 
REM History:
REM 
REM 	10-aug-95 (tutto01)
REM 	    Created.
REM 
REM	01-Mar-96 (wooke01)
REM	    Added missing directories.
REM	27-Oct-2004 (drivi01)
REM	    bat directories on windows were replaced with shell_win,
REM	    this script was updated to reflect that.

if "%ING_SRC%"=="" goto NOINGSRC
if "%INST_DRV%"=="" goto NOINSTDRV

if not EXIST %ING_SRC%\bin\rs.exe goto NORS
call rs
:NORS

echo.
echo.
echo.
echo.
echo.
echo.
echo _________________________________________________________________________
echo.
echo This program will create two directory structures needed for the install 
echo process...
echo.
echo  	%INST_DRV%\ingrescd
echo 	%INST_DRV%\ingreswk
echo.

pause

if not EXIST %INST_DRV%\ingrescd goto MKINGRESCD
echo.
echo %INST_DRV%\ingrescd already exists.  Skipping...
echo.
goto SKIPINGRESCD

:MKINGRESCD
echo on
mkdir %INST_DRV%\ingrescd
mkdir %INST_DRV%\ingrescd\oping12
@echo off

:SKIPINGRESCD
if not EXIST %INST_DRV%\ingreswk goto MKINGRESWK
echo.
echo %INST_DRV%\ingreswk already exists.  Skipping...
goto SKIPINGRESWK

:MKINGRESWK
mkdir %INST_DRV%\ingreswk
mkdir %INST_DRV%\ingreswk\output
mkdir %INST_DRV%\ingreswk\compress
mkdir %INST_DRV%\ingreswk\compress\ingres
mkdir %INST_DRV%\ingreswk\compress\ingres\bin
mkdir %INST_DRV%\ingreswk\compress\ingres\esqldemo
mkdir %INST_DRV%\ingreswk\compress\ingres\esqldemo\esqlc
mkdir %INST_DRV%\ingreswk\compress\ingres\esqldemo\cobol
mkdir %INST_DRV%\ingreswk\compress\ingres\files
mkdir %INST_DRV%\ingreswk\compress\ingres\vdba
mkdir %INST_DRV%\ingreswk\compress\ingres\demo
mkdir %INST_DRV%\ingreswk\compress\ingres\demo\api
mkdir %INST_DRV%\ingreswk\compress\ingres\demo\api\asc
mkdir %INST_DRV%\ingreswk\compress\ingres\demo\api\syc
mkdir %INST_DRV%\ingreswk\compress\ingres\demo\esqlc
mkdir %INST_DRV%\ingreswk\compress\ingres\demo\cobol
mkdir %INST_DRV%\ingreswk\compress\ingres\demo\udadts
mkdir %INST_DRV%\ingreswk\compress\ingres\files\abfdemo
mkdir %INST_DRV%\ingreswk\compress\ingres\files\webdemo
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\arabic
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\chineses
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\chineset
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\chtbig5
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\chteuc
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\chthp
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\decmulti
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\elot437
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\greek
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\hebrew
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\hproman8
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\ibmpc437
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\ibmpc850
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\iso88591
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\iso88592
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\iso88595
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\iso88599
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\kanjieuc
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\korean
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\shiftjis
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\slav852
mkdir %INST_DRV%\ingreswk\compress\ingres\files\charsets\thai
mkdir %INST_DRV%\ingreswk\compress\ingres\files\collatio
mkdir %INST_DRV%\ingreswk\compress\ingres\files\dictfile
mkdir %INST_DRV%\ingreswk\compress\ingres\files\english
mkdir %INST_DRV%\ingreswk\compress\ingres\files\english\messages
mkdir %INST_DRV%\ingreswk\compress\ingres\files\name
mkdir %INST_DRV%\ingreswk\compress\ingres\files\tutorial
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\asia
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\astrl
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\europe
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\gmt
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\mideast
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\na
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\sa
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\seasia
mkdir %INST_DRV%\ingreswk\compress\ingres\files\zoneinfo\sp
mkdir %INST_DRV%\ingreswk\compress\ingres\lib
mkdir %INST_DRV%\ingreswk\compress\ingres\sig
mkdir %INST_DRV%\ingreswk\compress\ingres\sig\errhelp
mkdir %INST_DRV%\ingreswk\compress\ingres\sig\ima
mkdir %INST_DRV%\ingreswk\compress\ingres\sig\star
mkdir %INST_DRV%\ingreswk\compress\ingres\utility
@echo off
:SKIPINGRESWK

echo.
echo.
echo _________________________________________________________________________
echo.
echo Several files will now be copied into place for the install process...
echo ** WARNING ** : Local changes will be overwritten!
echo.

pause

echo on
grep -v ^REM %ING_SRC%\front\st\shell_win\ipprecal.bat > %INST_DRV%\ingreswk\compress\ingres\ipprecal.bat
copy %ING_SRC%\front\st\install_nt\esqlc.isc       %INST_DRV%\ingreswk\compress\ingres
copy %ING_SRC%\front\st\install_nt\install.isc     %INST_DRV%\ingreswk\compress\ingres
copy %ING_SRC%\front\st\install_nt\net.isc         %INST_DRV%\ingreswk\compress\ingres
copy %ING_SRC%\front\st\install_nt\star.isc        %INST_DRV%\ingreswk\compress\ingres
copy %ING_SRC%\front\st\install_nt\tools.isc       %INST_DRV%\ingreswk\compress\ingres
copy %ING_SRC%\front\st\install_nt\vision.isc      %INST_DRV%\ingreswk\compress\ingres
copy %ING_SRC%\front\st\install_nt\geningnt.exe    %INST_DRV%\ingreswk
copy %ING_SRC%\front\st\install_nt\creating.bat    %INST_DRV%\ingreswk
copy %ING_SRC%\front\st\install_nt\ingresnt.mas    %INST_DRV%\ingreswk
copy %ING_SRC%\front\st\shell_win\iisustrt.bat           %INST_DRV%\ingreswk
@echo off
:SKIP
echo.
echo.
echo _________________________________________________________________________
echo.
echo The install process will need various files supplied by YOU.  These
echo files will now be searched for.  If found, the files in your current
echo path will be displayed.
echo.

pause

@echo off
echo.
echo CAZIP will be used to compress files.  Searching path...
call which cazip
echo.

echo.
echo ENCODINF will be used to encode files.  Searching path...
call which encodinf
echo.

@echo off
echo.
echo _________________________________________________________________________
echo.
echo Now checking for missing files that you must provide...
echo.

pause
echo.

if EXIST %INST_DRV%\ingreswk\compress\ingres\CAINLOGS.DL_ goto NF1
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\CAINLOGS.DL_
:NF1

if EXIST %INST_DRV%\ingreswk\compress\ingres\CALM_W32.DL_ goto NF2
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\CALM_W32.DL_
:NF2

if EXIST %INST_DRV%\ingreswk\compress\ingres\CATOCFGE.EX_ goto NF3
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\CATOCFGE.EX_
:NF3

if EXIST %INST_DRV%\ingreswk\compress\ingres\CATOPATH.EX_ goto NF4
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\CATOPATH.EX_
:NF4

if EXIST %INST_DRV%\ingreswk\compress\ingres\INSTALL.EXE  goto NF5
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\INSTALL.EXE 
:NF5

if EXIST %INST_DRV%\ingreswk\compress\ingres\OPINGNT_.DAT goto NF6
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\OPINGNT_.DAT
:NF6

if EXIST %INST_DRV%\ingreswk\compress\ingres\IINSTNT.DLL  goto NF7
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\IINSTNT.DLL 
:NF7

if EXIST %INST_DRV%\ingreswk\compress\ingres\IPPRECAL.BAT goto NF8
echo Missing the following... %INST_DRV%\ingreswk\compress\ingres\IPPRECAL.BAT
:NF8

echo.

goto END

:NOINSTDRV
echo.
echo INST_DRV is not set.  This tells this program what drive to use.
goto BADEND

:NOINGSRC
echo.
echo ING_SRC is not set.  This tells this program where to copy files from.
goto BADEND

:END
@echo off
echo.
echo _________________________________________________________________________
echo.
echo ** NOTE ** : Now would be a good time to modify the first few lines of
echo ingresnt.mas to define the source and destinations of various operations.
echo.

REM choice does not exist on alpha...
REM choice /c:yn Enter editor?
REM if errorlevel 2 goto NOEDIT
REM call vi %INST_DRV%\ingreswk\ingresnt.mas
REM :NOEDIT

:BADEND
@echo on
