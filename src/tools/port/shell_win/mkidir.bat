@echo off
REM This is loosely based on the unix mkidir shell script. There might be some
REM unnecessary directories, but better safe than sorry!
REM
REM History:
REM 19-may-95 (emmag)
REM     File created, based on contents of idirs.ccpp.
REM	Added some PC character set directories.
REM 	TODO: Remove UNIX\VMS specific directories once identified.
REM 07-feb-96 (tutto01)
REM	Updated to reflect the current release of CA-OpenIngres NT.
REM 10-Jun-96 (somsa01)
REM     Added vdba directory.
REM 24-june-1996(angusm)
REM	Add various charsets, + sig!tz
REM 06-aug-96 (harpa06)
REM	Added "%II_SYSTEM%\ingres\ice," ice\spyglass and ice\tutorial directories.
REM 01-apr-97 (harpa06)
REM	Added "%II_SYSTEM%\ingres\ice\tutorials," 
REM	%II_SYSTEM%\ingres\ice\tutorials\dsql and
REM	%II_SYSTEM%\ingres\ice\tutorials\macro
REM	Removed "%II_SYSTEM%\ingres\ice\tutorial."
REM 24-feb-98 (mcgem01)
REM	Add two new simplified Chinese character set directories CSGB2312
REM	and CSGBK
REM 05-may-1998 (canor01)
REM     Quote pathnames to support embedded spaces.
REM 08-Oct-98 (fanra01)
REM     Add new ice directories.
REM 09-Oct-98 (fanra01)
REM     Add ice\bin directory
REM
REM 15-Oct-98 (cucjo01)
REM    Add ice\samples
REM    Add ice\samples\app
REM    Add ice\samples\dbproc
REM    Add ice\samples\report
REM    Add ice\samples\query
REM    Add ice\scripts
REM    Remove ice\examples
REM    Remove ice\examples\app
REM    Remove ice\examples\dbproc
REM    Remove ice\examples\report
REM    Remove ice\examples\query
REM 06-Nov-98 (fanra01)
REM     Add ice demo and tutorial
REM 25-Jan-2000 (fanra01)
REM    Sir 100122
REM    Add apache, netscape and microsoft bin directories.
REM 01-may-2000 (abbjo03)
REM	Add sig\syncapiw directory.
REM 23-jan-2001 (somsa01)
REM	Added Unicode character set directory.
REM 05-jun-2001 (mcgem01)
REM	Added the sig\auditmgr directory.
REM 18-sep-2001 (abbjo03)
REM	Add the files/charsets/is885915 and files/charsets/win1252 directories.
REM 25-Jun-2002 (fanra01)
REM     Sir 108122
REM     Add inst1252 for installation character mappings.
REM 10-jul-2002 (somsa01)
REM	Added sig\imp.
REM 28-jan-2004 (thoda04)
REM	Added dotnet/assembly directory for the Ingres .NET Data Provider.
REM  30-jan-2004 (drivi01)
REM         Added files/utmtemplates directory.
REM 20-jul-2004 (somsa01)
REM	Added files/ucharmaps.
REM 20-jul-2004 (somsa01)
REM	Added files/rep, sig/rpclean.
REM 10-Aug-2004 (kodse01)
REM	Added ice/DTD, ice/public
REM 26-Aug-2004 (kodse01)
REM 	Added english/ing30 and its subdirectories.
REM 01-Sep-2004 (kodse01)
REM 	Added VDBA intermediate target directories vdba/lib and vdba/app.
REM 08-oct-2004 (somsa01)
REM	Removed $II_SYSTEM/ingres/ice/tutorials structure, and spyglass.
REM 27-Oct-2004 (drivi01)
REM	Obtained unix directory II_SYSTEM/ingres/files/rcfiles from common
REM	subscribe list.
REM 16-Jan-2005 (drivi01)
REM	Added directories for other languages in the build area.
REM 20-jul-2005 (rigka01) bug 114739,INGNET173
REM	Added win1253,iso88597, and pc737
REM 02-Feb-2006 (drivi01)
REM	Documentation structure has changed, ing302 is replaced with
REM	ingres and indexes do not require as many directories.
REM 03-Feb-2006 (drivi01)
REM	Updated desintation directory for embeded index to lower
REM	case index instead of INDEX.
REM 04-May-2006 (drivi01)
REM	Create assembly\v1.1 directory.
REM 25-Sep-2006 (drivi01) SIR 116656
REM     Added new directory to Jamdefs for .Net data provider 2.0.
REM	Directory name will be assembly\v2.0.
REM 07-Nov-2006 (drivi01) SIR 116833
REM	Added new directories for csharp demo and demodb data directory.
REM 01-Jun-2007 (drivi01)
REM	Add sig/netu directory.
REM 20-Jun-2007 (thoda04)
REM	Add assembly\v2.1 directory.
REM 27-Aug-2008 (drivi01)
REM	Rename documentation index directory from ingres to ING_INDEX
REM	and remove a few other residual directories that are no longer 
REM	used.
REM 30-Oct-2009 (maspa05) b122744
REM     Added sig\inglogs
REM 18-Aug-2010 (drivi01)
REM	Remove %II_SYSTEM%\ingres\files\dictfile.
REM	Doesn't appear to be used.
REM
mkdir "%II_SYSTEM%"
mkdir "%II_SYSTEM%\ingres"
mkdir "%II_SYSTEM%\ingres\abf"
mkdir "%II_SYSTEM%\ingres\bin"
mkdir "%II_SYSTEM%\ingres\ckp"
mkdir "%II_SYSTEM%\ingres\ckp\default"
mkdir "%II_SYSTEM%\ingres\data"
mkdir "%II_SYSTEM%\ingres\data\default"
mkdir "%II_SYSTEM%\ingres\demo"
mkdir "%II_SYSTEM%\ingres\demo\api"
mkdir "%II_SYSTEM%\ingres\demo\api\asc"
mkdir "%II_SYSTEM%\ingres\demo\api\syc"
mkdir "%II_SYSTEM%\ingres\demo\cobol"
mkdir "%II_SYSTEM%\ingres\demo\csharp"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel\solution"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel\solution\Help"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel\solution\Properties"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel\solution\Resources"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel\app"
mkdir "%II_SYSTEM%\ingres\demo\csharp\travel\app\Help"
mkdir "%II_SYSTEM%\ingres\demo\data"
mkdir "%II_SYSTEM%\ingres\demo\esqlc"
mkdir "%II_SYSTEM%\ingres\demo\udadts"
mkdir "%II_SYSTEM%\ingres\dmp"
mkdir "%II_SYSTEM%\ingres\dmp\default"
mkdir "%II_SYSTEM%\ingres\dotnet"
mkdir "%II_SYSTEM%\ingres\dotnet\assembly"
mkdir "%II_SYSTEM%\ingres\dotnet\assembly\v1.1"
mkdir "%II_SYSTEM%\ingres\dotnet\assembly\v2.0"
mkdir "%II_SYSTEM%\ingres\dotnet\assembly\v2.1"
mkdir "%II_SYSTEM%\ingres\files"
mkdir "%II_SYSTEM%\ingres\files\abfdemo"
mkdir "%II_SYSTEM%\ingres\files\charsets"
mkdir "%II_SYSTEM%\ingres\files\charsets\alt"
mkdir "%II_SYSTEM%\ingres\files\charsets\arabic"
mkdir "%II_SYSTEM%\ingres\files\charsets\chineses"
mkdir "%II_SYSTEM%\ingres\files\charsets\csgb2312"
mkdir "%II_SYSTEM%\ingres\files\charsets\csgbk"
mkdir "%II_SYSTEM%\ingres\files\charsets\chineset"
mkdir "%II_SYSTEM%\ingres\files\charsets\chtbig5"
mkdir "%II_SYSTEM%\ingres\files\charsets\chteuc"
mkdir "%II_SYSTEM%\ingres\files\charsets\chthp"
mkdir "%II_SYSTEM%\ingres\files\charsets\cw"
mkdir "%II_SYSTEM%\ingres\files\charsets\decmulti"
mkdir "%II_SYSTEM%\ingres\files\charsets\dosasmo"
mkdir "%II_SYSTEM%\ingres\files\charsets\elot437"
mkdir "%II_SYSTEM%\ingres\files\charsets\greek"
mkdir "%II_SYSTEM%\ingres\files\charsets\hebrew"
mkdir "%II_SYSTEM%\ingres\files\charsets\hproman8"
mkdir "%II_SYSTEM%\ingres\files\charsets\ibmpc437"
mkdir "%II_SYSTEM%\ingres\files\charsets\ibmpc850"
mkdir "%II_SYSTEM%\ingres\files\charsets\ibmpc866"
mkdir "%II_SYSTEM%\ingres\files\charsets\inst1252"
mkdir "%II_SYSTEM%\ingres\files\charsets\is885915"
mkdir "%II_SYSTEM%\ingres\files\charsets\iso88591"
mkdir "%II_SYSTEM%\ingres\files\charsets\iso88592"
mkdir "%II_SYSTEM%\ingres\files\charsets\iso88595"
mkdir "%II_SYSTEM%\ingres\files\charsets\iso88597"
mkdir "%II_SYSTEM%\ingres\files\charsets\iso88599"
mkdir "%II_SYSTEM%\ingres\files\charsets\kanjieuc"
mkdir "%II_SYSTEM%\ingres\files\charsets\korean"
mkdir "%II_SYSTEM%\ingres\files\charsets\pc857"
mkdir "%II_SYSTEM%\ingres\files\charsets\pc737"
mkdir "%II_SYSTEM%\ingres\files\charsets\pchebrew"
mkdir "%II_SYSTEM%\ingres\files\charsets\shiftjis"
mkdir "%II_SYSTEM%\ingres\files\charsets\slav852"
mkdir "%II_SYSTEM%\ingres\files\charsets\thai"
mkdir "%II_SYSTEM%\ingres\files\charsets\utf8"
mkdir "%II_SYSTEM%\ingres\files\charsets\warabic"
mkdir "%II_SYSTEM%\ingres\files\charsets\whebrew"
mkdir "%II_SYSTEM%\ingres\files\charsets\win1250"
mkdir "%II_SYSTEM%\ingres\files\charsets\win1252"
mkdir "%II_SYSTEM%\ingres\files\charsets\win1253"
mkdir "%II_SYSTEM%\ingres\files\charsets\wthai"
mkdir "%II_SYSTEM%\ingres\files\collatio"
mkdir "%II_SYSTEM%\ingres\files\collation"
mkdir "%II_SYSTEM%\ingres\files\deu"
mkdir "%II_SYSTEM%\ingres\files\dictfiles"
mkdir "%II_SYSTEM%\ingres\files\dynamic"
mkdir "%II_SYSTEM%\ingres\files\english"
mkdir "%II_SYSTEM%\ingres\files\english\ING_INDEX"
mkdir "%II_SYSTEM%\ingres\files\english\messages"
mkdir "%II_SYSTEM%\ingres\files\esn"
mkdir "%II_SYSTEM%\ingres\files\fra"
mkdir "%II_SYSTEM%\ingres\files\ita"
mkdir "%II_SYSTEM%\ingres\files\jpn"
mkdir "%II_SYSTEM%\ingres\files\mdb"
mkdir "%II_SYSTEM%\ingres\files\memory"
mkdir "%II_SYSTEM%\ingres\files\name"
mkdir "%II_SYSTEM%\ingres\files\ptb"
mkdir "%II_SYSTEM%\ingres\files\rcfiles"
mkdir "%II_SYSTEM%\ingres\files\rep"
mkdir "%II_SYSTEM%\ingres\files\sch"
mkdir "%II_SYSTEM%\ingres\files\tutorial"
mkdir "%II_SYSTEM%\ingres\files\ucharmaps"
mkdir "%II_SYSTEM%\ingres\files\utmtemplates"
mkdir "%II_SYSTEM%\ingres\files\webdemo"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\asia"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\astrl"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\europe"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\gmt"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\mideast"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\na"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\sa"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\seasia"
mkdir "%II_SYSTEM%\ingres\files\zoneinfo\sp"
mkdir "%II_SYSTEM%\ingres\ice"
mkdir "%II_SYSTEM%\ingres\ice\bin"
mkdir "%II_SYSTEM%\ingres\ice\bin\apache"
mkdir "%II_SYSTEM%\ingres\ice\bin\netscape"
mkdir "%II_SYSTEM%\ingres\ice\bin\microsoft"
mkdir "%II_SYSTEM%\ingres\ice\ColdFusion"
mkdir "%II_SYSTEM%\ingres\ice\DTD"
mkdir "%II_SYSTEM%\ingres\ice\public"
mkdir "%II_SYSTEM%\ingres\ice\samples"
mkdir "%II_SYSTEM%\ingres\ice\samples\app"
mkdir "%II_SYSTEM%\ingres\ice\samples\dbproc"
mkdir "%II_SYSTEM%\ingres\ice\samples\report"
mkdir "%II_SYSTEM%\ingres\ice\samples\query"
mkdir "%II_SYSTEM%\ingres\ice\icetool"
mkdir "%II_SYSTEM%\ingres\ice\icetutor"
mkdir "%II_SYSTEM%\ingres\ice\images"
mkdir "%II_SYSTEM%\ingres\ice\plays"
mkdir "%II_SYSTEM%\ingres\ice\plays\mdb"
mkdir "%II_SYSTEM%\ingres\ice\plays\src"
mkdir "%II_SYSTEM%\ingres\ice\plays\tutorialGuide"
mkdir "%II_SYSTEM%\ingres\ice\plays\tutor"
mkdir "%II_SYSTEM%\ingres\ice\scripts"
mkdir "%II_SYSTEM%\ingres\jnl"
mkdir "%II_SYSTEM%\ingres\jnl\default"
mkdir "%II_SYSTEM%\ingres\lib"
mkdir "%II_SYSTEM%\ingres\log"
mkdir "%II_SYSTEM%\ingres\log\deleted"
mkdir "%II_SYSTEM%\ingres\sig"
mkdir "%II_SYSTEM%\ingres\sig\auditmgr"
mkdir "%II_SYSTEM%\ingres\sig\errhelp"
mkdir "%II_SYSTEM%\ingres\sig\ima"
mkdir "%II_SYSTEM%\ingres\sig\imp"
mkdir "%II_SYSTEM%\ingres\sig\netu"
mkdir "%II_SYSTEM%\ingres\sig\rpclean"
mkdir "%II_SYSTEM%\ingres\sig\star"
mkdir "%II_SYSTEM%\ingres\sig\syncapiw"
mkdir "%II_SYSTEM%\ingres\sig\tz"
mkdir "%II_SYSTEM%\ingres\sig\inglogs"
mkdir "%II_SYSTEM%\ingres\utility"
mkdir "%II_SYSTEM%\ingres\vdba"
mkdir "%II_SYSTEM%\ingres\work"
mkdir "%II_SYSTEM%\ingres\work\default"

REM VDBA intermediate target folders
if not exist %ING_SRC%\front\vdba goto EXIT
if not exist %ING_SRC%\front\vdba\App mkdir %ING_SRC%\front\vdba\App
if not exist %ING_SRC%\front\vdba\Lib mkdir %ING_SRC%\front\vdba\Lib

:EXIT
