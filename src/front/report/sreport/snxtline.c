
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h>
# include	<sglob.h>
# include	<rglob.h>

/**
** Name:	snxtline.c	contains the routine s_next_line which
**				gets the next line from the source report file.
**
** Description:
**	This file defines:
**
**	s_next_line	gets the next line from the source report file.
**
** History:
**	3-apr-87 (grant)	created.
**              9/22/89 (elein) UG changes
**                      ingresug change #90025
**			fix for fix for 4675
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	s_next_line	- gets the next line from the source report file
**
** Description:
**	This routine gets the next line from the source report file. It returns
**	the number of characters in the line and the global variable Tokchar
**	is set to point to the beginning of the line buffer.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		The number of characters in the line. If it is zero, this
**		implies the end of the file.
**
** Side Effects:
**	The variable Line_buf is filled with the next line.
**	Tokchar points to the beginning of Line_buf.
**	Line_num is incremented.
**
** History:
**	3-apr-87 (grant)	implemented.
**	17-feb-1989 (danielt)
**		fix for bug 4675.  If EOF, set *Tokchar = EOS (the
**		state machine breaks down if there is a comment on the
**		last line of the .rw file.
*/

i4
s_next_line()
{
    i4  count;

    count = r_readln(Fl_input, Line_buf, MAXLINE);
    Line_num++;
    if (count != 0)
	r_g_set(Line_buf);
    else if (Tokchar != NULL)
	/* fix for bug 4675 */
	*Tokchar = EOS;
    return count;
}
