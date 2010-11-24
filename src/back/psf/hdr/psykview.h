/* psykview.c */
FUNC_EXTERN i4 psy_kview(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb,
	QEU_CB *qeu_cb);
FUNC_EXTERN i4 psy_basetbl(
	PSS_SESBLK *sess_cb,
	DB_TAB_ID *view_id,
	DB_TAB_ID *tbl_id,
	DB_ERROR *err_blk);
FUNC_EXTERN i4 psy_kregproc(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb,
	DB_TAB_NAME *procname);

