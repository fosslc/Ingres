/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <pc.h>
#include    <cs.h>
#include    <tr.h>
#include    <cm.h>
#include    <er.h>
#include    <si.h>
#include    <lo.h>
#include    <di.h>
#include    <nm.h>
#include    <me.h>
#include    <ex.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <lgkparms.h>
#include    <lgdstat.h>
#include    <pm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dm0llctx.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dmve.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmfrcp.h>
#include    <dmftrace.h>
#include    <lgclustr.h>

/*
** Name:	dmfdata.c
**
** Description:	Global data for dmf facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**  05-Oct-98 (merja01)
**      Removed global definition of file_name.  This conflicts with the
**      file_name function used in the axp_osf libmld.  Furthermore,
**      this global definition does not appear to be used anywhere
**      as all references to file_name are local.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      15-jun-2009 (stial01)
**          Changes for Redo/undo error handling
*/

/* dmfinit.c */
GLOBALDEF DMP_DCB *dmf_jsp_dcb = 0;	/* Current DCB  */

/* dmfrcp.c */
GLOBALDEF   LG_LXID		rcp_lx_id;
GLOBALDEF   LG_LGID		rcp_lg_id;
GLOBALDEF   LG_DBID		rcp_db_id;
GLOBALDEF   DB_TRAN_ID		rcp_tran_id;
GLOBALDEF   i4		rcp_node_id;
GLOBALDEF   LG_LA		bcp_la;
GLOBALDEF   LG_LSN		bcp_lsn;
GLOBALDEF   char	Db_stop_on_incons_db;
GLOBALDEF   i4		Db_startup_error_action;
GLOBALDEF   i4		Db_pass_abort_error_action;
GLOBALDEF   i4		Db_recovery_error_action;

/* dmfrfp.c */
GLOBALDEF DMP_DCB *dmf_rfp_dcb = 0;     /* Current DCB  */

