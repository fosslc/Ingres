:
#
# Copyright (c) 2004 Ingres Corporation
#
# Clean up databases left by SEP tests
#
# History:
#	26-sep-1989 (boba)
#	    Created based on clnqadbs.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

# check the args
while [ $# != 0 ]
do
	case $1 in
		-a)	aflag=true
			;;
		*)	echo '	Usage:  clntstdbs [-a]'
			exit 1
			;;
	esac
	shift
done

db=iidbdb
dbcat=iidatabase
name="name                    "
delim=""

if [ "$aflag" = "true" ]
then
	echo 'Removing ALL testenv databases.'
	ingres -s $db << EOS
range of d is $dbcat \g \script /tmp/tstdbs
retrieve (d.name) where d.own="testenv" \g \q
EOS
else
	echo 'Removing databases with names of the form "[a-z][a-z][a-z][0-9][0-9]*"'
	ingres -s $db << EOS
range of d is $dbcat \g \script /tmp/tstdbs
retrieve (d.name) where d.name="[a-z][a-z][a-z][0-9][0-9]*" \g \q
EOS
fi

awk -F'|' '{print $2}' /tmp/tstdbs | (
while read dbname
do
	if [ "$dbname" != "" -a "$dbname" !=  "$name" -a "$dbname" != "$delim" ]
	then
		echo DB: $dbname
		destroydb -s $dbname
	fi
done)
rm /tmp/tstdbs
