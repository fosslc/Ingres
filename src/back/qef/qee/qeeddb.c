/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <cv.h>
#include    <tm.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <scf.h>
#include    <ulf.h>
#include    <gca.h>
#include    <ulm.h>
#include    <ulh.h>
#include    <adf.h>
#include    <ade.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qefkon.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <sxf.h> 
#include    <qefdsh.h>
#include    <qefscb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <ex.h>
#include    <qefprotos.h>

/**
**
**  Name: QEEDDB.C - DDB-specific environment management
**
**  Description:
**      The routines in this file provide management of the DSH structures. 
**
**	    qee_d1_qid	    - create a query id using the PID and a timestamp
**	    qee_d2_tmp	    - create names for temporary tables
**	    qee_d3_agg	    - create aggregate support structures
**	    qee_d6_malloc   - create space in QEF's memory
**	    qee_d7_rptadd   - save the identity of a REPEAT QUERY's DSH for
**			      reinitialization when the session terminates
**	    qee_d8_undefrpt - reinitialize REPEAT QUERY DSHs since session is
**	    qee_d9_undefall - reinitialize REPEAT QUERY DSHs since session is
**			      terminating
**
**  History:    $Log-for RCS$
**	20-feb-89 (carl)
**	    Created
**	02-may-89 (carl)
**	    changed qee_d2_tmp to generate temporary table with names 
**	    contianing no more than 10 characters as required by COMMON SQL
**	06-may-89 (carl)
**	    added qee_d4_tds to allocate space for RQF to construct the
**	    LDB's tuple descriptor for SCF in the cases where tuples are
**	    returned such as SELECT and CURSOR FETCH
**	27-jun-89 (carl)
**	    added qee_d6_malloc, qee_d7_rptadd, qee_d8_undefrpt and 
**	    qee_d9_undefall
**	10-aug-89 (carl)
**	    fixed qee_d7_rptadd to cure a lint problem;
**	    fixed qee_d8_undefrpt to return error if a dsh ptr is NULL
**	    at session termination time
**	22-aug-89 (carl)
**	    modified qee_d4_tds for new hetnet tuple descriptor policy
**	12-sep-90 (carl)
**	    1)	changed to call qec_m3_malloc instead of qec_malloc
**	    2)	removed obsolete hetnet routines qee_d4_tds and qee_d5_tds
**      17-apr-91 (dchan)
**          o_pp was indirectly declared as PTR **o_pp in qee_d6_malloc().
**          The declaration of o_pp was changed from 
**          **o_pp to *o_pp.  
**      19-mar-91 (dchan)
**          The compiler didn't like the 
**          construct " & (PTR) new_p ". Modified qee_d7_rptadd() 
**          by adding a temporary variable
**          of type PTR and point it to new_p. Pass temp_new_p
**          instead of new_p when calling qee_d6_malloc().
**	30-oct-91 (fpang)
**	    In qee_d8_undefrpt(), if ulh_getmember() returns successfully,
**	    always release the object.
**	    Fixes B40712 and B40752.
**	18-jun-92 (davebf)
**	    Updated for Sybil - removed calls to qecmem.c
**	08-dec-92 (anitap)
**	    Included <psfparse.h> for CREATE SCHEMA.
**	21-dec-92 (anitap)
**	    Prototyped qee_d1_qid(), qee_d2_tmp(), qee_d3_agg(), 
**	    qee_d6_malloc(), qee_d7_rptadd(), qee_d8_undefrpt(), 
**	    qee_d9_undefal1().
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes. 
**      14-sep-93 (smc)
**          Made session_id in ulh_name a CS_SID.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	24-feb-04 (inkdo01)
**	    Change dsh_ddb_cb from QEE_DDB_CB instance to ptr.
**      13-sep-06 (stial01)
**          qee_d3_agg() alloc aligned db_data memory for agg results (b116627)
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*{
** Name: QEE_D1_QID - Create query id for sending to LDB
**
** Description:
**      Generate a query id using the timestamp and STAR's process id.
**
** Inputs:
**      v_dsh_p				data segment header
**
** Outputs:
**      v_dsh_p				data segment header
**	    .dsh_ddb_cb
**		.qee_d4_given_qid	query id
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-feb-89 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prorotyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/

VOID
qee_d1_qid(
	QEE_DSH		*v_dsh_p)
{
    QEE_DDB_CB	    *qee_p = v_dsh_p->dsh_ddb_cb;
    DB_CURSOR_ID    *csr_p;
    SYSTIME	    tm_now;
    char	    hi_ascii[QEK_015_LEN],
		    lo_ascii[QEK_015_LEN],
		    pid_ascii[QEK_015_LEN],
		    temp[QEK_050_LEN + DB_CURSOR_MAXNAME];
    PID		    pid;		/* an i4 */


    PCpid(& pid);			/* get process (server) id */
    CVla(pid, pid_ascii);		/* convert to ascii */

    TMnow(& tm_now);

    csr_p = & qee_p->qee_d4_given_qid;
    csr_p->db_cursor_id[0] = tm_now.TM_secs;
    csr_p->db_cursor_id[1] = tm_now.TM_msecs;
    CVla(tm_now.TM_secs, hi_ascii);
    CVla(tm_now.TM_msecs, lo_ascii);
    STpolycat((i4) 4,			/* 4 constituent pieces */
	"dd", lo_ascii, hi_ascii, pid_ascii, temp);
    MEmove(STlength(temp), temp, ' ', 
	DB_CURSOR_MAXNAME, csr_p->db_cur_name);

    csr_p = & qee_p->qee_d5_local_qid;
    csr_p->db_cursor_id[0] = 0;
    csr_p->db_cursor_id[1] = 0;
    MEfill(DB_CURSOR_MAXNAME, ' ', (PTR) csr_p->db_cur_name);

    return;
}


/*{
** Name: QEE_D2_TMP - create temporary table and query id data structures
**
** Description:
**      Create DDB-specific data structures.
** 
** Inputs:
**	 qef_rcb			request block
**       dsh				data segment header
**	 ulm				memory stream out of which the
**					DSH is created.
**
** Outputs:
**      qef_rcb                         
**	    .error.err_code                  one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE000D_NO_MEMORY_LEFT
**				E_QE000F_OUT_OF_OTHER_RESOURCES
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    the DSH is expanded
**
** History:
**	13-feb-89 (carl)
**          extracted from QEE_CREATE
**	02-may-89 (carl)
**	    changed to generate temporary table with names contianing 
**	    no more than 10 characters as required by COMMON SQL
**	21-dec-92 (anitap)
**	    Prototyped.
*/


DB_STATUS
qee_d2_tmp(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	ULM_RCB		*ulm)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = dsh->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;	
    QEE_DDB_CB	    *qee_p = dsh->dsh_ddb_cb;
    DB_CURSOR_ID    *csr_p;


    if (ddq_p->qeq_d3_elt_cnt > 0)
    {
	SYSTIME	    tm_now,
		    tm_last;
	char	    hi_ascii[QEK_015_LEN],
		    lo_ascii[QEK_015_LEN],
/*
		    pid_ascii[QEK_015_LEN],
*/
		    temp[QEK_050_LEN+DB_MAXNAME];/* must be > DB_MAXNAME */
	DD_TAB_NAME    *name_p;
	i4	    *long_p;
/*
	PID	    pid;		** an i4 **
*/
	i4	    m, n;
	char	    *p, *q;
	u_i2	    hi_len,
/*
		    lo_len,
*/
		    tmp_len;

#define	QEE_10SQL_NAME_LEN    10


	/* allocate space for array of temporary-table names and 
	** generate their names */

	/* 1.  allocate array for DD_TAB_NAMEs */

	ulm->ulm_psize = ddq_p->qeq_d3_elt_cnt * sizeof(DD_TAB_NAME);
	if (status = qec_malloc(ulm))
	{
	    qef_rcb->error.err_code = ulm->ulm_error.err_code;
	    qed_u10_trap();
	    return(status);
	}

	qee_p->qee_d1_tmp_p = (DD_TAB_NAME *) ulm->ulm_pptr;
					/* ptr to array of DD_TAB_NAMEs */
	/* 2.  allocate array for i4 status */

	ulm->ulm_psize = ddq_p->qeq_d3_elt_cnt * sizeof(i4);
	if (status = qec_malloc(ulm))
	{
	    qef_rcb->error.err_code = ulm->ulm_error.err_code;
	    qed_u10_trap();
	    return(status);
	}

	qee_p->qee_d2_sts_p = (i4 *) 
	ulm->ulm_pptr; 	/* ptr to array of i4s */

	/* initialize both allocated arrays */

	name_p = qee_p->qee_d1_tmp_p;
	long_p = qee_p->qee_d2_sts_p;

	tm_last.TM_secs = 0;
	tm_last.TM_msecs = 0;
/*	
	PCpid(& pid);		** get process (server) id **
	CVla(pid, pid_ascii);	** convert to ascii **
*/
	for (n = 0; n < ddq_p->qeq_d3_elt_cnt; n++)
	{
	    /* 3.  generate temporary table name */

	    TMnow(& tm_now);
	
	    if (tm_now.TM_secs == tm_last.TM_secs)
	    {
		if (tm_now.TM_msecs <= tm_last.TM_msecs)
		    tm_now.TM_msecs = tm_last.TM_msecs + 1;
	    }
	    CVla(tm_now.TM_secs, hi_ascii);
	    CVla(tm_now.TM_msecs, lo_ascii);
		
	    hi_len = STlength(hi_ascii);

	    /* transpose the hi_ascii string to get lower digits */

	    p = hi_ascii;		    
	    q = hi_ascii + hi_len - 1;		/* backup past EOS */
	    for (m = 0; m < (i4) hi_len - 1; m++)	/* do length - 1 characters */
	    {
		*p = *q;
		p++;
		q--;
	    }

	    STpolycat((i4) 3,	/* 3 constituent pieces */
	    	"z", lo_ascii, hi_ascii, temp);

	    tmp_len = STlength(temp);

	    /* use at most 10 characters */

	    if (tmp_len > QEE_10SQL_NAME_LEN)
		tmp_len = QEE_10SQL_NAME_LEN;
	    
	    MEmove(tmp_len, temp, ' ', sizeof(DD_TAB_NAME), 
		   (char *)name_p);

	    STRUCT_ASSIGN_MACRO(tm_now, tm_last);
						/* save for comparison */
	    name_p++;		/* pt to next slot */

	    /* 4.  initialize status word for this table */

	    *long_p = QEE_00M_NIL;
	    long_p++;		/* pt to next status word */
	} /* for */
    } 
    else
    {
	qee_p->qee_d1_tmp_p = (DD_TAB_NAME *) NULL;
	qee_p->qee_d2_sts_p = (i4 *) NULL;
    }

    /* 5.  initialize */

    qee_p->qee_d3_status = QEE_00Q_NIL;

    if (! (qp_p->qp_status & QEQP_RPT))
    {
	/* initialize if not repeat query */

	csr_p = & qee_p->qee_d4_given_qid;
	csr_p->db_cursor_id[0] = 0;
	csr_p->db_cursor_id[1] = 0;
	MEfill(DB_CURSOR_MAXNAME, ' ', (PTR) csr_p->db_cur_name);
    }

    csr_p = & qee_p->qee_d5_local_qid;
    csr_p->db_cursor_id[0] = 0;
    csr_p->db_cursor_id[1] = 0;
    MEfill(DB_CURSOR_MAXNAME, ' ', (PTR) csr_p->db_cur_name);

    return(status);
}


/*{
** Name: QEE_D3_AGG - create aggregate structures
**
** Description:
**      Create DDB-specific aggregate structures.
** 
** Inputs:
**	 qef_rcb			request block
**       dsh				data segment header
**	 ulm				memory stream out of which the
**					DSH is created.
**
** Outputs:
**      qef_rcb                         
**	    .error.err_code                  one of the following
**				E_QE0000_OK
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0019_NON_INTERNAL_FAIL
**				E_QE0002_INTERNAL_ERROR
**				E_QE000D_NO_MEMORY_LEFT
**				E_QE000F_OUT_OF_OTHER_RESOURCES
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    the DSH is expanded
**
** History:
**	13-feb-89 (carl)
**          extracted from QEE_CREATE
**	21-dec-92 (anitap)
**	    Prototyped.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


DB_STATUS
qee_d3_agg(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	ULM_RCB		*ulm)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = dsh->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;	
    QEE_DDB_CB	    *qee_p = dsh->dsh_ddb_cb;
    QEQ_D1_QRY	    *sub_p;
    DB_DATA_VALUE   *dv1_p, *dv2_p;



    if (ddq_p->qeq_d4_total_cnt == 0)
	return(E_DB_OK);			/* no aggregate actions */

    qee_p->qee_d6_dynamic_cnt = 
	ddq_p->qeq_d4_total_cnt - ddq_p->qeq_d5_fixed_cnt;

    /* 1.  allocate array for DB_DATA_VALUEs */

    ulm->ulm_psize = (ddq_p->qeq_d4_total_cnt + 1) * sizeof(DB_DATA_VALUE);
						/* 1-based */
    if (status = qec_malloc(ulm))
    {
	qef_rcb->error.err_code = ulm->ulm_error.err_code;
	qed_u10_trap();
	return(status);
    }

    qee_p->qee_d7_dv_p = (DB_DATA_VALUE *) ulm->ulm_pptr;
						/* ptr to array */

    /* 2.  copy user's parameters if any */

    if (ddq_p->qeq_d5_fixed_cnt > 0)
    {
	i4		    m;


	dv1_p = ddq_p->qeq_d6_fixed_data;
	dv2_p = qee_p->qee_d7_dv_p + 1;		/* 1-based */

	for (m = 0; m < ddq_p->qeq_d5_fixed_cnt; ++m)
	{
	    STRUCT_ASSIGN_MACRO(*dv1_p, *dv2_p);
	    ++dv1_p;
	    ++dv2_p;
	}	
    }

    if (qee_p->qee_d6_dynamic_cnt > 0)
    {
	QEF_AHD	    *act_p = qp_p->qp_ahd;
	i4	    maxbind = 0,
		    aggcnt,
		    ptrcnt,
		    i, j;
	char	    **pp;


	/* 3.  allocate space for array of ptrs to aggregate spaces */

	ptrcnt = ddq_p->qeq_d4_total_cnt + 1;
	ulm->ulm_psize = ptrcnt * sizeof(char *);
					/* extra ptr space for safety */
	if (status = qec_malloc(ulm))
	{
	    qef_rcb->error.err_code = ulm->ulm_error.err_code;
	    qed_u10_trap();
	    return(status);
	}

	qee_p->qee_d8_attr_pp = (char **) ulm->ulm_pptr;
					/* ptr to ptrs to attribute space */
	pp = qee_p->qee_d8_attr_pp;
	for (i = 0; i < ptrcnt; i++, pp++)
	    *pp = (char *) NULL;

	pp = qee_p->qee_d8_attr_pp + 1;	/* initialize as 1-based */

	/* 4.  traverse the action list to allocate space for aggregates */

	aggcnt = 0;
	for (i = 0; i < qp_p->qp_ahd_cnt && act_p; ++i)
	{
	    char	    *attr_p;
	    i4		     curaggsize = 0;
	
	    
	    if (act_p->ahd_atype == QEA_D6_AGG)
	    {

		/* 5.  allocate space to hold intermediate results 
		**     for current aggregate action */
	    
		sub_p = & act_p->qhd_obj.qhd_d1_qry;
		if (sub_p->qeq_q6_col_cnt > maxbind)
		    maxbind = sub_p->qeq_q6_col_cnt;	
					/* record maximum bind space */
		aggcnt += sub_p->qeq_q6_col_cnt;

		ulm->ulm_psize = sub_p->qeq_q10_agg_size;
		if (status = qec_malloc(ulm))
		{
		    qef_rcb->error.err_code = ulm->ulm_error.err_code;
		    qed_u10_trap();
		    return(status);
		}

		*pp = (char *) ulm->ulm_pptr;
					/* set ptr to attribute space */
		/* 6.  set up DB_DATA_VALUE ptr for current aggregate action */

		dv1_p = sub_p->qeq_q9_dv_p;
					/* ptr to expected DB_DATA_VALUEs */
		dv2_p = qee_p->qee_d7_dv_p + sub_p->qeq_q11_dv_offset;
					/* ptr into DB_DATA_VALUE array */
		curaggsize = 0;
		attr_p = *pp;
		for (j = 0; j < sub_p->qeq_q6_col_cnt; ++j)
		{
		    STRUCT_ASSIGN_MACRO(*dv1_p, *dv2_p);
		    dv2_p->db_data = (PTR) attr_p;
		    attr_p += DB_ALIGN_MACRO(dv1_p->db_length);
		    curaggsize += DB_ALIGN_MACRO(dv1_p->db_length);
		    ++dv1_p;
		    ++dv2_p;
		}	
		if (curaggsize != sub_p->qeq_q10_agg_size)
		{
		    qed_u10_trap();
		    status = qed_u2_set_interr(E_QE1995_BAD_AGGREGATE_ACTION,
			& qef_rcb->error);
		    return(status);
		}
		++pp;			/* advance to pt to next slot */
	    }
	    act_p = act_p->ahd_next;	/* advance */
	}

	if (aggcnt != qee_p->qee_d6_dynamic_cnt
	    ||
	    maxbind <= 0)
	{
	    qed_u10_trap();
	    status = qed_u2_set_interr(E_QE1999_BAD_QP,
			& qef_rcb->error);
	    return(status);
	}
	/* 7.  allocate max space for RQB_BIND array */

	ulm->ulm_psize = maxbind * sizeof(RQB_BIND);
	if (status = qec_malloc(ulm))
	{
	    qef_rcb->error.err_code = ulm->ulm_error.err_code;
	    qed_u10_trap();
	    return(status);
	}

	qee_p->qee_d9_bind_p = (PTR) ulm->ulm_pptr;
					/* ptr to RQR_BIND array */
    }

    return(status);
}


/*{
** Name: qee_d6_malloc - create space in ULM
**
** Description:
**      This routine creates space in QEF's memory stream. 
**
** Inputs:
**	i_space				amount wanted
**      qef_rcb                         request block
**	    .qef_cb			session control block
**
** Outputs:
**      o_pp				ptr to ptr
**	qef_rcb
**	    .error.err_code		one of the following
**				E_QE0000_OK
**				E_QE0002_INTERNAL_ERROR
**				E_QE000D_NO_MEMORY
**
**	Returns:
**	    E_DB_{OK, ERROR}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-jun-89 (carl)
**	    adapted 
**      17-apr-91 (dchan)
**          o_pp was indirectly declared as PTR **o_pp.
**          The declaration of o_pp was changed from 
**          **o_pp to *o_pp.  
**	21-dec-92 (anitap)
**	    Prototyped.
**          
*/


DB_STATUS
qee_d6_malloc(
	i4		i_space,
	QEF_RCB		*qef_rcb,
	PTR		*o_pp)
{
    GLOBALREF	    QEF_S_CB *Qef_s_cb;
    QEF_CB	    *qef_cb = qef_rcb->qef_cb;
    DB_STATUS	    status = E_DB_OK;
    ULM_RCB	    ulm;


    o_pp = (PTR *) NULL;

    /* allocate space in QEF memory */

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);

    /* 1.  allocate from session's stream */

    ulm.ulm_streamid_p = &qef_cb->qef_c3_streamid;
    ulm.ulm_psize = i_space;
    if (status = qec_malloc(&ulm))
    {
	qef_rcb->error.err_code = ulm.ulm_error.err_code;
	return(status);
    }
    qef_cb->qef_c4_streamsize += i_space;
    o_pp =  (PTR *) ulm.ulm_pptr;


    return(E_DB_OK);
}


/*{
** Name: QEE_D7_RPTADD - Save the handle of a repeat query's DSH in save list
**
** Description:
**      The list hangs off the DDB session control block as qes_d12_rptdsh_p.
**
** Inputs:
**	qef_rcb
**	    ->qef_cb
**
**      i_qso_handle			handle to the DSH
**
** Outputs:
**      if a new repeat query, a QES_RPT_HANDLE structure is appended to the
**	list from qef_cb->qef_c2_ddb_ses.qes_d12_handle_p
**
**	Returns:
**	    E_DB_OK or E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-jun-89 (carl)
**          adapted
**	10-aug-89 (carl)
**	    fixed qee_d7_rptadd to cure a lint problem
**      19-mar-91 (dchan)
**          The compiler didn't like the 
**          construct " & (PTR) new_p ". Added a temporary variable
**          of type PTR and point it to new_p before calling qee_d6_malloc().
**	21-dec-92 (anitap)
**	    Prototyped. 
*/


DB_STATUS
qee_d7_rptadd(
	QEF_RCB		*qef_rcb)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;
    PTR		    handle;
    QES_RPT_HANDLE  *hand_p,
		    *new_p;
    QSF_RCB	    qsf_rcb;
    i4		    i;
    PTR             temp_new_p;

    /* 1.  obtain handle if not already known */

    if (qef_rcb->qef_qso_handle == (PTR) NULL)
    {
	qsf_rcb.qsf_next = (QSF_RCB *)NULL;
	qsf_rcb.qsf_prev = (QSF_RCB *)NULL;
	qsf_rcb.qsf_length = sizeof(QSF_RCB);
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_sid = qef_rcb->qef_cb->qef_ses_id;
	qsf_rcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
	qsf_rcb.qsf_obj_id.qso_lname = sizeof (DB_CURSOR_ID);
	MEcopy((PTR)& qef_rcb->qef_qp, sizeof (DB_CURSOR_ID), 
	    (PTR) qsf_rcb.qsf_obj_id.qso_name);
	qsf_rcb.qsf_lk_state = QSO_SHLOCK;
	status = qsf_call(QSO_GETHANDLE, & qsf_rcb);
	if (status)
	{
	    if (qsf_rcb.qsf_error.err_code == E_QS0019_UNKNOWN_OBJ)
		qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
	    else
		qef_rcb->error.err_code = E_QE0014_NO_QUERY_PLAN;
	    return(status);
	}
	handle = qsf_rcb.qsf_obj_id.qso_handle;
    }
    else
	handle = qef_rcb->qef_qso_handle;

    /* 2. search for the existence of this handle in list */

    for (i = 0, hand_p = dds_p->qes_d12_handle_p; 
	 i < dds_p->qes_d11_handle_cnt; ++i)
    {
	if (handle == hand_p->qes_3_qso_handle)
	    return(E_DB_OK);			/* handle already registered */
	else
	{
	    /* advance to next repeat query handle */

	    if (hand_p->qes_2_next != NULL)
		hand_p = hand_p->qes_2_next;
	}
    }

    /* 3.  allocate structure for this new handle */

    status = qee_d6_malloc(
			(i4) sizeof(QES_RPT_HANDLE),
			 qef_rcb,
			  &temp_new_p);
    new_p = (QES_RPT_HANDLE *)temp_new_p;

    if (status)
	return(status);

    /* 4.  initialize handle structure and insert into list */
	
    new_p->qes_1_prev = NULL;
    new_p->qes_2_next = NULL;
    new_p->qes_3_qso_handle = handle;

    if (dds_p->qes_d11_handle_cnt == 0)
    {
	/* this is the first one in the list */
	
	dds_p->qes_d12_handle_p = (QES_RPT_HANDLE *) new_p;
	dds_p->qes_d11_handle_cnt = 1;		/* set count */
    }
    else
    {
	/* insert at head of list */

	new_p->qes_2_next = dds_p->qes_d12_handle_p;
	dds_p->qes_d12_handle_p = (QES_RPT_HANDLE *) new_p;	
						/* new first handle */
	dds_p->qes_d11_handle_cnt += 1;		/* increment count */
    }

    return(E_DB_OK);
}


/*{
** Name: QEE_D8_UNDEFRPT - Reinitialize given REPEAT QUERY DSH since session is
**			   terminating
**
** Description:
**      
**
** Inputs:
**	qef_rcb
**	    ->qef_cb
**		.qef_c2_ddb_ses
**	i_handle_p			repeat query handle
**  
** Outputs:
**      Related DSH's query state is undefined with the effect that following
**	sessions will have to redefine this repeat queryt to the LDB
**
**	Returns:
**	    E_DB_OK or E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-jun-89 (carl)
**          adapted 
**	10-aug-89 (carl)
**	    fixed qee_d8_undefrpt to return error if a dsh ptr is NULL
**	    at session termination time
**	30-oct-91 (fpang)
**	    If ulh_getmember() returns successfully, always release the
**	    object. 
**	21-dec-92 (anitap)
**	    Prototyped.
*/


DB_STATUS qee_d8_undefrpt(
	QEF_RCB	    *qef_rcb,
	PTR	    i_qso_handle)
{
    DB_STATUS	    
		    status = E_DB_OK,
		    sav_status = E_DB_OK;
    DB_ERROR
		    sav_err;
    i4	    err;
    GLOBALREF QEF_S_CB *Qef_s_cb;
    QEE_DSH	    *dsh_p   = (QEE_DSH *)NULL;
    QEF_QP_CB	    *qp;
    bool	    have_qp = FALSE;
    QSF_RCB	    qsf_rcb;
    ULM_RCB	    ulm;
    ULH_RCB	    ulh_rcb;

    struct
    {
	PTR	qso_handle;
	CS_SID	session_id;
    } ulh_name;



    /* 1.  acquire the QP using the handle */


    qsf_rcb.qsf_next = (QSF_RCB *)NULL;
    qsf_rcb.qsf_prev = (QSF_RCB *)NULL;
    qsf_rcb.qsf_length = sizeof(QSF_RCB);
    qsf_rcb.qsf_type = QSFRB_CB;
    qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
    qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rcb.qsf_sid = qef_rcb->qef_cb->qef_ses_id;
    ulh_rcb.ulh_hashid = Qef_s_cb->qef_ulhid;

    qsf_rcb.qsf_obj_id.qso_handle = i_qso_handle;

    qsf_rcb.qsf_lk_state = QSO_SHLOCK;
    status = qsf_call(QSO_LOCK, &qsf_rcb);
    if (status)
    {
	qef_rcb->error.err_code = E_QE0019_NON_INTERNAL_FAILURE;
	return(status);
    }

    have_qp = TRUE;
    qp = (QEF_QP_CB *) qsf_rcb.qsf_root;

    /* For sharable QP, sometimes the QP may have been destroyed by
    ** another session.  So check for a NULL root.  If so, release the
    ** lock and return. */

    if (qp == (QEF_QP_CB *) NULL)
    {
	qsf_call(QSO_UNLOCK, &qsf_rcb);
	return(E_DB_OK);
    }

    /* 2.  gotten the QP, acquire the DSH in ULH's cache */

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);

    ulh_name.qso_handle = qsf_rcb.qsf_obj_id.qso_handle;
    ulh_name.session_id = qef_rcb->qef_cb->qef_ses_id;

    status = ulh_getmember(&ulh_rcb, (unsigned char *) &qp->qp_id, 
		    (i4)sizeof(DB_CURSOR_ID),  (unsigned char *) & ulh_name, 
		    (i4)sizeof(ulh_name), ULH_DESTROYABLE, 0);

    if (DB_FAILURE_MACRO(status))
    {
	qef_error(ulh_rcb.ulh_error.err_code, 0L, status, & err,
		&qef_rcb->error, 0);
	qef_rcb->error.err_code = E_QE0102_CATALOG_DSH_OBJECT;
    }
    else
    {
	dsh_p = (QEE_DSH *)(ulh_rcb.ulh_object->ulh_uptr);
	ulm.ulm_streamid_p = &ulh_rcb.ulh_object->ulh_streamid;

	if (dsh_p != (QEE_DSH *)NULL)
	{
	    /* 3.  reset the defined state of the REPEAT QUERY in the DSH */

	    dsh_p->dsh_ddb_cb->qee_d3_status &= ~QEE_03Q_DEF;

	    /* 4.  must release the acquired DSH */

	    ulh_rcb.ulh_object = (ULH_OBJECT *)dsh_p->dsh_handle;
	}
	status = ulh_release(& ulh_rcb);
	if (status != E_DB_OK)
	{
	    qef_error(ulh_rcb.ulh_error.err_code, 0L, status, & err,
	    	      &qef_rcb->error, 0);
	    qef_rcb->error.err_code = E_QE0104_RELEASE_DSH_OBJECT;
	}

	/* must fall thru to release the QP */
    }

    /* 5.  release the QP */
    
    if (have_qp)
    {
	qsf_call(QSO_UNLOCK, & qsf_rcb);
    }
    if (status == E_DB_OK && sav_status)
    {
	status = sav_status;
	STRUCT_ASSIGN_MACRO(sav_err, qef_rcb->error);
    }
    return(status);
}

/*{
** Name: QEE_D9_UNDEFALL - Reinitialize all REPEAT QUERY DSHs since session is
**			   terminating
**
** Description:
**      
**
** Inputs:
**	qef_rcb
**	    ->qef_cb
**		.qef_c2_ddb_ses
**		    .qes_d12_handle_p	    list of repeat query handle(s)
**  
** Outputs:
**      Above handles released and qef_cb->qef_c2_ddb_ses.qes_d12_handle_p set
**	to NULL
**
**	Returns:
**	    E_DB_OK or E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-jun-89 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prototyped.
*/

DB_STATUS
qee_d9_undefall(
	QEF_RCB		*qef_rcb)
{
    DB_STATUS	    status = E_DB_OK,
		    sav_status = E_DB_OK;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;
    PTR		    qso_handle;
    QES_RPT_HANDLE  *hand_p;
    i4		    i;


    for (i = 0, hand_p = dds_p->qes_d12_handle_p; 
	 hand_p != (QES_RPT_HANDLE *) NULL; 
	 ++i, hand_p = hand_p->qes_2_next)
    {
	status = qee_d8_undefrpt(qef_rcb, hand_p->qes_3_qso_handle);
	if (status)
	{
	    sav_status = status;
	    STRUCT_ASSIGN_MACRO(qef_rcb->error, sav_error);
	}
    }

    if (status == E_DB_OK && sav_status)
    {
	status = sav_status;
	STRUCT_ASSIGN_MACRO(sav_error, qef_rcb->error);
    }
    return(status);
}

