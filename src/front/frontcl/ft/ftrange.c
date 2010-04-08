/*
**  ftrange.c
**
**  Routines to handle sideway scrolling within a field and
**  range specifications for QBF.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**	Created - 08/07/85 (dkh)
**	85/08/21  20:28:10  dave
**		Initial revision
**	85/09/07  22:48:58  dave
**		Added support for table field scrolling, etc. (dkh)
**	85/09/11  12:51:24  dave
**		Added FTrngon(). (dkh)
**	85/09/12  19:14:42  dave
**		Fixed a few small problems. (dkh)
**	85/09/13  22:54:01  dave
**		Ftrnglast mechanism redone to better support QBF. (dkh)
**	85/09/26  20:18:38  dave
**		Changed to support new scroll commands. (dkh)
**	85/10/04  23:19:31  dave
**		Added some debugging routines. (dkh)
**	85/10/26  20:33:40  dave
**		Fixed so that the first row in a table field is processed
**		correctly for building a query. (dkh)
**	85/10/29  18:37:07  dave
**		Fixed FTrngcval() to handle a NULL for arg "tbl". (dkh)
**	85/10/31  17:57:15  dave
**		Fixed to use new loop variable in FTrngxinit() otherwise it goes
**		into an infinite loop. (dkh)
**	85/11/05  21:10:04  dave
**		extern to FUNC_EXTERN for routines. (dkh)
**	85/11/27  11:55:20  dave
**		Added FTrngfldres() to handle resetting a field that has been
**		scrolled horizontally to the beginning of the field value. (dkh)
**	86/04/19  23:23:54  dave
**		Fix for BUG 8743. (dkh)
**	86/04/19  23:03:19  dave
**		Fix for BUG 8742. (dkh)
**	86/04/19  21:54:30  dave
**		Fix for BUG 8714. (dkh)
**	12-sep-86 (marian, dave)
**		Added if statement inside of bug fix 8743 to check for null.
**	16-mar-87 (bruceb)
**		Added code to support right-to-left fields for Hebrew project.
**		Mostly involves calling reversed versions of TDwin_str and
**		TDaddstr where appropriate.
**	03/20/87 (dkh) - Changed FTrngget() to FTgetbuf().
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	07-may-87 (bruceb)
**		Added calls to FTswapparens before calls on FDrngchk.  When
**		swapping contents of pdat, call FTswapparens again to restore
**		proper state.
**	04-06-87 (bruceb)
**		Additional comments, declarations of function types.
**		Also, properly set 'hdr' in FTgetdisp before use.
**	06/19/87 (dkh) - Code cleanup.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	09-dec-87 (kenl)
**		Changed FTrvalid, FTrngtrv, and FTchkrng
**		so that they are passed (and pass on) two more
**		parameters, a function pointer and a structure pointer.  This
**		is needed for QBF to properly build the FROM clause so QG can
**		read a Master row for a Master/Detail JoinDef.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	23-aug-88 (kenl)
**		Changed FTrvalid, FTrngtrv, and FTchkrng
**		so that they are passed (and pass on) one more
**		parameter,  the DML being spoken.  This is needed by
**		the validity checking routines so that they know if a
**		gateway is being talked to.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	04/14/90 (dkh) - Changed "# if defined" to "# ifdef".
**	07/10/90 (dkh) - Integrated more MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	04/12/91 (dkh) - Fixed bug 36958.
**	30-apr-93 (rudyw)
**		Fix bug 51109. A modification to routine FTwmvtab resulted in
**		NULL being returned to signify an invisible column. Calls to
**		that routine from FTrngucp/FTrngdcp need to handle the NULL.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
**      29-Sep-2006 (hanal04) Bug 116741
**          Added FTrngcat_alloc() which will allocate a buffer for use
**          in calls to FTrngcat(). The hardcoded 4096 buffers were
**          being blown and we were trashing the stack.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	"ftrange.h"
# include	<afe.h>
# include	<si.h>
# include	<st.h>
# include	<qg.h>
# include	<me.h>
# include	<er.h>

GLOBALREF	i2	FTrngtag;


/*
**  Warning! Tags are supposed to be i2s when used with the
**  FE allocation routines.  If that changes, we could
**  be in trouble.
*/


FUNC_EXTERN	WINDOW	*FTwmvtab();
FUNC_EXTERN	WINDOW	*FTfindwin();

VOID	FTaddstr();
VOID	FTwinstr();


RGRFLD	*FTgetrg();
bool	FTchkrng();
bool	FTrngtrv();


VOID
FTrgtag(tag)
i2	tag;
{
	FTrngtag = tag;
}


/*
**  Redisplay whatever has been saved in "rwin".
*/
VOID
FTrnglast(frm)
FRAME	*frm;
{
	TBLFLD	*tbl;
	RGFLD	*range;
	RGFLD	*rfld;
	RGRFLD	*rgptr;
	RGTFLD	*tgptr;
	WINDOW	*win;
	i4	fldno;
	i4	i;
	i4	j;
	i4	k;
	i4	maxrow;
	i4	maxcol;
	i4	winsize;
	i4	botrow;
	i4	curlstrow;
	i4	lastrow;

# ifdef	DATAVIEW
	_VOID_ IIMWrrvRestoreRngVals(frm);
# endif	/* DATAVIEW */

	range = (RGFLD *) frm->frrngptr;
	fldno = frm->frfldno;

	for (i = 0; i < fldno; i++)
	{
		rfld = &range[i];
		if (rfld->rgfltype == RGTBLFLD)
		{
			tbl = (frm->frfld[i])->fld_var.fltblfld;
			tgptr = rfld->rg_var.rgtblfld;

			/*
			**  Fix for BUG 8743. (dkh)
			*/
			curlstrow = tgptr->rglastrow;
			botrow = tgptr->rglastrow = tgptr->rgllstrow;
			lastrow = (curlstrow > botrow ? curlstrow : botrow);
			tgptr->rgtoprow = 0;

			maxrow = tbl->tfrows;
			maxcol = tbl->tfcols;
			for (j = 0; j < maxrow; j++)
			{
				tbl->tfcurrow = j;
				for (k = 0; k < maxcol; k++)
				{
					tbl->tfcurcol = k;

					/*
					**  Yes, this has the effect
					**  of allocating things.
					**  This is done so that we do
					**  it for fields that have not
					**  been touch.  Yes, it is expensive
					*/
					rgptr = FTgetrg(frm, i);

					rgptr->hcur = rgptr->tcur = NULL;
					*(rgptr->rwin) = '\0';
					if (rgptr->pdat != NULL)
					{
						win = FTfindwin(frm->frfld[i],
							FTwin, FTutilwin);
						FTaddstr(frm, i, win,
								rgptr->pdat);
						winsize = win->_maxx * win->_maxy;
						if (STlength(rgptr->pdat) > winsize)
						{
							FTtlxadd(rgptr, &(rgptr->pdat[winsize]));
						}
					}
				}
			}

			/*
			**  Fix for BUG 8743. (dkh)
			*/
			tbl->tfcurrow = 0;
			for (j = maxrow; j <= lastrow; j++)
			{
				tgptr->rgtoprow = j;
				for (k = 0; k < maxcol; k++)
				{
					tbl->tfcurcol = k;
					rgptr = FTgetrg(frm, i);
					rgptr->hcur = rgptr->tcur = NULL;
					if (j > botrow)
					{
						*(rgptr->rwin) = '\0';
						continue;
					}
					/* add if statement to check for null */
					if (rgptr->pdat != NULL)
					{
						win = FTfindwin(frm->frfld[i],
							FTwin, FTutilwin);
						winsize = win->_maxx * win->_maxy;
						STlcopy(rgptr->pdat, rgptr->rwin, winsize);
						if (STlength(rgptr->pdat) > winsize)
						{
							FTtlxadd(rgptr, &(rgptr->pdat[winsize]));
						}
					}
				}
			}

			tgptr->rgtoprow = 0;
			tbl->tfcurrow = 0;
			tbl->tfcurcol = 0;
		}
		else
		{
			rgptr = rfld->rg_var.rgregfld;
			rgptr->hcur = rgptr->tcur = NULL;
			if (rgptr->hbuf == NULL)
			{
				continue;
			}
			*(rgptr->rwin) = '\0';
			if (rgptr->pdat != NULL)
			{
				win = FTfindwin(frm->frfld[i], FTwin,
					FTutilwin);
				FTaddstr(frm, i, win, rgptr->pdat);
				winsize = win->_maxx * win->_maxy;
				if (STlength(rgptr->pdat) > winsize)
				{
					FTtlxadd(rgptr,
						&(rgptr->pdat[winsize]));
				}
			}
		}
	}
}

char *
FTrngcat_alloc(rgptr)
RGRFLD  *rgptr;
{
	SIZE_TYPE buflen = 3; /* i2 len and NULL to start with */
        char                    *tempbuf = NULL;

        /* The calculation below is based on the operations found in
        ** FTrngcat(). Changes in that routine must be reflected here.
        */

        if(rgptr->hcur)
            buflen += (SIZE_TYPE)(rgptr->hcur - rgptr->hbuf);

        if(rgptr->tcur)
            buflen += (SIZE_TYPE)(rgptr->tend - rgptr->tcur);

        buflen += STlength((char *)rgptr->rwin);

        if ((tempbuf = MEreqmem((u_i4)0, buflen, FALSE,
                                (STATUS *)NULL)) == NULL)
        {
            tempbuf = NULL;
        }

        return(tempbuf);
}

VOID
FTrngcat(rgptr, buf)
RGRFLD	*rgptr;
char	*buf;
{
	char	*cp;
	char	*bp;

	bp = buf;
	*bp = '\0';
	if (rgptr->hcur != NULL)
	{
		for (cp = rgptr->hbuf; cp <= rgptr->hcur;)
		{
			*bp++ = *cp++;
		}
	}
	*bp = '\0';
	if (rgptr->rwin != NULL)
	{
		STcat(buf, rgptr->rwin);
	}
	if (rgptr->tcur != NULL)
	{
		bp = &(buf[STlength(buf)]);
		for (cp = rgptr->tcur; cp <= rgptr->tend;)
		{
			*bp++ = *cp++;
		}
		*bp = '\0';
	}
	STtrmwhite(buf);
}


bool
FTrngcval(frm, fldno, tbl, win)
FRAME	*frm;
i4	fldno;
TBLFLD	*tbl;
WINDOW	*win;
{
	RGRFLD			*rgptr;
	i4			stlen;
	char			*tempbuf = NULL;
	DB_DATA_VALUE		dbv;
	DB_TEXT_STRING		*text;
	u_char			*buf;
	char			*fname;
	char			*cname;
	FLDHDR			*hdr;
	STATUS			rval;

	rgptr = FTgetrg(frm, fldno);
	FTwinstr(frm, fldno, win, rgptr->rwin);

        if ((tempbuf = FTrngcat_alloc(rgptr)) == NULL)
        {
            IIUGbmaBadMemoryAllocation(ERx("FTrngcval"));
        }

	text = (DB_TEXT_STRING *) tempbuf;
	buf = text->db_t_text;

	/*
	**  Call will place the range buffers into "buf" and
	**  null terminate it.
	*/
	FTrngcat(rgptr, buf);

# ifdef DATAVIEW
	_VOID_ IIMWgrvGetRngVal(frm, fldno, tbl, buf);
# endif	/* DATAVIEW */

	if (buf[0] != '\0')
	{
		hdr = (*FTgethdr)(frm->frfld[fldno]);
		if (hdr->fhdflags & fdREVDIR)
			FTswapparens(buf);
		if (tbl != NULL)
		{
			fname = tbl->tfhdr.fhdname;
			cname = hdr->fhdname;
		}
		else
		{
			fname = hdr->fhdname;
			cname = NULL;
		}

		/*
		**  Get a DB_DATA_VALUE out of the buffer spaces.
		*/
		stlen = STlength((char *) buf);
		text->db_t_count = stlen;
		dbv.db_datatype = DB_LTXT_TYPE;
		dbv.db_length = stlen + DB_CNTSIZE;
		dbv.db_prec = 0;
		dbv.db_data = (PTR) text;
		rval = (*FTrngchk)(frm, fname, cname, &dbv, (bool) FALSE,
			(char *) NULL, (char *) NULL, (bool) FALSE,
			0, NULL, NULL, NULL);
		if (rval != OK && rval != QG_NOSPEC)
		{
			frm->frcurfld = fldno;
                        MEfree(tempbuf);
			return(FALSE);
		}
	}
        MEfree(tempbuf);
	return(TRUE);
}



bool
FTrngrval(frm, fldno, tbl, row)
FRAME	*frm;
i4	fldno;
TBLFLD	*tbl;
i4	row;
{
	i4			i;
	i4			maxcol;
	RGRFLD			*rgptr;
	FLDVAL			*val;
	WINDOW			*win;
	char			*tempbuf = NULL;
	DB_TEXT_STRING		*text;
	DB_DATA_VALUE		dbv;
	i4			stlen;
	u_char			*buf;
	char			*fname;
	char			*cname;
	STATUS			rval;
	FLDHDR			*hdr;

	dbv.db_datatype = DB_LTXT_TYPE;
	dbv.db_prec = 0;

	fname = tbl->tfhdr.fhdname;
	maxcol = tbl->tfcols;
	val = tbl->tfwins + (row * maxcol);

# ifdef DATAVIEW
	_VOID_ IIMWbqtBegQueryTbl();
# endif /* DATAVIEW */

	for (i = 0; i < maxcol; i++, val++)
	{
		tbl->tfcurcol = i;
		cname = tbl->tfflds[i]->flhdr.fhdname;
		rgptr = FTgetrg(frm, fldno);
		win = FTwmvtab(tbl, i, val);
		FTwinstr(frm, fldno, win, rgptr->rwin);

		/*
		**  Call will place the range buffers into "buf" and
		**  null terminate it.
		*/
                if ((tempbuf = FTrngcat_alloc(rgptr)) == NULL)
                {
                    IIUGbmaBadMemoryAllocation(ERx("FTrngrval"));
                }

                text = (DB_TEXT_STRING *) tempbuf;
                buf = text->db_t_text;

		FTrngcat(rgptr, buf);

# ifdef DATAVIEW
		_VOID_ IIMWgrvGetRngVal(frm, fldno, tbl, buf);
# endif	/* DATAVIEW */

		if (buf[0] != '\0')
		{
			hdr = (*FTgethdr)(frm->frfld[fldno]);
			if (hdr->fhdflags & fdREVDIR)
				FTswapparens(buf);
			stlen = STlength((char *) buf);
			text->db_t_count = stlen;
			dbv.db_length = stlen + DB_CNTSIZE;
	                dbv.db_data = (PTR) text;
			rval = (*FTrngchk)(frm, fname, cname, &dbv, (bool)FALSE,
				(char *) NULL, (char *) NULL, (bool) FALSE,
				0, NULL, NULL, NULL);
			if (rval != OK && rval != QG_NOSPEC)
			{
				frm->frcurfld = i;

# ifdef DATAVIEW
				_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

                                MEfree(tempbuf);
				return(FALSE);
			}
		}
                MEfree(tempbuf);
	}

# ifdef DATAVIEW
	_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

	return(TRUE);
}


bool
FTrvalid(frm, bffunc, bfvalue, dml_level)
FRAME	*frm;
STATUS	(*bffunc)();
PTR	bfvalue;
i4	dml_level;
{
	i4	i;
	i4	j;
	i4	k;
	i4	toprow;
	i4	botrow;
	i4	lastrow;
	i4	fldno;
	i4	maxrow;
	i4	maxcol;
	i4	ocol;
	i4	orow;
	TBLFLD	*tbl;
	RGFLD	*range;
	RGFLD	*rfld;
	RGRFLD	*rgptr;
	RGTFLD	*tgptr;
	FIELD	*fld;
	FLDVAL	*val;
	WINDOW	*win;
	FLDHDR	*chdr;

# ifdef	DATAVIEW
	if (IIMWsrvSaveRngVals(frm) == FAIL)
		return(FALSE);
# endif	/* DATAVIEW */

	range = (RGFLD *) frm->frrngptr;
	fldno = frm->frfldno;

	for (i = 0; i < fldno; i++)
	{
		fld = frm->frfld[i];
		rfld = &range[i];
		if (rfld->rgfltype == RGTBLFLD)
		{
			tbl = fld->fld_var.fltblfld;
			tgptr = rfld->rg_var.rgtblfld;
			maxrow = tbl->tfrows;
			maxcol = tbl->tfcols;
			orow = tbl->tfcurrow;
			ocol = tbl->tfcurcol;

			/*
			**  Fix for BUG 8742. (dkh)
			*/
			tbl->tfcurrow = 0;
			toprow = tgptr->rgtoprow;
			if (!FTrngtrv(frm, i, tbl, tgptr, maxcol, 0, toprow,
			    bffunc, bfvalue, dml_level))
			{
				tbl->tfcurrow = orow;
				tbl->tfcurcol = ocol;
				return(FALSE);
			}
			tgptr->rgtoprow = toprow;

# ifdef DATAVIEW
			_VOID_ IIMWbqtBegQueryTbl();
# endif /* DATAVIEW */

			for (j = 0; j < maxrow; j++)
			{
				tbl->tfcurrow = j;
				val = tbl->tfwins + (j * maxcol);
				for (k = 0; k < maxcol; k++, val++)
				{
					tbl->tfcurcol = k;

					chdr = &(tbl->tfflds[k]->flhdr);

					/*
					**  Yes, this has the effect of
					**  allocating the object.  But
					**  has the desired effect that
					**  all fields will be allocated
					**  sometime along the way.
					*/
					rgptr = FTgetrg(frm, i);

					if (rgptr->hbuf == NULL ||
						chdr->fhdflags & fdINVISIBLE)
					{
						continue;
					}
					win = FTwmvtab(tbl, k, val);
					FTwinstr(frm, i, win, rgptr->rwin);

					if (!FTchkrng(frm, i, k, rgptr,
					    bffunc, bfvalue, dml_level))
					{
# ifdef DATAVIEW
						_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

						return(FALSE);
					}
				}
			}

# ifdef DATAVIEW
			_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

			/*
			**  Fix for BUG 8742. (dkh)
			*/
			botrow = toprow + tbl->tfrows;
			lastrow = tgptr->rglastrow;
			tbl->tfcurrow = 0;
			if (!FTrngtrv(frm, i, tbl, tgptr, maxcol,
				    botrow, lastrow + 1, bffunc,
				    bfvalue, dml_level))
			{
				tbl->tfcurrow = orow;
				tbl->tfcurcol = ocol;
				return(FALSE);
			}
			tgptr->rgtoprow = toprow;

			tbl->tfcurrow = orow;
			tbl->tfcurcol = ocol;
		}
		else
		{
			rgptr = rfld->rg_var.rgregfld;
			if (rgptr->hbuf == NULL)
			{
				continue;
			}
			win = FTfindwin(frm->frfld[i], FTwin, FTutilwin);
			FTwinstr(frm, i, win, rgptr->rwin);
			if (!FTchkrng(frm, i, 0, rgptr, bffunc,
				bfvalue, dml_level))
			{
				return(FALSE);
			}
		}
	}
	return(TRUE);
}



/*
**  Initialize table field stuff for retrieval.
*/
VOID
FTrgetinit(frm)
FRAME	*frm;
{
	i4	i;
	i4	fldno;
	RGFLD	*range;
	RGFLD	*rfld;
	RGTFLD	*tgptr;

	range = (RGFLD *) frm->frrngptr;
	fldno = frm->frfldno;
	for (i = 0; i < fldno; i++)
	{
		rfld = &range[i];
		if (rfld->rgfltype == RGTBLFLD)
		{
			/*
			**  Set to -1 so that the first call
			**  to FTrnxtrow will increment to
			**  zero, the first row.
			*/
			rfld->rg_var.rgtblfld->rgretrow = -1;
			tgptr = rfld->rg_var.rgtblfld;
			tgptr->rgllstrow = tgptr->rglastrow;
		}
	}
}


/*{
** Name:	FTgetdisp - Get display buffer for a field.
**
** Description:
**	Get a DB_DATA_VALUE pointer to the display buffer of
**	a field in range query mode.  Range query mode allows
**	users to more complex qualifications into a field
**	without being restricted to the physical limitations
**	of a field display size.  Although the latter feature
**	is not a necessity to support range queries and may
**	be implemented differently on different environments.
**
**	For the full duplex ASCII version, the implementation
**	of range queries can not use the normal display buffers
**	of a field due to horizontal field scrolling.  It is the
**	responsibility of this routine to package up the display
**	of a scrollable field into a LONG_TEXT DB_DATA_VALUE.
**	As a consequence, the full duplex ASCII version will
**	always allocate memory (using the passed in tag) to
**	create the LONG_TEXT DB_DATA_VALUE.  In essence, a copy
**	is always made, regardless of the "forcecopy" parameter.
**	Memory allocation is done via a call to "MEreqmem()".
**
**	A field is identified by its field number (parameter "fldno")
**	and column number ("colno") if the field is a table field.
**	The concept of the current row will already have been
**	set up by a previous call to "FTrnxtrow()".  A pointer
**	to the display DB_DATA_VALUE is returned in "dispdbv".
**
**	If the field is empty, A NULL pointer is returned.
**
**	For the normal case where horizontal scrolling fields are
**	not available, just return the pointer to the field's
**	display value (or a copy of it).
**
** Inputs:
**	frm		Pointer to form containing field.
**	fldno		Field (sequence) number of field.
**	colno		Column (sequence) number if field is a table field.
**	tag		Tag for memory allocation.
**	forcecopy	Force routine to copy the DB_DATA_VALUE.
**			For scrolling field versions of the system,
**			a copy is always done regardless of the
**			setting of this argument.
**
** Outputs:
**		dispdbv	Set to point to allocated display data value.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	A copy of the DB_DATA_VALUE is always made regardless of the
**	"forcecopy" argument on systems that have scrolling
**	field support.
**	Memory is allocated based on tag passed to it and
**	it is up to caller to free the memory.
**
** History:
**	03/20/87 (dkh) - Changed to use DB_DATA_VALUE/LONG_TEXT and
**			 renamed from FTrngget().
*/
VOID
FTgetdisp(frm, fldno, colno, tag, forcecopy, dispdbv)
FRAME		*frm;
i4		fldno;
i4		colno;
i2		tag;
i4		forcecopy;
DB_DATA_VALUE	**dispdbv;
{
	RGFLD		*range;
	RGFLD		*rfld;
	RGRFLD		*rgptr;
	RGRFLD		*rg2ptr;
	RGTFLD		*tgptr;
	char		*sp;
	DB_DATA_VALUE	*dbv;
	DB_TEXT_STRING	*text;
	i4		size;
	FLDHDR		*hdr;

	*dispdbv = NULL;

	range = (RGFLD *) frm->frrngptr;
	rfld = &range[fldno];
	if (rfld->rgfltype == RGTBLFLD)
	{
		tgptr = rfld->rg_var.rgtblfld;
		rg2ptr = tgptr->rgrows[tgptr->rgretrow];
		rgptr = &(rg2ptr[colno]);
		if (rgptr->hbuf == NULL)
		{
			return;
		}
	}
	else
	{
		if ((rgptr = rfld->rg_var.rgregfld) == NULL)
		{
			return;
		}
	}

	if ((sp = rgptr->pdat) == NULL)
	{
		return;
	}
	if (*sp == '\0')
	{
		return;
	}
	else
	{
		if ((dbv = (DB_DATA_VALUE *)MEreqmem((u_i4)tag,
		    (u_i4)sizeof(DB_DATA_VALUE),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			/*
			**  Could not allocate memory.
			*/
			IIUGbmaBadMemoryAllocation(ERx("FTgetdisp"));
		}
		size = STlength(sp);
		dbv->db_datatype = DB_LTXT_TYPE;
		dbv->db_length = size + DB_CNTSIZE;
		dbv->db_prec = 0;
		if ((dbv->db_data = MEreqmem((u_i4)tag,
		    (u_i4)dbv->db_length, TRUE, (STATUS *)NULL)) == NULL)
		{
			/*
			**  Could not allocate memory.
			*/
			IIUGbmaBadMemoryAllocation(ERx("FTgetdisp"));
		}
		text = (DB_TEXT_STRING *) dbv->db_data;
		text->db_t_count = size;
		MEcopy((PTR) sp, (u_i2) size, (PTR) text->db_t_text);
		hdr = (*FTgethdr)(frm->frfld[fldno]);
		if (hdr->fhdflags & fdREVDIR)
			FTswapparens(text->db_t_text);

		*dispdbv = dbv;
	}
}


bool
FTrnxtrow(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	i4	i;
	i4	realrows;
	RGFLD	*range;
	RGFLD	*rfld;
	RGTFLD	*tgptr;
	bool	anydata = FALSE;

	range = (RGFLD *) frm->frrngptr;
	rfld = &range[fldno];
	if (rfld->rgfltype != RGTBLFLD)
	{
		return(anydata);
	}
	tgptr = rfld->rg_var.rgtblfld;
	i = tgptr->rgretrow + 1;

	/*
	**  Fix for BUG 8714. (dkh)
	*/
	realrows = tgptr->rglastrow + 1;

	/*
	**  If a row has been allocated, then there is the possibility
	**  that data could have been entered.  Empty fields are taken
	**  care of later.
	*/
	for (; i < realrows; i++)
	{
		if (tgptr->rgrows[i] == NULL)
		{
			continue;
		}
		anydata = TRUE;
		break;
	}
	tgptr->rgretrow = i;
	return(anydata);
}



/*
**  Initialization for structs that have already been allocated.
*/
VOID
FTrngxinit(frm)
FRAME	*frm;
{
	i4	i;
	i4	j;
	i4	k;
	i4	maxrows;
	i4	maxcol;
	i4	fldno;
	RGFLD	*range;
	RGFLD	*rfld;
	RGRFLD	*rgptr;
	RGRFLD	*rg2ptr;
	RGTFLD	*tgptr;

	range = (RGFLD *) frm->frrngptr;
	fldno = frm->frfldno;

	for (k = 0; k < fldno; k++)
	{
		rfld = &range[k];
		if (rfld->rgfltype == RGTBLFLD)
		{
			tgptr = rfld->rg_var.rgtblfld;
			tgptr->rglastrow = tgptr->rgtoprow = 0;
			maxrows = tgptr->rgmaxrows;
			maxcol = tgptr->rgmaxcol;
			for (i = 0; i < maxrows; i++)
			{
				if ((rgptr = tgptr->rgrows[i]) == NULL)
				{
					continue;
				}
				for (j = 0; j < maxcol; j++)
				{
					rg2ptr = &(rgptr[j]);
					rg2ptr->tcur = rg2ptr->hcur = NULL;
					*(rg2ptr->rwin) = '\0';
				}
			}
		}
		else
		{
			rgptr = rfld->rg_var.rgregfld;
			if (rgptr->hbuf != NULL)
			{
				rgptr->tcur = rgptr->hcur = NULL;
				*(rgptr->rwin) = '\0';
			}
		}
	}
}


/*
**  Set special flags for range specification and
**  allocate storage for sideways scrolling.
**
**  Use frm->frrngptr to serve as pointer to structs.
*/

STATUS
FTrnginit(frm)
FRAME	*frm;
{
	FIELD	**fld;
	TBLFLD	*tbl;
	RGFLD	*range;
	RGFLD	*rfld;
	i4	i;
	i4	fldno;
	i2	tag;

	frm->frmflags |= fdRNGMD;
	if ((fldno = frm->frfldno) < 1)
	{
		return(FAIL);
	}
	if (frm->frrngptr != NULL)
	{
		FTrngxinit(frm);
		return(OK);
	}
	tag = FEgettag();
	frm->frrngtag = tag;
	if ((frm->frrngptr = FEreqmem((u_i4)tag,
	    (u_i4)(fldno * sizeof(RGFLD)), TRUE, (STATUS *)NULL)) == NULL)
	{
		return(FAIL);
	}

	range = (RGFLD *) frm->frrngptr;

	fld = frm->frfld;

	/*
	**  Loop does not have to deal with regular fields
	**  since the tag for them is 0 and the memory
	**  location has already been zeroed out by the
	**  FEreqmem above.
	*/

	for (i = 0; i < fldno; i++, fld++)
	{
		rfld = &range[i];
		if ((*fld)->fltag == FTABLE)
		{
			rfld->rgfltype = RGTBLFLD;
			if ((rfld->rg_var.rgtblfld =
			    (RGTFLD *)FEreqmem((u_i4)tag,
			    (u_i4)sizeof(RGTFLD),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				return(FAIL);
			}
			tbl = (*fld)->fld_var.fltblfld;
			if ((rfld->rg_var.rgtblfld->rgrows =
			    (RGRFLD **)FEreqmem((u_i4)tag,
			    (u_i4)(2 * tbl->tfrows * sizeof(RGRFLD *)),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				return(FAIL);
			}
			rfld->rg_var.rgtblfld->rgtoprow = 0;
			rfld->rg_var.rgtblfld->rgmaxrows = tbl->tfrows +
				tbl->tfrows;
			rfld->rg_var.rgtblfld->rgmaxcol = tbl->tfcols;
		}
		else
		{
			rfld->rgfltype = RGREGFLD;
			if ((rfld->rg_var.rgregfld =
			    (RGRFLD *)FEreqmem((u_i4)tag,
			    (u_i4)sizeof(RGRFLD),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				return(FAIL);
			}
		}
	}
	return(OK);
}



/*
**  Delete storage allocated for sideways scrolling, etc.
**  Also, reset special flags in frame and for insert mode.
*/
VOID
FTrngend(frm)
FRAME	*frm;
{
	frm->frmflags &= ~fdRNGMD;

	if (frm->frfldno < 1)
	{
		return;
	}

	FEfree((i2) frm->frrngtag);
	frm->frrngtag = 0;
	frm->frrngptr = NULL;

# ifdef	DATAVIEW
	_VOID_ IIMWfrvFreeRngVals(frm);
# endif	/* DATAVIEW */
}



/*
**  Add a null terminated string to the tail buffer.
**  Called from FTvi().
*/
VOID
FTtlxadd(rgptr, cp)
RGRFLD	*rgptr;
char	*cp;
{
	char	*bp;
	i4	i;

	i = STlength(cp);
	bp = cp + i - 1;
	for (; bp >= cp; )
	{
		if (rgptr->tcur == NULL)
		{
			rgptr->tcur = rgptr->tend;
		}
		else if (rgptr->tcur == rgptr->tbuf)
		{
			FTrngxpand(rgptr, RG_T_ADD);
			(rgptr->tcur)--;
		}
		else
		{
			(rgptr->tcur)--;
		}
		*(rgptr->tcur) = *bp--;
	}
}

VOID
FTunrng(frm)
FRAME	*frm;
{
	frm->frmflags &= ~fdRNGMD;
}

VOID
FTrngon(frm)
FRAME	*frm;
{
	frm->frmflags |= fdRNGMD;
}


STATUS
FTargrfld(rrgptr, tag, wsize)
RGRFLD	*rrgptr;
i2	tag;
i4	wsize;
{
	char	*cp;
	i4	size;

	size = MAX_I_BFSIZE + MAX_I_BFSIZE;
	if ((cp = (char *)FEreqmem((u_i4)tag, (u_i4)size, TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		return(FAIL);
	}
	rrgptr->hbuf = cp;
	rrgptr->tbuf = cp + MAX_I_BFSIZE;
	rrgptr->hcur = NULL;
	rrgptr->hend = cp + MAX_I_BFSIZE - 1;
	rrgptr->tcur = NULL;
	rrgptr->tend = rrgptr->tbuf + MAX_I_BFSIZE - 1;

	if ((rrgptr->rwin = (char *)FEreqmem((u_i4)tag, (u_i4)wsize, TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		return(FAIL);
	}

	return(OK);
}



i4
FTrgwsize(fld)
FIELD	*fld;
{
	FLDTYPE	*type;

	type = (*FTgettype)(fld);
	return((((type->ftwidth + type->ftdataln - 1)/type->ftdataln) * type->ftdataln) + 1);
}



RGRFLD *
FTgetrg(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	RGFLD	*range;
	RGFLD	*rgptr;
	RGRFLD	*rrgptr;
	RGTFLD	*trgptr;
	RGRFLD	**rowarray;
	i4	currow;
	i4	curcol;
	i4	maxcol;
	i4	newrows;
	i4	i;
	i4	ocol;
	i2	tag;

	range = (RGFLD *) frm->frrngptr;

	fld = frm->frfld[fldno];
	rgptr = &(range[fldno]);
	tag = frm->frrngtag;
	if (fld->fltag == FREGULAR)
	{
		rrgptr = rgptr->rg_var.rgregfld;
		if (rrgptr->hbuf == NULL)
		{
			if (FTargrfld(rrgptr, tag, FTrgwsize(fld)) != OK)
			{
				return(NULL);
			}
		}
	}
	else
	{
		tbl = fld->fld_var.fltblfld;
		currow = tbl->tfcurrow;
		curcol = tbl->tfcurcol;
		maxcol = tbl->tfcols;
		trgptr = rgptr->rg_var.rgtblfld;
		ocol = tbl->tfcurcol;

		/*
		**  For now, just allocate extra rows based on
		**  how many rows are displayed in the table field.
		*/

		if (currow + trgptr->rgtoprow >= trgptr->rgmaxrows)
		{
			newrows = trgptr->rgmaxrows + tbl->tfrows;
			if ((rowarray = (RGRFLD **)FEreqmem((u_i4)tag,
			    (u_i4)(newrows * sizeof(RGRFLD *)),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				return(NULL);
			}
			MEcopy((PTR) trgptr->rgrows,
				(u_i2) (trgptr->rgmaxrows * sizeof(RGRFLD *)),
				(PTR) rowarray);
		/*
			MEfree(trgptr->rgrows);
		*/
			trgptr->rgrows = rowarray;
			trgptr->rgmaxrows = newrows;
		}

		currow += trgptr->rgtoprow;

		if (trgptr->rglastrow < currow)
		{
			trgptr->rglastrow = currow;
		}

		if ((rrgptr = trgptr->rgrows[currow]) == NULL)
		{
			if ((trgptr->rgrows[currow] =
			    (RGRFLD *)FEreqmem((u_i4)tag,
			    (u_i4)(maxcol * sizeof(RGRFLD)),
			    TRUE, (STATUS *)NULL)) == NULL)
			{
				return(NULL);
			}

			rrgptr = trgptr->rgrows[currow];

			for (i = 0; i < maxcol; i++)
			{
				tbl->tfcurcol = i;
				if (FTargrfld(&(rrgptr[i]), tag,
					FTrgwsize(fld)) != OK)
				{
					return(NULL);
				}
			}
			tbl->tfcurcol = ocol;
		}
		rrgptr = &(rrgptr[curcol]);
	}
	return(rrgptr);
}



/*
**  Add a character to the tail buffer.
**  Make sure to check that if character to add is a blank and
**  we have not added anything, then just return.
*/

VOID
FTtladd(rgptr, ch)
RGRFLD	*rgptr;
char	ch;
{
	if (ch == ' '  && rgptr->tcur == NULL)
	{
		return;
	}
	if (rgptr->tcur == NULL)
	{
		rgptr->tcur = rgptr->tend;
	}
	else if (rgptr->tcur == rgptr->tbuf)
	{
		FTrngxpand(rgptr, RG_T_ADD);
		(rgptr->tcur)--;
	}
	else
	{
		(rgptr->tcur)--;
	}
	*(rgptr->tcur) = ch;
}


/*
**  Add a char to the head buffer.
*/
VOID
FThdadd(rgptr, ch)
RGRFLD	*rgptr;
char	ch;
{
	if (rgptr->hcur == NULL)
	{
		rgptr->hcur = rgptr->hbuf;
	}
	else if (rgptr->hcur == rgptr->hend)
	{
		FTrngxpand(rgptr, RG_H_ADD);
		(rgptr->hcur)++;
	}
	else
	{
		(rgptr->hcur)++;
	}
	*(rgptr->hcur) = ch;
}


/*
**  Shift the HEAD or TAIL through the window "i" times.
**  If i is less than zero then shifting left (i.e., stealing
**  from the HEAD and pushing onto the TAIL.  Greater than
**  zero then shifting right (i.e., stealing from TAIL and
**  pushing onto the HEAD.
*/
VOID
FTrngshft(rgptr, i, win, reverse)
RGRFLD	*rgptr;
i4	i;
WINDOW	*win;
bool	reverse;
{
	i4	j = 0;
	i4	x;
	i4	y;
	i4	length;
	char	*cp;
	char	*bp;
	char	*end;
	char	*cur;
	char	buf[4096];

	if (i == 0)
	{
		return;
	}

	if (reverse)
		TDwin_rstr(win, buf, FALSE);
	else
		TDwin_str(win, buf, FALSE);
	length = STlength(buf);
	end = buf + length - 1;
	if (i < 0)
	{
		/*
		**  Shift from head to tail.
		*/

		i = -i;
		cp = end;
		cur = rgptr->hcur;
		while (i > length)
		{
			for (j = 0; j < length; j++)
			{
				FTtladd(rgptr, *cp--);
			}
			i -= length;
			cp = end;
			for (j = 0; j < length; j++)
			{
				/*
				**  Yes, we are assuming there are
				**  enough chars to satisfy this loop.
				*/

				*cp-- = *cur--;
			}
			cp = end;
		}

		/*
		**  Add the remainder to the tail buffer.
		*/

		for (j = 0; j < i; j++)
		{
			FTtladd(rgptr, *cp--);
		}

		/*
		**  Shift the characters internally.
		*/

		for (; cp >= buf; )
		{
			*end-- = *cp--;
		}

		/*
		**  Shift in new chars from the head buffer.
		*/

		for (j = 0, end = buf + i - 1; j < i; j++)
		{
			*end-- = *cur--;
		}

		/*
		**  Adjust the current position of the head buffer.
		*/

		if (cur < rgptr->hbuf)
		{
			rgptr->hcur = NULL;
		}
		else
		{
			rgptr->hcur = cur;
		}
		if (reverse)
			x = win->_maxx - 1;
		else
			x = 0;
		y = 0;
	}
	else
	{
		/*
		**  Shift from tail to head.
		*/

		cp = buf;
		cur = rgptr->tcur;
		while (i > length)
		{
			for (j = 0; j < length; j++)
			{
				FThdadd(rgptr, *cp++);
			}
			i -= length;
			cp = buf;
			for (j = 0; j < length; j++)
			{
				/*
				**  Yes, we are assuming there are enough
				**  chars to satisfy this loop.
				*/

				*cp++ = *cur++;
			}
			cp = buf;
		}

		/*
		**  Add remainder to head buffer.
		*/

		for (j = 0; j < i; j++)
		{
			FThdadd(rgptr, *cp++);
		}

		/*
		**  Shift chars internally.
		*/

		bp = buf;
		for (; cp <= end; )
		{
			*bp++ = *cp++;
		}

		/*
		**  Shift in new characters from the tail buffer.
		*/

		for (j = 0; j < i; j++)
		{
			*bp++ = *cur++;
		}

		/*
		**  Adjust current position of tail buffer.
		*/

		if (cur > rgptr->tend)
		{
			rgptr->tcur = NULL;
		}
		else
		{
			rgptr->tcur = cur;
		}
		y = win->_maxy - 1;
		if (reverse)
			x = 0;
		else
			x = win->_maxx - 1;
	}

	/*
	**  Fix up what is in the window.
	*/

	if (reverse)
	{
		TDmove(win, (i4) 0, win->_maxx - 1);
		TDraddstr(win, buf);
	}
	else
	{
		TDmove(win, (i4) 0, (i4) 0);
		TDaddstr(win, buf);
	}
	TDmove(win, y, x);
}

VOID
FTrngfldres(rgptr, win, reverse)
RGRFLD	*rgptr;
WINDOW	*win;
bool	reverse;
{
	i4	i;

	if (rgptr->hcur != NULL)
	{
		i = rgptr->hcur - rgptr->hbuf + 1;
		FTrngshft(rgptr, -i, win, reverse);
	}
}



/*
**  Stuff a string from tail buffer into a buffer starting at last position.
*/

VOID
FTtlstuff(rgptr, size, cp)
RGRFLD	*rgptr;
i4	size;
char	*cp;
{
	char	*bp;
	i4	i = 0;

	for (bp = rgptr->tend; i < size; i++)
	{
		*cp-- = *bp--;
	}
}



/*
**  Expand buffer space for either the HEAD or TAIL.
*/

STATUS
FTrngxpand(rgptr, which)
RGRFLD	*rgptr;
i4	which;
{
	i4	size;
	i4	nextsize;
	char	*cp;

	if (which == RG_H_ADD)
	{
		/*
		**  Add to HEAD buffer.
		*/

		size = rgptr->hend - rgptr->hbuf;
	}
	else
	{
		/*
		**  Add to TAIL buffer.
		*/

		size = rgptr->tend - rgptr->tbuf;
	}

	nextsize = ++size + MAX_I_BFSIZE;

	if ((cp = (char *)FEreqmem((u_i4)FTrngtag, (u_i4)nextsize, TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		return(FAIL);
	}

	if (which == RG_H_ADD)
	{
		MEcopy(rgptr->hbuf, size, cp);
	/*
		MEfree(rgptr->hbuf);
	*/
		rgptr->hcur = cp + size - 1;
		rgptr->hbuf = cp;
		rgptr->hend = rgptr->hbuf + nextsize - 1;
	}
	else
	{
		FTtlstuff(rgptr, size, cp + nextsize - 1);
	/*
		MEfree(rgptr->tbuf);
	*/
		rgptr->tbuf = cp;
		rgptr->tend = cp + nextsize - 1;
		rgptr->tcur = rgptr->tend - size + 1;

	}

	return(OK);
}


RGTFLD *
FTgettg(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	RGFLD	*range;
	RGFLD	*rgptr;

	range = (RGFLD *) frm->frrngptr;

	/*
	**  Yes, we are blindly assuming that
	**  the field at "fldno" is a table field.
	*/

	rgptr = &(range[fldno]);
	return(rgptr->rg_var.rgtblfld);
}


/*
**  Copy information up in a table field.
**  Aka scrolling info up in a table field
**  or a down row.
*/

VOID
FTrngucp(frm, fldno, tbl, count)
FRAME	*frm;
i4	fldno;
TBLFLD	*tbl;
i4	count;
{
	i4	i;
	i4	j;
	i4	rows;
	i4	cols;
	i4	ocol;
	i4	orow;
	FLDHDR	*hdr;
	FLDVAL	*val;
	FLDVAL	*sval;
	WINDOW	*dwin;
	WINDOW	*swin;
	RGRFLD	*rgptr;
	RGTFLD	*tgptr;
	char	bufr[4096];

	cols = tbl->tfcols;
	rows = tbl->tfrows;
	ocol = tbl->tfcurcol;
	orow = tbl->tfcurrow;
	tgptr = FTgettg(frm, fldno);

	for (i = 0; i < count; i++)
	{
		tbl->tfcurrow = i;
		val = tbl->tfwins + (i * cols);
		for (j = 0; j < cols; j++, val++)
		{
			tbl->tfcurcol = j;
			rgptr = FTgetrg(frm, fldno);
			swin = FTwmvtab(tbl, j, val);
			FTwinstr(frm, fldno, swin, rgptr->rwin);
		}
	}
	tgptr->rgtoprow += count;

	for (i = count; i < rows; i++)
	{
		val = tbl->tfwins + ((i - count) * cols);
		sval = tbl->tfwins + (i * cols);
		for (j = 0; j < cols; j++, val++, sval++)
		{
			/*
			** works for reverse fields since getting
			** and putting immediately, without trimming
			** blanks.
			*/
			swin = FTwmvtab(tbl, j, sval);
			TDwin_str(swin, bufr, FALSE);
			/* dwin will be NULL if current column invisible */
			if ( (dwin = FTwmvtab(tbl, j, val)) != NULL )
			{
				TDaddstr(dwin, bufr);
			}
		}
	}
	for (i = rows - count; i < rows; i++)
	{
		tbl->tfcurrow = i;
		val = tbl->tfwins + (i * cols);
		for (j = 0; j < cols; j++, val++)
		{
			tbl->tfcurcol = j;
			rgptr = FTgetrg(frm, fldno);
			if ( (dwin = FTwmvtab(tbl, j, val)) != NULL)
			{
				TDerschars(dwin);
				if (*(rgptr->rwin) != '\0')
				{
					FTaddstr(frm, fldno, dwin, rgptr->rwin);
				}
			}
		}
	}
	tbl->tfcurcol = ocol;
	tbl->tfcurrow = orow;

	hdr = &(tbl->tfhdr);
	dwin = TDsubwin(FTwin, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy,
		hdr->fhposx, FTutilwin);
	TDtouchwin(dwin);
	TDrefresh(dwin);

# ifdef DATAVIEW
	_VOID_ IIMWrucRngUpCp(frm, fldno, count);
# endif	/* DATAVIEW */
}


/*
**  Copy information down in a table field.
**  Aka scrolling info down in a table field
**  or an up row.
*/
VOID
FTrngdcp(frm, fldno, tbl, count)
FRAME	*frm;
i4	fldno;
TBLFLD	*tbl;
i4	count;
{
	i4	i;
	i4	j;
	i4	cols;
	i4	rows;
	i4	ocol;
	i4	orow;
	FLDHDR	*hdr;
	FLDVAL	*val;
	FLDVAL	*sval;
	WINDOW	*dwin;
	WINDOW	*swin;
	RGRFLD	*rgptr;
	RGTFLD	*tgptr;
	char	bufr[4096];

	cols = tbl->tfcols;
	rows = tbl->tfrows;
	ocol = tbl->tfcurcol;
	orow = tbl->tfcurrow;
	tgptr = FTgettg(frm, fldno);

	if (tgptr->rgtoprow == 0)
	{
		tbl->tfcurrow = 0;
		return;
	}

	/*
	**  In case user wants to scroll more than we have.
	*/
	if (tgptr->rgtoprow < count)
	{
		count = tgptr->rgtoprow;
	}

	/*
	**  Save the bottom rows into internal structs before they
	**  are overridden by top rows being scrolled down.
	*/
	tbl->tfcurrow = tbl->tfrows;
	for (i = 0; i < count; i++)
	{
		(tbl->tfcurrow)--;
		val = tbl->tfwins + (tbl->tfcurrow * cols);
		for (j = 0; j < cols; j++, val++)
		{
			tbl->tfcurcol = j;
			rgptr = FTgetrg(frm, fldno);
			swin = FTwmvtab(tbl, j, val);
			FTwinstr(frm, fldno, swin, rgptr->rwin);
		}
	}

	for (i = rows - 1 - count; i >= 0; i--)
	{
		val = tbl->tfwins + ((i + count) * cols);
		sval = tbl->tfwins + (i * cols);
		for (j = 0; j < cols; j++, val++, sval++)
		{
			/*
			** works for reverse fields since getting
			** and putting immediately, without trimming
			** blanks.
			*/
			swin = FTwmvtab(tbl, j, sval);
			TDwin_str(swin, bufr, FALSE);
			/* dwin will be NULL if current column invisible */
			if ( (dwin = FTwmvtab(tbl, j, val)) != NULL )
			{
				TDaddstr(dwin, bufr);
			}
		}
	}

	tgptr->rgtoprow -= count;

	for (i = 0; i < count; i++)
	{
		tbl->tfcurrow = i;
		val = tbl->tfwins + (i * cols);
		for (j = 0; j < cols; j++, val++)
		{
			tbl->tfcurcol = j;
			rgptr = FTgetrg(frm, fldno);
			if ( (dwin = FTwmvtab(tbl, j, val)) != NULL)
			{
				TDerschars(dwin);
				if (*(rgptr->rwin) != '\0')
				{
					FTaddstr(frm, fldno, dwin, rgptr->rwin);
				}
			}
		}
	}

	tbl->tfcurcol = ocol;
	tbl->tfcurrow = orow;

	hdr = &(tbl->tfhdr);
	dwin = TDsubwin(FTwin, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy,
		hdr->fhposx, FTutilwin);
	TDtouchwin(dwin);
	TDrefresh(dwin);

# ifdef DATAVIEW
	_VOID_ IIMWrdcRngDnCp(frm, fldno, count);
# endif	/* DATAVIEW */
}


i4
FTrgscrup(frm, fldno, tbl, repts)
FRAME	*frm;
i4	fldno;
TBLFLD	*tbl;
i4	repts;
{
	RGTFLD	*tgptr;
	i4	toprow;
	i4	lstrow;
	i4	count;
	i4	drow;
	i4	i;

	tgptr = FTgettg(frm, fldno);
	toprow = tgptr->rgtoprow;
	lstrow = tgptr->rglastrow;
	if (toprow + tbl->tfcurrow + repts > lstrow)
	{
		/*
		**  Trying to scroll beyond "DATA SET".
		**  Reduce number of rows we need to move.
		*/
		count = lstrow - (toprow + tbl->tfcurrow) + 1; 
	}
	else
	{
		count = repts;
	}

	if ((drow = tbl->tfcurrow + count) < tbl->tfrows)
	{
		/*
		**  Don't need to move any rows physically.
		*/
		for (i = tbl->tfcurrow + 1; i <= drow; i++)
		{
			tbl->tfcurrow = i;
		}
	}
	else
	{
		count = drow - (tbl->tfrows - 1);
		FTrngucp(frm, fldno, tbl, count);
		tbl->tfcurrow = tbl->tfrows - 1;
	}
	drow = tgptr->rgtoprow + tbl->tfcurrow;
	if (tgptr->rglastrow <  drow)
	{
		tgptr->rglastrow = drow;
	}
}



bool
FTchkrng(frm, fldno, col, rgptr, bffunc, bfvalue, dml_level)
FRAME	*frm;
i4	fldno;
i4	col;
RGRFLD	*rgptr;
STATUS	(*bffunc)();
PTR	bfvalue;
i4	dml_level;
{
	i4			tag;
        char			*tempbuf = NULL;
	DB_TEXT_STRING		*text;
	DB_DATA_VALUE		dbv;
	i4			stlen;
	u_char			*buf;
	char			*fname;
	char			*cname;
	char			*ufldname;
	FIELD			*fld;
	FLDHDR			*hdr;
	TBLFLD			*tbl;
	STATUS			rval;

	tag = frm->frrngtag;

	fld = frm->frfld[fldno];
	if (fld->fltag == FREGULAR)
	{
		hdr = (*FTgethdr)(fld);
		fname = hdr->fhdname;
		cname = NULL;
		ufldname = fname;
	}
	else
	{
		tbl = fld->fld_var.fltblfld;
		hdr = &(tbl->tfhdr);
		fname = hdr->fhdname;
		hdr = &(tbl->tfflds[col]->flhdr);
		cname = hdr->fhdname;
		ufldname = cname;
	}

        if ((tempbuf = FTrngcat_alloc(rgptr)) == NULL)
        {
            IIUGbmaBadMemoryAllocation(ERx("FTchkrng"));
        }

	text = (DB_TEXT_STRING *) tempbuf;
	buf = text->db_t_text;

	/*
	**  Call will place the range buffers into "buf" and
	**  null terminate it.
	*/
	FTrngcat(rgptr, buf);

# ifdef DATAVIEW
	_VOID_ IIMWgrvGetRngVal(frm, fldno, tbl, buf);
# endif	/* DATAVIEW */

	if (rgptr->pdat != NULL)
	{
		MEfree(rgptr->pdat);
		rgptr->pdat = NULL;
	}
	if (buf[0] != '\0')
	{
		if (hdr->fhdflags & fdREVDIR)
			FTswapparens(buf);
		stlen = STlength((char *) buf);
		text->db_t_count = stlen;
		dbv.db_datatype = DB_LTXT_TYPE;
		dbv.db_length = stlen + DB_CNTSIZE;
		dbv.db_prec = 0;
		dbv.db_data = (PTR) text;
		rval = (*FTrngchk)(frm, fname, cname, &dbv, (bool) FALSE,
			(char *) NULL, (char *) NULL, (bool) FALSE,
			dml_level, NULL, NULL, NULL);
		if (rval != OK && rval != QG_NOSPEC)
		{
			frm->frcurfld = fldno;
                        MEfree(tempbuf);
			return(FALSE);
		}
		if (hdr->fhdflags & fdREVDIR)
			FTswapparens(buf);
		rgptr->pdat = STtalloc((u_i2) tag, (char *) buf);

		if (bffunc != NULL)
		{
		    _VOID_ (*bffunc)(ufldname, bfvalue);
		}
	}
        MEfree(tempbuf);
	return(TRUE);
}


bool
FTrngtrv(frm, fldno, tbl, tgptr, maxcol, startrow, endrow, bffunc,
		bfvalue, dml_level)
FRAME	*frm;
i4	fldno;
TBLFLD	*tbl;
RGTFLD	*tgptr;
i4	maxcol;
i4	startrow;
i4	endrow;
STATUS	(*bffunc)();
PTR	bfvalue;
i4	dml_level;
{
	i4	j;
	i4	k;
	RGRFLD	*rgptr;

# ifdef DATAVIEW
	_VOID_ IIMWbqtBegQueryTbl();
# endif /* DATAVIEW */

	for (j = startrow; j < endrow; j++)
	{
		tgptr->rgtoprow = j;
		for (k = 0; k < maxcol; k++)
		{
			tbl->tfcurcol = k;
			rgptr = FTgetrg(frm, fldno);
			if (!FTchkrng(frm, fldno, k, rgptr, bffunc,
				bfvalue, dml_level))
			{

# ifdef DATAVIEW
				_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

				return(FALSE);
			}
		}
	}

# ifdef DATAVIEW
	_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

	return(TRUE);
}


/*
** add a string to the window, taking note of which
** end of the window gets the string's beginning.
*/
VOID
FTaddstr(frm, fldno, win, string)
FRAME	*frm;
i4	fldno;
WINDOW	*win;
char	*string;
{
	FLDHDR	*hdr;

	hdr = (*FTgethdr)(frm->frfld[fldno]);
	if (hdr->fhdflags & fdREVDIR)
	{
		win->_curx = win->_maxx - 1;
		TDraddstr(win, string);
	}
	else
	{
		/* win->_curx is already 0, so no need to modify it here */
		TDaddstr(win, string);
	}
}


/*
** get a string from the window, taking note of
** which end of the window contains the start of the string.
*/
VOID
FTwinstr(frm, fldno, win, outbuf)
FRAME	*frm;
i4	fldno;
WINDOW	*win;
char	*outbuf;
{
	FLDHDR	*hdr;

	hdr = (*FTgethdr)(frm->frfld[fldno]);
	if (hdr->fhdflags & fdREVDIR)
	{
		TDwin_rstr(win, outbuf, FALSE);
	}
	else
	{
		TDwin_str(win, outbuf, FALSE);
	}
}
