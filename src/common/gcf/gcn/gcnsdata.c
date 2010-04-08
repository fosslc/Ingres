/*
** Copyright (c) 1996, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <sl.h>
#include <iicommon.h>
#include <qu.h>
#include <st.h>
#include <si.h>
#include <lo.h>
#include <cm.h>
#include <me.h>
#include <tm.h>
#include <gca.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcm.h>
#include <er.h>
#include <ex.h>
#include <pm.h>
#include <tr.h>

#ifndef GCF64
#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#endif /* !GCF64 */

/*
** Name:	gcndata.c
**
** Description:	Global data for GCN facility.
**
**  History:
**      12-Dec-95 (fanra01)
**          Created.
**	16-jul-96 (emmag)
**	    Updated for "new GCA".
**	27-sep-1996 (canor01)
**	    Updated again.
**	19-Mar-97 (gordy)
**	    Removed communications buffers.
**	17-Feb-98 (gordy)
**	    Expanded MIB entries to support GCM instrumentation.
**	27-Feb-98 (gordy)
**	    Added MIB entry for remote auth mechanism.
**	 4-Jun-98 (gordy)
**	    Added MIB entry for timeout.  Moved bedcheck queue from
**	    global to association thread.
**	10-Jun-98 (gordy)
**	    Added MIB entry for file compression point.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**     6-Apr-04 (gordy)
**        Changed MO access to ANY for registry and bedcheck objects.
**	 3-Aug-09 (gordy)
**	    Remove string length restrictions.
*/

/*
**  Data from gcnsinit.c
*/

GLOBALDEF	GCN_STATIC	IIGCn_static ZERO_FILL;


/*
**  Data from gcnsfile.c
*/
FUNC_EXTERN STATUS gcn_getflag();

/*
**	Name Server MIB Class definitions
*/	

GLOBALDEF MO_CLASS_DEF	gcn_classes[] =
{
    {
	0,
	GCN_MIB_CACHE_MODIFIED,
	MO_SIZEOF_MEMBER( GCN_STATIC, cache_modified.TM_secs ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, cache_modified.TM_secs ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_BEDCHECK_INTERVAL,
	MO_SIZEOF_MEMBER( GCN_STATIC, bedcheck_interval ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, bedcheck_interval ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_COMPRESS_POINT,
	MO_SIZEOF_MEMBER( GCN_STATIC, compress_point ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, compress_point ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_DEFAULT_CLASS,
	MO_SIZEOF_MEMBER( GCN_STATIC, svr_type ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, svr_type ),
	MOstrpget,
	MOstrset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_EXPIRE_INTERVAL,
	MO_SIZEOF_MEMBER( GCN_STATIC, ticket_exp_interval ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, ticket_exp_interval ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_HOSTNAME,
	MO_SIZEOF_MEMBER( GCN_STATIC, hostname ),
	MO_ANY_READ,			/* Needed for registry browsing */
	0,
	CL_OFFSETOF( GCN_STATIC, hostname ),
	MOstrpget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_INSTALLATION_ID,
	MO_SIZEOF_MEMBER( GCN_STATIC, install_id ),
	MO_ANY_READ,			/* Needed for registry browsing */
	0,
	CL_OFFSETOF( GCN_STATIC, install_id ),
	MOstrpget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_LOCAL_VNODE,
	MO_SIZEOF_MEMBER( GCN_STATIC, lcl_vnode ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, lcl_vnode ),
	MOstrpget,
	MOstrset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_REMOTE_VNODE,
	MO_SIZEOF_MEMBER( GCN_STATIC, rmt_vnode ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, rmt_vnode ),
	MOstrpget,
	MOstrset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_REMOTE_MECHANISM,
	MO_SIZEOF_MEMBER( GCN_STATIC, rmt_mech ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, rmt_mech ),
	MOstrpget,
	MOstrset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_LTCKT_CACHE_MISS,
	MO_SIZEOF_MEMBER( GCN_STATIC, ltckt_cache_miss ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, ltckt_cache_miss ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_LTCKT_CREATED,
	MO_SIZEOF_MEMBER( GCN_STATIC, ltckt_created ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, ltckt_created ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_LTCKT_EXPIRED,
	MO_SIZEOF_MEMBER( GCN_STATIC, ltckt_expired ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, ltckt_expired ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_LTCKT_USED,
	MO_SIZEOF_MEMBER( GCN_STATIC, ltckt_used ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, ltckt_used ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_RTCKT_CACHE_MISS,
	MO_SIZEOF_MEMBER( GCN_STATIC, rtckt_cache_miss ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, rtckt_cache_miss ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_RTCKT_CREATED,
	MO_SIZEOF_MEMBER( GCN_STATIC, rtckt_created ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, rtckt_created ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_RTCKT_EXPIRED,
	MO_SIZEOF_MEMBER( GCN_STATIC, rtckt_expired ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, rtckt_expired ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_RTCKT_USED,
	MO_SIZEOF_MEMBER( GCN_STATIC, rtckt_used ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_STATIC, rtckt_used ),
	MOintget,
	MOnoset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_TICKET_CACHE_SIZE,
	MO_SIZEOF_MEMBER( GCN_STATIC, ticket_cache_size ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, ticket_cache_size ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_TICKET_EXPIRE,
	MO_SIZEOF_MEMBER( GCN_STATIC, ticket_exp_time ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, ticket_exp_time ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_TIMEOUT,
	MO_SIZEOF_MEMBER( GCN_STATIC, timeout ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, timeout ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	0,
	GCN_MIB_TRACE_LEVEL,
	MO_SIZEOF_MEMBER( GCN_STATIC, trace ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCN_STATIC, trace ),
	MOintget,
	MOintset,
	(PTR)&IIGCn_static,
	MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCN_MIB_SERVER_ENTRY,
	MO_SIZEOF_MEMBER( GCN_TUP_QUE, gcn_instance ),
	MO_READ,
	0,
	CL_OFFSETOF( GCN_TUP_QUE, gcn_instance ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCN_MIB_SERVER_ADDRESS,
	MO_SIZEOF_MEMBER( GCN_TUP_QUE, gcn_tup.val ),
	MO_ANY_READ,			/* Needed for registry browsing */
	GCN_MIB_SERVER_ENTRY,
	CL_OFFSETOF( GCN_TUP_QUE, gcn_tup.val ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCN_MIB_SERVER_CLASS,
	MO_SIZEOF_MEMBER( GCN_TUP_QUE, gcn_type ),
	MO_ANY_READ,			/* Needed for registry browsing */
	GCN_MIB_SERVER_ENTRY,
	CL_OFFSETOF( GCN_TUP_QUE, gcn_type ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCN_MIB_SERVER_FLAGS,
	MO_SIZEOF_MEMBER( GCN_TUP_QUE, gcn_tup.uid ),
	MO_READ,
	GCN_MIB_SERVER_ENTRY,
	CL_OFFSETOF( GCN_TUP_QUE, gcn_tup.uid ),
	gcn_getflag,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCN_MIB_SERVER_OBJECT,
	MO_SIZEOF_MEMBER( GCN_TUP_QUE, gcn_tup.obj ),
	MO_ANY_READ,			/* Needed for registry browsing */
	GCN_MIB_SERVER_ENTRY,
	CL_OFFSETOF( GCN_TUP_QUE, gcn_tup.obj ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    { 0 }
};

