/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: RQFPRVT.H - RQF private data strcutures.
**
** Description:
**      RQF internal control block and its substructural definitions.
**
** History:
**      aug-2-88 (alexh)
**          created. Spinned off from rqf.h
**	10-nov-90 (georgeg)
**	    added some RQF flags, RQF_NOALIAS_NODE etc.
**	12-dec-90 (fpang)
**	    Added fields rql_partner_name, rql_user_name, and rql_password
**	    to RQL_ASSOC, that will be used to stash the corresponding
**	    string for a gca_request.
**	04-mar-1991 (fpang)
**	    Added RQTRACE macro, and changed print_flag to rqf_dbg_lvl
**	    to allow different level of debugging info printed.
**      06-mar-1991 (fpang)
**          1. Changed TRdisplays to RQTRACE.
**          2. Make sure association id is in all trace messages.
**          3. Support levels of tracing.
**	13-aug-1991 (fpang)
**	    Changes to support rqf trace points.
**	15-oct-1992 (fpang)
**	    Sybil Phase 2 changes :
**	    1. Define RQF_DBG_LVL as a common way to get debug level.
**	    2. RQL_ASSOC - changed type of rqs_scb to (CS_SCB *) and
**	       defined RQF_SCB_TYPE to support scc_gcomplete() changes.
**	    3. RQS_CB - added rqs_cpstream and rqs_setstream for memory
**	       management. Moved rqs_memleft to RQF_CB.rqf_ulm_memleft.
**	    4. RQF_CB - added rqf_ulm_memleft for memory management.
**	18-jan-1993 (fpang)
**	    Register database procedure changes :
**	    1. Prototypes for rqu_invproc(), rqu_get_rpdata() and 
**	       rqu_put_pparms().
**       6-Oct-1993 (fred)
**          Changed interface for rqu_putformat() to enable support of
**          decimal.
**      14-Oct-1993 (fred)
**          Changed interface to rqu_fetch_data().
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered RQTRACE() macro to make it portable. This change was
**	    necessary because rqf_trace() requires a variable number of,
**	    and variable sized arguments. It cannot become a varargs function
**	    as this practice is outlawed in main line code.
**	    Altered RQTRACE() to invoke TRformat which handles varargs
**	    correctly.
**	    Introduced a new macro, RQFMT(), to supply the additional arguments
**	    needed by TRformat. The macro has two parameters: the TRformat
**	    buffer and format string.
**	    The RQFMT() macro specifies rqf_format() directly as the TRformat
**	    function, bypassing the old rqf_trace() function. The "do I need
**	    to add a newline" flag to rqf_format() is determined by calling
**	    a new function, rqf_neednl(), from within the macro expansion.
**	    Deleted references to the old rqf_trace() and added prototype
**	    declaration for rqf_neednl().
**	    Defined RQTRBUFSIZE as 512 which was the size of the encapsulated
**	    buffer in rqf_trace().
**	    Updated description and example of how to use RQTRACE.
**	21-feb-94 (swm)
**	    Bug #59883
**	    In my 11-oct-93 change, I inadvertently missed the format buffer
**	    from the end of the parameter list in the definition of the RQFMT
**	    macro. Now added.
**	    Added rqs_trfmt and rqs_size elements to RQS_CB; these point at
**	    QEF_CB counterparts which are a ulm buffer for TRformat calls and
**	    the buffer size.
**	    Amended RQTRACE macro to check that rqs_trfmt buffer is allocated.
**	    Amended RQFMT macro to add buffer size parameter.
**	    Re-arranged RQF_DBG_LVL() macro.
**	1-mar-94 (robf)
**          Temporarily fix tracing problem until swm integration, add
**	    flag so RQF knows whether to forward security labels.
**	06-mar-96 (nanpr01)
**          Added the parameter pagesize in the function prototype.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2002 (devjo01)
**	    Change size of rqf_vnode to CX_MAX_NODE_NAME_LEN.
**      7-oct-2004 (thaju02)
**          Define rqf_ulm_memleft as SIZE_TYPE.
**	28-Jan-2009 (kibro01) b121521
**	    Add use_ingresdate parameter to rqu_putformat
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype rqu_error() with variable params,
**	    delete declarations of dmf_call, dmf_trace.
*/


/*
** RQTRACE - generate trace messages.
**
** Description :
**     This is a little faster than a function call.
**
** Inputs :
** Outptus :
** Usage :
**     RQTRACE(rqs,n)(RQFMT(rqs,"trace text"),p1,p2);
** Note :
**     Trace text should not have embedded newlines, just one at the end.
**     Output trbuf must be provided by the caller.
**
**  1) Rqf_facility->rqf_dbg_lvl >= n  -> II_RQF_LOG set
**  2) s && s->rqs_trfmt && s->rqs->rqs_dbg_lvl >= n   -> session tracepoint
**
**  History:
**	15-oct-1992 (fpang)
**	    Defined RQF_DBG_LVL, and modified RQTRACE to use it.
**	    RQF_DBG_LVL provides a common method to determined the
**	    debug level.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered RQTRACE() macro to make it portable. This change was
**	    necessary because rqf_trace() requires a variable number of,
**	    and variable sized arguments. It cannot become a varargs function
**	    as this practice is outlawed in main line code.
**	    Altered RQTRACE() to invoke TRformat which handles varargs
**	    correctly.
**	    Introduced a new macro, RQFMT(), to supply the additional arguments
**	    needed by TRformat. The macro has two parameters: the TRformat
**	    buffer and format string.
**	    The RQFMT() macro specifies rqf_format() directly as the TRformat
**	    function, bypassing the old rqf_trace() function. The "do I need
**	    to add a newline" flag to rqf_format() is determined by calling
**	    a new function, rqf_neednl(), from within the macro expansion.
**	    Deleted references to the old rqf_trace() and added prototype
**	    declaration for rqf_neednl().
**	    Defined RQTRBUFSIZE as 512 which was the size of the encapsulated
**	    buffer in rqf_trace().
**	    Updated description and example of how to use RQTRACE.
**	21-feb-94 (swm)
**	    Bug #59883
**	    In my 11-oct-93 change, I inadvertently missed the format buffer
**	    from the end of the parameter list in the definition of the RQFMT
**	    macro. Now added.
**	    Added rqs_trfmt and rqs_size elements to RQS_CB; these point at
**	    QEF_CB counterparts which are a ulm buffer for TRformat calls and
**	    the buffer size.
**	    Amended RQTRACE macro to check that rqs_trfmt buffer is allocated.
**	    Amended RQFMT macro to add buffer size parameter.
**	    Re-arranged RQF_DBG_LVL() macro.
**	30-Jul-2010 (kschendel) b124164
**	    Leave room for newline, null; since TR likes to eat 'em, we'll
**	    add our own.
*/

#define	    RQF_DBG_LVL(s,n) \
  ( ( ( s != (RQS_CB *)NULL) && (s->rqs_trfmt != (char *)NULL) ) && \
    ( (Rqf_facility->rqf_dbg_lvl >= n) || (s->rqs_dbg_lvl >= n) ) \
  ) 

#define	    RQTRACE(s,n)	if (RQF_DBG_LVL(s,n)) TRformat

#define	    RQTRBUFSIZE		512
#define	    RQFMT(s,f)	rqf_format,rqf_neednl(f),s->rqs_trfmt,s->rqs_trsize-2,f

#define     RQF_NOALIAS_NODE	0x0001L /* connect to a remote node, need */
					/* password also */
#define     RQF_PASS_REQUIRED	0x0002L /* need the password */



/*}
** Name: RQF_CB - RQF facility-wide control block
**
** Description:
**	RQF_CB store RQF facility attributes.
**
** History:
**      may-26-88 (alexh)
**		created
**	nov-21-88 (alexh)
**		removed dummy DB_RQF_ID definition
**	14-jul-1990 (georgeg)
**		added RQF_CB.rqf_num_nodes, RQF_CB.rqf_cluster_p
**	04-mar-1991 (fpang)
**		Changed print_flag to rqf_dbg_lvl.
**	15-oct-1992 (fpang)
**		Added rqf_ulm_memleft for memory management.
**	1-mar-94 (robf)
**              Added rqf_flagsmask
*/
#define	RQF_CB_TYPE	1957
typedef struct _RQF_CB
{
    PTR		    rqf_pool_id;     /* ulm pool id */
    char	    rqf_vnode[CX_MAX_NODE_NAME_LEN];
    u_i4	    rqf_len_vnode;	
    i4		    rqf_dbg_lvl;
    PTR		    rqf_cluster_p; /* cluster node names */
    i4	    rqf_num_nodes; /* number of nodes in the cluster */
    SIZE_TYPE	    rqf_ulm_memleft;  /* memleft across all streams */
    i4	    rqf_flagsmask;
} RQF_CB;

GLOBALREF	RQF_CB	*Rqf_facility;

/*}
** Name: RQC_GCA - communications state information with GCA.
**
** Description:
**	
**
** History:
**      jul-7-89 (georgeg)
**		Created.
*/
typedef struct _RQC_GCA
{
    i4		rqc_rxtx;		/* are we in the process of readind or writing ? */
#define	    RQC_IDDLE		    0x0000L	/* there is no GCA_SEND or GCA_RECEIVE outstanding */
#define	    RQC_RECV		    0x0001L	/* there is a GCA_RECEIVE outstanding */
#define	    RQC_SEND		    0x0002L	/* there is a GCA_SEND oustanding */
    i4		rqc_status;		/* generic status of association */
#define	    RQC_OK		    0x0000L
#define	    RQC_ASSOC_TERMINATED    0x0001L	/* it cannot be used any more but GCA still keep resorces allocated*/
#define	    RQC_TIMEDOUT	    0x0002L	/* the association timed out */
#define	    RQC_INTERRUPTED	    0x0003L	/* the association was interrupted */
#define	    RQC_GCA_ERROR	    0x0004L	/* GCA has reported an error on the association, see rqc_gca_status */
#define	    RQC_DISASSOCIATED	    0x0005L	/* the association has been dissassocited */
STATUS			rqc_gca_status;		/* the error status from gca */
CL_SYS_ERR		rqc_os_gca_status;	/* the error os status that GCA reported if any*/ 
} RQC_GCA;

/*}
** Name: RQL_ASSOC - LDB association information
**
** Description:
**	RQL_ASSOC is a list token of an LDBMS association information.
**      This is used to track a RQF session associations. This block
**	should be used to store LDB session context.
**
** History:
**      may-26-88 (alexh)
**		created
**      jul-7-89 (georgeg)
**		added .rql_comm.
**      09-nov-89 (georgeg)
**		added .rql_rqr_cb, this is a pointer to the RQR_CB of RQF. The 
**		caller of rqf_call() constructs and fills the request control 
**		we migh need access to thee info of that  RCB.
**	15-oct-192 (fpang)
**		Changed type of rql_scb to (CS_SCB *), and defined RQF_SCB_TYPE
**		to set rql_scb->cs_client type to. Scc_gcomplete() now resumes
**		sessions based on cs_client_type.
*/
typedef struct _RQL_ASSOC
{
    i4			rql_assoc_id;			/* GCA association id */
    DD_LDB_DESC		rql_lid;			/* LID specifies by QEF */
    i4			rql_alias_node;			/* are we connected on */
							/* a cluster alias node */
    i4			rql_noerror;			/* if this flag is set no error */
							/* will be reported to FE */
#define RQF_GCBFSIZE	1024
    char	    	rql_write_buff[RQF_GCBFSIZE];	/* gca write buffer */
    GCA_SD_PARMS	rql_sd;			/* gca send struct */
    char	    	rql_read_buff[RQF_GCBFSIZE];    /* gca read buffer */
    GCA_RV_PARMS	rql_rv;				/* receive struct */
    bool		rql_described;			/* tuple desc received */
    u_i2		rql_b_remained;			/* bytes unretrieved */
    char		*rql_b_ptr;			/* next byte */
    GCA_TX_DATA		rql_txn_id;			/* transaction id */
    struct _RQL_ASSOC	*rql_next;			/* remaining list of LDBMS associations */
    bool		rql_interrupted;		/* interrupt indicator */
    CS_SCB		*rql_scb;			/* asynch id for gca completion */
#define RQF_SCB_TYPE    CV_C_CONST_MACRO('R','Q','F','+');
    i4			rql_last_request;		/* last known request */
    bool		rql_turned_around;		/* flow turned around */
    i4		rql_re_status;			/* GCA_RE_DATA gca_rqstatus & GCA_LOG_TERM_MASK */
    bool		rql_diff_arch;			/* assoc partners h/w, s/w  */
    u_i4		rql_nostatments;		/* number of statments executed */
    i4			rql_timeout;			/* time in sec to wait on the association  */
    i4		rql_ldb_severity;		/* the severity of the LDB reported error */
    i4		rql_id_error;			/* the error # from the ldb if any */
    i4		rql_local_error;		/* the error # from the ldb if any */
    i4		rql_status;			/* the status of the association; not of the comm channel */
#define	    RQL_OK		    0x0000L		/* the association is doing well */
#define	    RQL_NOTESTABLISHED	    0x0001L		/* the association has not been established yet */
#define	    RQL_TIMEDOUT	    0x0002L		/* association timed out because of a user specified timeout */
#define	    RQL_TERMINATED	    0x0003L		/* the association has not been established yet */
#define	    RQL_LDB_ERROR	    0x0004L		/* the  association received an error block from the LDB*/
    RQC_GCA		rql_comm;			/* status of the association with GCA, of the comm channel */
    i4		rql_peer_protocol;		/* the GCA_PROTOCOL_LEVEL of the peer */
    RQR_CB		*rql_rqr_cb;			/* a pointer to the "request contol block" of RQF */
    char		*rql_partner_name;
    char		*rql_user_name;
    char		*rql_password;
} RQL_ASSOC;

/*}
** Name: RQS_CB - RQF session control block
**
** Description:
**	RQS_CB is used to store a RQF session information.
**      RQS_CB is initialized by calling rqf_initialize.
**
** History:
**      may-26-88 (alexh)
**		created
**	feb-02-89 (georgeg)
**		added "DD_PACKET *rqs_set_options" for the 
**		SQL/QUEL set options support.
**	15-oct-1992 (fpang)
**	    Added rqs_cpstream and rqs_setstream to manage memory
**	    for copy maps and set statements. Moved rqs_memleft to
**	    RQF_CB.rqf_ulm_memleft.
**	10-mar-94 (swm)
**	    Bug #59883
**	    Added rqs_trfmt and rqs_size to RQS_CB. This is to share use of
**	    the QEF TRformat trace buffer. This buffer is used in preference
**	    to an automatic buffer which could give rise to stack overflow
**	    on deeply nested calls.
*/
typedef struct _RQS_CB
{
    RQF_HEADER	    rqs_header;		/* standard control block header */
    RQF_CB	    *rqs_facility;	/* facility-wide control block */
    PTR		    rqs_streamid;	/* session ulm stream id */
    RQL_ASSOC	    *rqs_current_site;	/* site currently used */
    RQL_ASSOC	    *rqs_assoc_list;	/* list of LDB associations */
    RQL_ASSOC	    *rqs_dcnct_assoc;	/* Used only for dirrect connect */
    bool	    rqs_dc_mode;	/* Are we in direct connect mode or not */
    DB_LANG	    rqs_lang;		/* session language type */
    DD_PACKET	    *rqs_set_options;	/* list of set options SQL/QUEL*/
    DD_LDB_DESC	    rqs_cdb_name;	/* name of the CDB LDB */
    RQL_ASSOC	    *rqs_user_cdb;	/* a pointer to the CDB association */
					/*  that is used by the user and does */ 
					/*  not have a ingres_b flag set */
    DD_MODIFY	    rqs_mod_flags;	/* the +/-U, +/-Y flags of the user CDB assoc */ 
    DD_USR_DESC	    rqs_user;
    DD_COM_FLAGS    rqs_com_flags;	/* FE command flags to be propagated to the user */
    i4		    rqs_dbg_lvl;	/* session debug level */
    char 	    *rqs_trfmt;		/* pointer to QEF_CB TRformat buffer */
    i4		    rqs_trsize;		/* size of QEF_CB TRformat buffer */
    PTR		    rqs_cpstream;	/* ULM stream for cp maps */
    PTR		    rqs_setstream;	/* ULM stream for set queries */
} RQS_CB;

/*
** RQF TRACE POINTS
*/

#define RQT_128_LOG 120		/* RQF LOGGING */

/**
** Name: RQFPROTO - Prototype defintions for RQF
**
** Description:
**      This contains the prototype defintions for RQF.
**
** History:
**      19-oct-92 (teresa)
**          Initial creation
[@history_template@]...
**/

/*********************
** Module RQFMAIN.C **
*********************/

FUNC_EXTERN DB_ERRTYPE rqf_abort(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_begin(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_close(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_commit(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_query(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_define(	RQR_CB		*rqr_cb, 
					bool		gca_qry);
FUNC_EXTERN DB_ERRTYPE rqf_update(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_open_cursor(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_td_get(	RQR_CB		*rqr_cb, 
					RQL_ASSOC	*assoc);
FUNC_EXTERN DB_ERRTYPE rqf_delete(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_end(		RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_fetch(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_s_begin(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_s_end(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_interrupt(	RQR_CB		*rqr_cb, 
					i4		interrupt_type);
FUNC_EXTERN DB_ERRTYPE rqf_invoke(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_continue(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_secure(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_shutdown(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_startup(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_t_fetch(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_nt_fetch(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_endret(	RQR_CB		*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqf_transfer(	RQR_CB		*src_cb);
FUNC_EXTERN DB_ERRTYPE rqf_cp_get(	RQL_ASSOC	*assoc, 
					PTR		*td_data);
FUNC_EXTERN DB_ERRTYPE rqf_write(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_connect(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_disconnect(	RQR_CB		*rcb);
FUNC_EXTERN VOID rqf_check_interrupt(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_set_option(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_modify(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_cleanup(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_term_assoc(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_ldb_arch(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_restart(	RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_cluster_info(RQR_CB		*rcb);
FUNC_EXTERN DB_ERRTYPE rqf_set_trace(	RQR_CB		*rcb);
FUNC_EXTERN i4 rqf_format(PTR arg, i4 len, char buffer[]);
FUNC_EXTERN i4 * rqf_neednl(char fmt[]);
FUNC_EXTERN DB_STATUS rqf_call(		i4		rqr_type, 
					RQR_CB		*rqr_cb);

/* 
**  LGcluser() and LGcnode_info are defined in LG.H, so they are NOT included 
**  here to avoid error messages about double definitions
**  of constants.
**	FUNC_EXTERN bool LGcluster(	VOID);
**	FUNC_EXTERN VOID LGcnode_info(	char		*name, 
**					i4		l_name, 
**					CL_SYS_ERR	*sys_err);
*/

/********************
** Module RQUTIL.C **
********************/

FUNC_EXTERN VOID rqu_gcmsg_type(	i4		direction, 
					i4		typ, 
					RQL_ASSOC	*assoc);
FUNC_EXTERN VOID rqu_dbg_error(		RQL_ASSOC	*assoc, 
					i4		error);
FUNC_EXTERN VOID rqu_client_trace(	i4		msg_length, 
					char		*msg_buffer);
FUNC_EXTERN VOID rqu_relay(		RQL_ASSOC	*assoc, 
					GCA_RV_PARMS	*rv);
FUNC_EXTERN VOID rqu_error(		i4		log_error_only, 
  					i4		rqf_err, 
  					PTR		os_error, 
  					i4		num_parms, 
					... );
FUNC_EXTERN VOID rqu_gca_error(		RQL_ASSOC	*assoc, 
					GCA_ER_DATA	*gca_error_blk);
FUNC_EXTERN RQL_ASSOC *rqu_find_lid(	DD_LDB_DESC	*site, 
					RQL_ASSOC	*list);
FUNC_EXTERN DB_ERRTYPE rqu_send(	RQL_ASSOC	*assoc, 
					i4		msg_size,
					i4		msg_cont, 
					i4		msg_type);
FUNC_EXTERN VOID rqu_release_assoc(	RQL_ASSOC	*assoc);
FUNC_EXTERN VOID rqu_end_all_assoc(	RQR_CB		*rcb, 
					RQL_ASSOC	*assoc);
FUNC_EXTERN VOID rqu_dump_gca_msg(	RQL_ASSOC	*assoc);
FUNC_EXTERN VOID rqu_putinit(		RQL_ASSOC	*assoc, 
					i4		msg_type);
FUNC_EXTERN DB_ERRTYPE rqu_putflush(	RQL_ASSOC	*assoc, 
					bool		eod);
FUNC_EXTERN DB_ERRTYPE rqu_put_data(	RQL_ASSOC	*assoc, 
					char		*data, 
					i4		len);
FUNC_EXTERN DB_ERRTYPE rqu_putdb_val(	i4		data_type, 
					i4		precision, 
					i4		data_len, 
					char		*data_ptr, 
					RQL_ASSOC	*assoc);
FUNC_EXTERN DB_ERRTYPE rqu_putqid(	RQL_ASSOC	*assoc, 
					DB_CURSOR_ID	*qid);
FUNC_EXTERN DB_ERRTYPE rqu_putcid(	RQL_ASSOC	*assoc, 
					DB_CURSOR_ID	*cid);
FUNC_EXTERN VOID rqu_putformat(		DB_DATA_VALUE   *input,
					DB_LANG		lang, 
					char		*buffer,
					bool		use_ingresdate);
FUNC_EXTERN VOID rqu_cluster_info(	i4		*flags, 
					DD_NODE_NAME	*alt_node_name,
					DD_NODE_NAME	*node_name);
FUNC_EXTERN RQL_ASSOC  *rqu_new_assoc(	RQR_CB		*rqr_cb,
					RQS_CB		*session,
					DD_LDB_DESC	*ldb,
					DD_USR_DESC	*user_name, 
					DD_COM_FLAGS	*flags, 
					RQL_ASSOC	*assocmem_ptr,
					DD_DX_ID	*restart_dx_id);
FUNC_EXTERN RQL_ASSOC *rqu_ingres_priv(	RQL_ASSOC	*assoc,
					bool		ingres_b);
FUNC_EXTERN RQL_ASSOC *rqu_check_assoc(	RQR_CB		*rcb,
					i4		allocate);
FUNC_EXTERN i4  rqu_get_reply(		RQL_ASSOC	*assoc, 
					i4		timeout);
FUNC_EXTERN DB_ERRTYPE rqu_create_tmp(	RQL_ASSOC	*src_assoc,
					RQL_ASSOC	*dest_assoc,
					i4		lang,
					DD_ATT_NAME	**col_name_tab,
					DD_TAB_NAME	*table_name,
					i4		pagesize);
FUNC_EXTERN STATUS rqu_xsend(		RQL_ASSOC	*assoc,
					GCA_SD_PARMS	*send);
FUNC_EXTERN DB_STATUS rqu_mod_assoc(	RQS_CB		*session,
					RQL_ASSOC	*assoc,
					char		*dbname,
					DD_COM_FLAGS	*flags,
					DD_DX_ID	*restart_dx_id);
FUNC_EXTERN DB_ERRTYPE rqu_fetch_data(	RQL_ASSOC	*assoc,
					i4		fsize,
					PTR		dest,
					bool		*eod_ptr,
				        i4              tuple_fetch,
				        i4              *fetched_size,
				        i4              *eo_tuple);
FUNC_EXTERN DB_ERRTYPE rqu_getqid(	RQL_ASSOC	*assoc,
					DB_CURSOR_ID	*qid);
FUNC_EXTERN DB_ERRTYPE rqu_col_desc_fetch(
					RQL_ASSOC	*assoc,
					DB_DATA_VALUE	*col);
FUNC_EXTERN DB_ERRTYPE rqu_td_save(	i4		col_count,
					RQB_BIND	*bind,
					RQL_ASSOC	*assoc);
FUNC_EXTERN DB_ERRTYPE rqu_convert_data(RQB_BIND	*bind);
FUNC_EXTERN DB_ERRTYPE rqu_interrupt(	RQL_ASSOC	*assoc,
					i4		interrupt_type);
FUNC_EXTERN DB_ERRTYPE rqu_set_option(	RQL_ASSOC	*assoc,
					i4		msg_len,
					char		*msg_ptr,
					DB_LANG		lang);
FUNC_EXTERN STATUS rqu_gccall(		i4		req,
					PTR		msg,
					STATUS		*poll_addr,
					RQL_ASSOC	*assoc);
FUNC_EXTERN STATUS rqu_txinterrupt(	i4		req,
					PTR		msg,
					STATUS		*poll_addr,
					RQL_ASSOC	*assoc);
FUNC_EXTERN STATUS rqu_rxiack(		i4		req,
					PTR		msg,
					STATUS		*poll_addr,
					RQL_ASSOC	*assoc);
FUNC_EXTERN VOID rqu_dmp_gcaqry(	GCA_SD_PARMS	*sd,
					RQL_ASSOC	*assoc);
FUNC_EXTERN DB_ERRTYPE rqu_sdc_chkintr(	RQR_CB		*rcb);
FUNC_EXTERN VOID rqu_dmp_dbdv(		RQS_CB		*rqs,
					RQL_ASSOC	*assoc,
					DB_DT_ID	data_type,
					i2		precision,
					i4		data_len,
					PTR		data_ptr);
FUNC_EXTERN VOID rqu_dmp_tdesc(		RQL_ASSOC	*assoc,
					GCA_TD_DATA	*td_data);
FUNC_EXTERN DB_ERRTYPE rqu_put_qtxt(	RQL_ASSOC	*assoc,
					i4		msgtype,
					i4		qlanguage,
					i4		qmodifier,
					DD_PACKET	*ddpkt,
					DD_TAB_NAME	*tmpnm);
FUNC_EXTERN DB_ERRTYPE rqu_put_parms(	RQL_ASSOC	*assoc,
					i4		count,
					DB_DATA_VALUE	*dv);
FUNC_EXTERN DB_ERRTYPE rqu_put_iparms(	RQL_ASSOC	*assoc,
					i4		count,
					DB_DATA_VALUE	**dv);
FUNC_EXTERN DB_ERRTYPE rqu_put_moddata( RQL_ASSOC       *assoc,
					i4         index,
					i4             type,
					i4             prec,
					i4         leng,
					PTR             data);
FUNC_EXTERN DB_ERRTYPE rqf_invproc( 	RQR_CB  	*rqr_cb);
FUNC_EXTERN DB_ERRTYPE rqu_get_rpdata( 	RQL_ASSOC       *assoc,
					DB_CURSOR_ID	*pid,
					i4		*dbp_stat);
FUNC_EXTERN DB_ERRTYPE rqu_put_pparms( 	RQL_ASSOC       *assoc,
					i4             count,
					QEF_USR_PARAM    **prms);

