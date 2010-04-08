/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>		 
#include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<ex.h>
#include	<fe.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"il.h"

/**
** Name:	ilmain.c -	4GL Interpreter Main Entry Point Routine.
**
** Description:
**	Contains main routine for the interpreter.  Defines:
**
**	main()	4gl interpreter main entry point.
**
** History:
**	Revision 6.2  89/01  bobm
**	Initial revision.
**
**	30-aug-91 (leighb) Revived ImageBld: load default args.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      29-Sep-2004 (drivi01)
**          Updated NEEDLIBS to build iiinterp dynamically on both
**          windows and unix.
*/

# ifdef MSDOS
int	_stack = 25000;
# define main compatmain
# endif

# ifdef DGC_AOS
# define main IIITrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM = iiinterp

NEEDLIBS =   SHINTERPLIB SHFRAMELIB SHQLIB SHCOMPATLIB SHEMBEDLIB 

UNDEFS =	II_copyright
*/

GLOBALDEF bool IIabVision = FALSE;	/* Resolve reference in IAOM */

/*{
** Name:	main() -	4GL Interpreter Main Entry Point.
**
** Description:
**	This is really just a wrapper to call IIOtop, which is the
**	real interpreter main.  This main is used to build the generic
**	interpreter executable with no host language procedures.  When
**	an application has host language procedures, abf will link
**	an interpreter for the application.  It does so by generating
**	a main which contains the appropriate HL proc table, and calls
**	IIOtop.  This file provides an empty HL proc table.
**
** History:
**	1/89 (bobm) written
**      10/90 (stevet) Added IIUGinit call to initialize character set 
**            attribute table.
*/

main(argc, argv)
int	argc;
char	**argv;
{
	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES allocator.  User code will be linked as a separate
	** instance of the interpreter using "abfimain.c" as the main entry
	** point.  By default it will use the user's allocator.
	*/
	MEadvise( ME_INGRES_ALLOC );

	IIARload_defargs(&argc, &argv);

	/* Call IIUGinit to initialize character set attribute table. */
	if ( IIUGinit() != OK)
	{
	    PCexit(FAIL);
	}

	IIOtop( argc, argv, (ABRTSFUNCS *)NULL );
}
