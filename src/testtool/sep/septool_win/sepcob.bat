@echo off
REM This script makes a code that can be run using the Ingres-linked RTS.
REM It also builds the executable ingrts for you also.
REM It is called as an OS/TCL command from the SEP COBOL tests.
REM
REM Usage is:
REM	
REM	sepcob <querylanguage> <programname> <libnames...>
REM
REM where <querylanguage> is either 'sql' or 'quel'
REM	<programname> is the root part of a *.scb or *.qcb file
REM	<libnames...> are a list of libraries for this test (i.e.,
REM		the outputs of the "seplib" command, without the ".a"
REM		extension.) 
REM
REM Examples:
REM	
REM   To make a runnable code from a COBOL/QUEL file called "thing.qcb": 
REM
REM	sepcob quel thing
REM
REM   To run the file using ingrts:
REM
REM	ingrts thing
REM
REM   To make a runnable code from a COBOL/SQL source file called "stuff.scb" 
REM   and a library named "myforms.a":
REM	
REM	sepcob sql stuff myform
REM
REM   
REM About libraries:
REM
REM   The library names passed on the 'sepcob' command line must be portable
REM   (i.e., no path names, etc.).  Ideally, the same 'sepcob' command 
REM   should work on any platform (Unix, VMS... even MPE XL!) and in any
REM   test environment (PT, OSG, PKS...).
REM   Object files can also be passed in.
REM
REM History:
REM	13-Feb-1996 (somsa01)
REM		Created from sepcob.sh .
REM	18-Apr-2001 (rogch01)
REM		Filter out COFF / OMF linker warnings.  They are benign.
REM	 7-Feb-2002 (rogch01)
REM		Changes for Merant Net Express, which provides cbllink for
REM		linking.
REM	25-Sep-2003 (rogch01)
REM		Add sql-sqlcode hack as for Unix, lose use of cblnames.
REM		cblnames is called by cbllink.  When referencing it directly,
REM		multiply defined symbols appear - because there are two files
REM		created with them in, one by cbllink, one by cblnames.
REM	08-Jul-2010 (drivi01)
REM		Add libcmt.lib to the link line to resolve security_cookie
REM		symbol in the binaries. The symbol is there due to /GS flag
REM		that is used by the compiler by default.
REM

REM First, check args and determine $qsuffix, $preproc, and $sfile:

setlocal
if not "%1"=="" if not "%2"=="" goto NEXT
	PCecho "ERROR: usage is 'sepcob <querylanguage> <programname> <libraries>'"
	goto END

:NEXT
if "%1"=="quel" set qsuffix=qcb
if "%1"=="quel" set preproc=eqcbl

if "%1"=="sql" set qsuffix=scb
if "%1"=="sql" set preproc=esqlcbl

if "%1"=="sql-sqlcode" set qsuffix=scb
if "%1"=="sql-sqlcode" set preproc=esqlcbl -sqlcode

if "%1"=="" PCecho "ERROR: invalid query language: %1. Must be 'quel' or 'sql'"
if "%1"=="" goto END

REM if [ -r $2.$qsuffix -a -s $2.$qsuffix ]
if exist %2.%qsuffix% set sfile=%2
if not exist %2.%qsuffix% PCecho "ERROR: %2.%qsuffix% is not a valid input source file"
if not exist %2.%qsuffix% goto END

REM The remaining args (3 thru N) must be libraries.
REM Check to make sure they're in the current directory and
REM add them to the $libs list:

shift
shift
:WHILELOOP
if "%1"=="" goto ENDWHILE
	if exist %1.lib if not "%libs%"=="" set libs=%libs% %1.lib
	if exist %1.lib if "%libs%"=="" set libs=%1.lib
	if exist %1.lib goto NEXTLOOP

	if exist %1.obj if not "%libs%"=="" set libs=%libs% %1.obj
	if exist %1.obj if "%libs%"=="" set libs=%1.obj
	if exist %1.obj goto NEXTLOOP

	PCecho "ERROR: %1.lib is not a valid load library file"
	goto END
	
:NEXTLOOP
	shift
	goto WHILELOOP

:ENDWHILE
%preproc% %sfile%.%qsuffix%
if errorlevel 1 PCecho "ERROR: "%preproc%" failed on %sfile%"
if errorlevel 1 goto END

REM compile and link the executable 

cobol %sfile%.cbl /case /litlink;
cbllink -O%sfile%.exe %sfile%.obj %libs% libingres.lib libcmt.lib

:END
endlocal
