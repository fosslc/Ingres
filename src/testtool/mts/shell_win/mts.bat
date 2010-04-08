@echo off
REM
REM MTS - submit the mtscoord process and
REM	the requested number of mtslave process
REM
REM History:
REM	19-sep-1995 (somsa01)
REM		Created from mts.sh .
REM
REM %1 is database name
REM %2 is test id
REM %3 is number of slaves
REM %4 i traceflag

setlocal
del mts%2_cord.res
start /b mtscoord %1 %2 %3 %4 > mts%2_cord.res
echo. 
echo Started MTS coordinator

REM
REM add this to help in multi-STAR configurations -
REM the START table is destroyed by the setup
REM sql, then re-created by the coordinator.
REM It is possible for the slaves to run, and
REM exhaust their sleep time before the coordinator
REM can read all the slaves are up and create the table
REM again.  This lets the coordinator get a head start
REM on the slaves.
REM	jds	2/14/92
REM

sleep 60

call ipset count expr 1
call ipset condition %3 + 1

:BEGIN
if not "%count%"=="%condition%" goto DO
goto DONE

:DO
   del mts%2_%count%.res
   start /b mtsslave %1 %2 %count% %4 > mts%2_%count%.res
   echo. 
   echo Started MTS slave number %count%
   call ipset count expr %count% + 1
   goto BEGIN

:DONE
echo. 
echo All MTS slaves are running

REM wait until slaves and coordinator are done
call ipset count expr 1
:TAG1
  if not "%count%"=="%condition%" goto TAG2
  goto ENDSLAVE

:TAG2
  call ipset test_string grep "SLAVE #%count%: Terminating..." mts%2_%count%.res
  if "%test_string%"=="" goto TAG2
  call ipset count expr %count% + 1
  goto TAG1

:ENDSLAVE
  call ipset test_string grep "COORDINATOR: testing completed." mts%2_cord.res
  if "%test_slave%"=="" goto ENDSLAVE

echo. 
echo MTS coordinator and slaves are done

endlocal
