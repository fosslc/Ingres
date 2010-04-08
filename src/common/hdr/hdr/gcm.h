/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: GCM.H - Message definitions used by GCM
**
** Description:
**      This file contains definitions for all messages and data structures
**	used by GCM clients and servers.
**
** History: $Log-for RCS$
**      08-Jul-1992 (brucek)
**          Created.
**      11-Sep-1992 (brucek)
**          Added define for GCM_GCA_PROTOCOL_LEVEL.
**      22-Sep-1992 (brucek)
**          Use ANSI C function prototypes.
**      24-Sep-1992 (brucek)
**          Add GCM_TRAP_MASK macro to convert MO message types to bitflags.
**      01-Dec-1992 (brucek)
**          Move element_count out of GCM_MSG_HDR and make it separate
**	    field immediately in front of GCM_VAR_BINDING array.
**      12-Jan-1993 (brucek)
**          Remove definition of gcm_reg_trap (it's now in gcaint.h).
**	 2-Oct-98 (gordy)
**	    Consolidated GCF MIB class definitions in this file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Nov-01 (gordy)
**	    General cleanup as followup to removal of nat/longnat.
**	 6-Jun-03 (gordy)
**	    Enhanced GCA & GCC MIB information.  Added GCD MIB classes.
**	 7-Aug-06 (rajus01)
**	    Needed gcs.trace_level for iimonitor trace command support in
**	    GCF servers. Added all the other GCS MIB variables in case if
**	    they are needed in the future.
*/

#ifndef GCM_INCLUDED
#define GCM_INCLUDED

/*
** GCA MIB Class IDs
*/

#define GCA_MIB_NO_ASSOCS		"exp.gcf.gca.assoc_count"
#define GCA_MIB_DATA_IN			"exp.gcf.gca.data_in"
#define GCA_MIB_DATA_OUT		"exp.gcf.gca.data_out"
#define GCA_MIB_INSTALL_ID		"exp.gcf.gca.install_id"
#define GCA_MIB_MSGS_IN			"exp.gcf.gca.msgs_in"
#define GCA_MIB_MSGS_OUT		"exp.gcf.gca.msgs_out"
#define GCA_MIB_TRACE_LEVEL		"exp.gcf.gca.trace_level"
#define GCA_MIB_TRACE_LOG		"exp.gcf.gca.trace_log"
#define GCA_MIB_CLIENT			"exp.gcf.gca.client"
#define GCA_MIB_CLIENT_FLAGS		"exp.gcf.gca.client.flags"
#define GCA_MIB_CLIENT_LOCAL_PROTOCOL	"exp.gcf.gca.client.local_protocol"
#define GCA_MIB_CLIENT_API_VERSION	"exp.gcf.gca.client.api_version"
#define GCA_MIB_CLIENT_SRV_CLASS	"exp.gcf.gca.client.server_class"
#define GCA_MIB_CLIENT_LISTEN_ADDRESS	"exp.gcf.gca.client.listen_address"
#define GCA_MIB_ASSOCIATION		"exp.gcf.gca.assoc"
#define GCA_MIB_ASSOC_CLIENT_ID		"exp.gcf.gca.assoc.client_id"
#define GCA_MIB_ASSOC_FLAGS		"exp.gcf.gca.assoc.flags"
#define GCA_MIB_ASSOC_INBOUND		"exp.gcf.gca.assoc.inbound"
#define GCA_MIB_ASSOC_PARTNER_ID	"exp.gcf.gca.assoc.partner_id"
#define GCA_MIB_ASSOC_PARTNER_PROTOCOL	"exp.gcf.gca.assoc.partner_protocol"
#define GCA_MIB_ASSOC_SESSION_PROTOCOL	"exp.gcf.gca.assoc.session_protocol"
#define GCA_MIB_ASSOC_USERID		"exp.gcf.gca.assoc.userid"

/*
** GCN MIB Class IDs
*/

#define GCN_MIB_CACHE_MODIFIED		"exp.gcf.gcn.cache_modified"
#define GCN_MIB_BEDCHECK_INTERVAL	"exp.gcf.gcn.bedcheck_interval"
#define GCN_MIB_COMPRESS_POINT		"exp.gcf.gcn.compress_point"
#define GCN_MIB_DEFAULT_CLASS		"exp.gcf.gcn.default_class"
#define GCN_MIB_EXPIRE_INTERVAL		"exp.gcf.gcn.expire_interval"
#define GCN_MIB_HOSTNAME		"exp.gcf.gcn.hostname"
#define GCN_MIB_INSTALLATION_ID		"exp.gcf.gcn.installation_id"
#define GCN_MIB_LOCAL_VNODE		"exp.gcf.gcn.local_vnode"
#define GCN_MIB_REMOTE_VNODE		"exp.gcf.gcn.remote_vnode"
#define GCN_MIB_REMOTE_MECHANISM	"exp.gcf.gcn.remote_mechanism"
#define GCN_MIB_LTCKT_CACHE_MISS	"exp.gcf.gcn.ticket_lcl_cache_miss"
#define GCN_MIB_LTCKT_CREATED		"exp.gcf.gcn.ticket_lcl_created"
#define GCN_MIB_LTCKT_EXPIRED		"exp.gcf.gcn.ticket_lcl_expired"
#define GCN_MIB_LTCKT_USED		"exp.gcf.gcn.ticket_lcl_used"
#define GCN_MIB_RTCKT_CACHE_MISS	"exp.gcf.gcn.ticket_rmt_cache_miss"
#define GCN_MIB_RTCKT_CREATED		"exp.gcf.gcn.ticket_rmt_created"
#define GCN_MIB_RTCKT_EXPIRED		"exp.gcf.gcn.ticket_rmt_expired"
#define GCN_MIB_RTCKT_USED		"exp.gcf.gcn.ticket_rmt_used"
#define GCN_MIB_TICKET_CACHE_SIZE	"exp.gcf.gcn.ticket_cache_size"
#define GCN_MIB_TICKET_EXPIRE		"exp.gcf.gcn.ticket_expire"
#define GCN_MIB_TIMEOUT			"exp.gcf.gcn.timeout"
#define GCN_MIB_TRACE_LEVEL		"exp.gcf.gcn.trace_level"
#define GCN_MIB_SERVER_ENTRY		"exp.gcf.gcn.server.entry"
#define GCN_MIB_SERVER_ADDRESS		"exp.gcf.gcn.server.address"
#define GCN_MIB_SERVER_CLASS		"exp.gcf.gcn.server.class"
#define GCN_MIB_SERVER_FLAGS		"exp.gcf.gcn.server.flags"
#define GCN_MIB_SERVER_OBJECT		"exp.gcf.gcn.server.object"

/*
** GCC MIB Class IDs
*/

#define GCC_MIB_CONN_COUNT		"exp.gcf.gcc.conn_count"
#define GCC_MIB_IB_CONN_COUNT		"exp.gcf.gcc.ib_conn_count"
#define GCC_MIB_OB_CONN_COUNT		"exp.gcf.gcc.ob_conn_count"
#define GCC_MIB_IB_MAX			"exp.gcf.gcc.ib_max"
#define GCC_MIB_OB_MAX			"exp.gcf.gcc.ob_max"
#define GCC_MIB_IB_MODE			"exp.gcf.gcc.ib_encrypt_mode"
#define GCC_MIB_OB_MODE			"exp.gcf.gcc.ob_encrypt_mode"
#define GCC_MIB_IB_MECH			"exp.gcf.gcc.ib_encrypt_mech"
#define GCC_MIB_OB_MECH			"exp.gcf.gcc.ob_encrypt_mech"
#define GCC_MIB_DATA_IN	    	    	"exp.gcf.gcc.data_in"
#define GCC_MIB_DATA_OUT    	    	"exp.gcf.gcc.data_out"
#define GCC_MIB_MSGS_IN	    	    	"exp.gcf.gcc.msgs_in"
#define GCC_MIB_MSGS_OUT    	    	"exp.gcf.gcc.msgs_out"
#define GCC_MIB_PL_PROTOCOL		"exp.gcf.gcc.pl_proto_lvl"
#define GCC_MIB_REGISTRY_MODE		"exp.gcf.gcc.registry_mode"
#define GCC_MIB_TRACE_LEVEL		"exp.gcf.gcc.trace_level"
#define GCC_MIB_PROTOCOL		"exp.gcf.gcc.protocol"
#define GCC_MIB_PROTO_HOST		"exp.gcf.gcc.protocol.host"
#define GCC_MIB_PROTO_ADDR		"exp.gcf.gcc.protocol.addr"
#define GCC_MIB_PROTO_PORT		"exp.gcf.gcc.protocol.port"
#define GCC_MIB_REGISTRY		"exp.gcf.gcc.registry"
#define GCC_MIB_REG_HOST		"exp.gcf.gcc.registry.host"
#define GCC_MIB_REG_ADDR		"exp.gcf.gcc.registry.addr"
#define GCC_MIB_REG_PORT		"exp.gcf.gcc.registry.port"
#define GCC_MIB_CONNECTION		"exp.gcf.gcc.conn"
#define GCC_MIB_CONN_LCL_PROTOCOL	"exp.gcf.gcc.conn.lcl_addr.protocol"
#define GCC_MIB_CONN_LCL_NODE		"exp.gcf.gcc.conn.lcl_addr.node"
#define GCC_MIB_CONN_LCL_PORT		"exp.gcf.gcc.conn.lcl_addr.port"
#define GCC_MIB_CONN_RMT_PROTOCOL	"exp.gcf.gcc.conn.rmt_addr.protocol"
#define GCC_MIB_CONN_RMT_NODE		"exp.gcf.gcc.conn.rmt_addr.node"
#define GCC_MIB_CONN_RMT_PORT		"exp.gcf.gcc.conn.rmt_addr.port"
#define GCC_MIB_CONN_TRG_PROTOCOL	"exp.gcf.gcc.conn.trg_addr.protocol"
#define GCC_MIB_CONN_TRG_NODE		"exp.gcf.gcc.conn.trg_addr.node"
#define GCC_MIB_CONN_TRG_PORT		"exp.gcf.gcc.conn.trg_addr.port"
#define GCC_MIB_CONN_TARGET		"exp.gcf.gcc.conn.target"
#define GCC_MIB_CONN_USERID		"exp.gcf.gcc.conn.userid"
#define GCC_MIB_CONN_GCA_ID		"exp.gcf.gcc.conn.gca_assoc_id"
#define GCC_MIB_CONN_INBOUND		"exp.gcf.gcc.conn.inbound"
#define GCC_MIB_CONN_FLAGS		"exp.gcf.gcc.conn.flags"
#define GCC_MIB_CONN_AL_PROTOCOL	"exp.gcf.gcc.conn.al_proto_lvl"
#define GCC_MIB_CONN_AL_FLAGS		"exp.gcf.gcc.conn.al_flags"
#define GCC_MIB_CONN_PL_PROTOCOL	"exp.gcf.gcc.conn.pl_proto_lvl"
#define GCC_MIB_CONN_PL_FLAGS		"exp.gcf.gcc.conn.pl_flags"
#define GCC_MIB_CONN_SL_PROTOCOL	"exp.gcf.gcc.conn.sl_proto_lvl"
#define GCC_MIB_CONN_SL_FLAGS		"exp.gcf.gcc.conn.sl_flags"
#define GCC_MIB_CONN_TL_PROTOCOL	"exp.gcf.gcc.conn.tl_proto_lvl"
#define GCC_MIB_CONN_TL_FLAGS		"exp.gcf.gcc.conn.tl_flags"
#define GCC_MIB_CONN_TL_LCL_ID		"exp.gcf.gcc.conn.tl_lcl_id"
#define GCC_MIB_CONN_TL_RMT_ID		"exp.gcf.gcc.conn.tl_rmt_id"

/*
** Bridge MIB Class IDs
*/

#define GCB_MIB_CONN_COUNT		"exp.gcf.gcb.conn_count"
#define GCB_MIB_IB_CONN_COUNT		"exp.gcf.gcb.ib_conn_count"
#define GCB_MIB_CONNECTION		"exp.gcf.gcb.conn"
#define GCB_MIB_CONNECTION_FLAGS	"exp.gcf.gcb.conn.flags"
#define GCB_MIB_MSGS_IN	    	    	"exp.gcf.gcb.msgs_in"
#define GCB_MIB_MSGS_OUT    	    	"exp.gcf.gcb.msgs_out"
#define GCB_MIB_DATA_IN	    	    	"exp.gcf.gcb.data_in"
#define GCB_MIB_DATA_OUT    	    	"exp.gcf.gcb.data_out"


/*
** DAS MIB Class IDs
*/

#define GCD_MIB_CONN_COUNT		"exp.gcf.gcd.conn_count"
#define GCD_MIB_CONN_MAX		"exp.gcf.gcd.conn_max"
#define GCD_MIB_DBMS_COUNT		"exp.gcf.gcd.dbms_count"
#define GCD_MIB_POOL_COUNT		"exp.gcf.gcd.pool_count"
#define GCD_MIB_POOL_MAX		"exp.gcf.gcd.pool_max"
#define GCD_MIB_POOL_TIMEOUT		"exp.gcf.gcd.pool_timeout"
#define GCD_MIB_CLIENT_TIMEOUT		"exp.gcf.gcd.client_timeout"
#define GCD_MIB_TRACE_LEVEL		"exp.gcf.gcd.trace_level"
#define GCD_MIB_PROTOCOL		"exp.gcf.gcd.protocol"
#define GCD_MIB_PROTO_HOST		"exp.gcf.gcd.protocol.host"
#define GCD_MIB_PROTO_ADDR		"exp.gcf.gcd.protocol.addr"
#define GCD_MIB_PROTO_PORT		"exp.gcf.gcd.protocol.port"
#define GCD_MIB_CONNECTION		"exp.gcf.gcd.conn"
#define GCD_MIB_CONN_DBMS		"exp.gcf.gcd.conn.dbms"
#define GCD_MIB_CONN_CLIENT_USER	"exp.gcf.gcd.conn.client_user"
#define GCD_MIB_CONN_CLIENT_HOST	"exp.gcf.gcd.conn.client_host"
#define GCD_MIB_CONN_CLIENT_ADDR	"exp.gcf.gcd.conn.client_addr"
#define GCD_MIB_CONN_LCL_PROTOCOL	"exp.gcf.gcd.conn.lcl_addr.protocol"
#define GCD_MIB_CONN_LCL_NODE		"exp.gcf.gcd.conn.lcl_addr.node"
#define GCD_MIB_CONN_LCL_PORT		"exp.gcf.gcd.conn.lcl_addr.port"
#define GCD_MIB_CONN_RMT_PROTOCOL	"exp.gcf.gcd.conn.rmt_addr.protocol"
#define GCD_MIB_CONN_RMT_NODE		"exp.gcf.gcd.conn.rmt_addr.node"
#define GCD_MIB_CONN_RMT_PORT		"exp.gcf.gcd.conn.rmt_addr.port"
#define GCD_MIB_CONN_MSG_PROTOCOL	"exp.gcf.gcd.conn.msg_proto_lvl"
#define GCD_MIB_CONN_TL_PROTOCOL	"exp.gcf.gcd.conn.tl_proto_lvl"
#define GCD_MIB_DBMS			"exp.gcf.gcd.dbms"
#define GCD_MIB_DBMS_API_LVL		"exp.gcf.gcd.dbms.api_level"
#define GCD_MIB_DBMS_API_HNDL		"exp.gcf.gcd.dbms.api_handle"
#define GCD_MIB_DBMS_TARGET		"exp.gcf.gcd.dbms.target"
#define GCD_MIB_DBMS_USERID		"exp.gcf.gcd.dbms.userid"

/*
** GCS MIB Variable Names
*/

#define GCS_MIB_VERSION                 "exp.gcf.gcs.version"
#define GCS_MIB_INSTALL_MECH            "exp.gcf.gcs.installation_mechanism"
#define GCS_MIB_DEFAULT_MECH            "exp.gcf.gcs.default_mechanism"
#define GCS_MIB_TRACE_LEVEL             "exp.gcf.gcs.trace_level"
#define GCS_MIB_MECHANISM               "exp.gcf.gcs.mechanism"
#define GCS_MIB_MECH_NAME               "exp.gcf.gcs.mechanism.name"
#define GCS_MIB_MECH_VERSION            "exp.gcf.gcs.mechanism.version"
#define GCS_MIB_MECH_CAPS               "exp.gcf.gcs.mechanism.capabilities"
#define GCS_MIB_MECH_STATUS             "exp.gcf.gcs.mechanism.status"
#define GCS_MIB_MECH_ELVL               "exp.gcf.gcs.mechanism.encrypt_level"
#define GCS_MIB_MECH_OVHD               "exp.gcf.gcs.mechanism.overhead"


/*
**  Miscellaneous defines
*/

#define GCM_GCA_PROTOCOL_LEVEL	GCA_PROTOCOL_LEVEL_60
#define	GCM_TRAP_MASK(x)	(1 << (x-1) )	/* convert to bitflag */


/*
** GCM message types
*/

# define    GCM_GET     		0x60	/* GCM Get request */
# define    GCM_GETNEXT    		0x61	/* GCM GetNext request */
# define    GCM_SET     		0x62	/* GCM Set request */
# define    GCM_RESPONSE     		0x63	/* GCM Get/Set response */
# define    GCM_SET_TRAP   		0x64	/* GCM SetTrap request */
# define    GCM_TRAP_IND  		0x65	/* GCM Trap indication */
# define    GCM_UNSET_TRAP  		0x66	/* GCM UnSetTrap request */

# define    GCM_MIN_TYPE     		0x60	/* GCM min message type */
# define    GCM_MAX_TYPE     		0x6F	/* GCM max message type */


/*
**  GCM data objects
*/

/*
** GCM_IDENTIFIER: GCM Object identifier
*/
typedef struct _GCM_IDENTIFIER GCM_IDENTIFIER;

struct _GCM_IDENTIFIER
{
    i4		l_object_class;		/* Length of following array */
    char	object_class[1];	/* Variable length */
    i4		l_object_instance;	/* Length of following array */
    char	object_instance[1];	/* Variable length */
};

/*
** GCM_VALUE: GCM Object value
*/
typedef struct _GCM_VALUE GCM_VALUE;

struct _GCM_VALUE
{
    i4		l_object_value;		/* Length of following array */
    char	object_value[1];	/* Variable length */
};

/*
** GCM_VAR: GCM Variable 
*/
typedef struct _GCM_VAR GCM_VAR;

struct _GCM_VAR
{
    GCM_IDENTIFIER	var_name;	/* Variable name */
    GCM_VALUE		var_value;	/* Variable value */
    i4			var_perms;	/* Variable permissions */
};

/*
** GCM_MSG_HDR: GCM Message header (all messages)
*/
typedef struct _GCM_MSG_HDR GCM_MSG_HDR;

struct _GCM_MSG_HDR
{
    i4		error_status;		/* error indicator */
    i4		error_index;		/* error index */
    i4		future[2];		/* reserved for future use */
    i4		client_perms;		/* permissions claimed by client */
    i4		row_count;		/* number of rows requested */
};

/*
** GCM_TRAP_HDR: GCM Trap header
*/
typedef struct _GCM_TRAP_HDR GCM_TRAP_HDR;

struct _GCM_TRAP_HDR
{
    i4		trap_reason;		/* Trap reason (GET/SET) */
    i4		trap_handle;		/* Trap handle (GCM client) */
    i4		mon_handle;		/* Monitor handle (GCM responder) */
    i4		l_trap_address;		/* Length of following array */
    char	trap_address[1];	/* Variable Length */
};


/*
** GCM objects directly associated with messages
*/


/*
** GCM_QUERY_DATA: 
**
** Messages: GCM_GET, GCM_GETNEXT, GCM_SET, GCM_RESPONSE
*/
typedef struct _GCM_QUERY_DATA GCM_QUERY_DATA;

struct _GCM_QUERY_DATA
{
    GCM_MSG_HDR	msg_hdr;		/* GCM message header */
    i4	    	var_count;		/* Length of following array */
    GCM_VAR	var[1];			/* Variable length */
};

/* 
** GCM_TRAP_DATA: GCM Trap data
**
** Messages: GCM_SET_TRAP, GCM_TRAP_IND, GCM_UNSET_TRAP
*/
typedef struct _GCM_TRAP_DATA GCM_TRAP_DATA;

struct _GCM_TRAP_DATA
{
    GCM_MSG_HDR		msg_hdr;	/* GCM message header */
    GCM_TRAP_HDR	trap_hdr;	/* GCM trap header */
    i4	   		var_count;	/* Length of following array */
    GCM_VAR		var[1];		/* Variable length */
};

#endif /* GCM_INCLUDED */

