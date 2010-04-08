/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEU.H - utility routine control blocks
**
** Description:
**      The utility routines provide support for those activities in
** QEF that cannot be described in a query plan. 
**
** History: $Log-for RCS$
**      27-may-86 (daved)
**          written
**	19-jan-87 (rogerk)
**	    Moved QEU_COPY and related structures to QEFCOPY.H
**      18-jul-89 (jennifer)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	7-jan-91 (seputis)
**	    added qeu_mask, qeu_timeout, qeu_maxlocks to support
**	    the OPF ACTIVE flag
**     13-sep-93 (smc)
**          Made qeu_d_id a CS_SID.
**	14-nov-95 (kch)
**	    Added current query mode, qeu_qmode to QEU_CB. This change fixes
**	    bug 72504.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-Jan-2004 (schka24)
**	    Add partition definition info.
**	11-Apr-2008 (kschendel)
**	    Bundle up qeu qualification function stuff.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _QEU_CB QEU_CB;

/*
**  Defines of other constants.
*/


/*}
** Name: QEU_LOGPART_CHAR - Logical Partition characteristics
**
** Description:
**	When defining a partitioned table (whether create or modify),
**	the partition distribution definition is in a DB_PART_DEF
**	structure.  For the physical creation of the partitions, though,
**	the DB_PART_DEF does not suffice, because there is no place to
**	put physical characteristics such as location, storage structure,
**	key columns, etc.  These physical things are all defined by
**	a WITH option clause within the partition definition.  As
**	such a WITH clause is parsed, QEU_LOGPART_CHAR entries are
**	created containing the WITH info for that partition or set of
**	partitions.  At execution time, as individual partitions are
**	created, the appropriate QEU_LOGPART_CHAR entry will be consulted
**	so that QEU can tell DMF how the partition should be structured.
**
**	Since there may be no partition WITH clauses, or very many, the
**	entries are in a list headed by qeu_logpart_list in the QEU_CB,
**	rather than being an array.
**
**	Note: for the initial implementation, the only physical
**	characteristics allowed will be LOCATION=.  The char_array
**	and key_array arrays will always be null.
**
**	For the CREATE TABLE AS SELECT situation, the QEU_CB does not
**	appear in the final query plan, because the qef/dmu operations
**	involved have to occur in the context of a multi-phase statement
**	instead of being standalone.  For CREATE TABLE AS SELECT, the
**	QEU_LOGPART_CHAR list is attached to the DMU action header.
**	(The parse still builds a QEU_CB but OPC doesn't bother
**	duplicating it.)
**
** History:
**	27-Jan-2004 (schka24)
**	    Written for partitioned tables project.
**	20-Feb-2004 (schka24)
**	    Initial name very poorly chosen, change.
*/

typedef struct _QEU_LOGPART_CHAR
{
	struct _QEU_LOGPART_CHAR *next;	/* Pointer to next block */
	i2	dim;			/* Dimension at which to apply this
					** option block. (0 = outermost)
					*/
	u_i2	partseq;		/* Partition sequence number within
					** that dimension (1-origin!)
					*/
	u_i2	nparts;			/* Number of consecutive logical
					** partitions to apply the options to
					*/
	i2	extra;			/* Filler, notused at present */
	DM_DATA	loc_array;		/* Array of locations */
	DM_DATA char_array;		/* Array of DMU characteristics */
	DM_PTR	key_array;		/* Array of key column descriptors */
} QEU_LOGPART_CHAR;

/*
** Name: QEU_QUAL_PARAMS -- Qualification function parameter block
**
** Description:
**
**	When retrieving rows from some catalog for a QEU function, it's
**	allowed (indeed, encouraged) to pre-qualify the rows inside DMF
**	so that the QEU function doesn't have to look at unnecessary rows.
**	Essentially, we want to tap into the DMF row-qualification feature.
**
**	DMF row qualification is designed for qualifying ordinary queries,
**	and includes some extra "stuff" that doesn't need to be exposed
**	for QEU level qualification.  So, this parameter block has been
**	defined to simplify things for the QEU user.
**
**	The QEU user will generally define the qual-params block on the
**	stack (or somewhere else safe), and fill in any needed params
**	for the called function.  The called function is invoked with:
**
**	    db_status = (*function)(&adf_cb, &qual_params);
**
**	where the ADF cb belongs to DMF and can usually be ignored, and
**	the qual_params block can contain up to 4 pointers to "stuff" that
**	the called function can use.  The called function should set
**	qual_params.qeu_retval to ADE_TRUE if the row qualifies, and
**	to ADE_NOT_TRUE if it doesn't.
**
** History:
**
**	11-Apr-2008 (kschendel)
**	    Spiff up DMF qualification, define this for qeu.
*/

typedef struct
{
	PTR	qeu_qparms[4];		/* Anything, for use by function */
	PTR	qeu_rowaddr;		/* DMF puts address of row to qualify
					** here, before calling function */
	i4	qeu_retval;		/* Called function must set this as the
					** return value, ADE_TRUE means row
					** qualifies, ADE_NOT_TRUE means skip
					** it and get more rows. */
} QEU_QUAL_PARAMS;

/*}
** Name: QEU_CB - general utility control block
**
** Description:
**      This control block is used to specify input and output
** parameters to most of the utility routines.
**
** History:
**	27-may-86 (daved)
**          written
**      18-jul-89 (jennifer)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredictable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	07-jan-91 (seputis)
**	    added qeu_mask, qeu_timeout, qeu_maxlocks to support
**	    the OPF ACTIVE flag
**	19-may-92 (bonobo)
**	    Eliminated redundant typedef.
**	22-jun-92 (andre)
**	    defined qeu_f_qual, qeu_f_qarg, QEU_F_NEXT and QEU_F_RETURN
**	08-sep-92 (andre)
**	    defined QEU_TMP_TBL over qeu_flag
**	07-dec-92 (andre)
**	    defined qeu_ddl_info and qeu_qtext
**	11-feb-93 (andre)
**	    defined QEU_DROP_BASE_TBL_ONLY over qeu_flag; this mask will be used
**	    to indicate that only the specified base table is to be dropped
**	    (i.e. no attempt will be made to look up and destroy objects
**	    dependent on the specified base table); this bit will be honored
**	    when dropping BASE base tables (later it may be extended to STAR
**	    tables as well, but there is no need to do it now)
**	08-jun-93 (rblumer)
**	    defined QEU_WANT_USER_XACT flag; used by PSF when it wants a query
**	    to be run as part of a user transaction instead of the internal
**	    transaction that is sometimes used for parsing and compiling.
**	    To be used as part of the kluge verifying that constraints hold on
**	    existing data (during ALTER TABLE ADD CONSTRAINT).
**      13-sep-93 (rblumer)
**          remove obsolete QEU_WANT_USER_XACT flag.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	14-nov-95 (kch)
**	    Added current query mode, qeu_qmode. This change fixes bug 72504.
**	11-feb-99 (stephenb)
**	    Add tran_id field so that we can return tran id to SCF.
**	27-Jan-2004 (schka24)
**	    Added partitioning info pointer.
**      12-Apr-2004 (stial01)
**          Define qeu_length as SIZE_TYPE.
**	11-Apr-2008 (kschendel)
**	    Update qualification function types.
**      27-May-2008 (coomi01) Bug 120413
**          Add bit flag to qeu_flag to indicate presence of a new
**          default value.
*/
struct _QEU_CB
{
    /* standard stuff */
    QEU_CB     *qef_next;           /* The next control block */
    QEU_CB     *qeu_prev;           /* The previous one */
    SIZE_TYPE   qeu_length;         /* size of the control block */
    i2          qeu_type;           /* type of control block */
#define QEUCB_CB    4
    i2          qeu_s_reserved;
    PTR         qeu_l_reserved;
    PTR         qeu_owner;
    i4          qeu_ascii_id;       /* to view in a dump */
#define QEUCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'U', 'C')
    DB_TAB_ID   qeu_tab_id;         /* table id */
    PTR		qeu_acc_id;         /* access id */
    i4     qeu_count;          /* number of tuples - output */
#define QEU_ALL -1
    i4          qeu_tup_length;     /* length of tuple */
    i4          qeu_getnext;        /* reposition with new key */
#define QEU_NOREPO 0
#define QEU_REPO 1
    QEF_DATA    *qeu_input;         /* input tuples */
    QEF_PARAM	*qeu_param;	    /* input parameters */
    QEF_DATA    *qeu_output;        /* output tuple stream */
    i4		qeu_klen;	    /* length of key array in bytes*/
    DMR_ATTR_ENTRY **qeu_key;       /* key information. */
    DB_STATUS	(*qeu_qual)(void *,QEU_QUAL_PARAMS *);  /* qualification function */
    QEU_QUAL_PARAMS *qeu_qarg;	    /* 2nd argument to function */
    i4     qeu_d_op;           /* dmf operation. See DMF EIS for opcodes */
    void	*qeu_d_cb;	    /* dmf control block appropriate for
				    ** above named operation, typically a
				    ** DMU_CB, sometimes a DMT_CB or other
				    */
    QEU_LOGPART_CHAR *qeu_logpart_list;  /* Pointer to list of logical
				    ** partition characteristics, if any
				    */
    DB_CURSOR_ID qeu_qp;	    /* query plan id */
    PTR		qeu_qso;	    /* handle to QSF object */
    i4          qeu_pcount;         /* number of sets of input parameters */
    PTR		qeu_db_id;          /* a dmf database id */
    CS_SID	qeu_d_id;           /* a dmf session id */
    i4	qeu_flag;	    /* call modifier */
    
	/* WARNING  WARNING  WARNING
	** This flag field is not always used as a bit field;
	** sometimes it is set to values defined for other fields.
	** Some  code sets it to QEF_READONLY (currently 1),
	** other code sets it to DMU_U_DIRECT (currently 2L).
	**                                         (rblumer 08-jun-93)
	*/

#define     QEU_POSITION_ONLY	    0x02000000  /* qeu_get to perform 
						** DMR_POSITION only to 
						** determine if qulaified 
						** record exists.
						*/
#define	    QEU_DROP_BASE_TBL_ONLY  0x04000000
				    /*
				    ** only the specified base table is to be
				    ** dropped (i.e. no attempt will be made to
				    ** look up and destroy objects dependent on
				    ** the specified base table)
				    */
#define	    QEU_TMP_TBL		    0x08000000	/* dealing with a temp table */
						 /*
						 ** access by tid (value is in
						 ** qeu_tid)
						 */
#define	    QEU_BY_TID		    0x10000000
#define     QEU_SHOW_STAT           0x20000000   /* Return all to RDF. */
#define	    QEU_BYPASS_PRIV	    0x40000000   /* Don't check SECURITY privilege */
#define     QEU_DFLT_VALUE_PRESENT  0x80000000   /* A new default value was given */

    i4	qeu_access_mode;    /* DMF read or write */
    i4	qeu_lk_mode;	    /* lock mode for table open */
    ULM_RCB	*qeu_mem;	    /* memory control block with open stream */
    i4		qeu_eflag;	    /* type of error handling semantics */
/*#define QEF_EXTERNAL	0;*/
				    /* send error to user */
/*#define QEF_INTERNAL	1;*/
				    /* return error code to caller */

    DB_ERROR    error;              /* error block */
    i4	qeu_tid;	    /* tid of the tuple to be accessed */
    i4		qeu_mask;	    /* modifier masks */
#define                 QEU_TIMEOUT     1
/* Set if the qeu_timeout has a valid parameter to be used on table open*/
#define                 QEU_MAXLOCKS    2
/* Set if the qeu_maxlocks has a valid parameter to be used on table open*/
    i4	qeu_timeout;	    /* timeout specifier for table open */
    i4	qeu_maxlocks;	    /* maxlock specifier for table open */
    PTR		qeu_f_qual;	    /*
				    ** qualification function used inside QEF;
				    ** initially defined to enable update of
				    ** multiple rows; function must accept two
				    ** PTRs as parameters, the first one will be
				    ** a poinyter to t tuple, the second -
				    ** qeu_f_qarg; return values will be as
				    ** follows:
				    */

#define	    QEU_F_NEXT		    1	/* skip to next row */
#define	    QEU_F_RETURN	    2	/* row qualified */

    PTR		qeu_f_qarg;	    /* argument to QEF qualification function */

    PTR		qeu_ddl_info;	    /*
				    ** used for STAR CREATE/REGISTER/DROP to 
				    ** contain a ptr to QEU_DDL_INFO structure
				    */
    char	*qeu_qtext;	    /* query text for GW REGISTER TABLE/INDEX */
    i4		qeu_qmode;	    /* current query mode */
    DB_TRAN_ID	qeu_tran_id;	    /* current tran id */
};
