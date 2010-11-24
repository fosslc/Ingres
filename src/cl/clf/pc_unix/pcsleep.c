# include	<compat.h>
# include	<gl.h>
# include	<pc.h>

/*
 *	Copyright (c) 1983, 2001 Ingres Corporation
 *
 *	Name:
 *		PCsleep.c
 *
 *	Function:
 *		PCsleep
 *
 *	Arguments:
 *		i4	msec;
 *
 *	Result:
 *		Cause the calling process to sleep for msec milliseconds.
 *			Resolution is rounded up to the nearest second.
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	13-Mar-2001 (hanje04)
**	    PCsleep will now use usleep() instead of sleep() so that it
**	    is possible to sleep for < 1sec. 
**	19-Mar-2001 (hanje04)
**	    On some plaforms usleep will ONLY accept values of < 1sec.
**	    will use sleep if msec => 1000.
**	15-nov-2010 (stephenb)
**	    Inlcude pc.h for prototypes.
 */

VOID
PCsleep(msec)
u_i4	msec;
{
	if ( msec < 1000 )
	    usleep((u_long)(1000*msec));
	else
	    sleep( (unsigned)((msec + 999)/1000) );
}
