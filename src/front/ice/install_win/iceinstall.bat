@echo off
REM Name:   iceinstall.bat
REM
REM Description:
REM     Setup script for the
REM         ice admin tool
REM         ice tutorial
REM         ice samples
REM         ice demo

ingunset ING_SET_ICETUTOR

REM Setup tutorial users
sql icesvr < "%II_SYSTEM%\ingres\ice\scripts\iceuser.sql"

REM Change the backslash character in II_SYSTEM to forward slash
echo set II_WEB_SYSTEM="%II_SYSTEM%">"%II_TEMPORARY%\iiwebsys.bat"
sif "%II_TEMPORARY%\iiwebsys.bat" \ /
call "%II_TEMPORARY%\iiwebsys.bat"
del "%II_TEMPORARY%\iiwebsys.bat"

echo Installing ICE Administration Tool
sif "%II_SYSTEM%\ingres\ice\scripts\icetool.sql" [II_WEB_SYSTEM] %II_WEB_SYSTEM%
sql icesvr < "%II_SYSTEM%\ingres\ice\scripts\icetool.sql"
"%II_SYSTEM%\ingres\bin\makeid" 1095 1146 id ice_files icesvr
call "%II_SYSTEM%\ingres\bin\iceload.bat" icetool ice_files doc icesvr

echo Installing ICE Tutorial
sif "%II_SYSTEM%\ingres\ice\scripts\icetutor.sql" [II_WEB_SYSTEM] %II_WEB_SYSTEM%
sql icesvr < "%II_SYSTEM%\ingres\ice\scripts\icetutor.sql"
"%II_SYSTEM%\ingres\bin\makeid" 2079 2098 id ice_files icesvr
call "%II_SYSTEM%\ingres\bin\iceload.bat" icetutor ice_files doc icesvr

echo Installing ICE samples
sql icesvr < "%II_SYSTEM%\ingres\ice\scripts\icesample.sql"
sreport -s -uicedbuser icetutor "%II_SYSTEM%\ingres\ice\samples\report\existing.rw"
sreport -s -uicedbuser icetutor "%II_SYSTEM%\ingres\ice\samples\report\htmlrbf.rw"
sreport -s -uicedbuser icetutor "%II_SYSTEM%\ingres\ice\samples\report\htmlwrite.rw"
sreport -s -uicedbuser icetutor "%II_SYSTEM%\ingres\ice\samples\report\htmlwvar.rw"

sql -uicedbuser icetutor < "%II_SYSTEM%\ingres\ice\samples\dbproc\proc.sql"

echo Installing ICE demonstration
REM The icetutor database contains the tables for the plays demo.
REM The DBMS parameters for DMF cache size should have 4096 byte pages enabled
REM and the default page size should also be set to 4096 bytes.

sif "%II_SYSTEM%\ingres\ice\scripts\iceplays.sql" [II_WEB_SYSTEM] %II_WEB_SYSTEM%
sql icesvr < "%II_SYSTEM%\ingres\ice\scripts\iceplays.sql"
sql -uicedbuser icetutor < "%II_SYSTEM%\ingres\ice\scripts\icedemo.sql"
"%II_SYSTEM%\ingres\bin\makeid" 24 31 id ice_files icesvr
call "%II_SYSTEM%\ingres\bin\iceload.bat" plays ice_files doc icesvr
REM Set locking mode on application table
ingsetenv ING_SET_ICETUTOR "set lockmode on play_order where level=row, readlock=nolock"
