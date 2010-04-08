:
# Copyright (c) 2004, 2010 Ingres Corporation
# $Header $
#
# Name:
#	iilink - complete link of partially linked executables 
#		 iidbms.o, iircp.o, iijsp.o
#
# Usage:
#	iilink [-dbms] [-standard] [-udt 'string'] [-noudt] [-nosol]
#		[-lp32] [-lp64]
#
# Description:
#	This script loads iidbms, dmfrcp and dmfjsp. If the -dbms
#	flag is passed the only executable built is iidbms.
#
#	If the -standard flag is passed there will be no extension
#	attached to the name of the binaries. 
#
#	If the -merge flag is passed, iilink will attempt to create
#	iimerge regardless of what object files are available.
#
#	If the -noudt flag is passed, iilink will not ask any questions
#	about which udt or class object files to use, it will just use
#   	the default.
#
#	If the -udt 'string' flag is passed, the string is used as the
#	answer to the udt object files question.
#
#	If the -nosol flag is passed, iilink will not include the
#	Spatial Object files.
#
#	-lp32 and -lp64 are used with hybrid builds to specify the
#	type of link to be done, without asking.
#
#	If no flags are passed, iilink loads all the executables with
#	their standard names, iidbms, dmfrcp and dmfjsp.
#
# Exit status:
#	0 	OK
#	1	unknown option
#	2	cannot load iidbms
#	3	cannot load dmfrcp
#	4	cannot load dmfjsp
#	5	cannot load iimerge
#	6	cannot find appropriate object files
#	7	error creating symbolic link
#
##  History:
##      28-jun-90 (jonb)
##              Include iisysdep header file to define system-dependent
##              commands and constants.  Use cc instead of ld, and
##		eliminate -lc flags.
##	23-jul-1990 (jonb)
##		Create iimerge, with appropriate links, rather than
##		iidbms and friends if the appropriate object file is
##		available.
##	23-aug-1990 (jonb) 
##		Use iimerge.o instead of dmfmerge.o to create iimerge
##		executable, and don't use any libraries.
##	27-aug-1990 (jonb)
##		Add -noudt flag to prevent asking for UDT object files.
##		When this flag is used, we just assume that we want to
##		link with default_udt.
##	12-sep-1990 (jonb)
##		Add cacheutil to the list of programs ln'd to iimerge.
##	12-sep-1990 (jonb)
##		Use LINKCMD from iisysdep to link back-end executables.
##	18-sep-1990 (jonb)
##		Expand variable references in the input path name for
##		the object files.  This would enable the user to easily
##		mention files relative to $II_SYSTEM, for instance.
##      28-feb-1991 (jas/jonb)
##              Added libsd.a -- the field-upgradeable library which is
##		built from unix/cl/clf/sd for the Smart Disk project -- to 
##		the link commands for the back-end executable(s).
##      15-sep-1991 (aruna)
##             Cleared up the output from echolog that does not display
##             uniform boxes.
##	09-nov-1992 (fpang)
##	       Added IISTAR to list of progs linked to iimerge.
##	19-nov-1992 (lauraw)
##		The section that creates the filesystem links to iimerge
##		no longer codes the full pathname into the link because
##		the link is going to be encoded onto the distribution
##		medium and restored on the target system.
##	22-jan-1993 (bryanp)
##		Added rcpstat to list of progs linked to iimerge
##	19-mar-1993 (dianeh)
##		Removed references and call to chkins (gone for 6.5);
##		changed calls to echolog to iiecholg; added a little
##		echo of what's being loaded so it looks like something
##		is at least happening.
##	19-mar-1993 (dianeh)
##		Add iishowres to list of program links; alphabetized list.
##      15-sep-1993 (stevet)
##  	    	Added option to load Spatial object along with UDT.
##	27-oct-1993 (dianeh)
##		Clean up boxed text and prompts.
##	26-nov-1993 (tyler)
##		Remove calls to defunct scripts iiecholg and iiechonn.
##	29-nov-1993 (tyler)
##		iiechonn reinstated.
##	26-apr-94 (vijay)
##		Change modes on executables to 755. This was initially done
##		on VMS, and the release tools expect similar permissions
##		on unix.
##	11-may-94 (arc)
##		Set label of any server executables to INGRES_HIGH and
##		add required privileges for INGRES/Enhanced Security.
##		Use of SETLABEL should be outside test for SET_B1_PRIV
##		as HP-BLS sets labels (but not privileges). 
##		SETLABEL is a no-op on base product.
##	21-jul-95 (allst01)
##		pickup up cc from $CC and don't hardcode it as cc !
##
##	19-dec-97 (stephenb)
##		Add repstat to list of links to iimerge.
##      10-Oct-00 (hweho01)
##              Added CCLDSERVER to hold the special linker options
##              for dbms server only.
##	16-Jan-2001 (hanje04)
## 	    As part of fix for SIR 103876, added check for iisysdep 
##	    variable USE_SHARED_SVR_LIBS. If present server is built
##	    using shared libraries and an iimerge.o containing just
##	    dmfmerge.o. This can be over ridden by -standard flag
##	    or forced by -shared flag.
##	08-Aug-2001 (hanje04)
##	    Corrected link command for shared libraries as libspat.a was
##	    not being picked up when linking with Spatial Object Libraries.
##	11-mar-2002 (somsa01)
##	    If add_on64_enable is ON, then we're doing a 64-bit linkage.
##	06-Jun-2002 (hanch04)
##	    Added -lp32 and -lp32 to specify building a 32 or 64 bit
##	    merge if add_on64_enable is ON.
##	06-Jun-2002 (hanch04)
##	    Should not mention 64 bit libraries in comment on a non 64-bit
##	    platform.
##	15-Aug-2002 (hanch04)
##	    The lp64 exe's should link to iimerge in the local dir.
##	03-Sep-2002 (somsa01)
##	    Minor correction to printout. Also, key setting of ADD_ON64
##	    off of II_LP64_ENABLED.
##	18-Sep-2002 (somsa01)
##	    Minor changes to printout.
##      22-jan-2002 (huazh01)
##              define LIBRTE as /lib/librte.so
##              change 454829 instroduce clock_gettime(), which requires
##              linking to /lib/librte.so during the compile routine.
##      04-dec-2002 (huazh01)
##              remove the above change. linking to librte.so is 
##              required for dgi_us5. the required change is included in 
##              mkdefault.sh for dgi_us5.
##      22-Jan-2003 (hweho01)
##          The linker option "-bmaxdata:0x30000000" in CCLDSERVER  
##          is needed for the 32-bit servers to increase the addressed 
##          space only.  It should be removed for the 64-bit servers,   
##          because the default is unlimited.
##	23-Jun-2004 (hanje04)
##	    SIR 112535
##	    Remove references to Spatial Objects for Open Source.
##	30-Jul-2004 (hanje04)
##	    Remove references to libsd.a. (Smart Disk)
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##	27-Sep-2004 (hanje04)
##	    Demo UDT's are now supplied as a shared library instead of
##	    objects.
##	20-Oct-2004 (hanje04)
##	    Server should only link to server shared libraries.
##	08-Dec-2004 (hanje04)
##	    Re-align text border
##      04-Mar-2005 (hanje04)
##	    SIR 114034
##          Add support for reverse hybrid builds. If conf_ADD_ONHB is
##          defined then script will behave as it did prior to change.
##          However, if conf_ADD_ON32 is set -lp${RB} flag will work like
##          the -lp${HB} flag does for the conf_ADD_ONHB case except that
##          a 32bit iimerge will be built under $ING_BUILD/bin/lp${RB}.
##      05-Aug-2005 (hweho01)
##	    Display Demo UDT library as archive file for Tru64 (axp_osf)
##          platform.
##	14-Sep-2005 (hanje04)
##	    BUG 115264
##	    Add usage message, remove B1 references.
##	20-Sep-2005 (hanje04)
##	    SIR 115239
##	    Re-add options for loading Spatial Objects into the server. Only
##	    display it if it has been installed.
##	26-Oct-2005 (hanje04)
##	    SIR 115239
##	    Spatial Objects library needs to be shared library for
##	    cross release support on Linux.
##	27-Oct-2005 (hanje04)
##	    SIR 115239
##	    Reverse hybrids never build a 32bit server so don't offer.
##      25-may-2006 (hanal04) Bug 116138
##          Above change check for $shared but does not check if the
##          value is true or false.
##	01-May-2006 (hanje04)
##	    BUG 116056
##	    Stop bogus warning concerning absense of '-lspat'.
##	    Spatial library is now shared object so we don't need to check for
##	    libspat.a.
##	27-Jul-2006 (kschendel)
##	    (Datallegro) Add -udt option, doc others.
##      30-Apr-2007 (hweho01)
##          Some Unix platforms need to reference spatial library in full
##          path name when it is linked with iimerge, such as Solaris and
##          AIX.  ALso fix the hybrid directory name.
##      04-May-2007 (hweho01)
##          Enable iilink to handle the UDT objects and Spatial objects 
##          that are contained in static archives. 
##      14-May-2007 (hanal04) Bug 118320
##          Add -nosol flag to explicitly prevent spatial object library
##          from being linked or prompted. If spatials are configured
##          they are now automatically linked. User is prompted if
##          spatials are not configured and -nosol is not specified.
##      16-May-2007 (hanal04) Bug 118320
##          Adjust previous change for this bug to restore default of
##          not prompting for SOL if not configured.
##	24-Sep-2007 (bonro01)
##	    Include -nosol in iilink help output.
##       8-Oct-2007 (hanal04) Bug 119262
##          Added archive iimerge.a alternative to shared library server.
##	2-Nov-2007 (bonro01)
##	    Correct usage of -standard flag. It incorrectly turned
##	    off shared library linking on Linux when it is documented to
##	    only suppress the prompting of an executable suffix and
##	    use the standard executable names.
##      11-Dec-2007 (hanal04) Bug 119597
##          Add ingres library path to non "shared" link commands.
##	11-Feb-2008 (bonro01)
##	    Include Spatial object package into standard ingbuild and RPM
##	    install images instead of having a separate file.
##	25-Mar-2008 (bonro01)
##	    All platforms that use Spatial shared libraries should
##	    specify the full path on the link because SETUID programs
##	    can't use LD_LIBRARY_PATH.
##      18-Mar-2009 (hweho01)
##          Allow users to modify the linker value for 32-bit DBMS server
##          on AIX.
##      05-Jun-2009 (hweho01) SIR 121775 
##          Include linker option CCLDSERVER for building iimerge from  
##          archive iimerge.a. 
##	19-Jun-2009 (kschendel) SIR 122138
##	    Hybrid scheme changes, reflect here.
##	    Kill -L/usr/lib/fswitch, was probably some ancient sun2 thing,
##	    belonged in LDLIBMACH anyway.
##	    Don't key 64-bit add-on to II_LP64_ENABLED, since it's not set
##	    yet during initial build.  There's no harm in building a non-
##	    enabled server, anyway.  Key it from the build config only.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.
##            
#  DEST = utility
##-----------------------------------------------------------------

(LSBENV)
. iisysdep

[ $CMD ] || 
{
	CMD=iilink
	export CMD
}

# check for command arguments passed to this program

dbms=false
standard=false
merge=false
default=true
rcpjsp=false
askudt=true
udtstr=
adt_str=
# Will only askcls id spatials are also configured
askcls=true
lphb=false
lprb=false
asklphb=true

# If we are building a server with shared libraries set shared=true

if $USE_SHARED_SVR_LIBS
then
shared=true
else
shared=false
fi

if $USE_ARCHIVE_SVR_LIB
then
archive=true
else
archive=false
fi

#
# Set hybrid variables based on build_arch variable
#
RB=xx
HB=xx
if [ "$build_arch" = '32' -o "$build_arch" = '32+64' ] ; then
    RB=32
    HB=64
elif [ "$build_arch" = '64' -o "$build_arch" = '64+32' ] ; then
    RB=64
    HB=32
fi

## Set SERVER_HB if an add-on server might be possible.
## FIXME we assume that 32+64 hybrids have lp64 servers, while 64+32
## hybrids don't have lp32 servers.  This really ought to be controllable.
SERVER_HB=FALSE
if [ "$HB" = 64 -a "$build_arch" = '32+64' ] ; then
    SERVER_HB=TRUE
fi


# usage info
usage()
{
    cat << EOF
usage:
	iilink [-dbms] [-standard] [-noudt] [-nosol] [-shared]
		[-merge] [-lp${RB}]|[-lp${HB}]
	
	    -dbms	Builds only iidbms
	    -standard	Use standard binary names (default)
	    -noudt	Don't link in UDTs (or prompt)
	    -nosol	Don't link in Spatial Object Library (or prompt)
	    -shared	Build server from shared libraries (Linux default)
	    -merge	Unconditionally build iimerge binary
	    -lp${RB}	Build ${RB}bit server (default)
	    -lp${HB}	Build ${HB}bit server 

EOF
}
		
while [ $# -gt 0 ]
do
	a=$1
	case $a in
	  -lp${RB})	# -build the regular version if available
		lprb=true
		asklphb=false
		shift
		;;
	  -lp${HB})	# -build the hybrid version if available
		if [ "$SERVER_HB" != TRUE ] ; then
		    echo "This Ingres build does not support a $HB bit server."
		    exit 1
		fi
		lphb=true
		asklphb=false
		shift
		;;
	    -d*)	# -dbms, build dbms only
		dbms=true
		default=false
		shift
		;;

	-shared)	# -shared, builds dbms from shared libs.
		shared=true
		default=false
		standard=true
		shift
		;;

	    -st*)	# -standard, use standard binary names
		standard=true
		shift
		;;

	    -m*)   	# -merge, build iimerge unconditionally
		merge=true
		default=false
		shift
		;;

	    -nosol)   	# -nosol, don't ask for Spatial Objects Library.
		askcls=false
		shift
		;;

	    -noudt)   	# -noudt, don't ask for udt and class objects
		udtstr=
		askudt=false
		shift
		;;

	    -u*)	# -udt 'string' gives udt question answer
		askudt=false
		shift
		udtstr="$1"
		shift
		;;

	    *)		# unknown option error
		usage
		exit 1
		;;
	esac
done
if [ "$lprb" = 'true' -a "$lphb" = 'true' ] ; then
    echo "Please choose either -lp${RB} or -lp${HB}, not both"
    exit 1
fi

config_host=`iipmhost`

if $asklphb && [ "$SERVER_HB" = "TRUE" ] ;
then
        cat << !

 -----------------------------------------------------------------------
|                                                                       |
| A ${RB} bit or ${HB} bit INGRES server program can be loaded.               |
| If you want to load the ${HB} bit INGRES server answer yes; otherwise,   |
| press RETURN to continue and accept a ${RB} bit server.                  |
|                                                                       |
 -----------------------------------------------------------------------

Enter "yes" to load the ${HB} bit INGRES server program,
!
        iiechonn "or press RETURN to continue: "
        read lphb_yn

        if [ -z "$lphb_yn" ] || expr "$lphb_yn" : '[Nn][Oo]*' >/dev/null 2>&1
        then
	    :
	else
	    lphb=true
        fi
fi

if $lphb 
then
    add_onhb_dir=/lp${HB}
    if [ ${HB} = 64 ] ; then
	CCLDMACH=$CCLDMACH64
	LDLIBMACH=$LDLIBMACH64
	LDSRVORIGIN=$LDSRVORIGIN64
    else
	CCLDMACH=$CCLDMACH32
	LDLIBMACH=$LDLIBMACH32
	LDSRVORIGIN=$LDSRVORIGIN32
    fi
    BITS=$HB
else
    add_onhb_dir=
    BITS=$RB
    eval LDSRVORIGIN="\$LDSRVORIGIN${BITS}"
fi
if [ -n "$build_arch" -a $BITS = 'xx' ] ; then
    BITS=$build_arch
fi

## FIXME really should have CCLDSERVER32/64 here instead of hard coding
if [ "$BITS" != 32 ] ; then
    CCLDSERVER=
fi
if [ "$CCLDSERVER" != "" -a -s $II_SYSTEM/ingres/files/config.dat ]
then
   segments=`iigetres "ii.$config_host.dbms.*.image_data_segments"`
   if [ $segments -ge 3 -a $segments -le 8 ]
   then 
        CCLDSERVER='-bmaxdata:0x'"$segments"'0000000'
   fi
fi

default_udt=$II_SYSTEM/ingres/lib${add_onhb_dir}/iiuseradt.o
cls_str=$II_SYSTEM/ingres/lib${add_onhb_dir}/iiclsadt.o

# If we haven't been told explicitly what we should build, we'll figure
# it out based on what object files exist.

if $default
then
  if [ -f $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o ]
  then
    merge=true
  elif [ -f $II_SYSTEM/ingres/lib${add_onhb_dir}/iidbms.o ]
  then
    dbms=true
    [ -f $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o ] && \
    [ -f $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o ] && rcpjsp=true
  else
    cat << ! 

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| None of the object files needed to load the server can be found in    |
| the INGRES library directory.  Please retry the $CMD operation	|
| when one of the following files is present:                           |
|									|
|        \$II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o				|
|        \$II_SYSTEM/ingres/lib${add_onhb_dir}/iidbms.o					|
|									|
 -----------------------------------------------------------------------
!
    exit 6
  fi
fi

# -----------------------------------------------------------------------

if $merge 
then
	echo ""
	echo "Loading INGRES merged server program ..."
elif $dbms
then
    if $rcpjsp
    then
	echo ""
	echo "Loading INGRES binaries ..."
    else
	echo ""
	echo "Loading DBMS server ..."
    fi
fi

if [ "$shared" = "true" ] ;
then
	echo "Using Shared Libraries ..."
    if [ "$archive" = "true" ] ; then
        echo "Using archive library ..."
    fi
fi


if $UDT_LIB_IS_STATIC_ARCHIVE ; then
DemoUdt_library="\$II_SYSTEM/ingres/demo/udadts/libdemoudt.a"
else
DemoUdt_library="\$II_SYSTEM/ingres/demo/udadts/libdemoudt.1.$SLSFX"
fi

if $SPAT_LIB_IS_STATIC_ARCHIVE ; then
Spatial_Object_Library=$II_SYSTEM/ingres/lib${add_onhb_dir}/libspat.a
Spat_library=\$II_SYSTEM/ingres/lib${add_onhb_dir}/libspat.a
else
Spatial_Object_Library=$II_SYSTEM/ingres/lib${add_onhb_dir}/libspat.1.$SLSFX
Spat_library=\$II_SYSTEM/ingres/lib${add_onhb_dir}/libspat.1.$SLSFX
fi

# Prompt for inclusion of Spatial Objects if it's configured
if $lphb 
then
    spat=`iigetres ii.$config_host.config.spatial${HB}`
else
    spat=`iigetres ii.$config_host.config.spatial`
fi
[ "$spat" != "ON" ] && askcls=false

# spatial objects
if $UADT_CAP && $askcls
then
	cat << !

 -----------------------------------------------------------------------
|                                                                      |
| INGRES Spatial Objects consist of six spatial datatypes: POINT, BOX  |
| LINE, LINE SEGMENT, CIRCLE and POLYGON, as well as a number of       |
| spatial operators that operate on these spatial datatypes.  INGRES   |
| Spatial Objects have been installed and configured and will          |
| automatically be included in the INGRES installation.                |
| To prevent the inclusion of the INGRES Spatial Object                |
| library re-run iilink with the -nosol option.                        |
|                                                                      |
 -----------------------------------------------------------------------

!

      cls_yn="y"

      [ "$cls_yn" ] &&
      {
	if [ -f "$Spatial_Object_Library" ]
	then
          cls_str=$Spatial_Object_Library
	else 
            cat << !

 -----------------------------------------------------------------------
|                         ***WARNING***                                 |
|                                                                       |
| The object file for Spatial Objects:                                  |
|                                                                       |
|       $Spat_library
|                                                                       |
| does not exist on your system.  Loading will continue, but there      |
| will probably be errors.                                              |
|                                                                       |
 -----------------------------------------------------------------------

!
	fi
      }

fi

# UDT's
if $UADT_CAP && $askudt
then
	cat << ! 

 -----------------------------------------------------------------------
|									|
| These INGRES binaries are loaded to allow you to add User Defined 	|
| Data Types (UDTs) to this INGRES installation.			|
|									|
| You should now enter the modules where your User Defined Data Types	|
| are defined. You can either enter the name of an object file(s)       |
| or the name of a library.						|
| Examples are:								|
|	/project1/obj/*.o						|
|	/project1/obj/filename.o					|
|	/project1/lib/myuadt.a						|
|	$DemoUdt_library			|
!
if $lphb && [ "$SERVER_HB" = "TRUE" ] ;
then
	cat << ! 
|	\$II_SYSTEM/ingres/demo/udadts/lp${HB}/libdemoudt.1.$SLSFX		|
|	(for ${HB}-bit servers)						|
!
fi
	cat << ! 
|									|
| If you don't have any User Defined Data Types created, press RETURN,	|
| and the default object file will be used to load the INGRES binaries.	|
|									|
 -----------------------------------------------------------------------

Enter the full pathname of the object file or library to be loaded,	
!
	iiechonn "or press RETURN for the default object file: "
	read adt_str

	if [ -z "$adt_str" -a ! -f "$default_udt" ]
	then
		cat << !

 -----------------------------------------------------------------------
|			    ***WARNING***				|
|                                                                       |
| The default UDT object file:         		                        |
|                                                                       |
|       $default_udt
|                                                                       |
| does not exist on your system.  Loading will continue, but there      |
| will probably be errors.                                              |
|                                                                       |
 -----------------------------------------------------------------------

!
	fi

	if $standard
	then :
	else
		name=iidbms
		$merge && name=iimerge
		cat << !

 -----------------------------------------------------------------------
|									|
| An extension may be supplied at this time to differentiate your test	|
| binaries from the existing ones. 					|
|									|
| For example, if you enter the extension "test", the DBMS binary is 	|
| created at:								|
|									|
|	\$II_SYSTEM/ingres/bin${add_onhb_dir}/$name.test				|
|									|
| Otherwise, the DBMS binary is created at:				|
|									|
|	\$II_SYSTEM/ingres/bin${add_onhb_dir}/$name 					|
|									|
| where it overrides the existing DBMS binary.				|
|									|
 -----------------------------------------------------------------------

!
		iiechonn "Enter the file extension for the test binaries: "
		read ext_ans
	fi
else
	if [ -n "$udtstr" ] ; then
		adt_str="$udtstr"
	fi
fi

#  If, for one reason or another, we don't have a udt object string
#  yet, we'll use the default.
[ -z "$adt_str" ] && adt_str=$default_udt

#  Expand any variable references...
adt_str=`eval "echo $adt_str"`

if [ -z "$ext_ans" ]
then
	dbms_name=iidbms
	rcp_name=dmfrcp
	jsp_name=dmfjsp
	merge_name=iimerge
	star_name=iistar
else
	dbms_name=iidbms.$ext_ans
	rcp_name=dmfrcp.$ext_ans
	jsp_name=dmfjsp.$ext_ans
	merge_name=iimerge.$ext_ans
	star_name=iistar.$ext_ans
fi

#----------------------------------------------------------------------
#  Load iimerge if that's what we're supposed to do...

if $merge
then

cat << !

Loading iimerge ...
!

# If using shared libraries then use appropriate link command.

if [ "$shared" = "true" ] ;
then

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
	$LDLIBMACH

else
    if [ "$archive" = "true" ] ; then

        $CC $CCLDMACH $CCLDSERVER \
		-o $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.a \
                -L$II_SYSTEM/ingres/lib${add_onhb_dir} \
                $cls_str $adt_str \
                $LDLIBMACH

    else

        $CC $CCLDMACH $CCLDSERVER -o \
	        $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name \
	        $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
                -L$II_SYSTEM/ingres/lib${add_onhb_dir} \
	        $cls_str $adt_str \
	        $LDLIBMACH

    fi
fi

if [ $? != 0 ]
then
	cat << !

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| Cannot load the merged INGRES server program, iimerge.  Please	|
| correct the problem before running $CMD again.			|
|									|
 -----------------------------------------------------------------------

The command used to load this binary was:

!
    if [ "$shared" = "true" ] ;
    then

   cat << !

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
	$LDLIBMACH 
!
    else

        if [ "$archive" = "true" ] ; then

            cat << !

            $CC $CCLDMACH $CCLDSERVER \
		    -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name \
                    $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
                    $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.a \
                    $cls_str $adt_str \
                    $LDLIBMACH
!
        else

            cat << !

            $CC $CCLDMACH $CCLDSERVER \
	            -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name \
	            $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
	            $cls_str $adt_str \
	            $LDLIBMACH
!
        fi
    fi
	exit 5
fi


chmod 4755 $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name

echo ""
echo "Done loading $merge_name:"
echo `ls -l $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name`
echo "Creating links to $merge_name..."

prgs="cacheutil dmfacp dmfjsp dmfrcp iidbms iishowres iistar lartool lockstat logdump logstat rcpconfig rcpstat repstat"

cd $II_SYSTEM/ingres/bin
for p in $prgs
do
    pr=$II_SYSTEM/ingres/bin${add_onhb_dir}/$p
    rm -f $pr
    $LINKCMD ./$merge_name $pr
    if [ $? -eq 0 ] 
    then
      echo "$pr linked to $merge_name" 
    else
      cat << !

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| Unable to create a link.  Please correct the problem before running   |
| $CMD again.
|									|
 -----------------------------------------------------------------------

The command being executed was:

    $LINKCMD $II_SYSTEM/ingres/bin${add_onhb_dir}/$merge_name $pr

!
      exit 7
    fi
done

echo "Links to $merge_name have been created."


#-----------------------------------------------------------------------
#  If we're not doing iimerge, load iidbms if we're supposed to...

elif $dbms
then

cat << !

Loading iidbms ...
!

if [ "$shared" = "true" ] ;
then

$CC $CCLDMACH $LDSRVOROGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
	$LDLIBMACH

else

    if [ "$archive" = "true" ] ; then

        $CC $CCLDMACH $CCLDSERVER \
		-o $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.a \
                $cls_str $adt_str \
                $LDLIBMACH

    else

        $CC $CCLDMACH $CCLDSERVER \
	        -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name \
	        $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
	        $cls_str $adt_str \
	        $LDLIBMACH

    fi
fi

if [ $? != 0 ]
then
	cat << !

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| Cannot load the DBMS binary, iidbms\. Please correct the problem	|
| before running $CMD again.						 
|									|
 -----------------------------------------------------------------------

The command used to load this binary was:
!

    if [ "$shared" = "true" ] ;
    then
	cat << !

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
	$LDLIBMACH

!

    else
        if [ "$archive" = "true" ] ; then

            cat << !

            $CC $CCLDMACH $CCLDSERVER \
		-o $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.a \
                $LDLIBMACH

!

        else

	    cat << !

            $CC $CCLDMACH $CCLDSERVER \
		    -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name \
	            $II_SYSTEM/ingres/lib${add_onhb_dir}/iimerge.o \
	            $cls_str $adt_str \
	            $LDLIBMACH

!
        fi
    fi
	exit 2
fi

chmod 4755 $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name

echo ""
echo "Done loading $dbms_name:"
echo `ls -l $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name`


#-----------------------------------------------------------------------
#   Link iistar to iidbms

    pr=$II_SYSTEM/ingres/bin${add_onhb_dir}/$star_name
    rm -f $pr
    $LINKCMD $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name $pr
    if [ $? -eq 0 ] 
    then
      echo "$pr linked to $dbms_name" 
    else
      cat << !

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| Unable to create a link.  Please correct the problem before running   |
| $CMD again.
|									|
 -----------------------------------------------------------------------

The command being executed was:

    $LINKCMD $II_SYSTEM/ingres/bin${add_onhb_dir}/$dbms_name $pr

!
      exit 7
    fi

#-----------------------------------------------------------------------
#  If we're loading dmfrcp and dmfjsp separately, do that now...

if $rcpjsp
then

cat << !

Loading dmfrcp ...
!

if [ "$shared" = "true" ] ;
then

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
	$LDLIBMACH

else
    if [ "$archive" = "true" ] ; then

        $CC $CCLDMACH $CCLDSERVER \
		-o $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.a \
                $cls_str $adt_str \
                $LDLIBMACH

    else

        $CC $CCLDMACH $CCLDSERVER \
		-o $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name \
	        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o \
	        $cls_str $adt_str \
	        $LDLIBMACH

    fi
fi

if [ $? != 0 ]
then
	cat << !

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| Cannot load the RCP binary, dmfrcp. Please correct the problem	|
| before running $CMD again.						 
|									|
 -----------------------------------------------------------------------

The command used to load this binary was:
!
    if [ "$shared" = "true" ] ;
    then
	cat << !

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
        $LDLIBMACH

!

    else
        if [ "$archive" = "true" ] ; then

            cat << !

            $CC $CCLDMACH $CCLDSERVER \
		    -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name \
                    $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o \
                    $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.a \
                    $cls_str $adt_str \
                    $LDLIBMACH

!
        else

	    cat << !

            $CC $CCLDMACH $CCLDSERVER \
		    -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name \
	            $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfrcp.o \
	            $cls_str $adt_str \
	            $LDLIBMACH

!
        fi
    fi
	exit 3
fi

chmod 4755 $II_SYSTEM/ingres/bin/$rcp_name

echo ""
echo "Done loading $rcp_name:"
echo `ls -l $II_SYSTEM/ingres/bin${add_onhb_dir}/$rcp_name`

cat << !

Loading dmfjsp ...
!

if [ "$shared" = "true" ] ;
then

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
        $LDLIBMACH

else

    if [ "$archive" = "true" ] ; then

        $CC $CCLDMACH -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.a \
                $cls_str $adt_str \
                $LDLIBMACH

    else

        $CC $CCLDMACH -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name \
	        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o \
	        $cls_str $adt_str \
	        $LDLIBMACH

    fi
fi

if [ $? != 0 ]
then
	cat << !

 -----------------------------------------------------------------------
|			    ***ERROR***					|
|									|
| Cannot load the JSP binary, dmfjsp. Please correct the problem	|
| before running $CMD again.						 
|									|
 -----------------------------------------------------------------------

The command used to load this binary was:

!
    if [ "$shared" = "true" ] ;
    then

	cat << !

$CC $CCLDMACH $LDSRVORIGIN -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name \
        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o \
        -L$II_SYSTEM/ingres/bin${add_onhb_dir} -lbecompat.1 \
	-lscf.1 -lpsf.1 \
	-lopf.1 -lrdf.1 \
        -lqef.1 -lqsf.1 \
	-ldbutil.1 -ltpf.1 \
	-lrqf.1 -lgcf.1 \
	-lsxf.1 -lcuf.1 \
        -lgwf.1 -ldmf.1 \
	-lulf.1 -ladf.1 \
        $cls_str $adt_str \
        $LDLIBMACH

!

    else
        if [ "$archive" = "true" ] ; then

            cat << !

            $CC $CCLDMACH -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o \
                $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.a \
                $cls_str $adt_str \
                $LDLIBMACH

!
        else

	    cat << !

            $CC $CCLDMACH -o $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name \
	        $II_SYSTEM/ingres/lib${add_onhb_dir}/dmfjsp.o \
	        $cls_str $adt_str \
	        $LDLIBMACH

!
        fi
    fi
	exit 4
fi

chmod 4755 $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name

echo ""
echo "Done loading $jsp_name:"
echo `ls -l $II_SYSTEM/ingres/bin${add_onhb_dir}/$jsp_name`


fi
fi

exit 0
