:
#
# Copyright (c) 2004 Ingres Corporation
#
# Stop an Achilles session
#
# arg1 = pid
#
# History
# =======
#	10-Aug-93 (GordonW)
#		written.
#	12-Aug-93 (GordonW)
#		Added parse for username
#	16-Aug-93 (GordonW)
#		Check against a bogus pid.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
. $II_SYSTEM/ingres/utility/iisysdep
[ $SHELL_DEBUG ] && set -x

pid="$*"

if [ $# -ne 1 ]
then
	echo "usage: acstop pid..."
	exit 1
fi

# parse username for kill
entry=`$PSCMD | grep achilles | grep -v grep | grep $pid`
[ "$entry" = "" ] &&
{
	echo "acstop: Achilles $pid not found"
	exit 1
}

# loop until we can't do it
loop=0
while [ $loop -lt 5 ]
do
	entry=`$PSCMD | grep achilles | grep -v grep | grep $pid`
	[ "$entry" = "" ] && break
	username=`echo $entry | awk '{print $1}'`
	[ "$username" = "" ] &&
	{
		echo "acstop: Grabled ps command"
		exit 1
	}
	if qasetuser $username kill -TERM $pid | grep 'Not owner' >/dev/null 2>&1
	then
		if [ "$WHOAMI" = "root" ]
		then
			kill -TERM $pid >/dev/null 2>&1
		else
			echo "acstop: Need to run as root"
			loop=6
			break
		fi
	fi
	sleep 5
	loop=`expr $loop + 1`
done
[ $loop -lt 5 ] && msg="stopped" || msg="not stopped"
echo "acstop: Achilles $pid $msg"
exit 0
