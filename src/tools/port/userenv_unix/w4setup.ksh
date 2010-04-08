#
# Set up your csh environment to build INGRES on UNIX.
# This file is to be included with the `.' command to ksh.
#
# Your local mods should be:
#	1. Copy this to $NFSDISK/setup-ksh (or whatever);
#	2. Make a symbolic link from that to "~ingres/.w4gl3-ksh" for convenience;
#	3. Read it for sanity. Obviously, "platform=xxx" and "NFSDISK="
#	   will need to be customized.)
#
## History
##	13-aug-1993 (jab)
##		Converted from 2.0/02 to 3.0, and checked in.

NFSDISK=/m/corsac/w12
platform=xxx

ING_SRC=$NFSDISK/$platform-w4gl300001/src export ING_SRC
II_RELEASE=$NFSDISK/$platform-w4gl300001/release export II_RELEASE
ING_TST=$NFSDISK/$platform-w4gl200200/test export ING_TST
II_SYSTEM=whatever export II_SYSTEM
ING_BUILD=$NFSDISK/$platform-w4gl300001/build export ING_BUILD
II_PATCH=$NFSDISK/$platform-w4gl300001/patch export II_PATCH
ING_LOG=$NFSDISK/$platform-w4gl300001/logs export ING_LOG 
ING_TMP=$NFSDISK/$platform-w4gl300001/tmp export ING_TMP

# no 'TMPDIR ...'=here! export TMPDIR ...'
echo ING_SRC = $ING_SRC ING_BUILD = $ING_BUILD
echo II_SYSTEM = $II_SYSTEM ING_TST = $ING_TST
echo II_RELEASE = $II_RELEASE

PATH=.:$ING_BUILD/bin:$ING_BUILD/utility:/usr/bin:/usr/local/bin:\
/usr/ucb:\
$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$ING_SRC/bin:\
/usr/bin/X11:/bin:/etc:$HOME/bin
export PATH


MAPPATH=$ING_SRC/client.map export MAPPATH
CDPATH=.:$ING_SRC/cl:$ING_SRC/cl/clf:$ING_SRC/cl/clf/hdr:\
$ING_SRC/cl/hdr/hdr:$ING_SRC/front:$ING_SRC/common:\
$ING_SRC:$ING_SRC/x11-src:$ING_SRC/x11-src/x11r5:\
$ING_SRC/x11-src/motif1.2:$ING_SRC/x11-src/hdr:\
$ING_SRC/x11-src/x11tools:$ING_SRC/tools:\
$ING_SRC/tools/port:$ING_SRC/tools/port/ming:\
$ING_SRC/tools/port/conf:$ING_SRC/w4gl

umask 0
