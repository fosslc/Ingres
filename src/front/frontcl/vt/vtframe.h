/*
**  VTframe.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  static	char	Sccsid[] = "@(#)VTframe.h	30.1	12/7/84";
**	03/04/87 (dkh) - Added support for ADTs.
**	24-sep-96 (mcgem01)
**		externs changed to GLOBALREF's for global data project.
**	18-dec-96 (chech02)
**		changed VTgloby, VTglobx to GLOBALREF for redefined symbols.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# ifdef FTINTRN

# undef FTINTRN

# endif

# define	FTINTRN		WINDOW

# include	<termdr.h> 
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 
# include	<vt.h> 

GLOBALREF	i4	VTgloby;
GLOBALREF	i4	VTglobx;

GLOBALREF	i2	froperation[];
GLOBALREF	i1	fropflg[];
