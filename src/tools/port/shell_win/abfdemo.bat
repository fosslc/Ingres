@echo off
if not x%1==x goto doit
echo Use createdb [database-name] on your server to create a database to use
echo  for the PROJMGMT application.
echo Then run %0 [server-name]::[database-name] to load the database.
echo.
echo Example:	%0 myserver::abfdemo
goto dun
:doit


echo You must run this from the drive on which Ingres is installed.
echo That is, the same drive that is specified in the path, %II_SYSTEM%.
call ipchoice Do you want to continue? (y or n)
if errorlevel 2 goto dun
set CUR_ABFDIR=
echo set CUR_ABFDIR=> abftmp.1
if not x"%ING_ABFDIR%" == x"" goto skip1
ingprsym ING_ABFDIR > abftmp.2
goto skip1a
:skip1
echo %ING_ABFDIR%>abftmp.2
:skip1a
if exist "%II_SYSTEM%\ingres\bin\ipset.bat" goto skip1b
copy abftmp.1+abftmp.2  abftmp.bat > NUL
echo m 111 150 10F > abftmp.3
echo w>> abftmp.3
echo q>> abftmp.3
debug abftmp.bat < abftmp.3 >NUL
echo e > abftmp.3
edlin abftmp.bat < abftmp.3 > NUL
call abftmp.bat > NUL
goto skip1c
:skip1b
call ipset CUR_ABFDIR type abftmp.2
:skip1c
splitprm -n=%1 -s -e=ABFSERVERNAME > setabfsvr.bat
call setabfsvr.bat > NUL
del setabfsvr.bat  > NUL
splitprm -n=%1 -d -e=ABFDATABASENAME > setabfdb.bat
call setabfdb.bat > NUL
del setabfdb.bat  > NUL
del abftmp.* > NUL
if not "%ABFSERVERNAME%"=="" set ABFSERVERNAME=\%ABFSERVERNAME%
if not "%ING_ABFDIR%" == "" goto skip2
set ING_ABFDIR=%CUR_ABFDIR%%ABFSERVERNAME%
:skip2
set TMP_DEMODIR=%II_SYSTEM%\ingres\files\abfdemo
set ABFDEMODIR=%CUR_ABFDIR%
set DEMO_ING_ABFDIR=%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%\projmgmt

if exist "%II_SYSTEM%\ingres\bin\iiabf.exe" goto installedABF
echo Cannot locate ABF in the directory "%II_SYSTEM%\ingres\bin".
echo Please install the ABF application before running this setup.
goto dun

:installedABF

:cleanABF
if exist "%II_SYSTEM%\ingres\files\abfdemo\top.osq" goto dbABF
echo Setup has been unable to locate the demonstration source files.
echo Please install the demonstration and re-run this setup.
goto dun

:dbABF
if not exist "%DEMO_ING_ABFDIR%\ii_dbname.txt" goto sourceABF
if not exist "%DEMO_ING_ABFDIR%\ii_dbname2.txt" goto loadimageABF
echo Setup has detected a previous demonstration installation using the
echo database:
type         "%DEMO_ING_ABFDIR%\ii_dbname2.txt"
echo It must first be removed in order to rerun abfdemo.
call ipchoice Do you want the previous ABF demonstration removed and further processing to continue? (y or n)
if errorlevel 2 goto dun
echo Answer 'y' to the prompts that follow ...
if "%ABFSERVERNAME%"=="" goto deletebdemo
del "%ABFDEMODIR%\%ABFSERVERNAME%\%ABFDATABASENAME%\projmgmt\*"
del "%ABFDEMODIR%\%ABFSERVERNAME%\%ABFDATABASENAME%\*"
del "%ABFDEMODIR%\%ABFSERVERNAME%\*"
goto sourceABF
:deletebdemo
del "%ABFDEMODIR%\%ABFDATABASENAME%\projmgmt\*"
del "%ABFDEMODIR%\%ABFDATABASENAME%\*"

:sourceABF

echo Creating directories ...
md "%ABFDEMODIR%" > NUL 
echo "%ABFDEMODIR%" created.
if "%ABFSERVERNAME%"=="" goto createdbdir
md "%ABFDEMODIR%%ABFSERVERNAME%" > NUL
echo "%ABFDEMODIR%%ABFSERVERNAME%" created.
md "%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%" > NUL
echo "%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%" created.
md "%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%\projmgmt" >NUL
echo "%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%\projmgmt" created.
cd "%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%\projmgmt"
goto copydemo
:createdbdir
md "%ABFDEMODIR%\%ABFDATABASENAME%" > NUL 
echo "%ABFDEMODIR%\%ABFDATABASENAME%" created.
md "%ABFDEMODIR%\%ABFDATABASENAME%\projmgmt" >NUL 
echo "%ABFDEMODIR%\%ABFDATABASENAME%\projmgmt" created.
cd "%ABFDEMODIR%\%ABFDATABASENAME%\projmgmt"

:copydemo
copy "%TMP_DEMODIR%\*.*" "%DEMO_ING_ABFDIR%\." > NUL 

echo Fixing location of demonstration help files in .osq files ...
for %%i in ("%DEMO_ING_ABFDIR%\*.osq" "%DEMO_ING_ABFDIR%\*.tmp") do sif %%i "II_SYSTEM:[INGRES.FILES.ABFDEMO]" "%ABFDEMODIR%\%ABFSERVERNAME%" > NUL 

echo %1 > "%DEMO_ING_ABFDIR%\ii_dbname.txt"

:loadimageABF
cd "%DEMO_ING_ABFDIR%"
echo.
echo WARNING: reports, forms, procedures, etc. created in the next step
echo will replace those that may already exist in %ABFDATABASENAME%
echo.
call ipchoice Is this acceptable to you? (y or n)
if errorlevel 2 goto dun
echo Loading demonstration into %1 ...
tm.exe -qSQL %1 < sqlcopy.in 
copyform -i %1 qbf_forms.frm
copyapp in %1 iicopyapp.tmp -a"%DEMO_ING_ABFDIR%" -r
sreport -s %1 "%DEMO_ING_ABFDIR%\experience.rw"

@echo %1 > "%DEMO_ING_ABFDIR%\ii_dbname2.txt"

@echo off
:netdb

@echo off
echo.
rem if %ABFSERVERNAME%==. goto compbmsg
echo Setup complete.  Files installed to "%ABFDEMODIR%%ABFSERVERNAME%\%ABFDATABASENAME%"
goto msgcont
:compbmsg
echo Setup complete.  Files installed to "%ABFDEMODIR%\%ABFDATABASENAME%"
:msgcont
echo.
echo.
call ipchoice Do you wish to start abfdemo? (y or n)
if errorlevel 2 goto dun

abf %1

:dun
