:
## 
## Copyright (c) 2003, 2010 Ingres Corporation. All rights reserved. 
## 
## Name:
##	iirpmbuild.sh
##
## Usage: 
##	iirpmrename <rpm(s)> <base name> <install id>
##
##	<rpm>		One or more RPMs to be renamed and repackaged
##
##	<install id>	Unique identifier for the RPM name, normally 
##			installation ID.
##
## Description:
##
## History
##
##	24-oct-2003 (fanch01)
##	    Created.
##	27-Oct-2003 (hanje04)
##	    Added header and comments etc.
##	12-Jan-2004 (hanje04)
##	    Reduce command line params to 2 and 'awk' the rest of the info
##	    we need from it.
##	    Removed <spec match> as spec files will always have suffix .spec
##	    Removed <base name as we can get it from the RPM file name we've 
##	    been asked to repackage.
##	13-Jan-2004 (hanje04)
##	    When altering the SPEC file to update the package name we must also
##	    update the names of the dependancies and the Prefix
##	29-Jan-2004 (hanje04)
##	    Sign rpm packages
## 	02-Feb-2004 (hanje04)
##	    Moved --sign in rpmbuild to before --define as it was being taken
##	    as a define.
##	    Added -II to default to RPMPREFIX and split RPMPREFIX up into 
##	    RPMBASEPRFX and RPMPRFXID to make the string easier to replace.
##	24-Feb-2004 (hanje04)
##	    Echo usage message and not "wrong" when file location is incorrect.
##	    Determine RPMARCH from HOSTTYPE and not RPM. Renamed RPM's arch.
##	    will depend on this and NOT the source RPM's arch.
##	01-Mar-2004 (hanje04)
##	    Add checks for previously renamed packages and documentation 
##	    packages as neither of these can be renamed.
##	06-Mar-2004 (hanje04)
##	    Add RPMBLD variable to denote build command so that script will 
##	    work against rpm 3.0.6 as well as 4.1. Also remove --nosignature 
##	    flag as it is not supported by rpm 3.0.6.
##	18-Mar-2004 (hanje04)
## 	    Hard code RPMARCH to i386 for renaming purposes and force RPM to
##	    create the new RPMS as i386. HOSTTYPE doesn't always determine
##	    the architecture of the new RPMS.
##	21-Mar-2004 (hanje04)
##	    RPM package names now start with ingres2006. Changed BASENAME
##	    accordingly.
##	22-Mar-2004 (hanje04)
##	    Correct spelling mistake in "embeded".
##	27-Mar-2004 (hanje04)
##	    Changed default installation directory to /opt/CA/AdvantageIngresII
##	31-Mar-2004 (hanje04)
##	    Fix sedding of package dependencies so that EULA rpm is not change. 
##	04-Apr-2004 (hanje04)
##	    Ammend checking for doc package to work with new name.
##	15-Apr-2004 (hanje04)
##	    Look for PreReq and not Requires when replacing dependencies.
##	20-May-2004 (hanje04)
##	    Add check for EI packages, embedded version cannot be renamed.
##	22-Jul-2004 (hanje04)
##	    Change RPMBASEPRFX to /opt/CA/Ingres
##	30-Jul-2004 (hanje04)
##	    Replace EULA with CATOSL.
##	22-Nov-2004 (hanje04)
##	    Check /etc/profile is OK before running. If it's not rpmbuild will
##	    fail.
##	13-Dec-2004 (hanje04)
##	   Use -f when removing files
##	04-Feb-2004 (kodse01)
##	    Add support for other Linux platforms.
##	24-Feb-2005 (hanje04)
##	   Honor TMPDIR variable for temporary build location and check we
##	   have sufficient disk space before attempting the rebuild.
##	02-Mar-2005 (hanje04)
##	   Don't specify _tmpdir in rpmbuild line as it causes problems on 
##	   some linuxes.
##	10-Mar-2005 (hanje04)
##	   SIR 114034
##	   Fix BASENAME to handle package name after ingres2006- to start
##	   with numeric. e.g ingres2006-32bit
##	06-Apr-2005 (hanje04)
##	   Force posix output from df to make sure we get the info we need 
##	05-Jul-2005 (hanje04)
##	   Make sure we replace ALL instances of ${RPMPREFIX}.
##	30-Aug-2005 (sheco02)
##	   Request more for estimated disk space for NEEDFREE.
##	05-Jan-2005 (hanje04)
##	   SIR 115662
##	   Update for Ingres2006 rebranding/packaging.
##	   Also remove requirement to specify complete path and 
##	   allow multiple packages to be specified.
##	   Get architecture of target RPM from source RPM.
##	02-Feb-2006 (hanje04)
##	   BUG 115696
##	   Save license dependencies from source RPM so that they can
##	   be correctly preserved in the target.
##	   Also tokenize ingres2006.
##	03-Feb-2006 (hanje04)
##	   SIR 115688
##	   Fix up to work with eval license
##	07-May-2006 (hanje04)
##	   Bump version to ingres2007
##	29-Nov-2006 (hanje04)
##	   Add -q flag to suppress echoing of program name.
##	22-Jan-2007 (bonro01)
##	   Change version back for ingres2006 release 3
##      19-Mar-2007 (hanal04) Bug 114443
##         Later versions of rpm do not support the --buildroot flag. If
##         rpm does not support --buildroot and rpmbuild is not available
##         we must exit cleanly.
##	01-Oct-20-7 (hanje04)
##	   BUG 119218
##	   Make sure obsolete packages and dependencies are renamed correctly.
##	10-Oct-2007 (hanje04)
##	   Bug 119306
##	   Remove the --sign option from the rebuild command as it causes
##	   problems with some Linux which are setup to use PGP. The packages
##	   cannot be resigned on any machine other than that one which they
##	   were created anyway so it's fairly redundant anyway.
##	19-Nov-2007 (hanje04)
##	   BUG 119218
##	   Make sure 32bit package gets renamed properly too.
##	11-Aug-Jul-2008 (hweho01)
##	   Changed package name from ingres2006 to ingres for 9.2 release,
##	   use (PROD_PKGNAME) which will be substituted by jam during build.
##	13-Aug-2009 (hanje04) Bug 122588
##	   Add debug info and flag (-debug) to aid debugging. Use rsynch to
##	   create new build area rather than link which can cause RPMBUILD
##	   issues.
##	   Also remove %install tags from spec files as they can cause
##	   unwanted directory removal with later versions of RPM.
##	16-Feb-2010 (hweho01) Bug 122588
##	   Only generate the file list ( .lis ) in rpmListContents() if
##	   the debug flag is set.
##	14-Feb-2010 (hanje04)
##	   Replace 'Requires' tags too. PreReq is obsolete
##	   
##

self=`basename $0`
quiet=false
debug=false
ARCH=`uname -m`
PATH=/bin:/usr/bin
NOCLEAN=0
ATEXIT=
WORKDIR=
RPMFILES=
RPMSPEC=
rc=0
RPMBASEPRFX=/opt/Ingres/Ingres
RPMPRFXID=II
RPMPREFIX=${RPMBASEPRFX}${RPMPRFXID}
OUTDIR=`pwd`
SPECSFX=spec
PRODNAME=(PROD_PKGNAME)
[ "$TMPDIR" ] || TMPDIR=/tmp

if [ -x /usr/bin/rpmbuild ] ; then
    RPMBLD=rpmbuild
else
    bldroot=`rpm --help | grep -- --buildroot`
    if [ "$bldroot" = "" ] ; then
        cat << !

/usr/bin/rpmbuild not found and the installed version of rpm does not 
support the --buildroot flag. Please install rpm-build or a version of
rpm that supports the --buildroot flag.

!
        exit 1
    else
        RPMBLD=rpm
    fi

fi

# Check /etc/profile is intact
. /etc/profile || 
{
    cat << !
Error:
	/etc/profile exited with a non-zero return code 
	This problem must be resolved before $self can be continue

!
    exit 1
}

#
#-[ usage() - Display usage message ]-
#
usage()
{
cat << !
usage:
    $self rpm_packages(s) installation_id
    where 
	rpm_package(s)    = One or more RPM packages to be renamed which can
			    NOT have been previously renamed
	installation_id   = installation identifier to be embedded into
			    rpm_package

!
}

#
#-[cleanPush() - Push a command into the cleanup stack ]-
#
cleanPush()
{
    ATEXIT="$1 $ATEXIT"
}

#
#-[cleanAll() - Cleanup any temporary files ]-
#
cleanAll()
{
# back out any changes made
    if ((! $NOCLEAN)); then
	cd $OLDPWD
	for command in $ATEXIT; do
	    ${command};
	done
    fi
 
# Reset cleanup functions
    export ATEXIT=""
    return 0
}

#
#-[argsInit() - Parse and check command line arguments ]-
#
argsInit()
{
# check for proper number of command line arguments
    [ $# -lt 2 ] && {
	usage
	return 1
    }

    while [ "$1" ]
    do
	case "$1" in
	    [a-zA-Z][a-zA-Z0-9])
		# Check for valid installation ID
		instid=$1
		shift
		;;
	    -q)
		# quiet mode
		export quiet=true
		shift
		;;
	    -debug)
		# debug info
		export debug=true
		shift
		;;
	    *)
		# Check all other arguments are valid RPMS
		local rpm_basename=`echo $1 | \
		    sed -e"s%.*\(${PRODNAME}[^.]*\)-[0-9][^/]*\.rpm$%\1%"`

		# Has it been renamed already?
		[ "`echo ${rpm_basename} | grep [[:upper:]]`" ] &&
		{
		cat << !
$1 has already been renamed.

!
		    usage
		    return 1
		}

	        # Can it be renamed at all?
		[ "`echo ${rpm_basename} | cut -d- -f2`" = "documentation" ] || \
		[ "`echo ${rpm_basename} | cut -d- -f2`" = "license" ] || \
		[ "`echo ${rpm_basename} | cut -d- -f2`" = "EI" ] &&
		{
		    cat << !
$1 cannot be renamed.

!
		    usage
		    return 1
		}
		
		# do we need to append the path?
		if ( echo $1 | grep -q ^/ )
		then
		    file=$1
		else
		    file=${PWD}/$1
		fi
		# It's ok so add it to the list
		if [ "$rpmlist" ] ; then
		    rpmlist="$rpmlist ${file}"
	 	else
		    rpmlist=${file}
		fi
		shift
		;;
	esac # Argument checking
    done

	# Do we have valid a installation ID 
	[ "$instid" ] ||
	{
	    cat << !
The installation code is either missing or invalid.
The first character code must be a letter; the second may be a letter or
a number.

!
	    usage
	    return 1
	}

    return 0
}

#
#-[confOutDir() - setup temporary working directory ]-
#
confOutDir()
{
    [ "$1" ] || return 1
    local pkg=$1
    count=0
    while [[ -e ${TMPDIR}/${pkg}.${$}.${count} ]]; do
	(( count++ ))
    done
    export WORKDIR=${TMPDIR}/${pkg}.${$}.${count}
    export BUILDDIR=${WORKDIR}/build
    mkdir -p "${WORKDIR}" "${BUILDDIR}"
    rc=$?
    if (($rc)); then
	echo "creating workdir ($WORKDIR) failed ($rc)"
	return 1
    fi
    cleanPush argsInitClean
    
    # change to work directory
    $debug && echo "Working dir: ${WORKDIR}"
    cd "$WORKDIR"
    rc=$?
    if (($rc)); then
	echo "changing to workdir ($WORKDIR) failed ($rc)"
	return 1
    fi
    
    return 0
}
    

#
#-[argsInitClean() - remove temporary working directory ]-
#
argsInitClean()
{
   $debug || rm -rf $WORKDIR
}

#
#-[checkDiskSpace() - check we have enough space under $TMPDIR ]-
#
checkDiskSpace()
{
    if [[ ! -f $1 ]]; then
        usage
        return 1
    fi

    rc=0
    TMPFREE=`df -Pk $TMPDIR | tail -1 | awk '{print $4}'`
    RPMKBYTES=`ls -sk $1 | awk '{print $1}'`
    NEEDFREE=`expr 5 \* ${RPMKBYTES}`

    if [ ${TMPFREE} -lt ${NEEDFREE} ] ; then
	cat << EOF
Error:

${NEEDFREE}Kb of free space is required by $self under $TMPDIR to rename 

	$1.

${TMPFREE}Kb is currently available.

EOF
	rc=1
    fi

    return $rc
    
}

#
#-[ rpmListContents() - Generate file list from source RPM ]-
#
rpmListContents()
{
# stick rpm contents in RPMFILES
    if [[ ! -f $1 ]]; then
	usage
	return 1
    fi
    
    RPMFILES=`rpm -qlp "$1"`
    rc=$?
    if (($rc)); then
	echo "rpm contents failed ($rc)"
	return 1
    fi

    $debug && echo $RPMFILES > $1.lis

    # determine license dependecies of source package so they can
    # be preserved in the target
    licdep=`rpm -qp --requires "$1" | grep ${PRODNAME}-license |\
		cut -d' ' -f1`
    rc=$?
    if (($rc)); then
        echo "Failed to determine license dependencies"
        return 1
    fi

    return 0
}

#
#-[rpmSpecGet() - Get spec file name from source RPM file list ]-
#
rpmSpecGet()
{
# locates a spec file in RPMFILES
    count=0
    for file in $RPMFILES; do
	if [[ $file == *$SPECSFX ]]; then
	    RPMSPEC=$file
	    (( count++ ))
	fi;
    done
    if [[ $count > 1 ]]; then
	echo "too many ($count) spec file matches"
	return 1
    fi
    if [[ $count < 1 ]]; then
	echo "no spec file matches"
	return 1
    fi
    return 0
}

#
#-[ rpmArchiveUnpack() - Unpack source RPM ]-
#
rpmArchiveUnpack()
{
# unpack rpm archive
    $debug && echo "rpm2cpio \"$1\" | cpio -dumi --quiet"
    rpm2cpio "$1" | cpio -dumi --quiet
    rc=$?
    if (($rc)); then
	echo "unpacking failed ($rc)"
	return 1
    fi
# create link to new install dir
    mkdir -p ${BUILDDIR}${RPMBASEPRFX}$2

    rsflags='-a'
    $debug && rsflags+='v'
    rsync ${rsflags} ${WORKDIR}${RPMPREFIX}/ ${BUILDDIR}${RPMBASEPRFX}$2
    return 0
}

#
#-[ rpmSpecExists() - Verify spec file from old RPM ]-
#
rpmSpecExists()
{
# ensure spec file existed in rpm archive
    if [[ ! -f $WORKDIR$RPMSPEC ]]; then
	echo "spec file ($RPMSPEC) missing"
	return 1
    fi
    return 0
}

#
#-[ rpmSpecRename() - Generate new spec file for RPM ]-
#
rpmSpecRename()
{

    [ "${1}" ] || return 1
    local pkg=${1}

# rename the package in the spec
    cp $WORKDIR$RPMSPEC $WORKDIR/spec
    rc=$?
    if (($rc)); then
	echo "could not copy spec file to ($WORKDIR) ($rc)"
	return 1
    fi
    sed -e "s/^Name: ${pkg}/&\-$2/" \
	-e "s/^PreReq: ${PRODNAME}[a-zA-Z_1238,-]*/&-$2/" \
	-e "s/^Requires: ${PRODNAME}[a-zA-Z_1238,-]*/&-$2/" \
	-e "s/^Provides: ${PRODNAME}[a-zA-Z_1238,-]*/&-$2/" \
	-e "s/^Obsoletes: ca-ingres[a-zA-Z_1238,-]*/&-$2/" \
	-e "s/^Obsoletes: ingres2006[a-zA-Z_1238,-]*/&-$2/" \
     -e "s/^PreReq: ${PRODNAME}-license-....\?-[a-zA-Z0-9]*/PreReq: ${licdep}/" \
	-e "s,${RPMPREFIX},${RPMBASEPRFX}${2},g" \
	-e "s/II_INSTALLATION=II/II_INSTALLATION=$2/" \
	-e "s/^%install//g" \
	 $WORKDIR/spec > $WORKDIR$RPMSPEC
    rc=$?
    if (($rc)); then
	echo "could not rename package in the spec ($rc)"
	return 1
    fi
    $debug || rm -f $WORKDIR/spec
    # non-fatal, ignore
    return 0
}

#
#-[ rpmPackage() - Roll up new RPM ]-
#
rpmPackage()
{
    [ "${1}" ] || return 1

    local pkg=${1}
    local bldout=${WORKDIR}/bld.out
# package an rpm archive
    [ -r $WORKDIR$RPMSPEC ] ||
    {
	echo "Missing spec file $WORKDIR$RPMSPEC"
	return 1
    }
    $debug && echo "Build command: $RPMBLD -bb --target $RPMARCH --define \"_rpmdir $WORKDIR \" --buildroot $BUILDDIR $WORKDIR$RPMSPEC "
    $RPMBLD -bb --target $RPMARCH --define "_rpmdir $WORKDIR " --buildroot $BUILDDIR $WORKDIR$RPMSPEC >& ${bldout}
    rc=$?
    if (($rc)); then
	echo "rpm archive packing failed ($rc)"
	[ -r ${bldout} ] && $debug && cat ${bldout}
	return 1
    fi
    rm ${bldout}

    # move new rpm from workdir which will be removed
    mv $WORKDIR/$RPMARCH/${pkg}*.rpm $OUTDIR >& /dev/null
    rc=$?
    if (($rc)); then
	echo "looking for $WORKDIR/$RPMARCH/${pkg}*.rpm"
	echo "retrieving new rpm archive failed ($rc)"
	return 1
    fi
    return 0
}
#
#-[ Main - main body of script ]----------------------------------------------
#

    argsInit "$@" || rc=1

# Startup message
${quiet} ||
    cat << EOF
${self}

EOF

    for rpm in $rpmlist
    do
	# init or previous package failed, bail out
	[ $rc != 0 ] && break
	# get 'basename' and architecture from package name
	rpm_basename=`echo ${rpm} | \
		    sed -e"s%.*\(${PRODNAME}[^.]*\)-[0-9][^/]*\.rpm$%\1%"`
	RPMARCH=`echo ${rpm} | \
		    awk -F . '{print $(NF - 1) }'`

	echo "Renaming $rpm_basename for $RPMARCH to embed $instid"
	if
	    # Setup working directory
	    confOutDir "${rpm_basename}" && \
	    # Check we have enough disk space
	    checkDiskSpace "$rpm" && \
	    # Generate package list
	    rpmListContents "$rpm" && \
	    # find the spec file
	    rpmSpecGet && \
	    # Unpack the rpm
	    rpmArchiveUnpack "$rpm" "$instid" && \
	    # Check we have a spec file
	    rpmSpecExists && \
	    # Edit the spec file for the new package
	    rpmSpecRename "${rpm_basename}" "$instid" && \
	    # Build the new package
	    rpmPackage "${rpm_basename}" ;
	then
	    rc=0
	else
	    rc=1
	fi
	cleanAll

    done

exit $rc
