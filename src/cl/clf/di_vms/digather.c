/*
**    Copyright (c) 1996, Ingres Corporation
*/
        
# include   <compat.h>
# include   <cs.h>
# include   <di.h>

/*
# include   "digather.h"
*/

/**
** Name: digather.c - Functions used to control DI gatherWrite I/O
**
** Description:
**
**      This file contains the following external routines:
**
**	DIgather_setting()      -  Return sizeof(GIO) if GW is enabled
**      DIgather_write()        -  Buffer up/write out write I/O requests
**
** History:
*/
/* Globals */
GLOBALREF       i4      Di_gather_io;   /* GLOBALDEF in dilru.c */

/*
**  Forward and/or External typedef/struct references.
*/

/*{
** Name: DIgather_write	- GatherWrite external interface.
**
** Description:
**	This routine is called by any thread that wants to write multiple pages 
**	The caller indicates via the op parameter whether the write request 
**	should be batched up or/and the batch list should be written to disc.
**
** Inputs:
**	i4 op 		- indicates what operation to perform:
**				DI_QUEUE_LISTIO - adds request to threads 
**						  gatherwrite queue.
**				DI_FORCE_LISTIO - causes do_writev() to be 
**						  called for queued GIOs.
**				DI_CHECK_LISTIO - checks if this thread has
**						  any outstanding I/O requests.
**      DI_IO *f        - Pointer to the DI file context needed to do I/O.
**      i4    *n        - Pointer to value indicating number of pages to  
**                        write.                                          
**      i4     page     - Value indicating page(s) to write.              
**      char *buf       - Pointer to page(s) to write.                    
**	(evcomp)()	- Ptr to callers completion handler.
**      PTR closure     - Ptr to closure details used by evcomp.
**                                                                           
** Outputs:                                                                  
**      f               - Updates the file control block.                 
**      n               - Pointer to value indicating number of pages written.
**      err_code        - Pointer to a variable used to return operating system
**                        errors.                                         
**
**      Returns:
**          OK                         	Function completed normally. 
**          non-OK status               Function completed abnormally
**					with a DI error number.
**
**	Exceptions:
**	    none
**
** Side Effects:
**      The completion handler (evcomp) will do unspecified work. 
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
**	19-Sep-2001 (kinte01)
**	    Borrowed from Unix version -- removed all of the OS threads
**	    info for the moment
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
STATUS
DIgather_write( i4 op, char *gio_p, DI_IO *f, i4 *n, i4 page, char *buf, 
  VOID (*evcomp)(), PTR closure, i4 *uqueued, u_i8 *gw_pages, u_i8 *io_count, 
  CL_ERR_DESC *err_code)
{
    STATUS	big_status = OK, small_status = OK;

    return( big_status ? big_status : small_status );
}

/*{
** Name: DIgather_setting -  returns the gather write setting.
**
** Description:
**	If Di_gather_io is TRUE, returns sizeof(DI_GIO),
**	otherwise returns zero.
**
** Inputs:
**	None.
**      
** Outputs:
**      None.
**
** Returns:
**      sizeof(DI_GIO) or zero.
** 
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
*/
i4
DIgather_setting()
{
	return(0);
}

