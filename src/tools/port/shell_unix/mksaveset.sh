:
#
# Copyright (c) 1994, 2006 Ingres Corporation
#
#
# DEST = utility
#
## History
##	20-Oct-94 (GordonW)
##		created,
##	24-Oct-94 (GordonW)
##		added -no_ingres flag.
##	25-Oct-94 (GordonW)
##		add in taringres call.
##	15-Nov-94 (GordonW)
##		test if buildrel, tar worked,.
##	16-Nov-94 (GordonW)
##		use II_RELEASE_DIR not II_SYSTEM/install
##	19-Nov-94 (GordonW)
##		use either II_RELEASE_DIR or $II_SYSTEM/ingres/install to
##		find release.dat
##      29-Dec-94 (GordonW)
##              use $ING_SRC/tools/port/conf/CONFIG to get version id.
##	29-Dec-94 (GordonW)
##		left debugging in.
##	27-Feb-95 (hanch04)
##		added check for II_RELEASE_DIR, to prevent rm -rf *
##	18-Oct-96 (hanch04)
##		changed tartools to do compression
##      29-aug-97 (musro02)
##              Remove -C in alltar; not every platform has this feature
##	01-oct-97 (popri01)
##		added another II_RELEASE_DIR sanity check
##      12-jun-98 (musro02)
##              Remove setting of rm_release when $1 = -notools
##	02-sep-97 (rosga02)
##		Get $ver from cl/hdr/hdr/gv.h instead of
##		$ING_SRC/tools/port/conf/CONFIG where ING_VER may have more
##		than one occurence due to #ifdef. That may lead to a blank
##		in the tarfile and that in turn leads to an error in dirname
##		command on some platforms (like Solaris/x86) and thus blank in
##		fdir.    
##	21-dec-99 (vande02) (bonro01)
##		Added 'tartools -a' to audit the tools tar before attempting to
##		compress tools.tar (it may not exist if errors on files)
##      07-dec-01 (loera01) Bug 106565
##              Modified for other product builds.
##	15-jul-02 (devjo01)
##		Put double quotes around $conf_<product> to avoid run-time
##		syntax error if environment variable conf_<product> is
##		undefined.
##	17-Oct-2002 (bonro01)
##		Added unmksaveset to list of files to cleanup.
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##	15-Jul-2004 (hanje04)
##		Define DEST=utility so file is correctly pre-processed
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##	20-July-2005 (bonro01)
##		Re-write for Opensource saveset.
##	01-Sep-2005 (bonro01)
##	 	Use correct vers for Hybrid builds.
##	01-Sep-2005 (hweho01)
##	 	Modify the package name for IBM-AIX release.
##	28-Sep-2005 (bonro01)
##		Change package to only contain generic and one platform
##		readme.  Cancel package build if files are not found.
##	30-Sep-2005 (bonro01)
##		Include proper readme file for hybrid build.
##	12-Oct-2005 (hanje04)
##		Add RPM support for Linux. Pick up II_RELEASE_DIR if it's 
##	 	already set. Parse arguments with case statement instead of
##		if
##	14-Oct-2005 (hanje04)
##		GRRRRR. Need to include BASE package with RPMS!
##	20-Oct-2005 (hanje04)
##	 	Mac is mg5_osx not mac_osx
##	20-Oct-2005 (sheco02)
##		Modify to genrate NPTL directory package for rpm and include 
##		readme_int_lnx_nptl.html.
##	25-Oct-2005 (sheco02)
##		Minor fix to the above change.
##	02-Nov-2005 (sheco02)
##		Fixed tardir to be consistent to Linux rpm directory format.
##	05-Nov-2005 (hanje04)
##		Apply NPTL change to all Linuxes
##	29-Nov-2005 (hanje04)
##		Make sure we include NPTL in tardir where apropriate
##	04-Jan-2006 (bonro01)
##		Add -unhide option to include i18n, i18n64, and
##		documentation packages.
##	10-Jan-2006 (hanje04)
##		SIR 115662
##		Fix up RPM options to work with -unhide.
##		Generate package list from rpmconfig rather than use a 
##		static string.
##		Remove CATOSL references.
##	24-Jan-2004 (hanje04)
##		Always build all the RPMS, just roll up what we need in
##		the archive.
##	31-Jan-2006 (hanje04)
##	 	SIR 115688
##		Add -L flag allow savesets to be created with different
##		license dependencies. Currently only meaningful on Linux,
##		default is gpl.
##		Also append license type to saveset name too.
##	01-Feb-2006 (hanje04)
##		SIR 115688
##		Replace -L with -l so it doesn't cause problems on VMS.
##	01-Feb-2006 (hanje04)
##		Make sure unhidepkg is correctly set for RPM build
##	02-Feb-2006 (hanje04)
##		Bug 115239
##		Add -p flag to include spatial objects RPM in saveset.
##	03-Feb-2006 (hanje04)
##		SIR 115688
##		add -eval flag for evaluation saveset.
##	06-Feb-2006 (hanje04)
##		Grab $ING_SRC/LICENSE if we can't find LICENSE.${lictype}
##	21-Feb-2006 (bonro01)
##		The LICENSE file also needs to exist in the install directory
##		in the ingres.tar so that if the ingres.tar is used to
##		directly install Ingres, ingbuild will be able to find the
##		license to display during installation.
##		Use LICENSE file in $II_RELEASE_DIR/install/LICENSE to insure
##		that both versions of the LICENSE file match.
##		"buildrel -l" must specify which LICENSE to include in
##		the ingres.tar
##	23-Feb-2006 (bonro01)
##		Insure that release area is recreated with proper LICENSE
##		file. Fix parsing of -l parameter
##	10-Apr-2006 (hweho01)
##	        1) Added support for spatial release on Unix platform.	
##              2) Use "-s" in $spatflag to call buildrel because   
##                 "-p" is no longer a spatial parameter in that process.  
##              3) Change parameter -p into -sol for specifying spatial    
##                 package when mksaveset is invoked.  
##      10-May-2006 (hweho01)
##              Modified the option for Spatial release. 
##              Syntax to generate spatial release on Unix platforms :  
##                mksaveset             -- make Ingres releases 
##                mksaveset -sol        -- make Spatial release
##                mksaveset -all        -- make Ingres and Spatial release
##	15-Jun-2006 (hanje04)
##		When creating non-RPM saveset on Linux, add 'ingbuild' to the
##		'desc' variable to differentiate it from the RPM saveset.
##	11-May2006 (hanje04)
##		Bump release to Ingres 2007
##      18-Jun-2006 (hweho01)
##              Compression will not further reduce the size of tar file    
##              on AIX platforms, so skip it and save time.
##      18-Jun-2006 (hweho01)
##              Compression will not further reduce the size of tar file
##              on HP Tru64 platforms, so skip it and save time.
##	16-Jun-2006 (hanje04)
##		BUG 115239
##		For RPM there is no spatial saveset, just the RPM. Create it
##	 	copy it it II_RELEASE_DIR and we're done.
##	25-Oct-2006 (hanje04)
##		SIR 116877
##		Format of RPM saveset has to be changed for new GUI installer.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##	01-Dec-2006 (hanje04)
##		SIR 116877
##		Add pixmaps to saveset for GUI installer.
##	07-Dec-2006 (hanje04)
##		If the core RPM has no license dependencies, rebuild the 
##		license RPM anyway to keep mklicense happy.
##	 	NOTE: This change may be removed if the removal of license
##		dependencies is confirmed as permanent
##	03-Jan-2007 (bonro01)
##		Fix md5sum.txt so it does not checksum itself.
##	22-Jan-2007 (bonro01)
##		Change version back for ingres2006 release 3
##	20-Feb-2007 (bonro01)
##		Make ingbuild saveset file extension the same as the RPM
##	23-Mar-2007
##	    SIR 117985
##	    Add support for 64bit PowerPC Linux (ppc_lnx)
##	08-Feb-2008 (hanje04)
##	    SIR S119978
##	    Add support for Intel OS X
##	11-Feb-2008 (bonro01)
##	    Include Spatial object package into standard ingbuild and RPM
##	    install images instead of having a separate file.
##      11-Aug-2008 (hweho01)
##          Change saveset name from ingres2006 to ingres for 9.2 release,
##          use (PROD_PKGNAME) which will be substituted by jam during build.
##	20-Jul-2009 (bonro01)
##	    Use $build_arch to identify the correct Hybrid VERS setting.
##	06-Aug-2009 (hanje04)
##	    BUG 122571
##	    Add python package querying module ingpkgqry to RPM saveset.
##	08-Oct-2009 (hweho01) SIR 122138
##	    Avoid error "test: argument expected" on Tru64, check the   
##	    length of $build_arch setting before making the comparison,   
##	    because it is not defined for the non-hybrid-capables port.  
##	29-Mar-2010 (bonro01)
##	    Add -docrpm option to create the optional documentation rpm.
##	21-May-2010 (hanje04)
##	    SIR 123791
##	    License acceptence dialog is now part of the GUI, so just include
##	    the LICENSE file in the root of the saveset instead of
##	    ingres-LICENSE
##	
##                                         
. readvers
. (PROG1PRFX)sysdep
[ $SHELL_DEBUG ] && set -x
unamem=`uname -m`
unames=`uname -s`

if [ -z "$ING_ROOT" ]; then
    echo "Variable ING_ROOT must be defined."
    exit 1
fi

# use correct vers for Hybrid build
if [ -n "$build_arch" -a "$build_arch" = "32+64" ]; then
    vers=$VERS64
else
    vers=$VERS
fi

tarfile=""
rpmrelease=false
unhidepkg=""
lictype=gpl
spatflag=""
productname=(PROD_PKGNAME)
createdoc=false

if [ -f "$ING_BUILD/files/spatial.h" ]; then
    spatflag="-sol"
fi

while [ "$1" ]
do
    case "$1" in
	-docrpm)
		case $vers in
		    *_lnx|int_rpl)
			rpmrelease=true
			createdoc=true
			unhidepkg="-u"
			rpmrelease=true
			;;
		    *)
			echo "DOCRPM not supported on ${unamem} ${uanmes}"
			exit 1
			;;
		esac
		shift
		;;
	-unhide)
		unhidepkg="-u"
		shift
		;;
	-rpm)   case $vers in
		    *_lnx|int_rpl)
			rpmrelease=true
			;;
		    *)
			echo "RPM not supported on ${unamem} ${uanmes}"
			exit 1
			;;
		esac
		shift
		;;
	    -l)	
		# Verify specified license if valid
		case $2 in
		    gpl|\
		    com|\
		   eval|\
		     emb) lictype=$2
			  shift
			  ;;
		      *) echo "$2 is not a valid license: use gpl, com, emb or  eval"
			 exit 1
			 ;;
		esac
		shift
		;;
	    -sol|\
	    -SOL)
		spatflag="-sol"
		shift
		;;
	    -nosol|\
	    -NOSOL)
		spatflag=""
		shift
		;;
	     *)
		tarfile="$1"
		shift
		;;
    esac
done

set - `cat $ING_BUILD/version.rel` 
release=$2
nptlbuild=`head -1 $ING_BUILD/version.rel |cut -d')' -f2`
case $vers in
    int_lnx|\
    int_rpl)
	if $rpmrelease ; then
	    desc="pc-linux-i386"
	else
	    desc="pc-linux-ingbuild-i386"
	fi
	arch=i386
	;;
    a64_lnx)
	if $rpmrelease ; then
	    desc="linux-x86_64"
	else
	    desc="linux-ingbuild-x86_64"
	fi
	arch=$unamem
	;;
    i64_lnx)
	if $rpmrelease ; then
	    desc="linux-ia64"
	else
	    desc="linux-ingbuild-ia64"
	fi
	arch=$unamem
	;;
    ppc_lnx)
	if $rpmrelease ; then
	    desc="linux-ppc"
	else
	    desc="linux-ingbuild-ppc"
	fi
	arch=$unamem
	;;
    int_w32)
	desc="win-x86"
	;;
    a64_win)
	desc="win-x86_64"
	;;
    i64_win)
	desc="win-ia64"
	;;
    su9_us5)
	desc="sun-solaris-sparc-32-64bit"
	;;
    a64_sol)
	desc="sun-solaris-x86-32-64bit"
	;;
    r64_us5)
	desc="ibm-powerpc-32-64bit"
	;;
    hp2_us5)
	desc="hp-hpux-pa-risc-32-64bit"
	;;
    i64_hpu)
	desc="hp-hpux-ia64-32-64bit"
	;;
    axp_osf)
	desc="hp-tru64-5.1"
	;;
    axm_vms)
	desc="hp-vms-alpha"
	;;
    i64_vms)
	desc="hp-vms-ia64"
	;;
    usl_us5)
	desc="unixware-x86"
	;;
    mg5_osx)
	desc="apple-darwin-powerpc"
	;;
    int_osx)
	desc="apple-darwin-i386"
	;;
    *)
	echo "Config string $VERS unrecognized."
	exit 1
	;;
esac


# Append license type
desc="${lictype}-${desc}"
# Append NPTL if we need to
[ "$nptlbuild" = "NPTL" ] && desc="NPTL-${desc}"
# Generate RPM package list
$rpmrelease &&
{
    if [ "$unhidepkg" ] 
    then
	pkgmask="LICENSE"
    else
	pkgmask="i18n|documentation|LICENSE"
    fi

    if [ -z "$spatflag" ]; then
	pkgmask="${pkgmask}|spatial"
    fi

    pkglist=`grep '^$spec [[:alnum:]]' $II_MANIFEST_DIR/rpmconfig | \
		cut -d' ' -f2 | egrep -v "$pkgmask"`

}

#
# Making Ingres release on Linux and Unix platforms
#

[ -z "$II_RELEASE_DIR" ] &&
{
    if [ "$createdoc" = "true" ] ; then
        II_RELEASE_DIR=$ING_ROOT/release/b${build}_doc
        II_RPM_BUILDROOT=$ING_ROOT/rpm_doc
        export II_RPM_BUILDROOT 
    else
        II_RELEASE_DIR=$ING_ROOT/release/b${build}
    fi
    export II_RELEASE_DIR
}

if [ ! -d "$II_RELEASE_DIR" ]; then 
    mkdir -p $II_RELEASE_DIR
fi
if [ ! -d "$II_RELEASE_DIR" ]; then 
    echo "Could not create $II_RELEASE_DIR."
    exit 1
fi

if [ ! -d "$ING_ROOT/release" ]; then 
    mkdir -p $ING_ROOT/release
fi
if [ ! -d "$ING_ROOT/release" ]; then 
    echo "Could not create $ING_ROOT/release."
    exit 1
fi

# Run buildrel audit to ensure file permissions are correct
if [ "$createdoc" = "true" ]; then
    echo y | buildrel -a -u -l ${lictype}
else
    echo y | buildrel -a -l ${lictype}
fi

cd $II_RELEASE_DIR
echo "Filling Ingres release area $II_RELEASE_DIR..."
buildrel $unhidepkg ${spatflag} -l ${lictype}  ||
{
    echo "Can't do a buildrel..."
    exit 1
}
    

$rpmrelease && 
{
    echo "Generating RPM SPEC files"
    buildrel -r $unhidepkg ${spatflag} -l ${lictype} ||
    {
	echo "Failed to create SPEC files..."
	exit 1
    }
}

tardir=${productname}-$release-$build-$desc
if [ "$rpmrelease" = "true" -a "$createdoc" = "true" ] ; then
    iirpmbuild -unhide documentation
    cp -p $II_RPM_BUILDROOT/${arch}/${productname}-documentation-${release}-${build}.${arch}.rpm $ING_ROOT/release
    rm -rf $II_RPM_BUILDROOT $II_RELEASE_DIR
    exit 0
else 
if $rpmrelease ; then
    skipbld=false
    baserpm=${productname}-${release}-${build}.${arch}.rpm

    for pkg in $pkglist
    do
	rpm=${productname}-${pkg}-${release}-${build}.${arch}.rpm
	[ -f ${II_RPM_BUILDROOT}/${arch}/${rpm} ] || \
		[ -f ${II_RPM_BUILDROOT}/${arch}/${baserpm} ] &&
	{
	    echo "WARNING: One or more RPM already exists for this build"
	    echo "RPMs will NOT be recreated, existing files will be used"
	    skipbld=true
	    break
	}
    done

    # build rpms if we need to
    [ "${unhidepkg}" ] && unhide=-unhide
    $skipbld || iirpmbuild ${unhide} ${spatflag} ||
    {
	echo "Failed to create one or more RPM..."
	exit 1
    }

    tarfile=$tardir.tgz
else
    cd $ING_ROOT/release
    echo "Making $ING_ROOT/release/b${build}.tar..."
    taringres -nocompress $ING_ROOT/release/b${build}.tar ||
    {
	echo "Can't tar $ING_ROOT/release/b${build}.tar..."
	exit 1
    }

    tarfile=$tardir.tar
fi
fi

saveset=$ING_ROOT/release/$tardir

if [ ! -d $saveset ]; then
    mkdir $saveset
fi

# clear saveset directory
rm -rf $saveset/*

# Recreate saveset directory
cd $saveset
cp -p $ING_BUILD/bin/ingres_express_install ingres_express_install.sh || exit 1
cp -p $II_SYSTEM/readme.html . || exit 1
if [ "$nptlbuild" ] && [ "$nptlbuild" = "NPTL" ]; then  
cp -p $II_SYSTEM/readme_${vers}_nptl.html . || exit 1
else
cp -p $II_SYSTEM/readme_${vers}.html . || exit 1
fi

if ${rpmrelease}
then
    ## Format for RPM saveset is dictated by the needs of the GUI installer
    ## Layout should look like this:
    ##
    ##	<saveset root>
    ##		|
    ##		+--ingres_install
    ##		|
    ##		+--ingres_express_install
    ##		|
    ##		+--LICENSE
    ##		|
    ##		+--md5sum.txt
    ##		|
    ##		+-bin
    ##		|   |
    ##		|   +--inginstgui
    ##		|   |
    ##		|   +--ingpkgqry
    ##		|   |
    ##		|   +--iirpmrename
    ##		|   |
    ##		|   +--uninstall_ingres
    ##		|
    ##		+-pixmaps
    ##		|   |
    ##		|   +--<images need by installer>
    ##		|
    ##		+-rpm
    ##		|   |
    ##		|   +--<rpm packages>
    ##		|
    ##		+-- readme.html
    ##		|
    ##		+-- readme_${vers}.html
    ##
    ##

    # create bin, pixmaps and rpm directories
    mkdir bin pixmaps rpm || exit 1
    
    cp -p $ING_BUILD/bin/ingres_install . || exit 1
    cp -p $ING_BUILD/utility/iirpmrename bin || exit 1
    cp -p $ING_BUILD/bin/uninstall_ingres bin || exit 1
    cp -p $ING_BUILD/utility/inginstgui bin || exit 1
    cp -p $ING_BUILD/utility/ingpkgqry bin || exit 1

    echo "Copying RPMS from ${II_RPM_BUILDROOT}..."
    baserpm=${productname}-${release}-${build}.${arch}.rpm
    # Check base package has correct licensing info
    # if it has any at all
    licdep=`rpm -qp --requires ${II_RPM_BUILDROOT}/${arch}/${baserpm} | \
		grep ${productname}-license`
    if [ ${licdep} ] 
    then
	[ "${licdep}" = "${productname}-license-${lictype}" ] ||
	{
	    cat << EOF
The base package:

	${II_RPM_BUILDROOT}/${arch}/${baserpm}

Doesn't contain correct dependencies for the ${lictype} license, rebuilding...

EOF
	    # Remove the base package and rebuild it with the correct
	    # license dependencies
	    rm -f ${II_RPM_BUILDROOT}/${arch}/${baserpm}
	    buildrel -r -l ${lictype}
	    iirpmbuild core LICENSE
	
	    # Check it's right this time
	    ( rpm -qp --requires \
		${II_RPM_BUILDROOT}/${arch}/${baserpm} \
        	| grep -q ${productname}-license-${lictype} ) ||
	    {
		cat << EOF
Error
The base package:

        ${II_RPM_BUILDROOT}/${arch}/${baserpm}

Doesn't contain correct dependencies for the ${lictype} license, even after
rebuilding. Problem must be resolved before saveset can be built.

EOF
		exit 1
	    }

	}
    else
	Just rebuild the license RPM to keep mklicense happy
	buildrel -r -l ${lictype}
	iirpmbuild LICENSE
    fi

    # licensing
    # mklicense -l ${lictype} ||
    #{
    #	echo "Failed to create ingres-LICENSE"
    #	exit 1
    #}
    # License now displayed by GUI, just copy license file into place
    if [ -r $ING_SRC/LICENSE.${lictype} ]
    then
	cp $ING_SRC/LICENSE.${lictype} ./LICENSE || exit 1
    else
	cat << EOF
Error
Cannot locate license file:

	\$ING_SRC/LICENSE.${lictype}

The license file is required to before a saveset can be built

EOF
	exit 1
    fi


    # Copy RPMS
    cp ${II_RPM_BUILDROOT}/${arch}/${baserpm} rpm || exit 1
    for pkg in $pkglist
    do
	rpm=${productname}-${pkg}-${release}-${build}.${arch}.rpm
	[ -f ${II_RPM_BUILDROOT}/${arch}/${rpm} ] ||
	{
	     echo "Cannot locate ${II_RPM_BUILDROOT}/${arch}/${rpm}"
	     exit 1
	}
	cp ${II_RPM_BUILDROOT}/${arch}/${rpm} rpm || exit 1
   done

    # Copy images
    cp $ING_BUILD/files/pixmaps/* pixmaps ||
    {
	echo "Failed to find images used by installer"
	exit 1
    }

    # generate md5sums
   rm -f md5sum.txt
   find . -type f -a -not -name md5sum.txt -exec md5sum {} >> md5sum.txt \;

else
    cp -p $ING_BUILD/utility/install install.sh || exit 1
    if [ -r "${II_RELEASE_DIR}/install/LICENSE" ] ; then
	cp -p ${II_RELEASE_DIR}/install/LICENSE LICENSE || exit 1
    elif [ -r "${ING_SRC}/LICENSE" ] ; then
	cp -p ${ING_SRC}/LICENSE LICENSE || exit 1
    else
	echo "Cannot locate License file..."
	exit 1
    fi

    mv $ING_ROOT/release/b${build}.tar ingres.tar
fi

# Create the saveset
cd $ING_ROOT/release
rm -f $tarfile
echo "Making CD-ROM image saveset ($tarfile)..."
if $rpmrelease
then
    tar cvzf $tarfile $tardir ||
    {
	echo "Error creating $tarfile..."
	exit 1
    }
else
    tar cvf $tarfile $tardir ||
    {
	echo "Can't tar $tarfile..."
	exit 1
    }
    # Compress the saveset
    [ -x /usr/bin/compress ] && compress=/usr/bin/compress
    [ "$compress" = "" -a -x /bin/compress ] && compress=/bin/compress
    [ "$compress" = "" ] && compress=compress
    case $VERS in
	  *_lnx|\
        int_rpl)
	    echo "Compressing CD-ROM image saveset ($tarfile)..."
	    rm -rf $tarfile.gz
	    gzip $tarfile
	    mv $tarfile.gz $tardir.tgz
	    ;;
	usl_us5|\
	hp2_us5|\
	i64_hpu|\
	rs4_us5|\
	axp_osf)
	    # gzip is not available in default installation
	    # and compress will not furthur compress ingres.tar
	    ;;
	*)
	    echo "Compressing CD-ROM image saveset ($tarfile)..."
	    rm -f $tarfile.Z
	    $compress $tarfile
	    status=$?
	    if [ $status -eq 2 ]; then
		echo "File $tarfile is not compressible."
	    fi
	    ;;
    esac
fi

echo "Ingres release is done..."

exit 0
