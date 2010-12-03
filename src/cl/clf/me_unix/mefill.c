/*
** Copyright (c) 1983, 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>

# ifdef MEfill
# undef MEfill
# undef MEfilllng
# endif

/*
**
**
**	Name:
**		MEfill.c
**
**	Function:
**		MEfill
**
**	Arguments:
**		u_i2		n;
**		unsigned char 	val;
**		char		*mem;
**
**	Result:
**		Fill 'n' bytes starting at 'mem' with the unsigned 
**		character 'val'.
**	Side Effects:
**		None
**
**	History:
**		03/83 - (gb)
**			written
**		12/06/83 - (tpc)
**			Added VAX, DEFAULT ifdefs as part of the
**			integration of the performance version into the CL.
**
**		12/15/83 - Added idiot-proofing to VAX routine. (sat)
**
**		Revision 1.2  86/04/24  18:01:26  daveb
**		Don't use assembler on System V Vax --
**		the optimizer eats it.
**		Remove comment cruft
**	03-May-89 (GordonW)
**		remove the test for a NULL pointer since the CL
**		spec does not specify any behavior, big over-head.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <compat.h>";
**		If remapped in <me.h>, then this needs to be IIMEfill_func
**		so it can be called out of the macro;
**		If better, replace guts with bfill, bzero, or memset.
**	25-sep-1991 (mikem) integrated following change: 12-sep-91 (vijay)
**		Don't  define MEfill to II_MEfill_func if MEfill is "WRAPPED".
**		We want the MEfill routine to stay for those .c files which
**		did not include me.h.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-1993 (pearl)
**          	Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_DONT_SHIP_PROTOS
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-Dec-94 (GordonW)
**		comment after a endif
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	14-sep-1995 (sweeney)
**	    Remove obsolete assembler.
**	06-feb-1998 (canor01)
**	    Add MEfilllng().
**	04-mar-1998 (canor01)
**	    Fix typo in previous submission-i4 and u_i2 were swapped.
**	12-Jun-2001 (hanje04)
**	    Reintroduced MEfill, SIftell etc. as they are required by
**	    OpenRoad 4.0
**	16-jul-2001 (somsa01)
**	    Added inclusion of clconfig.h, and added missing function
**	    terminator.
**	15-nov-2010 (stephenb)
**	    Proto forward funcs.
*/


/*
** Forward references
*/
VOID
MEfill(
	register SIZE_TYPE	n,
	register unsigned char 	val,
	register char		*mem);
VOID
MEfilllng(
	register i4		n,
	register unsigned char 	val,
	register char		*mem);

/*ARGSUSED*/
VOID
MEfill(
	register SIZE_TYPE	n,
	register unsigned char 	val,
	register char		*mem)
{

	memset( mem, (int)val, n );
}

VOID
MEfilllng(
	register i4		n,
	register unsigned char 	val,
	register char		*mem)
{
    u_i2	fillbytes;

    while ( n )
    {
	fillbytes = (u_i2) (n > MAXI2) ? MAXI2 : n;
	n -= fillbytes;
	MEfill( fillbytes, val, mem );
    }
}
