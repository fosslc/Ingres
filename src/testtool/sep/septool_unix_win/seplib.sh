:
#
# Copyright (c) 2004 Ingres Corporation
#
# seplib -- Create a library for SEP to use.
#
## History:
##	06-Aug-91 (DonJ)
##		Created.
##      20-Aug-91 (DonJ)
##              Simplified conditional statements. Made other changes
##              suggested by Roger Taranto.
##      27-Aug-91 (DonJ) 
##              More improvements.
##	18-feb-1994 (donj)
##		Added logic to dot iisysdep. Added conditional for ranlib
##		dependent on env var, HASRANLIB.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
## 
#

CMD=seplnk
export CMD

if [ $# -eq 0 ]
then
     cat << !
     
Usage: seplib <libname> [<ofile1> ... <ofilen>]
     
!
exit 0
fi

###############################################################################
#
# Check local Ingres environment
#
###############################################################################

if [ ! -n "${II_SYSTEM}" ]
then
     cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|     The local Ingres environment must be setup and the     |
|     the installation running before continuing             |
|------------------------------------------------------------|

!
exit 1
fi

. $II_SYSTEM/ingres/utility/iisysdep


case $1 in
	*.a)	library=`basename $1`
		;;

	*.*)	library=`basename $1`
		;;

	*)	if [ -f `basename $1` ]
		then
		   library=`basename $1`
		else
		   library=`basename $1.a` 
		fi
		;;
esac

[ ! -f $library ] && ar ruc $library
shift

for i
do
	case $i in
		*.o)	objfile=`basename $i`
			;;

		*)	if [ -f `basename $i.o` ]
			then
			    objfile=`basename $i.o`
			else
			    objfile=`basename $i`
			fi
			;;
	esac

	[ -f $objfile ] && ar ruc $library $objfile
done

if [ -n "${HASRANLIB}" ]
then
    ranlib $library
fi

