/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: GWFINT.H - gateway internal/private interface.
**
** Description:
**	This file contains the definition of gateway internal interfaces which
**	are private to GWF.
**
** History:
**      21-Apr-1989 (alexh)
**          Created for Gateway project.
**	24-dec-89 (paul)
**	    Several updates over several days to make data structures match the
**	    code.
**	5-apr-90 (bryanp)
**	    Added improved support for memory configuration.
**	9-apr-90 (bryanp)
**	    Added gwf_gw_active and gwf_gw_xacts to support new gwf_call()
**	    interface. These fields are set at init time, and may be queried by
**	    calling gwf_call(GWF_SVR_INFO,...).
**	8-may-90 (alan)
**	    Added 'gws_username' to GW_SESSION structure.
**	19-sep-90 (linda)
**	    Cleaned up structure member names.  Specifically, made every
**	    structure member have a tag prepended; e.g. the GW_ATT_ENTRY
**	    structure members now all start with "gwat_".
**      10-jun-92 (schang)
**          GW merge.
**      8-Sep-92 (daveb)
**	     Remove suprious typedef causing compiler warnings.
**	14-sep-92 (robf)
**	     Added global & per-session tracing.
**      18-sep-92 (schang)
**          add the following field  in _GW_FACILITY
**          PTR             gwf_xhandle;
**      12-apr-92 (schang)
**         Add     bool        gws_interrupted;
**	04-jun-93 (ralph)
**	    DELIM_IDENT:
**	    Added GW_SESSION.gws_gw_info array, GW_SESSION.gws_dbxlate and
**	    GW_SESSION.gws_cat_owner to contain session-specific information
**	    with respect to case translation semantics.
**	    Added prototypes for gws_alter() and gws_gt_gw_sess().
**      24-jul-97 (stial01)
**          Add xrcb_gchdr_size to GWX_RCB.
**      23-jun-99 (chash01)  integrate changes
**        13-jun-96 (schang)
**          6.4 change integration into oping1.x
**          add GWX_VSINIT definition; change xrcb_xhandle to *xrcb_xhandle,
**          and add xrcb_xbitset field in gwx_rcb.
**        24-jun-96 (schang)
**          add gwf_xbitset field.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-sep-2000 (hanch04)
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      12-Aug-2010 (horda03) b124109
**          Defined prototype for gws_gt_parent_gw_sess().
**/

/* forward declarations */
typedef	struct _GW_SESSION  GW_SESSION;
typedef struct _GW_FACILITY GW_FACILITY;

/* facility globals */
GLOBALREF	GW_FACILITY	*Gwf_facility;	    /* defined in gwffac.c  */
GLOBALREF	GWF_ITAB	Gwf_itab;	    /* defined in gwftabi.c */
GLOBALREF	DB_STATUS	((*Dmf_cptr))();    /* defined in gwffac.c  */

/*
** The former contents of gwxit.h are now here.
*/

#define	GWX_VERSION	1

/*}
** Name: GW_VECTOR - A Gateway exit vector
**
** Description:
**	This structure  contains  an array  of  exit routine
**	address for a gateway.
**
** History:
**      2-May-1989 (alexh)
**          Created for Gateway project.
**	7-oct-92 (daveb)
**	    Add GWX_VSTERM for session termination.  Add
**	    xrcb_gw_session as an argument, to be passed by all
**	    calls to functions taking place in a session.  This
**	    gives exits access to their xrcb_gw_session->gws_exit_scb[]
**	    slots for exit-private SCBs.
**/
#define	GWX_VTERM	0
#define	GWX_VINFO	1

#define	GWX_VTABF	3
#define	GWX_VIDXF	4
#define	GWX_VOPEN	5
#define	GWX_VCLOSE	6
#define	GWX_VPOSITION	7
#define	GWX_VGET	8
#define	GWX_VPUT	9
#define	GWX_VREPLACE	10
#define	GWX_VDELETE	11
#define	GWX_VBEGIN	12
#define	GWX_VABORT	13
#define	GWX_VCOMMIT	14
#define	GWX_VATCB	15	/* Allocate gateway cspecific TCB */
#define	GWX_VDTCB	16	/* Deallocate gateway specific TCB */
#define GWX_VSTERM	17
#define GWX_VSINIT      18
#define	GWX_VMAX	19	/* Max must be largest opcode + 1 */

typedef DB_STATUS (*GWX_VECTOR[GWX_VMAX])();

/*}
** Name: GWX_RCB - GW Request control structure
**
** Description:
**	This structure is used for requesting service from the GWF exit.  The
**	structure should not be used to store permanent information.
**
** History:
**      26-Apr-1989 (alexh)
**          Created for Gateway project.
**	23-dec-89 (paul)
**	    Added ability to pass the RSB and TCB for the gateway object to the
**	    gateway exits. All the information in these structures may be
**	    required to run the gateway. In addition, the gateway needs to
**	    attach its own structures onto these 'generic' structures.
**	28-mar-90 (bryanp)
**	    New fields xrcb_dmf_cptr, xrcb_database_id, and xrcb_xact_id added
**	    to the GWX_RCB for use by the DMF gateway.
**	08-jun-90 (linda)
**	    Add xrcb_page_cnt field to return back the "page" count for the
**	    file.  For RMS this is the number of VMS blocks.  This number is
**	    not adjusted to represent Ingres pages because:  (1) I/O is done on
**	    a record basis, not block I/O, so there will be more I/O's anyway;
**	    (2) ingres pages are irrelevant even if we were doing block I/O;
**	    (3) it isn't particularly accurate anyway re: the actual amount of
**	    data in the file, since if the file is keyed there is space taken
**	    for those and in any case the amount of space allocated to the file
**	    does not represent the amount of space actually used.  Better to
**	    overestimate than to underestimate.  This information is only used
**	    by the optimizer.  May want to adjust at a later date.
**	30-sep-92 (robf)
**	    Added data4 value so table info can be passed in during the
**	    REGISTER operation.
**      18-sep-92 (schang)
**          add xrcb_xhandle in GWX_RCB to support server wide data inside a
**          specific gateway
**      24-jul-97 (stial01)
**          Add xrcb_gchdr_size to GWX_RCB.
**/
typedef struct
{
    PTR		    xrcb_rsb;		/* Pointer to the GW_RSB for this table
					** or NULL if this operation does not
					** apply to an open table.
					*/
    i4	    xrcb_gwf_version;	/* GWF version id */
    i4		    xrcb_gw_id;		/* gateway id */
    DB_TAB_NAME	    *xrcb_tab_name;	/* INGRES table name */
    DB_TAB_ID	    *xrcb_tab_id;	/* INGRES table id */
    i4	    xrcb_access_mode;	/* DMT_READ, DMT_WRITE */
    i4	    xrcb_flags;		/* Flags for record operations */
#define	GWX_GETNEXT	0L		/* Get next record */
#define	GWX_BYTID	1L		/* Get by record id */
    DMT_TBL_ENTRY   *xrcb_table_desc;	/* INGRES table description */
    i4	    xrcb_column_cnt;	/* column count */
    DMT_ATT_ENTRY   *xrcb_column_attr;	/* Array of DMT_ATT_ENTRY */
    DMT_IDX_ENTRY   *xrcb_index_desc;	/* Pointer to index entry if index */
    GWX_VECTOR	    *xrcb_exit_table;	/* array of ptrs to exit functions */
    i4	    xrcb_exit_cb_size;	/* exit rsb size */
    i4	    xrcb_xrelation_sz;	/* iigwX_relation size */
    i4	    xrcb_xattribute_sz;	/* iigwX_attribute size */
    i4	    xrcb_xindex_sz;	/* iigwX_index size */

					/* shared use data buffer structures */
    DM_DATA	    xrcb_var_data1;	/* a variable length data */
    DM_DATA	    xrcb_var_data2;	/* a variable length data */
    DM_DATA	    xrcb_var_data3;	/* a variable length data */
    DM_DATA	    xrcb_var_data4;	/* a variable length data */
    DM_PTR	    xrcb_var_data_list;	/* a list of variable length data */
    DM_PTR	    xrcb_mtuple_buffs;	/* multiple tuple bufferss, same size */
    i4	    xrcb_page_cnt;	/* number of "pages" in gateway file */
    PTR		    xrcb_xatt_tidp;	/* ptr to tidp tuple for extended attr.
					   relation for this gateway */
    DB_ERROR	    xrcb_error;		/* Contains error information */
    DB_STATUS	    (*xrcb_dmf_cptr)(); /* This function pointer contains the
					** address of dmf_call(). It is for
					** the DMF gateway only. The DMF gateway
					** uses this call pointer to avoid link
					** problems.
					*/
					/* Next 2 fields used by DMF gateway */
    PTR		    xrcb_database_id;   /* DMF database ID */
    PTR		    xrcb_xact_id;	/* DMF transaction ID */
    PTR             *xrcb_xhandle;      /* field to support server wide data */
                                        /* for a specific gateway,i.e. RMS   */
    i4		    xrcb_xbitset;       /* bit vector */
    GW_SESSION	    *xrcb_gw_session;   /* session information */
    PTR		    xrcb_gca_cb;	/* GC hdr size */
} GWX_RCB;



/*}
** Name: GW_ATT_ENTRY - Gateway table attribute structure
**
** Description:
**	This structure contains attribute information of a gateway table, which
**	includes regular INGRES information, extended attribute information
**	which is optional and private to the specific gateway
**
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**	28-dec-89 (paul)
**	    Updated to working code.	    
**/
typedef struct
{
    DMT_ATT_ENTRY	gwat_attr;	    /* INGRES attribute info */
    DM_DATA		gwat_xattr;	    /* extended attributed string */
} GW_ATT_ENTRY;

/*}
** Name: GW_IDX_ENTRY - Gateway index structure
**
** Description:
**	This structure contains index information of a gateway table, which
**	includes regular INGRES information and gateway specific index
**	information which is optional and private to the gateway.
**
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**	28-dec-89 (paul)
**	    updated to match (closer to working) code.
**/
typedef struct
{
    DMT_IDX_ENTRY	gwix_idx;	    /* INGRES attribute info */
    DM_DATA		gwix_xidx;	    /* GW extended index info pointer */
} GW_IDX_ENTRY;

/*}
** Name: GW_EXTCAT_HDR - Gateway extended catalog entry headers
**
** Description:
**	This structure (a union) describes the first few fields of the various
**	extended catalog entries.  Basically, although we cannot know the
**	entire layout for these catalogs (iigwXX_[relation, attribute, index]),
**	we must be able to assume the first few fields.  Those which must be
**	the same for all non-relational gateways are defined by this structure.
**
** History:
**	16-jan-91 (linda)
**	    Created.
*/
typedef struct
{
    DB_TAB_ID	gwxt_tab_id;	    /* first 2 fields always baseid, indexid */
    union
    {
	char	gwxt_xrel_misc[1];  /* iigwXX_relation's 3rd field undefined */
	i2	gwxt_xatt_id;	    /* iigwXX_attribute has attribute id */
	char	gwxt_xidx_misc[1];  /* iigwXX_index's 3rd field undefined */
    } gwxt_fld3;
} GW_EXTCAT_HDR;

/*}
** Name: GW_TCB - Gateway table strucuture
**
** Description:
**	This structure contains all information needed for a gateway table.
**	Since some of the information requires look up in the extended
**	catalogs, a list of gatway tcb's are cached at GW facility level, i.e.
**	sharable by all GWF sessions. When a table is opened for a session,
**	cached tcb list is searched for. If not found, a new tcb is created and
**	initialized with info read from extended catalog and added to the tcb
**	cache. The "internal_tcb" is optional and private to a specific
**	gateway.
**
**	There are several interesting points to make.
**
**	1.  There is a race condition where two users may decide to create a
**	    TCB for the same table. We allow this to happen since the TCB's are
**	    read-only, having several should not be an issue. It is important
**	    that gwt_remove and gwi_remove make sure to remove all TCB's
**	    corresponding to a specific table.
**
**  THE TROUBLE WITH THIS IS THAT WE USE gwt_use_count TO TELL HOW MANY USERS
**  ARE CURRENTLY ACCESSING A TABLE, AND WE DO DELTE TCB'S ON TABLE CLOSE TO
**  AVOID GROWING MEMORY INDEFINITELY.  THE CALLER IN THIS CASE (GWT_CLOSE)
**  KNOWS WHICH TCB IT HAS; HOWEVER, GWU_DELTCB SEARCHES THROUGH THE LIST FOR
**  ANY TCB WITH THE SAME TABLE ID; IF THAT ONE HAS A gwt_use_count OF > 0, IT
**  EMITS AN ERROR.
**
**	2.  We should not have to worry about a table being removed while it is
**	    still being referenced by another session. The INGRES concurrency
**	    mechanism should handle this correctly.
**
**	3.  The TCB list may only be accessed while it is protected by a
**	    semaphore. The list must be locked exclusively whenever an entry is
**	    being added or deleted. The list must be locked shared whenever it
**	    is being searched. If the desired TCB is found, its use_count must
**	    be incremented before the shared lock on the TCB list is removed.
**
**	4. TCB's and their affiliated gateway extended TCB's are allocated from
**	    their own ULM memory stream. The streams identifier is maintained
**	    in the TCB. The TCB may be deallocated by simply closing this
**	    stream.
**	    
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**	26-dec-89 (paul)
**	    Updated TCB to handle internal tcb allocation.
**	5-apr-90 (bryanp)
**	    Added tcb_memleft field to fully support GW_TCB's in their own
**	    memory streams.
**	06-oct-93 (swm)
**	    Bug #56443
**	    Change type of gwt_db_id from i4 to PTR for consistency
**	    with gwr_database_id which has now also changed to PTR.
**/
typedef struct _GW_TCB
{
    struct _GW_TCB  *gwt_next;		/* next TCB in GWF TCB list */
    i4		    gwt_gw_id;		/* gateway id */
    PTR		    gwt_db_id;		/* Database id for this table */
    DMT_TBL_ENTRY   gwt_table;		/* INGRES table info */
    DM_DATA	    gwt_xrel;		/* gateway extended relation info */
    GW_ATT_ENTRY    *gwt_attr;		/* array of GW column definitions */
    GW_IDX_ENTRY    *gwt_idx;		/* GW index info.
					** If this TCB describes an index, this
					** field contains a pointer to the
					** index description for the index
					** being referenced.  This field is
					** NULL if this TCB refers to a base
					** table.
					*/
    i4	    gwt_use_count;	/* current use count.
					**  = 0 NOT IN USE
					**  > 0 COUNT OF CURRENT USERS
					**  < 0 (not used at present)
					*/
    PTR		    gwt_internal_tcb;	/* Any gateway specific information
					** associated with this TCB.
					*/
    ULM_RCB	    gwt_tcb_mstream;	/* Memory stream descriptor for
					** allocating memory associated with
					** this TCB.
					*/
    SIZE_TYPE	    gwt_tcb_memleft;	/* Amount of memory left in this
					** GW_TCB's memory stream. Initialized
					** by gwu_gettcb(), maintained by
					** ULM routines.
					*/
} GW_TCB;

/*}
** Name: GW_FACILITY - GW facility control structure
**
** Description:
**	This structure contains information that is global to the GWF.  Updates
**	to this structure require synchronization, except during facility
**	startup and shutdown.
**
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**	9-apr-90 (bryanp)
**	    Added gwf_gw_active and gwf_gw_xacts to support new gwf_call()
**	    interface. These fields are set at init time, and may be queried by
**	    calling gwf_call(GWF_SVR_INFO,...).
**	27-sep-90 (linda)
**	    Added gwf_xatt_tidp field to support gateway secondary indexes.
**      22-apr-92 (schang)
**          GW merge
**	    30-jan-1992 (bonobo)
**	        Removed the redundant typedefs to quiesce the ANSI C 
**	        compiler warnings.
**      18-sep-92 (schang)
**          add the following field
**          PTR             gwf_xhandle;
**      23-jun-99 (chash01) integrate
**        24-jun-96 (schang)
**          add gwf_xbitset field.
**	12-Oct-2007 (kschendel)
**	    xhandle is PTR * in the xrcb, make it match here.  Not sure why,
**	    maybe the RMS gw wants it that way, who knows.
**/
struct _GW_FACILITY
{
    struct
    {
	bool		gwf_gw_exist;		/* whether gw was loaded */
	GWX_VECTOR	gwf_gw_exits;		/* gw exit vector */
	i4		gwf_rsb_sz;		/* session control block size */
	i4		gwf_xrel_sz;		/* extended relation size */
	i4		gwf_xatt_sz;		/* extended attribute size */
	i4		gwf_xidx_sz;		/* extended index size */
	DB_TAB_NAME	gwf_xrel_tab_name;	/* extend relation name */
	DB_TAB_NAME	gwf_xatt_tab_name;	/* extend attribute name */
	DB_TAB_NAME	gwf_xidx_tab_name;	/* extend index name */
	PTR		gwf_xatt_tidp;		/* extended attr. tidp tuple */
        PTR             *gwf_xhandle;           /* specific gateway data exit */
        i4		gwf_xbitset;             /* specific gw bit flags */
    } gwf_gw_info[GW_GW_COUNT];

    i4	    gwf_memleft;
    ULM_RCB	    gwf_ulm_rcb;	    /* GWF memory pool */
    GW_TCB	    *gwf_tcb_list;	    /* GW table info cache list */
    CS_SEMAPHORE    gwf_tcb_lock;	    /* tcb list lock */
    i4	    gwf_gw_active;	    /* non-zero if any GW is active. */
    i4	    gwf_gw_xacts;	    /* non-zero if any GW does xacts */
    i4	    gwf_trace[1024 / BITS_IN(i4)]; /* Global trace flags */
};

/*}
** Name: GW_RSB - GW record stream control structure
**
** Description:
**	This structure contains record stream information.  This structure
**	is allocated and initialized by gwt_open request.
**
** History:
**	28-dec-89 (paul)
**	    Added this history section.
**	26-mar-90 (linda)
**	    Added gwrsb_error field.
**	14-jun-90 (linda, bryanp)
**	    Add gwrsb_access_mode field.
**	16-jun-90 (linda)
**	    Add gwrsb_page_cnt field.
**	25-jan-91 (RickH)
**	    Add gwrsb_timeout field.  Temporary fix (!).
**	30-apr-1991 (rickh)
**	    Removed definition of tidjoin bit.
**	22-jul-1991 (rickh)
**	    Removed gwrsb_ing_base_tuple.
**	14-Apr-2008 (kschendel)
**	    Add new-style qualification stuff.
**/
typedef struct _GW_RSB
{
    struct _GW_RSB  *gwrsb_next;	    /* next session RSB */
    GW_SESSION	    *gwrsb_session;	    /* session owner */
    GW_TCB	    *gwrsb_tcb;		    /* rsb's tcb */
    i4	    gwrsb_flags;	    /* flags word */

#define	GWRSB_FILE_IS_OPEN  0x00000001L	    /* file has really been opened! */

    DM_DATA	    gwrsb_low_key;	    /* lower bound of range position.
					    ** NULL means no lower bound
					    */
    DM_DATA	    gwrsb_high_key;	    /* higher bound of range position.
					    ** NULL means no higher bound
					    */
    /* See dmrcb.h for more information on how the row qualification works.
    ** The necessary function address and params are passed thru to GWF
    ** so that the gateway can apply qualifications.
    ** Since the GWRSB is the gwf equivalent of the DMF RCB, i.e. the thing
    ** that identifies an opened instance, the qual stuff ends up here.
    ** Note that qualification must be specified at the time of POSITION
    ** in order to take effect.  (This is true of DMF as well.)
    */
    PTR		    *gwrsb_qual_rowaddr;    /* Address of row to be qualified
					    ** is stashed here before calling
					    ** the qual function. */

    DB_STATUS	    (*gwrsb_qual_func)(void *,void *);
					    /* Function to call to qualify
					    ** some particular row. */

    void	    *gwrsb_qual_arg;	    /* Argument to qualifcation function. */
    i4		    *gwrsb_qual_retval;	    /* Qual func sets *retval to
					    ** ADE_TRUE if row qualifies */
    PTR		    gwrsb_internal_rsb;	    /* private gateway specific RSB */
    ULM_RCB	    gwrsb_rsb_mstream;	    /* Memory stream used to allocate
					    ** information associated with this
					    ** record stream.  The stream is
					    ** opened at table 'open' and
					    ** closed at table 'close'.  All
					    ** memory allocated for this access
					    ** to the named table should be
					    ** allocated in this stream.
					    */
    i4	    gwrsb_access_mode;	    /* values are same as gw_rcb's */
    i4	    gwrsb_page_cnt;	    /* get from gwx_rcb at table open.*/
#define	GWF_DEFAULT_PAGE_COUNT	1

    i4	    gwrsb_timeout;	    /* holds dmp_rcb->rcb_timeout
					    ** TEMPORARY (!) 25-jan-90 */
    i4		    gwrsb_use_count;	    /* not used. */
    u_i4	    gwrsb_tid;		    /* tid -- derived from foreign
					    ** record id
					    */
    DB_ERROR	    gwrsb_error;
} GW_RSB;

/*}
** Name: GW_SESSION - GW session control structure
**
** Description:
**	This structure contains information for a GW session. The address of a
**	session structure is the GW session id. All data manipulations can only
**	be requested after a session is established.
**
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**	26-dec-89 (paul)
**	    Added a pointer to the ADF_CB so we can do data conversions.
**	8-may-90 (alan)
**	    Added 'gws_username' to GW_SESSION structure.
**	30-apr-1991 (rickh)
**		Removed definition of tidjoin bit.
**	14-sep-92 (robf)
**	     Added per-session tracing.
**      12-apr-93 (schang)
**          add     bool        gws_interrupted;
**	04-jun-93 (ralph)
**	    DELIM_IDENT:
**	    Added GW_SESSION.gws_gw_info array, GW_SESSION.gws_dbxlate and
**	    GW_SESSION.gws_cat_owner to contain session-specific information
**	    with respect to case translation semantics.
**	14-Apr-2008 (kschendel)
**	    Add adfcb / error-code pointers to be set during calls to
**	    a row qualifier function, as arithmetic exceptions during a
**	    qualification call are OK and must be handled.  (since we don't
**	    switch to qef context to call the qualifier any more.)
**/
struct _GW_SESSION
{
    PTR		gws_scf_session_id;	    /* SCF session id */
    SIZE_TYPE	gws_memleft;		    /* memory allocation limit */
    GW_RSB	*gws_rsb_list;		    /* list of current session' rsbs */
    bool	gws_txn_begun;		    /* transaction already begun */
    ULM_RCB	gws_ulm_rcb;		    /* sessional ulm memory stream */
    ADF_CB	*gws_adf_cb;		    /* Pointer to the ADF control block
					    ** for this session. We use the
					    ** session's control block since it
					    ** has all the user's global
					    ** settings such as date format,
					    ** decimal char, etc.
					    */
    char	gws_username[DB_OWN_MAXNAME+1];  /* Gateway session username */
    i4	gws_trace[1024 / BITS_IN(i4)]; /* Session level trace flags */
    PTR		gws_exit_scb[ GW_GW_COUNT ]; /* GW specific SCB, as needed */
    bool        gws_interrupted;            /* schang: interrupt support */
    u_i4	*gws_dbxlate;		    /* Case translation semantics flags
					    ** See <cuid.h> for masks.
					    */
    DB_OWN_NAME	*gws_cat_owner;		    /* Catalog owner name.  Will be
					    ** "$ingres" if regular ids lower;
					    ** "$INGRES" if regular ids upper.
					    */

    /* qfun-adfcb is normally NULL;  it's set while running a row-qualification
    ** function so that the GWF exception handler knows that arithmetic
    ** exceptions are plausible and can be caught.
    ** qfun-errptr is set by the logical layer to the request CB error code,
    ** so that a non-continuing (error) arithmetic exception can indicate
    ** to the calling facility that the ADF message has been issued.
    */
    ADF_CB	*gws_qfun_adfcb;	    /* ADF CB for row qualification */
    DB_ERROR	*gws_qfun_errptr;	    /* &cb->error for storing
					    ** exception handler info, so
					    ** that dmfcall exit knows.
					    ** ADF "user error" goes into the
					    ** error data, for QEF.
					    */
    struct
    {
	DB_TAB_NAME	gws_xrel_tab_name;	/* extend relation name */
	DB_TAB_NAME	gws_xatt_tab_name;	/* extend attribute name */
	DB_TAB_NAME	gws_xidx_tab_name;	/* extend index name */
    } gws_gw_info[GW_GW_COUNT];		    /* Case translated catalog names */
};



/*
** Name: GWF memory defaults
**
** Description:
**
** Default memory sizes. GWF uses these values to size its memory limits
** to 'reasonable' values.
**	GWF_FACMEM_DEFAULT	- GWF facility basic memory overhead. This is
**				  the memory to be used for GW_TCB's and other
**				  items which are not user-specific.
**	GWF_SESMEM_DEFAULT	- GWF per-user memory limit. This is the max
**				  amount of GWF memory which any one user can
**				  acquire.
**	GWF_TCBMEM_DEFAULT	- GWF per-table memory limit. This is the max
**				  amount of GWF memory which any one GW_TCB can
**				  acquire.
**
** Note that the total GWF memory usage is set (by gwf_def_pool_size()) to:
**	GWF_FACMEM_DEFAULT + (num_users * GWF_SESMEM_DEFAULT)
**
** Note also that the GW_FACILITY structure is allocated directly from SCF
** server-wide memory and so does not enter into this discussion.
**
** These values are used as defaults. Eventually, we plan to provide a set of
** server startup parameters which will allow these values to be configured for
** particular servers.
**
** History:
**	3-apr-90 (bryanp)
**	    Created and set to preliminary values.
*/
# define    GWF_FACMEM_DEFAULT	((i4)0x080000) /* 512K */
# define    GWF_SESMEM_DEFAULT	((i4)0x040000) /* 256K */
# define    GWF_TCBMEM_DEFAULT	((i4)0x010000) /*  64K */

/* func externs */

FUNC_EXTERN DB_STATUS gwf_init( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gwf_svr_info( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gwf_term( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gwi_register(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwi_remove(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwr_delete(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwr_get(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwr_position(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwr_put(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwr_replace(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gws_init( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gws_alter( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gws_term( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gws_check_interrupt(GW_SESSION *gwf_ses_cb);
FUNC_EXTERN DB_STATUS gwt_close(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwt_open(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwt_register( GW_RCB *gw_rcb );
FUNC_EXTERN DB_STATUS gwt_remove(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwu_info(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwx_abort(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwx_begin(GW_RCB *gw_rcb);
FUNC_EXTERN DB_STATUS gwx_commit(GW_RCB *gw_rcb);

/* VARARGS */
FUNC_EXTERN VOID gwf_error();

GW_SESSION *gws_gt_gw_sess( void );

GW_SESSION *gws_gt_parent_gw_sess( void );

DB_STATUS gwu_copen(DMT_CB	*dmt,
		    DMR_CB	*dmr,
		    PTR		sid,
		    PTR		db_id,
		    i4	gw_id,
		    PTR		txn_id,
		    DB_TAB_ID	*tab_id,
		    DB_TAB_NAME	*cat_name,
		    i4	access_mode,
		    i4		pos,
		    DB_ERROR	*err );

DB_STATUS gwu_cdelete(DMR_CB	*dmrcb,
		      DM_DATA	*dmdata,
		      GW_RCB	*gwrcb,
		      DB_ERROR	*err );

DB_STATUS gwu_deltcb(GW_RCB *gw_rcb, i4  exist_flag, DB_ERROR *err );

DB_STATUS gwu_atxn(GW_RSB *rsb, i4 gw_id, DB_ERROR *err );

DB_STATUS gwu_delrsb( GW_SESSION  *session, GW_RSB *rsb, DB_ERROR *err );

DB_STATUS gwf_errsend( i4 err_code );

GW_RSB *gwu_newrsb( GW_SESSION	*session,
		   GW_RCB	*gw_rcb,
		   DB_ERROR	*err );

FUNC_EXTERN DB_STATUS gwf_adf_error(
		ADF_ERROR	*adf_errcb,
		DB_STATUS	status,
		GW_SESSION	*scb,
		i4		*err_code);
