/******************************************************************************
**
** Copyright (c) 1987, Ingres Corporation
**
******************************************************************************/

#include   <compat.h>
#include   <cs.h>
#include   <di.h>

/* defines */

/* typedefs */

/* forward references */

/* externs */

/* statics */

/******************************************************************************
** Name: DIguarantee_space - Guarantee space is on disc.
**
** Description:
**	This routine will guarantee that once the call has completed
**	the space on disc is guaranteed to exist. The range of pages
**	must have already been reserved by DIalloc or DIgalloc the
**	result of calling this on pages not in this range is undefined.
**	
**   Notes:
**
**   a. On file systems which cannot pre-allocate space to a file this 
**      means that DIguarantee_space will be required to reserve the space 
**      by writing to the file in the specified range. On File systems which 
**      can preallocate space this routine will probably be a nop.
**
**   b. Currently the main use of this routine is in reduction of I/O when 
**      building tables on file systems which cannot preallocate space. See 
**      examples.
**
**   c. DI does not keep track of space in the file which has not yet
**      been guaranteed on disc this must be maintained by the client.
**
**   d. DI does not keep track of what pages have been either guaranteed on
**      disc or DIwritten and as this routine will overwrite whatever data may
**      or maynot already be in the requested palce the client has the ability 
**      to overwrite valid data.
**
**   e. The contents of the pages guaranteed by DIguarantee_space
**      are undefined until a DIwrite() to the page has happened. A DIread
**      of these pages will not fail but the contents of the page returned is
**      undefined.
**
** Inputs:
**      f               Pointer to the DI file context needed to do I/O.
**	start_pageno	Page number to start at.
**	number_pages	Number of pages to guarantee.
**
** Outputs:
**      err_code         Pointer to a variable used to return operating system 
**                       errors.
**    Returns:
**        OK
**				
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** Design Details:
**
**	On NT this is a nop as we both dialloc and digalloc prreallocate
**	space to the file.
** 
** History:
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
******************************************************************************/
STATUS
DIguarantee_space(DI_IO       *f,
                  i4          start_pageno,
                  i4          number_pages,
                  CL_ERR_DESC *err_code)
{
    err_code->errnum = OK;
    return( OK );
}
