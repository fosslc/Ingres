:
#
# Copyright (c) 2004 Ingres Corporation
#
# Convert an ACstart config file to an Achilles form
#
# Usage:
#	acconfig [-o filename] [-r n] [-u username] [-d dbname] cfg_filename
#
# History:
# --------
##	17-Aug-93 (GordonW)
##		written.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

[ "$WHOAMI" = "" ] && . $II_SYSTEM/ingres/utility/iisysdep
[ $SHELL_DEBUG ] && set -x

# Setup default(s) 
myname=`basename $0`
dbname="<DBNAME>"
child="<CHILDNUM>"
ifile="<NONE>"
ofile="<NONE>"
usr=$WHOAMI
iters=1

# Grab args
while true
do
	[ "$1" = "" ] && break
	if [ "X$1" = "X-o" ] 
	then
		shift
		ofile=$1
	elif [ "X$1" = "X-r" ] 
	then
		shift
		iters=$1
	elif [ "X$1" = "X-u" ] 
	then
		shift
		usr=$1
	elif [ "X$1" = "X-d" ] 
	then
		shift
		dbname=$1
	else 
		cfg=$1
		if [ ! -r $cfg -a ! -r $achilles_config/$cfg ]
		then
			echo "$myname: Cannot access $cfg" >&2
			exit 1
		fi
	fi
	[ "$2" = "" ] && break
	shift
done

# now read $TST_CFG
num=0
while read line
do
	# Comment line ?
	num=`expr $num + 1`
	if echo $line | grep '^#' >/dev/null 2>&1
	then
		continue
	fi

	# blank line ?
	if [ `echo $line | wc -c` -le 1 ]
	then
		continue
	fi

	# any test parameters ";"
	if echo $line | grep ';' >/dev/null 2>&1
	then
		nline=
		argvs=
		nargs=`echo $line | cut -d';' -f1`
		x=0
		for n in $nargs
		do
			[ $x -eq 0 ] && nline="$nline $n" || argvs="$argvs $n"
			x=`expr $x + 1`
		done
		nline="$nline `echo $line | cut -d';' -f2`"
		line=$nline
	fi

	# test legal line?
	set $line >/dev/null 2>&1
	if [ $# -ne 10 ]
	then
		echo "$myname: Error: $cfg line $num" >&2
		exit 1
	fi

	# parse Prg data
	prgline=`echo $line`
	set $prgline >/dev/null 2>&1
	prg=$1
	loops=$2
	i_slp=$3
	l_slp=$4
	q_slp=$5
	prgdata="$prg $dbname $argvs $loops $i_slp $child $l_slp $q_slp"

	# Parse Achilles data
	achline=`echo $line | awk '{print $6 " " $7 " " $8 " " $9 " " $10}'`
	set $achline >/dev/null 2>&1
	nkids=$1
	i_cnt=$2
	k_cnt=$3
	i_sec=$4
	k_sec=$5
	achdata="$nkids $iters $i_cnt $k_cnt $i_sec $k_sec $ifile $ofile $usr"

	echo "$prgdata ; $achdata"
done < $cfg
exit 0
