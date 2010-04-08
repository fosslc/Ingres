/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** gwmplace.h -- header for place related stuff; needs gca.h first.
**
** Description
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
**	    broke out of gwmint.h
**	11-Nov-1992 (daveb)
**	    Remove ifdef for GCA_FS_MAX_DATA -- it better be there now.
**	    Change conn_request/response to conn_rsb.
**	25-Nov-1992 (daveb)
**	    Add cursor for distributing vnode requests among
**	    installation capable servers.
**      24-jul-97 (stial01)
**          II_GM_CONN_BLK: Added conn_buffer_size, made conn_buffer a ptr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct II_GM_VNODE_BLK	GM_VNODE_BLK;
typedef struct II_GM_SRVR_BLK	GM_SRVR_BLK;
typedef struct II_GM_PLACE_BLK	GM_PLACE_BLK;



/*}
** Name:	GM_VNODE_BLK - entry for a vnode.
**
** Description:
**
**	Node in the GM_PLACES gwm_places tree.
**
**	One of these for each vnode that GM knows about.
**	It contains a pointer to a tree that contains
**	all accessible servers known in that vnode.
**
**	Key is the vnode name in the user domain, stashed
**	somewhere safe.  Comparision is STcompare.
**
**	The servers are to be queried each time we access
**	a new vnode, with maybe a timelimit.
**
**	To insert, gwm_places_sem must be held:
**
**		get gwm_places_sem;
**		if( not already in gwm_places )
**		{
**			v = allocate( sizeof *v );
**			init(v);
**			insert( &v->vnode_blk, gwm_places );
**		}
**		release gwm_places_sem;
**
**	You don't ever delete vnodes.  FIXME?
**
** History:
**	18-sep-92 (daveb)
**	    Created.
*/

struct II_GM_VNODE_BLK
{
    i4		vnode_expire;	/* when to re-query */
};



/*}
** Name:	GM_SRVR_BLK - entry for a server.
**
** Description:
**
**	part of node in the GM_PLACES tree.
**
**	One of these for each server known to GM
**	in a vnode.  Key is the place of the server,
**	stashed in a safe place.  Comparison is by STcompare.
**      It is a string of the form vnode::/@gca_address.  Neither
**      the place nor the gca_address part is optional, and there 
**      may be no dbname part from a general GCA address.  The GCN
**	is addressed as "/iinmsrvr".
**
**	We keep a count of the number of outstanding
**	connection/requests to the server, and block
**	using a condition until it's OK.
**
**	The place_semaphore must be held when writing srvr_connects.
**	The gwm_places_sem should not be held when modifying connects,
**	or waiting on a condition for connects.
**
**	For deletion, gwm_places_sem and place_sem must be held.  You should 
**	never delete a server with active connections.
**
**	Note that you should never be deleting a server with a connection!
**		
** History:
**	18-sep-92 (daveb)
**	    Created.
*/

struct II_GM_SRVR_BLK
{
    CS_CONDITION	srvr_cnd;	/* wait for connects */
    i4			srvr_connects;	/* active sessions */
    i4			srvr_valid;     /* time when this was last valid */
    i4			srvr_flags;	/* mods to type... */
    char		srvr_class[ GM_MIB_PLACE_LEN ];
    i4			srvr_state;
#	define GM_SRVR_OK	0
#	define GM_SRVR_FAILED	1
    SPTREE		srvr_conns;	/* connections for the server */
};



/*}
** Name:	GM_PLACE_BLK - entry for a place table
**
** Description:
**
**	Node in the GM_GLOBALS gwm_places tree.  Key points to a saved
**	string of the form [vnode][::/@gca_addrplace].
**	The vnode entry must be first, followed by known servers for
**	the vnode.  Vnode may be null for the current installation.
**
**	The place_srvr block is used for getting at the name server
**	for the vnode.
**
** History:
**	18-sep-92 (daveb)
**	    Created.
**	25-Nov-1992 (daveb)
**	    Add cursor for distributing vnode requests among
**	    installation capable servers.
*/

struct II_GM_PLACE_BLK
{
    SPBLK		place_blk;
    CS_SEMAPHORE	place_sem;
    i4			place_type;
#   define		GM_PLACE_VNODE		1
#   define		GM_PLACE_SRVR		2
    GM_VNODE_BLK	place_vnode;
    GM_SRVR_BLK		place_srvr;
    GM_PLACE_BLK	*place_cursor;
    char		place_key[1];
};


/*}
** Name:	GM_CONN_BLK - entry for a connection to a server.
**
** Description:
**
**	pointed to by a GM_SRVR_BLK, or by another GM_CONN_BLK
**
**	conn_blk.key is a value interpreted as an increasing value,
**	unique for the server.  (It may be seeded with a timestamp.)
**
**	One of these for each active connection to a server.
**	Contains necessary GCA buffers and state, pointers back
**	to the associated SRVR and RSB.
**
** History:
**	18-sep-92 (daveb)
**	    Created.
**	8-oct-92 (daveb)
**	    added GCA stuff.
**	11-Nov-1992 (daveb)
**	    Remove ifdef for GCA_FS_MAX_DATA -- it better be there now.
**	    Change conn_request/response to conn_rsb.
**      24-jul-97 (stial01)
**          II_GM_CONN_BLK: Added conn_buffer_size, made conn_buffer a ptr.
*/

struct II_GM_CONN_BLK
{
    SPBLK		conn_blk;
    CS_SEMAPHORE	conn_sem;		/* only one user at a time */
    GM_PLACE_BLK	*conn_place;		/* back pointer */
    i4			conn_state;
#	define		GM_CONN_UNUSED		1
#	define		GM_CONN_FORMATTED	2
#	define		GM_CONN_LOADING		3
#	define		GM_CONN_LOADED		4
#	define		GM_CONN_GCM_WORKING	5
#	define		GM_CONN_GCM_DONE	6
#	define		GM_CONN_UNLOADING	7
#	define		GM_CONN_EMPTIED		8
#	define		GM_CONN_ERROR		10
    
    GM_RSB		*conn_rsb;		/* what we're working on */

    /* GCM related stuff */

    GCA_FS_PARMS	conn_fs_parms;
    char		*conn_data_start;	/* where buffer starts */
    i4			conn_data_length;	/* how big buffer is */
    char		*conn_data_last;	/* next available */
    i4             conn_buffer_size;
    char		*conn_buffer;
};


/*}
** Name:	GM_REGISTER_BLK	- block for per-registered server data.
**
** Description:
**	BLock used temporarily to store results from a name server query.
**
** History:
**	9-oct-92 (daveb)
**	    created
*/
typedef struct
{
    SPBLK	reg_blk;
    i4		reg_entry;
    i4		reg_flags;
    char	reg_addr[ GM_MIB_PLACE_LEN ];
    char	reg_class[ GM_MIB_PLACE_LEN ];
    char	reg_object[ GM_MIB_PLACE_LEN ];

} GM_REGISTER_BLK;

/* gwmgcn.c */

FUNC_EXTERN DB_STATUS GM_ask_gcn( SPTREE *t, char *vnode );

/* gwmpmib.c */

FUNC_EXTERN STATUS GM_pmib_init(void);

/* gwmplace.c */

FUNC_EXTERN GM_CONN_BLK *GM_ad_conn( char *place );
FUNC_EXTERN void GM_rm_conn( GM_CONN_BLK *conn );
FUNC_EXTERN void GM_conn_error( GM_CONN_BLK *conn );

/* end of gwmplace.h */

