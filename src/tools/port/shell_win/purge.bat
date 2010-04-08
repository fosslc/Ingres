@echo off
REM
REM  Name:  purge.bat
REM
REM  Description:
REM	Wipes out all files on client that are not read-only.  
REM	
REM  History:
REM	26-jan-96 (tutto01)
REM	    Created.
REM	16-feb-96 (emmag)
REM	    Use compression when getting files from piccolo for those
REM	    of us on the wrong side of the Atlantic.
REM	14-sep-98 (mcgem01)
REM	    Update the treasure directories.
REM	25-sep-98 (mcgem01)
REM	    The job of the purge script should be just to purge all
REM	    generated files and not to update the client etc.  That
REM	    is the task of bang.
REM

echo.
echo WARNING!!!!!   Prepare to purge your source tree and update your client!!
echo.
cd %ING_SRC%
echo.
echo Are we in the right place to wipe out non-read only files in source tree????
echo (WARNING : Any "working" files will be lost!)
echo.
pwd
echo.
echo The following files are currently open:
echo.
p working
echo.
echo (Press Ctrl-C to cancel)
pause
echo.
echo Are you sure?????    (Press Ctrl-C to cancel)
echo.
pause

if not exist VERS goto NEVER_MIND

del back /s/q
del cl /s/q
del common /s/q
del dbutil /s/q
del front /s/q
del gl /s/q
del sig /s/q
del tools /s/q

goto EXIT

:NEVER_MIND
echo.
echo You should place a VERS file in your %ING_SRC% directory.  It will
echo be copied into tools\port\conf and therefore will not be accidentally
echo deleted.  Re-run purge once you have done so.
echo.

:EXIT
