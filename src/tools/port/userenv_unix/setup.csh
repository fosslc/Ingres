#
# Set up your csh environment to build INGRES on UNIX.
# This file is to be included with the `source' command to csh.
#
## History
##	24-nov-1992 (lauraw)
##		Converted from 65.c5; started History.
##		Removed ING_BUILD from MANPATH, added MAPPATH and TERM_INGRES.
##	02-dec-1992 (lauraw)
##		Combined setup.c5 and setup.c42 into setup.csh.
##	02-dec-1992 (lauraw)
##		Removed test environment setup. This will be handled by 
##		setup scripts delivered with the Test Kit.
##	18-jan-1996 (matbe01)
##		Added LPATH variable for HP9 environment with a C compiler at
##		release 3.25.  Windowview link edit in wview/xtermr4 can't
##		find the -lXaw etc. shared libraries otherwise.  The SHLIB_PATH
##		variable does not work despite what is noted in Mingbase.mk
##		(see comments in the if defined(hp8_us5)).
##	29-jul-1997 (walro03)
##		Added comments for II_CLPOLL_TRACE for tracing clpoll.c.

# These may change on different machines.
setenv ING_SRC		/src/65/01
setenv II_SYSTEM	/install/65/01
setenv ING_TST		/tst/65/01

setenv ING_BUILD	$II_SYSTEM/ingres
setenv MAPPATH		$ING_SRC/client.map

echo ING_SRC=$ING_SRC
echo ING_BUILD=$ING_BUILD
echo ING_TST=$ING_TST

# Put $ING_SRC first in MANPATH:
setenv MANPATH "${ING_SRC}:/usr/local/man:/usr/man"

# These are needed to run INGRES:
setenv ING_EDIT		/usr/bin/vi 	# on BSD change this to /usr/ucb/vi
setenv TERM_INGRES	vt100f

# Some variables that you may want to turn on during debugging.

# This forces use of non-delivered local .mnx files
# (useful if fast*.mnx and slow*.mnx not built).
#setenv II_MSG_TEST	true

# Comm Server
#setenv II_GCC_LOG		$ING_BUILD/gcc

# GCA/CL tracing
#setenv II_GC_LOG	$ING_BUILD/gc
#setenv II_GC_TRACE	10		# 10 is highest level of trace
#setenv II_CLPOLL_TRACE	2		# tracing clpoll.c

# protocols
#setenv II_GCC_TCP_LOG		on	# TCP TL logging
#setenv II_GCC_TCP_IP		<port#>	# non-default TCP listening point
#setenv II_GCC_DECNET_LOG	on	# DECnet TL logging
#setenv II_GCC_DECNET		<port#>	# non-default DECnet listeing port

# admin.
setenv II_DBMS_LOG	$ING_BUILD/server
setenv II_SLAVE_LOG	$ING_BUILD/slave
#setenv II_LGLK_LOG	true

# Special variables for vec31
setenv PPATH		$ING_SRC/vec31
setenv VEC		$PPATH/usr/vec

# Special shared library variable for hp9 systems - the default libraries
# must be included otherwise they are ignored once this variable is set.
setenv LPATH		/lib:/usr/lib:/usr/lib/X11R5:/usr/lib/X11R4

set path=(\
.\
$ING_BUILD/bin\
$ING_BUILD/utility\
~/bin\
/usr/local/bin\
/usr/lbin\
$ING_SRC/bin\
$PPATH/bin\
/usr/new\
/usr/ucb\
/bin\
/usr/bin\
/etc\
)

set cdpath=(\
. \
.. \
$ING_SRC \
$ING_SRC/admin\
$ING_SRC/admin/abfdemo\
$ING_SRC/admin/install\
$ING_SRC/back\
$ING_SRC/cl\
$ING_SRC/cl/clf\
$ING_SRC/cl/hdr\
$ING_SRC/common\
$ING_SRC/common/adf\
$ING_SRC/common/gcf\
$ING_SRC/common/hdr\
$ING_SRC/dbutil\
$ING_SRC/dbutil/duf\
$ING_SRC/front\
$ING_SRC/tech\
$ING_SRC/tech/info\
$ING_SRC/tools\
$ING_SRC/tools/port\
$ING_SRC/testtool\
$ING_SRC/testtool/sep\
$ING_BUILD\
)
# This emulates "export CDPATH" in csh. It may not work on System V:
setenv newcdpath "$cdpath"
