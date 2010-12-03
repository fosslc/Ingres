# ifndef VMS
/*
**  Copyright (c) 2004 Ingres Corporation
**
**  Name: rl.c -- adapted from rcheck.c to create a C version of syscheck.
**
**  Description:
**	With this adaptation, an attempt has been made to modularize the
**	original rcheck code, which is riddled with #ifdefs.  Eventually,
**	this code should be migrated into a CL module whose business it
**	is to check system resources.
**
**  Ancient History: (retained from rcheck.c)
**	Unknown (edg)
**		Created.
**	3-8-90 (Gladys Salgado)
**		Made some changes that are specific to SUN os 4.0
**		In SUN* 4.0 there is no limit to the shared memory
**		you can allocate but there is a limit on the size
**		of a shared memory segment.
**	4/23/90 (jkb)
**		Add include files and setdtablesize for sqs_us5 so
**		rcheck can be used to determine the number of file 
**		descriptors available on Sequent.
**	11-may-90 (huffman)
**		Mass integration from PD.
**	22-aug-1990 (jonb)
**		Use bzarch.h instead of compat.h to define the
**		configuration.
**	19-nov-1990 (rog)
**		Add hp3_us5 specific stuff.
**	01/24/91 (mikem/fredp)
**		Add setrlimit() call to determine the number of
**		of file descriptors available on SunOS 4.1 and
**		SystemVr4 systems.  Also, if on a Sun/SPARC machine
**		try to increase number of descriptors to the max.
**		supported on a SunDBE enhanced kernel.
**	03/07/91 (mikem)
**		The DBE code's call to setrusage() explicitly does
**		not check error returns as it is expected to fail on
**		non-DBE kernel systems. 
**	03-17-91 (szeng)
**		Integrate 6.3 porting changes for vax_ulx 
**		specific stuff.
**	03-17-91 (dchan)
**		Integrate 6.3 porting changes for ds3_ulx 
**		specific stuff.
**	03-28-91 (szeng)
**		Changed the two previous changes of 03-17-91
**		for RLIMIT_NOFILE to a more decent code style.
**	06-05-91 (fredv)
**		Integrate 6.3/02p porting changes for ris_us5.
**	07-09-91 (johnr)
**		Integrate hpux 8.0 porting changes for hp3_us5.
**		Removed unsupported RLIMIT stuff.
**	07-09-91 (johnr)
**		Replaced KERNEL definition except hp3_us5.
**	07-09-91 (johnr)
**		Corrected placement of KERNEL in last change.
**	10-21-91 (szeng)
**		Using correct formula to cacluate the number
**		of file descriptors for hp3.
**	08-nov-91 (gautam)
**		AIX 3.1 RS/6000 (ris_us5) values through nlist are
**		bogus. Put in official values obtained from IBM.
**	11-Nov-91 (wojtek)
**		Added dr6_us5 stuff, added additional semaphore
**		resource print for syscheck.
**	11-dec-91 (bentley)
**		For dr6_us5, dra_us5, added #include for ulimit.h 
**		to pick up correct ulimit() value.
**	09-dec-91 (dchan)
**		Experiment:  Ultrix kernel has been regen'ed to
**		use 1024 fd's.  So I had to ifdef'ed out the obsolete
**		code setting the number of files to _NFILE.
**	27-Feb-92 (sbull)
**		Added dr6_uv1 to list of machines
**	02-mar-92 (johnr)
**		Installed ds3_osf ifdefs for OSF/1.  
**		This includes defining
**		shminfo and seminfo to avoid include file entanglements.
**		Added one more to the list for the struct shminfo and 
**		seminfo.
**	29-apr-92 (purusho)
**		Integrate 6.2/02p changes to amd_us5
**	05-may-92 (pearl)
**		For nc5_us5: add definition of shminfo and seminfo;
**		it gets # of fd's from getdtablesize(), so don't
**		set it to _NFILE; kernel is /stand/unix.
**
** --- Ancient History of rcheck.c ends here ---
**
**  Modern History:
**	18-sep-92 (tyler)
**		Adapted from rcheck.c (now defunct) to be more manageable.
**	20-oct-92 (tyler)
**		undef'd _NFILE on sun_u42 and su4_u42 since it seemed to
**		break the file descriptor calculation.
**	11-jan-93 (tyler)
**		added RLexec() to returns values from "iisyslim", which
**		is used to parse the output of system utilities when
**		no C callable function is provided. 
**	15-jan-93 (tyler)
**		added call to get_rlimit() in RLprocFileDescHard() to
**		return hard limit on file descriptors.
**	29-mar-93 (sweeney)
**		Tack usl_us5 onto the other SVR4 cases. Use symbolic
**		constants for ulimit().
**	30-jan-93 (vijay)
**		Add a semicolon for a switch block which is ifdefed out for
**		aix.
**	 1-apr-93 (swm)
**		On axp_osf need define of _KERNEL (rather than KERNEL) to
**		pickup declaration of seminfo in <sys/sem.h>.
**		On axp_osf semmns (no. of semaphores in system) is not defined
**		in seminfo; instead we calculate the value by multiplying
**		semmni (no. of semaphore identifiers) by semmsl (no. of
**		semaphores per id).
**		Added axp_osf more to the list for the struct shminfo and 
**		seminfo.
**	14-jun-93 (lauraw)
**		Solaris changes: added su4_us5 cases to various other SVR4 
**		cases, and point namelist to /dev/ksyms.
**	14-jul-93 (tyler)
**		Eliminated pre-processor warning on FD_SETSIZE redefinition.
**	11-oct-93 (tomm)
**		Fix syscheck for hp8_us5 9.0.  Had to undef uchar to
**		prevent collision with system function with same name.
**		Used hp3 entry as springboard for hp8 entry.  Fixed 
**		STprintf %s -> %d since we're trying to print a number, not
**		a string.
**	13-oct-93 (tomm)
**		oops, accidently commented out my own #undef uchar. Fixed
**		that.
**	20-oct-93 (johnst)
**		For 64-bit axp_osf, define locate_kmem() address arg as 
**		unsigned long type, to match struct nlist.nvalue element type 
**		as defined in <nlist.h>, and avoid 64-to-32 bit truncation.
**	29-oct-93 (peterk)
**		on hp8_us5 pc.h needs clconfig.h to define 
**		xCL_069_UNISTD_H_EXISTS for correct declaration of getpid().
**		Note that mkming has been changed to add cl/clf/hdr 
**		to the include search path for front/st/util.
**	5-Dec-93 (seiwald)
**		Another random modification to this file that wants to be
**		both CL and non-CL: this time to get it to work on hp8_us5.
**		Remove inclusion of clsecret.h, since that's a known no-no 
**		for frontend code.  Removed inclusion of pc.h, since that's
**		what's missing the include to unistd.h.  Include unistd.h
**		directly for hp8_us5.
**	11-jan-94 (terjeb)
**		Corrected a problem for hp8_us5 in RLprocFileDescSoft(), as
**		this routine was returning the wrong value. Mistakently,
**		this routine would always return the value of _NFILE.
**	24-jan-94 (dianeh)
**		Removed references to Sun DBE stuff -- getting greater than
**		256 file descriptors was never a problem, only using them was.
**		So having a check for having gotten them was never necessary.
**		(Also fixed a typo and got rid of an obsolete config string.)
**	27-jan-94 (johnst)
**		Bug #58883.
**		Corrected a problem for axp_osf in RLprocFileDescSoft():
**		#undef obsolete _NFILE symbol, use getdtablesize() for soft
**              file limit.
**	22-feb-94 (tyler/fredv)
**		Fixed BUGS 51924 and 57714: syscheck always failed if
**		/dev/kmem was not readable.  Since it is not acceptable
**		to make /dev/kmem readable by ingres at all customer
**		sites, syscheck now reports that /dev/kmem can't be
**		opened but doesn't exit with an error as a result.
**		Fixed BUG 59842: report correct soft file descriptor
**		limit on all platforms.  The problem was that tabsize
**		was being set to _NFILE on all platforms, not just SunOS.
**	22-Feb-94 (ajc)
**		Added hp8_bls entries in line with hp8*
**      10-feb-95 (chech02)
**              Added rs4_us5 for AIX 4.1. 
**	17-mar-95 (nick)
**		Altered ris_us5 hardcoded values to latest - previous were
**		set for AIX 3.1 and have since increased.
**	24-mar-95 (smiba01)
**		Added entries for dr6_ues based on dr6_us5.
**      04-Apr-95 (nive/walro03/allmi01)
**              For dg8_us5, added own definition of shminfo and seminfo
**              structures with required fields.
**      20-Apr-95 (georgeb)
**              Added defines of _KMEMUSER and _KERNEL for usl_us5 in
**              order to pick up the correct sections from include files.
**              Also changed the method of getting memory resource values
**              as Unixware 2.0 does not support the nlist function.
**	01-may-95 (morayf)
**		Added odt_us5 changes based (loosely) on 6.4/05. A bit
**		tricky as some of the code structure has changed.
**      08-may-95 (THORO01)
**              Added check in RLcheck() to determine what version of HP/UX
**              a user is running since the config file was moved in HP 10.00.
**      15-jun-95 (popri01)
**              Enable nc5_us5 changes for nc4_us5
**      14-jul-95 (morayf)
**              Added sos_us5 in SOME places where odt_us5 tested for.
**              Hoewever, get/setrlimit() is in SCO OpenServer 5.0 so
**              use them not sysconf(). Get rid of unistd.h.
**      25-jul-95 (allmi01)
**              Added support for dgi_us5 (DG-UX on Intel platforms) following
**              dg8_us5.
**	16-aug-95 (rambe99)
**		Add changes for sqs_ptx from previous port.
**      18-aug-95 (pursch)
**              Add pym_us5 entries from 6.4/05.
**	16-nov-1995 (murf)
**		Added sui_us5 changes based upon su4_us5.
**	14-jul-95 (morayf)
**		Added sos_us5 in SOME places where odt_us5 tested for.
**		Hoewever, get/setrlimit() is in SCO OpenServer 5.0 so
**		use them not sysconf(). Get rid of unistd.h.
**	25-jul-95 (allmi01)
**		Added support for dgi_us5 (DG-UX on Intel platforms) following
**		dg8_us5.
**	14-dec-95 (morayf)
**		Added SNI RMx00 port (rmx_us5) Pyramid clone.
**		Moved embedded ')' from ulimit #if expression to end of
**		expression then added rmx_us5 to end of list.
**		Is this what dg8_us5, nc4_us5 and dgi_us5 need ? 
**		Added rmx_us5 to those needing unistd.h.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**		Commented out #define _KERNEL for axp-osf due to 2 decl for
**		seminfo.
**	26-feb-96 (morayf)
**		Made pym_us5 like rmx_us5 (SNI RMx00) for Pyramid port.
**	04-dec-96 (reijo01)
**		Updated some ii system calls with SystemCfgPrefix
**	28-feb-1997 (walro03
**		Updated for Tandem NonStop (ts2_us5).
**	30-may-1997 (muhpa01)
**		Don't include sys/systm.h for hp8_us5
**	20-jun-97 (popri01)
**		Like axp_osf, # of available semaphores in Unixware (usl_us5)
**		is now product of semmni and semmsl (semmns no longer used).
**	28-aug-97 (musro02)
**		add sqs_ptx to platforms needing:
**                  ulimit.h and unistd.h
**                  tabsize = (i4) sysconf( _SC_OPEN_MAX )
**                  static char *namelist = "/unix"
**      26-sep-1997 (mosj001)
**              SCO OpenServer restricts RLcheck (nlist) from reading
**              /stand/unix (must be root). nlist has been bypassed in
**              rl.c (find_symbol), thus 0 values for limit returned.
**              Game of charades to make syscheck run with OK results.
**              If we did not do this, ingbuild would be run in two parts
**              to allow user to set suid and owner=root for syscheck.
**	20-nov-1997 (walro03)
**		Don't include sys/systm.h for ris_us5.
**	21-nov-1997 (kosma01)
**	        Or for rs4_us5, don't include sys/systm.
**	24-feb-98 (toumi01)
**		Don't include sys/systm.h for Linux (lnx_us5).
**		Add Linux to definitions of seminfo and shminfo.
**	05-mar-98 (musro02)
**		add sqs_ptx to platforms not needing systm.h
** 	20-mar-1998 (fucch01)
**		Had added sgi_us5 support way back when, but forgot to add
**		comment about it.  Just had to update to include sgi_us5
**		among platforms using alternate symbols structure w/ shminfo
**		and seminfo.  Added sgi to platforms needing: 
**				    static char *namelist = "/unix"
**	02-Jul-98 (merja01)
**		Correct the way axp_osf reports total semaphores and
**		semaphore sets. Bug # 91807,
**	16-Jul-1998 (kosma01)
**		Compile errors because of an early end of comment, and
**		some # elseif instead of # elif. Also remove break 
**		statements causing many "statement can't be reached"
**		warnings from the switch(id) statement with all the
**		return statements in RLcheck.
**	24-Jul-1998 (hanch04)
**		Put ifdefs for axp_osf around locate_kmem64
**	29-jul-98 (muhpa01)
**		For HP-UX 11.00 32-bit port (hpb_us5), changed find_symbols()
**		to be able to read kernel symbol info from a 64-bit kernel.
**	03-Aug-1998 (kosma01)
**		locate_kmem64 really belongs to sgi_us5. sgi_us5
**		might have a 64 bit kernel, or a 32 bit kernel, check it
**		at runtime. Also have find_symbols64 call locate_kmem64,
**		so this actually works on sgi_us5.
**	03-sep-98 (toumi01)
**		Don't include nlist.h for Linux (lnx_us5).
**	03-nov-1998 (somsa01)
**	    Backed out change #434627.  (Bug #91887)
**	22-Feb-1999 (kosma01)
**		IRIX 6.5 and IRIX64 6.5 nolonger have the structure called
**		shminfo and seminfo. But the individual pieces of the requested
**		information are available in the kernel. Just not in a package,
**		like seminfo and shminfo.
**	10-may-1999 (walro03)
**		Remove obsolete version string amd_us5, dr6_ues, dr6_uv1,
**		dra_us5, ds3_osf, odt_us5, nc5_us5, sqs_us5, vax_ulx.
**      09-Jun-1999 (podni01)
**              For Siemens V5.44 rmx_us5 changed the way of getting shminfo
**              from /dev/mem instead of /dev/kmem(Siemens recommendations).
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	20-jul-1999 (popri01)
**		Don't include systm for Unixware (usl_us5).
**	23-Jul-1999 (merja01)
**              Backed out prior semaphore change for axp_osf.  Compaq
**              verified total semaphores on axp_osf (Digital UNIX/
**              Tru64 is semmni * semmsl.  issue 7395763 bug 91807 
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**              Correct rux_us5 changes that were applied using the rmx_us5
**              label.
**      25-Apr-2000 (wansh01)
**              Changed semaphore typedef for sgi_us5.
**      May-26-2000 (hweho01)
**              Added support for AIX 64-bit port (ris_u64).
**              Set AIX_SHM_MAXSIZE to 2GB. The hardcoded value is based
**              on AIX OS release 4.3.1. The shared memory segment size
**              and semaphore are not configurable, they are dynamically
**              allocated and deallocated up to the upper limits. 
**	14-Jun-2000 (hanje04)
**		Don't include sys/systm.h or nlist.h for ibm_lnx and add
**		definitions for seminfo and shminfo
**	08-Sep-2000 (hanje04)
**		For Alpha Linux, (axp_lnx) don't inlcude nlist.h or
**		sys/systm.h and seminfo and shminfo
**      20-Sep-2000 (hanal04) Bug 102684
**              Ensure the structures shminfo and seminfo have been decalred
**              before declaring variables of that type. sgi_us5.
**	Oct-10-2000 (ahaal01)
**		Set AIX_SHM_MAXSIZE to 2GB for AIX 32-bit port (rs4_us5).
**      13-oct-2000 (mosjo01)
**              somehow sgi_us5 string got mixed in with rux_us5 from 
**              prior changes causing compile errors. sgi_us5 string removed
**              wherever rux_us5 appeared.
**      07-Nov-2000 (hweho01)
**              Added new data structure to support OS rel 5.0 or above for 
**              axp_osf platform, since the size of shmmax and shmmin in 
**              shminfo has been changed from 4-byte to 8-byte.  
**	13-Nov-2000 (hanch04)
**	    Syscheck fails to read a 64 bit kernel.
**	    Check to see if the kernel is 32 or 64 bit.  We cannot read a
**	    64 bit kernel in a 32 bit build.  Found this command running
**	    "truss isainfo -b"
**      30-jan-2001 (bolke01) Bug 103098
**              Somehow su4_us5 string  in combination with _NFILE forces all
**		tabsize to use _NFILE.  Put new exclusion in for sqs_ptx.
**	21-May-2001 (hanje04)
**		Added missing \ to ifdef statment caused by error when 
**		Xing change 447373.
**	30-jul-2001 (toumi01)
**		Add support for AIX Itanium port (i64_aix) modeled after
**		ris_us5.
**	25-oct-2001 (somsa01)
**		Corrected bad #if statement.
**	26-oct-2001 (toumi01)
**		fix #if statement correction
**	08-May-2002 (hanje04)
**	 	Added support for Itanium Linux (i64_lnx)
**	10-Sep-2002 (hanch04)
**		Moved check for 64-bit Solaris kernal into syscheck.c
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	07-May-2003 (hanch04)
**	    syscheck will not exit FAIL if it thinks that the system does not
**	    have sufficient resources to run Ingres.  It will exit OK.
**	    The [ -f ] flag will force syscheck to revert to the old behavior 
**	    and return fail if the system does not have sufficient resources 
**	    to run Ingres. 
**      24-Dec-2003 (zhahu02)
**          Update for 4G SHMMAX (INGSRV2544) .
**	30-Jan-2004 (hanje04)
**	    Replace uint64_t with i8_u because uint64_t is not defined
**	    on all platforms in CVuint64la().
**       2-Mar-2004 (hanal04) Bug 111888
**          Previous change breaks axp_osf mark 2637 build. Added ifdefs
**          for CVuint64la() parameter.
**	08-Mar-2004 (hanje04)
**	    SIR 107679
**	    Improve syscheck functionality for Linux:
**	    Define more descriptive error codes so that ingstart can make a 
**	    more informed decision about whether to continue or not. (generic)
**	    Extend linux functionality to check shared memory values.
**	    Add -c flag and the ability to write out kernel parameters needed
**	    by the system in order to run Ingres.
**	    NOTE: This is really not a complete solution, the whole syscheck
**	    / rl utility needs rewriting as it is very difficult to follow
**	    and rather out of date.
**	05-Apr-2004 (xu$we01) 
**	    Corrected the problem of the wrong value returned for the
**	    the system soft file limit on SCO OpenServer(bug 112094).	
**      06-Jul-2004 (zhahu02)
**         Update for 4G SHMMAX (INGSRV2544), correcting a typo. 
**	18-Aug-2004 (hanje04)
**	    BUG 112844
**	    Fail out if LOuniq returns an error which it will now if mktemp()
**	    returns an error.
**	08-sep-2004 (abbjo03)
**	    Reorder history comments chronologically because we had two
**	    different sequences.
**	07-Oct-2004 (drivi01)
**	    Changed "rl.h" to <rl.h> reference and moved header into front/st/hdr.
**	10-Nov-2004 (bonro01)
**	    Fix syscheck to correctly detect used shared memory
**	    Lookup shmmni in Solaris based on OS level.
**	12-Jan-2004 (hanje04)
**	    Make len in find_symbols() SIZE_TYPE not int.
**	15-Mar-2005 (bonro01)
**	    Add support for Solaris AMD64 a64_sol
**	06-May-2005 (hanje04)
**	    Add support for Mac OS X. Used Linux as template as same sysctl
**	    interface is available. Semaphore limits had to be queried 
**	    individually as there doesn't seem to be a way to obtain the
**	    entire seminfo structure in one call to sysctl().
**	15-May-2005 (bonro01)
**	    Solaris 10 GA code removed shminfo and seminfo structures.
**	    a64_sol now uses Resource Control values to check resources.
**	    su9_us5 build must now check for resources differently on
**	    Solaris 8, 9 and 10.
**	    Use Resource Control values on Solaris 10.
**	    Solaris 8 does not support Resource Controls, and since
**	    syscheck is currently built on Solaris 8, these
**	    functions must be dynamically resolved when run on Solaris 10.
**	24-Aug-2005 (schka24)
**	    Extend Bob's a64-sol fixes to SPARC so that we can build
**	    and run on Solaris 10.  Map shm, sem stuff to the same generic
**	    info structs that a64 uses.
**	29-Aug-2005 (hweho01)
**	    Fix syscheck error and update the hard limits for shared memory  
**          and semaphores on AIX platform.
**	14-Nov-2006 (bonro01)
**	    shminfo values on Linux may be 32-bit or 64-bit depending on
**	    the kernel and a 32-bit syscheck would have difficulty retrieving
**	    a 64-bit kernel value.  shmmax and shmmni can be retrieved from
**	    /proc/sys/kernel regardless of kernel or executable type.
**	9-Apr-2007 (bonro01)
**	    sysctl() fails on newer linux kernels, use /proc/sys/kernel/sem
**	    instead.
**	11-May-2007 (kibro01) b118085
**	    syscheck should return correct soft limit of FDs on a64_sol
**	24-Oct-2007 (hanje04)
**	    BUG 114907
**	    Use same _shminfo definition on OSX as for Linux as sysctl returns
**	    values in similar form.
**      06-Dec-2007 (smeke01) b119577
**          On i64_hpu fix syscheck to correctly display shmmax/shmmin 
**          values above 2GB. 
**	07-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update OS X bit for Leopard. Switch to using sysctlbyname() as
**	    old defines have been deprecated in Leopard
**	12-Mar-2008 (hweho01) B120103
**	    Added support for AIX 6.1, include the shared memory and    
**          semaphore limits in the shminfo and seminfo list.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of unistd.h to eliminate gcc 4.3 warnings.
**	25-Mar-2010 (hanje04)
**	    SIR 123296
**	    Use full paths (via NMloc) when calling iisyslim so they can
**	    can be found for LSB installation without being in the path.
**	12-Oct-2010 (hanje04)
**	    Bug 124611
**	    Fix up find_symbols() for Linux to stop SEGVs when /proc isn't
**	    mounted.
**
*/


# include 	<compat.h>

#if defined (usl_us5)
#define _KMEMUSER
#endif

# include	<er.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<cv.h>
# include	<st.h>
# include 	<gl.h>
# include	<nm.h>
# include 	<unistd.h>

#if defined(any_hpux) || defined (hp8_bls)
/* hp8 has a function called uchar() in  */
/* /usr/include/sys/systmn.h */

# include       <sys/utsname.h>    /* RMT include uname() structure */

#undef uchar			
#endif

# ifdef LNX
# include <sys/sysctl.h>
# endif

# ifdef OSX
# include <sys/sysctl.h>
# include <sys/types.h>
# include <sys/sem.h>
# endif

# include	<cm.h>
# include	<pc.h>

# include	<rl.h>

# define	TEMP_DIR	ERx( "/tmp/" )

GLOBALDEF STATUS exitvalue = OK;  

/*
**  Quiet compiler about redefined macros we don't use.
*/
# undef	max
# undef	min

/* For dg8_us5 there are no shminfo and seminfo structures.  Therefore, we
* define our own with only the fields that we need.  Later, we will use the
* sysconf() call to fill those fields.
*/

#if defined(dg8_us5) || defined(dgi_us5)

#include <unistd.h>

struct shminfo
{
    unsigned long shmmax;
    unsigned long shmmni;
};

struct seminfo
{
    unsigned long semmns;
    unsigned long semmni;
};

#endif /* dg8_us5 dgi_us5 */

# include   <sys/stat.h>
# include   <sys/errno.h>
# include 	<sys/time.h>

# if defined(hp3_us5) || defined(any_hpux) || defined(hp8_bls) || \
     defined(nc4_us5) || defined(rmx_us5) || defined(rux_us5) || \
     defined(pym_us5) || defined(sqs_ptx) || defined(ts2_us5)

# include	<ulimit.h>
# include	<unistd.h>

# endif /* hp3_us5, hpux */

# if defined(sgi_us5)
# include	<limits.h>
# include	<unistd.h>
# endif /* sgi_us5 */

# if defined(sparc_sol)
# include       <sys/types.h>
# include	<unistd.h>
# include	<dlfcn.h>
# include       <sys/utsname.h>
struct utsname  su4_uname;
# endif /* sparc */

# if defined(a64_sol)
# include       <sys/types.h>
# include	<unistd.h>
# include	<rctl.h>
# endif /* a64_sol */

# if defined(thr_hpux)
# include       <sys/types.h>
# include       <sys/ksym.h>
# include       <libelf.h>
# endif

# if !(defined(hp3_us5) || defined(sgi_us5))
# define KERNEL

# include <sys/ipc.h>

# endif /* hp3_us5 */

# include 	<sys/resource.h>
# ifndef LNX
# include	<nlist.h>
# endif /* Linux */

# if defined(ds3_ulx)

# include	<sys/smp_lock.h>

# endif /* ds3_ulx */

# if defined(any_aix)
# include   <sys/utsname.h>
# include   <sys/systemcfg.h>
# include   <inttypes.h>
struct utsname  aix_uname;
# endif  /* aix  */

/*
** Need define of _KERNEL (rather than KERNEL) on axp_osf to pickup
** declaration of seminfo in <sys/sem.h>
*/
# ifdef axp_osf
# include       <sys/utsname.h>
struct utsname  axp_uname;
                                /* Currently, Ingres is built on 4.0D, added */
                                /* shminfoV5 structure, for rel. 5.x support.*/
static struct  _shminfoV5 {
        size_t  shmmax,         /* max shared memory segment size      */
                shmmin;         /* min shared memory segment size      */
        int     shmmni,         /* number of shared memory identifiers */
                shmseg;         /* max attached segments per process   */
} shminfoV5;

# include 	<sys/shm.h>
/*# define _KERNEL */
# include 	<sys/sem.h>

# else /* axp_osf */

# if defined(usl_us5)
# define _KERNEL
# endif /* usl_us5 */

# ifdef ts2_us5
/*
**  For Tandem NonStop (ts2_us5), we can get the shminfo and seminfo structures
**  from sys/shm.h and sys.sem.h, respectively, if we define _KERNEL first.
**  However, rl.c then gets numerous syntax errors in sys/ksynch.h.  So instead
**  we just insert the two structures here.
*/
struct		shminfo {
	int	shmmax,		/* max shared memory segment size */
		shmmin,		/* min shared memory segment size */
		shmmni,		/* # of shared memory identifiers */
		shmseg;		/* max attached shared memory     */
				/* segments per process           */
};
/*
 * Semaphore information structure.
 * 	For SVR4.2, the semmap, semmnu, and semusz fields
 *	of the seminfo structure are not used.  This is
 *	because everything is dynamically allocated.
 */
struct		seminfo {
	int	semmap;		/* XXX # of entries in semaphore map */
	int	semmni;		/* # of semaphore identifiers */
	int	semmns;		/* # of semaphores in system */
	int	semmnu;		/* XXX # of undeo structures in system */
	int	semmsl;		/* max # of semaphores per id */
	int	semopm;		/* max # of operations per semop call */
	int	semume;		/* max # of undo entries per process */
	int	semusz;		/* XXX size in bytes of undo structure */
	int	semvmx;		/* semaphore maximum value */
	int	semaem;		/* adust on exit max value */
};

# elif defined(sparc_sol)

struct  shminfo8 {
        size_t  shmmax,         /* max shared memory segment size */
                shmmin;         /* min shared memory segment size */
        int     shmmni,         /* # of shared memory identifiers */
                shmseg;         /* max attached shared memory     */
                                /* segments per process           */
} shminfo8;
struct  shminfo9 {
        size_t  shmmax;         /* max shared memory segment size */
        int     shmmni;         /* # of shared memory identifiers */
} shminfo9;

/* seminfo from Solaris 8 or 9 */
struct seminfo89 {
	int	semmni;		/* # of semaphore identifiers */
	int	semmns;		/* # of semaphores in system */
	int	semmnu;		/* # of undo structures in system */
	int	semmsl;		/* max # of semaphores per id */
	int	semopm;		/* max # of operations per semop call */
	int	semume;		/* max # of undo entries per process */
	int	semusz;		/* size in bytes of undo structure */
	int	semvmx;		/* semaphore maximum value */
	int	semaem;		/* adjust on exit max value */
};


/*
 * On Solaris SPARC and intel and AMD starting at Solaris 10
 * the shminfo and seminfo structures are no longer available
 * in the kernel.  In Solaris 10, the fields are available using the
 * resource control functions.
 */
struct  shminfo10 {
        i8	shmmax,         /* max shared memory segment size */
		shmmni;         /* # of shared memory identifiers */
} shminfo10;
struct	seminfo10 {
	i8	semmni,		/* # of semaphore identifiers */
		semmns,		/* # of semaphores in system */
		semmsl;		/* max # of semaphores per id */
} seminfo10;

/* Define our local flavor of shminfo, seminfo */
struct  shminfo {
        i8	shmmax,         /* max shared memory segment size */
		shmmni;         /* # of shared memory identifiers */
};
struct	seminfo {
	i8	semmni,		/* # of semaphore identifiers */
		semmns,		/* # of semaphores in system */
		semmsl;		/* max # of semaphores per id */
};

# elif defined(a64_sol)

/*
 * On Solaris SPARC and intel and AMD starting at Solaris 10
 * the shminfo and seminfo structures are no longer available
 * in the kernel.  In Solaris 10, the fields are available using the
 * resource control functions.
 * a64_sol is Solaris 10-only.
 */
struct  shminfo {
        i8	shmmax,         /* max shared memory segment size */
		shmmni;         /* # of shared memory identifiers */
};
struct	seminfo {
	i8	semmni,		/* # of semaphore identifiers */
		semmns,		/* # of semaphores in system */
		semmsl;		/* max # of semaphores per id */
};
# else
# include 	<sys/shm.h>
# include 	<sys/sem.h>
# endif /* ts2_us5 */

# endif /* axp_osf */

# if !(defined(ds3_ulx) || defined(hp3_us5) ||  defined(any_aix) || \
	defined(any_hpux) || defined (sqs_ptx) || \
        defined(sgi_us5) || defined(usl_us5) || \
	defined(LNX) || defined(OSX))

# include 	<sys/systm.h>

# endif /* ds3_ulx / hp3_us5 */

# include	<fcntl.h>

# if defined(hp3_us5) \
	|| defined(ds3_ulx) || defined(any_aix) \
	|| defined(dr6_us5) || defined(usl_us5) \
	|| defined(axp_osf) || defined(sparc_sol) || defined(any_hpux) \
	|| defined(hp8_bls) || defined(dg8_us5) \
        || defined(nc4_us5) || defined(a64_sol) \
        || defined(dgi_us5) || defined(sos_us5) || defined(pym_us5) \
	|| defined(sqs_ptx) || defined(sui_us5) || defined(rmx_us5) \
	|| defined(rux_us5) || defined(ts2_us5) 

static struct shminfo shminfo;
static struct seminfo seminfo;

# endif /* hp3_us5 / ds3_ulx / dr6_us5 ... */

# if defined(thr_hpux)
struct _shminfo_64
        {
                uint64_t        shmmax64;
                uint64_t        shmmin64;
                int             shmmni64;
                int             shmseg64;
        }; 
static struct _shminfo_64 shminfo_64; 
# endif /* newer hpux */

# if defined(sgi_us5)

struct _shminfo64 {
	long 	sshmseg;
	long 	shmmni;
	long long	shmmin;
	long long	shmmax;
} ;

struct _shminfo {
	long 	sshmseg;
	long 	shmmni;
	long 	shmmin;
	long 	shmmax;
} ;

struct _seminfo {
	long 	semaem;
	long	semvmx;
	long	semopm;
	long	semmsl;
	long	semmni;
	long	semmns;
} ;

static struct _shminfo64 shminfo64;
static struct _shminfo   shminfo;
static struct _seminfo   seminfo;

# endif /* sgi_us5 */

#if defined(LNX)

/* _shminfo defined in rl.h */
GLOBALDEF struct _shminfo shminfo;

struct _seminfo {
	i8 semmsl;
	i8 semmns;
	i8 semopm;
	i8 semmni;
} ;

static struct _seminfo seminfo;

#endif

#if defined(OSX)

/* _shminfo defined in rl.h */
GLOBALDEF struct _shminfo shminfo;

struct _seminfo {
	int semmsl;
	int semmns;
	int semopm;
	int semmni;
} ;

static struct _seminfo seminfo;

#endif

# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) || \
     defined(sos_us5) || defined(sui_us5)

# include     <ulimit.h>

# endif /* dr6_us5 / usl_us5 / sui_us5 */

# if defined(any_aix)
/*
** FIXME: In future, if the build is made on AIX 5.2, the following data  
**        can be retrieved by using vmgetinfo() with parm IPC_LIMITS. 
*/
typedef struct _aix_shminfo AIX_SHMINFO;
struct _aix_shminfo
{
  char  version[4];               /*  OS version                       */ 
  char  release[4];               /*  OS release                       */ 
  i8    shmmax;                   /*  32-bit process                   */ 
  i8    shmmax_p64;               /*  64-bit process for 32-bit kernel in 5.x.  
                                  **  This will not be used in 6.1,   
                                  **  because 32-bit kernel is deprecated.        
                                  */
  i8    shmmax_p64_k64;           /*  64-bit process for 64-bit kernel */ 
}; 
AIX_SHMINFO  aix_shminfo[] =
{
{"5","1",2147483648,  (INT64_C(68719476736)),   (INT64_C(68719476736))}, /*5.1*/
{"5","2",2147483648,(INT64_C(1099511627776)), (INT64_C(1099511627776))}, /*5.2*/
{"5","3",2147483648,(INT64_C(1099511627776)),(INT64_C(35184372088832))}, /*5.3*/ 
{"6","1",3489660928,(INT64_C(0)),            (INT64_C(35184372088832))}, /*6.1*/ 
{NULL, NULL,      0,                       0,                       0 } 
};

# define AIX_SHM_MAXSEG	        	131072
# define AIX_SHM_MAXSEG_SINCE_REL53_K64 1048576
# define AIX_SEM_MAXSEMID              	 65535
# define AIX_SEM_MAXSET	        	131072
# define AIX_SEM_MAXSET_SINCE_REL53_K64 1048576

# endif /* aix */

# if defined(sparc_sol) || defined(axp_osf) || defined (a64_sol)

# undef _NFILE

# endif /* sol / axp_osf */

# define BIG_ENOUGH 500

FUNC_EXTERN
VOID
CVuint64la(i, a)
register u_i8 i;
register char   *a;
{
        register char   *j;
        char            b[12];

        j = &b[11];
        *j-- = 0;

        do
        {
                *j-- = i % 10 + '0' ;
                i /= 10;
        } while (i);

        do
        {
                *a++ = *++j;
        } while (*j);
        return;
}

/*
** Executes the specified system-level command and returns the first line
** of output it produces.
*/
static STATUS 
RLexec( char *command, char *result )
{
	LOCATION fileloc;
	char *locpath, *host, *c;
	char tempbuf[ MAX_LOC + 1 ];
	char pathbuf[ MAX_LOC + 1 ];
	char cmdbuf[ BIG_ENOUGH + 1 ];
	CL_ERR_DESC cl_err;
	FILE *fp;

	/* prepare temporary file LOCATION */
	STcopy( ERx( "" ), tempbuf );
	LOfroms( FILENAME, tempbuf, &fileloc );
	/* If we can't get a uniq filename, return(FAIL) */
	if ( LOuniq( ERx( "IICR" ), NULL, &fileloc ) )
	    return(FAIL);
	LOtos( &fileloc, &locpath );
	STcopy( TEMP_DIR, pathbuf );
	STcat( pathbuf, locpath );
	LOfroms( PATH & FILENAME, pathbuf, &fileloc );

	/* prepare cmdbuf */
 	STprintf( cmdbuf, "%s %s", command, pathbuf );

	/* execute command - assume it doesn't fail */ 
	if( PCcmdline( NULL, cmdbuf, TRUE, NULL, &cl_err ) != OK )
		return( FAIL );

	/* extract the return value from first line of temporary file */
	if( SIfopen( &fileloc , ERx("r"), SI_TXT, BIG_ENOUGH, &fp ) != OK )
		return( FAIL );
	*result = '\0';
	SIgetrec( result, BIG_ENOUGH, fp );
	for( c = result; *c != EOS && *c != '\n'; CMnext( c ) );
	*c = EOS;
	SIclose( fp );
	LOdelete( &fileloc );

	return( OK );
}

/*
** Returns the maximum number of file descriptors per process.
*/
static STATUS
RLprocFileDescHard( char *buf )
{

# ifdef RLIMIT_NOFILE

	struct rlimit res;

	if( getrlimit( RLIMIT_NOFILE, &res ) == 0 ) 
	{
		/* rlim_t is a quad_t on OSX */
		STprintf( buf, "%d", (int)res.rlim_max );
		return( OK );
	}

# endif /* RLIMIT_NOFILE */

	return( FAIL );
}

/*
** Returns the number of file descriptors available to the current process. 
*/
static STATUS
RLprocFileDescSoft( char *buf )
{
	i4 tabsize;
	struct rlimit res;

# if defined( hp3_us5) || defined(any_hpux) || defined(hp8_bls) \
        || defined(dg8_us5) || defined(dgi_us5) \
	|| defined(rmx_us5) || defined(pym_us5) || defined(sqs_ptx) \
	|| defined(ts2_us5) || defined(rux_us5)

	tabsize = (i4) sysconf( _SC_OPEN_MAX );

# else /* hp3_us5 */

# ifdef RLIMIT_NOFILE 

	/* 
	** Try to increase the number of available descriptors on 
	** SunOS 4.1 or SystemVR4 systems.
	*/
	if( getrlimit( RLIMIT_NOFILE, &res ) == 0 ) 
	{
	    if( res.rlim_cur < res.rlim_max ) 
	    {
		res.rlim_cur = res.rlim_max;
		_VOID_ setrlimit( RLIMIT_NOFILE, &res );
	    }
	}

# endif /* RLIMIT_NOFILE */

# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) || \
     defined(nc4_us5) || defined(sui_us5)

        tabsize = (i4) ulimit( UL_GDESLIM, 0L );

# else /* dr6_us5 || usl_us5 || sui_us5 */

	tabsize = getdtablesize();

# endif /* dr6_u5/usl_us5/sui_us5 */

# endif /* hp3_us5 */

# if !(defined(hp3_us5) || defined(ds3_ulx) || \
      defined(any_hpux) || defined(hp8_bls)  || \
      defined(dg8_us5) || defined(nc4_us5) || defined(dgi_us5)  || \
      defined(rmx_us5) || defined(pym_us5) || defined(ts2_us5)  || \
      defined(rux_us5))

# if defined(dr6_us5) || defined(usl_us5) || defined(sui_us5)

        tabsize = (i4) ulimit( UL_GDESLIM, 0L );

# else /* dr6_us5 / usl_us5 / sui_us5 */ 

# ifdef _NFILE

# if !defined(sparc_sol) && !defined(sos_us5) && !defined(sqs_ptx)

	tabsize = _NFILE;

# endif

# else /* _NFILE */

	tabsize = getdtablesize();

# endif /* _NFILE */

# endif /* dr6_us5 */

# endif /* hp3_us5 / ds3_ulx / nc4_us5 */

	STprintf( buf, ERx( "%d" ), tabsize ); 
	return( OK );
}

/*
** Returns a variety of hard limits.
*/
static VOID
GetHardLimit( i4  limit, char *buf )
{

# if defined(hp3_us5) || defined(any_hpux) || defined(hp8_bls)

	/*
        **  For the hp 9000/300 machines ulimit returns the limit
        **  512 byte blocks.   I'm going to leave the output in blocks
        **  because if the limit is high it could overflow a long
        */
        if (limit == UL_GETFSIZE)
        {
         	STprintf( buf, ERx( "NONE" ) );
                return;
        }

# else /* hp3_us5 */

	struct rlimit limits;

	getrlimit( limit, &limits );

	if (limits.rlim_max == RLIM_INFINITY)
		STprintf( buf, ERx( "NONE" ) );
	else
		STprintf( buf, ERx( "%d" ), limits.rlim_max );

# endif /* hp3_us5 */
}

/*
** Returns a variety of soft limits.
*/
static VOID
GetSoftLimit( i4  limit, char *buf )
{

# if defined(hp3_us5) || defined(any_hpux) || defined(hp8_bls)

	/*
        **  For the hp 9000/300 machines ulimit returns the limit
        **  512 byte blocks.   I'm going to leave the output in blocks
        **  because if the limit is high it could overflow a long
        */
        if (limit == UL_GETFSIZE)
        {
               	STprintf( buf, ERx( "%d" ), 512 * ulimit( UL_GETFSIZE, 0l ) );
                return;
        }

# else /* hp3_us5 */

	struct rlimit limits;

	getrlimit( limit, &limits );

	if (limits.rlim_cur == RLIM_INFINITY)
		STprintf( buf, ERx( "%s" ), ERx( "NONE" ) );
	else
		STprintf( buf, ERx( "%d" ), limits.rlim_cur );

# endif /* hp3_us5 */
}

/*
** Returns the maximum file size.
*/
static STATUS
RLprocFileSizeHard( char *buf )
{
# if !defined(hp3_us5) && !defined(any_hpux) && !defined(hp8_bls)

	GetHardLimit( (i4) RLIMIT_FSIZE , buf );

# else /* hp3_us5 */

	GetHardLimit( (i4) UL_GETFSIZE , buf );

# endif /* hp3_us5 */

	return( OK );
}

/*
** Returns the current limit on file size.
*/
static STATUS
RLprocFileSizeSoft( char *buf )
{
# if !defined(hp3_us5) && !defined(any_hpux) && !defined(hp8_bls)

	GetSoftLimit( (i4) RLIMIT_FSIZE , buf );

# else /* hp3_us5 */

	GetSoftLimit( (i4) UL_GETFSIZE , buf );

# endif /* hp3_us5 */

	return( OK );
}

/*
** Returns the maximum resident set size.
*/
static STATUS
RLprocResSetHard( char *buf )
{
# if !defined(hp3_us5) && defined(RLIMIT_RSS) && !defined(any_hpux) && \
     !defined(hp8_bls)

	GetHardLimit( (i4) RLIMIT_RSS , buf );
	return( OK );

# else /* hp3_us5 / RLIMIT_RSS */

	return( FAIL );

# endif /* hp3_us5 / RLIMIT_RSS */
}

/*
** Returns the current limit on the resident set size.
*/
static STATUS
RLprocResSetSoft( char *buf )
{
# if !defined(hp3_us5) && !defined(any_hpux) && defined(RLIMIT_RSS) && \
     !defined(hp8_bls)

	GetSoftLimit( (i4) RLIMIT_RSS , buf );
	return( OK );

# else /* hp3_us5 / RLIMIT_RSS */

	return( FAIL );

# endif /* hp3_us5 / RLIMIT_RSS */
}

/*
** Returns the maximum CPU time limit.
*/
static STATUS
RLprocCPUtimeHard( buf )
char *buf;
{
# if !defined(hp3_us5) && !defined(any_hpux) && !defined(hp8_bls)

	GetHardLimit( (i4) RLIMIT_CPU , buf );
	return( OK );

# else /* hp3_us5 */

	return( FAIL );

# endif /* hp3_us5 */
}

/*
** Returns the current CPU time limit.
*/
static STATUS
RLprocCPUtimeSoft( buf )
char *buf;
{
# if !defined(hp3_us5)  && !defined(any_hpux) && !defined(hp8_bls)

	GetSoftLimit( (i4) RLIMIT_CPU , buf );
	return( OK );

# else /* hp3_us5 */

	return( FAIL );

# endif /* hp3_us5 */
}

/*
** Returns the maximum data segment size.
*/
static STATUS
RLprocDataSegHard( buf )
char *buf;
{
# if !defined(hp3_us5) && !defined(any_hpux) && !defined(hp8_bls)

	GetHardLimit( (i4) RLIMIT_DATA , buf );
	return( OK );

# else /* hp3_us5 */

	return( FAIL );

# endif /* hp3_us5 */
}

/*
** Returns the current limit on the size of the data segment.
*/
static STATUS
RLprocDataSegSoft( buf )
char *buf;
{
# if !defined(hp3_us5) && !defined(any_hpux) && !defined(hp8_bls)

	GetSoftLimit( (i4) RLIMIT_DATA , buf );
	return( OK );

# else /* hp3_us5 */

	return( FAIL );

# endif /* hp3_us5 */
}

/*
** Returns the maximum stack segment size.
*/
static STATUS
RLprocStackSegHard( buf )
char *buf;
{
# if !defined(hp3_us5) && !defined(any_hpux) && !defined(hp8_bls)

	GetHardLimit( (i4) RLIMIT_STACK , buf );
	return( OK );

# else /* hp3_us5 */

	return( FAIL );

# endif /* hp3_us5 */
}

/*
** Returns the current limit on the size of the stack segment.
*/
static STATUS
RLprocStackSegSoft( buf )
char *buf;
{
# if !defined(hp3_us5)  && !defined(any_hpux) && !defined(hp8_bls)

	GetSoftLimit( (i4) RLIMIT_STACK , buf );
	return( OK );

# else /* hp3_us5 */

	return( FAIL );

# endif /* hp3_us5 */
}

/*
** The following data structure are used for reading kernel memory files. 
*/

# if defined(dg8_us5) || defined(dgi_us5)
static i4  kernel = -1;
static bool kmem_no_read = FALSE;
static STATUS
open_kmem( i4  argc, char **argv)
{
}
/* Use sysconf() to return the values to fill our seminfo and shminfo
 * structures.  sysconf returns a -1 on error.
*/
find_symbols()
{
    if ((shminfo.shmmax = sysconf(_SC_SHMMAXSZ)) == -1)
    {
        printf("Can't get system total shared memory.\n");
        exit (1);
    }
    if ((shminfo.shmmni = sysconf(_SC_SHMSEGS)) == -1)
    {
        printf("Can't get maximum number of shared memory segments.\n");
        exit (1);
    }
    if ((seminfo.semmns = sysconf(_SC_NMSYSSEM)) == -1)
    {
        printf("Can't get system total semaphores.\n");
        exit (1);
    }
    if ((seminfo.semmni = sysconf(_SC_NSEMMAP)) == -1)
    {
        printf("Can't get number of semaphore sets.\n");
        exit (1);
    }
}

#elif defined(LNX)

#define VSIZE(x) sizeof(x)/sizeof(x[0]) /* size of vector (# elements) */
static i4  kernel = -1; /* /dev/kmem file descriptor */

/* Hack to force query of kernel objects
   we don't actually read /dev/kmem */
static bool kmem_no_read = FALSE; 

static STATUS
open_kmem( i4  argc, char **argv)
{
    LOCATION	loc;

    /* Check /proc is mounted and return an error if it's not */
    LOfroms( PATH, "/proc/sys/kernel", &loc );
    return( LOexist(&loc) );
}

/* Use /proc/sys/kernel special files to return the
 * values for the shminfo structure.
*/
VOID
find_symbols()
{
    SIZE_TYPE len=0; /* size of value requested */
    FILE	*infile;
    LOCATION	loc;
    char	inrecord[24];

    inrecord[0] = 0;
    LOfroms(PATH & FILENAME, "/proc/sys/kernel/shmmax", &loc);
    if ( SIopen(&loc, "r", &infile) == OK )
    {
        SIgetrec(inrecord, sizeof(inrecord), infile);
        STscanf(inrecord, "%Ld", &shminfo.shmmax);
        SIclose(infile);
    }

    inrecord[0] = 0;
    LOfroms(PATH & FILENAME, "/proc/sys/kernel/shmmni", &loc);
    if ( SIopen(&loc, "r", &infile) == OK )
    {
        SIgetrec(inrecord, sizeof(inrecord), infile);
        STscanf(inrecord, "%Ld", &shminfo.shmmni);
        SIclose(infile);
    }

    inrecord[0] = 0;
    LOfroms(PATH & FILENAME, "/proc/sys/kernel/shmall", &loc);
    if ( SIopen(&loc, "r", &infile) == OK )
    {
        SIgetrec(inrecord, sizeof(inrecord), infile);
        STscanf(inrecord, "%Ld", &shminfo.shmall);
        SIclose(infile);
    }

    inrecord[0] = 0;
    LOfroms(PATH & FILENAME, "/proc/sys/kernel/sem", &loc);
    if ( SIopen(&loc, "r", &infile) == OK )
    {
        SIgetrec(inrecord, sizeof(inrecord), infile);
        STscanf(inrecord, "%Ld %Ld %Ld %Ld", &seminfo.semmsl, &seminfo.semmns, &seminfo.semopm, &seminfo.semmni);
        SIclose(infile);
    }
    
}

# elif defined(OSX)
# define VSIZE(x) sizeof(x)/sizeof(x[0]) /* size of vector (# elements) */
static i4  kernel = -1; /* /dev/kmem file descriptor */

/* Hack to force query of kernel objects
   we don't actually read /dev/kmem */
static bool kmem_no_read = FALSE; 

/* sysctl strings for shared memory and semaphore info required */
const char *shmmax = "kern.sysv.shmmax";
const char *shmmni = "kern.sysv.shmmni";
const char *shmall = "kern.sysv.shmall";
const char *semmni = "kern.sysv.semmni";
const char *semmns = "kern.sysv.semmns";
const char *semmsl = "kern.sysv.semmsl";

static STATUS
open_kmem( i4  argc, char **argv)
{
/* If we don't return sucess limits are never queried */
    return( OK );
}

/* display system messages on error */
int
print_sysctl_err( const char *resource )
{
    return( printf("sysctl failed getting %s with system error %d (%s)\n", 
	resource,
	errno,
	strerror(errno) ) ) ;
}

/* Use sysctl() to return the values to fill our seminfo and shminfo
 * structures.  sysconf returns a -1 on error.
*/
VOID
find_symbols()
{
    SIZE_TYPE len=0; /* size of value requested */

    /* Request values, error on failure */
    len=sizeof(shminfo.shmmax);
    if (sysctlbyname(shmmax, &(shminfo.shmmax), &len, 0, 0) )
    {
	print_sysctl_err("SHMMAX");
	exitvalue = RL_KMEM_OPEN_FAIL;
	return;
    }

    if (sysctlbyname(shmmni, &(shminfo.shmmni), &len, 0, 0) )
    {
	print_sysctl_err("SHMMNI");
	exitvalue = RL_KMEM_OPEN_FAIL;
	return;
    }

    len=sizeof(shminfo.shmall);
    if (sysctlbyname(shmall, &(shminfo.shmall), &len, 0, 0) )
    {
	print_sysctl_err("SHMALL");
	exitvalue = RL_KMEM_OPEN_FAIL;
	return;
    }

    len=sizeof(seminfo.semmni);
    if (sysctlbyname(semmni, &(seminfo.semmni), &len, 0, 0) )
    {
	print_sysctl_err("SEMMNI");
	exitvalue = RL_KMEM_OPEN_FAIL;
	return;
    }
	
    len=sizeof(seminfo.semmns);
    if (sysctlbyname(semmns, &(seminfo.semmns), &len, 0, 0) )
    {
	print_sysctl_err("SEMMNS");
	exitvalue = RL_KMEM_OPEN_FAIL;
	return;
    }
	
    len=sizeof(seminfo.semmsl);
    if (sysctlbyname(semmsl, &(seminfo.semmsl), &len, 0, 0) )
    {
	print_sysctl_err("SEMMSL");
	exitvalue = RL_KMEM_OPEN_FAIL;
	return;
    }
	
}

#elif defined(a64_sol)

static bool kmem_no_read = FALSE;

static VOID
find_symbols()
{
    int raction;
    rctlblk_t *rblk;

    if ((rblk = malloc(rctlblk_size())) == NULL) {
	/* FIXME find_symbols() needs to have error return codes */
	return;
    }

    if (getrctl("project.max-shm-memory", NULL, rblk, RCTL_FIRST) == -1) {
	/* FIXME find_symbols() needs to have error return codes */
	return;
    }
    shminfo.shmmax = rctlblk_get_value(rblk);

    if (getrctl("project.max-shm-ids", NULL, rblk, RCTL_FIRST) == -1) {
	/* FIXME find_symbols() needs to have error return codes */
	return;
    }
    shminfo.shmmni = rctlblk_get_value(rblk);

    if (getrctl("project.max-sem-ids", NULL, rblk, RCTL_FIRST) == -1) {
	/* FIXME find_symbols() needs to have error return codes */
	return;
    }
    seminfo.semmni = rctlblk_get_value(rblk);

    if (getrctl("process.max-sem-nsems", NULL, rblk, RCTL_FIRST) == -1) {
	/* FIXME find_symbols() needs to have error return codes */
	return;
    }
    seminfo.semmsl = rctlblk_get_value(rblk);
}


#elif defined(any_aix)

static bool kmem_no_read = FALSE;
static VOID
find_symbols()
{
 bool  process_is_64 = FALSE ; 
 bool  kernel_is_64 = FALSE ; 
 i4    i ; 
 static bool  found = FALSE;

 if( found ) 
   return; 

#if defined(LP64)
    process_is_64 = TRUE; 
#endif

 if( __KERNEL_64() )   
    kernel_is_64 = TRUE;   

    /* For AIX OS 5.1,  the fields in ustname structure will be :   
    ** ustname.version is 5, ustname.release is 1. 
    */
 for( i = 0 ; aix_shminfo[i].version[0] != NULL ; i++ )
  {
   if( aix_uname.version[0] == aix_shminfo[i].version[0] &&
       aix_uname.release[0] == aix_shminfo[i].release[0]  ) 
    {
     if( process_is_64 )
      {
       if( kernel_is_64 )
          shminfo.shmmax = aix_shminfo[i].shmmax_p64_k64;  
       else
          shminfo.shmmax = aix_shminfo[i].shmmax_p64;  
      }
     else
          shminfo.shmmax = aix_shminfo[i].shmmax;  
     found = TRUE; 
     break; 
    } 
  }

 if ( !found )
  {
   printf("Can't get shared memory data for OS version %c release %c.\n", 
           aix_uname.version[0], aix_uname.release[0] ); 
     exit (1);
  }

   /* for OS 6.1 or OS 5.3 with 64-bit kernel  */
 if( ( aix_uname.version[0] == '6' && aix_uname.release[0] == '1' ) ||
     ( aix_uname.version[0] == '5' && aix_uname.release[0] == '3' &&
       kernel_is_64 == TRUE ) )
   {
     shminfo.shmmni = AIX_SHM_MAXSEG_SINCE_REL53_K64 ; 
     seminfo.semmni = AIX_SEM_MAXSET_SINCE_REL53_K64 ; 
   }
 else
   {
     shminfo.shmmni = AIX_SHM_MAXSEG ; 
     seminfo.semmni = AIX_SEM_MAXSET ; 
   }  
 seminfo.semmsl = AIX_SEM_MAXSEMID ;  

}  /* end of find_symbols() for aix */       

#else

static i4  kernel = -1;
static bool kmem_no_read = FALSE;
# if defined(rux_us5)
        static char *kernel_memory = "/dev/mem";
# else
        static char *kernel_memory = "/dev/kmem";
# endif

# if defined(hp3_us5) || defined(any_hpux) || defined(hp8_bls)

static char *namelist = "/hp-ux";

# elif defined(sos_us5) || defined(rmx_us5) ||  \
     defined(pym_us5) || defined(sqs_ptx) || defined(ts2_us5) || \
     defined(sgi_us5) || defined(rux_us5)

static char *namelist = "/unix";

# elif defined(sparc_sol) || defined(sui_us5)

static char *namelist="/dev/ksyms";

# elif defined(dr6_us5) || defined(usl_us5) || defined(nc4_us5)

static char *namelist = "/stand/unix";

# else /* dr6_us5 / nc4_us5 */

static char *namelist = "/vmunix";

# endif /* hp3_us5 */

/*
** Opens kernel memory files.
*/
static STATUS 
open_kmem( i4  argc, char **argv)
{
#if defined(sparc_sol)
	/* Pretend OK if sol 10-up, will use getrctl */
	if(su4_uname.release[2] == '1')
	    return (OK);
#endif;

	if( argc > 1 )
		kernel_memory = argv[ 1 ];
	if( argc > 2 )
		namelist = argv[ 2 ];
	if( (kernel = open( kernel_memory, 0 )) < 0 )
	{
		SIprintf( "Unable to open kernel memory file %s\n",
			kernel_memory);
		SIprintf( "\nAll shared memory resource checking has been disabled.\n\n" );
		return( FAIL );
	}
	return( OK );
}

/*
** Reads kernel memory files.
*/
static VOID
# if defined( axp_osf) || defined(rux_us5) 
locate_kmem( i4  size, unsigned long address, char *result )
# elif defined(sparc_sol)
locate_kmem( i4  size, size_t address, char *result )
# elif defined(thr_hpux)
locate_kmem( i4  size, uint64_t address, char *result )
# else
locate_kmem( i4  size, i4  address, char *result )
# endif /* axp_osf */
{
#if defined(rux_us5)
        address=address & 0x7fffffff;
#endif
	if( lseek( kernel, address, 0 ) != address )
	{
		SIfprintf( stderr, ERx( "can't access kernel address %8x\n" ),
			address );
		PCexit( exitvalue );
	}
	if( read( kernel, result, size) != size )
	{
		SIfprintf( stderr,
			ERx( "can't read all of structure at address %8x\n" ),
			address );
		PCexit( exitvalue );
	}
}

# if defined( sgi_us5) 
static VOID
locate_kmem64( i4  size, off64_t address, char *result )
{
        if( lseek64( kernel, address, 0 ) != address )
        {
                SIfprintf( stderr, ERx( "can't access kernel address %8x\n" ),
                        address );
                PCexit( exitvalue );
        }
        if( read( kernel, result, size) != size )
        {
                SIfprintf( stderr,
                        ERx( "can't read all of structure at address %8x\n" ),
                        address );
                PCexit( exitvalue );
        }
}
# endif /* sgi_us5 */

# if defined(ds3_ulx)

static char *symbols[] = {
	"_sminfo",
	"_seminfo",
	NULL
};

# else /* ds3_ulx */

char *symbols[] = {

# if defined(dr6_us5) || defined(usl_us5) || \
     defined(sparc_sol) || defined(any_hpux) || defined(hp8_bls) || \
     defined(nc4_us5) || defined(sos_us5) || defined(pym_us5) || \
     defined(sqs_ptx) || defined(sui_us5) || defined(rmx_us5) || \
     defined(ts2_us5) || defined(rux_us5)

	"shminfo",
        "seminfo",

# elif defined(sgi_us5)
        "shmmax",
	"shmmni",
	"semmsl",
	"semmni",
# else /* ris_us5 / dr6_us5  */

	"_shminfo",
	"_seminfo",

# endif /* ris_us5 / dr6_us5  */

	/*
	"_physmem",
	"_nswap",
	*/
	NULL
};

# endif /* ds3_ulx */

/*
** Not sure about this one.
*/
static void
find_symbols( void )
{
#ifdef usl_us5

        unsigned long   value = 0;
        unsigned long   info = 0;
        int ret;

        ret = getksym("shminfo", &value, &info);
        if (!ret)
        {
            locate_kmem(sizeof(shminfo), value, &shminfo);
        }

        value = 0; info = 0;

        ret = getksym("seminfo", &value, &info);
        if (!ret)
        {
            locate_kmem(sizeof(seminfo), value, &seminfo);
        }
#elif defined(thr_hpux)
        struct 
	{
		uint64_t        shmmax64;
		uint64_t        shmmin64;
		int             shmmni64;
		int             shmseg64;
	} shminfo64;
												uint64_t   value = 0;
	uint64_t   info = 0;
        int ret;

        ret = getksym( "shminfo", NULL, &value, &info );
        if ( !ret )
        {
		if ( sysconf( _SC_KERNEL_BITS ) == 64 )
		{       
                        locate_kmem( sizeof(shminfo64), value, &shminfo64 );
			shminfo_64.shmmax64 = shminfo64.shmmax64;
			shminfo_64.shmmin64 = shminfo64.shmmin64;
			shminfo.shmmax = (long)shminfo64.shmmax64;
			shminfo.shmmin = (long)shminfo64.shmmin64;
			shminfo.shmmni = shminfo64.shmmni64;
			shminfo.shmseg = shminfo64.shmseg64;
		}
		else
		{
			locate_kmem( sizeof(shminfo), value, &shminfo );
		}
        }

        value = 0; info = 0;

	ret = getksym( "seminfo", NULL, &value, &info );
        if ( !ret )
        {
            locate_kmem( sizeof(seminfo), value, &seminfo );
	}

# else /* newer hpux */


	register struct nlist	*np;
	register char		**cp;
	int			nl_ret;
	struct	nlist		sym_table[12];

#if defined(sparc_sol)
	if(su4_uname.release[2] != '1') {
#endif
	for( cp = symbols, np = sym_table; *cp != NULL; ++cp, ++np )
	{
		np->n_name = *cp;
		np->n_value = 0;
	}
	np->n_name = NULL;
	if( (nl_ret = nlist( namelist, sym_table )) != 0 )
	{
		int np_count=0;
		for( np = sym_table; np->n_name != NULL; ++np )
		{
		    if( np->n_value == 0 )
		    {
			SIfprintf( stderr, ERx( "kernel symbol %s not found\n" ), np->n_name );
			np_count++;
		    }
		}
		SIfprintf( stderr, ERx( "%d kernel symbols not found\n" ), np_count );
		PCexit( exitvalue );
	}
#if defined(sparc_sol)
	}
#endif

#if defined(sgi_us5)
	locate_kmem( sizeof( shminfo.shmmax ), sym_table[ 0 ].n_value,
		(char *) &shminfo.shmmax );
	locate_kmem( sizeof( shminfo.shmmni ), sym_table[ 1 ].n_value,
		(char *) &shminfo.shmmni );
	locate_kmem( sizeof( seminfo.semmsl ), sym_table[ 2 ].n_value,
		(char *) &seminfo.semmsl );
	locate_kmem( sizeof( seminfo.semmni ), sym_table[ 3 ].n_value,
		(char *) &seminfo.semmni );
#elif defined(axp_osf)
        if(axp_uname.release[1] >= '5')    /* is this release 5.x ?      */
                locate_kmem( sizeof( shminfoV5 ), sym_table[ 0 ].n_value,
                        (char *) &shminfoV5 );
        else
                locate_kmem( sizeof( shminfo ), sym_table[ 0 ].n_value,
                        (char *) &shminfo );
        locate_kmem( sizeof( seminfo ), sym_table[ 1 ].n_value,
                (char *) &seminfo );
#elif defined(sparc_sol)
/* Defines from Solaris 9/10 <rctl.h>, may already be defined
** if building on Solaris 9 or 10.
*/
#	if !defined(RCTL_FIRST)
#	    define RCTL_FIRST 0x00000000
#	endif
	/* Use getrctl for Sol 10 and up.  (Assume no Solaris 2.1's are
	** out there...)  Solaris 9 understands the system calls, but
	** apparently no resource controls are set up by default.
	*/
	if(su4_uname.release[2] == '1')
	{
	    void *handle;
	    void *rblk;
	    size_t (*rctlblk_size)(void);
	    int (*getrctl)(char *, void *, void *, uint_t );
	    size_t (*rctlblk_get_value)(void *);
#if defined(LP64)
	    handle = dlopen("/lib/sparcv9/libc.so", RTLD_NOW);
#else
	    handle = dlopen("/lib/libc.so", RTLD_NOW);
#endif
	    rctlblk_size = (size_t (*)(void))dlsym(handle, "rctlblk_size");
	    if (rctlblk_size == NULL)
		/* FIXME find_symbols() needs to have error return codes */
		return;
	    getrctl = (int (*)(char *, void *, void *, uint_t))dlsym(handle, "getrctl");
	    if (getrctl == NULL)
		/* FIXME find_symbols() needs to have error return codes */
		return;
	    rctlblk_get_value = (size_t (*)(void *))dlsym(handle, "rctlblk_get_value");
	    if (rctlblk_get_value == NULL)
		/* FIXME find_symbols() needs to have error return codes */
		return;

	    if ((rblk = malloc((*rctlblk_size)())) == NULL) {
	    /* FIXME find_symbols() needs to have error return codes */
		return;
	    }
	    if ((*getrctl)("project.max-shm-memory", NULL, rblk, RCTL_FIRST) == -1) {
		/* FIXME find_symbols() needs to have error return codes */
		return;
	    }
	    shminfo10.shmmax = (*rctlblk_get_value)(rblk);

	    if ((*getrctl)("project.max-shm-ids", NULL, rblk, RCTL_FIRST) == -1) {
		/* FIXME find_symbols() needs to have error return codes */
		return;
	    }
	    shminfo10.shmmni = (*rctlblk_get_value)(rblk);

	    if ((*getrctl)("project.max-sem-ids", NULL, rblk, RCTL_FIRST) == -1) {
		/* FIXME find_symbols() needs to have error return codes */
		return;
	    }
	    seminfo10.semmni = (*rctlblk_get_value)(rblk);

	    if ((*getrctl)("process.max-sem-nsems", NULL, rblk, RCTL_FIRST) == -1) {
		/* FIXME find_symbols() needs to have error return codes */
		return;
	    }
	    seminfo10.semmsl = (*rctlblk_get_value)(rblk);

	    dlclose(handle);
	    shminfo.shmmax = shminfo10.shmmax;
	    shminfo.shmmni = shminfo10.shmmni;
	    seminfo.semmni = seminfo10.semmni;
	    seminfo.semmns = 0;
	    seminfo.semmsl = seminfo10.semmsl;
	}
	else
	{
	    /* Solaris 9 or earlier, get info the old kmem way and
	    ** copy the values to our generic info structs.
	    */
	    struct seminfo89 old_seminfo;

	    if (su4_uname.release[2] == '9') {
		locate_kmem( sizeof( struct shminfo9 ), sym_table[ 0 ].n_value,
                        (char *) &shminfo9 );
		shminfo.shmmax = shminfo9.shmmax;
		shminfo.shmmni = shminfo9.shmmni;
	    }
	    else
	    {
		locate_kmem( sizeof( struct shminfo8 ), sym_table[ 0 ].n_value,
                        (char *) &shminfo8 );
		shminfo.shmmax = shminfo8.shmmax;
		shminfo.shmmni = shminfo8.shmmni;
	    }
	    locate_kmem( sizeof( old_seminfo ), sym_table[ 1 ].n_value,
                (char *) &old_seminfo );
	    seminfo.semmni = old_seminfo.semmni;
	    seminfo.semmns = old_seminfo.semmns;
	    seminfo.semmsl = old_seminfo.semmsl;
	}
#else
	locate_kmem( sizeof( shminfo ), sym_table[ 0 ].n_value,
		(char *) &shminfo );
	locate_kmem( sizeof( seminfo ), sym_table[ 1 ].n_value,
		(char *) &seminfo );
#endif
#endif /* usl_us5 */
}

#if defined(sgi_us5)
static void
find_symbols64(void) 
{
        register struct nlist64 *np64;
        register char           **cp;
        int                     nl_ret;
        struct  nlist64         sym_table64[12];

        for( cp = symbols, np64 = sym_table64; *cp != NULL; ++cp, ++np64 )
                np64->n_name = *cp;
        np64->n_name = NULL;
        if( (nl_ret = nlist64( namelist, sym_table64 )) != 0 )
        {
                SIfprintf( stderr, ERx( "%d symbols not found\n" ), nl_ret);
                PCexit( exitvalue );
        }
	locate_kmem64( sizeof( shminfo64.shmmax ), sym_table64[ 0 ].n_value,
		(char *) &shminfo64.shmmax );
	locate_kmem64( sizeof( shminfo64.shmmni ), sym_table64[ 1 ].n_value,
		(char *) &shminfo64.shmmni );
	locate_kmem64( sizeof( seminfo.semmsl ), sym_table64[ 2 ].n_value,
		(char *) &seminfo.semmsl );
	locate_kmem64( sizeof( seminfo.semmni ), sym_table64[ 3 ].n_value,
		(char *) &seminfo.semmni );

	if (shminfo64.shmmax > INT_MAX)
	   shminfo.shmmax = INT_MAX;
	else
           shminfo.shmmax = (long) shminfo64.shmmax;

	if (shminfo64.shmmni > INT_MAX)
	   shminfo.shmmni = INT_MAX;
	else
           shminfo.shmmni = (long) shminfo64.shmmni;
}
#endif /*sgi_us5 */

#endif /* dg8_us5 dgi_us5 Linux */

/*
** Returns maximum shared memory segment size.
*/
static STATUS
RLsysSharedSizeHard( char *buf )
{
	if( kmem_no_read )
		return( FAIL );


# if defined(any_aix) || defined(sparc_sol) || \
     defined(LNX) || defined(a64_sol) || defined(OSX)
        CVla8( shminfo.shmmax, buf );

# elif defined(axp_osf)

        if(axp_uname.release[1] >= '5')    /* is this release 5.x ?      */
           CVla8( shminfoV5.shmmax, buf);
        else
           STprintf( buf, ERx( "%d" ), shminfo.shmmax );

# else
	STprintf( buf, ERx( "%lu" ), shminfo.shmmax );

# if defined(thr_hpux)
        if ( sysconf( _SC_KERNEL_BITS ) == 64 )
        {
            char tmp1[20]="shmmax";
            CVuint64la( shminfo_64.shmmax64, tmp1 );
            STcopy( ERx(tmp1), buf );
        }
# endif /* newer hpux */
# endif /* aix */

	return( OK );
}

/*
** Returns current limit on shared memory segment size.
*/
static STATUS
RLsysSharedSizeSoft( char *buf )
{
	return( FAIL );
} 

/*
** Returns maximum number of shared memory segments.
*/
static STATUS
RLsysSharedNumHard( char *buf )
{
	if( kmem_no_read )
		return( FAIL );

# if defined(any_aix) || defined(LNX) || \
     defined(a64_sol) || defined(sparc_sol) || defined(OSX)

        CVla8( shminfo.shmmni, buf);

# elif defined(axp_osf)

        if(axp_uname.release[1] == '5')    /* is this release 5.x ?      */
           STprintf( buf, ERx( "%d" ), shminfoV5.shmmni );
        else
           STprintf( buf, ERx( "%d" ), shminfo.shmmni );

# else
	STprintf( buf, "%d", shminfo.shmmni );

# endif

	return( OK );
}

/*
** Returns number of shared memory segments currently available.
*/
static STATUS
RLsysSharedNumSoft( char *buf )
{
	i4 used, total;
	char command[MAX_LOC + 256];
	char *locstr;
	LOCATION loc;

	if( kmem_no_read )
		return( FAIL );

	/* get the number of shared memory segments in use */
        NMloc(SUBDIR, PATH, "utility", &loc); 
	LOtos(&loc, &locstr);
	STprintf(command, ERx("%s/%ssyslim -used_segs"), &locstr, SystemCfgPrefix);
	if( RLexec( command , buf ) != OK )
	{
		kmem_no_read = TRUE;
		return( FAIL );
	}
	CVan( buf, &used );

	/* get the total allocated */
	RLsysSharedNumHard( buf );
	CVan( buf, &total );

	/* write difference to the return buffer */
	STprintf( buf, "%d", total - used );

	return( OK );
}

static STATUS
RLsysSemaphoresHard( char *buf )
{
	int status = OK;

# if defined(any_aix)

        i8 aix_semmns ;  
        aix_semmns =  ( (i8) seminfo.semmsl * (i8) seminfo.semmni ) ;
        CVla8( aix_semmns, buf );
	

# elif defined(usl_us5) || defined(sgi_us5) || defined (axp_osf) || \
       defined(sqs_ptx) 

        STprintf( buf, ERx( "%d" ), seminfo.semmni * seminfo.semmsl );

# elif defined(a64_sol)

	status = FAIL;

# elif defined(sparc_sol)
	/* seminfo.semmns is the total number of semaphores in the system */
	if(su4_uname.release[2] != '1' )
	    STprintf( buf, "%ld", seminfo.semmns );
	else
	    status = FAIL;

# elif defined(LNX)

        CVla8( seminfo.semmns, buf );

# else /* usl_us5 || sgi_us5 || axp_osf*/

	STprintf( buf, "%d", seminfo.semmns );
	
# endif

	return( status );
}

static STATUS
RLsysSemaphoresSoft( char *buf )
{
	i4 used, total;
	char command[MAX_LOC+256];
	LOCATION loc;

	if( kmem_no_read )
		return( FAIL );

	/* get the number of semaphores in use */
        NMloc(SUBDIR, PATH, "utility", &loc); 
	STprintf(command,ERx("%s/%ssyslim -used_sems"), loc.string, SystemCfgPrefix);
	if( RLexec( command, buf ) != OK )
	{
		kmem_no_read = TRUE;
		return( FAIL );
	}
	CVan( buf, &used );

	/* get the total allocated */
	if (RLsysSemaphoresHard( buf ) != OK)
		return( FAIL );
	CVan( buf, &total );

	/* write difference to the return buffer */
	STprintf( buf, "%d", total - used );

	return( OK );
}

static STATUS
RLsysSemSetsHard( char *buf )
{

# if defined(any_aix) || defined(a64_sol) || defined(sparc_sol) || \
     defined(LNX)

        CVla8( seminfo.semmni, buf);

# else

	STprintf( buf, ERx( "%d" ), seminfo.semmni );

# endif

	return( OK );
}

static STATUS
RLsysSemSetsSoft( char *buf )
{
	i4 used, total;
	char command[MAX_LOC+256];
	LOCATION loc;

	if( kmem_no_read )
		return( FAIL );

	/* get the number of semaphore sets in use */
        NMloc(SUBDIR, PATH, "utility", &loc); 
	STprintf(command,ERx("%s/%ssyslim -used_sets"), loc.string, SystemCfgPrefix);
	if( RLexec( command, buf ) != OK )
	{
		kmem_no_read = TRUE;
		return( FAIL );
	}
	CVan( buf, &used );

	/* get the total allocated */
	RLsysSemSetsHard( buf );
	CVan( buf, &total );

	/* write difference to the return buffer */
	STprintf( buf, "%d", total - used );

	return( OK );
}

static STATUS
RLsysSemsPerIdHard( char *buf )
{

# if defined(any_aix)

	return( FAIL );

# else

#if defined(dg8_us5) || defined(dgi_us5) || defined(ts2_us5)
        STprintf( buf, ERx( "%d" ), seminfo.semmns / seminfo.semmni );
#elif defined(a64_sol) || defined(sparc_sol) || defined(LNX)
        CVla8( seminfo.semmsl, buf);
#else
        STprintf( buf, ERx( "%d" ), seminfo.semmsl );
#endif /* dg8_us5 dgi_us5 */
	return( OK );

# endif

}

static STATUS
RLsysSemsPerIdSoft( char *buf )
{
	return( FAIL );
}

static STATUS
RLsysSwapSpaceHard( char *buf )
{
	char command[MAX_LOC+256];
	LOCATION loc;
	if( kmem_no_read )
		return( FAIL );

        NMloc(SUBDIR, PATH, "utility", &loc); 
	STprintf(command,ERx("%s/%ssyslim -total_swap"), loc.string, SystemCfgPrefix);
	if( RLexec( command, buf ) != OK )
	{
		kmem_no_read = TRUE;
		return( FAIL );
	}
	return( OK );
}

static STATUS
RLsysSwapSpaceSoft( char *buf )
{
	char command[MAX_LOC+256];
	LOCATION loc;
	if( kmem_no_read )
		return( FAIL );

        NMloc(SUBDIR, PATH, "utility", &loc); 
	STprintf(command,ERx("%s/%ssyslim -free_swap"), loc.string, SystemCfgPrefix);
	if( RLexec( command, buf ) != OK )
	{
		kmem_no_read = TRUE;
		return( FAIL );
	}
	return( OK );
}

/*
**  Execute a command, blocking during its execution.
*/

STATUS
RLcheck( i4  id, char *buf, i4  argc, char **argv )
{
	int i;
	struct stat st;
# ifdef any_hpux
        struct utsname  uts;               /* define reference to struct */

	i = uname(&uts);                   /* get opsys release info     */

        if(uts.release[2] == '1'){         /* is this HP/UX 10.0+ ???    */
	    namelist="/stand/vmunix";      /* change pointer to config file */
            } 
# endif  /* any_hpux */

# if defined(axp_osf)

        static int uname_ret = 1;
        if( uname_ret != 0 )
          uname_ret = uname(&axp_uname);   /* get opsys release info     */

# endif  /* axp_osf */

# if defined(sparc_sol)

        static int uname_ret = 1;
        if( uname_ret != 0 )
          uname_ret = uname(&su4_uname);   /* get opsys release info     */

# endif  /* sparc_sol */

# if defined(any_aix)

        static int uname_ret = 1;
        if( uname_ret != 0 )
          uname_ret = uname(&aix_uname);   /* get opsys release info     */

# endif  /* any_aix */

	switch( id )
	{
		case RL_SYS_SHARED_SIZE_HARD:
		case RL_SYS_SHARED_NUM_HARD:
		case RL_SYS_SEMAPHORES_HARD:
		case RL_SYS_SEM_SETS_HARD:
		case RL_SYS_SEMS_PER_ID_HARD:
# if !defined(any_aix) && !defined(a64_sol) && !defined(dg8_us5) && \
     !defined(dgi_us5)
			/* The following doesn't work on AIX 3.1.
			   nor AIX 4.x                         */
			if( kernel < 0 && !kmem_no_read )
			{
				if( open_kmem( argc, argv ) != OK )
				{
					kmem_no_read = TRUE;
					exitvalue = RL_KMEM_OPEN_FAIL;
					return( FAIL );
				}
# if defined(sgi_us5)
				if (sysconf(_SC_KERN_POINTERS) == 64)  /* Is sgi kernel 32 or 64 bit? */
				    find_symbols64();
				else
# endif
				find_symbols();
			}
# else
		       	find_symbols();
# endif /* not sol, lnx, aix */
			if( kmem_no_read )
				return( FAIL );
			break;
	}

	switch( id )
	{
		case RL_PROC_FILE_DESC_HARD:
			return( RLprocFileDescHard( buf ) );

		case RL_PROC_FILE_DESC_SOFT:
			return( RLprocFileDescSoft( buf ) );

		case RL_PROC_FILE_SIZE_HARD:
			return( RLprocFileSizeHard( buf ) );

		case RL_PROC_FILE_SIZE_SOFT:
			return( RLprocFileSizeSoft( buf ) );

		case RL_PROC_RES_SET_HARD:
			return( RLprocResSetHard( buf ) );

		case RL_PROC_RES_SET_SOFT:
			return( RLprocResSetSoft( buf ) );

		case RL_PROC_CPU_TIME_HARD:
			return( RLprocCPUtimeHard( buf ) );

		case RL_PROC_CPU_TIME_SOFT:
			return( RLprocCPUtimeSoft( buf ) );

		case RL_PROC_DATA_SEG_HARD:
			return( RLprocDataSegHard( buf ) );

		case RL_PROC_DATA_SEG_SOFT:
			return( RLprocDataSegSoft( buf ) );

		case RL_PROC_STACK_SEG_HARD:
			return( RLprocStackSegHard( buf ) );

		case RL_PROC_STACK_SEG_SOFT:
			return( RLprocStackSegSoft( buf ) );

		case RL_SYS_SHARED_SIZE_HARD:
			return( RLsysSharedSizeHard( buf ) );

		case RL_SYS_SHARED_SIZE_SOFT:
			return( RLsysSharedSizeSoft( buf ) );

		case RL_SYS_SHARED_NUM_HARD:
			return( RLsysSharedNumHard( buf ) );

		case RL_SYS_SHARED_NUM_SOFT:
			return( RLsysSharedNumHard( buf ) );

		case RL_SYS_SEMAPHORES_HARD:
			return( RLsysSemaphoresHard( buf ) );

		case RL_SYS_SEMAPHORES_SOFT:
			return( RLsysSemaphoresSoft( buf ) );

		case RL_SYS_SEM_SETS_HARD:
			return( RLsysSemSetsHard( buf ) );

		case RL_SYS_SEM_SETS_SOFT:
			return( RLsysSemSetsSoft( buf ) );

		case RL_SYS_SEMS_PER_ID_HARD:
			return( RLsysSemsPerIdHard( buf ) );

		case RL_SYS_SEMS_PER_ID_SOFT:
			return( RLsysSemsPerIdSoft( buf ) );

		case RL_SYS_SWAP_SPACE_HARD:
			return( RLsysSwapSpaceHard( buf ) );

		case RL_SYS_SWAP_SPACE_SOFT:
			return( RLsysSwapSpaceSoft( buf ) );

	}

}

# endif /* VMS */
