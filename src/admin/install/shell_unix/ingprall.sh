:
# Copyright (c) 2004, 2010 Ingres Corporation
# $Header $
#
# Name:
#	ingprall
#
# Usage:
#	ingprall
#
# Description:
#	This routine prints the INGRES variables for all the clients
#	in an installation.
#
# Exit status:
#	0	ok
#
##  History:
##      28-jun-90 (jonb)
##              Include iisysdep header file to define system-dependent
##              commands and constants.
##	25-jun-91 (jonb)
##		Add newlines to awk expression.  Some platforms require
##		this, and it doesn't seem to bother anyone.
##	31-jan-94 (tyler)
##		Remove referenced to defunct echolog.
##	26-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.
##
#  PROGRAM = (PROG2PRFX)prall
# -----------------------------------------------------------------

(LSBENV)

. (PROG1PRFX)sysdep

CMD=(PROG2PRFX)prall

if [ -d $(PRODLOC)/(PROD2NAME)/admin ]
then :
else
	cat << !

$CMD: FATAL ERROR 1 ------------------------------------------------

The directory

	$(PRODLOC)/(PROD2NAME)/admin

does not exist or it is not accessible. This indicates that there are
no client nodes in this installation. Please refer to the Installation
and Operations Guide for information on how to set up a client installation.

!
	exit 1
fi

for i in $(PRODLOC)/(PROD2NAME)/admin/*
do
	if [ -d $i ] 
	then
		node=`echo $i | \
		      awk ' BEGIN { FS = "/" } 
				  { n = NF } 
				  { print $n }'`
		echo ""
		echo "INGRES variables for node "$node"..."
		echo ""
		eval II_ADMIN=$i (PROG2PRFX)prenv 
	fi
done

exit 0
