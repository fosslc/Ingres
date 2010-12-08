/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <st.h>
#include    <pc.h>
#include    <lg.h>
#include    <scf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ulm.h>

#include    <gwf.h>
#include    <gwfint.h>

/*
** Name: GWLGTST.C	- Read-only test version of the LG Gateway
**
** Description:
**	This file contains the routines which implement a read-only
**	gateway to the Logging System underneath the GWF non-SQL Gateway
**	mechanism. This gateway is intended primarily as a way to test
**	the server-level gateway code (GWF and DMF).
**
** History:
**	27-mar-90 (bryanp)
**	    Added this comment, brought up to date for GWF use.
**	10-Nov-92 (daveb)
**	    Include dmrcb.h for prototyped gwfint.h
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-sep-1996 (canor01)
**	    Moved global data definitions to gwftestdata.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-sep-2003 (abbjo03)
**	    Include pc.h for definition of PID (required by lg.h).
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add prototypes for all statics
*/

GLOBALREF   char    *Gwlgtst_version;

static DB_STATUS	gw08_term( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_tabf( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_idxf( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_open( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_close( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_position( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_get( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_put( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_replace( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_delete( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_begin( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_abort( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_commit( GWX_RCB	*gwx_rcb );
static DB_STATUS	gw08_info( GWX_RCB	*gwx_rcb );
FUNC_EXTERN DB_STATUS	gw08_init( GWX_RCB	*gwx_rcb );

/*
** Name: gw08_term	- terminate gateway
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
**	27-mar-90 (bryanp)
**	    Created.
*/
static DB_STATUS
gw08_term( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_tabf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


static DB_STATUS
gw08_idxf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}


static DB_STATUS
gw08_open( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_close( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_position( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

/*
** Name: gw08_get   - get next record
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
**	27-mar-90 (bryanp)
**	    Created.
*/
static DB_STATUS
gw08_get( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    char	    *record = (char *)gwx_rcb->xrcb_var_data1.data_address;
    LG_STAT	    lg_stat;
    CL_SYS_ERR	    sys_err;
    STATUS	    status;
    i4		    stat_size;

    /*
    ** Retrieve LG stats from logging system
    */
    status = LGshow(LG_S_STAT, (char *)&lg_stat, sizeof(lg_stat), 
  		    &stat_size, &sys_err);

    if ((status != OK) || (stat_size != sizeof(lg_stat)) ||
	(gwx_rcb->xrcb_var_data1.data_in_size != sizeof(lg_stat)))
    {
	gwx_rcb->xrcb_error.err_code = status;
	return (E_DB_ERROR);
    }

    STRUCT_ASSIGN_MACRO(lg_stat, (*(LG_STAT *)record));
    gwx_rcb->xrcb_var_data1.data_out_size = sizeof(lg_stat);

    return (E_DB_OK);
}


static DB_STATUS
gw08_put( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_replace( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_delete( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_begin( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_abort( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_commit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

static DB_STATUS
gw08_info( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_var_data1.data_address = Gwlgtst_version;
    gwx_rcb->xrcb_var_data1.data_in_size = STlength(Gwlgtst_version)+1;

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return( E_DB_OK );
}

/*
** Name: gw08_init	- LG Gateway initialization
**
** Description:
**	This routine performs exit initialization for the LG Gateway.
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
**	    xrelation_sz    set to 0
**	    xattribute_sz   set to 0
**	    xindex_size	    set to 0
**	    error.err_code  E_DB_OK
**			    E_GW0654_BAD_GW_VERSION
**
** Returns:
**	E_DB_OK		    Exit completed normally
**	E_DB_ERROR	    Exit completed abnormally
**
** History:
**	27-mar-90 (bryanp)
**	    Created.
*/
DB_STATUS
gw08_init( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    if (gwx_rcb->xrcb_gwf_version != GWX_VERSION)
    {
	gwx_rcb->xrcb_error.err_code = E_GW0654_BAD_GW_VERSION;
	return (E_DB_ERROR);
    }

    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM]	= gw08_term;
    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF]	= gw08_tabf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN]	= gw08_open;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE]	= gw08_close;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION]	= gw08_position;
    (*gwx_rcb->xrcb_exit_table)[GWX_VGET]	= gw08_get;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT]	= gw08_put;
    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE]	= gw08_replace;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE]	= gw08_delete;
    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN]	= gw08_begin;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT]	= gw08_commit;
    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT]	= gw08_abort;
    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO]	= gw08_info;
    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF]	= gw08_idxf;

    gwx_rcb->xrcb_exit_cb_size = 0;
    gwx_rcb->xrcb_xrelation_sz = 0;
    gwx_rcb->xrcb_xattribute_sz = 0;
    gwx_rcb->xrcb_xindex_sz = 0;

    gwx_rcb->xrcb_error.err_code = 0;
    return (E_DB_OK);
}
