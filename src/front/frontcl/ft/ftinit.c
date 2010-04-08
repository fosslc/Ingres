/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
**
**  FTinit
**
**	History
**	85/09/19  14:43:02  cgd
**		Cast all froperation, fropflg entries to i2.
**	85/10/08  13:13:59  jas
**		ibs integration
**	85/10/31  21:00:35  dave
**		Integrated porting changes. (dkh)
**	85/11/19  10:26:33  bruceb
**		ifdef'd code so that SEINGRES version would link:
**		ftgetch.c -- use TEget() instead of (*in_func)().
**		ftinit.c -- don't call FKinit_keys().
**	01/27/86 (garyb) Add support for EBCDIC.
**	86/03/14  12:24:19  garyb
**		IBM EBCDIC support.
**	10/01/86 (a.dea)
**		Changed froperation table to
**              assign fdopERR to all control keys and escape
**              and delete key for EBCDIC (for QA test).
**	86/11/18  21:50:49  dave
**		Initial60 edit of files
**	86/11/21  07:43:44  joe
**		Lowercased FT*.h
**	12/24/86 (dkh) Added support for new activations.
**	16-mar-87 (bruceb)
**		changed code for the space character from fdopRIGHT to
**		fdopSPACE for Hebrew project.  Done for both ASCII (0x20)
**		and EBCDIC (0x40) arrays since this is the version of FT
**		used by IBM for running QA tests and NOT used otherwise
**		for EBCDIC.
**	05/02/87 (dkh) - Changed to accept control block with utility procedure
**			 pointers used by the FT layer.
**	08/14/87 (dkh) - ER changes.
**	11/11/87 (dkh) - Code cleanup.
**	05/04/88 (dkh) - Venus changes.
**	07/23/88 (dkh) - Added exec sql immediate hook.
**	10/27/88 (dkh) - Performance changes.
**	11/13/88 (dkh) - Rearranged code so non-forms programs can link
**			 in IT* routines.
**	28-nov-88 (bruceb)
**		Added an event control arg to FTinit.
**	13-mar-89 (bruceb)
**		Initialize IIFTevcb at top, not at bottom, of FTinit().
**	16-mar-89 (bruceb)
**		Interface changed to get an FRS_CB instead of an FRS_EVCB.
**		Done for entry activation.
**	24-apr-89 (bruceb)
**		Set IIFTglcb for shell_enabled value.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	10/19/89 (dkh) - Added new function pointers to remove duplicate
**			 FT files in GT directory.
**	27-feb-90 (bruceb)
**		Inquire after the II_FRS_KEYFIND logical.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	07/10/90 (dkh) - Integrated more MACWS changes.
**	07/24/90 (dkh) - A more complete fix for bug 30408.
**	08/18/90 (dkh) - Fixed bug 31360.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	08/21/91 (dkh) - Fixed previous integration so things can link.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	18-dec-1996 (canor01)
**		Include size of arrays moved to ftdata.c to keep sizeof happy.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**          between compat.h and system limits.h on (sqs_ptx).
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add IIJS capabability flag to IIMWiInit for Java servlet.
**	06-sep-2005 (abbjo03)
**	    Copy return of NMgtAt to its own buffer so it will still be
**	    available when STbcompare is called.
*/

# include	<compat.h>
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<tdkeys.h>
# include	<mapping.h>
# include	<ctrl.h>
# include	<er.h>
# include	<erft.h>


GLOBALREF	i4	IITDAllocSize;

/* window to display frame driver mode */
GLOBALREF WINDOW	*frmodewin;

/* window to display frame driver messages */
/*GLOBALREF WINDOW	*msgwin;	*/

/* window for displaying all frames */
GLOBALREF WINDOW	*FTwin;
GLOBALREF WINDOW	*FTfullwin;

/* window for field cursor is currently on */
GLOBALREF WINDOW	*FTcurwin;

GLOBALREF WINDOW	*FTutilwin;	/* utility window */
GLOBALREF WINDOW	*IIFTdatwin;	/* window for fld data area */

GLOBALREF i4	FTwmaxy;
GLOBALREF i4	FTwmaxx;

GLOBALREF FRAME	*FTiofrm;

GLOBALREF FRS_EVCB	*IIFTevcb;
GLOBALREF FRS_AVCB	*IIFTavcb;
GLOBALREF FRS_GLCB	*IIFTglcb;

GLOBALREF char	*IIFTmpbMenuPrintBuf;

GLOBALREF	u_i2	FTgtflg;

GLOBALREF	VOID	(*iigtclrfunc)();
GLOBALREF	VOID	(*iigtreffunc)();
GLOBALREF	VOID	(*iigtctlfunc)();
GLOBALREF	VOID	(*iigtsupdfunc)();
GLOBALREF	bool	(*iigtmreffunc)();
GLOBALREF	VOID	(*iigtslimfunc)();
GLOBALREF	i4	(*iigteditfunc)();

GLOBALREF i2	froperation[IIMAX_CHAR_VALUE+2];
GLOBALREF i1	fropflg[IIMAX_CHAR_VALUE+2];
GLOBALREF	struct	frsop	frsoper[MAX_CTRL_KEYS + KEYTBLSZ];

STATUS
FTinit(proccb, frscb, mubuf)
FRS_UTPR	*proccb;
FRS_CB		*frscb;
char		*mubuf;
{
	WINDOW	*TDsubwin();
	char	*sp;
        char	spbuf[16];

	IIFTevcb = frscb->frs_event;
	IIFTavcb = frscb->frs_actval;
	IIFTglcb = frscb->frs_globs;

	IIFTmpbMenuPrintBuf = mubuf;

	FTprocinit(proccb);

	if(TDinitscr() != OK)
	{
		return(FAIL);
	}

# ifdef	PMFE
	FTmuset();	/* For ring menus */
# endif

	if((frmodewin = TDsubwin(stdmsg, (i4)1, (i4)14,
		(i4)LINES - 1, (i4)COLS - 15, NULL)) == NULL)
	{
		return(FAIL);
	}

	FTwmaxy = LINES + LINES;
	FTwmaxx = IITDAllocSize + IITDAllocSize;

	if (TDonewin(&FTfullwin, &FTcurwin, &FTutilwin, FTwmaxy, FTwmaxx) != OK)
	{
		return(FAIL);
	}

	if ((IIFTdatwin = TDsubwin(FTfullwin, FTfullwin->_maxy,
		FTfullwin->_maxx, 0, 0, NULL)) == NULL)
	{
		return(FAIL);
	}

	/*
	**  Set initial size of FTfullwin to be the physical
	**  size of the terminal minus menu and DG/CEO line.
	*/
	FTfullwin->_maxy = LINES - 1 - IITDflu_firstlineused;
	FTfullwin->_maxx = COLS;

# ifdef DATAVIEW
	/*
	**  Initialize MWS here so that the message is properly
	**  taken care of.
	*/
	if (IIMWiInit( (i4) IIMW, (i4) IIJS ) != OK)
	{
		return(FAIL);
	}
# endif /* DATAVIEW */

	if (FTmuinit() != OK)
	{
		return(FAIL);
	}

	FTwin = FTfullwin;

	FTmessage(ERx("    "), FALSE, FALSE);

	FKinit_keys();	/* initialize function keys, if any */

# ifdef DATAVIEW
	if (IIMWkmiKeyMapInit(keymap, MAX_CTRL_KEYS + KEYTBLSZ + 4,
		frsoper, sizeof(frsoper) / sizeof(struct frsop),
		froperation, (sizeof(froperation) / sizeof(i2)) - 1,
		fropflg) != OK)
	{
		_VOID_ IIMWcClose("");
		return(FAIL);
	}
# endif	/* DATAVIEW */

	NMgtAt(ERx("II_FRS_KEYFIND"), &sp);
	if (sp && *sp)
	    STlcopy(sp, spbuf, sizeof(spbuf) - 1);
	if ((sp == NULL) || (*sp == EOS)
	    || (STbcompare(ERget(F_FT0011_No_Keyfind), 0, spbuf, 0, TRUE) != 0))
	{
	    /*
	    ** If II_FRS_KEYFIND is not set, or is not set to 'false', enable
	    ** the feature.
	    */
	    IIFTglcb->enabled |= KEYFIND;
	}

	return(OK);
}
