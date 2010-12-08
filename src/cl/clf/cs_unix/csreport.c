/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <clipc.h>
#include    <tr.h>
#include    <cs.h>
#include    <fdset.h>
#include    <csev.h>
#include    <cssminfo.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <cx.h>
#include    <ex.h>

/**
**
**  Name: CSREPORT.C - Report on the installations shared memory resources
**
**  Description:
**	This is a short program to report to an installation manager on the
**	status of an installation
**
**          main() - main funtion of csreport
**
**
**  History:
**      08-sep-88 (anton)
**	    First Version
**	01-Mar-1989 (fredv)
**	   Use <systypes.h> and <clipc.h> instead of <systypes.h>, <sys/ipc.h>
**	   	and <sys/sem.h>.
**	   Added ifdef xCL_002 around struct semun.
**	27-mar-89 (anton)
**	    Changed ming hint from USELIBS to NEEDLIBS
**      15-apr-89 (arana)
**          Removed ifdef xCL_002 around struct semun.  Defined struct semun
**          in clipc.h ifndef xCL_002.
**	16-apr-90 (kimman)
**	    Adding ds3_ulx ifdef because ipcs returns the ipc-key as %d not
**		as 0x%x.
**	4-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Increase the size of vals from 64 to 128, to prevent RT from
**		core dumping;
**		Add xCL_075 around SYSV shared memory calls;
**		Add vax_ulx specific code (same as ds3_ulx).
**	6-jul-1992 (bryanp)
**	    No longer try to report on LG/LK segment.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (bryanp)
**	    Removed uses of css_semid, since it's no longer being used.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      31-jan-94 (mikem)
**          bug #58298
**          Changed CS_handle_wakeup_events() to maintain a high-water mark
**          for events in use to limit the scanning necessary to find
**          cross process events outstanding.  Previous to this each call to
**          the routine would scan the entire array, which in the default
**          configuration was 4k elements long.  Have csreport the high-water
**	    mark.
**	06-oct-1993 (tad)
**		Bug #56449
**		Changed %x to %p for pointers in reportSM().
**	2-Feb-1994 (fredv)
**		css_numwakeups should be referenced as sysseg->css_wakeup.css_numwakeups
**		instead of sysseg->css_numwakeups.
**	1-Mar-1995 (harpa06)
**          Bug #67217
**          Changed a TRdisplay statement from "Can't map system segement" to
**          "Can't map system segment".
**      23-Jun-92 (radve01)
**          session id printed in pointer format.
**      11-May-2001 (huazh01)
**          Modify the procedure reportSM, change the parameter for printing
**	    share memory key to '%<width>s'. Instead of printing in hex format,
**          it prints out the cssm_id in string format when it is not ds3_ulx
**	    and vax_ulx.
**	10-may-1999 (walro03)
**	    Remove obsolete version string vax_ulx.
**	12-may-2000 (somsa01)
**	    Modified MING hint to account for different products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
**      06-Apr-2005 (horda03) Bug 112371/INGSRV 2635
**          When displaying the wakeup block information, do so without
**          holding the css_spinlock. If the output is piped to "more"
**          then csreport could be stalled holding the css_spinlock
**          mutex, thus bring the Ingres installation to a halt. If the
**          application was then terminated, the css_spinlock would never
**          be released, and so the Installation hangs.
**	17-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/

/*
PROGRAM = (PROG0PRFX)csreport

MODE = SETUID

NEEDLIBS = COMPATLIB MALLOCLIB
*/

static void reportServs(CS_SERV_INFO *svinfo, i4 nservs);
static void reportSM(CS_SM_DESC *shm);
static VOID report_wakeups(CS_SMCNTRL	*sysseg);

/*{
** Name: main() - report on an installation
**
** Description:
**	report on shared memory and semaphores of an installation
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    PCexit()
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-sep-88 (anton)
**          First version
**      15-apr-89 (arana)
**          Removed ifdef xCL_002 around struct semun.  Defined struct semun
**          in clipc.h ifndef xCL_002.
**      28-Mar-91 (gautam)
**          Change display format in TRdisplay statement since the
**          csi_connect_id  field of CS_SERV_INFO struct is a char[]
**	6-jul-1992 (bryanp)
**	    No longer try to report on LG/LK segment.
**      31-jan-94 (mikem)
**          bug #58298
**          Changed CS_handle_wakeup_events() to maintain a high-water mark
**          for events in use to limit the scanning necessary to find
**          cross process events outstanding.  Previous to this each call to
**          the routine would scan the entire array, which in the default
**          configuration was 4k elements long.  Have csreport the high-water
**	    mark.
**	01-mar-94 (harpa06)
**	    Bug #67217
**	    Changed a TRdisplay statement from "Can't map system segement" to
**	    "Can't map system segment". 
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

main(argc, argv)
int	argc;
char	*argv[];
{
    CL_ERR_DESC	err_code;
    CS_SMCNTRL	*sysseg;

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

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
    {
	PCexit(FAIL);
    }

    if (CS_map_sys_segment(&err_code))
    {
	TRdisplay("Can't map system segment\n");
	PCexit(1);
    }
    CS_get_cb_dbg((PTR *) &sysseg);
    TRdisplay("Installation version %d\n", sysseg->css_version);
    TRdisplay("Max number of servers %d\n", sysseg->css_numservers);
    TRdisplay("Max number of threads %d\n", sysseg->css_wakeup.css_numwakeups);
    TRdisplay("\t Min free wakeup slot %d\n", sysseg->css_wakeup.css_minfree);
    TRdisplay("\t Max used wakeup slot %d\n", sysseg->css_wakeup.css_maxused);
    TRdisplay("Description of shared memory for control system:\n");
    reportSM(&sysseg->css_css_desc);
    TRdisplay("Event system: used space %d, length space %d\n",
	      sysseg->css_events_off, sysseg->css_events_end);
    TRdisplay("Server info:\n");
    reportServs(sysseg->css_servinfo, sysseg->css_numservers);
    report_wakeups(sysseg);
    PCexit(0);
}

# ifdef xCL_075_SYS_V_IPC_EXISTS
static void
reportSM(CS_SM_DESC *shm)
{
    char len[32];
    char id[16];
    char statement[64] =  "key 0x%";

    STprintf(id, "%x",  shm->cssm_id );
    STprintf(len, "%x", STlength( id ) );

    STcat( statement, len );
    STcat( statement, "s: size %d attach %p\n" );

    TRdisplay(statement, id, shm->cssm_size,
	      shm->cssm_addr);
}
# else
static void
reportSM(CS_SM_DESC *shm)
{
    TRdisplay("size %d attach %p\n", shm->cssm_size, shm->cssm_addr);
}
# endif	/* xCL_075_SYS_V_IPC_EXISTS */

static void
reportServs(CS_SERV_INFO *svinfo, i4 nservs)
{
    i4	i;

    for (i = 0; i < nservs; i++)
    {
	TRdisplay("server %d:\n", i);
# ifdef xCL_075_SYS_V_IPC_EXISTS
   	TRdisplay("inuse %d, pid %d, connect id %s, id_number %d, semid %d\n",
		  svinfo->csi_in_use, svinfo->csi_pid, svinfo->csi_connect_id,
		  svinfo->csi_id_number, svinfo->csi_semid);
# else
	TRdisplay("inuse %d, pid %d, connect id %s, id_number %d\n",
                  svinfo->csi_in_use, svinfo->csi_pid, svinfo->csi_connect_id,
                  svinfo->csi_id_number);
# endif
	TRdisplay("shared memory:\n");
	reportSM(&svinfo->csi_serv_desc);
	TRdisplay("\n");
	svinfo++;
    }
}

/*
** Typedef myEventHandlerStructType
**
** Description:
**      This is a local struct typedef
**      to hold details of EXdeclare context
**      and count any signals we might have
**      received whilst taking a spinlock
**
** History:
**      02-Apr-08 (coomi01) : b120195
**      Created.
*/
typedef struct {
    EX_CONTEXT         context;
    PTR                 wb_mem;
    int           signalsFound;
} myEventHandlerStructType;

/*
** Struct myEventHandlerStructType Ptr.
**
** Description:
**     This is a non re-entrant instance of 
**     the event handler structure.
*/ 
static
myEventHandlerStructType *nonRentrantClientPtr;


/*{
** Name:   eventHandler	
**
** Description:
**	React to any events whilst we have a spinlock
**      through an event handler.
**
** Inputs:
**	ex		The exception argument structure.
**
** Outputs:
**	Returns:        EXCONTINUES to ignore the event.
**
** Side Effects:
**	None.
**
**      02-Apr-08 (coomi01) : b120195
**      Introduce a very simple event handler.
**      Catches any signals, and does a simple count.
**
*/
static STATUS
recordEventHandler(EX_ARGS *ex)
{
    /*
    ** We have a signal, record it in our handler structure.
    */
    nonRentrantClientPtr->signalsFound++;

    /*
    **  Tell EXsignal to do no more processing
    */
    return EXCONTINUES;    
}


/*{
** Name:   blockSignals	
**
** Description:
**
**      Create a EXdeclare block to handle signals.
**      The handler, recordEventHandler, is setup 
**      to do nothing other than count signals
**      received. This effectively blocks the signal.
**
** Inputs:
**	myEventHandlerStructType* : Pointer to our control structure
**
** Outputs:
**	Returns:        None.
**
** Side Effects:
**	None.
**
** History:
**      02-Apr-08 (coomi01) : b120195
**      First edition.
*/
static VOID 
blockSignals(myEventHandlerStructType *ctx)
{
    /*
    ** We store the handler struct in global memory.
    ** This is not re-entrant, but for csreport we
    ** do not care.
    */
    nonRentrantClientPtr = ctx;

    /*
    ** Initailize our counter
    */
    ctx->signalsFound = 0;

    /*
    ** The declare block should NEVER get called, because
    ** our event handler returns EXCONTINUES to Exsignal()
    ** which therefore does not perform the long jump.
    **
    ** None the less, we code up the block and do what
    ** used to happen prior to the bug fix, exit 
    ** without clearing away the spinlock. This is 
    ** because we do not know whether we own the
    ** spinlock or not.
    **
    */
    if ( EXdeclare(recordEventHandler, &ctx->context) != OK )
    {
        /*
        ** Tidy away our trap asap 
        */
        EXdelete();
        
	/*
	** Free memory
	*/
	if (ctx->wb_mem)
	{
	    MEfree( ctx->wb_mem );	    
	}

	/* 
        ** Send an informative message, and exit.
        */
        TRdisplay( "Unexpected interrupt, this should not have happened\n");
        PCexit(-1);
    }
}

/*{
** Name:   releaseSignals
**
** Description:
**      Release the EXdeclare block, and return signal count.
**
** Inputs:
**	myEventHandlerStructType* : Pointer to our control structure
**
** Outputs:
**	Returns:        Count of signals received.
**
** Side Effects:
**	None.
**
** History:
**      02-Apr-08 (coomi01) : b120195
**      First edition.
**
*/
static int
releaseSignals(myEventHandlerStructType *ctx)
{
    EXdelete();
    return ctx->signalsFound;
}


/*{
** Name:   report_wakeups
**
** Description:
**      Open dbms shared memory, and report on wakeups
**
** Inputs:
**	sysseg  : Pointer to CS_SMCNTRL memory
**
** Outputs:
**      Use TRdisplays to print required details
**
** Returns:  
**      None.
**
** Side Effects:
**      None if we complete execution in a controlled manner.
**      If we exit prematurely, a deadlock on the dbms servers.
**
** History:
**	02-Apr-08 (coomi01) : b120195
**      Introduce a very simple event handler.
**      Catches any signals preventing the process dying
**      without releasing the spinlock.
*/
static VOID
report_wakeups(CS_SMCNTRL	*sysseg)
{
    CS_SM_WAKEUP_CB    *wakeup_blocks;
    i4			            i;
    i4                     no_wakeups;
    myEventHandlerStructType  control;

    /*
    ** Init our mem alloc ptr
    */
    control.wb_mem = (PTR)0;

    /*
    ** Block signal handling, so we can take a spinlock
    */
    blockSignals(&control);
    CS_getspin(&sysseg->css_spinlock);

    /* Bug 112371
    ** Rather than output the list of wakeup blocks while holding
    ** the css_spinlock, copy the blocks to local storage under
    ** the spinlock protection, then release the spinlock and
    ** output the details.
    **
    ** This drastically reduces the possibility of a User aborting
    ** this utility whilst the css_spinlock is held - and bringing the
    ** installation to a halt.
    */
    no_wakeups = sysseg->css_wakeup.css_numwakeups;

    /* 
    ** Get some local memory
    */
    control.wb_mem = MEreqmem( 0, no_wakeups * sizeof(CS_SM_WAKEUP_CB), FALSE, NULL);

    if (control.wb_mem)
    {
       wakeup_blocks = (CS_SM_WAKEUP_CB *)
                        ((char *)sysseg + sysseg->css_wakeups_off);

       MEcopy( (PTR) wakeup_blocks, no_wakeups * sizeof(CS_SM_WAKEUP_CB), control.wb_mem);

       CS_relspin(&sysseg->css_spinlock);

       /*
       ** Check for interupts
       */
       if ( releaseSignals(&control) )
       {
	   /*
	   ** In this version we don't care about nature
	   ** of the signal received. The event handler
	   ** did not keep enough detail.
	   **
	   ** - We will print a message and exit.
	   */
	   TRdisplay( "Interrupted\n");

	   /* 
	   ** Tidy away first
	   */
	   MEfree( control.wb_mem );
	   PCexit(-1);
       }

       wakeup_blocks = (CS_SM_WAKEUP_CB *) control.wb_mem;

       for (i = 0; i < no_wakeups; i++)
       {
           if (wakeup_blocks[i].inuse == 1)
           {
               TRdisplay("Block %d is in use; pending wakeup for pid %d, sid %p\n",
                           i, wakeup_blocks[i].pid, wakeup_blocks[i].sid);
           }
       }

       MEfree( control.wb_mem );
    }
    else
    {
       CS_relspin(&sysseg->css_spinlock);

       TRdisplay( "Error allocating memory in report_wakeups\n");

       /*
       ** Check for interupts
       */
       if ( releaseSignals(&control) )
       {
	   TRdisplay( "Interrupted\n");
	   PCexit(-1);
       }
    }
}
