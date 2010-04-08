/*
**	Copyright (c) 1985, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
#include	<ci.h>
#include	<me.h>
# include	<ex.h>

/**
** Name:    oslqmain.c -	OSL/QUEL Interpreted Frame Object Translator
**					Main Program Entry Point Routine.
** Description:
**	Defines 'main()' for program entry to the OSL/QUEL interpreted frame
**	object translator ("osl").
**
** History:
**      Revision 6.3/03/00  90/10  stevet
**      Added IIUGinit call to initialize character set attribute table.
**	Revision 6.0  87/06  wong
**	Simplified for 6.0.
**
**	Revision 5.1  86/12/04  17:04:10  wong
**	Added hint for UNDEF of 'kwtab'.  Modified to pass parser, etc., to
**	'osl()', as parameters.  Modified from "osl/main.c".
**
**	Revision 3.0
**	Written (jrc)
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic libraries SHQLIB, SHINTERPLIB
**	    and SHFRAMELIB to replace its static libraries. Added NEEDLIBSW 
**	    hint which is used for compiling on windows and compliments NEEDLIBS.
*/

# ifdef MSDOS
int	_stack = 20000;

# define main	compatmain
# endif

/*
** MKMFIN Hints:
**
PROGRAM		= osl
NEEDLIBS	= QUELLIB OSLLIB CODEGENLIB ILGLIB SHINTERPLIB 
			SHFRAMELIB SHQLIB SHCOMPATLIB ;
NEEDLIBSW	= SHADFLIB SHEMBEDLIB ;
UNDEFS		= II_copyright
*/

GLOBALDEF bool IIabVision = FALSE;	/* Resolve references in IAOM */

main(argc, argv)
i4	argc;
char	**argv;
{
    i4      quel();
    STATUS  osl();

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC );	/* use ME allocation */

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
    {
	PCexit(FAIL);
    }

    /* Translate */
    PCexit(osl(quel, argc, argv));
}
