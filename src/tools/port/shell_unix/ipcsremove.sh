:
#
# Copyright (c) 2004 Ingres Corporation
#
# ipcsremove:
#	Remove the ingres ipcs sections (shared memory, semaphores, and
#	Message queues).
#
# History:
#	02-Apr-89 (GordonW)
#		initial version
#	09-Apr-89 (GordonW)
#		added echo..
#	14-Apr-89 (GordonW)
#		movwed from tools/specials_unix to here (tools/shell_unix)
#	09-Jun-89 (GordonW)
#		att try at ipcs
#		and added logic for RT's garbled output
#	06-Jul-89 (GordonW)
#		revamped to kill off only II_INSTLLATION related ipcs.
#	17-Jul-89 (GordonW)
#	 	didn't work quite well, not removing all ipcs.
#	24-jul-89 (daveb)
#		Reorder for use on sequent with funny mmap SV shmem.
#	31-jul-89 (kimman)
#		Added shell commands to obtain the machine config name and to
#		use this config name to determine if we should use sed or cut.
#	31-jul-1989 (boba)
#		Change conf to tools/port$noise/conf.
#	6-oct-89 (daveb)
#		No ipc on convex.
#	6-oct-89 (daveb)
#		Gaak, fix egregious errors in previous convex change -- 
#		remove hasipcs half-logic, and use -d instead of -f.
#	16-oct-92 (lauraw)
#		Uses new readvers to get config string.
#	26-feb-98 (toumi01)
#		Different ipcrm syntax for Linux (lnx_us5)
#	14-jan-99 (toumi01)
#		Fix syntax errors in Linux integration (missing ;; in cases).
#	10-may-1999 (walro03)
#		Remove obsolete version string vax_ulx.
#	06-oct-1999 (toumi01)
#		Change Linux config string from lnx_us5 to int_lnx.
#	15-Jun-2000 (hanje04)
#		Add ibm_lnx to Linux syntax for ipcrm
#	11-Sep-2000 (hanje04)
#		Added axp_lnx (Alpha Linux) to icprm change.
#	03-Dec-2001 (hanje04)
#		Added support for IA64 Linux (i64_lnx)
#	05-Nov-2002 (hanje04)
#		As part of AMD x86_64 Linux port, genericise Linuxisms
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.

# set -x

[ -d /usr/convex ] && echo "No ipc on Convex to remove" && exit 0

tmpfile=/tmp/ipcs.$$
ipcfile=$ING_BUILD/ipcs.list

. readvers
vers=$config

# what ipcs command do we use ?
if (ipcs </dev/null) >/dev/null 2>&1
then
	whichipcs=ipcs
	whichipcrm=ipcrm
else
	if (att ipcs </dev/null) >/dev/null 2>&1
	then
		whichipcs="att ipcs"
		whichipcrm="att ipcrm"
	else
		echo "How do you remove the IPCS?"
		exit 1
	fi
fi

# if there is a ipcs.list file
if [ -f $ipcfile ]
then

	case $vers in
	    ds3_ulx)        
		sed -n "/ingres/s/< /$whichipcrm -/gp" $ipcfile |\
		cut -c1-16 | grep $whichipcrm >$tmpfile ;;

	    *)             
		sed -n "/ingres/s/< /$whichipcrm -/gp" $ipcfile |\
		sed -e "s/0x/#/g" | tr "#" "\012" | grep $whichipcrm >$tmpfile;;
	esac

	sh -x $tmpfile && rm -f $tmpfile && exit 0
fi

# remove Sequent stuff

ls -l /usr/tmp/SysVshmem/*  2>&1 | grep "ingres" > /dev/null  && ( 
	echo "csclean: Remove shared memory files..."
	rm -f /usr/tmp/SysVshmem/*
)

# if $whichipcs -m -s | grep ingres
# then
# 	echo "" >/dev/null
# else
# 	exit 0
# fi

# remove semaphores
last2=""
while [ "1" = "1" ]
do
	if $whichipcs -s | grep ingres >$tmpfile 2>&1
	then
		set -$- `cat $tmpfile`
		if [ "$last2" = "$2" ]
		then
			rm -f $tmpfile
			break
		else
			if [ "$1" != "s" ]
			then
				arg2=`echo $1 | sed -e 's/^s//g'`
			else
				arg2=$2
			fi
			case $vers in
			      *_lnx|\
                            int_rpl)        
				echo "$whichipcrm sem $arg2"
				$whichipcrm sem $arg2 ;;
			    *)
				echo "$whichipcrm -s $arg2"
				$whichipcrm -s $arg2 ;;
			esac
			last2=$2
		fi
	else
		rm -f $tmpfile
		break
	fi
done

# remove shared memory
last2=""
while [ "1" = "1" ]
do
	if $whichipcs -m | grep ingres >$tmpfile 2>&1
	then
		set -$- `cat $tmpfile`
		if [ "$last2" = "$2" ]
		then
			rm -f $tmpfile
			break
		else
			if [ "$1" != "m" ]
			then
				arg2=`echo $1 | sed -e 's/^m//g'`
			else
				arg2=$2
			fi
			case $vers in
			      *_lnx|\
                            int_rpl)        
				echo "$whichipcrm shm $arg2"
				$whichipcrm shm $arg2 ;;
			    *)
				echo "$whichipcrm -m $arg2"
				$whichipcrm -m $arg2 ;;
			esac
			last2=$2
		fi
	else
		rm -f $tmpfile
		break
	fi
done
exit 0
