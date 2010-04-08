/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <er.h>
#include    <ex.h>
#include    <cv.h>
#include    <cs.h>
#include    <me.h>
#include    <sp.h>
#include    <mo.h>
#include    <st.h>
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
#include    "scsint.h"

/*
** Name:	scsdata.c
**
** Description:	Global data for scs facility.
**
** History:
**	23-sep-96 (canor01)
**	    Created.
**      06-dec-96 (cohmi01)
**        Ch. "exp.scf.scs.scb_index" to pass back the 'thread id' (cs_self)
**        rather than addr of SCB, so it matches thread id's from MOs such as
**        lock and logging objects when using native threads. Bug 79557
**	7-aug-97 (inkdo01)
**	    Removed above change becuase tampering with the index class makes
**	    all other SCB classes inaccessible by index. Instead added new
**	    "self_id" class which is displayable session ID. This will allow 
**	    joining to session ID classes from other MOs (which was the 
**	    original problem).
**	11-feb-1998 (hanch04)
**	    changed du_dbcmptlvl from 8.0 to 8.5
**      16-Feb-98 (fanra01)
**          Modified to pass 'thread id'.  This change is in conjunction
**          with modifying the index method of SCBs.
**          Add a scb ptr entry in the class definition.  Allows return of
**          scb pointer from the pointer instance.
**      11-Mar-98 (fanra01)
**          Modified to remove MO_SESS_WRITE from
**          exp.scf.scs.scb_remove_session and exp.scf.scs.scb_crash_session.
**          Raises required privilege to MO_SERVER_WRITE.
**      02-Mar-2001 (hanch04)
**          Bumped release id to external "II 2.6" internal 860 or 8.6
**	24-oct-2001 (stephenb)
**	    Add definition of MO object exp.scf.scs.scb_lastquery
**      07-Apr-2003 (hanch04)
**          Bumped release id to external "II 2.65" internal 865 or 8.65
**      13-Jan-2004 (sheco02)
**          Bumped release id to external "II 3.0" internal 900 or 9.0
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPSCFLIBDATA
**	    in the Jamfile.
**	6-Jan-2005 (schka24)
**	    Use DBMS level macro from dudbms.qsh so that we don't have to
**	    keep editing this file every time the config version changes.
**      04-nov-2008 (stial01)
**          Moved static initialization of dbdb_dbtuple to scd_initiate()
*/

/*
**
LIBRARY = IMPSCFLIBDATA
**
*/

/*
** data from scsinit.c
*/

GLOBALDEF DU_DATABASE	      dbdb_dbtuple;

/*
** data from scsmo.c
*/

MO_GET_METHOD scs_self_get;
MO_GET_METHOD scs_pid_get;
MO_GET_METHOD scs_dblockmode_get;
MO_GET_METHOD scs_facility_name_get;
MO_GET_METHOD scs_act_detail_get;
MO_GET_METHOD scs_activity_get;
MO_GET_METHOD scs_description_get;
MO_GET_METHOD scs_query_get;
MO_GET_METHOD scs_lquery_get;
MO_GET_METHOD scs_assoc_get;

MO_SET_METHOD scs_remove_sess_set;
MO_SET_METHOD scs_crash_sess_set;

static char index_name[] = "exp.scf.scs.scb_index";
static char index_ptr [] = "exp.scf.scs.scb_ptr";

GLOBALDEF MO_CLASS_DEF Scs_classes[] =
{
    /* actually attached */

  { MO_INDEX_CLASSID|MO_CDATA_INDEX, index_name, 0, MO_READ,
	0, 0, scs_self_get, MOnoset, 0, MOidata_index },

  { MO_INDEX_CLASSID|MO_CDATA_INDEX, index_ptr, 0, MO_READ,
	0, 0, MOpvget, MOnoset, 0, MOidata_index },

  /* special methods, indexed from index_name. */

  { MO_CDATA_INDEX, "exp.scf.scs.scb_self",
	0, MO_READ, index_name,
	0, scs_self_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_pid",
	0, MO_READ, index_name,
	0, scs_pid_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_remove_session",
	0, MO_READ |
	    MO_SERVER_WRITE|MO_SYSTEM_WRITE|MO_SECURITY_WRITE,
	index_name,
	0, MOzeroget, scs_remove_sess_set,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_crash_session",
	0, MO_READ |
	    MO_SERVER_WRITE|MO_SYSTEM_WRITE|MO_SECURITY_WRITE,
	index_name,
	0, MOzeroget, scs_crash_sess_set,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_dblockmode",
	0, MO_READ, index_name,
	0, scs_dblockmode_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_facility_name",
	0, MO_READ, index_name,
	0, scs_facility_name_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_activity",
	0, MO_READ, index_name,
	0, scs_activity_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_act_detail",
	0, MO_READ, index_name,
	0, scs_act_detail_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_description",
	0, MO_SECURITY_READ, index_name,
	0, scs_description_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_query",
	0, MO_SECURITY_READ, index_name,
	0, scs_query_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_lastquery",
	0, MO_SECURITY_READ, index_name,
	0, scs_lquery_get, MOnoset,
	0, MOidata_index },

  /* generic methods, all indexed from index_name */

  { MO_CDATA_INDEX, "exp.scf.scs.scb_gca_assoc_id",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_cscb.cscb_assoc_id ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_cscb.cscb_assoc_id ),
	scs_assoc_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_database",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_dbname),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_dbname),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_dbowner",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_dbowner),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_dbowner),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_euser",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_susername ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_susername ),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_ruser",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_rusername ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_rusername ),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_terminal",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_terminal),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_terminal ),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_s_name",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_iusername ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_iusername ),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_group",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_egrpid ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_egrpid ),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_role",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_eaplid ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_eaplid ),
	MOstrget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_appl_code",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_appl_code ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_appl_code ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_client_info",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_gw_parms ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_gw_parms ),
	MOstrpget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_client_user",
	0, MO_READ, index_name, 0,
	scs_a_user_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_client_host",
	0, MO_READ, index_name, 0,
	scs_a_host_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_client_tty",
	0, MO_READ, index_name, 0,
	scs_a_tty_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_client_pid",
	0, MO_READ, index_name, 0,
	scs_a_pid_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_client_connect",
	0, MO_READ, index_name, 0,
	scs_a_conn_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_facility_index",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_cfac ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_cfac ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_priority",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_cur_priority ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_cur_priority ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_priority_limit",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_priority_limit ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_priority_limit ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_idle_limit",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_idle_limit ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_cur_idle_limit ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_connect_limit",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_cur_connect_limit ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_connect_limit ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_initial_connect_limit",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_connect_limit ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_connect_limit ),
	MOintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.scf.scs.scb_initial_idle_limit",
	MO_SIZEOF_MEMBER( SCD_SCB, scb_sscb.sscb_ics.ics_idle_limit ),
	MO_READ, index_name,
	CL_OFFSETOF( SCD_SCB, scb_sscb.sscb_ics.ics_idle_limit ),
	MOintget, MOnoset, 0, MOidata_index },

  { 0 }
};
