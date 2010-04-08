/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
#include    <er.h>
#include    <ex.h>
#include    <id.h>
#include    <pc.h>
#include    <tm.h>
#include    <cv.h>
#include    <cs.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <scfcontrol.h>
#include    <sc0e.h>

/**
**
**  Name: SCFINITDBG.C - Provide scf environment for module level debugging
**
**  Description:
{@comment_line@}...
**
**          scf_init_dbg()  - set up scf for module testing
**
**
**  History:    $Log-for RCS$
**      27-Mar-86 (fred)    
**          Created on Jupiter
**	13-jul-92 (rog)
**	    Included ddb.h and er.h.
**	6-nov-1992 (bryanp)
**	    Check CSinitate() return code.
**	16-dec-1992 (bryanp)
**	    Ensure that ics_eusername is set to &ics_susername.
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**	2-Jul-1993 (daveb)
**	    cast arg to CSset_sid.
**	6-jul-1993 (shailaja)
**	    Cast arguments for prototype compatibility.
**	6-sep-93 (swm)
**	    Change cast of scb from i4 to CS_SID to match revised CL
**	    specification.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	12-jul-1995 (canor01)
**	    The "nasty things" mentioned below were Unix-specific.
**	    New ones were added for DESKTOP.
**      15-jul-95 (emmag) (NT)
**	    Orlan's changes are NT not DESKTOP specific.  Changed define.
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	03-jun-1996 (canor01)
**	    Add initialization needed by operating-system threads.
**	10-feb-1997 (canor01)
**	    Remove NT reference to hPurgeEvent.  This is now created in
**	    GCinitiate and not needed here.
**	14-feb-1997 (canor01)
**	    As part of merging of Ingres-threaded and OS-threaded servers,
**	    make initialization dependent on run-time, not compile-time.
**	30-may-1997 (muhpa01)
**	    Set scb->cs_scb.cs_self = (CS_SID) scb for POSIX threads
**	18-apr-1997 (kosma01)
**	    Also create semaphore sc_mcb_semaphore for dmfacp, rcpstat and 
**	    other single threaded applications.
**	12-feb-1998 (canor01)
**	    Initialize the cs_di_event event for NT.
**	12-Mar-1998 (jenjo02)
**	    Session's MCB is now embedded in SCS_SSCB, not allocated
**	    separately.
**	20-Mar-1998 (jenjo02)
**	    Removed cs_access_sem, utilizing cs_event_sem instead to 
**	    protect cs_state and cs_mask.
**	09-Apr-1998 (jenjo02)
**	    Set SC_IS_MT in sc_capabilities if OS threads in use.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id. Note that for NT_GENERIC, cs_self
**	    remains the thread_id, at least for now.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	16-Nov-1998 (jenjo02)
**	    Replaced init of cs_event_sem, cs_event_cond with call to
**	    (new) CSMT_get_wakeup_block().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	20-dec-1999 (somsa01)
**	    cs_self is now (CS_SID *)scb for NT_GENERIC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-jun-2001 (somsa01)
**	    For NT, since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Add include of sc0e.h, use new form sc0ePut().
**	09-dec-2008 (joea)
**	    Bypass CSMT_get_wakeup_block on VMS.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/


/*
**  Forward and/or External function references.
*/


/*
** Definition of all external variables owned by this file.
*/

/*
** The server CB should be zero filled on entry;  in all status fields,
** zero (0) means uninitialized.
*/
#define	NEEDED_SIZE (((sizeof(SC_MAIN_CB) + (SCU_MPAGESIZE - 1))	& ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE)
GLOBALREF SC_MAIN_CB          *Sc_main_cb;           /* The server's server cb */

/*{
** Name: scf_init_dbg	- initialize server for module (other than scf) testing
**
** Description:
**      This routine performs some initialization of SCF so that other facilities 
**      can utilize the benefits of running scf without having to run under total 
**      control of the system control facility.  This routine is expected to be 
**      invoked ONLY by module testing.
**
** Inputs:
**      none
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    SCF is initialized, as much as is necessary to support the current
**	    functionality.
**
** History:
**	27-Mar-86 (fred)
**          Created on Jupiter.
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_username	--> ics_eusername	effective user id
**		ics_ucode	--> ics_sucode		copy of the session user
**							id
**		ics_uid		--> ics_ruid		real user uid (vms UIC)
**	6-nov-1992 (bryanp)
**	    Check CSinitate() return code.
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	    
**	    the following fields in SCS_ICS have been renamed:
**		ics_sucode	--> ics_iucode
**	16-dec-1992 (bryanp)
**	    Ensure that ics_eusername is set to &ics_susername.
**	2-Jul-1993 (daveb)
**	    cast arg to CSset_sid, prototyped.  Use proper PTR for allocation.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**	16-nov-93 (swm)
**	    Bug #56450
**	    Changed hard-wired size value of 32 passed to sc0m_allocate()
**	    to sizeof(SCD_SCB) (the type indicated is CS_SCB_TYPE).
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	10-feb-1997 (canor01)
**	    Remove NT reference to hPurgeEvent.  This is now created in
**	    GCinitiate and not needed here.
**	14-feb-1997 (canor01)
**	    As part of merging of Ingres-threaded and OS-threaded servers,
**	    make initialization dependent on run-time, not compile-time.
**	    Add call to CS_is_mt() to read threading status.
**	09-Apr-1998 (jenjo02)
**	    Set SC_IS_MT in sc_capabilities if OS threads in use.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id. Note that for NT_GENERIC, cs_self
**	    remains the thread_id, at least for now.
**	20-dec-1999 (somsa01)
**	    cs_self is now (CS_SID *)scb for NT_GENERIC.
*/
i4
scf_init_dbg(void)
{
    i4		status;
    SIZE_TYPE		size;
    SCS_MCB             *mcb_head;
    SCD_SCB		*scb, *scb_q_head;
    DB_OWN_NAME		*junk;
    SCF_CB		scf_cb;
    GLOBALREF const
			char	Version[];
    PTR			block;

    if (Sc_main_cb)
	return(E_DB_OK);
    status = CSinitiate((i4 *)0, (char ***)0, (CS_CB *)0);
    if (status)
	return (status);
    status = sc0m_get_pages(SC0M_NOSMPH_MASK,
			NEEDED_SIZE,
			&size,
			&block);
    Sc_main_cb = (SC_MAIN_CB *)block;
    if ((status) || (size != NEEDED_SIZE))
	return(E_SC0204_MEMORY_ALLOC);

    MEfill(sizeof(SC_MAIN_CB), 0, Sc_main_cb);

    ult_init_macro(&Sc_main_cb->sc_trace_vector, SCF_NBITS, SCF_VPAIRS, SCF_VONCE);
    PCpid(&Sc_main_cb->sc_pid);
    MEmove(STlength(Version),
		(PTR)Version,
		' ',
		sizeof(Sc_main_cb->sc_iversion),
		Sc_main_cb->sc_iversion);
    Sc_main_cb->sc_scf_version = SC_SCF_VERSION;
    Sc_main_cb->sc_nousers = 1;
    MEcopy("JPT_SERVER", 11, Sc_main_cb->sc_sname);

    /* Let all of SCF know if OS threads are in use */
    if (CS_is_mt())
	Sc_main_cb->sc_capabilities |= SC_IS_MT;

    for (;;)
    {
	status = sc0m_initialize();	    /* initialize the memory allocation */
	if (status != E_SC_OK)
	    break;

	/*
	** now, out of our pool, allocate the session control block for this fake
	** session.  It will be filled with information (fake, where appropriate)
	** concerning the session at hand.
	*/

	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4)sizeof(SCD_SCB),
				CS_SCB_TYPE,
				(PTR) SCD_MEM,
				CS_TAG,
				(PTR *) &scb);
	if (status != E_SC_OK)
	    break;

	/* now we do nasty things, but just for single user module testing */

# ifdef NT_GENERIC
#define CS_EVENT_NAME_W2K "%sINGRES_THREAD_ID_%d"
	{
	char			szEventName[MAX_PATH];
	SECURITY_ATTRIBUTES	sa;
	char			*ObjectPrefix;
	FUNC_EXTERN VOID	GVshobj(char **ObjectPrefix);

	iimksec (&sa);
	GVshobj(&ObjectPrefix);

	scb->cs_scb.cs_thread_id   = GetCurrentThreadId();
	scb->cs_scb.cs_self        = (CS_SID) scb;
	CSset_scb(scb);
        CSset_sid(scb->cs_scb.cs_self);
	DuplicateHandle (GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(),
			&scb->cs_scb.cs_thread_handle,
			(DWORD) NULL,
			TRUE,
			DUPLICATE_SAME_ACCESS);


    STprintf(szEventName, CS_EVENT_NAME_W2K, ObjectPrefix, 
		     scb->cs_scb.cs_thread_id);

	scb->cs_scb.cs_event_sem = CreateEvent(&sa,
	                                FALSE,
	                                FALSE,
	                                szEventName);

	scb->cs_scb.cs_di_event = CreateEvent(&sa,FALSE,FALSE,NULL);

	scb->cs_scb.cs_access_sem = CreateMutex(&sa,
	                                 FALSE,
	                                 NULL);
	}

# else /* NT_GENERIC */
# ifdef OS_THREADS_USED
	if (Sc_main_cb->sc_capabilities & SC_IS_MT)
	{
	    scb->cs_scb.cs_thread_id = CS_get_thread_id();
	    scb->cs_scb.cs_self = (CS_SID) scb;
	    PCpid(&scb->cs_scb.cs_pid);
	    CSMTset_scb(scb);
#if !defined(VMS)
	    CSMT_get_wakeup_block(scb);
#endif
	}
	else
	{
	    CSset_sid((CS_SCB *)scb);
	    scb->cs_scb.cs_self = (CS_SID) scb;
	    scb->cs_scb.cs_state = CS_COMPUTABLE;
	}
# else /* OS_THREADS_USED */
	CSset_sid((CS_SCB *)scb);
	scb->cs_scb.cs_self = (CS_SID) scb;
	scb->cs_scb.cs_state = CS_COMPUTABLE;
# endif /* OS_THREADS_USED */
# endif /* NT_GENERIC */

	/* now allocate the queues and que headers so that work can proceed normally */

	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4)sizeof(SCD_SCB),
				CS_SCB_TYPE,
				(PTR) SCD_MEM,
				CS_TAG,
				(PTR *) &scb_q_head);
	if (status != E_SC_OK)
	    break;
	scb_q_head->cs_scb.cs_next = scb_q_head->cs_scb.cs_prev = (CS_SCB *) scb;
	
	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4)sizeof(SCS_MCB),
				MCB_ID,
				(PTR) SCD_MEM,
				MCB_ASCII_ID,
				(PTR *) &mcb_head);
	if (status != E_SC_OK)
	    break;
	mcb_head->mcb_next = mcb_head->mcb_prev = mcb_head;

	mcb_head->mcb_facility = DB_SCF_ID;
	Sc_main_cb->sc_mcb_list = mcb_head;
	
        CSw_semaphore( &Sc_main_cb->sc_mcb_semaphore, CS_SEM_SINGLE, "SC MCB" );

	/* Initialize session's MCB */
	mcb_head = &scb->scb_sscb.sscb_mcb;
	mcb_head->mcb_next = mcb_head->mcb_prev = mcb_head;
	mcb_head->mcb_sque.sque_next = mcb_head->mcb_sque.sque_prev = mcb_head;
	mcb_head->mcb_session = scb->cs_scb.cs_self;
	mcb_head->mcb_facility = DB_SCF_ID;

        CSw_semaphore( &Sc_main_cb->sc_kdbl.kdb_semaphore, CS_SEM_SINGLE, "SC KDB" );
	Sc_main_cb->sc_kdbl.kdb_next = (SCV_DBCB *) &Sc_main_cb->sc_kdbl;
	Sc_main_cb->sc_kdbl.kdb_prev = (SCV_DBCB *) &Sc_main_cb->sc_kdbl;

	scb->scb_sscb.sscb_ics.ics_eusername = 
		&scb->scb_sscb.sscb_ics.ics_susername;
	junk = scb->scb_sscb.sscb_ics.ics_eusername;
	IDname((char **)&junk);
	IDuser(&scb->scb_sscb.sscb_ics.ics_ruid);
	MEcopy("jp", 3, &scb->scb_sscb.sscb_ics.ics_iucode);

	break;
    }

    if (status)
    {
	sc0ePut(NULL, E_SC0124_SERVER_INITIATE, NULL, 0);
	if (status & E_SC_MASK)
	{
	    sc0ePut(NULL, E_SC0204_MEMORY_ALLOC, NULL, 0);
	    sc0ePut(NULL, status, NULL, 0);
	}
	else
	{
	    sc0ePut(NULL, E_SC0200_SEM_INIT, NULL, 0);
	    sc0ePut(&scf_cb.scf_error, 0, NULL, 0);
	}
    }
	
    return(status);
}
