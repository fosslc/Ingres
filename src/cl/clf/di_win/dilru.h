/*
** Copyright (c) 1985, 1998 Ingres Corporation
*/

#include <qu.h>

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
**	08-dec-1997 (canor01)
**	    Copied from Unix CL.
**	28-jan-1998 (canor01)
**	    For re-write of NT LRU code, remove all reference to the 
**	    hash table and functions that used it.
**      04-mar-1998 (canor01)
**          Parameters to DIlru_flush were changed on Unix side.  Change
**          them here too.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
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
#define			DI_FILE_MODE	0700

#define			DI_ALLOC_SIZE	(2000)
					/* Number of bytes to allocate in
					** a block when you run out of slots
					** in the open file cache.
					*/
#define			DI_ENTRY_NUM	(DI_ALLOC_SIZE / sizeof(DI_FILE_DESC))
					/* number of slots that will fit in
					** a block of size DI_ALLOC_SIZE
					*/
#define			DI_HTB_BUCKET_NUM	61
					/* number of slots that will fit in
					** a block of size DI_ALLOC_SIZE
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
*/
struct _DI_FILE_DESC
{
    QUEUE	    fd_q;		/* queue link for hash table */
    QUEUE	    fd_lruq;		/* queue link for lru algorithm */
    DI_UNIQUE_ID    fd_uniq;		/* server unique id for file */
    i4 		    fd_refcnt;		/* reference count to this structure */
    STATUS	    fd_racestat;	/* Special status indicator */
					/* concurrent open case */
    CS_CONDITION    fd_open_cnd;	/* open in progres semaphore */
    HANDLE	    fd_fh;
};


/*
** Function Prototypes
*/


FUNC_EXTERN STATUS 	DIlru_close(
				DI_IO           *f,
				CL_ERR_DESC     *err_code );

FUNC_EXTERN STATUS	DIlru_free( CL_ERR_DESC     *err_code );

FUNC_EXTERN STATUS 	DIlru_init(
				CL_ERR_DESC 		*err_code);

FUNC_EXTERN STATUS 	DIlru_init_di( CL_ERR_DESC *err_code);

FUNC_EXTERN STATUS 	DIlru_open(
				DI_IO           *f,
				i4              flags,
				CL_ERR_DESC     *err_code );

FUNC_EXTERN STATUS      DIlru_inproc_open(
				DI_IO           *f,
				i4              create_flag,
				CL_ERR_DESC     *err_code );

FUNC_EXTERN STATUS      DIlru_flush(CL_ERR_DESC *err_code);


FUNC_EXTERN VOID        DIlru_set_di_error( 
				i4          *ret_status,
				CL_ERR_DESC *err_code,
				i4          intern_err,
				i4          meaningfull_err );

