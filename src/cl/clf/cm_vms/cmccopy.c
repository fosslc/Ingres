/*
** Copyright (c) 2005, 2008 Ingres Corporation
*/
# include	<compat.h>
# include	<cm.h>

/**
** Name:	cmccopy.c	- character copy routine for DOUBLE_BYTE
**
** Description:
**	Contains an internal CL routine used to implement string copy for
**	a given number of characters.
**
*/

/*
** Name:	CMccopy		-  String copy by character (not bytes)
**
**  Description:
**	THIS IS AN INTERNAL CL ROUTINE.  IT IS USED TO IMPLEMENT CMCOPY
**	ON DOUBLE_BYTE IMPLEMENTATIONS.  IT CAN ONLY BE USED BY CL ROUTINES
**	NOT BY MAINLINE CODE.
**
**	CMcopy will copy (not necessarily null-terminated) character data
**	from a source buffer into a destination buffer of a given length.
**	CMccopy returns the number of charecters it copied.
**
**	This routine, which is system-independent, requires the user
**	to include the global header <cm.h>.
**
**	length = CMcopy(source, len, dest);
**
**  Inputs:
**	source    - Pointer to beginning of source buffer.
**	len	  - Number of characters to copy.
**	dest	  - Pointer to beginning of destination buffer.
**
**  Outputs:
**	Returns:
**	    Number of characters actually copied 
**
**  History:
**	28-Feb-2005 (hanje04)
**	    Created.
**	10-jun-2008 (joea)
**	    Ported from Unix version.
*/
SIZE_TYPE
CMccopy(
u_char *source,
SIZE_TYPE len,
u_char *dest)
{
    SIZE_TYPE	    copied;

    for (copied = 0 ; copied < len ; ++copied)
        CMcpyinc(source, dest);   /* copy the character */
   
    return copied;
}
