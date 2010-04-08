/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <cv.h>
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
# include <gcn.h>
# include <gcm.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"
# include "gwmplace.h"

/**
** Name:	gwmgcn.c  - name server query stuff.
**
** Description:
**
**	This file contains routines for querying name servers.
**	this is a bootstrap step
**
**	This file defines:
**
**
** History:
**	9-oct-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    target is /iinmsvr, not /@iigcn.  Handle empty vnode. 
**	13-Nov-1992 (daveb)
**	    get classids from <gcn.h>.  Use GCN_NMSVR too.
**	    Make GM_gcn_unpack work.   Documented. 
**	    Simplify by doing all the packing more or less inline.
**	20-Nov-1992 (daveb)
**	    GXrequest now GM_request, taking GM_RSB instead of
**	    GX_RSB.  Don't need "gwmxtab.h" anymore.  Add TRdisplay
**	    logging to clean up later.
**	2-Dec-1992 (daveb)
**	    Set the guises in the fake local RSB; otherwise the GCN
**	    gets a random value, which causes all sorts of surprises.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	 2-Oct-98 (gordy)
**	    Moved GCN MIB class definitons to gcm.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**/



/*}
** Name:	GM_GCN_CLASS	- description of column in GCN server table.
**
** Description:
**	This describes what to ask for for one element in the GCN server
**	table.  
**
** History:
**	13-Nov-1992 (daveb)
**	    documented.
*/

typedef struct
{
    char	*classid;
    char	len;
    i4		type;
#	define	WANT_NAT	1
#	define	WANT_STR	2
    i4		reg_offset;

} GM_GCN_CLASS;

/*
** Table describing the "row" we want from the GCN on each query.
*/

static GM_GCN_CLASS gcn_classes[] =
{
    { GCN_MIB_SERVER_ENTRY,	sizeof(GCN_MIB_SERVER_ENTRY) -1,
      WANT_NAT, CL_OFFSETOF( GM_REGISTER_BLK, reg_entry ) },
    { GCN_MIB_SERVER_ADDRESS,	sizeof(GCN_MIB_SERVER_ADDRESS) -1,
      WANT_STR, CL_OFFSETOF( GM_REGISTER_BLK, reg_addr ) },
    { GCN_MIB_SERVER_CLASS,	sizeof(GCN_MIB_SERVER_CLASS) -1,
      WANT_STR, CL_OFFSETOF( GM_REGISTER_BLK, reg_class ) },
    { GCN_MIB_SERVER_FLAGS,	sizeof(GCN_MIB_SERVER_FLAGS) -1,
      WANT_NAT, CL_OFFSETOF( GM_REGISTER_BLK, reg_flags ) },
    { GCN_MIB_SERVER_OBJECT,	sizeof(GCN_MIB_SERVER_OBJECT) -1,
      WANT_STR, CL_OFFSETOF( GM_REGISTER_BLK, reg_object ) },
    { 0 }
};


/* forwards */

static STATUS GM_gcn_unpack( GM_RSB *gm_rsb, SPTREE *t );


/*{
** Name:	GM_ask_gcn	- ask a gcn for server info
**
** Description:
**	Returns a tree containing all the information about the
**	servers on a specified vnode, by name.
**
**	How to do it - use the XTAB stuff.
**
**	(1) initialize the output tree.
**	(2) allocate a GX_RSB.
**	(3) setup the key for a cross tab;
**	(4) loop over a cross-tab getnext from the node
**	(5) for each returned opblk, allocate a node and link in.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	t		uninitialized tree to use
**	vnode		the one to query.
**
** Outputs:
**	t		initialized, loaded with allocated nodes.
**
** Returns:
**	none.
**
** Side Effects:
**	allocates memory that must be freed for nodes in the tree.
**
** History:
**	9-oct-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    target is /iinmsvr, not /@iigcn.  Handle empty vnode. 
**	13-Nov-1992 (daveb)
**	    use GCN_NMSVR for class name.
**	    Collapse all the setup here; it's tiny.
**	18-Nov-1992 (daveb)
**	    Clean up end-of-scan so it doesn't return error.
**	2-Dec-1992 (daveb)
**	    Set the guises in the fake local RSB; otherwise the GCN
**	    gets a random value, which causes all sorts of surprises.
*/
DB_STATUS
GM_ask_gcn( SPTREE *t, char *vnode )
{
    DB_STATUS		db_stat = E_DB_OK;
    STATUS		cl_stat = OK;
    GM_RSB		gm_rsb;
    GM_GKEY		*key;
    GM_GCN_CLASS	*cp;
    char		target[ GM_MIB_PLACE_LEN ];

    /*  Setup fake local RSB with all read permissions set */

    gm_rsb.guises = MO_READ;

    STprintf( target, "%s::/%s", vnode, GCN_NMSVR );
    
    key = &gm_rsb.cur_key;
    GM_clearkey( key );
    
    STcopy( target, key->place.str );
    key->place.len = STlength( target );

    SPinit( t, GM_nat_compare );
    GM_i_opblk( GM_GETNEXT, target, &gm_rsb.request );

    while( db_stat == E_DB_OK )
    {
	/* setup query to get the next row */

	gm_rsb.request.used = 0;
	gm_rsb.response.used = 0;
	for( cp = gcn_classes; cp->classid != NULL ; cp++ )
	{
	    GM_trim_pt_to_opblk( &gm_rsb.request,
				cp->len, cp->classid );
	    GM_trim_pt_to_opblk( &gm_rsb.request,
				key->instance.len, key->instance.str );
	    gm_rsb.request.elements++;
	}

	/* get the next row. */

	if( (db_stat = GM_request( &gm_rsb, key, &cl_stat )) != OK )
	{
	    if( cl_stat != MO_NO_NEXT )
	    {
		/* "GWM Internal error:  Error querying name server %0c" */
		GM_1error( (GWX_RCB *)0, E_GW8081_GCN_ERROR,
			      GM_ERR_INTERNAL, 0, (PTR)target );
		GM_error( cl_stat );
	    }
	    break;
	}

	/* unpack the row and insert it in the tree */
	
	db_stat = GM_gcn_unpack( &gm_rsb, t );
    }

    db_stat = (cl_stat == OK) ? E_DB_OK : E_DB_ERROR;

    return( db_stat );
}


/*{
** Name:	GM_gcn_unpack	- unpack opblk to server tree.
**
** Description:
**	Given an OPBLK response to a GCN query, unpack entries into
**	GM_REGISTER_BLKs and insert them into the input tree.  Partial
**	row entries in the opblk are ignored, and cur_key is set to
**	the instance of the last completed row.  Reset response->used 
**	for reuse.
** Re-entrancy:
**	yes.
**
** Inputs:
**	gx_rsb		the rsb holding the state of the current
**			GCN scan.
**
**	    .gwm_rsb.cur_key
**			the previous key,
**	    .gwm_rsb.response.elements
**			number of MOobjects in the opblk.
**	    .gwm_rsb.response.buf
**			the packed MO objects.
**
**	t		the tree in which to insert register blocks.
**
** Outputs:
**	gx_rsb->gm_rsb->cur_key
**
** Returns:
**	<function return values>
**
** Exceptions: (if any)
**	<exception codes>
**
** Side Effects:
**	Allocates memory for the register blocks.
**
** History:
**	13-Nov-1992 (daveb)
**	    Documented, made work.  Rename local GM_gtelement to
**	    GM_scanresp for general use, moving it to gwmutil.c.
**	14-Apr-2008 (wanfr01)
**	    Bug 120262
**	    Store the instance string as fetched from the gcn 
**	    response block, rather than reconverting the key.
**	    MOlongout creates a different string if the gcn 
**	    is 32 bit and the server is 64 bit.
*/
static DB_STATUS
GM_gcn_unpack( GM_RSB *gm_rsb, SPTREE *t )
{
    DB_STATUS		db_stat = E_DB_OK;
    i4			i;
    i4			elements;
    GM_GCN_CLASS	*cp;
    GM_REGISTER_BLK	*reg;
     char		*oindex = "";

    char		*src;
    char		*classid;
    char		*instance;
    char		*value;
    i4			perms;
    
    src		= gm_rsb->response.buf;
    elements	= gm_rsb->response.elements;

    for( i = 0; i < elements && db_stat == E_DB_OK ; )
    {
	reg = (GM_REGISTER_BLK *)GM_alloc( sizeof( *reg ) );
	if( reg == NULL )
	{
	    db_stat = E_DB_ERROR;
	    break;
	}
	reg->reg_blk.key = (PTR)&reg->reg_entry;

	for( cp = gcn_classes; i < elements && cp->classid; cp++, i++ )
	{
	    GM_scanresp( &src, &classid, &instance, &value, &perms );
	    if( STcompare( classid, cp->classid ) )
	    {
		/* Means we're out of the classid -- NO_NEXT  */
		db_stat = E_DB_ERROR;
		break;
	    }
	    else if( cp == gcn_classes )
	    {
		oindex = instance;
	    }
	    else if ( STcompare( oindex, instance ) )
	    {
		/* "GWM Internal error:  Mismatched instances %0c %1c" */
		GM_2error( (GWX_RCB *)0, E_GW8082_GCN_UNPACK,
			  GM_ERR_INTERNAL, 0, (PTR)oindex, 0, (PTR)instance );
		db_stat = E_DB_ERROR;
		break;
	    }

	    if( cp->type == WANT_NAT )
		CVal( value, (i4 *)((char *)reg + cp->reg_offset) );
	    else
		STcopy( value, (char *)reg + cp->reg_offset );
	}

	if( cp->classid == NULL )
	{
	    /* if complete entry, insert it and mark key */

	    SPinstall( &reg->reg_blk, t );
	    STcopy ( instance, gm_rsb->cur_key.instance.str );
	    gm_rsb->cur_key.instance.len =
		STlength(gm_rsb->cur_key.instance.str);
	}
	else
	{
	    /* incomplete; toss it */

	    GM_free( (PTR)reg );
	    break;
	}
    }
    return( db_stat );
}

