/*
** Copyright (c) 1987, 2002 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<er.h>
#include	<me.h>
#include	<st.h>
#include	<nm.h>
#include	<tr.h>
#include	<pm.h>
#include	<gcatrace.h>
#include	<gcccl.h>
#include	<bsi.h>


/*{
** Name: GCqiobs, GCqiobk - stream/block protocol driver for VMS
**
** Description:
**	This a generic Comm Server protocol driver for VMS.  It
**	calls various BS protocol drivers defined through the BSI.H 
**	interface.
**
** Inputs:
**	 Function_code                   Specifies the requested function
**	     GCC_OPEN         ||
**	     GCC_CONNECT      ||
**	     GCC_DISCONNECT   ||
**	     GCC_SEND         ||
**	     GCC_RECEIVE      ||
**	     GCC_LISTEN
**
**	 parm_list                       A GCC_P_PLIST pointer containing
**					 all other inputs/outputs.
** Returns:
**	 STATUS
**	     OK                          operation successfully posted
**	     any CL failure code
**
** History:
**	6-Apr-91 (seiwald)
**	    Rewritten from UNIX gccbs.c.
**	11-Sep-91 (seiwald)
**	    Allow for GCC_DISCONNECT while GCC_SEND and/or GCC_RECEIVE
**	    still outstanding: use a separate BSP for disconn and defer
**	    disconn completion until last oustanding I/O completes.
**	15-Oct-91 (Hasty)
**	    Changed disconnect sequence from synchronous to asynchronous
**	    Added support to separate the decoding of a listen's address
**	    from a connection's request target host address.
**	    Added in GCbslport support for decoding  decnet's listen address
**	22-Jan-92 (seiwald)
**	    Removed GCbslport's special case for DECNET.  DECNET's
**	    (dcb->portmap)() handles that.
**	22-Jan-92 (seiwald)
**	    Removed lsn_flag to (*dcb->portmap)().  The port mapping is
**	    the same for local and remote addresses.
**	1-May-92 (seiwald)
**	    Some BS drivers leave bsp->buf pointing to dcb->portbuf after
**	    the listen function: make sure we don't stomp on the portbuf
**	    when formatting the final listen address.
**	31-Aug-92 (alan)
**	    Dynamically allocate overflow read buffer, rather than use a
**	    fixed 2048 byte buffer.  The IIGCB protocol bridge doesn't 
**	    follow normal protocol size negotiation rules, which sometimes
**	    resulted in overflow buffers larger than 2K.
**	08-Mar-93 (edg)
**	    Use PMget() in GCbslport to get listen addr.
**      30-Jun-93 (seiwald)
**	    Added support for "NO_PORT" to register function.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	10-oct-97 (kinte01)
**          Added support for Protocol Bridge. Bridge does not
**          require "II" value for listen port.
**      05-nov-99 (loera01) Bug 99059
**          In GCqiosm(), initialize node_id to NULL.  Failure to do so 
**          cause an ACCVIO when the GCC examines it and subsequently tr
**          to copy.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      06-dec-2002 (loera01) SIR 109246
**          In GCqiosm, initialize is_remote to FALSE.
**      12-dec-2002 (loera01) SIR 109237
**          Set the options field in GCC_P_PLIST to according to the results
**          of the protocol driver's accept operation (bsp->is_remote).
*/
/*
** External functions
*/
FUNC_EXTERN	STATUS	MEfree();
FUNC_EXTERN	int	GCX1_trace_alloc();

/*
** Forward functions
*/
static	VOID	GCqiosm();
static	VOID	GCbslport();

/* Per driver control block */

typedef struct _GC_DCB
{
	i4		subport;	/* for listen attempts */
	STATUS		last_status;
	char		lsn_port[64];	/* untranslated listen port */
	char		lsn_bcb[ BS_MAX_LBCBSIZE ];
    	char		portbuf[ GCC_L_NODE + GCC_L_PORT + 5 ];
	BS_PARMS	bsp;		/* for accepts */
} GC_DCB;

/* Per connection control block */

typedef struct _GC_PCB
{
	BS_PARMS	rbsp;		/* for receives */
	BS_PARMS	sbsp;		/* for sends */
	BS_PARMS	cbsp;		/* for connect, disconnect */
	char		*b1;		/* start of read buffer */
	char		*b2;		/* end of read buffer */
	i4		iocount;	/* read/write oustanding */
	GCC_P_PLIST	*disconn;	/* parms for pending disconn */
	char		bcb[ BS_MAX_BCBSIZE ];
	char		*rbuf;		/* start of overflow read buffer */
} GC_PCB;

i4 GCbs_trace = 0;

# define GCTRACE(n) if( GCbs_trace >= n )(void)TRdisplay

/* Actions in GCqiosm. */

typedef enum {
	GC_REG,		/* open listen port */
	GC_LSN,		/* wait for inbound connection */
	GC_LSNBK,	/* wait for inbound connection (on block conn) */
	GC_LSNCMP,	/* LISTEN completes */
	GC_CONN,	/* make outbound connection */
	GC_CONNBK,	/* make outbound connection (on block conn) */
	GC_CONNCMP,	/* CONNECT completes */
	GC_SND,		/* send TPDU */
	GC_SNDBK,	/* send TPDU (on block conn) */
	GC_SNDCMP,	/* SEND completes */
	GC_RCV,		/* receive TPDU */
	GC_RCV1,	/* received NDU header */
	GC_RCV2,	/* received NDU header */
	GC_RCV3,	/* RECEIVE completes */
	GC_RCV4,	/* RECEIVE completes */
	GC_RCVBK,	/* receive TPDU over (on block conn) */
	GC_RCV1BK,	/* RECEIVE completes */
	GC_DISCONN,	/* close a connection */
	GC_DISCONN1	/* async close a connection */
} GC_ACTIONS;

/* Action names for tracing. */

char *gc_names[] = { 
	"REG", "LSN", "LSNBK", "LSNCMP", "CONN", "CONNBK", "CONNCMP",
	"SND", "SNDBK", "SNDCMP", "RCV", "RCV1", "RCV2", "RCV3",
	"RCV4", "RCVBK"," RCV1BK", "DISCONN", "DISCONN1"
} ;

STATUS
GCqiobs( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{
    /*
    ** Compute initial state based on function code.
    */

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = GC_REG; break;
    case GCC_LISTEN: 	parms->state = GC_LSN; break;
    case GCC_CONNECT: 	parms->state = GC_CONN; break;
    case GCC_SEND: 	parms->state = GC_SND; break;
    case GCC_RECEIVE: 	parms->state = GC_RCV; break;
    case GCC_DISCONNECT:parms->state = GC_DISCONN; break;
    }

    GCX_TRACER_1_MACRO("GCbs>", parms,
		"function code = %d", func_code);

    GCTRACE(1)("GC%s: %x operation %s\n", 
	parms->pce->pce_pid, parms, gc_names[ parms->state ] );

    parms->generic_status = OK;
    parms->system_status.error = 0;

    /* Start sm. */

    GCqiosm( parms );
    return OK;
}

STATUS
GCqiobk( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{
    /*
    ** Compute initial state based on function code.
    */

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = GC_REG; break;
    case GCC_LISTEN: 	parms->state = GC_LSNBK; break;
    case GCC_CONNECT: 	parms->state = GC_CONNBK; break;
    case GCC_SEND: 	parms->state = GC_SNDBK; break;
    case GCC_RECEIVE: 	parms->state = GC_RCVBK; break;
    case GCC_DISCONNECT:parms->state = GC_DISCONN; break;
    }

    GCX_TRACER_1_MACRO("GCbk>", parms,
		"function code = %d", func_code);

    GCTRACE(1)("GC%s: %x operation %s\n", 
	parms->pce->pce_pid, parms, gc_names[ parms->state ] );

    parms->generic_status = OK;
    parms->system_status.error = 0;

    /* Start sm. */

    GCqiosm( parms );
    return OK;
}

static VOID
GCqiosm( parms )
GCC_P_PLIST	*parms;
{
    char			*portname, *hostname;
    GC_PCB			*pcb = (GC_PCB *)parms->pcb;
    GC_DCB			*dcb = (GC_DCB *)parms->pce->pce_dcb;
    BS_DRIVER			*bsd = (BS_DRIVER *)parms->pce->pce_driver;
    BS_PARMS			sbsp;
    BS_PARMS			*bsp;
    i4				len;

    /* Allocate per driver control block */

    if( !dcb )
    {
	dcb = (GC_DCB *)MEreqmem( 0, sizeof( *dcb ), TRUE, (STATUS *)0 );
	parms->pce->pce_dcb = (PTR)dcb;
    }

    if( !dcb )
    {
	parms->generic_status = GC_OPEN_FAIL;
	goto complete;
    }

    /* main loop of execution */

top:
    GCTRACE(2)("GC%s: %x state %s\n", 
	parms->pce->pce_pid, parms, gc_names[ parms->state ] );

    switch( parms->state )
    {
    case GC_REG:
	/*
	** Register before the first listen.
	*/

    	/* See whether this is a "no listen" protocol */

    	if(parms->pce->pce_flags & PCT_NO_PORT)
    	    goto complete;

	/* 
	** On first pass pick up the symbolic listen port name for
	** this protocol.  Use the parameter as a last resort.
	*/

        if( !dcb->lsn_port[0] )
        {
            /* By default, CommServer has symbolic value "II" (pce->pce_port)
            ** for listen_port. For Protocol Bridge, function_parms.open.port_id
            ** is the listen port by default.
            */
            if( ! STcompare( parms->function_parms.open.port_id, "II" ) )
            {
                /* Comm Server */
                GCbslport( parms->pce->pce_pid, dcb->lsn_port );
                if( !dcb->lsn_port[0] )
                {
                    STcopy( parms->function_parms.open.port_id, dcb->lsn_port );
                }
            }
            else
            {
                /*
                ** Call to GCbslport() is not required for bridge.
                */
                STcopy(parms->function_parms.open.port_id, dcb->lsn_port);
            }
        }

	/*
	** Translate listen port name into tcp port number. 
	** Fail with status from last listen if we have exhausted
	** usable listen subports.
	*/

	if( (*bsd->portmap)( dcb->lsn_port, dcb->subport, dcb->portbuf ) != OK )
	{
	    parms->generic_status = dcb->last_status;
	    goto complete;
	}

	GCTRACE(1)("GC%s: %x listening at %s\n", 
	    parms->pce->pce_pid, parms, dcb->portbuf );

	/* Extend listen with BS listen. */

	bsp = &dcb->bsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->status = OK;

	bsp->buf = dcb->portbuf;

	(*bsd->listen)( bsp );

	if( bsp->status != OK )
	{
	    dcb->subport++;
	    dcb->last_status = GC_OPEN_FAIL;
	    break;
	}

	/* Note protocol is open and print message to that effect. */

	if( dcb->subport )
	    STprintf( dcb->lsn_port + STlength( dcb->lsn_port ), 
		    "%d", dcb->subport );

	STprintf( dcb->lsn_port + STlength( dcb->lsn_port ), "(%s)", bsp->buf );

	GCTRACE(1)("GC%s: Listen port [%s]\n",
		parms->pce->pce_pid, dcb->lsn_port );

	parms->function_parms.open.lsn_port = dcb->lsn_port;

	goto complete;

    case GC_LSN:
    case GC_LSNBK:
	/*
	** Extend a listen to accept a connection.  
	*/

	/* Allocate pcb */

	len = sizeof( *pcb );
	pcb = (GC_PCB *)MEreqmem( 0, len, TRUE, (STATUS *)0 );
	parms->pcb = (PTR)pcb;

	if( !pcb )
	{
	    parms->generic_status = GC_LISTEN_FAIL;
	    goto complete;
	}

	/* Pick up connection with BS accept. */

	bsp = &pcb->cbsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->status = OK;

	parms->state = GC_LSNCMP;
        bsp->is_remote = FALSE;
	(*bsd->accept)( bsp );
	return;

    case GC_LSNCMP:
	/*
	** Listen ready.  
	*/
	/*
	**  Initialize node_id to NULL.  At present in VMS, that's all we do.
	**  Other platforms use the field.
	*/
	parms->function_parms.listen.node_id=NULL;

	bsp = &pcb->cbsp;

	if( bsp->status != OK )
	{
	    parms->generic_status = GC_LISTEN_FAIL;
	    goto complete;
	}

        GCTRACE(1)("GC%s: Connection is %s\n",
               parms->pce->pce_pid, bsp->is_remote ? "remote" : "local" );
        if (!bsp->is_remote)
            parms->options |= GCC_LOCAL_CONNECT;
        else
            parms->options = 0;            /* Remote peer */

	/* Set up read buffer pointers. */

	pcb->b1 = pcb->b2 = NULL;

	goto complete;

    case GC_CONN:
    case GC_CONNBK:
	/*
	** Make outgoing connection with ii_BSconnect 
	*/

	/* Allocate pcb */

	len = sizeof( *pcb ); 
	pcb = (GC_PCB *)MEreqmem( 0, len, TRUE, (STATUS *)0 );
	parms->pcb = (PTR)pcb;

	if( !pcb )
	{
	    parms->generic_status = GC_CONNECT_FAIL;
	    goto complete;
	}

	/* get the hostname/portname */

	hostname = parms->function_parms.connect.node_id; 
	portname = parms->function_parms.connect.port_id; 

	/* setup the connection name */

	dcb->portbuf[0] = '\0';
	if( hostname && *hostname )
	    STprintf( dcb->portbuf, "%s::", hostname );

	if( portname && *portname )
	    (void)(*bsd->portmap)( portname, 0,
		    dcb->portbuf + STlength( dcb->portbuf ) );

	GCTRACE(1)("GC%s: %x connecting to %s\n",
	    parms->pce->pce_pid, parms, dcb->portbuf );

	/* Make outgoing request call with BS request. */

	bsp = &pcb->cbsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->status = OK;

	bsp->buf = dcb->portbuf;

	parms->state = GC_CONNCMP;
	(*bsd->request)( bsp );
	return;

    case GC_CONNCMP:
	/*
	** Connect completes.  
	*/

	bsp = &pcb->cbsp;

	if( bsp->status != OK )
	{
	    parms->generic_status = GC_CONNECT_FAIL;
	    goto complete;
	}

	/* Set up read buffer pointers. */

	pcb->b1 = pcb->b2 = NULL;

	goto complete;

    case GC_SND:
	/*
	** Send NPDU with ii_BSwrite
	*/
	pcb->iocount++;

	bsp = &pcb->sbsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->status = OK;

	/* add NDU header (TPC specific length) */

	bsp->buf = parms->buffer_ptr - 2;
	bsp->len = parms->buffer_lng + 2;

	bsp->buf[0] = (u_char)(bsp->len % 256);
	bsp->buf[1] = (u_char)(bsp->len / 256);

	GCTRACE(2)("GC%s: %x send size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	/* Send data with BS send. */

	parms->state = GC_SNDCMP;
	(*bsd->send)( bsp );
	return;

    case GC_SNDBK:
	/*
	** Send NPDU with ii_BSwrite
	*/
	pcb->iocount++;

	bsp = &pcb->sbsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->status = OK;

	bsp->buf = parms->buffer_ptr;
	bsp->len = parms->buffer_lng;

	GCTRACE(2)("GC%s: %x send size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	/* Send data with BS send. */

	parms->state = GC_SNDCMP;
	(*bsd->send)( bsp );
	return;

    case GC_SNDCMP:
	/*
	** Send ready.
	*/

	bsp = &pcb->sbsp;

	if( bsp->status != OK )
	{
	    parms->generic_status = GC_SEND_FAIL;
	    goto iodone;
	}

	/* More to send? */

	if( !bsp->len )
	    goto iodone;
	    
	/* Send data with BS send. */
	(*bsd->send)( bsp );
	return;

    case GC_RCV:
	/*
	** Setup bounds of read area.
	*/
	pcb->iocount++;

	bsp = &pcb->rbsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->status = OK;

	bsp->buf = parms->buffer_ptr - 2;
	bsp->len = parms->buffer_lng + 2;

	/* Get what we can from the buffer */

	if( len = ( pcb->b2 - pcb->b1 ) )
	{
	    /* If we have the NDU header, use the length */

	    if( len >= 2 )
	    {
		i4 len2 = ((u_char *)pcb->b1)[0] +
			   ((u_char *)pcb->b1)[1] * 256;
		len = min( len, len2 );
	    }

	    GCTRACE(3)("GC%s: %x using %d\n",
		parms->pce->pce_pid, parms, len );

	    MEcopy( pcb->b1, len, bsp->buf );
	    bsp->buf += len;
	    bsp->len -= len;
	    pcb->b1 += len;

	    /* If overflow buffer empty, then free it */
	    if ( pcb->b1 == pcb->b2 )
		MEfree( pcb->rbuf );
	}

	parms->state = GC_RCV2;
	break;

    case GC_RCV2:
	/*
	** Data read.
	*/

	bsp = &pcb->rbsp;

	if( bsp->status != OK )
	{
	    parms->generic_status = GC_RECEIVE_FAIL;
	    goto iodone;
	}

	/* Reinvoke for short read */

	if( bsp->buf < parms->buffer_ptr )
	{
	    /* Read data with BS receive. */

	    parms->state = GC_RCV2;
	    (*bsd->receive)( bsp );
	    return;
	}

	/* strip NDU header */

	len = ((u_char *)parms->buffer_ptr)[-2] +
	      ((u_char *)parms->buffer_ptr)[-1] * 256 - 2;

	if( len < 0 || len > parms->buffer_lng )
	{
	    parms->generic_status = GC_RECEIVE_FAIL;
	    goto iodone;
	}

	parms->buffer_lng = len;

	/* Issue read for TPDU */

	GCTRACE(2)("GC%s: %x read size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	parms->state = GC_RCV4;
	break;

    case GC_RCV4:
	/*
	** Data read.
	*/

	bsp = &pcb->rbsp;

	if( bsp->status != OK )
	{
	    parms->generic_status = GC_RECEIVE_FAIL;
	    goto iodone;
	}

	/* Reinvoke for short read */

	len = bsp->buf - ( parms->buffer_ptr + parms->buffer_lng );

	if( len < 0 )
	{
	    /* Read data with BS receive. */

	    parms->state = GC_RCV4;
	    (*bsd->receive)( bsp );
	    return;
	}

	/* Copy any excess */

	if( len > 0 )
	{
	    GCTRACE(3)("GC%s: %x saving %d\n",
		parms->pce->pce_pid, parms, len );
	    pcb->rbuf = (char *)MEreqmem( 0, len, FALSE, (STATUS *)0 );
	    MEcopy( bsp->buf - len, len, pcb->rbuf );
	    pcb->b1 = pcb->rbuf;
	    pcb->b2 = pcb->rbuf + len;
	}

	goto iodone;

    case GC_RCVBK:
	/*
	** Setup bounds of read area.
	*/
	pcb->iocount++;

	bsp = &pcb->rbsp;
	bsp->func = GCqiosm;
	bsp->closure = (PTR)parms;
	bsp->timeout = -1;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->status = OK;

	bsp->buf = parms->buffer_ptr;
	bsp->len = parms->buffer_lng;

	/* Read data with BS receive. */

	parms->state = GC_RCV1BK;
	(*bsd->receive)( bsp );
	return;

    case GC_RCV1BK:
	/*
	** receive ready.
	*/

	bsp = &pcb->rbsp;

	if( bsp->status != OK )
	{
	    parms->generic_status = GC_RECEIVE_FAIL;
	    goto iodone;
	}

	/* Reinvoke if not end of message */

	if( bsp->len )
	{
	    /* Read data with BS receive. */

	    parms->state = GC_RCV1BK;
	    (*bsd->receive)( bsp );
	    return;
	}

	parms->buffer_lng = bsp->buf - parms->buffer_ptr;

	/* Got TPDU */

	GCTRACE(2)("GC%s: %x read size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	goto iodone;

    case GC_DISCONN:
	/*
	** Disconnect with ii_BSclose
	*/

	if( !pcb )
		goto complete;

	/* Close connection */

	bsp = &pcb->cbsp;
	bsp->syserr = &parms->system_status;
	bsp->lbcb = dcb->lsn_bcb;
	bsp->bcb = pcb->bcb;
	bsp->closure = (PTR)parms;
	bsp->status = OK;

	parms->state = GC_DISCONN1;
	(*bsd->close)( bsp );
        return;


    case GC_DISCONN1:
	/*
	** Disconn complete.
	*/

	pcb->disconn = parms;
	goto disconn;
    }
    goto top;

iodone:
    /*
    ** Drive completion routine.
    */

    GCTRACE(1)("GC%s: %x complete %s status %x\n",
	parms->pce->pce_pid, parms, gc_names[ parms->state ],
	parms->generic_status );

    (*parms->compl_exit)(parms->compl_id);

    pcb->iocount--;

disconn:
    /*
    ** If GCC_DISCONNECT called and no I/O's outstanding,
    ** Free memory & complete.
    */

    if( !( parms = pcb->disconn ) || pcb->iocount )
	return;

    MEfree( (PTR)pcb );


complete:
    /*
    ** Drive completion routine.
    */

    GCTRACE(1)("GC%s: %x complete %s status %x\n",
	parms->pce->pce_pid, parms, gc_names[ parms->state ],
	parms->generic_status );

    (*parms->compl_exit)(parms->compl_id);
}


/*
** Name: GCbslport - determine name of listen port
**
** Description:
**	Returns the string representation of the listen port, suitable
**	for handing to bsd->listen().  Uses the value of II_GCCxx_proto
**	for the default port name.
**
** Inputs:
**	proto - protocol, e.g. TCP_IP	
**
** Retruns:
**	the resulting port name
**
** History
**	24-May-90 (seiwald)
**	    Dredged from GC_tcp_lport and made protocol generic.
**	22-Jan-92 (seiwald)
**	    Copy result to output buffer rather than returning it.
**	08-Mar-93 (edg)
**	    Use PMget to get listen addr.
*/

static VOID
GCbslport( proto, out )
char	*proto;
char	*out;
{
	char *val = 0;
	char pmsym[ 128 ]; /* oh yeah */

	/* format the PM symbol */
	STprintf( pmsym, "!.%s.port", proto );

	if ( PMget( pmsym, &val ) == OK )
	{
	    STcopy( val, out );
	    return;
	}

	/*  No PM symbol.  Try with installation code. */

	NMgtAt( "II_INSTALLATION", &val );

        /* use a default - the installation code or defport */

        STcopy( val && *val ? val  : "", out );
}
