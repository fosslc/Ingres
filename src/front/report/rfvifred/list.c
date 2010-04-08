/*
**  list.c
**  contains routines which implement lists
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	12/21/84 (drh)	Changed to MEtcalloc using list tag. Added linit
**			routine to initialize list.
**	01/16/85 (drh)	Changed MEtfree to FEfree and MEtcalloc to FEtcalloc.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	31-may-89 (bruceb)
**		linit() now only FEfree's if listtag not 0.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	"decls.h"
# include	<er.h>
# include	<ug.h>

# define	LISTSIZE	50

# ifdef VMDEBUG
extern	FILE	*FE_dbgfile;
# endif

static LIST	*freelist = LNIL;
static i2	listtag = 0;

LIST *
lalloc()
{
	register LIST	*lp;
	register i4	i;

	if ( listtag == 0 )
	{
		listtag = FEgettag();
	}
	
	if (freelist == LNIL)
	{
		freelist = (LIST *)FEreqmem((u_i4)listtag,
		    (u_i4)(LISTSIZE * sizeof(struct listType)),
		    TRUE, (STATUS *)NULL);
		if (freelist == LNIL)
			IIUGbmaBadMemoryAllocation(ERx("lalloc"));
		for (lp = freelist, i = 1; i < LISTSIZE; i++, lp++)
			lp->lt_next = lp + 1;
	}
	lp = freelist;
	freelist = lp->lt_next;
	lp->lt_next = LNIL;
	return(lp);
}

/*
** free up a list element
*/

VOID
lfree(lt)
register LIST	*lt;
{
	if (lt != LNIL)
	{
		MEfill(sizeof(LIST), '\0', (char *) lt);
		lt->lt_next = freelist;
		freelist = lt;
	}
}

/*
** Free all list elements, and initialize the list head and tag.
*/

VOID
linit()
{
    if (listtag)
    {
	FEfree( listtag );
	listtag = 0;
    }
    freelist = LNIL;
}
