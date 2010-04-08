/*
**  Menu defintions for VIFRED
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	01/24/85 (drh)	Eliminated ct_muWrite, mu_muWrite, ct_muSequence,
**			mu_muSequence as part of cfe/vt integration
**	01/30/85 (drh)	Changed fdcats to catcall - lost in integration.
**	2/1/85 (peterk) - eliminated muHelp structures (cfe/mod).
**	08/14/87 (dkh) - ER changes.
**	09/04/87 (dkh) - Moved IIVFstrinit() to this file from vifred.qc
**			 so RBF won't pull in all of VIFRED.
**	12/16/87 (dkh) - Fixed jup bug 1609.
**	08/26/88 (dkh) - Changed ifdef to ifndef in IIVFgetmu().
**	02-jun-89 (bruceb)
**		In IIVFstrinit(), only allocate yes/no strings if not
**		already done (needed for multiple calls on VIFRED code).
**	01-sep-89 (cmr)	Added RBF menu option for Layout, changed Order to
**			ColOpt.
**	01/12/90 (dkh) - Moved UNDO menuitem to be after the
**			 ReportOptions menuitem in rbfCursor menu.
**	06-feb-90 (bruceb)
**		Moved RBF's report options function out of rbfCursor into
**		the vfcursor code.
**	28-mar-90 (bruceb)
**		Update menu structs for locator support.
**	5/22/90 (martym)
**		Changed "rbfedFldMu" to "rbfedFldMu1" and added "rbfedFldMu2".
**	16-jul-90 (bruceb)
**		Added "IIVFcmCursorMenu" as version of "cursor" when VIFRED
**		linked into ABF.
**	05-sep-90 (sylviap)
**		Added explanations to the Help, Keys. #32699.
**	19-sep-90 (sylviap)
**		Took out references to muHeading - no longer used.
**	02-jul-91 (kirke)
**		Corrected code in IIVFgetmu() to produce intended results.
**		post-increment operator was being used incorrectly.
**	08/06/91 (dkh) - Added explanation to explain why post-increment
**			 is not valid.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		Added __HIGHC__ pragam to break data segment that is greater
**		than 64KB into 2 pieces.
**	06/27/92 (dkh) - Added support for naming a component in the
**			 detail section of rbf layout.
**	22-jun-1999 (musro02) 
**	    Corrected pragma to #pragma
**      15-feb-2000 (musro02)
**              Corrected pragma to #pragma
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<st.h>
# include	<ug.h>
# include	<er.h>
# include	"ervf.h"

# ifdef	__HIGHC__
#pragma    Static_segment("VIF1DATA");
# endif

# ifndef FORRBF
static struct com_tab ct_cursor[] =
{
	NULL,	vfcreate,	CREATE,   0, 0, 0,		NULL,
	NULL,	delCom,		DELETE,	  0, 0, 0,		NULL,
	NULL,	edit2com,	EDIT2,	  0, 0, 0,		NULL,
	NULL,	selectcom,	SELECT,	  0, 0, 0,		NULL,
	NULL,	NULL,		UNDO,	  0, 0, 0,		NULL,
	NULL,	vfsequence,	ORDER,    0, 0, 0,		NULL,
	NULL,	NULL,		wrWRITE,  0, 0, 0,		NULL,

#ifndef POPUP_DISABLE 	/* to disable for FT3270  */
	NULL,	vfformattr,	FORMATR,  0, 0, 0,		NULL,
#endif

	NULL,	vfWhersCurs,	WHERCUR,  0, 0, 0,		NULL,
	NULL,	NULL,		GRPMOVE,  0, 0, 0,		NULL,
	NULL,	NULL,		ALIGN,    0, 0, 0,		NULL,
	NULL,	NULL,		HELP,	  0, 0, 0,		NULL,
	NULL,	NULL,		QUIT,	  0, 0, 0,		NULL,
	NULL,	NULL,		EXIT,	  0, 0, 0,		NULL,
	NULL,	NULL,		0,	  0, 0, 0,		NULL
};

static struct menuType mu_cursor =
{
	NULL,
	ct_cursor,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
#ifdef POPUP_DISABLE  /* one less menu item */
	13,
#else
	14,
#endif
	NULL,
	NULL,
	-1,
	-1,
	-1,
# ifdef POPUP_DISBLE
	13
# else
	14
# endif
};
GLOBALDEF MENU cursor = &mu_cursor;

static struct com_tab ct2_cursor[] =
{
	NULL,	vfcreate,	CREATE,   0, 0, 0,		NULL,
	NULL,	delCom,		DELETE,	  0, 0, 0,		NULL,
	NULL,	edit2com,	EDIT2,	  0, 0, 0,		NULL,
	NULL,	selectcom,	SELECT,	  0, 0, 0,		NULL,
	NULL,	NULL,		UNDO,	  0, 0, 0,		NULL,
	NULL,	vfsequence,	ORDER,    0, 0, 0,		NULL,
	NULL,	NULL,		wrWRITE,  0, 0, 0,		NULL,

#ifndef POPUP_DISABLE 	/* to disable for FT3270  */
	NULL,	vfformattr,	FORMATR,  0, 0, 0,		NULL,
#endif

	NULL,	vfWhersCurs,	WHERCUR,  0, 0, 0,		NULL,
	NULL,	NULL,		GRPMOVE,  0, 0, 0,		NULL,
	NULL,	NULL,		ALIGN,    0, 0, 0,		NULL,
	NULL,	NULL,		HELP,	  0, 0, 0,		NULL,
	NULL,	NULL,		QUIT,	  0, 0, 0,		NULL,
	NULL,	NULL,		0,	  0, 0, 0,		NULL
};

static struct menuType mu2_cursor =
{
	NULL,
	ct2_cursor,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
#ifdef POPUP_DISABLE  /* one less menu item */
	12,
#else
	13,
#endif
	NULL,
	NULL,
	-1,
	-1,
	-1,
# ifdef POPUP_DISABLE
	12
# else
	13
# endif
};
GLOBALDEF MENU IIVFcmCursorMenu = &mu2_cursor;
# endif


# ifndef FORRBF
static struct com_tab ct_entField[] =
{
	NULL,	enterTitle,	TITLE,	0,	0,	0,	NULL,
	NULL,	enterData,	DATA,	0,	0,	0,	NULL,
	NULL,	enterAttr,	ATTR,	0,	0,	0,	NULL,
	NULL,	NULL,		UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,		HELP,	0,	0,	0,	NULL,
	NULL,	NULL,		DONE,	0,	0,	0,	NULL,
	NULL,	NULL,		0,	0,	0,	0,	NULL
};

static struct menuType mu_entField =
{
	NULL,
	ct_entField,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	6,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	6
};
GLOBALDEF MENU entField = &mu_entField;
# endif


static struct com_tab ct_select[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	LEFT,	0,	0,	0,	NULL,
	NULL,	NULL,	CENTER,	0,	0,	0,	NULL,
	NULL,	NULL,	RIGHT,	0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_select =
{
	NULL,
	ct_select,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	7,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	7
};
/*
**  Name changed from "select", which is a 4.2bsd system call.  (jas)
*/
GLOBALDEF MENU select_m = &mu_select;


static struct com_tab ct_selbox[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_selbox =
{
	NULL,
	ct_selbox,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	3,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	3
};

GLOBALDEF MENU selbox_m = &mu_selbox;



# ifndef FORRBF
static struct com_tab ct_selFldMu[] =
{
	NULL,	NULL,	PLACE,		0,	0,	0,	NULL,
	NULL,	NULL,	LEFT,		0,	0,	0,	NULL,
	NULL,	NULL,	CENTER,		0,	0,	0,	NULL,
	NULL,	NULL,	RIGHT,		0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,		0,	0,	0,	NULL,
	NULL,	NULL,	selTITLE,	0,	0,	0,	NULL,
	NULL,	NULL,	selDATA,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,		0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,		0,	0,	0,	NULL,
	NULL,	NULL,	0,		0,	0,	0,	NULL
};

static struct menuType mu_selFldMu =
{
	NULL,
	ct_selFldMu,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	9,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	9
};
GLOBALDEF MENU selFldMu = &mu_selFldMu;
# endif



# ifdef FORRBF
static struct com_tab ct_rSFldMu[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	LEFT,	0,	0,	0,	NULL,
	NULL,	NULL,	CENTER,	0,	0,	0,	NULL,
	NULL,	NULL,	RIGHT,	0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_rSFldMu =
{
	NULL,
	ct_rSFldMu,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	7,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	7
};
GLOBALDEF MENU rSFldMu = &mu_rSFldMu;
# endif


# ifndef FORRBF
static struct com_tab ct_selComMu[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_selComMu =
{
	NULL,
	ct_selComMu,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	4,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	4
};
GLOBALDEF MENU selComMu = &mu_selComMu;
# endif


# ifndef FORRBF
static struct com_tab ct_edFldMu[] =
{
	NULL,	NULL,	ediTITLE,	0,	0,	0,	NULL,
	NULL,	NULL,	ediDATA,	0,	0,	0,	NULL,
	NULL,	NULL,	ediATTR,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,		0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,		0,	0,	0,	NULL,
	NULL,	NULL,	0,		0,	0,	0,	NULL
};

static struct menuType mu_edFldMu =
{
	NULL,
	ct_edFldMu,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	5,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	5
};
GLOBALDEF MENU edFldMu = &mu_edFldMu;
# endif



# ifdef FORRBF
static struct com_tab ct_rbfedFldMu1[] =
{
	NULL,	NULL,	ediDATA,	0, 0, 0,	NULL,
	NULL,	NULL,	ediATTR,	0, 0, 0,	NULL,
	NULL,	NULL,	HELP,		0, 0, 0,	NULL,
	NULL,	NULL,	UNDO,		0, 0, 0,	NULL,
	NULL,	NULL,	0,		0, 0, 0,	NULL
};

static struct menuType mu_rbfedFldMu1 =
{
	NULL,
	ct_rbfedFldMu1,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	4,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	4
};
GLOBALDEF MENU rbfedFldMu1 = &mu_rbfedFldMu1;
static struct com_tab ct_rbfedFldMu2[] =
{
	NULL,	NULL,	ediDATA,	0, 0, 0,	NULL,
	NULL,	NULL,	ediATTR,	0, 0, 0,	NULL,
	NULL,	NULL,	ediBREAK,	0, 0, 0,	NULL,
	NULL,	NULL,	HELP,		0, 0, 0,	NULL,
	NULL,	NULL,	UNDO,		0, 0, 0,	NULL,
	NULL,	NULL,	0,		0, 0, 0,	NULL
};

static struct menuType mu_rbfedFldMu2 =
{
	NULL,
	ct_rbfedFldMu2,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	5,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	5
};
GLOBALDEF MENU rbfedFldMu2 = &mu_rbfedFldMu2;
# endif



# ifdef FORRBF
static struct com_tab ct_rbfCursor[] =
{
	NULL,	vfcreate,	CREATE,		0, 0, 0,	NULL,
	NULL,	delCom,		DELETE,		0, 0, 0,	NULL,
	NULL,	edit2com,	EDIT2,		0, 0, 0,	NULL,
	NULL,	selectcom,	SELECT,		0, 0, 0,	NULL,
	NULL,	NULL,		LAYOUT,		0, 0, 0,	NULL,
	NULL,	NULL,		COLOPTS,	0, 0, 0,	NULL,
	NULL,	NULL,		PROFILE,	0, 0, 0,	NULL,
	NULL,	NULL,		ALIGN,		0, 0, 0,	NULL,
	NULL,	NULL,		NAMING,		0, 0, 0,	NULL,
	NULL,	NULL,		UNDO,		0, 0, 0,	NULL,
	NULL,	NULL,		wrWRITE,	0, 0, 0,	NULL,
	NULL,	NULL,		HELP,		0, 0, 0,	NULL,
	NULL,	NULL,		QUIT,		0, 0, 0,	NULL,
	NULL,	NULL,		EXIT,		0, 0, 0,	NULL,
	NULL,	NULL,		0,		0, 0, 0,	NULL
};

static struct menuType mu_rbfCursor =
{
	NULL,
	ct_rbfCursor,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	14,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	14
};
GLOBALDEF MENU rbfCursor = &mu_rbfCursor;
# endif


static struct com_tab ct_rbfSelect[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	LEFT,	0,	0,	0,	NULL,
	NULL,	NULL,	CENTER,	0,	0,	0,	NULL,
	NULL,	NULL,	RIGHT,	0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,	0,	0,	0,	NULL,
	NULL,	NULL,	RCOLUMN,0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_rbfSelect =
{
	NULL,
	ct_rbfSelect,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	8,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	8
};
GLOBALDEF MENU rbfSelect = &mu_rbfSelect;



# ifdef FORRBF
static struct com_tab ct_rbfFldMu[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	LEFT,	0,	0,	0,	NULL,
	NULL,	NULL,	CENTER,	0,	0,	0,	NULL,
	NULL,	NULL,	RIGHT,	0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,	0,	0,	0,	NULL,
	NULL,	NULL,	RCOLUMN,0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_rbfFldMu =
{
	NULL,
	ct_rbfFldMu,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	8,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	8
};
GLOBALDEF MENU rbfFldMu = &mu_rbfFldMu;
# endif



# ifdef FORRBF
static struct com_tab ct_rfColSel[] =
{
	NULL,	NULL,	PLACE,	0,	0,	0,	NULL,
	NULL,	NULL,	LEFT,	0,	0,	0,	NULL,
	NULL,	NULL,	CENTER,	0,	0,	0,	NULL,
	NULL,	NULL,	RIGHT,	0,	0,	0,	NULL,
	NULL,	NULL,	SHIFT,	0,	0,	0,	NULL,
	NULL,	NULL,	HELP,	0,	0,	0,	NULL,
	NULL,	NULL,	UNDO,	0,	0,	0,	NULL,
	NULL,	NULL,	0,	0,	0,	0,	NULL
};

static struct menuType mu_rfColSel =
{
	NULL,
	ct_rfColSel,
	(MOVE |NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	7,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	7
};
GLOBALDEF MENU rbfColSel = &mu_rfColSel;
# endif

static struct com_tab ct_boundmu[] =
{
	NULL,		NULL,		PLACE,		0, 0, 0, NULL,
	NULL,		NULL,		EXPANDIT,	0, 0, 0, NULL,
	NULL,		NULL,		HELP,		0, 0, 0, NULL,
	NULL,		NULL,		QUIT,		0, 0, 0, NULL,
	NULL,		NULL,		0,		0, 0, 0, NULL
};

static struct menuType mu_boundmu =
{
	NULL,
	ct_boundmu,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	4,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	4
};
GLOBALDEF MENU boundMu = &mu_boundmu;


# ifndef FORRBF
static struct com_tab ct_muSequence[] =
{
	NULL,		vfseqchg,	SEQCHANGE,	0, 0, 0, NULL,
	NULL,		vfseqdflt,	SEQDFLT,	0, 0, 0, NULL,
	NULL,		NULL,		UNDO,		0, 0, 0, NULL,
	NULL,		NULL,		HELP,		0, 0, 0, NULL,
	NULL,		NULL,		QUIT,		0, 0, 0, NULL,
	NULL,		NULL,		0,		0, 0, 0, NULL
};

static struct menuType mu_muSequence =
{
	NULL,
	ct_muSequence,
	(NULLRET | 0 | MU_RESET | MU_NEW),
	NULL,
	NULL,
	0,
	0,
	5,
	NULL,
	NULL,
	-1,
	-1,
	-1,
	5
};
GLOBALDEF MENU muSequence = &mu_muSequence;
# endif


/*{
** Name:	IIUFgetmu - Get menu items from message file.
**
** Description:
**	Get the necessary menuu items from the ER message file
**	for VIFRED.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Updates the above menu structures.
**
** History:
**	08/14/87 (dkh) - ER changes.
*/
VOID
IIVFgetmu()
{
	i4 i;

# ifndef FORRBF

	ct2_cursor[0].ct_name = ct_cursor[0].ct_name = ERget(FE_Create);
	ct2_cursor[1].ct_name = ct_cursor[1].ct_name = ERget(FE_Delete);
	ct2_cursor[2].ct_name = ct_cursor[2].ct_name = ERget(FE_Edit);
	ct2_cursor[3].ct_name = ct_cursor[3].ct_name = ERget(FE_Move);
	ct2_cursor[4].ct_name = ct_cursor[4].ct_name = ERget(FE_Undo);
	ct2_cursor[5].ct_name = ct_cursor[5].ct_name = ERget(F_VF0035_Order);
	ct2_cursor[6].ct_name = ct_cursor[6].ct_name = ERget(FE_Save);
#ifndef POPUP_DISABLE
	ct2_cursor[7].ct_name
	    = ct_cursor[7].ct_name = ERget(F_VF001f_DisplayStyle);
	i = 8;
#else
	i = 7;
#endif
	/*
	**  Can not count on order of evaluation to be left to right
	**  on some compilers so we can not use the post-increment
	**  operator that one normally expects to be valid here.
	*/
	ct2_cursor[i].ct_name = ct_cursor[i].ct_name =ERget(F_VF0056_WhersCurs);
	i++;
	ct2_cursor[i].ct_name = ct_cursor[i].ct_name =
		ERget(F_VF00BA_group_move);
	i++;
	ct2_cursor[i].ct_name = ct_cursor[i].ct_name = ERget(F_VF00B8_Align);
	i++;
	ct2_cursor[i].ct_name = ct_cursor[i].ct_name = ERget(FE_Help);
	i++;
	ct2_cursor[i].ct_name = ct_cursor[i].ct_name = ERget(FE_End);
	i++;
	ct_cursor[i].ct_name = ERget(FE_Quit);

	ct_entField[0].ct_name = ERget(F_VF0036_Title);
	ct_entField[1].ct_name = ERget(F_VF0037_DispFmt);
	ct_entField[2].ct_name = ERget(F_VF0038_Attr);
	ct_entField[3].ct_name = ERget(FE_Forget);
	ct_entField[4].ct_name = ERget(FE_Help);
	ct_entField[5].ct_name = ERget(FE_End);

# endif /* FORRBF */

	ct_select[0].ct_name = ERget(FE_Place);
	ct_select[1].ct_name = ERget(F_VF0039_Left);
	ct_select[2].ct_name = ERget(F_VF003A_Center);
	ct_select[3].ct_name = ERget(F_VF003B_Right);
	ct_select[4].ct_name = ERget(F_VF003C_Shift);
	ct_select[5].ct_name = ERget(FE_Help);
	ct_select[6].ct_name = ERget(FE_End);

	ct_selbox[0].ct_name = ERget(FE_Place);
	ct_selbox[1].ct_name = ERget(FE_Help);
	ct_selbox[2].ct_name = ERget(FE_End);

	ct_selbox[0].description = ERget(F_VF00BC_placeexpl);
	ct_selbox[1].description = ERget(F_FE0100_ExplFrameHelp);
	ct_selbox[2].description = ERget(F_FE0102_ExplEnd);

# ifndef FORRBF

	ct_selFldMu[0].ct_name = ERget(FE_Place);
	ct_selFldMu[1].ct_name = ERget(F_VF0039_Left);
	ct_selFldMu[2].ct_name = ERget(F_VF003A_Center);
	ct_selFldMu[3].ct_name = ERget(F_VF003B_Right);
	ct_selFldMu[4].ct_name = ERget(F_VF003C_Shift);
	ct_selFldMu[5].ct_name = ERget(F_VF0036_Title);
	ct_selFldMu[6].ct_name = ERget(F_VF003D_Format);
	ct_selFldMu[7].ct_name = ERget(FE_Help);
	ct_selFldMu[8].ct_name = ERget(FE_End);

# endif /* FORRBF */

# ifdef FORRBF

	ct_rSFldMu[0].ct_name = ERget(FE_Place);
	ct_rSFldMu[1].ct_name = ERget(F_VF0039_Left);
	ct_rSFldMu[2].ct_name = ERget(F_VF003A_Center);
	ct_rSFldMu[3].ct_name = ERget(F_VF003B_Right);
	ct_rSFldMu[4].ct_name = ERget(F_VF003C_Shift);
	ct_rSFldMu[5].ct_name = ERget(FE_Help);
	ct_rSFldMu[6].ct_name = ERget(FE_End);

	ct_rSFldMu[0].description = ERget(F_VF00A0_rbf_place);
	ct_rSFldMu[1].description = ERget(F_VF00A1_rbf_left);
	ct_rSFldMu[2].description = ERget(F_VF00A2_rbf_center);
	ct_rSFldMu[3].description = ERget(F_VF00A3_rbf_right);
	ct_rSFldMu[4].description = ERget(F_VF00A4_rbf_shift);
	ct_rSFldMu[5].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rSFldMu[6].description = ERget(F_VF00A5_rbf_end);

# endif /* FORRBF */

# ifndef FORRBF

	ct_selComMu[0].ct_name = ERget(FE_Place);
	ct_selComMu[1].ct_name = ERget(F_VF003C_Shift);
	ct_selComMu[2].ct_name = ERget(FE_Help);
	ct_selComMu[3].ct_name = ERget(FE_End);

	ct_edFldMu[0].ct_name = ERget(F_VF0036_Title);
	ct_edFldMu[1].ct_name = ERget(F_VF0037_DispFmt);
	ct_edFldMu[2].ct_name = ERget(F_VF0038_Attr);
	ct_edFldMu[3].ct_name = ERget(FE_Help);
	ct_edFldMu[4].ct_name = ERget(FE_End);

# endif /* FORRBF */

# ifdef FORRBF

	ct_rbfedFldMu1[0].ct_name = ERget(F_VF003E_Data);
	ct_rbfedFldMu1[1].ct_name = ERget(F_VF003F_ColOpt);
	ct_rbfedFldMu1[2].ct_name = ERget(FE_Help);
	ct_rbfedFldMu1[3].ct_name = ERget(FE_End);

	ct_rbfedFldMu1[0].description = ERget(F_VF00A6_rbf_display_f);
	ct_rbfedFldMu1[1].description = ERget(F_VF00A7_rbf_colopts);
	ct_rbfedFldMu1[2].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rbfedFldMu1[3].description = ERget(F_VF00A5_rbf_end);

	ct_rbfedFldMu2[0].ct_name = ERget(F_VF003E_Data);
	ct_rbfedFldMu2[1].ct_name = ERget(F_VF003F_ColOpt);
	ct_rbfedFldMu2[2].ct_name = ERget(F_VF0093_BrkOpt);
	ct_rbfedFldMu2[3].ct_name = ERget(FE_Help);
	ct_rbfedFldMu2[4].ct_name = ERget(FE_End);

	ct_rbfedFldMu2[0].description = ERget(F_VF00A6_rbf_display_f);
	ct_rbfedFldMu2[1].description = ERget(F_VF00A7_rbf_colopts);
	ct_rbfedFldMu2[2].description = ERget(F_VF00A8_rbf_brkopts);
	ct_rbfedFldMu2[3].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rbfedFldMu2[4].description = ERget(F_VF00A5_rbf_end);

	ct_rbfCursor[0].ct_name = ERget(FE_Create);
	ct_rbfCursor[1].ct_name = ERget(FE_Delete);
	ct_rbfCursor[2].ct_name = ERget(FE_Edit);
	ct_rbfCursor[3].ct_name = ERget(FE_Move);
	ct_rbfCursor[4].ct_name = ERget(F_VF006E_Layout);
	ct_rbfCursor[5].ct_name = ERget(F_VF003F_ColOpt);
	ct_rbfCursor[6].ct_name = ERget(F_VF0040_RepOpt);
	ct_rbfCursor[7].ct_name = ERget(F_VF00B8_Align);
	ct_rbfCursor[8].ct_name = ERget(F_VF00B6_Name);
	ct_rbfCursor[9].ct_name = ERget(FE_Undo);
	ct_rbfCursor[10].ct_name = ERget(FE_Save);
	ct_rbfCursor[11].ct_name = ERget(FE_Help);
	ct_rbfCursor[12].ct_name = ERget(FE_End);
	ct_rbfCursor[13].ct_name = ERget(FE_Quit);

	ct_rbfCursor[0].description = ERget(F_VF00A9_rbf_create);
	ct_rbfCursor[1].description = ERget(F_VF00AA_rbf_delete);
	ct_rbfCursor[2].description = ERget(F_VF00AB_rbf_edit);
	ct_rbfCursor[3].description = ERget(F_VF00AC_rbf_move);
	ct_rbfCursor[4].description = ERget(F_VF00AD_rbf_layout);
	ct_rbfCursor[5].description = ERget(F_VF00A7_rbf_colopts);
	ct_rbfCursor[6].description = ERget(F_VF00AE_rbf_ropts);
	ct_rbfCursor[7].description = ERget(F_VF00B9_exp_align);
	ct_rbfCursor[8].description = ERget(F_VF00B7_exp_name);
	ct_rbfCursor[9].description = ERget(F_VF00AF_rbf_undo);
	ct_rbfCursor[10].description = ERget(F_VF00B0_rbf_save);
	ct_rbfCursor[11].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rbfCursor[12].description = ERget(F_VF00B2_rbf_end2);
	ct_rbfCursor[13].description = ERget(F_VF00B1_rbf_quit);


	ct_rbfSelect[0].ct_name = ERget(FE_Place);
	ct_rbfSelect[1].ct_name = ERget(F_VF0039_Left);
	ct_rbfSelect[2].ct_name = ERget(F_VF003A_Center);
	ct_rbfSelect[3].ct_name = ERget(F_VF003B_Right);
	ct_rbfSelect[4].ct_name = ERget(F_VF003C_Shift);
	ct_rbfSelect[5].ct_name = ERget(FE_Column);
	ct_rbfSelect[6].ct_name = ERget(FE_Help);
	ct_rbfSelect[7].ct_name = ERget(FE_End);


	ct_rbfSelect[0].description = ERget(F_VF00A0_rbf_place);
	ct_rbfSelect[1].description = ERget(F_VF00A1_rbf_left);
	ct_rbfSelect[2].description = ERget(F_VF00A2_rbf_center);
	ct_rbfSelect[3].description = ERget(F_VF00A3_rbf_right);
	ct_rbfSelect[4].description = ERget(F_VF00A4_rbf_shift);
	ct_rbfSelect[5].description = ERget(F_VF00B3_rbf_column);
	ct_rbfSelect[6].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rbfSelect[7].description = ERget(F_VF00A5_rbf_end);

	ct_rbfFldMu[0].ct_name = ERget(FE_Place);
	ct_rbfFldMu[1].ct_name = ERget(F_VF0039_Left);
	ct_rbfFldMu[2].ct_name = ERget(F_VF003A_Center);
	ct_rbfFldMu[3].ct_name = ERget(F_VF003B_Right);
	ct_rbfFldMu[4].ct_name = ERget(F_VF003C_Shift);
	ct_rbfFldMu[5].ct_name = ERget(FE_Column);
	ct_rbfFldMu[6].ct_name = ERget(FE_Help);
	ct_rbfFldMu[7].ct_name = ERget(FE_End);

	ct_rbfFldMu[0].description = ERget(F_VF00A0_rbf_place);
	ct_rbfFldMu[1].description = ERget(F_VF00A1_rbf_left);
	ct_rbfFldMu[2].description = ERget(F_VF00A2_rbf_center);
	ct_rbfFldMu[3].description = ERget(F_VF00A3_rbf_right);
	ct_rbfFldMu[4].description = ERget(F_VF00A4_rbf_shift);
	ct_rbfFldMu[5].description = ERget(F_VF00B3_rbf_column);
	ct_rbfFldMu[6].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rbfFldMu[7].description = ERget(F_VF00A5_rbf_end);

	ct_rfColSel[0].ct_name = ERget(FE_Place);
	ct_rfColSel[1].ct_name = ERget(F_VF0039_Left);
	ct_rfColSel[2].ct_name = ERget(F_VF003A_Center);
	ct_rfColSel[3].ct_name = ERget(F_VF003B_Right);
	ct_rfColSel[4].ct_name = ERget(F_VF003C_Shift);
	ct_rfColSel[5].ct_name = ERget(FE_Help);
	ct_rfColSel[6].ct_name = ERget(FE_End);

	ct_rfColSel[0].description = ERget(F_VF00A0_rbf_place);
	ct_rfColSel[1].description = ERget(F_VF00A1_rbf_left);
	ct_rfColSel[2].description = ERget(F_VF00A2_rbf_center);
	ct_rfColSel[3].description = ERget(F_VF00A3_rbf_right);
	ct_rfColSel[4].description = ERget(F_VF00A4_rbf_shift);
	ct_rfColSel[5].description = ERget(F_FE0100_ExplFrameHelp);
	ct_rfColSel[6].description = ERget(F_VF00A5_rbf_end);

# endif /* FORRBF */

	ct_boundmu[0].ct_name = ERget(FE_Place);
	ct_boundmu[1].ct_name = ERget(F_VF0041_Expand);
	ct_boundmu[2].ct_name = ERget(FE_Help);
	ct_boundmu[3].ct_name = ERget(FE_End);

	ct_boundmu[0].description = ERget(F_VF00B4_margin_place);
	ct_boundmu[1].description = ERget(F_VF00B5_margin_expand);
	ct_boundmu[2].description = ERget(F_FE0100_ExplFrameHelp);
	ct_boundmu[3].description = ERget(F_VF00A5_rbf_end);
# ifndef FORRBF

	ct_muSequence[0].ct_name = ERget(FE_Edit);
	ct_muSequence[1].ct_name = ERget(F_VF0042_DefOrd);
	ct_muSequence[2].ct_name = ERget(FE_Forget);
	ct_muSequence[3].ct_name = ERget(FE_Help);
	ct_muSequence[4].ct_name = ERget(FE_End);

# endif /* FORRBF */
}


/*{
** Name:	IIVFstrinit - Initialize global strings for vifred.
**
** Description:
**	This routine initializes some global variables to fast
**	strings used by VIFRED.  This eliminates unnecessary
**	calls to ERget.  Resulting in faster (and smaller) code.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Updates global variables to point to fast strings.
**
** History:
**	08/21/87 (dkh) - ER changes.
*/
VOID
IIVFstrinit()
{
    if (IIVF_yes1 == NULL)
    {
	IIVF_yes1 = STalloc(ERget(F_UG0001_Yes1));
	IIVF_no1 = STalloc(ERget(F_UG0006_No1));

	/*
	**  Make sure everything is lowercased.
	*/
	CVlower(IIVF_yes1);
	CVlower(IIVF_no1);
    }
}
