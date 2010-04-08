:
# Copyright (c) 1993, 2010 Ingres Corporation
#
#
#
#  Name: iishlib -- shared functions used by INGRES setup programs. 
#
#  Usage:
#	. iishlib 
#
#  Description:
#	Executed in-line by various setup scripts to access shared
#	functions.
#
## History:
##	21-jun-93 (tyler)
##		Created.  History comments from contributing files
##		has not been preserved.  It's possible that this technique
##		won't work across all Bourne shells - hence the individual 
##		shell scripts aren't being removed right away in case the
##		change needs to be backed out.  Also, the commands used
##		to invoke the original shell scripts have been preserved
##		as comments in iisudbms et al for now.
##	25-jun-93 (tyler)
##		Modified $BATCH tests and removed unnecessary output 
##		produced during batch mode setup.
##	26-jul-93 (tyler)
##		Rephrased a message.  Added remove_temp_resources()
##		function which strips temporary resources from config.dat.
##	04-aug-93 (tyler)
##		Added pause_message().
##	20-sep-93 (tyler)
##		Improved get_timezone() prompts.
##	19-oct-93 (tyler)
##		Modified remove_temp_resources() to remove config.bck
##		when no longer needed.  Removed pause_message() since
##		ingbuild now issues a prompt when error status is
##		returned by a setup program.  Added defaults for (y/n)
##		questions.
##	22-nov-93 (tyler)
##		Improved mechanism for defaulting location values.
##		Changed "set -" to "set -$-" at daveb's suggestion.
##		Removed command which removes config.lck before
##		exits - now handled with "trap".  Force user to type
##		y or n at location confirmation prompt.  Added
##		cleanup() function to remove config.lck on success
##		exits.  Change system calls to shell procedure
##		invocations.  Added prompt() and touch_files() procedures.
##	29-nov-93 (tyler)
##		iiechonn reinstated.
##	16-dec-93 (tyler)
##		Define and use CONFIG_HOST instead of HOSTNAME
##		to compose configuration resource names.
##	31-jan-94 (tyler)
##		Remove extra "OpenINGRES" from location confirmation
##		prompt.
##      28-jan-95 (lawst01) Bug 65562
##              Function 'get_location' converts bytes needed then compares
##              result to number returned from function 'iidsfree' - this
##              function returns # of kilobytes available
##      03-apr-95 (canor01)
##              Changed 'mv' to a 'cp' followed by 'rm' to avoid messages
##              given by AIX 4.1 on permissions.
##	30-May-95 (johna)
##		Added get_num_of_processors to get the value for 
##		II_NUM_OF_PROCESSORS
##	08-Jun-95 (johna)
##		DEFAULT in previous change conflicted with the WORK locations
##		on an initial install - change the variable.
##	31-oct-95 (nick)
##		get_location() was using $LOCATION to prompt for another value
##		when it should be using $LOCATION_SYMBOL. #72067
##  03-Jul-97 (merja01)
##      Added check for multiple processors under axp_osf in
##      get_num_processors
##  05-Sep-97 (merja01)
##      Modified previous change to break check for axp_osf and
##      uname to get release into seperate if stmts.  The uname
##      command was causing problems on hp.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	05-jun-1998 (hanch04)
##		Changed NEED to NEEDKB
##	06-may-98 (popri01)
##		Use psrinfo for Unixware in number of processors logic and
##		transform default processors check to a case statement
##	24-aug-1998 (hanch04)
##		Re-wrote for multi log file project.
##	10-may-1999 (walro03)
##		Remove obsolete version string pyr_u42, sqs_us5.
##	29-oct-1999 (hanch04)
##		Use a case statement for primary and dual log
##	26-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##      18-sep-2001 (gupsh01)
##		Added functions check_response_file, check_response_write, 
##		mkresponse_msg to facilitate writing to the response file. 
##	25-sep-2001 (gupsh01)
##		Fixed the config_location function for -mkresponse option 
##		of iisudbms, to handle II_DATABASE, II_CHECKPOINT etc, writing
##		to response file.
##	29-jan-2002 (devjo01)
##		Add exit status to get_location && config_location to
##		allow user to "give up" if he cannot provide a file
##		spec on a device with sufficient storage.
##	14-dec-2004 (hayke02)
##		chmod the "default" directory to 700 rather than 777. This
##		change fixes problem INGINS 282, bug 113633.
##	27-mar-2002 (devjo01)
##		Add utility "box" & "centering" routines.
##	22-sep-2002 (devjo01)
##		Add NUMA enhancements, plus do_it()
##	6-Jan-2004 (schka24)
##		Fancy box stuff needs nawk on Solaris, get from iisysdep.
##	08-jan-2004 (somsa01)
##		We do not need to create an empty symbol.tbl file anymore.
##	03-feb-2004 (somsa01)
##		Backed out prior change for now.
##	11-Feb-2004 (hanje04)
##	 	BUG 111773
##		Make sure environment variables defining II_DATATBASE etc.
##		are picked up and used when set.
##	24-Mar-2004 (hanje04)
##		Unsure DEFAULT is honored if defined. Don't use VALUE from 
##		config.dat if it is.
##	12-apr-2004 (devjo01)
##		Move get_nickname, get_node_number. and create_cluster_dirs
##		here from iisudbms.sh so they can be used by iisunode.sh
##		Create gen_nodes_lists from code taken from iisudbms.sh
##		for similar reasons.
##	27-Sep-2004 (hanje04)
##		Add check_diskspace(). 
##	04-Nov-2004 (bonro01)
##		Add function to set the SETUID bit for the required
##		Ingres executables during install.
##	09-Nov-2004 (bonro01)
##		Removed $- from set because it fails on Solaris 9 and
##		it is not clear what benefit it ever provided.
##		The options in $- appear to be shell options and not
##		set command options so the set command will complain about
##		invalid options.
##	15-Nov-2004 (bonro01)
##		Allow ingbuild to install Ingres using any userid.
##	29-Nov-2004 (bonro01)
##		Change the # of CPU default from 1 to 2 for most platforms.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	21-Jan-2005 (hanje04)
##	 	Remove upgradedb from list of SETUID programs. It longer
##		needs setuid.
##	27-jan-2005 (devjo01)
##		- Add log_config_change_banner, lock_config_dat &
##		  lock_config_dat for use by cluster configuration scripts.
##		- Move grep_search_pattern here from iisudbms.
##	11-Feb-2005 (bonro01)
##		Move quiet_comsvrs() and restore_comsvrs() from iisudbms
##    10-Mar-2005 (hweho01)
##              Move COMMAND2 setup to the upgrade sections of iisudbms 
##              (also iisudbms64 ).
##	23-Mar-2005 (bonro01)
##		Prevent install from prompting the user for valid
##		database directories if running in batch mode.
##	08-Apr-2005 (bonro01)
##		Correctly write <default> to the response file only
##		when the value being set is null.
##	18-Sep-2006 (gupsh01)
##		Added get_date_type_alias() to configure whether the alias 
##		datatype date refers to ingresdate or ansidate.
##	01-Nov-2006 (gupsh01)
##		Correctly set the default date type.
##	12-Dec-2006 (gupsh01)
##		Modify the default for II_DATE_TYPE_ALIAS to ingresdate.
##       4-Jan-2007 (hanal04) Bug 117432
##              Default II_NUM_OF_PROCESSORS of 2 causes massive performance
##              degradation on single CPU machines if the default is
##              mistakenly selected or silently applied during rpm install.
##              Default is also applied during IceBreaker install. Revert
##              back to default of 1 which has a minor impact if incorrect.
##	12-Feb-2007 (bonro01)
##		Remove JDBC package.
##	24-Oct-2007 (hanje04)
##		BUG 114907
##		Use id -gn/-un on Mac for obtaining default user and group ids
##		as the output format from id is different and the 'sedding'
##		doesn't work.
##	07-jan-2008 (joea)
##		Discontinue use of cluster nicknames.
##	25-Jan-2008 (smeke01) b118877
##		Removed verifydb from list of executables that get the 
##		setuid bit set.		
##	12-Feb-2008 (hanje04)
##		SIR S119978
##		Add support for Intel OSX
##	02-Oct-2008 (hanje04)
##		Bug 120995
##		SD 131181
##		Try to use a unique filename in remove_temp_resources() so we 
##		don't clobber or try to use an existing file. Also improve error
##		handling so that if we do find an existing file we can't remove
##		we don't go ahead and use it.
##	05-Oct-2009 (hweho01) Bug 122688
##	        Enable check_directory routine to handle the permission   
##	        string which contains '.', it occurs on Fedora 11.   
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	10-Mar-2010 (bonro01) SIR 123296
##	    Fix Solaris problem and typo from previous change.
##	    Solaris does not support $(execute program) syntax
##
#  PROGRAM = (PROG1PRFX)shlib
#
#  DEST = utility
#----------------------------------------------------------------------------

(LSBENV)
(PROG5PRFX)_USER=$WHOAMI; export (PROG5PRFX)_USER

. (PROG1PRFX)sysdep
CONFIG_HOST=`(PROG1PRFX)pmhost`

# override existing value of II_CONFIG for non LSB environments
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    II_CONFIG=`ingprenv II_CONFIG` ; export II_CONFIG
else
    (PROG3PRFX)_CONFIG=$(PRODLOC)/(PROD2NAME)/files; export (PROG3PRFX)_CONFIG
    (PROG3PRFX)_LOG=$(PRODLOC)/(PROD2NAME)/files; export (PROG3PRFX)_LOG
fi

 
#
# skip_lines() - Skip one, or a specified number of lines.
#
#	$1 = lines to skip (default = 1)
#
skip_lines()
{
    numlines=${1:-1}
    while [ $numlines -gt 0 ]
    do
	echo ""
	numlines=`expr $numlines - 1`
    done
}

#
# wrap_lines() - consume input and echo it, wrapping lines to a specific width.
#
#	$1 = max width of a line in chars. (default = 72)
#
wrap_lines()
{
    width=${1:-72}
    fold -s -w $width
}

#
# box() - Consume stdin & convert it into boxed text sent to stdout.
#
#	$1 = box width (default = 72)
#	$2 = # blank lines above box (default = 1), if negative box top open
#	$3 = # blank lines below box (default = 1), if negative box bottom open
#
# 	1st Pipe input though 'expand' if stdin stream has 'tabs'
#
box()
{
    width=${1:-72}
    twidth=`expr $width - 5`
    labove=${2:-1}
    lbelow=${3:-1}
    skip_lines $labove
    fold -s -w $twidth | \
    $AWK -v width=$width -v tflag=$labove -v bflag=$lbelow \
	'BEGIN {
	    horz=substr(" ------------------------------------------------------------------------------", 1, width - 1 );
	    fmt="|  %-" width-5 "s |\n";
	    if (tflag >= 0) print horz }
	 { printf fmt, $0 }
	 END { if (bflag >= 0) print horz }'
    skip_lines $lbelow
}

#
# center_text() - transform stdin to centered text sent to stdout.
#
#	$1 = width of centering region (default=72)
#
center_text()
{
    width=${1:-72}
    sed 's/^[ ]*\(.*\)[ ]*$/\1/' | $AWK -v width=$width '{
	if ( length < width )
	{
	    w = ( width - length ) / 2;
	    while ( w > 0 )
	    {
		w = w - 1;
		printf " ";
	    }
	}
	print 
    }'
}
  

#
# titled_box() - Consume stdin & convert it into boxed text sent to stdout,
#		 with the first argument centered as a title on the 1st
#		 box line.
#
#	$1 = Text string (mandatory)
#	$2 = box width (default = 72)
#	$3 = # blank lines above box (default = 1), if negative, box top open
#	$4 = # blank lines below box (default = 1), if negative, box bottom open
#
titled_box()
{
    width=${2:-72}
    cwidth=`expr $width - 4`
    cat > /tmp/box.$$
    echo "$1" | center_text $cwidth | cat - /tmp/box.$$ | box $width $3 $4
    rm -f /tmp/box.$$
}

#
# error_box() - Consume stdin & convert it into boxed text sent to stdout,
#		with the centered text "*** ERROR ***" as the first line.
#
#	$1 = box width (default = 72)
#	$2 = # blank lines above box (default = 1), if negative box top open
#	$3 = # blank lines below box (default = 1), if negative box bottom open
#
# 	Pipe input though 'expand' if stdin stream has 'tabs'
#
error_box()
{
    titled_box '*** ERROR ***' $1 $2 $3
}

#
# fatal_box() - Consume stdin & convert it into boxed text sent to stdout,
#		with the centered text "*** FATAL ERROR ***" as the first line.
#
#	$1 = box width (default = 72)
#	$2 = # blank lines above box (default = 1), if negative box top open
#	$3 = # blank lines below box (default = 1), if negative box bottom open
#
# 	Pipe input though 'expand' if stdin stream has 'tabs'
#
fatal_box()
{
    titled_box '*** FATAL ERROR ***' $1 $2 $3
}

#
# warning_box() - Consume stdin & convert it into boxed text sent to stdout,
#		with the centered text "*** WARNING ***" as the first line.
#
#	$1 = box width (default = 72)
#	$2 = # blank lines above box (default = 1), if negative box top open
#	$3 = # blank lines below box (default = 1), if negative box bottom open
#
# 	Pipe input though 'expand' if stdin stream has 'tabs'
#
warning_box()
{
    titled_box '*** WARNING ***' $1 $2 $3
}


#
# in_set() - Checking if an element is already in a space delimited set.
#
#	Params:	$1 = set (double quote it!)
#		$2 = element to test.
#
in_set()
{
    for in_set_i in $1
    do
        [ "$in_set_i" = "$2" ] && return 0
    done
    return 1
}

#
# element_index() - Echo the index of an element of a space delimited
#		    set or "0".
#
#	Params:	$1 = set (double quote it!)
#		$2 = element to search for.
#
element_index()
{
    ei_i=0
    for ei_n in $1
    do
	ei_i=`expr $ei_i + 1`
	[ "$ei_n" = "$2" ] && 
	{
	    echo $ei_i
	    return 0
	}
    done
    echo "0"
    return 1
}

#
# nth_element() - Echo n'th element of a space delimited set.
#
#	Params:	$1 = set (double quote it!)
#		$2 = element to echo { n = 1 .. <n> }.
#
nth_element()
{
    ne_i=0
    for ne_n in $1
    do
	ne_i=`expr $ne_i + 1`
	[ $ne_i -eq $2 ] && 
	{
	    echo "$ne_n"
	    return
	}
    done
    echo ""
}

#
# gen_ok_set() - Create a numeric range string
#
#	Generate a string containing a valid set of integers
#	between 1 and a max value, excluding those numbers
#	which appear in an exclusion set.  String is formatted
#	as a comma separated list, with consecutive integers
#	replaced with the low and high numbers in the sequence
#	separated by a hyphen.
#
#	In passing, a default input value of the the lowest
#	available number is calculated.
#
#	Params:	$1 - exclusion set (double quote this)
#		$2 - max legitimate value.
#
#	Outputs: okset	- string containing valid values.
#		okdefault 	- default value or "" is no
#				  numbers are available to select.
#
gen_ok_set()
{
    # Set default as lowest unused #.
    okdefault=""
    okset=""
    i=0
    j=1
    k=0
    while [ $i -lt $2 ]
    do
	i=`expr $i + 1`
	in_set "$1" $i && continue
	[ "$okdefault" ] || okdefault="$i"
	[ $i -eq `expr $j + $k` ] &&
	{
	    k=`expr $k + 1`
	    continue
	}
	[ "$okset" ] && okset="${okset},"
	if [ $k -gt 1 ]
	then
	    okset="${okset}${j}-`expr $j + $k - 1`"
	elif [ $k -eq 1 ]
	then
	    okset="${okset}${j}"
	fi
	j=$i
	k=1
    done

    # Finish off last subset of "okset".
    [ "$okset" ] && okset="${okset},"
    if [ $k -gt 1 ]
    then
	okset="${okset}${j}-`expr $j + $k - 1`"
    else
	okset="${okset}${j}"
    fi
}


# grep_search_pattern()
#
#	Echo argument as a "search mask" for use with grep by
#	escaping all significant characters used by grep.
#
#	 $1	- string to convert into a grep search pattern
#
grep_search_pattern()
{
    echo "$1" | sed 's/[^ _A-Za-z0-9()]/\\&/g'
}


#
# touch_files() - create config.dat and symbol.tbl if non-existent
#
touch_files()
{
   # Locations are different for LSB setup
   if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      symbtbl=${II_ADMIN}/symbol.tbl
      cfgdat=${II_ADMIN}/config.dat
   else
      symbtbl=$(PRODLOC)/(PROD2NAME)/files/symbol.tbl
      cfgdat=$(PROG3PRFX)_CONFIG/config.dat
   fi
   # create symbol.tbl if it doesn't exist 
   [ -f ${symbtbl} ] ||
   {
      touch ${symbtbl} ||
      {
         cat << !

Unable to create ${symbtbl}.  Exiting... 

!
         exit 1
      }
   }

   # create config.dat if it doesn't exist
   [ -f ${cfgdat} ] ||
   {
      touch ${cfgdat} 2>/dev/null ||
      {
         cat << !

Unable to create ${cfgdat}.  Exiting...

!
         exit 1
      }
      chmod 644 ${cfgdat}
   }

   II_USERID=`iigetres ii.${CONFIG_HOST}.setup.owner.user`
   II_GROUPID=`iigetres ii.${CONFIG_HOST}.setup.owner.group`
   if [ -z "$II_USERID" -o -z "$II_GROUPID" ]; then
	if [ $VERS = "mg5_osx" ] || [ $VERS = "int_osx" ] ; then
	  $DOIT iisetres "ii.${CONFIG_HOST}.setup.owner.user" `id -un`
	  $DOIT iisetres "ii.${CONFIG_HOST}.setup.owner.group" `id -gn`
	else
	  eval `id | sed -e "s/ /|/g" -e "s/=/ /g" -e "s/(/ /g" -e "s/)/ /g" | awk 'BEGIN {RS = "|"} {print $1 "=" $3}'`
	  $DOIT iisetres "ii.${CONFIG_HOST}.setup.owner.user" $uid
	  $DOIT iisetres "ii.${CONFIG_HOST}.setup.owner.group" $gid
	fi
   fi
}

#=[ gen_nodes_lists ]============================================
#
#	Utility routine to generate list of existing cluster
#	nodes (NODESLST), and a list (same order) of node
#	number assignments (IDSLST).
#
gen_nodes_lists()
{
    # Get list of existing nodes and node numbers.
    NODESLST=`grep config.cluster.id $(PROG3PRFX)_CONFIG/config.dat | \
     cut -d'.' -f2`
    NODESLST=`echo $NODESLST | tr '[a-z]' '[A-Z]'`
    IDSLST=`grep config.cluster.id $(PROG3PRFX)_CONFIG/config.dat | \
     cut -d':' -f2`
    IDSLST=`echo $IDSLST`
}

#=[ get_node_number ]=============================================
#
#	Utility routine to prompt for a node number
#
#	Params:	$1 = RAD number or blank if regular cluster.
#
#	Data:	IDSLST - space delimited list of taken node id's.
#		NODESLST - space delimited list of nodes for
#			   each ID in IDSLST, in same order.
#		CLUSTER_MAX_NODE - Max valid Node ID
#
#	Outputs: II_CLUSTER_ID set to valid node ID, or cleared if no input.
#
get_node_number()
{
    II_CLUSTER_ID=""

    gen_ok_set "$IDSLST" $CLUSTER_MAX_NODE

    if [ -z "$okdefault" ]
    then
	cat << !

ERROR: Maximum number of nodes ($CLUSTER_MAX_NODE) for an Ingres cluster 
have already been configured.

Aborting setup.
!
	exit 1
    fi

    while [ -z "$II_CLUSTER_ID" ]
    do
	if $READ_RESPONSE
	then	# Get CLUSTER ID from response file.
	    GETRES="`iiread_response \"CLUSTER_NODE_ID${1}\" $RESINFILE`"
	elif $BATCH
	then	# Regular batch must use default
	    II_CLUSTER_ID="$okdefault"
	else	# Get CLUSTER ID interactively
	    while :
	    do
		iiechonn "Please enter an available node number from the set ($okset). [${okdefault}] "
		read GETRES junk
		[ "$GETRES" ] || break
		[ -n "`echo $GETRES | sed 's/[0-9]//g'`" ] && continue
		[ $GETRES -gt 0 -a $GETRES -le $CLUSTER_MAX_NODE ] && \
		 break
	    done
	fi

	II_CLUSTER_ID="$GETRES"
	[ "$II_CLUSTER_ID" ] || II_CLUSTER_ID="$okdefault"

	GETRES=`element_index "$IDSLST" "$II_CLUSTER_ID"`
	if [ $GETRES -gt 0 ]
	then
	    cat << !

Node number "${II_CLUSTER_ID}" is already in use by node `nth_element "$NODESLST" $GETRES`.

!
	    II_CLUSTER_ID=""
	fi

	$READ_RESPONSE && [ -z "$II_CLUSTER_ID" ] && 
	{
	    cat << !

ERROR: response file "${RESINFILE}" specified a CLUSTER_NODE_ID which
conflicted with an existing node.

!
	    exit 1
	}

    done

    # Have valid II_CLUSTER_ID now
    if $WRITE_RESPONSE
    then
	# configured than Ingres instance used to generate
	# the response file.
	check_response_write "CLUSTER_NODE_ID${1}" "$GETRES" \
	 replace allowdefault
    else
	$DOIT iisetres ii.`node_name $1`.config.cluster_mode "ON"
	$DOIT iisetres ii.`node_name $1`.config.cluster.id $II_CLUSTER_ID
    fi
    NODESLST="${NODESLST} `node_name $1 | tr '[a-z]' '[A-Z]'`"
    IDSLST="${IDSLST} ${II_CLUSTER_ID}"
}

#=[ create_node_dirs ]=========================================
#
#	Create various directories needed for clustering 
#
#	Params:	$1 = node name.
#
create_node_dirs()
{
    if $WRITE_RESPONSE
    then
	# Do nothing, we're just generating a response file
	:
    else
	#
	# Set up some cluster specific directories
	#
	if [ ! -d $(PROG3PRFX)_CONFIG ]; then
	    $DOIT mkdir $(PROG3PRFX)_CONFIG
	    $DOIT chmod 755 $(PROG3PRFX)_CONFIG
	fi
	if [ ! -d $(PROG3PRFX)_CONFIG/memory ]; then
	    $DOIT mkdir $(PROG3PRFX)_CONFIG/memory
	    $DOIT chmod 755 $(PROG3PRFX)_CONFIG/memory
	fi
	if [ ! -d $(PROG3PRFX)_CONFIG/memory/$1 ]; then
	    $DOIT mkdir $(PROG3PRFX)_CONFIG/memory/$1
	    $DOIT chmod 755 $(PROG3PRFX)_CONFIG/memory/$1
	fi
	if [ ! -d $(PROG3PRFX)_SYSTEM/ingres/admin ]; then
	    $DOIT mkdir $(PROG3PRFX)_SYSTEM/ingres/admin
	    $DOIT chmod 755 $(PROG3PRFX)_SYSTEM/ingres/admin
	fi
	if [ ! -d $(PROG3PRFX)_SYSTEM/ingres/admin/$1 ]; then
	    $DOIT mkdir $(PROG3PRFX)_SYSTEM/ingres/admin/$1
	    $DOIT chmod 755 $(PROG3PRFX)_SYSTEM/ingres/admin/$1
	fi
    fi
}

#
#	Utility debug routine
#
#	This routine normally just takes all the passed arguments,
#	and executes them, returning the final status code.
#	In other words, it should do absolutely nothing!  If
#	however, DEBUG_DRY_RUN is set, then no action is
#	actually taken, and the commands that would have been
#	executed are logged to DEBUG_DRY_RUN_LOG_FILE.
#
#	Certain commands that update config.dat are executed
#	regardless, since the setup programs need to read and
#	write values to operate, and we backup and restore the
#	config.dat to negate these changes anyway.
#
#	Care needs to be taken however, in that this does introduce
#	another reparsing pass, and characters that were escaped
#	when evaluated as parameters to do_it, will have lost the
#	escape character when reparsed here.
#
#	E.g.  "do_it createdb imadb -u\$ingres" would be reparsed as
#	 "createdb imadb -u$ingres", which would be executed as
#	 "createdb imadb -u", which would fail.  Solution is to
#	 double escape chars (e.g. "-u\\\$ingres").
#
#	When using do_it in a shell script, it is good practice to
#	have the call instantiated only when needed.  This is because,
#	outside a development environment, we may encounter user
#	provided data, which when re-parsed will be evaluated in-
#	correctly.
#
#	To do this, use a variable "DOIT" in place of do_it, which
#	is either undefined or empty, or equals "do_it".
#
#	When using this technique, any extra escapes needed by do_it,
#	need to be conditional themselves.
#
#	E.g. "$DOIT createdb imadb -u${DIX}\$ingres", where
#	DIX is "\\" if DOIT is do_it, and NULL or empty otherwise.
#
do_it()
{
    [ $DEBUG_LOG_FILE ] && echo "$1 $2 $3 $4 $5 $6 $7 $8 $9" >> $DEBUG_LOG_FILE

    if $DEBUG_DRY_RUN
    then
	case "$1" in
	(PROG1PRFX)genres|(PROG1PRFX)setres|(PROG1PRFX)rmres|(PROG1PRFX)getres|(PROG2PRFX)setenv|(PROG2PRFX)prenv)
	    # Actually execute these commands
	    ;;
	*)  # All other commands just skip.
	    return 0
	    ;;
	esac
    fi
    doit_params=$#
    doit_buf="$1"
    doit_num=1
    while [ $doit_num -lt $doit_params ]
    do
	doit_num=`expr $doit_num + 1`
	shift
	doit_buf="$doit_buf \"${1}\""
    done
    eval $doit_buf
    return $?
}

#
# prompt() - display a question and get a y/n response
#
prompt()
{
   while :
   do
      if [ "$2" ] ; then
         (PROG1PRFX)echonn $1 "(y/n) [$2] "
         read answer junk
         [ -z "$answer" ] && answer=$2 
      else
         (PROG1PRFX)echonn $1 "(y/n) "
         read answer junk
      fi
   
      case $answer in
         y|Y)
            return 0;;
         n|N)
            return 1;;
         *)
            echo ""
            echo "Please enter 'y' or 'n'."
            echo "";;
      esac
   done
}

#
# log_config_change_banner()
#
# $1 = Banner Text
# $2 = Box width
#
log_config_change_banner()
{
   # Locations are different for LSB setup
   if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      cfglog=${II_LOG}/config.log
   else
      cfglog=$(PRODLOC)/(PROD2NAME)/files/config.log
   fi
    echo "$1" | box ${2} >> ${cfglog}
    cat << ! >> ${cfglog}
host:		`uname -n`
user:		${WHOAMI}
terminal:	`tty | sed 's/^.*\///'`
date:		`date`

!
}

#
# lock_config_dat() - Generate a configuration lock file a'la CBF.
#
#	$1 = application locking
#
lock_config_dat()
{
   # Locations are different for LSB setup
   if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      cfglck=${II_ADMIN}/config.lck
   else
      cfglck=$(PRODLOC)/(PROD2NAME)/files/config.lck
   fi
    if [ -f ${cfglck} ]
    then
	error_box << !
${cfglck} exists.  Other users may be configuring this Ingres instance.

`cat ${cfglck} | expand`
!
	return 1
    else
	cat << ! >  ${cfglck}
application:			${1}
host:				`uname -n`
pid:				$$
terminal			`tty | sed 's/^.*\///'`
timestamp:			'`date`'
user:				$WHOAMI
!
	return 0
    fi
}

#
# unlock_config_dat()
#
unlock_config_dat()
{
   # Locations are different for LSB setup
   if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      cfglck=${II_ADMIN}/config.lck
   else
      cfglck=$(PRODLOC)/(PROD2NAME)/files/config.lck
   fi
    [ -f ${cfglck} ] && \
     rm -f ${cfglck}
}

#
# cleanup() - remove config.lck before exiting successfully
#
# in WRITE_RESPONSE mode do nothing unless we are told to ignore the response flag 
#
clean_exit()
{
if [ "$WRITE_RESPONSE" = "false" ] || [ "$IGNORE_RESPONSE" = "true" ] ; then
   unlock_config_dat > /dev/null 2>&1
   rm -f /tmp/*.$$ 1>/dev/null 2>/dev/null
   trap : 0
fi
   exit 0
}

#
# remove_temp_resources() - remove temporary setup resources from config.dat
#
remove_temp_resources()
{
   # Locations are different for LSB setup
   if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      cfgdat=${II_ADMIN}/config.dat
      cfgbck=${II_ADMIN}/config.bck
   else
      cfgdat=$(PROG3PRFX)_CONFIG/config.dat
      cfgbck=$(PROG3PRFX)_CONFIG/config.bck
   fi
   tmpconfdat=/tmp/config.$$
   # back it up error if we have a problem
   cp ${cfgdat} ${cfgbck} && \
      grep -v (PROG4PRFX).\\\*.setup ${cfgdat} >${tmpconfdat} && \
      cp ${tmpconfdat} ${cfgdat} && \
      rm -f ${cfgbck} && \
      rm -f ${tmpconfdat} && \
      chmod 644 ${cfgdat} || \
      cat << !

Unable to back up:

	${cfgdat}

in order to remove temporary resources.  All lines in this file which
begin with "(PROG4PRFX).*.setup." can be removed.

!
}

#
# validate_resource() - inverts return value of iivalres
#
validate_resource()
{
   if (PROG1PRFX)valres -v $1 $2 $3
   then
      return 1
   else
      return 0
   fi
}

#
# get_location() - prompts for a location with adequate free space
#
#	returns 0 if free space adequate
#	returns 1 if insufficient space, and no new path entered.
#
get_location()
{
   # prompt until location with adequate space is supplied
   while [ -d $BASE ] && [ "$NEEDKB" ] && \
       FREE=`(PROG1PRFX)dsfree $BASE ` && \
      [ $FREE -lt $NEEDKB  ]
   do
      cat << !
 
A minimum of $NEEDKB kilobytes is required by the $LOCATION_SYMBOL location.
The location you have specified only has $FREE kilobytes free.
 
!
      if $BATCH
      then
         return 1
      else
         (PROG1PRFX)echonn "Enter another path for $LOCATION_SYMBOL: "
         read BASE junk
         [ -z "$BASE" ] && return 1
      fi
   done
   return 0
}

#
# check_directory() -
#
check_directory()
{
   DIRECTORY_PATH=$1

   PERMISSION=$2

   [ $CREATE ] || CREATE=false

   umode=`expr $PERMISSION : '\(.\)..'`
   gmode=`expr $PERMISSION : '.\(.\).'`
   omode=`expr $PERMISSION : '..\(.\)'`
   lsmode=""

   for m in $umode $gmode $omode
   do
      case $m in

         0)
            p="---";;
         1)
            p="--x";;
         2)
            p="-w-";;
         3)
            p="-wx";;
         4)
            p="r--";;
         5)
            p="r-x";;
         6)
            p="rw-";;
         7)
            p="rwx";;
         *)
            echo "check_directory: bad permission string argument"
            return 1 
      esac
      lsmode="$lsmode$p"
   done

   if [ ! -d $DIRECTORY_PATH ]
   then
      if $CREATE
      then
         echo ""
         echo "Creating $DIRECTORY_PATH..."
         if $DOIT mkdir $DIRECTORY_PATH >/dev/null 2>&1
         then
            if $DOIT chmod $PERMISSION $DIRECTORY_PATH
            then
               return 0 
            else
               error_box << !

Cannot change mode of directory:

    $DIRECTORY_PATH

to $PERMISSION.

!
               return 1 
            fi
         else
            error_box << !

Cannot create directory:
       
    $DIRECTORY_PATH

!
            return 1 
         fi
      else
         error_box << !

The directory:

    $DIRECTORY_PATH

does not exist.

!
         return 1 
      fi
   fi

   if ls -Ld >/dev/null 2>&1 
   then
      Lflag=L
   else
      Lflag=""
   fi
   set - `ls -ld${Lflag} $DIRECTORY_PATH`

   # ignore first character which is the group setuid.
   MODES=`echo $1 | sed -n -e 's/\.//' -e 's/^.//p'`

   OWNER=$3

   if [ "$OWNER" != $(PROG5PRFX)_USER ]
   then
      error_box << !

The directory:

    $DIRECTORY_PATH

is not owned by '$(PROG5PRFX)_USER'.

!
      return 1 
   fi

   if [ "$MODES" != $lsmode ]
   then
      error_box << !

The directory:

    $DIRECTORY_PATH

has incorrect permissions ($MODES).

The correct permissions are $lsmode.

!
      echo "Trying to set the correct permissions..."
      if $DOIT chmod $PERMISSION $DIRECTORY_PATH
      then
         echo ""
         echo "Directory $DIRECTORY_PATH has been changed to $PERMISSION"
      else
         echo ""
         echo "Cannot change mode on directory $DIRECTORY_PATH"
         return 1 
      fi
   fi

   return 0
}

#
# bad_directory() - inverts return status of check_directory() 
#
bad_directory()
{
   BASE=$1

   DIRECTORY=$2

   PERMISSION=$3

   # make sure base directory exists
   if [ -d $BASE ]
   then :
   else
      cat << !

$BASE not found.

!
      return 0
   fi

   if check_directory $BASE/$DIRECTORY $PERMISSION
   then
	   return 1
   else
	   return 0
   fi
}

#
# node_name() - Modify CONFIG_HOST value if using NUMA clusters
#
#	Utility routine which echoes the "NUMA" host name.  If $1 empty
#	$CONFIG_HOST is returned, otherwise $CONFIG_HOST is prefixed
#	with 'r${1}_'.   This "decorated" CONFIG_HOST value is used
#	where individual virtual NUMA nodes must save parameters
#	specific to their RAD.
#
#	Params: $1 = RAD number or blank if not NUMA.
#
node_name()
{
    if [ -n "$1" ]
    then 
	echo "r${1}_${CONFIG_HOST}"
    else
	echo "${CONFIG_HOST}"
    fi
}

#
# dump_configured_logs() - display already configured values if any.
#
#	Params:	$1 - LOCATION_SYMBOL
#		$2 - HOST
#		$3 - Attribute Prefix (log_file or dual_log)
#
dump_configured_logs()
{
    VALUE=`(PROG1PRFX)getres (PROG4PRFX).${2}.rcp.log.${3}_1`
    [ "$VALUE" ] &&
    {
	echo
	LOG_FILE_NUM=1
	while [ "$VALUE" ] ; do
	    cat << !
$LOCATION_SYMBOL configured as $VALUE
!
	    LOG_FILE_NUM=`expr $LOG_FILE_NUM + 1`
	    VALUE=`(PROG1PRFX)getres (PROG4PRFX).${2}.rcp.log.${3}_$LOG_FILE_NUM`
	done
	return 0
    }
    return 1
}

#
# config_location() - configure an Ingres location variable
#
#	returns 0 if all is well
#	returns 1 if directory with correct permissions and sufficient
#		  space was not specified.
config_location()
{
   LOCATION_SYMBOL=$1

   LOCATION_TEXT=$2

   LOCATION_NUMA_HOST=`node_name $3`

   DESCRIPTION="the default location"
   CHECKPOINT=false
   FULLPATH=false
   DEFAULT=""

   case "$LOCATION_SYMBOL" in

      -*)
         echo "config_location: illegal option: $LOCATION_SYMBOL" 
         ;;

      II_DATABASE)
         MODE=700
         DATALOC=true
         BACKUPLOC=false
         NEEDKB=6144 # required to create iidbdb 
         DIR=data
	 [ "$II_DATABASE" ] && DEFAULT=$II_DATABASE
         ;;

      II_CHECKPOINT)
         MODE=700
         DATALOC=true
         BACKUPLOC=true
         CHECKPOINT=true
         NEEDKB=2
         DIR=ckp
	 [ "$II_CHECKPOINT" ] && DEFAULT=$II_CHECKPOINT
         ;;

      II_JOURNAL)
         MODE=700
         DATALOC=true
         BACKUPLOC=true
         NEEDKB=2
         DIR=jnl
	 [ "$II_JOURNAL" ] && DEFAULT=$II_JOURNAL
         ;;

      II_DUMP)
         MODE=700
         DATALOC=true
         BACKUPLOC=true
         NEEDKB=2
         DIR=dmp
	 [ "$II_DUMP" ] && DEFAULT=$II_DUMP
         ;;

      II_WORK)
         MODE=700
         DATALOC=true
         BACKUPLOC=false
         NEEDKB=3
         DIR=work
	 [ "$II_WORK" ] && DEFAULT=$II_WORK
         ;;

      II_LOG_FILE)
         MODE=700
         DATALOC=false
         BACKUPLOC=false
         NEEDKB=`(PROG1PRFX)getres (PROG4PRFX).${LOCATION_NUMA_HOST}.rcp.file.kbytes`
         DIR=log
         DESCRIPTION="a location"
	 [ "$II_LOG_FILE" ] && DEFAULT=$II_LOG_FILE
         ;;

      II_DUAL_LOG)
         MODE=700
         DATALOC=false
         BACKUPLOC=false
         NEEDKB=`(PROG1PRFX)getres (PROG4PRFX).${LOCATION_NUMA_HOST}.rcp.file.kbytes`
         DIR=log
         DESCRIPTION="a location"
	 [ "$II_DUAL_LOG" ] && DEFAULT=$II_DUAL_LOG
         ;;

      ING_ABFDIR)
         MODE=777
         DATALOC=false
         BACKUPLOC=false
         DIR=abf
         FULLPATH=true
         ;;

      *)
         echo "config_location(): location symbol $LOCATION_SYMBOL unknown."
         exit 1
         ;;
   esac

   VALUE=`(PROG2PRFX)prenv $LOCATION_SYMBOL`

   [ "$VALUE" ] &&
   {
      cat << !

$LOCATION_SYMBOL configured as $VALUE
!
      return 0
   }

   case "$LOCATION_SYMBOL" in
      II_LOG_FILE)
	if dump_configured_logs $LOCATION_SYMBOL $LOCATION_NUMA_HOST log_file
	then
	    return 0
	fi
	;;

      II_DUAL_LOG)
	if dump_configured_logs $LOCATION_SYMBOL $LOCATION_NUMA_HOST dual_log
	then
	    return 0
	fi
	;;
   esac

   # get default location path
   VALUE=`(PROG1PRFX)getres (PROG4PRFX).$CONFIG_HOST.setup.$LOCATION_SYMBOL`
   [ "$VALUE" ] && [ -z "$DEFAULT" ] &&
   {
      DEFAULT=`eval $VALUE`
      $CHECKPOINT &&
      {
         II_DATABASE=`(PROG2PRFX)prenv II_DATABASE`
         [ "$DEFAULT" = "$II_DATABASE" ] && DEFAULT=""
      }
   }

   if $BATCH ; then
      if [ "$DEFAULT" ] ; then
          BASE=$DEFAULT
      else
          BASE=$(PRODLOC)
      fi
      if [ "$READ_RESPONSE" = "true" ] ; then
         location_value=`iiread_response "${LOCATION_SYMBOL}${3}" $RESINFILE` 
         if [ ! -z "$location_value" ] ; then
             BASE=$location_value
         fi
      fi
      cat << !

$LOCATION_SYMBOL configured as $BASE
!
   else
      cat << !

Please enter $DESCRIPTION for the $LOCATION_TEXT:
!
      # prompt for location path
      BASE=""
      while [ -z "$BASE" ] ; do
         if [ -z "$DEFAULT" ] ; then
            (PROG1PRFX)echonn "$LOCATION_SYMBOL: "
         else
            (PROG1PRFX)echonn "[$DEFAULT] "
         fi
         read BASE junk
         [ -z "$BASE" ] && BASE=$DEFAULT
      done
   fi

   WARNING=false
   # warn the user not to use a device which may contain database files
   # for checkpoint, journal or dump files.
   if $BACKUPLOC ; then
      if $CHECKPOINT ; then
         WARNING=true	# always display warning for II_CHECKPOINT
      else
         # only display warning for II_JOURNAL and II_DUMP if different
         # from II_CHECKPOINT.
         if [ $BASE != `(PROG2PRFX)prenv II_CHECKPOINT` ] ; then
            WARNING=true
         else
            WARNING=false
         fi
      fi
   fi

   if $WARNING ; then
      II_DATABASE=`(PROG2PRFX)prenv II_DATABASE`
      $BATCH ||
      {
         warning_box << !

Do not store checkpoint, journal or dump files for a database on \
the same physical device as its data, or you will not be able to \
recover the data stored on that device if it fails.  Please verify \
that the default location you have entered for your
$LOCATION_TEXT: 

       $BASE

is on a different physical device from the default location \
you have entered for your database files:

       $II_DATABASE

!
         prompt \
            "Is the value you have entered for $LOCATION_SYMBOL correct?" ||
         {
            NEWBASE=""
            while [ -z "$NEWBASE" ] ; do
               echo ""
               (PROG1PRFX)echonn "Enter another value for $LOCATION_SYMBOL: "
               read NEWBASE junk
            done
            BASE=$NEWBASE
         }
      }
   fi


   CREATE=true
   DIR_OK=true

   if $FULLPATH ; then
      SUFFIX=/(PROD2NAME)/$DIR
      while : ; do
         expr "$BASE" : ".*$SUFFIX" > /dev/null 2>&1 || BASE=${BASE}$SUFFIX
         check_directory $BASE $MODE && break
         if $BATCH
         then
            return 1
         else
            (PROG1PRFX)echonn "Enter another value for $LOCATION_SYMBOL: "
            read BASE junk
            get_location || return 1
         fi
      done
   else
      # only check disk space if directory does not exist
      [ ! -d $BASE/(PROD2NAME)/$DIR ] && 
      {
	 get_location || return 1
      }

      # check/create the "(PROD2NAME)" sub-directory with mode 755
      while bad_directory $BASE (PROD2NAME) 755; do
         if $BATCH
         then
            return 1
         else
            BASE=""
            while [ -z "$BASE" ] ; do
               (PROG1PRFX)echonn "Enter another value for $LOCATION_SYMBOL: "
               read BASE junk
            done
            [ ! -d $BASE/(PROD2NAME)/$DIR ] && 
            {
               get_location || return 1
            }
         fi
      done

      # check/create $DIR sub-directory with the appropriate mode
      check_directory $BASE/(PROD2NAME)/$DIR $MODE || DIR_OK=false
   fi

   # create "default" directory for database, checkpoint, journal, and
   # dump locations with mode 777
   ### if $DATALOC && [ $stat = 0 ] ; then
   if $DATALOC && $DIR_OK ; then
      # we need to chmod the "default" directory to 777 because
      # the (PROD2NAME) process that writes to this directory is installed
      # with "setuid" privilege and System V uses the "real" users
      # permission.  By chmoding the parent directory to 700, all users
      # but the owner are prevented from writing to this directory.

      # chmod the "default" directory to 700 rather than 777
      check_directory $BASE/(PROD2NAME)/$DIR/default 700 || DIR_OK=false 
   fi

   # Write final messages

   if $DIR_OK ; then
   case "$LOCATION_SYMBOL" in

      II_LOG_FILE)
	if [ "$WRITE_RESPONSE" = "true" ] ; then 
	    check_response_write "II_LOG_FILE${3}" $BASE replace
	else
	    $DOIT (PROG1PRFX)setres (PROG4PRFX).$LOCATION_NUMA_HOST.rcp.log.log_file_1 $BASE
        fi
	;;

      II_DUAL_LOG)
	if [ "$WRITE_RESPONSE" = "true" ] ; then 
	    check_response_write II_DUAL_LOG${3} $BASE replace
	else
	    $DOIT (PROG1PRFX)setres (PROG4PRFX).$LOCATION_NUMA_HOST.rcp.log.dual_log_1 $BASE
	fi
	;;

      *)
	    if [ "$WRITE_RESPONSE" = "true" ] ; then 
		 check_response_write $LOCATION $BASE replace
	    fi
	    (PROG2PRFX)setenv $LOCATION_SYMBOL $BASE
	;;
   esac
   else
      error_box << !

Errors occurred while trying to create directory

      $BASE

for location $LOCATION_SYMBOL. Please correct these problems and re-run 
$CMD.

!
      exit 1 
   fi
   return 0
}

#
# get_installation_code() -- prompt for a valid installation id 
#
get_installation_code()
{

# in a write response mode the (PROG3PRFX)_INSTALLATION may already have been set 
# for this setup. In such a case prompt before continuing.
   if [ "$WRITE_RESPONSE" = "true" ] ; then
      RESVAL=`iiread_response (PROG3PRFX)_INSTALLATION $RESOUTFILE` 
      if [ ! -z "$RESVAL" ] ; then
         cat << !

(PROG3PRFX)_INSTALLATION value has already been set to $RESVAL in response file.

!
         if prompt "Do you wish to modify this value of (PROG3PRFX)_INSTALLATION?" n 
	 then
	    echo
 	 else
	    exit 1
	 fi
      fi  
   fi
   cat << !

To differentiate between multiple (PROD1NAME) installations set up to run
on the same machine, you must assign each installation a two-letter code
that is unique across the machine. The first character of the installation
code must be a letter; the second character may be a letter or a number.

The installation code you choose will be assigned to the (PROD1NAME) variable
(PROG3PRFX)_INSTALLATION. (An (PROD1NAME) variable corresponds to a UNIX environment
variable.)

Before you enter an installation code, make sure that it is not in use
by another (PROD1NAME) installation on "$HOSTNAME". 

!

   [ -z "$DEFAULT" ] && [ "$WRITE_RESPONES" = "false" ] && DEFAULT=`(PROG1PRFX)getres (PROG4PRFX)."*".setup.(PROG4PRFX)_installation`
   [ -z "$DEFAULT" ] && DEFAULT="(PROG3PRFX)"

   NOTDONE=true
   while $NOTDONE ; do 
      (PROG1PRFX)echonn "Please enter a valid installation code [$DEFAULT] "
      read (PROG3PRFX)_INSTALLATION junk
      case "$(PROG3PRFX)_INSTALLATION" in

         "")
            NOTDONE=false
            ;;

         [a-zA-Z][a-zA-Z0-9])
            NOTDONE=false
            ;;

         *)
            cat << !

You have entered an invalid installation code.  The first character of
the installation code must be a letter; the second character may be a
letter or a number.

!
            ;;
      esac
   done 

   [ -z "$(PROG3PRFX)_INSTALLATION" ] && (PROG3PRFX)_INSTALLATION=$DEFAULT
}

#
# get_timezone() -- prompt for a valid timezone
#
get_timezone()
{

   cat << !

You must now specify the time zone this installation is located in.
To specify a time zone, you must first select the region of the world
this installation is located in.  You will then be prompted with a
list of the time zones in that region.
!

   NOT_DONE=true
   while $NOT_DONE ; do
      # display valid entries
      (PROG1PRFX)valres -v (PROG4PRFX)."*".setup.region BOGUS_REGION $TZ_RULE_MAP
      (PROG1PRFX)echonn "Please enter one of the named regions: "
      read REGION junk
      [ -z "$REGION" ] && REGION=BOGUS_REGION
      while validate_resource (PROG4PRFX)."*".setup.region $REGION $TZ_RULE_MAP
      do
         (PROG1PRFX)echonn "Please enter one of the named regions: "
         read REGION junk
         [ -z "$REGION" ] && REGION=BOGUS_REGION
      done

      REGION_TEXT=`(PROG1PRFX)getres (PROG4PRFX)."*".setup.$REGION`
      cat << !

Now enter one of the following $REGION_TEXT.
!
      # display valid entries
      (PROG1PRFX)valres -v (PROG4PRFX)."*".setup.$REGION.tz BOGUS_TIMEZONE $TZ_RULE_MAP
      cat << !
If you have selected the wrong region, press RETURN. 

!
      (PROG1PRFX)echonn "Please enter a valid time zone: "
      read (PROG3PRFX)_TIMEZONE_NAME junk
      if [ "$(PROG3PRFX)_TIMEZONE_NAME" ] ; then
         while validate_resource (PROG4PRFX)."*".setup.$REGION.tz $(PROG3PRFX)_TIMEZONE_NAME \
            $TZ_RULE_MAP
         do
            (PROG1PRFX)echonn "Please enter a valid time zone: "
            read (PROG3PRFX)_TIMEZONE_NAME junk
            [ -z "$(PROG3PRFX)_TIMEZONE_NAME" ] && (PROG3PRFX)_TIMEZONE_NAME=BOGUS_TIMEZONE
         done

         TZ_TEXT=`(PROG1PRFX)getres (PROG4PRFX)."*".setup.$REGION.$(PROG3PRFX)_TIMEZONE_NAME`
         cat << !

The time zone you have selected is:

	$(PROG3PRFX)_TIMEZONE_NAME ($TZ_TEXT)

If this is not the correct time zone, you will be given the opportunity to
select another region. 

!
         if prompt "Is this time zone correct?" y
         then 
            NOT_DONE=false
         else
            cat << !

Please select another time zone region.
!
         fi 
      else
         cat << !

Please select another time zone region.
!
      fi
   done

}

#
# pause() -- display pause message and prompt once 
#
pause()
{
   CONTINUE="Press RETURN to continue:"
   (PROG1PRFX)echonn "$CONTINUE" ; read junk
}

#
# get_date_type_alias() -- Gets users choice for setting II_DATE_TYPE_ALIAS
#
get_date_type_alias()
{
	DATE_ALIAS_RESPONSE=false
	DEF_DATE_TYPE=ingresdate

	cat << !

You can configure 'date' keyword in SQL to refer to ingresdate or 
ansidate data type. 

Ingresdate is a composite data type and stores date, time, 
timestamp or interval data in the same column. 
Before the introduction of ansidate, date referred to ingresdate. 

Ansidate refers to ANSI specified Date type and can only store a 
date value, in the format YYYY-MM-DD.

The value you specify will be used to set the date_type_alias 
configuration parameter. If you choose no for associating 
Ingresdate to date it will be set to ansidate.

!
	if prompt "Do you wish to associate date data type to refer to ingresdate ?" y 
	then 
		        (PROG3PRFX)_DATE_TYPE_ALIAS=$DEF_DATE_TYPE
			cat << !

Date now refers to ingresdate data type

!
		else
		        (PROG3PRFX)_DATE_TYPE_ALIAS=ansidate
			cat << !

Date refers to ansi date data type

!
		fi
}
#
# get_num_processors() -- prompt for a number of CPUs
#
get_num_processors()
{
	cat << !

You must now specify the number of CPUs (processors) in this machine
so that (PROD1NAME) may be set up for this server. If you do not know
the exact number of CPUs, but know that this is a multi-cpu machine,
enter a value of 2.

The value you specify will be used to set the (PROD1NAME) variable
(PROG3PRFX)_NUM_OF_PROCESSORS and possibly other configuration variables.

!
	case $VERS in
		pyr_us5 | pym_us5 | sqs_ptx)
			DEF_PROCS=2
                	;;
## axp_osf and usl_us5 check for multiple processors on machine ##
		usl_us5)
			DEF_PROCS=`/usr/sbin/psrinfo -n`
                	;;
		axp_osf)
			if [ `uname -r | tr -d V` -ge 4.0 ]
			then
				DEF_PROCS=`/usr/sbin/psrinfo -n | tr -d 'a-z A-Z = .'`
			else
				DEF_PROCS=1
			fi
                	;;
		*)
			DEF_PROCS=1
                	;;
	esac

	[ -z "$DEF_CPUS" ] && DEF_CPUS=`(PROG2PRFX)prenv (PROG3PRFX)_NUM_OF_PROCESSORS`
	[ -z "$DEF_CPUS" ] && DEF_CPUS=$DEF_PROCS

	NOTDONE=true
	while $NOTDONE ; do
		(PROG1PRFX)echonn "Please enter the number of CPUs in this machine [$DEF_CPUS] "
		read (PROG3PRFX)_NUM_OF_PROCESSORS junk

		[ -z "$(PROG3PRFX)_NUM_OF_PROCESSORS" ] && (PROG3PRFX)_NUM_OF_PROCESSORS=$DEF_CPUS
		PROCNUM=`expr "$(PROG3PRFX)_NUM_OF_PROCESSORS" + 0 2> /dev/null`
		if [ "$PROCNUM" = "$(PROG3PRFX)_NUM_OF_PROCESSORS" ] && [ $PROCNUM -ge 1 ]
		then
			NOTDONE=false;
		else
			cat << !

You have entered an invalid number of CPUs. Please enter a numeric
value, or hit RETURN for the default value.

!
		fi
	done

	echo " "
	echo (PROG3PRFX)_NUM_OF_PROCESSORS is $(PROG3PRFX)_NUM_OF_PROCESSORS
	echo " "
}

#
# check_response_file() -- creates the response file if it does not exits. 
#
check_response_file()
{
if [ "$WRITE_RESPONSE" = "true" ] ; then
[ -f $RESOUTFILE ] ||
   {
      touch $RESOUTFILE ||
      {
         cat << !

Unable to create response file $RESOUTFILE.  Exiting...

!
          exit 1
      }
      eval echo "! Ingres response file, created by utility ingbuild. " > $RESOUTFILE
   }
fi

if [ "$READ_RESPONSE" = "true" ] ; then
   [ -f $RESINFILE ] ||
   {
         cat << !

Response file $RESINFILE not found, check input file name.  Exiting...

!
         exit 1
   }
fi
}

#
# check_response_write() -- writes to the response file 
#
# This routines checks if the file exists and if a value is already set
# for this variable. If the value is already set then do not replace.
# if the input parameter includes the flag replace then the value of the
# variable in the response file.
#
#	check_response_write Variable Value [replace]
#
check_response_write()
{
   if [ -n "$1" -a \( -n "$2" -o \
	\( "$3" = "allowdefaults" -o "$4" = "allowdefaults" \) \) ] ; then

	check_response_file

	PRESENT=`eval grep $1 $RESOUTFILE`
	VALU="$2"
	if [ -z "$VALU" ] ; then
	    VALU="<default>"
	    cat << !
<default> written to response file.
!
	fi

	if [ -n "$PRESENT" -a \( "$3" = "replace" -o "$3" = "delete" \) ]
	then #delete the value from response file
	    tempfile=/tmp/${RESOUTFILE}.$$

	    touch $tempfile ||
	    {
		cat << ! 

	Write error to /tmp directory. Check if you have
	permission to access the temp directory. 

!
	    }

	    grep -v "${1}=" $RESOUTFILE  >> $tempfile 
	
	    rm -f $RESOUTFILE || {
                cat << !

        Unable to replace the value for $1 in $RESOUTFILE
	check permission to modify $RESOUTFILE

!
               }

	    mv $tempfile $RESOUTFILE || {
                cat << !

	Unable to remove the value for $1 in $RESOUTFILE

!
               }
	fi # end replace/delete flag present
	
	[ "$3" = "delete" ] || echo "$1=$VALU" >> $RESOUTFILE
  else
       cat << !

ERROR: No parameters specified

Usage: check_response_write NAME VALUE [ replace | delete ] [allowdefaults]

!
  fi #parameter checking
}

#
# mkresponse_msg() -- writes a message that we are writing to the response file
#
mkresponse_msg()
{
cat << !

    This is -mkresponse mode, no setup will be done yet. 
    The parameter values will be written to the 
    response file $RESOUTFILE. 

!
[ "$1" = "allowdefaults" ] && cat << !
    NOTE: in some cases taking the default value for a prompt
    will add a special default value "<default>" to the response
    file.   When this response file is then taken as input, this
    special value will cause the set up file to use the default
    value for its context, which may or may not be the same as
    the default value for the context where the response file was 
    generated.

    To assure that a specific value is used, type in that value,
    even if it matches the default value.

!
}

#
#=[ check_diskspace() ]===============================================
#
# Checks whether required amount of diskspace is free in specified locations,
# taking into account physical location of each of the Ingres locations
#
# Parameters:
#	$1 - Space required under II_SYSTEM
#	$2 - Space required under II_DATABASE
#	$3 - Space required under II_CHECKPOINT
#	$4 - Space required for Transaction Log
# 
# Returns:
#	0 - O.K.
#	1 - Bad Args
#	2 - II_SYSTEM insufficient space
#	3 - II_DATABASE insufficient space
#	4 - II_CHECKPOINT insufficient space
#	5 - II_LOG_FILE insufficient space
#	6 - II_DUAL_LOG insufficient space

check_diskspace()
{

[ $# -lt 3 ] && 
{
    error_box << !
Usage:
check_diskspace S1 S2 S2
	
	where S1, S2 & S3 are the required free space under II_SYSTEM,
	II_DATABASE and II_CHECKPOINT respectively
!
    return 1
}

rc=0
iisys_need=$1
data_need=$2
ckp_need=$3
log_need=$4
[ "$log_need" ] ||  log_need=0

iisys_dev=`df -k $II_SYSTEM | grep ^\/dev | awk '{print $1}'`
[ "$II_DATABASE" ] || II_DATABASE=`ingprenv II_DATABASE`
[ "$II_DATABASE" ] && data_dev=`df -k $II_DATABASE  |\
			grep ^\/dev | awk '{print $1}'`

[ "$II_CHECKPOINT" ] || II_CHECKPOINT=`ingprenv II_CHECKPOINT`
[ "$II_CHECKPOINT" ] && ckp_dev=`df -k $II_CHECKPOINT |\
			grep ^\/dev | awk '{print $1}'`

[ "$II_LOG_FILE" ] || II_LOG_FILE=`ingprenv II_LOG_FILE`
[ "$II_LOG_FILE" ] && log_dev=`df -k $II_LOG_FILE |\
			grep ^\/dev | awk '{print $1}'`

[ "$II_DUAL_LOG" ] || II_DUAL_LOG=`ingprenv II_DUAL_LOG`
[ "$II_DUAL_LOG" ] && dual_dev=`df -k $II_DUAL_LOG |\
			grep ^\/dev | awk '{print $1}'`

# Check each location, and set disk space needs for each device
# accordingly

# II_DATABASE
    if [ ! "$data_dev" ] || [ "$data_dev" = "$iisys_dev" ] ; then
	iisys_need=`eval expr $iisys_need + $data_need`
	data_loc=iisys
	data_dev=''
    else
	data_loc=data
    fi

# II_CHECKPOINT
    if [ ! "$ckp_dev" ] || [ "$ckp_dev" = "$iisys_dev" ] ; then
	iisys_need=`eval expr $iisys_need + $ckp_need`
	ckp_loc=iisys
	ckp_dev=''
    else
	case "$ckp_dev" in
	    "$data_dev") ckp_loc=data 
			 ckp_dev=''
			 data_need=`eval expr $data_need + $ckp_need`
			 ;;
	   	      *) ckp_loc=ckp
		   	 ;;
	esac
	
    fi

# II_LOG_FILE
    if [ ! "$log_dev" ] || [ "$log_dev" = "$iisys_dev" ] ; then
	iisys_need=`eval expr $iisys_need + $log_need`
	log_loc=iisys
	log_dev=''
    else
	case "$log_dev" in
	       "$data_dev") log_loc=data
			    log_dev=''
			    data_need=`eval expr $data_need + $log_need`
			    ;;
	  	"$ckp_dev") log_loc=ckp
			    log_dev=''
			    ckp_need=`eval expr $ckp_need + $log_need`
			    ;;
			 *) log_loc=log
			    ;;
	esac
    fi

# II_DUAL_LOG
    if [ "$dual_dev" = "$iisys_dev" ] ; then
	iisys_need=`eval expr $iisys_need + $dual_need`
	dual_loc=iisys
	dual_dev=''
    elif [ "$dual_dev" ] ; then
	case "$dual_dev" in
	       "$data_dev") dual_loc=data
			    dual_dev=''
			    data_need=`eval expr $data_need + $dual_need`
			    ;;
		"$ckp_dev") dual_loc=ckp
			    dual_dev=''
			    ckp_need=`eval expr $ckp_need + $dual_need`
			    ;;
		"$log_dev") dual_log=log
			    log_need=`eval expr $log_need + $dual_need`
			    dual_dev=''
			    ;;
			 *) dual_log=dual
		    ;;
	esac

    fi

# Check the disk space on each of the devices
    [ "$iisys_dev" ] && 
    {
        free=`iidsfree $II_SYSTEM`
        [ $free -lt $iisys_need ] &&
        {
	    error_box << EOF
ERROR:
II_SYSTEM location:

	$II_SYSTEM

has only ${free}KB of free space
${iisys_need}KB is required to install this package
EOF

            rc=2
	}
    }

    [ "$data_dev" ] && 
    {
        free=`iidsfree $II_DATABASE`
        [ $free -lt $data_need ] &&
        {
	    error_box << EOF
ERROR:
II_DATABASE location:

	$II_DATABASE

has only ${free}KB of free space
${data_need}KB is required to install this package
EOF

            rc=3
	}
    }

    [ "$ckp_dev" ] && 
    {
        free=`iidsfree $II_CHECKPOINT`
        [ $free -lt $ckp_need ] &&
        {
	    error_box << EOF
ERROR:
II_CHECKPOINT location:

	$II_CHECKPOINT

has only ${free}KB of free space
${ckp_need}KB is required to install this package
EOF

            rc=4
	}
    }

    [ "$log_dev" ] && 
    {
        free=`iidsfree $II_LOG_FILE`
        [ $free -lt $log_need ] &&
        {
	    error_box << EOF
ERROR:
II_LOG_FILE location:

	$II_LOG_FILE

has only ${free}KB of free space
${log_need}KB is required to install this package
EOF

            rc=5
	}
    }

    [ "$dual_dev" ] && 
    {
        free=`iidsfree $II_DUAL_LOG`
        [ $free -lt $dual_need ] &&
        {
	    error_box << EOF
ERROR:
II_DUAL_LOG location:

	$II_DUAL_LOG

has only ${free}KB of free space
${dual_need}KB is required to install this package
EOF

            rc=6
	}
    }

    return $rc
} # check_diskspace

#
# set_setuid() -- Sets the SETUID bit for all ingres executables.
#
set_setuid()
{
SETUIDFILES="$II_SYSTEM/ingres/bin/iimerge
	     $II_SYSTEM/ingres/bin/lp64/iimerge
	     $II_SYSTEM/ingres/utility/csreport
	     $II_SYSTEM/ingres/utility/lp64/csreport
	     $II_SYSTEM/ingres/files/iipwd/ingvalidpw.dis
	     $II_SYSTEM/ingres/sig/star/wakeup"

for f in $SETUIDFILES
do
[ -x $f ] && chmod u+s $f
done

}

#=[ quiet_comsvrs ]==========================================================
#
# Set startup count for all "com" servers to 0 and store current values.
# Startup counts can be restored using restore_comsvr
#
quiet_comsvrs()
{
    # If Star is in use, we need to upgrade iidbdb without Star first,
    # and then start Star and do the rest.  The Star server opens
    # iidbdb and would get an error.
    STAR_SERVERS=`iigetres ii.$CONFIG_HOST.ingstart."*".star`
    [ -z "$STAR_SERVERS" ] && STAR_SERVERS=0
    if [ $STAR_SERVERS -ne 0 ]
    then
	$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".star 0
    fi

    # If ICE is in use, turn off the ICE server until we get databases
    # upgraded.
    ICE_SERVER=`iigetres ii.$CONFIG_HOST.ingstart."*".icesvr`
    [ -z "$ICE_SERVER" ] && ICE_SERVER=0
    if [ $ICE_SERVER -ne 0 ]
    then
	$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".icesvr 0
    fi

    # rmcmd was turned off already, above.
    # Make sure the variable is defined in case of drop out and restart.
    # (ideally the original would be remembered somewhere semi permanent,
    # oh well.)
    [ -z "$RMCMD_SERVERS" ] && RMCMD_SERVERS=0
    
    # If gcd is in use, turn off the gcc server until we get databases
    # upgraded.
    GCD_SERVER=`iigetres ii.$CONFIG_HOST.ingstart."*".gcd`
    [ -z "$GCD_SERVER" ] && GCD_SERVER=0
    if [ $GCD_SERVER -ne 0 ]
    then
	$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".gcd 0
    fi

    # If gcc is in use, turn off the gcc server until we get databases
    # upgraded.
    GCC_SERVER=`iigetres ii.$CONFIG_HOST.ingstart."*".gcc`
    [ -z "$GCC_SERVER" ] && GCC_SERVER=0
    if [ $GCC_SERVER -ne 0 ]
    then
        $DOIT iisetres ii.$CONFIG_HOST.ingstart."*".gcc 0
    fi

    # If Bridge Server is in use, turn it off until upgrading is done.
    ## I've seen 2.5 installations with the Bridge server turned on,
    ## apparently by default;  and when r3 tried to start, its Bridge
    ## server pooped out and failed the upgrade.  Let's get thru the
    ## upgrade and the Ingres admin can worry about it later.
    GCB_SERVER=`iigetres ii.$CONFIG_HOST.ingstart."*".gcb`
    [ -z "$GCB_SERVER" ] && GCB_SERVER=0
    if [ $GCB_SERVER -ne 0 ]
    then
	$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".gcb 0
    fi

    # Export values
     export STAR_SERVERS ICE_SERVER RMCMD_SERVERS GCD_SERVER \
	GCC_SERVER GCB_SERVER
 
}

#=[ restore_comsvrs ]=======================================================
#
#	Restore startup counts of all "com" servers to values stored by
#	quiet_comsvrs
#
restore_comsvrs()
{
    if [ $RMCMD_SERVERS -ne 0 ] ; then
	$DOIT iisetres ii.$CONFIG_HOST.ingstart."*".rmcmd 1
    fi
    if [ $GCC_SERVER -ne 0 ] ; then
	$DOIT iisetres  ii.$CONFIG_HOST.ingstart."*".gcc $GCC_SERVER
    fi
    if [ $GCD_SERVER -ne 0 ] ; then
	$DOIT iisetres  ii.$CONFIG_HOST.ingstart."*".gcd $GCD_SERVER
    fi
    if [ $ICE_SERVER -ne 0 ] ; then
	$DOIT iisetres  ii.$CONFIG_HOST.ingstart."*".icesvr $ICE_SERVER
    fi
    if [ $GCB_SERVER -ne 0 ] ; then
	$DOIT iisetres  ii.$CONFIG_HOST.ingstart."*".gcb $GCB_SERVER
    fi
}
