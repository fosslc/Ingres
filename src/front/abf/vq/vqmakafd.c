/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	<er.h>
# include	<cm.h>
# include	<fmt.h>
# include	<ft.h>
# include	<adf.h>
# include	<frame.h>
# include	<abclass.h>
# include	<oocat.h>
# include	<ooclass.h>
# include	<oodefine.h>

# include	<metafrm.h>

# include	"ervq.h"
# include	"vqloc.h"
# include	"vqhotspo.h"


/*
** Name:	vqmakafd.c -	make frame flow diagram frame
**
** Description:
**	This file has routines to dynamically create the frame flow
*	diagram frame.
**
**	This file defines:
**
**	IIVQmffMakeFfFrame	make initial frame flow diagram frame
**
** History:
**	11/26/89 (tom) - extracted from ff.qsc 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

# define FWID 15                /* frame deptiction box width */

static i4  _upper_con_col;  	/* column that the upper connector line
				   meets the horz. connector line */
static i4  _start_con_col;  	/* start column for the horz. connector */
static i4  _end_con_col;  	/* end column for the horz. connector */

static char *_expr_char;	/* expand right left and down characters */
static char *_expl_char;
static char *_expd_char;

static char *_from_text;	/* text that indicates that the top frame
				   in the afd display is called from another
				   frame that is presently off the diagram. */
static i4  _from_len;

/* change and error, and new indicators for the frame display, 
   and their lengths */
static char *_chg_ind;
static i4  _chg_len = 0;
static char *_err_ind;
static i4  _err_len;
static char *_cust_ind;
static i4  _cust_len;
static char *_new_ind;
static i4  _new_len;
static char *_id2_txt;
static i4  _id2_len;

#define LOC static 
LOC bool ff_PutFrame(); 
LOC VOID ff_PostLimit(); 
LOC bool ff_HorzConn(); 
LOC VOID ff_PostHotSpot();

	/* dynamic frame creation routines */
FUNC_EXTERN FRAME *IIFDofOpenFrame();	

FUNC_EXTERN i4  IIVQnmNumMenus();
FUNC_EXTERN VOID IIVQcsCenterString();
FUNC_EXTERN VOID IIVQtsTruncateString();

GLOBALREF APPL *IIVQappl;
GLOBALREF IIVQrowmax;
GLOBALREF IIVQcolmax;



/*{
** Name:	IIVQmffMakeFfFrame	- make frame flow diagram frame
**
** Description:
**	This routine drives off the ff state struct, and creates a
**	display frame for use in displaying the frame flow diagram.
**
** Inputs:
**	char *ffform;		- name of frame to create
**	FF_STATE *ff_table;	- start of stack of frames.
**	FF_STATE *ff;		- pointer into the stack
**	OO_OBJECT *cur_obj;	- the current object
**	FRMINFO *frm_info;  - ptr to an array of frame info
**
** Outputs:
**	Returns:
**		FRAME *	  ptr to newly created frame struct	
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/26/89 (tom) - created
*/

FRAME *
IIVQmffMakeFfFrame(ffform, ff_table, ff, cur_obj, frm_info)
char *ffform;
FF_STATE *ff_table;
FF_STATE *ff;
OO_OBJECT *cur_obj;
FRMINFO *frm_info;
{
	FRAME	*fr;
	MFMENU	**menu;
	i4	row;
	i4	i;
	i4	len;
	i4	num;
	i4	disp;
	i4	nummenus;
	i4	coloff;
	i4	expcol;
	char	buf[100];

	/* reset the button array pointer */
	IIVQhsiHotSpotIdx = 0;

	/* if it's the first time called.. fetch message text */
	if (_chg_len == 0)
	{
		_expr_char = ERget(F_VQ0020_expr_char);
		_expl_char = ERget(F_VQ0021_expl_char);
		_expd_char = ERget(F_VQ0022_expd_char);

		_from_text = ERget(F_VQ0023_from_text);
		_from_len = STlength(_from_text);
		
		_chg_ind = ERget(F_VQ001C_chg_ind);
		_chg_len = STlength(_chg_ind);

		_err_ind = ERget(F_VQ001D_err_ind);
		_err_len = STlength(_err_ind);

		_new_ind = ERget(F_VQ001E_new_ind);
		_new_len = STlength(_new_ind);

		_cust_ind = ERget(F_VQ001F_cust_ind);
		_cust_len = STlength(_cust_ind);

		_id2_txt = ERget(F_VQ00F8_FF_id2);
		_id2_len = STlength(_id2_txt);
	}

	/* this is a gross check of the message file */
	if (_id2_len > 26)
	{
		IIVQer0(E_VQ00CC_Bad_Msg_File);
		return (NULL);
	}

	/* open a dynamic frame */
	if ((fr = IIFDofOpenFrame(ffform, (PTR)NULL)) == NULL) 
		return (NULL);

		
	/* output the frame title */
	IIFDattAddTextTrim(fr, 0, 0, (i4)fd2COLOR, ERget(F_VQ00F7_FF_ident));

	/* output the name of the application we are working with */

	/* first the mock field title */

	IIFDattAddTextTrim(fr, 1, 0, (i4)0, _id2_txt);

	len = CMcopy(IIVQappl->name, 26 - _id2_len, buf);

	/* if we truncated the name, append dots.. */
	if (len != STlength(IIVQappl->name))
	{
		buf[len++] = ERx('.');
		buf[len++] = ERx('.');
	}
	buf[len] = EOS;

	/* put out the name of the application as if it were a field */  
	IIFDattAddTextTrim(fr, 1, _id2_len, (i4)fd1COLOR, buf);

	/* put the top frame on the screen */
	if (ff_PutFrame(ff, fr, ff[0].ao, (char*)NULL, 0, 0, 0, TRUE, cur_obj, 
		frm_info) == FALSE)
	{
		return (NULL);
	}

	/* if we are not at the top of the application tree */
	if (ff != ff_table)
	{
		/* put out the '..from: ' text */
		IIFDattAddTextTrim(fr, 2, 47, (i4)0, _from_text);

		/* put out the name of the frame that called us */
		IIFDattAddTextTrim(fr, 2, 47 + _from_len,
			(i4)fd1COLOR, ff[-1].ao->name);

		ff_PostHotSpot(fr, 1, 47, 3, 
			47 + _from_len + STlength(ff[-1].ao->name), 
			-1, 0, FALSE);
	}

	/* for each of the rows in the frame flow state array */
	for (row = 0; ff[row].ao != NULL && row < IIVQrowmax - 1; ++row)
	{

		/* if the number of columns to be displayed in this
		   row is less than the max allowed.. then attempt
		   to center the cols, .. if not then limit the
		   number of frames displayed on this row to our 
		   screen constrained maximum */
		num = nummenus = IIVQnmNumMenus(ff[row].ao);
	
		if (num == 0)
		{
			break;
		}


		if (num < IIVQcolmax) 
		{
			coloff = (IIVQcolmax - num) / 2;
		}
		else
		{
			coloff = 0;
			num = IIVQcolmax;
		}

		/* post the column which was posted by the
		   parent frame's connector before we start
		   to paint the child frames */
		ff_PostLimit(TRUE, _upper_con_col);

		/* get a pointer into the menuitem pointer array 
		   for the first displayed frame on this row */
		disp = ff[row + 1].disp;
		menu =  &((USER_FRAME*)ff[row].ao)->mf->menus[disp];
		expcol = ff[row + 1].col;

		/* for the frames to be displayed on this row */
		for (i = 0; i < num; i++, menu++)
		{
			/* put the frame representation on the frame */
			if (ff_PutFrame(ff, fr, (*menu)->apobj, (*menu)->text, 
					row + 1, i + coloff, disp + i,
					i == expcol, cur_obj, frm_info) 
					== FALSE)
			{
				return (NULL);
			}
		}

		/* paint the horizontal connector which runs above
		   the frames we just painted.. pass in whether there
		   are expansion points on the each of the ends */
		if (ff_HorzConn(fr, disp > 0, disp + num < nummenus, row,
			disp - 1, disp + num) 
			== FALSE)
		{
			return (NULL);
		}

	}

	/* close the frame dynamic frame definition */
	if (IIFDcfCloseFrame(fr) == FALSE)
	{
		return (NULL);
	}

	return (fr);
}

/*{
** Name:	ff_PutFrame	- put a frame representation onto dyn frame
**
** Description:
**	Put a frame representation onto the dynamically created frame.
**
** Inputs:
**	FRAME	*fr;	dynamic frame to put new mf representation into
**	OO_OBJECT *ao;	ptr to application object to output
**	char *mtext;	menuitem text
**	i4 row;	screen row we are to paint on 
**	i4 col;	column of ff (these coords are not frame
**				coords.. rather they are rows and
**				cols of the frame representations)
**	i4 idx;	index into the menu array of this item
**	bool expand_below; - flag it say whether we are to expand 
**			     frames below or just leave an arrow
**	OO_OBJECT *cur_obj; - current object
**	FRMINFO *frm_info;  - ptr to an array of frame info
**	
**
** Outputs:
**
**	Returns:
**		bool	TRUE = success, else allocation failed
**	
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/26/89 (tom) written
*/
LOC bool
ff_PutFrame(ff, fr, ao, mtext, row, col, idx, expand_below, cur_obj, frm_info)
FF_STATE *ff;
FRAME	*fr;
OO_OBJECT *ao;
char *mtext;
i4  row;
i4  col;
i4  idx;
bool expand_below;
OO_OBJECT *cur_obj;
FRMINFO *frm_info;
{
	register i4	fcol;
	register i4	flin;
	i4 len;
	char *txt;
	USER_FRAME *uf;
	char buf[100];

	/* the top row is in a special position, and has no menuitem 
	   but it does have the "..from frame: blah" text */
	if (row == 0)
	{
		fcol = 29;
		flin = 1;
	}
	else	
	{
		fcol = 1 + 19 * col;
		flin = 8 * row + 1;

		/* truncate the menuitem text into the buffer */
		len = CMcopy(mtext, 16, buf);

		/* if we truncated then append.. */
		if (STlength(mtext) != len)
		{
			buf[len++] = ERx('.');
			buf[len++] = ERx('.');
		}
		buf[len] = 0;

		/* put the menu item text on as trim */
		if (IIFDattAddTextTrim(fr, flin - 2, fcol + 1, 
					(i4)fd1COLOR, buf)
				== NULL)
		{
			return (FALSE);
		}

		/* put the line that passes thru the menuitem text */
		if (IIFDabtAddBoxTrim(fr, flin-3, fcol+3, flin, fcol+3, (i4)0)
				== NULL)
		{
			return (FALSE);
		}
	}

	/* post the connector column for establishing the start and end
	   column limits of the horizontal line connector */
	ff_PostLimit(FALSE, fcol + 3);

	/* paint the box representing the frame */
	if (IIFDabtAddBoxTrim(fr, flin, fcol, flin + 2, fcol + FWID, (i4)0) 
			== NULL)
		return (FALSE);

	ff_PostHotSpot(fr, flin, fcol, flin + 2, fcol + FWID,    
			row, idx, FALSE);

	/* see if there is any status to report about this frame */
	len = 0;
	uf = (USER_FRAME*)ao;
	switch (ao->class)
	{
	case OC_MUFRAME:
	case OC_APPFRAME:
	case OC_UPDFRAME:
	case OC_BRWFRAME:
		/* see if it is a customized frame */
		if (uf->flags & CUSTOM_EDIT)
		{
			/* if it is custom.. we treat it like a 
			   user frame/proc */
			if (  uf->compdate != (char*)NULL 
			   && uf->compdate[0] != EOS
			   ) 
			{
				len = _err_len;
				txt = _err_ind;
			}
			else
			{
				len = _cust_len;
				txt = _cust_ind;
			}
		}
		/* see if there was an incomplete visual query */ 
		else if (uf->mf->state & (MFST_GENBAD | MFST_VQEERR))
		{
			len = _err_len;
			txt = _err_ind;
		}
		/* if the frame is brand new.. */
		else if (uf->mf->state & MFST_NEWFR)
		{
			len = _new_len;
			txt = _new_ind;
		}
		/* see if a generatable frame needs gen'ing */
		else if (uf->mf->state & MFST_DOGEN)
		{
			len = _chg_len;
			txt = _chg_ind;
		}
		else
		{
			goto CHECK_COMP_ERROR;
		}
		break;

	CHECK_COMP_ERROR:

	case OC_OSLFRAME:
	case OC_OSLPROC: 
	case OC_HLPROC:
		/* the compdate field is set to null upon successful
		   compiles, it is set to the date of the last 
		   failed compile attempt if the compile fails */
		if (uf->compdate != (char*)NULL && uf->compdate[0] != EOS) 
		{
			len = _err_len;
			txt = _err_ind;
		}
		break;
	}

	if (len != 0)	/* if we have a status indicator, put it up */
	{
		if (IIFDattAddTextTrim(fr, flin, fcol + FWID - len, 
			(i4) fd3COLOR, txt) == NULL)
		{
			return (FALSE);
		}
	}

	/* center the frame name into the buffer */
	IIVQcsCenterString(ao->name, buf, 14);

	/* paint the centered name into the box, the name is hilited
	   if it is the current app obj.. and it is the one that is
	   exapanded on the next row (the 'current' object could be
	   in more than one place on the row) */
	if (IIFDattAddTextTrim(fr, flin+1, fcol+1, 
			(i4)( (expand_below && ao == cur_obj)
				? fdRVVID : 0) + fd1COLOR, buf) == NULL)
		return (FALSE);

	/* paint the frame type into the botom border of the box */
	STprintf(buf, ERx("<%s>"), frm_info[IIVQotObjectType(ao)].name);
	if (IIFDattAddTextTrim(fr, flin + 2, fcol + FWID - STlength(buf), 
			(i4)0, buf) == NULL)
		return (FALSE);

	/* if this frame has any menuitems */
	if (IIVQnmNumMenus(ao))
	{
		/* if this frame is the frame that is expanded on 
		   the next level.. then remember the connector column,
		   and paint the upper connector line */ 
		if (  row < IIVQrowmax - 1 
		   && ff[row].ao == ao 
		   && expand_below
		   )
		{
			_upper_con_col = fcol + 3;

			if (IIFDabtAddBoxTrim(fr, flin + 2, fcol + 3, 
					flin + 5, fcol + 3, (i4)0) == NULL)
				return (FALSE);
		}
		else
		{
			/* paint short vertical line */
			if (IIFDabtAddBoxTrim(fr, flin + 2, fcol + 3, 
					flin + 3, fcol + 3, (i4)0) == NULL)
				return (FALSE);

			/* just paint the expansion character */
			if (IIFDattAddTextTrim(fr, flin + 3, fcol + 3, 
					(i4)0, _expd_char) == NULL)
				return (FALSE);

			ff_PostHotSpot(fr, flin + 3, fcol + 1, 
				flin + 4, fcol + 5,
				row, idx, TRUE);
		}
		    
	}
	
	return (TRUE);
}

/*{
** Name:	ff_PostLimit	- post the limits of horz. connector
**
** Description:
**	This function is called to post the limits of the horizontal
**	connector lines.. 
**	
**
** Inputs:
**	bool init;	are we to initialize the start and end
**	i4 col;	column that is required to be present
**			in the horz. line.
**
** Outputs:
**	Returns:
**		VOID
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/27/89 (tom) - written
*/
LOC VOID
ff_PostLimit(init, col)
bool init;
i4   col;	
{

	if (init)
		_start_con_col = _end_con_col = col;
	else if (col < _start_con_col)
		_start_con_col = col;
	else if (col > _end_con_col)
		_end_con_col = col;
}

/*{
** Name:	ff_HorzConn	- paint the horizontal connector line 
**
** Description:
**	Paint the horizontal connector line which runs just above the
**	menuitems which are right above the frame box representations.
**	The end points of this line are posted as the various
**	contributors are painted onto the form.
**
** Inputs:
**	FRAME *fr;	frame to paint the line in
**	bool left;	is an expansion to the left necessary
**	bool right;	is an expansion to the right necessary
**	bool row;	row that the connector is on
**	i4 idx_left;	index of frame to the left (hotspot navigation)
**	i4 idx_right;	index of frame to the right (hotspot navigation)
**
** Outputs:
**	Returns:
**		bool 	TRUE=success, FALSE allocation failure
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/27/89  (tom) - written
*/
LOC bool
ff_HorzConn(fr, left, right, row, idx_left, idx_right)
register FRAME *fr;
bool left;
bool right;
i4  row;
i4  idx_left;
i4  idx_right;
{
	register i4  flin;

	/* calulate the frame line number, given the row */
	flin = 5 + 8 * row + 1;

	/* post the left end expansion indicator */
	if (left)
	{
		_start_con_col -= 2;
		if (IIFDattAddTextTrim(fr, flin, 
				_start_con_col - 1, (i4)0, _expl_char) == NULL)
			return (FALSE);	

		ff_PostHotSpot(fr, flin - 1, _start_con_col - 2, 
			flin + 1, _start_con_col + 1,      
			row + 1, idx_left, FALSE);
	}

	/* post the right end expansion indicator */
	if (right)
	{
		_end_con_col += 2;
		if (IIFDattAddTextTrim(fr, flin, 
				_end_con_col + 1, (i4)0, _expr_char) == NULL)
			return (FALSE);	

		ff_PostHotSpot(fr, flin - 1, _end_con_col - 1, 
			flin + 1, _end_con_col + 2,      
			row + 1, idx_right, FALSE);
	}

	/* if there's a line to draw.. draw the line */
	if (_end_con_col - _start_con_col > 1)
		if (IIFDabtAddBoxTrim(fr, flin, _start_con_col, 
				flin, _end_con_col, (i4)0) == NULL)
			return (FALSE);
		
	return (TRUE);
}

/*{
** Name:	ff_PostHotSpot	- post a new button to the button array
**
** Description:
**	This routine creates an peice of invisible box trim which will
**	operate as a button using the windex technology.
**
** Inputs:
**	FRAME *fr;	frame to paint the line in
**	i4 blin;	begin line
**	i4 bcol;	begin col
**	i4 elin;	end line
**	i4 ecol;	end col 
**	i4 row;	row to remember
**	i4 col;	col to remember
**	bool down;	col to remember
**
** Outputs:
**	Returns:
**		bool 	TRUE=success, FALSE allocation failure
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/27/89  (tom) - written
*/
LOC VOID 
ff_PostHotSpot(fr, blin, bcol, elin, ecol, row, col, down)
register FRAME *fr;
i4  blin;
i4  bcol;
i4  elin;
i4  ecol;
i4  row;
i4  col;
bool down;
{
	TRIM *box;

	if (  IIVQhsiHotSpotIdx >= FF_MAXHOTSPOTS
	   || (box = IIFDabtAddBoxTrim(fr, blin, bcol, elin, ecol, 
		(i4)(fdTRHSPOT | fdINVISIBLE))) == NULL)
	{
		return;
	}
	fr->frmflags |= fdTRHSPOT;	

	IIVQhsaHotSpotArray[IIVQhsiHotSpotIdx].num = (i2)fr->frtrimno - 1;
	IIVQhsaHotSpotArray[IIVQhsiHotSpotIdx].row = (i2)row;
	IIVQhsaHotSpotArray[IIVQhsiHotSpotIdx].col = (i2)col;
	IIVQhsaHotSpotArray[IIVQhsiHotSpotIdx].down = (i2)down;

	IIVQhsiHotSpotIdx++;	
}



/*{
** Name:	IIVQcsCenterString	- center text in buffer
**
** Description:
**	Center text in buffer of a given length.
**
** Inputs:
**	char *text;	ptr to text
**	char *str;	pointer to caller's buffer for the field name
**			must be at least size + 1 bytes long.
**	i4  size;	size of buffer
**
** Returns:
**
** History:
**	3/20/89	(tom) created
*/
VOID
IIVQcsCenterString(text, str, size)
char *text;
char *str;
i4   size;
{
	f_strctr(text, STlength(text), str, size);
	str[size] = EOS;
	
}
