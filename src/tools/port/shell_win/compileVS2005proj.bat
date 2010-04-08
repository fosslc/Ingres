@echo off
REM
REM  Name: compileVS2005proj.bat
REM
REM  Description:
REM	This module is responsible for building any projects that require Visual
REM	Studio 2005 compiler.
REM	Its purpose is to preserve environment for the rest of the build and only
REM	build selected projects with VS2005, while the rest of the code is built
REM	with Visual Studio .NET 2003.
REM	
REM
REM  History:
REM	25-sep-2006 (drivi01)
REM	    Created.
REM	08-Nov-2006 (drivi01)
REM	    This module was originally written to build .NET Data Provider 2.0
REM	    only.  Now, it's been employed to build any projects requiring
REM	    Visual Studio 2005 environment.
REM

set OLDPATH=%PATH%
set PATH=%VS2005SDK%\2006.04\VisualStudioIntegration\Common\Assemblies;%VS2005%\VC\PlatformSDK\Bin;%VS2005%\VC\Bin;%VS2005%\Common7\IDE;%VS2005%\Common7\Tools\bin;%VS2005%\SDK\v2.0\bin;%PATH%
echo PATH=%PATH%

set OLDINCLUDE=%INCLUDE%
set INCLUDE=%VS2005%\VC\include;%VS2005%\VC\atlmfc\include;%VS2005%\VC\PlatformSDK\Include;%VS2005%\SDK\v2.0\include;
echo INCLUDE=%INCLUDE%

set OLDLIB=%LIB%
set LIB=%VS2005%\SDK\v2.0\lib;%VS2005%\VC\PlatformSDK\Lib;%VS2005%\VC\lib;%VS2005%\VC\atlmfc\lib;%VS2005%\VC\PlatformSDK\Lib;%VS2005%\VC\lib;
echo LIB=%LIB%

call devenv %*
echo devenv %*

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%