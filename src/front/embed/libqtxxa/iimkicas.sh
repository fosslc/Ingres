:
#
# Copyright (c) 2004 Ingres Corporation
#
# $Header $
#
# Name:
#       iimkicas - Build an Ingres Context Manager Application Server for
#                  use with Ingres/DTP for TUXEDO
#
# Usage:
#       iimkicas
#
# Description:
#
# Exit status:
#       0       OK
#       1       ROOTDIR not defined
#       2       II_SYSTEM not defined
#	3	$ROOTDIR/bin not writable
## History
##
##	04-mar-1994 (larrym,mhughes)
##	    written.
##	20-apr-98 (mcgem01)
##	    Product name change to Ingres.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
#  DEST = utility
##-----------------------------------------------------------------

CMD=iimkicas

[ $ROOTDIR ] ||
{
    cat << !

$CMD: FATAL ERROR 1 --------------------------------

Environment variable ROOTDIR is not defined.  Before running $CMD
you must define ROOTDIR as follows:

    In sh:      ROOTDIR=XXX; export ROOTDIR

    In csh:     setenv ROOTDIR XXX

where XXX is the full path name of the TUXEDO installation on your system
!
    exit 1
}

[ $II_SYSTEM ] ||
{
    cat << !

$CMD: FATAL ERROR 2 --------------------------------

Environment variable II_SYSTEM is not defined.  Before running $CMD
you must define II_SYSTEM as follows:

    In sh:      II_SYSTEM=XXX; export II_SYSTEM

    In csh:     setenv II_SYSTEM XXX

where XXX is the full path name of the directory to contain the installation.
!
    exit 2
}

[ -w $ROOTDIR/bin ] ||
{
    cat << !

$CMD: FATAL ERROR 3 --------------------------------

The directory $ROOTDIR/bin 
must be writable to run $CMD. 
Please fix the permisions and run again.

!
    exit 3
}

buildserver -v -r INGRES/ICAS -o $ROOTDIR/bin/iitxicas
