goto SKIP
REM @echo off

REM This copies all of the necessary support files into %II_SYSTEM% 
REM so that you can start OpenINGRES up successfuly during porting.
REM
REM This should be run after the mkidir batch file.
REM
REM History:
REM	26-may-95 (emmag)
REM	    File created.
REM	26-Aug-2004 (lakvi01)
REM	    Changed tools\port\bat to tools\port\bat_win.
REM	27-Oct-2004 (drivi01)
REM	    bat directories on windows were replaced with shell_win,
REM	    this script was updated to reflect that.
REM
@echo on
copy %ING_SRC%\front\frontcl\files\ansif.map    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\ansinf.map   %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\at386.map    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\frs.map      %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\pckermit.map %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\wy60at.map   %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\ibmpc.map    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\winpcalt.map %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\termcap      %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\abfurts.h    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\eqpname.h    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\eqsqlca.h    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\eqsqlda.h    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\frame2.h     %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\frame60.h    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\frame61.h    %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\iixa.h       %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\xa.h         %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\cbfhelp.dat  %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\cbfunits.dat %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\iiud.scr     %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\iiud64.scr   %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files\iiud65.scr   %II_SYSTEM%\ingres\files
copy %ING_SRC%\admin\install\dbtmpltbs\*        %II_SYSTEM%\ingres\dbtmplt

copy %ING_SRC%\front\frontcl\files_unix\startsql.dst %II_SYSTEM%\ingres\files\startsql
copy %ING_SRC%\front\frontcl\files_unix\startsql.dst %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files_unix\startup.dst  %II_SYSTEM%\ingres\files\startup
copy %ING_SRC%\front\frontcl\files_unix\startup.dst  %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\frontcl\files_unix\dayfile.dst  %II_SYSTEM%\ingres\files
copy %ING_SRC%\cl\clf\ut\utexe.def         %II_SYSTEM%\ingres\files
copy %ING_SRC%\cl\clf\ck\cktmpl.def        %II_SYSTEM%\ingres\files
copy %ING_SRC%\front\dict\dictfile\*       %II_SYSTEM%\ingres\files\dictfiles
copy %ING_SRC%\front\w4gl\dictfile\*       %II_SYSTEM%\ingres\files\dictfiles
copy %ING_SRC%\front\abf\abf\*.hlp         %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\abf\abfrt\*.hlp       %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\abf\fg\*.hlp          %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\abf\vq\*.hlp          %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\misc\ingcntrl\*.hlp   %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\qbf\qbf\*.hlp         %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\report\rbf\*.hlp      %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\report\report\*.hlp   %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\report\rfvifred\*.hlp %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\st\config\*.hlp       %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\st\ipm\*.hlp          %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\st\netutil\*.hlp      %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\st\specials\*.hlp     %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\tm\fstm\*.hlp         %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\tm\qr\*.hlp           %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\tools\erconvert\*.hlp %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\utils\oo\*.hlp        %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\utils\tblutil\*.hlp   %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\utils\uf\*.hlp        %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\front\vifred\vifred\*.hlp   %II_SYSTEM%\ingres\files\english
copy %ING_SRC%\common\hdr\hdr\generr.h     %II_SYSTEM%\ingres\files
REM copy %ING_SRC%\common\gcf\files\gcccset.nam  %II_SYSTEM%\ingres\files\charsets
REM copy %ING_SRC%\common\gcf\files\gcccset.xlt  %II_SYSTEM%\ingres\files\charsets
copy %ING_SRC%\common\gcf\files\chineset.xlt %II_SYSTEM%\ingres\files\charsets
copy %ING_SRC%\common\gcf\files\iiname.all   %II_SYSTEM%\ingres\files\name
copy %ING_SRC%\common\gcf\files\japanese.xlt %II_SYSTEM%\ingres\files\charsets

cd %ING_SRC%\front\st\shell_win
nmake -a

cd %ING_SRC%\front\frontcl\files
nmake -a

cd %ING_SRC%\front\st\crsfiles
nmake -a

cd %ING_SRC%\cl\clf\specials
nmake -a

call gd dictfile
nmake -a

call gd visitut
copy *.doc %II_SYSTEM%\ingres\files\tutorial
copy copy.in %II_SYSTEM%\ingres\files\tutorial

call gd tmgl
nmake -a

call gd gl\bin
nmake -a iizic.exe

call gd tmgl
nmake zonefiles

copy %ING_SRC%\front\st\starview\*.hlp %II_SYSTEM%\ingres\files\english

copy %ING_SRC%\front\abf\fg\*.tf %II_SYSTEM%\ingres\files\english

call gd errhelp
nmake -a

copy %ING_SRC%\sig\ima\ima\* %II_SYSTEM%\ingres\sig\ima

copy %ING_SRC%\sig\star\star\read.me %II_SYSTEM%\ingres\sig\star
copy %ING_SRC%\sig\star\star\iialarm.frm %II_SYSTEM%\ingres\sig\star

copy %ING_SRC%\front\utils\tblutil\tudefaults.hlp %II_SYSTEM%\ingres\files\english

call gd abfdemo
copy *.osq %II_SYSTEM%\ingres\files\abfdemo
copy *.hlp %II_SYSTEM%\ingres\files\abfdemo
copy *.dat %II_SYSTEM%\ingres\files\abfdemo
copy *.rw %II_SYSTEM%\ingres\files\abfdemo
copy *.frm %II_SYSTEM%\ingres\files\abfdemo
copy *.tmp %II_SYSTEM%\ingres\files\abfdemo
copy *.in %II_SYSTEM%\ingres\files\abfdemo
copy *.sc %II_SYSTEM%\ingres\files\abfdemo
copy *.sql %II_SYSTEM%\ingres\files\abfdemo
copy *.txt %II_SYSTEM%\ingres\files\abfdemo

:SKIP

copy %ING_SRC%\tools\port\resource\*.ico %II_SYSTEM%\ingres\bin

copy %ING_SRC%\front\abf\hdr\oslhdr.h %II_SYSTEM%\ingres\files

copy %ING_SRC%\front\tools\erxtract\messages.doc %II_SYSTEM%\ingres\files\english\messages

REM need to add this one later...
touch %II_SYSTEM%\ingres\files\english\messages\messages.txt

call gd adm
nmake -a

copy %ING_SRC%\front\frontcl\files_unix\vt* %II_SYSTEM%\ingres\files


cd %ING_SRC%\tools\port\shell_win
copy iquel.bat %II_SYSTEM%\ingres\bin
copy quel.bat %II_SYSTEM%\ingres\bin
copy isql.bat %II_SYSTEM%\ingres\bin
copy sql.bat %II_SYSTEM%\ingres\bin
copy accessdb.bat %II_SYSTEM%\ingres\bin
copy ingprsym.bat %II_SYSTEM%\ingres\utility
copy mkingusr.bat %II_SYSTEM%\ingres\bin
copy makestdcat.bat %II_SYSTEM%\ingres\sig\star
copy iirunschd.bat %II_SYSTEM%\ingres\sig\star
copy query.bat %II_SYSTEM%\ingres\bin
copy abfimage.bat %II_SYSTEM%\ingres\bin
copy catalogdb.bat %II_SYSTEM%\ingres\bin
copy abfdemo.bat %II_SYSTEM%\ingres\bin
copy vision.bat %II_SYSTEM%\ingres\bin
copy visiontutor.bat %II_SYSTEM%\ingres\utility
copy deltutor.bat %II_SYSTEM%\ingres\utility
copy deldemo.bat %II_SYSTEM%\ingres\bin
copy imageapp.bat %II_SYSTEM%\ingres\bin
