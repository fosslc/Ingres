:
#
#  Copyright (c) 2001, 2004 Ingres Corporation
#
#  Name:
#	(PROG1PRFX)sunet -- set-up script for (PROD1NAME) Networking
#
#  Usage:
#	(PROG1PRFX)sunet [ host ]
#
#  Description:
#	This script should only be called by the (PROD1NAME) installation
#	utility or by the (PROG2PRFX)mknfs wrapper.  It sets up (PROD1NAME) Networking
#	on a client or server.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	18-jan-93 (tyler)
##		Created.	
##	16-feb-93 (tyler)
##		Vastly improved.
##	18-feb-93 (tyler)
##		Added Net server listen address initialization.
##      15-mar-93 (tyler)
##		Changed if/then and while loop coding style.  Added error
##		checking for symbol.tbl and config.dat creation.  Added
##		batch mode.
##	17-mar-93 (tyler)
##		Added code which validates installation code specified 
##		on the command line in batch mode.  Added check for syntax
##		errors in configuration files.
##	26-apr-93 (tyler)
##		Minor formatting change.
##	21-jun-93 (tyler)
##		Added "Press RETURN to continue:" prompt after errors.
##		An interrupt now cause the program to exit with an error
##		status. 
##	23-jul-93 (tyler)
##		Send netutil output to /dev/null instead of /tmp/netutil.dat
##		(which wasn't needed and wasn't getting cleaned up).  Removed
##		-installation flag from iimaninf call.
##	26-jul-93 (tyler)
##		Included iishlib and added call to remove_temp_resources()
##		before final (completion) exits to strip temporary resources
##		from config.dat.  Added call to iisulock to lock config.dat
##		during setup.  Switched to using pause_message(). 
##	04-aug-93 (tyler)
##		Removed Net server port status initialization.  Replaced
##		calls to pause() before failure exits by calls to
##		pause_message().
##	20-sep-93 (tyler)
##		Make sure II_CONFIG is set before it is referenced.
##		Also, cleaned up previously muddled code which sets the
##		value.  Fixed broken messages for client/server cases.
##		Allow II_INSTALLATION and II_TIMEZONE_NAME to be defined
##		in Unix environment before setup.  Changed default
##		value of II_TIMEZONE_NAME to NA-PACIFIC.  Set the
##		local_vnode resource explicitly.
##	19-oct-93 (tyler)
##		Improved output format.  II_CONFIG need to remain set
##		on an NFS-client (BUG 55400) throughout setup.  Removed
##		reference to the "Running INGRES" chapter of the Installation
##		Guide.  Removed redundant message which reported syntax
##		error in config.dat or protect.dat.  Don't get
##		II_TIMEZONE_NAME from the environment because ingbuild 
##		sets it to IIGMT.  Stopped clearing the screen at the
##		beginning of the script.  Added default responses to
##		(y/n) questions.  Removed calls to pause_message().
##	22-nov-93 (tyler)
##		Removed multiple instances of code which removed config.lck
##		on exit - now handled by "trap".  Replaced several system
##		calls with calls to shell procedures.  Replaced iimaninf
##		call with ingbuild call.
##	29-nov-93 (tyler)
##		iiechonn reinstated.
##	30-nov-93 (tyler)
##		ingbuild must be invoked by its full path name.  Don't
##		set II_CONFIG in symbol.tbl.
##	01-dec-93 (tyler)
##		Corrected erroneous text which described installation
##		passwords as being specific to a protocol.
##	16-dec-93 (tyler)
##		Use CONFIG_HOST instead of HOSTNAME to compose
##		configuration resource names.  Consolidate privilege
##		resources using PM wildcard mechanism.  Fixed BUG
##		which broke non-NFS client setup.
##	31-jan-94 (tyler)
##		Updated privilege string syntax.	
##	22-feb-94 (tyler)
##		Fixed BUG 59157: fixed incorrect default protocol
##		addresses.  Added default port for SNA LU0.  Fixed
##		BUG 59539: Added Net listen address configuration
##		for Berkeley sockets. 
##      08-Apr-94 (michael)
##              Changed references of II_AUTHORIZATION to II_AUTH_STRING where
##              appropriate.
##      31-jan-95 (tutto01) Bug 65309
##              Added code to get the value of II_CHARSETxx from the NFS
##              mounted system.  Before this neccesary variable was not being
##              set, causing translation problems (see bug#)
##      03-apr-95 (canor01)
##              Traps cannot be canceled inside a function in AIX 4.1, so
##              explicitly issue "trap 0" before calling clean_exit()
##	02-jun-95 (forky01)
##		Now ask for II_CHARSET if not set in server to allow Ingres
##		Net clients to specify charset.  Fix bug 69105.
##      15-jun-95 (popri01)
##              nc4_us5 complained about VERSION sed string. Escape the '/'.
##      17-jul-95 (harpa06)
##              Added feature of removing all references to INGRES/Net in 
##		config.dat when removing the package through INGBUILD.
##	17-Nov-95 (hanch04)
##		Changed ping to PINGCMD
##	22-Nov-95 (hanch04)
##		Added loggin of script
##	8-dec-95 (stephenb)
##		Fix typo in user visible text.
##	08-dec-95 (hanch04)
##		Do not pause on errors in batch mode.
##  6-mar-1995 (angusm)
##      Change dated 22-Nov-95 reverses order of setting of
##      HOSTNAME, resulting in bug 74588 (can't set up NFS
##      client from SERVER end). Changed order back to previous.
##	9-may-1996 (angusm)
##		Add PINGARGS to PINGCMD for platforms where 'ping' needs
##		arguments to ensure that 'pinging' is not done indefinitely
##		causing script to hang.
##	10-jul-96 (hanch04)
##		Move echo into do_setup to remove extra output in express inst.
##	31-jul-96 (hanch04)
##		Move trap into do_setup. 
##	06-aug-96 (hanch04)
##		Move some steps back out of do_setup, problem with ingmknfs
##		Bug 74588
##	19-may-97 (hayke02)
##		If the server installation is SQL-92 compliant, we now set up
##		the NFS client installation to be SQL-92 compliant as well.
##		This change fixes bug 78211.
##	15-may-1997 (hanch04)
##		Added batch for ingmknfs
##      29-Aug-97 (hanal04)
##              Flag iisetres and iigenres to Read/Write the config.dat
##              file in II_CONFIG_LOCAL if II_CONFIG_LOCAL is set (b78563).
##      02-Sep-97 (hanal04)       
##              Correct alignment of if's and fi's from previous change.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	09-jun-1998 (hanch04)
##		We don't use II_AUTH_STRING for Ingres II
##      21-Jul-1998 (hanal04)
##              - Rework NFS client fix : backup config.dat, make changes,
##              copy new config to admin/client directory and restore
##              the original config.dat.
##              - Undo the previous change which has been applied to 
##              Ingres 2.0 code line. 
##              - Add II_AUTHORIZATION to client symbol table as OpenRoad
##              applications will use this. b91480.
##              - When setting II_CHARSET in NFS client symbol table
##              make sure we do so under CLIENT_ADMIN.
##	30-jul-1998 (muhpa01)
##		For HP 11.00, hpb_us5, SNAplus2 support in iigcc/iigcb utilizes
##		shared libs in /opt/sna/lib.  These libs are not dynamically
##		loadable with shl_load/shl_findsym, so if they are not present,
##		dummy versions are created in $II_SYSTEM/ingres/lib to allow
##		the communications servers to start without problems.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##    09-feb-1999 (loera01) Bug 95164
##            Revised default configuration of DECNet so that the
##            "_0" string isn't appended to the listen address. Makes 
##            rollover of the address possible for multiple GCC's.
##	13-apr-1999 (muhpa01)
##		hp8_us5 now supports SNAplus2 - add it to check made for
##		hpb_us5.  Also, add new dummy lib for libmgr.sl.  And finally,
##		clean any old dummies & reset as links to libcompat.1.sl.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##      27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##	31-aug-2000 (hanch04)
##	    If this is an embeded install use the tng.rfm
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based
##              installer on Linux.
##	17-Sep-2001 (gupsh01)
##	     Added support for -mkresponse flags, to generate a response file, 
##	     instead of doing the install.
##	22-Oct-2001 (loera01) SIR 106118
##              Do not prompt for installation passwords in other products.  
##	24-Oct-2001 (gupsh01)
##	     Added support for -exresponse flag, to install from a response file.
##	05-Feb-2002 (devjo01)
##	     Modified to support installation on a cluster.
##      18-Jul-2002 (hanch04)
##              Changed Installation Guide to Getting Started Guide.
##	08-Jan-2004 (somsa01)
##	    We do not need to create an empty symbol.tbl file anymore.
##	20-Jan-2004 (bonro01)
##	    Removed ingbuild calls for PIF installs
##      15-Jan-2004 (hanje04)
##              Added support for running from RPM based install.
##              Invoking with -rpm runs in batch mode and uses
##              version.rel to check version instead of ingbuild.
##	03-feb-2004 (somsa01)
##	    Removed symbol.tbl change for now.
##	09-Mar-2004 (bonro01)
##	    Removed extra 'shift' statements.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	11-Jun-2004 (somsa01)
##	    Cleaned up code for Open Source.
##      16-Sep-2004 (ashco01) Bug: 112323, Problem: INGINS237
##              install.log contains misleading response value to installation
##              password setup prompt. The correct response (entered by the user)
##              is now recorded.
##	21-Sep-2004 (hanje04)
##	    SIR 113101
##	    Remove all trace of EMBEDDED or MDB setup. Moving to separate 
##	    package.
##	23-Nov-2004 (hanje04)
##	    Remove the NET_ from response file parameters. Shouldn't be
##	    any different from those in DBMS setup.
##	    Also check for II_CHARSET and not II_CHARSET$II_INSTALLATION
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##      07-Apr-2005 (hanje04)
##          Bug 114248
##          When asking users to re-run scripts it's a good idea to tell
##          them which script needs re-running.
##	16-Jun-2005 (hanje04)
##	    BUG 114692
##	    Always need to set trap whether we're running WRITE_RESPONSE or
##	    not.
##	    Don't bail out if SERVER_HOST=true and iisudbms hasn't been run
##	    during RPM setup. If ca-ingres-net comes first in the package
##	    list there's nothing we can do about it. Doesn't seem to 
##	    cause any problems anyway.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##	    Change Networking Guide to Connectivity Guide.
##	26-Oct-2007 (bonro01)
##	    Correct "! $RPM" syntax which is not valid on Solaris.
##	19-Mar-2008 (rajus01) Bug 120030, SD issue 126301
##	    Customer reported hang in ingmknfs utility while running it
##	    from an NFS client machine to configure Ingres/NET client. But the 
##	    hang actually occurs in 'iisunet' when 'ingmknfs' invokes it with
##	    just the hostname alone as the input argument. Currently 'iisunet'
##	    expects only "-batch", "-mkresponse", "-exresponse", "-vbatch", and
##	    "-rpm" in the while loop.
##      01-Jul-2008 (Ralph Loen) Bug 120583
##          For client installations, explain the impact of specifying
##          an installation password for the local installation.
##	29-Jul-2009 (rajus01) Issue:137354, Bug:122277
##	    During upgrade gcn.local_vnode gets modified. The setup scripts
##	    should not set/modify this value. My research indicates that
##	    the Ingres setup scripts started to set this parameter as per the 
##	    change 405090 (21-Sep-1993). Later came along the change 420349
##	    (bug 70999) in Sep 1995 which defined this parameter for Unix
##	    platforms in crs grammar (all.crs). So, the Unix scripts during
##	    fresh install or upgrade should not set this parameter explicitly.
##	    My change removes the definition from the setup scripts. 
##	    Also changed SERVER_HOST to check against the value `iipmhost` 
##	    instead of $HOSTNAME.
##	19-Oct-2009 (frima01) Issue:137354, Bug:122277
##	    Replaced `iipmhost` with $HOSTNAME again since iipmhost replaces .
##	    in the hostname with _
##	17-Nov-2009 (frima01) Issue:137354, Bug:122277
##	    Now replaced $HOSTNAME with new `iinethost -local` command when
##	    checking SERVER_HOST because we need the name in lower case.
##	20-Feb-2010 (hanje04)
##	   SIR 123296
##	   Add support for LSB builds
##	14-Sep-2010 (rajus01) Bug 124381, SD issue 146492
##	   Set the mechanism_location_lp32 to the value pointed by 
##	   mechanism_location in the case of upgrade. The mechanism_location
##	   _lp64 will remain unmodified. And remove the config parameter 
##	   mechanism_location as it is no longer required for
##	   the hybrid platforms.
##	   
##
#  PROGRAM = (PROG1PRFX)sunet
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
   (PROG3PRFX)_CONF_DIR=${cfgloc}
 
   cp -p $(PROG3PRFX)_CONF_DIR/config.dat $(PROG3PRFX)_CONF_DIR/config.tmp
 
   trap "cp -p $(PROG3PRFX)_CONF_DIR/config.tmp $(PROG3PRFX)_CONF_DIR/config.dat; \
         rm -f $(PROG3PRFX)_CONF_DIR/config.tmp; exit 1" 1 2 3 15
 
   cat $(PROG3PRFX)_CONF_DIR/config.dat | grep -v 'gcc' | \
       grep -v '\.net'  >$(PROG3PRFX)_CONF_DIR/config.new
 
   rm -f $(PROG3PRFX)_CONF_DIR/config.dat
 
   mv $(PROG3PRFX)_CONF_DIR/config.new $(PROG3PRFX)_CONF_DIR/config.dat
 
   rm -f $(PROG3PRFX)_CONF_DIR/config.tmp
 
   cat << !
  (PROD1NAME) Networking has been removed.
 
!
 
else

ACTUAL_HOSTNAME=$HOSTNAME

[ "$VERS" = "hpb_us5" -o "$VERS" = "hp8_us5" ] && [ ! -d "/opt/sna/lib" ] &&
{
	[ -f $(PRODLOC)/(PROD2NAME)/lib/libappc.1 ] && rm $(PRODLOC)/(PROD2NAME)/lib/libappc.1
	[ -f $(PRODLOC)/(PROD2NAME)/lib/libcsv.1 ] && rm $(PRODLOC)/(PROD2NAME)/lib/libcsv.1
	[ -f $(PRODLOC)/(PROD2NAME)/lib/libmgr.sl ] && rm $(PRODLOC)/(PROD2NAME)/lib/libmgr.sl
	ln -s $(PRODLOC)/(PROD2NAME)/lib/libcompat.1.sl $(PRODLOC)/(PROD2NAME)/lib/libappc.1
	ln -s $(PRODLOC)/(PROD2NAME)/lib/libcompat.1.sl $(PRODLOC)/(PROD2NAME)/lib/libcsv.1
	ln -s $(PRODLOC)/(PROD2NAME)/lib/libcompat.1.sl $(PRODLOC)/(PROD2NAME)/lib/libmgr.sl
}

BATCH=false
NOTBATCH=true
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
IGNORE_RESPONSE="true"
export IGNORE_RESPONSE
RPM=false
BATCH=false
NOTBATCH=true
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
SELF=`basename $0`

while [ $# != 0 ]
do
# check for batch flag
    if [ "$1" = "-batch" ] ; then
       BATCH=true
       NOTBATCH=false
       INSTLOG="2>&1 | cat >> $II_LOG/install.log"
       shift ;
    elif [ "$1" = "-rpm" ] ; then
       RPM=true
       BATCH=true
       NOTBATCH=false
       INSTLOG="2>&1 | cat >> $II_LOG/install.log"
       shift ;
    elif [ "$1" = "-mkresponse" ] ; then
       WRITE_RESPONSE=true
       BATCH=false
       NOTBATCH=true
       shift;
       if [ "$1" ] ; then
            RESOUTFILE="$1"
            shift ;
       fi
    elif [ "$1" = "-exresponse" ] ; then
       READ_RESPONSE=true
       BATCH=true
       NOTBATCH=false
       shift;
       if [ "$1" ] ; then
            RESINFILE="$1"
            shift ;
       fi
    elif [ "$1" = "-vbatch" ] ; then
       BATCH=true
       NOTBATCH=false
       shift;
    else break; 
    fi
    
    export WRITE_RESPONSE
    export READ_RESPONSE
    export RESOUTFILE
    export RESINFILE
    
done

       [ "$1" ] &&
       {
	    # reset HOSTNAME for remote NFS-client
  	    HOSTNAME=$1
  	    CONFIG_HOST=`echo $HOSTNAME | sed "s/[.]/_/g"`
  	    CLIENT_ADMIN=$(PRODLOC)/(PROD2NAME)/admin/$HOSTNAME
	    shift
       }
    
trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
2>/dev/null; exit 1" 0 1 2 3 15

SEC_MECH_LOC=`iigetres ii.$CONFIG_HOST.gcf.mechanism_location`
if [ -n "$SEC_MECH_LOC" ] ; then 
if [ x"$conf_BUILD_ARCH_32_64" = x"TRUE" ] ; then
$DOIT iisetres -v "ii.$CONFIG_HOST.gcf.mechanism_location_lp32" "$SEC_MECH_LOC"
$DOIT iiremres "ii.$CONFIG_HOST.gcf.mechanism_location"
fi
fi

do_setup()
{
echo "Setting up (PROD1NAME) Networking..."

trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
2>/dev/null; exit 1" 0 1 2 3 15

# create the response file if it does not exist.
check_response_file #check for response files.

(PROG1PRFX)sulock "(PROD1NAME) Networking setup" || exit 1
	
touch_files # make sure symbol.tbl and config.dat exists

if [ "$WRITE_RESPONSE" = "true" ] ; then
     mkresponse_msg
else ## **** Skip everything that we do in WRITE_RESPONSE mode ****

if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
# create backup of original config.dat - if config.nfs already exists
# we bombed out of an earlier NFS client creation so restore the
# original config.dat and keep the nfs backup
   if [ -f ${cfgloc}/config.nfs ] ; then
      cp -p ${cfgloc}/config.nfs ${cfgloc}/config.dat
   else
      cp -p ${cfgloc}/config.dat ${cfgloc}/config.nfs
   fi
fi

# grant owner and root all privileges
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.privileges.user.root \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.privileges.user.$(PROG5PRFX)_USER \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

fi #end WRITE_RESPONSE mode

if [ -f $(PRODLOC)/(PROD2NAME)/install/release.dat ]; then
   VERSION=`$(PRODLOC)/(PROD2NAME)/install/(PROG2PRFX)build -version=net` ||
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


if [ "$WRITE_RESPONSE" = "false" ] ; then

RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

The $VERSION version of (PROD1NAME) Networking has already been
set up on "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
}

#
# The following will determine what type of installation (Server,
# NFS-client or non-NFS-client) we are dealing with. 
#
CONFIG_SERVER_HOST=`(PROG1PRFX)getres (PROG4PRFX)."*".config.server_host`

SERVER_HOST=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_SERVER_HOST.gcn.local_vnode`

fi # end of WRITE_RESPONSE mode 

SERVER_SETUP=false
NONNFS_CLIENT_SETUP=false
if [ "$WRITE_RESPONSE" = "true" ] ; then
    cat << !

You can configure the response file to set up networking either 
    1) In a DBMS server installation. 
or  2) In a non NFS client installation.
For more detail about setting up netwoking refer to the Installation Guide.

!
if  prompt "Do you wish to configure response file to set up networking \n in a DBMS server installation?" y 
then
    SERVER_SETUP=true
    cat << !

This procedure will set up networking for Server Installation.
!
else
    NONNFS_CLIENT_SETUP=true
    cat << !

This procedure will set up networking for non-NFS client 
!
fi
cat << !

Note: The -mkresponse option does not allow you to specify an 
installation password during the setup procedure nor allow authorizing 
yourself (the installation owner) or all "$HOSTNAME" users
for access to the default server. You must specify this later, using the 
Ingres Network Utility or Visual DBA (see the System Administrator Guide) 
or the netutil utility (see the Command Reference Guide).

!
prompt "Do you want to continue this setup procedure?" y || exit 1
elif  [ "$SERVER_HOST" ] && [ "$SERVER_HOST" = `iinethost -local` ] ; then
    SERVER_SETUP=true
elif $CLUSTERSUPPORT ; then
    CLUSTER_ID="`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.config.cluster.id`"
    case "$CLUSTER_ID" in
    [1-9]|[12][0-9]|3[0-2])	# Setup for cluster
	SERVER_SETUP=true
	;;
    *)				# normal non-server setup.
	CLUSTER_ID=""
	SERVER_SETUP=false
	;;
    esac
else
    SERVER_SETUP=false
fi

# if we are generating a response file for server installation or
# serverhost is set and serverhost is not changed for nfs installation
#
if [ "$SERVER_SETUP" = "true" ] ; then
   # configuring (PROD1NAME) Networking on a server

  if [ "$WRITE_RESPONSE" = "false" ] ; then

   SERVER_SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_SERVER_HOST.config.dbms.$RELEASE_ID`
   # Never bail here for RPM installs. Sometimes iisunet gets run first
   [ "$SERVER_SETUP" != "complete" ] && [ ! "$RPM" ] &&
   {
      cat << !

The setup procedure for the following version of the (PROD1NAME) Intelligent
DBMS:

	$VERSION

must be completed up before (PROD1NAME) Networking can be set up on this host:

	$HOSTNAME

!
      exit 1
   }

   [ -z "$(PROG3PRFX)_INSTALLATION" ] && (PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
   [ -z "$(PROG3PRFX)_INSTALLATION" ] &&
   {
      cat << ! 

(PROG3PRFX)_INSTALLATION is not set!  Setup aborted.

!
      exit 1
   }
fi # end WRITE_RESPONSE
   cat << ! 

This procedure will set up the following version of (PROD1NAME) Networking:

	$VERSION

to run on local host:

	$HOSTNAME

The (PROD1NAME) Intelligent DBMS has been set up on this host; therefore,
this installation has the capacity to act as an (PROD1NAME) "server", which
means it can service queries against local (PROD1NAME) databases.  An
(PROD1NAME) server can also service remote queries if (PROD1NAME) Networking
is installed.

If you do not need access to this (PROD1NAME) server from other hosts,
then you do not need to set up (PROD1NAME) Networking.

!


   $BATCH || prompt "Do you want to continue this setup procedure?" y \
      || exit 1

  if [ "$WRITE_RESPONSE" = "false" ] ; then
   if [ "$SETUP" != "defaults" ] ; then
      # generate default configuration resources
      echo ""
      echo "Generating default configuration..."
      NET_RFM_FILE=$(PROG3PRFX)_CONFIG/net.rfm

      if (PROG1PRFX)genres $CONFIG_HOST $NET_RFM_FILE ; then
         (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID defaults
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
   fi #end SETUP !=defaults

   # display (PROG3PRFX)_INSTALLATION 
   cat << !
   
(PROG3PRFX)_INSTALLATION configured as $(PROG3PRFX)_INSTALLATION. 

!
  fi #end of WRITE_RESPONSE mode
else
   # configuring (PROD1NAME) Networking on a client

   if [ "$SERVER_HOST" ] ; then
      # NFS-client setup

      NFS_CLIENT=true

  if [ "$WRITE_RESPONSE" = "false" ] ; then

      # get server's installation code 
      SERVER_ID=`eval (PROG3PRFX)_ADMIN=${symbloc} \
         (PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`

      # determine whether (PROD1NAME) Networking is set up locally  
      SERVER_NET_SETUP=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_SERVER_HOST.config.net.$RELEASE_ID`

      [ "$SERVER_NET_SETUP" != "complete" ] &&
      {
         cat << !

The setup procedure for the following version of (PROD1NAME) Networking:

	$VERSION

must be completed for the (PROD1NAME) server installation "$SERVER_ID" on
host:

	$SERVER_HOST

before any (PROD1NAME) NFS-clients can be set up. 

!
         exit 1
      }

      cat << !

This procedure will set up the following version of (PROD1NAME) Networking:

	$VERSION

to run on host:

	$HOSTNAME

This procedure will set up an (PROD1NAME) NFS-client which will enable
users on "$HOSTNAME" to access databases stored in installation "$SERVER_ID" on
host:

	$SERVER_HOST

An (PROD1NAME) NFS-client installation uses Network File System services
to share executables and other files with an (PROD1NAME) server installation.

!
      $BATCH || prompt "Do you want to continue this setup procedure?" y \
         || exit 1

  fi # end of WRITE_RESPONSE

      if [ "$HOSTNAME" != "$ACTUAL_HOSTNAME" ] ; then
         # remote NFS-client setup, so prompt for remote (mounted) (PRODLOC)

         SERVER_PASSWD=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_SERVER_HOST.setup.passwd.$RELEASE_ID`
         cat << !

In order to set up an (PROD1NAME) NFS-client remotely (i.e.  while running
on the local server "$SERVER_HOST"), you must know the remote path name
which will be used to NFS-mount the local (PROD1NAME) directory tree.

If you do not know the correct value for (PRODLOC) (i.e. as it will
appear "$HOSTNAME") do not attempt to complete this setup procedure.

!
         if [ "$SERVER_PASSWD" = "on" ] || [ "$SERVER_PASSWD" = "ON" ] ; then 
            cat << !
Also note that although an (PROD1NAME) installation password was created
for this server during the local setup procedure, you cannot create 
an installation password authorization for any users on the remote
host during this procedure, since (PROD1NAME) Networking authorization
information can only be administered on the remote host.

If you want to create an installation password authorization for users
on the remote host during this procedure, you must run this program
on the client system.

!
         fi

         $BATCH || prompt "Do you want to continue this setup procedure?" y || exit 1

         NOT_DONE=true
         while $NOT_DONE ; do
	    $BATCH && NOT_DONE=false
            echo ""
            $BATCH || echo "Enter the value of (PRODLOC) as it will appear on $HOSTNAME:"
            $BATCH || read SERVER_(PRODLOC) junk
            cat << !

The remainder of this procedure assumes that the following value of
(PRODLOC) on $HOSTNAME is correct:

	$SERVER_(PRODLOC)

Make sure that this is the correct path name, since the resulting
installation will be unusable otherwise. 

!
            $BATCH || prompt "Is this the path name correct?" y && NOT_DONE=false 
         done
      else
         SERVER_(PRODLOC)=$(PRODLOC)
      fi

      if [ "$WRITE_RESPONSE" = "true" ] ; then
	    check_response_write NET_SERVER_(PRODLOC) $SERVER_(PRODLOC) replace
      fi

      if [ "$READ_RESPONSE" = "true" ] ; then
            RESVAL=`iiread_response SERVER_(PRODLOC) $RESINFILE`
            if [ ! -z "$RESVAL" ] ; then
                SERVER_(PRODLOC)=$RESVAL      
            fi
      fi

  if [ "$WRITE_RESPONSE" = "false" ] ; then

      # create the admin directory for this client
      ADMIN_ROOT=$(PRODLOC)/(PROD2NAME)/admin
      CLIENT_ADMIN=$ADMIN_ROOT/$HOSTNAME
   
      [ ! -d $ADMIN_ROOT ] &&
      {
         mkdir $ADMIN_ROOT
         chmod 755 $ADMIN_ROOT 
      }
   
      [ ! -d $CLIENT_ADMIN ] &&
      {
         mkdir $CLIENT_ADMIN
         chmod 755 $CLIENT_ADMIN
      }
  fi #end of WRITE_RESPONSE mode

      # create symbol.tbl if it doesn't exist
      [ -f $CLIENT_ADMIN/symbol.tbl ] ||
      {
         touch $CLIENT_ADMIN/symbol.tbl ||
         {
            cat << !

Unable to create $CLIENT_ADMIN/symbol.tbl.  Exiting...

!
            exit 1
         }
      }

  if [ "$WRITE_RESPONSE" = "false" ] ; then

      # (PROG3PRFX)_CONFIG _must_ be set in the client's symbol.tbl
      (PROG3PRFX)_CONFIG=`(PROG2PRFX)prenv (PROG3PRFX)_CONFIG`
      if [ -z "$(PROG3PRFX)_CONFIG" ] || [ "$HOSTNAME" != "$ACTUAL_HOSTNAME" ] ; then
         eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG3PRFX)_CONFIG \
            $SERVER_(PRODLOC)/(PROD2NAME)/files
      fi

      # reset (PROG3PRFX)_CONFIG in case it was unset
      (PROG3PRFX)_CONFIG=`(PROG2PRFX)prenv (PROG3PRFX)_CONFIG`


      if [ "$SETUP" != defaults ] ; then
         cat << !

Generating default configuration...
!
	 # Bug 78211 - if server is SQL-92, set client to be as well

         SQL_92=`(PROG1PRFX)getres (PROG4PRFX).$SERVER_HOST.fixed_prefs.iso_entry_sql-92`
         if [ "$SQL_92" = "ON" ] ; then
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.fixed_prefs.iso_entry_sql-92 ON
         fi

	NET_RFM_FILE=$(PROG3PRFX)_CONFIG/net.rfm

	if (PROG1PRFX)genres $CONFIG_HOST $NET_RFM_FILE ; then
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID defaults
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
  fi #end of WRITE_RESPONSE mode

      # get (PROG3PRFX)_INSTALLATION 
      (PROG3PRFX)_INSTALLATION=`eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
      if [ "$(PROG3PRFX)_INSTALLATION" ]
      then 
         cat << !
      
(PROG3PRFX)_INSTALLATION configured as $(PROG3PRFX)_INSTALLATION. 
!
      else
         DEFAULT=$SERVER_ID
         if $BATCH ; then
            if [ "$READ_RESPONSE" = "true" ] ; then
	       (PROG3PRFX)_INSTALLATION=`iiread_response (PROG3PRFX)_INSTALLATION $RESINFILE` 
	       if [ -z "$(PROG3PRFX)_INSTALLATION" ] ; then
		  (PROG3PRFX)_INSTALLATION=$DEFAULT
	       fi	
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
               (PROG3PRFX)_INSTALLATION=$DEFAULT
            fi
         else
            get_installation_code 
         fi
      fi

      if [ "$WRITE_RESPONSE" = "true" ] ; then
            eval check_response_write (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION replace
      else

      # get other defaults from server symbol.tbl 
      (PROG3PRFX)_TEMPORARY=`eval (PROG3PRFX)_ADMIN=${symbloc} \
         (PROG2PRFX)prenv (PROG3PRFX)_TEMPORARY`
      (PROG5PRFX)_EDIT=`eval (PROG3PRFX)_ADMIN=${symbloc} (PROG2PRFX)prenv (PROG5PRFX)_EDIT`

      SERVER_VNODE=$SERVER_HOST

      # set gcn remote_vnode resource
      (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.remote_vnode $SERVER_VNODE
	
      fi #end of WRITE_RESPONSE mode
   else # non-NFS-client setup 
      NFS_CLIENT=false
      
      cat << !

This procedure will set up the following version (PROD1NAME) Networking:

	$VERSION

to run on local host:

	$HOSTNAME

This procedure will set up a non-NFS (PROD1NAME) client installation which
will allow you to access (remote or local) (PROD1NAME) servers, without
using Network File System services to share executables and other files
with the server installation.

!
      $BATCH || prompt "Do you want to continue this setup procedure?" y \
         || exit 1

$NONNFS_CLIENT_SETUP  ||
{
      cat << !

After you set up (PROD1NAME) Networking in this installation, you will have the
capability to establish connections to (PROD1NAME) server(s).  In order
to connect to an (PROD1NAME) server, (PROD1NAME) Networking must be set up on
the server(s) and the necessary (PROD1NAME) Networking authorizations must
be created. 

!
      cat << !
As part of this procedure, you will be able to create an (PROD1NAME) Networking
authorization which allows you (the installation owner) or all "$HOSTNAME"
users to access a default server.

!
      $BATCH || prompt "Do you want to continue this setup procedure?" y \
         || exit 1
}

#check for write_response if set skip
    if [ "$WRITE_RESPONSE" = "false" ] ; then

      if [ "$SETUP" != "defaults" ] ; then
         # generate default configuration resources
         cat << !

Generating default configuration...
!
        NET_RFM_FILE=$(PROG3PRFX)_CONFIG/net.rfm

        if (PROG1PRFX)genres $CONFIG_HOST $NET_RFM_FILE ; then
            (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID defaults
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
    fi # end WRITE_RESPONSE mode

      # get (PROG3PRFX)_INSTALLATION 
      (PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
      if [ "$(PROG3PRFX)_INSTALLATION" ] ; then 
         cat << !
      
(PROG3PRFX)_INSTALLATION configured as $(PROG3PRFX)_INSTALLATION. 
!
      else
         if $BATCH ; then
	    if [ "$READ_RESPONSE" = "true" ] ; then
	       (PROG3PRFX)_INSTALLATION=`iiread_response (PROG3PRFX)_INSTALLATION $RESINFILE` 
	       [ -z "$(PROG3PRFX)_INSTALLATION" ] && (PROG3PRFX)_INSTALLATION="(PROG3PRFX)"
            elif [ "$2" ] ; then
               (PROG3PRFX)_INSTALLATION="$2"
            else
               (PROG3PRFX)_INSTALLATION="(PROG3PRFX)"
            fi
         else
            get_installation_code 
         fi
      fi
   fi

   if [ "$WRITE_RESPONSE" = "true" ] ; then
	eval check_response_write (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION replace
   fi 
   # now doing stuff to both NFS and non-NFS-clients

   # get server's character set code
   VALUE=`eval (PROG3PRFX)_ADMIN=${symbloc} \
            (PROG2PRFX)prenv (PROG3PRFX)_CHARSET$SERVER_ID`
   if [ "$VALUE" ] && [ "$WRITE_RESPONSE" = "false" ] ; then
       # set (PROG3PRFX)_CHARSETxx to same value as server
       if [ "$CLIENT_ADMIN" ] ; then
          eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG3PRFX)_CHARSET$(PROG3PRFX)_INSTALLATION \
                               \"$VALUE\"
       else
          (PROG2PRFX)setenv (PROG3PRFX)_CHARSET$(PROG3PRFX)_INSTALLATION $VALUE
       fi
       cat << !
 
(PROG3PRFX)_CHARSET$(PROG3PRFX)_INSTALLATION configured as $VALUE.
!
   else # setting character code
       if $BATCH ; then
	  # batch mode default
	  VALUE=ISO88591
	  if [ "$READ_RESPONSE" = "true" ] ; then
             RSP_VALUE=`iiread_response (PROG3PRFX)_CHARSET $RESINFILE`
             if [ ! -z "$RSP_VALUE" ] ; then
               if validate_resource ii."*".setup.ii_charset $RSP_VALUE
               then
                  cat << !
  
Characterset supplied ( $RSP_VALUE ) is illegal.
Character set will be set to default [ $VALUE ]
  
!
               else
                  VALUE=$RSP_VALUE
               fi
             fi
          fi #end READ_RESPONSE
       else
	  cat << !
     
(PROD1NAME) supports different character sets.  You must now enter the
character set you want to use with this (PROD1NAME) installation.
 
IMPORTANT NOTE: You will be unable to change character sets once you
make your selection.  If you are unsure of which character set to use,
exit this program and refer to the (PROD1NAME) Installation Guide.
!
	  # display valid entries
	  (PROG1PRFX)valres -v (PROG4PRFX)."*".setup.(PROG4PRFX)_charset BOGUS_CHARSET
	  DEFAULT="ISO88591"
	  (PROG1PRFX)echonn "Please enter a valid character set [$DEFAULT] "
	  read VALUE junk
	  [ -z "$VALUE" ] && VALUE=$DEFAULT
	  NOT_DONE=true
	  while $NOT_DONE ; do
	     if [ -z "$VALUE" ] ; then
		VALUE=$DEFAULT
	     elif validate_resource (PROG4PRFX)."*".setup.(PROG4PRFX)_charset $VALUE
	     then
		# reprompt
		(PROG1PRFX)echonn "Please enter a valid character set [$DEFAULT] "
		read VALUE junk
	     else
		CHARSET_TEXT=`(PROG1PRFX)getres (PROG4PRFX)."*".setup.charset.$VALUE`
		cat << !
     
The character set you have selected is:
     
	$VALUE ($CHARSET_TEXT)
     
!

		$BATCH || if prompt "Is this the character set you want to use?" y
		then
		   NOT_DONE=false
		else
		   cat << !
 
Please select another character set.
!
		   (PROG1PRFX)valres -v (PROG4PRFX)."*".setup.(PROG4PRFX)_charset BOGUS_CHARSET
		   (PROG1PRFX)echonn "Please enter a valid character set [$DEFAULT] "
		   read VALUE junk
		fi
	     fi
	  done
       fi #end batch.

     if [ "$WRITE_RESPONSE" = "true" ] ; then
	eval check_response_write (PROG3PRFX)_CHARSET${(PROG3PRFX)_INSTALLATION} $VALUE replace	
     else
       if [ "$CLIENT_ADMIN" ] ; then
          eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG3PRFX)_CHARSET$(PROG3PRFX)_INSTALLATION \
                               \"$VALUE\"
       else
          (PROG2PRFX)setenv (PROG3PRFX)_CHARSET$(PROG3PRFX)_INSTALLATION $VALUE
       fi
     fi #end WRITE_RESPONSE mode
   fi #setting character code

   if [ "$WRITE_RESPONSE" = "false" ] ; then
   # get defaults from configuration rules
   for SYMBOL in (PROG3PRFX)_TEMPORARY (PROG5PRFX)_EDIT 
   do
      DEFAULT=`(PROG1PRFX)getres (PROG4PRFX)."*".setup.$SYMBOL`
      VALUE=`(PROG2PRFX)prenv $SYMBOL`
      if [ -z "$VALUE" ] ; then
         if [ "$DEFAULT" ] && [ -z "$VALUE" ] ; then 
            eval "$SYMBOL=`eval echo $DEFAULT`"
         else
            cat << !

ERROR: $SYMBOL not defaulted in rule base.

!
            exit 1
         fi
      else
         eval "$SYMBOL=`eval echo $VALUE`"
      fi
   done
   fi #end WRITE_RESPONSE mode

   # get (PROG3PRFX)_TIMEZONE_NAME value
   (PROG3PRFX)_TIMEZONE_NAME=`eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)prenv (PROG3PRFX)_TIMEZONE_NAME`
   if [ "$(PROG3PRFX)_TIMEZONE_NAME" ] ; then 
      cat << !

(PROG3PRFX)_TIMEZONE_NAME configured as $(PROG3PRFX)_TIMEZONE_NAME. 
   
!
   else
      TZ_RULE_MAP=$(PROG3PRFX)_CONFIG/net.rfm
      if $BATCH ; then
	 if [ "$READ_RESPONSE" = "true" ] ; then
            (PROG3PRFX)_TIMEZONE_NAME=`iiread_response (PROG3PRFX)_TIMEZONE_NAME $RESINFILE`
         fi
         [ -z "$(PROG3PRFX)_TIMEZONE_NAME" ] && 
		(PROG3PRFX)_TIMEZONE_NAME=`eval (PROG3PRFX)_ADMIN=${symbloc} \
			(PROG2PRFX)prenv (PROG3PRFX)_TIMEZONE_NAME`
         #If we still cannot set II_TIMEZONE then this should be set to NA-PACIFIC 
         [ -z "$(PROG3PRFX)_TIMEZONE_NAME" ] && (PROG3PRFX)_TIMEZONE_NAME="NA-PACIFIC"
      else
         get_timezone 
         echo ""
      fi
   fi

   if [ "$WRITE_RESPONSE" = "true" ] ; then
	eval check_response_write (PROG3PRFX)_TIMEZONE_NAME $(PROG3PRFX)_TIMEZONE_NAME replace
   else 
   # set miscellaneous symbol
   eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG3PRFX)_TIMEZONE_NAME $(PROG3PRFX)_TIMEZONE_NAME
   eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG3PRFX)_INSTALLATION $(PROG3PRFX)_INSTALLATION
   eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG3PRFX)_TEMPORARY $(PROG3PRFX)_TEMPORARY
   eval (PROG3PRFX)_ADMIN=$CLIENT_ADMIN (PROG2PRFX)setenv (PROG5PRFX)_EDIT $(PROG5PRFX)_EDIT 
   fi #end WRITE_RESPONSE mode
fi

#
# Issue:137354, Bug:122277. The "local_vnode" gets updated during upgrade. 
# While researching on this issue it became apparent that iisunet script 
# gets executed both during fresh install and during upgrade and it
# doesn't have the capablity to detect upgrade.
# This change addresses just the problem stated in the issue.
#
# Don't set gcn local_vnode resource explicitly. Use the default local_vnode
# set in crs grammar (all.crs). This all.crs file is included in all of the
# "*.rfm" files.

if [ "$WRITE_RESPONSE" = "false" ] ; then

echo "Configuring Net server listen addresses..."

# set default Net server listen addresses
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".async.port ""
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".decnet.port (PROG3PRFX)_GCC${(PROG3PRFX)_INSTALLATION}
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".iso_oslan.port OSLAN_${(PROG3PRFX)_INSTALLATION}
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".iso_x25.port X25_${(PROG3PRFX)_INSTALLATION}
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".sna_lu62.port "<none>"
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".sockets.port $(PROG3PRFX)_INSTALLATION
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".spx.port $(PROG3PRFX)_INSTALLATION
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".tcp_ip.port $(PROG3PRFX)_INSTALLATION
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcc."*".tcp_wol.port $(PROG3PRFX)_INSTALLATION

fi #end of WRITE_RESPONSE

[ "$HOSTNAME" != "$ACTUAL_HOSTNAME" ] &&
{
   #
   # since we are setting up an NFS-client remotely (from the server), we
   # can't allow the user to enter an installation password.
   #

if [ "$WRITE_RESPONSE" = "false" ] ; then
   (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID complete
   remove_temp_resources
fi # end WRITE_RESPONSE mode

   cat << !

You must use the "(PROG0PRFX)netutil" program to authorize users of installation $(PROG4PRFX)_INSTALLATION
on host:

	$HOSTNAME

before they have access to local databases.  Refer to the (PROD1NAME) Connectivity
Guide for information on using "(PROG0PRFX)netutil".

Note that "(PROG0PRFX)netutil" must be run on $HOSTNAME to authorize $HOSTNAME users.

(PROD1NAME) Networking has been successfully set up in this installation.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) client.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!

if [ "$WRITE_RESPONSE" = "false" ] ; then
   if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
      cp -p ${cfgloc}/config.dat $CLIENT_ADMIN/config.dat
      cp -p $CLIENT_ADMIN/config.dat $CLIENT_ADMIN/config.$CONFIG_HOST
      mv -f ${cfgloc}/config.nfs \
            ${cfgloc}/config.dat
   fi
fi #end of WRITE_RESPONSE mode
   $BATCH || pause
   trap : 0
   clean_exit
} # setting NFS client remotely.

# now do installation password setup
if [ "$SERVER_HOST" ] && [ "$SERVER_HOST" = "$HOSTNAME" ] && \
   [ -z "$NFS_CLIENT" ]
then
   # this is a server
   cat << !

Users of other (PROD1NAME) installations must be authorized to connect
to this server installation.  For a user to be authorized, correct
authorization information must be set up locally and on remote clients.
You can authorize remote users for access to this server using
installation passwords (in addition to (PROD1NAME) Networking user
passwords supported in past releases).

Installation passwords offer the following advantages over user passwords:

+ Remote users do not need login accounts on the server host.

+ Installation passwords are independent of host login passwords. 

+ Installation passwords are not transmitted over the network in any
  form, thus providing greater security than user passwords.

+ User identity is always preserved.

If you need more information about (PROD1NAME) Networking authorization,
please refer to the (PROD1NAME) Connectivity Guide.

!

$NONNFS_CLIENT_SETUP   ||
{
  $BATCH || pause
   cat << !

You now have the option of creating an installation password for this
server.  If you choose not to set up an installation password on this
server, then you must use the "(PROG0PRFX)netutil" program (as described in the
(PROD1NAME) Connectivity Guide) to authorize remote users for access
to this server.

Be aware that simply creating an installation password on this server
does not automatically authorize remote users for access.  To authorize
a remote user, a valid password must be entered into the (PROD1NAME)
Networking authorization profile for that user on the client.  You can
authorize remote users in either of the following ways:

o Use "(PROG0PRFX)netutil" as described in the (PROD1NAME) Connectivity Guide.

o Select the installation password authorization option (only valid if
  your network supports TCP/IP) during the setup procedure for (PROD1NAME)
  Networking on the client.  Note: this option is not available when 
  client setup is performed remotely over NFS.

!
}
   if $NOTBATCH && prompt \
      "Do you want to create an installation password for this server?"
   then
      if [ "$WRITE_RESPONSE" = "false" ] ; then
      cat << ! 
y

Starting the (PROD1NAME) name server for password creation... 
!
      # start the name server in order to run (PROG0PRFX)netutil
      (PROG1PRFX)run (PROG1PRFX)gcn >/dev/null ||
      {
         cat << !

The name server failed to start.  See the (PROD1NAME) error log:

	$II_LOG/errlog.log

for a description of the error.  You must correct the problem and re-run
$SELF.

!
         exit 1
      }
      cat << ! 

Note that installation passwords must be unique in the first eight characters.

!
      # turn off local echo for password entry
      stty -echo
      NOT_DONE=true
      while $NOT_DONE ; do
         (PROG1PRFX)echonn "Enter installation password: "
         read PASSWORD junk
         echo ""
         (PROG1PRFX)echonn "Retype password: "
         read VERIFY_PASSWORD junk
         echo ""
         if [ "$PASSWORD" != "$VERIFY_PASSWORD" ] ; then
            cat << !

Password mismatch.  Please try again. 

!
         else
            NOT_DONE=false
         fi
      done
      stty echo

      echo "C G login $HOSTNAME * $PASSWORD" >/dev/null
      if (PROG0PRFX)netutil -file- << !
C G login $HOSTNAME * $PASSWORD
!
      then
         cat << !

Installation password created.  The name server has been shut down.
!
         (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.setup.passwd.$RELEASE_ID on
      else
         cat << !
Unable to create installation password due to "(PROG0PRFX)netutil" failure.

!
         (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.setup.passwd.$RELEASE_ID off 
      fi
      # stop the name server
      (PROG1PRFX)namu << ! >/dev/null
stop
y
q
!
      fi #end WRITE_RESPONSE mode
   else # batch
      cat << !
n
Installation password not configured
!
      (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.setup.passwd.$RELEASE_ID off
   fi # end nobatch
   if [ "$WRITE_RESPONSE" = "false" ] ; then
      (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID complete
      remove_temp_resources
      cat << !

(PROD1NAME) Networking has been successfully set up in this installation.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) server.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!
      if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
         cp -p ${cfgloc}/config.dat $CLIENT_ADMIN/config.dat
         cp -p $CLIENT_ADMIN/config.dat $CLIENT_ADMIN/config.$CONFIG_HOST
         mv -f ${cfgloc}/config.nfs ${cfgloc}/config.dat
      fi
   fi # end of WRITE_RESPONSE
   $BATCH || pause
   trap : 0
   clean_exit
fi # out of the loop

# if here, we are setting up a client of some sort
if $NFS_CLIENT ; then
   SERVER_(PROG3PRFX)_INSTALLATION=`eval (PROG3PRFX)_ADMIN=${symbloc} \
      (PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
   SERVER_TCP_IP_PORT=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_SERVER_HOST.gcc."*".tcp_ip.port`
   SERVER_PASSWD=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_SERVER_HOST.setup.passwd.$RELEASE_ID`


   if [ "$SERVER_PASSWD" = "on" ] || [ "$SERVER_PASSWD" = "ON" ] ; then
      # installation password created on server
      cat << !

An installation password was created for the default server (installation
"$SERVER_(PROG3PRFX)_INSTALLATION" on host "$SERVER_HOST") during the (PROD1NAME)
Networking setup procedure on that host.

If your network supports the TCP/IP protocol and you know the installation
password which was created on the server, then you have the option of
authorizing yourself (the installation owner) or all "$HOSTNAME" users
for access to the default server.

!
      if $NOTBATCH && \
         prompt "Do you want to authorize any users at this time?" y
      then
         echo "" 
         if prompt "Do you want to authorize all users on \"$HOSTNAME\"?" y ; then
            # (PROG0PRFX)netutil authorization entry will be (G)lobal
            AUTH_TYPE=G
         else
            # (PROG0PRFX)netutil authorization entry will be (P)rivate
            AUTH_TYPE=P
            cat << !

This procedure will only authorize the installation owner ($(PROG5PRFX)_USER) to
access the server.
!
         fi
      else #batch
         if [ "$WRITE_RESPONSE" = "false" ] ; then
         (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID complete
         remove_temp_resources
         fi
         cat << !

(PROD1NAME) Networking has been successfully set up in this installation.
Refer to the (PROD1NAME) Connectivity Guide for instructions on how to use
the "(PROG0PRFX)netutil" program to authorize local users for access to remote
(PROD1NAME) servers.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) client.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!
         if [ "$WRITE_RESPONSE" = "false" ] ; then
         if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
            cp -p ${cfgloc}/config.dat $CLIENT_ADMIN/config.dat
            cp -p $CLIENT_ADMIN/config.dat $CLIENT_ADMIN/config.$CONFIG_HOST
            mv -f ${cfgloc}/config.nfs ${cfgloc}/config.dat
         fi
         fi # end WRITE_RESPONSE flag.
         $BATCH || pause
         trap : 0
         clean_exit
      fi
   else
      # no installation password created on server

      if [ "$WRITE_RESPONSE" = "false" ] ; then
      (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID complete
      remove_temp_resources
      fi #end of WRITE_RESPONSE flag

      cat << !

(PROD1NAME) Networking has been successfully set up for this installation.
Refer to the (PROD1NAME) Connectivity Guide for instructions on how to use
the "(PROG0PRFX)netutil" program to authorize local users for access to remote
(PROD1NAME) servers.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) client.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!
      if [ "$WRITE_RESPONSE" = "false" ] ; then
      if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
         cp -p ${cfgloc}/config.dat $CLIENT_ADMIN/config.dat
         cp -p $CLIENT_ADMIN/config.dat $CLIENT_ADMIN/config.$CONFIG_HOST
         mv -f ${cfgloc}/config.nfs ${cfgloc}/config.dat
      fi
      fi #end of WRITE_RESPONSE flag 
      $BATCH || pause
      trap : 0
      clean_exit
   fi
else
   # non-NFS-client installation
   (PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`
   DEFAULT_SERVER_TCP_IP_PORT=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.gcc."*".tcp_ip.port`

if [ "$WRITE_RESPONSE" = "false" ] ; then
   # get installation code of default server
   cat << ! 

You can now create a local authorization entry either for yourself
(the installation owner) or for all users on "$HOSTNAME".  A local
authorization will enable you to access an (PROD1NAME) server
installation over TCP/IP.

To create an authorization you will need the following information:

o The name of the host on which the (PROD1NAME) server resides.

o A valid (PROD1NAME) Networking TCP/IP listen address for the server.
  (The default TCP/IP listen address is the value of (PROG3PRFX)_INSTALLATION
  on the server). 

o A valid installation password for the server.

!
   if $NOTBATCH && \
      prompt "Do you want to create an authorization entry?" y
   then
      echo "" 
      if prompt "Do you want to authorize all users on \"$HOSTNAME\"?" y ; then
         # (PROG0PRFX)netutil authorization entry will be (G)lobal
         AUTH_TYPE=G
      else
         # (PROG0PRFX)netutil authorization entry will be (P)rivate
         AUTH_TYPE=P
         cat << !

This procedure will only authorize the installation owner ($(PROG5PRFX)_USER) to
access the server.
!
      fi

      # prompt for default server host, ping to verify
      NOT_DONE=true
      while $NOT_DONE ; do
         echo ""
         (PROG1PRFX)echonn "Enter the name of the server host machine: "  
         read SERVER_HOST junk
         if [ "$SERVER_HOST" = "$HOSTNAME" ] ; then 
         cat << !

The host name you entered, '$SERVER_HOST', is the same as your local 
host name.  The installation password you specify will be written to the 
netutil database with the vnode name '${SERVER_HOST}_server', and the 
Name Server configuration entry 'gcn.remote_vnode' will be added as 
'${SERVER_HOST}_server'.

You will still need to define the installation password separately for your 
local host with the vnode name '$SERVER_HOST' in order for your local host
to support installation passwords, if required.

!
         fi

         # ping should probably go into (PROG1PRFX)sysdep
         if $PINGCMD $SERVER_HOST $PINGARGS 1>/dev/null 2>/dev/null ; then
            NOT_DONE=false
         else
            cat << !

The host you have specified does not respond. 

!
            prompt "Do you want to enter another host?" y || \
               NOT_DONE=false 
         fi

         # make up different remote_vnode if same as local host 
         if [ "$SERVER_HOST" = "$HOSTNAME" ] ; then 
            SERVER_VNODE=${SERVER_HOST}_server
         else
            SERVER_VNODE=$SERVER_HOST
         fi
      done

      # set gcn remote_vnode resource
      (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.gcn.remote_vnode $SERVER_VNODE

      # prompt for the TCP/IP listen address
      echo ""
      (PROG1PRFX)echonn "Please enter the server's TCP/IP listen address: "
      read SERVER_TCP_IP_PORT junk
   else  # else the followis is for batch mode 
        (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID complete
        remove_temp_resources
      cat << !

(PROD1NAME) Networking has been successfully set up for this installation.
Refer to the (PROD1NAME) Connectivity Guide for instructions on how to use
the "(PROG0PRFX)netutil" program to authorize local users for access to remote
(PROD1NAME) servers.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) client.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!
      if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
         cp -p ${cfgloc}/config.dat $CLIENT_ADMIN/config.dat
         cp -p $CLIENT_ADMIN/config.dat $CLIENT_ADMIN/config.$CONFIG_HOST
         mv -f ${cfgloc}/config.nfs ${cfgloc}/config.dat
      fi
      $BATCH || pause
      trap : 0
      clean_exit
   fi #end not-batch
 fi #end of WRITE_RESPONSE mode
fi #end of non-NFS-clients 

#
# this part is the same for NFS and non-NFS-clients
# For read / write response mode we do none of this.
# use netutil etc to accomplish this task.
#
if [ "$WRITE_RESPONSE" = "false" ] ; then
cat << !

Starting the (PROD1NAME) name server for password entry... 

!
# start the name server in order to run (PROG0PRFX)netutil
(PROG1PRFX)run (PROG1PRFX)gcn ||
{
   cat << !
The name server failed to start.  See the (PROD1NAME) error log:

	$II_LOG/errlog.log

for a description of the error.  You must correct the problem and re-run
$SELF.

!
   exit 1
}

# prompt for password with local echo off
stty -echo
NOT_DONE=true
while $NOT_DONE ; do
   (PROG1PRFX)echonn "Enter the installation password created on the server: "
   read PASSWORD junk
   echo ""
   (PROG1PRFX)echonn "Retype password: "
   read VERIFY_PASSWORD junk
   echo ""
   if [ "$PASSWORD" != "$VERIFY_PASSWORD" ] ; then
      cat << !

Password mismatch.  Please try again. 

!
   else
      NOT_DONE=false
   fi
done

# run (PROG0PRFX)netutil to enter authorization information
stty echo
if (PROG0PRFX)netutil -file- << !
C G C $SERVER_VNODE $SERVER_HOST tcp_ip $SERVER_TCP_IP_PORT 
C $AUTH_TYPE login $SERVER_VNODE * $PASSWORD 
!
then
   cat << !

Remote authorization created.  The name server has been shut down.

!
   (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.setup.passwd.$RELEASE_ID on
else
   cat << !
Unable to create installation password due to (PROG0PRFX)netutil failure. 

!
   (PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.setup.passwd.$RELEASE_ID off
fi

# run (PROG1PRFX)namu to stop the name server
(PROG1PRFX)namu << ! >/dev/null
stop
y
q
!
(PROG1PRFX)setres (PROG4PRFX).$CONFIG_HOST.config.net.$RELEASE_ID complete
remove_temp_resources
cat << !
(PROD1NAME) Networking has been successfully set up in this installation.

You can now use the "(PROG2PRFX)start" command to start your (PROD1NAME) client.
Refer to the (PROD1NAME) Installation Guide for more information about
starting and using (PROD1NAME).

!
   if [ "$(PROG3PRFX)_CONFIG_LOCAL" ] ; then
      cp -p ${cfgloc}/config.dat $CLIENT_ADMIN/config.dat
      cp -p $CLIENT_ADMIN/config.dat $CLIENT_ADMIN/config.$CONFIG_HOST
      mv -f ${cfgloc}/config.nfs ${cfgloc}/config.dat
   fi
fi #end WRITE_RESPONSE
   $BATCH || pause
trap : 0
clean_exit
}

eval do_setup $INSTLOG
trap : 0
exit 0
fi
