/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: TPFPROTO.H - Prototypes for TPF.
**
** Description:
**      This file defines prototypes for TPF.
**
** History: 
**      16-oct-92 (teresa)
**	    initial creation.
**	10-oct-93 (swm)
**	    Bug #56448
**	    Added prototype declaration for tpf_u0_tprintf which has now been
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
**      24-Sep-1999 (hanch04)
**	    Added tp2_put, remove tp2_u13_log
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2001 (jenjo02)
**	    Made tpd_u2_err_internal() a macro passing __FILE__
**	    and __LINE__ to the real function so that the next
**	    poor sap that gets stuck with a E_TP0020_INTERNAL
**	    has a fighting chance.
**/

/* prototype declarations for TPF */
FUNC_EXTERN DB_STATUS
tp2_c0_log_lx(
	TPD_DX_CB   *i1_dxcb_p,
	TPD_LX_CB   *i2_lxcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_c1_log_dx(
	TPD_DX_CB   *i1_dxcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_c2_log_dx_state(
	bool	     i1_commit_b,
	TPD_DX_CB   *v1_dxcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_c3_delete_dx(
	TPD_DX_CB   *i1_dxcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_c4_loop_to_log_dx_state(
	bool		i1_commit_b,
	TPD_DX_CB	*v1_dxcb_p,
	TPR_CB		*v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_c5_loop_to_delete_dx(
	TPD_DX_CB	*v1_dxcb_p,
	TPR_CB		*v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_d0_crash(
	TPR_CB		*v_rcb_p);
FUNC_EXTERN VOID
tp2_d1_suspend(
	TPR_CB		*v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_m0_commit_dx(
	TPR_CB		*v_tpr_p);
FUNC_EXTERN  DB_STATUS
tp2_m1_end_1pc(
	TPR_CB	    *v_tpr_p,
	bool	    i_to_commit);
FUNC_EXTERN DB_STATUS
tp2_m2_end_2pc(
	TPD_DX_CB   *v_dxcb_p,
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_m3_p0_commit_read_lxs(
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_m4_p1_secure_write_lxs(
	TPD_DX_CB	*i1_dxcb_p,
	TPR_CB		*v_tpr_p,
	bool		*o1_logged_p,
	bool		*o2_secured_p);
FUNC_EXTERN DB_STATUS
tp2_m5_p2_close_all_lxs(
	TPD_DX_CB	*i1_dxcb_p,
	bool		 i2_commit_b,
	bool		 i3_logstate_b,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_m6_p3_loop_to_close_lxs(
	TPD_DX_CB	*v_dxcb_p,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_m7_restart_and_close_lxs(
	bool		i1_restart_b,
	TPD_DX_CB	*v_dxcb_p,
	bool		i2_commit_b,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r0_recover(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r1_recover_all_ddbs(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r2_recover_1_ddb(
	TPD_CD_CB	*v1_cdcb_p,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r3_bld_cdb_list(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r4_bld_odx_list(
	TPD_CD_CB   *v1_cdcb_p,
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r5_bld_lx_list(
	TPD_CD_CB   *i1_cdcb_p,
	TPD_OX_CB   *v1_oxcb_p,
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r6_close_odx(
	TPD_CD_CB	*i1_cdcb_p,
	TPD_OX_CB	*v1_oxcb_p,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r7_close_all_lxs(
	TPD_DX_CB	*v1_dxcb_p,
	bool		i2_commit_b,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r8_p1_recover(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r9_p2_recover(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r10_get_dx_cnts(
	TPR_CB	    *v_tpr_p,
	bool	    *o1_recovery_p);
FUNC_EXTERN DB_STATUS
tp2_r11_bld_srch_list(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r12_set_ddb_recovered(
	TPD_CD_CB	*i1_cdcb_p,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tp2_r13_resecure_all_ldbs(
	TPD_DX_CB	*i1_dxcb_p,
	TPR_CB		*v_tpr_p,
	bool		*o1_willing_p);
FUNC_EXTERN DB_STATUS
tp2_u1_restart_1_ldb(
	TPD_DX_CB   *i1_dxcb_p,
	TPD_LX_CB   *v1_lxcb_p,
	TPR_CB	    *v_rcb_p,
	i4	    *o1_result_p);
FUNC_EXTERN DB_STATUS
tp2_u2_msg_to_ldb(
	TPD_DX_CB   *i1_dxcb_p,
	bool	     i2_commit_b,
	TPD_LX_CB   *v1_lxcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_u3_msg_to_cdb(
	bool	     i1_commit_b,
	DD_LDB_DESC *i2_cdb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_u4_init_odx_sess(
	DD_LDB_DESC *i1_cdb_p,
	TPR_CB	    *v_rcb_p,
	TPD_OX_CB   *o1_oxcb_p);
FUNC_EXTERN DB_STATUS
tp2_u5_term_odx_sess(
	TPD_OX_CB   *v1_oxcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN VOID
tp2_u6_cdcb_to_ldb(
	TPD_CD_CB   *i1_cdcb_p,
	DD_LDB_DESC *o1_cdb_p);
FUNC_EXTERN DB_STATUS
tp2_u7_term_lm_cb(
	TPD_LM_CB   *v1_lmcb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_u8_restart_all_ldbs(
	bool		i_recover_b,
	TPD_DX_CB	*v_dxcb_p,
	TPR_CB		*v_rcb_p,
	i4		*o1_dropout_cnt_p,
	bool		*o2_giveup_b_p);
FUNC_EXTERN DB_STATUS
tp2_u9_loop_to_restart_all_ldbs(
	bool	    i1_recover_b,
	TPD_DX_CB   *v_dxcb_p,
	TPR_CB	    *v_rcb_p,
	bool	    *o1_giveup_b_p);
FUNC_EXTERN DB_STATUS
tp2_u10_loop_to_restart_cdb(
	DD_LDB_DESC *i1_cdb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_u11_loop_to_send_msg(
	bool	     i1_commit_b,
	DD_LDB_DESC *i2_ldb_p,
	TPR_CB	    *v_rcb_p);
FUNC_EXTERN DB_STATUS
tp2_u12_term_all_ldbs(
	TPD_DX_CB	*v_dxcb_p,
	TPR_CB		*v_rcb_p);
FUNC_EXTERN VOID tp2_put(i4 tpf_error,
                          CL_ERR_DESC *os_error,
                          i4 num_params,
                          i4 p1l, PTR p1,
                          i4 p2l, PTR p2,
                          i4 p3l, PTR p3,
                          i4 p4l, PTR p4,
                          i4 p5l, PTR p5,
                          i4 p6l, PTR p6 );
FUNC_EXTERN VOID
tpd_m1_set_dx_id(
	TPD_SS_CB	*v_sscb_p);
FUNC_EXTERN VOID
tpd_m2_init_dx_cb(
	TPR_CB		*v_tpr_p,
	TPD_SS_CB	*o1_sscb_p);
FUNC_EXTERN DB_STATUS
tpd_m3_term_dx_cb(
	TPR_CB		*v_tpr_p,
	TPD_SS_CB	*o1_sscb_p);
FUNC_EXTERN DB_STATUS
tpd_m4_ok_ldb_for_dx(
	i4	    i1_cur_dxmode,
	TPD_LX_CB   *i2_lxcb_p,
	TPD_LX_CB   *i3_1pc_lxcb_p,
	TPR_CB	    *v_tpr_p,
	bool	    *o1_ok_p,
	i4	    *o2_new_dxmode_p);
FUNC_EXTERN DB_STATUS
tpd_m5_ok_write_ldbs(
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_m6_register_ldb(
	bool	i1_update_b,
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_m7_srch_ldb(
	i4		i1_which,
	DD_LDB_DESC	*i2_ldb_p,
	TPR_CB		*v_tpr_p,
	TPD_LX_CB	**o1_lxcb_pp);
FUNC_EXTERN DB_STATUS
tpd_m8_new_dx_mode(
	bool	    i1_update_b,
	TPD_LX_CB   *i2_lxcb_p,
	TPR_CB	    *v_tpr_p,
	i4	    *o1_newmode_p,
	bool	    *o2_ok_p);
FUNC_EXTERN DB_STATUS
tpd_m9_ldb_to_lxcb(
	DD_LDB_DESC *i1_ldb_p,
	TPR_CB	    *v_tpr_p,
	TPD_LX_CB   **o1_lxcb_pp,
	bool	    *o2_new_cb_p);
FUNC_EXTERN DB_STATUS
tpd_m10_is_ddb_open(
	DD_DDB_DESC	*i1_ddb_p,
	TPR_CB		*v_tpr_p,
	bool		*o1_ddb_open_p);
FUNC_EXTERN DB_STATUS
tpd_m11_is_ddb_open(
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_p1_prt_gmtnow(
	TPR_CB	    *v_tpr_p,
	bool	    i_to_log);
FUNC_EXTERN VOID
tpd_p2_prt_rqferr(
	i4		 i_rqf_id,
	RQR_CB		*i_rqr_p,
	TPR_CB		*v_tpr_p,
	bool		 i_to_log);
FUNC_EXTERN VOID
tpd_p3_prt_dx_state(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN VOID
tpd_p4_prt_tpfcall(
	TPR_CB	*v_tpr_p,
	i4	i_call_id);
FUNC_EXTERN DB_STATUS
tpd_p5_prt_tpfqry(
	TPR_CB		*v_tpr_p,
	char		*i_txt_p,
	DD_LDB_DESC	*i_ldb_p,
	bool		 i_to_log);
FUNC_EXTERN DB_STATUS
tpd_p6_prt_qrytxt(
	TPR_CB	    *v_tpr_p,
	char	    *i1_txt_p,
	i4	    i2_txtlen,
	bool	    i3_to_log);
FUNC_EXTERN VOID
tpd_p8_prt_ddbdesc(
	TPR_CB		    *v_tpr_p,
	TPC_I1_STARCDBS	    *i_starcdb_p,
	bool		    i_to_log);
FUNC_EXTERN DB_STATUS
tpd_p9_prt_ldbdesc(
	TPR_CB		*v_tpr_p,
	DD_LDB_DESC     *i_ldb_p,
	bool		 i_to_log);
FUNC_EXTERN TPD_SP_CB *
tpd_s1_srch_sp(
	TPD_SS_CB	*v_sscb_p,
	DB_SP_NAME	*sp_name,
	i4		*o1_good_cnt_p);
FUNC_EXTERN DB_STATUS
tpd_s2_clear_all_sps(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_s3_send_sps_to_ldb(
	TPR_CB		*v_tpr_p,
	TPD_LX_CB	*v1_lxcb_p);
FUNC_EXTERN DB_STATUS
tpd_s4_abort_to_svpt(
	TPR_CB		*v_tpr_p,
	i4		 i_new_svpt_cnt,
	DB_SP_NAME	*i_svpt_name_p);
FUNC_EXTERN i4
tpd_u0_trimtail(
	char		*i_str_p,
	i4		i_maxlen,
	char		*o_str_p);
FUNC_EXTERN DB_STATUS
tpd_u1_error(
	DB_ERRTYPE  i1_errcode,
	TPR_CB	    *v_tpr_p);

/* Identify where internal error was discovered */
#define	tpd_u2_err_internal(v_tpr_p) \
    tpd_u2_err_internal_fcn(v_tpr_p, __FILE__, __LINE__)

FUNC_EXTERN DB_STATUS
tpd_u2_err_internal_fcn(TPR_CB *v_tpr_p,
			char	*file,
			i4	line);

FUNC_EXTERN VOID
tpd_u3_trap(
	VOID);
FUNC_EXTERN DB_STATUS
tpd_u4_rqf_call(
	i4	i_call_id,
	RQR_CB	*v_rqr_p,
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_u5_gmt_now(
	TPR_CB	    *v_tpr_p,
	DD_DATE	    o_gmt_now);
FUNC_EXTERN DB_STATUS
tpd_u6_new_cb(
	i4		i1_stream,
	TPR_CB		*v_tpr_p,
	PTR		*o1_cbptr_p);
FUNC_EXTERN DB_STATUS
tpd_u7_term_any_cb(
	i4		i1_stream,
	PTR		*i2_cbptr_p,
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_u8_close_streams(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_u9_2pc_ldb_status(
	RQR_CB	    *i1_rqr_p,
	TPD_LX_CB   *o1_lxcb_p);
FUNC_EXTERN DB_STATUS
tpd_u10_fatal(
	DB_ERRTYPE  i1_errcode,
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tpd_u11_term_assoc(
	DD_LDB_DESC *i1_ldb_p,
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN VOID
tpd_u12_trim_cdbstrs(
	DD_LDB_DESC *i1_cdb_p,
	char	    *o1_cdb_node,
	char	    *o2_cdb_name,
	char	    *o3_cdb_dbms);
FUNC_EXTERN VOID
tpd_u13_trim_ddbstrs(
	TPC_I1_STARCDBS	    
    *i1_starcdb_p,
	char	    *o1_ddb_name,
	char	    *o2_ddb_owner,
	char	    *o3_cdb_name,
	char	    *o4_cdb_node,
	char	    *o5_cdb_dbms);
FUNC_EXTERN DB_STATUS
tpd_u14_log_rqferr(
	i4		 i1_rqfcall,
	RQR_CB		*i2_rqr_p,
	TPR_CB		*v_tpr_p);

FUNC_EXTERN DB_STATUS
tpf_m1_process(
	i4	i_call,
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m2_trace(
	TPR_CB	    *v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m3_abort_to_svpt(
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m4_savepoint(
	TPR_CB		*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m5_startup_svr(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m6_shutdown_svr(
	i4	v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m7_init_sess(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_m8_term_sess(
	TPR_CB	*v_tpr_p);
FUNC_EXTERN bool
tpf_m9_transition_ok(
	i4	i1_dxstate,
	i4	i2_call,
	TPR_CB	*v_tpr_p);
FUNC_EXTERN DB_STATUS
tpf_call(
	i4	i_request,
	TPR_CB	*v_tpr_p);
FUNC_EXTERN VOID
tpf_u0_tprintf(
	TPR_CB		*v_tpr_p,
	i4		cbufsize,
	char		*cbuf);
FUNC_EXTERN VOID
tpf_u1_p_sema4(
	i4		 i1_mode,
	CS_SEMAPHORE	*i2_sema4_p);
FUNC_EXTERN VOID
tpf_u2_v_sema4(
	CS_SEMAPHORE	*i1_sema4_p);
FUNC_EXTERN DB_STATUS
tpf_u3_msg_to_ldb(
	DD_LDB_DESC	*i1_ldb_p,
	bool		i2_commit_b,
	TPR_CB		*v_rcb_p);
FUNC_EXTERN DB_STATUS
tpq_d1_delete(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
FUNC_EXTERN DB_STATUS
tpq_i1_insert(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
FUNC_EXTERN DB_STATUS
tpq_i2_query(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p,
	char            *o_qry_p);
FUNC_EXTERN DB_STATUS
tpq_s1_select(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
FUNC_EXTERN DB_STATUS
tpq_s2_fetch(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
FUNC_EXTERN DB_STATUS
tpq_s3_flush(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
FUNC_EXTERN DB_STATUS
tpq_s4_prepare(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
FUNC_EXTERN DB_STATUS
tpq_u1_update(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p);
