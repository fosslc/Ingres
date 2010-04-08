/*
**	Copyright (c) 2004 Ingres Corporation
*/

/* # include's */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	afpad.c -	Determine padding for byte alignment.
**
** Description:
**	This file contains and defines:
**
** 	afe_pad - Determine padding for byte alignment.
**
** History:
**	Written	2/6/87	(dpr)
**      01-dec-93 (smc)
**	    Bug #58882
**          Commented lint pointer truncation warning.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	afe_pad - Determine padding for byte alignment.
**
** Description:
**	This routine determines how much padding is necessary to place
**	data for the given datatype at the given address. This is necessary
**	for byte alignment issues when loading up buffers to hold results
**	from database queries. Given the address where we wish to place the 
**	data, and the datatype of the data, afe_pad will return an offset.
**	This offset is the number of bytes of padding to add to the starting
**	address for proper byte alignment. 
**
** Inputs:
**	cb		pointer to the ADF_CB
**	
**	address		starting address of the data block in question.
**	
**	datatype	the DB_DT_ID of the data to be added.
**
**	offset		a pointer to a nat.
**
** Outputs:
**	offset		filled in with the the number of bytes to skip.
**
**	Returns:
**		OK
**
** History:
**	Written	2/6/87	(dpr)
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
i4
afe_pad(cb, address, datatype, offset)
ADF_CB	    *cb;
PTR	    address;
DB_DT_ID    datatype;
i4	    *offset;
{
    PTR aligned;

    *offset = 0;
    aligned = ME_ALIGN_MACRO(address, sizeof(ALIGN_RESTRICT));
    if (aligned != address)
	*offset = (i4) (aligned - address);
    return OK;
}
