:
## Copyright (c) 2004, 2010 Ingres Corporation 
## Name:
##	iirpmbuild.sh
##
## Usage:
##	iirpmbuild
##
## Description:
##
##	Builds RPM packages from existing Ingres build area.
##
## Depends on:
##
##  II_RPM_BUILDROOT = directory to unpack install image, destination 
##		      for built RPMs
##  II_RPM_PREFIX    = default target directory for RPM install
##                     must match prefix in spec file!
##  II_RELEASE_DIR   = buildrel output directory (to locate install.pgz)
##
## History
##
##	24-oct-2003 (fanch01)
##	    Created.
##	27-Oct-2003 (hanje04)
##	    Added header and comments etc.
##	29-Jan-2004 (hanje04)
##	    Sign rpm packages
##	06-Mar-2004 (hanje04)
##	    Add RPMBLD variable to denote build command so that script will 
##	    work against rpm 3.0.6 as well as 4.1.
##	    Also remove rpmPackage function as it is no longer used.
##	27-Mar-2004 (hanje04)
##	    Make sure RPM are always built with defined architecture.
##	    For now, i386
##	28-Jun-2004 (hanje04)
##	    Add functionality to build Embedded saveset.
##	15-Jul-2004 (hanje04)
##	    Make sure libioutil.1.so is copied over to the RPM build area
##	    as it's NOT in the regular manifest.
##	17-Jul-2004 (hanje04)
##	    Make sure mdb.tar.gz is copied to the RPM build area.
##	13-Aug-2004 (hanje04)
##	    Make sure files/mdb dir exists. Exclude ice, spatial, secure
##	    packages from embedded saveset.
##	30-Jul-2004 (hanje04)
##	    Don't build Embedded saveset if II_RPM_EMB_PREFIX isn't set.
##	12-Aug-2004 (hanje04)
##	    Update for replacement of tar archives with pax archives.
##	29-Aug-2004 (hanje04)
##	    Only create documentation area if we need to.
##	21-Sep-2004 (hanje04)
##	    SIR 113101
##	    Remove all trace of EMBEDDED or MDB setup. Moving to separate 
##	    package.
##	22-Sep-2004 (hanje04)
##	    Update doc link to reflect new version.
##	25-Oct-2004 (hanje04)
##	    Add support for MDB package.
##	22-Nov-2004 (hanje04)
##	   Get build number from VERS file
##	05-Jan-2005 (hanje04)
##	   Get Ingres version from CONFIG file.
##	18-Jan-2005 (hanje04)
##	   Add support for other Linuxes.
##	19-Jan-2004 (kodse01)
##	   Corrected the comments to work for i64_lnx.
##	08-Mar-2005 (hanje04)
##         SIR 114034
##	   Add support for reverse hybrid builds.
##      04-Jan-2006 (hanje04)
##	    SIR 115662
##	    Virtually a complete re-write:
##          Change method such that only the archives that are required for
##          a particular packages are actually unpacked. Then remove them
##          once we're done. This allows RPM to package much more efficiently.
##          Allow packages for building to be specified on the command line.
##	24-Jan-2006 (hanje04)
##	    Move setting of -unhide flag so that pkglist is set correctly
##	31-Jan-2006 (hanje04)
##	    SIR 115688
##	    Enable building of licesing RPM.
##	06-Feb-2006 (hanje04)
##	    Exclude spatial by default and add -p flag to include it
##	13-Feb-2006 (hanje04)
##	    Remove don't abort if flags aren't matched.
##	14-Feb-2006 (hanje04)
##	    Make sure pkglst is set correctly when -p or -u are specified.
##	16-Jun-2006 (hanje04)
##	    BUG 115239
##	    spatial is now just a standalone package so add -sol flag
##	    to just create this package.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##	23-Mar-2007
##	    SIR 117985
##	    Add support for 64bit PowerPC Linux (ppc_lnx)
##	2-Nov-2007 (bonro01)
##	    Fix -unhide parameter to allow building RPM with documentation.
##	11-Feb-2008 (bonro01)
##	    Remove separate Spatial object install pacakge.
##	    Include Spatial object package into standard ingbuild and RPM
##	    install images instead of having a separate file.
##	22-Jun-2009 (kschendel) SIR 122138
##	    Readvers figures out primary vers now.
##	13-Aug-2009 (hanje04)
##	    BUG 122571
##	    Define buildroot to rpmbuild command as it defaults to ~/SRPM
##	    for newer versions of the rpm toolset (4.6+)
##	    Remove 'rpm' as build command, no longer supprted
##	10-Jan-2010 (hanje04)
##	    SIR 123296
##	    Change the method a little so that each package is created
##	    separately. This improves build time and means default build
##	    options can be used.
##	    Add support for LSB Builds. Buildrel creates savesets with full
##	    dir tree, so method reflects that.
##	    Also add -extract-only to assist with SRPM building
##	04-Mar-2010 (hanje04)
##	    SIR 123296
##	    Add -use-dir <dir> to allow all package to be extracted to the 
##	    same directory. Used by SRPM build. Make sure we report errors
##	    more clearly. Fix building of documentation package.
##	05-Apr-2010 (hanje04)
##	    SIR 123296
##	    Add -gen-flists to generate file and directory lists for SRPM
##	    builds under $ING_BUILD/files/srpmflists. Under Fedora packaging
##	    standards. RPMs should own all directories they create as well as
##	    files.
##	07-Apr-2010 (hanje04)
##	    SIR 123296
##	    Exclude %{_sysconfdir}/pam.d from dirlists
##

self=`basename $0`
ARSFX=pgz

# Check VERS file
. readvers

vers=$config

# Set rpm architecture
case $vers in
    #IA32 Linux
    int_lnx|\
    int_rpl)    ARCH=i386
		;;
    #IA64 Linux | AMD64 LINUX | S390 Linux
   i64_lnx|\
   a64_lnx|\
   ppc_lnx|\
    ibm_lnx)
		ARCH=`uname -m`
		;;
    *)	    echo "$config is not a recognised Linux platorm"
	    exit 1
		;;
esac

if [ -x /usr/bin/rpmbuild ] ; then
    RPMBLD=rpmbuild
else
    echo "Cannot locate 'rpmbuild', aborting..."
    exit 1
fi

shopt -s nullglob

usage()
{
    args="[-unhide][-extract-only[-use-dir <dir>]][-gen-flists"
    for a in $pkglst
    do
        args="${args}] [${a}"
    done

    args="${args}]"
	
    # FIXME!! format the argument list
    cat << EOF 

Usage:
$self $args

EOF

    exit 1
}
	   
#
# -[ error() ] - Wrapper for echoing errors
#
error()
{
    msg="Unspecified error"
    [ "$1" ] && msg="$1"

    cat << EOF
Error:
$msg

EOF

    return 0
}
# error()

#
#-[ clean_exit() ] - Clean up and exit with specified error or zero-----------
#
clean_exit()
{
    # Set return code
    rc=0
    [ $1 ] && rc=$1

    # Reset any traps and exit
    trap - 0
    exit $rc
}
# clean_exit()

#
#-[ get_pkg_list() ] - Get Valid rpm package list from manifest
#
get_pkg_list()
{
    [ -r "${rpmconfig}" ] ||
    {
	error "Cannot locate ${rpmconfig}"
	clean_exit 2
    }

    # Remove unwanted packages from the build list
    if ${unhide}
    then
        pkgmask=DUMMY
    else
        pkgmask="i18n|documentation"
    fi

    if ! ${spatial}
    then
	pkgmask="${pkgmask}|spatial"
    fi

    # Scan rpmconfig for package
    grep '^$spec' "${rpmconfig}" | awk '
	$2 == "" { printf "core\n" ; }
	$2 != "" { printf "%s\n",$2 ; }' | \
	egrep -v ${pkgmask}
}
# get_pkg_list()

envInit()
{
# check environment for proper variables
    rc=0
    if [ "$conf_LSB_BUILD" ] ; then
	export LSB_BUILD=true
    else
	export LSB_BUILD=false
    fi
    if [[ "$II_RPM_BUILDROOT" == "" ]];  then
	error "Set II_RPM_BUILDROOT to location for RPMs and install image"
	(( rc++ ))
    fi
    if $LSB_BUILD ; then
	# for LSB builds, archive contain full directory structure
	export II_RPM_PREFIX=""
 	doc_loc=""
	[ "$II_CONFIG" ] && ingsetenv II_CONFIG $II_CONFIG
    elif [[ "$II_RPM_PREFIX" == "" ]]; then
	error "Set II_RPM_PREFIX so that it matches the spec file prefix"
	(( rc++ ))
    elif (( $rc == 0 )); then
	mkdir -p "$II_RPM_BUILDROOT"
	if [[ ! -d "$II_RPM_BUILDROOT" ]]; then
	    error "$II_RPM_BUILDROOT does not exist and could not create"
	    (( rc++ ))
	fi
	doc_loc=/usr/share/doc
    fi
    
    # clear exdir if it's set
    [ -n "$exdir" -a "$exdir" != "/" ] && rm -rf $exdir/*

    # get version info from rpmconfig
    rpm_basename=`grep '^$sub.*rpm_basename' ${rpmconfig} | awk '{print $3}'`
    rpm_version=`grep '^$sub.*rpm_version' ${rpmconfig} | awk '{print $3}'`
    rpm_release=`grep '^$sub.*rpm_release' ${rpmconfig} | awk '{print $3}'`

    if [[ "$II_RELEASE_DIR" == "" || ! -d "$II_RELEASE_DIR"  ]]; then
	error "Set II_RELEASE_DIR to a valid directory containing buildrel output"
	(( rc++ ))
    fi

    export rpm_basename rpm_version rpm_release doc_loc
    return $rc

}

#
#-[ unpack_archives() - Unpack archives to be packaged in RPM ]
#
unpack_archives()
{
    [ "$1" ] || return 1
    if [ "$1" = "core" ] ; then
	local rpm=""
	local pkgdir=${rpm_basename}-${rpm_version}-${rpm_release}
	local prefix=$II_RPM_PREFIX
    elif [ "$1" = "LICENSE" ] ; then
	# no archive for LICENSE RPM
	return 0
    elif [ "$1" = "documentation" ] ; then
	local rpm="$1"
	local pkgdir=${rpm_basename}-${rpm_version}-${rpm}-${rpm_release}
	local prefix=$doc_loc
    else
	local rpm="$1"
	local pkgdir=${rpm_basename}-${rpm_version}-${rpm}-${rpm_release}
	local prefix=$II_RPM_PREFIX
    fi

    if ! $build && [ -n "$exdir" ] ; then
	local buildroot=$exdir/$prefix
    else
        local buildroot=$II_RPM_BUILDROOT/${pkgdir}/$prefix
    fi
   
    pkgmask=DUMMY
    if ! ${spatial} 
    then
        pkgmask="spatial"
    fi

    # get list of archives for RPM from rpmconfig using awk.
    # 	- scan through rpmconfig until we find the "$spec" value
    # 	  we're looking for.
    #   - When we do turn on printing and print out all the "$features"
    #	  associated with the "$spec"
    #   - When we find another "$spec" turn of printing and scan through to
    #	  the end.
    archlist=`awk -v spec=$rpm 'BEGIN{
	prnt=0 ;
	}
	prnt==0 && $1 == "$spec" && $2 == spec { prnt=1 ; }
	$1 == "$feature" && prnt == 1 { printf "%s\n", $2 ;} 
	prnt==1 && $1 == "$spec" && $2 != spec { prnt=0; } ' \
	"${rpmconfig}" | egrep -v ${pkgmask}`

    rc=$?

    # Check we have somesort of list
    [ -z "${archlist}" ] || [ $rc != 0 ] && 
    {
	error "Failed to create archive list"
	return 1
    }

    # Now clear the decks and unpack the archives
    [ -z "$exdir" ] && rm -rf ${buildroot}
    mkdir -p ${buildroot}
    cd ${buildroot}

    for arch in $archlist
    do
	[ -r "${II_RELEASE_DIR}/${arch}/install.${ARSFX}" ] ||
	{
	    cat << EOF
Warning:
Cannot locate ${II_RELEASE_DIR}/${arch}/install.${ARSFX}
skipping...

EOF
	    return 0
	}

	echo "unpacking $arch..."
	tar xzf ${II_RELEASE_DIR}/${arch}/install.${ARSFX} --no-same-permissions ||
	{
	    error "Failed to unpack $arch"
	    return 3
	}

	# Documentation is a little special
	if [ "$rpm" = "documentation" ] ; then
	    if $LSB_BUILD ; then
		cd usr/share/doc > /dev/null 2>&1 &&
		    mv ${rpm_basename}/english ${rpm_basename}-${rpm_version}
	    else
	        mv ingres/files/english \
		    ${buildroot}/${rpm_basename}-${rpm_version}
	    fi
	    [ $? != 0 ] &&
	    {
	        error "error: Could not create doc area under $buildroot"
	        return 5
	    }
	    rm -rf ingres
	fi
    done


    # create log dir for LSB builds
    if $LSB_BUILD && [ $rpm = "client" ] ; then
	mkdir -p ${buildroot}/var/log/ingres \
	${buildroot}/var/lib/ingres/files/rcfiles \
	    ${buildroot}/etc/rc.d/init.d &&
	        mkrc > /tmp/mkrc$$.out 2>&1 &&
		cp $ING_BUILD/files/rcfiles/ingres \
			${buildroot}/etc/rc.d/init.d ||
		{
		    cat /tmp/mkrc$$.out
		    return 3
		}
	rm -f /tmp/mkrc$$.out
    fi
    return 0

}

build_rpm()
{
    [ -z "$1" ] &&
    {
	error "No package specified for building"
	return 1
    }

    # generate spec file name and location
    local pkg=$1
    local specdir=ingres/files/rpmspecfiles
    if $LSB_BUILD ; then
	local specloc=/usr/libexec/${specdir}
    else
        local specloc=${II_RPM_PREFIX}/${specdir}
    fi
    local specsrc=${II_SYSTEM}/${specdir}
    if [ "$pkg" = "core" ] ; then
	local specname=${rpm_basename}-${rpm_version}-${rpm_release}
    else
	local specname=${rpm_basename}-${rpm_version}-${pkg}-${rpm_release}
    fi
    local bldroot=${II_RPM_BUILDROOT}/${specname}
    local base=${specname}
    local log=$II_RPM_BUILDROOT/${base}.log
    
    specfile=${bldroot}/${specloc}/${specname}.spec
    specsrc=${specsrc}/${specname}.spec

    # Copy spec file into place for non-LSB builds
    if ! $LSB_BUILD && [ "$pkg" != "LICENSE" ] && [ "$pkg" != "documentation" ]
    then
        [ -d "${bldroot}/${specloc}" ] || mkdir -p "${bldroot}/${specloc}"
        [ "$pkg" = "core" ] && mkdir -p "${bldroot}/var/lib/ingres"
        [ "$pkg" != "LICENSE" ] && cp $specsrc $specfile
    fi

    # Build package
    echo "packaging $base ..."
    ${RPMBLD} --buildroot ${bldroot} -bb \
	--define "_rpmdir ${II_RPM_BUILDROOT}" --target \
	${ARCH} ${specsrc} >& ${log}

    rc=$?
    if (( $rc )); then
	error "packaging $base failed, see ${base}.log for details"
    fi
}

gen_srpm_flists()
{
    if $LSB_BUILD ; then
	core=client
    else
	core=core
    fi

    listdir=$ING_BUILD/files/srpmflists
    srpmflag="-S"
    flists="$(echo $@ |sed -e 's/ /,/g')"
    plist=$@
    $unhide && srpmflag="$srpmflag -u"
    # Generate initial lists using buildrel
    echo "Generating file lists"
    buildrel $srpmflag > /tmp/br$$.out 2>&1
    [ $? != 0 ] && cat /tmp/br$$.out
    rm -f /tmp/br$$.out

    # Now the dirlists, starting with core/client
    [ -f $listdir/ingres-${core}.flist ] ||
    {
	echo "Error: cannot locate $listdir/ingres-${core}.flist"
	return 1
    }
    cdirlis=/tmp/ingres-${core}$$.dlist
    skpdirs="%{_bindir}$|%{_var}/lib/ingres/files/name$|%{_sysconfdir}/pam.d$"

    # generate a list of all unique directory names in the file list
    # excluding those we don't want
    echo "Generating directory list for ${core} package "
    grep ^%attr $listdir/ingres-${core}.flist | \
	sed -e 's,\/[^/]*$,,' | \
	sort --key=8 -u | \
	awk '$2 ~ "(.*,ingres,ingres)" \
		{ printf "%%dir %attr(755,ingres,ingres) %s\n", $8 } ;
	     $2 ~ "(.*,root,root)" \
		{ printf "%%dir %attr(755,root,root) %s\n", $8 }' | \
	egrep -v $skpdirs > $cdirlis

    # ...and the rest
    for pkg in $plist
    do
	if [ "$pkg" = "$core" ]
	then
	    continue
	else
	    dlis=/tmp/ingres-${pkg}$$.dlist
	    dtmp=/tmp/dlis.$$
    	    # generate a list of all unique directory names in the file list
    	    # excluding those we don't want
            echo "Generating directory list for ${pkg} package..."
	    [ -f $listdir/ingres-${pkg}.flist ] ||
	    {
		echo "Error: cannot locate $listdir/ingres-${pkg}.flist"
		return 1
	    }
    	    grep ^%attr $listdir/ingres-${pkg}.flist | \
		sed -e 's,\/[^/]*$,,' | \
		sort --key=8 -u | \
		awk '$2 ~ "(.*,ingres,ingres)" \
			{ printf "%%dir %attr(755,ingres,ingres) %s\n", $8 } ;
	     	     $2 ~ "(.*,root,root)" \
			{ printf "%%dir %attr(755,root,root) %s\n", $8 }' | \
		egrep -v $skpdirs > $dtmp
	    # remove directories that are already covered by the core package
	    cat $cdirlis $dtmp | \
		sort --key=2 | uniq -d | \
		cat $dtmp - | \
		sort | uniq -u > $dlis
	    rm  -f $dtmp
	fi
    done

    # finally check for duplicates
    echo "Removing duplicates..."
    dups=$(eval cat /tmp/ingres-{$flists}$$.dlist | \
		sort --key=2 | uniq -d | awk '{print $2}')
    if [ -n "$dups" ]
    then
	for d in $dups
	do
	    # remove from rest
	    sed -i -e "\,${d}$,d" $(eval echo /tmp/ingres-{$flists}$$.dlist)
	    # add to core
	    echo "%{dir} $d" >> $cdirlis
	done

	# double check they're all gone
	dups=$(eval cat /tmp/ingres-{$flists}$$.dlist | \
		sort --key=2 | uniq -d | awk '{print $2}')
 	[ -z "$dups" ] || 
	{
	    cat << EOF
ERROR: Duplicate directories detected in file lists:

$dups

EOF
	    return 1
	}
    fi

    # Add the dir lists to the flists
    for pkg in $plist
    do
	cat /tmp/ingres-${pkg}$$.dlist >> ${listdir}/ingres-${pkg}.flist &&
	    rm /tmp/ingres-${pkg}$$.dlist
    done
}
#
#-[ Main - main body of script ]----------------------------------------------
#

# Announce startup
cat << EOF

iirpmbuild

EOF

# Get valid package list from manifest
[ -z "$II_MANIFEST_DIR" ] &&
{
    error "II_MANIFEST_DIR is not set"
    clean_exit 1
}

unhide=false
spatial=false
extract=true
build=true
flist=false
exdir=""

# parse command line

export unhide
export spatial
export rpmconfig="${II_MANIFEST_DIR}/rpmconfig"

# If no arguments are given build everything
pkglst=`get_pkg_list`
[ -z "$1" ] && bldlst="${pkglst}"

# Parse command line
while [ "$1" ]
do
  case "$1" in
    -unhide) 
	unhide=true
	pkglst=`get_pkg_list`
	shift
	;;
   -s*) spatial=true
	pkglst=`get_pkg_list`
	shift
	;;
   -gen-flists)
	build=false
	extract=false
	flist=true
	shift
	;;
   -extract-only)
	build=false
	extract=true
	shift
	;;
   -use-dir)
	[ -d "$2" -a -w "$2" ] ||
	{
	    echo "$2 does not exist or is writable"
	    usage
	}
	exdir=$2
	shift ; shift
	;;
     *)
	match=false
	for pkg in $pkglst
	do 
	    # Check args are valid packages
	    [ "$1" = "$pkg" ] &&
	    {
	    if [ "${bldlst}" ] ; then 
		    bldlst="${bldlst} $1"
	    else
		    bldlst="$1"
	    fi
	    # break out when we find a match
	    match=true
	    break
	    }

        done
	
        # if we found a match check the next arg
        ${match} && shift && continue

        # otherwise print usage message and exit
        error "$1 is not a valid package"
        usage
	;;
  esac # parsing arguments.

done

[ -z "$bldlst" ] && bldlst="${pkglst}"

export pkglst
# Check build environment
umask 022
envInit || clean_exit 3

# Build packages
for p in $bldlst
do
	if $extract  ; then
	    unpack_archives $p ||
            {
	        echo "ERROR: Unpacking archives for $p failed with $?"
	        clean_exit 4
            }
	fi
	
	if $build ; then
	    echo "Building $p RPM..."
	    build_rpm $p ||
            {
	        echo "ERROR: Building the $p RPM,  failed with $?"
	        clean_exit 5
            }
	fi

done

if $flist ; then
    gen_srpm_flists $pkglst ||
    {
	echo "ERROR: Failed to generate SRPM file lists"
  	clean_exit 6
    }
fi

clean_exit
