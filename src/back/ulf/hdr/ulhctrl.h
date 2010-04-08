/*
** Copyright (c) 2004 Ingres Corporation
**
*/
/**
** Name: ULHCTRL.H - Control Blocks and Def's used internally by ULH.
**
** Description:
**      This header file contains definitions of all data structures and
**	constants used internally by ULH.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	18-sep-87 (puree)
**	    Add user semaphores to hash objects.  Remove all other semaphores
**	    but the table semaphore to avoid unneccesary semaphore overhead.
**	02-nov-87 (puree)
**	    Added alias support.
**	04-apr-90 (andre)
**	    Defined a new structure, ULH_HALIAS, to represent an alias and moved
**	    several fields from ULH_HOBJ into it.  This was required to support
**	    multiple aliases.
**      14-aug-91 (jrb)
**          Changed ulh_usem to be a CS_SEMAPHORE instead of SCF_SEMAPHORE in
**          ULH_HOBJ.
**      07-apr-93 (smc)
**          Commented out text after ifdef.
**	03-jun-1996 (canor01)
**	    Change two char members of ULH_HOBJ to nats.  This structure in
**	    part overlaps with a ULH_OBJECT structure.  With the Solaris 4.0
**	    C compiler, the structures don't align with one another.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Apr-2004 (stial01)
**          Define tcb_length as SIZE_TYPE.
**/

/*
**      The define statement below indicates the beginning of
**      user-visible portion of an object.
*/

#define uobject	ulh_obj.ulh_link.object   /* user-visible object */
#define MAX_ALIAS_LEN  (sizeof(PTR)+sizeof(DB_TAB_NAME)+sizeof(DB_OWN_NAME))
#define ULH_EXCLUSIVE	1


/*}
** Name: ULH_LINK - Queue Linker Strucutre
**
** Description:
**      In order to share common queue manipulation routines among various
**      ULH queues, a standard pointer structure, ULH_LINK, is implemented.
**      This structure is embedded in the data structure of each member of
**      a queue (or a list).  Members of a queue are linked together using
**      the ULH_LINK's.. Multiple ULH_LINK's are needed for a structure
**      that may become a member to multiple lists as the same time.
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_LINK
{
    struct _ULH_LINK	*next;		/* pointer to the ULH_LINK of
					** the next element */
    struct _ULH_LINK	*back;		/* pointer to the ULH_LINK of
					** the previous element */
    PTR			object;		/* pointer to the beginning of
					** this element */
}   ULH_LINK;

/*}
** Name: ULH_UNIT - A Unit of A Queue/List Member
**
** Description:
**      This structure is embedded in every object or class header
**      block.  Its main purpose is to ensure the relative location
**	of object/class name string and the ULH_LINK within each 
**	object/class header.  This allows the use of a single search
**	routine for both the object and the class buckets.
**
**      Caution:
**	This structure overlays a portion of  the ULH_OBJECT structure 
**	in ULH.H.  Any 	changes in this structure must be manually copied 
**	to ULH_OBJECT. 
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_UNIT
{
    ULH_LINK	    ulh_link;		    /* hash bucket linked list */
    char	    ulh_hname[ULH_MAXNAME]; /* object/class name */
    i4		    ulh_namelen;	    /* length of name */
}   ULH_UNIT;

/*}
** Name: ULH_QUEUE - Queue Header
**
** Description:
**      This queue header structure is used for all ULH queues.
**
** History:
**      08-jan-87 (puree)
*/
typedef struct _ULH_QUEUE
{
    ULH_LINK	    *head;		/* pointer to the ULH_LINK of 
					** of the first element */
    ULH_LINK	    *tail;		/* pointer to the ULH_LINK of
					** the last element */
}   ULH_QUEUE;

/*}
** Name: ULH_BUCKET - Hash Bucket
**
** Description:
**      A hash table is a linear array of hash buckets.  The hash buckets
**      for an object table and for a class table are identical.  A hash
**      bucket array is created when the table is created.  It is allocated
**      from the same memory stream as the ULH_TCB structure.  The pointer
**      to the object bucket array is kept in ULH_TCB.ulh_otab and the
**      pointer to the class bucket array is kept in ULH_TCB.ulh_ctab.
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_BUCKET
{
    ULH_QUEUE	    b_queue;		/* header of object list */
    i4		    b_count;		/* count of objects in the bucket */
}   ULH_BUCKET;

/*}
** Name: ULH_CHDR - A Class Header
**
** Description:
**      A class is a linked list of objects that bear certain relationship
**      to each other.  The class header block, ULH_CHDR, is used to
**      represent a class.  It contains the name string of the class and
**      a queue header to which member objects are linked together.
**      Classes that yield the same hash key are linked together to the
**      same hash bucket in the class table.
**
**	All class header blocks are allocated at the same time, and
**	from the same memory stream, as the class table (the class
**	bucket array).  The empty class headers are linked together
**	in the LIFO queue, ULH_TCB.ulh_chq.
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_CHDR
{
    ULH_BUCKET	    *cl_bptr;		/* back pointer to hash bucket */
    ULH_UNIT	    cl_unit;		/* hash unit for this class header */
    ULH_QUEUE	    cl_queue;		/* queue of objects in this class */
    i4		    cl_count;
}   ULH_CHDR;

/*}
** Name: ULH_HOBJ - Internal ULH Object Header
**
** Description:
**      This structure is the object header used internally by ULH.  It
**      contains all information pertaining to an object.  The structure
**      is created and destroyed together with the object it represents.
**      The data space for the structure is allocated in its owns memory
**      stream from the table memory pool.  The user-visible portion of
**      of the object header begins at .ulh_unit.ulh_link.object and
**      extends to the end of the structure.  When a user is granted
**      access to an object, the pointer to this structure is returned
**      in ULH_RCB.ulh_object->ulh_id.  This pointer is required for all
**      subsequent accesses to the object.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	02-nov-87 (puree)
**	    added back ptr for alias bucket
**	4-apr-90 (andre)
**	    To facilitate implementation of synonyms, we wil  (finally) start
**	    supporting multiple aliases.  As a result, the following fields
**	    will be moved from ULH_HOBJ into ULH_HALIAS:
**		ulh_ap, ulh_alink, ulh_alias_len, ulh_halias
**	    We also need to add a  queue structure to be used when we need to
**	    access the list of synonyms associated with this object
**      14-aug-91 (jrb)
**          Changed ulh_usem to be a CS_SEMAPHORE instead of SCF_SEMAPHORE.
**	29-may-92 (teresa)
**	    Added ulh_uniq_id for class support.
*/
typedef struct _ULH_HOBJ
{
    ULH_BUCKET	    *ulh_bp;		/* back pointer to hash bucket */
    ULH_CHDR	    *ulh_cp;		/* back pointer to class header */
    ULH_LINK	    ulh_clink;		/* linked list for class */
    ULH_LINK	    ulh_llink;		/* linked list for LRU queue */
    ULH_QUEUE	    ulh_aq;		/*
					** list of aliases defined for this
					** object
					*/
    i4		    ulh_lindx;		/* LRU queue index */
    i4		    ulh_ucount;		/* lock mode/user count */
    i4	    ulh_flags;		/* object flags */
#define		ULH_NEW		(char)1	/* newly created object */
#define		ULH_USED	(char)0	/* multiple-use object */
    i4	    ulh_type;		/* object type indicator */
    ULH_UNIT	    ulh_obj;		/* hash unit for this object */
    CS_SEMAPHORE   ulh_usem;		/* user semaphore */
    PTR		    ulh_streamid;	/* associated memory stream id */
    PTR		    ulh_uptr;		/* user-defined pointer */
    i4		    ulh_uniq_id;	/* unique identifier for this member of
					** a class -- not used unless obj is a
					** class object */
}   ULH_HOBJ;

/*}
** Name: ULH_HALIAS - Internal ULH Object Alias structure
**
** Description:
**      This structure represents an alias defined for some object.
**	It was added in order to allow for support of multiple aliases.
**	Each object will have a (possible empty) list of aliases associated with
**	it.  Each alias will be a part of 2 linked lists: one associating it
**	with a specific hash backet and the other - with the object for which it
**	was defined.  Each alias will also contain a pointer to the object for
**	which it was defined to facilitate access to the object (and all info
**	contained therein) once an alias has been located.
**
**	NOTE: if some changes are made to this structure or any structures
**	      therein, ULH_H_ALIAS_SIZE in ulh.h must be changed to reflect the
**	      them.
** History:
**      04-apr-90 (andre)
**          defined.
*/
typedef struct _ULH_HALIAS
{
    ULH_BUCKET	    *ulh_ap;		/* back pointer to alias bucket */
    ULH_LINK	    ulh_alink;		/* linked list for alias bucket */
    ULH_LINK	    ulh_objlink;	/*
					** linked list of aliases for a given
					** object
					*/
    ULH_HOBJ	    *ulh_obj;		/*
					** object for which this alias has been
					** defined
					*/
    i4		    ulh_alias_len;		/* actual length of alias */
    char	    ulh_halias[MAX_ALIAS_LEN];	/* object alias */
}   ULH_HALIAS;

/*}
** Name: ULH_TCB - Hash Table Control Block
**
** Description:
**      This data strucuture is the primary control block for a (logical)
**      hash table.  It contains information needed for manipulation of
**      the table.  The strucuture is created when the table is created
**      via the ulh_init function.  It is allocated from the memory pool
**      and stream as specified in the arguments to the function.  The
**      pointer to this strucutre is kept in ULH_RCB.ulh_hashid which
**      is required for all subsequent accesses to the table.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	05-apr-90 (andre)
**	    add ulh_asize,ulh_maxalias,ulh_alias_count
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _ULH_TCB ULH_TCB;
struct _ULH_TCB
{
    ULH_TCB	    *next_tcb;		/* standard server header fields */
    ULH_TCB	    *prev_tcb;
    SIZE_TYPE	    tcb_length;		/* length og a TCB */
    i2		    tcb_type;		/* identifier */
#define	ULH_TYPE    0x0101
    i2		    tcb_s_reserved;	/* reserved for memory management */
    PTR		    tcb_l_reserved;
    PTR		    tcb_owner;		/* Owner field */
#define ULH_OWNER   CV_C_CONST_MACRO('U', 'L', 'H', '_')
    i4		    tcb_ascii_id;	/* Ascii ID */
#define ULH_ID	    CV_C_CONST_MACRO('T', 'C', 'B', ' ')

    ULH_TCB	    *ulh_tcb;		/* self ptr for validation purpose */
    CS_SEMAPHORE    ulh_sem;		/* table semaphore */
    i4		    ulh_facility;	/* Information for ULM interface */
    PTR		    ulh_poolid;	
    PTR		    ulh_streamid;
    SIZE_TYPE	    *ulh_memleft;
    ULH_QUEUE	    ulh_lru1;		/* 1st-access LRU queue header */
    ULH_QUEUE	    ulh_lru2;		/* multiple access LRU queue header */
    i4		    lru_cnt[2];		/* count of objects in LRU 1 & 2 */
    i4		    lru1_limit;		/* limit for signle-access objects */
    ULH_BUCKET	    *ulh_alstab;	/* ptr to alias table */
    ULH_BUCKET	    *ulh_otab;		/* pointer to object table */
    i4		    ulh_osize;		/* size of the object table */
    i4		    ulh_maxobj;		/* max. number of objects */
    i4		    ulh_asize;		/* size of the alias table */
    i4	    ulh_maxalias;	/* maximum number of aliases */
    i4	    ulh_alias_count;	/* current number of aliases */
    i4		    ulh_objcnt;		/* current count of objects */
    i4		    ulh_fixcnt;		/* current count of fixed objects */
    ULH_BUCKET	    *ulh_ctab;		/* pointer to the class table */
    i4		    ulh_csize;		/* size of the class table */
    i4		    ulh_maxclass;	/* max number of classes */
    i4		    ulh_ccnt;		/* current count of classes */
    ULH_CHDR	    *ulh_chq;		/* list of available class headers */

#ifdef xDEV_TEST
    i4		    ulh_max_objcnt;	/* actual max objects in table */
    i4		    ulh_b_avg;		/* avg objects per bucket */
    i4		    ulh_b_max;		/* max objects in a bucket */
    i4		    ulh_requests;	/* count of cache searches */
    i4		    ulh_hits;		/* count of hits */
    i4		    ulh_o_created;	/* count of objects created */
    i4		    ulh_o_destroyed;	/* count of objects destroyed */
#endif /* xDEV_TEST */
};
