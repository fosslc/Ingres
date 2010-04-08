/*
** Copyright (c) 1987, 2008 Ingres Corporation
**
**
*/

/**
** Name: DMFCSP.H - Definition of CSP data structures.
**
** Description:
**      This module defines the data structures used by the CSP.
**
** History: $Log-for RCS$
**      02-jun-1987 (Derek)
**          Created for Jupiter.
**      14-feb-1991 (jennifer)
**          Added new state and event to handle setting a timer
**          to delay dying after a disconnect occurs.
**	1-nov-1991 (bryanp)
**	    Rename NODE to CSP_NODE so that this file can be included together
**	    with <lo.h>, which also defines NODE.
**	7-jul-1992 (ananth)
**	    Prototyping DMF.
**	26-apr-1993 (bryanp)
**	    Added MST_T_RESUME_LOCKS message type.
**	15-may-1993 (rmuth)
**	    Add define for CLUSTER_PM_NAME_LENGTH,CLUSTER_PM_VALUE_LENGTH
**	26-jul-1993 (bryanp)
**	    Replaced MAX_NODE_NAME_LENGTH with LGC_MAX_NODE_NAME_LEN.
**	21-aug-1996 (boama01)
**	    Added E_DM9930_CSP_EXCEPTION_ABORT for CSP exception handling.
**	02-may-2001 (devjo01)
**	    Major changes for s103715.
**	    - Remove VMS specific structures.
**	28-may-2002 (devjo01)
**	    Add CSP_ACT_6_ALERT.
**	25-jan-2004 (devjo01)
**	    Make most of this header file conditional on conf_CLUSTER_BUILD.
**	14-may-2004 (devjo01)
**	    Add CSP_ACT_7_LEAVE.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**      30-sep-2004 (stial01)
**          Moved cluster online checkpoint defines for non-cluster builds
**	31-Mar-2005 (jenjo02)
**	    Externalized csp_initialize function, now called by
**	    dmr_log_init rather than the CSP thread.
**	01-Apr-2005 (jenjo02)
**	    Made csp_initialize a dummy if not conf_CLUSTER_BUILD.
**	22-Apr-2005 (fanch01)
**	    Added CSP_ACT_9_CLASS_SYNC_REQUEST, CSP_ACT_10_CLASS_SYNC_REPLY,
**	    CSP_ACT_11_CLASS_START, CSP_ACT_12_CLASS_STOP,
**	    CSP_ACT_13_DBMS_START, dmfcsp_class_start, dmfcsp_class_stop
**	    for dbms class recovery.
**      11-may-2005 (stial01)
**          updated dmfcsp_class_start, dmfcsp_class_stop prototypes
**	25-may-2005 (devjo01)
**	    Remove unused struct MSG_SENT.
**	28-may-2008 (joea)
**	    Add I_DM9931/36 informational messages.
**/


#if defined(conf_CLUSTER_BUILD)

typedef struct _ACTION	ACTION;		/*  CSP action item. */


/*}
** Name: ACTION - A CSP action item.
**
** Description:
**      This routine is structure is returned to the caller when
**	the asynchronous CSP engine determines that a high level
**	action is required.
**
** History:
**      10-jun-1987 (Derek)
**          Created for Jupiter.
[@history_template@]...
*/
struct _ACTION
{
    ACTION *act_next;			/* Next action item. */
    ACTION *act_prev;			/* Previous action item. */
    i4	    act_type;			/* Type of action desired. */
#define			CSP_ACT_1_NEW_MASTER	1
#define			CSP_ACT_2_NODE_FAILURE	2
#define			CSP_ACT_3_RCVRY_CMPL	3
#define			CSP_ACT_4_SHUTDOWN	4
#define			CSP_ACT_5_IVB_SEEN	5
#define			CSP_ACT_6_ALERT		6
#define			CSP_ACT_7_LEAVE		7
#define			CSP_ACT_8_CKP		8
#define			CSP_ACT_9_CLASS_SYNC_REQUEST	9
#define			CSP_ACT_10_CLASS_SYNC_REPLY	10
#define			CSP_ACT_11_CLASS_START	11
#define			CSP_ACT_12_CLASS_STOP	12
#define			CSP_ACT_13_DBMS_START	13

    i4	    act_node;			/* Target node for action. */
    CS_SID  act_resume;			/* If non-zero, resume this
					   SID after action processed. */
    
    CSP_CX_MSG  act_msg;                /* For some actions, the original msg
                                        ** may contain additional action args */
};

/*
**  Error messages related to CSP operation.
*/

#define		    E_DM9900_CSP_INITIALIZE		(E_DM_MASK + 0x9900L)
#define		    E_DM9901_CSP_ACTION_FAILURE		(E_DM_MASK + 0x9901L)
#define		    E_DM9902_CSP_CONFIG_ERROR		(E_DM_MASK + 0x9902L)
#define		    E_DM9903_CSP_CONFIG_FORMAT		(E_DM_MASK + 0x9903L)
#define		    E_DM9904_CSP_DECNET			(E_DM_MASK + 0x9904L)
#define		    E_DM9905_CSP_NOT_NODE		(E_DM_MASK + 0x9905L)
#define		    E_DM9906_CSP_RCPCONFIG		(E_DM_MASK + 0x9906L)
#define		    E_DM9907_CSP_CREATE_RCP		(E_DM_MASK + 0x9907L)
#define		    E_DM9908_CSP_CREATE_ACP		(E_DM_MASK + 0x9908L)
#define		    E_DM9909_CSP_TIMER			(E_DM_MASK + 0x9909L)
#define		    E_DM990A_CSP_STALL_CLEAR		(E_DM_MASK + 0x990AL)
#define		    E_DM990B_CSP_DEADLOCK_CLEAR		(E_DM_MASK + 0x990BL)
#define		    E_DM990C_CSP_MASTER_LOCK		(E_DM_MASK + 0x990CL)
#define		    E_DM990D_CSP_NO_MSGS		(E_DM_MASK + 0x990DL)
#define		    E_DM990E_CSP_READ_ACTION		(E_DM_MASK + 0x990EL)
#define		    E_DM990F_CSP_NETSHUT		(E_DM_MASK + 0x990FL)
#define		    E_DM9910_CSP_BADMSG			(E_DM_MASK + 0x9910L)
#define		    E_DM9911_CSP_CANIO			(E_DM_MASK + 0x9911L)
#define		    E_DM9912_CSP_READ_QUEUE		(E_DM_MASK + 0x9912L)
#define		    E_DM9913_CSP_DEBUG_ON		(E_DM_MASK + 0x9913L)
#define		    E_DM9914_CSP_WRITE_COMPLETE		(E_DM_MASK + 0x9914L)
#define		    E_DM9915_CSP_CONNECT_FAIL		(E_DM_MASK + 0x9915L)
#define		    E_DM9916_CSP_ACCEPT_FAIL		(E_DM_MASK + 0x9916L)
#define		    E_DM9917_CSP_REJECT_FAIL		(E_DM_MASK + 0x9917L)
#define		    E_DM9918_CSP_WRITE_ERROR		(E_DM_MASK + 0x9918L)
#define		    E_DM9919_CSP_WRITE_ACTION		(E_DM_MASK + 0x9919L)
#define		    E_DM991A_CSP_NODE_LOCK		(E_DM_MASK + 0x991AL)
#define		    E_DM991B_CSP_FAILOVER		(E_DM_MASK + 0x991BL)
#define		    E_DM991C_CSP_MASTER_SIX		(E_DM_MASK + 0x991CL)
#define		    E_DM991D_CSP_NO_NODE_LOCK		(E_DM_MASK + 0x991DL)
#define		    E_DM991E_CSP_BAD_BROADCAST		(E_DM_MASK + 0x991EL)
#define		    E_DM991F_CSP_CSID			(E_DM_MASK + 0x991FL)
#define		    E_DM9920_CSP_GETLKI			(E_DM_MASK + 0x9920L)
#define		    E_DM9921_CSP_REFLECT_CONVERT	(E_DM_MASK + 0x9921L)
#define		    E_DM9922_CSP_BAD_DEADLOCK		(E_DM_MASK + 0x9922L)
#define		    E_DM9923_CSP_ACTION_ORDER		(E_DM_MASK + 0x9923L)
#define		    E_DM9924_CSP_BAD_CONNECT		(E_DM_MASK + 0x9924L)
#define		    E_DM9925_CSP_MASTER_PATHLOST	(E_DM_MASK + 0x9925L)
#define		    E_DM9926_CSP_CONNECT_MASTER		(E_DM_MASK + 0x9926L)
#define             E_DM9927_CSP_SET_TIMER              (E_DM_MASK + 0x9927L)
#define		    E_DM9930_CSP_EXCEPTION_ABORT	(E_DM_MASK + 0x9930L)
#define		    I_DM9931_CSP_PERF_NODE_RECOV	(E_DM_MASK + 0x9931L)
#define		    I_DM9932_CSP_RECOV_NODE		(E_DM_MASK + 0x9932L)
#define		    I_DM9933_CSP_RECOV_DBMS_CLASS	(E_DM_MASK + 0x9933L)
#define		    I_DM9934_CSP_RESTART_RECOV_COMPL	(E_DM_MASK + 0x9934L)
#define		    I_DM9935_CSP_NODE_RECOV_COMPL	(E_DM_MASK + 0x9935L)
#define		    I_DM9936_CSP_LOCK_PROC_RESUMED	(E_DM_MASK + 0x9936L)


/* Function prototype definitions. */

FUNC_EXTERN DB_STATUS csp_initialize(bool *recovery_needed);

FUNC_EXTERN DB_STATUS dmfcsp(DMC_CB *);

FUNC_EXTERN DB_STATUS dmfcsp_msg_monitor(DMC_CB *);

FUNC_EXTERN DB_STATUS dmfcsp_msg_thread(DMC_CB *);

FUNC_EXTERN VOID dmfcsp_shutdown(VOID);

FUNC_EXTERN DB_STATUS dmfcsp_class_start(char *class_name, i4 node, 
		PID pid, i2 lg_id_id);
FUNC_EXTERN DB_STATUS dmfcsp_class_stop(char *class_name, i4 node, 
		PID pid, i2 lg_id_id);

#else

/*
** If not a cluster build, CSP entry points should not be called.
** if by chance they are, then these macros set an error status.
*/
# define dmfcsp(dmc_cb)			E_DM002A_BAD_PARAMETER
# define dmfcsp_msg_monitor(dmc_cb)	E_DM002A_BAD_PARAMETER
# define dmfcsp_msg_thread(dmc_cb)	E_DM002A_BAD_PARAMETER
# define dmfcsp_class_start(class_name, pid)	E_DM002A_BAD_PARAMETER
# define dmfcsp_class_stop(class_name, pid)	E_DM002A_BAD_PARAMETER

/* Simply ignore dmfcsp_shutdown, csp_initialize. */
# define dmfcsp_shutdown()
# define csp_initialize(recovery_needed)	OK

#endif /* conf_CLUSTER_BUILD */
