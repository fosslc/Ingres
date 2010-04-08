/*
**  Copyright (c) 2004 Ingres Corporation
**
**  DUMPWIN.c
**
**  Dump the window to the file
**  the cursor position is marked with an underscore
**
**  DEFINES
**
**	TDdumpwin(win, file, cursor)
**		WINDOW	*win		The window to dump
**		FILE	*file		The file to write to
**		bool	cursor
**
**  HISTORY
**	written	 3/9/82	 (jrc)
**	02/26/86 (garyb) -- EBCDIC support for character displayed after
**		 cursor.
**	08/14/87 (dkh) - ER changes.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**      26-Sep-95 (fanra01)
**          Added a flush of write buffer for SEP since dumping one screen
**          overflows the write buffer in TC and causes a forced write to disk.
**          The remaining output would then require further data in the buffer
**          to flush to disk.
**	13-Apr-2005 (hweho01)
**	    Avoid compiler error on AIX, not use the old style  
**          parameter list for dmpchr() routine.
**	19-Apr-2005 (schka24)
**	    Have to forward the routine declaration properly if you're going
**	    to use an ANSI declaration later on.
**  20-Sep-2005 (fanra01)
**      Relocate the copyright message.
**      Add return type to the TDdumpwin function.
*/

/*
**	static	char	*Sccsid = "%W%	%G%";
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	<termdr.h>

#ifndef EBCDIC
#define ENDCURSOR 0x08
#else
#define ENDCURSOR 0x16
#endif

#ifdef SEP

#include <tc.h>

GLOBALREF   TCFILE  *IIFTcommfile;

#endif /* SEP */

/* Forward procedure declarations */
static VOID dmpchr(char achar, FILE *file);


VOID
TDdumpwin(awin, afile, cursor)
WINDOW	*awin;
FILE	*afile;
bool	cursor;
{
	reg	WINDOW	*win = awin;
	reg	FILE	*file = afile;
	reg	char	**line;
    	reg	char	*cp;
	reg	i4	y;
    	reg	i4	x;
	reg	i4	ey;
	reg	i4	ex;
		char	buffer[5];



	dmpchr('#',file);
	dmpchr('#',file);
	dmpchr('\n',file);

	ex = win->_begx + win->_maxx + 2;
	ey = win->_begy + win->_maxy;
	for (x = win->_begx; x < ex; x++)
	{
		dmpchr('_', file);
	}
	dmpchr('\n', file);
	ex -= 2;
	for (y = win->_begy, line = win->_y; y < ey; y++, line++)
	{
		dmpchr('|', file);
		for (x = win->_begx, cp = *line; x < ex; x++, cp++)
		{
			if (cursor && y == win->_cury && x == win->_curx)
			{
				STprintf(buffer, ERx("_%c"), ENDCURSOR);
				dmpchr(buffer[0],file);
				dmpchr(buffer[1],file);
			}
			dmpchr(*cp, file);
		}
		dmpchr('|', file);
		dmpchr('\n', file);
	}
	ex += 2;
	for (x = win->_begx; x < ex; x++)
	{
		dmpchr('_', file);
	}
	dmpchr('\n', file);
	dmpchr('@', file);
	dmpchr('@', file);
	dmpchr('\n', file);
# if defined(NT_GENERIC) && defined(SEP)
        TCflush(IIFTcommfile);
# endif             /* NT_GENERIC && SEP */
}


static VOID
dmpchr(char achar, FILE *file)
{

#ifdef SEP

	if (IIFTcommfile != NULL)
	    TCputc(achar,IIFTcommfile);
	else

#endif /* SEP */

	    SIputc(achar,file);

}
