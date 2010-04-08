/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h> 
# include	<tm.h>
# include	<termdr.h> 
# include	<er.h> 

/**
** Name:	tdprnwin.c	- write window to file.
**
** Description:
**	This file defines:
**
**	TDprnwin	write a window to text file.
**
** History:
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	TDprnwin	- write a window to a file.
**
** Description:
** 	Write the window to a text file.  This writes it
**	in a format for printing to a printer, rather than the
**	format of 'TDdumpwin' which writes it in a debugging
**	format.  Also, this does not put the cursor position
**	in the file, as does TDdumpwin.
**
**	This also checks to see if graphics characters are being
**	output, in which case, they are translated into
**	standard line drawing characters.
**
** Inputs:
**	win	The window to write to the file.
**	file	The opened file to write to.
**
** Outputs:
**	Returns:
**		VOID.
**	Exceptions:
**		none.
**
** Side Effects:
**	output to named file.
**
** History:
**	07-aug-1985 (peter)	Stolen from TDdumpwin.
**	21-oct-1985 (peter)	Added time to output.
*/
VOID
TDprnwin(awin, afile)
WINDOW	*awin;
FILE	*afile;
{
	reg	WINDOW	*win = awin;
	reg	FILE	*file = afile;
	reg	char	**line;
	reg	char	**fonts;	/* Maps to graphic bits */
    	reg	char	*cp;
	reg	char	*fp;		/* Points to graphic chars */
	reg	char	curchar;	/* Current character */
	reg	i4	y;
    	reg	i4	x;
	reg	i4	ey;
	reg	i4	ex;
	SYSTIME		stime;		/* System time */
	char		timestr[101];	/* Hold string for time */
	i4		len;		/* length of time string */

	if (awin == NULL || afile == NULL)
	{
		return;
	}
	ex = win->_begx + win->_maxx;
	ey = win->_begy + win->_maxy;

	/* First put in the system time */
	TMnow(&stime);
	TMstr(&stime, timestr);
	len = STtrmwhite(timestr);
	SIfprintf(file, ERx("___ %s __"), timestr);	
	for (x = win->_begx + len + 7; x < ex; x++)
	{
		SIputc('_', file);	
	}
	SIputc('\n', file);
	for (y = win->_begy, line = win->_y, fonts = win->_da;
		 y < ey; y++, line++, fonts++)
	{
		for (x = win->_begx, cp = *line, fp = *fonts; 
			x < ex; x++, cp++, fp++)
		{
			curchar = *cp;
			/* If a graphic character, translate */
			if (_LINEDRAW & *fp)
			{	/* It is a graphic character */
				if ((curchar == *QC) ||
				    (curchar == *QJ) ||
				    (curchar == *QB) ||
				    (curchar == *QE) ||
				    (curchar == *QA) ||
				    (curchar == *QD) ||
				    (curchar == *QI))
				{
					curchar = '+';
				}
				if ((curchar == *QF))
				{
					curchar = '-';
				}
				if ((curchar == *QK) ||
				    (curchar == *QH) ||
				    (curchar == *QG))
				{
					curchar = '|';
				}
			}
			SIputc(curchar, file);	
		}
		SIputc('\n', file);	    
	}
	for (x = win->_begx; x < ex; x++)
	{
		SIputc('_', file);	
	}
	SIputc('\n', file);
}
