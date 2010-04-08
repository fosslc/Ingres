/******************************************************************************
**
** Copyright (c) 1987, 2003 Ingres Corporation
**
******************************************************************************/

#define    INCL_DOSERRORS

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   <me.h>
#include   <st.h>
#include   <winbase.h>
#include   "dilru.h"

/* # defines */

/* typedefs */

/* forward references */

/* externs */

GLOBALREF i4 Di_moinited;	/* in DIMO */

/* statics */

/******************************************************************************
**
** Name: DIcreate - Creates a new file.
**
** Description:
**      The DIcreate routine is used to create a direct access file.
**      Space does not need to be allocated at time of creation.
**      If it is more efficient for a host operating system to
**      allocate space at creation time, then space can be allocated.
**      The size of the page for the file is fixed at create time
**      and cannot be changed at open.
**
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      path                 Pointer to an area containing path name.
**      pathlength           Length of pathname.
**      filename             Pointer to an area containing file name.
**      filelength           Length of file name.
**      pagesize             Value indicating the size of the page
**                           in bytes.
**
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**          OK
**          DI_BADDIR        Error in path specification.
**          DI_BADCREATE     Error creating file.
**          DI_BADFILE       Bad file context.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_EXCEED_LIMIT  Too many open files.
**          DI_EXISTS        File already exists.
**          DI_DIRNOTFOUND   Path not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	10-jul-1995 (canor01)
**	    save the pathname and filename in the DI_IO structure for
**	    possible later use by DIdelete()
**      18-jul-1995 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**          the proper values.
**	18-jul-95 (emmag)
**	    Create with the FILE_FLAG_OVERLAPPED flag is required when
**	    using overlapped structures.
**	16-aug-95 (reijo01) #70642
**	    Create with a security_descriptor. Otherwise some files, in 
**	    particular journal files, were being created with permissions
** 	    that only allowed user system to update or read them.
**	30-aug-1995 (shero03)
**	    Call DImo_init.
**	26-mar-1996 (canor01)
**	    If file exists, error return can be ERROR_FILE_EXISTS.
**	24-feb-1997 (cohmi01)
**	    Return DI_EXCEED_LIMIT for additional limits that may be reached. (b80242)
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**	28-jan-1998 (canor01)
**	    Simplify the LRU code for NT by keeping all open files on a
**	    queue.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	24-jul-2000 (somsa01)
**	    cs_sid has been changed to cs_thread_id.
**	23-sep-2003 (somsa01)
**	    Initialize io_nt_gw_fh to INVALID_HANDLE_VALUE.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIcreate(DI_IO      *f,
         char       *path,
         u_i4       pathlength,
         char       *filename,
         u_i4       filelength,
         i4         pagesize,
         CL_SYS_ERR *err_code)
{
	STATUS ret_val = OK;
	CS_SCB	*scb;

	CSget_scb(&scb);

	/* default return */
	
	CLEAR_ERR(err_code);

	if (!Di_moinited)
	     DImo_init();

	/* Check for some bad parameters. */

	if (pathlength > DI_PATH_MAX ||
	    pathlength == 0 ||
	    filelength > DI_FILENAME_MAX ||
	    filelength == 0) {
		return (DI_BADPARAM);
	}

	/* Initialize file control block. */

	MEfill(sizeof(DI_IO), '\0', f);

	f->io_system_eof     = 0xffffffff;
	f->io_type           = DI_IO_ASCII_ID;
	f->io_bytes_per_page = pagesize;

    	/* save path and filename in DI_IO struct to cache open file descriptors. */

    	MEcopy((PTR) path, (u_i2) pathlength, (PTR) f->io_pathname);
    	f->io_l_pathname = pathlength;
    	MEcopy((PTR) filename, (u_i2) filelength, (PTR) f->io_filename);
    	f->io_l_filename = filelength;

	f->io_mode = DI_IO_WRITE;

#ifdef  xDEV_TST
    TRdisplay("DIcreate: %t/%t\n", f->io_l_pathname, f->io_pathname,
              f->io_l_filename, f->io_filename);
#endif
 
    CS_synch_init( &f->io_sem );

    CS_synch_lock( &f->io_sem );

    QUinit((QUEUE *)f);
 
    do
    {
  	f->io_nt_fh = f->io_nt_gw_fh = INVALID_HANDLE_VALUE;
 
        /*
        ** open this file, closing another DI file if necessary (ie. if we
        ** have run out of open file descriptors.)
        ** NOTE: We used to pass in the DI_NORETRY flag anyway have taken
        ** this out.
        */
	if ( f->io_nt_fh == INVALID_HANDLE_VALUE )
            ret_val = DIlru_open(f, DI_FCREATE, err_code);
        if ( ret_val != OK )
            break;
 
        ret_val = DIlru_close(f, err_code);
        if ( ret_val != OK )
            break;
 
    } while (FALSE);

    CS_synch_unlock( &f->io_sem );

    /* invalidate this file pointer */
    f->io_type = DI_IO_CLOSE_INVALID;

    CS_synch_destroy( &f->io_sem );
 
    return(ret_val);

}
