/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>
# include	<pc.h>

#if defined(xCL_066_UNIX_DOMAIN_SOCKETS) || defined(xCL_TLI_UNIX_EXISTS)

# include	<errno.h>
# include	<bsi.h>

# include	<systypes.h>
# include	<clsocket.h>
# include	"handylocal.h"

# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif /* xCL_006_FCNTL_H_EXISTS */


/*
** Name: bsunixaddr.c - support routines for Unix Domain Sockets
**
** Description:
**
** History:
**	17-Sep-92 (gautam)
**	   Created from bssockunix.c
**	10-Feb-93 (smc)
**	   Commented out text after endif.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Nov-2010 (frima01) SIR 124685
**	    Added include of handylocal.h for prototype.
*/


/*
** Name: BS_unix_addr - turn a string into unix domain socket path
**
** Description:
**	This routine takes a listen address and turns it into a unix
**	domain socket pathname.  The inputs are these:
**
**		INPUT			OUTPUT
**		-------			------
**		/path/file		/path/file
**		/path/			/path/ii.pid
**		(null)			/tmp/ii.pid
*/

STATUS
BS_unix_addr( buf, s )
char	*buf;
struct sockaddr_un *s;
{
	PID		pid;
	i4		len;

	/* Keep it in the family */

	s->sun_family = AF_UNIX;

	if( !( len = STlength( buf ) ) || buf[ len - 1 ] == '/' )
	{
	    /* if given no hard path, use /tmp/ii.pid */
	    /* if given /dir../, use /dir../ii.pid */

	    PCpid( &pid );

	    STprintf( s->sun_path, "%sii.%d", len ? buf : "/tmp/", pid );
	}
	else
	{
	    /* just use the path given */

	    STcopy( buf, s->sun_path );
	}

# ifdef UNIXDEBUG
	TRdisplay( "unix_addr '%s' is '%s'\n", buf, s->sun_path );
# endif /* UNIXDEBUG */
	return OK;
}
#endif /* xCL_066_UNIX_DOMAIN_SOCKETS */
