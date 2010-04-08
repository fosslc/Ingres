/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cm.h>
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
# include <dmucb.h>
# include <ulf.h>
# include <ulm.h>
# include <gca.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"

/**
** Name: gwfmob.c	- IMA gateway subs for the IIMIB_OBJECTS table.
**
** Description:
**	This file contains routines that might implement a
**	gateway to MO GL layer using the GWF non-SQL Gateway
**	mechanism.
**
** Defines the following functions:
**	FIXME
**
** History:
**	19-feb-91 (daveb/bryanp)
**	    Modified from gwftest/gwflgtst.c for proof of concept.
**	6-Nov-91 (daveb)
**	    Modified from gwf_mi_works.c to gwf_mo.c now that MI
**	    has mutated.
**	16-Jan-92 (daveb)
**	    More modified to reflect objects/xtab split.
**	29-Jan-92 (daveb)
**	    modified for MO.10.
**	18-Jun-92 (daveb)
**	    Need to format rel and attr tuples for this too.
**	30-jun-92 (daveb)
**	    request/response GM_OPBLK's are in the RSB, not allocated.
**	    Define GMMOB_TUP_COLS constant.
**	1-Jul-92 (daveb)
**	    Decipher and document args to GOopen, save the attributes
**	    for later use.  Clear out keys completely. 
**	29-sep-92 (daveb)
**	    Call GM_open_sub for open, and hand new srclen to GM_ad_out_col.
**	29-sep-92 (daveb)
**	    Fill in GM_ATT_ENTRYs in the open.
**	30-sep-92 (daveb)
**	    Major revisions to MOget to do place iteration and position-key
**	    validation here.  Add bad position MIB objects to see how we
**	    are doing with key selectivity.  Rename GMposition GM_position,
**	    GMop to GM_op.
**	11-Nov-1992 (daveb)
**	    Make GO_replace work.  GM_op takes rsb, not opblks and key now.
**	19-Nov-1992 (daveb)
**	    include <gca.h> now that we're using GCA_FS_MAX_DATA for the
**	    OPBLK buffer size.
**	24-Nov-1992 (daveb)
**	    Store sub-gateway type (GWM_MOB_TABLE) in rel entry, so
**	    open doesn't need to figure it out.
**	25-Nov-1992 (daveb)
**	    Generalize tabf:  allow different column names, and use the
**	    now mandatory 'is' clause to say what it is.  Still need
**	    to make sure it's sane afterwards.
**	2-Feb-1993 (daveb)
**	    improve comments.  fix typos ("is" should be "inits").
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**	31-Oct-1995 (reijo01)
**	    When one row is requested and the status comes back MO_NO_NEXT,
**	    don't treat it as an error. Just format the tuple buffer and 
**	    continue as normal.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

/*}
** Name:	GO_RSB	- Flat object table RSB.
**
** Description:
**	GW RSB for flat tables; nothing special here, it's GM standard.
**
** History:
**	22-sep-92 (daveb)
**	    documented.
*/
typedef struct
{
    GM_RSB	gwm_rsb;
} GO_RSB;


/* forward refs */

static DB_STATUS GOterm( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOtabf( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOopen( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOclose( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOposition( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOget( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOput( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOreplace( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOdelete( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GObegin( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOcommit( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOabort( /* GWX_RCB *gwx_rcb */ );
static DB_STATUS GOidxf( /* GWX_RCB *gwx_rcb */ );

static DB_STATUS GO_fmt_tuple( GO_RSB *go_rsb, GM_GKEY *key,
			      char *cbuf, i4  perms,
			      i4  tup_len, char *tup );

/* Call counters */

static i4 GO_terms ZERO_FILL;
static i4 GO_tabfs ZERO_FILL;
static i4 GO_idxfs ZERO_FILL;
static i4 GO_opens ZERO_FILL;
static i4 GO_closes ZERO_FILL;
static i4 GO_positions ZERO_FILL;
static i4 GO_gets ZERO_FILL;
static i4 GO_puts ZERO_FILL;
static i4 GO_replaces ZERO_FILL;
static i4 GO_deletes ZERO_FILL;
static i4 GO_begins ZERO_FILL;
static i4 GO_aborts ZERO_FILL;
static i4 GO_commits ZERO_FILL;
static i4 GO_inits ZERO_FILL;
static i4 GO_bad_ipos ZERO_FILL;
static i4 GO_bad_cpos ZERO_FILL;

/*
** Registration table, loaded into MO at gateway initialization.
** In turn they are made visible through SQL by the gateway.
*/
static MO_CLASS_DEF GO_classes[] =
{
    { 0, "exp.gwf.gwm.num.mob_inits", sizeof(GO_inits), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_inits, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_terms", sizeof(GO_terms), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_terms, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_tabfs", sizeof(GO_tabfs), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_tabfs, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_idxfs", sizeof(GO_idxfs), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_idxfs, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_opens", sizeof(GO_opens), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_opens, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_closes", sizeof(GO_closes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_closes, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_positions", sizeof(GO_positions), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_positions, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_gets", sizeof(GO_gets), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_gets, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_puts", sizeof(GO_puts), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_puts, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_replaces", sizeof(GO_replaces), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_replaces, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_deletes", sizeof(GO_deletes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_deletes, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_begins", sizeof(GO_begins), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_begins, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_aborts", sizeof(GO_aborts), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_aborts, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_commits", sizeof(GO_commits), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_commits, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_bad_ipositions", sizeof(GO_bad_ipos), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_bad_ipos, MOcdata_index },
    { 0, "exp.gwf.gwm.num.mob_bad_cpositions", sizeof(GO_bad_cpos), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GO_bad_cpos, MOcdata_index },
    { 0 }
};




/*
** Name: GOterm	- terminate gateway
**
** Description:
**	This exit performs exit shutdown, which may include shutdown
**	of the foreign data manager.
**
** Inputs:
**	gwx_rcb		- Standard control block
**
** Outputs:
**	gwx_rcb->xrcb_error.err_code	    - set to E_DB_OK
**
** Returns:
**	E_DB_OK
**
** History:
**	19-feb-91 (daveb)
**	    Created.
*/
static DB_STATUS
GOterm( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_terms );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

/*
** Name:  GOtabf
**
** Format appropriate iigwXX_relation and iigwXX_attribute entries for
** this table registration.  Stores the sub-gateway flag GM_MOB_TABLE
** in the relation flags, as well as other structure information.
**
** Performs what error checking on sensible structure/key combinations that
** can be done here, which isn't much.  At the time the table is registered,
** it doesn't have the final structure, but is a heap.  It's only at real
** table open time that we can validate the registration.  This should be
** improved in a future release.
**
**  Inputs:
**	 gwx_rcb->xrcb_tab_name
**			The name of the table being registered.
**
**	 gwx_rcb->xrcb_tab_id
**			INGRES table id.
**
**	 gwx_rcb->xrcb_gw_id
**			Gateway id, derived from DBMS type.
**			clause of the register statement.
**
**	 gwx_rcb->xrcb_var_data1
**			source of the gateway table, in the 'from'
**			clause of the 'register' statement.  Ignored here.
**
**	 gwx_rcb->xrcb_var_data2
**			path of the gateway table.
**			How does this differ from the source? FIXME
**			Ignored here anyway.
**
**	 gwx_rcb->xrcb_column_cnt
**			column count of the table, number of elements
**			in column_attr and var_data_list.
**
**	 gwx_rcb->column_attr
**			INGRES column information (array of DMT_ATT_ENTRYs).
**
**	 gwx_rcb->xrcb_var_data_list
**			Array of extended column attribute strings,
**			from 'is' clause of the target list of the
**			register statement.  A null indicates no
**			extended attributes.  
**
**	 gwx_rcb->xrcb_var_data3
**			tuple buffer for the iigwX_relation
**
**	 gwx_rcb->xrcb_mtuple_buffs
**			An allocated array of iigwX_attribute tuple buffers.
**
**  Outputs:
**	 gwx_rcb->xrcb_var_data3
**			iigwX_relation tuple.
**
**	 gwx_rcb->xrcb_mtuple_buffs
**			iigwX_attribute tuples.
**
**	 gwx_rcb->xrcb_error.err_code
**			E_GW0000_OK or E_GW0001_NO_MEM.
**
**  Returns:
**	 E_DB_OK
**	 E_DB_ERROR.
**
**  History:
**	24-Nov-1992 (daveb)
**	    Store sub-gateway type (GWM_MOB_TABLE) in rel entry, so
**	    open doesn't need to figure it out.
**	2-Feb-1993 (daveb)
**	    improve comments.
*/
static DB_STATUS
GOtabf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    i4			numatts;
    char		*src;
    GM_XATT_TUPLE	*xatp;
    DMT_ATT_ENTRY	*dmt_att;
    DMU_GWATTR_ENTRY	**gwatt;
    DM_PTR		*ptr;
    DB_STATUS		db_stat = E_DB_OK;

    GM_incr( &GM_globals.gwm_stat_sem, &GO_tabfs );
    do
    {
	/* checkup and do relation entry */

	if( (db_stat = GM_tabf_sub( gwx_rcb, GM_MOB_TABLE )) != E_DB_OK )
	    break;

	/* scan through and format attribute entries */

	numatts = gwx_rcb->xrcb_column_cnt;
	ptr = &gwx_rcb->xrcb_mtuple_buffs;
	gwatt = (DMU_GWATTR_ENTRY **)gwx_rcb->xrcb_var_data_list.ptr_address;
	dmt_att = gwx_rcb->xrcb_column_attr;
	xatp = (GM_XATT_TUPLE *)ptr->ptr_address;

	for( ; numatts-- ; xatp++, dmt_att++, gwatt++ )
	{
	    xatp->xatt_tblid = *gwx_rcb->xrcb_tab_id;
	    xatp->xatt_attnum = dmt_att->att_number;

	    if( !((*gwatt)->gwat_flags_mask & DMGW_F_EXTFMT) )
	    {
		/* E_GW8181_OBJ_MISSING_IS_CLAUSE:SS50000_MISC_ING_ERRORS
		   "IMA:  Column %0c is missing the required IS clause.
		   Legal values are 'SERVER', 'VNODE', 'CLASSID', 'INSTANCE',
		   'VALUE', or 'PERMISSIONS'" */

		GM_1error( (GWX_RCB *)0, E_GW8181_OBJ_MISSING_IS_CLAUSE,
			  GM_ERR_USER,
			  GM_dbslen( dmt_att->att_name.db_att_name ),
			  (PTR)&dmt_att->att_name );
		db_stat = E_DB_ERROR;
		break;
	    }
	    else		/* Check 'IS' clause validity */
	    {
		src = (*gwatt)->gwat_xbuffer;
		if( (db_stat = GM_check_att( src )) != E_DB_OK )
		{
		    /* E_GW8182_OBJ_BAD_IS_CLAUSE:SS50000_MISC_ING_ERRORS
		       "IMA:  Column %0c has an invalid IS clause %1c.  Legal
		       values are 'SERVER', 'VNODE', 'CLASSID', 'INSTANCE',
		       'VALUE', or 'PERMISSIONS'" */
		    GM_2error( (GWX_RCB *)0, E_GW8182_OBJ_BAD_IS_CLAUSE,
			      GM_ERR_USER,
			      GM_dbslen( dmt_att->att_name.db_att_name ),
			      (PTR)&dmt_att->att_name,
			      0, src );
		    break;
		}
		STmove( src, ' ', sizeof(xatp->xatt_classid),
		       xatp->xatt_classid);
	    }
	}

	if( db_stat == E_DB_OK )
	    gwx_rcb->xrcb_error.err_code = E_DB_OK;

    } while(FALSE);

    return( db_stat );
}

/*{
** Name:  GOidxf	- do index registration.	FIXME
**
**  Description:
**
**	Currently error/no-op.  No indexes supported in IMA (yet).
**
**	Format extended gateway indexes iigwX_index tuple into the provided
**	tuple buffer.  Note the size of this buffer was returned from the init
**	request.
**  Inputs:
**	 gwx_rcb->xrcb_tab_id
**			INGRES table id.
**	 gwx_rcb->xrcb_var_data1
**			source of the gateway table.
**	 gwx_rcb->xrcb_var_data2
**			path of the gateway table.  FIXME.
**  Outputs:
**	 gwx_rcb->xrcb_var_data2
**			path of the gateway table. 
**	 gwx_rcb->xrcb_error.err_code
**			FIXME
**  Returns:
**	 FIXME
** History:
**	whenever (daveb)
**		Created.
*/
static DB_STATUS
GOidxf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_idxfs );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*
** Name: GOopen	- open this 'table'
**
** Description:
**	Open as an 'objects' table.  Create the internal stuff needed,
**	and validate the table structure and keys for sanity.
**
** Inputs:
**
**   gwx_rcb->
**	xrcb_access_mode
**			table access mode, from gwr_access_mode
**	xrcb_rsb
**			GW_RSB for this table.
**
**	    .gwrsb_rsb_mstream
**			Stream to use for memory.
**	    .gwrsb_tcb->gwt_attr
**			INGRES and extended attributes for this table.
**
**	xrcb_data_base_id
**			PTR database id from gwr_database_id.
**	xrcb_xact_id
**			transaction id, from gwr_xact_id.
**	xrcb_tab_name
**			table name, from gwr_tab_name.
**	xrcb_tab_id
**			table id, from gwr_tab_id.
**
** Outputs:
**	xrcb_rsb.gwrsb_page_cnt
**			set to a likely value.
**	xrcb_rsb.gwrsb_internal_rsb
**			Gets pointer to our allocated and set up GO_RSB.
**	xrcb_error.err_code
**			E_DB_OK or error status.
**
** Side Effects:
**	Memory is allocated from xrcb_rsb.gwrsb_rsb_mstream.
**
** Returns:
**	E_DB_OK or E_DB_ERROR.
**
** History:
**	19-feb-91 (daveb)
**	    Created.
**	5-dec-91 (daveb)
**	    modified for vectored out table handling.
**	30-jun-92 (daveb)
**	    - initial debugging: request and response aren't allocated,
**	    initialize ncols for use by GM_ad_out_col.
**	1-jul-92 (daveb)
**	    Keep track of the output attributes for use by GM_ad_out_col.
**	    Null the key strings as well as they're values.  do_op is
**	    cavalier about looking at the length, and it's cheaper to
**	    clear them here.  Symtom is different results on subsequent
**	    scans in a session.
**	28-sep-92 (daveb)
**	    Call GM_open_sub for bulk of open
**	29-sep-92 (daveb)
**	    Fill in GM_ATT_ENTRYs in the open.
**	2-Feb-1993 (daveb)
**	    improve comments.
**	06-oct-93 (swm)
**	    Bug #56443
**	    Changed comment "PTR/i4 database id from gwr_database_id."
**	    to "PTR database id from gwr_database_id." as gwr_database_id
**	    is now a PTR. On axp_osf PTR and i4 are not the same size.
*/
static DB_STATUS
GOopen( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS		db_stat;
    i4			i;
    
    char		*tblname = gwx_rcb->xrcb_tab_name->db_tab_name;
    char		*err_att;
    GW_RSB		*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB		*tcb = rsb->gwrsb_tcb;
    GM_XREL_TUPLE	*xrel = (GM_XREL_TUPLE *)tcb->gwt_xrel.data_address;
    GM_RSB		*gwm_rsb;
    GM_ATT_ENTRY	*gwm_atts;

    i4			xrel_flags;
    i4		err_stat;

    GM_incr( &GM_globals.gwm_stat_sem, &GO_opens );
    db_stat = GM_open_sub( gwx_rcb, sizeof( GO_RSB ) );

    if( db_stat == E_DB_OK )
    {
	/* Sanity check on column types. */
	
	rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
	gwm_rsb = (GM_RSB *)rsb->gwrsb_internal_rsb;
	gwm_atts = gwm_rsb->gwm_gwm_atts;
	
	for( i = 0 ; i < gwm_rsb->ncols; i++ )
	{
	    if( gwm_atts[i].gma_type == 0 )
	    {
		/* Officially this can't happen, because
		   we validated the is clauses at tabf time. */
		
		/* E_GW8183_OBJ_BAD_COL_TYPE:SS50000_MISC_ING_ERRORS
		   "GWM: Internal error:  Column '%0c' has bad type '%1c'" */
		
		GM_2error( (GWX_RCB *)0, E_GW8182_OBJ_BAD_IS_CLAUSE,
			  GM_ERR_USER,
			  GM_dbslen( gwm_atts[i].gma_dmt_att->att_name.db_att_name ),
			  (PTR)&gwm_atts[i].gma_dmt_att->att_name,
			  0, (PTR)gwm_atts[i].gma_col_classid );
		
		db_stat = E_DB_ERROR;
	    }
	    
	}
	
	/* Make sure we have sensible key and structure combinations.
	   Unfortunately, this boils down to tedious case analysis,
	   and happens at open rather than register time.  That's
	   because we don't have the key info at register, and because we
	   didn't have the storage structure stuff here, we had to
	   save it in the xrel_flags */
	
	MEcopy( (PTR)&xrel->xrel_flags, sizeof(xrel_flags), (PTR)&xrel_flags );
	err_stat = OK;
	err_att = NULL;
	switch( gwm_rsb->nkeys )
	{
	case 3:
	    
	    if( gwm_atts[0].gma_key_type != GMA_SERVER && 
	       gwm_atts[0].gma_key_type != GMA_VNODE )
	    {
		err_att = gwm_atts[0].gma_key_dmt_att->att_name.db_att_name;
		err_stat = E_GW8188_NOT_PLACE;
	    }
	    if( gwm_atts[1].gma_key_type != GMA_CLASSID )
	    {
		err_att = gwm_atts[1].gma_key_dmt_att->att_name.db_att_name;
		err_stat = E_GW8189_NOT_CLASSID;
	    }
	    if( gwm_atts[2].gma_key_type != GMA_INSTANCE )
	    {
		err_att = gwm_atts[2].gma_key_dmt_att->att_name.db_att_name;
		err_stat = E_GW818A_NOT_INSTANCE;
	    }
	    if( !(xrel_flags & GM_UNIQUE) )
	    {
		err_stat = E_GW818B_NOT_UNIQUE;
	    }
	    break;
	    
	case 2:
	    
	    if( gwm_atts[0].gma_key_type == GMA_SERVER || 
	       gwm_atts[0].gma_key_type == GMA_VNODE )
	    {
		if( gwm_atts[1].gma_key_type != GMA_CLASSID )
		{
		    err_att = 
			gwm_atts[1].gma_key_dmt_att->att_name.db_att_name;
		    err_stat = E_GW8189_NOT_CLASSID;
		}
		if( xrel_flags & GM_UNIQUE )
		{
		    err_stat = E_GW818C_IS_UNIQUE;
		}
	    }
	    else if( gwm_atts[0].gma_key_type == GMA_CLASSID )
	    {
		if( gwm_atts[1].gma_key_type != GMA_INSTANCE )
		{
		    err_att = 
			gwm_atts[1].gma_key_dmt_att->att_name.db_att_name;
		    err_stat = E_GW818A_NOT_INSTANCE;
		}
		if( !(xrel_flags & GM_UNIQUE) )
		{
		    err_stat = E_GW818B_NOT_UNIQUE;
		}
	    }
	    else
	    {
		err_att = gwm_atts[0].gma_key_dmt_att->att_name.db_att_name;
		err_stat = E_GW818D_NOT_PLACE_CLASSID;
	    }
	    break;
	    
	case 1:
	    
	    if( gwm_atts[0].gma_key_type != GMA_SERVER && 
	       gwm_atts[0].gma_key_type != GMA_VNODE &&
	       gwm_atts[0].gma_key_type != GMA_CLASSID )
	    {
		err_att = gwm_atts[0].gma_key_dmt_att->att_name.db_att_name;
		err_stat = E_GW818D_NOT_PLACE_CLASSID;
	    }
	    if( xrel_flags & GM_UNIQUE )
	    {
		err_stat = E_GW818C_IS_UNIQUE;
	    }
	    break;
	    
	case 0:
	    
	    /* Can't give error here, because the first open is
	       just after the GOtabf, and before the keys are
	       setup in DMF */

	    break;
	    
	default:
	    err_stat = E_GW818F_TOO_MANY_KEYS;
	    break;
	}
	if( err_stat != OK )
	{
	    db_stat = E_DB_ERROR;
	    if( err_att == NULL )
	    {
		GM_1error( gwx_rcb, err_stat, GM_ERR_USER,
			  GM_dbslen( tblname ), (PTR)tblname );
	    }
	    else
	    {
		GM_2error( gwx_rcb, err_stat, GM_ERR_USER,
			  GM_dbslen( tblname ), (PTR)tblname,
			  GM_dbslen( err_att ), (PTR)err_att);
	    }
	}
    }
    return( db_stat );
}



/*{
** Name:  GOclose	- close objects RSB stream.
**
** Note:  memory allocated by GOopen is freed at a higher level when
**	  the entire stream is released.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		
**
** Outputs:
**	gwx_rcb
**
** Returns:
**	DB_STATUS
**
** History:
**	2-Feb-1993 (daveb)
**	    documented.
*/
static DB_STATUS
GOclose( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_closes );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*
** Name: GOposition	- position the record stream.
**
** Description:
**	Set the "current" and "last" keys for a scan, and prepare
**	for the first call.
**
** Inputs:
**	gwx_rcb->xrcb_rsb.
**	    .xrcb_var_data1.data_in_size	number of start key attrs
**						supplied
**	    .xrcb_var_data1.data_address	tuple holding start key attrs
**
**	    .xrcb_var_data2.data_in_size	number of stop key attrs
**						supplied
**	    .xrcb_var_data2.data_address	tuple holding stop key attrs
**
**	    .xrcb_var_data_lst.ptr_address	key description array.
**	    .xrcb_var_data_lst.ptr_in_count	number of elements in key
**						description array.
** Outputs:
**	gwx_rcb->xrcb_rsb->gwrsb_internal_rsb.cur_key
**	gwx_rcb->xrcb_rsb->gwrsb_internal_rsb.last_key
**
** Returns:
**	DB_STATUS.
**
** History:
**	10-apr-90 (bryanp)
**	    Created.
**	17-Jan-92 (daveb)
**	    Heavily modified for MOB.
**	30-jun-92 (daveb)
**	    Always set cl_stat; no random return!  Symtom: position failure.
**	17-Sep-92 (daveb)
**	    Put guts into GM_position; it's common with GXposition.
*/
static DB_STATUS
GOposition( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS db_stat;

    GM_incr( &GM_globals.gwm_stat_sem, &GO_positions );
    db_stat = GM_pos_sub( gwx_rcb, FALSE );
    return( db_stat );

}

/*
** Name: GOget   - get next record
**
** Description:
**	This exit performs a record get from a positioned record stream.
**
** Inputs:
**	gwx_rcb->
**	  xrcb_rsb	    record stream control block
**	  var_data1.
**	    data_address    address of the tuple buffer
**	    data_in_size    the buffer size
**        gwrsb_internal_rsb.
**          didfirst        FALSE if this is first time through.
**          low_name         if didfirst == FALSE, either an exact key to
**                          start the search, or a empty length string
**                          for a full scan.  If didfirst == TRUE, then
**                          the previous key found.
**          low_len         length of the low_name.
**          high_name        key to terminate search when >
**          high_len        length of high_name, 0 if no high key.
**
** Outputs:
**	gwx_rcb->
**	  var_data1.
**	    data_address    record stored at this address
**	    data_out_size   size of the record returned
**	  error.err_code    E_DB_OK
**			    E_GW0641_END_OF_STREAM
**        gwrsb_internal_rsb.
**          didfirst           set to TRUE.
**          low_name	    updated to record key found.
**          low_len         updated to current length of low_name.
**
** Returns:
**	E_DB_OK		    Function completed normally
**	E_DB_ERROR	    Function completed abnormally, or EOF reached
**
** History:
**	19-feb-91 (daveb)
**	    Sketch MO get; assumes record output buffer is big enough and
**	    relatively permanent.
**	17-Jan-92 (daveb)
**	    Heavily modified for MOB.
**	30-jun-92 (daveb)
**	    Check for last key valid before declaring end-of-scan.  If it's
**	    empty, you'd get end on the first record.
**	1-Jul-92 (daveb)
**	    Remove check on output tuple size and it's pre-emptive fill.
**	    Assume that later code does the right thing based on the
**	    passed in attributes.  Force cur_key.valid after first get
**	    so we use the right thing the next time through.  Number
**	    attributes to GM_ad_out_col from 1 rather than 0 to match
**	    attr id's in catalogs.  Do perms in decimal, not octal.
**	16-jul-92 (daveb)
**	    GM_key_compare > 0, not GM_key_compare >= 0, so we get some rows
**	    back when lastkey is same as curr key.
**	29-sep-92 (daveb)
**	    For xtab place support, GM_ad_out_col got a srclen argument;
**	    pass in 0 here in all cases.
**	30-sep-92 (daveb)
**	    Major revisions to do place iteration and position-key
**	    validation here.
**	11-Nov-1992 (daveb)
**	    GM_op now takes rsb, not opblks and key.
**	25-Nov-1992 (daveb)
**	    Completely generalize the handling of columns, removing all
**	    the hard-wiring.
**	31-Oct-1995 (reijo01)
**	    When one row is requested and the status comes back MO_NO_NEXT,
**	    don't treat it as an error. Just format the tuple buffer and 
**	    continue as normal.
*/
static DB_STATUS
GOget( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    STATUS	cl_stat = OK;
    DB_STATUS	db_stat = E_DB_OK;

    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GO_RSB	*go_rsb = (GO_RSB *)rsb->gwrsb_internal_rsb;
    GM_GKEY	*key;

    /* used with MO calls */
    i4		op;
    i4		lcbuf;
    char	cbuf[ GM_MIB_VALUE_LEN ];
    i4		perms;

    GM_incr( &GM_globals.gwm_stat_sem, &GO_gets );

    /* May need to loop over places to get next row. */

    key = &go_rsb->gwm_rsb.cur_key;
    for(;;)
    {
	if (go_rsb == 0)
	{
	    gwx_rcb->xrcb_error.err_code = E_GW0401_NULL_RSB;
	    db_stat = E_DB_ERROR;
	    break;
	}

	/* Check for end of scan.  The cur_key is always valid for this. */

	if( go_rsb->gwm_rsb.last_key.valid &&
	   GM_key_compare( key, &go_rsb->gwm_rsb.last_key ) > 0)
	{
	    gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
	    db_stat = E_DB_ERROR;
	    break;
	}
	
	/* if this is first call and we have a real key, try MOget */

	op = (!go_rsb->gwm_rsb.didfirst && key->valid) ? MO_GET : MO_GETNEXT;

	/* this actually runs the operation.  if local, just calls MO;
	   otherwise does OPBLK/GCM stuff. */

	lcbuf = sizeof(cbuf);
	db_stat = GM_op( op, &go_rsb->gwm_rsb,
			&lcbuf, cbuf, &perms, &cl_stat );

	go_rsb->gwm_rsb.didfirst = TRUE;

	/*
	** (1)  Got it, format it up and we're done.
	** (2)  NO_INSTANCE or NO_CLASSID when we did a GET, so it
	**      must have been a bad position.  Knock key down to
	**      a scan and try it again.
	** (3)  NO_NEXT at this place; look for the next place
	**      and try again.
	** (4)  MO_GET and MO_NO_NEXT; format buffer and return normal. 
	** (5)  Unexpected error.
	*/

	if( db_stat == E_DB_OK )
	{
	    /* for subsequent calls on this table */
	    key->valid = TRUE;
	    db_stat = GO_fmt_tuple( go_rsb, key, cbuf, perms,
			       gwx_rcb->xrcb_var_data1.data_in_size,
			       (char *)gwx_rcb->xrcb_var_data1.data_address );
	    if( db_stat == OK )
		break;
	}
	else			/* not E_DB_OK */
	{
	    /* Maybe bad position.  Try to force a scan it if that is
	       reasonable */
	    
	    if( op == MO_GET &&
	       (cl_stat == MO_NO_INSTANCE || cl_stat == MO_NO_CLASSID ))
	    {
		GM_incr( &GM_globals.gwm_stat_sem, &GO_bad_ipos );

		key->instance.str[0] = EOS;
		key->instance.len = 0;
		if( cl_stat == MO_NO_CLASSID )
		{
		    key->classid.str[0] = EOS;
		    key->classid.len = 0;
		}
		/* continues */
	    }
	    else if( op == MO_GETNEXT )
	    {
		if( cl_stat != MO_NO_NEXT && cl_stat != MO_NO_INSTANCE )
		{
		    /* FIXME:  may don't log this at all, or other
		       specific codes? */
		    GM_error( (i4)cl_stat );
		}
		if( OK != (cl_stat = GM_rsb_place( &go_rsb->gwm_rsb ) ) )
		{
		    gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
		    db_stat = E_DB_ERROR;
		    break;
		}
		/* continues */
	    }
	    else if( op == MO_GET && cl_stat == MO_NO_NEXT)
	    {
	    	/* for subsequent calls on this table */
	    	key->valid = TRUE;
	    	db_stat = GO_fmt_tuple( go_rsb, key, cbuf, perms,
			       gwx_rcb->xrcb_var_data1.data_in_size,
			       (char *)gwx_rcb->xrcb_var_data1.data_address );
	    	if( db_stat == OK )
		    break;
	    }
		/* continues */
	    else		/* completely unexpected error */
	    {
		GM_error( (i4)cl_stat );
		gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
		break;
	    }
	}
    }
    return ( db_stat );
}

/*{
** Name:	GOput	- insert a new row.
**
** Description:
**	insert a new row.  unimplemented for GWM.
**
** Inputs:
**	gwx_rcb		ignored.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented better.
*/
static DB_STATUS
GOput( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_puts );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:	GOreplace	- replace row at current position.
**
** Description:
**
**	Replace the tuple at the current position.
**
** Re-entrancy:
**	yes.
**
**  Inputs:
**	 gwx_rcb->xrcb_var_data1.data_address
**			the tuple to put.
**	 gwx_rcb->xrcb_var_data1.data_in_size
**			the length of the tuple.
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**			gateway private state for the open instance.
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
**
** History:
**	11-Nov-1992 (daveb)
**	    Documented and made do something.
**	12-Jan-1993 (daveb)
**	    Always return OK err_code.  We report all errors before we exit.
*/
static DB_STATUS
GOreplace( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB		*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GO_RSB		*go_rsb = (GO_RSB *)rsb->gwrsb_internal_rsb;
    GM_ATT_ENTRY	*gwm_atts;	/* table */
    GM_ATT_ENTRY	*gwm_att;	/* one of interest */

    char	*tuple = (char *)gwx_rcb->xrcb_var_data1.data_address;

    STATUS	cl_stat = OK;
    DB_STATUS	db_stat = E_DB_OK;

    i4		perms;
    i4		width;
    i2		slen;
    char	*newval;
    i4		i;

    GM_incr( &GM_globals.gwm_stat_sem, &GO_replaces );

    /* FIXME:  validate cur_key against the tuple;
       if it changed, the user is trying to update
       key values, which we shouldn't allow. */

    rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    gwm_atts = go_rsb->gwm_rsb.gwm_gwm_atts;
    for( i = 0 ; i < go_rsb->gwm_rsb.ncols ; i++ )
    {
	/* Only the first GMA_VALUE is taken! */

	if( gwm_atts[ i ].gma_type == GMA_VALUE )
	{
	    gwm_att = &gwm_atts[i];
	    newval = &tuple[ gwm_att->gma_att_offset ];

	    /* FIXME: This assumes the column is a varchar */

	    MECOPY_CONST_MACRO( (PTR)newval, sizeof(slen), (PTR)&slen );
	    newval += sizeof(slen);

	    /* FIXME:  assumes it's safe to clobber what follow! */
	    width = slen;
	    newval[ width ] = EOS;

	    db_stat = GM_op( GM_SET, &go_rsb->gwm_rsb,
			    &width, newval, &perms, &cl_stat );
	    break;
	}
    }
    
    if( db_stat != E_DB_OK )
	GM_error( (i4) cl_stat );

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( db_stat );
}


/*{
** Name:	GOdelete	- Delete positioned row no-op.
**
** Description:
**	Delete row no-op.
**
** Inputs:
**	gwx_rcb		ignored.
**
** Outputs:
**	none
**
** Returns:
**	E_DB_OK
**
** History:
**	2-Feb-1993 (daveb)
**	    documented better.
*/
static DB_STATUS
GOdelete( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_deletes );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:	GObegin	- begin transaction.
**
** Description:
**	Note begin of transaction.
**
** Inputs:
**	gwx_rsb		ignored.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK.
**
** Side Effects:
**	counts calls.
**
** History:
**	20-Nov-1992 (daveb)
**	    documented.
*/
static DB_STATUS
GObegin( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_begins );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:	GOabort	- abort transaction no-op.
**
** Description:
**	abort transaction no-op.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	gwx_rcb		ignored.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented better.
*/
static DB_STATUS
GOabort( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_aborts );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:	GOcommit	- commit transaction no-op.
**
** Inputs:
**	gwx_rcb		ignored.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented better.
*/
static DB_STATUS
GOcommit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GO_commits );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*
** Name: GOinit	- initializaiton for this table GW.
**
** Description:
**	This routine performs one-time initialization for this sub-gateway.
**
** Inputs:
**	gwx_rcb
**
** Outputs:
**	gwx_rcb->xrcb_exit_table	filled in.
**
** Returns:
**	E_DB_OK		Exit completed normally
**	other		status returned by called code.
**
** History:
**	19-feb-91 (daveb)
**	    Created.
**	5-dec-91 (daveb)
**	    stripped down.
**	2-Feb-1993 (daveb)
**	    documented better
*/
DB_STATUS
GOinit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    (VOID) MOclassdef( MAXI2, GO_classes );

    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM]	= GOterm;
/*    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO]	= GOinfo; */

    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF]	= GOtabf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF]	= GOidxf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN]	= GOopen;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE]	= GOclose;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION]	= GOposition;
    (*gwx_rcb->xrcb_exit_table)[GWX_VGET]	= GOget;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT]	= GOput;
    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE]	= GOreplace;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE]	= GOdelete;
    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN]	= GObegin;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT]	= GOcommit;
    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT]	= GOabort;
    (*gwx_rcb->xrcb_exit_table)[GWX_VATCB]	= NULL;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDTCB]	= NULL;

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:	GO_fmt_tuple	-- format output tuple
**
** Description:
**	Given an open table instance, and data about the object,
**	format an output tuple with the right data types.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	go_rsb		the object rsb, for column info.
**	key		the key for this row, used for key columns.
**	cbuf		the value, as a string.
**	perms		the perms of the object.
**	tup_len		length of the tuple buffer.
**
** Outputs:
**	tuple		filled in.
**	
** Returns:
**	E_DB_OK or error.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented.
*/

static DB_STATUS
GO_fmt_tuple( GO_RSB *go_rsb, GM_GKEY *key, char *cbuf, i4  perms,
	     i4  tup_len, char *tuple )
{	
    DB_STATUS		db_stat = E_DB_OK;
    i4			col;
    i4			used;
    i4			srclen;
    char		*q;
    char		*src;
    GM_ATT_ENTRY	*gwm_att;
    char		permsbuf[ 20 ];

    /* format up the output tuple */
    
    used = 0;
    
    for( col = 0; db_stat == E_DB_OK && col < go_rsb->gwm_rsb.ncols; col++ )
    {
	/* locate the column description */
	
	gwm_att = &go_rsb->gwm_rsb.gwm_gwm_atts[ col ];
	
	/* if it's a classid, pull out of the response block */
	
	switch( gwm_att->gma_type )
	{
	case GMA_PLACE:
	case GMA_SERVER:
	    src = key->place.str;
	    srclen = 0;
	    break;
	    
	case GMA_VNODE:
	    
	    src =  key->place.str;
	    for( q = src;
		*q && q < &src[GM_MIB_PLACE_LEN] && *q != ':' ; )
		q++;
	    srclen = q - src;
	    break;
	    
	case GMA_CLASSID:
	    
	    src = key->classid.str;
	    srclen = 0;
	    break;
	    
	case GMA_INSTANCE:
	    
	    src = key->instance.str;
	    srclen = 0;
	    break;
	    
	case GMA_VALUE:
	    
	    src = cbuf;
	    srclen = 0;
	    break;
	    
	case GMA_PERMS:
	    
	    STprintf( permsbuf, "%d", perms );
	    src = permsbuf;
	    srclen = 0;
	    break;
	    
	default:
	    
	    /* "GWM:  Internal error:  bad attr type %0d for column
	       %1d (%2c)" */

	    GM_3error( (GWX_RCB *)0, E_GW8186_BAD_ATTR_TYPE, GM_ERR_INTERNAL,
		      sizeof( gwm_att->gma_type ), (PTR)&gwm_att->gma_type,
		      sizeof( col ), (PTR)&col,
		      GM_dbslen( gwm_att->gma_dmt_att->att_name.db_att_name ),
		      (PTR)&gwm_att->gma_dmt_att->att_name );

	    db_stat = E_DB_ERROR;
	    break;
	}
	if( db_stat == E_DB_OK )
	{
	    db_stat = GM_ad_out_col( src,
				    srclen,
				    col,
				    &go_rsb->gwm_rsb,
				    tup_len,
				    tuple,
				    &used );
	}
    }
    return( db_stat );
}
