:
#  Copyright (c) 2005, 2010 Ingres Corporation
#
# Name: iisuspat64.sh
#
# Purpose:
#	Setup program for Ingres 64bit Spatial Object Library
#
## History
##	20-Sep-2005 (hanje04)
##	    SIR 115239
##	    Created.
##	26-Oct-2005 (hanje04)
##	    SIR 115239
##	    Spatial Objects Library needs to be shared library on
##	    Linux.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
#

# DEST = utility

# Initialize variables

. iishlib
. iisysdep

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false
REMOVE=false

config_host=`iipmhost`

# Process command line args
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
elif [ "$1" = "-rpm" ] ; then
   RPM=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-rmpkg" ] ; then
   REMOVE=true
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
fi

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

#
# do_iisuspat64 - Setup package
#
do_iisuspat64()
{
    echo "Setting up Ingres 64bit Spatial Object Library"

    check_response_file

    if $WRITE_RESPONSE ; then
   	mkresponse_msg
	rc=$?
    else ## **** Skip everything if we are in WRITE_RESPONSE mode ****
 	## Check library is installed
	if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	    libdir=/var/lib/ingres
	else
	    libdir=$II_SYSTEM/ingres/lib
        fi
	if [ -f "$libdir/lp64/libspat.a" ] || \
		[ -f "$libdir/lp64/libspat.1.$SLSFX" ] ; then
	    iisetres ii.$config_host.config.spatial64 ON
	    rc=$?
	else
	    echo "Ingres 64bit Spatial Object Library is not installed"
	    rc=1
	fi
    fi #end WRITE_RESPONSE mode 

}

#
# do_rmspat64 - Remove package
#
do_rmspat64()
{
    echo "Removing Ingres 64bit Spatial Object Library"
    iisetres ii.$config_host.config.spatial64 OFF
}

if $REMOVE ; then
    eval do_rmspat64 $INSTLOG
    rc=$?
else
    eval do_iisuspat64 $INSTLOG
    rc=$?
fi

exit $rc
