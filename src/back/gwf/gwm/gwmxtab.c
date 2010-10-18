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
**	GXterm	- terminate gateway
**	GXtabf
**	GXidxf
**	GXopen	- open this 'table'
**	GXclose
**	GXposition	- position the record stream.
**	GXget   - get next record
**	GXput
**	GXreplace
**	GXdelete
**	GXbegin
**	GXabort
**	GXcommit
**	GXinit	- initializaiton for this table GW.
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
**	1-oct-92 (daveb)
**		Add MIB objects for bad positions so we can see if they
**		are doing us any good.
**	9-oct-92 (daveb)
**		broke into gwmxtab.c and gwmxutil.c
**	17-Nov-1992 (daveb)
**	    move GX_next_place inline to GXget
**	19-Nov-1992 (daveb)
**	    include <gca.h> now that we're using GCA_FS_MAX_DATA for the
**	    OPBLK buffer size.
**	23-Nov-1992 (daveb)
**	    no_classid is expected on get to uninteresting place, so
**	    don't log it as bad error.
**	24-Nov-1992 (daveb)
**	    Store sub-gateway type (GWM_XTAB_TABLE) in rel entry, so
**	    open doesn't need to figure it out.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-Aug-1993 (daveb)
**	    Don't log error on unexpectred attribute type; it's already
**	    been handled in GM_ad_out_column.
**      21-Jan-1994 (daveb) 59125
**          Add tracing, set by objects.
**       8-Apr-1994 (daveb) 60738
**          Do GET only after a complete position, not a partial one.
**  	    If you GET a partial key, you'll get no rows, so you
**          should GETNEXT to get the first potential match.
**       9-May-1994 (daveb) 62631
**  	    Fix to 60738 had us getnext on subsequent get of same
**  	    row, when we should be doing a get there.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/

/* forwards */

static DB_STATUS GXterm();
static DB_STATUS GXtabf();
static DB_STATUS GXopen();
static DB_STATUS GXclose();
static DB_STATUS GXposition();
static DB_STATUS GXget();
static DB_STATUS GXput();
static DB_STATUS GXreplace();
static DB_STATUS GXdelete();
static DB_STATUS GXbegin();
static DB_STATUS GXcommit();
static DB_STATUS GXabort();
static DB_STATUS GXidxf();

static void GM_oops1( GM_RSB *gm_rsb );

/* Call counters */

static i4 GX_terms ZERO_FILL;
static i4 GX_tabfs ZERO_FILL;
static i4 GX_idxfs ZERO_FILL;
static i4 GX_opens ZERO_FILL;
static i4 GX_closes ZERO_FILL;
static i4 GX_positions ZERO_FILL;
static i4 GX_gets ZERO_FILL;
static i4 GX_puts ZERO_FILL;
static i4 GX_replaces ZERO_FILL;
static i4 GX_deletes ZERO_FILL;
static i4 GX_begins ZERO_FILL;
static i4 GX_aborts ZERO_FILL;
static i4 GX_commits ZERO_FILL;
static i4 GX_bad_pos ZERO_FILL;

/*
** Registration table, loaded into MO at gateway initialization.
** In turn they are made visible through SQL by the gateway.
*/

static MO_CLASS_DEF GX_classes[] =
{
    { 0, "exp.gwf.gwm.num.xt_terms", sizeof(GX_terms), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_terms, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_tabfs", sizeof(GX_tabfs), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_tabfs, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_idxfs", sizeof(GX_idxfs), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_idxfs, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_opens", sizeof(GX_opens), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_opens, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_closes", sizeof(GX_closes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_closes, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_positions", sizeof(GX_positions), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_positions, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_gets", sizeof(GX_gets), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_gets, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_puts", sizeof(GX_puts), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_puts, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_replaces", sizeof(GX_replaces), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_replaces, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_deletes", sizeof(GX_deletes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_deletes, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_begins", sizeof(GX_begins), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_begins, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_aborts", sizeof(GX_aborts), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_aborts, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_commits", sizeof(GX_commits), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_commits, MOcdata_index },
    { 0, "exp.gwf.gwm.num.xt_bad_positions", sizeof(GX_bad_pos), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&GX_bad_pos, MOcdata_index },

    { 0 }
};


/*
** Name: GXterm	- terminate gateway
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
GXterm( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_terms );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

/*
** Name:  GXtabf
**
** Format appropriate iigwXX_relation and iigwXX_attribute entries for
** this table registration.  Key point is stashing the object id's
** from the 'is' clause of the target list in the extended attr
** table.  Also stores the sub-gateway flag GM_MOB_TABLE
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
**	 gwx_rcb->xrcb_tab_id
**			INGRES table id.
**	 gwx_rcb->xrcb_gw_id
**			Gateway id, derived from DBMS type.
**			clause of the register statement.
**	 gwx_rcb->xrcb_var_data1
**			source of the gateway table, in the 'from'
**			clause of the 'register' statement.  Ignored here.
**	 gwx_rcb->xrcb_var_data2
**			path of the gateway table.
**			How does this differ from the source? FIXME
**			Ignored here anyway.
**	 gwx_rcb->xrcb_column_cnt
**			column count of the table, number of elements
**			in column_attr and var_data_list.
**	 gwx_rcb->column_attr
**			INGRES column information (array of DMT_ATT_ENTRYs).
**	 gwx_rcb->xrcb_var_data_list
**			Array of extended column attribute strings,
**			from 'is' clause of the target list of the
**			register statement.  A null indicates no
**			extended attributes.  
**	 gwx_rcb->xrcb_var_data3
**			tuple buffer for the iigwX_relation
**	 gwx_rcb->xrcb_mtuple_buffs
**			An allocated array of iigwX_attribute tuple buffers.
**
**  Outputs:
**	 gwx_rcb->xrcb_var_data3
**			iigwX_relation tuple.
**	 gwx_rcb->xrcb_mtuple_buffs
**			iigwX_attribute tuples.
**	 gwx_rcb->xrcb_error.err_code
**			E_GW0000_OK or E_GW0001_NO_MEM.
**  Returns:
**	 E_DB_OK
**	 E_DB_ERROR.
**
**  History:
**	24-Nov-1992 (daveb)
**	    Store sub-gateway type (GWM_XTAB_TABLE) in rel entry, so
**	    open doesn't need to figure it out.
**      11-May-1994 (daveb) 63344
**          Do more checking of IS clause.  Insist on VNODE, SERVER or
**  	    classid, and that there is at least one classid.
*/
static DB_STATUS
GXtabf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_XATT_TUPLE	*xatp;
    DMT_ATT_ENTRY	*dmt_att;
    DMU_GWATTR_ENTRY	**gwatt;
    DM_PTR		*ptr;
    char		*src;
    i4			numatts;
    DB_STATUS		db_stat = E_DB_OK;
    bool    	    	got_user_att;

    GM_incr( &GM_globals.gwm_stat_sem, &GX_tabfs );
    do
    {
	/* sanity checks and format relation entry */

	if( (db_stat = GM_tabf_sub( gwx_rcb, GM_XTAB_TABLE )) != E_DB_OK )
	    break;

	/* scan through and format attribute entries */

	numatts = gwx_rcb->xrcb_column_cnt;
	ptr = &gwx_rcb->xrcb_mtuple_buffs;
	gwatt = (DMU_GWATTR_ENTRY **)gwx_rcb->xrcb_var_data_list.ptr_address;
	dmt_att = gwx_rcb->xrcb_column_attr;
	xatp = (GM_XATT_TUPLE *)ptr->ptr_address;
	got_user_att = FALSE;

	for( ; numatts-- ; xatp++, dmt_att++, gwatt++ )
	{
	    xatp->xatt_tblid = *gwx_rcb->xrcb_tab_id;
	    xatp->xatt_attnum = dmt_att->att_number;

	    if( !((*gwatt)->gwat_flags_mask & DMGW_F_EXTFMT) )
	    {
		/* "IMA:  Column '%0c' is missing the required IS clause.
		   Legal values are 'SERVER', 'VNODE', or user classid" */

		GM_1error( (GWX_RCB *)0, E_GW8341_TAB_MISSING_IS_CLAUSE,
			  GM_ERR_USER,
			  dmt_att->att_nmlen, dmt_att->att_nmstr );
		db_stat = E_DB_ERROR;
		break;
	    }
	    else
	    {
		src = (*gwatt)->gwat_xbuffer;

		/* Check 'is' validity here as much as
		   possible.  Allow only user att, VNODE
		   or SERVER, and make sure we have at
		   least one user att. */

		switch( GM_att_type( src ) )
		{
		case GMA_VNODE:
		case GMA_SERVER:
		    break;
		case GMA_USER:
		    got_user_att = TRUE;
		    break;
		default:
		    GM_2error( (GWX_RCB *)0,
			      E_GW834B_INVALID_XTAB_IS_CLAUSE,
			      GM_ERR_USER,
			      dmt_att->att_nmlen, dmt_att->att_nmstr,
			      GM_dbslen( src ),
			      (PTR)src );
		    db_stat = E_DB_ERROR;
		    break;
		}
		if( db_stat == E_DB_OK )
		    STmove( src, ' ', sizeof(xatp->xatt_classid),
			   xatp->xatt_classid);
	    }
	}

	if( !got_user_att )
	{
	    GM_0error( (GWX_RCB *)0, E_GW834C_XTAB_NO_USER_ATT, GM_ERR_USER );
	    db_stat = E_DB_ERROR;
	}
	if( db_stat == E_DB_OK )
	    gwx_rcb->xrcb_error.err_code = E_DB_OK;

    } while(FALSE);

    return( db_stat );
}



/*{
** Name:  GXidxf
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
GXidxf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_idxfs );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*
** Name: GXopen	- open this 'table'
**
**
** Description:
**	Open as a cross-tab table.  Create the internal stuff needed,
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
**			Gets pointer to our allocated and set up GX_RSB.
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
**	20-Jul-92 (daveb)
**	    Record where "PLACE" goes in the out tuple.
**	28-sep-92 (daveb)
**	    moved guts to common GM_open_sub(), classify gma_type here.
**	25-Nov-1992 (daveb)
**	    Column type lookup moved to GM_open_sub, just do XTAB
**	    special case here.
**	7-Dec-1992 (daveb)
**	    Remove all special stuff; it's now handled in GM_open_sub()
**	    completely.
**	3-Feb-1993 (daveb)
**	    improve documentation.
**	06-oct-93 (swm)
**	    Bug #56443
**	    Changed comment "PTR/i4 database id from gwr_database_id."
**	    to "PTR database id from gwr_database_id." as gwr_database_id
**	    is now a PTR. On axp_osf PTR and i4 are not the same size.
*/
static DB_STATUS
GXopen( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS		db_stat;
    GW_RSB		*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB		*tcb = rsb->gwrsb_tcb;
    GM_XREL_TUPLE	*xrel = (GM_XREL_TUPLE *)tcb->gwt_xrel.data_address;
    GM_RSB		*gwm_rsb;
    GM_ATT_ENTRY	*gwm_atts;
    
    i4			rflags;
    char		*tblname = gwx_rcb->xrcb_tab_name->db_tab_name;
    DMT_ATT_ENTRY	*err_att;
    i4		err_stat;

    if( GM_globals.gwm_trace_opens )
	TRdisplay("GWMXOPEN opening table %50s\n", tblname );

    GM_incr( &GM_globals.gwm_stat_sem, &GX_opens );
    db_stat = GM_open_sub( gwx_rcb, sizeof( GX_RSB ) );
    
    if( db_stat == E_DB_OK )
    {
	/* Sanity check on column types. */
	
	rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
	gwm_rsb = (GM_RSB *)rsb->gwrsb_internal_rsb;
	gwm_atts = gwm_rsb->gwm_gwm_atts;
	
	/* Make sure we have sensible key and structure combinations.
	   Unfortunately, this boils down to tedious case analysis,
	   and happens at open rather than register time.  That's
	   because we don't have the key info at register, and because we
	   didn't have the storage structure stuff here, we had to
	   save it in the xrel_flags */
	
	/* we don't know that xrel is aligned... */
	MEcopy( (PTR)&xrel->xrel_flags, sizeof(rflags), (PTR)&rflags );
	err_stat = OK;
	err_att = NULL;
	switch( gwm_rsb->nkeys )
	{
	case 2: 

	    if( gwm_atts[0].gma_key_type == GMA_SERVER ||
	       gwm_atts[0].gma_key_type == GMA_VNODE )
	    {
		if( gwm_atts[1].gma_key_type != GMA_USER )
		{
		    err_att = gwm_atts[0].gma_key_dmt_att;
		    err_stat = E_GW8348_NOT_USER;
		}
		if( !(rflags & GM_UNIQUE) )
		{
		    err_stat = E_GW818B_NOT_UNIQUE;
		}
	    }
	    else
	    {
		err_att = gwm_atts[0].gma_key_dmt_att;
		err_stat = E_GW8188_NOT_PLACE;
	    }
	    break;
	    
	case 1:
	    
	    if( gwm_atts[0].gma_key_type != GMA_SERVER &&
	       gwm_atts[0].gma_key_type != GMA_VNODE &&
	       gwm_atts[0].gma_key_type != GMA_USER )
	    {
		err_att = gwm_atts[0].gma_key_dmt_att;
		err_stat = E_GW8349_NOT_PLACE_USER;
	    }
	    if( gwm_atts[0].gma_key_type == GMA_USER &&
	       !(rflags & GM_UNIQUE) )
	    {
		err_stat = E_GW818B_NOT_UNIQUE;
	    }
	    if( gwm_atts[0].gma_key_type != GMA_USER &&
	       (rflags & GM_UNIQUE) )
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
	    err_stat = E_GW834A_TAB_TOO_MANY_KEYS;
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
			  err_att->att_nmlen, err_att->att_nmstr);
	    }
	}
    }

    if( GM_globals.gwm_trace_opens )
    {
	TRdisplay("GWMXOPEN exit for table %32s, db_stat %d\n", tblname,
		  db_stat );
	TRdisplay("\tgwx_rsb %p, gwm_rsb %p\n", rsb, gwm_rsb );
    }

    return( db_stat );
}


/*{
** Name:  GXclose
**
** Description:
**	Close the opened table.  Note that memory obtained by GXopen is
**	freed at a higher level when the entire ULM stream is released.
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
GXclose( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	*rsb;
    GX_RSB	*xt_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GX_closes );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;

    if( GM_globals.gwm_trace_closes )
    {
	rsb = (GW_RSB *) gwx_rcb->xrcb_rsb;
	xt_rsb = (GX_RSB *)rsb->gwrsb_internal_rsb;
	TRdisplay("GWMXCLOSE rsb %p, xt_rsb %p\n",
		  rsb, xt_rsb );
    }
    return( E_DB_OK );
}


/*
** Name: GXposition	- position the record stream.
**
** Description:
**	Set the "current" and "last" keys for a scan, and prepare
**	for the first call.
**
**	The only parts of the key that we are interested in (with
**	this version) are the place and the instance associated
**	with the index attribute, and there can only be one index
**	attribute.
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
**	17-Sep-92 (daveb)
**	    Put guts into GM_position; it's common with GOposition.
**	3-Feb-1993 (daveb)
**	    document better.
*/
static DB_STATUS
GXposition( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;

    GM_incr( &GM_globals.gwm_stat_sem, &GX_positions );
    db_stat = GM_pos_sub( gwx_rcb, TRUE );

    return( db_stat );
}

/*
** Name: GXget   - get next record
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
**	2-Jul-92 (daveb)
**	    Check cur_key.valid to select op type too, otherwise first of
**	    a scan fails.
**	23-Nov-1992 (daveb)
**	    no_classid is expected on get to uninteresting place, so
**	    don't log it as bad error.
**	23-Aug-1993 (daveb)
**	    Don't log error on unexpectred attribute type; it's already
**	    been handled in GM_ad_out_column.
**	21-Jan-1994 (daveb)
**	    add tracing.
**       8-Apr-1994 (daveb) 60738
**          Do GET only after a complete position, not a partial one.
**  	    If you GET a partial key, you'll get no rows, so you
**          should GETNEXT to get the first potential match.
**       9-May-1994 (daveb) 62631
**  	    Fix to 60738 had us getnext on subsequent get of same
**  	    row, when we should be doing a get there.
**      10-May-1994 (daveb) 63344
**          When only columns in row are pseudo-objects (VNODE, SERVER),
**          return a row for the place, then declare end-of-place.
*/
static DB_STATUS
GXget( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	*rsb;
    DB_STATUS	db_stat = E_DB_OK;
    STATUS	cl_stat = OK;
    GX_RSB	*xt_rsb;
    i4		op;
    i4		n;
    GM_GKEY	key;		/* used by GX_gt_partial_row for 
				   place and instance */
    GM_GKEY	*cur_key;	

    db_stat = E_DB_OK;
    rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    xt_rsb = (GX_RSB *)rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GX_gets );

    /* copy whole key locally */

    cur_key = &xt_rsb->gwm_rsb.cur_key;
    key = *cur_key;

    if( GM_globals.gwm_trace_gets )
    {
	TRdisplay("GWMXGET rsb %p, xt_rsb %p\n",
		  rsb, xt_rsb );
	GM_dump_key( "GWMXGET cur_key", cur_key );
    }

    /* This loop repeats in case (a) we don't get a row from the
       current place and need to iterate, or (b) our initial position
       failed and we need to scan the initial place's instances */

    if (xt_rsb == NULL)
    {
	gwx_rcb->xrcb_error.err_code = E_GW0401_NULL_RSB;
	db_stat = E_DB_ERROR;
    }
    else while( TRUE )
    {
	/* Terminate scan if we're past valid stop key */

	if( (xt_rsb->gwm_rsb.last_key.valid &&
	     GM_key_compare( cur_key, &xt_rsb->gwm_rsb.last_key ) > 0 ) )
	{
	    if( GM_globals.gwm_trace_gets )
	    {
		TRdisplay("  GWMXGET:  declaring END_OF_STREAM, xt_rsb %p\n",
			  xt_rsb);
		GM_dump_key("  cur key:  ", cur_key );
		GM_dump_key("  last key: ",  &xt_rsb->gwm_rsb.last_key );
	    }
	
	    gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
	    db_stat = E_DB_ERROR;
	    break;
	}
	
	/* GET only immediately after a complete position, or getting
          remainder of row that has been started.  Otherwise GETNEXT */

	if( !xt_rsb->gwm_rsb.didfirst && key.valid && key.instance.len )
	    op = GM_GET;	/* newly positioned with a complete key */
	else
	    op = GM_GETNEXT;	/* other new-row cases.

	/* Setup output tuple */
	
	xt_rsb->tuple = gwx_rcb->xrcb_var_data1.data_address;
	xt_rsb->tup_len = gwx_rcb->xrcb_var_data1.data_in_size;
	xt_rsb->tup_used = 0;
	xt_rsb->tup_cols = 0;

	/* This loop gets partial rows; other error checking done is
	   in the place loop. */

	if( GM_globals.gwm_trace_gets )
	    TRdisplay("  GWMXGET:  getting row for xt_rsb %p\n", xt_rsb);

	while( db_stat == E_DB_OK && xt_rsb->tup_cols < xt_rsb->gwm_rsb.ncols )
	{
	    /* reset this to match start */
	    key.instance = cur_key->instance;

	    /* setup request block of what will fit that we need */
	    GM_i_opblk( op, key.place.str, &xt_rsb->gwm_rsb.request );
	    for( n = xt_rsb->tup_cols; n < xt_rsb->gwm_rsb.ncols; n++ )
		if( GX_ad_req_col( xt_rsb, n ) != E_DB_OK )
		    break;
	    db_stat = GM_request( &xt_rsb->gwm_rsb, &key, &cl_stat );

	    /* subsquent ops on this row are GET */
	    
	    if( db_stat == E_DB_OK )
		db_stat = GX_ad_to_tuple( xt_rsb, &cl_stat );
	    else
		db_stat = db_stat; /* DEBUG */
	}

	/* always doing GETNEXT after first row attempted */

	xt_rsb->gwm_rsb.didfirst = TRUE;

	if( db_stat == E_DB_OK )
	{
	    /* this  row is good, copy key back and we're done. */
	    *cur_key = key;
	    break;
	}

	/* Something bad:  position, next place, other error */

	if( GM_globals.gwm_trace_gets )
	    TRdisplay("  GWMXGET:  get failed, cl_status %x\n",
		      cl_stat );

	if( op == GM_GET )
	{
	    if( cl_stat == MO_NO_INSTANCE )
	    {
		/* Position failed?  Try again in this place,
		   scanning the index object w/o logging error */
		
		GM_incr( &GM_globals.gwm_stat_sem, &GX_bad_pos );
		
		if( GM_globals.gwm_trace_gets )
		    TRdisplay("  GWMXGET:  GET position failed, resetting xt_rsb %p\n",
			      xt_rsb );
	    
		key.instance.str[0] = EOS;
		key.instance.len = 0;
	    }
	    else if( cl_stat == OK ) /* shouldn't happen! */
	    {
		GM_error( E_GW8347_GX_GET_OK_STAT_ERR );
		GM_oops1( &xt_rsb->gwm_rsb );
	    }
	    /* err 81C3 logged in GM_ad_out_col, so don't here. */
	    else if( cl_stat != MO_NO_INSTANCE && cl_stat != MO_NO_CLASSID
		    && cl_stat != E_GW81C3_UNEXPECTED_ATTR_TYPE )
	    {
		GM_error( E_GW8344_GX_GET_PART_ERR );
		GM_error( cl_stat );
		GM_oops1( &xt_rsb->gwm_rsb );
		db_stat = E_DB_ERROR;
		break;
	    }
	}
	else if( op == GM_GETNEXT )
	{
	    if( cl_stat == OK )	/* "shouldn't happen!" */
	    {
		/* "GETNEXT db error w/OK status!" */
		GM_error( E_GW8345_GX_GETNEXT_ERR );
		GM_oops1( &xt_rsb->gwm_rsb );
	    }
	    else if( cl_stat != MO_NO_NEXT &&
		    cl_stat != MO_NO_INSTANCE &&
		    cl_stat != MO_NO_CLASSID &&
		    cl_stat != E_GW8381_INCONSISTENT_ROW )
	    {
		/* log other errs? we  get BS_CONNECT errors to dead
		   servers here;  We don't want to know, so do't
		   do anything */
		cl_stat = cl_stat; /* DEBUG */
		if( GM_globals.gwm_trace_gets )
		{
		    TRdisplay( "  GWMXGET:  partial row GETNEXT error, xt_rsb %p\n",
			      xt_rsb );
		    GM_error( cl_stat );
		}
	    }
	    else
	    {
		cl_stat = cl_stat; /* DEBUG */
	    }
	    
	    /* Go to next place and try again w/o terminating scan */
	    
	    db_stat = E_DB_OK;
	    
	    if( GM_globals.gwm_trace_gets )
		TRdisplay("  GWMXGET:  trying next place, xt_rsb %p\n",
			  xt_rsb);

	    if( (cl_stat = GM_rsb_place( &xt_rsb->gwm_rsb )) == OK )
	    {
		key = *cur_key;
		key.instance.str[0] = EOS;
		key.instance.len = 0;
		/* is this needed? */
		STcopy( cur_key->place.str, xt_rsb->gwm_rsb.request.place );
		if( GM_globals.gwm_trace_gets )
		    GM_dump_key("  GWMXGET  new key: ", &key );
	    }
	    else		/* next place not OK, call it EOF */
	    {
		if( cl_stat != MO_NO_NEXT )
		{
		    /* "next place from" */
		    GM_error( E_GW8346_NEXT_PLACE_ERR );
		    GM_error( cl_stat );
		    GM_oops1( &xt_rsb->gwm_rsb );
		}
		if( GM_globals.gwm_trace_gets )
		    TRdisplay("  GWMXGET:  no places END_OF_STREAM, xt_rsb %p\n",
			      xt_rsb);

		gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
		db_stat = E_DB_ERROR;
		break;
	    }
	}
    }
    if( GM_globals.gwm_trace_gets )
    {
	TRdisplay("  GWMXGET exit, db_stat %d\n", db_stat );
	GM_dump_key( "GWMXGET exit cur_key", cur_key );
    }

    return( db_stat );
}


/*{
** Name:  GXput	- insert a new row.
**
** Description:
**	no-op.
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
GXput( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_puts );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:  GXreplace	- replace row at current position.
**
** Description:
**	Replace row is a no-op for cross-tabs.  All XTABs are read-only.
**
** Re-entrancy:
**	yes.
**
**  Inputs:
**	 gwx_rcb	ignored.
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented better.
*/

static DB_STATUS
GXreplace( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_replaces );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:  GXdelete	delete a row.
**
** Description:
**	delete a row.  No-op for XTAB; all are read-only.
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
GXdelete( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_deletes );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:  GXbegin	- begin transaction.
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
**	3-Feb-1993 (daveb)
**	    better doc.
*/

static DB_STATUS
GXbegin( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_begins );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:  GXabort	- abort transaction no-op.
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
GXabort( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_aborts );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:  GXcommit	- commit transaction no-op.
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
GXcommit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GX_commits );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*
** Name: GXinit	- initializaiton for this table GW.
**
** Description:
**	This routine performs one-time initializaiton for this sub-gateway.
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
**	5-dec-92 (daveb)
**	    stripped down.
**	3-Feb-1993 (daveb)
**	    documented better.
*/
DB_STATUS
GXinit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    (VOID) MOclassdef( MAXI2, GX_classes );

    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM]	= GXterm;
/*  (*gwx_rcb->xrcb_exit_table)[GWX_VINFO]	= GXinfo; */

    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF]	= GXtabf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF]	= GXidxf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN]	= GXopen;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE]	= GXclose;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION]	= GXposition;
    (*gwx_rcb->xrcb_exit_table)[GWX_VGET]	= GXget;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT]	= GXput;
    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE]	= GXreplace;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE]	= GXdelete;
    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN]	= GXbegin;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT]	= GXcommit;
    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT]	= GXabort;
    (*gwx_rcb->xrcb_exit_table)[GWX_VATCB]	= NULL;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDTCB]	= NULL;

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


/*{
** Name:	GM_oops1	- internal error function for XTAB.
**
** Description:
**	Error function when something that isn't supposed to happen
**	does.  Dumps out the opblocks with TRdisplay.
**	Presumes a nice message is printed first.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwm_rsb->request		the request to dump
**	gwm_rsb->response		the response to dump
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
*/

static void
GM_oops1( GM_RSB *gm_rsb )
{
    TRdisplay("The request was:\n");
    GM_opblk_dump( &gm_rsb->request );
    TRdisplay("The response is:\n");
    GM_opblk_dump( &gm_rsb->response );
}
