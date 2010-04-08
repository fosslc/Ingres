/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<eqlang.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<iisqlca.h>
# include	<iilibq.h>

/**
+*  Name: iilang.c - Change the notion of the current EQUEL language.
**
**  Description:
**	Handles changes to the current host language.
**
**  Defines:
**	IIlang
**
**  Notes:
**	None.
-*
**  History:
**	10-oct-1986	- Removed pre-6.0 globals (mrw)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIlang - Change the current source language for EQUEL.
**
**  Description:
**	The new language is checked for legality and then installed.
**
**  Inputs:
**	newlang		- The new language.
**
**  Outputs:
**	Returns:
**	    The previous host language.
**	Errors:
**	    None.
-*
**  Side Effects:
**	The current source language is changed.
**  History:
**	18-oct-1982 - Needed in ABF. (jrc)
**	10-oct-1986 - Removed pre-6.0 globals. Allowed the new languages. (mrw)
*/

i4
IIlang(newlang)
    i4		newlang;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		oldlang;

    oldlang = IIlbqcb->ii_lq_host;
    if (EQ_C <= newlang && newlang <= EQ_ADA)
	IIlbqcb->ii_lq_host = newlang;
    return (oldlang);
}
