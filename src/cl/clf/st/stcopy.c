# include	<compat.h>
# include	<gl.h>
# include	<st.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
**
**	STcopy - Copy string.
**
**	Copy Null-terminated string to buffer.  Null terminate in buffer.
**
**	History:
**      02-apr-2001 (mcgem01)
**          File added back in for backward compatability.
*/
#ifdef STcopy
#undef STcopy
#endif 

VOID
STcopy(
	const char	*string,
	char	*buffer)
{
	strcpy(buffer,string);
}
