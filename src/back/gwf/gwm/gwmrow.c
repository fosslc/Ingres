/*
**Copyright (c) 2004 Ingres Corporation
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

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"
# include "gwmxtab.h"

# ifndef xDEBUG
# define xDEBUG
# endif

/**
** Name: gwmrow.c	- IMA gateway subs for row ops.
**
** Description:
**
**    This file contains functions that do row oriented operations
**    on GM opblocks.  It's distinct from the object at a time operations
**    in gwmop.c (GM_op, GM_lop, and GM_rop) because it does row
**    oriented caching.  Even that is questionable!
**
**	GM_request	-- grind row opblk request, local or remote
**
**	GM_lrequest 	-- Handle a local row request using MO.
**	GM_rrequest	-- Handle a local row request using GCM.
**
** History:
**	19-feb-91 (daveb/bryanp)
**	    Modified from gwftest/gwflgtst.c for proof of concept.
**	6-Nov-91 (daveb)
**	    Modified from gwf_mi_works.c to gwf_mo.c now that MI
**	    has mutated.
**	15-Jan-92 (daveB)
**	    Much modified from above.
**	16-Sep-92 (daveb)
**		Make sure to set db_stat on NO_NEXT cl_stat.
**	9-oct-92 (daveb)
**		broke into gwmxtab.c and gwmxutil.c
**	13-Nov-1992 (daveb)
**	    simply GM_ad_to_tuple by using new GM_scanresp to scan
**	    GETRESP block.  Fix local_request: perms are a i4, not
**	    a string in the getresp block.
**	14-Nov-1992 (daveb)
**	    In GXrequest, take 0 length place as this server too.
**	17-Nov-1992 (daveb)
**	    move GX_next_place inline to GXget
**	19-Nov-1992 (daveb)
**	    include <gca.h> now that we're using GCA_FS_MAX_DATA for the
**	    OPBLK buffer size.
**	19-Nov-1992 (daveb)
**	    split into gwmxutil.c, which knows about GX_RSBs, and
**	    and gwmrow.c, which does row-oriented OPBLOCK operations.
**	24-Nov-1992 (daveb)
**	    take guises from the RSB rather than calculating them all the time.
**	1-Feb-1993 (daveb)
**	    Fix typo, update some document related stuff for accuracy.
**	    In the past, GM_request was GXrequest, GM_lrequest was
**	    GX_local_request, and GM_rrequest was GX_remote_request.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	19-Aug-1993 (daveb)
**	    Positioned place wasn't working right, because it was going
**	    into the GM_GET case without an instance value.  When instance
**	    is 0 length, go into the GM_GETNEXT case to locate the first
**	    instance in the place.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**/

/* forward */

static bool GM_resp_cache( char *kinstance, GM_RSB *gm_rsb, STATUS *cl_stat );

static DB_STATUS GM_lrequest( GM_RSB *gm_rsb,
			     GM_GKEY *key,
			     STATUS *cl_stat );

static DB_STATUS GM_rrequest( GM_RSB *gm_rsb,
			     GM_GKEY *key,
			     STATUS *cl_stat );



/* ----------------------------------------------------------------
**
** Row oriented request related
*/

/*{
** Name:	GM_request	-- grind ROW opblk request, local or remote
**
** Description:
**
**    Given a request for a row, fill in a response with the right data.
**
**    If we hit EOF, as indicated by actual return, mismatched classid
**    or instance, return E_DB_ERROR and cl_stat of MO_NO_NEXT.
**
**    On successful GETNEXT, update the input key to be new classid.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gm_rsb		the stream in question
**	    .request	the request to process.
**	    .response	the returned answer.
**
**	key		key space used up
**
** Outputs:
**	gm_rsb		the stream in question
**	    .response	the returned answer.
**
**	key		key space used up
**
**	cl_stat		filled in with error stat if any, and MO_NO_NEXT
**			if we hit end of scan.
**
** Returns:
**	E_DB_OK		if all right.
**	E_DB_ERROR	on any error, including EOF.
**
**
** History:
**	22-sep-92 (daveb)
**	    documented.
**	14-Nov-1992 (daveb)
**	    Take 0 length place as this server too.
*/
DB_STATUS
GM_request( GM_RSB *gm_rsb, GM_GKEY *key, STATUS *cl_stat )
{
    DB_STATUS db_stat;

    if( *gm_rsb->request.place == EOS ||
       !STcompare( gm_rsb->request.place, GM_my_server() ) ||
       !STcompare( gm_rsb->request.place, GM_my_vnode() ) )
    {
	db_stat = GM_lrequest( gm_rsb, key, cl_stat );
    }
    else
    {
	db_stat = GM_rrequest( gm_rsb, key, cl_stat );
    }

    return( db_stat );
}



/**
** Name: GM_lrequest -- Handle a local ROW request using MO.
**
**    Given a request for a row, fill in a response with the right data.
**    Uses the same instance for all classid's, and for getnext, only
**    looks up the index once, for the first classid encountered.
**
**    If we hit EOF, as indicated by actual return, mismatched classid
**    or instance, return E_DB_ERROR and cl_stat of MO_NO_NEXT.
**
**    On successful GETNEXT, update the input key to be new classid.
**
** Inputs:
**
**	gm_rsb
**
**	    request
**			The request we'll process.
**
**	key		is buffer space we will be using.  We don't
**			look at the current contents
**
** Outputs:
**
**	gm_rsb
**
**	    response
**			Our output location.
**
**	key		will be filled in with the last classid/instance
**			we looked up.  Place is ignored.
**
**	cl_stat		Filled in if return != E_DB_OK; usually MO_NO_NEXT
**
** Returns:
**	E_DB_OK, or E_DB_ERROR; if EOF, E_DB_ERROR and cl_stat MO_NO_NEXT.
**
** History:
**	2-jul-92 (daveb)
**		account for EOS when calculating classid/instance lengths
**		using pointer arithmetic.
**	17-jul-92 (daveb)
**		Can't use cur_key for buffer space, because we overwrite
**		the instance that would be used in subsequent partial
**		row getnext operations.  Instead, use key space handed in
**		that won't conflict with cur_key, making it the caller's
**		problem to update cur_key when a whole row has been
**		acquired.
**	16-Sep-92 (daveb)
**		Make sure to set db_stat on NO_NEXT cl_stat.
**	13-Nov-1992 (daveb)
**	    perms in RESP block is now a nat.
**	24-Nov-1992 (daveb)
**	    take guises from the RSB rather than calculating them all
**	    the time.
**	19-Aug-1993 (daveb)
**	    Positioned place wasn't working right, because it was going
**	    into the GM_GET case without an instance value.  When instance
**	    is 0 length, go into the GM_GETNEXT case to locate the first
**	    instance in the place.
*/
static DB_STATUS
GM_lrequest( GM_RSB *gm_rsb, GM_GKEY *key, STATUS *cl_stat )
{
    DB_STATUS db_stat = E_DB_OK;
    GM_OPBLK *request = &gm_rsb->request;
    GM_OPBLK *response = &gm_rsb->response;

    i4  element;
    char *p;
    char *q;
    
    i4  lobuf;
    char obuf[ GM_MIB_VALUE_LEN ];
    i4 operms;
    
    i4  used = 0;
    i4  op = request->op;
    
    GM_i_opblk( GM_GETRESP, request->place, response );
    
    /* copy instance from buf to key, with length */

    for( p = request->buf; *p++ ; )
	continue;
    for( q = key->instance.str; *q++ = *p++ ; )
	continue;	    
    key->instance.len = q - key->instance.str - 1;
	
    /* for each item, get and put while there's room */
    
    p = request->buf;
    for( element = 0; element < request->elements; element++ )
    {
	lobuf = sizeof(obuf);
	
	/* always take classid from buffer to key */

	for( q = key->classid.str; *q++ = *p++ ; )
	    continue;	    
	key->classid.len = q - key->classid.str - 1;
	
	/* if no instance, need to getnext a GET to get the first */

	if( op == GM_GETNEXT || key->instance.len == 0 ) /* first time only */
	{
	    /* make the instance in the buffer be the key */

	    for( q = key->instance.str; *q++ = *p++ ; )
		continue;
	    key->instance.len = q - key->instance.str - 1;
	    
	    *cl_stat = MOgetnext( gm_rsb->guises,
				 sizeof(key->classid.str),
				 sizeof(key->instance.str), 
				 key->classid.str,
				 key->instance.str,
				 &lobuf,
				 obuf,
				 &operms );
	    
	    if( *cl_stat != OK )
	    {
		break;
	    }
	    else if ( STcompare( request->buf, key->classid.str ) )
	    {
		*cl_stat = MO_NO_NEXT;
		break;
	    }
		
	    /* Subsequent gets on this row will use the same instance,
	       so turn the op back to straight get. */
	    
	    op = GM_GET;
	    key->classid.len = STlength( key->classid.str );
	    key->instance.len = STlength( key->instance.str );
	    key->valid = TRUE;
	}
	else if( op == GM_GET )	/* first or subsequent cases */
	{
	    /* use the instance in the key, not from buffer */
	    while( *p++ )
		continue;

	    *cl_stat = MOget( gm_rsb->guises,
			     key->classid.str,
			     key->instance.str,
			     &lobuf, obuf, &operms );
	    if ( *cl_stat != OK )
		break;
	}
	else			/* unknown OP */
	{
	    /* "GWM Internal error:  unexpected OPBLK op %0d" */

	    GM_1error( (GWX_RCB *)0, E_GW8046_BAD_OP, 
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( op ), (PTR)&op );
	    *cl_stat = MO_BAD_MSG;
	    break;
	}
	
	/* now pack the response buffer */
	
	if( E_DB_OK !=
	   GM_str_to_opblk( response, key->classid.len, key->classid.str ) )
	    break;
	if( E_DB_OK !=
	   GM_str_to_opblk( response, key->instance.len, key->instance.str ) )
	    break;
	if( E_DB_OK !=
	   GM_str_to_opblk( response, STlength(obuf), obuf ) )
	    break;
	if( E_DB_OK != 
	   GM_pt_to_opblk( response, sizeof(operms), (char *)&operms ) )
	    break;
	used = response->used;
	response->elements++;
    }

    if( *cl_stat != OK )
    {
	response->err_element = element;
	db_stat = E_DB_ERROR;
    }

    response->used = used;
    response->status = *cl_stat;

    return( db_stat );
}


/**
** Name: GM_rrequest -- Handle a remote ROW request using GCM.
**
**    Handle a remote request using GCM.  
**
**    Given a request for a row, fill in a response with the right data.
**    On successful GETNEXT, update the input key to be new classid.
**
**    If we hit EOF, as indicated by actual return, mismatched classid
**    or instance, return E_DB_ERROR and cl_stat of MO_NO_NEXT.
**
**    This version handles additional rows that might be present
**    in the response buffer.  If the first row matches the key,
**    we move the next row down to the beginning of the buffer.
**
**    FIXME:  this is very complicated, and does copies where
**	      pointers should be used.
**
** Inputs:
**
**	gm_rsb
**
**	    gwm_rsb.request
**			The request we'll process.
**
**	key		is buffer space we will be using.  We don't
**			look at the current contents.
**
** Outputs:
**
**	gm_rsb
**
**	    gwm_rsb.response
**			Our output location.
**
**	key		will be filled in with the last classid/instance
**			we looked up.  Place is ignored.
**
**	cl_stat		Filled in if return != E_DB_OK; usually MO_NO_NEXT
**
** Returns:
**	E_DB_OK, or E_DB_ERROR; if EOF, E_DB_ERROR and cl_stat MO_NO_NEXT.
**
** History:
**	2-jul-92 (daveb)
**		account for EOS when calculating classid/instance lengths
**		using pointer arithmetic.
**	17-jul-92 (daveb)
**		Can't use cur_key for buffer space, because we overwrite
**		the instance that would be used in subsequent partial
**		row getnext operations.  Instead, use key space handed in
**		that won't conflict with cur_key, making it the caller's
**		problem to update cur_key when a whole row has been
**		acquired.
**	16-Sep-92 (daveb)
**		Make sure to set db_stat on NO_NEXT cl_stat.
**	17-Nov-1992 (daveb)
**	    Be sure to update key!  It's an output variable!
**	    Also, check for overrun (NO_NEXT).
**	19-Nov-1992 (daveb)
**	    Make read-ahead work.
*/
static DB_STATUS
GM_rrequest( GM_RSB *gm_rsb, GM_GKEY *key, STATUS *cl_stat )
{
    DB_STATUS		db_stat = E_DB_OK;
    GM_OPBLK		*response = &gm_rsb->response;
    
    char *p;			/* cursor into response->buf */
    
    char *classid;		/* extracted from buf */
    char *instance;		/* extracted from buf */
    char *value;		/* extracted from buf */
    i4  perms;			/* extracted from buf */
    i4  i;
    
    do				/* one time only. */
    {
	GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_rreq_calls );
	
	/* check for pre-read data (op can only be GET or GETNEXT)  */

	if( GM_resp_cache( key->instance.str, gm_rsb, cl_stat ) )
	{
	    i = 0;		/* DEBUG */
	}
	else
	{
	    /* nope, need to get more data from the remote */

	    db_stat = GM_gcm_msg( gm_rsb, cl_stat );
	    if( db_stat == E_DB_OK )
	    {
		i = 0;		/* DEBUG */
	    }
	    else
	    {
		/* he's toast, pass error back up the chain */
		if( *cl_stat != MO_NO_NEXT || response->err_element <= 0 )
		    break;

		/* On NO_NEXT in pre-read stuff, truncate buffer to
		   good stuff and pretend we got it cleanly.  The
		   only trick here is to find a new "used" value */

		p = response->buf;
		for( i = 0; i < response->err_element; i++ )
		    GM_scanresp( &p, &classid, &instance, &value, &perms );

		/* p now at classid spot of bad element, i = err_element */

		response->used = p - response->buf;
		response->elements = i;
		response->err_element = -1;
		response->status = *cl_stat = OK;
		db_stat = E_DB_OK;
	    }
	}

	/* copy the newly found output instance to the key. */

	p = response->cursor;
	GM_scanresp( &p, &classid, &instance, &value, &perms );
	STcopy( instance, key->instance.str );
	key->instance.len = value - instance - 1;

    } while( FALSE );

    /* pass up error status(es) */

    if( *cl_stat == OK )
	*cl_stat = response->status;

    if( db_stat != E_DB_OK || *cl_stat != OK )
	GM_breakpoint();

    return( db_stat );
}


/**
** Name: GM_resp_cache -- Check response block for row read-ahead.
**
**    If it looks like the response block already has the stuff
**    we want baased on the request block, setup up it's cursor
**    and cur_element, and return TRUE.  Otherwise return FALSE.
**
**    If it looks like we hit EOF, then set cl_stat to NO_NEXT
**    and return FALSE.
**
** Inputs:
**
**	gm_rsb
**
**	    .request
**		.buf	the classid of interest.
**			The request we'll process.
**
**	    .response
**			the maybe-reused response to check.
**
**	kinstance	the instance in question.
**
**	cl_stat		to fill in if EOF found.
**
** Outputs:
**
**	gm_rsb
**
**	    response	output location
**
**		.cursor
**		.curs_element
**
**	cl_stat		sometimes filled in with MO_NO_NEXT if FALSE.
**
** Returns:
**	TRUE		if found and cursor set up.
**	FALSE		if not found, maybe setting cl_stat to MO_NO_NEXT.
**
** History:
**	19-Nov-1992 (daveb)
**	    split from GXremote_request for sanity.
**	1-Feb-1993 (daveb)
**	    Document name change from GX_resp_cache to GM_resp_cache.
*/

static bool
GM_resp_cache( char *kinstance, GM_RSB *gm_rsb, STATUS *cl_stat )
{
    bool		found = FALSE;

    GM_OPBLK		*request = &gm_rsb->request;
    GM_OPBLK		*response = &gm_rsb->response;

    char *lastloc;		/* last good place in buf. */
    char *p;			/* cursor into response->buf */
    char *classid;		/* extracted from buf */
    char *instance;		/* extracted from buf */
    char *value;		/* extracted from buf */
    i4  perms;			/* extracted from buf */
    i4  obj;
    i4  element;
    
    if( response->elements <= 0	||
       response->status != OK	||
       response->op != GM_GETRESP			||
       STcompare( request->place, response->place )     	||
       GM_globals.gwm_gcm_usecache == 0 )
    {
	found = FALSE;
    }
    else
    {
	/* Check the cached response block -- pick out what's there */
	
	p = GM_keyfind( request->buf, kinstance, response, &element );
	if( p == NULL )
	{
	    found = FALSE;
	}
	else
	{
	    response->cursor = p;
	    response->curs_element = element;
	    
	    /* if GET, we're done; on GETNEXT, we need
	       to find the _next_ "row" that is here, if any */
	    
	    if( request->op == GM_GET )
	    {
		found = TRUE;
	    }
	    else if( request->op == GM_GETNEXT )
	    {
		
		/* Skip past all the objects for the column and read one
		   more to get the first of the "next" row which follows. */
		
		lastloc = &response->buf[ response->used ];
		for( obj = 0; obj <= request->elements && p < lastloc; obj++ )
		    GM_scanresp( &p, &classid, &instance, &value, &perms );
		
		if( response->curs_element + obj < response->elements &&
		   p < lastloc )
		{
		    if( !STcompare( classid, request->cursor ) )
		    {
			response->cursor = classid;
			response->curs_element += (obj - 1);
			found = TRUE;
		    }
		    else
		    {
			/* Next should have been here, give it up now. */
			response->status = MO_NO_NEXT;
			response->err_element = response->curs_element;
		    }
		}
		else		/* ran out of elements or space */
		{
		    found = FALSE;
		}
	    }
	    else		/* bad OP; log? */
	    {
		found = FALSE;
	    }
	}
    }

    if( found )
	GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_rreq_hits );
    else
	GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_rreq_misses );

    if( *cl_stat != OK )
	GM_breakpoint();

    return( found );
}
