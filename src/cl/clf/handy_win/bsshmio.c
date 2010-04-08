/*
** Copyright (c) 1988, 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<cs.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<pc.h>
# include	<lo.h>
# include	<nm.h>

# include	<systypes.h>
# include       <clsocket.h>

# include	<bsi.h>
# include	"bsshmio.h"

# include	<errno.h>



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
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	30-Nov-2004 (drivi01)
**	  Updated i4 to SIZE_TYPE to port change #473049 to windows.
*/
# ifdef MCT
GLOBALREF CS_SEMAPHORE	*ME_page_sem;
# endif /* MCT */

extern	VOID	sock_ok();
extern  STATUS  sock_ext_info();
i4  shm_getcliservidx();

#define MAX_SHM_SIZE 8192

#define ME_SHMEM_DIR "memory"

static GC_BATCH_SHM     bsm, *GC_bsm[2] = {NULL,NULL};


/*
** Support routines - not visible outside this file.
*/

VOID
IIshm_clear_dirty()
{
  i4  idx;

  if (GC_bsm[0])
  {
    idx = shm_getcliservidx();
    GC_bsm[idx]->bat_dirty = 0;
  }
}

#define SHM_FD_OPEN  0
#define SHM_FD_CLOSE 1

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
    SIZE_TYPE	alloc_pages;
    BCB		*bcb;
    i4		size;
    i4		flags;

    if ( bsp != NULL )
    {
	bcb = (BCB *)bsp->bcb;
	bsp->status = OK;

	if( !( len = STlength( bsp->buf ) ) ||
           (STbcompare( bsp->buf, 9, "/tmp/IIB.", 9, FALSE ) &&
            STbcompare( bsp->buf, 4, "IIB.", 4, FALSE ) ))
	{
	    /* if given no "IIB.*" name , use IIB.pid */

	    PCpid( &pid );

	    STprintf( bsm.bat_id, "IIB.%d", pid );
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
# ifdef MCT
	gen_Psem(ME_page_sem);
# endif /* MCT */
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
# ifdef MCT
	gen_Vsem(ME_page_sem);
# endif /* MCT */
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
	    i4 idx;
    	    idx = shm_getcliservidx();
	    return (PTR) (GC_bsm[!idx]);
	}
	return (PTR) GC_bsm[0];
	
}

/*
** BS entry points
*/


i4  shm_getcliservidx()
{
    static PID  pid=0;

    if (pid ==0)
       PCpid (&pid);
    if ( GC_bsm[0]->process_id == pid )  /* are we Server? */
       return 0;
    return 1;
}


