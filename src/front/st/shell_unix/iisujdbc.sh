:
#
#  Copyright (c) 2001, 2004 Ingres Corporation
#
#  Name: (PROG1PRFX)sujdbc -- setup script for (PROD1NAME) JDBC Server
#
#  Usage:
#	(PROG1PRFX)sujdbc
#
#  Description:
#	This script should only be called by the (PROD1NAME) installation
#	utility.  It sets up the (PROD1NAME) JDBC Server.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	25-Apr-2000 (hanch04)
##		Created.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##      27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##	10-May-2000 (rajus01)
##	      Installation of Ingres JDBC server with Comm server
##	      makes the listen address roll over to the next listen
##	      port. This change makes the JDBC server come up
##	      on fixed listen port ${II_INSTALLATION}7 and displays
##	      the configured protocol and port.
##	18-Sep-2001 (gupsh01)
##	      Added option for -mkresponse to write values to the response file.
##	23-Oct-2001 (gupsh01)
##	      Added code to handle -exresponse option.
##     07-Nov-2001 (loera01) SIR 106307 
##	      Added notation about zero startup count after successful
##	      installation.  Fixed Ingres references in comments and script.
##	20-Jan-2004 (bonro01)
##	    Removed ingbuild calls for PIF installs.
##      15-Jan-2004 (hanje04)
##              Added support for running from RPM based install.
##              Invoking with -rpm runs in batch mode and uses
##              version.rel to check version instead of ingbuild.
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	12-May-2004 (bonro01)
##	    Add -vbatch option
##	11-Jun-2004 (somsa01)
##	    Cleaned up code for Open Source.
##	28-Nov-2004 (hanje04)
##	    Remove references to vcbf as it's no longer available on Unix.
##	28-Nov-2004 (hanje04)
##	    Remove references to vcbf as it's no longer available on Unix.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##
#  PROGRAM = (PROG1PRFX)sujdbc
#
#  DEST = utility
#----------------------------------------------------------------------------

if [ "$1" = "-rmpkg" ] ; then
   cat << !
  The (PROD1NAME) JDBC Server has been removed

!
exit 0
fi

# check for response file modes
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
INSTLOG="2>&1 | tee -a $(PRODLOC)/(PROD2NAME)/files/install.log"
RPM=false

# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $(PRODLOC)/(PROD2NAME)/files/install.log"
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
elif [ "$1" = "-rpm" ] ; then
   RPM=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $(PRODLOC)/(PROD2NAME)/files/install.log"
else
   BATCH=false
   NOTBATCH=true
fi
export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f $(PRODLOC)/(PROD2NAME)/files/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

do_(PROG1PRFX)sujdbc()
{
echo "Setting up the (PROD1NAME) JDBC Server..."

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f $(PRODLOC)/(PROD2NAME)/files/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

. (PROG1PRFX)sysdep

. (PROG1PRFX)shlib

check_response_file #check for response files.

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## Skip everything if we are in WRITE_RESPONSE mode 

(PROG1PRFX)sulock "(PROD1NAME) JDBC Server setup" || exit 1

if [ -f $(PRODLOC)/(PROD2NAME)/install/release.dat ]; then
   VERSION=`$(PRODLOC)/(PROD2NAME)/install/(PROG2PRFX)build -version=das` ||
   {
       cat << !

$VERSION

!
      exit 1
   }
else
   VERSION=`head -1 $(PRODLOC)/(PROD2NAME)/version.rel` ||
   {
       cat << !

Missing file $(PRODLOC)/(PROD2NAME)/version.rel

!
      exit 1
   }
fi


RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.config.jdbc.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

(PROD1NAME) JDBC Server has already been set up on "$HOSTNAME".

!

   $BATCH || pause
   trap : 0
   clean_exit
}

cat << !

This procedure will set up the following version of (PROD1NAME) JDBC Server:

        $VERSION

to run on local host:

        $HOSTNAME

!

$BATCH || prompt "Do you want to continue this setup procedure?" y  || exit 1


      # generate default configuration resources
      echo ""
      echo "Generating default configuration..."
      if (PROG1PRFX)genres $CONFIG_HOST $(PRODLOC)/(PROD2NAME)/files/jdbc.rfm ; then
	 (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.jdbc.$RELEASE_ID complete
      else
         cat << !
An error occurred while generating the default configuration.

!
         exit 1
      fi
(PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION` || exit 1
if [ "$(PROG3PRFX)_INSTALLATION" = "" ]
then
    (PROG3PRFX)_INSTALLATION="(PROG3PRFX)"
fi
`(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.jdbc."*".port ${(PROG3PRFX)_INSTALLATION}7`
INSTALLID=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.jdbc."*".port` || exit 1
PROTOCOL=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.jdbc."*".protocol` || exit 1
      cat << !

Default configuration generated.
!
cat << !

(PROD1NAME) JDBC Server Listen Protocol is $PROTOCOL
(PROD1NAME) JDBC Server Listen Port is  $INSTALLID
(PROD1NAME) JDBC Server has been successfully set up in this installation
with a startup count of zero.  Please adjust the startup count and check the
listen address with the (PROG0PRFX)cbf utility.

!
fi #end of WRITE_RESPONSE mode
$BATCH || pause
trap : 0
clean_exit

}

eval do_(PROG1PRFX)sujdbc $INSTLOG
trap : 0
exit 0
