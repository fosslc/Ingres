:
#
# Copyright (c) 2004 Ingres Corporation
#
# Build INGRES from scratch
#
## History
##	28-apr-93 (dianeh)
##		Created.
##	30-apr-93 (dianeh)
##		Removed a bogus line that I meant to remove before
##		I submitted this.
##	3-May-93 (tomm)
##		Make csphil checking optional.
##	3-May-93 (tomm)
##		Ooops, -eq should be = for string testing. Changed it.
##	04-may-93 (dianeh)
##		Avoid adding even more flags -- just make csphil a
##		warning rather than an exit 1; do the same for the
##		(commented-out for now) cltest block; correct references
##		to command name; add some missing echo's.
##	11-may-93 (vijay)
##		Add mkshlibs.
##	25-may-93 (johnst)
##		Correct "versfile=$ING_SRC/tools/port/conf$vers_src/VERS" to
##		"versfile=$ING_SRC/tools/port$vers_src/conf/VERS" to get 
##              correct path for BAROQUE environment.
##	29-jun-93 (dianeh)
##		Change the way headers get built in common/hdr/hdr to reflect
##		the change in the targets list I made in the MINGS file there.
##      16-jun-93 (johnst)
##              Fix front end headers build for BAROQUE environment:
##		- cd to $ING_SRC/front, not $ING_SRC/front$vers_src, as
##		  there's no such place.
##	27-jul-93 (dianeh)
##		Changed where the manifest file lives, and added a mk;
##		changed the name of the manifest file to release.dat) and
##		removed the link (handled by the mk); uncommented cltest.
##	03-aug-93 (dianeh)
##		Added new megl directory; need gc and id before ercompile;
##		refinements in build order; etc.
##	19-sep-93 (peterk)
##		add $MINGFLAGS to set ming flags from environment
##		e.g., -j# flag; fix make of front/embed/hdrs for baroque;
##		use +j to turn off parallel builds where necessary
##		(primarily directories with grammars);
##		do front facility hdrs directly for baroque
##		do non-grammar libs in front/embed without +j
##	23-sep-93 (dianeh)
##		New glf/mu modules for ercompile; eqdef.h for eqc/esqlc.
##	12-oct-93 (peterk)
##		Changes to mk.sh and mkming.sh for baroque simplify running
##		mk at group level - group MING file includes facility hdrs,
##		so no need to go directly into facility hdrs now; combine
##		multiple mk's at same level into one call to mk; when mk'ing
##		dbutil hdrs do all hdrs in group, not just in duf/hdr.
##	19-oct-93 (peterk)
##		back/psf/yacc doesn't need +j flag; fix typo in abf build
##	27-oct-93 (vijay)
##		Make sure build no. gets into vers file.
##	27-oct-93 (vijay)
##		Undo above change. The default build no is known only to
##		readvers.
##	30-oct-93 (peterk)
##		debug build option -dbg for back, cl, common, dbutil, gl,
##		and front/st
##	19-oct-93 (vijay)
##		formindex is now linked to the shared libs. So it won't be
##		available till the end. So, do frms last.
##	30-nov-93 (lauraw)
##		Add +j to "mk files" in common/hdr/hdr so the two ercompiles
##		don't get run simultaneously. Add checks for directories that
##		may not exist before cd-ing there.
##	16-dec-93 (lauraw)
##		When building eqdef.h, specify target to avoid errors trying
##		to invoke the .INSTALLLINK rule before those files have been
##		built.
##	01-feb-94 (lauraw)
##		Use ming, not mk, in back/dmf/specials_unix and common/hdr/hdr 
##		-- mk no longer works on MINGS files.
##	24-feb-94 (vijay)
##		Need install_unix/ipcl.o in the library before building
##		iimaninf.
##	26-apr-94 (lauraw)
##		ercompile appears to need MO now.
##	26-apr-94 (peterk)
##		ercompile needs gl/sp also so just build all of gl
##	23-jan-95 (mnorton)
##		make dbutil/duf/hdr *before* making the back and dbutil
##		libs, so that dbutil/duf/duve builds cleanly.
##		Make compform befor front/st/{install ipm}.
##		Check II_AUTH_STRING with ingchkdate.
##		Add installation ID option.
##	4-feb-95 (mnorton)
##		Extensive changes to make the build on-pass.
##		Added start/stop sections functionality.
##	27-feb-95 (hanch04)
##		readded nomkmings option, minor changes to handle baroque path
##      24-may-95 (sweeney)
##		changes to make ingbuild work in a development environment.
##		copy release.dat to the appropriate manifest directory,
##		create links for programs used by ingbuild to check path.
##	1-june-95 (hanch04)
##		Touch ups, removed chmod 644 for shlibs, added yapp to
##		be built earlier.
##	14-june-95 (hanch04)
##		make sure common aif hdr get done first
##	22-sep-95 (hanch04)
##		front end precompiles get +j
##		front end should build files then lib because of tuxedo 
##	06-Dec-1995 (walro03)
##		Echo current section number at the beginning of the step.
##	15-dec-95 (hanch04)
##		change release notes for 1200
##	05-feb-96 (toumi01)
##		add build for $ING_TST/be/msfc test tools.
##	01-nov-1996 (walro03)
##		Copy Visual DBA to $ING_BUILD.
##	12-nov-96 (hanch04)
##		make csmt if it exists or cs if it exists
##		make me and megl with +j
##	29-jan-1997 (hanch04)
##		change release notes for 2000
##	09-apr-1997 (walro03)
##		In step 4, build CS executables in both the cl/clf/cs and
##		cl/clf/csmt directories, if they exist.
##	11-Apr-1997 (merja01)
##		Alter link command of iibintst and iiutltst to use -f
##		force option.  Failure to use the force was causing problems
##		on Digital UNIX 4.0.
##	18-Apr-1997 (toumi01)
##		Build both cs and csmt (if they exist).
##	22-jun-1998 (walro03)
##		Do not copy VDBA to $ING_BUILD/files; it is no longer delivered
##		on Unix.
##	17-aug-1998 (walro03)
##		1)  In step 3, finish building tools/port stuff that requires
##		    libcompat.a.
##		2)  In step 3, build missing stuff (man pages)
##		    from tools/port/ming.
##	09-sep-1998 (walro03)
##		Append Release Notes Supplement to Release Notes file.  Do not
##		dummy up release_sol.doc; it is not needed.
##	10-dec-1998 (walro03)
##		Build man pages from tools/port/ming more intelligently.
##	09-jan-1999 (somsa01)
##		In case the TNG option is set in the VERS file, make a
##		version.rel file.
##	04-mar-1999 (walro03)
##		In step 6, remove the ciutil build and check for valid auth
##		string.
##	28-jun-1999 (hanch04)
##	    Changed release.doc release.txt
##      09-Aug-1999 (merja01)
##          X-integ old 1.2 change to Extract appropriate release notes 
##          using the ING_VER from tools!port!conf!CONFIG.
##	16-feb-2000 (somsa01)
##	    Added running of buildmw script, which will build the MainWin
##	    tools.
##	26-apr-2000 (hayke02)
##		Removed TNG (embedded Ingres). This change fixes bug 101325.
##	09-Mar-2000 (rajus01)
##	    Build libjdbc.a
##	24-apr-2000 (somsa01)
##	    Added support for other product builds.
##	01-May-2000 (hanch04)
##	    Build libingres before the shared libraries so that the ice
##	    shared library can be built.
##	16-may-2000 (somsa01)
##	    Make sure we don't refer to 'dbutil' and 'ipm' for other procuct
##	    builds.
##	18-may-2000 (hanch04)
##	    Put quotes around other product variables.
##	19-mar-2001 (somsa01)
##	    Build lib<product>.a for other product builds.
##	27-Mar-2001 (bonro01)
##	    Fix release notes file creation.  The ING_VER variable was
##	    changed last year to contain the genlevel instead of our
##	    internal release identifier. (i.e. II 2.5/0011).  Now the
##	    characters after the '/' do not match the release notes
##	    file naming convention.
##	17-aug-2001 (somsa01)
##	    Now that we use Visual MainWin to build the GUI tools, we do
##	    not need buildmw anymore.
##	02-nov-2001 (somsa01)
##	    From now on, we will create a version.rel file to be included
##	    in the BASIC package.
##      12-Nov-2001 (loera01) Bug 106350
##          Fix release notes file creation for other products.
##      13-Jan-2002 (hanal04) Bug 106820.
##          Add $vers_src to path for tools/port/conf/CONFIG.
##	04-apr-2002 (somsa01)
##	    Added building of the 64-bit add-on binaries.
##	07-Aug-2002 (hanch04)
##	    Added a mkmings -lp64 for the 64-bit build.
##	20-Aug-2002 (hanch04)
##	    mk the 64 bit bitsin before running mkbzarch
##	03-Sep-2002 (somsa01)
##	    Changed mkshlibs64 to "mkshlibs -lp64".
##	27-Sep-2002 (somsa01)
##	    Run "genrelid -lp64" to make version.rel if this is a hybrid
##	    build. Also, removed obsolete section 24 (64-bit binaries).
##	17-Mar-2003 (hanch04)
##	    Run mkming, then ming, then mkming again in psl
##	28-Jan-2004 (gupsh01)
##	    Added building adf!adn files.
##	02-Feb-2004 (hanje04)
##	    Copy rpmconfig to II_MANIFEST_DIR
##      20-Apr-2004 (loera01)
##          Build common/odbc/hdr before the rest of ODBC.
##	22-Apr-2004 (bonro01)
##	    Run mk for pdf documentation.
##	11-Jun-2004 (somsa01)
##	    Cleaned up code for Open Source.
##	22-jun-04 (toumi01)
##	    Remove orphaned "else".
##      14-Jul-2004 (hanal04) Bug 112660
##          Add -F flag to allow user to specified a forced rebuild of all
##          files. newbuild on HP2 mark2638 machine inhyl1h1 was not
##          rebuilding all dependancies when iicommon.h changed.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##      05-Jan-2005 (loera01) 
##          Build odbc/hdr before compiling odbc/driver.
##	9-Sep-2006 (bonro01)
##	    Simplify genrelid so that is does not require -lpHB parm.
##		
#

CMD=`basename $0`
force=""

usage='[<cfg_str>] [-F] [-b <build_no>] [-m <mach> -p <portid>] [-nomkmings] [-dbg]
          [-inst <installation_id>]

       Use: "newbuild -help | more" for detailed information.
'

help='[<cfg_str>] [-b <build_no>] [-m <mach> -p <portid>] [-nomkmings] [-dbg]
          [-inst <installation_id>]

       Where:

         cfg_str            is the configuration string for this build
                            (e.g.: su4_u42, ris_us5, etc.).

       NOTE: The config string is required if no VERS file exists yet.

	 -F                 Issue ming and mk command with -F to force 
			    a rebuild.

         -b <build_no>      is the current build-level number (e.g., -b67);
                            default is 99.  Valid numbers are from 99-00.
                            NOTE: Use this flag only if the build-level
                            has changed since the last time this command
                            was run. 

         -m <mach>          is the machine name of the system you are
                            working on (e.g., -m sparc).

         -p <portid>        is the name of the operating system you are
                            working on (e.g., -p"SunOS 4.1.1")


       NOTE: The -m and -p flags (and their arguments) are required if
             you have a vec31 directory in $ING_SRC.

       Use the -nomkmings flag to suppress the running of mkmings.
       WARNING: Use the -nomkmings flag with great care, since
                mkming can be needed for many reasons, not just
                added or deleted files (e.g., NEEDLIBS lines may
                have changed).


       Use the -dbg flag for debug builds of back, cl, common, dbuil, gl

         -inst <installation_id>
                            is the two character installation id to use.
                            Default is "II".
' 

[ "$SHELL_DEBUG" ] && set -x
# Check for required environment variables
: ${ING_SRC?"must be set"} ${ING_BUILD?"must be set"} ${ING_TST?"must be set"}

# Check for user ingres
[ `IFS="()"; set - \`id\`; echo $2` = ingres ] ||
{ echo "$CMD: You must be ingres to run this command. Exiting..." ; exit 1 ; }

[ "$ING_VERS" ] && { vers_src="/$ING_VERS/src" ; vers="/$ING_VERS" ; }

versfile=$ING_SRC/tools/port$vers_src/conf/VERS

# default the installation id
inst_id="II"

# default the start/stop sections
start_sec=0
stop_sec=100

# default the oMINGFLAGS because of sections
oMINGFLAGS="$MINGFLAGS"

# Parse the command line
while [ $# -gt 0 ]
do
  case "$1" in
        [a-z]*) config_string="$1"
                shift
                ;;
           -b*) [ $# -gt 1 ] || { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
                build_no="$2" ; shift ; shift
                case "$build_no" in
                  [0-9][0-9]) ;;
                           *) echo "" ; echo "Illegal build number: $build_no"
                              echo "" ; echo "Usage:" ; echo "$CMD $usage"
                              exit 1
                              ;;
                esac
                ;;
            -F) force="-F "
		shift
		;;
	  -dbg) dbg="-f MINGDBG "
		shift
		;;
         -help) echo "Usage:" ; echo "$CMD $help" ; exit 0
                ;;
           -m*) [ $# -gt 1 ] || { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
                mach="$2" ; shift ; shift
                ;;
    -nomkmings) no_mkmings=true
                shift
                ;;
         -inst) [ $# -gt 1 ] || { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
		inst_id="$2" ; shift ; shift
		;;
           -p*) [ $# -gt 1 ] || { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
                portid="$2" ; shift ; shift
                ;;
        -start) [ $# -gt 1 ] || { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
		start_sec="$2" ; shift ; shift
		case "$start_sec" in
		   [0-9]|[0-9][0-9]) ;;
				  *) echo "" ; echo "Illegal start section: $start_sec"
				     echo "" ; echo "Usage:" ; echo "$CMD $usage"
				     exit 1
				     ;;
                esac
		;;
         -stop) [ $# -gt 1 ] || { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
		stop_sec="$2" ; shift ; shift
		case "$stop_sec" in
		   [0-9]|[0-9][0-9]) ;;
				  *) echo "" ; echo "Illegal stop section: $stop_sec"
				     echo "" ; echo "Usage:" ; echo "$CMD $usage"
				     exit 1
				     ;;
    		esac
		;;
            -*) echo "Usage:" ; echo "$CMD $usage" ; exit 1
                ;;
             *) echo "Usage:" ; echo "$CMD $usage" ; exit 1
                ;;
  esac
done

##  ################ Begin baseline/variable checking ####################

##  Always execute the checking section. It makes sure everything is
##  in place in order to begin. Do not put sectional checking around
##  this area.

# Make sure we have what we need
[ -f $versfile ] ||
{
  [ "$config_string" ] ||
  { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
}
#[ -d $ING_SRC/vec31 ] &&
#{
#  [ "$mach" -a "$portid" ] ||
#  { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
#}

## Build INGRES from fresh source
# Edit the VERS file if it's there; create it if it's not
if [ -f $versfile ] ; then
  [ "$build_no" ] &&
  {
    ed $versfile <<-! >/dev/null 2>&1
    /^build[ ]*=/
    d
    w
    q
!
    echo "build=$build_no" >> $versfile
  }
else
  echo "config=$config_string" > $versfile
  if [ "$build_no" ] ; then
    echo "build=$build_no" >> $versfile
  fi
fi

[ $BUILD_64BIT ] &&
{
iisetres "ii.*.setup.add_on64_enable" ON
}

## #################### SECTION ONE: ming ####################
cur_sec=1

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
cd $ING_SRC/tools/port$vers_src/ming
echo "" ; echo '**** Building ming ...' ;  date
sh BOOT

# Build the MING files
[ "$no_mkmings" ] ||
{
  echo "" ; echo '**** Generating MING files ...' ;  date
  mkmings

[ $BUILD_64BIT ] &&
{
# Build the MING64 files
  echo "" ; echo '**** Generating MING64 files ...' ;  date
  mkmings -lp64
}
}
}


## ################### SECTION TWO: port/tools ###############
MINGFLAGS=$force$MINGFLAGS
cur_sec=2
 
[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build the tools shell-scripts, libraries, and executables
echo "" ; echo '**** Building tools/port ...' ;  date
cd $ING_SRC/tools/port$vers_src
[ $BUILD_64BIT ] &&
{
## Build 64 bit first to get bitsin64 needed for mkbzarch
mk64 $MINGFLAGS
}
mk $MINGFLAGS
}


## ################### SECTION THREE: libcompat.a ############
cur_sec=3
 
[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Invoke debug build starting here if opted for
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="$dbg$MINGFLAGS"

# Begin building libcompat.a...
echo "" ; echo '**** Begin building libcompat.a ...' ;  date
cd $ING_SRC/cl/clf$vers_src/me_unix
echo "==== $ING_SRC/cl/clf$vers_src/me_unix: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j lib
}

cd $ING_SRC/cl/clf$vers_src
echo "==== $ING_SRC/cl/clf$vers_src: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}

# Build the GL portions of libcompat.a
echo "" ; echo '**** Building gl part of libcompat.a ...' ;  date
cd $ING_SRC/gl/glf$vers_src/megl
echo "==== $ING_SRC/gl/glf$vers_src/megl: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j lib
}

cd $ING_SRC/gl/glf$vers_src
echo "==== $ING_SRC/gl/glf$vers_src: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}

# Build ercompile
echo "" ; echo '**** Building executables in cl/clf/er_unix_win ...' ;  date
cd $ING_SRC/cl/clf$vers_src/er_unix_win
echo "==== $ING_SRC/cl/clf$vers_src/er_unix_win: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

# Build message files and ercompile'd .h files
echo "" ; echo '**** Building .mnx files and .msg->.h files ...' ;  date
cd $ING_SRC/common/hdr$vers/hdr
mkfecat
echo "==== $ING_SRC/common/hdr$vers/hdr: mk $MINGFLAGS files"
# Override "-j" or this will get out of sync:
ming $MINGFLAGS +j files
ming $force erclf.h erglf.h erscf.h erusf.h

# Finish building libcompat.a
echo "" ; echo '**** Finish building libcompat.a ...' ;  date
cd $ING_SRC/cl/clf$vers_src
mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}

# A few pieces of tools/port require libcompat.a.  Go back now and finish
# up tools/port
echo "" ; echo '**** Finishing tools/port ...' ;  date
cd $ING_SRC/tools/port$vers_src
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

# Get the pieces of tools/port/ming that were missed by BOOT:  man pages, etc.
cd $ING_SRC/tools/port$vers_src/ming
for file in `ls *\.1`
do
	mk $MINGFLAGS $ING_SRC/man1/$file
done
}


## ################### SECTION FOUR: cs ######################
cur_sec=4

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build CS executables
echo "" ; echo '**** Building CS executables ...' ;  date

if [ -d "$ING_SRC/cl/clf$vers_src/csmt_unix" ]
then
    cd $ING_SRC/cl/clf$vers_src/csmt_unix
    echo "==== $ING_SRC/cl/clf$vers_src/csmt_unix: mk $MINGFLAGS "
    mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
    mk64 $MINGFLAGS
}
fi

if [ -d "$ING_SRC/cl/clf$vers_src/cs_unix" ]
then
    cd $ING_SRC/cl/clf$vers_src/cs_unix
    echo "==== $ING_SRC/cl/clf$vers_src/cs_unix: mk $MINGFLAGS "
    mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
    mk64 $MINGFLAGS
}
fi 

# Test CS
echo "" ; echo '**** Running csphil ...' ;  date
if $ING_BUILD/utility/csphil >/dev/null 2>&1 ; then
  echo "$CMD: csphil passed."
else
  echo "" ; echo "********** W A R N I N G **********"
  echo "" ; echo "$CMD: csphil did not pass."
  echo "This is not a fatal build error, but you should resolve"
  echo "the problem, then re-run csphil to verify CS is correct."
fi
[ $BUILD_64BIT ] &&
{
echo "" ; echo '**** Running csphil ...' ;  date
if $ING_BUILD/utility/lp64/csphil >/dev/null 2>&1 ; then
  echo "$CMD: 64 bit csphil passed."
else
  echo "" ; echo "********** W A R N I N G **********"
  echo "" ; echo "$CMD: 64 bit csphil did not pass."
  echo "This is not a fatal build error, but you should resolve"
  echo "the problem, then re-run csphil to verify CS is correct."
fi

}

# turn off dbg builds in front
MINGFLAGS="$oMINGFLAGS"
}


## ################### SECTION FIVE: eqc and esqlc ##################
cur_sec=5

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build front-end tools: eqmerge and iyacc
echo "" ; echo '**** Building eqmerge and iyacc ...' ;  date
for dir in eqmerge yycase yacc yapp
{
  cd $ING_SRC/front/tools$vers_src/$dir
  mk $MINGFLAGS lib 
  mk $MINGFLAGS 
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib 
  mk64 $MINGFLAGS 
}
}

# Build eqdef.h -- now needed by eqc/esqlc
echo "" ; echo '**** Building eqdef.h ...' ;  date
cd $ING_SRC/front/frontcl$vers_src/specials
mk $ING_BUILD/files/eqdef.h
# Build ercompile'd front-end header files
echo "" ; echo "**** Building ercompile'd front-end header files ..."
cd $ING_SRC/front/hdr$vers/hdr
for msgfile in `ls *.msg 2>/dev/null`
{
  ming $force `basename $msgfile .msg`.h
}
# Build front-end libraries for eqc and esqlc
echo "" ; echo '**** Building front-end libraries for eqc and esqlc ...' ;  date
for dir in afe fmt ug
{
  cd $ING_SRC/front/utils$vers_src/$dir
  echo "==== $ING_SRC/front/utils$vers_src/$dir: mk $MINGFLAGS lib"
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}

}
# Build embedded libraries needed for eqc and esqlc
echo "" ; echo '**** Building embedded language headers ...' ;  date
cd $ING_SRC/front/embed$vers/hdr
echo "==== $ING_SRC/front/embed$vers/hdr: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs
echo "" ; echo '**** Building libraries for eqc and esqlc ...' ;  date
cd $ING_SRC/front/embed$vers_src/equel
echo "==== $ING_SRC/front/embed$vers_src/equel: mk $MINGFLAGS lib"

# Turn debug builds on
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="$dbg$MINGFLAGS"

mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}


cd $ING_SRC/common/adf$vers_src
echo "==== $ING_SRC/common/adf$vers_src: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
for dir in cuf
{
  cd $ING_SRC/common/$dir$vers_src
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk $MINGFLAGS lib
}
}

# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"

# Build eqc and esqlc
echo "" ; echo '**** Building eqc and esqlc ...' ;  date
for dir in c csq
{
  cd $ING_SRC/front/embed$vers_src/$dir
  echo "==== $ING_SRC/front/embed$vers_src/$dir: mk $MINGFLAGS +j lib"
  mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS +j lib
}

  echo "==== $ING_SRC/front/embed$vers_src/$dir: mk $MINGFLAGS"
  mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS
}

}
}


## ################### SECTION SIX: ciutil and auth ###################
cur_sec=6

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Make sure that symbol.tbl exists
[ -f $II_SYSTEM/ingres/files/symbol.tbl ]
{
   echo "" ; echo "**** Touching symbol.tbl ..."
   touch $II_SYSTEM/ingres/files/symbol.tbl
}

# Turn debug builds on
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="$dbg$MINGFLAGS"

# Build nm here to enable auth string workaround
echo "" ; echo '**** Building NM utilities ...' ;  date
cd $ING_SRC/cl/clf$vers_src/nm_unix
echo "==== $ING_SRC/cl/clf$vers_src/nm_unix: mk $MINGFLAGS"
mk $MINGFLAGS +j
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j
}

}


## ################### SECTION SEVEN: common hdr #######################
cur_sec=7

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build remaining common/hdr/hdr generated .h files (depends on eqc & esqlc)
echo "" ; echo '**** Building .qsh->.h files ...' ;  date
cd $ING_SRC/common/hdr$vers/hdr
echo "==== $ING_SRC/common/hdr$vers/hdr: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs

# Build byacc
echo "" ; echo '**** Building byacc ...' ;  date
cd $ING_SRC/back/psf$vers_src/yacc
echo "==== $ING_SRC/back/psf$vers_src/yacc: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}

echo "==== $ING_SRC/back/psf$vers_src/yacc: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

cd $ING_SRC/back/psf$vers_src/psl
echo "==== $ING_SRC/back/psf$vers_src/psl: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
mkming
mk $MINGFLAGS +j pslgram.c pslsgram.c
mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j lib
mkming -lp64
mk64 $MINGFLAGS +j pslgram.c pslsgram.c
mk64 $MINGFLAGS +j lib
}

echo "==== $ING_SRC/back/psf$vers_src/psl: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

}


## ################### SECTION EIGHT: cl ########################
cur_sec=8

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build CL executables
echo "" ; echo '**** Building CL executables ...' ;  date
cd $ING_SRC/cl/clf$vers_src
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}
cd $ING_SRC/gl/glf$vers_src
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}
}


## ################### SECTION NINE: dbutil, langs, some back ##################
cur_sec=9

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec

# Build dbutil headers
echo "" ; echo '**** Building dbutil headers ...' ;  date
cd $ING_SRC/dbutil
echo "==== $ING_SRC/dbutil: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs

echo "" ; echo '**** Building common/aif headers ...' ;  date
cd $ING_SRC/common/aif$vers/hdr
echo "==== $ING_SRC/common/aif$vers/hdr: mk $MINGFLAGS "
mk $MINGFLAGS 
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS 
}

echo "" ; echo '**** Building common/odbc headers ...' ;  date
cd $ING_SRC/common/odbc$vers/hdr
echo "==== $ING_SRC/common/odbc$vers/hdr: mk $MINGFLAGS "
mk $MINGFLAGS 

# Build libgcf.a
echo "" ; echo '**** Building common/gcf headers and libraries ...' ;  date
cd $ING_SRC/common/gcf$vers_src/gcc
mk $MINGFLAGS +j hdrs lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j lib
}
cd $ING_SRC/common/gcf$vers_src
mk $MINGFLAGS hdrs lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}

# Build libjdbc.a
echo "" ; echo '**** Building common/jdbc headers and libraries ...' ;  date
cd $ING_SRC/common/jdbc$vers_src/jdbc
mk $MINGFLAGS +j hdrs lib
[ $BUILD_64BIT ] &&
{
mk $MINGFLAGS +j lib
}


# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"

# Continue building libraries...
echo "" ; echo '**** Building front-end files ...' ;  date
for dir in files files_unix
{
  cd $ING_SRC/front/frontcl$vers_src/$dir
  echo "==== $ING_SRC/front/frontcl$vers_src/$dir: mk $MINGFLAGS files"
  mk $MINGFLAGS files
}
echo "" ; echo '**** Building embedded-language libraries ...' ;  date
for d in copy dclgen libq libqgca libqxa sqlca equel
{
    cd $ING_SRC/front/embed$vers_src/$d
    echo "==== $ING_SRC/front/embed$vers_src/$d: mk $MINGFLAGS lib"
    mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
    mk64 $MINGFLAGS lib
}

}

# Build the duve hdr files
echo "" ; echo '**** Building duve hdr files ...' ;  date
cd $ING_SRC/dbutil/duf$vers/hdr
echo "==== $ING_SRC/dbutil/duf$vers_src/hdr: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
echo "" ; echo '**** Building back and dbutil libraries ...' ;  date
for dir in back dbutil
{
  cd $ING_SRC/$dir
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}
}
echo "" ; echo '**** Building back libraries ...' ;  date
for dir in back
{
  cd $ING_SRC/$dir
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}
}
}


## ################### SECTION TEN: back, common, admin ##################
cur_sec=10

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
  cd $ING_SRC/common/odbc$vers/hdr
echo "==== $ING_SRC/common/odbc$vers/hdr: mk $MINGFLAGS "
  mk $MINGFLAGS 
echo "" ; echo '**** Build Common aif  ...' ;  date
  cd $ING_SRC/common/aif$vers/hdr
echo "==== $ING_SRC/common/aif$vers/hdr: mk $MINGFLAGS "
  mk $MINGFLAGS 
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS 
}

echo "==== $ING_SRC/common/aif$vers_src: mk $MINGFLAGS lib"
  cd $ING_SRC/common/aif$vers_src
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}

# Continue building executables...
echo "" ; echo '**** Continue building executables ...' ;  date
for dir in back common admin
{
  cd $ING_SRC/$dir
  mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS
}
}


# Build dbutil/duf/specials_unix.
echo "" ; echo '**** Building dbutils specials ...' ;  date
cd $ING_SRC/dbutil/duf$vers_src/specials_unix
echo "$ING_SRC/dbutil/duf$vers_src/specials_unix: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

# Test CL
echo "" ; echo '**** Running cltest ...' ;  date
if $ING_BUILD/bin/cltest >/dev/null 2>&1 ; then
  echo "$CMD: cltest passed."
else
  echo "" ; echo "********** W A R N I N G **********"
  echo "" ; echo "$CMD: cltest did not pass."
  echo "This is not a fatal build error, but you should resolve"
  echo "the problem, then re-run cltest to verify CL is correct."
fi
[ $BUILD_64BIT ] &&
{
echo "" ; echo '**** Running 64 bit cltest ...' ;  date
if $ING_BUILD/bin/lp64/cltest >/dev/null 2>&1 ; then
  echo "$CMD: 64 bit cltest passed."
else
  echo "" ; echo "********** W A R N I N G **********"
  echo "" ; echo "$CMD: 64 bit cltest did not pass."
  echo "This is not a fatal build error, but you should resolve"
  echo "the problem, then re-run cltest to verify CL is correct."
fi

}

# Build iimerge.o
echo "" ; echo '**** Building iimerge.o ...' ;  date
cd $ING_SRC/back/dmf$vers_src/specials_unix
echo "==== $ING_SRC/back/dmf$vers_src/specials_unix: mk $MINGFLAGS"
ming $MINGFLAGS
[ $BUILD_64BIT ] &&
{
ming64 $MINGFLAGS
}
}


## ################### SECTION ELEVEN: compform ########################
cur_sec=11

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build front-end headers
echo "" ; echo '**** Building front-end header files ...' ;  date
cd $ING_SRC/front/hdr$vers/hdr
echo "==== $ING_SRC/front/hdr$vers/hdr: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs
cd $ING_SRC/front
mk $MINGFLAGS hdrs

# NOTE: Do not build everything in front/st, because some things
#       are forms-based, requiring formindex and copyform/compform
echo "" ; echo '**** Begin building front/st ...' ;  date
cd $ING_SRC/front/st$vers/hdr
echo "==== $ING_SRC/front/st$vers/hdr: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs
cd $ING_SRC/front/st$vers_src
mk $MINGFLAGS files shell
#mk $MINGFLAGS shell

# Build frame library here to allow compform build
# build frame/valid grammars +j for non-parallel
echo "" ; echo '**** Building frame libraries ...' ;  date
cd $ING_SRC/front/frame$vers_src/valid
echo "==== $ING_SRC/front/frame$vers_src/valid: mk $MINGFLAGS lib"
mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j lib
}

cd $ING_SRC/front/frame$vers_src
echo "==== $ING_SRC/front/frame$vers_src: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}


# Build some front libraries to allow compform to build
echo "" ; echo '**** Building some utils libraries ...' ;  date
  cd $ING_SRC/front/utils$vers_src/feds 
  echo "==== $ING_SRC/front/utils$vers_src/feds: mk $MINGFLAGS +j lib"
  mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS +j lib
}

for dir in uf ui
{
   cd $ING_SRC/front/utils$vers_src/$dir
   echo "==== $ING_SRC/front/utils$vers_src/$dir: mk $MINGFLAGS lib"
   mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS lib
}

}

echo "" ; echo '**** Building some frontcl libraries ...' ;  date
for dir in runsys ft termdr libqsys
{
   cd $ING_SRC/front/frontcl$vers_src/$dir
   echo "==== $ING_SRC/front/frontcl$vers_src/$dir: mk $MINGFLAGS lib"
   mk $MINGFLAG lib
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS lib
}

}

# Build compform to allow one-pass through front/st/install
echo "" ; echo '**** Building compform ...' ;  date
cd $ING_SRC/front/frame$vers_src/compfrm
echo "==== $ING_SRC/front/frame$vers_src/compfrm: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS
}

}


## ################### SECTION TWELVE: st and some libs ###################
cur_sec=12

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Continue the front/st
echo "" ; echo '**** Continuing front/st ...' ;  date
cd $ING_SRC/front/st$vers_src
for dir in config install util
{
  cd $ING_SRC/front/st$vers_src/$dir
  echo "==== $ING_SRC/front/st$vers_src/$dir: mk $MINGFLAGS lib"
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}

}

# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
# Build front-end libraries needed by front/st utilities
echo "" ; echo '**** Building front-end libraries for front/st utilities ...' ;  date
for dir in frame runtime tbacc
{
  cd $ING_SRC/front/frame$vers_src/$dir
  echo "==== $ING_SRC/front/frame$vers_src/$dir: mk $MINGFLAGS lib"
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}

}
cd $ING_SRC/front/frame$vers_src/valid
echo "==== $ING_SRC/front/frame$vers_src/valid: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j lib
}

for dir in ft termdr libqsys runsys
{
  cd $ING_SRC/front/frontcl$vers_src/$dir
  echo "==== $ING_SRC/front/frontcl$vers_src/$dir: mk $MINGFLAGS lib"
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}

}
}


## ################### SECTION THIRTEEN: utilities #####################
cur_sec=13

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Turn debug builds on
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="$dbg$MINGFLAGS"

# Build iimaninf
echo "" ; echo '**** Building iimaninf ...' ;  date
cd $ING_SRC/front/st$vers_src/install_unix
echo "==== $ING_SRC/front/st$vers_src/install_unix: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}

cd $ING_SRC/front/st$vers_src/install
echo "==== $ING_SRC/front/st$vers_src/install: mk $MINGFLAGS iimaninf"
mk $MINGFLAGS $ING_BUILD/utility/iimaninf

# Build configuration utilities
echo "" ; echo '**** Building configuration utilities ...' ;  date
cd $ING_SRC/front/st$vers_src/config
echo "==== $ING_SRC/front/st$vers_src/config: mk $MINGFLAGS..."
mk $MINGFLAGS $ING_BUILD/utility/iigenres
mk $MINGFLAGS $ING_BUILD/utility/iiremres
mk $MINGFLAGS $ING_BUILD/utility/iiresutl
mk $MINGFLAGS $ING_BUILD/utility/iisetres
mk $MINGFLAGS $ING_BUILD/utility/iivalres

# Build installation and startup utilities
echo "" ; echo '**** Building ST utilities ...' ;  date
cd $ING_SRC/front/st$vers_src/util
echo "==== $ING_SRC/front/st$vers_src/util: mk $MINGFLAGS"
mk $MINGFLAGS +j
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS +j
}


# iivertst should now have been build. Create links to iibintst, iiutltst
# used by ingbuild to check correct search path
[ -f $ING_BUILD/utility/iivertst ] &&
    ln -f $ING_BUILD/utility/iivertst $ING_BUILD/utility/iiutltst &&
    ln -f $ING_BUILD/utility/iivertst $ING_BUILD/bin/iibintst
}


## ################### SECTION FOURTEEN: files ########################
cur_sec=14

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build description and collation files
echo "" ; echo '**** Building description and collation files ...' ;  date
cd $ING_SRC/common/adf$vers_src/adm
echo "==== $ING_SRC/common/adf$vers_src/adm: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS
}

echo "" ; echo '**** Building unicode mapping files ...' ;  date
cd $ING_SRC/common/adf$vers_src/adn
echo "==== $ING_SRC/common/adf$vers_src/adn: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS
}

# Build .def files out of cl/clf/specials_unix
echo "" ; echo '**** Building .def files ...' ;  date
cd $ING_SRC/cl/clf$vers_src/specials_unix
echo "==== $ING_SRC/cl/clf$vers_src/specials_unix: mk $MINGFLAGS files"
mk $MINGFLAGS files

# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"

# Build dict files and utilities
echo "" ; echo '**** Building dict files ...' ;  date
cd $ING_SRC/front/dict$vers_src/dictfile
echo "==== $ING_SRC/front/dict$vers_src/dictfile: mk $MINGFLAGS files"
mk $MINGFLAGS files
echo "" ; echo '**** Building dict utilities ...' ;  date
cd $ING_SRC/front/dict$vers_src/dictutil
echo "==== $ING_SRC/front/dict$vers_src/dictutil: mk $MINGFLAGS lib"
mk $MINGFLAGS  lib
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS lib
}
echo "==== $ING_SRC/front/dict$vers_src/dictutil: mk $MINGFLAGS"
mk $MINGFLAGS +j
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS
}

}


## ################### SECTION FIFTEEN: ingbuild, others ##################
cur_sec=15

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build ingbuild.
echo "" ; echo '**** Building ingbuild ...' ;  date
cd $ING_SRC/front/st$vers_src/install
echo "==== $ING_SRC/front/st$vers_src/install: mk $MINGFLAGS exe"
mk $MINGFLAGS exe

# Build the character set installation utility and the character set files
echo "" ; echo '**** Building chinst ...' ;  date
cd $ING_SRC/front/misc$vers_src/chinst
echo "==== $ING_SRC/front/misc$vers_src/chinst: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS
}

echo "" ; echo '**** Building character set files ...' ;  date
cd $ING_SRC/front/misc$vers_src/chsfiles
echo "==== $ING_SRC/front/misc$vers_src/chsfiles: mk $MINGFLAGS files"
mk $MINGFLAGS files
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS files
}

# Establish pseudo-installation environment for iimaninf
# NOTE: iimaninf is required by iisudbms to get the version number
{
echo "" ; echo '**** Building pseudo-installation environment for iimaninf ...' ;  date
}

cd $ING_SRC/front/st$vers_src/specials_unix
echo "==== $ING_SRC/front/st$vers_src/specials_unix: mk $MINGFLAGS"

mk $MINGFLAGS
[ -r release.dat ] ||
{
  echo "" ; echo "********** W A R N I N G **********"
  echo "" ; echo "$CMD: Cannot read release.dat file."
  echo "This is not a fatal build error, but you will not be"
  echo "able to start INGRES until this problem is resolved."
}
# figure out where to put release.dat for ingbuild. This
# logic matches that in front/st/install/ipparse.y
[ -n "$II_MANIFEST_DIR" ] &&
if [ -d "$II_MANIFEST_DIR" ]
then
    MANIFEST_DIR=$II_MANIFEST_DIR
fi ||

if [ -d $ING_BUILD/manifest ]
then
    MANIFEST_DIR=$ING_BUILD/manifest
fi ||

if [ -d $ING_BUILD/install ]
then
    MANIFEST_DIR=$ING_BUILD/install 
fi ||

if [ -d $ING_BUILD/files ] # if this doesn't exist we're in _real_ trouble
then 
    MANIFEST_DIR=$ING_BUILD/files  # perhaps we should flag an error
fi

# make a link if you can, else copy over
echo "copying release.dat to $MANIFEST_DIR"
ln release.dat $MANIFEST_DIR/release.dat ||
    cp release.dat $MANIFEST_DIR/release.dat

# same for the .prt files
echo "copying *.prt to $MANIFEST_DIR"
for part in *.prt
do
	echo "copying $part to $MANIFEST_DIR"
	ln $part $MANIFEST_DIR/$part || cp $part $MANIFEST_DIR/$part
done

echo "copying rpmconfig to $MANIFEST_DIR"
cp rpmconfig $MANIFEST_DIR/rpmconfig

# Turn debug builds on
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="$dbg$MINGFLAGS"
}


## ################### SECTION SIXTEEN: iimerge links, id #################
cur_sec=16

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec

# Build back-end executables
echo "" ; echo '**** Building the 32 bit iimerge links ...' ;  date
iilink -standard -noudt -lp32
[ $BUILD_64BIT ] &&
{
echo "" ; echo '**** Building the 64 bit iimerge links ...' ;  date
iilink -standard -noudt -lp64
}

# Set installation id
echo "" ; echo "**** Setting installation id to $inst_id ..." ; date
ingsetenv II_INSTALLATION_ID $inst_id
}


## ################### SECTION SEVENTEEN: remaining front libs ###############
cur_sec=17

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Force this hdr build. For some reason tutblinf.sh must be forced.
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="-F"

# Be sure that front/utils/tblutil hdrs are built
echo "" ; echo '**** Making sure tblutil hdrs are built ...' ;  date
cd $ING_SRC/front/utils$vers_src/tblutil
echo "====$ING_SRC/front/utils$vers_src/tblutil: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs

# Resume normal MINGFLAGS
MINGFLAGS="$oMINGFLAGS"

# Build front-end libs needed by formindex, copyform, and compform
echo ""
echo '**** Building front-end libraries for formindex/copyform/compform ...' ;  date

  cd $ING_SRC/front/utils$vers_src/copyutil
  echo "==== $ING_SRC/front/utils$vers_src/copyutil: mk $MINGFLAGS +j lib"
  mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS +j lib
}

  cd $ING_SRC/front/utils$vers_src/oo
  echo "==== $ING_SRC/front/utils$vers_src/oo: mk $MINGFLAGS +j lib"
  mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS +j lib
}

for dir in frame frontcl vifred utils
{
  cd $ING_SRC/front/$dir$vers_src
  # Skip specials_unix for now, since libabfrts.a doesn't exist yet
  curdir=`pwd` ; [ "`basename $curdir`" = "specials_unix" ] && continue
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
  mk64 $MINGFLAGS lib
}

}

# Build compform-generated files
echo "" ; echo "**** Building copyform/compform'd library modules ..."
for dir in ipm install_unix
{
  cd $ING_SRC/front/st$vers_src/$dir
  echo "==== $ING_SRC/front/st$vers_src/$dir: mk $MINGFLAGS lib"
  mk $MINGFLAGS lib
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS lib
}

}

# Build the rest of the front-end libs, files
echo "" ; echo '**** Building front-end libraries, and files ...' ;  date
# build abf grammars +j for non-parallel
for dir in quel sql
{
    [ -d $ING_SRC/front/abf$vers_src/$dir ] &&
    {
       cd $ING_SRC/front/abf$vers_src/$dir
       echo "==== $ING_SRC/front/abf$vers_src/$dir: mk $MINGFLAGS +j lib"
       mk $MINGFLAGS +j lib
[ $BUILD_64BIT ] &&
{
       mk64 $MINGFLAGS +j lib
}

    }
}
for dir in ada adasq basic basicsq c cobol cobolsq fortran \
	   fortransq  pascal pascalsq pl1 pl1sq
{
    [ -d $ING_SRC/front/embed$vers_src/$dir ] &&
    {
       cd $ING_SRC/front/embed$vers_src/$dir
       echo "==== $ING_SRC/front/embed$vers_src/$dir: mk $MINGFLAGS +j lib"
       mk $MINGFLAGS +j files lib
[ $BUILD_64BIT ] &&
{
       mk64 $MINGFLAGS +j files lib
}

    }
}

cd $ING_SRC/front
mk $MINGFLAGS files lib
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS lib
}

}


## ################### SECTION EIGHTEEN: dbutil ####################
cur_sec=18

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build dbutil executables
echo "" ; echo '**** Building dbutil executables ...' ;  date
cd $ING_SRC/dbutil
echo "$ING_SRC/dbutil: mk $MINGFLAGS"
mk $MINGFLAGS
[ $BUILD_64BIT ] &&
{
   mk64 $MINGFLAGS
}

}


## ################### SECTION NINETEEN: libingres.a ################
cur_sec=19

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build libingres.a
echo "" ; echo '**** Building libingres.a ...' ;  date
mergelibs
[ $BUILD_64BIT ] &&
{
mergelibs -lp64
}
}



## ################### SECTION TWENTY: mkshlibs ####################
cur_sec=20

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build front end shared libraries now.
echo "" ; echo '**** Created shared libs ...' ; date
mkshlibs
[ $BUILD_64BIT ] &&
{
mkshlibs -lp64
}

}


## ################### SECTION TWENTYONE: FE executables ################
cur_sec=21

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build Vec31
[ -d $ING_SRC/vec31 ] &&
{
  echo "" ; echo '**** Building Vec31 ...' ;  date
  cd $ING_SRC/vec31/usr/vec/bin
  sed -e "s/^MACH=.*$/MACH=$mach/" -e "s/^PORTID=.*$/PORTID=\"$portid\"/" \
      -e "s/^PORTBY=.*$/PORTBY=\"`whoami` `date`\"/" Make/FLAGS > FLAGS
  chmod +x *
  cd $ING_SRC/vec31/src
  mk $MINGFLAGS exe files
}

# Build the remaining front-end executables
echo ""
echo '**** Building front-end executables ...' ;  date
cd $ING_SRC/front
mk $MINGFLAGS exe
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS exe
}


# Build front end forms now, and whatever else.
echo ""
echo '**** Building front-end forms ...' ;  date
cd $ING_SRC/front
mk $MINGFLAGS 
[ $BUILD_64BIT ] &&
{
mk64 $MINGFLAGS 
}

chmod 644 $ING_BUILD/files/english/rtiforms.fnx
}


## ################### SECTION TWENTYTWO: testtools, sig ###################
cur_sec=22

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build test tools and sig
echo "" ; echo '**** Building testing tools and SIG ...' ;  date
for dir in testtool sig
{
  [ -d $ING_SRC/$dir ] &&
  { 
    for part in hdrs lib files exe
    {
       cd $ING_SRC/$dir
       mk $MINGFLAGS +j $part
    }
  }
}
  
# Build the stress test tools
[ -d $ING_TST/stress ] &&
{
   echo "" ; echo '**** Building stress tools ...' ;  date
   cd $ING_TST/stress
   mk $MINGFLAGS 
}

# Build the listexec stuff
[ -d $ING_TST/suites/shell ] &&
{
   echo "" ; echo '**** Building test shells ...' ;  date
   cd $ING_TST/suites/shell
   mk $MINGFLAGS
}

# Build the msfc test tools
[ -d $ING_TST/be/msfc ] &&
{
   echo "" ; echo '**** Building be/msfc tools ...' ;  date
   cd $ING_TST/be/msfc
   mk $MINGFLAGS 
}
}


## ################### SECTION TWENTYTHREE: docs ######################
cur_sec=23

[ "$cur_sec" -ge "$start_sec" ] &&
[ "$cur_sec" -le "$stop_sec" ] &&
{
echo "**** Newbuild section" $cur_sec
# Build tech
[ -d $ING_SRC/tech ] &&
{
   echo "" ; echo '**** Building tech notes ...' ;  date
   cd $ING_SRC/tech
   mk $MINGFLAGS
}

# Try to move Release Notes
## Extract correct release notes from CONFIG ##
    reltxt=`sed -n "/# define ING_VER/p" $ING_SRC/tools/port$vers_src/conf/CONFIG | \
         tr -d './' | awk '{ printf"%srel.txt", substr($5,1,2) "00" }'`
[ -f $ING_SRC/tools/techpub$vers_src/genrn/$reltxt ] &&
{
   echo "" ; echo '**** Copying release notes to build area ...' ;  date
   cp $ING_SRC/tools/techpub$vers_src/genrn/$reltxt $ING_BUILD/release.txt
   chmod 644 $ING_BUILD/release.txt
}
config_string=`grep '^config=' $versfile | sed 's/config=//'`
[ -f $ING_SRC/tools/techpub$vers_src/genrn/rel_$config_string.txt ] &&
{
   echo "" ; echo '**** Copying platform-specific release notes to build area ...' 
   cat $ING_SRC/tools/techpub$vers_src/genrn/rel_$config_string.txt >> $ING_BUILD/release.txt
}

[ -f $ING_BUILD/release.txt ] ||
{
   echo "" ; echo '************* !! WARNING !! *************'
   echo ' **** Faking the release.txt file!!!'
   touch $ING_BUILD/release.txt
}

# Put correct perms on release.txt
chmod 644 $ING_BUILD/release.txt

# Create the version.rel file, and place it in $ING_BUILD
genrelid > $ING_BUILD/version.rel
chmod 644 $ING_BUILD/version.rel

# Build doc pdf
[ -d $ING_SRC/tools/techpub/pdf ] &&
{
   echo "" ; echo '**** Building pdf documentation ...' ;  date
   cd $ING_SRC/tools/techpub/pdf
   mk $MINGFLAGS
}

} 


## ######################  SECTION TWENTYFOUR: #####################
cur_sec=24


echo "**** Newbuild section" $cur_sec
echo "" ; echo "$CMD done: `date`"
exit 0
