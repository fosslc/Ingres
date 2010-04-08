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
# include <st.h>
# include <tm.h>
# include <tr.h>
# include <erglf.h>
# include <sp.h>
# include <mo.h>
# include <scf.h>
# include <adf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <ulf.h>
# include <ulm.h>
# include <gca.h>
# include <gcm.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"
# include "gwmplace.h"

# ifndef xDEBUG
# define xDEBUG
# endif

/*
** Name: gwmgcm.c -- GCM communication routines for GWM/IMA
**
** Description:
**
** Exports the following functions:
**
**	GM_cancel	- cancel outstanding ops on a connection.
**	GM_gcm_msg	- do GWM message from OPBLKs.
**
** Internal functions:
**
**	GM_prep_conn	- translate OPBLK to GCM message.
**	GM_load_gcm	- set up a GCM message from an OPBLK
**	GM_pack_element - put one opblk element into a GCM buffer
**	GM_gcmsend	- make GCa_call for GCM request.
**	GM_cgcm		- GCM completion routine for fastselect
**	GM_unload_gcm	- unpack GCM response to an OPBLK.
**	GM_unpack_element	- unpack an element in a GCM msg to an OPBLK
**	GM_op_to_msg	- map OPBLK op to GCA msg.
**	GM_msg_to_op	- map GCA message to opblk response op.
**	GM_ptStrGcm	- put a string to GCM message in GM_CONN_BLK
**	GM_ptNatGcm	- put integer to GCA message in a GM_CONN_BLK.
**	GM_ptLnGcm	- put integer to GCA message in a GM_CONN_BLK.
**	GM_gtStrGcm	- move string from CONN_BLK to OPBLK
**	GM_gtNatGcm	- get a number out of the GCA msg in the CONN_BLK.
**	GM_gtLnGcm	- get a i4 out of the GCA msg in the CONN_BLK.
**
** History:
**	17-sep-92 (daveb)
**	    Created.
**	8/9-oct-92 (daveb)
**	    added the real guts.
**	11-Nov-1992 (daveb)
**	    force default connection in GM_prep_conn.
**	14-Nov-1992 (daveb)
**	    handle all GCM errors as NO_NEXT on this place.  All conn routines
**	    get *cl_stat's so they may report errors nicely.  
**	20-Nov-1992 (daveb)
**	    handle errs as NO_INSTANCE, so, not to conflict with pre-read.
**	24-Nov-1992 (daveb)
**	    take guises from the RSB rather than calculating them all the time.
**	3-Dec-1992 (daveb)
**	    Simplfy all the setup in GM_prep_conn; it's transparent.
**	2-Feb-1993 (daveb)
**	    Improve comments.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	    Changed GM_cgcm() to take PTR rather than i4 async_id.
**	24-jan-1994 (gmanning)
**	    Change unsigned i4  to u_i4 in order to compile on NT.
**	23-aug-95 (wonst02) Fix managed objects (MO) in Windows/NT
**	    The async_id passed to GM_cgcm is really an SCB pointer, not a SID,
**	    so get the SID from the SCB and pass it to CSresume.
**      24-Jul-1997 (stial01)
**          GM_prep_conn() conn_buffer is now 'conn->conn_buffer_size'
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      23-Jun-99 (hweho01)
**          Added *CS_find_scb() function prototype. Without the 
**          explicit declaration, the default int return value   
**          for a function will truncate a 64-bit address
**          on ris_u64 platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Deleted CS_find_scb() prototype, now correctly typed
**	    in cs.h.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
*/

typedef DB_STATUS GM_GCM_FUNC( GM_CONN_BLK *conn, STATUS *cl_stat );

/* missing from <gca.h> */

FUNC_EXTERN STATUS IIGCa_call();

/* forwards */

static GM_GCM_FUNC GM_prep_conn;
static GM_GCM_FUNC GM_load_gcm;
static GM_GCM_FUNC GM_gcmsend;
static GM_GCM_FUNC GM_unload_gcm;

static void GM_cgcm( PTR async_id );

static DB_STATUS GM_pack_element( i4  op, char **op_loc, GM_CONN_BLK *conn );

static void GM_msg_to_op( GM_CONN_BLK *conn, i4  *op );
static DB_STATUS GM_op_to_msg( i4  op, i4  *msg );

static DB_STATUS GM_ptStrGcm( char *src, GM_CONN_BLK *conn );
static DB_STATUS GM_ptNatGcm( i4  val, GM_CONN_BLK *conn );
static DB_STATUS GM_ptLnGcm( i4 val, GM_CONN_BLK *conn );

static void GM_gtNatGcm( GM_CONN_BLK *conn, i4  *val );
static void GM_gtLnGcm( GM_CONN_BLK *conn, i4 *val );
static DB_STATUS GM_gtStrGcm( GM_CONN_BLK *conn );

/*{
** Name: GM_cancel	- cancel outstanding ops on a connection.
**
** Description:
**	Given a GM_CONN_BLK, cancel any outstanding operations; we
**	are about to vaporize the block and don't want anything going
**	on.
**
**	Currently a no-op, but may be needed later.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn_blk	the connection in question.
**
** Outputs:
**	conn_blk	may be written.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/
VOID
GM_cancel( GM_CONN_BLK *conn_blk )
{
}



/*{
** Name: GM_gcm_msg	- do GWM message from OPBLKs.
**
** Description:
**	Given a request OPBLK and a response OPBLK, go do the
**	right GCM messaging to make it work.
**
**	The GCM message may not handle all the elements requested in the 
**	OPBLK, and the OPBLK may not be able to hold all the result back
**	from GCM.  The calling code must be prepared to handle these
**	truncations without error.
**
**	In this version, we only support "fast-select" queries;
**	in the future, we may keep some history and decide to
**	keep longer lived connections.  (Connections are complicated
**	because there are only the pre-ordained completion handlers
**	available for non-fast select connecitons, and they live in
**	SCF.)
**
**	The steps are:
**
**	(1) Acquire a CONNECT block to the place in question.
**	(2) prepare the CONNECT block
**	(3) translate the request OPBLK to the buffer in the CONNECT
**		block.
**	(4) send it via FASTSELECT;
**	(5) unpack it into the response opblk.
**	(6) release the CONNECT block.
**
**
**	In retrospect, this might better have been done with a state-machine.
**
** Re-etnrancy:
**	yes.
**
** Inputs:
**	request		OPBLK of data to send to other place.
**	response	OPBLK to put response.
**	cl_stat		place to put error.
**
** Outputs:
**	response	filled in.
**	cl_stat	        filled in on error.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Side Effects:
**	Many errors may be logged and lost on the way down and back up.
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    take args in rsb now.
**	14-Nov-1992 (daveb)
**	    handle any error as a no-next.  This isn't the best way.
**	    handle all GCM errors as NO_NEXT on this place.  All conn routines
**	    get *cl_stat's so they may report errors nicely.
*/

DB_STATUS
GM_gcm_msg( GM_RSB *gm_rsb, STATUS *cl_stat )
{
    DB_STATUS		db_stat = E_DB_ERROR;
    GM_CONN_BLK		*conn;
    GM_OPBLK		*request = &gm_rsb->request;
    GM_OPBLK		*response = &gm_rsb->response;
    i4			got_sem = FALSE;

    /* setup the response block */
    GM_i_opblk( GM_BADOP, request->place, response );
    response->status = *cl_stat = MO_NO_INSTANCE;

    do
    {
	if( (conn = GM_ad_conn( request->place )) == NULL )
	    break;
	
	if( (db_stat = GM_gt_sem( &conn->conn_sem )) != E_DB_OK )
	    break;
	got_sem = TRUE;
	
	conn->conn_rsb = gm_rsb;
	
	*cl_stat = OK;

	if( (db_stat = GM_prep_conn( conn, cl_stat )) != E_DB_OK )
	    break;
	
	if( (db_stat = GM_load_gcm( conn, cl_stat )) != E_DB_OK )
	    break;
	
	if( (db_stat = GM_gcmsend( conn, cl_stat )) != E_DB_OK )
	    break;
	
	if( (db_stat = GM_unload_gcm( conn, cl_stat )) != E_DB_OK )
	    break;
	
    } while( FALSE );
    
    if( got_sem )
	GM_release_sem( &conn->conn_sem );

    if( conn != NULL )
	GM_rm_conn( conn );

    return( db_stat );
}


/*{
** Name: GM_prep_conn	- translate OPBLK to GCM message.
**
** Description:
**	Translate a GM_OPBLK to an appropriate GCM message, as contained
**	in the GM_CONN_BLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	request		the opblk with the request.
**	conn		the connect block to fill
**
** Outputs:
**	conn		filled in.
*
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    Force default connection here.  Pass protocol level.
**	14-Nov-1992 (daveb)
**	    take *cl_stat to fill in.  Separate local and remote cases,
**	    though remote isn't known to work.
**	3-Dec-1992 (daveb)
**	    Simplfy all the setup; it's transparent.
**      24-Jul-1997 (stial01)
**          GM_prep_conn() conn_buffer is now 'conn->conn_buffer_size'
*/

static DB_STATUS
GM_prep_conn( GM_CONN_BLK *conn, STATUS *cl_stat )
{
    DB_STATUS		db_stat = E_DB_OK;
    STATUS		local_status;
    GM_OPBLK		*request = &conn->conn_rsb->request;
    GCA_FS_PARMS	*fs_parms;

    *cl_stat = OK;
    if( conn->conn_state == GM_CONN_EMPTIED )
    {
	conn->conn_data_last = conn->conn_data_start;
    }
    else			/* first time this CONN_BLK */
    {
	conn->conn_data_start =
		conn->conn_data_last = conn->conn_buffer;
	
	conn->conn_data_length = conn->conn_buffer_size;
    } 

    if( db_stat == E_DB_OK )
    {
	fs_parms = &conn->conn_fs_parms;

	db_stat = GM_op_to_msg( request->op, &fs_parms->gca_message_type );

	fs_parms->gca_partner_name = request->place;

	/* Take me as I am:  ingres */
	fs_parms->gca_user_name = NULL;
	fs_parms->gca_password = NULL;
	fs_parms->gca_account_name = NULL;
	
	fs_parms->gca_modifiers = GCA_RQ_GCM;
	fs_parms->gca_buffer = conn->conn_buffer;
	fs_parms->gca_b_length = conn->conn_buffer_size;
	fs_parms->gca_completion = GM_cgcm;
	fs_parms->gca_peer_protocol = GCA_PROTOCOL_LEVEL_60;
	fs_parms->gca_closure = (PTR)conn;

	conn->conn_state = GM_CONN_FORMATTED;
    }

    return( db_stat );
}


/*{
** Name: GM_load_gcm - set up a GCM message from an OPBLK
**
** Description:
**	Prepares GCM message from an OPBLK.  If the OPBLK
**	has more than will fit, than it is dropped silently.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn		the connection with a GCM buffer to fill,
**
** Outputs:
**	conn		GCM message filled in.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	12-Nov-1992 (daveb)
**	    bump last after skippling element first time through.
**	14-Nov-1992 (daveb)
**	    take *cl_stat to fill in.  conn now has rsb instead
**	    of direct pointer to the request.  Remember to account
**	    for space of element count location.  Use = instead of ==
**	    for equality comparison!
**	24-Nov-1992 (daveb)
**	    take guises from the RSB rather than calculating them all the time.
*/
static DB_STATUS
GM_load_gcm( GM_CONN_BLK *conn, STATUS *cl_stat )
{
    GM_OPBLK	*request = &conn->conn_rsb->request;
    DB_STATUS	db_stat = E_DB_OK;
    i4		perms;
    i4		elements;
    char	*elem_loc;
    char	*op_loc;
    char	*valid_msg;

    *cl_stat = OK;
    conn->conn_state = GM_CONN_LOADING;

    /*
    ** Setup the header
    */

    /* skip past error_status, error_index, future[0] and [1]  */

    conn->conn_data_last += (sizeof(i4) + (3 * sizeof(i4)));

    /* We don't have session permission off this process */
    perms = conn->conn_rsb->guises & ~(MO_SES_READ|MO_SES_WRITE);

    /* put permissions claimed for this session */
    GM_ptNatGcm( perms, conn );

    GM_ptNatGcm( GM_globals.gwm_gcm_readahead, conn );

    /* mark element count location for now */
    elem_loc = conn->conn_data_last;
    conn->conn_data_last += sizeof(i4);

    /* Put the things in the opblk -- if no room, just stop. */

    op_loc = request->buf;
    valid_msg = conn->conn_data_last;
    for( elements = 0;
	db_stat == E_DB_OK && op_loc < &request->buf[ request->used ];
	elements++ )
    {
	db_stat = GM_pack_element( request->op, &op_loc, conn );
	if( db_stat == OK )
	    valid_msg = conn->conn_data_last;
	else
	    break;
    }
    conn->conn_data_last = valid_msg;

    /* now that we know it, put the element count */

    MEcopy( (PTR)&elements, sizeof(elements), (PTR)elem_loc );

    conn->conn_fs_parms.gca_msg_length =
	conn->conn_data_last - conn->conn_data_start;

    conn->conn_state = GM_CONN_LOADED;

    return( E_DB_OK );
}


/*}
** Name: GM_pack_element - put one opblk element into a GCM buffer
**
** Description:
**
**	Formats the next element in an OPBLK buffer into a GCM
**	buffer, checking for space.  Only complete requests
**	are written.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		GM_GET | GM_GETNEXT.
**	op_loc		pointer to pointer to next opblk element
**	conn		the conn to put it into.
**
** Outputs:
**	op_loc		pointer to pointer to next opblk element
**	data_area	pointer to pointer to next GCM buffer space.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/
static DB_STATUS
GM_pack_element( i4  op, char **op_loc, GM_CONN_BLK *conn )
{
    DB_STATUS	db_stat;
    char	*classid;
    char	*instance;
    char	*value;
    char	*save_op_loc;
    char	*save_conn_data_last;

    save_op_loc = *op_loc;
    save_conn_data_last = conn->conn_data_last;

    /* Setup the source locations */

    classid = *op_loc;
    while( *(*op_loc)++ != EOS )	/* skip past EOS */
	    continue;

    instance = *op_loc;
    while( *(*op_loc)++ != EOS )	/* skip past EOS */
	    continue;

    if( op != GM_SET )			/* only have string for SET */
    {
	value = "";
    }
    else				/* this is the set value */
    {
	value = *op_loc;
	while( *(*op_loc)++ != EOS )
	    continue;
    }
	
    /* copy the sources to the GCM message */
    
    do
    {
	if( (db_stat = GM_ptStrGcm( classid, conn )) != E_DB_OK )
	    break;
	if( (db_stat = GM_ptStrGcm( instance, conn )) != E_DB_OK )
	    break;
	if( (db_stat = GM_ptStrGcm( value, conn )) != E_DB_OK )
	    break;
	db_stat = GM_ptNatGcm( 0, conn ); /* no perms at all */

    } while ( FALSE );

    return( db_stat );
}


/*{
** Name: GM_gcmsend	- make GCa_call for GCM request.
**
** Description:
**	Makes the call...
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn		the connect block with the prepared message to send.
**
** Outputs:
**	conn
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	12-Nov-1992 (daveb)
**	    Need GCA_ALT_EXIT for our completiong routien to be seen.
**	    Add fs_parms var to simplify debugging.
**	14-Nov-1992 (daveb)
**	    take *cl_stat to fill in.
**	20-Nov-1992 (daveb)
**	    Put in more TRdisplay error logging, to fix later.
**	2-Dec-1992 (daveb)
**	    Check for server failed in quick abort case.  Trying to avoid
**	    lots of errors re-connectiong to non-gcm servers.
**	08-sep-93 (swm)
**	    Changed sid type from i4 to CS_SID to match recent CL
**	    interface revision.
**	20-jul-95 (canor01)
**	    pass the SCB to IIGCa_call() instead of the SID
*/
static DB_STATUS
GM_gcmsend( GM_CONN_BLK *conn, STATUS *cl_stat )
{
    DB_STATUS	db_stat;
    CS_SID	sid;
    i4		resume = 0;
    STATUS	local_status;
    GCA_FS_PARMS	*fs_parms = &conn->conn_fs_parms;
    GM_PLACE_BLK	*place = conn->conn_place;

    conn->conn_state = GM_CONN_GCM_WORKING;
    CSget_sid(&sid);
    
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_gcm_sends );

    if( place->place_type == GM_PLACE_SRVR &&
       ((place->place_srvr.srvr_flags & GCA_RG_MIB_MASK) == 0 ||
       place->place_srvr.srvr_state == GM_SRVR_FAILED ))
    {
	/* nothing can be had from this server.  We did a lot of
	   setup work for nothing. FIXME -- move this test higher up? */

	conn->conn_rsb->response.err_element = 0;
	conn->conn_rsb->response.status = *cl_stat = MO_NO_INSTANCE;
	conn->conn_state = GM_CONN_EMPTIED;
	return( E_DB_ERROR );
    }

    do
    {
	*cl_stat = IIGCa_call(GCA_FASTSELECT,
			      (GCA_PARMLIST *)fs_parms,
			      GCA_ASYNC_FLAG | GCA_ALT_EXIT | resume,
			      (PTR)CS_find_scb(sid),
			      (i4) -1,
			      &local_status );
	
	if (*cl_stat != OK)
	{
	    /* "GWM Internal error:  Error making gcm send to %0c" */
	    GM_1error( (GWX_RCB *)0, E_GW8042_GCA_CALL, GM_ERR_INTERNAL,
		      0, (PTR)conn->conn_rsb->request.place );
	    GM_error( *cl_stat );
	    conn->conn_state = GM_CONN_ERROR;
	}
	else
	{
	    /* wait for completion routine to wake us */

	    *cl_stat = CSsuspend(CS_BIO_MASK, 0, 0);
	    if (*cl_stat != OK)
	    {
		/* "GWM Internal error:  CSsuspend problem" */
		GM_error( E_GW8043_CS_SUSPEND );
		GM_error( *cl_stat );
		conn->conn_state = GM_CONN_ERROR;
	    }
	    else		/* completion handler called */
	    {
		switch ( *cl_stat = conn->conn_fs_parms.gca_status )
		{
		case OK:

		    resume = 0;
		    conn->conn_state = GM_CONN_GCM_DONE;
		    break;
		    
		case E_GCFFFE_INCOMPLETE:

		    GM_incr( &GM_globals.gwm_stat_sem,
			    &GM_globals.gwm_gcm_reissues );
		    resume = GCA_RESUME;
		    break;

		case E_GC0032_NO_PEER:

		    resume = 0;

		    /* "GWM Internal error:  Got 'no peer' to %0c (%1c %2x)" */
		    GM_3error( (GWX_RCB *)0, E_GW8044_NO_PEER,
			      GM_ERR_INTERNAL,
			      0, place->place_key,
			      0, place->place_srvr.srvr_class,
			      sizeof(place->place_srvr.srvr_flags),
			      (PTR)&place->place_srvr.srvr_flags );
		    GM_error( *cl_stat );
		    conn->conn_state = GM_CONN_ERROR;
		    break;

		default:

		    resume = 0;

		    /* "GWM Internal error:
		       GCM completion error to %0c (%1c %2x)" */
		    GM_3error( (GWX_RCB *)0, E_GW8045_GCA_COMPLETION,
			      GM_ERR_INTERNAL,
			      0, (PTR)place->place_key,
			      0, (PTR)place->place_srvr.srvr_class,
			      sizeof(place->place_srvr.srvr_flags),
			      (PTR)&place->place_srvr.srvr_flags );
		    GM_error( *cl_stat );
		    conn->conn_state = GM_CONN_ERROR;
		    place->place_srvr.srvr_state = GM_SRVR_FAILED;
		    break;
		}
	    }
	}
    } while( resume != 0 );
    
    db_stat = (*cl_stat == OK) ? E_DB_OK : E_DB_ERROR;

    return( db_stat );
}


/*{
** Name: GM_cgcm	- GCM completion routine for fastselect
**
** Description:
**	This is the GCA completion routine used for finishing GCM
**	messages.  It just wakes the thread up.  It needs to check
**	the results and possibly re-try if needed.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	async_id	interpreted as a sid to resume.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** Side Effects:
**	The CSsuspended thread is resumed.
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	06-oct-93 (swm)
**	    Bug #56447
**	    Cast async_id to CS_SID to eliminate compiler warning.
**	23-aug-95 (wonst02) Fix managed objects (MO) in Windows/NT
**	    The async_id being passed is really an SCB pointer, not a SID,
**	    so get the SID from the SCB and pass it to CSresume.
*/
static void
GM_cgcm( PTR async_id )
{
    CSresume( CSfind_sid( (CS_SCB *)async_id ) );
    return;
}



/*{
** Name: GM_unload_gcm	-- unpack GCM response to an OPBLK.
**
** Description:
**	Takes the response to a GCM query and puts it into an OPBLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn		connection with the response buffer.
**	response	the opblk to fill.
**
** Outputs:
**	response	filled in.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	13-Nov-1992 (daveb)
**	    return error if GCM message had failure status.
**	14-Nov-1992 (daveb)
**	    take *cl_stat to fill in.  conn has ptr to rsb now, not
**	    direct pointer to the response block.
**	18-Nov-1992 (daveb)
**	    deal with read-ahead issues.  Some bug are running out of
**	    response buffer space, which I hadn't considered possible,
**	    but was because I didn't enlarge it to match the GCA size.
**	    I'll adjust it after I fix this here, so that if the GCA
**	    size ever changes the logic will still be correct.
*/
static DB_STATUS
GM_unload_gcm( GM_CONN_BLK *conn, STATUS *cl_stat )
{
    DB_STATUS	db_stat = E_DB_OK;
    GM_OPBLK	*response;
    i4		junk;
    i4		i;
    i4		row_cnt;
    i4		perms;

    *cl_stat = OK;
    conn->conn_state = GM_CONN_UNLOADING;
    response = &conn->conn_rsb->response;
    GM_msg_to_op( conn, &response->op );

    /* unpack the header */
    
    conn->conn_data_last = conn->conn_data_start;
    
    GM_gtLnGcm( conn, &response->status );
    GM_gtNatGcm( conn, &response->err_element );
    *cl_stat = response->status;
    
    GM_gtNatGcm( conn, &junk );	/* skip future[0] */
    GM_gtNatGcm( conn, &junk );	/* skip future[1] */
    GM_gtNatGcm( conn, &junk );	/* skip client perms */
    GM_gtNatGcm( conn, &row_cnt );	/* row_count */
    
    /* FIXME is the GCM element count the same as ours? */
    
    GM_gtNatGcm( conn, &response->elements );
    
    /* now move elements to the OPBLK */
    
    for( i = 0 ; db_stat == E_DB_OK && i < response->elements; i++ )
    {
	do			/* one time only */
	{
	    /* classid */
	    if( (db_stat = GM_gtStrGcm( conn )) != E_DB_OK )
		break;
	    
	    /* instance */
	    if( (db_stat = GM_gtStrGcm( conn )) != E_DB_OK )
		break;
	    
	    switch( response->op )
	    {
	    case GM_GETRESP:
		
		/* put value and perms */
		if( (db_stat = GM_gtStrGcm( conn )) == E_DB_OK )
		{
		    GM_gtNatGcm( conn, &perms );
		    db_stat = GM_pt_to_opblk( response,
					     sizeof(perms),
					     (char *)&perms );
		}
		break;
		
	    case GM_SETRESP:
		
		/* toss value and perms */
		GM_gtLnGcm( conn, &junk );
		conn->conn_data_last += junk;
		GM_gtNatGcm( conn, &perms );		
		break;
		
	    default:
		
		/* E_GW8046_BAD_OP:SS50000_MISC_ING_ERRORS
		   "GWM Internal error:  unexpected OPBLK op %0d" */
		GM_1error( (GWX_RCB *)0, E_GW8046_BAD_OP,
			  GM_ERR_INTERNAL|GM_ERR_LOG,
			  sizeof(response->op), (PTR)&response->op );
		db_stat = E_DB_ERROR;
		break;
	    }
	    
	} while( FALSE );
    }
    response->elements = i;
    conn->conn_state = GM_CONN_EMPTIED;

    db_stat = *cl_stat == OK ? E_DB_OK : E_DB_ERROR;

    return( db_stat );
}


/*{
** Name: GM_op_to_msg	- map OPBLK op to GCA msg.
**
** Description:
**	For each relevant GM_OPBLK op type, return the right
**	type to use for a GCA message.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		op in question
**
** Outputs:
**	none.
**
** Returns:
**	msgtype
**
** Side effects:
**	logs error on bad mapping.
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/

static DB_STATUS
GM_op_to_msg( i4  op, i4  *msg )
{
    DB_STATUS db_stat = E_DB_OK;
    
    switch( op )
    {
    case GM_GET:
	*msg = GCM_GET;
	break;

    case GM_GETNEXT:
	*msg = GCM_GETNEXT;
	break;

    case GM_SET:
	*msg = GCM_SET;
	break;

    default:

	/* E_GW8046_BAD_OP:SS50000_MISC_ING_ERRORS
	   "GWM Internal error:  unexpected OPBLK op %0d" */
	GM_1error( (GWX_RCB *)0, E_GW8046_BAD_OP,
		  GM_ERR_INTERNAL|GM_ERR_LOG, sizeof(op), (PTR)&op );
	db_stat = E_DB_ERROR;
	break;
    }
    return( db_stat );
}



/*{
** Name: GM_msg_to_op	- map GCA message to opblk response op.
**
** Description:
**	Turn the GCM message type into an appropriate OPBLK op.
**	This is complicated because GCM uses GCM_RESPONSE for almost
**	everything, and we need to look at the OPBLK request type to
**	determine the right response type.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn->
**	    conn_fs_parms.gca_message_type	GCM message type.
**	    conn_conn_rsb->conn_request->op		request type.
**
** Outputs:
**	op	filled in
**
** Returns:
**	void
**
** Side effects:
**	logs error on bad mapping.
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/

static void
GM_msg_to_op( GM_CONN_BLK *conn, i4  *op )
{
    switch( conn->conn_fs_parms.gca_message_type )
    {
    case GCM_RESPONSE:
	
	switch( conn->conn_rsb->request.op )
	{
	case GM_GET:
	case GM_GETNEXT:
	    
	    *op = GM_GETRESP;
	    break;
	    
	case GM_SET:
	    
	    *op = GM_SETRESP;
	    break;
	    
	default:
	    
	    *op = GM_BADOP;
	    break;
	}
	break;
	
    default:
	
	*op = GM_BADOP;
	break;
    }
}


/*{
** Name: GM_ptStrGcm	 - put a string to GCM message in GM_CONN_BLK
**
** Description:
**	Put an EOS terminated string into the GCM buffer as a GCA-style
**	length described string.  The length descriptor is a i4.
**	Checks for room in the output buffer before doing anything.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	src		the EOS terminated string source.
**	conn		the block containg the message being set up.
**
** Outputs:
**	conn		is written.
**
** Returns:
**	E_DB_OK		done.
**	E_DB_ERROR	no room in buffer.
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/
static DB_STATUS
GM_ptStrGcm( char *src, GM_CONN_BLK *conn )
{
    DB_STATUS db_stat = E_DB_OK;
    i4 len;

    len = STlength( src );

    if((conn->conn_data_length - (conn->conn_data_last -
				  conn->conn_data_start)) < len + sizeof(len))
    {
	db_stat = E_DB_ERROR;
    }
    else
    {
	(void) GM_ptLnGcm( len, conn );
	STcopy( src, conn->conn_data_last );
	conn->conn_data_last += len;
    }
    return( db_stat );
}



/*{
** Name: GM_ptNatGcm	- put integer to GCA message in a GM_CONN_BLK.
**
** Description:
**	
**	put integer to a GCA message in a GM_CONN_BLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	val		the value to put.
**	conn		containing the GCA message being set up.
**
** Outputs:
**	conn		written.
**
** Returns:
**	E_DB_OK		done.
**	E_DB_ERROR	no room in buffer.
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	24-jan-1994 (gmanning)
**	    Change unsigned i4  to u_i4 in order to compile on NT.
*/
static DB_STATUS
GM_ptNatGcm( i4  val, GM_CONN_BLK *conn )
{
    u_i4 uval = (u_i4) val;
    DB_STATUS db_stat = E_DB_OK;
    if( (conn->conn_data_length - (conn->conn_data_last -
				   conn->conn_data_start)) < sizeof(val) )
    {
	db_stat = E_DB_ERROR;
    }
    else
    {
	MEcopy( (PTR)&uval, sizeof(uval), (PTR)conn->conn_data_last );
	conn->conn_data_last += sizeof(uval);
    }
    return( db_stat );
}


/*{
** Name: GM_ptLnGcm	- put i4 to GCA message in a GM_CONN_BLK.
**
** Description:
**	
**	Put i4 to GCA message in a GM_CONN_BLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	val		the value to put.
**	conn		containing the GCA message being set up.
**
** Outputs:
**	conn		written.
**
** Returns:
**	E_DB_OK		done.
**	E_DB_ERROR	no room in buffer.
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/
static DB_STATUS
GM_ptLnGcm( i4 val, GM_CONN_BLK *conn )
{
    DB_STATUS db_stat = E_DB_OK;
    if( (conn->conn_data_length - (conn->conn_data_last -
				   conn->conn_data_start)) < sizeof(val) )
    {
	db_stat = E_DB_ERROR;
    }
    else
    {
	MEcopy( (PTR)&val, sizeof(val), (PTR)conn->conn_data_last );
	conn->conn_data_last += sizeof(val);
    }
    return( db_stat );
}


/*{
** Name: GM_gtStrGcm	- move string from CONN_BLK to OPBLK
**
** Description:
**	Copy a string out of the GCA message in the CONN_BLK and 
**	put it in the OPBLK.  Check for out of room in the OPBLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn		block with a message being unloaded.
**	opblk		response block being loaded.
**
** Outputs:
**	conn
**	    .conn_data_last	bumped to next place
**	opblk			updated.
**
** Returns:
**	E_DB_OK		done.
**	E_DB_ERROR	no room in buffer.
**
** History:
**	8-oct-92 (daveb)
**	    created.
**	12-Nov-1992 (daveb)
**	    remember to bump after getting length.
*/
static DB_STATUS
GM_gtStrGcm( GM_CONN_BLK *conn )
{
    GM_OPBLK *response = &conn->conn_rsb->response;
    DB_STATUS	db_stat;
    i4	len;

    MEcopy( (PTR)conn->conn_data_last, sizeof(len), (PTR)&len );
    conn->conn_data_last += sizeof(len);

    db_stat = GM_pt_to_opblk( response, len + 1, conn->conn_data_last );
    if( db_stat == E_DB_OK )
    {
	response->buf[ response->used - 1 ] = EOS;
	conn->conn_data_last += len;
    }
    return( db_stat );
}



/*{
** Name: GM_gtNatGcm	- get a number out of the GCA msg in the CONN_BLK.
**
** Description:
**	Extract the integer that's at the current location in the CONN_BLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn		containing GCA message being unloaded.
**
** Outputs:
**	conn
**	    conn_data_last	updated
**	val			filled with value.
**
** Returns:
**	none.
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/
static void
GM_gtNatGcm( GM_CONN_BLK *conn, i4  *val )
{
    MEcopy( (PTR)conn->conn_data_last, sizeof(*val), (PTR)val );
    conn->conn_data_last += sizeof(*val);
}



/*{
** Name: GM_gtLnGcm	- get a i4 out of the GCA msg in the CONN_BLK.
**
** Description:
**	Extract the integer that's at the current location in the CONN_BLK.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	conn		containing GCA message being unloaded.
**
** Outputs:
**	conn
**	    conn_data_last	updated
**	val			filled with value.
**
** Returns:
**	none.
**
** History:
**	8-oct-92 (daveb)
**	    created.
*/
static void
GM_gtLnGcm( GM_CONN_BLK *conn, i4 *val )
{
    MEcopy( (PTR)conn->conn_data_last, sizeof(*val), (PTR)val );
    conn->conn_data_last += sizeof(*val);
}

