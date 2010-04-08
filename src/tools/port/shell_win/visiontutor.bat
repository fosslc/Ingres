@echo off
REM
REM visiontutor.bat - Creates a database and loads it with the sample data.
REM
if x%1==x echo Usage is %0 node::database
if x%1==x goto dun
echo Upgrading %1 (if necessary) . . .
upgradefe %1 ingres vision
echo Copying sample data . . .
copy %II_SYSTEM%\ingres\files\tutorial\*.doc
echo Loading VisionTutor Data to %1 . . .
sql %1 <%II_SYSTEM%\ingres\files\tutorial\copy.in >visiontu.lst
del *.sup
echo VisionTutor Data loaded to %1
echo Please examine the file VISIONTU.LST for any errors.
:dun
