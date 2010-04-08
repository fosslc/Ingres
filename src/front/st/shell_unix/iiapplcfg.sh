:
# Copyright (c) 2009 Ingres Corporation
#
#  Name: iiapplcfg
#
#  Usage:
#  	iiapplcfg -t <cfg type> | -r
#	    -t	Configuration type where <cfg type> is one of
#		    TXN BI CM
#	    -r	Revert to system defaults
#
#
#  Description:
#	Applies a specified pre-defined configuration to Ingres which
# 	has been optimized for specific use cases.
#
#	    TXN - Transactional System
#	    BI - Business Intelligence System
#	    CM - Content Management System
#	    
#
#  Exit Status:
#       0       ok
#       1       bad command line option specified
#
#  PROGRAM = (PROG0PRFX)iiapplcfg
#
#  DEST = utility
## History:
##	09-Jul-2009 (hanje04)
##	    SIR 122309
##	    Created.
##	03-Aug-2009 (hanje04)
##	    SIR 122309
##	    Correct value for cursor_limit for BI settings. WAY too high
##	07-Oct-2009 (hanje04)
##	    Bug 122658
##	    connect_limit for BI should be 64 not the default (32).


basename=`basename $0`
validcfg="TXN BI CM"
cfgtype=""
revert=false
allparms="blob_etab_page_size
buffer_count
dmf_separate
p16k_status
p32k_status
cache_dynamic
connect_limit
cursor_limit
dmf_group_size
max_tuple_length
opf_active_limit
opf_joinop_timeout
opf_memory
opf_new_enum
opf_timeout_factor
system_isolation
system_lock_level
system_readlock
table_auto_structure
per_tx_limit
lock_limit"

. iisysdep
. iishlib


usage()
{
    cat << EOF
usage:
    $basename -t <cfg type> | -r
	-t	Configuration type where <cfg type> is one of
		    ${validcfg}
	-r	Revert to defaults

EOF
    exit 1
}

echo_cmd()
{
    ${ECHO_CMD} "$@\n"
}

echo_error()
{
    echo_cmd "ERROR: $1"
}

clean_exit()
{
    if [ -z "$1" ] || [ $1 == 0 ] ; then
	exit 0
    else
	if $revert ; then
	    echo_cmd "An error occurred whilst reverting to (PROD1NAME) defaults"
	else
	    echo_cmd "An error occurred whilst applying the '$cfgtype' configuration"
	fi
    fi
	exit $1
}
	
#
# Name:
# 	validcfg
#
# Description:
#	Check if passed argument is a valid conf type
# 		
# Params:
#	$1 -	Config type to parse
#       
# Returns:
#	0 - 	$1 is valid config type
#	1 -	$1 is invalid
#
validcfg()
{
    [ -z "$1" ] && return 1
    cfg=$1
    for c in $validcfg
    do
	[ $cfg = $c ] && return 0
    done
    return 1
}
  
   
#
# Name:
# 	apply_config_xxx()
#
# Description:
#	Apply config for configuration type xxx
#	Applies multiple functions.
# 		
# Params:
#	None
#       
# Returns:
#	0 - 	On Success
#	1 -	On Failure
#
apply_config_txn()
{
    rc=0
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.blob_etab_page_size"  16384  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.cursor_limit"  128  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.max_tuple_length"  0  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.opf_joinop_timeout"  100 &&
    iisetres -v +p "ii.${CONFIG_HOST}.dbms.*.opf_memory"  50000000  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.opf_new_enum"  ON  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.opf_timeout_factor"  1  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p8k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.cache.p16k_status" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p16k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.cache.p32k_status" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p32k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_isolation"  read_committed &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_lock_level"  row &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_maxlocks"  1200 &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.table_auto_structure"  ON  &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.lock.per_tx_limit"  3250 &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.log.buffer_count"  100
    rc=$?

    return $rc
}

apply_config_bi()
{
    rc=0
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.blob_etab_page_size"  16384 &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.cache_dynamic"  ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.connect_limit"  64  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.cursor_limit"  32  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.dmf_group_size"  32  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.max_tuple_length"  0 &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.opf_joinop_timeout"  100 &&
    iisetres -v +p "ii.${CONFIG_HOST}.dbms.*.opf_memory"  50000000 &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.opf_new_enum"  ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.opf_timeout_factor"  1 &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p8k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.cache.p16k_status" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p16k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.cache.p32k_status" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p32k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_isolation"  read_committed &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_lock_level"  row &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_maxlocks"  500 &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_readlock"  nolock &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.table_auto_structure"  ON &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.lock.per_tx_limit"  15000 &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.lock.lock_limit"  325000 &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.log.buffer_count"  50
    rc=$?

    return $rc
}

apply_config_cm()
{
    rc=0
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.blob_etab_page_size"  16384  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.connect_limit"  256  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.cursor_limit"  128  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.max_tuple_length"  0 &&
    iisetres -v +p "ii.${CONFIG_HOST}.dbms.*.opf_active_limit"  51 &&
    iisetres -v +p "ii.${CONFIG_HOST}.dbms.*.opf_memory"  23855104  &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p8k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.cache.p16k_status" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p16k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.cache.p32k_status" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.private.*.p32k.dmf_separate" ON &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_isolation"  read_committed &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_lock_level"  row &&
    iisetres -v "ii.${CONFIG_HOST}.dbms.*.system_readlock"  nolock  &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.lock.per_tx_limit"  15000 &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.lock.lock_limit"  100000 &&
    iisetres -v "ii.${CONFIG_HOST}.rcp.log.buffer_count"  200 
    rc=$?

    return $rc
}
# end of load_config_xxx() functions


#
# Name:
# 	apply_config()
#
# Description:
#	Apply a specified config type.
# 		
# Params:
#	Config to apply
#       
# Returns:
#	0 - 	On Success
#	1 -	On Failure
#
apply_config()
{
    rc=0
    case "$cfgtype" in
	TXN)
	    echo_cmd "Applying Transactional Configuration..."
 	    apply_config_txn
	    rc=$?
	    ;;
	BI) 
	    echo_cmd "Applying Buisiness Intelligence Configuration..."
	    apply_config_bi
	    rc=$?
	    ;;
	CM)
	    echo_cmd "Applying Content Management Configuration..."
	    apply_config_cm
	    rc=$?
	    ;;
	*)
	    echo_error "'$cfgtype' is an unknown configuration type"
	    rc=1
	    ;;
    esac
    return $rc
}

#
# Name:
# 	revert_config()
#
# Description:
#	Revert all parameters touched by this script to Ingres defaults
# 		
# Params:
#	None
#       
# Returns:
#	0 - 	On Success
#	1 -	On Failure
#
revert_config()
{
    rc=0

    # unprotect things first values are arbitrary
    echo_cmd "Reverting to (PROD1NAME) System Defaults..."
    iisetres -p "ii.${CONFIG_HOST}.dbms.*.opf_active_limit"  32
    iisetres -p "ii.${CONFIG_HOST}.dbms.*.opf_memory"  10240000
    for p in $allparms
    do
	iiinitres $p $II_CONFIG/dbms.rfm
	rc=$?
	[ $rc != 0 ] && break
    done
    return $rc
}

#
# main() body of script starts here
#
echo_cmd "(PROD1NAME)/$basename\n"
while [ $# -gt 0 ]
do
    case $1 in
	-t) if validcfg $2 ; then
		cfgtype=$2
		shift ; shift
		break
	    else
		echo_error "Invalid config type '$2'"
		usage
	    fi
	    ;;
	-r) revert=true
	    shift
	    break
	    ;;
	*)
	    echo_error "Invalid parameter: '$1'"
	    usage
    esac
done

if $revert ; then
    revert_config
    rc=$?
elif [ "$cfgtype" ] ; then
    apply_config 
    rc=$?
else
    usage
fi

echo_cmd

[ $rc = 0 ] && echo_cmd Done
clean_exit $rc
