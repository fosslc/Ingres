/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<lo.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<qg.h>
# include	<mqtypes.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<trimtxt.h>
# include	<lqgdata.h>
# include 	<erfg.h>
# include 	"framegen.h"

/**
** Name:	fgdefmnu.c	-	Generate default menu form 
**
** Description:
**	This file defines:
**
**	IIFGmm_makeMenu		Create default menu form
**	IIFGflcFmtLcCols	Format ListChoices-style menu columns
**
** History:
**	6/8/89 (Mike S)	Initial version
**	7/6/89 (Mike S) Always call IIFDcfCloseFrame
**	7/17/89 (Mike S) Save form to database (interface change)
**	8/30/89 (Mike S) Improve default form appearance
**	12/19/89 (dkh) - VMS shared lib changes.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	3/91 (Mike S) - Make ListChoices-style menu
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/
/* # define's */
# define MINWIDTH	80		/* Minimum width of form produced */
# define ITEMSTART	0		/* Starting column for menuitem */
# define ITEMWIDTH	15		/* Width of menuitem displayed */
# define DESCSTART	20		/* Starting column for description */
# define DESCWIDTH	60		/* Width of description displayed */
# define GROUPSIZE	3		/* Group menuitmes in threes */
# define DEFWIDTH	80		/* Default form width */

/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN	FRAME 	*IIFDofOpenFrame();
FUNC_EXTERN	bool	IIFDcdCloseFrame();
FUNC_EXTERN	TBLFLD	*IIFDatfAddTblFld();
FUNC_EXTERN	VOID	FTterminfo();
FUNC_EXTERN	STATUS	IIFGflcFmtLcCols();

/* static's */
static const char command_column[] 	= ERx("command");
static const char expl_column[] 	= ERx("explanation");
static DB_DATA_VALUE command_dbv = {NULL, FE_MAXNAME, DB_CHA_TYPE, 0};
static DB_DATA_VALUE expl_dbv =    {NULL, DESCWIDTH, DB_CHA_TYPE, 0};

/*{
** Name:	IIFGmm_makeMenu	- 	Create default menu form.
**
** Description:
**	Create a menu form from a menu-type metaframe.  We use one of two
**	layouts:	
**
**	Classic:
**
**	Line 0		Title (short remarks or frame name) + frame type
**	Line 1		(blank)
**	Line 2-4	Menuitems 1-3
**	Line 5		(blank)
**	Line 6-8	Menuitems 4-6
**	Line 9		(blank)
**	Line 10-12	Menuitems 7-9
**	Line 13		(blank)
**	Line 14-16	Menuitems 10-12
**	Line 17		(blank)
**	Line 18-20	Menuitems 13-15
**
**	If we're going to leave a 'widow' in the last group, the
**	first group is extended to four items.
**
**	There's currently a limt of 15 menuitems; if this is extended, we'll
**	continue the pattern.
**
**	Each menuitem has the following basic layout:
**	Columns 0-14	Menuitem name (from metaframe structure)
**	Columns 20-79	Short remarks for menuitem's frame
**	If the longest row is short enough to be centered, all the menuitems
**	will be moved right to center the longest row.
**
**	Short remarks are currently limited to 60 chars; we'll truncate them at
**	60 chars to be safe.
**
** 	ListChoices:
**	Line 0		Title (short remarks or frame name) + frame type
**	Line 1		(blank)
**	Lines 2-n	Tablefield.  
**			Column 1: command.  c15, scrolling to FE_MAXNAME
**			Column 2: explanation.  c60
** Inputs:
**	metaframe	METAFRAME *	Visual Query's metaframe
**
** Outputs:
**	none
**
**	Returns:
**			STATUS		OK if form was saved to database
**					FAIL otherwise
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	6/9/89	(Mike S)	Initial version
**	7/27/89	(Mike S)	Center text.
*/
STATUS
IIFGmm_makeMenu( metaframe )
METAFRAME *metaframe;
{
	i4	lineno = 0;	/* Current line on form */
	i4	item;		/* Current menuitem number */
	bool	lcs;		/* ListChoice style */

	FRAME	*frame;		/* Frame to be produced */
	char	buffer[255];	/* text buffer */
	char	*trim;		/* Trim string pointer */
	USER_FRAME *apobj = (USER_FRAME *)metaframe->apobj;
				/* Application object from metaframe */
	TRIMTXT	*trimtxt;	/* Title trim pointer */
	char	*desc;		/* description text for sub-frame */
	bool	opened = FALSE;	/* TRUE if dynamic frame is opne */
	i4	status;		/* Form save status */
	i4	maxlength;	/* Length of longest row */
	i4	curlength;	/* Length of current row */
	i4	rowstart;	/* Adjustment needed for centering */
	bool	widow;		/* Will the default algorithm leave a widow */
	i4	term_cols;	/* Terminal width */

	/* Determine menu style */
	lcs = (apobj->flags & APC_MS_MASK) == APC_MSTAB;

	/* If the form system is active, use current width */
	if (IILQgdata()->form_on)
	{
		FT_TERMINFO	terminfo;

		FTterminfo(&terminfo);
		term_cols = terminfo.cols;
	}
	else
	{
		term_cols = DEFWIDTH;
	}

	/* Allocate frame strcture */
	frame = IIFDofOpenFrame(apobj->form->name);
	if (frame == NULL)
		goto nomem;
	opened = TRUE;

	/* Add title line(s) */
	if (IIFGfftFormatFrameTitle(metaframe, term_cols, &trimtxt) == OK)
	{
		for (; trimtxt != NULL; trimtxt = trimtxt->next) 
		{
			if (IIFDattAddTextTrim(
				frame, trimtxt->row, trimtxt->column, 
				(i4)0, trimtxt->string) 
			    == NULL)	
				goto nomem;
			lineno = max(lineno, trimtxt->row);
		}
		lineno++;
	}

	if (!lcs)
	{
		/* Get maximum length of item lines */
		for (item = 0, maxlength=0; item < metaframe->nummenus; item++)
		{
			/* First, get the length of the menuitem */
			curlength = ITEMSTART + 
				min(STlength(metaframe->menus[item]->text), 
					ITEMWIDTH);

			/* Now figure in the remarks field */
			desc = ((USER_FRAME *)metaframe->menus[item]->apobj)->short_remark;
			if ((desc != NULL) && (*desc != EOS))
				curlength = DESCSTART + 
					min(STlength(desc), DESCWIDTH);

			maxlength = max(maxlength, curlength);
		}

		/* Calculate where to start each row */
		rowstart = max((term_cols - maxlength) / 2, 0);
		
		/* Add all menuitem lines */
		widow = (metaframe->nummenus % GROUPSIZE) == 1;
		for (item = 0; item < metaframe->nummenus; item++)
		{

			if (widow)
			{
				/* Skip a line before the 1st, 5th, 8th, etc.*/
				if (item == 0)
					lineno++;
				else if (((item != 1) && (item%GROUPSIZE) == 1))
					lineno++;
			}
			else
			{
				/* Skip a line before the 1st, 4th, 7th etc. */
				if ( (item % GROUPSIZE) == 0)	
					lineno++;
			}

			/* If menuitem name is too long, 
			** copy a truncated version 
			*/
			if (STlength(metaframe->menus[item]->text) > ITEMWIDTH)
			{
				STlcopy(metaframe->menus[item]->text, buffer, 
					ITEMWIDTH);
				trim = buffer;
			}
			else
			{
				trim = metaframe->menus[item]->text;
			}

			/* Create the menuitem trim */
			if (IIFDattAddTextTrim(frame, lineno, 
					       ITEMSTART + rowstart, 
					       (i4)0, trim)
				== NULL)
				goto nomem;

			/* Create description trim ,if there's a description */
			desc = ((USER_FRAME *)metaframe->menus[item]->apobj)->short_remark;
			if ((desc != NULL) && (*desc != EOS))
			{
				/* 
				**	If the description is too long, 
				**	copy a truncated version 
				*/
				if (STlength(desc) > DESCWIDTH)
				{
					STlcopy(desc, buffer, DESCWIDTH);
					trim = buffer;
				}
				else
				{
					trim = desc;
				}
			
				/* Create the menuitem trim */
				if (IIFDattAddTextTrim(frame, lineno, 
						       rowstart + DESCSTART, 
						       (i4)0, trim) 
					== NULL)
					goto nomem;
			}
			lineno++;
		}
	}
	else
	{
		TBLFLD *iitf;
		FLDCOL *command_fc, *expl_fc;
		i4 nrows = max(metaframe->nummenus, 1);
		i4 tfwidth;
		i4 colno;

		/* Make table field */
		tfwidth = ITEMWIDTH + DESCWIDTH + 3;
		colno = (term_cols - tfwidth) / 2;
		iitf = IIFDatfAddTblFld(frame, TFNAME, ++lineno, colno, nrows,
					FALSE, FALSE);
		if (iitf == NULL)
			goto nomem;
		iitf->tfhdr.fhd2flags |= fdVQLOCK;

		/* Make columns */
		if (IIFGflcFmtLcCols(&command_fc, &expl_fc) != OK)
			goto nomem;
		if (IIFDaecAddExistCol(frame, iitf, command_fc) != OK)
			goto nomem;
		if (IIFDaecAddExistCol(frame, iitf, expl_fc) != OK)
			goto nomem;
	}

	/* Convert all the information we've accumulated into a FRAME */
	if (!IIFDcfCloseFrame(frame))
		goto nomem;
	opened = FALSE;

	/* All is well; save it to the database */
	status = IIFGscfSaveCompForm((USER_FRAME *)metaframe->apobj, frame);
	if (status == 0)
	{
		/* Save number of menuitems */
		metaframe->state &= ~MFST_MIMASK;
		metaframe->state |= 
			((metaframe->nummenus << MFST_MIOFFSET) & MFST_MIMASK);
		return(OK);
	}
	else
	{
		return(FAIL);
	}
	
	/* Come here if any allocation fails */
nomem:
	if (opened) IIFDcfCloseFrame(frame);
	IIUGerr(E_FG002F_Defmnu_NoDynMem, 0, 0);
	return (FAIL);
}

/*{
** Name:	IIFGflcFmtLcCols - Format columns for ListChoices-style menus
**
** Description:
**	A ListChoice-style menu has two columns in its tablefield:
**
**		command		The Menuitem text
**		explanation	The short explanation for the child frame
**
**	This routine creates the column structures with the correct format.
**	It's called both from form createion and form fixup.
**
** Inputs:
**
** Outputs:
**	cmd_col		FLDCOL *	Command column
**	expl_col	FLDCOL *	Explanation column
**
**	Returns:
**		STATUS
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	3/1 (Mike S) Initial version
*/
STATUS
IIFGflcFmtLcCols(cmd_col, expl_col)
FLDCOL 	**cmd_col;
FLDCOL	**expl_col;
{
	STATUS status;

	status = IIFDfcFormatColumn(command_column, 0, ERx(""), &command_dbv, 
				    ITEMWIDTH, cmd_col);
	if (status != OK)
		return status;
	(*cmd_col)->flhdr.fhd2flags |= fdVQLOCK;

	status = IIFDfcFormatColumn(expl_column, 0, ERx(""), &expl_dbv, 
				    0, expl_col);
	if (status != OK)
		return status;
	(*expl_col)->flhdr.fhd2flags |= fdVQLOCK;
	return OK;
}
