/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPSCFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <cs.h>
#include    <me.h>
#include    <sp.h>
#include    <mo.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>

#include    <lk.h>

#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <duf.h>

#include    <ddb.h>
#include    <qefrcb.h>
#include    <psfparse.h>

#include    <dudbms.h>
#include    <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scserver.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>

#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>

/*
** Name:	scddata.c
**
** Description:	Global data for scd facility.
**
** History:
**	23-sep-96 (canor01)
**	    Created.
**	14-May-1998 (jenjo02)
**	    Removed obsolete sc_writebehind.
**      16-Oct-98 (fanra01)
**          Add server name as class member.
**      14-Jan-1999 (fanra01)
**          Replaced previously deleted entry for sc_writebehind.  This field
**          is no longer used in 2.5 and is maintained for backward
**          compatibility.  This object will now always return a value of 0.
**	23-aug-1999 (somsa01)
**	    Added scd_start_sampler_set and scd_stop_sampler_set to start and
**	    stop the Sampler thread, respectively.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Added exp.scf.scd.rule_upd_prefetch.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPSCFLIBDATA
**	    in the Jamfile.
**	18-Oct-2004 (shaha03)
**	    SIR 112918, Added configurable default cursor open mode support.
**	24-Oct-2005 (toumi01) BUG 115449
**	    Don't hold MO mutex for scd_shut_set so that we don't deadlock
**	    on ourselves.
**	30-aug-06 (thaju02)
**	    Add exp.scf.scd.rule_del_prefetch. (B116355)
**	01-may-2008 (smeke01) b118780
**	    Corrected name of sc_avgrows to sc_totrows.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added MustLog_DB_Lst to hold Must Log DB list. 
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
**      11-Aug-2010 (hanal04) Bug 124180
**          Added money_compat for backwards compatibility of money
**          string constants.
*/

/*
** data from scdinit.c
*/

GLOBALDEF i4        Sc_server_type; /* for handing to scd_initiate */
GLOBALDEF PTR            Sc_server_name = NULL;
GLOBALDEF SCS_IOMASTER   IOmaster;       /* Anchor for list of databases */
GLOBALDEF SCS_DBLIST     MustLog_DB_Lst; /* Anchor for list of databases */


/*
** data from scdmo.c
*/

MO_GET_METHOD scd_state_get;
MO_GET_METHOD scd_listen_get;
MO_GET_METHOD scd_shutdown_get;
MO_GET_METHOD scd_capabilities_get;
MO_GET_METHOD scd_pid_get;
MO_GET_METHOD scd_server_name_get;

MO_SET_METHOD scd_crash_set;
MO_SET_METHOD scd_close_set;
MO_SET_METHOD scd_open_set;
MO_SET_METHOD scd_shut_set;
MO_SET_METHOD scd_stop_set;
MO_SET_METHOD scd_start_sampler_set;
MO_SET_METHOD scd_stop_sampler_set;

# define SC_MAIN_CB_ADDR        NULL

GLOBALDEF MO_CLASS_DEF Scd_classes[] =
{
    /* all actually attached */

    /* ---------------------------------------------------------------- */

  /* READ/WRITE */

  { 0, "exp.scf.scd.server.max_connections",
	sizeof(Sc_main_cb->sc_max_conns),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_max_conns), MOintget, MOintset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.reserved_connections",
	sizeof(Sc_main_cb->sc_rsrvd_conns),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_rsrvd_conns), MOintget, MOintset,
	SC_MAIN_CB_ADDR, MOcdata_index },

    /* ---------------------------------------------------------------- */

  /* WRITE ONLY */

  { 0, "exp.scf.scd.server.control.crash", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_crash_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.control.close", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_close_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.control.open", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_open_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.control.shut", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_shut_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.control.start_sampler",
	sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_start_sampler_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { MO_NO_MUTEX, "exp.scf.scd.server.control.stop",
	sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_stop_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.control.stop_sampler",
	sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ|MO_SERVER_WRITE|MO_SYSTEM_WRITE,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask),
	MOzeroget, scd_stop_sampler_set,
	SC_MAIN_CB_ADDR, MOcdata_index },

    /* ---------------------------------------------------------------- */

  /* READ ONLY */

  { 0, "exp.scf.scd.server.pid",
	0,
	MO_READ,  0,
	0, scd_pid_get, MOnoset,
	0, MOcdata_index },

  { 0, "exp.scf.scd.start.name",
	0,
	MO_READ,  0,
	0, scd_server_name_get, MOnoset,
	0, MOcdata_index },

  { 0, "exp.scf.scd.server.current_connections",
	sizeof(Sc_main_cb->sc_current_conns),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_current_conns), MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.highwater_connections",
	sizeof(Sc_main_cb->sc_current_conns),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_highwater_conns), MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.listen_mask", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask), MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.listen_state", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask), scd_listen_get, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server.shutdown_state", sizeof(Sc_main_cb->sc_listen_mask),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_listen_mask), scd_shutdown_get, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.state_num", sizeof(Sc_main_cb->sc_state),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_state),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.state_str", sizeof(Sc_main_cb->sc_state),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_state),
	scd_state_get, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.capabilities_num", sizeof(Sc_main_cb->sc_capabilities),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_capabilities),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },
  { 0, "exp.scf.scd.capabilities_str", sizeof(Sc_main_cb->sc_capabilities),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_capabilities),
	scd_capabilities_get, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.irowcount", sizeof(Sc_main_cb->sc_irowcount),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_irowcount),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.totrows", sizeof(Sc_main_cb->sc_totrows),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_totrows),
	MOuintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.selcnt", sizeof(Sc_main_cb->sc_selcnt),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_selcnt),
	MOuintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.flatflags", sizeof(Sc_main_cb->sc_flatflags),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_flatflags),
	MOuintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.csrflags", sizeof(Sc_main_cb->sc_csrflags),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_csrflags),
	MOuintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.defcsrflag", sizeof(Sc_main_cb->sc_defcsrflag),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_defcsrflag),
	MOuintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.psf_mem", sizeof(Sc_main_cb->sc_psf_mem),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_psf_mem),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.acc", sizeof(Sc_main_cb->sc_acc),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_acc),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.money_compat", sizeof(Sc_main_cb->sc_money_compat),
	MO_READ,  0,
        CL_OFFSETOF( SC_MAIN_CB, sc_money_compat),
        MOintget, MOnoset,
        SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.rule_depth", sizeof(Sc_main_cb->sc_rule_depth),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_rule_depth),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.rule_del_prefetch",sizeof(Sc_main_cb->sc_rule_del_prefetch_off),
        MO_READ,  0,
        CL_OFFSETOF( SC_MAIN_CB, sc_rule_del_prefetch_off),
        MOintget, MOnoset,
        SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.rule_upd_prefetch",sizeof(Sc_main_cb->sc_rule_upd_prefetch_off),
        MO_READ,  0,
        CL_OFFSETOF( SC_MAIN_CB, sc_rule_upd_prefetch_off),
        MOintget, MOnoset,
        SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.soleserver", sizeof(Sc_main_cb->sc_soleserver),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_soleserver),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.fastcommit", sizeof(Sc_main_cb->sc_fastcommit),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_fastcommit),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.names_reg", sizeof(Sc_main_cb->sc_names_reg),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_names_reg),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.writebehind", 0,
	MO_READ,  0,
	0,
	MOzeroget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.nousers", sizeof(Sc_main_cb->sc_nousers),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_nousers),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.min_priority", sizeof(Sc_main_cb->sc_min_priority),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_min_priority),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.max_priority", sizeof(Sc_main_cb->sc_max_priority),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_max_priority),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.norm_priority", sizeof(Sc_main_cb->sc_norm_priority),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_norm_priority),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.gclisten_fail", sizeof(Sc_main_cb->sc_gclisten_fail),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_gclisten_fail),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.risk_inconsistency",
	sizeof(Sc_main_cb->sc_risk_inconsistency),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_risk_inconsistency),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.no_star_cluster", sizeof(Sc_main_cb->sc_no_star_cluster),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_no_star_cluster),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.nostar_recovr", sizeof(Sc_main_cb->sc_nostar_recovr),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_nostar_recovr),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.server_name", sizeof(Sc_main_cb->sc_sname),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_sname),
	MOstrget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0, "exp.scf.scd.startup_time", sizeof(Sc_main_cb->sc_startup_time),
	MO_READ,  0,
	CL_OFFSETOF( SC_MAIN_CB, sc_startup_time),
	MOintget, MOnoset,
	SC_MAIN_CB_ADDR, MOcdata_index },

  { 0 }
};
