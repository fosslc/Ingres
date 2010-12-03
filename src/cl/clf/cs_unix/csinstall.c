/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <clipc.h>
#include    <er.h>
#include    <me.h>
#include    <cs.h>
#include    <fdset.h>
#include    <csev.h>
#include    <si.h>
#include    <cv.h>
#include    <nm.h>
#include    <pc.h>
#include    <pe.h>
#include    <lo.h>
#include    <tr.h>
#include    <cx.h>
#include    <cssminfo.h>
#include    <pm.h>

/* for ME_getkey, probably ought to be in me */

#include    <meprivate.h>

/**
**
**  Name: CSINSTALL.C - Install Shared Memory for an installation
**
**  Description:
**      This program will create and initialize shared memory and
**	semaphores to allow the UNIX CL to function.  This sets up resources
**	for LG,LK and CS.
**
**          main() - The csinstall main program
**
**
**  History:
**      08-sep-88 (anton)
**          First version
**	01-Mar-1989 (fredv)
**	   Use <systypes.h> and <clipc.h> instead of <systypes.h>, <sys/ipc.h>
**	   	and <sys/sem.h>.
**	   Added ifdef xCL_002 around struct semun.
**	27-mar-89 (anton)
**	    Changed ming hint from USELIBS to NEEDLIBS
**	15-apr-89 (arana)
**	    Removed ifdef xCL_002 around struct semun.  Defined struct semun
**	    in clipc.h ifndef xCL_002.
**	12-jun-89 (rogerk)
**	    Changed arguments to MEsmdestroy to take character segment key
**	    instead of LOCATION pointer.  Changed remsem() to take segment
**	    name also.  Changed ME_loctokey calls to ME_getkey.
**	23-mar-90 (mikem)
**	    Added CSinitiate() call to support use of CS{p,v}_semaphore() for
**	    cross process semaphores used to protect UNIX specific LG/LK 
**	    shared memory segment.
**	4-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add serialization support for odt_us5;
**		Add xCL_075 around SYSV shared memory calls;
**		Add a TRset_file to stdout for csinstall.
**	8-june-90 (blaise)
**	    Removed call to NMloc() which shouldn't have been added in the
**	    above change.
**	25-sep-1991 (mikem) integrated following change: 9-aug-1991 (bryanp)
**	    B39082: If processes are still alive, don't reinstall segments.
**	3-jul-1992 (bryanp)
**	    Pass new "num_of_wakeups" argument to CS_create_sys_segment()
**	    Don't need to destroy "lockseg" any more.
**	11-nov-1992 (mikem)
**	    Initialized nusers to 0, otherwise random stack value was being
**	    passed to CS_create_sys_segment(), sometimes causing multi-megabyte
**	    system segments to be created.
**	23-Apr-1993 (fredv)
**	    Add NO_OPTIM for ris_us5.
**		The problem: csinstall couldn't create the system segment.
**		Why: the return status from CS_create_sys_segment wasn't 
**		     assigned to the register that the optimizer expected.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (bryanp)
**	    Change default num of servers from MAXSERVERS/2 to MAXSERVERS/4 now
**	    that MAXSERVERS has changed from 32 to 128.
**	20-aug-93 (ed)
**	    add missing include
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	20-oct-1993 (lauraw/bryanp)
**		Remove call to CSinitiate. It isn't needed & generates
**		ugly warning messages on screen (map_sys_segment fails).
**      13-feb-1995 (chech02)
**              Added rs4_us5 for AIX 4.1.
**	2-may-1995 (wadag01)
**		Applied 6.4/05 changes:
**		"22-nov-1991 (rudyw)
**	    	Remove odt serialization stuff since ingres 64 unbundled
**		from ODT."
**	31-may-1996 (sweeney)
**		set permissions mask so that "key" files created in
**	 	$II_SYSTEM/ingres/files/memory have sensible permissions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      15-aug-2002 (hanch04)
**          For the 32/64 bit build,
**          call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	19-sep-2002 (devjo01)
**	    Provide handling for "-rad" command line option used to 
**	    specify target RAD in NUMA environments.
**	04-Oct-2004 (hweho01)
**	    Removed rs4_us5 from NO_OPTIM list. 
**          Compiler : AIX VisualAge C 6.006.
**	24-Aug-2009 (kschendel) 121804
**	    Need meprivate.h to satisfy gcc 4.3.
**	14-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/



# ifdef xCL_075_SYS_V_IPC_EXISTS
static	VOID	remsem(char *);
# endif

/*{
** Name: main()	- csinstall main program.
**
** Description:
**	Call ME and CS to create segments.
**
** Inputs:
**	argc		argument count
**	argv		argument string vector
**				one numeric arugment which is max servers
**				for that installation.
**
** Outputs:
**      none
**
**	Returns:
**	    via PCexit
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-sep-88 (anton)
**          First version
**	12-jun-89 (rogerk)
**	    Changed arguments to MEsmdestroy to take character segment key
**	    instead of LOCATION pointer.  Changed remsem() to take segment
**	    name also.
**	23-mar-90 (mikem)
**	    Added CSinitiate() call to support use of CS{p,v}_semaphore() for
**	    cross process semaphores used to protect UNIX specific LG/LK 
**	    shared memory segment.
**	25-sep-1991 (mikem) integrated following change: 9-aug-1991 (bryanp)
**	    B39082: If processes are still alive, don't reinstall segments.
**	3-jul-1992 (bryanp)
**	    Pass new "num_of_users" argument to CS_create_sys_segment()
**	    Don't need to destroy "lockseg" any more.
**	11-nov-1992 (mikem)
**	    Initialized nusers to 0, otherwise random stack value was being
**	    passed to CS_create_sys_segment(), sometimes causing multi-megabyte
**	    system segments to be created.
**	26-jul-1993 (bryanp)
**	    Change default num of servers from MAXSERVERS/2 to MAXSERVERS/4 now
**	    that MAXSERVERS has changed from 32 to 128.
**	20-oct-1993 (lauraw/bryanp)
**		Remove call to CSinitiate. It isn't needed & generates
**		ugly warning messages on screen (map_sys_segment fails).
**	31-may-1996 (sweeney)
**		set permissions mask so that "key" files created in
**	 	$II_SYSTEM/ingres/files/memory have sensible permissions.
**	19-sep-2002 (devjo01)
**	    Allow specification of target RAD for NUMA installations.
**	 8-may-2007 (wanfr01)
**	    SIR 118278 - Add error details when you fail to 
**	    create shared memory
**       22-Oct-2008 (coomi01) b121111
**          Pick up the predicted connect_sum from the dbms configuration setup.
**          If this is greater than the restrictive nserv*150 use this to configure
**          the number of wakeup blocks.
**	07-Nov-2008 (hweho01)
**	    Fix SEGV on 64-bit (Solaris/Sparc); need to include pm.h and  
**	    other header file, so pointer returned from PMhost() wouldn't 
**	    be truncated.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**	08-Jan-2010 (thaju02) Bug 123336
**	    Add svr_slots configuration param.
*/

main(argc, argv)
int	argc;
char	*argv[];
{
    STATUS	status;
    CL_ERR_DESC	err_code;
    auto i4	nserv;
    i4		nusers = 0;
    i4		i;
    char	segname[64];
    char        request[128];
    char        *aNusers;
    CS_SMCNTRL	*sysseg;
    PID		pid;

    MEadvise(ME_INGRES_ALLOC);

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    {
	char	*lp64enabled;

	/*
	** Try to exec the 64-bit version
	*/
	NMgtAt("II_LP64_ENABLED", &lp64enabled);
	if ( (lp64enabled && *lp64enabled) &&
	   ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
	     !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
	{
	    PCexeclp64(argc, argv);
	}
    }
#endif  /* hybrid */

    TRset_file(TR_F_OPEN, "stdio", 5, &err_code);

    PEumask("rw-r--");

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
    {
	PCexit(FAIL);
    }

    nserv = 0;
    while (++argv, --argc)
    {
	if (**argv == '-')
	{
	    
	    /*
	    ** check the system segment to see if there are any processes
	    ** using the shared memory. If there are, don't allow it to
	    ** be destroyed unless they said "-f" or "-F".
	    */
	    PCpid(&pid);
	    if (CS_map_sys_segment(&err_code) == OK)
	    {
		CS_get_cb_dbg((PTR *) &sysseg);
		for (i = 0; i < sysseg->css_numservers; i++)
		{
		    if (sysseg->css_servinfo[i].csi_in_use != 0 &&
			sysseg->css_servinfo[i].csi_pid != pid &&
			kill(sysseg->css_servinfo[i].csi_pid, 0) != -1)
		    {
			TRdisplay(
		    "csinstall: Process %d is still using the shared memory.\n",
				    sysseg->css_servinfo[i].csi_pid);
			if (*((*argv)+1) != 'f' && *((*argv)+1) != 'F')
			    PCexit(FAIL);
		    }
		}
	    }

	    /* destroy any left over server segements */
	    
	    for (i = 0; i < MAXSERVERS; i++)
	    {
		
		STcopy("server.", segname);
		CVna(i, segname+STlength(segname));
		
# ifdef xCL_075_SYS_V_IPC_EXISTS
		remsem(segname);
# endif
		(VOID) MEsmdestroy(segname, &err_code);
	    }
	    
	    /* destroy the system shared memory segment */
	    
# ifdef xCL_075_SYS_V_IPC_EXISTS
	    remsem("sysseg.mem");
# endif
	    (VOID) MEsmdestroy("sysseg.mem", &err_code);
	}
	else
	{
	    CVan(*argv, &nserv);
	}
    }

    if (nserv == 0)
    {
	PTR	svrslots;
	if (PMget("ii.*.config.svr_slots", &svrslots) == OK)
	    CVan(svrslots, &nserv);
	else
	    nserv = MAXSERVERS/4;
    }

    /*
    ** Bug 121111
    **
    ** When the number of users exceeds space set up in shared
    ** mem as wake-up blocks, the server may crash.
    **
    ** The dbms.connect_sum is an estimate of the number of users
    ** the client may wish to connect to the system.
    */
    nusers = 0;
    STprintf( request, 
	      "%s.%s.dbms.connect_sum",
	      SystemCfgPrefix, 
	      PMhost());
    
    if ( OK == PMget(request, &aNusers) )
    {
	/* 
	** Covert to an integer 
	*/
	CVan(aNusers, &nusers);
    }

    /*
    ** Compare with original hard-wired ratio
    */
    if (nusers < (nserv * 150))
    {
	/*
	** If the hardwired figure was bigger, use it.
	*/
	nusers =  nserv *  150;
    }
    else if (nusers > MAXSERVERS * 150)
    {
	/*
	** But do not exceed the hardwired limit
	** - This is 19200, which is large enough.
	*/
	nusers = MAXSERVERS * 150;
    }

    if (nserv > MAXSERVERS || nserv < 1)
    {
	SIprintf("Can not support more than %d servers\n", MAXSERVERS);
	status = FAIL;
    }
    else
    {
	if (status = CS_create_sys_segment(nserv, nusers, &err_code))
	{
	    SIprintf("Error %d creating the system shared memory segment\n", status);
	}
    }

    PCexit(status);
}

# ifdef xCL_075_SYS_V_IPC_EXISTS
static VOID
remsem(char *segname)
{
    int			id;
    union semun uarg;

    id = semget(ME_getkey(segname), 0, 0);
    uarg.val = 0;
    if (id >= 0)
	semctl(id, 0, IPC_RMID, uarg);
}
# endif /* xCL_075_SYS_V_IPC_EXISTS */

