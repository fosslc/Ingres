/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <bt.h>
#include <er.h>
#include <gc.h>
#include <me.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <tm.h>

#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcnint.h>

/*
** Name: gcnshash.c
**
** Description:
**	Hashing functions for Name Queue access.  Vnode and ticket
**	queues need fast access methods for handling server resolve
**	requests.  These queues may be very large and linear searchs
**	are too slow.  Transient server queues are very small and
**	are easily handled with linear searchs.
**
**	gcn_nq_hash	Initialize Name Queue for hashed access.
**	gcn_nq_unhash	Free hash resources.
**	gcn_nq_hadd	Add tuple to hash table.
**	gcn_nq_hdel	Delete tuple from hash table.
**	gcn_nq_hscan	Search hash table for tuple pattern.
**
**	The hash algorithm used is a dynamic or variable hash which
**	starts with a small hash table and extends as necessary to
**	fit the data.  The basic data elements are a hash table
**	an index and a mask.  The initial setup consists of a table
**	with a single entry (bucket) as follows:
**
**	index   1
**	       +-+
**	       +-+	
**	mask  x01
**	
**	All work is done in the single hash bucket until it is filled
**	and needs to be split, which may also require the table to be
**	extended.  The table is extended by adding a new level which 
**	is twice the size of the existing level.  Entries in the bucket 
**	to be split are moved to the next level at index * 2 and 
**	index * 2 + 1 based on the bit in their hash values referenced 
**	by the mask.
**
**	Splitting the initial bucket followed by the third bucket would
**	result in the following tables:
**
**	index   1 2 3
**	       +-+-+-+
**	       +-+-+-+
**	mask  x01|x02|
**
**	index   1 2 3     6 7
**	       +-+-+-+-+-+-+-+
**	       +-+-+-+-+-+-+-+
**	mask  x01|x02|  x04  |
**
**	Note that not only the entries in the bucket to be split need
**	to be rehashed to the next level.  When bucket 3 split creating
**	buckets 6 & 7, bucket 2 was not effected.  When bucket 2 splits,
**	creating buckets 4 & 5, no other buckets will be affected and
**	the table will not need extending.
**
**	Searching begins at the top level, index 1, and moves down to
**	the first active bucket by ANDing the hash value with the mask
**	and selecting a new index of current * 2 or current * 2 + 1
**	and shifting the mask to the left.
**	
**	The actual hashing implementation is designed to handle the
**	following Name Queue characteristics:
**
**	1.  Name Queues which require hashed access have different 
**	    components for their keys.
**	2.  The expected Name Queue sizes range in magnitude from  
**	    ten to several thousand.
**	3.  Some Name Queues can have a large percentage of their
**	    entries with the same hash value.
**	4.  Name Queue tuples are already maintained on a queue and
**	    the CL QU routines do not easily permit multiple queues.
**
**   Implementation details:
**
**	The Name Server assigns a hashing function, which returns 
**	a hash value for a GCN_TUP_QUE element, to a Name Queue by 
**	calling gcn_nq_hash().  Hashed access to a Name Queue is
**	only initialized when a hashing function is assigned.
**
**	The hash table is an array of pointers to buckets.  Nine
**	levels (2^0 - 2^8) is considered sufficient for the amount
**	of data expected and is not too deep for Queues which
**	degenerate into linear lists due to identical hash values.
**	Because of the limited depth of the table, it is pre-
**	allocated rather than mess with reallocation for extension.
**
**	Buckets will hold arrays of GCN_TUP_QUE pointers and their
**	hash values (as an additional optimization for searching
**	and splitting).  Bucket size is set according to the 
**	characteristics of the associated Name Queue with the
**	exception that all overflow buckets are larger in size.
**
**	The implementation is highly dependent on GCN_HASH being
**	declared as a u_i1.  Array sizing and mask testing rely
**	heavily on the characteristics of an 8-bit unsigned value.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Macro for sizing BT bit stream arrays.
*/
#define	BT_SIZE( bits )		(((bits) + BITSPERBYTE - 1) / BITSPERBYTE)


typedef struct _HASH_ENTRY	HASH_ENTRY;
typedef struct _HASH_BUCKET	HASH_BUCKET;
typedef struct _HASH_CB		HASH_CB;
typedef struct _SRCH_CB		SRCH_CB;


/*
** Hash bucket tuple entries.
*/
struct _HASH_ENTRY
{

    GCN_HASH	hval;			/* Hash value for tuple */
    GCN_TUP_QUE	*tup;			/* Name Queue tuple */

};


/*
** Hash buckets.
**
** The entry array is variable length to permit
** better handling characteristics for small and
** large Name Queues.
*/
struct _HASH_BUCKET
{

    HASH_BUCKET	*next;			/* Overflow */
    i4		size;			/* Bucket size */
    char	used[BT_SIZE(GCN_HASH_LARGE)];	/* Entry usage bitmap */
    HASH_ENTRY	entry[1];		/* Variable length entry array */

};


/*
** Hash control block.
**
** Hash table is a heap of buckets.
*/
struct _HASH_CB
{

    HASH_FUNC	func;			/* Hashing function for table */
    i4		size;			/* Default bucket size */

#define	HASH_TABLE_SIZE	512		/* Room for levels 2^0 through 2^8 */

    HASH_BUCKET	*table[ HASH_TABLE_SIZE ];

};

/*
** Search control block.
**
** Info used while search hash table for bucket
** associated with hash value, and searching
** bucket for tuple entries (slots).
*/
struct _SRCH_CB
{

    GCN_HASH	hval;			/* Target hash value */
    i4		index;			/* Hash table index */
    GCN_HASH	mask;			/* Hash value level mask */
    HASH_BUCKET	*bckt;			/* Hash bucket */
    i4		slot;			/* Slot in hash bucket */

};


/*
** Local functions.
*/
static	HASH_BUCKET *	table_search( HASH_CB *, SRCH_CB * );
static	STATUS		split( GCN_QUE_NAME *, i4, GCN_HASH );
static	STATUS		new_bucket( HASH_CB *, i4  );
static	STATUS		extend( HASH_BUCKET * );



/*
** Name: gcn_nq_hash
**
** Description:
**	Initialize a Name Queue for hashed access.  The function
**	provided will be used to generate hash values for tuples
**	in the associated queue.
**
**	This function must be called after gcn_nq_init() or
**	gcn_nq_create() has initialized the queue, but prior 
**	to loading the queue with the first call to gcn_nq_lock().
**
** Input:
**	nq		Name Queue.
**	func		Hashing function.
**	size		GCN_HASH_SMALL or GCN_HASH_LARGE.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK, E_GC0134_GCN_INT_NQ_ERROR, E_GC0121_GCN_NOMEM
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

STATUS
gcn_nq_hash( GCN_QUE_NAME *nq, HASH_FUNC func, i4  size )
{
    STATUS status;

    /*
    ** Make sure not already initialized
    ** and queue has not been loaded.
    */
    if ( nq->gcn_hash  ||  nq->gcn_incache )  
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "gcn_nq_hash: called at wrong time!\n" );

	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0134_GCN_INT_NQ_ERROR, NULL, 0, NULL );

	return( E_GC0134_GCN_INT_NQ_ERROR );
    }

    /*
    ** Allocate hash control block (includes hash table)
    ** and the initial hash bucket.
    */
    nq->gcn_hash = MEreqmem( 0, sizeof( HASH_CB ), TRUE, NULL );

    if ( ! nq->gcn_hash )  
    {
	ER_ARGUMENT earg[1];

	earg->er_value = ERx("hash table");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, 
		   E_GC0121_GCN_NOMEM, NULL, 1, (PTR)earg );

	return( E_GC0121_GCN_NOMEM );
    }

    ((HASH_CB *)nq->gcn_hash)->func = func;
    ((HASH_CB *)nq->gcn_hash)->size = size;

    /*
    ** Hash table begins with single bucket.
    */
    if ( (status = new_bucket( (HASH_CB *)nq->gcn_hash, 1 )) != OK )
    {
	MEfree( nq->gcn_hash );
	nq->gcn_hash = NULL;
    }

    return( status );
}


/*
** Name: gcn_nq_unhash
**
** Description:
**	Frees resources for hashing structures allocated for a
**	Name Queue.  May be used to flush all entries and leave
**	hash table empty or completely free hash table.
**
** Input:
**	nq		Name Queue.
**	flush		TRUE to remove all entries but leave hash table.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

VOID
gcn_nq_unhash( GCN_QUE_NAME *nq, bool flush )
{
    HASH_CB	*hash_cb = (HASH_CB *)nq->gcn_hash;
    HASH_BUCKET	*bucket, *next;
    HASH_BUCKET	*first = NULL;
    i4		i;

    if ( ! hash_cb )  return;

    /*
    ** Free buckets including overflow.
    */
    for( i = 1; i < HASH_TABLE_SIZE; i++ )
	for( bucket = hash_cb->table[i]; bucket; bucket = next )
	{
	    next = bucket->next;
	    hash_cb->table[i] = bucket->next = NULL;

	    /*
	    ** First bucket found is saved for reuse
	    ** (if needed), all other buckets are freed.
	    */
	    if ( flush  &&  ! first )
		first = bucket;
	    else
		MEfree( (PTR)bucket );
	}

    if ( flush )
    {
	/*
	** Initialize hash table.
	*/
	if ( ! first )
	    new_bucket( hash_cb, 1 );	/* shouldn't happen */
	else
	{
	    /*
	    ** Reuse a bucket.
	    */
	    hash_cb->table[1] = first;
	    MEfill( sizeof( first->used ), 0, (PTR)first->used );
	    MEfill( first->size * sizeof( first->entry[0] ), 
		    0, (PTR)first->entry );
	}

	if ( IIGCn_static.trace >= 5 )
	    TRdisplay( "gcn_nq_unhash: hash table cleared.\n" );
    }
    else
    {
	/*
	** Free the hash control block.
	*/
	MEfree( nq->gcn_hash );
	nq->gcn_hash = NULL;
    }

    return;
}


/*
** Name: gcn_nq_hadd
**
** Description:
**	Add an entry into a Name Queue hash table.
**
** Input:
**	nq		Name Queue
**	tup		Tuple being added.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK, FAIL (internal error), E_GC0121_GCN_NOMEM.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

STATUS
gcn_nq_hadd( GCN_QUE_NAME *nq, GCN_TUP_QUE *tup )
{
    HASH_CB	*hash_cb = (HASH_CB *)nq->gcn_hash;
    HASH_BUCKET	*bucket;
    SRCH_CB	srch;
    STATUS	status;
    i4		slot;
    i4		overflow = 0;
    bool	found = FALSE;

    if ( ! hash_cb )  return( FAIL );	/* should not happen! */

    srch.hval = (*hash_cb->func)( &tup->gcn_tup );  /* Hash value for tuple */
    srch.index = 1;
    srch.mask = 0x01;

    while( ! found )
    {
	/*
	** Find bucket associated with tuple hash value.
	*/
	if ( ! (bucket = table_search( hash_cb, &srch )) )
	    return( FAIL );	/* should not happen! */

	/*
	** Is the bucket full?
	*/
	if ( BTcount( bucket->used, bucket->size ) != bucket->size )
	    found = TRUE;	/* bucket not full */
	else
	{
	    /*
	    ** At intermediate levels we simply need to split the
	    ** bucket and continue the search.  The search will
	    ** continue at the current index and move to one of
	    ** the new buckets from the split.  Intermediate levels
	    ** have a bit set in the hash value mask for testing
	    ** their representitive bit while the mask is 0x00
	    ** for the lowest level.
	    */
	    if ( srch.mask )
	    {
		if ( (status = split( nq, srch.index, srch.mask )) != OK )
		    return( status );
	    }
	    else
	    {
		/*
		** Search overflow list for a bucket which is not full.
		*/
		while( bucket->next )
		{
		    bucket = bucket->next;
		    overflow++;

		    if ( BTcount(bucket->used, bucket->size) != bucket->size )
		    {
			found = TRUE;	/* bucket is not full */
			break;
		    }
		}

		if ( ! found )
		{
		    /*
		    ** Add new overflow bucket since
		    ** all other buckets are full.
		    */
		    if ( (status = extend( bucket )) != OK )
			return( status );

		    bucket = bucket->next;
		    overflow++;
		    found = TRUE;
		}
	    }
	}
    }

    /*
    ** Find empty slot and save tuple info.
    ** We need to find a cleared bit, but
    ** BTnext() only searchs for set bits.
    */
    for( slot = 0; slot < bucket->size; slot++ )
	if ( ! BTtest( slot, bucket->used ) )  break;

    BTset( slot, bucket->used );
    bucket->entry[ slot ].hval = srch.hval;
    bucket->entry[ slot ].tup = tup;

    if ( IIGCn_static.trace >= 5 )
	if ( overflow )
	    TRdisplay( "gcn_nq_hadd: index %d overflow %d slot %d\n", 
		       srch.index, overflow, slot );
	else
	    TRdisplay( "gcn_nq_hadd: index %d slot %d\n", srch.index, slot );

    return( OK );
}


/*
** Name: gcn_nq_hdel
**
** Description:
**	Delete an entry from a Name Queue hash table.
**
** Input:
**	nq		Name Queue
**	tup		Tuple being deleted.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if entry found.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

bool
gcn_nq_hdel( GCN_QUE_NAME *nq, GCN_TUP_QUE *tup )
{
    HASH_CB	*hash_cb = (HASH_CB *)nq->gcn_hash;
    HASH_BUCKET	*bucket;
    SRCH_CB	srch;
    i4		slot;
    i4		overflow = 0;
    bool	found = FALSE;

    if ( ! hash_cb )  return( FALSE );	/* should not happen! */

    srch.hval = (*hash_cb->func)( &tup->gcn_tup );  /* Hash value for tuple */
    srch.index = 1;
    srch.mask = 0x01;

    /*
    ** Find bucket associated with tuple hash value.
    */
    bucket = table_search( hash_cb, &srch );

    /*
    ** Search bucket (and possibly overflow) for tuple.
    */
    while( ! found  &&  bucket )
    {
	/*
	** Search bucket entries for tuple.
	*/
	for(slot = -1; (slot = BTnext(slot, bucket->used, bucket->size)) >= 0;)
	    if ( bucket->entry[ slot ].tup == tup )
	    {
		BTclear( slot, bucket->used );
		bucket->entry[ slot ].tup = NULL;	/* just to be safe */
		found = TRUE;

		if ( IIGCn_static.trace >= 5 )
		    if ( overflow )
			TRdisplay("gcn_nq_hdel: index %d overflow %d slot %d\n",
				    srch.index, overflow, slot );
		    else
			TRdisplay( "gcn_nq_hdel: index %d slot %d\n", 
				    srch.index, slot );
		break;
	    }

	if ( ! found )  
	{
	    /*
	    ** Check overflow (if any).
	    */
	    bucket = bucket->next;
	    overflow++;
	}
    } 

    if ( ! found  &&  IIGCn_static.trace >= 1 )
	TRdisplay( "gcn_nq_hdel: failed to locate tuple 0x%p (hash %d)\n",
		   tup, srch.hval );

    return( found );
}


/*
** Name: gcn_nq_hscan
**
** Description:
**	Retrieve a list of tuples matching a tuple mask (mask
**	may contain wildcards, but not in fields used in hash
**	key).  A scan control block maintains current scan info
**	for subsequent calls.  The control block (a PTR) should
**	be set to NULL prior to the first call and should not
**	be changed between subsequent calls.
**
**	This routine is also able to return a list of Name Queue
**	entry pointers corresponding to the tuple list returned,
**	which may be used as input to gcn_nq_qdel() to delete the
**	matched tuples directly.
**
**	DUE TO CURRENT IMPLEMENTATION, THERE MAY ONLY BE ONE
**	SCAN ACTIVE AT ANY GIVEN TIME.
**
** Input:
**	nq		Name Queue.
**	tupmask		Tuple mask with matching criteria.
**	scan_cb		Scan control block, set initially to NULL.
**	tupmax		Number of entries in following vectors.
**
** Output:
**	tupvec		Matching tuples, may be NULL.
**	qvec		Matching queue pointers, may be NULL.
**
** Returns:
**	i4		Number of matching entries.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

i4
gcn_nq_hscan( GCN_QUE_NAME *nq, GCN_TUP *tupmask, PTR *scan_cb,
	      i4  tupmax, GCN_TUP **tupvec, PTR *qvec )
{
    HASH_CB		*hash_cb = (HASH_CB *)nq->gcn_hash;
    static SRCH_CB	srch_cb;
    SRCH_CB		*srch = &srch_cb;
    GCN_TUP_QUE		*tq;
    i4			count = 0;

    if ( ! hash_cb )  return( 0 );	/* should not happen! */

    if ( ! *scan_cb )
    {
	/*
	** Initialize scan control block.
	** Search for bucket associated 
	** with the target hash value.
	*/
	*scan_cb = (PTR)srch;
	srch->hval = (*hash_cb->func)( tupmask );  /* Hash value for tuple */
	srch->index = 1;
	srch->mask = 0x01;
	srch->slot = -1;

	srch->bckt = table_search( hash_cb, srch );
    }

    while( count < tupmax  &&  srch->bckt )
    {
	/*
	** Search bucket for tuples matching tuple template.
	*/
	while( (srch->slot = BTnext( srch->slot, srch->bckt->used, 
				     srch->bckt->size )) >= 0 )
	{
	    /*
	    ** See if entry matches tuple template.  We do a
	    ** quick check on hash values first before the
	    ** more costly gcn_tup_compare().
	    */
	    if ( srch->bckt->entry[ srch->slot ].hval == srch->hval )
	    {
		tq = srch->bckt->entry[ srch->slot ].tup;
		
		if (gcn_tup_compare(&tq->gcn_tup, tupmask, nq->gcn_subfields))
		{
		    if ( tupvec )  tupvec[ count ] = &tq->gcn_tup;
		    if ( qvec )  qvec[ count ] = (PTR)tq;
		    if ( ++count >= tupmax )  break;
		}
	    }
	}

	/*
	** If we haven't filled the output arrays,
	** then we must have exhausted the entries
	** in the bucket.  Check overflow buckets.
	*/
	if ( count < tupmax )
	{
	    srch->bckt = srch->bckt->next;
	    srch->slot = -1;
	}
    }

    if ( IIGCn_static.trace >= 3 )
	TRdisplay( "gcn_nq_hscan [%s]: %s,%s,%s = %d\n", nq->gcn_type,
		   tupmask->uid, tupmask->obj, tupmask->val, count );

    return( count );
}


/*
** Name: table_search
**
** Description:
**	Searches a hash table for the bucket corresponding to a
**	hash value.  The search control block should be initialized
**	by the caller as follows:
**
**	    srch_cb->hval	Target hash value.
**	    srch_cb->index	1.
**	    srch_cb->mask	0x01.
**
**	Client initialization of the search control block permits
**	searching to be restarted if necessary (bucket splitting).
**	The search control block is modified as follows:
**
**	    srch_cb->index	Index of bucket returned.
**	    srch_cb->mask	Mask value for level of bucket.
**
** Input:
**	hash_cb		Hash control block.
**	srch_cb		Search control block.
**
** Output:
**	None.
**
** Returns:
**	HASH_BUCKET *	Bucket matching hash value, NULL if internal error.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

static HASH_BUCKET *
table_search( HASH_CB *hash_cb, SRCH_CB *srch_cb )
{
    while( ! hash_cb->table[ srch_cb->index ]  &&  srch_cb->mask )
    {
	srch_cb->index *= 2;
	if ( srch_cb->hval & srch_cb->mask )  srch_cb->index++;
	srch_cb->mask <<= 1;
    }

    if ( ! hash_cb->table[ srch_cb->index ]  &&  IIGCn_static.trace >= 1 )
    {
	/*
	** This should not happen!
	*/
	TRdisplay("hash search: internal error, could not find hash bucket\n");
    }

    return( hash_cb->table[ srch_cb->index ] );
}


/*
** Name: split
**
** Description:
**	Splits the hash bucket for provided index in hash table.
**	Child hash buckets are allocated and entries in parent
**	hash bucket are moved to children using the hash value
**	mask provided.  Parent hash bucket is freed.
**
**	This routine assumes the parent bucket is full!
**
** Input:
**	nq		Name Queue.
**	index		Hash table index for bucket.
**	mask		Hash mask for level of index.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK, E_GC0121_GCN_NOMEM.
**
** History:
**	16-Jun-98 (gordy)
*/

static STATUS
split( GCN_QUE_NAME *nq, i4  index, GCN_HASH mask )
{
    HASH_CB	*hash_cb = (HASH_CB *)nq->gcn_hash;
    HASH_BUCKET	*bckt = hash_cb->table[ index ];
    HASH_BUCKET	*bckt1;
    STATUS	status;
    i4		slot, slot1, hash_bit;
    i4		index1 = index * 2;

    /*
    ** The parent bucket is moved to be a child bucket
    ** based on the hash value of the first entry.  A
    ** new bucket is allocated for the sibling index.
    */
    hash_cb->table[ index ] = NULL;

    if ( (hash_bit = bckt->entry[ 0 ].hval & mask) )
    {
	if ( IIGCn_static.trace >= 5 )
	    TRdisplay( "hash split: index %d relocated to %d\n",
		       index, index1 + 1 );

	index = index1 + 1;
    }
    else
    {
	if ( IIGCn_static.trace >= 5 )
	    TRdisplay( "hash split: index %d relocated to %d\n",
		       index, index1 );

	index = index1;
	index1++;
    }

    if ( (status = new_bucket( hash_cb, index1 )) != OK )
	return( status );

    hash_cb->table[ index ] = bckt;
    bckt1 = hash_cb->table[ index1 ];

    /*
    ** Now redistribute the entries between the
    ** two sibling buckets.  Note that the first
    ** entry of the original bucket is in the
    ** correct place.  We take advantage of the
    ** fact that the original bucket is full and
    ** the new bucket is empty.
    */
    for( slot = 1, slot1 = 0; slot < bckt->size; slot++ )
	if ( (bckt->entry[ slot ].hval & mask) != hash_bit )
	{
	    BTclear( slot, bckt->used );
	    BTset( slot1, bckt1->used );
	    bckt1->entry[ slot1 ].hval = bckt->entry[ slot ].hval;
	    bckt1->entry[ slot1 ].tup = bckt->entry[ slot ].tup;
	    bckt->entry[ slot ].tup = NULL;	/* Just to be safe */

	    if ( IIGCn_static.trace >= 5 )
		TRdisplay( "hash split: moved [%d,%d] to [%d,%d]\n",
			   index, slot, index1, slot1 );
	    slot1++;
	}

    return( OK );
}


/*
** Name: new_bucket
**
** Description:
**	Allocates a hash bucket of the proper size and places it
**	in the Name Queue hash table at the requested index.
**
** Input:
**	hash_cb		Hash control block.
**	index		Hash table index for bucket.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK, E_GC0121_GCN_NOMEM.
**
** History:
**	16-Jun-98 (gordy)
*/

static STATUS
new_bucket( HASH_CB *hash_cb, i4  index )
{
    HASH_BUCKET	*bucket;
    STATUS	status = OK;
    i4		length = sizeof( HASH_BUCKET ) + 
			 sizeof( HASH_ENTRY ) * (hash_cb->size - 1);

    if ( (bucket = (HASH_BUCKET *)MEreqmem( 0, length, TRUE, NULL )) )
    {
	bucket->size = hash_cb->size;
	hash_cb->table[ index ] = bucket;
    }
    else
    {
	ER_ARGUMENT earg[1];

	status = E_GC0121_GCN_NOMEM;
	earg->er_value = ERx("hash bucket");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 1, (PTR)earg );
    }

    return( status );
}


/*
** Name: extend
**
** Description:
**	Allocates an overflow hash bucket and inserts it 
**	into the overflow list for the bucket provided.
**
** Input:
**	bucket		Hash bucket to be extended.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK, E_GC0121_GCN_NOMEM.
**
** History:
**	16-Jun-98 (gordy)
*/

static STATUS
extend( HASH_BUCKET *bucket )
{
    HASH_BUCKET	*new_bckt;
    STATUS	status = OK;
    i4		length = sizeof( HASH_BUCKET ) + 
			 sizeof( HASH_ENTRY ) * (GCN_HASH_LARGE - 1);

    if ( (new_bckt = (HASH_BUCKET *)MEreqmem( 0, length, TRUE, NULL )) )
    {
	new_bckt->size = GCN_HASH_LARGE;
	new_bckt->next = bucket->next;
	bucket->next = new_bckt;
    }
    else
    {
	ER_ARGUMENT earg[1];

	status = E_GC0121_GCN_NOMEM;
	earg->er_value = ERx("hash bucket (overflow)");
	earg->er_size = STlength( earg->er_value );
	gcu_erlog( 0, IIGCn_static.language, status, NULL, 1, (PTR)earg );
    }

    return( status );
}

