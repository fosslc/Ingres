@echo off
REM
REM makelocs -- set up INGRES test installation for rodv tests
REM
REM History:
REM 	18-oct-1995 (somsa01)
REM		Created from makelocs.sh .
REM
REM must run as INGRES super-user, either ingres or as a porter.
REM

REM create directories:
setlocal
REM locations used in tests
set qaloc1=

call ipset tmpdir ingprenv II_TEMPORARY
echo %ING_BUILD%| cut -c1 > %tmpdir%\makelocs.tmp
call ipset drv PCread %tmpdir%\makelocs.tmp 1
del %tmpdir%\makelocs.tmp
%drv%:
cd %ING_BUILD%
:LOOP
if "%1"=="" goto ENDLOOP
echo %1>> %tmpdir%\makelocs.tmp
shift
goto LOOP

:ENDLOOP
echo data>> %tmpdir%\makelocs2.tmp
echo ckp>> %tmpdir%\makelocs2.tmp
echo jnl>> %tmpdir%\makelocs2.tmp

call ipset count expr 0
call ipset count2 expr 0
:LOOP2
call ipset count expr %count% + 1
call ipset loc PCread %tmpdir%\makelocs.tmp %count%
if "%loc%"=="" goto ENDLOOP2
:SUBLOOP
  call ipset count2 expr %count2% + 1
  call ipset area PCread %tmpdir%\makelocs2.tmp
  if "%area%"=="" goto LOOP2
    cd > %tmpdir%\makelocs3.tmp
    cd %area%\%loc% 2> %tmpdir%\makelocs4.tmp
    call ipset flag PCread %tmpdir%\makelocs4.tmp 1
    if "%flag%"=="" goto DIREXISTS
      PCecho "Making directory %area%\%loc%..."
      mkdir %area%\%loc%
      del %tmpdir%\makelocs3.tmp
      del %tmpdir%\makelocs4.tmp
      goto SUBLOOP
:DIREXISTS
    PCecho "Directory %area%\%loc% already exists."
    call ipset drv PCread %tmpdir%\makelocs3.tmp
    cd %drv%
    call ipset drv cut -c1 %tmpdir%\makelocs3.tmp
    %drv%:
    del %tmpdir%\makelocs3.tmp
    del %tmpdir%\makelocs4.tmp
    goto SUBLOOP
      
:ENDLOOP2
del %tmpdir%\makelocs.tmp
del %tmpdir%\makelocs2.tmp
endlocal
REM add to system catalog:

echo Adding locations...

set TERM_INGRES=vt100
set II_TERMCAP_FILE=
accessdb -Z%ING_SRC%\\testtool\\sep\\files\\makelocs.key
