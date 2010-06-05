/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

/**
** Name: QEFPROTOS.H - Prototypes for external QEF functions
**
** Description:
**	This header file contains ANSI prototypes for all QEF functions
**	that are external to their own files but internal to QEF.  This
**	includes all external QEF functions except:
**
**		qef_call	Not prototyped because this is the facility's
**				entry point, visible outside QEF.
**
**		qef_error	Not included because it has a variable
**				number of arguments and so cannot be
**				prototyped.
**
** History: $Log-for RCS$
**	22-Dec-92 (rickh)
**	    Created the stub for this file.
**	22-dec-92 (anitap)
**	    Added external definitions for qef_handler() and qef_2handler().	
**	05-jan-93 (rickh)
**	    Added prototypes for QEN and QEQ.
**	08-jan-93 (davebf)
**	    Added prototypes for QEC, QED, and QEL
**	11-jan-93 (rickh)
**	    Added prototypes for QEK.  Removed qeq_p2_get_act, which is static.
**	15-jan-93 (rickh)
**	    Two new prototypes from QEQDDB.C:  qeq_d11_regproc,
**	    qeq_d12_regproc.
**	18-jan-93 (kwatts)
**	    New proto for qeq_ade_base (Smart Disk development)
**	15-jan-93 (rickh)
**	    Added qea_createIntegrity.
**	18-jan-93 (rickh)
**	    Added arguments to qeu_dbu.
**	18-jan-93 (anitap)
**	    Added qea_savecontext(), qeachkerror(), qea_cqsfobj(). Modified the
**	    declaration of qea_cschema().
**	    Added qeq_restore().
**	03-feb-93 (anitap)
**	    Added prototypes for functions in QEF. Changed qea_cschema() and
**	    qea_execimm() to qea_cshma() and qea_xcimm() to correspond to coding
**	    standards. Made qeq_restore() a static function.
**	    Removed some fields not needed in the declaration of qea_cqsfobj().
**	19-feb-93 (rickh)
**	    Added qea_reserveID.
**      04-mar-93 (anitap)
**	    Changed function names to conform to standard. Added prototype for
**	    qeq_qp_apropos().
**	12-mar-93 (rickh)
**	    Added qeu_tableOpen and qeu_findOrMakeDefaultID.
**	18-mar-93 (jhahn)
**	    Added qea_prc_insert() and qea_closetemp().
**	11-may-93 (anitap)
**	    Added qeq_updrow().
**	01-jun-93 (barbara)
**	    Added qel_a2_case_col() for Star
**	9-jun-93 (rickh)
**	    Added 2 new arguments to qeu_dbu.
**	24-jun-93 (robf)
**	    Added qeu_mac_access(), qeu_default_label(),
**	    qeu_clabaudit(), qeu_dlabaudit()
**	07-jul-93 (anitap)
**	    Added new arguments to qea_reserveID(), qea_schema(). 
**	    Removed QEF_DSH from argument list for qea_scontext().
**	    Added new arguments to qea_chkerror(), qea_cobj().
**	    Added prototype for qea_dobj().
**	20-jul-1993 (pearl)
**	    Bug 45912.  Change parameter to qeq_ksort(); pass pointer to dsh 
**	    instead of pointer to dsh_key.
**	21-jul-1993 (rog)
**	    Exception handlers return STATUS, not nat.
**      13-sep-93 (smc)
**          Changed sess_id in qeu_secaudit and session_id qeu_audit to CS_SID.
**	25-aug-93 (rickh)
**	    Added command argument to qea_chkerror.
**	26-aug-93 (jhahn)
**	    Added page_count to qeq_qp_apropos.
**	28-sep-93 (anitap)
**	    Added prototype for qea_dsh(). Made qea_dobj() an internal 
**	    function.
**	14-dec-93 (rickh)
**	    Added qen_u33_resetSortFile.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Added prototype declaration for qec_tprintf which has now been
**	    made portable by eliminating p1 .. p2 PTR paramters. The caller
**	    is now required to pass a string already formatted by STprintf.
**	    This is necessary because STprintf is a varargs function whose
**	    parameters can vary in size; the current code breaks on any
**	    platform where the STprintf size types are not the same (these
**	    are usually integers and pointers).
**	    This approach has been taken because varargs functions outside CL
**	    have been outlawed and using STprintf in client code is less
**	    cumbersome than trying to emulate varargs with extra size or
**	    struct parameters.
**	19-jan-94 (jhahn)
**	    Added qeq_release_dsh_resources.
**	20-jan-94 (rickh)
**	    Added qef_mo_register.
**      20-aug-95 (ramra01)
**          Added new function qen_origagg for simple aggregate opt.
**      06-feb-96 (johnst)
**          Bug #72202.
**          Changed qee_ade_base() arguments to match the interface of
**          qeq_ade_base() from which it was originally cloned off,
**          restoring the two {qen_base, num_bases} arguments instead of 
**          passing the higher level struct QEN_ADF, as (eric) first defined
**          it. The Smart Disk code in qeq_validate() needs to call both
**          qee_ade_base() and qeq_ade_base() to pass in the actual address
**          arrays and number of bases used in the Smart Disk case.
**	15-oct-96 (nick)
**	    qeq_release_dsh_resources() has no return value ...
**      06-mar-96 (nanpr01)
**          Changed the parameter of  qen_putIntoTempTable from i4  to
**	    QEN_TEMP_TABLE.
**	23-jan-97 (shero03)
**	    B79960: Externalize qeu_v_col_drop 
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_destroy(), qee_cleanup() prototypes.
**	19-Nov-1997 (jenjo02)
**	    Added prototype for qeq_unwind_dsh_resources().
**	24-nov-97 (inkdo01)
**	    Added prototypes for qes_putheap, qes_getheap.
**	01-Apr-1998 (hanch04)
**	    Added qef_session
**	21-jul-98 (inkdo01)
**	    Changed qen_position, qeq_ksort protocols to support topsort-less
**	    descending sorts.
**	28-oct-98 (inkdo01)
**	    Added qea_retrow for row-producing procedures.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	19-mar-99 (inkdo01)
**	    Added functions for hash join support.
**	jan-00 (inkdo01)
**	    Changes for hash join funcs.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Deleted qef_session() prototype, replaced by
**	    GET_QEF_CB macro.
**	24-Jan-2001 (jenjo02)
**	    Added qec_mfree() prototype.
**	23-apr-01 (inkdo01)
**	    Add hash aggregation external funcs + 1 more hash join func.
**	17-Apr-2001 (hweho01)
**	    Added prototype for qef_error(). Listed it as variable-arg 
**          function,
**	05-Mar-2002 (jenjo02)
**	    Prototyped QEU_CSEQ, QEU_ASEQ, QEU_DSEQ for 
**	    sequence generators.
**	18-mar-02 (inkdo01)
**	    Added prototype for qea_seqop to perform next/current value 
**	    sequence operations.
**      22-oct-2002 (stial01)
**          Added prototype for qet_xa_abort()
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          Added qeu_d_check_conix() which checks to see if the supplied
**          table/index has a constraint dependency.
**	20-nov-03 (inkdo01)
**	    Added "open_flag" parm for qen_func's (for parallel query
**	    processing).
**	18-dec-03 (inkdo01)
**	    Reluctantly adding QEE_DSH * as parm to all qen_ and qea_ funcs
**	    for parallel query project. Also changed "bool reset" to 
**	    "i4 function" to enable a more OO approach to action/node
**	    executors in future.
**	2-jan-04 (inkdo01)
**	    Change qeq_vopen() from static to EXTERN.
**	8-jan-04 (toumi01)
**	    Added function for exchange support.
**	22-jan-04 (inkdo01)
**	    Changed qeq_vopen back to static and introduced new function
**	    qeq_part_open to alloc DMR_CB/DMT_CB and open partition.
**	11-Feb-2004 (schka24)
**	    Change to qeu-dbu; added qeu_create_table.
**	23-feb-04 (inkdo01)
**	    Added DSH parms to various sort calls.
**	01-Jul-2004 (jenjo02)
** 	    Added function prototypes for qet_connect/disconnect.
**	16-Jul-2004 (schka24)
**	    Remove unused qeq_iterate.  Remove qee_create, now static.
**	20-Sep-2004 (jenjo02)
**	    Globalized qen_keyprep(), was static in qenorig.
**	3-Jun-2005 (schka24)
**	    Fix a couple old-style declarations to shut up the C compiler.
**	14-Nov-2005 (schka24)
**	    Remove indptr's from hash probe funcs.
**	6-Dec-2005 (kschendel)
**	    Kill off sorter skbuild.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code and reflects the
**	    situation since parallel query more completely.)
**	12-Jan-2006 (kschendel)
**	    Revise the infamous qen_u21_execute_cx.
**	29-may-06 (dougi)
**	    Added qef_procerr() to provide database procedure diagnostics.
**	26-Jun-2006 (jonj)
**	    Deleted qet_xa_abort, added qet_xa_prepare,
**	    qet_xa_commit, qet_xa_rollback, qet_xa_start, qet_xa_end.
**      13-mar-2006 (horda03) Bug 112560/INGSRV2880
**          Added temp_tbl parameter to qeu_d_check_conix, to signify whether
**          the table is a GLOBAL SESSION TEMPORARY.
**	11-Apr-2008 (kschendel)
**	    Add qualification-function setter.
**      22-may-2008 (huazh01)
**          remove qeu_qrule_by_name() as it is gone together with the 
**          removal of qen_dmr_cx(). This fixes bug 120415.
**	25-may-2008 (dougi)
**	    Add qen_tproc() for table procedures.
**	26-Jul-2007 (kschendel) SIR 122513
**	    Changes for generalized partition qualification.
**	8-Sep-2008 (kibro01) b120693
**	    Remove unused action parameter from qeq_part_open
**      27-May-2008 (gefei01)
**          Add input parameter isTproc to qee_fetch() and qeq_dsh()
**      09-Sep-2008 (gefei01)
**          Added prototypes for qen_tproc, qeq_dshtorcb and qeq_rcbtodsh.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Rename qef_error function to qefErrorFcn(),
**	    called by qef_error() bridge and qefError() macros.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	4-Jun-2009 (kschendel) b122118
**	    Add open-and-link, close-and-unlink.  Remove error returns from
**	    qe90 costing helpers.  Add qen-inner-sort so that qen-sort can
**	    look like all the others.  Remove some qee stuff that can be
**	    static.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Merge changes for hash join/agg improvements: re-type hash join
**	    utility funcs, add RLL compress / decompress, add qee hash
**	    reservation routines.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Merge put and load into put.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	12-Apr-2010 (gupsh01) SIR 123444
**	    Added support for ALTER TABLE RENAME table/columns.
*/

/*	QEC functions	*/

FUNC_EXTERN DB_STATUS
qec_initialize(
QEF_SRCB       *qef_srcb );

FUNC_EXTERN DB_STATUS
qec_shutdown(
QEF_SRCB       *qef_srcb );

FUNC_EXTERN DB_STATUS
qec_begin_session(
QEF_CB         *qef_cb );

FUNC_EXTERN DB_STATUS
qec_end_session(
QEF_CB       *qef_cb );

FUNC_EXTERN DB_STATUS
qec_alter(
QEF_CB             *qef_cb );

FUNC_EXTERN DB_STATUS
qec_debug(
QEF_DBG        *qef_dbg );

FUNC_EXTERN DB_STATUS
qec_trace(
DB_DEBUG_CB		*debug_cb );

FUNC_EXTERN DB_STATUS
qec_info(
QEF_CB       *qef_cb );

FUNC_EXTERN DB_STATUS
qec_malloc(
ULM_RCB	    *ulm_rcb );

FUNC_EXTERN DB_STATUS
qec_mopen(
ULM_RCB	    *ulm_rcb );

FUNC_EXTERN DB_STATUS
qec_mfree(
ULM_RCB	    *ulm_rcb );

FUNC_EXTERN VOID
qec_tprintf(
QEF_RCB		*qef_rcb,
i4		cbufsize,
char		*cbuf );

FUNC_EXTERN i4
qec_trimwhite(
i4	len,
char	*buf );

FUNC_EXTERN VOID
qec_pss(
QEF_CB         *qef_cb );

FUNC_EXTERN VOID
qec_pdsh(
QEE_DSH        *qee_dsh );

FUNC_EXTERN VOID
qec_pqep(
QEN_NODE       *qen_node );

FUNC_EXTERN VOID
qec_pqp(
QEF_QP_CB          *qef_qp );

FUNC_EXTERN void
qec_prnode(
QEN_NODE	   *node,
i4		   *number );

FUNC_EXTERN void
pratts(
i4		num_atts,
DMT_ATT_ENTRY	**atts );

FUNC_EXTERN VOID
qec_mfunc(
QEF_FUNC        qef_func[] ); 

/*	QED functions	*/

FUNC_EXTERN DB_STATUS
qed_c1_conn(
QEF_RCB       *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_c2_cont(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_c3_disc(
QEF_RCB       *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_e1_exec_imm(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_e4_exec_qry(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_p1_range(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_p2_set(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_q1_init_ddv(
QEF_SRCB       *qef_srcb );

FUNC_EXTERN DB_STATUS
qed_q2_shut_ddv(
QEF_SRCB       *qef_srcb );

FUNC_EXTERN DB_STATUS
qed_q3_beg_ses(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_q4_end_ses(
QEF_RCB       *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_q5_ldb_slave2pc(
DD_LDB_DESC	*v1_ldb_p,
QEF_RCB		*v2_qer_p );

FUNC_EXTERN DB_STATUS
qed_r1_query(
QEF_RCB       *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r2_fetch(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r3_flush(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r4_vdata(
QEF_RCB       *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r5_begin(
QEF_RCB	    *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r6_commit(
QEF_RCB	    *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r7_tp_rq(
QEF_RCB       *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_r8_diff_arch(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s0_sesscdb(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s1_iidbdb(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s2_ddbinfo(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s3_ldbplus(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s4_usrstat(
QEF_RCB         *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s5_ldbcaps(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s6_modify(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s7_term_assoc(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s8_cluster_info(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_s9_system_owner(
QEF_RCB		  *v_qer_p );

FUNC_EXTERN u_i4
qed_u0_trimtail(
char		*i_str1_p,
u_i4		i_len1,
char		*o_str2_p );

FUNC_EXTERN DB_STATUS
qed_u1_gen_interr(
DB_ERROR    *o_dberr_p );

FUNC_EXTERN DB_STATUS
qed_u2_set_interr(
DB_ERRTYPE  i_setcode,
DB_ERROR    *o_dberr_p );

FUNC_EXTERN DB_STATUS
qed_u3_rqf_call(
i4	i_call_id,
RQR_CB	*v_rqr_p,
QEF_RCB	*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_u4_qsf_acquire(
QEF_RCB		*i_qer_p,
QSF_RCB		*v_qsr_p );

FUNC_EXTERN DB_STATUS
qed_u5_qsf_destroy(
QEF_RCB		*i_qer_p,
QSF_RCB		*i_qsr_p );

FUNC_EXTERN DB_STATUS
qed_u6_qsf_root(
QEF_RCB		*i_qer_p,
QSF_RCB		*v_qsr_p );

FUNC_EXTERN DB_STATUS
qed_u7_msg_commit(
QEF_RCB         *v_qer_p,
DD_LDB_DESC	*i_ldb_p );

FUNC_EXTERN DB_STATUS
qed_u8_gmt_now(
QEF_RCB	    *i_qer_p,
DD_DATE	    o_gmt_now );

FUNC_EXTERN DB_STATUS
qed_u9_log_forgiven(
QEF_RCB	    *i_qer_p );

FUNC_EXTERN DB_STATUS
qed_u11_ddl_commit(
QEF_RCB         *qer_p );

FUNC_EXTERN VOID
qed_u12_map_rqferr(
RQR_CB		*i_rqr_p,
DB_ERROR	*o_err_p );

FUNC_EXTERN VOID
qed_u13_map_tpferr(
TPR_CB		*i_tpr_p,
DB_ERROR	*o_err_p );

FUNC_EXTERN DB_STATUS
qed_u14_pre_commit(
QEF_RCB         *qer_p );

FUNC_EXTERN VOID
qed_u15_trace_qefcall(
QEF_CB		*i_qefcb_p,
i4		i_call_id );

FUNC_EXTERN DB_STATUS
qed_u16_rqf_cleanup(
QEF_RCB         *qer_p );

FUNC_EXTERN DB_STATUS
qed_u17_tpf_call(
i4	i_call_id,
TPR_CB	*v_tpr_p,
QEF_RCB	*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_w0_prt_gmtnow(
QEF_RCB	    *v_qer_p );

FUNC_EXTERN DB_STATUS
qed_w1_prt_ldbdesc(
QEF_RCB		*v_qer_p,
DD_LDB_DESC	*i_ldb_p );

FUNC_EXTERN VOID
qed_w2_prt_rqferr(
QEF_RCB		*v_qer_p,
i4		i1_call_id,
RQR_CB		*i2_rqr_p );

FUNC_EXTERN VOID
qed_w3_prt_tpferr(
QEF_RCB		*v_qer_p,
i4		i1_call_id,
TPR_CB		*i2_tpr_p );

FUNC_EXTERN VOID
qed_w4_prt_qefcall(
QEF_CB	*i_qefcb_p,
i4	i_call_id );

FUNC_EXTERN VOID
qed_w5_prt_qeferr(
QEF_RCB		*v_qer_p,
i4		i_call_id );

FUNC_EXTERN VOID
qed_w6_prt_qperror(
QEF_RCB		*i_qer_p );

FUNC_EXTERN DB_STATUS
qed_w7_prt_qryinfo(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qed_w8_prt_qefqry(
QEF_RCB		*v_qer_p,
char		*i_txt_p,
DD_LDB_DESC	*i_ldb_p );

FUNC_EXTERN DB_STATUS
qed_w9_prt_db_val(
QEF_RCB         *v_qer_p,
DB_DATA_VALUE	*i1_dv_p );

FUNC_EXTERN DB_STATUS
qed_w10_prt_qrytxt(
QEF_RCB         *v_qer_p,
char		*i1_txt_p,
i4		i2_txtlen );

/*	QEE functions	*/

FUNC_EXTERN	DB_STATUS
qee_shutdown();

FUNC_EXTERN	DB_STATUS 
qee_cleanup(
	QEF_RCB		*qef_rcb,
	QEE_DSH		**dsh);

FUNC_EXTERN	DB_STATUS 
qee_destroy(
	QEF_CB		*qef_cb,
	QEF_RCB		*qef_rcb,
	QEE_DSH		**dsh);

FUNC_EXTERN	DB_STATUS
qee_fetch(
	QEF_RCB		*qef_rcb,
	QEF_QP_CB	*qp,
	QEE_DSH		**dsh,
	i4		page_count,
	QSF_RCB		*qsf_rcb,
        bool            isTproc);

FUNC_EXTERN	bool
qee_chkeob(
	QEF_AHD	    *act,
	QEF_AHD	    **eobptr,
	i4	    count );

FUNC_EXTERN	DB_STATUS
qee_hfile(
	QEF_RCB		    *qef_rcb,
	QEE_DSH		    *dsh,
	QEN_NODE	    *node,
	ULM_RCB		    *ulm);

FUNC_EXTERN DB_STATUS qee_hash_reserve(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	QEF_AHD		*act,
	ULM_RCB		*ulm,
	ULM_RCB		*temp_ulm);

FUNC_EXTERN DB_STATUS qee_hashInit(
        QEF_RCB		*qef_rcb,
        QEE_DSH		*dsh,
        QEN_NODE	*node,
	SIZE_TYPE	session_limit,
	bool		always_reserve,
	ULM_RCB		*ulm);

FUNC_EXTERN DB_STATUS qee_hashaggInit(
        QEF_RCB		*qef_rcb,
        QEE_DSH		*dsh,
        QEF_QEP		*qepp,
	SIZE_TYPE	session_limit,
	bool		always_reserve,
	ULM_RCB		*ulm);

FUNC_EXTERN	DB_STATUS
qee_dbparam(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	QEE_DSH       	*callers_dsh,
	QEF_CP_PARAM	cp_params[],
	i4		ucount,
	bool 		nested);

FUNC_EXTERN	DB_STATUS
qee_return_dbparam(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	QEE_DSH       	*callers_dsh,
	QEF_CP_PARAM	cp_params[],
	i4		ucount);

FUNC_EXTERN	DB_STATUS
qee_update_byref_params(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh);

FUNC_EXTERN void qee_resolve_xaddrs(
	QEE_DSH		*dsh,
	QEN_NODE	*node,
	bool		follow_qp);

FUNC_EXTERN	VOID
qee_d1_qid(
	QEE_DSH		*v_dsh_p);

FUNC_EXTERN	DB_STATUS
qee_d2_tmp(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	ULM_RCB		*ulm);

FUNC_EXTERN	DB_STATUS
qee_d3_agg(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	ULM_RCB		*ulm);

FUNC_EXTERN	DB_STATUS
qee_d6_malloc(
	i4		i_space,
	QEF_RCB		*qef_rcb,
	PTR		*o_pp);

FUNC_EXTERN	DB_STATUS
qee_d7_rptadd(
	QEF_RCB		*qef_rcb);

FUNC_EXTERN	DB_STATUS 
qee_d8_undefrpt(
	QEF_RCB	    *qef_rcb,
	PTR	    i_qso_handle);

FUNC_EXTERN	DB_STATUS
qee_d9_undefall(
	QEF_RCB		*qef_rcb);

FUNC_EXTERN	DB_STATUS
qee_p0_prt_dsh(
	QEF_RCB		*v_qer_p,
	QEE_DSH		*i_dsh_p);

FUNC_EXTERN	DB_STATUS
qee_p1_dsh_gen(
	QEF_RCB		*v_qer_p,
	QEE_DSH		*i_dsh_p);

FUNC_EXTERN	DB_STATUS
qee_p2_dsh_ddb(
	QEF_RCB		*v_qer_p,
	QEE_DSH		*i_dsh_p);

FUNC_EXTERN	DB_STATUS
qee_p3_qry_id(
	QEF_RCB		*v_qer_p,
	QEE_DSH         *i_dsh_p);

FUNC_EXTERN	DB_STATUS
qee_p4_ldb_qid(
	QEF_RCB		*v_qer_p,
	QEE_DSH         *i_dsh_p);

FUNC_EXTERN	DB_STATUS
qee_p5_qids(
	QEF_RCB		*v_qer_p,
	QEE_DSH         *i_dsh_p);

/*	QEF functions	*/

FUNC_EXTERN	STATUS
qef_mo_register( void );

FUNC_EXTERN 	STATUS 
qef_handler(
	EX_ARGS            *ex_args);

FUNC_EXTERN 	STATUS 
qef_2handler(
        EX_ARGS            *ex_args);

FUNC_EXTERN	bool
qe1_chk_error(
        char        *caller,
        char        *callee,
        i4     status,
        DB_ERROR    *err_blk);

FUNC_EXTERN 	bool
qe2_chk_qp(
        QEE_DSH     	   *dsh);

FUNC_EXTERN	DB_STATUS
qef_adf_error(
	ADF_ERROR          *adf_errcb,
        DB_STATUS          status,
        QEF_CB             *qef_cb,
	DB_ERROR	   *errptr);

FUNC_EXTERN void
qef_procerr(
	QEF_CB		*qef_cb,
	QEE_DSH		*currdsh);

FUNC_EXTERN	DB_STATUS
qes_init(
        QEE_DSH         *dsh,
        QEN_SHD         *shd,
        QEN_NODE        *node,
        i4              rowno,
        i4              dups);

FUNC_EXTERN	DB_STATUS
qes_insert(
        QEE_DSH         *dsh,
        QEN_SHD         *shd,
        DB_CMP_LIST     *sortkey,
        i4              kcount);

FUNC_EXTERN	DB_STATUS
qes_endsort(
        QEE_DSH         *dsh,
        QEN_SHD         *shd);

FUNC_EXTERN	DB_STATUS
qes_dumploop(
        QEE_DSH          *dsh,
        QEN_SHD          *shd,
        DMR_CB           *dmr);

FUNC_EXTERN	DB_STATUS
qes_put(
        QEE_DSH         *dsh,
        QEN_SHD         *shd);

FUNC_EXTERN	DB_STATUS
qes_putheap(
        QEE_DSH         *dsh,
        QEN_SHD           *shd,
        DB_CMP_LIST       *sortkey,
        i4                kcount);

FUNC_EXTERN	DB_STATUS
qes_getheap(
        QEE_DSH         *dsh,
        QEN_SHD           *shd,
        DB_CMP_LIST       *sortkey,
        i4                kcount);

FUNC_EXTERN	DB_STATUS
qes_sorter(
        QEE_DSH         *dsh,
        QEN_SHD           *shd,
        DB_CMP_LIST       *sortkey,
        i4                kcount);

FUNC_EXTERN	DB_STATUS
qes_dump(
        QEE_DSH          *dsh,
        QEN_SHD          *shd,
        DMR_CB           *dmr);

/*	QEK functions	*/

FUNC_EXTERN	i4
qek_c1_str_to_code(
char		*i_str_p );

FUNC_EXTERN	i4
qek_c2_val_to_code(
char		*i_str_p );

/*	QEL functions	*/

FUNC_EXTERN DB_STATUS
qel_01_ddb_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_02_objects(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_03_object_base(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_04_tableinfo(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_05_ldbids(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_06_ldb_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_07_ldb_dbcaps(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_11_his_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_12_stats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_13_histograms(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_21_ndx_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_22_indexes(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_23_ndx_cols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
QEC_INDEX_ID	*i_xid_p );

FUNC_EXTERN DB_STATUS
qel_24_ndx_lnks(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_25_lcl_cols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
QEC_INDEX_ID	*i_xid_p,
i4		*o_cnt_p,
DD_COLUMN_DESC	**o_cid_pp );

FUNC_EXTERN VOID
qel_26_ndx_name(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
i4		i_ndx_ord,
DD_TAB_NAME	*o_given_p );

FUNC_EXTERN DB_STATUS
qel_31_std_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_32_tables(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_33_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_34_altcols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_35_registrations(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_51_ddb_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_52_ldbids(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_53_caps_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_54_tableinfo(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_55_objects(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_56_ldb_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_57_longnames(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_58_dbcapabilities(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_59_adjust_1_cap(
i4		i_upd_id,
i4		i_cap_lvl,
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_61_his_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_62_stats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_63_histograms(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_71_ndx_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_72_xcolumns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
QEC_INDEX_ID	*i_id_p );

FUNC_EXTERN DB_STATUS
qel_73_indexes(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_74_ndx_lnks(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_75_ndx_ids(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p,
QEC_INDEX_ID	    *i_xid_p,
DB_TAB_ID	    *o_ndx_id_p );

FUNC_EXTERN DB_STATUS
qel_81_std_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_82_tables(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_83_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_84_altcols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_85_registrations(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN bool
qel_a0_map_col(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
char		*i_name_p,
DD_COLUMN_DESC	**o_coldesc_pp );

FUNC_EXTERN DB_STATUS
qel_a1_min_cap_lvls(
bool		i_use_pre_b,
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN VOID
qel_a2_case_col(
QEF_RCB		*i_qer_p,
char		*i_colptr,
u_i4	i_xlate,
DD_CAPS		*i_ldb_caps);

FUNC_EXTERN DB_STATUS
qel_c0_cre_lnk(
QEF_RCB		*v_qer_p );  

FUNC_EXTERN DB_STATUS
qel_c1_cre_p1(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_c2_cre_p2(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_c3_obj_base(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_c4_ldbid_short(
bool		    i_alias_b,
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_c5_ldbid_long(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_c6_ldb_tables(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_c7_ldb_chrs(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d0_des_lnk(
QEF_RCB		*v_qer_p ); 

FUNC_EXTERN DB_STATUS
qel_d1_des_p1(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d2_des_p2(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d3_objects(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d4_tableinfo(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d5_ldbinfo(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d6_ndxinfo(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d7_iistatscnt(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_d8_des_p2(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_e0_cre_lnk(
QEF_RCB		*i_qer_p );

FUNC_EXTERN DB_STATUS
qel_i1_insert(
QEF_RCB		*v_qer_p, 
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_i2_query(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_i3_vdata(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_r1_remove(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_s0_setup(
i4		i1_canned_id,
RQB_BIND	*i2_rq_bind_p,
DD_LDB_DESC	*i3_ldb_p,
QEF_RCB		*v_qer_p,
QEC_LINK	*o1_lnk_p,
QEQ_1CAN_QRY	*o2_sel_p );

FUNC_EXTERN DB_STATUS
qel_s1_select(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_s2_fetch(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_s3_flush(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_s4_prepare(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_u1_update(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p );

FUNC_EXTERN DB_STATUS
qel_u2_query(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p,
char		*o_qry_p );

FUNC_EXTERN DB_STATUS
qel_u3_lock_objbase(
QEF_RCB		*v_qer_p );

/*	QEN functions	*/

FUNC_EXTERN DB_STATUS
qen_cpjoin(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function );

FUNC_EXTERN DB_STATUS
qen_fsmjoin(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function );

FUNC_EXTERN DB_STATUS
qen_print_row(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh);

FUNC_EXTERN DB_STATUS
qen_isjoin(
QEN_NODE	*node,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function );

FUNC_EXTERN DB_STATUS
qen_kjoin(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_hjoin(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_exchange(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN void
qen_bcost_begin(
QEE_DSH            *dsh,
TIMERSTAT   *timerstat,
QEN_STATUS  *qenstatus );

FUNC_EXTERN void
qen_ecost_end(
QEE_DSH            *dsh,
TIMERSTAT   *btimerstat,
QEN_STATUS  *qenstatus );

FUNC_EXTERN DB_STATUS
qen_orig(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_origagg(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_position(
QEN_TKEY           *qen_tkey,
DMR_CB             *dmr_cb,
QEE_DSH            *dsh,
i4		   key_width,
bool		   readback );

FUNC_EXTERN DB_STATUS
qen_qp(
QEN_NODE	*node,
QEF_RCB		*rcb,
QEE_DSH         *dsh,
i4		function );

FUNC_EXTERN DB_STATUS
qen_sejoin(
QEN_NODE	*node,
QEF_RCB		*rcb,
QEE_DSH         *dsh,
i4		function );

FUNC_EXTERN DB_STATUS
qen_sort(
QEN_NODE           *node,
QEF_RCB		   *qef_rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_inner_sort(
QEN_NODE           *node,
QEF_RCB		   *qef_rcb,
QEE_DSH            *dsh,
DMR_CB		   *dmr_get,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_tjoin(
QEN_NODE           *node,
QEF_RCB            *rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN DB_STATUS
qen_tsort(
QEN_NODE           *node,
QEF_RCB            *qef_rcb,
QEE_DSH            *dsh,
i4		   function );

FUNC_EXTERN QEF_HASH_DUP * qen_hash_rll(
	i4		skip,
	QEF_HASH_DUP	*rowptr,
	PTR		where);

FUNC_EXTERN QEF_HASH_DUP * qen_hash_unrll(
	i4		skip,
	QEF_HASH_DUP	*rowptr,
	PTR		where);

FUNC_EXTERN void qen_hash_value(
	QEF_HASH_DUP	*rowptr,
	i4		keysz);

FUNC_EXTERN DB_STATUS
qen_hash_partition(
QEE_DSH         *dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	*buildptr,
bool		*partflag,
bool		try_compress);
 
FUNC_EXTERN DB_STATUS
qen_hash_build(
QEE_DSH         *dsh,
QEN_HASH_BASE	*hbase);
 
FUNC_EXTERN DB_STATUS
qen_hash_probe1(
QEE_DSH         *dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	*probeptr,
QEN_OJMASK	*ojmask,
bool		*gotone);

FUNC_EXTERN DB_STATUS
qen_hash_probe2(
QEE_DSH         *dsh,
QEN_HASH_BASE	*hbase,
QEF_HASH_DUP	**buildptr,
QEF_HASH_DUP	**probeptr,
QEN_OJMASK	*ojmask);

FUNC_EXTERN DB_STATUS
qen_hash_closeall(
QEN_HASH_BASE	*hbase);

FUNC_EXTERN DB_STATUS
qen_u1_append(
QEN_HOLD   *hold,
QEN_SHD    *shd,
QEE_DSH    *dsh );

FUNC_EXTERN DB_STATUS
qen_u2_position_tid(
QEN_HOLD   *hold,
QEE_DSH    *dsh,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_u3_position_begin(
QEN_HOLD   *hold,
QEE_DSH    *dsh,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_u4_read_positioned(
QEN_HOLD   *hold,
QEE_DSH    *dsh,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_u5_save_position(
QEN_HOLD   *hold,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_u6_clear_hold(
QEN_HOLD   *hold,
QEE_DSH    *dsh,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_u7_to_unget(
QEN_HOLD   *hold,
QEN_SHD    *shd,
QEE_DSH    *dsh );

FUNC_EXTERN DB_STATUS
qen_u8_current_tid(
QEN_HOLD   *hold,
QEN_SHD    *shd,
u_i4	   *tidp );

FUNC_EXTERN DB_STATUS
qen_u9_dump_hold(
QEN_HOLD   *hold,
QEE_DSH    *dsh,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_execute_cx(
	QEE_DSH *dsh,
	ADE_EXCB *excb);

FUNC_EXTERN DB_STATUS
qen_u31_release_mem(
QEN_HOLD   *hold,
QEE_DSH    *dsh,
QEN_SHD    *shd );

FUNC_EXTERN DB_STATUS
qen_u32_dosort(
QEN_NODE	*sortnode,
QEF_RCB		*rcb,
QEE_DSH		*dsh, 
QEN_STATUS	*node_status,    
QEN_HOLD	*hold,
QEN_SHD         *shd,
i4		function );

FUNC_EXTERN DB_STATUS
qen_u33_resetSortFile(
QEN_SHD    *shd,
QEE_DSH    *dsh );

FUNC_EXTERN DB_STATUS
qen_u40_readInnerJoinFlag(
QEN_HOLD	*hold,
QEE_DSH 	*dsh,
QEN_SHD         *shd,
bool		*innerJoinFlag );

FUNC_EXTERN DB_STATUS
qen_u41_storeInnerJoinFlag(
QEN_HOLD	*hold,
QEN_SHD         *shd,
QEE_DSH		*dsh,
bool		oldValue,
bool		newValue );

FUNC_EXTERN DB_STATUS
qen_u42_replaceInnerJoinFlag(
QEN_HOLD	*hold,
QEN_SHD         *shd,
QEE_DSH		*dsh );

FUNC_EXTERN DB_STATUS
qen_u101_open_dmf(
QEN_HOLD   *hold,
QEE_DSH    *dsh );

FUNC_EXTERN DB_STATUS qen_initTempTable(
	QEE_DSH         *dsh,
	i4		tempTableNumber
);

FUNC_EXTERN DB_STATUS qen_putIntoTempTable(
	QEE_DSH         *dsh,
	QEN_TEMP_TABLE	*tempTable
);

FUNC_EXTERN DB_STATUS qen_rewindTempTable(
	QEE_DSH         *dsh,
	i4		tempTableNumber
);

FUNC_EXTERN DB_STATUS qen_getFromTempTable(
	QEE_DSH         *dsh,
	i4		tempTableNumber
);

FUNC_EXTERN DB_STATUS qen_destroyTempTable(
	QEE_DSH         *dsh,
	i4		tempTableNumber
);

FUNC_EXTERN DB_STATUS qen_doneLoadingTempTable(
	QEE_DSH         *dsh,
	i4		tempTableNumber
);

FUNC_EXTERN DB_STATUS
qen_initTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH         	*dsh );

FUNC_EXTERN DB_STATUS
qen_dumpTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH         	*dsh );

FUNC_EXTERN DB_STATUS
qen_rewindTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH         	*dsh );

FUNC_EXTERN DB_STATUS
qen_readTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH         	*dsh,
u_i4	   		tid );

FUNC_EXTERN DB_STATUS
qen_appendTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH         	*dsh,
u_i4	   		tid );

FUNC_EXTERN DB_STATUS
qen_keyprep(
QEE_DSH         	*dsh,
QEN_NODE		*node );

FUNC_EXTERN DB_STATUS qen_openAndLink(
	DMT_CB		*dmtcb,
	QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS qen_closeAndUnlink(
	DMT_CB		*dmtcb,
	QEE_DSH		*dsh);

FUNC_EXTERN void qen_set_qualfunc(
	DMR_CB		*dmr,
	QEN_NODE	*node,
	QEE_DSH		*dsh);

/*	QEQ functions	*/

FUNC_EXTERN DB_STATUS
qea_aggf(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function );

FUNC_EXTERN DB_STATUS
qea_haggf(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function,
bool		    first_time );

FUNC_EXTERN DB_STATUS
qea_haggf_closeall(
QEN_HASH_AGGREGATE *hbase);

FUNC_EXTERN DB_STATUS
qea_callproc(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function,		/* Unused */
i4		    state )		/* Unused */;

FUNC_EXTERN DB_STATUS
qea_prc_insert(
QEF_AHD             *act,
QEE_DSH             *dsh,
i4		    function,		/* Unused */
i4                  state)              /* Unused */;

FUNC_EXTERN DB_STATUS
qea_closetemp(
QEF_AHD             *act,
QEE_DSH             *dsh,
i4		    function,		/* Unused */
i4                  state)              /* Unused */;

FUNC_EXTERN DB_STATUS
qea_commit(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function,
i4		    state );

FUNC_EXTERN DB_STATUS
qea_delete(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function,
i4		state );

FUNC_EXTERN DB_STATUS
qea_pdelete(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function,
i4		state );

FUNC_EXTERN DB_STATUS
qea_dmu(
QEF_AHD		*qea_act,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh );

FUNC_EXTERN DB_STATUS
qea_event(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh );

FUNC_EXTERN DB_STATUS
qea_fetch(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function );

FUNC_EXTERN DB_STATUS
qea_seqop(
QEF_AHD		*action,
QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS
qea_retrow(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
i4		function );

FUNC_EXTERN DB_STATUS
qea_iproc(
QEF_AHD		*qea_act,
QEF_RCB		*qef_rcb );

FUNC_EXTERN i4
qea_findlen(
char	    *parm,
i4	    len );

FUNC_EXTERN DB_STATUS
qea_0list_file(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_1drop_file(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_2create_db(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_3destroy_db(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_4alter_db(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_5add_location(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_67_check_patch(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh,
i4			patch_flag );

FUNC_EXTERN DB_STATUS
qea_8_finddbs(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_12alter_extension(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_9_readconfig(
			QEF_RCB		*qef_rcb,
			QEF_DBP_PARAM	*db_parm,
			QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS
qea_10_updconfig(	QEF_RCB		*qef_rcb,
			QEF_DBP_PARAM	*db_parm,
			QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS
qea_11_deldmp_config(	QEF_RCB		*qef_rcb,
			QEF_DBP_PARAM	*db_parm,
			QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS
qea_13_convert_table(
QEF_RCB            	*qef_rcb,
QEF_DBP_PARAM		*db_parm,
QEE_DSH			*dsh );

FUNC_EXTERN DB_STATUS
qea_message(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb );

FUNC_EXTERN DB_STATUS
qea_emessage(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb );

FUNC_EXTERN DB_STATUS
qea_proj(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function );

FUNC_EXTERN DB_STATUS
qea_append(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function,
i4		state );

FUNC_EXTERN DB_STATUS
qea_raggf(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function,
bool		    first_time );

FUNC_EXTERN DB_STATUS
qea_rdelete(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
QEE_DSH		    *assoc_dsh,
i4		    function,
i4		    state );

FUNC_EXTERN DB_STATUS
qea_replace(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function,
i4		state );

FUNC_EXTERN DB_STATUS
qea_preplace(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH         *dsh,
i4		function,
i4		state );

FUNC_EXTERN DB_STATUS
qea_rollback(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function,
i4		    state );

FUNC_EXTERN DB_STATUS
qea_rupdate(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
QEE_DSH		    *assoc_dsh,
i4		    function,
i4		    state );

FUNC_EXTERN DB_STATUS
qea_sagg(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb,
QEE_DSH             *dsh,
i4		    function );

FUNC_EXTERN	DB_STATUS
qea_createIntegrity(
    QEF_AHD		*qea_act,
    QEF_RCB		*qef_rcb,
    QEE_DSH		*dsh );

FUNC_EXTERN DB_STATUS
qea_cview(
    QEF_AHD             *qea_act,
    QEF_RCB             *qef_rcb,
    QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS
qea_d_integ(
    QEF_AHD             *qea_act,
    QEF_RCB             *qef_rcb);

FUNC_EXTERN DB_STATUS
qea_reserveID(
    DB_TAB_ID   *tableID,
    QEF_CB      *qef_cb,
    DB_ERROR    *err );

FUNC_EXTERN DB_STATUS
qea_schema(
	QEF_RCB         *qef_rcb,
	QEUQ_CB		*qeuq_cb,
        QEF_CB          *qef_cb,
        DB_SCHEMA_NAME  *qef_schname,
        i4              flag);

FUNC_EXTERN DB_STATUS
qea_xcimm(
QEF_AHD		*act,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
DB_TEXT_STRING	*text,
PST_INFO	*qea_info);

FUNC_EXTERN void 
qea_scontext(
        QEF_RCB         *qef_rcb,
        QEE_DSH         *dsh,
        QEF_AHD         *action);

FUNC_EXTERN DB_STATUS
qea_chkerror(
        QEE_DSH         *dsh,
	QSF_RCB		*qsf_rb,
	i4		flags );

/*	bits in flags argument of qea_chkerror	*/

#define	QEA_CHK_DESTROY_TEXT_OBJECT	0x00000001
	/* release text storage in QSF */
#define	QEA_CHK_LOOK_FOR_ERRORS		0x00000002
	/* look for errors returned by the previously executed statement */

FUNC_EXTERN DB_STATUS
qea_cobj(
	QEF_RCB         *qef_rcb,
	QEE_DSH		*dsh,
        PST_INFO        *qea_info,
        DB_TEXT_STRING  *text,
	QSF_RCB		*qsf_rb);

FUNC_EXTERN DB_STATUS
qea_dsh(
	QEF_RCB         *qef_rcb,
        QEE_DSH         *dsh,
        QEF_CB          *qef_cb);

FUNC_EXTERN DB_STATUS
qeq_open(
QEF_RCB		    *qef_rcb );

FUNC_EXTERN DB_STATUS
qeq_close(
QEF_RCB		    *qef_rcb );

FUNC_EXTERN DB_STATUS
qeq_fetch(
QEF_RCB		*qef_rcb );

FUNC_EXTERN DB_STATUS
qeq_query(
QEF_RCB		*qef_rcb,
i4		mode );

FUNC_EXTERN bool
qeq_qp_apropos(
QEF_VALID      	*vl,
i4		page_count,
bool		care_if_smaller);

FUNC_EXTERN VOID
qeq_release_dsh_resources(
QEE_DSH	    *dsh);

FUNC_EXTERN VOID
qeq_unwind_dsh_resources(
QEE_DSH	    *dsh);

FUNC_EXTERN DB_STATUS
qeq_dsh(
QEF_RCB		*qef_rcb,
i4		must_find,
QEE_DSH		**dsh,
bool		in_progress,
i4              page_count,
bool            isTproc );

FUNC_EXTERN VOID
qeq_nccalls(
QEE_DSH		*dsh,
QEN_NODE	*node,
i4		*ncost_calls );

FUNC_EXTERN DB_STATUS
qeq_cleanup(
QEF_RCB	    *qef_rcb,
DB_STATUS   c_status,
bool	    release );

FUNC_EXTERN void qeq_close_dsh_nodes(
	QEE_DSH *dsh);

FUNC_EXTERN DB_STATUS
qeq_endret(
QEF_RCB		*qef_rcb );

FUNC_EXTERN DB_STATUS
qeq_chk_resource(
QEE_DSH		*dsh,
QEF_AHD		*action );

FUNC_EXTERN DB_STATUS
qeq_a1_doagg(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p );

FUNC_EXTERN DB_STATUS
qeq_a2_fetch(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p,
bool		*o_tupread_p,
bool		*o_eod_p );

FUNC_EXTERN DB_STATUS
qeq_a3_flush(
QEF_RCB		*v_qer_p,
DD_LDB_DESC	*i_ldb_p );

FUNC_EXTERN DB_STATUS
qeq_c1_close(
QEF_RCB	    *v_qer_p,
QEE_DSH	    *i_dsh_p );

FUNC_EXTERN DB_STATUS 
qeq_c2_delete(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_c3_fetch(
QEF_RCB	    *v_qer_p,
QEE_DSH	    *i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_c4_open(
QEF_RCB		*v_qer_p,
QEE_DSH		*v_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_c5_replace(
QEF_RCB	    *v_qer_p,
QEE_DSH	    *i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_d0_val_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p );

FUNC_EXTERN DB_STATUS
qeq_d1_qry_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    **v_act_pp );

FUNC_EXTERN DB_STATUS
qeq_d2_get_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p );

FUNC_EXTERN DB_STATUS 
qeq_d3_end_act(
QEF_RCB		*v_qer_p );

FUNC_EXTERN DB_STATUS
qeq_d4_sub_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p );

FUNC_EXTERN DB_STATUS
qeq_d5_xfr_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p );

FUNC_EXTERN DB_STATUS 
qeq_d6_tmp_act(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_d7_rpt_act(
QEF_RCB		*v_qer_p,
QEF_AHD		**v_act_pp );

FUNC_EXTERN DB_STATUS
qeq_d8_def_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p );

FUNC_EXTERN DB_STATUS
qeq_d9_inv_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p );

FUNC_EXTERN DB_STATUS
qeq_d10_agg_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p );

FUNC_EXTERN DB_STATUS
qeq_d21_val_qp(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_d22_err_qp(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p1_sub_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEF_AHD		*i2_sub_p );

FUNC_EXTERN DB_STATUS
qeq_p3_xfr_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEQ_D3_XFR	*i2_xfr_p );

FUNC_EXTERN DB_STATUS
qeq_p5_def_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEF_AHD		*i2_def_p );

FUNC_EXTERN DB_STATUS
qeq_p6_inv_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p );

FUNC_EXTERN DB_STATUS
qeq_p10_prt_qp(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p11_qp_hdrs(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p12_qp_ldbs(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p13_qp_vals(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p,
i4		 i_dvcnt,
DB_DATA_VALUE	*i_dv_p );

FUNC_EXTERN DB_STATUS
qeq_p14_qp_acts(
QEF_RCB		*v_qer_p,
QEE_DSH		*i1_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p15_prm_vals(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p,
i4		 i_dvcnt,
DB_DATA_VALUE  **i_dv_pp );

FUNC_EXTERN DB_STATUS
qeq_p20_sub_qry(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEQ_D1_QRY	*i2_sub_p );

FUNC_EXTERN DB_STATUS
qeq_p21_hdr_gen(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p22_hdr_ddb(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p31_opn_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p32_cls_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p33_fet_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p34_del_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_p35_upd_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p );

FUNC_EXTERN DB_STATUS
qeq_validate(
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh,
QEF_AHD		*action,
i4		function,
bool		init_action );

FUNC_EXTERN void qeq_join_pqeof(
	QEE_DSH		*dsh,
	QEN_PART_QUAL	*pqual);

FUNC_EXTERN void qeq_join_pqreset(
	QEE_DSH		*dsh,
	QEN_PART_QUAL	*pqual);

FUNC_EXTERN DB_STATUS qeq_join_pquals(
	QEE_DSH		*dsh,
	QEN_PART_QUAL	*pqual);

FUNC_EXTERN DB_STATUS
qeq_part_open(
QEF_RCB		*qef_rcb, 
QEE_DSH		*dsh, 
PTR		rowptr,
i4		rowsize,
i4		dmrix,
i4		dmtix,
u_i2		partno);

FUNC_EXTERN DB_STATUS qeq_pqual_eval(
	QEN_PQ_EVAL	*pqe,
	QEE_PART_QUAL	*rqual,
	QEE_DSH		*dsh);

FUNC_EXTERN DB_STATUS qeq_pqual_init(
	QEE_DSH		*dsh,
	QEN_PART_QUAL	*pqual,
	DB_STATUS	(*cx_init)(QEE_DSH *,QEN_ADF *) );

FUNC_EXTERN DB_STATUS
qeq_ksort(
QEN_TKEY           *qen_tkey,
QEF_KEY		   *qef_key,
QEE_DSH            *dsh,
bool		   keysort,
bool		   descending );

FUNC_EXTERN DB_STATUS
qeq_ade(
QEE_DSH            *dsh,
QEN_ADF            *qen_adf);

FUNC_EXTERN DB_STATUS
qeq_updrow(
QEF_CB          *qef_cb);

FUNC_EXTERN	DB_STATUS qeq_d11_regproc(
QEF_RCB           *v_qer_p,
QEF_AHD           *act_p);

FUNC_EXTERN	DB_STATUS qeq_d12_regproc(
QEF_RCB           *v_qer_p,
QEF_AHD           *act_p);

/*	QET functions	*/

FUNC_EXTERN DB_STATUS
qet_abort(
QEF_CB         *qef_cb);

FUNC_EXTERN DB_STATUS
qet_autocommit(
QEF_CB         *qef_cb);

FUNC_EXTERN DB_STATUS
qet_begin(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_commit(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_savepoint(
QEF_CB         *qef_cb);

FUNC_EXTERN DB_STATUS
qet_secure(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_resume(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_xa_start(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_xa_end(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_xa_prepare(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_xa_commit(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_xa_rollback(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_t1_abort(
QEF_CB         *qef_cb);

FUNC_EXTERN DB_STATUS
qet_t2_begin(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_t3_commit(
QEF_CB     *qef_cb);

FUNC_EXTERN DB_STATUS
qet_t4_savept(
QEF_CB         *qef_cb);

FUNC_EXTERN DB_STATUS
qet_t5_register(
QEF_RCB         *v_qer_p,
DD_LDB_DESC	*i_ldb_p,
DB_LANG		i_lang,
i4		i_mode);

FUNC_EXTERN DB_STATUS
qet_t6_ddl_cc(
QEF_RCB         *v_qer_p);

FUNC_EXTERN DB_STATUS
qet_t7_p1_recover(
QEF_RCB         *v_qer_p);

FUNC_EXTERN DB_STATUS
qet_t8_p2_recover(
QEF_RCB         *v_qer_p);

FUNC_EXTERN DB_STATUS
qet_t9_ok_w_ldbs(
QEE_DSH		*i_dsh_p,
QEF_RCB		*v_qer_p,
bool		*o1_ok_p);

FUNC_EXTERN DB_STATUS
qet_t10_is_ddb_open(
QEF_RCB         *v_qer_p);

FUNC_EXTERN DB_STATUS
qet_t11_ok_w_ldb(
DD_LDB_DESC	*i1_ldb_p,
QEF_RCB		*v_qer_p,
bool		*o1_ok_p);

FUNC_EXTERN DB_STATUS
qet_t20_recover(
QEF_RCB         *v_qer_p);

FUNC_EXTERN DB_STATUS
qet_connect(
QEF_RCB    *qef_rcb,
PTR	   *tran_id,
DB_ERROR   *error);

FUNC_EXTERN DB_STATUS
qet_disconnect(
QEF_RCB    *qef_rcb,
PTR	   *tran_id,
DB_ERROR   *error);

/*	QEU functions	*/

FUNC_EXTERN DB_STATUS
qeu_10_get_obj_base(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_11_object_base(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_12_objects(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_13_tables(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_14_columns(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_15_views(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_16_tree(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_17_dbdepends(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_20_des_view(
QEF_RCB         *i_qer_p);

FUNC_EXTERN DB_STATUS
qeu_21_des_depends(
QEF_RCB         *i_qer_p,
QEC_LINK	*v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_22_sel_dbdepends(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p,
i4		    *o_cnt_p,
DB_TAB_ID	    *o_vids_p);

FUNC_EXTERN DB_STATUS
qeu_23_des_a_view(
QEF_RCB		    *i_qer_p,
DB_TAB_ID	    *i_vid_p);

FUNC_EXTERN DB_STATUS
qeu_24_view_cats(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_25_norm_cats(
QEF_RCB		    *i_qer_p,
QEC_LINK    	    *v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_26_tree(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_27_views(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_28_dbdepends(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p);

FUNC_EXTERN DB_STATUS
qeu_aplid(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_caplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_daplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb,
bool		pdbpriv);

FUNC_EXTERN DB_STATUS
qeu_aaplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_gaplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*aplid,
DB_APPLICATION_ID   *aptuple);

FUNC_EXTERN DB_STATUS
qeu_audit(
CS_SID       session_id,
i4           type,
i4           access,
char         *name,
i4           l_name,
i4      info_number,
i4           n_args,
PTR          args,
DB_ERROR     *err);

FUNC_EXTERN DB_STATUS
qeu_secaudit( 
    int		force,
    CS_SID	sess_id,
    char	*objname,
    DB_OWN_NAME *objowner,
    i4	objlength,
    SXF_EVENT	auditevent,
    i4	msg_id,
    SXF_ACCESS	accessmask,
    DB_ERROR    *err);

FUNC_EXTERN DB_STATUS
qeu_secaudit_detail( 
    int		force,
    CS_SID	sess_id,
    char	*objname,
    DB_OWN_NAME *objowner,
    i4	objlength,
    SXF_EVENT	auditevent,
    i4	msg_id,
    SXF_ACCESS	accessmask,
    DB_ERROR    *err,
    char	*detailtext,
    i4	detaillen,
    i4	detailnum);

FUNC_EXTERN DB_STATUS
qeu_ccomment(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dcomment(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cdbacc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_ddbacc(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dbpriv(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_pdbpriv(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*grantee,
char		*gtype);

FUNC_EXTERN VOID
qeu_xdbpriv(
i4		control,
i4		flags,
char		*string,
i4		*l_string);

FUNC_EXTERN DB_STATUS
qeu_d1_cre_tab(
QEF_RCB		*v_qer_p);

FUNC_EXTERN DB_STATUS
qeu_d2_des_tab(
QEF_RCB		*v_qer_p);

FUNC_EXTERN DB_STATUS
qeu_d3_cre_view(
QEF_CB          *qef_cb,
QEUQ_CB		*i_quq_p);

FUNC_EXTERN DB_STATUS
qeu_d4_des_view(
QEF_CB          *qef_cb,
QEUQ_CB		*i_quq_p);

FUNC_EXTERN DB_STATUS
qeu_d5_alt_tab(
QEF_RCB		*v_qer_p,
DMT_CB		*i_dmt_p);

FUNC_EXTERN DB_STATUS
qeu_d6_cre_view(
QEF_CB          *qef_cb,
QEUQ_CB		*i_quq_p);

FUNC_EXTERN DB_STATUS
qeu_d7_b_copy(
QEF_RCB       *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_d8_e_copy(
QEF_RCB     *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_d9_a_copy(
QEF_RCB     *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_cevent(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_devent(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_group(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cgroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_dgroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_ggroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*group,
char		*member,
DB_USERGROUP	*ugtuple);

FUNC_EXTERN DB_STATUS
qeu_pgroup(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*member);

FUNC_EXTERN DB_STATUS
qeu_cloc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_aloc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dloc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_append(
QEF_CB      *qef_cb,
QEU_CB      *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_close(
QEF_CB      *qef_cb,
QEU_CB      *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_delete(
QEF_CB          *qef_cb,
QEU_CB          *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_get(
QEF_CB          *qef_cb,
QEU_CB          *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_open(
QEF_CB          *qef_cb,
QEU_CB         *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_load(
QEF_CB      *qef_cb,
QEU_CB      *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_replace(
QEF_CB          *qef_cb,
QEU_CB          *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_credbp(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dbp_status(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_drpdbp(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cview(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dview(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cprot(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dprot(
QEF_CB          *qef_cb,
QEUQ_CB	    	*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_revoke(
QEF_CB          *qef_cb,
QEUQ_CB	    	*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cintg(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dintg(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
bool		integ,
bool		cons);

FUNC_EXTERN DB_STATUS
qeu_d_cascade(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DB_QMODE	obj_type,
DMT_TBL_ENTRY	*tbl_entry);

FUNC_EXTERN DB_STATUS
qeu_d_check_conix(
        QEF_CB          *qef_cb,
        QEUQ_CB         *qeuq_cb,
        ULM_RCB         *ulm,
        DB_TAB_ID       *ixid,
	bool            temp_tbl, 
        i4              *error,
        bool            *found_one);

FUNC_EXTERN DB_STATUS
qea_renameExecute(
    QEF_AHD             *qea_act,
    QEF_RCB             *qef_rcb,
    QEE_DSH             *dsh );

FUNC_EXTERN DB_STATUS
qeu_renameValidate(
QEF_CB          *qef_cb,
QEUQ_CB         *qeuq_cb,
ULM_RCB         *ulm,
DB_ERROR        *error_block,
DMT_TBL_ENTRY   *table_entry,
bool            isColumnRename,
DMF_ATTR_ENTRY  **dmf_attr);

FUNC_EXTERN DB_STATUS
qeu_rename_grants(
QEF_CB          *qef_cb,
QEUQ_CB         *qeuq_cb,
DB_TAB_NAME     *newName);

FUNC_EXTERN DB_STATUS
qeu_dstat(
QEF_CB          *qef_cb,
QEUQ_CB	    	*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_qid(
QEUQ_CB		*qeuq_cb,
DB_TAB_ID	*tab_id,
DB_QRY_ID	*qtext_id,
DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS
qeu_dbdelete(
QEF_CB		*qef_cb,
QEUQ_CB		*qeuq_cb,
DB_TAB_ID	*tab_id,
i4		type,
i4		qid,
DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS
qeu_access_perm(
QEF_CB          *qef_cb,
QEU_CB		*qeu_cb,
DMU_CB		*dmu_cb,
i4		flag);

FUNC_EXTERN DB_STATUS
qeu_crule(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_drule(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
bool		from_drop_rule);

FUNC_EXTERN DB_STATUS
qeu_dschema(
QEF_CB          *qef_cb,
QEUQ_CB         *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_b_copy(
QEF_RCB       *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_e_copy(
QEF_RCB     *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_r_copy(
QEF_RCB       *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_w_copy(
QEF_RCB       *qef_rcb);

FUNC_EXTERN DB_STATUS
qeu_dbu(
QEF_CB		*qef_cb,
QEU_CB		*qeu_cb,	/* READONLY argument */
i4		tabid_count,
DB_TAB_ID	*table_id,
i4		*row_count,
DB_ERROR	*error_block,
i4		caller );

FUNC_EXTERN DB_STATUS
qeu_query(
QEF_CB		   *qef_cb,
QEU_CB             *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_create_table(
QEF_RCB		*qefrcb,
DMU_CB		*dmu_cb,
QEU_LOGPART_CHAR *lpc_list,
i4		tabid_count,
DB_TAB_ID	*table_id);

FUNC_EXTERN DB_STATUS
qeu_tableOpen(
    QEF_RCB		*qef_rcb,
    QEU_CB		*qeu_cb,
    u_i4		baseID,
    u_i4		indexID
);

FUNC_EXTERN DB_STATUS
qeu_findOrMakeDefaultID(
    QEF_RCB		*qef_rcb,
    QEU_CB		*IIDEFAULTqeu_cb,
    QEU_CB		*IIDEFAULTIDXqeu_cb,
    DB_IIDEFAULT	*parserTuple,
    DB_DEF_ID		*returnedDefaultID
);

FUNC_EXTERN DB_STATUS
qeu_dqrymod_objects(
QEF_CB		*qef_cb,
QEUQ_CB		*qeuq_cb,
i4		*tbl_status_mask,
i4		*tbl_2_status_mask);

FUNC_EXTERN DB_STATUS
qeu_msec(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_create_synonym(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_drop_synonym(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN i4
qeu_cmp_tabids(
DB_TAB_ID	*tabid,
DB_IISYNONYM	*syn_tuple);

FUNC_EXTERN i4
qeu_cmp_baseids(
u_i4		*baseid,
DB_IISYNONYM	*syn_tuple);

FUNC_EXTERN DB_STATUS
qeu_btran(
QEF_CB	    *qef_cb,
QEU_CB      *qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_etran(
QEF_CB         *qef_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_atran(
QEF_CB         *qef_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_alter_timestamp(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_auser(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cuser(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_duser(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_guser(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*user,
char		*group,
DU_USER		*ustuple);

FUNC_EXTERN VOID
qeu_rdfcb_init(
        PTR  		rdfcb_ptr,
        QEF_CB          *qef_cb);

FUNC_EXTERN DB_STATUS
qeu_v_ubt_id(
        QEF_CB          *qef_cb,
        DB_TAB_ID       *view_id,
        DB_TAB_ID       *ubt_id,
        DB_ERROR        *err_blk);

FUNC_EXTERN DB_STATUS
qeu_v_col_drop(
	ULM_RCB		*ulm,
	QEF_CB		*qef_cb,
	QEU_CB		*tqeu,
	DB_TAB_ID	*dep_id,
	DB_TAB_ID	*base_id,
	DB_TREE_ID	*tree_id,
	i2		obj_num,
	DB_NAME		*obj_name,
	DB_OWN_NAME	*obj_owner,
	DB_QMODE	depobj_type,
	i4		col_attno,
	bool		cascade,
	DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS
qeu_mdfy_timestamp(
	QEF_CB          *qef_cb,
	DB_TAB_ID       *tbl_id,
	i4         *error);

FUNC_EXTERN DB_STATUS
qeu_force_qp_invalidation(
	QEF_CB          *qef_cb,
	DB_TAB_ID       *obj_id,
	bool            obj_timestamp_already_altered,
	bool		*qps_invalidated,
	DB_ERROR        *err_blk);

FUNC_EXTERN DB_STATUS
qeu_p_ubt_id(
        QEF_CB          *qef_cb,
        DB_OWN_NAME     *dbp_owner,
        DB_DBP_NAME     *dbp_name,
        DB_TAB_ID       *dbp_ubt_id,
        i4         *error);

FUNC_EXTERN DB_STATUS
qeu_clabaudit(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dlabaudit(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_cprofile(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_aprofile(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dprofile(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_gprofile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*profile,
DU_PROFILE	*protuple);

FUNC_EXTERN DB_STATUS
qeu_merge_user_profile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DU_USER		*user,
DU_PROFILE	*profile
);

FUNC_EXTERN DB_STATUS
qeu_unmerge_user_profile(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DU_USER		*user,
DU_PROFILE	*profile
);

FUNC_EXTERN DB_STATUS
qeu_profile_user(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DU_PROFILE	*old_ptuple,
DU_PROFILE	*new_ptuple
);

FUNC_EXTERN DB_STATUS
qeu_evraise(
QEF_CB          *qef_cb,
QEF_RCB		*qer_rcb
);

FUNC_EXTERN VOID
qea_evtrace(
QEF_RCB		*qef_rcb,
i4		print,
DB_ALERT_NAME	*alert, 
DB_DATA_VALUE	*tm,
i4		txtlen,
char		*txt 
);

FUNC_EXTERN DB_STATUS
qeu_dsecalarm(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
bool		from_drop_alarm);

FUNC_EXTERN DB_STATUS
qeu_csecalarm(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_db_exists(
QEF_CB	*qef_cb,
QEUQ_CB *qeuq_cb,
DB_DB_NAME *dbname,
i4	access,
DB_OWN_NAME *dbowner
);

FUNC_EXTERN DB_STATUS
qeu_rolegrant(
QEF_CB      *qef_cb,
QEUQ_CB	    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_prolegrant(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*grantee,
char		*rolename);

FUNC_EXTERN DB_STATUS
qeu_rrevoke(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_rgrant(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb);

FUNC_EXTERN DB_STATUS
qeu_cseq(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_aseq(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN DB_STATUS
qeu_dseq(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb);

FUNC_EXTERN VOID 
qefErrorFcn(
DB_ERROR	*dberror,
i4 		errorno,
i4  		detail,
i4 		err_type,
i4 		*err_code,
DB_ERROR 	*err_blk,
PTR 		FileName,
i4  		LineNumber,
i4 		num_parms, ...);

#ifdef HAS_VARIADIC_MACROS

/* New, preferred macro format includes dberror */
#define qefError(dberror,errno,detail,err_type,err_code,err_blk, ...) \
    qefErrorFcn(dberror,errno,detail,err_type,err_code,err_blk, \
    		__FILE__,__LINE__,__VA_ARGS__)

/* Bridge macro does not include dberror */
#define qef_error(errno,detail,err_type,err_code,err_blk, ...) \
    qefErrorFcn(NULL,errno,detail,err_type,err_code,err_blk, \
    		__FILE__,__LINE__,__VA_ARGS__)

#else

/* Variadic macros not supported */
#define qef_error qef_errorNV

FUNC_EXTERN VOID 
qef_errorNV(
i4 		errorno,
i4  		detail,
i4 		err_type,
i4 		*err_code,
DB_ERROR 	*err_blk,
i4 		num_parms, ...);

#define qefError qefErrorNV

FUNC_EXTERN VOID 
qefErrorNV(
DB_ERROR	*dberror,
i4 		errorno,
i4  		detail,
i4 		err_type,
i4 		*err_code,
DB_ERROR 	*err_blk,
i4 		num_parms, ...);

#endif /* HAS_VARIADIC_MACROS */

FUNC_EXTERN DB_STATUS
qen_tproc(
QEN_NODE        *node,
QEF_RCB         *rcb,
QEE_DSH         *dsh,
i4              function);

FUNC_EXTERN VOID
qeq_dshtorcb(
QEF_RCB         *qef_rcb,
QEE_DSH         *dsh);

FUNC_EXTERN VOID
qeq_rcbtodsh(
QEF_RCB         *qef_rcb,
QEE_DSH         *dsh);
