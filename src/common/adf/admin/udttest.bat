@echo off
rem
rem  Run the UDT demo, and compare the results to the canonical output (*.log).
rem

set DEMO=%II_SYSTEM%\ingres\demo\udadts

if not "%2" == "" goto gotparms

echo.
echo Usage: udttest dbname output_directory
echo.
echo.


:gotfirst

echo This program will execute the sample queries that use the User Defined
echo Datatypes (UDTs).  These queries are provided in the UDT demonstration
echo directory (%DEMO%).

echo NOTE: The sample queries can only be run if the iiudtnt.dll in the
echo %II_SYSTEM%\ingres\bin directory has been linked with the UDT object 
echo code provided in the UDT demonstration directory.

echo.
echo First stop any running installation by typing "ingstop".
echo Then make the new iiudtnt.dll by typing "NMAKE" in 
echo the %DEMO% directory.
echo Finally, restart the installation by typing "ingstart".

goto ezend

:gotparms

if not exist %2 mkdir %2
if exist %2 goto dirmade
echo.
echo Error creating directory %2
goto ezend

:dirmade

set TESTLIST="intlist op_load op_test cpx zchar pnum_test"

echo Running %DEMO%\intlist.qry . . .
echo Output file: %2\intlist.out
echo sql -s -kf %1 ^< %DEMO%\intlist.qry
call sql -s -kf %1 < %DEMO%\intlist.qry > %2\intlist.out

echo Running %DEMO%\op_load.qry . . .
echo Output file: %2\op_load.out
echo sql -s -kf %1 ^< %DEMO%\op_load.qry
call sql -s -kf %1 < %DEMO%\op_load.qry > %2\op_load.out


echo Running %DEMO%\op_test.qry . . .
echo Output file: %2\op_test.out
echo sql -s -kf %1 ^< %DEMO%\op_test.qry
call sql -s -kf %1 < %DEMO%\op_test.qry > %2\op_test.out


echo Running %DEMO%\cpx.qry . . .
echo Output file: %2\cpx.out
echo sql -s -kf %1 ^< %DEMO%\cpx.qry
call sql -s -kf %1 < %DEMO%\cpx.qry > %2\cpx.out

echo Running %DEMO%\zchar.qry . . .
echo Output file: %2\zchar.out
echo sql -s -kf %1 ^< %DEMO%\zchar.qry
call sql -s -kf %1 < %DEMO%\zchar.qry > %2\zchar.out

echo Running %DEMO%\pnum_test.qry . . .
echo Output file: %2\pnum_test.out
echo sql -s -kf %1 ^< %DEMO%\pnum_test.qry
call sql -s -kf %1 < %DEMO%\pnum_test.qry > %2\pnum_test.out

echo Finished running all tests.
echo You may compare the results of your tests,
echo which are in the %2 directory, with the
echo canonical results in the %DEMO%\*.log files.

:ezend
set DEMO=
