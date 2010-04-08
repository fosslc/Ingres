# include 	<compat.h>
# include 	<gl.h>
# include 	<lo.h>
# include 	<st.h>


/*
 *	Copyright (c) 1983 Ingres Corporation
 *
 *	Name: LOcopy
 *
 *	Function: LOcopy copies one location into another.
 *
 *	Arguments: 
 *		oldloc -- the location which is to be copied.
 *		newlocbuf -- buffer to be associated with new location.
 *		newloc -- the destination of the copy.
 *
 *	Result:
 *		newloc will contain copy of oldloc.
 *
 *	Side Effects:
 *		none.
 *
 *	History:
 *		4/4/83 -- (mmm) written
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing includes
*/
VOID
LOcopy(oldloc, newlocbuf, newloc)
register LOCATION	*oldloc, *newloc;
char			newlocbuf[];
{
	STcopy(oldloc->string, newlocbuf);
	LOfroms(oldloc->desc, newlocbuf, newloc);
}
