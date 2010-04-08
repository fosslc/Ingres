:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name:
#	ingmknfs -- stand-alone program for setting up INGRES NFS client
#		instllallations.
#
#  Usage:
#	ingmknfs [ [ -batch -s $II_SYSTEM ] host { host } ]
#
#  Description: This script can be run on the server or client machine
#	in order to setup an INGRES NFS client installation.  INGRES/Net
#	must be installed on the server before this program can be used. 
#
#	Batch mode can be run on the server to setup clients with
#	the server system defaults.  The client's II_SYSTEM can be defined
#	with the -s flag.
#
## History:
##	18-jan-93 (tyler)
##		Created.	
##	19-oct-93 (tyler)
##		Added message to the beginning announcing which remote
##		host is being configured.
##	22-nov-93 (tyler)
##		Replaced system calls with shell procedure invocations.
##	16-dec-93 (tyler)
##		Fixed dumb shell coding bug.
##	19-may-1997 (hanch04)
##		Added -batch flag to install client with system defaults.
##		-s flag is to define the client's II_SYSTEM if different.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG2PRFX)mknfs
#
#  DEST = utility
#----------------------------------------------------------------------------
(LSBENV)

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib

#
# If arguments were passed, called iisunet repeatedly to set up an NFS
# client for each named (remote) host.
#
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $(PRODLOC)/(PROD2NAME)/files/install.log"
   shift ;
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $(PRODLOC)/(PROD2NAME)/files/install.log"
fi
export BATCH
export INSTLOG


do_setup()
{
if [ $# != 0 ]
then

   SERVER_(PRODLOC)=$(PRODLOC)
   [ "$1" = "-s" ] && 
   {
	SERVER_(PRODLOC)=$2
	shift ;
	shift ;
   }
   export SERVER_(PRODLOC)
	   
for host in $*
   do
      cat << !

Configuring NFS-client on "$host"...

!
      $BATCH || iisunet $host
      $BATCH && iisunet -batch $host

   done
   clean_exit
fi

#
# If no arguments were passed, run iisunet to set up one (local) NFS client.
#
iisunet
clean_exit
}
 
eval do_setup $* $INSTLOG
