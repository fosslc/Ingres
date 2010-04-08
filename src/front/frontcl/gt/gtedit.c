/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<te.h>
# include	<cm.h>
# include	<termdr.h>
# include	<graf.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	<kst.h>
# include	<gtdef.h>
# include	"ergt.h"
# include	<ug.h>

/**
** Name:	gtedit.c -	Graphics System Edit Module.
**
** Description:
**	This module handles interactive editing of graphics objects in the
**	Graphics system through the Forms system interface.  It defines:
**
**	GTedit
**	GTmsg
**	GTfmsg_pause()	print message and wait for return.
**	GTpoint
**	GTpark
**	GTgetch
**	GTaflush
**	GTcursync()	synchronize cursor with frame driver.
**	GTtabpath()	set explicit tab path for next GTedit/GTpoint
**
** History:
**		85/09/30  15:02:59  bobm
**		Initial revision.
**
**		86/03/14  12:15:57  bobm
**		tab stop setting - GTtabpath
**
**		86/04/02  15:24:43  bobm
**		move to next tab stop if you're already on the first one
**
**		86/04/16  10:14:01  wong
**		Pass through escape sequence to GKS device.
**
**		5/27/86 (bruceb)	- allow tabbing even if only one item
**		in the graph.  (fix for bug 9070)  Also allow
**		tabbing if only one item is visible even though
**		covering others.
**	25-nov-86 (bruceb) fix for b9788
**		Allow GTtabpath to be called with a NULL path
**		as means of turning off the tabbing function.
**		In GTedit, no longer reset Tabpath at end of
**		function--done instead in vigraph!ed[bar|line|scat].qc.
**		(This allows tabbing between axes after a ^W.)
**	18-dec-86 (bruceb) Bug 9314
**		Sound the bell instead of back-spacing when
**		an unmapped function key is pressed.
**	24-sep-87 (bruceb)
**		Cast arg to ERget in border_err to ER_MSGID.
**		Added call on FTshell for the shell function key.
**	29-sep-87 (bruceb)
**		Restrict calls on FTshell to from within GTedit
**		(can't call from within GTpoint.)
**	17-nov-88 (bruceb)
**		Added dummy arg in call on TEget.  (arg used in timeout.)
**	2-Feb-89  (Mike S.)
**		Check for non-standard backspace and cursor-down commands.
**	27-Mar-89 (Mike S)
**		Add GTlock calls for DG.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	28-mar-90 (bruceb)
**		Added locator support.  If under GTpoint(), can only click
**		on the graph; if under GTedit(), can also click on the menu.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**	21-jan-93 (pghosh)
**		Modified COORD to IICOORD. This change was supposed to be 
**		done by leighb when the ftdefs.h file was modified to have 
**		IICOORD.
**	26-Aug-1993 (fredv) - Included <st.h>.
**	09-Mar-1999 (schte01)
**       Changed idx and ord assignments so there are no dependencies on order 
**       of evaluation (in tab_jump and srch_next routines respectively).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototypes for do_cursor(), border_err(), tab_next(),
**	  tab_prev(), tab_jump(), srch_next(), srch_prev(), sync_loc(),
**	  set_cobj() & cursor_ext() to prevent compiler errors with
**	  GCC 4.0 due to conflict with implicit declaration.
**/

GLOBALREF bool fdrecover;

GLOBALREF char *G_tc_cm, *G_tc_up, *G_tc_nd, *G_tc_ce, *G_tc_do, *G_tc_bc;
GLOBALREF char G_last[];
GLOBALREF bool G_mref;
GLOBALREF bool G_plotref;
GLOBALREF bool G_locate;
GLOBALREF bool G_restrict;

GLOBALREF WINDOW *curscr;

GLOBALREF i4  G_lines;

GLOBALREF GR_FRAME *G_frame;

i4	GTchout ();
char	*TDtgoto();
VOID	GTmsg();	/* forward reference */
bool	GTovlp();
VOID	IIGTfldFrmLocDecode();

FUNC_EXTERN	VOID	IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	VOID	IIFTmldMenuLocDecode();
FUNC_EXTERN	i4	IITDlioLocInObj();
FUNC_EXTERN	VOID	IIFTsmShiftMenu();

static i4 do_cursor();
static i4 border_err();
static i4 tab_next();
static i4 tab_prev();
static i4 tab_jump();
static STATUS srch_next();
static STATUS srch_prev();
static i4 sync_loc();
static i4 set_cobj();
static i4 cursor_ext();

static i4  Loc_row, Loc_col;
static float Loc_x, Loc_y;
static GR_OBJ *Cur_obj;
static i4  Comps;
static bool Sortflag;
static i4  Bellcount,Keycount;

/*
** Set in do_cursor to indicate the legal screen region for cursor motion.
** Used also by IIGTfld() for locator support.
*/
static i4	c_low, c_hi;	/* column range */
static i4	r_low, r_hi;	/* row range */

static bool	loc_w_menu = TRUE;	/* Can click locator on menuline. */

/* alternate tab path */
static XYPOINT *Tabpath = NULL;
static i4  Tablen;

/*{
** Name:	GTcursync() -	Synchronize cursor with Frame Driver.
**
**	This routine moves the screen cursor to the position
**	in which the Frame driver left it.  The alternate entry point
**	(GTcsunl) restores the cursor and then unlocks the screen.
*/

VOID
GTcursync ()
{
	TDtputs(TDtgoto(G_tc_cm, curscr->_curx, curscr->_cury), 1, GTchout);
}
# ifdef DGC_AOS
VOID
GTcsunl ()
{
	TDtputs(TDtgoto(G_tc_cm, curscr->_curx, curscr->_cury), 1, GTchout);
	TEflush();
	GTlock(FALSE, curscr->_cury, curscr->_curx);
}
# endif
/* local defines for efficiency */
# define GTcursync() \
	TDtputs(TDtgoto(G_tc_cm, curscr->_curx, curscr->_cury), 1, GTchout)
# define GTcsunl()  \
	{ TDtputs(TDtgoto(G_tc_cm, curscr->_curx, curscr->_cury), 1, GTchout); \
	  TEflush(); \
	  GTlock(FALSE, curscr->_cury, curscr->_curx); }
/*{
** Name:	GTtabpath - set tab path for next GTedit/GTpoint call
**
**	NOTE: storage pointed to by path must still be valid at the time
**		of the next GTpoint/GTedit call.
**
**	Call with NULL path to explicitly turn off special tab path.
**
** Inputs:
**		path: points for alternate tab path.
**		len: number of points.
*/
GTtabpath(path, len)
XYPOINT *path;
i4  len;
{
	Tablen = len;
	Tabpath = path;
}

/*{
** Name:	GTgetch()
**
**	"getchar" for GT lib.  Handle keystroke collection
*/
KEYSTRUCT *
GTgetch ()
{
	KEYSTRUCT *key;
	KEYSTRUCT *TDgetch();

	TEflush();

	key = TDgetch(curscr);

	/* mimic FTgetch decision */
	if (!KY && fdrecover)
	{
		TDrcvstr(key->ks_ch);
	}

	return (key);
}

/*	TEflush cannot be used in files that include gks.h (TEflush is
**	a macro).
*/
VOID
GTaflush()
{
	TEflush();
}

VOID
GTpark (x, y)
float x,y;
{
	i4 r,c;

	G_frame->cx = x;
	G_frame->cy = y;
	GTxftoa(x, y, &r, &c);
	G_frame->crow = r;
	G_frame->ccol = c;
}

VOID
GTpoint (x, y, ext)
float *x, *y;
register EXTENT *ext;
{
	FRS_CB cb;
	FRS_EVCB ev;

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif

	/* data coming back from do_cursor which will be ignored */
	cb.frs_event = &ev;

	loc_w_menu = FALSE;
	_VOID_ do_cursor(ext, FALSE, G_locate, &cb);
	loc_w_menu = TRUE;

	if (Loc_x > ext->xhi)
		Loc_x = ext->xhi;
	if (Loc_x < ext->xlow)
		Loc_x = ext->xlow;
	if (Loc_y > ext->yhi)
		Loc_y = ext->yhi;
	if (Loc_y < ext->ylow)
		Loc_y = ext->ylow;
	*x = Loc_x;
	*y = Loc_y;
	Tabpath = NULL;
# ifdef DGC_AOS
	GTcsunl();
# else
	GTcursync();
# endif
}

i4
GTedit (frscb)
FRS_CB *frscb;
{
	i4	code;

	if (G_plotref)
	{
		G_plotref = FALSE;
		return (fdopREFR);
	}

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	code = do_cursor(G_frame->lim, TRUE, G_locate, frscb);
	if (G_restrict)
		GTsubcomp(Loc_x, Loc_y);
	else
	{
		set_cobj(G_locate);
		G_frame->cur = Cur_obj;
		if (Cur_obj != NULL)
			G_frame->comp = Cur_obj->type;
		else
			G_frame->comp = 0;
	}

	G_frame->crow = Loc_row;
	G_frame->ccol = Loc_col;
	G_frame->cx = Loc_x;
	G_frame->cy = Loc_y;
# ifdef DGC_AOS
	GTcsunl();
# else
	GTcursync();
# endif
	return code;
}

static
do_cursor (ext, frm, locator, frscb)
register EXTENT *ext;
bool		frm;		/* from frame system */
register bool	locator;	/* use locator device */
FRS_CB		*frscb;		/* return key data to FRS */
{
	i4		code;		/* key code from FRS */
	i4		row;
	i4		col;
	FRS_EVCB	*evcb;		/* control block data - return to FRS */
	KEYSTRUCT	*key;
	IICOORD		obj;
	i4		event;

	char	*TDtgoto();

	evcb = frscb->frs_event;

	G_mref = FALSE;
	GTstlupdt (FALSE);

	/* remember floating point y values REVERSED from rows */
	GTxftoa (ext->xlow, ext->yhi, &r_low, &c_low);
	GTxftoa (ext->xhi, ext->ylow, &r_hi, &c_hi);

	obj.begy = r_low;
	obj.begx = c_low;
	obj.endy = r_hi;
	obj.endx = c_hi;
	IITDpciPrtCoordInfo(ERx("do_cursor"), ERx("graphics"), &obj);

	/* get current row and column */
	Loc_col = G_frame->ccol;
	Loc_row = G_frame->crow;
	if (Loc_col < c_low)
		Loc_col = c_low;
	if (Loc_col > c_hi)
		Loc_col = c_hi;
	if (Loc_row < r_low)
		Loc_row = r_low;
	if (Loc_row > r_hi)
		Loc_row = r_hi;

	sync_loc(FALSE);

	TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row), 1, GTchout);

	Sortflag = TRUE;
	code = 0;
	Keycount = 0;
	Bellcount = -1;

	for (;;)
	{
		register i4	oldcode;
		u_char		ch;
		i4		ignore;
		bool		junk;
		i4		tidx;

		if (locator)
		{
			register STATUS loc_stat;

			loc_stat = GTlocdev(&Loc_x, &Loc_y, ext, TRUE);
			if (loc_stat == OK)
			{
				sync_loc(locator);
				return (fdopMENU);
			}

			locator = FALSE;
			if (loc_stat < 0)
				border_err(F_GT0001_No_mouse);
			else
				sync_loc(locator);

			TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row), 1, GTchout);
			continue;
		}

		oldcode = code;
		++Keycount;
		evcb->event = 0;
		if (FKmapkey((key = GTgetch()), IIGTfldFrmLocDecode, &code,
		    &ignore, &junk, &ch, evcb) != OK)
		{
			FTbell();
			if (evcb->event != fdNOCODE)
			{
			    /* Not just a bad click of the locator button. */
			    border_err(F_GT0002_undefined_keystroke);
			}
			continue;
		}

		/*
		** if fdopERR & not a function key or multikey
		** character, put actual character into code
		*/
		if (code == fdopERR && key->ks_fk == 0 &&
						!CMdbl1st(key->ks_ch))
		{
			code = (i4)((key->ks_ch)[0]);
		}

		switch (code)
		{
		case fdopLEFT:
			if (Loc_col > c_low)
			{
				if (G_tc_bc == (char *) NULL || *G_tc_bc == EOS)
				    	TEput ('\010');
				else
					TDtputs(G_tc_bc, 1, GTchout);
				--Loc_col;
			}
			else
				border_err (F_GT0003_left_edge);
			break;
		case fdopDOWN:
			if (Loc_row < r_hi)
			{
				if (G_tc_do == (char *) NULL || *G_tc_do == EOS)
				    	TEput ('\012');
				else
					TDtputs(G_tc_do, 1, GTchout);
				++Loc_row;
			}
			else
				border_err (F_GT0004_bottom_edge);
			break;
		case fdopUP:
			if (Loc_row > r_low)
			{
				TDtputs(G_tc_up, 1, GTchout);
				--Loc_row;
			}
			else
				border_err (F_GT0006_top_edge);
			break;
		case fdopRIGHT:
			if (Loc_col < c_hi)
			{
				++Loc_col;
				TDtputs(G_tc_nd, 1, GTchout);
			}
			else
				border_err (F_GT0007_right_edge);
			break;
		case fdopGOTO:
			row = evcb->gotorow;
			col = evcb->gotocol;
			TDtputs(TDtgoto(G_tc_cm, col, row), 1, GTchout);
			Loc_row = row;
			Loc_col = col;
			break;
		case fdopNROW:
		case fdopNEXT:
			if (Tabpath != NULL)
			{
				if (oldcode != fdopNROW && oldcode != fdopPREV && oldcode != fdopNEXT)
					tidx = tab_next();
				else
					++tidx;
				tab_jump(tidx);
				break;
			}
			if (G_restrict)
			{
				border_err(F_GT0008_no_subcomponents);
				break;
			}
			if (srch_next(locator) != OK)
				border_err(F_GT0009_no_next_comp);
			else
				TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row),
								1, GTchout);
			break;
		case fdopPREV:
			if (Tabpath != NULL)
			{
				if (oldcode != fdopNROW && oldcode != fdopPREV && oldcode != fdopNEXT)
					tab_prev();
				else
					--tidx;
				tab_jump(tidx);
				break;
			}
			if (G_restrict)
			{
				border_err(F_GT0008_no_subcomponents);
				break;
			}
			if (srch_prev(locator) != OK)
				border_err(F_GT000A_no_prev_comp);
			else
				TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row),
								1, GTchout);
			break;
		case fdopMENU:
			sync_loc(locator);
			event = evcb->event;
			if ((event == fdopSCRLT) || (event == fdopSCRRT))
			{
			    IIFTsmShiftMenu(event);
			    evcb->event = fdopMENU;
			}
			return fdopMENU;
			break;
		case fdopREFR:
			if (frm)
			{
				sync_loc(locator);
				return fdopREFR;
			}
			border_err (F_GT000B_refresh_fail);
			break;
		case (i4) '=':
			GTmsg(G_last);
			TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row),
								1, GTchout);
			break;
		case (i4) '^':
			sync_loc(locator);
			locator = TRUE;
			break;
		case (i4) '}':
			sync_loc(locator);
			set_cobj(locator);
			if (G_restrict || Cur_obj == NULL || oldcode == (i4) '}')
			{
				if (Loc_row >= r_hi)
				{
					border_err (F_GT0004_bottom_edge);
					break;
				}
				Loc_row = r_hi;
			}
			else
				GTxftoa (Loc_x, Cur_obj->ext.ylow, &Loc_row,
								&Loc_col);
			TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row),
								1, GTchout);
			break;
		case (i4) '{':
			sync_loc(locator);
			set_cobj(locator);
			if (G_restrict || Cur_obj == NULL || oldcode == (i4) '{')
			{
				if (Loc_row <= r_low)
				{
					border_err (F_GT0006_top_edge);
					break;
				}
				Loc_row = r_low;
			}
			else
				GTxftoa (Loc_x, Cur_obj->ext.yhi, &Loc_row,
								&Loc_col);
			TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row), 1, GTchout);
			break;
		case fdopBEGF:
			sync_loc(locator);
			set_cobj(locator);
			if (Cur_obj == NULL || oldcode == fdopBEGF)
			{
				if (Loc_col <= c_low)
				{
					border_err (F_GT0003_left_edge);
					break;
				}
				Loc_col = c_low;
			}
			else
				GTxftoa (Cur_obj->ext.xlow, Loc_y, &Loc_row,
								&Loc_col);
			TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row), 1, GTchout);
			break;
		case fdopENDF:
			sync_loc(locator);
			set_cobj(locator);
			if (Cur_obj == NULL || oldcode == fdopENDF)
			{
				if (Loc_col >= c_hi)
				{
					border_err (F_GT0007_right_edge);
					break;
				}
				Loc_col = c_hi;
			}
			else
				GTxftoa (Cur_obj->ext.xhi, Loc_y, &Loc_row,
								&Loc_col);
			TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row), 1, GTchout);
			break;

		case fdopSHELL:
			/* Restriction: not usable when called from GTpoint */
			if (frm)
			{
				FTshell();
				sync_loc(locator);
				return(fdopREFR);
			}
			border_err (F_GT000B_refresh_fail);
			break;

		default:
			if (evcb->event == fdopFRSK)
			{
				sync_loc(locator);
				return (code);
			}
			border_err (F_GT0002_undefined_keystroke);
			break;
		}	/* end switch */
	}	/* end for (never reached) */
}

VOID
GTmsg (s)
char *s;
{
	extern i4  G_homerow;

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	G_mref = TRUE;
	GTdmpmsg (s);
	TDtputs(TDtgoto(G_tc_cm, 0, G_MROW), 1, GTchout);
	TEwrite(s, (i4) STlength(s));
	TDtputs(G_tc_ce, 1, GTchout);
	TEflush();
# ifdef DGC_AOS
	GTlock(FALSE, G_homerow, 0);
# endif
}

static
border_err (n)
i4  n;
{
	GTsetwarn(ERget((ER_MSGID) n));
	if ((Bellcount + 1) < Keycount)
		FTbell();
	Bellcount = Keycount;
}

/*
** routines for alternate tab path:
**	tab_next finds nearest "next" loc.
**	tab_prev finds nearest "prev" loc.
**	tab_jump moves cursor
*/
static
tab_next ()
{
	register i4  i;
	i4 ret;
	float		low, high, diff, tmp;

	sync_loc(FALSE);
	low = Loc_y - G_frame->csy/2.0;
	high = Loc_y + G_frame->csy/2.0;
	diff = 9.9E12;
	ret = -1;

	/*
	** first, try to find something to the right
	*/
	for (i=0; i < Tablen; ++i)
	{
		if (Tabpath[i].x < Loc_x || Tabpath[i].y < low ||
							Tabpath[i].y >high)
			continue;
		if ((tmp = Tabpath[i].x - Loc_x) < diff)
		{
			diff = tmp;
			ret = i;
		}
	}
	if (ret > 0)
		return(ret);

	/*
	** nothing to the right.  Look for something underneath.
	*/
	for (i=0; i < Tablen; ++i)
	{
		if (Tabpath[i].y < high)
			continue;
		tmp = (Tabpath[i].y - Loc_y)/G_frame->csy + Tabpath[i].x;
		if (tmp < diff)
		{
			diff = tmp;
			ret = i;
		}
	}
	if (ret > 0)
		return(ret);

	/*
	** nothing underneath.	pick nearest the top
	*/
	for (i=0; i < Tablen; ++i)
	{
		tmp = (1.0 - Tabpath[i].y)/G_frame->csy + Tabpath[i].x;
		if (tmp < diff)
		{
			diff = tmp;
			ret = i;
		}
	}
	return(ret);
}

static
tab_prev ()
{
	register i4  i;
	i4 ret;
	float		low, high, diff, tmp;

	sync_loc(FALSE);
	low = Loc_y - G_frame->csy/2.0;
	high = Loc_y + G_frame->csy/2.0;
	diff = 9.9E12;
	ret = -1;

	/*
	** first, try to find something to the left
	*/
	for (i=0; i < Tablen; ++i)
	{
		if (Tabpath[i].x > Loc_x || Tabpath[i].y < low ||
							Tabpath[i].y >high)
			continue;
		if ((tmp = Loc_x - Tabpath[i].x) < diff)
		{
			diff = tmp;
			ret = i;
		}
	}
	if (ret > 0)
		return(ret);

	/*
	** nothing to the left.	 Look for something above.
	*/
	for (i=0; i < Tablen; ++i)
	{
		if (Tabpath[i].y > low)
			continue;
		tmp = (Loc_y - Tabpath[i].y)/G_frame->csy + Tabpath[i].x;
		if (tmp < diff)
		{
			diff = tmp;
			ret = i;
		}
	}
	if (ret > 0)
		return(ret);

	/*
	** nothing underneath.	pick nearest the bottom
	*/
	for (i=0; i < Tablen; ++i)
	{
		tmp = Tabpath[i].y/G_frame->csy + Tabpath[i].x;
		if (tmp < diff)
		{
			diff = tmp;
			ret = i;
		}
	}
	return(ret);
}

static
tab_jump (idx)
register i4  idx;
{
	i4 old_row, old_col;

	/* don't trust mods of negative values */
	if (Tablen > 1)
	{
		if (idx < 0)
			idx += (-idx / Tablen + 1)*Tablen;
		idx %= Tablen;
	}
	else
		idx = 0;

	old_row = Loc_row;
	old_col = Loc_col;

	Loc_x = Tabpath[idx].x;
	Loc_y = Tabpath[idx].y;
	sync_loc(TRUE);

	/*
	** if we're still the same place, go to next tab stop
	** this should mainly be taking care of initial tab
	** key for which the cursor is already parked on a tab
	** location.
	*/
	if (Loc_row == old_row && Loc_col == old_col && Tablen > 1)
	{
		++idx;
		idx = idx % Tablen;
		Loc_x = Tabpath[idx].x;
		Loc_y = Tabpath[idx].y;
		sync_loc(TRUE);
	}

	TDtputs(TDtgoto(G_tc_cm, Loc_col, Loc_row), 1, GTchout);
}

/*
** bolzano finds a visible spot on an object by dividing the extent by
** the divx, divy arguments to obtain points to try, and if not visible
** making a recursive call with divx and divy increased.  No increase
** when a parameter results in a division smaller than a cursor block, and
** return FAIL when no visible point is found, and neither parameter can
** be bumped.  Sets Loc_x and Loc_y in any case.  Called initially with
** divx = divy = 2.  Probably easier to show pictorially:
**
**	      ---------------		 ----------------
**	      |		    |		 |		|
**	      |		    |		 |    X	   X	|
**   level 1  |	     X	    |	level 2	 |		|  and so on ...
**	      |		    |		 |    X	   X	|
**	      |		    |		 |		|
**	      ---------------		 ----------------
**
** divx and divy are increased according to new = old * 2 - 1, which moves
** you out to the edge pretty quickly without retrying many points ( 2, 3,
** 5, 9, 17, 33, 65 ... retry 4 points at level 4 and 6, retry 16 at level
** 7).
*/

static bool
bolzano (ptr, divx, divy, locator)
register GR_OBJ *ptr;
register i4	divx,
		divy;
bool		locator;
{
	register i4	i, j;
	float		xinc, yinc, x, y;

	xinc = (ptr->ext.xhi - ptr->ext.xlow)/((float) divx);
	yinc = (ptr->ext.yhi - ptr->ext.ylow)/((float) divy);
	x = ptr->ext.xlow;

	for (i=1; i < divx; ++i)
	{
		x += xinc;
		y = ptr->ext.ylow;
		for (j=1; j < divy; ++j)
		{
			y += yinc;
			GTxftoa (x, y, &Loc_row, &Loc_col);
			sync_loc(locator);
			set_cobj(locator);
			if (Cur_obj == ptr)
				return (TRUE);
		}
	}

	i = divx;
	j = divy;
	if (xinc >= G_frame->csx)
		divx = 2*divx - 1;
	if (yinc >= G_frame->csy)
		divy = 2*divy - 1;

	if (i == divx && j == divy)
		return (FALSE);

	return (bolzano(ptr, divx, divy, locator));
}

/*
**	dumbsort fills in scratch items with order for tab key.	 The list
**	is linked in drawing order, which must be maintained, and is
**	probably different.  Turns off Sortflag, sets Comps.  Components in
**	unshown layers will be left with -1 in the scratch item, and
**	Comps will on count those shown.
*/
static
dumbsort ()
{
	GR_OBJ	*ptr, *best;
	i4 count;
	float	dist, new;
	float	cx, cy;

	Comps = 0;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->layer > G_frame->clayer)
			continue;
		ptr->scratch = -1;
		++Comps;
	}

	/*
	** the sort - very dumb.  However, we have a short list.
	** Using a better technique isn't worth the extra complexity.
	** We simply zap through the list Comps times, finding the
	** largest remaining element, and filling it in.
	*/
	for (count = Comps - 1; count >= 0; --count)
	{
		dist = -1.0;
		best = NULL;
		for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		{
			if (ptr->layer > G_frame->clayer)
				continue;
			if (ptr->scratch >= 0)
				continue;
			cx = (ptr->ext.xlow + ptr->ext.xhi) / 2.0;
			cy = (ptr->ext.ylow + ptr->ext.yhi) / 2.0;
			new = cx + (1.0 - cy) / G_frame->csy;
			if (new > dist)
			{
				best = ptr;
				dist = new;
			}
		}
		if (best == NULL)
			GTsyserr(S_GT0001_GTedit_sort_err,0);
		best->scratch = count;
	}

	Sortflag = FALSE;
}

static GR_OBJ *
near_next ()
{
	GR_OBJ	*ptr, *best;
	float	dist, new;
	EXTENT ext;

	/*
	** first, try to find something to the right
	*/
	ext.xlow = Loc_x;
	ext.xhi = 1.0;
	ext.ylow = Loc_y - G_frame->csy / 2.0;
	ext.yhi = Loc_y + G_frame->csy;
	dist = 1.0;
	best = NULL;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch < 0)
			continue;
		if (GTovlp(&ext, &(ptr->ext)))
		{
			new = ptr->ext.xlow - Loc_x;
			if (new < dist)
			{
				best = ptr;
				dist = new;
			}
		}
	}
	if (best != NULL)
		return(best);
	/*
	** nothing to the right.  Look for something underneath.
	*/
	best = NULL;
	dist = 2.0 / G_frame->csy;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch < 0)
			continue;
		if (ptr->ext.yhi > Loc_y)
			continue;
		new = (Loc_y - ptr->ext.yhi)/G_frame->csy + ptr->ext.xlow;
		if (new < dist)
		{
			best = ptr;
			dist = new;
		}
	}
	if (best != NULL)
		return(best);
	/*
	** nothing underneath.	pick nearest the top
	*/
	best = NULL;
	dist = 2.0 / G_frame->csy;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch < 0)
			continue;
		new = (1.0 - ptr->ext.yhi)/G_frame->csy + ptr->ext.xlow;
		if (new < dist)
		{
			best = ptr;
			dist = new;
		}
	}
	return(best);
}

static GR_OBJ *
near_prev ()
{
	GR_OBJ	*ptr, *best;
	float	dist, new;
	EXTENT ext;

	/*
	** first, try to find something to the left
	*/
	ext.xlow = 0.0;
	ext.xhi = Loc_x;
	ext.ylow = Loc_y - G_frame->csy / 2.0;
	ext.yhi = Loc_y + G_frame->csy;
	dist = 1.0;
	best = NULL;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch < 0)
			continue;
		if (GTovlp(&ext, &(ptr->ext)))
		{
			new = Loc_x - ptr->ext.xhi;
			if (new < dist)
			{
				best = ptr;
				dist = new;
			}
		}
	}
	if (best != NULL)
		return(best);
	/*
	** nothing to the left.	 Look for something above.
	*/
	best = NULL;
	dist = 2.0 / G_frame->csy;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch < 0)
			continue;
		if (ptr->ext.ylow < Loc_y)
			continue;
		new = (ptr->ext.ylow - Loc_y)/G_frame->csy - ptr->ext.xhi;
		if (new < dist)
		{
			best = ptr;
			dist = new;
		}
	}
	if (best != NULL)
		return(best);
	/*
	** nothing above.  pick nearest the bottom
	*/
	best = NULL;
	dist = 2.0 / G_frame->csy;
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch < 0)
			continue;
		new = ptr->ext.ylow/G_frame->csy - ptr->ext.xhi;
		if (new < dist)
		{
			best = ptr;
			dist = new;
		}
	}
	return(best);
}

static STATUS
srch_next (locator)
bool	locator;
{
	register GR_OBJ *ptr, *svptr;
	register i4	ord,
			org;
	i4		orow, ocol;

	if (Sortflag)
		dumbsort();
	if (Comps < 1)
		return (FAIL);

	/* we must recover Loc_row,  Loc_col on fail if we've called bolzano */
	orow = Loc_row;
	ocol = Loc_col;

	sync_loc(locator);
	set_cobj(locator);

	if (Cur_obj == NULL)
	{
		ptr = near_next();
		if (bolzano(ptr, 2, 2, locator))
			return (OK);
		if (Comps == 1)
		{
			Loc_row = orow;
			Loc_col = ocol;
			return (FAIL);
		}
	}
	else
	{
		ptr = Cur_obj;
	}

	svptr = ptr;

	/*
	** Comps >= 1 at this point.
	** If Comps == 1, then org, ord == 0, and (++0 % 1 == 0),
	** so loop exits immediately
	*/
	org = ord = ptr->scratch;
	for (++ord, ord = ord % Comps; ord != org; ++ord, ord = ord % Comps)
	{
		for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		{
			if (ptr->scratch == ord)
				break;
		}
		if (ptr == NULL)
			GTsyserr (S_GT0002_GTedit_fwd_fail, 0);
		if (bolzano(ptr, 2, 2, locator))
			return (OK);
	}

	/*
	** only one item is visible (if all is well), so tab to it.
	** if tab would cause no change in location, return FAIL so
	** user knows that input has been received.
	** added for fix to bug 9070
	*/
	if (bolzano(svptr, 2, 2, locator))
	{
		if (Loc_row != orow || Loc_col != ocol)
			return (OK);
	}
	Loc_row = orow;
	Loc_col = ocol;
	return (FAIL);
}

static STATUS
srch_prev (locator)
bool	locator;
{
	register GR_OBJ *ptr, *svptr;
	register i4	ord,
			org;
	i4		orow, ocol;

	if (Sortflag)
		dumbsort();
	if (Comps < 1)
		return (FAIL);

	/* we must recover Loc_row,  Loc_col on fail if we've called bolzano */
	orow = Loc_row;
	ocol = Loc_col;

	sync_loc(locator);
	set_cobj(locator);

	if (Cur_obj == NULL)
	{
		ptr = near_prev();
		if (bolzano(ptr, 2, 2, locator))
			return (OK);
		if (Comps == 1)
		{
			Loc_row = orow;
			Loc_col = ocol;
			return (FAIL);
		}
	}
	else
	{
		ptr = Cur_obj;
	}

	svptr = ptr;

	/*
	** Comps >= 1 at this point.
	** If Comps == 1, then org == 0, ord == (Comps -1) == 0,
	** so loop exits immediately
	*/
	org = ptr->scratch;
	ord = org - 1;
	if (ord < 0)
		ord = Comps - 1;
	while (ord != org)
	{
		for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		{
			if (ptr->scratch == ord)
				break;
		}
		if (ptr == NULL)
			GTsyserr (S_GT0003_GTedit_rev_fail, 0);
		if (bolzano(ptr, 2, 2, locator))
			return (OK);
		--ord;
		if (ord < 0)
			ord = Comps - 1;
	}

	/*
	** only one item is visible (if all is well), so go to it.
	** if 'movement' would cause no change in location, return FAIL so
	** user knows that input has been received.
	** added for fix to bug 9070.
	*/
	if (bolzano(svptr, 2, 2, locator))
	{
		if (Loc_row != orow || Loc_col != ocol)
			return (OK);
	}
	Loc_row = orow;
	Loc_col = ocol;
	return (FAIL);
}

static
sync_loc (locator)
bool	locator;
{
	if (! locator)
		GTxatof(Loc_row, Loc_col, &Loc_x, &Loc_y);
	else
		GTxftoa(Loc_x, Loc_y, &Loc_row, &Loc_col);
}

/*
**	set current object
*/
static
set_cobj (locator)
bool	locator;
{
	register GR_OBJ *ptr;
	i4 lyr;
	EXTENT expt;

	/* search with a cursor sized box or a line width sized box */
	if (locator)
	{
		expt.xlow = Loc_x;
		expt.xhi = Loc_x + G_frame->lx;
		expt.ylow = Loc_y;
		expt.yhi = Loc_y + G_frame->ly;
	}
	else
		cursor_ext (&expt);
	Cur_obj = NULL;
	for (ptr = G_frame->tail; ptr != NULL; ptr = ptr->prev)
	{
		if (GTovlp(&expt, &(ptr->ext)) && ptr->layer <= G_frame->clayer)
		{
			Cur_obj = ptr;
			break;
		}
	}
}

static
cursor_ext (ext)
EXTENT *ext;
{
	/* get center of cursor */
	GTxatof(Loc_row, Loc_col, &(ext->xlow), &(ext->ylow));
	ext->xlow -= G_frame->csx/2.0;
	ext->ylow -= G_frame->csy/2.0;
	ext->xhi = ext->xlow + G_frame->csx;
	ext->yhi = ext->ylow + G_frame->csy;
}

/*{
** Name:	GTfmsg_pause() -	Print message and wait for return.
**
**	Move to the bottom of screen and wait for a <CR>.  Prompt if message
**	string is set.	As a side effect, all characters typed up to the <CR>
**	are echoed to enable users to type printer escapes, etc.
**
**	This routine is almost identical with 'GTmsg_pause()' in "gtmsgp.c"
**	except that it uses the Forms system.
**
** Parameters:
**	prompt -- (input only)	Message string.
**
** History:
**	01/86 (jhw) -- Written.
**	04/86 (jhw) -- Call 'GTputs()' to output escape sequence to GKS device.
**	12-dec-86 (bruceb) Fix for bug 11079
**		Only call FTbell if NOT generating a [HIT RETURN].
*/

VOID
GTfmsg_pause (prompt)
char	*prompt;
{
	register char	ch;

	VOID	GTputs();
	VOID	GTmsg();

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	if (prompt == (char *)NULL)
	{
		TDtputs(TDtgoto(G_tc_cm, 0, G_lines), 1, GTchout);
		FTbell();
	}
	else
	{	/* Note:  'GTmsg()' moves cursor to bottom of screen */
		char	buf[DEF_COLS];

		IIUGfmt(buf, DEF_COLS, ERget(F_GT000C_RET_format), 1, prompt);
		GTmsg(buf);
	}

	/* Wait for a <cr> - use TE to avoid buffering of escape by TD */

	for (ch = TEget((i4)0) ; ch != '\r' && ch != '\n' ; ch = TEget((i4)0))
	{
		char	output[2];

		/* Pass through possible <ESC> sequence to GKS device */
		output[0] = ch; output[1] = EOS;
		GTputs(output);
	}
# ifdef DGC_AOS
	GTcsunl();
# else
	GTcursync();
# endif
}


/*{
** Name:	IIGTfldFrmLocDecode	- Determine effect of locator click.
**
** Description:
**	Determine whether or not the user clicked on a legal location.
**	Legal locations are anywhere within the region defined by
**	(r_low,c_low) and (r_hi,c_hi).
**
** Inputs:
**	key	User input.
**
** Outputs:
**	evcb
**	    .event	fdNOCODE if illegal location; fdopGOTO if legal.
**	    .gotorow	Row on the screen if legal location.
**	    .gotocol	Col on the screen if legal location.
**
**	Returns:
**		None.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	28-mar-90 (bruceb)	Written.
*/
VOID
IIGTfldFrmLocDecode(key, evcb)
KEYSTRUCT	*key;
FRS_EVCB	*evcb;
{
    i4		row;
    i4		col;
    IICOORD	frm;
    char	buf[50];

    row = key->ks_p1;
    col = key->ks_p2;

    /* If user has clicked on the menu line, call menu decode routine. */
    if (row == LINES - 1)
    {
	/* Menu line isn't accessible when under GTpoint. */
	if (loc_w_menu)
	    IIFTmldMenuLocDecode(key, evcb);
	else
	    evcb->event = fdNOCODE;
	return;
    }

    frm.begy = r_low;
    frm.begx = c_low;
    frm.endy = r_hi;
    frm.endx = c_hi;

    if (!IITDlioLocInObj(&frm, row, col))
    {
	evcb->event = fdNOCODE;
    }
    else
    {
	evcb->event = fdopGOTO;
	evcb->gotorow = row;
	evcb->gotocol = col;

	STprintf(buf, ERx("graphics screen coords (%d,%d)"), row, col);
	IITDposPrtObjSelected(buf);
    }
}
