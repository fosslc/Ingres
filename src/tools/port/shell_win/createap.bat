@echo off
if not x%1==x goto doit
echo Use createdb [database-name] on your server to create a database to use
echo  for the PROJMGMT application.
echo Then run %0 [server-name]::[database-name] to load the database.
echo 
echo Example:	%0 myserver::abfdemo
goto dun
:doit

set TMP_DEMODIR=%II_SYSTEM%\ingres\files\abfdemo
set ABFDEMODIR=%II_SYSTEM%\ingres\abf
set DEMO_ING_ABFDIR=%ABFDEMODIR%

if exist %II_SYSTEM%\ingres\bin\iiabf.exe goto installedABF
echo Cannot locate ABF in the directory %II_SYSTEM%\ingres\bin.
echo Please install the application before running this set-up.
goto dun

:installedABF

:cleanABF
if exist %II_SYSTEM%\ingres\files\abfdemo\top.osq goto dbABF
echo Set-up has been unable to locate the demonstration source files.
echo Please install the demonstration and re-run this set-up.
goto dun

:dbABF
if not exist %ABFDEMODIR%\ii_dbname.txt goto sourceABF
echo %1 > tmpabfdemodb
findstr /G:%ABFDEMODIR%\ii_dbname.txt tmpabfdemodb
if not errorlevel 1 goto loadimageABF
echo Set-up has detected a previous demonstration installation using the
echo database
type         %ABFDEMODIR%\ii_dbname.txt
echo Please remove it and re-run the set-up.
goto dun:

:sourceABF
echo Locating demonstration files in the %ABFDEMODIR% directory.

echo Creating directories ...
md %ABFDEMODIR% > NUL 2>&1
echo %ABFDEMODIR% created.
md %ABFDEMODIR%\%1 > NUL 2>&1
echo %ABFDEMODIR%\%1 created.
md %ABFDEMODIR%\%1\projmgmt >NUL 2>&1
echo %ABFDEMODIR%\%1\projmgmt created.
cd %ABFDEMODIR%

copy %TMP_DEMODIR%\*.* %DEMO_ING_ABFDIR% > NUL 2>&1

for %%i in (%DEMO_ING_ABFDIR%\*.osq %DEMO_ING_ABFDIR%\*.tmp) do sif %%i "II_SYSTEM:[INGRES.FILES.ABFDEMO]" %DEMO_ING_ABFDIR%\ > NUL 2>&1
del *.bak

:loadimageABF
echo Loading demonstration into %1 ...
call sql %1 < sqlcopy.in 
copyform -i %1 qbf_forms.frm
copyapp in %1 iicopyapp.tmp -a%ABFDEMODIR%
sreport -s %1 %TMP_DEMODIR%\experience.rw

echo %1> tmpabfdemodb
findstr /C:"::" tmpabfdemodb > NUL
if not errorlevel 1 goto netdb
sysmod %1
:netdb
del tmpabfdemodb

@echo off
echo.
echo Set-up complete.  Files installed to %ABFDEMODIR%.
echo.
echo.
ipchoice Do you wish to start abfdemo?
if errorlevel 2 goto dun

abf %1

:dun
