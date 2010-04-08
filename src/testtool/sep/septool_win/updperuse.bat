@echo off
REM updperuse -- peruse differences between sep canons and altconons
REM
REM History:
REM	19-oct-1995 (somsa01)
REM		Created from updperuse.sh .
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
setlocal
set bflag=false
call ipset tmpdir ingprenv II_TEMPORARY
call ipset morevar PCwhich more
if "%morevar%"=="" goto NEXT
  echo %morevar%| cut -f 1 -d "." > %tmpdir%\upduse.tmp
  call ipset moreflag PCread %tmpdir%\upduse.tmp 1
  del %tmpdir%\upduse.tmp
  if "%moreflag%"=="%windir%\system32\more" set morevar=more /E
  if not "%moreflag%"=="%windir%\system32\more" set morevar=more
  goto CONTINU
:NEXT
call ipset morevar PCwhich pg
if "%morevar%"=="" goto NEXT2
  set morevar=pg
  goto CONTINU
:NEXT2
  PCecho "Cannot find MORE or PG - using CAT."
  set morevar=cat

:CONTINU
REM set defaults
set batch=false
set new_testenv=%ING_TST%

REM check the args
:LOOP
if "%1"=="" goto ENDLOOP
  if not "%1"=="-b" goto NEXTCAS
    set batch=true
    goto ENDCASE
:NEXTCAS
  if not "%1"=="-l" goto NEXTCAS2
    shift
    PCecho "Getting label %1 ..."
    set orig_label=%tmpdir%\label$$"
    label -g %1 > %orig_label% 2> c:\nul
    orig_temp="/tmp/updper$$"
    goto ENDCASE
:NEXTCAS2
  if not "%1"=="-n" goto NEXTCAS3
    shift
    set new_testenv=%1
    goto ENDCASE
:NEXTCAS3
  if not "%1"=="-o" goto NEXTCAS4
    shift
    set orig_testenv=%1
    goto ENDCASE
:NEXTCAS4
  PCecho "usage: updperuse { -o <orig testenv> | -l <orig label> } [ -n <new testenv> ]"
  PCecho "       [ -b ]"
  goto DONE
:ENDCASE
shift
goto LOOP

:ENDLOOP
REM set directory & file symbols
set updok=%new_testenv%\\UPDOK
touch %updok%

if not "%orig_testenv%"=="" if "%orig_label%"=="" goto CONTINU2
if "%orig_testenv%"=="" if not "%orig_label%"=="" goto CONTINU2
if "%orig_testenv%"=="" if "%orig_label%"=="" goto CONTINU2
  PCecho "Must secify original testenv directory or label."
  goto DONE

:CONTINU2
PCecho "Finding and comparing all *.sep files ..."

PCfind %new_testenv% -name "*.sep" -print | egrep -v "SCCS|RCS|ORIG" > %tmpdir%\newfile.tmp
call ipset count expr 0
:SEPDO
call ipset count expr %count% + 1
call ipset newfile PCread %tmpdir%\newfile.tmp %count%
if "%newfile%"=="" goto SEPDONE
    grep "%newfile%" %updok% > c:\nul
    if errorlevel 0 if not errorlevel 1 set redo=false
    if errorlevel 0 if not errorlevel 1 goto ENDIF
        if not "%orig_testenv%"=="" echo %newfile%> %tmpdir%\file2.tmp
        if not "%orig_testenv%"=="" call ipset oldfile sed "s#%new_testenv%#%orig_testenv%#" %tmpdir%\file2.tmp
        if not "%orig_testenv%"=="" del %tmpdir%\file2.tmp
        if not "%orig_testenv%"=="" goto ENDIF2
	    set oldfile=%orig_temp%
	    call ipset newdir dirname %newfile%
	    call ipset newbase basename %newfile%
            cd > %tmpdir%\file2.tmp
            cd %newdir%
            echo %newdir%> %tmpdir%\file3.tmp
            call ipset flag cut -c2 %tmpdir%\file3.tmp
            if "%flag%"==":" call ipset drv cut -c1 %tmpdir%\file3.tmp
            if not "%flag%"==":" set drv=
            if not "%drv%"=="" %drv%:
            del %tmpdir%\file3.tmp
            have %newbase% 2> c:\nul > %tmpdir%\havelist.tmp
            grep %newbase% %tmpdir%\havelist.tmp > %tmpdir%\havefile.tmp
            call ipset havefile sed "s/[ 	].*//" %tmpdir%\havefile.tmp
            del %tmpdir%\havelist.tmp
            del %tmpdir%\havefile.tmp
            
            grep "%havefile%" %orig_label% > %tmpdir%\lrev.tmp
            call ipset lrev sed "s/^[^ 	]*[ 	]*\([^ 	]*\).*$/\1/" %tmpdir%\lrev.tmp
            del %tmpdir%\lrev.tmp

	    display -r "%lrev%" %newbase%> %oldfile% 2> c:\nul

            call ipset drv PCread %tmpdir%\file2.tmp 1
            cd %drv%
            call ipset drv cut -c1 %tmpdir%\file2.tmp
            %drv%:
            del %tmpdir%\file2.tmp
:ENDIF2
    	cmp %oldfile% %newfile% > c:\nul
    	if errorlevel 1 if not "%batch%"=="true" set redo=true
        if errorlevel 1 goto ENDIF
	    PCecho "%newfile% unchanged"
	    PCecho "%newfile%">> %updok%
	    set redo=false
:ENDIF
:DOREDO
    if not "%redo%"=="true" goto REDODONE
        set redo=false
        echo.
        PCecho "--- begin: altcompare %oldfile% %newfile% ---"
        altcompare %oldfile% %newfile% | %morevar%
        PCecho "--- end: altcompare %oldfile% %newfile% ---"
        set prompting=true
:DOPROMPTING
        if not "%prompting%"=="true" goto PROMPTINGDONE
            set prompting=false
            if errorlevel 2 if not errorlevel 3 set desire=n
            echo.
            if "%bflag%"=="true" if not "%desire%"=="n" choice /c:hnadvsob /n h)elp n)ext a)ltcompare d)iff v)iew s)hell o)k b)ye: 
            if "%bflag%"=="true" if not "%desire%"=="n" goto CASE
            PCecho "h)elp n)ext a)ltcompare d)iff v)iew s)hell o)k b)ye: " 
:CASE
            if errorlevel 2 goto AMENU
echo.
PCecho "h)elp		Print this message."
PCecho "n)ext		Go to next altcompare (default). "
PCecho "a)ltcompare	Show this altcompare again."
PCecho "d)iff		Show difference between old and new script."
PCecho "v)iew		Run view against both old and new scripts."
PCecho "s)hell		Launch a SHELL, cmd default."
PCecho "o)k		Mark difference as OK in %updok% so it won't be seen again."
PCecho "b)ye		Exit updperuse."
                    set prompting=true
                    goto CASEEND
:AMENU
            if errorlevel 4 goto DMENU
            if errorlevel 2 if not errorlevel 3 goto CASEEND
                set redo=true
                goto CASEEND
:DMENU
            if errorlevel 5 goto VMENU
                echo.
                PCecho "*** begin: diff %oldfile% %newfile% ***"
                diff %oldfile% %newfile% | %morevar%
                PCecho "*** end: diff %oldfile% %newfile% ***"
                set prompting=true
                goto CASEEND
:VMENU
            if errorlevel 6 goto SMENU
                view %oldfile% %newfile%
                set prompting=true 
                goto CASEEND
:SMENU
            if errorlevel 7 goto OMENU
                if "%SHELL%"=="" set SHELL=cmd
                PCecho "peruse: continuing..."
		set redo=true
                goto CASEEND
:OMENU
            if errorlevel 8 goto BMENU
                PCecho "%newfile%">> %updok%
                goto CASEEND
:BMENU
            if errorlevel 9 goto CASEEND
                goto DONE
:CASEEND
            goto DOPROMPTING
:PROMPTINGDONE
        goto DOREDO
:REDODONE
goto SEPDO
:SEPDONE
if not "%orig_label%"=="" del %orig_label% %orig_temp%
:DONE
if exist %tmpdir%\newfile.tmp del %tmpdir%\newfile.tmp
endlocal
