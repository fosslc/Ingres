/*
**Copyright (c) 2004 Ingres Corporation
*/
/**
**
** Name: DMRTHREADS.H	- DMR Definitions for Threaded Record Access
**
** Description:
**	Contains types and functions used by threaded
**	DMR services.
**
** History:
**	12-Nov-2003 (jenjo02)
**	    Created for Partitioned Table project.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Standarize function output error to
**	    DB_ERROR * rather than i4 *.
*/
typedef struct _DMR_RTL		DMR_RTL;
typedef struct _DMR_RTB		DMR_RTB;

/*}
** Name: DMR_RTL  - RTB Threading Link
**
** Description:
**	Links dmrThreads attached to a RTB.
**
** History:
**	12-Nov-2003 (jenjo02)
**	    Created for Parallel/Partitioning project.
*/
struct _DMR_RTL
{
    DMR_RTL	*rtl_next;	/* Next RTL on the list */
    DMR_RTL	*rtl_prev;	/* Prev RTL on the list */
    DMP_TCB	*rtl_tcb;	/* Agent's assigned TCB */
    DMP_RCB	*rtl_rcb;	/* Agent's opened RCB */
    CS_SID	rtl_sid;	/* Agent's session id */
    i4		rtl_thread_id;	/* ...for debugging */
};
    

/*}
** Name: DMR_RTB  - RCB Threading Block
**
** Description:
**	When threading RCBs, this structure is allocated and
**	pointed to by the associated RCB (rcb_rtb_ptr).
**	It is used to synchronize the execution of
**	dmrThreads with the main Query Thread.
**
** History:
**	12-Nov-2003 (jenjo02)
**	    Created for Parallel/Partitioning project.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define RTB_CB as DM_RTB_CB.
**	26-Nov-2008 (jonj)
**	    SIR 120874: rtb_err_code expanded to rtb_error.
*/
struct _DMR_RTB
{
    PTR     	rtb_q_next;		/* Next in idle RTB list */
    PTR     	rtb_q_prev;		/* unused */
    SIZE_TYPE   rtb_length;		/* Length of control block. */
    i2		rtb_type;               /* Type of control block for
                                        ** memory manager. */
#define		RTB_CB              DM_RTB_CB
    i2		rtb_s_reserved;         /* Reserved for future use. */
    PTR		rtb_l_reserved;         /* Reserved for future use. */
    PTR		rtb_owner;              /* Owner of control block for
                                        ** memory manager.  RTB will be
                                        ** owned by its TCB */
    i4		rtb_ascii_id;           /* Ascii identifier for aiding
                                        ** in debugging. */
#define		RTB_ASCII_ID        CV_C_CONST_MACRO('#', 'R', 'T', 'B')
    DMP_RCB	*rtb_rcb;		/* The threaded Base RCB */
    DMR_RTL	*rtb_rtl;		/* Queue of attached threads */
    i4		rtb_dmr_op;		/* The DMR_? operation */
    i4		rtb_dm2r_flags;		/* Flags for DM2R_? function */
    i4		rtb_dmr_seq;		/* n-th operation on DMR */
    DMR_CB	*rtb_dmr;		/* The DMR_CB of the
					** operation in progress */
    PTR		rtb_async_fcn;		/* Async completion function */
    DB_STATUS	rtb_status;		/* The operation's status */
    DB_ERROR	rtb_error;		/* ...and error information */
    i4		rtb_si_dupchecks;	/* Number of SI dupchecks
					** yet to complete. */
    i4		rtb_si_agents;		/* Number of attached SI agents */
    i4		rtb_agents;		/* Number of attached agents */
    i4		rtb_active;		/* Number of agents working on dmr */
    i4		rtb_state;		/* The state of things: */
#define	RTB_BUSY	0x00000001L	/* RTB being (re)constructed */
#define	RTB_IDLE	0x00000002L	/* RTB and dmrThreads are idle */
#define	RTB_CLOSE	0x00000004L	/* Time to become idle */
#define	RTB_TERMINATE	0x00000008L	/* End attached dmrThread(s) */

    i4		rtb_cond_waiters;	/* Number of condition waiters */
    CS_SEMAPHORE rtb_cond_sem;
    CS_CONDITION rtb_cond;
};

FUNC_EXTERN DB_STATUS	dmrThrOp(
				i4		op,
				DMR_CB		*dmr);

FUNC_EXTERN DB_STATUS	dmrThrSync(
				DMR_CB		*dmr,
				DMP_RCB		*r);

FUNC_EXTERN DB_STATUS	dmrThrError(
				DMR_CB		*dmr,
				DMP_RCB		*r,
				DB_STATUS	status);

FUNC_EXTERN VOID	dmrThrClose(
				DMP_RCB		*r);

FUNC_EXTERN VOID	dmrThrTerminate(
				DMP_TCB		*t);
