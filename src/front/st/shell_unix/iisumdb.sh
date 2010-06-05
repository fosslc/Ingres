:
#
#  Copyright (c) 2001, 2004 Ingres Corporation
#
#  Name:
#	iisumdb -- Setup Ingres for embedded use and create MDB
#
#  Usage:
#	iisumdb 
#
#  Description:
#       This script will setup an existing Ingres installaion for use as
#	a repository for other CA products. It will also install MDB.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	27-Sep-2004 (hanje04)
##	    Created.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##      07-Apr-2005 (hanje04)
##          Bug 114248
##          When asking users to re-run scripts it's a good idea to tell
##          them which script needs re-running.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	08-Mar-2010 (thaju02)
##	    Remove max_tuple_length.
##
#  PROGRAM = iisumdb
#
#  DEST = utility
#----------------------------------------------------------------------------

. iisysdep
  
. iishlib

BATCH=false
NOTBATCH=true
RPM=false
INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
SELF=`basename $0`
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
IGNORE_RESPONSE="true"
export IGNORE_RESPONSE
REDO=false
DOCFG=true
DOMDB=true

#
# Do a basic sanity check (II_SYSTEM set to a directory).
#
[ -z $II_SYSTEM -o ! -d $II_SYSTEM ] &&
{
    cat << !

II_SYSTEM must be set to the Ingres root directory before running $SELF.

Please correct your environment before running $SELF again.

!
    exit 1
}

# check for flags
while [ "$1" ]
do
    if [ "$1" = "-batch" ] ; then
	BATCH=true
	NOTBATCH=false
	INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
	shift ;
    elif [ "$1" = "-mkresponse" ] ; then
	WRITE_RESPONSE=true
	BATCH=false
	NOTBATCH=true
	if [ "$2" ] ; then
            RESOUTFILE="$2"
            shift ;
	fi
	shift ;
    elif [ "$1" = "-exresponse" ] ; then
	READ_RESPONSE=true
	BATCH=true
	NOTBATCH=false
	if [ "$2" ] ; then
            RESINFILE="$2"
            shift ;
	fi
	shift ;
    elif [ "$1" = "-vbatch" ] ; then
	BATCH=true
	NOTBATCH=false
	shift;
    elif [ "$1" = "-rpm" ] ; then
	RPM=true
	BATCH=true
	NOTBATCH=false
	INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
	shift;
    elif [ "$1" = "-reinitcfg" ] ; then
	REDO=true
	DOMDB=false
	shift;
    elif [ "$1" = "-clean" ] ; then
	REDO=true
	shift;
    else
	echo $1 is an invalid switch to $SELF
	shift;
	exit 1
    fi
done

# Set clean up on exit
trap "rm -f $II_SYSTEM/ingres/files/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit $rc" 0 1 2 3 4 15

##export WRITE_RESPONSE
##export RESOUTFILE
export READ_RESPONSE
export RESINFILE

#=[ config_mdb ]====================================================
#
#	Routine to configure server for MDB
#
config_mdb()
{
    rc=0
    # API settings
    iisetres ii.$CONFIG_HOST.gcc.*.registry_type peer
    iisetres ii.$CONFIG_HOST.gcn.registry_type peer
    iisetres ii.$CONFIG_HOST.registry.tcp_ip.status ON

    # Turn on all cache sizes up to 32K
    for cache_size in 4 8 16 32 64
    {
	$DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.cache.p${cache_size}k_status ON
	$DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.cache.p${cache_size}k_status ON
    }

    # Set cachsize guidlines
    # Commands to set cache sizes and group buffer independently have 
    # been commented out. Uncomment to use

    # 2K
    $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.cache_guideline medium
    $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.cache_guideline medium

    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.dmf_cache_size 10000
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.dmf_cache_size 10000
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.dmf_group_count 1500
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.dmf_group_count 1500

    # 4K
    $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.p4k.cache_guideline medium
    $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.p4k.cache_guideline medium

    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p4k.*.dmf_cache_size 7500
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p4k.*.dmf_cache_size 7500
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p4k.*.dmf_group_count 900
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p4k.*.dmf_group_count 900

    # 8K
    $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.p8k.cache_guideline small
    $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.p8k.cache_guideline small

    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p8k.*.dmf_cache_size 2000
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p8k.*.dmf_cache_size 2000
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p8k.*.dmf_group_count 250
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p8k.*.dmf_group_count 250

    # 16K
    $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.p16k.cache_guideline medium
    $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.p16k.cache_guideline medium

    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p16k.*.dmf_cache_size 3000
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p16k.*.dmf_cache_size 3000
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p16k.*.dmf_group_count 375
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p16k.*.dmf_group_count 375

    # 32K
    $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.p32k.cache_guideline small
    $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.p32k.cache_guideline small

    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p32k.*.dmf_cache_size 375
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p32k.*.dmf_cache_size 375
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p32k.*.dmf_group_count 45
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p32k.*.dmf_group_count 45

    # 64K
    $DOIT iisetres ii.$CONFIG_HOST.dbms.private.*.p64k.cache_guideline small
    $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.*.p64k.cache_guideline small

    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p64k.*.dmf_cache_size 175
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p64k.*.dmf_cache_size 175
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.private.p64k.*.dmf_group_count 20
    ##  $DOIT iisetres ii.$CONFIG_HOST.dbms.shared.p64k.*.dmf_group_count 20

    # Check log file is big enough
    LOG_KBYTES=`iigetres ii.${CONFIG_HOST}.rcp.file.kbytes` 
    [ -z "$LOG_KBYTES" ] || [ $LOG_KBYTES -lt 131072 ] &&
    {
	error_box << !
Transaction log file must be at least 131073Kb for setup to continue.

Please correct this problem before running $SELF again.

!
	rc=1
    } 

    [ $rc != 0 ] && exit $rc
    return 0
}

# Main routine for setting up MDB
do_setup()
{
# Set clean up on exit
trap "rm -f $II_SYSTEM/ingres/files/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit $rc" 0 1 2 3 4 15

# Take config lock
iisulock "Ingres MDB setup" || exit 1

# Determine Ingres version info
if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
    VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
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

# Determine MDB version info
MDBVERS=013 #FIXME need to obtain automatially

SETUP=`iigetres ii.$CONFIG_HOST.config.mdb.$MDBVERS` || exit 1

# Already done?
[ "$SETUP" = "complete" ] && ! $REDO &&
{
   cat << !

The $MDBVERS version of MDB has already been
setup on "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
}

#Check DBMS package is setup
DBMS_SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID` || exit 1

if [ "$DBMS_SETUP" != "complete" ]
then
   cat << !

The setup procedure for the following version of Ingres Intelligent DBMS:

	$VERSION

must be completed up before MDB can be set up on host:

	$HOSTNAME

!
   exit 1
fi

CONFIG_HOST=`iigetres ii."*".config.server_host`
SERVER_HOST=`iigetres ii.$CONFIG_SERVER_HOST.gcn.local_vnode`
II_INSTALLATION=`ingprenv II_INSTALLATION`

      cat << !

This procedure will set up the following version MDB:

	$MDBVERS

to run against the $II_INSTALLATION instance of Ingres on local host:

	$HOSTNAME

!
      $BATCH || prompt "Do you want to continue this setup procedure?" y \
         || exit 1

$REDO && $DOCFG &&
{
    cat << !

Re-initializing confuration for MDB
!
}

# Configure installation for MDB.
config_mdb || rc=2 

# Skip the rest if database already exists.
$DOMDB &&
{
	# Start the installation
	[ $rc = 0 ] &&
	{
  	    cat << ! 

Staring the Ingres Server...
!
	    $DOIT ingstart > /dev/null 2>&1 ||
	    {
	        $DOIT ingstop >/dev/null 2>&1
	        cat << ! | error_box
The Ingres server could not be started.  See the server error log (\$II_SYSTEM/ingres/files/errlog.log) for a full description of the problem. Once the problem has been corrected, you must re-run $SELF before attempting to use the Ingres instance.
!
	         rc=3
	    }
        }

# Setup MDB
	    [ $rc = 0 ] && 
	    {
# Check response file options
	        [ "$READ_RESPONSE" = "true" ] && 
		{
		    dbname=`iiread_response II_MDB_DBNAME $RESINFILE`
		    clean=`iiread_response  CLEAN_MDB $RESINFILE`
		}

		if [ "$dbname" ] ; then
		    II_MDB_DBNAME=$dbname
		else
		    II_MDB_DBNAME=mdb
		fi

		if [ "$clean" = "true" ] ; then
		    loadflag=-clean
		else
		    loadflag=""
		fi

# Unpack mdb archive
		$DOIT pushd ${II_SYSTEM}/ingres/files/mdb > /dev/null 2>&1 && \
	        $DOIT gunzip -c mdb.tar.gz | tar xf - >/dev/null 2>&1 && \
# Create and load the database
		$DOIT sh ./loadmdb local $II_MDB_DBNAME $loadflag && \
		$DOIT iisetres ii.$CONFIG_HOST.mdb.mdb_dbname \
						 ${II_MDB_DBNAME} && \
		$DOIT iisetres ii.$CONFIG_HOST.config.mdb.$MDBVERS complete || 
		{
	        cat << ! | error_box
An error occured initializing MDB.

You should contact Ingres Corporation Technical Support to resolve this problem. 
!
		rc=4
		}

# Clean up and go back to where we started
		$DOIT rm -rf ./backup ./DDL >/dev/null 2>&1
		$DOIT popd >/dev/null 2>&1
	    }

	    cat << !

Shutting down the Ingres server...
!
	    $DOIT ingstop >/dev/null 2>&1
} # REINITCFG
exit $rc
}

eval do_setup $INSTLOG
