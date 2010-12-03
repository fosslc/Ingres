/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** gwmint.h -- internal header for IMA SQL MIB server.
**
** The gateway is defined in the following files:
**
**	gwmint.h	-- this file, common to all
**	gwmima.c	-- main entry points for gwf, dispatches to below.
**	gwmmob.c	-- iimib_objects table
**	gwmxtab.c	-- other tables, dynamically defined.
**	gwmgcm.c	-- GCM request handling
**	gwmutil.c	-- misc utility functions.
**
** History:
**	wherever (daveb)
**	    Created.
**	30-sep-92 (daveb)
**	    Renamed everything for 7 char uniqueness.
**	    Add GM_XTAB_VNODE and GM_XTAB_SERVER.
**	    Revise GM_ATT_ENTRY, a make pointer to an
**	    array of them part of the GM_RSB.
**	9-oct-92 (daveb)
**	    Broke place stuff into gwmplace.h so everyone wouldn't
**	    need to include <gca.h>
**	11-Nov-1992 (daveb)
**	    get rid of cursor in GM_RSB.
**	14-Nov-1992 (daveb)
**	    add gma_att_offset to simplfy tuple packing.  It's
**	    figured out at open time. (GM_ATT_ENTRY).
**	14-Nov-1992 (daveb)
**	    make gwm_def_vnode and new gwm_this_server be buffers,
**	    not just pointers. (GM_GLOBALS)
**	24-Nov-1992 (daveb)
**	    Figure the guises once and stash them in the GM_RSB here.
**	25-Nov-1992 (daveb)
**	    To generalize attr handling, define complete list of
**	    GM_xxx_STRs, and provide GM_check_att and GM_att_type
**	    in gwmutil.c
**	3-Dec-1992 (daveb)
**	    Add place default string to globals.
**	8-Dec-1992 (daveb)
**	    more error code defs here from gwf.h; they are
**	    still, unfortunately, copied from ergwf.h.
**	1-Feb-1993 (daveb)
**	    Remove GM_PLACE_STR definition; it isn't legal anymore.
**	    Update GM_ATT_ENTRY documentation to match the code.
**	12-Mar-1993 (daveb)
**	    Remove externs of GM_st_partial_key and GM_st_key.
**	    They're static now.
**	5-Jan-1994 (daveb)
**	    Add GM_get_pmsym for bug 58358.
**      21-Jan-1994 (daveb) 59125
**          Add tracing variables and objects.
**      11-May-1994 (daveb) 63344
**  	    Add messages 834B and 834C for more detectable register errs.
**	7-march-1997 (angusm)
**	    Increase length of GM_MIB_PLACE_LEN so that 
**	    GM_GLOBALS.gwm_def_domain, GM_GLOBALS.gwm_this_server 
**	    do not overflow for hostnames > 24 chars 
**	    e.g. hostnames with domains and stuff. (bug 75657).
**      24-Jul-1997 (stial01)
**          Added gwm_gchdr_size to II_GM_GLOBALS
**      13-May-98 (fanra01)
**          Addd prototype for new function GM_chk_priv.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-jan-2007 (kibro01)
**	    Removed special 'is' clause list to put in back/hdr/hdr/gwf.h
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**     22-apr-2010 (stial01)
**          Fixed GM_XATT_TUPLE(iigw07_attribute), GM_XIND_TUPLE(iigw07_index)
**          Use DB_EXTFMT_SIZE for register table newcolname IS 'ext_format'
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing function prototypes.
*/

/*----------------------------------------------------------------
**  
** Manifest constants
*/

/* value on max key lens -- FIXME values are wrong, but handy now. */

# define GM_MIB_PLACE_LEN	DB_EXTFMT_SIZE
# define GM_MIB_CLASSID_LEN	DB_EXTFMT_SIZE
# define GM_MIB_INSTANCE_LEN	DB_EXTFMT_SIZE
# define GM_MIB_VALUE_LEN	DB_MAXTUP

/* Number of pages to say any oepn gateway table has at open */

# define GM_DEF_PAGE_CNT	1000

/* flag args to GM_XXerror functions */

# define GM_ERR_INTERNAL	0x0001 /* send to dbms.log */
# define GM_ERR_LOG		0x0002 /* ERsend it to errlog.log */
# define GM_ERR_USER		0x0004 /* send back to the user */

/*
** These error message ids are directly from ergwf.h,
** which is authoritative.
*/
#define E_GW8041_GCA_FORMAT            0x000f8041
#define E_GW8042_GCA_CALL              0x000f8042
#define E_GW8043_CS_SUSPEND            0x000f8043
#define E_GW8044_NO_PEER               0x000f8044
#define E_GW8045_GCA_COMPLETION        0x000f8045
#define E_GW8046_BAD_OP                0x000f8046
#define E_GW8081_GCN_ERROR             0x000f8081
#define E_GW8082_GCN_UNPACK            0x000f8082
#define E_GW80C1_BAD_FROM_CLAUSE       0x000f80c1
#define E_GW80C2_IDX_BAD_FROM_CLAUSE   0x000f80c2
#define E_GW80C3_INGRES_NOT_DBA        0x000f80c3
#define E_GW80C4_NO_SUB_GATEWAY        0x000f80c4
#define E_GW80C5_INGRES_MUST_REGISTER  0x000f80c5
#define E_GW80C6_ERRMAP_ERR_BUT_OK     0x000f80c6
#define E_GW80C7_SHOULD_HAVE_SEEN      0x000f80c7
#define E_GW80C8_ERRMAP_BAD_STATUS     0x000f80c8
#define E_GW8141_NULL_DMT_ATTR         0x000f8141
#define E_GW8142_BAD_INT_SIZE          0x000f8142
#define E_GW8143_BAD_TYPE              0x000f8143
#define E_GW8144_BAD_KEYATTR           0x000f8144
#define E_GW8145_BAD_ATTR_COUNT        0x000f8145
#define E_GW8146_GM_POS_ERROR          0x000f8146
#define E_GW8181_OBJ_MISSING_IS_CLAUSE 0x000f8181
#define E_GW8182_OBJ_BAD_IS_CLAUSE     0x000f8182
#define E_GW8183_OBJ_BAD_COL_TYPE      0x000f8183
#define E_GW8184_BAD_XREL_BUF          0x000f8184
#define E_GW8185_BAD_NUMATTS           0x000f8185
#define E_GW8186_BAD_ATTR_TYPE         0x000f8186
#define E_GW8187_BAD_XATT_BUF          0x000f8187
#define E_GW8188_NOT_PLACE             0x000f8188
#define E_GW8189_NOT_CLASSID           0x000f8189
#define E_GW818A_NOT_INSTANCE          0x000f818a
#define E_GW818B_NOT_UNIQUE            0x000f818b
#define E_GW818C_IS_UNIQUE             0x000f818c
#define E_GW818D_NOT_PLACE_CLASSID     0x000f818d
#define E_GW818E_NO_KEYS               0x000f818e
#define E_GW818F_TOO_MANY_KEYS         0x000f818f
#define E_GW81C1_ROP_ERROR             0x000f81c1
#define E_GW81C2_CVAL_ERR              0x000f81c2
#define E_GW81C3_UNEXPECTED_ATTR_TYPE  0x000f81c3
#define E_GW8201_LOST_PLACE            0x000f8201
#define E_GW8202_QUERY_VNODE_ERR       0x000f8202
#define E_GW8203_VNODE_IN_SERVER       0x000f8203
#define E_GW8204_SELF_NOT_INSTALL      0x000f8204
#define E_GW8205_UNEXPECTED_DOM_TYPE   0x000f8205
#define E_GW82C1_NO_SCB                0x000f82c1
#define E_GW82C2_NO_DOMAIN_START       0x000f82c2
#define E_GW8301_NO_LISTEN_ADDR        0x000f8301
#define E_GW8302_ALLOC_ERROR           0x000f8302
#define E_GW8303_DBA_ERROR             0x000f8303
#define E_GW8304_USER_ERROR            0x000f8304
#define E_GW8305_SESS_ALLOC_ERROR      0x000f8305
#define E_GW8306_USER_PRIV_ERR         0x000f8306
#define E_GW8307_DBNAME_ERR            0x000f8307
#define E_GW8341_TAB_MISSING_IS_CLAUSE 0x000f8341
#define E_GW8342_TAB_BAD_IS_CLAUSE     0x000f8342
#define E_GW8344_GX_GET_PART_ERR       0x000f8344
#define E_GW8345_GX_GETNEXT_ERR        0x000f8345
#define E_GW8346_NEXT_PLACE_ERR        0x000f8346
#define E_GW8347_GX_GET_OK_STAT_ERR    0x000f8347
#define E_GW8348_NOT_USER              0x000f8348
#define E_GW8349_NOT_PLACE_USER        0x000f8349
#define E_GW834A_TAB_TOO_MANY_KEYS     0x000f834a
#define E_GW834B_INVALID_XTAB_IS_CLAUSE     0x000f834b
#define E_GW834C_XTAB_NO_USER_ATT     0x000f834c
#define E_GW8381_INCONSISTENT_ROW      0x000f8381



/*----------------------------------------------------------------
**
** Structures
*/

/* forward refs */

typedef struct II_GM_GLOBALS	GM_GLOBALS;
typedef struct II_GM_RSB	GM_RSB;
typedef struct II_GM_OPBLK	GM_OPBLK;

/* this defined in gwmplace.h */

typedef struct II_GM_CONN_BLK	GM_CONN_BLK;

/*----------------
**
** System Catalog related
*/

/*}
** Name:	GM_XREL_TUPLE
**
** Description:
**
**	The iigwXX_relation tuple -- len must be multiple of 4,
**	or won't align!
**
**	xrel_tblid	the id of the table (key).
**
**	xrel_flags	flags for the table
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
**	1-Feb-1993 (daveb)
**	    documented GM_XREL_TUPLE better.  We now use xrel_flags
**	    to verify table registration options (ugh), and to
**	    identify the target sub-gateway.
*/
typedef struct
{
    DB_TAB_ID	xrel_tblid;
    i4		xrel_flags;
#	define GM_MOB_TABLE	(0x01)
#	define GM_XTAB_TABLE	(0x02)
    
#	define GM_UPDATE_OK	(0x10)
#	define GM_JOURNALED	(0x20)

#	define GM_SORTED	(0x100)
#	define GM_KEYED		(0x200)

#	define GM_UNIQUE	(0x1000)

} GM_XREL_TUPLE;

/*
**  length of a row in iigwXX_relation:
**	{ tblid, flags }
*/
# define GM_XREL_LEN	(sizeof( GM_XREL_TUPLE ))

/*}
** Name:	GM_XATT_TUPLE
**
** Description:
**
**	The iigwXX_attribute tuple -- len must be multiple of 4,
**	or won't align!
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/

typedef struct
{
    DB_TAB_ID	xatt_tblid;
    i2		xatt_attnum;
    char	xatt_classid[ GM_MIB_CLASSID_LEN ];
} GM_XATT_TUPLE;

/*
**  length of row in iigwXX_attribute:
**	{ tblid, attnum, classid }
*/
# define GM_XATTR_LEN  (sizeof( GM_XATT_TUPLE ))

/*}
** Name:	GM_XIDX_TUPLE
**
** Description:
**
**	The iigwXX_index tuple -- len must be multiple of 4,
**	or won't align!
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    DB_TAB_ID	xatt_tblid;
    i2		xatt_attnum;
    char	xatt_classid[ GM_MIB_CLASSID_LEN ];
} GM_XIDX_TUPLE;

/*
** length of row in iigwXX_index
*/
# define GM_XINDEX_LEN (sizeof( GM_XIDX_TUPLE ))


/*----------------------------------------------------------------
**
** Other types
*/

/* exit func, reverse engineered */

/*}
** Name:	GW_EXIT_FUNC  -- typedef for exit functions
**
** Description:
**
**	Each gateway exit is really this type.  Used
**	for declaring tables of pointers.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef DB_STATUS GW_EXIT_FUNC( GWX_RCB *gwx_rcb );

/*}
** Name:	GM_GW -- gateway identification
**
** Description:
**
**	Each RSB keeps a pointer to one of these as
**	a way to identify the proper callouts, either
**	for MOB tables or XTABs.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    GW_EXIT_FUNC	*init;	/* called on gw init */
    GWX_VECTOR		exit_table;
    char		*name;	/* 'objects' or 'tables' */
    i4			len;	/* name length */

} GM_GW;
   
/*}
** Name:	GM_ATT_ENTRY	-- place to store per-column stuff on open.
**
** Description:
**
**	An array 0..ncols of these are allocated and filled in
**	at table open time.  The array is indexed by key or column
**	number, starting at 0.
**
**	The fields are:
**
**	gma_att_offset	for col N, the byte offset in a tuple
**
**	gma_col_classid	For xtab tables, the string of the classid
**			for the column.  It may contain the reserved
**			strings for place, installation and server.
**			It contains pointers into the extended
**			attribute information handed in on open.
**			NOTE: these are NOT null terminated!
**
**	gma_dmt_att	Pointers to the ingres column information
**			for the column.  This is computed because we don't
**			know the order of the rows in gwm_ing_attrs.
**			Forcing the issue once at open time keeps up from
**			having to do it on every column reference.
**
**	gma_type	a GWM type for the column, classifying the 'is'
**			clause.
**
**	gma_key_offset	for key N, the offset in a key (not tuple)
**			buffer where the object may be found.  This
**			is computed by scanning the DMT_ATT_ENTRYs
**			for the table.
**
**	gma_key_type	gma_type of key N
**
**	gma_key_dmt_att	dmt att for the key N att
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
**	28-sep-92 (daveb)
**	    Changed around a lot.  Now used.
**	14-Nov-1992 (daveb)
**	    add gmt_att_offset to simplfy tuple packing.  It's
**	    figured out at open time.
**	8-Dec-1992 (daveb)
**	    Add gma_key_dmt_att to track key types.
**	1-Feb-1993 (daveb)
**	    Update GM_ATT_ENTRY documentation to match the code.
*/
typedef struct
{
    i4			gma_att_offset;		/* offset of att in a tuple */
    char		*gma_col_classid;	/* ptr to classid */
    DMT_ATT_ENTRY	*gma_dmt_att;		/* dmt att for tuple att */
    i4			gma_type;	/* Defines are in back/hdr/gwf.h */
    i4			gma_key_offset;		/* offset in key of this n */
    i4			gma_key_type;		/* gma_type of key n */
    DMT_ATT_ENTRY	*gma_key_dmt_att;	/* dmt att for key att */

} GM_ATT_ENTRY;


/*----------------------------------------------------------------
**
** Key related, see gwmkey.c
*/

/*}
** Name:	GM_ANY_KEY	-- general key type, for type punning.
**
** Description:
**
**	All other keys follow this structure, but have differering
**	length buffers.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    i4  len;
    char str[1];
} GM_ANY_KEY;

/*}
** Name:	GM_PLACE_GKEY	-- holds a place part of a key.
**
** Description:
**
**	A global key, used in GM_RSBs.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    i4	len;			/* actual len */
    char str[ GM_MIB_PLACE_LEN + 1 ];
} GM_PLACE_KEY;

/*}
** Name:	GM_CLASSID_KEY	-- holds classid part of a key.
**
** Description:
**
**	A global key, used in GM_RSBs.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    i4	len;			/* actual len */
    char str[ GM_MIB_CLASSID_LEN + 1 ];
} GM_CLASSID_KEY;

/*}
** Name:	GM_INST_KEY	-- holds instance part of a key.
**
** Description:
**
**	A global key, used in GM_RSBs.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    i4	len;			/* actual len */
    char str[ GM_MIB_INSTANCE_LEN + 1 ];
} GM_INST_KEY;

/* global key, used in RSBs */

/*}
** Name:	GM_GKEY	-- common GM key
**
** Description:
**
**	A global key, used in GM_RSBs.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
*/
typedef struct
{
    i4			valid; /* place, classid and instance are valid */
    GM_PLACE_KEY	place;
    GM_CLASSID_KEY	classid;
    GM_INST_KEY		instance;

} GM_GKEY;

/*----------------
**
** OPBLK related -- see gwmop.c
*/

/*}
** Name:	GM_OPBLK	-- operation block
**
** Description:
**
**	An internal block for stuffing requests 
**	and responses.
**
**	The format of an op block for various "op"s are:
**
**	GM_GET
**
**	GM_GETNEXT
**
**	GM_SET
**
**	GM_GETRESP
**
**	GM_SETRESP
**
** Members:
**	op		the operation in question, determines the format
**			of elements in the buffer.
**
**	status		cl status of the operation, OK if no error.
**
**	elements	number of elements in the buffer.
**
**	err_element	element that provoked value in status field.
**
**	curs_element	the element where the cursor points.
**
**	cursor		the current place in the buffer.
**
**	used		bytes used in the buffer.
**
**	place		the target place for the operation.
**
**	buf		the data area.
**
** History:
**	18-sep-92 (daveb)
**	    Documented.
**	11-Nov-1992 (daveb)
**	    get rid of cursor.
**	14-Nov-1992 (daveb)
**	    make gwm_def_vnode and new gwm_this_server be buffers,
**	    not just pointers.
**	19-Nov-1992 (daveb)
**	    Use GCA_FS_MAX_DATA for bufsiz, to have all the read-ahead
**	    we can get.
**	19-Nov-1992 (daveb)
**	    split op into req_op and res_op so we can be sure
**	    a response is a GETNEXT, and how many elements it
**	    is chunked by.
*/

# define GM_OP_BUFSIZ		GCA_FS_MAX_DATA

/* operations */

struct II_GM_OPBLK
{
    i4	op;			
#	define GM_BADOP		0
#	define GM_GET		1
#	define GM_GETNEXT	2
#	define GM_SET		3
#	define GM_GETRESP	4
#	define GM_SETRESP	5
    i4 status;		/* for lower levels */
    i4  elements;		/* response elements in the buf */
    i4  err_element;		/* which request element was in err */
    i4  curs_element;		/* where the cursor points */
    char *cursor;		/* where to get stuff, for cacheing */
    i4  used;			/* bytes used in buf */
    char place[ GM_MIB_PLACE_LEN ];
    char buf[ GM_OP_BUFSIZ ];
};

    
/*----------------------------------------------------------------
**
** GM specific RSB
*/


/*}
** Name:	GM_RSB - base resource block for open instance
**
** Description:
**	Contains state necessary for one "open" of a table made
**	visible by the IMA gateway.
**
** History:
**	19-feb-91 (daveb)
**	15-Jan-92 (daveb)
**	24-Nov-1992 (daveb)
**	    Figure the guises once and stash them in the GM_RSB here.
**	2-Dec-1992 (daveb)
**	    Rename place_dom to dom_type, move keys down to the
**	    end so it's nicer to dump in a debugger.
*/

struct II_GM_RSB
{
    GM_GW	*gw;			/* pointer to my gw desc */
    i4		guises;			/* my guises for this session */
    i4		dom_type;		/* type of place interation to do:
					   either GMA_VNODE or GMA_SERVER */
    bool	didfirst;		/* done anything with cur_key yet? */
    i4		ncols;			/* number of attributes */
    i4		nkeys;			/* number of key attributes. */

    /* move this to tbl_desc */
    GW_ATT_ENTRY	*gwm_att_list; /* ingres and xcat attributes */
    GM_ATT_ENTRY	*gwm_gwm_atts;	/* our local attribute info */

    GM_CONN_BLK		*gwm_conn;	/* active connection block */

    GM_GKEY	cur_key;		/* current key of interest */
    GM_GKEY	last_key;		/* last key of interest this place */

    GM_OPBLK	request;
    GM_OPBLK	response;

    /* any table specific data always follows */
};



/*----------------------------------------------------------------
**
** Global level
*/

/*}
** Name:	GM_GLOBALS - block of globals private to all GM.
**
** Description:
**
**	One instance of this exists globally visible to GM.
**
**	The places tree holds two types of nodes.  VNODEs identify
**	nodes known to us, and are pointed to by the SCF_DOMAIN
**	tree for each session.   These are followed by zero or
**	more SRVR blocks for the managed processes in that vnode.
**
**	gwm_tree_sem must be held when reading or updating the places tree.
**	An operation should not hold the semaphore when doing long-running
**	operations.
**
**	The gwm_stat_sem is used to protect statistics counters.
**
**	gwm_connects is a limit on the number of simutaneous connects that
**	may exist; it is to be protected by gwm_stat_sem.
**
** History:
**	18-sep-92 (daveb)
**	    Created.
**	23-Nov-1992 (daveb)
**	    change default ttl to 60 secs from 600 (10 minutes).
**	    New servers are invisible until ttl has passed, and
**	    10 minutes was too long.
**	3-Dec-1992 (daveb)
**	    Add place default string to globals.
**      21-Jan-1994 (daveb)
**          Add tracing variables and objects.
**      24-Jul-1997 (stial01)
**          Added gwm_gchdr_size to II_GM_GLOBALS
*/

struct II_GM_GLOBALS
{
    CS_SEMAPHORE	gwm_stat_sem;	/* lock statistic counters */
    CS_SEMAPHORE	gwm_places_sem;	/* lock vnodes and places */

    ULM_RCB		gwm_ulm_rcb;	/* for global allocation */

    SPTREE		gwm_places;	/* vnodes and srvrs, vnodes first */
    char		gwm_def_vnode[ GM_MIB_PLACE_LEN + 1 ];
    char		gwm_this_server[ GM_MIB_PLACE_LEN + 1 ];
    char		gwm_def_domain[ GM_MIB_PLACE_LEN + 1 ];
#	define GM_DOM_VNODE_STR		"\"VNODE\""
#	define GM_DOM_SERVER_STR	"\"SERVER\""

    i4			gwm_time_to_live; /* secs for servers to live */
# define	GM_DEFAULT_TTL	60
    i4			gwm_gcn_checks;
    i4			gwm_gcn_queries;

    CS_CONDITION	gwm_conn_cnd;	/* on places_sem, total conns. */

    i4			gwm_connects;	/* total outgoing connections */
    i4			gwm_max_conns;	/* limit on total outgoing */
    i4			gwm_max_place;	/* limit on conn to one place */
    PTR			gwm_gca_cb; /* GCA cb */

    /* tracing */
    bool		gwm_trace_terms;
    bool		gwm_trace_tabfs;
    bool		gwm_trace_idxfs;
    bool		gwm_trace_opens;
    bool		gwm_trace_closes;
    bool		gwm_trace_positions;
    bool		gwm_trace_gets;
    bool		gwm_trace_puts;
    bool		gwm_trace_replaces;
    bool		gwm_trace_deletes;
    bool		gwm_trace_begins;
    bool		gwm_trace_aborts;
    bool		gwm_trace_commits;
    bool		gwm_trace_infos;
    bool		gwm_trace_inits;
    bool		gwm_trace_errors;

    /* stats */
    i4		gwm_gcm_sends;
    i4		gwm_gcm_reissues;
    i4		gwm_gcm_errs;
    i4		gwm_gcm_readahead; /* rows to read-ahead */
    i4		gwm_gcm_usecache; /* look at readahead? */

    i4		gwm_op_rop_calls;
    i4		gwm_op_rop_hits;
    i4		gwm_op_rop_misses;

    i4		gwm_rreq_calls;
    i4		gwm_rreq_hits;
    i4		gwm_rreq_misses;

    i4		gwm_terms;
    i4		gwm_tabfs;
    i4		gwm_idxfs;
    i4		gwm_opens;
    i4		gwm_closes;
    i4		gwm_positions;
    i4		gwm_gets;
    i4		gwm_puts;
    i4		gwm_replaces;
    i4		gwm_deletes;
    i4		gwm_begins;
    i4		gwm_aborts;
    i4		gwm_commits;
    i4		gwm_infos;
    i4		gwm_inits;
    i4		gwm_errors;
};



/*----------------------------------------------------------------
**
** Session level
*/

/*}
** Name:	GM_SCB	- per session data for GWM
**
** Description:
**	Defines the user domain.  Memory for gs_domain nodes
**	comes from GM_alloc, and must be GM_free()-ed before
**	releasing the session block itself.
**
** History:
**	7-oct-92 (daveb)
**	    created.
*/
typedef struct
{
    CS_SEMAPHORE	gs_sem;		/* for locking this block & tree */
    SPTREE		gs_domain;	/* per-session domain tree */

} GM_SCB;



/*----------------------------------------------------------------
**  
**  Externs, global data and functions.
*/

/* gwmerr.c */

FUNC_EXTERN VOID GM_uerror ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type, i4  num_parm,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4,
			    i4 psize5, PTR pval5,
			    i4 psize6, PTR pval6,
			    i4 psize7, PTR pval7,
			    i4 psize8, PTR pval8,
			    i4 psize9, PTR pval9,
			    i4 psize10, PTR pval10 );

FUNC_EXTERN VOID GM_error( STATUS error_number );

FUNC_EXTERN VOID GM_0error( GWX_RCB *gwx_rcb, i4 error_no,
			   i4  error_type );

FUNC_EXTERN VOID GM_1error( GWX_RCB *gwx_rcb, i4 error_no,
			   i4  error_type,
			   i4 psize1, PTR pval1 );

FUNC_EXTERN VOID GM_2error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2 );

FUNC_EXTERN VOID GM_3error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3 );

FUNC_EXTERN VOID GM_4error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4 );

FUNC_EXTERN VOID GM_5error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type, 
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4,
			    i4 psize5, PTR pval5 );

FUNC_EXTERN VOID GM_6error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type, 
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4,
			    i4 psize5, PTR pval5,
			    i4 psize6, PTR pval6 );

FUNC_EXTERN VOID GM_7error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4,
			    i4 psize5, PTR pval5,
			    i4 psize6, PTR pval6,
			    i4 psize7, PTR pval7 );

FUNC_EXTERN VOID GM_8error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4,
			    i4 psize5, PTR pval5,
			    i4 psize6, PTR pval6,
			    i4 psize7, PTR pval7,
			    i4 psize8, PTR pval8 );

FUNC_EXTERN VOID GM_9error ( GWX_RCB *gwx_rcb, i4 error_no,
			    i4  error_type,
			    i4 psize1, PTR pval1,
			    i4 psize2, PTR pval2,
			    i4 psize3, PTR pval3,
			    i4 psize4, PTR pval4,
			    i4 psize5, PTR pval5,
			    i4 psize6, PTR pval6,
			    i4 psize7, PTR pval7,
			    i4 psize8, PTR pval8,
			    i4 psize9, PTR pval9 );


FUNC_EXTERN VOID GM_10error ( GWX_RCB *gwx_rcb, i4 error_no,
			     i4  error_type,
			     i4 psize1, PTR pval1,
			     i4 psize2, PTR pval2,
			     i4 psize3, PTR pval3,
			     i4 psize4, PTR pval4,
			     i4 psize5, PTR pval5,
			     i4 psize6, PTR pval6,
			     i4 psize7, PTR pval7,
			     i4 psize8, PTR pval8,
			     i4 psize9, PTR pval9,
			     i4 psize10, PTR pval10 );

/* gwmgcm.c */

FUNC_EXTERN VOID GM_cancel( GM_CONN_BLK *conn_blk );

FUNC_EXTERN DB_STATUS GM_gcm_msg( GM_RSB *rsb, STATUS *cl_stat );

/* gwmima.c */

GLOBALREF GM_GLOBALS GM_globals;

FUNC_EXTERN VOID GM_incr( CS_SEMAPHORE *sem, i4 *var );

/* gwmkey.c */

FUNC_EXTERN void GM_clearkey( GM_GKEY *key );

FUNC_EXTERN i4  GM_key_compare( GM_GKEY *this, GM_GKEY *that );

FUNC_EXTERN DB_STATUS GM_pos_sub( GWX_RCB *gwx_rcb, bool xtab );

FUNC_EXTERN void GM_dump_key( char *msg, GM_GKEY *key );

/* gwmmob.c */

FUNC_EXTERN GW_EXIT_FUNC GOinit;

/* gwmop.c */

FUNC_EXTERN DB_STATUS GM_op( i4  op,
			    GM_RSB *rsb,
			    i4  *lsbufp,
			    char *sbuf,
			    i4  *perms,
			    STATUS *cl_stat );

FUNC_EXTERN char *GM_keyfind( char *kclassid, char *kinstance,
			     GM_OPBLK *opblk, i4  *element );

FUNC_EXTERN VOID GM_i_opblk( i4  op, char *place, GM_OPBLK *gwm_opblk );

FUNC_EXTERN DB_STATUS GM_pt_to_opblk( GM_OPBLK *block, i4  len, char *str );

FUNC_EXTERN DB_STATUS GM_str_to_opblk( GM_OPBLK *block, i4  len, char *str );

FUNC_EXTERN DB_STATUS GM_trim_pt_to_opblk( GM_OPBLK *block, i4  len,
					  char *str );

FUNC_EXTERN void GM_opblk_dump( GM_OPBLK *block );

/* gwmplace.c */

FUNC_EXTERN STATUS GM_rsb_place( GM_RSB *gwm_rsb );

FUNC_EXTERN STATUS GM_klast_place( GM_RSB *gwm_rsb );

FUNC_EXTERN VOID GM_key_place( GM_RSB *gwm_rsb );

/* gwmpmib.c */

FUNC_EXTERN STATUS GM_pmib_init(void);

/* gwmrow.c */

FUNC_EXTERN DB_STATUS GM_request( GM_RSB *gm_rsb,
				 GM_GKEY *key,
				 STATUS *cl_stat );

/* gwmscb.c */

FUNC_EXTERN STATUS GM_scb_startup(void);

FUNC_EXTERN STATUS GM_rm_scb(void);

FUNC_EXTERN STATUS GM_domain( i4  next, char *this, char **new );

/* gwmutil.c */

FUNC_EXTERN DB_STATUS GM_open_sub( GWX_RCB *gwx_rcb, i4  rsb_size );

FUNC_EXTERN DB_STATUS GM_tabf_sub( GWX_RCB *gwx_rcb, i4 xrel_flags );

FUNC_EXTERN DB_STATUS GM_check_att( char *name );

FUNC_EXTERN i4  GM_att_type( char *name );

FUNC_EXTERN DB_STATUS GM_pt_and_trim( char *str, i4  space, char **bufp );

FUNC_EXTERN PTR GM_alloc( i4  len );

FUNC_EXTERN void GM_free( PTR obj );

FUNC_EXTERN PTR GM_ualloc( i4  len, ULM_RCB *ulm_rcb, CS_SEMAPHORE *sem );

FUNC_EXTERN char *GM_my_vnode(void);
FUNC_EXTERN char *GM_my_server(void);
FUNC_EXTERN i4  GM_my_guises(void);

FUNC_EXTERN DB_STATUS GM_gt_dba( DB_OWN_NAME *dba );
FUNC_EXTERN DB_STATUS GM_gt_user( DB_OWN_NAME *dba );
FUNC_EXTERN DB_STATUS GM_dbname_gt( DB_NAME *dbname );

FUNC_EXTERN STATUS GM_gt_sem( CS_SEMAPHORE *sem );
FUNC_EXTERN STATUS GM_i_sem( CS_SEMAPHORE *sem );
FUNC_EXTERN STATUS GM_release_sem( CS_SEMAPHORE *sem );
FUNC_EXTERN STATUS GM_rm_sem( CS_SEMAPHORE *sem );

FUNC_EXTERN DB_STATUS GM_tup_put( i4  slen,
				 PTR src,
				 i4  dlen,
				 i4  tuplen,
				 char *tuple,
				 i4  *tup_used );

FUNC_EXTERN void GM_scanresp( char **srcp,
			     char **classid,
			     char **instance,
			     char **value,
			     i4  *perms );

FUNC_EXTERN DB_STATUS GM_ad_out_col( char *src,
				    i4  srclen,
				    i4  col,
				    GM_RSB *gwm_rsb,
				    i4  tup_len, 
				    char *tuple,
				    i4  *tup_used );

FUNC_EXTERN SP_COMPARE_FUNC GM_nat_compare;

FUNC_EXTERN void GM_dumpmem( char *mem, i4  len );
FUNC_EXTERN void GM_breakpoint(void);

FUNC_EXTERN void GM_just_vnode( char *place, i4  olen, char *outbuf );
FUNC_EXTERN void GM_gca_addr_from( char *place, i4  olen, char *outbuf );

FUNC_EXTERN i4  GM_dbslen( char *str );

FUNC_EXTERN void GM_info_reg( GWX_RCB *gwx_rcb, i4  *journaled, i4  *structure,
			     i4  *unique, i4  *update );

FUNC_EXTERN VOID GM_get_pmsym( char *sym, char *dflt, char *result, i4  len );

FUNC_EXTERN void GM_dump_dmt_att( char *msg, DMT_ATT_ENTRY *dmt_att );

FUNC_EXTERN void GM_dump_gwm_att( char *msg, GM_ATT_ENTRY *gwm_att );

FUNC_EXTERN void GM_dump_gwm_katt( char *msg, GM_ATT_ENTRY *gwm_att );

FUNC_EXTERN bool GM_chk_priv( char *user_name, char *priv_name );

/* gwmxtab.c */

FUNC_EXTERN GW_EXIT_FUNC GXinit;


FUNC_EXTERN VOID GM_i_out_row( GWX_RCB *gwx_rcb );
FUNC_EXTERN i4 GM_att_offset( GM_RSB *gwx_rcb, i4 att_number );
FUNC_EXTERN i4 GM_key_offset( GM_RSB *gwx_rcb, i4 key_seq );
/* end of gwmint.h */
