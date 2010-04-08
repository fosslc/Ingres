:
##Copyright (c) 2004, 2010 Ingres Corporation
#  $Header $
#
#  Name: mkrawarea - set up a raw device as an INGRES area
#
#  Usage:
#	mkrawarea
#
#  Description:
#	Guide user thru procedure for setting up a raw device as
#	an INGRES area.
#
#	This script must be run as 'root'.
#
#	The raw disk should just be any disk device that does not have a
#	unix file system on it (ie. /dev/rxy0d).  We will then create a 
#	hard link from the <area_path> to the disk device.
#   
#	We also check that the raw device is not created in partition "a" or "c".
#	These partitions are reserved to access a disk and raw devices should
#	not be created there.
#   
#	After everything is ok the needed subdirectories are created as
#	needed and a hard link is created to the device as
#	    <area_path>/ingres/data/default/iirawdevice
#	and the number of blocks in the device stored in
#	    <area_path>/ingres/data/default/iirawblocks
#
#	Note that any changes made to this path construction will
#	impact or break LOingpath. DM2F is sensitive to the selection of
#	filenames and the content of "iirawblocks". For those reasons
#	the equivalent defined DM2F_RAW_??? values are used in here
#	as variable names as a reminder that if either changes, so
#	must the other. 
#
#	See back/dmf/hdr/dm2f.h
#
# Exit status:
#	0	OK
#	1	must be root to run	
#	2	required symbol not defined
#	3	<area_path> does not exist
#	4	path not a character special device
#	5	device contains file system
#	6	can't determine size of device
#	7	device too small for file
#	8	/etc/mknod of file failed
#	9	cannot use partition a or c to create a raw device
#	10	the area directory contains files and cannot be used
#
## History
##	19-Mar-2001 (jenjo02)
##	    Sculpted from mkrawlog.sh
##	09-May-2001 (jenjo02)
##	    Rewrote such that user must supply <area_path> rather
##	    than using $II_SYSTEM/ingres/data/raw/<device_name>
##	    construct. This makes the raw area creation process 
##	    identical to that used for cooked areas and locations.
##	18-Sep-2001 (jenjo02)
##	    Renamed from MKRAWARE to MKRAWAREA
##	19-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  DEST = utility
# ------------------------------------------------------------------------
(LSBENV)

. iisysdep

DM2F_RAW_KBLKSIZE=64
DM2F_RAW_MINBLKS=64
DM2F_RAW_LINKNAME="iirawdevice"
DM2F_RAW_BLKSNAME="iirawblocks"

CMD=mkrawarea
export CMD

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

Ingres/$CMD

In order to use $CMD, you must know the login of the owner
of the Ingres installation for which you wish to create a
raw area.

!
while [ -z "$ING_USER" ]
do
    iiechonn "Please enter the login of the installation owner: "
    read ING_USER junk
done

cat << !

In order to be used as a raw (unbuffered) Ingres area,
a raw device must be a character special file and must contain
at least $DM2F_RAW_MINBLKS ${DM2F_RAW_KBLKSIZE}k blocks.
The special character raw device you specify must not contain
a Unix file system.

!

if $ACPART_NOT_ALLOWED
then
   cat << !
 -----------------------------------------------------------------------
|			      ***WARNING***				|
|									|
|  Do not create the raw area on a disk partition which			|
|  begins on cylinder 0.  Unix disk partitions which begin on cylinder	|
|  0 are used to identify and access other partitions on the disk. If	|
|  you attempt to create a raw file on a 0 cylinder			| 
|  partition, your disk will become inaccessible.			| 
|									|
 -----------------------------------------------------------------------

!
fi

# Prompt for raw device path
while [ -z "$DEVICE_PATH" ]
do
    iiechonn "Please enter path of raw device: "
    read DEVICE_PATH junk
done

if [ -n "$DEVICE_PATH" ] && $ACPART_NOT_ALLOWED
then
  if [ `echo $DEVICE_PATH | grep "a$" | wc -l` -eq 1 -o `echo $DEVICE_PATH \
     | grep "c$" | wc -l` -eq 1 ] 
  then
     cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|  You cannot create a raw area in partition "a" or partition "c".	|
|									|
|  You should create the raw area in another partition. If you have	|
|  already created the raw area in one of these partitions contact	|
|  your system administrator to correct the error.			|
|									|
 -----------------------------------------------------------------------

!
         exit 9
    fi
fi

if [ `ls -lL $DEVICE_PATH 2>/dev/null | awk '$1 ~ /^[Cc]/' | wc -l` -eq 0 ] || \
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
|  Create a raw device before running $CMD.				|
|									|
 -----------------------------------------------------------------------

!
   exit 4
fi

BPATH=`echo $DEVICE_PATH | sed "$BPATHSED"`

if $DFCMD 2>/dev/null | awk '{print $1}' | grep $BPATH >/tmp/$CMD.$$
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
   cat /tmp/$CMD.$$
   cat << !
|									|
|  The raw device serving as an INGRES area must NOT contain a		|
|  file system.								|
|									|
 -----------------------------------------------------------------------

!
   rm -f /tmp/$CMD.$$
   exit 5
fi

rm -f /tmp/$CMD.$$

# Prompt for area directory
cat << !

You must supply the root path of the area to which this raw device will
be located, for example,

	/otherplace/new_area

The requisite subdirectories /ingres/data/default will be created if
they do not exist.
!

while [ -z "$AREA_PATH" ]
do
    iiechonn "Please enter the root path of area: "
    read AREA_PATH junk
done

# Make sure area directory exists
[ ! -d $AREA_PATH ] &&
{
   cat << !

The directory

	$AREA_PATH

must exist before you can run $CMD.

!
   exit 3
}


# create ingres subdirectory if necessary
FULL_PATH=$AREA_PATH/ingres
[ ! -d $FULL_PATH ] &&
{
   if mkdir $FULL_PATH
   then :
      chmod 755 $FULL_PATH
      chown $ING_USER $FULL_PATH
   else
      cat << !

Unable to create directory: 

	 $FULL_PATH

Root must have write access to the file system in order to create a
raw area.

!
      exit 3
   fi
}

# create data subdirectory if necessary
FULL_PATH=$FULL_PATH/data
[ ! -d $FULL_PATH ] &&
{
   if mkdir $FULL_PATH
   then :
      chmod 700 $FULL_PATH
      chown $ING_USER $FULL_PATH
   else
      cat << !

Unable to create directory: 

	 $FULL_PATH

Root must have write access to the file system in order to create a
raw area.

!
      exit 3
   fi
}

# create default subdirectory if necessary
FULL_PATH=$FULL_PATH/default
[ ! -d $FULL_PATH ] &&
{
   if mkdir $FULL_PATH
   then :
      chmod 777 $FULL_PATH
      chown $ING_USER $FULL_PATH
   else
      cat << !

Unable to create directory: 

	 $FULL_PATH

Root must have write access to the file system in order to create a
raw area.

!
      exit 3
   fi
}

# The default directory must be empty
[ `ls $FULL_PATH 2>/dev/null | wc -l` -ne 0 ] &&
{
    cat << !

The area directory

	$FULL_PATH

contains one or more files. To be used as a raw area, the
directory must be empty.

!
    exit 10
}

echo ""
echo "Checking size of raw device $DEVICE_PATH..."

dd if=$DEVICE_PATH of=/dev/null bs=${DM2F_RAW_KBLKSIZE}k >/tmp/$CMD.$$ 2>&1

if fgrep "+" /tmp/$CMD.$$ > /tmp/${CMD}out.$$ 2>&1
then
    TOTAL_DEVICE_BLOCKS=`sed -e 's/+.*//' -e '2,$d' /tmp/${CMD}out.$$`
    rm -f /tmp/${CMD}\*.$$
else
cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
| Error determining size of raw device.					|
|									|
 -----------------------------------------------------------------------

!
   rm -f /tmp/${CMD}*.$$
   exit 6
fi

cat << ! 

The raw device contains $TOTAL_DEVICE_BLOCKS ${DM2F_RAW_KBLKSIZE}k blocks
!
if [ "$TOTAL_DEVICE_BLOCKS" -lt "$DM2F_RAW_MINBLKS" ]
then
   cat << !

 -----------------------------------------------------------------------
|			      ***ERROR***				|
|									|
|  The raw device you have specified:					|
|									|
|	$DEVICE_PATH 
|									|
|  does not have enough free space.  To be used as a raw area,		|
|  the raw device you specify must have at least 			|
|  $DM2F_RAW_MINBLKS ${DM2F_RAW_KBLKSIZE}k blocks available.
|									|
 -----------------------------------------------------------------------

!
   exit 7
fi

#
#  Get the major and minor device numbers of the device specified for the
#  raw log partition.  The numbers appear in the output from ls -l as two
#  decimal digits separated by a comma, and some number, possibly zero, of
#  spaces...
#
#  and make a symbolic file link to the raw device in the form
#
#    $FULL_PATH/iirawdevice
#
#  and store the total number of blocks as a space-terminated 
#  character string in 
#
#    $FULL_PATH/iirawblocks
#

if [ "$USE_SYMBOLIC_LINKS" = "true" ]
then
    $LINKCMD $DEVICE_PATH $FULL_PATH/$DM2F_RAW_LINKNAME
else
    set - `ls -lL $DEVICE_PATH | sed "$SED_MAJOR_MINOR"`
    mknod $FULL_PATH/$DM2F_RAW_LINKNAME c $1 $2
fi

status=$?

if [ $status -eq 0 ]
then
    chown $ING_USER $FULL_PATH/$DM2F_RAW_LINKNAME >/dev/null 2>&1
    chmod 600 $FULL_PATH/$DM2F_RAW_LINKNAME

    # character string file must be no less than 16 bytes
    echo "$TOTAL_DEVICE_BLOCKS                " > \
      $FULL_PATH/$DM2F_RAW_BLKSNAME

    chown $ING_USER $FULL_PATH/$DM2F_RAW_BLKSNAME >/dev/null 2>&1
    chmod 600 $FULL_PATH/$DM2F_RAW_BLKSNAME
else
    cat << !

 -----------------------------------------------------------------------
|				***ERROR***				|
|									|
|  Error linking							|
|	$FULL_PATH/$DM2F_RAW_LINKNAME 
|  file to raw device.							|
|									|
 -----------------------------------------------------------------------

!
      exit 8
fi


cat << !

The following Ingres special files have been created:

   $FULL_PATH/$DM2F_RAW_LINKNAME
   $FULL_PATH/$DM2F_RAW_BLKSNAME
 
The "area name" to use with ACCESSDB or CREATE LOCATION is
 
   $AREA_PATH

!

exit 0
