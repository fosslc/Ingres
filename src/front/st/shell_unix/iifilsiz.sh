:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: iifilsiz
#
#  Usage:
#	iifilsiz [-b] bin_file out_file
#         -b gives size in blocks
#
#  Description:
#	Called by the configuration rules system and from setup scripts
#	to return the size of various files. 
#
## History:
##	18-jan-93 (tyler)
##		Created.	
##	16-feb-93 (tyler)
##		No longer displays message if file not found and no
##		longer assumes file is located in $II_SYSTEM/ingres/bin.
##		Also, named changed from iibinsiz to iifilsiz.
##	29-mar-93 (sweeney)
##		SVR4 includes the file's group name as well as owner
##		in the output from ls -l, with the result that $4 is
##		the group instead of the size. ls -lo works on
##		Berkeley, SVR4, and SVR3 (sco).
##      18-oct-93 (smc)
##              Above change does not cater for axp_osf which still
##              outputs the group as $4.
##	09-nov-93 (smc)
##              In some sort of mental aberration my previous change
##              used the non-deliverable `readvers`. Altered to use
##		LSSIZEFLAGS which is set up in mksysdep.sh. 
##	11-nov-1997 (canor01)
##		Us LSBLOCKFLAGS to return size in blocks.
##	30-nov-1998 (toumi01)
##		Put iisysdep LS.*FLAGS variables in quotes so that parms
##		with embedded spaces can be used.
##		Correct redirection arg test from != 0 to !=1.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##	15-Feb-2010 (hanje04)
##	        SIR 123296
##		Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)filsiz
#
#  DEST = utility
#----------------------------------------------------------------------------
(LSBENV)

. (PROG1PRFX)sysdep

if [ "$1" = "-b" ]
then
    IILSFLAGS="$LSBLOCKFLAGS"
    IISZFIELD=1
    shift
else
    IILSFLAGS="$LSSIZEFLAGS"
    IISZFIELD=4
fi

if [ $# != 1 ]
then
   ls $IILSFLAGS $1 2>/dev/null | awk '{ print $'${IISZFIELD}' }' >"$2"
else
   ls $IILSFLAGS $1 2>/dev/null | awk '{ print $'${IISZFIELD}' }'
fi
