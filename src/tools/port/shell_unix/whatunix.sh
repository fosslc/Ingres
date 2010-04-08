:
#
# Copyright (c) 2004 Ingres Corporation
#
# Whatunix.sh
#
# Usage:  (in a makefile):
# 		CFLAGS=-O -D`whatunix`
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

if test -r /bin/universe	# on a pyramid
then
	OPATH=$PATH
	PATH=/bin
	case `universe` in	# universe is dumb, looking only at argv[0]
	att)	echo "SYS5"
		;;

	ucb)	echo "BSD42"
		;;

	*)	echo unknown operating system! 1>&2
		;;
	esac
	PATH=$OPATH
else		# on something that is not a pyrmaid
	if grep SIGTSTP /usr/include/signal.h > /dev/null
	then		# berkeley unix
		if test -r /usr/include/whoami.h	# 4.1
		then
			echo "BSD41"
		else					# 4.2
			echo "BSD42"
		fi
	else			# ATT unix
		echo "SYS5"
	fi
fi
