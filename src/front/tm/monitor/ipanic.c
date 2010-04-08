# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
** ipanic.c -- TM Panic Routine
**
**	Description:
**		Now simply a cover routine for erstwhile TM "syserrs" which
**		now calls IIUGerr() to print message and die.
**
**	History:
**		11/84 (ATTIS) -- First written.
**
**		12/84 (jhw)   -- Added messages for when file not found
**			and for when message not found.
**
**		07/85 (gb)    -- Moved to iutil, letting individual syserr()'s
**			handle differences (would be nice to have one syserr,
**			but ...).  Zapped panfilen() use NMf_open().  Zapped
**			extra SYSERR printed out.  Check SIgetrec() status.
**
**		Revision 3.22.40.2  85/07/12  11:37:05	daveb
**		Updated 2.0 ipanic to 3.0
**
**		Revision 3.22.40.3  85/07/14  11:33:36	ingres
**		use syserr()
**		don't use version anymore
**
**		Revision 1.2  85/10/11	13:59:25  daveb
**		Don't use SIfprintf(stderr, "msg") since this might
**		be in a backend and need to use uprintf instead.
**		Send all output through the syserr() that happens
**		to be loaded.
**
**		Revision 1.3  86/03/14	11:36:10  garyb
**		Change (VOID) to _VOID_.
**
**		10/87 (peterk) - change to use ER MSGID's for error msgs.
**			and use IIUGerr() to print them.
**		29-Sep-2009 (frima01) 122490
**			Add return type for ipanic() to avoid gcc 4.3 warnings.
*/

VOID
ipanic(err_num, p1, p2, p3, p4, p5)
ER_MSGID	err_num;
char		*p1, *p2, *p3, *p4, *p5;
{
	IIUGerr(err_num, UG_ERR_FATAL, 5, p1, p2, p3, p4, p5);
}

