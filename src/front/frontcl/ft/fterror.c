/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"ftframe.h"
#include	<si.h>
#include	"ftfuncs.h"
#include	<erft.h>
# include	<frsctblk.h>

/**
** Name:    fterror.c
**
** History:
**	11/02/85 - (dkh)
**		Added calls to FTdmpmsg to put more information into
**		screen dump files for testing purposes.
**	03/06/87 (dkh) - Added support for ADTs.
**	02/10/87 - (KY)
**		Added handling for KEYSTRUCT structure becouse TDgetc()
**		returns pointer of KEYSTRUCT structure.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	07/27/89 (dkh) - Added special marker parameter in call to fmt_multi.
**	22-mar-90 (bruceb)
**		Added locator support.  Clicking in the error box causes a
**		return.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	05/23/90 (dkh) - Added casts for above NULLPTR change.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**      3/21/91 (elein)         (b35574) Add TRUE parameter to call to
**                              fmt_multi. TRUE is used internally for boxed
**                              messages only.  They need to have control
**                              characters suppressed.
**      26-sep-91 (jillb/jroy--DGC) (from 6.3)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	i4	TDrefresh();
FUNC_EXTERN	i4	TDclear();
FUNC_EXTERN	WINDOW	*TDsubwin();
FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	VOID	IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	i4	IITDlioLocInObj();

static WINDOW	*IIerrwin = NULL;	/* Window to print error message */
static WINDOW	*IImsgarea = NULL;
static WINDOW	*IIprtarea = NULL;
static FMT	*IIerrpfmt = NULL;
static PTR	IIwksp = NULL;
static DB_DATA_VALUE	IIerrdbv = {0};

FTerror (message)
reg char	*message;
{
	reg char	*msgptr; /* pointer to current line in message */
	reg WINDOW	*win;
	KEYSTRUCT	*result;
	char		fmtstr[60];
	char		dmpmsg[MAX_TERM_SIZE + 1];
	i4		fmtsize;
	i4		len;
	ADF_CB		*ladfcb;
	PTR		blk;
	DB_DATA_VALUE	ldbv;
	DB_TEXT_STRING	*text;
	bool		more;
	IICOORD		errbox;	/* Location of the error message. */

	ldbv.db_datatype = DB_CHR_TYPE;
	ldbv.db_length = STlength(message);
	ldbv.db_prec = 0;
	ldbv.db_data = (PTR) message;
	ladfcb = FEadfcb();

	win = IIerrwin;
	if(win == NULL)		/* If the error window does not exist */
	{			/* create it */

		/* Use the bottom five lines */
		IIerrwin = TDnewwin((i4)7, (i4)COLS - 2, (i4)LINES - 8,
			(i4)1);

		if(IIerrwin == NULL)	/* If not created, return error */
		{
			IIUGbmaBadMemoryAllocation(ERx("FTerror"));
			return(-1);
		}
		IIerrwin->_relative = FALSE;	/* window is absolute */

		if ((IImsgarea = TDsubwin(IIerrwin, 4, COLS - 6,
			IIerrwin->_begy + 1, IIerrwin->_begx + 2,
			NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("FTerror"));
			return(-1);
		}
		if ((IIprtarea = TDsubwin(IIerrwin, 1, 12, IIerrwin->_begy + 5,
			COLS - 20, NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("FTerror"));
			return(-1);
		}
		win = IIerrwin;

		STprintf(fmtstr, ERx("cj0.%d"), COLS - 6);
		if ((fmt_areasize(ladfcb, fmtstr, &fmtsize) == OK) &&
			((blk = MEreqmem((u_i4)0, (u_i4)fmtsize, TRUE,
			    (STATUS *)NULL)) != NULL) &&
			(fmt_setfmt(ladfcb, fmtstr, blk, &IIerrpfmt,
				&len) == OK))
		{
			;
		}
		else
		{
			IIerrpfmt = NULL;
		}
		IIerrdbv.db_datatype = DB_LTXT_TYPE;
		IIerrdbv.db_prec = 0;
		IIerrdbv.db_length = COLS - 6 + DB_CNTSIZE;
		if ((IIerrdbv.db_data = MEreqmem((u_i4)0,
		    (u_i4)IIerrdbv.db_length, TRUE, (STATUS *)NULL)) == NULL)
		{
			IIerrpfmt = NULL;
		}
		fmt_workspace(ladfcb, IIerrpfmt, &ldbv, &len);
		if ((IIwksp = MEreqmem((u_i4)0, (u_i4)len, TRUE,
		    (STATUS *)NULL)) == NULL)
		{
			IIerrpfmt = NULL;
		}
	}

	errbox.begy = IIerrwin->_begy;
	errbox.begx = IIerrwin->_begx;
	errbox.endy = IIerrwin->_begy + IIerrwin->_maxy - 1;
	errbox.endx = IIerrwin->_begx + IIerrwin->_maxx - 1;
	IITDpciPrtCoordInfo(ERx("FTerror"), ERx("errbox"), &errbox);

	(*FTdmpmsg)(ERx("\nERROR MESSAGE FROM ERROR FILES THRU FDERROR:\n"));

	TDwinmsg(ERx(" "));			/* Erase current message */
	TDrefresh(win);

	TDbox(win, '|', '-', '+');
	TDsetattr(IIprtarea, (i4) fdRVVID);
	IIprtarea->_cury = IIprtarea->_curx = 0;
	TDaddstr(IIprtarea, ERget(FE_HitEnter));

	for (msgptr = message; *msgptr; msgptr++)
	{
		if (*msgptr == '\n' || *msgptr == '\r')
		{
			*msgptr = ' ';
		}
	}

	msgptr = message;

	IImsgarea->_cury = IImsgarea->_curx = 0;

	if (IIerrpfmt == NULL)
	{
		TDaddstr(IImsgarea, msgptr);
		(*FTdmpmsg)(ERx("%s\n"), msgptr);
	}
	else
	{
		text = (DB_TEXT_STRING *)IIerrdbv.db_data;
		IIfmt_init(ladfcb, IIerrpfmt, &ldbv, IIwksp);
		for (;;)
		{
			if (fmt_multi(ladfcb, IIerrpfmt, &ldbv, IIwksp,
				&IIerrdbv, &more, FALSE, TRUE) != OK)
			{
				return(0);
			}
			if (!more)
			{
				break;
			}
			TDxaddstr(IImsgarea, text->db_t_count, text->db_t_text);
			MEcopy((PTR) text->db_t_text, (u_i2) text->db_t_count,
				(PTR) dmpmsg);
			(*FTdmpmsg)(ERx("%s\n"), dmpmsg);
		}
	}

	(*FTdmpmsg)(ERx("\nEND OF ERROR MESSAGE FROM ERROR FILES\n"));


	TDrefresh(win);				/* Print error message */

# ifdef DATAVIEW
	if (IIMWimIsMws())
	{
		if(IIMWdmDisplayMsg(message, FALSE, (FRS_EVCB *) NULL) == OK)
			return(0);
		else
			return(-1);
	}
# endif	/* DATAVIEW */

	do					/* Wait for carriage return */
	{
		result = FTgetc(stdin);
		if (result->ks_fk == KS_LOCATOR)
		{
		    if (IITDlioLocInObj(&errbox, result->ks_p1, result->ks_p2))
		    {
			IITDposPrtObjSelected(ERx("errbox"));
			break;
		    }
		    else
		    {
			FTbell();
		    }
		}
	} while (result->ks_ch[0] != '\r' && result->ks_ch[0] != '\n'
			&& result->ks_ch[0] != ' ');
	TDclear(win);				/* Clear window */
	TDrefresh(win);			/* Clear bottom of the screen */
	return(0);			/* return INGRES error occured */
}
