/*
**	Copyright (c) 2004 Ingres Corporation
*/
#ifndef DI_INCLUDED
#define DI_INCLUDED 1

#include    <qu.h>

/*
** Filename related symbolic constants.
** These have to be defined here so dicl.h can use them.
*/

# define                DI_PATH_MAX        128
# define                DI_FILENAME_MAX    36

#include    <dicl.h>

/**CL_SPEC
** Name:	DI.h	- Define DI function externs
**
** Specification:
**
** Description:
**	Contains DI function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	03-mar-1999 (walro03)
**	    Include qu.h for definition of QUEUE needed in dicl.h.
**	29-Jun-1999 (jenjo02)
**	    Added function prototypes for DIgather_write(), 
**	    DIgather_setting().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-Apr-2001 (jenjo02)
**	    Added gw_io, gw_pages to DIgather_write() prototype.
**	5-Nov-2009 (kschendel) SIR 122757
**	    Added DIget_direct_align.
**	25-Nov-2009 (kschendel) SIR 122747
**	    Move common DI definitions here to fix VMS compile problem.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
**/

/* masks for the DIopen() flags argument:
** SYNC: open the file in a mode such that writes are synchronous,
**	i.e. the write does not return until the data hits the drive.
** USER_SYNC:  open the file in a mode where the caller will force
**	data to the drive, using DIforce.  Individual writes need not
**	be synchronous, *unless* the platform has no useful DIforce
**	available.
** Note that if neither SYNC nor USER_SYNC is given, the file is opened
** in a mode where there are no guarantees about file data reaching
** the disk.  (as would be appropriate for temp or work files.)
**
** LOG_FILE:  hint that this file is the transaction log
** NOT_LRU:  hint that the file should remain open and not be
**	subject to file-descriptor LRU swapping.
** DIRECTIO:  open the file using "direct" I/O, which (if implemented)
**	allows I/O to bypass the filesystem cache (Unix).  Platforms
**	that have no direct I/O can just ignore the flag.
** PRIVATE:  says that the file is private to a single thread, and so
**	some concurrency issues are not relevant.  (For instance,
**	the file EOF in the DI_IO can always be believed, io_sem is
**	not needed, etc.)
*/

# define		DI_SYNC_MASK		0x001
# define		DI_USER_SYNC_MASK	0x002
# define		DI_LOG_FILE_MASK	0x004
# define		DI_NOT_LRU_MASK		0x008
# define		DI_DIRECTIO_MASK	0x010
# define		DI_PRIVATE_MASK		0x020

/* DI return status codes common to all platforms.
** Platforms may define additional return codes, but those additions are
** not allowed to be specifically meaningful to callers.
*/

# define                DI_OK           	0
# define                DI_BADPARAM     	(E_CL_MASK + E_DI_MASK + 0x01)
# define		DI_BADFILE		(E_CL_MASK + E_DI_MASK + 0x02)
# define                DI_ENDFILE      	(E_CL_MASK + E_DI_MASK + 0x03)
# define		DI_BADOPEN		(E_CL_MASK + E_DI_MASK + 0x04)
# define		DI_BADCLOSE		(E_CL_MASK + E_DI_MASK + 0x05)
# define		DI_BADWRITE		(E_CL_MASK + E_DI_MASK + 0x06)
# define		DI_BADREAD		(E_CL_MASK + E_DI_MASK + 0x07)
# define		DI_BADEXTEND		(E_CL_MASK + E_DI_MASK + 0x08)
# define		DI_BADDIR		(E_CL_MASK + E_DI_MASK + 0x09)
# define		DI_BADINFO		(E_CL_MASK + E_DI_MASK + 0x0A)
# define		DI_BADSENSE		(E_CL_MASK + E_DI_MASK + 0x0B)
# define		DI_BADCREATE		(E_CL_MASK + E_DI_MASK + 0x0C)
# define		DI_BADDELETE    	(E_CL_MASK + E_DI_MASK + 0x0D)
# define                DI_BADRNAME     	(E_CL_MASK + E_DI_MASK + 0x0E)
# define                DI_EXCEED_LIMIT 	(E_CL_MASK + E_DI_MASK + 0x0F)
# define                DI_DIRNOTFOUND  	(E_CL_MASK + E_DI_MASK + 0x10)
# define                DI_FILENOTFOUND 	(E_CL_MASK + E_DI_MASK + 0x11)
# define                DI_EXISTS       	(E_CL_MASK + E_DI_MASK + 0x12)
# define                DI_BADLIST 		(E_CL_MASK + E_DI_MASK + 0x13)
/*	codes 14 thru 1A are platform specific at present */
# define                DI_ACCESS               (E_CL_MASK + E_DI_MASK + 0x1B)
# define                DI_NODISKSPACE          (E_CL_MASK + E_DI_MASK + 0x1C)
/*	codes 1D thru 1F are open, higher numbers are platform specific
**	at present
*/

/*
** Related symbolic constants used for multiple locations in DIroutines.
**      To allow for multiple locations for one table (i.e.
**      table spread accross multiple disk drives) two new
**      constants have been added to DI.  DI_EXTENT_SIZE is
**      the number of pages to be written to each disk before
**      writing to the next disk.  For example if DI_EXTENT_SIZE
**      is four and there are three locations, the logical pages
**      0-24 would be allocated as follows:
**               Location 0  Pages 0,1,2,3,  12,13,24,15
**               Location 1  Pages 4,5,6,7,  16,17,18,19
**               Location 2  Pages 7,8,9,10, 11,12,13,14
**
**      The DI_EXTENT_SIZE must be a power of two to make the
**      calculations for determining where to write a logical
**      page simple.  There is an additional constant used
**      in these calculations which is log2 of the extent
**      size and is used in shifting.  The constant is
**      DI_LOG2_EXTENT_SIZE.
**
**	FIXME these two numbers have *nothing* to do with DI and should
**	be moved.  (probably to DMF.)
*/
# define                DI_EXTENT_SIZE     16
# define                DI_LOG2_EXTENT_SIZE 4

/*
** I/O direction flags for DIopen.
*/

# define                DI_IO_READ         0L	/* Read only */
# define                DI_IO_WRITE        1L	/* Read and write */

/*
** Operation flags for DIwrite_list()
*/
#define DI_QUEUE_LISTIO 0x01
#define DI_CHECK_LISTIO 0x02
#define DI_FORCE_LISTIO 0x04


/* Function prototypes */

FUNC_EXTERN STATUS  DIalloc(
			DI_IO         *f,
			i4	      n,
			i4	      *page,
			CL_SYS_ERR    *err_code
);

FUNC_EXTERN STATUS  DIclose(
			DI_IO          *f,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIcreate(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			char           *filename,
			u_i4          filelength,
			i4            pagesize,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIdelete(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			char           *filename,
			u_i4          filelength,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIdircreate(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			char           *dirname,
			u_i4          dirlength,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIdirdelete(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			char           *dirname,
			u_i4          dirlength,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS  DIflush(
			DI_IO          *f,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIforce(
			DI_IO          *f,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN u_i4 DIget_direct_align(void);

FUNC_EXTERN STATUS DIlistdir(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			STATUS         (*func)(),
			PTR            arg_list,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIlistfile(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			STATUS         (*func)(),
			PTR            arg_list,
			CL_SYS_ERR     *err_code
);
FUNC_EXTERN STATUS  DIopen(
			DI_IO          *f,
			char           *path,
			u_i4          pathlength,
			char           *filename,
			u_i4          filelength,
			i4            pagesize,
			i4            mode,
			u_i4      flags,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS  DIread(
			DI_IO	       *f,
			i4        *n,
			i4        page,
			char           *buf,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIrename(
			DI_IO	       *di_io_unused,
			char           *path,
			u_i4          pathlength,
			char           *oldfilename,
			u_i4          oldlength,
			char           *newfilename,
			u_i4          newlength,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS DIsense(
			DI_IO          *f,
			i4        *page,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS  DIwrite(
			DI_IO	       *f,
			i4            *n,
			i4        page,
			char           *buf,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS 	DIgalloc(
			DI_IO               *f,
			i4             n,
			i4             *page,
			CL_ERR_DESC         *err_code );



FUNC_EXTERN STATUS 	DIguarantee_space(
			DI_IO               *f,
			i4             start_pageno,
			i4             number_pages,
			CL_ERR_DESC         *err_code );

FUNC_EXTERN i4		DIgather_setting(void);

FUNC_EXTERN STATUS 	DIgather_write(
			i4		op,
			char		*gather,
			DI_IO		*f,
			i4		*n,
			i4		page,
			char		*buf,
			VOID		(*evcomp)(),
			PTR		closure,
			i4		*queued_count,
			u_i8		*gw_pages,
			u_i8		*gw_io,
			CL_ERR_DESC     *err_code );

#endif /* DI_INCLUDED */
