:
#
#  Copyright (c) 2004 Ingres Corporation  
#
#  Name: iisuscha -- HA SUN Cluster Node setup script for Ingres DBMS
#
#  Usage:
#	iisuscha
#
#  Description:
#	This script is currently used to setup Ingres to run in a 
#	Sun Cluster.
#
#  Exit Status:
#	0       setup procedure completed.
#	1       setup procedure did not complete.
#
#  DEST = utility
## History:
##	15-Mar-2004 (hanch04)
##		Created.	
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##
#----------------------------------------------------------------------------
SELF=`basename $0`

WRITE_RESPONSE=false
READ_RESPONSE=false
BATCH=false
DEBUG_DRY_RUN=false
DEBUG_FAKE_NEW=false
DOIT=""
DIX=""
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"

# check parameters
if [ "$1" = "-batch" ] ; then
    BATCH=true
    INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
elif [ "$1" = "-vbatch" ] ; then
    BATCH=true
elif [ "$1" = "-mkresponse" ] ; then
    WRITE_RESPONSE=true
    [ "$2" ] && RESOUTFILE="$2"
elif [ "$1" = "-exresponse" ] ; then
    READ_RESPONSE=true
    BATCH=true  #run as if running -batch flag.
    [ "$2" ] && RESINFILE="$2"
elif [ "$1" = "-test" ] ; then
    #
    # This prevents actual execution of most commands, while
    # logging commands being executed to iisudbms.dryrun.$$.
    # config.dat & symbol.tbl values are appended to this
    # file, prior to restoring the existing copies (if any)
    #
    DEBUG_LOG_FILE="/tmp/${SELF}_test_output_`date +'%Y%m%d_%H%M'`"
    DOIT=do_it
    DIX="\\"
    shift
    while [ "$1" ]
    do
        case "$1" in
        new)
            DEBUG_FAKE_NEW=true
            DEBUG_DRY_RUN=true
            ;;
        dryrun)
            DEBUG_DRY_RUN=true
            ;;
        esac
    done

    cat << !
Logging $SELF run results to $DEBUG_LOG_FILE

DEBUG_DRY_RUN=$DEBUG_DRY_RUN  DEBUG_FAKE_NEW=$DEBUG_FAKE_NEW
!
    cat << ! >> $DEBUG_LOG_FILE
Starting $SELF run on `date`
DEBUG_DRY_RUN=$DEBUG_DRY_RUN  DEBUG_FAKE_NEW=$DEBUG_FAKE_NEW

!
elif [ "$1" ] ; then
    cat << !

ERROR: invalid parameter "{$1}".

Usage $SELF [ -batch [<inst_id>] | -vbatch [<inst_id>] | \
           -mkresponse [ <resfile> ] | -exresponse [ <resfile> ] ]
!
    exit 1
fi

export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

. iisysdep

. iishlib

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

#=[ MAIN ]========================================================
#
#       Routine containing main body of DBMS setup
#
do_iisuscha()
{
    echo "Setting up Ingres for Sun Clusters..."

    cat << !

$SELF can configure a logical host to be used in running Ingres 
in on a High Avaliability Sun Cluster. 

It can also add node independent configurations for hosts.

All Ingres nodes in this Ingres cluster must be shutdown
in order to run this procedure.
!
         echo ""
         prompt "Do you want to continue this setup procedure?" y || exit 1

            cat << !

Shutting down the Ingres server...

!
#            $DOIT ingstop >/dev/null 2>&1
#            $DOIT ingstop -f >/dev/null 2>&1

    SERVER_HOST=`ingprenv II_HOSTNAME`
    if [ -n "$SERVER_HOST" ] ; then
      cat << !

The Ingres Intelligent DBMS installation located at:

        $II_SYSTEM
  
is already set up to run as logical host:

        $SERVER_HOST
!
      $BATCH || pause
      trap : 0
      clean_exit
    fi

    $BATCH ||
    {
	prompt "Do you want to configure a logical hostname?" y &&
	{
	    SERVER_HOST=`iigetres ii."*".config.server_host` || exit 1
	    (PROG1PRFX)echonn "Please enter the logical hostname: "
	    read II_HOSTNAME junk
	    cp $(PROG3PRFX)_CONFIG/config.dat $(PROG3PRFX)_CONFIG/config.bck
	    sed "s/ii.$SERVER_HOST/ii.$II_HOSTNAME/" \
		$(PROG3PRFX)_CONFIG/config.dat \
		> /tmp/config.$$
	    cp /tmp/config.$$  $(PROG3PRFX)_CONFIG/config.dat
	    $DOIT rm -f $II_SYSTEM/ingres/files/config.lck \
		/tmp/*.$$ >/dev/null 2>&1
	    (PROG2PRFX)setenv II_HOSTNAME $II_HOSTNAME
	    iisetres ii."*".config.server_host $II_HOSTNAME
	    iisetres ii.$II_HOSTNAME.gcn.local_vnode $HOSTNAME
            cat << !

Ingres logical hostname setup complete.
!

	    clean_exit
	}

	while true
	do
        prompt "Do you want to add a high availability host?" y &&
        {   
            SERVER_HOST=`iigetres ii."*".config.server_host` || exit 1
            (PROG1PRFX)echonn "Please enter the hostname to be added: "
            read II_HOSTNAME junk
            cp $(PROG3PRFX)_CONFIG/config.dat $(PROG3PRFX)_CONFIG/config.bck
            grep ii.$SERVER_HOST  $(PROG3PRFX)_CONFIG/config.dat \
	        | sed "s/ii.$SERVER_HOST/ii.$II_HOSTNAME/" \
		> /tmp/config.$$
	    cat /tmp/config.$$  >> $(PROG3PRFX)_CONFIG/config.dat
	    iisetres ii.$II_HOSTNAME.gcn.local_vnode $HOSTNAME
	    $DOIT rm -f $II_SYSTEM/ingres/files/config.lck \
		/tmp/*.$$ >/dev/null 2>&1
            cat << !

Ingres high availability host $II_HOSTNAME added.

!
	    continue
        }
	break
	done


    }
exit 0
}

trap "clean_up ; exit 1" 0 1 2 3 15

$DOIT iisulock "Ingres Sun Cluster HA setup" || exit 1

eval do_iisuscha $INSTLOG
trap : 0


