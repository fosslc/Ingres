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
** Name: gwmxtab.c	- IMA gateway subs xtabs
**
** Description:
**	This file contains routines that implement a
**	general gateway to MO GL layer using the GWF non-SQL Gateway
**	mechanism.  It supports arbitrary tables constructed out
**	otjects having the same indeces.  The main restrinction is
**	that these tables are read-only.  Updates must be done
**	through the main IIMIB_OBJECTS table.
**
**	Unimplemented features:
**
**		security enforcement
**		deletes
**		puts
**      	updates
**		transactions
**
**	This file defines:
**
**	GX_ad_req_col		-- add a column to a request block
**	GX_i_out_row		-- setup tuple output buffer
**	GX_ad_to_tuple		-- add response to tuple buffer
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
**	    nuke GX_gt_partial_row, move request stuff to gwmrow.c
**	    get rid of getcol and recvcol.  No value, but a lot
**	    of confusion.
**	24-Nov-1992 (daveb)
**	    Add missing 'ignore case' arg to STbcompare.
**	13-Jan-1993 (daveb)
**	    In GX_ad_to_tuple, 
**	    break on inconsistant row, don't try to put it anyway!
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      10-May-1994 (daveb) 63344
**          Handle case where there's nothing in the opblock, but we
**  	    do have SERVER and/or VNODE to fill in.
**	31-may-95 (reijo01) Bug 68867
**		Add code to handle case where a query for data from a remote vnode
**      caused rows to skipped. The problem was caused by either large rows
**      or many rows of data being obtained from a remote vnode via gcm.
**		
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
[@history_template@]...
**/


/* ---------------------------------------------------------------- 
**
**  Request block packing/unpacking.
*/

/*{
** Name:	GX_ad_req_col	-- add a column to a request block
**
** Description:
**
**	Given a pointer to the RSB, fill in the request opblk with the data
**	necesary to get column n of the table.  Return E_DB_OK if it fit,
**	not-OK if we are out of room in the request block.
**	(No room is common for large rows).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	xt_rsb		the rsb to handle
**
**	    .gwm_gwm_atts
**			attributes in the row, in attribute order,
**			with what we need to know here.
**	
**	    .request	the request data;
**
**	n		the column to put into the request buffer, starting
**			at column 0.
**
** Outputs:
**
**	xt_rsb
**	     .request	filled in with new request data (classid, instance).
**
** Returns:
**	OK		if did it.
**	E_DB_ERROR	if out of room in the request.
**
** History:
**	20-jul-92 (daveb)
**	    Special case:  skip any GM_XTAB_PLACE column completely. 
**	22-sep-92 (daveb)
**	    documentd.
**	29-sep-92 (daveb)
**	    Simplify a lot now that we have gwm_gwm_atts holding the
**	    information we need here in the right format.  Just plug
**	    the data in and go.
*/

DB_STATUS
GX_ad_req_col( GX_RSB *xt_rsb, i4  n )
{
    GM_RSB		*gwm_rsb = &xt_rsb->gwm_rsb;
    GM_OPBLK		*request = &xt_rsb->gwm_rsb.request;
    GM_ATT_ENTRY	*gmatt;

    DB_STATUS		db_stat = E_DB_OK;

    i4  used;
    i4  alen;

    char		*instance;
    i4			instlen;

    char		obclass[ GM_MIB_CLASSID_LEN + 1 ];

    /* append request entry for this classid column if there is room.
       If no room, report it. */

    gmatt = &gwm_rsb->gwm_gwm_atts[ n ];
    if( gmatt->gma_type == GMA_USER )
    {
	/* normalize the user classid into obclass */

	STncpy( obclass, gmatt->gma_col_classid, GM_MIB_CLASSID_LEN );
	obclass[ GM_MIB_CLASSID_LEN ] = EOS;
	STtrmwhite( obclass );
	alen = STlength( obclass );

	/* pick the instance to use from cur_key.  We don't take the
	   instance from the local key (which we aren't being handed)
	   so we get the same instance for the whole row across multiple
	   GX_gt_partial_rows  */
	
	instance = xt_rsb->gwm_rsb.cur_key.instance.str;
	instlen = xt_rsb->gwm_rsb.cur_key.instance.len;
	
	/* copy both to the request, updating used only
	   if both classid and instance fit. */

	used = request->used;	/* save current */
	if( (db_stat = GM_trim_pt_to_opblk( request, alen, obclass ))
	   != E_DB_OK ||
	   (db_stat = GM_trim_pt_to_opblk( request, instlen, instance ))
	   != E_DB_OK )
	{
	    db_stat = E_DB_ERROR;
	    request->used = used; /* restore to current */
	}
	else
	{
	    request->elements++;
	}
    }
    return( db_stat );
}


/* ---------------------------------------------------------------- 
**
** row related
*/

/*{
** Name:	GX_i_out_row	-- setup tuple output buffer
**
** Description:
**	 Setup up the row in the rcb for later formatting.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	gwx_rcb->xrcb_rsb->xrcb_var_data1	the output tuple buffer
**
** Outputs:
**	gwx_rsb->xrcb_rsb->gwrsb_internal_rsb	set up to point to tuple.
**
** Returns:
**	none.
**
** History:
**	20-Jul-92 (daveb) 
**	    Tricky part is flagging where a PLACE value needs to be
**	    made visible.
**	3-Feb-1993 (daveb)
**	    documented better.  Tricks are gone, at least from here.
*/
VOID
GX_i_out_row( GWX_RCB *gwx_rcb )
{
    GW_RSB		*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GX_RSB		*xt_rsb = (GX_RSB *)rsb->gwrsb_internal_rsb;

    xt_rsb->tuple = gwx_rcb->xrcb_var_data1.data_address;
    xt_rsb->tup_len = gwx_rcb->xrcb_var_data1.data_in_size;
    xt_rsb->tup_used = 0;
    xt_rsb->tup_cols = 0;
}


/*{
** Name:   GX_ad_to_tuple	-- add response to tuple buffer
**  
** Description:
**
**	Add all the data from the response block to the tuple.
**
** Inputs:
**	rsb->gwrsb_internal_rsb
**		.tuple		the tuple buffer
**		.tup_cols	columns in the tuple
**		.tup_used	bytes used in the tuple
**		.gwm_rsb.ncols	colums in the output tuple
**		.gwm_att_list	column descriptions
**		.response.used	space used in the buffer
**		.response.buf	responses to the query.
**
** Outputs:
**	rsb->gwrsb_internal_rsb.
**		.tuple		data buffer written
**		.tup_cols	columns written in the tuple
**		.tup_used	bytes used in the tuple
**		.tup_perms	perms for the tuple.
**
** Returns:
**	OK	if succeeded
**	other	error status.
**
** History:
**	20-jul-92 (daveb)
**	    Add special case handling of place.
**	29-sep-92 (daveb)
**	    Generalize place handling by using new gwa_att_type.
**	    Now there are three types of place, and you can have
**	    multiple places in an output tuple.
**	13-Nov-1992 (daveb)
**	    simply by using new GM_scanresp to scan GETRESP block
**	17-Nov-1992 (daveb)
**	    improve error detection by checking each returned object
**	    for right classid and instance.
**	19-Nov-1992 (daveb)
**	    get rid of getcol and recvcol.  No value, but a lot
**	    of confusion.
**	20-Nov-1992 (daveb)
**	    Set err_element on elements, not columns, which are off.
**	24-Nov-1992 (daveb)
**	    Add missing 'ignore case' arg to STbcompare.
**	7-Dec-1992 (daveb)
**	    Add better error logging, and do good checking of our
**	    att type, using new GMA_USER type for us.
**	13-Jan-1993 (daveb)
**	    Break on inconsistant row, don't try to put it anyway!
**      10-May-1994 (daveb) 63344
**          Handle case where there's nothing in the opblock, but we
**  	    do have SERVER and/or VNODE to fill in.
**	31-may-95 (reijo01) Bug 68867
**		Add code to handle case where a query for data from a remote vnode
**      caused rows to skipped. The problem was caused by either large rows
**      or many rows of data being obtained from a remote vnode via gcm.
*/
DB_STATUS
GX_ad_to_tuple( GX_RSB *xt_rsb, STATUS *cl_stat )
{
    DB_STATUS	db_stat = E_DB_OK;
    GM_OPBLK	*response = &xt_rsb->gwm_rsb.response;
    GM_ATT_ENTRY *gwm_att;

    char	*q;
    char	*p = response->cursor;
	/*
	**	Set lastp to point to last byte of the tuple buffer.
	**
	*/
    char	*lastp = &response->buf[ response->used ];
    i4		element = response->curs_element;

    char	*first_instance = NULL;
    char	*src;
    i4		srclen;
    i4		classlen;
    i4		col;
    
    char	*classid;
    char	*instance;
    char	*value;
    i4		perms;
    bool    	go_on;

    do				/* one-time break loop */
    {
	/* for each column that might be in the response,
	   while there is stuff in the the response, put the
	   data into the tuple */

	go_on = TRUE;
	for( col = xt_rsb->tup_cols;
	    db_stat == E_DB_OK && col < xt_rsb->gwm_rsb.ncols && go_on;
	    col++ )
	{
	    /* locate the column description */

	    gwm_att = &xt_rsb->gwm_rsb.gwm_gwm_atts[ col ];

	    /* if it's a classid, pull out of the response block */

	    switch( gwm_att->gma_type )
	    {
	    case GMA_USER:

		if( element >= response->elements && p >= lastp )
		{
			/*
			** 	Decrement the col count because for loop will increment it
			**  even though the column was not processed. This way after 
			**  another gcm response buffer is filled we will start filling
			**  the tuple buffer at the right column.
			*/
		    go_on = FALSE;
			col--;
		}
		else
		{
		    GM_scanresp( &p, &classid, &instance, &value, &perms );
		    src = value;
		    srclen = STlength( src );
		    if( first_instance == NULL )
			first_instance = instance;

		    /* check each att for match against classid, and make
		       sure it's the same instance as the current key.  If
		       not, set cl_stat to MO_NO_NEXT, etc. */

		    classlen = STlength( classid );
		    if( STncmp( classid, gwm_att->gma_col_classid, classlen ) ||
		       STcompare( instance, first_instance ) )
		    {

			/* E_GW8381_INCONSISTENT_ROW:SS50000_MISC_ING_ERRORS
			   "A cross-tab row being constructed had objects with 
			   inconsistent instance values.  This can either be 
			   caused by a dirty read (objects added or deleted 
			   during the query) or by registering objects as 
			   columns with non-matching instance index objects."*/

			/* FIXME: invalidate whole place with MO_NO_NEXT,
			   or should we try this place some more? */

			*cl_stat = E_GW8381_INCONSISTENT_ROW;
			response->status = *cl_stat;
			response->err_element = element;

			db_stat = E_DB_ERROR;
			break;
		    }
		    element++;
		}
		break;

	    case GMA_SERVER:
	    case GMA_VNODE:

		src = xt_rsb->gwm_rsb.cur_key.place.str;
		srclen = xt_rsb->gwm_rsb.cur_key.place.len;

		/* for vnode, stop at the ':' */

		if( gwm_att->gma_type == GMA_VNODE )
		{
		    for( q = src;
			*q && q < &p[GM_MIB_PLACE_LEN] && *q != ':' ; )
			q++;

		    srclen = q - src;
		}
		break;

	    default:

		/* "GWM:  Internal error:  bad attr type %0d for column
		   %1d (%2c)" */
		
		GM_3error( (GWX_RCB *)0, E_GW8186_BAD_ATTR_TYPE,
			  GM_ERR_INTERNAL,
			  sizeof( gwm_att->gma_type ),
			  (PTR)&gwm_att->gma_type,
			  sizeof( col ), (PTR)&col,
			  GM_dbslen( gwm_att->gma_dmt_att->att_name.db_att_name ),
			  (PTR)&gwm_att->gma_dmt_att->att_name );

		db_stat = E_DB_ERROR;
		break;
	    }

	    /* now put what's at src for srclen into the tuple */

	    if( go_on && db_stat == E_DB_OK )
		db_stat = GM_ad_out_col( src,
					srclen,
					col,
					&xt_rsb->gwm_rsb,
					xt_rsb->tup_len,
					xt_rsb->tuple,
					&xt_rsb->tup_used );
	}

	xt_rsb->tup_cols = col;

    } while( FALSE );

    return( db_stat );
}

