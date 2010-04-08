#
# Set up your csh environment to build INGRES on UNIX.
# This file is to be included with the `source' command to csh.
#
# Your local mods should be:
#	1. Copy this to $NFSDISK/setup-csh (or whatever);
#	2. Make a symbolic link from that to "~ingres/.w4gl3" for convenience;
#	3. Read it for sanity. Obviously, "platform=xxx" and "NFSDISK="
#	   will need to be customized.)
#
## History
##	13-aug-1993 (jab)
##		Converted from 2.0/02 to 3.0, and checked in.

set NFSDISK=/m/corsac/w12
set platform=xxx

setenv ING_SRC $NFSDISK/$platform-w4gl300001/src
setenv II_RELEASE $NFSDISK/$platform-w4gl300001/release
setenv ING_TST $NFSDISK/$platform-w4gl200200/test 
setenv II_SYSTEM whatever
setenv ING_BUILD $NFSDISK/$platform-w4gl300001/build
setenv II_PATCH $NFSDISK/$platform-w4gl300001/patch
setenv ING_LOG  $NFSDISK/$platform-w4gl300001/logs
setenv ING_TMP $NFSDISK/$platform-w4gl300001/tmp

# no 'setenv TMPDIR ...' here!
echo ING_SRC = $ING_SRC ING_BUILD = $ING_BUILD
echo II_SYSTEM = $II_SYSTEM ING_TST = $ING_TST
echo II_RELEASE = $II_RELEASE

set path = (. $ING_BUILD/{bin,utility} /usr/bin /usr/local/bin \
	/usr/ucb \
	$II_SYSTEM/ingres/{bin,utility} $ING_SRC/bin \
	/usr/bin/X11 /bin /etc $HOME/bin)

if ( -d /usr/local/lang ) set path = (/usr/local/lang $path )
if ( -d /m/hound/usr/local/lang ) set path = (/m/hound/usr/local/lang $path )

setenv MAPPATH $ING_SRC/client.map
set cdpath = (. $ING_SRC/cl $ING_SRC/cl/clf $ING_SRC/cl/clf/hdr \
		$ING_SRC/cl/hdr/hdr $ING_SRC/front $ING_SRC/common \
		$ING_SRC $ING_SRC/x11-src $ING_SRC/x11-src/x11r5 \
		$ING_SRC/x11-src/motif1.2 $ING_SRC/x11-src/hdr \
		$ING_SRC/x11-src/x11tools $ING_SRC/tools \
		$ING_SRC/tools/port $ING_SRC/tools/port/ming \
		$ING_SRC/tools/port/conf $ING_SRC/w4gl)
set filec
set history=200


alias   w4need  'p need | egrep "front\\!hdr\\!hdr|apped|arfom|dbobj|fleas|w4gl\\!|w4gldemo|woof" | egrep -v "cdt|testclasses|wcolit"'
alias   w4look  'egrep "front\\!hdr\\!hdr|apped|arfom|dbobj|fleas|w4gl\\!|w4gldemo|woof" | egrep -v "cdt|testclasses|wcolit"'
alias   x11need  'p need | egrep "motif|x\\!11r5"'
umask 0
