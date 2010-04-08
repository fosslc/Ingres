/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <gc.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcu.h>
#include <jdbc.h>

/*
** Name: jdbcgca.c
**
** Description:
**	JDBC server interface to GCA.
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
*/	

static	PTR		gca_cb = NULL;
static	bool		listening = FALSE;

/*
** GCA_LCB: Listen control block.
*/

typedef struct 
{

    i4			state;		/* Current state */
    i4			sp;		/* State stack counter */
    i4			ss[ 5 ];	/* State stack */
    u_i4		flags;

#define	LCB_SHUTDOWN	0x0001

    STATUS		stat;		/* Non-GCA status result */
    STATUS		*statusp;	/* Current result status */
    GCA_PARMLIST	parms;		/* GCA request parameters */
    i4			assoc_id;
    i4			protocol;
    
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
};


/*
** State labels
*/

# define	LBL_NONE	0
# define	LBL_LS_CHK	1
# define	LBL_LS_RESUME	2
# define	LBL_SHUTDOWN	3
# define	LBL_RESPOND	4
# define	LBL_ERROR	5
# define	LBL_DONE	6
# define	LBL_DA_CHK	7
# define	LBL_DA_RESUME	8
# define	LBL_EXIT	9

# define	LBL_MAX		10

static	i4	labels[ LBL_MAX ] ZERO_FILL;


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
		LSA_EXIT,	LBL_NONE,

};


/*
** Name: gca_sm
**
** Description:
**	State machine processing loop for the JDBC server GCA
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
*/

static void
gca_sm( i4 sid )
{
    GCA_LCB 	*lcb = &gca_lcb_tab[ sid ];
    bool 	branch = FALSE;

top:

    if ( 
	 JDBC_global.trace_level >= 5  &&
	 lsn_states[ lcb->state ].action != LSA_LABEL  &&
	 lsn_states[ lcb->state ].action != LSA_GOTO
       )
	TRdisplay( "%4d    JDBC  %s status 0x%x %s\n", sid, 
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
	    lcb->state = 0;	/* Initialize state */

	    /*
	    ** Exit if there shutting down or there is an
	    ** active listen.  Otherwise, the current
	    ** thread continues as the new listen thread.
	    */
	    if ( lcb->flags & LCB_SHUTDOWN  ||  listening )  return;
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
	    branch = (lcb->flags & LCB_SHUTDOWN) ? TRUE : FALSE;
	    break;

	case LSA_SET_SHUT:		/* Prepare to shutdown server */
	    GCshut();
	    lcb->stat = E_GC0040_CS_OK;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_CLEAR_ERR :		/* Set status to OK */
	    lcb->stat = OK;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_LOG :			/* Log an error */
	    if ( *lcb->statusp != OK  &&  *lcb->statusp != FAIL  &&
		 *lcb->statusp != E_GC0032_NO_PEER )
	        gcu_erlog( 0, JDBC_global.language, 
			   *lcb->statusp, NULL, 0, NULL );
	    break;

	case LSA_CHECK :		/* Background checks */
	    jdbc_idle_check();
	    jdbc_pool_check();
	    break;

	case LSA_LISTEN:			/* Post a listen */
	    {
		i4	timeout = -1;
		SYSTIME now;

		MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms );
		lcb->statusp = &lcb->parms.gca_ls_parms.gca_status;
		listening = TRUE;
		TMnow( &now );

		if ( JDBC_global.client_idle_limit )
		{
		    i4 secs;

		    if ( JDBC_global.client_check.TM_secs <= 0 )
			secs = JDBC_global.client_idle_limit;
		    else  if ( TMcmp( &now, &JDBC_global.client_check ) < 0 )
			secs = JDBC_global.client_check.TM_secs - now.TM_secs;
		    else
			secs = JDBC_global.client_idle_limit / 2;

		    if ( timeout <= 0  ||  secs < timeout )  timeout = secs;
		}

		if ( JDBC_global.pool_idle_limit )
		{
		    i4 secs;

		    if ( JDBC_global.pool_check.TM_secs <= 0 )
			secs = JDBC_global.pool_idle_limit;
		    else  if ( TMcmp( &now, &JDBC_global.pool_check ) < 0 )
			secs = JDBC_global.pool_check.TM_secs - now.TM_secs;
		    else
			secs = JDBC_global.pool_idle_limit / 2;

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
			       GCA_ASYNC_FLAG, (PTR)sid, timeout, &lcb->stat );
		if ( lcb->stat != OK )  break;
	    }
	    return;

	case LSA_LS_RESUME :		/* Resume a listen */
	    IIGCa_cb_call( &gca_cb, GCA_LISTEN, &lcb->parms, 
			   GCA_RESUME, (PTR)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;

	case LSA_REPOST:		/* Repost a listen */
	    {
		i4 id;

		listening = FALSE;

		for( id = 0; id < LCB_MAX; id++ )
		    if ( ! gca_lcb_tab[ id ].state )
		    {
			gca_sm( id );
			break;
		    }
	    }
	    break;

	case LSA_LS_DONE :		/* Listen request has completed */
	    lcb->assoc_id = lcb->parms.gca_ls_parms.gca_assoc_id;
	    break;

	case LSA_NEGOTIATE :		/* Validate client */
	    lcb->protocol = min( lcb->parms.gca_ls_parms.gca_partner_protocol,
				 JDBC_GCA_PROTO_LVL );

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
		    case GCA_ID_SHUTDOWN :
			lcb->flags |= LCB_SHUTDOWN;
			break;
		}

		lcb->parms.gca_ls_parms.gca_aux_data = 
						(PTR)(aux_data + aux_len);
		lcb->parms.gca_ls_parms.gca_l_aux_data -= 
						aux_hdr.len_aux_data;
	    }
	    break;

	case LSA_REJECT :		/* Reject a client request */
	    lcb->stat = E_JD010B_NO_CLIENTS;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_RQRESP :		/* Respond to client request */
            MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms);
	    lcb->parms.gca_rr_parms.gca_assoc_id = lcb->assoc_id;
	    lcb->parms.gca_rr_parms.gca_request_status = *lcb->statusp;
	    lcb->parms.gca_rr_parms.gca_local_protocol = lcb->protocol;
	    lcb->statusp = &lcb->parms.gca_rr_parms.gca_status;

	    IIGCa_cb_call( &gca_cb, GCA_RQRESP, &lcb->parms, 
			   GCA_ASYNC_FLAG, (PTR)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;

	case LSA_DISASSOC :		/* Disconnect association */
            MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms );
	    lcb->parms.gca_da_parms.gca_association_id = lcb->assoc_id;
	    lcb->statusp = &lcb->parms.gca_da_parms.gca_status;

	    IIGCa_cb_call( &gca_cb, GCA_DISASSOC, &lcb->parms, 
			   GCA_ASYNC_FLAG, (PTR)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK ) break;
	    return;

	case LSA_DA_RESUME :		/* Resume disassociate */
	    IIGCa_cb_call( &gca_cb, GCA_DISASSOC, &lcb->parms, 
			   GCA_RESUME, (PTR)sid, -1, &lcb->stat );
	    if ( lcb->stat != OK )  break;
	    return;
    }

    if ( branch )  lcb->state = labels[ lsn_states[ lcb->state - 1 ].label ];

    goto top;
}


/*
** Name: jdbc_gca_init
**
** Description:
**	Initialize the JDBC server GCA interface.
**
** Inputs:
**	None.
**
** Outputs:
**	myaddr		Server listen address.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
*/

STATUS
jdbc_gca_init( char *myaddr )
{
    GCA_PARMLIST	parms;
    STATUS		status;

    MEfill( sizeof(parms), 0, (PTR) &parms);
    parms.gca_in_parms.gca_local_protocol = JDBC_GCA_PROTO_LVL;
    parms.gca_in_parms.gca_normal_completion_exit = gca_sm;
    parms.gca_in_parms.gca_expedited_completion_exit = gca_sm;
    parms.gca_in_parms.gca_modifiers |= GCA_API_VERSION_SPECD;
    parms.gca_in_parms.gca_api_version = GCA_API_LEVEL_5;

    IIGCa_cb_call( &gca_cb, GCA_INITIATE, 
		   &parms, GCA_SYNC_FLAG, NULL, -1, &status );
			
    if ( status != OK || (status = parms.gca_in_parms.gca_status) != OK )
    {
	gcu_erlog( 0, JDBC_global.language, status, 
		   &parms.gca_in_parms.gca_os_status, 0, NULL );
	return( status );
    }

    MEfill( sizeof(parms), 0, (PTR) &parms);
    parms.gca_rg_parms.gca_srvr_class = JDBC_SRV_CLASS;
    parms.gca_rg_parms.gca_l_so_vector = 0;
    parms.gca_rg_parms.gca_served_object_vector = NULL;
    parms.gca_rg_parms.gca_installn_id = "";
	    
    IIGCa_cb_call( &gca_cb, GCA_REGISTER, 
		   &parms, GCA_SYNC_FLAG, NULL, -1, &status );

    if ( status != OK || (status = parms.gca_rg_parms.gca_status) != OK )
    {
	gcu_erlog( 0, JDBC_global.language, 
		   status, &parms.gca_rg_parms.gca_os_status, 0, NULL );
	return( status );
    }

    gca_sm( 0 );
    STcopy( parms.gca_rg_parms.gca_listen_id, myaddr );

    return( OK );
}


/*
** Name: jdbc_gca_term
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
jdbc_gca_term( void )
{
    GCA_PARMLIST	parms;
    STATUS		status;

    MEfill( sizeof(parms), 0, (PTR) &parms);
    IIGCa_cb_call( &gca_cb, GCA_TERMINATE, 
		   &parms, GCA_SYNC_FLAG, NULL, -1, &status );
			
    return;
}

