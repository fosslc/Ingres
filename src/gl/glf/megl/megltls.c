/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<mu.h>
# include	<qu.h>
# include	<pc.h>


/**
** Name: megltls.c
**
** Description:
**	This file defines the thread-local-storage memory routines:
**
**	MEtls_create		Create a thread-local-storage object.
**	MEtls_destroy		Destroy a thread-local-storage object.
**	MEtls_get		Get a thread's local storage for an object.
**	MEtls_set		Set a thread's local storage for an object.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
**	31-jan-1997 (wilan06)
**	    Cast tid to unsigned. Win95's thread IDs are negative 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Jan-2007 (rajus01)
**        Replaced PCtid() with PCtidx().
**/

# define	METLS_BLOCK		16	/* Alloc block size */
# define	METLS_BUCKETS		32	/* Number of hash buckets */

/*
** The following hash function assumes thread IDs
** are small contiguous integers.  If there is a
** system for which this produces clustering, it
** can easily be changed.
*/
# define	METLS_HASH( tid )	(((u_i4)tid) % METLS_BUCKETS)


/*
** Name: METLS_VALUE
**
** Description:
**	Holds the thread-local-storage value for each thread.
*/

typedef struct _METLS_VALUE
{

    QUEUE	tls_q;
    i4		tls_flags;

# define METLS_DYNAMIC		0x01		/* Dynamically allocated */

    i4	tls_tid;			/* Thread ID */
    PTR		tls_value;			/* Thread's local storage */

} METLS_VALUE;


/*
** Name: METLS_CB
**
** Description:
**	Thread-local-storage control block represented
**	by a thread-local-storage object key.
*/

typedef struct _METLS_CB
{

    MU_SEMAPHORE	tls_semaphore;

# define	METLS_SEMAPHORE_NAME	"METLS"

    u_i4		tls_mode;			/* Threading mode */

# define METLS_SINGLE		0x01
# define METLS_MULTI		0x02

    METLS_VALUE		tls_default;			/* Single-threaded */
    QUEUE		tls_buckets[ METLS_BUCKETS ];	/* Multi-threaded */
    QUEUE		tls_free;			/* Unused entries */

} METLS_CB;


/*
** Forward references.
*/

static VOID		tls_convert( METLS_CB *tls_cb );
static STATUS		tls_append( METLS_CB *tls_cb, i4 tid, PTR value );
static METLS_VALUE *	tls_search( METLS_CB *tls_cb, i4 tid );



/*{
** Name: MEtls_create
**
** Description:
**	Creates a thread-local-storage object key to manage the
**	per-thread storage for an object which has separate memory
**	images for each thread.
**
**	Per-thread storage for a thread-local-storage object is
**	provided by each thread using whatever memory manager is
**	best suited to the thread/object usage.  The thread-local-
**	storage object key acts as a global resource allowing each 
**	thread to save/retrieve its storage indepedent of other
**	threads which also use the object.
**
** Inputs:
**	None.
**
** Outputs:
**	tls_key		Thread-local-storage object key.
**
** Returns:
**	STATUS		OK if successful, ME error code if allocation fails.
**
** Re-entrancy:
**	Individual keys must be protected as global resources.
**
** Exceptions:
**	None.
**
** Side Effects:
**	Memory is allocated for each key.  Any previous value for key is lost
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
*/

STATUS
MEtls_create( METLS_KEY *tls_key )
{
    STATUS	status = OK;

    if ( ! tls_key )  return( FAIL );

    *tls_key = MEreqmem( 0, sizeof( METLS_CB ), TRUE, &status );

    if ( *tls_key )
    {
	METLS_CB	*tls_cb = (METLS_CB *)*tls_key;

	MUi_semaphore( &tls_cb->tls_semaphore );
	MUn_semaphore( &tls_cb->tls_semaphore, METLS_SEMAPHORE_NAME );

	/*
	** Start in single threaded mode.  Keys are
	** global resources and must be protected by 
	** the caller during creation/destruction, so 
	** we do not lock the control block semaphore 
	** here.
	*/
	tls_cb->tls_mode = METLS_SINGLE;
	tls_cb->tls_default.tls_tid = PCtidx();
    }

    return( status );
}



/*{
** Name: MEtls_destroy
**
** Description:
**	Free the resources associated with a thread-local-storage
**	object key.  
**
**	The per-thread storage managed by the thread-local-storage 
**	object key can be freed by providing a destructor function
**	(with calling sequence identical to MEfree()).  The function
**	will be called for each threads storage value managed by the 
**	key.  If no destructor function is provided, the client must 
**	ensure that thread local storage is disposed of correctly.
**
** Inputs:
**	tls_key		Thread-local-storage object key.
**	destructor	Function to free thread local storage, may be NULL.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK if successful, ME error code if fails.
**
** Re-entrancy:
**	Individual keys must be protected as global resources.
**
** Exceptions:
**	None.
**
** Side Effects:
**	Dynamically allocated memory is freed.  If a destructor function
**	is provided, memory allocated by threads and associated with the
**	key will be freed by calling the destructor function.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
**	14-May-2009 (kschendel) b122041
**	    Change destructor parm to void * to match MEfree.
*/

STATUS
MEtls_destroy( METLS_KEY *tls_key, STATUS (*destructor)( void * ) )
{
    STATUS	status = OK;

    if ( ! tls_key )  return( FAIL );

    if ( *tls_key )
    {
	METLS_CB	*tls_cb = (METLS_CB *)*tls_key;
	METLS_VALUE	*entry;
	QUEUE		*next;
	i4		bucket;
	STATUS		r_stat;

	/*
	** Free resources associated with key.  Keys are
	** global resources and must be protected by the
	** caller during creation/destruction, so we do
	** not lock the control block semaphore here.
	*/
	switch( tls_cb->tls_mode )
	{
	    case METLS_SINGLE :
		if ( destructor  &&  tls_cb->tls_default.tls_value )
		    status = (*destructor)( tls_cb->tls_default.tls_value );
		break;
	    
	    case METLS_MULTI :
		/*
		** Remove non-dynamic entries from the free queue,
		** which we want to use to hold dynamic entries.
		** Non-dynamic entries are from blocks headed by
		** dynamic entries (or the single-threaded default
		** value) and may be discarded.
		*/
		for( 
		     entry = (METLS_VALUE *)tls_cb->tls_free.q_next;
		     (QUEUE *)entry != &tls_cb->tls_free;
		     entry = (METLS_VALUE *)next
		   )
		{
		    next = entry->tls_q.q_next;

		    if ( ! (entry->tls_flags & METLS_DYNAMIC) )
			QUremove( &entry->tls_q );
		}

		/*
		** Scan all the value queues, freeing any remaining
		** thread storage (if requested) and moving dynamically
		** allocated entries to the free queue.
		*/
		for( bucket = 0; bucket < METLS_BUCKETS; bucket++ )
		    while( (entry = (METLS_VALUE *)
			    QUremove( tls_cb->tls_buckets[ bucket ].q_next )) )
		    {
			if ( destructor  &&  entry->tls_value )
			{
			    r_stat = (*destructor)( entry->tls_value );
			    if ( status == OK )  status = r_stat;
			}

			if ( entry->tls_flags & METLS_DYNAMIC )
			    QUinsert( &entry->tls_q, &tls_cb->tls_free );
		    }

		/*
		** The free queue now contains just the dynamically
		** allocated entries.  Free them.
		*/
		while( (entry = 
			(METLS_VALUE *)QUremove( tls_cb->tls_free.q_next )) )
		{
		    r_stat = MEfree( (PTR)entry );
		    if ( status == OK )  status = r_stat;
		}
		break;
	}

	MUr_semaphore( &tls_cb->tls_semaphore );

	/*
	** Free the thread-local-storage control block.
	*/
	r_stat = MEfree( *tls_key );
	if ( status == OK )  status = r_stat;
	*tls_key = NULL;
    }

    return( status );
}



/*{
** Name: MEtls_get
**
** Description:
**	Returns the per-thread storage value maintained by a
**	thread-local-storage object key for the current thread
**	as set by MEtls_set().
**
**	Returns NULL if MEtls_set() has not been called by the
**	current thread for a key.  This feature provides a
**	mechanism to determining if per-thread storage needs 
**	to be allocated.
**	
** Inputs:
**	tls_key		Thread-local-storage object key.
**
** Outputs:
**	tls_value	Thread's storage associated with key, may be NULL.
**
** Returns:
**	STATUS		OK if successful, ME error code if allocation fails.
**
** Re-entrancy:
**	Re-entrant.
**
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
*/

STATUS
MEtls_get( METLS_KEY *tls_key, PTR *tls_value )
{
    METLS_CB	*tls_cb;
    METLS_VALUE	*entry;
    i4	tid = PCtidx();
    STATUS	status = OK;

    if ( ! tls_key  ||  ! *tls_key )  return( FAIL );

    tls_cb = (METLS_CB *)*tls_key;
    *tls_value = NULL;

    switch( tls_cb->tls_mode )
    {
	case METLS_SINGLE :
	    /*
	    ** Check if current thread same as default.
	    ** If not, leave NULL since hasn't been set.
	    **
	    ** There is no need to lock the control block
	    ** in single-threaded mode.  If conversion to 
	    ** multi-threaded mode is in progress, this is 
	    ** either the default thread or a thread which 
	    ** has not yet set its value (we would alread 
	    ** be in multi-threaded mode otherwise) and 
	    ** single-threaded processing will produce the 
	    ** correct results without conflicting with 
	    ** other thread activity.
	    */
	    if ( tid == tls_cb->tls_default.tls_tid )  
		*tls_value = tls_cb->tls_default.tls_value;
	    break;

	case METLS_MULTI :
	    if ( (status = MUp_semaphore( &tls_cb->tls_semaphore )) != OK )
		break;

	    /*
	    ** See if value has been set.  If not, leave NULL.
	    */
	    if ( (entry = tls_search( tls_cb, tid )) )
		*tls_value = entry->tls_value;

	    MUv_semaphore( &tls_cb->tls_semaphore );
	    break;

	default :
	    status = FAIL;
	    break;
    }

    return( status );
}



/*{
** Name: MEtls_set
**
** Description:
**	Sets the per-thread storage value maintained by a
**	thread-local-storage object key for the current
**	thread.
**
**	The per-thread storage may be allocated by whatever
**	means or memory manager best suits the thread and/or
**	key.
**
** Inputs:
**	tls_key		Thread-local-storage object key.
**	tls_value	Threads storage associated with key, may be NULL.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK if successful, ME error code if fails.
**
** Re-entrancy:
**	Re-entrant.
**
** Exceptions:
**	None.
**
** Side Effects:
**	Memory may be dynamically allocated.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
*/

STATUS
MEtls_set( METLS_KEY *tls_key, PTR tls_value )
{
    METLS_CB	*tls_cb;
    METLS_VALUE	*entry;
    i4	tid = PCtidx();
    STATUS	status = OK;

    if ( ! tls_key  ||  ! *tls_key )  return( FAIL );

    tls_cb = (METLS_CB *)*tls_key;

    switch( tls_cb->tls_mode )
    {
	case METLS_SINGLE :
	    /*
	    ** If this is the default thread, just need to 
	    ** save the value.  There is no need to lock the 
	    ** control block in single-threaded mode.
	    */
	    if ( tid == tls_cb->tls_default.tls_tid )  
	    {
		tls_cb->tls_default.tls_value = tls_value;
		break;
	    }

	    /*
	    ** If the value is NULL then we don't need to 
	    ** do anything, NULL will be returned for all
	    ** but the default thread in single threaded
	    ** mode.  Otherwise, need to convert to multi-
	    ** threaded mode.
	    */
	    if ( ! tls_value )  break;

	    /*
	    ** Convert to multi-threaded mode and add this 
	    ** thread's value.  The control block must be
	    ** locked during conversion and while in multi-
	    ** threaded mode.
	    */
	    if ( (status = MUp_semaphore( &tls_cb->tls_semaphore )) != OK )
		break;

	    /*
	    ** Now that the control block is locked, recheck 
	    ** the threading mode to avoid race conditions 
	    ** with multiple threads attempting conversion 
	    ** simultaneously.
	    */
	    if ( tls_cb->tls_mode == METLS_SINGLE )  tls_convert( tls_cb );

	    /*
	    ** We don't need to search the value queues
	    ** since this thread's value can't have been
	    ** set previously.
	    */
	    status = tls_append( tls_cb, tid, tls_value );

	    MUv_semaphore( &tls_cb->tls_semaphore );
	    break;

	case METLS_MULTI :
	    if ( (status = MUp_semaphore( &tls_cb->tls_semaphore )) != OK )
		break;

	    /*
	    ** Check to see if the value has already
	    ** been set and add if necessary.
	    */
	    if ( (entry = tls_search( tls_cb, tid )) )
		entry->tls_value = tls_value;
	    else
		status = tls_append( tls_cb, tid, tls_value );

	    MUv_semaphore( &tls_cb->tls_semaphore );
	    break;

	default :
	    status = FAIL;
	    break;
    }

    return( status );
}


/*
** Name: tls_convert
**
** Description:
**	Convert a thread-local-storage control block from
**	single-threaded mode to multi-threaded mode.
**
** Input:
**	tls_cb		Thread-local-storage control block.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** Re-entrancy:
**	Caller must lock control block semaphore.
**
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
*/

static VOID
tls_convert( METLS_CB *tls_cb )
{
    i4  bucket;

    /*
    ** Initialize queues used for multi-threaded mode.
    */
    QUinit( &tls_cb->tls_free );

    for( bucket = 0; bucket < METLS_BUCKETS; bucket++ )
	QUinit( &tls_cb->tls_buckets[ bucket ] );
    
    /*
    ** Add the default thread's entry into the value queues.
    */
    bucket = METLS_HASH( tls_cb->tls_default.tls_tid );
    QUinsert( &tls_cb->tls_default.tls_q, &tls_cb->tls_buckets[ bucket ] );

    tls_cb->tls_mode = METLS_MULTI;

    return;
}


/*
** Name: tls_append
**
** Description:
**	Add a new value entry to a thread-local-storage control block.
**
** Input:
**	tls_cb		Thread-local-storage control block.
**	tid		Thread ID.
**	tls_value	Thread's local-storage value.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK if successful, ME error code if fails.
**
** Re-entrancy:
**	Caller must lock control block semaphore.
**
** Exceptions:
**	None.
**
** Side Effects:
**	Memory may be dynamically allocated.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
*/

static STATUS
tls_append( METLS_CB *tls_cb, i4 tid, PTR tls_value )
{
    METLS_VALUE *entry;
    i4		bucket, count;
    STATUS	status;

    /*
    ** Make sure there is an unused entry available.
    */
    if ( tls_cb->tls_free.q_next == &tls_cb->tls_free )
    {
	if ( ! (entry = (METLS_VALUE *)
		MEreqmem(0, sizeof(METLS_VALUE) * METLS_BLOCK, TRUE, &status)) )
	    return( status );

	/*
	** The first entry in the block is flagged as
	** dynamic since its address is the only one
	** valid for passing to MEfree().
	*/
	entry->tls_flags = METLS_DYNAMIC;

	/*
	** Save all the entries on the free list.
	*/
	for( count = METLS_BLOCK; count; count--, entry++ )
	    QUinsert( &entry->tls_q, &tls_cb->tls_free );
    }

    /*
    ** Initialize an unused entry and move it to the value queues.
    */
    entry = (METLS_VALUE *)QUremove( tls_cb->tls_free.q_next );
    entry->tls_tid = tid;
    entry->tls_value = tls_value;

    bucket = METLS_HASH( tid );
    QUinsert( &entry->tls_q, &tls_cb->tls_buckets[ bucket ] );

    return( OK );
}


/*
** Name: tls_search
**
** Description:
**	Searches the value queues of a thread-local-storage 
**	control block for an entry for a thread.
**
** Input:
**	tls_cb		Thread-local-storage control block.
**	tid		Thread ID.
**
** Output:
**	None.
**
** Returns:
**	METLS_VALUE *	Thread's value entry, NULL if not found.
**
** Re-entrancy:
**	Caller must lock control block semaphore.
**
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**	11-Nov-96 (gordy)
**	    Created.
*/

static METLS_VALUE *
tls_search( METLS_CB *tls_cb, i4 tid )
{
    METLS_VALUE	*value;
    i4		bucket = METLS_HASH( tid );

    for( 
	 value = (METLS_VALUE *)tls_cb->tls_buckets[ bucket ].q_next;
	 (QUEUE *)value != &tls_cb->tls_buckets[ bucket ];
	 value = (METLS_VALUE *)value->tls_q.q_next
       )
	if ( value->tls_tid == tid )  return( value );

    return( NULL );
}

