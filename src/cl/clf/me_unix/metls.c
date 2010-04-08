/*
** Copyright (c) 1997, 2003 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>

/* 
** These "thread-local storage" (or "thread-specific data") 
** functions are replacements for similar functions provided
** by the operating system.  The operating-system library functions
** are usually much more efficient.  If, for some reason, they 
** cannot be used, these replacements can be used.
**	07-Apr-2000 (jenjo02)
**	    Optimized code to use dirty reads instead of 
**	    mutexed reads, added volatile version to each
**	    hash list, ensure that "tid" is correctly cast
**	    as CS_THREAD_ID, never CS_SID.
*/
#if defined(OS_THREADS_USED) && !defined(xCL_094_TLS_EXISTS)
# include   <systypes.h>
# include   <cs.h>
# include   <meprivate.h>
# include   <qu.h>

/*
** Thread-local storage structures
*/
typedef struct _ME_TLS_HASH     ME_TLS_HASH;

struct _ME_TLS_HASH
{
    QUEUE               q;              /* queue the entries together */
    volatile u_i4	version;	/* hash queue update tick */
};

typedef struct _ME_TLS_LIST     ME_TLS_LIST;

struct _ME_TLS_LIST
{
    QUEUE               q;              /* queue the entries together */
    CS_THREAD_ID        tid;            /* thread id */
    ME_TLS_KEY		key;		/* key value */
    PTR                 value;          /* the allocated storage */
};

# define ME_TLS_BUCKETS	2048

/* ME_tls_keyq -- hash table to store the data */
static ME_TLS_HASH	ME_tls_keyq[ME_TLS_BUCKETS] ZERO_FILL;

static	i4		ME_tls_initialized = 0;
static	CS_SYNCH	ME_tls_mutex;

#ifdef xDEBUG
# define SYNCH_LOCK(mutex)						\
  {                                                                     \
    CS_SCB	*scb;                                                   \
    CS_SEMAPHORE sem;                                                   \
    i4		saved_state;                                            \
									\
    CSget_scb(&scb);                                                    \
    if (scb)                                                            \
    {                                                                   \
    	/*                                                              \
    	** If mutex is blocked, we must wait.                           \
    	*/                                                              \
    	MEcopy("ME_tls_mutex", sizeof("ME_tls_mutex"), sem.cs_sem_name);\
    	scb->cs_sync_obj = (PTR)&sem;                                   \
    	saved_state = scb->cs_state;                                    \
    	scb->cs_state = CS_MUTEX;                                       \
    }                                                                   \
									\
    CS_synch_lock( &mutex );                                            \
									\
    if (scb)                                                            \
    {                                                                   \
    	scb->cs_state = saved_state;                                    \
    }                                                                   \
  }
#else
# define SYNCH_LOCK(mutex)						\
    CS_synch_lock( &mutex );
#endif

/**
**  Name: METLS.C - Thread-local storage routines
**
**  Description:
**	This module contains the code which implements 
**	thread-local storage.
**
**
**  History:
**	03-may-1996 (canor01)
**	    Created to provide thread-local storage services.
**	    The library functions that come with Solaris do not
**	    work with the Ingres memory allocator.
**	22-nov-1996 (canor01)
**	    Changed most return values from STATUS to VOID to
**	    ease replacement by macros on platforms that have native
**	    support for TLS.  Changed names of functions from "MEtls..."
**	    to "ME_tls..." to avoid conflict with new functions in GL.
**	18-dec-1996 (canor01)
**	    Simplify ME_tls_createkey() by allocating next key from
**	    a static array.  On thread shutdown, only destroy keys that
**	    have been allocated.
**	15-jan-1997 (wonst02)
**	    Add xDEBUG code to treat the CS_synch_lock as an INGRES semaphore
**	    so it can be seen by the sampler in iimonitor.
**	26-feb-1997 (canor01)
**	    Include only if cannot use system TLS functions.
**      03-Nov-1997 (hanch04)
**          Pass address to QUremove
**	07-Apr-2000 (jenjo02)
**	    Optimized code to use dirty reads instead of 
**	    mutexed reads, added volatile version to each
**	    hash list, ensure that "tid" is correctly cast
**	    as CS_THREAD_ID, never CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-mar-2003 (somsa01)
**	    Removed usage of obsolete ME_tls_keytab, corrected
**	    ME_tls_destroyall().
*/

static
VOID
ME_tls_init()
{
    register i4  	i;

    CS_synch_init( &ME_tls_mutex );
    ME_tls_initialized = 1;
    CS_synch_lock( &ME_tls_mutex );
    for ( i = 0; i < ME_TLS_BUCKETS; i++ )
    {
	QUinit( &ME_tls_keyq[i].q );
	ME_tls_keyq[i].version = 0;
    }
    CS_synch_unlock( &ME_tls_mutex );
}

static
STATUS
ME_tls_search_list( 
CS_THREAD_ID tid, 
ME_TLS_KEY key, 
ME_TLS_LIST **listp,
ME_TLS_HASH **hashp)
{
    register ME_TLS_LIST *tls_list_entry;
    i4		bucket;
    u_i4	version;

    bucket = ( key + (tid << 4) ) % ME_TLS_BUCKETS;

    *hashp = &ME_tls_keyq[bucket];

    do
    {
	tls_list_entry = (ME_TLS_LIST *)(*hashp)->q.q_next;
	version = (*hashp)->version;

	while ( tls_list_entry != (ME_TLS_LIST *)*hashp )
	{
	    if ( CS_thread_id_equal(tls_list_entry->tid, tid) &&
		 tls_list_entry->key == key )
	    {
		*listp = tls_list_entry;
		return(OK);
	    }
	    /* If the list has changed, restart */
	    if ( version == (*hashp)->version )
		tls_list_entry = (ME_TLS_LIST *)tls_list_entry->q.q_next;
	    else
		break;
	}
    } while ( version != (*hashp)->version );

    *listp = (ME_TLS_LIST *)*hashp;
    return(FAIL);
}
/*{
** Name: 	ME_tls_createkey		- create a TLS key
**
** Description:
**	Get a unique key to reference a slot for this thread
**	only.
**
** Inputs:
**	None
**
** Outputs:
**	key - A unique key that references a slot for this thread only.
**
**	Returns:
**	    OK	- success
**	    !OK - failure 
**	
**  History:
**	22-nov-1996 (canor01)
**	    Change return value from STATUS to VOID to make it easier
**	    to replace with a macro on platforms with TLS support.
**	    Change name to avoid conflicts with GL ME functions.
**	18-dec-1996 (canor01)
**	    Simplify ME_tls_createkey() by allocating next key from
**	    a static array.
*/
VOID
ME_tls_createkey( ME_TLS_KEY *key, STATUS *status )
{
    if ( !ME_tls_initialized )
	ME_tls_init();

    SYNCH_LOCK( ME_tls_mutex );

    *key = ME_tls_initialized++;

    CS_synch_unlock( &ME_tls_mutex );

    *status = OK;

    return;
}
/*{
** Name:        ME_tls_set         - Assign a value to tls memory
**
** Description:
**      Assign a pointer value to the unique key id for this thread.
**
** Inputs:
**      key - unique key that references a slot for this thread only.
**	value - the value to store in the slot
**
** Outputs:
**
**      Returns:
**          OK  - success
**          !OK - failure 
**
**  History:
**	22-nov-1996 (canor01)
**	    Change return value from STATUS to VOID to make it easier
**	    to replace with a macro on platforms with TLS support.
**	    Change name to avoid conflicts with GL ME functions.
*/
VOID
ME_tls_set( ME_TLS_KEY key, PTR value, STATUS *status )
{
    ME_TLS_LIST 	*tls_list_entry;
    ME_TLS_LIST		*tls_new_entry;
    ME_TLS_HASH		*hash;

    *status = OK;

    if ( ME_tls_search_list( CS_get_thread_id(), key, &tls_list_entry, &hash ) == OK )
    {
	tls_list_entry->value = value;
    }
    else
    {
	if ( tls_new_entry = (ME_TLS_LIST *)MEreqmem(0, sizeof(ME_TLS_LIST), 
						TRUE, NULL) )
	{
	    SYNCH_LOCK( ME_tls_mutex );
	    hash->version++;
	    QUinsert( &tls_new_entry->q, &tls_list_entry->q );
	    tls_new_entry->tid = CS_get_thread_id();
	    tls_new_entry->value = value;
	    tls_new_entry->key = key;
	    CS_synch_unlock( &ME_tls_mutex );
	}
	else
	    *status = FAIL;
    }

    return;
}
/*{
** Name:        ME_tls_get         - Retrieve a value from tls memory
**
** Description:
**      Retrieve a pointer value for the unique key id for this thread.
**
** Inputs:
**      key - unique key that references a slot for this thread only.
**
** Outputs:
**	value - the stored value
**
**      Returns:
**          OK  - success
**          !OK - failure
**
**  History:
**	22-nov-1996 (canor01)
**	    Change return value from STATUS to VOID to make it easier
**	    to replace with a macro on platforms with TLS support.
**	    Change name to avoid conflicts with GL ME functions.
**      31-Jul-2001 (hanal04) Bug 105265 INGSRV 1496
**          Do no reference the tls_list_entry found in ME_tls_search_list()
**          without holding the ME_tls_mutex as ME_tls_destroy could be
**          releasing the memory once we it has removed the entry from the
**          linked list.
*/
VOID
ME_tls_get( ME_TLS_KEY key, PTR *value, STATUS *status )
{
    ME_TLS_LIST 	*tls_list_entry;
    ME_TLS_HASH		*hash;

    SYNCH_LOCK( ME_tls_mutex );

    if ( ME_tls_search_list( CS_get_thread_id(), key, &tls_list_entry, &hash ) == OK )
    {
	*value = tls_list_entry->value;
        CS_synch_unlock( &ME_tls_mutex );
	*status = OK;
    }
    else
    {
        CS_synch_unlock( &ME_tls_mutex );
	*value = NULL;
	*status = FAIL;
    }

    return;
}
/*{
** Name:        ME_tls_destroy         - Destroy slots associated with a key
**
** Description:
**      Destroy the thread-local storage associated with a 
**	particular key for this thread only.
**
** Inputs:
**      key - unique key that references a slot for this thread only.
**      destructor - pointer to a function to de-allocate the memory.
**
** Outputs:
**
**      Returns:
**          OK  - success
**          !OK - failure
**
**  History:
**	22-nov-1996 (canor01)
**	    Change return value from STATUS to VOID to make it easier
**	    to replace with a macro on platforms with TLS support.
**	    Change name to avoid conflicts with GL ME functions.
**	    Remove destructor parameter; always use MEfree.
*/
VOID
ME_tls_destroy( ME_TLS_KEY key, STATUS *status )
{
    ME_TLS_LIST 	*tls_list_entry;
    ME_TLS_HASH		*hash;

    if ( ME_tls_search_list( CS_get_thread_id(), key, &tls_list_entry, &hash ) == OK )
    {
	SYNCH_LOCK( ME_tls_mutex );
	hash->version++;
	QUremove( &tls_list_entry->q );
	CS_synch_unlock( &ME_tls_mutex );
	MEfree( tls_list_entry->value );
	MEfree( (char *)tls_list_entry );
	*status = OK;
    }
    else
	*status = FAIL;

    return;
}
/*{
** Name:        ME_tls_destroyall     - Destroy a thread's local storage
**
** Description:
**      Destroy the thread-local storage associated with a
**      all keys for a particular thread.
**
** Inputs:
**      destructor - pointer to a function to de-allocate the memory.
**
** Outputs:
**
**      Returns:
**          OK  - success
**          !OK - failure
**
**  History:
**	22-nov-1996 (canor01)
**	    Change return value from STATUS to VOID to make it easier
**	    to replace with a macro on platforms with TLS support.
**	    Change name to avoid conflicts with GL ME functions.
**	    Remove destructor parameter; always use MEfree.
**	18-dec-1996 (canor01)
**	    Only destroy keys that have been allocated.
**	19-mar-2003 (somsa01)
**	    Removed usage of obsolete ME_tls_keytab.
*/
VOID
ME_tls_destroyall( STATUS *status )
{
    ME_TLS_KEY	key;
    STATUS	ret;

    *status = OK;

    /* keys begin at 1 */
    for ( key = 1; key < ME_tls_initialized; key++ )
    {
	ME_tls_destroy( key, &ret );
	if ( ret != OK )
	    *status = ret;
    }

    return;
}

#endif	/* OS_THREADS_USED && !xCL_094_TLS_EXISTS */
