/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>

/**
** Name:	cmicopy.c	- copy routine for DOUBLE_BYTE implementations.
**
** Description:
**	Contains an internal CL routine used to implement CMcopy on DOUBLE_BYTE
**	implementations.
**
*/

/*
** Name:	cmicopy		- Implements CMcopy
**
**  Description:
**	THIS IS AN INTERNAL CL ROUTINE.  IT IS USED TO IMPLEMENT CMCOPY
**	ON DOUBLE_BYTE IMPLEMENTATIONS.  IT CAN ONLY BE USED BY CL ROUTINES
**	NOT BY MAINLINE CODE.
**
**	CMcopy will copy (not necessarily null-terminated) character data
**	from a source buffer into a destination buffer of a given length.
**	Like STlcopy, CMcopy returns the number of bytes it copied
**	(which may differ from the number of bytes requested to be copied
**	if the last byte was double byte and could not be completely copied).
**
**	This routine, which is a system-independent macro, requires the user
**	to include the global header <cm.h>.  Also note that because some of 
**	the macro expansions may use copy_len twice, you should be aware of
**	possible side effects (ie, ++ and -- operators).
**
**	length = CMcopy(source, copy_len, dest);
**
**  Inputs:
**	source    - Pointer to beginning of source buffer.
**	copy_len  - Number of bytes to copy.
**	dest	  - Pointer to beginning of destination buffer.
**
**  Outputs:
**	Returns:
**	    Number of bytes actually copied (this may be less than copy_len
**	    in the case of double-byte characters, but will never be more).
**
**  History:
**	20-apr-1987	- Written (joe)
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
*/
u_i4
cmicopy(source, len, dest)
u_char	*source;
u_i4	len;
u_char	*dest;
{
    u_i4	    rlen;

    rlen = 0;
    for (;;)
    {
        /*
        ** Find out how many bytes to copy for
        ** this character, but don't change
        ** what source points to.
        */
	CMbyteinc(rlen,source);
	/*
 	** See if there is enough room
	*/
	if (rlen <= len)
	{
	    CMcpyinc(source, dest);   /* copy the character */
	}
	else
	{
	    /*
 	    ** Last character won't fit, so
	    ** decrement counter by size of character
	    ** and break out of loop.
	    */
	    CMbytedec(rlen, source);
	    break;
        }
    }
    return(rlen);
}
