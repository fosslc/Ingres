/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<pc.h>
# include	<er.h>

# include	<errno.h>
# include	<bsi.h>

# ifdef xCL_TLI_UNIX_EXISTS

# include	<systypes.h>
# include	<sys/stream.h>
# include	<tiuser.h>
# include	<sys/tihdr.h>

# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif

# ifdef xCL_007_SYS_FILE_EXISTS
# include	<sys/file.h>
# endif

# include	<sys/stropts.h>

# include	"bstliio.h"


/*
** Name: bstliunix.c - BS interface to Streams loopback driver
**
** Description:
**	This file provides access to the Streams loopback driver via TLI
**	using the generic BS interface defined in bsi.h.  See that file 
**	for a description of the interface.
**
**	The following functions are internal to this file:
**
**      The following functions are exported as part of the BS interface:
**
**              unix_listen - establish an endpoint for accepting connections
**              unix_request - make an outgoing connection
**
** History:
**	17-Sep-92 (gautam)
** 	   Based on Gordon's loopback driver
**	9-nov-92 (gautam)
**	    Fix from Kola for checking length of buffer in unix_detect().
**      04-dec-92 (deastman)
**         Add TLI_UNIX support for pym_us5.
**	12-jan-93 (seiwald)
**	   Removed all the hocus pocus about UNIX domain sockets.  The
**	   TLI loopback driver (at least on sequent) doesn't need any sort
**	   of file - much less named FIFO's - for addressing.  We cook
**	   a "flex-address" by concatenating ii. and the process pid.
**	   This whole thing should be called "bstliloop.c".
**	02-Mar-93 (brucek)
**	   Added tli_release to BS_DRIVER.
**	04-Mar-93 (edg)
**	   Removed detect logic.
**	16-Jun-93 (brucek)
**	   We now unconditionally use /dev/ticotsord as Unix device name.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-Jul-93 (brucek)
**	   Added tcp_transport arg on call to tli_listen.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-nov-2002 (loera01)
**          Re-introduced unix_accept as a wrapper for tli_accept() with
**          is_remote in BS_PARMS set to FALSE.
**	4-Dec-2008 (kschendel)
**	    Declare errno correctly (i.e. via errno.h).
*/

# define TLI_UNIX	"/dev/ticotsord"


/*
** BS entry points
*/

/*
** Name: unix_listen - establish an endpoint for accepting connections
*/

static VOID
unix_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	char	laddr[ 64 ];	/* for PID */
	char	*addr;
	i4	size;

	/* clear fd for sanity */

	lbcb->fd = -1;

	/* get device */

	tli_addr( TLI_UNIX, bsp->buf, lbcb->device, &addr );

	/* get listen port, if specified */

	if( !*addr )
	{
	    PID		pid;

	    PCpid( &pid );
	    STprintf( addr = laddr, "ii.%d", pid );
	}

	/* do tli listen */

	size = STlength( addr );

	tli_listen( bsp, addr, size, size, FALSE );

	if( bsp->status != OK )
	    return;

	/* get name for output */

	STcopy( addr, bsp->buf = lbcb->port );
}

/*
** Name: unix_request - make an outgoing connection
*/

static VOID
unix_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	char	device[ MAXDEVNAME ];
	char	*addr;
	i4	size;

	/* clear fd for sanity */

	bcb->fd = -1;

	/* get device */

	tli_addr( TLI_UNIX, bsp->buf, device, &addr );

	/* do tli connect request */

	size = STlength( addr );

	tli_request( bsp, device, addr, size, NULL, 0, size );
}

/*
** Name: unix_accept - accept a single connect request.
**
** Inputs:
**      bsp             parameters for BS interface. 
**
** Outputs:
**	bsp->is_remote gets set to TRUE.
**
** Returns:
**	void.
**
** Side effects:
**	bsp gets modified by tli_accept().
**
** History:
**      19-nov-2002 (loera01)
**          Created.
*/

static VOID
unix_accept( bsp )
BS_PARMS	*bsp;
{
    bsp->is_remote = FALSE;
    tli_accept(bsp);
}

/*
** Exported driver specification.
*/

BS_DRIVER BS_tliunix = {
	sizeof( BCB ),
	sizeof( LBCB ),
	unix_listen,
	tli_unlisten,
	unix_accept,
	unix_request,
	tli_connect,
	tli_send,
	tli_receive,
	tli_release,
	tli_close,
	tli_regfd,
	tli_sync,	/* save */
	tli_sync,	/* restore */
	0,	
} ;

# endif /* xCL_TLI_UNIX_EXISTS */

