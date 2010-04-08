@echo off
@REM  Copyright (c) 1995, 1997 Ingres Corporation
REM
REM  Name: ipset  - Set a command enviroment variable to the output from a 
REM		    command.
REM
REM  Usage: 
REM	ipset [variable_name] [command] {arg1, arg2, ...}
REM
REM  Description:
REM 	Sets a command enviroment variable to the output of the specified 
REM    command.
REM 	Executes the command and pipe results to ipsetp. ipsetp will write to 
REM 	standard output a MS-DOS set command that is constructed from 
REM 	the command line argument and standard input.
REM	The set command is piped into a temporary batch file which is then 
REM	executed and deleted(in that order).
REM
REM  History:
REM	18-jul-1995 (reijo01)
REM		Created.  
REM	05-jan-96 (tutto01)
REM		Make sure that ipsettmp.bat is written to a writable directory.
REM	26-mar-96 (tutto01)
REM		Specify full path so that executable is found during Windows 95
REM		installation.
REM	24-nov-1997 (canor01)
REM		Quote pathnames to support embedded spaces. Change "@REM" to
REM		"REM" so the make will strip comments.
REM 
@%2 %3 %4 %5 %6 %7 %8 %9 | "%II_SYSTEM%\ingres\bin\ipsetp" %1 > "%II_SYSTEM%\ingres\ipsettmp.bat"
@call "%II_SYSTEM%\ingres\ipsettmp.bat"
@erase "%II_SYSTEM%\ingres\ipsettmp.bat"
