/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: QEFQP.H - The top of the query plan
**
** Description:
**      A query execution plan is composed of many different actions. Each 
**  action describes an activity required to successfully execute a query.
**  These actions include such things as opening a relation, creating a 
**  relation, retrieving data, displaying data, computing aggregates, 
**  beginning transactions, ending transactions, making savepoints, etc.
**  At various points in computing the query execution plan, the plan can
**  be diagnosed as invalid at which time the query is aborted. A query
**  becomes invalid when certain assumptions that we correct at query 
**  definition are no longer correct. There are two such events that can
**  cause a query to become invalid. 1) The timestamps of the affected
**  relations become outdated (a relation was modified, protections added, etc)
**  or 2) a parameter used  as a key in the query has changed from 
**  patern matching to non-patern matching.
**
** History:
**      11-mar-86 (daved)
**          written.
**	21-oct-87 (puree)
**	    Added qp_qconst_mask field for query constant bit map to
**	    QEF_QP_CB.
**	02-may-88 (puree)
**	    Added the structure QEF_DBP_PARAM and modified QEF_QP_CB for
**	    the DB procedure project.
**	31-aug-88 (carl)
**	    Added Titan changes to QEF_QP_CB
**	26-sep-88 (puree)
**	    Modified QEF_QP_CB for in-memory sorter support.
**	08-may-89 (carl)
**	    Added QEQP_01QM_RETRIEVE, QEQP_02QM_DEFCURS
**	07-oct-89 (carl)
**	    Added QEQP_03QM_RETINTO
**	15-jan-89 (paul)
**	    Enhanced for rules processing. Added structures for describing
**	    parameter lists for nested procedure calls.
**      25-jul-89 (jennifer)
**          Added new field to QP to point to the audit information 
**          built by the parser.
**	12-dec-89 (fred)
**	    Added QEQP_LARGE_OBJECTS flag to indicate that a query returns at
**	    least one large object -type column.
**	11-aug-92 (rickh)
**	    Added count of temporary tables.
**	11-oct-92 (jhahn)
**	    Added qp_resources & value-lock for dropped procedures to QEF_QP_CB;
**	    added qef_proc_resource and qef_resource structures
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	12-jan-93 (jhahn)
**	    Added qp_setInput to QEF_QP_CB for set input procedures.
**	20-jul-1993 (pearl)
**          Bug 45912.  Added qp_kor_cnt to QEF_QP_CB to keep track of number
**          of KOR's; also used to set the kor_id field in the QEF_KOR
**          structure.
**      15-oct-93 (eric)
**          Moved QEF_RESOURCE to QEFACT.H because QEF_VALID now refers to
**          it and header includes support this arrangement.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-nov-2002 (stial01)
**          Added QEQP_NEED_TRANSACTION (b109101)
**      12-Apr-2004 (stial01)
**          Define qp_length as SIZE_TYPE.
*/

/*
**  Constants for query processing
**
*/
#define QEF_NEXT_GET    0       /* get next row */
#define QEF_FIRST_GET   1       /* get first row with new value */
#define QEF_CURRENT_GET 2       /* get first row with current value */


/*}
** Name: QEF_DBP_PARAM - Definition of a DB procedure parameter.
**
** Description:
**      This structure describes the format of an entry in the list
**	of database procedure parameters.
**
** History:
**      02-may-88 (puree)
**          created for DB procedure project.
**      26-oct-92 (jhahn)
**	    Added dbp_byref_... to structure for handling byref parameters.
**	9-june-06 (dougi)
**	    Added dbp_flags field to define parameter modes.
[@history_template@]...
*/
typedef struct _QEF_DBP_PARAM
{
    char		dbp_name[DB_MAXNAME];	/* name of the parameter */
    i4			dbp_rowno;		/* DSH buffer row # */
    i4			dbp_offset;		/* DSH buffer offset */
    DB_DATA_VALUE	dbp_dbv;		/* DBV of the parameter */
    i4		dbp_byref_value_rowno;		/* DSH buffer row
						** of DB_DATA_VALUE
						** describing source for
						** byref param. (db_datatype
						** will be set to DB_NODT if
						** not passed byref.) */

    i4		dbp_byref_value_offset;		/* DSH buffer offset
						** of DB_DATA_VALUE
						** describing source for
						** byref param. */
    i4		dbp_mode;			/* parameter mode (in, out,
						** inout) */
#define	DBP_PMIN	0x01			/* IN */
#define	DBP_PMOUT	0x02			/* OUT */
#define	DBP_PMINOUT	0x04			/* INOUT */
}   QEF_DBP_PARAM;



/*}
** Name: QEQ_DDQ_CB - DDB query plan control block
**
** Description:
**      This construct contains a DDB query termination plan and a list 
**  of LDB descriptors for the main query plan.  This is a substructure
**  embedded as qp_ddq_cb in QEF_QP_CB.
**
** History:
**      09-aug-88 (carl)
**          written
**      6-feb-89 (seputis)
**          added d4-d7 for parameterized queries, used by qee_create to
**	    allocate space in the data segment
**	01-mar-93 (barbara)
**	    Add field qeq_d9_delown for saving owner name for DELETE CURSOR.
**	    Owner name is used in GCA1_DL_DATA.
*/

typedef struct _QEQ_DDQ_CB
{
    QEF_AHD	    *qeq_d1_end_p;	    /* ptr to termination query 
					    ** subplan for purging temporary
					    ** tables, executed before the
					    ** entire Query Plan is discarded,
					    ** NULL if none */
    QEQ_LDB_DESC    *qeq_d2_ldb_p;	    /* ptr to first element of list
					    ** of LDBs participating in the
					    ** QP; should never be NULL */
    i4		     qeq_d3_elt_cnt;	    /* number of elements (each a 
					    ** DD_NAME) of array to contain
					    ** temporary-table names and
					    ** attribute types for the QP;
					    ** QEF must allocate space for
					    ** such an array */
    i4               qeq_d4_total_cnt;	    /* total number of parameters
					    ** for entire query plan */
    i4               qeq_d5_fixed_cnt;	    /* number of parameters supplied
					    ** with user query */
    DB_DATA_VALUE   *qeq_d6_fixed_data;	    /* data supplied with user query */
    DD_NAME	    *qeq_d7_deltable;	    /* for a cursor, this would be a ptr
					    ** to the translated name for the
					    ** target table to be deleted, used
					    ** only on a delete cursor action */
    i4		    qeq_d8_mask;	    /* mask field containing various
					    ** booleans */
    DD_NAME	    *qeq_d9_delown;	    /* for a cursor, pointer to the
					    ** LDB owner of the table to be
					    ** deleted, used on delete cursor
					    ** action. */
#define                 QEQ_D8_INGRES_B	    1
/* this flag is set when a query using ii_dd* tables on the coordinator
** site is found, and furthermore it is a retrieval query,  this instructs
** QEF to commit the privileged association to avoid deadlocks */
}   QEQ_DDQ_CB;


/*}
** Name: QEF_QP_CB - Query plan header
**
** Description:
**      This is the top of a query plan. It contains the query identifier 
**	and enough information to allocate the buffers and control blocks.
**
**	When QEF is called to execute a QP for the first time, a QP is fetched
**	from QSF. The root of the QP is this structure, the QEF_QP_CB.
**
**	QEE routines will read this structure and allocate the necessary control
**	blocks and arrays in the DSH structure (see QEFDSH.H). The various
**	index fields in the actions and nodes refer to rows or control blocks
**	that have been allocated in the DSH according to the specifications 
**	in this structure. 
**
**	In coding a QP, then, the following approach is suggested.
**
**	1)  determine what rows, dmr_cbs, dmt_cbs, ade_cxs and hold files
**	    are necessary to execute the query. Number all rows beginning 
**	    with 0.  Number the other structures mentioned (hereafter 
**	    referred to as control blocks) starting with 0.
**
**	2)  note that the ade_cxs are attached to actions and nodes but the
**	    parameter array must be allocated off of the DSH and is thus 
**	    considered a control block.
**
**	3)  count the number of rows, this value goes in the qp_row_cnt field.
**  
**	4)  create an array of #3 nats and fill in the array with the row
**	    lengths corresponding to the row numbers.
**
**	5)  specify in qp_cb_num the number of control blocks used.
**
**	6)  specify in qp_cb_cnt the number of bytes of memory the control
**	    blocks will consume.
**  
**	7)  count the number of hold files required. This value goes in
**	    qp_hld_cnt.  Remember that only QEN_JOIN nodes need hold files.
**	    Number the holde files starting at 0.
**
**	8)  create an array of the number of hold files length and fill in the
**	    elements with the size of the hold file. Array[0] gets length of
**	    hold file 0, etc.
**
**	9)  qp_res_row_sz gets the length of the result tuple so that SCF and
**	    others know how much space to allocate for the results.
**
**	10) qp_status should be treated as a bit field. Mark whether or not the
**	    QP is a repeat query, can have updates performed on it (important
**	    for cursor statements), can have deletes performed on it, uses
**	    deferred or direct update semantics.
**
**	11) qp_upd_cb gets the control block number of the DMR_CB that will
**	    have the update tuple. If another QP wanted to update the current
**	    row in this QP, the DMR_CB specified here would contain the
**	    desired row. Remember that the CBs are numbered from 0. A DMR_CB
**	    number of 5 doesn't mean that this is the fifth DMR_CB; it means
**	    that the desired DMR_CB can be found in the fifth slot in the
**	    DSH's dsh_cbs array (which is an array of pointers).
**
**	12) qp_key_sz contains the number of bytes required to hold all the
**	    keys in this query. Keys are used to provide efficient access to
**	    the specified relations.
**
**	13) the qp_id contains the query plan identifier. This is required for
**	    all repeat queries and is optional in all others. The name is
**	    generated outside of QEF but access to the QP is provided by the
**	    qp_id.
**
**	14) the qp_stat_cnt specifies the number of the required QEN status 
**	    blocks. This value is calculated by counting the number of 
**	    nodes in all of the QEPs in this QP.
**
**	15) At start of compilation for a query, zero out the qp_qconst_mask.
**	    For each call to ade_instr_gen(), pass in the address of this 
**	    qp_qconst_mask.
**
**	16) Finally, we have a count of the number of actions in qp_ahd_cnt
**	    and a linked list of the actions in qp_ahd.
**
** History:
**	11-mar-86 (daved)
**          written
**	21-oct-87 (puree)
**	    Added qp_qconst_mask for query constant bit map.
**	02-may-88 (puree)
**	    Added qp_ndbp_params and qp_dbp_params to contain count and
**	    pointer to procedure parameter list.
**	31-aug-88 (carl)
**	    Added Titan changes to QEF_QP_CB:
**		1.  new field qp_ddq_cb of type QEQ_DDB_CB,
**		2.  comments to clarify Titan's needs.
**	01-sep-88 (puree)
**	    Roll in Eric's changes for DB procedure built-in variables.
**	    Modify VLUP/VLT handling:
**		add qp_pcx_cnt.
**		delete  qp_stat_cnt
**		delete  qp_cb_cnt
**		rename  qp_cb_num to qp_cb_cnt
**	26-sep-88 (puree)
**	    Modified for QEF in-memory sorter.  Deleted qp_hld_len.  Added
**	    qp_sort_cnt.
**	08-may-89 (carl)
**	    Added QEQP_01QM_RETRIEVE, QEQP_02QM_DEFCURS
**      25-jul-89 (jennifer)
**          Added new field to QP to point to the audit information 
**          built by the parser.
**	07-oct-89 (carl)
**	    Added QEQP_03QM_RETINTO
**	12-dec-89 (fred)
**	    Added QEQP_LARGE_OBJECTS flag to indicate that a query returns at
**	    least one large object -type column.
**	25-jan-91 (andre)
**	    Added qp_shr_rptqry_info.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	11-aug-92 (rickh)
**	    Added count of temporary tables.
**	11-oct-92 (jhahn)
**	    Added qp_resources and value lock for dropped procedures
**	12-jan-93 (jhahn)
**	    Added qp_setInput for set input procedures.
**	20-jul-1993 (pearl)
**          Bug 45912.  Added qp_kor_cnt to QEF_QP_CB to keep track of number
**          of KOR's; also used to set the kor_id field in the QEF_KOR
**          structure.
**	16-sep-93 (robf)
**	    Removed qp_audit pointer, moved into QEF_AHD.
**      15-oct-93 (eric)
**          added qp_cnt_resources, and QEQP_DEFERRED.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	1-oct-98 (inkdo01)
**	    Added flag to indicate COMMIT/ROLLBACK in a procedure plan.
**	    This allows internal temp tables for embedded DMLs to be
**	    persistent across a session, rather than run afoul of commit
**	    logic which frees them before proc is done with them.
**	28-jan-99 (inkdo01)
**	    Add status flag for row-producing procedures.
**	1-feb-01 (inkdo01)
**	    Add counter for hash aggregate structures.
**	15-mar-02 (inkdo01)
**	    Add anchor to sequence list.
**	24-feb-04 (inkdo01)
**	    Added flags indicating when to allocate QEF_RCB saveareas
**	    with DSH (after changing RCB instances to ptrs). Changed
**	    qp_status #define's to 0xnnn notation at same time.
**	24-mar-04 (inkdo01)
**	    Added QEQP_PARALLEL_PLAN flag to denote || plans.
**	24-aug-04 (inkdo01)
**	    Added fields for global base array implementation (part of
**	    || queries, but also standalone SIR).
**	24-Mar-2005 (schka24)
**	    Get rid of qconst mask, all we need is a flag.
**	1-Dec-2005 (kschendel)
**	    Add replacement-selection count.
**	14-Feb-2006 (kschendel)
**	    Remove above, didn't need it after all.
**	25-may-06 (dougi)
**	    Add QEQP_ISDBP flag to allow id of db procedure QPs.
**	17-july-2007 (dougi)
**	    Identify scrollable cursor QPs.
**	20-Jul-2007 (kschendel) SIR 122513
**	    Add number-of-pquals for qee-part-qual allocation.
**	26-Sep-2007 (kschendel) SIR 122512
**	    Added FOR-nesting depth, number of exch's not over orig's.
**	30-oct-2007 (dougi)
**	    QEQP_REPDYN identifies repeat dynamic queries and qp_params
**	    is an array of repeat query parm descriptors.
**	28-jan-2008 (dougi)
**	    Add qp_rssplix for scrollable cursors.
**	7-july-2008 (dougi)
**	    Add QEQP_TPROC_RCB for table procedures.
**	2-mar-2009 (dougi) bug 121773
**	    Add QEQP_LOCATORS flag for LOB locators & cached dynamic.
*/

struct _QEF_QP_CB
{
    /* standard jupiter structure header */
    QEF_QP_CB      *qp_next;            /* forward pointer */
    QEF_QP_CB      *qp_prev;            /* backward pointer */
    SIZE_TYPE       qp_length;          /* length of this structure */
    i2              qp_type;            /* type of control block */
#define QEQP_CB 5
    i2              qp_s_reserved;      /* memory mgmt */
    PTR             qp_l_reserved;
    PTR             qp_owner;           /* memory mgmt */
    i4              qp_ascii_id;        /* ascii string. really a char[4] */
#define QEQP_ASCII_ID     CV_C_CONST_MACRO('Q', 'E', 'Q', 'P')
    /* structure specific fields */
    i4              qp_status;		/* a bit mask of the following */
#define QEQP_UPDATE	0x1	        /* updates are allowed for cursor */
#define QEQP_RPT	0x2	        /* this is a repeat query */
#define QEQP_DELETE	0x4		/* deletes are allowed for cursor */
#define	QEQP_SQL_QRY	0x8		/* query plan for an SQL query */
#define QEQP_SHAREABLE	0x10		/* query plan for a shareable query */
#define QEQP_SINGLETON	0x20		/* QP will return at most 1 row */
#define	QEQP_LARGE_OBJECTS  0x40	/* QP returns > 0 large object columns*/
#define QEQP_DEFERRED	0x80		/* update/insert is deferred, otherwise
					** it's direct
					*/
#define QEQP_DBP_COMRBK 0x100		/* procedure with embedded COMMIT/
					** ROLLBACK */
#define QEQP_ROWPROC	0x200		/* row-producing procedure */
#define QEQP_NEED_TRANSACTION 0x400	/* need begin transaction */
#define	QEQP_EXEIMM_RCB	0x800		/* plan contains DDL or exec imm
					** and needs RCB savearea in DSH */
#define	QEQP_CALLPR_RCB	0x1000		/* plan contains proc/rule invocation
					** and needs RCB savearea in DSH */
#define	QEQP_PARALLEL_PLAN 0x2000	/* QP contains some exchange nodes 
					** making it a || plan */
#define QEQP_GLOBAL_BASEARRAY 0x4000	/* ADF CXs compiled using a global
					** base array - not local */
#define QEQP_NEED_QCONST 0x8000		/* Set if query uses (or may use)
					** query constants, so ADF query-
					** constant block must be allocated
					** at query startup */
#define QEQP_ISDBP	0x10000		/* this QP is for a procedure */
#define	QEQP_SCROLL_CURS 0x20000	/* scrollable cursor QP */
#define	QEQP_REPDYN	0x40000		/* repeat dynamic query */
#define QEQP_TPROC_RCB	0x80000		/* query plan has tproc - qeq_query()
					** must set up RCB for proc. */
#define	QEQP_LOCATORS	0x00100000	/* query plan has at least one LOB
					** locator result column */
    DB_CURSOR_ID    qp_id;		/* name of query plan */
    i4		    qp_qmode;		/* query mode. */
#define QEQP_01QM_RETRIEVE  1		/* must be the same as PSQ_RETRIEVE */
#define QEQP_02QM_DEFCURS   15		/* must be the same as PSQ_DEFCURS */
#define QEQP_03QM_RETINTO   2		/* must be the same as PSQ_RETINTO */
#define QEQP_EXEDBP    89		/* must be the same as PSQ_EXEDBP */

    i4		    qp_res_row_sz;	/* space required for result tuple 
					** in bytes.
					*/
    i4		    qp_upd_cb;		/* control block number in DSH 
					** containing the row
					** that can be updated if an update
					** command should require it.
					** This value is an index into the
					** dsh_cbs array in the DSH. The value
					** in the array is a pointer to a
					** DMR_CB.
					*/
    i4		    qp_key_row;		/* dsh_row index for key buffer */
    i4              qp_key_sz;          /* space required for constant keys 
					** in bytes. This array is allocated
					** in the DSH structure.
					*/
    i4              qp_row_cnt;         /* number of row buffers */
    i4              *qp_row_len;        /* row buffer lengths in bytes */
    i4              qp_kor_cnt;         /* bug 45912; number of KOR nodes */
    i4		    qp_stat_cnt;	/* number QEN status blocks */
    i4		    qp_sort_cnt;	/* number of sort nodes in the QP, 
					** including ones under QEN_QP */
    i4		    qp_cb_cnt;		/* number of DMF control blocks */
    i4              qp_hld_cnt;         /* number of hold file buffers */
    i4              qp_hash_cnt;        /* number of hash join structures */
    i4              qp_hashagg_cnt;     /* number of hash aggregate structures */
    i4		    qp_pqual_cnt;	/* Number of partition qual structs */
    i4              qp_ahd_cnt;         /* number of all actions required
                                        ** for query.  This includes actions
					** in QP nodes.
                                        */
    i4		    qp_pcx_cnt;		/* number of CXs with user's params */
    i2		    qp_for_depth;	/* Deepest FOR-loop nesting */
    u_i2	    qp_pqhead_cnt;	/* Total exchange child subplans,
					** not counting orig's */
    QEF_AHD        *qp_ahd;             /* list of actions required 
                                        ** for query 
                                        */
    PTR		    qp_sqlda;		/* description of rows returned by
					** this query plan.
					** This structure is a ULC_SQLDA.
					*/
    /* Qp_dbp_params is a pointer to an array of DB procedure parameter
    ** definitions. Qp_ndbp_params gives the number of of elements in
    ** the array.
    */
    i4		    qp_ndbp_params;
    QEF_DBP_PARAM   *qp_dbp_params;

    /* Repeat query plans have an array of DB_DATA_VALUEs to show the
    ** compiled data types of any parameters used to coerce parm sets 
    ** originating from possibly distinct OPENs. This is needed for
    ** repeat queries accessed through cursors. */
    i4		    qp_nparams;		/* count of parms */
    DB_DATA_VALUE   **qp_params;	/* array of ptrs to parm descriptors */

    /* Additions for DB procedure built-in variables.
    ** Qp_rrowcnt_row and qp_orowcnt_offset give the DSH row number and
    ** offset within the row where QEF should place the final row count
    ** for each query. If they are -1 then this feature has been turned
    ** off.
    */
    i4		    qp_rrowcnt_row;
    i4		    qp_orowcnt_offset;

    /* Similiar to qp_rrowcnt_row and qp_orowcnt_offset, qp_rerrorno_row
    ** qp_oerrorno_offset give the DSH row number and offset where QEF
    ** QEF should place the final error number, if one exists, for each
    ** query. If they are -1 then this feature has been turned off.
    */
    i4		    qp_rerrorno_row;
    i4		    qp_oerrorno_offset;

    /* Information needed to construct QEF_USR_PARAM structures for each    */
    /* CALLPROC statement in this QP. The QP contains the number of	    */
    /* structures required (one per CALLPROC) and the number of parameters  */
    /* in each structure; each CALLPROC statement has a separate parameter  */
    /* count description. */
    i4		    qp_cp_cnt;
    i4		    *qp_cp_size;	/* Pointer to an array of parameter */
					/* counts, one for each CALLPROC in */
					/* the QP. */
    DB_SHR_RPTQRY_INFO	*qp_shr_rptqry_info;/*
					    ** info required by PSF to help in
					    ** determining if a QUEL repeat
					    ** query may use this query plan
					    */
    /* Star Stuff */
    QEQ_DDQ_CB	    qp_ddq_cb;		/* (+Titan) contains 1) a termination 
					** action list of DROP/DESTROY TABLE
					** queries for dropping temporary 
					** tables, 2) a list of LDB descriptors 
					** for use of the current DDB QP, and
					** 3) number of elements in the 
					** temporary-table and attribute 
					** array for the QP */
    i4		    qp_tempTableCount;	/* number of temporary tables in QP */
    QEF_VALID	    *qp_setInput;	/* For query plans of set input
					** procedures describes set input
					** parameter */
    i4              qp_drop_procedure_ddl_lk_value;
                            /* timestamp based on value lock which is updated
        		    ** whenever stored procedure is dropped. If value
	        	    ** matches current value of value lock procedure
		            ** resources do not need to be verified.
        		    */

	/* Qp_resources is a list of resources (tables, db procs) to verify
	** Qp_cnt_resources holds the number of elements on the list.
	*/
    QEF_RESOURCE    *qp_resources;  	/* list of resources to verify */
    i4		    qp_cnt_resources;
    QEF_SEQUENCE    *qp_sequences;	/* list of sequences to open/close */
    i4		    qp_rssplix;		/* dsh_cbs index os spooled result
					** set control block */
};

