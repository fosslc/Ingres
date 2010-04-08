:
#  Copyright (c) 1993, 2010 Ingres Corporation
# 
# 
#1
#1  Name: ipcclean
#1
#1  Usage:
#1
#2    For stand-alone or non-NUMA cluster configurations:
#2
#1	   ipcclean
#1
#2    For NUMA cluster configurations:
#2
#2	   ipcclean { -r[ad]{=| }<rad_id> | -n[ode]{=| }<node_name> } ... \
#2		    | -local
#2
#2	where <rad_id> is a valid RAD id configured on the local host,
#2	and <node_name> is a valid node name or node alias (nickname)
#2	for a node on the local host, and "-local" targets all RADs
#2	configured on the local host.
#2
#1  Description:
#1	Removes INGRES shared memory key files and semaphore resources.
#1
## History:
##	18-jan-93 (tyler)
##		Adapted from old ipcclean.	
##	04-nov-93 (vijay)
##		Remove lgk segment.
##	24-apr-95 (canor01)
##		Remove other shared memory segments (notably iievents.001).
##	12-Jul-1995 (walro03)
##		Fix bug 69811: ipcrm shmid(nn) not found.  Caused when ipcrm
##		invoked the first time marks shared memory segments for
##		removal, but ipcs invoked the second time still reports them
##		as present.  Then ipcrm invoked the second time fails because
##		the shared memory segment has already been marked for removal,
##		and cannot be so marked again.  Fix is to build a single key
##		list based on output from csreport, lgkmkey, and iimemkey for
##		each file in ingres/files/memory.  Then build a grep pattern
##		list from these keys, and invoke ipcs and ipcrm only once.
##      28-aug-1997 (musro02)
##              For sqs_ptx, add a varient of the final sed script for IPCRM
##      03-sep-1997 (rigka01) bug #85144
##              remove use of lgkmkey since it is redundant - output
##              included in iimemkey loop and iimemkey obtains key
##              via same method.  This frees lgkmkey from having to
##              be modified for the purpose here of correct comparison
##              with IPCS output (8 hex digits - leading zeros included). 
##              See related bug #85043. 
##	05-nov-1997(angusm)
##		bug 86530: the 'tr' command is sensitive to env variable
##		'LANG' being set, hence will generate unexpected values 
##		e.g. if LANG=sv_SE.iso88591. This is because the
##		abbreviation [A-F], [a-f] may have other meanings
##		when some of these letters can appear in accented
##		forms. Expand the items in brackets, and all will be well.
##	13-nov-1997 (bobmart)
##		Modified angusm's fix a tad; removed brackets since they're
##		not required in his format (all was not well with dg8_us5).
##	3-feb-1998(angusm)
##		cross-integrated the above from oping12
##	06-mar-1998 (toumi01)
##		For Linux (lnx_us5) use a different technique to remove
##		shared memory and semaphores, since the ipcs and ipcrm
##		commands and the ids of memory are very non-standard.
##	20-Sep-1999 (bonro01)
##		Modified to support Unix clusters.
##	06-oct-1999 (toumi01)
##		Change Linux config string from lnx_us5 to int_lnx.
##	21-mar-2001 (hayke02)
##		Add new pattern for su4_us5 - leading zeroes are stripped
##		by ipcs on Solaris 2.7 onward. This change fixes bug
##		102539.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##	14-Jun-2000 (hanje04)
##		Added ibm_lnx to non standard method for removing shared
##		memory and semaphores.
##	08-Sep-2000 (hanje04)
##		Alpha Linux (axp_lnx) will use int_lnx tecnique for removing
##		shared memory.
##	25-jun-2001 (devjo01)
##		Use files/memory/${NODE}/<segnam> rather than 
##		files/memory/<segname>${NODE}.  This greatly simplifies
##		meshared.c, and the shell scripts.
##	08-May-2002 (hanje04)
##		Added support for Itanium Linux (i64_lnx) and ammended grep for
##	        "resource deleted" to include "resource(s) delete" as later
##	        verions of ipcrm on Linux print the latter.
##	19-Jul-2002 (hanje04)
##	 	Bug 108321
##	 	Changed grep on output of ipcrm so that it catches new version
##		of message; 'resource(s) deleted'
##	23-sep-2002 (devjo01)
##		Add -rad, -node, and -local parameters to allow exact
##		specification of target when running on a NUMA cluster host.
##      08-Oct-2002 (hanje04)
##          As part of AMD x86-64 Linux port, replace individual Linux
##          defines with generic *_lnx define where apropriate
##	19-May-2003 (hanje04)
##	    BUG 108321
##	    In later Linux releases (SuSE 8.0 etc) the error messages from ipcrm
##	    are being echoed to stderr and not stdout. To catch all messages
##	    redirect all output to /dev/null.
##	29-May-2003 (hanje04)
##	    BUG 108321
##	    Removed extra space in redirect causing ipccclean to fail.
##	29-May-2003 (hanje04)
##	    BUG 108321
##	    GRRRR! Use egrep instead of grep on Linux for deriving semid
##	31-jan-2005 (devjo01) - 13926758;01
##	    - Add CATOSL, correct copyright.
##	    - Change cluster check to examine cluster_mode, not cluster.id.
##	    - Distinguish between directories & files when cleaning up
##	      "memory" files.
##	23-May-2005 (hanje04)
##	    Mac OS X doesn't have ipcrm or ipcs. Support has been added
##	    for Mac OS X using versions of ipcs and ipcrm downloaded and
##	    built from http://stix.homeunix.net/software/#darwinipc.
##	    These may or may not end up shipping with the Ingres distro 
##	    on this platform.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##	05-Sep-2007 (wanfr01) 
##	    Bug 119066
##	    ipcs removes leading zeros off keys on a64 solaris.
##	    Bug 102539 needs to be extended for a64.sol
##	12-Feb-2008 (hanje04)
##	    SIR S119978
##	    Replace mg5_osx with generic OSX
##      18-Aug-2008 (horda03) Bug 120787
##          Allow individual shared memory segments to be removed. New
##          (optional) parameter -cache=<cache_name>
##      20-Aug-2008 (horda03) Bug 120787
##          On some platforms ls .... returns a warning if no files found.
##	22-Jun-2009 (kschendel) SIR 122138
##	    Check for su9 as well as su4.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  DEST = utility
#----------------------------------------------------------------------------
(LSBENV)

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    cfgloc=$(PRODLOC)/(PROD2NAME)/files
    symbloc=$(PRODLOC)/(PROD2NAME)/files
fi
export cfgloc symbloc


self=`basename $0`

#
#	clean_shared - guts of ipcclean.  Remove shared memory for one node
#
clean_shared()
{
# build keys list based on csreport
if [ -n "$cache_name" ] ; then
   keys=""
else
   keys="`csreport $1 | grep '^.key' | grep -v 'size 0' | sed 's/.....\(.*\): .*/\1/'`" 
# add to keys list based on lgkmkey
# keys="`lgkmkey $1` $keys"
fi

# "-local" flag forces pmhost to evaluate any passed NUMA context args,
# and to supress echo of host name if args are invalid, or NUMA clustering
# is in use, and no arguments establishing the NUMA context were provided.
NODE="`(PROG1PRFX)pmhost $1 -local`"
[ $NODE ] ||
{
    cat << !
$self: RAD context argument missing.

!
    exit 1
}

GETRES="`(PROG1PRFX)getres (PROG4PRFX).$NODE.config.cluster_mode | \
 tr '[a-z]' '[A-Z]'`"
if [ "X$GETRES" != "XON" ];
then
    MEMDIR="."
else
    MEMDIR="$NODE"
fi

# add to keys list based on files in ingres/files/memory
cd ${cfgloc}/memory/$MEMDIR

for MEMNAME in ${cache_name}*
do
	if [ -f "$MEMNAME" ]
	then
	case $VERS in
	        *_lnx|int_rpl)
		shmkey="`(PROG1PRFX)memkey $MEMNAME`"
		ipcrm shm $shmkey >> /dev/null 2>&1
		;;
	    axp_lnx)
		shmkey="`(PROG1PRFX)memkey $MEMNAME`"
		shmid="`ipcs -m | egrep -i $shmkey | awk '{ print $2 }'`"
		ipcrm shm $shmkey >> /dev/null 2>&1
		;;
	      *_osx)
		outfile=/tmp/ipcclean.$$
		shmkey="`(PROG1PRFX)memkey $MEMNAME`"
		shmid="`ipcs -m | egrep -i -- $shmkey | awk '{ print $2 }'`"
		ipcrm -m $shmid >> $outfile 2>&1
		[ $? != 0 ] && cat $outfile
		rm -f $outfile
		keys=$shmkey
		;;
	    *)
		keys="`(PROG1PRFX)memkey $1 $MEMNAME` $keys"
		;;
	esac
	fi
done
if [ ! -z "$keys" ]
then
    case $VERS in
	a64_us5)
    	    pattern=`echo $keys | sed -e "s/0x00*/0x0*/g" | sed "s/ / | /g" | tr "ABCDEF" "abcdef"`
	    ;;
	su9_us5|\
	su4_us5)
    	    pattern=`echo $keys | sed -e "s/0x00*/0x0*/g" | sed "s/ / | /g" | tr "ABCDEF" "abcdef"`
	    ;;
	*)
    	    pattern=`echo $keys | sed "s/ / | /g" | tr "ABCDEF" "abcdef"`
	    ;;
    esac
    if [ "$pattern" ]
    then
        case $VERS in
            sqs_ptx)
                $IPCRM `$IPCSCMD | egrep "$pattern" | sed "s/\(. *[^ ]*\).*/-\1/
"`
                ;;
	        *_lnx|int_rpl)
		shmkey=`ipcs -s | egrep -i "$pattern" | awk '{ print $2 }'`
		ipcrm sem $shmkey  >> /dev/null 2>&1
                ;;
	        *_osx)
		outfile=/tmp/ipcclean.$$
		shmkey=`ipcs -s | egrep -i -- "$pattern" | awk '{ print $2 }'`
		ipcrm -s $shmkey  >> $outfile 2>&1
		[ $? != 0 ] && cat $outfile
		rm -f $outfile
                ;;
            *)
                 $IPCRM `$IPCSCMD | egrep "$pattern" | sed "/\(. *[^ ]*\).*/s//-\1/"`
         ;;
        esac
    fi
fi

for f in ${cfgloc}/memory/$MEMDIR/${cache_name}*
do
    [ -f $f ] && \
     rm -f $f
done
}

#
#	add_rad - Verify & add one RAD to to do list
#
add_rad()
{
    [ -n "$1" ] ||
    {
	cat << !

$self: missing rad value.
!
	exit 1
    }

    [ "`echo $1 | sed 's/[0-9]*//'`" ] &&
    {
	cat << !

$self: bad rad value ($1).
!
    }

    [ "`(PROG1PRFX)pmhost -rad $1`" ] ||
    {
	cat << !

$self: rad "$1" is not configured on host "`(PROG1PRFX)pmhost`".
!
	exit 1
    }
    to_do_list="$to_do_list -rad=$1"
}

#
#	add_node - Verify & add one local node to "to do" list
#
add_node()
{
    [ -n "$1" ] ||
    {
	cat << !

$self: missing node value.
!
	exit 1
    }

    [ "`(PROG1PRFX)pmhost -node $1`" ] ||
    {
	cat << !

$self: node "$1" is not configured on host "`(PROG1PRFX)pmhost`".
!
	exit 1
    }
    to_do_list="$to_do_list -node=$1"
}

#
#	MAIN
#
cache_name=""
to_do_list=""
while [ -n "$1" ]
do
    case "$1" in
    -rad=*|-r=*)
	add_rad "`echo $1 | sed 's/-r[ad]*=\(.*\)/\1/'`"
	;;
    -rad|-r)
	shift
	add_rad "$1"
	;;
    -node=*|-n=*)
	add_node "`echo $1 | sed 's/-n[ode]*=\(.*\)/\1/'`"
	;;
    -node|-n)
	shift
	add_node "$1"
	;;
    -local)	# Clean all local RADs
	rads="`iinumainfo -radcount`"
	while [ $rads -gt 0 ]
	do
	    [ -n "`iipmhost -rad=$rads`" ] && add_rad $rads
	    rads=`expr $rads - 1`
	done
        ;;
    -cache=*)
        cache_name=`echo $1 | awk -F= '{ print $2 }'`
	;;
    *)	# dump usage
	mask="1"
	$CLUSTERSUPPORT && [ 0 -ne `iinumainfo -radcount` ] && mask="12"
	cat $0 | grep "^#[$mask]" | sed 's/^#.//'
	exit 1
	;;
    esac
    shift
done

if [ -n "$to_do_list" ]
then
    for target in $to_do_list
    do
	clean_shared "$target"
    done
else
    clean_shared
fi
