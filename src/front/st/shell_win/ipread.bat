@REM  Copyright (c) 1995 Ingres Corporation
@REM
@REM  Name: ipread  - Set a command enviroment variable to the input from the
@REM		      keyboard.
@REM
@REM  Usage: 
@REM	ipread [variable_name] 
@REM
@REM  Description:
@REM 	Sets a command enviroment variable to the input entered from a console. 
@REM    Executes ipreadp which reads a line from standard input and writes
@REM  	it to standard output. This gets piped to ipsetp which will construct
@REM    a MS-DOS set variable command and pipe that command to standard output.
@REM    Next the set command is redirected to a temporary batch file which 
@REM  	will be executed and deleted(in that order).
@REM
@REM  History:
@REM	18-jul-1995 (reijo01)
@REM		Created.  
@REM 
@ipreadp | ipsetp %1 > ipsettmp.bat
@call ipsettmp.bat
@erase ipsettmp.bat
