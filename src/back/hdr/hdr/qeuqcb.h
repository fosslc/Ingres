/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEUQCB.H - control blocks for high level QEU query functions
**
** Description:
**      Certain high level, predetermined, queries are performed for other
**	facilities, notably RDF. The structures in this file are required
**	to effectively produce the results of the queries,
**
** History:
**      17-jun-86 (daved)
**          written
**      19-jul-86 (jennifer)
**          Fixed names of structure where typed wrong. 
**          For example queq_ano changed to qeuq_ano, etc.
**          Added two new field for passing the query id when a
**          view query test is retrieved.  Also added a 
**          field to allow the special permits, all to all, etc.
**          to be set.
**	28-apr-88 (puree)
**	    Modify QEUQ_CB for DB procedure.
**	07-nov-88 (carl)
**	    Added QEUQ_DDB_CB for Titan.
**	    Mofified QEUQ_CB for Titan.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added qeuq_cagid, qeuq_agid_tup, qeuq_agid_mask;
**	    Defined flags for processing iiapplication_id, iiusergroup.
**	06-mar-89 (neil)
**          Rules: Modified QEUQ_CB for rules.
**          Deleted the definition of QEUQ_BLK which is unused.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added qeuq_cdbpriv, qeuq_dbpr_tup, qeuq_dbpr_mask;
**	    Added definitions for QEU_GDBPRIV, QEU_RDBPRIV masks.
**      25-jun-89 (jennifer)
**          Added defintions for qeuq_cuser, qeuq_cloc, and qeuq_cdbacc.
**          and other security related routines.
**	07-sep-89 (ralph)
**	    Removed QEU_USTATUS and QEU_UGROUP.
**	    Added QEU_DEFAULT_GROUP for CREATE/ALTER GROUP.
**	24-oct-89 (neil)
**	    Alerters: Modified existing fields in QEUQ_CB for events.
**	    would this be non-zero; but they are non-zero for gateway tables).
**	16-jan-90 (ralph)
**	    Add comment to explain new uses for qeuq_permit_mask.
**	14-feb-90 (carol)
**	    Add comment to explain new uses for qeuq_uld_tup and qeuq_culd
**	    for comments support.
**	21-jun-90 (linda)
**	    Added qeuq_status_mask field, taken from dmt_tbl_entry's
**	    tbl_status_mask field, so that we know what kind of object
**	    we have.  In particular, this is used when removing a gateway
**	    table which has a view defined; qeu_dview() was using high_time
**	    and low_time to identify a view (assuming that only for views
**	    would this be non-zero; but they are non-zero for gateway tables).
**	12-jul-93 (robf)
**	     Added qeuq_sec_mask for ENABLE/DISABLE SECURITY_ALARM
**	22-jul-92 (andre)
**	    defined QEU_DROP_ALL over qeuq_flag_mask.
**	23-jul-92 (andre)
**	    defined QEU_DROP_CASCADE over qeuq_flag_mask
**	23-jul-92 (andre)
**	    defined masks over qeuq_permit_flag to indicate whether the
**	    opefration pertains to permits, security_alarms, or both + moved
**	    QEU_DROP_ALL from qeuq_flag_mask to qeuq_permit_flag + defined
**	    QEU_SKIP_ABANDONED_OBJ_CHECK over qeuq_permit_flag
**	24-jul-92 (andre)
**	    defined QEU_REVOKE_PRIVILEGES over qeuq_permit_mask
**	02-aug-92 (andre)
**	    defined QEU_REVOKE_ALL over qeuq_permit_mask
**     13-sep-93 (smc)
**          Made qeuq_d_id a CS_SID.
**	29-sep-93 (stephenb)
**	    Added qeuq_tabname field to QEUQ_CB.
**      22-jul-1996 (ramra01)
**          Alter table project: added qeuq_aid - attribute internal id
**          Added QEU_DROP_COLUMN - Destruction of object limited to
**          association with a specific table column where the column attr
**          number passed in qeuq_ano.
**      22-dec-1996 (nanpr01)
**	    Bug : 79853 qeuq_aid should be defined as u_i2 rather than a nat.
**      26-mar-1997 (nanpr01)
**	    Bug : 81224 Referential integrity fix.
**	  28-aug-2002 (gygro01) ingsrv1859
**		b108474 (b108635) defined QEU_DEP_CONS_DROPPED over
**		qeuq_flag_mask. Will be set during QEU_DROP_COLUMN 
**		processing to check if dependent referential constraint 
**		has been dropped previously.
**      12-Apr-2004 (stial01)
**          Define qeuq_length as SIZE_TYPE.
[@history_line@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _QEUQ_DDB_CB	    QEUQ_DDB_CB;
typedef	struct _QEUQ_CB		    QEUQ_CB;

/*}
** Name: QEUQ_DDB_CB - Control block containing additional information for Titan
**
** Description:
**      This control block contains additional information required by Titan.
**
** History:
**	04-nov-88 (carl)
**          written
*/

struct _QEUQ_DDB_CB
{
    DB_LANG	qeu_1_lang;			/* language of view definition
						*/
    bool	qeu_2_view_chk_b;		/* TRUE if check option 
						** specified in CREATE VIEW
						** statement, FALSE otherwise */
    i4		qeu_3_row_width;		/* size of row */
};

/*}
** Name: QEUQ_CB - main control block for QEU query functions
**
** Description:
**      This control block is required for all high level query functions.
**
** History:
**	17-jun-86 (daved)
**          written
**      19-jul-86 (jennifer)
**          Fixed names of structure where typed wrong. 
**          For example queq_ano changed to qeuq_ano, etc.
**          Added two new field for passing the query id when a
**          view query test is retrieved.  Also added a 
**          field to allow the special permits, all to all, etc.
**          to be set.
**	28-apr-88 (puree)
**	    Add qeuq_dbp_tup for DB procedure.  Also use qeuq_qid to return
**	    the procedure id back to the caller.
**	16-aug-88 (puree)
**	    Add define for QEU_DBP_PROTECTION for qeu_flag_mask.
**	04-nov-88 (carl)
**	    Added qeuq_ddb_cb for Titan.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added qeuq_cagid, qeuq_agid_tup, qeuq_agid_mask;
**	    Defined flags for processing iiapplication_id, iiusergroup.
**	06-mar-89 (neil)
**          Rules: Added qeuq_cr, qeuq_rul_tup and QEU_RULE.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added qeuq_cdbpriv, qeuq_dbpr_tup, qeuq_dbpr_mask;
**	    Added definitions for QEU_GDBPRIV, QEU_RDBPRIV masks.
**      25-jun-89 (jennifer)
**          Added defintions for qeuq_cuser, qeuq_cloc, and qeuq_cdbacc
**          and other security related routines.
**	07-sep-89 (ralph)
**	    Removed QEU_USTATUS and QEU_UGROUP.
**	    Added QEU_DEFAULT_GROUP for CREATE/ALTER GROUP.
**	24-oct-89 (neil)
**	    Alerters: Commented the "uld" fields to work for events too and
**	    added flag QEU_EV_PROTECTION).  If you do not require a new
**	    entry in the structure (your data is mutually exclusive in 
**	    relation to the other tuples) then reuse the field qeuq_uld_tup.
**	16-jan-90 (ralph)
**	    Add comment to explain new uses for qeuq_permit_mask.
**	05-mar-90 (andre)
**	    define QEU_DBA_QUEL_VIEW and QEU_VGRANT_OK.
**	21-jun-90 (linda)
**	    Added qeuq_status_mask field, taken from dmt_tbl_entry's
**	    tbl_status_mask field, so that we know what kind of object
**	    we have.  In particular, this is used when removing a gateway
**	    table which has a view defined; qeu_dview() was using high_time
**	    and low_time to identify a view (assuming that only for views
**	    would this be non-zero; but they are non-zero for gateway tables).
**	09-nov-90 (andre)
**	    defined QEU_DROP_IDX_SYN
**	09-jul-91 (andre)
**	    defined QEU_SPLIT_PERM over qeuq_flag_mask
**	22-jul-92 (andre)
**	    defined QEU_DROP_ALL over qeuq_flag_mask.
**	23-jul-92 (andre)
**	    defined QEU_DROP_CASCADE over qeuq_flag_mask
**	23-jul-92 (andre)
**	    defined masks over qeuq_permit_flag to indicate whether the
**	    opefration pertains to permits, security_alarms, or both + moved
**	    QEU_DROP_ALL from qeuq_flag_mask to qeuq_permit_flag + defined
**	    QEU_SKIP_ABANDONED_OBJ_CHECK over qeuq_permit_flag
**	24-jul-92 (andre)
**	    defined QEU_REVOKE_PRIVILEGES over qeuq_permit_mask
**	31-jul-92 (andre)
**	    replaced QEU_DBA_QUEL_VIEW with QEU_DBA_VIEW and QEU_QUEL_VIEW
**	02-aug-92 (andre)
**	    defined QEU_REVOKE_ALL over qeuq_permit_mask
**	13-aug-92 (andre)
**	    added qeuq_v2b_col_xlate and qeuq_b2v_col_xlate
**	08-sep-92 (andre)
**	    undefined QEU_DBA_QUEL_VIEW and defined QEU_DROP_TEMP_TABLE over
**	    qeuq_flag_mask
**	22-feb-93 (andre)
**	    defined QEU_DROPPING_SCHEMA
**	12-jul-93 (robf)
**	     Added qeuq_sec_mask for ENABLE/DISABLE SECURITY_ALARM
**	10-sep-93 (andre)
**	    defined QEU_FORCE_QP_INVALIDATION
**	29-sep-93 (stephenb)
**	    Added qeuq_tabname, we need the tablename to write security audit
**	    records in QEF.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	22-jul-1996 (ramra01)
**	    Alter table project: added qeuq_aid - attribute internal id
**	    Added QEU_DROP_COLUMN - Destruction of object limited to 
**	    association with a specific table column where the column attr
**	    number passed in qeuq_ano.
**      22-dec-1996 (nanpr01)
**	    Bug : 79853 qeuq_aid should be defined as u_i2 rather than a nat.
**	10-may-02 (inkdo01)
**	    Added QEU_SEQ_PROTECTION for sequence support.
**	  28-aug-2002 (gygro01) ingsrv1859
**		b108474 (b108635) defined QEU_DEP_CONS_DROPPED over
**		qeuq_flag_mask. Will be set during QEU_DROP_COLUMN 
**		processing to check if dependent referential constraint 
**		has been dropped previously.
[@history_line@]...
*/
struct _QEUQ_CB
{
    QEUQ_CB     *qeuq_next;     /* The next qeuq control block */
    QEUQ_CB     *qeuq_prev;     /* The previous one */
    SIZE_TYPE   qeuq_length;    /* size of the control block */
    i2          qeuq_type;      /* type of control block */
#define QEUQCB_CB    8
    i2          qeuq_s_reserved;
    PTR         qeuq_l_reserved;
    PTR         qeuq_owner;
    i4          qeuq_ascii_id;  /* to view in a dump */
#define QEUQCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'U', 'Q')
    i4		qeuq_pno;	/* protection number */
    i4		qeuq_ino;	/* integrity number */
    u_i2	qeuq_ano;	/* attribute ordinal number - attid */
    u_i2	qeuq_aid;	/* attribute internal id - attintl_id */
    i4		qeuq_ct;	/* number of tree tuples */
    i4		qeuq_ci;	/* number of integrity tuples */
    i4		qeuq_cp;	/* number of protection tuples */
    i4		qeuq_cq;	/* number of qrytext tuples */
    i4		qeuq_cagid;	/* number of aplid/group tuples */
    i4		qeuq_cr;	/* number of rule tuples */
    i4		qeuq_cdbpriv;	/* number of iidbpriv tuples */
    i4          qeuq_culd;      /* number of utility tuples: iiuser, iilocation
                                ** iidbaccess, iisecuritystate, iievent,
				** iidbms_comment, iilabelmap
				*/
    i4		qeuq_tsz;	/* size of table id array */
    DB_TAB_ID	*qeuq_rtbl;	/* result table id for permit,integ */
    DB_TAB_ID	*qeuq_tbl_id;	/* internal table id array */
    i4		*qeuq_tbl_type;	/* table type array (e.g., tproc) */
    DD_OBJ_TYPE	*qeuq_obj_typ_p;/* Titan: object type array to match above:
				** DD_1OBJ_LINK, DD_2OBJ_TABLE, or DD_3OBJ_VIEW
				*/
    QEF_DATA	*qeuq_tre_tup;	/* iitree tuple stream */
    QEF_DATA	*qeuq_int_tup;	/* iiintegrity tuple stream */
    QEF_DATA	*qeuq_pro_tup;	/* iiprotection tuple stream */
    QEF_DATA	*qeuq_qry_tup;	/* iiqrytext tuple stream */
    QEF_DATA	*qeuq_dbp_tup;	/* iiprocedure tuple stream */
    QEF_DATA	*qeuq_agid_tup;	/* iiapplication_id/iiusergroup tuple stream */
    QEF_DATA	*qeuq_rul_tup;	/* iirule tuple stream */
    QEF_DATA	*qeuq_dbpr_tup;	/* iidbpriv tuple stream */
    QEF_DATA	*qeuq_uld_tup;	/* utility tuple data stream includes: iiuser,
				** iilocations, iidbaccess, iisecuritystate,
				** iievent, DB_1_IICOMMENT, iilabelmap  */
    PTR	        qeuq_dmf_cb;	/* DMF control block for create view */
    PTR		qeuq_db_id;	/* a dmf database id */
    CS_SID	qeuq_d_id;      /* a dmf session id */
    i4	qeuq_flag_mask;	/* request modifier for qrymod operations */

#define	QEU_QRYTEXT	    0x00L   /* iiqrytext tuples are returned */
#define QEU_PROTECTION	    0x01L   /* iiprotection tuples are returned */
#define QEU_TREE	    0x02L   /* iitree tuples are returned */
#define QEU_INTG	    0x04L   /* iiintegrity tuples are returned */
#define QEU_DBA_VIEW	    0x08L   /* creating a view owned by the DBA */
#define QEU_QUEL_VIEW	    0x10L   /* creating QUEL view;
				    ** if creating a QUEL view owned by the DBA,
				    ** create an "access" permit once the view
				    ** has been successfully created
				    */
#define QEU_DROP_TEMP_TABLE 0x20L   /* dropping temporary table */
				    /*
				    ** It has been determined that user will be
				    ** allowed to grant a permit on his/her
				    ** view.
				    ** This bit will be used to communicate
				    ** this info to QEF to pass it on to DMF
				    */
#define QEU_VGRANT_OK	    0x40L
				    /*
				    ** drop synonyms defined for indices which
				    ** may have been created for this table
				    */
#define	QEU_DROP_IDX_SYN    0x80L

				    /* cascading destruction is taking place */ 
#define QEU_DROP_CASCADE    0x0100L

				    /* qef to make up a name */
#define QEU_CONSTRUCT_CONSTRAINT_NAME 	0x0200L

				    /*
				    ** This bit will be set if we are performing
				    ** cascading destruction of a schema; it is
				    ** intended to be used in qeu_d_cascade() to
				    ** ensure that we drop dependent dbprocs
				    ** which belong to the schema being
				    ** destroyed rather than trying to mark
				    ** them as abandoned
				    */
#define	QEU_DROPPING_SCHEMA 0x0400L

				    /*
				    ** when invoking a function to 
				    **  - drop objects (such as rules, permits,
				    **    integrities, etc.) associated with or
				    **	- drop views or 
				    **  - downgrade dbprocs 
				    ** dependent on an object (such as table, 
				    ** view, or dbproc) or privilege which was 
				    ** dropped, this bit will be used to 
				    ** indicate that the timestamp of some base
				    ** table on which the object to be  
				    ** dropped or downgraded (for dbprocs only)
				    ** depends must be altered in order to force
				    ** invalidation of QPs which depend on it 
				    */
#define	QEU_FORCE_QP_INVALIDATION	0x0800L

				/* destruction of objects limited to association
				** with a specific table column.  Column attr
				** number passed in qeuq_ano.
				*/
#define QEU_DROP_COLUMN     0x1000L

				/* b108474 (b108635)
				** During drop column cascade destruction
				** dependent object on referential constraint
				** can not be checked only via column attr
				** bitmap due to different cols on different
				** tables and ie. insert rule do not have col
				** bitmap set as they are not col specific.
				** This will initially only be used for 
				** system generated rules with referential
				** constraint dependencies.
				*/
#define QEU_DEP_CONS_DROPPED	0x2000L

    DB_QRY_ID    qeuq_qid;      /* query id of view or id of procedure */
    i4           qeuq_permit_mask;
                                /*
				** flags for creation/destruction of permits
				** and security_alarms
				*/
				
				/*
				** must be used when creating permits OR
				** security_alarms and when dropping permits AND
				** security_alarms as a part of destroying an
				** object on which they are defined
				*/
#define	QEU_PERM_OR_SECALM	0x01

				/*
				** must be used when destroying permits
				*/
#define	QEU_PERM		0x02

				/*
				** must be used when destroying security_alarms
				*/
#define QEU_SECALM		0x04

				/*
				** drop ALL permits and/or security_alarms
				*/
#define	QEU_DROP_ALL		0x08

				/*
				** creating/destroying dbproc privilege(s)
				*/
#define QEU_DBP_PROTECTION	0x10

				/*
				** creating/destroying dbevent privilege(s)
				*/
#define QEU_EV_PROTECTION	0x20

				/*
				** split the permit to ensure that each
				** tuple represents exactly one privilege
				*/
#define	QEU_SPLIT_PERM		0x40

				/*
				** skip checking whether destroying a permit
				** would result in some objects being abandoned;
				** this would happen if
				**   - we are deleting permits and
				**     security_alarms (for tables and views
				**     only) as a part of destroying (and, in 
				**     the case of dbprocs, marking a dbproc
				**     ungrantable or dormant) and qeu_dprot()
				**     need not be concerned about making
				**     objects abandoned by destroying specified
				**     permit(s)
				**   - we are deleting permits which represent a
				**     subset of privileges granted by newly
				**     created permit(s)
				*/
#define	QEU_SKIP_ABANDONED_OBJ_CHECK	0x80

				/*
				** processing REVOKE privilege as opposed to
				** DROP PERMIT; setting of QEU_DROP_CASCADE in
				** qeuq_flag_mask determines whether revocation
				** is to be cascaded or restricted
				*/
#define	QEU_REVOKE_PRIVILEGES		0x0100

				/*
				** processing REVOKE ALL [PRIVILEGES]; this
				** means that we will report an error to the
				** caller only if no privileges we granted by
				** the grantor to the specified grantee
				*/
#define	QEU_REVOKE_ALL			0x0200

				/*
				** creating/destroying sequence privilege(s)
				*/
#define QEU_SEQ_PROTECTION		0x0400

    u_i4	qeuq_agid_mask;	/* special flags for processing
				** iiapplication_id/iiusergroup requests
				*/
#define	QEU_CGROUP	0x0001	/* Append tuples to   iiusergroup */
#define	QEU_DGROUP	0x0002	/* Delete tuples from iiusergroup */
#define	QEU_PGROUP	0x0004	/* Drop all group members (purge) */
#define	QEU_DEFAULT_GROUP 0x0100 /* Adding default group for user */

#define	QEU_CAPLID	0x0010	/* Append tuples to   iiapplication_id */
#define	QEU_DAPLID	0x0020	/* Delete tuples from iiapplication_id */
#define	QEU_AAPLID	0x0040	/* Update tuples in   iiapplication_id */
#define QEU_GROLE	0x0100	/* Grant on role */
#define QEU_RROLE	0x0200	/* Revoke on role */
    i4		qeuq_dbpr_mask;	/* special flags for processing
				** iidbpriv, iirolegrant requests
				*/
#define	QEU_GDBPRIV	1	/* Database privileges are granted */
#define	QEU_RDBPRIV	2	/* Database privileges are revoked */
    i4		qeuq_sec_mask;	/* special flag for processing 
				   security state requests */
#define QEU_SEC_NORM    0x02    /* Normal auditing */
#define QEU_ESECSTATE	0x04	/* Enable security state */
#define QEU_DSECSTATE   0x08	/* Disable security state */
    i4	qeuq_status_mask;   /* from dmt_tbl_entry.tbl_status_mask, to
				    ** let us know what kind of object we
				    ** have (i.e., gateway table...)
				    */
    PTR		qeuq_indep;	/* descriptors of objects and privileges on
				** which new objects (e.g. views, dbprocs)
				** and permits depend
				*/

				/*
				** this function will translate a map of 
				** attributes of an updateable view into a
				** map of attributes of a specified underlying
				** table or view
				*/
    DB_STATUS		(*qeuq_v2b_col_xlate)();
				/*
				** this function will translate a map of a table
				** or view into a map of attributes of a
				** specified updateable view which is defined on
				** top of that table or view
				*/
    DB_STATUS           (*qeuq_b2v_col_xlate)();

    i4		qeuq_eflag;	/* type of error handling semantics: either
				** QEF_EXTERNAL or QEF_INTERNAL */
    QEUQ_DDB_CB	qeuq_ddb_cb;	/* additional information for Titan */
    DB_ERROR    error;		/* error return block */
    DB_TAB_NAME	qeuq_tabname;	/* table name for auditing */
    bool	qeuq_keydropped;/* whether key was dropped */
    u_i2	qeuq_keyattid;	/* key attribute ordinal number - attid */
    i2		qeuq_keypos;	/* dropped col key position */
};

/* bit masks used when calling qeu_access_perm() */
#define	    QEU_TO_PUBLIC   (i4) 0x01
/* [@type_definitions@] */
