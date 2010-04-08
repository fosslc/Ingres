/*
** select.c
** contains select commands
** Written (10/12/81)
**
** History:
**	02/01/85 (drh)	Copied PeterK's change splitting movecomp into
**			into movecomp.c.
**	02-25-85 (dkh)	Commented out routine "moveComp()".  Routine
**			is now in file by itself.  Done on SPHINX.
**	04/30/85 (gac)	Blink selected fields instead of beeping with dollars.
**	01/15/86 (jas)	Renamed `select' menu to `select_m'; select() is a
**			4.2bsd system call.
**	14-oct-86 (bruceb) fix for bug 10369
**		removed limit on number of display objects.
**	12/31/86 (dkh) - Added support for new activations.
**	07/08/86 (scl)	Use FTspace in vfshrwid for 3270 support
**	06/20/86 (a.dea) -- Change (void) to _VOID_ for CMS.
**	08/14/87 (dkh) - ER changes.
**	10/02/87 (dkh) - Help file changes.
**	10/25/87 (dkh) - Error message cleanup.
**	04/25/88 (tom) - Box/Line support.
**	08/03/88 (tom) - Fix bug re: data format overlaps title
**	04/06/89 (dkh) - Allow empty field titles again.
**	01-sep-89 (cmr)	get rid of reference to DETAIL; use whichSec().
**	9/21/89 (elein) - (PC integration) add declaration of num 
**			 in vfMoveBottomDown(num)
**	09/26/89 (dkh) - Previous change should be "nat" instead of "int".
**	22-nov-89 (cmr)- RBF change: In placeField() do not restrict a field
**			 to the detail section only.
**	12/05/89 (dkh) - Changed interface to VTcursor() so VTLIB could
**			 be moved into ii_framelib shared lib.
**	07-mar-90 (bruceb)
**		Lint:  Add 5th param to FTaddfrs() calls.
**	30-mar-90 (bruceb)
**		Force Vifred menu display back to first menuitem.
**	12-apr-90 (bruceb)
**		Ifdef FORRBF the changes made to placeField() on 22-nov-89.
**	19-apr-90 (bruceb)
**		No longer call setLimit(); it no longer exists.
**	09-may-90 (bruceb)
**		Call VTcursor() with new arg indicating whether there is
**		'not' a menu associated with this call.
**	5/5/90 (martym)
**		Added "resetFldnfo()" for RBF.
**      06/09/90 (esd) - Check IIVFsfaSpaceForAttr instead of
**                       calling FTspace, so that whether or not
**                       to leave room for attribute bytes can be
**                       controlled on a form-by-form basis.
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**      06/12/90 (esd) - When boxing a field on a 3270, leave space
**                       for an attribute between the box and title/data
**                       on the left as well as the right (bug 21618).
**      06/12/90 (esd) - When boxing or unboxing a field, set new member
**                       ps_attr of POS structure appropriately.
**	10-jul-90 (cmr)
**		resetFldInfo: check for null editpos (field with no heading).
**	21-jul-90 (cmr)
**		resetFldInfo: don't use glob[y,x] to determine POS (in case a
**		Shift occured).  Just use selPos (since it's readily available).
**	03/25/91 (dkh) - Replaced NULL with fdNOCODE as a case label to
**			 avoid problems on SCO.
**	06/28/92 (dkh) - Added support for rulers in vifred/rbf.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Included some more headers to satisfy function declarations.
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
# include	"decls.h"
# include	<ug.h>
# include	<ui.h>
# include	<frsctblk.h>
# include	<si.h>
# include	<vt.h>
# include	<er.h>
# include	"ervf.h"
# include	"vfhlp.h"
# include	<rtype.h>
# include	<rcons.h>
# include	<rglob.h>



/*
** routines communicate through some global variables which describe
** the position currently on and the actual type of object it is
*/

static POS	*selPos = NULL;		/* current feature */
static i4	selObj = PBLANK;	/* name of feature */
static SMLFLD	selfd = {0};

FUNC_EXTERN	STATUS		IImumatch();
FUNC_EXTERN	i4		VFfeature();
bool				placecmd();
# ifdef	FORRBF
FUNC_EXTERN	Sec_node	*whichSec();
FUNC_EXTERN	COPT		*rFgt_copt();
VOID 				resetFldInfo(POS *);
# endif

GLOBALREF	FT_TERMINFO	IIVFterminfo;
GLOBALREF	FILE		*vfdfile;
GLOBALREF	i4		vfrealx;

static MENU	selMu = NULL;

/*{
** Name:	selectcom	- Process user selection and move of a feature.
**
** Description:
** 	Select command has been entered.
** 	Determine what is being selected then allow movement
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		nothing
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	<manual entries>
*/
i4
selectcom()
{
	i4	menu_val;
	i4	poscount;
	FLD_POS *posarray = NULL;
	FRS_EVCB evcb;

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);

	/*
	**  Restricting a user to a certain boundary of the screen
	**  was found to be hard to support under FT/VT inteface.
	**  Therefore, it will not be supported on the first pass. (dkh)
	*/

	if (vfrealx == endxFrm + 1 + IIVFsfaSpaceForAttr)
	{
		vfwmove();
		return 0;
	}
	else
	{
		selPos = onPos(globy, globx);
		if (selPos == NULL)
		{
			IIUGerr(E_VF00B0_Can_only_select_MOVE,
				UG_ERR_ERROR, 0);
			return 0;
		}
		selObj = selPos->ps_name;
	}


	switch (selPos->ps_name)
	{
	case PBOX:
		selMu = selbox_m;

		/*
		**  If not a cross hair, then reset global position.
		*/
		if (selPos->ps_attr != IS_STRAIGHT_EDGE)
		{
			globy = selPos->ps_begy;
			globx = selPos->ps_begx;
		}
		break;

	case PFIELD:
		FDToSmlfd(&selfd, selPos);
# ifdef FORRBF
		if (selPos->ps_column != NULL)
			selMu = rbfFldMu;
		else
			selMu = rSFldMu;
# else
		selMu = selFldMu;
# endif
		break;

	case PTABLE:
	case PTRIM:
		if (RBF && selPos->ps_column != NULL)
			selMu = rbfSelect;
		else
			selMu = select_m;
		break;

	case PSPECIAL:
		/*
		**  Modified for fix to BUG 5167. (dkh)
		*/

		if (RBF)
		{
			if (globy == endFrame)
			{
				vflmove();
			}
			else
			{
				IIUGerr(E_VF00B1_Cant_move_a_RBF_Sect,
					UG_ERR_ERROR, 0);
			}
		}
		else
		{
			vflmove();
		}
		return 0;		/* (gac) Fixed bug #4052 */

	default:
		syserr(ERget(S_VF00B2_Unknown_position_in),
			selPos->ps_name);
		break;
	}

	/* Force the next FTputmenu() call to re-construct the menu. */
	selMu->mu_flags |= MU_NEW;

	vfdisplay(selPos, selObj, TRUE);	/* Hilight selected field */
	VTxydraw(frame, globy, globx);
	for (;;)
	{
		evcb.event = fdNOCODE;
		FTclrfrsk();
		FTaddfrs(1, HELP, NULL, 0, 0);
		FTaddfrs(3, UNDO, NULL, 0, 0);
		vfmu_put(selMu);
		/*
		** slUp and slDown are local variables which are set depending
		** on whether RBF is true or not
		*/
		vfposarray(&poscount, &posarray);
		if (VTcursor(frame, &globy, &globx, endFrame,
			endxFrm + 1 + IIVFsfaSpaceForAttr,
			poscount, posarray, &evcb, VFfeature,
			RBF, TRUE, FALSE, FALSE, FALSE) 
			== fdopSCRLT)
		{
			vfwider(frame, IIVFterminfo.cols/4);
			continue;
		}
		if (globx > endxFrm)
		{
			globx = endxFrm;
		}
		VTdumpset(vfdfile);

		if (evcb.event == fdopFRSK)
		{
			menu_val = evcb.intr_val;
		}
		else if (evcb.event == fdopMUITEM)
		{
			if (IImumatch(selMu, &evcb, &menu_val) != OK)
			{
				continue;
			}
		}
		else
		{
			/*
			**  Fix for BUG 7123. (dkh)
			*/
			menu_val = FTgetmenu(selMu, &evcb);
			if (evcb.event == fdopFRSK)
			{
				menu_val = evcb.intr_val;
			}
		}


		VTdumpset(NULL);
		switch (menu_val)
		{
		case fdNOCODE:
		default:
			continue;

		case HELP:
			if (selMu == select_m)
			{
# ifdef FORRBF
				vfhelpsub(RFH_TMOVE,
				    ERget(S_VF00B3_MOVE_Trim_Command),
				    selMu);
# else
				vfhelpsub(VFH_SELECT,
				    ERget(S_VF00B4_MOVE_Component_Comm),
				    selMu);
# endif
			}
# ifdef FORRBF
			else if (selMu == rbfSelect)
			{
				vfhelpsub(RFH_MOVE,
				    ERget(S_VF00B5_MOVE_Heading_or_det),
				    selMu);
			}
# else
			else if (selMu == selFldMu)
			{
				vfhelpsub(VFH_SLFLD,
				    ERget(S_VF00B6_MOVE_Field_Command),
				    selMu);
			}
# endif

# ifdef FORRBF
			else if (selMu == rSFldMu)
			{
				vfhelpsub(RFH_SLDET,
				    ERget(S_VF00B7_MOVE_Detail_Command),
				    selMu);
			}
			else if (selMu == rbfFldMu)
			{
				vfhelpsub(RFH_SLCOL,
				   ERget(S_VF00B8_MOVE_Detail_Command),
				   selMu);
			}
			else if (selMu == rbfColSel)
			{
				vfhelpsub(RFH_CMOVE,
				    ERget(S_VF00B9_Move_Column_Command),
				    selMu);
			}
# endif
			else if (selMu == selbox_m)
			{
				if (selPos->ps_attr == IS_STRAIGHT_EDGE)
				{
					vfhelpsub(VFH_SEMOV,
					    ERget(S_VF0173_MOVING_STR_EDGE),
					    selMu);
				}
				else
				{
					vfhelpsub(VFH_BOXMOV,
					    ERget(S_VF0133_MOVE_Box_Command),
				   	    selMu);
				}
			}
			else
			{
# ifdef FORRBF
				vfhelpsub(RFH_MOVE,
				   ERget(S_VF00BA_MOVE_Detail_Command),
				   selMu);
# else
				vfhelpsub(VFH_SLCOM,
				    ERget(S_VF00BB_MOVE_Title_or_fmt),
				    selMu);
# endif
			}
			continue;

# ifndef FORRBF
		case selTITLE:
			selObj = PTITLE;
			selMu = selComMu;
			vfdisplay(selPos, selObj, TRUE);
			continue;

		case selDATA:
			selObj = PDATA;
			selMu = selComMu;
			vfdisplay(selPos, selObj, TRUE);
			continue;
# endif

		case SHIFT:
			vfshiftmode();
			if (!placecmd())
				continue;
			break;

		case PLACE:
			vfclosemode();
			if (!placecmd())
				continue;
			break;

		case RIGHT:
			if (!rightcom())
				continue;
			break;

		case LEFT:
			if (!leftcom())
				continue;
			break;

		case CENTER:
			if (!centercom())
				continue;
			break;

# ifdef FORRBF
		case RCOLUMN:
			selObj = PCOLUMN;
			selMu = rbfColSel;
			vfdisplay(selPos, selObj, TRUE);
			continue;
# endif

		case UNDO:
			vfdisplay(selPos, selObj, FALSE);
			break;
		}
		break;
	}
	MEfree(posarray);
	vfTrigUpd();
	vfUpdate(globy, globx, TRUE);
	return 0;
}

/*
** place command has been selected
** what to do depends on what selObj is
** is feature selected is whole then just try and place at the
** current position checking for overlaps
** if the feature is not whole (e.g. a title of a field has been
** selected) then see if size of structure must be changed
*/
bool
placecmd()
{

	bool stat = FALSE;

	switch (selObj)
	{
# ifdef FORRBF
	case PCOLUMN:
		stat = place_Column(selPos, globy, globx, TRUE);
		if (stat)
			resetFldInfo(selPos);
		return(stat);
# endif

	case PTRIM:
		return (placeTrim(selPos, globy, globx, TRUE));

	case PBOX: 
		return (placeBox(selPos, globy, globx, TRUE)); 

	case PFIELD:
		/* if statement added for bug fix 1199.	 Needed
		** since compoenents may be forced down in RBF
		** and we need to have the correct position for
		** a move and place sequence.  (dkh)
		*/
# ifdef FORRBF
		FDToSmlfd(&selfd, selPos);
# endif

		stat = (placeField(selPos, &selfd, globy, globx, TRUE));

# ifdef FORRBF
		if (stat)
			resetFldInfo(selPos);
# endif
		return(stat);

	case PTITLE:
		return (placeTitle(selPos, &selfd, globy, globx, TRUE));

	case PDATA:
		return (placeData(selPos, &selfd, globy, globx, TRUE));

# ifndef FORRBF
	case PTABLE:
		return (placeTable(selPos, globy, globx, TRUE));
# endif

	default:
		syserr(ERget(S_VF00BC_placecmd_inconsisten), selObj);
		/* does not return */
	}
	/*NOTREACHED*/
}

/*
** formatting commands
** right, left and center
*/
bool
rightcom()
{
# ifdef FORRBF
	i4	minx;
# endif
	i4	maxx;
	reg	POS	*ps;

	ps = selPos;
# ifdef FORRBF
	if (selObj == PCOLUMN)
		vfcolsize(ps, &minx, &maxx);
	else
	{
		minx = ps->ps_begx;
		maxx = ps->ps_endx;
	}
# else
	maxx = ps->ps_endx;
# endif
	globx = ps->ps_begx + endxFrm - maxx;
	return (placecmd());
}

bool
leftcom()
{
	reg	POS	*ps = selPos;
	i4	minx;
# ifdef FORRBF
	i4	maxx;
# endif

# ifdef FORRBF
	if (selObj == PCOLUMN)
		vfcolsize(ps, &minx, &maxx);
	else
	{
		minx = ps->ps_begx;
		maxx = ps->ps_endx;
	}
# else
	minx = ps->ps_begx;
# endif
	globx = ps->ps_begx - minx;
	return (placecmd());
}

bool
centercom()
{
	i4	minx, maxx;
	reg	POS	*ps;

	ps = selPos;
# ifdef FORRBF
	if (selObj == PCOLUMN)
		vfcolsize(ps, &minx, &maxx);
	else
	{
		minx = ps->ps_begx;
		maxx = ps->ps_endx;
	}
# else
	minx = ps->ps_begx;
	maxx = ps->ps_endx;
# endif

	ps = selPos;
	globx = (endxFrm + 1 - (maxx - minx))/2;
	globx += ps->ps_begx - minx;
	return (placecmd());
}

/*
** Try placing the pos at the passed x y
** if it fits them actually place it there
** otherwise print an error message
*/
bool
placeField(ps, fd, y, x, noUndo)
reg	POS	*ps; 
reg	SMLFLD	*fd;
	i4	y,
		x;
bool	noUndo;
{
	Sec_node *whichSec();

	if (x + (ps->ps_endx - ps->ps_begx) >= endxFrm + 1)
	{
		IIUGerr(E_VF00BD_Cant_move_the_field, UG_ERR_ERROR, 0); 
		return (FALSE); 
	} 
# ifdef	FORRBF
	if ( (FDgethdr((FIELD *)ps->ps_feat))->fhd2flags & fdDERIVED )
		if ( (whichSec(y, &Sections))->sec_type < SEC_BRKFTR )
		{
			IIUGerr(E_VF0161_Cant_move_the_agg, UG_ERR_ERROR, 0); 
			return (FALSE); 
		}
# endif

	vfTestDump();
	if (noUndo)
	{
		unLinkPos(ps);
		if (ps->ps_column != NULL)
			delColumn(ps->ps_column, ps);
		setGlobUn(slFIELD, ps, fd);
	}
	moveField(ps, fd, y, x, noUndo);
	return (TRUE);
}

bool
moveField(ps, fd, y, x, noUndo)
reg	POS	*ps;
reg	SMLFLD	*fd;
i4		y,
		x;
bool		noUndo;
{
	i4		ex;

	ex = x + (ps->ps_endx - ps->ps_begx);
	savePos(ps);
	vfersPos(ps);
	ps->ps_endx = ex;
	ps->ps_endy = y + (ps->ps_endy - ps->ps_begy);
	ps->ps_begx = x;
	ps->ps_begy = y;
	fitPos(ps, noUndo);
	insPos(ps, TRUE);
	if (noUndo && ps->ps_column != NULL)
		insColumn(ps->ps_column, ps);
	moveFld(ps, fd, frame->frscr);
	return (TRUE);
}
/*
** Try placing the table at the passed x y
** if it fits them actually place it there
** otherwise print an error message
*/

# ifndef FORRBF
bool
placeTable(ps, y, x, noUndo)
reg	POS	*ps;
	i4	y,
		x;
	bool	noUndo;
{
	/*
	**  Note:  Code not changed since table fields are allowed
	**  to be from one edge of the terminal screen to the other.
	**  (dkh)
	*/

	if (x + (ps->ps_endx - ps->ps_begx) >= endxFrm + 1)
	{
		IIUGerr(E_VF00BF_Cant_move_the_table, UG_ERR_ERROR, 0);
		return (FALSE);
	}
	vfTestDump();
	if (noUndo)
	{
		if (ps->ps_column != NULL)
			delColumn(ps->ps_column, ps);
		unLinkPos(ps);
		setGlobUn(slTABLE, ps, NULL);
	}
	moveTbl(ps, y, x, noUndo);
	return (TRUE);
}

moveTbl(ps, y, x, noUndo)
reg	POS	*ps;
	i4	y,
		x;
	bool	noUndo;
{
	i4		ex;

	ex = x + (ps->ps_endx - ps->ps_begx);
	if (noUndo)
		vfersPos(ps);
	savePos(ps);
	ps->ps_endx = ex;
	ps->ps_endy = y + (ps->ps_endy - ps->ps_begy);
	ps->ps_begx = x;
	ps->ps_begy = y;
	fitPos(ps, noUndo);
	insPos(ps, TRUE);
	if (noUndo && ps->ps_column != NULL)
		insColumn(ps->ps_column, ps);
	moveTab(ps, frame->frscr);
	return (TRUE);
}
# endif

/*
** place some trim here
*/
bool
placeTrim(ps, y, x, noUndo)
POS	*ps;
i4	y,
	x;
bool	noUndo;
{
	if (x + (ps->ps_endx - ps->ps_begx) >= endxFrm + 1)
	{
		IIUGerr(E_VF00C0_Cant_move_the_trim, UG_ERR_ERROR, 0);
		return (FALSE);
	}
	vfTestDump();
	if (noUndo)
	{
		if (ps->ps_column != NULL)
			delColumn(ps->ps_column, ps);
		unLinkPos(ps);
		setGlobUn(slTRIM, ps, NULL);
	}
	moveTrim(ps, y, x, noUndo);
	return (TRUE);
}

VOID
moveTrim(ps, y, x, noUndo)
POS	*ps;
i4	y,
	x;
bool	noUndo;
{
	TRIM	*tr;
	i4	ex;

	ex = x + (ps->ps_endx - ps->ps_begx);
	vfersPos(ps);
	savePos(ps);
	ps->ps_endx = ex;
	ps->ps_endy = y + (ps->ps_endy - ps->ps_begy);
	ps->ps_begx = x;
	ps->ps_begy = y;
	ps->ps_feat = ps->ps_feat;
	fitPos(ps, noUndo);
	insPos(ps, TRUE);
	if (noUndo && ps->ps_column != NULL)
		insColumn(ps->ps_column, ps);
	tr = (TRIM *) ps->ps_feat;
	vfTestDump();
	tr->trmx = x;
	tr->trmy = y;
	vfUpdate(y, x, TRUE);
}

/*{
** Name:	placeBox	- place box at given location 
**
** Description:
**	Place a box at the given location. No "splatter" effect 
**	occurs with boxes.
**
** Inputs:
**	POS	*ps;	- position structure
**	i4	y,x;	- the to position to place it.
**	i4	noUndo; - flag to say that this is not an undo operation
**
** Outputs:
**	Returns:
**		bool  - returns TRUE if the box was correctly placed, 
**			else FALSE
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	04/18/88  (tom) - first written	
*/
bool
placeBox(ps, y, x, noUndo)
POS	*ps;
i4	y,
	x;
bool	noUndo;
{
	if ((x + (ps->ps_endx - ps->ps_begx) >= endxFrm + 1) &&
		ps->ps_attr != IS_STRAIGHT_EDGE)
	{
		IIUGerr(E_VF012e_Cant_move_the_box, UG_ERR_ERROR, 0);
		return (FALSE);
	}
	vfTestDump();
	if (noUndo)
	{
		unLinkPos(ps);

		/*
		**  Only set undo mechanism if not a cross hair.
		*/
		if (ps->ps_attr != IS_STRAIGHT_EDGE)
		{
			setGlobUn(slBOX, ps, NULL);
		}
	}
	moveBox(ps, y, x, noUndo);
	return (TRUE);
}

/*{
** Name:	moveBox	- do actual placement of the box at the location 
**
** Description:
**		Do actual placement of box at a location.
**
** Inputs:
**	POS	*ps;	- position structure
**	i4	y,x;	- the to position to place it.
**	i4	noUndo; - flag to say that this is not an undo operation
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	04/18/88  (tom) - first written	
*/
VOID
moveBox(ps, y, x, noUndo)
POS	*ps;
i4	y,
	x;
bool	noUndo;
{
	i4	ex;
	i4	ey;
	bool	no_changes;

	ex = x + (ps->ps_endx - ps->ps_begx);
	ey = y + (ps->ps_endy - ps->ps_begy);

	vfersPos(ps);
	savePos(ps);

	/*
	**  If moving a horizontal cross hair, only set the y information.
	*/
	if (ps->ps_attr != IS_STRAIGHT_EDGE ||
		(ps->ps_attr == IS_STRAIGHT_EDGE && ps == IIVFhcross))
	{
		/*
		**  If moving the horizontal cross hair and the
		**  destination is the bottom margin, put the cross
		**  hair just above the bottom margin.
		**
		**  Also move the cursor to the new location.
		*/
		if (ps == IIVFhcross && y == endFrame)
		{
			ey--;
			y--;
			globy = y;
		}
		ps->ps_endy = ey;
		ps->ps_begy = y;
	}

	/*
	**  If moving a vertical cross hair, only set the x information.
	*/
	if (ps->ps_attr != IS_STRAIGHT_EDGE ||
		(ps->ps_attr == IS_STRAIGHT_EDGE && ps == IIVFvcross))
	{
		ps->ps_endx = ex;
		ps->ps_begx = x;
	}

	/* Note: at this point other features would call fitPos to 
	   move other features around and push the bottom of the
	   form down.. but boxes do not move other feature around..
	   most notably boxes will not push down the PSPECIAL feature
	   which represents the end of form margin.. so we must do it
	   manually.  */

	/*
	**  Only set up for undo if not a cross hair.
	*/
	if (noUndo && ps->ps_attr != IS_STRAIGHT_EDGE)
		setGlMove(ps);

	/*
	**  If moving a cross hair, we must nuke the undo info
	**  since an undo after moving a cross hair causes the
	**  wrong screen image to be displayed.  This is an
	**  artifact of how the undo mechanism manages the before
	**  and after screen images to support undo.
	**
	**  We also need to save and restore the noChanges value
	**  since setGlobUn will change it.
	*/
	if (ps->ps_attr == IS_STRAIGHT_EDGE)
	{
		no_changes = noChanges;
		setGlobUn(unNONE, (POS *) NULL, (i4 *) NULL);
		noChanges = no_changes;
	}

	/*
	**  We never increase form size when moving cross hairs.
	*/
	if (ps->ps_attr != IS_STRAIGHT_EDGE && ey >= endFrame)
	{
		/* +2 gives 1 blank line */
		vfMoveBottomDown(ey - endFrame + 2);
	}

	insPos(ps, FALSE); 
/*
	if (noUndo && ps->ps_column != NULL) 	
		insColumn(ps->ps_column, ps); 
*/
	vfTestDump();
}

/*{
** Name:	vfMoveBottomDown	- move form bottom down
**
** Description:
**		Move the bottom of the form down. This function is 
**		necessary due to the fact that boxes do not move
**		features around when they are created or moved..
**		but at times it is necessary to move the bottom 
**		margin PSPECIAL feature down, like when boxes
**		are created on the bottom line, and when boxes
**		are moved such as to force the bottom of the 
**		form down.
**
** Inputs:
**	i4 num;	- number of lines to move down.
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**	The form's bottom is moved 
**
** History:
**	06/28/88 (tom) - written
*/
VOID
vfMoveBottomDown(num)
i4  num;
{
	POS	*endps;
	endps = line[endFrame]->nd_pos;
	savePos(endps);
	vfersPos(endps);
	unLinkPos(endps);
	vfnewLine(num);
	newFrame(num);

	endps->ps_begy += num;
	endps->ps_endy += num;

	insPos(endps, FALSE);
}


/*
** try and place the title of some field here
*/
bool
placeTitle(ps, fd, y, x, noUndo)
POS	*ps;
SMLFLD	*fd;
i4	y,
	x;
bool	noUndo;
{
	i4	tx;

	if ((tx = x + (fd->tex - fd->tx)) >= endxFrm + 1)
	{
		IIUGerr(E_VF00C1_Cant_move_the_title, UG_ERR_ERROR, 0);
		return (FALSE);
	}
	vfTestDump();
	if (noUndo)
	{
		setGlobUn(slTITLE, ps, fd);
	}
	fd->tex = tx;
	fd->tx = x;
	fd->ty = y;
	fd->tey = y;
	fixField(ps, fd, noUndo, PTITLE);
	return (TRUE);
}

/*
** try and place the data window of some field here
*/
bool
placeData(ps, fd, y, x, noUndo)
POS	*ps;
SMLFLD	*fd;
i4	y,
	x;
bool	noUndo;
{
	i4	dx;

	if ((dx = x + (fd->dex - fd->dx)) >= endxFrm + 1)
	{
		IIUGerr(E_VF00C2_Cant_move_the_fmt, UG_ERR_ERROR, 0);
		return (FALSE);
	}
	vfTestDump();
	if (noUndo)
	{
		setGlobUn(slDATA, ps, fd);
	}
	fd->dex = dx;
	fd->dey = y + fd->dey - fd->dy;
	fd->dx = x;
	fd->dy = y;
	fixField(ps, fd, noUndo, PFIELD);
	return (TRUE);
}
/*
** move a field over to a different place
*/
VOID
fixField(ps, fd, noUndo, goodFeat)
reg	POS	*ps;
reg	SMLFLD	*fd;
	i4	noUndo;
	i4	goodFeat;
{
# ifndef FORRBF
	reg	FIELD	*ft;
		FLDHDR	*hdr;
	i4	spacesize;
# endif

# ifndef FORRBF
	spacesize = IIVFsfaSpaceForAttr;
# endif

	if (noUndo)
	{
		wrOver();

		if (ps->ps_column != NULL)
			delColumn(ps->ps_column, ps);
		unLinkPos(ps);
	}
	if (titleDataOver(fd))
	{
		vfTestDump();
		moveSubComp(fd, goodFeat);
	}
	savePos(ps);
	ps->ps_begx = min(fd->tx, fd->dx);
	ps->ps_begy = min(fd->ty, fd->dy);
	ps->ps_endx = max(fd->tex, fd->dex);
	ps->ps_endy = max(fd->tey, fd->dey);
# ifndef FORRBF
	ft = (FIELD *)ps->ps_feat;
	hdr = FDgethdr(ft);
	if (hdr->fhdflags & fdBOXFLD)
	{
		if (ps->ps_begy == 0 || ps->ps_begx <= spacesize ||
			ps->ps_endx >= endxFrm - spacesize)
		{
			IIUGerr(E_VF00C3_Cant_box_this_field, UG_ERR_ERROR, 0);
			hdr->fhdflags &= (~fdBOXFLD);
		}
		else
		{
			ps->ps_begx -= 1 + spacesize;
			ps->ps_endx += 1 + spacesize;
			ps->ps_begy--;
			ps->ps_endy++;
		}
	}
# endif
	fitPos(ps, noUndo);
	insPos(ps, TRUE);
	if (noUndo && ps->ps_column != NULL)
		insColumn(ps->ps_column, ps);
	moveComp(ps, fd);
}

bool
titleDataOver(fd)
SMLFLD	*fd;
{
	i4	spacesize;

	spacesize = IIVFsfaSpaceForAttr;

	if (*(fd->tstr) == 0)
		return (FALSE);
	return (fd->ty >= fd->dy && fd->ty <= fd->dey &&
		((fd->tx >= fd->dx && fd->tx <= fd->dex + spacesize) ||
		 (fd->dx >= fd->tx && fd->dx <= fd->tex + spacesize)));
}

/*
** title and data overLap
** determine which needs to be moved and move it
** first try moving right or left, and then move down
*/
VOID
moveSubComp(fd, goodFeat)
SMLFLD	*fd;
i4	goodFeat;
{
	reg	i4	*b;		/* bad component */
	reg	i4	*be;		/* will be either x or y */
	reg	i4	*g;		/* good component */
	reg	i4	*ge;
	reg	i4	size;

	/*
	**  Note: Most of the code has not been changed due to the
	**  assumption that display attribute bytes take up only
	**  one space on the display.  If this is no longer true then
	**  the code must change to accomodate this. (dkh)
	*/

	if (goodFeat == PTITLE)
	{
		b = &fd->dx; be = &fd->dex;
		g = &fd->tx; ge = &fd->tex;
	}
	else
	{
		g = &fd->dx; ge = &fd->dex;
		b = &fd->tx; be = &fd->tex;
	}
	size = *be - *g + 2;	/* size of overlap one added for extra space*/
	if (*b < *g && (*b - size) >= 0)
	{
		*b -= size;
		*be -= size;
		return;
	}
	size = *be - *b + 2;	/* size of bad comp one added for extra space*/
	if (*ge + size < endxFrm + 1)
	{
		*b = *ge + 2;
		*be = *b + size - 2;
		return;
	}
	/*
	** couldn't move right or left,
	** move down
	*/
	if (goodFeat == PTITLE)
	{
		b = &fd->dy; be = &fd->dey;
		g = &fd->ty; ge = &fd->tey;
	}
	else
	{
		g = &fd->dy; ge = &fd->dey;
		b = &fd->ty; be = &fd->tey;
	}
	size = *be - *g + 1;
	if (*b < *g && *b - size >= 0)
	{
		*b = *b - size;
		*be = *be - size;
		return;
	}
	/*
	** nothing else worked
	** move bad component below good one
	** don't have to worry about being off screen
	** other things will do that
	*/
	size = *be - *b;
	*b = *ge + 1;
	*be = *b + size;
	return;
}

VOID
vfboundary(shrink_len)
i4	shrink_len;
{
	i4	menu_val;
	i4	poscount;
	FLD_POS *posarray = NULL;
	FRS_EVCB evcb;

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);

	if (shrink_len)
	{
		globy--;
	}

	/* Force the next FTputmenu() call to re-construct the menu. */
	boundMu->mu_flags |= MU_NEW;

	for (;;)
	{
		evcb.event = fdNOCODE;
		FTclrfrsk();
		FTaddfrs(1, HELP, NULL, 0, 0);
		FTaddfrs(3, QUIT, NULL, 0, 0);
		vfmu_put(boundMu);
		vfposarray(&poscount, &posarray);
		_VOID_ VTcursor(frame, &globy, &globx, endFrame - 1, endxFrm,
			poscount, posarray, &evcb, VFfeature,
			RBF, TRUE, FALSE, FALSE, FALSE);
		VTdumpset(vfdfile);

		if (evcb.event == fdopFRSK)
		{
			menu_val = evcb.intr_val;
		}
		else if (evcb.event == fdopMUITEM)
		{
			if (IImumatch(boundMu, &evcb, &menu_val) != OK)
			{
				continue;
			}
		}
		else
		{
			menu_val = FTgetmenu(boundMu, &evcb);
			if (evcb.event == fdopFRSK)
			{
				menu_val = evcb.intr_val;
			}
		}


		VTdumpset(NULL);
		switch(menu_val)
		{
		case fdNOCODE:
		default:
			continue;

		case HELP:
			vfhelpsub(VFH_EXPSHR,
				ERget(S_VF00C4_Move_Form_Boundaries),
				boundMu);
			continue;

		case PLACE:
			if (shrink_len)
			{
				vfshrlen();
			}
			else
			{
				vfshrwid();
			}
			break;

		case EXPANDIT:
			if (shrink_len)
			{
				vfexplen();
			}
			else
			{
				vfexpwid();
			}
			break;

		case QUIT:
			break;
		}
		break;
	}
	MEfree(posarray);
}


VOID
vflmove()
{
	vfboundary(TRUE);
}

VOID
vfshrlen()
{
	i4	lasty;

	if (IIVFxh_disp)
	{
		vfersPos(IIVFhcross);
		vfersPos(IIVFvcross);
		unLinkPos(IIVFhcross);
		unLinkPos(IIVFvcross);
	}

	lasty = vflfind();

	if (lasty <= 0)		/* insure that forms are least 1 line long */
		lasty = 1;

	if (globy > lasty)
	{
		lasty = globy;
	}
	vflchg(lasty, FALSE);
}

VOID
vfexplen()
{
	i4	lines;

	lines = IIVFterminfo.lines/4;
	vfnewLine(lines);
	newFrame(lines);
	endFrame -= lines;
	vflchg(endFrame + lines, TRUE);
}


VOID
vflchg(lasty, delxh)
i4	lasty;
i4	delxh;
{
	VFNODE	*nd;
	POS	*ps;
	TRIM	*tr;

	/*
	**  If we are displaying cross hairs, drop them before
	**  determining whether the form's length can change.
	*/
	if (IIVFxh_disp && delxh)
	{
		vfersPos(IIVFhcross);
		vfersPos(IIVFvcross);
		unLinkPos(IIVFhcross);
		unLinkPos(IIVFvcross);
	}

	setGlobUn(slBMARGIN, NULL, endFrame);
	nd = line[endFrame];
	ps = nd->nd_pos;
	ps->ps_begy = lasty;
	ps->ps_endy = lasty;
	tr = (TRIM *) ps->ps_feat;
	tr->trmy = lasty;
	line[endFrame] = NULL;
	line[lasty] = nd;
	globy = endFrame = lasty;

	/*
	**  Put the cross hairs back if necessary.
	*/
	if (IIVFxh_disp)
	{
		IIVFvcross->ps_endy = endFrame - 1;
		if (IIVFhcross->ps_begy >= endFrame)
		{
			IIVFhcross->ps_begy = IIVFhcross->ps_endy =
				endFrame - 1;
		}
		insPos(IIVFhcross, FALSE);
		insPos(IIVFvcross, FALSE);
	}
}


VOID
vfwmove()
{
	vfboundary(FALSE);
}


VOID
vfshrwid()
{
	i4	lastx;

	lastx = vfwfind();
	if (globx > lastx)
	{
		lastx = globx - 1;
	}


	vfwchg(lastx);
}


VOID
vfexpwid()
{
	register i4	cols;

	cols = IIVFterminfo.cols/4;
	vfwider(frame, cols);
	endxFrm -= cols;
	vfwchg(endxFrm + cols);
}

VOID
vfwchg(lastx)
i4	lastx;
{
	/*
	**  Changing the width of the form.  Must also change the
	**  horizontal cross hair if it is displayed.  The vertical
	**  cross hair must also get moved in if it will be to the
	**  right of the right margin.
	*/
	if (IIVFxh_disp)
	{
		vfersPos(IIVFhcross);
		unLinkPos(IIVFhcross);

		IIVFhcross->ps_endx = lastx;

		insPos(IIVFhcross, FALSE);

		if (IIVFvcross->ps_begx > lastx)
		{
			vfersPos(IIVFvcross);
			unLinkPos(IIVFvcross);

			IIVFvcross->ps_begx = lastx;
			IIVFvcross->ps_endx = lastx;

			insPos(IIVFvcross, FALSE);
		}
	}

	setGlobUn(slRMARGIN, NULL, endxFrm);
	vf_wchg(lastx);
	globx = endxFrm + 1;
}

VOID
vf_wchg(lastx)
i4	lastx;
{
	endxFrm = lastx;
}


# ifdef FORRBF
VOID resetFldInfo(editpos)
POS	*editpos;
{

	POS *end;
	FIELD *ft;
	i4 ord;
	COPT *copt;

	end = editpos;
	do 
	{
		switch(editpos->ps_name)
		{
			case PFIELD:
				ft = (FIELD *) editpos->ps_feat;
				if (!((FDgethdr(ft))->fhd2flags & fdDERIVED))
					if (((whichSec(editpos->ps_begy, 
						&Sections))-> sec_type != 
						SEC_DETAIL))
					{
						ord = r_mtch_att(ft->fldname);
						copt = rFgt_copt(ord);
						copt->copt_whenprint = 'a';
					}
				break;
		}
		editpos = editpos->ps_column;
	}
	while (editpos && editpos != end);
	
	return;
}
# endif
