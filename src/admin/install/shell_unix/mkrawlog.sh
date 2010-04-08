:
# Copyright (c) 2004 Ingres Corporation
#  $Header $
#
#  Name: mkrawlog - set up a raw device file as an INGRES transaction log
#
#  Usage:
#	mkrawlog [ -dual ]
#
#  Description:
#	Guide user thru procedure for setting up a raw device as
#	the INGRES log file.
#
#	This script must be run as 'root'.
#
#	The raw disk should just be any disk device that does not have a
#	unix file system on it (ie. /dev/rxy0d).  We will then create a hard 
#	link from $II_LOG_FILE/ingres/ingres_log to the disk device told us.  
#
#	If the parameter "dual" is passed, then we will create a hard
#	link from $II_DUAL_LOG/ingres/iidual_log to the disk device told us.
#   
#	We also check that the raw device is not created in partition a nor c.
#	These partitions are reserved to access a disk and raw devices should
#	not be created there.
#   
#	After everything is ok a hard link is created to the device for
#	$II_LOG_FILE/ingres/log/ingres_log or $II_DUAL_LOG/ingres/iidual_log.
#
# Exit status:
#	0	OK
#	1	must be root to run	
#	2	required symbol not defined
#	3	location does not exist or is non-writable
#	4	path not a character special device
#	5	device contains file system
#	6	can't determine size of device
#	7	device too small for log file
#	9	/etc/mknod of log file failed
#	10	cannot use partition a nor c to create a raw device
#	11	primary logfile is different size than dual
#	12	unable to install shared memory
#	13	iimklog failed
#	14	a transaction log already exists
#       15      rcpstat failed
#
## History
##	26-apr-1990 (kimman)
##		Adding fix if 'dd' doesn't return 0 but still returns
##		'f+p records in' and 'f+p records out' as we did in ingresug
##	11-may-90 (huffman)
##		Mass integration from PD.
##	27-jun-90 (jonb)
##		Use iisysdep header for system-dependent command definitions.
##	14-Nov-90 (sanjive)
##		Fix a bug. The file $II_CONFIG/rawlog.size was being created
##		with root as owner (since this script is run by root), making
##		it inaccessible to scripts which need to know the rawlog size.
##		Change owner of the file to ingres if it was created
##		successfully.
##	18-jun-91 (jonb)
##		Use sed instead of awk to get the major/minor numbers for
##		the device, so we don't need to know explicit column numbers.
##	24-jun-91 (hhsiung)
##		DPX/2 200 and DPX/2 300 using different format for
##		device name alought they binary compatable.
##		DPX/2 200 uses /dev/rdsk/9s9 and 300 uses /dev/rdsk/c9d9s9
##		Need using different sed commands
##	26-jun-91 (lauraw)
##		Use chown instead of /etc/chown (as per "UNIX Shell in
##		INGRES Product and Tools" standard) so it can be found via
##		path.
##	1-jul-91 (jonb)
##		Remove hard-coded path for mknod.
##	25-jul-91 (harry)
##		Make sure that symbol.tbl created here has proper ownerships
##		and permissions.
##	20-aug-91 (hhsiung)
##		Used BPATHSED defined in iisysdep ( from mksysdep.sh ) to
##		figure out what sed command should be used to get BPATH.
##		That will catch those machines have different representation
##		of raw log device name.
##		Also removed 6/24/91's machine specific change of the above
##		problem.
##      15-sep-1991 (aruna)
##		Cleared up the output from iiecholg that does not display
##		uniform boxes.
##	06-Nov-1991 (vijay)
##		Do not give warning about a and c partitions for machines
##		which don't have the limitation, namely IBM and HP.
##		Also removed extra iisysdep.
##		Make sure it is run by root.
##      12-Nov-1991 (gautam)
##		The df command used to check if the raw device is used for a
##		file system is different on the RS/6000.
##      20-dec-1991 (bryanp)
##		Added dual logging support. The dual log is made by passing
##		the parameter "dual". It is very similar to the primary log.
##		The basic differences are:
##		II_LOG_FILE ==> II_DUAL_LOG
##		ingres_log  ==> iidual_log
##		prompts are slightly altered
##	13-jan-1993 (terjeb)
##		Replace direct sed call with SED_MAJOR_MINOR string, which 
##		have been added to mksysdep/iisysdep. This takes care of 
##		the HP-UX platforms where minor numbers are expressed 
##		slightly different. Also fixed quoting on BPATHSED.
##	15-mar-1993 (dianeh)
##		Changed calls to ingprenv1 to ingprenv (ingprenv1 gone for 6.5).
##	24-jun-1993 (tyler)
##		Cleaned up and made to work with the Onyx tools.  Removed
##		call to chkins (defunct).  rawlog.size is now stored
##		in the ingres/log directory instead of II_CONFIG.  Removed
##		reference to rcpconfig.dat (defunct).  Raw log size (bytes)
##		is now stored in config.dat (ii.*.rcp.file.size).
##	18-oct-1993 (tyler)
##		Inserted missing '/' in rawlog.size path.  Create config.dat
##		if it doesn't exist.  Explicitly test for character special
##		transaction log since -f test doesn't evaluate to true for 
##		character special files on all systems.
##	08-nov-93 (vijay)
##		Run csinstall as ingres. Since rcpconfig is setuid
##		ingres, it fails to format the log if root creates sysseg.
##	26-nov-93 (tyler)
##		Remove calls to defunct scripts iiecholg and iiechonn.
##	29-nov-93 (tyler)
##		iiechonn reinstated.
##	05-jan-93 (tyler)
##		Corrected broken test for character special file.
##	11-jan-94 (tyler)
##		Fixed BUG 57808 which complained that message should 
##		warn against using using partitions which begin with 
##		cylinder 0, instead of "a" or "c".  Execute iimklog
##		as installation owner. 
##	12-jan-94 (tyler)
##		Use $SU_FLAGS (now defined in iisysdep) to pass the
##		-f option (in addition to -c) to su which is needed on
##		BSD machines to avoid picking up a bogus environment
##		(if the installation owner's default SHELL is csh).
##	31-jan-94 (tyler)
##		Removed debugging statement.
##      29-jan-95 (tutto01)
##              Added -L option to "ls" command in order to reference
##              the actual file rather than a possible link.  Now a
##              link to a character special file can be used. (bug 63139)
##	 9-oct-1995 (nick)
##		Fix a mass of problems - we can get 'C' back off character
##		special device as well as 'c'.  Also the use of 
##		USE_SYMBOLIC_LINKS appears to never reached here.
##		Correct mess in attempts to determine size of other 
##		transaction logfile ; we can't rely on the existence of
##		'ls -L' - use the AWKBYTES value obtained from iisysdep
##		instead since that is what it is there for.
##		Test for other transaction logfile just used '-f' - this
##		just tests for a regular file on many boxes so use '-c' as
##		well.
##		Calls to 'rcpstat' need to be through 'su'.
##		Use 'ls -ln' to avoid problems where group name is greater
##		that eight characters and hence overflows into size.
##		AIX can return 'Frw-' if the logfile is NFS mounted !
##		Use AWKOWNER to determine owner of other logfile and compare
##		with $ING_USER, not 'ingres'.
##		Comparison of sizes compared bytes with kbytes !
##	 9-apr-96 (nick)
##		Ensure all components of the alternate logfile exist before
##		constructing the path/filename to test.  #75512
##	28-jun-96 (nick)
##		Minimum logfile size is now 16MB. #77442, #77444
##	13-Nov-1997 (hanch04)
##	    Changed DEVICE_BYTES to use DEVICE_KBYTES for large file support
##	    We keep track of Kbytes instead of bytes
##      20-apr-98 (mcgem01)
##              Changed OpenIngres to Ingres.
##      29-aug-95 (stephenb)
##              Mostly re-written to cope with multiple log partitions.
##      29-aug-1998 (hanch04)
##              Mostly re-written to cope with multiple log partitions.
##		Changed orginal design from log file name to log file locations.
##	18-dec-1998 (hanch04)
##		Clean up output
##	15-mar-99 (rigka01)
##		Bug 95780.  CBF displays incorrect raw log file size due to
##		update of rcp.file.size here instead of rcp.file.kbytes.
##	03-nov-1999 (hanch04)
##		Added TOTAL_LOG_PARTS to set log_file_parts to recalculate
##		log_writer threads
##	19-jun-2000 (hanch04)
##		Need to calculate OTHER_LOG_KBYTES for primary and dual log.
##      17-Nov-2000 (hweho01)
##              For axp_osf platform, translate "." in hostname string to "_"   
##              so it match with the entries in config.dat file   
##      6-Apr-2001 (zhahu02)
##              Bug 104426, Problem INGSRV1423
##              Changed the code for checking the existing raw log file.
##	02-jun-2002 (devjo01)
##		UNIX cluster support. s103715.
##	14-jun-2002 (devjo01)
##		Misc. fixes.  
##		- Correctly check for existing log partitions.
##		- Correctly calculate # of partitions.
##		- Make sure log file name never exceeds 36 chars.
##	3-Jul-2002 (bonro01)
##		Bug 108175.  Fixed message for minimum size of raw log.
##      13-Sep-2002 (xu$we01)
##              The script checks that the raw device is not created in
##              partition which begins on cylinder 0 by checking that
##              the partition name does not end in 'a' or 'c'. This check
##              does not take into account user defined partition names
##              which may end in 'a' or 'c' even if they are not in
##              partition zero.
##              Fixing it by poping up a warning and prompt a choice when
##              detect the partition name end in 'a' or 'c'.
##	04-nov-2002 (somsa01)
##		Modified to run "ls" from "/bin", and where using the -L
##		option for "ls" also use the -o option.
##	26-nov-2002 (somsa01)
##		Use CONFIG_HOST rather than HOSTNAME when performing
##		config.dat searches.
##	19-jun-2003 (drivi01)
##		Added code to check for existence of a raw partition in 
##		config.dat when the raw log file has been deleted manually.
##		mkrawlog script will fail  with an error if it thinks
##		partition already exists and ask user to delete log files
##		from cbf.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##      06-Dec-2004 (hweho01)
##          Fixed the shared lib loading error on Solaris (su4_us5).   
##          Since env. variables with LD_ prefix will not be passed to     
##          the new shell after command su, need to setup it explicitly. 
##          Star #13824827. 
##	9-Dec-2004 (schka24)
##	    Solaris is also su9_us5.  Also, fix more cases of using
##	    config-host instead of host-name for iisetres/iigetres.
##	    (later) back out su9_us5 stuff, that's VERS64; I had my
##	    build garbled, and I fat-fingered the change bringing it
##	    over for submission.  So just remove it.  Retain the
##	    hostname change.
##	26-Jan-2005 (bonro01)
##	    Fixed the shared lib loading error on HP hpb_us5 & hp2_us5
##	    Recent changes to the iiremres, iigetres, iisetres, iimklog
##	    utilities now require run-time access to some shared libraries.
##	26-Apr-2005 (bonro01)
##	    Fixed shared lib loading on Solaris a64.sol
##      20-Feb-2008 (huazh01)
##          1) Fixed shared lib loading on HP-UX i64.hpu.
##          2) Add exit status 15 for 'rcpstat' failure.
##          This fixes bug 119844.
##	11-Mar-2008 (bonro01)
##	    Bug 119844 also applies to platform hp2.us5
##	    Also reported as bug 118250.
##	    When security option SU_DEFAULT_PATH is set, this script
##	    fails to find command ipcclean and csinstall.
##	23-Apr-2009 (frima01)
##	    Add change to TMP_DIR prior to su calls to avoid permission
##	    problems with current directory.
##
#  DEST = utility
# ------------------------------------------------------------------------

. iisysdep
. iishlib

rem_logs()
{
    for NUM_PARTS in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
    do
	if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
	    iiremres ii.$CONFIG_HOST.rcp.log.log_file_${NUM_PARTS}
	    iiremres ii.$CONFIG_HOST.rcp.log.raw_log_file_${NUM_PARTS}
	else
	    iiremres ii.$CONFIG_HOST.rcp.log.dual_log_${NUM_PARTS}
	    iiremres ii.$CONFIG_HOST.rcp.log.raw_dual_log_${NUM_PARTS}
	fi
    done
}

exit_err()
{
   rem_logs
   exit 10
}

#
# Generates "decorated" log name.
#
# $1 = part#
#
gen_log_name()
{
    paddedpart=`echo "0${1}" | sed 's/0\([0-9][0-9]\)/\1/'`
    echo "${FILE_SYMBOL_VALUE}.l${paddedpart}${LOG_EXT}" | cut -c 1-36
}

if [ "$1" = "-dual" -o "$1" = "-DUAL" ]
then
   DUAL_LOG=true
   LOCATION_SYMBOL="II_DUAL_LOG"
   FILE_SYMBOL="II_DUAL_LOG_NAME"
   LOG_PROMPT="A backup transaction log"
else
   DUAL_LOG=false
   LOCATION_SYMBOL="II_LOG_FILE"
   FILE_SYMBOL="II_LOG_FILE_NAME"
   LOG_PROMPT="An Ingres transaction log"
fi

CMD=mkrawlog
export CMD
TMP_DIR=/tmp

# Make sure it is run by root.
if [ "$WHOAMI" != "root" ]
then
   cat << !

You must be logged on as root to run $CMD.

!
   exit 1
fi

# Prompt for login of installation owner
cat << !

Ingres/mkrawlog

In order to use mkrawlog, you must know the login of the owner
of the Ingres installation for which you wish to create a
raw transaction log.

!
iiechonn "Please enter the login of the installation owner: "
read ING_USER junk

# make sure II_LOG_FILE or II_DUAL_LOG is set
LOCATION_SYMBOL_VALUE=`iigetenv II_SYSTEM`
[ -z "$LOCATION_SYMBOL_VALUE" ] &&
{
   cat << !

The symbol II_SYSTEM must be defined.

!
   exit 2
}

# make sure II_LOG_FILE_NAME or II_DUAL_LOG_NAME is set
if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
FILE_SYMBOL_VALUE=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_name`
else
FILE_SYMBOL_VALUE=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_name`
fi

[ -z "$FILE_SYMBOL_VALUE" ] &&
{
   cat << !

The symbol $FILE_SYMBOL must be defined.

!
   exit 2
}

# make sure location directory exists
[ ! -d $LOCATION_SYMBOL_VALUE ] &&
{
   cat << !

The directory specified by $LOCATION_SYMBOL:

	$LOCATION_SYMBOL_VALUE

must exist before you can run $CMD.

!
   exit 3
}

# make sure II_CONFIG is defined
II_CONFIG=`ingprenv II_CONFIG`
[ -z "$II_CONFIG" ] && II_CONFIG=$II_SYSTEM/ingres/files

# create config.dat if it doesn't exist
[ -f $II_CONFIG/config.dat ] ||
{
   touch $II_CONFIG/config.dat 2>/dev/null ||
   {
      cat << !

Unable to create $II_CONFIG/config.dat.  Exiting...

!
      exit 1
   }
   chmod 644 $II_CONFIG/config.dat
   chown $ING_USER $II_CONFIG/config.dat
}

# create ingres directory if necessary
[ ! -d $LOCATION_SYMBOL_VALUE/ingres ] &&
{
   if mkdir $LOCATION_SYMBOL_VALUE/ingres
   then :
      chmod 755 $LOCATION_SYMBOL_VALUE/ingres
      chown $ING_USER $LOCATION_SYMBOL_VALUE/ingres
   else
      cat << !

Unable to create directory: 

	 $LOCATION_SYMBOL_VALUE/ingres

Root must have write access to the file system in order to create a
transaction log file.

!
      exit 3
   fi
}

# create ingres/log directory if necessary
[ ! -d $LOCATION_SYMBOL_VALUE/ingres/log ] &&
{
   if mkdir $LOCATION_SYMBOL_VALUE/ingres/log
   then :
      chmod 700 $LOCATION_SYMBOL_VALUE/ingres/log
      chown $ING_USER $LOCATION_SYMBOL_VALUE/ingres/log
   else
      cat << !

Unable to create directory: 

	 $LOCATION_SYMBOL_VALUE/ingres/log

You must have write access to the file system in order to create a
transaction log file.

!
      exit 3
   fi
}

# If this is a clustered installation, log name needs to be 
# "decorated" with name of node.
LOG_EXT=""
if $CLUSTERSUPPORT
then
    II_CLUSTER="`ingprenv II_CLUSTER`"
    if [ -z "$II_CLUSTER" ] ; then
	while :
	do
	    iiechonn \
"Will this installation be part of an Ingres cluster (y/n) [N] "
	    read answer junk
	    [ -z "$answer" ] && answer='N'

	    case $answer in
	    y|Y)
		ingsetenv 'II_CLUSTER' '1'
		;;
	    n|N)
		ingsetenv 'II_CLUSTER' '0'
		;;
	    *)
		echo ""
		echo "Please enter 'y' or 'n'."
		echo ""
		continue
		;;
	    esac
	    II_CLUSTER="`ingprenv II_CLUSTER`"
	    break
	done
    fi 
    if [ "1" = "$II_CLUSTER" ] ; then
	LOG_EXT=`echo "_$HOSTNAME" | tr '[a-z]' '[A-Z]'`
    fi
fi

# abort if log file already exists
for NUM_PARTS in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
do
    if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
      LOG_FILE_LOCATION=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_${NUM_PARTS}`
    else
      LOG_FILE_LOCATION=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_${NUM_PARTS}`
    fi
    if [ $LOG_FILE_LOCATION ] ; then
	logfilename="`gen_log_name $NUM_PARTS`"

	if [ -f "$LOG_FILE_LOCATION/ingres/log/$logfilename" -o \
	     -c "$LOG_FILE_LOCATION/ingres/log/$logfilename" ]
	then
	   cat << !

$LOG_PROMPT:

	$LOG_FILE_LOCATION/$logfilename

already exists as a location.  You must delete all partitions of a log
using cbf before creating a new one. 

!
	   exit 14 
	fi
    fi
done
for NUM_PARTS in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
do
    if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
	  LOG_FILE_LOCATION=`iigetres ii.$CONFIG_HOST.rcp.log.raw_log_file_${NUM_PARTS}`
    else
	  LOG_FILE_LOCATION=`iigetres ii.$CONFIG_HOST.rcp.log.raw_dual_log_${NUM_PARTS}`
    fi

    if [ "$LOG_FILE_LOCATION" != "" ] 
    then
       cat << !

$LOG_PROMPT:

	$LOG_FILE_LOCATION

already exists as a location.  You must delete all partitions of a log
using cbf before creating a new one.

!
       exit 14
   fi
done
MIN_LOG_KBYTES=16384

cat << !

 -----------------------------------------------------------------------
|									|
|  In order to be used as a raw (unbuffered) Ingres transaction   	|
|  log, a raw device must be a character special file and must contain  |
|  at least $MIN_LOG_KBYTES Kbytes.  The special character raw device you
|  specify must not contain a Unix file system.				| 
|									|
 -----------------------------------------------------------------------

!

if $ACPART_NOT_ALLOWED
then
   cat << !
 -----------------------------------------------------------------------
|			      ***WARNING***				|
|									|
|  Do not create the raw transaction log on a disk partition which	|
|  begins on cylinder 0.  Unix disk partitions which begin on cylinder	|
|  0 are used to identify and access other partitions on the disk. If	|
|  you attempt to create a raw transaction log on a 0 cylinder		| 
|  partition, your disk will become inaccessible.			| 
|									|
 -----------------------------------------------------------------------

!
fi

TOTAL_NUM_PARTS=0
NOT_DONE=1
while [ $TOTAL_NUM_PARTS -le 16 ] && [ $NOT_DONE -eq 1 ]
do

    if [ $TOTAL_NUM_PARTS -gt 0 ] ; then
	iiechonn "Please enter the next path of raw device file or \n return to end: "
	read DEVICE_PATH junk
    fi
    while [ -z "$DEVICE_PATH" ] && [ $TOTAL_NUM_PARTS -eq 0 ]
    do
	iiechonn "Please enter path of raw device file: "
	read DEVICE_PATH junk
    done
    if [ -z "$DEVICE_PATH" ] ; then
	NOT_DONE=0
    else
   if $ACPART_NOT_ALLOWED
   then
      if [ `echo $DEVICE_PATH | grep "a$" | wc -l` -eq 1 -o `echo $DEVICE_PATH \
         | grep "c$" | wc -l` -eq 1 ] 
      then
         cat << !

 -----------------------------------------------------------------------
|                               ***WARNING***                           |
|                                                                       |
|  Please make sure the raw device is not created in partition "a" nor  |
|  partition "c".                                                       |
|                                                                       |
|  If you have already created the raw device in one of these           |
|  partitions contact your system administrator to correct the error.   |
|                                                                       | 
 -----------------------------------------------------------------------

!
         prompt "Are you sure that you want to continue  ?" y || exit_err
      fi
   fi
   TOTAL_NUM_PARTS=`expr $TOTAL_NUM_PARTS + 1`
   if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
      iisetres ii.$CONFIG_HOST.rcp.log.raw_log_file_${TOTAL_NUM_PARTS} $DEVICE_PATH
      iisetres ii.$CONFIG_HOST.rcp.log.log_file_${TOTAL_NUM_PARTS} $LOCATION_SYMBOL_VALUE
      iisetres ii.$CONFIG_HOST.rcp.log.log_file_parts $TOTAL_NUM_PARTS
    else
      iisetres ii.$CONFIG_HOST.rcp.log.raw_dual_log_${TOTAL_NUM_PARTS} $DEVICE_PATH
      iisetres ii.$CONFIG_HOST.rcp.log.dual_log_${TOTAL_NUM_PARTS} $LOCATION_SYMBOL_VALUE
      iisetres ii.$CONFIG_HOST.rcp.log.log_file_parts $TOTAL_NUM_PARTS
   fi

if [ `/bin/ls -lLo $DEVICE_PATH 2>/dev/null | awk '$1 ~ /^[Cc]/' | wc -l` -eq 0 ] || \
   [ ! -c $DEVICE_PATH ]
then
   cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|	$DEVICE_PATH 
|									|
|  is not a character special file.					|
|									|
|  Create a raw device before running mkrawlog.				|
|									|
 -----------------------------------------------------------------------

!
   rem_logs
   exit 4
fi

BPATH=`echo $DEVICE_PATH | sed "$BPATHSED"`

if $DFCMD 2>/dev/null | awk '{print $1}' | grep $BPATH >/tmp/mkraw.$$
then
   cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|	$DEVICE_PATH 
|									|
|  appears as though it may contain the following file system:		|
|									|
!
   echo "|	"
   cat /tmp/mkraw.$$
   cat << !
|									|
|  The raw device serving as the INGRES log file should NOT contain a	|
|  file system.								|
|									|
 -----------------------------------------------------------------------

!
   rm -f /tmp/mkraw.$$
   rem_logs
   exit 5
fi
	rm -f /tmp/mkraw.$$
	fi
done

echo ""
echo "Checking size of raw device..."

NUM_PARTS=1
TOTAL_DEVICE_BLOCKS=0
while [ $NUM_PARTS -le $TOTAL_NUM_PARTS ]
do
    if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
      DEVICE_PATH=`iigetres ii.$CONFIG_HOST.rcp.log.raw_log_file_${NUM_PARTS}`
    else
      DEVICE_PATH=`iigetres ii.$CONFIG_HOST.rcp.log.raw_dual_log_${NUM_PARTS}`
   fi

    dd if=$DEVICE_PATH of=/dev/null bs=32k >/tmp/mkraw.$$ 2>&1
    if fgrep "+" /tmp/mkraw.$$ > /tmp/mkrawout.$$ 2>&1
    then
	if [ $NUM_PARTS -eq 1 ] ; then
	    PREV_DEVICE_BLOCKS=`sed -e 's/+.*//' -e '2,$d' /tmp/mkrawout.$$`
	    TOTAL_DEVICE_BLOCKS=$PREV_DEVICE_BLOCKS
	else
	    CURRENT_DEVICE_BLOCKS=`sed -e 's/+.*//' -e '2,$d' /tmp/mkrawout.$$`
	    if [ $CURRENT_DEVICE_BLOCKS -ne $PREV_DEVICE_BLOCKS ] ; then
		cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
| Error determining size of raw device.					|
|									|
 -----------------------------------------------------------------------

!
		rm -f /tmp/mkraw.$$
		rm -r /tmp/mkrawout.$$
		rem_logs
		exit 6
	    else
		PREV_DEVICE_BLOCKS=$CURRENT_DEVICE_BLOCKS
	 	TOTAL_DEVICE_BLOCKS=`expr $TOTAL_DEVICE_BLOCKS + $CURRENT_DEVICE_BLOCKS`
	    fi
	fi 
	rm -f /tmp/mkraw.$$
	rm -r /tmp/mkrawout.$$
	NUM_PARTS=`expr $NUM_PARTS + 1`
    else
   cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
| Error determining size of raw device.					|
|									|
 -----------------------------------------------------------------------

!
   rm -f /tmp/mkraw.$$
   rm -r /tmp/mkrawout.$$
   rem_logs
   exit 6
fi

DEVICE_KBYTES=`expr $TOTAL_DEVICE_BLOCKS \* 32`
done

# change directory to TMP_DIR, in case the current directory
# isn't accessible for $ING_USER
cd $TMP_DIR

# install shared memory to run rcpstat
# Note: ipcclean now uses iipmhost which needs libcompat.so, so we need
# to make sure that library path is passed.
# csinstall is statically linked, doesn't need the library path.
# Linux passes LD_LIBRARY_PATH thru su, but Solaris doesn't.

if [ "$VERS" = "su4_us5" -o "$VERS" = "a64_sol" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64 ipcclean >/dev/null"
elif [ "$VERS" = "hpb_us5" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 ipcclean >/dev/null"
elif [ "$VERS" = "i64_hpu" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib ipcclean >/dev/null"
else
   su $ING_USER $SU_FLAGS ipcclean
fi

if [ "$VERS" = "hpb_us5" ] ;  then
  su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 csinstall - >/tmp/csinstall.$$"
elif [ "$VERS" = "i64_hpu" ] ;  then
  su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib csinstall - >/tmp/csinstall.$$"
else
  su $ING_USER $SU_FLAGS "csinstall - >/tmp/csinstall.$$" 
fi

status=$?

if [ $status -eq 0 ] ;
then :
else
{
   cat /tmp/csinstall.$$
   rm -f /tmp/csinstall.$$
   cat << !

Unable to install shared memory.  You must correct the problem described
above and re-run $CMD.

!
   rem_logs
   exit 12 
}
fi
rm -f /tmp/csinstall.$$

# construct path to other logfile ( if it exists and is enabled )
if $DUAL_LOG ; then
   II_LOG_FILE=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_1`
   II_LOG_FILE_NAME=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_name`
   if [ "$II_LOG_FILE" -a "$II_LOG_FILE_NAME" ]
   then
      II_LOG_FILE_PATH=${II_LOG_FILE}/ingres/log/${II_LOG_FILE_NAME}.l01
      if [ -f $II_LOG_FILE_PATH -o -c $II_LOG_FILE_PATH ] ; then
         if [ "$VERS" = "hpb_us5" ] ;  then
            su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/bin SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 rcpstat -silent -enable" >/dev/null
         elif [ "$VERS" = "i64_hpu" ] ;  then
            su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/bin LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib rcpstat -silent -enable" >/dev/null
         else
            su $ING_USER $SU_FLAGS "rcpstat -silent -enable" >/dev/null
         fi
      
         status=$?
         if [ $status -eq 0 ] ;
         then
            THIS_LOG="backup"
            OTHER_LOG="Ingres"
            OTHER_LOG_PATH=$II_LOG_FILE_PATH 
         else
            cat << !

rcpstat -silent -enable fails. You must correct the problem described
above and re-run $CMD.

!
            rem_logs
            exit 15
         fi
      fi
   fi
else
   II_DUAL_LOG=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_1`
   II_DUAL_LOG_NAME=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_name`
   if [ "$II_DUAL_LOG" -a "$II_DUAL_LOG_NAME" ]
   then
      II_DUAL_LOG_PATH=${II_DUAL_LOG}/ingres/log/${II_DUAL_LOG_NAME}.l01
      if [ -f $II_DUAL_LOG_PATH -o -c $II_DUAL_LOG_PATH ] ; then
         if [ "$VERS" = "hpb_us5" ] ;  then
            su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/bin SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 rcpstat -silent -enable -dual" >/dev/null
         elif [ "$VERS" = "i64_hpu" ] ;  then
            su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/bin LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib rcpstat -silent -enable -dual" >/dev/null
         else
            su $ING_USER $SU_FLAGS "rcpstat -silent -enable -dual" >/dev/null
         fi

         status=$?
         if [ $status -eq 0 ] ;
         then
            THIS_LOG="Ingres"
            OTHER_LOG="backup"
            OTHER_LOG_PATH=$II_DUAL_LOG_PATH 
         else
            cat << !

rcpstat -silent -enable -dual fails. You must correct the problem described
above and re-run $CMD.

!
            rem_logs
            exit 15
         fi
      fi
   fi
fi

if [ "$VERS" = "su4_us5" -o "$VERS" = "a64_sol" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64 ipcclean >/dev/null"
elif [ "$VERS" = "hpb_us5" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 ipcclean >/dev/null"
elif [ "$VERS" = "i64_hpu" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib ipcclean >/dev/null"
else
   su $ING_USER $SU_FLAGS ipcclean
fi

[ "$OTHER_LOG_PATH" ] &&
{
   # make sure the other log has same of partitions

   NOT_DONE=1
   NUM_PARTS=1
   while [ $NOT_DONE -eq 1 ]
   do
	if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
	    LOG_FILE_LOCATION=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_${NUM_PARTS}`
	else
	    LOG_FILE_LOCATION=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_${NUM_PARTS}`
	fi
	if [ "$LOG_FILE_LOCATION" ] ; then
	    NUM_PARTS=`expr $NUM_PARTS + 1`
	else
	    NOT_DONE=0
	    NUM_PARTS=`expr $NUM_PARTS - 1`
	fi
   done

   if [ $TOTAL_NUM_PARTS -ne $NUM_PARTS ] ; then
      cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|  The $OTHER_LOG transaction log for this installation has $NUM_PARTS partitions.
|									|
|  The $THIS_LOG transaction log has $TOTAL_NUM_PARTS partitions.
|									|
|  The two transaction logs must have the same number of partions.	|
|									|
|  Please adjust the file and/or devices so that they are identical     |
|  and rerun $CMD.							|
|									|
 -----------------------------------------------------------------------

!
      rem_logs
      exit 11
   fi
   # figure out the size of the other log and set the raw log to the same size.

   # if other log is readable, it's not a raw device

   if [ `/bin/ls -l $OTHER_LOG_PATH 2>/dev/null | awk '$1 ~ /^[F-]r/' | \
      wc -l` -eq 1 ]
   then
      OTHER_LOG_BYTES=`eval "/bin/ls -ln $OTHER_LOG_PATH | $AWKBYTES"`
      OTHER_LOG_KBYTES=`expr $OTHER_LOG_BYTES \/ 1024`
      OTHER_LOG_KBYTES=`expr $OTHER_LOG_KBYTES \* $TOTAL_NUM_PARTS`

   #if file is not readable, check whether the file is configured as a raw
   # device

   elif [ `/bin/ls -lLo $OTHER_LOG_PATH 2>/dev/null | awk '$1 ~ /^[Cc]/' | \
	wc -l` -eq 1 ]
   then
      OTHER_LOG_OWNER=`eval "/bin/ls -lLo $OTHER_LOG_PATH | $AWKOWNER"`
      if [ "$ING_USER" != "$OTHER_LOG_OWNER" ]
      then
         echo "$CMD: error: $OTHER_LOG_PATH is not owned by $ING_USER."
         rem_logs
         exit 1
      fi

      echo ""
      echo "Checking $OTHER_LOG (raw) log size..."

      # calculate size
      if dd if=$OTHER_LOG_PATH of=/dev/null bs=32k >/tmp/ckraw.$$ 2>&1
      then :
      else
         rm -f /tmp/ckraw.$$
         echo "$CMD: error determining size of raw device."
         rem_logs
         exit 2
      fi
      DEVICE_BLOCKS=`sed -e 's/+.*//' -e '2,$d' /tmp/ckraw.$$`
      rm -f /tmp/ckraw.$$
      OTHER_LOG_KBYTES=`expr $DEVICE_BLOCKS \* 32`
      OTHER_LOG_KBYTES=`expr $OTHER_LOG_KBYTES \* $TOTAL_NUM_PARTS`
   fi

   echo ""
   echo "Current INGRES log file size is $OTHER_LOG_KBYTES Kbytes"

   if [ $OTHER_LOG_KBYTES -ne $DEVICE_KBYTES ]
   then
      cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|  The $OTHER_LOG transaction log for this installation is $OTHER_LOG_KBYTES Kbytes.
|									|
|  The $THIS_LOG transaction log is $DEVICE_KBYTES Kbytes.
|									|
|  The two transaction logs must be the same size.			|
|									|
|  Please adjust the file and/or device sizes so that they are identical|
|  and rerun $CMD.							|
|									|
 -----------------------------------------------------------------------

!
      rem_logs
      exit 11
   fi
}
# size of raw device file is known 

#
# store size of raw device in a file so other scripts can read it
# to avoid running (very slow) dd command if possible.
#
echo $DEVICE_KBYTES > $LOCATION_SYMBOL_VALUE/ingres/log/rawlog.size
status=$?
if [ $status -eq 0 ]
then
   chown $ING_USER $LOCATION_SYMBOL_VALUE/ingres/log/rawlog.size \
      >/dev/null 2>&1
fi
iisetres ii.$CONFIG_HOST.rcp.file.kbytes $DEVICE_KBYTES

cat << ! 

The raw log device contains $DEVICE_KBYTES Kbytes.
!
if [ "$DEVICE_KBYTES" -lt "$MIN_LOG_KBYTES" ]
then
   cat << !

 -----------------------------------------------------------------------
|			      ***ERROR***				|
|									|
|  The raw device you have specified:					|
|									|
|	$DEVICE_PATH 
|									|
|  does not have enough free space.  To be used as a raw transaction	|
|  log file, the raw you specify must have at least $MIN_LOG_KBYTES bytes
|  free.								|
|									|
 -----------------------------------------------------------------------

!
   rem_logs
   exit 7
fi

#
#  Get the major and minor device numbers of the device specified for the
#  raw log partition.  The numbers appear in the output from ls -l as two
#  decimal digits separated by a comma, and some number, possibly zero, of
#  spaces...
#
NUM_PARTS=1
while [ $NUM_PARTS -le $TOTAL_NUM_PARTS ]
do
    if [ "$LOCATION_SYMBOL" = "II_LOG_FILE" ] ; then
	DEVICE_PATH=`iigetres ii.$CONFIG_HOST.rcp.log.raw_log_file_${NUM_PARTS}`
    else
	DEVICE_PATH=`iigetres ii.$CONFIG_HOST.rcp.log.raw_dual_log_${NUM_PARTS}`
    fi

    logfilename="`gen_log_name $NUM_PARTS`"
    if [ "$USE_SYMBOLIC_LINKS" = "true" ]
    then
	$LINKCMD $DEVICE_PATH $LOCATION_SYMBOL_VALUE/ingres/log/$logfilename
    else
	if [ $NUM_PARTS -lt 10 ] ; then
	set - `/bin/ls -lLo $DEVICE_PATH | sed "$SED_MAJOR_MINOR"`
	mknod $LOCATION_SYMBOL_VALUE/ingres/log/$FILE_SYMBOL_VALUE.l0$NUM_PARTS c $1 $2
	else
	set - `/bin/ls -lLo $DEVICE_PATH | sed "$SED_MAJOR_MINOR"`
	mknod $LOCATION_SYMBOL_VALUE/ingres/log/$FILE_SYMBOL_VALUE.l$NUM_PARTS c $1 $2
	fi
    fi
    status=$?

    if [ $status -eq 0 ]
    then :
    else
	cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|  Error linking $logfilename file to raw device.
|									|
 -----------------------------------------------------------------------

!
      rem_logs
      exit 9
    fi

    chown $ING_USER $LOCATION_SYMBOL_VALUE/ingres/log/$logfilename \
	>/dev/null 2>&1
    chmod 600 $LOCATION_SYMBOL_VALUE/ingres/log/$logfilename
    NUM_PARTS=`expr $NUM_PARTS + 1`
done

if $DUAL_LOG
then
   DUAL_ARG="-dual"
else
   DUAL_ARG=""
fi
export DUAL_ARG

if [ "$VERS" = "su4_us5" -o "$VERS" = "a64_sol" ] ; then
  su $ING_USER $SU_FLAGS "/usr/bin/env LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64 iimklog $DUAL_ARG -erase >/dev/null"
elif [ "$VERS" = "hpb_us5" ] ; then
  su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 iimklog $DUAL_ARG -erase >/dev/null"
elif [ "$VERS" = "i64_hpu" ] ;  then
  su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib iimklog $DUAL_ARG -erase >/dev/null"
else
   su $ING_USER $SU_FLAGS "iimklog $DUAL_ARG -erase >/dev/null"
fi
status=$?
if [ $status -eq 0 ]
then :
else
    cat << !
Unable to erase the transaction log.

!
       exit 13
fi

echo "Formatting raw transaction log..."

if [ "$VERS" = "hpb_us5" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 csinstall -"
elif [ "$VERS" = "i64_hpu" ] ;  then
   su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib csinstall -"
else
   su $ING_USER $SU_FLAGS "csinstall -"
fi

status=$?

if [ $status -eq 0 ] ;
then :
else
{
   cat << !

Unable to install shared memory.

!
   exit 1
}
fi

if $DUAL_LOG ; then
   if rcpconfig -init_dual >/dev/null ; then 
      FORMATTED=true;
   else
      FORMAT_OK=false
   fi
else
   if rcpconfig -init_log >/dev/null ; then
      FORMAT_OK=true
   else 
      FORMAT_OK=false
   fi
fi

if [ "$VERS" = "su4_us5" -o "$VERS" = "a64_sol" ] ;  then
  su $ING_USER $SU_FLAGS "/usr/bin/env LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64 ipcclean >/dev/null"
elif [ "$VERS" = "hpb_us5" ] ;  then
  su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility SHLIB_PATH=$II_SYSTEM/ingres/lib LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64 ipcclean >/dev/null"
elif [ "$VERS" = "i64_hpu" ] ;  then 
  su $ING_USER $SU_FLAGS "/usr/bin/env PATH=$PATH:$II_SYSTEM/ingres/utility LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib ipcclean >/dev/null"
else
  su $ING_USER $SU_FLAGS ipcclean
fi

if $FORMAT_OK ; then
   cat << !

Transaction log formatted successfully.

!
else
   cat << !

Unable to format raw transaction log.

This indicates an error if the INGRES/DBMS has already been set up in
this installation.

!
fi

cat << !
 -----------------------------------------------------------------------
|									|
!
if [ $TOTAL_NUM_PARTS -eq 1 ] ; then
    logfilename="`gen_log_name 1`"
    cat << !
|  The following raw transaction log has been created:			|
|									|
|	$LOCATION_SYMBOL_VALUE/ingres/log/$logfilename
!
else
    cat << !
|  A raw transaction log with $TOTAL_NUM_PARTS parts has been
|  created with components:			|
|									|
!
    NUM_PARTS=1
    while [ $NUM_PARTS -le $TOTAL_NUM_PARTS ]
    do
	logfilename="`gen_log_name $NUM_PARTS`"
	echo "|	$LOCATION_SYMBOL_VALUE/ingres/log/$logfilename"
	NUM_PARTS=`expr $NUM_PARTS + 1`
    done
fi

$FORMAT_OK || 
{
   cat << !
|									|
|  The transaction log needs to be formatted before it can be used.	|
!
}

cat << !
|									|
 -----------------------------------------------------------------------

!

exit 0
