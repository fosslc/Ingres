:
#
# Copyright (c) 2006 Ingres Corporation
#
# This script builds UNIX versions of `clsecret.h,' a file containing
# all configuration dependent definitions that are visible only to
# the CL.  It extracts information that used to be kept in the
# globally visible bzarch.h.  The earlier bzarch.h was written by mkhdr.
# The new one is written by mkbzarch.
#
# The "clsecret.h" file written here is included by "cl/cl/hdr/clconfig.h",
# which contains documentation about what each of the defines here means.
#
# Definitions like "BSD" or "WECOV" or "SYSV" or "SYS5" are NOT ALLOWED.
# You must define a specific capability and create a test for it.
#
# BE SURE TO UPDATE CLCONFIG.H IF ANYTHING IS ADDED TO THIS PROGRAM.
#
# History:
#	10-oct-88 (daveb)
#		Created from mkhdr.sh
#	25-Mar-89 (GordonW)
#		Added xCL_040_EXOS_EXISTS (Excelan TCP/IP), 
#		xCL_041_DECNET_EXISTS, xCL_042_TLI_EXISTS
#		xCL_043_SYSV_POLL_EXISTS
#	28-mar-89 (russ)
#		Added xCL_044_SETREUID_EXISTS.
#	4-apr-89 (markd)
#	        Hard-coded Sequent Sys.v defines
#       06-apr-89 (russ)
#               Moving xCL_036_VTIMES_H_EXISTS to bzarch.h because
#               it is referenced in cl/hdr/hdr/tm.h.
#	13-apr-89 (russ)
#		Added xCL_045_LDFCN_H_EXISTS.
#	13-Apr-1989 (fredv)
#		Excluded the testing of the existence of fd_set in types.h
#			for rtp_us5 because fd_set is defined there but is
#			ifdef-ed by a false condition.
#	08-may-89 (russ)
#		Set LIBC correctly for odt_us5 using the AT&T COFF C compiler.
#		Add a new variable, $pre, to allow for customization of
#		the grep for symbols in the nm output.  Also remove some
#		obsolete odt_us5 exos "junk".
#	12-may-89 (markd)
#		Add have mkdir and rmdir for sqs_us5.  These will be pulled 
#		in from bsd.	
# 	1-jun-89 (markd)
#		In sqs_us5 pulling in sigvec and iitimer from bsd 
#		universe too. 
#	5-jun-89 (daveb)
#		Add mathsigs.
#	13-jun-89 (seng)
#		The PS/2 has no leading underscore in archives.
# 	15-jun-89 (markd)
#		Pulled in fsync from bsd for att universe for sequent 
#	18-jul-89 (daveb)
#		Check for SETDTABLESIZE, set BMAP_CACHE and DIRECT_IO.
#	31-jul-1989 (boba)
#		Change conf to tools/port$noise/conf.
#	14-Aug-1989 (fredv)
#		Added the shadow passwd case to for RT.
#		FNDELAY is defined within a never never reach
#		ifdef-endif in fcntl.h of RT, added the case for RT
#		not to grep FNDELAY from fcntl.h.
#	15-Aug-1989 (kimman)
#		put space between "rtp_us5"] to be "rtp_us5" ]
#	23-aug-89 (daveb)
#		Fix typos in sequent special feature defines.
#     22-aug-89 (russ)
#             Don't define xCL_001_WAIT_UNION on odt_us5.
#     28-aug-89 (russ)
#             Add xCL_065_SHADOW_PASSWD for odt_us5.
#	18-sep-89 (pholman)
#		Check in '/bin' for lpr too !
#	27-Oct-89 (rexl)
#		Added cases for 3bx_us5.
#	27-oct-89 (terryr)
#		modified TERMIO_EXISTS to be generated only if sgtty.h
#		does not, we want to use sgtty.h whereever possible
#		as it avoids INGRES - ksh compatibility problems
#	7-nov-89 (blaise)
#		Changed the 'pre' variable for for dr3, dr4, bul to just
#		"any number of _'s", to stop _bufsync being matched to fsync.
#		Don't define xCL_001_WAIT_UNION for bul_us5
#	14-nov-89 (blaise)
#		Added xCL_067_GETCWD_EXISTS for those system V machines
#		which have getcwd instead of getwd.
#	14-nov-89 (blaise)
#		Added xCL_042_TLI_EXISTS and xCL_043_SYSV_POLL_EXISTS, and 
#		prevented xCL_020_SELECT_EXISTS from being defined, for
#		bul_us5
#	24-nov-89 (swm)
#		Made TYPESIG grep <sys/signal.h> as well as <signal.h>.
#		Added
#			xCL_068_USE_SIGACTION
#			xCL_069_UNISTD_H_EXISTS
#			xCL_070_LIMITS_H_EXISTS
#		by default, xCL_068_USE_SIGACTION will not be defined if
#		xCL_011_USE_SIGVEC is; these two defines are mutually exclusive.
#		Added tests for /usr/bin/lp and /bin/lp line printer commands.
#		Added dr6_us5 definations for LIBC and pre strings.
#	24-nov-89 (swm)
#		Finger trouble in last change (TYPESIG). Made second grep
#		file /usr/include/sys/signal.h (missing "sys/" put in).
#	29-nov-89 (swm)
#		Fixed SIGACTION probing.
#	14-Dec-89 (GordonW)
#		Cleaned up the configuration for "select/poll". The default is
#		Select="yes", and Poll="no"
#		Also added UNIX domain sockets, setpriority, rename, and
#		getrlimit setups.
#	15-Dec-89 (GordonW)
#		Added xCL_074_WRITEV_READV_EXISTS for systems that support 
#		"readv/writev" system calls.
#	20-dec-1989 (boba)
#		Add comments requesting that special libc scan override
#		#define's be comment with name of corresponding library.
#	28-dec-89 (rog)
#		Added xCL_075_SVS_V_IPC_EXISTS which tests for sys/ipc.h
#		and sys/sem.h so we can include them.
#	11-Jan-90 (GordonW)
#		Added back the "bul_us5" setup which i removed.
#		The following is the logic for determining 'select' or 'poll'
#		on machines where they both exists.
#		The default case is whatever the script finds first it uses,
#		if "select=any" is true. If you wnat to force the usage of a
#		certaiin type set the "select=" to that type.
#		Also, TLI was beening testing by looking for "t_open" within
#		libc.a which is wrong. Now look for "/usr/lib/libnsl_s.a"
#		which is the standard TLI user library.
#	16-jan-90 (blaise)
#		Add the capability to force the usage of sigvec, sigaction
#		or neither, by setting the variable Sigvec. For bul_us5,
#		remove the forced #define of 042_TLI_EXISTS following gordonw's
#		addition of a better test for this; and add "Select=poll"
#		to force the use of poll rather than select.
#	16-jan-90 (blaise)
#		gettimeofday() does not set the zone field in some (or
#		maybe all) system V machines. Therefore add a test to 
#		check whether gettimeofday is working as it should before
#		defining xCL_005_GETTIMEOFDAY_EXISTS.
#	05-feb-1990 (swm)
#		Set Select and Signal variables for dr6_us5. Also specified
#		xCL_065_SHADOW_PASSWD, xCL_073_CAN_POLL_FIFOS defines.
#		Extended test for xCL_001_WAIT_UNION to grep for "union" in
#		the <sys/wait.h> file.
#		Extended test for xCL_014_FD_SET_TYPE_EXISTS to grep
#		<sys/select.h> (as well as <sys/types.h>).
#		Added tests to check for xCL_071_UCONTEXT_H_EXISTS and
#		xCL_072_SIGINFO_H_EXISTS.
#	06-feb-1990 (swm)
#		Added automatic test for xCL_076_USE_UNAME. This is defined
#		only if gethostname() is not present and uname() is present.
#	07-feb-90 (swm)
#		Corrected xCL_075_SVS_V_IPC_EXISTS to  xCL_075_SYS_V_IPC_EXISTS
#				   ^				^
#	12-feb-90 (twong)
#		add in new define (xCL_077_BSD_MMAP) for mmap.
#		add in new define (xCL_078_CVX_SEM) for Convex semaphore
#		routines.  Also, change make tzone to cc -o tzone tzone.c;
#		causes problems on Convex.
#	14-feb-90 (swm)
#		Added automatic tests for xCL_079_SYSCONF_EXISTS and
#		xCL_080_GETPAGESIZE_EXISTS.
#		Extended test for xCL_034_GETRLIMIT_EXISTS to check for
#		RLIMIT_RSS as this does not exist under System Release 4;
#		At present this appears to be the only limit used with
#		the CL.
#		Dont use xCL_077_BSD_MMAP on dr6_us5.
#	13-mar-1990 (boba)
#		Fix broken test for xCL_014_FD_SET_TYPE_EXISTS.
#		Can't assume /usr/include/sys/select.h exists.
#	12-apr-1990 (boba)
#		Fix symbol names in some comments.  Makes it easier to
#		compare xCL_'s in mksecret.sh and clconfig.h.
#	06-mar-90 (twong)
#		change tests for xCL_077_BSD_MMAP and xCL_078_CVX_SEM to
#		check for existence of libc routines used in shared
#		memory and semaphore routines.
#	07-mar-90 (swm)
#		Corrected gethostname()/uname() logic to conform to
#		intended behaviour; no test for xCL_076_USE_UNAME is
#		done if gethostname() exists.
#	12-apr-1990 (kimman)
#		Integrating in some fixes for BSD_MMAP and USE_UNAME
#		logic by twong and swm.
#	17-may-90 (blaise)
#		Added xCL_082_SYS_V_STREAMS_IPC, to define whether the 
#		streams driver should be used for IPC. This symbol
#		integrated from 62, but with 082 instead of 077.
#	23-may-1990 (rog)
#		Changed the grep for fd_set: if select.h doesn't exist,
#		the grep exits with an error even if fd_set is in types.h.
#	31-may-90 (blaise)
#		Removed obsolete xCL_037_SYSV_DIRS, which is never used.
#       30-aug-90 (jillb--DGC)
#	        Added case section for establishment of TYPESIG (void or
#               int). Some boxes may get the wrong thing defined by automated
#               procedure which keys on '*signal' even though using 'sigvec'
#               (e.g., ps2_us5 and dg8_us5).  This is an integration from
#               6202p to 63p per (rudyw 27-feb-90 in mksecret.sh).
#		The libc.a functions added by fpang for the 6202p port
#		do NOT need to be added for the 63p port.
#	30-oct-90 (rog)
#		Don't define xCL_002_SEMUN_EXISTS or xCL_070_LIMITS_H_EXISTS
#		for hp3_us5.
#	08-jan-91 (sandyd)
#		Added cases for sqs_ptx.  This was reverse-engineered from the
#		manually maintained clsecret.h that the DBMS group had set up
#		for sqs_ptx.  (Because of that, I am unable to supply the
#		requested comments about which libraries contain which 
#		functions.  All I know is the desired end result for clsecret.h;
#		I don't know all the why's.)  I also had to hack in special
#		cases for xCL_015_SYS_TIME_H_EXISTS, and for
#		xCL_019_TCPSOCKETS_EXIST, since the automated checks would
#		define those but jkb tells me they should NOT be defined.
#	12-feb-91 (mikem)
#		Added define for xCL_081_SUN_FAST_OSYNC_EXISTS, which indicates
#	        that sun's version of fast OSYNC exists (ie. only does one write
#		where possible).  
#		Also added define of xCL_083_USE_SUN_DBE_INCREASED_FDS", which
#		indicates that SUN's DBE extensions may be supported.  Code 
#		ifdef'd on this does not require that DBE be installed, it
#		handles expected errors when DBE is not installed.
#	12-mar-91 (seng)
#		Specific changes for RS/6000.
#		Need to force xCL_001_WAIT_UNION, union structure is defined
#		  in another include file, not wait.h.
#		Take out select structure references.  The RS/6000 allows
#		  the select call to use either the AIX or BSD flavor of
#		  argument.  We use the BSD flavor.
#		Add xCL_065_SHADOW_PASSWD for RS/6000.
#       18-mar-91 (szeng)
#               Integrated 6.3 changed for xCL_017_UNSIGNED_IOBUF_PTR on
#               vax_ulx.
#       20-mar-91 (dchan)
#               Integrated 6.3 changed for xCL_017_UNSIGNED_IOBUF_PTR on
#               ds3_ulx.
#	11-mar-91 (rudyw)
#		Set 'sigvec' so odt_us5 avoids defining xCL_068_USE_SIGACTION
##		due to 'float exception' problem which exists when used.
#		Set up case to allow control over TLI detection and use.
#   21-jun-91 (mguthrie)
#       Defined PRINTER_CMD for bu2_us5 and bu3_us5.
#       18-aug-91 (ketan)
#               Added entries for nc5_us5, defined printer command, LIBC
#	03-oct-91 (emerson)
#		Defined xCL_085_PASS_DOUBLES_UNALIGNED for SPARC platforms.
#       05-Aug-91 (Sanjive)
#               Add entries for dra_us5 (ICL DRS3000 running SVR4).
#               Use poll and sigaction. Set the LIBC and pre variables and don't
#               use xCL_010_FSYNC_EXISTs.
#               Also integrated the following change from ingres6302p :-
#               09-Nov-90 (Sanjive)
#                       Amended test for xCL_034_GETRLIMIT_EXISTS. It was being
#                       turned on ONLY if RLIMIT_RSS existed.
#       12-sep-91 (hhsiung)
#               Don't define xCL_010_FSYNC_EXISTS for bu2 and bu3.  Fsync()
#               call is too slow.
#
#	18-dec-91 (gillnh2o & edf)
#		Fixed syntax error on a test from the previous fix.
#	24-jan-92 (sweeney)
#		renamed xCL_042_TLI_EXISTS to xCL_TLI_TCP_EXISTS
#		to reflect its actual use.
#		Added xCL_TLI_OSLAN_EXISTS and xCL_TLI_X25_EXISTS.
#	31-jan-92 (seiwald)
#		Added xCL_BLAST_EXISTS for sun4.
#	19-feb-92 (jab)
#		Added hp8 case for the SEMUN_EXISTS stuff below. It was just
#		done for the hp3. (And made corrections for the invalid (on hp8)
#		assumption that if you had termio.h, you had BSD tty stuff.)
#	26-feb-92 (jab)
#		Added support for xCL_086_SETPGRP_0_ARGS so we can tell if
#		the "setpgrp" system call takes two args, or zero.
#	6-Apr-92 (seiwald)
#		New xCL_SUN_LU62_EXISTS.
#	31-mar-92 (johnr)
#		Added ds3_osf to code which inhibits the union in the
#		wait function.
#	20-mar-92 (gautam )
#		Have RS/6000 define xCL_088_FD_EXCEPT_LIST for LU6.2 support
#	13-apr-1992 (jnash - integrated by bonobo)
#		Add xCL_MLOCK_EXISTS and xCL_SETEUID for locking dmf cache.
#       21-apr-1992 (kimman)
#               Have ds3_ulx define xCL_018_TERMIO_EXISTS if sys/termio.h
#		exists and even if sys/sgtty.h exists.
#       06-feb-92 (sbull)
#               Added entry for the dr6_uv1 (ICL DRS6000 SYS5.4 V1+).
#               define xCL_065_SHADOW_PASSWD, xCL_073_CAN_POLL_FIFOS,
#               xCL_085_PASS_DOUBLES_UNALIGNED. Also set pre='^.*\|' and
#               LIBC=/usr/ccs/ilb/libc.a. Also added a dummy PRINTER_CMD entry
#               since v1+ doesn't support a printer
#	29-apr-92 (purusho)
#               Added xCL_086_BAD_RAWIO for those machines which have
#               4096 bytes multiple only read/write/lseek on the raw
#               device For eg: Amdahl & AIX/370
#               Added xCL_087_IDIV_SIGEMT  for fixed point/decimal
#               exception  on amd_us5
#	29-apr-1992 (pearl)
#		Corrected exclusion of nc5_us5 from defining
#		xCL_066_UNIX_DOMAIN_SOCKETS.  We want this and it works.
#		Also cleaned up nc5_us5 entries to delete defines that were
#		commented out.
#       26-may-1992 (kevinm)
#               Don't define xCL_002_SEMUN_EXISTS for dg8_us5.  union semun
#               is commented out in /usr/include/sys/sem.h.
#       27-may-1992 (kevinm)
#               Define xCL_DG8_DIRECT_IO and set Sigvec="sigaction" on
#               dg8_us5.
#       07-aug-1992 (dianeh)
#       	Removed superfluous -a in front of [ -r /usr/include/limits.h ].
#	17-aug-92 (peterk)
#		Entries for su4_us5 based on bonobo's su4_sol and dr6_us5.
#		You can poll a fifo filesystem, according to the "STREAMS
#		Programmer's Guide, Sun OS 5.0"
#	18-aug-92 (thall)
#		Added sqs_ptx to xCL_086_SETPGRP_0_ARGS group, added defines
#		for xCL_019_TCPSOCKETS_EXIST, xCL_030_SETITIMER_EXISTS, 
#		xCL_TLI_UNIX_EXISTS.
#	20-aug-92 (thall)
#		Backed out xCL_TLI_UNIX_EXISTS define.
#       21-aug-92 (kevinm)
#             Added defines xCL_TLI_NETWARE_EXISTS and xCL_042_TLI_EXISTS
#             for the dg8_us5 platform.
# 	28-aug-92 (sweeney)
#		DomainOS 10.4 - libc is /lib/libc, no suffix.
#		Function names are not prefixed. Also, don't
#		hardwire xCL's (yet) because they are all found
#		anyway and its's better to find them programmatically.
#	28-aug-92 (sweeney)
#		apl_us5:- union semun is conditionally declared on
#		__cplusplus, don't switch on SEMUN_EXISTS.
#	28-aug-92 (sweeney)
#		apl_us5:- add /lib/clib to LIBC (cos dup2 lives there).
#	28-aug-92 (sweeney)
#		don't define CVX_SEM for apl_us5. Define GETTIMEODAY
#		and SYS_TIME_H
#	7-sep-92 (gautam)
#		Defined xCL_TLI_OSLAN_EXISTS for ris_us5 and hp8_us5
#	21-sep-92 (seiwald)
#		Defined xCL_TLI_SPX_EXISTS for su4_u42 and dg8_us5.
#	9-oct-92 (gautam)
#		Added in test for xCL_091_BAD_STDIO_FD
#	16-oct-1992 (lauraw)
#		Now uses readvers to get config string.
#	30-oct-1992 (lauraw)
#		Changed the "trap '' 0" at the end of this script to "trap 0"
#		because on some platforms the former has no effect. In any
#		case, the latter is correct -- we are *unsetting* the trap
#		for 0, not setting it to be ignored.
#	05-nov-92 (mikem)
#	        su4_us5 port.  Changed test for xCL_034_GETRLIMIT_EXISTS
#		to only look for the function in the library instead of looking
#		for the library instance and existence of RLIMIT_RSS define
#		in the resource.h header.  The code which needs to use 
#		RLIMIT_RSS should be ifdef'd on RLIMIT_RSS rather than use
#		xCL_034_GETRLIMIT_EXISTS.  On solaris 2.1 getrlimit() is
#		available (and can be used to get number of file descriptors)
#		but the RLIMIT_RSS option does not exist.
#	19-jan-93 (sweeney)
#		usl_us5 - libc is in /usr/ccs/lib, symbol prefix is '.'
#       19-jan-93 (sweeney)
#		Programatically discover whether shmdt can cope with spare
#		arguments.
#	16-feb-93 (sweeney)
#		Make sure we toss any output from the second cc
#		of shmdtargs.c as well.
#	04-mar-93 (vijay)
#		Add nm prefix for ris.us5. Else grep mlock gets numlocks and
#		other stuff. Also use /bin/nm and not /usr/ucb/nm.
#	10-mar-93 (mikem)
#		Use poll rather than select for su4_us5.  The current select
#		seems to have a bug where it returns EINVAL sometimes for no
#		apparent reason.  Disabled use of BSD sockets for su4_us5.
#	16-mar-93 (smc)
#		Added axp_osf changes: nm prefix string.
#		Added straight define of xCL_MLOCK_EXISTS for axp_osf,
#		mlock() is implemented as memlk() in librt.a and therefore
#		is not resolved by grepping the nm output.
#		Also make axp_osf use isgaction rather than sigvec.
#       01-apr-93 (smc)
#		Added axp_osf to the xCL_086_SETPGRP_0_ARGS case.
#	22-apr-93 (sweeney)
#		usl_us5 has shadow passwords. Go with (native) poll.
#		Use sigaction(). SPX/IPX is supported on this platform.
#       22-apr-93 (ajc)
#               Modified for hp8_bls, same changes as for hp8_us5.
#               Added xCL_NO_INADDR_ANY for tcp/ip maxsix.
#	30-apr-93 (vijay)
#		Remove TLI_OSLAN for ris_us5 for now. Library is corrupt and
#		it does not build.
#	26-jul-93 (mikem)
#		Added support for generating # define xCL_PROCFS_EXISTS when
#		a machine supports the "/proc" filesystem.
#	26-jul-93 (lauraw/brucek)
#		Added xCL_019_TCPSOCKETS_EXIST for su4_us5.
#	05-aug-93 (swm)
#		Switch off xCL_PROCFS_EXISTS for axp_osf. The sys/procfs.h
#		include file does not declare a prusage_t type as required
#		by clf/tm/procfs_perfstat(), nor are some of the prpsinfo_t
#		type elements used present in the axp_osf version. On axp_osf
#		clf/tm/getrusage_perfstat() will get picked up instead.
#	05-aug-93 (swm)
#		Removed erroneous extra line accidently introduced in
#		previous change.
#	28-jul-93 (johnr)
#		Added su4_us5 to the xCL_086_SETPGRP_0_ARGS case for 
#		hp8/sqs/apx.
#	9-aug-93 (robf)
#               Added su4_cmw
#	16-aug-93 (kwatts)
#		Changed support for generating # define xCL_PROCFS_EXISTS
#		to not work for dr6_us5 since the DRS6000 has a
#		different interface. Also set xCL_086_SETPGRP_0_ARGS for
#		dr6_us5.
#	20-sep-93 (mikem)
#		su4_us5 port change.  Change test to determine 
#	        xCL_002_SEMUN_EXISTS from a grep for "semun" to a grep for
#		union.*semun.  In solaris 2.3 a field named "semunp" is 
#		defined in sem.h.
#	24-sep-93 (zaki)
#		Changed default for the pre variable to be '_*[^A-Za-z]' 
#		to stop "Symbols from /lib/libc.a[msemlock.o]" matching with 
#		mlock.  xCL_MLOCK_EXISTS was getting defined incorrectly
# 		on the hp8_us5.
#	14-oct-93 (tomm)
#		Commented out # define xCL_TLI_OSLAN_EXISTS for hp8.  We
#		expect this to be uncommented out at some point, but don't
#		use OSI right now.
#	27-oct-93 (seiwald)
#		Forcibly remove tzone and tzone.c, to avoid pesky prompting
#		by rm.
#	23-nov-93 (seiwald)
#		Forcibly remove all files, since this script is executed
#		by a setuid program and thus rm thinks the user doesn't
#		have write permission.
#	14-oct-1993 (swm)
#		Add hooks to use Posix asynch. i/o. This will be enabled on
#		axp_osf, dr6_us5 and usl_us5 provided pending positive
#		feedback from the respective vendors on some issues (listed
#		below).
#		xCL_ASYNC_IO for Posix asynchronous i/o can be defined
#		manually and eliminates the need for i/o slave processes.
#		Before enabling xCL_ASYNC_IO the following should be checked
#		carefully:
#		o Is it supported for regular files as well as raw devices?
#		o Would other file operations - open(), close(), rename(),
#		  mknod(), creat() ...  block? (It is ok for these to block
#		  in a slave process not in a DBMS server).
#		o Are there sufficent file descriptors available in a DBMS to
#		  support the required number of GCA connections and async.
#		  disk i/o?
#		o and of course, does it actually perform better?
#	10-dec-93 (nrb)
#		Switch off xCL_PROCFS_EXISTS for usl_us5. The sys/procfs.h
#		include file does not declare a prusage_t type.
#		Also, added usl_us5 to the xCL_086_SETPGRP_0_ARGS case.
#	22-feb-94 (nrb)
#		On axp_osf the number of arguments passed to shmdt() is 
#		incorrectly defined. The 'generic test' does not detect this
#		and so xCL_SHMDT_ONE_ARG is explicitly set for axp_osf.
#		Also, put space between "usl_us5"] to be "usl_us5" ].
#	20-apr-94 (mikem)
#		su4_us5 port.  At least for solaris gettimeofday() exists,
#		but only with a single argument (struct timeval *tp).  It can
#		be used to provide the current time in subsecond increments,
#		but will not provide timezone information. 
#	24-apr-94 (seiwald) bug #58961
#		Don't use TLI on Solaris: the word from Sun is that sockets
#		are better, and they are already turned on.  Using both causes
#		(among other things) the GCC to have two TCP listen addresses.
#	21-DEc-94 (GordonW)
#		Added hp8_us5 SNA.
#       04-jan-95 (canor01, chech02)
#               Added rs4_us5 for AIX 4.1. 
#       26-apr-95 (canor01)
#               Added xCL_092_NO_RAW_FSYNC for AIX 4.1 (fsync returns
#		error on character special files
#       24-Feb-95 (brucel)
#               Add xCL_SUN_LU62_EXISTS for solaris (su4_us5).
#       21-Nov-94 (nive/walro03)
#               For dg8_us5 during OpenINGRES 1.1 FCS2 port
#               1. Removed xCL_TLI_NETWARE_EXISTS
#               2. Removed not needed definition of xCL_42_TLI_EXISTS
#               3. Switch off xCL_PROCFS_EXISTS. The sys/procfs.h
#                  include file does not declare a prusage_t type.
#               4. Added dg8_us5 to the xCL_086_SETPGRP_0_ARGS case.
#               5. When using the ansi conformance mode of the C compiler you
#                  get an error when shmdt is referenced with more than one
#                  argument. This doesn't occur under normal compilation. So,
#                  the 'generic test' will not work for dg8_us5 and we are
#                  using the same method used by nrb above. Set
#                  xCL_SHMDT_ONE_ARG explicitly.
#               6. dg8_us5 is a multienvironment system and although dirent.h
#                  exists, direct structures are used instead of dirent
#                  structures and BSD DIR functions are used. Therefore we don't
#                  wish to define xCL_025_DIRENT_EXISTS.
#               7. Set xCL_005_GETTIMEOFDAY_EXISTS and 
#                  xCL_GETTIMEOFDAY_TIME_AND_TZ.
#	09-mar-95 (smiba01)
#		Integrated changes for ICL ES+ platform from ingres63p.
#		Also, Switch off xCL_PROCFS_EXISTS for dr6_ues.
#	1-may-95 (morayf)
#		 Added missing '&&' to check for getrlimit()
#		 (otherwise xCL_034_GETRLIMIT_EXISTS was always set ! )
#	1-may-95 (wadag01)
#		Aligned with 6.4/05	Select="poll"
#					Sigvec="sigaction"
#					xCL_092_CAREFUL_PUTC.
#		Added odt_us5 to the list of platforms that:
#			define xCL_086_SETPGRP_0_ARGS for SysV-style setpgrp()
#			do not define xCL_002_SEMUN_EXISTS.
#	16-may-95 (morayf)
#		For usl_us5 explicitly define xCL_005_GETTIMEOFDAY_EXISTS and
#		xCL_GETTIMEOFDAY_TIME_AND_TZ since Unixware 2.0 gettimeofday
#		needs 2 args, but doesn't fill in the 2nd one (timezone). Thus
#		the tzone.c test thinks it only requires 1 argument.
#		Note that the 2nd arg should really be (void *), but the
#		compiler accepts (struct timezone *) !
#       15-jun-95 (popri01)
#               Add nc4_us5 to PRE, LIBC, and PRINTER defines, per nc5_us5
#               Enable nc5_us5 #defines for nc4_us5
#               Add nc4_us5 to the xCL_086_SETPGRP_0_ARGS case
#               Undef xCL_PROCFS_EXISTS for nc4_us5
#		Add define for variable parm list GETTIMEOFDAY prototype
#	22-jun-95 (gmcquary)
#		Added sqs_ptx port specific change.
#	22-jun-95 (chimi03)
#		For hp8_us5, only do the define for xCL_TLI_TCP_EXISTS if
#		libnsl_s.a AND libnsl_s.sl both exist.  Previously, the
#		define was generated if just libnsl_s.a existed.
#	27-jun-95 (peeje01)
#		Do not define xCL_SUN_LU62_EXISTS for su4_us5DBL
#	20-jul-95 (morayf)
#		Added many sos_us5 thangs. Some changes from odt_us5:
#		getrlimit() exists on OpenServer 5.0
#			=> set xCL_034_GETRLIMIT_EXISTS.
#		nm output has symbol at RHS => pre='^.*\|' 
#		wait union exists in sys/wait.h but is not used 
#			=> Unset xCL_001_WAIT_UNION.
#		union semun does NOT exist, but allow test to fail to find it
#			i.e it will unset xCL_002_SEMUN_EXISTS.
#		Use TLI TCP library.
#		xCL_072_SIGINFO_H_EXISTS should be set via test.
#		Some unchanged things:-
#			SETPGRP_0_ARGS, CAREFUL_PUTC, SHADOW_PASSWD are set.	
#	25-jul-95 (allmi01)
#		Added support for dgi_us5.
#	26-jul-95 (hanch04)
#		Fix typos
#	06-Nov-95 (allmi01)
#		Fixed space between " and ] for Solaris.
#	11-Dec-1995 (walro03/mosjo01/thoro01)
#		Bug 70066: For DG/UX (dg8_us5) and AIX (ris_us5), add define of 
#		xCL_092_NO_RAW_FSYNC
#       05-Jan-96 (hanch04)
#             Added if vers for su4_us5, sol 2.5 should not use BSD calls
#	10-nov-1995 (murf)
#		Added sui_us5 to emulate su4_us5 specifics. These changes
#		have been added for the port of Solaris on Intel.
#		xCL_SUN_LU62_EXISTS is not included for sui_us5 as the SNA
#		feature is not supported. sui_us5 was not added to the section 
#		relating to passong doubles as this is SPARC specific.
#	12-dec-95 (wadag01)
#		Made sure that sos_us5 does not define xCL_TLI_TCP_EXISTS.
#	15-jan-96 (stephenb)
#		added sui_us5 string to su4_us5 string in above hanch04 change.
#	06-dec-95 (morayf)
#		Added rmx_us5 code for SNI RMx00 port.
#	17-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
#		Remove define xCL_086_SETPGRP_0_ARGS for axp_osf.
#	03-mar-96 (morayf)
#		Added pym_us5 to those defining xCL_SHMDT_ONE_ARG.
#       07-May-96 (mosjo01 and moojo03)
#               Remove xCL_077_BSD_MMAP from ris_us5 and rs4_us5.
#	03-jun-1996 (canor01)
#		Remove xCL_ASYNC_IO (POSIX Async I/O) from su4_us5, since
#		POSIX async i/o is not supported yet by Solaris.
#   12-nov-96 (merja01)
#       Add xCL_065_SHADOW_PASSWD to axp_osf for shadow
#       password capability
#	26-feb-1997 (canor01)
#		Added xCL_093_MALLOC_OK and xCL_094_TLS_EXISTS.
#   25-Apr-1997 (merja01)
#       Add xCL_094_TLS_EXISTS, xCL_093_MALLOC_OK, and 
#       xCL_ASYNC_IO to axp_osf definition for 2.0 pthreads and
#       async IO.
#	15-may-1997 (muhpa01)
#		Use sigaction instead of sigvec on HP-UX.
#		Added xCL_093_MALLOC_OK and xCL_094_TLS_EXISTS for hp8_us5.
#		Don't define xCL_001_WAIT_UNION for hp8_us5.
#		Removed commented define for xCL_TLI_OSLAN_EXISTS.
#   21-may-1997 (musro02)
#       For sqs_ptx define ...UNIX_EXISTS instead of ...TCP_EXISTS;
#       Add sqs_ptx to platforms  not needing ...SEMUN_EXISTS and
#       not needing ...PROCFS_EXISTS
#   28-jul-1997 (walro03)
#		Updated for Tandem (ts2_us5).
#   08-Aug-1997 (merja01)
#       Explicitly set TYPESIG to void on axp_osf to avoid
#       numerous compile warnings in command.c
#   15-Aug-1997 (schte01)
#       Add xCL_DIRECT_IO and xCL_PRIORITY_ADJUST for axp_osf. 
#       (See clconfig.h for description).  
#	15-aug-1997 (popri01)
#		Enabled native O/S thread-local storage management for usl_us5.
#	02-sep-1997 (muhpa01)
#		Added define for xCL_DIRECT_IO for hp8_us5
#
#       14-feb-1997 (mosjo01/linke01)
#               Initially will build w/o xCL_ASYNC_IO for dgi_us5 until
#               clean build, then will try xCL_ASYNC_IO.
#               Added dgi_us5 to xCL_092_NO_RAW_FSYNC.
#       26-Mar-1997 (bonro01)
#               Added new config defines for xCL_MISSING_DG_REENTRANT_ROUTINES
#               These are used to determine when gethostbyname_r,
#               gethostbyaddr_r econvert, and fconvert are missing.
#       29-Aug-1997 (hweho01)
#               Define xCL_NEED_SEM_CLEANUP for dgi_us5.
#       29-Aug-1997 (kosma01)
#               Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5 
#               to agree with arguments of gettimeofday() in <sys/time.h>.
#		Also added xCL_TIME_CNVRT_TIME_AND_BUFFER
#		This platform's asctime() does not get a length parameter.
#	24-sep-1997 (hanch04)
#	    remove integration errors
#	26-Sep-1997 (bonro01)
#		Removed xCL_MISSING_DG_REENTRANT_ROUTINES.
#		Removed duplicate comments.
#	27-mar-1998 (walro03)
#		Added xCL_094_TLS_EXISTS for Tandem NonStop (ts2_us5).
#	05-Nov-1997 (hanch04)
#	    Added xCL_ASYNC_IO for su4_us5 for conf_B64(Solaris 2.6 and up) 
#	08-dec-1997 (canor01)
#	    Enable xCL_003_GETRUSAGE_EXISTS for Solaris.
#	24-dec-1997 (canor01)
#	    Add xCL_WAITPID_EXISTS. Synchronize Ingres and Jasmine changes
#	    to allow common source.
#	27-Jan-1998 (muhpa01)
#		Removed xCL_094_TLS_EXISTS for hp8_us5 - turning off
#		support for POSIX_DRAFT_4 threads.
#	03-mar-1998 (canor01)
#	    Add xCL_RESERVE_STREAM_FDS for all OS threaded platforms.
#	06-Mar-1998 (kosma01)
#	    Add rs4_us5 to the list using xCL_DIRECT_IO to avoid missing
#	    symbols pread and pwrite.
#	    Also remove xCL_TIME_CNVRT_TIME_AND_BUFFER. fix for asctime_r
#	    is now handled in cl/clf/hdr/tmcl.h
#	    Remove ASYNC_IO for rs4_us5. ASYNC_IO for rs4_us5 needs more
#	    work to be used on rs4_us5, but it causes install problems even
#	    even if customer never intends to use it. (Could not load program
#           iisulock. Symbol kaio_rdwr in /lib/libc_r.a is undefined) 
#	18-feb-1998 (toumi01)
#		Added Linux (lnx_us5) to LIBC definitions.
#		For Linux modify test for setting xCL_014_FD_SET_TYPE_EXISTS.
#		Add lnx_us5 to list of platforms for which we should not
#		test for defining xCL_PROCFS_EXISTS.
#		Add lnx_us5 to the xCL_086_SETPGRP_0_ARGS case.
#	03-mar-1998 (canor01)
#	    Add xCL_RESERVE_STREAM_FDS for all OS threaded platforms.
#       5-Mar-1998 (allmi01/linke01)
#               Remove xCL_016_VALLOC_EXISTS from pym_us5
#       12-Mar-1998 (allmi01/linke01)
#               Remove xCL_016_VALLOC_EXISTS from pym_us5
#      20-Apr-1998 (merja01)
#             Removed xCL_ASYNC_IO from axp_osf.  Both internal and external
#             threads perform better without async IO.
#	26-Mar-1998 (muhpa01)
#		Added settings for new HP configuration, hpb_us5, which is
#		used for HP-UX 11.0 32-bit; don't define xCL_TLI_TCP_EXISTS
#		for hpb_us5.  Also, add hpb_us5 to list of platforms which
#		define xCL_RESERVE_STREAM_FDS.
#	01-Jun-1998 (hanch04)
#	    Added xCL_NO_AUTH_STRING_EXISTS and xCL_CA_LICENSE_CHECK for
#	    INGRESII and new CA license
#	05-Jun-1988 (allmi01)
#		Added entries for Silicon Graphics (sgi_us5).
#	17-jun-1998 (allmi01)
#	    Added xCL_093_MALLOC_OK / xCL_094_TLS_EXISTS for this platform.
#	02-jul-1998 (toumi01)
#		Add xCL_065_SHADOW_PASSWD for Linux.
#	10-Jul-1998 (fucch01)
#		Added sgi_us5 to platforms that don't define xCL_PROCFS_EXISTS
#		to resolve compile problem w/ tmprfstat.c in cl/clf/tm.  Using
#		xCL_003_GETRUSAGE_EXISTS compiles fine.
#	03-aug-1998 (toumi01)
#		Linux is IngresII but does not use CA licensing, so make
#		it a special case for xCL_NO_AUTH_STRING_EXISTS and
#		xCL_CA_LICENSE_CHECK.
#		Define xCL_065_SHADOW_PASSWD for Linux.
#     11-jul-1998 (popri01)
#		For Unixware (usl_us5), remove xCL_RESERVE_STREAM_FDS because
#		the duplicated socket fd's get connect errors.
#	28-jul-1998 (hanch04)
#	    xCL_NO_AUTH_STRING_EXISTS and xCL_CA_LICENSE_CHECK should always
#	    be defined.
#	03-sep-1998 (toumi01)
#		Force Sigvec='sigaction' for Linux (needed for libc6 build).
#     24-nov-1998 (musro02)
#         For sqs_ptx remove xCL_TLI_UNIX_EXISTS so that no ...TLI... is defined
#		
#	30-nov-1998 (toumi01)
#		For the free Linux version, reenable II_AUTH_STRING checking,
#		to be used as a mechanism to "time bomb" the product.
#	07-Dec-1998 (muhpa01)
#	    Don't define xCL_TLI_TCP_EXISTS for hp8_us5.
#	18-dec-1998 (hanch04)
#	    xCL_SYS_PAGE_MAP defined to use the system free memory mapping
#	    routines.
#       18-jan-1999 (matbe01)
#               Remove xCL_016_VALLOC_EXISTS for nc4_us5 and fix integration
#               error by sgi_us5 which affected pym_us5.
#       22-jan-1999 (hanch04)
#		Removed extra fi that caused a syntax error
#       05-mar-1999 (matbe01)
#               Refined test for setting xCL_014_FD_SET_TYPE_EXISTS rs4_us5.
#	15-mar-1999 (popri01)
#		For UW 7, use new xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR.
#	17-Mar-1999 (kosma01)
#	    sgi_us5 is a pthread platform now. Add it to the list of platforms
#	    that need to reserve low file descriptors for use by standard
#	    i/o stream functions. xCL_RESERVE_STREAM_FDS
#	    Because we are compiling for -mips3 -n32 (new 32 bit format ELF)
#	    any system library reference s/b to a /usr/lib32/mips3 library.
#	    Trying to remove any BSD dependencies. change to SYSV setpgrp().
#	    xCL_086_SETPGRP_0_ARGS.
#	28-Apr-1999 (ahaal01)
#	    Removed xCL_034_GETRLIMIT_EXISTS for rs4_us5.  Getrlimit
#	    returns too big of a value for number of file descriptors
#	    in iiCL_get_fd_table_size (in clfd.c).
#       30-Apr-1999 (allmi01)
#           Added entries to rmx_us5 to add OS POSIX threads support.
#       07-May-1999 (allmi01/popri01)
#           For Siemens (rmx_us5):
#           Add to list of platforms not wanting xCL_001_WAIT_UNION.
#           Disallow use of vfork.
#           Disable xCL_RESERVE_STREAM_FDS (as in Unixware).
#	10-may-1999 (walro03)
#	    Remove obsolete version string amd_us5, apl_us5, bu2_us5, bu3_us5,
#	    bul_us5, dr3_us5, dr4_us5, dr6_ues, dr6_uv1, dra_us5, ds3_osf,
#	    odt_us5, nc5_us5, ps2_us5, rtp_us5, sqs_us5, vax_ulx, 3bx_us5.
#       03-jul-1999 (podni01)
#           Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
#	20-jul-1999 (toumi01)
#		Add xCL_093_MALLOC_OK for Linux (lnx_us5).
#	13-sep-1999 (hanch04)
#	    Re-add check for fsync to define xCL_010_FSYNC_EXISTS
#	14-Sep-1999 (allmi01/bonro01)
#	    Added xCL093 and xCL094 defines for dgi_us5 (activate POSIX
#	    thread local stroage).
#	01-oct-1999 (toumi01)
#		Generalize the control of licensing (AUTH STRING vs. CA
#		LICENSING) for Linux.  For this platform (because of the
#		special needs for the full-commercial, free-download, SDK,
#		TNG, and special-expiring  versions) we need to control
#		this based on VERS file options:
#			option=NO_AUTH_STRING
#			option=CA_LICENSE
#		rather than overloading INGRESII to control these license
#		options.
#	06-oct-1999 (toumi01)
#		Change Linux config string from lnx_us5 to int_lnx.
#	   21-oct-1999 (johco01)
#		 Add sui_us5 to platforms that do not use standard memory
#               library functions.
#       10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
#               Correct rux_us5 changes that were applied using the rmx_us5
#               label.
#	20-apr-2000 (somsa01)
#	    Added other products as alternate product builds.
#       23-apr-2000 (hweho01)
#           Added support for AIX 64-bit platform (ris_u64).    
#	05-Jun-2000 (ahaal01)
#	    Removed xCL_NEED_SEM_CLEANUP for rs4_us5 since it is not needed.
#	15-Jun-2000 (hanje04)
#		For OS/390 Linux added ibm_lnx to Linux specific changes and
#		definitions.
#	1-Aug-2000 (bonro01)
#	    AXP_OSF vfork, starting with release 5.0 started having SEGV
#	    problems in freopen() in the child process started with vfork.
#	    Currently xCL_023_VFORK_EXISTS is only referenced in test tools
#	    achilles and sep.
#	30-aug-2000 (somsa01)
#	    Added xCL_067_GETCWD_EXISTS for HP platforms.
#	19-apr-1999 (hanch04)
#	    Added su4_us5
#	11-Sep-2000 (hanje04
#	    Added support for Alpha Linux (axp_lnx).
#	17-Oct-2000 (hanje04)
#	    Added xCL_067_GETCWD_EXISTS for Intel Linux.
#	02-Nov-2000 (hweho01)
#	    Added xCL_067_GETCWD_EXISTS for Alpha (axp_osf) platform.
#       22-dec-2000 (mosjo01)
#               Resurrecting sigaction for sgi_us5 platform to
#               fix problem with repmgr. Applicable to 2.0, 2.5 and 3.0.
#       01-jan-2001 (mosjo01)
#           Added xCL_067_GETCWD_EXISTS for Sequent Dynix/ptx platforms.
#           Also  xCL_064_SQT_DIRECT_IO for sqs_ptx remains viable for 2.5
#           since II_DIRECT_IO functionality has been integrated in cldio.c
#           from 2.0 to 2.5 along with additional tweaking.
#       28-Feb-2001 (musro02)
#          Added xCL_067_GETCWD_EXISTS for NCR (nc4_us5) platform.
#	19-mar-2001 (somsa01)
#	    Other product builds do not use CA Licensing.
#	12-Jan-2001 (bonro01)
#           Add rmx_us5 to list of platforms not wanting xCL_001_WAIT_UNION.
#	    pcreap.c fails to compile because the wait union is not in
#	    the standard wait.h header file, it is under the /usr/ucbinclude
#	    directories.  However, I'm not able to include the proper UCB
#	    includes directories in the build because then the math.h UCB
#	    header file gets errors all over the build because it is including
#	    a UCB header file called fp.h which is a header file that
#	    conflicts with gl/hdr/hdr/fp.h  The easiest fix is to not use
#	    the wait union.
#	23-Apr-2000 (hweho01)
#	    Added xCL_067_GETCWD_EXISTS for ris_u64  platforms.
#           Removed ris_u64 from the list of xCL_086_SETPGRP_0_ARGS. 
#	24-Apr-2001 (ahaal01)
#	    Added xCL_067_GETCWD_EXISTS for AIX (rs4_us5) platform.
#	    Removed AIX (rs4_us5) from the list of xCL_086_SETPGRP_0_ARGS.
#      11-june-2001 (legru01)
#           removed incorrect variable xCL_GETTIMEOFDAY_TIME_AND_TZ
#           replaced with xCL_005_GETTIMEOFDAY_EXITS 
#           and xCL_GETTIMEOFDAY_TIMEONLY to support system time function
#           "gettimeofday()"
#	19-jul-2001 (stephenb)
#	    Further changes for i64_aix
#	22-aug-2001 (toumi01)
#	    Force use of sigaction for i64_aix
#	03-Sep-2001 (hanje04)
#	    Added support for IA64 Linux (i64_lnx)
#	11-Dec-2001 (bonro01)
#	    USL_US5, started having abends in the sep module when vforking
#	    processes.  The usl.us5 build was moved to Unixware 7.1.1 on
#	    a cluster machine which may have played a factor in the vfork
#	    problem.  Changing to a regular fork() eliminated the problem.
#	11-dec-2001 (somsa01)
#	    Porting changes for hp2_us5.
#	29-may-2002 (somsa01)
#	    For hpb_us5, removed the setting of xCL_094_TLS_EXISTS. In
#	    investigating problems with the "csphil" program, it was
#	    found that pthread_getspecific() would return a NULL value
#	    (value not defined for key in specific thread) even though
#	    a successful pthread_setspecific() was executed. This
#	    happened sporadically. Thus, we will be using our own TLS
#	    functions.
#	12-sep-2002 (devjo01)
#	    Add conditional declare of xCL_095_USE_LOCAL_NUMA_HEADERS
#	    for Tru64 5.0 build environments.
#	17-Oct-2002 (bonro01)
#	    For sgi_us5, added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR to fix
#	    compiler warnings.
#       21-Oct-2002 (hweho01)
#           Added change for 32/64 bit hybrid build on AIX platform. 
#	31-oct-2002 (somsa01)
#	    From HP-UX, removed xCL_DIRECT_IO and use "stdlim" to decide
#	    if we should set xCL_091_BAD_STDIO_FD. Furthermore, added
#	    check for pwrite() to see if we should define xCL_DIRECT_IO
#	    (we should if we do not have pwrite). Also, changed
#	    xCL_DIRECT_IO to xCL_NO_ATOMIC_READ_WRITE_IO, which more
#	    matches its meaning.
#	06-Nov-2002 (hanje04)
#	    Added suport for AMD x86-64 Linux and genericise Linux defines
#	    where ever apropriate.
#	27-Mar-2003 (jenjo02)
#	    Added generation of xCL_DIRECTIO_EXISTS if directio() function
#	    exists. This is -not- the same as or related to
#	    xCL_DIRECT_IO. SIR 109925
#	07-Feb-2003 (bonro01)
#	    Added support for IA64 HP (i64_hpu)
#       23-Sep-2002 (hanje04)
#           Make sure xCL_018_TERMIO_EXISTS is set correctly on Linux without
#           having to create "dummy" system headers.
#	23-dec-2003 (hayke02)
#	    Modify previous change so that we use ='s rather than =='s when
#	    testing for the Linux vers identifiers.
#	11-Jun-2004 (somsa01)
#	    Cleaned up code for Open Source.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#	18-Nov-2004 (hanje04)
#	    BUG 113487/ISS 13680685
#	    Define xCL_SYS_PAGE_MAP for Linux.
#       22-Dec-2004 (nansa02)
#           Changed all trap 0 to trap : 0 for bug (113670).
#	11-Jan-2004 (hanje04)
#	    Define xCL_094_TLS_EXISTS & xCL_RESERVE_STREAM_FDS for all Linux
#	26-Jan-2005 (hanje04)
#	    FIXME!!!!
#	    Hardcode defines usually generated by mathsigs for AMD Linux.
#	    mathsigs is failing on SuSE 9.2 pro for AMD64 and just exiting
#	    with a SIG_FPE. Looks like glibc bug as it works on other AMD64
#	    Linuxes
#	26-Jan-2005 (hanje04)
#	   Correct typos!!
#	25-Feb-2005 (hanje04)
#	   Implementing xCL_RESERVE_STREAM_FDS has caused a number
#          of problems. Until these can be resolved it cannot be used.
#	   See issues 13935786 & 13941751 for more info.
#	15-Mar-2005 (bonro01)
#	    Add support for Solaris AMD64 a64_sol
#       18-Apr-2005 (hanje04)
#           Add support for Max OS X (mac_osx).
#           Based on initial changes by utlia01 and monda07.
#	31-May-2005 (schka24)
#	    Fine-tune a64_sol to be in line with other Solaris platforms.
#	    (TLS exists, etc.)
#	10-Jun-2005 (toumi01)
#	    For Linux specify Select='poll' so that we use poll vs. select,
#	    which is marginally faster (about 3%) for this OS.
#	23-Aug-2005 (schka24)
#	    Fix libc definition for Solaris 10 (works for older ones too).
#	18-Oct-2005 (hanje04)
# 	    Remove tzone files after use.
#	04-Feb-2006 (hweho01)
# 	    Defined xCL_034_GETRLIMIT_EXISTS for AIX platform.
#	28-Apr-2006 (kschendel)
#	    (Mark Kirkwood) Don't define a bogus printer command.  If the
#	    build machine doesn't have printing installed, the build blows
#	    up for non-obvious reasons.  Always define something.
#	27-Jun-2006 (raddo01)
#		bug 116311: remove workaround for mathsigs core dump on a64_lnx.
#	07-Oct-2006 (lunbr01)  Sir 116548
#	    Defined xCL_TCPSOCKETS_IPV6_EXIST for platforms that support IPV6.
#	11-Oct-2006 (hanje04)
#	    BUG: 116919
#	    Linux platorms only use "union" for wait() status when USE_BSD is
#	    defined (which we don't). Don't define xCL_001_WAIT_UNION on Linux.
#	    Also define xCL_USE_WAIT_MACROS for those platforms that have them
#	    this allows PCreap() to more accurately determine the state of
#	    child processes. (Needed by GUI installer)
#	24-Oct-2006 (lunbr01)  Sir 116548
#	    Defined xCL_TCPSOCKETS_IPV6_EXIST for all platforms that
#	    support IPv6 rather than just Linux (prior change).
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##       7-Nov-2006 (hanal04) SIR 117044
##          Correct case typo from change 484294.
#	02-Jan-2007 (lunbr01)  Bug 117251, Sir 116548
#	    Generate xCL_TCPSOCKETS_IPV6_EXIST for platforms that
#	    support IPv6 but in not-quite-standard libraries/headers
#	    such as HP-UX and Unixware.
#	23-Mar-2007
#	    SIR 117985
#	    Add support for 64bit PowerPC Linux (ppc_lnx)
#	30-Apr-2007 (hanje04)
#	    SIR 117985
#	    Correct method used to conditionally include rpcsvc/rex.h on Linux.
#	    There is no reliable way of determining if it's needed until the
#	    compilation fails. Use that to define xCL_NEED_RPCSVC_REX_HDR
#	    on Linux if we do need it.
#	08-Feb-2008 (hanje04)
#	    SIR S119978
#	    Add support for Intel OS X
#	23-Jun-2008 (bonro01)
#	    Removed obsolete xCL_067_GETCWD_EXISTS.  All platforms support
#	    getcwd() now.
#	03-Sep-2008 (hanje04)
#	    Define xCL_STROPTS_H_EXISTS if /usr/include/stropts.h exists.
#	    It doesn't for FC9 and we handle that.
##	18-Jun-2009 (kschendel) SIR 122138
##	    readvers can return either config string now for hybrid, make
##	    sure that we're testing all possibilities and not just the
##	    32-bit string.
##	    Stop digging around in libc, use trial compile/link.  It's
##	    more generic and eliminates noisy complaints from nm on linux.
##	17-Jul-2009 (kschendel) SIR 122138
##	    Get rid of the sort, plays hob with any attempt at conditionals.
##	17-Aug-2009 (frima01) SIR 122138
##	    Slightly change determination of fd_set again to work on HP-UX.
##	16-Sep-2009 (hanje04)
##	    Define xCL_005_GETTIMEOFDAY_EXISTS for OSX, not sure why it wasn't
##	    before but it needs it now
##	24-Sep-2009 (hanje04)
##	    BUG 122827
##	    Definition of fd_set on Linux has moved to sys/select.h on later
##	    versions (e.g. fc11). Remove redundant *_lnx block and add
##	    linux/types.h to list files to check for other platforms,
##	    which already contains sys/select.h
##	17-Nov-2010 (kschendel) SIR 124685
##	    Prefer sigaction to sigvec.  Pipe grep errors to dev-null when
##	    looking for fd-set stuff.
##	19-Nov-2010 (frima01) BUG 124750
##	    Set gettimeofday arguments for i64.lnx.
##	23-Nov-2010 (kschendel)
##	    Drop a few more obsolete ports.  Fix test for TYPESIG on Solaris.
##	1-Dec-2010 (bonro01) SIR 124685
##	    Fix Solaris build problem introduced by change 508738.
##	    
#	    
#
header=clsecret.h
date=`date`

# stdout into newly created header

exec > $header

# don't leave it around if we have trouble

#trap 'rm -f $header' 0

. readvers

product="Ingres"
machine=`machine`
whatunix=`whatunix`

# create a new target:

cat << !
/*
** $product $header created on $date
** by the $0 shell script
*/
!

# version file generated by running the source transfer script
# generated by mkreq.sh.  GV_VER and GV_ENV used in gver.roc.

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

vers=$config

(

#
# These capabilities are not reliably determined by inspection.
#
# If you make a special case for your machine, be sure to duplicate
# all the defines from the generic *) case.
#
#	xCL_019_TCPSOCKETS_EXIST	BSD sockets(2) exist, but
#					    /usr/include/netinet/in.h doesn't
#	xCL_022_AIX_LU6_2		AIX style SNA/LU6.2 protocol is here,
#					    but /usr/include/luxsna.h isn't.
#	PRINTER_CMD		Command to use to line print by UTprint.
#	xCL_065_SHADOW_PASSWD		Systems that using shadow passwd file.
#       xCL_TLI_OSLAN_EXISTS		ISO OSI local area network.
#	xCL_TLI_X25_EXISTS		ISO OSI x25 network.
#

#
# Default is any kind of "select/poll" it finds.
# To force the useage of a certain "select" pr "poll"
# set the following to either "poll" or "select".
# such as:
#
# abc_us5)	Select="poll"
#		;;
#    or
# xyz_u42)	Select="select"
#		;;
#    or
# lmn_us5)	Select="none"
#		echo '# define xCL_043_SYSV_POLL_EXISTS'
#		;;
#
Select="any"

# Similarly, with sigvec/sigaction. If both exist, default is to define
# xCL_011_USE_SIGVEC. To force the usage of one or the other or
# neither, use the following format:
#
# xxx_ooo)	Sigvec="sigvec"
#		;;
# or
# xxx_ooo	Sigvec="sigaction"
#		;;
# or
# xxx_ooo)	Sigvec="none"
#		;;
Sigvec="any"

	if [ $vers = int_lnx ] || [ $vers = int_rpl ] || \
	   [ $vers = ibm_lnx ] || [ $vers = axp_lnx ] || \
	   [ $vers = i64_lnx ] || [ $vers = a64_lnx ] || \
	   [ $vers = ppc_lnx ]
	then
		if [ " $conf_NO_AUTH_STRING" = " true" ]
		then
			echo '# define xCL_NO_AUTH_STRING_EXISTS'
		fi
		if [ " $conf_CA_LICENSE" = " true" ]
		then
			echo '# define xCL_CA_LICENSE_CHECK'
		fi
	fi

case $vers in
r64_us5|\
rs4_us5)	echo '# define xCL_065_SHADOW_PASSWD'
		echo '# define xCL_088_FD_EXCEPT_LIST'
		echo '# define xCL_005_GETTIMEOFDAY_EXISTS'
		echo '# define xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR'
		# echo '# define xCL_TLI_OSLAN_EXISTS'
		;;

su9_us5|\
su4_us5)        echo '# define xCL_065_SHADOW_PASSWD'
                echo '# define xCL_073_CAN_POLL_FIFOS'
                echo '# define xCL_BLAST_EXISTS'
		# Do not set xCL_SUN_LU62_EXISTS for Doublebyte:
		if [ -z "$conf_DBL" -a -d /opt/SUNWconn/snap2p/p2p_lib/include ] ; then
		    echo '#if ! defined(LP64)'
		    echo '# define xCL_SUN_LU62_EXISTS'
		    echo '#endif'
		fi
                echo '# define xCL_TLI_UNIX_EXISTS'
                echo '# define xCL_NO_INADDR_ANY'
		echo '# define xCL_OVERRIDE_IDIV_HARD'
		echo '# define xCL_076_USE_UNAME'
	        echo "# define xCL_SYS_PAGE_MAP"
		# Solaris really should be GETTIMEOFDAY_TIMEONLY_V.
		# SPARC however is compiled with _SVID_GETTOD defined (why??)
		# which brings in the one-parameter gettimeofday prototype.
		echo "# define xCL_005_GETTIMEOFDAY_EXISTS"
		echo "# define xCL_GETTIMEOFDAY_TIMEONLY"
                Select=poll
                ;;
a64_sol)
		echo '# define xCL_065_SHADOW_PASSWD'
		# Solaris defines this explicitly since it has
		# both getwd and getcwd, which flunks the libc
		# inspection test below
		echo '# define xCL_OVERRIDE_IDIV_HARD'
		# See above Solaris note.
		# a64-sol is special-cased in tmtime, so use voidptr.
		echo "# define xCL_005_GETTIMEOFDAY_EXISTS"
		echo "# define xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR"
	        echo "# define xCL_SYS_PAGE_MAP"
		;;
sgi_us5)
		echo '# define xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR'
		echo "# define xCL_GETTIMEOFDAY_TIMEONLY_V"
		Sigvec="sigaction"
                Select="poll"
                ;;
i64_hpu|\
hp2_us5|\
hpb_us5|\
hp8_us5)
		Sigvec="sigaction"
		;;
axp_osf)	echo "# define xCL_MLOCK_EXISTS"
                echo "# define xCL_065_SHADOW_PASSWD"
                echo "# define xCL_PRIORITY_ADJUST"
		Sigvec="sigaction"
		;;
usl_us5)	echo '# define xCL_065_SHADOW_PASSWD'
		echo '# define xCL_TLI_SPX_EXISTS'
		echo '# define xCL_005_GETTIMEOFDAY_EXISTS'
		echo '# define xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR'
		Select='poll'
		Sigvec='sigaction'
		;;
 *_lnx|int_rpl)	echo '# define xCL_065_SHADOW_PASSWD'
	        echo "# define xCL_SYS_PAGE_MAP"
                echo "# define xCL_005_GETTIMEOFDAY_EXISTS"
                echo "# define xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR"
		Select='poll'
		Sigvec='sigaction'
		;;
   *_osx)	echo "# define xCL_SYS_PAGE_MAP"
	        echo "# define xCL_USE_ETIMEDOUT"
		echo '# define xCL_005_GETTIMEOFDAY_EXISTS'
		echo '# define xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR'
		Sigvec='sigaction'
		;;
esac

#
# libc.a functions -- figured out except on machines where it isn't
# 		reliable, or in dual universe situations where you
#		know better.
#

# tf = filename for trial compile

tf=/tmp/secret$$
rm -f ${tf}*

#
#
# The following tests whether gettimeofday() sets the zone field before
# setting xCL_005_GETTIMEOFDAY_EXISTS; this is done because the test will
# fail on some(all?) sysV machines which have gettimeofday().
#
# Test is hardwired for some platforms, those are excluded.

case $vers in
    su4_us5 |\
    su9_us5 |\
    a64_sol |\
    r64_us5 |\
    rs4_us5 |\
    sgi_us5 |\
    usl_us5 |\
    i64_lnx |\
      *_osx)
	    ## hardwired
	    ;;
	*)
	echo '
#include <sys/types.h>
#include <sys/time.h>

main()

{
    struct timeval	time;
    struct timezone	zone;

    zone.tz_minuteswest = 1234;
    zone.tz_dsttime = 5678;
    if (gettimeofday(&time, &zone) == -1) {
	perror("gettimeofday");
	exit(1);
    }
    if (zone.tz_minuteswest == 1234 || zone.tz_dsttime == 5678)
	return(1);
    else
	return(0);
    }
' > ${tf}.c

	if (cc -o $tf ${tf}.c) > /dev/null 2>&1
	then
	    if `$tf`
	    then
		echo "# define xCL_005_GETTIMEOFDAY_EXISTS"
		echo "# define xCL_GETTIMEOFDAY_TIME_AND_TZ"
	    else
		echo "# define xCL_005_GETTIMEOFDAY_EXISTS"
		echo "# define xCL_GETTIMEOFDAY_TIMEONLY"
	    fi
	fi
	rm -f ${tf}*
	;;
esac


case "$vers" in
axp_osf|su4_us5|su9_us5)
		echo "# define xCL_SHMDT_ONE_ARG"
		;;
*)
echo '
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void
shmdtargs()
{
shmdt((char *)10000
#ifndef xCL_SHMDT_ONE_ARG
		   , 0, 0
#endif
			 );
}
' > ${tf}.c
cc -c ${tf}.c -o ${tf}.o > /dev/null 2>&1 || 
	(cc -c -DxCL_SHMDT_ONE_ARG ${tf}.c -o ${tf}.o > /dev/null 2>&1 &&
		echo "# define xCL_SHMDT_ONE_ARG")
	rm -f ${tf}*
	;;
esac

	# A bunch of these could probably be assumed these days, since
	# most Unixen make at least a show of complying with Posix / SuS.

	echo 'main(){getrusage();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_003_GETRUSAGE_EXISTS'
	rm -f ${tf}*

	echo 'main(){getdtablesize();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_004_GETDTABLESIZE_EXISTS'
	rm -f ${tf}*

	echo 'main(){waitpid();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_WAITPID_EXISTS'
	rm -f ${tf}*

	# This one is an if-doesn't-work
	echo 'main(){pwrite();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 ||
		echo '# define xCL_NO_ATOMIC_READ_WRITE_IO'
	rm -f ${tf}*

	echo 'main(){directio();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_DIRECTIO_EXISTS'
	rm -f ${tf}*

	echo 'main(){mkdir();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_008_MKDIR_EXISTS'
	rm -f ${tf}*

	echo 'main(){rmdir();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_009_RMDIR_EXISTS'
	rm -f ${tf}*

	echo 'main(){fsync();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_010_FSYNC_EXISTS'
	rm -f ${tf}*

	echo 'main(){dup2();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_012_DUP2_EXISTS'
	rm -f ${tf}*

	echo 'main(){valloc();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_016_VALLOC_EXISTS'
	rm -f ${tf}*

	echo 'main(){opendir();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_024_DIRFUNC_EXISTS'
	rm -f ${tf}*

	echo 'main(){execvp();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_029_EXECVP_EXISTS'
	rm -f ${tf}*

	echo 'main(){setitimer();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_030_SETITIMER_EXISTS'
	rm -f ${tf}*

	echo 'main(){ftok();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_039_FTOK_EXISTS'
	rm -f ${tf}*

	echo 'main(){getrlimit();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_034_GETRLIMIT_EXISTS'
	rm -f ${tf}*

	echo 'main(){rename();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_035_RENAME_EXISTS'
	rm -f ${tf}*

	echo 'main(){writev();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_074_WRITEV_READV_EXISTS'
	rm -f ${tf}*


	if [ $vers != "su4_us5" ] && [ $vers != "su9_us5" ] 
	then
	    echo 'main(){setreuid();}' >${tf}.c
	    cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_044_SETREUID_EXISTS'
	    rm -f ${tf}*
	fi

if [ $vers != 'usl_us5' -a $vers != 'sgi_us5' -a $vers != 'axp_osf' ] ; then
	#
	#  SGI vfork, probably in conjunction with some -D defines we
	#  we shouldn't be using, will not admit to having any children.
	#  When wait() is run after vfork, wait() only return -1 and 
	#  errno was ECHLD.
	#
	#  Siemens vfork is not supported in an (OS) threaded environment.
	#  We lose file handles in spawned sep processes, as one symptom.
	#
	#  AXP vfork, starting with release 5.0 started having SEGV problems
	#  in freopen() in the child process started with vfork.
	#
	echo 'main(){vfork();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_023_VFORK_EXISTS'
	rm -f ${tf}*
fi

if [ $vers != "su4_us5" -a $vers != "hp8_us5" \
          -a $vers != "hpb_us5" -a $vers != "su9_us5"  \
          -a $vers != "axp_osf" -a $vers != "a64_sol"  \
	  -a $vers != "hp2_us5" -a $vers != "i64_hpu" ]
then
	echo 'main(){setpriority();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_033_SETPRIORITY_EXISTS'
	rm -f ${tf}*
fi

	if [ "$Select" = 'any' ] ; then
	    echo 'main(){select();}' >${tf}.c
	    cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		Select="select"
	    rm -f ${tf}*
	fi

	#
	# Prefer sigaction if it's available.
	# These defines are mutually exclusive
	#
	if [ "$Sigvec" = 'any' ] ; then
	    echo 'main(){sigaction();}' >${tf}.c
	    cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		Sigvec="sigaction"
	    rm -f ${tf}*
	fi

	#
	# Use sigvec if it exists and no sigaction
	#
	if [ $vers != "rs4_us5" -a $vers != "r64_us5" -a "$Segvec" = 'any' ]
	then
	    echo 'main(){sigvec();}' >${tf}.c
	    cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		Sigvec="sigvec"
	    rm -f ${tf}*
	fi

	#
	# Now decide whether to use sigvec or sigaction
	# (Assumption about BSD semantics with with sigvec
	# may be wrong, but won't hurt)
	#
	if [ "$Sigvec" = "sigvec" ]
	then
		echo "# define xCL_011_USE_SIGVEC"
		echo "# define xCL_013_CANT_KILL_SUID_CHILD"
	elif [ "$Sigvec" = "sigaction" ]
	then
		echo "# define xCL_068_USE_SIGACTION"
	fi

	#
	# hide hp/ux stupidity.
	#
	echo 'main(){sigvector();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define sigvec sigvector'
	rm -f ${tf}*

if [ $vers != "su4_us5" -a $vers != "su9_us5" ]
then
	echo 'main(){gethostname();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 ||
		(
		echo 'main(){uname();}' >${tf}.c
		cc -o $tf ${tf}.c >/dev/null 2>&1 &&
			echo "# define xCL_076_USE_UNAME"
		)
	rm -f ${tf}*
fi

if [ $vers != "r64_us5" -a $vers != "rs4_us5" ]
then
	echo 'main(){mmap();munmap();msync();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo "# define xCL_077_BSD_MMAP"
	rm -f ${tf}*
fi

	echo 'main(){mset();mclear()}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo "# define xCL_078_CVX_SEM"
	rm -f ${tf}*

	echo 'main(){sysconf();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_079_SYSCONF_EXISTS'
	rm -f ${tf}*

if [ $vers != "su4_us5" -a $vers != "su9_us5" ]
then
	echo 'main(){getpagesize();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_080_GETPAGESIZE_EXISTS'
	rm -f ${tf}*
fi

	echo 'main(){mlock();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_MLOCK_EXISTS'
	rm -f ${tf}*

	echo 'main(){seteuid();}' >${tf}.c
	cc -o $tf ${tf}.c >/dev/null 2>&1 &&
		echo '# define xCL_SETEUID_EXISTS'
	rm -f ${tf}*

#
# File inspection derived
#

case ${vers} in
    hp8_us5|\
    hpb_us5|\
    hp2_us5|\
    i64_hpu|\
    int_rpl|\
       *lnx)
	    ## wait() uses int for status
	    ## if wait() macros are defined, use those too.
	    [ -r /usr/include/sys/wait.h ] &&
		grep "WIFEXITED" /usr/include/sys/wait.h >/dev/null 2>&1 &&
		echo "# define xCL_USE_WAIT_MACROS"
		;;
    r64_us5|\
    rs4_us5)
	    echo "# define xCL_001_WAIT_UNION"
		;;
	  *)
	    [ -r /usr/include/sys/wait.h ] &&
		grep "union" /usr/include/sys/wait.h >/dev/null 2>&1 &&
		echo "# define xCL_001_WAIT_UNION"
		;;
esac


[ $vers != "hp8_us5" -a $vers != "hpb_us5" -a $vers != "hp2_us5" -a $vers != "i64_hpu" ] &&
		grep "union.*semun" /usr/include/sys/sem.h >/dev/null 2>&1 &&
	echo "# define xCL_002_SEMUN_EXISTS"

[ -r /usr/include/fcntl.h -o -r /usr/include/sys/fcntl.h ] &&
	echo "# define xCL_006_FCNTL_H_EXISTS"

[ -r /usr/include/sys/file.h ] &&
	echo "# define xCL_007_FILE_H_EXISTS"

    grep fd_set /usr/include/linux/types.h > /dev/null 2>&1 || \
	grep fd_set /usr/include/sys/types.h > /dev/null || \
        grep fd_set /usr/include/sys/select.h > /dev/null || \
        grep fd_set /usr/include/sys/time.h > /dev/null &&
	echo "# define xCL_014_FD_SET_TYPE_EXISTS"

    grep 'unsigned.*_ptr' /usr/include/stdio.h > /dev/null 2>/dev/null &&
	echo "# define xCL_017_UNSIGNED_IOBUF_PTR"

if [ "$vers" = "int_lnx" -o "$vers" = "ibm_lnx" -o \
   "$vers" = "i64_lnx" -o "$vers" = "a64_lnx" -o \
   "$vers" = "axp_lnx" -o "$vers" = "int_rpl" -o \
   "$vers" = "ppc_lnx" ]
then
        [ -r /usr/include/termio.h ] &&
                 echo "# define xCL_018_TERMIO_EXISTS"
elif [ "$vers" = "mg5_osx" -o "$vers" = "int_osx" -o "$vers" = 'ppc_osx' ]
then
	[ -r /usr/include/termios.h ] &&
		echo "# define xCL_TERMIOS_EXISTS"
elif [ "$vers" != hp8_us5 -a "$vers" != "hpb_us5" -a "$vers" != "hp2_us5" -a "$vers" != "i64_hpu" ]
then
	[ -r /usr/include/sys/termio.h -a ! -r /usr/include/sys/sgtty.h ] &&
		echo "# define xCL_018_TERMIO_EXISTS"
else
	[ -r /usr/include/sys/termio.h ] &&
		echo "# define xCL_018_TERMIO_EXISTS"
fi

    [ -r /usr/include/sys/time.h ] &&
	echo "# define xCL_015_SYS_TIME_H_EXISTS"

    [ -r /usr/include/netinet/in.h ] &&
        echo "# define xCL_019_TCPSOCKETS_EXIST"
    [ -r /usr/include/netinet/in.h ] &&
        grep 'sockaddr_in6' /usr/include/netinet/in*.h > /dev/null 2>&1 &&
	        echo "# define xCL_TCPSOCKETS_IPV6_EXIST"

[ -r /usr/include/sys/un.h ] &&
	echo "# define xCL_066_UNIX_DOMAIN_SOCKETS"

[ -r /usr/include/sys/select.h ] &&
	[ `eval grep SELLIST /usr/include/sys/select.h | wc -l` -gt 0 ] &&
	[ $vers != "r64_us5" -a $vers != "rs4_us5" ] &&
		echo "# define xCL_021_SELSTRUCT"

[ -r /usr/include/luxsna.h ] &&
	echo "# define xCL_022_AIX_LU6_2"
[ -d /usr/include/sna ] &&
	echo "# define xCL_HP_LU62_EXISTS"

[ -f /usr/include/dirent.h ] &&
	echo "# define xCL_025_DIRENT_EXISTS"

# Some HP systems
[ -f /usr/include/ndir.h ] &&
	echo "# define xCL_026_NDIR_H_EXISTS"

# Some Uniplus systems.
[ -f /usr/include/sys/ndir.h ] &&
	echo "# define xCL_027_SYS_NDIR_H_EXISTS"

# Matra (a/k/a Encore) xm7_us5
[ -f /usr/include/sys/aux/auxdir.h ] &&
	echo "# define xCL_028_SYS_AUX_AUXDIR_H_EXISTS"

# AIX: (Goes with SELLIST)
[ -f /usr/include/sys/select.h ] &&
	[ $vers != "r64_us5"  -a $vers != "rs4_us5" ] &&
	echo "# define xCL_031_SYS_SELECT_H_EXISTS"

# AIX: Some fcntls want FNDELAY, others O_NDELAY.
grep FNDELAY /usr/include/fcntl.h > /dev/null 2>&1 &&
	[ $vers != "r64_us5"  -o $vers != "rs4_us5" ] &&
	echo "# define xCL_032_FNDELAY_FCNTL_ARG"

# Excelan TCP/IP
[ -r /usr/include/exos/ex_errno.h ] &&
	echo "# define xCL_040_EXOS_EXISTS"

# DECnet
[ -r /usr/include/netdnet/dn.h ] &&
	echo "# define xCL_041_DECNET_EXISTS"

# for DY, ldfcn.h indicates WECO-style ld'ing
[ -r /usr/include/ldfcn.h ] &&
	echo "# define xCL_045_LDFCN_H_EXISTS"

# SYSV POLL
[ -r /usr/include/stropts.h ] &&
{
	echo "# define xCL_STROPTS_H_EXISTS"
	[ "$Select" = "any" ] && Select="poll"
}

# Now determine what they want
if [ "$Select" = "select" ]
then
	echo "# define xCL_020_SELECT_EXISTS"
fi
if [ "$Select" = "poll" ]
then
	echo "# define xCL_043_SYSV_POLL_EXISTS"
fi


# SYSV <unistd.h>
    [ -r /usr/include/unistd.h ] &&
	echo "# define xCL_069_UNISTD_H_EXISTS"

# SYSV <limits.h>
    [ -r /usr/include/limits.h ] &&
	echo "# define xCL_070_LIMITS_H_EXISTS"

# SYSV <ucontext.h>
[ -r /usr/include/ucontext.h ] &&
	echo "# define xCL_071_UCONTEXT_H_EXISTS"

# SYSV <siginfo.h>
[ -r /usr/include/siginfo.h ] &&
	echo "# define xCL_072_SIGINFO_H_EXISTS"

# TLI_TCP library
case "$vers" in
i64_hpu|hp2_us5|hpb_us5|hp8_us5) ;;
*)	    [ -r /usr/lib/libnsl_s.a ] && echo "# define xCL_TLI_TCP_EXISTS"
			;;
esac

# SYSV IPC
[ -r /usr/include/sys/ipc.h -a -r /usr/include/sys/sem.h ] &&
	echo "# define xCL_075_SYS_V_IPC_EXISTS"

# SYSv Streams
[ -f /dev/ing -o -r /dev/iistream ] &&
        echo "# define xCL_082_SYS_V_STREAMS_IPC"

# handling math exceptions via signals

mathsigs


# Determine TYPESIG setting. TYPESIG is also associated with sigvec in the CL.
# Some systems define *signal differently.

case $vers in
axp_osf | *_lnx | int_rpl | su4_us5 | su9_us5 | a64_sol)
		echo '# define TYPESIG void'
		;;
sgi_us5)        echo '# define TYPESIG __sigret_t'
		;;
*)              if grep '*signal' /usr/include/signal.h \
				  /usr/include/sys/signal.h | \
				  grep void > /dev/null
		then
			echo '# define TYPESIG void'
		else
			echo '# define TYPESIG int'
		fi
		;;
esac	

# SPARC platforms pass doubles (by value) as pairs of integers
# (aligned as integers) even though doubles normally require 8-byte alignment.

case $vers in
su4_us5|su9_us5) echo "# define xCL_085_PASS_DOUBLES_UNALIGNED"
		;;
esac	

# Some, in fact, most platforms (that I know about) have zero arguments
# passed to setpgrp(2). The default code right now uses two args,
# but we're making it easier to change it:
case $vers in
	hp8_us5|hpb_us5|su9_us5|su4_us5|usl_us5|\
	sgi_us5|*_lnx|int_rpl|\
	hp2_us5|i64_hpu|a64_sol)
		echo "# define xCL_086_SETPGRP_0_ARGS"
		;;
	*osx)
	  	# Definition has changed between releases
		spgtst=/tmp/sgptst$$
		cat << EOF > ${spgtst}.c
# include <stdlib.h>
# include <unistd.h>
int
main(int argc, char **argv )
{
    pid_t	pid;
    pid_t	pgrp;

    setpgrp( pid, pgrp );
    exit(0);
}
EOF
		if ( cc -o ${spgtst} ${spgtst}.c > /dev/null 2>&1 ) ; then
		    echo "# define xCL_087_SETPGRP_2_ARGS"
		else
		    echo "# define xCL_086_SETPGRP_0_ARGS"
		fi
		rm -f ${spgtst}*
		;;
*)
		echo "# define xCL_087_SETPGRP_2_ARGS"
		;;
esac	



#
# This figures out the printer command to use
#
if [ "$PRINTER_CMD" ]; then
	echo '#define PRINTER_CMD "$PRINTER_CMD"'
elif [ -f /usr/ucb/lpr ]; then
	echo '# define PRINTER_CMD "/usr/ucb/lpr"'
elif [ -f /usr/bin/lpr ]; then
	echo '# define PRINTER_CMD "/usr/bin/lpr"'
elif [ -f /bin/lpr ]; then
	echo '# define PRINTER_CMD "/bin/lpr"'
elif [ -f /usr/bin/lp ]; then
	echo '# define PRINTER_CMD "/usr/bin/lp"'
elif [ -f /bin/lp ]; then
	echo '# define PRINTER_CMD "/bin/lp"'
else
	# no lp on this build machine, use lp as specified by Posix.
	echo '# define PRINTER_CMD "/usr/bin/lp"  /* No lp here, using default */'
fi

#
# "/proc" fs?
#   Generate the define if the machine supports the "/proc" filesystem.  Assume
#   if "/usr/include/sys/procfs.h" exists then the machine does support the 
#   "/proc" filesystem.
if [ "$vers" != "axp_osf" -a "$vers" != "int_lnx" -a \
   "$vers" != "usl_us5" -a "$vers" != "sgi_us5" -a \
   "$vers" != "ibm_lnx" -a "$vers" != "axp_lnx" -a \
   "$vers" != "i64_lnx" -a "$vers" != "a64_lnx" -a \
   "$vers" != "a64_sol" -a "$vers" != "int_rpl" -a \
   "$vers" != "ppc_lnx" ]
then
	[ -r /usr/include/sys/procfs.h ] &&
		echo "# define xCL_PROCFS_EXISTS"
fi

#
# This test determines whether this sytem has the xCL_091_BAD_STDIO_FD
# problem where stdio fails on an fd even though the open_file limit
# is not exceeded. This is a bug in stdio implementations on many
# System V platforms, include SunOS.
#
case $vers in
a64_sol|\
su4_us5|\
su9_us5)	echo "# define xCL_091_BAD_STDIO_FD"
		;;
      *)	stdlim
		;;
esac

# Posix async. i/o is often provided via libaio.a. But it should only be
# enabled manually, per platform, subject to the following checks.
#	Before enabling xCL_ASYNC_IO the following should be checked
#	carefully:
#	o Is it supported for regular files as well as raw devices?
#	o Would other file operations - open(), close(), rename(),
#	  mknod(), creat() ...  block? (It is ok for these to block
#	  in a slave process not in a DBMS server).
#	o Are there sufficent file descriptors available in a DBMS to
#	  support the required number of GCA connections and async.
#	  disk i/o?
#	o and of course, does it actually perform better?
#
# async i/o exists in rs4_us5 and su4_us5, but needs more testing
# 
# POSIX Async I/O is no longer supported in Solaris 2.5, but there
# is support for Solaris Async I/O.  Man pages state "It is our
# intention to provide support for these interfaces in future
# releases."
# Solaris 2.6 supports POSIX Async I/O
#
case $vers in
i64_hpu|\
hpb_us5|\
hp2_us5)	echo "# define xCL_ASYNC_IO"
		;;
su4_us5)	if [ "$conf_B64" ]
                then
			echo "# define xCL_ASYNC_IO"
                fi
		;;
su9_us5)	echo "# define xCL_ASYNC_IO"
		;;
esac

# 
# AIX 4.1 fsync() will return EINVAL when the file is RAW character
# special file.
#
case $vers in
r64_us5|\
rs4_us5)	echo "# define xCL_092_NO_RAW_FSYNC"
		;;
esac

#
# The following defines determine how a server deals with standard
# library memory functions.
#
# xCL_093_MALLOC_OK -- Ingres on many Unix platforms cannot
# allow the standard "malloc()" function to be linked with the
# server due to incompatibilities with shared-memory allocation.
# If xCL_093_MALLOC_OK is not defined, libmalloc.a will be
# built with replacements for the system malloc/calloc/etc calls
# that use MEreqmem (using sbrk() or another method that does not
# conflict with shared memory.
#
# xCL_094_TLS_EXISTS -- For servers built with operating-system threads,
# private storage for each thread is allocated using thread-local
# storage (TLS).  For System V threads, these are the thr_getspecific(),
# etc. functions.  For POSIX threads, they are pthread_getspecific(),
# etc.  If a server cannot be built using the system TLS functions
# (usually because they depend on the system malloc()), replacement
# functions from the CL will be used unless xCL_094_TLS_EXISTS is defined.
case $vers in
su4_us5|\
su9_us5|\
a64_sol)        echo "# define xCL_093_MALLOC_OK"
                echo "# define xCL_094_TLS_EXISTS"
                ;;
i64_hpu|\
hpb_us5|\
hp2_us5)        echo "# define xCL_093_MALLOC_OK"
                ;;
hp8_us5)        echo "# define xCL_093_MALLOC_OK"
                ;;
r64_us5|\
rs4_us5)        echo "# define xCL_093_MALLOC_OK"
		echo "# define xCL_094_TLS_EXISTS"
                ;;
axp_osf)        echo "# define xCL_094_TLS_EXISTS"
                echo "# define xCL_093_MALLOC_OK" 
                ;;
usl_us5)        echo "# define xCL_093_MALLOC_OK"
                echo "# define xCL_094_TLS_EXISTS"
                ;;
sgi_us5)        echo "# define xCL_093_MALLOC_OK"
                echo "# define xCL_094_TLS_EXISTS"
                ;;
 *_lnx|int_rpl) echo "# define xCL_093_MALLOC_OK"
                echo "# define xCL_094_TLS_EXISTS"
                ;;
  *_osx)        echo "# define xCL_093_MALLOC_OK"
                echo "# define xCL_094_TLS_EXISTS"
                ;;
esac

#
# OS-threaded platforms need to reserve low file descriptors for
# use by standard i/o stream functions.
#
case $vers in
su9_us5|su4_us5|r64_us5|rs4_us5|axp_osf|hpb_us5|sgi_us5|hp2_us5|i64_hpu|a64_sol)
		echo "# define xCL_RESERVE_STREAM_FDS"
		;;
esac

#
# xCL_095_USE_LOCAL_NUMA_HEADERS -- Tru64 5.0 build adjustment
#
case $vers in
axp_osf)        if test -f /usr/include/numa.h
                then
                    :
		else
		    echo "# define xCL_095_USE_LOCAL_NUMA_HEADERS"
                fi
                ;;
esac

#
# xCL_NEED_RPCSVC_REX_HDR - Conditional inclusion of problematic header
# 			    file on Linux
#
case $vers in
    *_lnx|int_rpl)
	cfile=/tmp/rextst.$$.c
	ofile=/tmp/rextst.$$
	cat << EOF > ${cfile}
# include <termios.h>

int
main()
{
    /* tchars sometimes defined in rpcsvc/rex.h */
    struct tchars	tc;

    return(0);

}
EOF
	cc -o ${ofile} ${cfile} > /dev/null 2>&1
	[ -x ${ofile} ] || echo "# define xCL_NEED_RPCSVC_REX_HDR"
	rm -f ${ofile} ${cfile}
	;;
esac

#
# k
echo ""

)
echo "/* End of $header */"

trap : 0
