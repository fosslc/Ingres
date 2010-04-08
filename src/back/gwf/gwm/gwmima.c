/*
**Copyright (c) 2004 Ingres Corporation
**
** NO_OPTIM = int_lnx int_rpl
**
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
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

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"

/**
** Name: gwfima.c	- prototype version of an IMA Gateway
**
** Description:
**
**	Main exit entry point for the GWM IMA gateway.
**
** Exports the following functions:
**
**	gw07_init()	-- init function for gwf!gwtabi_dbms.c
**
**	GM_incr()	-- increment counter under mutex.
**
** Internal functions:
**
**	GM_gw_from()	-- locate GW description given a from clause.
**	GM_flags_gw()	-- locate GW description given an xrel flags field.
**	GM_init()	-- initialize entry vector for client.
**	GM_term()	-- shutdown the gateway completely.
**	GM_tabf()	-- create xrelation and xattribute entries
**	GM_idxf()	-- create xrelation and xindex entries
**	GM_open()	-- open a new record stream for a named table.
**	GM_close()	-- close record stream for an opened table.
**	GM_position()	-- position record stream for opened table.
**	GM_get()	-- get next record from positioned/opened table
**	GM_put()	-- insert record at position in opened table.
**	GM_replace()	-- replace record in opened table.
**	GM_delete()	-- delete positioned recored in opened table.
**	GM_begin()	-- start a new transaction.
**	GM_abort()	-- abort current transaction (no effect).
**	GM_commit()	-- commit current transaction (no effect).
**	GM_info()	-- return info about the gateway.
**
** History:
**	7-Nov-91 (daveb)
**		created from prototype gwfmi.c
**	5-Dec-91 (daveb)
**		modified some more.
**	5-Jun-92 (daveb)
**		changed to use gw07_i.
**	16-Jun-92 (daveb)
**		Correct non-working logic for checking status returns.
**		Caused gateway to hang by not recognizing that we got
**		the sem, leaving it stu
**	30-jun-92 (daveb)
**		Lookup more by name; no previous open is valid;
**		Don't fill in err_stat's here, assume sub-gw's do it.
**	12-oct-92 (daveb)
**		With prototypes in GWF, need to include dmrcb.h.
**	11-Nov-1992 (daveb)
**	    fire up SCB MIB, log error status on classdefs.
**	    get def vnode right.
**	    Init VSTERM entry.
**	13-Nov-1992 (daveb)
**	    fire up place mib too.
**	14-Nov-1992 (daveb)
**	    more def_vnode fixups.
**	18-Nov-1992 (daveb)
**	    set up gcm stuff in globals.
**	19-Nov-1992 (daveb)
**	    include <gca.h> now that we're using GCA_FS_MAX_DATA for the
**	    OPBLK buffer size.
**	24-Nov-1992 (daveb)
**	    Use the 'from' clause to determine if MOB or XTAB instead of
**	    table name.  Save the sub gateway type in the XREL 'flags' field.
**	2-Dec-1992 (daveb)
**	    In init, clear this_vnode.
**	3-Dec-1992 (daveb)
**	    Set gwm_def_domain in init.
**	12-Jan-1993 (daveb)
**	    In GM_init, call GM_my_vnode() and GM_my_server() at startup here to
**	    avoid MO recursion problems later if they are called inside
**	    a get routine.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	2-Feb-1993 (daveb)
**	    Improve comments, use ERx, typo in GW version string.
**	7-jul-1993 (shailaja)
**	    Fixed compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Make comparisons with DB_INGRES_NAME and DB_NDINGRES_NAME
**	    case insensitive.
**	    NOTE:  This is being reintegrated after being regressed.
**	16-Sep-1993 (daveb)
**	    Don't GM_error startup errors, because they'd get sent
**	    to the non-existant user-session.  GM_ERR_LOG them
**	    instead.
**	16-Sep-1993 (daveb)
**	    Use PMgetDefault(1) for vnode instead of PMget().
**	5-Jan-1994 (daveb) 58358
**	    Don't use PMgetDefault above; use GChostname and
**  	    local_vnode resource to determine.  We want domain
**  	    places to map to vnodes or the host, not the PM host 
**  	    identifier.  This matters when the host has dots, which
**  	    PMhostname maps to underscores.
**      21-Jan-1994 (daveb) 59125
**          Add tracing variables and objects.
**	04-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to gwmdata.c.
**      24-Jul-1997 (stial01)
**          GM_init() Init GM_globals.gwm_gchdr_size
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Feb-2002 (hanje04)
**	    BUG 107141
**	    Added NO_OPTIM for Intel Linux to stop SEGV during server startup
**	    if hostname or II_HOSTNAME is in UPPER CASE.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
*/

/* forward refs */

static GM_GW *GM_gw_from( char *fromplace );
static GM_GW *GM_flags_gw( i4 xrel_flags );

static DB_STATUS GM_errmap( DB_STATUS db_stat, GWX_RCB *gwx_rcb );

static DB_STATUS GM_init( GWX_RCB *gwx_rcb );
static DB_STATUS GM_term();
static DB_STATUS GM_sterm();
static DB_STATUS GM_tabf();
static DB_STATUS GM_open();
static DB_STATUS GM_close();
static DB_STATUS GM_position();
static DB_STATUS GM_get();
static DB_STATUS GM_put();
static DB_STATUS GM_replace();
static DB_STATUS GM_delete();
static DB_STATUS GM_begin();
static DB_STATUS GM_commit();
static DB_STATUS GM_abort();
static DB_STATUS GM_info();
static DB_STATUS GM_idxf();

/* Global vars */

GLOBALREF   char    *GM_version;

/*
** any protection that is needed internally.
*/
GLOBALREF GM_GLOBALS GM_globals;

/* static vars */

/*
** Registration table, loaded into MO at gateway initialization.
** In turn they are made visible through SQL by the gateway.
*/

GLOBALREF MO_CLASS_DEF GM_classes[];

static GM_GW Gwm_gwdescs[] =
{
    { GOinit, {0},  "objects", sizeof("objects") - 1 },
    { GXinit, {0},  "tables", sizeof("tables") - 1 },
    { 0 }
};

/*
** Name: GM_incr	- increment count with mutex protection
**
** Description:
**	Increment the i4, protected by the mutex.  Any sem errors is
**	logged, leaving the counter alone.
**
** Inputs:
**	sem		semaphore to use to guard the counter.
**	counterp	pointer to i4 to increment.
**
** Outputs:
**	counterp	counter is incremented.
**
** Returns:
**	none.
**
** History:
**	5-Dec-91 (daveb)
**		created.
**	16-Jun-92 (daveb)
**		Correct non-working logic for checking status returns.
**		Caused gateway to hang by not recognizing that we got
**		the sem, leaving it stuck.
**	29-Jun-92 (daveb)
**		Psem takes an exclusive argument; was handing it only sem.
**	2-Feb-1993 (daveb)
**	    improve documentation.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
*/
VOID
GM_incr( CS_SEMAPHORE *sem, i4 *counterp )
{
    STATUS cl_stat;

    if( OK == (cl_stat = CSp_semaphore( TRUE, sem ) ) )
    {
	(*counterp)++;
	cl_stat = CSv_semaphore( sem );
    }
    if( cl_stat != OK )
    {
	GM_uerror( (GWX_RCB *)0, cl_stat, GM_ERR_INTERNAL|GM_ERR_LOG, 0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0,
		  (i4)0, (PTR)0
		  );
    }
}


/*{
**  Name: GM_gw_from - locate right gateway description for a table
**
** Description:
**
**	Given the from clause, pick a sub-gateway.  Null if bad from clause.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	fromplace	either 'objects' or 'tables'
**
** Outputs:
**	none
**
** Returns:
**	pointer to GM_GW entry, or NULL.
**
** History:
**	22-sep-92 (daveb)
**	    documented.
**	24-Nov-1992 (daveb)
**	    Select gateway base on 'from' clause data.
**	2-Feb-1993 (daveb)
**	    improve comments.   
*/

static GM_GW *
GM_gw_from( char *fromplace )
{
    GM_GW *gwp;

    for( gwp = Gwm_gwdescs; gwp->init != NULL; gwp++ )
	if( gwp->name == NULL || !MEcmp( fromplace, gwp->name, gwp->len ) )
	    break;

    return( gwp->init == NULL ? NULL : gwp );
}


/*{
** Name:	GM_flags_gw \- map flags from saved register to sub-gateway.
**
** Description:
**	Given the flags saved in the extended relation from the register,
**	locate the right sub-gateway.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	flags		the saved flags from the extended relation entry.
**
** Outputs:
**	none.
**
** Returns:
**	GM_GW pointer or NULL.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented.
*/
static GM_GW *
GM_flags_gw( i4 flags )
{
    char *fromplace = "";

    if( flags & GM_MOB_TABLE )
	fromplace = "objects";
    else if( flags & GM_XTAB_TABLE )
	fromplace = "tables";

    return( GM_gw_from( fromplace ) );
}



/*
** Name: gw07_init	- Public IMA Gateway initialization
**
** Description:
**	This routine performs exit initialization for the IMA Gateway.
**
** Inputs:
**	gwx_rcb->	    GW Exit Request Block
**	    gwf_version	    GWF Version ID
**	    exit_table	    Address of exit routine table
**
** Outputs:
**	gwx_rcb->
**	    exit_table	    table contains addresses of exit routines
**	    exit_cb_size    exit control block size
**	    xrelation_sz    set to GM_XREL_LEN
**	    xattribute_sz   set to GM_XATTR_LEN
**	    xindex_size	    set to GM_XINDEX_LEN
**	    error.err_code  E_DB_OK
**			    E_GW0654_BAD_GW_VERSION
**
** Returns:
**	E_DB_OK		    Exit completed normally
**	E_DB_ERROR	    Exit completed abnormally
**
** History:
**	19-feb-91 (daveb)
**	    Created.
*/
DB_STATUS
gw07_init( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
	return( GM_init( gwx_rcb ) );
}


/*
** Name: GM_init	- MO Gateway initialization
**
** Description:
**	This routine performs exit initialization for the MO Gateway.
**
** Inputs:
**	gwx_rcb->	    GW Exit Request Block
**	    gwf_version	    GWF Version ID
**	    exit_table	    Address of exit routine table
**
** Outputs:
**	gwx_rcb->
**	    exit_table	    table contains addresses of exit routines
**	    exit_cb_size    exit control block size
**	    xrelation_sz    set to GM_XREL_LEN
**	    xattribute_sz   set to GM_XATTR_LEN
**	    xindex_size	    set to GM_XINDEX_LEN
**	    error.err_code  E_DB_OK
**			    E_GW0654_BAD_GW_VERSION
**
** Returns:
**	E_DB_OK		    Exit completed normally
**	E_DB_ERROR	    Exit completed abnormally
**
** History:
**	19-feb-91 (daveb)
**	    Created.
**	11-Nov-1992 (daveb)
**	    fire up SCB MIB, log error status on classdefs.
**	    get def vnode right.
**	13-Nov-1992 (daveb)
**	    fire up place mib too.
**	14-Nov-1992 (daveb)
**	    more def_vnode fixups.
**	18-Nov-1992 (daveb)
**	    set up gcm stuff in globals.
**	19-Nov-1992 (daveb)
**	    use non-zero default time-to-live.
**	2-Dec-1992 (daveb)
**	    In init, clear this_vnode.
**	12-Jan-1993 (daveb)
**	    Call GM_my_server() at startup here to set default domain and
**	    avoid MO recursion problems later if they are called inside
**	    a get routine.
**	12-Jan-1993 (daveb)
**	    Assume that err_code was set by sub-gw on an error.
**	14-Sep-1993 (daveb)
**	    Get default vnode from PM instead of NMgtAt.
**	16-Sep-1993 (daveb)
**	    Don't GM_error startup errors, because they'd get sent
**	    to the non-existant user-session.  GM_ERR_LOG them
**	    instead.
**	16-Sep-1993 (daveb)
**	    Use PMgetDefault(1) for vnode instead of PMget().
**	5-Jan-1994 (daveb) 58358
**	    Don't use PMgetDefault above; use GChostname and
**  	    local_vnode resource to determine.  We want domain
**  	    places to map to vnodes or the host, not the PM host 
**  	    identifier.  This matters when the host has dots, which
**  	    PMhostname maps to underscores.
**      24-Jul-1997 (stial01)
**          GM_init() Init GM_globals.gwm_gchdr_size
*/
static DB_STATUS
GM_init( GWX_RCB *gwx_rcb )
{
    STATUS	cl_stat;
    DB_STATUS	rdb_stat;
    DB_STATUS	db_stat = E_DB_OK;
    GM_GW	*gwp;
    i4		i;
    char	*def_vnode;

    char    	hostname[ GM_MIB_PLACE_LEN ];

# ifndef STcompare
    FUNC_EXTERN i4  STcompare(char *a, char *b);
# endif

    if (gwx_rcb->xrcb_gwf_version != GWX_VERSION)
    {
	gwx_rcb->xrcb_error.err_code = E_GW0654_BAD_GW_VERSION;
	return (E_DB_ERROR);
    }

    /* can't do anything w/o this semaphore */

    if( (cl_stat = CSw_semaphore( &GM_globals.gwm_stat_sem,
				 CS_SEM_SINGLE,
				 "GM glbl stats" )) != OK )
    {
	GM_0error( (GWX_RCB *)0,  (i4)cl_stat , GM_ERR_LOG );
	return( E_DB_ERROR );
    }

    if( GM_i_sem( &GM_globals.gwm_places_sem ) != OK )
	return( E_DB_ERROR );
    CSn_semaphore( &GM_globals.gwm_places_sem, "GM glbl places" );

    if( (cl_stat = CScnd_init( &GM_globals.gwm_conn_cnd ) ) != OK )
    {
	GM_0error( (GWX_RCB *)0,  (i4)cl_stat , GM_ERR_LOG );
	return( E_DB_ERROR );
    }
    (void)CScnd_name( &GM_globals.gwm_conn_cnd, "GM glbl conns" );
	
    SPinit( &GM_globals.gwm_places, STcompare );

    /* FIXME, PMget these guys and set non zero... */

    GM_globals.gwm_connects = 0;
    GM_globals.gwm_time_to_live = GM_DEFAULT_TTL;
    GM_globals.gwm_gcn_checks = 0;
    GM_globals.gwm_gcn_queries = 0;
    GM_globals.gwm_max_conns = 0;
    GM_globals.gwm_max_place = 0;
    GM_globals.gwm_gca_cb = gwx_rcb->xrcb_gca_cb;

    GChostname( hostname, sizeof(hostname) );
    GM_get_pmsym( "!.local_vnode", hostname, GM_globals.gwm_def_vnode,
		 sizeof( GM_globals.gwm_def_vnode ));

    GM_globals.gwm_this_server[0] = EOS;
    STcopy( GM_my_server(), GM_globals.gwm_def_domain );

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_inits );
    cl_stat = MOclassdef( MAXI2, GM_classes );
    if( cl_stat != OK )
	GM_0error( (GWX_RCB *)0,  (i4)cl_stat , GM_ERR_LOG );
    cl_stat = GM_scb_startup();
    if( cl_stat != OK )
	GM_0error( (GWX_RCB *)0,  (i4)cl_stat , GM_ERR_LOG );
    cl_stat = GM_pmib_init();
    if( cl_stat != OK )
	GM_0error( (GWX_RCB *)0,  (i4)cl_stat , GM_ERR_LOG );


    GM_globals.gwm_gcm_sends = 0;
    GM_globals.gwm_gcm_reissues = 0;
    GM_globals.gwm_gcm_errs = 0;
    GM_globals.gwm_gcm_readahead = 0;
    GM_globals.gwm_gcm_usecache = TRUE;
    
    GM_globals.gwm_op_rop_calls = 0;
    GM_globals.gwm_op_rop_hits = 0;
    GM_globals.gwm_op_rop_misses = 0;
    
    GM_globals.gwm_rreq_calls = 0;
    GM_globals.gwm_rreq_hits = 0;
    GM_globals.gwm_rreq_misses = 0;

    /*
    ** init sub-gateways, latching any error status.
    ** This is somewhat tricky, becuase we are reusing our
    ** own call out table as output for their init routines,
    ** so we all can have the same interface.  I'm not sure
    ** that this is a great idea, but it lets the sub-gateways
    ** be written as if they were top GWs.
    */
    for( gwp = Gwm_gwdescs; gwp->init != NULL; gwp++ )
    {
	if( ( rdb_stat = (*gwp->init)( gwx_rcb ) ) != E_DB_OK )
	{
	    db_stat = rdb_stat;
	}
	else			/* copy exits in our place */
	{
	    for( i = 0; i < GWX_VMAX; i++ )
		(gwp->exit_table)[ i ] = (*gwx_rcb->xrcb_exit_table)[ i ];
	}
    }

    /* now plant our exits after we've gotten sub-gateways. */

    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM]	= GM_term;
    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO]	= GM_info;

    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF]	= GM_tabf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF]	= GM_idxf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN]	= GM_open;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE]	= GM_close;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION]	= GM_position;
    (*gwx_rcb->xrcb_exit_table)[GWX_VGET]	= GM_get;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT]	= GM_put;
    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE]	= GM_replace;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE]	= GM_delete;
    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN]	= GM_begin;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT]	= GM_commit;
    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT]	= GM_abort;
    (*gwx_rcb->xrcb_exit_table)[GWX_VATCB]	= NULL;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDTCB]	= NULL;
    (*gwx_rcb->xrcb_exit_table)[GWX_VSTERM]	= GM_sterm;

    /*  we don't have a special RCB */

    gwx_rcb->xrcb_exit_cb_size = 0;

    /* these are "well known" relation types */
    
    gwx_rcb->xrcb_xrelation_sz = GM_XREL_LEN;
    gwx_rcb->xrcb_xattribute_sz = GM_XATTR_LEN;
    gwx_rcb->xrcb_xindex_sz = GM_XINDEX_LEN;

    /* assume gwx_rcb->xrcb_error.err_code was set by sub-gateway on error */

    return (db_stat);
}


/*
** Name: GM_term	- terminate gateway
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
**	5-dec-91 (daveb)
**	    latch most recent db_stat, but go through all the tables.
*/
static DB_STATUS
GM_term( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS rdb_stat;
    DB_STATUS db_stat = E_DB_OK;
    GM_GW *gwp;
    
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_terms );

    /* shutdown sub-gateways */

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    for( gwp = Gwm_gwdescs; gwp->name != NULL; gwp++ )
	if( ( rdb_stat = (*gwp->exit_table[ GWX_VTERM ])( gwx_rcb ) )
	   != E_DB_OK )
	    db_stat = rdb_stat;

    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_tabf
**
**	Whatever is necessary to register the table for all time, even past
**	termination of the current server.  This means adding entries to the
**	iigwXX_ tables.  (These tables contain information that is unique to the
**	gateway; DMF looks the appropriate rows up at table open time and hands
**	them back for our use.)  TABF formats gateway extended gateway tables`
**	relation and attribute tuples into the provided tuple buffers.  The
**	sizes of these buffers were returned from the INIT request.  Called
**	when the table is registered.
**  Inputs:
**	 gwx_rcb->xrcb_tab_name
**		The name of the table being registered.
**	 gwx_rcb->xrcb_tab_id
**		INGRES table id.
**	 gwx_rcb->xrcb_gw_id
**		Gateway id, derived from DBMS type.  clause of the
**		register statement.
**	 gwx_rcb->xrcb_column_cnt
**		column count of the table, number of elements in column_attr
**		and var_data_list.
**	 gwx_rcb->column_attr
**		INGRES column information.
**	 gwx_rcb->xrcb_var_data_list
**		Array of extended column attribute strings, from
**		'gateway magic' of the target list of the register statement.
**		A null indicates no extended attributes.
**	 gwx_rcb->xrcb_var_data1
**		gateway options  from the register statement.
**	 gwx_rcb->xrcb_var_data2
**		path of the gateway table (the 'from' clause).
**
**	 gwx_rcb->xrcb_var_data3
**
**			tuple buffer for the iigwX_relation
**
**	 gwx_rcb->xrcb_mtuple_buffs
**
**			An allocated array of iigwX_attribute tuple buffers.
**
**			.ptr_size	length of all attr buffers
**			.ptr_in_count	number of attributes
**			.ptr_address	where they got put.
**  Outputs:
**	 gwx_rcb->xrcb_var_data3
**			.data_in_size	length of relation attribute
**			.data_address	iigwX_relation tuple.
**	 gwx_rcb->xrcb_mtuple_buffs
**			iigwX_attribute tuples.
**	 gwx_rcb->xrcb_error.err_code
**			E_GW0000_OK or E_GW0001_NO_MEM.
**  Returns:
**	 E_DB_OK
**	 E_DB_ERROR.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
**	12-oct-92 (daveb)
**		Simplify no-op if statement.
**	24-Nov-1992 (daveb)
**	    Use 'from' clause to determine if 'object' or 'tables' GW register.
**	    (Swiped code from gwfsxa).
*/
static DB_STATUS
GM_tabf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat = E_DB_ERROR;
    GM_GW	*gwp;
    char	*name = gwx_rcb->xrcb_tab_name->db_tab_name;
    char	*from = gwx_rcb->xrcb_var_data2.data_address;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_tabfs );

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    gwp = GM_gw_from( from );
    if( gwp != NULL )
    {
	db_stat = (*gwp->exit_table[ GWX_VTABF ])( gwx_rcb );
    }
    else
    {
	/* E_GW80C1_BAD_FROM_CLAUSE:SS50000_MISC_ING_ERRORS
	   "IMA Register error:  Invalid 'from' clause value %0c for table %1c.
	   Valid choices are 'objects' or 'tables'" */

	GM_2error( (GWX_RCB *)0, E_GW80C1_BAD_FROM_CLAUSE,
		  GM_ERR_USER,
		  GM_dbslen( from ), (PTR)from,
		  GM_dbslen( name ), (PTR)name );
	db_stat = E_DB_ERROR;
    }
    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_idxf
**  Description:
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
**			E_DB_OK or error code.
**	 gwx_rcb->xrcb_error.err_code
**			E_GW0000_OK or E_GW0001_NO_MEM.
**  Returns:
**	 E_DB_OK
**	 E_DB_ERROR
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
**	12-oct-92 (daveb)
**		Simplify no-op if statement.
**	24-Nov-1992 (daveb)
**	    Use the 'from' clause to determine if MOB or XTAB instead of
**	    table name.  Save the sub gateway type in the XREL 'flags' field.
*/
static DB_STATUS
GM_idxf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat = E_DB_ERROR;
    GM_GW	*gwp;
    char	*name = gwx_rcb->xrcb_tab_name->db_tab_name;
    char	*from = gwx_rcb->xrcb_var_data2.data_address;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_idxfs );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    gwp = GM_gw_from( from );
    if( gwp != NULL )
    {
	db_stat = (*gwp->exit_table[ GWX_VIDXF ])( gwx_rcb );
    }
    else
    {
	/* "IMA Register index error:  Invalid 'from' clause value %0c
	   for table %1c. Valid choices are 'objects' or 'tables'"  */
	GM_2error( (GWX_RCB *)0, E_GW80C2_IDX_BAD_FROM_CLAUSE,
		  GM_ERR_USER,
		  GM_dbslen(from), (PTR)from,
		  GM_dbslen(name), (PTR)name );
	db_stat = E_DB_ERROR;
    }

    return( GM_errmap( db_stat, gwx_rcb ) );
}


/*
** Name: GM_open	- open this table
**
**  Description:
**	Allocate the local RSB from the per-instance memory stream, and
**	initialize it as necessary.  Perform any other table startup that is
**	needed.  It is given the INGRES attribute list for the table, and the
**	extended gateway catalog entries for the table formatted by us at
**	register time
**
**      At present, we also enforce "ingres is the DBA" here as well.
**
**  Inputs:
**	 gwx_rcb->xrcb_rsb->gwrsb_rsb_mstream
**			Stream to do ulm_palloc
**			from to obtain memory.
**	 gwx_rcb->xrcb_tab_id
**			INGRES table id.
**	 gwx_rcb->access_mode
**			table access mode.  FIXME
**	 "gwx_rcb->xrcb_exit_cb??? probably xrcb by now...",
**	 gwx_rcb->xrcb_table_desc
**			INGRES description (DMT_TBL_ENTRY *)
**	 gwx_rcb->xrcb_var_data1
**			iigwX_relation tuple
**	 gwx_rcb->xrcb_column_desc
**			INGRES column descriptions (DMT_ATT_ENTRY *)
**	 gwx_rcb->xrcb_mtuple_bufs
**			array of iigwX_attribute tuples containing GW specific information
**			per-column.
**	 gwx_rcb->xrcb_index_desc
**			INGRES index description (DMT_IDX_ENTRY *)
**	 gwx_rcb->xrcb_var_data2
**			iigwX_index tuple.
**  Outputs:
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**			newly allocated local RSB, type unknown to caller.
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	19-feb-91 (daveb)
**	    Created.
**	30-jun-92 (daveb)
**	    initiail debugging: client allocates gwm_rsb, so can't
**	    set it until after it's exit returns OK.
**	    Don't fill in err_stat's here, assume sub-gw's do it.
**	24-Nov-1992 (daveb)
**	    Use 'from' clause to determine if 'object' or 'tables' GW register.
**	    (Swiped code from gwfsxa).   Save the sub gateway type in the
**	    XREL 'flags' field.
**	04-jun-1993 (ralph)
**	    DELIM_IDENT:
**	    Make comparisons with DB_INGRES_NAME and DB_NDINGRES_NAME
**	    case insensitive.
**	    NOTE:  This is being reintegrated after being regressed.
*/
static DB_STATUS
GM_open( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS		db_stat = E_DB_ERROR;
    GM_GW		*gwp;
    GW_RSB		*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB		*tcb = gw_rsb->gwrsb_tcb;
    GM_XREL_TUPLE	*xrel = (GM_XREL_TUPLE *)tcb->gwt_xrel.data_address;
    char		*name = gwx_rcb->xrcb_tab_name->db_tab_name;
    DB_OWN_NAME		dba;
    DB_NAME		dbname;

    GM_RSB	*gwm_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_opens );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    do
    {
	if( (db_stat = GM_gt_dba( &dba )) != E_DB_OK )
	    break;

	if( (db_stat = GM_dbname_gt( &dbname ) ) != E_DB_OK )
	    break;

	/* check for "$ingres" as the DBA */

	if( STbcompare( DB_NDINGRES_NAME, sizeof(DB_NDINGRES_NAME)-1,
		       (char *)&dba.db_own_name, sizeof( dba.db_own_name ), 
		       TRUE ) &&
	   STbcompare( DB_INGRES_NAME, sizeof(DB_INGRES_NAME)-1,
		      (char *)&dba.db_own_name, sizeof( dba.db_own_name ),
			TRUE ) )
	{
	    /* "IMA:  You may only open IMA tables in a database
	       owned by ingres.\nThe current database '%0c' is owned
	       by '%1c'\n" */
	    GM_2error( (GWX_RCB *)0, E_GW80C3_INGRES_NOT_DBA,
		      GM_ERR_USER|GM_ERR_LOG,
		      GM_dbslen( dbname.db_name ), (PTR)&dbname.db_name,
		      GM_dbslen( dba.db_own_name ), (PTR)&dba.db_own_name );
	    db_stat = E_DB_ERROR;
	    break;
	}

	gwp = GM_flags_gw( xrel->xrel_flags );
	if( gwp == NULL )
	{
	    /* E_GW80C4_NO_SUB_GATEWAY:SS50000_MISC_ING_ERRORS
	       "IMA: Can't find gateway for table '%0c' with saved
	       flags %1d\n", */

	    GM_2error( (GWX_RCB *)0, E_GW80C4_NO_SUB_GATEWAY,
		      GM_ERR_INTERNAL|GM_ERR_LOG, 0, name,
		      sizeof( xrel->xrel_flags ), (PTR)&xrel->xrel_flags );

	    db_stat = E_DB_ERROR;
	}
	else 
	{
	    db_stat = (*gwp->exit_table[ GWX_VOPEN ])( gwx_rcb );
	    if( db_stat == E_DB_OK )
	    {
		/* presume client has allocated internal rsb */
		gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;
		gwm_rsb->gw = gwp;
	    }
	    /* presume he logged any errors */
	}

    } while( FALSE );

    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_close
**
**  Description:
**	Close an IMA RSB
**
**  Inputs:
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
*/
static DB_STATUS
GM_close( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;
    GW_RSB	*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_closes );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    db_stat = (*gwm_rsb->gw->exit_table[ GWX_VCLOSE ])( gwx_rcb );
    return( GM_errmap( db_stat, gwx_rcb ) );
}


/*
** Name: GM_position	- position the record stream.
**
** Description:
**	Set the "current" key for a scan, and prepare for the first call.
**
**  Description:
**	A range of key values may be defined if the table is an index.
**	Otherwise it will be scanned.  
**  Inputs:
**	 gwx_rcb->xrcb_idx_id
**		>= 0 if index positioning is requested.
**	 gwx_rcb->xrcb_var_data1.data_address
**		The input start key data.
**			actually rcb->rcb_ll_ptr; what is that to?  Possibly
**			an array of pointers to the keys, but that doesn't dump
**			sensibly
**		    Here is a critical piece of code from dm2r.c:
**			MEcopy(k->attr_value, a[k->attr_number].length,
**			    &r->rcb_ll_ptr[a[k->attr_number].key_offset]);
**	 gwx_rcb->xrcb_var_data1.data_in_size
**		The length of the input start key data, or 0 if not keyed.
**		actually rcb->rcb_ll_given, number of keys given?
**	 gwx_rcb->xrcb_var_data2.data_address
**		The input stop key data.
**	 gwx_rcb->xrcb_var_data2.data_in_size
**		The length of the input stop key data, or 0 if not keyed.
**		gateway private state for the open instance.
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**		gateway private state for the open instance.
**	 gwx_rcb->xrcb_in_data_lst.ptr_address
**			tcb->tcb_att_key_ptr list (one for each key attr).
**	 gwx_rcb->xrcb_in_data_lst.ptr_in_count
**			tcb->tcb_keys; (number of attrs in above).
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
*/
static DB_STATUS
GM_position( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;
    GW_RSB	*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_positions );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    db_stat = (*gwm_rsb->gw->exit_table[ GWX_VPOSITION ])( gwx_rcb );
    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name: GM_get   - get next record
**
** Description:
**	get record from a positioned record stream.
**
** Inputs:
**	gwx_rcb->
**	  xrcb_rsb	    record stream control block
**	  var_data1.
**	    data_address    address of the tuple buffer
**	    data_in_size    the buffer size
**        gwrsb_internal_rsb.
**	    table		pointer to the table description
**
** Outputs:
**	gwx_rcb->
**	  var_data1.
**	    data_address    record stored at this address
**	    data_out_size   size of the record returned
**	  error.err_code    E_DB_OK
**			    E_GW0641_END_OF_STREAM
**
** Returns:
**	E_DB_OK		    Function completed normally
**	E_DB_ERROR	    Function completed abnormally, or EOF reached
**
** History:
**	19-feb-91 (daveb)
**	    Sketch MO get; assumes record output buffer is big enough and
**	    relatively permanent.
**	5-Dec-91 (daveb)
**	    stripped for vectored call out.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
*/
static DB_STATUS
GM_get( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;
    GW_RSB	*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_gets );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    db_stat = (*gwm_rsb->gw->exit_table[ GWX_VGET ])( gwx_rcb );
    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_put	-- put new row to the gateway.
**  Description:
**	Put a record to the table.  A previous position is not needed.
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
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
*/
static DB_STATUS
GM_put( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;
    GW_RSB	*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_puts );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    db_stat = (*gwm_rsb->gw->exit_table[ GWX_VPUT ])( gwx_rcb );
    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_replace	-- replace row.
**  Description:
**			Replace the tuple at the current position.
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
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
*/
static DB_STATUS
GM_replace( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;
    GW_RSB	*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_replaces );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    db_stat = (*gwm_rsb->gw->exit_table[ GWX_VREPLACE ])( gwx_rcb );
    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_delete	-- delete row
**  Description:
**	Delete the current tuple, either after a position or a get.
**  Inputs:
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**			gateway private state for the open instance.
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		Don't fill in err_stat's here, assume sub-gw's do it.
*/
static DB_STATUS
GM_delete( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    DB_STATUS	db_stat;
    GW_RSB	*gw_rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GM_RSB	*gwm_rsb = (GM_RSB *)gw_rsb->gwrsb_internal_rsb;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_deletes );
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    db_stat = (*gwm_rsb->gw->exit_table[ GWX_VDELETE ])( gwx_rcb );
    return( GM_errmap( db_stat, gwx_rcb ) );
}

/*
** Name:  GM_begin
**  Description:
**	Begin a transaction after an open.    This exit may be set to a NULL
**	function if no transaction management is possible.
**  Inputs:
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**			gateway private state for the open instance.
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		don't bother calling sub-gateways, just return OK.
**		It would costs a name lookup, and we don't really care.
*/
static DB_STATUS
GM_begin( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_begins );
    return( E_DB_OK );
}

/*
** Name:  GM_abort
**  Description:
**	Abort the current transaction, after a begin; exit may be NULL if no
**	transaction support is possible.
**  Inputs:
**	 gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**			gateway private state for the open instance.
**  Outputs:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		don't bother calling sub-gateways, just return OK.
**		It would costs a name lookup, and we don't really care.
*/
static DB_STATUS
GM_abort( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_aborts );
    return( E_DB_OK );
}

/*
** Name:  GM_commit
**  Description:
**	Commit the current transaction after a BEGIN.  The exit may be NULL if
**	no transaction management is possible.
**  Inputs:
**	gwx_rcb->xrcb_rsb->gwrsb_internal_rsb
**			gateway private state for the open instance.
**  Outputs:
**  Returns:
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code.
** History:
**	whenever (daveb)
**		Created.
**	30-jun-92 (daveb)
**		don't bother calling sub-gateways, just return OK.
**		It would costs a name lookup, and we don't really care.
*/
static DB_STATUS
GM_commit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_commits );
    return( E_DB_OK );
}

/*
** Name:  GM_info
**  Description:
**			Fill in string with version of the gateway.
**  Inputs:
**	 None.
**  Outputs:
**	 gwx_rcb->xrcb_var_data1.data_address
**			pointed to a null terminated version string.
**	 gwx_rcb->xrcb_error.err_code
**			E_DB_OK or error code.
**  Returns:
**	 E_DB_OK
**			or error code
*/
static DB_STATUS
GM_info( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_infos );
    gwx_rcb->xrcb_var_data1.data_address = GM_version;
    gwx_rcb->xrcb_var_data1.data_in_size = STlength(GM_version)+1;

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}



/*
** Name: GM_sterm - common code to cleanup a session.
**
** Description:
**	Calls GM_rm_scb for the session.
**
** Inputs:
**	gwx_rcb		ignored.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented better.
*/
static DB_STATUS
GM_sterm( GWX_RCB *gwx_rcb )
{
    return( GM_rm_scb() );
}

/*{
** Name:	GM_errmap	\- map returned sub-gateway errors nicely.
**
** Description:
**	figures out the right DB_STATUS to return, and sets the rcb's
**	xrcb_error code to the proper thing.
**
**	In general, we expect the sub-gateways to completely log and process
**	their errors, so we shouldn't ever return a reportable error.  Thus,
**	any unexpected error left lying around is logged with "you should have
**	seen this already", and then we clear the error status to "already
**	reported to GWF doesn't cascade a pile more on top of us.
**
**	The one returned error that must be propagated up is EOF on get.
**
** Re-entrancy:
**	yes
**
** Inputs:
**	db_stat		returned by sub-gw.
**	gwx_rcb		of the request.
**
** Outputs:
**	gwx_rcb->xrcb_error.err_code.
**
** Returns:
**	status		to return to GWF.
**
** History:
**	2-Feb-1993 (daveb)
**		documented.    
*/
static DB_STATUS
GM_errmap( DB_STATUS db_stat, GWX_RCB *gwx_rcb )
{
    i4	    *error = &gwx_rcb->xrcb_error.err_code;

    switch( *error )
    {
    case E_GW0641_END_OF_STREAM:
	break;

    case E_DB_OK:

	if( db_stat != E_DB_OK )
	{
# if 0
	    /* "GWM:  Internal error:  bad status %0d to GM_errmap, but
	       OK cl_status" */

	    GM_1error( gwx_rcb, E_GW80C6_ERRMAP_ERR_BUT_OK, GM_ERR_INTERNAL,
		      sizeof( db_stat ), (PTR)&db_stat );
# endif
	    *error = E_GW050E_ALREADY_REPORTED;
	}
	break;

    default:

	if( db_stat == E_DB_OK )
	    GM_0error( (GWX_RCB *)0,  E_GW80C8_ERRMAP_BAD_STATUS , GM_ERR_LOG );
	    
	GM_0error( (GWX_RCB *)0,  E_GW80C7_SHOULD_HAVE_SEEN , GM_ERR_LOG );
	GM_0error( (GWX_RCB *)0,  *error , GM_ERR_LOG );
	*error = E_GW050E_ALREADY_REPORTED;
	break;
    }

    return( db_stat );
}


