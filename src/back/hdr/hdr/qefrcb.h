/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: QEFRCB.H - data structures for requesting QEF operations.
**
** Description:
**      This file contains the main control block, QEF_RCB, used in
**	requesting various QEF operations, and other data structures
**	refered to in the QEF_RCB.
**
** History:
**      10-jun-86 (daved)
**	    created.
**	28-apr-88 (puree)
**	    added QEF_USR_PARAM structure and modified QEF_RCB to add
**	    the pointer to QEF_USR_PARAM array used in executing a
**	    DB procedure.
**	20-aug-88 (carl)
**	    Added Titan changes:
**		1.  DDB request control block, QEF_DDB_REQ,
**		2.  new fileds qef_r1_distrib, qef_r2_qef_lvl, and
**		    qef_r3_ddb_req in QEF_RCB.
**	13-dec-88 (puree)
**	    document the meanings of qef_param struct
**	17-jan-89 (carl)
**	    modified QEF_DDB_REQ
**	24-jan-89 (puree)
**	    update qef_param description.
**	14-feb-89 (carl)
**	    added qef_d10_tupdesc_p to QEF_DDB_REQ allow RQF to return LDB's 
**	    tuple descriptor to SCF
**	20-feb-89 (paul)
**	    Added field to support passing procedure call parameters as an
**	    array of parameters rather than an array of pointers to parameters.
**	    Both schemes are now supported.
**	03-mar-89 (paul)
**	    Added qef_intstate. This makes some internal state information for
**	    QEF available to SCF so it knows if rules processing is in progress.
**	01-apr-89 (paul)
**	    Added qef_context_cnt to record how many nested QP's are currently
**	    active.
**	17-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Changed format of QEF_ALT;
**	    Defined constants for QEF_ALT.alt_code.
**	17-apr-89 (paul)
**		Added qef_rule_depth to replace incorrect use of qef_stat to
**		count the current rule nesting depth.
**		Added qef_mode as space to remember the current QEQ mode.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Defined QEC_IGNORE request code for qec_alter.
**	28-jan-90 (carl)
**	    added QEF_04DD_RECOVERY (define) to qer_d13_ctl_info of 
**	    QER_DDB_REQ to enable QED_RECOVER_P1 to indicate to SCF
**	    whether recovery is necessary or not 
**	30-jan-90 (ralph)
**	    Add alt_i4, QEC_USTAT to QEF_ALT for passing user status bits to QEF
**	08-aug-90 (ralph)
**	    Fix UNIX portability problems
**	12-feb-91 (mikem)
**	    Added two new fields qef_val_logkey and qef_logkey to the QEF_RCB
**	    structure to support returning logical key values to the client
**	    which caused the insert of the tuple and thus caused them to be
**	    assigned.
**	25-feb-91 (rogerk)
**	    Added QEF_ALT mode QEC_ONERROR_STATE for setting transaction
**	    error handling mode.  Added qef_stm_error field in which to
**	    return the error handling mode from qec_info calls.
**	    Part of SET SESSION WITH ON_ERROR changes.
**      11-dec-1991 (fred)
**	    Alter meaning of qef_count field.  This field was previously unused.
**	    It is now used to indicate the number of filled buffers which were
**	    returned from qef_call().  This is different than the number of
**	    returned rows in cases where large object/peripheral types are
**	    returned.  These may take up many buffers, yet not comprise a whole
**	    row.  Therefore, both a row count (qef_rowcount (an input/output
**	    parameter) and a buffer count (qef_count) are necessary.
**
**	    This field may also be used if we ever get around to having the
**	    ability to send compressed datatypes (i.e. NULLs only one byte,
**	    varchar taking up only the number of characters needed, etc.)
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**      26-jun-92 (anitap)
**          Added qef_upd_rowcnt for setting the value of rowcount for update
**          in QEF_RCB.
**	26-oct-92 (jhahn)
**	    Added parm_flags and QEF_IS_BYREF to QEF_USR_PARAM 
**	    for DB procedures..
**      27-oct-92 (anitap)
**          Added pointer to query text handle for CREATE SCHEMA/constraints 
**	    project. This pointer is passed on to SCF by QEF so that PSF can be
**	    called to parse the query text for statements in CREATE SCHEMA and
**	    rules and procedures for constraints to resolve backward references. Also 
**	    added new defines to qef_intstate to indicate to SCF that 
**	    QEA_EXEC_IMM type of action is being processed.
**	08-dec-92 (rickh)
**	    Added QEF_TUPLE_PACKER for FIPS CONSTRAINTS (6.5).
**	12-jan-93 (jhahn)
**	    Added qef_dbpId and qef_setInputId to qefrcb for set input
**	    procedures.
**	15-apr-93 (jhahn)
**	    Added qef_no_xact_stmt and qef_illegal_xact_stmt to qefrcb 
**	    for catching transaction control statements when they are
**	    disallowed.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added qef_dbxlate to QEF_RCB, which points to case translation mask.
**	    Added qef_cat_own to QEF_RCB, which points to catalog owner name.
**	24-jun-93 (anitap)
**	    Added qef_lk_id to QEF_RCB to pass on the lock id of the execute 
**	    immediate query text object to SCF to be used for QSO_DESTROY. 
**	    Changed QEF_SAVED_RSRC to QEF_DBPROC_QP in QEF_RCB.
**      26-july-1993 (jhahn)
**          Added parm_index to QEF_USR_PARAM for handling byref parameters
**	13-sep-93 (smc)
**	    Made qef_dmf_id & qef_sess_id a CS_SID.
**      26-oct-93 (anitap)
**	    Added a qef_stmt_type and flag QEF_DGTT_AS_SELECT to set for
**	    "declare global temporary table as..".	
**	25-nov-93 (robf)
**          Add qef_group/aplid to pass session group/role values
**      21-jan-94 (iyer)
**          Add QEF_RCB.qef_tran_id in support of dbmsinfo('db_tran_id').
**	04-apr-94 (anitap)
**	    Added ulm_streamid & ulm_memleft to QEF_RCB.
**	    We want to remember these particulars for the memory
**	    stream that we open in QEF for E.I. query text so that we
**	    close the stream after we are done with execute immediate
**	    processing.
**      13-jun-97 (dilma04)
**          Addded QEF_CONSTR_PROC flag to QEF_RCB indicating that QEF is 
**          processing an action associated with an integrity constraint.
**	04-jan-2000 (rigka01) bug#99199
**	    add another value for qeu_flag so that qeu_btran can recognize
**	    db procedure statements.  new value is QEF_DBPROC. 
**	10-Jan-2001 (jenjo02)
**	    Deleted redundant qef_dmf_id, use qef_sess_id instead.
**      19-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**          Added QEF_RDF_INTERNAL and QEF_RDF_MSTRAN as new qef_stmt_type(s).
**      12-Apr-2004 (stial01)
**          Define qef_length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
{@history_template@}
**/

typedef struct _QEF_ALT QEF_ALT;
typedef struct _QEF_DATA QEF_DATA;
typedef struct _QEF_PARAM QEF_PARAM;
typedef struct _QEF_RCB QEF_RCB;
typedef struct _QEF_CB	QEF_CB;
typedef struct _QEU_COPY QEU_COPY;

/*}
** Name: QEF_ALT - alter constant array element
**
** Description:
**      These elements are used to specify the constant to change and
**	its new value.
**
** History:
**     29-apr-86 (daved)
**          written
**	17-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Changed format of QEF_ALT;
**	    Defined constants for QEF_ALT.alt_code.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Defined QEC_IGNORE request code for qec_alter.
**	30-jan-90 (ralph)
**	    Add alt_i4, QEC_USTAT to QEF_ALT for passing user status bits to QEF
**	08-aug-90 (ralph)
**	    Fix UNIX portability problems
**	25-feb-91 (rogerk)
**	    Added QEF_ALT mode QEC_ONERROR_STATE for setting transaction
**	    error handling mode.  Part of SET SESSION WITH ON_ERROR changes.
**	03-feb-93 (andre)
**	    defined QEC_DBID, QEC_UDBID, and QEC_EFF_USR_ID + added alt_own_name
**	    to alt_value.  QEC_RESET_EFF_USER_ID will be replaced with a call to
**	    QEC_ALTER (very soon)
**/
struct _QEF_ALT
{
    i4      alt_length;		/* Length of item */
    i4      alt_code;		/* Item code */
#define		QEC_DBPRIV	 1  /* Database privilege mask; u_i4   */
#define		QEC_QROW_LIMIT	 2  /* Database privilege mask; i4	    */
#define		QEC_QDIO_LIMIT	 3  /* Database privilege mask; i4	    */
#define		QEC_QCPU_LIMIT	 4  /* Database privilege mask; i4	    */
#define		QEC_QPAGE_LIMIT	 5  /* Database privilege mask; i4	    */
#define		QEC_QCOST_LIMIT	 6  /* Database privilege mask; i4	    */
#define		QEC_ONERROR_STATE 7 /* Query err-handling type; i4     */
#define		QEC_IGNORE	 8  /* Ignore this entry		    */
#define		QEC_USTAT	 9  /* User permissions			    */
#define		QEC_DBID	10  /* database id; PTR			    */
#define		QEC_UDBID	11  /* unique database id; i4		    */
#define		QEC_EFF_USR_ID	12  /* effective user id; DB_OWN_NAME	    */
#define		QEC_MAX_ALT	12  /* Maximum value for alt_code	    */
    union
    {
	char	    *alt_char;	    /* Character value */
	u_i4	    alt_u_i4;  /* Unsigned i4 value */
	i4	    alt_i4;	    /* i4 value */
	PTR	    alt_ptr;	    /* Pointer value */
	DB_OWN_NAME *alt_own_name;  /* DB_OWN_NAME value */
    }	    alt_value;
    i4	    *alt_rlength;	/* Result length */
};

/*}
** Name: QEF_DATA - QEF data block
**
** Description:
**      Data blocks can be linked to gether to provide room for
**	an arbitrary amount of input and output. Each data block
**	holds one tuple.
**
** History:
**     23-may-86 (daved)
**          written
*/
struct _QEF_DATA
{
    QEF_DATA   *dt_next;        /* next data block */
    i4          dt_size;        /* size of block in bytes */
    PTR         dt_data;        /* pointer to the beginning of
                                ** the data area.
                                */
};

/*}
** Name: QEF_PARAM - qef parameter descriptor block
**
** Description:
**	This structure contains pointers to the user's parameters for a query.
**	The dt_size is the count of parameters received (by SCF) and the 
**	dt_data is the address of the array of pointers to the values of the
**	parameters.  These are the actual data values, not the DB_DATA_VALUE
**	structures.  The parameters are numbered from 1 to N corresponding to
**	the index to the pointer array.  The first element of the array 
**	(index 0) is skipped.  That is, to pass N parameters, N+1 elements of
**	the pointers array are required.
**
** History:
**	1-aug-86 (daved)
**          written
**	13-dec-88 (puree)
**	    document it.
**	24-jan-89 (puree)
**	    update the description.
**	22-june-06 (dougi)
**	    Added QEF_IS_BYREF for expanded parameter modes.
*/
struct _QEF_PARAM
{
    QEF_PARAM   *dt_next;       /* Next QEF_PARAM  */
    i4          dt_size;        /* Number of parameters passed into QEF */
    PTR         *dt_data;       /* Address of an array of pointers.
				** Each pointer in this points to the value 
				** of a query parameter.  This value is the 
				** actual paremeter value and not the 
				** DB_DATA_VALUE structure.  There will be
				** (dt_size + 1) pointers in this array.
				** The first pointer is not used.
                                */
};


/*}
** Name: QEF_USR_PARAM - User's parameter block for database procedures
**
** Description:
**	This is the structure of an entry in the input parameter array
**	for executing a DB procedure.  Each entry in the array describes
**	an actual parameter given by the user.  Please notice that
**	procedure parameters are referenced by name.  There is no specific
**	ordering of the entries in the array and one or more parameters 
**	can be missing.
**
** History:
**	28-apr-88 (puree)
**	    written
**	26-oct-92 (jhahn)
**	    Added parm_flags and QEF_IS_BYREF for DB procedures..
**      26-july-1993 (jhahn)
**          Added parm_index for handling byref parameters
**	20-march-2008 (dougi)
**	    Added QEF_IS_POSPARM for positional (nameless) parameter support.
*/
typedef struct _QEF_USR_PARAM
{
    char	    *parm_name;	/* name of the parameter */
    i4		    parm_nlen;	/* length of name above */
    DB_DATA_VALUE   parm_dbv;	/* data descriptor of the parameter */
    i4		    parm_flags;
#define	    QEF_IS_BYREF    	0x01	/* Set iff parm is passed byref */
#define	    QEF_IS_OUTPUT	0x02	/* iff parm is output - initially these
					** only come from JDBC (& ODBC?) */
#define	    QEF_IS_POSPARM	0x04	/* iff parm is nameless */
    i4		    parm_index; /* index into formal parameter list of
				** associated parameter.
				*/
} QEF_USR_PARAM;


/*}
** Name: QED_CONN_INFO - DIRECT CONNECT request information.
**
** Description:
**      This data structure is used to supply complete control information
**  for direct connection with an LDB, i.e., for establishing a local
**  session and for continuing actions within a CONNECT state.
**
** History:
**      10-may-88 (carl)
**          written
*/

typedef struct _QED_CONN_INFO
{
    DD_LDB_DESC	    qed_c1_ldb;		/* LDB for DIRECT CONNECT */
    bool	    qed_c2_newusr_b;	/* TRUE if given new user id and
					** optionally password,FALSE otherwise 
					*/
    DD_USR_DESC	    qed_c3_usr_desc;	/* new connection user id & password,
					** valid if above field is TRUE */
    PTR		    qed_c4_exchange_p;	/* ptr to structure for exchanging
					** information between SCF and RQF,
					** typically used for QED_C2_CONT */
    bool	    qed_c5_tran_ok_b;	/* TRUE if OK to connect within a 
					** transaction, FALSE otherwise; 
					** for example, set to TRUE for
					** DIRECT EXECUTE IMMEDIATE, FALSE
					** for DIRECT CONNECT */
}   QED_CONN_INFO;


/*}
** Name: QED_OUTPUT_INFO - Information for receiving a tuple
**
** Description:
**      This data structure is used to describe a tuple for its reception,
**  typically used by RDF.
**
** History:
**      10-may-88 (carl)
**          written
*/

typedef struct _QED_OUTPUT_INFO
{
    /* client parameters */

    i4		    qed_o1_col_cnt;	/* number of columns in tuple */
    PTR		    qed_o2_bind_p;	/* ptr to array of column binding info
					** structure, dimension must match
					** above count */
    /* QEF's output parameters */

    bool	    qed_o3_output_b;	/* set to TRUE by QEF if there is 
					** output data, FALSE otherwise */
}   QED_OUTPUT_INFO;


/*}
** Name: QED_QUERY_INFO - Information for processing an LDB query.
**
** Description:
**	This structure is used to provide the necessary information
**  for sending a query to an LDB for execution.
**
** History:
**      10-may-88 (carl)
**	    written
*/

typedef struct _QED_QUERY_INFO
{
    i4		     qed_q1_timeout;	/* timeout quantum, 0 if none */
    DB_LANG	     qed_q2_lang;	/* DB_QUEL or DB_SQL, DB_NOLANG if 
					** unknown */
    DD_QMODE	     qed_q3_qmode;	/* DD_1MODE_READ, or DD_2MODE_UPDATE, 
					** DD_0MODE_NONE if unknown */
    DD_PACKET	    *qed_q4_pkt_p;	/* query components */
    i4		     qed_q5_dv_cnt;	/* number of data values specified
					** query, i.e., number of occurrences
					** of ~V in the query, must be 0 if
					** none */
    DB_DATA_VALUE   *qed_q6_dv_p;	/* ptr to first of DB_DATA_VALUE array
					** containing as many elements as above
					** count, must be NULL if not used */
} QED_QUERY_INFO;


/*}
** Name: QED_LDB_QRYINFO - Information for processing a query for an LDB.
**
** Description:
**	This structure is used to provide the necessary information
**  for processing an LDB query.
**
** History:
**      10-may-88 (carl)
**	    written
*/

typedef struct _QED_LDB_QRYINFO
{
    QED_QUERY_INFO  qed_i1_qry_info;	/* information about the query */
    DD_LDB_DESC	    qed_i2_ldb_desc;	/* descriptor of target LDB */
} QED_LDB_QRYINFO;


/*}
** Name: QED_DDL_INFO - DDL statement processing information.
**
** Description:
**      This data structure is used to supply information for processing
**  DDL statements: 
**
**	    CREATE/DEFINE LINK,
**	    DROP/DESTROY LINK,
**	    CREATE TABLE,
**	    DROP/DESTROY TABLE
**
** History:
**      10-may-88 (carl)
**          written
*/

typedef struct _QED_DDL_INFO
{
    DD_OBJ_NAME	      qed_d1_obj_name;	    /* object name (DDB level) */
    DD_OWN_NAME	      qed_d2_obj_owner;	    /* object owner name (DDB level) */
    i4		      qed_d3_col_count;	    /* number of columns in table,
					    ** valid if columns are mapped */
    DD_COLUMN_DESC  **qed_d4_ddb_cols_pp;   /* ptr to array of ptrs to DDB 
					    ** column descriptors, valid if 
					    ** columns are mapped */
    QED_QUERY_INFO   *qed_d5_qry_info_p;    /* ptr to query, typically CREATE 
					    ** TABLE for an LDB; NULL if none */
    DD_2LDB_TAB_INFO *qed_d6_tab_info_p;    /* ptr to LDB table information, 
					    ** used by CREATE LINK/TABLE */
    DB_TAB_ID	      qed_d7_obj_id;	    /* for passing object id, e.g., 
					    ** to identify a link */
    DD_OBJ_TYPE	      qed_d8_obj_type;	    /* DD_2OBJ_TABLE for CREATE TABLE
					    ** or DD_1OBJ_LINK for CREATE LINK
					    */
    QED_QUERY_INFO   *qed_d9_reg_info_p;    /* ptr to IIREGISTRATIONS query 
					    ** text, NULL if none */
    i4	      qed_d10_ctl_info;	    /* control information */

#define	QED_0DD_NIL	    0x0000L
#define	QED_1DD_DROP_STATS  0x0001L	    /* ON if existing statistics info
					     * becomes obsolete on REFRESH, 
					     * OFF otherwise */
#define	QED_2DD_SUB_SCHEMA  0x0002L	    /* ON if local schema becomes
					     * a subset of the old on REFRESH, 
					     * OFF otherwise */
    PTR		      qed_d11_anything_p;   /* reserved for future use */
}   QED_DDL_INFO;

/*}
** Name: QEF_DDB_REQ - DDB request control block
**
** Description:
**	This structure is used to provide the necessary information
**  for invoking DDB functions and obtaining output.
**
** History:
**	20-aug-88 (carl)
**	    written
**	17-jan-89 (carl)
**	    renamed qer_d3_usr_p to qer_d8_usr_p and added qer_d9_com_p
**	14-feb-89 (carl)
**	    added qef_d10_tupdesc_p to allow RQF to return LDB's tuple
**	    descriptor to SCF
*/

typedef struct _QEF_DDB_REQ
{
    DD_DDB_DESC	    *qer_d1_ddb_p;	/* current DDB descriptor, NULL if 
					 * none */
    DD_1LDB_INFO    *qer_d2_ldb_info_p;	/* current LDB info, NULL if 
					 * none */
    QED_QUERY_INFO   qer_d4_qry_info;	/* specify information for sending 
					 * a query to above LDB */
    QED_OUTPUT_INFO  qer_d5_out_info;	/* tuple desc for receiving a tuple 
					 * from QEF */
    QED_CONN_INFO    qer_d6_conn_info;	/* DIRECT CONNECT info */
    QED_DDL_INFO     qer_d7_ddl_info;	/* data definition statement info */
    DD_USR_DESC	    *qer_d8_usr_p;	/* real user descriptor, NULL if none */
    DD_COM_FLAGS    *qer_d9_com_p;	/* ptr to command flags, NULL if none */
    PTR		     qer_d10_tupdesc_p;	/* ptr to LDB's tuple descriptor, 
					 * returned by RQF for SCF */
    DD_MODIFY	     qer_d11_modify;	/* used to modify LDB association 
					 * parameters; set by SCF */
    bool	     qer_d12_ddl_concur_on_b;	
					/* TRUE | FALSE for SET DDL_CONCURRENCY
					 * ON | OFF */
    i4	     qer_d13_ctl_info;	/* control information if relevant */

#define	QEF_00DD_NIL_INFO	    0x0000L

#define	QEF_01DD_LDB_GENERIC_ERROR  0x0001L

					/* reserved to indicate a generic error
					 * returned by an LDB */

#define	QEF_02DD_LDB_BAD_SQL_QUERY  0x0002L

					/* reserved to indicate failure to
					 * process an SQL subquery by an LDB
					 */

#define	QEF_03DD_LDB_XACT_ENDED	    0x0004L

					/* reserved to indicate termination
					 * of an LX as a result of a COMMIT, 
					 * ABORT, subquery or operation at
					 * an LDB */

#define	QEF_04DD_RECOVERY	    0x0010L

					/* result returned by QED_P1_RECOVER:
					 * ON if orphan DXs found in one or
					 * more DDBs when QED_RECOVER_P1 is
					 * called at server startup time,
					 * implying that QED_RECOVER_P2 must
					 * be called; OFF otherwise */

#define	QEF_05DD_DDB_OPEN	    0x0020L

					/* result returned by QED_IS_DDB_OPEN:
					 * ON if DDB requires no recovery,
					 * OFF otherwise, i.e., DDB currently
					 * under recovery */

    i4	     qer_d14_ses_timeout;	
					/* timeout ticks for session */
    bool	     qer_d15_diff_arch_b;	
					/* (QED_7RDF_DIFF_ARCH) TRUE if LDB 
					** has architecture different from
					** STAR's, FALSE otherwise */
    PTR		     qer_d16_anything_p;/* reserved for future use */
    i4	     qer_d17_anything;	/* reserved for future use */
} QEF_DDB_REQ;


/*}
** Name: QEF_RCB - request control block for QEF
**
** Description:
**	Request action of QEF. This control block contains the user definable
**  parameters for QEF calls.
**
**  The qef_count parameter merits some discussion, which is longwinded, so is
**  more easily placed here than in the side section below.
**
**  qef_count is an input & output parameter.  On input, how many rows of
**  output are desired?  This also indicates how many input buffers are being
**  provided.
**
**  On output, it is the [integral] number of rows returned.  This parameter is
**  used in concert with the qef_count field listed below.  qef_rowcount
**  indicates the number of end-of tuples which have been returned by the time
**  this control block is returned.  I.e. not necessarily all of them were
**  returned entirely within this call.
**
**  For example, if I have a select statement which fetchs a row which is 10K
**  wide (because of peripheral datatypes).  If the caller asks for 3
**  rows/buffers, and the buffer size is 2K, then the following will happen:
**
**	Call #	    qef_rowcount    qef_count
**		    input/output
**	    1		3/0		3
**	    2		3/1		3
**	    3		3/0		3
**	    4		3/1		3
**	    5		3/1		3
**  At this point 3 whole rows have been
**  returned, in 15 2K buffers.  If there is
**  only one tuple left which qualifies, the
**  sequence will continue as
**	    6		3/0		3
**	    7		3/1		2
**	(no more rows)
**
** History:
**	10-jun-86 (daved)
**	    written
**	28-apr-88 (puree)
**	    added qef_usr_param for DB procedure parameter array.
**	20-aug-88 (carl)
**	    Added Titan changes: new fileds qef_r1_distrib, qef_r2_level, 
**	    and qef_r3_ddb_req 
**	03-mar-89 (paul)
**	    Added qef_intstate. This makes some internal state information for
**	    QEF available to SCF so it knows if rules processing is in progress.
**	01-apr-89 (paul)
**	    Added qef_context_cnt to record how many nested QP's are currently
**	    active.
**	17-apr-89 (paul)
**		Added qef_rule_depth to replace incorrect use of qef_stat to
**		count the current rule nesting depth.
**		Added qef_mode as space to remember the current QEQ mode.
**	12-feb-91 (mikem)
**	    Added two new fields qef_val_logkey and qef_logkey to the QEF_RCB
**	    structure to support returning logical key values to the client
**	    which caused the insert of the tuple and thus caused them to be
**	    assigned.
**	25-feb-91 (rogerk)
**	    Added qef_stm_error field in which to return the error handling 
**	    mode from qec_info calls. Part of SET SESSION WITH ON_ERROR changes.
**	24-may-91 (andre)
**	    Add qef_username.  This field will be used to communicate name of
**	    the current session user (it will get set at session startup, but
**	    may change as a result of SET USER AUTHORIZATION)
**      11-dec-1991 (fred)
**	    Added narrative on the use of qef_count.  No changes, only comment
**	    changes.  This field was previously unused.  It is now used for the
**	    return of large tuples from QEF.  It is different from qef_rowcount
**	    only when large objects are involved.
**      26-jun-92 (anitap)
**          Added qef_upd_rowcnt field in which to return the rowcount value
**          for update.
**      27-oct-92 (anitap)
**          Added pointer to query text handle for CREATE SCHEMA/constraints 
**	    project. This pointer is passed on to SCF by QEF so that PSF can be
**          called to parse the query text for the statements in CREATE
**          SCHEMA/rules and procedures for constraints to resolve backward 
**	    references. Also added new defines to qef_intstate to indicate to 
**	    SCF that QEA_EXEC_IMM type of action is being processed.
**	12-jan-93 (jhahn)
**	    Added qef_dbpId and qef_setInputId to qefrcb for set input
**	    procedures.
**	15-apr-93 (jhahn)
**	    Added qef_no_xact_stmt and qef_illegal_xact_stmt for catching
**	    transaction control statements when they are disallowed.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added qef_dbxlate to QEF_RCB, which points to case translation mask.
**	    Added qef_cat_own to QEF_RCB, which points to catalog owner name.
**	24-jun-93 (anitap)
**	    Added qef_lk_id to QEF_RCB so that the qsf lock id for the execute
**	    immediate query text object can be passed on to SCF to be used 
**	    for QSO_DESTROY. Else will get the following error message:
**	    "E_QS0013_BAD_LOCKID - The lock ID supplied does not match 
**	     the one assigned to the QSF object when it was exclusively locked."
**	    Changed QEF_SAVED_RSRC to QEF_DBPROC_QP. 
**	22-sep-93 (andre)
**	    added qef_sess_startup_flags and defined QEF_UPGRADEDB_SESSION
**	26-oct-93 (anitap)
**	    Added qef_stmt_type and flag QEF_DGTT_AS_SELECT for temporary
**	    tables for "declare global temporary table as...".
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	24-nov-93 (robf)
**          Added qef_group/aplid
**	26-nov-93 (robf)
**	    Added qef_evname/owner/text/l_text for new QEU_RAISE_EVENT request.
**	21-Jan-94 (iyer)
**	    Add QEF_RCB.qef_tran_id in support of dbmsinfo('db_tran_id').
**	    DM0M_OBJECT.
**	01-feb-1993 (smc)
**	    Removed duplicated qef_stmt_type added via a mental aberration
**	    during previous change.
**	04-apr-94 (anitap)
**	    Added ulm_streamid & ulm_memleft to QEF_RCB.
**	    We want to remember these particulars for the memory
**	    stream that we open in QEF for E.I. query text so that we
**	    close the stream after we are done with execute immediate
**	    processing.
**      13-jun-97 (dilma04)
**          Addded QEF_CONSTR_PROC flag indicating that QEF is processing
**          an action associated with an integrity constraint.
**	14-sep-00 (hayke02)
**	    Added QEF_GCA1_INVPROC as a new qef_stmt_type, indicating that
**	    a DBP is being executed from a libq front end (e.g. ESQLC).
**	    this change fixes bug 102320.
**	28-oct-98 (inkdo01)
**	    Added qef_nextout to supplement qef_output. For row-producing
**	    procs, this is the next buffer to be filled (on successive 
**	    calls to qea_retrow).
**	10-Jan-2001 (jenjo02)
**	    Deleted redundant qef_dmf_id, use qef_sess_id instead.
**      19-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**          Added QEF_RDF_INTERNAL and QEF_RDF_MSTRAN as new qef_stmt_type(s).
**          QEF_RDF_INTERNAL indicates that this is a internal query.
**          For bug 107330 the internal query is a pst_showtab()
**          on a remote table. The information is not currently in
**          the RDF cache when we call rdf_gdesc().
**          QEF_RDF_MSTRAN indicates that a QEF_RDF_INTERNAL query
**          caused QEF to open an MST and we were not previously in an MST.
**          The caller that initiated the query using rdd_query()
**          should commit the internal query via rdd_fetch() hitting
**          eof or rdd_flush(). See rdd_buffcap().
**          QEF_RDF_INTERNAL and QEF_RDF_MSTRAN must be unset after
**          use. It is suggested that this be done using ALL_QEF_RDF.
**	26-Jun-2006 (jonj)
**	    Add QET_XA_START_JOIN, QET_XA_END_FAIL, QET_COMMIT_1PC
**	25-jan-2007 (dougi)
**	    Add scrollable cursor fetch fields.
**	9-apr-2007 (dougi)
**	    Add scrollable cursor return fields (e.g. current cursor 
**	    position). It isn't clear whether they should be here or in the
**	    QEF_CB.
**	24-apr-2007 (dougi)
**	    Remove scrollable cursor #define's - they're already in gca.h.
*/
struct _QEF_RCB
{
    /* standard stuff */
    QEF_RCB     *qef_next;      /* The next control block */
    QEF_RCB     *qef_prev;      /* The previous one */
    SIZE_TYPE   qef_length;     /* size of the control block */
    i2          qef_type;       /* type of control block */
#define QEFRCB_CB    6
    i2          qef_s_reserved;
    PTR         qef_l_reserved;
    PTR         qef_owner;
    i4          qef_ascii_id;   /* to view in a dump */
#define QEFRCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'R', 'C')
    CS_SID	qef_sess_id;	/* session id of session */
    QEF_CB	*qef_cb;	/* session control block */
    i4		qef_stat;	/* This field is used during a QEF_INFO call */
				/* to return the current transaction status. */
    i4		qef_stm_error;	/* Used to return statement error mode. */
    i4		qef_open_count;	/* number of open cursors */
    DB_SP_NAME *qef_spoint;     /* name of a savepoint */
    DB_TAB_TIMESTAMP qef_comstamp;/* date of last commit transaction stmt */
    i4          qef_pcount;     /* how many sets of input parameters */
    QEF_PARAM  *qef_param;      /* ptr to the first QEF_PARAM structure */
    QEF_USR_PARAM **qef_usr_param; /* ptr to an array of pointers to 
				   ** QEF_USR_PARAM's that contains the 
				   ** descriptors of values of usr's input 
				   ** parameters for executing a database 
				   ** procedure */
    i4	qef_dbp_status;	/* return status from DB procedure execution */
    i4	qef_rowcount;   /*
				**  Input:  Number of rows desired & number of
				**  row buffers provided.
				**  Output: Number of end-of-rows provided by
				**  this call.
				**
				**  See discussion above.
				*/
    i4	qef_targcount;	/* how many rows were touched in query */
    QEF_DATA   *qef_output;     /* where to put result information */
    QEF_DATA   *qef_nextout;	/* next available output buffer */
    i4		qef_curspos;	/* position of cursor after this query */
    i4		qef_rowstat;	/* flags to control setting of gca_errd0 for
				** scrollable cursors (see GCA_ROW_xxx vals) */
    DB_CURSOR_ID qef_qp;        /* name of query plan */
    PTR		qef_qso_handle;	/* handle to query plan if not zero */
    i4          qef_qacc;       /* access to query plan */
#define QEF_UPDATE  0           /* query opened for read/write */
#define QEF_READONLY 1         /* query opened for read only */
#define QEF_DBPROC 2           /* query opened for connect via db procedure */
    i4          qef_count;      /*
				**  number of output buffers which were filled.
				**  This may be different that the number of
				**  output rows.  This will be the case where
				**  large objects (BLOBs or peripheral
				**  datatypes) are in use, because the tuple
				**  size will not include the size of the
				**  peripheral object (which is, effectively,
				**  unlimited).  In these cases, if 10 buffers
				**  are passed in as indicated by having the
				**  qef_rowcount field set to 10, it is possible
				**  to return a qef_rowcount of 0, but have all
				**  10 buffers filled.  In this case,
				**  qef_rowcount will be 0, and qef_count will
				**  be 10.  Other combinations are possible.
				*/
    i4		qef_fetch_offset; /* result set offset (for scrollable cursors) */
    u_i4	qef_fetch_anchor; /* reference point for scrollable cursor
				** offset (uses GCA_ANCHOR_xxx values) */
    i4          qef_version;    /* version number */
    struct
    {
	DB_CURSOR_ID	qef_p_crsr_id;
	char		qef_p_user[DB_OWN_MAXNAME];
	i4		qef_p_dbid;
    } qef_dbpname;		/* An alias for a procedure (QP) that QEF    */
				/* needs compiled for execution. This value */
				/* is filled in when QEF returns with the   */
				/* error code E_QE0119_LOAD_QP. An alias is */
				/* required so that the QP can be uniquely  */
				/* identified. It may not belong to the	    */
				/* current user or the dba. */
    DB_TAB_ID	qef_dbpId;	/* 0 or ID of above procedure */
    DB_TAB_ID	qef_setInputId; /* table id if set input procedure */
    /* runtime information */
    i4		qef_auto;	/* transaction handling flag */
#define QEF_ON	1		/* autocommit after each statment */
#define QEF_OFF 2		/* don't autocommit */
    i4          qef_asize;      /* number of constants to alter */
    QEF_ALT    *qef_alt;        /* alteration array */
    DB_OWN_NAME *qef_evowner;	/* Event owner */
    DB_EVENT_NAME *qef_evname;    /* Event name */
    char	*qef_evtext;    /* Event text */
    i4		qef_ev_l_text;  /* Length of even text */
    /* general purpose stuff */
    i4	qef_operation;	/* operation that QEF was called with */
    i4          qef_modifier;   /* general command modifier */
#define	QET_XA_START_JOIN	1
#define	QET_XA_END_FAIL		1
#define	QET_XA_COMMIT_1PC	1
    i4          qef_flag;       /* secondary command modifier */
    PTR		qef_server;	/* address of server control block */
    PTR		qef_db_id;      /* a dmf database id */
    QEU_COPY   *qeu_copy;	/* the copy control block */
    ADF_CB	*qef_adf_cb;	/* Session ADF control block address */
				/* NOTE only set for QEF begin session!
				** Use ADF CB in qef cb or dsh otherwise.
				*/
    i4		qef_remnull;	/* set to TRUE if NULLS skipped in agg eval */
    i4		qef_intstate;	/* QEF internal state on return from QEF */
#define	QEF_DBPROC_QP	0x01	/* There are resources held by QEF after    */
				/* return to caller. This flag bit indicates*/
				/* QEF assumes it is being called back to   */
				/* perform further processing. It must be   */
				/* called back to either continue	    */
				/* processing or to clean up if an error    */
				/* occurs. If this status bit is set, the   */
				/* QEF caller must not change the QEF_RCB   */
				/* between calls to QEF except the the	    */
				/* fields qef_qp and qef_qso_handle. This   */
				/* state is generally returned if QEF	    */
				/* requires a valid QP be loaded during	    */
				/* rules processing or nested procedure	    */
				/* invocation. */
#define	QEF_CLEAN_RSRC	0x02	/* If QEF_DBPROC_QP is set and the caller  */
				/* of QEF encounters a fatal error, QEF	    */
				/* must be recalled with QEF_CLEAN_RSRC set */
				/* to release currently allocated resources.*/
#define QEF_ZEROTUP_CHK	0x04	/* If QEF_ZEROTUP_CHK is set then the callers */
				/* query plan was invalidated because of    */
				/* results due to query flattening problems */
				/* The plan should be rebuilt without	    */
				/* flattening turned on.		    */
#define QEF_EXEIMM_PROC  0x08   /* QEF is in the middle of processing an    */ 
                                /* Execute Immediate action.                */
#define QEF_EXEIMM_CLEAN 0x10	/* If QEF_EXEIMM_PROC is set and the caller */
				/* of QEF encounters a fatal error, QEF     */
				/* must be recalled with QEF_EXEIMM_CLEAN   */
				/* set to release currently allocated       */
				/* resources				    */
#define QEF_CONSTR_PROC  0x20   /* QEF is processing an action associated   */
                                /* with an integrity constraint.            */
    i4		qef_context_cnt;/* Number of nested QP's currently active.  */
				/* This value is zero for the outer query   */
				/* and is incremented once for each	    */
				/* CALLPROC action. It if ever exceeds the  */
				/* session's max allowed depth, the	    */
				/* current query is aborted */
    i4		qef_rule_depth;	/* The current depth of rules execution.    */
				/* Each time a rule action list is invoked  */
				/* this value is incremented, each time a   */
				/* rule action list is completed this value */
				/* is decremented. Used to decide how much  */
				/* execution context to unwind on an error. */
    i4		qef_mode;	/* Used to save the original request mode   */
				/* for qeq_query. The mode may change as we */
				/* execute nested procedures and must be    */
				/* restored on return from a nested QP. */
    i4		qef_eflag;	/* type of error handling semantics:   
                                ** either QEF_EXTERNAL or QEF_INTERNAL */
    DB_ERROR    error;          /* control block for error processing */
    DB_OWN_NAME	qef_username;	/* name of the session user */
    i4	qef_val_logkey; /* If either QEF_TABKEY or QEF_OBJKEY asserted
				** then a valid logical key has been stored in
				** qef_logkey, and should be returned to the 
				** client.
				*/
#define QEF_TABKEY	0x01	/* Asserted if tabkey assigned by last insert */
#define QEF_OBJKEY	0x02	/* Asserted if objkey assigned by last insert */
    DB_OBJ_LOGKEY_INTERNAL 
		qef_logkey;	/* logical key value last assigned by an insert
				** into a table with a system maintained logical
				** key.
				*/
    QSF_RCB	     *qsf_rcb;       /* QSF request control block which is
				     ** needed to destroy the E.I. qtext.
				     */
    QSO_OBID         qef_qsf_handle; /* handle to the query text to be 
				     ** parsed 
				     */
    i4		     qef_lk_id;	     /* lock id of execute immediate query text
				     ** object to be passed on to SCF.
				     */ 
    SIZE_TYPE        *ulm_memleft;   /* Pointer to counter to be decremented
                                     ** every time memory is assigned from
                                     ** the pool to a stream, and incremented
                                     ** every time memory is released from a
                                     ** stream (i.e. a stream is closed).  A
                                     ** pointer is used since many streams
                                     ** may need to point to the same
                                     ** counter.
                                     */
    PTR             ulm_streamid;    /* Stream id used to identify stream
                                     ** from which pieces will be allocated.
                                     */

    PTR              qef_info;       /* info passed back to PSF for accurate
                                     ** error message reporting. Pointer to 
				     ** PST_INFO structure. Cannot define it
				     ** as PST_INFO because qefrcb.h refers 
				     ** to psfparse.h for PST_INFO and 
				     ** psfparse.h refers to QED_DDL_INFFO
				     ** defined in qefrcb.h. Circular	
				     ** references.
                                     */
    
     i4              qef_upd_rowcnt;  /* Update rowcount value flag */
#define QEF_ROWCHGD  1          /* Return the count of rows changed */
#define QEF_ROWQLFD  2          /* Return the count of rows which qualified */

     i4              qef_stmt_type;	/* type of statement issued by user */
#define	QEF_DGTT_AS_SELECT              0x01
#define	QEF_GCA1_INVPROC		0x02	/* DBP executed from a libq
						** front end e.g. ESQLC */
#define QEF_RDF_INTERNAL                0x04    /* RDF internal query */
#define QEF_RDF_MSTRAN                  0x08    /* QEF initiated an MST. 
                                                ** Should only be set if
						** RDF caused us to open
						** a QEF query (QEF_MSTRAN)
						** when we were (QEF_NOTRAN).
						** Used to signify need to
						** commit RDF/QEF query before
						** executing USER stmt.
						*/
#define ALL_QEF_RDF			(QEF_RDF_INTERNAL | QEF_RDF_MSTRAN)

    DB_DISTRIB	    qef_r1_distrib;	/* a DDB operation if DB_DSTYES,
					** a regular operation otherwise */
    DD_QEF_LEVEL    qef_r2_qef_lvl;	/* output for QEC_INFO: DD_0QEF_DDB
					** if backend server, DD_1QEF_DDB if 
					** DDB server */
    QEF_DDB_REQ	    qef_r3_ddb_req;	/* DDB session control block */
    bool	qef_no_xact_stmt; /* No transaction statments allowed */
    bool	qef_illegal_xact_stmt;	/* Illegal transaction statment
					** attempted */
    u_i4	    *qef_dbxlate;	/* Case translation semantics flags
					** See <cuid.h> for masks.
					*/
    DB_OWN_NAME	    *qef_cat_owner;	/* Catalog owner name.  Will be
					** "$ingres" if regular ids are lower;
					** "$INGRES" if regular ids are upper.
					*/
    DB_OWN_NAME	    *qef_group;		/* Session group name */
    DB_OWN_NAME	    *qef_aplid;		/* Session role name */
    i4		qef_sess_startup_flags;
					/*
					** this session will be running 
					** UPGRADEDB
					*/
#define	QEF_UPGRADEDB_SESSION		0x01

    DB_TRAN_ID	qef_tran_id;	/* Transaction identifier.
				** Filled in by qec_info().
				*/
};

/*}
** Name: QEF_TUPLE_PACKER - Arguments for the tree/queryText packers
**
** Description:
**	This structure describes the argument structure passed to the
**	routines which pack tree and querytext into tuples.
**
** History:
**	08-dec-92 (rickh)
**	    Created.
**/
typedef struct	_QEF_TUPLE_PACKER	{
    DB_DISTRIB		qtp_distrib;
    QEF_DATA		**qtp_data;
    i4		*qtp_count;
    DB_MEMORY_ALLOCATOR	*qtp_memoryAllocator;
    DB_ERROR		*qtp_error;
    i4			qtp_mode;
}	QEF_TUPLE_PACKER;

