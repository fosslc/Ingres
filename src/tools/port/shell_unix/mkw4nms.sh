:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkw4nms
#
# 
# Make a set of files in /tmp (or $TMPDIR) that contains the
# nm(1) output of all the libraries in $ING_BUILD/lib. This is
# handy for quick troubleshooting of "what routine is in what library?"
# sorts of questions.

# (Why put this in a shell script? Well, it's used a lot in ports, and
# if you screw up the commands you can bash your $ING_BUILD/lib!)

#   History:
#		22-mar-93 (jab)
#           Created.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
#
if [ -z "$ING_BUILD" ]
then
	echo \$ING_BUILD must be set
	exit 1
fi
dir=${TMPDIR:-/tmp}/nms
if  [ ! -d $dir ]
then
	mkdir -p $dir
	if [ $? -ne 0 ] 
	then
		echo Cannot create $dir
		exit 1
	fi
fi

cd $ING_BUILD/lib
for lib in *.a
do
	echo "nm $lib > $dir/$lib"
	nm $lib > $dir/$lib
done
