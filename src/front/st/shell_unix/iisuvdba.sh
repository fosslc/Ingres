:
#
#  Copyright (c) 2004 Ingres Corporation
#
#  Name: iisuvdba -- setup script for Ingres VDBA
#
#  Usage:
#	iisuvdba
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility.  It sets up the Ingres VDBA Database. 
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	17-Nov-1997 (hanch04)
##		Created.
##	18-Nov-1997 (hanch04)
##		Added prompt to continue
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##
##	12-jun-98 (mosjo01)
##		I put in a sleep 3 before ingstop to allow active sessions
##		to complete.
##	26-Aug-1998 (hanch04)
##		Moved imadb to iisudbms
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##      27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based
##              installer on Linux.
##	18-Sep-2001 (gupsh01)
##		Added option -mkresponse for generating a response file, 
##		instead of actually carrying out the install.
##	23-Oct-2001 (gupsh01)
##		Added code to handle -exresponse option.
##	20-Jan-2004 (bonro01)
##		Removed ingbuild calls for PIF installs
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##
#  DEST = utility
#----------------------------------------------------------------------------

if [ "$1" = "-rmpkg" ] ; then
   cat << !
  The Ingres VDBA has been removed

!
exit 0
fi

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp

# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
fi
export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

if [ "$WRITE_RESPONSE" = "false" ] ; then
  trap "rm -f $II_SYSTEM/ingres/files/config.lck /tmp/*.$$ 1>/dev/null \
     2>/dev/null; exit 1" 0 1 2 3 15
fi

do_iisuvdba()
{
echo "Setting up the Ingres VDBA..."

. iisysdep

. iishlib

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## **** Skip everything if we are in WRITE_RESPONSE mode ****

iisulock "Ingres VDBA setup" || exit 1

if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
    VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=vdba` ||
    {
	cat << !

$VERSION

!
	exit 1
    }
else
    VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
    {
	cat << !

Missing file $II_SYSTEM/ingres/version.rel

!
	exit 1
    }
fi

RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`iigetres ii.$CONFIG_HOST.config.vdba.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

Ingres VDBA has already been set up on "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
}

DBMS_SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`

if [ "$DBMS_SETUP" != "complete" ]
then
   cat << !

The setup procedures for the following version of Ingres Intelligent
DBMS:

        $VERSION

must be completed up before Ingres VDBA can be set up on host:

        $HOSTNAME

!
   $BATCH || pause
   exit 1
fi

cat << !

This procedure will set up the following version of Ingres VDBA:

        $VERSION

to run on local host:

        $HOSTNAME

!

$BATCH || prompt "Do you want to continue this setup procedure?" y  || exit 1

iisetres ii.$CONFIG_HOST.config.vdba.$RELEASE_ID complete
cat << !

Ingres VDBA has been successfully set up in this installation.

!

fi #end of WRITE_RESPONSE mode

$BATCH || pause
trap : 0
clean_exit

}

eval do_iisuvdba $INSTLOG
trap : 0
exit 0
