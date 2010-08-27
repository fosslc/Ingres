/*
**Copyright (c) 2004 Ingres Corporation
*/

/* Make sure we have QSO_NAME */
#include <qsf.h>

/**
** Name: QEFACT.H - This file describes the action control block
**
** Description:
**      The Action Control Block describes a specific action
**      (query execution, file creation, etc) that must be
**      executed to completion before the next action is begun.
**      One or more actions constitute a statement.
**
** History:
**      18-Mar-1986 (daved)
**          written
**	16-feb-88 (puree)
**	    Modified QEF_VALID to set lock mode nased on the estimated
**	    pages touched by the query.
**	02-may-88 (puree)
**	    Add new actions to QEF_AHD and modified the union of structures
**	    in qhd_obj to the union of pointers. Added structures for
**	    new action objects.
**	20-aug-88 (carl)
**          Added Titan changes to QEF_AHD
**	15-jan-89 (paul)
**	    Add new action CALLPROC. Update QEP action to include nested
**	    action list for rule invocations.
**	30-mar-89 (paul)
**	    Added query cost estimates to QEF_QEP for resource control.
**	14-feb-89 (carl)
**	    Added QEA_D7_OPN, QEA_D8_CRE, QEA_D9_UPD
**	07-apr-89  (teg)
**	    added new action IPROC.
**	18-apr-89 (paul)
**	    Convert update cursor action to a standard QEP style action
**	    structure to support rules processing on cursors. Also add
**	    support for a delete cursor action.
**	09-may-89 (neil)
**	    Rules: Added ahd_rulename to QEF_CALLPROC for tracing.
**      25-oct-89 (davebf)
**          Add ahd_ruprow to QEF_QEP to hold index of dsh_row of row
**          update-able by cursored update
**	10-dec-90 (neil)
**	    Alerters: Added structures and definitions for event support.
**	    QEA_EVREGISTER, QEA_EVDEREG, QEA_EVRAISE types & QEF_EVENT action.
**	21-jul-90 (carl)
**	    Added #define QEA_D0_NIL 100 to QEF_AHD as the base for all 
**	    STAR action types
**	27-feb-91 (ralph)
**	    Added QEF_MESSAGE.ahd_mdest to support the DESTINATION clause
**	    of the MESSAGE and RAISE ERROR statements.
**      14-jul-92 (fpang)
**	    Fixed compiler warnings, QEQ_TXT_SEG, and QEQ_LDB_DESC.
**	17-aug-92 (rickh)
**	    Improvements for FIPS CONSTRAINTS.
**      26-oct-92 (jhahn)
**	    Added struct QEF_CP_PARAM and modified QEF_CALLPROC
**	    for handling nested calls and byref parameters in DB procs.
**      27-oct-92 (anitap)
**          Added QEA_CREATE_SCHEMA and QEA_EXEC_IMM action types for CREATE
**          SCHEMA project. Also added the structures QEF_CREATE_SCHEMA and
**	    QEF_EXEC_IMM for the actions.
**	02-dec-92 (rickh)
**	    Added QEF_CREATE_INTEGRITY_STATEMENT.
**	07-dec-92 (teresa)
**	    added new action: QEA_D1_REGPROC, which uses structure
**	    QEQ_D10_REGPROC
**	23-dec-92 (andre)
**	    defined QEF_CREATE_VIEW
**	02-jan-93 (rickh)
**	    Added several fields to QEF_CREATE_INTEGRITY_STATEMENT to hold
**	    DSH row numbers of data that must persist over EXEC IMMED actions.
**	12-jan-93 (jhahn)
**	    Improvements for FIPS CONSTRAINTS.
**	17-feb-93 (rickh)
**	    Added schema name to QEF_CREATE_INTEGRITY_STATEMENT.
**	13-mar-93 (andre)
**	    defined QEF_DROP_INTEGRITY and added qhd_drop_integrity to qhd_obj
**	2-apr-93 (rickh)
**	    Added integrity number to QEF_CREATE_INTEGRITY.
**	17-may-93 (jhahn)
**	    Added ahd_failed_b_temptable and ahd_failed_a_temptable to QEF_QP
**	     for support of statement level unique indexes.
**	9-aug-93 (rickh)
**	    Added a flag to the QEF_CREATE_INTEGRITY statement instructing
**	    QEF to cook up a SELECT statement verifying that a REFERENTIAL
**	    CONSTRAINT actually holds.
**	26-aug-93 (rickh)
**	    Added flag to integrity action to remind us to save the rowcount
**	    if this NOT NULL integrity is part of a CREATE TABLE AS SELECT
**	    statement.
**	13-jan-94 (rickh)
**	    Removed a dreaded trigraph sequence.
**	17-jan-94 (rickh)
**	    In the CREATE INTEGRITY header, split the modifyRefingProcedure
**	    field into two fields: insertRefingProcedure and
**	    updateRefingProcedure.
**	18-apr-94 (rblumer)
**	    in QEF_CREATE_INTEGRITY_STATEMENT,
**	    changed qci_integrityTuple2 to qci_knownNotNullableTuple.
**      19-dec-94 (sarjo01) from 18-feb-94 (markm)
**          added the vl_mustlock flag to make sure that we readlock even
**          if "nolocks" has been set when updating a relation. (58681)
**	29-april-1997(angusm)
**	    Flag up cross-table updates so we can enforce ambiguous
**	    replace checking properly (bug 79623) - add QEA_XTABLE_UPDATE
**	10-aug-1995 (cohmi01)
**	    Add QEF_SET_TIDJOIN to validation flags, for prefetch calculation.
**	7-nov-95 (inkdo01)
**	    Replaced many instances of QEN_ADF and several of QEN_TEMP_TABLE 
**	    with pointers to those structures. Moved qhdobj union to end of 
**	    QEF_AHD.
**	12-nov-1998 (somsa01)
**	    In QEF_VALID, added QEF_SESS_TEMP for Global Temporary Tables.
**	    (Bug #94059)
**      12-Apr-2004 (stial01)
**          Define ahd_length as SIZE_TYPE.
**	12-Dec-2005 (kschendel)
**	    Replace remaining in-line QEN_ADF structs with pointers so that
**	    we can handle them more generically.
**	4-Jun-2009 (kschendel) b122118
**	    Some comment improvements.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query - Moved some constants to enums for ease
**	    of debugging.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_line@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _QEF_VALID QEF_VALID;
typedef struct _QEQ_TXT_SEG	QEQ_TXT_SEG;
typedef struct _QEQ_LDB_DESC	QEQ_LDB_DESC;
typedef struct _QEF_EXEC_IMM	QEF_EXEC_IMM;
typedef	struct _QEF_RELATED_OBJECT	QEF_RELATED_OBJECT;
typedef struct _QEF_SCROLL	QEF_SCROLL;


/*}
** Name: QEF_PROC_RESOURCE - DBP specific info for resources
**
** Description:
**
**	This variant of a resource list entry is for table procedures
**	used by the query plan.  There is one entry per distinct
**	table proc.
**
**	DB procedures aren't "validated" as such, so we don't make
**	resource list entries for other DB procedure usages (explicit
**	callproc or rule-fired dbproc).  Table procs are more deeply
**	entwined into a particular action's qep, though, and the
**	tproc is opened and its tables validated at the start of the
**	QP just like we do for tables.
**
** History:
**      99-foo-93 (jhahn)
**          created
**	13-jan-94 (rickh)
**	    Removed a dreaded trigraph sequence.
**	5-june-2008 (dougi)
**	    Added qr_procedure_qpix, name, owner for table procedures.
**	8-dec-2008 (dougi)
**	    Drop qr_procedure_qpix - it's no longer needed.
**	17-Jun-2010 (kschendel) b123775
**	    Get rid of valid list for table procs.  Make dbpalias a
**	    real QSO_NAME.
*/
typedef struct _QEF_PROC_RESOURCE
{
    /* The procedure ID is available to QEF, but at the moment it's
    ** only used by OPF to aid in generating the resource list.
    */
    i4 qr_procedure_id1; /* high order of procedure id */
    i4 qr_procedure_id2; /* low order of procedure id */

    /* Front-end / alias / "real" name of the table procedure.
    ** The db_cursor_id part of the DB_CURSOR_ID here is zero.  It will
    ** get filled in by QSF when the alias is translated.  The translated
    ** DB_CURSOR_ID is stored in the runtime resource list.
    */
    QSO_NAME	qr_dbpalias;	/* Alias name of table procedure */

    /* If anything other than tprocs ever generate PROC resources,
    ** we'll probably need a is-tproc boolean here.
    */
} QEF_PROC_RESOURCE;

/*}
** Name: QEF_MEM_CONSTTAB - defines MS Access OR transformed in-memory 
**	constants table
**
** Description:
**      This holds the address of the in-memory table, along with the
**	number of "rows" and row size.
[@comment_line@]...
**
** History:
**	11-feb-97 (inkdo01)
**	    Written.
[@history_line@]...
*/
typedef struct _QEF_MEM_CONSTTAB 
{
    PTR		qr_tab_p;		/* address of table. Rows are 
					** stored in contiguous memory */
    i4		qr_rowcount;		/* number of "rows" */
    i4		qr_rowsize;		/* size of each row (in bytes) */
} QEF_MEM_CONSTTAB;

/*}
** Name: QEF_TBL_RESOURCE - Table specific info for resources.
**
** Description:
**      This holds the resource information required to open and validate
**      tables. There are three types of tables (temp, set input, and
**      "regular" tables) since they get different validation and opening
**      treatment.
**	There is only one resource struct for a table in a qp, even if there
**	are multiple correlation references to the table.
**      All of the QEF_VALID structs for a table hang off this struct.
[@comment_line@]...
**
** History:
**      5-oct-93 (eric)
**          created
**	11-feb-97 (inkdo01)
**	    Added flag and pointer for MS Access OR transformation (in memory
**	    constant table)
**	17-Jun-2010 (kschendel) b123775
**	    Take timestamp out of valid list, put here, since this is what
**	    does the actual validating.
*/
typedef struct _QEF_TBL_RESOURCE
{
    i4		qr_tbl_type;
#define	    QEQR_REGTBL	    1
#define	    QEQR_TMPTBL	    2
#define	    QEQR_SETINTBL   3
#define	    QEQR_MCNSTTBL   4

	/* If this is a temp table, then qr_temp_id identifies which one.
	** It is set to opf's global range variable number. This is used
	** only by OPC as an aid in setting up the resource list.
	*/
    PTR		qr_temp_id;

	/* Qr_valid is a listed list of valid structs that
	** use the table for this resource. They are linked by the vl_alt
	** field. Qr_lastvalid is a pointer to the last one on the list.
	*/
    QEF_VALID	*qr_valid;
    QEF_VALID	*qr_lastvalid;
	/* If this is a MS Access OR transform in-memory constant table, 
	** qr_cnsttab_p is the address of its defining structure.
	*/
    QEF_MEM_CONSTTAB	*qr_cnsttab_p;

	/* The timestamp used for the actual validation of the table */
    DB_TAB_TIMESTAMP
                qr_timestamp;	/* timestamp to validate with. 0 indicates
				** validation not required.
				*/
} QEF_TBL_RESOURCE;

/*}
** Name: QEF_RESOURCE - Defines a object resource that is used by a query plan
**
** Description:
**      This struct forms a linked list of all database objects that a query
**      plan uses. There will be one struct for each object (table, dbproc,
**      etc) used, optionally with all usage instances of the object hanging 
**	off of it.
**      This structure servers several functions:
**	    - it generalizes the list of objects to be validated from
**	      tables to all objects.
**	    - it provides a unique list of objects
**	    - it plays a part in sharing open tables (dmr_cb's), which
**	      reduces the amount of resources that a query plan needs.
**       
**      Currently, only tables and procedures are defined. In the future,
**      other db objects should be added to the list.
**       
**      The resource list is now the "real" list of object to be validated.
**      The QEF_VALID struct now indicates that a table needs to be opened,
**      not validated as it's name implies. Unfortunately, QEF_VALID structs
**      are everywhere, so the name stands for the time being.
[@comment_line@]...
**
** History:
**      15-oct-93 (eric)
**          created
[@history_template@]...
*/
typedef struct _QEF_RESOURCE
{
    struct _QEF_RESOURCE *qr_next; /* forward pointer */
    i4  qr_type;		/* type of resource to be verified */
#define QEQR_PROCEDURE 1	/* stored procedure resource */
#define QEQR_TABLE     2	/* table resource */

	/* Qr_id_resource is a monotonically increasing number (starting
	** with 0) for each table in the resource list.  It's used to
	** index into the dsh_resources array, addressing the mutable
	** part of the resource list.  (The QEF_RESOURCE itself is of
	** course read-only during execution, as multiple sessions might
	** be sharing the query plan.)
	*/
    i4		qr_id_resource;

    union
    {
	QEF_PROC_RESOURCE qr_proc;
	QEF_TBL_RESOURCE  qr_tbl;
    } qr_resource;	/* union of possible resources */
} QEF_RESOURCE;

/*}
** Name: QEF_VALID - List of tables to open
**
** Description:
**	The valid list is an obsolete name for the list of tables to
**	open.  Each action that needs tables opened has a list of
**	QEF_VALID entries indicating which tables and what mode to use.
**	A table might be listed multiple times if there are multiple
**	references (correlation names / cursors) to that table.
**	The action list is linked by the vl_next field.
**
**	Each valid list entry is also accessible from a resource list
**	entry, which is the actual validator entry.  vl_alt links
**	all valid entries for the same table (ie resource).
**
**	By convention, the last element in the list is the result
**	relation (if there is any).   (is this still true??)
**
**	Valid-list entries are part of the query plan, and as such are
**	read-only.  Limited exceptions are made for situations where the
**	query plan can't reasonably be shared, such as a CTAS, where we
**	need to fill in the actual table ID created.
**
**	Note: with the creation of the resource list, QEF_VALID no longer
**	represents the list of tables to validate. Instead, the QEF_VALID
**	struct represents an open instance of a table in a query plan 
**	(QEF_VALID always has had the latter meaning, it no longer has the
**	former meaning). Along with this change, QEF_VALID structs now live
**	on the top most action that uses them (meaning that actions below
**	QEN_QP nodes still don't have valid structs).
**
** History:
**     18-mar-86 (daved)
**          written
**	28-jul-87 (daved & puree)
**	    add a field, vl_alt, which groups all valid list entries
**	    of the same range variable.
**	16-feb-88 (puree)
**	    added the vl_est_pages, vl_total_pages, and vl_status.
**	    Removed the vl_lock_mode.  QEF will figure out the appropriate
**	    lock mode from vl_est_pages and vl_rw.
**	12-Jan-93 (jhahn)
**	    added vl_flags and vl_tab_id_index for handling set input
**	    parameters.
**      15-oct-93 (eric)
**          Added support for resource lists
**      19-dec-94 (sarjo01) from 18-feb-94 (markm)
**          added the vl_mustlock flag to make sure that we readlock even
**          if "nolocks" has been set when updating a relation. (58681)
**	11-feb-97 (inkdo01)
**	    Add QEF_MCNSTTBL flag to identify MS Access OR transformed
**	    in memory constant tables (vl_dmr_cb is used to store table
**	    address for these guys)
**	12-nov-1998 (somsa01)
**	    Added QEF_SESS_TEMP for Global Temporary Tables.  (Bug #94059)
**	3-dec-03 (inkdo01)
**	    Updated for partitioned table access.
**	5-june-2008 (dougi)
**	    Added QEF_TPROC flag for table procedures.
**	20-Nov-2009 (kschendel) SIR 122890
**	    Add needs-Xlock flag.
**	17-Jun-2010 (kschendel) b123775
**	    Move timestamp to resource list;  delete unused members;
**	    drop tproc flag, we don't make valid list entries for tprocs
**	    any more.
*/
struct _QEF_VALID
{
    QEF_VALID   *vl_next;           /* next file to open */
    DB_TAB_ID	vl_tab_id;	    /* table to open - main table in the
				    ** case of partitioning */
    i4	vl_rw;		    /* read or write access to table */
				    /* use DMF constants */

/* FIXME:  this estimated / total / size-sensitive stuff could be moved
** to the resource list entry.  Need to make sure that qp->qp_setInput
** is tied in properly before doing that.
*/
    i4	vl_est_pages;	    /* # of page that OPF estimates will be
				    ** touched by this query. */
    i4	vl_total_pages;	    /* total # of pages in the relation at
				    ** optimization time */
    bool	vl_size_sensitive;  /* TRUE if the QP sensitive to # of pages
				    ** in the relation */
    i4	        vl_dmf_cb;          /* index into DSH->DSH_CBS 
				    ** containing the DMT_CB to open file.
				    ** For partitioned tables, this is index
				    ** to start of block of DMT_CBs, one per
				    ** partition, with an additional [-1]
				    ** entry for the master.
				    */
    i4		vl_dmr_cb;	    /* index into the DSH->DSH_CBS containing
				    ** the DMR_CB.  Partitions handled same
				    ** as DMT_CB's.
				    */
    PTR		vl_debug;
    QEF_VALID	*vl_alt;	    /* next valid entry for current range var */
    i4		vl_flags;
#define QEF_SET_INPUT	0x01	    /* A set-input temporary table, see
				    ** vl-tab-id-index below */
#define QEF_REG_TBL	0x02	    /* Resource is a REGTBL resource */
#define QEF_MCNSTTBL	0x04	    /* MS Access OR in-memory constant table */
#define QEF_SESS_TEMP	0x08	    /* a Global Session Temporary Table */
#define	QEF_PART_TABLE	0x10	    /* a partitioned table */
/*	notused		0x20	*/
#define QEF_NEED_XLOCK	0x40	    /* If IX lock requested, use X instead.
				    ** This is needed if the table is going
				    ** to be LOADed.
				    */
    i4		vl_tab_id_index;    /* Only needed in set-input DB procs:
				    ** this gives the dsh "row" number of a
				    ** temp that holds the db-tab-id of the
				    ** actual gtt input param, as set up by
				    ** the callproc action.  Valid-list
				    ** entries that name the gtt inside the
				    ** DBP use vl-tab-id-index to retrieve
				    ** the table ID for the current dbp
				    ** invocation.  "crude, but effective."
				    */

    i4		vl_partition_cnt;   /* count of partitions in partitioned 
				    ** table. */
    DB_TAB_ID	*vl_part_tab_id;    /* ptr to array of tab_id's for 
				    ** partitions */
    bool        vl_mustlock;        /* if query is an update or delete then
                                    ** set this flag, this is to ensure that
                                    ** later on we do not attempt noreadlock
                                    ** access.
                                    */

};


/*}
** Name: QEF_SEQUENCE	- ANSI sequence descriptor
**
** Description:
**      This contains descriptive information about sequences referenced 
**	by the query plan. There is both a global chain anchored in the
**	QP header (for opening/closing purposes) and a local chain anchored
**	in an action header (for requesting next/current values).
**
**	There's also a QEF_SEQ_LINK structure tacked on the end to allow
**	action headers to thread through sequence structures in a M : N
**	manner.
**
** History:
**	15-mar-02 (inkdo01)
**	    Written for sequence support.
*/
typedef struct _QEF_SEQUENCE
{
    DB_NAME		qs_seqname;	/* sequence name */
    DB_OWN_NAME		qs_owner;	/* owner */
    DB_TAB_ID		qs_id;		/* internal sequence ID */
    i4			qs_rownum;	/* DSH row index */
    i4			qs_cbnum;	/* DSH cbs index (for DMS_CB) */
#define QS_NEXTVAL	0x01		/* sequence requires next operation
					** (otherwise, just current) */
    i4			qs_flag;
    struct _QEF_SEQUENCE *qs_qpnext;	/* global thread (within whole QP) */
} QEF_SEQUENCE;


typedef struct _QEF_SEQ_LINK
{
    struct _QEF_SEQ_LINK *qs_ahdnext;	/* next link structure for action header */
    struct _QEF_SEQUENCE *qs_qsptr;	/* sequence it corresponds to */
} QEF_SEQ_LINK;

/*}
** Name: QEQ_TXT_SEG - Subquery text segment structure
**
** Description:
**      This data structure is used to supply a subquery's text.  If a
**  subquery includes one or more temporary tables whose names are to be
**  supplied by QEF or attribute types to be supplied by RQF, then the text 
**  must be broken into a list of segments terminated by a NULL.  Adjacent
**  segments separated by a temporary-table name or attribute type are 
**  connected by such a data structure specifying an index into the DD_NAME 
**  array whose element will contain an appropriate name at the time the
**  subquery is put together for the LDB.
**
** History:
**      09-aug-88 (carl)
**          written
**      14-jul-92 (fpang)
**          Removed typedef, it's forward declared above.
*/

struct _QEQ_TXT_SEG
{
    bool	    qeq_s1_txt_b;	/* TRUE if specifying a text segment, 
					** FALSE if a temporary table */
    DD_PACKET	    *qeq_s2_pkt_p;      /* ptr to text info */
    QEQ_TXT_SEG     *qeq_s3_nxt_p;	/* ptr to next text segment, NULL if
					** none */
};

/*}
** Name: QEQ_LDB_DESC - LDB descriptor construct
**
** Description:
**      This structure contains an LDB descriptor and a pointer to the next
**  LDB descriptor in a list, used by all DDB query plans.
**
** History:
**      09-aug-88 (carl)
**          written
**
**      14-jul-92 (fpang)
**          Removed typedef, it's forward declared above.
*/

struct _QEQ_LDB_DESC
{
    DD_LDB_DESC	     qeq_l1_ldb_desc;	    /* LDB desc */
    QEQ_LDB_DESC    *qeq_l2_nxt_ldb_p;	    /* ptr to next LDB desc, NULL
					    ** if none */
}   ;


/*}
** Name: QEF_SCROLL - scrollable cursor information.
**
** Description:
**	Structure contains information needed for execution of scrollable
**	(and possibly keyset) cursors.
**
** History:
**	5-apr-2007 (dougi)
**	    Written for scrollable cursors.
**	2-oct-2007 (dougi)
**	    Add row buffer slot for temp table row (for posn'd upd/del).
**	28-jan-2008 (dougi)
**	    Drop ahd_rssplix so it can be put in QP header.
**	8-Jun-2009 (kibro01) b122171
**	    Add in ahd_rsnattr to break up a scrolling cursor into multiple
**	    columns if they are larger than adu_maxstring.
*/
struct _QEF_SCROLL
{
    DMF_ATTR_ENTRY  **ahd_rsattr; 	/* ptr to result set "column" desc */
    i4		    ahd_rsdmtix; 	/* cbs index to result set DMT_CB */
    i4		    ahd_rswdmrix; 	/* cbs index to result set 
					** write DMR_CB */
    i4		    ahd_rsrdmrix; 	/* cbs index to result set 
					** read DMR_CB */
    i4		    ahd_rspsize; 	/* spooler page size */
    i4		    ahd_rsrsize; 	/* result set row size (keyset size
					** for KEYSET cursor) */
    i4		    ahd_rsrrow;		/* row no of result set temp table */
    i4		    ahd_rskscnt; 	/* count of keyset keys (or 0 if not
					** a keyset cursor) */
    DMF_ATTR_ENTRY  *ahd_rskattr; 	/* ptr to array of keyset key 
					** descriptors */
    i4		    *ahd_rskoff1; 	/* ptr to array of offsets of keys
					** in result set row */
    i4		    *ahd_rskoff2; 	/* ptr to array of offsets of keys
					** in base table row */
    QEN_ADF	    *ahd_rskcurr; 	/* ptr to CX to re-materialize
					** result set row from base table */
    i4		    ahd_rsbtrow; 	/* row no of base table row */
    i4		    ahd_rsbtdmrix; 	/* cbs index to base table DMR_CB */
    i4		    ahd_tidrow;		/* original tidrow for KEYSET */
    i4		    ahd_tidoffset;	/* original tidoffset for KEYSET */
    i4		    ahd_rsnattr;	/* Number of items in ahd_rsattr */
} ;

/*}
** Name: QEF_QEP - 
**
** Description:
[@comment_line@]...
**
** History:
**	30-mar-89 (paul)
**	    Added query cost estimates for resource control.
**	18-apr-89 (paul)
**	    Convert update cursor action to a standard QEP style action
**	    structure to support rules processing on cursors. Also add
**	    support for a delete cursor action.
**      25-oct-89 (davebf)
**          Add ahd_ruprow to QEF_QEP to hold index of dsh_row of row
**          update-able by cursored update
**	17-may-93 (jhahn)
**	    Added ahd_failed_b_temptable and ahd_failed_a_temptable for
**	    support of statement level unique indexes.
**      19-jun-95 (inkdo01)
**          Replaced 2-valued i4  ahd_any with flag word ahd_qepflag
**	7-nov-95 (inkdo01)
**	    Replaced many instances of QEN_ADF and several of QEN_TEMP_TABLE 
**	    with pointers to those structures. Moved qhdobj union to end of 
**	    QEF_AHD.
**	29-oct-97 (inkdo01)
**	    Added AHD_NOROWS to indicate contained query will return nothing.
**	19-dec-96 (inkdo01)
**	    Added ptr to bitmap of columns in SET list of UPDATE, to improve
**	    update performance.
**	22-oct-98 (inkdo01)
**	    Added AHD_FORGET for GET on select of for-loop.
**	    Added AHD_NOVALIDATE for GET on select of for-loop.
**	3-mar-00 (inkdo01)
**	    Added ahd_firstn for "select first n ..." feature.
**	6-feb-01 (inkdo01)
**	    Earlier, made changes for hash aggregation, now (hopefully last)
**	    changes to handle overflow from hash aggregation.
**	21-apr-01 (inkdo01)
**	    One last change - add work buffer index for overflow ADF code.
**	15-mar-01 (inkdo01)
**	    Added link to sequence chain (for sequence support).
**	20-jan-04 (inkdo01)
**	    Added DB_PART_DEF * to action header for insert/update (/load?)
**	    of partitioned tables.
**	5-feb-04 (inkdo01)
**	    Added AHD_PCOLS_UPDATE flag to indicate update set clause refs
**	    a partitioning column and row may move to another partition.
**	18-Jul-2005 (schka24)
**	    Add ahd_ruporig to hold the node number of the originating
**	    node (the one marked QEN_PUPDDEL_NODE).
**	7-Dec-2005 (kschendel)
**	    Change hash-agg attr list into compare list.
**	14-june-06 (dougi)
**	    Add ahd_before_act for BEFORE triggers.
**	22-jan-2007 (dougi)
**	    Tidy up a bit (drop some long disused fields) and add another
**	    substructure to u1 to define scrollable cursor stuff.
**	5-apr-2007 (dougi)
**	    Move scrollable cursor info to a separate structure.
**	18-july-2007 (dougi)
**	    Add ahd_offsetn for result offset clause.
**	17-Jul-2007 (kschendel) SIR 122513
**	    Add flag for PC aggregation.
**	4-Aug-2007 (kschendel) SIR 122512
**	    Add FOR nesting depth counter, and contains-hashop flag.  Both
**	    are for the new hash reservation scheme.
**	13-dec-2007 (dougi)
**	    Changes to support variable offset "n"/first "n".
**	4-Jun-2009 (kschendel) b122118
**	    Add is-main flag for QE11 trace point.  Rearrange slightly for
**	    better structure packing.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Add load-CTAS flag.
*/
typedef struct _QEF_QEP
{
    QEN_NODE       *ahd_qep;            /* first node in QEP tree */
    QEN_NODE       *ahd_postfix;        /* postfix list of QEP */
    i4              ahd_qep_cnt;        /* number of nodes in QEP */
    QEN_ADF	   *ahd_current;	/* structure necessary
					** to materialize current row.
					** The result of this value
					** is ready for a replace call
					** an append call or to give
					** to the user.
					** If this is an QEA_AGGF 
					** action, this CX computes
					** the agg values.
					*/
    QEF_AHD	    *ahd_before_act;	/* Action list to execute before each
					** row of this QEP has been processed */
    QEF_AHD	    *ahd_after_act;	/* Action list to execute after	    */
					/* each row of this QEP has been    */
					/* processed. Applies only to	    */
					/* update, delete, insert	    */
					/* operations currently.*/

    i4		    ahd_odmr_cb;	/* control block number for
					** use in updates, deletes
					** and appends. Set to -1
					** if not needed. This value
					** is an index into the 
					** DSH->DSH_CBS struct where
					** a pointer to a DMR_CB should
					** be found. If this DMR_CB
					** refers to a table that is
					** also refered to by another
					** (different) DMR_CB, 
					** the table needs to
					** be opened twice so both
					** DMR_CBs have their own record
					** access id. This is done by
					** creating an additional
					** validation list entry.
					** For cursor update, this is the DMF
					** control block containing the update
					** row in the associated query. This
					** value is an index into dsh->dsh_cbs
					** where the DMR_CB can be found.
					** Previously ahd_cb in the QEF_RUP
					** structure.
					*/
    i4		    ahd_tidrow;		/* Index into DSH->DSH_ROW
					** containing row with the
					** update tid.
					*/
    i4		    ahd_tidoffset;	/* offset in above row of tid */
    i4		    ahd_proj;		/* index to DMR cb for projected table
					** for aggregate function.
					*/
    QEN_ADF	    *ahd_constant;	/* constant (no vars) expression.
					** For cursor update, the update
					** qualification expression */
    PTR		    ahd_qhandle;	/* Handle for associated QP for	    */
					/* cursor update and delete. */
    QEN_ADF	    *ahd_by_comp;	/* if ahd_proj_qep is used, this
					** field allows the projected
					** by-list values to be compared
					** to the by-list values 
					** in the agg computation scan.
					** if the projected by-list is
					** less than the computation by
					** list, this expression 
					** projects a default agg result
					** tuple.
					**
					*/
    QEF_SEQ_LINK    *ahd_seq;		/* for GET/APP/UPD headers with 
					** sequence operations 
					*/
    i4		    ahd_qepflag;        
#define AHD_ANY             0x01	/* TRUE if this is a solo,
					** simple ANY aggregate.
					** Also, set to TRUE if target
					** list has no variables
					** and you want execution
					** to stop after one row
					** has been found. For example,
					** target list is constant.
					*/
#define AHD_CSTAROPT        0x02	/* optimizeable count(*) */
#define AHD_NOROWS	    0x04	/* query tree under this action
					** has predicates which assure no
					** rows will be returned */
#define AHD_FORGET	    0x08	/* TRUE if this QEA_GET owns the
					** select qtree of a for-loop. It
					** is looped over and must not 
					** perform the qeq_validate init
					** logic.
					*/
#define	AHD_PART_TAB	    0x10	/* TRUE if inserting/updating
					** a partitioned table (DB_PART_DEF
					** stuff has been copied)
					*/
#define AHD_PCOLS_UPDATE    0x20	/* TRUE if update set clauses 
					** reference at least one partitioning
					** column (row may change p's)
					*/
#define AHD_4BYTE_TIDP	    0x40	/* TRUE if ahd_tidoffset addrs
					** a 4-byte tidp */
#define	AHD_SCROLL	    0x80	/* TRUE if this is the GET on top
					** of a scrollable cursor */
#define	AHD_KEYSET	    0x100	/* TRUE if this is a KEYSET scrollable
					** cursor */
#define	AHD_PARM_FIRSTN	    0x200	/* TRUE if ahd_firstn is a parm index
					** and not an integer constant */
#define	AHD_PARM_OFFSETN    0x400	/* TRUE if ahd_offsetn is a parm index
					** and not an integer constant */

#define AHD_LOAD_CTAS	    0x800	/* For QEA_LOAD only: set if loading
					** a newly created table, guaranteed
					** to be empty (so we can tell DMF).
					** On for CTAS, off for insert/select
					*/

#define AHD_PART_SEPARATE   0x1000	/* "Partition compatible" aggregation,
					** all rows going into a group must
					** arrive from the same partition, so
					** when "end-of-group" is signalled
					** from below, can flush pending and
					** reset.  Only used for hash agg at
					** the present time (wouldn't be much
					** use for sorted aggs).
					*/

#define AHD_CONTAINS_HASHOP 0x2000	/* This action is a hash agg,
					** or contains a hash join somewhere
					** in the associated query subplan.
					** Qee uses this to skip hash operation
					** allocation processing for actions
					** with no hash-ops underneath.
					*/

#define AHD_MAIN	    0x4000	/* This action is part of a "main"
					** subquery, so it's returning rows
					** to the user and not to a QP node.
					** Used for the QE11 trace point.
					*/

	/* Ahd_reprow and ahd_repsize give the row number and the
	** size of the source row for a positioned update (QEA_PUPD).
	** This is the row in its pre-replaced state and should be
	** copied on top of the post-replaced row before the
	** ahd_current is evaluated. If ahd_reprow is -1 then no
	** copying is needed, even if this is a positioned update.
	*/
    i4		    ahd_reprow;
    i4		    ahd_repsize;

    QEF_SCROLL	    *ahd_scroll;	/* ptr to scrollable cursor stuff */

	/* The following union simply shares storage used for update/delete
	** action headers and storage used for aggregation action headers.
	*/
    union {
	struct {
	    i4		    ahd_b_row;	/* Index into DSH_ROW at which the  */
					/* before image of a modified row   */
					/* is placed. */
	    i4		    ahd_a_row;	/* Index into DSH_ROW at which the  */
					/* after image of a modified row is */
					/* placed. */
	    i4		    ahd_b_tid;	/* Index into DSH_ROW at which the
					** tid for the 'old' row is placed.
					*/
	    i4		    ahd_a_tid;	/* Index into DSH_ROW at which the
					** tid for the 'new' row is placed.
					*/
	    QEN_TEMP_TABLE  *ahd_failed_b_temptable;
					/* hold file for storing before images
					** of potentialy failed updates for
					** handling statement level unique
					** constraints.
					*/
	    QEN_TEMP_TABLE  *ahd_failed_a_temptable;
					/* hold file for storing after images
					** of potentialy failed updates for
					** handling statement level unique
					** constraints.
					*/
	} s1;

	struct {
	    i4		    ahd_agwbuf;	/* Index into ahd_current buffer array
					** of aggregate work buffer.
					*/
	    i4		    ahd_aghbuf;	/* Index into ahd_by_comp buffer array
					** of aggregate hash buffer.
					*/
	    i4		    ahd_agobuf;	/* Index into ahd_agoflow buffer array
					** of aggregate overflow buffer.
					*/
	    i4		    ahd_agowbuf; /* Index into ahd_agoflow buffer array
					** of aggregate work buffer (for MAIN code).
					*/
	    i4		    ahd_aggest;	/* estimated number of groups */
	    i4		    ahd_agcbix;	/* index into dsh_hashagg for ptr to
					** hash aggregation root structure */
	    i4		    ahd_agdmhcb; /* index into dsh_cbs for ptr to 
					** DMH_CB for overflow files (if needed) */
	    i4		    ahd_bycount; /* Count of group by exprs */
	    DB_CMP_LIST	    *ahd_aghcmp; /* Pointer to base of compare-list
					** descriptors for hash-aggregate
					** by-list attributes.  Actually this
					** is degenerate, we transform the
					** concatenated by-list vals into one
					** big BYTE(n) entry
					*/
	    QEN_ADF	    *ahd_agoflow; /* Hash aggregation overflow 
					** ADF CX */
	} s2;
	    
    } u1;

    i2		    ahd_for_depth;	/* FOR-loop nesting level */

    i1		    ahd_duphandle;      /* what to do on 
					** replace/append if duplicate
					** key encountered
					*/
#define QEF_SKIP_DUP	    0			/* skip on duplicated key */
#define QEF_ERR_DUP	    1			/* error on unique key update */

    /* The following fields contains the cost estimates generated by OPF    */
    /* for executing this query. The convention is that the action header   */
    /* for the first action of a query contains the cost estimate for the   */
    /* entire query. Additional action headers contain zero in the cost	    */
    /* estimate fields. OPC summarizes the estimated costs into the first   */
    /* action header. Additionally, if the cost estimate is zero, it is not */
    /* checked against the user's resource limit. */
    bool	    ahd_estimates;	/* 1 == no estimates in this node    */
					/* 0 == estimates in this node */
    i4	    ahd_cost_estimate;	/* The total cost estimate	    */
					/* generated by OPF. It is a	    */
					/* function of cpu and io costs. */
    i4	    ahd_dio_estimate;	/* Logical IO estimate. The	    */
					/* optimizer does know about	    */
					/* multi-page reads so this value   */
					/* is not the same as the page	    */
					/* estimate. */
    i4	    ahd_cpu_estimate;	/* CPU cost estimate */
    i4	    ahd_row_estimate;	/* The estimate number of rows	    */
					/* being returned to the user by    */
					/* this QEP. This estimate is valid */
					/* only for GET AND OPEN CURSOR	    */
					/* operations */
    i4    	    ahd_page_estimate;	/* Estimated number of logical	    */
					/* pages processed by this QEP.     */
    /* Cursor gets that fetch in-place might be the driving QP for a
    ** nested in-place update (RUP or RDEL or PUPD).  The nested updating
    ** statement might need to get at the source's current partition number.
    ** It's also going to need to get at the row number that's holding the
    ** currently fetched row.  ahd_ruprow holds that row number.  ahd_ruporig
    ** holds the node number of the originating node so that execution time
    ** can get at that node's status info and extract the current partition.
    ** (ahd_ruporig is copied to the RUP QP, ahd_ruprow isn't.)
    */
    i4              ahd_ruprow;          /* Index into DSH_ROW of cursor     */
                                        /* update-able row                  */
    i4		    ahd_ruporig;	/* Node number of originating node that
					** fetches said row.  Probably an orig
					** but doesn't have to be. NOTE! in
					** an RUP QP, refers to node #'s in
					** the assoc get QP, *not* the rup qp.
					*/
    DB_COLUMN_BITMAP *ahd_upd_colmap;	/* update columns bitmap            */
					/* ptr to bit map of columns in SET
					** list of UPDATE statement, or nil
					*/
    i4		    ahd_firstn;		/* Number of rows to return if 
					** "select first n ..." was 
					** specified.
					*/
    i4		    ahd_offsetn;	/* Number of rows to skip if
					** result offset clause was specified.
					*/
    i4		    ahd_firstncb;	/* dsh_cbs[] index for QEA_FIRSTN,
					** or -1 if not needed */
    DB_PART_DEF	    *ahd_part_def;	/* address of table partitioning 
					** definition structure (for insert/
					** update to partitioned tables) */
    i4		    ahd_dmtix;		/* index to DSH_CBS of result table
					** DMT_CB */
} QEF_QEP;



/*}
** Name: QEF_DMU - 
**
** Description:
**	A DMU action header is compiled for CREATE TABLE AS SELECT
**	(or DGTT AS SELECT).  Actually, several DMU action headers may
**	be compiled, depending on circumstances.
**
**	DMU action headers can also be compiled for situations that
**	require an implicit temp table, such as subqueries that
**	generate function aggregates, or projections of by-lists.
**
**	A DMU action header indicates a DMU operation (create, modify, etc)
**	is to take place within the context of an enclosing statement.
**	DMU operations can take place in the context of a standalone
**	statement as well, but in that case either a different kind of
**	action header is compiled (e.g. CTBL for CREATE TABLE), or
**	no query plan is produced at all (e.g. MODIFY and CREATE INDEX,
**	which flow directly from the parse to the sequencer to QEF
**	into the QEU_DBU function).
**
**	The action header has a pointer to the DMF control block, which
**	is typically a DMU_CB.  For implicit temporary table creation,
**	the control block might be a DMT_CB.  (QEF will be doing a
**	direct create-temp call to DMF instead of a DMU call that
**	asks for a temporary table.  I guess there's a difference.
**	Also QEF does some extra whirling to re-use implicit temporary
**	tables in some cases.)
**
**	When an action creates (or possibly modifies) a partitioned
**	table, the individual physical partitions may have particular
**	characteristics, such as a location.  OPC will attach the
**	QEU_LOGPART_CHAR blocks created by the parse to this action
**	header, so that QEF can do the right thing as it creates
**	partitions.  For now, the QEU_LOGPART_CHAR list will only be
**	found attached to a CREATE TABLE action, but in theory such
**	a list could be attached to MODIFY or CREATE INDEX actions too.
**
**	Presently the only DMF actions that will be seen are create
**	table, modify table, and create temporary table.  All other
**	possibilities are compiled differently for now.
**
** History:
**	??? ???
**	    probably around the time of create schema.
**	9-Feb-2004 (schka24)
**	    Add the physical partition info list.
**	20-Feb-2004 (schka24)
**	    s/physical/logical/
*/
typedef struct _QEF_DMU
{
    i4		ahd_func;           /* DMF function id */
    void	*ahd_cb;	    /* control block template 
					** for DMF. The value
					** depends upon the type
					** of DMF function.
					** See the DMF/EIS for detail.
					*/
    /* pointer to QEF_VALID entry that uses this table.
    ** Each valid list entry references a DMT_CB.
    ** The table id field of this
    ** DMT_CB will be updated if this is a create command. If this
    ** is a create command and you do not want the table id field
    ** updated, set AHD_ALT to NULL. On other commands, if this field
    ** has a value, it refers to a valid list entry containing a
    ** DMT_CB that has the
    ** table id required for the command. If the AHD_CB field contains
    ** the correct table id, set the AHD_ALT value to NULL.
    ** 
    ** Note: Queries that require AHD_ALT to have non-null values
    ** also require that the table in question be opened in
    ** this query. It must be referenced in a AHD_VALID list. If
    ** the QP being built doesn't have any opens, use
    ** the QEU_DBU operation.
    */
    QEF_VALID	    *ahd_alt;            

    /* For DMU_CREATE_TABLE functions, if the table is to be partitioned,
    ** there may be a list of logical partition info blocks rooted here.
    */
    struct _QEU_LOGPART_CHAR *ahd_logpart_list;
} QEF_DMU;                              /* DMF action */



/*}
** Name: QEF_DDL_STATEMENT - describe a ddl statement
**
** Description:
[@comment_line@]...
**
** History:
**	17-aug-92 (rickh)
**	    Created for FIPS CONSTRAINS project.
**	17-nov-92 (rickh)
**	    Added pointer to qeu_cb.
[@history_template@]...
*/
typedef	struct	_QEF_DDL_STATEMENT
{
    PTR		ahd_qeuCB;	/* QEU_CB describing DDL statement */
    PTR		ahd_qeuqCB;	/* QEUQ_CB describing DDL statement */
} QEF_DDL_STATEMENT;


/*}
** Name: QEF_RENAME_STATEMENT - Descriptor for RENAME statement
**
** Description:
[@comment_line@]...
**
** History:
**	18-Mar-2010 (gupsh01) SIR 123444
**	    Added for rename table/columns.
**
*/
typedef	struct	_QEF_RENAME_STATEMENT
{
    PTR			qrnm_ahd_qeuCB;		/* QEU_CB describing DDL statement */
    PTR			qrnm_ahd_qeuqCB;	/* QEUQ_CB describing DDL statement */
    DB_TAB_ID           qrnm_tab_id;		/* Table id for table being renamed */
    DB_OWN_NAME         qrnm_owner;		/* owner of tab constraint applies to */
    i4			qrnm_type;		/* Type of rename operation */ 
#define			QEF_RENAME_TABLE_TYPE	0x0001
#define			QEF_RENAME_COLUMN_TYPE	0x0002
    DB_TAB_NAME         qrnm_old_tabname;	/* Original table name renamed */
    DB_TAB_NAME         qrnm_new_tabname;	/* new table name, if table rename op */
    DB_ATT_ID		qrnm_col_id;		/* Column id,   if column rename */	
    DB_ATT_NAME		qrnm_old_colname;	/* Column name, if column rename*/
    DB_ATT_NAME		qrnm_new_colname;	/* New column name, if column rename */
    DB_TAB_ID           qrnm_ctemp_tab_id;	/* Table id for temporary table to 
						** execute dependent action */
    i4			qrnm_ulm_rcb;		/* DSH row holding the ulm_rcb that
						** describes a memory stream which
						** persists over the execute immediate
						** actions for creating supportive
						** rules and procedures */
    i4			qrnm_state;	   	/* State of the next rename action */
#define			QEF_RENAME_INIT		0x0001	/* Initial state setting up of temp table */ 
#define			QEF_RENAME_CALLBACK 	0x0002  /* Callback to execute query text */
#define			QEF_RENAME_CLEANUP 	0x0003  /* Cleanup */
   i4			qrnm_pstInfo;			/* PST_INFO for qef callback call */
} QEF_RENAME_STATEMENT;

/*}
** Name: QEF_CREATE_INTEGRITY_STATEMENT - describe a create integrity statement
**
** Description:
[@comment_line@]...
**
** History:
**	02-dec-92 (rickh)
**	    Created for FIPS CONSTRAINTS project.
**	30-jan-93 (rickh)
**	    Added pointers to more objects which just persist across
**	    execute immediates.
**	17-feb-93 (rickh)
**	    Added schema name to QEF_CREATE_INTEGRITY_STATEMENT.
**	26-mar-93 (andre)
**	    added qci_indep.  When processing creation of a new REFERENCES
**	    privilege, it will point at a PSQ_INDEP_OBJECTS structure describing
**	    a UNIQUE or PRIMARY KEY constraint and, if the owner of the
**	    <referenced table> is different from that of the <referencing
**	    table>, description of a REFERENCES privilege on column(s) of the
**	    <referenced table> on which the new constraint will depend
**	2-apr-93 (rickh)
**	    Added integrity number to QEF_CREATE_INTEGRITY.
**	9-aug-93 (rickh)
**	    Added a flag to the QEF_CREATE_INTEGRITY statement instructing
**	    QEF to cook up a SELECT statement verifying that a REFERENTIAL
**	    CONSTRAINT actually holds.  Also added a remembrance of the 
**	    REFERENTIAL CONSTRAINT creator's initial state.
**	26-aug-93 (rickh)
**	    Added flag to integrity action to remind us to save the rowcount
**	    if this NOT NULL integrity is part of a CREATE TABLE AS SELECT
**	    statement.
**	17-jan-94 (rickh)
**	    In the CREATE INTEGRITY header, split the modifyRefingProcedure
**	    field into two fields: insertRefingProcedure and
**	    updateRefingProcedure.
**	18-apr-94 (rblumer)
**	    changed qci_integrityTuple2 to qci_knownNotNullableTuple.
**	31-mar-98 (inkdo01)
**	    Added fields to support constraint index definition with clause.
**	26-may-98 (inkdo01)
**	    Added flags for delete/update cascade/set null referential actions.
**	20-oct-98 (inkdo01)
**	    Added flags for delete/update restrict refact.
**	3-may-2007 (dougi)
**	    Added flag to force table to btree on constrained columns (instead
**	    of building secondary index).
*/
typedef	struct	_QEF_CREATE_INTEGRITY_STATEMENT
{
    i4			qci_flags;
#define	QCI_CONSTRAINT	0x00000001	/* set if this is an ANSI constraint */
#define QCI_SUPPORT_SUPPLIED	0x00000002
			/* no need to fabricate an index.  the user linked
			** this constraint to one at ALTER TABLE time.
			** (NOT supported. This pre-dates the constraint with 
			** clause which is supported.)
			*/
#define QCI_CONS_NAME_SPEC  0x00000004  /* name has been specified by user */

#define	QCI_SAVE_ROWCOUNT   0x00000008	/* save the rowcount from a previous
	** SELECT action.  During the execution of a CREATE TABLE AS SELECT
	** statement, we may have to add some NOT NULL integrities to the
	** target table.  This flag tells us to emit a correct rowcount
	** at the end of all the actions.
	*/

#define	QCI_VERIFY_CONSTRAINT_HOLDS	0x00000010
	/*
	** This tells QEF that we're adding a CONSTRAINT to a
	** pre-existing table.  This flag instructs QEF to cook up 
	** a SELECT query verifying that the condition holds.
	*/

#define QCI_INDEX_WITH_OPTS		0x00000020
	/*
	** "with" clause accompanied constraint definition to 
	** specifiy override index options. 
	*/

#define QCI_NEW_NAMED_INDEX		0x00000040
	/*
	** Explicitly named index to be created for this constraint.
	*/

#define QCI_OLD_NAMED_INDEX		0x00000080
	/*
	** Explicitly named, but pre-existing, referenced in 
	** constraint definition. No index needs to be created.
	*/

#define QCI_NO_INDEX			0x00000100
	/*
	** "with" clause specified no index to support constraint
	** (only permissable for referential constraints).
	*/

#define QCI_BASE_INDEX			0x00000200
	/*
	** Base table structure will support constraint.
	*/

#define QCI_SHARED_INDEX		0x00000400
	/*
	** Index for this constraint will be shared amongst others.
	*/

#define QCI_DEL_CASCADE			0x00001000
	/*
	** Constraint has DELETE CASCADE referential action.
	*/

#define QCI_DEL_SETNULL			0x00002000
	/*
	** Constraint has DELETE SET NULL referential action.
	*/

#define QCI_DEL_RESTRICT		0x00004000
	/*
	** Constraint has DELETE RESTRICT referential action.
	*/

#define QCI_UPD_CASCADE			0x00008000
	/*
	** Constraint has UPDATE CASCADE referential action.
	*/

#define QCI_UPD_SETNULL			0x00010000
	/*
	** Constraint has UPDATE SET NULL referential action.
	*/

#define QCI_UPD_RESTRICT		0x00020000
	/*
	** Constraint has UPDATE RESTRICT referential action.
	*/

#define QCI_TABLE_STRUCT		0x00040000
	/*
	** Modify table to btree on constrained columns, rather than
	** build secondary index.
	*/

    DB_TAB_ID	qci_suppliedSupport;	/* if PST_SUPPORT_SUPPLIED is
					** set, then this is the id of the
					** index (or base table) whose
					** clustering scheme supports this
					** constraint (NOT supported) */

    DB_INTEGRITY	*qci_integrityTuple;
		/* tuple to insert into iiintegrity.  PSF fills in the
		** constraint name, schema id, constraint flags, and
		** the bit map of restricted columns.
		*/

    DB_INTEGRITY	*qci_knownNotNullableTuple;
		/* 2nd tuple to insert into iiintegrity. Only used for
	        ** CHECK constraints that cause a column to become "known
		** "known not nullable."  PSF fills in the constraint
		** name, schema id, constraint flags, and the bit map of
		** "known not nullable" columns.
		*/
    DB_TAB_NAME	      qci_cons_tabname; /* table name constraint applies to */
    DB_OWN_NAME	      qci_cons_ownname; /* owner of tab constraint applies to */
    PTR		      qci_cons_collist; /* in-order linked list of unique
					** or referencing column ids.  these
					** are represented as PST_COL_IDs.
					*/
    DB_TAB_NAME	      qci_ref_tabname;  /* referenced table name (if any)   */
    DB_OWN_NAME	      qci_ref_ownname;  /* schema name for referenced table */
    PTR		      qci_ref_collist;	/* in-order linked list of 
					** referenced column ids, represented
					** as PST_COL_IDs.
					*/
    PTR		      qci_key_collist;	/* in-order linked list of column ids
					** of key columns in index (when index
					** is shared by more than one 
					** constraint), represented as
					** PST_COL_IDs.
					*/
    DB_CONSTRAINT_ID  qci_ref_depend_consid; 
		/* if a referential constraint, the unique constraint 
		** (on the referenced table) upon which this referential 
		** constraint depends 
		*/
    DB_TEXT_STRING	*qci_integrityQueryText;
		/* this is what gets stored in IIQRYTEXT */

    DB_TEXT_STRING	*qci_checkRuleText;
		/* a somewhat altered form of the query text.  this is
		** courteously cooked up for us by PSF.  we use this to
		** build the rule for CHECK constraints.  */

    PTR			qci_queryTree;	/* pointer to a PST_QTREE */


    DB_SCHEMA_NAME	qci_integritySchemaName;
		/* name of the schema that the integrity is defined in */
    PTR			qci_indep;
		/*
		** address of a PSQ_INDEP_OBJECTS structure describing object(s)
		** and/or privilege(s) on which the constraint being created
		** depends
		*/
    DB_TAB_NAME		qci_idxname;	/* index name (explicitly spec.) */
    DM_DATA		qci_idxloc;	/* index location (from with) */
    i4			qci_idx_fillfac;  /* fillfactor (from with) */
    i4			qci_idx_leaff;    /* leaffill (from with) */
    i4			qci_idx_nonleaff; /* non leaffill (from with) */
    i4		qci_idx_page_size;  /* pagesize (from with) */
    i4		qci_idx_minpgs;   /* minpages (from with) */
    i4		qci_idx_maxpgs;   /* maxpages (from with) */
    i4		qci_idx_alloc;    /* alloc (from with) */
    i4		qci_idx_extend;   /* extend (from with) */
    i4		qci_idx_struct;   /* ix structure (from with) */

    /*
    ** The following fields are the DSH row numbers of data that is filled
    ** in at QEF time and that must persist over the stacking of DSH contexts.
    ** That stacking occurs when supporting rules and procedures are cooked up
    ** and schlepped back to PSF for execute-immediating.  After each rule
    ** or procedure creation, we return to this parent action for further
    ** instruction.
    */

    i4			qci_internalConstraintName;
			/* DSH row holding the constraint name we construct
			** from the constraint ID */

    i4			qci_constraintID;
			/* DSH row holding the constraint's ID */

    i4			qci_integrityNumber;
			/* DSH row holding the constraint's integrity # */

    i4			qci_ulm_rcb;	/* DSH row holding the ulm_rcb that
					** describes a memory stream which
					** persists over the execute immediate
					** actions for creating supportive
					** rules and procedures */

    i4			qci_constrainedTableID;
					/* DSH row holding the ID of the
					** constrained table */

    i4			qci_referredTableID;
					/* DSH row holding the DB_TAB_ID
					** of the referenced table for
					** referential constraints */



    i4			qci_cnstrnedTabAttributeArray;
					/* DSH row holding a pointer to
					** the attribute array
					** of the constrained table */

    i4			qci_referredTableAttributeArray;
					/* DSH row holding a pointer to
					** the attribute array
					** of the referenced table */

    i4			qci_nameOfIndex;
					/* DSH row holding a
					** DB_TAB_NAME which in turn holds
					** the name of the index created for
					** a UNIQUE or REFERENTIAL CONSTRAINT*/

    i4			qci_nameOfInsertRefingProc;
					/* DSH row number holding a DB_NAME
					** containing the name of a procedure
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This procedure is fired
					** on inserts into the referring table*/

    i4			qci_nameOfUpdateRefingProc;
					/* DSH row number holding a DB_NAME
					** containing the name of a procedure
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This procedure is fired
					** on updates of the referring table*/

    i4			qci_nameOfDeleteRefedProc;
					/* DSH row number holding a DB_NAME
					** containing the name of a procedure
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This procedure is fired
					** on deletes from the referenced table*/

    i4			qci_nameOfUpdateRefedProc;
					/* DSH row number holding a DB_NAME
					** containing the name of a procedure
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This procedure is fired
					** on modifies of the referenced table*/

    i4			qci_nameOfInsertRefingRule;
					/* DSH row number holding a DB_NAME
					** containing the name of a rule
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This rule is fired
					** on inserts into the referencing
					** table*/

    i4			qci_nameOfUpdateRefingRule;
					/* DSH row number holding a DB_NAME
					** containing the name of a rule
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This rule is fired
					** on updates of the referencing
					** table*/

    i4			qci_nameOfDeleteRefedRule;
					/* DSH row number holding a DB_NAME
					** containing the name of a rule
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This rule is fired
					** on deletes from the referenced
					** table*/

    i4			qci_nameOfUpdateRefedRule;
					/* DSH row number holding a DB_NAME
					** containing the name of a rule
					** which enforces a REFERENTIAL
					** CONSTRAINT.  This rule is fired
					** on updates of the referenced
					** table*/

    i4			qci_pstInfo;	/* row holding PST_INFO structure */

    i4			qci_state;	/* DSH row holding state information:
					** which dependent action are we
					** cooking up? */

    i4			qci_initialState;
					/* DSH row containing the initial
					** value of the above variable. */

} QEF_CREATE_INTEGRITY_STATEMENT;

/*
** Name: QEF_CREATE_VIEW  - description of CREATE/DEFINE VIEW statement
**
** Description:
**	This structure contains a description of CREATE/DEFINE VIEW statement;
**	in essence this is a translation of PST_CREATE_VIEW structure which will
**	be understood by QEF
**
** History:
**	23-dec-92 (andre)
**	    written
**      28-feb-93 (andre)
**          added offsets into DSH for items that must persist across
**          EXECUTE IMMDIATE processing
**	28-may-93 (andre)
**	    changed type of ahd_crt_view_qeuqcb from PTR to (struct _QEUQ_CB *)
*/
typedef struct QEF_CREATE_VIEW_
{
    struct _QEUQ_CB	*ahd_crt_view_qeuqcb;
    DB_TAB_NAME		ahd_view_name;
    i4			ahd_crt_view_flags;


						/*
						** CHECK OPTION will need to be
						** enforced dynamically after
						** INSERT into the view
						*/
#define	    AHD_CHECK_OPT_DYNAMIC_INSRT     0x01

						/*
						** CHECK OPTION will need to be
						** enforced dynamically after
						** UPDATE; depending on whether
						** AHD_VERIFY_UPDATE_OF_ALL_COLS
						** is set, enforcement will have
						** to take place after any
						** UPDATE of the view or after
						** UPDATE involving columns
						** described by ahd_updt_cols
						*/
#define	    AHD_CHECK_OPT_DYNAMIC_UPDT	    0x02

						/*
						** will be set if UPDATE of
						** every updatable column of the
						** view has a potential to
						** result in violation of CHECK
						** OPTION
						*/
#define	    AHD_VERIFY_UPDATE_OF_ALL_COLS   0x04

						/*
						** creating a STAR view
						*/
#define	    AHD_STAR_VIEW		    0x08

    DB_COLUMN_BITMAP	ahd_updt_cols;
    i4			ahd_ulm_rcb;
    i4			ahd_state;
    i4			ahd_view_id;
    i4			ahd_view_owner;
    i4			ahd_check_option_dbp_name;
    i4			ahd_pst_info;
    i4			ahd_tbl_entry;
} QEF_CREATE_VIEW;

/*
** Name: QEF_DROP_INTEGRITY - description of ALTER TABLE DROP CONSTRAINT
**			      statement
**
** Description:
**	This structure contains a description of ALTER TABLE DROP CONSTRAINT
**	statement; in essence this is a translation of PST_DROP_INTEGRITY
**	structure which will be understood by QEF
**
** History:
**	13-mar-93 (andre)
**	    written
*/
typedef struct QEF_DROP_INTEGRITY_
{
    i4			    ahd_flags;
					    /*
					    ** this bit must be set to indicate
					    ** that destruction of an
					    ** "old-style" integrity is being
					    ** requested
					    */
#define	AHD_DROP_INGRES_INTEGRITY	0x01
					    /*
					    ** this bit must be set to indicate
					    ** that destruction of a (UNIQUE,
					    ** CHECK, or REFERENCES) cosntraint
					    ** is being requested
					    */
#define	AHD_DROP_ANSI_CONSTRAINT	0x02
					    /*
					    ** this bit must be set to indicate
					    ** that  cascading destruction of
					    ** integrity(ies) is being requested
					    */
#define	AHD_DROP_CONS_CASCADE		0x04
					    /*
					    ** this bit must be set to indicate
					    ** that all integrities of the
					    ** specified type ("old style"
					    ** and/or new-style) are to be
					    ** destroyed
					    */
#define	AHD_DROP_ALL_INTEGRITIES	0x08

					    /*
					    ** if AHD_DROP_INGRES_INTEGRITY bit
					    ** is set, this field will contain
					    ** integrity number if a single
					    ** integrity is being dropped;
					    ** if AHD_DROP_ALL_INTEGRITIES is
					    ** set, contents of ahd_integ_number
					    ** are undefined
					    */
    i4			ahd_integ_number;
    
					    /*
					    ** id of the table integrity(ies) on
					    ** which is/are to be dropped
					    */
    DB_TAB_ID		ahd_integ_tab_id;

					    /*
					    ** name of the table integrity(ies)
					    ** on which is/are to be dropped
					    */
    DB_TAB_NAME		ahd_integ_tab_name;

					    /*
					    ** schema to which integrity(ies)
					    ** belongs
					    */
    DB_OWN_NAME		ahd_integ_schema;


					    /*
					    ** if AHD_DROP_ANSI_CONSTRAINT bit
					    ** is set AND
					    ** AHD_DROP_ALL_INTEGRITIES bit is
					    ** not set (i.e. we are being asked
					    ** to drop a specific ANSI-style
					    ** integrity), this field will
					    ** contain name of the constraint to
					    ** be dropped
					    */
    DB_CONSTRAINT_NAME	ahd_cons_name;

					    /*
					    ** if AHD_DROP_ANSI_CONSTRAINT bit
					    ** is set, this field will contain 
					    ** id of the schema to which 
					    ** constraint(s) being dropped
					    ** belong
					    */
    DB_SCHEMA_ID	ahd_cons_schema_id;

} QEF_DROP_INTEGRITY;

/*}
** Name: QEF_RUP - 
**
** Description:
[@comment_line@]...
**
** History:
[@history_template@]...
*/
typedef struct _QEF_RUP
{
    PTR		    ahd_qhandle;	/* handle to associated query */
    i4              ahd_cb;             /* DMF control block containing
					** the update row in the
					** associated query.
					** This value is an index into
					** DSH->DSH_CBS where a DMR_CB
					** will be referenced.
					*/
    QEN_ADF	    *ahd_current;	/* update tuple 
					** replace operation.
					** This struct contains info
					** so that the new tuple can
					** be created.
					** The output tuple here is
					** the tuple in the update 
					** DMR_CB specified as the
					** update control block.
					*/
    i4		    ahd_duphandle;      /* what to do on 
					** replace/append if duplicate
					** key encountered;   
					** see QEF_QEP.ahd_duphandle for defines
					*/
} QEF_RUP;



/*}
** Name: QEF_IF - If statement specific info for actions.
**
** Description:
[@comment_line@]...
**
** History:
[@history_template@]...
*/
typedef struct _QEF_IF
{
	/* Ahd_condition is the condition expression for the if statment */
    QEN_ADF	    *ahd_condition;

	/* Ahd_true and ahd_false are pointers to actions that should be
	** executed if ahd_condition was TRUE or FALSE (respectively).
	*/
    QEF_AHD	    *ahd_true;
    QEF_AHD	    *ahd_false;
}   QEF_IF;



/*}
** Name: QEF_RETURN	- QP information specific to a RETURN statement.
**
** Description:
**      This structure contains information specific to the QEF_RETURN
**	action.  The ahd_rtn contains the following information:
**	    .qen_adf	    ADE CX to materialize the return status value.
**	    .qen_pos	    index to dsh_cbs that contains ADE_EXCB
**			    parameters.
**	    .qen_base
**		.qen_array  is QEN_OUT.
**	    .qen_uoutput    index to entry in qen_base that contains
**			    ptr to where the value of the return status
**			    goes.  Notice that the return status value
**			    is a non-nullable i4.
** History:
**      10-may-88 (puree)
**          created for a RETURN action in DB procedure.
[@history_template@]...
*/
typedef struct _QEF_RETURN
{
    QEN_ADF	    *ahd_rtn;
}   QEF_RETURN;



/*}
** Name: QEF_MESSAGE - QP information specific to a MESSAGE statement.
**
** Description:
**      This structure contains information specific to the QEF_MESSAGE
**	action.  There are two CXs in this structure; one for the message
**	number (ahd_mnumber) and the other for the message text (ahd_mtext).
**	The two QEN_ADXs contain the information below:
**	    .qen_adf	    ADE CX to materialize the message number/text.
**	    .qen_pos	    index to dsh_cbs that contains ADE_EXCB
**			    parameters.
**	    .qen_base
**		.qen_array  is QEN_OUT.
**	    .qen_uoutput    index to entry in qen_base that contains
**			    ptr to where the value of the message number/text
**			    goes.  Notice that the message number is a 
**			    non-nullable i4.
**
** History:
**      10-may-88 (puree)
**	    create for message action in DB procedure.
**	27-feb-91 (ralph)
**	    Added QEF_MESSAGE.ahd_mdest to support the DESTINATION clause
**	    of the MESSAGE and RAISE ERROR statements.
[@history_template@]...
*/
typedef struct _QEF_MESSAGE
{
    QEN_ADF	    *ahd_mnumber;
    QEN_ADF	    *ahd_mtext;
    i4		    ahd_mdest;
}   QEF_MESSAGE;


/*}
** Name: QEF_CP_PARAM - Describes a parameter being passed to a DB proc.
**
** Description:
**      This structure is used to keep a list  to describe the parameter sbeing
**	passed from a CALLPROC action.
**	The list will ibe maintained statically as part of the query plan. To keep
**	ahd_parm static between multiple uses of the same query plan
**	ahd_parm->parm_dbv->db_data instead of being set to point
**	to the parameter value will be set to NULL. The parameter value
**	will be stored in the caller's dsh and will be indicated by
**	ahd_parm_row and ahd_parm_offset.
**
** History:
**      26-oct-92 (jhahn)
**	    create for handling byref parameters in DB procs.
[@history_template@]...
*/
typedef struct _QEF_CP_PARAM
{
    QEF_USR_PARAM   ahd_parm;		/* Description of parameter */
    i4		    ahd_parm_row;	/* row no. in dsh of the parameter */
    i4		    ahd_parm_offset;	/* offset in row of parameter */
}   QEF_CP_PARAM;


/*}
** Name:    QEF_CALLPROC
**
** Description:
**	This structure contains the information required to execute a
**	call db procedure action. The structure contains the name of the
**	procedure to invoke and a CX to construct the actual parameters
**	for the procedure invocation.
**
**	The name of the procedure is represented as a QSF ALIAS to allow
**	QEF to look up the query plan by name. The QP will not be able to
**	store the actual QP ID since it contains a possibly changing
**	timestamp.
**
** History:
**	15-jan-89 (paul)
**	    Written to support rules processing.
**	09-may-89 (neil)
**	    Rules: Added ahd_rulename to QEF_CALLPROC for tracing.
**	01-jan-91 (pholman)
**	    Added ahd_ruleowner for security auditing
**      26-oct-92 (jhahn)
**	    removed ahd_plistno and added ahd_params for
**	    handling byref parameters in DB procs. Added ahd_return_status
**	    for nested procedure calls.
**	12-Jan-93 (jhahn)
**	    added ahd_procedureID and ahd_proc_temptable for set input
**	    procedures.
**	24-jul-96 (inkdo01)
**	    added ahd_gttid for global temptab procedure parms.
**	14-june-06 (dougi)
**	    Added ahd_procp_return to return values of output parms to
**	    before triggers and flag to indicate type of trigger.
**	15-Jan-2010 (jonj)
**	    Add QEF_CP_CONSTRAINT to identify a constraint so
**	    procedures without parameters are properly identified.
**	    This replaces QEN_CONSTRAINT.
**	22-Jun-2010 (kschendel) b123775
**	    Make dbp alias a real QSO_NAME.
*/
typedef struct _QEF_CALLPROC
{
    DB_NAME		ahd_rulename;	/* If this field is not empty then 
					** when the called procedure is invoked
					** this name should be traced.  This
					** allows rule names to be traced as
					** fired.  This field is unused for
					** regular nested procedure calls.
					*/
    DB_OWN_NAME		ahd_ruleowner;	/* For security auditing. */
    /* Alias structure for naming the DB procedure to call. Note that the    */
    /* timestamp component of the DB_CURSOR_ID will must be zero to look    */
    /* this procedure name up as an alias. */
    QSO_NAME		ahd_dbpalias;
    DB_TAB_ID		ahd_procedureID; /* ID of above procedure or 0 */

    QEN_TEMP_TABLE	*ahd_proc_temptable; /* NULL if not set input proc */

    QEN_ADF	    *ahd_procparams;	/* CX for creating actual	    */
					/* parameters (stored in DSH). */
    QEN_ADF	    *ahd_procp_return;	/* CX to return output parm values 
					** to BEFORE trigger caller */
    i4		    ahd_pcount;		/* Actual parameter count. */
    QEF_CP_PARAM    *ahd_params;	/* Actual parameter list */
    DB_TAB_ID	    ahd_gttid;		/* temptab id (if global temptab 
					** proc parm) */
    struct {
	i4		ret_rowno;		/* DSH row of return status */
	i4		ret_offset;		/* DSH offet of return status */
	DB_DATA_VALUE	ret_dbv;		/* DBV of return status */
    } ahd_return_status;
    i4		    ahd_flags;		/* flag byte */
#define	QEF_CP_BEFORE	0x01		/* BEFORE trigger */
#define	QEF_CP_AFTER	0x02		/* AFTER trigger */
#define	QEF_CP_OUTPARMS	0x04		/* procedure has OUT/INOUT parms */
#define	QEF_CP_CONSTRAINT 0x08		/* procedure is a constraint */
} QEF_CALLPROC;

/*}
** Name:    QEF_PROC_INSERT
**
** Description:
**	This structure contains the information required to execute a
**	insert for deferred call db procedure action. The structure contains
**	 the name of the procedure to invoke and a CX to construct the actual
**	 parameters for the procedure invocation.
**
** History:
**	15-oct-92 (jhahn)
**	    Written to support statement level rules processing.
*/
typedef struct _QEF_PROC_INSERT
{
    QEN_TEMP_TABLE	*ahd_proc_temptable;
    QEN_ADF	    *ahd_procparams;	/* CX for creating actual	    */
					/* parameters (stored in DSH). */
} QEF_PROC_INSERT;

/*
** Name:    QEF_INVOKE_RULE
**
** Description:
**	This structure contains the information required to execute an
**	INVOKE_RULE_ACTION
**
** History:
**	28-apr-93 (jhahn)
**	    Written to support statement level rules processing.
*/
typedef struct _QEF_INVOKE_RULE
{
    QEF_AHD	    *ahd_rule_action_list;
} QEF_INVOKE_RULE;

/*
** Name:    QEF_CLOSETEMP
**
** Description:
**	This structure contains the information required to close an
**	internal temptable. The structure contains a pointer to
**	the associated temptable.
**
** History:
**	15-oct-92 (jhahn)
**	    Written to support statement level rules processing.
*/
typedef struct _QEF_CLOSETEMP
{
    QEN_TEMP_TABLE	*ahd_temptable_to_close;
} QEF_CLOSETEMP;

/*}
** Name:    QEF_IPROC
**
** Description:
**	This structure contains the information required to execute an
**	internal (QEF HARDWIRED) procedure.
**
** History:
**	7-apr-89 (teg)
**	    Written to support QEF Internal Procedures.
**
*/
typedef struct _QEF_IPROC
{
    /* Alias structure for evoking QEF Internal Procedrue.
    */
    DB_DBP_NAME	    ahd_procnam;	/* name of procedure */
    DB_OWN_NAME	    ahd_ownname;	/* procedure owner */
					/* parameter list for this call */
} QEF_IPROC;

/*}
** Name: QEF_CREATE_SCHEMA - Contains the definition for the action 
**				QEA_CREATE_SCHEMA.
**
** Description:
**	This structure holds the schema name and schema owner. OPC
**	will copy over the schema name and owner from PST_CREATE_SCHEMA 
**	structure to QEF_CREATE_SCHEMA structure.
**
** History:
**	27-oct-92 (anitap)
**	    Created for CREATE SCHEMA project.
*/
typedef struct _QEF_CREATE_SCHEMA
{
      DB_SCHEMA_NAME	qef_schname;	/* schema name */
      DB_OWN_NAME	qef_schowner;	/* schema owner */
} QEF_CREATE_SCHEMA;

/*}
** Name: QEF_EXEC_IMM - Contains the definition for the action QEA_EXEC_IMM.
**
** Description:
**      This structure holds the pointer to query text which needs to be
**      parsed. For all the statements in CREATE SCHEMA, the pointer to query 
**	text is passed to OPF by the parser. QEF will call SCF which will in 
**	turn call PSF to parse the query text. This is to take care of 
**	backward/forward references. After QEF creates the tables, PSF
**      will be called to parse the statements referring to the tables just
**      created. By default all CREATE VIEW and GRANT statements will be 
**	compiled into QEA_EXEC_IMM actions regardless of any forward/backward 
**      referencing.
**
** History:
**      27-oct-2 (anitap)
**          Created for CREATE SCHEMA project.
*/
struct _QEF_EXEC_IMM
{
   DB_TEXT_STRING   *ahd_text;       /* pointer to query text which needs
                                     ** to be parsed
                                     */
   PST_INFO     *ahd_info;           /* pst_info pointer copied to 
				     ** ahd_info
				     */
};

/*}
** Name: QEF_EVENT - RAISE/REGISTER/REMOVE EVENT action data.
**
** Description:
**	This structure defines the information used for all event processing
**	statements.
**
**	The 'flags' and 'value' fields are only used by the RAISE statement
**	and should be cleared otherwise.
**
** History:
**	10-dec-90 (neil)
**	    Written for Terminator II/alerters.
*/
typedef struct _QEF_EVENT
{
    DB_ALERT_NAME	ahd_event;	/* Installation-unique event name */
    i4		ahd_evflags;	/* Modifiers per statement */
# define  QEF_EVNOSHARE		0x01	/* RAISE with NOSHARE indicates to SCF
					** to report only to current session
					** if it's registered.
					*/
    QEN_ADF		*ahd_evvalue;	/* Optional text for RAISE statement */
} QEF_EVENT;


/*}
** Name: QEQ_D1_QRY - Action defining an n-segment remote query 
**
** Description:
**      This action structure contains: 1) a query mode for distributed
**  transaction management, 2) a (multiple-segment) query text and 3) the 
**  descriptor of the intended LDB.  Each subquery that peforms a 
**  retrieval must be immediately followed by a QEQ_D2_GET action for
**  the query plan to be correct.
**
** History:
**      09-aug-88 (carl)
**          written
**      06-mar-96 (nanpr01)
**          Added qeq_q14_page_size to pass the pagesize of the temporary 
**	    table.
**
*/

typedef struct _QEQ_D1_QRY
{
    DB_LANG	     qeq_q1_lang;	    /* DB_SQL or DB_QUEL, part of
					    ** LDB protocol */
    i4		     qeq_q2_quantum;	    /* timeout quantum for subquery,
					    ** 0 if none */
    i2    	     qeq_q3_read_b;	    /* TRUE if subquery is a retrieval,
					    ** FALSE otherwise */
    i2		     qeq_q3_ctl_info;	    /* for embedding control info
					    */
#define QEQ_001_READ_TUPLES	    0x0001L 
					    /* ON if subquery performs a 
					    ** SELECT, to be followed by 
					    ** a GET action; OFF otherwise */
#define QEQ_002_USER_UPDATE	    0x0002L	    
					    /* ON if subquery performs a 
					    ** target-site update; OFF 
					    ** otherwise */ 
#define QEQ_003_ROW_COUNT_FROM_LDB  0x0004L	    
					    /* ON if subquery returns a row
					    ** count to be returned to the 
					    ** user; OFF otherwise */ 
    QEQ_TXT_SEG	    *qeq_q4_seg_p;	    /* ptr to first subquery text 
					    ** segment, NULL if none */
    DD_LDB_DESC	    *qeq_q5_ldb_p;	    /* ptr to a DD_LDB_DESC, LDB
					    ** for executing subquery */
    i4		     qeq_q6_col_cnt;	    /* number of columns in temporary
					    ** table, 0 if not applicable */
    DD_ATT_NAME	    **qeq_q7_col_pp;	    /* ptr to first of any array of
					    ** ptrs to column names */
    i4		     qeq_q8_dv_cnt;	    /* number of data values accompaning
					    ** subquery, 0 if none */
    DB_DATA_VALUE   *qeq_q9_dv_p;	    /* descriptors for intermediate 
					    ** data values to be returned by 
					    ** the LDB */
    i4		    qeq_q10_agg_size;	    /* expected size of subquery result 
					    ** from LDB */
    i4              qeq_q11_dv_offset;	    /* offset into data value array
					    ** in DSH for subsequent subqueries 
					    ** for storing returned data */
    bool	    qeq_q12_qid_first;	    /* TRUE if QID is at the head
					    ** of the subquery's parameter 
					    ** list, FALSE if at the end */
    PTR		    qeq_q13_csr_handle;	    /* handle to cursor's dsh, used
					    ** by QEA_D9_UPD; 0 if not used */
    i4         qeq_q14_page_size;      /* page size of the temporary
                                            ** table                        */
}   QEQ_D1_QRY;


/*}
** Name: QEQ_D2_GET - Action to read pending data from an LDB which has
**		      successfully executed a SELECT/RETRIEVE subquery.
**
** Description:
**      This action structure identifies the LDB with pending data for
**  Titan's consumption.  It is used to provide an anchor for QEF to
**  read data for SCF after executing a retrieval subquery.
**
**
** History:
**      09-sep-88 (carl)
**          written
**	31-aug-01 (inkdo01)
**	    Added qeq_g1_firstn to support "first n" syntax in Star.
**      13-feb-08 (rapma01)
**          SIR 119650
**          Removed qeq_g1_firstn since first n is not currently
**          supported in star
**
*/

typedef struct _QEQ_D2_GET
{
    DD_LDB_DESC	    *qeq_g1_ldb_p;	/* ptr to a DD_LDB_DESC, descriptor of
					** LDB with pending data */
}   QEQ_D2_GET;


/*}
** Name: QEQ_D3_XFR - Action information prescribing a data transfer 
**		      from one LDB to another.
**
** Description:
**      This action structure contains information for a source, a CREATE 
**  temporary table and a destination subquery for transferring data.
**
** History:
**      09-aug-88 (carl)
**          written
**
*/

typedef struct _QEQ_D3_XFR
{
    QEQ_D1_QRY	    qeq_x1_srce;	    /* source query (a SELECT/RETRIEVE
					    ** statement) and LDB, to be first
					    ** executed */
    QEQ_D1_QRY	    qeq_x2_temp;	    /* CREATE TABLE query for the
					    ** sink LDB, to be executed next 
					    ** when the column types become
					    ** available from the tuple
					    ** descriptor returned as part of
					    ** the result from above query */
    QEQ_D1_QRY	    qeq_x3_sink;	    /* sink query (a COPY FROM file
					    ** statement) and LDB, to be
					    ** executed after above temporary  
					    ** table has been created */
}   QEQ_D3_XFR;


/*}
** Name: QEQ_D4_LNK - Action creating a link to an LDB table created in same
**		      DDB query plan
**
** Description:
**      This action structure provides complete information for QEF to 
**  perform a CREATE LINK action on an LDB table created by a "CREATE 
**  TABLE as SUBSELECT" statement.  It should be the last action following
**  others creating  the table with an LDB and filling it with data.
**
** History:
**      09-sep-88 (carl)
**          written
**
*/

typedef struct _QEQ_D4_LNK
{
    QED_DDL_INFO    *qeq_l1_lnk_p;	/* ptr to CREATE LINK information */
}   QEQ_D4_LNK;

/*}
** Name: QEQ_D10_REGPROC - Action to execute a register procedure.
**
** Description:
**      This action structure provides complete information for QEF to 
**  execute a registered procedure in a specific LDB.  If this action is
**  in a query plan, it must be the only action in the plan.
**
** History:
**      14-dec-92 (teresa)
**          Created for REGISTERED PROCEDURES.
**
*/
typedef struct _QEQ_D10_REGPROC
{
    DB_LANG	    qeq_r1_lang;	    /* DB_SQL or DB_QUEL, part of
					    ** LDB protocol */
    i4		    qeq_r2_quantum;	    /* timeout quantum for subquery,
					    ** 0 if none */
    DD_REGPROC_DESC *qeq_r3_regproc;	    /* details of the regproc */
    PTR		    qeq_r4_csr_handle;	    /* handle to cursor's dsh, used
					    ** by QEA_D9_UPD; 0 if not used */
} QEQ_D10_REGPROC;

/*}
** Name: QEF_RELATED_OBJECT - Describe a related object.
**
** Description:
**
**	The current action may be interested in the IDs of other objects
**	created in this linked list of actions.  For instance, a CONSTRAINT
**	may need to know the ID of the table it applies to;  that table
**	may just have been created in the preceding action.
**
** History:
**	17-nov-92 (rickh)
**	    Creation.
*/
struct _QEF_RELATED_OBJECT
{
    QEF_RELATED_OBJECT	*qdo_nextRelatedObject;

    i4				qdo_ID;	/* row number of DSH row
				** containing the related object's ID */

    i4				qdo_type;	/* type of related object */

};

/*}
** Name: QEF_AHD - Describe an action
**
** Description:
**      Actions read tuples for QEPs, call DMF, update tuples, provide
**	projections for aggregate processing, etc. They are generally
**	large units of work within a query plan.
**
**	Processing occurs by reading a tuple from a QEP (if one exists) and
**	then calling ADE with values in the QEN_ADF structure to create the 
**	result tuple. The result tuple is then dispatched to DMF or the user.
**
**  NOTES:
**	PROJECTIONS of aggregate function result tables.
**	The processing of aggregate functions that use the standard QUEL
**	semantics (all by values are represented), requires the projection
**	of the "by" values.  These tuples are then joined to the
**	aggregate results on the by-lists so that all by-list values are
**	represented in the aggregate result relation. The QEP for the projection
**	action only needs to retrieve the tuples in the projection, the 
**	ACTION itself will append these tuples to a temporary sort table.
**	The QEP doesn't need to sort the tuples.
**
**	The row buffer used for putting tuples into the projection temp file
**	must be the same as the buffer used for retrieving tuples from
**	the projection temp file. Moreover, the CB used for reading and
**	writing the projection table must be the same. This means that 
**	the ahd_proj field in a AGGF action should be the same as the
**	ahd_odmr_cb field in the corresponding PROJ action.
**
**	Note that until change 448896, the projection technique was also
**	used for SQL function aggregates, even though SQL guarantees one
**	set of grouping columns/expressions for the whole query (making 
**	the projection merge phase unnecessary). Projection is now only
**	done if differing sets of bycols exist amongst the function aggs.
**
**	CX for AGGREGATE_FUNCTIONS
**	ahd_current:
**	    SINIT   put the "by" values to the output row.
**		    initialize the the aggregate result values.
**		    The first tuple must be fetched for SINIT to work.
**	    SMAIN   compare the "by" value from the tuple just retrieve with
**		    the one in the output row.  If the values are equal,
**		    run remaining CX to accumulate the aggregate result. BYCOMPARE
**		    only done for QEA_AGGF, QEA_RAGGF. Rest of aggregation
**		    code is run for all aggregates.
**
**	    SFINIT  finalize the aggregate result value and copy it to
**		    the output row buffer.
**
**	ahd_by_comp:
**	    SINIT   compare the "by" value from the projection tuple to the
**		    value in the tuple last fetched from the qualification
**		    QEP (QEA_AGGF only).
**	    SMAIN   project the result tuple which include the aggregate
**		    result and the "by" values to the output row buffer. 
**		    This output row is different from the output row of
**		    ahd_current (QEA_AGGF). 
**		    Or, project by expressions and compute hash number from 
**		    their concatenated values (QEA_HAGGF).
**  
** History:
**     18-mar-86 (daved)
**          written
**	28-jul-87 (daved & puree)
**	    change ahd_alt to be a pointer to a valid list entry. The basic
**	    usage of this field is unchanged with the exception that if the
**	    command is a create, each element in the valid list entry list
**	    on the same vl_alt chain will be updated.
**	14-sep-87 (rogerk)
**	    Added QEA_LOAD ahd action type.
**	02-may-88 (puree)
**	    Modified for DB procedure project. Add new actions:
**		QEA_COMMIT
**		QEA_ROLLBACK
**		QEA_IF
**		QEA_RETURN
**		QEA_MESSAGE
**	    Also made qhd_qep, qhd_dmu, qhd_rup seperated structures named
**	    QEF_QEP, QEF_DMU, QEF_RUP, repectively.
**	20-aug-88 (carl)
**          Added Titan changes (indicated by the symbol +Titan)
**	15-jan-89 (paul)
**	    Enhanced for rules processing. Add action QEA_CALLPROC.
**	18-apr-89 (paul)
**	    Convert update cursor action to a standard QEP style action
**	    structure to support rules processing on cursors. Also add
**	    support for a delete cursor action.
**	10-dec-90 (neil)
**	    Alerters: New actions and union field.
**	21-jul-90 (carl)
**	    Added #define QEA_D0_NIL 100 as the base for all STAR action types
**	17-aug-92 (rickh)
**	    Added ddl statement types for FIPS CONSTRAINTS.
**	22-oct-92 (jhahn)
**	    Added dml statement types for FIPS CONSTRAINTS.
**	13-nov-92 (anitap)
**          Added the action header types for PST_CREATE_SCHEMA & PST_EXEC_IMM
**          statement types.
**	07-dec-92 (teresa)
**	    add support for QEA_D10_REGPROC
**	23-dec-92 (andre)
**	    defined QEA_CREATE_VIEW and added qhd_create_view
**	12-Jan-93 (jhahn)
**	    Added QEF_STATEMENT_UNIQUE and QEF_NEEDS_BEF_TID for FIPS
**	    constraints.
**	17-may-93 (jhahn)
**	    Added qhd_invoke_rule and removed QEF_NEEDS_BEF_TID for FIPS
**	    constraints.
**	10-sep-93 (anitap)
**	    Corrected comment for QEA_CREATE_SCHEMA.	
**	16-sep-93  (robf)
**          Add ahd_audit for security auditing (moved from QP)
**	    Add QEA_UPD_ROW_SECLABEL flag
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	7-nov-95 (inkdo01)
**	    Replaced many instances of QEN_ADF and several of QEN_TEMP_TABLE with
**	    pointers to those structures. Moved qhdobj union to end of QEF_AHD.
**	22-oct-98 (inkdo01)
**	    Added QEA_FOR, QEA_RETROW for for-loop in row producing procedures 
**	    feature.
**	19-jan-01 (inkdo01)
**	    Added QEA_HAGGF to compute function aggregates using hash grouping.
**	19-mar-04 (inkdo01)
**	    Added QEA_NODEACT flag to help || query cleanup.
**	29-may-06 (dougi)
**	    Added ahd_stmtno to log procedure statement numbers for improved
**	    diagnostics.
**	19-Apr-2007 (kschendel) b122118
**	    Remove ahd-keys, nothing used it.
8*	18-Mar-2010 (gupsh01) SIR 123444 
**	    Added support for Alter table rename table/column. 
*/
struct _QEF_AHD
{
    /* standard stuff */
    QEF_AHD     *ahd_next;       /* The next control block */
    QEF_AHD     *ahd_prev;       /* The previous one */
    SIZE_TYPE   ahd_length;      /* size of the control block */
    i2          ahd_type;        /* type of control block */
#define QEAHD_CB    1
    i2          ahd_s_reserved;
    PTR          ahd_l_reserved;
    PTR         ahd_owner;
    i4          ahd_ascii_id;    /* to view in a dump */
#define QEAHD_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'H', 'D')
    enum ahd_atype {			/* action */
	QEA_DMU =		1,	/* perform a DMF operation */
	QEA_APP =		2,	/* append operation */
	QEA_SAGG =		3,	/* perform a simple aggregate */
	QEA_PROJ =		4,	/* project an aggregate function */
	QEA_AGGF =		5,	/* compute an aggregate function */
	QEA_UPD =		6,	/* update operation */
	QEA_DEL =		7,	/* delete operation */
	QEA_GET =		8,	/* fetch operation */
	QEA_UTL =		9,	/* utility operation */
	QEA_RUP =		10,	/* update of tuple in associated QP 
					** This action is used by update
					** cursor commands where one
					** QP updates the current tuple
					** in another cursor QP
					*/
	QEA_RAGGF =		11,	/* return the aggregate result row
					** in a row instead of putting it
					** into the result relation.
					** There is no result relation for
					** aggs computed with this action.
					** This action is only available
					** from a QEN_QP node and will
					** only produce no-project (SQL)
					** aggregates.
					*/
	QEA_LOAD =		12,	/* load operation */
	QEA_PUPD =		13,	/* Positioned update */
	QEA_PDEL =		14,	/* Positioned delete */
	QEA_COMMIT =		15,	/* Commit for DB procedure */
	QEA_ROLLBACK =		16,	/* Rollback for DB procedure */
	QEA_IF =		17,	/* If stmt for DB procedure */
	QEA_RETURN =		18,	/* return stmt for DB procedure */
	QEA_MESSAGE =		19,	/* message stmt for DB procedure */
	QEA_CALLPROC =		20,	/* Invoke a DB procedure */
	QEA_EMESSAGE =		21,	/* Generate an error message */
	QEA_IPROC =		22,	/* perform QEF Internal Procedure */
	QEA_RDEL =		23,	/* Cursor delete action */
	QEA_EVREGISTER =	24,	/* REGISTER EVENT - data is QEF_EVENT */
	QEA_EVDEREG =		25,	/* REMOVE EVENT - data is QEF_EVENT */
	QEA_EVRAISE =		26,	/* RAISE EVENT - data is QEF_EVENT */
	QEA_CREATE_RULE =	27,	/* takes QEF_DDL_STATEMENT */
	QEA_CREATE_PROCEDURE =	28,	/* takes QEF_DDL_STATEMENT */
	QEA_CREATE_INTEGRITY =	29,	/* takes QEF_CREATE_INTEGRITY_STATEMENT;*/
	QEA_CREATE_TABLE =	30,	/* takes QEF_DDL_STATEMENT */
	QEA_DROP_INTEGRITY =	31,	/* takes QEF_DROP_INTEGRITY */
	QEA_MODIFY_TABLE =	32,	/* takes QEF_DDL_STATEMENT */
	QEA_PROC_INSERT =	33,	/* Insert for deferred proc call  */
	QEA_CLOSETEMP =		34,	/* Close temptable */
	QEA_INVOKE_RULE =	35,	/* Invoke a rule action list */

	QEA_CREATE_SCHEMA =	36,	/* takes QEF_CREATE_SCHEMA */
	QEA_EXEC_IMM =		37,	/* takes QEF_EXEC_IMM */
	QEA_CREATE_VIEW =	38,	/* takes QEF_CREATE_VIEW */

	QEA_FOR =		39,	/* For stmt for DB procedure */
	QEA_RETROW =		40,	/* Return row for DB procedure */
	QEA_HAGGF =		41,	/* Same as QEA_RAGGF, except it 
					** assumes no input ordering and 
					** groups input rows by hashing */
	QEA_RENAME_STATEMENT =  42,	/* takes QEF_RENAME_STATEMENT */

	QEA_D0_NIL =		100,	/* base for all STAR action types, NOT
					** a real action type */
	QEA_D1_QRY =		101,	/* (+Titan) a subquery action to go with
                                        ** QEQ_D1_QRY */
	QEA_D2_GET =		102,	/* (+Titan) a get action, it always
					** follows a retrieval subquery to
					** allow QEF to peform data fetching 
                                        ** to go with QEQ_D2_GET */
	QEA_D3_XFR =		103,	/* (+Titan) a data transfer action 
					** consisting of 3 sequential 
					** subqueries: SELECT/RETRIEVE,
					** CREATE TABLE and COPY FROM STAR 
                                        ** to go with QEQ_D3_XFR */
	QEA_D4_LNK =		104,	/* (+Titan) a CREATE LINK action to
					** finish off a CREATE TABLE statement 
					** uses QEQ_D4_LNK */
	QEA_D5_DEF =		105,	/* (+Titan) an action defining an LDB
					** repeat query, to go with QEQ_D1_QRY 
					** (NOT used) */
	QEA_D6_AGG =		106,	/* (+Titan) an action defining a 
					** parameterized subquery to go with 
                                        ** QEQ_D1_QRY, the intermediate result
                                        ** returned by the LDB will be placed
                                        ** in allocated space in the data 
					** segment for subsequent subqueries */
	QEA_D7_OPN =		107,	/* (+Titan) an action defining an OPEN
					** CURSOR subquery, uses QEQ_D1_QRY */
	QEA_D8_CRE =		108,	/* (+Titan) an action defining a CREATE
					** AS SUBSELECT subquery, uses 
					** QEQ_D1_QRY */
	QEA_D9_UPD =		109,	/* (+Titan) an action defining a CURSOR
					** UPDATE subquery, uses QEQ_D1_QRY */
	QEA_D10_REGPROC =	110,	/* perform Registered Procedure,
					** uses QEQ_D10_REGPROC action */
    }		ahd_atype;
    /* initialization stuff */
    QEN_ADF	*ahd_mkey;		/* make keys - only use
					** the ADE_SMAIN segment to 
					** produce the constant keys.
					*/
    QEF_VALID	*ahd_valid;		/* list of relations to open 
					** and validate. 
					*/
    i4		ahd_flags;
#define QEA_NEWSTMT	0x01		/* indicating new DMF statement */
#define	QEA_NODEACT	0x02		/* Action header sits atop a node tree
					** (flag is a short cut for 
					** || query cleanup) */
#define QEA_DEF		0x08		/* all cursors are deferred */
#define QEA_DIR		0x10		/* all cursors are direct */

    /* Invalidate this query plan if the QEN_NODE tree for this action
    ** returns zero rows. When the QP invalidation is done, the
    ** QEF_ZEROTUP_CHK bit in the qef_intstate of the qef_rcb should
    ** be set.
    */
#define	QEA_ZEROTUP_CHK	0x20
#define QEF_STATEMENT_UNIQUE	0x40	/* Does this have a statement level
					** unique constraint
					*/
#define  QEA_NEED_QRY_SYSCAT	0x2000  /* QUERY_SYSCAT privilege required */
#define  QEA_XTABLE_UPDATE	0x4000	/* cross-table update */
    i4	ahd_ID;	/* DSH row where this object's ID will be stuffed so it can
		** later be shoved into an iidbdepends tuple as necessary.  
		** this is currently compiled in for the rules and procedures 
		** generated by CONSTRAINTS.  */
#define	QEA_UNDEFINED_OBJECT_ID	-1 

    QEF_RELATED_OBJECT	*ahd_relatedObjects;
		/* pointer to a linked list of ids of other objects
		** that are interesting to this action.
		*/
    PTR			ahd_audit;	/* Audit information */
    i4			ahd_stmtno;	/* "statement number" if action hdr 
					** generated by dbproc */
    /* inkdo01 - copied qhd_obj from after ahd_atype to here to */
    /* permit minimal allocation of QEF_AHD - 1/11/95 */
    union 
    {
	QEF_QEP	    qhd_qep;		/* action with QEP */
        QEF_DMU	    qhd_dmu;		/* DMF action */
        QEF_RUP	    qhd_rup;		/* update cursor action */ /* used ? */
	QEF_IF	    qhd_if;		/* If statement */
	QEF_RETURN  qhd_return;		/* Return statment */
	QEF_MESSAGE qhd_message;	/* Message statment */
	QEF_CALLPROC qhd_callproc;	/* CALLPROC statement */
	QEF_PROC_INSERT qhd_proc_insert;/* PROC_INSERT statement */
	QEF_CLOSETEMP qhd_closetemp;	/* Close temptable statement */
	QEF_INVOKE_RULE qhd_invoke_rule;/* Invoke rule action list */
	QEF_IPROC   qhd_iproc;		/* Internal QEF Procedure Action */
	QEF_EVENT   qhd_event;		/* RAISE, REGISTER, REMOVE EVENT data */
	QEF_DDL_STATEMENT	qhd_DDLstatement;
	QEF_CREATE_SCHEMA	qhd_schema;	/* takes QEF_CREATE_SCHEMA */
	QEF_EXEC_IMM            qhd_execImm;    /* EXECUTE IMMEDIATE  statement
                                                */
	QEF_CREATE_INTEGRITY_STATEMENT	qhd_createIntegrity;
	QEF_CREATE_VIEW			qhd_create_view;
	QEF_DROP_INTEGRITY		qhd_drop_integrity;
	QEF_RENAME_STATEMENT		qhd_rename;

	QEQ_D1_QRY  qhd_d1_qry;		/* (+Titan) a subquery */
	QEQ_D2_GET  qhd_d2_get;		/* (+Titan) a get action */
	QEQ_D3_XFR  qhd_d3_xfr;		/* (+Titan) a data transfer */
	QEQ_D4_LNK  qhd_d4_lnk;		/* (+Titan) a CREATE LINK action */
	QEQ_D10_REGPROC qed_regproc;	/* execute a registed procedure */
    } qhd_obj;
};
