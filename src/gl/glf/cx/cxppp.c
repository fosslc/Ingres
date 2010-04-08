/*
**    Copyright (c) 2005, 2008 Ingres Corporation
**
*/


# include	<compat.h>
# include       <cv.h>
# include	<pc.h>
# include	<cs.h>
# include	<er.h>
# include	<lk.h>
# include	<me.h>
# include	<pm.h>
# include	<st.h>
# include	<tr.h>
# include	<cx.h>
# include	<cxprivate.h>

# if defined(VMS)

# include	<gcdecnet.h>
#include <nfbdef.h>

/* Additional VMS header files. */
#include <lib$routines.h>
# include       <starlet.h>
# include       <dvidef.h>
# include       <efndef.h>
# include       <iledef.h>
# include       <iodef.h>
# include       <iosbdef.h>
# include       <msgdef.h>

#define MBX_PROT_MASK	0xff00	/* owner/system: all, group/world: none */
#define MBX_MAX_MSG	sizeof (LK_DSM) * 8

static bool cx_deadlock_debug = FALSE;

/*
** This mega-hack allows use to test the CXppp routines
** in a simple stand-alone program (cxppptest.c).
*/
# if defined(INCLUDED_FROM_CXPPPTEST)
# undef CSsuspend
# define CSsuspend( f, t, e ) (sys$hiber(), 0)
# undef CSresume
# define CSresume( s )	(sys$wake(0, 0), 0)
# endif /* INCLUDED_FROM_CXPPPTEST */


/*
** Name: cxppp.c - Cluster Extension for point -> point pipes.
**
** Description:
**	This file provides a facility providing virtual uni-directional
**	data pipes between members of a cluster.  These have been placed
**	here instead of in GC as they may in some implementations exploit
**	intra-cluster specific interfaces (i.e. VMS ICC, Tru64 IMC)
**
**	The functions fall into three groups:
**
**	Admin functions common to readers & writers:
**
**	   CXppp_alloc - Allocate data structure for the pipe.
**	   CXppp_free  - deallocate data structure for the pipe, closing
**	     pipe if needed.
**	
**	Those used to manage the target for the data pipes.  These
**	routines merge and buffer input from multiple writers and
**	provide functionlity somewhat like the UNIX "select".
**
**	   CXppp_listen - Publish the end point for the pipes.
**	   CXppp_read - Return pointer and size info for next piece
**	    of buffered data, otherwise suspend.
**	   CXppp_close - Close off read end of all pipes.
**
**	Those used to manage a "write" pipe:
**
**	   CXppp_connect - Connect to an existing listener.
**	   CXppp_write - ship data down a connection.
**	   CXppp_disconnect - Close off write end of the pipe.
**
** History:
**	16-may-2005 (devjo01/loera01)
**	    Created by liberally lifting code from gca.   
**	31-may-2005 (devjo01)
**	    Correct broken CXppp_read declaration in "stub" area.
**	02-jan-2007 (abbjo03/loera01/jenjo02)
**	    Improve some of the VMS datatype declarations.  Add trace param
**	    to CXppp_alloc for use by deadlock threads debugging.  
**	21-Feb-2007 (jonj/abbjo03)
**	    Give precedence to stalled connections.  If maxrecstoread is zero,
**	    release consumed data space, unstall any stalled connections,
**	    return without actually reading anything.  Additional tracing and
**	    debugging code.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
*/

typedef		unsigned	OSSTATUS;

# define SET_BIT( b, mask ) ( *(b) |= (mask) )
# define CLR_BIT( b, mask ) ( *(b) &= ~(mask) )

typedef struct _CX_PPP_DECNET_CON
{
    IOSB	 cx_dnppp_iosb;
    II_VMS_CHANNEL  cx_dnppp_dchan;
    II_VMS_CHANNEL  cx_dnppp_mchan;
    i4		 cx_dnppp_status;
    char	*cx_dnppp_buf;		/* Base address of buffer */
    i4		 cx_dnppp_bufsize;	/* Size of buffer in bytes */
    i4		 cx_dnppp_bufhead;	/* Leading edge of valid data. */
    i4		 cx_dnppp_bufnext;	/* Next pos. to read from. */
    i4		 cx_dnppp_buftail;	/* Trailing edge of unprocessed data */
    i4		 cx_dnppp_bytes;	/* Bytes available for read */
    i4		 cx_dnppp_readhead;	/* Stable bufhead for read */
    struct _CX_PPP_DECNET_CTX *cx_dnppp_pctx;	/* Parent context. */
    DECNET_MSG	 cx_dnppp_msg_area;	        /* For control messages */
} CX_PPP_DECNET_CON;

typedef struct _CX_PPP_DECNET_CTX 
{
    char	*cx_dnppp_name;		/* "service" name */
    i4		 cx_dnppp_maxrcon;	/* # of potential readconnects. */
    i4		 cx_dnppp_errsmask;	/* Mask of connections with errors. */
    i4		 cx_dnppp_inusemask;	/* Mask of active connection structs */
    i4		 cx_dnppp_stallmask;	/* Mask of stalled connects */
    i4		 cx_dnppp_unstallmask;	/* Mask of resumeable connects */
    i4		 cx_dnppp_active;	/* if 1, accept is active. */
    i4		 cx_dnppp_pending;      /* # pending operations. */
    i4		 cx_dnppp_pendast;	/* # pending ASTs */
    i4		 cx_dnppp_rcon;		/* Last channel read */
    CS_SID	 cx_dnppp_session;	/* Session where we're running */
    CX_PPP_DECNET_CON	cx_dnppp_cons[1];/* 1st elem of connections array */
}	CX_PPP_DECNET_CTX;

static OSSTATUS attach_decnet(II_VMS_CHANNEL *, II_VMS_CHANNEL *);
static OSSTATUS mbx_name(II_VMS_CHANNEL mchan, char *mboxname);
static void  cx_continue_ast( CS_SID * );
static void  cx_decnet_deassign( CX_PPP_DECNET_CON *pcon );
static void  cx_decnet_close( CX_PPP_DECNET_CON *pcon );
static void  cx_decnet_close_0( CX_PPP_DECNET_CON *pcon );



/*{
** Name: CXppp_alloc - Allocate a PPP context structure.
**
** Description:
**
**	Allocates all memory resources required by a PPP context.
**
** Inputs:
**
**	hctx - address of pointer to return PPP context.
**		
**	name - Required name string for pipe "read" end point,
**	       may be NULL for a write end point.
**
**	readconnections - number of pipes this context will service,
**	       pass a zero (0) for write contexts.
**
**	bufsize - Size of buffer in bytes for a reader.  Is rounded up
**	       to next multiple of align restrict.  Should be zero (0)
**	       for a writer, as writes are not buffered.
**
** Outputs:
**	*hctx - Updated with address of context.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CL2C11_CX_E_BADPARAM  - Bad parameter passed.
**		E_CL2C12_CX_E_INSMEM	- Couldn't alloc struct &/or bufs.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
**	23-Jan-2007 (jonj)
**	    Do not round caller's buf_size, ok to round it locally for
**	    our alignment purposes.
*/
STATUS
CXppp_alloc( CX_PPP_CTX *hctx, char *name, i4 readconnections, i4 buf_size,
	bool trace )
{
    CX_PPP_DECNET_CTX  *ctx;
    CX_PPP_DECNET_CON  *pcon;
    STATUS		status;
    char	       *bufptr;
    i4			con;
    i4			bufsize = buf_size;

    cx_deadlock_debug = trace;

    if ( !name ) name = "";
    bufsize = sizeof(ALIGN_RESTRICT) * 
     ((bufsize + sizeof(ALIGN_RESTRICT) - 1)/sizeof(ALIGN_RESTRICT));
    ctx = (CX_PPP_DECNET_CTX *)MEreqmem(0, sizeof(CX_PPP_DECNET_CTX)
		+ readconnections * (sizeof(CX_PPP_DECNET_CON) + bufsize)
		+ bufsize + STlength(name) + 1, 0, &status);
    if ( NULL == ( *hctx = ctx ) )
    {
	status = E_CL2C12_CX_E_INSMEM;
    }
    else
    {
	ctx->cx_dnppp_name =
	 (char *)(&ctx->cx_dnppp_cons[readconnections + 1]) +
	 (readconnections + 1) * bufsize;
	STcopy( name, ctx->cx_dnppp_name );
	ctx->cx_dnppp_maxrcon = readconnections;
	ctx->cx_dnppp_errsmask = 0;
	ctx->cx_dnppp_stallmask = 0;
	ctx->cx_dnppp_unstallmask = 0;
	ctx->cx_dnppp_inusemask = 0;
	ctx->cx_dnppp_active = 0;
	ctx->cx_dnppp_pending = 0;
	ctx->cx_dnppp_pendast = 0;
	ctx->cx_dnppp_rcon = 0;
	ctx->cx_dnppp_session = (CS_SID)0;

	if ( bufsize )
	    bufptr = (char *)(&ctx->cx_dnppp_cons[readconnections + 1]);
	else
	    bufptr = NULL;

	for ( pcon = ctx->cx_dnppp_cons; 
	      pcon <= &ctx->cx_dnppp_cons[readconnections];
	      pcon++ )
	{
	    pcon->cx_dnppp_dchan = pcon->cx_dnppp_mchan = -1;
	    pcon->cx_dnppp_status = OK;
	    pcon->cx_dnppp_buf = bufptr;
	    pcon->cx_dnppp_bufsize = buf_size; /* caller's unrounded size */
	    pcon->cx_dnppp_bufhead = 0; /* Leading edge of valid data. */
	    pcon->cx_dnppp_bufnext = 0;	/* Next pos. to read from. */
	    pcon->cx_dnppp_buftail = 0; /* Trailing edge of unprocessed data */
	    pcon->cx_dnppp_pctx = ctx;
	    bufptr += bufsize;
	}
	status = OK;
    }
    return status;
}


/*{
** Name: CXppp_free - Deallocate all resources associated with a context.
**
** Description:
**
**	Perform an implicit close/disonnect, and free memory allocated
**	to context structure.
**
** Inputs:
**
**	hctx - address of pointer to return PPP context.
**		
** Outputs:
**	*hctx - Updated to NULL.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		other codes as per close/disconnect.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
*/
STATUS
CXppp_free( CX_PPP_CTX *hctx )
{
    STATUS	status;

    status = CXppp_close( *hctx );
    MEfree( *hctx );
    *hctx = NULL;
    return status;
}



/*{
** Name: CXppp_listen - Establish read terminus for our pipes.
**
** Description:
**
**	Publishes read end for our pipes.  
**
** Inputs:
**
**	ctx - address of previously allocated PPP context.
**		
** Outputs:
**	None.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CL2C49_CX_E_PPP_LISTEN- Could not establish endpoint.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
*/

/* Declarations for routines used by CXppp_listen to implement accept. */
static void cx_decnet_accept( CX_PPP_DECNET_CON * );
static void cx_decnet_accept_0( CX_PPP_DECNET_CON * );
static void cx_decnet_accept_1( CX_PPP_DECNET_CON * );
static void cx_decnet_receive( CX_PPP_DECNET_CON * );
static void cx_decnet_receive_0( CX_PPP_DECNET_CON * );
static void cx_decnet_close_con( CX_PPP_DECNET_CON *pcon );
static void cx_decnet_close_con_0( CX_PPP_DECNET_CON *pcon );

STATUS
CXppp_listen( CX_PPP_CTX ctx )
{
    CX_PPP_DECNET_CTX	*dnctx = ctx;
    STATUS		 status = OK;
    OSSTATUS             osstatus = 0;
    struct dsc$descriptor_s net_func_desc;
    struct dsc$descriptor_s net_name_desc;
    DECNET_FUNC          net_func;

    do
    {
	/* Sanity checks */
	if ( dnctx->cx_dnppp_maxrcon == 0 ||
	     dnctx->cx_dnppp_inusemask != 0 ||
	     dnctx->cx_dnppp_cons[1].cx_dnppp_bufsize == 0 )
	{
	    /* Not a valid ctx or allocated for write only. */
	    status = E_CL2C49_CX_E_PPP_LISTEN;
	    break;
	}

	dnctx->cx_dnppp_inusemask = (1<<0);
	CSget_sid( &dnctx->cx_dnppp_session );
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

	osstatus = attach_decnet( &dnctx->cx_dnppp_cons[0].cx_dnppp_dchan,
				  &dnctx->cx_dnppp_cons[0].cx_dnppp_mchan );

	if (osstatus != SS$_NORMAL)
	{
	    TRdisplay( "%s:%d - osstatus = %d\n", __FILE__, __LINE__,
			osstatus ); /* FIX-ME */
	    TRflush();
	    break;
	}

	/* Publish the channels, for debugging */
        TRdisplay("%@ CXppp_listen: CTX %x CON[0] %x dchan %x mchan %x\n",
		dnctx, 
		&dnctx->cx_dnppp_cons[0],
		dnctx->cx_dnppp_cons[0].cx_dnppp_dchan,
		dnctx->cx_dnppp_cons[0].cx_dnppp_mchan), TRflush();

	/*
	** Build parameters required to declare network name.  Save the
	** global network mailbox and network channels.  
	*/

	net_func.type = NFB$C_DECLNAME;
	net_func.parm = 0;
	net_func_desc.dsc$a_pointer = (char *)&net_func;
	net_func_desc.dsc$w_length = 5;

	net_name_desc.dsc$w_length = STlength(dnctx->cx_dnppp_name);
	net_name_desc.dsc$a_pointer = dnctx->cx_dnppp_name;

	osstatus = sys$qiow(EFN$C_ENF,
			    dnctx->cx_dnppp_cons[0].cx_dnppp_dchan,
			    IO$_ACPCONTROL, 
			    &dnctx->cx_dnppp_cons[0].cx_dnppp_iosb,
			    0,
			    0,
			    &net_func_desc,
			    &net_name_desc,
			    0, 0, 0, 0);

	if( (osstatus != SS$_NORMAL ) ||
	    (osstatus = dnctx->cx_dnppp_cons[0].cx_dnppp_iosb.iosb$w_status )
		      != SS$_NORMAL )
	{
	    sys$dassgn(dnctx->cx_dnppp_cons[0].cx_dnppp_dchan);
	    sys$dassgn(dnctx->cx_dnppp_cons[0].cx_dnppp_mchan);
	    TRdisplay( "%s:%d - osstatus = %d\n", __FILE__, __LINE__,
			osstatus ); /* FIX-ME */
	    TRflush();
	    break;
	}

	/* Start accepting connections */
	dnctx->cx_dnppp_active = 1;
	dnctx->cx_dnppp_pending = 1;  /* Trick to suppress unwanted Resume
					 being issued before 1st read. */
	cx_decnet_accept( dnctx->cx_dnppp_cons );
    } while (0);

    if ( OK == status && osstatus != SS$_NORMAL )
    {
	TRdisplay( "%s:%d - status = %x, osstatus = %d\n",
	__FILE__, __LINE__, status, osstatus ); /* FIX-ME */ TRflush();
	dnctx->cx_dnppp_cons[0].cx_dnppp_iosb.iosb$w_status = osstatus;
	status = E_CL2C49_CX_E_PPP_LISTEN;
    }

    if ( OK != status )
    {
	TRdisplay( "%s:%d - status = %d\n", __FILE__, __LINE__,
		    status ); /* FIX-ME */ TRflush();
	cx_decnet_close_con( dnctx->cx_dnppp_cons );
	dnctx->cx_dnppp_cons[0].cx_dnppp_iosb.iosb$w_status = osstatus;
    }
	
    return status;
}

/*
**	cx_decnet_accept - 1st step in accept a connection process.
**
**	Place a read on the network mailbox for interested parties
**	to announce their intention to connect.
*/
static void
cx_decnet_accept( CX_PPP_DECNET_CON  *pcon )
{
    CX_PPP_DECNET_CTX  *dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS       	osstatus;
    i4			connum;
    i4			mask; 

    do
    {
	/* Find next free con struct */
	for ( connum = dnctx->cx_dnppp_maxrcon; connum > 0 ; connum-- )
	{
	    mask = (1 << connum);
	    if ( !(dnctx->cx_dnppp_inusemask & mask) )
	    {
		pcon = &dnctx->cx_dnppp_cons[connum];
		SET_BIT(&dnctx->cx_dnppp_inusemask, mask);
		break;
	    }
	}
	if ( 0 == connum )
	{
	    /* 
	    ** If there are no free con structs, this AST just returns,
	    ** and dnctx will be unable to accept new connections until
	    ** one is freed.  Note: if when closing a read connection we
	    ** see that cx_dnppp_active is zero, the closing routine
	    ** must call cx_decnet_accept to re-enable accepting connects,
	    ** unless this is part of closing the read context.
	    */
	    dnctx->cx_dnppp_active = 0;
	    break;
	}

	/* Reset error mask for this connection */
	CLR_BIT(&dnctx->cx_dnppp_errsmask, mask );

	/*
	** Place a read on the network mailbox.  This will
	** complete when we have an incoming connection request.
	*/
	if (cx_deadlock_debug && !lib$ast_in_prog())
	    TRdisplay("%@ cx_decnet_accept: QIO read mchan %x\n",
		dnctx->cx_dnppp_cons[0].cx_dnppp_mchan), TRflush();
	osstatus = sys$qio(0,
			 dnctx->cx_dnppp_cons[0].cx_dnppp_mchan,
			 IO$_READVBLK, 
			 &pcon->cx_dnppp_iosb,
			 cx_decnet_accept_0,
			 pcon,
			 &pcon->cx_dnppp_msg_area,
			 sizeof(pcon->cx_dnppp_msg_area),
			 0, 0, 0, 0);

	if (osstatus != SS$_NORMAL)
	{
	    cx_decnet_deassign( pcon );
	    pcon->cx_dnppp_status = E_CL2C49_CX_E_PPP_LISTEN;
	    SET_BIT( &dnctx->cx_dnppp_errsmask, mask );
	    dnctx->cx_dnppp_active = 0;
	}
	else
	{
	    dnctx->cx_dnppp_active = 1;
	}
    } while (0);
}	/* cx_decnet_accept */
	
/*
**	cx_decnet_accept_0 - 2nd step in "accept a connection" process.
**
**	If read posted in step (1) completes successfully, then alloc a
**	mailbox and two channels for the new connection, and establish
**	the new connection.
*/
static void  
cx_decnet_accept_0( CX_PPP_DECNET_CON  *pcon )
{
    CX_PPP_DECNET_CTX  *dnctx = pcon->cx_dnppp_pctx;
    struct dsc$descriptor_s	ncb_desc; /* Network Connect Block */
    OSSTATUS              	osstatus;

    if (cx_deadlock_debug && pcon->cx_dnppp_msg_area.msg_type != MSG$_CONNECT && !lib$ast_in_prog())
	TRdisplay("%@ cx_decnet_accept_0: msg_type %d\n",
	    pcon->cx_dnppp_msg_area.msg_type), TRflush();
    do
    {
	if ( ( osstatus = pcon->cx_dnppp_iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    break;
	}

	/*
	** Accept the connection request: create a mailbox and assign a
	** channel to it. 
	*/

 	osstatus = attach_decnet( &pcon->cx_dnppp_dchan,
	                          &pcon->cx_dnppp_mchan );
	if (osstatus != SS$_NORMAL) 
	{
	    break;
        }

	ncb_desc.dsc$a_pointer = pcon->cx_dnppp_msg_area.info;
	ncb_desc.dsc$w_length  = pcon->cx_dnppp_iosb.iosb$w_bcnt;

	/* Publish the channels, for debugging */
        TRdisplay("%@ cx_decnet_accept_0: CTX %x CON[%d] %x QIO access dchan %x mchan %x\n",
		dnctx,
		pcon - dnctx->cx_dnppp_cons,
		pcon,
		pcon->cx_dnppp_dchan,
		pcon->cx_dnppp_mchan), TRflush();

	osstatus = sys$qio(0,
			 pcon->cx_dnppp_dchan,
			 IO$_ACCESS, 
			 &pcon->cx_dnppp_iosb,
			 cx_decnet_accept_1,
			 pcon,
			 0, 
			 &ncb_desc,
			 0, 0, 0, 0);
    } while (0);

    if ( osstatus != SS$_NORMAL )
    {
	TRdisplay("%@ cx_decnet_accept_0: qio ACCESS fail status %x\n",
	    osstatus); TRflush();
	pcon->cx_dnppp_status = E_CL2C49_CX_E_PPP_LISTEN;
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
	cx_decnet_deassign( pcon );	/* Dealloc any resources. */
	cx_decnet_accept( pcon );	/* Get ready to try again. */
    }
} /* cx_decnet_accept_0 */

/*
** 	cx_decnet_accept_1 - 3nd step in accept a connection process.
**
**	Step (3) of accept process.   If step (2) completes successfully,
**	then we post a receive to start data transfer from the writer
**	initiating the connection.
**
**	Always repost an accept regardless of result.
*/
static void  
cx_decnet_accept_1( CX_PPP_DECNET_CON *pcon )
{
    CX_PPP_DECNET_CTX  *dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS      osstatus;

    if( ( osstatus = pcon->cx_dnppp_iosb.iosb$w_status ) != SS$_NORMAL )
    {
	TRdisplay("%@ cx_decnet_accept_1: qio ACCESS fail status %x\n",
	    osstatus); TRflush();
	pcon->cx_dnppp_status = E_CL2C49_CX_E_PPP_LISTEN;
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
	cx_decnet_deassign( pcon );	/* Dealloc any resources. */
    }

    /* Either way, try again for another connection */
    cx_decnet_accept( pcon );

    if ( osstatus == SS$_NORMAL )
    {
	/* Channel is ready, post the read. */
	cx_decnet_receive( pcon );
    }
}

/*
** Name: cx_decnet_receive - read data from a stream
**
**	Step (1) of receive state.
*/
static void
cx_decnet_receive( CX_PPP_DECNET_CON *pcon )
{
    CX_PPP_DECNET_CTX	*dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS        osstatus;
    i4		    amount;

    /* Calculate maximum amount of data to draw from decnet. */
    if ( pcon->cx_dnppp_bufhead < pcon->cx_dnppp_buftail )
	amount = pcon->cx_dnppp_buftail - pcon->cx_dnppp_bufhead;
    else
	amount = pcon->cx_dnppp_bufsize - pcon->cx_dnppp_bufhead;
    if (amount <= 0)
	TRdisplay("%@ cx_decnet_receive: non-positive data size %d\n", amount), TRflush();
    if (amount >= 65536)
	TRdisplay("%@ cx_decnet_receive: potential non-positive data size %d\n",
	    amount), TRflush();

    /* short receives are OK */
    if (cx_deadlock_debug && !lib$ast_in_prog())
	TRdisplay("%@ cx_decnet_receive: QIO read dchan %x amount %d head"
	    " %d tail %d\n", pcon->cx_dnppp_dchan, amount,
	    pcon->cx_dnppp_bufhead, pcon->cx_dnppp_buftail), TRflush();
    /* Count a pending AST for debugging */
    CSadjust_counter( &dnctx->cx_dnppp_pendast, 1 );
    osstatus = sys$qio(
		    0,
		    pcon->cx_dnppp_dchan,
		    IO$_READVBLK | IO$M_MULTIPLE,
		    &pcon->cx_dnppp_iosb,
		    cx_decnet_receive_0,
		    pcon,
		    pcon->cx_dnppp_buf + pcon->cx_dnppp_bufhead,
		    amount, 
		    0, 0, 0, 0 );

    if ( osstatus != SS$_NORMAL )
    {
	TRdisplay("%@ cx_decnet_receive: QIO read dchan %x osstatus %x\n",
	    pcon->cx_dnppp_dchan, osstatus), TRflush();
	pcon->cx_dnppp_status = E_CL2C4A_CX_E_PPP_RECEIVE;
	pcon->cx_dnppp_pctx->cx_dnppp_errsmask |=
	    (1 << (pcon - pcon->cx_dnppp_pctx->cx_dnppp_cons));
	cx_decnet_close_con( pcon );
    }
}

/*
** Name: cx_decnet_receive_0 - complete read from a stream
**
**	Step (2) of receive states.
**
**	Disconnect from writer if we have a failure, otherwise
**	post the next recieve.
*/
static void
cx_decnet_receive_0( CX_PPP_DECNET_CON *pcon )
{                      
    CX_PPP_DECNET_CTX  *dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS      osstatus;

    /* Count one less pending AST for debugging */
    CSadjust_counter( &dnctx->cx_dnppp_pendast, -1 );

    /* Check status or for 0 length read (EOF) */          
    osstatus = pcon->cx_dnppp_iosb.iosb$w_status;
    if (cx_deadlock_debug && !lib$ast_in_prog())
	TRdisplay("%@ cx_decnet_receive_0: dchan %x pendast %d IOSB status %x\n\t"
	    "byte count %d head %d tail %d\n", pcon->cx_dnppp_dchan,
	    dnctx->cx_dnppp_pendast,
	    pcon->cx_dnppp_iosb.iosb$w_status, pcon->cx_dnppp_iosb.iosb$w_bcnt,
	    pcon->cx_dnppp_bufhead, pcon->cx_dnppp_buftail), TRflush();

    if ( SS$_BUFFEROVF == osstatus )
    {
	/* We expect this, as we sometimes provide a too small buffer */
	osstatus = SS$_NORMAL;
    }

    if (osstatus != SS$_NORMAL) 
    {
	TRdisplay("%@ cx_decnet_receive_0: dchan %x pendast %d osstatus %x\n\t"
	    "byte count %d head %d tail %d\n", pcon->cx_dnppp_dchan,
	    dnctx->cx_dnppp_pendast, osstatus,
	    pcon->cx_dnppp_iosb.iosb$w_bcnt,
	    pcon->cx_dnppp_bufhead, pcon->cx_dnppp_buftail), TRflush();

	pcon->cx_dnppp_status = E_CL2C4A_CX_E_PPP_RECEIVE;
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
	cx_decnet_close_con( pcon );
    }
    else if (pcon->cx_dnppp_iosb.iosb$w_bcnt == 0)
    {
        TRdisplay("%@ cx_decnet_receive_0: byte count ZERO\n"); TRflush();
    }
    else
    {
	pcon->cx_dnppp_bufhead += pcon->cx_dnppp_iosb.iosb$w_bcnt;
	if ( pcon->cx_dnppp_bufhead == pcon->cx_dnppp_bufsize )
	{
	    /* We've wrapped */
	    pcon->cx_dnppp_bufhead = 0;
	}

	if ( pcon->cx_dnppp_bufhead == pcon->cx_dnppp_buftail )
	{
	    /* We've caught up to the tail, we need to stall. */
	    SET_BIT( &dnctx->cx_dnppp_stallmask, \
		(1 << (pcon - dnctx->cx_dnppp_cons)) );
	}
	else
	{
	    /* Post next read for this channel */
	    cx_decnet_receive( pcon );
	}
    }

    /*
    ** Error or no, wake reader if reader is currently suspended.
    ** Reader will post an accept if "active" is zero, and a slot is free.
    */
    if ( 1 == CSadjust_counter( &dnctx->cx_dnppp_pending, 1 ) )
    {
	if (cx_deadlock_debug && !lib$ast_in_prog())
	    TRdisplay("%@ cx_decnet_receive_0: dchan %x resumed\n",
		pcon->cx_dnppp_dchan), TRflush();
	CSresume( dnctx->cx_dnppp_session );
    }
    else if (cx_deadlock_debug && !lib$ast_in_prog())
    {
	TRdisplay("%@ cx_decnet_receive_0: dchan %x not resumed, pending %d\n",
	    pcon->cx_dnppp_dchan, dnctx->cx_dnppp_pending); TRflush();
    }
} /* cx_decnet_receive_0 */
    
 
/*
** Close a connection from readers side, and start accepting
** new connections if freeing this connection makes a slot
** available, and no CX_PPP_DECNET_CON structs were free before.
*/
static void  
cx_decnet_close_con( CX_PPP_DECNET_CON *pcon )
{                      
    CX_PPP_DECNET_CTX *dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS      osstatus;

    if ( dnctx->cx_dnppp_inusemask & 
         (1 << (pcon - dnctx->cx_dnppp_cons)) )
    {
	/* Only do this if in use */
        if (cx_deadlock_debug && !lib$ast_in_prog())
            TRdisplay("%@ cx_decnet_close_con: cancel dchan %x\n",
                pcon->cx_dnppp_dchan), TRflush();
	osstatus = sys$cancel( pcon->cx_dnppp_dchan );

	osstatus = sys$qio(0, 
			 pcon->cx_dnppp_dchan,
			 IO$_DEACCESS|IO$M_SYNCH, 
			 &pcon->cx_dnppp_iosb,
			 cx_decnet_close_con_0,
			 pcon,
			 0, 0, 0, 0, 0, 0);

	if( osstatus != SS$_NORMAL )
	{
	    pcon->cx_dnppp_status = E_CL2C4C_CX_E_PPP_CLOSE;
	    cx_decnet_close_con_0( pcon );
	}
    }
}

/*
** Completion routine for closing a read connection.
*/
static void  
cx_decnet_close_con_0( CX_PPP_DECNET_CON *pcon )
{                      
    CX_PPP_DECNET_CTX *dnctx = pcon->cx_dnppp_pctx;

    cx_decnet_deassign( pcon );
    if ( !dnctx->cx_dnppp_active )
    {
	cx_decnet_accept(pcon);
    }
}


/*{
** Name: CXppp_read - Return pointer to buffered data or suspend.
**
** Description:
**
**	Will return a pointer to any buffered data of at least
**	recordsize bytes. 
**	
**	Preference is given to those buffers which are fullest.
**	It is here also that we check for the need to un-stall
**	buffers which had gotten completely full but now have
**	some space freed up.
**	
** Inputs:
**
**	ctx - address of previously allocated PPP context.
**		
**	recordsize - quantum of data required.  Buffered data of
**		less than this size is ignored until more is read,
**		and read "cursor" only advances in integer multiples
**		of this size.  ALL calls to read should pass the
**		same record size. (Maybe should be an alloc param?).
**
**	maxrecstoread - Max number to return in recordsread.  If non-zero
**		this may foster "fairness", as one very full buffer
**		will only get maxrecstoread records processed instead
**		of monopolizing the reader thread despite other buffers
**		being as or almost as full.
**		If zero, this call is handled as a non-read read, i.e.
**		buffered data just consumed by the previous read is
**		made available to pending read requests.
**
**	timeout - Timeout value passed to CSsuspend if there is no data
**		buffered.  If 0, routine will suspend until at least 
**		recordsize bytes are available in at least one of its
**		buffers.
**
**	nextrecord - Address of pointer to hold address of available
**		buffered data.
**		
**	recordsread - address of integer place # of records available.
**		
** Outputs:
**
**	*nextrecord - If status OK will hold address of available
**		buffered data.
**		
**	*recordsread - If status OK will hold # of records available.
**		
**	None.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CS0008_INTERRUPTED    - Interrupted.
**		E_CS0009_TIMEOUT	- Time out without receiving data.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
**	12-jan-2007 (jonj)
**	    Modified to round-robin the channels instead of weighting the
**	    "busiest" and starving the least-busiest.
**	21-Jan-2007 (jonj)
**	    Grab bufhead into readhead, reference readhead while AST 
**	    modifies bufhead.
**	14-Feb-2007 (jonj)
**	    Give precedence to stalled connections.
**	21-Feb-2007 (jonj)
**	    If maxrecstoread is zero, release consumed data space,
**	    unstall any stalled connections, return without actually
**	    reading anything.
*/
STATUS
CXppp_read( CX_PPP_CTX ctx, i4 recordsize, i4 maxrecstoread, i4 timeout,
            char **nextrecord, i4 *recordsread )
{
    CX_PPP_DECNET_CTX *dnctx = ctx;
    CX_PPP_DECNET_CON *pcon;
    i4		con, temp, pending, active;
    i4		stalled_con;
    STATUS	status = OK;

    /* Set up for search */
    *recordsread = 0;
    *nextrecord = NULL;
    
    /* Report deferred decnet errors */
    if ( dnctx->cx_dnppp_errsmask & (1<<0) )
    {
	status = cx_translate_status(
	 dnctx->cx_dnppp_cons[0].cx_dnppp_iosb.iosb$w_status );
	if (status)
	    TRdisplay("%@ CXppp_read:  deferred error %x\n", status), TRflush();
	CLR_BIT( &dnctx->cx_dnppp_errsmask, (1<<0) );
	if ( E_CL2C49_CX_E_PPP_LISTEN ==
	     dnctx->cx_dnppp_cons[0].cx_dnppp_status )
	{
	    cx_decnet_accept( dnctx->cx_dnppp_cons );
	}
    }

    while ( status == OK && 0 == *recordsread )
    {
	/* Set up for a search */
	pending = dnctx->cx_dnppp_pending;
	con = 0;
	active = 0;
	stalled_con = 0;

	/* Loop over all connections finding all with something to read */
	while ( con++ < dnctx->cx_dnppp_maxrcon )
	{
	    pcon = &dnctx->cx_dnppp_cons[con];

	    /*
	    ** Advance tail to next, since data made available through
	    ** nextrecord from previous call to CXppp_read is implicitly
	    ** consumed. 
	    */
	    pcon->cx_dnppp_buftail = pcon->cx_dnppp_bufnext;

	    /* Grab current bufhead value from AST */
	    pcon->cx_dnppp_readhead = pcon->cx_dnppp_bufhead;

	    /* Can a stalled connection be resumed?. */
	    if ( dnctx->cx_dnppp_unstallmask & (1 << con) )
	    {
		/*
		** There's now some space to read more data. 
		** reset stall bit, and post a new read.
		*/
		if (cx_deadlock_debug && !lib$ast_in_prog())
		    TRdisplay("%@ CXppp_read: %x is unstalled\n",
			pcon->cx_dnppp_dchan), TRflush();
		CLR_BIT( &dnctx->cx_dnppp_stallmask, (1 << con) );
		CLR_BIT( &dnctx->cx_dnppp_unstallmask, (1 << con) );
		cx_decnet_receive( pcon );
	    }

	    /* Really reading? */
	    if ( maxrecstoread > 0 )
	    {
		if ( pcon->cx_dnppp_readhead < pcon->cx_dnppp_buftail )
		{
		    /* [XXH====TXXXX] case */
		    pcon->cx_dnppp_bytes =  pcon->cx_dnppp_readhead +
			pcon->cx_dnppp_bufsize - pcon->cx_dnppp_buftail;
		}
		else if ( pcon->cx_dnppp_readhead > pcon->cx_dnppp_buftail )
		{
		    /* [===TXXXXXH==] case */
		    pcon->cx_dnppp_bytes = pcon->cx_dnppp_readhead - pcon->cx_dnppp_buftail;
		}
		else if ( dnctx->cx_dnppp_stallmask & (1 << con) )
		{
		    /* if head = tail & stalled then full */
		    if (cx_deadlock_debug && !lib$ast_in_prog())
			TRdisplay("%@ CXppp_read: *** %x is stalled ***\n",
			    pcon->cx_dnppp_dchan), TRflush();
		    pcon->cx_dnppp_bytes = pcon->cx_dnppp_bufsize;

		    /* Note (first) stalled connection */
		    if ( stalled_con == 0 )
			stalled_con = con;
		}
		else
		{
		    pcon->cx_dnppp_bytes = 0;
		}

		if ( pcon->cx_dnppp_bytes / recordsize )
		    active++;
	    }
	}

	/* If just freeing up buffer space, unstalling, return */
	if ( maxrecstoread == 0 )
	    return(OK);

	if ( active == 0 )
	{
	    /* There is nothing to return right now */
	    if (cx_deadlock_debug)
		TRdisplay("%@ CXppp_read: CSadjust_counter session %p "
		    "cx_dnppp_pending %d pending local %d\n\t"
		    "inuse %x stall %x unstall %x pendast %d\n",
		    dnctx->cx_dnppp_session, dnctx->cx_dnppp_pending, pending,
		    dnctx->cx_dnppp_inusemask, dnctx->cx_dnppp_stallmask, 
		    dnctx->cx_dnppp_unstallmask, dnctx->cx_dnppp_pendast), 
		    TRflush();
	    if ( 0 == CSadjust_counter( &dnctx->cx_dnppp_pending, -pending ) )
	    {
		/* No one posted while we were searching, suspend. */
		if ( timeout == 0 && cx_deadlock_debug )
		    timeout = 30;
		temp = CS_INTERRUPT_MASK;
		if ( timeout != 0 )
		    temp |= CS_TIMEOUT_MASK;
		/* Show session suspended on BIO read */
		do 
		{
		    status = CSsuspend( temp, timeout, 0 );
		    if ( status == E_CS0009_TIMEOUT && cx_deadlock_debug )
		    {
			TRdisplay("%@ CXppp_read timeout: dnctx %x pending %d errsmask %x nextrecord %x recordsread %d\n\t"
				"inuse %x stall %x unstall %x pendast %d\n",
				dnctx, dnctx->cx_dnppp_pending,
				dnctx->cx_dnppp_errsmask,
				*nextrecord, *recordsread,
			    dnctx->cx_dnppp_inusemask, dnctx->cx_dnppp_stallmask, 
			    dnctx->cx_dnppp_unstallmask, dnctx->cx_dnppp_pendast), 
			TRflush();
			CS_breakpoint();
		    }
		} while ( status == E_CS0009_TIMEOUT );
	    }
	}
	else
	{
	    /* Find next active channel to return data from, round-robin */

	    for ( con = ++dnctx->cx_dnppp_rcon; ; ++con)
	    {
		/* If at last channel, wrap to first */
		if ( con > dnctx->cx_dnppp_maxrcon )
		    con = 1;

		/* Stalled connection overrides round-robin */
		if ( stalled_con )
		    con = stalled_con;

		pcon = &dnctx->cx_dnppp_cons[con];

		/* If this one has something to read, take it */
		if ( pcon->cx_dnppp_bytes )
		{
		    /* Update last channel read */
		    dnctx->cx_dnppp_rcon = con;
		    break;
		}
	    }

	    /*
	    ** Return pointer to data, and adjust "next" to point
	    ** past an integer numbers of records in the buffer.
	    **
	    ** "tail" & "next" always adjust in record size increments,
	    ** while "head" is subject to the whims of decnet.
	    */
	    *nextrecord = pcon->cx_dnppp_buf + pcon->cx_dnppp_buftail;

	    if ( pcon->cx_dnppp_readhead <= pcon->cx_dnppp_buftail )
	    {
		/*[XXH====TXXXX] case */
		temp = pcon->cx_dnppp_bufsize - pcon->cx_dnppp_buftail;
	    }
	    else
	    {
		/*[===TXXXXXH==] case */
		temp = pcon->cx_dnppp_readhead - pcon->cx_dnppp_buftail;
	    }

	    *recordsread = temp / recordsize;
	    if ( maxrecstoread && *recordsread > maxrecstoread )
		*recordsread = maxrecstoread;
	    pcon->cx_dnppp_bufnext = pcon->cx_dnppp_buftail +
	       (*recordsread) * recordsize;
	    if ( pcon->cx_dnppp_bufnext == pcon->cx_dnppp_bufsize )
		pcon->cx_dnppp_bufnext = 0;
	    if ( dnctx->cx_dnppp_stallmask & (1 << con) )
	    {
		/* When we've finish processing this read we can resume. */
		SET_BIT( &dnctx->cx_dnppp_unstallmask, (1 << con) );
	    }
	}
    }

    return status;
}	/* CXppp_read */



/*{
** Name: CXppp_close - Close all pipes for Listener.
**
** Description:
**
**	Break connection to all writers, and deallocate all
**	communication resources while retaining structure.
**
** Inputs:
**
**	ctx - address of previously allocated PPP context.
**		
** Outputs:
**	None.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CL2C4C_CX_E_PPP_CLOSE - Some problem deallocating.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
*/
STATUS
CXppp_close( CX_PPP_CTX ctx )
{
    CX_PPP_DECNET_CTX *dnctx = ctx;
    CX_PPP_DECNET_CON	*pcon;

    for ( pcon = dnctx->cx_dnppp_cons;
          pcon <= dnctx->cx_dnppp_cons + dnctx->cx_dnppp_maxrcon;
	  pcon++ )
    {
	if ( dnctx->cx_dnppp_inusemask & 
	     (1 << (pcon - dnctx->cx_dnppp_cons)) )
	{
	    cx_decnet_close( pcon );
	    (void)CSsuspend( CS_BIO_MASK, 0, 0 );
	}
    }
    return OK;
}



/*{
** Name: CXppp_connect - Establish connection with a listener. 
**
** Description:
**
**	Connect with a previoly established listener.
**
** Inputs:
**
**	ctx - address of previously allocated PPP context.
**		
**	host - Name of target host to find listener.
**
**	service - Name of target listener.
**
** Outputs:
**	None.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CL2C4D_CX_E_PPP_CONNECT - Could not establish endpoint.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
*/
STATUS
CXppp_connect( CX_PPP_CTX ctx, char *host, char *service )
{
    CX_PPP_DECNET_CTX		*dnctx = ctx;
    CX_PPP_DECNET_CON		*pcon  = dnctx->cx_dnppp_cons;
    struct dsc$descriptor_s	 ncb_desc;	/* Network Connect Block */
    STATUS			 status = OK;
    OSSTATUS			 osstatus;
    
    do
    {
	/* Sanity checks */
	if ( dnctx->cx_dnppp_maxrcon != 0 ||
	     dnctx->cx_dnppp_inusemask != 0 )
	{
	    /* Not a valid ctx or allocated for write only. */
	    status = E_CL2C4D_CX_E_PPP_CONNECT;
	    break;
	}

	dnctx->cx_dnppp_inusemask = (1<<0);
	CSget_sid( &dnctx->cx_dnppp_session );

 	osstatus = attach_decnet( &pcon->cx_dnppp_dchan,
	                          &pcon->cx_dnppp_mchan );
 	if (osstatus != SS$_NORMAL)
	{
	    break;
	}

	/* Publish channels for debugging */
	TRdisplay("%@ CXppp_connect: CTX %x CON[0] %x dchan %x mchan %x\n",
			dnctx,
			pcon,
			pcon->cx_dnppp_dchan,
			pcon->cx_dnppp_mchan), TRflush;

	/* 
	** Format host, and service into 'host::"/task=port"'
	** Be careful, no bounds checking.
	*/
	STprintf( (char*)&pcon->cx_dnppp_msg_area, "%s::\"task=%s\"", 
	         host, service );

	/* Construct Network Connect Block (NCB) */
	ncb_desc.dsc$a_pointer = (char*)&pcon->cx_dnppp_msg_area;
	ncb_desc.dsc$w_length = STlength( (char*)&pcon->cx_dnppp_msg_area );

	/* Request logical link connection to remote DECNET listener */

	osstatus = sys$qio(0,
			   pcon->cx_dnppp_dchan,
			   IO$_ACCESS, 
			   &pcon->cx_dnppp_iosb,
			   cx_continue_ast,
			   &dnctx->cx_dnppp_session,
			   0,
			   &ncb_desc, 
			   0, 0, 0, 0);
	if( osstatus != SS$_NORMAL )
	{
	    break;
	}

	status = CSsuspend( CS_BIO_MASK, 0, 0 );
	if ( status )
	{
	    break;
	}

	if ((osstatus = pcon->cx_dnppp_iosb.iosb$w_status) != SS$_NORMAL)
	{
	    break;
	}

	/* 
	** Issue read for the logical link completion message
	** to the network mailbox.
	*/

	osstatus = sys$qio(0,
			 pcon->cx_dnppp_mchan,
			 IO$_READVBLK, 
			 &pcon->cx_dnppp_iosb,
			 cx_continue_ast,
			 &dnctx->cx_dnppp_session,
			 &pcon->cx_dnppp_msg_area, 
			 sizeof(DECNET_MSG), 
			 0, 0, 0, 0);

	if ( osstatus != SS$_NORMAL )
	{
	    break;
	}

	status = CSsuspend( CS_BIO_MASK, 0, 0 );
	if ( status )
	{
	    break;
	}

	if( ( osstatus = pcon->cx_dnppp_iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    break;
	}

	/* Check the logical link completion message */

	if( pcon->cx_dnppp_msg_area.msg_type != MSG$_CONFIRM )
	{
	    status = E_CL2C4D_CX_E_PPP_CONNECT;
	}

    } while (0);

    if ( OK == status )
    {
	pcon->cx_dnppp_iosb.iosb$w_status = osstatus;
        status = cx_translate_status( osstatus );
    }
    return status;
}



/*{
** Name: CXppp_write - Write data to an open connection.
**
** Description:
**
**	Send bytes to a open connection.  Write is asynchronous,
**	but routine will suspend until all data is sent.
**
** Inputs:
**
**	ctx - address of previously allocated PPP context.
**		
**	buffer - address of data to write
**
**	recordsize - size of record to write.
**
**	recordcount - number of records to write.
**
** Outputs:
**	None.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CL2C4B_CX_E_PPP_SEND  - Could not establish endpoint.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
**	12-Feb-2007 (jonj)
**	    Added debugging aids to find out why writes occasionally
**`	    hang.
**	04-may-2007 (abbjo03)
**	    After the QIO, you shouldn't look at the IOSB status until you
**	    know the I/O has completed.
**	08-Oct-2007 (abbjo03)
**	    Reduce write timeout from 30 to 5 seconds, which is still
**	    way more than ought to be needed.
**	30-Apr-2008 (jonj)
**	    Make suspend interruptable. If we get caught in a neverending
**	    hung write situation, we still want the RCP to shut down
**	    "cleanly".
**
*/
static void  cx_write_ast( CX_PPP_DECNET_CTX *dnctx );
void cx_write_ast( CX_PPP_DECNET_CTX *dnctx )
{
    CX_PPP_DECNET_CON *pcon = dnctx->cx_dnppp_cons;
    pcon->cx_dnppp_readhead++;
    CSresume( dnctx->cx_dnppp_session );
    pcon->cx_dnppp_readhead++;
}

STATUS
CXppp_write( CX_PPP_CTX ctx, char *buffer, i4  recordsize, i4 recordcount )
{
    CX_PPP_DECNET_CTX *dnctx = ctx;
    CX_PPP_DECNET_CON *pcon  = dnctx->cx_dnppp_cons;
    STATUS	       status = OK;
    OSSTATUS           osstatus;
    i4		       bytesleft;

    bytesleft = recordsize * recordcount;

    /* Use "readhead" to debug qio/AST/CSresume progress */
    pcon->cx_dnppp_readhead = 0;

    do
    {
	if (cx_deadlock_debug)
	    TRdisplay("%@ CXppp_write: bytesleft %d dchan %x\n", bytesleft,
		pcon->cx_dnppp_dchan), TRflush();
	osstatus = sys$qio(
			0,
			pcon->cx_dnppp_dchan,
			IO$_WRITEVBLK,
			&pcon->cx_dnppp_iosb, 
			cx_write_ast,
			dnctx,
			buffer,
			bytesleft,
			0, 0, 0, 0);

	if (osstatus != SS$_NORMAL)
	{
	    TRdisplay("%@ CXppp_write: failed sys$qio on channel %d: "
		"status %x\n", pcon->cx_dnppp_dchan, osstatus);
	    TRflush();
	    break;
	}

	/* Show suspended on BIO write */
        do
        {
	    /* For debugging, force a 5-second timeout; hung writes are serious */
	    status = CSsuspend( CS_BIOW_MASK | CS_TIMEOUT_MASK | CS_INTERRUPT_MASK,
				5, 0 );
	    if ( status == E_CS0009_TIMEOUT )
	    {
		TRdisplay("%@ CXppp_write timeout: dnctx %x pcon %x dchan %x writing %d of %d bytes, readhead %d osstatus %x\n",
		    dnctx, pcon, pcon->cx_dnppp_dchan,
		    bytesleft, recordsize * recordcount,
		    pcon->cx_dnppp_readhead, pcon->cx_dnppp_iosb.iosb$w_status);
		TRflush();
		CS_breakpoint();
		pcon->cx_dnppp_readhead += 5;
	    }
        } while ( status == E_CS0009_TIMEOUT );

	osstatus = pcon->cx_dnppp_iosb.iosb$w_status;
	if ( ( OK != status ) ||
	     ( SS$_NORMAL != osstatus && SS$_DATAOVERUN != osstatus ) )
	{
	    TRdisplay("%@ CXppp_write: failed sys$qio on channel %d: "
		"IOSB status %x\n", pcon->cx_dnppp_dchan, osstatus), TRflush();
	    break;
	}

	buffer += pcon->cx_dnppp_iosb.iosb$w_bcnt;
	bytesleft -= pcon->cx_dnppp_iosb.iosb$w_bcnt;
	if (cx_deadlock_debug && !lib$ast_in_prog())
	    TRdisplay("%@ CXppp_write: wrote %d bytes bytesleft %d dchan %x\n",
		pcon->cx_dnppp_iosb.iosb$w_bcnt, bytesleft,
		pcon->cx_dnppp_dchan), TRflush();
    } while ( OK == status && bytesleft > 0 );

    if ( OK == status )
    {
	pcon->cx_dnppp_iosb.iosb$w_status = osstatus;
	status = cx_translate_status( osstatus );
    }

    if ( OK != status )
    {
	TRdisplay("%@ CXppp_write: failed on channel %d, status %x, calling close\n",
		    pcon->cx_dnppp_dchan, status), TRflush();
	pcon->cx_dnppp_status = E_CL2C4B_CX_E_PPP_SEND;
	dnctx->cx_dnppp_errsmask |= (1<<0);
	cx_decnet_close( pcon );
	(void)CSsuspend( CS_BIO_MASK, 0, 0 );
    }
    return status;
}



/*{
** Name: CXppp_disconnect - Deallocate writer communication resources.
**
** Description:
**
**	close coonection for a writer.
**
** Inputs:
**
**	ctx - pointer to PPP context.
**		
** Outputs:
**	none
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		E_CL2C4C_CX_E_PPP_CLOSE - Problem in deallocation.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-may-2005 (devjo01)
**	    Created.
*/
STATUS
CXppp_disconnect( CX_PPP_CTX ctx )
{
    return CXppp_close( ctx );
}


/*
** Close a connection (for close or disconnect), then resume caller.
*/
static void  
cx_decnet_close( CX_PPP_DECNET_CON *pcon )
{                      
    CX_PPP_DECNET_CTX *dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS      osstatus;

    /* Only do this if in use */
    osstatus = sys$cancel( pcon->cx_dnppp_dchan );

    osstatus = sys$qio(0, 
		     pcon->cx_dnppp_dchan,
		     IO$_DEACCESS|IO$M_SYNCH, 
		     &pcon->cx_dnppp_iosb,
		     cx_decnet_close_0,
		     pcon,
		     0, 0, 0, 0, 0, 0);

    if( osstatus != SS$_NORMAL )
    {
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
	pcon->cx_dnppp_status = E_CL2C4C_CX_E_PPP_CLOSE;
	CSresume( dnctx->cx_dnppp_session );
    }
}

/*
** Completion routine for final connection close.
*/
static void  
cx_decnet_close_0( CX_PPP_DECNET_CON *pcon )
{                      
    cx_decnet_deassign( pcon );
    CSresume( pcon->cx_dnppp_pctx->cx_dnppp_session );
}

/* ---- Utility routines ---- */

/*
** Resume session on AST completion.
*/
static void
cx_continue_ast( CS_SID *session )
{
    if (cx_deadlock_debug && !lib$ast_in_prog())
	TRdisplay("% cx_continue_ast: call CSresume %p\n", *session), TRflush();
    CSresume( *session );
}

/*
** Deallocate/deassign network resources for a connection. */
static void  
cx_decnet_deassign( CX_PPP_DECNET_CON *pcon )
{                      
    CX_PPP_DECNET_CTX *dnctx = pcon->cx_dnppp_pctx;
    OSSTATUS      osstatus;

    osstatus = pcon->cx_dnppp_iosb.iosb$w_status;

    if( osstatus != SS$_NORMAL) 
    {
	pcon->cx_dnppp_status = E_CL2C4C_CX_E_PPP_CLOSE;
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
    }
    osstatus = sys$dassgn(pcon->cx_dnppp_dchan);
    if( osstatus != SS$_NORMAL) 
    {
	pcon->cx_dnppp_status = E_CL2C4C_CX_E_PPP_CLOSE;
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
    }
    osstatus = sys$dassgn(pcon->cx_dnppp_mchan);
    if( osstatus != SS$_NORMAL) 
    {
	pcon->cx_dnppp_status = E_CL2C4C_CX_E_PPP_CLOSE;
	dnctx->cx_dnppp_errsmask |=
	    (1 << (pcon - dnctx->cx_dnppp_cons));
    }
    pcon->cx_dnppp_dchan = pcon->cx_dnppp_mchan = -1;
    dnctx->cx_dnppp_inusemask &= 
     ~( 1 << ( pcon - dnctx->cx_dnppp_cons ) );
}

/* Create a mailbox, and assign an associated channel to "_NET:".
** The mailbox is used to receive status & interrupt messages
*/

static OSSTATUS
attach_decnet(II_VMS_CHANNEL *dchan, II_VMS_CHANNEL *mchan)
{
    OSSTATUS		osstatus;
    char		netmbx_name[20];
    struct dsc$descriptor_s	netmbx_desc;
    struct dsc$descriptor_s     net_desc;

    *mchan = -1;
    *dchan = -1;
    osstatus = sys$crembx(0, mchan, MBX_MAX_MSG, MBX_MAX_MSG, MBX_PROT_MASK,
	0, 0);
    if (osstatus != SS$_NORMAL)
	return (osstatus);

    /* Determine mailbox name */
    osstatus = mbx_name(*mchan, netmbx_name);
    if (osstatus != SS$_NORMAL)
	return (osstatus);

    /* Set up descriptors for ...
    ** network mailbox and network psuedo-device.
    */
    netmbx_desc.dsc$a_pointer = netmbx_name;
    netmbx_desc.dsc$w_length = STlength(netmbx_name);
    net_desc.dsc$a_pointer = "_NET:";
    net_desc.dsc$w_length = 5;

    /* Associate mailbox just created with network psuedo-device */
    osstatus = sys$assign(&net_desc, dchan, 0, &netmbx_desc);
    return (osstatus);
}

/* Determine systems name for mailbox given a channel to it.
** This routine exists in gcacl.c but the desire is to keeep this
** module self-contained (apart from official CL routines).
*/

static OSSTATUS
mbx_name(II_VMS_CHANNEL mbx_chan, char *mbx_name)
/* mbx_name : Must point to area >= 16 bytes */
{
    unsigned short mbx_namelen = 0;
    OSSTATUS	osstatus;
    ILE3	dvi_list[] =
		 {
		    /* We want the name and it's length*/
		    {16, DVI$_DEVNAM, mbx_name, &mbx_namelen},
		    {0, 0, 0, 0}
		 };
    IOSB	iosb;

    /* Ask the O.S. for the mailbox name */

    osstatus = sys$getdviw(EFN$C_ENF, 	    /* event flag */
			mbx_chan,   /* Channel to mailbox whose name we want */
			0, 
			&dvi_list, 
			&iosb, 0, 0, 0 );
    if (osstatus & 1)
	osstatus = iosb.iosb$w_status;
    if ((osstatus & 1) != 0)
    {
	/* If OK then null terminate */                 
	mbx_name[mbx_namelen] = 0;
    }
    return (osstatus);
}

# else

/*
** All non-VMS platforms.
*/

STATUS
CXppp_alloc( CX_PPP_CTX *hctx, char *name, i4 readconnections, i4 bufsize,
	bool trace )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

STATUS
CXppp_free( CX_PPP_CTX *hctx )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

STATUS
CXppp_listen( CX_PPP_CTX ctx )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

STATUS
CXppp_read( CX_PPP_CTX ctx, i4 recordsize, i4 maxrecstoread, i4 timeout,
            char **nextrecord, i4 *recordsread )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

CXppp_close( CX_PPP_CTX ctx )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

STATUS
CXppp_connect( CX_PPP_CTX ctx, char *host, char *service )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

STATUS
CXppp_write( CX_PPP_CTX ctx, char *buffer, i4  recordsize, i4 recordcount )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

STATUS
CXppp_disconnect( CX_PPP_CTX ctx )
{
    return E_CL2C10_CX_E_NOSUPPORT;
}

# endif
