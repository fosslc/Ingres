/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcmsg.h>

/*
** Name: jdbcgcc.c
**
** Description:
**	JDBC server interface to GCC.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial TL connection info.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.
**	 4-Nov-99 (gordy)
**	    Added support for handling TL interrupts.
**	16-Nov-99 (gordy)
**	    Skip unknown connection parameters.
**	13-Dec-99 (gordy)
**	    Negotiate GCC parameters prior to session start
**	    so that values are available to session layer.
**	21-Dec-99 (gordy)
**	    Added check for clients idle for excessive period.
**	17-Jan-00 (gordy)
**	    Fine-tune the client idle checking.
**	 3-Mar-00 (gordy)
**	    Moved CCB init to general initializations.
**	10-Apr-01 (loera01) Bug 104459
**	    When JCI_RQST_FAIL is received, go to disconnect state
**          (JCS_DISC) and do a disconnect (JCAS_16) instead of
**          going to the abort state (JCS_ABRT) and abort (JCAS_14).
**          Otherwise the JDBC server will not clean up aborted sessions.
*/	

static	GCC_PCT		*gcc_pct = NULL;

#define	JCI_OPEN_RQST	0
#define JCI_LSTN_RQST	1
#define	JCI_RECV_RQST	2
#define	JCI_RECV_CONN	3
#define	JCI_RECV_DATA	4
#define	JCI_RECV_DISC	5
#define	JCI_RECV_DONE	6
#define	JCI_RECV_INT	7
#define	JCI_SEND_RQST	8
#define	JCI_SEND_DONE	9
#define	JCI_SEND_QEUE	10
#define	JCI_ABRT_RQST	11
#define	JCI_DISC_RQST	12
#define	JCI_RQST_DONE	13
#define	JCI_RQST_FAIL	14

#define	JCI_CNT		15

static	char	*inputs[ JCI_CNT ] =
{
    "JCI_OPEN_RQST", "JCI_LSTN_RQST", "JCI_RECV_RQST", 
    "JCI_RECV_CONN", "JCI_RECV_DATA", "JCI_RECV_DISC", "JCI_RECV_DONE", 
    "JCI_RECV_INT",  "JCI_SEND_RQST", "JCI_SEND_DONE", "JCI_SEND_QEUE", 
    "JCI_ABRT_RQST", "JCI_DISC_RQST", "JCI_RQST_DONE", "JCI_RQST_FAIL",
};

#define	JCS_IDLE	0
#define	JCS_OPEN	1
#define	JCS_LSTN	2
#define	JCS_CONN	3
#define	JCS_RECV	4
#define	JCS_SEND	5
#define	JCS_SHUT	6
#define	JCS_ABRT	7
#define	JCS_DISC	8

#define	JCS_CNT		9

static	char	*states[ JCS_CNT ] =
{
    "JCS_IDLE", "JCS_OPEN", "JCS_LSTN", "JCS_CONN",
    "JCS_RECV", "JCS_SEND", "JCS_SHUT", "JCS_ABRT", "JCS_DISC",
};

#define	JCA_NOOP	0
#define	JCA_OPEN	1
#define	JCA_ODON	2
#define	JCA_OERR	3
#define	JCA_NEWC	4
#define	JCA_LSTN	5
#define	JCA_LERR	6
#define	JCA_CCNF	7
#define	JCA_RECV	8
#define	JCA_PROC	9
#define	JCA_INT		10
#define	JCA_HEAD	11
#define	JCA_SEND	12
#define	JCA_QEUE	13
#define	JCA_DEQU	14
#define	JCA_FSRQ	15
#define	JCA_DCNF	16
#define	JCA_ABRT	17
#define	JCA_DISC	18
#define	JCA_SRCB	19
#define	JCA_FRCB	20
#define	JCA_FCCB	21

#define	JCA_CNT		22

static	char	*actions[ JCA_CNT ] =
{
    "JCA_NOOP", "JCA_OPEN", "JCA_ODON", "JCA_OERR",
    "JCA_NEWC", "JCA_LSTN", "JCA_LERR", "JCA_CCNF", 
    "JCA_RECV", "JCA_PROC", "JCA_INT",  "JCA_HEAD", 
    "JCA_SEND", "JCA_QEUE", "JCA_DEQU", "JCA_FSRQ", 
    "JCA_DCNF", "JCA_ABRT", "JCA_DISC", 
    "JCA_SRCB", "JCA_FRCB", "JCA_FCCB",
};

#define	JCAS_00		0
#define	JCAS_01		1
#define	JCAS_02		2
#define	JCAS_03		3
#define	JCAS_04		4
#define	JCAS_05		5
#define	JCAS_06		6
#define	JCAS_07		7
#define	JCAS_08		8
#define	JCAS_09		9
#define	JCAS_10		10
#define	JCAS_11		11
#define	JCAS_12		12
#define	JCAS_13		13
#define	JCAS_14		14
#define	JCAS_15		15
#define	JCAS_16		16
#define	JCAS_17		17
#define	JCAS_18		18
#define	JCAS_19		19

#define	JCAS_CNT	20
#define	JCA_MAX		3

static u_i1	act_seq[ JCAS_CNT ][ JCA_MAX ] =
{
    { JCA_NOOP,	0,	  0	   },	/* JCAS_00 */
    { JCA_OPEN,	0,	  0	   },	/* JCAS_01 */
    { JCA_ODON,	JCA_NEWC, JCA_FCCB },	/* JCAS_02 */
    { JCA_OERR,	JCA_FCCB, 0        },	/* JCAS_03 */
    { JCA_LSTN,	0,	  0	   },	/* JCAS_04 */
    { JCA_NEWC,	JCA_SRCB, JCA_RECV },	/* JCAS_05 */
    { JCA_LERR,	JCA_FCCB, 0	   },	/* JCAS_06 */
    { JCA_CCNF,	JCA_RECV, JCA_SEND },	/* JCAS_07 */
    { JCA_RECV,	JCA_PROC, 0	   },	/* JCAS_08 */
    { JCA_HEAD,	JCA_SEND, 0	   },	/* JCAS_09 */
    { JCA_HEAD,	JCA_QEUE, 0	   },	/* JCAS_10 */
    { JCA_FRCB,	JCA_DEQU, JCA_SEND },	/* JCAS_11 */
    { JCA_DCNF,	JCA_SEND, 0	   },	/* JCAS_12 */
    { JCA_DCNF,	JCA_FSRQ, JCA_QEUE },	/* JCAS_13 */
    { JCA_ABRT, JCA_SEND, 0        },	/* JCAS_14 */
    { JCA_ABRT, JCA_FSRQ, JCA_QEUE },	/* JCAS_15 */
    { JCA_DISC,	0,	  0	   },	/* JCAS_16 */
    { JCA_FRCB,	0,	  0	   },	/* JCAS_17 */
    { JCA_FCCB,	0,	  0	   },	/* JCAS_18 */
    { JCA_RECV, JCA_INT,  JCA_FRCB },	/* JCAS_19 */
};

/*
** Name: sm_info
**
** Description:
**	State machine definition table.  This is a concise list
**	of the expected inputs and states with their resulting
**	new states and actions.  This table is used to build
**	the sparse state machine table (matrix).
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	 4-Nov-99 (gordy)
**	    Added entries for TL interrupts.
*/

static struct
{
    u_i1	input;
    u_i1	state;
    u_i1	next;
    u_i1	action;

} sm_info[] =
{
    /*
    ** Accept new clients.
    */
    { JCI_OPEN_RQST,	JCS_IDLE,	JCS_OPEN,	JCAS_01 },
    { JCI_RQST_DONE,	JCS_OPEN,	JCS_IDLE,	JCAS_02 },
    { JCI_RQST_FAIL,	JCS_OPEN,	JCS_IDLE,	JCAS_03 },
    { JCI_LSTN_RQST,	JCS_IDLE,	JCS_LSTN,	JCAS_04 },
    { JCI_RQST_DONE,	JCS_LSTN,	JCS_CONN,	JCAS_05 },
    { JCI_ABRT_RQST,	JCS_LSTN,	JCS_ABRT,	JCAS_14 },
    { JCI_RQST_FAIL,	JCS_LSTN,	JCS_IDLE,	JCAS_06 },
    { JCI_RECV_CONN,	JCS_CONN,	JCS_SEND,	JCAS_07 },
    { JCI_ABRT_RQST,	JCS_CONN,	JCS_ABRT,	JCAS_14 },
    { JCI_RQST_FAIL,	JCS_CONN,	JCS_DISC,	JCAS_16 },

    /*
    ** General processing.
    **
    ** Wait for client request.  We maintain a single active 
    ** receive request during the life of the connection, so
    ** explicit receive requests are not permitted.  Only a
    ** single send request may be active, so subsequent
    ** requests are queued until the active request completes.
    */
    { JCI_RECV_DATA,	JCS_RECV,	JCS_RECV,	JCAS_08 },
    { JCI_RECV_DISC,	JCS_RECV,	JCS_SHUT,	JCAS_12 },
    { JCI_RECV_INT,	JCS_RECV,	JCS_RECV,	JCAS_19 },
    { JCI_SEND_RQST,	JCS_RECV,	JCS_SEND,	JCAS_09 },
    { JCI_ABRT_RQST,	JCS_RECV,	JCS_ABRT,	JCAS_14 },
    { JCI_RQST_FAIL,	JCS_RECV,	JCS_DISC,	JCAS_16 },

    { JCI_RECV_DATA,	JCS_SEND,	JCS_SEND,	JCAS_08 },
    { JCI_RECV_DISC,	JCS_SEND,	JCS_SHUT,	JCAS_13 },
    { JCI_RECV_INT,	JCS_SEND,	JCS_SEND,	JCAS_19 },
    { JCI_SEND_RQST,	JCS_SEND,	JCS_SEND,	JCAS_10 },
    { JCI_SEND_DONE,	JCS_SEND,	JCS_RECV,	JCAS_17 },
    { JCI_SEND_QEUE,	JCS_SEND,	JCS_SEND,	JCAS_11 },
    { JCI_ABRT_RQST,	JCS_SEND,	JCS_ABRT,	JCAS_14 },
    { JCI_RQST_FAIL,	JCS_SEND,	JCS_DISC,	JCAS_16 },

    /*
    ** Shut down a connection.
    */
    { JCI_SEND_RQST,	JCS_SHUT,	JCS_SHUT,	JCAS_17 },
    { JCI_SEND_DONE,	JCS_SHUT,	JCS_DISC,	JCAS_16 },
    { JCI_SEND_QEUE,	JCS_SHUT,	JCS_SHUT,	JCAS_11 },
    { JCI_ABRT_RQST,	JCS_SHUT,	JCS_DISC,	JCAS_16 },
    { JCI_RQST_FAIL,	JCS_SHUT,	JCS_DISC,	JCAS_16 },

    { JCI_RECV_DATA,	JCS_ABRT,	JCS_ABRT,	JCAS_17 },
    { JCI_RECV_DISC,	JCS_ABRT,	JCS_ABRT,	JCAS_17 },
    { JCI_RECV_DONE,	JCS_ABRT,	JCS_ABRT,	JCAS_17 },
    { JCI_RECV_INT,	JCS_ABRT,	JCS_ABRT,	JCAS_17 },
    { JCI_SEND_RQST,	JCS_ABRT,	JCS_ABRT,	JCAS_17 },
    { JCI_SEND_DONE,	JCS_ABRT,	JCS_DISC,	JCAS_16 },
    { JCI_SEND_QEUE,	JCS_ABRT,	JCS_ABRT,	JCAS_11 },
    { JCI_ABRT_RQST,	JCS_ABRT,	JCS_DISC,	JCAS_16 },
    { JCI_RQST_FAIL,	JCS_ABRT,	JCS_DISC,	JCAS_16 },

    { JCI_RECV_DATA,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_RECV_DISC,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_RECV_DONE,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_RECV_INT,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_SEND_RQST,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_SEND_DONE,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_SEND_QEUE,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_RQST_FAIL,	JCS_DISC,	JCS_DISC,	JCAS_17 },
    { JCI_ABRT_RQST,	JCS_DISC,	JCS_IDLE,	JCAS_18 },
    { JCI_RQST_DONE,	JCS_DISC,	JCS_IDLE,	JCAS_18 },
};

static struct
{

    u_i1	next;
    u_i1	action;

} smt[ JCS_CNT ][ JCI_CNT ] ZERO_FILL; 

/* 
** Request queue.  
*/ 

static	bool	gcc_active = FALSE;
static	QUEUE	rcb_q;

/*
** Forward references.
*/

static	void		gcc_sm( JDBC_RCB *rcb );
static	void		gcc_eval( JDBC_RCB *rcb );
static	void		gcc_complete( PTR );


/*
** Name: gcc_request
**
** Description:
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

static void
gcc_request( JDBC_RCB *rcb )
{
    if ( gcc_active )
    {
	if ( JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC queueing request %s\n",
			rcb->ccb->id, inputs[ rcb->request ] );

	QUinsert( &rcb->q, rcb_q.q_prev );
	return;
    }

    gcc_active = TRUE;
    gcc_sm( rcb );

    while( (rcb = (JDBC_RCB *)QUremove( rcb_q.q_next )) )
    {
	if ( JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC processing queued event %s\n",
			rcb->ccb->id, inputs[ rcb->request ] );

	gcc_sm( rcb );
    }

    gcc_active = FALSE;
    return;
}


/*
** Name: gcc_sm
**
** Description:
**	State machine processing loop for the JDBC server GCC
**	processing.
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
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial TL connection info.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.
**	 4-Nov-99 (gordy)
**	    Added TL interrupts.
**	16-Nov-99 (gordy)
**	    Skip unknown connection parameters to permit
**	    future extensibility (client can't known our
**	    protocol level until we respond to the request).
**	13-Dec-99 (gordy)
**	    Negotiate GCC parameters prior to session start
**	    so that values are available to session layer.
*/

static void
gcc_sm( JDBC_RCB *rcb )
{
    JDBC_CCB	*ccb = rcb->ccb;
    GCC_P_PLIST	*p_list;
    STATUS	status;
    u_i1	i, next_state, action;

    gcc_eval( rcb );

    if ( ccb->gcc.state >= JCS_CNT  ||  rcb->request >= JCI_CNT  ||
	 smt[ ccb->gcc.state ][ rcb->request ].next >= JCS_CNT  ||
	 smt[ ccb->gcc.state ][ rcb->request ].action >= JCAS_CNT )
    {
	ER_ARGUMENT	args[2];

	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC sm invalid %d %d => %d %d\n",
			ccb->id, ccb->gcc.state, rcb->request,
			smt[ ccb->gcc.state ][ rcb->request ].next,
			smt[ ccb->gcc.state ][ rcb->request ].action );

	args[0].er_size = sizeof( rcb->request );
	args[0].er_value = (PTR)&rcb->request;
	args[1].er_size = sizeof( ccb->gcc.state );
	args[1].er_value = (PTR)&ccb->gcc.state;
	gcu_erlog( 0, JDBC_global.language, 
		   E_JD0103_TL_FSM_STATE, NULL, 2, (PTR)args );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC state %s input %s => %s\n", ccb->id, 
		   states[ ccb->gcc.state ], inputs[ rcb->request ],
		   states[ smt[ ccb->gcc.state ][ rcb->request ].next ] );

    action = smt[ ccb->gcc.state ][ rcb->request ].action;
    next_state = smt[ ccb->gcc.state ][ rcb->request ].next;

    for( i = 0; i < JCA_MAX; i++ )
    {
	if ( JDBC_global.trace_level >= 4  &&  act_seq[action][i] != JCA_NOOP )
	    TRdisplay( "%4d    JDBC action %s\n", ccb->id, 
		       actions[ act_seq[ action ][ i ] ] );

	switch( act_seq[ action ][ i ] )
	{
	case JCA_NOOP :	/* End of action sequence */
	    i = JCA_MAX;
	    break;

	case JCA_OPEN :	/* Open the GCC Listen port */
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->function_invoked = GCC_OPEN;
	    p_list->function_parms.open.port_id = ccb->gcc.pce->pce_port;
	    p_list->function_parms.open.lsn_port = ccb->gcc.pce->pce_port;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->pcb = NULL;
	    p_list->options = 0;
	    p_list->buffer_lng = 0;
	    p_list->buffer_ptr = NULL;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_OPEN, p_list );
	    if ( status != OK )
	    {
		ER_ARGUMENT	args[2];

		args[0].er_size = 4;
		args[0].er_value = "OPEN";
		args[1].er_size = sizeof( status );
		args[1].er_value = (PTR)&status;
		gcu_erlog( 0, JDBC_global.language, 
			    E_JD0105_NTWK_RQST_FAIL, NULL, 2, (PTR)args );

		ccb->gcc.pce->pce_flags &= ~PCT_OPEN;
		ccb->gcc.pce->pce_flags |= PCT_OPN_FAIL;
		GCshut();
	    }
	    break;

	case JCA_ODON :
	    {
		ER_ARGUMENT	args[2];
		char		port[ GCC_L_PORT * 2 + 5 ];

		if ( ! STcompare(ccb->gcc.pce->pce_port,
				 rcb->gcc.p_list.function_parms.open.lsn_port) )
		    STcopy( ccb->gcc.pce->pce_port, port );
		else
		    STprintf( port, "%s (%s)", ccb->gcc.pce->pce_port,
			      rcb->gcc.p_list.function_parms.open.lsn_port );

		args[0].er_size = STlength( ccb->gcc.pce->pce_pid );
		args[0].er_value = ccb->gcc.pce->pce_pid;
		args[1].er_size = STlength( port );
		args[1].er_value = port;
		gcu_erlog( 0, JDBC_global.language, 
			    E_JD0106_NTWK_OPEN, NULL, 2, (PTR)args );
	    }
	    break;
	
	case JCA_OERR :
	    {
		ER_ARGUMENT	args[2];

		args[0].er_size = 4;
		args[0].er_value = "OPEN";
		args[1].er_size = sizeof( rcb->gcc.p_list.generic_status );
		args[1].er_value = (PTR)&rcb->gcc.p_list.generic_status;
		gcu_erlog( 0, JDBC_global.language, 
			    E_JD0105_NTWK_RQST_FAIL, NULL, 2, (PTR)args );

		ccb->gcc.pce->pce_flags &= ~PCT_OPEN;
		ccb->gcc.pce->pce_flags |= PCT_OPN_FAIL;
		GCshut();
	    }
	    break;

	case JCA_NEWC :
	    {
		JDBC_CCB *lsn_ccb;
		JDBC_RCB *lsn_rcb;

		if ( ! (lsn_ccb = jdbc_new_ccb())  ||  
		     ! (lsn_rcb = jdbc_new_rcb( lsn_ccb )) )
		{
		    if ( lsn_ccb )  jdbc_del_ccb( lsn_ccb );
		    GCshut();
		    break;
		}

		lsn_ccb->gcc.pce = ccb->gcc.pce;
		lsn_rcb->request = JCI_LSTN_RQST;
		gcc_request( lsn_rcb );
	    }
	    break;

	case JCA_LSTN :
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->function_invoked = GCC_LISTEN;
	    p_list->function_parms.listen.port_id = ccb->gcc.pce->pce_port;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->pcb = NULL;
	    p_list->options = 0;
	    p_list->buffer_lng = 0;
	    p_list->buffer_ptr = NULL;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_LISTEN, p_list );
	    if ( status != OK )
	    {
		ER_ARGUMENT	args[2];

		args[0].er_size = 6;
		args[0].er_value = "LISTEN";
		args[1].er_size = sizeof( status );
		args[1].er_value = (PTR)&status;
		gcu_erlog( 0, JDBC_global.language, 
			    E_JD0105_NTWK_RQST_FAIL, NULL, 2, (PTR)args );

		ccb->gcc.pce->pce_flags &= ~PCT_OPEN;
		ccb->gcc.pce->pce_flags |= PCT_OPN_FAIL;
		GCshut();
	    }
	    break;

	case JCA_LERR :
	    {
		ER_ARGUMENT	args[2];

		args[0].er_size = 6;
		args[0].er_value = "LISTEN";
		args[1].er_size = sizeof( rcb->gcc.p_list.generic_status );
		args[1].er_value = (PTR)&rcb->gcc.p_list.generic_status;
		gcu_erlog( 0, JDBC_global.language, 
			    E_JD0105_NTWK_RQST_FAIL, NULL, 2, (PTR)args );

		ccb->gcc.pce->pce_flags &= ~PCT_OPEN;
		ccb->gcc.pce->pce_flags |= PCT_OPN_FAIL;
		GCshut();
	    }
	    break;

	case JCA_CCNF :	/* Read request, Negotiate params, prepare confirm */
	    {
		JDBC_TL_HDR	hdr;
		JDBC_TL_PARAM	param;
		u_i1		*ptr = rcb->buffer - JDBC_TL_HDR_SZ;
		u_i1		size = JDBC_TL_PKT_MIN;
		u_i1		*msg_param = NULL;
		u_i2		msg_len = 0;
		STATUS		status;

		while( rcb->buf_len >= (CV_N_I1_SZ * 2) )
		{
		    CV2L_I1_MACRO( &rcb->buffer[ rcb->buf_ptr ], 
				   &param.param_id, &status );
		    rcb->buf_ptr += CV_N_I1_SZ;
		    rcb->buf_len -= CV_N_I1_SZ;

		    CV2L_I1_MACRO( &rcb->buffer[ rcb->buf_ptr ], 
				   &param.pl1, &status );
		    rcb->buf_ptr += CV_N_I1_SZ;
		    rcb->buf_len -= CV_N_I1_SZ;

		    if ( param.pl1 < 255 )
			param.pl2 = param.pl1;
		    else  if ( rcb->buf_len >= CV_N_I2_SZ )
		    {
			CV2L_I2_MACRO( &rcb->buffer[ rcb->buf_ptr ], 
					&param.pl2, &status );
			rcb->buf_ptr += CV_N_I2_SZ;
			rcb->buf_len -= CV_N_I2_SZ;
		    }
		    else
		    {
			if ( JDBC_global.trace_level >= 1 )
			    TRdisplay("%4d    JDBC no extended param (%d,%d)\n",
				      ccb->id, rcb->buf_len, CV_N_I2_SZ );
			jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
			return;
		    }

		    if ( param.pl2 > rcb->buf_len )
		    {
			if ( JDBC_global.trace_level >= 1 )
			    TRdisplay( "%4d    JDBC no CR parameter (%d,%d)\n",
				       ccb->id, rcb->buf_len, param.pl2 );
			jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
			return;
		    }

		    switch( param.param_id )
		    {
		    case JDBC_TL_CP_PROTO :
			if ( param.pl2 == CV_N_I1_SZ )
			    CV2L_I1_MACRO( &rcb->buffer[ rcb->buf_ptr ], 
					   &ccb->gcc.proto_lvl, &status );
			else
			{
			    if ( JDBC_global.trace_level >= 1 )
				TRdisplay( "%4d    JDBC CR param length: %d\n",
					   ccb->id, param.pl2 );
			    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
			    return;
			}
			break;

		    case JDBC_TL_CP_SIZE :
			if ( param.pl2 == CV_N_I1_SZ )
			    CV2L_I1_MACRO( &rcb->buffer[ rcb->buf_ptr ], 
					   &size, &status );
			else
			{
			    if ( JDBC_global.trace_level >= 1 )
				TRdisplay( "%4d    JDBC CR param length: %d\n",
					   ccb->id, param.pl2 );
			    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
			    return;
			}
			break;

		    case JDBC_TL_CP_MSG :
			msg_param = &rcb->buffer[ rcb->buf_ptr ];
			msg_len = param.pl2;
			break;

		    default :
			if ( JDBC_global.trace_level >= 1 )
			    TRdisplay( "%4d    JDBC unknown CR param ID: %d\n",
				       ccb->id, param.param_id );
			break;
		    }

		    rcb->buf_ptr += param.pl2;
		    rcb->buf_len -= param.pl2;
		}

		if ( rcb->buf_len > 0 )
		{
		    if ( JDBC_global.trace_level >= 1 )
			TRdisplay("%4d    JDBC extra data for CR request: %d\n",
				  ccb->id, rcb->buf_len );
		    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
		    return;
		}

		if ( ccb->gcc.proto_lvl < JDBC_TL_PROTO_1  ||  
		     size < JDBC_TL_PKT_MIN )
		{
		    if ( JDBC_global.trace_level >= 1 )
			TRdisplay( "%4d    JDBC invalid param value: (%d,%d)\n",
				   ccb->id, ccb->gcc.proto_lvl, size );
		    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
		    return;
		}

		/*
		** Negotiate connetion parameters.
		*/
		if ( ccb->gcc.proto_lvl > JDBC_TL_DFLT_PROTO )
		    ccb->gcc.proto_lvl = JDBC_TL_DFLT_PROTO;
		if ( size > JDBC_TL_PKT_DEF )  size = JDBC_TL_PKT_DEF;
		ccb->max_buff_len = 1 << size;
		ccb->max_mesg_len = ccb->max_buff_len - JDBC_TL_HDR_SZ;

		status = jdbc_sess_begin( ccb, &msg_param, &msg_len );
		if ( status != OK )
		{
		    jdbc_gcc_abort( ccb, status );
		    return;
		}

		/*
		** Build connection confirmation message.
		*/
		rcb->buf_ptr = 0;
		rcb->buf_len = 0;
		hdr.tl_id = JDBC_TL_ID;
		hdr.msg_id = JDBC_TL_CC;

		CV2N_I4_MACRO( &hdr.tl_id, &ptr[0], &status );
		CV2N_I2_MACRO( &hdr.msg_id, &ptr[ CV_N_I4_SZ ], &status );

		param.param_id = JDBC_TL_CP_PROTO;
		CV2N_I1_MACRO( &param.param_id, 
				&rcb->buffer[ rcb->buf_len ], &status );
		rcb->buf_len += CV_N_I1_SZ;

		param.pl1 = 1;
		CV2N_I1_MACRO( &param.pl1, 
				&rcb->buffer[ rcb->buf_len ], &status );
		rcb->buf_len += CV_N_I1_SZ;

		CV2N_I1_MACRO( &ccb->gcc.proto_lvl, 
				&rcb->buffer[ rcb->buf_len ], &status );
		rcb->buf_len += CV_N_I1_SZ;

		param.param_id = JDBC_TL_CP_SIZE;
		CV2N_I1_MACRO( &param.param_id, 
				&rcb->buffer[ rcb->buf_len ], &status );
		rcb->buf_len += CV_N_I1_SZ;

		param.pl1 = 1;
		CV2N_I1_MACRO( &param.pl1, 
				&rcb->buffer[ rcb->buf_len ], &status );
		rcb->buf_len += CV_N_I1_SZ;

		CV2N_I1_MACRO( &size, &rcb->buffer[ rcb->buf_len ], &status );
		rcb->buf_len += CV_N_I1_SZ;

		if ( JDBC_global.charset  &&  *JDBC_global.charset )
		{
		    param.param_id = JDBC_TL_CP_CHRSET;
		    CV2N_I1_MACRO( &param.param_id, 
				    &rcb->buffer[ rcb->buf_len ], &status );
		    rcb->buf_len += CV_N_I1_SZ;

		    param.pl1 = STlength( JDBC_global.charset );
		    CV2N_I1_MACRO( &param.pl1, 
				    &rcb->buffer[ rcb->buf_len ], &status );
		    rcb->buf_len += CV_N_I1_SZ;

		    MEcopy( (PTR)JDBC_global.charset, param.pl1,
			    (PTR)&rcb->buffer[ rcb->buf_len ] );
		    rcb->buf_len += param.pl1;
		}

		if ( msg_len > 0  &&  msg_param )
		{
		    param.param_id = JDBC_TL_CP_MSG;
		    CV2N_I1_MACRO( &param.param_id, 
				    &rcb->buffer[ rcb->buf_len ], &status );
		    rcb->buf_len += CV_N_I1_SZ;

		    if ( msg_len < 255 )
		    {
			param.pl1 = msg_len;
			CV2N_I1_MACRO( &param.pl1, 
				       &rcb->buffer[ rcb->buf_len ], &status );
			rcb->buf_len += CV_N_I1_SZ;
		    }
		    else
		    {
			param.pl1 = 255;
			CV2N_I1_MACRO( &param.pl1, 
				       &rcb->buffer[ rcb->buf_len ], &status );
			rcb->buf_len += CV_N_I1_SZ;

			param.pl2 = msg_len;
			CV2N_I2_MACRO( &param.pl1, 
				       &rcb->buffer[ rcb->buf_len ], &status );
			rcb->buf_len += CV_N_I2_SZ;
		    }

		    MEcopy( (PTR)msg_param, msg_len, 
			    (PTR)&rcb->buffer[ rcb->buf_len ] );
		    rcb->buf_len += msg_len;
		}
	    }
	    break;

	case JCA_RECV :	/* Post a receive request on the client connection */
	    {
		JDBC_RCB *rcv_rcb;

		if ( ! (rcv_rcb = jdbc_new_rcb( ccb )) )
		{
		    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		    return;
		}

		rcv_rcb->request = JCI_RECV_RQST;
		p_list = &rcv_rcb->gcc.p_list;
		p_list->pce = ccb->gcc.pce;
		p_list->pcb = ccb->gcc.pcb;
		p_list->function_invoked = GCC_RECEIVE;
		p_list->compl_exit = gcc_complete;
		p_list->compl_id = (PTR)rcv_rcb;
		p_list->options = 0;

		/*
		** Space has been reserved in the buffer for
		** the NL and TL Headers.  The current buffer
		** points at the actual data, so allow room
		** for the TL Header.  The protocol driver
		** will handle the NL Header itself.
		*/
		p_list->buffer_lng = rcv_rcb->buf_max + JDBC_TL_HDR_SZ;
		p_list->buffer_ptr = (char *)rcv_rcb->buffer - JDBC_TL_HDR_SZ;

		/*
		** If IO has been paused, save the RCB for
		** posting the receive later.  Otherwise,
		** post the request now.
		*/
		if ( ccb->gcc.flags & JDBC_GCC_XOFF )
		{
		    ccb->gcc.xoff_rcb = (PTR)rcv_rcb;

		    if ( JDBC_global.trace_level >= 4 )
			TRdisplay( "%4d    JDBC recv delayed pending IB XON\n",
				   ccb->id );
		}
		else
		{
		    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_RECEIVE, 
								 p_list );
		    if ( status != OK )  
		    {
			if ( JDBC_global.trace_level >= 1 )
			    TRdisplay( "%4d    JDBC GCC receive error: 0x%x\n",
				       ccb->id, status );
			jdbc_gcc_abort( ccb, FAIL );
			return;
		    }
		}
	    }
	    break;

	case JCA_PROC : /* Process a data message */
	    jdbc_sess_process( rcb );
	    break;

	case JCA_INT : /* Interrupt processing */
	    jdbc_sess_interrupt( ccb );
	    break;

	case JCA_HEAD :	/* Build the TL Data Header */
	    {
		JDBC_TL_HDR	hdr;
		u_i1		*ptr = rcb->buffer + 
				       rcb->buf_ptr - JDBC_TL_HDR_SZ;
		STATUS		status;

		hdr.tl_id = JDBC_TL_ID;
		hdr.msg_id = JDBC_TL_DT;

		CV2N_I4_MACRO( &hdr.tl_id, &ptr[0], &status );
		CV2N_I2_MACRO( &hdr.msg_id, &ptr[ CV_N_I4_SZ ], &status );
	    }
	    break;

	case JCA_SEND :	/* Send a message on the client connection  */
	    rcb->request = JCI_SEND_RQST;
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->pcb = ccb->gcc.pcb;
	    p_list->function_invoked = GCC_SEND;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->options = GCC_FLUSH_DATA;
	    p_list->buffer_lng = rcb->buf_len + JDBC_TL_HDR_SZ;
	    p_list->buffer_ptr = (char *)(rcb->buffer + 
					  rcb->buf_ptr - JDBC_TL_HDR_SZ);

	    if ( JDBC_global.trace_level >= 5 )
	    {
		TRdisplay( "%4d    JDBC sending %d bytes\n",
			   ccb->id, p_list->buffer_lng );
		gcu_hexdump( p_list->buffer_ptr, p_list->buffer_lng );
	    }

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_SEND, p_list );
	    if ( status != OK )  
	    {
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC GCC send error: 0x%x\n",
			       ccb->id, status );
		jdbc_gcc_abort( ccb, FAIL );
		return;
	    }
	    break;

	case JCA_QEUE :	/* Place current request on send request queue */
	    QUinsert( &rcb->q, ccb->gcc.send_q.q_prev );
	    jdbc_msg_xoff( ccb, TRUE );	/* Disable outbound IO */
	    break;

	case JCA_DEQU :	/* Get next request on send request queue */
	    rcb = (JDBC_RCB *)QUremove( ccb->gcc.send_q.q_next );
	    ccb = rcb->ccb;

	    /* When the send queue is empty, enable outbound IO */
	    if ( ccb->gcc.send_q.q_next == &ccb->gcc.send_q )
		jdbc_msg_xoff( ccb, FALSE );
	    break;

	case JCA_FSRQ :	/* Flush send request queue */
	    {
		JDBC_RCB *frcb;

		while( (frcb = (JDBC_RCB *)QUremove(ccb->gcc.send_q.q_next)) )
		    jdbc_del_rcb( frcb );
		jdbc_msg_xoff( ccb, FALSE );	/* enable outbound IO */
	    }
	    break;

	case JCA_DCNF :	/* Build disconnect confirm message */
	    {
		JDBC_TL_HDR	hdr;
		u_i1		*ptr = rcb->buffer - JDBC_TL_HDR_SZ;
		STATUS		status;

		rcb->buf_ptr = 0;
		rcb->buf_len = 0;
		hdr.tl_id = JDBC_TL_ID;
		hdr.msg_id = JDBC_TL_DC;
		CV2N_I4_MACRO( &hdr.tl_id, &ptr[0], &status );
		CV2N_I2_MACRO( &hdr.msg_id, &ptr[ CV_N_I4_SZ ], &status );
	    }
	    break;

	case JCA_ABRT : /* Build disconnect request message */
	    {
		JDBC_TL_HDR	hdr;
		JDBC_TL_PARAM	param;
		u_i1		*ptr = rcb->buffer - JDBC_TL_HDR_SZ;
		STATUS		status;

		rcb->buf_ptr = 0;
		rcb->buf_len = 0;
		hdr.tl_id = JDBC_TL_ID;
		CV2N_I4_MACRO( &hdr.tl_id, &ptr[0], &status );
		hdr.msg_id = JDBC_TL_DR;
		CV2N_I2_MACRO( &hdr.msg_id, &ptr[ CV_N_I4_SZ ], &status );

		if ( rcb->gcc.p_list.generic_status != OK  &&
		     rcb->gcc.p_list.generic_status != FAIL )
		{
		    param.param_id = JDBC_TL_DP_ERR;
		    CV2N_I1_MACRO( &param.param_id,
				    &rcb->buffer[ rcb->buf_len ], &status );
		    rcb->buf_len += CV_N_I1_SZ;
		
		    param.pl1 = CV_N_I4_SZ;
		    CV2N_I1_MACRO( &param.pl1,
				    &rcb->buffer[ rcb->buf_len ], &status );
		    rcb->buf_len += CV_N_I1_SZ;
		
		    CV2N_I4_MACRO( &rcb->gcc.p_list.generic_status, 
				    &rcb->buffer[ rcb->buf_len ], &status );
		    rcb->buf_len += CV_N_I4_SZ;
		}
	    }
	    break;

	case JCA_DISC :	/* Disconnect */
	    /*
	    ** Shutdown the server connection
	    */
	    jdbc_sess_end( ccb );

	    /*
	    ** Shutdown the client connection.
	    */
	    rcb->request = JCI_DISC_RQST;
	    p_list = &rcb->gcc.p_list;
	    p_list->pce = ccb->gcc.pce;
	    p_list->pcb = ccb->gcc.pcb;
	    p_list->function_invoked = GCC_DISCONNECT;
	    p_list->compl_exit = gcc_complete;
	    p_list->compl_id = (PTR)rcb;
	    p_list->options = 0;
	    p_list->buffer_lng = 0;
	    p_list->buffer_ptr = NULL;

	    status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_DISCONNECT, 
							 p_list );

	    if ( status != OK )  
	    {
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC GCC disconnect error: 0x%x\n",
			       ccb->id, status );
		jdbc_gcc_abort( ccb, FAIL );
	    }
	    break;

	case JCA_SRCB : /* Save current RCB for abort requests */
	    ccb->gcc.abort_rcb = (PTR)rcb;
	    break;

	case JCA_FRCB :	/* Free the request control block */
	    jdbc_del_rcb( rcb );
	    break;

	case JCA_FCCB :	/* Free the client connection control block */
	    jdbc_del_ccb( ccb );
	    jdbc_del_rcb( rcb );	/* Since CCB gone, free RCB too */
	    return;
	}
    }

    ccb->gcc.state = next_state;

    return;
}


/*
** Name: gcc_eval
**
** Description:
**	Evaluate the request and adjust based on additional
**	state information.
**
**	Expands JCI_RECV_DONE based on type of message received.
**	Check JCI_SEND_DONE for queued send requests.
**
** Input:
**	rcb		Request control block.
**
** Output:
**	rcb->request	New request ID.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 4-Nov-99 (gordy)
**	    Added TL interrupts.
**      10-Apr-01 (loera01) Bug 104459
**          When an error is detected, set input to JCI_ABRT_RQST instead of
**	    JCI_REQUEST_FAIL.
*/

static void
gcc_eval( JDBC_RCB *rcb )
{
    GCC_P_PLIST	*p_list = &rcb->gcc.p_list;
    u_i1	orig = rcb->request;

    switch( rcb->request )
    {
    case JCI_RECV_DONE :
	if ( JDBC_global.trace_level >= 5 )
	{
	    TRdisplay( "%4d    JDBC received %d bytes\n",
		       rcb->ccb->id, p_list->buffer_lng );
	    gcu_hexdump( (char *)rcb->buffer - JDBC_TL_HDR_SZ, 
		       p_list->buffer_lng );
	}

	if ( p_list->buffer_lng < JDBC_TL_HDR_SZ )
	{
	    rcb->request = JCI_ABRT_RQST;
	    p_list->generic_status = E_JD010A_PROTOCOL_ERROR;
	}
	else
	{
	    JDBC_TL_HDR	hdr;
	    u_i1	*ptr = rcb->buffer - JDBC_TL_HDR_SZ;
	    STATUS	status;

	    CV2L_I4_MACRO( &ptr[ 0 ], &hdr.tl_id, &status );
	    CV2L_I2_MACRO( &ptr[ CV_N_I4_SZ ], &hdr.msg_id, &status );

	    rcb->buf_len = p_list->buffer_lng - JDBC_TL_HDR_SZ;
	    rcb->buf_ptr = 0;

	    if ( hdr.tl_id == JDBC_TL_ID )
	    {
		switch( hdr.msg_id )
		{
		case JDBC_TL_CR : rcb->request = JCI_RECV_CONN;	break;
		case JDBC_TL_DT : rcb->request = JCI_RECV_DATA;	break;
		case JDBC_TL_DR : rcb->request = JCI_RECV_DISC;	break;
		case JDBC_TL_DC : rcb->request = JCI_RECV_DONE;	break;
		case JDBC_TL_INT: rcb->request = JCI_RECV_INT;	break;

		default :
		    rcb->request = JCI_ABRT_RQST;
		    p_list->generic_status = E_JD010A_PROTOCOL_ERROR;
		    break;
		}
	    }
	    else
	    {
		rcb->request = JCI_ABRT_RQST;
		p_list->generic_status = E_JD010A_PROTOCOL_ERROR;
	    }
	}
	break;

    case JCI_SEND_DONE :
	if ( rcb->ccb->gcc.send_q.q_next != &rcb->ccb->gcc.send_q )
	    rcb->request = JCI_SEND_QEUE;
	break;
    }

    if ( orig != rcb->request  &&  JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC gcc_eval %s => %s\n", 
		   rcb->ccb->id, inputs[ orig ], inputs[ rcb->request ] );

    return;
}


/*
** Name: gcc_complete
**
** Description:
**	Callback routine to handle GCC completions.
**
** Input:
**	ptr	Completion ID, a request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Update client idle limit.
**   	10-Apr-01 (loera01) Bug 104459
**          Change default error input to JCI_ABRT_RQST instead of JCI_RQST_FAIL
**          and set error status.
**          
*/

static void
gcc_complete( PTR ptr )
{
    JDBC_RCB	*rcb = (JDBC_RCB *)ptr;
    JDBC_CCB	*ccb = rcb->ccb;
    GCC_P_PLIST	*p_list = &rcb->gcc.p_list;
    u_i1	orig = rcb->request;

    /*
    ** Update the idle time limit on any network completion.
    */
    if ( ! JDBC_global.client_idle_limit )
	ccb->gcc.idle_limit.TM_secs = 0;
    else
    {
	TMnow( &ccb->gcc.idle_limit );
	ccb->gcc.idle_limit.TM_secs += JDBC_global.client_idle_limit;
    }

    /*
    ** Check for request failure or convert request
    ** type to the associated request completion.  
    **
    ** We don't care if a disconnect request fails
    ** and we need to distinguish between send/recv 
    ** failures and the disconnect completion, so 
    ** always return JCI_RQST_DONE in this case.
    */
    if ( p_list->generic_status != OK )
	rcb->request = (rcb->request == JCI_DISC_RQST)
		       ? JCI_RQST_DONE : JCI_RQST_FAIL;
    else
	switch( rcb->request )
	{
	case JCI_RECV_RQST : rcb->request = JCI_RECV_DONE; break;
	case JCI_SEND_RQST : rcb->request = JCI_SEND_DONE; break;

	case JCI_LSTN_RQST :
	    /* TOOD: need DISC (pcb) if LSTN fails? */
	    rcb->ccb->gcc.pcb = p_list->pcb;
	    rcb->request = JCI_RQST_DONE; 
   	    break;

	case JCI_OPEN_RQST : 
	case JCI_DISC_RQST :
	    rcb->request = JCI_RQST_DONE; 
	    break;
    
	default :
	    p_list->generic_status = E_JD0109_INTERNAL_ERROR;
	    rcb->request = JCI_ABRT_RQST;
	    return;
	}

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC GCC completion %s => %s, 0x%x\n", 
		   rcb->ccb->id, inputs[ orig ], inputs[ rcb->request ], 
		   p_list->generic_status );

    gcc_request( rcb );

    return;
}


/*
** Name: jdbc_gcc_init
**
** Description:
**	Initialize the JDBC server GCC interface.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Queue added for active CCBs.
**	11-Jan-00 (gordy)
**	    Initialize the client check time.  Config file info
**	    checked when loaded.
**	 3-Mar-00 (gordy)
**	    Moved CCB init to general initializations.
*/

STATUS
jdbc_gcc_init( void )
{
    STATUS	status;
    CL_ERR_DESC	cl_err;
    JDBC_CCB	*ccb;
    JDBC_RCB	*rcb;
    u_i1	i;

    QUinit( &rcb_q );
    MEfill( sizeof( smt ), 0xff, (PTR)smt );

    if ( JDBC_global.client_idle_limit )
    {
	TMnow( &JDBC_global.client_check );
	JDBC_global.client_check.TM_secs += JDBC_global.client_idle_limit;
    }

    for( i = 0; i < ARR_SIZE( sm_info ); i++ )
    {
	smt[ sm_info[i].state ][ sm_info[i].input ].next = sm_info[i].next;
	smt[ sm_info[i].state ][ sm_info[i].input ].action = sm_info[i].action;
    }

    if ( GCpinit( &gcc_pct, &status, &cl_err ) != OK )
    {
	gcu_erlog( 0, JDBC_global.language, status, &cl_err, 0, NULL );
	return( status );
    }

    for( i = 0; i < gcc_pct->pct_n_entries; i++ )
    {
	GCC_PCE *pce = &gcc_pct->pct_pce[ i ];
	
	if ( pce->pce_flags & PCT_NO_PORT  ||
	     STbcompare( JDBC_global.protocol, 0, pce->pce_pid, 0, TRUE ) )  
	{
	    pce->pce_flags |= PCT_OPN_FAIL;
	    continue;
	}

	pce->pce_flags |= PCT_OPEN;
	STcopy( JDBC_global.port, pce->pce_port );

	if ( ! (ccb = jdbc_new_ccb())  ||  ! (rcb = jdbc_new_rcb( NULL )) )
	{
	    if ( ccb )  jdbc_del_ccb( ccb );
	    return( E_JD0108_NO_MEMORY );
	}

	ccb->gcc.pce = pce;
	rcb->ccb = ccb;
	rcb->request = JCI_OPEN_RQST;
	gcc_request( rcb );
    }

    return( OK );
}


/*
** Name: jdbc_gcc_term
**
** Description:
**	Shutdown the GCC interface.
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
**	 3-Jun-99 (gordy)
**	    Created.
*/

void
jdbc_gcc_term( void )
{
    STATUS	status;
    CL_ERR_DESC	cl_err;

    GCpterm( gcc_pct, &status, &cl_err );
    gcc_pct = NULL;

    return;
}


/*
** Name: jdbc_gcc_send
**
** Description:
**	Send a GCC message.
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

void
jdbc_gcc_send( JDBC_RCB *rcb )
{
    rcb->request = JCI_SEND_RQST;
    gcc_request( rcb );
    return;
}


/*
** Name: jdbc_idle_check
**
** Description:
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
**	21-Dec-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Make sure it is time to do idle check.  Clients idle
**	    limit is initialized if 0 and ignored if negative.
**	    Adjust idle check for next client to reach limit.
*/

void
jdbc_idle_check( void )
{
    QUEUE	*q, *next;
    SYSTIME	now;

    /*
    ** Is it time to check the clients?
    */
    TMnow( &now );
    if ( ! JDBC_global.client_idle_limit  ||  
	 TMcmp( &now, &JDBC_global.client_check ) < 0 )  return;

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC checking for idle clients\n", -1 );

    /*
    ** Set our next check time to one full period.
    ** If there are any clients which will timeout
    ** prior to that time, we adjust the next check
    ** time accordingly below.
    */
    STRUCT_ASSIGN_MACRO( now, JDBC_global.client_check );
    JDBC_global.client_check.TM_secs += JDBC_global.client_idle_limit;

    for( q = JDBC_global.ccb_q.q_next; q != &JDBC_global.ccb_q; q = next )
    {
	JDBC_CCB *ccb = (JDBC_CCB *)q;
	next = q->q_next;

	/*
	** If idle limit time is 0 then the connection
	** is initializing and we initialize the idle 
	** limit.  If idle limit time is negative then 
	** the connection is being aborted and we do 
	** nothing.  Otherwise, check to see if client
	** has exceeded its idle limit.
	*/
	if ( ! ccb->gcc.idle_limit.TM_secs )
	{
	    TMnow( &ccb->gcc.idle_limit );
	    ccb->gcc.idle_limit.TM_secs += JDBC_global.client_idle_limit;
	}
	else  if (  ccb->gcc.idle_limit.TM_secs > 0 )
	{  
	    if ( TMcmp( &now, &ccb->gcc.idle_limit ) >= 0 )
		jdbc_gcc_abort( ccb, E_JD010D_IDLE_LIMIT );
	    else  
	    {
		if ( JDBC_global.trace_level >= 5 )
		    TRdisplay( "%4d    JDBC Idle %d seconds\n", ccb->id, 
			       JDBC_global.client_idle_limit -
			       (ccb->gcc.idle_limit.TM_secs - now.TM_secs) );

		/*
		** If this client will timeout prior to our
		** next check time, reset the check time to
		** match this client.
		*/
		if ( TMcmp( &ccb->gcc.idle_limit, 
			    &JDBC_global.client_check ) < 0 )
		{
		    STRUCT_ASSIGN_MACRO( ccb->gcc.idle_limit,
					 JDBC_global.client_check );
		}
	    }
	}
    }

    return;
}


/*
** Name: jdbc_gcc_xoff
**
** Description:
**	Enable or disable inbound IO on a client connection.
**
** Input:
**	ccb	Connection control block.
**	pause	TRUE to pause IO, FALSE to continue.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-Oct-99 (gordy)
**	    Created.
*/

void
jdbc_gcc_xoff( JDBC_CCB *ccb, bool pause )
{
    if ( pause )
    {
	if ( ! (ccb->gcc.flags & JDBC_GCC_XOFF)  &&
	     JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC IB XOFF requested\n", ccb->id );

	ccb->gcc.flags |= JDBC_GCC_XOFF;
    }
    else  if ( ! ccb->gcc.xoff_rcb )
    {
	if ( ccb->gcc.flags & JDBC_GCC_XOFF  &&  
	     JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC IB XON requested\n", ccb->id );

	ccb->gcc.flags &= ~JDBC_GCC_XOFF;
    }
    else
    {
	GCC_P_PLIST	*p_list = &((JDBC_RCB *)ccb->gcc.xoff_rcb)->gcc.p_list; 
	STATUS		status;

	if ( JDBC_global.trace_level >= 4 )
	    TRdisplay( "%4d    JDBC IB XON requested: posting recv\n",
		       ccb->id );

	ccb->gcc.flags &= ~JDBC_GCC_XOFF;
	ccb->gcc.xoff_rcb = NULL;
	status = (*ccb->gcc.pce->pce_protocol_rtne)( GCC_RECEIVE, p_list );

	if ( status != OK )  
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC GCC receive error: 0x%x\n",
			   ccb->id, status );
	    jdbc_gcc_abort( ccb, FAIL );
	    return;
	}
    }

    return;
}


/*
** Name: jdbc_gcc_abort
**
** Description:
**	Abort a connection.
**
** Input:
**	ccb	Connection control block.
**	status	Error status.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	21-Dec-99 (gordy)
**	    Reset idle limit to avoid contention.
**	17-Jan-00 (gordy)
**	    Idle limit reset is now negative rather than 0
**	    (0 indicates an uninitialized control block).
*/

void
jdbc_gcc_abort( JDBC_CCB *ccb, STATUS status )
{
    JDBC_RCB	*rcb = (JDBC_RCB *)ccb->gcc.abort_rcb;

    /*
    ** By reseting the idle limit we avoid any 
    ** contention with background processing.
    */
    ccb->gcc.idle_limit.TM_secs = -1;

    if ( status != OK  &&  status != FAIL )
    {
	gcu_erlog(0, JDBC_global.language, E_JD0107_CONN_ABORT, NULL, 0, NULL);
	gcu_erlog(0, JDBC_global.language, status, NULL, 0, NULL);
    }

    if ( rcb )
    {
	ccb->gcc.abort_rcb = NULL;
	rcb->request = JCI_ABRT_RQST;
	rcb->gcc.p_list.generic_status = status;
	gcc_request( rcb );
    }
    
    return;
}

