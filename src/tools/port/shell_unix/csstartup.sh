:
#
# Copyright (c) 2004 Ingres Corporation
#
# Development startup of the DBMS server and related processes
#
# History:
#	25-Mar-89 (GordonW)
#		initial addition
#	02-Apr-89 (GordonW)
#		added a stop point based upon argv 1
#	09-Apr-89 (GordonW)
#		changed so that rcpconfig.default is inputting
#		thru this shell script
#	14-Apr-89 (GordonW)
#		moved from tools/specials_unix to here (tools/shell_unix)
#	09_jun-89 (GordonW)
#		don't use absolute pathnames
#	10-June-89 (GordonW)
#		added IIGCC and IIGCN
#		added the ability to start with Name Server or not
#	18-jun-89 (daveb)
#		Start server -sole -fast -write 4 for speed.  Also
#		add -f option to not do rcpconfig and log file erase.
#	20-June-1989 (fredv)
#		Added tests to find a correct ps command and use env instead 
#		of printenv if printenv doesn't exist.
#	22-jun-89 (daveb)
#		Match tp1 benchmark server flags and rcpconfig data, except
#		for stack size which we leave at the default.
#	29-jun-89 (daveb)
#		Start up iigcn and iigcc with iirun, so the installation
#		shows up in the ps output.  Check for the installation when
#		looking for the processes.
#	06-Jul-89 (GordonW)
#		added logic to shutdown only the current II_INSTALLATION
#		we do this by asaving the IPCS before starting the
#		installation and agetting a diff after the instattlation is
#		up. These are the only IPCS we remove.
#	6-oct-89 (daveb)
#		No ipcs on convex.
#	1-nov-1990 (jonb)
#		Use "ensure" to make sure we're running as ingres, rather
#		than testing locally.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

# set -x

hasipcs=true
[ -d /usr/convex ] && hasipcs=false

ipcfile=$ING_BUILD/ipcs.list

dbmsopts="\
	-connected_sessions 50 \
	-cursors_per_session 16 \
	-database_count 8 \
	-sole_server \
	-fast_commit \
	-write_behind 4 \
	-dmf.cache_size 1024"

# one more than the number of write behind threads, sez, jeff anton
ingsetenv II_NUM_SLAVES 5

II_INSTALLATION=`ingprenv | grep II_INSTALLATION | sed 's/.*=//'`

fast=false
while :
do
	case $1 in
	"")	break;;
	-f)	fast=true;;
	*)	stop=$1;;
	esac
	shift
done

# 0) test if INGRES
ensure ingres
#
# get starting ipcs 
#
if $hasipcs
then
	tmpfile=/tmp/ipc1.$$
	if (ipcs </dev/null) >/dev/null 2>&1
	then
		whichipcs=ipcs
	else
		if (att ipcs </dev/null) >/dev/null 2>&1
		then
			whichipcs="att ipcs"
		else
			echo "How do you look at the IPCS?"
			exit 1
		fi
	fi
	$whichipcs | grep ingres | grep -v "grep ingres" >$tmpfile
fi

# Decide to use SYSV/BSD ps
if ps -xau 2> /dev/null > /dev/null
then
	pscmd="ps -aux"
else
	pscmd="ps -ef"
fi

# Decide to use printenv or env
if (printenv </dev/null) >/dev/null 2>&1
then
    envcmd=printenv
else
    envcmd=env
fi

# remove any logfiles and system resources
csshutdown
$hasipcs && ipcsremove

# 1) CSinstall
if [ "$stop" = "csinstall" ]
then
	echo "CSstartup: breakpoint hit: [$stop]"
	exit 0
fi
if csinstall
then
	echo "CSINSTALL OK"
else
	exit 1
fi
$whichipcs | grep ingres | grep -v "grep ingres" | diff - $tmpfile >$ipcfile

# 2) rcpconfig
if [ "$stop" = "rcpconfig" ]
then
	echo "CSstartup: breakpoint hit: [$stop]"
	exit 0
fi
case $fast in
false)
	if rcpconfig -init << EOF
8
128
32
1
16
98
65
95
511
511
5000
128
80
y
EOF
	then
		echo "RCPCONFIG OK"
	else
		exit 1
	fi
	;;
esac


# 3) dmfrcp
if [ "$stop" = "dmfrcp" ]
then
	echo "CSstartup: breakpoint hit: [$stop]"
	exit 0
fi
if iirun dmfrcp
then
	echo "DMFRCP OK"
else
	exit 1
fi

# 4) dmfacp
if [ "$stop" = "dmfacp" ]
then
	echo "CSstartup: breakpoint hit: [$stop]"
	exit 0
fi
if iirun dmfacp
then
	echo "DMFACP OK"
else
	exit 1
fi
$whichipcs | grep ingres | grep -v "grep ingres" | diff - $tmpfile >$ipcfile

# 5) iirundbms
NS=""
tst="*`$envcmd | grep II_GCC_TEST_NO_XLATE`"
if  [ "$tst" = "*" ]
then
	tst="*`ingprsym II_GCC_TEST_NO_XLATE`"
	if [ "$tst" = "*" ]
	then
		NS="-names"
	fi
fi
if [ "$NS" = "-names" ]
then
	echo "Using the Name Server"
else
	if [ "$stop" = "iirundbms" ]
	then
		echo "CSstartup: breakpoint hit: [$stop]"
		exit 0
	fi
	if [ "$stop" = "iidbms" ]
	then
		echo "CSstartup: breakpoint hit: [$stop]"
		exit 0
	fi
	echo "Not using the Name Server"
	if iirundbms $NS $dbmsopts
	then
		echo "IIRUNDBMS OK"
	else
		exit 1
	fi
fi

# 6) iigcn and iigcc
if [ "$NS" = "-names" ]
then
	if [ "$stop" = "iigcn" ]
	then
		echo "CSstartup: breakpoint hit: [$stop]"
		exit 0
	fi
	echo "Starting Name Server"
	iirun iigcn
	sleep 5
	num=`$pscmd | grep "iigcn $II_INSTALLATION" | wc -l | sed -e 's/ //g'`
	if [ "$num" = "1" ]
	then
		echo "IIGCN FAILED..."
	else
		echo "IIGCN OK"
	fi
else
	echo "Not starting the Name Server"
fi
if [ "$stop" = "iigcc" ]
then
	echo "CSstartup: breakpoint hit: [$stop]"
	exit 0
fi
echo "Starting Comm Server"
iirun iigcc
sleep 5
num=`$pscmd | grep "iigcc $II_INSTALLATION" | wc -l | sed -e 's/ //g'`
if [ "$num" = "1" ]
then
	echo "IIGCC FAILED..."
else
	echo "IIGCC OK"
fi

if [ "$NS" = "-names" ]
then
	if [ "$stop" = "iirundbms" ]
	then
		echo "CSstartup: breakpoint hit: [$stop]"
		exit 0
	fi
	if [ "$stop" = "iidbms" ]
	then
		echo "CSstartup: breakpoint hit: [$stop]"
		exit 0
	fi
	if iirundbms $NS $dbmsopts
	then
		echo "IIRUNDBMS OK"
	else
		exit 1
	fi
fi


exit 0
