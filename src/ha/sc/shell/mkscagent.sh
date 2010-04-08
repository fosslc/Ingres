:
#
# Copyright (c) 2004 Ingres Corporation
#
#
# DEST = utility
#
## History
##
##      03-Oct-2005 (bonro01)
##          Created for Solaris High Availiability Cluster Support
##	27-Jan-2006 (bonro01)
##	    Rename for Ingres Corporation
##	13-Aug-2007 (bonro01)
##	    Create installation tar in release directory.

. readvers
. (PROG1PRFX)sysdep
[ $SHELL_DEBUG ] && set -x

prog=`basename $0`
TMPDIR=/tmp/build_IngresSCAgent.$$
CURDIR=`pwd`

while [ ! "$1" = "" ]
do
	tarfile="$1"
        shift

	[ "$1" != "" ]  &&
	{
		echo "Usage: $prog [tarfile]"
		exit 1
	}
done

# use correct vers for Hybrid build
if [ "$VERS64" ]; then
    config=$VERS64
else
    config=$VERS
fi

case $config in
    su9_us5)
        desc="sun-solaris-sparc-32-64bit"
        ;;
    a64_sol)
        desc="sun-solaris-x86-32-64bit"
        ;;
    *)
        echo "Config string $VERS unrecognized."
        exit 1
        ;;
esac

set - `cat $ING_BUILD/version.rel`
release=$2
tardir=IngresSCAgent-$release.$build-$desc
[ -z "$tarfile" ] && {
    if [ ! -d "$ING_ROOT/release" ]; then
        mkdir -p $ING_ROOT/release
    fi
    if [ ! -d "$ING_ROOT/release" ]; then
        echo "Could not create $ING_ROOT/release."
        exit 1
    fi
    tarfile=$ING_ROOT/release/$tardir.tar
}

fname=`basename $tarfile`
fdir=`dirname $tarfile`
[ "$fdir" = "." ] && fdir=`pwd`
tarfile=$fdir/$fname

trap "cd $CURDIR; rm -rf $TMPDIR" 0 1 2 3 15

mkdir -p $TMPDIR/$tardir
cd $ING_BUILD/opt/Ingres/IngresSCAgent
tar cf $TMPDIR/$tardir/SCAgent.tar .
compress $TMPDIR/$tardir/SCAgent.tar

cp -p $ING_BUILD/utility/install_scha $TMPDIR/$tardir/install.sh || exit 1
cd $TMPDIR
tar cf $tarfile $tardir ||
{
	echo "Error creating $tarfile tarfile"
	exit 1
}

echo "$tarfile created successfully"
