@echo off
if "%INSTALL_DEBUG%" == "DEBUG" echo on
REM
@REM  Copyright (c) 1995, 1997 Ingres Corporation
REM
REM  Name: iisuabf -- setup script for Ingres Vision component.
REM
REM  Usage:
REM	iisuabf
REM
REM  Description:
REM	This script should only be called by the Ingres installation
REM	utility.  It sets up the Ingres Vision component.
REM
REM  Exit Status:
REM	0	setup procedure completed.
REM	1	setup procedure did not complete.
REM
REM Note:
REM	All lines beginning with uppercase REM will be stripped from
REM	the distributed version of this file.
REM
REM History:
REM	25-sep-1995 (reijo01)
REM	    Delete abflnk.opt.old if it exists.
REM	17-nov-95 (tutt01)
REM	    Cleaned up script output.
REM	04-dec-95 (tutto01)
REM	    Added path to ingres library in .opt files.
REM	31-jan-96 (tutto01)
REM	    Added full path to executables for Windows 95 install, which
REM	    did not "see" modified path setting.
REM	01-apr-96 (tutto01)
REM	    Added missing path to ingprenv.
REM	07-may-96 (emmag)
REM	    Remove abf.bat, isql.bat iquel.bat and vision.bat if they
REM	    exist, as these have been replaced with executables.
REM	15-nov-1997 (canor01)
REM         Quote pathnames for support of embedded spaces.
REM	20-apr-98 (mcgem01)
REM	    Product name change to Ingres.
REM

echo WH Setting up CA-Vision...
echo WD Creating files and directories...

set II_CONFIG=%II_SYSTEM%\ingres\files
"%II_SYSTEM%\ingres\bin\ingsetenv" II_CONFIG "%II_CONFIG%"

if not exist "%II_SYSTEM%\ingres\bin\abf.bat" goto CLEANTM
del /F "%II_SYSTEM%\ingres\bin\abf.bat" > NUL
del /F "%II_SYSTEM%\ingres\bin\vision.bat" > NUL

:CLEANTM
if not exist "%II_SYSTEM%\ingres\bin\isql.bat" goto CLEANED
del /F "%II_SYSTEM%\ingres\bin\isql.bat" > NUL
del /F "%II_SYSTEM%\ingres\bin\iquel.bat" > NUL

:CLEANED
if exist "%II_SYSTEM%\ingres\bin\iiabf.exe" goto LABEL2
echo.
echo ERROR: Attempted to set up ABF when ABF is not installed!
echo        Attempted to set up ABF but abf executable is missing.
goto BADEXIT

:LABEL2
call "%II_SYSTEM%\ingres\bin\iitouch"
call "%II_SYSTEM%\ingres\bin\ipset" ING_ABFDIR "%II_SYSTEM%\ingres\bin\ingprenv" ING_ABFDIR

if not "%ING_ABFDIR%"=="" goto CR8ABF
"%II_SYSTEM%\ingres\bin\ingsetenv" ING_ABFDIR "%II_SYSTEM%\ingres\abf"
set ING_ABFDIR=%II_SYSTEM%\ingres\abf

:CR8ABF
if exist %ING_ABFDIR% goto CONLOC
REM echo.
REM echo Creating ABF Directory...
mkdir "%ING_ABFDIR%" >NUL

REM echo.
REM echo The following directory will be the location used to store 
REM echo Application-By-Forms, Vision and VisionPro applications:
REM echo.
REM echo 	%ING_ABFDIR%
REM echo.
REM echo This default location can be changed later by redefining the 
REM echo Ingres variable ING_ABFDIR, using the command (without quotes):
REM echo.
REM echo 	"ingsetenv ING_ABFDIR <location>"
REM echo.
REM echo This variable can also be defined in the NT environment.
REM echo.



:CONLOC
goto SKIPLOC
echo.
echo                              ***ERROR***                               
echo 									
echo  Unable to configure the default storage location for Application-By-	
echo  Forms, Vision, and VisionPro application files.  Please attempt to	
echo  resolve the problem and re-execute this setup program.		
echo.                                                                         
echo. 
echo 

:SKIPLOC

cd "%II_SYSTEM%\ingres\files"
 
if not exist abf.opt goto NOABFOPT
echo.  
echo                             *** NOTICE ***			     
echo. 									     
echo  This release uses two separate files, abflnk.opt and abfdyn.opt, to  
echo  list linker libraries and options for ABF Image and Go operations    
echo  respectively.  These two files replace the abf.opt file.	     
echo. 									     
echo  Any customizations made to abf.opt will need to be manually added    
echo  to abflnk.opt and abfdyn.opt if desired.			     
echo.
:NOABFOPT

if not exist abflnk.opt goto NOABFLNK
REM echo. 
REM echo                             *** NOTICE ***			     
REM echo. 									     
REM echo  A version of abflnk.opt already exists, but needs to be upgraded.  A 
REM echo  copy of the existing file will be saved in abflnk.opt.old and a new  
REM echo  version will be created.  Any customizations to the original version
REM echo  will need to be manually added to the new file.			     
REM echo. 									     
if exist abflnk.opt.old del abflnk.opt.old
rename abflnk.opt abflnk.opt.old >NUL

:NOABFLNK
copy abflnk.new abflnk.opt >NUL
echo %II_SYSTEM%\ingres\lib\ingres.lib >> abflnk.opt


REM Now take care of abfdyn.opt


if not exist abfdyn.opt goto NOABFDYN
REM echo. 
REM echo                             *** NOTICE ***			     
REM echo. 									     
REM echo  A version of abfdyn.opt already exists, but needs to be upgraded.  A 
REM echo  copy of the existing file will be saved in abfdyn.opt.old and a new  
REM echo  version will be created.  Any customizations to the original version
REM echo  will need to be manually added to the new file.			     
REM echo. 									     
if exist abfdyn.opt.old del abfdyn.opt.old
rename abfdyn.opt abfdyn.opt.old >NUL

:NOABFDYN
REM Just like UNIX does...
REM
copy abflnk.opt abfdyn.opt >NUL

goto GOODEXIT


:BADEXIT 
echo.
echo Application-By-Forms setup is NOT complete.
goto QUIT


:GOODEXIT
echo.
echo WD CA-Vision setup complete...


:QUIT
echo.


:EXIT
