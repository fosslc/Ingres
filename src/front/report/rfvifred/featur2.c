/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**  featur2.c
**
**  contains:
**	nextFeat()
**	prevFeat()
**
**  history:
**	1/28/85 (peterk) - {next,prev}Feat split off from feature.c
**	11/23/85 (dkh) - Fix for BUG 6017. (dkh)
**	26-jun-89 (bruceb)	Fix for jupiter bug 6771.
**		Fixed 'previous' feature; cursor now moves to prior
**		features on the same line as starting object.
**		Fixed 'next' feature; if cursor starts out not on an
**		object, goes to (by preference) an object that starts
**		to the right or below the start point.
**	19-apr-90 (bruceb)	Fix for bug 21294.
**		Removed use of lowLimit, upLimit.  Use 0, endFrame instead.
**		RBF no longer restricts cursor to current section.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"

/*
** Find the next feature from the current positon
**
**   (Note: This function is called from the VTcursor routine
**	which violates the strict VT protocol..)
**
** The definition of next feature depends whether the cursor is currently 
** on a feature or not.
**
** If on a feature then the next feature is the leftmost feature that
** is to the right of the current feature yet starts on the same line
**  as the current feature , or the leftmost feature on the lines below.
** 
** If not on a feature, then it is the first feature bumped into
** when traveling left to right top to bottom.
**
** In any case boxes are not to be landed on by these functions.
**
*/

POS *
nextFeat()
{
	register VFNODE	*lt,
			**lp;
	register POS	*ps;
	register i4	y;
	i4		x;
	
	y = globy;
	x = globx;
	lp = &line[y];
	for (lt = *lp; lt != NNIL; lt = vflnNext(lt, y))
	{
		ps = lt->nd_pos;
		if ((ps->ps_name != PBOX) && (ps->ps_begx > x)
			&& (ps->ps_begy == y))
		{
			return(ps);
		}
	}

	/*
	**  If we are on the End-of-Form marker, set it back
	**  so the rest of the code works.
	*/
	if (y == endFrame)
	{
		y--;
	}
	for (;;)
	{
		if (y == endFrame - 1)
		{
			y = 0;
			lp = &line[y];
		}
		else
		{
			y++;
			lp++;
		}
    		if (inLimit(y))
		{
    			continue;
		}
		for (lt = *lp; lt != NNIL; lt = vflnNext(lt, y))
		{
			ps = lt->nd_pos;
			if ((ps->ps_name != PBOX) && (y == ps->ps_begy))
			{
				return(ps);
			}
		}
	}
}

/*
** the previous feature
** is meant to be the opposite of the next feature
*/

POS *
prevFeat()
{
	register VFNODE	*lt,
			**lp;
	register i4	y;
	i4		x;
	register POS	*ps;
	POS		*ops;
	bool		onPs = FALSE;


	y = globy;
	x = globx;

	ps = onPos(y, x);

	if (ps && ps->ps_name == PBOX)
		ps = NULL;

	if (ps != NULL)
	{
		/* Place x,y at top left of feature. */
		y = ps->ps_begy;
		x = ps->ps_begx;
		onPs = TRUE;
	}

	ps = NULL;
	ops = NULL;
	lp = &line[y];

	/* search forward through the current line */
	for (lt = *lp; lt != NNIL; lt = vflnNext(lt, y))
	{
		ps = lt->nd_pos;
		/*
		** If no longer 'previous' to the starting object, then if
		** already identified an object on the line to the left of
		** starting object, return that object.  Otherwise, start
		** looking for objects on prior lines of the form.
		*/
		if (ps->ps_begx >= x)
		{
			if (ops != NULL)
			{
				return(ops);
			}
			break;
		}

		if (ps->ps_begy == y && ps->ps_name != PBOX)
		{
			ops = ps;	/* remember this possible previous. */
		}
	}		
	for (;;)
	{
		if (y == 0)
		{
			y = endFrame - 1;
			lp = &line[y];
		}
		else
		{
			y--;
			lp--;
		}
    		if (inLimit(y))
		{
    			continue;
		}
		ops = NULL;
		ps = NULL;
		for (lt = *lp; lt != NNIL; lt = vflnNext(lt, y))
		{
			ps = lt->nd_pos;
			if ((!onPs || y == ps->ps_begy) && ps->ps_name != PBOX)
				ops = ps;
		}
		if (ops != NULL)
		{
			return(ops);
		}
	}
}
