/*
**  Allocate space for and set up defaults for a new window.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  newwin.c
**
**  History:
**	xx/xx/xx	-- Created.
**	12/03/86(KY) -- Added handling for WINDOW->_dx
**	25-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	01-oct-87 (bab)
**		TDmakenew sets the new WINDOW struct members _starty and
**		_startx to 0.  This setting is equivalent to the way
**		windows worked before.
**	05/04/90 (dkh) - Integrated changes from PC into 6.4 code line.
**	05/30/90 (dkh) - Fixed problems found on VMS.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<me.h>

# define	MAXUI2		0xffff	/* max size of unsigned 2 byte int */

# ifdef PMFE
# define	PMFE_MAX_ALLOC	0xfff0
# endif


WINDOW	*TDmakenew();


/*{
** Name:	IITD1waWinAlloc - Allocate array memory for a WINDOW.
**
** Description:
**	Allocates memory for the arrays used in a WINDOW.
**
** Inputs:
**	win		Pointer to WINDOW to allocate memory for.
**
** Outputs:
**
**	Returns:
**		OK	If memory successfully allocated.
**		FAIL	If memory not allocated.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	27-mar-90 (fraser)
**		First written.
**	05/04/90 (dkh) - Changed for integration into 6.4 code line.
*/
STATUS
IITD1waWinAlloc(win)
WINDOW	*win;
{
	reg	i4	i;
	reg	i4	nl;
	reg	i4	nc;
	reg	u_i4	total_sz;
	reg	u_i4	narray_sz;
	reg	u_i4	carray_sz;
	reg	char	*p;
	reg	char	**ap;

	nl = win->_maxy;
	nc = win->_maxx;

	narray_sz = nl * sizeof(i4);
	carray_sz = nl * sizeof(char *);

	total_sz =  (2 * narray_sz) + (4 * carray_sz);

	if ((p = (char *) MEreqmem((u_i4) 0, total_sz, (bool) FALSE,
		(STATUS *) NULL)) == NULL)
	{
		return(FAIL);
	}

	win->_y = (char **) p;
	p += carray_sz;

	win->_da = (char **) p;
	p += carray_sz;

	win->_dx = (char **) p;
	p += carray_sz;

	win->_freeptr = (char **) p;
	p += carray_sz;

	win->_firstch = (i4 *) p;
	p += narray_sz;

	win->_lastch = (i4 *) p;

	for (i = 0, ap = (char **)win->_freeptr; i < nl; i++)
	{
		*ap++ = NULL;
	}

	return(OK);
}


/*{
** Name:	IITD2waWinAlloc - Allocate cell memory for a WINDOW.
**
** Description:
**	This routine simply allocates the memory for each image plane
**	for a WINDOW.
**
** Inputs:
**	win		Pointer to WINDOW to allocate memory for.
**	first_newline	First line in WINDOW that needs new memory.
**
** Outputs:
**
**	Returns:
**		OK	Memory sucessfully allocated.
**		FAIL	Memory not allocated.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	27-mar-90 (fraser)
**		First writte.  Note that MEreqlng is used for the first
**		request.
**	05/04/90 (dkh) - Changed for integration into 6.4 code line.
*/
STATUS
IITD2waWinAlloc(win, first_newline)
WINDOW	*win;
i4	first_newline;
{
	reg	i4	i;
	reg	i4	nl;
	reg	i4	nc;
	reg	char	**y;
	reg	char	**da;
	reg	char	**dx;
	reg	char	*p;
	reg	char	*p1;
	reg	char	*p2;
	reg	char	**freep;
	reg	i4	j;
	reg	u_i4	linesize;
	u_i4	numofcells;

	nl = win->_maxy;
	nc = win->_maxx;

	numofcells = nc * (nl - first_newline);

	/*
	**  If we are able to grab all the memory we want in one
	**  shot, great!
	*/
# ifdef PMFE_MAX_ALLOC
	if (((3 * numofcells) <= (u_i4) PMFE_MAX_ALLOC) &&
		((p = (char *) MEreqmem((u_i4) 0, (SIZE_TYPE) (3 * numofcells),
		(bool) FALSE, (STATUS *) NULL)) != NULL))
# else
	if ((p = (char *) MEreqmem((u_i4) 0, (SIZE_TYPE) (3 * numofcells),
		(bool) FALSE, (STATUS *) NULL)) != NULL)
# endif /* PMFE_MAX_ALLOC */
	{
		/*
		**  Save away pointer so it can be later deleted.
		*/
		win->_freeptr[first_newline] = p;

		TDfill(numofcells, (u_char) ' ', p);
		TDfill(2 * numofcells, (u_char) EOS, p + numofcells);

		p1 = p + numofcells;
		p2 = p1 + numofcells;

		y = &(win->_y[first_newline]);
		da = &(win->_da[first_newline]);
		dx = &(win->_dx[first_newline]);

		for (i = first_newline; i < nl; i++)
		{
			*y++ = p;
			p += nc;
			*da++ = p1;
			p1 += nc;
			*dx++ = p2;
			p2 += nc;
		}
	}
	else
	{
		/*
		**  Otherwise, we will need to allocate it piece-meal.
		*/
		linesize = 3 * nc;
		freep = &(win->_freeptr[first_newline]);
		y = &(win->_y[first_newline]);
		da = &(win->_da[first_newline]);
		dx = &(win->_dx[first_newline]);
		for (i = first_newline; i < nl; i++)
		{
			if ((p = (char *)MEreqmem((u_i4) 0, linesize,
				(bool) FALSE, (STATUS *)NULL)) == NULL)
			{
				for (j = first_newline; j < i; j++)
				{
					MEfree((PTR) win->_freeptr[j]);
				}
				return(FAIL);
			}
			*freep++ = p;

			*y++ = p;
			TDfill((u_i4) nc, (u_char) ' ', p);
			p += nc;

			*da++ = p;
			TDfill((u_i4) nc, (u_char) EOS, p);
			p += nc;

			*dx++ = p;
			TDfill((u_i4) nc, (u_char) EOS, p);
		}
	}

	return(OK);
}



WINDOW *
TDnewwin(num_lines, num_cols, begy, begx)
i4	num_lines;
i4	num_cols;
i4	begy;
i4	begx;
{
	reg	WINDOW	*win;
	reg	i4	i;
	reg	i4	by;
	reg	i4	bx;
	reg	i4	nl;
	reg	i4	nc;
	reg	i4	halfsize;
	reg	i4	size;
		char	*bufptr;
	reg	char	*daptr;
	reg	char	*dxptr;

	by = begy;
	bx = begx;
	nl = num_lines;
	nc = num_cols;

	if (nl == 0)
		nl = LINES - by;

	if (nc == 0)
		nc = COLS - bx;

	if ((win = TDmakenew(nl, nc, by, bx)) == NULL)
		return((WINDOW *) NULL);

	if (IITD2waWinAlloc(win, 0) != OK)
	{
		IITD1dDelwin(win);
		MEfree((PTR)win);
		return((WINDOW *) NULL);
	}

	win->_parent = NULL;
	return(win);
}

WINDOW *
TDsubwin(aorig, num_lines, num_cols, begy, begx, swin)
WINDOW	*aorig;
i4	num_lines;
i4	num_cols;
i4	begy;
i4	begx;
WINDOW	*swin;
{
	reg	WINDOW	*orig = aorig;
	reg	i4	i;
	reg	WINDOW	*win;
	reg	i4	by;
	reg	i4	bx;
	reg	i4	nl;
	reg	i4	nc;
	reg	i4	j;
	reg	i4	k;

	by = begy;
	bx = begx;
	nl = num_lines;
	nc = num_cols;

	/*
	** make sure window fits inside the original one
	*/

	if (	by < orig->_begy 
	   ||	bx < orig->_begx
	   ||	by + nl > orig->_maxy + orig->_begy
	   ||	bx + nc > orig->_maxx + orig->_begx)
	{
		return((WINDOW *) NULL);
	}
	if (nl == 0)
	{
		nl = orig->_maxy - orig->_begy - by;
	}
	if (nc == 0)
	{
		nc = orig->_maxx - orig->_begx - bx;
	}
	if (swin == NULL)
	{
		if ((win = TDmakenew(nl, nc, by, bx)) == NULL)
		{
			return((WINDOW *) NULL);
		}
	}
	else
	{
		win = swin;
		win->_begy = by;
		win->_begx = bx;
		win->_maxy = nl;
		win->_maxx = nc;
		win->_cury = win->_curx = 0;
		for (i = 0; i < nl; i++)
		{
			win->_firstch[i] = win->_lastch[i] = _NOCHANGE;
		}
	}
	j = by - orig->_begy;
	k = bx - orig->_begx;
	for (i = 0; i < nl; i++)
	{
		win->_dx[i] = &orig->_dx[j][k];
		win->_da[i] = &orig->_da[j][k];
		win->_y[i] = &orig->_y[j++][k];
	}
	win->_flags |= _SUBWIN;
	win->_parent = orig;
	return(win);
}

/*
**	This routine sets up a window buffer and returns a pointer to it.
*/

WINDOW *
TDmakenew(num_lines, num_cols, begy, begx)
i4	num_lines;
i4	num_cols;
i4	begy;
i4	begx;
{

	reg	i4	i;
		WINDOW	*win;
	reg	i4	by;
	reg	i4	bx;
	reg	i4	nl;
	reg	i4	nc;
	reg	i4	yarraysz;
	reg	i4	chgarraysz;
	reg	i4	size;
		i4	*space;

	by = begy;
	bx = begx;
	nl = num_lines;
	nc = num_cols;

	if ((win = (WINDOW *)MEreqmem((u_i4)0, (u_i4)sizeof(WINDOW),
	    FALSE, (STATUS *)NULL)) == NULL)
	{
		return((WINDOW *) NULL);
	}

	/*
	**  Allocate memory for arrays used by window.
	*/
	win->_maxy = nl;
	win->_maxx = nc;
	if (IITD1waWinAlloc(win) != OK)
	{
		MEfree((PTR) win);
		return((WINDOW *) NULL);
	}

	win->_cury = win->_curx = 0;
	win->_clear = (nl == LINES && nc == COLS);
	win->_maxy = nl;
	win->_maxx = nc;
	win->_begy = by;
	win->_begx = bx;
	win->_starty = 0;
	win->_startx = 0;
	win->_flags = 0;
	win->_relative = TRUE;
	win->_scroll = win->lvcursor = FALSE;
	win->_parent = NULL;

	for (i = 0; i < nl; i++)
	{
		win->_firstch[i] = win->_lastch[i] = _NOCHANGE;
	}
	if (bx + nc == COLS)
	{
		win->_flags |= _ENDLINE;
		if (bx == 0 && nl == LINES && by == 0)
			win->_flags |= _FULLWIN;
		if (by + nl == LINES)
			win->_flags |= _SCROLLWIN;
	}

	return(win);
}





/*
**  Added for fix to BUG 5737.
**  This routine fills an arbitrary size buffer with
**  whatever you want using MEfill. (dkh)
*/

TDfill(size, with, ptr)
i4	size;
u_char	with;
char	*ptr;
{
	u_i2	fillsize;

	while (size > 0)
	{
		if (size > (i4) MAXUI2)
		{
			fillsize = (u_i2) MAXUI2;
		}
		else
		{
			fillsize = (u_i2) size;
		}
		MEfill(fillsize, with, ptr);
		ptr += fillsize;
		size -= fillsize;
	}
}
