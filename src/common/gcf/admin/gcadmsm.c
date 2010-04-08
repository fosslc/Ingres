/*
**
** This is an unpublished work containing confidential and proprietary
** information of Computer Associates.  Use, disclosure, reproduction,
** or transfer of this work without the express written consent of
** Computer Associates is prohibited.
*/


#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <sp.h>
#include <mo.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gc.h>
#include <gcaint.h>
#include <sqlstate.h>
#include <gcu.h>
#include <gcadm.h>
#include <gcadmint.h>

/*
** Name: gcadmsm.c 
**
** Description:
**	an interface with GCF servers and iimonitor. 
**
** History:
**	 9-10-2003 (wansh01) 
**	    Created.
/*
** Forward references.
*/


GLOBALDEF GCADM_GLOBAL  GCADM_global ZERO_FILL;

/* 
**
** GCADM FSM definitions.
*/

#define	ADMS_IDLE	0		/* FSM initial/final state*/
#define	ADMS_ACPT 	1		/* accept connection    */
#define	ADMS_RECVMD	2		/* receive MD_ASSOC 	*/
#define	ADMS_ACPTMD	3		/* accept MD_ASSOC msg 	*/
#define	ADMS_GETRQST	4		/* get client request	*/
#define	ADMS_RQST	5		/* requests active	*/
#define	ADMS_RSLT	6		/* return result	*/
#define	ADMS_CMPL	7		/* return result complete */
#define	ADMS_RLSE	8		/* release connection 	*/
#define	ADMS_DISC	9		/* GCA_DISASSOC active	*/

#define	ADMS_CNT	10

static	char	*states[ ADMS_CNT ] =
{
    "ADMS_IDLE", "ADMS_ACPT", "ADMS_RECVMD",
    "ADMS_ACPTMD", "ADMS_GETRQST", "ADMS_RQST", "ADMS_RSLT", 
    "ADMS_CMPL", "ADMS_RLSE", "ADMS_DISC", 
};


#define	ADMI_NEW_SESSION	0	/* client conencting..  */
#define ADMI_SEND_CMPL		1	/* GCA_SEND complete	*/
#define	ADMI_RECV_MDASSOC	2	/* received GCA_MD_ASSOC msg	*/
#define	ADMI_RECV_RELEASE	3	/* received GCA_RELEASE msg	*/
#define	ADMI_RECV_QUERY		4	/* received GCA_QUERY msg	*/
#define	ADMI_RSLT		5	/* return result to client*/
#define	ADMI_TUPL		6	/* return result to client*/
#define	ADMI_RQST_CMPL		7	/* request complete */
#define	ADMI_END_SESSION	8	/* disconnecting... */
#define ADMI_RQST_FAIL		9       /*  send or receive failed */
#define ADMI_RECV_INVALID	10      /*  receive invalid */
#define ADMI_DISC_DONE		11      /*  disconnect done */


#define	ADMI_CNT		12

static	char	*events[ ADMI_CNT ] =
{
	"ADMI_NEW_SESSION", "ADMI_SEND_CMPL", 
	"ADMI_RECV_MDASSOC", "ADMI_RECV_RELEASE", 
	"ADMI_RECV_QUERY",  "ADMI_RSLT", 
	"ADMI_TUPL", "ADMI_RQST_CMPL", 
	"ADMI_END_SESSION", "ADMI_RQST_FAIL", 
	"ADMI_RECV_INVALID", "ADMI_DISC_DONE",
};



#define	ADMA_NOOP	0	/* Do nothing 			*/
#define	ADMA_SEND	1	/* send a GCA message 		*/
#define	ADMA_RECV	2	/* receive a GCA message 	*/
#define	ADMA_DISA	3	/* issue GCA_DISASSOC service 	*/
#define	ADMA_ACPT	4	/* build GCA_ACCEPT msg		*/
#define	ADMA_QUERY	5	/* handle query message 	*/
#define	ADMA_TUPL 	6	/* build GCA_TUPLE msg 		*/
#define	ADMA_RESP	7	/* build GCA_RESPONSE msg	*/
#define	ADMA_RLSE 	8	/* build GCA_RELEASE msg 	*/
#define	ADMA_CALLBACK	9	/* request/result call back	*/
#define	ADMA_FAIL 	10	/* set scb status 		*/
#define	ADMA_BAD_STAT	11	/* invalid msg/state combo	*/
#define	ADMA_FREE_RCB 	12	/* free rcb only		*/
#define	ADMA_FREE 	13	/* free both rcb and scb 	*/
 

#define	ADMA_CNT	14

static	char	*actions[ ADMA_CNT ] =
{
    "ADMA_NOOP", "ADMA_SEND",
    "ADMA_RECV", "ADMA_DISA", 
    "ADMA_ACPT", "ADMA_QUERY", 
    "ADMA_TUPL", "ADMA_RESP",
    "ADMA_RLSE", "ADMA_CALLBACK", 
    "ADMA_FAIL", "ADMA_BAD_STAT", 
    "ADMA_FREE_RCB", "ADMA_FREE",
};

#define	ADMAS_00		0
#define	ADMAS_01		1
#define	ADMAS_02		2
#define	ADMAS_03		3
#define	ADMAS_04		4
#define	ADMAS_05		5
#define	ADMAS_06		6
#define	ADMAS_07		7
#define	ADMAS_08		8
#define	ADMAS_09		9
#define	ADMAS_10		10
#define	ADMAS_11		11
#define	ADMAS_12		12

#define	ADMAS_CNT		13
#define	ADMA_MAX		3


static u_i1	act_seq[ ADMAS_CNT ][ ADMA_MAX ] =
{
    { ADMA_SEND,	0,		0	},	/* ADMAS_00 */
    { ADMA_RECV,	0,		0	},	/* ADMAS_01 */
    { ADMA_DISA, 	0,		0	},	/* ADMAS_02 */
    { ADMA_ACPT,	ADMA_SEND,	0	},	/* ADMAS_03 */
    { ADMA_QUERY,	ADMA_CALLBACK,	ADMA_FREE_RCB},	/* ADMAS_04 */
    { ADMA_TUPL,	ADMA_SEND,	0	},	/* ADMAS_05 */
    { ADMA_RESP,	ADMA_SEND,	0	},	/* ADMAS_06 */
    { ADMA_RLSE,	ADMA_SEND,	0	},	/* ADMAS_07 */
    { ADMA_CALLBACK,	ADMA_FREE_RCB,	0	},	/* ADMAS_08 */
    { ADMA_CALLBACK,	ADMA_FREE,	0	},	/* ADMAS_09 */
    { ADMA_FAIL,	ADMA_RLSE,	ADMA_SEND},	/* ADMAS_10 */
    { ADMA_BAD_STAT,	ADMA_RLSE,	ADMA_SEND},	/* ADMAS_11 */
    { ADMA_FAIL,	ADMA_DISA, 	0,	},	/* ADMAS_12 */
	
};

typedef struct
{

    u_i1	next;
    u_i1	action;

} SMT[ ADMS_CNT ][ ADMI_CNT ]; 

static SMT  smt ZERO_FILL;

/*
** Name: sm_info
**
** Description:
**	State machine definition table.  This is a concise list
**	of the states and expected inputs with their resulting
**	new states and actions.  This table is used to build
**	the sparse state machine table (matrix).
**
** History:
**	 11-Sept-2003 (wansh01)
**	    Created.
*/


static struct
{
    u_i1	state;
    u_i1	event;
    u_i1	next;
    u_i1	action;

} sm_info[] =
{
    /*
    **   sending a gca_accept msg  
    */
    { ADMS_IDLE,	ADMI_NEW_SESSION,	ADMS_ACPT,	ADMAS_03 },

    { ADMS_ACPT,	ADMI_SEND_CMPL,		ADMS_RECVMD,	ADMAS_01 },
    { ADMS_ACPT,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_12 },
    
	/*
	**   received a gca_md_assoc msg 
	*/
    { ADMS_RECVMD,	ADMI_RECV_MDASSOC,	ADMS_ACPTMD,	ADMAS_03 },
    { ADMS_RECVMD,	ADMI_RECV_QUERY,	ADMS_RLSE,	ADMAS_11 },
    { ADMS_RECVMD,	ADMI_RECV_INVALID,	ADMS_RLSE,	ADMAS_10 },
    { ADMS_RECVMD,	ADMI_RECV_RELEASE,	ADMS_DISC,	ADMAS_12 },
    { ADMS_RECVMD,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_12 },
	
	/*
	**   sending a gca_accept msg    
	*/
	
    { ADMS_ACPTMD,	ADMI_SEND_CMPL,		ADMS_GETRQST,	ADMAS_01 },
    { ADMS_ACPTMD,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_12 },
	
	/*
	**  receive a request from client 
	*/
    { ADMS_GETRQST,	ADMI_RECV_QUERY,	ADMS_RQST,	ADMAS_04 },
    { ADMS_GETRQST,	ADMI_RECV_MDASSOC,	ADMS_RLSE,	ADMAS_11 },
    { ADMS_GETRQST,	ADMI_RECV_INVALID,	ADMS_RLSE,	ADMAS_10 },
    { ADMS_GETRQST,	ADMI_RECV_RELEASE,	ADMS_DISC,	ADMAS_12 },
    { ADMS_GETRQST,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_12 },
	
	/*
	**  return result/resultlst/error to client   
	*/
    { ADMS_RQST,	ADMI_RSLT,		ADMS_RSLT,	ADMAS_00 },
    { ADMS_RQST,	ADMI_RQST_CMPL,		ADMS_CMPL,	ADMAS_06 },
    { ADMS_RQST,	ADMI_END_SESSION,	ADMS_RLSE,	ADMAS_07 },
    
	/*
	**  return result tuple  
	*/
    { ADMS_RSLT,	ADMI_SEND_CMPL,		ADMS_RQST,	ADMAS_08 },
    { ADMS_RSLT,	ADMI_TUPL,		ADMS_RSLT,	ADMAS_05 },
    { ADMS_RSLT,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_12 },
	
	
	/*
	**  return result cmplete  
	*/
    { ADMS_CMPL,	ADMI_SEND_CMPL,		ADMS_GETRQST,	ADMAS_01 },
    { ADMS_CMPL,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_12 },
	
	/*
	**  send/recv failed  
	*/
    { ADMS_RLSE,	ADMI_SEND_CMPL,		ADMS_DISC,	ADMAS_02 },
    { ADMS_RLSE,	ADMI_RQST_FAIL,		ADMS_DISC,	ADMAS_02 },
	
	/*
	**  terminate  
	*/
    { ADMS_DISC,	ADMI_DISC_DONE,		ADMS_IDLE,	ADMAS_09 },
    { ADMS_DISC,	ADMI_RQST_FAIL,		ADMS_IDLE,	ADMAS_09 },
};

static  bool    gcadm_active = FALSE;
static  void 	gcadm_complete( PTR  );
static  GCADM_RCB * gcadm_new_rcb( );
static  void    gcadm_free_rcb( GCADM_RCB * );
static  void 	gcadm_event( GCADM_RCB *  );
static 	void    gcadm_sm( GCADM_RCB * );
static  int 	gcadm_format_re_data( GCADM_SCB * ); 
static  void 	gcadm_resume( GCADM_RCB * );

/*
** Name: gcadm_smt_init
**
** Description:
**	init gcadm state machine.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 16-Sept-03 (wansh01)
**	    Created.
*/
void
gcadm_smt_init( )
{

    int i; 
 
    MEfill( sizeof( smt ), 0xff, (PTR)smt );

    for( i = 0; i < ARR_SIZE( sm_info ); i++ )
    {
	smt[ sm_info[i].state ][ sm_info[i].event ].next = sm_info[i].next;
	smt[ sm_info[i].state ][ sm_info[i].event ].action = sm_info[i].action;
    }
    return;
}

/*
** Name: gcadm_eval
**
** Description:
**	Evaluate and validate input rcb->event. 
**
**
** Input:
**	rcb		Request control block.
**
** Output:
**	rcb->event 	New event ID.
**
** Returns:
**	void
**
** History:
**	 3-Oct-03 (wansh01)
**	    Created.
*/

static void
gcadm_eval( GCADM_RCB *rcb )
{
    u_i1	orig = rcb->event;
    GCADM_SCB  *scb  = rcb->scb; 


    switch( rcb->event )  
    {
    case ADMI_SEND_CMPL:
   	/*  there is more rslt to be sent ?? */ 
	if ( (PTR)scb->rsltlst_entry != (PTR)scb->rsltlst_end )
	    rcb->event = ADMI_TUPL; 
	else 
	{
	    scb->rsltlst_entry = NULL; 
	    scb->rsltlst_end = NULL; 
	}
	break; 

    case ADMI_RECV_MDASSOC:
	{
	char    *md_parms; 
	i4 	data_len, p_index, gca_type, gca_prec;
	i4	gca_l_value, gca_value; 

   	/*  validate GCA_MD_ASSOC parms   */
	md_parms = (char *) scb->parms.gca_rv_parm.gca_data_area;

	md_parms += GCA_GETI4_MACRO( md_parms, &data_len); 
	if ( data_len == 1 ) 
	{
	    md_parms += GCA_GETI4_MACRO( md_parms, &p_index); 
	    if ( p_index == GCA_SVTYPE )
	    {
		md_parms += GCA_GETI4_MACRO( md_parms, &gca_type ); 
		md_parms += GCA_GETI4_MACRO( md_parms, &gca_prec ); 
		md_parms += GCA_GETI4_MACRO( md_parms, &gca_l_value ); 
		md_parms += GCA_GETI4_MACRO( md_parms, &gca_value ); 
		if ( ( gca_type == GCA_TYPE_INT )  && 
		    ( gca_l_value == sizeof ( i4 ))  && 
		    ( gca_value == GCA_MONITOR) )
			break; 
	    }
	}
	rcb->status = E_GC5007_INVALID_MD_PARMS;
	rcb->event = ADMI_RECV_INVALID;
	break; 
	}
    }    

    if ( orig != rcb->event  &&  GCADM_global.gcadm_trace_level >= 5 )
	TRdisplay( "%4d   GCADM eval: %s  => %s \n", 
 			scb->aid,  events[ orig ], events[ rcb->event ]);

    return;
}



/*
** Name: gcadm_complete
**
** Description:
**	Callback routine to handle GCA service calls completions.
**
** Input:
**	ptr,  request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-03 (wansh01)
**	    Created.
*/

static void
gcadm_complete( PTR ptr )
{
    GCADM_RCB	*rcb = (GCADM_RCB *)ptr;  
    GCADM_SCB   *scb = rcb->scb;
    
    /*
    ** Check for request failure or convert request
    ** type to the associated request completion.  
    ** if term state, we don't care if request is complete or failed 
    ** return to caller.    
    **
    */

    if ( scb->parms.gca_all_parm.gca_status == E_GCFFFE_INCOMPLETE )
    {
 	gcadm_resume ( rcb );
	return; 
    }

    if ( GCADM_global.gcadm_state != GCADM_ACTIVE )
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	    TRdisplay( "%4d   GCADM complete: ADM not active\n", -1 );
	if ( rcb->operation ==  GCA_DISASSOC )
	    gcadm_free_scb( scb );
	gcadm_free_rcb( rcb );
	return; 
    }

    if ( GCADM_global.gcadm_trace_level >= 4 )
	TRdisplay( "%4d   GCADM complete: entry\n", scb->aid);

    if ( GCADM_global.gcadm_trace_level >= 5 )
	TRdisplay( "%4d   GCADM complete: GCA operation= %d, status =%d\n", 
		scb->aid, rcb->operation,
		scb->parms.gca_all_parm.gca_status );

    if ( scb->parms.gca_all_parm.gca_status != OK)    
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	    TRdisplay( "%4d   GCADM complete: GCA operation= %d failed \n", 
		scb->aid, rcb->operation );
	rcb->event = ADMI_RQST_FAIL;
    	rcb->status = scb->parms.gca_all_parm.gca_status;
	gcadm_event( rcb );
	return;
    }
	
    switch( rcb->operation )
    {
	case GCA_RECEIVE : 
	    switch (scb->parms.gca_rv_parm.gca_message_type)
	    {
	    case GCA_RELEASE:  
		rcb->event = ADMI_RECV_RELEASE;
		break;
	    case GCA_MD_ASSOC:
		rcb->event = ADMI_RECV_MDASSOC;
		break;
	    case GCA_QUERY:
		rcb->event = ADMI_RECV_QUERY;
		break;
	    default:
		if ( GCADM_global.gcadm_trace_level >= 1 )
		   TRdisplay( "%4d   GCADM complete: recv unexpected msg %d \n", 	            scb->aid, scb->parms.gca_rv_parm.gca_message_type); 
		rcb->status = E_GC5004_INVALID_MSG;
		rcb->event = ADMI_RECV_INVALID;
		break;
	    }
	    break;	

	case GCA_SEND: 
	    rcb->event = ADMI_SEND_CMPL; 
	    break;

	case GCA_DISASSOC: 
 	    rcb->event = ADMI_DISC_DONE; 
	    break;  
			
	default :
	    rcb->status = E_GC5005_INTERNAL_ERROR;
	    rcb->event = ADMI_RQST_FAIL;
	    return;
    }

    gcadm_event( rcb );
    return;

}

/*
** Name: gcadm_request
**
** Description:
**	Evaluate the input GCADM request and convert to rcb event 
**
**
** Input:
**	request		GCADM request.   	
**	scb		Session control block.
**
** Output:
**	rcb->event 	event ID.
**
** Returns:
**	void
**
** History:
**	 3-Oct-03 (wansh01)
**	    Created.
*/

STATUS 
gcadm_request( u_i1 request, GCADM_SCB *scb )
{
   GCADM_RCB   *rcb; 

	
    if  ( request != GCADM_RQST_NEW_SESSION ) 
	if (scb->admin_flag & GCADM_CALLBACK ) 
	    scb->admin_flag &= ~GCADM_CALLBACK;
	else 
	    return( E_GC5009_DUP_RQST ); 

    if ( rcb = gcadm_new_rcb() )
	rcb->scb = scb; 
    else 
	return( E_GC5002_NO_MEMORY ); 

    switch( request )
    {
    case GCADM_RQST_NEW_SESSION :
	rcb->event = ADMI_NEW_SESSION;
	break;

    case GCADM_RQST_RSLT:
    case GCADM_RQST_RSLTLST:
	rcb->event = ADMI_RSLT;
	break;

    case GCADM_RQST_ERROR:
	rcb->event = ADMI_RSLT;
	scb->admin_flag |= GCADM_ERROR;
	break;

    case GCADM_RQST_DONE:
	rcb->event = ADMI_RQST_CMPL;
	break;

    case GCADM_RQST_DISC:
	rcb->event = ADMI_END_SESSION;
	break;

    default:
	if ( GCADM_global.gcadm_trace_level >= 1 )
	    TRdisplay( "%4d   GCADM request: recv unexpected request %d \n", 
		    scb->aid, request); 
	gcadm_free_rcb ( rcb );
	return( E_GC5005_INTERNAL_ERROR );

    }

    if ( GCADM_global.gcadm_trace_level >= 5 )
	TRdisplay( "%4d   GCADM request: request=%d, event=%s\n", 
			scb->aid,  request, events[ rcb->event ] );

    gcadm_event( rcb ); 
    return( OK );
}
/*
** Name: gcadm_event
**
** Description:
**
** Input:
**	rcb	
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 23-Oct-03 (wansh01)
**	    Created.
*/

static void 
gcadm_event( GCADM_RCB  *rcb )
{

   GCADM_SCB *scb=rcb->scb;

    if ( gcadm_active )
    {
	if ( GCADM_global.gcadm_trace_level >= 5 )
	    TRdisplay( "%4d   GCADM event: queueing event %s  \n",
		    scb->aid, events[ rcb->event ]);
	QUinsert( &rcb->q, GCADM_global.rcb_q.q_prev); 
	return;
    }

    gcadm_active = TRUE;
    gcadm_sm( rcb );

    while( ( rcb  = (GCADM_RCB  *)QUremove( GCADM_global.rcb_q.q_next )) )
    {
	if ( GCADM_global.gcadm_state != GCADM_ACTIVE )
	{
	    if ( GCADM_global.gcadm_trace_level >= 1 )
		TRdisplay( "%4d   GCADM event: ADM not active\n", -1 );
	    gcadm_free_rcb ( rcb );
	}
	else
	{
	    if( GCADM_global.gcadm_trace_level >= 5 )
		TRdisplay( "%4d   GCADM event: processing queued event %s \n",
			scb->aid, events[ rcb->event ]);
	    gcadm_sm( rcb );
	}
    }

    gcadm_active = FALSE;
    return;
}


/*
** Name: gcadm_sm
**
** Description:
**	State machine processing loop for the GCADM processing.
**
** Inputs:
**	scb 
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** History:
**	 12-Sept-03 (wansh01)
**	    Created.
*/

void
gcadm_sm( GCADM_RCB *rcb )
{
           
    u_i1	i, action;
    GCADM_SCB   *scb = rcb->scb; 
    STATUS      status; 
    i4		save_aid;

    if ( scb->sequence >= ADMS_CNT  ||  rcb->event >= ADMI_CNT ) 
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	    TRdisplay( "%4d   GCADM sm: invalid %d %d \n",
		    scb->aid, scb->sequence, rcb->event);
	scb->status = E_GC5005_INTERNAL_ERROR;  
	return;
    }


    gcadm_eval( rcb );

    if ( scb->sequence >= ADMS_CNT  ||  rcb->event >= ADMI_CNT  ||
	 smt[ scb->sequence ][ rcb->event ].next >= ADMS_CNT  ||
	 smt[ scb->sequence ][ rcb->event ].action >= ADMAS_CNT )
    {
	if ( GCADM_global.gcadm_trace_level >= 1 )
	    TRdisplay( "%4d   GCADM sm: invalid %d %d \n",
		    scb->aid, scb->sequence, rcb->event);
	gcadm_errlog ( E_GC5003_FSM_STATE );
	return;
    }

    if ( GCADM_global.gcadm_trace_level >= 4 )
	TRdisplay( "%4d   GCADM sm: entry\n",scb->aid ); 

    if ( GCADM_global.gcadm_trace_level >= 5 )
	TRdisplay( "%4d   GCADM sm: state %s event %s => %s\n", scb->aid, 
		   states[ scb->sequence ], events[ rcb->event ],
		   states[ smt[ scb->sequence ][ rcb->event ].next ] );

    action = smt[ scb->sequence ][ rcb->event ].action;

    /*
    ** Make the state transition now 
    */

    scb->sequence = smt[ scb->sequence ][ rcb->event ].next;
    save_aid = scb->aid; 

    for( i = 0; i < ADMA_MAX; i++ )
    {
	if ( GCADM_global.gcadm_trace_level >= 5  &&  
	     act_seq[action][i] != ADMA_NOOP )
	    TRdisplay( "%4d   GCADM sm: action %s, %p\n", save_aid, 
		       actions[ act_seq[ action ][ i ] ], rcb );


	switch( act_seq[ action ][ i ] )
	{
	case ADMA_NOOP :		/* Do nothing, end of action sequence */
	    i = ADMA_MAX;
	    break;
	
	case ADMA_SEND :		/* issue GCA_SEND request  */
	    MEfill(sizeof(scb->parms), 0, &scb->parms);
	
	    rcb->operation = GCA_SEND; 
	    scb->parms.gca_sd_parm.gca_message_type = scb->msg_type;
	    scb->parms.gca_sd_parm.gca_end_of_data = TRUE;
	    scb->parms.gca_sd_parm.gca_buffer = scb->buffer;
	    scb->parms.gca_sd_parm.gca_msg_length = scb->buf_len; 
	    scb->parms.gca_sd_parm.gca_association_id = scb->aid;
	    scb->parms.gca_sd_parm.gca_completion = gcadm_complete;
	    scb->parms.gca_sd_parm.gca_closure = (PTR)rcb;
	    if ( scb->msg_type == GCA_TUPLES )
		scb->parms.gca_sd_parm.gca_descriptor = (PTR)(&scb->tdesc_buf); 

	    gca_call( &scb->gca_cb, GCA_SEND,
		    (GCA_PARMLIST *)&scb->parms,
		    GCA_ASYNC_FLAG | GCA_ALT_EXIT,
		    (PTR)rcb, -1, &status);

	    if ( status != OK )  
	    {
		if ( GCADM_global.gcadm_trace_level >= 1 )
		    TRdisplay( "%4d   GCADM sm: send error: 0x%x\n",
		       save_aid, status );
		rcb->status = status; 
		rcb->event = ADMI_RQST_FAIL;
		gcadm_event ( rcb );
	    }
	    break;	

	case ADMA_RECV :		/* Issue GCA_RECEIVE request */
	    MEfill(sizeof(scb->parms), 0, &scb->parms);

	    rcb->operation = GCA_RECEIVE;	
	    scb->parms.gca_rv_parm.gca_buffer = scb->buffer;  
	    scb->parms.gca_rv_parm.gca_assoc_id = scb->aid;
	    scb->parms.gca_rv_parm.gca_b_length = GCADM_global.gcadm_buff_len;
	    scb->parms.gca_rv_parm.gca_flow_type_indicator = GCA_NORMAL;
	    scb->parms.gca_rv_parm.gca_completion = gcadm_complete; 
	    scb->parms.gca_rv_parm.gca_closure = (PTR)rcb; 
		    
	    gca_call( &scb->gca_cb, GCA_RECEIVE,
		    (GCA_PARMLIST *)&scb->parms,
		    GCA_ASYNC_FLAG | GCA_ALT_EXIT,
		    (PTR)rcb, -1, &status);
		    
	    if ( status != OK )  
	    {
		if ( GCADM_global.gcadm_trace_level >= 1 )
		    TRdisplay( "%4d   GCADM sm: receive error: 0x%x\n",
		       save_aid, status );
		rcb->status = status; 
		rcb->event = ADMI_RQST_FAIL;
		gcadm_event ( rcb );
	    }
	    break;

	case ADMA_DISA :		/* Issue GCA_DISASSOC request */
	    /*
	    ** Disassociate the client connection.
	    */
	    rcb->operation = GCA_DISASSOC; 
	    scb->admin_flag |= GCADM_DISC; 
	    MEfill(sizeof(scb->parms.gca_da_parm), 0, &scb->parms.gca_da_parm);
	    scb->parms.gca_da_parm.gca_association_id = scb->aid;
	    scb->parms.gca_da_parm.gca_completion = gcadm_complete; 
	    scb->parms.gca_da_parm.gca_closure = (PTR)rcb;

	    gca_call( &scb->gca_cb, GCA_DISASSOC,
			(GCA_PARMLIST *)&scb->parms.gca_da_parm,
			GCA_ASYNC_FLAG | GCA_ALT_EXIT,
			(PTR)rcb, -1, &status);
		    
	  
	    if ( status != OK )  
	    {
		if ( GCADM_global.gcadm_trace_level >= 1 )
		    TRdisplay( "%4d   GCADM sm: disassociate error: 0x%x\n",
			   save_aid, status );
		rcb->status = status; 
		rcb->event = ADMI_RQST_FAIL;
		gcadm_event ( rcb );
	    }
	    break; 

	case ADMA_ACPT :		/* build gca_accept msg  */

	    scb->msg_type = GCA_ACCEPT;
	    scb->buf_len  = 0;
	    break;  

	case ADMA_QUERY : 		/* process query */
				
	    {
		char   *q_data; 
		i4     lang_id, q_modifier, gca_type, gca_prec;

		/* parse received data to get query text */
		/* save query len and text in scb       */
		q_data = (char *) scb->parms.gca_rv_parm.gca_data_area;
		q_data += GCA_GETI4_MACRO( q_data, &lang_id );
		q_data += GCA_GETI4_MACRO( q_data, &q_modifier );
		q_data += GCA_GETI4_MACRO( q_data, &gca_type );
		q_data += GCA_GETI4_MACRO( q_data, &gca_prec );

		if ( gca_type == DB_QTXT_TYPE)  
		{	 
		    if ( q_modifier & GCA_COMP_MASK )
        		scb->admin_flag |= GCADM_COMP_MASK;
		    q_data += GCA_GETI4_MACRO( q_data, &scb->buf_len );
		    q_data += GCA_GETBUF_MACRO( q_data, scb->buf_len, 
						scb->buffer );
		    scb->status = OK;
		}
		break;  
	    }	

	case ADMA_TUPL :		/* build GCA_TUPLES msg  */
	    {
	    GCADM_RSLT *rslt;
	    i4 	fill_len, zero=0;
	    i2 	i2_len;
	    GCA_TU_DATA *tu_data;

	    rslt = (GCADM_RSLT *) scb->rsltlst_entry;
	    scb->rsltlst_entry = (GCADM_RSLT *) rslt->rslt_q.q_next;

	    scb->msg_type = GCA_TUPLES; 

	    tu_data = (GCA_TU_DATA *)scb->buffer;
	    if ( scb->admin_flag & GCADM_COMP_MASK )
		scb->buf_len = rslt->rslt_len +2 ;
	    else 
		scb->buf_len = scb->row_len; 

	    i2_len = (i2)rslt->rslt_len; 
	    tu_data += GCA_PUTI2_MACRO( &i2_len, tu_data);

	    tu_data += GCA_PUTBUF_MACRO( rslt->rslt_text, rslt->rslt_len, 
					tu_data);
	    if ( ! ( scb->admin_flag & GCADM_COMP_MASK ))
	    {
		fill_len = scb->row_len - rslt->rslt_len;
		MEfill ( fill_len, 0, tu_data );
	    }

	    scb->row_count++;
	    break; 
	    }


	case ADMA_RESP :		/* build gca_response msg  */

	    scb->msg_type = GCA_RESPONSE;
	    scb->buf_len  = gcadm_format_re_data( scb );
	    break;  

	case ADMA_RLSE :		/* build gca_relese msg   */

	    scb->msg_type = GCA_RELEASE;
	    scb->buf_len  = gcadm_format_err_data 
			( scb->buffer, NULL, scb->status, 0, NULL );
	    break;

	case ADMA_CALLBACK : 		/* request result call back 	*/
	{
	    GCADM_RQST_PARMS rqst_parms; 
	    GCADM_CMPL_PARMS cmpl_parms; 

	    scb->admin_flag |= GCADM_CALLBACK;

	    if ( scb->rqst_cb )
	    {
		GCADM_RQST_FUNC	rqst_cb = scb->rqst_cb;

		rqst_parms.gcadm_scb = (PTR)scb;	
		rqst_parms.rqst_cb_parms = scb->rqst_cb_parms;
		rqst_parms.length = scb->buf_len;
		rqst_parms.request = scb->buffer;
		rqst_parms.sess_status = scb->status;
		scb->rqst_cb = NULL;
		(rqst_cb)( &rqst_parms ); 
	    }
	    else  if ( scb->rslt_cb )
	    {	
		GCADM_CMPL_FUNC	rslt_cb = scb->rslt_cb;

		cmpl_parms.gcadm_scb = (PTR)scb;	
		cmpl_parms.rslt_cb_parms = scb->rslt_cb_parms;
		cmpl_parms.rslt_status = scb->status;
		scb->rslt_cb = NULL;
		(rslt_cb)( &cmpl_parms ); 
	    }
	    break;  
	}
		
	case ADMA_FAIL : 		/* set failed status  	*/
				
		if ( rcb->status )
		    scb->status = rcb->status; 
		else
		    scb->status = FAIL; 

		break;  

	case ADMA_BAD_STAT :		/* set status error  */

		scb->status = E_GC5008_INVALID_MSG_STATE;
		break;

		
	case ADMA_FREE_RCB :		/* Free RCB */
		gcadm_free_rcb( rcb  );   
		rcb = NULL; 
		break;

	case ADMA_FREE :		/* Free SCB */
		gcadm_free_rcb( rcb  );   
		gcadm_free_scb( scb  );   
		rcb = NULL; 
		scb = NULL; 
		break;
	
	}	
    }

    return;
}

/*
** Name: gcadm_new_scb
**
** Description:
**	Allocate a new session control block.  The data buffer
**	is initialized for building MSG layer messages.
**
** Input:
**	scb		Session control block.
**
** Output:
**	None.
**
** Returns:
**	GCADM_SCB *	New session control block.
**
** History:
**	 16-Sept-2003 (wansh01)
**	    Created.
*/

GCADM_SCB *
gcadm_new_scb( i4 aid, PTR gca_cb )
{
   GCADM_SCB  *scb;

    scb = (GCADM_SCB *)(* GCADM_global.alloc_rtn )( sizeof( GCADM_SCB ) );
   
    if ( scb )
    {
	MEfill( sizeof( GCADM_SCB ), 0, scb);
	scb->buffer = (char *) (*GCADM_global.alloc_rtn)
			    ( GCADM_global.gcadm_buff_len );
   }
   
    if ( scb->buffer )
    {
	scb->aid  =  aid;
	scb->gca_cb = gca_cb;
	QUinit( &scb->q );
	QUinsert( &scb->q, GCADM_global.scb_q.q_prev); 
    }
    else
    {
	(*GCADM_global.dealloc_rtn)( (PTR)scb );
	scb = NULL;
    }

    return( scb );
}




/*
** Name: gcadm_free_scb
**
** Description:
**	Free a session control block and the associated message buffer.
**
** Input:
**	scb	Request control block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

void
gcadm_free_scb( GCADM_SCB *scb )
{
	
    /* remove scb from queue before free   */

     QUremove( &scb->q);  

    if ( scb->buffer )
	(*GCADM_global.dealloc_rtn)( (PTR)scb->buffer );
			 
    if ( scb )
	(*GCADM_global.dealloc_rtn)( (PTR)scb );
    
    return;
}


/*
** Name: gcadm_new_rcb
**
** Description:
**	Allocate a new request control block.  
**
** Input:
**	none
**
** Output:
**	None.
**
** Returns:
**	GCADM_RCB *	New request control block.
**
** History:
**	 16-Sept-2003 (wansh01)
**	    Created.
*/

GCADM_RCB *
gcadm_new_rcb( )
{
   GCADM_RCB  *rcb;


    rcb = (GCADM_RCB *)(* GCADM_global.alloc_rtn )( sizeof( GCADM_RCB ) );
   
    if ( rcb )  
    {
	MEfill(sizeof( GCADM_RCB ), 0, rcb);
	QUinit( &rcb->q );
    }

    return( rcb );
}




/*
** Name: gcadm_free_rcb
**
** Description:
**	Free a request control block.
**
** Input:
**	rcb	Request control block to be freed.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

void
gcadm_free_rcb( GCADM_RCB *rcb )
{
	
    if ( rcb )
	( *GCADM_global.dealloc_rtn )( (PTR)rcb );
    
    return;
}
/*
** Name: gcadm_errlog
**
** Description:
**	error log routine.
**
** Input:
**	scb	
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

void
gcadm_errlog(  STATUS  errcode )
{

    if ( GCADM_global.errlog_rtn  )
	(*GCADM_global.errlog_rtn)( errcode );
    else 
	TRdisplay( "%4d   GCADM errlog: error code = %d\n",  -1, errcode);
    
    return;
}

/*
** Name: gcadm_format_err_data
**
** Description:
**	format GCA_ER_DATA routine.
**
** Input:
**	buffer		buffer area that contains GCA_ER_DATA object
**	sqlstate	SQLSTATE 
**	error_code	DBMS error code  
**	error_len 	error text length   
**	error_text 	error text   
**
** Output:
**	void 		
**
** Returns:
**	buf_len		length of GCA_ER_DATA 
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

int
gcadm_format_err_data( char * buffer, char * sqlstate, i4 error_code, 
	    i4 error_len, char * error_text )
{
    char   *er_data;
    i4     one = 1, zero = 0;
    i4     data_type = DB_CHA_TYPE;
    i4     ss_code, severity = GCA_ERFORMATTED;
    i4     msg_len;


    if ( GCADM_global.gcadm_trace_level >= 5 )
	TRdisplay( "%4d   GCADM format_err_data:  error_code= %d\n",						 -1, error_code );
    /* 
    ** Set default sqlstate
    */
    if (  sqlstate  == NULL )
	sqlstate = SS50000_MISC_ERRORS;

    ss_code = ss_encode( sqlstate );

    /* 
    ** Format GCA_ER_DATA object  
    */
    er_data = (char *) buffer;
    if ( ( error_code == FAIL ) || ( error_code == OK )) 
	er_data += GCA_PUTI4_MACRO( &zero, er_data);	/* gca_l_e_element */
    else 	
    {
	er_data += GCA_PUTI4_MACRO( &one, er_data);	/* gca_l_e_element */
	er_data += GCA_PUTI4_MACRO( &ss_code, er_data );/* gca_id_error */ 
	er_data += GCA_PUTI4_MACRO( &zero, er_data); 	/* gca_id_server */
	er_data += GCA_PUTI4_MACRO( &zero, er_data); 	/* gca_server_type */
	er_data += GCA_PUTI4_MACRO( &severity, er_data );/* gca_serverity */
	er_data += GCA_PUTI4_MACRO( &error_code, er_data);/* gca_local_error */

	/* 
	** Check to see if there is error text 
	*/
	if ( error_len == 0  ||  ! error_text )
	    er_data += GCA_PUTI4_MACRO( &zero, er_data);/* gca_l_error_parm */
	else
	{
	    er_data += GCA_PUTI4_MACRO( &one, er_data);	/* gca_l_error_parm */
	    er_data += GCA_PUTI4_MACRO( &data_type, er_data );   /* gca_type */
	    er_data += GCA_PUTI4_MACRO( &zero, er_data );    /* gca_precision */

	/* 
	** Calculate the message len to ensure not over max buffer len  
	*/
	    msg_len = er_data - (char *)buffer + sizeof(i4);	
	    if ( (error_len + msg_len) > GCADM_global.gcadm_buff_len )
		  error_len = GCADM_global.gcadm_buff_len - msg_len;

	    er_data += GCA_PUTI4_MACRO( &error_len, er_data);  /* gca_l_value */
	    er_data += GCA_PUTBUF_MACRO( error_text, error_len, er_data);
							    /* gca_value */
	}
    }

    return( er_data - (char *)buffer );  
    
}
/*
** Name: gcadm_format_re_data
**
** Description:
**	format GCA_RE_DATA routine.
**
** Input:
**	buffer		buffer area that contains GCA_RE_DATA object	
**	admion_flag 	scb admin flag to set rqstatus  	
**
** Output:
**	None.
**
** Returns:
**	buf_len		length of GCA_RE_DATA object  
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

int
gcadm_format_re_data( GCADM_SCB * scb ) 
{
    char   *re_data;
    i4     zero = 0;
    i4 	   norow_count = GCA_NO_ROW_COUNT;
    i4     rqstatus;
    i4     key_len= 16;
    char   key_mask[ 16 ];

	
    if ( GCADM_global.gcadm_trace_level >= 5 )
	TRdisplay( "%4d   GCADM format_re_data: row_count= %d\n", scb->aid, 
					scb->row_count  );

    /* 
    ** Format GCA_RE_DATA object  
    */
    re_data = (char *) scb->buffer;
    re_data += GCA_PUTI4_MACRO( &zero, re_data);	/* gca_rid   */

    if ( scb->admin_flag & GCADM_ERROR )
    {
	rqstatus = GCA_FAIL_MASK | GCA_LOG_TERM_MASK;
	scb->admin_flag &= ~GCADM_ERROR;
    }
    else
	rqstatus = GCA_LOG_TERM_MASK;

    if ( scb->admin_flag & GCADM_COMP_MASK )
    	scb->admin_flag &= ~GCADM_COMP_MASK;

    re_data += GCA_PUTI4_MACRO( &rqstatus, re_data );	/* gca_rqstatus */ 

    if ( scb->row_count  > 0 )
    {
	re_data += GCA_PUTI4_MACRO( &scb->row_count, re_data);
	scb->row_count = 0; 
    }
    else 	
	re_data += GCA_PUTI4_MACRO( &norow_count, re_data);/* gca_rowcount */

    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_rhistamp */
    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_rlostamp */
    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_cost */
    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_errd0 */
    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_errd1 */
    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_errd4 */
    re_data += GCA_PUTI4_MACRO( &zero, re_data); 	/* gca_errd5 */
    re_data += GCA_PUTBUF_MACRO( &key_mask, key_len, re_data);
							/* gca_logkey */

    return( re_data - (char *)scb->buffer );  
    
}
/*
** Name: gcadm_resume
**
** Description:
**	resume GCA services. 
**
** Input:
**	rcb 		request control block
**
** Output:
**	None.
**
** Returns:
**	none 
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

void 
gcadm_resume( GCADM_RCB * rcb )
{
     STATUS status; 
    GCADM_SCB * scb=rcb->scb;
     
	if ( GCADM_global.gcadm_trace_level >= 4 )
	    TRdisplay( "%4d   GCADM resume: entry\n", scb->aid  );
	/*
	** resume GCA services 
	*/

	MEfill(sizeof(scb->parms.gca_all_parm), 0, &scb->parms.gca_all_parm);
	scb->parms.gca_all_parm.gca_assoc_id = scb->aid;
	scb->parms.gca_all_parm.gca_completion = gcadm_complete; 
	scb->parms.gca_all_parm.gca_closure = (PTR)rcb;

	gca_call( &scb->gca_cb, rcb->operation,
		    (GCA_PARMLIST *)&scb->parms,
		    GCA_ASYNC_FLAG | GCA_ALT_EXIT | GCA_RESUME,
		    (PTR)rcb, -1, &status);
		    
	  
	if ( status != OK )  
	{
	    if ( GCADM_global.gcadm_trace_level >= 1 )
		TRdisplay( "%4d   GCADM resume: disassociate error: 0x%x\n",
		       scb->aid, status );
	    if ( rcb ) 
		rcb->status = status; 
	}
	return; 
}
