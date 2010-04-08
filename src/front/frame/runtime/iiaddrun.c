/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h> 
# include	<frscnsts.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iiaddrun.c
**
** Description:
**	Add necessary runtime structure to a frame,
**	and add the frame to the active list.
**
**	Public (extern) routines defined:
**		IIaddrun		- Add runtime structure to a frame.
**		IIFRaffAddFrmFrres2	- Add frres2 runtime-only structure to
**					  a frame.
**	Private (static) routines defined:
**		IIFRcsbClrScrollBit	- Turn off any scrolling-field bits.
**					  FT3270 only.
**
** History:
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	24-feb-1983  -  Added table field implementation  (ncg)
**	11-feb-1985  -  yanked code common to both iiaddfrm and
**			iifrminit and made separate routine. (bruceb)
**	5/8/85 - Added support for long menus. (dkh)
**	05-jan-1987  -  Changed calls to IIbadmem. (prs)
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	20-jun-88 (bruceb)
**		Added IIFRcsbClrScrollBit routine for IBM.  Needed
**		so that forms created with scrolling fields will work
**		on IBM, minus the scrolling characteristic.
**	10/28/88 (dkh) - Performance changes.
**	22-nov-88 (bruceb)
**		Initialize the event control block's timeout field.
**		Currently, the timeout value for all frames is identical,
**		so need to set this to the global value so as not to
**		alter that global value when this frame is displayed.
**	08-mar-89 (bruceb)
**		Added new IIFRaffAddFrmFrres2() routine.  Used to allocate
**		memory for the frame's FRRES2 struct and initialize the
**		'original field' value to BADFLDNO.  This illegal value will
**		have the benefit of triggering EA on initial display of the
**		form.
**	03/22/89 (dkh) - Fixed venus bug 4632.
**	04/07/89 (dkh) - Fixed allocation of RUNFRM structure to have
**			 cleared out memory.
**	05-jun-89 (bruceb)
**		Changed STalloc(name) to FEtsalloc(tag, name).
**	11-sep-89 (bruceb)
**		Call IIFDdvcDetermineVisCols() to set form up for any invisible
**		columns.
**	30-mar-90 (bruceb)
**		Initialize menu's mu_colon (for locator support.)
**	04/18/90 (dkh) - Integrated IBM porting changes.
**	24-apr-90 (bruceb)
**		frres2 now contains form's maxy size--for resized forms.
**	08/11/91 (dkh) - Added changes so that all memory allocated
**			 for a dynamically created form is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	TBSTRUCT	*IItget();
FUNC_EXTERN	VOID		 IIFDdvcDetermineVisCols();


/*{
** Name:	IIaddrun	-	Add runtime structure to a frame
**
** Description:
**	Allocates the RUNFRM for the frame and it's subsidiary pieces,
**	including the menu, control character, and FRSkey structures.
**	Build list of fields and tablefields on the frame.  Add the frame
**	to the list of displayed frames.
**
** Inputs:
**	frm		Ptr to the frame
**	name		Name of the frame
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
*/
i4
IIaddrun(frm, name)
reg	FRAME	*frm;
char		*name;
{
	RUNFRM			*runf;
	COMMS			*runcm;
	struct	menuType	*runmu;
	struct	com_tab		*comtab;
	reg	i4		i;
	FIELD			**fld;		/* table field in frame */
	TBSTRUCT		*tb;		/* runtime table rep    */
	FRSKY			*frskey;
	FRS_EVCB		*evcb;
	i4			tag;
	char			*procname = ERx("IIaddrun");

	tag = frm->frtag;

	/*
	**	Allocate and initialize the frame activation structure.
	*/
	if ((runf = (RUNFRM *)FEreqmem((u_i4)tag, (u_i4)sizeof(RUNFRM), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);	/* never returns */
	}

	runf->fdfrmnm = (char *)FEtsalloc((u_i2)tag, name);
	runf->fdrunfrm = frm;
	/*
	**	Alloc. and init. the control character command structure.
	*/
	if ((runcm = (COMMS *)FEreqmem((u_i4)tag,
		(u_i4)(c_MAX * sizeof(COMMS)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}

	runf->fdruncm = runcm;
	/*
	**	Alloc. and init. the menu structure.
	*/
	if ((runmu = (struct menuType *)FEreqmem((u_i4)tag,
	    (u_i4)sizeof(struct menuType), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}

	if ((comtab = (struct com_tab *)FEreqmem((u_i4)tag,
	    (u_i4)((MAX_MENUITEMS + 1) * sizeof(struct com_tab)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}

	runmu->mu_coms = comtab;
	runmu->mu_flags = 1; /* NULLRET */
	runmu->mu_colon = -1;
	/*
	**	Alloc. and init. the rest of the structures.
	*/
	runf->fdrunmu = runmu;
	runf->fdmunum = 0;
	runf->fdrunmd = fdrtNOP;

	/* 
	**	Alloc. and init. the table field structures.
	** 	Loop through the frame, and for each table field (if any) 
	** 	initialize the neccesary data structures and add onto the 
	**	frame's table list.
	*/
	runf->fdruntb = NULL;
	for (i = 0, fld = frm->frfld; i < frm->frfldno; i++, fld++)
	{
		if ((*fld)->fltag != FTABLE)
			continue;

		/* fld is now a table field, initialize structure */
		if ((tb = IItget(frm, *fld) ) == NULL)
		{
			return (FALSE);
		}
		tb->tb_nxttb = runf->fdruntb;	/* add onto list */
		runf->fdruntb = tb;

		IIFDdvcDetermineVisCols((*fld)->fld_var.fltblfld);
	}

# ifdef	TBLNONSEQ
	/*
	** Repeat for non-sequential fields.
	*/
	for (i = 0, fld = frm->frnsfld; i < frm->frnsno; i++, fld++)
	{
		if ((*fld)->fltag != FTABLE)
			continue;

		/* fld is now a table field, initialize structure */
		if ((tb = IItget(frm, *fld) ) == NULL)
		{
			return (FALSE);
		}
		tb->tb_nxttb = runf->fdruntb;	/* add onto list */
		runf->fdruntb = tb;
	}
# endif

	/*
	** Alloc. and init. the FRS data info structure.  Call to another 
	** routine, as FRS data constants may grow with new "inquire" and
	** "set" Equel commands.
	*/
	if ((runf->fdfrs = IIfsinit((u_i4) tag)) == NULL)
	{
		return (FALSE);
	}

	/*
	**  Allocate the activate frs key strucutre
	*/
	if ((frskey = (FRSKY *)FEreqmem((u_i4)tag,
	    (u_i4)(MAX_FRS_KEYS * sizeof(FRSKY)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}

	runf->fdfrskey = frskey;

	/*
	**  Allocate the event control block.
	*/
	if ((evcb = (FRS_EVCB *)FEreqmem((u_i4)tag, (u_i4)sizeof(FRS_EVCB),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}
	evcb->timeout = IIfrscb->frs_event->timeout;
	runf->fdevcb = evcb;

	/*
	**	Add frame to active list
	*/
	runf->fdrunnxt = NULL;
	runf->fdlisnxt = IIrootfrm;
	IIfrmio = IIrootfrm = runf;

	return (TRUE);
}

/*{
** Name:	IIFRaffAddFrmFrres2	- Add frres2 runtime-only structure to
**					  a frame.
**
** Description:
**	Add the frres2 structure to the passed-in frame.  This is a
**	runtime-only structure so won't be stored in the db, formfile or
**	compiled form.  Currently the structure is used as a method of
**	triggering field entry activations.  Initializing origfld to BADFLDNO
**	will force an EA on initial display of a form.
**	ADFLDNO.  Also setting savetblpos to FALSE so that code
**	checking this value can always assume it's properly set.
**
**	No return value is set since failure in this routine is fatal.
**
** Inputs:
**	frm		Ptr to frame to which the struct is added
**
** Outputs:
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Side Effects:
**	frres2 added to frame and initialized.
**
** History:
**	16-mar-89 (bruceb)	Written.
*/
VOID
IIFRaffAddFrmFrres2(frm)
FRAME	*frm;
{
    /*
    **  Allocate the FRRES2 structure (for entry activation).
    */
    if ((frm->frres2 = (FRRES2 *)FEreqmem((u_i4)frm->frtag,
	(u_i4)sizeof(FRRES2), TRUE, (STATUS *)NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("IIFRaffAddFrmFrres2"));
    }
    frm->frres2->origfld = BADFLDNO;
    frm->frres2->savetblpos = FALSE;
    frm->frres2->frmaxy = frm->frmaxy;
}
