:
#
# Copyright (c) 2004 Ingres Corporation
#
#	nroff|more UNIX tools or INGRES executables man page if it exists
#	else call system man
#
# History:
#	31-jul-1989 (boba)
#		Update from 5.0 to 6.1.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
for pages in $*
do
	for page in $ING_BUILD/man*/$pages.* $ING_SRC/man*/$pages.* \
		/usr/local/man/man*/$pages.* /usr/man/man*/$pages.*
	do
		if [ -f $page ]
		then
			if whatunix | fgrep 'BSD' > /dev/null
			then
				nroff -man $page | cat -s | more
			else
				nroff -man $page | more
			fi
		fi
	done
done
