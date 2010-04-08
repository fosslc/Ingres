/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sd.h>
#include    <sr.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adudate.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <adf.h>
#include    <adp.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmccb.h>
#include    <dmse.h>
#include    <dmucb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0pbmcb.h>
#include    <dm0s.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1i.h>
#include    <dm1h.h>
#include    <dm1r.h>
#include    <dm1s.h>
#include    <dm1p.h>
#include    <dm2f.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm2r.h>
#include    <dm2rlct.h>
#include    <dmftrace.h>
#include    <dml.h>
#include    <dmpbuild.h>
#include    <dmfgw.h>
#include    <dmpepcb.h>
#include    <dma.h>
#include    <dmxe.h>
#include    <dm2rep.h>
#include    <dmpecpn.h>
#include    <cui.h>
#include    <dbdbms.h>

/*
** Name: dm2rep.c - replication operations
**
** Description:
**	This file contains all the routines that are associated with
**	replication operations at the dm2 level, the following routines
**	are defined in this file.
**
**	External functions:
**
**	    dm2rep_capture - main data capture detector, entry point
**			     for replication data capture.
**	    dm2rep_qman    - replicator queue management, the routine reads
**			     records from the input queue and writes to
**			     the distribution queue
**
** History:
**	19-apr-96 (stephenb)
**	    created for DBMS replication phase 1
**	16-sep-1996 (canor01)
**	    Add missing asterisk on err_code reference in dm2rep_qman().
**	14-aug-96 (stephenb)
**	    Fixes to collision resolution priority lookup.
**	21-aug-96 (stephenb)
**	    Add code for cdds lookup table checks.
**	24-jan-97 (stephenb)
**	    Changes to some of the fields in the shadow table (build_shadow)
**	    for special case where shadow record doesn't exist for the
**	    base record we have updated.
**      13-Jun-97 (fanra01)
**          Fix double memory free. Exception on insert into table.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call and alter code to cope 
**	    with upper case catalog names (sql92).
**	25-jun-97 (stephenb)
**	    Message W_DM5571_REP_DISABLE_CDDS should be 
**	    W_DM9571_REP_DISABLE_CDDS
**	15-sep-97 (stephenb)
**	    add code to cope with blobs when building archive records.
**      16-Oct-97 (fanra01)
**          Changed parameters to be pointers when calling ule_format.
**          Attempted output of error message during activate would exception.
**	20-nov-97 (stephenb)
**	    Fix bug 85136: whacked out scenarios which re-insert a record
**	    that has existed before and been updated, or update a record
**	    back to a value it had at some previous time, cause multiple
**	    un-archived shadow records. We need to deal with this and work
**	    out which one to use.
**	26-nov-97 (stephenb)
**	    set input and distribution queue locking strategy using
**	    CBF parameters.
**	14-jan-1998 (muhpa01)
**	    Fix parameters in calls to ule_format in build_shadow - need to
**	    pass address of value, not value.
**	25-feb-97 (stephenb)
**	    Return deadlock errors to caller of DMF. Add casts to MEfree calls
**	    to satisfy proto (someone must have added the proto recently). Add
**	    generic deadlock DM0042 to list of deadlock errors when handling
**	    deadlock in the queue management threads.
**	26-mar-97 (stephenb)
**	    Some rare deadlock situations slip through the net and cause
**	    error returns from dm2rep_qman()
**	14-May-98 (consi01) bug 90848 problem ingrep 34
**	    Removed consistency check as a tid of 0 is valid for tables
**	    with a hash structure.
**	07-may-1999 (somsa01)
**	    In build_shadow(), when acessing record entries out of shad_curr,
**	    offset shad_curr->shad_rec, NOT shad_curr. Also, removed unused
**	    variables, and added static prototype for check_paths().
**      01-14-2000 consi01 bug 103342 problem INGREP 84
**          Initialise local pointer variables to NULL. This ensures we do
**          not call MEfree with an undefined value.
**	10-may-1999 (somsa01)
**	    Forgot to change longnat to i4.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-dec-2000 (wansh01)
**	    code in line 2918 and 3141 if (*err_code = E_DM0055_NONEXT) 
**	    should be == not =             
**	28-sep-2001 (somsa01)
**	    Fixed compiler warnings on NT_IA64.
**	01-May-2002 (bonro01)
**	    Fix memory overlay caused by using wrong length for char array
**	02-Dec-2002 (hanje04).
**	    Bug 103342 Problem INGREP 84
**	    Change 448638 was not fully crossed from oping20 in May-2001.
**	    Add missing changes by hand.
**	11-nov-2002 (gygro01) b109056/ingrep128
**	    Initialising DMP_RCB struct pointer with NULL in dm2rep_qman to
**	    prevent segmentation violation in dm2t_close as they might be
**	    not initialised in a table deadlock situation.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          Disable RCB_CSRR_LOCK when writing the shadow record. Failure
**          to do this will cause a Physical Lock to be taken on the updated
**          shadow pages.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Rememebr the dmxe_flags used to begin a tx to commit/abort the TX.
**      22-Jan-2007 (hanal04) Bug 117533
**          Silence compiler warning messages under hpb_us5.
**	17-Apr-2007 (kibro01) b118117
**	    Use dmxe_flags in both dmxe_abort calls, and ensure that the flags
**	    aren't ambiguous (DMXE_INTERNAL being used as DMXE_WILLING_COMMIT
**	    in dmxe_abort, for instance - change made to dmxe.h)
**	28-Feb-2008 (kschendel) SIR 122739
**	    Minor changes to reflect the new rowaccess data structure.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2rep_? functions converted to DB_ERROR *, use
**	    new form uleFormat.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
*/
/*
** internal function protos
*/

static DB_STATUS	find_shadow(
				i2			trans_type,
				char			*int_date,
				DMP_DCB			*dcb,
				DMP_RCB			*base_rcb,
				DMP_TCB			*shad_tcb,
				DMP_SHAD_REC_QUE	**shad_head,
				char			*record,
				i2			*prio,
				i2			*cdds_no,
				DB_ERROR		*dberr);
static DB_STATUS 	build_shadow(
				DMP_DCB			*dcb,
				DMP_RCB			*base_rcb,
				DMP_TCB			*base_tcb,
				DMP_RCB			*shad_rcb,
				DMP_TCB			*shad_tcb,
				DMP_SHAD_REC_QUE	*shad_head,
				i2			trans_type,
				char			*record,
				char			*int_date,
				bool			fake,
				i2			*prio,
				i2			*cdds_no,
				DB_ERROR		*dberr);
static DB_STATUS 	build_archive(
				DMP_DCB			*dcb,
				DMP_RCB			*base_rcb,
				DMP_RCB			*arch_rcb,
				DMP_SHAD_REC_QUE	*shad_head,
				DB_ERROR		*dberr);
static DB_STATUS 	update_shadow(
				DMP_RCB			*shad_rcb,
				DMP_TCB			*shad_tcb,
				DMP_RCB			*base_rcb,
				DMP_TCB			*base_tcb,
				DMP_SHAD_REC_QUE	*shad_head,
				DB_ERROR		*dberr);
static DB_STATUS 	add_input_queue(
				DMP_DCB			*dcb,
				DMP_RCB			*base_rcb,
				i2			trans_type,
				DMP_SHAD_REC_QUE	*shad_head,
				char			*int_date,
				i2			dd_priority,
				i2			cdds_no,
				DB_ERROR		*dberr);
static DB_STATUS	check_paths (
				DMP_DCB			*dcb,
				DMP_RCB			*cdds_rcb,
				DMP_RCB			*paths_rcb,
				char			*cdds_rec,
				char			*paths_rec,
				DB_ERROR		*dberr);
static DB_STATUS 	check_priority(
				DMP_RCB			*base_rcb,
				char			*record,
				i2			*prio,
				DB_ERROR		*dberr);
static DB_STATUS	cdds_lookup(
				DMP_RCB			*base_rcb,
				char			*record,
				i2			*cdds_no,
				DB_ERROR		*dberr);

extern bool		LG_status_is_abort(
				i4			*lx_id);

/*
** Name: dm2rep_capture - check for and perform replication data capture
**
** Description:
**	This routine fors the entry point for replication data capture,
**	this routine will be called if the database is set up for replication
**	and we assume that all replication tables exist. The routine will
**	check if the table in question is replicated and will then call
**	maintainance routines to update the shadow and archive tables and
**	set the replication bit in the log record header.
**
** Inputs:
**	action		action to perform data capture on
**			valis values are DM2REP_DELETE, DM2REP_UPDATE, 
**			DM2REP_INSERT
**	rcb		base table rcb
**	record		updated record (NULL for deletes)
**
** Outputs:
**	err_code
**
** Returns:
**	DB_STATUS
**
** History:
**	19-apr-96 (stephenb)
**	    created for DBMS replication phase 1
**	14-aug-96 (stephenb)
**	    Adding code for priority based collision resolution
**	21-aug-96 (stephenb)
**	    Add code for cdds lookup table checks.
**	25-jun-97 (stephenb)
**	    Code cleanup, add error message E_DM9572_REP_CAPTURE_FAIL to
**	    be sure we are reporting an error on bad status.
**	25-feb-97 (stephenb)
**	    Return deadlock errors to caller of DMF
**	5-nov-98 (stephenb)
**	    Use cursor stability isolation for shadow and archive, we are
**	    protected by the base table lock on the unique key vale so
**	    we don't need to hold on to the locks for the entire transaction.
**	1-Nov-07 (kibro01) b119003
**	    Added message DM9581 to make shadow-duplicates error clear.
**	    Also fix DM9553 error so it doesn't crash.
*/
DB_STATUS
dm2rep_capture(	
	i4		action,
	DMP_RCB		*rcb,
	char		*record,
	DB_ERROR	*dberr)
{
    DMP_DCB		*dcb = rcb->rcb_tcb_ptr->tcb_dcb_ptr;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DMP_RCB		*shad_rcb = rcb->rep_shad_rcb;
    DMP_RCB		*arch_rcb = rcb->rep_arch_rcb;
    DMP_RCB		*shad_idx_rcb = rcb->rep_shadidx_rcb;
    DMP_TCB		*shad_tcb;
    DMP_SHAD_REC_QUE	*shad_rec = NULL;
    DB_STATUS		status;
    char		int_date[12];
    i2			prio = 0;
    i2			cdds_no = 0;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** due to some strange work with core system catalogs during allocation
    ** of the TCB, we may get called for some catalogs, so there is a safety
    ** check here to prevent us from replicating them
    */
    if (tcb->tcb_rel.relstat & TCB_CATALOG)
    {
	tcb->tcb_replicate = TCB_NOREP;
	return(E_DB_OK);
    }
    /*
    ** Consistency check
    */
    if (shad_rcb == NULL || arch_rcb == NULL)
    {
	uleFormat(dberr, E_DM9553_REP_NO_SHADOW, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)1, DB_MAXNAME, tcb->tcb_rel.relid.db_tab_name);
	return(E_DB_ERROR);
    }
    shad_tcb = rcb->rep_shad_rcb->rcb_tcb_ptr;

    /* set iso level down, the base table locks the resource for us */
    shad_idx_rcb->rcb_iso_level = RCB_CURSOR_STABILITY;
    shad_idx_rcb->rcb_state |= RCB_CSRR_LOCK;
    shad_rcb->rcb_iso_level = RCB_CURSOR_STABILITY;
    shad_rcb->rcb_state |= RCB_CSRR_LOCK;
    shad_rcb->rcb_k_mode = RCB_K_IS;
    arch_rcb->rcb_iso_level = RCB_CURSOR_STABILITY;
    arch_rcb->rcb_state |= RCB_CSRR_LOCK;
    arch_rcb->rcb_k_mode = RCB_K_IS;

    switch(action)
    {
	case DM2REP_DELETE:
	    /*
	    ** find un-archived shadow record(s) for this base record
	    */
	    status = find_shadow(DM2REP_DELETE, int_date, dcb, rcb, shad_tcb, 
		&shad_rec, record, &prio, &cdds_no, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** build and insert archive records
	    */
	    status = build_archive(dcb, rcb, arch_rcb, shad_rec, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** update in_archive field in shadow record(s) and replace shadow 
	    ** record(s) in table
	    */
	    status = update_shadow(shad_rcb, shad_tcb, rcb, tcb, shad_rec, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** build and insert new shadow record
	    */
	    status = build_shadow(dcb, rcb, tcb, shad_rcb, shad_tcb, shad_rec,
		DM2REP_DELETE, record, int_date, FALSE, &prio, &cdds_no, 
		dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** add input queue record
	    */
	    status = add_input_queue(dcb, rcb, DM2REP_DELETE, shad_rec,
		int_date, prio, cdds_no, dberr);

	    break;
	case DM2REP_INSERT:
	    /*
	    ** get non archived shadow records (for deletes only)
	    */
	    status = find_shadow(DM2REP_INSERT, int_date, dcb, rcb, shad_tcb, 
	    	&shad_rec, record, &prio, &cdds_no, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** update in_archive field in shadow record(s) and replace shadow 
	    ** record(s) in table
	    */
	    status = update_shadow(shad_rcb, shad_tcb, 
		rcb, tcb, shad_rec, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** add new shadow record
	    */
	    status = build_shadow(dcb, rcb, tcb, shad_rcb, shad_tcb, shad_rec,
		DM2REP_INSERT, record, int_date, FALSE, &prio, &cdds_no,
		dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** add input queue record
	    */
	    status = add_input_queue(dcb, rcb, DM2REP_INSERT, shad_rec,
		int_date, prio, cdds_no, dberr);

	    break;
	case DM2REP_UPDATE:
	    /*
	    ** get non-archived shadow records
	    */
	    status = find_shadow(DM2REP_UPDATE, int_date, dcb, rcb, shad_tcb, 
		&shad_rec, record, &prio, &cdds_no, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** add new archive records using shadow info
	    */
	    status = build_archive(dcb, rcb, arch_rcb, shad_rec, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** set in_archive field in shadow records
	    */
	    status = update_shadow(shad_rcb, shad_tcb, 
		rcb, tcb, shad_rec, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** add new shadow record
	    */
	    status = build_shadow(dcb, rcb, tcb, shad_rcb, shad_tcb, shad_rec,
		DM2REP_UPDATE, record, int_date, FALSE, &prio, &cdds_no,
		dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** add input queue record
	    */
	    status = add_input_queue(dcb, rcb, DM2REP_UPDATE, shad_rec,
		int_date, prio, cdds_no, dberr);

	    break;
	default:
	    /* bad option */
	    SETDBERR(dberr, 0, E_DM9554_REP_BAD_OPERATION);
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
            	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	err_code, (i4)0);
	    return(E_DB_ERROR);
	    break;
    }

    /*
    ** cleanup
    */
    if (shad_rec)
    	MEfree((char *)shad_rec);
    if (status != E_DB_OK && dberr->err_code == E_DM0046_DUPLICATE_RECORD)
    {
	uleFormat(NULL, E_DM9581_REP_SHADOW_DUPLICATE, NULL, ULE_LOG, 
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 
		(i4)1, DB_MAXNAME, tcb->tcb_rel.relid.db_tab_name);
	SETDBERR(dberr, 0, E_DM0046_DUPLICATE_RECORD);
    }
    else if (status != E_DB_OK && dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, NULL, 0, NULL, err_code, 0);
	/* return deadlock errors to DMF caller */
	if (dberr->err_code == E_DM9042_PAGE_DEADLOCK ||
            dberr->err_code == E_DM9044_ESCALATE_DEADLOCK ||
            dberr->err_code == E_DM9045_TABLE_DEADLOCK ||
            dberr->err_code == E_DM905A_ROW_DEADLOCK)
	{
	    uleFormat(NULL, E_DM9572_REP_CAPTURE_FAIL, NULL, ULE_LOG, 
	    		(DB_SQLSTATE *)NULL, 0, 0, 0, err_code, 0);
	    SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
	}
	else
	    SETDBERR(dberr, 0, E_DM9572_REP_CAPTURE_FAIL);

    }

    return (status);
}

/*
** Name: find_shadow - find unarchived shadow record(s) for this base record
**
** Description:
** 	This routine finds the unarchived shaddow record for this base
**	table record and returns it in the shadow table RCB, if there is no
**	record one is created
**
** Inputs:
**	trans_type		type of update
**	dcb			database control block
**	base_rcb		base table rcb
**	shad_tcb		shadow table tcb
**	shad_head		pointer to shadow records
**	record			updated base record
**
** Outputs:
**	int_date		current date and time in internal format
**	shad_head		queue of qualifying shadow records
**	err_code
**
** Returns:
**	status
**
** Side Effects:
** 	Shadow table rcb will be positioned on the found shadow record.
**	Memory is allocated for the shadow record, calling routine must free
**	when done.
**
** History:
**	25-apr-96 (stephenb)
**	    Created for phase 1 of the DBMS replication project
**	14-aug-96 (stephenb)
**	    Adding code for priority based collision resolution
**	21-aug-96 (stephenb)
**	    Add code for cdds lookup table checks.
**	13-mar-97 (stephenb)
**	    When looking for shadow records for updates use the old record
**	    value, not the new one. Fakes for update records should also 
**	    use the old record.
**	27-apr-97 (stephenb)
**	    shadow table secondary now contains the in_archive field, check for
**	    it there before retrieveing the base table record.
**	29-apr-97 (stephenb)
**	    use offset rather than key_offset when scrolling through the key
**	    fields for index lookups, if the key fields are in different
**	    order to the base table fields, they will have different offsets.
**	7-may-97 (stephenb)
**	    Trying to copy the in_archive field into shad_rec before allocating
**	    memory causes a segv (oops), move copy after allocation.
**	4-jun-97 (stephenb)
**	    Allow the replication key to be a secondary index, we will use 
**	    the collumn attributes to work out what has been used for a 
**	    replication key, rather than assuming it's the primary index.
**      13-Jun-97 (fanra01)
**          Fix double memory free. Exception on insert into table.
**	25-jun-97 (stephenb)
**	    When searching for shadow records, make sure we only
**	    incriment the number found for those that are not in_archive.
**	    also re-set in_archive value for next loop itteration.
**	27-oct-97 (stephenb)
**	    variable 'i' is overloaded, causing the code to take an incorrect
**	    path when there are no shadow records. alter one coccurence to
**	    'num_recs'. We should also use the shadow TCB when checking key
**	    length, the base TCB may have no keys (we can replicate from 
**	    secondary indexes).
**	4-nov-97 (stephenb)
**	    We can't use the shadow (or it's index) TCB in the above change to
**	    determin key width, we have to work it out from the attribute
**	    data retrieved from dd_regist_columns.
**	2-dec-97 (stephenb)
**	    tcb_atts_ptr should be indexed from 1 not zero when checking
**	    for replication key width.
**	8-dec-97 (stephenb)
**	    When calculating key attributes for position on shadow index,
**	    attribute numbers always start at 1 and grow for each key, they
**	    do not necesarily follow the base table att number. This is the
**	    cause of bug 87582.
**      23-sep-98 (gygro01)
**          Bug 92707
**          When calculating key attributes for position on shadow index and
**          the order of the key attributes is different than the one of the
**          base table, the base_key array should be filled in the key 
**          sequence order, otherwise we can't position on the shadow index.
**          Shadow index is always created in the key sequence order of the
**          primary key of the base table (fix for bug 77354). 
**	04-April-2005 (inifa01) b114221 INGSRV3230
**	    An update transaction on a replicated table fails with error; E_US1208
**	    duplicate records where found, if there is no unarchived record for
**	    the row being updated in the shadow table.  
**	    Fix: In this case build_shadow() is called to 'fake' the row in the 
**	    shadow table.  Added missing check for fake = FALSE so that build_shadow()  
**	    is not called again after dm2r_get() returns E_DM0055_NONEXT. 
*/
static DB_STATUS
find_shadow(
	i2			trans_type,
	char			*int_date,
	DMP_DCB			*dcb,
	DMP_RCB			*base_rcb,
	DMP_TCB			*shad_tcb,
	DMP_SHAD_REC_QUE	**shad_head,
	char			*record,
	i2			*prio,
	i2			*cdds_no,
	DB_ERROR		*dberr)
{
    DMP_TCB		*base_tcb = base_rcb->rcb_tcb_ptr;
    DMP_RCB		*shad_idx_rcb = base_rcb->rep_shadidx_rcb;
    DMP_TCB		*shad_idx_tcb = base_rcb->rep_shadidx_rcb->rcb_tcb_ptr;
    DMP_RCB		*shad_rcb = base_rcb->rep_shad_rcb;
    DM2R_KEY_DESC	*base_key = NULL;
    char		*shad_idx_rec = NULL;
    STATUS		cl_stat;
    i4			i;
    i4			num_recs;
    char		*info_start;
    DMP_SHAD_REC_QUE	*shad_rec = NULL;
    DB_STATUS		status;
    DM_TID		shad_idx_tid;
    bool		fake = FALSE;
    i4			num_keys = 0;
    i4			key_width = 0;
    i4                  key_seq = 0;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;) /* error checking */
    {
        /*
    	** get the key info from the base record
    	*/
    	base_key = (DM2R_KEY_DESC *)MEreqmem(0, 
	    base_tcb->tcb_rel.relatts * sizeof(DM2R_KEY_DESC), TRUE, &cl_stat);
    	if (base_key == NULL || cl_stat != OK)
    	{
	    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	err_code, (i4)0);
	    status = E_DB_ERROR;
	    break;
    	}
        for (i = 0; i < base_tcb->tcb_rel.relatts; i++)
        {
	    if (base_tcb->tcb_data_rac.att_ptrs[i]->rep_key_seq == 0)
		continue;
            key_seq = base_tcb->tcb_data_rac.att_ptrs[i]->rep_key_seq - 1;
	    base_key[key_seq].attr_operator = DM2R_EQ;
	    base_key[key_seq].attr_number = 
                base_tcb->tcb_data_rac.att_ptrs[i]->rep_key_seq;
	    /*
	    ** use old record value for deletes (we have no new record)
	    ** and updates.
	    */
	    if (trans_type == DM2REP_DELETE || trans_type == DM2REP_UPDATE)
	    	base_key[key_seq].attr_value = base_rcb->rcb_record_ptr +
		    base_tcb->tcb_data_rac.att_ptrs[i]->offset;
	    else
	    	base_key[key_seq].attr_value = record + 
		    base_tcb->tcb_data_rac.att_ptrs[i]->offset;
	    num_keys++;
    	}
    	/*
    	** now position using shadow table's secondary index
	** and then get shadow table record
    	*/
    	status = dm2r_position(shad_idx_rcb, DM2R_QUAL, base_key, 
	    num_keys, (DM_TID *)NULL, 
	    dberr);
	if (status != E_DB_OK)
	    break;
	shad_idx_rec = (char *)MEreqmem(0, (u_i4)shad_idx_tcb->tcb_rel.relwid,
	    TRUE, &cl_stat);
	if (shad_idx_rec == NULL || cl_stat != OK)
	{
	    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	err_code, (i4)0);
	    status = E_DB_ERROR;
	    break;
	}
	shad_rec = (DMP_SHAD_REC_QUE *)
	    MEreqmem(0, (u_i4)(sizeof(DMP_SHAD_REC_QUE)) + 
	    shad_tcb->tcb_rel.relwid, TRUE, &cl_stat);
	if (shad_rec == NULL || cl_stat != OK)
	{
	    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	err_code, (i4)0);
	    status = E_DB_ERROR;
	    break;
	}
	*shad_head = shad_rec;
	shad_rec->next_rec = shad_rec;
	shad_rec->prev_rec = shad_rec;
	for (num_recs = 0;;)
	{
	    status = dm2r_get(shad_idx_rcb, &shad_idx_tid, DM2R_GETNEXT, 
	    	shad_idx_rec, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT && num_recs == 0)
		{
		    /* no shadow record, so create one , O.K. for inserts*/
		    if (trans_type == DM2REP_INSERT)
		    {
			status = E_DB_OK;
			break;
		    }
		    else
		    {
			/* use the old base record for updates and deletes */
			status = build_shadow(dcb, base_rcb, base_tcb, 
			    shad_rcb, shad_tcb, shad_rec, DM2REP_INSERT, 
			    base_rcb->rcb_record_ptr, int_date, TRUE, prio, 
			    cdds_no, dberr);
			if (status != E_DB_OK)
			    break;
			fake = TRUE;
		    }
		    
		 }
		 else if (dberr->err_code == E_DM0055_NONEXT)
		 {
		    status = E_DB_OK; 
	    	    break;
		 }
		 else
		     break;
	    }
	    /*
	    ** The secondary index should contain the in_archive field to 
	    ** prevent un-necesary base table lookups, so we'll check it here
	    */
	    shad_rec->shad_info.in_archive = 0;
	    if (!fake)
	    {
	        for (i = 1; i <= shad_idx_tcb->tcb_rel.relatts; i++)
	        {
	            if (MEcmp(
		        (base_rcb->rcb_xcb_ptr && 
		          base_rcb->rcb_xcb_ptr->xcb_scb_ptr &&
		          base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &&
		          (*base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &
			      CUI_ID_REG_U)) ? "IN_ARCHIVE" : "in_archive", 
		        shad_idx_tcb->tcb_atts_ptr[i].name.db_att_name, 10) == 0) 
	            {
		        /* this is the in_archive field */
		        MEcopy(shad_idx_rec + shad_idx_tcb->tcb_atts_ptr[i].offset,
		            1, (char *)&shad_rec->shad_info.in_archive);
		        break;
	            }
		} 
	    }

	    if (shad_rec->shad_info.in_archive)
	        continue;
	    if (!fake)
	    {
	    	/*
	    	** NOTE: we assume here that tidp is always the last field in 
	    	** an index table
	    	*/
	    	MEcopy(shad_idx_rec + shad_idx_tcb->tcb_rel.relwid - 
		    sizeof(DM_TID), sizeof(DM_TID), (char *)&shad_rec->tid);
	    	status = dm2r_get(shad_rcb, &shad_rec->tid, DM2R_BYTID, 
		    shad_rec->shad_rec, dberr);
	    	if (status != E_DB_OK)
		    break;
	    }
	    /*
	    ** test for in_archive and trans_type fields, we do this here to
	    ** avoid un-necesary MEcopys
	    */
	    key_width = 0;
	    for (i = 1; i <= base_tcb->tcb_rel.relatts; i++)
	    {
		if (base_tcb->tcb_atts_ptr[i].rep_key_seq)
		    key_width += base_tcb->tcb_atts_ptr[i].length;
	    }
            info_start = shad_rec->shad_rec + key_width;
	    MEcopy(info_start + 36, 1, (char *)&shad_rec->shad_info.in_archive);
	    MEcopy(info_start + 39, 2, (char *)&shad_rec->shad_info.trans_type);
	    if (shad_rec->shad_info.in_archive || 
		(trans_type == DM2REP_INSERT && 
		shad_rec->shad_info.trans_type != DM2REP_DELETE))
		continue;
	    /*
	    ** shadow table fixed columns are at the end of the table,
	    ** due to bus alignment issues we have to copy the record
	    ** into the DMP_REP_SHAD_INFO structure one field at a time
	    */
	    MEcopy(info_start, 2, (char *)&shad_rec->shad_info.database_no);
	    MEcopy(info_start + 2, 4, 
	    	(char *)&shad_rec->shad_info.transaction_id);
	    MEcopy(info_start + 6, 4, (char *)&shad_rec->shad_info.sequence_no);
	    MEcopy(info_start + 10, 12, 
		(char *)&shad_rec->shad_info.trans_time);
	    /* add 1 for date null byte */
	    MEcopy(info_start + 23, 12, (char *)&shad_rec->shad_info.dist_time);
	    /* add 1 for date null byte */
	    MEcopy(info_start + 37, 2, (char *)&shad_rec->shad_info.cdds_no);
	    MEcopy(info_start + 45, 2, 
		(char *)&shad_rec->shad_info.old_source_db);
	    MEcopy(info_start + 47, 4, 
		(char *)&shad_rec->shad_info.old_transaction_id);
	    MEcopy(info_start + 51, 4, 
		(char *)&shad_rec->shad_info.old_sequence_no);
	    /*
	    ** get another shad rec and add to queue
	    */
	    shad_rec->next_rec = (DMP_SHAD_REC_QUE *)MEreqmem(0, (u_i4)
		(sizeof(DMP_SHAD_REC_QUE) + shad_tcb->tcb_rel.relwid), 
		TRUE, &cl_stat);
	    if (shad_rec->next_rec == NULL || cl_stat != OK)
	    {
	    	uleFormat(dberr, E_DM9557_REP_NO_SHAD_MEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		status = E_DB_ERROR;
		break;
	    }
	    shad_rec->next_rec->prev_rec = shad_rec;
	    shad_rec = shad_rec->next_rec;
	    if (fake)
		break;
	    num_recs++;
	}
	if (*shad_head == shad_rec) /* no shadow records */
	    *shad_head = NULL;
	/*
	** cleanup unwanted memory, and remove from queue
	*/
	if (shad_rec->prev_rec)
	    shad_rec->prev_rec->next_rec = NULL;
	if (shad_rec)
        {
	    MEfree((char *)shad_rec);
            shad_rec = NULL;
        }

	break;
    }
    /*
    ** cleanup
    */
    if (base_key)
	MEfree((char *)base_key);

    if (shad_idx_rec)
	MEfree(shad_idx_rec);

    if (shad_rec)
	MEfree((char *)shad_rec);

    return (status);
}

/*
** Name: build_archive - build a new archive record from shadow and base tables
**
** Description:
**	build an archive record from the shadow record and the base table
**	record.
**
** Inputs:
**	dcb		database control block
**	base_rcb	RCB of base table
**	arch_rcb	RCB of archive table
**	shad_head	queue of qualifying shadow records
**
** Outputs:
**	err_code
**
** Returns:
**	status
**
** Side effects:
**	adds new archive records for given shadow records
**
** History:
**	26-apr-96 (stephenb)
**	    Created for phase 1 of DBMS replication project
**	29-apr-97 (stephenb)
**	    Add code to deal with the fact that archive fields may be in a 
**	    different order to base table fields.
**	15-sep-97 (stephenb)
**	    add code to cope with blobs when building archive records.
**	16-oct-97 (stephenb)
**	    do not check for duplicates when inserting archive records that
**	    contain peripherals (blobs), the duplicate check doesn't work
**	    properly.
**	3-nov-97 (stephenb)
**	    initialize status.
**	14-apr-98 (stephenb)
**	    Set the rcb pointer in the archive table coupon, some of the
**	    dmpe functions need to use it.
**	11-may-1998 (thaju02)
**	    segv occurs in dmpe_put(). changed ADP_POP_LONG_TEMP to 
**	    ADP_POP_SHORT_TEMP. (B90819)
**	    Also backed out 14-apr-1998 (stephenb) change, not needed.
**      12-jun-1998 (thaju02)
**	    changed pop_temporary = ADP_POP_LONG_TEMP; regression bug
**	    (B91469)
**	26-apr-99 (stephenb)
**	    Init new pop_info field in pop_cb.
**	11-May-2004 (schka24)
**	    Name change for pop-temporary flag.
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**      20-Dec-2004 (inifa01) Bug 112748/INGREP164
**          Deleting records from a replicated table containing BLOB
**          columns could sometimes lead to error E_DM9049 crashing the
**          server with stack dump; adc_xform(), adu_couponify(),
**          build_archive(),dm2rep_capture() etc.
**          FIX:  If the adu_redeem() call was just to add the terminating
**          NULL byte and no segment data was retrieved, then call adu_couponify
**          in a way that it can check for more segments or terminate if none.
**	6-Dec-2006 (kibro01) b117206
**	    Deleting records from a replicated tables containing BLOB
**	    columns still crashes after the fix to b112748.  The NEED_NULL
**	    has been marked and adu_redeem has gone through setting fip_done
**	    and the segment length to 0, along with the status being OK.  The
**	    final segment has already been processed in adu_couponify at this stage
**	    so under these circumstances we can loop round and allow adu_redeem
**	    to exit the loop.
**      15-dec-2006 (huazh01)
**          Extends the fix for b114508 to build_archive(). 
**          This fixes bug 117355.
*/
static DB_STATUS
build_archive(
	DMP_DCB			*dcb,
	DMP_RCB			*base_rcb,
	DMP_RCB			*arch_rcb,
	DMP_SHAD_REC_QUE	*shad_head,
	DB_ERROR		*dberr)
{
    DMP_TCB		*base_tcb = base_rcb->rcb_tcb_ptr;
    DMP_TCB		*arch_tcb = arch_rcb->rcb_tcb_ptr;
    ADF_CB		*adf_scb = base_rcb->rcb_adf_cb;
    i4			size = 0;
    i4			rep_size = 0;
    STATUS		cl_stat;
    i4			i, j;
    char		*archive_rec = NULL;
    DMP_SHAD_REC_QUE	*shad_rec;
    DB_STATUS		status = E_DB_OK;
    char		*workptr = NULL;
    char		*resptr = NULL;
    char		*arch_work = NULL;
    char		*inptr = NULL;
    char		*arch_cpn = NULL;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (shad_head == NULL) 
	/* nothing to do */
	return (E_DB_OK);
    /*
    ** find size of variable part of record
    */
    for (i = 1; i <= base_tcb->tcb_rel.relatts; i++)
    {
	if (base_tcb->tcb_atts_ptr[i].replicated)
	    size += base_tcb->tcb_atts_ptr[i].length;
    }
    /*
    ** now allocate memory for record
    */
    archive_rec = (char *)MEreqmem((u_i4)0, size + 
	sizeof(shad_head->shad_info.database_no) + 
	sizeof(shad_head->shad_info.transaction_id) +
	sizeof(shad_head->shad_info.sequence_no), TRUE, &cl_stat);
    if (archive_rec == NULL || cl_stat != OK)
    {
	uleFormat(dberr, E_DM9558_REP_NO_ARCH_MEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    /*
    ** fill variable part of records (info is the same for all records)
    ** rcb_record_ptr should contain old record value for update/delete
    */
    for (i = 1; i <= base_tcb->tcb_rel.relatts; i++)
    {
	if (base_tcb->tcb_atts_ptr[i].replicated)
	{
	    /*
	    ** find the offset in the archive record, we do this by looking
	    ** for the field name
	    */
	    for (j = 1; j <= arch_tcb->tcb_rel.relatts; j++)
	    {
		if (MEcmp(arch_tcb->tcb_atts_ptr[j].name.db_att_name,
		    base_tcb->tcb_atts_ptr[i].name.db_att_name, DB_MAXNAME) 
		    == 0)
	 	    break;
	    }
	    if (j > arch_tcb->tcb_rel.relatts)
	    {
		/* archive field not found */
		uleFormat(dberr, E_DM956F_REP_NO_ARCH_COL, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)2, 0, 
		    base_tcb->tcb_atts_ptr[i].name.db_att_name, 0,
		    base_tcb->tcb_rel.relid.db_tab_name);
		MEfree(archive_rec);
		return (E_DB_ERROR);
	    }
	    /*
	    ** check for non-null peripheral datatypes (BLOBs)
	    */
	    if ((base_tcb->tcb_atts_ptr[i].flag & ATT_PERIPHERAL) &&
		(arch_tcb->tcb_atts_ptr[j].type > 0 ||
		/* we have a null datatype so test null byte */
		 *(archive_rec + arch_tcb->tcb_atts_ptr[j+1].offset - 1) == 0))
	    {
		DB_DATA_VALUE	result, coupon, workspace, arch_coup, input;
		ADP_PERIPHERAL	cpn, *inp_cpn;
		ADP_LO_WKSP	arch_wksp;
		ADP_LO_WKSP     *temp_wksp =(ADP_LO_WKSP *)NULL;
		i4		cont = 0;
		i4		offset = ADP_HDR_SIZE + sizeof(i4);
		i4		acplen = (arch_tcb->tcb_atts_ptr[j].type > 0) ?
					 sizeof(ADP_PERIPHERAL) : 
					 sizeof(ADP_PERIPHERAL) + 1;

		/* allocate result buffers */
		arch_cpn = MEreqmem(0, sizeof(ADP_PERIPHERAL) + 1, TRUE, 
		    &cl_stat);
    		if (arch_cpn == NULL || cl_stat != OK)
    		{
		    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		    status = E_DB_ERROR;
		    break;
    		}
		workptr = MEreqmem(0, DB_MAXTUP + sizeof(ADP_LO_WKSP), 
		    TRUE, &cl_stat);
    		if (workptr == NULL || cl_stat != OK)
    		{
		    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		    status = E_DB_ERROR;
		    break;
    		}
		resptr = MEreqmem(0, DB_MAXTUP, TRUE, &cl_stat);
    		if (resptr == NULL || cl_stat != OK)
    		{
		    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		    status = E_DB_ERROR;
		    break;
    		}
		arch_work = MEreqmem(0, DB_MAXTUP, TRUE, &cl_stat);
    		if (arch_work == NULL || cl_stat != OK)
    		{
		    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		    status = E_DB_ERROR;
		    break;
    		}
		inptr = MEreqmem(0, DB_MAXTUP, TRUE, &cl_stat);
    		if (inptr == NULL || cl_stat != OK)
    		{
		    uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		    status = E_DB_ERROR;
		    break;
    		}

		/* set up the data handlers */
		MEcopy(base_rcb->rcb_record_ptr + 
		    base_tcb->tcb_atts_ptr[i].offset, sizeof(ADP_PERIPHERAL),
		    &cpn);
		coupon.db_data = (char *)&cpn;
		result.db_data = resptr;
		input.db_data = inptr;
		workspace.db_data = workptr;
		result.db_datatype = base_tcb->tcb_atts_ptr[i].type;
		coupon.db_datatype = base_tcb->tcb_atts_ptr[i].type;
		input.db_datatype = arch_tcb->tcb_atts_ptr[j].type;
		workspace.db_datatype = 0;
		result.db_length = DB_MAXTUP;
		input.db_length = DB_MAXTUP;
		coupon.db_length = sizeof(ADP_PERIPHERAL);
		workspace.db_length = DB_MAXTUP + sizeof(ADP_LO_WKSP);
		coupon.db_prec = 0;
		result.db_prec = 0;
		input.db_prec = 0;
		workspace.db_prec = 0;
		result.db_collID = base_tcb->tcb_atts_ptr[i].collID;
		coupon.db_collID = base_tcb->tcb_atts_ptr[i].collID;
		input.db_collID = arch_tcb->tcb_atts_ptr[j].collID;
		workspace.db_collID = -1;

		arch_coup.db_data = arch_cpn;
		arch_coup.db_datatype = arch_tcb->tcb_atts_ptr[j].type;
		arch_coup.db_length = acplen;
		arch_coup.db_prec = 0;
		arch_coup.db_collID = arch_tcb->tcb_atts_ptr[j].collID;
		/* clear null byte */
		if (arch_tcb->tcb_atts_ptr[j].type < 0)
		    arch_cpn[acplen - 1] = 0;
		arch_wksp.adw_fip.fip_work_dv.db_data = arch_work;
		arch_wksp.adw_fip.fip_work_dv.db_length = DB_MAXTUP;
		arch_wksp.adw_fip.fip_work_dv.db_prec = 0;
		arch_wksp.adw_fip.fip_work_dv.db_collID = -1;
		/* Couponify to a temp, will get moved to archive etab
		** when record put is finalized.
		** FIXME could we set up pop-info so that put optim would
		** happen?  we know the target table...
		*/
		arch_wksp.adw_fip.fip_pop_cb.pop_temporary = ADP_POP_TEMPORARY;
		arch_wksp.adw_fip.fip_pop_cb.pop_info = NULL;

		inp_cpn = (ADP_PERIPHERAL *)inptr;
		inp_cpn->per_tag = ADP_P_GCA_L_UNK;
		inp_cpn->per_length0 = 0;
		inp_cpn->per_length1 = 0;
		inp_cpn->per_value.val_nat = 1;

		/* construct the archive coupon from the base table coupon */
		status = E_DB_INFO;
		while (status == E_DB_INFO)
		{
		    status = adu_redeem(adf_scb, &result, &coupon, 
			&workspace, cont);
		    if (status > E_DB_INFO)
			break;
		    MEcopy(resptr + offset, DB_MAXTUP - offset, inptr + offset);

                    temp_wksp = (ADP_LO_WKSP *)workspace.db_data;

                    /*
                    ** The point here is if there are no more segments or data to get/couponify,
                    ** and adu_couponify wasn't given enough information to determine that it
                    ** needs to terminate on the last pass, then call it in such a away that it
                    ** can check if there are more segments to read and if not terminate.
                    */
                    if ((temp_wksp->adw_fip.fip_state == ADW_F_DONE_NEED_NULL) &&
                        (arch_wksp.adw_shared.shd_exp_action == ADW_GET_DATA) &&
                        (status == E_DB_OK))
                        arch_wksp.adw_shared.shd_exp_action = ADW_NEXT_SEGMENT;

		    /* If we only read this extra segment for the null, and we are 
		    ** already marked as finished, and the status is E_DB_OK,
		    ** this means we looped round before for the extra NULL already,
		    ** and now we have a 0-length segment, which needs no processing
		    ** from adu_couponify, so skip round to adu_redeem, which will
		    ** then terminate this loop (kibro01) b117206
		    */
		    if ((temp_wksp->adw_fip.fip_state == ADW_F_DONE_NEED_NULL) &&
			(temp_wksp->adw_fip.fip_done) && (status == E_DB_OK))
			continue;

		    status = adu_couponify(adf_scb, cont, &input, &arch_wksp,
			&arch_coup);
		    if (status > E_DB_INFO)
			break;
		    cont = 1;
		    offset = 0;
		}
		if (status > E_DB_INFO)
		{
		    char	att_name[DB_MAXNAME + 1];
		    char	tab_name[DB_MAXNAME + 1];

		    MEcopy(base_tcb->tcb_atts_ptr[i].name.db_att_name, 
			DB_MAXNAME, att_name);
		    att_name[DB_MAXNAME] = EOS;
		    MEcopy(base_tcb->tcb_rel.relid.db_tab_name, 
			DB_MAXNAME, tab_name);
		    tab_name[DB_MAXNAME] = EOS;
		    uleFormat(dberr, E_DM9576_REP_CPN_CONVERT, (CL_ERR_DESC *)NULL, ULE_LOG,
            		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            		err_code, (i4)2, STtrmwhite(att_name), att_name, 
			STtrmwhite(tab_name), tab_name);
		    break;
		}
		MEcopy(arch_cpn, acplen, 
		    archive_rec + arch_tcb->tcb_atts_ptr[j].offset);
		status = E_DB_OK;
	    }
	    else
	    {
	        MEcopy(base_rcb->rcb_record_ptr + 
		    base_tcb->tcb_atts_ptr[i].offset,
		    base_tcb->tcb_atts_ptr[i].length, 
		    archive_rec + arch_tcb->tcb_atts_ptr[j].offset);
	    }
	    rep_size += base_tcb->tcb_atts_ptr[i].length;
	}
    }

    /* 
    ** now the fixed part for each record, then write record
    */

    /* b117355, disable RCB_CSRR_LOCK while writing archive_rec */
    arch_rcb->rcb_state &= ~RCB_CSRR_LOCK;

    for(shad_rec = shad_head; shad_rec; 
	shad_rec = shad_rec->next_rec)
    {
        if (status != E_DB_OK)
	    break;
	size = rep_size;
    	MEcopy((char *)&shad_rec->shad_info.database_no, 
	    sizeof(shad_rec->shad_info.database_no), 
	    archive_rec + size);
    	size += sizeof(shad_rec->shad_info.database_no);
    	MEcopy((char *)&shad_rec->shad_info.transaction_id, 
	    sizeof(shad_rec->shad_info.transaction_id),
	    archive_rec + size);
    	size += sizeof(shad_rec->shad_info.transaction_id);
    	MEcopy((char *)&shad_rec->shad_info.sequence_no, 
	    sizeof(shad_rec->shad_info.sequence_no),
	    archive_rec + size);
        /*
    	** insert new archive record, we cannot check for duplicates if
	** the record contains a long datatype, since the check assumes
	** the coupon of the record retrieved for comparison will be word 
	** alligned, and derefferences an integer field in it. Since the
	** record used for the comparison is lifted directly from the 
	** pages memory area, and records may have an odd offset in a page, 
	** a non word alligned record may be used for comparison, which 
	** causes a bus error.
    	*/
	if (arch_tcb->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    status = dm2r_put(arch_rcb, DM2R_DUPLICATES, archive_rec, dberr);
	}
	else
	{
    	    status = dm2r_put(arch_rcb, DM2R_NODUPLICATES, archive_rec, dberr);
	}
    }

    /* Reinstate RCB_CSRR_LOCK */
    arch_rcb->rcb_state |= RCB_CSRR_LOCK;


    /* cleanup */
    if (archive_rec)
        MEfree(archive_rec);
    if (workptr)
        MEfree(workptr);
    if (resptr)
        MEfree(resptr);
    if (arch_work)
        MEfree(arch_work);
    if (inptr)
        MEfree(inptr);
    if (arch_cpn)
        MEfree(arch_cpn);

    return(status);
}

/*
** Name: update_shadow - update in_archive field in shadow records
**
** Decsription:
**	This routine takes given shadow records and updates them to indicate
**	that a related archive record exists by seting the in_archive field
**	and then writes the records back to the shadow table
**
** Inputs:
**	shad_rcb 		shadow table rcb
**	shad_tcb		shadow table TCB
**	base_rcb		base table rcb
**	base_tcb		base table TCB
**	shad_head		queue of qualifying shadow records
**
** Outputs:
**	err_code
**
** Returns:
**	status
**
** History:
**	30-apr-96 (stephenb)
**	    Initial creation for phase 1 of DBMS replication project
**	14-May-98 (consi01) bug 90848 problem ingrep 34
**	    Removed consistency check as a tid of 0 is valid for tables
**	    with a hash structure.
*/
static DB_STATUS
update_shadow(
	DMP_RCB			*shad_rcb,
	DMP_TCB			*shad_tcb,
	DMP_RCB			*base_rcb,
	DMP_TCB			*base_tcb,
	DMP_SHAD_REC_QUE	*shad_head,
	DB_ERROR		*dberr)
{
    i4			i;
    DB_STATUS		status;
    i4			offset = 0;
    DMP_SHAD_REC_QUE	*shad_rec;
    i1			in_archive = 1;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (shad_head == NULL) 
	/* nothing to do */
	return (E_DB_OK);
    /*
    ** find offset of in_archive field
    */
    for(i = 1; i <= shad_tcb->tcb_rel.relatts; i++)
    {
	if (MEcmp(shad_tcb->tcb_atts_ptr[i].name.db_att_name, 
		(base_rcb->rcb_xcb_ptr && 
		  base_rcb->rcb_xcb_ptr->xcb_scb_ptr &&
		  base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &&
		  (*base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &
		  CUI_ID_REG_U)) ? "IN_ARCHIVE" : "in_archive", 10) == 0)
	{
	    offset = shad_tcb->tcb_atts_ptr[i].offset;
	    break;
	}
    }
    if (offset ==0)
    {
	/* can't find in_archive field */
	uleFormat(dberr, E_DM9559_REP_NO_IN_ARCH, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)1, DB_MAXNAME, base_tcb->tcb_rel.relid);
	return (E_DB_ERROR);
    }
    /*
    ** update and replace record(s) in database
    */
    for(shad_rec = shad_head; shad_rec; 
	shad_rec = shad_rec->next_rec)
    {
	MEcopy((char *)&in_archive, sizeof(in_archive), 
	    shad_rec->shad_rec + offset);
    	status = dm2r_replace(shad_rcb, &shad_rec->tid, DM2R_BYTID, 
	    shad_rec->shad_rec, (char *)NULL, dberr);
	if (status != E_DB_OK)
	    break;
    }

    return (status);
}

/*
** Name: build_shadow - build a new shadow table record
**
** Description:
**	This routine builds and inserts a new shadow table record using the
**	unique key from the base record and other shadow information
**
** Inputs:
**	dcb		database control block
**	base_rcb	base table RCB
**	base_tcb	base table TCB
**	shad_rcb	shadow table rcb
**	shad_tcb	shadow table TCB
**	shad_head	queue of qualifying shadow records
**	trans_type	type of update to capture
**	record		updated base table record
**	int_date	current date and time in internal format
**	fake		fake a record for find_shadow ?
**
** Outputs:
**	err_code
**
** Returns:
**	status
**
** Side Effects:
**	inserts a record in the shadow table
**
** History:
**	30-apr-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
**	14-aug-96 (stephenb)
**	    Adding code for priority based collision resolution
**	21-aug-96 (stephenb)
**	    Add code for cdds lookup table checks.
**	24-jan-97 (stephenb)
**	    Changes to some of the fields in the shadow table (build_shadow)
**	    for special case where shadow record doesn't exist for the
**	    base record we have updated.
**	27-apr-97 (stephenb)
**	    Add code to cope with the fact that the shadow table fields may be
**	    in a different order to the base table fields. Also correct error
**	    messaging so the parameters are printed correctly.
**	4-jun-97 (stephenb)
**	    Allow the replication key to be a secondary index, we will use 
**	    the collumn attributes to work out what has been used for a 
**	    replication key, rather than assuming it's the primary index.
**	23-jun-97 (stephenb)
**	    if this is a delete then use the current record when calling
**	    cdds_lookup() and check_priority(), we have no new record
**	    to use.
**	25-jun-97 (stephenb)
**	    Code cleanup, report error E_DM9573_REP_NO_SHAD when shad_head
**	    is NULL.
**	27-jun-97 (stephenb)
**	    Don't check non replicated columns when looking for key values.
**	20-nov-97 (stephenb)
**	    Fix bug 85136: whacked out scenarios which re-insert a record
**	    that has existed before and been updated, or update a record
**	    back to a value it had at some previous time, cause multiple
**	    un-archived shadow records. We need to deal with this and work
**	    out which one to use.
**      12-Apr-1999 (stial01)
**          build_shadow() Distinguish leaf/index info for BTREE/RTREE indexes
**	07-may-1999 (somsa01)
**	    When acessing record entries out of shad_curr, offset
**	    shad_cur->shad_rec, NOT shad_curr.
**	11-April-2005 (inifa01)
**	    Added dbname to message text for W_DM9579_REP_MULTI_SHADOW.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          Remove the RCB_CSRR_LOCK from the shadow rcb.
**      27-July-2005 (hanal04) Bug 114928 INGSRV3365
**          Correct parameter count for W_DM9579 call to ule_format().
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
*/
static DB_STATUS
build_shadow(
	DMP_DCB			*dcb,
	DMP_RCB			*base_rcb,
	DMP_TCB			*base_tcb,
	DMP_RCB			*shad_rcb,
	DMP_TCB			*shad_tcb,
	DMP_SHAD_REC_QUE	*shad_head,
	i2			trans_type,
	char			*record,
	char			*int_date,
	bool			fake,
	i2			*prio,
	i2			*cdds_no,
	DB_ERROR		*dberr)
{
    i4			i, j;
    i4			size = 0;
    i1			in_archive;
    i2			new_key;
    char		*shad_rec;
    STATUS		cl_stat;
    DB_STATUS		status;
    i4			default_value = 0;
    i2			def_small = 0;
    ADF_CB		adf_scb;
    DB_DATA_VALUE	date_from;
    DB_DATA_VALUE	date_to;
    i4		now;
    char		date_str[26];
    i4			sequence;
    DMP_SHAD_REC_QUE	*shad_curr, *shad_used;
    ADF_CB		*cmp_scb = base_rcb->rcb_adf_cb;
    DB_DATA_VALUE	adc_dv1, adc_dv2;
    i4			date_null;
    i4			comp;
    DB_ATTS             **base_keyatts;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    if (shad_head == NULL && trans_type != DM2REP_INSERT && !fake) 
    {
	/* no shadow record to work from */
	char	shad_tab[DB_MAXNAME +1], base_tab[DB_MAXNAME +1];

	MEcopy(shad_tcb->tcb_rel.relid.db_tab_name, DB_MAXNAME, shad_tab);
	MEcopy(base_tcb->tcb_rel.relid.db_tab_name, DB_MAXNAME, base_tab);
	shad_tab[DB_MAXNAME] = base_tab[DB_MAXNAME] = EOS;
	uleFormat(dberr, E_DM9573_REP_NO_SHAD, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)2, STtrmwhite(shad_tab), shad_tab,
	    STtrmwhite(base_tab), base_tab);
	return (E_DB_ERROR);
    }
    shad_rec = MEreqmem(0, shad_tcb->tcb_rel.relwid, TRUE, &cl_stat);
    if (shad_rec == NULL || cl_stat != OK) 
    {
	uleFormat(dberr, E_DM9557_REP_NO_SHAD_MEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    /*
    ** add keys first
    */
    for (i = 0; i < base_tcb->tcb_rel.relatts; i++)
    {
	base_keyatts = base_tcb->tcb_key_atts;

	/*
	** check if it's a replication key
	*/
	if (base_tcb->tcb_data_rac.att_ptrs[i]->replicated == FALSE ||
	    base_tcb->tcb_data_rac.att_ptrs[i]->rep_key_seq == 0)
	    continue;
	/*
	** the shadow table may not contain the keys in the same order, so
	** we need to search for the correct field to find the offset
	** at which to add the record (we do this by field name).
	*/
	for (j = 0; j < shad_tcb->tcb_rel.relatts; j++)
	{
	    if (MEcmp(shad_tcb->tcb_data_rac.att_ptrs[j]->name.db_att_name, 
		base_tcb->tcb_data_rac.att_ptrs[i]->name.db_att_name, DB_MAXNAME) == 0)
		break;
	}
	if (j == shad_tcb->tcb_rel.relatts)
    	{
	    /* index attribute not found in shadow table */
	    uleFormat(dberr, E_DM956D_REP_NO_SHAD_KEY_COL, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                err_code, (i4)2, 0, base_tcb->tcb_rel.relid.db_tab_name,
		0, base_keyatts[i]->name.db_att_name);
	    MEfree(shad_rec);
	    return(E_DB_ERROR);
    	}
	/* make sure field sizes match */
	if (base_tcb->tcb_data_rac.att_ptrs[i]->length != 
	    shad_tcb->tcb_data_rac.att_ptrs[j]->length)
	{
	    uleFormat(dberr, E_DM956E_REP_BAD_KEY_SIZE, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                err_code, (i4)4, 0, 
		base_keyatts[i]->name.db_att_name, 0,
		base_tcb->tcb_rel.relid.db_tab_name, sizeof(i4), 
		&(base_keyatts[i]->length), sizeof(i4),
		&(shad_tcb->tcb_data_rac.att_ptrs[j]->length));
	    MEfree(shad_rec);
	    return(E_DB_ERROR);
	}
	/*
	** use old key values for delete (we don't have new ones)
	*/
	if (trans_type == DM2REP_DELETE)
	    MEcopy(base_rcb->rcb_record_ptr + 
		base_tcb->tcb_data_rac.att_ptrs[i]->offset,
		base_tcb->tcb_data_rac.att_ptrs[i]->length, 
		shad_rec + shad_tcb->tcb_data_rac.att_ptrs[j]->offset);
	else
	    MEcopy(record + base_tcb->tcb_data_rac.att_ptrs[i]->offset,
	    	base_tcb->tcb_data_rac.att_ptrs[i]->length, 
		shad_rec + shad_tcb->tcb_data_rac.att_ptrs[j]->offset);
	size += base_tcb->tcb_data_rac.att_ptrs[i]->length;
    }
    /* now the fixed part */
    MEcopy((char *)&dcb->dcb_rep_db_no, 
	sizeof(dcb->dcb_rep_db_no), shad_rec + size);
    size += sizeof(dcb->dcb_rep_db_no);
    /* low part of tran id only for now */
    MEcopy((char *)&base_rcb->rcb_tran_id.db_low_tran, 
	sizeof(base_rcb->rcb_tran_id.db_low_tran), shad_rec + size);
    size += sizeof(base_rcb->rcb_tran_id.db_low_tran);
    if (fake)
	sequence = base_rcb->rcb_xcb_ptr->xcb_rep_seq - 1;
    else
	sequence = base_rcb->rcb_xcb_ptr->xcb_rep_seq;
    MEcopy((char *)&sequence, 
	sizeof(base_rcb->rcb_xcb_ptr->xcb_rep_seq), shad_rec + size);
    size += sizeof(base_rcb->rcb_xcb_ptr->xcb_rep_seq);
    /*
    ** perform date conversion in ADF and fill in date fields
    */
    adf_scb.adf_dfmt = DB_DMY_DFMT;
    adf_scb.adf_errcb.ad_ebuflen = 0;
    adf_scb.adf_exmathopt = ADX_IGN_MATHEX;
    now = TMsecs();
    cl_stat = TMcvtsecs(now, date_str);
    if (cl_stat != OK)
    {
	uleFormat(dberr, E_DM955D_REP_BAD_DATE_CVT, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	MEfree(shad_rec);
	return(E_DB_ERROR);
    }
    date_from.db_datatype = DB_CHR_TYPE;
    date_from.db_prec     = 0;
    date_from.db_length   = (i4)STlength(date_str) + 1;
    date_from.db_data     = date_str;
    date_from.db_collID	  = -1;
    date_to.db_datatype   = DB_DTE_TYPE;
    date_to.db_prec       = 0;
    date_to.db_length     = 12;
    date_to.db_data       = int_date;
    date_to.db_collID	  = -1;
    cl_stat = TMtz_init(&(adf_scb.adf_tzcb));
    if (cl_stat != OK)
    {
	uleFormat(dberr, E_DM955D_REP_BAD_DATE_CVT, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	MEfree(shad_rec);
	return(E_DB_ERROR);
    }
    status = adu_14strtodate(&adf_scb, &date_from, &date_to);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, E_DM955D_REP_BAD_DATE_CVT, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	MEfree(shad_rec);
	return(E_DB_ERROR);
    }
    MEcopy(int_date, 12, shad_rec + size);
    size += 13; /* add 1 because this is a nulable field */
    /* save offset of date null for later */
    date_null = size - 1;
    MEcopy(int_date, 12, shad_rec + size);
    size += 13; /* add 1 because this is a nulable field */
    in_archive = 0;
    MEcopy((char *)&in_archive, sizeof(in_archive), shad_rec + size);
    size += sizeof(in_archive);
    /*
    ** check cdds lookup table
    */
    if (base_rcb->rep_cdds_rcb)
    {
	if (trans_type == DM2REP_DELETE)
	    status = cdds_lookup(base_rcb, base_rcb->rcb_record_ptr, cdds_no,
		dberr);
	else
	    status = cdds_lookup(base_rcb, record, cdds_no, dberr);
	if (status != E_DB_OK)
	{
            MEfree(shad_rec);
	    return(status);
	}
    }
    if (*cdds_no)
        MEcopy((char *)cdds_no, sizeof(*cdds_no), shad_rec + size);
    else
        MEcopy((char *)&base_tcb->tcb_rep_info->dd_reg_tables.cdds_no,
	    sizeof(base_tcb->tcb_rep_info->dd_reg_tables.cdds_no), 
	    shad_rec + size);
    size += sizeof(base_tcb->tcb_rep_info->dd_reg_tables.cdds_no);
    MEcopy((char *)&trans_type, sizeof(trans_type), shad_rec + size);
    size += sizeof(trans_type);
    /*
    ** check priority
    */
    if (base_rcb->rep_prio_rcb)
    {
	if (trans_type == DM2REP_DELETE)
	    status = check_priority(base_rcb, base_rcb->rcb_record_ptr, prio,
		dberr);
	else
	    status = check_priority(base_rcb, record, prio, dberr);
	if (status != E_DB_OK)
	{
            MEfree(shad_rec);
	    return(status);
	}
    }
    else
        *prio = 0;
    MEcopy((char *)prio, sizeof(*prio), shad_rec + size);
    size += sizeof(*prio);
    if (fake)
	new_key = 1;
    else
    	new_key = 0;
    MEcopy((char *)&new_key, sizeof(new_key), shad_rec + size);
    size += sizeof(new_key);
    /*
    ** old values (from last shadow record)
    */
    if (trans_type == DM2REP_INSERT || fake)
    {
	MEcopy((char *)&def_small, sizeof(def_small), shad_rec + size);
	size += sizeof(def_small);
	MEcopy((char *)&default_value, sizeof(default_value), shad_rec + size);
	size += sizeof(default_value);
	MEcopy((char *)&default_value, sizeof(default_value), shad_rec + size);
	size += sizeof(default_value);
    }
    else
    {
	/*
	** if we have more than one old shadow record (due to some freak 
	** delete/re-add of records with the same replication keys, or 
	** multiple updates which end up updating to a previously used 
	** replication key), then we must use the most recent record.
	*/
	adc_dv1.db_datatype = DB_DTE_TYPE;
	adc_dv1.db_prec = 0;
	adc_dv1.db_length = sizeof(shad_head->shad_info.trans_time);
	adc_dv1.db_collID = -1;

	adc_dv2.db_datatype = DB_DTE_TYPE;
	adc_dv2.db_prec = 0;
	adc_dv2.db_length = sizeof(shad_head->shad_info.trans_time);
	adc_dv2.db_collID = -1;

	for (shad_used = shad_curr = shad_head; shad_curr != NULL;
	    shad_curr = shad_curr->next_rec)
	{
	    /* ignore null dates and don't bother checking the same record */
	    if (shad_used == shad_curr || *(shad_curr->shad_rec + date_null))
		continue;
	    else
	    {
		char	tabname[DB_MAXNAME +1];
		char	dbname[DB_MAXNAME +1];
	    	/*
	    	** if we got here, then we have many unarchived shadow
	    	** records, log a warning
	    	*/
		MEcopy(base_tcb->tcb_rel.relid.db_tab_name, 
		    DB_MAXNAME, tabname);
		MEcopy(dcb->dcb_name.db_db_name, DB_MAXNAME,
		    dbname);
		tabname[DB_MAXNAME] = 0;
		dbname[DB_MAXNAME] = 0;
	    	uleFormat(dberr, W_DM9579_REP_MULTI_SHADOW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, (i4)3, STtrmwhite(tabname), tabname,
		    STtrmwhite(dbname), dbname,
		    sizeof(base_rcb->rcb_tran_id.db_low_tran),
		    &(base_rcb->rcb_tran_id.db_low_tran));
	    }
	    adc_dv1.db_data = shad_curr->shad_info.trans_time;
	    adc_dv2.db_data = shad_used->shad_info.trans_time;
	    status = adc_compare(cmp_scb, &adc_dv1, &adc_dv2, &comp);
    	    if (status != E_DB_OK)
    	    {
		char	tabname[DB_MAXNAME +1];

		MEcopy(base_tcb->tcb_rel.relid.db_tab_name, 
		    DB_MAXNAME, tabname);
		tabname[DB_MAXNAME] = 0;
		uleFormat(dberr, E_DM957A_REP_BAD_DATE_CMP, (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)2, 
		    sizeof(base_rcb->rcb_tran_id.db_low_tran),
		    &(base_rcb->rcb_tran_id.db_low_tran), STtrmwhite(tabname), 
		    tabname);
		MEfree(shad_rec);
		return(E_DB_ERROR);
    	    }
	    if (comp >= 0) /* current date is greater than one in hand */
		shad_used = shad_curr;
	}

    	MEcopy((char *)&shad_used->shad_info.database_no, 
	    sizeof(shad_used->shad_info.database_no), shad_rec + size);
        size += sizeof(shad_used->shad_info.database_no);
    	MEcopy((char *)&shad_used->shad_info.transaction_id, 
	    sizeof(shad_used->shad_info.transaction_id), shad_rec + size);
    	size += sizeof(shad_used->shad_info.transaction_id);
    	MEcopy((char *)&shad_used->shad_info.sequence_no, 
	    sizeof(shad_used->shad_info.sequence_no), shad_rec + size);
	size += sizeof(shad_used->shad_info.sequence_no);
    }
    /*
    ** consistency check
    */
    if (size != shad_tcb->tcb_rel.relwid)
    {
	/* oops! wrong number of bytes in shadow record */
	uleFormat(dberr, E_DM955A_REP_BAD_SHADOW, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
        MEfree(shad_rec);
	return(E_DB_ERROR);
    }
    /* Disable RCB_CSRR_LOCK, while writting the shadow record */

    shad_rcb->rcb_state &= ~RCB_CSRR_LOCK;

    /*
    ** now write the record
    */
    status = dm2r_put(shad_rcb, (i4)0, shad_rec, dberr);


    /* Reinstate RCB_CSRR_LOCK */

    shad_rcb->rcb_state |= RCB_CSRR_LOCK;

    /*
    ** if we are faking shadow for caller, then copy into shad_head
    ** and add tid for caller
    */
    if (fake)
    {
	MEcopy(shad_rec, size, shad_head->shad_rec);
	shad_head->tid.tid_i4 = shad_rcb->rcb_currenttid.tid_i4;
    }
    /*
    ** cleanup
    */
    MEfree(shad_rec);

    return(status);
}

/*
** Name: add_input_queue - add a record to the input queue
**
** Description:
**	This routines adds a record the the input queue table using the given
**	update information
**
** Inputs:
**	dcb		database control block
**	base_rcb	base table RCB
**	trans_type	type of update to capture
**	shad_head	queue of qualifu]ying shadow records
**	int_date	current date and time in internal format
**
** Outputs:
**	err_code
**
** Returns:
**	DB_STATUS
**
** Side Effects:
**	adds records to dd_input_queue taking appropriate locks.
**
** History:
**	20-may-96 (stephenb)
**	    Initial creation as part of DBMS replication (phase 1)
**	14-aug-96 (stephenb)
**	    Adding code for priority based collision resolution
**	21-aug-96 (stephenb)
**	    Add code for cdds lookup table checks.
*/
static DB_STATUS
add_input_queue(
	DMP_DCB			*dcb,
	DMP_RCB			*base_rcb,
	i2			trans_type,
	DMP_SHAD_REC_QUE	*shad_head,
	char			*int_date,
	i2			dd_priority,
	i2			cdds_no,
	DB_ERROR		*dberr)

{
    char		*input_q_rec;
    DMP_RCB		*input_q_rcb = base_rcb->rcb_xcb_ptr->xcb_rep_input_q;
    STATUS		cl_stat;
    i4			size = 0;
    DB_STATUS		status;
    i2			dummy_i2 = 0;
    i4			dummy_i4 = 0;
    char		nulbyte = 1;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** consistency check
    */
    if (input_q_rcb == NULL)
    {
	uleFormat(dberr, E_DM955E_REP_NO_INPUT_QUEUE, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    if (shad_head == NULL && trans_type != DM2REP_INSERT)
	return(E_DB_ERROR);

    input_q_rec = MEreqmem(0, input_q_rcb->rcb_tcb_ptr->tcb_rel.relwid, TRUE,
	&cl_stat);
    if (input_q_rec == NULL || cl_stat != OK)
    {
	uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    /*
    ** build the record
    */
    /* database_no */
    MEcopy((char *)&dcb->dcb_rep_db_no, sizeof(dcb->dcb_rep_db_no), 
	input_q_rec);
    size += sizeof(dcb->dcb_rep_db_no);
    /* transaction_id */
    MEcopy((char *)&base_rcb->rcb_xcb_ptr->xcb_tran_id.db_low_tran,
	sizeof(base_rcb->rcb_xcb_ptr->xcb_tran_id.db_low_tran),
	input_q_rec + size);
    size += sizeof(base_rcb->rcb_xcb_ptr->xcb_tran_id.db_low_tran);
    /* sequence_no */
    MEcopy((char *)&base_rcb->rcb_xcb_ptr->xcb_rep_seq,
	sizeof(base_rcb->rcb_xcb_ptr->xcb_rep_seq), input_q_rec + size);
    size += sizeof(base_rcb->rcb_xcb_ptr->xcb_rep_seq);
    /* bump sequence number */
    base_rcb->rcb_xcb_ptr->xcb_rep_seq += 2;
    /* trans_type */
    MEcopy((char *)&trans_type, sizeof(trans_type), input_q_rec + size);
    size += sizeof(trans_type);
    /* table_no */
    MEcopy((char *)&base_rcb->rcb_tcb_ptr->tcb_rep_info->dd_reg_tables.tabno,
	sizeof(base_rcb->rcb_tcb_ptr->tcb_rep_info->dd_reg_tables.tabno),
	input_q_rec + size);
    size += sizeof(base_rcb->rcb_tcb_ptr->tcb_rep_info->dd_reg_tables.tabno);
    if (trans_type == DM2REP_INSERT) /* no old data for inserts */
    {
        /* old_sourcedb */
    	MEcopy((char *)&dummy_i2, sizeof(dummy_i2), input_q_rec + size);
    	size += sizeof(dummy_i2);
        /* old_transaction_id */
    	MEcopy((char *)&dummy_i4, sizeof(dummy_i4), input_q_rec + size);
        size += sizeof(dummy_i4);
        /* old_sequence_no */
    	MEcopy((char *)&dummy_i4, sizeof(dummy_i4), input_q_rec + size);
        size += sizeof(dummy_i4);
    }
    else
    {
        /* old_sourcedb */
        MEcopy((char *)&shad_head->shad_info.database_no, 
	    sizeof(shad_head->shad_info.database_no), input_q_rec + size);
        size += sizeof(shad_head->shad_info.database_no);
        /* old_transaction_id */
        MEcopy((char *)&shad_head->shad_info.transaction_id,
	    sizeof(shad_head->shad_info.transaction_id), 
	    input_q_rec + size);
        size += sizeof(shad_head->shad_info.transaction_id);
        /* old_sequence_no */
        MEcopy((char *)&shad_head->shad_info.sequence_no,
	    sizeof(shad_head->shad_info.sequence_no), input_q_rec + size);
        size += sizeof(shad_head->shad_info.sequence_no);
    }
    /* trans_time (not realy necessary since the field is null) */
    MEcopy(int_date, 12, input_q_rec + size);
    size += 12;
    /* set null byte to indicate local update */
    MEcopy(&nulbyte, 1, input_q_rec + size);
    size += 1;
    /* cdds_no */
    if (cdds_no)
	MEcopy((char *)&cdds_no, sizeof(cdds_no), input_q_rec + size);
    else
        MEcopy(
	    (char *)&base_rcb->rcb_tcb_ptr->tcb_rep_info->dd_reg_tables.cdds_no,
	    sizeof(base_rcb->rcb_tcb_ptr->tcb_rep_info->dd_reg_tables.cdds_no),
	    input_q_rec + size);
    size += sizeof(base_rcb->rcb_tcb_ptr->tcb_rep_info->dd_reg_tables.cdds_no);
    /* dd_priority */
    MEcopy((char *)&dd_priority, sizeof(dd_priority), input_q_rec + size);
    size += sizeof(dd_priority);
    /*
    ** consistency check
    */
    if (size != input_q_rcb->rcb_tcb_ptr->tcb_rel.relwid)
    {
	uleFormat(dberr, E_DM955F_REP_BAD_INPUT_QUEUE, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    /*
    ** now write the record and clean up
    */
    status = dm2r_put(input_q_rcb, (i4)0, input_q_rec, dberr);


    MEfree(input_q_rec);

    return(status);
}

/*
** Name: dm2rep_qman - replicator input/distribution queue management
**
** Description:
**	This routine manages the replicator input and distribution
**	queues (currently both database tables, dd_input_queue and
**	dd_distribution_queue). The routine reads records from the
**	input queue for a given transaction and then uses dd_paths and
**	dd_db_cdds records to construct distribution queue records and
**	insert them into the distribution queue. The routine will be
**	called from the ditributiuon queue manager system thread under
**	normal operation, but may be called before removing the database
**	from the server in a last ditch attempt to clear outstanding
**	replications.
**
**	It is assumed that the database has already been opened.
**
** Inputs:
**	dcb		database control block
**	tx_id		low part of the transaction id
**	trans_time	time of original transaction commit
**
** Outputs:
**	err_code	error condition
**
** Returns:
**	DB_STATUS
**
** Side Effects:
**	adds rows to dd_distribution_queue and deletes from dd_input_queue
**	taking appropriate locks.
**
** History:
**	28-may-96 (stephenb)
**	    Initial creation for DBMS replication, phase 1
**	5-aug-96 (stephenb)
**	    Add code for restart recovery, we will check all input queue 
**	    entries in this case.
**	16-sep-1996 (canor01)
**	    Add missing asterisk on err_code reference.
**	9-may-97 (stephenb)
**	    If there are no paths to replicate to, then print a warning in
**	    the error log, and save the input queue record. Note that we will
**	    remove the transaction queue entry, so the record will only be 
**	    distributed during restart/re-open recovery.
**	20-jun-97 (stephenb)
**	    alter path checking code, add call to check_paths which will
**	    record no-replicating CDDSs in the DCB for future usage.
**	21-jun-97 (stephenb)
**	    do not error out when dm2r_position on the input queue returns
**	    no records (DM0055), this is a valid return.
**	29-oct-97 (stephenb)
**	    add code to deal with deadlocks, this is a non-fatal error and 
**	    we should re-try.
**	26-nov-97 (stephenb)
**	    set input and distribution queue locking strategy using
**	    CBF parameters.
**	26-jan-98 (stephenb)
**	    add journal flag to begin/end transaction calls, otherwhise we
**	    don't journal the changes made by the distribution threads.
**	5-mar-98 (stephenb)
**	    Crash recorvery, choose the most recent update date for the
**	    dist queue records when we don't have the real commit date.
**	    Curently all records will receive a different date, the actual
**	    update date for that initial update, which confuses the 
**	    replication server.
**	26-mar-98 (stephenb)
**	    Some rare deadlock sitations slip through the net and cause
**	    error returns, fix this.
**	1-jun-98 (stephenb)
**	    add input_q_rcb parameter to function so that it can be called
**	    by a thread that already has the input queue table open. Test the
**	    passed parameter before attempting to open the input queue table.
**	29-sep-98 (stephenb)
**	    Re-set input_q_rcb when closing the table during deadlock
**	    rollback processing. The dm2t_close() does not re-set it which
**	    means we may not re-open the table in the re-try loop, this gives 
**	    us a stale RCB to work with which can cause bus errors, segv's 
**	    and LGwite() errors (because of a bad logging handle). Also
**	    check for the case where input_q_rcb is -1 and eliminate the
**	    dependency on the caller having allocated a pointer to place the
**	    address of the created RCB.
**	01-14-2000 consi01 bug 103342 problem INGREP 84
**	    Initialise local pointer variables to NULL. This ensures we do
**	    not call MEfree with an undefined value.
**	4-dec-98 (stephenb)
**	    trans_time is now an HRSYSTIME pointer, re-code date conversion
**	    to use adu_hrtimetodate(), also use AD_DATENTRNL instead of
**	    char[12] for dates, it looks neater.
**	01-May-2002 (bonro01)
**	    Fix memory overlay caused by using wrong length for char array
**	02-Dec-2002 (hanje04).
**	    Bug 103342 Problem INGREP 84
**	    Change 448638 was not fully crossed from oping20 in May-2001.
**	    Add missing changes by hand.
**	11-nov-2002 (gygro01) b109056/ingrep128
**	    Initialising DMP_RCB struct pointer with NULL to prevent
**	    segmentation violation in dm2t_close as they might be
**	    not initialised in a table deadlock situation.
**      20-sept-2004 (huazh01)
**          changed prototype for dmxe_commit(). 
**          INGSRV2643, b111502.
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**	21-jul-2006 (kibro01) bug 114168
**	    Add parameter to decide whether deadlocks should result in
**	    stopping our attempt to perform distribution.  If a deadlock
**	    occurs and this is a hijacked call, it means there must be another
**	    process attempting to perform distribution.  In that case the
**	    replication can be done by them instead (if this is a hijacked
**	    call) so that the user can get into the database in a reasonable
**	    timeframe.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502, doesn't
**	    apply to r3.
**	16-Aug-2007 (kibro01) b118626
**	    Add call to LG_status_is_abort to determine if the transaction
**	    has entered the force-abort state, so we have to abort it.
**	20-Aug-2007 (wanfr01)
**	    Bug 118986
**	    Table rcbs need to be NULLed after closing them, or you risk a
**	    double close.
**	10-Apr-2008 (kibro01) b120237
**	    Don't clean up (ready for a retry) when this is a hijacked dbopen 
**	    since we're not going to retry.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise adf_cb since we use the value inside it in adc_compare.
**	22-May-2008 (jonj)
**	    After dmxe_abort-ing the transaction, note that we no longer
**	    have one so we don't try to commit/abort again and get unnerving
**	    E_DM9017_BAD_LOG_SHOW errors.
*/
DB_STATUS
dm2rep_qman(
	DMP_DCB		*dcb,
	i4		tx_id,
	HRSYSTIME	*trans_time,
	DMP_RCB		*in_q_rcb,
	DB_ERROR	*dberr,
	bool		open_db)
{
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RCB		*dd_paths_rcb=(DMP_RCB *)NULL;
    DMP_RCB		*dd_db_cdds_rcb=(DMP_RCB *)NULL;
    DMP_RCB		*dist_q_rcb=(DMP_RCB *)NULL;
    DM2R_KEY_DESC       key;
    DM2R_KEY_DESC	dd_paths_key[3];
    DM2R_KEY_DESC	dd_db_cdds_key[3];
    DM_TID              tid;
    DB_TRAN_ID		tran_id;
    i4		log_id;
    i4		lock_id;
    char		*dd_paths_rec = NULL;
    char		*input_q_rec = NULL;
    char		*dd_db_cdds_rec = NULL;
    char		*dist_q_rec = NULL;
    STATUS		cl_stat;
    i4			size = 0;
    i4			tries;
    i2			priority = 0;
    DB_STATUS		status;
    DB_STATUS		local_stat;
    bool		tx_started;
    ADF_CB              adf_scb;
    DB_DATA_VALUE	last_date;
    DB_DATA_VALUE	curr_date;
    AD_DATENTRNL	int_date;
    i4		local_err;
    char		nullbyte;
    i4			i;
    bool		cdds_replicating = TRUE;
    i4		rep_iq_lock;
    i4		rep_dq_lock;
    i4		rep_maxlocks;
    RECOVERY_TXQ	*recovery_txq = 0;
    RECOVERY_TIDS	*recovery_tids = 0;
    PTR			*txq_next = 0;
    PTR			*tids_next = 0;
    i4			diff;
    PTR			rqstart = 0;
    PTR			rtstart = 0;
    bool		no_prev_entry = TRUE;
    i4			tidq_curr = 0;
    bool		mdwarn = FALSE;
    bool		iq_passed = FALSE;
    DMP_RCB		*input_q_rcb=(DMP_RCB *)NULL;
    i4                  dmxe_flags;
    bool		force_abort = FALSE;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    /*
    ** consistency check
    */
    if (dcb->rep_input_q.db_tab_base == 0 || dcb->rep_dd_paths.db_tab_base == 0)
	return (E_DB_ERROR);

    /*
    ** initialize data values 
    */
    last_date.db_datatype	= DB_DTE_TYPE;
    last_date.db_prec		= 0;
    last_date.db_length		= 12;
    last_date.db_collID		= -1;
    curr_date.db_datatype	= DB_DTE_TYPE;
    curr_date.db_prec		= 0;
    curr_date.db_length		= 12;
    curr_date.db_collID		= -1;
    /*
    ** setup locking strategy
    */
    if (dmf_svcb->svcb_rep_iqlock == DMC_C_ROW)
	rep_iq_lock = DM2T_RIX;
    else if (dmf_svcb->svcb_rep_iqlock == DMC_C_TABLE)
	rep_iq_lock = DM2T_X;
    else
	rep_iq_lock = DM2T_IX;

    if (dmf_svcb->svcb_rep_dqlock == DMC_C_ROW)
	rep_dq_lock = DM2T_RIX;
    else if (dmf_svcb->svcb_rep_dqlock == DMC_C_TABLE)
	rep_dq_lock = DM2T_X;
    else
	rep_dq_lock = DM2T_IX;

    /* allow a minimum maxlocks value of 50 ... */
    if (dmf_svcb->svcb_rep_dtmaxlock > 50)
	rep_maxlocks = dmf_svcb->svcb_rep_dtmaxlock;
    /* ...but default to 100 */
    else if (dmf_svcb->svcb_lk_maxlocks > 100)
	rep_maxlocks = dmf_svcb->svcb_lk_maxlocks;
    else 
	rep_maxlocks = 100;

    dmxe_flags = ((dcb->dcb_status & DCB_S_JOURNAL) ?
                       (DMXE_INTERNAL | DMXE_JOURNAL) : (DMXE_INTERNAL));

    for (tries = 1;;tries++) /* error handling */
    {
        /* start a TX */
        status = dmxe_begin(DMXE_WRITE, dmxe_flags, dcb,
	    dcb->dcb_log_id, &dcb->dcb_db_owner, (i4)0, 
	    &log_id, &tran_id, &lock_id, (DB_DIS_TRAN_ID *)NULL, dberr);
    	if (status != E_DB_OK)
	    break;
    	tx_started = TRUE;
        /* open the input queue, if we weren't passed it */
	if (in_q_rcb == NULL || in_q_rcb == (DMP_RCB *)-1)
	{
    	    status = dm2t_open(dcb, &dcb->rep_input_q, rep_iq_lock,
	        DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, rep_maxlocks,
	        (i4)0, log_id, lock_id, (i4)0, (i4)0,
	        (i4)0, &tran_id, &timestamp, &input_q_rcb, (DML_SCB *)NULL, 
	        dberr);
    	    if (status != E_DB_OK)
	        break;
	}
	else
	{
	    input_q_rcb = in_q_rcb;
	    iq_passed = TRUE;
	}
    	/* open dd_paths */
        status = dm2t_open(dcb, &dcb->rep_dd_paths, DM2T_IS,
	    DM2T_UDIRECT, DM2T_A_READ, (i4)0, rep_maxlocks,
	    (i4)0, log_id, lock_id, (i4)0, (i4)0,
	    (i4)0, &tran_id, &timestamp, &dd_paths_rcb, (DML_SCB *)NULL,
	    dberr);
        if (status != E_DB_OK)
	    break;
	/* open dd_db_cdds */
        status = dm2t_open(dcb, &dcb->rep_dd_db_cdds, DM2T_IS,
	    DM2T_UDIRECT, DM2T_A_READ, (i4)0, rep_maxlocks,
	    (i4)0, log_id, lock_id, (i4)0, (i4)0,
	    (i4)0, &tran_id, &timestamp, &dd_db_cdds_rcb, (DML_SCB *)NULL,
	    dberr);
        if (status != E_DB_OK)
	    break;
	/* open the distribution queue */
        status = dm2t_open(dcb, &dcb->rep_dist_q, rep_dq_lock,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, rep_maxlocks,
	    (i4)0, log_id, lock_id, (i4)0, (i4)0,
	    (i4)0, &tran_id, &timestamp, &dist_q_rcb, (DML_SCB *)NULL,
	    dberr);
        if (status != E_DB_OK)
	    break;
	/*
	** get memory for the records
	*/
        input_q_rec = MEreqmem(0, 
	    input_q_rcb->rcb_tcb_ptr->tcb_rel.relwid, TRUE, &cl_stat);
        if (input_q_rec == NULL || cl_stat != OK)
        {
	    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    status = E_DB_ERROR;
	    break;
        }
        dd_paths_rec = MEreqmem(0, 
	    dd_paths_rcb->rcb_tcb_ptr->tcb_rel.relwid, TRUE, &cl_stat);
        if (dd_paths_rec == NULL || cl_stat != OK)
        {
	    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    status = E_DB_ERROR;
	    break;
        }
        dd_db_cdds_rec = MEreqmem(0, 
	    dd_db_cdds_rcb->rcb_tcb_ptr->tcb_rel.relwid, TRUE, &cl_stat);
        if (dd_db_cdds_rec == NULL || cl_stat != OK)
        {
	    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    status = E_DB_ERROR;
	    break;
        }
	/* check all paths are present and correct */
	if (dcb->dcb_bad_cdds == NULL) /* not checked before */
	{
	    status = check_paths(dcb, dd_db_cdds_rcb, dd_paths_rcb, 
		dd_db_cdds_rec, dd_paths_rec, dberr);
	    if (status != E_DB_OK)
		break;
	    if (dcb->dcb_bad_cdds == NULL)
		dcb->dcb_bad_cdds = (i2 *)-1;
	}
        dist_q_rec = MEreqmem(0, 
	    dist_q_rcb->rcb_tcb_ptr->tcb_rel.relwid, TRUE, &cl_stat);
        if (dist_q_rec == NULL || cl_stat != OK)
        {
	    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    status = E_DB_ERROR;
	    break;
        }
	/*
	** get memory for restart recovery, we'll grab enough for 100
	** transactions and 500 distribution records (it's only about 5K),
	** and take more later if we need it.
	*/
	if (trans_time == (HRSYSTIME *)-1)
	{
	    rqstart = MEreqmem(0, (TXQ_SIZE * sizeof(RECOVERY_TXQ)) + 
		(TIDQ_SIZE * sizeof(RECOVERY_TIDS)) + (2 * sizeof(PTR)), 
		TRUE, &cl_stat);
            if (rqstart == NULL || cl_stat != OK)
            {
		SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    	status = E_DB_ERROR;
	    	break;
            }
	    recovery_txq = (RECOVERY_TXQ *)rqstart;
	    txq_next = (PTR *)(recovery_txq + TXQ_SIZE);
	    rtstart = (char *)txq_next + sizeof(PTR);
	    recovery_tids = (RECOVERY_TIDS *)rtstart;
	    tids_next = (PTR *)(recovery_tids + TIDQ_SIZE);
	}
        /* 
	** position the input queue, retrieve all records for restart recovery
	*/
	if (tx_id == -1)
	{
            status = dm2r_position(input_q_rcb, DM2R_ALL, (DM2R_KEY_DESC *)NULL, 
		(i4)0, &tid, dberr);
            if (status != E_DB_OK)
	        break;
	}
	else
	{
            key.attr_operator = DM2R_EQ;
            key.attr_number = 2; /* transaction_id field */
            key.attr_value = (char *)&tx_id;
            status = dm2r_position(input_q_rcb, DM2R_QUAL, &key, (i4)1,
			&tid, dberr);
	    if (status != E_DB_OK)
	    {
	        if (dberr->err_code == E_DM0055_NONEXT) /* out of records */
	        {
		    status = E_DB_OK;
		    break;
	        }
	        else
		    break;
             }
	}
        for (;;) /* each input queue record */
    	{
	    status = dm2r_get(input_q_rcb, &tid, DM2R_GETNEXT, 
	        input_q_rec, dberr);
	    if (status != E_DB_OK)
	    {
	        if (dberr->err_code == E_DM0055_NONEXT) /* out of records */
	        {
		    status = E_DB_OK;
		    break;
	        }
	        else
		    break;
             }
	     /* check that this CDDS is replicating */
	     cdds_replicating = TRUE;
	     if (dcb->dcb_bad_cdds != NULL && dcb->dcb_bad_cdds != (i2 *)-1)
	     {
	    	i2		local_cdds;
		i2		*check_cdds = dcb->dcb_bad_cdds;

	        MEcopy(input_q_rec + 
		    input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[10].offset, 2, 
		    &local_cdds);
	     	for (i = 0; check_cdds[i] != -1; i++)
		{
		    if (local_cdds == check_cdds[i]) /* not replicating */
		    {
			cdds_replicating = FALSE;
			break;
		    }
		    if (i == 9)
		    {
			check_cdds = (i2 *)(dcb->dcb_bad_cdds + 10*sizeof(i2));
			if (check_cdds == NULL)
			    break;
			i = 0;
		    }
		}
		if (cdds_replicating == FALSE)
		    continue;
	     }
	     /*
	     ** check for the latest update time for recovery
	     */
	     if (trans_time == (HRSYSTIME *)-1)
	     {
	        recovery_txq = (RECOVERY_TXQ *)rqstart;
		txq_next = (PTR *)(recovery_txq + TXQ_SIZE);
		no_prev_entry = TRUE;
		for (i = 0; recovery_txq[i].tx_id != 0; i++)
		{
		    if (MEcmp(&recovery_txq[i].tx_id, input_q_rec + 
			input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].offset, 
			sizeof(recovery_txq[i].tx_id))
			== 0)
		    {
			/* 
			** have come across this TX before, check date 
			** and update if necessary
			*/
			no_prev_entry = FALSE;
    			last_date.db_data = recovery_txq[i].last_date;
			curr_date.db_data = input_q_rec + 
			    input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[9].offset;
			status = adc_compare(&adf_scb, &last_date, &curr_date,
			    &diff);
			if (status != E_DB_OK)
			{
			    char	dbname[DB_MAXNAME + 1];

			    MEcopy(dcb->dcb_name.db_db_name, DB_MAXNAME,
				dbname);
			    dbname[DB_MAXNAME] = 0;
			    uleFormat(dberr, E_DM957B_REP_BAD_DATE, (CL_ERR_DESC *)NULL, 
				ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
				(i4 *)NULL, err_code, (i4)2, 
				sizeof(recovery_txq[i].tx_id), 
				recovery_txq[i].tx_id, STtrmwhite(dbname), 
				dbname);
			    break;
			}
			if (diff < 0)
			    /* update with new high date */
			    MEcopy(curr_date.db_data, 12, last_date.db_data);
			break;
		    }
		    else if (i == TXQ_SIZE - 1 && *txq_next == NULL) 
			/* end of list */
			break;
		    else if (i == TXQ_SIZE - 1) /* reached end of this block */
		    {
			recovery_txq = (RECOVERY_TXQ *)(*txq_next);
			txq_next = (PTR *)(recovery_txq + TXQ_SIZE);
			i = -1; /* will be incremented to 0 at end of loop */
		    }
		}
		if (status != E_DB_OK)
		    break;
		if (no_prev_entry)
		{
		    /* a new TX, add it to the list */
		    if (i == TXQ_SIZE - 1 && *txq_next == NULL) 
		    {
			/* need more memory */
	    		*txq_next = MEreqmem(0, TXQ_SIZE * sizeof(RECOVERY_TXQ)
			    + sizeof(PTR), TRUE, &cl_stat);
            		if (*txq_next == NULL || cl_stat != OK)
            		{
			    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    		    status = E_DB_ERROR;
	    		    break;
            		}
			recovery_txq = (RECOVERY_TXQ *)(*txq_next);
			txq_next = (PTR *)(recovery_txq + TXQ_SIZE);
			i = 0;
		    }
		    MEcopy(input_q_rec +
			input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].offset,
			sizeof(recovery_txq[i].tx_id), &recovery_txq[i].tx_id);
		    MEcopy(input_q_rec +
			input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[9].offset, 12,
			recovery_txq[i].last_date);
		}
	     }

	     /* position dd_paths */
	     dd_paths_key[0].attr_operator = DM2R_EQ;
	     dd_paths_key[0].attr_number = 1; /* cdds_no */
	     dd_paths_key[0].attr_value = input_q_rec + 
		input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[10].offset;
	     dd_paths_key[1].attr_operator = DM2R_EQ;
	     dd_paths_key[1].attr_number = 2; /* localdb */
	     dd_paths_key[1].attr_value = (char *)&dcb->dcb_rep_db_no;
	     dd_paths_key[2].attr_operator = DM2R_EQ;
	     dd_paths_key[2].attr_number = 3; /* sourcedb */
	     dd_paths_key[2].attr_value = input_q_rec;
	     status = dm2r_position(dd_paths_rcb, DM2R_QUAL, dd_paths_key,
		(i4)3, &tid, dberr);
	     if (status != E_DB_OK)
		break;
	     /*
	     ** get records 
	     */
	     for (i = 0;;i++) /* each dd_paths record */
	     {
		status = dm2r_get(dd_paths_rcb, &tid, DM2R_GETNEXT,
		    dd_paths_rec, dberr);
	        if (status != E_DB_OK)
	        {
	            if (dberr->err_code == E_DM0055_NONEXT) /* out of records */
	            {
		        status = E_DB_OK;
		        break;
	            }
	            else
		        break;
                 }
		 /*
		 ** position dd_db_cdds
		 */
		 dd_db_cdds_key[0].attr_operator = DM2R_EQ;
		 dd_db_cdds_key[0].attr_number = 1; /* cdds_no */
		 dd_db_cdds_key[0].attr_value = dd_paths_rec;
		 dd_db_cdds_key[1].attr_operator = DM2R_EQ;
		 dd_db_cdds_key[1].attr_number = 2; /* database_no */
		 dd_db_cdds_key[1].attr_value = dd_paths_rec + 
		     /* offset of targetdb */
		     dd_paths_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].offset;
		 status = dm2r_position(dd_db_cdds_rcb, DM2R_QUAL, 
		     dd_db_cdds_key, (i4)2,
		     &tid, dberr);
		 if (status != E_DB_OK)
		 {
	            if (dberr->err_code == E_DM0055_NONEXT) /* no records*/
	            {
			uleFormat(NULL, W_DM9562_REP_NO_DD_DB_CDDS, (CL_ERR_DESC *)NULL, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)3, 0, 
			    (i2 *)dd_db_cdds_key[1].attr_value, 0, 
			    (i2 *)dd_db_cdds_key[0].attr_value, 0, &tx_id);
		        status = E_DB_WARN;
		        break;
	            }
		    else
		     	break;
		 }
		 /* get the record (there should be exactly one) */
		 status = dm2r_get(dd_db_cdds_rcb, &tid, DM2R_GETNEXT,
		     dd_db_cdds_rec, dberr);
		 if (status != E_DB_OK)
		 {
	            if (dberr->err_code == E_DM0055_NONEXT) /* no records*/
	            {
			uleFormat(NULL, W_DM9562_REP_NO_DD_DB_CDDS, (CL_ERR_DESC *)NULL, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)3, 0, 
			    (i2 *)dd_db_cdds_key[1].attr_value, 0, 
			    (i2 *)dd_db_cdds_key[0].attr_value, 0, &tx_id);
		        status = E_DB_WARN;
		        break;
	            }
		    else
		     	break;
		 }
		 /*
		 ** now we can build the distribution queue record
		 */
		 /* targetdb */
		 MEcopy(dd_paths_rec + 
		     dd_paths_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].offset,
		     dd_paths_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].length,
		     dist_q_rec);
		 size = dd_paths_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].length;
		 /* sourcedb */
		 MEcopy(input_q_rec, 
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[1].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[1].length;
		 /* transaction_id */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].length;
		 /* sequence_no */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[3].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[3].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[3].length;
		 /* trans_type */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].length;
		 /* table_no */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[5].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[5].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[5].length;
		 /* old_sourcedb */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[6].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[6].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[6].length;
		 /* old_transaction_id */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[7].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[7].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[7].length;
		 /* old_sequence_no */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[8].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[8].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[8].length;
		 /* trans_time */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[9].offset + 12, 1,
		     &nullbyte);
		 /* 
		 ** we have no trans time for re-start recovery so we'll just
		 ** use the trans time placed into the input queue record
		 ** for now, well replace it later at the end of the recovery
		 ** cycle.
		 */
		 if (nullbyte && trans_time != (HRSYSTIME *)-1)
		 {
		     adf_scb.adf_dfmt = DB_DMY_DFMT;
		     adf_scb.adf_errcb.ad_ebuflen = 0;
		     adf_scb.adf_exmathopt = ADX_IGN_MATHEX;
		     cl_stat = TMtz_init(&(adf_scb.adf_tzcb));
    		     if (cl_stat != OK)
    		     {
        	        uleFormat(dberr, E_DM955D_REP_BAD_DATE_CVT, (CL_ERR_DESC *)NULL, 
			    ULE_LOG,
            		   (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            		   err_code, (i4)0);
		        break;
		     }
		     status = adu_hrtimetodate(trans_time, &int_date, err_code);
    		     if (status != E_DB_OK)
    		     {
			SETDBERR(dberr, 0, *err_code);
        	        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
            		   (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            		   err_code, (i4)0);
        	        uleFormat(dberr, E_DM955D_REP_BAD_DATE_CVT, (CL_ERR_DESC *)NULL, 
			    ULE_LOG,
            		   (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            		   err_code, (i4)0);
		        break;
		     }
		     MEcopy((char *)&int_date, sizeof(AD_DATENTRNL), 
			    dist_q_rec + size);
		 }
		 else /* use input queue date */
		     MEcopy(input_q_rec +
		         input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[9].offset, 12,
		         dist_q_rec + size);

		 size += 12; /* size of internal date */
		 /* cdds_no */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[10].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[10].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[10].length;
		 /* dd_priority */
		 MEcopy(input_q_rec +
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[11].offset,
		     input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[11].length,
		     dist_q_rec + size);
		 size += input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[11].length;
		 /*
		 ** consistency check
		 */
		 if (size != dist_q_rcb->rcb_tcb_ptr->tcb_rel.relwid)
		 {
		     status = E_DB_ERROR;
	    	     uleFormat(dberr, E_DM9560_REP_BAD_DIST_QUEUE, (CL_ERR_DESC *)NULL, 
			 ULE_LOG,
            		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            		err_code, (i4)0);
		     break;
		 }
		 /*
		 ** print warning for recovery records
		 */
		 if (trans_time == (HRSYSTIME *)-1 && !mdwarn)
		 {
		     char	dbname[DB_MAXNAME + 1];

		     MEcopy(dcb->dcb_name.db_db_name, DB_MAXNAME, dbname);
		     dbname[DB_MAXNAME] = 0;
	    	     uleFormat(dberr, W_DM956A_REP_MANUAL_DIST, (CL_ERR_DESC *)NULL, 
			 ULE_LOG,
            		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            		err_code, (i4)1, STtrmwhite(dbname), dbname);
		     mdwarn = TRUE;
		 }
		 /*
		 ** add record
		 */
		 status = dm2r_put(dist_q_rcb, (i4)0, dist_q_rec, dberr);
		 if (status != E_DB_OK)
		     break;
		/*
		** for recovery, we'll store the TID so that we can go back and
		** update the record with the correct time stamp later
		*/
		if (trans_time == (HRSYSTIME *)-1)
		{
		    if (tidq_curr == TIDQ_SIZE)
		    {
			/* out of space, grab some more */
	    		*tids_next = MEreqmem(0, TIDQ_SIZE * 
			    sizeof(RECOVERY_TIDS) + sizeof(PTR), 
			    TRUE, &cl_stat);
            		if (*tids_next == NULL || cl_stat != OK)
            		{
			    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    		    status = E_DB_ERROR;
	    		    break;
            		}
			recovery_tids = (RECOVERY_TIDS *)(*tids_next);
			tids_next = (PTR *)(recovery_tids + TIDQ_SIZE);
			tidq_curr = 0;
		    }
		    MEcopy(input_q_rec +
			input_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].offset,
			sizeof(recovery_tids[tidq_curr].tx_id), 
			&recovery_tids[tidq_curr].tx_id);
		    recovery_tids[tidq_curr].tid.tid_i4 = 
			dist_q_rcb->rcb_currenttid.tid_i4;
		    tidq_curr++;
		}
	    } /* each dd_paths record */
	    if (status != E_DB_OK)
		break;
	    /*
	    ** delete input queue record
	    */
	    status = dm2r_delete(input_q_rcb, &tid, DM2R_BYPOSITION, dberr);
	    if (status != E_DB_OK)
		break;
	    if (LG_status_is_abort(&log_id))
	    {
		force_abort = TRUE;
		status = E_DB_ERROR;
		break;
	    }
	} /* each input queue record */
	/*
	** for recovery, go back and update dd_distrib_queue with 
	** correct time
	*/
	if (status == E_DB_OK && trans_time == (HRSYSTIME *)-1)
	{
	    recovery_tids = (RECOVERY_TIDS *)rtstart;
	    tids_next = (PTR *)(recovery_tids + TIDQ_SIZE);
	    for (tidq_curr = 0; recovery_tids[tidq_curr].tx_id != 0; 
		tidq_curr++)
	    {
		if (tidq_curr == TIDQ_SIZE) /* end of this block */
		{
		    if (*tids_next == 0) /* no more left */
			break;
		    else
		    {
			recovery_tids = (RECOVERY_TIDS *)(*tids_next);
			tids_next = (PTR *)(recovery_tids + TIDQ_SIZE);
			tidq_curr = 0;
		    }
		}
		/* 
		** get the record back 
		*/
		status = dm2r_get(dist_q_rcb, &recovery_tids[tidq_curr].tid,
		    DM2R_BYTID, dist_q_rec, dberr);
		if (status != E_DB_OK)
		{
		    char	dbname[DB_MAXNAME + 1];

		    MEcopy(dcb->dcb_name.db_db_name, DB_MAXNAME, dbname);
		    dbname[DB_MAXNAME] = 0;
		    uleFormat(NULL, E_DM957C_REP_GET_ERROR, (CL_ERR_DESC *)NULL, 
			ULE_LOG, 
			(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			err_code, (i4)1, STtrmwhite(dbname), dbname);
		    break;
		}
		/* 
		** update the time stamp 
		*/
		no_prev_entry = TRUE;
		recovery_txq = (RECOVERY_TXQ *)rqstart;
		txq_next = (PTR *)(recovery_txq + TXQ_SIZE);
		for (i = 0; recovery_txq[i].tx_id != 0; i++)
		{
		    if (i == TXQ_SIZE)
		    {
			if (*txq_next == 0) /* no more left */
			    break;
			else
			{
			    recovery_txq = (RECOVERY_TXQ *)(*txq_next);
			    txq_next = (PTR *)(recovery_txq + TXQ_SIZE);
			    i = 0;
			}
		    }
		    if (recovery_txq[i].tx_id == recovery_tids[tidq_curr].tx_id)
		    {
			no_prev_entry = FALSE;
			break;
		    }
		}
		if (no_prev_entry)
		{
		    char	dbname[DB_MAXNAME + 1];

		    MEcopy(dcb->dcb_name.db_db_name, DB_MAXNAME,
			dbname);
		    dbname[DB_MAXNAME] = 0;
		    uleFormat(dberr, E_DM957D_REP_NO_DISTQ, (CL_ERR_DESC *)NULL, 
			ULE_LOG, 
			(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			err_code, (i4)2, 
			sizeof(recovery_tids[tidq_curr].tx_id), 
			recovery_tids[tidq_curr].tx_id, STtrmwhite(dbname), 
			dbname);
		    status = E_DB_ERROR;
		    break;
		}
		MEcopy(recovery_txq[i].last_date, 12, dist_q_rec + 
		    dist_q_rcb->rcb_tcb_ptr->tcb_atts_ptr[10].offset);
		/* 
		** replace record 
		*/
		status = dm2r_replace(dist_q_rcb, &recovery_tids[tidq_curr].tid,
		    DM2R_BYTID, dist_q_rec, (char *)NULL, dberr);
		if (status != E_DB_OK)
		{
		    char	dbname[DB_MAXNAME + 1];

		    MEcopy(dcb->dcb_name.db_db_name, DB_MAXNAME,
			dbname);
		    dbname[DB_MAXNAME] = 0;
		    uleFormat(NULL, E_DM957E_REP_DISTQ_UPDATE, (CL_ERR_DESC *)NULL, 
			ULE_LOG, 
			(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			err_code, (i4)2, 
			sizeof(recovery_tids[tidq_curr].tx_id), 
			recovery_tids[tidq_curr].tx_id, STtrmwhite(dbname), 
			dbname);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    if (status != E_DB_OK)
		break;
	}
	/*
	** check for deadlock
	*/
	if (status != E_DB_OK && (dberr->err_code == E_DM9042_PAGE_DEADLOCK ||
	    dberr->err_code == E_DM9044_ESCALATE_DEADLOCK ||
	    dberr->err_code == E_DM9045_TABLE_DEADLOCK ||
	    dberr->err_code == E_DM905A_ROW_DEADLOCK ||
	    dberr->err_code == E_DM0042_DEADLOCK))
	{
	    /*
	    ** If this is a hijacked call, then even with deadlock don't try
	    ** again - there's another process which will finish this
	    ** replication off.  Also, don't clean up the transaction - this
	    ** will be done outside the main loop and shouldn't be done twice.
	    ** (kibro01) b120237
	    */
	    if (tries < 3 && !open_db)
	    {
	        /* clean up, rollbcak, and try again */
    		if (input_q_rcb && !iq_passed)
		{
		    status = dm2t_close(input_q_rcb, (i4)0, &local_dberr);
	    	    if (status != E_DB_OK)
	            {
	        	uleFormat(dberr, E_DM9577_REP_DEADLOCK_ROLLBACK, 
			    (CL_ERR_DESC *)NULL, ULE_LOG,
            	    	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)0);
			break;
	    	    }
		    input_q_rcb = NULL;
		}
    		if(dd_paths_rcb)
		{
		    status = dm2t_close(dd_paths_rcb, (i4)0, &local_dberr);
	    	    if (status != E_DB_OK)
	            {
	        	uleFormat(dberr, E_DM9577_REP_DEADLOCK_ROLLBACK, 
			    (CL_ERR_DESC *)NULL, ULE_LOG,
            	    	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)0);
			break;
	    	    }
		    dd_paths_rcb = NULL;
		}
    		if (dd_db_cdds_rcb)
		{
		    status = dm2t_close(dd_db_cdds_rcb, (i4)0, &local_dberr);
	    	    if (status != E_DB_OK)
	            {
	        	uleFormat(dberr, E_DM9577_REP_DEADLOCK_ROLLBACK, 
			    (CL_ERR_DESC *)NULL, ULE_LOG,
            	    	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)0);
			break;
	    	    }
		    dd_db_cdds_rcb = NULL;
		}
    		if (dist_q_rcb)
		{
		    status = dm2t_close(dist_q_rcb, (i4)0, &local_dberr);
	    	    if (status != E_DB_OK)
	            {
	        	uleFormat(dberr, E_DM9577_REP_DEADLOCK_ROLLBACK, 
			    (CL_ERR_DESC *)NULL, ULE_LOG,
            	    	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)0);
			break;
	    	    }
		    dist_q_rcb = NULL;
		}
    		if (dd_paths_rec)
		    MEfree(dd_paths_rec);
    		if (input_q_rec)
		    MEfree(input_q_rec);
    		if (dd_db_cdds_rec)
		    MEfree(dd_db_cdds_rec);
    		if(dist_q_rec)
		    MEfree(dist_q_rec);
	    	status = dmxe_abort(NULL, dcb, &tran_id, log_id, lock_id, 
	    	    dmxe_flags, (i4)0, NULL, dberr);
		/* Note we no longer have a transaction */
		tx_started = FALSE;
	    	if (status != E_DB_OK)
	    	{
		    uleFormat(dberr, E_DM9577_REP_DEADLOCK_ROLLBACK, 
			(CL_ERR_DESC *)NULL, ULE_LOG,
            	        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	        err_code, (i4)0);
		    break;
	        }

		continue;
	    }
	    else
	    {
	        uleFormat(dberr, W_DM9578_REP_QMAN_DEADLOCK, 
		    (CL_ERR_DESC *)NULL, ULE_LOG,
            	    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	    err_code, (i4)0);
		status = E_DB_WARN;
		break;
	    }
	}
	break;
    }
    /* check for deadlock */
    if (status > E_DB_WARN &&
	(dberr->err_code == E_DM9042_PAGE_DEADLOCK ||
	dberr->err_code == E_DM9044_ESCALATE_DEADLOCK ||
	dberr->err_code == E_DM9045_TABLE_DEADLOCK ||
	dberr->err_code == E_DM905A_ROW_DEADLOCK ||
	dberr->err_code == E_DM0042_DEADLOCK))
    {
	uleFormat(dberr, W_DM9578_REP_QMAN_DEADLOCK, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	status = E_DB_WARN;
    }
    /*
    ** cleanup
    */
    /* dd_input_queue */
    if (input_q_rcb && !iq_passed)
	local_stat = dm2t_close(input_q_rcb, (i4)0, &local_dberr);

    /* dd_paths */
    if(dd_paths_rcb)
	local_stat = dm2t_close(dd_paths_rcb, (i4)0, &local_dberr);

    /* dd_db_cdds */
    if (dd_db_cdds_rcb)
	local_stat = dm2t_close(dd_db_cdds_rcb, (i4)0, &local_dberr);

    /* dd_distrib_queue */
    if (dist_q_rcb)
	local_stat = dm2t_close(dist_q_rcb, (i4)0, &local_dberr);

    /* free up memory */
    if (dd_paths_rec)
	MEfree(dd_paths_rec);

    if (input_q_rec)
	MEfree(input_q_rec);

    if (dd_db_cdds_rec)
	MEfree(dd_db_cdds_rec);

    if(dist_q_rec)
	MEfree(dist_q_rec);

    if (rqstart)
	rtstart = *((PTR *)(rqstart + TXQ_SIZE * sizeof(RECOVERY_TXQ) + 
	    sizeof(PTR) + TIDQ_SIZE * sizeof(RECOVERY_TIDS)));
    while (rqstart)
    {
	recovery_txq = 
	    *((RECOVERY_TXQ **)(rqstart + TXQ_SIZE * sizeof(RECOVERY_TXQ)));
	MEfree(rqstart);
	rqstart = (char *)recovery_txq;
    }
    while (rtstart)
    {
	recovery_tids = 
	    *((RECOVERY_TIDS **)(rtstart + TIDQ_SIZE * sizeof(RECOVERY_TIDS)));
	MEfree(rtstart);
	rtstart = (char *)recovery_tids;
    }


    /* close transaction, if we have one */
    if ( tx_started )
    {
	if (status == E_DB_OK)
	    status = dmxe_commit(&tran_id, log_id, lock_id, dmxe_flags,
		&timestamp, dberr);
	else
	    local_stat = dmxe_abort(NULL, dcb, &tran_id, log_id, lock_id, 
		dmxe_flags, (i4)0, NULL, &local_dberr);
    }

    /* Just because a hijacked thread failed to complete the replication doesn't mean
    ** we can't go into the database.  Let them in anyway (kibro01) b118626
    */
    if (force_abort && open_db)
	status = E_DB_OK;

    return(status);
}

/*
** Name: check_priority - check lookup table for replication priority
**
** Description:
**	retrieves records from the priority lookup table and checks them
**	against the given record to see if it should be assigned a
**	replication priority, no match returns priority = 0.
**
** Inputs:
**	base_rcb	- base table RCB
**	record		- updated record
**
** Outputs:
**	None
**
** Returns:
**	priority
**
** History:
**	17-jun-96 (stephenb)
**	    First written for DBMS replication phase 1
**	14-aug-96 (stephenb)
**	    Minor fix to error checking
**	26-jun-97 (stephenb)
**	    If the field in the priority lookup table is a varchar, then
**	    only compare to the length indicated, the rest of the field may
**	    contain junk. Also tidy up error reporting.
*/
static DB_STATUS
check_priority(
	DMP_RCB		*base_rcb,
	char		*record,
	i2		*prio,
	DB_ERROR	*dberr)
{
    DMP_TCB		*base_tcb = base_rcb->rcb_tcb_ptr;
    DMP_RCB		*prio_rcb = base_rcb->rep_prio_rcb;
    DMP_TCB		*prio_tcb = base_rcb->rep_prio_rcb->rcb_tcb_ptr;
    STATUS		cl_stat;
    DM2R_KEY_DESC	key;
    DM_TID		tid;
    i4			matched_values;
    bool		no_match;
    i2			int_pri;
    char		*prio_rec = NULL;
    DB_STATUS		status;
    i4			i, j;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** get priority record
    */
    *prio = 0;
    prio_rec = MEreqmem(0, prio_tcb->tcb_rel.relwid, TRUE, &cl_stat);
    if (prio_rec == NULL || cl_stat != OK)
    {
	uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    for (;;) /* error checking */
    {
    	status = dm2r_position(prio_rcb, DM2R_ALL, &key, (i4)0,
	    (DM_TID *)NULL, dberr);
    	if (status != E_DB_OK)
	    break;
	for(;;) /* each priority record */
	{
	    matched_values = 0;
	    status = dm2r_get(prio_rcb, &tid, DM2R_GETNEXT, prio_rec, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT) /* out of records */
		{
		    status = E_DB_OK;
		    break;
		}
		else
		    break;
	    }
	    /*
	    ** check for matching fields in the base table
	    */
	    for (i = 1; i <= prio_tcb->tcb_rel.relatts; i++)
	    {
		no_match = TRUE;
		if (MEcmp(prio_tcb->tcb_atts_ptr[i].name.db_att_name, 
		      (base_rcb->rcb_xcb_ptr && 
		  	base_rcb->rcb_xcb_ptr->xcb_scb_ptr &&
		  	base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &&
		  	(*base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &
		  	CUI_ID_REG_U)) ? 
		      "DD_PRIORITY" : "dd_priority", 11) == 0)
		{
		    MEcopy(prio_rec + prio_tcb->tcb_atts_ptr[i].offset,
			prio_tcb->tcb_atts_ptr[i].length, (char *)&int_pri);
		    continue;
		}
		for (j = 1; j <= base_tcb->tcb_rel.relatts; j++)
		{
		    if (MEcmp(prio_tcb->tcb_atts_ptr[i].name.db_att_name,
			base_tcb->tcb_atts_ptr[j].name.db_att_name, 
			DB_MAXNAME) == 0)
		    {
			i2	base_size;

			if (prio_tcb->tcb_atts_ptr[i].length !=
			    base_tcb->tcb_atts_ptr[j].length)
			{
			    char	prio_fld[DB_MAXNAME +1];
			    char	prio_name[DB_MAXNAME +1];
			    char	base_name[DB_MAXNAME +1];

			    MEcopy(prio_tcb->tcb_atts_ptr[i].name.db_att_name,
				DB_MAXNAME, prio_fld);
			    MEcopy(prio_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, prio_name);
			    MEcopy(base_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, base_name);
			    prio_fld[DB_MAXNAME] = prio_name[DB_MAXNAME] = 
				base_name[DB_MAXNAME] = EOS;
			    uleFormat(dberr, E_DM9565_REP_BAD_PRIO_LOOKUP, 
				(CL_ERR_DESC *)NULL, ULE_LOG,
            		        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			        (i4 *)NULL, err_code, (i4)5, 
				STtrmwhite(prio_fld), prio_fld, 
			        STtrmwhite(prio_name), prio_name, (i4)0,
			        prio_tcb->tcb_atts_ptr[i].length, 
				STtrmwhite(base_name), base_name, (i4)0,
			        base_tcb->tcb_atts_ptr[j].length);
			    status = E_DB_ERROR;
			    break;
			}
			else if (base_tcb->tcb_atts_ptr[j].type !=
			    prio_tcb->tcb_atts_ptr[i].type)
			{
			    /* fields are not of the same type */
			    char	prio_fld[DB_MAXNAME +1];
			    char	prio_name[DB_MAXNAME +1];
			    char	base_name[DB_MAXNAME +1];

			    MEcopy(prio_tcb->tcb_atts_ptr[i].name.db_att_name,
				DB_MAXNAME, prio_fld);
			    MEcopy(prio_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, prio_name);
			    MEcopy(base_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, base_name);
			    prio_fld[DB_MAXNAME] = prio_name[DB_MAXNAME] = 
				base_name[DB_MAXNAME] = EOS;
			    uleFormat(dberr, E_DM9575_REP_BAD_PRIO_TYPE, 
				(CL_ERR_DESC *)NULL, ULE_LOG,
            		        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			        (i4 *)NULL, err_code, (i4)3,
				STtrmwhite(prio_fld), prio_fld,
				STtrmwhite(prio_name), prio_name,
				STtrmwhite(prio_name), prio_name);
			    status = E_DB_ERROR;
			    break;
			}
			/*
			** field names match, check if values do
			*/
			if (abs(base_tcb->tcb_atts_ptr[j].type) == DB_VCH_TYPE)
			{
			   /*  varchar fields, more checking needed  */
			    MEcopy(record + base_tcb->tcb_atts_ptr[j].offset,
				2, &base_size);
			    base_size = base_size + 2; /* vch length */
			}
			else
			    base_size = base_tcb->tcb_atts_ptr[j].length;
			if (MEcmp(prio_rec + prio_tcb->tcb_atts_ptr[i].offset,
			    record + base_tcb->tcb_atts_ptr[j].offset,
			    base_size) == 0)
			      matched_values++;
			no_match = FALSE;
			break;
		    }
		}
		if (status != E_DB_OK)
		    break;
		if (no_match) /* no matchig fields in the base table */
		{
		    uleFormat(dberr, E_DM9566_REP_NO_PRIO_MATCH, 
			(CL_ERR_DESC *)NULL, ULE_LOG,
            		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			(i4 *)NULL, err_code, (i4)3, DB_MAXNAME,
			prio_tcb->tcb_atts_ptr[i].name.db_att_name, 
			DB_MAXNAME, prio_tcb->tcb_rel.relid, 
			DB_MAXNAME, base_tcb->tcb_rel.relid);
		    status = E_DB_ERROR;
		    break;
		}
	    } /* each field in prio table */
	    if (status != E_DB_OK)
		break;
	    /*
	    ** if all values match, then use the priority
	    ** (subtract 1 for dd_priority field)
	    */
	    if (matched_values == prio_tcb->tcb_rel.relatts - 1)
	    {
		*prio = int_pri;
		break;
	    }
	} /* each priority record */
	break;
    }
    /*
    ** cleanup
    */
    if (prio_rec)
	MEfree(prio_rec);

    return (status);
}

/*
** Name: cdds_lookup - check cdds lookup table
**
** Description:
**	retrieves records from the cdds lookup table and checks them
**	against the given record to see if it should be assigned a
**	cdds for hrozontal partitioning, no match returns cdds = 0.
**
** Inputs:
**	base_rcb	- base table RCB
**	record		- updated record
**
** Outputs:
**	cdds_no
**
** Returns:
**	DB_STATUS
**
** History:
**	21-aug-96 (stephenb)
**	    First written
**	3-jun-97 (stephenb)
**	    dd_routing has now changed to cdds_no
**	22-jun-97 (stephenb)
**	    alter Mecopy to correct no of bytes to cope with above change
**	    dd_rouing has 10 bytes but cdds_no only has 7
**	25-jun-97 (stephenb)
**	    When comparing cdds fields with base table fields, if they 
**	    are varchar, only compare to stated varchar length (the rest of the
**	    field may contain junk). Also tidy up error reporting and make
**	    it illegal to have a CDDS field that is of different type to
**	    it's base table counterpart.
*/
static DB_STATUS
cdds_lookup(
	DMP_RCB		*base_rcb,
	char		*record,
	i2		*cdds_no,
	DB_ERROR	*dberr)
{
    DMP_TCB		*base_tcb = base_rcb->rcb_tcb_ptr;
    DMP_RCB		*cdds_rcb = base_rcb->rep_cdds_rcb;
    DMP_TCB		*cdds_tcb = base_rcb->rep_cdds_rcb->rcb_tcb_ptr;
    STATUS		cl_stat;
    DM2R_KEY_DESC	key;
    DM_TID		tid;
    i4			matched_values;
    bool		no_match;
    i2			int_cdds;
    char		*cdds_rec = NULL;
    DB_STATUS		status;
    i4			i, j;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** get cdds record
    */
    *cdds_no = 0;
    cdds_rec = MEreqmem(0, cdds_tcb->tcb_rel.relwid, TRUE, &cl_stat);
    if (cdds_rec == NULL || cl_stat != OK)
    {
	uleFormat(dberr, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            err_code, (i4)0);
	return(E_DB_ERROR);
    }
    for (;;) /* error checking */
    {
    	status = dm2r_position(cdds_rcb, DM2R_ALL, &key, (i4)0,
	    (DM_TID *)NULL, dberr);
    	if (status != E_DB_OK)
	    break;
	for(;;) /* each cdds record */
	{
	    matched_values = 0;
	    status = dm2r_get(cdds_rcb, &tid, DM2R_GETNEXT, cdds_rec, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT) /* out of records */
		{
		    status = E_DB_OK;
		    break;
		}
		else
		    break;
	    }
	    /*
	    ** check for matching fields in the base table
	    */
	    for (i = 1; i <= cdds_tcb->tcb_rel.relatts; i++)
	    {
		no_match = TRUE;
		if (MEcmp(cdds_tcb->tcb_atts_ptr[i].name.db_att_name, 
		      (base_rcb->rcb_xcb_ptr && 
		  	base_rcb->rcb_xcb_ptr->xcb_scb_ptr &&
		  	base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &&
		  	(*base_rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &
		  	CUI_ID_REG_U)) ? 
		      "CDDS_NO" : "cdds_no", 7) == 0)
		{
		    MEcopy(cdds_rec + cdds_tcb->tcb_atts_ptr[i].offset,
			cdds_tcb->tcb_atts_ptr[i].length, (char *)&int_cdds);
		    continue;
		}
		for (j = 1; j <= base_tcb->tcb_rel.relatts; j++)
		{
		    if (MEcmp(cdds_tcb->tcb_atts_ptr[i].name.db_att_name,
			base_tcb->tcb_atts_ptr[j].name.db_att_name, 
			DB_MAXNAME) == 0)
		    {
			i2	base_size;

			if (cdds_tcb->tcb_atts_ptr[i].length !=
			    base_tcb->tcb_atts_ptr[j].length)
			{
			    char	cdds_fld[DB_MAXNAME +1];
			    char	cdds_name[DB_MAXNAME +1];
			    char	base_name[DB_MAXNAME +1];

			    MEcopy(cdds_tcb->tcb_atts_ptr[i].name.db_att_name,
				DB_MAXNAME, cdds_fld);
			    MEcopy(cdds_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, cdds_name);
			    MEcopy(base_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, base_name);
			    cdds_fld[DB_MAXNAME] = cdds_name[DB_MAXNAME] = 
				base_name[DB_MAXNAME] = EOS;
			    uleFormat(dberr, E_DM956B_REP_BAD_CDDS_LOOKUP, 
				(CL_ERR_DESC *)NULL, ULE_LOG,
            		        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			        (i4 *)NULL, err_code, (i4)5, 
				STtrmwhite(cdds_fld), cdds_fld, 
			        STtrmwhite(cdds_name), cdds_name, (i4)0,
			        cdds_tcb->tcb_atts_ptr[i].length,
				STtrmwhite(base_name), base_name, (i4)0,
			        base_tcb->tcb_atts_ptr[j].length);
			    status = E_DB_ERROR;
			    break;
			}
			else if (base_tcb->tcb_atts_ptr[j].type !=
			    cdds_tcb->tcb_atts_ptr[i].type)
			{
			    /* fields are not of the same type */
			    char	cdds_fld[DB_MAXNAME +1];
			    char	cdds_name[DB_MAXNAME +1];
			    char	base_name[DB_MAXNAME +1];

			    MEcopy(cdds_tcb->tcb_atts_ptr[i].name.db_att_name,
				DB_MAXNAME, cdds_fld);
			    MEcopy(cdds_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, cdds_name);
			    MEcopy(base_tcb->tcb_rel.relid.db_tab_name,
				DB_MAXNAME, base_name);
			    cdds_fld[DB_MAXNAME] = cdds_name[DB_MAXNAME] = 
				base_name[DB_MAXNAME] = EOS;
			    uleFormat(dberr, E_DM9574_REP_BAD_CDDS_TYPE, 
				(CL_ERR_DESC *)NULL, ULE_LOG,
            		        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			        (i4 *)NULL, err_code, (i4)3,
				STtrmwhite(cdds_fld), cdds_fld,
				STtrmwhite(cdds_name), cdds_name,
				STtrmwhite(base_name), base_name);
			    status = E_DB_ERROR;
			    break;
			}
			/*
			** field names match, check if values do
			*/
			if (abs(base_tcb->tcb_atts_ptr[j].type) == DB_VCH_TYPE)
			{
			   /*  varchar fields, more checking needed  */
			    MEcopy(record + base_tcb->tcb_atts_ptr[j].offset,
				2, &base_size);
			    base_size = base_size + 2; /* vch length */
			}
			else
			    base_size = base_tcb->tcb_atts_ptr[j].length;
			if (MEcmp(cdds_rec + cdds_tcb->tcb_atts_ptr[i].offset,
			    record + base_tcb->tcb_atts_ptr[j].offset, 
			    base_size) == 0)
			      matched_values++;
			no_match = FALSE;
			break;
		    }
		}
		if (status != E_DB_OK)
		    break;
		if (no_match) /* no matchig fields in the base table */
		{
		    uleFormat(dberr, E_DM956C_REP_NO_CDDS_MATCH, 
			(CL_ERR_DESC *)NULL, ULE_LOG,
            		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			(i4 *)NULL, err_code, (i4)3, DB_MAXNAME,
			cdds_tcb->tcb_atts_ptr[i].name.db_att_name, 
			DB_MAXNAME, cdds_tcb->tcb_rel.relid, 
			DB_MAXNAME, base_tcb->tcb_rel.relid);
		    status = E_DB_ERROR;
		    break;
		}
	    } /* each field in cdds table */
	    if (status != E_DB_OK)
		break;
	    /*
	    ** if all values match, then use the priority
	    ** (subtract 1 for dd_priority field)
	    */
	    if (matched_values == cdds_tcb->tcb_rel.relatts - 1)
	    {
		*cdds_no = int_cdds;
		break;
	    }
	} /* each cdds lookup record */
	break;
    }
    /*
    ** cleanup
    */
    if (cdds_rec)
	MEfree(cdds_rec);

    return (status);
}

/*
** Name: check_paths
**
** Description:
**	Check that all CDDS's in the current database have correct dd_paths
**	entries where appropriate, and mark the CDDS non-replicating if
**	no path is found where there should be one.
**	information will be used thereafter.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	DB_STATUS
**
** History:
**	27-may-97 (stephenb)
**	    Initial creation (fixing bug 74849)
**	24-jun-97 (stephenb)
**	    Don't check paths if there is only none or one database in the
**	    CDDS, it's un-necesary and it crashed the DBMS.
**      16-Oct-97 (fanra01)
**          Changed parameters to be pointers when calling ule_format.
**          Attempted output of error message during activate would exception.
**  09-Oct-98 (merja01)
**      Correct Unaligned access violations on axp_osf by using
**      ME_ALIGN_MACRO to copy calculated address to ptrs.
**	07-may-1999 (somsa01)
**	    This function is a static function.
**	05-May-05 (wanfr01 for jenjo02)
**	    Bug 114461, INGSRV3292
**	    Correct some memory overwrite errors.
**
*/
static DB_STATUS
check_paths (
	DMP_DCB		*dcb,
	DMP_RCB		*cdds_rcb,
	DMP_RCB		*paths_rcb,
	char		*cdds_rec,
	char		*paths_rec,
	DB_ERROR	*dberr)
{
    DM2R_KEY_DESC	dd_paths_key[2];
    DB_STATUS		status = E_DB_OK;
    DM_TID		tid;
    i2			full_peer = 1;
    i2			*cdds_list = NULL;
    CDDS_INFO		*cdds_info = NULL;
    CDDS_INFO		*current_cdds = NULL;
    CDDS_INFO		*check_cdds = NULL;
    STATUS		cl_stat;
    i2			*curr_cdds_list;
    i2			**next_cdds_list;
    i4			num_recs, i, num_bad_cdds = 10;
    CDDS_INFO		*next_cdds = NULL;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* 
    ** get all CDDSs records
    */
    status = dm2r_position(cdds_rcb, DM2R_ALL, (DM2R_KEY_DESC *)NULL, (i4)2,
	&tid, dberr);
    if (status != E_DB_OK)
    {
	if (dberr->err_code == E_DM0055_NONEXT) /* no records*/
	{
	    /*
	    ** no databases in any CDDS, it's O.K. to have
	    ** no paths in this case, so we won't bother checking
	    ** any further
	    */
	    status = E_DB_OK;
	    dm0s_mlock(&dcb->dcb_mutex);
	    if (dcb->dcb_bad_cdds == NULL) /* no one has changed it while
					   ** we were checking */
	    	dcb->dcb_bad_cdds = (i2 *)-1;
	    dm0s_munlock(&dcb->dcb_mutex);
	    return (E_DB_OK);
	}
	else
	    return(status);
    }
    for (;;) /* error handling */
    {
    	for (num_recs = 1;;num_recs++) /* each dd_db_cdds record */
    	{
	    status = dm2r_get(cdds_rcb, &tid, DM2R_GETNEXT, cdds_rec, dberr);
            if (status != E_DB_OK)
            {
	        if (dberr->err_code == E_DM0055_NONEXT) /* no more records*/
	        {
	            status = E_DB_OK;
		    break;
	        }
	        else
	            break;
            }
	    /* get memory to store CDDS info */
	    current_cdds = 
		(CDDS_INFO *)MEreqmem(0, sizeof(CDDS_INFO), FALSE, &cl_stat);
	    if (current_cdds == NULL || cl_stat != OK)
	    {
		SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	        status = E_DB_ERROR;
	        break;
            }
	    /* and queue it */
	    if (cdds_info == NULL) /* empty queue */
	    {
	        cdds_info = current_cdds;
	        cdds_info->cdds_next = NULL;
	        cdds_info->cdds_prev = NULL;
	    }
	    else if (cdds_info->cdds_prev == NULL) /* one entry */
	    {
	        cdds_info->cdds_prev = cdds_info->cdds_next = current_cdds;
		current_cdds->cdds_next = current_cdds->cdds_prev = cdds_info;
	    }
	    else /* several entries */
	    {
	        cdds_info->cdds_prev->cdds_next = current_cdds;
	        cdds_info->cdds_prev = current_cdds;
	        current_cdds->cdds_next = cdds_info;
	    }
	    /* fill out the entry */
	    MEcopy(cdds_rec, 2, (char *)&current_cdds->cdds_no);
	    MEcopy(cdds_rec + cdds_rcb->rcb_tcb_ptr->tcb_atts_ptr[2].offset, 2,
	        (char *)&current_cdds->database_no);
	    MEcopy(cdds_rec +  cdds_rcb->rcb_tcb_ptr->tcb_atts_ptr[3].offset, 2,
	        (char *)&current_cdds->target_type);
        }
	if (status != E_DB_OK)
	    break;
	/* 
	** if there is none or one database, then there's no 
	** need to check paths 
	*/
	if (cdds_info == NULL || 
	    cdds_info->cdds_next == NULL || cdds_info->cdds_prev == NULL)
	    break;
	/* allocate memory to hold list of CDDSs to check */
	cdds_list = (i2 *)MEreqmem(0, sizeof(i2)*num_recs, FALSE, &cl_stat);
	if (cdds_list == NULL || cl_stat != OK)
	{
	    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
	    status = E_DB_ERROR;
	    break;
        }
	cdds_list[0] = -1;
	/*
	** find a list of CDDSs to check
	*/
	for (current_cdds = cdds_info;;)
	{
	    /* local database must be full peer (target type = 1) in the CDDS */
	    if (current_cdds->target_type != 1 || 
		current_cdds->database_no != dcb->dcb_rep_db_no)
	    {
		current_cdds = current_cdds->cdds_next;
		if (current_cdds == cdds_info)
		    break;
		else
		    continue;
	    }
	    /* add CDDS to list if it's not there already */
	    for (i = 0; cdds_list[i] != -1 && 
		cdds_list[i] != current_cdds->cdds_no ; i++);
	    if (cdds_list[i] == -1)
	    {
		cdds_list[i] = current_cdds->cdds_no;
		cdds_list[i + 1] = -1;
	    }
	    current_cdds = current_cdds->cdds_next;
	    if (current_cdds == cdds_info)
		break;
	}
	/*
	** check paths for each CDDS
	*/
	for (i = 0; cdds_list[i] != -1; i++)
	{
	    for (current_cdds = cdds_info;;)
	    {
		/* find other databases in this CDDS */
		if (current_cdds->cdds_no != cdds_list[i] ||
		    current_cdds->database_no == dcb->dcb_rep_db_no)
		{
		    current_cdds = current_cdds->cdds_next;
		    if (current_cdds == cdds_info)
			break;
		    else
		        continue;
		}

		/* check the path */
	    	dd_paths_key[0].attr_operator = DM2R_EQ;
	    	dd_paths_key[0].attr_number = 1; /* cdds_no */
	    	dd_paths_key[0].attr_value = (char *)&cdds_list[i];
	    	dd_paths_key[1].attr_operator = DM2R_EQ;
	    	dd_paths_key[1].attr_number = 4; /* targetdb */ 
	    	dd_paths_key[1].attr_value = (char *)&current_cdds->database_no;

	    	status = dm2r_position(paths_rcb, DM2R_QUAL, dd_paths_key,
		    (i4)2, &tid, dberr);
	    	if (status != E_DB_OK)
                {
	            if (dberr->err_code == E_DM0055_NONEXT) /* no records*/
	            {
			uleFormat(NULL, W_DM9571_REP_DISABLE_CDDS, (CL_ERR_DESC *)NULL, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)3, 
			    sizeof(current_cdds->database_no), 
			    &current_cdds->database_no,
			    sizeof(dcb->dcb_rep_db_no), &dcb->dcb_rep_db_no,
			    sizeof(cdds_list[i]), &cdds_list[i]);
			if (num_bad_cdds == 10)
			{
			    curr_cdds_list = (i2 *)MEreqmem(0, 
			        sizeof(i2)*10 + sizeof(PTR) + sizeof(ALIGN_RESTRICT), FALSE, &cl_stat);
			    if (dcb->dcb_bad_cdds == NULL)
				dcb->dcb_bad_cdds = curr_cdds_list;
			    else
				*next_cdds_list = curr_cdds_list;
                next_cdds_list = (i2 **)ME_ALIGN_MACRO(
                   (curr_cdds_list + 10), sizeof(ALIGN_RESTRICT));
			    *next_cdds_list = NULL;
			    num_bad_cdds = 0;
			}
			curr_cdds_list[num_bad_cdds] = cdds_list[i];
			num_bad_cdds++;
			curr_cdds_list[num_bad_cdds] = -1;
			status = E_DB_OK;
		        break;
	            }
	            else
	                break;
                }
		for (;;) /* paths records */
		{
		    i2		target_db; 

		    status = dm2r_get(paths_rcb, &tid, DM2R_GETNEXT,
		        paths_rec, dberr);
	            if (status != E_DB_OK)
	            {
	                if (dberr->err_code == E_DM0055_NONEXT) /* no records */
	                {
			    uleFormat(NULL, W_DM9571_REP_DISABLE_CDDS, (CL_ERR_DESC *)NULL, 
				ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
				(i4 *)NULL, err_code, (i4)3, 
			        sizeof(current_cdds->database_no), 
			        &current_cdds->database_no,
			        sizeof(dcb->dcb_rep_db_no), &dcb->dcb_rep_db_no,
			        sizeof(cdds_list[i]), &cdds_list[i]);
			    if (num_bad_cdds == 10)
			    {
			        curr_cdds_list = (i2 *)MEreqmem(0, 
			            sizeof(i2)*10 + sizeof(PTR) + sizeof(ALIGN_RESTRICT), 
				    FALSE, &cl_stat);
			        if (dcb->dcb_bad_cdds == NULL)
				    dcb->dcb_bad_cdds = curr_cdds_list;
			        else
				    *next_cdds_list = curr_cdds_list;
                    next_cdds_list = (i2 **)ME_ALIGN_MACRO(
                      (curr_cdds_list + 10), sizeof(ALIGN_RESTRICT));
			        *next_cdds_list = NULL;
			        num_bad_cdds = 0;
			    }
			    curr_cdds_list[num_bad_cdds] = cdds_list[i];
			    num_bad_cdds++;
			    curr_cdds_list[num_bad_cdds] = -1;
			    status = E_DB_OK;
		            break;
	                }
	                else
		            break;
		    }
		    /* check the record for databas no */
		    MEcopy(paths_rec + 
			paths_rcb->rcb_tcb_ptr->tcb_atts_ptr[4].offset,
			2, &target_db);
		    if (target_db != current_cdds->database_no)
			continue;
		    else
			break;
                }
		if (status != E_DB_OK)
		    break;
		current_cdds = current_cdds->cdds_next;
		if (current_cdds == cdds_info)
		    break;
	    }
	    if (status != E_DB_OK)
		break;
	}
	break;
    }
    /*
    ** cleanup
    */
    /* cdds info structures */
    if (cdds_info)
    {
    	for (current_cdds = cdds_info;;)
    	{
	    next_cdds = current_cdds->cdds_next;
	    MEfree((char *)current_cdds);
	    current_cdds = next_cdds;
	    if (current_cdds == cdds_info || current_cdds == NULL)
	    	break;
        }
    }
    /* list of CDDSs */
    if (cdds_list)
        MEfree((char *)cdds_list);

    return(status);
}
