/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<cs.h>
# include	<me.h>
# include	<st.h>

/*
**	STalloc - Allocate string.
**
**	Allocate permanent storage for the Null terminated string.
**
**	History:
**		2-15-83   - Written (jen)
**		24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
**		9/29/87 (daveb) -- Update to use nats.
**		26-aug-89 (rexl)
**		    Added calls to protect stream allocator.
**	        30-nov-92 (pearl)
**		    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped 
**		    function headers.
**		08-feb-93 (pearl)
**		    Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**	11-aug-93 (ed)
**	    unconditional prototypes
**	16-nov-95 (thoda04)
**	    Add #ifdef MCT around semaphore code.
**	    Correct cast of tag argument in MEreqmem call.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


char	*
STalloc(
	register char	*string)
{
	register size_t	len;
	char		*p;

	if(string == NULL)
		return (NULL);
	len = STlength(string) + 1;
	p = (char *) MEreqmem( (u_i2) 0, (u_i4)len, FALSE, 
		(STATUS *) NULL);
	if( p == NULL )
	{
		return (NULL);
	}
	STcopy(string, p);
	return (p);
}
