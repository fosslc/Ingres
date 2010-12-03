/*
** Copyright (c) 1990, 2008 Ingres Corp.
*/

#include    <compat.h>
#include    <systypes.h>
#include    <gl.h>
#include    <machconf.h>
#include    <nm.h>
#include    <cv.h>

/**
**
**  Name: MACHCONF.C - machine configuration information.
**
**  Description:
**	Routines to specify machine, OS dynamic configuration information. These
**	issues are very likely to be very system specific (ie. it is very
**	likely this file will have many ifdef's).
**
**	    ii_sysconf()        - external interface to module.
**          ii_num_processors() - best guess at number of processors on machine.
**
**  History:
**      02-oct-90 (mikem)
**	    Created.
**	04-jan-91 (jkb)
**	   change #if to #if defined, the preprocessor doesn't like the #if
**	   format
**	06-apr-92 (mikem)
**	   SIRS #43444 and #43445
**	   Added support for IISYSCONF_MAX_SEM_LOOPS and 
**	   IISYSCONF_MAX_DESCHED_USEC_SLEEP.  Also added support to dynamically
**	   determine the number of processors on Suns.
**	21-jun-93 (mikem)
**	   Added su4_us5 (solaris) support to dynamically determine the number of
**	   processors on the current machine.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      02-aug-93 (shailaja)
**         cleared QAC warnings.
**      06-jan-94 (johnst)
**	   Bug #58887
**	   Added axp_osf (DEC Alpha) support to dynamically determine the 
**	   number of processors on the current machine.
**	   Also added #include <systypes.h> to cure lots of compiler typedef 
**	   warnings that were occuring due to later inclusion of <sys/types.h>.
**      15-feb-94 (johnst)
**	   Bug #59782
**	   Added ICL dr6_us5 support to dynamically determine the 
**	   number of processors on the current machine.
**      30-Nov-94 (nive)
**         Added support for dg8_us5 during OpenINGRES 1.1 port
**	30-may-95 (canor01)
**	   Added AIX 4.1 (rs4_us5) support to dynamically determine the 
**	   number of processors on the current machine. (Same as
**	   su4_us5.)
**	22-jun-95 (allmi01)
**	   Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	14-dec-95 (wadag01)
**	   Added SCO (odt_us5 and sos_us5) support to dynamically determine the 
**	   number of processors on the current machine.
**	18-dec-95 (morayf)
**	   Added rmx_us5 support for SNI RMx00 (and also pym_us5 for Pyramid).
**	   An entry for 'pyr_us5' defining processor count as 2 was here,
**	   but I have removed it.
**	23-apr-1996 (canor01)
**	   For Windows NT, get number of processors from GetSystemInfo().
**      27-may-97 (mcgem01)
**         Cleaned up compiler warnings.
**	10-may-1999 (walro03)
**	    Remove obsolete version string odt_us5, pyr_u42, sqs_us5.
**      03-jul-99 (podni01)
**         Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      30-jul-99 (hweho01)
**         Added support for AIX 64-bit (ris_u64) platform. 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-feb-2001 (somsa01)
**	    Porting changes to i64_win.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      22-Jan-2007 (hanal04) Bug 117533
**          Prototype hp8_us5_get_num_of_processors() to silence warnings
**          on hpb.
**	19-nov-2008 (abbjo03)
**	    Add support for VMS.
**	20-Jun-2009 (kschendel) SIR 122138
**	    New sparc and hp symbols no longer imply old ones, fix here.
**      03-nov-2010 (joea)
**          Complete prototypes for ii_num_processors and get_value.
**/


/*
**  Forward and/or External function references.
*/

static i4 ii_num_processors(void);    /* best guess at number of processors */

static i4 get_value(char *symbol, i4 default_value);

#if defined(any_hpux)
static i4     hp8_us5_get_num_of_processors();
#endif


/*{
** Name: ii_sysconf()	- return info about machine configuration issues.
**
** Description:
**	Provides a method for the application to determine the current value
**	of configurable system limits, hardware configurations, ...
**
**	This routine currently understands the following items:
**
**	IISYSCONF_PROCESSORS	- number of processors available on 
**				  machine.  1 indicates single 
**				  processor, >= 2 indicates a 
**				  multiprocessor.
**	IISYSCONF_MAX_SEM_LOOPS	- Number of times to loop in CSp_semaphore()
**				  while trying to obtain a multi-process
**				  semaphore held by another process, on a MP
**				  machine,  before going to sleep for 
**				  II_DESCHED_USEC_SLEEP micro-seconds.  
**	IISYSCONF_DESCHED_USEC_SLEEP - 
**				  Number of micro-seconds to sleep after
**				  II_MAX_SEM_LOOPS loops have been executed
**				  in CSp_semaphore() while trying to obtain a 
**				  multi-process semaphore held by another
**				  process, on a MP machine.
**
** Inputs:
**      item                            configuration parameter of interest.
**					    IISYSCONF_PROCESSORS
**					    IISYSCONF_MAX_SEM_LOOPS
**					    IISYSCONF_MAX_DESCHED_USEC_SLEEP
**
** Outputs:
**      value                           value of item requested.
**
**	Returns:
**	    OK   - success.
**	    FAIL - no knowledge of requested parameter. 
**
** History:
**      02-oct-90 (mikem)
**         Created.
**	06-apr-92 (mikem)
**	   SIR #43445
**	   Added support for IISYSCONF_MAX_SEM_LOOPS and 
**	   IISYSCONF_MAX_DESCHED_USEC_SLEEP.
*/
STATUS
ii_sysconf(i4 item, i4 *value)
{
    i4	ret_val = OK;

    switch (item)
    {
	case IISYSCONF_PROCESSORS:
	    *value = ii_num_processors();
	    break;

	case IISYSCONF_MAX_SEM_LOOPS:
	    *value = get_value("II_MAX_SEM_LOOPS", DEF_MAX_SEM_LOOPS);
	    break;

	case IISYSCONF_DESCHED_USEC_SLEEP:
	    *value = 
		get_value("II_DESCHED_USEC_SLEEP", DEF_DESCHED_USEC_SLEEP);
	    break;

	default:
	    ret_val = FAIL;
	    break;
    }

    return(ret_val);
}

/*{
** Name: ii_num_processors()	- return the number of processors
**
** Description:
**	Routine returns the number of processors on a given machine.
**	This is a "best guess", as most machines/os's do not have
**	a method for programatically determining this information.
**
**	The default for this function is to return the value 1 
**	(ie. single processor).  Machines that should default to multiple
**	processor (ie. sequent, pyramid, ...), should ifdef in a value of
**	2.  If any machine has an actual runtime call it should be used 
**	rather than these ifdef's.
**
**	The routine will also be user tunable by the specification of the
**	environment variable II_NUM_OF_PROCESSORS.  If this variable is set
**	(either with ingsetenv or a user level symbol), then that value is
**	used instead.  This allows users to override defaults.  This is
**	valuable in multiprocessor systems if ingres were to use the actual
**	number of processors for runtime configuration (ie. start up N - 1
**	if there are N processors on the machine); and is also helpful if the
**	system defaults to the wrong value (ie. sequent delivers a one 
**	processor system, or a current single processor company delivers a 
**	multiple processor system in the future).
**
**	Note that the only penalty for picking the wrong value for 
**	II_NUM_PROCESSORS should be non-optimal performance.  This symbol
**	should not be used by code which will only run correctly only if
**	the symbol is set correctly, as the setting of this variable is
**	only a best guess.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**
**	Returns:
**	      num	returns number of processors on the
**			machine.
**			    1 - single processor
**			    2 - multiprocessor (2 or more 
**				processors). If you can't get
**				actual number of processors, but
**				it is known it is a 
**				multiprocessor then set it to 2.
**			    >2- actual number of processors.
**	
**
** History:
**      02-oct-90 (mikem)
**         Created.
**	04-jan-91 (jkb)
**	   change #if to #if defined, the preprocessor doesn't like the #if
**	   format
**	06-apr-92 (mikem)
**	   SIR #43445
**	   On Suns running 4.1.X the number of processors can be determined at
**	   runtime.  su4_u42_get_num_of_processors() returns the number of
**	   processors.
**      12-jan-93 (terjeb)
**         Added code to determind the number of processors for HP-UX.
**         hp8_us5_get_num_of_processors() returns the number of physical
**         operational processors.
**	22-mar-93 (pholman/robf)
**	    Add entry for su4_cmw (SunOS CMW) based upon su4_bsd.
**	21-jun-93 (mikem)
**	   Added su4_us5(solaris) support to dynamically determine the number of
**	   processors on the current machine.
**	02-aug-93 (shailaja)
**	   cleared QAC warnings.
**      06-jan-94 (johnst)
**	   Bug #58887
**	   Added axp_osf (DEC Alpha) support to dynamically determine the 
**	   number of processors on the current machine.
**      15-feb-94 (johnst)
**	   Bug #59782
**	   Added ICL dr6_us5 support to dynamically determine the 
**	   number of processors on the current machine.
**	23-apr-1996 (canor01)
**	   For Windows NT, get number of processors from GetSystemInfo().
*/
static i4
ii_num_processors()
{
    i4		num_processors = DEF_PROCESSORS; 
    
    /* add an ifdef for any machine that is shipped as a multi-processor in its
    ** default configuration.
    */

#if defined(sqs_ptx) 
    num_processors = 2;
#endif
#if defined(rmx_us5) || defined(pym_us5) || defined(rux_us5)
    num_processors = rmx_us5_get_num_of_processors();
#endif
#if defined(su4_u42) || defined(su4_cmw)
    num_processors = su4_u42_get_num_of_processors();
#endif
#if defined(sparc_sol) || defined(any_aix)
    /* *** FIXME?  what about a64_sol? */
    num_processors = su4_us5_get_num_of_processors();
#endif
#if defined(any_hpux)
    num_processors = hp8_us5_get_num_of_processors();
#endif
#if defined(axp_osf)
    num_processors = axp_osf_get_num_of_processors();
#endif
#if defined(dr6_us5)
    num_processors = dr6_us5_get_num_of_processors();
#endif
# if defined(dg8_us5) || defined(dgi_us5)
        num_processors = dg8_us5_get_num_of_processors();
# endif /* dg8_us5 dgi_us5 */
#if defined(axm_vms) || defined(i64_vms)
    num_processors = vms_get_num_of_processors();
#endif
# if defined(NT_GENERIC)
    {
	SYSTEM_INFO sysinfo;

	GetSystemInfo( &sysinfo );
	num_processors = sysinfo.dwNumberOfProcessors;
    }
# endif /* NT_GENERIC */
    
    num_processors = get_value("II_NUM_OF_PROCESSORS", num_processors);

    return(num_processors);
}

/*{
** Name: get_value()	- get value 
**
** Description:
**	Return value of symbol input.  If symbol is not set in the environment
**	then return "default_value".
**
** Inputs:
**	symbol		The string representation of the symbol (ie. "II_SYM")
**	default_value	An integer default value to be returned if symbol not
**			found in the environment.
** Outputs:
**	none.
**
**
**	Returns:
**	      value of the symbol if set in environment, else returns
**	      whatever "default_value" was input.
**
** History:
**      06-apr-92 (mikem)
**         Created.
*/
static i4
get_value(char *symbol, i4 default_value)
{
    i4		value = default_value; 
    char	*string;
    i4	env_value;
    
    NMgtAt(symbol, &string);
    if (string                &&
	*string               &&
	(!CVal(string, &env_value)))
    {
	/* user override of default */
	value = env_value;
    }

    return(value);
}

#if defined(su4_u42) || defined(su4_cmw)

/* sun specific code to determine number of processors */

#undef u_char
#undef u_int
#undef u_long
#undef u_short
#undef ushort
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioccom.h>
#include <sun/mem.h>

/* The following is necessary if the code is built on a 4.1.1 Sun system,
** as MIOCGPAM was not defined until 4.1.2.  This definition has been taken
** out of a 4.1.2 <sun/mem.h>.
*/
#if !defined(MIOCGPAM)
#define   MIOCGPAM        _IOWR(M, 3, u_int)
#endif

/*{
** Name: su4_u42_get_num_of_processors() - return number of processors on a sun.
**
** Description:
**	Sun Multi-processors running SUN OS 4.1.X support a special ioctl()
**	call which returns a value which can be used to figure out how many
**	processors are enabled.  This call will either fail on SP machines or
**	it will return a value that will be interpreted as only one processor
**	enabled.  This routine will not be valid under Solaris 2.0/SUNOS 5.0.
**	On any errors routine returns "1" processor.
**
**	This routine makes the magic ioctl() call and returns the number of 
**	processors on the machine.
**
**	If any error is encountered then the routine will default to 1 
**	processor.
**
** Inputs:
**      none.
**
** Outputs:
**	none.
**
**	Returns:
**	    number of processors on the machine.
**
** History:
**      06-apr-92 (mikem)
**	    SIR #43444
**          Created.
*/
static i4
su4_u42_get_num_of_processors()
{
    int		num_processors = 0;
    int		bitcount;
    int		fd;
    int		pam;

    if ((fd = open("/dev/null", O_RDONLY)) > 0)
    {
	if (ioctl(fd, MIOCGPAM, &pam) == 0)
	  {
	    /* # of set bits in "pam" is the number of processors */

	    num_processors = 0;
	    for (bitcount = 0; bitcount < BITS_IN(int); bitcount++)
	    {
		if (0x01 & pam)
		    num_processors++;
		pam >>= 1;
	    }
	}
	close(fd);
    }

    if (num_processors < 1)
    {
	/* return 1 on all errors */
	num_processors = 1;
    }

    return(num_processors);
}
#endif /* su4_u42 */

#if defined(any_hpux)

#undef u_char
#undef u_short
#undef u_int
#undef u_long
#undef ushort
#include <sys/syscall.h>
#include <sys/mp.h>
#include <sys/utsname.h>

/*{
** Name: hp8_us5_get_num_of_processors() - return number of processors on 
**                                         an HP9000 S800 machine.
**
** Description:
**	HP PA-RISC systems running HP-UX 8.06+ and later supports
**	mpctl() calls, which can be used to determind the number of
**	configured processors in the machine.
**
**	This routine has been safe-guarded, so that no attempt
**	to execute this call on a kernel which does not support this
**	call will be attempted. We also do not attempt to perform this
**	operation if we are executing on an S700 machine. This is done 
**	because this call might not be provided on future os releases 
**	for this machine.
**
**	This routine makes the mpctl() call and returns the number of
**	processors on the machine. If any error is encountered then the
**	routine will default to 1 processor.
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
**      Returns:
**          number of processors on the machine.
**
** History:
**      12-jan-93 (terjeb)
**          Created after the SUN model.
**	21-Jan-2005 (shaha03)
**	    Modified function to call GETNUMSPUS for all versions of HP-UX.
*/
static i4
hp8_us5_get_num_of_processors()
{
    int         num_processors = 0;

    if ( ( num_processors = GETNUMSPUS() ) < 1 )
    {
	num_processors = 1;		/* return 1 on all errors */
    }
    return(num_processors);
}

#endif /* hp8_us5 */

#if defined(sparc_sol) || defined(any_aix)

#undef u_char
#undef u_short
#undef u_int
#undef u_long
#undef ushort
#undef ulong
#include <unistd.h>

/*{
** Name: su4_us5_get_num_of_processors() - Get number of active processors
**
** Description:
**      longer comment telling what function does.
**      usually spans more than one line. 
** Description:
**	Return number of active processors on a sun running solaris 2.2 or
**	higher.  The _SC_NPROCESSORS_ONLN argument to sysconf provides the
**	Number of processors online.  The _SC_NPROCESSORS_CONF can be used
**	if one wishes to get the Number of processors (CPUs) configured.
**      On errors sysconf returns -1, in which case this routine will default
**	to returning 1 processor.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    number of processors on the machine.
**
** History:
**      21-jun-93 (mikem)
**          Created.
**
*/
static i4
su4_us5_get_num_of_processors(void)
{
    long		num_processors = 0;

    num_processors = sysconf(_SC_NPROCESSORS_ONLN);

    if ((num_processors = sysconf(_SC_NPROCESSORS_ONLN)) < 1)
    {
	/* default to 1 on all errors */
	num_processors = 1;
    }

    return((i4) num_processors);
}

#endif

#if defined(axp_osf)

# include <sys/sysinfo.h>
# include <machine/hal_sysinfo.h>

/*{
** Name: axp_osf_get_num_of_processors() - Get number of active processors
**
** Description:
**	Return number of processors on the machine.
**	On successful completion, getsysinfo() returns a value indicating the 
**	number of requested items returned. If the information requested is 
**	not available, it returns a zero.  Otherwise -1 is returned.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    number of processors on the machine.
**
** History:
**      06-jan-94 (johnst)
**	    Bug #58887
**          Created.
**
*/
static i4
axp_osf_get_num_of_processors()
{
    i4	num_processors;

    if (getsysinfo((unsigned long) GSI_MAX_CPU, &num_processors, 
	    (unsigned long) sizeof(num_processors), (int *) 0, (char *) 0) <= 0)
    {
	/* default to 1 on all errors */
	num_processors = 1;
    }

    return(num_processors);
}

# endif /* axp_osf */

#if defined(axm_vms) || defined(i64_vms)
#include <starlet.h>
#include <ssdef.h>
#include <efndef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <syidef.h>
static i4
vms_get_num_of_processors()
{
    i4 num_processors;
    ILE3 itmlst[] =
	{ { sizeof(num_processors), SYI$_AVAILCPU_CNT, &num_processors, 0},
	  { 0, 0, 0, 0}
	};
    IOSB iosb;
    int status = sys$getsyiw(EFN$C_ENF, NULL, NULL, itmlst, &iosb, NULL, 0);
    if (status & 1)
	status = iosb.iosb$w_status;
    if (status != SS$_NORMAL)
	num_processors = 1;
    return num_processors;
}
#endif /* axm_vms || i64_vms */

#if defined(dr6_us5)
/*{
** Name: dr6_us5_get_num_of_processors() - return number of processors on an ICL DRS-6000.
**
** Description:
**
**      On icl DRS-6000 boxes there is an option for multiprocessor extensions cards.
**      This procedure detects support for the option and will return 1 if whenever 
**      it is not found. On the other hand should the option exists, it makes
**      a sysconfig system call to determin the number of processors running. It
**      then checks to ensure that the function call will return at least one
**      processor in the event of a system error.
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
**      Returns:
**          number of processors on the machine.
**
** History:
**      15th-feb-94 (johnst)
**	   Bug #59782.
**         Added; integrated from 6.4/04.
**         Added support to dynamically determin num of processors for 
**	   ICL DRS/NX 6000s with mpe's.
*/

#if defined(xCL_069_UNISTD_H_EXISTS)
#include <sys/unistd.h>
#endif

static i4
dr6_us5_get_num_of_processors()
{
    int         num_processors=1;

#if defined(xCL_079_SYSCONF_EXISTS) && defined(_SC_NPROCESSORS_ONLN)

    num_processors = sysconf(_SC_NPROCESSORS_ONLN);

    if (num_processors < 1)
    {
        /* return 1 on all errors */
        num_processors = 1;
    }

#endif /* SYSCONF_EXISTS ... */

    return(num_processors);
}
#endif /* dr6_us5 */

# if defined(dg8_us5) || defined(dgi_us5)
#include <sys/dg_sys_info.h>

dg8_us5_get_num_of_processors()
{
        struct dg_sys_info_machine_info machine_info;
        i4         dgstat;
    i4             num_processors = 0;

        dgstat = dg_sys_info((long *)&machine_info,
                     DG_SYS_INFO_MACHINE_INFO_TYPE,
                     DG_SYS_INFO_MACHINE_CURRENT_VERSION);
        if (dgstat < 0)
        {
                /* default on DG machines is 2 */
                num_processors = 2;
        }
        else
        {
        num_processors = machine_info.number_of_processors;
        }

    return((i4) num_processors);
}
#endif /* dg8_us5 dgi_us5 */


# if defined(sos_us5)
#include <sys/utsname.h>
/*{
** Name: sco_us5_get_num_of_processors() - return number of processors on 
**                                         a SCO v4.2 or SCO 5.0 machine.
**
** Description:
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
**      Returns:
**          number of processors on the machine.
**
** History:
**      07-sep-95 (morayf)
**          Created.
*/

static long
sco_us5_get_num_of_processors()
{
	struct scoutsname info;
	int	num_processors;
	
	if (__scoinfo( &info, sizeof(struct scoutsname)) < 0)
	{
		/* Default to 1 on failure */

		num_processors = 1;
	}
	else
		num_processors = info.numcpu > 0 ? info.numcpu : 1;

	return((long) num_processors);
}

#endif /* sos_us5 */

#if defined(rmx_us5) || defined(pym_us5) || defined(rux_us5)

#include <sys/types.h>
#include <sys/mpcntl.h>

static i4
rmx_us5_get_num_of_processors()
{
    i4             num_processors = 0;

    if ((num_processors = mpcntl(MPCNTL_CPUCNT, (void *)NULL)) < 1)
                num_processors = 2;	/* default SNI RMx00 to 2 */

    return((i4) num_processors);
}

#endif /* rmx_us5, pym_us5 */
