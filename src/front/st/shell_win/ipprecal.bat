@echo off
@REM  Copyright (c) 1995, 1997 Ingres Corporation
REM
REM  Name: ipprecall  - Execute before CA-Ingres installation.
REM
REM  Usage: 
REM	ipprecall %II_SYSTEM%
REM
REM  Description:
REM 	Executes commands before the CA-installer unloads the files for 
REM	each product.
REM	For now this file will simply delete the ipsetup.bat file if it
REM	exists.
REM
REM  History:
REM	09-aug-1995 (reijo01)
REM		Created.  
REM	14-aug-1995 (fauma01)
REM		Updated to use %II_SYSTEM% directly, if it exists.
REM	24-nov-1997 (canor01)
REM		Quote pathnames to support embedded spaces.
REM 
if "%II_SYSTEM%" == "" goto EXIT
if not exist "%II_SYSTEM%\ingres\bin\ipsetup.bat" goto EXIT
erase "%II_SYSTEM%\ingres\bin\ipsetup.bat"
:EXIT
