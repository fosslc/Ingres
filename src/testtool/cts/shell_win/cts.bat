@echo off
REM History:
REM	15-sep-1995	New cts.bat created (from cts.sh) for 
REM                     SEPized CTS (somsa01)
REM	08-Dec-1995 (somsa01)
REM		Integrated changes made by wadag01 on 24-nov-1995 to cts.sh .
REM	29-Dec-1995 (somsa01)
REM		Cleaned up printout of messages to resemble the shell script.
REM	17-Jan-1996 (somsa01)
REM		Replaced "rm" command with "del".
REM
REM This is the "cts" command issued by the CTS SEP tests.
REM It starts up the CTS coordinator and slaves and waits till they're done.
REM
REM $1 is database name
REM $2 is test number (from test_num table)
REM $3 is number of slaves
REM $4 is the SEP test id (passed by SEP but not used in Unix)

REM the trace flag should always be 0 for this "cts" command.
REM
setlocal
set trace_flag=0

REM All result files are suffixed ".res".
REM Old .res files are removed before the CTS programs start.
REM The .res files produced by this test are left so SEP can diff them.

REM The Coordinator outputs a file called ctscoord.res. 
REM
set coordfile=%TST_OUTPUT%\output\%4_c

REM The Slave executable outputs files called ctsslv1.res, ctsslv2.res, and
REM so forth. 
REM
set slavefile=%TST_OUTPUT%\output\%4_

if exist %coordfile%.res del %coordfile%.res
start /b ctscoord %1 %2 %3 %trace_flag% > %coordfile%.res
echo. 
echo Started CTS coordinator

call ipset count expr 1
call ipset condition expr %3 + 1

:BEGIN
if not "%count%"=="%condition%" goto DO
goto DONE

:DO
   if exist %slavefile%_%count%.res del %slavefile%_%count%.res
   start /b ctsslave %1 %2 %count% %trace_flag% > %slavefile%%count%.res
   echo.
   PCecho "Started CTS slave number %count%"
   call ipset count expr %count% + 1
   goto BEGIN

:DONE
 
echo.
PCecho "All CTS slaves are running"

REM wait until slaves and coordinator are done
call ipset count expr 1
:TAG1
  if not "%count%"=="%condition%" goto TAG2
  goto ENDSLAVE

:TAG2
  call ipset test_string grep "SLAVE #%count%: Terminating..." %slavefile%%count%.res
  if "%test_string%"=="" goto TAG2
  call ipset count expr %count% + 1
  goto TAG1

:ENDSLAVE
  call ipset test_string grep "COORDINATOR: cleaning up and exiting ..." %coordfile%.res
  if "%test_string%"=="" goto ENDSLAVE

echo.
PCecho "CTS coordinator and slaves are done"
echo.

endlocal
