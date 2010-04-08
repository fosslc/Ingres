:
#
# Copyright (c) 2004 Ingres Corporation
#
#	Remove all files from $ING_BUILD/abf owned by testenv.
#
# History:
#	26-sep-1989 (boba)
#		Created based on clnqaabf
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

echo "Removing all files from $ING_BUILD/abf owned by testenv ..."
find $ING_BUILD/abf -user testenv -type d -print > /tmp/cln$$
for d in `cat /tmp/cln$$`
do
	if [ -d $d ]
	then
		echo Removing $d
		rm -rf $d
	fi
done
rm -f /tmp/cln$$
