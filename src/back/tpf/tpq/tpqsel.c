/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>
#include    <tpfproto.h>

/**
**  Name: TPQSEL.C - Utility routines for retrieving from catalogs.
**
**  Description:
**	    tpq_s1_select   - send a SELECT statement to an LDB
**	    tpq_s2_fetch    - read a tuple from data stream
**	    tpq_s3_flush    - flush the data stream
**	    tpq_s4_prepare  - tpq_s1_select + tpq_s2_fetch
**
**
**  History:    $Log-for RCS$
**	21-oct-89 (carl)
**          adapted
**	07-jul-90 (carl)
**	    changed tpq_s1_select to use extended definition of catalog 
**	    iidd_ddb_dxldbs
**	03-oct-90 (fpang)
**	    Numerous casting changes for 64 sun4 unix port.
**	09-oct-90 (carl)
**	    process trace points
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	10-oct-93 (tam)
**	    Use 'CAP_SLAVE2PC' instead of 'SLAVE2PC' in querying 
**	    iidbcapabilities. (b50446)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2001 (jenjo02)
**	    #include tpfproto.h for typing tpd_u2_err_internal macro.
**/


/*{
** Name: TPQ_S1_SELECT - send a SELECT statement to an LDB (CDB)
**
** Description:
**	This routine sends a SELECT statement on the designated catalog
**	to an LDB, holds the data stream for repeated tuple fetch.  
**
** Inputs:
**      v_tpr_p			    TPR_CB
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_1_can_id	    catalog id
**		.qcb_4_ldb_p	    ptr to CDB/LDB
** Outputs:
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_5_eod_b	    TRUE if no data, FALSE otherwise
**	
**	v_tpr_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	21-oct-89 (carl)
**          adapted
**	07-jul-90 (carl)
**	    changed to use extended definition of catalog iidd_ddb_dxldbs
**	09-oct-90 (carl)
**	    process trace points
**	25-mar-93 (barbara)
**	    Lookup in iitables based on 'iidd_ddb_dxlog' must search for
**	    that catalog's name in both lower case and upper case.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-oct-93 (tam)
**	    Use 'CAP_SLAVE2PC' instead of 'SLAVE2PC' in querying 
**	    iidbcapabilities. (b50446)
*/


DB_STATUS
tpq_s1_select(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status;
    TPD_SS_CB	    *ss_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    DD_LDB_DESC	    *ldb_p = v_qcb_p->qcb_4_ldb_p;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    RQB_BIND	    *bind_p = v_qcb_p->qcb_11_rqf_bind;
    i4		    can_id = v_qcb_p->qcb_1_can_id;
    bool            trace_qry_102 = FALSE,
                    trace_err_104 = FALSE;
    i4         i4_1,
                    i4_2;
    char	    where[200],
		    qrytxt[1000];


    if (ult_check_macro(& ss_p->tss_3_trace_vector, TSS_TRACE_QUERIES_102,
            & i4_1, & i4_2))
    {
        trace_qry_102 = TRUE;
    }

    if (ult_check_macro(& ss_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }
    /* 1.  set up SELECT query */

    switch (can_id)
    {
    case SEL_10_DXLOG_CNT:
	{
	    TPC_D1_DXLOG	*dxlog_p = v_qcb_p->qcb_3_ptr_u.u1_dxlog_p;


	    v_qcb_p->qcb_6_col_cnt = 1;		/* only 1 column in result tuple
						*/

	    /* set up column type and size for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & v_qcb_p->qcb_10_total_cnt;

	    STprintf(
		qrytxt,
		"select count(*) from iidd_ddb_dxlog;");
	}
	break;

    case SEL_11_DXLOG_ALL:
	{
	    TPC_D1_DXLOG	*dxlog_p = v_qcb_p->qcb_3_ptr_u.u1_dxlog_p;


	    v_qcb_p->qcb_6_col_cnt = TPC_01_CC_DXLOG_13;   /* 13 columns */

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_1_dx_id1;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_2_dx_id2;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxlog_p->d1_3_dx_name;
	    dxlog_p->d1_3_dx_name[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_4_dx_flags;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_5_dx_state;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxlog_p->d1_6_dx_create;
	    dxlog_p->d1_6_dx_create[DD_25_DATE_SIZE] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxlog_p->d1_7_dx_modify;
	    dxlog_p->d1_7_dx_modify[DD_25_DATE_SIZE] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_8_dx_starid1;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_9_dx_starid2;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxlog_p->d1_10_dx_ddb_node;
	    dxlog_p->d1_10_dx_ddb_node[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DD_256_MAXDBNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxlog_p->d1_11_dx_ddb_name;
	    dxlog_p->d1_11_dx_ddb_name[DD_256_MAXDBNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxlog_p->d1_12_dx_ddb_dbms;
	    dxlog_p->d1_12_dx_ddb_dbms[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxlog_p->d1_13_dx_ddb_id;

	    STprintf(
	    qrytxt,
	    "%s %s %s;",
	    "select dx_id1, dx_id2, dx_name, dx_flags, dx_state, dx_create, ",
	    "dx_modify, dx_starid1, dx_starid2, dx_ddb_node, dx_ddb_name, ",
	    "dx_ddb_dbms, dx_ddb_id from iidd_ddb_dxlog");
	}
	break;

    case SEL_20_DXLDBS_CNT_BY_DXID:
	{
	    TPC_D2_DXLDBS	*dxldbs_p = v_qcb_p->qcb_3_ptr_u.u2_dxldbs_p;


	    v_qcb_p->qcb_6_col_cnt = 1;		/* only 1 column in result tuple
						*/

	    /* set up column type and size for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & v_qcb_p->qcb_10_total_cnt;

	    STprintf(
		where,
		"where ldb_dxid1 = %d and ldb_dxid2 = %d",
		dxldbs_p->d2_1_ldb_dxid1,
		dxldbs_p->d2_2_ldb_dxid2);

	    STprintf(
		qrytxt,
		"select count(*) from iidd_ddb_dxldbs %s;",
		where);
	}
	break;

    case SEL_21_DXLDBS_ALL_BY_DXID:
	{
	    TPC_D2_DXLDBS	*dxldbs_p = v_qcb_p->qcb_3_ptr_u.u2_dxldbs_p;


	    v_qcb_p->qcb_6_col_cnt = TPC_02_CC_DXLDBS_11;   /* 11 columns */

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_1_ldb_dxid1;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_2_ldb_dxid2;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxldbs_p->d2_3_ldb_node;
	    dxldbs_p->d2_3_ldb_node[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DD_256_MAXDBNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxldbs_p->d2_4_ldb_name;
	    dxldbs_p->d2_4_ldb_name[DD_256_MAXDBNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxldbs_p->d2_5_ldb_dbms;
	    dxldbs_p->d2_5_ldb_dbms[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_6_ldb_id;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_7_ldb_lxstate;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_8_ldb_lxid1;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_9_ldb_lxid2;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dxldbs_p->d2_10_ldb_lxname;
	    dxldbs_p->d2_10_ldb_lxname[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dxldbs_p->d2_11_ldb_lxflags;

	    STprintf(
		where,
		"where ldb_dxid1 = %d and ldb_dxid2 = %d",
		dxldbs_p->d2_1_ldb_dxid1,
		dxldbs_p->d2_2_ldb_dxid2);

	    STprintf(
		qrytxt,
		"%s %s %s %s;",
		"select ldb_dxid1, ldb_dxid2, ldb_node, ldb_name, ",
		"ldb_dbms, ldb_id, ldb_lxstate, ldb_lxid1, ldb_lxid2, ",
		"ldb_lxname, ldb_lxflags from iidd_ddb_dxldbs ",
		where);
	}
	break;

    case SEL_30_CDBS_CNT:
	{
	    TPC_I1_STARCDBS *starcdbs_p = v_qcb_p->qcb_3_ptr_u.u3_starcdbs_p;


	    v_qcb_p->qcb_6_col_cnt = 1;		/* only 1 column in result tuple
						*/

	    /* set up column type and size for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & v_qcb_p->qcb_10_total_cnt;

	    STprintf(
		qrytxt,
		"select count(*) from iistar_cdbs;");
	}
	break;

    case SEL_31_CDBS_ALL:
	{
	    TPC_I1_STARCDBS *starcdbs_p = v_qcb_p->qcb_3_ptr_u.u3_starcdbs_p;


	    v_qcb_p->qcb_6_col_cnt = TPC_04_CC_STARCDBS_11;   /* 11 columns */

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_1_ddb_name;
	    starcdbs_p->i1_1_ddb_name[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_2_ddb_owner;
	    starcdbs_p->i1_2_ddb_owner[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_3_cdb_name;
	    starcdbs_p->i1_3_cdb_name[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_4_cdb_owner;
	    starcdbs_p->i1_4_cdb_owner[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_5_cdb_node;
	    starcdbs_p->i1_5_cdb_node[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_6_cdb_dbms;
	    starcdbs_p->i1_6_cdb_dbms[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_7_scheme_desc;
	    starcdbs_p->i1_7_scheme_desc[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_8_create_date;
	    starcdbs_p->i1_8_create_date[DD_25_DATE_SIZE] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = TPC_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) starcdbs_p->i1_9_original;
	    starcdbs_p->i1_9_original[TPC_8_CHAR_SIZE] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & starcdbs_p->i1_10_cdb_id;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & starcdbs_p->i1_11_cdb_cap;

	    STprintf(
		qrytxt,
		"select %s %s %s;",
		"ddb_name, ddb_owner, cdb_name, cdb_owner, cdb_node, cdb_dbms,",
		"scheme_desc, create_date, original, cdb_id, cdb_capability",
		"from iistar_cdbs");
	}
	break;

    case SEL_40_DBCAPS_BY_2PC:
	{
	    TPC_L1_DBCAPS	*dbcaps_p = v_qcb_p->qcb_3_ptr_u.u4_dbcaps_p;


	    v_qcb_p->qcb_6_col_cnt = TPC_03_CC_DBCAPS_2;   /* 2 columns */

	    /* set up column types and sizes for fetching with RQF */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dbcaps_p->l1_1_cap_cap;
	    dbcaps_p->l1_1_cap_cap[DB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) dbcaps_p->l1_2_cap_val;
	    dbcaps_p->l1_2_cap_val[DB_MAXNAME] = EOS;

	    STprintf(
		qrytxt,
		"%s %s;",
		"select cap_capability, cap_value from iidbcapabilities",
		"where cap_capability = 'CAP_SLAVE2PC'");
	}
	break;

    case SEL_60_TABLES_FOR_DXLOG:
	{
	    v_qcb_p->qcb_3_ptr_u.u0_ptr = (PTR) NULL;

	    v_qcb_p->qcb_6_col_cnt = 1;		/* only 1 column in result tuple
						*/

	    /* set up column type and size for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & v_qcb_p->qcb_10_total_cnt;

	    STprintf(
		qrytxt,
    "select count(*) from iitables where table_name = 'iidd_ddb_dxlog' \
or table_name = 'IIDD_DDB_DXLOG';");
	}
	break;

    case SEL_50_CLUSTER_FUNC:
    default:
	status = tpd_u2_err_internal(v_tpr_p);
	return(status);
    }

    if (trace_qry_102)
	tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, TPD_0TO_FE);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = (PTR) v_tpr_p->tpr_rqf;
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = ldb_p;			/* LDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;
    rqr_p->rqr_no_attr_names = TRUE;	/* internal query requires no column
					** names */

    /* 3.  execute CDB query */

    status = tpd_u4_rqf_call(RQR_NORMAL, rqr_p, v_tpr_p);
    if (status)
    {
	if (! trace_qry_102 && trace_err_104)
	    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, 
		TPD_0TO_FE);
    }
    v_qcb_p->qcb_5_eod_b = rqr_p->rqr_end_of_data;

    return(status);
}

/*{
** Name: TPQ_S2_FETCH - Read the next tuple from a sending LDBMS
**
** Description:
**      This routine reads the next tuple from a sending LDBMS.
**  
** Inputs:
**      v_tpr_p			    TPR_CB
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_1_can_id	    DDB/LDB catalog id
**		.qcb_4_ldb_p	    ptr to LDB
** Outputs:
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_5_eod_b	    TRUE if no data, FALSE otherwise
**	
**	v_tpr_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	21-oct-89 (carl)
**          adapted
*/


DB_STATUS
tpq_s2_fetch(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    /* 1. verify state */

    if (v_qcb_p->qcb_5_eod_b)			/* no data to read */
	return(E_DB_OK);

    /* 2.  set up to call RQF to obtain next tuple */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = v_tpr_p->tpr_rqf;   
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = v_qcb_p->qcb_4_ldb_p;
    rqr_p->rqr_col_count = v_qcb_p->qcb_6_col_cnt;
    rqr_p->rqr_bind_addrs = v_qcb_p->qcb_11_rqf_bind;
						/* must use invariant array */
    /* 3.  request 1 tuple */

    status = tpd_u4_rqf_call(RQR_T_FETCH, rqr_p, v_tpr_p);
    if (status)
    {
	/* must flush before returning */

	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, sav_error);
	ignore = tpq_s3_flush(v_tpr_p, v_qcb_p);
	STRUCT_ASSIGN_MACRO(sav_error, v_tpr_p->tpr_error);
	return(status);
    }

    v_qcb_p->qcb_5_eod_b = rqr_p->rqr_end_of_data;
    if (! v_qcb_p->qcb_5_eod_b)
	v_qcb_p->qcb_12_select_cnt++;

    return(E_DB_OK);
}


/*{
** Name: TPQ_S3_FLUSH - flush all unread tuples from sending LDB
**
** Description:
**      This routine flushes all unread tuples from the sending LDB.
**
** Inputs:
**      v_tpr_p			    TPR_CB
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qec_c2_ldb_p	    ptr to LDB
**		.qec_c4_eod_b	    FALSE 
** Outputs:
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qec_c4_eod_b	    FALSE 
**	
**	v_tpr_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	21-oct-89 (carl)
**          adapted
*/


DB_STATUS
tpq_s3_flush(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    /* 1. verify state */

    if (v_qcb_p->qcb_5_eod_b)			/* nothing to flush */
	return(E_DB_OK);

    v_qcb_p->qcb_5_eod_b = TRUE;

    /* 2.  set up to call RQF to flush */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = v_tpr_p->tpr_rqf;
    rqr_p->rqr_1_site = v_qcb_p->qcb_4_ldb_p;

    status = tpd_u4_rqf_call(RQR_T_FLUSH, rqr_p, v_tpr_p);
    return(status);
}


/*{
** Name: TPQ_S4_PREPARE - Equivalent to calling tpq_s1_select and then 
**			  tpq_s2_fetch.
**
** Description:
**      This routine sends a SELECT query and then if successful requests a
**  first tuple.
**  
** Inputs:
**      v_tpr_p			    TPR_CB
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_1_can_id	    DDB/LDB catalog id
**		.qcb_2_rqf_bind_p   ptr to RQF binding array
**		.qcb_4_ldb_p	    ptr to LDB
** Outputs:
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_5_eod_b	    TRUE if no data, FALSE otherwise
**	
**	v_tpr_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	21-oct-89 (carl)
**          adapted
*/


DB_STATUS
tpq_s4_prepare(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status;


    v_qcb_p->qcb_12_select_cnt = 0;

    /* 1.  send SELECT query */

    status = tpq_s1_select(v_tpr_p, v_qcb_p);
    if (status)
	return(status);


    if (! v_qcb_p->qcb_5_eod_b)			/* any data to read ? */
	status = tpq_s2_fetch(v_tpr_p, v_qcb_p);

    return(status);
}

