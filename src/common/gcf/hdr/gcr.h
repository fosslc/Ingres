/*
** Name: gcr.h
**
** Description:
**	GCA Router to support embedded Comm Server configuration.
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	 9-Jan-96 (gordy)
**	    Added GCR_STATIC definition and reference for IIGCr_static.
**	21-Oct-96 (kamal03)
**		Moved GCR_STATIC reference for IIGCr_static under 
**		#ifdef GCF_EMBEDDED_GCC  to resolve LINK problems when
**		building without embedded com server. 
**	30-June-99 (rajus01)
**	    Removed references to GCR_SENDPEER, GCR_RECVPEER, GCsendpeer(),
**	    GCrecvpeer().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef _GCR_INCLUDED_
#define _GCR_INCLUDED_

typedef struct _GCR_STATIC	GCR_STATIC;
typedef struct _GCR_CONN_INFO	GCR_CONN_INFO;
typedef struct _GCR_SR_INFO	GCR_SR_INFO;
typedef struct _GCR_CB		GCR_CB;

/*
** GCA Router request types.  These coorespond to GCA CL or GCA
** requests.
*/
#define	GCR_INITIATE		1
#define	GCR_REGISTER		2
#define	GCR_LISTEN		3
#define GCR_RQRESP		4
#define	GCR_REQUEST		5
#define	GCR_SEND		6
#define	GCR_RECEIVE		7
#define	GCR_PURGE		8
#define	GCR_SAVE		9
#define	GCR_RESTORE		10
#define	GCR_DISCONN		11
#define	GCR_RELEASE		12
#define	GCR_TERMINATE		13


/*
** Name: GCR_STATIC
**
** Description:
**	Global data for embedded Comm Server router.
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
**	 4-Aug-99 (rajus01)
**          Add gcr_embed_flag to enable/disable built-in embedded comsvr
**          This flag takes value from II_EMBEDDED_GCC environment logical.
*/

struct _GCR_STATIC
{

    /*
    ** Queues for router control blocks.  The
    ** control blocks are either active, waiting
    ** to be freed, or freed.
    */
    QUEUE	gcr_active_q;
    QUEUE	gcr_wait_q;
    QUEUE	gcr_free_q;

    /*
    ** Flag which takes value from an
    ** environment logical (II_EMBEDDED_GCC)
    ** to enable/disable built-in Embedded Comm
    ** Server support.
    */

    bool	gcr_embed_flag;

};


/*
** Name: GCR_CONN_INFO
**
** Description:
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

struct _GCR_CONN_INFO
{

    /*
    ** Info from original requests.
    */
    SVC_PARMS		*svc_parms;	/* GC CL service parameters */
    GCA_PARMLIST	*parmlist;	/* GCA parameter list */
    PTR			mde;		/* GCC MDE */

};

/*
** Name: GCR_SR_INFO
**
** Description:
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

struct _GCR_SR_INFO
{
    i4			flags;

#define	GCR_F_GCA	0x01		/* GCA request waiting */
#define	GCR_F_GCC	0x02		/* GCC request waiting */
#define GCR_F_CONT	0x04		/* GCC request not completed */
#define	GCR_F_PURGE	0x08		/* Purge receive request */
#define	GCR_F_IACK	0x10		/* Accept only GCA_IACK message */

    /*
    ** Info from original requests.
    */
    SVC_PARMS		*svc_parms;	/* GC CL service parameters */
    GCA_PARMLIST	*parmlist;	/* GCA parameter list */
    PTR			mde;		/* GCC MDE */

    /*
    ** Current info.
    */
    i4			i_msg_len;	/* Remaining input message length */
    i4			i_buf_len;	/* Remaining input buffer length */
    char		*i_buf_ptr;	/* Current input buffer position */

    i4			o_msg_len;	/* Current output message length */
    i4			o_buf_len;	/* Remaining output buffer length */
    char		*o_buf_ptr;	/* Current output buffer position */

    GCA_MSG_HDR		hdr;		/* Header sent by GCA in GCR_SEND */
    i4			hdr_len;	/* Amount of header already read */
    bool		end_of_data;	/* Last piece of message */

};

/*
** Name: GCR_CB
**
** Description:
**	GCA Router control block.  Provides the link between the GCA
**	Association Control Block (GCA_ACB) and the GCC Connection
**	Control Block (GCC_CCB) for a given client-server channel.
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	 9-Jan-96 (gordy)
**	    Added queue entry.
**	30-June-99 (rajus01)
**	    Added aux_data. Removed acb, GCR_F_SL, GCR_S_SL, GCR_I_SL,
**	    GCR_A_SL.
*/

struct _GCR_CB
{
    QUEUE		q;
    PTR			ccb;		/* GCC Connection info */
    i4			state;		/* Current embedded interface state */
    i4			flags;

#define	GCR_F_RELEASE	0x01		/* GCA_RELEASE message processed */
#define	GCR_F_FS	0x02		/* Fast Select message present */
    u_i4		assoc_id;

    /*
    ** Info needed to process the current request.
    */
    SVC_PARMS		*svc_parms;	/* GC CL service parameters */
    GCA_PARMLIST	*parmlist;	/* GCA parameter list */
    PTR			mde;		/* GCC MDE */

    /*
    ** Buffers for info exchange.
    */
    GCA_PEER_INFO	peer;		/* Peer info */
    PTR			aux_data; 	/* GCA_AUX_DATA */
    GCA_SEC_LABEL_DATA	*sl;		/* Security Label */
    PTR			fs_data;	/* Fast Select info */
    u_i2		fs_len;

    /*
    ** Coordinate actions between GCA and GCC.
    */
    GCR_CONN_INFO	conn;
    GCR_SR_INFO		recv[ 2 ];	/* Normal and Expedited channels */
    GCR_SR_INFO		send[ 2 ];
};

/*
** States for the embedded Comm Server interface State Machine
*/

#define	GCR_S_INIT	0	/* Initial state */
#define	GCR_S_RQST	1	/* Connection requested */
#define GCR_S_SPEER	2	/* Peer info sent */
#define	GCR_S_FS	3	/* Fast Select info sent */
#define GCR_S_RPEER	4	/* Peer info requested */
#define	GCR_S_CONN	5	/* Connection completed */
#define	GCR_S_DISCON	6	/* Disconnect in progress */
#define	GCR_S_DONE	7	/* Disconnect completed */

#define	GCR_S_COUNT	8

/*
** Input events for the embedded Comm Server interface State Machine
*/

#define GCR_I_UNKNOWN	0	/* Unknown input */
#define GCR_I_RQST	1	/* GCR_REQUEST from GCA */
#define	GCR_I_SPEER	2	/* GCR_SENDPEER from GCA */
#define	GCR_I_FS	3	/* GCR_SEND of Fast Select info from GCA */
#define	GCR_I_RPEER	4	/* GCR_RECVPEER from GCA */
#define	GCR_I_RQRESP	5	/* GCR_RQRESP from embedded Comm Server */
#define GCR_I_RECEIVE	6	/* GCR_RECEIVE from GCA */
#define GCR_I_SEND	7	/* GCR_SEND from GCA */
#define	GCR_I_DATA	8	/* GCR_SEND from embedded Comm Server */
#define GCR_I_NEED	9	/* GCR_RECEIVE from embedded Comm Server */
#define GCR_I_PURGE	10	/* GCR_PURGE from GCA */
#define	GCR_I_DISCONN	11	/* GCR_DISCONN from GCA */
#define	GCR_I_ABORT	12	/* GCR_DISCONN from embedded Comm Server */
#define	GCR_I_INVALID	13	/* Input invalid for current state */

#define	GCR_I_COUNT	14

/*
** Actions for the embedded Comm Server interface State Machine
*/

#define	GCR_A_UNKNOWN	0	/* Unknown input */
#define	GCR_A_NOOP	1	/* Nothing to do, return OK */
#define	GCR_A_RQST	2	/* GCR_REQUEST from GCA */
#define	GCR_A_SPEER	3	/* GCR_SENDPEER from GCA */
#define	GCR_A_FS	4	/* GCR_SEND Fast Select info from GCA */
#define	GCR_A_RPEER	5	/* GCR_RECVPEER from GCA */
#define	GCR_A_RQRESP	6	/* GCR_RQRESP from embedded Comm Server */
#define GCR_A_RECEIVE	7	/* GCR_RECEIVE from GCA */
#define GCR_A_SEND	8	/* GCR_SEND from GCA */
#define	GCR_A_DATA	9	/* GCR_SEND from embedded Comm Server */
#define GCR_A_NEED	10	/* GCR_RECEIVE from embedded Comm Server */
#define GCR_A_PURGE	11	/* GCR_PURGE from GCA */
#define	GCR_A_DISCONN	12	/* First disconnect request */
#define	GCR_A_DONE	13	/* Second disconnect request */
#define	GCR_A_ERROR	14	/* Disconnect in progress, abort request */
#define	GCR_A_INVALID	15	/* Input invalid for current state */


/*
** Redirect GCA CL function calls from GCA and
** GCA calls from the Embedded Comm Server into
** the router.  Note that we don't do this for
** GCR source files since they need to call the
** original functions.
*/

#ifdef GCF_EMBEDDED_GCC

/*
** Router Global Data Area.
*/
GLOBALREF GCR_STATIC	*IIGCr_static;

#ifndef _GCR_INTERNAL_

#define	GCinitiate( alloc, free )	gcr_gca( GCR_INITIATE, NULL )
#define	GCregister( svc_parms )		gcr_gca( GCR_REGISTER, svc_parms )
#define	GClisten( svc_parms )		gcr_gca( GCR_LISTEN, svc_parms )
#define	GCrequest( svc_parms )		gcr_gca( GCR_REQUEST, svc_parms )
#define	GCsend( svc_parms )		gcr_gca( GCR_SEND, svc_parms )
#define	GCreceive( svc_parms )		gcr_gca( GCR_RECEIVE, svc_parms )
#define	GCpurge( svc_parms )		gcr_gca( GCR_PURGE, svc_parms )
#define	GCsave( svc_parms )		gcr_gca( GCR_SAVE, svc_parms )
#define	GCrestore( svc_parms )		gcr_gca( GCR_RESTORE, svc_parms )
#define	GCdisconn( svc_parms )		gcr_gca( GCR_DISCONN, svc_parms )
#define	GCrelease( svc_parms )		gcr_gca( GCR_RELEASE, svc_parms )
#define	GCterminate( svc_parms )	gcr_gca( GCR_TERMINATE, svc_parms )

/*
** We only want to redirect IIGCa_call inside
** GCC code since GCA defines/references this
** entry point.  We've already eliminated GCR
** code at this point, so we pick a suitable
** symbol from gcc.h to further limit the
** following definition to GCC.
*/
#ifdef GCC_STANDALONE

#define IIGCa_call			gcr_gcc

#endif /* GCC_STANDALONE */

#endif /* ! GCR_INTERNAL */

#endif /* GCF_EMBEDDED_GCC */



/*
** Function references.
*/
FUNC_EXTERN	STATUS		gcr_gca( i4  request, SVC_PARMS *svc_parms );

FUNC_EXTERN	STATUS		gcr_gcc( i4		service_code,
					 GCA_PARMLIST	*parmlist,
					 i4		indicators,
					 PTR		async_id,
					 i4	time_out,
					 STATUS		*status );

FUNC_EXTERN	STATUS		gcr_gcc_sm(GCR_CB *, i4  request, bool down );
FUNC_EXTERN	STATUS		gcr_term_gcc( VOID );

#endif /* ! _GCR_INCLUDED_ */
