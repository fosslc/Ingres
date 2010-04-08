/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<ci.h>
# include	<er.h>
# include	<ex.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"erca.h"

/**
** Name:	camain.c -	CopyApp Main Entry Point.
**
** Description:
**	Defines
**		main--calls copyappl, the true main routine
**
** History:
**
**	Revision 2.0  83/11/15  arthur
**	Written.
**	2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**	Revision 6.0  88/03/16  dave
**	03/16/88 (dkh) - Added EXdelete call to conform to coding spec.
**	31-oct-89 (mgw)
**		Added RUNTIMELIB to the ming hints to resolve references
**		to IIFRgpsetio, IIFRgpcontrol, and IIprmptio in iaom!iammeth.c
**		(derived from iammeth.qsc).
**      01-oct-90 (stevet)
**              Added IIUGinit() call to initialize character set attribute
**              table.
**	26-march-91 (blaise)
**		Allow copyapp to run with Vision-only capabilities.
**	28-mar-91 (sandyd)
**		Previous change needed <er.h>.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	24-mar-1994 (daver)
**		Remove obsolete ABFTO60LIB from NEEDLIBS.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	18-jun-97 (hayke02)
**	    Add dummy calls to force inclusion of libruntime.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	   Updated NEEDLIBS to link copyapp dynamically on both
**	   unix and windows.	    
*/

# ifdef DGC_AOS
# define main IICArsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM =	copyapp

NEEDLIBS =	COPYAPPLIB CAUTILLIB COPYUTILLIB \
		COPYFORMLIB SHINTERPLIB SHFRAMELIB \
		SHQLIB SHCOMPATLIB SHEMBEDLIB

UNDEFS = II_copyright
*/

GLOBALREF bool IIabVision ;	/* resolve references in iaom */

main(argc, argv)
i4	argc;
char	*argv[];
{
	EX_CONTEXT	context;
	STATUS          retstat;        /* Return status */

	EX	FEhandler();	/* interrupt handler for front-ends */

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

	/* Add IIUGinit call to initialize character set attribute table */

	if ( IIUGinit() != OK)
	{
	        PCexit(FAIL);
	}

	if ( EXdeclare(FEhandler, &context) != OK )
	{
		EXdelete();
		IIUGerr(E_CA0005_EXCEPTION, 0, 0);
		PCexit(FAIL);
	}
	copyappl(argc, argv);
	EXdelete();
	PCexit( OK );
}

/* this function should not be called. Its purpose is to    */
/* force some linkers that can't handle circular references */
/* between libraries to include these functions so they     */
/* won't show up as undefined                               */
dummy1_to_force_inclusion()
{
    IItscroll();
    IItbsmode();
}
