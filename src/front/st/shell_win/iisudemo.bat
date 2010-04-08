@echo off
REM
@REM Copyright (c) 1995, 2007 Ingres Corporation
REM  
REM Name: iisudemo -- Set-up script for Ingres Demonstrations
REM
REM Usage:
REM     iisudemo
REM     
REM Description:
REM     Configuration script for demonstrations.
REM
REM History:
REM     01-Feb-2007 (fanra01)
REM         Bug 117620
REM         Created.
REM         Update the application configuration file to reflect the instance
REM         code.
REM	22-Feb-2007 (drivi01)
REM	    Add quotes around directory pathes.
setlocal

if "%INSTALL_DEBUG%" == "DEBUG" echo on

:csharp
call ipset INSTANCE_CODE ingprsym II_INSTALLATION
if "%INSTANCE_CODE%" == "II" goto endcsharp
"%II_SYSTEM%\ingres\bin\sif" "%II_SYSTEM%\ingres\demo\csharp\travel\app\AppConfigDataSet.xml" "<ParamValue>II</ParamValue>" "<ParamValue>%INSTANCE_CODE%</ParamValue>"
"%II_SYSTEM%\ingres\bin\sif" "%II_SYSTEM%\ingres\demo\csharp\travel\solution\AppConfigDataSet.xml" "<ParamValue>II</ParamValue>" "<ParamValue>%INSTANCE_CODE%</ParamValue>"
:endcsharp

goto end

:end
endlocal
