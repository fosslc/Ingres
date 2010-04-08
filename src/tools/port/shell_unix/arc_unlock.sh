:
#
# Copyright (c) 2004 Ingres Corporation
#
#	Unlock an archive.
#
#	This file releases a lock obtained by a previous call
#	to arc_lock.
#
#	23-mar-89 (russ)
#		Created.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

#
# check number of args
#
if [ $# -ne 1 ]
then
	echo usage: $0 library.a
	exit 1
fi

#
# generate name for lockfile
#

base=`echo $1 | sed 's/\(.*\)\.a$/\1/'`
lock_file=${base}.L

#
# remove lock
#

rm -f $lock_file

exit 0
