:
#
# Copyright (c) 2004 Ingres Corporation
#
# bang -- Build Another iNGres
#
# Does all build steps in the right order.
#
## History:
##	17-jun-1989 (boba)
##		Created for steps documented in first draft of
##		6.1 porting guide.
##	26-jun-89 (daveb)
##		Need ING_BUILD and ING_SRC, not ING_HOME; add missing "then"
##		Problem:  Need to start installation at some point so we
##			  can do formindex with feworkdb. dbutil/specials_unix
##			  sort of wants to do this, but what if you've got one
##			  running already?
##	29-jun-89 (daveb)
##		Try to start the server if (when) it isn't up.
##		Handle fsgw precedence problem by building lib twice.
##	20-july-89 (daveb)
##		Add MKARGS env var, for suuport of things like -j8 on
##		multi-cpu or very big machines.
##	240-jul-89 (daveb)
##		Fix mkfecat, which was being done in the wrong directory
##		and was broken by the previous change.
##	31-jul-1989 (boba)
##		Move from bang61.sh to bang.sh when added to ingresug.
##	16-aug-1989 (harry)
##		Since  "mk lib" in ING_SRC/back/dmf/dmf needs hdr files from
##		common/hdr/hdr, add it to the list of things to build in back.
##	24-aug-1989 (nnl)
##		Moved a couple of "cd" commands to put them right before
##		the operations that need them.
##	31-aug-1989 (nnl)
##		Changed "mk iimerge" to "mkiimerge."
##	11-sep-1989 (boba)
##		Make frontcl bulkfiles before building equel & esql.
##		Make front lib & exe before server up & iidbdb exists.
##	18-sep-1989 (nasser)
##		Changed $ING_SRC/frontcl to $ING_SRC/front/frontcl.
##	6-oct-89 (daveb)
##		Need to do admin before front, because csstartup uses
##		some of the admin/install/shell thingys.
##	20-nov-89 (rog)
##		Add mk hdrs in dbutil before mk lib as per the porting guide.
##	21-dec-1989 (boba)
##		Awkcc must be built before appchk.
##	09-jan-1990 (boba)
##		Modify build dependencies as discovered in sanity
##		build.  Remove "admin" as an independent step (it's
##		inside back and front).  Handle embed precedence
##		problem by doing lib twice.  No longer need to
##		handle fsgw precedence problem by doing lib twice.
##		Build utils/ui before linking dbutil.  Move initial
##		booting of server and creation of iidbdb to back
##		section and leave server booting only in front
##		section.  This still requires that "iistartup -init"
##		have been run before running bang!
##	10-jan-1990 (boba)
##		The embed dependency in really on headers.
##	14-jan-1990 (boba)
##		Add build of sig.
##	15-jan-1990 (boba)
##		Add build of "tests".  This directory will soon be called
##		"testtool" as per a recent agreement with PV, so allow
##		for this possibility at this point.  Also, fix message
##		insisting that you be ingres "or" rtingres to run bang.
##	15-jan-1990 (boba)
##		Fix stupid "fix" for ensure.  Real fix is for ensure to
##		accept multiple names.
##	27-mar-1990 (boba)
##		Add making of collation files.
##	27-aug-1990 (jonb)
##		Add mk in back/dmf/specials_unix to do pre-link of iimerge.o.
##		Then do iilink instead of mkiimerge to create iimerge      
##		executable.  Also, set BANG for mk to use for OUT file names,
##		and increment .bang_count.
##	13-sep-1990 (jonb)
##		Add some more information to the output header, and echo
##		a few comments to serve as landmarks in the output file.
##	23-jan-1991 (rog)
##		Need to do a 'mk lib' in testtool before doing 'mk'.
##	27-mar-1991 (jonb)
##		Add star section.  Removed abfdemo.  Removed section that
##		checks for awkcce.
##	06-jun-1991 (hallman)
##		Moved star section to follow back and moved startup of
##		servers to star section so that star isn't started until
##		after it's built, and remove createdb since it's done
##		automatically by iistartup.
##	15-aug-1991 (gillnh2o)
##		Added mk -F in admin/install/specials to force the 
##		creation of a new version.rel just incase, GV_VER
##		in CONFIG has changed for this target machine.
##	31-jan-1993 (dianeh)
##		Changed parameter to 'mk' from bulkfiles to files to
##		reflect change to ming rule in Mingbase.mk; changed
##		History comment-block to double pound-signs.
##	30-apr-1993 (dianeh)
##		Re-write for 6.5 -- note that this version doesn't
##		allow for specified facilities to be built -- it
##		just rebuilds it all (allowing for facilities to
##		be specified may come later).
##	03-may-93 (vijay)
##		Add mkshlibs to create shared libraries. Remove extraneous mk.
##	29-jun-93 (dianeh)
##		Change the way headers get built in common/hdr/hdr to reflect
##		the change in the targets list I made in the MINGS file there.
##      22-sep-93 (peterk)
##              add $MINGFLAGS to set ming flags from environment
##              e.g., -j# flag; fix make of front/embed/hdrs for baroque;
##		use +j to turn off parallel builds where needed in dirs with
##		grammars; do front facility hdrs directly for baroque;
##	12-oct-93 (peterk)
##		Changes to mk.sh and mkming.sh for baroque simplify running
##		mk at group level - group MING file includes facility hdrs,
##		so no need to go directly into facility hdrs now; combine
##		multiple mk's at same level into one call to mk; when mk'ing
##		dbutil hdrs do all hdrs in group, not just in duf/hdr.
##	19-oct-93 (peterk)
##		front/frame/valid needs +j for non-parallel for grammars,
##		add a few other +j's; fixed typo in build in abf.
##      27-oct-93 (vijay)
##              Make sure build no. gets into vers file.
##	27-oct-93 (vijay)
##		Undo above change. The default build no is known only to
##		readvers.
##	29-oct-93 (vijay)
##		Don't explicitly make pslgram.o etc since they take a looong
##		time to compile and may not actually be needed. Instead move
##		the +j flag to the whole directory.
##	30-oct-93 (peterk)
##		debug build option -dbg for back, cl, common, dbutil, gl,
##		and front/st
##	30-nov-93 (lauraw)
##		Add +j to "mk files" in common/hdr/hdr so the two ercompiles
##		don't get run simultaneously. Add checks for directories that
##		may not exist before cd-ing there.
##	10-jan-94 (vijay)
##		Don't use mk in common/hdr/hdr and dmf/specials_unix, since
##		these contain MINGS files. Use ming instead. mk has been 
##		changed to avoid MINGS using MINGS files.
##	01-feb-94 (lauraw)
##		Half of the change in the previous description was never
##		made, i.e., use ming instead of mk in common/hdr/hdr.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
#

usage='[ -b <build_no> ] [ -nomkmings ] [-dbg]

       Where:

          build_no      is the number of the in-house build level that
                        the "build=" line in VERS should be set to.

       NOTE: Specifying a build number is optional, but be sure the
             build number is up-to-date in VERS or else the version
             number will be wrong in INGRES.

       Use the -nomkmings flag to suppress the running of mkmings.
       WARNING: Use the -nomkmings flag with great care, since
                mkming can be needed for many reasons, not just
                added or deleted files (e.g., NEEDLIBS lines may
                have changed).

       Use the -dbg flag for debug builds of back, cl, common, dbutil, gl,
       and front/st.

       White-space between flags and their arguments is optional.
'

CMD=`basename $0`

# Check for required environment variables
: ${ING_SRC?"must be set"} ${ING_BUILD?"must be set"}

# Check for BAROQUE environment
[ "$ING_VERS" ] && { vers_src="/$ING_VERS/src" ; vers="/$ING_VERS" ; }

versfile=$ING_SRC/tools/port$vers_src/conf/VERS

[ -f $versfile ] ||
{ echo "$CMD: $versfile not found. Exiting..." ; exit 1 ; }

while [ $# -gt 0 ]
do
  case "$1" in
           -b*) if [ `expr $1 : '.*'` -eq 4 ] ; then
                  build_no="`expr $1 : '-b\(.*\)'`" ; shift
                else
                  [ $# -gt 1 ] ||
                  { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
                  build_no="$2" ; shift ; shift
                fi
                case "$build_no" in
                  [0-9][0-9]) ;;
                           *) echo "" ; echo "Illegal build number: $build_no"
                              echo "" ; echo "Usage:" ; echo "$CMD $usage"
                              exit 1
                              ;;
                esac
                ;;
	  -dbg) dbg="-f MINGDBG "
		shift
		;;
    -nomkmings) no_mkmings=true
                shift
                ;;
             *) echo "" ; echo "Usage: $CMD $usage" ; exit 1
                ;;
  esac
done

# Edit VERS
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

# Build the tools
echo "" ; echo '**** Running BOOT in tools/port/ming ...'
cd $ING_SRC/tools/port$vers_src/ming
sh BOOT
[ "$no_mkmings" ] ||
{
  echo "" ; echo '**** Generating MING files ...'
  mkmings
}
echo "" ; echo '**** Building tools/port ...'
cd $ING_SRC/tools/port$vers_src
mk $MINGFLAGS

# Invoke debug build starting here if opted for
oMINGFLAGS="$MINGFLAGS"
MINGFLAGS="$dbg$MINGFLAGS"

# Build libcompat.a and libmalloc.a
echo "" ; echo '**** Building libcompat.a ...'
cd $ING_SRC/cl/clf$vers_src
mk $MINGFLAGS lib
cd $ING_SRC/gl/glf$vers_src
mk $MINGFLAGS lib

# Build ercompile                       ## Links in libcompat.a and libmalloc.a
echo "" ; echo '**** Building ercompile, etc. ...'
cd $ING_SRC/cl/clf$vers_src/er_unix_win
echo "==== $ING_SRC/cl/clf$vers_src/er_unix_win: mk $MINGFLAGS"
mk $MINGFLAGS

# Build message and header files               ## Depends on ercompile, but this
echo ""                                        ## should change when *.msg files
echo '**** Building .mnx and header files ...' ## move.
cd $ING_SRC/common/hdr$vers/hdr
mkfecat
echo "==== $ING_SRC/common/hdr$vers/hdr: mk $MINGFLAGS files"
# Override any "-j" here or this gets out of sync
ming $MINGFLAGS +j files
ming erclf.h erglf.h erscf.h erusf.h

## NOTE: Depends on common/hdr/hdr ercompile'd .h files, but this is wrong
##       and should change once the *.msg files move.
# Rebuild cs library modules
echo "" ; echo '**** Rebuilding cl/clf/cs_unix modules ...'
cd $ING_SRC/cl/clf$vers_src/cs_unix
echo "==== $ING_SRC/cl/clf$vers_src/cs_unix: mk $MINGFLAGS lib"
mk $MINGFLAGS lib

# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
# Build eqmerge                         ## Depends on libcompat.a 
echo "" ; echo '**** Building eqmerge ...'
cd $ING_SRC/front/tools$vers_src/eqmerge
echo "==== $ING_SRC/front/tools$vers_src/eqmerge: mk $MINGFLAGS"
mk $MINGFLAGS

# Build iyacc                           ## Depends on libyacc.a & libcompat.a
echo ""                                 ## (Needed by front/embed)
echo '**** Building iyacc ...'
cd $ING_SRC/front/tools$vers_src/yycase
echo "==== $ING_SRC/front/tools$vers_src/yycase: mk $MINGFLAGS"
mk $MINGFLAGS
cd $ING_SRC/front/tools$vers_src/yacc
echo "==== $ING_SRC/front/tools$vers_src/yacc: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
echo "==== $ING_SRC/front/tools$vers_src/yacc: mk $MINGFLAGS files"
mk $MINGFLAGS files
echo "==== $ING_SRC/front/tools$vers_src/yacc: mk $MINGFLAGS"
mk $MINGFLAGS 

# Build front header files                        ## Depends on eqc & esqlc
echo "" ; echo '**** Building front/hdr/hdr header files ...'
cd $ING_SRC/front/hdr$vers/hdr                    ## (Needed by front/abf/hdr)
echo "==== $ING_SRC/front/hdr$vers/hdr: mk $MINGFLAGS hdrs"
mk $MINGFLAGS hdrs
echo "" ; echo '**** Building front header files ...'
cd $ING_SRC/front
mk $MINGFLAGS hdrs

# Build embedded language libraries                ## Depends on eqmerge, iyacc,
echo ""                                            ## & ercompile (needed by
echo '**** Building embedded-language libraries ...'    ## eqc & esqlc)
for d in copy dclgen libq libqgca libqxa sqlca equel
{
    cd $ING_SRC/front/embed$vers_src/$d
    echo "==== $ING_SRC/front/embed$vers_src/$d: mk $MINGFLAGS lib"
    mk $MINGFLAGS lib
}

cd $ING_SRC/front/embed$vers_src
echo "==== $ING_SRC/front/embed$vers_src: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib

# Build front utilities libraries                ## Needed by eqc & esqlc
echo "" ; echo '**** Building front utilities libraries ...'
for dir in afe fmt ug
{
  cd $ING_SRC/front/utils$vers_src/$dir
  echo "==== $ING_SRC/front/utils$vers_src/$dir: mk $MINGFLAGS lib"
  mk $MINGFLAGS lib
}

# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
# Build libadf.a                                 ## Needed by eqc & esqlc
echo "" ; echo '**** Building libadf.a ...'
cd $ING_SRC/common/adf$vers_src
mk $MINGFLAGS lib

# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
# Build eqc and esqlc
echo "" ; echo '**** Building eqc and esqlc ...'
for dir in c csq                          ## Depends on embedded-language libs,
{                                         ## front utilities libs, libadf.a &
  cd $ING_SRC/front/embed$vers_src/$dir   ## libcompat.a
  echo "==== $ING_SRC/front/embed$vers_src/$dir: mk $MINGFLAGS +j"
  mk $MINGFLAGS +j
}

## Depends on eqc & esqlc, so really should rebuild after eqc/esqlc have
## been rebuilt (because now they're new) -- which is why I don't build
## .qsh files until after eqc & esqlc are new (see build in common/hdr/hdr
## above -- but the .msg->.h in common/hdr/hdr should go away soon).
# Build header files
echo "" ; echo '**** Building common/hdr/hdr header files ...'
cd $ING_SRC/common/hdr$vers/hdr
echo "==== $ING_SRC/common/hdr$vers/hdr: ming $MINGFLAGS hdrs"
ming $MINGFLAGS hdrs
echo "" ; echo '**** Building dbutil header files ...'
cd $ING_SRC/dbutil
mk $MINGFLAGS hdrs
echo "" ; echo '**** Rebuilding front header files ...'
cd $ING_SRC/front
mk $MINGFLAGS hdrs

# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
# Build the backend yacc
echo "" ; echo '**** Building byacc ...'
cd $ING_SRC/back/psf$vers_src/yacc
echo "==== $ING_SRC/back/psf$vers_src/yacc: mk $MINGFLAGS lib"
mk $MINGFLAGS lib
echo "==== $ING_SRC/back/psf$vers_src/yacc: mk $MINGFLAGS"
mk $MINGFLAGS

# Build libpsf.a
echo "" ; echo '**** Building libpsf.a ...'

# XXX would be nice to limit +j to the *gram.o files
# to do this we need to leave them around in the directory
# perhaps need a modified DO.O.C which preserves them.

cd $ING_SRC/back/psf$vers_src/psl
echo "==== $ING_SRC/back/psf$vers_src/psl: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
echo "==== $ING_SRC/back/psf$vers_src/psl: mk $MINGFLAGS"
mk $MINGFLAGS

# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
echo "" ; echo '**** Building {common,dbutil,back} libraries ...'
for dir in common dbutil back            ## dbutil depends on eqc & esqlc
{
  cd $ING_SRC/$dir
  mk $MINGFLAGS lib
}
# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
echo "" ; echo '**** Building tools libraries ...'
cd $ING_SRC/tools
mk $MINGFLAGS lib

# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
# Build backend, dbutil, and common executables
echo "" ; echo '**** Building CL executables ...'
cd $ING_SRC/cl/clf$vers_src
mk $MINGFLAGS
cd $ING_SRC/gl/glf$vers_src
mk $MINGFLAGS
for dir in back dbutil common
{
  echo "" ; echo "**** Building $dir executables ..."
  cd $ING_SRC/$dir
  mk $MINGFLAGS
}
# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
for dir in admin tools 
{
  echo "" ; echo "**** Building $dir executables ..."
  cd $ING_SRC/$dir
  mk $MINGFLAGS
}
# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
# Build iimerge.o
echo "" ; echo '**** Building iimerge.o ...'
cd $ING_SRC/back/dmf$vers_src/specials_unix
echo "==== $ING_SRC/back/dmf$vers_src/specials_unix: ming $MINGFLAGS"
ming $MINGFLAGS

# Build iimerge and its associated executables
echo "" ; echo '**** Linking iimerge.o executables ...'
iilink -standard -noudt

# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
# Build collation files
echo "" ; echo '**** Building collation files ...'
cd $ING_SRC/common/adf$vers_src/adm
echo "==== $ING_SRC/common/adf$vers_src/adm: mk $MINGFLAGS"
mk $MINGFLAGS

# Build frontend files
echo "" ; echo '**** Building front-end files ...'
cd $ING_SRC/front
mk $MINGFLAGS files

# Build the remainder of the front-end libraries
echo "" ; echo '**** Building front-end libraries ...'
# build abf grammars +j for non-parallel
for dir in quel sql
{
    [ -d $ING_SRC/front/abf$vers_src/$dir ] && 
    {
        cd $ING_SRC/front/abf$vers_src/$dir
        echo "==== $ING_SRC/front/abf$vers_src/$dir: mk $MINGFLAGS +j lib"
        mk $MINGFLAGS +j lib
    }
}
# build frame/valid grammars +j for non-parallel
cd $ING_SRC/front/frame$vers_src/valid
echo "==== $ING_SRC/front/frame$vers_src/valid: mk $MINGFLAGS +j lib"
mk $MINGFLAGS +j lib
cd $ING_SRC/front
mk $MINGFLAGS lib

# Build shared libs if available
mkshlibs

# Build vec31
[ -d $ING_SRC/vec31 ] &&
{
  echo "" ; echo '**** Building vec31 ...'
  cd $ING_SRC/vec31
  mk $MINGFLAGS exe
  mk $MINGFLAGS files
}

# Build the remainder of the front-end executables
## NOTE: This can come before vec31, once vec31 makefiles are fixed.
echo "" ; echo '**** Building front-end executables ...'
cd $ING_SRC/front
mk $MINGFLAGS

# Build libingres.a
echo "" ; echo '**** Building libingres.a ...'
mergelibs

# Build testtools, SIG
for dir in testtool sig
{
  [ -d $ING_SRC/$dir ] &&
  {
    echo "" ; echo "**** Building $dir ..."
    cd $ING_SRC/$dir
    mk $MINGFLAGS hdrs lib files exe
  }
}

# Build tech
[ -d $ING_SRC/tech ] &&
{
  echo "" ; echo '**** Building technical notes ...'
  cd $ING_SRC/tech
  mk $MINGFLAGS
}

# Recheck the world...should come up clean
##NOTE: For now rungraph and vigraph will always relink after
##      building vec31 because of the makefiles in vec31.

# Turn debug builds on
MINGFLAGS="$dbg$MINGFLAGS"
for dir in cl common back dbutil 
{
  [ -d $ING_SRC/$dir ] &&
  {
    echo "" ; echo "**** Final check for $dir ..."
    cd $ING_SRC/$dir
    mk $MINGFLAGS
  }
}
# turn off dbg builds here
MINGFLAGS="$oMINGFLAGS"
for dir in tools vec31 front admin sig testtool tech
{
  [ -d $ING_SRC/$dir ] &&
  {
    echo "" ; echo "**** Final check for $dir ..."
    cd $ING_SRC/$dir
    mk $MINGFLAGS
  }
}

echo "$CMD done: `date`"
exit 0
