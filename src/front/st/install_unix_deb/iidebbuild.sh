:
## Copyright (c) 2007 Ingres Corporation 
## Name:
##	iidebbuild.sh
##
## Usage:
##	iidebbuild
##
## Description:
##
##	Builds DEB packages from existing Ingres build area.
##
## Depends on:
##
##  II_DEB_BUILDROOT = directory to unpack install image, destination 
##		      for built RPMs
##  II_RELEASE_DIR   = buildrel output directory (to locate install.pgz)
##
## History
##
##	28-Jun-2007 (hanje04)
##	    Created from iirpmbuild.sh.
##	22-Jun-2009 (kschendel) SIR 122138
##	    Readvers figures out primary vers now.
##

self=`basename $0`
ARSFX=pgz

# Check VERS file
. readvers

vers=$config

# Set deb architecture
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

if [ -x /usr/bin/dpkg-deb ] ; then
    DEBBLD=dpkg-deb
else
    cat << EOF
$self

Cannot locate dpkg-deb utility, need to build DEB packages. To install
run:
	apt-get install dpkg

EOF
    exit 1	
fi

shopt -s nullglob

if [ -x /usr/bin/dpkg-scanpackages ] ; then
    DEBSCAN=dpkg-scanpackages
else
    cat << EOF
$self

Cannot locate dpkg-scanpackage utility, need to build DEB packages. To install
run:
	apt-get install dpkg-devel

EOF
    exit 1	
fi

usage()
{
    args="[-unhide"
    for a in $pkglst
    do
        args="${args}] [${a}"
    done

    args="${args}]"
	
    echo "Usage:"
    cat << EOF 
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

    [ $rc -ne 0 ] && error "An error occured creating this DEB"
    # Reset any traps and exit
    trap - 0
    exit $rc
}
# clean_exit()

#
#-[ get_pkg_list() ] - Get Valid deb package list from manifest
#
get_pkg_list()
{
    [ -r "${debconfig}" ] ||
    {
	error "Cannot locate ${debconfig}"
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

    # Scan debconfig for package
    grep '^$spec' "${debconfig}" | awk '
	$2 == "" { printf "core\n" ; }
	$2 != "" { printf "%s\n",$2 ; }' | \
	egrep -v ${pkgmask}
}
# get_pkg_list()

envInit()
{
# check environment for proper variables
    rc=0
    if [[ "$II_DEB_BUILDROOT" == "" ]];  then
	error "Set II_DEB_BUILDROOT to location for DEB packages build area"
	(( rc++ ))
    fi
    
    if [[ "$II_RPM_PREFIX" == "" ]]; then
	error "Set II_RPM_PREFIX so that it matches the spec file prefix"
        (( rc++ ))
    fi

    if [[ "$II_RELEASE_DIR" == "" || ! -d "$II_RELEASE_DIR"  ]]; then
	error "Set II_RELEASE_DIR to a valid directory containing buildrel output"
	(( rc++ ))
    fi

# get version info from debconfig
    deb_basename=`grep '^$sub.*rpm_basename' ${debconfig} | awk '{print $3}'`
    deb_version=`grep '^$sub.*rpm_version' ${debconfig} | awk '{print $3}'`
    deb_doc_prefix=`grep '^$sub.*rpm_doc_prefix' ${debconfig} | awk '{print $3}'`
    export deb_basename deb_version deb_doc_prefix
    return $rc
}

#
#-[ unpack_archives() - Unpack archives to be packaged in RPM ]
#
unpack_archives()
{
    [ "$1" ] || return 1

    if [ "$1" = "core" ] ; then
	local deb=""
    elif [ "$1" = "LICENSE" ] ; then
	# no archive for LICENSE RPM
	return 0
    else
	local deb="$1"
    fi


    # check build area exists (created by buildrel -d)
    [ -d "$II_DEB_BUILDROOT/${deb_basename}-${deb_version}/$1" ] ||
    {
	error "Build area for $1 package doesn't exist:

	$II_DEB_BUILDROOT/${deb_basename}-${deb_version}/$1

Has \"buildrel -d\" been run?"
	return 1
    }

    # create the rest of the tree
    if [ "$deb" == "documentation" ] ; then
	builddir=$II_DEB_BUILDROOT/${deb_basename}-${deb_version}/$1/${deb_doc_prefix}/${deb_basename}-${deb_version}
    else
	builddir=$II_DEB_BUILDROOT/${deb_basename}-${deb_version}/$1/${II_RPM_PREFIX}
    fi

    mkdir -p ${builddir}

    # get list of archives for DEB from debconfig using awk.
    # 	- scan through debconfig until we find the "$spec" value
    # 	  we're looking for.
    #   - When we do, turn on printing and print out all the "$features"
    #	  associated with the "$spec"
    #   - When we find another "$spec" turn of printing and scan through to
    #	  the end.
    archlist=`awk -v spec=$deb 'BEGIN{
	prnt=0 ;
	}
	prnt==0 && $1 == "$spec" && $2 == spec { prnt=1 ; }
	$1 == "$feature" && prnt == 1 { printf "%s\n", $2 ;} 
	prnt==1 && $1 == "$spec" && $2 != spec { prnt=0; } ' \
	"${debconfig}"`

    rc=$?

    # Check we have somesort of list
    [ -z "${archlist}" ] || [ $rc != 0 ] && 
    {
	error "Failed to create archive list"
	return 1
    }

    # Now unpack the archives
    cd ${builddir}

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
	pax -pp -rzf ${II_RELEASE_DIR}/${arch}/install.${ARSFX} ||
	{
	    error "Failed to unpack $arch"
	    return 3
	}
    done

    return 0

}

generate_md5sums()
{
    [ "$1" ] || return 1

    # check file list exists (created by buildrel -d)
    debloc="$II_DEB_BUILDROOT/${deb_basename}-${deb_version}/$1"
    cntrlfile="${debloc}/DEBIAN/control"
    filelist="${debloc}/DEBIAN/filelist"
    md5sums="${debloc}/DEBIAN/md5sums"
    [ -f "${cntrlfile}" ] ||
    {
	error "Control file for $1 package doesn't exist:

	${cntrlfile}

Has \"buildrel -d\" been run?"
	return 1
    }

    [ -f "${filelist}" ] ||
    {
	error "File list file for $1 package doesn't exist:

	${filelist}

Has \"buildrel -d\" been run?"
	return 1
    }

    cd ${debloc}

    # remove old file if needed
    if [ -f ${md5sums} ] ; then
	rm -f ${md5sums} || return 1
    fi

    # generate checksums
    for f in `cat ${filelist} | sed -e 's,^/,,'`
    do
	md5sum $f >> ${md5sums} || return 1
    done

    # check file was created
    if [ ! -f ${md5sums} ] ; then
	error "Failed to create md5sum file for $deb package"
	return 1
    fi

    return 0

}

build_deb()
{
    [ -z "$1" ] &&
    {
	error "No package specified for building"
	return 1
    }

    # check file list exists (created by buildrel -d)
    pkg=$1
    if [ "${pkg}" = "core" ] ; then
	deb=${deb_basename}-${deb_version}
    else
	deb=${deb_basename}-${pkg}-${deb_version}
    fi
    bldloc="$II_DEB_BUILDROOT/${deb_basename}-${deb_version}"
    debloc="$II_DEB_BUILDROOT/${ARCH}"
    cntrlfile="${bldloc}/${pkg}/DEBIAN/control"
    md5sums="${bldloc}/${pkg}/DEBIAN/md5sums"
    [ -f "${cntrlfile}" ] ||
    {
	error "Control file for $1 package doesn't exist:

	${cntrlfile}

Has \"buildrel -d\" been run?"
	return 1
    }

    [ -f "${md5sums}" ] ||
    {
	error "md5sum file for $1 package doesn't exist:

	${md5sums}
"
	return 1
    }

    [ -d ${debloc} ] || mkdir -p ${debloc}
    rc=$?

    [ ${rc} != 0 ] &&
    {
	error "Failed to create output directory:

	${debloc}
"
	return 1
    }

    cd ${bldloc}

    # fix dir permissions
    find . -type d -exec chmod 755 {} \;

    echo "packaging ${pkg}"
    ${DEBBLD} --build ${pkg} > \
	${II_DEB_BUILDROOT}/${deb}.log 2>&1
    rc=$?
    
    if (( $rc )); then
	error "packaging ${pkg} failed, see ${pkg}.log for details"
    fi

    # Package build sucessfull, copy to final location
    echo "Moving to ${debloc}/${deb}.${ARCH}.deb..."
    mv ${pkg}.deb ${debloc}/${deb}-${ARCH}.deb

    rc=$?

    return $rc
}

create_pkg_file()
{
    debloc="$II_DEB_BUILDROOT/${ARCH}"

    [ ! -d ${debloc} ] &&
    {
	error "Cannot locate DEB directory: ${debloc}"
	return 1
    }

    cd ${debloc}
    echo "Creating DEB packages file: Packages.gz"
    ( ${DEBSCAN} ./ /dev/null | gzip - > Packages.gz ) ||
    {
	error "An error occurred whilst creating DEB packages file"
	return 1
    }

    return 0
}

#
#-[ Main - main body of script ]----------------------------------------------
#

# Announce startup
cat << EOF

${self}

EOF

# Get valid package list from manifest
[ -z "$II_MANIFEST_DIR" ] &&
{
    error "II_MANIFEST_DIR is not set"
    clean_exit 1
}

unhide=false
spatial=false

# parse command line

export unhide
export spatial
export debconfig="${II_MANIFEST_DIR}/debconfig"

# If no arguments are given build everything
pkglst=`get_pkg_list`
[ -z "$1" ] && bldlst="${pkglst}"

# Parse command line
while [ "$1" ]
do
  case "$1" in
    -u) 
	unhide=true
	pkglst=`get_pkg_list`
	shift
	;;
   -s*) spatial=true
	pkglst=spatial
	bldlst=spatial
	[ -d "$SOL_RELEASE_DIR" ] ||
	{
	    error "SOL_RELEASE_DIR is not set or doesn't not exist"
	    clean_exit 6
	}
	export II_RELEASE_DIR=$SOL_RELEASE_DIR
	shift
	# only one spatial package so we're done
	break
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

export pkglst
# Check build environment
envInit || clean_exit 3

# Build packages
for p in $bldlst
do
	echo "Building $p DEB..."
	unpack_archives $p || clean_exit 4
	generate_md5sums $p || clean_exit 5
	build_deb $p || clean_exit 6
done

# Create deb packages repository file
create_pkg_file || clean_exit 7

clean_exit
