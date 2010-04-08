/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefscb.h>
#include    <tm.h>
#include    <ex.h>
#include    <dmmcb.h>
#include    <dmucb.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QENSORT.C - Sort node
**
**  Description:
**      The routine in this file loads temporary sort tables 
**
**          qen_sort	 - load temporary sort table.
**	    qen_s_dump	 - dump the internal sort buffer to DMF sorter.
**
**
**  History:    $Log-for RCS$
**      28-aug-86 (daved)
**          written
**	16-mar-87 (rogerk) Changed interface to DMR_LOAD to use dmr_mdata
**	    instead of dmr_data.
**	02-feb-88 (puree)
**	    Added reset flag to the function arguments.
**	11-mar-88 (puree)
**	    Removed DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	01-jun-88 (puree)
**	    Added debug code for qen_sort.
**	26-sep-88 (puree)
**	    Modified qen_sort for QEF in-memory sorter.  Implemented 
**	    qen_s_dump.
**	07-jan-91 (davebf)
**	    Modified for insert in_memory sorter which eliminates duplicates
**	    upon insert.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	04-may-1992 (bryanp)
**	    Changed dmt_location from a DB_LOC_NAME to a DM_DATA.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      23-aug-93 (andys)
**          If there is a commit in the middle of a database procedure,
**          then the holdfile may have been closed. If we then do a
**          dmr_dump_data on the closed file then we fall over. When
**          the commit happens (in qea_commit) then
**          dmt_record_access_id is set no NULL for the DMT_CB.  So,
**          check if this field is NULL, if it is then set status to
**          indicate that hold file should be initialised. [bug 44556]
**          [was 04-aug-92 (andys) in 6.4].
**	19-may-95 (ramra01)
**	    Changed the sort calling sequence to call a heap sort and
**	    not the insertion sort for performance enhancement. This will 
**          perform well under all conditions as indicated in the 
**	    test cases. 
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instance by pointer in 
**	    QEN_SORT.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          In qen_sort(), set DSH_SORT flag before calling an input function, 
**          and reset it before returning to the caller.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       1-Nov-2002 (hanal04) Bug 109013 INGSRV 1986
**          Calls to DMR_LOAD require SORT or NOSORT to be explicitly set
**          (See change 438296). 
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**/

/*	static functions	*/

static	DB_STATUS
qen_s_dump(
QEN_SHD		   *shd,
QEE_DSH		   *dsh,
QEN_NODE           *node,
DMR_CB		   *dmr_get,
i4		   rowno,
bool		   heap_sort,
bool		   no_qef );




/*{
** Name: QEN_SORT	- load temporary sort table
**
** Description:
**      This routine is used to read tuples from the underneath node and
**      load them into the sort table. The parent of this routine must be
**	a QEN_SIJOIN node. The basic algorithm is:
**
**	    1) Read tuples from the underneath node
**	    2) Project tuples on the sort attributes
**	    3) Append tuples to sorter
**
**	This routine does not operate like a standard node-operator routine,
**	in that you don't call it to supply rows.  You call it once to run
**	the sorter and set up the result as a hold file.  The caller then
**	uses qen_u3_position_begin and qen_u4_read_positioned to retrieve
**	rows from the hold file.  This probably makes row retrieval a
**	bit quicker than retrieval from a top-sort since it avoids a small
**	handful of if-tests.
**
** Inputs:
**      node                            the sort node data structure
**	rcb				request block
**	    .qef_cb.qef_dsh		the data segment header
**	dmr_get    			DMR_CB of parent used to
**					read sorted records.
**
** Outputs:
**      rcb
**	    .error.err_code		one of the following
**					E_QE0000_OK
**					E_QE0019_NON_INTERNAL_FAIL
**					E_QE0002_INTERNAL_ERROR
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
**      28-aug-86 (daved)
**          written
**	16-mar-87 (rogerk) Changed interface to DMR_LOAD to use dmr_mdata
**	    instead of dmr_data.
**	29-sep-87 (rogerk)
**	    zero out dmr_s_estimated_records rows before dmr_load call.
**	02-nov-87 (puree)
**	    Set dmr_s_estimated_records from qen_est_tuples.
**	02-feb-88 (puree)
**	    Added reset flag to the function arguments.
**	11-mar-88 (puree)
**	    Removed DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	01-jun-88 (puree)
**	    Added debug code to prevent AV if qen_sort is not called by
**	    qen_sijoin.
**	26-sep-88 (puree)
**	    Implement QEF in-memory sorter.
**	28-sept-89 (eric)
**	    Incremented qen_status->node_rcount to keep track of how many
**	    rows this node returns. This will be used to fix the Sybase bug.
**	07-jan-91 (davebf)
**	    Modified for insert in_memory sorter which eliminates duplicates
**	    upon insert.
**	31-jan-91 (davebf)
**	    Fix bug 34959.
**	10-sept-91 (vijay)
**	    Initialize status to OK. If not reset, not first time etc, the
**	    status variable is never used and may have garbage in it.(bug38794)
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to CSswitch.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-aug-93 (andys)
**          If there is a commit in the middle of a database procedure,
**          then the holdfile may have been closed. If we then do a
**          dmr_dump_data on the closed file then we fall over. When
**          the commit happens (in qea_commit) then
**          dmt_record_access_id is set no NULL for the DMT_CB.  So,
**          check if this field is NULL, if it is then set status to
**          indicate that hold file should be initialised. [bug 44556]
**          [was 04-aug-92 (andys) in 6.4].
**	14-dec-93 (rickh)
**	    Call qen_u33_resetSortFile when releasing SHD memory.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF structure instance by pointer in 
**	    QEN_SORT.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DSH_SORT flag before calling an input function, and reset it
**          before returning to the caller of this routine.
**	18-jul-97 (inkdo01)
**	    Add support for tracepoint QE92 which uses the old insert sort
**	    for duplicates removing sorts. A very small number of such sorts
**	    (those with very large numbers of dups) may perform better with 
**	    insert than heap).
**       1-Nov-2002 (hanal04) Bug 109013 INGSRV 1986
**          Calls to DMR_LOAD require SORT or NOSORT to be explicitly set
**          (See change 438296). Add DMR_HEAPSORT to prevent E_DM002A
**          in start_load().
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
**	2-jan-04 (inkdo01)
**	    Added end-of-partition-grouping logic.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, STATUS) to
**	    ptrs to arrays of ptrs.
**	17-mar-04 (toumi01)
**	    Add support for "close" call to allow parallel query termination.
**	8-apr-04 (inkdo01)
**	    Added dsh parm to QEF_CHECK_FOR_INTERRUPT macro.
**	28-apr-04 (inkdo01)
**	    Add support for "end of partition group" call.
**	04-Nov-2004 (jenjo02)
**	    Only CSswitch on root DSH or if not OS-threaded.
**	6-Dec-2005 (kschendel)
**	    Relocate compare-list to node, opc builds it now.
**	13-Jan-2006 (kschendel)
**	    Revise CX evaluation calls.
**	16-Feb-2006 (kschendel)
**	    Return after close, don't keep going.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
**	1-feb-2008 (dougi)
**	    Add qe94 to disable QEF sort altogether.
**	4-Jun-2009 (kschendel) b122118
**	    Use slightly faster/more convenient dsh test for qe90, and
**	    remove pointless error returns from cost begin/end.  Minor
**	    streamlining. Make qen_sort call the same as everyone else, add
**	    qen_inner_sort for the old qen_sort entry point with a non-null
**	    dmrcb.
*/

/* The standard (*node)(...) entry point should only be used with a
** nonzero function, e.g. for open, close, end-of-group.
*/

DB_STATUS
qen_sort(
QEN_NODE           *node,
QEF_RCB		   *qef_rcb,
QEE_DSH		   *dsh,
i4		   function )
{
    return (qen_inner_sort(node, qef_rcb, dsh, NULL, function));
}

DB_STATUS
qen_inner_sort(
QEN_NODE           *node,
QEF_RCB		   *qef_rcb,
QEE_DSH		   *dsh,
DMR_CB		   *dmr_get,
i4		   function )
{
    QEF_CB	*qef_cb = dsh->dsh_qefcb;
    PTR		*cbs = dsh->dsh_cbs;
    ADE_EXCB	*ade_excb;
    DMR_CB	*dmr_load = (DMR_CB*) cbs[node->node_qen.qen_sort.sort_load];
    QEN_NODE	*out_node = node->node_qen.qen_sort.sort_out;
    QEE_XADDRS	*node_xaddrs = dsh->dsh_xaddrs[node->qen_num];
    QEN_STATUS	*qen_status = node_xaddrs->qex_status;
    QEN_SHD	*shd = dsh->dsh_shd[node->node_qen.qen_sort.sort_shd];
    DB_STATUS	status = E_DB_OK;
    bool	reset = ((function & FUNC_RESET) != 0);
    bool	out_reset = reset;
    bool	heap_sort = TRUE, no_qef = FALSE;
    i4		rowno;
    DM_MDATA	dm_mdata;
    ULM_RCB	ulm_rcb;
    i4	val1;
    i4	val2;
    TIMERSTAT	    timerstat;
    DMT_CB          *dmt_cb;


    /* Do open processing, if required. Only if this is the root node
    ** of the query tree do we continue executing the function. */
    if ((function & TOP_OPEN || function & MID_OPEN) && 
	!(qen_status->node_status_flags & QEN1_NODE_OPEN))
    {
	status = (*out_node->qen_func)(out_node, qef_rcb, dsh, MID_OPEN);
	qen_status->node_status_flags |= QEN1_NODE_OPEN;
	if (function & MID_OPEN)
	    return(E_DB_OK);
	function &= ~TOP_OPEN;
    }
    /* Do close processing, if required. */
    if (function & FUNC_CLOSE)
    {
	if (!(qen_status->node_status_flags & QEN8_NODE_CLOSED))
	{
	    status = (*out_node->qen_func)(out_node, qef_rcb, dsh, FUNC_CLOSE);
	    qen_status->node_status_flags = 
		(qen_status->node_status_flags & ~QEN1_NODE_OPEN) | QEN8_NODE_CLOSED;
	}
	return(E_DB_OK);
    }
    /* End of partition group call just gets passed down. */
    if (function & FUNC_EOGROUP)
    {
	status = (*out_node->qen_func)(out_node, qef_rcb, dsh, FUNC_EOGROUP);
	return(E_DB_OK);
    }


    /* Check for cancel, context switch if not MT */
    CScancelCheck(dsh->dsh_sid);
    if (QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) == E_DB_ERROR)
	return (E_DB_ERROR);

    /* If the trace point qe90 is turned on then gather cpu and dio stats */
    if (dsh->dsh_qp_stats)
    {
	qen_bcost_begin(dsh, &timerstat, qen_status);
    }

    if (node->node_qen.qen_sort.sort_dups == DMR_NODUPLICATES &&
	ult_check_macro(&qef_cb->qef_trace, 92, &val1, &val2))
	    heap_sort = FALSE;

    if (ult_check_macro(&qef_cb->qef_trace, 94, &val1, &val2))
    {
	no_qef = TRUE;
	qen_status->node_u.node_sort.node_sort_status = QEN9_DMF_SORT;
    }

#ifdef xDEBUG
	(VOID) qe2_chk_qp(dsh);
	if (dmr_get == (PTR)0 || dmr_get == (PTR)1)
	{
	    dsh->dsh_error.err_code = E_QE1005_WRONG_JOIN_NODE;
	    return (E_DB_ERROR);
	}
#endif

    for (;;)	    /* to break off in case of error */
    {
loop_reset:
	if (reset && qen_status->node_status != QEN0_INITIAL)
	{
	    /* Dump old data in the sorter:
	    ** For QEF in-memory sort, Close the sort buffer memory stream */
	    if (qen_status->node_u.node_sort.node_sort_status == QEN0_QEF_SORT)
	    {
		status = qen_u33_resetSortFile( shd, dsh );
		if (status != E_DB_OK)	{ break; }

		qen_status->node_status = QEN0_INITIAL;
	    }	

	    /* For DMF sort, call DMF to dump data in the sorter. */
	    else 
	    {
		dmt_cb = (DMT_CB *)cbs[node->node_qen.qen_sort.sort_create];
		if (dmt_cb->dmt_record_access_id == (PTR)NULL)
		{
		    /*
		    ** A commit has closed the sort hold file. It should
		    ** be initialised.
		    */
		    status = E_DB_OK ;
	            qen_status->node_status = QEN0_INITIAL;
		}
		else
		{	
		    dmr_load->dmr_flags_mask = 0;
		    status = dmf_call(DMR_DUMP_DATA, dmr_load);
		    if (status != E_DB_OK)
		    {
		        dsh->dsh_error.err_code = dmr_load->error.err_code;
		        break;
		    }
		    dmr_load->dmr_flags_mask = (node->node_qen.qen_sort.sort_dups | DMR_HEAPSORT);
		    dmr_load->dmr_count = 1;    /* reset for subsequent loads */
		    qen_status->node_status = QEN1_EXECUTED;
		}
	    }

	}

	rowno = node->node_qen.qen_sort.sort_mat->qen_output;
	ade_excb = node_xaddrs->qex_otmat;

	/* If this is the first time execution, or if the node is reset, 
	** initialize the sorter. If this is not, just go get a tuple.
	*/
	if (qen_status->node_status == QEN0_INITIAL || 
	    qen_status->node_status == QEN1_EXECUTED)
	{
	    if (qen_status->node_status == QEN0_INITIAL)
	    {
		if (qen_status->node_u.node_sort.node_sort_status == QEN0_QEF_SORT)
		{
		    status = qes_init(dsh, shd, node, rowno,
				node->node_qen.qen_sort.sort_dups);
		    if (status != E_DB_OK)
		    {
			if (dsh->dsh_error.err_code == E_QE000D_NO_MEMORY_LEFT)
			{
			    /* out of space, convert it to DMF sort */
			    qen_status->node_u.node_sort.node_sort_status = QEN9_DMF_SORT;
			    status = E_DB_OK;
			}
			else
			{
			    break;
			}
		    }
		}

		if (qen_status->node_u.node_sort.node_sort_status == QEN9_DMF_SORT)
		{
		    status = qen_s_dump(shd, dsh, node, 
				dmr_get, rowno, heap_sort, no_qef);
		    if (status != E_DB_OK)
			break;
		}
		qen_status->node_status = QEN1_EXECUTED;
	    }

	    /* Common code for QEN0_INITIAL and QEN1_EXECUTED */

	    /* Get dm_mdata ready for DMF loading  */
	    dm_mdata.data_address = dsh->dsh_row[rowno];
	    dm_mdata.data_size = dsh->dsh_qp_ptr->qp_row_len[rowno];
	    dmr_load->dmr_mdata = &dm_mdata;

            dsh->dsh_qp_status |= DSH_SORT;

	    /* Get rows from the underneath node and append them to the 
	    ** sorter */
	    for (;;)
	    {
		/* fetch rows */
		status = (*out_node->qen_func)(out_node, qef_rcb, dsh,
				(out_reset) ? FUNC_RESET : NO_FUNC);
		out_reset = FALSE;
		if (status != E_DB_OK)
		{
		    /* the error.err_code field in qef_rcb should already be
		    ** filled in.
		    */
		    if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			if (node->qen_flags & QEN_PART_SEPARATE)
			    qen_status->node_u.node_sort.node_sort_flags |=
					QEN1_SORT_LAST_PARTITION;
					/* show last group */
			status = E_DB_OK;
		    }
		    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION &&
			!(node->qen_flags & QEN_PART_SEPARATE))
		    {
			/* End of rows in partitioning group. Start reading
			** next group or return 00A5 up to caller. */
			reset = out_reset = TRUE;
			goto loop_reset;
		    }
		    if (dsh->dsh_error.err_code == E_QE00A5_END_OF_PARTITION)
		    {
			status = E_DB_OK;
		    }

		    break;
		}
		
		/* project the attributes into sort tuple row */
		status = qen_execute_cx(dsh, ade_excb);
		if (status != E_DB_OK)
		    break;

		/* append the sort tuple into the sorter - note periodic
		** differences between heap sort and trace point mandated 
		** insert sort */
		if (qen_status->node_u.node_sort.node_sort_status == QEN0_QEF_SORT)
		{
		    if (heap_sort) status = qes_put( dsh, shd);   
		     else status = qes_insert(dsh, shd,
                         node->node_qen.qen_sort.sort_cmplist,
                         node->node_qen.qen_sort.sort_sacount);

		    if (status != E_DB_OK && 
			dsh->dsh_error.err_code == E_QE000D_NO_MEMORY_LEFT)
		    {
			/* out of space, convert it to DMF sort */
			qen_status->node_u.node_sort.node_sort_status = QEN9_DMF_SORT;
			status = qen_s_dump(shd, dsh, node, 
					dmr_get, rowno, heap_sort, no_qef);
		    }
		    if (status != E_DB_OK)
			break;
		}

		if (qen_status->node_u.node_sort.node_sort_status == QEN9_DMF_SORT)
		{
		    status = dmf_call(DMR_LOAD, dmr_load);
		    if (status != E_DB_OK)
		    {
			dsh->dsh_error.err_code = dmr_load->error.err_code; 
			break;
		    }
		}
	    }
	    if (status != E_DB_OK)
		break;

	    /* End of loading loop, start sorting */
	    if (qen_status->node_u.node_sort.node_sort_status == QEN0_QEF_SORT)
	    {
		if (heap_sort)
                {
                    status = qes_sorter(dsh, shd,
                         node->node_qen.qen_sort.sort_cmplist,
                         node->node_qen.qen_sort.sort_sacount); 
                    if (status != E_DB_OK &&
                        dsh->dsh_error.err_code == E_QE000D_NO_MEMORY_LEFT)
                    {
                        /* out of space, convert it to DMF sort */
                        qen_status->node_u.node_sort.node_sort_status = QEN9_DMF_SORT;
                        status = qen_s_dump(shd, dsh, node,
					dmr_get, rowno, heap_sort, no_qef);
                    }
                    if (status != E_DB_OK)
                        break;
		}
		else qes_endsort(dsh, shd);

	    }
	    else    /* DMF */
	    {
		/* Tell DMF that there are no more rows */
		dmr_load->dmr_flags_mask = DMR_ENDLOAD;
		status = dmf_call(DMR_LOAD, dmr_load);
		if (status != E_DB_OK)
		{
		    dsh->dsh_error.err_code = dmr_load->error.err_code;
		    break;
		}
	    }
	    /* Mark the node is ready to return tuples */
	    qen_status->node_status = QEN3_GET_NEXT_INNER;
	}

	/* Increment the count of rows that this node has returned */
	qen_status->node_rcount++;
	dsh->dsh_error.err_code = 0;

	/* print tracing information DO NOT xDEBUG THIS */
	if (node->qen_prow != NULL &&
		(ult_check_macro(&qef_cb->qef_trace, 100+node->qen_num,
				    &val1, &val2) ||
		    ult_check_macro(&qef_cb->qef_trace, 99,
				    &val1, &val2)
		)
	    )
	{
	    if (status == E_DB_OK)
	    {
		status = qen_print_row(node, qef_rcb, dsh);
		if (status != E_DB_OK)
		{
		    return(status);
		}
	    }
	}

	break;
    }	/* end of error-break loop */

#ifdef xDEBUG
    (VOID) qe2_chk_qp(dsh);
#endif

    if (dsh->dsh_qp_stats)
    {
	qen_ecost_end(dsh, &timerstat, qen_status);
    }

    dsh->dsh_qp_status &= ~DSH_SORT;

    return (status);
}


/*
**	04-may-1992 (bryanp)
**	    Changed dmt_location from a DB_LOC_NAME to a DM_DATA.
**	06-mar-1996 (nanpr01)
**	     Set the pagesize in DMT_CB from sort_pagesize.
**       1-Nov-2002 (hanal04) Bug 109013 INGSRV 1986
**          Calls to DMR_LOAD require SORT or NOSORT to be explicitly set
**          (See change 438296). Though this is being added in qes_sort()
**          I have included the setting here to maintain  code consistency.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**	1-feb-2008 (dougi)
**	    Add tp qe94 to disable QEF sort.
*/


static	DB_STATUS
qen_s_dump(
QEN_SHD		   *shd,
QEE_DSH		   *dsh,
QEN_NODE           *node,
DMR_CB		   *dmr_get,
i4		   rowno,
bool		   heap_sort,
bool		   no_qef )
{
    PTR		*cbs = dsh->dsh_cbs;
    DMT_CB	*dmt = (DMT_CB *)cbs[node->node_qen.qen_sort.sort_create];
    DMR_CB	*dmr_load = (DMR_CB*)cbs[node->node_qen.qen_sort.sort_load];
    DMT_CHAR_ENTRY	char_array;
    DB_STATUS	status;


    for (;;)	    /* to break off in case of error */
    {
	/* Initialize the DMT_CB for the sorter table */
	dmt->dmt_flags_mask = DMT_LOAD | DMT_DBMS_REQUEST;
	dmt->dmt_db_id = dsh->dsh_qefcb->qef_rcb->qef_db_id;
	dmt->dmt_tran_id = dsh->dsh_dmt_id;
	MEmove(8, (PTR) "$default", (char) ' ', 
	    sizeof(DB_LOC_NAME),
	    (PTR) dmt->dmt_location.data_address);
	dmt->dmt_location.data_in_size = sizeof(DB_LOC_NAME);


	/* Initialize table attribute descriptors */
	dmt->dmt_attr_array.ptr_address = 
		    (PTR) node->node_qen.qen_sort.sort_atts;
	dmt->dmt_attr_array.ptr_in_count = 
		    node->node_qen.qen_sort.sort_acount;
	dmt->dmt_attr_array.ptr_size = sizeof (DMF_ATTR_ENTRY);

	/* Initialize the the sort key descriptors */
	dmt->dmt_key_array.ptr_address = 
		(PTR) node->node_qen.qen_sort.sort_satts;
	dmt->dmt_key_array.ptr_in_count = 
		node->node_qen.qen_sort.sort_sacount;
	dmt->dmt_key_array.ptr_size = sizeof (DMT_KEY_ENTRY);

        /*  Pass the page size */
        dmt->dmt_char_array.data_address = (PTR) &char_array;
        dmt->dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
        char_array.char_id = DMT_C_PAGE_SIZE;
        char_array.char_value = node->node_qen.qen_sort.sort_pagesize;
 

	/* Create the sorter table */
	status = dmf_call(DMT_CREATE_TEMP, dmt);
	if (status != E_DB_OK)
	{
	    if (dmt->error.err_code == E_DM0078_TABLE_EXISTS)
	    {
		dsh->dsh_error.err_code = E_QE0050_TEMP_TABLE_EXISTS;
		status = E_DB_ERROR;
	    }
	    else
	    {
		dsh->dsh_error.err_code = dmt->error.err_code;
	    }
	    break;
	}

	/* Open the sorter table */
	dmt->dmt_flags_mask = 0;
	dmt->dmt_sequence = dsh->dsh_stmt_no;
	dmt->dmt_access_mode = DMT_A_WRITE;
	dmt->dmt_lock_mode = DMT_X;
	dmt->dmt_update_mode = DMT_U_DIRECT;
        dmt->dmt_char_array.data_address = 0;
        dmt->dmt_char_array.data_in_size = 0; 

	status = qen_openAndLink(dmt, dsh);
	if (status != E_DB_OK)
	{
	    break;
	}

	/* Initialize the DMR_CB for loading the sorter */
	dmr_load->dmr_access_id = dmt->dmt_record_access_id;
	dmr_load->dmr_count = 1;
	dmr_load->dmr_flags_mask = (node->node_qen.qen_sort.sort_dups | DMR_HEAPSORT);
	dmr_load->dmr_s_estimated_records = node->qen_est_tuples;
	dmr_load->dmr_tid = 0;

	/* Initialize the DMR_CB for reading the sorter */
	dmr_get->dmr_access_id = dmt->dmt_record_access_id;
	dmr_get->dmr_data.data_address = dsh->dsh_row[rowno];
	dmr_get->dmr_data.data_in_size =  dsh->dsh_qp_ptr->qp_row_len[rowno];
	dmr_get->dmr_tid = 0;

	if (no_qef)
	    break;				/* leave if nothing there */

	/* If we have tuples in the in-memory sort buffer, load them into
	** DMF sorter table now and write current tuple. */
	if (heap_sort) status = qes_dump(dsh, shd, dmr_load);
	 else status = qes_dumploop(dsh, shd, dmr_load);
	break;
    }	/* end of error-break loop */
    return (status);
}
