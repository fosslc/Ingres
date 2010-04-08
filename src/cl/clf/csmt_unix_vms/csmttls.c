/*
**Copyright (c) 2004 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
/*
** These are fast-access extensions to the thread-local storage
** functions that exist in ME (metls.c).  They are required only
** on platforms that need metls.c.  See that file for details.
*/
#if defined(OS_THREADS_USED) && !defined(xCL_094_TLS_EXISTS)
# include   <systypes.h>
# include   <pc.h>
# include   <cs.h>
# include   <me.h>
# include   <csinternal.h>
# include   "csmtlocal.h"

/*
** Thread-local storage structures
*/
typedef struct _CS_TLS_LIST     CS_TLS_LIST;

struct _CS_TLS_LIST
{
    PTR                 value;          /* the allocated storage */
    CS_THREAD_ID        tid;            /* thread id */
    u_i4		reads;		/* number of reads */
    u_i4		dirty;		/* number of dirty reads */
    u_i4		writes;		/* number of writes */
};

/* 
** CS_tls_keyq -- hash table to store SCB ptrs (allocated in csinterface.c) 
** (Initially contains a single entry for the admin thread's SCB.)
*/

static CS_TLS_LIST	CS_tls_admin ZERO_FILL;
static CS_TLS_LIST	*CS_tls_keyq = &CS_tls_admin;
static u_i4		CS_tls_buckets = 1;

static volatile u_i4 	CS_tls_version ZERO_FILL;

# define CS_TLS_HASH(tid) ((tid << 4) % CS_tls_buckets)

static	CS_SYNCH	CS_tls_mutex;

/*
**  Name: CSTLS.C - Thread-local storage routines for CS scb only
**
**  Description:
**	This module contains the code which implements thread-local storage 
**	for the cs SCB. It is designed to be fast by doing "dirty" reads on
**	the hash bucket array to find a match, instead of having to obtain a
**	mutex. The mutex is used only to protect against multiple, simultaneous
**	updates.
**
**  History:
**	17-jan-1997 (wonst02)
**	    Created by cloning from MEtls.c.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmttls.c and moved all calls to CSMT...
**      10-apr-1997 (canor01)
**          Include only if cannot use system TLS functions.
**	04-Apr-2000 (jenjo02)
**	    CSMT_tls_measure() use thread id, not sid, as argument.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-mar-2001 (toumi01)
**	    add #include me.h to pick up definition of IIMEreqmem
*/

/*{
** Name: CSMT_tls_search_list - Search for an SCB entry in the hash table
**
** Description:
**      Search for the slot for the SCB for this thread. The synch lock 
**	does NOT need to be held by the caller before calling this routine for
**	a read-only look but should be held before calling to find an empty
**	slot to update.
**
**	The algorithm relies on looking at an update counter before and after
**	the search and comparing. If not equal (a "dirty" read), simply try
**	again.
**	
**	Why does this algorithm work? Yes, there is theoretically a chance that
**	an updater updates the counter just before a reader looks at it, and 
**	the reader looks at the slot that the updater is updating (on another
**	CPU), and the slot in the process of being updated looks like the
**	thread ID of the reader. HOWEVER, for this to happen, several 
**	conditions would have to occur simultaneously:
**	1.	The timing would have to be perfect for the scenario described,
**		and the number of updates is very small--only when a thread
**		(session) is created or destroyed.
**	2.	The reader would have to be looking up a thread that has a hash
**		collision, since, otherwise, the reader would be looking at a 
**		different hash table slot.
**	3.	The slot being updated would have to look like the thread ID of
**		the reader, a one-in-two-to-the-thirty-second-minus-one chance.
**
** Inputs:
**	tid			Thread ID to be hashed.
**	listp			Pointer to an area for the slot address.
**
** Outputs:
**	listp			Pointer to the slot.
**
**      Returns:
**          OK  - success
**          !OK - failure 
**
**  History:
*/
static
STATUS
CSMT_tls_search_list( CS_THREAD_ID tid, CS_TLS_LIST **listp )
{
    register CS_TLS_LIST *tls_list_entry;
    CS_TLS_LIST		*bucket_entry;
    unsigned int	version;

    bucket_entry = CS_tls_keyq + CS_TLS_HASH(((u_i4)tid));
    do
    {
    	tls_list_entry = bucket_entry;
    	version = CS_tls_version;
    	do
    	{
	    if ( CS_thread_id_equal(tls_list_entry->tid, tid) )
	    {
	    	if (version == CS_tls_version)
	    	{
	    	    *listp = tls_list_entry;	/* A "clean" read */
		    ++(tls_list_entry->reads);
		    return OK;
	    	}
		++(tls_list_entry->dirty);
	    	break;		/* A "dirty" read - start over. */
	    }
	    /* Search next sequential slot: if past end, wrap to beginning */
	    if (++tls_list_entry == CS_tls_keyq + CS_tls_buckets)
	    	tls_list_entry = CS_tls_keyq;
    	} while (tls_list_entry != bucket_entry);
    } while (version != CS_tls_version);

    /* Not found: search for an empty slot...this is most likely an insert */
    do                                                                   	
    {                                                                    	
        if (tls_list_entry->tid == 0 && tls_list_entry->value == NULL)     	
            break;                                                   	
        /* Search next sequential slot: if past end, wrap to beginning */	
        if (++tls_list_entry == CS_tls_keyq + CS_tls_buckets)
            tls_list_entry = CS_tls_keyq;                             	
    } while (1);
    *listp = tls_list_entry;
    
    return FAIL;
}

/*{
** Name:        CSMT_tls_init - Initialize the CS_tls hash table
**
** Description:
**	Get memory for the CS_tls hash array based on the max. num. of sessions.
**
** Inputs:
**	max_sessions			Max. num. of sessions allowed
**
** Outputs:
**	status				Status...
**
**      Returns:
**          OK  - success
**          !OK - failure 
**
**  History:
*/
VOID
CSMT_tls_init( u_i4 max_sessions, STATUS *status )
{
    u_i4	buckets = max_sessions * 2 + 1;

    if (buckets > CS_tls_buckets)
    {
    	CS_tls_buckets = buckets;
	if (CS_tls_keyq != &CS_tls_admin)
	    MEfree(CS_tls_keyq);
    	CS_tls_keyq = (CS_TLS_LIST *) 
    	    MEreqmem(0, sizeof(CS_TLS_LIST) * CS_tls_buckets, TRUE, status);
    }
}

/*{
** Name:        CSMT_tls_set         - Assign a value to tls memory
**
** Description:
**      Assign a pointer value to the SCB for this thread.
**
** Inputs:
**	value - the value to store in the slot
**
** Outputs:
**
**      Returns:
**          OK  - success
**          !OK - failure 
**
**  History:
*/
VOID
CSMT_tls_set( PTR value, STATUS *status )
{
    CS_THREAD_ID	tid;
    CS_TLS_LIST 	*tls_list_entry;
    CS_TLS_LIST		*tls_new_entry;

    CS_thread_id_assign(tid, CS_get_thread_id());

    CS_synch_lock( &CS_tls_mutex );

    if ( CSMT_tls_search_list( tid, &tls_list_entry ) != OK )
    {
    	/* This is the first time in for this thread */
    	++CS_tls_version;
	CS_thread_id_assign(tls_list_entry->tid, tid);
	tls_list_entry->value = value;
    }
    else
    {
	tls_list_entry->value = value;
    }
    ++(tls_list_entry->writes);

    CS_synch_unlock( &CS_tls_mutex );

    *status = OK;
    return;
}

/*{
** Name:        CSMT_tls_get         - Retrieve a value from tls memory
**
** Description:
**      Retrieve a pointer value for the SCB for this thread.
**
** Inputs:
**
** Outputs:
**	value - the stored value
**
**      Returns:
**          OK  - success
**          !OK - failure
**
**  History:
*/
VOID
CSMT_tls_get( PTR *value, STATUS *status )
{
    CS_THREAD_ID        tid;
    CS_TLS_LIST 	*tls_list_entry;

    CS_thread_id_assign(tid, CS_get_thread_id());

    if ( CSMT_tls_search_list( tid, &tls_list_entry ) != OK )
    {
	*value = NULL;
	*status = FAIL;
	return;
    }

    *value = tls_list_entry->value;
    *status = OK;
    return;
}

/*{
** Name:        CSMT_tls_destroy         - Destroy slot associated SCB
**
** Description:
**      Destroy the thread-local storage associated with an 
**	SCB for this thread only.
**
** Inputs:
**
** Outputs:
**
**      Returns:
**          OK  - success
**          !OK - failure
**
**  History:
*/
VOID
CSMT_tls_destroy( STATUS *status )
{
    CS_THREAD_ID        tid;
    CS_TLS_LIST 	*tls_list_entry;
    PTR			value;

    CS_thread_id_assign(tid, CS_get_thread_id());

    CS_synch_lock( &CS_tls_mutex );

    if ( CSMT_tls_search_list( tid, &tls_list_entry ) != OK )
    {
        CS_synch_unlock( &CS_tls_mutex );
	*status = FAIL;
        return;
    }

    /* Increment the version first, then update the hash bucket entry */
    ++CS_tls_version;
    value = tls_list_entry->value;
    tls_list_entry->tid = 0;
    tls_list_entry->value = NULL;
    tls_list_entry->reads = 0;
    tls_list_entry->writes = 0;
    tls_list_entry->dirty = 0;

    CS_synch_unlock( &CS_tls_mutex );

    MEfree( value );

    *status = OK;
    return;
}

/*{
** Name:        CSMT_tls_measure - Measure statistics for iisampler
**
** Description:
**      Called from iisampler.c to measure statistics of the CS_tls hash
**	algorithm. This way, the measurements are collected only when the
**	sampler is running, and the measuring process does not drag down
**	the hashing algorithm itself, which has a NEED for SPEED.
**
** Inputs:
**	tid				Thread ID of the thread being measured.
**	numtlsslots, numtlsprobes,	Pointers to u_i4s for returning info.
**	    numtlsdirty
**
** Outputs:
**	numtlsslots			Num. of hash slots currently in use.
**	numtlsprobes			Num. of probes needed to find this tid.
**	numtlsdirty			Num. of "dirty" reads to find this tid.
**
**      Returns:
**
**  History:
*/
VOID
CSMT_tls_measure( CS_THREAD_ID tid, u_i4 *numtlsslots, u_i4 *numtlsprobes,
		i4 *numtlsreads, i4 *numtlsdirty, i4 *numtlswrites )
{
    register CS_TLS_LIST *tls_list_entry;
    register CS_TLS_LIST *bucket_entry;
    unsigned int	version;
    u_i4		numprobes;

    ++(*numtlsslots);
    bucket_entry = CS_tls_keyq + CS_TLS_HASH(((u_i4)tid));
    do
    {
	numprobes = 0;
    	tls_list_entry = bucket_entry;
    	version = CS_tls_version;
    	do
    	{
	    ++numprobes;
	    if ( CS_thread_id_equal(tls_list_entry->tid, tid) )
	    {
	    	if (version == CS_tls_version)
	    	{
	    	    /* A "clean" read */
	    	    *numtlsprobes += numprobes;
		    *numtlsreads += tls_list_entry->reads;
		    *numtlsdirty += tls_list_entry->dirty;
		    *numtlswrites += tls_list_entry->writes;
		    return;
	    	}
	    	break;		/* A "dirty" read - start over. */
	    }
	    /* Search next sequential slot: if past end, wrap to beginning */
	    if (++tls_list_entry == CS_tls_keyq + CS_tls_buckets)
	    	tls_list_entry = CS_tls_keyq;
    	} while (tls_list_entry != bucket_entry);
    } while (version != CS_tls_version);

}

#endif
