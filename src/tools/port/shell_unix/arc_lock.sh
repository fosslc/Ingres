:
#
# Copyright (c) 2004 Ingres Corporation
#
#	Lock an archive.
#
#	This file checks and waits on a lockfile associated with
#	an archive file.  When the lock is released, this process
#	acquires the lock and exits.
#
#	23-mar-89 (russ)
#		Created.
#	4-oct-89 (daveb)
#		Add 10 minute timeout.
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
# set up lock message
#

lock_msg="archive $1 locked by `whoami`"

#
# generate name for lockfile
#

base=`echo $1 | sed 's/\(.*\)\.a$/\1/'`
lock_file=${base}.L

#
# wait for lock to become free
#

if [ -f $lock_file ]
then
	cat $lock_file
	ls -l $lock_file

	# give it 10 minutes, at 5 secs/try = 12 tries/minute.
	tries=120
	while [ -f $lock_file -a $tries -gt 0 ]
	do
		sleep 5
		echo trying...
	    tries=`expr $tries - 1`
	done
	if [ -f $lock_file ]
	then
		echo "Timeout, blasting presumably orphaned lock"
		rm -f $lock_file
	fi
fi

#
# lock file
#

echo $lock_msg > $lock_file

exit 0
