/*
**    Copyright (c) 1987, 2003 Ingres Corporation
*/
 
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<me.h>
# include	<st.h>
# include	<tr.h>
# include	<nm.h>
# include	<cv.h>
# include       <descrip.h>
# include       <snalu0def.h>
# include       <snalibdef.h>
# include       <stsdef.h>
# include 	<gcccl.h>
# include	<gcatrace.h>
# include	<lib$routines.h>

/*
** GCSNALU0.C - SNA LU0 protocol driver for INGRES/NET
**
** Description:
**	This file implements the SNA LU0 protocol driver for INGRES/NET.
**	It consists of a protocol driver entry point GCsnalu0, a state
**	machine table snalu0_states, a state machine snalu0_sm(), and a 
**	few ancilliary support routines.
**
**	    GCsnalu0() - SNA LU0 protocol driver entry point
**
**	    SNA_ADDR - SNA address, in terms of string descriptors
**	    SNA_LPCB - SNA listen control block, one per protocol driver
**	    SNA_PCB - SNA protocol control block, one per connection
**
**	    snalu0_states - state table for SNA LU0 state machine
**	    snalu0_acts - action names for tracing
**
**	    snalu0_sm() - SNA LU0 state machine
**	    snalu0_notify() - handle SNA asyncronous notify
**	    snalu0_lsnaddr() - parse an sna listen address 
**	    snalu0_conaddr() - parse an sna connect address 
**
**	SNA Connection Establishment Parameters
**
**                Node   Access  Sess-addr user-data  bind
**      -------------------------------------------------------------
**	(for listen)
**      Passive | node | port  | session  |JEFF0000 | 0x31...JEFFXXXX
**      Connect | node | port  |          |JEFFXXXX | 0x31...JEFFXXXX
**
**	(for connect)
**      Connect | node | port  |          |JEFF0000 | 0x31...JEFFXXXX
**
** History:
**	27-Feb-92 (seiwald)
**	    Written to replace previous version.
**	30-Apr-92 (eric) bug #45949
**	    Increased pcb buffer sizes to be at least 4103 on transfer
**	    operations to correct the problem of specifing a buffer that
**	    is off the end of the address space.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	    clean up compiler warnings.
**      12-dec-2002 (loera01)  SIR 109237
**          In GCsnalu0(), set the options field in GCC_P_PLIST to zero 
**          (remote peer).
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
*/

/*
**  Forward and/or External function references.
*/

typedef i4 VMS_STATUS;


FUNC_EXTERN	STATUS		MEfree();

static		VOID		snalu0_sm();
static 		VOID  		snalu0_notify();	
static		VOID		snalu0_lsnaddr();
static		VOID		snalu0_conaddr();

/*
**  Defines of other constants.
*/
# define SNA_BUFFER_COPY( len, source, destination ) \
		  MEcopy( source, len, destination );

# define INIT_STATIC_DESCRIPTOR( dsc, buffer ) \
				 dsc.dsc$w_length = sizeof( buffer ); \
				 dsc.dsc$b_dtype = DSC$K_DTYPE_T; \
				 dsc.dsc$b_class = DSC$K_CLASS_S; \
				 dsc.dsc$a_pointer = buffer;

# define GCTRACE(n) if( snalu0_trace >= n )(void)TRdisplay

# define VMS_OK( status )	( status & 1 )
# define SNA_OK( status ) 	( status & STS$M_SUCCESS )

# define    SNA_IOSV_STATUS( iosv ) ( ((VMS_STATUS *)iosv)[1] )

# define    SNA_MAGIC		"JEFF"
# define    SNA_L_MAGIC		8
# define    SNA_RU_SIZE		4096
# define    SNA_RU_HEADER	7
# define    SNA_K_MAXRUSIZE	( ((SNA_RU_SIZE + SNA_RU_HEADER) + 64) & ~0x3F )

/*
** Name: SNA_ADDR - SNA address, in terms of string descriptors
**
** Description:
** 	This structure contains the string descriptors which point into
** 	a string buffer containing the actual string.
*/

typedef struct _SNA_ADDR
{
    struct dsc$descriptor_s	node_dsc;
    struct dsc$descriptor_s	access_dsc;		    
    int				session_number;	   
} SNA_ADDR;

/*
** Name: SNA_LPCB - SNA listen control block, one per protocol driver
**
** Description:
**	This structure holds the state information necessary for the
**	whole SNA protocol driver, without regard for any particular
**	connection.
*/

typedef struct _SNA_LPCB
{
    char			port_buffer[ 64 ];
    STATUS	    		first_listen;	    /* flag for open failed */
    i4	    		sna_efn;	    /* sna event flag */
} SNA_LPCB;

/*
** Name: SNA_PCB - SNA protocol control block, one per connection
**
** Description:
**	This structure holds the state information for each connection,
**	including some information for each outstanding I/O operation.
**	N.B.  each GCC_P_PLIST also contains some state information for 
**	the particular I/O operation.
*/

typedef struct _SNA_PCB
{
    int				session_id;	    /* session id */
    i4				evt_term;	    /* evt_term received flag */
    i4				io_count;	    /* read/write oustanding */
    GCC_P_PLIST			*disconn;	    /* pending disconn */

    SNA_ADDR			sna_addr;	    /* connect address */

    int				ru_l;	    	    /* length of ru buffer */
    int				su_l;	    	    /* length of su buffer */

    struct dsc$descriptor_s     ru_dsc;	    	    /* request/response	*/
    struct dsc$descriptor_s     su_dsc;	    	    /* request/response	*/
    struct dsc$descriptor_s	user_data_dsc;	    /* connect user data */
    struct dsc$descriptor_s	notify_dsc;	    /* needed on connect */
    struct dsc$descriptor_s	conn_stat_dsc;	    /* connection status */
    struct dsc$descriptor_s	send_stat_dsc;	    /* send status */
    struct dsc$descriptor_s	recv_stat_dsc;	    /* receive status */

    /* state we must hold for the SNA API */

    struct {
	i4			sense;
	int			f_seqno;
	int			l_seqno;
	int			req_ind;
	int			more_data;
    } sna_state;

    /* Buffers for above descriptors */

    char			conn_stat_buffer[ snalu0$k_min_status_vector ];
    char			send_stat_buffer[ snalu0$k_min_status_vector ];
    char			recv_stat_buffer[ snalu0$k_min_status_vector ];

    char			user_data_buffer[ SNA_L_MAGIC ];
    char			notify_buffer[ snalu0$k_min_notify_vector ]; 
    char			bind_buffer[ SNA_K_MAXRUSIZE ];
    char			sdt_buffer[ SNA_K_MAXRUSIZE ];

} SNA_PCB;

/*
** Definition of all global variables owned by this file.
*/

static		i4 		snalu0_trace = 0;

static		VMS_STATUS	(*sna_rcv_msg)();
static		VMS_STATUS	(*sna_rqst_con)();
static		VMS_STATUS	(*sna_rqst_dis)();
static		VMS_STATUS	(*sna_xmit_msg)();
static		VMS_STATUS	(*sna_xmit_rsp)();


/*
** Name: snalu0_states - state table for SNA LU0 state machine
**	 snalu0_acts - action names for tracing
**
** Description:
**	The body of snalu0_sm() is a simple state machine controlled by
**	the array of states snalu0_states.  Each state has one action,
**	as defined below (and named by snalu0_acts for tracing
**	purposes), and two potential next states based on the outcome
**	of the action.
**
**	So that some actions may be async (returning immediately and 
**	signaling completion by reinvoking snalu0_sm()), the code
**	which selects the next state is at the top of snalu0_sm().
**	The state machine is then a loop as such:
**
**	     top:
**		branch to next state
**		print tracing info
**		run action for current state
**		goto top
**
**	(State 0 has no action.)
**
**	The current state of the SNA LU0 state machine is stored in
**	parms->state.
**
** History:
**	16-Nov-89 (seiwald)
**	    Created.
*/

/* Actions in snalu0_sm. */

# define	SNA_OPEN	1	/* entry for GCC_OPEN */
# define	SNA_LISTEN	2	/* entry for GCC_LISTEN */
# define	SNA_CONNECT	3	/* entry for GCC_CONNECT */
# define	SNA_SEND	4	/* entry for GCC_SEND */
# define	SNA_RECEIVE	5	/* entry for GCC_RECEIVE */
# define	SNA_DISCONNECT	6	/* entry for GCC_DISCONNECT */
# define	SNA_NEWPCB	7	/* intialize protocol control block */
# define	SNA_RCV_SDT	8	/* issue receive for SDT */
# define	SNA_RQST_DIS	9	/* issue sna disconnect */
# define	SNA_RQST_CON	10	/* issue sna connect (active) */
# define	SNA_XMIT_POS	11	/* issue sna xmit_msg positive */
# define	SNA_XMIT_NEG	12	/* issue sna xmit_msg negative */
# define	SNA_LISTEN1	13	/* look for magic chars in bind */
# define	SNA_SEND1	14	/* get result of send */
# define	SNA_RECEIVE1	15	/* get result of receive */
# define	SNA_COMPLETE	16	/* OPEN,LISTEN,CONNECT complete */
# define	SNA_RWCOMPLETE	17	/* SEND,RECEIVE complete */
# define	SNA_DISCOMPLETE	18	/* DISCONNECT complete */

/* Action names for tracing. */

char *snalu0_acts[] = {
	"NONE", "OPEN", "LISTEN", "CONNECT", "SEND", "RECEIVE",
	"DISCONNECT", "NEWPCB", "RCV_SDT", "RQST_DIS", "RQST_CON",
	"XMIT_POS", "XMIT_NEG", "LISTEN1", "SEND1", "RECEIVE1",
	"COMPLETE", "RWCOMPLETE", "DISCOMPLETE" 
} ;

static struct {
	char		action;
	char		next[2];
} snalu0_states[] = {
    /* state	action		next	branch	*/

    /* GCC_OPEN */

    /* 0 */	0,		1, 	1,	
    /* 1 */	SNA_OPEN,	22,	22,

    /* GCC_LISTEN */

    /* 2 */	0,		3,	3,
    /* 3 */	SNA_NEWPCB,	4,	22,
    /* 4 */	SNA_LISTEN,	5,	22,
    /* 5 */	SNA_LISTEN1,	6,	21,	/* reject */
    /* 6 */	SNA_XMIT_POS,	7,	22,
    /* 7 */	SNA_RCV_SDT,	8,	22,
    /* 8 */	SNA_XMIT_POS,	9,	22,
    /* 9 */	SNA_RQST_DIS,	10,	22,
    /* 10 */	SNA_RQST_CON,	11,	22,
    /* 11 */	SNA_XMIT_POS,	12,	22,
    /* 12 */	SNA_RCV_SDT,	13,	22,
    /* 13 */	SNA_XMIT_POS,	22,	22,

    /* GCC_CONNECT */

    /* 14 */	0,		15,	15,
    /* 15 */	SNA_NEWPCB,	16,	22,
    /* 16 */	SNA_CONNECT,	17,	22,
    /* 17 */	SNA_RQST_CON,	18,	22,
    /* 18 */	SNA_XMIT_POS,	19,	22,
    /* 19 */	SNA_RCV_SDT,	20,	22,
    /* 20 */	SNA_XMIT_POS,	22,	22,

    /* reject connection */

    /* 21 */	SNA_XMIT_NEG,	22,	22,

    /* GCC_LISTEN, GCC_CONNECT complete */

    /* 22 */	SNA_COMPLETE,	0,	0,

    /* GCC_SEND */

    /* 23 */	0,		24,	24,
    /* 24 */	SNA_SEND,	25,	30,
    /* 25 */	SNA_SEND1,	30,	30,

    /* GCC_RECEIVE */

    /* 26 */	0,		27,	27,
    /* 27 */	SNA_RECEIVE,	28,	30,
    /* 28 */	SNA_RECEIVE1,	29,	30,
    /* 29 */	SNA_XMIT_POS,	30,	30,

    /* GCC_SEND/GCC_RECEIVE complete */

    /* 30 */	SNA_RWCOMPLETE,	0,	0,

    /* GCC_DISCONNECT */

    /* 31 */	0,		32,	32,
    /* 32 */	SNA_DISCONNECT,	33,	34,
    /* 33 */	SNA_RQST_DIS,	34,	34,
    /* 34 */	SNA_DISCOMPLETE, 0,	0,
} ;

/*
** Name: GCsnalu0() - SNA LU0 protocol driver entry point
**
** Description:
**	This is the exported interface for the SNA LU0 protocol driver,
**	conforming to the INGRES/NET protocol driver interface.
**
**	This routine merely sets up the state number for snalu0_sm().
**	Note the use of hardwired state numbers.
**
** Inputs:
**	func_code	what operation to perform
**			duplicates parms->function_invoked
**	parms		parameter list for life of operation
**
** Returns:
**	OK
*/

STATUS
GCsnalu0( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{
    /*
    ** Compute initial state based on function code.
    */

    /*
    **  Client is always remote.
    */
    parms->options = 0;

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = 0; break;
    case GCC_LISTEN: 	parms->state = 2; break;
    case GCC_CONNECT: 	parms->state = 14; break;
    case GCC_SEND: 	parms->state = 23; break;
    case GCC_RECEIVE: 	parms->state = 26; break;
    case GCC_DISCONNECT:parms->state = 31; break;
    }

    parms->generic_status = OK;
    parms->system_status.error = 0;

    /* Start sm. */

    snalu0_sm( &parms );

    return OK;
}

/*
** Name: snalu0_sm() - SNA LU0 state machine
**
** Description:
**	This is the SNA LU0 state machine, which orchestrates all SNA
**	operations.  It is programmed by the state table snalu0_states.
**
**	N.B.  pparms must be a pointer to the parameter list, because
**	snalu0_sm() is the AST completion routine for SNA API calls,
**	and the SNA API insists on passing the AST completion parameter
**	"by reference," which means it dereferences the completion
**	parameter, stores it somewhere, and then calls the completion 
**	routine with the address of the stored completion parameter.
**
** Inputs:
**	pparms	pointer to parameter list 
*/

static VOID
snalu0_sm( pparms )
GCC_P_PLIST	**pparms;
{
    GCC_P_PLIST			*parms = *pparms;
    SNA_PCB			*pcb = (SNA_PCB *)parms->pcb;
    SNA_LPCB			*lpcb = (SNA_LPCB *)parms->pce->pce_dcb;
    bool			frombottom = 0;
    VMS_STATUS			status;
    i4				branch;

    /* Allocate per driver control block */

    if( !lpcb )
    {
	lpcb = (SNA_LPCB *)MEreqmem( 0, sizeof( *lpcb ), TRUE, (STATUS *)0 );
	parms->pce->pce_dcb = (PTR)lpcb;
    }

    if( !lpcb )
    {
	parms->generic_status = GC_OPEN_FAIL;
	(*parms->compl_exit)(parms->compl_id);
	return;
    }

    /*
    ** Pick up status of SNA I/O call.
    */

    switch( snalu0_states[ parms->state ].action )
    {
    case SNA_XMIT_POS:
	    /* Ignore status if evt_term set during xmit_pos */

	    if( pcb->evt_term )
		break;

	    /* fall through */

    case SNA_LISTEN:
    case SNA_XMIT_NEG:
    case SNA_RCV_SDT:
    case SNA_RQST_DIS:
    case SNA_RQST_CON:
	{
	    status = SNA_IOSV_STATUS( pcb->conn_stat_buffer );

	    /* Check if unbound */

	    if( pcb->evt_term )
		status = SNA$_UNBINDREC;

	    if( status == SNA$_SESIN_USE )
		/* maybe retry??? XXX */;

	    if( !SNA_OK( status ) )
	    {
		parms->generic_status = GC_NTWK_STATUS;
		parms->system_status.error = status;
	    }
	    break;
	}

    case SNA_SEND:
	{
	    status = SNA_IOSV_STATUS( pcb->send_stat_buffer );

	    /* Check if unbound */

	    if( pcb->evt_term )
		status = SNA$_UNBINDREC;

	    if( !SNA_OK( status ) )
	    {
		parms->generic_status = GC_SEND_FAIL;
		parms->system_status.error = status;
	    }
	    break;
	}

    case SNA_RECEIVE:
	{
	    status = SNA_IOSV_STATUS( pcb->recv_stat_buffer );

	    /* Check if unbound */

	    if( pcb->evt_term )
		status = SNA$_UNBINDREC;

	    if( !SNA_OK( status ) )
	    {
		parms->generic_status = GC_RECEIVE_FAIL;
		parms->system_status.error = status;
	    }
	    break;
	}
    }

    /* main loop of execution */

top:
    /*
    ** Go to next selected state.
    */

    branch = parms->generic_status != OK;
    parms->state = snalu0_states[ parms->state ].next[ branch ];

    GCTRACE(2)( "snalu0: %x state %2d act %s status %x %s\n",
			pcb,
			parms->state,
			snalu0_acts[ snalu0_states[ parms->state ].action ],
			parms->generic_status,
			frombottom ? "loop" : "" );

    /*
    ** Peform this state's action.
    */

    switch( snalu0_states[ parms->state ].action )
    {
    case SNA_OPEN:
	/*
	** Register before the first listen.
	*/
	{
	    int	i;
	    char *trace;

	    $DESCRIPTOR( sna_image_dsc, "SNALU0SHR" );

	    static struct {
		    int	(**funcptr)();
		    char	*name;
	    } sna_func_vec[ 5 ] = {
		    &sna_rcv_msg,   "SNALU0$RECEIVE_MESSAGE",
		    &sna_rqst_con,  "SNALU0$REQUEST_CONNECT",
		    &sna_rqst_dis,	"SNALU0$REQUEST_DISCONNECT",
		    &sna_xmit_msg,  "SNALU0$TRANSMIT_MESSAGE",
		    &sna_xmit_rsp, 	"SNALU0$TRANSMIT_RESPONSE"
	    } ;

	    NMgtAt( "II_GCSNA_TRACE", &trace );

	    if( trace && *trace )
		CVan( trace, &snalu0_trace );

	    /*
	    ** Declare an exception handler, since RTL signals errors.
	    ** Note that LIB$ESTABLISH does not return any value.
	    */

	    lib$establish( lib$sig_to_ret );

	    for( i = 0; i < 5; i++ )
	    {
		struct dsc$descriptor_s sym_dsc;

		/*
		* dynamically load the symbol....
		*/

		sym_dsc.dsc$a_pointer = sna_func_vec[ i ].name;
		sym_dsc.dsc$w_length = STlength( sym_dsc.dsc$a_pointer );
		sym_dsc.dsc$b_class = DSC$K_CLASS_S;
		sym_dsc.dsc$b_dtype = DSC$K_DTYPE_T;

		status = lib$find_image_symbol(
			     &sna_image_dsc, 
			     &sym_dsc, 
			     sna_func_vec[ i ].funcptr );

		if( !VMS_OK( status ) )
		    goto syserr;
	    }

	    /*
	    ** Get an event flag.
	    */ 

	    status = lib$get_ef( &lpcb->sna_efn );

	    if( !VMS_OK( status ) )
		goto syserr;

	    /* 
	    ** If no listen port is given, we won't accept connections.
	    */

	    if( !STcompare( parms->function_parms.open.port_id, "0" ) )
	    {
		STcopy( "none", lpcb->port_buffer );
		parms->pce->pce_flags |= PCT_NO_PORT;
	    }
	    else
	    {
		/* Save the listen address where we can trust it. */

		STcopy( parms->function_parms.open.port_id, lpcb->port_buffer );
	    }

	    /* Note protocol is open and print message to that effect. */

	    GCTRACE(1)("snalu0: Listen port [%s]\n", lpcb->port_buffer);
	    parms->function_parms.open.lsn_port = lpcb->port_buffer;

	    break;

	syserr:
	    parms->generic_status = GC_OPEN_FAIL;
	    parms->system_status.error = status;
	    break;
	}

    case SNA_NEWPCB:
	/*
	** Allocated a PCB.
	*/
	{
	    pcb = (SNA_PCB *)MEreqmem( 0, sizeof( *pcb ), TRUE, (STATUS *)0 );
	    parms->pcb = (PTR)pcb;

	    if( !pcb )
	    {
		parms->generic_status = GC_LISTEN_FAIL;
		break;
	    }

	    /* Initialize string descriptor pieces. */

	    INIT_STATIC_DESCRIPTOR( pcb->notify_dsc, pcb->notify_buffer );
	    INIT_STATIC_DESCRIPTOR( pcb->user_data_dsc, pcb->user_data_buffer );
	    INIT_STATIC_DESCRIPTOR( pcb->conn_stat_dsc, pcb->conn_stat_buffer );
	    INIT_STATIC_DESCRIPTOR( pcb->send_stat_dsc, pcb->send_stat_buffer );
	    INIT_STATIC_DESCRIPTOR( pcb->recv_stat_dsc, pcb->recv_stat_buffer );

	    break;
	}

    case SNA_LISTEN:
	/*
	** Extend a listen.
	*/
	{
	    /*
	    ** Execute the Connect request with a connect type
	    ** of passive inorder to open a listen for an IBM
	    ** initiated interrupt.
	    */

	    /* Parse the listen address and stash into sna_addr. */

	    snalu0_lsnaddr( lpcb->port_buffer, &pcb->sna_addr );

	    /* Fill in user data. XXX not needed, heh? */

	    STmove( SNA_MAGIC, NULL, SNA_L_MAGIC, pcb->user_data_buffer );

	    /* Point ru_dsc at bind_buffer */

	    INIT_STATIC_DESCRIPTOR( pcb->ru_dsc, pcb->bind_buffer );

	    status = (*sna_rqst_con)(
		    &pcb->session_id,			/* session-id */
		    &pcb->conn_stat_dsc, 		/* status-blk */
		    &snalu0$k_passive,			/* conn-type */
		    &pcb->sna_addr.node_dsc,		/* node-desc */
		    &pcb->sna_addr.access_dsc,		/* acc-name */
		    0,					/* circuit */
		    &pcb->sna_addr.session_number,	/* sess-addr */
		    0,					/* applic-prog */
		    0,					/* logon-mode */
		    0,					/* user-id */
		    0,					/* pass-word */
		    &pcb->user_data_dsc,		/* data */
		    snalu0_notify,			/* notify-rtn */
		    &pcb,				/* notify-parm */
		    &pcb->notify_dsc,			/* notify-status */
		    &pcb->ru_dsc,			/* bind-buf */
		    &pcb->ru_l,				/* bind-len */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Drop here on failure to post listen. */

	    parms->generic_status = GC_LISTEN_FAIL;
	    parms->system_status.error = status;


	    break;
	}

    case SNA_LISTEN1:
	/*
	** Check results of passive connect.
	*/
	{
	    char	*buf;
	    int		i;

	    /* Look for the SNA_MAGIC cookie in the bind_buffer.  */

	    for( buf = pcb->bind_buffer, i = pcb->ru_l; i; i--, buf++ )
		if( !MEcmp( SNA_MAGIC, buf, 4 ) )
	    {
		MEcopy( buf, (u_i2)SNA_L_MAGIC, pcb->user_data_buffer );
		break;
	    }

	    GCTRACE(2)("snalu0: found magic at %d\n", pcb->ru_l - i );

	    /* If no magic cookie, this is some bogus connection. */

	    if( !i )
		parms->generic_status = GC_LISTEN_FAIL;

	    break;
	}

    case SNA_CONNECT:
	/*
	** Prepare for outbound connection.
	*/
	{
	    /* setup the connection name */

	    GCTRACE(1)("snalu0: %x connecting to %s::%s\n", 
			pcb, 
			parms->function_parms.connect.node_id,
			parms->function_parms.connect.port_id );

	    /* 
	    ** Set string descriptors for node/port 
	    */

	    snalu0_conaddr( 
			parms->function_parms.connect.node_id,
			parms->function_parms.connect.port_id,
			&pcb->sna_addr );

	    /* Fill in user_data */

	    STmove( SNA_MAGIC, NULL, SNA_L_MAGIC, pcb->user_data_buffer);

	    break;
	}

    case SNA_RQST_CON:
	/*
	** Issue a connect to the IBM partner.
	*/
	{
	    /* 
	    ** This call is used for both the callback on inbound 
	    ** connections as well as outbound connections.  
	    **
	    ** For inbound connections SNA_LISTEN1 fills user_data_buffer 
	    ** with JEFFXXXX, where XXXX a control block identifier that 
	    ** the PLU can recognise as a callback connection.  
	    **
	    ** For outbound connection SNA_CONNECT fills user_data_buffer 
	    ** with JEFF0000, where 0000 is binary zeros.
	    */

	    /* (Re)zero in case this is a callback connection. */

	    pcb->session_id = 0;

	    /* Point ru_dsc at bind_buffer */

	    INIT_STATIC_DESCRIPTOR( pcb->ru_dsc, pcb->bind_buffer );

	    /* Execute connect */

	    status = (*sna_rqst_con)(
		    &pcb->session_id,			/* session-id */
		    &pcb->conn_stat_dsc,		/* status-blk */
		    &snalu0$k_active,			/* conn-type */
		    &pcb->sna_addr.node_dsc,		/* node-desc */
		    &pcb->sna_addr.access_dsc,		/* acc-name */
		    0,					/* circuit */
		    0,					/* sess-addr */
		    0,					/* applic-prog */
		    0,					/* logon-mode */
		    0,					/* user-id */
		    0,					/* pass-word */
		    &pcb->user_data_dsc,		/* data */
		    snalu0_notify,			/* notify-rtn */
		    &pcb,				/* notify-parm */
		    &pcb->notify_dsc,			/* motify-status */
		    &pcb->ru_dsc,			/* bind-buf */
		    &pcb->ru_l,				/* bind-len */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Here on error. */

	    parms->generic_status = GC_CONNECT_FAIL;
	    parms->system_status.error = status;


	    break;
	}

    case SNA_RQST_DIS:
	/*
	** Unbind the old connection.
	*/
	{
	    /* If no session, don't try disconnecting. */

	    if( !pcb->session_id )
		break;

	    /* send the unbind */

	    status = (*sna_rqst_dis)(
		    &pcb->session_id,			/* session-id */
		    &pcb->conn_stat_dsc,		/* status-blk */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */	
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Here on error. */

	    parms->generic_status = GC_LISTEN_FAIL;
	    parms->system_status.error = status;

	    break;
	}

    case SNA_XMIT_NEG:
	/*
	** Send a negative response.
	*/
	{
	    /* ru_dsc points to last received ru. */

	    status = (*sna_xmit_rsp)(
		    &pcb->session_id,			/* session-id */
		    &pcb->conn_stat_dsc,		/* status-blk */
		    &pcb->ru_dsc,			/* buff */
		    &pcb->ru_l,				/* buff-size */
		    &snalu0$k_negative_rsp,		/* resp-type */
		    &pcb->sna_state.sense,		/* sense */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Here on error. */

	    parms->generic_status = GC_SEND_FAIL;
	    parms->system_status.error = status;

	    break;
	}

    case SNA_XMIT_POS:
	/*
	** Send a positive response.
	*/
	{
	    /* ru_dsc points to last received ru. */

	    status = (*sna_xmit_rsp)(
		    &pcb->session_id,			/* session-id */
		    &pcb->conn_stat_dsc,		/* status-blk */
		    &pcb->ru_dsc,			/* buff */
		    &pcb->ru_l,				/* buff-size */
		    &snalu0$k_positive_rsp,		/* resp-type */
		    &pcb->sna_state.sense,		/* sense */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Here on error. */

	    parms->generic_status = GC_SEND_FAIL;
	    parms->system_status.error = status;

	    break;
	}

    case SNA_RCV_SDT:
	/*
	** Receive the SDT, start data traffic message 
	*/
	{
	    /* Point ru_dsc at sdt_buffer */

	    INIT_STATIC_DESCRIPTOR( pcb->ru_dsc, pcb->sdt_buffer );

	    status = (*sna_rcv_msg)(
		    &pcb->session_id,			/* session-id */
		    &pcb->conn_stat_dsc,		/* status-blk */
		    &pcb->ru_dsc,			/* buff */
		    &pcb->ru_l,				/* buff-size */
		    &pcb->sna_state.req_ind,		/* req-ind */
		    &pcb->sna_state.more_data,		/* more-data */
		    0,					/* msg-class */
		    0,					/* msg-type */
		    0,					/* flow */
		    0,					/* alt-code */
		    0,					/* beg-brack */
		    0,					/* end-brack */
		    &pcb->sna_state.sense,		/* sense */
		    0,					/* resp-type */
		    0,					/* end-data */
		    &pcb->sna_state.f_seqno,		/* seq-num-first */
		    &pcb->sna_state.l_seqno,		/* seq-num-last */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Here on error. */

	    parms->generic_status = GC_LISTEN_FAIL;
	    parms->system_status.error = status;

	    break;
	}

    case SNA_SEND:
	/*
	** Send NPDU.
	*/
	{
	    /* Set the data buffer descriptor */

	    GCTRACE(2)("snalu0: %x send size %d\n", pcb, parms->buffer_lng );

	    pcb->io_count++;

	    pcb->su_l = parms->buffer_lng + SNABUF$K_HDLEN;
	    pcb->su_dsc.dsc$w_length = pcb->su_l;
	    pcb->su_dsc.dsc$b_class = DSC$K_CLASS_S;
	    pcb->su_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
	    pcb->su_dsc.dsc$a_pointer = parms->buffer_ptr - SNABUF$K_HDLEN;

	    /* Set the message class and response type */

	    status = (*sna_xmit_msg)(
		    &pcb->session_id,			/* session-id */
		    &pcb->send_stat_dsc,		/* status-blk */
		    &pcb->su_dsc,			/* buff */
		    &pcb->su_l,				/* buff-size */
		    &snalu0$k_mclass_unformatted_fm,	/* msg-class */
		    &TRUE,				/* alt-code */
		    0,					/* end-brack */
		    &snalu0$k_rsp_rqe3,			/* resp-type */
		    0,					/* more-data */
		    &TRUE,				/* turn-retain */
		    0,					/* seq-num-first */
		    0,					/* seq-num-last */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-prm */

	    if( VMS_OK( status ) )
		return;

	    /* Drop here on failure to post send. */

	    parms->generic_status = GC_SEND_FAIL;
	    parms->system_status.error = status;

	    break;
	}

    case SNA_SEND1:
	/*
	** Pick up send results.
	*/
	{
	    /* Set send size. */

	    parms->buffer_lng = pcb->su_l - SNABUF$K_HDLEN;

	    break;
	}

    case SNA_RECEIVE:
	/*
	** Receive an NPU.
	*/
	{
	    /* 
	    ** Expedited data will always be received by the normal
	    ** flow.  The flow flag tells us if it was sent expedited
	    ** or normal but we should never get expedited.
	    */

	    pcb->io_count++;

	    /* Set the data buffer descriptor */

	    INIT_STATIC_DESCRIPTOR( pcb->ru_dsc, pcb->bind_buffer );

	    status = (*sna_rcv_msg)(
		    &pcb->session_id,			/* session-id */
		    &pcb->recv_stat_dsc,		/* status-blk */
		    &pcb->ru_dsc,			/* buff */
		    &pcb->ru_l,				/* buff-size */
		    &pcb->sna_state.req_ind,		/* req-ind */
		    &pcb->sna_state.more_data,		/* more-data */
		    0,					/* msg-class */
		    0,					/* msg-type */
		    0,					/* flow */
		    0,					/* alt-code */
		    0,					/* beg-brack */
		    0,					/* end-brack */
		    &pcb->sna_state.sense,		/* sense */
		    0,					/* resp-type */
		    0,					/* end-data */
		    &pcb->sna_state.f_seqno,		/* seq-num-first */
		    &pcb->sna_state.l_seqno,		/* seq-num-last */
		    &lpcb->sna_efn,			/* event-flag */
		    snalu0_sm,				/* ast-addr */
		    &parms );				/* ast-par */

	    if( VMS_OK( status ) )
		return;

	    /* Drop here on failure to post receive. */

	    parms->generic_status = GC_RECEIVE_FAIL;
	    parms->system_status.error = status;

	    break;
	}

    case SNA_RECEIVE1:
	/*
	** Read completion.
	*/
	{
	    /* Check for abnormal read. */

	    if( parms->buffer_lng < pcb->ru_l )
	    {
		parms->generic_status = GC_RECEIVE_FAIL;
		parms->system_status.error = SNA$_BUFTOOSHO;
		break;
	    }

	    /* Because it is necessary to add a buffer (memory) copy to the
	    ** SNALU0 protocol driver, the VAXC builtin _MOVC3 is used
	    ** to avoid the overhead of calling functions to copy the data.
	    */

	    /* get size of the read */

	    parms->buffer_lng = pcb->ru_l - SNABUF$K_HDLEN;

	    SNA_BUFFER_COPY( pcb->ru_l, pcb->ru_dsc.dsc$a_pointer,
			     parms->buffer_ptr - SNABUF$K_HDLEN );

	    GCTRACE(2)("snalu0: %x read size %d\n", pcb, parms->buffer_lng );

	    break;
	}

    case SNA_DISCONNECT:
	/*
	** Prepare for disconnect.
	*/
	{
	    if( !pcb )
	    {
		(*parms->compl_exit)(parms->compl_id);
		return;
	    }

	    pcb->disconn = parms;
	    pcb->io_count++;
	    break;
	}

    case SNA_RWCOMPLETE:
	/*
	** Drive completion routine.
	*/

	(*parms->compl_exit)(parms->compl_id);

	/* fall through */

    case SNA_DISCOMPLETE:
	/*
	** If GCC_DISCONNECT called and no I/O's outstanding,
	** Free memory & complete.
	*/

	if( --pcb->io_count || !( parms = pcb->disconn ) )
	    return;

	GCTRACE(1)("snalu0: %x freeing PCB\n", pcb );

	MEfree( (PTR)pcb );

    case SNA_COMPLETE:
	/*
	** Drive completion routine.
	*/

	(*parms->compl_exit)(parms->compl_id);
	return;
    }

    /* For tracing */

    frombottom++;

    goto top;
}

/*									    
** Name: snalu0_notify() - handle SNA asyncronous notify
**
** Description:
** 	The SNA API calls this routine when some important async event
**	has occurred - the only one we are interested in is the link
**	termination (SNAEVT$K_TERM).
**
** Inputs:
**	event_flag	pointer to async event indicator
**	ppcb		pointer to connection's PCB
**
** Side effects:
**	Sets pcb->evt_term on link termination.
*/

static VOID
snalu0_notify( event_flag, ppcb )
i4		*event_flag;
SNA_PCB		**ppcb;
{
    SNA_PCB	*pcb = *ppcb;
    
    GCTRACE(1)( "snalu0: %x notify %x\n", pcb, *event_flag );

    switch( *event_flag )
    {
	case SNAEVT$K_COMERR:		/* network communication error */
	case SNAEVT$K_RCVEXP:		/* data received on expedited flow  */
	case SNAEVT$K_PLURESET:		/* IBM sent CLEAR */
	case SNAEVT$K_UNBHLD:		/* IBM sent UNBIND type 2 */
	default:
	    break;

	case SNAEVT$K_TERM:		/* link termination */
	    pcb->evt_term++;
	    break;
    }
}

/*
** Name: snalu0_lsnaddr() - parse an sna listen address 
**
** Description:
**     	An SNA listen address is respresented as a string:
**
**	    II_GCCxx_SNA_LU0 = node.port.session (e.g. snagw1.gcvacb5.10)
**
**	This routine splits this address into the node, access, and
**	session_number components of the SNA_ADDR structure for the 
**	passive connection request.
**
** Inputs:
**	buf		value of II_GCCx_SNA_LU0
**
** Outputs:
**	sna_addr	pointers into buf
*/

static VOID
snalu0_lsnaddr( buf, sna_addr )
char		*buf;
SNA_ADDR	*sna_addr;
{
	char	*buf0;

	/* Set string descriptor constants */

	sna_addr->node_dsc.dsc$b_class = DSC$K_CLASS_S;
	sna_addr->node_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
	sna_addr->access_dsc.dsc$b_class = DSC$K_CLASS_S;
	sna_addr->access_dsc.dsc$b_dtype = DSC$K_DTYPE_T;

	/* node.access.session_number */

	for( buf0 = buf; *buf && *buf != '.'; buf++ )
	    ;

	sna_addr->node_dsc.dsc$a_pointer = buf0;
	sna_addr->node_dsc.dsc$w_length = buf - buf0;

	if( *buf == '.' )
	    buf++;

	/* access.session_number */

	for( buf0 = buf; *buf && *buf != '.'; buf++ )
	    ;

	sna_addr->access_dsc.dsc$a_pointer = buf0;
	sna_addr->access_dsc.dsc$w_length = buf - buf0;

	if( *buf == '.' )
	    buf++;

	/* session_number */

	CVan( buf, &sna_addr->session_number );
}

/*
** Name: snalu0_conaddr() - parse an sna connect address 
**
** Description:
**     	An SNA connect address is respresented by two strings:
**
**	    node: node.circuit
**	    port: access
**
**	This routine fills in the SNA_ADDR structure for the active 
**	connectionr request.
**
**	N.B.  circuit is tossed.
**
** Inputs:
**	node		node entry from netu
**	port		port entry from netu
**
** Outputs:
**	sna_addr	pointers into node, port
*/

static VOID
snalu0_conaddr( node, port, sna_addr )
char		*node;
char		*port;
SNA_ADDR	*sna_addr;
{
	char *buf;

	/* Node may be "gateway.circuit" - ignore circuit */
	/* We always have, and therefore always must. */

	for( buf = node; *buf && *buf != '.'; buf++ )
	    ;

	/* 
	** Set string descriptors for node/port 
	*/

	sna_addr->node_dsc.dsc$b_class = DSC$K_CLASS_S;
	sna_addr->node_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
	sna_addr->node_dsc.dsc$a_pointer = node;
	sna_addr->node_dsc.dsc$w_length = buf - node;

	sna_addr->access_dsc.dsc$b_class = DSC$K_CLASS_S;
	sna_addr->access_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
	sna_addr->access_dsc.dsc$a_pointer = port;
	sna_addr->access_dsc.dsc$w_length = STlength( port );
}
