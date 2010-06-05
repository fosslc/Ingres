/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <tm.h>
#include    <sr.h>
#include    <me.h>
#include    <er.h>
#include    <ex.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dmftrace.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm0m.h>
#include    <dm1c.h>
#include    <dm1b.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <sxf.h>
#include    <dma.h>
#include    <cm.h>
#include    <st.h>

/**
** Name:  DMSSEQ.C - Functions used to work with Sequence Generators.
**
** Description:
**
**		Contains functions to work with a Server's
**		Sequence Generators' cache.
**    
**      dms_open_seq()    -  Bind a Sequence Generator to a
**			     transaction.
**      dms_close_seq()   -  Release Sequence object after
**			     CREATE/ALTER/DROP.
**      dms_next_seq()    -  Return next/current value of Sequence.
**
**	dms_end_tran()	  -  Cleanup exclusively bound Sequences for
**			     commit/abort transaction.
**	dms_close_db()	  -  Preserve all cached Sequences linked to
**			     a DCB.
**	dms_destroy_db()  -  Destroy all Sequences linked to
**			     a DCB.
**	dms_stop_server() -  Destroy all Sequences for all closed
**			     databases.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Invented.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	10-May-2004 (schka24)
**	    ODCB scb need not equal thread SCB for factota, remove checks.
**	    Make sure we're using parent lock timeout (should be the same,
**	    but let paranoia reign).
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	24-apr-2007 (dougi)
**	    Replaced DB_MAX_DECxxx's with hard coded 16 & 31 to match
**	    iisequence definition.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	24-Aug-2009 (kschendel) 121804
**	    Need mh.h to satisfy gcc 4.3.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/* static function declarations */
static DML_SEQ *FindSeq( DML_XCB	*xcb,
			 DMS_SEQ_ENTRY	*seq);

static VOID PutTxnCvalue( DMS_CB	  *dms, 
			  DML_XCB	  *xcb,
			  DMS_SEQ_ENTRY*dms_seq,
			  DML_SEQ	  *s,
			  PTR	  	  value);

static VOID GetTxnCvalue( DMS_CB	  *dms,
			  DML_XCB	  *xcb,
			  DMS_SEQ_ENTRY*dms_seq,
			  DML_SEQ	  *s,
			  PTR	  	  value);

/* EX handler for decimal overflow, defined in dmsiiseq.c */
FUNC_EXTERN	STATUS	  dms_exhandler(EX_ARGS	*ex_args);


/*{
** Name: dms_open_seq - Binds a Sequence in anticipation of its use.
**
**  INTERNAL DMF call format:    status = dms_open_seq(&dms_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMS_OPEN_SEQ,&dms_cb);
**
** Description:
**
** Inputs:
**      dms_cb 
**         .type                            Must be set to DMS_SEQ_CB.
**         .length                          Must be at least sizeof(DMS_CB).
**         .dms_tran_id                     Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**         .dms_flags_mask		    
**		DMS_CURR_VALUE		    CURRENT VALUE (DML)
**		DMS_NEXT_VALUE		    NEXT VALUE (DML)
**		DMS_CREATE		    CREATE SEQUENCE (DDL)
**		DMS_ALTER		    ALTER SEQUENCE (DDL)
**		DMS_DROP		    DROP SEQUENCE (DDL)
**         .dms_db_id                       Must be the identifier for the 
**                                          odcb returned from open database.
**         .dms_seq_array.data_address      Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DMS_SEQ_ENTRY.
**                                          See below for description of 
**                                          <dms_seq_array> entries.
**         .dms_seq_array.data_in_size      Length of seq_array data in bytes.
**         <dmu_seq_array> entries are of type DMS_SEQ_ENTRY and 
**         must have following values:
**         seq_name                         Name of the Sequence
**	   seq_owner			    Schema owner
**         seq_id                           Sequence id.
**
** Outputs:
**      dms_cb   
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM0079_BAD_OWNER_NAME
**                                          E_DM010C_TRAN_ABORTED:
**					    
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dms_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dms_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dms_cb.error.err_code.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	06-Feb-2003 (jenjo02)
**	    Cleaned up LK error handling, installed "timeout" as
**	    a parm to LKrequest().
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**      29-Sep-2004 (fanch01)
**          Conditionally add LK_LOCAL flag if database/table is confined
**	    to one node.
**	13-Jul-2006 (kiria01)  b112605
**	    Extended use of the seq_cache member such that a negative value
**	    is to be interpreted as value exhaustion. Zero now uniquiely
**	    means 'cache reload' needed. Positive values indicate remaining
**	    cache. Dropped use of seq_version.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
*/
DB_STATUS
dms_open_seq(
PTR        dms_cb)
{
    DMS_CB          *dms = (DMS_CB*)dms_cb;
    DML_XCB	    *xcb;
    DML_ODCB	    *odcb;
    DMP_DCB	    *dcb;
    DML_SCB	    *scb;
    DML_SEQ	    *s;
    DMS_SEQ_ENTRY   *dms_seq = (DMS_SEQ_ENTRY*)NULL;
    DB_STATUS	    status;
    i4		    error = 0, local_error, lk_error = 0;
    i4		    lock_mode;
    i4		    lock_flags;
    LK_LOCK_KEY	    lock_key;
    LK_VALUE	    lock_value;
    i4		    timeout;
    i4		    count;
    CL_ERR_DESC	    sys_err;
    STATUS	    cl_status;

    CLRDBERR(&dms->error);

    xcb = (DML_XCB*) dms->dms_tran_id;
    scb = xcb->xcb_scb_ptr;
    odcb = (DML_ODCB*)dms->dms_db_id;
    
    if ( dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) )
	SETDBERR(&dms->error, 0, E_DM003B_BAD_TRAN_ID);
    else
    {
	/* Check for external interrupts */
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dms->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dms->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dms->error, 0, E_DM0064_USER_ABORT);
	    else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		SETDBERR(&dms->error, 0, E_DM0132_ILLEGAL_STMT);
	}
    }

    if ( dms->error.err_code == 0 )
    {
	if ( xcb->xcb_x_type & XCB_RONLY &&
	     dms->dms_flags_mask & DMS_DDL )
	    SETDBERR(&dms->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	else if ( dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) )
	    SETDBERR(&dms->error, 0, E_DM0010_BAD_DB_ID);

	/*  Check that this is a update transaction on the database 
	**  that can be updated. */
	else if ( odcb != xcb->xcb_odcb_ptr )
	    SETDBERR(&dms->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	else
	{
	    dcb = odcb->odcb_dcb_ptr;
	    /* There must be at least one SEQ_ENTRY */
	    dms_seq = (DMS_SEQ_ENTRY*)dms->dms_seq_array.data_address;
	    count = dms->dms_seq_array.data_in_size / sizeof(DMS_SEQ_ENTRY);

	    if ( dms_seq == NULL || count <= 0 )
		SETDBERR(&dms->error, 0, E_DM002A_BAD_PARAMETER);
	}
    }

    if ( dms->error.err_code == 0 )
    {
	/*
	** Determine type of Sequence lock.
	**
	** If Sequence DDL (CREATE/ALTER/DROP)
	** or if the database is locked X,
	** get an exclusive lock; if DDL,
	** also make it interruptable.
	**
	** If normal DML open, use an IX
	** lock so that multiple transactions
	** can share and update the sequence, 
	** but changes to the Sequence will
	** be blocked.
	**
	** All locks are logical locks and persist
	** until end of transaction.
	**
	** ALTER SEQUENCE causes the second-half
	** of the sequence id (db_tab_index) to be
	** incremented when the IISEQUENCE tuple
	** is rewritten in qeu_aseq().
	*/
	if ( dms->dms_flags_mask & DMS_DDL )
	{
	    lock_mode = LK_X;
	    lock_flags = dcb->dcb_bm_served == DCB_SINGLE ?
			LK_INTERRUPTABLE | LK_LOCAL :
			LK_INTERRUPTABLE;
	}
	else 
	{
	    if ( odcb->odcb_lk_mode == ODCB_X )
		lock_mode = LK_X;
	    else
		lock_mode = LK_IX;
	    lock_flags = dcb->dcb_bm_served == DCB_SINGLE ? LK_LOCAL : 0;
	}

	/* Use parent session's lock timeout */
	if ( odcb->odcb_scb_ptr->scb_timeout == DMC_C_SYSTEM )
	    timeout = 0;
	else
	    timeout = odcb->odcb_scb_ptr->scb_timeout;

	lock_key.lk_type = LK_SEQUENCE;
	lock_key.lk_key1 = dcb->dcb_id;

	/* Place appropriate lock on each Sequence */
	for ( dms_seq; count; dms_seq++, count-- )
	{
	    MEcopy((char *)&dms_seq->seq_owner, 8, (char *)&lock_key.lk_key2); 
	    MEcopy((char *)&dms_seq->seq_name, 12, (char *)&lock_key.lk_key4);

	    dms_seq->seq_status = 0;


	    if ( (cl_status = LKrequest(lock_flags,
		    xcb->xcb_lk_id,
		    &lock_key, lock_mode,
		    &lock_value, (LK_LKID*)NULL,
		    timeout, &sys_err)) 
			!= LK_OK && cl_status != LK_VAL_NOTVALID )
	    {
		if ( cl_status == LK_NOLOCKS )
		{
		    lk_error = E_DM9D03_SEQUENCE_NO_LOCKS;
		    SETDBERR(&dms->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		}
		else if ( cl_status == LK_INTERRUPT || 
			  cl_status == LK_INTR_GRANT )
		{
		    lk_error = E_DM9D06_SEQUENCE_LOCK_INTERRUPT;
		    SETDBERR(&dms->error, 0, E_DM0065_USER_INTR);
		}
                else if ( cl_status == LK_INTR_FA )
                {
                    lk_error = E_DM9D06_SEQUENCE_LOCK_INTERRUPT;
                    SETDBERR(&dms->error, 0, E_DM016B_LOCK_INTR_FA);
                }
		else if ( cl_status == LK_DEADLOCK )
		{
		    lk_error = E_DM9D04_SEQUENCE_DEADLOCK;
		    SETDBERR(&dms->error, 0, E_DM0042_DEADLOCK);
		}
		else if ( cl_status == LK_TIMEOUT )
		{
		    lk_error = E_DM9D0B_SEQUENCE_LOCK_TIMEOUT;
		    SETDBERR(&dms->error, 0, E_DM004D_LOCK_TIMER_EXPIRED);
		}
		else if ( cl_status == LK_UBUSY )
		{
		    /* Output error message iff DMZ_SES_MACRO(35)
		    ** is on in this SCB */
		    if ( DMZ_SES_MACRO(35) )
		    {
			CS_SID	sid;
			CSget_sid(&sid);

			if ( DMZ_MACRO((GET_DML_SCB(sid))->scb_trace, 35) )
			{
			    uleFormat(NULL, E_DM9D0C_SEQUENCE_LOCK_BUSY, 
				&sys_err, ULE_LOG, NULL, 
				(char*)NULL, 0L, (i4*)NULL, &local_error, 4, 
				sizeof(dms_seq->seq_name), &dms_seq->seq_name,
				sizeof(dcb->dcb_name), &dcb->dcb_name,
				0, lock_mode,
				0, sys_err.moreinfo[0].data.string);
			}
		    }
		    lk_error = E_DM9D0C_SEQUENCE_LOCK_BUSY;
		    SETDBERR(&dms->error, 0, E_DM004D_LOCK_TIMER_EXPIRED);
		    break;
		}
		else
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);

		    lk_error = E_DM901C_BAD_LOCK_REQUEST;

		    uleFormat(NULL, lk_error, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 2, 0, lock_mode, 
			0, xcb->xcb_lk_id);
		    SETDBERR(&dms->error, 0, E_DM9D07_SEQUENCE_LOCK_ERROR);
		}
		uleFormat(NULL, lk_error, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 3, 
			sizeof(dms_seq->seq_name), &dms_seq->seq_name,
			sizeof(dcb->dcb_name), &dcb->dcb_name,
			0, lock_mode);
		break;
	    }
	    
	    /*
	    ** Find or create DML_SEQ
	    ** and bind it to dms_seq->seq_seq
	    */

	    dm0s_mlock(&dcb->dcb_mutex);

	    /*
	    ** CREATE doesn't yet know the sequence id.
	    ** Set it to an invalid value (-1,0).
	    ** The correct id will be supplied when
	    ** the CREATE completes successfully
	    ** (dms_close_seq()).
	    */
	    if ( dms->dms_flags_mask == DMS_CREATE )
	    {
		dms_seq->seq_id.db_tab_base  = -1;
		dms_seq->seq_id.db_tab_index = 0;
	    }

	    /* Find by db_id, name, owner */
	    if ( !(s = FindSeq(xcb, dms_seq)) )
	    {
		/* Not found, make a new one */

		/* First look for a vacant DML_SEQ for this DB */

		for ( s = dcb->dcb_seq;
		      s && s->seq_id;
		      s = s->seq_q_next );

		if ( s == (DML_SEQ*)NULL )
		{
		    /* Allocate memory for DML_SEQ */

		    if ( status = dm0m_allocate((i4)sizeof(DML_SEQ),
			DM0M_ZERO, (i4)SEQ_CB, (i4)SEQ_ASCII_ID, 
			(char *)dcb, (DM_OBJECT **)&s, &dms->error) )
		    {
			dm0s_munlock(&dcb->dcb_mutex);
			break;
		    }
		    else
		    {
			char	sem_name[CS_SEM_NAME_LEN*2];
			char	seq_name[DB_SEQ_MAXNAME+1];

			s->seq_q_next = (DML_SEQ*)NULL;
			s->seq_q_prev = (DML_SEQ*)NULL;

			MEcopy((PTR)&dms_seq->seq_name,
				DB_SEQ_MAXNAME, seq_name);
			seq_name[DB_SEQ_MAXNAME] = EOS;
			STtrmwhite(seq_name);
			STpolycat(2, "SeqGen ", seq_name, sem_name);
			dm0s_minit(&s->seq_mutex, sem_name);
		    }
		}

		dm0s_mlock(&s->seq_mutex);
		STRUCT_ASSIGN_MACRO(dms_seq->seq_name, s->seq_name);
		STRUCT_ASSIGN_MACRO(dms_seq->seq_owner, s->seq_owner);
		s->seq_db_id = dcb->dcb_id;
		s->seq_id = dms_seq->seq_id.db_tab_base;
		s->seq_cache = 0;
		s->seq_flags = 0;
	    }


	    /* If DDL, move Sequence to DCB list */
	    if ( dms->dms_flags_mask & DMS_DDL )
		lock_value.lk_mode = LK_IX;

	    /*
	    ** Place the DML_SEQ on the appropriate list. 
	    **
	    ** If opened for DDL, put on DCB list.
	    **
	    ** If opened for DML and SEQUENCE lock granted
	    ** mode is LK_X, put on XCB list, otherwise
	    ** put on DCB list.
	    */

	    if ( s->seq_lock_mode != lock_value.lk_mode )
	    {
		/* Remove from whatever list it's on, if any */
		if ( s->seq_q_next )
		    s->seq_q_next->seq_q_prev = s->seq_q_prev;
		if ( s->seq_q_prev )
		    s->seq_q_prev->seq_q_next = s->seq_q_next;

		if ( s == dcb->dcb_seq )
		    dcb->dcb_seq = s->seq_q_next;
		else if ( s == xcb->xcb_seq )
		    xcb->xcb_seq = s->seq_q_next;
		else if ( s == dmf_svcb->svcb_seq )
		    dmf_svcb->svcb_seq = s->seq_q_next;

		if ( (s->seq_lock_mode = lock_value.lk_mode) == LK_X )
		{
		    /* X-locked DML_SEQs go on XCB list */
		    if ( s->seq_q_next = xcb->xcb_seq )
			xcb->xcb_seq->seq_q_prev = s;
		    s->seq_q_prev = (DML_SEQ*)NULL;
		    xcb->xcb_seq = s;
		}
		else
		{
		    /* ...all others go on DCB list */
		    if ( s->seq_q_next = dcb->dcb_seq )
			dcb->dcb_seq->seq_q_prev = s;
		    s->seq_q_prev = (DML_SEQ*)NULL;
		    dcb->dcb_seq = s;
		}
	    }
	    /* Return bound DML_SEQ* to caller */
	    dms_seq->seq_seq = (PTR)s;
	    dms_seq->seq_cseq = (PTR)NULL;
	    dm0s_munlock(&s->seq_mutex);
	    dm0s_munlock(&dcb->dcb_mutex);
	}
    }

    if ( dms->error.err_code )
    {
	/* Lock-related errors already reported */
	if ( !lk_error )
	{
	    uleFormat(&dms->error, 0, NULL ,ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	    if ( dms_seq )
		uleFormat( &dms->error, E_DM9D05_SEQUENCE_OPEN_FAILURE, 
			&sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 2, 
			sizeof(dms_seq->seq_name), &dms_seq->seq_name,
			sizeof(dcb->dcb_name), &dcb->dcb_name);
	    SETDBERR(&dms->error, 0, E_DM9D0A_SEQUENCE_ERROR);
	}
	status = E_DB_ERROR;
    }
    else
	status = E_DB_OK;

    return(status);
}

/*{
** Name: dms_close_seq - Closes a Sequence after DDL.
**
**  INTERNAL DMF call format:    status = dms_close_seq(&dms_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMS_CLOSE_SEQ,&dms_cb);
**
** Description:
**
** Inputs:
**      dms_cb 
**         .type                            Must be set to DMS_SEQ_CB.
**         .length                          Must be at least sizeof(DMS_CB).
**         .dms_tran_id                     Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**         .dms_flags_mask		    
**		DMS_CREATE		    CREATE SEQUENCE operation
**		DMS_ALTER		    ALTER SEQUENCE operation
**		DMS_DROP		    DROP SEQUENCE operation
**					    None of the above, query
**					    execution stating intent
**					    to use Sequence(s).
**         .dms_db_id                       Must be the identifier for the 
**                                          odcb returned from open database.
**         .dms_seq_array.data_address      Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DMS_SEQ_ENTRY.
**                                          See below for description of 
**                                          <dms_seq_array> entries.
**         .dms_seq_array.data_in_size      Length of seq_array data in bytes.
**         <dmu_seq_array> entries are of type DMS_SEQ_ENTRY and 
**         must have following values:
**         seq_seq                          Pointer to bound DML_SEQ
**	   seq_lock_list		    Lock list holding the 
**					    SEQUENCE lock
**	   seq_lock_id			    The lock id of that lock.
**
** Outputs:
**      dms_cb   
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM010C_TRAN_ABORTED:
**					    
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dms_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dms_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dms_cb.error.err_code.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	28-Apr-2005 (jenjo02)
**	    On successful ALTER, set seq_version to zero to
**	    induce refetch of IISEQUENCE.
*/
DB_STATUS
dms_close_seq(
PTR        dms_cb)
{
    DMS_CB          *dms = (DMS_CB*)dms_cb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;
    DML_ODCB	    *odcb;
    DMP_DCB	    *dcb;
    DML_SEQ	    *s;
    DMS_SEQ_ENTRY   *dms_seq;
    DB_STATUS	    status;
    i4		    error = 0, local_error;
    i4		    count;
    LK_VALUE	    lock_value;
    CL_ERR_DESC	    sys_err;
    STATUS	    cl_status;

    CLRDBERR(&dms->error);

    /* Nothing to do if DML */
    if ( dms->dms_flags_mask & DMS_DML )
	return(E_DB_OK);

    xcb = (DML_XCB*) dms->dms_tran_id;
    scb = xcb->xcb_scb_ptr;
    odcb = (DML_ODCB*)dms->dms_db_id;
    
    if ( dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) )
	SETDBERR(&dms->error, 0, E_DM003B_BAD_TRAN_ID);
    else
    {
	/* Check for external interrupts */
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dms->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dms->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dms->error, 0, E_DM0064_USER_ABORT);
	    else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		SETDBERR(&dms->error, 0, E_DM0132_ILLEGAL_STMT);
	}
    }
    
    if ( dms->error.err_code == 0 )
    {
	if ( xcb->xcb_x_type & XCB_RONLY &&
	     dms->dms_flags_mask & DMS_DDL )
	    SETDBERR(&dms->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	else if ( dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB))
	    SETDBERR(&dms->error, 0, E_DM0010_BAD_DB_ID);

	/*  Check that this is a update transaction on the database 
	**  that can be updated. */
	else if ( odcb != xcb->xcb_odcb_ptr )
	    SETDBERR(&dms->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	else
	{
	    dcb = odcb->odcb_dcb_ptr;
	    /* There must be at least one SEQ_ENTRY */
	    dms_seq = (DMS_SEQ_ENTRY*)dms->dms_seq_array.data_address;
	    count = dms->dms_seq_array.data_in_size / sizeof(DMS_SEQ_ENTRY);

	    if ( dms_seq == NULL || count <= 0 )
		SETDBERR(&dms->error, 0, E_DM002A_BAD_PARAMETER);
	}
    }

    for ( dms_seq; dms->error.err_code == 0 && count; dms_seq++, count-- )
    {
	s = (DML_SEQ*)dms_seq->seq_seq;

	dm0s_mlock(&s->seq_mutex);

	if ( dms_seq->seq_status ) 
	{
	    /* If CREATE failed, vacate the DML_SEQ */
	    if ( dms->dms_flags_mask == DMS_CREATE )
		s->seq_id = 0;
	}
	else
	{
	    /* If CREATE succeeded, fill in sequence id */
	    if ( dms->dms_flags_mask == DMS_CREATE )
		s->seq_id = dms_seq->seq_id.db_tab_base;

	    /* If DROP succeeded, vacate the DML_SEQ */
	    else if ( dms->dms_flags_mask == DMS_DROP )
		s->seq_id = 0;

	    /*
	    ** If ALTER succeeded, set the in-memory
	    ** version to zero. This will cause a 
	    ** fetch of the updated IISEQUENCE tuple
	    ** by the next transaction to reference it,
	    ** even repeat queries which may not
	    ** qeq_validate the sequence.
	    */
	    else if ( dms->dms_flags_mask == DMS_ALTER )
		s->seq_cache = 0; /* Directly affect cache count b112605 */
	}

	dm0s_munlock(&s->seq_mutex);
    }

    if ( dms->error.err_code )
    {
	uleFormat(&dms->error, 0, NULL ,ULE_LOG, NULL, 
			NULL, 0, NULL, &local_error, 0);

	if ( dms_seq )
	    uleFormat( &dms->error, E_DM9D09_SEQUENCE_CLOSE_FAILURE, 
	    	    &sys_err, ULE_LOG , NULL,
		    (char * )NULL, 0L, (i4 *)NULL,
		    &local_error, 2, 
		    sizeof(dms_seq->seq_name), &dms_seq->seq_name,
		    sizeof(dcb->dcb_name), &dcb->dcb_name);
	SETDBERR(&dms->error, 0, E_DM9D0A_SEQUENCE_ERROR);
	status = E_DB_ERROR;
    }
    else
	status = E_DB_OK;

    return(status);
}

/*{
** Name: dms_next_seq - Retrieves next sequence value.
**
**  INTERNAL DMF call format:    status = dms_next_seq(&dms_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMS_NEXT_SEQ,&dms_cb);
**
** Description:
**
** Inputs:
**      dms_cb 
**         .type                            Must be set to DMS_SEQ_CB.
**         .length                          Must be at least sizeof(DMS_CB).
**         .dms_tran_id                     Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**         .dms_flags_mask		    
**         .dms_db_id                       Must be the identifier for the 
**                                          odcb returned from open database.
**         .dms_seq_array.data_address      Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DMS_SEQ_ENTRY.
**                                          See below for description of 
**                                          <dms_seq_array> entries.
**         .dms_seq_array.data_in_size      Length of seq_array data in bytes.
**         <dmu_seq_array> entries are of type DMS_SEQ_ENTRY and 
**         must have following values:
**	   seq_seq			    Pointer to bound DML_SEQ.
**
** Outputs:
**         .seq_value                       Where to return the value.
**	       .data_address		    Pointer to the area.
**	       .data_out_size		    Size of that area, must
**					    match Sequence's seq_dlen.
**      dms_cb   
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0079_BAD_OWNER_NAME
**                                          E_DM010C_TRAN_ABORTED:
**					    
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dms_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dms_cb.error.err_code.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	24-Feb-2003 (jenjo02)
**	    Removed check for read-only transaction.
**	    iisequence updates are performed under-the-covers
**	    ala "update_rel()" and are neither logged nor
**	    journalled (B109703, STAR 12526687).
**	06-Mar-2003 (jenjo02)
**	    Add EX handler to catch those nasty decimal
**	    overflow situations.
**	    If seq_cache is zero, the sequence is exhausted,
**	    return EXCEEDED.
**	27-Feb-2005 (schka24)
**	    Pass 9D00 to qef un-logged;  can happen when repeated QP
**	    references a sequence that's no longer there, and didn't
**	    already have a DMS_SEQ around, and there's no other validation
**	    test that the QP fails.
**	24-apr-2007 (dougi)
**	    Hard coded precision, length values for decimal sequences to
**	    match iisequence (since decimals have bigger precision now).
**	4-june-2008 (dougi)
**	    Upgrade integer support to 64-bit (bigint).
**	15-june-2008 (dougi)
**	    Add support for unordered (random) sequences.
**	9-sep-2009 (stephenb)
**	    copy system generated (identity column) sequences to the SCB
**	    so that we can use them for the last_identity() SQL function
**	    through dmf_last_id().
*/
DB_STATUS dms_next_seq(
PTR        dms_cb)
{
    DMS_CB          *dms = (DMS_CB*)dms_cb;
    DML_XCB	    *xcb;
    DML_ODCB	    *odcb;
    DMP_DCB	    *dcb;
    DML_SEQ	    *s;
    DMS_SEQ_ENTRY   *dms_seq;
    DB_STATUS	    status;
    CL_ERR_DESC     sys_err;
    PTR	            value;
    i4		    error = 0, local_error;
    i4		    count;
    u_char	    tdec[20];
    i4		    tprec, tscale;
    EX_CONTEXT	    ex;
    DML_SCB	    *dml_scb;

    CLRDBERR(&dms->error);

    xcb = (DML_XCB*) dms->dms_tran_id;
    odcb = (DML_ODCB*)dms->dms_db_id;
    dcb = odcb->odcb_dcb_ptr;
    dml_scb = xcb->xcb_scb_ptr;
    
    if ( dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) )
	SETDBERR(&dms->error, 0, E_DM003B_BAD_TRAN_ID);
    else
    {
	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dms->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dms->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dms->error, 0, E_DM0064_USER_ABORT);
	    else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		SETDBERR(&dms->error, 0, E_DM0132_ILLEGAL_STMT);
	}
    }

    if ( dms->error.err_code == 0 )
    {
	if ( dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB))
	    SETDBERR(&dms->error, 0, E_DM0010_BAD_DB_ID);

	/*  Check that this is a update transaction on the database 
	**  that can be updated. */
	else if (odcb != xcb->xcb_odcb_ptr)
	    SETDBERR(&dms->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	else
	{
	    /* There must at least one SEQ_ENTRY */
	    dms_seq = (DMS_SEQ_ENTRY*)dms->dms_seq_array.data_address;
	    count = dms->dms_seq_array.data_in_size / sizeof(DMS_SEQ_ENTRY);
	    if ( dms_seq == NULL || count == 0 ||
		(dms->dms_flags_mask != DMS_NEXT_VALUE &&
		 dms->dms_flags_mask != DMS_CURR_VALUE) )
	    {
		SETDBERR(&dms->error, 0, E_DM002A_BAD_PARAMETER);
	    }
	}
    }

    /* Return next/current value for each Sequence */
    for ( dms_seq; dms->error.err_code == 0 && count; dms_seq++, count-- )
    {
	/*
	** Each Sequence has been at least IX locked
	** and a DML_SEQ inserted onto the dcb/xcb list.
	*/
	if ( (s = (DML_SEQ*)dms_seq->seq_seq) &&
	      s->seq_id == dms_seq->seq_id.db_tab_base &&
	      (value = dms_seq->seq_value.data_address) )
	{
	    if ( dms->dms_flags_mask == DMS_CURR_VALUE )
		/* CURRENT VALUE: */
		GetTxnCvalue(dms, xcb, dms_seq, s, value);
	    else
	    {
		/* NEXT VALUE: */

		dm0s_mlock(&s->seq_mutex);

		/*
		** If it's time to reload the cache, update DML_SEQ from
		** IISEQUENCE. This will have been zeroed on exit from
		** dbs_close_seq which will check for ALTER. This will
		** ensure that the cached values are re-read. (b112605)
		*/
		if ( s->seq_cache == 0 )
		{
		    dms_fetch_iiseq((PTR)dms, xcb, s);
		}
		
		if ( dms->error.err_code == 0 &&
		     dms_seq->seq_value.data_out_size != s->seq_dlen )
		{
		    SETDBERR(&dms->error, 0, E_DM002A_BAD_PARAMETER);
		}
		else if ( dms->error.err_code == 0 )
		{
		    switch ( s->seq_dtype )
		    {
		      case DB_INT_TYPE:

			/* NEXT VALUE: check for wrap or exhaustion */
			if (!(s->seq_flags & SEQ_UNORDERED) &&
			    (s->seq_next.intval > s->seq_max.intval ||
			     s->seq_next.intval < s->seq_min.intval ||
			     s->seq_cache < 0 )) /* b112605 */
			{
			    if ( (s->seq_flags & SEQ_CYCLE) == 0 )
			    {
				/* Boundary breach */
				SETDBERR(&dms->error, 0, E_DM9D01_SEQUENCE_EXCEEDED);
				break;
			    }

			    /* Asc, wrap to min; dsc, wrap to max */
			    if ( s->seq_incr.intval > 0 )
				s->seq_next.intval = s->seq_min.intval;
			    else
				s->seq_next.intval = s->seq_max.intval;
			}

			/* Return the value */
			if (s->seq_dlen == 8)
			    *(i8*)value = s->seq_next.intval;
			else *(i4*)value = (i4)s->seq_next.intval;
			
			/* 
			** if this is a value for an identity column, save the value
			** for the session for later reference with last_identity()
			*/
			if (s->seq_flags & SEQ_SYSGEN)
			    STRUCT_ASSIGN_MACRO(s->seq_next, dml_scb->scb_last_id);

			/* Increment the value if cache remains */
			if ( --s->seq_cache > 0) /* b112605 */
			{
			    if (s->seq_flags & SEQ_UNORDERED)
			    {
				s->seq_next.intval = 
				    dms_nextUnorderedVal(s->seq_next.intval, 
							s->seq_dlen);
				if ( s->seq_next.intval == s->seq_start.intval 
					&& (s->seq_flags & SEQ_CYCLE) == 0 )
				{
				    /* Boundary breach */
				    SETDBERR(&dms->error, 0, E_DM9D01_SEQUENCE_EXCEEDED);
				    break;
				}
			    }
			    else s->seq_next.intval += s->seq_incr.intval;

			    /* Note that the value has been updated */
			    s->seq_flags |= SEQ_UPDATED;
			}
			
			break;

		      case DB_DEC_TYPE:

			/* NEXT VALUE: check for wrap or exhaustion */
			if ( (MHpkcmp(s->seq_next.decval, 31, 0,
				     s->seq_max.decval, 31, 0)) > 0
			    ||
			     (MHpkcmp(s->seq_next.decval, 31, 0,
				     s->seq_min.decval, 31, 0)) < 0
			    || s->seq_cache < 0 ) /* b112605 */
			{
			    /* Boundary reached, wrap if cyclable */
			    if ( (s->seq_flags & SEQ_CYCLE) == 0 )
			    {
				SETDBERR(&dms->error, 0, E_DM9D01_SEQUENCE_EXCEEDED);
				break;
			    }

			    CVlpk(0, 31, 0, (PTR)tdec);

			    /* Asc, wrap to min; dsc, wrap to max */
			    if ( MHpkcmp(s->seq_incr.decval, 31, 0,
				     (PTR)tdec, 31, 0) > 0 )
			    {
				MEcopy((PTR)&s->seq_min.decval, 
				       sizeof(s->seq_next.decval),
				       (PTR)&s->seq_next.decval);
			    }
			    else
			    {
				MEcopy((PTR)&s->seq_max.decval, 
				       sizeof(s->seq_next.decval),
				       (PTR)&s->seq_next.decval);
			    }
			}

			/* Return the value */
			CVpkpk(s->seq_next.decval, 31, 0,
			       s->seq_prec, 0, value);

			/* Increment the value if cache remains */
			if ( --s->seq_cache > 0 ) /* b112605 */
			{
			    if ( EXdeclare(dms_exhandler, &ex) )
				/* Overflow, invalidate cache */
				s->seq_cache = 0;
			    else
			    {
				MHpkadd(s->seq_next.decval, 31, 0,
				    s->seq_incr.decval, 31, 0,
				    (PTR)tdec, &tprec, &tscale);

				CVpkpk((PTR)tdec, tprec, tscale, 31, 0, 
				   (PTR)&s->seq_next.decval);

				/* Note that the value has been updated */
				s->seq_flags |= SEQ_UPDATED;
			    }
			    EXdelete();
			}

			break;
		    }
		}

		if ( dms->error.err_code == E_DM9D00_IISEQUENCE_NOT_FOUND )
		{
		    /* iisequence no longer in catalog, vacate DML_SEQ */
		    s->seq_id = 0;
		}
		dm0s_munlock(&s->seq_mutex);

		/* Save transaction's current sequence value */
		if ( dms->error.err_code == 0 )
		    PutTxnCvalue(dms, xcb, dms_seq, s, value);
	    }
	}
	else
	    SETDBERR(&dms->error, 0, E_DM002A_BAD_PARAMETER);
    }

    if ( dms->error.err_code )
    {
	/* These are "expected" and handled by QEF */
	if ( dms->error.err_code != E_DM9D01_SEQUENCE_EXCEEDED
	  && dms->error.err_code != E_DM9D00_IISEQUENCE_NOT_FOUND )
	{
	    uleFormat(&dms->error, 0, NULL ,ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	}
	return(E_DB_ERROR);
    }
    else
	return(E_DB_OK);
}

/*{
** Name: dms_end_tran - Called at end of transaction.
**
** Description:
**		If COMMIT, updates each exclusively bound
**		IISEQUENCE with the now-current value and
**		preserves the cache.
**		If ABORT, tosses the cache.
**
**		Deallocate all of the transaction's 
**		Sequence currencies.
**
** Inputs:
**	txn_state			    Commit, abort indicator.
**	xcb				    Transaction's XCB.
**	
** Outputs:
**      error				    Reason for failure.
**					    
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          err_code.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	10-Dec-2004 (jenjo02)
**	    Don't dms_update_iiseq() if all the cached
**	    values have been drawn; the IISEQUENCE
**	    tuple will have the correct "next" value
**	    which we'll refetch on the next reference.
*/
DB_STATUS
dms_end_tran(
i4	    txn_state,
DML_XCB     *xcb,
DB_ERROR    *error)
{
    DMP_DCB	    *dcb = xcb->xcb_odcb_ptr->odcb_dcb_ptr;
    DML_SEQ	    *s;
    DML_CSEQ	    *cs;
    DMS_CB	    dms;
    DB_STATUS	    status = E_DB_OK;

    /* Sequences appear on the XCB list only when X locked */
    while ( s = xcb->xcb_seq )
    {
	xcb->xcb_seq = s->seq_q_next;

	if ( txn_state == DMS_TRAN_COMMIT )
	{
	    /* 
	    ** For each Sequence whose current value has
	    ** be updated and that still has cached values, 
	    ** update the current value in
	    ** the corresponding IISEQUENCE tuple.
	    */
	    if ( s->seq_flags & SEQ_UPDATED && s->seq_id &&
		 s->seq_cache > 0 &&	 /* b112605 */
		(status = dms_update_iiseq((PTR)&dms, xcb, s)) )
	    {
		*error = dms.error;
		break;
	    }
	}
	else
	    /* ABORT: Toss the cache  */
	    s->seq_cache = 0;

	/* Move DML_SEQ from XCB to DCB list */
	s->seq_lock_mode = LK_IX;
	dm0s_mlock(&dcb->dcb_mutex);
	if ( s->seq_q_next = dcb->dcb_seq )
	    dcb->dcb_seq->seq_q_prev = s;
	s->seq_q_prev = (DML_SEQ*)NULL;
	dcb->dcb_seq = s;
	dm0s_munlock(&dcb->dcb_mutex);
    }

    /* Deallocate the transaction's Sequence currencies */
    while ( cs = xcb->xcb_cseq )
    {
	xcb->xcb_cseq = cs->cseq_q_next;
	cs->cseq_seq = (DML_SEQ*)NULL;
	dm0m_deallocate((DM_OBJECT**)&cs);
    }

    return(status);
}

/*{
** Name: dms_close_db - Called when a database is being
**			closed.
**
** Description:
**		To preserve a Database's Sequence caches,
**		move still-valid caches from the DCB list
**		to the SVCB list. When the database is
**		opened again and the sequence referenced,
**		it will be moved from the SVCB list to
**		the DCB.
**
** Inputs:
**	dcb			Database being closed.
**	
** Outputs:
**      none
**					    
**      Returns:
**          void
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
*/
VOID
dms_close_db(
DMP_DCB	    *dcb)
{
    DML_SEQ	*s;

    /* Move each Sequence to SVCB list, or destroy */
    dm0s_mlock(&dmf_svcb->svcb_dq_mutex);

    while ( s = dcb->dcb_seq )
    {
	dcb->dcb_seq = s->seq_q_next;

	/* Preserve the Sequence if still valid cached values */
	if ( s->seq_cache > 0 && s->seq_id )
	{
	    /* This means "it's on the SVCB list" */
	    s->seq_lock_mode = LK_N;

	    /* Move DML_SEQ from DCB to SVCB list */
	    if ( s->seq_q_next = dmf_svcb->svcb_seq )
		dmf_svcb->svcb_seq->seq_q_prev = s;
	    s->seq_q_prev = (DML_SEQ*)NULL;
	    dmf_svcb->svcb_seq = s;
	}
	else
	{
	    s->seq_id = 0;
	    dm0s_mrelease(&s->seq_mutex);
	    dm0m_deallocate((DM_OBJECT**)&s);
	}
    }

    dm0s_munlock(&dmf_svcb->svcb_dq_mutex);

    return;
}

/*{
** Name: dms_stop_server - When the Server is terminated,
**			   deallocate all remaining 
**			   DML_SEQ's on the SVCB list.
**
** Description:
**		When a Server is closed, all Sequences that
**		were allocated on behalf of all databases must
**		be deallocated.
**
**		Note that unused cached Sequence values are
**		forever lost.
**
** Inputs:
**	none
**	
** Outputs:
**      none
**					    
**      Returns:
**          void
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
*/
VOID
dms_stop_server()
{
    DML_SEQ	    *s;

    while ( s = dmf_svcb->svcb_seq )
    {
	s->seq_id = 0;
	dmf_svcb->svcb_seq = s->seq_q_next;
	dm0s_mrelease(&s->seq_mutex);
	dm0m_deallocate((DM_OBJECT**)&s);
    }

    return;
}

/*{
** Name: dms_destroy_db -  When a database is destroyed,
**			   deallocate all of its
**			   DML_SEQ's on the SVCB list.
**
** Description:
**		When a Database is destroyed, all Sequences that
**		were allocated on its behalf must
**		be deallocated.
**
** Inputs:
**	db_id		Database Id
**	
** Outputs:
**      none
**					    
**      Returns:
**          void
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
*/
VOID
dms_destroy_db(
i4	db_id)
{
    DML_SEQ	    *s, *ns;

    dm0s_mlock(&dmf_svcb->svcb_dq_mutex);

    ns = dmf_svcb->svcb_seq;

    while ( s = ns )
    {
	ns = s->seq_q_next;

	if ( s->seq_db_id == db_id )
	{
	    s->seq_id = 0;

	    /* Remove from SVCB list */
	    if ( s->seq_q_next )
		s->seq_q_next->seq_q_prev = s->seq_q_prev;
	    if ( s->seq_q_prev )
		s->seq_q_prev->seq_q_next = s->seq_q_next;
	    else
		dmf_svcb->svcb_seq = s->seq_q_next;
	    dm0s_mrelease(&s->seq_mutex);
	    dm0m_deallocate((DM_OBJECT**)&s);
	}
    }
    dm0s_munlock(&dmf_svcb->svcb_dq_mutex);

    return;
}

/*{
** Name: FindSeq - Return pointer to a DML_SEQ, matching on
**		    db_id, sequence name and owner.
**
** Description:
**		Searches XCB, DCB, SVCB lists of opened DML_SEQ.
**		If found, mutexes the Sequence.
**
** Inputs:
**	xcb			Transaction's XCB.
**	seq			DMS_SEQ of interest.
**	
** Outputs:
**	none
**
**      Returns:
**	DML_SEQ*		Pointer if found,
**				null if not found.
**				If found, seq_mutex is
**				locked.
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	06-Mar-2003 (jenjo02)
**	    Allow for possibility that DML_SEQ may be 
**	    older -or- newer than DMS_SEQ_ENTRY
**	    (repeat queries).
*/
static DML_SEQ *
FindSeq(
DML_XCB		*x,
DMS_SEQ_ENTRY	*seq)
{
    DML_SEQ	*s, **n = &x->xcb_seq;
    i4		db_id = x->xcb_odcb_ptr->odcb_dcb_ptr->dcb_id;
    i4		i = 2;
    
    /* Search XCB list, then DCB list, then DMF_SVCB list */
    do
    {
	for ( s = *n; s; s = s->seq_q_next )
	{
	    if ( s->seq_db_id == db_id &&
		 (MEcmp((PTR)&s->seq_name, (PTR)&seq->seq_name,
			sizeof(DB_NAME)) == 0) &&
		 (MEcmp((PTR)&s->seq_owner, (PTR)&seq->seq_owner,
			sizeof(DB_OWN_NAME)) == 0) )
	    {
		dm0s_mlock(&s->seq_mutex);

		/*
		** DB, name and owner match, but the id may not
		** (DROP,CREATE). Invalidate the version
		** to cause it to be refetched from the catalog.
		*/
		if ( s->seq_id < seq->seq_id.db_tab_base )
		{
		    /* This, then, is the new sequence id */
		    s->seq_id = seq->seq_id.db_tab_base;
		    s->seq_cache = 0;
		}
		else if ( s->seq_id > seq->seq_id.db_tab_base )
		    /*
		    ** Caller's DMS_SEQ_ENTRY is out of date 
		    ** (repeat query, perhaps)
		    */
		    seq->seq_id.db_tab_base = s->seq_id;

		break;
	    }
	}
	if ( s == (DML_SEQ*)NULL )
	{
	    if ( --i )
		n = &x->xcb_odcb_ptr->odcb_dcb_ptr->dcb_seq;
	    else
		n = &dmf_svcb->svcb_seq;
	}
    } while ( s == (DML_SEQ*)NULL && i >= 0 );

    return(s);
}

/*{
** Name: PutTxnCvalue - Save transaction's current value.
**
** Description:
**		If necessary, allocates a DMS_CSEQ for the
**		transaction, saves the Sequence current
**		value.
**
** Inputs:
**	dms		Caller's DMS_CB
**	x		Transaction's XCB.
**	dms_seq		Caller's DMS_SEQ.
**	s		DML_SEQ of sequence.
**	value		Current value of the
**			sequence.
**	
** Outputs:
**	error		Why it failed if it did.
**
**      Returns:
**	void
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	7-feb-2009 (dougi) Bug 121630
**	    bigint sequences - not i4.
*/
static VOID
PutTxnCvalue(
DMS_CB		*dms,
DML_XCB		*x,
DMS_SEQ_ENTRY	*dms_seq,
DML_SEQ		*s,
PTR		value)
{
    DML_CSEQ	*cs;
    i4		error;

    /* Locate DML_CSEQ on transaction's list */
    if ( !(cs = (DML_CSEQ*)dms_seq->seq_cseq) ||
	   cs->cseq_seq != s )
    {
	for ( cs = x->xcb_cseq; 
	      cs && cs->cseq_seq != s; 
	      cs = cs->cseq_q_next );
    }

    if ( cs == (DML_CSEQ*)NULL )
    {
	if ( dm0m_allocate((i4)sizeof(DML_CSEQ),
		0, (i4)CSEQ_CB, (i4)CSEQ_ASCII_ID, 
		(char *)x, (DM_OBJECT **)&cs, &dms->error) )
	{
	    return;
	}

	/* Init and link to xcb->cseq list */
	cs->cseq_seq   = s;
	cs->cseq_q_next = x->xcb_cseq;
	x->xcb_cseq = cs;

	/* Set pointer to DML_CSEQ in caller's DMS_SEQ_ENTRY */
	dms_seq->seq_cseq = (PTR)cs;
    }

    /* Set the value */
    if ( s->seq_dtype == DB_INT_TYPE )
	cs->cseq_curr.intval = *(i8*)value;
    else if ( s->seq_dtype == DB_DEC_TYPE )
	CVpkpk(value, s->seq_prec, 0,
	       s->seq_prec, 0, cs->cseq_curr.decval);

    return;
}

/*{
** Name: GetTxnCvalue - Return transaction's current value
**			for a Sequence Generator.
**
** Description:
**		Returns the current value of a Sequence as
**		realized by a transaction's last "next value"
**		Sequence operation.
**
** Inputs:
**	dms		Caller's DMS_CB
**	x		Transaction's XCB.
**	dms_seq		Caller's DMS_SEQ.
**	s		DML_SEQ of sequence.
**	
** Outputs:
**	value		Current value of the
**			sequence.
**	error		Why it failed if it did.
**
**      Returns:
**	void
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Created.
**	7-feb-2009 (dougi) Bug 121630
**	    bigint sequences - not i4.
*/
static VOID
GetTxnCvalue(
DMS_CB		*dms,
DML_XCB		*x,
DMS_SEQ_ENTRY	*dms_seq,
DML_SEQ		*s,
PTR		value)
{
    DML_CSEQ	*cs;

    /* Locate DML_CSEQ on transaction's list */
    if ( !(cs = (DML_CSEQ*)dms_seq->seq_cseq) ||
	   cs->cseq_seq != s )
    {
	for ( cs = x->xcb_cseq;
	      cs && cs->cseq_seq != s;
	      cs = cs->cseq_q_next );
    }

    if ( cs )
    {
	if ( dms_seq->seq_value.data_out_size == s->seq_dlen )
	{
	    /* Return transaction's current value */
	    if ( s->seq_dtype == DB_INT_TYPE )
		*(i8*)value = cs->cseq_curr.intval;
	    else if ( s->seq_dtype == DB_DEC_TYPE )
		CVpkpk(cs->cseq_curr.decval, s->seq_prec, 0,
		       s->seq_prec, 0, value);
	}
	else
	    SETDBERR(&dms->error, 0, E_DM002A_BAD_PARAMETER);
    }
    else 
	SETDBERR(&dms->error, 0, E_DM9D02_SEQUENCE_NOT_FOUND);

    return;
}

/*{
** Name: dms_nextUnorderedVal Return the next "random" sequence value.
**
** Description:
**		Returns the next value in sequence using
**		Roy's spiffy quasi-random sequence generator.
**
** Inputs:
**	n		Current sequence value.
**	seqlen		Length of sequence (32/64 bit) used to select
**			correct algorithm.
**	
** Outputs:
**
**      Returns:	Next sequence value.
**	void
**
** History:
**	15-june-2008 (dougi)
**	    Written for random sequence generation.
*/
FUNC_EXTERN i8 
dms_nextUnorderedVal(
i8		currval,
i4		seqlen)
{
    i4		nextval = (i4)currval;

    /* Compute next value in sequence using Roy's algorithms, an 
    ** explanation of which will hopefully be supplied (some day). */

    switch (seqlen) {
      case 4:		/* 32-bit sequence */
      {
	i4	nextval = (i4)currval;

	nextval = nextval/2 + (nextval%2 + nextval/8 % 2) %2 * (2<<29);
	return((i8)nextval);
      }

      case 8:		/* 64-bit sequence */
      {
	i8		nextval;

	nextval = currval/2 + ((currval%2 + currval/2 %2) %2<<62);
	return(nextval);
      }
    }

}
