#
# Set up your ksh or sh environment to build INGRES on UNIX.
# This file is to be included with the `.' command to ksh or sh.
#
## History
##	24-nov-1992 (lauraw)
##		Converted from 65k.k5; started History.
##		Removed ING_BUILD from MANPATH, added MAPPATH and TERM_INGRES.
##	02-dec-1992 (lauraw)
##		Combined setup.k5 and setup.k42 into setup.ksh.
##	02-dec-1992 (lauraw)
##		Removed test environment setup. This will be handled by 
##		setup scripts delivered with the Test Kit.
##      18-jan-1996 (matbe01)
##              Added LPATH variable for HP9 environment with a C compiler at
##              release 3.25.  Windowview link edit in wview/xtermr4 can't
##              find the -lXaw etc. shared libraries otherwise.  The SHLIB_PATH
##              variable does not work despite what is noted in Mingbase.mk
##              (see comments in the if defined(hp8_us5)).
##	29-jul-1997 (walro03)
##		Added comments for II_CLPOLL_TRACE for tracing clpoll.c.

# These may change on different machines.
ING_SRC=/src/65/01;		export ING_SRC
II_SYSTEM=/install/65/01;	export II_SYSTEM
ING_TST=/tst/65/01;		export ING_TST

ING_BUILD=$II_SYSTEM/ingres;	export ING_BUILD
MAPPATH=$ING_SRC/client.map;	export MAPPATH

echo ING_SRC=$ING_SRC
echo ING_BUILD=$ING_BUILD
echo ING_TST=$ING_TST

# Put $ING_SRC first in MANPATH:
MANPATH="${ING_SRC}:/usr/local/man:/usr/man";	export MANPATH
 
# These are needed to run INGRES (for BSD, change /usr/bin/vi to /usr/ucb/vi):
ING_EDIT=/usr/bin/vi;				export ING_EDIT 
TERM_INGRES=vt100f;				export TERM_INGRES

# Some variables that you may want to turn on during debugging.

# This forces use of non-delivered local .mnx files
# (useful if fast*.mnx and slow*.mnx not built).
#II_MSG_TEST=true;			export II_MSG_TEST

# Comm Server
#II_GCC_LOG=$ING_BUILD/gcc;	export II_GCC_LOG

# GCA/CL tracing
#II_GC_LOG=$ING_BUILD/gc;	export II_GC_LOG
#II_GC_TRACE=10;		export II_GC_TRACE
				# 10 is highest level of trace
#II_CLPOLL_TRACE=2;		export II_CLPOLL_TRACE
				# tracing clpoll.c

# protocols
#II_GCC_TCP_LOG=on;		export II_GCC_TCP_LOG	# TCP TL logging
#II_GCC_TCP_IP=<port#>;		export II_GCC_TCP_IP
				# non-default TCP listening point
#II_GCC_DECNET_LOG=on;		export II_GCC_DECNET_LO	# DECnet TL logging
#II_GCC_DECNET=<port#>;		export II_GCC_DECNET
				# non-default DECnet listeing port

# admin.
II_DBMS_LOG=$ING_BUILD/server;	export II_DBMS_LOG
II_SLAVE_LOG=$ING_BUILD/slave;	export II_SLAVE_LOG
#II_LGLK_LOG=true;		export II_LGLK_LOG

# Special variables for vec31
PPATH=$ING_SRC/vec31;		export PPATH
VEC=$PPATH/usr/vec;		export VEC

# Special shared library variable for hp9 systems - the default libraries
# must be included otherwise they are ignored once this variable is set.
LPATH=/lib:/usr/lib:/usr/lib/X11R5:/usr/lib/X11R4;	export LPATH

PATH="\
:.\
:$ING_BUILD/bin\
:$ING_BUILD/utility\
:~/bin\
:/usr/local/bin\
:/usr/lbin\
:$ING_SRC/bin\
:$PPATH/bin\
:/usr/new\
:/usr/ucb\
:/bin\
:/usr/bin\
:/etc\
";	export PATH

CDPATH="\
:.\
:..\
:$ING_SRC\
:$ING_SRC/admin\
:$ING_SRC/admin/abfdemo\
:$ING_SRC/admin/install\
:$ING_SRC/back\
:$ING_SRC/cl\
:$ING_SRC/cl/clf\
:$ING_SRC/cl/hdr\
:$ING_SRC/common\
:$ING_SRC/common/adf\
:$ING_SRC/common/gcf\
:$ING_SRC/common/hdr\
:$ING_SRC/dbutil\
:$ING_SRC/dbutil/duf\
:$ING_SRC/front\
:$ING_SRC/tech\
:$ING_SRC/tech/info\
:$ING_SRC/tools\
:$ING_SRC/tools/port\
:$ING_SRC/testtool\
:$ING_SRC/testtool/sep\
:$ING_BUILD\
";	export CDPATH
