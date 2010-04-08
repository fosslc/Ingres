/*
**	Copyright (c) 2004 Ingres Corporation
**	All right reserved.
*/

#include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
#include	<ci.h>
#include	<me.h>
# include	<ex.h>

/**
** Name:    oslsmain.c -	OSL/SQL Interpreted Frame Object Translator
**					Main Program Entry Point Routine.
** Description:
**	Defines 'main()' for program entry to the OSL/SQL interpreted frame
**	object translator ("osl").
**
** History:
**	Revision 6.0  87/03/17  wong
**	Copied from "quel/oslqmain.c".
**      Revision 6.3/03/00  90/10/2   stevet
**      Added IIUGinit call to initialize character set attribute table.
**
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
**	    Updated NEEDLIBS to link dynamic library SHQLIB, SHFRAMELIB
**	    and SHINTERPLIB to replace
**	    its static libraries. Added NEEDLIBSW hint which is used
**	    for compiling on windows and compliments NEEDLIBS.
*/

# ifdef MSDOS
int	_stack = 20000;

# define main	compatmain
# endif

# ifdef DGC_AOS
# define main IIOYrsmRingSevenMain
# endif

/*
** MKMFIN Hints:
**
PROGRAM		= oslsql
NEEDLIBS	= SQLLIB OSLLIB CODEGENLIB ILGLIB SHINTERPLIB \
			SHFRAMELIB SHQLIB SHCOMPATLIB 
NEEDLIBSW	= SHEMBEDLIB SHADFLIB 
UNDEFS =	II_copyright
*/

GLOBALDEF bool	IIabVision = FALSE;		/* Resolve references in IAOM */

main(argc, argv)
i4	argc;
char	**argv;
{
    i4      sql();
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
    PCexit(osl(sql, argc, argv));
}
