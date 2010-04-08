@echo off
set ii_args=%1
:loop
shift
if "%1"=="" goto end
set ii_args=%ii_args% %1
goto loop
:end
echo on
@iiabf imageapp %ii_args%
@set ii_args=
