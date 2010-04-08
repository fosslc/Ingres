# include	<compat.h>
# include	<gl.h>
# include	<me.h>

/*
**
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		MEmove.c
**
**	Function:
**		MEcopy
**
**	Arguments:
**		i2	source_length;
**		char	*source;
**		c1	pad_char;
**		i2	dest_length;
**		char	*dest;
**
**	Result:
**
** 
** History:
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	14-sep-1995 (sweeney)
**	    Purge obsolete RCS history and VAX assembler.
*/

/* lint thinks these arguments are unused */

/*ARGSUSED*/
VOID
MEmove(
	u_i2	source_length,
	PTR	source,
	char	pad_char,
	u_i2	dest_length,
	PTR	dest)
{
	register u_i4  index;

	if (source_length < dest_length)
	{
		for(index = source_length; index; index--)
			*(char *)dest++ = *(char *)source++;
		for (++source_length; source_length <= dest_length; source_length++)
			*(char *)dest++ = pad_char;
	}
	else
		for (; dest_length; dest_length--)
			*(char *)dest++ = *(char *)source++;
}
