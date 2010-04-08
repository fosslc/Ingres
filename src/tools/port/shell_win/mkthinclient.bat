@echo off
REM
REM	Name: mkthinclient.bat
REM
REM	Description:
REM	     This batch file creates thinclient directory and copies selected binaries 
REM	     and files there to form a thin client.
REM
REM	History:
REM	     12-Jan-2005 (drivi01)
REM		Created.
REM	     27-Jan-2005 (drivi01)
REM	        Added missing files in files directory, added utility and temp directories
REM		as well as setclient.bat.
REM	     03-Feb-2005 (drivi01)
REM		Removed protect.dat from the list of files since it's a generated file.
REM	     03-Feb-2005 (drivi01)
REM		Add setclient.bat to the .caz file.
REM	    08-Aug-2005 (drivi01)
REM		Took out copying of chineset.xlt and japaneses.xlt b/c
REM	         	they're both included now in gcccset.xlt.
REM	    26-Oct-2005 (drivi01)
REM		Modified script to touch symbol.tbl in thinclient instead
REM		of copying an empty file b/c symbol.tbl is no longer created
REM		during build.
REM	    09-Nov-2005 (drivi01)
REM	        Added 3 additional charsets to the list included in thinclient.
REM	    03-Dec-2008 (drivi01)
REM		Modify build script for rolling the image to sign thinclient.
REM		Shared libraries are signed in II_SYSTEM\ingres\bin only
REM		and this script is being modified to copy shard libraries
REM	        from II_SYSTEM\ingres\bin instead of lib.


echo.
echo Creating thinclient, ingres, and bin directories.
echo.
if not exist %ING_ROOT%\thinclient mkdir %ING_ROOT%\thinclient   
if not exist %ING_ROOT%\thinclient\ingres mkdir %ING_ROOT%\thinclient\ingres   
if not exist %ING_ROOT%\thinclient\ingres\bin mkdir %ING_ROOT%\thinclient\ingres\bin  

echo.
echo Copying executable from %II_SYSTEM%/ingres/bin to thinclient.
echo.
copy  %II_SYSTEM%\ingres\bin\iigcc.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iigcn.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iigcstop.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iinamu.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ingprenv.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ingsetenv.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ingunset.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ingwrap.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ipsetp.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\sif.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ingprsym.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\ipcclean.exe  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying executables from %II_SYSTEM%/ingres/utility to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\temp mkdir %ING_ROOT%\thinclient\ingres\temp
if not exist %ING_ROOT%\thinclient\ingres\utility mkdir %ING_ROOT%\thinclient\ingres\utility

copy  %II_SYSTEM%\ingres\utility\syscheck.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iiconcat.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iigenres.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iigetenv.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iigetres.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iiingloc.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iipmhost.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iiremres.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iiresutl.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iirun.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iirundbms.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iisetres.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\iivalres.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\ingstart.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\utility\ingstop.exe  %ING_ROOT%\thinclient\ingres\utility   
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying shared libraries from %II_SYSTEM%/ingres/lib to thinclient.
echo.
copy  %II_SYSTEM%\ingres\bin\iilibadfdata.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibadf.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibapi.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibcompatdata.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibcompat.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibcuf.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibembeddata.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibembed.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibqdata.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibq.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibframedata.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibframe.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibgcfdata.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\bin\iilibgcf.dll  %ING_ROOT%\thinclient\ingres\bin   
if ERRORLEVEL 1 goto EXIT


echo.
echo Copying files from %II_SYSTEM%/ingres/files to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\files mkdir %ING_ROOT%\thinclient\ingres\files  
if ERRORLEVEL 1 goto EXIT

copy  %II_SYSTEM%\ingres\files\all.crs  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\das.crs  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\dayfile  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\dayfile.dst  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\default.rfm  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Eqdef.in  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Eqenv.qp  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Eqsqlca.a  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Eqsqlca.in  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Eqsqlcd.in  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Frs.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Ibmpc.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Iiud.scr  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Iiud64.scr  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Iiud65.scr  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Io8256.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Ioalsys.a  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Line1.gr  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\M30n.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\M91e.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mac2.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws00.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws01.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws02.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws03.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws04.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws05.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Mws06.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\net.crs  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\net.rfm  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\oipfctrs.reg  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Pc-220.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Pc-305.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Pc-386.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Pckermit.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Pt35.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Pt35t.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Scat1.gr  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\startsql  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\startsql.dst  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\startup  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\startup.dst  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Suntype5.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
touch  %ING_ROOT%\thinclient\ingres\files\symbol.tbl   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\termcap  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Tk4105.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\tng.crs  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\tng.rfm  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\tz.crs  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Vt100f.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Vt100i.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Vt100nk.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Vt200i.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Vt220.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Vt220ak.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Winpcalt.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Wy60at.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\Xmlname.map  %ING_ROOT%\thinclient\ingres\files   
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying charsets to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\files\charsets mkdir %ING_ROOT%\thinclient\ingres\files\charsets  
if ERRORLEVEL 1 goto EXIT

copy  %II_SYSTEM%\ingres\files\charsets\gcccset.nam  %ING_ROOT%\thinclient\ingres\files\charsets   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\charsets\gcccset.xlt  %ING_ROOT%\thinclient\ingres\files\charsets   
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying ucharmaps to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\files\ucharmaps mkdir %ING_ROOT%\thinclient\ingres\files\ucharmaps
if ERRORLEVEL 1 goto EXIT

copy %II_SYSTEM%\ingres\files\ucharmaps\aliasmaptbl  %ING_ROOT%\thinclient\ingres\files\ucharmaps
if ERRORLEVEL 1 goto EXIT
copy %II_SYSTEM%\ingres\files\ucharmaps\ca-iso8859-1-latin1-2004  %ING_ROOT%\thinclient\ingres\files\ucharmaps
if ERRORLEVEL 1 goto EXIT
copy %II_SYSTEM%\ingres\files\ucharmaps\ca-win1252-latin1-2004  %ING_ROOT%\thinclient\ingres\files\ucharmaps
if ERRORLEVEL 1 goto EXIT
copy %II_SYSTEM%\ingres\files\ucharmaps\default  %ING_ROOT%\thinclient\ingres\files\ucharmaps
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\alt mkdir %ING_ROOT%\thinclient\ingres\files\charsets\alt
copy  %II_SYSTEM%\ingres\files\charsets\alt\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\alt   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\arabic mkdir %ING_ROOT%\thinclient\ingres\files\charsets\arabic
copy  %II_SYSTEM%\ingres\files\charsets\arabic\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\arabic   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\chineses mkdir %ING_ROOT%\thinclient\ingres\files\charsets\chineses
copy  %II_SYSTEM%\ingres\files\charsets\chineses\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\chineses   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\chineset mkdir %ING_ROOT%\thinclient\ingres\files\charsets\chineset
copy  %II_SYSTEM%\ingres\files\charsets\chineset\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\chineset   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\chtbig5 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\chtbig5
copy  %II_SYSTEM%\ingres\files\charsets\chtbig5\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\chtbig5   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\chteuc mkdir %ING_ROOT%\thinclient\ingres\files\charsets\chteuc
copy  %II_SYSTEM%\ingres\files\charsets\chteuc\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\chteuc   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\chthp mkdir %ING_ROOT%\thinclient\ingres\files\charsets\chthp
copy  %II_SYSTEM%\ingres\files\charsets\chthp\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\chthp   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\csgb2312 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\csgb2312
copy  %II_SYSTEM%\ingres\files\charsets\csgb2312\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\csgb2312   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\csgbk mkdir %ING_ROOT%\thinclient\ingres\files\charsets\csgbk
copy  %II_SYSTEM%\ingres\files\charsets\csgbk\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\csgbk   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\cw mkdir %ING_ROOT%\thinclient\ingres\files\charsets\cw
copy  %II_SYSTEM%\ingres\files\charsets\cw\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\cw   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\decmulti mkdir %ING_ROOT%\thinclient\ingres\files\charsets\decmulti
copy  %II_SYSTEM%\ingres\files\charsets\decmulti\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\decmulti   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\dosasmo mkdir %ING_ROOT%\thinclient\ingres\files\charsets\dosasmo
copy  %II_SYSTEM%\ingres\files\charsets\dosasmo\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\dosasmo   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\elot437 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\elot437
copy  %II_SYSTEM%\ingres\files\charsets\elot437\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\elot437   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\greek mkdir %ING_ROOT%\thinclient\ingres\files\charsets\greek
copy  %II_SYSTEM%\ingres\files\charsets\greek\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\greek   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\hebrew mkdir %ING_ROOT%\thinclient\ingres\files\charsets\hebrew
copy  %II_SYSTEM%\ingres\files\charsets\hebrew\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\hebrew   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\hproman8 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\hproman8
copy  %II_SYSTEM%\ingres\files\charsets\hproman8\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\hproman8   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc437 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc437
copy  %II_SYSTEM%\ingres\files\charsets\ibmpc437\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc437   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc850 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc850
copy  %II_SYSTEM%\ingres\files\charsets\ibmpc850\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc850   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc866 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc866  
copy  %II_SYSTEM%\ingres\files\charsets\ibmpc866\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\ibmpc866   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\is885915 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\is885915  
copy  %II_SYSTEM%\ingres\files\charsets\is885915\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\is885915  
if ERRORLEVEL 1 goto EXIT
 
if not exist %ING_ROOT%\thinclient\ingres\files\charsets\iso88591 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\iso88591  
copy  %II_SYSTEM%\ingres\files\charsets\iso88591\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\iso88591   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\iso88592 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\iso88592  
copy  %II_SYSTEM%\ingres\files\charsets\iso88592\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\iso88592   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\iso88595 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\iso88595  
copy  %II_SYSTEM%\ingres\files\charsets\iso88595\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\iso88595   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\iso88597 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\iso88597  
copy  %II_SYSTEM%\ingres\files\charsets\iso88597\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\iso88597   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\iso88599 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\iso88599  
copy  %II_SYSTEM%\ingres\files\charsets\iso88599\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\iso88599   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\kanjieuc mkdir %ING_ROOT%\thinclient\ingres\files\charsets\kanjieuc  
copy  %II_SYSTEM%\ingres\files\charsets\kanjieuc\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\kanjieuc   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\korean mkdir %ING_ROOT%\thinclient\ingres\files\charsets\korean  
copy  %II_SYSTEM%\ingres\files\charsets\korean\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\korean   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\pc737 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\pc737
copy  %II_SYSTEM%\ingres\files\charsets\pc737\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\pc737
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\pc857 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\pc857  
copy  %II_SYSTEM%\ingres\files\charsets\pc857\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\pc857   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\pchebrew mkdir %ING_ROOT%\thinclient\ingres\files\charsets\pchebrew  
copy  %II_SYSTEM%\ingres\files\charsets\pchebrew\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\pchebrew   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\shiftjis mkdir %ING_ROOT%\thinclient\ingres\files\charsets\shiftjis  
copy  %II_SYSTEM%\ingres\files\charsets\shiftjis\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\shiftjis   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\slav852 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\slav852  
copy  %II_SYSTEM%\ingres\files\charsets\slav852\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\slav852   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\thai mkdir %ING_ROOT%\thinclient\ingres\files\charsets\thai  
copy  %II_SYSTEM%\ingres\files\charsets\thai\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\thai   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\utf8 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\utf8  
copy  %II_SYSTEM%\ingres\files\charsets\utf8\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\utf8   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\warabic mkdir %ING_ROOT%\thinclient\ingres\files\charsets\warabic  
copy  %II_SYSTEM%\ingres\files\charsets\warabic\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\warabic   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\whebrew mkdir %ING_ROOT%\thinclient\ingres\files\charsets\whebrew  
copy  %II_SYSTEM%\ingres\files\charsets\whebrew\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\whebrew   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\win1250 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\win1250  
copy  %II_SYSTEM%\ingres\files\charsets\win1250\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\win1250   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\win1252 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\win1252  
copy  %II_SYSTEM%\ingres\files\charsets\win1252\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\win1252   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\win1253 mkdir %ING_ROOT%\thinclient\ingres\files\charsets\win1253  
copy  %II_SYSTEM%\ingres\files\charsets\win1253\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\win1253   
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\charsets\wthai mkdir %ING_ROOT%\thinclient\ingres\files\charsets\wthai  
copy  %II_SYSTEM%\ingres\files\charsets\wthai\*.*  %ING_ROOT%\thinclient\ingres\files\charsets\wthai   
if ERRORLEVEL 1 goto EXIT


echo.
echo Copying message files to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\files\english mkdir %ING_ROOT%\thinclient\ingres\files\english  

copy  %II_SYSTEM%\ingres\files\english\fast_v4.mnx  %ING_ROOT%\thinclient\ingres\files\english   
if ERRORLEVEL 1 goto EXIT
copy  %II_SYSTEM%\ingres\files\english\slow_v4.mnx  %ING_ROOT%\thinclient\ingres\files\english   
if ERRORLEVEL 1 goto EXIT

echo.
echo Copying name file to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\files\name mkdir %ING_ROOT%\thinclient\ingres\files\name  
copy  %II_SYSTEM%\ingres\files\name\iiname.all  %ING_ROOT%\thinclient\ingres\files\name  
if ERRORLEVEL 1 goto EXIT 


echo.
echo Copying zoneinfo to thinclient.
echo.
if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo  

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\asia mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\asia
copy %II_SYSTEM%\ingres\files\zoneinfo\asia\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\asia
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\astrl mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\astrl
copy %II_SYSTEM%\ingres\files\zoneinfo\astrl\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\astrl
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\europe mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\europe
copy %II_SYSTEM%\ingres\files\zoneinfo\europe\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\europe
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\gmt mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\gmt
copy %II_SYSTEM%\ingres\files\zoneinfo\gmt\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\gmt
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\mideast mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\mideast
copy %II_SYSTEM%\ingres\files\zoneinfo\mideast\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\mideast
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\na mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\na
copy %II_SYSTEM%\ingres\files\zoneinfo\na\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\na
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\sa mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\sa
copy %II_SYSTEM%\ingres\files\zoneinfo\sa\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\sa
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\seasia mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\seasia
copy %II_SYSTEM%\ingres\files\zoneinfo\seasia\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\seasia
if ERRORLEVEL 1 goto EXIT

if not exist %ING_ROOT%\thinclient\ingres\files\zoneinfo\sp mkdir %ING_ROOT%\thinclient\ingres\files\zoneinfo\sp
copy %II_SYSTEM%\ingres\files\zoneinfo\sp\* %ING_ROOT%\thinclient\ingres\files\zoneinfo\sp
if ERRORLEVEL 1 goto EXIT


echo.
echo Copy setclient.bat to thinclient.
echo.
copy %II_SYSTEM%\ingres\bin\setclient.bat %ING_ROOT%\thinclient
if ERRORLEVEL 1 goto EXIT
goto DONE


 
:DONE
echo.
echo Making API_int_w32.caz file.
echo.
cd /d %ING_ROOT%/thinclient
rm API_int_w32.caz
cazipxp -rw ingres setclient.bat API_int_w32.caz
cazipxp -l API_int_w32.caz>thinclient.lis
cd /d %ING_SRC%

echo.
echo Thinclient completed successfully!!!
echo.
goto EXITSUCCESS

:EXIT
cd /d %ING_ROOT%/thinclient
echo cazipxp -l API_int_w32.caz > thinclient.lis
echo ERROR: There was one or more errors copying files to thinclient. >> thinclient.lis
echo ERROR: There was one or more errors copying files to thinclient. > error.log
cd /d %ING_SRC%
echo ERROR: There was one or more errors copying files to thinclient.
endlocal

:EXITSUCCESS
endlocal
