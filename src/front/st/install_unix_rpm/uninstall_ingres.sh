:
#
#  Copyright (c) 2005, 2008 Ingres Corporation
#
#  Name: uninstall_ingres.sh -- Uninstall Ingres on Linux.
#
#  Usage:
#       uninstall_ingres.sh { [package ID] | [ -a (--all) ] } 
#		[ -y (--yes) ] [ -c (--clean) ]
#
#  Description:
#	Provides a single point of accesss for removing any or 
#	all Ingres instances installed on a machine using RPM.
#
#	By default (run without any parameters) it will generate
#	a list of standard (non renamed) Ingres rpms installed 
#	and prompt for confirmation before removing them.
#
#  Parameters:
#	package ID 	- Two character ID embedded in the names of 
#			  renamed package to be removed.
#			  NOTE: This refers explicitly to the RPM package
#			  names and NOT the value of II_INSTALLATION for
#			  the instance to be removed.
#		-a	- Remove ALL Ingres RPM packages for ALL Ingres
#                         installations. CANNOT be used if specifying an
#                         installation ID
#		-y	- Answer 'yes' to all prompts (do not prompt)
#		-c	- Remove the $II_SYSTEM/ingres directory after
#			  the uninstall has completed.
#	
#
#  Exit Status:
#       0       uninstall procedure completed.
#       1       not run as root
#       2       invalid argument
#       3       uninstall declined
#
## DEST = utility
##
## History:
##
##    06-Jan-2005 (hanje04)
##	Created.
##    02-Feb-2005 (hanje04)
##	Fix up remove_all, change syntax of a couple of messages, set $host
##    23-Feb-2005 (hanje04)
##	Use character sets (e.g. [:upper:]) instead of ranges (e.g A-Z) in 
##	pattern matching as ranges can mean different things depending on
##	collation. 
##	Fix method for detemining II_SYSTEM for installed packages, previous
##	method only gave default prefix value and not actual.
##    22-Mar-2005 (hanje04)
##	Fix -c flag, $II_SYSTEM/ingres wasn't being removed.
##	Also tighten up use of -a with specified installation ID.
##	If both are specified script should fail. As -a means ALL Ingres
##	RPMS. Change usage message too.
##    28-Apr-2005 (hanje04)
##	In remove_all() fix up error message to return correct package name
##	when reporting it as invalid.
##	Rename pkglst variable in same function to elimiate confusion with
##	pkglist which is used elsewhere.
##	Improved grep mask for locating individual installs so that it works
##	with new 32bit package.
##    11-Jul-2005 (hanje04)
##	BUG 114823
##	Use whoami not $USER to check user's ID. $USER doesn't get changed 
##	by su.
##    17-Aug-2005 (hanje04)
##	BUG 110569
##	Better maintain state info so that script doesn't always report
##	"All specified RPM installations have been removed."
##	Correct typos
##    04-Oct-2005 (hanje04)
##	BUG 115267
##	Impove usage message so that it is clear what the platform ID refers
##	to.
##    19-Jan-2006 (hanje04)
##	SIR 115662
##	Update for Ingres 2006 rebranding changes. Will only remove
##	ingres2006 packages.
##    01-Feb-2006 (hanje04)
##	SIR 115688
##	Replace references to CATOSL with license.
##	07-May-2006 (hanje04)
##	   Bump version to ingres2007
##	23-Oct-2006 (bonro01)
##	   Change version back for ingres2006 release 2
##	22-Jan-2007 (bonro01)
##	   Change version back for ingres2006 release 3
##	01-Feb-2007 (hanje04)
##	    SD 113663
##	    BUG 117596
##	    Don't remove documentation package with the rest of the save set.
##      11-Aug-2008 (hweho01)
##	   Change package name from ingres2006 to ingres for 9.2 release,
##	   use (PROD_PKGNAME) which will be substituted by jam during build.
##	07-Jul-2010 (hanje04)
##	   BUG 124047
##	   Tighten up search patten so we don't pick up ingresvw when looking
##	   for ingres-... packages.
##	   Script seems to be quite badly broken for Ingres 10 in a number of
##	   places, (-a would miss non-renamed packages) so fix up to find all
##	   (PROD_PKGNAME) packages (including ingres2006 and ca-ingres, when
##	   PROD_PKGNAME is ingres).
##
self=`basename $0`
host=`hostname`
inst_id=''
rm_all=false
clean=false
prompt=true
force=false
num_ok=0
num_fail=0

#
#-[ usage() -  Print syntax message ]-----------------------------------------
#
usage()
{
    cat << EOF
Usage:
    $self [ package ID] | [ -a (--all) ] } [ -y (--yes) ] 
			[ -c (--clean) ]

where:
	package ID 	- Two character ID embedded in the names of 
			  renamed package to be removed.
			  NOTE: This refers explicitly to the RPM package
			  names and NOT the value of II_INSTALLATION for
			  the instance to be removed.
		-a	- Remove ALL Ingres RPM packages for ALL Ingres
			  installations. CANNOT be used if specifying a
			  package ID
		-y	- Answer 'yes' to all prompts (do not prompt)
		-c	- Remove the $II_SYSTEM/ingres directory after
			  the uninstall has completed.

WARNING!
If the -y option is specified with the -c option the installation location 
and everything underneath it will be removed without additional prompting.
EOF
}
# usage()

prompt()
{
    read -p "Do you want to continue? " resp
    case "$resp" in
	     [Yy]|\
     [Yy][Ee][Ss])
		    return 0
	   	    ;;
	        *)
		    return 1
		    ;;
    esac
}
# prompt()

#
#-[ clean_exit() ] - Clean up and exit with specified error or zero-----------
#
clean_exit()
{
    # Set return code
    rc=0
    [ $1 ] && rc=$1

    # Reset any traps and exit
    trap - 0
    exit $rc
}
# clean_exit()

#
#-[ get_prefix() ] - Get RPM Prefix value for given package
#
get_prefix()
{
    [ "$1" ] || return 1
    rpm -ql $1 | grep iisysdep | sed "-e s:/ingres/utility/iisysdep::"
}
# get_prefix()

#
#-[ rpm_remove() ] - Remove specified RPMS
#
rpm_remove()
{
    if $force ; then
	rmflags="-e --allmatches --noscripts --notriggers"
    else
	rmflags="-e --allmatches"
    fi

    [ "$1" ] || return 1
    # remove all versions of specified packages
    rpm $rmflags "$@"
}
# rpm_remove()

#
#-[ remove_installation() ] - Remove specified RPM installation(s) if no
# installation ID is specified the ingres group of packages will be
# removed
#
remove_installation()
{
    [ "$1" ] || return 1

    echo "Generating package list for the $1 installation..."
    # Remove each specified installation in turn
    while [ "$1" ]
    do 
	case "$1" in
	  [A-Z][A-Z]|\
          [A-Z][0-9])
			pkglist=`rpm -qa | \
				     egrep "(PROD_PKGNAME)(2006)*-.*${1}" |\
				     grep -v license`
			core=`rpm -qa | \
				egrep "(PROD_PKGNAME)(2006)*-${1}-[1-9][0-9]*\."`
			;;
	   ingres)
			pkglist=`rpm -qa | egrep "(PROD_PKGNAME)(2006)*-" | \
				    grep -v [[:upper:]] | \
				    egrep -v "license|documentation"`
			core=`rpm -qa | \
				egrep "(PROD_PKGNAME)(2006)*-[1-9][0-9]*\."`
			;;
	     *)
			echo "$1 in an invalid identifier, skipping..."
			shift
			continue
			;;
	esac

	[ -z "$pkglist" ] &&
	{
	    inst=$1
	    [ "$1" = "def" ] && inst=default
	    cat << EOF
No packages found for the $1 installation, skipping...

EOF
	    shift
	    continue
	}

	# If we're cleaning out everything get II_SYSTEM
	$clean && ii_sys=`get_prefix $core`

	$prompt &&
	{
	    echo "The following packages are about to be removed..."
	    echo 
	    for pkg in $pkglist
	    do
		echo $pkg
	    done
	    echo

	    # Confirm removal with user
	    prompt ||
	    {
		shift
		continue
	    }
	}

	# Remove RPMs 
	rpm_remove $pkglist ||
	{
	    if $force ; then
		cat << EOF
Unable to remove installation. 
Please contact Ingres Corporation Technical support.

EOF
		export force=false
		# flag error
		export num_fail=`eval expr $num_fail + 1`
		shift
		continue
	    else
		cat << EOF
One or more error occured during uninstallation. Re-trying...

EOF
		export force=true
		continue
	    fi
	}
	export force=false


# If removal is sucessfull cleanup if we're meant to
	$clean && [ -d "$ii_sys" ] &&
	{
	    $prompt &&
	    {
		cat << EOF
The following directory and everything underneath it is about to be removed:

	$ii_sys/ingres

EOF
		prompt || { shift && continue ; }
	    }

   	    echo "Removing $ii_sys/ingres..."
	    rm -rf $ii_sys/ingres
	}
	# Bump removal count
	export num_ok=`eval expr $num_ok + 1`
	shift
    done
}
# remove_insatlation

#
#-[ remove_all() - remove ALL ingres RPM installations from host -------------
#
remove_all()
{
    instlst=/tmp/ingrpm$$.lis
    # Check user really wants to do this
    $prompt &&
    {
	cat << EOF
WARNING!
You are about to remove ALL Ingres RPM installations from $host

EOF
	prompt || clean_exit 3
    }
    # Get RPM package list ignoring ingres-license
    echo "Generating list of installations..."
    rpm -qa | grep -v license | \
	egrep "(PROD_PKGNAME)(2006)*-([[:upper:]]|[[:digit:]]+\.[[:digit:]])" |\
	 sort > $instlst
    
    # Remove each installation in turn
    for pkg in `cat $instlst`
    do
 	inst=`echo "$pkg" | cut -d- -f2`
	case $inst in
	    # Renamed installation
	    [A-Z][A-Z]|\
	    [A-Z][0-9])
			remove_installation $inst
			;;
	    # 'Vanilla' installation
   1[0-9].[0-9]\.[0-9]|\
   [3-9]\.[0-9]\.[0-9])
			remove_installation (PROD_PKGNAME)
			;;
		     *)
			# Not installation at all
			echo "$pkg is not a Ingres RPM package, skipping..."
			continue
			;;
	esac
    done

    return 0
}
# remove_all


#
#-[ Main - main body of script ]----------------------------------------------
#

# Startup message
cat << EOF
Uninstall Ingres

EOF

## Check user
if [ "`whoami`" != "root" ] ; then
    cat << EOF
$self must be run as root

EOF
    usage
    exit 1
fi

while [ "$1" ]
do
    case $1 in 
        [A-Z][A-Z]|\
        [A-Z][0-9])
		    if [ "$inst_id" ] ; then
			inst_id="$inst_id $1"
		    else
			inst_id=$1
		    fi
		    shift
                   ;;
	  --all|-a)
		    rm_all=true
		    shift
		    ;;
	  --yes|-y)
		    prompt=false
		    shift
		    ;;
	--clean|-c)
		    clean=true
		    shift
		    ;;
		 *)
		    usage
		    exit 2
		    ;;
    esac
done

if $rm_all ; then
    # Cannot specify with installation ID
    [ "$inst_id" ] && 
    {
	echo "Cannot specify -a or --all with $inst_id"
	echo ""
	usage
	clean_exit 1
    }

    # Remove all ingres installations
    remove_all
elif [ "$inst_id" ] ; then
    for id in $inst_id
    do
    # Remove each specified ingres installation
	remove_installation $id
    done
else
    # Remove standard (not renamed) ingres packages
	remove_installation (PROD_PKGNAME)
fi

if [ $num_ok -eq 0 ] && [ $num_fail -eq 0 ] ; then
    echo "No RPM installations have been removed."
elif [ $num_ok -eq 0 ] && [ $num_fail -gt 0 ] ; then
    cat << EOF
WARNING!
Only $num_ok of the specified installations were successfully removed.
Errors occured during the removal of $num_fail installations.
EOF
else
    echo "All specified RPM installations have been removed."
fi
exit $num_fail
	   

