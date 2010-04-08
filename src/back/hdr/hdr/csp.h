/*
** Copyright (c) 2001, 2005 Ingres Corporation
**
**
*/

/**
** Name: CSP.H - CSP definitions visible through entire backend.
**
** Description:
**      This module defines the data structures used by the CSP "process",
**	which need to be visble at both the SCF, and DMF levels.
**
** History: $Log-for RCS$
**	02-may-2001 (devjo01)
**	    Major chabges for s103715.
**	    - Add CSP_CHANNEL, CSP_CX_MSG.
**	    - Remove VMS specific structures.
**	01-sep-2004 (stial01)
**	    Support cluster online checkpoint
**      11-may-2005 (stial01)
**          Added CSP_MSG_12_DBMS_RCVR_CMPL, reduced CSP_CX_MSG_CLASS_BYTES
**          to make room in class event for lg_id_id
**	25-may-2005 (devjo01)
**	    Convert CSP_CX_MSG to a union, and perform other tricks
**	    to get size under 16 bytes.  Fanch01 changed inline
**	    class names with chits for same.
[@history_template@]...
**/


/* CX MSG channel used for cluster state transition messages. */
#define	CSP_CHANNEL	0

typedef union _CSP_CX_MSG CSP_CX_MSG;	/*  CX MSG used by CSP, etc. */ 


/*}
** Name: CSP_CX_MSG	- CX_MSG structure used with the Ingres cluster.
**
** Description:
**      This UNION describes the format of the CX MSG varient used
**	with the Ingres cluster configuration.
**
**	IMPORTANT NOTE: All variants must reserve two bytes at the
**	start for the csp_msg_action & csp_msg_node.
**
** History:
**      02-may-2001 (devjo01)
**          Created.
**      22-Apr-2005 (fanch01)
**          Added CSP_MSG_8_CLASS_SYNC_REPLY, CSP_MSG_9_CLASS_START,
**          CSP_MSG_10_CLASS_STOP, CSP_MSG_11_DBMS_START defines to
**          support cluster dbms class failover.  Added class_sync,
**          class_event, dbms_start to support cluster dbms class failover.
**          Reordered structure members to reduce compiler padding and
**          bring the message within the 32 byte limit.
**      25-May-2005 (fanch01)
**          Add long message class name functionality. Structure reordered.
**	25-may-2005 (devjo01)
**	    Further pack structures to get into 16 bytes for VMS.
**	10-Dec-2007 (jonj)
**	    Move CKP_CSP_? defines here from dmfjsp.h,
**	    add CKP_PHASE_NAME strings for lockstat, etc.
**
[@history_template@]...
*/
union _CSP_CX_MSG
{
    u_i1	csp_generic[CX_DLM_VALUE_SIZE];
# define csp_msg_action csp_generic[0]
#   define	CSP_MSG_1_JOIN			1
#   define	CSP_MSG_2_LEAVE			2
#   define	CSP_MSG_3_RCVRY_CMPL		3
#   define	CSP_MSG_4_NEW_MASTER		4
#   define	CSP_MSG_5_RPTIVB		5
#   define	CSP_MSG_6_ALERT 		6
#   define	CSP_MSG_7_CKP			7
#   define	CSP_MSG_8_CLASS_SYNC_REPLY	8
#   define	CSP_MSG_9_CLASS_START		9
#   define	CSP_MSG_10_CLASS_STOP		10
#   define	CSP_MSG_11_DBMS_START		11
#   define	CSP_MSG_12_DBMS_RCVR_CMPL	12
# define csp_msg_node csp_generic[1]
    struct
    {
	u_i2		csp_reserved;
	CX_NODE_BITS	node_bits;
    }	rcvry;
    struct
    {
	u_i2		csp_reserved;
	i4			chit;
	i4			length;
    }	alert;
    struct
    {
	u_i2		csp_reserved;
	union
	{
	    i4              init_chit;		/* phase == CSP_CKP_INIT */
	    i4              sbackup_flag;	/* phase == CSP_CKP_SBACKUP */
	    i4              status_rc;		/* phase == CSP_CKP_STATUS */
	    i4		    abort_phase;	/* phase == CSP_CKP_ABORT */
	    i4              misc;		/* all other CKP phases */
	} ckp_var;
	i4			dbid;
	i1			phase;	
/* Phases of online cluster checkpoint */
#define CKP_CSP_NONE			0
#define CKP_CSP_INIT			1
#define CKP_CSP_STALL			2
#define CKP_CSP_SBACKUP			3
#define CKP_CSP_RESUME			4
#define CKP_CSP_CP			5
#define CKP_CSP_EBACKUP			6
#define CKP_CSP_DBACKUP			7
#define CKP_CSP_STATUS			8
#define CKP_CSP_ERROR			9
#define CKP_CSP_ABORT			10

/* Associated tags for lockstat, etc */
#define	CKP_PHASE_NAME "\
0,INIT,STALL,SBACKUP,RESUME,CP,EBACKUP,DBACKUP,STATUS,\
ERROR,ABORT"

	i1			ckp_node;
	i1			flags;
#define			CKP_CRASH	0x01	/* For testing error handling */
    }	ckp;		/* ckpdb - 14 bytes max */
    struct
    {
	u_i2		csp_reserved;
	u_i4		roster_id;	/* after join, watch for
					** roster w/this id
						*/
    }	join;
    struct
    {
	u_i2		csp_reserved;
	u_i2		node;		/* dest node */
	u_i4		roster_id;	/* class roster id */
	i4		chit;		/* long msg chit */
	i4		count;		/* class definition count */
    }	class_sync;
    struct
    {
	u_i2		csp_reserved;
#define CSP_CX_MSG_CLASS_BYTES \
             ( CX_DLM_VALUE_SIZE - 2*sizeof(i2) - sizeof(PID) )
	i2		lg_id_id;
	PID		pid;
	i4		class_name;	/* Chit to redeem class name. */
    }	class_event;
    struct
    {
	u_i2		csp_reserved;
	u_i2		node;
	i4		class_name;	/* Chit to redeem class name. */
    }	dbms_start;
};

