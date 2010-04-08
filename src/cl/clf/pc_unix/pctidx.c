/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <gl.h>
# include       <clconfig.h>
# include       <me.h>
# include       <pc.h>
#if !defined(xCL_094_TLS_EXISTS)
# include       <meprivate.h>
#endif

/*
** Name: PCtidx
**
** Description:
**	Returns a well-behaved thread ID: thread ID will be in
**	range 1::max-active-thread-count.
**
**	Returns 0 if an error occurs.
**
**	Native TLS storage is used to store a well-behaved thread
**	ID for each thread.  When a thread exits (as driven by the
**	TLS destructor callback), the assigned TID is saved on a 
**	free list.  The first time a thread requests its TID, any 
**	free TID is assigned.  If there are no free TIDs, a new 
**	TID is allocated and assigned.
**
**  History:
**	09-Jan-2007 (gordy & rajus01)
**	    Created. 
**	20-Feb-2007 (rajus01) Bug 117714, SD 113461
**	    Added check for AIX systems.
**	21-Feb-2007 (rajus01) Bug 117714
**	    Fixed typo in AIX release id.
**	05-Jul-2007 (lunbr01) Bug 118662
**	    Free all TLS storage allocated by internal Ingres CL TLS
**	    routines (ME_tls*) for those platforms not using OS
**	    TLS routines (ie, xCL_094_TLS_EXISTS) not defined. Eg, HP-UX.
**	    Fixes memory leak for multi-threaded applications; DBMS server
**	    already has similar code in CSMT_del_thread().
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# if defined(OS_THREADS_USED)

# include   <systypes.h>
# include   <cs.h>

static  CS_SYNCH        pc_tidx_mutex;

typedef struct _pc_tid_map PC_TID_MAP;

struct _pc_tid_map
{
    PC_TID_MAP	*next;
    i4	tid;
};

static bool		initialized = FALSE;
static PC_TID_MAP	*tid_maps = NULL;
static u_i4		next_tid = 1;
#if defined(POSIX_THREADS)
static pthread_key_t	tls_key;
#else
/* SYS_V_THREADS */
static thread_key_t	tls_key;
#endif


static void		thread_exit_callback( void *arg );

u_i4
PCtidx(void)
{
/* Use PCtid() for platforms other than Solaris & HPUX */
#if !defined(sparc_sol) && !defined(a64_sol) && !defined(any_hpux) && !defined(any_aix)
    return( PCtid() );
#else
    PC_TID_MAP *tid_map = NULL;

    if ( ! initialized )
    {
	/* Warning!!: There is a threading issue having synch_init here.. .
	** If a thread comes in, inits and locks,
	** and blocks before setting initialized then another thread can enter,
	** init and fail to see the original lock. It is a common race 
	** condition caused by needing an init in a function which doesn't 
	** have a save init point. The rest of the CL does pretty much the
	** same way.. Decided to leave it as such for now...
	*/
	CS_synch_init( &pc_tidx_mutex );
	CS_synch_lock( &pc_tidx_mutex );
	if ( ! initialized  &&
#if defined(POSIX_THREADS)
	     pthread_key_create( &tls_key, thread_exit_callback ) == 0
#else
	    /* SYS_V_THREADS */
	     thr_keycreate( &tls_key, thread_exit_callback ) == 0
#endif
           )

	    initialized = TRUE;
	CS_synch_unlock( &pc_tidx_mutex );

	if ( ! initialized )  return( 0 );
    }

    /*
    ** See if thread has a TID assigned.
    */
#if defined(POSIX_THREADS)
    if ( (tid_map = (PC_TID_MAP *)pthread_getspecific( tls_key )) == NULL )
#else
    /* SYS_V_THREADS */
    thr_getspecific( tls_key, (void **)&tid_map );
    if ( tid_map == NULL )
#endif
    {
	/*
	** See if there is a free TID.
	*/
	CS_synch_lock( &pc_tidx_mutex );
	if ( tid_maps != NULL )
	{
	    /*
	    ** Use free TID.
	    */
	    tid_map = tid_maps;
	    tid_maps = tid_maps->next;
	    tid_map->next = NULL;
	}
	CS_synch_unlock( &pc_tidx_mutex );

	if ( tid_map == NULL )
	{
	    /*
	    ** Create new TID.
	    */
	    tid_map = (PC_TID_MAP *)MEreqmem(0, sizeof(PC_TID_MAP), TRUE, NULL);

	    if ( tid_map == NULL )  return( 0 );
	    tid_map->next = NULL;

	    CS_synch_lock( &pc_tidx_mutex );
	    tid_map->tid = next_tid++;
	    CS_synch_unlock( &pc_tidx_mutex );
	}

#if defined(POSIX_THREADS)
    	if ( pthread_setspecific( tls_key, tid_map ) )
#else
        /* SYS_V_THREADS */
    	if ( thr_setspecific( tls_key, tid_map ) )
#endif
	{
	    /* An error occured. Add the memory to free tid_maps */
	    CS_synch_lock( &pc_tidx_mutex );
	    tid_map->next = tid_maps;
	    tid_maps = tid_map;
	    CS_synch_unlock( &pc_tidx_mutex );
	    return( 0 );
	}
    }

    return( tid_map->tid );
#endif
}

static void
thread_exit_callback( void *arg )
{
    STATUS	status;
    /*
    ** Threads TID map is saved as a free TID.
    */
    PC_TID_MAP *tid_map = (PC_TID_MAP *)arg;
    CS_synch_lock( &pc_tidx_mutex );
    tid_map->next = tid_maps;
    tid_maps = tid_map;
    CS_synch_unlock( &pc_tidx_mutex );
#if !defined(xCL_094_TLS_EXISTS)
    ME_tls_destroyall( &status );
#endif
    return;
}
#endif /* OS_THREADS_USED */
