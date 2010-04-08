:
#
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: (PROG1PRFX)sudas -- setup script for (PROD1NAME) DAS Server
#
#  Usage:
#	(PROG1PRFX)sudas
#
#  Description:
#	This script should only be called by the (PROD1NAME) installation
#	utility.  It sets up the (PROD1NAME) DAS Server.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	19-Mar-2003 (wansh01)
##		Created.
##	15-Apr-2003 (wansh01)
##		Checking for existing JDBC server and configure DAS in
##		the same way and disbale JDBC server start up. 
##	23-Jun-2003 (wansh01)
##		renamed iicpyres to iicpydas.
##	14-Jul-2003 (wansh01)
##		set gcd startup count to existing JDBC startup count. 
##	20-Jan-2004 (bonro01)
##		Remove ingbuild calls for PIF installs.
##	15-Jan-2004 (hanje04)
##		Added support for running from RPM based install.
##	 	Invoking with -rpm runs in batch mode and uses
##		version.rel to check version instead of ingbuild.
##		Add while loop for parameter checking so that -rpm
##		can be used with -exresponse
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##	02-Mar-204 (hanje04)
##	    Use cat and not tee to write log file when called with -exresponse
##	    to stop output being written to screen as well as log.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	22-Mar-2004 (hanje04)
##	   Correct -exresponse logging change by not defining INSTLOG for 
##	   exresponse at all. Just use the default value.
##	12-May-2004 (bonro01)
##	   Add -vbatch option.
##	18-Nov-2004 (hweho01)
##	   Updated the startup count message. Issue #13791384. 
##	28-Nov-2004 (hanje04)
##	    Remove references to vcbf as it's no longer available on Unix.
##	28-Nov-2004 (hanje04)
##	    Remove references to vcbf as it's no longer available on Unix.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##      24-Jan-2007 (Ralph Loen) Bug 117541
##          Check for the DAS startup count before iigenres is invoked
##          to determine whether or not iicpydas should be executed.
##          Iigenres now sets the DAS startup count to 1.
##      13-Mar-2007 (hanal04) Bug 117906
##          Correct the handling of -mkresponse filename.rsp. After a
##          shift $1 is the filename not $2.
##	18-Oct-2007 (bonro01)
##	    Set das server count to zero when un-installed and
##	    un-install DAS config options.
##	28-May-2008 (bonro01)
##	    Re-format output message.
##	14-Dec-2007 (rajus01) SIR 119618
##	    Create Ingres JDBC driver properties file.
##      06-Mar-09 (rajus01) QA issue 134733  Bug #121753
##          Ingbuild reports misleading error about iijdbcprop file. This
##          is due to the incorrect return code that got introduced with
##          my recent piccolo change 495467 to fix the issues with command
##          line usage of this utility. It didn't cross my mind that this
##          utility is invoked from DAS setup program during installation.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##

##
#  PROGRAM = (PROG1PRFX)sudas
#
#  DEST = utility
#----------------------------------------------------------------------------

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib

if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    II_CONFIG=$(PRODLOC)/(PROD2NAME)/files
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc

if [ "$1" = "-rmpkg" ] ; then
   II_CONF_DIR=$cfgloc

   cp -p $II_CONF_DIR/config.dat $II_CONF_DIR/config.tmp

   trap "cp -p $II_CONF_DIR/config.tmp $II_CONF_DIR/config.dat; \
         rm -f $II_CONF_DIR/config.tmp; exit 1" 1 2 3 15

   cat $II_CONF_DIR/config.dat | grep -v '\.gcd' >$II_CONF_DIR/config.new

   rm -f $II_CONF_DIR/config.dat

   mv $II_CONF_DIR/config.new $II_CONF_DIR/config.dat

   rm -f $II_CONF_DIR/config.tmp

   cat << !
  The (PROD1NAME) Data Access Server has been removed

!
exit 0
fi

# check for response file modes
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false
BATCH=false
NOTBATCH=true
INSTLOG="2>&1 | tee -a $(PROG3PRFX)_LOG/install.log"

# check for batch flag
while [ $# != 0 ]
do
    if [ "$1" = "-batch" ] ; then
       BATCH=true
       NOTBATCH=false
       INSTLOG="2>&1 | cat >> $(PROG3PRFX)_LOG/install.log"
       shift
    elif [ "$1" = "-vbatch" ] ; then
       BATCH=true
       NOTBATCH=false
       shift
    elif [ "$1" = "-mkresponse" ] ; then
       WRITE_RESPONSE=true
       BATCH=false
       NOTBATCH=true
       shift
       if [ "$1" ] ; then
            RESOUTFILE="$1"
       shift
       fi
    elif [ "$1" = "-exresponse" ] ; then
       READ_RESPONSE=true
       BATCH=true
       NOTBATCH=false
       shift
       if [ "$1" ] ; then
            RESINFILE="$1"
       shift
       fi
    elif [ "$1" = "-rpm" ] ; then
       BATCH=true
       NOTBATCH=false
       INSTLOG="2>&1 | cat >> $(PROG3PRFX)_LOG/install.log"
       RPM=rpm
       shift
    else
       BATCH=false
       NOTBATCH=true
       shift
    fi
done # [ $# != 0 ]
export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f $cfgloc/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

do_(PROG1PRFX)sudas()
{
echo "Setting up the (PROD1NAME) Data Access Server..."
if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f $cfgloc/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

check_response_file #check for response files.

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## Skip everything if we are in WRITE_RESPONSE mode 

(PROG1PRFX)sulock "(PROD1NAME) Data Access Server setup" || exit 1

if [ -f $(PRODLOC)/(PROD2NAME)/install/release.dat ]; then
   VERSION=`$(PRODLOC)/(PROD2NAME)/install/(PROG2PRFX)build -version=das` ||
   {
       cat << !

$VERSION

!
      exit 1
   }
elif [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
   VERSION=`head -1 /usr/share/ingres/version.rel` ||
   {
       cat << !

Missing file /usr/share/ingres/version.rel

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


DASCOUNT=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.(PROG2PRFX)start.$.gcd` || exit 1
JDBCCOUNT=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.(PROG2PRFX)start.$.jdbc` || exit 1
RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.config.gcd.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

(PROD1NAME) Data Access Server has already been set up on "$HOSTNAME".

!

   $BATCH || pause
   trap : 0
   clean_exit
}

cat << !

This procedure will set up the following version of 
(PROD1NAME) Data Access Server:

        $VERSION

to run on local host:

        $HOSTNAME

!

$BATCH || prompt "Do you want to continue this setup procedure?" y  || exit 1


      # generate default configuration resources
      echo ""
      echo "Generating default configuration..."
      if (PROG1PRFX)genres $CONFIG_HOST $II_CONFIG/das.rfm ; then
	 (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.gcd.$RELEASE_ID complete
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

echo "Configuring Data Access Server listen addresses..."

# set default DAS server listen addresses
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".async.port ""
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".decnet.port (PROG3PRFX)_GCD${(PROG3PRFX)_INSTALLATION}7
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".iso_oslan.port OSLAN_${(PROG3PRFX)_INSTALLATION}7
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".iso_x25.port X25_${(PROG3PRFX)_INSTALLATION}7
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".sna_lu62.port "<none>"
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".sockets.port ${(PROG3PRFX)_INSTALLATION}7
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".spx.port ${(PROG3PRFX)_INSTALLATION}7
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".tcp_ip.port ${(PROG3PRFX)_INSTALLATION}7
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcd."*".tcp_wol.port ${(PROG3PRFX)_INSTALLATION}7

NEW_DASCOUNT=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.(PROG2PRFX)start.$.gcd` || exit 1


echo ""
echo ""
echo "Default configuration generated and startup count of the DAS server is $NEW_DASCOUNT."
echo ""
echo ""

# Create Ingres JDBC driver properties file.
if [ -x $(PRODLOC)/(PROD2NAME)/bin/(PROG1PRFX)jdbcprop ]; then
{
    echo "Executing Ingres JDBC driver properties generator utility..."
    $(PRODLOC)/(PROD2NAME)/bin/(PROG1PRFX)jdbcprop -c
}
else
    echo "Ingres JDBC driver properties generator utility not found."
fi

if [ "$JDBCCOUNT" != "" -a "$JDBCCOUNT" != "0"  -a "$DASCOUNT" = "" ]; then 
{
echo "Configuring Data Access Server from existing JDBC server and
disabling JDBC server startup..."
      if (PROG1PRFX)cpydas (PROG4PRFX).$CONFIG_HOST  ; then
 	(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.(PROG2PRFX)start.$.jdbc 0
 	(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.(PROG2PRFX)start.$.gcd $JDBCCOUNT
	cat << !

Configuration copied and startup count of the JDBC server(s) set to zero. 

!
      else
         cat << !
An error occurred while generating configuration from existing JDBC.

!
         exit 1
      fi
}
fi

cat << !

(PROD1NAME) Data Access Server has been successfully set up in this
installation. Please adjust the startup count and check the listen address
with the (PROG0PRFX)cbf utility.

!
fi #end of WRITE_RESPONSE mode
$BATCH || pause
trap : 0
clean_exit

}

eval do_(PROG1PRFX)sudas $INSTLOG
trap : 0
exit 0
