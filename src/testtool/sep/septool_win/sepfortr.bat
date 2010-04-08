@echo off
REM This script makes an executable file from a FORTRAN source file.
REM It is called as an OS/TCL command from the SEP FORTRAN tests.
REM
REM Usage is:
REM
REM       sepfortr <querylanguage> <programname> <libnames...>
REM
REM where <querylanguage> is either 'sql' or 'quel'
REM       <programname> is the root part of a *.sa or *.qa file
REM       <libnames...> are a list of libraries for this test (i.e.,
REM               the outputs of the "seplib" command, without the ".lib"
REM               extension.)
REM
REM Examples:
REM
REM   To make an executable called "thing.exe" from an FORTRAN/QUEL
REM   file called "thing.qf":
REM
REM       sepfortr quel thing
REM
REM   To make an executable called "stuff.exe" from and FORTRAN/SQL source
REM   file called "stuff.sf" and a library named "myforms.lib":
REM
REM       sepfortr sql stuff myform
REM
REM About libraries:
REM
REM   The library names passed on the 'sepfortr' command line must be portable
REM   (i.e., no path names, etc.).  Ideally, the same 'sepfortr' command
REM   should work on any platform (Unix, VMS... even MPE XL!) and in any
REM   test environment (PT, OSG, PKS...).
REM
REM   Use the environment variable SEP_FORTRAN_LD to pass libraries to 'sepfortr'
REM   that are not specific to an individual test. Also, use SEP_FORTRAN_LD to
REM   pass names of libraries that are platform dependent. For example,
REM
REM    set SEP_FORTRAN_LD=%II_SYSTEM%\ingres\lib\libingres.lib
REM
REM   But DON'T set SEP_FORTRAN_LD from within a SEP script! Always set it
REM   BEFORE you invoke SEP or Listexec!
REM
REM History:
REM     11-Apr-2001 (fanra01)
REM         Created from sepfortr.sh.
REM	 2-Sep-2003 (rogch01)
REM		Use sqlcode hack.
REM		08-Oct-2003 (fanra01)
REM		    Add nodefaultlib flag to remove library warning
REM   	18-aug-2004 (drivi01)
REM          Updated link library names to new
REM          jam names.

setlocal

REM Check for an acceptable query language.
if not "%1"=="" if not "%2"=="" goto checklang
    echo ERROR: usage is %0 ^<querylanguage^> ^<programname^> [[^<libraries^>]..]
    goto end

:checklang
if "%1"=="quel" set qsuffix=qf&set preproc=eqf&goto checkfile
if "%1"=="sql" set qsuffix=sf&set preproc=esqlf&goto checkfile
if "%1"=="sql-sqlcode" set qsuffix=sf&set preproc=esqlf -sqlcode&goto checkfile
    echo ERROR: invalid query language: %1. Must be 'quel' or 'sql'
    goto end

REM Check that the source file exists.
:checkfile
if exist %2.%qsuffix% set sfile=%2&shift&shift&goto getlibs
echo ERROR: %2.%qsuffix% is not a valid input source file
goto end

REM Get any libraries or objects specified on the command line.
:getlibs
if "%1"=="" goto endloop
    if exist %1.lib if not "%libs%"=="" set libs=%libs% %1.lib&goto nextparm
    if exist %1.lib if "%libs%"=="" set libs=%1.lib&goto nextparm
    if exist %1.obj if not "%libs%"=="" set libs=%libs% %1.obj&goto nextparm
    if exist %1.obj if "%libs%"=="" set libs=%1.obj&goto nextparm

    echo ERROR: %1.lib or %1.obj is not a valid load library file
    goto end

:nextparm
    set ext=
    shift
    goto getlibs

:endloop

REM Set the compiler command
set compiler=%DF%
if "%compiler%" == "" set compiler=DF

REM Set compiler options
set fortopt=/nologo /compile_only /name:as_is /iface:nomixed_str_len_arg /iface:cref /libs:static /traceback /warn:nogeneral /ccdefault:default /fpscomp:none /threads

REM Set compiler defines
set fortdef=-D_WIN32=1 -D_X86_=1 

REM Set include directories
rem set incdirs=/includedir:%II_SYSTEM%\ingres\files

REM Perform precompilation on the source file
echo %preproc% %sfile%.%qsuffix%
%preproc% %sfile%.%qsuffix%
if errorlevel 1 echo ERROR: "%preproc%" failed on %sfile%&goto end

REM Libraries
set deflibs=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

REM Link options
set lnkopt=/nologo /subsystem:console /incremental:no /machine:I386 /nodefaultlib:msvcrt.lib

REM Generate object file and executable.
echo %compiler% %fortopt% %fortdef% %incdirs% /source:%sfile%.for
%compiler% %fortopt% %fortdef% %incdirs% /source:%sfile%.for
echo link %lnkopt% /OUT:%sfile%.exe %sfile%.obj %deflibs% %libs% %SEP_FORTRAN_LD% %II_SYSTEM%\ingres\lib\libingres.lib
link %lnkopt% /OUT:%sfile%.exe %sfile%.obj %deflibs% %libs% %SEP_FORTRAN_LD% %II_SYSTEM%\ingres\lib\libingres.lib

:end
endlocal
