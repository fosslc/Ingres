:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

# grepall - "egrep" for $1 in all source files in all subdirectories

if [ -z "$1" ]
then
	echo usage: $0 pattern [dir]
	exit 1
fi

if [ -n "$2" ]
then
	(
		if cd $2
		then
			echo $2:
			files="`echo *.*[chys]`"
			if [ "$files" != "*.*[chys]" ]
			then
				egrep "$1" $files
			fi
		else
			echo "\t...can't change to directory $2"
		fi
	)
	for d in $2/*
	do
		if [ -d $d ]
		then
			if [ -r $d ]
			then
				$0 "$1" $d
			else
				echo "\t...directory $d unreadable"
			fi
		fi
	done
else
	for d in *
	do
		if [ -d $d ]
		then
			if [ -r $d ]
			then
				$0 "$1" $d
			else
				echo "\t...directory $d unreadable"
			fi
		fi
	done
fi
