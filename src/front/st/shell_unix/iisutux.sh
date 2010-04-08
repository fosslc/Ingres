:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Setup program for the Tuxedo Product
#
## History
##	21-Apr-94 (larrym)
##		Created.
##	22-nov-95 (hanch04)
##		add batchmode and log of install
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##	23-oct-2001 (gupsh01)
##		Added code for handling -exresponse and -mkresponse.
##	29-jan-2002 (devjo01)
## 		Set permissions for tuxedo directory.
##      15-Jan-2004 (hanje04)
##              Added support for running from RPM based install.
##              Invoking with -rpm runs in batch mode and uses
##              version.rel to check version instead of ingbuild.
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
#

if [ "$1" = "-rmpkg" ] ; then
   cat << !
   DTP for Tuxedo has been removed

!
exit 0
fi

. iisysdep
. iishlib
# override value of II_CONFIG set by ingbuild
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    II_CONFIG=$II_SYSTEM/ingres/files
    export II_CONFIG
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc


# DEST = utility

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false

# check for batch flag
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
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
fi

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

do_iisutux()
{
echo "Setting up DTP for Tuxedo..."

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## **** Skip everything if we are in WRITE_RESPONSE mode ****

if [ -d ${cfgdir}/tuxedo ]
then
    echo ${cfgdir}/tuxedo/ exists.
else
    echo Creating ${cfgloc}/tuxedo/
    mkdir ${cfgloc}/tuxedo
    chmod 755 ${cfgloc}/tuxedo
fi
fi #end WRITE_RESPONSE mode 

exit 0
}
eval do_iisutux $INSTLOG
