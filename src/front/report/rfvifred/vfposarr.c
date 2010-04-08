/*
** vfposarr.c
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	2/12/85 (peterk) - split off from cursor.c
**	14-oct-86 (bab)	fix for bug 10369
**		handle increasing array size as forms are
**		created with large numbers of display objects.
**		(neccesitated by forms with > 256 objects)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	18-oct-88 (sylviap)	
**		Changed INIT_POSSZ to DB_GW2_MAX_COLS +1.
**      17-apr-90 (esd)
**              Don't put non-editable items into tab stop table;
**              according to Dave Hung, these are the ones with
**              ps_name == PSPECIAL.
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
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<vt.h>
# include	<me.h>

/* max number of objects on form starts at INIT_POSSZ, doubles each time */
# define	INIT_POSSZ	DB_MAX_COLS + 1

static	i4	posarrsz	= INIT_POSSZ;

static	vfincr_posarr();


VOID
vfposarray(poscount, array)
i4	*poscount;
FLD_POS	**array;
{
	reg	VFNODE	**lp;
	reg	VFNODE	*lt;
	reg	i4	i;
	reg	i4	count = 0;
	reg	FLD_POS	*ptr;

	if (*array == NULL)	/* then, allocate the array */
	{
		*array = (FLD_POS *)MEreqmem((u_i4)0,
		    (u_i4)(sizeof(FLD_POS) * posarrsz), FALSE, (STATUS *)NULL);
	}

	ptr = *array;
	lp = line;
	for (i = 0; i < endFrame; i++, lp++)
	{
		for (lt = *lp; lt != NNIL; lt = vflnNext(lt, i))
		{
			if ((lt->nd_pos->ps_begy == i)
			    && (lt->nd_pos->ps_name != PSPECIAL))
			{
				/* if array too small */
				if (count >= posarrsz)
				{
					/* extend the array */
					vfincr_posarr(array);
					ptr = &(*array)[count];
				}
				ptr->posy = i;
				ptr->posx = lt->nd_pos->ps_begx;
				ptr++;
				count++;
			}
		}
	}
	*poscount = count;	
}


static
vfincr_posarr(array)
FLD_POS	**array;
{
	FLD_POS	*new_arr;
	i4	newsize = posarrsz * 2;
	i4	i;

	/* allocate array twice as large */
	new_arr = (FLD_POS *)MEreqmem((u_i4)0,
	    (u_i4)(sizeof(FLD_POS) * newsize), FALSE, (STATUS *)NULL);
	/* transfer old array contents to new array */
	for (i = 0; i < posarrsz; i++)
	{
		new_arr[i].posy = (*array)[i].posy;
		new_arr[i].posx = (*array)[i].posx;
	}
	MEfree(*array);		/* free up old array's memory */
	*array = new_arr;	/* and reset old array pointer */
	posarrsz = newsize;
}
