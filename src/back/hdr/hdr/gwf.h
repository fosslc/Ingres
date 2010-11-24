/*
**Copyright (c) 2004 Ingres Corporation
*/
/**
** Name: GWF.H - gateway external interface.
**
** Description:
**	This file contains the definition of gateway external interfaces. The
**	parameters are usually read-only, even if it is passed by reference,
**	unless explicitly specified as output parameter.
**
** History:
**      21-Apr-1989 (alexh)
**          Created for Gateway project.
**	11-sep-89 (linda)
**	    Rather than pointing to DMF's dmt_cb and dmu_cb structs, we
**	    duplicate some of the fields here in the GW_RCB struct.  They are
**	    copied in originally during the parse phase.
**	28-dec-89 (paul)
**	    Remove unused fields, idx_id, gwchar_array, gwattr_array.  General
**	    cleanup to conform to server coding conventions.  Moved error
**	    definitions to the end of the file. They make reading structure
**	    definitions difficult if they are embedded in the definition.
**	9-apr-90 (bryanp)
**	    For the new gwf_call() interface, I added the GWF_OPERATION type, a
**	    set of GWF_OPERATION values, and some new messages.  I also added
**	    gwr_dmf_cptr, gwr_gw_active, and gwr_gw_xacts to the gw_rcb
**	    structure.
**	18-apr-90 (bryanp)
**	    Added error messages for the RMS exits, and added generic error
**	    messages used by GWF. All RMS error message numbers contain the
**	    string "_RMS_" in them for easy recognition.
**	21-may-90 (linda)
**	    Added RMS-Gateway-specific datatype errors here (from gwrmsdt.h in
**	    gwf!gwrms_vms directory).
**	22-may-90 (linda)
**	    Put RMS-Gateway-specific datatype errors back into gwrmsdt.h.  Left
**	    note here about which #'s are reserved.
**	11-jun-1990 (bryanp)
**	    Additional error codes:
**		E_GW5484_RMS_DATA_CVT_ERROR
**		E_GW050D_DATA_CVT_ERROR
**		E_GW050E_ALREADY_REPORTED
**		E_GW0303_SCC_ERROR_ERROR
**	12-jun-90 (linda)
**	    New error code:
**		E_GW54A3_BAD_XFMT_PARAM
**	18-jun-90 (linda, bryanp)
**	    New error codes:
**		E_GWxxxx_DEFAULT_STATS_USED
**		E_GWxxxx_RECORD_ACCESS_CONFLICT
**	5-jul-90 (linda)
**	    New error code:
**		E_GWXXXX_GWRMS_NO_DEFAULT
**	19-sep-90 (linda)
**	    Cleaned up structure member names.  Specifically, made every
**	    structure member have a tag prepended; e.g. the GW_RCB structure
**	    members now all start with "gwr_".
**	26-nov-90 (linda)
**	    Added error code E_GW0327_DMF_DEADLOCK, to be returned back up to
**	    DMF whenever the gateway gets deadlock from a DMF call.
**	14-Jun-91 (rickh)
**	    Changed name of GWR_BYPOS to GWR_GETNEXT, since that's how it's
**	    used.
**	11-jul-91 (rickh)
**	    Added E_GW5013_RMS_BAD_INDEX_VALUE.
**	7-aug-91 (rickh)
**	    Added E_GW5014_RMS_RFA_TOO_LONG.
**	17-sep-92 (robf)
**	    Added error codes for the C2 Audit Gateway (SXA) (4000 range)
**	21-sep-92 (robf)
**	    Removed "typedef" from GW_DMP_ATTS to silence compiler warnings.
**	24-sep-92 (robf)
**	    Added E_GW0220_GWU_BAD_XATTR_OFFSET error
**	16-sep-92 (Daveb)
**	    Remove all E_GWxxxx defines; people should include <ergwf.h>
**	    to get them.
**	7-oct-92 (daveb)
**	    define GW_IMA and GW_SXA slots.
**	2-Nov-1992 (daveb)
**	    cleaned up error definition for GWM.
**	22-oct-92 (rickh)
**	    Removed GW_DMP_ATTS.  This was yet another structure identical
**	    to DB_ATTS.  Since the DMF references to DMP_ATTS have been
**	    replaced with DB_ATTS, there's no reason to keep this other
**	    kludgy clone.  Typedef'd GW_DMP_ATTR to be _DM_COLUMN;  removed
**	    the declaration of GW_DMP_ATTR, which was an exact copy of
**	    _DM_COLUMN.
**	3-feb-1993 (markg)
**	    Changed the name of some error codes to be 32 characters or
**	    less in length as these were causing problems with ercompile.
**      09-apr-93 (schang)
**          add user interrupt support
**      23-jul-93 (schang)
**          add FUNC_EXTERN gwf_call and gwf_trace, and a new error
**          #define E_GW0342_RPTG_OUT_OF_DATA           (E_GW_MASK | 0X0342).
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Added gwr_dbxlate and gwr_cat_owner to GW_RCB to allow GWF to
**	    operate against databases with varying case translation semantics.
**	    Added E_GW0221_GWS_ALTER_ERROR error to report errors when trying
**	    to alter a session's case translation semantics.
**	    Added GWS_ALTER operation; adjusted GWF_MAX_OPERATION.
**      08-sep-93 (smc)
**          Changed gwr_scf_session_id and gwr_session_id to CS_SID.
**      01-oct-93 (johnst)
**	    Bug #56443
**          Changed gwr_database_id from i4 to PTR, to match actual
**	    usage in the code.
**     24-jul-1997 (stial01)
**          Added gwr_gchdr_size to GW_RCB.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Correctly cast gwr_session_id to PTR, not CS_SID.
**      12-Apr-2004 (stial01)
**          Define gwr_length as SIZE_TYPE.
**	17-Jan-2007 (kibro01)
**	    Move special 'is' clause object names here and put them in
**	    an array so they can be searched in pslsgram.yi
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**/

/*
** Forward declarations and incomplete structure types
*/
typedef	struct	_GW_RCB	GW_RCB;

/*}
** Name: GW_RCB - GW Request control structure
**
** Description:
**	This structure is used for requesting service from the GWF.  The
**	structure should not be used to store permanent information.
**
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**	9-apr-90 (bryanp)
**	    For the new gwf_call() interface, I added gwr_dmf_cptr,
**	    gwr_gw_active, and gwr_gw_xacts to the gw_rcb structure.  Also
**	    added 'type' and length' and the other pieces of a 'standard'
**	    control block header.
**	14-jun-90 (linda, bryanp)
**	    Add GWT_OPEN_NOACCESS access code, indicating that table can only
**	    be closed, no record access permitted.  For gateways, the
**	    underlying physical file will not be opened and need not exist.
**	18-jun-90 (linda, bryanp)
**	    Add GWT_MODIFY access code, indicating that table is being opened
**	    to obtain statistical information.  If the table doesn't exist, a
**	    warning is issued and default values are used.
**	7-oct-92 (daveb)
**	    define GW_IMA and GW_SXA slots.  Add gwr_scfcb_size and
**	    gwr_server as part of having SCF make the per-session startup
**	    calls, making GWF a first-class facility.
**	18-dec-92 (robf)
**	    Add GWF_VALIDGW entry point.
**	04-jun-93 (ralph)
**	    DELIM_IDENT:
**	    Added gwr_dbxlate and gwr_cat_owner to GW_RCB to allow GWF to
**	    operate against databases with varying case translation semantics.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**     24-jul-1997 (stial01)
**          Added gwr_gchdr_size to GW_RCB.
**	08-Jan-2001 (jenjo02)
**	    Correctly cast gwr_session_id to PTR, not CS_SID.
**	12-Apr-2008 (kschendel)
**	    Expand qualification function stuff to match new DMF style.
**	12-Oct-2010 (kschendel) SIR 124544
**	    gwr_char_array replaced with DMU_CHARACTERISTICS.
**/

#define	GWF_VERSION	1

struct _GW_RCB
{
    GW_RCB	    *gwr_next;		/* not currently used. */
    GW_RCB	    *gwr_prev;		/* not currently used. */
    SIZE_TYPE	    gwr_length;		/* length of this control block */
    i2		    gwr_type;		/* the type of the control block */
#define		GWR_CB_TYPE	    (i2) 0x01
    i2		    gwr_s_reserved;
    PTR		    gwr_l_reserved;
    PTR 	    gwr_owner;
    i4		    gwr_ascii_id;
#define		GWR_ASCII_ID	    CV_C_CONST_MACRO('g', 'r', 'c', 'b')
    i4	    gwr_gw_id;		/* gateway id */
#define	GW_IMS		1		/* IMS gateway identifier  - IBM */
#define	GW_RMS		2		/* RMS gateway identifier  - VAX/VMS */
#define	GW_VSAM		3		/* VSAM gateway identifier - IBM */
#define	GW_4		4
#define	GW_5		5
#define	GW_SXA		6		/* SXA gateway - INGRES internal */
#define	GW_IMA		7		/* IMA gateway - INGRES internal */
#define	GW_8		8
#define	GW_9		9
#define	GW_10		10
#define	GW_GW_COUNT	GW_10		/* Currently allow maximum of 10
					** gateways
					*/
    i4         gwr_access_mode;	/* Table access mode for open request */

#define	GWT_READ	    1L		/* Read request */
#define	GWT_WRITE	    2L		/* Write request */
#define GWT_OPEN_NOACCESS   3L		/* Open but no access */
#define GWT_MODIFY	    4L		/* Open for statistics */

    i4	    gwr_gw_sequence;	/* Sequence number used to relate a
					** group of open tables and indices to
					** the statement they are associated
					** with.
					*/
    i4	    gwr_flags;		/* Flag for get operations. */
#define	GWR_GETNEXT	    0L		/* Regular get next */
#define	GWR_BYTID	    1L		/* Get by "tid" (record id) */

    PTR	            gwr_xact_id;	/* Transaction ID. */
    PTR 	    gwr_database_id;	/* Database identifier. */
    DB_TAB_NAME     gwr_tab_name;	/* Database table name. */
    DB_OWN_NAME	    gwr_tab_owner;	/* Database table owner */
    DB_TAB_ID       gwr_tab_id;		/* Database table identifier.
					** base table id if this is a base
					** table request or secondary index id
					** if this is a request for access via
					** a secondary index.
					*/
/*
**  Take 6 fields below from dmu_cb struct.
*/
    DM_PTR          gwr_key_array;	/* Array of key attributes. */
    DM_PTR          gwr_attr_array;	/* Array of attributes. */
    struct _DMU_CHARACTERISTICS *gwr_dmf_chars;  /* DMF table attributes */
    DM_DATA         gwr_conf_array;	/* Array of configuration 
					** variables.
					*/
    CS_SID	    gwr_scf_session_id;	/* SCF session id */
    DMT_TBL_ENTRY   *gwr_table_entry;	/* table info */

    /* See dmrcb.h for more information on how the row qualification works.
    ** The necessary function address and params are passed thru to GWF
    ** so that the gateway can apply qualifications.
    */
    PTR		    *gwr_qual_rowaddr;	    /* Address of row to be qualified
					    ** is stashed here before calling
					    ** the qual function. */

    DB_STATUS	    (*gwr_qual_func)(void *,void *);
					    /* Function to call to qualify
					    ** some particular row. */

    void	    *gwr_qual_arg;	    /* Argument to qualifcation function. */
    i4		    *gwr_qual_retval;	    /* Qual func sets *retval to
					    ** ADE_TRUE if row qualifies */

    DM_DATA	    gwr_in_vdata1;	/* an input variable data */
    DM_DATA	    gwr_in_vdata2;	/* an input variable data */
    DM_PTR	    gwr_in_vdata_lst;	/* input list of variable length data */
					/*
					** The following parameters are output
					** parameters from GWF. They are
					** updated by GWF.
					*/
    DM_DATA	    gwr_out_vdata1;	/* a variable length data */
    DM_PTR	    gwr_mtuples;	/* multiple tuples of the same size */
    DB_TAB_NAME	    gwr_cat_name1;	/* a catalog name */
    DB_TAB_NAME	    gwr_cat_name2;	/* a catalog name */
    i4	    gwr_open_cnt;	/* number of table opens */
    i4	    gwr_pos_cnt;	/* number of position requests */
    i4	    gwr_get_cnt;	/* number of get requests */
    i4	    gwr_put_cnt;	/* number of put requests */
    i4	    gwr_del_cnt;	/* number of delete requests */
    i4	    gwr_rep_cnt;	/* number of replace requests */

    i4	    gwr_column_cnt;	/* column count */
    i4	    gwr_tuple_count;	/* tuple count */
    i4	    gwr_page_count;	/* page count */
    DB_TID	    gwr_tid;		/* tid derived from foreign record id */

    DB_ERROR	    gwr_error;		/* error return code */

    PTR	    gwr_gca_cb;		/* GCA cb */
    DB_STATUS	    (*gwr_dmf_cptr)();	/* Address of dmf_call(). */
    i4	    gwr_gw_active;	/* Set non-zero if gateway is active*/
    i4	    gwr_gw_xacts;	/* Set non-zero if any gw does xacts*/
    i4	    gwr_scfcb_size;	/* Set to per-sess CB for SCF to get */
    PTR		    gwr_server;		/* Set to facility global block */

    /*
    ** The following elements are used to establish case translation semantics
    ** and the catalog owner name so that GWF can work with databases of
    ** varying case translation semantics.
    */
    u_i4	    *gwr_dbxlate;	/* Case translation semantics flags
					** See <cuid.h> for masks.
					*/
    DB_OWN_NAME	    *gwr_cat_owner;	/* Catalog owner name.  Will be
					** "$ingres" if regular ids are lower;
					** "$INGRES" if regular ids are upper.
					*/

    /*
    ** The following elements are private to GWF. They may only be modified by
    ** GWF. The session_id must be passed with every call for a given session.
    ** It is returned by gws_init.  The rsb must be passed on every call
    ** associated with an open table.  It is returned by gwt_open.
    */
    PTR	    	    gwr_session_id;	/* 
					** gateway session id - (GW_SESSION *)
					*/
    PTR		    gwr_gw_rsb;		/* GW opened record stream context
					** (GW_RSB *)
					*/
};

/*}
** Name: GWF_ITAB_ - gateway initialization table structure.
**
** Description:
**	This  structure  defines  the  structure  of gateway initialization
**	table.  This  table  contains   the addresses  of  gwX_init routines
**	for all  installed gateway exits.
**
** History:
**      24-Apr-1989 (alexh)
**          Created for Gateway project.
**/
typedef DB_STATUS (*GWF_ITAB[GW_GW_COUNT])();

/*
** Name: GWF_OPERATION	    - operation code for a GWF operation.
**
** Description:
**	Gateway operations. Each of the following operations is invoked 
**	through the gwf_call interface.
**
**	This field is a i4  rather than a i4 because code switches on it.
**
** History:
**	9-apr-90 (bryanp)
**	    Created.
**	04-jun-93 (ralph)
**	    DELIM_IDENT:
**	    Added GWS_ALTER operation; adjusted GWF_MAX_OPERATION.
*/

typedef	    i4		GWF_OPERATION;

#define	    GWF_MIN_OPERATION	1
#define	    GWF_INIT		1
#define	    GWF_TERM		2
#define	    GWS_INIT		3
#define	    GWS_TERM		4
#define	    GWT_REGISTER	5
#define	    GWT_REMOVE		6
#define	    GWT_OPEN		7
#define	    GWT_CLOSE		8
#define	    GWI_REGISTER	9
#define	    GWI_REMOVE		10
#define	    GWR_POSITION	11
#define	    GWR_GET		12
#define	    GWR_PUT		13
#define	    GWR_REPLACE		14
#define	    GWR_DELETE		15
#define	    GWX_BEGIN		16
#define	    GWX_ABORT		17
#define	    GWX_COMMIT		18
#define	    GWU_INFO		19
#define	    GWF_SVR_INFO	20
#define	    GWU_VALIDGW		21
#define	    GWS_ALTER		22
#define	    GWF_MAX_OPERATION	22

/*
**  Gateway error codes.  All gateway error codes are defined here.
**
**  The gateway facility code is defined in dbms.h.
**
**  Error handling scheme is as follows:
**
**	There are a number of error categories:
**
**	exit error		--  specific gateway exit routine failure
**	gwf error		--  generic gateway routine failure
**	other facility error	--  other facility function failed
**	interface violation	--  caller sent bad inputs
**	informational		--  nothing logged; info passed to caller
**	miscellaneous		--  internally detected fatal errors
**	user errors		--  detailed message sent to user
**
**	All errors except user errors are logged by gwf and result in
**	transaction, session and/or server abort or cleanup.  User errors are
**	translated by higher levels (e.g. DMF, QEF) into "friendly" messages
**	for the user.  Currently, they are also logged by gwf.  An example of a
**	user error would be "disk quota exceeded" -- we would like the user to
**	see the message, and we don't need to shut down the session or server
**	for this type of error.
**  History:
**	10-apr-90 (linda)
**	    Rearranged error codes into groups; added many new ones.
**	16-apr-90 (linda)
**	    Added error code for bad flag to gwu_deltcb()
**		E_GW0666_BAD_FLAG
**	    Also added flag values
**		GWU_TCB_MANDATORY
**		GWU_TCB_OPTIONAL
**	18-apr-90 (bryanp)
**	    Added error messages for the RMS exits, and added generic
**	    error messages used by GWF. All RMS error message numbers contain
**	    the string "_RMS_" in them for easy recognition.
**	11-jun-1990 (bryanp)
**	    Additional error codes:
**		E_GW5484_RMS_DATA_CVT_ERROR
**		E_GW050D_DATA_CVT_ERROR
**		E_GW050E_ALREADY_REPORTED
**		E_GW0303_SCC_ERROR_ERROR
**      16-feb-93 (schang)
**          add user interrupt support
**	7-jul-93 (robf)
**	    Added E_GW407D_SXA_NO_PRIV
**	14-Apr-2008 (kschendel)
**	    Added GW0023 placeholder.
*/
#define	GWF_USERERR	1
#define	GWF_INTERR	2
#define	GWF_CALLERR	3

/*
**  Flags to be send to gwu_deltcb() routine, indicating whether or not the
**  tcb to be deleted must exist.
*/

#define	GWU_TCB_MANDATORY   ((i4) 1)
#define	GWU_TCB_OPTIONAL    ((i4) 2)

#define	GWF_ERROR			(DB_GWF_ID << 16)
#define	E_GW_MASK			GWF_ERROR

#define	E_GW0000_OK			E_DB_OK

/* Special placeholder error that says "error already issued", for
** pass-back thru DMF back to QEF.
*/
#define E_GW0023_USER_ERROR		    (E_GW_MASK | 0x0023)

/*
**  Exit errors -- these error codes are returned by the specific gateway
**  routines on failure.
**	0100 - 01FF
*/
#define	E_GW0100_GWX_VTERM_ERROR	    (E_GW_MASK | 0x0100)
#define	E_GW0101_GWX_VINFO_ERROR	    (E_GW_MASK | 0x0101)
#define	E_GW0102_GWX_VTABF_ERROR	    (E_GW_MASK | 0x0102)
#define	E_GW0103_GWX_VIDXF_ERROR	    (E_GW_MASK | 0x0103)
#define	E_GW0104_GWX_VOPEN_ERROR	    (E_GW_MASK | 0x0104)
#define	E_GW0105_GWX_VCLOSE_ERROR	    (E_GW_MASK | 0x0105)
#define	E_GW0106_GWX_VPOSITION_ERROR	    (E_GW_MASK | 0x0106)
#define	E_GW0107_GWX_VGET_ERROR		    (E_GW_MASK | 0x0107)
#define	E_GW0108_GWX_VPUT_ERROR		    (E_GW_MASK | 0x0108)
#define	E_GW0109_GWX_VREPLACE_ERROR	    (E_GW_MASK | 0x0109)
#define	E_GW010A_GWX_VDELETE_ERROR	    (E_GW_MASK | 0x010A)
#define	E_GW010B_GWX_VBEGIN_ERROR	    (E_GW_MASK | 0x010B)
#define	E_GW010C_GWX_VABORT_ERROR	    (E_GW_MASK | 0x010C)
#define	E_GW010D_GWX_VCOMMIT_ERROR	    (E_GW_MASK | 0x010D)
#define	E_GW010E_GWX_VATCB_ERROR	    (E_GW_MASK | 0x010E)
#define	E_GW010F_GWX_VDTCB_ERROR	    (E_GW_MASK | 0x010F)
#define	E_GW0110_GWX_VINIT_ERROR	    (E_GW_MASK | 0x0110)
#define	E_GW0111_GWX_VCONNECT_ERROR	    (E_GW_MASK | 0x0111)

/*
**  GWF errors -- these errors are returned by generic gateway routines on
**  failure.
**	0200 - 02FF
*/
#define	E_GW0200_GWF_INIT_ERROR		    (E_GW_MASK | 0x0200)
#define	E_GW0201_GWF_TERM_ERROR		    (E_GW_MASK | 0x0201)
#define	E_GW0202_GWS_INIT_ERROR		    (E_GW_MASK | 0x0202)
#define	E_GW0203_GWS_TERM_ERROR		    (E_GW_MASK | 0x0203)
#define	E_GW0204_GWT_REGISTER_ERROR	    (E_GW_MASK | 0x0204)
#define	E_GW0205_GWT_REMOVE_ERROR	    (E_GW_MASK | 0x0205)
#define	E_GW0206_GWT_OPEN_ERROR		    (E_GW_MASK | 0x0206)
#define	E_GW0207_GWT_CLOSE_ERROR	    (E_GW_MASK | 0x0207)
#define	E_GW0208_GWI_REGISTER_ERROR	    (E_GW_MASK | 0x0208)
#define	E_GW0209_GWI_REMOVE_ERROR	    (E_GW_MASK | 0x0209)
#define	E_GW020A_GWR_POSITION_ERROR	    (E_GW_MASK | 0x020A)
#define	E_GW020B_GWR_GET_ERROR		    (E_GW_MASK | 0x020B)
#define	E_GW020C_GWR_PUT_ERROR		    (E_GW_MASK | 0x020C)
#define	E_GW020D_GWR_REPLACE_ERROR	    (E_GW_MASK | 0x020D)
#define	E_GW020E_GWR_DELETE_ERROR	    (E_GW_MASK | 0x020E)
#define	E_GW020F_GWX_BEGIN_ERROR	    (E_GW_MASK | 0x020F)
#define	E_GW0210_GWX_ABORT_ERROR	    (E_GW_MASK | 0x0210)
#define	E_GW0211_GWX_COMMIT_ERROR	    (E_GW_MASK | 0x0211)
#define	E_GW0212_GWU_INFO_ERROR		    (E_GW_MASK | 0x0212)
#define	E_GW0213_GWU_COPEN_ERROR	    (E_GW_MASK | 0x0213)
#define	E_GW0214_GWU_DELTCB_ERROR	    (E_GW_MASK | 0x0214)
#define	E_GW0215_GWU_NEWRSB_ERROR	    (E_GW_MASK | 0x0215)
#define	E_GW0216_GWU_DELRSB_ERROR	    (E_GW_MASK | 0x0216)
#define	E_GW0217_GWU_ATXN_ERROR		    (E_GW_MASK | 0x0217)
#define	E_GW0218_GWU_CDELETE_ERROR	    (E_GW_MASK | 0x0218)
#define	E_GW0219_GWU_GETTCB_ERROR	    (E_GW_MASK | 0x0219)
#define	E_GW021A_GWU_XATTR_BLD_ERROR	    (E_GW_MASK | 0x021A)
#define	E_GW021B_GWU_XIDX_BLD_ERROR	    (E_GW_MASK | 0x021B)
#define	E_GW021C_GWF_SVR_INFO_ERROR	    (E_GW_MASK | 0x021C)
#define	E_GW021D_GWU_MAP_IDX_REL_ERROR	    (E_GW_MASK | 0x021D)
#define	E_GW021E_GWU_MAP_IDX_ATT_ERROR	    (E_GW_MASK | 0x021E)
#define	E_GW021F_GWU_FIND_ASSOC_ERROR	    (E_GW_MASK | 0x021F)
#define	E_GW0220_GWU_BAD_XATTR_OFFSET	    (E_GW_MASK | 0x0220)
#define	E_GW0221_GWS_ALTER_ERROR	    (E_GW_MASK | 0x0221)

/*
**  Other facility errors -- error codes indicating failure of specific
**  function requests from other facilities.
**	0300 - 03FF
*/
#define	E_GW0300_SCU_MALLOC_ERROR	    (E_GW_MASK | 0x0300)
#define	E_GW0301_SCU_MFREE_ERROR	    (E_GW_MASK | 0x0301)
#define	E_GW0302_SCU_INFORMATION_ERROR	    (E_GW_MASK | 0x0302)
#define E_GW0303_SCC_ERROR_ERROR	    (E_GW_MASK | 0x0303)

#define	E_GW0310_ULM_STARTUP_ERROR	    (E_GW_MASK | 0x0310)
#define	E_GW0311_ULM_SHUTDOWN_ERROR	    (E_GW_MASK | 0x0311)
#define	E_GW0312_ULM_OPENSTREAM_ERROR	    (E_GW_MASK | 0x0312)
#define	E_GW0313_ULM_CLOSESTREAM_ERROR	    (E_GW_MASK | 0x0313)
#define	E_GW0314_ULM_PALLOC_ERROR	    (E_GW_MASK | 0x0314)

#define	E_GW0320_DMR_PUT_ERROR		    (E_GW_MASK | 0x0320)
#define	E_GW0321_DMR_GET_ERROR		    (E_GW_MASK | 0x0321)
#define	E_GW0322_DMR_DELETE_ERROR	    (E_GW_MASK | 0x0322)
#define	E_GW0323_DMR_POSITION_ERROR	    (E_GW_MASK | 0x0323)
#define	E_GW0324_DMT_OPEN_ERROR		    (E_GW_MASK | 0x0324)
#define	E_GW0325_DMT_CLOSE_ERROR	    (E_GW_MASK | 0x0325)
#define	E_GW0326_DMT_SHOW_ERROR		    (E_GW_MASK | 0x0326)
#define	E_GW0327_DMF_DEADLOCK		    (E_GW_MASK | 0x0327)

#define	E_GW0330_CSP_SEMAPHORE_ERROR	    (E_GW_MASK | 0x0330)
#define	E_GW0331_CSV_SEMAPHORE_ERROR	    (E_GW_MASK | 0x0331)

#define	E_GW0340_ADE_CX_SPACE_ERROR	    (E_GW_MASK | 0x0340)

/*
** schang : user interrupt support
*/
#define E_GW0341_USER_INTR                  (E_GW_MASK | 0X0341)
/*
** E_GW0342_RPTG_OUT_OF_DATA never generates an error message
*/
#define E_GW0342_RPTG_OUT_OF_DATA           (E_GW_MASK | 0X0342)
/*
**  Bad parameter errors -- used for sanity checks on parameters, control block
**  values, etc.  These are usually coding errors.
**	0400 - 04FF
*/
#define	E_GW0400_BAD_GWID		    (E_GW_MASK | 0x0400)
#define	E_GW0401_NULL_RSB		    (E_GW_MASK | 0x0401)
#define	E_GW0402_RECORD_ACCESS_CONFLICT	    (E_GW_MASK | 0x0402)
#define E_GW0403_NULL_SESSION_ID	    (E_GW_MASK | 0x0403)

/*
**  Informational -- used to return information to caller, e.g. to inform DMF
**  that no gateway is enabled.
**	0500 - 05FF
*/
#define	E_GW0500_GW_TRANSACTIONS	    (E_GW_MASK | 0x0500)
#define	E_GW0501_NO_GW_TRANSACTIONS	    (E_GW_MASK | 0x0501)
#define	E_GW0502_NO_GATEWAY		    (E_GW_MASK | 0x0502)
#define	E_GW0503_NONEXISTENT_TABLE	    (E_GW_MASK | 0x0503)
#define	E_GW0504_LOCK_QUOTA_EXCEEDED	    (E_GW_MASK | 0x0504)
#define	E_GW0505_FILE_SECURITY_ERROR	    (E_GW_MASK | 0x0505)
#define	E_GW0506_FILE_SYNTAX_ERROR	    (E_GW_MASK | 0x0506)
#define	E_GW0507_FILE_UNAVAILABLE	    (E_GW_MASK | 0x0507)
#define E_GW0508_DEADLOCK		    (E_GW_MASK | 0x0508)
#define	E_GW0509_LOCK_TIMER_EXPIRED	    (E_GW_MASK | 0x0509)
#define	E_GW050A_RESOURCE_QUOTA_EXCEED	    (E_GW_MASK | 0x050A)
#define	E_GW050B_DUPLICATE_KEY		    (E_GW_MASK | 0x050B)
#define	E_GW050C_NOT_POSITIONED		    (E_GW_MASK | 0x050C)
#define E_GW050D_DATA_CVT_ERROR		    (E_GW_MASK | 0x050D)
#define E_GW050E_ALREADY_REPORTED	    (E_GW_MASK | 0x050E)
#define	E_GW050F_DEFAULT_STATS_USED	    (E_GW_MASK | 0x050F)

/*
**  Miscellaneous -- internally detected bugs, etc.
**	0600 - 06FF
*/
#define	E_GW0600_NO_MEM			    (E_GW_MASK | 0x0600)

#define	E_GW0610_BAD_TCB_CACHE		    (E_GW_MASK | 0x0610)
/*  #define	E_GW0611_CANT_LOCK_TCB_LIST	(E_GW_MASK + 0x0611)	*/
#define	E_GW0612_NULL_TCB_LIST		    (E_GW_MASK | 0x0612)
#define	E_GW0613_TCB_IN_USE		    (E_GW_MASK | 0x0613)
#define	E_GW0614_BAD_TCB_ALLOCATE	    (E_GW_MASK | 0x0614)

/*  #define	E_GW0620_XREL_OPEN_FAILED	(E_GW_MASK + 0x0620)	*/
/*  #define	E_GW0621_XATTR_OPEN_FAILED	(E_GW_MASK + 0x0621)	*/
/*  #define	E_GW0622_XIDX_OPEN_FAILED	(E_GW_MASK + 0x0622)	*/
#define	E_GW0623_XCAT_PUT_FAILED	    (E_GW_MASK | 0x0623)
/*  #define	E_GW0624_XCAT_GET_FAILED	(E_GW_MASK + 0x0624)	*/
/*  #define	E_GW0625_XCAT_REP_FAILED	(E_GW_MASK + 0x0625)	*/
/*  #define	E_GW0626_XCAT_DEL_FAILED	(E_GW_MASK + 0x0626)	*/
/*  #define	E_GW0627_GWX_VGET_ERROR		(E_GW_MASK + 0x0627)	*/
/*  #define	E_GW0628_NO_CATALOG_ID		(E_GW_MASK + 0x0628)	*/
/*  #define	E_GW0629_XCAT_OPEN_FAILED	(E_GW_MASK + 0x0629)	*/
/*  #define	E_GW062A_XCAT_POSITION_FAILED	(E_GW_MASK + 0x062A)	*/
/*  #define	E_GW062B_XREL_GET_FAILED	(E_GW_MASK + 0x062B)	*/
#define	E_GW062C_XREL_CLOSE_FAILED	    (E_GW_MASK | 0x062C)
/*  #define	E_GW062D_XATT_GET_FAILED	(E_GW_MASK + 0x062D)	*/
#define	E_GW062E_XATT_CLOSE_FAILED	    (E_GW_MASK | 0x062E)
#define	E_GW062F_WRONG_NUM_ATTRIBUTES	    (E_GW_MASK | 0x062F)
/*  #define	E_GW0630_XIDX_GET_FAILED	(E_GW_MASK + 0x0630)	*/
#define	E_GW0631_XIDX_CLOSE_FAILED	    (E_GW_MASK | 0x0631)
#define	E_GW0632_WRONG_NUM_INDEXES	    (E_GW_MASK | 0x0632)
#define	E_GW0633_CANT_FIND_INDEX	    (E_GW_MASK | 0x0633)
#define	E_GW0634_NULL_RECORD_STREAM	    (E_GW_MASK | 0x0634)

/*  #define	E_GW0640_TX_BEGIN_FAILED	(E_GW_MASK + 0x0640)	*/
#define	E_GW0641_END_OF_STREAM		    (E_GW_MASK | 0x0641)
/*  #define	E_GW0642_BAD_GATEWAY_SHUTDOWN	(E_GW_MASK + 0x0642)	*/
/*  #define	E_GW0643_ERROR_DEALLOCATING_RSB	(E_GW_MASK + 0x0643)	*/
/*  #define	E_GW0644_TABLE_REGISTER_FAILED	(E_GW_MASK + 0x0644)	*/

/*  #define	E_GW0650_BAD_CLOSE		(E_GW_MASK + 0x0650)	*/
/*  #define	E_GW0651_BAD_XATT_FORM		(E_GW_MASK + 0x0651)	*/
/*  #define	E_GW0652_BAD_IDX_SOURCE		(E_GW_MASK + 0x0652)	*/
/*  #define	E_GW0653_BAD_REL_SOURCE		(E_GW_MASK + 0x0653)	*/
#define	E_GW0654_BAD_GW_VERSION		    (E_GW_MASK | 0x0654)
/*  #define	E_GW0655_BAD_OPEN		(E_GW_MASK + 0x0655)	*/

#define	E_GW0660_BAD_OPERATION_CODE	    (E_GW_MASK | 0x0660)
#define	E_GW0661_BAD_CB_TYPE		    (E_GW_MASK | 0x0661)
#define	E_GW0662_BAD_CB_LENGTH		    (E_GW_MASK | 0x0662)
#define	E_GW0663_INTERNAL_ERROR		    (E_GW_MASK | 0x0663)
#define	E_GW0664_UNKNOWN_OPERATION	    (E_GW_MASK | 0x0664)
#define	E_GW0665_UNKNOWN_EXCEPTION	    (E_GW_MASK | 0x0665)
#define	E_GW0666_BAD_FLAG		    (E_GW_MASK | 0x0666)
#define	E_GW0667_BAD_CHAR_VALUE		    (E_GW_MASK | 0x0667)
#define E_GW0668_UNEXPECTED_EXCEPTION       (E_GW_MASK | 0x0668)

/*
**  User errors -- non-fatal errors to be returned to the user.
**	0700 - 07FF
*/

/* none yet... */

/*
** Special gateway error types.  E_GW0800_GWF_USERERR gets special handling,
** similar to how E_PS0001_USERERR is handled.
*/
/*  #define	E_GW0800_USER_ERROR		(E_GW_MASK + 0x0800)	*/
/*  #define	E_GW0801_INTERNAL_ERROR		(E_GW_MASK + 0x0801)	*/
/*  #define	E_GW0802_CALLER_ERROR		(E_GW_MASK + 0x0802)	*/
/*  #define	E_GW0803_INT_OTHER_FAC_ERROR	(E_GW_MASK + 0x0803)	*/
/*  #define	E_GW0804_BADERRTYPE		(E_GW_MASK + 0x0804)	*/
/*
************************ SXA (C2) GATEWAY ERRORS ****************************
**
*/

#define E_GW4001_SXA_NO_INDEXES 	(E_GW_MASK + 0x4001)
#define E_GW4002_SXA_NO_UPDATE 	(E_GW_MASK + 0x4002)
#define E_GW4003_SXA_SCAN_ONLY 	(E_GW_MASK + 0x4003)
#define E_GW4004_SXA_NO_REGISTER_UPDATE 	(E_GW_MASK + 0x4004)
#define E_GW4005_SXA_NO_REGISTER_JNL 		(E_GW_MASK + 0x4005)
#define E_GW4006_SXA_REGISTER_NO_SEC	 	(E_GW_MASK + 0x4006)
#define E_GW4007_SXA_REGISTER_BAD_TYPE	 	(E_GW_MASK + 0x4007)
#define E_GW4008_SXA_REGISTER_MISMATCH 		(E_GW_MASK + 0x4008)
#define E_GW4009_SXA_REGISTER_BAD_COLUMN 	(E_GW_MASK + 0x4009)
#define E_GW4010_SXA_OPEN_NO_SECURITY 	(E_GW_MASK + 0x4010)
#define E_GW4011_SXA_OPEN_NO_INDEXES    (E_GW_MASK + 0x4011)
#define E_GW4012_SXA_NO_AUDITING        (E_GW_MASK + 0x4012)
#define E_GW4050_SXA_NULL_RCB 		(E_GW_MASK + 0x4050)
#define E_GW4051_SXA_BAD_GW_ID 		(E_GW_MASK + 0x4051)
#define E_GW4052_SXA_BAD_XREL_LEN 	(E_GW_MASK + 0x4052)
#define E_GW4053_SXA_BAD_XATTR_LEN 	(E_GW_MASK + 0x4053)
#define E_GW4054_SXA_BAD_NUM_COLUMNS 	(E_GW_MASK + 0x4054)
#define E_GW4055_SXA_UNEXPECTED_GW_ATT 	(E_GW_MASK + 0x4055)
#define E_GW4056_SXA_CANT_GET_USER_PRIVS 	(E_GW_MASK + 0x4056)
#define E_GW4057_SXA_OPEN_WRITE 	(E_GW_MASK + 0x4057)
#define E_GW4058_SXA_OPEN_ERROR 	(E_GW_MASK + 0x4058)
#define E_GW4059_SXA_CLOSE_ERROR 	(E_GW_MASK + 0x4059)
#define E_GW405A_SXA_BAD_PALLOC         (E_GW_MASK + 0x405A)
#define E_GW4060_SXA_CLOSE_NOT_OPEN 	(E_GW_MASK + 0x4060)
#define E_GW4061_SXA_POSITION_NOT_OPEN 	(E_GW_MASK + 0x4061)
#define E_GW4062_SXA_POSITION_ERROR 	(E_GW_MASK + 0x4062)
#define E_GW4063_SXA_GET_NOT_OPEN 	(E_GW_MASK + 0x4063)
#define E_GW4064_SXA_READ_ERROR 	(E_GW_MASK + 0x4064)
#define E_GW4065_SXA_TO_INGRES_ERROR 	(E_GW_MASK + 0x4065)
#define E_GW4066_SXA_CVT_ERROR 		(E_GW_MASK + 0x4066)
#define E_GW4067_SXA_NO_REGISTER_KEYED  (E_GW_MASK + 0x4067)
#define E_GW4077_SXA_BAD_ATTID 		(E_GW_MASK + 0x4077)
#define E_GW4078_SXA_BAD_SCF_INFO 	(E_GW_MASK + 0x4078)
#define E_GW4079_SXA_NULL_EXTENDED_RSB  (E_GW_MASK + 0x4079)
#define E_GW407A_SXA_BAD_MO_ATTACH      (E_GW_MASK + 0x407A)
#define E_GW407B_SXA_LOG_NOT_EXIST	(E_GW_MASK + 0x407B)
#define E_GW407C_SXA_GET_BY_TID         (E_GW_MASK + 0x407C)
#define E_GW407D_SXA_NO_PRIV		(E_GW_MASK + 0x407D)
/*
************************ RMS GATEWAY ERRORS *********************************
** RMS Gateway errors. These error codes are generated by the RMS Gateway
** exits. Some of these error codes are checked for by GWF and mapped back
** into 'appropriate' GWF error codes for return to DMF. No error code
** requiring parameters is returned to GWF; all errors requiring parameter
** substitution are logged directly by the RMS Gateway exits.
**
** NOTE: RMS Gateway exits also may return one of the 'standard' exit errors
**	 (e.g., E_GW0104_GWX_VOPEN_ERROR).
**
** Message numbers E_GW5000 through E_GW57FF are reserved for the RMS
** Gateway's use. Of these, messages E_GW5000 through E_GW53FF are reserved
** for internal RMS Gateway messages requiring full parameter substituion,
** while E_GW5400 through E_GW57FF are reserved for the so-called 'traceback'
** messages which are returned to GWF.
*/

/*
** RMS Gateway messages with parameter substitution (E_GW5000 - E_GW53FF):
*/
#define	E_GW5000_RMS_OPEN_ERROR			(E_GW_MASK | 0x5000)
#define	E_GW5001_RMS_CLOSE_ERROR		(E_GW_MASK | 0x5001)
#define	E_GW5002_RMS_CONNECT_ERROR	        (E_GW_MASK | 0x5002)
#define	E_GW5003_RMS_DISCONNECT_ERROR		(E_GW_MASK | 0x5003)
#define	E_GW5004_RMS_FIND_SEQ_ERROR		(E_GW_MASK | 0x5004)
#define	E_GW5005_RMS_FIND_KEY_ERROR		(E_GW_MASK | 0x5005)
#define	E_GW5006_RMS_GET_ERROR			(E_GW_MASK | 0x5006)
#define	E_GW5007_RMS_PUT_ERROR			(E_GW_MASK | 0x5007)
#define	E_GW5008_RMS_UPDATE_ERROR		(E_GW_MASK | 0x5008)
#define	E_GW5009_RMS_DELETE_ERROR		(E_GW_MASK | 0x5009)
#define	E_GW500A_RMS_FLUSH_ERROR		(E_GW_MASK | 0x500A)
#define	E_GW500B_RMS_FILE_NOT_INDEXED		(E_GW_MASK | 0x500B)
#define	E_GW500C_RMS_CVT_TO_ING_ERROR		(E_GW_MASK | 0x500C)
#define	E_GW500D_CVT_TO_ING_ERROR		(E_GW_MASK | 0x500D)
#define	E_GW500E_CVT_TO_RMS_ERROR		(E_GW_MASK | 0x500E)
#define	E_GW500F_CHECK_ACCESS_ERROR		(E_GW_MASK | 0x500F)
#define	E_GW5010_UNSUPPORTED_FILE_TYPE		(E_GW_MASK | 0x5010)
#define	E_GW5011_CVT_KEY_TO_RMS_ERROR		(E_GW_MASK | 0x5011)
#define	E_GW5012_RMS_REWIND_ERROR		(E_GW_MASK | 0x5012)
#define	E_GW5013_RMS_BAD_INDEX_VALUE		(E_GW_MASK | 0x5013)
#define	E_GW5014_RMS_RFA_TOO_LONG		(E_GW_MASK | 0x5014)

/*
** RMS Gateway traceback messages, E_GW5400 through E_GW57FF:
*/
#define	E_GW5400_RMS_FILE_ACCESS_ERROR		(E_GW_MASK | 0x5400)
#define	E_GW5401_RMS_FILE_ACT_ERROR		(E_GW_MASK | 0x5401)
#define	E_GW5402_RMS_BAD_DEVICE_TYPE		(E_GW_MASK | 0x5402)
#define	E_GW5403_RMS_BAD_DIRECTORY_NAME		(E_GW_MASK | 0x5403)
#define	E_GW5404_RMS_OUT_OF_MEMORY		(E_GW_MASK | 0x5404)
#define	E_GW5405_RMS_DIR_NOT_FOUND		(E_GW_MASK | 0x5405)
#define	E_GW5406_RMS_FILE_LOCKED		(E_GW_MASK | 0x5406)
#define	E_GW5407_RMS_FILE_NOT_FOUND		(E_GW_MASK | 0x5407)
#define	E_GW5408_RMS_BAD_FILE_NAME		(E_GW_MASK | 0x5408)
#define	E_GW5409_RMS_LOGICAL_NAME_ERROR		(E_GW_MASK | 0x5409)
#define	E_GW540A_RMS_FILE_PROT_ERROR		(E_GW_MASK | 0x540A)
#define	E_GW540B_RMS_FILE_SUPERSEDED		(E_GW_MASK | 0x540B)
#define	E_GW540C_RMS_ENQUEUE_LIMIT		(E_GW_MASK | 0x540C)
#define	E_GW540D_RMS_DEADLOCK			(E_GW_MASK | 0x540D)
#define	E_GW540E_RMS_ALREADY_LOCKED		(E_GW_MASK | 0x540E)
#define	E_GW540F_RMS_PREVIOUSLY_DELETED		(E_GW_MASK | 0x540F)
#define	E_GW5410_RMS_READ_LOCKED_RECORD		(E_GW_MASK | 0x5410)
#define	E_GW5411_RMS_RECORD_NOT_FOUND		(E_GW_MASK | 0x5411)
#define	E_GW5412_RMS_READ_AFTER_WAIT		(E_GW_MASK | 0x5412)
#define	E_GW5413_RMS_RECORD_LOCKED		(E_GW_MASK | 0x5413)
#define	E_GW5414_RMS_TIMED_OUT			(E_GW_MASK | 0x5414)
#define	E_GW5415_RMS_RECORD_TOO_BIG		(E_GW_MASK | 0x5415)
#define	E_GW5416_RMS_INVALID_DUP_KEY		(E_GW_MASK | 0x5416)
#define	E_GW5417_RMS_DEVICE_FULL		(E_GW_MASK | 0x5417)
#define	E_GW5418_RMS_DUPLICATE_KEY		(E_GW_MASK | 0x5418)
#define	E_GW5419_RMS_INDEX_UPDATE_ERROR		(E_GW_MASK | 0x5419)
#define	E_GW541A_RMS_NO_CURRENT_RECORD		(E_GW_MASK | 0x541A)
#define	E_GW541B_RMS_INTERNAL_MEM_ERROR		(E_GW_MASK | 0x541B)
#define	E_GW541C_RMS_DELETE_FROM_SEQ		(E_GW_MASK | 0x541C)
#define E_GW541D_RMS_INV_REC_SIZE		(E_GW_MASK | 0x541D)
#define E_GW541E_RMS_INV_UPD_SIZE		(E_GW_MASK | 0x541E)

/*
** Messages E_GW5480 - E_GW549F are reserved to rmsmap.c:
*/
#define	E_GW5480_RMSCX_TO_INGRES_ERROR		(E_GW_MASK | 0x5480)
#define	E_GW5481_RMSCX_TCB_EST_ERROR		(E_GW_MASK | 0x5481)
#define	E_GW5482_RMS_TO_INGRES_ERROR		(E_GW_MASK | 0x5482)
#define	E_GW5483_RMS_FROM_INGRES_ERROR		(E_GW_MASK | 0x5483)
#define E_GW5484_RMS_DATA_CVT_ERROR		(E_GW_MASK | 0x5484)
#define E_GW5485_RMS_RECORD_TOO_SHORT		(E_GW_MASK | 0x5485)

/*
** Messages E_GW54A0 - E_GW54AF are reserved to gw02rms.c
** These should probably be renamed to contain _RMS_ ...
*/
#define	E_GW54A0_WRONG_NUM_PARMS		(E_GW_MASK | 0x54A0)
#define	E_GW54A1_NO_SUCH_DATATYPE		(E_GW_MASK | 0x54A1)
#define	E_GW54A2_INVALID_INTEGER		(E_GW_MASK | 0x54A2)
#define E_GW54A3_BAD_XFMT_PARAM			(E_GW_MASK | 0x54A3)
#define	E_GW54A4_RMS_NO_DEFAULT			(E_GW_MASK | 0x54A4)
#define	E_GW54A5_BAD_RECORD_LENGTH		(E_GW_MASK | 0x54A5)
#define	E_GW54A6_BAD_FORMAT_STRING		(E_GW_MASK | 0x54A6)

/*
**  RMS-specific datatype conversion errors are E_GW7000 - E_GW7999 and are
**  defined in gwrms_vms!gwrmserr.h.
*/

/*
** GWM (IMA) errors are 8000-8fff.  These are copied directly from
** ergwf.h, derived from ergwf.msg.
*/
#define E_GW8000_GO_BAD_XREL_BUF       0x000f8000
#define E_GW8001_GO_BAD_NUMATTS        0x000f8001
#define E_GW8100_GM_POS_ERROR          0x000f8100
#define E_GW8101_GM_ALLOC_ERROR        0x000f8101
#define E_GW8200_GX_BAD_XREL_BUF       0x000f8200
#define E_GW8201_GX_GET_ERR            0x000f8201
#define E_GW8301_LOST_USER_PLACE       0x000f8301
#define E_GW8302_LOST_PLACE            0x000f8302
#define E_GW8303_BAD_VNODE_FROM_PLACE  0x000f8303



/*}
**  Name:   GW_DMP_ATTR	--  analog of DMP_ATTRIBUTE
**
**  This structure is at the moment an exact analog of a DMP_ATTRIBUTE
**  structure, defined locally to DMF in dmf!hdr!dmp.h.  We can't just use that
**  one due to the separation of facilities.  Possible approaches are to change
**  build tools (on all environments) such that compile-time search lists see
**  DMF headers as well as GWF headers when compiling GWF modules.  This makes
**  some sense, since conceptually GWF is a physical layer "underneath" DMF;
**  also it would allow us to stay in synch without effort.  Or perhaps the
**  relevant code (in gwf!gwf!gwfidx.c) can be moved to dmf!dmf!dmfgw.c, so
**  that it isn't a problem.  For now I have chosen to keep this analog
**  structure.
**
**  History:
**	sometime (Linda, I think)
**	    Creation.
**	26-oct-92 (rickh)
**	    Reduced this to a typedef.  Moved _DM_COLUMN to DMF.H so it
**	    is visible outside DMF.  Someday, someone can really fix this.
*/
typedef struct _DM_COLUMN	GW_DMP_ATTR;

/* 23-jul-93 (schang)
**     function prototype
*/

FUNC_EXTERN DB_STATUS gwf_call();
FUNC_EXTERN DB_STATUS gwf_trace();

/* Name of the special place object for use in 'is' clauses which appear
** in an array below for use in the parser
*/

# define GM_VNODE_STR           "VNODE"
# define GM_SERVER_STR          "SERVER"
# define GM_CLASSID_STR         "CLASSID"
# define GM_INSTANCE_STR        "INSTANCE"
# define GM_VALUE_STR           "VALUE"
# define GM_PERMS_STR           "PERMISSIONS"

# define                GMA_UNKNOWN     0        /* unknown */
# define                GMA_CLASSID     1       /* att is a classid */
# define                GMA_PLACE       2       /* att is full place */
# define                GMA_VNODE       4       /* att is vnode of place */
# define                GMA_SERVER      8       /* att is a server place */
# define                GMA_INSTANCE    16      /* att is mob instance */
# define                GMA_VALUE       32      /* att is mob value */
# define                GMA_PERMS       64      /* att is mob perms */
# define                GMA_USER        128     /* att is user classid */

#if defined GM_WANT_ARRAY
/*}
** Name:        GM_ATT_DESC     -- descriptions of atts in object table.
**
** Description:
**      Describes the attributes in an object table.
**
** History:
**      29-sep-92 (daveb)
**          Created.
**      25-Nov-1992 (daveb)
**          renamed from GO_ATT_ENTRY, moved to gwmutil.
**      1-Feb-1993 (daveb)
**          Remove "PLACE" definition in GM_atts; it isn't legal anymore.
**      18-Jan-2007 (kibro01)
**          Moved to back/hdr/gwf.h so it can be examined in parsing
*/

typedef struct
{
        char    *name;          /* for xatt */
        char    namelen;
        i4      type;           /* for GM_ATT_ENTRY */
} GM_ATT_DESC;

static GM_ATT_DESC GM_atts[] =
{
    { GM_VNODE_STR,     sizeof(GM_VNODE_STR) - 1,       GMA_VNODE },
    { GM_SERVER_STR,    sizeof(GM_SERVER_STR) - 1,      GMA_SERVER },
    { GM_CLASSID_STR,   sizeof(GM_CLASSID_STR) - 1,     GMA_CLASSID },
    { GM_INSTANCE_STR,  sizeof(GM_INSTANCE_STR) - 1,    GMA_INSTANCE },
    { GM_VALUE_STR,     sizeof(GM_VALUE_STR) - 1,       GMA_VALUE },
    { GM_PERMS_STR,     sizeof(GM_PERMS_STR) - 1,       GMA_PERMS },
    { 0, 0, 0 }
};
#endif

