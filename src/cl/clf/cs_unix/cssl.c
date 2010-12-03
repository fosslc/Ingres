/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = dr6_us5 dr6_ues i64_aix
*/

# include    <compat.h>
# include    <gl.h>
# include    <st.h>
# include    <clconfig.h>
# include    <systypes.h>
# include    <clsigs.h>
# include    <fdset.h>
# include    <cs.h>
# include    <csev.h>
# include    <cssminfo.h>
# include    <ex.h>
# include    <tr.h>
# include    <er.h>
# include    <me.h>
# include    <nm.h>
# include    <cv.h>
# include    <di.h>
# include    <dislave.h>
# include    <csinternal.h>
# include    <cslocal.h>
# include    <exinternal.h>

/**
**
**  Name: CSSL.C - Control system for slave processes
**
**  Description:
**	This file contains functions specific to control of slave
**	processes.  This file defines:
**
**	CS_slave_main()	-	main slave process function.
**	CS_init_slave() -	slave process control initialiazation
**
**
**  History:
 * Revision 1.9  89/01/29  18:21:10  jeff
 * New select driven event system
 * 
 * Revision 1.8  89/01/24  05:01:01  roger
 * Make use of new CL error descriptor.
 * 
 * Revision 1.7  89/01/18  17:30:16  jeff
 * pipe & signal wakeups & CS_find_events will never block
 * 
 * Revision 1.6  89/01/05  15:15:39  jeff
 * moved header file
 * 
 * Revision 1.5  88/12/14  13:55:49  jeff
 * event system optimizations
 * 
 * Revision 1.4  88/09/08  16:14:38  jeff
 * comment improvements and shared memory cleanup fixs
 * 
 * Revision 1.3  88/08/31  17:17:25  roger
 * Changes needed to build r
 * 
 * Revision 1.2  88/08/24  11:30:10  jeff
 * cs up to 61 needs
 * 
 * Revision 1.6  88/05/31  11:25:22  anton
 * added spin lock
 * 
 * Revision 1.5  88/04/21  12:08:35  anton
 * changed location of fdset.h since sun doesn't do the right thing.
 * 
 * Revision 1.4  88/04/18  13:04:35  anton
 * code cleanup
 * 
 * Revision 1.3  88/03/21  12:23:14  anton
 * Event system appears solid
 * 
**      22-feb-88 (anton)
**	    Created.
**	5-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Make more checks to see if server has gone away;
**		Add calls to iiCSwrite_pipe instead of a direct "write";
**		Add ifdef xCL_075... around SysV shared memory calls;
**		Add code for xCL_078_CVX_SEM to support Convex semaphores;
**		Set NO_OPTIM for dr6_us5.
**	8-june-90 (blaise)
**	    Replaced call to iiCSwrite_pipe with a "write". iiCSwrite_pipe
**	    has not (yet?) been integrated into 63.
**	24-Aug-90 (jkb)
**	    Change the mechanism for the slave alerting the dbms server for
**	    pipes to signals.  On some SYSV systems you can't poll on
**	    a pipe.
**	15-apr-91 (vijay)
**	     Added error handling for SIGDANGER for ris_us5. This signal is sent
**	     to all processes when free swap space is getting low.
**	03-mar-92 (jnash)
**	    Fix problem where LG and DI slaves created slave log files of the
**	    same name.  Also, fail if TRset_file call fails when slave
**	    logging is requested.
**	22-Oct-1992 (daveb)
**	    Move things around to make startup failure easier to
**	    diagnose.  Give separate return statuses for each exit,
**	    and try to log a nicer message.
**	26-Oct-1992 (daveb)
**	    Decode status vals with ERget (it's ok in a slave).
**      05-nov-1992 (mikem)
**          su4_us5 port.  Added NO_OPTIM.  Using the same compiler as su4_u42.
**	26-apr-1993 (fredv)
**	    Moved clsigs.h up to avoid redeclaration of EXCONTINUE.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	9-aug-93 (robf)
**	     Add su4_cmw
**	16-aug-93 (ed)
**	    add <st.h>
**	13-mar-95 (smiba01)
**	    Added dr6_ues to NO_OPTIM line.
**      22-jan-1997 (canor01)
**          Protect change in server control block's csi_events_outstanding
**          flag so changes do not get lost in concurrent access.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          use csmt version of this file
**	04-feb-1998 (somsa01) X-integration of 432936 from oping12 (somsa01)
**	    Added code to handle the "slave exit" signal.
**	30-may-2000 (toumi01)
**	    Send signals to new csi_signal_pid (vs. csi_pid).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    move CSEV_CALLS from csslave.c to cssl.c so that it will be
**	    available in libcompat.a
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      14-Nov-2002 (hanch04)
**          Removed NO_OPTIM for Solaris
**      28-Dec-2003 (horda03) Bug 111532/INGSRV2649
**          Buffer passed to EXsys_report() too small.
**    30-Aug-2004 (hanje04)
**        BUG 112936.
**        IIDBMS SEGV's creating database when II_THREAD_TYPE set to INTERNAL.
**        Tempory workaround for GA, silently dissable INTERNAL THREADS. i.e.
**        ingore II_THREAD_TYPE and always use OS threads. FIX ME!!
**	14-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/


/*
**  Forward and/or External typedef/struct references.
*/

GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;
GLOBALREF	CS_SERV_INFO	*Cs_svinfo;


/*
**  Forward and/or External function references.
*/


static void CSslave_exit(PTR address);
static i4 CSslave_handler(EX_ARGS *);

# ifdef SIGDANGER
GLOBALDEF       bool    iiCSrcvd_danger = FALSE;    /* flag set in handler */

#ifdef  xCL_011_USE_SIGVEC
static TYPESIG
CSslave_danger(i4 signum, i4 code, struct sigcontext *scp);
# else
static TYPESIG
CSslave_danger(i4 signum);
# endif

# endif /* SIGDANGER */


/*
**  Defines of other constants.
*/

/*
**      this is a comment about the below
**	constants. 
*/


/* typedefs go here */

/*
** Definition of all global variables owned by this file.
*/


/*
**  Definition of static variables and forward static functions.
*/

static	char	*CSev_base;


/*{
** Name: CS_init_slave - Main procedure of a CS slave process
**
** Description:
**	This function should be called from where ever main is defined
**	for a slave process.  It takes control and runs the slave process
**	that serves requests from its master.
**
** Inputs:
**	argc		argc from main(argc, argv)
**	argv		argv from main(argc, argv)
**
** Outputs:
**
**	Returns:
**	    never
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-feb-88 (anton)
**          Created.
**	22-Oct-1992 (daveb)
**	    Move things around to make startup failure easier to
**	    diagnose.  Give separate return statuses for each exit,
**	    and try to log a nicer message.
**      22-jan-1997 (canor01)
**          Protect change in server control block's csi_events_outstanding
**          flag so changes do not get lost in concurrent access.
**	04-feb-1998 (somsa01 X-integration of 432936 from oping12 (somsa01)
**	    Added code to handle a "slave exit" signal.
**      05-Oct-1999 (wanfr01)
**          Spinlocks on the server info block (Cs_svinfo) is only valid
**          for OS threading.  Bug 91805, INGSRV 453
**	30-Oct-2001 (inifa01)  Bug 91805 INGSRV 453
**	   Amended fix to bug 91805 to not allow spinlocks on 
**	   CS_svinfo on OS threaded platforms when use of ingres threads is
**	   turned on (II_THREAD_TYPE is set to INTERNAL).
**	22-nov-2001 (hayke02)
**	    Modify above change so that we only call spinlock functions
**	    if OS threading is used and II_THREAD_TYPE is not set to
**	    internal (which is recommeneded for hpb_us5). This change
**	    fixes bug 106436.
**	15-Jun-2004 (schka24)
**	    Safe env variable handling.
**	10-Nov-2010 (kschendel) SIR 124685
**	    Ditch obsolete "GC slave", whatever that was.  Hardwire to
**	    nearly-as-obsolete DI slaves.
*/

void
CS_init_slave(int argc, char **argv)
{
    i4		slave_no, slave_type, serve_no, subslave;
    fd_set	fdmask;
    register CSEV_CB	*evcb;
    register CSSL_CB	*slcb;
    CSEV_CB	*last_evcb;
    CSEV_SVCB	*svcb;
    CL_ERR_DESC	errcode;
    i4		i;
    char	*str;
    PTR		address;
    STATUS	status;
    EX_CONTEXT	excontext;
    char	path[200];
    bool	cs_mt;

    NMgtAt("II_SLAVE_LOG", &str);
    if ((str) && (*str))
    {
	if( argc > 4 )
	    STlpolycat(4, sizeof(path)-1, str, argv[3], ".", argv[4], path);
	else
	    STlpolycat(2, sizeof(path)-1, str, ".err", path );
	status = TRset_file(TR_F_OPEN, path, STlength(path), &errcode);
	if (status)
	{
	    PCexit(1);
	}
    }

#ifdef OS_THREADS_USED
    cs_mt = TRUE;
    str = NULL;
#ifndef LNX
    NMgtAt("II_THREAD_TYPE", &str);
    if ((str) && (*str))
    {
	if ( STcasecmp(str, "INTERNAL") == 0 )
	    cs_mt = FALSE;
    }
#endif /* NO INTERNAL THREADS FOR LINUX */
#endif /* OS_THREADS_USED */

    if (EXdeclare(CSslave_handler, &excontext))
    {
	/* An exception excaped the handler, big problems */
	EXdelete();
	TRdisplay("Exception escaped slave handler, exit 2\n");
	PCexit(2);
    }

# ifdef SIGDANGER
    i_EXsetothersig(SIGDANGER, CSslave_danger);
# endif /* SIGDANGER */

    if (argc != 6 || CVan(argv[1], &slave_type) || CVan(argv[2], &slave_no) ||
	CVan(argv[3], &serve_no) || CVan(argv[4], &subslave) ||
	slave_type != 1)
    {
        TRdisplay("Bad arguments to slave, exit 3\n" );
	PCexit(3);
    }

    CS_recusage();

    if (CS_map_sys_segment(&errcode))
    {
	TRdisplay("map sys segment %d, exit 4\n", errcode.errnum);
	PCexit(4);
    }
    if (CS_map_serv_segment(serve_no, &address, &errcode))
    {
	TRdisplay("map serv segment %d, exit 5\n", errcode.errnum);
	PCexit(5);
    }
    CSev_base = (char *)address;
    svcb = (CSEV_SVCB *)address;
    slcb = &svcb->slave_cb[slave_no];
    FD_ZERO(&fdmask);
    str = argv[5];
    for (i = 0; i < FD_SETSIZE && *str != EOS; i += 4)
    {
	if (*str & 1)
	{
	    FD_SET(i, &fdmask);
	}
	if (*str & 2)
	{
	    FD_SET(i+1, &fdmask);
	}
	if (*str & 4)
	{
	    FD_SET(i+2, &fdmask);
	}
	if (*str & 8)
	{
	    FD_SET(i+3, &fdmask);
	}
	++str;
    }
    if( (status = DI_init_slave(fdmask, FD_SETSIZE)) != OK )
    {
      TRdisplay("Slave evinit error, status %x:\n%s\nslave exit 6\n",
		    status, ERget(status) );
      PCexit( 6 );
    }
    last_evcb = (CSEV_CB *)(CSev_base + slcb->events);
    status = OK;
    while (status == OK)
    {
	/* find evcb */
	evcb = last_evcb;
	CS_ACLR(&Cs_svinfo->csi_subwake[slcb->slsembase + subslave]);
	while (status == OK)
	{
	    if ((evcb->flags & (EV_INPG|EV_SLCP)) == EV_INPG &&
		evcb->forslave == subslave)
	    {
		break;
	    }
	    evcb = (CSEV_CB *)((char *)evcb + slcb->evsize);
	    if (evcb >= (CSEV_CB *)(CSev_base + slcb->evend))
	    {
		evcb = (CSEV_CB *)(CSev_base + slcb->events);
	    }
	    if (evcb == last_evcb)
	    {
# ifdef xCL_075_SYS_V_IPC_EXISTS
# ifdef SIGDANGER
		status = CS_slave_sleep(slcb->slsemid, slcb->slsembase + subslave);
# else /* SIGDANGER */
		status = CS_sleep(slcb->slsemid, slcb->slsembase + subslave);
# endif /* SIGDANGER */
# endif
# ifdef xCL_078_CVX_SEM
                status = 
		    CS_sleep(&Cs_svinfo->csi_csem[slcb->slsembase + subslave]);
# endif
		CS_ACLR(&Cs_svinfo->csi_subwake[slcb->slsembase + subslave]);
	    }
	}
	CS_TAS(&Cs_svinfo->csi_subwake[slcb->slsembase + subslave]);
	if (status != OK)
	{
	    TRdisplay("slave error, status sleep %x:\n%s\n", 
		      status, ERget(status) );
	    break;
	}
	last_evcb = (CSEV_CB *)((char *)evcb + slcb->evsize);
	if (last_evcb >= (CSEV_CB *)(CSev_base + slcb->evend))
	{
	    last_evcb = (CSEV_CB *)(CSev_base + slcb->events);
	}

	/*
	** If this slave is signalled to exit, call CSslave_exit().
	*/
	if (evcb->flags & EV_EXIT)
	    CSslave_exit(address);

	status = DI_slave(evcb);
        if( status != OK )
	{
	    TRdisplay("slave error calling evhandle, status %x:\n%s\n",
		      status, ERget(status) );
	    break;
	}
#ifdef OS_THREADS_USED
	if (cs_mt)
	    CS_getspin(&Cs_svinfo->csi_spinlock); 
#endif /* OS_THREADS_USED */
	evcb->flags |= EV_SLCP;
	CS_TAS(&svcb->event_done[slave_no]);
	CS_TAS(&Cs_svinfo->csi_events_outstanding);
	if (CS_TAS(&Cs_svinfo->csi_wakeme))
	{
		(void)kill(Cs_svinfo->csi_signal_pid,SIGUSR2);
	}
#ifdef OS_THREADS_USED
	if (cs_mt)
	    CS_relspin(&Cs_svinfo->csi_spinlock); 
#endif /* OS_THREADS_USED */
    }
    TRdisplay("Fatal error in slave, status %x:\n%s\n, slave exit 7\n",
	      status, ERget(status) );
    PCexit(7);
}

/*{
** Name: CSslave_handler - Top level exception hander for slave process
**
** Description:
**	This is the top level exception hander for a slave process.
**	It is a should not happen handler except for EXSTOP which indicates
**	that the slave was merly stoped and can be continued.
**
** Inputs:
**	exargs		arguements from EX
**
** Outputs:
**	returns
**
**	Returns:
**	    EXRESIGNAL		Resignal to get UNIX default action
**	    EXDECLARE		Return to main body of slave.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-feb-88 (anton)
**          Created
**	12-aug-91 (bonobo)
**	    Updated ERsend() to include ER_ERROR_MSG.
*/
static i4
CSslave_handler(EX_ARGS *exargs)
{
    char	buf[EX_MAX_SYS_REP];
    i4		i;
    CL_ERR_DESC	error;

    if (!EXsys_report(exargs, buf))
    {
	STprintf(buf, "Exception is %x", exargs->exarg_num);
	for (i = 0; i < exargs->exarg_count; i++)
	    STprintf(buf+STlength(buf), ",a[%d]=%x",
		     i, exargs->exarg_array[i]);
    }
    TRdisplay("csslave: %t\n", STlength(buf), buf);
    ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);
    if (exargs->exarg_num == EXSTOP)
	return(EXRESIGNAL);
    return(EXDECLARE);
}

# ifdef SIGDANGER
/*
** Name: CSslave_danger - exception handler for SIGDANGER.
**
** Description:
**      This is the exception hander for sigdangers. On AIX (RS/6000),
**      all processes receive this signal when swap space is going low.
**      If we don't handle it during semop waiting, the slave exits due
**      to interrupt in system call, and consequently the server dies.
**
** History:
**      2-apr-91 (vijay)
**          Created
*/
#ifdef  xCL_011_USE_SIGVEC

static TYPESIG
CSslave_danger(i4 signum, i4 code, struct sigcontext *scp)

# else

static TYPESIG
CSslave_danger(i4 signum)

# endif

{
# if !defined(xCL_011_USE_SIGVEC)
        /* re-establish handler */
        signal(signum, CSslave_danger);
# endif
        iiCSrcvd_danger = TRUE;
}
# endif /* SIGDANGER */

/*
** Name: CSslave_exit - Exit a CS slave process
**
** Description:
**	This function is called by CS_init_slave() when it receives the
**	EV_EXIT signal.
**
** Inputs:
**	slave_serv_ptr	pointer to server shared memory segment from
**			CS_init_slave
**
** Outputs:
**	none.
**
**	Returns:
**	    never
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-feb-1998 (somsa01)
**	    Created.
*/

static void
CSslave_exit(slave_serv_ptr)
PTR	slave_serv_ptr;
{
    CL_ERR_DESC err_code;
    i4  i;
    char segname[48];

    CS_find_server_slot(&i);
    STcopy("server.", segname);
    CVna(i, segname+STlength(segname));
    /* Detach the server segment from this slave. */
    MEdetach(segname, slave_serv_ptr, &err_code);

    /* Detach the system segment from this slave. */
    MEfree_pages((PTR)Cs_sm_cb,
	Cs_sm_cb->css_css_desc.cssm_size / ME_MPAGESIZE,
	&err_code);

    /* Exit the slave process. */
    PCexit(OK);
}
