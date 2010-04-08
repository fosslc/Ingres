REM
REM	buildh.bat for front!tools!erxtract
REM
REM History:
REM	01-oct-93 (dianeh)
REM		Created.
REM	11-oct-93 (dianeh)
REM		Apparently, some messages start with S_.
REM	04-nov-93 (dianeh)
REM		Only pick up S_DU, not all S_'s.
REM	29-mar-95 (albany)
REM		Removed explicit setting of awk & sed symbols.  This
REM		is now picked up from CPE_BUILDENV_LOCALS.COM and
REM		CPE_BUILDENV_SYMS.LIST.  Fixed typo ( : -> . )
REM		Added check for pre-existing files to cut down on
REM		worthless build chatter.
REM	11-dec-1995 (canor01)
REM		Create buildh.bat for DESKTOP from buildh.com for VMS.
REM	03-apr-1996 (harpa06)
REM		Changed the reference from "fe_cat.msg" to "fecat.msg" since
REM 		this is the file that contains the FE application messages.
REM	08-jun-1999 (canor01)
REM		Changed messages.doc to messages.readme.
REM
REM
REM Build messages.readme
 echo "Building messages.readme ..."
 yapp -DDESKTOP -H## messages.readme >%ii_system%\ingres\files\english\messages\messages.readme
REM
REM Build messages.txt
REM Set up awk and sed temporary scripts
rem  create awk.input
echo /^^[EWI]_^|^^S_DU/ { print "" ; print ; print "" ; }>awk.input
echo /^^\/\*%%/,/^^\*\// { print ; }>>awk.input
rem create sed.input
echo s/User Action:/Recommendation:/>sed.input
echo s/Description:/Explanation:/>>sed.input
echo s:^^/\*%::>>sed.input
echo s:^^\*\*::>>sed.input
echo s:^^\*/::>>sed.input
REM
REM Delete the old messages.txt file
set MSGTXT=%ii_system%\ingres\files\english\messages\messages.txt
if exist %MSGTXT% del %MSGTXT%
REM 
REM Create buildh1.bat
echo awk -f awk.input %%ING_SRC%%\common\hdr\hdr\%%1 ^>awk.out>buildh1.bat
echo sed -f sed.input awk.out ^>^>%%MSGTXT%%>>buildh1.bat
echo del awk.out>>buildh1.bat
REM
REM Create the new messages.txt file
 echo "Building messages.txt ..."
set MSGFILES=eradf.msg eraif.msg erclf.msg erdmf.msg erduf.msg ergcf.msg erglf.msg ergwf.msg eropf.msg erpsf.msg erqef.msg erqsf.msg errdf.msg errqf.msg erscf.msg ersxf.msg ertpf.msg erulf.msg erusf.msg fecat.msg

for %%f in ( %MSGFILES% ) do call buildh1.bat %%f
del awk.input 
del sed.input
del buildh1.bat
set MSGTXT=
set MSGFILES=
