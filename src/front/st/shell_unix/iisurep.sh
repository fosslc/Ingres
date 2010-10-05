:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#	iisurep
#
#  Usage:
#	iisurep  (called from ingbuild, generally)
#
#  Description:
#	Configure the DD_RSERVERS location and set link file for REP
#
#  Exit status:
#	0	OK
#       99	REP is not installed
#
##
## History:
##	13-dec-96 (joea)
##		Created.
##	22-apr-97 (joea)
##		Correct setup for when shared libraries are used.
##	11-dec-97 (joea)
##		Don't create replnk.opt since we'll deliver a RepServer
##		executable.
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
##      18-sep-2001 (gupsh01)
##            Modified to add the -mkresponse flag.
##	23-oct-2001 (gupsh01)
##		Added code to handle -exresponse flag.
##	02-nov-2001 (gupsh01)
##		Corrected for user defined response filename.
##      15-Jan-2004 (hanje04)
##              Added support for running from RPM based install.
##              Invoking with -rpm runs in batch mode and uses
##              version.rel to check version instead of ingbuild.
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##      29-Sep-2010 (thich01)
##          Call iisetres to indicate this script ran successfully so the post
##          install script detects its success.
##
#  DEST = utility
#----------------------------------------------------------------------------

if [ "$1" = "-rmpkg" ] ; then
   cat << !
  Replication Option has been removed

!
exit 0
fi

. iisysdep
. iishlib
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

# check for batch flag
BATCH=false
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false


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
            WRITE_RESPONSE=true; export WRITE_RESPONSE;
            BATCH=false
            if [ "$2" ] ; then
                RESOUTFILE=$2
		shift;
            fi
            export RESOUTFILE;
            export INSTLOG;
            shift ;;
        -exresponse)
            READ_RESPONSE=true; export READ_RESPONSE;
            BATCH=true
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
	    echo "ERROR: Unknown option to iisurep: $1" ;
	    echo "usage: iisurep [ -batch ] [-vbatch] [-mkresponse] [-exresponse] [response file]"
	    exit 1 ;;
    esac
done

trap "exit 1" 0 1 2 3 15

do_iisurep()
{
echo "Setting up Replicator..."

trap "exit 1" 0 1 2 3 15

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then
    mkresponse_msg
else                    #SKIP the REST

[ -f $II_SYSTEM/ingres/bin/repmgr ] || 
	[ x"$conf_LSB_BUILD" = x"TRUE" -a -f /usr/bin/repmgr ] ||
{
   echo "ERROR: Attempted to set up Replicator when repmgr is not installed!"
   cat << !

Attempted to set up Replicator, but repmgr executable is missing.

!
   exit 1 
}

if [ -z "$DD_RSERVERS" ] 
then
    if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	DD_RSERVERS=/var/lib/ingres/rep
    else
	DD_RSERVERS=$II_SYSTEM/ingres/rep
    fi
fi

if [ ! -d $DD_RSERVERS ]
then
	echo Creating RepServer Directories...
	mkdir $DD_RSERVERS
	chmod 777 $DD_RSERVERS
	mkdir $DD_RSERVERS/servers
	chmod 777 $DD_RSERVERS/servers
	for n in 1 2 3 4 5 6 7 8 9 10
	do	mkdir $DD_RSERVERS/servers/server$n
		chmod 777 $DD_RSERVERS/servers/server$n
	done
fi

# Configure DBMS Replication parameters (TBD)

iisetres ii.$CONFIG_HOST.dbms."*".rep_qman_threads 1
iisetres ii.$CONFIG_HOST.dbms."*".rep_txq_size 50

cat << !

Replicator setup complete.

!
fi #end WRITE_RESPONESE flag.
   if [ -f $II_SYSTEM/ingres/install/release.dat ] ; then
       VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
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
   SETUP=`iigetres ii.$CONFIG_HOST.config.abf.$RELEASE_ID`
   if [ "$SETUP" = "complete" ]
   then
        # If testing, then pretend we're not set up, otherwise scram.
        $DEBUG_DRY_RUN ||
        {
            cat << !
   
The $VERSION version of the Replication Option has
already been set up on local host "$HOSTNAME".
   
!
            $BATCH || pause
            trap : 0
            exit 0
        }
        SETUP=""
   fi

iisetres ii.$CONFIG_HOST.config.rep.$RELEASE_ID complete
$BATCH || pause
trap : 0
exit 0
}
eval do_iisurep $INSTLOG
trap : 0
exit 0
