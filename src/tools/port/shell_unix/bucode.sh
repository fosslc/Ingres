:
#
# Copyright (c) 2004 Ingres Corporation
#
# Backup piccolo working files, can be run as a nightly cron job
#
# scipt is designed
# for baroque paths, will need to be altered for porting paths
#
# $1 = base path (e.g. mainsol)
# $2 = private path (e.g.rplus)
# $3 = -port (optional flag to convert code path to porting path 
#		before backing up)
#
# Requires the directory $HOME/backups to exist
#
## jan-95 (stephenb)
##	written
## 28-apr-99 (peeje01)
##      correct path name of $ING_SRC
##      add usage message
## 12-oct-2001 (stephenb)
##	Add porting path function from my private version, and cleanup
##	code in general
## 25-oct-2001 (stephenb)
##	Add a multitude of functionality, including support for remote copy
##	and parameter parsing (making ordering irrelevant)
## 25-oct-2001 (bonro01)
##	Fixed if statements with missing quotes around variables.
##	Removed hardcoded /usr/local/bin/p location.
##	Added 'head -1' to limit p here to one output line.
##	Erased old compressed backup before compress command to 
##	eliminated (y/n) prompt to replace file.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

#Usage="`basename $0` base (eg ing30sol) ppath (eg rplus) [-port]"
if [ "$1"  = "-help" ] ; then
    echo "Usage:"
    echo "bucode [-base path] [-ppath pathname] [-port] [-host hostname [-user usernname]] "
    echo ""
    echo "-base path = Directory to root of source, $ING_SRC assumed if ommited"
    echo "-ppath pathname = Private path name, $ING_VERS or porting path assumedif ommited"
    echo "-port = convert baroque path to porting path before backing up"
    echo "-host hostname = rcp backup to remote host hostname"
    echo "-user username = use remote user username to rcp (requires -host)"
    exit
fi
# check environment
if [ -f `which p` ] ; then
    pbin=`which p`
else 
    echo "Cant find piccolo in path, trying /usr/local/bin/p"
    if [ -f /usr/local/bin/p ] ; then
    	pbin="/usr/local/bin/p"
    else
	echo "/usr/local/bin/p does not exist...exiting"
	exit 1
    fi
fi

if [ -d $HOME/backups ] ; then
    echo "using $HOME/backups as backup directory"
    budir=$HOME/backups
else
    echo "$HOME/backups not found, using /tmp as backup directory"
    budir=/tmp
fi

for parm in $*
    do
	if [ "$last" = "-base" ] ; then #expecting baseline name
	    ING_SRC=$parm
	    last=""
	elif [ "$last" = "-ppath" ] ; then #expecting ppath name
	    ING_VERS=$parm
	    last=""
	elif [ "$last" = "-host" ] ; then #expecting hostname
	    hostname=$parm
	    last=""
	elif [ "$last" = "-user" ] ; then #expecting username
	    username=$parm
	    last=""
	elif [ "$parm" = "-port" ] ; then
	    convert_port="TRUE"
	elif [ "$parm" = "-base" ] ; then
	    last="-base"
	elif [ "$parm" = "-ppath" ] ; then
	    last="-ppath"
	elif [ "$parm" = "-host" ] ; then
	    last="-host"
	elif [ "$parm" = "-user" ] ; then
	    last="-user"
	fi
    done

if  [ ! "$ING_SRC" ] ; then
    echo "ING_SRC not set, please set up your environment, exiting backup"
    exit 1
elif [ ! -d "$ING_SRC" ] ; then
    echo "$ING_SRC is not a valid directory, exiting..."
    exit 1;
fi
if [ ! "$ING_VERS" ] ; then
    echo "ING_VERS not set or provided, porting path assumed..."
fi
if [ "$user" ] && [ ! "$hostname" ] ; then
    echo "Username provided without remote hostname...exiting"
    exit 1
fi
# check mappath
if [ ! "$MAPPATH" ] ; then
    echo "MAPPATH not set, trying /devsrc/client.map..."
    MAPPATH=/devsrc/client.map
fi
if [ ! -f "$MAPPATH" ] ; then
    echo "Client mapfile not found, exiting...."
    exit 1
fi
# get current date and time for backup filename
date="`date`"
filedate=`echo $date | awk '{ m["Jan"]=1 ; m["Feb"]=2 ; m["Mar"]=3 ; m["Apr"]=4 ; m["May"]=5 ; m["Jun"]=6 ; m["Jul"]=7 ; m["Aug"]=8 ; m["Sep"]=9 ; m["Oct"]=10 ; m["Nov"]=11 ; m["Dec"]=12 ; printf "%02d%02d\n", m[$2],$3}'`

# temporary files
if [ "$ING_VERS" ] ; then
    opened=/tmp/opened.$ING_VERS
    bufiles=/tmp/bufiles.$ING_VERS
else
    opened=/tmp/opened.port
    bufiles=/tmp/bufiles.port
fi

# get real value of $ING_SRC (copes with soft liked directory paths)
SRC_ROOT=`cd $ING_SRC; pwd`

if [ "$ING_VERS" ] ; then
   codeline=$ING_VERS
else
   codeline="porting"
fi
echo ""
echo "Backup of base $ING_SRC, codeline $codeline at: $date"

# go to a mapped directory
if [ "$ING_VERS" ] ; then #should have a clients directory
    if [ -d $ING_SRC/clients/$ING_VERS ] ; then
	cd $ING_SRC/clients/$ING_VERS
    else
	echo "No clients directory....canot locate a mapped directory, exiting"
	exit 1
    fi
else # porting path, assume $ING_SRC is mapped
    cd $ING_SRC;
fi
# find client name
client=`$pbin here | head -1 | awk '{print $2}'`
if [ "$client" = "valid" ] ; then #location not mapped
    echo "`pwd` is not a mapped directory...exiting"
    exit 1
fi

# get working files
$pbin working > $opened

# find the location of the files, then let awk re-format to a nice
# format for tar
$pbin there -l $opened | awk '{print $3"/"$4}' | sed -e s,$SRC_ROOT/,,g > $bufiles
cd $SRC_ROOT
echo "Backing up the following files relative to $SRC_ROOT:"

# tar up the files (also add the opened list so we know what we had)
tar cvf $budir/bu_"$client"_$filedate.tar `cat $bufiles` $opened

#convert to porting path
if [ "$convert_port" ] ; then
    echo "Converting backup to porting path..."
    cd /tmp
    mkdir butmp
    cd butmp
    tar xf $budir/bu_"$client"_$filedate.tar
    find */*/$ING_VERS -name src -type d -exec mkports {} \;
    find */*/$ING_VERS -name hdr -type d -exec mkporth {} \;
    tar cf $budir/bu_"$client"_$filedate.tar * $opened
    cd ..
    rm -rf butmp
fi

# compress the backup to save space
if [ -f `which gzip` ] ; then
    echo "Using gzip to compress backup..."
    `which gzip` $budir/bu_"$client"_$filedate.tar
    filename=$budir/bu_"$client"_$filedate.tar.gz
elif [ -f `which compress` ] ; then
    echo "Using compress to compress backup..."
    filename=$budir/bu_"$client"_$filedate.tar.Z
    rm -f $filename
    `which compress` $budir/bu_"$client"_$filedate.tar
else
    filename=$budir/bu_"$client"_$filedate.tar
fi

#copy to remote host
if [ "$hostname" ] ; then
    echo "Copying backup to remote host $hostname"
    if [ "$username" ] ; then
	remhost=$username@$hostname
    else
	remhost=$hostname
    fi
    rcp $filename $remhost:
fi
exit 0
