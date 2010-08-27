/*
**    Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<ck.h>
# include	<me.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<cv.h>
# include	<nm.h>
# include	<gcatrace.h>
# include	<gcccl.h>

# include       <descrip.h>
# include       <ssdef.h>
# include       <dvidef.h>
# include	<efndef.h>
# include       <iledef.h>
# include       <iodef.h>
# include       <iosbdef.h>
# include       <msgdef.h>
/*
# include       <nfbdef.h>
*/
#define NFB$C_DECLNAME 21		/* Declare name */
# include	<starlet.h>
# include       <astjacket.h>

# undef u_char	/* redefined in sys/types.h */

# ifndef	E_BS_MASK
# define	E_BS_MASK 0
# endif

# include	<bsi.h>
# include	<gcdecnet.h>


/*
** Name: gcdecnet.c - BS interface to Decnet via VMS QIO's.
**
** Description:
**	This file provides access to TCP IPC via VMS QIO calls, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are exported as part of the BS interface:
**
**		decnet_accept - accept a single incoming connection
**		decnet_close - close a single connection
**		decnet_listen - establish an endpoint for accepting connections
**		decnet_receive - read data from a stream
**		decnet_request - make an outgoing connection
**		decnet_send - send data down the stream
**		decnet_unlisten - stop accepting incoming connections
**		decnet_ok - dummy function returning OK
**
** History:
**	03-Oct-91 (hasty)
**	    Ported.
**	21-Jan-92 (seiwald)
**	    Tidied up (removed blank lines and unused variables).
**	    Rewrote and documented GC_decnet_addr(), which worked not at all.
**	21-Jan-92 (seiwald)
**	    Removed lsn_flag to GC_decnet_addr().  The port mapping is
**	    the same for local and remote addresses.
**	04-Mar-93 (edg)
**	    Detect no longer in BS_DRIVER.
**      16-jul-93 (ed)
**	    added gl.h
**      19-Jan-1998 (hanal04/horda03) Bug 94918.
**          GCC Listen Address was being interpreted incorrectly.
**	09-feb-1999 (loera01)
**	    For GC_decnet_addr(), did a little cleanup, and made a couple
**	    of small changes to make the algorithm suitable for Unix
**	    and 2.0 and above.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	04-jun-01 (kinte01)
**	    Replace STindex with STchr
**      05-dec-02 (loera01) SIR 109246
**          Set bsp->is_remote to TRUE.
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      22-Dec-2008 (stegr01)
**          Itanium VMS port.
**      13-May-2010 (Ralph Loen) Bug 120552
**          Add output argument to GC_decnet_addr() to return the "actual"
**          symbolic port (base port plus subport).
*/

typedef long VMS_STATUS;

/*
** Name: LBCB, BCB - listen control block, connection control block
*/

typedef struct _LBCB
{	IOSB			iosb;		/* iosb for open function */
    	u_i2			mchan;		/* Mailbox channel */
    	u_i2			dchan;		/* Device channel */
} LBCB;

typedef struct _BCB
{
    	DECNET_MSG		msg_area;
	IOSB			riosb;		/* iosb for receives */
	IOSB			siosb;          /* iosb for sends    */
	IOSB			ciosb;		/* iosb for control  */
    	u_i2			mchan;		/* Mailbox channel */
    	u_i2			dchan;		/* Device channel */
} BCB;

static VOID decnet_accept_0();
static VOID decnet_accept_1();
static VOID decnet_request_0();
static VOID decnet_request_1();
static VOID decnet_send_0();
static VOID decnet_receive_0();
static VOID decnet_close_0();
static VOID decnet_ok();
static i4   attach_decnet();
static i4   mbx_name();

static VOID  
decnet_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB			*lbcb = (LBCB *)bsp->lbcb;
	VMS_STATUS              status;
	struct dsc$descriptor_s net_func_desc;
	struct dsc$descriptor_s net_name_desc;
	DECNET_FUNC             net_func;

	/*
	** Create a global network mailbox.  This is used for all subsequent
	** LISTEN's for inbound connections.  The NFB$C_DECLNAME function is
	** invoked to declare this task as a network task to DECnet.  This
	** also causes all connection requests to be sent this mailbox.  As
	** each request is accepted, a new mailbox and channel  
	** allocated.  The channel of the network mailbox represents a
	** global status variable for this routine.  It is not possible to
	** make it state- or context-free.
	*/

 	status = attach_decnet( &lbcb->dchan, &lbcb->mchan );

 	if (status != SS$_NORMAL) {
	    bsp->status = GC_OPEN_FAIL;
	    bsp->syserr->error = status;
	    return;
	}

	/*
	** Build parameters required to declare network name.  Save the
	** global network mailbox and network channels.  
	*/

	net_func.type = NFB$C_DECLNAME;
	net_func.parm = 0;
	net_func_desc.dsc$a_pointer = &net_func;
	net_func_desc.dsc$w_length = 5;

	net_name_desc.dsc$w_length = STlength( bsp->buf );
	net_name_desc.dsc$a_pointer = bsp->buf;

	status = sys$qiow(EFN$C_ENF,
			  lbcb->dchan,
			  IO$_ACPCONTROL, 
		          &lbcb->iosb,
 			  0,
			  0,
		          &net_func_desc,
		          &net_name_desc,
		          0, 0, 0, 0);

	if( (status != SS$_NORMAL ) ||
	    (status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    sys$dassgn( lbcb->dchan );
	    sys$dassgn( lbcb->mchan );
	    bsp->status = GC_OPEN_FAIL;
	    bsp->syserr->error = status;
	}
}

static VOID  
decnet_accept( bsp )
BS_PARMS	*bsp;
{
	BCB			*bcb = (BCB *)bsp->bcb;
	LBCB			*lbcb = (BCB *)bsp->lbcb;
	VMS_STATUS       	status;

        bsp->is_remote = TRUE;
	/*
	** Place a read on the network mailbox.  This will
	** complete when we have an incoming connection request.
	*/

	status = sys$qio(0,
			 lbcb->mchan,
			 IO$_READVBLK, 
			 &bcb->ciosb,
			 decnet_accept_0,
			 bsp,
			 &bcb->msg_area, 
			 sizeof(bcb->msg_area),
			 0, 0, 0, 0);

	if (status  != SS$_NORMAL)
	{
	    bsp->status = GC_LISTEN_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
        }

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID  
decnet_accept_0( bsp )
BS_PARMS	*bsp;
{
	BCB				*bcb = (BCB *)bsp->bcb;
	LBCB				*lbcb = (BCB *)bsp->lbcb;
	struct dsc$descriptor_s		ncb_desc; /* Network Connect Block */
	VMS_STATUS              	status;

	if( ( status = bcb->ciosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = GC_LISTEN_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
        }

	/*
	** Accept the connection request: create a mailbox and assign a
	** channel to it. 
	*/

 	status = attach_decnet( &bcb->dchan, &bcb->mchan );
 
	if (status != SS$_NORMAL) 
	{
	    bsp->status = GC_LISTEN_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
        }

	ncb_desc.dsc$a_pointer = bcb->msg_area.info;
	ncb_desc.dsc$w_length  = bcb->ciosb.iosb$w_bcnt;

	status = sys$qio(0,
			 bcb->dchan,
			 IO$_ACCESS, 
			 &bcb->ciosb,
			 decnet_accept_1,
			 bsp,
			 0, 
			 &ncb_desc,
			 0, 0, 0, 0);
 	
	if( status != SS$_NORMAL )
	{
	    bsp->status = GC_LISTEN_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID  
decnet_accept_1(bsp)
BS_PARMS	*bsp;
{
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	if( ( status = bcb->ciosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = GC_LISTEN_FAIL;
	    bsp->syserr->error = status;
 	}

	/*  Drive the completion routine  */     

	(*(bsp->func))( bsp->closure );
}

static VOID  
decnet_request(bsp)
BS_PARMS	*bsp;
{
	BCB			*bcb = (BCB *)bsp->bcb;
        struct dsc$descriptor_s	ncb_desc;	/* Network Connect Block */
	VMS_STATUS              status;
	char			porttmp[ 128 ];
	char			*p;
	
 	status = attach_decnet( &bcb->dchan, &bcb->mchan );

 	if (status != SS$_NORMAL)
	{
	    bsp->status = GC_CONNECT_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/* Turn host::port into host::"/task=port" ( back into bcb->buf ) */

	if( ( p = STchr( bsp->buf, ':') ) != NULL && p[1] == ':' )
	{
	    STcopy( p + 2, porttmp );
	    STprintf( p + 2, "\"task=%s\"", porttmp );
	}

	/* Construct Network Connect Block (NCB) */
	
	ncb_desc.dsc$a_pointer = bsp->buf;
	ncb_desc.dsc$w_length = STlength( bsp->buf );

	/* Request logical link connection to remote DECNET listener */

	status = sys$qio(0,
			 bcb->dchan,
			 IO$_ACCESS, 
			 &bcb->ciosb,
			 decnet_request_0,
			 bsp,
			 0,
			 &ncb_desc, 
			 0, 0, 0, 0);

	if (status  != SS$_NORMAL)
	{
	    bsp->status = GC_CONNECT_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}
	return;

complete:
	/*  Drive the completion routine  */     

	(*(bsp->func))( bsp->closure );  


}

static VOID  
decnet_request_0(bsp)
BS_PARMS	*bsp;
{
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	if ((status = bcb->ciosb.iosb$w_status) != SS$_NORMAL)
	{
	    bsp->status = GC_CONNECT_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/* 
	** Issue read for the logical link completion message
	** to the network mailbox.
	*/

	status = sys$qio(0,
			 bcb->mchan,
			 IO$_READVBLK, 
			 &bcb->ciosb,
			 decnet_request_1,
			 bsp,
			 &bcb->msg_area, 
			 sizeof(DECNET_MSG), 
			 0, 0, 0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = GC_CONNECT_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}
	return;

complete:
	/*  Drive the completion routine  */     

	(*(bsp->func))( bsp->closure );  
}

static VOID  
decnet_request_1(bsp)
BS_PARMS	*bsp;
{
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	if( ( status = bcb->ciosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = GC_CONNECT_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/* Check the logical link completion message */

	if( bcb->msg_area.msg_type != MSG$_CONFIRM )
	{
	    bsp->status = GC_CONNECT_FAIL;
	    bsp->syserr->error = status;
	}

complete:
	/*  Drive the completion routine  */     

	(*(bsp->func))( bsp->closure );  
}

/*
** Name: decnet_send - send data down the stream
*/

static VOID
decnet_send( bsp )
BS_PARMS	*bsp;
{
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	status = sys$qio(
			0,
			bcb->dchan,
			IO$_WRITEVBLK,
			&bcb->siosb, 
			decnet_send_0,
			bsp,
			bsp->buf,
			bsp->len,
			0, 0, 0, 0);

 	if( status != SS$_NORMAL )
	{
	    bsp->status = GC_SEND_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}

 	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
decnet_send_0( bsp )
BS_PARMS	*bsp;
{

	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	if( ( status = bcb->siosb.iosb$w_status ) == 0 )
	    status = SS$_NORMAL;

	if( status != SS$_NORMAL )
	{
	    bsp->status = GC_SEND_FAIL;
	    bsp->syserr->error = status;
	}

	bsp->buf += bcb->siosb.iosb$w_bcnt;
	bsp->len -= bcb->siosb.iosb$w_bcnt;

	(*(bsp->func))( bsp->closure );
}


/*
** Name: decnet_receive - read data from a stream
*/

static VOID
decnet_receive( bsp )
BS_PARMS	*bsp;
{
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	/* short receives are OK */

	/* Issue read for DECNET status message.
	** Interrupt data is one possibility.
	*/

	status = sys$qio(
			0,
			bcb->dchan,
			IO$_READVBLK,
			&bcb->riosb,
			decnet_receive_0,
			bsp,
			bsp->buf,
			bsp->len, 
			0, 0, 0, 0 );

	if( status != SS$_NORMAL )
	{
	    bsp->status = GC_RECEIVE_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}
	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
decnet_receive_0( bsp )
BS_PARMS	*bsp;
{                      
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	/* Check status or for 0 length read (EOF) */          
                                 
	status = bcb->riosb.iosb$w_status;

	if( ( status != SS$_NORMAL ) || bcb->riosb.iosb$w_bcnt == 0 )
	{
	    bsp->status = GC_RECEIVE_FAIL;
	    bsp->syserr->error = status;
	    goto complete;
	}

	bsp->buf += bcb->riosb.iosb$w_bcnt; 
	bsp->len = 0;

complete:

	(*(bsp->func))( bsp->closure );
}

static VOID  
decnet_close( bsp )
BS_PARMS	*bsp;
{                      
	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;

	status = sys$cancel( bcb->dchan );

	status = sys$qio(0, 
			 bcb->dchan,
			 IO$_DEACCESS|IO$M_SYNCH, 
			 &bcb->ciosb,
			 decnet_close_0,
			 bsp,
			 0, 0, 0, 0, 0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = GC_DISCONNECT_FAIL;
	    bsp->syserr->error = status;
	    sys$dassgn(bcb->dchan);
	    sys$dassgn(bcb->mchan);
	    goto complete;
	}
	return;

complete:
	(*(bsp->func))( bsp->closure );
}

static VOID  
decnet_close_0(bsp)
BS_PARMS	*bsp;
{                      

	BCB		*bcb = (BCB *)bsp->bcb;
	VMS_STATUS      status;
	VMS_STATUS      status1;

	status = bcb->ciosb.iosb$w_status;

	if( status != SS$_NORMAL) 
	{
	    bsp->status = GC_DISCONNECT_FAIL;
	    bsp->syserr->error = status;
	}

	status1 = sys$dassgn( bcb->dchan );
	status = sys$dassgn( bcb->mchan );

	if( ( status != SS$_NORMAL) || ( status1 != SS$_NORMAL ) )
	{
		bsp->status = GC_DISCONNECT_FAIL;
		bsp->syserr->error = status;
	}

	(*(bsp->func))( bsp->closure );
}

/* ---- Utility routines ---- */

/* Create a mailbox, and assign an associated channel to "_NET:".
** The mailbox is used to receive status & interrupt messages
*/

static i4
attach_decnet(dchan, mchan)
short	*mchan;
short	*dchan;
{
    i4             status;
    char		netmbx_name[20];
    struct dsc$descriptor_s	netmbx_desc;
    struct dsc$descriptor_s     net_desc;

    *mchan = -1;
    *dchan = -1;
    status = sys$crembx(0,mchan,0,0,0xff00,0,0);
    if (status != SS$_NORMAL)
	return (status);

    /* Determine mailbox name */
    status = mbx_name(*mchan, netmbx_name);
    if (status != SS$_NORMAL)
	return (status);

    /* Set up descriptors for ...
    ** network mailbox and network psuedo-device.
    */
    netmbx_desc.dsc$a_pointer = netmbx_name;
    netmbx_desc.dsc$w_length = STlength(netmbx_name);
    net_desc.dsc$a_pointer = "_NET:";
    net_desc.dsc$w_length = 5;

    /* Associate mailbox just created with network psuedo-device */
    status = sys$assign(&net_desc, dchan, 0, &netmbx_desc);
    return (status);
}

/* Determine systems name for mailbox given a channel to it.
** This routine exists in gcacl.c but the desire is to keeep this
** module self-contained (apart from official CL routines).
*/

static i4
mbx_name( mbx_chan, mbx_name)
i4 mbx_chan;
char *mbx_name;		/* Must point to area >= 16 bytes */
{
    u_i2 	mbx_namelen = 0;
    i4	status;
    ILE3		dvi_list[] =
		 {
		    /* We want the name and it's length*/
		    {16, DVI$_DEVNAM, mbx_name, &mbx_namelen},
		    {0, 0, 0, 0}
		 };
    IOSB	iosb;

    /* Ask the O.S. for the mailbox name */

    status = sys$getdviw(EFN$C_ENF, 	    /* event flag */
			mbx_chan,   /* Channel to mailbox whose name we want */
			0, 
			&dvi_list, 
			&iosb, 0, 0, 0 );
    if (status & 1)
	status = iosb.iosb$w_status;
    if ((status & 1) != 0)
    {
	/* If OK then null terminate */                 
	mbx_name[mbx_namelen] = 0;
    }
    return (status);
}

static VOID
decnet_ok( bsp )
BS_PARMS	*bsp;
{
	bsp->status = OK;
}
/*
** Name: GC_decnet_addr - turn a abbreviated decnet port into a real one
**
** Description:
**	Decnet addresses for INGRES/NET, as known to decnet, are of the form:
**
**		II_GCCxx_N
**
**	Users may use any of the following shorthand address formats
**	when refering to decnet addresses:
**
**		N
**		xx
**		xxN
**		II_GCCxx
**		II_GCCxx_N
**
**	This routine maps the above into II_GCCxx_N.  If xx is not given
**	as part of the address, it is ommitted in the output address.
**	When N is not given as part of the address, the subport parameter
**	will be used.
**
** Inputs:
**	pin	input address
**	subport	value to use for N
**
** Outputs:
**	pout	output address
**
** Returns:
**	FAIL	if N given and subport != 0 or subport > 9
**	OK	otherwise
**
** History:
**	21-Sep-92 (seiwald) bug #45772
**	    Fixed listen logical in production installations.
**      19-Jan-1999 (hanal04/horda03) Bug 94918
**          The character pointer p was not advanced sufficiently
**          when the pin parameter was of the form II_GCCxxxxx.
**          Thus the function failed to determine the real decnet
**          address. Also, if an installation id is specified,
**          this is limited to 2 characters (the char pointer inst
**          is set to the start of the installation id from the PIN
**          parameter; but this parameter could contain the port
**          number too - e.g II_GCCAB_1).
**          There was also a problem where a listen address of II_GCC
**          was deemed invalid (so II_GCC would be returned). II_GCC
**          is a valid listen address and should have the port id
**          appended (e.g II_GCC ==> II_GCC_x).
**	09-feb-1999 (loera01)
**	    Changed STbcompare of "II_GCC" to account for all six characters
**	    instead of the first five.  Changed iigcc from a i4 to a bool,
**	    and made sure the resulting port was in uppercase.
**      13-May-2010 (Ralph Loen) Bug 120552
**          Add output argument to return the "actual"
**          symbolic port (base port plus subport).
*/
GC_decnet_addr( pin, subport, pout, addrout )
char	*pin;
i4	subport;
char	*pout;
char    *addrout;
{
	char	*inst = 0;
	char	*n = 0;
	bool	iigcc = FALSE;
	char	*p = pin; 
	char	portbuf[ 5 ];

        STcopy(pin, addrout);

	/* Pull pin apart */

	if( !STbcompare( p, STlength("II_GCC"), "II_GCC", 0, TRUE )  )
	{
	    iigcc = TRUE;
	    p += STlength("II_GCC");   /* Bug 94918 */
	}

	if( CMalpha( &p[0] ) && ( CMalpha( &p[1] ) | CMdigit( &p[1] ) ) )
	{
	    inst = p;
	    p += 2;
	}

	if( iigcc && p[0] == '_' )
        {
	    p++;
	}

	if( CMdigit( &p[0] ) )
	{
	    n = p;
	    p++;
	}

	/* Is port an unrecognised form? */

	if( p[0] || !inst && !n && !iigcc)
	{
	    if( subport )
		return FAIL;

	    STcopy( pin, pout );

	    return OK;
	}

	/* Put pout together. */

	if( n && subport || subport > 9 )
	    return FAIL;

	if( !n )
	    CVna( subport, n = portbuf );

	if( !iigcc && !inst )
	    NMgtAt( "II_INSTALLATION", &inst );

	if( !inst )
	    inst = "";

        /* Bug 94918. Only use a max. of 2 characters from INST.
        ** If INST = "" then pout will be of the form II_GCC_x.
        **
        ** If INST = "AB" or "AB_1" then the pout will be
        ** of the form II_GCCAB_x.
        */
	STprintf( pout, "II_GCC%0.2s_%s", inst, n );
        CVupper(pout);
	return OK;
}


/*
** Exported driver specification.
*/

GLOBALDEF BS_DRIVER GC_decnet = {
	sizeof( BCB ),
	sizeof( LBCB ),
	decnet_listen,
	decnet_ok,
	decnet_accept,
	decnet_request,
	decnet_ok,		/* connect not needed */
	decnet_send,
	decnet_receive,
	decnet_close,
	decnet_ok,		/* regfd not needed */
	decnet_ok,		/* save not needed */
	decnet_ok,		/* restore not needed */
	GC_decnet_addr,	
} ;
