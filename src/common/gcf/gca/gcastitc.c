/*
** Copyright (c) 1987, 2004 Ingres Corporation All Rights Reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <qu.h>
#include    <cs.h>
#include    <mu.h>
#include    <mo.h>
#include    <gca.h>
#include    <gc.h>
#include    <gcaint.h>
#include    <gcm.h>

/**
**
**  Name: GCASTATIC.C - Static storage for GCA
**
**  Description:
**      This file declares all static storage required by GCA.
**
**  History:
**      14-Apr-1987 (berrow)
**          Created.
**      18-Jun-1987 (berrow)
**          Added Data Object descriptor arrays.
**      14-Jul-1987 (berrow)
**          Updated Data Object descriptor arrays.
**      04-Aug-87 (jbowers)
**          Changed type of gca_q_atbl from QUEUE to PROTECTED_Q.
**          Added inclusion for gcainternal.h to include PROTECTED_Q definition.
**      01-Sep-1987 (berrow)
**	    Replaced all static data by a single pointer to a structure
**	    containing the globally used data elements (GCA storage not
**	    specific to a single association).  This is now allocated at
**	    run time (by the GCA_REGISTER or GCA_INITIATE services).
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	16-Aug-91 (seiwald)
**	    Added IIGCa_version string for identifying releases.
**	6-Apr-92 (seiwald)
**	    6.4/02 GCF.
**	31-Oct-92 (seiwald)
**	    6.4/03 GCF.
**	10-Dec-92 (seiwald)
**	    6.5/beta GCF.
**      14-jul-93 (ed)
**          unnested dbms.h
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  In GCA, change from 
**	    using handed in semaphore routines to MU semaphores; these
**	    can be initialized, named, and removed.  The others can't
**	    without expanding the GCA interface, which is painful and
**	    practically impossible.
**	20-Nov-95 (gordy)
**	    Oping 1.2
**	 3-Sep-96 (gordy)
**	    Restructured GCA global data.
**	30-sep-1996 (canor01)
**	    Move GCA MIB class definitions here from gcainit.c, since
**	    they need the address of IIGCa_global in initialization, which
**	    is not acceptable to the Windows NT compiler unless IIGCa_global
**	    is defined in the same file.
**	29-May-97 (gordy)
**	    Minor changes in PEER info.
**	17-Feb-98 (gordy)
**	    Permit tracing to be managed through GCM.
**	28-Jul-98 (gordy)
**	    Added installation ID for server bedchecks.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**	 6-Jun-03 (gordy)
**	    Enhanced MIB information.
**	 6-Apr-04 (gordy)
**	    Changed MO access to ANY for registry and bedcheck objects.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**/

/* Jam hints
**
LIBRARY = IMPGCFLIBDATA
**
*/


/*
** Definition of all global variables owned by this file.
*/

/*
** Name: IIGCa_global
**
** Description:
**	GCA global data independent of GCA_INITIATE requests.
*/
GLOBALDEF  GCA_GLOBAL        IIGCa_global ZERO_FILL;

/*
** name: IIGCa_version
**
** Description:
**	Version string to be included in libraries/executables.
*/
static char IIGCa_version[] = "GCF 2.0 (03-Sep-96)";


/*
**	GCA MIB Class definitions
*/	

GLOBALDEF MO_CLASS_DEF	gca_classes[] =
{
    {
	0,
	GCA_MIB_NO_ASSOCS,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_acb_active ),
	MO_ANY_READ,			/* Needed for GCN bedcheck */
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_acb_active ),
	MOintget,
	MOnoset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_DATA_IN,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_data_in ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_data_in ),
	MOintget,
	MOnoset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_DATA_OUT,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_data_out ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_data_out ),
	MOintget,
	MOnoset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_INSTALL_ID,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_install_id ),
	MO_ANY_READ,			/* Needed for GCN bedcheck */
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_install_id ),
	MOstrget,
	MOnoset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_MSGS_IN,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_msgs_in ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_msgs_in ),
	MOintget,
	MOnoset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_MSGS_OUT,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_msgs_out ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_msgs_out ),
	MOintget,
	MOnoset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_TRACE_LEVEL,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_trace_level ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_trace_level ),
	MOintget,
	MOintset,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	0,
	GCA_MIB_TRACE_LOG,
	MO_SIZEOF_MEMBER( GCA_GLOBAL, gca_trace_log ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCA_GLOBAL, gca_trace_log ),
	MOstrget,
	gca_trace_log,
	(PTR)&IIGCa_global,
	MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCA_MIB_CLIENT,
	MO_SIZEOF_MEMBER( GCA_CB, gca_cb_id ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_CB, gca_cb_id ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_CLIENT_FLAGS,
	MO_SIZEOF_MEMBER( GCA_CB, gca_flags ),
	MO_ANY_READ,			/* Needed for GCN check */
	GCA_MIB_CLIENT,
	CL_OFFSETOF( GCA_CB, gca_flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_CLIENT_LOCAL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCA_CB, gca_local_protocol ),
	MO_READ,
	GCA_MIB_CLIENT,
	CL_OFFSETOF( GCA_CB, gca_local_protocol ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_CLIENT_API_VERSION,
	MO_SIZEOF_MEMBER( GCA_CB, gca_api_version ),
	MO_READ,
	GCA_MIB_CLIENT,
	CL_OFFSETOF( GCA_CB, gca_api_version ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_CLIENT_SRV_CLASS,
	MO_SIZEOF_MEMBER( GCA_CB, gca_srvr_class ),
	MO_ANY_READ,			/* Needed for GCN bedcheck */
	GCA_MIB_CLIENT,
	CL_OFFSETOF( GCA_CB, gca_srvr_class ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_CLIENT_LISTEN_ADDRESS,
	MO_SIZEOF_MEMBER( GCA_CB, gca_listen_address ),
	MO_ANY_READ,			/* Needed for registry bedcheck */
	GCA_MIB_CLIENT,
	CL_OFFSETOF( GCA_CB, gca_listen_address ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCA_MIB_ASSOCIATION,
	MO_SIZEOF_MEMBER( GCA_ACB, assoc_id ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_ACB, assoc_id ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_CLIENT_ID,
	MO_SIZEOF_MEMBER( GCA_ACB, client_id ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, client_id ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_FLAGS,
	MO_SIZEOF_MEMBER( GCA_ACB, gca_flags ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, gca_flags ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOCIATION,
	MO_SIZEOF_MEMBER( GCA_ACB, assoc_id ),
	MO_READ,
	0,
	CL_OFFSETOF( GCA_ACB, assoc_id ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_INBOUND,
	MO_SIZEOF_MEMBER( GCA_ACB, inbound ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, inbound ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_PARTNER_ID,
	MO_SIZEOF_MEMBER( GCA_ACB, partner_id ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, partner_id ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_PARTNER_PROTOCOL,
	MO_SIZEOF_MEMBER( GCA_ACB, req_protocol ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, req_protocol ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_SESSION_PROTOCOL,
	MO_SIZEOF_MEMBER( GCA_ACB, gca_protocol ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, gca_protocol ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCA_MIB_ASSOC_USERID,
	MO_SIZEOF_MEMBER( GCA_ACB, user_id ),
	MO_READ,
	GCA_MIB_ASSOCIATION,
	CL_OFFSETOF( GCA_ACB, user_id ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    { 0 }
};

