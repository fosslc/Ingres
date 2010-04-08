/*
** Copyright (c) 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<pc.h>
# include	<lo.h>
# include	<nm.h>
# include	<cs.h>

# include	<systypes.h>
# include       <clsocket.h>

# include	<bsi.h>
# include	"bsshmio.h"

# include	<errno.h>

/* External variables */


/*
** Name: bsshmio.c - BS interface to single client shared memory
**
** Description:
**	This file provides access to a single client shared memory
**      interface, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are internal to this file:
**
**		shm_addr - turn a string into unix domain socket path
**
**	The following functions are exported as part of the BS interface:
**
**		shm_listen - establish an endpoint for accepting connections
**		shm_unlisten - stop accepting incoming connections
**		shm_accept - accept a single incoming connection
**		shm_request - make an outgoing connection
**		shm_connect - complete an outgoing connection
**
** History:
**	08-apr-95 (chech02,lawst01,canor01)
**	    Made from bssockunix.c
**	25-apr-95 (canor01)
**	    change permissions on memory directory while running
**	    a shared-memory batchmode connection
**	05-jun-95 (canor01)
**	    Back out above change. It is not needed in final implementation.
**	    Change GC_BATCH_SHM structure to be allocated as an array of
**	    two, one for send and one for receive, to catch those cases
**	    where both client and server try to send at once.
**	08-nov-1995 (canor01)
**	    Marked shared memory as free only if this process owns it.
**	10-nov-95 (hanch04)
**	    change refs for sock_xxx to iisock_xxx, cross integrating
**	    421436 from 11, this file does not exist in 11.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores.
**	02-Dec-1997 (allmi01)
**	    Added a cast of char * to bsm in the SIprintf call to resolve
**	    picky compiler errors.
**	19-jan-1999 (muhpa01)
**	    Remove NO-OPTIM for hp8_us5.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_uv1.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-nov-2002 (loera01)
**          Set is_remote to FALSE in BS_PARMS.
*/

extern	VOID	sock_ok();
extern  STATUS  sock_ext_info();

#define MAX_SHM_SIZE 8192

#define ME_SHMEM_DIR "memory"

static GC_BATCH_SHM	bsm, *GC_bsm[2] = {NULL,NULL};


/*
** Support routines - not visible outside this file.
*/

VOID
IIshm_clear_dirty()
{
  i4   idx;

  if (GC_bsm[0])
  {
    idx = shm_getcliservidx();
    GC_bsm[idx]->bat_dirty = 0;
  }
}

/*
** Name: shm_addr - turn a string into unix shared memory id
**
** Description:
**	This routine takes a listen address and turns it into a unix
**	shared memory id.
**
*/
#define SHM_ATTACH  0
#define SHM_CREATE  1
#define SHM_BASEPTR 2

PTR
shm_addr( bsp, op )
BS_PARMS	*bsp;
i4		op;
{
    PID		pid;
    i4		len;
    PTR		memory;
    SIZE_TYPE	alloc_pages;
    BCB		*bcb;
    i4		size;
    i4		flags;
    LOCATION		location, temp_loc;
    char        	loc_buf[MAX_LOC + 1];
    char        	loc_buf_fd[MAX_LOC + 1];
    char                *perms;

    if ( bsp != NULL )
    {
	bcb = (BCB *)bsp->bcb;
	bsp->status = OK;

	if( !( len = STlength( bsp->buf ) ) ||
           (STcmp( bsp->buf, "/tmp/IIB." ) &&
            STcmp( bsp->buf, "IIB." ) ))
	{
	    /* if given no "IIB.*" name , use IIB.pid */

	    PCpid( &pid );

	    STprintf( (char *)(bsm.bat_id), "IIB.%d", pid );
	}
	else
	{
	     char *s;

	    /* just use the path given */
            for (s= bsp->buf; s && *s != 'I'; s++);
	        STcopy( s, (char *) bsm.bat_id );
	}

# ifdef SHMDEBUG
	TRdisplay( "unix_addr '%s' is '%s'\n", bsp->buf, bsm.bat_id );
# endif /* SHMDEBUG */

	size = MAX_SHM_SIZE + sizeof(GC_BATCH_SHM);
	bsm.bat_pages =  size / ME_MPAGESIZE;
        if ( size % ME_MPAGESIZE )
	    bsm.bat_pages++;
    }

	if ( GC_bsm[0] == NULL )
	{
	flags = ME_SSHARED_MASK;
	if ( op == SHM_CREATE )
	    flags |= (ME_CREATE_MASK|ME_MZERO_MASK);
   	bsp->status = MEget_pages( flags,
                              bsm.bat_pages*2,
			      (char*)&bsm.bat_id,
			      (PTR*)&GC_bsm[0],
			      &alloc_pages,
			      bsp->syserr );
	if ( op == SHM_CREATE && bsp->status == ME_ALREADY_EXISTS )
	    bsp->status = MEget_pages( ME_SSHARED_MASK,
                              bsm.bat_pages*2,
			      (char*)&bsm.bat_id,
			      (PTR*)&GC_bsm[0],
			      &alloc_pages,
			      bsp->syserr );
  	if ( bsp->status )
	    return( NULL );

	GC_bsm[1] = (GC_BATCH_SHM*)((char*)GC_bsm[0]+
                                    bsm.bat_pages*ME_MPAGESIZE);

	if ( op == SHM_CREATE )
	{
	    STcopy( (char *) bsm.bat_id, (char *) GC_bsm[0]->bat_id );
	    GC_bsm[0]->bat_pages = 
	    GC_bsm[1]->bat_pages = bsm.bat_pages*ME_MPAGESIZE;
	}

	}

	if ( bsp == NULL && op == SHM_ATTACH )
	{
	    i4  idx;
    	    idx = shm_getcliservidx();
	    return (PTR) (GC_bsm[!idx]);
	}
	return (PTR) GC_bsm[0];
	
}


/*
** BS entry points
*/

/*
** Name: shm_listen - establish an endpoint for accepting connections
*/

static VOID
shm_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	struct  sockaddr_un     s[1];
	int     size = sizeof( *s );
	int	fd;
	GC_BATCH_SHM	*bsm;
	PID	pid;


	/* get listen port, if specified */

	if( ( bsm = (GC_BATCH_SHM*) shm_addr( bsp, SHM_CREATE ) ) == NULL )
	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}
	GC_bsm[0]->bat_inuse  = GC_bsm[1]->bat_inuse = 0;
	GC_bsm[0]->bat_signal = GC_bsm[1]->bat_signal = 0;
	GC_bsm[0]->bat_dirty  = GC_bsm[1]->bat_dirty  = 0;

	PCpid(&pid);
	GC_bsm[0]->process_id = pid;
	GC_bsm[1]->process_id = 0;

	bsp->pid = 0;

	STprintf(s->sun_path, "/tmp/%s", GC_bsm[0]->bat_id);
	s->sun_family = AF_UNIX;

        /* clear fd for sanity */

	lbcb->fd = -1;

	/* Remove old entry */

	(void)unlink( s->sun_path );


	/* do socket listen */

	iisock_listen( bsp, s, size );

	if( bsp->status != OK )
		return;

	/* format name for output */
	STprintf( lbcb->port, "%s", s->sun_path);
	bsp->buf = lbcb->port;

#if !(defined(sqs_ptx) || defined(dr6_us5) || \
      defined(nc4_us5)) || defined(pym_us5)
      (VOID)chmod(s->sun_path, 0666);
#endif

}

/*
** Name: shm_unlisten - stop accepting incoming connections
*/

static VOID
shm_unlisten( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
    LOCATION		location, temp_loc;
    char        	loc_buf[MAX_LOC + 1];
    char        	loc_buf_fd[MAX_LOC + 1];
    char                *perms;


	bsp->status = MEfree_pages( (PTR) GC_bsm[0], 
			(GC_bsm[0]->bat_pages/ME_MPAGESIZE)*2, bsp->syserr );

	if( bsp->status != OK )
		return;

	GC_bsm[0] = GC_bsm[1] = NULL;

        iisock_unlisten( bsp );

	if (bsp->status != OK)
	     return;

        (void)unlink(lbcb->port);

}
	
/*
** Name: shm_accept - accept a single incoming connection
**       The logic in this function assumes that only a 'server'
**       will issue an 'accept'.
*/

static VOID
shm_accept( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	struct sockaddr_un      s[1];
	int     size = sizeof( *s );

        bsp->is_remote = FALSE;
	if ( GC_bsm[0]->bat_inuse && PCis_alive(GC_bsm[1]->process_id) == FALSE)
	{
	    iisock_error( bsp, BS_ACCEPT_ERR );
	    return;
	}


	bcb->fd = -1;
	iisock_accept (bsp, s, size);
#if 0
# if ! defined(xCL_043_SYSV_POLL_EXISTS) && defined(xCL_020_SELECT_EXISTS)
	/* using the SELECT function, not POLL */
	/* wakeup server callback function */
        GC_bsm[0]->bat_signal = 1;
#endif
#endif
	bcb->fd *= -1;
	GC_bsm[0]->bat_inuse = 1;
	return;
}

/*
** Name: shm_request - make an outgoing connection
*/
static VOID
shm_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	GC_BATCH_SHM	*bsm;
	PID		pid;
	struct  sockaddr_un     s[1];
	i4     size = sizeof( *s );


	/* translate address */

	if( ( bsm = (GC_BATCH_SHM*) shm_addr( bsp, SHM_ATTACH ) ) == NULL )
    	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	if ( GC_bsm[0]->bat_inuse && PCis_alive(GC_bsm[1]->process_id) == TRUE)
        {
	    iisock_error( bsp, BS_CONNECT_ERR );
	    return;
	}

	PCpid( &pid );
	GC_bsm[1]->process_id = pid;


	bsp->status = OK;
	bcb->fd = -1;

        STprintf(s->sun_path, "/tmp/%s", GC_bsm[0]->bat_id);
        s->sun_family = AF_UNIX;

	GC_bsm[1]->bat_signal = 1;     /* wake client callback function */

        iisock_request(bsp, s, size);
	bcb->fd *= -1;
}

/*
** Name: shm_connect - complete an outgoing connection
*/

static VOID
shm_connect( bsp )
BS_PARMS	*bsp;
{
	PID	pid;

	GC_bsm[0]->bat_inuse = 1;
	GC_bsm[1]->bat_signal = 1;  /* set 'client' signal flag */
	bsp->status = OK;
	return;
}

/*
** Name: shm_send - send data down the stream
*/
 
static VOID
shm_send( bsp )
BS_PARMS *bsp;
{
    BCB	*bcb = (BCB *)bsp->bcb;
    i4             idx;
    char           *shmbuf;
 
    if( bcb->optim & BS_SOCKET_DEAD )
    {
	bsp->status = BS_WRITE_ERR;
	return;
    }

    idx = shm_getcliservidx();
    if (GC_bsm[0]->bat_inuse == 0 
        || PCis_alive(GC_bsm[!idx]->process_id) == FALSE)
    {
      iisock_error(bsp, BS_WRITE_ERR);
      bcb->optim |= BS_SOCKET_DEAD;
      return;
    }

    while (GC_bsm[!idx]->bat_dirty)
      /*II_nap(1)*/;
 
    shmbuf = (char *)GC_bsm[!idx] + sizeof(GC_BATCH_SHM) + 8;
    if (bsp->buf < shmbuf ||  bsp->buf > shmbuf + MAX_SHM_SIZE)
       MEcopy(bsp->buf, bsp->len, shmbuf);
    else
       GC_bsm[!idx]->bat_offset = bsp->buf - shmbuf;


    bcb->optim |= BS_SKIP_W_SELECT;
    bsp->buf   += bsp->len;
    bsp->status = OK;
    GC_bsm[!idx]->bat_sendlen = bsp->len;
    bsp->len    = 0;
    GC_bsm[!idx]->bat_dirty = 1;
    GC_bsm[!idx]->bat_signal = 1;
    while (GC_bsm[!idx]->bat_dirty && !GC_bsm[idx]->bat_dirty)
      /*II_nap(1)i */;
}


/*
** Name: shm_receive- receive data from a stream
*/
 
static VOID
shm_receive( bsp )
BS_PARMS *bsp;
{
   BCB		*bcb = (BCB *)bsp->bcb;
   i4           idx, sigoff=0;
   char         *shmbuf;
 
    if ( bcb->optim & BS_SOCKET_DEAD)
    {
	bsp->status = BS_READ_ERR;
	return;
    }	 

    idx = shm_getcliservidx();
      
    if (GC_bsm[idx]->bat_sendlen == 0 ||
        PCis_alive(GC_bsm[!idx]->process_id) == FALSE)
    {
       bsp->status = BS_READ_ERR;
       bcb->optim |= BS_SOCKET_DEAD;
       SETCLERR( bsp->syserr, 0, 0);
       return;
    }	
 
    shmbuf = (char *)GC_bsm[idx] + sizeof(GC_BATCH_SHM) + 8;
    if (bsp->buf < shmbuf ||  bsp->buf > shmbuf + MAX_SHM_SIZE)
    {
       MEcopy(shmbuf + GC_bsm[idx]->bat_offset, 
              GC_bsm[idx]->bat_sendlen, bsp->buf);
       sigoff =1 ;
    }
    GC_bsm[idx]->bat_offset = 0;

    bcb->optim  |= BS_SKIP_W_SELECT;
    bsp->buf    +=  GC_bsm[idx]->bat_sendlen;
    bsp->len    -=  GC_bsm[idx]->bat_sendlen;
    GC_bsm[idx]->bat_sendlen = 0;
    bsp->status  = OK;
    if (sigoff)
       GC_bsm[idx]->bat_dirty = 0;
       
}



i4   shm_getcliservidx()
{
    static PID  pid=0;

    if (pid ==0)
       PCpid (&pid);
    if ( GC_bsm[0]->process_id == pid )  /* are we Server? */
       return 0;
    return 1;
}

/*
** Name: shm_close- close a single connection
*/
static VOID
shm_close( bsp )
BS_PARMS *bsp;
{
   BCB		*bcb = (BCB *)bsp->bcb;
   PID		pid;
 
	 /* unregister file descriptors */
	  
    (void)iiCLfdreg( bcb->fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
    (void)iiCLfdreg( bcb->fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );

    if (close(bcb->fd* -1) < 0)
    {
	iisock_error( bsp, BS_CLOSE_ERR );
	return;
    }

    PCpid(&pid);

    if (GC_bsm[0] != NULL)
    {
      i4  idx;

     idx = shm_getcliservidx();
     if (GC_bsm[idx]->process_id == pid)
     {
         if ( idx )
             GC_bsm[idx]->process_id = 0;
         GC_bsm[idx]->bat_signal = 0;
         GC_bsm[idx]->bat_dirty = 0;
         GC_bsm[idx]->bat_offset = 0;
         GC_bsm[0]->bat_inuse = 0;
     }
    }
    bsp->status = OK;
}

/*
** Name: shm_regfd - register for blocking operation
*/
 
bool
shm_regfd( bsp )
BS_PARMS        *bsp;
{
        int     fd;
        int     op;
        BCB     *bcb = (BCB *)bsp->bcb;
        LBCB    *lbcb = (LBCB *)bsp->lbcb;
 
        switch( bsp->regop )
        {
        case BS_POLL_ACCEPT:
                fd = lbcb->fd; op = FD_READ; break;
 
        case BS_POLL_SEND:
                if( bcb->optim & BS_SKIP_W_SELECT )
                {
                        bcb->optim &= ~BS_SKIP_W_SELECT;
                        return FALSE;
                }
                /* fall through */
 
        case BS_POLL_CONNECT:
                fd = bcb->fd; op = FD_WRITE; break;
 
        case BS_POLL_RECEIVE:
                if( bcb->optim & BS_SKIP_R_SELECT )
                {
                        bcb->optim &= ~BS_SKIP_R_SELECT;
                        return FALSE;
                }
 
                fd = bcb->fd; op = FD_READ; break;
 
        default:
                return FALSE;
        }
 
        /* Pass control to CLpoll */
 
        iiCLfdreg( fd, op, bsp->func, bsp->closure, bsp->timeout );
 
        return TRUE;
}


/*
** Name: shm_ok - dummy function returning OK
*/
 
VOID
shm_ok( bsp )
BS_PARMS        *bsp;
{
        bsp->status = OK;
}


/*
** Exported driver specification.
*/

BS_DRIVER BS_shm = {
	sizeof( BCB ),
	sizeof( LBCB ),
	shm_listen,
	shm_unlisten,
	shm_accept,
	shm_request,
	shm_connect,
	shm_send,
	shm_receive,
        0,      /* no orderly release */
	shm_close,
	shm_regfd,
	shm_ok,	/* save not needed */
	shm_ok,	/* restore not needed */
	0,	/* remote port translation needed */
	0       /* extended information */
} ;

