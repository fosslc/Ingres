/*
**	static	char	Sccsid[] = "@(#)vfrbf.c 1.3	2/6/85";
*/

/*
** vfrbf.c
**
** Copyright (c) 2004 Ingres Corporation
**
** The RBF startup routines
**
** History
**	written 4/7/82 (jrc)
**	06/24/86 (a.dea) -- Changed ifdef CMS to FT3270.
**	08/14/87 (dkh) - ER changes.
**	12/26/89 (dkh) - VMS shared lib changes.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<si.h>
# include	<er.h>

extern	i4	(*ExitFn)();

GLOBALREF	FT_TERMINFO	IIVFterminfo;

/*
** Static state variables which tell how I was previously
** called.
*/

# ifdef FORRBF
static bool	vfprevcall = FALSE;
static bool	vffrmdone = FALSE;

extern	FILE	*vfdfile;
# endif


# ifdef FORRBF

static i4
vfrbfexit()
{
	PCexit(0);
}


vfrbf()
{
	FTclear();
	scrmsg(ERx("     "));
	ExitFn = vfrbfexit;
#ifdef FT3270
	/*
	**  When running VIFRED the FTspace routine will return a value
	**  of 1 to tell VIFRED that each field must be assumed to
	**  be 1 byte longer to make room on the screen for attribute
	**  bytes. Under RBF we want to allow fields to touch, since
	**  there are no attribute bytes on printers. (scl)
	*/
	FTsetspace(0);
#endif
	vfcursor(!vffrmdone, FALSE);
#ifdef FT3270
	/*
	**  Set it back
	*/
	FTsetspace(1);
#endif
	return(!noChanges);
}

VOID
vfrbfinit(frm)
FRAME	*frm;
{
	extern	i2	Cs_tag;
	extern	i2	Rbfrm_tag;

	/*
	**  Set up dump file for RBF testing.  This hack
	**  needed due to FT/VT interface. (dkh)
	*/

	FDvfrbfdmp(&vfdfile);

	FTterminfo(&IIVFterminfo);

	vflinetag = Cs_tag;
	vffrmtag = Rbfrm_tag;
	frame = frm;
	buildLines(frame, &Sections, Cs_top, Cs_length);
	frame->frmode = fdcmINSRT;
	vffrmdone = TRUE;
	vfprevcall = TRUE;
}
# endif
