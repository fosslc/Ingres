/*
** Copyright (c) 2008, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <gc.h>
#include <me.h>
#include <si.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcu.h>
#include <gcdint.h>

/*
** Name: gcdgca.c
**
** Description:
**	GCD server interface to GCA.
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	21-Dec-99 (gordy)
**	    Permit GCA_LISTEN to timeout and add actions to check
**	    for timeout and perform client idle checking.
**	17-Jan-00 (gordy)
**	    GCA_LISTEN must now timeout for connection pooling and
**	    client timeout.  Check times now verified by the check
**	    routines themselves.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**      24-May-04 (wansh01)
**          Support gcadm interface.
**      24-Aug-04 (wansh01)
**          Support gcadm show command.
**      24-Sept-04 (wansh01)
**          Added GCD_global.gcd_lcl_id (Server listen address) to support 
**	    SHOW SERVER command.
**	18-Oct-04 (wansh01)
**	    Moved Check user privilege for admin session to gca(gcaasync.c). 
**      05-Jun-08 (Ralph Loen)  SIR 120457
**          Moved startup message to gcd_gca_init() from main() in iigcd, to 
**          be consistent with the format of the GCC.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	21-Jul-09 (gordy)
**	    Remove restrictions on length of usernames.
**	 31-Mar-10 (gordy & rajus01) SD issue 142688, Bug 123270
**	    Moved the LSA_REPOST logic to gcd_gca_activate() to resolve the
**	    hang issue on VMS platform from an iimonitor session 
**	    after 2 successful attempts. 
**	24-Aug-10 (gordy)
**	    Register support for DAS MIB.
*/	

static	PTR		gca_cb = NULL;
static	bool		listening = FALSE;

/*
** GCA_LCB: Listen control block.
*/

typedef struct 
{

    i4			state;		/* Current state */
#define LCB_DEPTH       5               /* Stack depth */

    u_i1                sp;             /* State stack counter */
    u_i1                labels[LCB_DEPTH + 1];  /* State label stack */
    u_i2                ss[LCB_DEPTH];  /* State stack */

    u_i4		flags;

#define LCB_ADMIN       0x0001
#define LCB_QUIESCE     0x0002
#define LCB_SHUTDOWN    0x0004

    STATUS		stat;		/* Non-GCA status result */
    STATUS		*statusp;	/* Current result status */
    GCA_PARMLIST	parms;		/* GCA request parameters */
    i4			assoc_id;
    i4			protocol;
    char		*username;
    
} GCA_LCB;

/*
** This server only supports GCA connections
** to request server shutdown, so only a few
** listen control blocks are required to
** support Name Server bedchecks and GCM
** operations.
*/

#define	LCB_MAX		5

static GCA_LCB		gca_lcb_tab[ LCB_MAX ] ZERO_FILL; 
static char 		mbuf[ GCD_MSG_BUFF_LEN ] ZERO_FILL; 

/*
** State actions.
*/

#define	LSA_LABEL	0	/* label */
#define	LSA_INIT	1	/* initialize */
#define	LSA_GOTO	2	/* goto unconditionally */
#define	LSA_GOSUB	3	/* call subroutine */
#define	LSA_RETURN	4	/* return from subroutine */
#define	LSA_EXIT	5	/* terminate thread */
#define	LSA_IF_RESUME	6	/* if INCOMPLETE */
#define LSA_IF_TIMEOUT	7	/* if TIMEOUT */
#define	LSA_IF_ERROR	8	/* if status not OK */
#define	LSA_IF_SHUT	9	/* if SHUTDOWN requested */
#define	LSA_SET_SHUT	10	/* SHUTDOWN server */
#define	LSA_CLEAR_ERR	11	/* set status to OK */
#define	LSA_LOG		12	/* log an error */
#define	LSA_CHECK	13	/* Background checks */
#define	LSA_LISTEN	14	/* post a listen */
#define	LSA_LS_RESUME	15	/* resume a listen */
#define	LSA_REPOST	16	/* repost listen */
#define	LSA_LS_DONE	17	/* complete a listen */
#define	LSA_NEGOTIATE	18	/* validate client request */
#define	LSA_REJECT	19	/* reject client request */
#define	LSA_RQRESP	20	/* respond to a request */
#define	LSA_DISASSOC	21	/* disassociate */
#define	LSA_DA_RESUME	22	/* resume a disassoc */
#define LSA_IF_ADMIN	23      /* monitor command */
#define LSA_ADM_SESS	24      /* execute monitor command*/
#define LSA_ADM_RLS	25      /* format release msg */
#define LSA_SEND	26	/* send msg */
#define LSA_CLEAR_USR	27	/* clear user thread */


static	char	*lsn_sa_names[] = 
{
  "LSA_LABEL",      "LSA_INIT",       "LSA_GOTO", 
  "LSA_GOSUB",      "LSA_RETURN",     "LSA_EXIT",
  "LSA_IF_RESUME",  "LSA_IF_TIMEOUT", "LSA_IF_ERROR",   
  "LSA_IF_SHUT",    "LSA_SET_SHUT",   
  "LSA_CLEAR_ERR",  "LSA_LOG",        "LSA_CHECK",
  "LSA_LISTEN",     "LSA_LS_RESUME",  "LSA_REPOST",     
  "LSA_LS_DONE",    "LSA_NEGOTIATE",  "LSA_REJECT",
  "LSA_RQRESP",     "LSA_DISASSOC",   "LSA_DA_RESUME",  
  "LSA_IF_ADMIN",   "LSA_ADM_SESS",   "LSA_ADM_RLS",
  "LSA_SEND",       "LSA_CLEAR_USR"
};


/*
** State labels
*/

# define	LBL_INIT	0
# define	LBL_LS_CHK	1
# define	LBL_LS_RESUME	2
# define	LBL_SHUTDOWN	3
# define	LBL_RESPOND	4
# define	LBL_ERROR	5
# define	LBL_DONE	6
# define	LBL_DA_CHK	7
# define	LBL_DA_RESUME	8
# define	LBL_EXIT	9
# define        LBL_ADM_RQST    10
# define        LBL_ADM_ERR     11


# define	LBL_MAX		12
# define        LBL_NONE        LBL_MAX

static	i4	labels[ LBL_MAX ] ZERO_FILL;

/*
** Each label has a position in the state table,
** which is determined during initialization, and
** a description of the general processing state
** associated with the label.
*/

static struct
{
    i2		state;
    char	*desc;
} gcd_labels[] =
{
    { -1,	"LISTEN" },		/* LBL_INIT */
    { -1,	"PROCESS" },		/* LBL_LS_CHK */
    { -1,	"PROCESS" },		/* LBL_LS_RESUME */
    { -1,	"SHUTDOWN" },		/* LBL_SHUTDOWN */
    { -1,	"RESPOND" },		/* LBL_RESPOND */
    { -1,	"DONE" },		/* LBL_ERROR */
    { -1,	"DONE" },		/* LBL_DONE */
    { -1,	"DISCONNECT" },		/* LBL_DA_CHK */
    { -1,	"DISCONNECT" },		/* LBL_DA_RESUME */
    { -1,	"EXIT" },		/* LBL_EXIT */
    { -1,	"ADMIN" },		/* LBL_ADM_RQST */
    { -1,	"ADMIN" },		/* LBL_ADM_ERR */
};

/*
** Name: lsn_states
**
** Description:
**	Each state in GCN has one action and one branch state identified
**	by a label.  If a state action does not branch, execution falls
**	through to the subsequent state
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Check for GCA_LISTEN timeout and add CHECK action at exit.
*/

static struct 
{

  i4	action;
  i4	label;

} lsn_states[] = 
{
	/*
	** Initialize
	*/
	LSA_LABEL,	LBL_INIT,

		LSA_CLEAR_ERR,	LBL_NONE,
		LSA_INIT,	LBL_EXIT,		/* If listening */

	/* 
	** Post and process GCA_LISTEN
	*/

		LSA_LISTEN,	LBL_NONE,
		LSA_REPOST,	LBL_NONE,

	LSA_LABEL,	LBL_LS_CHK,

		LSA_IF_RESUME,	LBL_LS_RESUME,	/* if INCOMPLETE */
		LSA_LS_DONE,	LBL_NONE,	
		LSA_IF_TIMEOUT,	LBL_DONE,
		LSA_IF_ERROR,	LBL_ERROR,
		LSA_NEGOTIATE,	LBL_NONE,
		LSA_IF_SHUT,	LBL_SHUTDOWN,	/* if SHUTDOWN requested */
		LSA_IF_ADMIN,	LBL_ADM_RQST,	/* if ADMIN request */
		LSA_REJECT,	LBL_NONE,
		LSA_GOTO,	LBL_RESPOND,

	/*
	** Resume a GCA_LISTEN request
	*/

	LSA_LABEL,	LBL_LS_RESUME,

		LSA_LS_RESUME,	LBL_NONE,
		LSA_GOTO,	LBL_LS_CHK,

	/* 
	** Shutdown requested.
	*/

	LSA_LABEL,	LBL_SHUTDOWN,

		LSA_SET_SHUT,	LBL_NONE,
		LSA_GOTO,	LBL_RESPOND,

	/* 
	** ADMIN  request.
	*/

	LSA_LABEL,	LBL_ADM_RQST,

		LSA_RQRESP,	LBL_NONE,
		LSA_IF_ERROR,   LBL_ERROR,
		LSA_ADM_SESS,	LBL_ADM_ERR,
		LSA_GOTO,	LBL_EXIT, 

	LSA_LABEL,	LBL_ADM_ERR,

		LSA_ADM_RLS,	LBL_NONE, 	/* format gca_release msg */
		LSA_SEND,	LBL_NONE,
		LSA_IF_ERROR,	LBL_ERROR, 
		LSA_GOTO,	LBL_DONE, 

	/* 
	** Respond to client request.
	*/

	LSA_LABEL,	LBL_RESPOND,

		LSA_RQRESP,	LBL_NONE,
		LSA_IF_ERROR,	LBL_ERROR,
		LSA_GOTO,	LBL_DONE, 

	/* 
	** Log error, disconnect and exit.
	*/

	LSA_LABEL,	LBL_ERROR,

		LSA_LOG,	LBL_NONE,
		LSA_CLEAR_ERR,	LBL_NONE,

	/* 
	** Disconnect and exit.
	*/

	LSA_LABEL,	LBL_DONE,

		LSA_DISASSOC,	LBL_NONE,

	LSA_LABEL,	LBL_DA_CHK,

		LSA_IF_RESUME,	LBL_DA_RESUME, 	/* if INCOMPLETE */
		LSA_LOG,	LBL_NONE,
		LSA_GOTO,	LBL_EXIT,

	/*
	** Resume a GCA_LISTEN request
	*/

	LSA_LABEL,	LBL_DA_RESUME,

		LSA_DA_RESUME,	LBL_NONE,
		LSA_GOTO,	LBL_DA_CHK, 

	/*
	** Terminate thread.
	*/

	LSA_LABEL,	LBL_EXIT,

		LSA_CHECK,	LBL_NONE,
		LSA_CLEAR_ERR,	LBL_NONE,
		LSA_CLEAR_USR,	LBL_NONE,
		LSA_EXIT,	LBL_NONE,

};


/*
** Name: gca_sm
**
** Description:
**	State machine processing loop for the GCD server GCA
**	listen processing.
**
** Inputs:
**	sid	ID of session to activate.
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Permit GCA_LISTEN to timeout and add actions to check
**	    for timeout and perform client idle checking.
**	17-Jan-00 (gordy)
**	    GCA_LISTEN must now timeout for connection pooling and
**	    client timeout.  Check times now verified by the check
**	    routines themselves.
**	21-Jul-09 (gordy)
**	    Dynamically allocate username.
*/

void
gca_sm( i4 sid )
{
    GCA_LCB 	*lcb = &gca_lcb_tab[ sid ];
    bool 	branch = FALSE;

top:

    if ( 
	 GCD_global.gcd_trace_level >= 5  &&
	 lsn_states[ lcb->state ].action != LSA_LABEL  &&
	 lsn_states[ lcb->state ].action != LSA_GOTO
       )
	TRdisplay( "%4d    GCD  %s status 0x%x %s\n", sid, 
		   lsn_sa_names[ lsn_states[ lcb->state ].action ],
		   lcb->statusp ? *lcb->statusp : OK,
		   branch ? " (branch)" : "" );

    branch = FALSE;

    switch( lsn_states[ lcb->state++ ].action )
    {
    	case LSA_INIT :			/* Initialize, branch if listening */
	    branch = listening;
	    lcb->sp = 0;
	    lcb->flags = 0;
	    lcb->username = NULL;

	    if ( ! labels[ 1 ] )
	    {
		i4 i;

		for( i = 0; i < ARR_SIZE( lsn_states ); i++ )
		    if ( lsn_states[ i ].action == LSA_LABEL )
			labels[ lsn_states[ i ].label ] = i + 1;
	    }
	    break;

	case LSA_GOTO : 		/* Branch unconditionally */
	    branch = TRUE;
	    break;

	case LSA_GOSUB :		/* Call subroutine */
	    lcb->ss[ lcb->sp++ ] = lcb->state;
	    branch = TRUE;
	    break;
	
	case LSA_RETURN :		/* Return from subroutine */
	    lcb->state = lcb->ss[ --lcb->sp ];
	    break;

	case LSA_EXIT :			/* Terminate thread */

	    if ( GCD_global.flags & GCD_SVR_QUIESCE )
	    {
	    /*
	    ** There should be no admin sessions.  There
	    ** may be a session listening and there will
	    ** always be the current session which is
	    ** in the process of exiting.
	    */
	    i4 user_count = gcd_sess_info( NULL, GCD_USER_COUNT );
            i4 sys_count = gcd_sess_info( NULL, GCD_SYS_COUNT );
            i4 count = user_count + sys_count;


	    if ( GCD_global.adm_sess_count < 1  &&
				 count <= ( listening ? 2 : 1) )
		 GCD_global.flags |= GCD_SVR_SHUT;
	    }
	    /*
	    ** Exit if there shutting down or there is an
	    ** active listen.  Otherwise, the current
	    ** thread continues as the new listen thread.
	    */
	    /*
	    ** Shutdown the DAS Server.
	    */
	    if ( GCD_global.flags & GCD_SVR_SHUT )
		GCshut();

	    if ( listening )
	    {
	    /*
	    ** Terminating thread.
	    */
		lcb->state = 0;
	        return;
	    }

	    break;

	case LSA_IF_RESUME :		/* Branch if INCOMPLETE */
	    branch = (*lcb->statusp == E_GCFFFE_INCOMPLETE);
	    break;

	case LSA_IF_TIMEOUT :		/* Branch if TIMEOUT */
	    branch = (*lcb->statusp == E_GC0020_TIME_OUT);
	    break;

	case LSA_IF_ERROR :		/* Branch if status not OK */
	    branch = (*lcb->statusp != OK);
	    break;

	case LSA_IF_SHUT :		/* Branch if SHUTDOWN requested */
	    branch = ((lcb->flags & (LCB_QUIESCE | LCB_SHUTDOWN)) != 0);

	    break;

	case LSA_SET_SHUT:		/* Prepare to shutdown server */
	    if ( lcb->flags & LCB_QUIESCE )
		GCD_global.flags |= GCD_SVR_QUIESCE | GCD_SVR_CLOSED; 
	   
	    if ( lcb->flags & LCB_SHUTDOWN )
		GCD_global.flags |= GCD_SVR_SHUT | GCD_SVR_CLOSED; 

	    GCshut();
	    lcb->stat = E_GC0040_CS_OK;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_IF_ADMIN :		/* Branch if ADMIN request */
	    /*
	    ** Branch if admin session requested.
	    */
	    if (lcb->flags & LCB_ADMIN) branch = TRUE;
	    break;

       	
	case LSA_ADM_SESS: 
	    /*
	    ** Start an admin session.
	    */
	    lcb->statusp = &lcb->stat;
	    lcb->stat = gcd_adm_session( lcb->parms.gca_ls_parms.gca_assoc_id,
				gca_cb, lcb->username ? lcb->username 
						      : "<unknown>" );

	    if ( lcb->stat != OK )  branch = TRUE;
	    break;

	case LSA_ADM_RLS: 
	    /*
	    ** format gca_release message 
	    */
	    {
		char   *er_data;
    		i4     one = 1, zero = 0;
    		i4     data_type = DB_CHA_TYPE;
    		i4     ss_code=50000, severity = GCA_ERFORMATTED;

		MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms);
		lcb->parms.gca_sd_parms.gca_message_type = GCA_RELEASE;
    		/* 
    		** Format GCA_ER_DATA object  
    		*/
    		er_data = (char *) mbuf;
		er_data += GCA_PUTI4_MACRO( &one, er_data);/* gca_l_e_element */
		er_data += GCA_PUTI4_MACRO( &ss_code, er_data );/* gca_id_error */ 
		er_data += GCA_PUTI4_MACRO( &lcb->statusp, er_data); /* gca_id_server */
		er_data += GCA_PUTI4_MACRO( &zero, er_data); /* gca_server_type */
		er_data += GCA_PUTI4_MACRO( &severity, er_data );/* gca_serverity */
		er_data += GCA_PUTI4_MACRO( &zero, er_data);/* gca_local_error */
		lcb->parms.gca_sd_parms.gca_msg_length = (er_data - (char *)mbuf);
	    	break;
	    }

	case LSA_CLEAR_ERR :		/* Set status to OK */
	    lcb->stat = OK;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_CLEAR_USR : 
	    if ( lcb->username )
	    {
	    	MEfree( (PTR)lcb->username );
		lcb->username = NULL;
	    }
	    break;

	case LSA_LOG :			/* Log an error */
	    if ( *lcb->statusp != OK  &&  *lcb->statusp != FAIL  &&
		 *lcb->statusp != E_GC0032_NO_PEER )
	        gcu_erlog( 0, GCD_global.language, 
			   *lcb->statusp, NULL, 0, NULL );
	    break;

	case LSA_CHECK :		/* Background checks */
	    gcd_idle_check();
	    gcd_pool_check();
	    break;

	case LSA_LISTEN:			/* Post a listen */
	    {
		i4	timeout = -1;
		SYSTIME now;

		MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms );
		lcb->statusp = &lcb->parms.gca_ls_parms.gca_status;
		listening = TRUE;
		TMnow( &now );

		if ( GCD_global.client_idle_limit )
		{
		    i4 secs;

		    if ( GCD_global.client_check.TM_secs <= 0 )
			secs = GCD_global.client_idle_limit;
		    else  if ( TMcmp( &now, &GCD_global.client_check ) < 0 )
			secs = GCD_global.client_check.TM_secs - now.TM_secs;
		    else
			secs = GCD_global.client_idle_limit / 2;

		    if ( timeout <= 0  ||  secs < timeout )  timeout = secs;
		}

		if ( GCD_global.pool_idle_limit )
		{
		    i4 secs;

		    if ( GCD_global.pool_check.TM_secs <= 0 )
			secs = GCD_global.pool_idle_limit;
		    else  if ( TMcmp( &now, &GCD_global.pool_check ) < 0 )
			secs = GCD_global.pool_check.TM_secs - now.TM_secs;
		    else
			secs = GCD_global.pool_idle_limit / 2;

		    if ( timeout <= 0  ||  secs < timeout )  timeout = secs;
		}

		/*
		** If there is a timeout, leave some
		** lee-way to ensure we actually pass
		** the target check time.  Also, convert
		** seconds to milli-seconds.
		*/
		if ( timeout >= 0 )  timeout = (timeout + 10) * 1000;

		IIGCa_cb_call( &gca_cb, GCA_LISTEN, &lcb->parms, 
			       GCA_ASYNC_FLAG, (PTR)(SCALARP)sid, timeout, &lcb->stat );
		if ( lcb->stat != OK )  break;
	    }
	    return;

	case LSA_LS_RESUME :		/* Resume a listen */
	    IIGCa_cb_call( &gca_cb, GCA_LISTEN, &lcb->parms, 
			   GCA_RESUME, (PTR)(SCALARP)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;

	case LSA_REPOST:		/* Repost a listen */
	    listening = FALSE;
	    gcd_gca_activate();
	    break;

	case LSA_LS_DONE :		/* Listen request has completed */
	    lcb->assoc_id = lcb->parms.gca_ls_parms.gca_assoc_id;
	    break;

	case LSA_NEGOTIATE :		/* Validate client */
	    lcb->protocol = min( lcb->parms.gca_ls_parms.gca_partner_protocol,
				 GCD_GCA_PROTO_LVL );

	    if ( lcb->parms.gca_ls_parms.gca_user_name  &&
	         *lcb->parms.gca_ls_parms.gca_user_name )
		lcb->username = STalloc(lcb->parms.gca_ls_parms.gca_user_name);

	    /*
	    ** Check for shutdown/quiesce request.
	    */
	    while( lcb->parms.gca_ls_parms.gca_l_aux_data > 0 )
	    {
		GCA_AUX_DATA	aux_hdr;
		char		*aux_data;
		i4		aux_len;

		MEcopy( lcb->parms.gca_ls_parms.gca_aux_data,
			sizeof( aux_hdr ), (PTR)&aux_hdr );
		aux_data = (char *)lcb->parms.gca_ls_parms.gca_aux_data +
			   sizeof( aux_hdr );
		aux_len = aux_hdr.len_aux_data - sizeof( aux_hdr );

		switch( aux_hdr.type_aux_data )
		{
		    case GCA_ID_QUIESCE :
			lcb->flags |= LCB_QUIESCE;
			break;
		    case GCA_ID_SHUTDOWN :
			lcb->flags |= LCB_SHUTDOWN;
			break;

		    case GCA_ID_CMDSESS :
			lcb->flags |= LCB_ADMIN;
			break;
		}

		lcb->parms.gca_ls_parms.gca_aux_data = 
						(PTR)(aux_data + aux_len);
		lcb->parms.gca_ls_parms.gca_l_aux_data -= 
						aux_hdr.len_aux_data;
	    }
	    break;

	case LSA_REJECT :		/* Reject a client request */
	    lcb->stat = E_GC480B_NO_CLIENTS;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_SEND:		/* Send gca message  */
	    lcb->parms.gca_sd_parms.gca_assoc_id = lcb->assoc_id;
	    lcb->statusp = &lcb->parms.gca_sd_parms.gca_status;

	    IIGCa_cb_call( &gca_cb, GCA_SEND, &lcb->parms, 
			   GCA_ASYNC_FLAG, (PTR)(SCALARP)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;

	case LSA_RQRESP :		/* Respond to client request */
            MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms);
	    lcb->parms.gca_rr_parms.gca_assoc_id = lcb->assoc_id;
	    lcb->parms.gca_rr_parms.gca_request_status = *lcb->statusp;
	    lcb->parms.gca_rr_parms.gca_local_protocol = lcb->protocol;
	    lcb->statusp = &lcb->parms.gca_rr_parms.gca_status;

	    IIGCa_cb_call( &gca_cb, GCA_RQRESP, &lcb->parms, 
			   GCA_ASYNC_FLAG, (PTR)(SCALARP)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;

	case LSA_DISASSOC :		/* Disconnect association */
            MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms );
	    lcb->parms.gca_da_parms.gca_association_id = lcb->assoc_id;
	    lcb->statusp = &lcb->parms.gca_da_parms.gca_status;

	    IIGCa_cb_call( &gca_cb, GCA_DISASSOC, &lcb->parms, 
			   GCA_ASYNC_FLAG, (PTR)(SCALARP)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK ) break;
	    return;

	case LSA_DA_RESUME :		/* Resume disassociate */
	    IIGCa_cb_call( &gca_cb, GCA_DISASSOC, &lcb->parms, 
			   GCA_RESUME, (PTR)(SCALARP)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;
    }

    if ( branch )  lcb->state = labels[ lsn_states[ lcb->state - 1 ].label ];

    goto top;
}


/*
** Name: gcd_gca_activate
**
** Description:
**      Search for an inactive session and activate the first one found.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      bool  ( TRUE or FALSE )
**
** History:
**       29-Mar-10 (gordy & rajus01)
**          Created.
*/
bool
gcd_gca_activate()
{
    i4 id;

    for( id = 0; id < LCB_MAX; id++ )
        if ( ! gca_lcb_tab[ id ].state )
        {
            gca_sm( id );
            return( TRUE );
        }
    return( FALSE );
}


/*
** Name: gcd_gca_init
**
** Description:
**	Initialize the GCD server GCA interface.
**
** Inputs:
**	None.
**
** Outputs:
**	GCD_global.gcd_lcl_id		Server listen address.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	24-Aug-10 (gordy)
**	    Register support for DAS MIB.
*/

STATUS
gcd_gca_init( )
{
    GCA_PARMLIST	parms;
    STATUS		status;
    char                buffer[512];

    MEfill( sizeof(parms), 0, (PTR) &parms);
    parms.gca_in_parms.gca_local_protocol = GCD_GCA_PROTO_LVL;
    parms.gca_in_parms.gca_normal_completion_exit = gca_sm;
    parms.gca_in_parms.gca_expedited_completion_exit = gca_sm;
    parms.gca_in_parms.gca_modifiers |= GCA_API_VERSION_SPECD;
    parms.gca_in_parms.gca_api_version = GCA_API_LEVEL_5;

    IIGCa_cb_call( &gca_cb, GCA_INITIATE, 
		   &parms, GCA_SYNC_FLAG, NULL, -1, &status );
			
    if ( status != OK || (status = parms.gca_in_parms.gca_status) != OK )
    {
	gcu_erlog( 0, GCD_global.language, status, 
		   &parms.gca_in_parms.gca_os_status, 0, NULL );
	return( status );
    }

    MEfill( sizeof(parms), 0, (PTR) &parms);
    parms.gca_rg_parms.gca_srvr_class = GCD_SRV_CLASS;
    parms.gca_rg_parms.gca_l_so_vector = 0;
    parms.gca_rg_parms.gca_served_object_vector = NULL;
    parms.gca_rg_parms.gca_installn_id = "";
    parms.gca_rg_parms.gca_modifiers = GCA_RG_DASVR;
	    
    IIGCa_cb_call( &gca_cb, GCA_REGISTER, 
		   &parms, GCA_SYNC_FLAG, NULL, -1, &status );

    if ( status != OK || (status = parms.gca_rg_parms.gca_status) != OK )
    {
	gcu_erlog( 0, GCD_global.language, 
		   status, &parms.gca_rg_parms.gca_os_status, 0, NULL );
	return( status );
    }

    gca_sm( 0 );
    STcopy( parms.gca_rg_parms.gca_listen_id, GCD_global.gcd_lcl_id );
    STprintf(buffer, "\nDAS Server = %s\n", GCD_global.gcd_lcl_id);
    SIstd_write(SI_STD_OUT, buffer);

    return( OK );
}


/*
** Name: gcd_gca_term
**
** Description:
**	Shutdown the GCA interface.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
*/

void
gcd_gca_term( void )
{
    GCA_PARMLIST	parms;
    STATUS		status;

    MEfill( sizeof(parms), 0, (PTR) &parms);
    IIGCa_cb_call( &gca_cb, GCA_TERMINATE, 
		   &parms, GCA_SYNC_FLAG, NULL, -1, &status );
			
    return;
}
