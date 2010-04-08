:
#
#  Copyright (c) 2006 Ingres Corporation
#
#  Name: (PROG1PRFX)sukerberos -- setup script for (PROD1NAME) Kerberos Driver
#  Usage:
#	(PROG1PRFX)sukerberos
#
#  Description:
#	This script should only be called by the (PROD1NAME) installation
#	utility.  It sets up the (PROD1NAME) Kerberos Driver.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	08-Jan-2002 (loera01)
##		Created.
##	20-Jan-2004 (bonro01)
##		Removed ingbuild call for PIF installs
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	05-Oct-2005 (bonro01)
##		Remove incorrect documentation reference.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##      22-Aug-2006 (Fei Ge) Bug 116528
##          Removed reference to ca.com. 
##       9-Nov-2006 (hanal04) Bug 117044
##          Ensure clean_exit() in iishlib.sh cleans up properly by setting
##          WRITE_RESPONSE, READ_RESPONSE and IGNORE_RESPONSE.
##      03-Aug-2007 (Ralph Loen) SIR 118898
##          Use iinethost to obtain the default domain name.
##      10-Aug-2007 (Ralph Loen) SIR 118898
##          Remove iigenres items, as they are now added from all.crs.
##          Prompting for domain is no longer necessary, since the
##          iinethost utility should enter the correct host name into
##          config.dat.  Iisukerberos now prompts for three main
##          configurations.
##      29-Nov-2007 (Ralph Loen) Bug 119358
##         "Server-level" (gcf.mech.user_authentication) has been deprecated.
##         Menu option 3 becomes "standard" Kerberos authentication, i.e., 
##         Kerberos listed in the general mechanism list and as a remote 
##         mechanism for the Name Server.
##      14-Jan-2008 (Ralph Loen) Bug 119739
##         Exit with status 0 for batch mode.  Batch mode currently does
##         nothing since SIR 118898 generates default Kerberos configuration
##         from all.crs.
##	26-Mar-2008 (bonro01)
##	    The "==" conditional string operator is not POSIX compliant and
##	    only works for the bash shell.  This fails on the bourne shell
##	    on Solaris so the standard "=" should be used for string comparison.
##      14-May-2008 (Ralph Loen) Bug 117737
##         Added calls to iiinitres to set DBMS-type mechanism values to "none"
##         in case of an upgrade.
##	20-Feb-2010 (hanje04)
##	   SIR 123296
##	   Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)sukerberos
#
#  DEST = utility
#----------------------------------------------------------------------------

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib
# override value of II_CONFIG set by ingbuild
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    cfgloc=$(PRODLOC)/(PROD2NAME)/files
    symbloc=$(PRODLOC)/(PROD2NAME)/files
fi
export cfgloc symbloc

BATCH=false
NOTBATCH=true
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
WRITE_RESPONSE=false
READ_RESPONSE=false
IGNORE_RESPONSE="true"
export IGNORE_RESPONSE
optional_config()
{
NOT_DONE=true
while $NOT_DONE; do
cat << !
Basic Kerberos Configuration Options

1.  Client Kerberos configuration
2.  Name Server Kerberos authentication
3.  Standard Kerberos authentication configuration (both 1 and 2)
0.  Exit

!
iiechonn "Select from [0 - 3] above [0]: "
read ENTRY junk
[ "$ENTRY" = "" ] && ENTRY=0
case $ENTRY in
    1)
        cat << !

Client configuration enables this installation to use Kerberos to authenticate 
against a remote installation that has been configured to use Kerberos for 
authentication.  This is the mimimum level of Kerberos authentication.

!
        PROMPTING=true
        while $PROMPTING; do
echo "Select 'a' to add client-level Kerberos authentication,"
iiechonn "select 'r' to remove client-level Kerberos authentication: "
        read ADDorREM junk
        ADDorREM=`echo $ADDorREM | tr '[A-Z]' '[a-z]' `
        case $ADDorREM in
        a)
             (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcf.mechanisms "kerberos"
            cat << !

Client Kerberos configuration added.

You must add the "authentication_mechanism" attribute in netutil for 
each remote node you wish to authenticate using Kerberos.  The 
"authentication_mechanism" attribute must be set to "kerberos".  There 
is no need to define users or passwords for vnodes using Kerberos 
authentication.

!
            PROMPTING=false
            pause
            ;;
        r)
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcf.mechanisms ""
            echo "Client Kerberos configuration removed"
            echo ""
            PROMPTING=false
            pause
            ;;
        *)
            echo "Entry must be 'a' or 'r'"
            pause
            ;;
        esac
        done
        ;;
    2)
        cat << !

Name Server Kerberos authentication allows the local Name Server to 
authenticate using Kerberos.

!
        PROMPTING=true
        while $PROMPTING; do
echo "Select 'a' to add Kerberos authentication for the local Name Server,"
iiechonn "select 'r' to remove Name Server Kerberos authentication: "

        read ADDorREM junk
        ADDorREM=`echo $ADDorREM | tr '[A-Z]' '[a-z]' `
        case $ADDorREM in
        a)
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.mechanisms "kerberos"
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.remote_mechanism "kerberos"
            echo ""
            echo "Kerberos authentication has been added to the Name Server on $FULL_HOSTNAME"
            echo ""
            PROMPTING=false
            pause
            ;;
        r)
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.mechanisms ""
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.remote_mechanism "none"
            echo ""
            echo "Kerberos authentication has been removed from the Name Server on $FULL_HOSTNAME"
            echo ""
            PROMPTING=false
            pause
            ;;
        *)
            echo "Entry must be 'a' or 'r'"
            pause
            ;;
        esac
        done
        ;;
    3)
        cat << !

Standard Kerberos authentication specifies Kerberos as the remote 
authentication mechanism for the Name Server, and allows clients to specify 
Kerberos for remote servers that authenticate with Kerberos.

!
        PROMPTING=true
        while $PROMPTING; do
echo "Select 'a' to add standard Kerberos authentication,"
iiechonn "select 'r' to remove standard Kerberos authentication: "

        read ADDorREM junk
        ADDorREM=`echo $ADDorREM | tr '[A-Z]' '[a-z]' `
        case $ADDorREM in
        a)
             (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcf.mechanisms "kerberos"
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.mechanisms "kerberos"
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.remote_mechanism "kerberos"
            echo ""
            echo "Standard Kerberos authentcation added." 
            cat << !

You must add the "authentication_mechanism" attribute in netutil for
each remote node you wish to authenticate using Kerberos.  The
"authentication_mechanism" attribute must be set to "kerberos".  There
is no need to define users or passwords for vnodes using Kerberos
authentication.

!
            PROMPTING=false
            pause
            ;;
        r)
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcf.mechanisms ""
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.mechanisms ""
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.remote_mechanism "none"
            echo ""
	    echo "Standard Kerberos authentcation removed." 
            echo ""
            PROMPTING=false
            pause
            ;;
        *)
            echo "Entry must be 'a' or 'r'"
            pause
            ;;
        esac
        done
        ;;
    0)
        cat << !

Kerberos configuration complete.  Be sure the GSS API library, libgssapi.$SLSFX,
resides in your library path, as defined by the environment variable
LD_LIBRARY_PATH or LIBPATH.
 
You may use the (PROG0PRFX)cbf utility to fine-tune Kerberos security options.

!
        NOT_DONE=false
        ;;
    *)  echo "Entry must be in range 0 to 3"
        pause
esac
done
trap : 0
clean_exit
}
# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
   shift ;
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   shift;
fi

export WRITE_RESPONSE
export READ_RESPONSE
FULL_HOSTNAME=`iinethost`

trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
2>/dev/null; exit 1" 0 1 2 3 15

if [ "$1" = "-rmpkg" ] ; then

   (PROG3PRFX)_CONF_DIR=${cfgloc}
 
   cp -p $(PROG3PRFX)_CONF_DIR/config.dat $(PROG3PRFX)_CONF_DIR/config.tmp
 
   trap "cp -p $(PROG3PRFX)_CONF_DIR/config.tmp $(PROG3PRFX)_CONF_DIR/config.dat; \
         rm -f $(PROG3PRFX)_CONF_DIR/config.tmp; exit 1" 1 2 3 15
 
   cat $(PROG3PRFX)_CONF_DIR/config.dat | grep -v 'gcf\.mech\.kerberos' \
       | grep -v 'kerberos_driver' > $(PROG3PRFX)_CONF_DIR/config.new
 
   rm -f $(PROG3PRFX)_CONF_DIR/config.dat
 
   mv $(PROG3PRFX)_CONF_DIR/config.new $(PROG3PRFX)_CONF_DIR/config.dat
 
   rm -f $(PROG3PRFX)_CONF_DIR/config.tmp

   cat << !
  (PROD1NAME) Kerberos Driver has been removed.

!
else

do_(PROG1PRFX)sukerberos()
{

echo "Setting up the (PROD1NAME) Kerberos Driver..."


(PROG1PRFX)sulock "(PROD1NAME) Kerberos Driver setup" || exit 1

if [ -f $(PRODLOC)/(PROD2NAME)/install/release.dat ]; then
   VERSION=`$(PRODLOC)/(PROD2NAME)/install/(PROG2PRFX)build -version` ||
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


RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.config.kerberos_driver.$RELEASE_ID` || exit 1

cat << !

This procedure will set up the following version of (PROD1NAME) Kerberos driver:

        $VERSION

to run on local host:

        $FULL_HOSTNAME

!

prompt "Do you want to continue this setup procedure?" y  || exit 1

# Generate default configuration resources in case "iisukerberos -rmpkg" 
# was invoked previously.
echo ""
echo "Generating default configuration..."

      if (PROG1PRFX)genres $CONFIG_HOST $II_CONFIG/net.rfm 
      then
         echo "Default configuration generated."
      else
         cat << !
An error occurred while generating the default configuration.

!
         exit 1
      fi
#
# Initialize DBMS server mechanisms to "none" if not already initialized
# in iigenres.
#
iiinitres -keep mechanisms $II_CONFIG/dbms.rfm > /dev/null 2>&1
iiinitres -keep mechanisms $II_CONFIG/star.rfm > /dev/null 2>&1
iiinitres -keep mechanisms $II_CONFIG/rms.rfm > /dev/null 2>&1

eval optional_config $INSTLOG
pause
}

if $BATCH
then
# Batch mode currently does nothing since the default Kerberos configuration
# is handled in all.crs, thus any script that executes iigenres adds Kerberos
# in the same manner as the other security mechanisms.
   exit 0
else
   eval do_(PROG1PRFX)sukerberos $INSTLOG
   trap : 0
   exit 0
fi
fi
