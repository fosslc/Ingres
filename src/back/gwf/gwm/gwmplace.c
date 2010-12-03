/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <me.h>
# include <sp.h>
# include <st.h>
# include <tm.h>
# include <tr.h>
# include <erglf.h>
# include <mo.h>
# include <scf.h>
# include <adf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <dmucb.h>
# include <ulf.h>
# include <ulm.h>
# include <gca.h>
# include <gcm.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"
# include "gwmplace.h"

/**
** Name:	gwmplace.c  -- place and domain related stuff.
**
** Description:
**
**	This file contains routines for manipulating user domains,
**	an internal list of domains, and connections to servers
**	in known domains.
**
**	Format of a place string is a pre-resolved name server target
**	string of the form:
**
**		[vnode::][dbname]/@gca_address
**
**	missing vnode:: is interpreted as the II_LCL[xx]_VNODE;
**
**	dbname is always ignored if present;
**
**	missing /@gca_address is treated as "any server registered
**	for GCA_RG_INSTALL in it's flags."
**
**	This file defines:
**
**	GM_first_place	- get first place for this user domain.
**	GM_next_place	- get next server in the user domain.
**	GM_user_dom	- locate user domain tree.
**	GM_query_vnode	- locate servers on a given vnode.
**	GM_gt_vnode	- add a vnode entry, creating if needed.
**	GM_rm_vnode	- delete vnode from known tree.
**	GM_ad_srvr	- add a known server.
**	GM_zp_srvr	- drop a server entry.
**	GM_ad_conn	- add a connection for a server place.
**	GM_rm_conn	- delete/drop a connection
**	GM_zp_connections	- when server is dead.
**	GM_kfirst_place	- set cur key to first place in user domain.
**	GM_knext_place	- set key to next place in user domain.
**
** History:
**	21-Sep-92 (daveb)
**	    created.
**	9-oct-92 (daveb)
**	    include <dmrcb.h> because of gwfint.h prototypes.  Include
**	    gca.h.  Move GM_k{first,next}_place here so gwmkey.c doesn't
**	    need to include "gwmplace.h".
**	11-Nov-1992 (daveb)
**	    include <gcm.h> for things in the CONN_BLK in gwmplace.h
**
**	    If no place_blk, then clear cur_key; default
**	    domain is current server.
**
**	    Not an error to have no place; it's the default single
**	    server domain.
**	16-Nov-1992 (daveb)
**	    Simplify mostly common gt_vnode and ad_srvr with new GM_i_place.
**	18-Nov-1992 (daveb)
**	    fix debugging TRdisplay segv.
**	2-Dec-1992 (daveb)
**	    Major rewrite/reorg to handle domain type place iteration.
**	    Consolidate GM_next_place and GM_first_place into one routine;
**	    Ditto GM_kfirst_place and GM_knext_place to GM_rsb_place.
**	3-Dec-1992 (daveb)
**	    Cleaned out cruft in GM_last_place.   Use simpler GM_domain
**	    system, handle final domains that are servers.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x for %p for pointer values in GM_conn_dump().
**      21-Mar-1994 (daveb) 60738
**          Add GM_key_place to properly set position keys.  Otherwise
**  	    we fail to return any rows on a partial key.
**      10-May-1994 (daveb) 62631
**          documented better.  Undo cursor stuff, which doesn't work
**          right.  Sometimes it would return a server from the
**          previous vnode, causing, too many places to be scanned.
**  	    Pending a real fix, We'll pound the first server we find.
**      24-jul-97 (stial01)
**          GM_ad_conn() Determine 'conn_buffer_size' and alloc with GM_CONN_BLK
**      08-Jul-98 (fanra01)
**          Modified order of release and remove of semaphore in GM_zp_srvr.
**          This would cause the message E_CL250A to appear when extending the
**          domain.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-feb-2003 (devjo01)
**	    Corrected E_GW8204 test in GM_vn_server to avoid referencing 
**	    through a NULL 'pb'.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	24-Aug-10 (gordy)
**	    Skip servers registered with NMSVR mib.
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing static prototypes
**/

/* forwards */

static char *GM_place( char *this, i4  dom_type );

static void GM_domsrch( char *this, i4  dom_type,
		       char *srch_dom, char **ret_dom );

static GM_PLACE_BLK *GM_nxt_server( GM_PLACE_BLK *pb );

static GM_PLACE_BLK *GM_vn_server( GM_PLACE_BLK *vnode );

char *GM_last_place(void);

static GM_PLACE_BLK *GM_gt_vnode( char *vnode );
static GM_PLACE_BLK *GM_ad_srvr( char *srvr );

static void GM_zp_connections( GM_PLACE_BLK *place_blk );
static GM_PLACE_BLK *GM_i_place( char *place_name );
static void GM_zp_srvr( GM_PLACE_BLK *place_blk );
static STATUS GM_query_vnode( GM_PLACE_BLK *vnode_place );
static bool GM_check_vnode( GM_PLACE_BLK *pb );
static void GM_rm_vnode( GM_PLACE_BLK *place );
static char * GM_conn_state( GM_CONN_BLK *conn );
static void GM_conn_dump( GM_CONN_BLK *conn );
static void GM_req_conn( GM_CONN_BLK *conn );
static void GM_resp_conn( GM_CONN_BLK *conn );



/*{
** Name:	GM_rsb_place	- set key to first/next place in user domain.
**
** Description:
**	Sets the cur_key in the rsb to the next place in the user domain,
**	assuming the current place is valid.  Clears out the classid and
**	instance if it's a next place.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwm_rsb		the rsb with the cur_key; classid must be valid.
**
** Outputs:
**	gwm_rsb		the cur_key is updated.
**
** Returns:
**	OK		if it worked
**	MO_NO_NEXT	if it didn't.
**
** History:
**	2-Dec-1992 (daveb)
**	    collapsed from knext_place and kfirst_place, handle
**	    dom_type.
*/

STATUS
GM_rsb_place( GM_RSB *gwm_rsb )
{
    STATUS cl_stat = MO_NO_NEXT;
    char *new_place;
    i4  old_len;

    old_len =  gwm_rsb->cur_key.place.len;
    if( gwm_rsb->dom_type == GMA_UNKNOWN )
    {
	new_place =  old_len ? NULL : GM_my_server();
    }
    else
    {
	new_place = GM_place( old_len ? gwm_rsb->cur_key.place.str : NULL,
			     gwm_rsb->dom_type );
    }

    if( new_place != NULL )
    {
	STcopy( new_place, gwm_rsb->cur_key.place.str );
	gwm_rsb->cur_key.place.len = STlength( new_place );
	if( old_len != 0 )
	{
	    gwm_rsb->cur_key.classid.str[0] = EOS;
	    gwm_rsb->cur_key.classid.len = 0;
	    gwm_rsb->cur_key.instance.str[0] = EOS;
	    gwm_rsb->cur_key.instance.len = 0;
	}
	cl_stat = OK;
    }
    return( cl_stat );
}



/*{
** Name:	GM_klast_place	- set last_key to last place in user domain.
**
** Description:
**	Finds the last place in the user domain (maybe a vnode)
**	and sets the last key there.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwm_rsb		session block containing the last_key.
**
** Outputs:
**	gwm_rsb		
**
** Returns:
**	OK	if did it
**	FAIL	if first place couldn't be found.
**
** History:
**	23-Nov-1992 (daveb)
**	    Created.
*/

STATUS
GM_klast_place( GM_RSB *gwm_rsb )
{
    GM_GKEY *key = &gwm_rsb->last_key;
    char *place;

    if( gwm_rsb->dom_type == GMA_UNKNOWN )
	place = GM_my_server();
    else
	place = GM_last_place();

    if( place != NULL )
    {
	STcopy( place, key->place.str );
	key->place.len = STlength( place );
    }
    else
    {
	key->place.str[0] = EOS;
	key->place.len = 0;
    }
    /* FIXME: necessary? */
    STcopy( "ZZZZZZZZZ", key->classid.str );
    key->classid.len = STlength( key->classid.str );
    STcopy( key->classid.str, key->instance.str );
    key->instance.len = key->classid.len;
    return( OK );
}


/*{
** Name:	GM_place	- get first/next place for this user domain.
**
** Description:
**	Get the first or next thing of interest in the user domain.
**
**	This is frightfully complicated, because the user domain may
**	have both vnode entries and explicit server entries.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	this		NULL
**	dom_type	GMA_VNODE or GMA_SERVER, the type of entry wanted.
**
** Outputs:
**	none.
**
** Returns:
**	The name of the requested place, or NULL if no more exist in
**	the user domain.
**
** History:
**	2-Dec-1992 (daveb)
**	    created.
**	3-Dec-1992 (daveb)
**	    Handle single domain of this server correctly.
**      10-May-1994 (daveb)
**          If we want vnode, and current is vnode, don't continue
**  	    to search this place, go to next place.
*/

static char *
GM_place( char *this, i4  dom_type )
{
    char	*ret_dom = NULL;
    char	*dom_this = NULL;
    i4		first;

    if( this == NULL )		/* just get first domain */
    {
	/* first place in user domain */
	GM_domain( TRUE, this, &dom_this );
    }
    else			/* get (next) domain to search */
    {
	/* If 'this' is a server, we get back it's vnode
	   and we're done, so this and dom_this are different.
	   If we get the same thing back, then it was either
	   a server entry in the domain, or it was a vnode
	   in the domain.  Either way, get the next domain entry. */

	/* get domain place for this server, either vnode or server */
	GM_domain( FALSE, this, &dom_this );
	if( dom_this != NULL && 
	   ( STequal( this, dom_this ) || dom_type == GMA_VNODE ) )
	{
	    /* next place in user domain */
	   GM_domain( TRUE, dom_this, &dom_this );
	   this = NULL;
       }
    }

    /* While not found, and there's somewhere to look, try to find a
       place of the dom_type requested.  On subsequent iterations,
       go to the next place in the user domain. */

    for( first = TRUE; dom_this != NULL && ret_dom == NULL ; first = FALSE )
    {
	if( !first )
	{
	    /* next place in user domain */
	    GM_domain( TRUE, dom_this, &dom_this );
	    this = NULL;
	}

	if( dom_this != NULL )
	    GM_domsrch( this, dom_type, dom_this, &ret_dom  );
    }
    return( ret_dom );
}


/* given user place, dom_type and domain to search,
    update dom_this to the right place and return TRUE
    or return FALSE */

static void
GM_domsrch( char *this, i4  dom_type, char *srch_dom, char **ret_dom )
{
    i4		got_places_sem = FALSE;
    i4		is_me;
    i4		found = FALSE;

    char	tbuf[ GM_MIB_PLACE_LEN ];
    char	dbuf[ GM_MIB_PLACE_LEN ];

    SPBLK		lookup;
    GM_PLACE_BLK	*vnode;
    GM_PLACE_BLK	*pb = NULL;

    if( this != NULL )
	GM_just_vnode( this, sizeof(tbuf), tbuf );
    GM_just_vnode( srch_dom, sizeof(dbuf), dbuf );

    is_me = !STcompare( srch_dom, GM_my_server() );

    do
    {
	/* if this domain entry is a server, and we want a vnode,
	   and it's not this server, then it isn't interesting. */
	
	if ( dbuf[0] != EOS && dom_type == GMA_VNODE && !is_me )
	    break;
	    
	/* Now we need to lock the places tree if it isn't already */
	    
	if( !got_places_sem && GM_gt_sem( &GM_globals.gwm_places_sem ) != OK )
	    break;
	got_places_sem = TRUE;
	
	/* Find the vnode of the domain place in our tree */
	
	vnode = GM_gt_vnode( dbuf[0] != EOS ? dbuf : srch_dom );
	
	/* Make sure we have up date info about it's servers */
	
	if( (got_places_sem = GM_check_vnode( vnode )) == FALSE )
	    break;
	
	pb = vnode;

	/* if domain is a user server and we want a server,
	   or it's a VNODE and the user server is us,
	   we're going to try our best to get to it,
	   even if the name server doesn't know about it. */
	
	if( dbuf[0] != EOS && (dom_type != GMA_VNODE || is_me ) )
	{
	    lookup.key = srch_dom;
	    pb = (GM_PLACE_BLK *)SPlookup( &lookup, &GM_globals.gwm_places );
	    if( pb == NULL )
	    {
		pb = GM_ad_srvr( srch_dom );
		SPinit( &pb->place_srvr.srvr_conns, GM_nat_compare );
		pb->place_srvr.srvr_valid = TMsecs();
		
		/* Don't know what it is */
		STcopy( "UNKNOWN", pb->place_srvr.srvr_class );
		
		/* Assume it does everything */
		pb->place_srvr.srvr_flags = ~0;
	    }
	    if( pb != NULL )
	    {
		found = TRUE;
		break;
	    }
	}
	
	/* Now:  vnode points to the vnode entry for the srch_dom
	   		(which might have been a server)

		pb points to the server entry or the vnode entry. */

	/* If this is a server, locate it in our tree in pb.
	   If it's a vnode, we already have it. */
	
	if( this != NULL && tbuf[0] != EOS )
	{
	    lookup.key = this;
	    pb = (GM_PLACE_BLK *)SPlookup( &lookup, &GM_globals.gwm_places );
	}
	
	/* If we don't have this place or vnode,
	   check next place in the domain */
	
	if( pb == NULL )
	    break;
	
	/* Now find what we're looking for. */
	
	switch( dom_type )
	{
	case GMA_SERVER:
	    
	    pb = GM_nxt_server( pb );
	    break;
	    
	case GMA_VNODE:
	    
	    if( dbuf[0] != EOS )
	    {
		/* shouldn't happen; if so, go to next domain */
		GM_error( E_GW8203_VNODE_IN_SERVER );
		break;
	    }
	    pb = GM_vn_server( vnode );
	    break;
	    
	default:
	    /* "GWM: internal error:  unexpected domain type %0d
	       in GM_place" */
	    
	    GM_1error( (GWX_RCB *)0, E_GW8205_UNEXPECTED_DOM_TYPE,
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( dom_type ), (PTR)&dom_type );
	    pb = NULL;
	    break;
	}
	if( pb != NULL )
	    found = TRUE;

    } while( FALSE );
    
    if( found && pb != NULL )
	*ret_dom = pb->place_key ;

    if( got_places_sem )
	GM_release_sem( &GM_globals.gwm_places_sem );
    
    return;
}



/*{
** Name:	GM_nxt_server  -- next server in a vnode
**
** Description:
**	Given the place of a server, return the next one in the
**      same vnode, or NULL.   Called with the place semaphore held.
**
** Re-entrancy:
**	Locks places tree.
**
** Inputs:
**	pb  	    	current server.
**
** Outputs:
**	none.
**
** Returns:
**	pointer to next server in same vnode, or NULL.
**
** History:
**	10-May-94 (daveb)
**  	    documented better.
*/
static GM_PLACE_BLK *
GM_nxt_server( GM_PLACE_BLK *pb )
{
    char	sbuf[ GM_MIB_PLACE_LEN ];
    char	vbuf[ GM_MIB_PLACE_LEN ];
    i4		vlen;
    i4		found;
    
    /* make vbuf be "vnode::" */

    GM_just_vnode( pb->place_key, sizeof(sbuf), sbuf );
    STprintf( vbuf, "%s::", sbuf[0] == EOS ? pb->place_key : sbuf );
    vlen = STlength( vbuf );

    /* advance to server in the vnode */
    for( found = FALSE; pb != NULL && !found ; )
    {
	pb = (GM_PLACE_BLK *)SPfnext( &pb->place_blk );
	if( pb != NULL && ! STbcompare( vbuf, vlen,
				       pb->place_key, vlen, FALSE ) )
	    found = TRUE;
    }
    return( found ? pb : NULL );
}


/*{
** Name:	GM_vn_server  -- return a vnode server for a vnode.
**
** Description:
**	Given a vnode pb, return pointer to a server in it that
**	will handle installation wide queries, or NULL.  If the
**	this server can do it, return it.
**
**  	This was originally intended to round-robin capable
**  	servers in a remote vnode using the place_cursor field,
**  	but it was buggy.  Until that is resolved, we'll pound the
**      first server that is capable.
**
** Re-entrancy:
**	called with the places semaphore held, and the semaphore for
**	the vnode in question.
**
** Inputs:
**	vnode	    pointer to locked vnode 
**
** Outputs:
**	none.
**
** Returns:
**	pointer to an installation-capable PLACE or NULL.
**
** History:
**      10-May-1994 (daveb) 62631
**          documented better.  Undo cursor stuff, which doesn't work
**          right.  Sometimes it would return a server from the
**          previous vnode, causing, too many places to be scanned.
**  	    Pending a real fix, We'll pound the first server we find.
**      5-Nov-2001 (zhahu02) b104451/INGSRV1425
**          to allow queries on ima table with vnode column when using an alias
**          of a dbms server. 
**	27-feb-2003 (devjo01)
**	    Corrected E_GW8204 test to avoid referencing through a NULL 'pb'.
*/

static GM_PLACE_BLK *
GM_vn_server( GM_PLACE_BLK *vnode )
{
    GM_PLACE_BLK *pb;
    SPBLK	lookup;
    i4		found = FALSE;

    /* look for the best capable installation server in
       this vnode.  Best means:  this server if
       it will do, or a fairly selected one from
       those on another vnode.  The place_cursor
       is used to 'fairly' distribute these requests. */
    
    if( !STcompare( vnode->place_key, GM_my_vnode() ) )
    {
	/* this server -- locate it and go */
	lookup.key = GM_my_server();
	pb = (GM_PLACE_BLK *)SPlookup( &lookup,
				      &GM_globals.gwm_places );
	
	if( pb && ((pb->place_srvr.srvr_flags & GCA_RG_INSTALL) || (pb->place_srvr.srvr_flags=1&&pb->place_srvr.srvr_valid>0&&pb->place_srvr.srvr_class!=NULL)))
	{
	    found = TRUE;
	}
	else
	{
	    /* "GWM: internal error:  this process '%0c' isn't
	       a vnode server!?" */
	    
	    GM_1error( (GWX_RCB *)0, E_GW8204_SELF_NOT_INSTALL,
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      0, (PTR)lookup.key );
	    pb = NULL;
	}
    }
    else
    {
	/* Remote place: pick first server after the cursor
	   that can handle it.   */
	
	pb = vnode->place_cursor;
	do
	{
	    if( pb == NULL )
		pb = GM_nxt_server( vnode );
	    else
		pb = (GM_PLACE_BLK *)SPfnext( &pb->place_blk );
	    
	    if( pb == NULL || pb->place_type == GM_PLACE_VNODE )
		pb = GM_nxt_server( vnode );
	    
	    if( pb != NULL && (pb->place_srvr.srvr_flags & GCA_RG_INSTALL) )
		found = TRUE;
	    
	} while( !found && pb != vnode->place_cursor );
	
# ifdef FIXED_CURSOR_ROUND_ROBIN
	/* ifdefing this out has us always return the first server,
	   which is better than returning earlier servers.  FIXME */

	vnode->place_cursor = found ? pb : NULL; 
# endif
    }

    return( found ? pb : NULL );
}


/*{
** Name:	GM_last_place	- get last place for this user domain.
**
** Description:
**	Get the last server in the last vnode in the user domain.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	place block for last place in user domain, or NULL.  Place
**	might point to a vnode, not a server.
**
** History:
**	23-Nov-1992 (daveb)
**	    created.
**	3-Dec-1992 (daveb)
**	    cleaned out cruft; use simpler GM_domain system, handle
**	    final domains that are servers.
*/
char *
GM_last_place(void)
{
    
    GM_PLACE_BLK *pb = NULL;	/* this place */
    GM_PLACE_BLK *lpb = NULL;	/* last place */

    char	*dom_this = NULL;
    char	*last_dom;
    char	vbuf[ GM_MIB_PLACE_LEN ];

    bool	got_places_sem = FALSE;

    do				/* ONE TIME loop */
    {
	/* locate the last thing in the domain */
	do
	{
	    last_dom = dom_this;
	    (void) GM_domain( TRUE, dom_this, &dom_this );
	} while( dom_this != NULL );

	if( last_dom == NULL )
	    break;

	/* If it's a server, we're done */

	GM_just_vnode( last_dom, sizeof(vbuf), vbuf );
	if( vbuf[0] )
	    break;

	/* It's a vnode.  lock places tree while we look at it. */

	if( GM_gt_sem( &GM_globals.gwm_places_sem ) != OK )
	    break;
	got_places_sem = TRUE;

	pb = GM_gt_vnode( last_dom );
	if( pb == NULL )
	    break;

	/* get up-to-date list of servers in it */

	got_places_sem = GM_check_vnode( pb );

	/* pb points to this vnode.  Advance to next vnode, or to the end
	   of the places tree.   The last key is the "previous" one we
	   get to, possibly the vnode we're at. */

	do
	{
	    lpb = pb;
	    pb = (GM_PLACE_BLK *) SPfnext( &pb->place_blk );
	} while( pb != NULL && pb->place_type != GM_PLACE_VNODE );
	
	last_dom = lpb->place_key;

    } while( FALSE );

    if( got_places_sem )
	(void)GM_release_sem( &GM_globals.gwm_places_sem );

    return( last_dom );
}



/*{
** Name:	GM_gt_vnode	- get node in places tree, adding if needed.
**
** Description:
**	Allocate block, set it up, and insert it.  Called with the
**	gwm_places_sem held.  Logs any underlying errors.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	vnode	name of the vnode to get.
**
** Outputs:
**	none.	
**
** Returns:
**	The newly linked place block.  NULL on allocation failure or
**	other error.
**
** History:
**	22-sep-92 (daveb)
**	    created.
**	16-Nov-1992 (daveb)
**	    simplify using more powerful GM_i_place.
*/

static GM_PLACE_BLK *
GM_gt_vnode( char *vnode )
{
    GM_PLACE_BLK *place_blk;
    SPBLK	lookup;
    char	sem_name[ CS_SEM_NAME_LEN + GM_MIB_PLACE_LEN ];
    
    do
    {
	lookup.key = vnode;
	place_blk = (GM_PLACE_BLK *)SPlookup( &lookup, &GM_globals.gwm_places);
	if( place_blk != NULL )
	    break;

	place_blk = GM_i_place( vnode );
	if( place_blk == NULL )
	    break;
	    
	CSn_semaphore( &place_blk->place_sem,
		      STprintf( sem_name, "GM vnode %s",
			       place_blk->place_key ) );

	place_blk->place_type = GM_PLACE_VNODE;

	/* extra field in vnode */
	place_blk->place_vnode.vnode_expire = 0;
	    
	SPinstall( &place_blk->place_blk, &GM_globals.gwm_places );

    } while( FALSE );

    return( place_blk );
}


/*{
** Name:	GM_rm_vnode	- delete vnode from known tree.
**
** Description:
**	Does nothing; we don't want to do this, here for completeness
**	in case we get ideas later.  Must be called with gwm_places_sem
**	held.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	place_blk	a GM_PLACE_VNODE to delete.
**
** Outputs:
**	none.	
**
** Returns:
**	none
**
** History:
**	22-sep-92 (daveb)
**	    created
*/

static void
GM_rm_vnode( GM_PLACE_BLK *place )
{
    /* does nothing, we don't use it. */
}


/*{
** Name:	GM_ad_srvr	- add a server, which must not exist.
**
** Description:
**	Allocate block, initialize, and insert into gwm_places.
**	Logs any underlying errors.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	place		string name of the server, in the form
**			"[dbname::]dbname/@gca_address".
** Outputs:
**	<param name>	<output value description>
**
** Returns:
**	Newly allocated block, or NULL on allocation failure or error.
**
** History:
**	22-sep-92 (daveb)
**	    created.
**	16-Nov-1992 (daveb)
**	    simplify using more powerful GM_i_place.
**	20-Aug-1993 (daveb)
**	    make sem_name buf bigger to handle long places (on VMS).
*/

static GM_PLACE_BLK *
GM_ad_srvr( char *place )
{
    GM_PLACE_BLK *place_blk;
    i4  	len;
    char	sem_name[ CS_SEM_NAME_LEN + GM_MIB_PLACE_LEN ];

    do
    {
	place_blk = GM_i_place( place );
	if( place_blk == NULL )
	    break;
	    
	CSn_semaphore( &place_blk->place_sem, 
		      STprintf( sem_name, "GM srvr %s",
			       place_blk->place_key ) );

	place_blk->place_type = GM_PLACE_SRVR;

	SPinstall( &place_blk->place_blk, &GM_globals.gwm_places );

    } while(FALSE);

    return( place_blk );
}


/*{
** Name:	GM_zp_srvr	- drop a server entry.
**
** Description:
**	Delete the specified server block.  Called while holding
**	the gwm_places_semaphore.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	place_blk	the server block to delete.
**
** Outputs:
**	none.
**
** Returns:
**	none.	
**
** Side Effects:
**	Will drop any outstanding connections to the server.
**
** History:
**	22-sep-92 (daveb)
**	    created.
**      08-Jul-98 (fanra01)
**          Modified order of release and remove of semaphore.
*/
static void
GM_zp_srvr( GM_PLACE_BLK *place_blk )
{
    GM_SRVR_BLK *srvrp;
    bool got_place_sem = FALSE;

    do
    {
	if( OK != GM_gt_sem( &place_blk->place_sem ) )
	    break;
	got_place_sem = TRUE;
	
	srvrp = &place_blk->place_srvr;

	if( srvrp->srvr_connects != 0 )
	{
	    /* oops, someone's connected to a dead server */
	    GM_zp_connections( place_blk );
	}
	    
	SPdelete( &place_blk->place_blk, &GM_globals.gwm_places );
	CScnd_free( &srvrp->srvr_cnd );

        if( got_place_sem )
        {
            /*
            ** Release the semaphore before removing it and freeing the
            ** memory.
            */
            (void) GM_release_sem( &place_blk->place_sem );
            got_place_sem = FALSE;
        }
	(void) GM_rm_sem( &place_blk->place_sem );
	GM_free( (PTR) place_blk );

    } while ( FALSE );

    if( got_place_sem )
	(void) GM_release_sem( &place_blk->place_sem );
}


/*{
** Name:	GM_ad_conn	- add a connection for a server place.
**
** Description:
**	Allocates a new GM_CONN_BLK for the specified place, potentially
**	waiting for the connection limit.
**
**	Because this may block (waiting for connections to complete),
**	gwm_places_sem and the place semaphore may not be held at the
**	time of the call.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	place_name	The place name, in the form "vnode:whatever"
**
** Outputs:
**	none.
**
** Returns:
**	New connection block, or NULL on allocation failure or 
**	semaphore failure.
** History:
**	22-sep-92 (daveb)
**	    created.
**      24-jul-97 (stial01)
**          GM_ad_conn() Determine 'conn_buffer_size' and alloc with GM_CONN_BLK
*/

GM_CONN_BLK *
GM_ad_conn( char *place_name )
{
    GM_PLACE_BLK	*place_blk;
    GM_CONN_BLK		*conn = NULL;
    bool	got_places_sem = FALSE;
    bool	got_place_sem = FALSE;
    SPBLK	lookup;
    char	sem_name[ CS_SEM_NAME_LEN + GM_MIB_PLACE_LEN ];
    char	fixed_name[ GM_MIB_PLACE_LEN ];
    i4     conn_buffer_size;

    do
    {
	if( GM_gt_sem( &GM_globals.gwm_places_sem ) != OK )
	    break;
	got_places_sem = TRUE;

	lookup.key = place_name;
	place_blk = (GM_PLACE_BLK *)SPlookup( &lookup,
					     &GM_globals.gwm_places );
	if( place_blk == NULL ||
	   place_blk->place_type != GM_PLACE_SRVR )
	{
	    /* if it's a server class, just use the vnode as the place. */
	    GM_gca_addr_from( place_name, sizeof(fixed_name), fixed_name );
	    if( fixed_name[0] != '/' || fixed_name[1] == '@' )
		break;

	    GM_just_vnode( place_name, sizeof(fixed_name), fixed_name );
	    lookup.key = fixed_name;
	    place_blk = (GM_PLACE_BLK *)SPlookup( &lookup,
						 &GM_globals.gwm_places );
	    if( place_blk == NULL )
	    {
		/* "GWM: internal error:  no place %0c found in
		   internal storage"*/

		GM_1error( (GWX_RCB *)0, E_GW8201_LOST_PLACE,
			  GM_ERR_INTERNAL|GM_ERR_LOG,
			  0, (PTR)place_name );
		place_blk = NULL;
		break;
	    }
	}

	/* wait till a global connection is available  */
	
	while( GM_globals.gwm_connects > GM_globals.gwm_max_conns )
	    CScnd_wait( &GM_globals.gwm_conn_cnd,
		       &GM_globals.gwm_places_sem );

	/* may have to back this out later on error */

	GM_globals.gwm_connects++;

	/* now get the place sem */

	if( GM_gt_sem( &place_blk->place_sem ) != OK )
	    break;
	got_place_sem = TRUE;

	/* done with places_sem, except for error */

	if( GM_release_sem( &GM_globals.gwm_places_sem ) != OK )
	    break;
	got_places_sem = FALSE;

	/* wait for OK connection count */

	while( place_blk->place_srvr.srvr_connects >
	      GM_globals.gwm_max_place )
	{
	    CScnd_wait( &place_blk->place_srvr.srvr_cnd,
		       &place_blk->place_sem );
	}

	conn_buffer_size = GCA_FS_MAX_DATA;
	conn = (GM_CONN_BLK *)GM_alloc( sizeof( *conn)  + conn_buffer_size );
	if( conn == NULL )
	{
	    /* nothing to update in place now */
	    GM_release_sem( &place_blk->place_sem );
	    got_place_sem = FALSE;

	    /* but we need to drop count */
	    GM_gt_sem( &GM_globals.gwm_places_sem );
	    got_places_sem = TRUE;

	    GM_globals.gwm_connects--;
	    break;
	}

	place_blk->place_srvr.srvr_connects++;
	
	/* Extra casts for picky compilers */
	conn->conn_blk.key = (PTR) ((SCALARP) TMsecs());
	conn->conn_place = place_blk;
	GM_i_sem( &conn->conn_sem );
	conn->conn_buffer_size = conn_buffer_size;
	conn->conn_buffer = (char *)conn + sizeof( *conn);

	CSn_semaphore( &conn->conn_sem,
		      STprintf( sem_name, "GM CONN %s:%d",
			       place_blk->place_key,
			       place_blk->place_srvr.srvr_connects ));
	
	SPinstall( &conn->conn_blk,
		 &place_blk->place_srvr.srvr_conns );

	conn->conn_state = GM_CONN_UNUSED;

    } while(FALSE);
    
    if( got_place_sem )
	(void) GM_release_sem( &place_blk->place_sem );
    if( got_places_sem )
	(void) GM_release_sem( &GM_globals.gwm_places_sem );

    return( conn );
}


/*{
** Name:	GM_rm_conn	- delete/drop a connection
**
** Description:
**	Drops the connection, assuming it is invalid.  Called
**	with the place and the places semaphore held.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn	the connection block to take out.
**
** Outputs:
**	none.
**
** Returns:
**	none
**
** Side Effects:
**
**	Updates the global and server specific connection counts,
**	signaling conditions for anyone who may be waiting for
**	an available connection.
**
** History:
**	22-sep-92 (daveb)
**	    written.
**	13-Nov-1992 (daveb)
**	    was releasing places twice, and place not at all.
**	19-Nov-1992 (daveb)
**	    Typo, || instead of && resulted in always invalidating
**	    the vnode on any connection release, resulting in too
**	    gcn queries.
**	21-Sep-2005 (wanfr01)
**	    INGSRV3338, Bug 114714
**	    Added sanity check:  Must signal the GWM conditions
**	    no matter what or every IMA query will hang afterwards
**	17-Dec-2003 (jenjo02)
**	    Replaced CScnd_signal with CScnd_broadcast to 
**	    wake up all, not just one, waiters.
*/

void
GM_rm_conn( GM_CONN_BLK *conn )
{
    bool got_places_sem = FALSE;
    bool got_place_sem = FALSE;
    bool got_conn_sem = FALSE;
    GM_PLACE_BLK *place_blk;
    char cond_released = FALSE;
    char sp_conn_released = FALSE;
    CS_SID sessionid;

    do
    {
	/* get conn sem, places sem, and place sem. */

	place_blk = conn->conn_place;

	if( GM_gt_sem( &conn->conn_sem ) != OK )
	    break;
	got_conn_sem = TRUE;

	if( GM_gt_sem( &GM_globals.gwm_places_sem ) != OK )
	    break;
	got_places_sem = TRUE;

	if( GM_gt_sem( &place_blk->place_sem ) != OK )
	    break;
	got_place_sem = TRUE;

	/* if conn state is bad, mark vnode as needing a check */

	if( conn->conn_state != GM_CONN_UNUSED &&
	   conn->conn_state != GM_CONN_EMPTIED )
	{
	    GM_conn_error( conn );
	    /* FIXME - log error? */
	}

	/* delete block from this place */

	SPdelete( &conn->conn_blk,
		 &place_blk->place_srvr.srvr_conns );

	place_blk->place_srvr.srvr_connects--;
        sp_conn_released = TRUE;
	CScnd_broadcast( &place_blk->place_srvr.srvr_cnd );

	/* now release conn, place, places */

	GM_release_sem( &conn->conn_sem );
	got_conn_sem = FALSE;
	GM_rm_sem( &conn->conn_sem );
	GM_free( (PTR)conn );

	GM_globals.gwm_connects--;
	cond_released = TRUE;
	CScnd_broadcast( &GM_globals.gwm_conn_cnd );

    } while(FALSE);
    
    /* If you didn't signal the gwm conditions yet, display a warning
    ** and signal them, to avoid hanging every IMA query afterwards
    */
    if (cond_released == FALSE)
    {
	CSget_sid(&sessionid);
	TRdisplay ("%@  Warning: Forced gwm_conn release for session %p\n",sessionid);

	GM_globals.gwm_connects--;
	CScnd_broadcast( &GM_globals.gwm_conn_cnd );

	if (sp_conn_released == FALSE)
	{
	   TRdisplay ("%@  Warning: Forced sp_conn release for session %p\n",sessionid);
	   place_blk->place_srvr.srvr_connects--;
	}
    }

    if( got_conn_sem )
	(void) GM_release_sem( &conn->conn_sem );

    if( got_places_sem )
	(void) GM_release_sem( &GM_globals.gwm_places_sem );

    if( got_place_sem )
	(void) GM_release_sem( &place_blk->place_sem );

}


/*{
** Name:	GM_zp_connections	- when server is dead.
**
** Description:
**	Drop any connections hanging on this server, becuase we
**	believe that it is dead.  Called while holding the semaphore
**	on the server.
**
** Re-entrancy:
**	yes, connections protected by semaphores.
**
** Inputs:
**	place_blk	a GM_PLACE_SRVR of one that is dead but which
**			has a non-zero connection count.
**
** Outputs:
**	none.
**
** Returns:
**	none
**
** History:
**	22-Sep-92 (daveb)
**	    Created;
*/

static VOID
GM_zp_connections( GM_PLACE_BLK *place_blk )
{
    GM_CONN_BLK *cb;
    SPTREE *conn_tree;

    conn_tree = &place_blk->place_srvr.srvr_conns;

    for( cb = (GM_CONN_BLK *)SPfhead( conn_tree );
	cb != NULL; cb = (GM_CONN_BLK *)SPfnext( &cb->conn_blk ) )
    {
	(void) GM_cancel( cb );
	GM_rm_conn( cb );
    }
}


/*{
** Name:	GM_query_vnode - locate servers on a given vnode.
**
** Description:
**	Queries the name server on the specified vnode and enters all it's
**	servers into the gwm_places tree.
**
**	This is a potentially very slow operation, so it should not be done
**	very often, and should not be called while holding any semaphores.
**
** Re-entrancy:
**	yes.	
**
** Inputs:
**	place_blk	with GM_PLACE_VNODE, the node to query.
**
** Outputs:
**	none
**
** Returns:
**	OK
**	other		error status.
**
** Side Effects:
**
** History:
**	22-sep-92 (daveb)
**	    created.
**	18-Nov-1992 (daveb)
**	    fix debugging TRdisplay segv.
**	24-Aug-10 (gordy)
**	    Skip servers registered with NMSVR mib.  These are 
**	    Name Servers from other installations registered with 
**	    the master Name Server and are not a part of the target
**	    vnode domain.  Also, the local Name Server registers
**	    with both NMSVR and IINMSVR mib and the NMSVR entry
**	    can be ignored.
*/
static STATUS
GM_query_vnode( GM_PLACE_BLK *vnode_place )
{
    GM_PLACE_BLK	*pb;

    SPBLK	lookup;
    STATUS	cl_stat;
    i4		timenow;
    char	pbuf[ GM_MIB_PLACE_LEN ];
    bool	got_places_sem = FALSE;
    SPTREE	node_tree;
    GM_REGISTER_BLK *reg;

    timenow = TMsecs();
    do
    {
	/* run query against it's GCN */

	(void) GM_ask_gcn( &node_tree, vnode_place->place_key );

	/* lock tree */

	if( OK != (cl_stat = GM_gt_sem( &GM_globals.gwm_places_sem ) ) )
	    break;
	got_places_sem = TRUE;

	if( OK != (cl_stat = GM_gt_sem( &vnode_place->place_sem ) ) )
	    break;

	vnode_place->place_vnode.vnode_expire =
	    timenow + GM_globals.gwm_time_to_live;	

	/* For each one in GCN list, if exists, mark valid now.  Else
	   create new srvr, valid now.  Then delete the node in the
	   GCN list. */

	while( (reg = (GM_REGISTER_BLK *)SPdeq( &node_tree.root )) != NULL )
	{
	    /*
	    ** Name Servers register in their own domains with the
	    ** IINMSVR MIB.  They also register with the registry
	    ** master Name Server using the NMSVR MIB.  A Name Server
	    ** registered with NMSVR MIB is not a part of the target
	    ** domain, so skip it.
	    */ 
	    if ( reg->reg_flags & GCA_RG_NMSVR )
	    {
		GM_free( (PTR)reg );
	    	continue;
	    }

	    STprintf( pbuf, "%s::/@%s",
		     vnode_place->place_blk.key, reg->reg_addr );

	    lookup.key = pbuf;
	    pb = (GM_PLACE_BLK *)SPlookup( &lookup, &GM_globals.gwm_places );
	    if( pb == NULL )
	    {
		/* really new place */
		pb = GM_ad_srvr( pbuf );
		SPinit( &pb->place_srvr.srvr_conns,
		       GM_nat_compare );
	    }
	    else
	    {
		/* update of existing server entry */
	    }

	    if( pb == NULL )
	    {
		GM_error( E_GW0600_NO_MEM );
		break;
	    }

	    /* FIXME ignoing object? */

	    pb->place_srvr.srvr_valid = timenow;
	    STcopy( reg->reg_class, pb->place_srvr.srvr_class );
	    pb->place_srvr.srvr_flags = reg->reg_flags;
	    GM_free( (PTR)reg );
	}

	/* for each one, if it's not valid now, delete it. */
	
	for( pb = (GM_PLACE_BLK *)SPfnext( &vnode_place->place_blk );
	    pb && pb->place_type == GM_PLACE_SRVR;
	     pb = (GM_PLACE_BLK *)SPfnext( &pb->place_blk ) )
	{
	    if( pb->place_srvr.srvr_valid != timenow )
	    {
# ifdef xDEBUG
		TRdisplay( "query vnode ZAPPING server %s %s %x\n",
			  pb->place_key,
			  pb->place_srvr.srvr_class,
			  pb->place_srvr.srvr_flags );
# endif
		GM_zp_srvr( pb );
	    }
	}

	(void) GM_release_sem( &vnode_place->place_sem );

    } while( FALSE );
    
    /* unlock tree */
    
    if( got_places_sem )
	(void)GM_release_sem( &GM_globals.gwm_places_sem );

    if( cl_stat != OK )
    {
	GM_error( E_GW8202_QUERY_VNODE_ERR );
	GM_error( cl_stat );
    }

    return( cl_stat );
}


/*{
** Name: GM_check_vnode		- query vnode if it's time to do so.
**
** Description:
**	If the vnode described by the argument needs to be queried,
**	do so to update our list of servers.  Called while holding the
**	gwm_places_sem.  It will unlock this while doing long-running
**	operations.  It returns whether the semaphore is held (normally
**	TRUE), which might be false in some error cases, or if we got
**	interrupted while waiting.
**	
** Re-entrancy:
**	yes.
**
** Inputs:
**	pb	GM_PLACE_VNODE place_blk for the node.
**
**	    .vnode_expire	< TMsecs() if time to query.
**	
** Outputs:
**	pb->vnode.expire	updated if vnode is queried to
**				TMsecs() + gwm_time_to_live.
**
** Returns:
**	TRUE		if gwm_places_sem is held on exit (normal case).
**	FALSE		if not held (error condition, possibly interrupt).
**
** Side Effects:
**
** History:
**	<manual entries>
*/
static bool
GM_check_vnode( GM_PLACE_BLK *pb )
{
    bool got_places_sem = TRUE;
    i4 timenow = TMsecs();
    
    /* if needed, query the nameserver on the node */
    do
    {
	GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_gcn_checks );

	if( pb->place_vnode.vnode_expire <= timenow )
	{
	    /* don't hold tree sem while doing query! */
	    
	    if( GM_release_sem( &GM_globals.gwm_places_sem ) != OK )
		break;
	    got_places_sem = FALSE;
	    
	    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_gcn_queries );

	    if( GM_query_vnode( pb ) != OK )
		break;
	    
	    if( GM_gt_sem( &GM_globals.gwm_places_sem ) != OK )
		break;
	    got_places_sem = TRUE;
	}
    } while(FALSE);
    
    return( got_places_sem );
}



/*{
** Name:	GM_i_place	- allocate and init place block.
**
** Description:
**	Allocates and sets up a place block for a key, either
**	for a vnode or a server.  Inits all conditions, trees,
**	semaphores, etc., but doesn't set the type field or
**	name the semaphore.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	key		name of vnode or server to use.
**	ppb		pointer to a place block to fill in.
**
** Outputs:
**	ppb		filled in with allocated place block pointer
**			or set NULL.
**
** Returns:
**	pointer to the newly allocated block, or NULL on failure.
**
** Side Effects:
**	Allocates memory with GM_alloc.
**	Any errors are logged to errlog.
**
** History:
**	16-Nov-1992 (daveb)
**	    created.
*/

static GM_PLACE_BLK *
GM_i_place( char *key )
{
    GM_PLACE_BLK	*place_blk;
    GM_SRVR_BLK		*srvrp;
    STATUS		cl_stat = OK;
    i4			len;
    
    do
    {
	len = sizeof( *place_blk ) + STlength( key ) + 1;
	place_blk = (GM_PLACE_BLK *)GM_alloc( len );
	if( place_blk == NULL )
	{
	    /* allocation error already logged by GM_alloc */
	}
	else
	{
	    /* copy key into safe space we allocated above */

	    place_blk->place_blk.key = place_blk->place_key;
	    STcopy( key, place_blk->place_key );
	    
	    /* setup our semaphore */
	    if( OK != (cl_stat = GM_i_sem( &place_blk->place_sem ) ) )
		break;
    
	    /* always clear this */
	    place_blk->place_vnode.vnode_expire = 0;
	    place_blk->place_cursor = NULL;

	    srvrp = &place_blk->place_srvr;

	    CScnd_init( &srvrp->srvr_cnd );
	    srvrp->srvr_connects = 0;
	    srvrp->srvr_valid = 0;
	    srvrp->srvr_flags = 0;
	    srvrp->srvr_class[0] = EOS;
	    srvrp->srvr_state = GM_SRVR_OK;
	    SPinit( &srvrp->srvr_conns, GM_nat_compare );
	}

    } while( FALSE );

    /* Any bad stat logged by GM_i_sem */
    if( cl_stat != OK )
    {
	GM_free( (PTR)place_blk );
	place_blk = NULL;
    }
    return( place_blk );
}




/*{
** Name:	GM_conn_error	- note error on a connection.
**
** Description:
**	Notes error happened on the connection.  This means we should
**	check the vnode's iigcn again to validate the server list.
**
** Inputs:
**	conn		the connection that had a problem.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Side Effects:
**
** History:
**	16-Nov-1992 (daveb)
**	    created.
*/

void
GM_conn_error( GM_CONN_BLK *conn )
{
    GM_PLACE_BLK *vnode;

    for( vnode = conn->conn_place;
	vnode && vnode->place_type != GM_PLACE_VNODE ;
	vnode = (GM_PLACE_BLK *)SPfprev( &vnode->place_blk ) )
	continue;

    if( vnode != conn->conn_place )
	GM_gt_sem( &vnode->place_sem );

    vnode->place_vnode.vnode_expire = 0;

    if( vnode != conn->conn_place )
	GM_release_sem( &vnode->place_sem );
}


static char *
GM_conn_state( GM_CONN_BLK *conn )
{
    switch( conn->conn_state )
    {
    case GM_CONN_UNUSED:
	return( "GM_CONN_UNUSED" );
    case GM_CONN_FORMATTED:
	return( "GM_CONN_FORMATTED" );
    case GM_CONN_LOADING:
	return( "GM_CONN_LOADING" );
    case GM_CONN_LOADED:
	return( "GM_CONN_LOADED" );
    case GM_CONN_GCM_WORKING:
	return( "GM_CONN_GCM_WORKING" );
    case GM_CONN_GCM_DONE:
	return( "GM_CONN_GCM_DONE" );
    case GM_CONN_UNLOADING:
	return( "GM_CONN_UNLOADING" );
    case GM_CONN_EMPTIED:
	return( "GM_CONN_EMPTIED" );
    case GM_CONN_ERROR:
	return( "GM_CONN_ERROR" );
    default:
	return( "(unknown CONN state)" );
    }
}

static void
GM_conn_dump( GM_CONN_BLK *conn )
{
    TRdisplay("CONN BLK %p, key %x\n", conn, conn->conn_blk.key );
    TRdisplay("\tsem %p, place %p (%s), state %d: %s\n",
	      &conn->conn_sem,
	      conn->conn_place,
	      conn->conn_place->place_key,
	      conn->conn_state,
	      GM_conn_state( conn ) );
    TRdisplay("\trsb %p, relevant buf:\n", conn->conn_rsb );
    GM_dumpmem( conn->conn_data_start,
	       conn->conn_data_last - conn->conn_data_start );
}

static void
GM_req_conn( GM_CONN_BLK *conn )
{
    GM_opblk_dump( &conn->conn_rsb->request );
}

static void
GM_resp_conn( GM_CONN_BLK *conn )
{
    GM_opblk_dump( &conn->conn_rsb->response );
}

/*{
** Name:	GM_key_place	- find key place for (partial) place
**
** Description:
**	Finds the place in the user domain that corresponds to a
**   	requested position key.  This is the last place in the user
**   	domain that has a complete match for the key provided.  If
**  	No place in the domain matches the partial key, then the key
**  	is set to the "impossibly" large key value.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	rsb 	    	holds cur_key and dom_type we're checking
**  	    .cur_key	what it was set to
**  	    .dom_type	what we want for keys.
**
** Outputs:
**	key
**  	    .cur_key	maybe changed to a valid key of dom_type.
**
** Returns:
**	none.
**
** Exceptions: (if any)
**	none
**
** History:
**      18-Mar-1994 (daveb) 60738
**          created.
*/


VOID
GM_key_place( GM_RSB *gwm_rsb )
{
    GM_GKEY save_key;		/* last good key we found */
    GM_GKEY *key;		/* key we're checking/changing */
    GM_GKEY *tkey;		/* temp key we're using */
    GM_RSB  trsb;		/* rsb to use for lookup */
    STATUS  cl_stat;		
    i4	    cmp_val;

    /* key we're validating */
    key = &gwm_rsb->cur_key;

    /* possible result */
    save_key.place.len = 0;
    save_key.place.str[0] = EOS;

    /* setup fake lookup place, same as rsb but changeable */
    trsb = *gwm_rsb;		/* structure assignment */

    /* tkey is the one we've lookup up, maybe bad */
    tkey = &trsb.cur_key;
    tkey->place.len = 0;	/* start with empty key to force scan */
    tkey->place.str[0] = EOS;

    GM_rsb_place( &trsb );
    while( (cmp_val = STbcompare( tkey->place.str, tkey->place.len,
				 key->place.str, key->place.len, 
				 FALSE )) <= 0 )
    {
	/* if partial key matches this, then it might be the one. */

	save_key.place = tkey->place;

	/* take the first of any partial matches and stop. */
	if( cmp_val == 0 )
	    break;

	/* try next place in the domain */
	if( OK != (cl_stat = GM_rsb_place( &trsb )))
	{
	    /* no match at all, place key at the end */
	    if( save_key.place.len == 0 )
	    {
		/* copy empty string, padded with 0xff */
		STmove( "", ~0, sizeof(tkey->place.str),
		       tkey->place.str );
		tkey->place.len = sizeof(tkey->place.str);
	    }
	    break;
	}
    }
    if( STcompare( key->place.str, save_key.place.str ) )
    {
	TRdisplay(" changed partial key %s to %s in domain\n",
		  key->place.str, save_key.place.str );
	key->place = save_key.place;
    }
}
