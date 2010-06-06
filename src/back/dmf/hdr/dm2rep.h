/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** dm2rep.h -  defines for DBMS replication
**
** Description:
**	defines routines and other objects needed to perform DBMS replication
**
** History:
**	2-may-96 (stephenb)
**	   created for phase 1 of DBMS replication
**	24-jan-97 (stephenb)
**	   Add definitions for replicator catalogs.
**	11-feb-97 (stephenb)
**	   add file extension to DMC_REP_MEM (VMS needs it).
**	21-apr-97 (stephenb)
**	   Move replicator catalog defines to rep.h in common!hdr!hdr
**	20-jun-97 (stephenb)
**	    Add CDDS_INFO struct.
**	5-mar-98 (stephenb)
**	    Add structs RECOVERY_TIDS and RECOVERY_TXQ
**	1-jun-98 (stephenb)
**	    Add extra parm to dm2rep_qman()
**	4-dec-98 (stephenb)
**	    trans_time is now an HRSYSTIME
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-jul-2006 (kibro01) bug 114168
**	    Add extra parm to dm2rep_qman() to know whether this is
**          a hijacked open_db.
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2rep_? functions converted to DB_ERROR *
**	20-May-2010 (thaju02) Bug 123427
**	    Add lk_id param to dm2rep_qman().
*/

/*
** replication modes
*/
#define		DM2REP_INSERT		1
#define		DM2REP_UPDATE		2
#define		DM2REP_DELETE		3
/*
** replication shared memory segment
*/
#define		DMC_REP_MEM		"Repshmem.mem"
/*
** lock events for LKevent
*/
#define		REP_READQ		1
#define		REP_READQ_VAL		1
#define		REP_FULLQ		2
#define		REP_FULLQ_VAL		2

/*
** function prototypes
*/
FUNC_EXTERN DB_STATUS		dm2rep_capture(
        				i4             action,
        				DMP_RCB         *rcb,
					char		*record,
        				DB_ERROR        *dberr);
FUNC_EXTERN DB_STATUS		dm2rep_qman(
        				DMP_DCB         *dcb,
        				i4         tx_id,
        				HRSYSTIME       *trans_time,
					DMP_RCB		*input_q_rcb,
					i4		lk_id,
        				DB_ERROR        *dberr,
					bool		from_open);

/*
** Name: DMC_REP - replication global control block
**
** Description:
**	contains global definitions for DBMS replication
**
** History:
**	22-may-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
*/
typedef struct _DMC_REP
{
    CS_SEMAPHORE	rep_sem; /* semaphore to protect this global */
    i4			seg_size; /* no of records in shared memory */
    i4			queue_start; /* fisrt element in circular queue */
    i4			queue_end; /* last element in circular queue */
} DMC_REP;

/*
** Name: DMC_REPQ - circular queue of transaction records
**
** Description:
**	each occurence of this structure describes an element of
**	the replication transaction queue that resides in the replication
**	shared memory segment, this is a circular queue which is semaphore
**	protected.
**
** History:
**	22-may-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
**	1-dec-98 (stephenb)
**	    make trans_time a HRSYSTIME so that we can hold nano-second 
**	    granularity.
*/
typedef struct _DMC_REPQ
{
    i4	tx_id; /* low part of transaction to process */
    HRSYSTIME	trans_time; /* date of commit */
    DB_DB_NAME	dbname;
    bool	active; /* transaction is being handled */
} DMC_REPQ;

/*
** Name: RECOVERY_TXQ - Replicator recovery transaction queue
**
** Description:
**	Contains a list of recovered trnsactions. It's used to fake the
**	commit date from the most recent update date when DMC_REPQ is
**	unavailable (i.e. during restart recovery)
**
** History:
**	5-mar-98 (stephenb)
**	    Created.
*/
typedef struct _RECOVERY_TXQ
{
    i4	tx_id;
    char	last_date[12];
} RECOVERY_TXQ;
#define		TXQ_SIZE	100

/*
** Name: RECOVERY_TIDS
**
** Description:
**	Contains list of TIDs and transaction IDs for records added to the
**	distribution queue during restart recovery.
**
** History:
**	5-mar-98 (stephenb)
**	    Created
*/
typedef struct _RECOVERY_TIDS
{
    i4	tx_id;
    DM_TID	tid;
} RECOVERY_TIDS;
#define		TIDQ_SIZE	500

/*
** Name: CDDS_INFO - cdds information
**
** Description:
**	The structure contains CDDS information, it is used to determin
**	whether paths to other databases in the CDDS are present and 
**	correct. The information is derived from the dd_db_cdds table.
**
** History:
**	29-may-97 (stephenb)
**	    Initial creation
*/
typedef struct _CDDS_INFO	CDDS_INFO;
struct _CDDS_INFO
{
    CDDS_INFO	*cdds_next;
    CDDS_INFO	*cdds_prev;
    i2          cdds_no;
    i2          database_no;
    i2          target_type;
};
