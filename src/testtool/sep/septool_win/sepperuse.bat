@echo off
REM sepperuse -- peruse sep tests with differences
REM
REM History:
REM	19-oct-1995 (somsa01)
REM		Created from sepperuse.sh .
REM
REM 	03-Apr-1997 (clate01)
REM		Shortened label names, in order to run successfully
REM		on Windows'95, label names longer than 8 chars are
REM		truncated, which caused problems
setlocal
if not "%ING_TST%"=="" goto CONTINU
  PCecho "The variable ING_TST is not set!!"
  goto DONE

:CONTINU
if not "%TST_LISTEXEC%"=="" goto CONTINU2
  PCecho "The variable TST_LISTEXEC is not set!!"
  goto DONE

:CONTINU2
call ipset tmpdir ingprenv II_TEMPORARY
call ipset morevar PCwhich more
if "%morevar%"=="" goto NEXT
  echo %morevar%| cut -f 1 -d "." > %tmpdir%\sepuse.tmp
  call ipset moreflag PCread %tmpdir%\sepuse.tmp 1
  del %tmpdir%\sepuse.tmp
  if "%moreflag%"=="%windir%\system32\more" set morevar=more /E
  if not "%moreflag%"=="%windir%\system32\more" set morevar=more
  if not "%moreflag%"=="%windir%\system32\more" set diffscan=+/DIFF
  goto CONTINU3
:NEXT
call ipset morevar PCwhich pg
if "%morevar%"=="" goto NEXT2
  set morevar=pg
  set diffscan=+/DIFF/
  goto CONTINU3
:NEXT2
  PCecho "Cannot find MORE or PG - using CAT."
  set morevar=cat

:CONTINU3
REM check the args
set alllog=false
set moreparam=
:LOOP
if "%1"=="" goto ENDLOOP
  if not "%1"=="-a" goto NEXTCAS
    set alllog=true
    goto ENDCASE
:NEXTCAS
  if not "%1"=="-d" goto NEXTCAS2
    set moreparam=%diffscan%
    goto ENDCASE
:NEXTCAS2
  if not "%1"=="-o" goto NEXTCAS3
    shift
    cd > %tmpdir%\file.tmp
    cd %1 2> %tmpdir%\file2.tmp
    call ipset flag PCread %tmpdir%\file2.tmp 1
    del %tmpdir%\file2.tmp
    if not "%flag%"=="" goto DONE
      echo %1> %tmpdir%\file2.tmp
      call ipset flag cut -c2 %tmpdir%\file2.tmp
      set drv=
      if "%flag%"==":" call ipset drv cut -c1 %tmpdir%\file2.tmp
      if not "%drv%"=="" %drv%:
      call ipset tst_output cd
      call ipset drv cut -c1 %tmpdir%\file.tmp
      %drv%:
      del %tmpdir%\file2.tmp
      del %tmpdir%\file.tmp
      goto ENDCASE
:NEXTCAS3
  echo %1| cut -c1 > %tmpdir%\file.tmp
  call ipset flag PCread %tmpdir%\file.tmp 1
  del %tmpdir%\file.tmp
  if not "%flag%"=="-" goto NEXTCAS4
    PCecho "usage: sepperuse [ -a ] [ -o <outdir> ] [ <cfg-file-name> ... ]"
    goto DONE
:NEXTCAS4    
  if "%1"=="" goto ENDLOOP
  echo %1>> %tmpdir%\files.tmp
  shift
  goto NEXTCAS4
:ENDCASE
shift
goto LOOP

:ENDLOOP
call ipset files PCread %tmpdir%\files.tmp 1
if "%files%"=="" set nocfg=true
if not "%files%"=="" set nocfg=false


REM set directory & file symbols
if "%tst_output%"=="" set tst_output=%ING_TST%\\output
set sepok=%tst_output%\\SEPOK
touch %sepok%

cd %tst_output%
echo %tst_output%> %tmpdir%\file.tmp
call ipset flag cut -c2 %tmpdir%\file.tmp
set drv=
if "%flag%"==":" call ipset drv cut -c1 %tmpdir%\file.tmp
del %tmpdir%\file.tmp
if not "%drv%"=="" %drv%:

if "%alllog%"=="" goto ELSE
    for %%i in (*.log) do set list=%list% %%i
    goto CONTINU4
:ELSE
    set allout=%tmpdir%\\peruse.$$
    if "%nocfg%"=="" goto ELSE2
	cat %tst_output%\\*.out> %allout%
        goto ENDIF
:ELSE2
        call ipset count expr 0
:LOOP2
        call ipset count expr %count% + 1
        call ipset file PCread %tmpdir%\files.tmp %count%
        if "%file%"=="" goto ENDIF
            if exist %TST_LISTEXEC%\%file% goto CONTLOOP
	        PCecho "%TST_LISTEXEC%\\%file% does not exist!"
	        goto DONE

:CONTLOOP
            echo %file%> %tmpdir%\file2.tmp
            call ipset out sed "s/.cfg/.out/" %tmpdir%\file2.out
            del %tmpdir%\file2.out

            if exist %out% cat %out%>> %allout%
            if exist %out% goto LOOP2
	        PCecho "Cannot find %out% SEP output file.  Have you run the %file% tests?"
	        goto DONE

:ENDIF
    awk "
        /\=\>/ {
	    while (index($4,"/") != 0) {
	        $4 = substr($4,index($4,"/")+1)
	    }
	    $4 = substr($4,1,index($4,"."))
	    save = sprintf("%slog",$4)
        }

        /Number of diff/ {
	    if ($6 != 0)
	        print save
        }
    " < $allout > %tmpdir%\list.tmp

    del %allout%

:CONTINU4
call ipset count expr 0
:DOFILE
call ipset count expr %count% + 1
call ipset i PCread %tmpdir%\list.tmp
if "%i%"=="" goto DONEFILE
    grep "%i%" %sepok% > c:\nul
    if errorlevel 0 if not errorlevel 1 set redo=false
    if errorlevel 1 set redo=true
:DOREDO
    if not "%redo%"=="true" goto DONEREDO
	set redo=false
        echo.
	PCecho "--- begin (%tst_output%\\%i%) ---"
	%morevar% %moreparam% %i%
	PCecho "--- end (%tst_output%\\%i%) ---"
	set prompting=true
:DOPROMPTING
        if not "%prompting%"=="true" goto DONEPROMPTING
	    set prompting=false
            echo.
	    choice /c:hnrposb /n h)elp n)ext r)epeat p)rint o)k s)hell b)ye: 
            if errorlevel 2 goto NMENU
echo.
PCecho "h)elp		Print this message."
PCecho "n)ext		Go to next log file, leaving this for later perusal."
PCecho "r)epeat		Show this log file again."
PCecho "p)rint		Print the log file with lpr -p."
PCecho "o)k		Mark log file as OK in %sepok% so it won't be seen again."
PCecho "s)hell		Launch a SHELL, cmd default."
PCecho "b)ye		Exit sepperuse."
		    set prompting=true
		    goto CASEEND
:NMENU
                if errorlevel 3 goto RMENU
                    goto CASEEND
:RMENU
                if errorlevel 4 goto PMENU
                    set redo=true
                    goto CASEEND
:PMENU
                if errorlevel 5 goto OMENU
                    start /b lpr -p %tst_output%\%i%
                    set redo=true
                    goto CASEEND
:OMENU                    
                if errorlevel 6 goto SMENU
                    PCecho "%i%">> %sepok%
                    goto CASEEND
:SMENU
                if errorlevel 7 goto BMENU
                    if "%SHELL%"=="" set SHELL=cmd.exe
                    PCecho "sepperuse: continuing..."
                    set redo=true
                    goto CASEEND
:BMENU
                if errorlevel 8 goto CASEEND
                    goto DONEFILE
:CASEEND
	goto DOPROMPTING
:DONEPROMPTING
    goto DOREDO
:DONEREDO
goto DOFILE
:DONEFILE
del %tmpdir%\list.tmp
:DONE
if exist %tmpdir%\files.tmp del %tmpdir%\files.tmp
if exist %tmpdir%\file.tmp del %tmpdir%\file.tmp
endlocal
