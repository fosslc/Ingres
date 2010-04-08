:
#
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: iidsfree -- echo free blocks on part of a filesystem
#
#  Usage:
#   iidsfree [ directory ]
#
#  Description:
#   If no directory is supplied, default is current directory.
#
#   This version works for either  System V, with output like:
#   /usr/lib/font     (/dev/mp2  ):    21376 blocks    4070 i-nodes
#   /usr/rti          (/dev/mp14 ):    23860 blocks    9842 i-nodes
#   /                 (/dev/mp0  ):     6052 blocks    2231 i-nodes
#
#   or with AIX (IBM RT/PC) output like:
#   Device       Mounted on      total     free  used     ifree  used
#   /dev/hd0     /               17560     4144   76%      2504   16%
#   /dev/hd1     /u               3808     1048   72%       927    9%
#   /dev/hd2     /usr            29308     5008   82%      4005   20%
#
#   or with BSD style output like:
#   Filesystem    kbytes    used   avail capacity  Mounted on
#   /dev/xfd6e    259027  197360   35764    85%    /ug
#   
#   or with Ultrix output like:
#   Filesystem    total    kbytes  kbytes  percent
#   node          kbytes    used    free   used    Mounted on
#   /dev/ra1g      42005   35395    2409    94%    /ug
#
#   or with AIX 3.1 (IBM RS6000) output like:
#   Filesystem    Total KB    free %used   iused %iused Mounted on
#   /dev/hd4         49152   10676   78%    1313    10% /
#   /dev/hd2        278528   35080   87%    8588    12% /usr
#   /dev/hd3         12288   11560    5%      71     1% /tmp
#   /dev/rellv      204800   99104   51%    1224     2% /release
#
#   or with AIX 4.1 (IBM RS6000) output like:
#   Filesystem    512-blocks      Free %Used    Iused %Iused Mounted on
#   /dev/hd4           49152     10676   78%     1313    10% /
#   /dev/hd2          278528     35080   87%     8588    12% /usr
#   /dev/hd3           12288     11560    5%       71     1% /tmp
#   /dev/rellv        204800     99104   51%     1224     2% /release
#
#   or with Linux GNU fileutils 4.0 output like:
#   Filesystem           1k-blocks      Used Available Use% Mounted on
#   /dev/hda1              2179887   1385658    681537  67% /
#
#   We handle both cases dynamically because there are "System V" versions
#   that have the BSD file systems and BSD df output.
#
#  Exit Status:
#   0   succeeded.
#   1   failed.
#
## History:
##   18-jan-93 (tyler)
##      Adapted from diskfree.   
##   26-mar-93 (vijay)
##	 Source iisysdep.
##   28-jan-95 (lawst01)  Bug 65562
##      Simplified kilobyte computation - old method would lead to 
##      overflow errors for file systems with VERY large amounts of
##      free space
##   27-dec-1994 (canor01)
##	 new format for AIX 4.1 'df' command
##   6-jun-95 (hanch04)
##       Bug #69119
##	 added check for /usr/5bin flavor of df in BSD
##   15-jan-96 (toumi01; from 1.1 axp_osf port)
##	 new format for DEC OSF 3.2 'df' command
##   08-feb-96 (toumi01)
##	 Generalize DEC OSF for 3.x and handle the fact that the header
##	 literals after "Filesystem" float based on the lengths of the
##	 file system names.
##   26-jul-96 (muhpa01)
##	 Modified section for DEC OSF (now Digital Unix) to detect the
##	 changed df header for DEC Unix 4.0
##   25-feb-98 (toumi01)
##	Generalize OSF (variable spacing) to also support Linux (lnx_us5)
##   02-jun-99 (toumi01)
##	Add support for the new style of the Linux GNU fileutils 4.0.
##   22-jun-1999 (musro02)
##	Modify in an attempt to cope with nsf mounted file systems
##      (See bug 79518)
##   06-oct-1999 (toumi01)
##	Linux config string for Intel has been changed to int_lnx.
##   25-apr-2000 (somsa01)
##	Updated MING hint for program name to account for different
##	products.
##   01-feb-2000 (marro11)
##	Added entry shared by dg8_us5, dgi_us5, nc4_us5, pym_us5 when using
##	"df -k".
##	Changing format for AIX 4.1 such that it recognizes "df -k" rather
##	than "df", as we're interested in Kbytes, not 512 byte blocks.
##	Corresponding change to DF in mksysdep.sh done at the same time.
##   15-feb-2000 (musro02)
##      Modify in an attempt to cope with nfs mounted file systems
##      (See bug 79518)
##   23-feb-2001 (hweho01)
##      Added entry for AIX 4.3.x, the output is listed in 512 byte  
##      blocks. Note: DF is defined as "df" in iisysdep. 
##   31-Oct-2002 (hanje04)
##	Check for 1K-Blocks as well as 1k-Blocks in the Linux df output.
##	It's changed in later distributions.	
##    14-mar-2003 (hanch04)
##	Bug #109490
##	look for both kbyte or kbytes because some international
##	languages display kbyte.
##    17-Aug-2004 (hanje04)
##	Bug #112840
##	Force language consistency by setting LANG=C before running
##	anything.
##    08-Oct-2004 (toumi01)
##	Bug #112840
##	Force language even harder with LC_ALL=C.
##    06-Apr-2005 (bonro01)
##      Export LANG so that 'df -k' outputs the expected english headings.
##    18-Apr-2005 (hanje04)
##      Add support for Max OS X (mac_osx)).
##      Based on initial changes by utlia01 and monda07.
##    22-Dec-2005 (toumi01)
##	Unset LC_COLLATE in an ongoing effort to force df output in English
##	so that this script can parse it.
##	06-Feb-2008 (hanje04)
##	    SIR S119978
##	    Match for Avail or Available on OSX
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)dsfree
#
#  DEST = utility
#--------------------------------------------------------------------------

(LSBENV)

. (PROG1PRFX)sysdep

LANG=C
LC_ALL=C
LC_COLLATE=
export LANG LC_ALL LC_COLLATE

if [ "$1" = "-f" ] 
then
   filesys=true
   shift
fi

fdir=${1-`pwd`}
# The following line yields a directory in $dir that has
# a mounted file system as a base.
sdir=`pwd`; cd $fdir; dir=`pwd`; cd $sdir
df=/tmp/FREE.$$
$DF > $df

# DEC OSF/1 V3.x or Digital Unix 4.0 style ..
# It looks like BSD, but not quite. It has to be before the BSD check though.
if [ -n "`sed -n '/^Filesystem *1024-blocks        Used       Avail Capacity  Mounted on/p' $df`" ] || [ -n "`sed -n '/^Filesystem *1024-blocks *Used *Available *Capacity *Mounted on/p' $df`" ]
then
   set -$- `$DF $dir | sed /Mounted/d`
   echo $4
   rm -f $df
   exit 0
fi

# Linux GNU fileutils 4.0 style ..
# The GNU fileutils 3.x style is like the Digital Unix style.
if [ -n "`sed -n '/^Filesystem *1[Kk]-blocks *Used *Available *Use% *Mounted on/p' $df`" ]
then
   set -$- `$DF $dir | sed /Mounted/d`
   echo $4
   rm -f $df
   exit 0
fi

# IBM AIX 4.3.x style ..
# The numbers are listed in 512-blocks, DF is defined as "df" in iisysdep.   
if [ -n "`sed -n '/^Filesystem  *512-blocks  *Free %Used  *Iused %Iused Mounted on/p' $df`" ]
then
   set -$- `$DF $dir | sed /Mounted/d`
   echo $3
   rm -f $df
   exit 0
fi

# IBM AIX 3.1 style ..
# It looks like BSD, but not quite. It has to be before the BSD check though.
if [ -n "`sed -n '/^Filesystem    Total KB    free %used   iused %iused Mounted on/p' $df`" ]
then
   set -$- `$DF $dir | sed /Mounted/d`
   echo $3
   rm -f $df
   exit 0
fi

# IBM AIX 4.1 style ..
# It looks like BSD, but not quite. It has to be before the BSD check though.
if [ -n "`sed -n '/^Filesystem  *1024-blocks  *Free %Used  *Iused %Iused Mounted on/p' $df`" ]
then
   set -$- `$DF $dir | sed /Mounted/d`
   echo $3
   rm -f $df
   exit 0
fi

# Data General Motorola, DG Intel, NCR, Pyramid, perhaps others...
#
if [ -n "`sed -n '/^ *[Ff]ilesystem  *[Kk]bytes  *[Uu]sed  *[Aa]vail *[Cc]apacity  *[Mm]ounted on */p' $df`" ]
then
   set -$- `$DF $dir | sed '/ [Mm]ounted on/d'`
   echo $4
   rm -f $df
   exit 0
fi

if [ -n "`sed -n '/^ *[Ff]ilesystem  *[Kk]byte  *[Uu]sed  *[Aa]vail *[Cc]apacity  *[Mm]ounted on */p' $df`" ]
then
   set -$- `$DF $dir | sed '/ [Mm]ounted on/d'`
   echo $4
   rm -f $df
   exit 0
fi

# BSD /usr/5bin style ..
# It looks like BSD, but not quite. It has to be before the BSD check though.
# BSD should use /bin/df.  This is here just in case.
# /usr/5bin/df cannot handle disks larger than 2 gig.
if [ -n "`sed -n '/^Filesystem            bavail/p' $df`" ]
then
   set -$- `$DF $dir | sed /bavail/d`
   BLOCK=`expr $2 / 2`
   if [ $filesys ]
   then
      echo $BLOCK $4
   else
      echo $BLOCK 
   fi
   rm -f $df
   exit 0
fi

# Mac OSX style, pretty much like Linux GNU
if [ -n "`sed -n '/^Filesystem *512-blocks *Used *Avail.* *Capacity *Mounted on/p' $df`" ]
then
   set -$- `$DF $dir | sed /Mounted/d`
   echo $4
   rm -f $df
   exit 0
fi

# BSD style...

if [ -n "`sed -n '/^F/p' $df`" ]
then
   set -$- `$DF $dir | sed /kbytes/d`
   if [ $filesys ]
   then
      echo $4 $6
   else
      echo $4
   fi
   rm -f $df
   exit 0
fi


# System V or AIX style:
#
# loop until backwards to / until you find a file system that looks
# like it's holding the directory.  Should always find root /, but
# echos '0' if there's trouble.
#
if [ -n "`sed -n '/^D/p' $df`" ]
then
   setblock='BLOCK=$4'   # AIX style
else
   setblock='BLOCK=$3'   # Regular ol' SYSV style
fi

IFS="():    "
while :
do
   line=`grep "$dir " $df`
   if [ -z "$line" ]
   then
      case $dir in
         /|.) echo 0; rm -f $df; exit 1;;
         *) dir=`dirname $dir`;;
      esac
   else
      set -$- `echo $line`
#        the following use of firstchar solves bug 79518 concerning
#        nfs mounted file systems (musro02)
      firstchar=`echo $3 | cut -c1,1`
      if [ "$firstchar" = "/" ]; then  #is parm 3 a directory path?
          setblock='BLOCK=$4'          #nfs may cause parm 3 to be a dir path
      fi
      eval $setblock
      break;
   fi
done
 
## lawst01 28-jan-95 Bug 65562  following 2 lines replaces 3 lines below
DIVISOR=`expr 1024 / $BTPERBLK`
BLOCK=`expr $BLOCK / $DIVISOR`

## lawst01 28-jan-95 Bug 65562  removes following 3 lines
##BLOCK=`expr $BLOCK \* $BTPERBLK`
##BLOCK=`expr $BLOCK + 1023`
##BLOCK=`expr $BLOCK / 1024`
## lawst01 28-jan-95 Bug 65562 ---------

if [ $filesys ]
then
   echo $BLOCK $1
else
   echo $BLOCK
fi 

rm -f $df
exit 0
