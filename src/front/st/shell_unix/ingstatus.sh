:
# Copyright (c) 2005, 2010 Ingres Corporation
#

#
#  Name:  
#	ingstatus -- Reports running Ingres processes.
#
#  Usage: 
#	ingstop
#
#  Description: 
#	This program reports running processes in an installation.
#
#  Exit Status:
#	0	the requested operation succeeded.
#	1	the requested operation failed.
#
## History:
##	11-Feb-2005 (hanje04)
##		Created from ingstop.
##      25-Apr-2007 (hanal04) Bug 118194
##              JDBC Server has been deprecated. Removed all references
##              to the JDBC Server.
##      04-Jun-2009 (shust01) Bug 122139
##              program was not showing the status of the db2udb gateway
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG2PRFX)status
#
#  DEST = utility
#----------------------------------------------------------------------------
(LSBENV)

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib

#
#	cleanup()
#
#	Remove temporary work files on exit
#
cleanup()
{
    [ -z "$debugging" ] && rm -f /tmp/*.$$ 2>/dev/null
}

#
# Checks if server $1 is still running using method $2 ("ps" or "cs"report).
# Returns 0 if running, non-zero otherwise.  If on a NUMA host, and 
# method = cs, $3 must be RAD affinity argument.
#
server_active()
{
   scan_id=$1
   scan_method=$2

   pid=`grep "connect id $scan_id" /tmp/csreport.$$ | awk '{ print $4 }'`
   [ -z "$pid" ] && return 1
 
   if [ "$scan_method" = "cs" ] ; then
      [ `csreport | grep "inuse 1" | tr , \ | grep "pid $pid" | wc -l` -eq 1 ]
   else
      [ `$PSCMD | awk '{print $2}' | grep "^${pid}\$" | wc -l` -eq 1 ]
   fi
   return $?
}

#
# Waits no more than $2 seconds for server $1 to exit.  Escalates time spent
# napping between server checks (Fibonacci series); $3 dictates scanning
# method employed (either "ps" or "cs"report).
# Return 0 if server process has exited; non-zero otherwise.
#
wait_on_server()
{
   scan_id=$1
   max_wait_on_server=`expr $2 + 1`
   scan_method=$3

   cur_time=0
   cum_time=1
   new_time=1
   server_up=1
   while [ $cum_time -le $max_wait_on_server ]
   do
      sleep $cur_time
 
      server_active $scan_id $scan_method ||
      {
         server_up=0
         break
      }
 
      cur_time=$new_time
      new_time=$cum_time
      cum_time=`expr $cum_time + $cur_time`
   done
 
   return $server_up
}

#  Writes messages to both stdout and "ingstart.log"
#
#  Supply
#    $*		: message(s) to write; if no args, read from stdin
#
#  Return:
#    0		: OK
#
wmsg()
{
    if [ $# -eq 0 ] ; then
	cat -     | sed 's/^/!/' | sed 's/^.//'
    else
	echo -e "$*" | sed 's/^/!/' | sed 's/^.//'
    fi
    return 0
}

#  Scans for requested list of object ids in supplied file; assumes file has
#  leading and trailing whitespaces removed (for accurate grepping).
# 
#  Supply:
#    $1		: <file> containing active objects (one object per line)
#    $2		: object type/description
#    $3-n	: list of objects to scan
#
#  Return:
#    $?		: status of call; 0 OK, != 0 fail
#    <stdout>	: number of matches
#    <stderr>	: diagnostics; should be written to stdout by caller
#
#  Global:
#    <file>	: overwritten list of object ids (those selected)
#
scan_oid()
{

    oidpres="$1"
    oiddesc="$2"
    [ ! -r "$oidpres" ] && echo "0" && return 0

    cat /dev/null > /tmp/oidlist.$$
    oidmiss=""
    if [ "$3" = "all" ] ; then
	cat $oidpres			>> /tmp/oidlist.$$
    else
	shift
	shift
	for oid in $* ; do
	    grep "^$oid$" $oidpres	>> /tmp/oidlist.$$
	    [ $? -ne 0 ] && oidmiss="$oidmiss $oid"
	done
	oidmiss=`echo "$oidmiss" | sed 's/^  *//'`
    fi
    if [ -n "$oidmiss" ] ; then
	oidcurr=`cat $oidpres | tr -s '\012' ' '`
	oidhits=`cat /tmp/oidlist.$$ | tr -s '\012' ' '`
	echo "An inputted $oiddesc id(s) is not currently active." 1>&2
	echo "   Ignoring:  $oidmiss" 1>&2
	echo "     Active:  $oidcurr" 1>&2
	echo "  Targeting:  ${oidhits:-<none>}" 1>&2
	echo "" 1>&2
    fi
    sort -u /tmp/oidlist.$$ > $oidpres
    oidcount=`wc -l < $oidpres | tr -d ' '`

    echo $oidcount
    return 0
}

#  Reports status of supplied object
# 
#  Supply:
#    $1		: count
#    $2		: max allowed
#    $3		: product installed (1 = true, 0 = false)
#    $4		: object type/description; example "name server (iigcn)"
#
#  Return:
#    $?		: status of call; 0 OK, != 0 fail
#
report_oid()
{
(
    oid_cnt="$1"
    max_cnt="$2"
    instald="$3"
    oiddesc="$4"

    pluralize="sed -e 's/server/servers/' -e 's/process/processes/'"
    service="Ingres $II_INSTALLATION"

    if [ $oid_cnt -eq 1 ] ; then
        # echo "The name server (iigcn) is running."
        # echo "There is one ICE server (icesvr) running."
        if [ "$max_cnt" -eq 1 ] ; then
            printf "%-39s%6s%-12s\n" "${service} ${oiddesc}" "-" " running"
        else
            printf "%-39s%6s%-12s\n" "${service} ${oiddesc}" "-" " 1 running"
        fi

    elif [ $oid_cnt -gt 1 ] ; then
        # echo "There are too many ($oid_cnt) name servers (iigcn) running."
        # echo "There are $icesvr_count ICE servers (icesvr) running."
        rptmsg0="$oid_cnt"
        [ "$oid_cnt" -gt "$max_cnt" ] && rptmsg0="too many ($oid_cnt)"
        printf "%-39s%6s%-12s\n" "${service} ${oiddesc}" "-" " $rptmsg0 running"

    elif [ $oid_cnt -eq 0 -a $instald -eq 1 ] ; then
        # echo "There is no name server (iigcn) running."
        # echo "There are no Oracle Gateway servers running."
            printf "%-39s%6s%-12s\n" "${service} ${oiddesc}" "-" " not active"
    fi

    return 0
)
}

#
#	cleanup_shared_mem
#
#
#	Wait for any background processing to complete and collect logs.
#
report_remote_results()
{
    iteration=0
    while [ -n "${STOPPIDS}" ]
    do
	$PSCMD > /tmp/ps.$$
	for rinfo in $STOPPIDS
	do
	    stoppid="`echo $rinfo | sed 's/^.*\.//'`"
	    nodename="`echo $rinfo | sed 's/\..*$//'`"
	    mask="\$2==${stoppid} { print 1 }"
	    if [ -z "`awk \"$mask\" /tmp/ps.$$`" ]
	    then
		# Display results for remote ingstop (don't log it)
		cat << ! | center_text 76 | box 76 1 0 | wmsg
(PROD1NAME)/(PROG2PRFX)stop results for $nodename
!
		cat /tmp/$self_$nodename.$$

		# remove it from list
		mask="s/ `echo $rinfo | sed 's/[^ _A-Za-z0-9()]/\\\\&/g'` / /"
		STOPPIDS="`echo \"${STOPPIDS} \" | \
		 sed -e \"${mask}\" -e 's/ \$//'`"
		[ -z "$STOPPIDS" ] && break 2
	    elif [ iteration -eq 0 ]
	    then
		# Whine about delay
		wrap_lines 76 << ! | wmsg

Waiting for $nodename to shutdown ...
!
	    fi
	done
	iteration=`expr $iteration + 1`
	[ iteration -ge 10 ] && iteration=0
	sleep 6
    done
    return 0
}

#
#	exec_locals
#
#	Run local sub-processes to shut down other NUMA nodes on host.
#
#	in:	components - list of components to shut down.
#		targets - list of NUMA nodes this host.
#		STOPPIDS - list of pids for sub-processes.
#
#	out:	STOPPIDS - argumented with additional sub-process pids.
#
exec_locals()
{
    # Check if no need for action against other nodes this host.
    [ -n "$components" ] && 
    {
	notfirst=false
	for node in $targets
	do
	    if $notfirst
	    then
		$self -node $node $components $options \
		 > /tmp/$self_$node.$$  2>&1 &
		STOPPIDS="${STOPPIDS} ${node}.$!"
	    fi
	    notfirst=true
	done
    }
}

#
#	cluster_opt_check - check if cluster only option enabled.
#
cluster_opt_check()
{
    $CLUSTERSUPPORT ||
    {
	wrap_lines 76 << ! | wmsg

$self: $1 parameter is only supported in a clustered configuration.

!
	exit 1
    }
}

#
#	append_node - add node to list if not a duplicate.
#
#	$1	- node name
#	$2	- value node was derived from.
#	$3	- value type.
#
append_node()
{
    [ -z "$1" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: $3 "$2" is not configured in the cluster.

!
	exit 1
    }
    if in_set "$targets" "$1" 
    then
	# duplicate, don't want it
	:
    else
	targets="$targets $1"
    fi
    return 0
}

#
#	add_host - Add all nodes for a host to target set
#
add_host()
{
    in_set "$target_hosts" $1 ||
    {
	# Add all vnodes this host to target_sets
	target_hosts="$target_hosts $1"
	target_sets="$target_sets *"
    }
    return 0
}

#
#	add_node - Evaluate argument as a potential target node.
#
add_node()
{
    [ -z "$1" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: $badarg argument value missing.

!
	exit 1
    }
    node="`(PROG1PRFX)pmhost -node=$1`"
    [ -z "$node" ] && 
    {
	in_set "$host_names" $1 &&
	{
	    add_host $1
	    return 0
	}
    }
    append_node "$node" "$1" "node name"
    return 0
}

#
#	add_rad - Evaluate argument as a target RAD spec.
#
add_rad()
{
    cluster_opt_check $badarg

    [ -z "$1" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: $badarg argument value missing.

!
	exit 1
    }

    [ -n "`echo $1 | sed 's/[0-9]*//'`" ] &&
    {
	wrap_lines 76 << ! | wmsg

$self: bad syntax for RAD value ($1).

!
	exit 1
    }
    append_node "`(PROG1PRFX)pmhost -rad=$1`" "$1" "RAD"
    return 0
}
    
#
#	MAIN
#

#
# Start initialization
#
rm -f /tmp/*.$$ 2>/dev/null
KILL=false
IMMEDIATE=false
STOPPIDS=""
self=`basename $0`
hostname=`(PROG1PRFX)pmhost`
if [ x"${conf_LSB_BUILD}" = x"TRUE" ] ; then
    config_dat="$(PROG3PRFX)_ADMIN/config.dat"
else
    config_dat="$(PROG3PRFX)_CONFIG/config.dat"
fi

#
# See if we have to worry about cluster stuff.
#
if $CLUSTERSUPPORT
then
    # Only enable cluster options if supportable and configured.
    [ "1" = "`ingprenv II_CLUSTER`" ] || CLUSTERSUPPORT=false
fi

if $CLUSTERSUPPORT
then
    #
    # Allow -cluster, -node, -rad & -local options.  Get data
    # needed to validate input for cluster options.  Redirect log.
    #

    host_names=""
    numa_nodes="`grep -i \
	'^(PROG1PRFX)\.[^.]*\.config.numa.rad:[ 	]*[1-9][0-9]*$' \
	$config_dat | awk -F. '{ printf \" %s\",\$2 }'`"
    if [ -n "$numa_nodes" ]
    then
	# We have at least one NUMA configured node.
	# Collect Host names for these puppies.
	for node in $numa_nodes
	do
	    host=`(PROG1PRFX)getres "(PROG1PRFX).$node.config.host"`
	    [ -z "$host" ] &&
	    {
		error_box 76 << ! | wmsg
Configuration shows node "$node" configured as a NUMA virtual node, but no host name for this node was found.
!
		exit 1
	    }
	    host_names="$host_names $host"
	done

	# Don't allow Implicit RAD context.
	II_DEFAULT_RAD=
	export II_DEFAULT_RAD
    fi
fi

minutes=0
seconds=0;

##
##  Allow user to specify which object to query;
##
##  Objects to date (2.5) 	var_suffix	identify
##  ---------------------	----------	--------
##	iigcn			gcn		(PROG1PRFX)gcfid
##	dmfrcp			rcp		ps
##	dmfacp			acp		ps
##	iidbms[=<idlist>]	dbms		ps,csreport
##	iistar[=<idlist>]	star		(PROG1PRFX)gcfid
##	icesvr[=<idlist>]	ice		(PROG1PRFX)gcfid
##	rmcmd			rmcmd		ps
##	iigcc[=<idlist>]	gcc		(PROG1PRFX)gcfid
##	iigcb[=<idlist>]	gcb		(PROG1PRFX)gcfid
##	oracle[=<idlist>]	ora		(PROG1PRFX)gcfid
##	sybase[=<idlist>]	syb		(PROG1PRFX)gcfid
##	informix[=<idlist>]	inf		(PROG1PRFX)gcfid
##	db2udb[=<idlist>]	udb		(PROG1PRFX)gcfid
##
##
##  Objects to date (2.65)      var_suffix      identify
##  ---------------------       ----------      --------
##      iigcd[=<idlist>]        gcd             (PROG1PRFX)gcfid

statids_gcn=""
statids_rcp=""
statids_acp=""
statids_rmcmd=""
statids_ice=""
statids_dbms=""
statids_gcc=""
statids_gcb=""
statids_gcd=""
statids_star=""
statids_ora=""
statids_syb=""
statids_inf=""
statids_udb=""

# Set option & target sets to empty
options=""
targets=""
nontargets=""
target_hosts=""
target_sets=""
components=""
debugging=""

while [ -n "$1" ]
do
    badarg="$1"
    case "$1" in

	-cluster|-c)
	    # Act against all nodes
	    cluster_opt_check $1

	    targets="`grep -i \
	'^(PROG1PRFX)\.[^.]*\.config.cluster.id:[ 	]*[1-9][0-9]*$' \
	$config_dat | awk -F. '{ printf \" %s\",\$2 }'`"
	    ;;

	-local)
	    cluster_opt_check $1
	    add_host $hostname
	    ;;

	-node|-n)
	    shift
	    add_node "$1"
	    ;;

	-node=*|-n=*)
	    add_node "`echo $1 | sed 's/-n[ode]*=\(.*\)/\1/'`"
	    ;;

	[A-Za-z][A-Za-z_0-9]*)
	    # Node name
	    add_node "$1"
	    ;;

	-rad|-r)
	    shift
	    add_rad "$1"
	    ;;

	-rad=*|-r=*)
	    add_rad "`echo $1 | sed 's/-r[ad]*=\(.*\)/\1/'`"
	    ;;

	-(PROG1PRFX)gcn|-dmfrcp|-dmfacp|-rmcmd)
	    components="$components $1"
	    partial=true
	    case $1 in
		-(PROG1PRFX)gcn)		statids_gcn="all" ;;
		-dmfrcp)	statids_rcp="all" ;;
		-dmfacp)	statids_acp="all" ;;
		-rmcmd)		statids_rmcmd="all" ;;
	    esac
	    ;;

	-icesvr*|-(PROG1PRFX)dbms*|-(PROG1PRFX)gcc*|-(PROG1PRFX)gcd*|-(PROG1PRFX)gcb*|-(PROG1PRFX)star*|\
	-oracle*|-sybase*|-informix*|-db2udb*)
	    components="$components $1"
	    partial=true
	    case $1 in
		-(PROG1PRFX)dbms*)	sufx="dbms" ;;
		-(PROG1PRFX)star*)	sufx="star" ;;
		-icesvr*)	sufx="ice" ;;
		-(PROG1PRFX)gcc*)	sufx="gcc" ;;
                -(PROG1PRFX)gcd*)       sufx="gcd" ;;
		-(PROG1PRFX)gcb*)	sufx="gcb" ;;
		-oracle*)	sufx="ora" ;;
		-sybase*)	sufx="syb" ;;
		-informix*)	sufx="inf" ;;
		-db2udb*)	sufx="udb" ;;
	    esac

	    if [ `expr "$1" : ".*=."` -gt 0 ] &&
	       [ `expr "$1" : ".*[=,]all"` -eq 0 ] ; then
		oids=`echo "$1" | sed 's/^[^=]*=//' | tr ',' ' '`
		eval "statids_${sufx}=\"\$statids_${sufx} $oids\""
	    else
		eval "statids_${sufx}='all'"
	    fi
	    ;;
	-debug)
	    options="$options $1"
	    debugging="Y"
	    set -x
	    ;;
	*)
	    wrap_lines 76 << ! | wmsg

$self: Bad argument ($1).

Usage: $self
!
	    $CLUSTERSUPPORT && \
		wrap_lines 76 << ! | wmsg
 [ -c[luster] ] [ [ [-n[ode]] node_name | nick_name ] | -r[ad] radid ... ]
!
	    
	    wmsg ""
	    exit 1
	    ;;
    esac
    shift
done

if [ -z "$targets" ]
then
    # Normal case.  No nodes were specified, just do a full stop on local host. 
    foreground="$hostname"
    target_hosts="$hostname"
    target_sets="."
fi

if [ -n "$targets" -o -z "`(PROG1PRFX)pmhost -node $hostname`" ] 
then
    #
    # Cluster case, and one or more target nodes was listed,
    # or local node is a NUMA node.
    #
    # Group target nodes into sets by host
    #
    targn=0
    for node in $targets
    do
	targn=`expr $targn + 1`
	if in_set "$numa_nodes" $node
	then
	    #
	    # For NUMA nodes, see if all virtual nodes for a host
	    # have been specified.
	    #
	    nodehost=`nth_element "$host_names" \
	      \`element_index "$numa_nodes" $node\``
	    in_set "$target_hosts" $nodehost && continue

	    hostn=0
	    nodeset=".${node}"
	    allhits=true 
	    for host in $host_names
	    do
		hostn=`expr $hostn + 1`
		if [ "$host" = "$nodehost" ]
		then
		    numanode=`nth_element "$numa_nodes" $hostn`
		    [ "$numanode" = "$node" ] && continue
		    in_set "$targets" $numanode &&
		    {
			nodeset="${nodeset}.${numanode}"
			continue;
		    }
		    allhits=false
		fi
	    done
	    
	    target_hosts="$target_hosts $nodehost"
	    if $allhits
	    then
		target_sets="$target_sets ."
	    else
		target_sets="$target_sets $nodeset"
	    fi
	else
	    #
	    # Regular cluster node.  host = node in this case
	    #
	    in_set "$target_hosts" $node && continue
	    target_hosts="$target_hosts $node"
	    target_sets="$target_sets ."
	fi
    done

    #
    # If current host is in the set of target_hosts, it is the foreground
    #
    foreground=""
    in_set "$target_hosts" "$hostname" && foreground="$hostname"
    if [ -z "$foreground" ]
    then
	for host in $target_hosts
	do
	    foreground="$host"
	    break
	done
    fi

    #
    # Launch any background processing
    #
    targets=""		# 'targets' being reused as set of local NUMA nodes.
    hostn=0
    for host in $target_hosts
    do
	hostn=`expr $hostn + 1`

	# Retreive set of nodes for this host
	nodeset=`nth_element "$target_sets" $hostn`
	if [ "$nodeset" = "." ]
	then
	    nodeset=""
	else
	    nodeset="`echo \"$nodeset\" | sed 's/\./-n /g'`"
	fi

	[ "$host" = "$hostname" ] &&
	{
	    targets="`echo $nodeset | sed 's/\./ /'`"
	    continue
	}

	# Resolve machine name for remote host
	machid=`(PROG1PRFX)getres \
	       "(PROG1PRFX).${host}.gcn.local_vnode"`
	[ -z "$machid" ] && 
	{
	    warning_box 76 << ! | wmsg
$self: Cannot resolve network address for host "${host}"
!
	    continue
	}

	if [ "$host" = "$foreground" ]
	then
	    rsh "$machid" $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)rsh \
	     $(PRODLOC) $self $nodeset $components $options
	else
	    rsh "$machid" $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)rsh \
	     $(PRODLOC) $self $nodeset $components $options \
	     > /tmp/$self_$host.$$  2>&1 &
	    STOPPIDS="${STOPPIDS} ${host}.$!"
	fi
    done

    if [ "$foreground" = "$hostname" ]
    then
	if in_set "$host_names" "$hostname"
	then
	    # Flesh out set of local target nodes, and
	    # create a set of spared local nodes
	    
	    hostn=0
	    for host in $host_names
	    do
		hostn=`expr $hostn + 1`
		if [ "$host" = "$hostname" ]
		then
		    node=`nth_element "$numa_nodes" $hostn`
		    in_set "$targets" $node || nontargets="$nontargets $node"
		fi
	    done
	    if [ -z "$targets" ]
	    then
		targets="$nontargets"
		nontargets=""
	    fi

	    numa_parts="-dmfrcp -dmfacp -(PROG1PRFX)dbms -(PROG1PRFX)star -icesvr"
	    if [ -z "$components" ]
	    then
		components="$numa_parts"
	    else
		oldcomp="$components"
		components=""
		for part in $oldcomp
		do
		    in_set "$numa_parts" $part && \
		     components="$components $part"
		done
	    fi

	    for node in $targets
	    do
		# Use 1st NUMA node for operations on common processes.
		rad=`(PROG1PRFX)getres "(PROG1PRFX).$node.config.numa.rad"`
		[ -z "$rad" ] &&
		{
		    fatal_box 76 << ! | wmsg
Configuration error.  Connot find RAD id for NUMA node "$node".
!
		    exit 1
		}
		II_DEFAULT_RAD="$rad"
		export II_DEFAULT_RAD

		# Process other nodes (if any) later.
		break
	    done
	fi
    fi
    node=`(PROG1PRFX)pmhost -local`
    if [ -n "$STOPPIDS" ]
    then
	wrap_lines 76 << ! | wmsg

Processing for the other node(s) specified will go on in the background,
with the results reported when foreground processing is complete.
!
    fi
fi

[ "$foreground" = "$hostname" ] ||
{
    #
    # No local operations, just wait & report results
    #
    report_remote_results

    [ -z "$debugging" ] && rm -f /tmp/*.$$ 2>/dev/null
    exit 0
}

#
# From this point on, all operations are against one node,
# with the NUMA context if any set by II_DEFAULT_RAD.
#
stat_all=0
if [ -z "$statids_gcn$statids_rcp$statids_acp$statids_rmcmd" ] &&
   [ -z "$statids_dbms$statids_star$statids_ice$statids_gcc" ] &&
   [ -z "$statids_gcb$statids_ora$statids_syb" ] &&
   [ -z "$statids_gcd$statids_inf$statids_udb" ]
then
    if [ -z "$nontargets" ]
    then
	stat_all=1
	statids_gcn="all"
	statids_rmcmd="all"
	statids_gcc="all"
	statids_gcd="all"
	statids_gcb="all"
	statids_ora="all"
	statids_syb="all"
	statids_inf="all"
	statids_udb="all"
    fi
    statids_rcp="all"
    statids_acp="all"
    statids_dbms="all"
    statids_star="all"
    statids_ice="all"
fi

if [ "$statids_rcp" = "all" ] ; then
    statids_rcp="all"
    statids_acp="all"
    statids_dbms="all"
    statids_star="all"
    statids_ice="all"
    [ -n "$nontargets" ] || statids_rmcmd="all"
fi

# set trap on exit to clean up temporary files
trap "cleanup ; exit 1" 1 2 3 15

if grep -i "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
   server_host=true
else
   server_host=false
fi    

(PROG3PRFX)_INSTALLATION=`(PROG2PRFX)prenv (PROG3PRFX)_INSTALLATION`

icesvr_count=0 # number of ice server processes
dbms_count=0   # number of dbms server processes
rcp_count=0    # number of recovery processes
acp_count=0    # number of archiver processes
star_count=0   # number of star server processes
gcc_count=0    # number of comm server processes
gcd_count=0    # number of DAS server processes
gcb_count=0    # number of bridge server processes
gcn_count=0    # number of name server processes
ora_count=0    # number of oracle gateway server processes
syb_count=0    # number of sybase gateway server processes
inf_count=0    # number of informix gateway server processes
udb_count=0    # number of db2udb gateway server processes
rmcmd_count=0  # number of rmcmd server processes
shut_proc=0    # number of processes that are actually shutdown

$PSCMD > /tmp/ps.$$

WSRE='[	 ][	]*'
SEDTRIM="s/^$WSRE//;s/$WSRE$//"
IIGCFID_FILTER="sed -e 's/ *(.*)//' -e '$SEDTRIM' | sort -u 2> /dev/null"

if [ -n "$statids_gcn" ] ; then

    # get name server status, report count
    #
    gcn_count=`eval "(PROG1PRFX)gcfid (PROG1PRFX)gcn | $IIGCFID_FILTER" | wc -l | tr -d ' '`
    report_oid $gcn_count 1 1 "name server ((PROG1PRFX)gcn)"
fi


# Initialize temp files used to record current active objects
#
for f in psinst (PROG0PRFX)csreport (PROG1PRFX)dbms (PROG1PRFX)star icesvr (PROG1PRFX)gcc (PROG1PRFX)gcd (PROG1PRFX)gcb (PROG1PRFX)ora (PROG1PRFX)syb (PROG1PRFX)inf (PROG1PRFX)udb
do
    cat /dev/null > /tmp/$f.$$
done


if [ -n "$statids_rcp" -o -n "$statids_dbms" -o -n "$statids_ice" ] ||
   [ -n "$statids_acp" -o -n "$statids_star" -o -n "$statids_rmcmd" ] ; then

    # get status of Ingres processes from csreport
    #
    for pid in `(PROG0PRFX)csreport | grep "inuse 1" | tr , \  | tee /tmp/(PROG0PRFX)csreport.$$ | \
       awk '{ print $4 }'` 
    do
       grep $pid /tmp/ps.$$ >> /tmp/psinst.$$
    done
fi

if [ -n "$statids_rcp" ] ; then

    # get status of recovery server, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
	recovery_installed=1
    else
	recovery_installed=0
    fi
    rcp_count=`grep recovery /tmp/psinst.$$ | wc -l | tr -d ' '`
    report_oid $rcp_count 1 $recovery_installed "recovery server (dmfrcp)"
fi

if [ -n "$statids_ice" ] ; then

    # get list of ICE servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.icesvr" $config_dat >/dev/null ; then
	icesvr_installed=1
    else
	icesvr_installed=0
    fi
    ois="ICE server (icesvr)"
    eval "(PROG1PRFX)gcfid icesvr | $IIGCFID_FILTER" > /tmp/icesvr.$$ 
    icesvr_count=`wc -l < /tmp/icesvr.$$ | tr -d ' '`
    report_oid $icesvr_count 100 $icesvr_installed "$ois"

    if [ $icesvr_count -ne 0 ] ; then
	icesvr_count=`scan_oid /tmp/icesvr.$$ "$ois" $statids_ice \
		2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_dbms" ] ; then

    # get list of DBMS servers, report count
    #
    for pid in `grep (PROG1PRFX)dbms /tmp/psinst.$$ | grep -v "/(PROG1PRFX)dbms recovery" | \
       grep -v "/(PROG1PRFX)star star" | awk '{ print $2 }'`
    do
       grep "pid $pid" /tmp/(PROG0PRFX)csreport.$$ | awk '{ print $7 }' >> /tmp/(PROG1PRFX)dbms.$$
    done

    # sort list of DBMS servers 
    #
    sort /tmp/(PROG1PRFX)dbms.$$ | sed "$SEDTRIM" > /tmp/s(PROG1PRFX)dbms.$$
    mv /tmp/s(PROG1PRFX)dbms.$$ /tmp/(PROG1PRFX)dbms.$$

    if grep "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
	dbms_installed=1
    else
	dbms_installed=0
    fi
    ois="DBMS server ((PROG1PRFX)dbms)"
    dbms_count=`wc -l < /tmp/(PROG1PRFX)dbms.$$ | tr -d ' '`
    report_oid $dbms_count 100 $dbms_installed "$ois"

    if [ $dbms_count -ne 0 ] ; then
	dbms_count=`scan_oid /tmp/(PROG1PRFX)dbms.$$ "$ois" $statids_dbms 2> \
		    /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_star" ] ; then

    # get list of Star servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*star" $config_dat >/dev/null ; then
	star_installed=1
    else
	star_installed=0
    fi
    ois="Star server ((PROG1PRFX)star)"
    eval "(PROG1PRFX)gcfid (PROG1PRFX)star | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)star.$$ 
    star_count=`wc -l < /tmp/(PROG1PRFX)star.$$ | tr -d ' '`
    report_oid $star_count 100 $star_installed "$ois"

    if [ $star_count -ne 0 ] ; then
	star_count=`scan_oid /tmp/(PROG1PRFX)star.$$ "$ois" $statids_star \
	    	2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_gcc" ] ; then

    # get list of Net servers, report count
    #
    ois="Net server ((PROG1PRFX)gcc)"
    eval "(PROG1PRFX)gcfid (PROG1PRFX)gcc | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)gcc.$$ 
    gcc_count=`wc -l < /tmp/(PROG1PRFX)gcc.$$ | tr -d ' '`
    report_oid $gcc_count 100 1 "$ois"

    if [ $gcc_count -ne 0 ] ; then
	gcc_count=`scan_oid /tmp/(PROG1PRFX)gcc.$$ "$ois" $statids_gcc 2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_gcd" ] ; then

    # get list of DAS servers, report count
    #
    ois="Data Access server ((PROG1PRFX)gcd)"
    eval "(PROG1PRFX)gcfid (PROG1PRFX)gcd | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)gcd.$$
    gcd_count=`wc -l < /tmp/(PROG1PRFX)gcd.$$ | tr -d ' '`
    report_oid $gcd_count 100 1 "$ois"

    if [ $gcd_count -ne 0 ] ; then
        gcd_count=`scan_oid /tmp/(PROG1PRFX)gcd.$$ "$ois" $statids_gcd 2> /tmp/(PROG1PRFX)itmp.$$`
        [ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_gcb" ] ; then

    # get list of Bridge servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*gcb" $config_dat >/dev/null ; then
	gcb_installed=1
    else
	gcb_installed=0
    fi
    ois="Bridge server ((PROG1PRFX)gcb)"
    eval "(PROG1PRFX)gcfid (PROG1PRFX)gcb | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)gcb.$$ 
    gcb_count=`wc -l < /tmp/(PROG1PRFX)gcb.$$ | tr -d ' '`
    report_oid $gcb_count 100 $gcb_installed "$ois"

    if [ $gcb_count -ne 0 ] ; then
	gcb_count=`scan_oid /tmp/(PROG1PRFX)gcb.$$ "$ois" $statids_gcb 2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_rmcmd" ] ; then

    # get status of RMCMD process, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.rmcmd" $config_dat >/dev/null ; then
	rmcmd_installed=1
    else
	rmcmd_installed=0
    fi
    grep "rmcmd $(PROG3PRFX)_INSTALLATION" /tmp/ps.$$ > /tmp/rmcmd.$$
    rmcmd_count=`wc -l < /tmp/rmcmd.$$ | tr -d ' '`
    report_oid $rmcmd_count 1 $rmcmd_installed "RMCMD process (rmcmd)"
fi

if [ -n "$statids_acp" ] ; then

    # get status of archiver process, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*dbms" $config_dat >/dev/null ; then
	archiver_installed=1
    else
	archiver_installed=0
    fi
    acp_count=`grep dmfacp /tmp/psinst.$$ | wc -l | tr -d ' '`
    report_oid $acp_count 1 $archiver_installed "archiver process (dmfacp)"
fi

if [ -n "$statids_ora" ] ; then

    # get list of Oracle Gateway servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*oracle" $config_dat >/dev/null ; then
       oracle_installed=1
    else
       oracle_installed=0
    fi
    ois="Oracle Gateway server"
    eval "(PROG1PRFX)gcfid oracle | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)ora.$$
    ora_count=`wc -l < /tmp/(PROG1PRFX)ora.$$ | tr -d ' '`
    report_oid $ora_count 100 $oracle_installed "$ois"

    if [ $ora_count -ne 0 ] ; then
	ora_count=`scan_oid /tmp/(PROG1PRFX)ora.$$ "$ois" $statids_ora 2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_syb" ] ; then

    # get list of Sybase Gateway servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*sybase" $config_dat >/dev/null ; then
       sybase_installed=1
    else
       sybase_installed=0
    fi
    ois="Sybase Gateway server"
    eval "(PROG1PRFX)gcfid sybase | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)syb.$$
    syb_count=`wc -l < /tmp/(PROG1PRFX)syb.$$ | tr -d ' '`
    report_oid $syb_count 100 $sybase_installed "$ois"

    if [ $syb_count -ne 0 ] ; then
	syb_count=`scan_oid /tmp/(PROG1PRFX)syb.$$ "$ois" $statids_syb 2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_inf" ] ; then

    # get list of Informix Gateway servers, report count
    #
    if grep "$hostname.*(PROG2PRFX)start.*informix" $config_dat >/dev/null ; then
       informix_installed=1
    else
       informix_installed=0
    fi
    ois="Informix Gateway server"
    eval "(PROG1PRFX)gcfid informix | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)inf.$$
    inf_count=`wc -l < /tmp/(PROG1PRFX)inf.$$ | tr -d ' '`
    report_oid $inf_count 100 $informix_installed "$ois"

    if [ $inf_count -ne 0 ] ; then
	inf_count=`scan_oid /tmp/(PROG1PRFX)inf.$$ "$ois" $statids_inf 2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi

if [ -n "$statids_udb" ] ; then

    # get list of DB2 UDB Gateway servers, report count
    #
    if grep "$pmhost.*(PROG2PRFX)start.*db2udb" $config_dat >/dev/null ; then
       db2udb_installed=1
    else
       db2udb_installed=0
    fi
    ois="DB2 UDB Gateway server"
    eval "(PROG1PRFX)gcfid db2udb | $IIGCFID_FILTER" > /tmp/(PROG1PRFX)udb.$$
    udb_count=`wc -l < /tmp/(PROG1PRFX)udb.$$ | tr -d ' '`
    report_oid $udb_count 100 $db2udb_installed "$ois"

    if [ $udb_count -ne 0 ] ; then
	udb_count=`scan_oid /tmp/(PROG1PRFX)udb.$$ "$ois" $statids_udb 2> /tmp/(PROG1PRFX)itmp.$$`
	[ -s "/tmp/(PROG1PRFX)itmp.$$" ] && cat /tmp/(PROG1PRFX)itmp.$$
    fi
fi


# We're done, now clean up and exit
   if [ "$stat_all" -ne 0 ] ; then
	statscope="in"
   else
	statscope="matching your request in"
   fi
proc_count=`eval expr $dbms_count + $star_count + $icesvr_count + \
   $gcc_count  + $gcd_count + $gcb_count  +  $gcn_count + \
   $rcp_count  + $acp_count + $rmcmd_count + $ora_count  + $syb_count + \
   $inf_count  + $udb_count `

[ -n "$targets" ] && exec_locals

if [ -n "$STOPPIDS" ]
then
    cat << !
Processing of ingstop on remote nodes will continue
!
	report_remote_results
 fi
 [ -z "$debugging" ] && rm -f /tmp/*.$$ 2>/dev/null
 exit 0
