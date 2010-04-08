/*
**    Copyright (c) 1983, 2001 Ingres Corporation
*/
# include	<compat.h>  
# include	<gl.h>
# include	<er.h>  
# include	<st.h>  
# include	<lo.h>  
# include	"lolocal.h"


/*
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
 *		9/13/83 -- (dd) VMS CL
 *		9/28/89 -- (Mike S) Set up new LOCATION fields
 *		18-jun-93 (ed) made LOcopy a VOID as indicated in CL spec
 *
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**      01-feb-1994 (wolf) 
**          Prototype changed (in GLHDR), change type of "newlocbuf" to match.
**      09-sep-2000 (bolke01) 
**          Following change for B102291 the vax specific method was invalidated. (102565)
**          Note: (kinte01) bug 102565 was actually introduced by fix for bug
**              102372 & not bug 102291
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/
VOID
LOcopy(oldloc, newlocbuf, newloc)
register LOCATION	*oldloc, *newloc;
char			*newlocbuf;
{
	STcopy(oldloc->string, newlocbuf);
        LOfroms(oldloc->desc, newlocbuf, newloc);
}
