:
#
# Copyright (c) 2004 Ingres Corporation
#
# uptime for BSD and SV (daveb)
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

[ -f /usr/ucb/uptime ] && exec /usr/ucb/uptime
[ -f /usr/bin/uptime ] && exec /usr/bin/uptime

if [ -f /usr/bin/sar ]
then
    sar -uq 2 1 | awk '
	/^[0-9]/	{ printf "%4s %4s %4s %5s ", $2, $3, $4, $5 }
	/^[ 	]/	{ printf "%8s %8s %8s %8s\n", $1, $2, $3, $4 }
	/Average/	{}'
else
    echo "Sorry, no /usr/ucb/uptime or /usr/bin/sar found"
fi

