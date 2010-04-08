:
#
# Copyright (c) 2004 Ingres Corporation
#
# whoami -- print effective user name
# mimics the 4.2BSD command
# This is for BSD:
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

[ -f /usr/ucb/whoami ] && exec /usr/ucb/whoami

# use /usr/bin/whoami for Bull, following code not work under
# Korn shell. -- DPX/2

[ -f /usr/bin/whoami ] && exec /usr/bin/whoami

# else SYS5
if [ -f /bin/id -o -f /usr/bin/id ]
then
	IFS="()"
	set - `id`
	echo $2
else
	touch /tmp/who$$
	set - `ls -l /tmp/who$$`
	echo $3
	rm /tmp/who$$
fi
