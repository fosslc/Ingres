:
#
# Copyright (c) 2004 Ingres Corporation
#
# Development shutdown of the DBMS server and related processes
#
# History:
#	24-Mar-89 (GordonW)
#		initial addition
#	02-Apr-89 (GordonW)
#		added kill to iislave processes
#	04-Apr-89 (GordonW)
#		the loop for killing iislaves could loop forever.
#		exit if kill fails.
#	08-Apr-89 (GordonW)
#		modified for SYSV ps command
#		and made easier to read
#	14-Apr-89 (GordonW)
#		moved from tools/specials_unix to here (tools/shell_unix)
#	09-Jun-89 (GordonW)
#		don't remove the log files unless -d is specified on 
#		command line.
#	16-jun-89 (daveb)
#		kill iigcc and iigcn too.
#	20-June-1989 (fredv)
#		Added f option to SYSV ps command because ps -el only shows
#		iimerge processes, not dmfacp, dmfrcp, iidbms if they are
#		symbolic links.
#	01-Jul-89 (GordonW)
#		revamped to use II_INSTALLATION code
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
# set -x

# first find all the servers based upon this installation
II="*`ingprsym II_INSTALLATION`"
if [ "$II" = "*" ]
then
	echo "You can't use this to kill installation"
	echo "You have no II_INSTLLATION defined"
	exit 0
fi
II=`ingprsym II_INSTALLATION`
echo "Destroying II_INSTALLATION $II"

# run cscleanup
cscleanup >/dev/null 2>&1

# Remove log files
if [ "$1" = "-d" ]
then
	log=`echo $II_SLAVE_LOG`*
	if [ "$log" != "*" ]
	then
		echo "rm -f $log"
		rm -f $log
	fi
	echo "rm -f $ING_BUILD/files/errlog.log"
	echo "rm -f $II_DBMS_LOG"
	echo "rm -f $ING_BUILD/files/*ACP.LOG"
	echo "rm -f $ING_BUILD/files/*RCP.LOG"
	rm -f $ING_BUILD/files/errlog.log
	rm -f $II_DBMS_LOG
	rm -f $ING_BUILD/files/*ACP.LOG
	rm -f $ING_BUILD/files/*RCP.LOG
fi

# Remove locking files
echo "rm -f $ING_BUILD/files/lockseg"
echo "rm -f $ING_BUILD/files/sysseg"
echo "rm -f $ING_BUILD/files/server.?"
rm -f $ING_BUILD/files/lockseg
rm -f $ING_BUILD/files/sysseg
rm -f $ING_BUILD/files/server.?

# what kind of system are we running on BSD/SYSv
PS="aux"
SYSv="no"
if (uname </dev/null) >/dev/null 2>&1
then
	PS="efl"
	SYSv="yes"
fi

# Now kill off the server programs
for PRG in iidbms dmfacp dmfrcp iigcc iigcn
do
	set -$- `ps -$PS | grep $PRG | grep $II | grep -v "grep $PRG"`
	if [ "$SYSv" = "yes" ]
	then
		if [ "$3" = "" ]
		then
			continue
		fi
		if shift >/dev/null 2>&1
		then
			shift
		else
			continue
		fi
	fi
	if [ "$2" != "0" ]
	then
		WHAT="-9 $2"
	else
		WHAT="-9 "
	fi
	if [ "$WHAT" != "-9 " ]
	then
		echo "kill $WHAT ($PRG)"
		kill $WHAT
	fi
done

# for now don't kill any slaves...
exit 0

# kill the  SLAVE processes
sleep 5
last2=""
while [ "1" = "1" ]
do
	set -$- `ps -$PS | grep iislave | grep $II | grep -v "grep iislave"`
	if [ "$SYSv" = "yes" ]
	then
		if [ "$3" = "" ]
		then
			break
		fi
		if shift >/dev/null 2>&1
		then
			shift
		else
			break
		fi
	fi
	if [ "$2" != "0" ]
	then
		iislave="-9 $2"
	else
		iislave="-9 "
	fi
	if [ "$iislave" != "-9 " ]
	then
		if [ "$iislave" = "$lastslave" ]
		then
			break
		fi
		echo "kill $iislave (iislave)"
		if kill $iislave
		then
			lastslave=$iislave
			continue
		else
			break
		fi
	else
		break
	fi
done

exit 0
