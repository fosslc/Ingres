/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPGWFLIBDATA
**
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
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
# include "gwmplace.h"

/*
** Name:	gwmdata.c
**
** Description:	Global data for gwm facility.
**
** History:
**
**      19-Oct-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGWFLIBDATA
**	    in the Jamfile.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

/*
**  Data from gwmima.c
*/

GLOBALDEF   char    *GM_version = ERx("GWM_GW-1.0");

GLOBALDEF GM_GLOBALS GM_globals ZERO_FILL;

GLOBALDEF MO_CLASS_DEF GM_classes[] =
{
    { 0, "exp.gwf.gwm.glb.inits",
	  sizeof(GM_globals.gwm_inits), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_inits, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.terms",
	  sizeof(GM_globals.gwm_terms), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_terms, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.tabfs",
	  sizeof(GM_globals.gwm_tabfs), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_tabfs, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.idxfs",
	  sizeof(GM_globals.gwm_idxfs), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_idxfs, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.opens",
	  sizeof(GM_globals.gwm_opens), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_opens, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.closes",
	  sizeof(GM_globals.gwm_closes), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_closes, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.positions",
	  sizeof(GM_globals.gwm_positions), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_positions, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.gets",
	  sizeof(GM_globals.gwm_gets), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_gets, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.puts",
	  sizeof(GM_globals.gwm_puts), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_puts, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.replaces",
	  sizeof(GM_globals.gwm_replaces), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_replaces, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.deletes",
	  sizeof(GM_globals.gwm_deletes), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_deletes, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.begins",
	  sizeof(GM_globals.gwm_begins), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_begins, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.aborts",
	  sizeof(GM_globals.gwm_aborts), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_aborts, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.commits",
	  sizeof(GM_globals.gwm_commits), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_commits, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.infos",
	  sizeof(GM_globals.gwm_infos), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_infos, MOcdata_index },
    { 0, "exp.gwf.gwm.glb.errors",
	  sizeof(GM_globals.gwm_errors), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_errors, MOcdata_index },

    { 0, "exp.gwf.gwm.trace.inits",
	  sizeof(GM_globals.gwm_trace_inits), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_inits, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.terms",
	  sizeof(GM_globals.gwm_trace_terms), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_terms, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.tabfs",
	  sizeof(GM_globals.gwm_trace_tabfs), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_tabfs, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.idxfs",
	  sizeof(GM_globals.gwm_trace_idxfs), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_idxfs, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.opens",
	  sizeof(GM_globals.gwm_trace_opens), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_opens, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.closes",
	  sizeof(GM_globals.gwm_trace_closes), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_closes, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.positions",
	  sizeof(GM_globals.gwm_trace_positions), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_positions, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.gets",
	  sizeof(GM_globals.gwm_trace_gets), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_gets, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.puts",
	  sizeof(GM_globals.gwm_trace_puts), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_puts, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.replaces",
	  sizeof(GM_globals.gwm_trace_replaces), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_replaces, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.deletes",
	  sizeof(GM_globals.gwm_trace_deletes), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_deletes, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.begins",
	  sizeof(GM_globals.gwm_trace_begins), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_begins, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.aborts",
	  sizeof(GM_globals.gwm_trace_aborts), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_aborts, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.commits",
	  sizeof(GM_globals.gwm_trace_commits), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_commits, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.infos",
	  sizeof(GM_globals.gwm_trace_infos), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_infos, MOcdata_index },
    { 0, "exp.gwf.gwm.trace.errors",
	  sizeof(GM_globals.gwm_trace_errors), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_trace_errors, MOcdata_index },

    { 0 }
};

/*
**  Data from gwmpmib.c
*/

/* the mib for PLACES */    

MO_INDEX_METHOD GM_pindex;
MO_INDEX_METHOD GM_vindex;
MO_INDEX_METHOD GM_sindex;
MO_INDEX_METHOD GM_cindex;

MO_GET_METHOD GM_conn_get;
MO_GET_METHOD GM_cplace_get;
MO_GET_METHOD GM_srv_get;
MO_GET_METHOD GM_sflags_get;

MO_SET_METHOD GM_adddomset;
MO_SET_METHOD GM_deldomset;

static char place_index[] = "exp.gwf.gwm.places.index";
static char conn_index[] = "exp.gwf.gwm.connects.index";

GLOBALDEF MO_CLASS_DEF GM_pmib_classes[] =
{
    /* single value guys */

/* 0 */
    { 0, "exp.gwf.gwm.glb.sem.stat",
	  sizeof(CS_SEMAPHORE *), MO_READ, 0,
	  0, MOuivget, MOnoset,
	  (PTR)&GM_globals.gwm_stat_sem, MOcdata_index },

/* 1 */
    { 0, "exp.gwf.gwm.glb.sem.places",
	  sizeof(CS_SEMAPHORE *), MO_READ, 0, 0, MOuivget, MOnoset,
	  (PTR)&GM_globals.gwm_places_sem, MOcdata_index },

/* 2 */
    { 0, "exp.gwf.gwm.glb.def_vnode",
	  sizeof(GM_globals.gwm_def_vnode), MO_READ, 0,
	  0, MOstrget, MOnoset,
	  (PTR)&GM_globals.gwm_def_vnode, MOcdata_index },

/* 3 */
    { 0, "exp.gwf.gwm.glb.this_server",
	  sizeof(GM_globals.gwm_this_server), MO_READ, 0,
	  0, GM_srv_get, MOnoset,
	  0, MOcdata_index },

/* 3 */
    { 0, "exp.gwf.gwm.glb.def_domain",
	  sizeof(GM_globals.gwm_def_domain), MO_READ|MO_SERVER_WRITE, 0,
	  0, GM_srv_get, MOnoset,
	  0, MOcdata_index },

    /* gwm_time_to_live */

/* 4 */
    { 0, "exp.gwf.gwm.glb.time_to_live",
	  sizeof(GM_globals.gwm_time_to_live), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_time_to_live, MOcdata_index },


    { 0, "exp.gwf.gwm.glb.gcn_checks",
	  sizeof(GM_globals.gwm_gcn_checks), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_gcn_checks, MOcdata_index },


    { 0, "exp.gwf.gwm.glb.gcn_queries",
	  sizeof(GM_globals.gwm_gcn_queries), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_gcn_queries, MOcdata_index },

    /* gwm_conn_cnd */

/* 5 */
    { 0, "exp.gwf.gwm.glb.cnd.connections",
	  sizeof(CS_CONDITION *), MO_READ, 0,
	  0, MOuivget, MOnoset,
	  (PTR)&GM_globals.gwm_conn_cnd, MOcdata_index },

    /* gwm_connects */

/* 6 */
    { 0, "exp.gwf.gwm.glb.connects.active",
	  sizeof(GM_globals.gwm_connects), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_connects, MOcdata_index },

    /* gwm_max_conns */
/* 7 */
    { 0, "exp.gwf.gwm.glb.connects.max",
	  sizeof(GM_globals.gwm_max_conns), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_max_conns, MOcdata_index },

    /* gwm_max_place */

/* 8 */
    { 0, "exp.gwf.gwm.glb.connects.place.max",
	  sizeof(GM_globals.gwm_max_place), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_max_place, MOcdata_index },


/* 9 */
    { 0, "exp.gwf.gwm.glb.gcm.sends",
	  sizeof(GM_globals.gwm_gcm_sends), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_gcm_sends, MOcdata_index },

/* 10 */
    { 0, "exp.gwf.gwm.glb.gcm.reissues",
	  sizeof(GM_globals.gwm_gcm_reissues), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_gcm_reissues, MOcdata_index },

/* 11 */
    { 0, "exp.gwf.gwm.glb.gcm.errs",
	  sizeof(GM_globals.gwm_gcm_errs), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_gcm_errs, MOcdata_index },

/* FIXME 11 */
    { 0, "exp.gwf.gwm.glb.gcm.readahead",
	  sizeof(GM_globals.gwm_gcm_readahead), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_gcm_readahead, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.gcm.usecache",
	  sizeof(GM_globals.gwm_gcm_usecache), MO_READ|MO_SERVER_WRITE, 0,
	  0, MOintget, MOintset,
	  (PTR)&GM_globals.gwm_gcm_usecache, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.num.rop_calls",
	  sizeof(GM_globals.gwm_op_rop_calls), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_op_rop_calls, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.num.rop_hits",
	  sizeof(GM_globals.gwm_op_rop_hits), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_op_rop_hits, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.num.rop_misses",
	  sizeof(GM_globals.gwm_op_rop_misses), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_op_rop_misses, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.num.rreq_calls",
	  sizeof(GM_globals.gwm_rreq_calls), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_rreq_calls, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.num.rreq_hits",
	  sizeof(GM_globals.gwm_rreq_hits), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_rreq_hits, MOcdata_index },

    { 0, "exp.gwf.gwm.glb.num.rreq_misses",
	  sizeof(GM_globals.gwm_rreq_misses), MO_READ, 0,
	  0, MOintget, MOnoset,
	  (PTR)&GM_globals.gwm_rreq_misses, MOcdata_index },

/* ================================================================*/

/* place tree, including vnodes and servers */

/* 12 */
    { MO_INDEX_CLASSID|MO_CDATA_INDEX, place_index,
	  MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_blk.key), MO_READ, 0,
	  CL_OFFSETOF(GM_PLACE_BLK, place_blk.key), MOstrpget, MOnoset,
	  0, GM_pindex },

/* 13 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.places.sem",
	   sizeof(CS_SEMAPHORE *), MO_READ, place_index,
	   CL_OFFSETOF(GM_PLACE_BLK, place_sem), MOuivget, MOnoset,
	   0, GM_pindex },

/* 14 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.places.type",
	   MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_type),
	   MO_READ, place_index,
	   CL_OFFSETOF(GM_PLACE_BLK, place_type),
	   MOuintget, MOnoset,
	   0, GM_pindex },

    /* valid only when places.type indicates vnode. */

/* 15 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.vnode.expire",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_vnode.vnode_expire),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_vnode.vnode_expire),
       MOintget, MOnoset, 0, GM_pindex },

    /* always valid (for connection for vnode::/iinmsvr */

/* 16 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.cnd",
       sizeof(CS_CONDITION *),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_cnd),
       MOuivget, MOnoset, 0, GM_pindex },

/* 17 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.connects",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_connects),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_connects),
       MOintget, MOnoset, 0, GM_pindex },

/* 17 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.state",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_state),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_state),
       MOintget, MOnoset, 0, GM_pindex },

/* 18 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.valid",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_valid),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_valid),
       MOintget, MOnoset, 0, GM_pindex },

/* 19 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.class",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_class),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_class),
       MOstrget, MOnoset, 0, GM_pindex },

/* 20 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.flags",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_flags),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_flags),
       MOintget, MOnoset, 0, GM_pindex },

    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.flags_str",
       MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_flags),
       MO_READ, place_index,
       CL_OFFSETOF(GM_PLACE_BLK, place_srvr.srvr_flags),
       GM_sflags_get, MOnoset, 0, GM_pindex },

/* 21 */
    {  MO_CDATA_INDEX, "exp.gwf.gwm.servers.conn_tree",
	   MO_SIZEOF_MEMBER(GM_PLACE_BLK, place_srvr.srvr_conns),
	   MO_READ, place_index,
	   sizeof(SPTREE *),
	   MOuivget, MOnoset, 0, GM_pindex },

/* ---------------------------------------------------------------- */

# if 0

/* connection tree, with <server-instance>.<conn-key> as the instance */

/* 22 */
    { MO_INDEX_CLASSID|MO_CDATA_INDEX, conn_index,
	  0, MO_READ, 0,
	  0, GM_conn_get, MOnoset,
	  0, GM_cindex },

/* 23 */
    { MO_CDATA_INDEX, "exp.gwf.gwm.connects.sem",
	  sizeof(CS_SEMAPHORE *),
	  MO_READ, conn_index,
	  CL_OFFSETOF(GM_CONN_BLK, conn_sem),
	  MOuivget, MOnoset, 0, GM_cindex },
	  
/* 24 */
    { MO_CDATA_INDEX, "exp.gwf.gwm.connects.state",
	  MO_SIZEOF_MEMBER( GM_CONN_BLK, conn_state ),
	  MO_READ, conn_index,
	  CL_OFFSETOF(GM_CONN_BLK, conn_state),
	  MOintget, MOnoset, 0, GM_cindex },
	  
/* 25 */
    { MO_CDATA_INDEX, "exp.gwf.gwm.connects.place",
	  sizeof( GM_PLACE_BLK * ),
	  MO_READ, conn_index,
	  CL_OFFSETOF(GM_CONN_BLK, conn_place),
	  GM_cplace_get, MOnoset, 0, GM_cindex },
	  
/* 26 */
    { MO_CDATA_INDEX, "exp.gwf.gwm.connects.fs_parms.gca_status",
	  MO_SIZEOF_MEMBER( GM_CONN_BLK, conn_fs_parms.gca_status ),
	  MO_READ, conn_index,
	  CL_OFFSETOF(GM_CONN_BLK, conn_fs_parms.gca_status ),
	  MOintget, MOnoset, 0, GM_cindex },
	  
# endif

    { 0 }
} ;
