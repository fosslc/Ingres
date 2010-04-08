:
#
# Copyright (c) 2004 Ingres Corporation
#
# ensure -- ensure that I am running as one of $*
#
## History:
##	25-apr-1986 (seiwald)
##		Written.
##	15-jan-1990 (boba)
##		Allow ensure to accept multiple names.
##		Also remove old rcs log & add history.
##	1-nov-1990 (jonb)
##		Use iisysdep to set user id.
##	20-Dec-94 (GordonW)
##		full path of iisysdsep
##	25-Apr-2000 (somsa01)
##		Added capability for other product builds.
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##		Added capability for EDBC builds.
##	14-Jul-2004 (hanje04)
##		Use whoami not iisysdep (!) to set WHOAMI
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##

WHOAMI=`whoami`

for user in $*
do
	[ $WHOAMI = $user ] && exit 0
done

echo "You must run this program as `echo $* | sed \"s/ / or /g\"`." >&2
exit 1
