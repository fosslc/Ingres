@echo off
setlocal
REM
REM  Name: tartools.bat
REM
REM  Description:
REM	Similiar to UNIX's tartools, it checks if all of the testing tools
REM	are in place.
REM
REM  History:
REM	21-oct-2004 (somsa01)
REM	    Created.
REM

REM
REM Only support a "-a" option for now
REM

if not "%1" == "-a" goto DONE

if exist %ING_SRC%\tartools_report.txt del %ING_SRC%\tartools_report.txt


REM
REM Set static list of tools we will need
REM

SET FILES_LIST=basickeys.sep commands.sep header.txt ibmpc.sep makelocs.key septerm.sep termcap.sep vt100.sep vt220.sep winpcalt.sep

SET MAN1_LIST=altcompare.1 clningtst.1 clntstdbs.1 sep.1 sepperuse.1 tac.1 updperuse.1

SET BIN1_LIST=PCdate.exe PCecho.exe PCread.exe PCwhich.exe achilles.exe acstart.exe altcompare.bat authnetu.bat basename.exe ckpdb1_check.exe ckpdb1_load.exe ckpdb_abort.exe ckpdb_append.exe ckpdb_delapp.exe ckpdbtst.bat cksepenv.bat clningtst.bat clntstdbs.bat cts.bat ctscoord.exe ctsslave.exe dirname.exe ercompile.exe executor.exe ferun.bat getseed.exe icetest.exe

SET BIN2_LIST=iyacc.exe lck1.bat lck2.bat lck3.bat listexec.exe makelocs.bat msfcchk.exe msfccldb.exe msfcclup.exe msfccrea.exe msfcdrv.exe msfcsync.exe msfctest.exe mts.bat mtscoord.exe mtsslave.exe netutilcmd.bat numtoexit.exe peditor.exe qasetuser.exe qasetuserfe.exe qasetusertm.exe qawtl.exe readB.exe readlog.exe rollchk.exe rollcldb.exe rollclup.exe rollcrea.exe rolldrv.exe rollsync.exe

SET BIN3_LIST=rolltest.exe run.bat sep.exe sepcc.bat sepchild.exe sepesqlc.bat seplib.bat seplnk.bat sepperuse.bat sleep.exe tac.exe updperuse.bat


REM
REM Let's see if they exist
REM

for %%i in (%FILES_LIST%) do if not exist %ING_TOOLS%\files\%%i echo %ING_TOOLS%\files\%%i >> %ING_SRC%\tartools_report.txt

for %%i in (%MAN1_LIST%) do if not exist %ING_TOOLS%\man1\%%i echo %ING_TOOLS%\man1\%%i >> %ING_SRC%\tartools_report.txt

for %%i in (%BIN1_LIST%) do if not exist %ING_TOOLS%\bin\%%i echo %ING_TOOLS%\bin\%%i >> %ING_SRC%\tartools_report.txt
for %%i in (%BIN2_LIST%) do if not exist %ING_TOOLS%\bin\%%i echo %ING_TOOLS%\bin\%%i >> %ING_SRC%\tartools_report.txt
for %%i in (%BIN3_LIST%) do if not exist %ING_TOOLS%\bin\%%i echo %ING_TOOLS%\bin\%%i >> %ING_SRC%\tartools_report.txt


if not exist %ING_SRC%\tartools_report.txt goto DONE
echo There are some test tools missing:
echo.
type %ING_SRC%\tartools_report.txt
echo.
echo A log of these missing files has been saved in %ING_SRC%\tartools_report.txt

:DONE
endlocal
