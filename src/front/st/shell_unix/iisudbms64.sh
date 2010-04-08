:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#	iisudbms64 -- set-up script for Ingres DBMS (64-bit support)
#
#  Usage:
#	iisudbms64
#
#  Description: This script should only be called by the Ingres
#	installation utility.  It sets up Ingres Star Distributed
#	DBMS on a server which already has the Ingres 
#	DBMS and Ingres Networking set up.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	28-Aug-2002 (hanch04)
##	    Created
##	06-Sep-2002 (somsa01)
##	    Corrected setting of II_LP64_ENABLED. Also, make sure we
##	    turn off the RMCMD server before the setup.
##	23-Sep-2002 (hanch04)
##	    Change the ordering of install so that II_LP64_ENABLED is
##	    set before we call iisudbms.sh.  We don't need to do
##	    any work in here.
##	20-Jan-2004 (bonro01)
##	    Remove ingbuild calls for PIF installs.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##	04-Nov-2004 (bonro01)
##	    Set the SETUID bits in the install scripts because the pax
##	    program does not set the bits when run as a normal user.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##      10-Mar-2005 (hweho01)
##          In a 2.5 to r3 upgrade, imadb is left unupgraded if Star 
##          server wasn't installed in the previous release and all 
##          databases are selected to upgrade. 
##          To fix the error, need to always call upgradedb with iidbdb
##          as the parameter before upgrade all the user database.
##	07-Apr-2005 (hanje04)
##	    Bug 114248
##	    When asking users to re-run scripts it's a good idea to tell
##	    them which script needs re-running.
##	01-Mar-2005 (bonro01)
##	    Add code to upgrade from 32-bit to 64-bit databases.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  DEST = utility
#----------------------------------------------------------------------------

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

if [ "$1" = "-rmpkg" ] ; then
   cp -p $cfgloc/config.dat $cfgloc/config.tmp
 
   trap "cp -p $cfgloc/config.tmp $cfgloc/config.dat; \
         rm -f $cfgloc/config.tmp; exit 1" 1 2 3 15
 
   rm -f $cfgloc/config.dat
 
   mv $cfgloc/config.new $cfgloc/config.dat
 
   rm -f $cfgloc/config.tmp
 
   cat << !
  The Ingres DBMS (64-bit support) has been removed.
 
!
 
else

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp

# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
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
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   [ "$1" ] &&
   {
      # reset HOSTNAME for remote NFS client
      HOSTNAME=$1
      CLIENT_ADMIN=$II_SYSTEM/ingres/admin/$HOSTNAME
   }
fi

SELF=`basename $0`

export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

do_iisudbms64()
{
echo "Setting up Ingres DBMS (64-bit support)..."

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## **** Skip everything if we are in WRITE_RESPONSE mode ****


iisulock "Ingres DBMS (64-bit support) setup" || exit 1

ACTUAL_HOSTNAME=$HOSTNAME

#
# Set the SETUID bit for all necessary files extracted from
# the install media.
#
set_setuid

touch_files # make sure symbol.tbl and config.dat exist

if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
   VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms64` ||
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
   VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
   {
       cat << !

Missing file $II_SYSTEM/ingres/version.rel

!
      exit 1
   }
fi

RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`iigetres ii.$CONFIG_HOST.config.dbms64.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

The $VERSION version of Ingres DBMS (64-bit support) has already been
set up on "$HOSTNAME".

!
   trap : 0
   clean_exit
}

cat << ! 

This procedure will set up the following version of the Ingres 
DBMS (64-bit support):

	$VERSION

to run on local host:

	$HOSTNAME
!

# generate default configuration resources
cat << !

Generating default configuration...
!

BITSINKERNEL=`syscheck -kernel_bits`
if [ "$BITSINKERNEL" = "32" ] ; then
    cat <<!

 -------------------------------------------------------------------------
| WARNING: This system is not configured for 64-bit support.              |
|                                                                         |
| Ingres will be configured without 64-bit support.                       |
 -------------------------------------------------------------------------
!
    $BATCH || pause
    exit 1
fi

if [ "$BITSINKERNEL" = "64" ] ; then
    ingsetenv II_LP64_ENABLED TRUE
fi

# If this is an attempt to install the 64-bit server add-on after the
# 32-bit server has already been install, then this routine is necessary
# to upgrade the database tables from 32-bit to 64-bit, otherwise
# When the 32-bit and 64-bit servers are installed at the same time, the
# iisudbms script handles the upgrade of all the databases.
# The ingbuild process assures that iisudbms64 will always run before iisudbms.
SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID` || exit 1
if [ "$SETUP" = "complete" ] ; then

    # verify whether this is an upgrade
    NONFATAL_ERRORS=
    IMADB_ERROR=
    II_DATABASE=`ingprenv II_DATABASE`
    if [ -d $II_DATABASE/ingres/data/default/iidbdb ] ; then
	# Looks like an upgrade
   
	# upgrade the system catalogs and optionally all databases 
   
	$BATCH || cat << !
  
Before you can access the existing databases in this Ingres instance, they
must be upgraded to support the 64-bit Ingres server which
you have installed.  You have the option of upgrading all the databases
contained in this Ingres instance now.  Alternately, you can upgrade them
later by hand using the "upgradedb" utility.
 
Be aware that the time required to upgrade all local databases will
depend on their size, number, and complexity; therefore it is not possible
to accurately predict how long it will take.  Also note that the Ingres
master database (iidbdb) will be upgraded at this time whether or not
you choose to upgrade your databases.
 
!
	UPGRADE_DBS=false
	def=y
	$BATCH ||
	{
	    if prompt "Upgrade all databases in this Ingres instance now?" $def
	    then
		UPGRADE_DBS=true
	    else
		UPGRADE_DBS=false
	    fi
	}
       
	# Note stdout append here since "command2" might run first
	rm -f $II_LOG/upgradedb.log
	if $UPGRADE_DBS ; then
	    COMMAND="upgradedb -all >>$II_LOG/upgradedb.log 2>&1"
	    COMMAND2="upgradedb iidbdb >>$II_LOG/upgradedb.log 2>&1"
	else
	    COMMAND="upgradedb iidbdb >>$II_LOG/upgradedb.log 2>&1"
	fi

	# See if there's already an imadb.  "Real" imadb's are in the
	# default data location.
	CREATE_IMADB=Y
	if [ -z "$FROM64" -a -d $II_DATABASE/ingres/data/default/imadb ] ; then
	    CREATE_IMADB=
	fi

	# Set startup count for all "com" server to zero for the duration
	# of the upgrade and save current values
        # Turn off remote-command servers until we can get it set up
	# or db's upgraded
	RMCMD_SERVERS=`iigetres ii.$CONFIG_HOST.ingstart."*".rmcmd`
	[ -z "$RMCMD_SERVERS" ] && RMCMD_SERVERS=0
	if [ $RMCMD_SERVERS -ne 0 ] ; then
	    $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".rmcmd 0
	fi
	quiet_comsvrs


        wostar=
	if [ $STAR_SERVERS -ne 0 ] ; then
		wostar=' without Star'
        fi
	if [ "$UPGRADE_DBS" = 'true' ] ; then
	    cat << ! 

Starting the Ingres server$wostar to upgrade all system and user databases...
!
	else
	    cat << ! 

Starting the Ingres server$wostar to upgrade system databases...
!
	fi

	memsize=`iishowres`

	$DOIT ingsetenv II_LG_MEMSIZE $memsize
	$DOIT ingstart $RAD_ARG ||
	{
	    $DOIT ingstop >/dev/null 2>&1
	    cat << ! | error_box
The Ingres server could not be started.  See the server error log ($II_LOG/errlog.log) for a full description of the problem. Once the problem has been corrected, you must re-run $SELF before attempting to use the Ingres instance.
!
	     $DOIT ingunset II_LG_MEMSIZE
	     exit 1
	}

	if [ "$UPGRADE_DBS" = "true" ]
	then
	    echo "Upgrading Ingres master and system databases..."
	    if $DOIT eval $COMMAND2 ; then
              if [ $STAR_SERVERS -ne 0 ] ; then
		cat << !

Shutting down the Ingres server...
!
		$DOIT ingstop >/dev/null 2>&1
		$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star $STAR_SERVERS
		cat << ! 

Starting the Ingres servers including Star to upgrade user databases...
!
		$DOIT ingstart $RAD_ARG ||
		{
		    $DOIT ingstop >/dev/null 2>&1
		    cat << ! | error_box
The Ingres server could not be started.  See the server error log ($II_LOG/errlog.log) for a full description of the problem. Once the problem has been corrected, you must re-run $SELF before attempting to use the Ingres instance.
!
		    $DOIT ingunset II_LG_MEMSIZE
		    exit 1
		}
              fi
	    else
		# Error upgrading iidbdb pre-all
		cat << ! | error_box
A system database could not be upgraded.  A log of the attempted upgrade can be found in:

	$II_LOG/upgradedb.log

The problem must be corrected before the installation can be used.

Contact Ingres Corporation Technical Support for assistance.
!
		$DOIT ingstop >/dev/null 2>&1
                if [ $STAR_SERVERS -ne 0 ] ; then
		  $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star $STAR_SERVERS
                fi
		$DOIT ingunset II_LG_MEMSIZE
		exit 1
	    fi
        else
          if [ $STAR_SERVERS -ne 0 ]
          then
              $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star $STAR_SERVERS
          fi
	fi

	if $UPGRADE_DBS ; then
	    echo "Upgrading all system and user databases..."
	else
	    echo "Upgrading Ingres master and system databases..."
	fi
	if $DOIT eval $COMMAND ; then
	    # FIXME - sleep to stop DB lock race condition
	    sleep 1
	    $DOIT sysmod +w iidbdb
	    cat << !

Checkpointing Ingres master database...
!
	    # FIXME - sleep to stop DB lock race condition
	    sleep 1
	    $DOIT ckpdb +j iidbdb -l +w > \
	      $II_LOG/ckpdb.log 2>&1 ||
	    {
		cat << ! | warning_box
An error occurred while checkpointing your master database.  A log of the attempted checkpoint can be found in:

$II_LOG/ckpdb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
		NONFATAL_ERROR=Y
	    }

	    ## upgradedb is now kind enough to auto upgrade icesvr and
	    ## other ice installation db's when iidbdb is upgraded.

	else
	    # upgradedb on iidbdb or on all failed
	    UPGRADE_ERROR=Y
	    NONFATAL_ERROR=Y
	    if $UPGRADE_DBS ; then
		cat << ! | error_box
An error occurred while upgrading your system databases or one of the user databases in this Ingres instance.  A log of the attempted upgrade can be found in:

$II_LOG/upgradedb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
	    else
		cat << ! | error_box
An error occurred while upgrading your system databases.  A log of the attempted upgrade can be found in: 

$II_LOG/upgradedb.log 

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
	    fi
	fi

	# Set startup counts back to original values
	restore_comsvrs
    fi

    cat << !

Shutting down the Ingres server...
!
    $DOIT ingstop >/dev/null 2>&1
    if [ -z "$NONFATAL_ERRORS" ] ; then
	cat << !

Ingres DBMS (64-bit support) setup complete.
!
    else
	cat << !

Ingres DBMS (64-bit support) setup completed, with errors.

Some errors were encountered creating, checkpointing, or upgrading
one or more databases.  The installation will be marked "complete",
but the errors should be reviewed and corrected before attempting to
use this Ingres instance.  
!
    fi
    $DOIT ingstop -f >/dev/null 2>&1
    cat << !

Refer to the Ingres Installation Guide for information about
starting and using Ingres.
      
!
fi

iisetres ii.$CONFIG_HOST.config.dbms64.$RELEASE_ID complete
remove_temp_resources

fi #end WRITE_RESPONSE mode
trap : 0
clean_exit
}
eval do_iisudbms64 $INSTLOG
trap : 0
exit 0
fi
