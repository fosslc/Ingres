:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
# $Header: /cmlib1/ingres63p.lib/unix/tools/port/shell/overwrite.sh,v 1.1 90/03/09 09:18:33 source Exp $

# overwrite: copy standard input to output after EOF;
# from K&P "Unix Programming Environment", p 154.

opath=$PATH
PATH=/bin:/usr/bin

case $# in
0|1)	echo 'Usage: overwrite file cmd [args]' 1>&2; exit 2
esac

file=$1; shift
new=/tmp/overwr1.$$; old=/tmp/overwr2.$$
trap 'rm -f $new $old; exit 1' 1 2 15		# clean up files

if PATH=$opath "$@" >$new			# collect input
then
	cp $file $old		# save original file
	trap '' 1 2 15		# we are committed; ignore signals
	cp $new $file
else
	echo "overwrite: $1 failed, $file unchanged" 1>&2
	exit 1
fi
rm -f $new $old
