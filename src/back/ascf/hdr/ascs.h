/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ASCS.H - defines and function prototypes for ascs. 
**
** Description:
**     Function prototypes for ascs specific functions.
**
** History:
**      18-Jan-1999 (fanra01)
**          Created.
**      12-Feb-99 (fanra01)
**          Add prototypes for ascs_ctlc, ascs_scan_scbs and ascs_remove_sess.
**      16-Feb-1999 (fanra01)
**          Add prototype for ascs_chk_priv.
**      05-Jun-2000 (fanra01)
**          Bug 101345
**          Add prototype for ascs_gca_clear_outbuf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Dec-2001 (gordy)
**	    Removed unneeded parameter from ascs_get_data().
*/

/*
**  Forward and/or External typedef/struct references.
*/

/*
** Func Externs of (some) things exported outside scs
*/

FUNC_EXTERN i4 ascs_initiate(SCD_SCB *scb );

FUNC_EXTERN DB_STATUS ascs_terminate(SCD_SCB *scb );

FUNC_EXTERN DB_STATUS ascs_add_vardata(SCD_SCB *scb, GCA_DATA_VALUE *gdv);
FUNC_EXTERN DB_STATUS ascs_gca_get(SCD_SCB *scb, PTR get_ptr, i4 get_len);
FUNC_EXTERN i4  ascs_gca_data_available(SCD_SCB *scb);

FUNC_EXTERN DB_STATUS ascs_bld_query(SCD_SCB *scb, i4  *next_state);
FUNC_EXTERN DB_STATUS ascs_get_language_type ( SCD_SCB *scb, u_i4* langid);

FUNC_EXTERN VOID ascs_ctlc(SCD_SCB *sid );
FUNC_EXTERN STATUS ascs_atfactot(SCF_CB *scf );

/* ascsgcaint.c */
FUNC_EXTERN DB_STATUS ascs_gca_clear_outbuf(SCD_SCB *scb);
FUNC_EXTERN VOID ascs_gca_error(DB_STATUS status, i4 err_code, 
				i4 error_blame );
FUNC_EXTERN DB_STATUS ascs_gca_put(SCD_SCB *scb, i4  msgtype, i4 len, 
				PTR data);
FUNC_EXTERN DB_STATUS ascs_gca_clear_outbuf(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_gca_outbuf(SCD_SCB *scb, i4 msg_len, 
			char *msg_ptr);
FUNC_EXTERN DB_STATUS ascs_gca_flush_outbuf(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_gca_recv(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_gca_send(SCD_SCB *scb);


FUNC_EXTERN STATUS ascs_get_data( SCD_SCB *scb, i4 row, i4 col, 
				  DB_DATA_VALUE *db_data);
FUNC_EXTERN DB_STATUS ascs_format_tuples(SCD_SCB *scb, URSB *ursb);
FUNC_EXTERN DB_STATUS ascs_format_tuple(SCD_SCB *scb, i4  row, URSB *ursb);
FUNC_EXTERN DB_STATUS ascs_format_results(SCD_SCB *scb, URSB *ursb);
FUNC_EXTERN DB_STATUS ascs_format_scanid(SCD_SCB *scb);

/* ascsdbfcn.c */
DB_STATUS ascs_dbms_task (SCD_SCB *scb );

/* ascseauth.c */
FUNC_EXTERN DB_STATUS ascs_check_external_password ( SCD_SCB *scb,
        DB_OWN_NAME *authname, DB_OWN_NAME *e_authname, DB_PASSWORD *password,
        bool        auth_role);

FUNC_EXTERN DB_STATUS ascs_bld_procedure(SCD_SCB *scb, URSB *ursb, char *Method,
                            char *Interface);
FUNC_EXTERN DB_STATUS ascs_dispatch_procedure(SCD_SCB *scb, URSB *ursb);
FUNC_EXTERN DB_STATUS ascs_finish_procedure(SCD_SCB *scb, URSB *ursb);
FUNC_EXTERN DB_STATUS ascs_get_parm(SCD_SCB *scb, i4  *ParmNameLen, 
			char *ParmName, i4 *ParmMask, 
			GCA_DATA_VALUE *value_hdr, char *Value);
FUNC_EXTERN STATUS ascs_disassoc(i4 assoc_id );
FUNC_EXTERN i4 ascs_trimwhite( i4 len, char *buf );

/* ascsformat.c */
FUNC_EXTERN DB_STATUS ascs_format_tuples(SCD_SCB *scb, URSB *ursb);
FUNC_EXTERN DB_STATUS ascs_format_response(SCD_SCB *scb, i4 resptype,
    i4 qrystatus, i4 rowcount);

/* ascssvc.c */
FUNC_EXTERN DB_STATUS ascs_declare(SCF_CB *scf_cb );

FUNC_EXTERN DB_STATUS ascs_clear(SCF_CB *scf_cb );

FUNC_EXTERN DB_STATUS ascs_disable(SCF_CB *scf_cb );

FUNC_EXTERN DB_STATUS ascs_enable(SCF_CB *scf_cb );

FUNC_EXTERN STATUS ascs_attn(i4 eid, SCD_SCB *scb );
FUNC_EXTERN STATUS ascs_format(SCD_SCB *scb, char *stuff, i4  powerful,
				i4 suppress_sid );
FUNC_EXTERN i4  ascs_facility(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_i_interpret(SCD_SCB *scb );
FUNC_EXTERN STATUS ascs_iformat(SCD_SCB *scb, i4 powerful, i4 suppress_sid,
    i4 err_output );
FUNC_EXTERN bool ascs_chk_priv( char *user_name, char *priv_name );

/* ascsmntr.c */
FUNC_EXTERN void ascs_scan_scbs( SCS_SCAN_FUNC *func, PTR arg );

FUNC_EXTERN void ascs_remove_sess( SCD_SCB *scb );
FUNC_EXTERN int ascs_msequencer(i4 op_code, SCD_SCB  *scb, i4 *next_op );

/* ascsmo.c */
FUNC_EXTERN VOID ascs_mo_init(void);
FUNC_EXTERN STATUS ascs_scb_attach( SCD_SCB *scb );
FUNC_EXTERN STATUS ascs_scb_detach( SCD_SCB *scb );

/* ascsqncr.c */
FUNC_EXTERN DB_STATUS ascs_process_interrupt(SCD_SCB *scb);

/* ascsquery.c */
FUNC_EXTERN DB_STATUS ascs_dispatch_query(SCD_SCB *scb);

/* ascsstub.c */
FUNC_EXTERN DB_STATUS ascs_dbadduser(SCF_CB    *scf_cb);
FUNC_EXTERN DB_STATUS ascs_dbdeluser(SCF_CB    *scf_cb);
