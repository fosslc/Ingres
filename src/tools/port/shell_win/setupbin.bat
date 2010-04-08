@echo off
REM This copies all of the necessary executables and bactch files into 
REM %II_SYSTEM%\INGRES\BIN so that you can start OpenINGRES up 
REM successfuly during porting.
REM
REM This should be run after the mkidir and iisetup batch files.
REM
REM History:
REM	29-may-95 (emmag)
REM	    File created.
REM	26-Aug-2004 (lakvi01)
REM	    Modified it to reflect the new directory structure.
REM	27-Oct-2004 (drivi01)
REM	    bat directories on windows were replaced with shell_win,
REM	    this script was updated to reflect that.
REM
@echo on
copy %ING_SRC%\tools\port\shell_win\accessdb.bat   %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\alterdb.bat    %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\auditdb.bat    %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\catalogdb.bat  %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\ckcopyd.bat    %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\ckcopyt.bat    %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\ckpdb.bat      %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\infodb.bat     %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\mkingusr.bat   %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\infodb.bat     %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\quel.bat       %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\rolldb.bat     %II_SYSTEM%\ingres\bin
copy %ING_SRC%\tools\port\shell_win\sql.bat        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\cl\bin\ingsetenv.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\cl\bin\ingunset.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\cl\bin\ingprenv.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\cl\bin\ercomp.exe             %II_SYSTEM%\ingres\bin
copy %ING_SRC%\common\bin\iigcc.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\common\bin\iigcn.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\common\bin\iigcb.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\common\bin\iinamu.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\common\bin\netu.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\common\bin\aducompile.exe     %II_SYSTEM%\ingres\bin
copy %ING_SRC%\dbutil\bin\createdb.exe       %II_SYSTEM%\ingres\bin
copy %ING_SRC%\dbutil\bin\destroydb.exe      %II_SYSTEM%\ingres\bin
copy %ING_SRC%\dbutil\bin\sysmod.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\dbutil\bin\upgradedb.exe      %II_SYSTEM%\ingres\bin
copy %ING_SRC%\dbutil\bin\createdb.exe       %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\cacheutil.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\dmfacp.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\dmfjsp.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\iidbms.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\iimonitor.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\lartool.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\lgkmemusg.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\lgtpoint.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\lkevtst.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\lockstat.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\logstat.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\optimizedb.exe       %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\rcpconfig.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\rcpstat.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\back\bin\statdump.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\chinst.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\compform.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\config.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\copyapp.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\copydb.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\copyrep.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\eqc.exe             %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\eqmerge.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\esqlc.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\fstm.exe            %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iigenres.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iipmhost.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iiremres.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iiresutl.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iisetres.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iivalres.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\ingcntrl.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\ingmenu.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\ingstart.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\ingstop.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\iyacc.exe           %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\qbf.exe             %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\rbf.exe             %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\report.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\sreport.exe         %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\tables.exe          %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\tm.exe              %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\unloaddb.exe        %II_SYSTEM%\ingres\bin
copy %ING_SRC%\front\bin\vifred.exe          %II_SYSTEM%\ingres\bin
