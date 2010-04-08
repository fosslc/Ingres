/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <dmacb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qefscb.h>

#include    <ade.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>
#include    <rdf.h>
#include    <st.h>

/**
**
**  Name: QEADMU.C - perform the DBU action item
**
**  Description:
**      Any DBU (DMU) action items are performed by the routines
**  in this file. 
**
**	The QEA_DMU actions are mid-query actions, as opposed to standalone
**	creates or modifies.  At present (Apr 2004), the only ways to get
**	here are: from the CREATE TABLE AS SELECT query, which combines
**	create and modify DBU actions with a query;  and OPF/OPC
**	generated temp file create requests.
**
**	Plain CREATE TABLE has its own action header that doesn't come
**	here (it dispatches to DBU directly).  Plain MODIFY, CREATE
**	INDEX, DROP, etc dispatch directly to DBU from the sequencer
**	and have no query plan or action header at all.
**
**	(Some of the confusion surrounding this whole area is perhaps
**	explainable.  It appears that the old RTI development team was
**	in the midst of incrementally moving DDL from direct-execute
**	to going thru a query plan, partly for create schema and perhaps
**	to eventually support DDL in DB procedures.  That whole plan
**	was never completed, leaving the code in a weird half-done state.)
**
**          qea_dmu - execute a DBU
**
**
**  History:    $Log-for RCS$
**	1-jul-86 (daved)    
**          written
**	19-may-88 (puree)
**	    modified qea_dmu to clear dmt_record_access_id when the table is
**	    closed.
**	05-oct-88 (puree)
**	    modified qeu_dmu due to change in user error 5102 format by PSF.
**	09-nov-88 (puree)
**	    modified qeu_dmu to report the location name that is invalid 
**	    when a table is created.
**      29-jul-89 (jennifer)
**          Added auditing of DML operations.
**	20-nov-92 (rickh)
**	    New arguments to qeu_dbu.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	9-jun-93 (rickh)
**	    Added new arguments to qeu_dbu call.
**	01-sep-93 (davebf)
**	    fixed problem with iterative DBPs involving temporary tables
**	10-sep-93 (anitap)
**	    Added support for implicit creation of schema for CREATE TABLE AS
**	    SELECT in qea_dmu().
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	8-nov-93 (rickh)
**	    Changed the error reporting under E_DM001D_BAD_LOCATION_NAME
**	    to refer to the dmt_cb rather than the dmu_cb.
**	26-oct-93 (anitap)
**	    check for qef_stmt_type and do not create schema for "global temp 
**	    table as select .." if QEF_DGTT_AS_SELECT.
**      27-Aug-97 (fanra01)
**          Bug 84939, 85093
**          Message E_US15A0 not displayed.
**          Add initialisation of the error reporting semantics field eflag
**          to default to QEF_EXTERNAL before using the control block,
**          indicating that errors should be reported to the user.
**	06-jul-1998 (somsa01)
**	    In qea_dmu(), added setting of dsh_dgtt_cb_no in the case of
**	    QEF_DGTT_AS_SELECT.  (Bug #91827)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Deleted qef_dmf_id, use dmf_sess_id instead.
**      16-dec-2002 (stial01)
**          qea_dmu() After creating session temp, RDF_INVALIDATE (B109189)
**	5-Feb-2004 (schka24)
**	    Partitioned tables - call utility for table creating.
**	    Remove 16-dec-2002 edit, session temps are now invalidated out
**	    of RDF at session exit.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**/


/*{
** Name: QEA_DMU	- database manipulation utility action item
**
** Description:
**      The specified DMU is executed. This routine sets up a call
**  to the QEU_DMU routine, which does the actual work. Performance
**  is not a requirement on the DBU calls. The extra function call won't
**  slow things down much anyway. 
**
**	As noted in the file intro, you can only get here for
**	creates and modifies done as part of a query.  Ie, temp
**	table creates, or create table as select.
**
**  QEU_DMU will log any appropriate errors. 
**
** Inputs:
**      qef_rcb                         qef request block
**	    .qef_cb			session control block
**	qea_ahd				action entry for the DBU
**	    .qhd_obj.qhd_dmu.ahd_func	DMF function id
**	    .qhd_obj.qhd_dmu.ahd_cb	DMU control block template
**	    .qhd_obj.qhd_dmu.ahd_alt	valid list containing control block 
**					number to place 
**					table id if this is an open table.
**					set to NULL if table id should not be
**					copied.
** Outputs:
**      qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE0004_NO_TRANSACTION
**				E_QE000D_NO_MEMORY_LEFT
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jul-86 (daved)
**          written
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	19-may-88 (puree)
**	    clear the dmt_record_access_id so we know that this table is
**	    closed.
**	05-oct-88 (puree)
**	    modified qeu_dmu due to change in user error 5102 format by PSF.
**	09-nov-88 (purre)
**	    modified qeu_dmu to report the location name that is invalid 
**	    when a table is created.
**	20-nov-92 (rickh)
**	    New arguments to qeu_dbu.
**	9-jun-93 (rickh)
**	    Added new arguments to qeu_dbu call.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	01-sep-93 (davebf)
**	    fixed problem with iterative DBPs involving temporary tables
**	10-sep-93 (anitap)
**	    Added support for implicit creation of schema for CREATE TABLE AS
**	    SELECT statement.
**	8-nov-93 (rickh)
**	    Changed the error reporting under E_DM001D_BAD_LOCATION_NAME
**	    to refer to the dmt_cb rather than the dmu_cb.
**	26-oct-93 (anitap)
**	    Do not create schema for "global temporary table as select...".
**      16-may-95 (ramra01)
**          Reuse the temp tables used in a dbproc that is in a loop.
**          Bug 68464.
**          Had to undo the fix as implemented on 1-sep-93 by davebf. The 
**          context has since changed and required to implement the idea of
**          utilising the instances of the temp table as defined in the
**	    vl struct.      
**      01-nov-95 (ramra01)
**          compiler warnings fix
**	07-jan-1997 (canor01)
**	    If we get an error during CREATE TABLE, do not continue to
**	    qea_schema(), which will reset the status.
**      27-Aug-97 (fanra01)
**          Bug 84939, 85093
**          Add initialisation of the error reporting semantics field eflag
**          to default to QEF_EXTERNAL before using the control block,
**          indicating that errors should be reported to the user.
**	06-jul-1998 (somsa01)
**	    Added setting of dsh_dgtt_cb_no in the case of QEF_DGTT_AS_SELECT.
**	    (Bug #91827)
**	 3-nov-2000 (hayke02)
**	    Partial cross integration of sarjo01's ingres!oping12 change
**	    439635 - we now call dmr_dump_data(). This ensures correct reuse
**	    of temp tables in looping DB procs. This change fixes bug 102928.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	11-Feb-2004 (schka24)
**	    Call new create utility for partitions.
**	6-Apr-2004 (schka24)
**	    Update rowcount in dsh, not rcb.  Some comment updates.
**	    (later) Looks like temp create shouldn't touch rowcount.
**	1-Jul-2004 (schka24)
**	    qeu error should go into dsh as well, caused silly QE0018.
**	    Use passed-in dsh instead of qef-rcb dsh.
**	07-Apr-2008 (thaju02)
**	    Close partitions to avoid modify failing with E_DM9C01 in
**	    dm0p_close_pages. (B120208)
**	4-Jun-2009 (kschendel) b122118
**	    Use close-and-unlink utility.
[@history_line@]...
*/
DB_STATUS
qea_dmu(
QEF_AHD		*qea_act,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh )
{
    QEU_CB              qeu_cb;
    DB_STATUS		status;
    DMU_CB		dmu_tcb;	/* temporary control block */
    DMU_CB		*dmu_cb;
    DMT_CB		dmt_tcb;	/* temporary control block */
    DMT_CB		*dmt_cb;
    QEF_VALID		*vl;
    i4		operation;
    i4		error;
    i4		errcode;
    DMT_CB		*dmt2_cb,*dmt3_cb,*dmt4_cb;
    DMT_CB		*next;
    i4			cb_no;
    PTR                 *cbs;
    i4			tabid_count;
    i4			i;
    DB_TAB_ID		*table_ids;
    DB_TAB_ID		table_id;
    ULM_RCB		ulm;


    dsh->dsh_error.err_code = E_QE0000_OK;
    operation = qea_act->qhd_obj.qhd_dmu.ahd_func;
    vl = qea_act->qhd_obj.qhd_dmu.ahd_alt;
    cbs = dsh->dsh_cbs;

    /* handle DMUs and DMTs seperately */
    switch (operation)
    {
	/* Note that qeu_dmu normally does transaction management.
	** We bypass that mechanism by setting the third parameter to
	** false.
	*/
	case DMU_DESTROY_TABLE:
	case DMU_INDEX_TABLE:
	case DMU_MODIFY_TABLE:
	case DMU_RELOCATE_TABLE:
	    if (vl)
		cb_no   = vl->vl_dmf_cb;
	    else
		cb_no = -1;

	STRUCT_ASSIGN_MACRO(*(DMU_CB*)qea_act->qhd_obj.qhd_dmu.ahd_cb, dmu_tcb);
	    if (cb_no > -1)
	    {
		dmt_cb = (DMT_CB *) cbs[cb_no];
		STRUCT_ASSIGN_MACRO(dmt_cb->dmt_id, dmu_tcb.dmu_tbl_id);
	    }
	    /* look through list of open tables and close any involved in
	    ** this DBU call.
	    ** *FIXME?* leaves unanswered what might happen to refs to the
	    ** same table in other parts of a parallel query;  however at
	    ** present we should never see DDL except in the parent.
	    */
	    dmt_cb = (DMT_CB *) dsh->dsh_open_dmtcbs;
	    for (; dmt_cb; dmt_cb = next)
	    {
		next = (DMT_CB *)(dmt_cb->q_next);
		if ((dmt_cb->dmt_id.db_tab_base == 
			dmu_tcb.dmu_tbl_id.db_tab_base &&
		    dmt_cb->dmt_id.db_tab_index == 
			dmu_tcb.dmu_tbl_id.db_tab_index) ||
		    ((dmt_cb->dmt_id.db_tab_base ==
			dmu_tcb.dmu_tbl_id.db_tab_base) &&
		    (dmt_cb->dmt_id.db_tab_index & DB_TAB_PARTITION_MASK)))
		{
		    status = qen_closeAndUnlink(dmt_cb, dsh);
		    if (status != E_DB_OK)
		    {
			qef_error(dmt_cb->error.err_code, 0L, status,
			    &error, &dsh->dsh_error, 0);
			return (status);
		    }
		}
	    }   
	    /* setup for QEU_DMU call */
	    qeu_cb.qeu_db_id	= qef_rcb->qef_db_id;
	    qeu_cb.qeu_d_id	= qef_rcb->qef_sess_id;
	    qeu_cb.qeu_d_op	= qea_act->qhd_obj.qhd_dmu.ahd_func;
	    qeu_cb.qeu_d_cb	= (PTR) &dmu_tcb;
	    /* Make sure we're not passing partitioning junk to dbu.
	    ** This modify (which must be part of a create table as
	    ** select modify) has to be a straight, non-relocating,
	    ** non-repartitioning modify.
	    */
	    qeu_cb.qeu_logpart_list = NULL;
	    dmu_tcb.dmu_part_def = NULL;
	    dmu_tcb.dmu_ppchar_array.data_in_size = 0;
	    dmu_tcb.dmu_ppchar_array.data_address = NULL;
	    /* FIXME opc should ideally set dmu_nphys_parts properly */
	    /* For now we'll bank on dmf not noticing that it's wrong */
	    dmu_tcb.dmu_nphys_parts = 0;
            /*
            **  Set default error reporting semantics to user.
            */
            qeu_cb.qeu_eflag    = QEF_EXTERNAL;

	    /* Do the modify -- allow the rowcount to be updated. */
	    status = qeu_dbu( qef_rcb->qef_cb, &qeu_cb, 0, (DB_TAB_ID *) NULL,
			      &dsh->dsh_qef_rowcount,
			      &dsh->dsh_error, ( i4  ) QEF_INTERNAL );
	    break;

	case DMU_CREATE_TABLE:

	    STRUCT_ASSIGN_MACRO(*(DMU_CB*)qea_act->qhd_obj.qhd_dmu.ahd_cb,
		 dmu_tcb);
	    dmu_cb			= &dmu_tcb;
	    dmu_cb->type		= DMU_UTILITY_CB;
	    dmu_cb->length		= sizeof(DMU_CB);
	    dmu_cb->dmu_db_id		= qef_rcb->qef_db_id;
	    dmu_cb->dmu_flags_mask	= 0;
	    dmu_cb->dmu_tran_id		= dsh->dsh_dmt_id;

	    if (dmu_cb->dmu_part_def == NULL)
	    {
		tabid_count = 1;
		table_ids = &table_id;
	    }
	    else
	    {
		/* Partitioned table, we need to fill in the partition
		** table-id's in the validation list entries.  qeqpart
		** needs these for opening up the partitions as rows
		** are tossed at them.
		** Since there might be lots of partitions, allocate
		** some dsh-stream memory for them.  (and the master.)
		*/
		STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
		ulm.ulm_streamid_p = &dsh->dsh_streamid;
		tabid_count = dmu_cb->dmu_part_def->nphys_parts + 1;
		ulm.ulm_psize = tabid_count * sizeof(DB_TAB_ID);
		status = qec_malloc(&ulm);
		if (DB_FAILURE_MACRO(status))
		{
		    dsh->dsh_error.err_code = ulm.ulm_error.err_code;
		    return (status);
		}
		table_ids = (DB_TAB_ID *) ulm.ulm_pptr;
	    }
	    status = qeu_create_table(qef_rcb, dmu_cb,
			qea_act->qhd_obj.qhd_dmu.ahd_logpart_list,
			tabid_count, table_ids);
	    STRUCT_ASSIGN_MACRO(qef_rcb->error, dsh->dsh_error);
	    if (DB_SUCCESS_MACRO(status) && vl)
	    {
		for (;vl; vl = vl->vl_alt)
		{
		    cb_no = vl->vl_dmf_cb;
		    dmt_cb = (DMT_CB*)cbs[cb_no];
		    STRUCT_ASSIGN_MACRO(table_ids[0], dmt_cb->dmt_id);

		    if (vl->vl_flags & QEF_PART_TABLE)
		    {
			/* If it's hanging off this list, I guess it's
			** the right one... update partition ID's
			*/
			i = tabid_count-1;
			if (i > vl->vl_partition_cnt)
			    i = vl->vl_partition_cnt;
			while (--i >= 0)
			{
			    STRUCT_ASSIGN_MACRO(table_ids[i+1],
				vl->vl_part_tab_id[i]);
			}
		    }

		    if (qef_rcb->qef_stmt_type & QEF_DGTT_AS_SELECT)
			dsh->dsh_dgtt_cb_no = cb_no;
		}
	    }
	    /* probably irrelevant, but we're at the start of the
	    ** process - no rowcount yet.
	    */
	    dsh->dsh_qef_rowcount   = -1;
	    
	    break;


	case DMT_CREATE_TEMP:
	    /* check for case that this is in an iterating DBP -- and the last iteration
	    ** created and opened a temp table */
	    if (vl)
	    {
		cb_no = vl->vl_dmf_cb;
		dmt2_cb = (DMT_CB*)cbs[cb_no];
		if(dmt2_cb->dmt_record_access_id != (PTR)NULL)
		{

/* if this is a dbproc in a loop, the temp tables cannot be destroyed     **
** followed by create and open. The table validation has changed since    **
** the earlier fix was implemented. The old FIX will not with in  1.1     **
** code. The right thing to do is to reuse the Temp tables and not have   **
** to deal with the refresh of rcb in the DSH blocks etc...               **
** By notifying the PROJ node of the fact that the temp table exist and   **
** can be reused, it will empty out the temp table and reuse it in DMR_   **
** LOAD phase.   <--- bug 68464                                           */ 
		    DMR_CB	*temp_dmr_cb;

        	    for(;vl; vl = vl->vl_alt)
        	    {
			temp_dmr_cb = (DMR_CB *)cbs[vl->vl_dmr_cb];
			temp_dmr_cb->dmr_flags_mask = 0x00000000;
			status = dmf_call(DMR_DUMP_DATA, temp_dmr_cb);
			temp_dmr_cb->dmr_flags_mask =
						DMR_EMPTY | DMR_TABLE_EXISTS;
                    }
		    return status;
		}
	    }

	    STRUCT_ASSIGN_MACRO(*(DMT_CB*)qea_act->qhd_obj.qhd_dmu.ahd_cb,
		 dmt_tcb);
	    dmt_cb			= &dmt_tcb;
	    dmt_cb->type		= DMT_TABLE_CB;
	    dmt_cb->length		= sizeof(DMT_CB);
	    dmt_cb->dmt_db_id		= qef_rcb->qef_db_id;
	    dmt_cb->dmt_tran_id		= dsh->dsh_dmt_id;

	    /* now do the actual DMF operation */
	    status = dmf_call(DMT_CREATE_TEMP, dmt_cb);
	    
	    if (status == E_DB_OK && vl)
	    {
		for (;vl; vl = vl->vl_alt)
		{
		    cb_no = vl->vl_dmf_cb;
		    dmt2_cb = (DMT_CB*) cbs[cb_no];
		    STRUCT_ASSIGN_MACRO(dmt_cb->dmt_id, dmt2_cb->dmt_id);
		    MEcopy((PTR)&dmt_cb->dmt_owner, sizeof(DB_OWN_NAME),
			   (PTR)&dmt2_cb->dmt_owner);
		    dmt2_cb->dmt_record_access_id = (PTR)NULL;
		}
	    }
	    if (status != E_DB_OK)
	    {
		errcode = dmt_cb->error.err_code;
		if (qef_rcb->qef_eflag == QEF_EXTERNAL)
		{
		    switch (errcode)
		    {
			DB_LOC_NAME	*loc;
			i4		loc_idx;

			case E_DM001D_BAD_LOCATION_NAME:
			    loc_idx = dmt_cb->error.err_data;
			    loc = ((DB_LOC_NAME *)
				 (dmt_cb->dmt_location.data_address)) + loc_idx;
			    qef_error(5113L, 0L, status, &error,&dsh->dsh_error, 1,
				sizeof(DB_LOC_NAME), (PTR)(loc));
			    errcode = E_QE0025_USER_ERROR;
			    break;

			case E_DM0078_TABLE_EXISTS:
			    errcode = E_QE0050_TEMP_TABLE_EXISTS;
			    status = E_DB_SEVERE;
			    break;

			default:
			    break;
		    }
		}
		dsh->dsh_error.err_code = errcode;
	    }
	    /* Rowcount shouldn't be touched at all, we're in the
	    ** middle of doing other stuff
	    */
	    break;


	default:
	    status = E_DB_ERROR;
    }
    return (status);    
}
