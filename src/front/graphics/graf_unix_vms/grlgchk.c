/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<graf.h>

/**
** Name:	grlgchk.c -	Graphics System Legend Association Checker.
**
** Description:
**	Defines:
**
**	GRlgchk()	check graph chart legend association.
**
** History:
**/


/*{
** Name:	GRlgchk() -		Check Graph Chart Legend Association.
**
** Description:
**	utility to make sure legend is associated right.  The "one-chart /
**	one-legend" assumption is being made here.
**
** Inputs:
**	frame		The graph chart frame structure.
**
** Outputs:
**	none
**
** History:
**	12/85 (rlm)	written
*/

GRlgchk (frame)
GR_FRAME *frame;
{
	register GR_OBJ *ptr,*cptr,*loptr;
	register LEGEND *lptr;

	lptr = NULL;
	cptr = NULL;
	for (ptr = frame->head; ptr != NULL; ptr = ptr->next)
	{
		ptr->legend = NULL;
		switch (ptr->type)
		{
		case GROB_LEG:
			lptr = (LEGEND *) (loptr = ptr)->attr;
			break;
		case GROB_BAR:
		case GROB_SCAT:
		case GROB_LINE:
			cptr = ptr;
			break;
		default:
			break;
		}
	}

	if (lptr != NULL)
	{
		if (cptr != NULL)
		{
			cptr->legend = lptr;
			lptr->assoc = cptr;
		}
		else
			lptr->assoc = NULL;
		lptr->obj = loptr;
	}
}
