@echo off
REM
REM fixnt.bat - patch up early installations which may have files
REM		located in the wrong places.
REM
REM	09-aug-95 (tutto01)
REM 		Created.
REM

echo.
echo.
echo fixnt.bat...
echo.
echo.
echo This program will relocate existing files in your installation to where
echo they actually belong (bin vs. utility).  Since most makefiles have now
echo been corrected, once you run this program, you should not need to run it
echo again.
echo.
echo This batch file should be used for any fixes of this nature, so feel free
echo to modify what it does.
echo.
pause
@echo on

REM Copying files from bin to utility ...

copy %II_SYSTEM%\ingres\bin\aducompile.exe	%II_SYSTEM%\ingres\utility\aducompile.exe
copy %II_SYSTEM%\ingres\bin\cbf.exe	%II_SYSTEM%\ingres\utility\cbf.exe
copy %II_SYSTEM%\ingres\bin\deltutor.bat	%II_SYSTEM%\ingres\utility\deltutor.bat
copy %II_SYSTEM%\ingres\bin\iigenres.exe	%II_SYSTEM%\ingres\utility\iigenres.exe
copy %II_SYSTEM%\ingres\bin\iigetenv.exe	%II_SYSTEM%\ingres\utility\iigetenv.exe
copy %II_SYSTEM%\ingres\bin\iigetres.exe	%II_SYSTEM%\ingres\utility\iigetres.exe
copy %II_SYSTEM%\ingres\bin\iimklog.exe	%II_SYSTEM%\ingres\utility\iimklog.exe
copy %II_SYSTEM%\ingres\bin\iipmhost.exe	%II_SYSTEM%\ingres\utility\iipmhost.exe
copy %II_SYSTEM%\ingres\bin\iiremres.exe	%II_SYSTEM%\ingres\utility\iiremres.exe
copy %II_SYSTEM%\ingres\bin\iiresutl.exe	%II_SYSTEM%\ingres\utility\iiresutl.exe
copy %II_SYSTEM%\ingres\bin\iirun.exe	%II_SYSTEM%\ingres\utility\iirun.exe
copy %II_SYSTEM%\ingres\bin\iirundbms.exe	%II_SYSTEM%\ingres\utility\iirundbms.exe
copy %II_SYSTEM%\ingres\bin\iisetres.exe	%II_SYSTEM%\ingres\utility\iisetres.exe
copy %II_SYSTEM%\ingres\bin\iisuabf.bat	%II_SYSTEM%\ingres\utility\iisuabf.bat
copy %II_SYSTEM%\ingres\bin\iisudbms.bat	%II_SYSTEM%\ingres\utility\iisudbms.bat
copy %II_SYSTEM%\ingres\bin\iisunet.bat	%II_SYSTEM%\ingres\utility\iisunet.bat
copy %II_SYSTEM%\ingres\bin\iisutm.bat	%II_SYSTEM%\ingres\utility\iisutm.bat
copy %II_SYSTEM%\ingres\bin\iivalres.exe	%II_SYSTEM%\ingres\utility\iivalres.exe
copy %II_SYSTEM%\ingres\bin\iizck.exe	%II_SYSTEM%\ingres\utility\iizck.exe
copy %II_SYSTEM%\ingres\bin\iizic.exe	%II_SYSTEM%\ingres\utility\iizic.exe
copy %II_SYSTEM%\ingres\bin\ingprsym.bat	%II_SYSTEM%\ingres\utility\ingprsym.bat
copy %II_SYSTEM%\ingres\bin\ingstart.exe	%II_SYSTEM%\ingres\utility\ingstart.exe
copy %II_SYSTEM%\ingres\bin\ingstop.exe	%II_SYSTEM%\ingres\utility\ingstop.exe
copy %II_SYSTEM%\ingres\bin\ipcclean.exe	%II_SYSTEM%\ingres\utility\ipcclean.exe
copy %II_SYSTEM%\ingres\bin\visiontutor.bat	%II_SYSTEM%\ingres\utility\visiontutor.bat

REM Deleting files from bin (that don't belong there) ...

del %II_SYSTEM%\ingres\bin\aducompile.exe
del %II_SYSTEM%\ingres\bin\cbf.exe
del %II_SYSTEM%\ingres\bin\deltutor.bat
del %II_SYSTEM%\ingres\bin\iigenres.exe
del %II_SYSTEM%\ingres\bin\iigetenv.exe
del %II_SYSTEM%\ingres\bin\iigetres.exe
del %II_SYSTEM%\ingres\bin\iimklog.exe
del %II_SYSTEM%\ingres\bin\iipmhost.exe
del %II_SYSTEM%\ingres\bin\iiremres.exe
del %II_SYSTEM%\ingres\bin\iiresutl.exe
del %II_SYSTEM%\ingres\bin\iirun.exe
del %II_SYSTEM%\ingres\bin\iirundbms.exe
del %II_SYSTEM%\ingres\bin\iisetres.exe
del %II_SYSTEM%\ingres\bin\iisuabf.bat
del %II_SYSTEM%\ingres\bin\iisudbms.bat
del %II_SYSTEM%\ingres\bin\iisunet.bat
del %II_SYSTEM%\ingres\bin\iisutm.bat
del %II_SYSTEM%\ingres\bin\iivalres.exe
del %II_SYSTEM%\ingres\bin\ingprsym.bat
del %II_SYSTEM%\ingres\bin\ingstart.exe
del %II_SYSTEM%\ingres\bin\ingstop.exe
del %II_SYSTEM%\ingres\bin\ipcclean.exe
del %II_SYSTEM%\ingres\bin\ipprod.exe
del %II_SYSTEM%\ingres\bin\iizic.exe
del %II_SYSTEM%\ingres\bin\iizck.exe
del %II_SYSTEM%\ingres\bin\visiontutor.bat

REM Copying files from utility to bin ...

copy %II_SYSTEM%\ingres\utility\abf.ico	%II_SYSTEM%\ingres\bin\abf.ico
copy %II_SYSTEM%\ingres\utility\abfdemo.bat	%II_SYSTEM%\ingres\bin\abfdemo.bat
copy %II_SYSTEM%\ingres\utility\abfimage.bat	%II_SYSTEM%\ingres\bin\abfimage.bat
copy %II_SYSTEM%\ingres\utility\accessdb.bat	%II_SYSTEM%\ingres\bin\accessdb.bat
copy %II_SYSTEM%\ingres\utility\ad102fin.exe	%II_SYSTEM%\ingres\bin\ad102fin.exe
copy %II_SYSTEM%\ingres\utility\ad102uin.exe	%II_SYSTEM%\ingres\bin\ad102uin.exe
copy %II_SYSTEM%\ingres\utility\ad103fin.exe	%II_SYSTEM%\ingres\bin\ad103fin.exe
copy %II_SYSTEM%\ingres\utility\ad103uin.exe	%II_SYSTEM%\ingres\bin\ad103uin.exe
copy %II_SYSTEM%\ingres\utility\ad104uin.exe	%II_SYSTEM%\ingres\bin\ad104uin.exe
copy %II_SYSTEM%\ingres\utility\ad301fin.exe	%II_SYSTEM%\ingres\bin\ad301fin.exe
copy %II_SYSTEM%\ingres\utility\alterdb.exe	%II_SYSTEM%\ingres\bin\alterdb.exe
copy %II_SYSTEM%\ingres\utility\auditdb.exe	%II_SYSTEM%\ingres\bin\auditdb.exe
copy %II_SYSTEM%\ingres\utility\cacheutil.exe	%II_SYSTEM%\ingres\bin\cacheutil.exe
copy %II_SYSTEM%\ingres\utility\caingres.ico	%II_SYSTEM%\ingres\bin\caingres.ico
copy %II_SYSTEM%\ingres\utility\catalogdb.bat	%II_SYSTEM%\ingres\bin\catalogdb.bat
copy %II_SYSTEM%\ingres\utility\chinst.exe	%II_SYSTEM%\ingres\bin\chinst.exe
copy %II_SYSTEM%\ingres\utility\ckcopyd.bat	%II_SYSTEM%\ingres\bin\ckcopyd.bat
copy %II_SYSTEM%\ingres\utility\ckcopyt.bat	%II_SYSTEM%\ingres\bin\ckcopyt.bat
copy %II_SYSTEM%\ingres\utility\ckpdb.exe	%II_SYSTEM%\ingres\bin\ckpdb.exe
copy %II_SYSTEM%\ingres\utility\compform.exe	%II_SYSTEM%\ingres\bin\compform.exe
copy %II_SYSTEM%\ingres\utility\copyapp.exe	%II_SYSTEM%\ingres\bin\copyapp.exe
copy %II_SYSTEM%\ingres\utility\copydb.exe	%II_SYSTEM%\ingres\bin\copydb.exe
copy %II_SYSTEM%\ingres\utility\copyform.exe	%II_SYSTEM%\ingres\bin\copyform.exe
copy %II_SYSTEM%\ingres\utility\copyrep.exe	%II_SYSTEM%\ingres\bin\copyrep.exe
copy %II_SYSTEM%\ingres\utility\cor02fin.exe	%II_SYSTEM%\ingres\bin\cor02fin.exe
copy %II_SYSTEM%\ingres\utility\cor02uin.exe	%II_SYSTEM%\ingres\bin\cor02uin.exe
copy %II_SYSTEM%\ingres\utility\createdb.exe	%II_SYSTEM%\ingres\bin\createdb.exe
copy %II_SYSTEM%\ingres\utility\dclgen.exe	%II_SYSTEM%\ingres\bin\dclgen.exe
copy %II_SYSTEM%\ingres\utility\deldemo.bat	%II_SYSTEM%\ingres\bin\deldemo.bat
copy %II_SYSTEM%\ingres\utility\destroydb.exe	%II_SYSTEM%\ingres\bin\destroydb.exe
copy %II_SYSTEM%\ingres\utility\dmfacp.exe	%II_SYSTEM%\ingres\bin\dmfacp.exe
copy %II_SYSTEM%\ingres\utility\dmfjsp.exe	%II_SYSTEM%\ingres\bin\dmfjsp.exe
copy %II_SYSTEM%\ingres\utility\dmfrcp.exe	%II_SYSTEM%\ingres\bin\dmfrcp.exe
copy %II_SYSTEM%\ingres\utility\eqc.exe	%II_SYSTEM%\ingres\bin\eqc.exe
copy %II_SYSTEM%\ingres\utility\ercomp.exe	%II_SYSTEM%\ingres\bin\ercomp.exe
copy %II_SYSTEM%\ingres\utility\esqlc.exe	%II_SYSTEM%\ingres\bin\esqlc.exe
copy %II_SYSTEM%\ingres\utility\fstm.exe	%II_SYSTEM%\ingres\bin\fstm.exe
copy %II_SYSTEM%\ingres\utility\iiabf.exe	%II_SYSTEM%\ingres\bin\iiabf.exe
copy %II_SYSTEM%\ingres\utility\iiabf1.bat	%II_SYSTEM%\ingres\bin\iiabf1.bat
copy %II_SYSTEM%\ingres\utility\iidbms.exe	%II_SYSTEM%\ingres\bin\iidbms.exe
copy %II_SYSTEM%\ingres\utility\iiexport.exe	%II_SYSTEM%\ingres\bin\iiexport.exe
copy %II_SYSTEM%\ingres\utility\iigcb.exe	%II_SYSTEM%\ingres\bin\iigcb.exe
copy %II_SYSTEM%\ingres\utility\iigcc.exe	%II_SYSTEM%\ingres\bin\iigcc.exe
copy %II_SYSTEM%\ingres\utility\iigcn.exe	%II_SYSTEM%\ingres\bin\iigcn.exe
copy %II_SYSTEM%\ingres\utility\iiimport.exe	%II_SYSTEM%\ingres\bin\iiimport.exe
copy %II_SYSTEM%\ingres\utility\iiinterp.exe	%II_SYSTEM%\ingres\bin\iiinterp.exe
copy %II_SYSTEM%\ingres\utility\iimonitor.exe	%II_SYSTEM%\ingres\bin\iimonitor.exe
copy %II_SYSTEM%\ingres\utility\iinamu.exe	%II_SYSTEM%\ingres\bin\iinamu.exe
copy %II_SYSTEM%\ingres\utility\iisustrt.bat	%II_SYSTEM%\ingres\bin\iisustrt.bat
copy %II_SYSTEM%\ingres\utility\iitouch.bat	%II_SYSTEM%\ingres\bin\iitouch.bat
copy %II_SYSTEM%\ingres\utility\imageapp.bat	%II_SYSTEM%\ingres\bin\imageapp.bat
copy %II_SYSTEM%\ingres\utility\infodb.exe	%II_SYSTEM%\ingres\bin\infodb.exe
copy %II_SYSTEM%\ingres\utility\ingcntrl.exe	%II_SYSTEM%\ingres\bin\ingcntrl.exe
copy %II_SYSTEM%\ingres\utility\ingmenu.exe	%II_SYSTEM%\ingres\bin\ingmenu.exe
copy %II_SYSTEM%\ingres\utility\ingprenv.exe	%II_SYSTEM%\ingres\bin\ingprenv.exe
copy %II_SYSTEM%\ingres\utility\ingsetenv.exe	%II_SYSTEM%\ingres\bin\ingsetenv.exe
copy %II_SYSTEM%\ingres\utility\ingunset.exe	%II_SYSTEM%\ingres\bin\ingunset.exe
copy %II_SYSTEM%\ingres\utility\ipchoice.exe	%II_SYSTEM%\ingres\bin\ipchoice.exe
copy %II_SYSTEM%\ingres\utility\ipm.exe	%II_SYSTEM%\ingres\bin\ipm.exe
copy %II_SYSTEM%\ingres\utility\ipprod.exe	%II_SYSTEM%\ingres\bin\ipprod.exe
copy %II_SYSTEM%\ingres\utility\ipreadp.exe	%II_SYSTEM%\ingres\bin\ipreadp.exe
copy %II_SYSTEM%\ingres\utility\ipset.bat	%II_SYSTEM%\ingres\bin\ipset.bat
copy %II_SYSTEM%\ingres\utility\ipsetp.exe	%II_SYSTEM%\ingres\bin\ipsetp.exe
copy %II_SYSTEM%\ingres\utility\ipwrite.exe	%II_SYSTEM%\ingres\bin\ipwrite.exe
copy %II_SYSTEM%\ingres\utility\iquel.bat	%II_SYSTEM%\ingres\bin\iquel.bat
copy %II_SYSTEM%\ingres\utility\isql.bat	%II_SYSTEM%\ingres\bin\isql.bat
copy %II_SYSTEM%\ingres\utility\lartool.exe	%II_SYSTEM%\ingres\bin\lartool.exe
copy %II_SYSTEM%\ingres\utility\lgkmemusg.exe	%II_SYSTEM%\ingres\bin\lgkmemusg.exe
copy %II_SYSTEM%\ingres\utility\lgtpoint.exe	%II_SYSTEM%\ingres\bin\lgtpoint.exe
copy %II_SYSTEM%\ingres\utility\lkevtst.exe	%II_SYSTEM%\ingres\bin\lkevtst.exe
copy %II_SYSTEM%\ingres\utility\lockstat.exe	%II_SYSTEM%\ingres\bin\lockstat.exe
copy %II_SYSTEM%\ingres\utility\logdump.exe	%II_SYSTEM%\ingres\bin\logdump.exe
copy %II_SYSTEM%\ingres\utility\logstat.exe	%II_SYSTEM%\ingres\bin\logstat.exe
copy %II_SYSTEM%\ingres\utility\mkingusr.bat	%II_SYSTEM%\ingres\bin\mkingusr.bat
copy %II_SYSTEM%\ingres\utility\modifyfe.exe	%II_SYSTEM%\ingres\bin\modifyfe.exe
copy %II_SYSTEM%\ingres\utility\msc01uin.exe	%II_SYSTEM%\ingres\bin\msc01uin.exe
copy %II_SYSTEM%\ingres\utility\msvcrt20.dll	%II_SYSTEM%\ingres\bin\msvcrt20.dll
copy %II_SYSTEM%\ingres\utility\nets.ico	%II_SYSTEM%\ingres\bin\nets.ico
copy %II_SYSTEM%\ingres\utility\netu.exe	%II_SYSTEM%\ingres\bin\netu.exe
copy %II_SYSTEM%\ingres\utility\netu.ico	%II_SYSTEM%\ingres\bin\netu.ico
copy %II_SYSTEM%\ingres\utility\netutil.exe	%II_SYSTEM%\ingres\bin\netutil.exe
copy %II_SYSTEM%\ingres\utility\opingsvc.exe	%II_SYSTEM%\ingres\bin\opingsvc.exe
copy %II_SYSTEM%\ingres\utility\optimizedb.exe	%II_SYSTEM%\ingres\bin\optimizedb.exe
copy %II_SYSTEM%\ingres\utility\osl.exe	%II_SYSTEM%\ingres\bin\osl.exe
copy %II_SYSTEM%\ingres\utility\oslsql.exe	%II_SYSTEM%\ingres\bin\oslsql.exe
copy %II_SYSTEM%\ingres\utility\pdm01uin.exe	%II_SYSTEM%\ingres\bin\pdm01uin.exe
copy %II_SYSTEM%\ingres\utility\printform.exe	%II_SYSTEM%\ingres\bin\printform.exe
copy %II_SYSTEM%\ingres\utility\qbf.exe	%II_SYSTEM%\ingres\bin\qbf.exe
copy %II_SYSTEM%\ingres\utility\qbf1.bat	%II_SYSTEM%\ingres\bin\qbf1.bat
copy %II_SYSTEM%\ingres\utility\quel.bat	%II_SYSTEM%\ingres\bin\quel.bat
copy %II_SYSTEM%\ingres\utility\query.bat	%II_SYSTEM%\ingres\bin\query.bat
copy %II_SYSTEM%\ingres\utility\rbf.exe	%II_SYSTEM%\ingres\bin\rbf.exe
copy %II_SYSTEM%\ingres\utility\rcpconfig.exe	%II_SYSTEM%\ingres\bin\rcpconfig.exe
copy %II_SYSTEM%\ingres\utility\rcpstat.exe	%II_SYSTEM%\ingres\bin\rcpstat.exe
copy %II_SYSTEM%\ingres\utility\relocatedb.exe	%II_SYSTEM%\ingres\bin\relocatedb.exe
copy %II_SYSTEM%\ingres\utility\report.exe	%II_SYSTEM%\ingres\bin\report.exe
copy %II_SYSTEM%\ingres\utility\rollforwarddb.exe	%II_SYSTEM%\ingres\bin\rollforwarddb.exe
copy %II_SYSTEM%\ingres\utility\server.ico	%II_SYSTEM%\ingres\bin\server.ico
copy %II_SYSTEM%\ingres\utility\servproc.exe	%II_SYSTEM%\ingres\bin\servproc.exe
copy %II_SYSTEM%\ingres\utility\setperm.exe	%II_SYSTEM%\ingres\bin\setperm.exe
copy %II_SYSTEM%\ingres\utility\sql.bat	%II_SYSTEM%\ingres\bin\sql.bat
copy %II_SYSTEM%\ingres\utility\sreport.exe	%II_SYSTEM%\ingres\bin\sreport.exe
copy %II_SYSTEM%\ingres\utility\start.ico	%II_SYSTEM%\ingres\bin\start.ico
copy %II_SYSTEM%\ingres\utility\starview.exe	%II_SYSTEM%\ingres\bin\starview.exe
copy %II_SYSTEM%\ingres\utility\statdump.exe	%II_SYSTEM%\ingres\bin\statdump.exe
copy %II_SYSTEM%\ingres\utility\stop.ico	%II_SYSTEM%\ingres\bin\stop.ico
copy %II_SYSTEM%\ingres\utility\sysmod.exe	%II_SYSTEM%\ingres\bin\sysmod.exe
copy %II_SYSTEM%\ingres\utility\tables.exe	%II_SYSTEM%\ingres\bin\tables.exe
copy %II_SYSTEM%\ingres\utility\tables1.bat	%II_SYSTEM%\ingres\bin\tables1.bat
copy %II_SYSTEM%\ingres\utility\tm.exe	%II_SYSTEM%\ingres\bin\tm.exe
copy %II_SYSTEM%\ingres\utility\unloaddb.exe	%II_SYSTEM%\ingres\bin\unloaddb.exe
copy %II_SYSTEM%\ingres\utility\upgradedb.exe	%II_SYSTEM%\ingres\bin\upgradedb.exe
copy %II_SYSTEM%\ingres\utility\upgradefe.exe	%II_SYSTEM%\ingres\bin\upgradefe.exe
copy %II_SYSTEM%\ingres\utility\vardlg.exe	%II_SYSTEM%\ingres\bin\vardlg.exe
copy %II_SYSTEM%\ingres\utility\verifydb.exe	%II_SYSTEM%\ingres\bin\verifydb.exe
copy %II_SYSTEM%\ingres\utility\vifred.exe	%II_SYSTEM%\ingres\bin\vifred.exe
copy %II_SYSTEM%\ingres\utility\vifred1.bat	%II_SYSTEM%\ingres\bin\vifred1.bat
copy %II_SYSTEM%\ingres\utility\vision.bat	%II_SYSTEM%\ingres\bin\vision.bat
copy %II_SYSTEM%\ingres\utility\wrapimon.exe	%II_SYSTEM%\ingres\bin\wrapimon.exe

REM Deleting files from utility (that don't belong there) ...

del %II_SYSTEM%\ingres\utility\abf.ico
del %II_SYSTEM%\ingres\utility\abfdemo.bat
del %II_SYSTEM%\ingres\utility\abfimage.bat
del %II_SYSTEM%\ingres\utility\accessdb.bat
del %II_SYSTEM%\ingres\utility\ad102fin.exe
del %II_SYSTEM%\ingres\utility\ad102uin.exe
del %II_SYSTEM%\ingres\utility\ad103fin.exe
del %II_SYSTEM%\ingres\utility\ad103uin.exe
del %II_SYSTEM%\ingres\utility\ad104uin.exe
del %II_SYSTEM%\ingres\utility\ad301fin.exe
del %II_SYSTEM%\ingres\utility\alterdb.exe
del %II_SYSTEM%\ingres\utility\auditdb.exe
del %II_SYSTEM%\ingres\utility\cacheutil.exe
del %II_SYSTEM%\ingres\utility\caingres.ico
del %II_SYSTEM%\ingres\utility\catalogdb.bat
del %II_SYSTEM%\ingres\utility\chinst.exe
del %II_SYSTEM%\ingres\utility\ckcopyd.bat
del %II_SYSTEM%\ingres\utility\ckcopyt.bat
del %II_SYSTEM%\ingres\utility\ckpdb.exe
del %II_SYSTEM%\ingres\utility\compform.exe
del %II_SYSTEM%\ingres\utility\copyapp.exe
del %II_SYSTEM%\ingres\utility\copydb.exe
del %II_SYSTEM%\ingres\utility\copyform.exe
del %II_SYSTEM%\ingres\utility\copyrep.exe
del %II_SYSTEM%\ingres\utility\cor02fin.exe
del %II_SYSTEM%\ingres\utility\cor02uin.exe
del %II_SYSTEM%\ingres\utility\createdb.exe
del %II_SYSTEM%\ingres\utility\dclgen.exe
del %II_SYSTEM%\ingres\utility\deldemo.bat
del %II_SYSTEM%\ingres\utility\destroydb.exe
del %II_SYSTEM%\ingres\utility\dmfacp.exe
del %II_SYSTEM%\ingres\utility\dmfjsp.exe
del %II_SYSTEM%\ingres\utility\dmfrcp.exe
del %II_SYSTEM%\ingres\utility\eqc.exe
del %II_SYSTEM%\ingres\utility\ercomp.exe
del %II_SYSTEM%\ingres\utility\esqlc.exe
del %II_SYSTEM%\ingres\utility\fstm.exe
del %II_SYSTEM%\ingres\utility\iiabf.exe
del %II_SYSTEM%\ingres\utility\iiabf1.bat
del %II_SYSTEM%\ingres\utility\iidbms.exe
del %II_SYSTEM%\ingres\utility\iiexport.exe
del %II_SYSTEM%\ingres\utility\iigcb.exe
del %II_SYSTEM%\ingres\utility\iigcc.exe
del %II_SYSTEM%\ingres\utility\iigcn.exe
del %II_SYSTEM%\ingres\utility\iiimport.exe
del %II_SYSTEM%\ingres\utility\iiinterp.exe
del %II_SYSTEM%\ingres\utility\iimonitor.exe
del %II_SYSTEM%\ingres\utility\iinamu.exe
del %II_SYSTEM%\ingres\utility\iisustrt.bat
del %II_SYSTEM%\ingres\utility\iitouch.bat
del %II_SYSTEM%\ingres\utility\imageapp.bat
del %II_SYSTEM%\ingres\utility\infodb.exe
del %II_SYSTEM%\ingres\utility\ingcntrl.exe
del %II_SYSTEM%\ingres\utility\ingmenu.exe
del %II_SYSTEM%\ingres\utility\ingprenv.exe
del %II_SYSTEM%\ingres\utility\ingsetenv.exe
del %II_SYSTEM%\ingres\utility\ingunset.exe
del %II_SYSTEM%\ingres\utility\ipchoice.exe
del %II_SYSTEM%\ingres\utility\ipm.exe
del %II_SYSTEM%\ingres\utility\ipprod.exe
del %II_SYSTEM%\ingres\utility\ipreadp.exe
del %II_SYSTEM%\ingres\utility\ipset.bat
del %II_SYSTEM%\ingres\utility\ipsetp.exe
del %II_SYSTEM%\ingres\utility\ipwrite.exe
del %II_SYSTEM%\ingres\utility\iquel.bat
del %II_SYSTEM%\ingres\utility\isql.bat
del %II_SYSTEM%\ingres\utility\lartool.exe
del %II_SYSTEM%\ingres\utility\lgkmemusg.exe
del %II_SYSTEM%\ingres\utility\lgtpoint.exe
del %II_SYSTEM%\ingres\utility\lkevtst.exe
del %II_SYSTEM%\ingres\utility\lockstat.exe
del %II_SYSTEM%\ingres\utility\logdump.exe
del %II_SYSTEM%\ingres\utility\logstat.exe
del %II_SYSTEM%\ingres\utility\mkingusr.bat
del %II_SYSTEM%\ingres\utility\modifyfe.exe
del %II_SYSTEM%\ingres\utility\msc01uin.exe
del %II_SYSTEM%\ingres\utility\msvcrt20.dll
del %II_SYSTEM%\ingres\utility\nets.ico
del %II_SYSTEM%\ingres\utility\netu.exe
del %II_SYSTEM%\ingres\utility\netu.ico
del %II_SYSTEM%\ingres\utility\netutil.exe
del %II_SYSTEM%\ingres\utility\opingsvc.exe
del %II_SYSTEM%\ingres\utility\optimizedb.exe
del %II_SYSTEM%\ingres\utility\osl.exe
del %II_SYSTEM%\ingres\utility\oslsql.exe
del %II_SYSTEM%\ingres\utility\pdm01uin.exe
del %II_SYSTEM%\ingres\utility\printform.exe
del %II_SYSTEM%\ingres\utility\qbf.exe
del %II_SYSTEM%\ingres\utility\qbf1.bat
del %II_SYSTEM%\ingres\utility\quel.bat
del %II_SYSTEM%\ingres\utility\query.bat
del %II_SYSTEM%\ingres\utility\rbf.exe
del %II_SYSTEM%\ingres\utility\rcpconfig.exe
del %II_SYSTEM%\ingres\utility\rcpstat.exe
del %II_SYSTEM%\ingres\utility\relocatedb.exe
del %II_SYSTEM%\ingres\utility\report.exe
del %II_SYSTEM%\ingres\utility\rollforwarddb.exe
del %II_SYSTEM%\ingres\utility\server.ico
del %II_SYSTEM%\ingres\utility\servproc.exe
del %II_SYSTEM%\ingres\utility\setperm.exe
del %II_SYSTEM%\ingres\utility\sql.bat
del %II_SYSTEM%\ingres\utility\sreport.exe
del %II_SYSTEM%\ingres\utility\start.ico
del %II_SYSTEM%\ingres\utility\starview.exe
del %II_SYSTEM%\ingres\utility\statdump.exe
del %II_SYSTEM%\ingres\utility\stop.ico
del %II_SYSTEM%\ingres\utility\sysmod.exe
del %II_SYSTEM%\ingres\utility\tables.exe
del %II_SYSTEM%\ingres\utility\tables1.bat
del %II_SYSTEM%\ingres\utility\tm.exe
del %II_SYSTEM%\ingres\utility\unloaddb.exe
del %II_SYSTEM%\ingres\utility\upgradedb.exe
del %II_SYSTEM%\ingres\utility\upgradefe.exe
del %II_SYSTEM%\ingres\utility\vardlg.exe
del %II_SYSTEM%\ingres\utility\verifydb.exe
del %II_SYSTEM%\ingres\utility\vifred.exe
del %II_SYSTEM%\ingres\utility\vifred1.bat
del %II_SYSTEM%\ingres\utility\vision.bat
del %II_SYSTEM%\ingres\utility\wrapimon.exe

@echo off
echo Your installation should now contain the right executables in the right 
echo places.
