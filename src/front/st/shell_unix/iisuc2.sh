:
#
#  Copyright (c) 1992, 2010 Ingres Corporation
#
#  Name:
#	iisuc2 -- set up C2 security auditing package 
#
#  Usage:
#	iisuc2
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility.  It sets up the C2 auditing option.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	19-oct-93 (tyler)
##		Created.	
##	04-nov-93 (dianeh)
##		Fixed a typo and some initial caps.
##	22-nov-93 (tyler)
##		Replaced system calls with shell procedure invocations.
##		Replaced iimaninf call with ingbuild call.
##	30-nov-93 (tyler)
##		ingbuild must be invoked by its full path name.
##	16-dec-93 (tyler)
##		Use CONFIG_HOST instead of HOSTNAME to compose
##		configuration resource names.  Name changed from
##		iisuc2sa.sh to iisuc2.sh for simplicity.
##	16-mar-94 (robf)
##	        Reworked, C2 and ES are different, with different 
##	14-apr-94 (arc)
##		Fixed typo (auditng -> auditing).
##	16-may-95 (hanch04)
##		added batch mode option
##	15-jun-95 (popri01)
##		nc4_us5 complained about VERSION sed string. Escape the '/'.
##	22-nov-95 (hanch04)
##		Added log of install
##      08-nov-1996 (canor01)
##              Update older values for audit mechanism, ca_high, ca_low.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##    27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based
##              installer on Linux.
##      04-Sep-2001 (gupsh01)
##              Added flags input -mk_response, ex_response for creating and
##              executing response files.
##      23-Oct-2001 (gupsh01)
##              Make changes to implement reading parameter values from response file. 
##              (flag -exresponse).
##	02-nov-2001 (gupsh01)
##		Added support for user given response file. 
##	11-feb-2002 (devjo01)
##		Allow secondary cluster node setup.
##	08-jan-2004 (somsa01)
##		We do not need to create an empty symbol.tbl file anymore.
##	20-Jan-2004 (bonro01)
##		Remove ingbuild call for PIF installs.
##    27-Jan-2004 (hanje04)
##            Added -rpm flag to support installing from RPM files.
##	03-feb-2004 (somsa01)
##		Backed out symbol.tbl change for now.
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	24-Jan-2006 (kschendel)
##	    Change audit mech back to INGRES.
##	02-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
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
   cat << !
  Ingres C2 Security Auditing has been removed

!
exit 0
fi

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false
BATCH=false
INSTLOG="2>&1 | tee -a $II_LOG/install.log"

# check for batch flag 
BATCH=false
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
 
while [ $# != 0 ]
do
    case "$1" in
        -batch)
            BATCH=true ; export BATCH ; 
	    INSTLOG="2>&1 | cat >> $II_LOG/install.log"
	    export INSTLOG;
            shift ;;
	-vbatch)
	    BATCH=true ; export BATCH ;
            export INSTLOG;
            shift ;;
        -mkresponse)
            WRITE_RESPONSE=true ; export WRITE_RESPONSE ;
            if [ "$2" ] ; then
                RESOUTFILE=$2
		shift;
            fi
            export RESOUTFILE;
            export INSTLOG;
            shift ;;
        -exresponse)
            READ_RESPONSE=true ; export READ_RESPONSE ;
	    BATCH=true ; export BATCH ;
            if [ "$2" ] ; then
                RESINFILE=$2
		shift;
            fi
            export RESINFILE;
            export INSTLOG;
            shift ;;
        -rpm)
            BATCH=true ; export BATCH ; 
	    RPM=true
	    INSTLOG="2>&1 | cat >> $II_LOG/install.log"
	    export INSTLOG;
            shift ;;
        *)
            echo "ERROR: Unknown option to iisuc2: $1" ;
            echo "usage: iisuc2 [ -batch ] [ -vbatch ] [ -mkresponse] \
[ -exresponse] [ response file]"
            exit 1 ;;
    esac
done


if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm ${cfgloc}/config.lck 1>/dev/null 2>/dev/null; exit 1" 1 2 15
fi

do_iisuc2()
{
echo "Setting up Ingres C2 Security Auditing..."
if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm ${cfgloc}/config.lck 1>/dev/null 2>/dev/null; exit 1" 1 2 15
fi

check_response_file # check if the response files do exist.

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else

iisulock "Ingres C2 Security Auditing setup" || exit 1

# create symbol.tbl in $II_SYSTEM/ingres/files if not found
[ -f ${symbloc}/symbol.tbl ] ||
{
   touch ${symbloc}/symbol.tbl ||
   {
      cat << !

Unable to create ${symbloc}/symbol.tbl.  Exiting...

!
      rm ${cfgloc}/config.lck
      exit 1
   }
}

# create config.dat if it doesn't exist
[ -f ${cfgloc}/config.dat ] ||
{
   touch ${cfgloc}/config.dat 2>/dev/null ||
   {
      cat << !

Unable to create ${cfgloc}/config.dat.  Exiting...

!
      rm ${cfgloc}/config.lck
      exit 1
   }
   chmod 644 ${cfgloc}/config.dat
}

SERVER_HOST=`iigetres ii."*".config.server_host`

if [ -n "$SERVER_HOST" -a "$SERVER_HOST" != "$CONFIG_HOST" ] ; then
   CLUSTER_ID=""
   if $CLUSTERSUPPORT ; then
      CLUSTER_ID="`(PROG1PRFX)getres \
	    (PROG4PRFX).$CONFIG_HOST.config.cluster.id`"
      case "$CLUSTER_ID" in
      [1-9]|[12][0-9]|3[0-2])     # Setup for cluster
	  ;;
      *)          # normal non-server setup.
	  CLUSTER_ID=""
	  ;;
      esac
   fi
   [ -z "$CLUSTER_ID" ] &&
   {
      cat << !

   This is an Ingres client installation.

   Ingres C2 Security Auditing can only be set up for server installations.

!
      rm ${cfgloc}/config.lck
      exit 1
   } 
fi

if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
    VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
    {
   # display output of failed ingbuild call
   cat << !

$VERSION

!
      rm ${cfgloc}/config.lck
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
      rm ${cfgloc}/config.lck
      exit 1
   }
fi

RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`iigetres ii.$CONFIG_HOST.config.c2.$RELEASE_ID` ||
{
   rm ${cfgloc}/config.lck
   exit 1
}

[ "$SETUP" = "complete" ] &&
{
   cat << !

Ingres C2 Security Auditing has already been set up on "$HOSTNAME".

!
   rm ${cfgloc}/config.lck
   $BATCH || pause
   exit 0
}

SERVER_SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`

[ "$SERVER_SETUP" != "complete" ] &&
{
   cat << !

The setup procedure for the following version of the Ingres
Intelligent DBMS:

	$VERSION

must be completed up before security auditing can be set up on
this host:

	$HOSTNAME

!
   rm ${cfgloc}/config.lck
   exit 1
} 

cat << ! 

This procedure will set up Ingres Security Auditing for local host:

	$HOSTNAME

Generating default security auditing configuration...
!

# check for older values
CA_HIGH=`iigetres ii.$CONFIG_HOST.secure.ca_high`
INGRES_HIGH=`iigetres ii.$CONFIG_HOST.secure.ingres_high`
CA_LOW=`iigetres ii.$CONFIG_HOST.secure.ca_low`
INGRES_LOW=`iigetres ii.$CONFIG_HOST.secure.ingres_low`
AUDIT_MECH=`iigetres ii."*".c2.audit_mechanism`

if [ "$AUDIT_MECH" = "CA" ]
then
    iisetres ii."*".c2.audit_mechanism INGRES
fi
if [ -z "$CA_LOW" -a ! -z "$INGRES_LOW" ]
then
    iisetres ii.$CONFIG_HOST.secure.ca_low "$INGRES_LOW"
    iiremres ii.$CONFIG_HOST.secure.ingres_low
fi
if [ -z "$CA_HIGH" -a ! -z "$INGRES_HIGH" ]
then
    iisetres ii.$CONFIG_HOST.secure.ca_high "$INGRES_HIGH"
    iiremres ii.$CONFIG_HOST.secure.ingres_high
fi

iigenres $CONFIG_HOST $II_CONFIG/secure.rfm || 
{
   cat << !
An error occurred while generating the default configuration.

!
   rm ${cfgloc}/config.lck
   exit 1
}

#
# Audit log prompts go here.  Use iisetres to add values to config.dat.
#

cat << !

Ingres C2 Security Auditing is now set up. 

!
rm ${cfgloc}/config.lck
iisetres ii.$CONFIG_HOST.config.c2.$RELEASE_ID complete

fi #end $WRITE_RESPONSE 

   $BATCH || pause
exit 0
}
eval do_iisuc2 $INSTLOG
trap : 0
exit 0
