/*
** delete.c
**
** contains the routines to delete lines and features
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	08/14/87 (dkh) - ER changes.
**	10/25/87 (dkh) - Error message cleanup.
**	07-apr-89 (bruceb)
**		Modified delFeat() to reject request if the field is
**		restricted (locked by VQ).
**	01-sep-89 (cmr)- added new RBF routine delnLines() and added 
**			 del_ok flag to delFeat() so that special
**			 lines may be deleted in RBF (TRUE when called
**			 from delnLines().
**	16-oct-89 (cmr)- changed call interface of delnLines().  Also
**			 increment move level for each line deleted so
**			 vfUpdate() will update screen appropriately.
**	24-jan-90 (cmr)	 Get rid of calls to newLimits(); superseded by
**			 one call to SectionsUpdate() by vfUpdate().
**	26-feb-90 (cmr)	 RBF -- Make sure not to delete all fields when 
**			 deleting n lines (delnLines);  must have at
**			 least one field to report on.
**	19-apr-90 (cmr)	 RBF - if we are deleting an aggregate remove it
**			 from the copt agu_list if it is unique.
**	01-jun-90 (cmr)	 RBF - when checking for the last detail component(s)
**			 include aggregates as well as fields.
**	6/6/90 (martym)  RBF - Added call to Reset_copt().
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**	21-jul-90 (cmr)	 fix for bug 31759
**			 delnLines - do resetMoveLevel and clearLine here 
**			 instead of in the main LAYOUT loop
**			 (in case a Cancel or End is done w/o any changes).
**	03-aug-90 (cmr)  fix for bug 32080 and 32083
**			 Change to previous fix (31759).  Check flag before
**			 doing initialization (should only happen once per
**			 Layout operation not everytime this routine is called.
**	06/28/92 (dkh) - Added support for rulers in vifred/rbf.
**	05/19/93 (dkh) - Fixed bug 50791.  The layout handling code will
**			 now correctly handle straightedges when deleting
**			 a section.
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
# include	"vfunique.h"
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"


FUNC_EXTERN	void	vfdelnm();
FUNC_EXTERN	void	IIVFcross_hair();
GLOBALREF	i4	vfrealx;

i4
delCom()
{
	register POS	*ps;


	if (vfrealx == endxFrm + 1 + IIVFsfaSpaceForAttr)
	{
		IIUGerr(E_VF0035_Can_not_delete_right, UG_ERR_ERROR, 0);
		return;
	}
	if (globy == endFrame)
	{
		IIUGerr(E_VF0036_Cant_delete_the_last, UG_ERR_ERROR, 0);
		return;
	}
	ps = onPos(globy, globx);
	if (ps != NULL)
		delFeat(ps, TRUE, FALSE);
	else
		delLine(globy);
}
#ifdef FORRBF
STATUS
delnLines( y, numlines, initialized )
i4	y, numlines;
bool	initialized;
{
	VFNODE	*p, *q;
	FIELD	*fd;
	POS	*ps;
	int	i;
	bool	delete_all = TRUE;

	/*
	** Don't delete n lines if doing so would cause the report to be
	** empty; make sure that at least one field (or aggregate) exists
	** outside of this section.
	*/
	for ( i = 0; i < endFrame && delete_all; i++ )
	{
		for ( p = line[i]; p; p = vflnNext(p,i) )
		{
			ps = p->nd_pos;
			if ( ps->ps_name != PFIELD )
				continue;
			fd = (FIELD *)ps->ps_feat;
			if ( ps->ps_begy < y || ps->ps_begy > y + numlines )
			{
				delete_all = FALSE;
				break;
			}
		}
	}
	if ( delete_all )
	{
		IIUGerr(E_VF0163_Cant_delete_the_last, UG_ERR_ERROR, 0);
		return(FAIL);
 	}

	if (!initialized)
	{
		resetMoveLevel();
		clearLine();
	}

	/*
	**  Drop both cross hairs if they are being displayed.
	**  The vertical one is dropped since we don't want it
	**  to get in the way of the deletes and we need to
	**  resize it anyway.  The horizontal one is being dropped
	**  since we really don't know where it may end up once
	**  a section has been dropped.  So we always move the
	**  horizontal cross hair to the detail section when
	**  a section is deleted.  The detail section is the
	**  only one that is guaranteed to always be present.
	**
	**  Note that this routine is only used by RBF's delete
	**  section code and we assume that in what we are doing here.
	**  If this assumption changes, the code below will probably
	**  have to change.
	*/
	if (IIVFxh_disp)
	{
		vfersPos(IIVFhcross);
		unLinkPos(IIVFhcross);

		vfersPos(IIVFvcross);
		unLinkPos(IIVFvcross);
	}

	while ( numlines-- )
	{
		p = line[y];
		while ( p )
		{
			q = p;
			p = vflnNext( p, y );
			ps = q->nd_pos;
			delFeat(ps, TRUE, TRUE);
		}
		nextMoveLevel();
		doDelLine(y);

		if (globy >= endFrame)
			globy = endFrame - 1;
	}

	/*
	**  We depend on the RBF delete section code to call
	**  IIVFdscDelSecCleanup() to put back the cross hairs,
	**  if necessary.
	*/

	return(OK);
}


/*{
** Name:	IIVFdscDelSecCleanup - Cleanup things when deleting a section.
**
** Description:
**	Do cleanup when a rbf section has been deleted.  At the moment,
**	the only cleanup is with re-displaying any straightedges.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/19/93 (dkh) - Initial version.
*/
void
IIVFdscDelSecCleanup()
{
	if (IIVFxh_disp)
	{
		IIVFcross_hair();
	}
}

# endif
/*
** delete a feature by erasing it from the screen
*/
VOID
delFeat(ps, noUndo, del_ok)
register POS	*ps;
bool		noUndo;
bool		del_ok;
{
	FLDHDR		*hdr;
	FIELD		*fd;

	switch (ps->ps_name)
	{
	case PTRIM:
		if (noUndo)
		{
			vfersPos(ps);		/* erase from screen */ 
			unLinkPos(ps);		/* remove from line tab */
			setGlMove(ps);
			setGlobUn(dlTRIM, ps, NULL);
		}
		vffrmcount(ps, -1);	/* decrement counter in frame struct */
		if (ps->ps_column != NULL)
			delColumn(ps->ps_column, ps);
		break;

	case PBOX:
		if (ps->ps_attr == IS_STRAIGHT_EDGE)
		{
			IIUGerr(E_VF016A_cant_del_x_hair, UG_ERR_ERROR, 0);
			break;
		}
		if (noUndo)
		{
			vfersPos(ps);
			unLinkPos(ps);
			setGlMove(ps);
			setGlobUn(dlBOX, ps, NULL);
		}
		break;
# ifndef FORRBF
	case PTABLE:
		fd = (FIELD *) ps->ps_feat;
		hdr = &(fd->fld_var.fltblfld->tfhdr);
		if (hdr->fhd2flags & fdVQLOCK)
		{
		    IIUGerr(E_VF0123_field_restricted, UG_ERR_ERROR, 0);
		    return;
		}

		if (noUndo)
		{
			vfersPos(ps);
			unLinkPos(ps);
			setGlMove(ps);
			setGlobUn(dlTABLE, ps, NULL);
		}
		vffrmcount(ps, -1);

		vfdelnm(hdr->fhdname, AFIELD);
		break;
# endif

	case PFIELD:
		fd = (FIELD *) ps->ps_feat;
		hdr = FDgethdr(fd);
		if (hdr->fhd2flags & fdVQLOCK)
		{
		    IIUGerr(E_VF0123_field_restricted, UG_ERR_ERROR, 0);
		    return;
		}

		/*
		** if a field is deleted which has some associated
		** trim, then we must also delete the trim
		*/
# ifdef FORRBF
		/* Do not allow the user to delete the last detail component */
		if (frame->frfldno == 1)
		{
			IIUGerr(E_VF0037_Cant_delete_the_last, UG_ERR_ERROR, 0);
			return;
		}
		/*
		** Remove_agu removes this agg fld from the copt 
		** agu_list if it is unique.
		*/
		if (hdr->fhd2flags & fdDERIVED)
			Remove_agu(fd);
		else
			Reset_copt(fd);
# endif
		if (ps->ps_column != NULL && noUndo)
		{
			setGlMove(ps);
			setGlobUn( dlCOLUMN, ps, NULL);
			unLinkCol(ps);
			ersCol(ps);
		}
		else if (noUndo)
		{
			vfersPos(ps);
			unLinkPos(ps);
			setGlMove(ps);
			setGlobUn(dlFIELD, ps, NULL);

			vfdelnm(hdr->fhdname, AFIELD);
		}
		if (ps->ps_column != NULL)
			vfcolcount(ps, -1);
		else
			vffrmcount(ps, -1);
		break;

	case PSPECIAL:
		if ( !del_ok )
		{
			IIUGerr(E_VF0038_Cant_delete_a_RBF_Se, UG_ERR_ERROR, 0);
			return;
		}
		if (noUndo)
		{
			vfersPos(ps);		/* erase from screen */ 
			unLinkPos(ps);		/* remove from line tab */
			setGlMove(ps);
			vffrmcount(ps, -1);	
		}
	}
}

/*
** delete a line by moving things up in the line table
*/
VOID
delLine(y)
i4	y;
{
	i4	delhcross = FALSE;
	register VFNODE **lp;

	/*
	**  If cross hair is on line we want to delete, drop the
	**  cross hair first.
	*/
	if (IIVFxh_disp && IIVFhcross->ps_begy >= y)
	{
		delhcross = TRUE;
		vfersPos(IIVFhcross);
		unLinkPos(IIVFhcross);
	}

	if (IIVFxh_disp)
	{
		vfersPos(IIVFvcross);
		unLinkPos(IIVFvcross);
	}

	lp = &line[y];
	if (*lp != NULL)
	{
		IIUGerr(E_VF0039_Cant_delete_a_line, UG_ERR_ERROR, 0);
		if (IIVFxh_disp)
		{
			if (delhcross)
			{
				insPos(IIVFhcross, FALSE);
			}
			insPos(IIVFvcross, FALSE);
		}
		return;
	}
	if (y == endFrame)
	{
		IIUGerr(E_VF003A_Cant_delete_the_last, UG_ERR_ERROR, 0);
		if (IIVFxh_disp)
		{
			if (delhcross)
			{
				insPos(IIVFhcross, FALSE);
			}
			insPos(IIVFvcross, FALSE);
		}
		return;
	}
	if (endFrame == 1)
	{
		IIUGerr(E_VF003B_Cant_delete_all_the, UG_ERR_ERROR, 0);
		if (IIVFxh_disp)
		{
			if (delhcross)
			{
				insPos(IIVFhcross, FALSE);
			}
			insPos(IIVFvcross, FALSE);
		}
		return;
	}

	setGlobUn(dlLINE, (POS *) y, 0);	/* post undo */

	doDelLine(y);				/* do the work */

	if (globy >= endFrame)
		globy = endFrame - 1;

	/*
	**  Put the horizontal cross hair back, if necessary.
	**  Also readjust the size of the vertical cross hair.
	*/
	if (IIVFxh_disp)
	{
		if (delhcross)
		{
			if (IIVFhcross->ps_begy >= endFrame)
			{
				IIVFhcross->ps_begy = IIVFhcross->ps_endy =
					endFrame - 1;
			}
			insPos(IIVFhcross, FALSE);
		}

		IIVFrvxRebuildVxh();
	}
}
	
/*{
** Name:	doDelLine	- do delete of given line 
**
** Description:
**		Delete given line.. no checks are made and undo
**		is not set.. this was extracted so it could be 
**		called from Form resize code.
**
** Inputs:
**	i4	y;	- line index to delete (0 based)
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	05/26/88 (tom) - extracted from delLine 
*/
VOID
doDelLine(y)
i4	y;
{	
	register VFNODE **lp;
	register VFNODE	**nlp;
	i4		i;

	lp = &line[y];
	nlp = &line[y+1];

	for (i = y; i < endFrame; i++)
	{
		adjustPos(*nlp,  -1, i);
		*lp++ = *nlp++;
	}
	*lp = NNIL;

	endFrame--;
}
