/*
**Copyright (c) 2004 Ingres Corporation
*/

/**CL_SPEC
** Name: DILRU.H - This file contains the internal structs for DIlru routines.
**
** Specification:
**  Description:
**
**	The DI module must support up to 256 open files (TCB's).  Since
**	most if not all UNIX boxes can not support this many open file
**	descriptors this module caches currently open files per thread.
**
**	At DIopen time a path and filename are passed into the function.
**	At this time an new numeric id (unique to the server - ie. all processes
**	connected to a shared memory segment.  This id is currently made up of
**	a timestamp, process id, and a counter) is assigned
**	to the file by DIlru_uniq.  Then DI attempts to open the file using
**	DIlru_open.  This function first attempts an open() on the file.
**	If it gets an error, then it closes a file using (DIlru_close) in
**	the descriptor cache, and retries the open until it either
**	succeeds in opening the file or runs out of files to close.
**
**	When it succeeds in opening the file it adds an entry to the
**	file descriptor cache.  The number of entries in the descriptor
**	cache is <= the number of files a single process can open.
**
**	Upon successful open the DI_IO struct passed in by the client of
**	DI will contain a copy of the filename, filename length, pathname,
**	pathname length, and the unique id.  This copy of the path and
**	file name could be changed to pointers if the client of DI would
**	guarantee the pointers passed into DIopen would be valid for the
**	lifetime of the DI_IO structure.  This optimization would cut down
**	the overhead per open file caused by these copies (currently approx.
**	128 bytes * 256 open files).
**
**	On successive DI calls a pointer to a DI_IO struct is passed in.
**	In this structure is the unique id assigned by DIlru_uniq.  The
**	unique id is used to do a hash search of the open file descriptor
**	cache.  If the file is found then the current open file descriptor
**	is used, else the path and filename information stored in the DI_IO
**	by DIopen are used to open the file (using DIlru_close if necessary).
**      
**	The above design takes into account the operation of DI in the UNIX
**	server environment.  In that environment there may only be one copy
**	of the DI_IO structure per server (assuming all DMF control blocks
**	come from shared memory).  DI has been designed to accept the 
**	information stored in the DI_IO struct and map that to a process
**	specific open file descriptor.
**
**	A side effect of this implementation is that DI will eventually eat
**	all available file descriptors.  All other CL routines used by the
**	DBMS, which use file descriptors need to be changed to call 
**	DIlru_free(), to free up file descriptors if necessary.
**
**  History:
**      03-mar-87 (mmm)
**          Created new for 6.0.
**	13-Feb-89 (anton)
**	    Added flags for DIlru_open
**	20-Mar-90 (anton)
**	    Added semaphore and race status fields to DI_FILE_DESC
**	01-Oct-90 (anton)
**	    Use CS conditions to protect lru race problem
**	30-October-1992 (rmuth)
**	    - Prototype.
**	    - Add DIlru_set_di_error;
**	30-November-1992 (rmuth)
**	    Add err_code to some functions.
**	30-feb-1993 (rmuth)
**	   Add DI_FILE_MODE for file permissions and change to 0700 as 
**	   opposed to 0777 at the request of the C2 people.
**	26-jul-93 (mikem)
**	    fixed unclosed comment, su4_us5 compiler issued warning.
**	23-aug-1993 (bryanp)
**	    Added DI_lru_slmapmem prototype.
**	18-Feb-1997 (jenjo02)
**	    Major overhaul.
**	    Removed hash buckets, free queue, reliance on single
**	    DI_misc_sem mutex. Now there's a mutex per DI_FILE_DESC
**	    and a single queue of all descriptors with opened ones
**	    at the front of the list and closed/free ones at the tail.
**	22-Dec-1998 (jenjo02)
**	    Added support for optional fd-per-thread-per-file support.
**	    To enable it, define DI_FD_PER_THREAD in here.
**	21-May-1999 (hanch04)
**	    Change DI_FILE_MODE from 0700 to 0600 for Solaris OS to 
**	    page out files system cache data pages that would have been read
**	    into the Ingres cache anyway.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	8-Sep-2005 (schka24)
**	    Enable FD-per-thread for Solaris.
**	06-Oct-2005 (jenjo02)
**	    Productize DI_FD_PER_THREAD as 
**	    "ii.$.$.$.fd_affinity: FILE | THREAD"
**	    config parameter.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DI_FILE_DESC		DI_FILE_DESC;
typedef struct _DI_HASH_TABLE_CB	DI_HASH_TABLE_CB;


/*
**  Forward and/or External function/procedure references.
*/


/*
**  Local Definitions.
*/
#define			DI_FILE_MODE	0600

#define			DI_ENTRY_NUM	64
					/* Number of DI_FILE_DESC to allocate
					** when we've run out
					*/

/* Flags for DIlru_open */
#define			DI_FCREATE	1	/* create the file */
#define			DI_NORETRY	2	/* don't retry the open */

/*}
** Name: DI_FILE_DESC - An entry in the file descriptor cache.
**
** Description:
**	This structure is used to keep track of unique id's and file 
**	descriptors.  Every open file is assigned a unique id by DIlru_unique().
**	Every DI file currently opened by a single process/session is stored
**	in an entry in the file descriptor cache.   On every DI call the
**	unique id is obtained from the DI_IO struct passed in by the client.  
**	This id is used to search the file descriptor cache for an open file 
**	descriptor matching the unique id.  If an entry can not be found then 
**	DIlru_open will open the file using information stored in the DI_IO 
**	struct.  DIlru_open will remove an entry from the file descriptor cache
**	using DIlru_free if the open fails due to out-of-file-descriptor error.
**
** History:
**     03-mar-87 (mmm)
**          Created new for jupiter.
**	20-Mar-90 (anton)
**	    Added semaphore and race status fields for DI fixs
**	26-jul-93 (mikem)
**	    fixed unclosed comment, su4_us5 compiler issued warning.
**	18-Feb-1998 (jenjo02)
**	    Removed fd_lruq, fd_condition, added fd_mutex.
**	30-Sep-2005 (jenjo02)
**	    Make fd_mutex a CS_SYNCH
**	14-Oct-2005 (jenjo02)
**	    Add SL_OPEN_IN_PROGRESS racestat for
**	    Slave open; allows release of fd_mutex while
**	    the Slave is doing an open.
*/
struct _DI_FILE_DESC
{
    QUEUE	    fd_q;		/* link to other allocated fds */
#ifdef OS_THREADS_USED
    QUEUE	    fd_io_q;		/* queue of fds on same DI_IO */
#endif /* OS_THREADS_USED */
    volatile DI_UNIQUE_ID fd_uniq;	/* server unique id for file */
    i4		    fd_refcnt;		/* reference count to this structure */
    volatile i4	    fd_state;		/* State of this fd: */
#define		FD_FREE		0
#define		FD_IN_USE	1
    STATUS	    fd_racestat;	/* Special status indicator */
					/* concurrent open case */
#define		SL_OPEN_IN_PROGRESS 0x80000000
    CS_SYNCH        fd_mutex;		/* A mutex for this FD */
    short	    fd_fd_in_slave[DI_MAX_SLAVES];
			/* array of file descriptor values for each slave */
#define	fd_unix_fd fd_fd_in_slave[0]	/* for compatapility */
};

/*}
** Name: DI_HASH_TABLE_CB - Control block for file descriptor cache
**
** Description:
**	All information needed to access the DI file descriptor cache is
**	located in the DI_HASH_TABLE_CB.  This structure should be
**	allocated once out of local/per-session memory either statically
**	as a GLOBALDEF or dynamically out of per-process heap space.
**	Initialization happens dynamically the first time it is 
**	accessed (currently all DIlru_* entry points check to see if the
**	structure is initialized and initialize it if it isn't - it would be 
**	desirable to add a DIinitialize() routine which could pass in the 
**	maximum number of files a process could open in a single session.)
**
** History:
**     03-mar-87 (mmm)
**          Created new for jupiter.
**	18-Feb-1998 (jenjo02)
**	    Removed htb_free_list, htb_buckets, added some stats.
**	30-Sep-2005 (jenjo02)
**	    Make htb_fd_list_mutex a CS_SYNCH.
*/
struct _DI_HASH_TABLE_CB
{
    i4		    htb_fd_list_count;	/* Number of DI_FILE_DESC */
    CS_SYNCH        htb_fd_list_mutex;	/* A mutex to lock the fd list */
    QUEUE    	    htb_fd_list;	/* queue of DI_FILE_DESC's */
    u_i4	    htb_seqno;		/* file open sequence number */
    struct
    {
      i4     	    fds_open;		/* # of files open now */
      i4	    max_fds;		/* max # of files open at once */
      u_i4	    hits; 		/* # of requests satisfied by cache */
      u_i4	    requests;		/* # of requests of the cache */
      u_i4	    opens;		/* # of file opens */
      u_i4	    lru_closes;		/* # of files closed by LRU */
      u_i4	    io_hits;		/* # hits in DI_IO list */
    }		htb_stat;
};

/*
** Function Prototypes
*/

FUNC_EXTERN STATUS 	DIlru_close(
				DI_IO           *f,
				CL_ERR_DESC     *err_code );

FUNC_EXTERN STATUS	DIlru_free( CL_ERR_DESC     *err_code );

FUNC_EXTERN STATUS 	DIlru_init(
				DI_HASH_TABLE_CB        **global_htb,
				CL_ERR_DESC 		*err_code);

FUNC_EXTERN STATUS 	DIlru_init_di( CL_ERR_DESC *err_code);

FUNC_EXTERN STATUS 	DIlru_open(
				DI_IO           *f,
				i4             flags,
				DI_OP           *op,
				CL_ERR_DESC     *err_code );

/* Increment the pin count on an existing file descriptor */
FUNC_EXTERN VOID	DIlru_pin(DI_OP *op);

/* assign unique numeric id to new file */
FUNC_EXTERN  STATUS      DIlru_uniq(
				DI_IO           *f,
				CL_ERR_DESC 	*err_code);

FUNC_EXTERN VOID        DIlru_dump( VOID );

FUNC_EXTERN STATUS      DI_complete(
				CSEV_CB *evcb );

FUNC_EXTERN STATUS      DIlru_release(
				DI_OP  *op,
				CL_ERR_DESC *err_code );

FUNC_EXTERN STATUS      DIlru_flush( 
				CL_ERR_DESC *err_code);


FUNC_EXTERN VOID        DIlru_set_di_error( 
				i4     *ret_status,
				CL_ERR_DESC *err_code,
				i4     intern_err,
				i4     meaningfull_err );


FUNC_EXTERN STATUS	DI_lru_slmapmem(
				ME_SEG_INFO	*seginfo,
				STATUS		*intern_status,
				STATUS		*send_status);
