/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

/**
** Name: gcaint.h - Definitions used only by GCA
**
** Description:
**      Definitions in this file are for GCA internal use only.
**
** History: $Log-for RCS$
**      14-Apr-1987 (berrow)
**          Created.
**      12-Jan-89 (jbowers)
**          Added gca_lsncnfrm() callback routine to IIGCa_static.
**	31-Jul-89 (seiwald)
**	    New gca_api_version number.
**	20-Sep-89 (seiwald)
**	    New structure for association table in IIGCa_static.
**	    Removed GCA_AS_TBL and associated defines.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	10-Nov-89 (seiwald)
**	    Decrufted.  New gca_trace_level in GCA_STATIC.
**	10-Nov-89 (seiwald)
**	   Removed declaration of SVC_PARMS protected queue.
**	12-Dec-89 (seiwald)
**	   Removed antiquated gca_listen_assoc.  
**	30-Dec-89 (seiwald)
**	   Added gca_syncing flag to keep track of outstanding sync
**	   operations.
**	18-Jun-90 (seiwald)
**	   Decrufted.  Removed irrelevant history before 1989.
**	31-Dec-90 (seiwald)
**	   Added GCA_SAVE_DATA structure for added responsibility of
**	   GCA_SAVE/GCA_RESTORE.
**	7-Jan-91 (seiwald)
**	   Removed GCA_CL_MAX_SAVE_SIZE to <gcaint.h>.
**	   Got GC_IPC_PROTOCOL, GCA_EXP_SIZE, GCA_CL_HDR_SZ from <gcaint.h>.
**	3-April-91 (seiwald)
**	   Added GCA_NORM_SIZE as a default for size_advise.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	14-April-92 (brucek)
**	   Upped GCA_NS_BUF_SZ to 1024.
**	28-may-92 (seiwald)
**	   Added gca_no_security to GCA_STATIC to support II_GCF_SECURITY.
**	19-Oct-92 (brucek)
**	   Added gca_listen_address to GCA_STATIC.
**	19-Nov-92 (seiwald)
**	   New end_of_group bit in GCA_MSG_HDR to indicate sender's idea
**	   of grouping.  Bumped GC_IPC_PROTOCOL to reflect this.
**	07-Jan-93 (edg)
**	   Consolidated all FUNC_EXTERN's from GCA C files here and
**	   GLOBALREF's from gcagref.h.  gcagref.h deleted.
**	08-Jan-93 (edg)
**	   Move FUNC_EXTERN IIGCa_call to gca.h instead.
**	11-Jan-93 (brucek)
**	   Added FUNC_EXTERNs for gcm_[un]reg_trap.
**	12-Jan-93 (brucek)
**	   Added auth_user to GCA_STATIC.
**	10-Mar-93 (edg)
**	   Added acbs_in_use to GCA_STATIC.gca_acbs for use in MIB.
**	14-Mar-94 (seiwald)
**	   Straightened out the FUNC_EXTERN's.
**	4-Apr-94 (seiwald)
**	   Hold header length in IIGCa_static.
**	19-Apr-94 (seiwald)
**	   gca_register() removed.
**	16-May-94 (seiwald)
**	    gca_attribs(), gca_format(), and gca_interpret() removed.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	 4-Dec-95 (gordy)
**	    Added embedded Name Server interface.  Cleaned up GCA_STATIC.
**	 3-Sep-96 (gordy)
**	    Reorganized GCA global data.  Added GCA_CB control blocks to
**	    allow multiple GCA users to call GCA_INITIATE.  GCA_STATIC
**	    renamed to GCA_GLOBAL since not all data is static.
**	31-Jan-97 (gordy)
**	    Enhancing gca_to_len(), gca_to_fmt() to accept formatted
**	    GCD_TD_DATA descriptor structures in addition to GCA_TD_DATA
**	    byte stream descriptors.
**	24-Feb-97 (gordy)
**	    Added GC_IPC_PROTOCOL_5 to indentify individual internal levels.
**	10-Apr-97 (gordy)
**	    Separated GCA and CL specific info from gc.h.  GCA specific
**	    info moved to this file.
**	21-May-97 (gordy)
**	    New GCF security handling.  Revamped PEER_INFO structures.
**	    Original PEER_INFO now called GCA_OLD_PEER.
**	17-Oct-97 (rajus01)
**	    Added GCA_RMT_VAL, GCA_RMT_INFO.
**	04-Dec-97 (rajus01)
**	    Moved gca_pwd_auth(), gca_srv_auth() function definitions
**	    to gcaauth.c.
**	18-Dec-97 (gordy)
**	    Added counter for synchronous requests (moved from global)
**	    and flag for ACB deletion for multi-threaded sync requests.
**	    Moved sync request counter to ACB for multi-threading.
**	29-Dec-97 (gordy)
**	    Security labels passed in peer info.  Cleaned up size_advise 
**	    handling.  Allow nested gca_service subroutines.
**	12-Jan-98 (gordy)
**	    Added lcl_host to GCA_RQNS for direct GCA network connections.
**	17-Feb-98 (gordy)]
**	    Added GCA trace log to globals to support management through GCM.
**	14-May-98 (gordy)
**	    Added remote authentication mechanism in GCA_RQNS info.
**	19-May-98 (gordy)
**	    Removed static buffers from GCA_RQCB, added dynamic resolve buffer.
**	26-May-98 (gordy)
**	    Added flag in globals for ignore/fail of remote auth errors.
**	 4-Jun-98 (gordy)
**	    Save timeout so both Name Server and Server connection
**	    requests can be timed out.
**	28-Jul-98 (gordy)
**	    Added installation ID to GLOBALs for bedchecking of servers.
**	    Move GCA_AUX_DATA to gca.h since exposed by GCA_LISTEN.
**	 8-Jan-99 (gordy)
**	    Add flag for including timestamps in tracing.
**	30-Jun-99 (rajus01)
**	    gca_is_eog() now takes protocol version as parameter instead
**	    of acb.
**	27-Aug-99 (i4jo01)
**	    Increase number of connections granted to 32. (b98558).
**	22-Mar-02 (gordy)
**	    General reorganization associated with cleanup of gca.h.
**	 6-Jun-03 (gordy)
**	    Enhanced association MIB information.
**	 9-Aug-04 (gordy)
**	    Add ACB flag to indicate active normal channel send.
**	03-Aug-05 (gordy / clach04)
**	    115053 : Added def for new GCA routine gcu_restore().
**	 1-Mar-06 (gordy)
**	    The NS registration connection is now kept open to provide
**	    status information updates.  The ACB and registration info
**	    is now saved in the GCA control block.  Added ability to
**	    call operations with another SVC_PARMS so that the NS
**	    connection can be handled internally.
**	11-May-06 (gordy)
**	    Protect registration ACB with semaphore.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**/

#ifndef GCA_INT_INCLUDED
#define GCA_INT_INCLUDED

#include <lo.h>
#include <me.h>
#include <mu.h>

/*
** Macros to build and decompose GCA messages.
*/

# define GCA_GETI2_MACRO( s, d ) \
	(MEcopy((PTR)s, sizeof(i2), (PTR)d), sizeof(i2))

# define GCA_GETI4_MACRO( s, d ) \
	(MEcopy((PTR)s, sizeof(i4), (PTR)d), sizeof(i4))

# define GCA_GETBUF_MACRO( s, len, d ) \
	(MEcopy((PTR)s, len, (PTR)d), len)

# define GCA_PUTI2_MACRO( s, d ) \
	(MEcopy((PTR)s, sizeof(i2), (PTR)d), sizeof(i2))

# define GCA_PUTI4_MACRO( s, d ) \
	(MEcopy((PTR)s, sizeof(i4), (PTR)d), sizeof(i4))

# define GCA_PUTBUF_MACRO( s, len, d ) \
	(MEcopy((PTR)s, len, (PTR)d), len)


/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _GCA_MSG_HDR GCA_MSG_HDR;
typedef struct _GCA_GLOBAL GCA_GLOBAL;
typedef struct _GCA_CB GCA_CB;
typedef struct _GCA_ACB GCA_ACB;
typedef struct _GCA_SVC_PARMS GCA_SVC_PARMS;
typedef struct _GCA_SR_SM GCA_SR_SM;
typedef struct _GCA_RMT_ADDR GCA_RMT_ADDR ;
typedef struct _GCA_PEER_V5 GCA_PEER_V5;
typedef struct _GCA_PEER_V6 GCA_PEER_V6;
typedef struct _GCA_PEER_V7 GCA_PEER_V7;
typedef struct _GCA_PEER_INFO GCA_PEER_INFO;
typedef struct _GCA_SEC_LABEL_DATA GCA_SEC_LABEL_DATA;
typedef struct _GCA_SAVE_DATA GCA_SAVE_DATA;
typedef struct _GCA_RQNS GCA_RQNS;
typedef struct _GCA_RQCB GCA_RQCB;
typedef struct _GCA_RMT_VAL GCA_RMT_VAL; 
typedef struct _GCA_RMT_INFO GCA_RMT_INFO;


/*
**  Defines of other constants.
*/

/*
** The following constants are used internally by GCA.
*/

# define	GCA_NORM_SIZE	  4096	/* Normal data buffer size (default) */
# define	GCA_EXP_SIZE	    64  /* Expedited data buffer size */


/*}
** Name: GCA_MSG_HDR - Header structure for all GCA message buffers.
**
** Description:
**       
**      GCA_MSG_HDR contains message and buffer related information internal
**      to GCA.  It is the initial data component of every GCA message buffer.
**      It is initialized by the GCA_FORMAT service.
**
** History:
**      15-Apr-87 (jbowers)
**          Structure creation
**
*/
struct _GCA_MSG_HDR
{
    i4		buffer_length;		/* Total length of message buffer */
    i4		msg_type;		/* message type indicator:        */
					/* values are defined in gca.h    */
    i4		data_offset;		/* Offset of message data - defunct */
    i4		data_length;		/* Actual length of message data  */

    struct
    {

	BITFLD	end_of_data:1;		/* last fragment of message   */
	BITFLD	flow_indicator:1;	/* Normal/Expedited flow      */
	BITFLD	end_of_group:1;		/* last message in group      */

    } flags;

    char	pad[3];			/* pad to length 0 modulo 4	  */

};


/*}
** Name: GCA_GLOBAL - GCA global data 
**
** Description:
**       
**	The GCA_GLOBAL area contains global data values which are 
**	independent from the values provided in any GCA_INITIATE 
**	call.
**
**	Since we must consistently use the same memory management
**	routines, the routines provided by the first GCA_INITIATE
**	call are used and stored here.
**
** History:
**      01-Sep-87 (berrow)
**          First structure creation
**      13-Apr-1994 (daveb) 62240
**          Add traffic stats
**	 4-Dec-95 (gordy)
**	    Reorganized, added several new members.
**	 3-Sep-96 (gordy)
**	    Extracted GCA_INITIATE info to control block GCA_CB.
**	    Renamed to GCA_GLOBAL since some data is not static.
**	21-May-97 (gordy)
**	    New GCF security handling.  Added server key.
**	18-Dec-97 (gordy)
**	    Added counter for synchronous requests (moved from global)
**	    and flag for ACB deletion for multi-threaded sync requests.
**	    Moved sync request counter to ACB for multi-threading.
**	17-Feb-98 (gordy)
**	    Added GCA trace log to globals to support management through GCM.
**	26-May-98 (gordy)
**	    Added flag for ignore/fail of remote auth errors.
**	28-Jul-98 (gordy)
**	    Added installation ID for bedchecking of servers.
**	 8-Jan-99 (gordy)
**	    Add flag for including timestamps in tracing.
**	 4-Aug-99 (rajus01)
**	    Add gca_embed_gcc_flag to enable/disable built-in embedded comsvr 
**	    This flag takes value from II_EMBEDDED_GCC environment logical.
**	31-Jul-09 (gordy)
**	    User ID is dynamically allocated.
*/

struct _GCA_GLOBAL
{
    /* Static values */
    bool		gca_initialized;	/* Initialization flag */
    bool		gca_embedded_gcn;	/* Embedded NS Interface */
    bool		gca_rmt_auth_fail;	/* Fail on remote auth error */
    i4			gca_trace_level;	/* Tracing level */
    bool		gca_trace_time;		/* Trace timestamp */
    char 		gca_trace_log[MAX_LOC];	/* Tracing log */
    bool		gca_embed_gcc_flag;	/* Embedded COMSVR Flag */

    /* Memory management routines */
    STATUS      	(*gca_alloc)();
    STATUS      	(*gca_free)();

    /* Counters for GCA operations */
    i4			gca_initiate;		/* GCA_INITIATE requests */
    i4			gca_terminate;		/* GCA_TERMINATE requests */

    /* Association control blocks */
    MU_SEMAPHORE	gca_acb_semaphore;	/* multi-threading protection */
    i4			gca_acb_max;		/* length of gca_acb_array */
    i4			gca_acb_active;		/* number of ACB's in use */
    GCA_ACB		**gca_acb_array;	/* ACB pointers */

    /* Identifiers */
    char		gca_install_id[4];		    /* Installation */
    char 		gca_srvr_id[GCA_SERVER_ID_LNG];     /* Server ID */
    char		gca_srvr_key[GCA_SERVER_ID_LNG];    /* Server key */
    char		*gca_uid;			    /* User ID */

    /* IO statistics */
    u_i4		gca_msgs_in;
    u_i4		gca_msgs_out;
    u_i4		gca_data_in;
    u_i4		gca_data_out;
    u_i4		gca_data_out_bits;	/* Carry-over values */
    u_i4		gca_data_in_bits;
};



/*}
** Name: GCA_CB - GCA Control Block
**
** Description:
**       
**	The GCA_CB area contains all storage not specific to a single
**	association but are dependent on info provided at GCA_INITIATE
**	time.  
**
** History:
**	 3-Sep-96 (gordy)
**	    Extracted from GCA_STATIC.
**	21-May-97 (gordy)
**	    New GCF security handling.  Removed user password authentication.
**	 1-Mar-06 (gordy)
**	    Added ACB and parmlist for the NS registration connection.
**	11-May-06 (gordy)
**	    Protect registration ACB with semaphore.
*/

struct _GCA_CB
{
    /* GCA_INITIATE called flag */
    bool	gca_initialized;

    /* Control block ID */
    i4		gca_cb_id;

    /* Global flags */
    u_i4	gca_flags;

    /* Local GCA protocol version. */
    i4		gca_local_protocol;

    /* Access point interface version. */
    i4		gca_api_version;

    /* Registration info */
    char	gca_listen_address[GCA_SERVER_ID_LNG];
    char 	gca_srvr_class[GCA_SERVER_ID_LNG];

    GCA_RG_PARMS	gca_reg_parm;
    MU_SEMAPHORE	gca_reg_semaphore;	/* multi-threading protection */
    GCA_ACB		*gca_reg_acb;

    /* Length of GCA_MSG_HDR across GCA interface: zero at API_LEVEL_5 */
    i4		gca_header_length;

    /* User's normal completion exit */
    void        (*normal_ce)();

    /* User's expedited completion exit */
    void        (*expedited_ce)();

    /* User-supplied GCA_LISTEN acceptance confirmation calback routine. */
    bool	(*lsncnfrm)();
};


/*
** Name: GCA_SVC_PARMS
**
** Description:
**	GCA internal service parameters.
**
** History:
**	10-Apr-97 (gordy)
**	    Extracted from SVC_PARMS.
**	27-May-98 (gordy)
**	    Added define for stack size.
**	 4-Jun-98 (gordy)
**	    Added timeout so both Name Server and Server connection
**	    requests can be timed out.
**	 1-Mar-06 (gordy)
**	    Added call parms to allow call/return operations with an 
**	    unrelated SVC_PARMS (permits processing on the internal
**	    NS registration connection).  Increased gosub/call stack
**	    to handle the additional call depth.
*/

struct _GCA_SVC_PARMS
{
    bool		in_use;	    	/* svc_parms in use */
    GCA_CB		*gca_cb;	/* GCA Control Block */
    GCA_ACB		*acb;		/* ACB for this request */
    i4			state;		/* for multi-state gca operations */
    i4			service_code;	/* The service to be performed */
    PTR			parameter_list;	/* GCA request parameters */
    i4			time_out;	/* Request timeout */
    SVC_PARMS		gc_parms;	/* GC request parameters */

#define	GCA_STACK_SIZE	5

    i4			ss[ GCA_STACK_SIZE ];	/* state stack */
    i4			sp;		/* state stack counter */

    GCA_SVC_PARMS	*call_parms;	/* CALL/CALLRET alternate SVC_PARMS */
};


/*
** Name: GCA_RMT_ADDR - A type of aux data containing remote addressing info.
**
** Description:
**	The following structure contains the addressing info returned by
**	Name Service, required for making a remote connection.  This
**	info is passed to a communication server.  Its type identifier is
**	GCA_ID_RMT_ADDR.
**
** History:
**      01-Jul-88 (jbowers)
**          Initial structure implementation
**      01-Nov-88 (jbowers)
**          Added user_id and password elements.
**	21-Jul-09 (gordy)
**	    Hard code lengths since these must not change to maintain
**	    backward compatibility.
*/

struct _GCA_RMT_ADDR
{
    char	partner_id[ 64 ];
    char	protocol_id[ 16 ];
    char	node_id[ 64 ];
    char	port_id[ 64 ];
    char	user_id[ 32 ];
    char	password[ 64 ];
};

/*
** Name: GCA_RMT_VAL
**
** Description:
**	Remote connection and other attribute info returned by Name Server.
**
** History:
**	03-Oct-97 (rajus01)
**	    Extracted from GCA_RMT_ADDR. Added encryption mechanism
**	    and encryption mode values.
*/
struct _GCA_RMT_VAL
{
    i4 		rmtval_id;

#define	GCA_RMT_PRTN_ID		1	/* partner  id */
#define GCA_RMT_PROTO_ID	2	/* network protocol id */
#define GCA_RMT_NODE_ID		3	/* network address */
#define GCA_RMT_PORT_ID		4	/* listen address */
#define GCA_RMT_USER		5	/* username */
#define GCA_RMT_PASSWD		6	/* password */
#define GCA_RMT_AUTH		7	/* remote authentication */
#define GCA_RMT_EMECH		8	/* encryption mechanism */
#define GCA_RMT_EMODE		9	/* encryption mode */

    i4		rmtval_len;
    char	rmtval_value[1];

};

/*
** Name: GCA_RMT_INFO
**
** History:
**	03-Oct-97 (rajus01)
**	    Created.
*/
struct _GCA_RMT_INFO
{
    GCA_RMT_VAL	 rmtvalues[1];
};


/*
** Name: GCA_PEER_V*
**
** Description:
**	Structures representing peer info versions passed between processes.
**
**	For backward compatibility, the first two elements must be
**	i4s and the second element must be the internal protocol
**	level or data structure version.
**
**
** History:
**      14-May-87 (berrow)
**          Initial structure implementation
**      01-Jul-88 (jbowers)
**          Added GCA_AUX_DATA structure to GCA_PEER_INFO.
**      01-Nov-88 (jbowers)
**	    Turned 2 "futures" into internal protocol id and status.
**	1-Aug-89 (seiwald)
**	    Exchanged gca_partner_protocol and gca_version.
**	    Gca_version was being used as if it was partner_protocol.
**	11-Aug-89 (jorge)
**	    Corrected gca_partner_protocol to be a i4, retired gca_version
**	    as gca_past1.
**	 2-May-97 (gordy)
**	    Renamed ot GCA_OLD_PEER to make way for new, extensible peer info.
**	17-Jul-09 (gordy)
**	    Renamed GCA_OLD_PEER to GCA_PEER_V5 to standardize subsequent 
**	    versions.  Copied current GCA_PEER_INFO to GCA_PEER_V6 and added
**	    GCA_PEER_V7.  Hard code array lengths to ensure static size in 
**	    future.
*/

struct _GCA_PEER_V5
{
    i4		gca_partner_protocol;
    i4		gca_version;
    i4		gca_status;
    i4		gca_old1[2];
    i4		gca_old2;
    char	gca_user_id[ 32 ];
    char	gca_password[ 64 ];
    u_i4	gca_flags;

    struct
    {
	i4		type_aux_data;
	GCA_RMT_ADDR	rmt_addr;
    } gca_aux_data;
};

struct _GCA_PEER_V6
{

    i4		gca_length;		/* Length of fixed portion */
    i4		gca_version;		/* Internal IPC version */
    i4		gca_id;			/* 'GCAP' (ascii big-endian) */
    u_i4	gca_flags;		/* Exchanged indicators */
    i4		gca_partner_protocol;   /* Partner's GCA protocol level */
    i4		gca_status;		/* Server to client status */
    char	gca_user_id[ 32 ];	/* User alias */
    i4		gca_aux_count;		/* Number of aux data elements */

    /*
    ** Fixed length data is followed by variable length data:
    **	GCA_AUX_DATA	gca_aux_data[*];
    */
};

struct _GCA_PEER_V7
{

    i4		gca_length;		/* Length of entire peer info */
    i4		gca_version;		/* Internal IPC version */
    i4		gca_id;			/* 'GCAP' (ascii big-endian) */
    u_i4	gca_flags;		/* Exchanged indicators */
    i4		gca_partner_protocol;   /* Partner's GCA protocol level */
    i4		gca_status;		/* Server to client status */
    i4		gca_user_len;		/* User alias length */
    i4		gca_aux_count;		/* Number of aux data elements */

    /*
    ** Fixed length data is followed by variable length data:
    **	char		gca_user_id[*];
    **	GCA_AUX_DATA	gca_aux_data[*];
    */
};

#define	GCA_PEER_ID	0x47434150	/* 'GCAP' (ascii big-endian) */


/*
** Name: GCA_PEER_INFO
**
** Description:
**	Peer information initially exchanged on an association.
**
** History:
**	 2-May-97 (gordy)
**	    Created.
**	17-Jul-09 (gordy)
**	    This structure is now the in memory representation
*8	    of the exchanged peer info.
*/

struct _GCA_PEER_INFO
{

    i4		gca_version;

# define GCA_IPC_PROTOCOL_5	5	/* EOG carried in GCA_MSG_HDR */
# define GCA_IPC_PROTOCOL_6	6	/* Variable length aux data */
# define GCA_IPC_PROTOCOL_7	7	/* Variable length user ID */

# define GCA_IPC_PROTOCOL	GCA_IPC_PROTOCOL_7

    u_i4	gca_flags;		/* Exchanged indicators */
    i4		gca_partner_protocol;   /* Partner's GCA protocol level */
    i4		gca_status;		/* Server to client status */
    i4		gca_aux_count;		/* Number of aux data elements */
    char	*gca_user_id;		/* User alias */

};


/*
** Name: GCA_SR_SM - Send/receive state machines
**
** Description:
**       
**      GCA_SEND and GCA_RECEIVE buffer client requests, managing intermediate
**	buffers for both normal and expedited data.  A GCA_SR_SM controls
**	the state of each request and the state of the buffer.
**
**	Svcbuf points to data being copied to/from the buffer.  
**
**	Buffer points to the base of allocated buffer.
**
**	Hdr stores the GCA_MSG_HDR on receives (and soon on sends).
**	When this structure moves to mainline, hdr will be declared
**	properly.
**
** History:
**	22-Dec-89 (seiwald)
**	    Written.
**	19-Aug-96 (gordy)
**	    Added usrptr for message segment concatenation.
**	21-May-98 (gordy)
**	    Use GCA_MSG_HDR structure rather than char array 
**	    for proper alignment.
**	17-Aug-2010 (thaju02 for Gordy) Bug 124252
**	    Define usrlen as an i8 for 64-bit. 
**	    This is due to unusual values returned from gco_encode() 
**	    during message processing.  This will be removed with the 
**	    removal of  GCA formatted interface.
*/

struct _GCA_SR_SM 
{
    char		*usrbuf;	/* user buffer */
    char		*usrptr;	/* active part of buffer */
#if defined(LP64)
    i8			usrlen;		/* user buffer length */
#else
    i4			usrlen;		/* user buffer length */
#endif
    char		*svcbuf;	/* service buffer */
    i4			svclen;		/* length of svcbuf */
    char		*buffer;	/* buffer to below */
    char		*bufptr;	/* active part of buffer */
    char		*bufend;	/* end of active part */
    i4			bufsiz;		/* size of buffer */
    GCA_MSG_HDR		hdr;		/* Message header */
    PTR			doc;		/* GCC_DOC for encodingd/decoding */
};


/*
** Name: GCA_RQNS: resolved addressing info
**
** Description:
**      GCA_RQNS holds the contents from the Name Server GCA_NS_RESOLVED
**      message for GCA_REQUEST to use.
**
**      For local connections lcl_count will be 1 or more, lcl_addr
**      will list local servers, and rmt_count will be 1.  ALL OTHER
**      FIELDS MUST BE SET TO "".
**
**      For remote connections lcl_count will be 1 or more; lcl_addr
**      will list local Comm Servers; rmt_count will be 1 or more;
**      node, procotol and port will list remote Comm Servers; username
**      and password will be the login information for the host of the
**      remote Comm Server; rmt_dbname will be the original target name
**      with the vnode:: stripped.
**
**      GCA_REQUEST tries each lcl_addr in turn, handing it to
**      GCrequest as the partner name.
**
**      GCA_REQUEST tries each node, protocol, and port in turn only if
**      the error status in the peer info received from the local Comm
**      Server indicates that the network address was invalid.
**
** History:
**      15-Mar-91 (seiwald)
**          Extracted from GCA_RQCB.
**	21-May-97 (gordy)
**	    New GCF security handling.  Revamped for new NS info.
**	04-Dec-97 (rajus01)
**	    Added rmt_auth_len. 
**	12-Jan-98 (gordy)
**	    Added lcl_host for direct GCA network connections.
**	14-May-98 (gordy)
**	    Added remote authentication mechanism.
*/

#define GCA_SVR_MAX	32	/* Must match GCN_SVR_MAX */

struct _GCA_RQNS
{
    char	*lcl_user;

    i4		lcl_count;
    char	*lcl_host[ GCA_SVR_MAX ];
    char	*lcl_addr[ GCA_SVR_MAX ];
    i4		lcl_l_auth[ GCA_SVR_MAX ];
    char	*lcl_auth[ GCA_SVR_MAX ];

    char	*rmt_db;
    char	*rmt_user;
    char	*rmt_pwd;
    i4  	rmt_auth_len;	/* Length of remote auth */
    char	*rmt_auth;	/* Remote auth */
    char	*rmt_mech;	/* Remote auth mechanism */
    char	*rmt_emech;	/* Encryption mechanism */
    char	*rmt_emode;	/* Encryption mode */

    i4		rmt_count;
    char	*node[ GCA_SVR_MAX ];
    char	*protocol[ GCA_SVR_MAX ];
    char	*port[ GCA_SVR_MAX ];

};


/*
** Name: GCA_RQCB - request control block
**
** Description:
**	Information required during GCA_REQUEST processing.
**
** History:
**	04-Aug-89 (seiwald)
**	    Created.
**	29-Dec-97 (gordy)
**	    Removed unused members.
**	19-May-98 (gordy)
**	    Removed static buffers (use ACB send/receive buffers),
**	    add dynamic buffer for GCN_RESOLVE info.
*/

struct _GCA_RQCB 
{
    STATUS	connect_status;		/* status before GCdisconn */
    CL_ERR_DESC connect_syserr;		/* syserr before GCdisconn */
    i4		lcl_indx;		/* index up to ns.lcl_count */
    i4		rmt_indx;		/* index up to ns.rmt_count */
    char	*rslv_buff;		/* GCN_RESOLVE info */
    GCA_RQNS 	ns; 			/* response from name server */

    /*
    ** When resolving GCN info through the embedded Name
    ** Server interface, storage is required for the
    ** result pointers in the response structure, ns,
    ** declared above.  The following structure provides
    ** the required storage and serves as the interface
    ** control block to the embedded Name Server.  It is
    ** only allocated if the embedded Name Server interface
    ** is called.
    */
    PTR		grcb;
};


/*
** Name: GCA_ACB - Association Control Block
**
** Description:
**       
**      The ACB contains information about an association which is required
**      by various elements during the duration of an association.  It is 
**      pointed to by the association table.
**
**      The structure rcv_save and the elements snd_rq and rcv_rq are all 
**      specified as arrays of size 2.  These are to accommodate normal and
**      expedited flow data.  In code that actually uses these elements, the
**      index will be specified as GCA_NORMAL or GCA_EXPEDITED, whose values
**      are respectively 0 and 1, to provide more meaningful documentation
**      in the code sequence.  So, e.g., the send request pointer for 
**      expedited data is specified as 
**
**      acb->snd_rq[GCA_EXPEDITED].
**
** History:
**      17-Apr-87 (jbowers)
**          First structure creation
**	1-Aug-89 (seiwald)
**	    Added gca_send_peer_info so mainline may exchange peer info.
**	    CL formerly did so with a pair of buffers.  Now mainline has
**	    the pair of buffers.
**	09-Oct-89 (seiwald)
**	    Heterogenous support changed.  New GCA_SEND no longer need spare 
**	    SVC_PARMS for sending GCA_TO_DESCR.  Changed names to reflect 
**	    GCA_SEND sending a GCA_TO_DESCR, not GCA_IT_DESCR.
**	16-Feb-90 (seiwald)
**	    Zombie checking removed from mainline to VMS CL.  Removed
**	    references to ACB flags controlling zombie checking.
**	15-May-91 (seiwald)
**	    Purging state has been split into sender purging (we sent 
**	    a GCA_ATTENTION message - acb->flags.spurging) and receiver 
**	    purging (we received it - acb->flags.rpurging), so that 
**	    receiver purging may be handled differently.  This supports 
**	    a change in the purging done by GCA_RECEIVE (c.f.).
**	14-Jan-93 (edg)
**	    Added connection control block (from SVC_PARMS).
**	20-Nov-95 (gordy)
**	    Added remote for embedded Comm Server configuration.
**	18-Feb-97 (gordy)
**	    Added ACB concatenate flag to patch Name Server protocol.
**	    Moved other ACB booleans into flags.
**	21-May-97 (gordy)
**	    Added aux data buffers as no contained directly in peer info.
**	29-Dec-97 (gordy)
**	    Added gc_size_advise in addition to our size_advise.
**	 6-Jun-03 (gordy)
**	    Enhanced MIB information.
**	 9-Aug-04 (gordy)
**	    Add flag to indicate active normal channel send.
*/

struct _GCA_ACB
{
    i4              assoc_id;           /* Association identifier */
    PTR		    gc_cb;		/* GCA CL control block */
    i4		    size_advise;	/* GCA buffer size */
    i4		    gc_size_advise;	/* CL recommended buffer size */
    GCA_RQCB	    *connect;		/* connection control block */
    i4		    *syncing;		/* Synchronous request count */

    /* Association state */

    struct
    {

	BITFLD      delete_acb:1;	/* ACB should be deleted */
	BITFLD	    send_active:1;	/* GCsend() active for normal chan */
	BITFLD	    rpurging:1;		/* received ATTN and purging */
	BITFLD	    spurging:1;		/* sent ATTN and purging */
	BITFLD	    iack_owed:1;	/* Next message sent must be IACK */
	BITFLD	    chop_mks_owed:1;	/* TRUE: Chop marks must be sent */
	BITFLD      initial_seg:1;	/* TRUE if next seg begins message */
	BITFLD      concatenate:1;	/* TRUE to concatenate msg segs */
	BITFLD	    heterogeneous:1;	/* TRUE if association is het. */
	BITFLD      remote:1;		/* TRUE for outgoing remote assoc. */
	BITFLD	    mo_attached:1;	/* TRUE if MO attached */

    } flags;

    char	    *to_descr;		/* Ptr. to GCA_TO_DESCR msg area */
    i4		    sz_to_descr;	/* alloc'ed area of to_descr */
    u_i4	    id_to_descr;	/* Used to detect tuple_id change */

    PTR		    td_cb;		/* Tuple descriptor control block */
    PTR		    td_prog;		/* Compiled tuple descriptor program */

    /* Parms for gca service calls */

    GCA_SVC_PARMS   snd_parms[2];	/* norm + exp send */
    GCA_SVC_PARMS   rcv_parms[2];	/* norm + exp recv */
    GCA_SVC_PARMS   con_parms;		/* listen, request, disassoc */

    /* State machines */

    char	    *buffers;		/* buffer space for send/recv */

    GCA_SR_SM 	    recv[2]; 
    GCA_SR_SM 	    send[2]; 

    /* Connection info */

    char	    *user_id;		/* User ID of association */
    char	    *client_id;		/* User ID of client */
    char	    *partner_id;	/* Partner access ID */
    char	    *inbound;		/* GCA LISTEN (Y) or REQUEST (N) */
    i4		    gca_protocol;	/* Protocol level of association */
    i4		    req_protocol;	/* Requested protocol level */
    u_i4	    gca_flags;		/* Association indicators */

    /* Peer info */

    GCA_PEER_INFO   gca_peer_info;	/* Peer GCA peer info (recv) */
    GCA_PEER_INFO   gca_my_info;	/* My GCA peer info (send) */
    i4		    gca_aux_max;	/* AUX data buffer */
    i4		    gca_aux_len;
    char	    *gca_aux_data;

};


/*}
** Name: GCA_SAVE_DATA - structure for passing GCA association between processes
**
** Description:
**	Connection information can be passed between parent/child 
**	processes by GCA_SAVE and GCA_RESTORE.  This structure describes 
**	the GCA level information which must be passed.
**
**	GCA_SAVE_LEVEL_MAJOR must be changed if incompatible changes are
**	made to this structure; this will cause GCA_RESTORE to reject
**	the connection.
**
**	GCA_SAVE_LEVEL_MINOR can be changed when minor, compatible,
**	changes are made to this structure.
**
**	This structure should remain the same size; a "future" section
**	is provided.  If the size of the structure changes, so must 
**	GCA_SAVE_LEVEL_MAJOR.
**
** History:
**	31-Dec-90 (seiwald)
**	    Created.
*/

# define	GCA_SAVE_LEVEL_MAJOR	0x0604
# define	GCA_SAVE_LEVEL_MINOR	0x0001

struct _GCA_SAVE_DATA 
{
    short	    save_level_major;	/* for validating save format */
    short	    save_level_minor;	/* for compatible changes */
    i4		    cl_save_size;	/* size of cl_save_buf */
    i4		    user_save_size;	/* size of user_save_buf */

    /* Portions of the ACB which must be saved. */

    i4              assoc_id;           /* Association identifier */
    i4		    size_advise;	/* GCA buffer size */
    i4	    	    heterogeneous;	/* TRUE if association is het. */
    char	    future[104];	/* for more junk */

    /* Followed by CL level info. */

    /* Followed by user level info. */
};


/*
** Name: GCA_SEC_LABEL_DATA
**
** Description:
**	This is passed immediately following the peer info in a secure
**	system. 
**
** History:
**	10-aug-93 (robf)
**	   Created
**	23-dev-93  (robf)
**         Changed GC_L_SECLABEL to 140 for HP huge security labels.
**	21-Jul-09 (gordy)
**	    Deprecated GC_L_SECLABEL.
*/

struct _GCA_SEC_LABEL_DATA
{
    i4	version;
    i4	flags;

    /*
    ** Security label information. The is added at GCA_PROTOCOL_LEVEL 61, any
    ** partners less than this should  not be sent the info
    */
    i4 	label_type;	/* Type of security label */

    /* Big enough for any known security label*/
    char	sec_label[ 140 ]; /*Partner label */
};


/*
** References to global variables declared in other GCA C files.
*/

GLOBALREF GCA_GLOBAL	IIGCa_global;	/* GCA global data area */

# define GCA_DEBUG_MACRO( x ) \
	if ( IIGCa_global.gca_trace_level >= x ) \
	    (IIGCa_global.gca_trace_time ? gcx_timestamp() : x), \
	    TRdisplay


/*
** GCA internal functions
*/

FUNC_EXTERN GCA_ACB     *gca_add_acb( VOID );
FUNC_EXTERN GCA_ACB     *gca_rs_acb( i4  assoc_id );
FUNC_EXTERN GCA_ACB     *gca_find_acb( i4  assoc_id );
FUNC_EXTERN i4		gca_next_acb( i4  assoc_id );
FUNC_EXTERN STATUS      gca_del_acb( i4  assoc_id );
FUNC_EXTERN VOID	gca_free_acbs( VOID );
FUNC_EXTERN STATUS	gca_append_aux( GCA_ACB *, i4  len, PTR aux );
FUNC_EXTERN STATUS	gca_aux_element(GCA_ACB *, i4  type, i4  len, PTR aux);
FUNC_EXTERN VOID	gca_del_aux( GCA_ACB * );
FUNC_EXTERN VOID	gca_save_peer_user( GCA_PEER_INFO *, char * );
FUNC_EXTERN STATUS	gca_seclab(GCA_ACB *, GCA_PEER_INFO *, GCA_RQ_PARMS *);
FUNC_EXTERN STATUS	gca_usr_auth( GCA_ACB *, i4  len, PTR buff );
FUNC_EXTERN STATUS	gca_auth( GCA_ACB *, GCA_PEER_INFO *, 
				  GCA_RQ_PARMS *, bool, i4, PTR );
FUNC_EXTERN STATUS	gca_srv_key( GCA_ACB *, char * );
FUNC_EXTERN STATUS	gca_rem_auth( char *, char *, i4 *, PTR );
FUNC_EXTERN VOID        gca_service( PTR closure );
FUNC_EXTERN VOID        gca_resume( PTR closure );
FUNC_EXTERN VOID        gca_complete( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN VOID        gca_initiate( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN STATUS	gca_trace_log( i4, i4, char *, i4, PTR );
FUNC_EXTERN STATUS	gca_chk_priv( char *user_name, char *priv_name );
FUNC_EXTERN VOID        gca_restore( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN STATUS      gcu_restore( char *id, bool clear, i4 *length, PTR buff );
FUNC_EXTERN VOID        gca_save( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN VOID        gca_terminate( GCA_SVC_PARMS *svc_parms );
FUNC_EXTERN PTR         gca_alloc( u_i4 size );
FUNC_EXTERN char *	gca_str_alloc( char *str );
FUNC_EXTERN VOID        gca_free( PTR ptr );
FUNC_EXTERN bool        gca_is_eog( i4  message_type, u_i4 gca_prot );
FUNC_EXTERN i4		gca_descr_id( bool frmtd, PTR ptr );
FUNC_EXTERN STATUS	gca_resolved( i4, char *, GCA_RQCB *, i4 );

FUNC_EXTERN STATUS	gca_ns_init( VOID );
FUNC_EXTERN VOID	gca_ns_term( VOID );
FUNC_EXTERN STATUS	gca_ns_resolve( char *, char *, char *, 
					char *, char *, GCA_RQCB * );

#endif

