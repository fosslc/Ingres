:
#
#  Copyright (c) 2004, 2005 Ingres Corporation
#
#  Name: mkjam -- Generate Jamfile
#
#  Usage:
#       mkjam [dir1,dir2....,dirn]
#
#  Description:
#       This script can be used to generate Jamfiles in new facilities
#	or update existing Jamfiles when new files have been added
#
#  Exit Status:
#       0       Jamfile successfully created
#       1       Creation of Jamfile failed
#
#
## History: 
##	11-Jun-2004 (hanje04)
##	    Created.
##	14-Jul-2004 (hanje04)
##	    Define archive defined in shared libraries as we no longer
##	    source shlibinfo
##	14-Jul-2004 (hanje04)
##	    Add timezone files to known file types (.tz)
##	20-Jul-2004 (hanje04)
##	    Correct FILDIR for ice!plays!tutorialguide
##	19-Jul-2004 (kodse01)
##	    Made changes for Windows.
##	30-jul-2004 (somsa01)
##	    When cycling through directories on Windows, also change the
##	    Windows slash to the UNIX slash.
##	2-aug-2004 (stephenb)
##	    fix up for platform extemnsions in directory names
##	    don't build things not for my platform
##	05-Aug-2004 (kodse01)
##	    Look for the hint 'LIBRARY = <IMPORTDATALIB>' in source files
##	    to differentiate with regular library.
##	24-Aug-2004 (kodse01)
##	    Made changes to work with CYGWIN tools also in Windows.
##	24-Aug-2004 (drivi01)
##	    Modified case loop that parses the location string o search
##	    for _unix_win b/c otherwise it doesn't grab unix directories
##	    due to the fac that it never falls through to win case.
##	    Also modified conditions of the IIINClUDE statement to use
##	    the same case statement for location scan and search for
##	    unix, win, or VMS substring or always include if none of 
##	    those are present.
##      24-Aug-2004 (hanje04)
##        Update special rule for cl!clf!ck to reflect new dir name,
##        ck_unix
##	25-Aug-2004 (drivi01)
##	    Added rules for writing Jamfile in rpclean_win directory
##	    so that it makes a library out of a file.
##	26-Aug-2004 (bonro01)
##	    Update JAM build to support 64-bit hybrid build.
##	09-Sep-2004 (kodse01)
##	    ls */$JAMFILE behaves differently in GNUWin32. Added
##	    sed statement to make it work with GNUWin32 also.
##	22-Sep-2004 (drivi01)
##	    Added code to case statement for mkjam to ingore shell
##	    directories on windows. Added two new hints: NEEDLIBSW
##	    contains files that will be used specifically for compiling
##	    windows executable and will compliment NEEDLIBS; LIBOBJECT
##	    will specify which files need to be copied to II_SYSTEM/
##	    ingres/lib directory. Also when mkjam is run in directories
##	    with _unix or _win suffix the library name will be modified
##	    to omit the suffix.
##	13-Oct-2004 (schka24)
##	    Fix ! [ x = y ] --> [ x != y ]
##	15-Oct-2004 (bonro01)
##	    Remove code for old 64-bit hybrid technique.
##	27-Oct-2004 (drivi01)
##	    Some directories were renamed to directories with OS identifier
##	    at the end, this change is to reflect directory name changes.
##	4-Oct-2004 (drivi01)
##	    Changed directions of slashes on windows for $dirs variable,
##	    slashes were disolving causing no Jamfile in ING_SRC when 
##	    directory was passed as a parameter to mkjam.sh script.
##	12-jan-2005 (abbjo03)
##	    Do not create Jamfiles for front!embed.
##	21-Jan-2005 (hanje04)
##	    Add support language files under common!i18n
##	25-jan-2005 (abbjo03)
##	    Remove Vision tutorial.
##	10-Feb-2005 (bonro01)
##	    Add tst directory for test tools.
##	04-Apr-2005 (lakvi01)
##	    Ported for Windows AMD64 architecture (a64_win). 
##      08-Apr-2005 <gorvi01>
##        Ported for Windows IA64 architecture (i64_win).
##	13-Apr-2005 (bonro01)
##	    Prevent mkjam from deleting a read-only file by default.
##	    Add -f flag for force over-write of read-only file.
##	15-Apr-2005 (bonro01)
##	    Don't give "not writable" error if Jamfile does not exist.
##      20-Apr-2005 (loera01) SIR 114358
##          Renamed drivers_unix_vms to drivers, as Kerberos is now
##          supported on all platforms except AIX.
##      20-Jan-2006 (loera01) SIR 115615
##          Changed ODBC help page reference to IILIBODBCCFG.HLP.
##	21-Jun-2006 Ibonro01)
##	    Correctly build SubDir entries in Jamfiles under
##	    $ING_SRC/tst which is a symbolic link to $ING_TST
##	10-May-2007 (fanra01)
##	    Add a couple of commands to remove extraneously appended carriage
##	    returns on echo; especially for the cygwin utils on Windows.
##	    Additional character was causing some conditionals to fail.
##	29-May-2007 (drivi01)
##	    Update previous change, add another'\' to "\r" pattern.
##	    shell seems to strip out one '\' from a pattern resulting
##	    in a wrong result.
##	31-Jul-2007 (hanje04)
##	    BUG 118848
##	    Make sure we don't inlcude GPL pixmap directory if enterprise
##	    version is present.
##	13-Aug-2007 (bonro01)
##	    Fix building Solaris High Availability failover package.
##	30-May-2007 (hanje04)
##	    BUG 119214
##	    Remove '+1' from sort command when it's not supported as
##	    later versions of Linux don't support it.
##	25-Jan-2010 (kschendel) b123194
##	    Windows build issues: when building with Cygwin, make sure that
##	    key directory names are normalized (using cygpath) so that
##	    name comparisons work.  Also, Cygwin likes /dev/null for the
##	    null device, not c:/nul.  (Commands issued directly into the
##	    dos command interpreter need nul or nul:, of course, but mkjam
##	    is run via a unix-like shell.)
##	12-Aug-2010 (drivi01)
##	    Make sure that Jamfiles produced by cygwin 1.7 shell are
##	    readable my FTJam which means they need to be in the DOS
##	    format.  
##	    Add u2d commands to force Jamfiles to be in DOS format.
##

CMD=`basename $0`

[ -n "$ING_VERS" ] && noise=`tr -d '\r' "/$ING_VERS/src"`

usage='[-f] [ <dir> [<dir>...] ]


       where:

          -f         Force update of read-only Jamfile.

         <dir>       is the pathname of the directory(s) you want
                     MING files generated in.

       Examples:
       # Do source-level directories in front/st
         mkjam $ING_SRC/front/st/*
       # Do source-level directories and facility-level directory in front/st
         mkjam $ING_SRC/front/st/* $ING_SRC/front/st
       # Do only facility-level directory in tools/port and admin/install
         mkjam $ING_SRC/tools/port $ING_SRC/admin/install
'

: ${ING_SRC?"ING_SRC must be set"}
: ${config_string="$CONFIG_STRING"}

if [ "$config_string" = "int_w32" -o "$config_string" = "a64_win" -o "$config_string" = "i64_win" ] ; then
	WINDOWS="TRUE" ;
else
	WINDOWS="FALSE" ;
fi

   if [ "$WINDOWS" = "TRUE" ] ; then
    LS=ls
    ING_SRC=`cmd /C echo %ING_SRC% | sed 's:\\\:/:g'`
    ING_SRC=`echo $ING_SRC | tr "[:upper:]" "[:lower:]" | tr -d '\r'`
    DEVNULL=c:/nul
    if [ "$USE_CYGWIN" = "TRUE" -o "$TERM" = "cygwin" ] ; then
        PWDIR="cygpath -am ."
	ING_SRC=`cygpath -am $ING_SRC | tr "[:upper:]" "[:lower:]"`
	ING_TST=`cygpath -am $ING_TST | tr "[:upper:]" "[:lower:]"`
	DEVNULL=/dev/null
    else
        PWDIR=pwd
    fi
else
    LS=/bin/ls
    DEVNULL=/dev/null
    PWDIR=pwd
fi

LIBRTE="/lib/librte.so"

forceupdate=false
# Parse arguments, if any
for arg in $*
{
    case "$1" in
    -f)
	forceupdate=true
        ;;
    -*)
	echo "Illegal option: $1"
        echo "Usage:" ; echo "$CMD $usage" ; exit 1
        ;;
    *)
	dirs="$*" ; 
		if [ "$WINDOWS" = "TRUE" ] ; then
			dirs=`cmd /C echo $dirs | tr -d '\r' | sed 's:\\\:/:g'`
		fi
		break
        ;;
    esac
}


# platform
case "$config_string" in
    *_vms)	platform=vms ;;
    *_w32)  	platform=win ;;
    *_win)	platform=win ;;
    *)		platform=unix ;;
esac

# sed scripts

#reform='2,$s:^:	:;s:$: \\:;$s: \\$::'
#reform='2,$s,^,	,;'
if [ "$WINDOWS" = "TRUE" ] ; then
    reform='2,$s,^,	,;'
else
    reform='2,$s,^,	,;'
fi

libcheck='s/$(\(.*\))/$(\1:T=N)$(:E=\1 undefined)/'

if [ -x /usr/ucb/fmt ] || [ -x /usr/bin/fmt ] ; then
  fmt='fmt -64 | sed "$reform"'
else
  fmt='sed "$reform"'
fi

gethints=': loop
    /\\$/{; N; s/\\//; t loop
    }
    s:^\([A-Z][A-Z_]*\)[	 ]*=[	 ]*\(.*\):\1="\2" :p'

shhints=': loop
    /\\$/{; N; s/\\//; t loop
    }
    s:^#[	 ]*\(DEST\)[	 ]*=[	 ]*\(.*\):\1="\2" :p
    s:^#[	 ]*\(PROGRAM\)[	 ]*=[	 ]*\(.*\):\1="\2" :p
    s:^#[	 ]*\(MODE\)[	 ]*=[	 ]*\(.*\):\1="\2" :p'

# Default Jamfile name
jamdflt="Jamfile"
jamdbg="Jamdbg"

## check if +1 is a valid sort flag
plusone=+1
sort +1 /dev/null >/dev/null 2>&1 || plusone='-k 2'
export plusone


# Build the Jamfile files
for d in ${dirs-.}
{
 (
  [ -d "$d" ] || continue                   # Not a valid source directory
  case "$d" in
	*_unix_win*)           #need this case for directories common to unix 
			       #and windows (i.e. unix_win) b/c otherwise on 
			       #windows ext will always be unix for unix_win case.
		case "$config_string" in
			int_w32) ext=win ;;
			*_win) ext=win ;;
			*) ext=unix ;;
		esac
		;;
	*_unix*) ext=unix ;;
	*_vms*) ext=vms ;;
	*_win*) ext=win ;;
  esac
  if [ "$ext" ] && [ "$ext" != "$platform" ] ; then
	continue
  fi
  [ "$d" = "." ] && d=`$PWDIR`
  cd $d
  if [ "$WINDOWS" = "TRUE" ] ; then
     d=`echo $d | tr -d '\r' | sed 's:\\\:/:g' | tr "[:upper:]" "[:lower:]"`
  fi

  [ "$jamfile" ] || jamfile=$jamdflt
  # write out Jamdbg file for debug builds in all directories
  rm -f $jamdbg
  if [ "$WINDOWS" != "TRUE" ]
then
  cat << EOF >$jamdbg
/*
** INGRES Jamfile DBG file
*/

CCMACH = \$(CCMACH_DBG)
CCLDMACH = \$(CCLDMACH_DBG)
OPTIM = \$(OPTIM_DBG)

Include $jamfile
EOF
fi




if [ "$d" = $ING_SRC ] # ING_SRC is a simple case
then
    dir=ING_SRC
    set -$- ING_SRC
else
    dir=`$PWDIR`
    if [ "$WINDOWS" = "TRUE" ] ; then
        dir=`$PWDIR | tr "[:upper:]" "[:lower:]"`
    fi
    case "$dir" in
	$ING_SRC/*) 
		if [ "$WINDOWS" = "TRUE" ] ; then
                  dir=`echo $dir | sed "s%^$ING_SRC[/]*%%g"`
               else
                  dir=`echo $dir | sed "s:^$ING_SRC[/]*::"` # No Jamfile file for 
               fi
		[ "$dir" ] || continue                 # top-level src dir
                ;;                    
	$ING_TST/*)
		if [ "$WINDOWS" = "TRUE" ] ; then
                  dir=`echo $dir | sed "s%^$ING_TST[/]*%tst/%g"`
               else
                  dir=`echo $dir | sed "s:^$ING_TST[/]*:tst/:"` # No Jamfile file for 
               fi
		[ "$dir" ] || continue                 # top-level src dir
                ;;                    
             *) while expr $dir : ".*/.*" > $DEVNULL     # $ING_VERS path
                do
		  if [ "$WINDOWS" = "TRUE" ] ; then
                      dir=`echo $dir | sed "s%[^/]*/%%g"`
                  else
                      dir=`echo $dir | sed "s:[^/]*/::"`
                  fi
                  if [ -d $ING_SRC/$dir ] ; then
                    break
                  fi
                done
                ;;
  esac

# Tokenize the current directory name and put the pieces into the
# positional parameters ; then parse the pieces to determine group,
# facility, subsystem, and "action" (ACT).
# For example: back/dmf/dml is GROUP=$1, FACILITY=$2, SUBSYSTEM=$3;
# for BAROQUE paths: GROUP=$1, FACILITY=$2, SUBSYSTEM=$5

  [ -n "$ING_VERS" ] &&
  {
    vers='/$(ING_VERS)'
    sh_vers="/$ING_VERS"
    src='/src'
    ls_vers="/$ING_VERS/*"
  }

  if [ "$WINDOWS" = "TRUE" ] ; then
      set -$- `echo $dir | sed "s%/% %g"`
  else
      set -$- `echo $dir | sed "s:/: :g"`
  fi

fi

  GROUP=$1
  case $dir in
	      ING_SRC) # Top level ING_SRC directory
		       ACT=top 
		       ;;
               admin|\
                back|\
                  cl|\
              common|\
              dbutil|\
               front|\
                  gl|\
		  ha|\
                 sig|\
                tech|\
            testtool|\
		 tst|\
               tools|\
                w4gl|\
		x11src|\
		x11-src|\
                 vec*)
                       ACT=group
                       ;;
    */*/$ING_VERS/hdr) #SOURCE, 6.0 baroque
                       FACILITY=$2 ; SUBSYS=$4 ; ACT=source
                       ;;
    */*/$ING_VERS/src) #GROUP, 6.0 baroque
                       FACILITY=$2 ; ACT=facility
                       ;;
  */$ING_VERS/src/*/utmtemplates) #VTM templates for baroque
		       FACILITY=$2 ; SUBSYS=$5 ; COMPONENT=$6 ; ACT=source
		       ;;
*/*/$ING_VERS/src/*/*) #Extra level for baroque ICE
                       FACILITY=$2 ; SUBSYS=$5 ; COMPONENT=$6; ACT=demos
                       ;;
  */*/$ING_VERS/src/plays) #ICE plays -extra level here
                       FACILITY=$2 ; SUBSYS=$5 ; ACT=facility
                       ;;
  */*/$ING_VERS/src/samples) #ICE samples -extra level here
                       FACILITY=$2 ; SUBSYS=$5 ; ACT=facility
                       ;;
  */*/$ING_VERS/src/drivers) #GCF security mechanisms have extra level
                       FACILITY=$2 ; SUBSYS=$5 ; ACT=facility
                       ;;
  */*/$ING_VERS/src/*) #6.0 baroque; e.g. dmf, dml
                       FACILITY=$2 ; SUBSYS=$5 ; ACT=source
                       ;;
     */*/utmtemplates) #VTM templates
		       FACILITY=$2 ; SUBSYS=$3 ; COMPONENT=$4 ; ACT=source
		       ;;
              */*/*/*) #Extra level for ICE
                       FACILITY=$2 ; SUBSYS=$3 ; COMPONENT=$4 ; ACT=demos
                       ;;
           */*/plays|\
          */*/samples) #ICE demos -extra facility level here
                       FACILITY=$2 ; SUBSYS=$3 ; ACT=facility
                       ;;
          */*/drivers) #GCF security mechanisms have extra level
                       FACILITY=$2 ; SUBSYS=$3 ; ACT=facility
                       ;;
          */*/vdba) #GCF security mechanisms have extra level
                       FACILITY=$2 ; SUBSYS=$3 ; ACT=bfiles
                       ;;
  sig/rep/rpclean_win) #SIG needs a library here to build rpclean executable on windows
  			if [ "$WINDOWS" = "TRUE" ] ; then
				FACILITY=$2 ; SUBSYS=$3 ; ACT=slib
			else
				FACILITY=$2 ; SUBSYS=$3 ; ACT=sfiles
			fi
			;;
                sig/*/*) #SIG bulk copy the files by default
                       FACILITY=$2 ; SUBSYS=$3 ; ACT=sfiles
                       ;;
                */*/*) #6.0
                       FACILITY=$2 ; SUBSYS=$3 ; ACT=source
                       ;;
                  */*) #6.0 cl                    ## What does this mean -- cl?
                       SUBSYS=$2 ; ACT=facility   ## Is cl somehow different?
                       ;;
  esac

# Remove unix-specific part of directory name ? WHY removed pending answer
#   echo $SUBSYS | grep -s '_unix' && SUBSYS=`echo $SUBSYS | sed s/_unix//`
# Set uppercase subsys (USUBSYS) for prepending to LIB
# and initial-cap subsys (LSUBSYS) for w4gldemo stuff
  [ "$SUBSYS" ] &&
  { 
    USUBSYS="`echo $SUBSYS | tr '[a-z]' '[A-Z]'`"
    LSUBSYS="`expr $SUBSYS : '\(.\).*' | \
              tr '[a-z]' '[A-Z]'``expr $SUBSYS : '.\(.*\)'`"
  }
    UFACILITY=`echo $FACILITY | tr '[a-z]' '[A-Z]'`

  : ${ACT=source}

# Set generic values for BINDIR and FILDIR
  BINDIR='$(INGBIN)' ; FILDIR='$(INGFILES)' ; MAN1DIR='$(TOOLSMAN1)'

# Point LIB variable appropriately 
# HDR's defined in Jamrules file.

  LIB='${LIBRARY}'
  LCLLIB=${USUBSYS}LIB
  use_local_lib=false

  case $GROUP in
        back) LIB=${UFACILITY}LIB
              ;;
          cl) LIB=COMPATLIB
              ;;
          gl) LIB=COMPATLIB
              ;;
      common) LIB=${UFACILITY}LIB
              ;;
      dbutil) LIB=DBUTILLIB
              ;;
       front) LIB=${USUBSYS}LIB
		case $FACILITY in
		    st) DEST=utility ;;
		esac
              ;;
         sig) 
	      FILDIR=$SUBSYS
              ;;
    testtool) case $FACILITY in
		achilles) LIB=${UFACILITY}LIB
			  ;;
		    lar|\
		    sep)  BLDRULE=IITOOLSEXE
		          LIB=${USUBSYS}LIB
			  ;;
		       *) LIB=${USUBSYS}LIB
              		  ;;
	      esac
              ;;
       tools) LIB=${USUBSYS}LIB
              ;;
        w4gl) LIB=${USUBSYS}LIB
	      case $SUBSYS in
		[dD]ata|[eE]xports)
			FILDIR='$(W4GLDEMO)'
			ACT=files;;
		[bB]itmaps)
			FILDIR='$(W4GLDEMO)'
			ACT=files;;
		[sS]trings)
			FILDIR='$(W4GLDEMO)'
			ACT=files;;
		[eE]xternal)
			FILDIR='$(W4GLSAMP)'/extevent
			SPECFILES='w4glext.c w4glext.h extevent.c';;
		dictfile)
			FILDIR=IIDICTFILES
			ACT=files;;
		*)
              		;;
	       esac
              ;;
           *) MAN1DIR='$(TECHMAN1)'
              LIB=${USUBSYS}LIB
              ;;
  esac

# special cases

  case $GROUP in
        back) case $SUBSYS in
                yacc) LIB=BYACCLIB ;;
              esac
              ;;
          cl) case $SUBSYS in
                      ci*|\
                      cs*|\
                    csmt*|\
                      lg*) BINDIR='$(INGUTIL)' ;;
                      sd*) LIB=SDLIB ;;
              esac
              ;;
      common) case $FACILITY in
		adf) case $SUBSYS in
		      adn) FDIR=ucharmaps ;;
		     esac
		     ;;
		aif) case $SUBSYS in
		       acm|\
		       saw) LIB=${USUBSYS}LIB ;;
		       ait) use_local_lib=true ;;
			 *) LIB=APILIB ;;
		     esac
		     ;;
		ddf) case $SUBSYS in
		       ddi) LIB=DDILIB ;;
		     esac
		     ;;
                odbc) case $SUBSYS in
                       config_unix) LIB=ODBCCFGLIB${LP64} ;;
		       config_win) LIB=ODBCCFGLIB${LP64} ;;
                       manager) LIB=ODBCMGRLIB${LP64} ;;
                      esac
                      ;;
		gcf) case $SUBSYS in
		       gcc|gcd|gcn) use_local_lib=true ;;
                        gcj) ACT=bfiles ; FILDIR= ;;
			files) ACT=dfiles ; FILDIR=charsets ;;
		     esac
		     ;;
	       i18n) ACT=dfiles ; FILDIR=$SUBSYS ;;
	      esac
	      ;;
        tech) case $SUBSYS in
                advisor) FILDIR=ADVISORFILES; ACT=files ;;
              esac
              ;;
       front) case $FACILITY in
		 ice) 
		     FILDIR="$SUBSYS"
		     case $SUBSYS in
			    DTD) ACT=icedemo ; FILDIR=$SUBSYS
				 ;;
		      iceclient) LIB=ICECLILIB
				 ;;
		       ICEStack) LIB=ICECONFLIB
				 ;;
			icetool) ACT=icedemo
				 ;;
		       icetutor) ACT=icedemo
				 ;;
			 images) ACT=icedemo
				 ;;
			  plays) FILDIR="$SUBSYS"
				 case $COMPONENT in
				    demo) ACT=icedemo
				         ;;
				    tutorialGuide) ACT=icedemo
			  			   FILDIR="$SUBSYS/$COMPONENT"
				         ;;
				 esac
				 ;;
			 public) ACT=icedemo
				 ;;
		        samples) 
			  	FILDIR="$SUBSYS/$COMPONENT"
				case $COMPONENT in
			             app) ACT=icedemo
				         ;;
			             dbproc) ACT=icedemo
				         ;;
			             query) ACT=icedemo
				         ;;
			             report) ACT=icedemo
				         ;;
				 esac
				 ;;
			 scripts) 
				 ACT=icedemo ; FILDIR="$SUBSYS"
				 ;;

		     esac
		     ;;
		 *) case $SUBSYS in
			abfdemo) FILDIR=$SUBSYS ; ACT=dfiles
				 ;;
		       dictfile) ACT=dfiles ; FILDIR=dictfiles
				 ;;
			  files) ACT=files
				 ;;
			 frame|\
			  valid) LIB=FDLIB
				 ;;
		       install*) LIB=INSTALLLIB
				 ;;
			   libq) LIB=LIBQLIB
				 ;;
		      macrodemo) FILDIR=IIICEMACRODEMO; ACT=demos
				 ;;
			report*) LIB=REPORTLIB
				 ;;
		       repfiles) ACT=dfiles ; FILDIR=rep
				 ;;
			 tbacc|\
			runtime) LIB=RUNTIMELIB
				 ;;
			termdr|\
			     ft) LIB=FTLIB
				 ;;
			   vdba) FILDIR=$SUBSYS
				 ;;
			    vt*) LIB=VTLIB
				 ;;
			webdemo) FILDIR=IICEWEBDEMO ; ACT=demos
				 ;;
		      esac
		      ;;
		  esac
		  ;;
        ha) case $FACILITY in
		 sc) case $SUBSYS in
		       specials) FILDIR=IIHAAGENT ; ACT=demos ;;
		     esac
		     ;;
             esac
             ;;
  esac

  case " $COMPONENT" in
     " ") echo "$GROUP!$FACILITY!$SUBSYS: doing $ACT" | sed "s/!!/!/;s/!:/:/" >&2
          ;;
       *) echo "$GROUP!$FACILITY!$SUBSYS!$COMPONENT: doing $ACT" | sed "s/!!/!/;s/!:/:/" >&2
          ;;
  esac


  #
  # If library reflects directory name remove platofrm identifier 
  # from a library name if directory has a suffix which is a 
  # platform identifier.
  #
  case "$LIB" in 
	*_UNIX_WIN*) LIB=`echo $LIB | sed "s/_UNIX_WIN//g"`
			;;
	*_UNIX*)     LIB=`echo $LIB | sed "s/_UNIX//g"`
			;;
	*_VMS*)      LIB=`echo $LIB | sed "s/_VMS//g"`
			;;
	*_WIN*)      LIB=`echo $LIB | sed "s/_WIN//g"`
			;;
  esac

export GROUP FACILITY SUBSYS COMPONENT
[ "$GROUP\!$SUBSYS" = "front\!embed" ] && exit

  [ -f $jamfile -a ! -w $jamfile -a "$forceupdate" = "false" ] &&
  {
	echo "$jamfile is not writable."
	exit 1
  }
  JAMFILE=$jamfile
  rm -f $JAMFILE

# Write out Jamfile file

# Start with header
  (
  echo "#"
  echo "# Jamfile file for $GROUP!$FACILITY!$SUBSYS!$COMPONENT" |
	    sed "s/[!][!][!]*/!/;s/!$//"
  echo "#"
  echo ""

# Tell jam we are part of a bigger picture
  [ "$GROUP" = ING_SRC ] && eval GROUP=''
  echo "SubDir ING_SRC $GROUP $FACILITY $SUBSYS $COMPONENT ;" | \
	sed "s/[ ][ ]* / /g"
  echo ""

# Preserve MANIFEST files. If one exists, include it and move on
  [ -f MANIFEST ] && {
  echo "include `$PWDIR`/MANIFEST ;"
  continue

  }

# Calculate hdr and other information for directory
  [ "$GROUP" ] &&
  {
  echo "IISUBSYS $GROUP $FACILITY $SUBSYS $COMPONENT ;" | sed "s/[ ][ ][ ]*/ /g"
  echo ""
  }


  case $ACT in
# FIXME!!!!!!
# Will not work for baroque environment
#  group for loc in  `($LS */$JAMFILE 2>$DEVNULL | cut -d/ -f1 )` 
#	    {
#		echo "IIINCLUDE $GROUP $FACILITY $SUBSYS $COMPONENT $loc ;" | \
#		sed "s/  / /g"
#	    }
#            ;;
      top|\
    group|\
  facility) for loc in  `($LS */$JAMFILE 2>$DEVNULL | sed 's:\\\:/:g' | cut -d/ -f1 )` 
	    {
		# Special case check for front!st!gpl_pixmap_unix_win which 
		# should not be included if front!st!enterprise_pixmap_unix_win
		# exists. gpl_pixmap_unix_win is a replacement for the non
		# GPL icons contained in enterprise_pixmap_unix_win which
		# cannot be distributed under GPL
		case "$loc" in
		    *gpl_pixmap*)
			[ -f ./enterprise_pixmap_unix_win/$JAMFILE ] &&
			    continue
			    ;;
		esac
		  #this case sorts location directories to make sure 
		  #that only a subset of directories included in the 
		  #Jamfile, for example on windows only unix_win and 
		  #_win will be included and also any directory with 
		  #no _unix, _windows, or _vms string in location string.
		  case "$loc" in 
			*_unix_win*) 
				case "$config_string" in
					int_w32) ext=win ;;
					*_win) ext=win ;;
					*) ext=unix ;;
				esac
				;;
			*_unix*) ext=unix ;;
			*_vms*) ext=vms ;;
			*_win*) ext=win ;;
			ha)	# ha is only for Solaris
				case "$config_string" in
					su9_us5) ext=unix ;;
					a64.sol) ext=unix ;;
					*) ext=skip ;;
				esac
				;;
			*) ext= ;;
		  esac
		if [ "$ext" = "$platform" ] || [ -z "$ext" ] ; then
		echo "IIINCLUDE $GROUP $FACILITY $SUBSYS $COMPONENT $loc ;" | \
		sed "s/[ ][ ][ ]*/ /g"
		fi
	    }
            ;;
     file) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" `
	      for f in $FILES
	      {
              echo "IIFILE $FILDIR/$f : $f ;"
	      }
            ;;
# CHECKME
#    bulk) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" | \
#                        eval $fmt `
#             echo "$FILDIR : $FILES ;"
     files) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" `
              echo "IIFILES $FILES ;" | eval $fmt
            ;;
     bfiles) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" `
              echo "IIBFILES $FILDIR : $FILES ;" | eval $fmt
            ;;
     dfiles) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" `
              echo "IIDFILES $FILDIR : $FILES ;" | eval $fmt
            ;;
       slib) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" `
	      echo "IILIBRARY SIGLIB : $FILES ;" | eval $fmt
	      #echo "IISIGFILES $FILDIR : $FILES ;" |eval $fmt
	    ;;
     sfiles) FILES=`$LS | egrep -v "^MING|OUT|^MANIFEST|^Jam*|.*\.sav" `
              echo "IISIGFILES $FILDIR : $FILES ;" | eval $fmt
            ;;
   icedemo) FILES=`$LS | egrep -v "^MING|OUT|^Jam*|.*\.sav"`
            for file in $FILES
            {
              case  $file in 
		     Makefile.ccpp)
			echo "IIICEDEMOMK $FILDIR/Makefile : $file ;"
		     ;;
		     ingres.dtd) # special case
              	        echo "IIICEFILE $FILDIR/$file : $file ;"
			echo "IIFILE $file : $file ;"
			echo ""
		     ;;
                     *.c)
                     MAIN=`egrep -l "^main *\(" $file 2>$DEVNULL`
		     if [ -n "$MAIN" ]
		     then
			eval `sed -n "$gethints" $MAIN`
                     	case $DEST in
                		utility) BLDRULE=IIUTILEXE
                         	;;
                    		bin) BLDRULE=IIBINEXE
                         	;;
                    		lib) BLDRULE=IILIBEXE
                         	;;
                  		tools) BLDRULE=IITOOLSEXE
                         	;;
                  		files) BLDRULE=IIFILES
                         	;;
                 		icebin) BLDRULE=IIICEBINEXE
                         	;;
                 		schabin) BLDRULE=IISCHABINEXE
                         	;;
                 		vcshabin) BLDRULE=IIVCSHABIN
                         	;;
                      		*) [ -z "$BLDRULE" ] && BLDRULE=IIBINEXE
                         	;;
              	     	esac
                     	echo $BLDRULE $PROGRAM : $file ;
		     else
			LIBS="$(LIBS) $file"
		     fi
              	     echo "FILE $FILDIR/$file : $file ;"
		     ;;
		     *)
              	     echo "IIICEFILE $FILDIR/$file : $file ;"
                     ;;
              esac
            }
            [ "$LIBS" ] &&
            {
		echo ""
           	ARCHIVE=`echo "$LIB" | sed "$libcheck"`
              	echo "IILIBRARY $ARCHIVE : $LIBS ;" | eval $fmt
              	echo ""
            }
	    [ ! -n "$LIBS" ] && echo ""
            ;;
     group) for loc in `( $LS *$ls_vers/MANIFEST 2>$DEVNULL )` 
	    {
		echo "IIINCLUDE $GROUP `dirname $loc` ;" | \
		sed "s,/, ,g;s/  / /g"
	    }
            ;;
    source)
#	    [ "$GROUP" = tools ] &&
#            echo "CCDEFS: (BSD42) (SYS5) (DIRFUNC) (NO_DIRFUNC) (DIRENT)"
#            [ "$DEFLIB" ] && echo "$DEFLIB"
#            [ "$GROUP" = tools ] && [ -n "$DEFLIB" -o -n "$GROUP" ] && echo ""
#            [ "$LOCAL_CC" ] && echo "CC = $LOCAL_CC"
#	    [ "$SOURCE_SH" ] && echo "SOURCE_SH = $SOURCE_SH"
#	    [ "$SOURCE_QSH" ] && echo "SOURCE_QSH = $SOURCE_QSH"

            # Set suffixes
            SRCSUF=" \
                    *.c
                    *.lex \
                    *.lfm \
                    *.qc  \
                    *.qsc \
                    *.roc \
                    *.s   \
                    *.sc  \
                    *.st  \
                    *.y   \
                    *.yi  \
                    *.yf  \
                    *.sy  \
                    *.ccpp \
                    "
            ## Get filenames
            # Remove generated files from target list:
            # The higher the number in the suf array, the higher the
            # precedence over files with lower-numbered suffixes;
            # e.g., a .qsc file has a higher precedence than a .sc and/or
            # a .c file with the same base filename -- i.e., remove files
            # with a "lower precedence" (generated files) from the
            # "target list" that appear more than once in the 'ls' output
            # (as an added bonus -- also remove y.tab.c files).
            FILES="`$LS $SRCSUF 2>$DEVNULL`"
	    if [ "$WINDOWS" != "TRUE" ] ; then
            TARGETS=`echo $FILES | tr ' ' '\012' | \
                     awk 'BEGIN { suf["c"] = 1 ;
                                  suf["sc"] = 2 ;
                                  suf["qc"] = 3 ;
                                  suf["qsc"] = 4 ;
                                  suf["lex"] = 5;
                                  suf["lfm"] = 6;
                                  suf["roc"] = 7;
                                  suf["s"] = 8;
                                  suf["y"] = 9;
                                  suf["yi"] = 10;
                                  suf["yf"] = 11;
                                  suf["sy"] = 12;
                                  suf["st"] = 13;
                                }
                   {
                     split($0, filename, ".")
                     if (filename[1] == "y" && filename[2] == "tab")
                       next
                     if (target[filename[1]] < suf[filename[2]])
                       target[filename[1]] = suf[filename[2]]
                   }
                   END { 
                   for (i in target)
                   { 
                     if (target[i] == 1)
                       print i ".c"
                     if (target[i] == 2)
                       print i ".sc"
                     if (target[i] == 3)
                       print i ".qc"
                     if (target[i] == 4)
                       print i ".qsc"
                     if (target[i] == 5)
                       print i ".lex"
                     if (target[i] == 6)
                       print i ".lfm"
                     if (target[i] == 7)
                       print i ".roc"
                     if (target[i] == 8)
                       print i ".s"
                     if (target[i] == 9)
                       print i ".y"
                     if (target[i] == 10)
                       print i ".yi"
                     if (target[i] == 11)
                       print i ".yf"
                     if (target[i] == 12)
                       print i ".sy"
                     if (target[i] == 13)
                       print i ".st"
                   }
                 }' | sort -t'.' -r ${plusone}`
            else
                    TARGETS=`echo $FILES | tr ' ' '\012' | \
                     awk 'BEGIN { suf["c"] = 1 ;
                                  suf["sc"] = 2 ;
                                  suf["qc"] = 3 ;
                                  suf["qsc"] = 4 ;
                                  suf["lex"] = 5;
                                  suf["lfm"] = 6;
                                  suf["roc"] = 7;
                                  suf["s"] = 8;
                                  suf["y"] = 9;
                                  suf["yi"] = 10;
                                  suf["yf"] = 11;
                                  suf["sy"] = 12;
                                  suf["st"] = 13;
                                }
                   {
                     split($0, filename, ".")
                     if (filename[1] == "y" && filename[2] == "tab")
                       next
                     if (target[filename[1]] < suf[filename[2]])
                       target[filename[1]] = suf[filename[2]]
                   }
                   END {
                   for (i in target)
                   {
                     if (target[i] == 1)
                       print i ".c"
                     if (target[i] == 2)
                       print i ".sc"
                     if (target[i] == 3)
                       print i ".qc"
                     if (target[i] == 4)
                       print i ".qsc"
                     if (target[i] == 5)
                       print i ".lex"
                     if (target[i] == 6)
                       print i ".lfm"
                     if (target[i] == 7)
                       print i ".roc"
                     if (target[i] == 8)
                       print i ".s"
                     if (target[i] == 9)
                       print i ".y"
                     if (target[i] == 10)
                       print i ".yi"
                     if (target[i] == 11)
                       print i ".yf"
                     if (target[i] == 12)
                       print i ".sy"
                     if (target[i] == 13)
                       print i ".st"
                   }
                 }'`
            fi

            YIS=`$LS *.yi 2>$DEVNULL`
            for i in $YIS
            {
              ROC=`sed -n '/.*TABLE_FILE.*-y/ {
              s///p
              q
              }' $i`
              TARGETS="$TARGETS $ROC"
            }
# CHECKME
	    CPP=`$LS *.cpp 2>$DEVNULL`
	    [ "$CPP" ] && [ "$TARGETS" ] && TARGETS="$TARGETS $CPP"
	    [ "$CPP" ] && [ ! "$TARGETS" ] && TARGETS="$CPP"
# TILL HERE

            # Sort out main modules (and partial-loads)
            MAINS=
            [ "$TARGETS" ] && MAINS=`egrep -l "^main *\(" $TARGETS 2>$DEVNULL`
            PARTIAL_LDS=
            [ "$MAINS" ] && 
              PARTIAL_LDS=`egrep -l "^MODE.*=.*.*PARTIAL_LD" $MAINS`
            [ "$PARTIAL_LDS" ] && 
              MAINS=`echo "$MAINS" | fgrep -v -x "$PARTIAL_LDS"`
            # Pyramid fgrep fails on empty expression
            [ "$MAINS" ] || MAINS=" "

            # Determine the library source modules
            IMPDATALIBS=
            [ "$TARGETS" ] && 
              IMPDATALIBS=`egrep -l "^LIBRARY[	 ]*=.*" $TARGETS`

	    LIBS=`echo "$TARGETS" | fgrep -v -x "$MAINS"`
	    LIBS=`echo "$LIBS" | fgrep -v -x "$IMPDATALIBS"`

            # Determine the import data library source modules
            for i in $IMPDATALIBS
            {
                LIBRARY=
                IMPDATALIB=
                eval `sed -n "$gethints" $i`
                [ "$LIBRARY" ] && IMPDATALIB="$LIBRARY" && break
            }

	    #Determine library object files
	    LIBOBJECTS=
	    [ "$TARGETS" ] &&
	      LIBOBJECTS=`egrep -l "^LIBOBJECT[   ]*=.*" $TARGETS`

	    LCL_LIBS=

	    [ "$use_local_lib" = "true" ] &&
	    {
		[ "$LCLLIB" ] && LOCALLIB=$LCLLIB
		[ "$LCLLIB" ] || LOCALLIB=LOCALLIB
		case $FACILITY:$SUBSYS in 
		    gcf:gcn) LCL_LIBS=`echo "$LIBS" | egrep "^gcns"`
			     LIBS=`echo "$LIBS" | egrep -v "^gcns"`
			     ;;
		          *) LCL_LIBS="$LIBS"
		       	     LIBS=
		             ;;
		esac
	    }
# CHECKME
	    # Set "NO_OPs" for problem files
	    
   	    for i in $TARGETS
	    {
                NO_OPTIM=
	        eval `sed -n "$gethints" $i`
		[ "$NO_OPTIM" ] && echo "IINOOPTIM $i : $NO_OPTIM ;"
	    }
# TILL HERE

            # Determine .SHELL targets versus .HDR targets
            for file in `$LS *.sh 2>$DEVNULL`
            {
              read char junk < $file
              [ "$char" = ":" ] &&
              { SHELLS="$SHELLS $file" ; continue ; }
              [ -f `basename $file .sh`.qsh ] ||
              SHS="$SHS $file"
            }

	    # Get lists of other file types
	    CCPP=`$LS *.ccpp 2>$DEVNULL`
	    DEF=`$LS *.def 2>$DEVNULL`
	    DTD=`$LS *.dtd 2>$DEVNULL`
            FRMS=`$LS *.frm 2>$DEVNULL`
            HLPS=`$LS *.hlp 2>$DEVNULL`
            HDRS=`$LS *.h 2>$DEVNULL`
            MAN1S=`$LS *.1 2>$DEVNULL`
            MSGS=`$LS *.msg 2>$DEVNULL| eval $fmt`
            PARS=`$LS *.par 2>$DEVNULL| eval $fmt`
            QHS=`$LS *.qh 2>$DEVNULL`
            QSHS=`$LS *.qsh 2>$DEVNULL`
            TFS=`$LS *.tf 2>$DEVNULL`
	    TMPLT=`$LS *.template 2>$DEVNULL`
	    RC=`$LS *.rc 2>$DEVNULL`
	    SQL=`$LS *.sql 2>$DEVNULL`
            VTMTMPLT=`$LS *.tpl 2>$DEVNULL` 
	    TZ=`$LS *.tz 2>$DEVNULL`
	    VTMCFG=`$LS *.cfg 2>$DEVNULL`
	    VTMJAR=`$LS UTMProject.jar 2>$DEVNULL`
            VTMZIP=`$LS aquathemepack11.zip 2>$DEVNULL`
	    [ "$FACILITY:$SUBSYS" = "utm:vtmhelp" ] && \
	    	VTMHELP=`$LS *.gif *.html *.css *.js *.ids 2>$DEVNULL`
	    XML=`$LS *.xml 2>$DEVNULL`
	
	# setting up shared library builds: 
	# add PIC flag to cl|gl|front|common directories.

	# shlibinfo may not be available and runnable (since it calls ming)
	if [ "$WINDOWS" != "TRUE" ] ; then
	    if [ "$GROUP" = "tools" ] 
	    then
	       use_shared_libs=false
	    else
	       use_shared_libs=true
	       MCOMPAT_LIBS="COMPATLIB "
	       MLIBQ_LIBS="UILIB LIBQSYSLIB LIBQLIB COPYLIB LIBQGCALIB GCFLIB \
			UGLIB FMTLIB AFELIB ADFLIB CUFLIB SQLCALIB LIBQXALIB "
	       MFRAME_LIBS="ABFRTLIB GENERATELIB OOLIB UFLIB RUNSYSLIB RUNTIMELIB \
			FDLIB MTLIB VTLIB FTLIB FEDSLIB "
	       MINTERP_LIBS="ILRFLIB IOILIB IAOMLIB INTERPLIB "
	    fi
	else
	   use_shared_libs=true
        fi

	if $use_shared_libs
	then
        [ "$LIBS" ] && [ "" ] && # Don't process PIC flags, remove once moved to
			      # Jamrules
	{
	     lib=`echo $LIB | tr -d '$()'`
	     add_pic='CCDEFS += $(CCPICFLAG)'
	     netscape_api='CCDEFS += $(NETSCAPE_API_FLAG)'
	     apache_api='CCDEFS += $(APACHE_API_FLAG)'
	     case "$GROUP" in
		    cl)	case $SUBSYS in
			  ck|\
			  jf|\
			  sr) if [ X$conf_SVR_SHL = Xtrue ] 
			      then echo $add_pic; fi ;;
			   *) echo $add_pic ;;
			esac ;;
		    gl)	echo $add_pic ;;
		common) echo $add_pic ;;
		  back)	if [ X$conf_SVR_SHL = Xtrue ] 
			then echo $add_pic; fi ;;
	        dbutil) echo $add_pic ;;
	          w4gl) echo $add_pic ;;
		 front)	if (echo " $MLIBQ_LIBS $MFRAME_LIBS $MINTERP_LIBS $MICESGADI_LIBS $MICEISAPI_LIBS $MICENSAPI_LIBS" |
			    grep "[ |	]$lib " >$DEVNULL )
			then  echo $add_pic; fi ;

			case $SUBSYS in
			   raat) echo $add_pic ;;
			   iceclient) echo $add_pic ;;
			   gcnext) echo $add_pic ;;
			   jni) echo $add_pic ;;
			   netscape)	if [ X$conf_SVR_SHL = Xtrue ]
                        		then
                            			echo $add_pic
					fi
					echo $netscape_api 
					;;
			   apache) if [ X$conf_SVR_SHL = Xtrue ]
                                        then
                                                echo $add_pic
                                        fi
					echo $apache_api 
					;;
			esac 
			;;
	     esac
	}
	fi

	if [ "$SUBSYS" = "ascd" ] ; then
# REMOVE ME!!!!
#	    echo 'CCLDMACH += $(LD_ICE_FLAGS)'

	    echo ""
	fi

	    echo ""
            for i in $MAINS
            {
              PROGRAM=`expr $i : '\([^.]*\)'`
              NEEDLIBS=
	      NEEDLIBSW=
	      NEEDOBJ=
              MODE=
              UNDEFS=
              SYMS=
	      DEST=
	      case "$GROUP" in
		tools) DEST=tools ;;
		*)
	          case "$FACILITY:$SUBSYS" in
	    	    clf:cs*| \
	    	    clf:ci*| \
	     install:shell_unix*)
			DEST=utility ;;
	          esac 
		  ;;
	      esac


# Use the "gethints" sed script to assign values to variables from the
# main source module --
# (e.g., MODE = SETUID in source module will assign the value here)
              eval `sed -n "$gethints" $i`
# Apparently tools and testtools don't need libcompat.a (and neither does
# something that's already mentioned it in their NEEDLIBS hint), but give
# it to everyone else, even if they didn't specify it.
	      if [ "$conf_EDBC" = "true" ]
	      then
		PROGRAM=`echo "$PROGRAM" | sed 's/(PROG0PRFX)/edbc/' |
		sed 's/(PROG1PRFX)/edbc/' | sed 's/(PROG2PRFX)/edbc/' |
		sed 's/(PROG3PRFX)/ED/' | sed 's/(PROG4PRFX)/ed/'`
	      else
		PROGRAM=`echo "$PROGRAM" | sed 's/(PROG0PRFX)//' |
		sed 's/(PROG1PRFX)/ii/' | sed 's/(PROG2PRFX)/ing/' |
		sed 's/(PROG3PRFX)/II/' | sed 's/(PROG4PRFX)/ii/'`
	      fi

              case "$GROUP:$NEEDLIBS" in 
                    tools:*|\
                 testtool:*|\
                       ha:*|\
                  *:*COMPAT*) ;;
                           *) NEEDLIBS="$NEEDLIBS COMPATLIB"
                              ;;
              esac

	      # If there is a local library, add it to NEEDLIBS
	      [ "$use_local_lib" = "true" ] && NEEDLIBS="$LOCALLIB $NEEDLIBS"

# Add XAU4LIB to NEEDLIBS when in the Xau subsystem of X

	      if [ $SUBSYS = "Xau" ]
	      then
		  NEEDLIBS="$NEEDLIBS XAU4LIB";
	      fi

	# Convert all library names to the appropriate shared library names.
	# Executables under st should be able to run before the installation
	# of shared libs. Some of them (eg. ipm, netutil) need to link with
	# dmf, di and other objects not available in the shared libs.
	# Therefore do not link them to shared libs.
	# Also tools, esql etc are needed before sh libs are available.
	# dict/exe's are called by setuid upgradedb, so cannot use shared libs.
	# 

	[ "$GROUP" = "front" ] &&
	{
	    case "$FACILITY:$SUBSYS" in
	    dict:* | st:* | tools:* | frame:compfrm | misc:chinst )
		;;
	    embed:ada | embed:adasq )
		;;
	    embed:cobol | embed:cobolsq )
	    	;;
	    embed:c | embed:csq )
	    	;;
	    embed:fortran | embed:fortransq )
		;;
	    web:web )
		;;
	    web:install )
		;;

	    *)
		need_compat=false need_libq=false
		need_frame=false need_interp=false
		NON_SHLIBS=
		for lib in $NEEDLIBS
		do
		   if (echo " $MCOMPAT_LIBS "|grep "[ |	]$lib " >$DEVNULL)
		   then 
			need_compat=true
		   elif (echo " $MLIBQ_LIBS "|grep "[ |	]$lib " >$DEVNULL)
		   then
			need_libq=true
		   elif (echo " $MFRAME_LIBS "|grep "[ |	]$lib " >$DEVNULL)
		   then
			need_frame=true
		   elif (echo " $MINTERP_LIBS "|grep "[ |	]$lib " >$DEVNULL)
		   then
			need_interp=true
		   else
			NON_SHLIBS="$NON_SHLIBS $lib"
		   fi
		done
		NEEDLIBS="$NON_SHLIBS"
		if $need_interp ; then NEEDLIBS="$NEEDLIBS SHINTERPLIB" ;fi
		if $need_frame ; then NEEDLIBS="$NEEDLIBS SHFRAMELIB" ;fi
		if $need_libq ; then NEEDLIBS="$NEEDLIBS SHQLIB" ;fi
		if $need_compat ; then NEEDLIBS="$NEEDLIBS SHCOMPATLIB" ;fi

		# this is required to make sure that ingres malloc library is
		# linked in first.
		UNDEFS="$UNDEFS malloc"
		;;
	    esac
	}

              case "$DEST" in
                utility) BLDRULE=IIUTILEXE
                         ;;
                    bin) BLDRULE=IIBINEXE
                         ;;
                    lib) BLDRULE=IILIBRARY
                         ;;
                  tools) BLDRULE=IITOOLSEXE
                         ;;
                  files) BLDRULE=IIFILES
                         ;;
                 icebin) BLDRULE=IIICEBINEXE
                         ;;
                schabin) BLDRULE=IISCHABINEXE
                       	 ;;
               vcshabin) BLDRULE=IIVSCHABINEXE
                         ;;
                      *) [ -z "$BLDRULE" ] && BLDRULE=IIBINEXE
                         ;;
              esac

                echo "$BLDRULE $PROGRAM : $i ;"
                [ "$NEEDLIBS" ] && echo "IINEEDLIBS $PROGRAM : $NEEDLIBS ;" | \
			sed s:'$(LIBRARY)':$LIB: | eval $fmt 
		[ "$NEEDLIBSW" ] && echo "IINEEDLIBSW $PROGRAM : $NEEDLIBSW ; " | \
			sed s:'$(LIBRARY)':$LIB: | eval $fmt
		[ "$NEEDOBJ" ] && echo "IINEEDOBJ $PROGRAM : $NEEDOBJ ; " | \
			sed s:'$(LIBRARY)':$LIB: | eval $fmt
                [ "$UNDEFS" ] && echo "IIUNDEFS $PROGRAM : $UNDEFS ;" | \
			sed s:'$(LIBRARY)':$LIB: | eval $fmt 
              echo "" 
            }  # end of for-i-in-MAINS loop

            # Get ming hints for shell scripts.
            for shell in $SHELLS
            {
              PROGRAM=`expr $shell : '\([^.]*\)'`
	      case "$GROUP" in
		tools|\
		testtool) DEST=tools ;;
		cl)
		    case $SUBSYS in
			ci| \
			cs| \
			cv) DEST=utility ;;
		    esac
			;;
		admin)
		     case $SUBSYS in 
			shell_unix) DEST=utility ;;
		     esac
			;;
		*) DEST= ;;
	      esac
              MODE=
 
              eval `sed -n "$shhints" $shell 2> $DEVNULL`
 
	      if [ "$conf_EDBC" = "true" ]
	      then
		PROGRAM=`echo "$PROGRAM" | sed 's/(PROG0PRFX)/edbc/' |
		sed 's/(PROG1PRFX)/edbc/' | sed 's/(PROG2PRFX)/edbc/' |
		sed 's/(PROG3PRFX)/ED/' | sed 's/(PROG4PRFX)/ed/'`
	      else
		PROGRAM=`echo "$PROGRAM" | sed 's/(PROG0PRFX)//' |
		sed 's/(PROG1PRFX)/ii/' | sed 's/(PROG2PRFX)/ing/' |
		sed 's/(PROG3PRFX)/II/' | sed 's/(PROG4PRFX)/ii/'`
	      fi

              case "$DEST" in
                utility) BLDRULE=IIUTILSH
                         ;;
                    bin) BLDRULE=IIBINSH
                         ;;
                  tools) BLDRULE=IITOOLSSH
                         ;;
                  files) BLDRULE=IIFILESSH
                         ;;
                schabin) BLDRULE=IISCHABINSH
                         ;;
               vcshabin) BLDRULE=IIVCSHABINSH
                         ;;
                      *) [ -z "$BLDRULE" ] && BLDRULE=IIBINSH
                         ;;
              esac
 
	      if [ "$WINDOWS" != "TRUE" ] ; then
		  echo "$BLDRULE $PROGRAM : $shell ;"
              fi
            }
            [ "$SHELLS" ] && echo ""
 
            [ "$MAN1S" ] && 
            {
              echo "IITMANFILES $MAN1S ;"
              echo ""
            }

	    [ "$CCPP" ] && {
	    case "$FACILITY:$SUBSYS" in
		st:specials_unix) 
		    for ccpp in $CCPP
		    {
			# Catch special cases
			case $ccpp in
                          #lib.ccpp) echo "IILIBPRT lib.prt : lib.ccpp ; "
                #                     echo ""
                #                     ;;
			release.ccpp) echo "IIRELDAT release.dat : release.ccpp ; "
				      echo ""
		    		      echo "IIINSTALLDAT install.dat : release.dat ;"
				      echo ""
				      ;;
		      rpmconfig.ccpp) echo "IICCPPFILE \$(II_MANIFEST_DIR)/rpmconfig : rpmconfig.ccpp ;"
				      echo ""
				      ;;
				   *) PRT="$ccpp $PRT"
				      ;;
			esac
		     }
		     echo "IIPRT $PRT ;" | eval $fmt
		    ;;
		odbc:hdr)
	    	    for ccpp in $CCPP
	    	    {
			FILE=`echo $ccpp | cut -d. -f1` # loose the suffix
			if [ "$ccpp" = "ocfginfo.ccpp" ] ; then
		   	    BLDRULE=IICCPPFILE ; 
			    TSFX='dat' 
			    echo "$BLDRULE \$(INGMANIFEST)/$FILE.$TSFX : $ccpp ;"
			else
		   	    BLDRULE=IIHDRCCPP ; 
			    TSFX='h'
			    echo "$BLDRULE $FILE.$TSFX : $ccpp ;"
			    echo "IIFILE $FILE.$TSFX : $FILE.$TSFX ;"
			fi

			echo ""
	    	    }
		    ;;
		clf:ck_unix)
		    for ccpp in $CCPP
	    	    {
		   	BLDRULE=IICCPPFILE ; DESTDIR='$(INGFILES)' ; TSFX='def' 
			[ "$ccpp" = "cktmpl_wrap.ccpp" ] && TSFX=tpl
			FILE=`echo $ccpp | cut -d. -f1` # loose the suffix
			echo "$BLDRULE $DESTDIR/$FILE.$TSFX : $ccpp ;"
	    	    }
		    ;;

		*)
	    	    for ccpp in $CCPP
	    	    {
		   	BLDRULE=IICCPPFILE ; DESTDIR='$(INGFILES)' ; TSFX='def' 
			FILE=`echo $ccpp | cut -d. -f1` # loose the suffix
			echo "$BLDRULE $DESTDIR/$FILE.$TSFX : $ccpp ;"
	    	    }
		    ;;
	    esac
	    echo "" 
	    }
		
	    for def in $DEF
	    {
		case "$FACILITY:$SUBSYS" in
		    clf:*) echo "IIFILE $def : $def ;"
			;;
		       *)
			;;
		esac
	    }
	    [ "$DEF" ] && echo ""

	    for tmplt in $TMPLT
	    {
		DESTDIR='$(INGFILES)/rpmtemplates'
		echo "FILE $DESTDIR/ca-$tmplt : $tmplt ;"
	    }

	    for tz in $TZ
	    {
		echo "IITIMEZONE $tz ;"
		echo ""
	    }

	    for rc in $RC
	    {
		if [ "$WINDOWS" != "TRUE" ] ; then
		DESTDIR='$(INGFILES)/rcfiles'
		echo "FILE $DESTDIR/$rc : $rc ;"
                fi
	    }

            for vtmtmplt in $VTMTMPLT
	    {
		DESTDIR='$(INGFILES)/utmtemplates'
		echo "FILE $DESTDIR/$vtmtmplt : $vtmtmplt ;"
	    }

	    for vtmcfg in $VTMCFG
	    {
		DESTDIR='$(INGFILES)'
		echo "FILE $DESTDIR/$vtmcfg : $vtmcfg ;"
	    }

	    for vtmjar in $VTMJAR
	    {
		DESTDIR='$(INGLIB)'
		echo "FILE $DESTDIR/$vtmjar : $vtmjar ;"
	    }

	    for vtmzip in $VTMZIP
	    {
		DESTDIR='$(INGFILES)'
		echo "FILE $DESTDIR/$vtmzip : $vtmzip ;"
	    }
	     		
	    for vtmhelp in $VTMHELP
	    {
		DESTDIR='$(INGFILES)/english/vtmhelp'
		echo "FILE $DESTDIR/$vtmhelp : $vtmhelp ;"
	    }

            case $SUBSYS in
              files) FILES=`$LS | egrep -v "^MING|OUT|^Jam*|.*\.sav"`
                       echo "IIFILES $FILES ;" | eval $fmt
                     ;;
            esac

            [ "$SPECFILES" ] && echo SPECFILES = $SPECFILES
            for f in $SPECFILES
            {
              echo "FILE $FILDIR/$f : $f ;"
            }

            # Header targets
            [ "$MSGS" ] && \
            {
              echo "IIMSGHDR $MSGS ;"
              echo ""
            }
            for f in $QSHS
            {
              echo "IIQSHHDR `basename $f .qsh`.h : $f ;"
            }
            [ "$QSHS" ] && echo ""
            for f in $QHS
            {
              echo "IIQHHDR `basename $f .qh`.h : $f ;"
            }
            [ "$QHS" ] && echo ""
            for f in $SHS
            {
              echo "IISHHDR `basename $f .sh`.h : $f ;"
            }
            [ "$SHS" ] && echo ""

	    [ "$HDRS" ] && \
	    {
		case $FACILITY:$SUBSYS in
		    embed:libqtxxa_unix) echo "IIFILES $HDRS ;" | eval $fmt
				    echo ""
					;;
		esac
	    }
			
            # Forms targets
            for f in $FRMS
            {
              echo "IIFORM $f ;"
            }
            [ "$FRMS" ] && echo "" 

            # Message targets
	    [ "$HLPS" ] && \
	    {
		case "$FACILITY:$SUBSYS" in
		 st:specials_unix ) 
			echo "IIINSTALLHLP $HLPS ;" | eval $fmt
				;;
		 odbc:config_win )
			echo "IIHLPODBC \$(INGBIN)/IILIBODBCCFG.HLP : $HLPS ;"
				;;
			      *)
            			for f in $HLPS
            			{
            			  echo "FILE \$(INGMSG)/$f : $f ;"
            			}
				;;
		esac
	    }
		
            [ "$HLPS" ] && echo "" 

	    
            for f in $TFS
            {
              echo "FILE \$(INGMSG)/$f : $f ;"
            }
            [ "$TFS" ] && echo "" 

            # Parsers
            [ $PARS ] && 
            {
	      case "$FACILITY:$SUBSYS" in
		tools:yacc|\
		tools:pgbig )
              	    echo "File \$(IYACCPAR) : $PARS ;"
  	      	    echo ""
			;;
	      	psf:yacc ) 
		    echo "File \$(BYACCPAR) : $PARS ;"
  	            echo ""
			;;
	      esac
            }
            [ "$PARS" ] && echo "" 

	    [ "$LCL_LIBS" ] &&
	    {
	      echo "IILIBRARY $LCLLIB : $LCL_LIBS ;" | eval $fmt
	      echo ""
	    }

            [ "$LIBS" ] &&
            {
	      if [ "$WINDOWS" = "TRUE" ] ; then
              case "$FACILITY:$SUBSYS" in
               clf:handy )
                        for fl in $LIBS
                        {
                                if [ ! $fl = "bsshmio.c" ] &&
                                   [ ! $fl = "bssockdnet.c" ] &&
                                   [ ! $fl = "bssockio.c" ] &&
                                   [ ! $fl = "bssocktcp.c" ] &&
                                   [ ! $fl = "bssockunix.c" ] &&
                                   [ ! $fl = "bstcpaddr.c" ] &&
                                   [ ! $fl = "bstcpport.c" ] &&
                                   [ ! $fl = "bstliio.c" ] &&
                                   [ ! $fl = "bstliosi.c" ] &&
                                   [ ! $fl = "bstlispx.c" ] &&
                                   [ ! $fl = "bstlitcp.c" ] &&
                                   [ ! $fl = "bstliunix.c" ] &&
                                   [ ! $fl = "bsunixaddr.c" ] &&
                                   [ ! $fl = "cldio.c" ] &&
                                   [ ! $fl = "clpoll.c" ] &&
                                   [ ! $fl = "diremul.c" ] &&
                                   [ ! $fl = "dup2.c" ] &&
                                   [ ! $fl = "ftok.c" ] &&
                                   [ ! $fl = "histo.c" ] &&
                                   [ ! $fl = "iisubst.c" ] &&
                                   [ ! $fl = "itimer.c" ] ; then
                                        LIBS2="$LIBS2 $fl"
                                fi
                        }
                      echo "IILIBRARY $LIB : $LIBS2 ;" | eval $fmt
                      echo ""
#                     echo "IILIBRARY COMPATLIB : $LIB ;" | eval $fmt
#                     echo ""
                        ;;
               clf:* )
                      echo "IILIBRARY $LIB : $LIBS ;" | eval $fmt
                      echo ""
#                     echo "IILIBRARY COMPATLIB : $LIB ;" | eval $fmt
#                     echo ""
                        ;;
               *:* )
                      echo "IILIBRARY $LIB : $LIBS ;" | eval $fmt
                      echo ""
                        ;;
              esac
              else
                  echo "IILIBRARY $LIB : $LIBS ;" | eval $fmt
                  echo ""
              fi
            }

            [ "$IMPDATALIBS" ] &&
            {
                  echo "IILIBRARY $IMPDATALIB : $IMPDATALIBS ;" | eval $fmt
            }

	    [ "$LIBOBJECT" ] &&
	    {
		  echo ""
		  echo "IILIBOBJECT $LIBOBJECTS ; " | eval $fmt
	    }

	    # XML files
	    [ "$XML" ] && 
	    {
	    case "$FACILITY:$SUBSYS" in
		adf:adn)
		    echo "IIUCHARFILES $XML ;" | eval $fmt 
		    echo ""
		    for xml in $XML
		    {
			if [ "$xml" = "charmapalias.xml" ] ; then
			    echo "IIUCHARALIAS \$(INGFILES)/$FDIR/aliasmaptbl : $xml ;"
			else
			    t=`echo $xml | cut -d. -f1` # no suffix on target
			    echo "IIUCHARMAP \$(INGFILES)/$FDIR/$t : ${xml} ;" \
							| eval $fmt
			fi
		    }
		    echo ""
            	    ;;
	    esac
	    }

	    # DTD files
	    [ "$DTD" ] &&
	    {
	    case "$FACILITY:$SUBSYS" in
		adf:adn)
		    echo "IIUCHARFILES $DTD ;" | eval $fmt 
		    echo ""
			;;
	    esac
	    }

  esac              # end of case-$ACT-in

  ) >> $JAMFILE    # end of write subshell

 )                  # end of for-loop subshell
}                   # end of for-d-in loop

if [ "$platform" = "win" ] && [ -n "$USE_CYGWIN" ] ; then
     if [ ! -z $jamdflt ] ; then	
	u2d $jamdflt
     else
	u2d $jamfile
     fi
fi

exit 0
