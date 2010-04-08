@echo off

REM
REM History:
REM	26-Apr-2005 (drivi01) 
REM		Created.
REM
REM


echo.
echo Creating %ING_ROOT%/pdbs directory
echo.

if not exist %ING_ROOT%\pdbs mkdir %ING_ROOT%\pdbs

echo.
echo Copying pdbs to %ING_ROOT%/pdbs
echo.

copy %II_SYSTEM%\ingres\bin\*.pdb %ING_ROOT%\pdbs
if ERRORLEVEL 1 goto EXIT
copy %II_SYSTEM%\ingres\lib\*.pdb %ING_ROOT%\pdbs
if ERRORLEVEL 1 goto EXIT
copy %II_SYSTEM%\ingres\utility\*.pdb %ING_ROOT%\pdbs
if ERRORLEVEL 1 goto EXIT
goto DONE

:EXIT
echo ERROR: There was one or more errors copying pdb files to %ING_ROOT%/pdbs.


:DONE
echo.
echo %ING_ROOT%/pdbs was created successfully!!!