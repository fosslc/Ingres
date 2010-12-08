/*
**Copyright (c) 2006, 2007 Ingres Corporation
*/
/*
** Name: bssockio.h - common definitions for BS interface to sockets
**
** History:
**	6-Sep-90 (seiwald)
**	    Extracted from bssockunix.c.
**      11-may-93 (arc)
**          Add security label field to Connection Control Block.
**	12-nov-93 (robf)
**          Remove security label field from BCB.
**      17-Mar-94 (seiwald)
**          Once an I/O error occurs on a socket, avoid trying further
**          I/O's: they tend to hang on SVR4 machines.
**	1-jun-94 (robf)
**          Add SOCK_TRACE macro to preserve tracing previously added.
**	1-jun-94 (arc)
**          SOCK_TRACE uses sock_trace variable (not sock_debug).
**      28-jun-00 (loera01) Bug 101800:
**          Cross-integrated from change 276901 in ingres63p:
**          Move BCB struct to bsi.h, so that the protocol driver can see the
**	    contents.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 7-oct-2006 (lunbr01)  Sir 116548
**	    Add addrinfo data for listens to LBCB to support IPv6.
**	 1-mar-2007 (lunbr01)  Bug 117783 + Sir 116548
**	    Add counter of good sockets in aiList to LBCB.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Tighten up callback prototype.
*/

#define  SOCK_TRACE(n)	if(sock_trace >= n)(void)TRdisplay
/*
** Name: LBCB - listen control block
*/

typedef struct _LBCB
{
	i4	fd;			/* listen fd (active) */
	char	port[ 108 ];		/* port we're listening on */
	VOID	(*func)(void *, i4);	/* saved regop callback function */
	void	*closure;		/* Listen request parm list */
	PTR	aiList;			/* addrinfo list from getaddrinfo() */
	i4	num_sockets;		/* # of sockets/addrs in aiList */
	i4	num_good_sockets;	/* # of good sockets in aiList */
	struct	fdinfo
	{
	    i4      fd_ai;		/* listen fd per entry in aiList*/
	    i4      fd_state;		/* state of fd */
#define		    FD_STATE_INITIAL		0
#define		    FD_STATE_REGISTERED		1
#define		    FD_STATE_PENDING_LISTEN_REQ	2
	    PTR     lbcb_self;		/* ptr to this lbcb...to set context */
	}	*fdList;
} LBCB;
