/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:  DMXCB.H - Typedefs for DMF transaction support.
**
** Description:
**      This file contains the control block used for all transaction
**      operations.  Note, not all the fields of the control block need
**      to be set for every operation.  Refer to the Interface Design
**      Specification for inputs required for a specific operation.
**
** History:
**      01-sep-85 (jennifer)
**         Created.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      20-Jan-89 (ac)
**          Added 2PC support.
**	27-Feb-1997 (jenjo02)
**	    Added DMX_NOLOGGING for non-logged transactions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**/

/*}
** Name:  DMX_CB - DMF transaction call control block
**
** Description:
**      This typedef defines the control block required for the following
**      DMF transaction operation:
**
**      DMX_ABORT - Abort transaction.
**      DMX_BEGIN - Begin transaction.
**      DMX_COMMIT - Commit transaction.
**      DMX_SAVEPOINT - Save the state of a transaction.
**	DMX_XA_START - Start or join an XA transaction
**	DMX_XA_END - Disassociate from an XA transaction
**	DMX_XA_PREPARE - Prepare to commit an XA transaction
**	DMX_XA_COMMIT - Commit an XA transaction
**	DMX_XA_ROLLBACK - Rollback an XA transaction.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	27-Feb-1997 (jenjo02)
**	    Added DMX_NOLOGGING for non-logged transactions.
**	29-Jan-2004 (jenjo02)
**	    Add DMX_CONNECT to connect to existing transaction
**	    contexts.
**	26-Jun-2006 (jonj)
**	    Add DMX_XA_END_SUCCESS, DMX_XA_END_SUSPEND, 
**	    DMX_XA_END_FAIL option bits.
**	6-Jul-2006 (kschendel)
**	    Comment update.
*/
typedef struct _DMX_CB
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMX_TRANSACTION_CB  12
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         ascii_id;             
#define                 DMX_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'X')
    DB_ERROR        error;                  /* Common DMF error block. */
    PTR             dmx_session_id;         /* SCF SESSION ID obtained from
                                            ** SCF.  Every transaction
                                            ** must be associated with a 
                                            ** session. */
    PTR             dmx_db_id;              /* DMF database id for session.
					/* Really an ODCB pointer, so it's the
					** same for all threads of a session.
					*/
    i4         dmx_flags_mask;         /* Modifier to operation. */
#define                 DMX_READ_ONLY       0x01L
    i4         dmx_option;             /* Modifier to operation. */
#define                 DMX_SP_ABORT        0x0001L
#define                 DMX_USER_TRAN       0x0002L
#define			DMX_NOLOGGING 	    0x0004L
#define			DMX_CONNECT	    0x0008L
					    /* Connect to existing
					    ** log/log context; 
					    ** dmx_tran_id is that
					    ** context and will contain
					    ** new context on return */
#define			DMX_XA_END_SUCCESS  0x0010L
					    /* xa_end(TMSUCCESS) */
#define			DMX_XA_END_SUSPEND  0x0020L 
					    /* xa_end(TMSUSPEND) */
#define			DMX_XA_END_FAIL	    0x0040L
					    /* xa_end(TMFAIL) */
#define			DMX_XA_COMMIT_1PC   0x0080L
					    /* xa_commit(TMONEPHASE) */
#define			DMX_XA_START_XA	    0x0100L
					    /* xa_start */
#define			DMX_XA_START_JOIN   0x0200L
					    /* xa_start(TMJOIN) */
#define			DMX_XA_START_RESUME 0x0400L
					    /* xa_start(TMRESUME) */
    PTR	            dmx_tran_id;            /* DMF TRANSACTION ID returned as
                                            ** a result of a begin transaction
                                            ** operation. */
    DB_TRAN_ID      dmx_phys_tran_id;       /* Physical transaction identifier */
    DB_SP_NAME      dmx_savepoint_name;     /* Savepoint name. */
    DM_DATA         dmx_data;               /* Savepoint data. */
    DB_TAB_TIMESTAMP 
                    dmx_commit_time;        /* Timestamp indicating commit 
                                            ** time. */
    DB_DIS_TRAN_ID  dmx_dis_tran_id;       /* Distributed transaction identifier */
}   DMX_CB;
