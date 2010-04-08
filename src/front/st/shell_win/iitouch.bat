@echo off
@REM  Copyright (c) 1995, 1997 Ingres Corporation
REM 
REM  Name: iitouch - Create the symbol.tbl and config.dat if they do not exist 
REM
REM  Description:
REM 	Create the symbol.tbl and config.dat if they do not exist with the
REM 	MS-DOS equililent.
REM 
REM  History:
REM 	19-jul-95 (reijo01)
REM 		Created.
REM	24-nov-1997 (canor01)
REM		Quote pathnames to support embedded spaces. Change "@REM" to
REM		"REM" so the make will strip the comments.

@if exist "%II_SYSTEM%\ingres\files\symbol.tbl" goto CONFIG
	@REM > "%II_SYSTEM%\ingres\files\symbol.tbl"

:CONFIG
@if exist "%II_CONFIG%\config.dat" goto EXIT
	@REM > "%II_SYSTEM%\ingres\files\symbol.tbl"
:EXIT
