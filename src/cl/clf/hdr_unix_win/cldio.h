/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: CLDIO.H - Used for the CL dio abstraction.
**
** Description:
**	Prototypes for the CL dio abstraction.
**
**	30-November-1992 (rmuth)
**	    Created.
**	05-Nov-1997 (hanch04)
**	    Added large file system defines
**	21-jul-1997 (canor01)
**          Add DIFILE_FD macro to limit DI file descriptors to numbers
**          above those used by stream IO (parallels CLPOLL_FD macro in
**          clpoll.h c.f.).
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	08-Jun-1999 (jenjo02)
**	    Added IIdio_writev() prototype.
**      28-May-2002 (hanal04) Bug 107686 INGSRV 1764
**          Moved prototype for dodirect() from cldio.c so that
**          DIalloc() can use it to determine whether we need to
**          redirect the allocation request to DIgalloc().
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	9-Nov-2009 (kschendel) SIR 122757
**	    Add prototypes for fallocate reservations.  Remove linux
**	    directio-realign-reallocate stuff.
**/

/*
**  Forward and/or External typedef/struct references.
*/
FUNC_EXTERN	int 	IIdio_open(
				char *file,
				int mode,
				int perms,
				int pagesize,
				u_i4 *fprop,
				CL_ERR_DESC *err);

FUNC_EXTERN 	int 	IIdio_write(
				int fd,
				char *buf,
				int len,
				OFFSET_TYPE off,
				OFFSET_TYPE *loffp,
				u_i4 fprop,
				CL_ERR_DESC *err);

FUNC_EXTERN 	int 	IIdio_writev(
				int fd,
				char *iov,
				int iovcnt,
				OFFSET_TYPE off,
				OFFSET_TYPE *loffp,
				u_i4 fprop,
				CL_ERR_DESC *err);

FUNC_EXTERN 	int 	IIdio_read(
				int fd,
				char *buf,
				int len,
				OFFSET_TYPE off,
				OFFSET_TYPE *loffp,
				u_i4 fprop,
				CL_ERR_DESC *err);

FUNC_EXTERN	OFFSET_TYPE IIdio_get_file_eof(
				i4   fd,
				u_i4 fprop);

FUNC_EXTERN	STATUS IIdio_get_reserved(
				i4 fd,
				i8 *reserved,
				CL_ERR_DESC *err);

FUNC_EXTERN	STATUS IIdio_reserve(
				i4 fd,
				i8 offset,
				i8 nbytes,
				CL_ERR_DESC *err);

/*
** Name: FPROP_* - macros for file properties
**
** Description:
**    A collection of macros and function definitions to enable
**    per file properties.
**
**    Properties include:
**	"soft" allocation strategy
**      RAW access is being done on the file
**      O_DIRECT access is being done on the file
**	File has been opened at least once
**	File is private to a thread
**
** 01-apr-2004 (fanch01)
**    Created.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	    Macro names, defines changed from "FDPR_" to "FPROP_"
**	    to reflect this distinction.
**	    DIRECTIO is now requested on individual files
**	    via DIopen(DI_DIRECTIO_MASK), rather than defaulting
**	    to "all" files.
**	9-Nov-2009 (kschendel) SIR 122757
**	    Add "reservations" strategy.  Remove alignment stuff, no longer
**	    needed.  Remove macros for simple bit test or set.
*/

/* file descriptor properties associated with files */

/* File has been opened once and the underlying filesystem properties
** have been determined.  No need to re-determine them if the fd is
** lru'ed out and the file is reopened later.
*/
#define FPROP_OPEN 0x80000000

/* raw flag */
#define FPROP_RAW 0x40000000

/* Direct IO should be enabled. */
#define FPROP_DIRECT 0x20000000

/* Direct IO is requested by DIopen.  (It may not be possible to honor
** the request, which is why directio and directio-request are separate.)
*/
#define FPROP_DIRECT_REQ 0x10000000

/* File is private to a thread, not sharable or accessed concurrently.
** Primary implications: don't need io-sem to protect fd positioning;
** don't need to worry about anyone else extending file.
*/
#define FPROP_PRIVATE 0x08000000

/* Bits reserved for allocation strategy: */
#define FPROP_ALLOCSTRATEGY_MASK 0x00000f00

/* Define "soft" allocation strategies.
** DI always allocates space before reads or writes to that space are
** allowed.  There is "soft" allocation, via DIalloc, which may not
** guarantee that the space is really there and readable, but does allow
** writing to the space.  Then there is "hard" allocation, which is a
** persistent allocation that can be read or written; this is DIgalloc.
** Hard allocated pages that haven't been written must be read as zero.
** (On most/all filesystems this means that real zeros have to be written.)
**
** The currently available soft allocation strategies are:
**
** VIRT - virtual, let later writes extend the file
**	this is a "do nothing" strategy that lets actual writes grow the file.
** TRNC - ftruncate() can grow the file without writing or leaving holes
** EXTD - extend with lseek beyond EOF and write one page.
**	this would be for an FS that allocates space instead of leaving
**	holes when seeking beyond EOF
** RESV  - handles EOF like VIRT, ie does nothing special.
**	But, attempts to reserve contiguous space on disk using the
**	(linux specific) fallocate() system call.  This syscall reserves
**	persistent space on disk, but does not move EOF.
*/

#define FPROP_ALLOCSTRATEGY_VIRT 0
#define FPROP_ALLOCSTRATEGY_TRNC 1
#define FPROP_ALLOCSTRATEGY_EXTD 2
#define FPROP_ALLOCSTRATEGY_RESV 3

/* flag and value manipulation */
#define FPROP_CLEAR(fprop) (fprop = 0)
#define FPROP_COPY(s_fprop,t_fprop) (t_fprop = s_fprop)
#define FPROP_ALLOCSTRATEGY_SET(fprop, s) ((fprop) |= ((s & 0x3) << 8))
#define FPROP_ALLOCSTRATEGY_GET(fprop) (((fprop) >> 8) & 0x3)

/*
** Name: DIFILE_FD() - macros for renumbering fds for use with DI.
**
** Description:
**	Certain implementations of the CL allow both SI and DI 
**	functions to be mixed within the same server process.  SI
**	is built on stream i/o functions that are limited to use
**	of file descriptors between 0 and 255 inclusive.  To 
**	reserve as many of these file descriptors for use by SI,
**	DI file descriptors should be allocated >255.
**
**      To ensure low numbered fds are available we follow all DI calls
**      which allocate a fd with a call to the DIFILE_FD() macro.
**
**      A sample use is:
**              fd = open(...);
**              DIFILE_FD( fd );
**
**      This macro dups the fd to one above STREAM_RESVFD if the fd is
**      below STREAM_RESVFD, closing the old fd and setting fd to the
**      new one.
**
** History:
**	21-jul-1997 (canor01)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# if defined(xCL_RESERVE_STREAM_FDS)

static int IIdio_file_fd( int fd );

# define STREAM_RESVFD  255

# define        DIFILE_FD( fd ) ( fd = IIdio_file_fd( fd ) )

# else /* xCL_RESERVE_STREAM_FDS */

# define        DIFILE_FD( fd )

# endif /* xCL_RESERVE_STREAM_FDS */
