:
# Copyright (c) 2010 Ingres Corporation
#
#  Name: setupingres
#
#  Usage:
#       setupingres [response file]
#
#  Description:
#	Runs Ingres "iisu" setup scripts based on contents of
#	setup.todo file located under /var/lib/ingres. Intended to be
#	run from service ingresXX configure during or after installation
#	via RPM/DEB
#
#
## History:
##	20-Jan-2010 (hanje04)
##	    Created.
##	16-Jun-2010 (hanje04)
##	    Bug 123931
##	    Response file is passed as first argument not second

self=`basename $0`
(LSBENV)

# Check environment
[ -z "$II_SYSTEM" ] &&
{
    echo "ERROR: II_SYSTEM is not set"
    exit 1
}

if [ -x "$II_SYSTEM/ingres/bin/ingprenv" ]
then
    instid=`$II_SYSTEM/ingres/bin/ingprenv II_INSTALLATION`
    [ -z "${instid}" ] &&
    {
	echo "ERROR: II_INSTALLATION is not set"
	exit 2
    }
    ingenv="$II_SYSTEM/.ing${instid}bash"
    ingdir=/var/lib/ingres/${instid}
    versrel="$II_SYSTEM/ingres/version.rel"
    source ${ingenv} ||
    {
	cat << EOF
ERROR:
Failed to source Ingres runtime environment, aborting ${self}

EOF
	exit 3
    }
fi

. iisysdep
if [ x"$conf_LSB_BUILD" == x"TRUE" ] ; then
    ingdir=/var/lib/ingres
    versrel=/usr/share/ingres/version.rel
else
    ingdir=/var/lib/ingres/${instid}
    versrel="$II_SYSTEM/ingres/version.rel"
fi
versno=`head -1 ${versrel} | cut -d' ' -f2`
bldno=`head -1 ${versrel} | cut -d/ -f2 | cut -d\) -f1`
setuptodo=${ingdir}/setup.todo
pid=$$
runfile=/var/run/${self}${instid}.pid
respfile=
rc=0

purge_todo()
{
    [ "$1" ] || return
    sed -i -e "/$1/d" $setuptodo
}

setup_dbms()
{
    # run pre setup to check for potential errors
    # then run the setup
    sh iidbmspreinst -r ingres -v ${versno}-${bldno} &&
	sh iigenpostinst -r ingres -v ${versno}-${bldno} -p dbms -s iisudbms
    rc=$?
    if [ $rc = 0 ] ; then
	purge_todo iidbms
    fi
    return $rc
}
 
setup_net()
{
    # run pre setup to check for potential errors
    # then run the setup
    sh iigenpostinst -r ingres -v ${versno}-${bldno} -p net -s iisunet
    rc=$?
    if [ $rc = 0 ] ; then
	purge_todo iisunet
    fi
    return $rc
}

# check if we're already running
[ -f ${runfile} ] &&
{
    oldpid=`head -1 $runfile`
    if [ -d /proc/${oldpid} ] ; then
        cat << EOF
Ingres setup process is already running with PID `cat ${runfile}`
EOF
        exit 10
    else
        rm -f ${runfile}
    fi
}
trap "rm -f ${runfile}" 0 ERR
echo $$ > ${runfile}    

# Check there's actually something to do,
# exit silently if there isn't
[ -r $setuptodo ] || [ -s $setuptodo ] || exit 0

# Check for response file
respfile=${1:-}
if [ "$respfile" ] ; then
    if [ -r "$respfile" ] ; then
	export II_RESPONSE_FILE="${respfile}"
    else
        echo "\"$respfile\" is not a valid response file, aborting..."
        exit 15
    fi
fi

echo "Running setup for Ingres ${versno}-${bldno}..."
# Need to run Net and DBMS setup before the rest
if ( grep -q iisudbms $setuptodo ) ; then
    setup_dbms
    rc=$?
fi
if [ $rc = 0 ] && ( grep -q iisunet $setuptodo ) ; then
    setup_net
    rc=$?
fi
if [ $rc = 0 ] ; then
    for scr in `cat $setuptodo`
    do
        sh iigenpostinst -r ingres -v ${versno}-${bldno} -s $scr 
        rc=$?
        if [ $rc = 0 ] ; then
	    purge_todo $scr
        else
	    break
        fi
    done
fi
if [ $rc != 0 ] ; then
	cat << EOF
ERROR:
An error occured during setup, aborting ${self}

EOF
fi

exit $rc
