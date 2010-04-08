:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name:
#	iisubr -- set-up script for Ingres Protocol Bridge.
#
#  Usage:
#	iisubr -batch [inst_code]
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility or by the ingmknfs wrapper.  It sets up Ingres
#	Protocol Bridge on a server.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	29-Feb-96 (rajus01)
##		Created. Culled from iisunet.sh	
##      18-Apr-96 (hanch04)
##              Added colon at top of file so mkming would make it a script.
##      26-Sep-96 (rajus01)
##		Added time delay between starting and stopping the nameserver
##		to avoid Protocol Bridge Server hanging during installation.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	09-jun-1998 (hanch04)
##		We don't use II_AUTH_STRING for Ingres II
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##    27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##	12-may-2000 (somsa01)
##		Added support for multiple product builds.
##	01-sep-2000 (somsa01)
##		For HP 11.00, hpb_us5, SNAplus2 support in iigcb utilizes
##		shared libs in /opt/sna/lib.  These libs are not dynamically
##		loadable with shl_load/shl_findsym, so if they are not present,
##		dummy versions are created in $II_SYSTEM/ingres/lib to allow
##		the communications servers to start without problems.
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based 
##              installer on Linux.
##	04-Sep-2001 (gupsh01)
##		Added flags input -mk_response, ex_response for creating and 
##		exetuging response files.  
##	03-Oct-2001 (hanch04)
##		Bug # 101162
##		If -rmpkg is passed it should remove the protocol bridge 
##		and quit else it should do the install
##	23-Oct-2001 (gupsh01)
##		Make changes to implement reading parameter values 
##		from response file. (flag -exresponse).
##	11-feb-2002 (devjo01)
##		Allow secondary cluster node setup.
##      01-Mar-2002 (wansh01)
##              Moved set defalut port logic to bridge.crs.
##	18-Jul-2002 (hanch04)
##		Changed Installation Guide to Getting Started Guide.
##	20-Jan-2004 (bonro01)
##		Remove ingbuild call for PIF installation.
##	27-Jan-2004 (hanje04)
##		Added -rpm flag to support installing from RPM files.
##	    	Add while loop for checking parameter so that -rpm can be 
##		used with -exresponse
##	02-Mar-204 (hanje04)
##	    Use cat and not tee to write log file when called with -exresponse
##	    to stop output being written to screen as well as log.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	22-Mar-2004 (hanje04)
##	   Correct -exresponse logging change by not defining INSTLOG for 
##	   exresponse at all. Just use the default value.
##	18-Nov-2004 (sheco02)
##	   Remove the part of starting name server and then stopping it because
##	   it is unnecessary at all.
##	29-Nov-2004 (sheco02)
##	   Minor fix for the previous removal of the name server.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##    29-Jul-2009 (rajus01) Issue:137354, Bug:122277
##          During upgrade gcn.local_vnode gets modified. The setup scripts
##          should not set/modify this value. My research indicates that
##          the Ingres setup scripts started to set this parameter as per the
##          change 405090 (21-Sep-1993). Later came along the change 420349
##          (bug 70999) in Sep 1995 which defined this parameter for Unix
##          platforms in crs grammar (all.crs). So, the Unix scripts during
##          fresh install or upgrade should not set this parameter 
##          explicitly. 
##	15-Dec-2009 (hanje04)
##	    Bug 123097
##	    Remove references to II_GCNXX_LCL_VNODE as it's obsolete.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)subr
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
   (PROG3PRFX)_CONF_DIR=$cfgloc
 
   cp -p $(PROG3PRFX)_CONF_DIR/config.dat $(PROG3PRFX)_CONF_DIR/config.tmp
 
   trap "cp -p $(PROG3PRFX)_CONF_DIR/config.tmp $(PROG3PRFX)_CONF_DIR/config.dat; \
         rm -f $(PROG3PRFX)_CONF_DIR/config.tmp; exit 1" 1 2 3 15
 
   cat $(PROG3PRFX)_CONF_DIR/config.dat | grep -v 'gcb' | \
       grep -v '\.bridge'  >$(PROG3PRFX)_CONF_DIR/config.new
 
   rm -f $(PROG3PRFX)_CONF_DIR/config.dat
 
   mv $(PROG3PRFX)_CONF_DIR/config.new $(PROG3PRFX)_CONF_DIR/config.dat
 
   rm -f $(PROG3PRFX)_CONF_DIR/config.tmp
 
   cat << !
  (PROD1NAME) Protocol Bridge has been removed.
 
!

else

# set up the shared library dummies for SNAplus2, if necessary
[ "$VERS" = "hpb_us5" -o "$VERS" = "hp8_us5" ] && [ ! -d "/opt/sna/lib" ] &&
{
    [ -f $II_SYSTEM/ingres/lib/libappc.1 ] && rm $II_SYSTEM/ingres/lib/libappc.1
    [ -f $II_SYSTEM/ingres/lib/libcsv.1 ] && rm $II_SYSTEM/ingres/lib/libcsv.1
    [ -f $II_SYSTEM/ingres/lib/libmgr.sl ] && rm $II_SYSTEM/ingres/lib/libmgr.sl
    ln -s $II_SYSTEM/ingres/lib/libcompat.1.sl $II_SYSTEM/ingres/lib/libappc.1
    ln -s $II_SYSTEM/ingres/lib/libcompat.1.sl $II_SYSTEM/ingres/lib/libcsv.1
    ln -s $II_SYSTEM/ingres/lib/libcompat.1.sl $II_SYSTEM/ingres/lib/libmgr.sl
}

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
       INSTLOG="2>&1 | tee -a $(PROG3PRFX)_LOG/install.log"
       shift
    elif [ "$1" = "-mkresponse" ] ; then
       WRITE_RESPONSE=true
       BATCH=false
       NOTBATCH=true
       INSTLOG="2>&1 | tee -a $(PROG3PRFX)_LOG/install.log"
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
       RPM=true
       shift
    else
       BATCH=false
       NOTBATCH=true
       INSTLOG="2>&1 | tee -a $(PROG3PRFX)_LOG/install.log"
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

do_iisubridge()
{
echo "Setting up the (PROD1NAME) Protocol Bridge..."
if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f $cfgloc/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi


check_response_file #if mkresponse mode make sure that the response files exist.

if [ "$WRITE_RESPONSE" = "true" ] ; then 
  mkresponse_msg
  eval check_response_write (PROG3PRFX)_CONFIG $(PROG3PRFX)_CONFIG replace
else
    (PROG1PRFX)sulock "(PROD1NAME) Protocol Bridge setup" || exit 1

touch_files # make sure symbol.tbl and config.dat exist

# override local (PROG3PRFX)_CONFIG symbol.tbl setting
  (PROG2PRFX)setenv (PROG3PRFX)_CONFIG $(PROG3PRFX)_CONFIG

SERVER_HOST=`(PROG1PRFX)getres (PROG4PRFX)."*".config.server_host` || exit 1

# grant owner and root all privileges 
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.privileges.user.root \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.privileges.user.$(PROG5PRFX)_USER \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

#
# Don't set gcn local_vnode resource explicitly. Use the default local_vnode
# set in crs grammar (all.crs). This all.crs file is included in all of the
# "*.rfm" files.

[ -z "$SERVER_HOST" ]  &&
{
   SERVER_ID=`eval (PROG3PRFX)_ADMIN=$symbloc (PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
   [ "$SERVER_ID" ] && 
   {
      # convert FQDNs as per PMhost()
      SERVER_HOST=`eval (PROG3PRFX)_ADMIN=$symbloc \
         (PROG1PRFX)pmhost`
   }
}

if [ -n "$SERVER_HOST" -a "$SERVER_HOST" != "$CONFIG_HOST" ] ; then
    CLUSTER_ID=""
    if $CLUSTERSUPPORT ; then
	CLUSTER_ID="`(PROG1PRFX)getres \
	    (PROG4PRFX).$CONFIG_HOST.config.cluster.id`"
	case "$CLUSTER_ID" in
	[1-9]*)     # Setup for cluster
	    ;;
	*)          # normal non-server setup.
	    CLUSTER_ID=""
	    ;;
	esac
    fi
    [ -z "$CLUSTER_ID" ] &&
    { 
	cat << !

The (PROD1NAME) Protocol Bridge  installation located at:

	$(PRODLOC)

is already set up to run from:

	$SERVER_HOST
!
	$BATCH || pause
	trap : 0
	clean_exit
    }
fi

if [ -f $(PRODLOC)/(PROD2NAME)/install/release.dat ]; then
   VERSION=`$(PRODLOC)/(PROD2NAME)/install/(PROG2PRFX)build -version=bridge` ||
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

SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.config.bridge.$RELEASE_ID`
if [ "$SETUP" = "complete" ] ; then
   cat << !

The $VERSION version of the (PROD1NAME) Protocol Bridge has
already been set up on local host "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
fi
cat << ! 

This procedure will set up the following release of the (PROD1NAME)
Protocol Bridge:

	$VERSION

to run on local host:
 
	$HOSTNAME

This installation will be owned by:

	$(PROG5PRFX)_USER
!

fi #end WRITE_RESPONSE

#
# get (PROG3PRFX)_INSTALLATION 
#
[ -z "$(PROG3PRFX)_INSTALLATION" ] && (PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
if [ "$(PROG3PRFX)_INSTALLATION" ] ; then 
   cat << !

(PROG3PRFX)_INSTALLATION configured as $(PROG3PRFX)_INSTALLATION. 
!
if [ "$WRITE_RESPONSE" = "true" ] ; then
   eval check_response_write (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION replace
else
   (PROG2PRFX)setenv (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION
fi
else
   if $BATCH ; then
      if [ "$READ_RESPONSE" = "true" ] ; then
         (PROG3PRFX)_INSTALLATION=`iiread_response (PROG3PRFX)_INSTALLATION $RESINFILE`
         [ -z "$(PROG3PRFX)_INSTALLATION" ] && (PROG3PRFX)_INSTALLATION="(PROG3PRFX)"
      elif [ "$2" ] ; then
         case "$2" in 
            [a-zA-Z][a-zA-Z0-9])
               (PROG3PRFX)_INSTALLATION="$2"
               ;;
            *)
               cat << !

The installation code you have specified is invalid. The first character
code must be a letter; the second may be a letter or a number.

!
               exit 1
               ;;
         esac
      else
         (PROG3PRFX)_INSTALLATION="(PROG3PRFX)"
      fi
      cat << !

(PROG3PRFX)_INSTALLATION configured as $(PROG3PRFX)_INSTALLATION. 
!
   else
      get_installation_code
   fi
   if [ "$WRITE_RESPONSE" = "true" ] ; then
	 eval check_response_write (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION replace
   else 
         (PROG2PRFX)setenv (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION
   fi
fi

$BATCH || prompt "Do you want to continue this setup procedure?" y \
|| exit 1

if [ "$WRITE_RESPONSE" = "false" ] ; then
   if [ "$SETUP" != "defaults" ] ; then
      # generate default configuration resources
      echo ""
      echo "Generating default configuration..."
      if (PROG1PRFX)genres $CONFIG_HOST $(PROG3PRFX)_CONFIG/bridge.rfm ; then
        (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.bridge.$RELEASE_ID defaults
      else
         cat << !
An error occurred while generating the default configuration.

!
         exit 1
      fi
   else
      cat << !

Default configuration generated.
!
   fi


(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.bridge.$RELEASE_ID complete
remove_temp_resources
cat << !
(PROD1NAME) Protocol Bridge has been successfully set up in this installation.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) client.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!
fi #end of WRITE_RESPONSE
   $BATCH || pause
trap : 0
clean_exit
}

eval do_iisubridge $INSTLOG
trap : 0
exit 0
fi
