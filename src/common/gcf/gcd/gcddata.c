/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gcu.h>
#include    <gcdint.h>
#include    <gcdapi.h>
#include    <gcdmsg.h>
#include    <gcdint.h>
#include    <sp.h>
#include    <mo.h>
#include    <gl.h>
#include    <gcm.h>

/**
**
**  Name: GCDGLBL.C - Global storage for GCD
**
**  Description:
**      This file declares all global storage required by GCD.
**
**  History:
**      13-Mar-2003 (wansh01)
**          Initial module creation.
**	17-jul-2003 (wansh01)
**          Added gcd_classes[].  		
**	08-Aug-2003 (devjo01)
**	    Add gcd_class_cnt;
**	 1-Oct-03 (gordy)
**	    Rename DONE message to RESULT.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Removed LIBRARY jam hint.  This file is now included in the
**          GCC local library only.
**	 5-Dec-07 (gordy)
**	    Minor changes to client tracking values.
**	25-Mar-10 (gordy)
**	    Added batch message.
**/

/*
** Pointer to GCD global data structure
*/


GLOBALDEF GCD_GLOBAL	GCD_global ZERO_FILL;


GLOBALDEF GCULIST msgMap[] =
{
    { MSG_CONNECT,	"CONNECT" },
    { MSG_DISCONN,	"DISCONNECT" },
    { MSG_XACT,		"XACT" },
    { MSG_QUERY,	"QUERY" },
    { MSG_CURSOR,	"CURSOR" },
    { MSG_DESC,		"DESC" },
    { MSG_DATA,		"DATA" },
    { MSG_ERROR,	"ERROR" },
    { MSG_RESULT,	"RESULT" },
    { MSG_REQUEST,	"REQUEST" },
    { MSG_BATCH,	"BATCH" },
    { 0, NULL }
};

/*
** Name: gcd_classes
**
** Description:
**	MO definitions for GCD MIB objects.
*/
GLOBALDEF MO_CLASS_DEF	gcd_classes[] =
{
    {
	0,
	GCD_MIB_CONN_COUNT,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, client_cnt ),
	MO_READ,
	0,
	CL_OFFSETOF( GCD_GLOBAL, client_cnt ),
	MOintget,
	MOnoset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
	0,
	GCD_MIB_DBMS_COUNT,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, cib_active ),
	MO_READ,
	0,
	CL_OFFSETOF( GCD_GLOBAL, cib_active ),
	MOintget,
	MOnoset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
	0,
	GCD_MIB_CONN_MAX,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, client_max ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCD_GLOBAL, client_max ),
	MOintget,
	MOintset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
	0,
	GCD_MIB_POOL_COUNT,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, pool_cnt ),
	MO_READ,
	0,
	CL_OFFSETOF( GCD_GLOBAL, pool_cnt ),
	MOintget,
	MOnoset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
	0,
	GCD_MIB_POOL_MAX,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, pool_max ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCD_GLOBAL, pool_max ),
	MOintget,
	MOintset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
	0,
	GCD_MIB_POOL_TIMEOUT,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, pool_idle_limit ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCD_GLOBAL, pool_idle_limit ),
	MOintget,
	MOintset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
	0,
	GCD_MIB_CLIENT_TIMEOUT,
	MO_SIZEOF_MEMBER( GCD_GLOBAL, client_idle_limit ),
	MO_READ | MO_WRITE,
	0,
	CL_OFFSETOF( GCD_GLOBAL, client_idle_limit ),
	MOintget,
	MOintset,
	(PTR)&GCD_global,
	MOcdata_index
    },
    {
        0,
        GCD_MIB_TRACE_LEVEL,
       	MO_SIZEOF_MEMBER( GCD_GLOBAL, gcd_trace_level),
        MO_READ | MO_WRITE,
        0,
       	CL_OFFSETOF( GCD_GLOBAL, gcd_trace_level),
        MOintget,
        MOintset,
        (PTR)&GCD_global,
        MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCD_MIB_PROTOCOL,
	MO_SIZEOF_MEMBER( GCD_PIB, pid ),
	MO_READ,
	0,
	CL_OFFSETOF( GCD_PIB, pid ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_PROTO_HOST,
	MO_SIZEOF_MEMBER( GCD_PIB, host ),
	MO_READ,
	GCD_MIB_PROTOCOL,
	CL_OFFSETOF( GCD_PIB, host ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_PROTO_ADDR,
	MO_SIZEOF_MEMBER( GCD_PIB, addr ),
	MO_READ,
	GCD_MIB_PROTOCOL,
	CL_OFFSETOF( GCD_PIB, addr ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_PROTO_PORT,
	MO_SIZEOF_MEMBER( GCD_PIB, port ),
	MO_READ,
	GCD_MIB_PROTOCOL,
	CL_OFFSETOF( GCD_PIB, port ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCD_MIB_CONNECTION,
	MO_SIZEOF_MEMBER( GCD_CCB, id ),
	MO_READ,
	0,
	CL_OFFSETOF( GCD_CCB, id ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_DBMS,
	MO_SIZEOF_MEMBER( GCD_CCB, cib ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, cib ),
	MOptrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_CLIENT_ADDR,
	MO_SIZEOF_MEMBER( GCD_CCB, client_addr ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, client_addr ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_CLIENT_HOST,
	MO_SIZEOF_MEMBER( GCD_CCB, client_host ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, client_host ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_CLIENT_USER,
	MO_SIZEOF_MEMBER( GCD_CCB, client_user ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, client_user ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_LCL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCD_CCB, gcc.lcl_addr.protocol ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, gcc.lcl_addr.protocol ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_LCL_NODE,
	MO_SIZEOF_MEMBER( GCD_CCB, gcc.lcl_addr.node_id ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, gcc.lcl_addr.node_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_LCL_PORT,
	MO_SIZEOF_MEMBER( GCD_CCB, gcc.lcl_addr.port_id ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, gcc.lcl_addr.port_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_RMT_PROTOCOL,
	MO_SIZEOF_MEMBER( GCD_CCB, gcc.rmt_addr.protocol ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, gcc.rmt_addr.protocol ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_RMT_NODE,
	MO_SIZEOF_MEMBER( GCD_CCB, gcc.rmt_addr.node_id ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, gcc.rmt_addr.node_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_RMT_PORT,
	MO_SIZEOF_MEMBER( GCD_CCB, gcc.rmt_addr.port_id ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, gcc.rmt_addr.port_id ),
	MOstrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_MSG_PROTOCOL,
	MO_SIZEOF_MEMBER( GCD_CCB, msg.proto_lvl ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, msg.proto_lvl ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_CONN_TL_PROTOCOL,
	MO_SIZEOF_MEMBER( GCD_CCB, tl.proto_lvl ),
	MO_READ,
	GCD_MIB_CONNECTION,
	CL_OFFSETOF( GCD_CCB, tl.proto_lvl ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	GCD_MIB_DBMS,
	sizeof( PTR ),
	MO_READ,
	0,
	0,
	MOpvget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_DBMS_API_LVL,
	MO_SIZEOF_MEMBER( GCD_CIB, level ),
	MO_READ,
	GCD_MIB_DBMS,
	CL_OFFSETOF( GCD_CIB, level ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_DBMS_API_HNDL,
	MO_SIZEOF_MEMBER( GCD_CIB, conn ),
	MO_READ,
	GCD_MIB_DBMS,
	CL_OFFSETOF( GCD_CIB, conn ),
	MOptrget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_DBMS_TARGET,
	MO_SIZEOF_MEMBER( GCD_CIB, database ),
	MO_READ,
	GCD_MIB_DBMS,
	CL_OFFSETOF( GCD_CIB, database ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	GCD_MIB_DBMS_USERID,
	MO_SIZEOF_MEMBER( GCD_CIB, username ),
	MO_READ,
	GCD_MIB_DBMS,
	CL_OFFSETOF( GCD_CIB, username ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    }
};

GLOBALDEF i4 gcd_class_cnt = sizeof(gcd_classes)/sizeof(gcd_classes[0]);

