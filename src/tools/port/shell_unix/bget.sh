:
#
# Copyright (c) 2004 Ingres Corporation
#
#	Branch a file from one library in the new library.
#	Can only handle one file at a time.
#
#	Can be modified for use in any library.
#	This is originally set up for MANMANX.
#
#	Origin library is :  main
#
#	New library is    :  ndmf-api
#
#	Requires a have list of the original files.
#
#	Have list is      : /devsrc/mainxhp/MMX.have
#
#	14-feb-95 (hanch04)
#		created.
#	20-may-95 (hanch04)
#		disabled bget, must be customized for each branch
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

[ "$SHELL_DEBUG" ] && set -x$-

[ $# -ge 1 ] ||
{
    echo "Usage: $0 filename"
    exit 1
}
pidnum=$$
newlib=
oldlib=
havefile=$ING_SRC/${newlib}.have
file=$1
branchlib=`p here | awk '{print $3}'`
originlib=`echo $branchlib|sed "s/$newlib/$oldlib/"`
version=`grep "$originlib $file" $havefile|awk '{print $3}'`
branch_input=`echo "$branchlib $file branch $originlib $file $version"`

echo $branch_input|p reserve -l -
echo "Branching file for use in $newlib" > /tmp/branch.comment
echo "" > /tmp/branch.buglist
echo $branch_input > /tmp/branch.$pidnum
p sc -b /tmp/branch.buglist -d /tmp/branch.comment -l /tmp/branch.$pidnum
