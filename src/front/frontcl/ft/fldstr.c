/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	FLDSTR.c  -  Dump Field to String
**	
**	This routine takes the contents of the data window of a
**	field and places it in a character buffer.  Any trailing
**	blanks are deleted.
**	
**	Arguments:  fld    - field to dump
**		    string - string to dump into
**	
**	History:  JEN - 1 Nov 1981  (written)
**		  DKH - 17 July 1985 Updated for no-echo attribute.
**	06/19/87 (dkh) - Code cleanup.
**	09-nov-87 (bab)
**		Added code to handle scrolling fields; get data from
**		underlying scroll buffer when called for such a field.
**	09-feb-88 (bruceb)
**		Changed scrolling flag to sit in fhd2flags; can't use
**		most significant bit of fhdflags.
**	01-apr-88 (bruceb)
**		Catch input that is too long for the underlying data
**		space (for character datatypes only.)
**	20-apr-89 (bruceb)
**		Added IIFTefsEmptyFldStr() to handle placing empty
**		contents into the fvdsdbv.  For clearrest semantics
**		in mandatory fields.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	04/03/90 (dkh) - Integrated MACWS changes.
**	16-apr-90 (bruceb)
**		Added IIFTrfsRestoreFldStr() to 'reverse' the effects
**		of calling IIFTefs().
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	03/21/91 (dkh) - Integrated changes from PC group.
**	07/19/93 (dkh) - Changed call to adc_lenchk() to match
**			 interface change.  The second parameter is
**			 now a i4  instead of a bool.
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
# include	"ftframe.h"
# include	<afe.h>
# include	<scroll.h>
# include	<erft.h>
# include	<er.h>

FUNC_EXTERN ADF_CB	*FEadfcb();
FUNC_EXTERN VOID	IIAFddcDetermineDatatypeClass();
FUNC_EXTERN VOID	FTbotsav();
FUNC_EXTERN VOID	FTbotrst();

static	i4	IIFTstrlen = 0;	/* Set by IIFTefs(). */


FTfld_str(hdr, val, win)
FLDHDR	*hdr;
FLDVAL	*val;
WINDOW	*win;
{
	reg	DB_TEXT_STRING	*text;
	reg	i4		i;
	reg	i4		count = 0;
	reg	u_char		*cur;
	reg	u_char		*end;
	reg	u_char		*cp;
	reg	char		*dx;
		SCROLL_DATA	*scroll;
		i4		class;
		i4		internlen;
		DB_DATA_VALUE	sdbv;

	text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
	cp = val->fvbufr;

# ifdef DATAVIEW
	/*
	**  The value of the field has already been placed in val->fvbufr
	**  as well as in the scroll structure if it is a scrolling field.
	**  We just set win to NULL so that this function does not do any
	**  copying.
	*/
	if (IIMWimIsMws())
		win = NULL;
# endif	/* DATAVIEW */

	/*
	**  Set up pointer to trim trailing blanks if
	**  we are accessing a field that is not currently
	**  displayed or it is a no eho field.
	*/
	if (win == NULL || (hdr->fhdflags & fdNOECHO))
	{
		count = text->db_t_count;
		cp += count;
	}
	else if (hdr->fhd2flags & fdSCRLFD)
	{
		/*
		**  Get characters from the underlying scroll
		**  buffer into the display buffer.
		*/
		scroll = (SCROLL_DATA *)val->fvdatawin;
		cur = (u_char *)scroll->left;
		end = (u_char *)scroll->right;
		count = end - cur + 1;
		while (cur <= end)
			*cp++ = *cur++;
	}
	else
	{
		/*
		**  Get characters off of field and into the
		**  display buffer.
		*/
		for (i = 0; i < win->_maxy; i++)
		{
			cur = (u_char *) &win->_y[i][0];
			end = (u_char *) &win->_y[i][win->_maxx - 1];
			dx = &win->_dx[i][0];
			while (cur <= end)
			{
				if (*dx & _PS)
				{
					cur++;
				}
				else
				{
					count++;
					*cp++ = *cur++;
				}
				dx++;

			}
		}
	}

	/*
	**  Set up for trimming trailing blanks.
	*/
	end = val->fvbufr;
	cp--;

	/*
	**  Trim trailing blanks and decrement count as
	**  we go along.
	*/
	while (cp >= end)
	{
		if (*cp != ' ')
		{
			break;
		}
		cp--;
		count--;
	}

	IIAFddcDetermineDatatypeClass(val->fvdbv->db_datatype, &class);
	if (class == CHAR_DTYPE)
	{
		adc_lenchk(FEadfcb(), FALSE, val->fvdbv, &sdbv);
		internlen = sdbv.db_length;
		if (count > internlen)
		{
			/*
			**  If truncating the string, save the bottom line
			**  of the screen, put out the warning message,
			**  restore the bottom line, and reset the length
			**  count.
			*/
			FTbotsav();
			IIUGmsg(ERget(F_FT0010_STR_TRUNCATED), (bool)TRUE,
				1, hdr->fhdname);
			FTbotrst();
			count = internlen;
		}
	}

	/*
	**  Set count for display buffer.
	*/
	text->db_t_count = count;
}


/*{
** Name:	IIFTefsEmptyFldStr	- Empty out a field's fvdsdbv
**
** Description:
**	Empty out a field's fvdsdbv (fvbufr) by setting the count field
**	to 0.  Used by FTinsert as a means of clearing the buffer, when
**	a user empties a mandatory field, without losing the contents of
**	the window/scrolling buffer.  Not used with noecho fields.
**
** Inputs:
**	val	FLDVAL for the field being 'cleared'.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	IIFTstrlen is set for restoration by IIFTrfs().
**
** History:
**	20-apr-89 (bruceb)	Written.
*/

VOID
IIFTefsEmptyFldStr(val)
FLDVAL	*val;
{
    IIFTstrlen = ((DB_TEXT_STRING *) val->fvdsdbv->db_data)->db_t_count;
    ((DB_TEXT_STRING *) val->fvdsdbv->db_data)->db_t_count = 0;
}


/*{
** Name:	IIFTrfsRestoreFldStr	- Restore a field's fvdsdbv
**
** Description:
**	Restore a field's fvdsdbv (fvbufr) by resetting the count field
**	to its prior value.  This value was stashed away in 'strfld' by
**	IIFTefs().  Used by FTinsert as a means of restoring the buffer, after
**	a user empties a mandatory field.  Not used with noecho fields.
**
** Inputs:
**	val	FLDVAL for the field being restored
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	16-apr-90 (bruceb)	Written.
*/

VOID
IIFTrfsRestoreFldStr(val)
FLDVAL	*val;
{
    ((DB_TEXT_STRING *) val->fvdsdbv->db_data)->db_t_count = IIFTstrlen;
}
