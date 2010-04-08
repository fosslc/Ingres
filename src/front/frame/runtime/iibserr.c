/*
**	iibserr.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<erfi.h>
# include       <er.h>

/**
** Name:	iibserr.c	-	Box size error routine.
**
** Description:
**
**	check popup box sizes.
**
** History:
**	4/88 (bobm)	created
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIFRbserror -	Check boz size.
**
** Description:
**	checks size of popup box, and produces appropriate error
**	message if wrong.
**
** Inputs:
**	p - popup box
**	row - minimum rows.
**	col - minimum cols.
**	what - what is being popped up.
**
** Returns:
**	TRUE - an error was found.
**	FALSE - box OK.
**
** History:
**	4/88 (bobm)	written
*/

bool
IIFRbserror(POPINFO *p, i4 row, i4 col, char *what)
{
    if (p->begx < 0 || p->begy < 0)
    {
	IIFDerror(E_FI2261_PopStart,1,what);
	return(TRUE);
    }
    if ((p->maxy != 0 && p->maxy < row) || (p->maxx != 0 && p->maxx < col))
    {
	IIFDerror(E_FI2260_PopBox,3,what,&row,&col);
	return(TRUE);
    }

    return(FALSE);
}
