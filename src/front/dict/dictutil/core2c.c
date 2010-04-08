/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<dictutil.h>

/**
** Name:	main.c -	Main Routine and Entry Point for module cleanup
**
** Description:
**	main		The main program.
**
** History:
**	4/90 (bobm) written.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**/


/*{
** Name:	main() -	Main Entry Point for module cleanup.
**
** Description:
**	Perform cleanup.
**
** Side Effects:
**	Updates system catalogs.
**
** History:
**	4/90 (bobm) written.
*/

/*
**	MKMFIN Hints
PROGRAM =	cor02fin

NEEDLIBS =	DICTUTILLIB SHQLIB COMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{
	PCexit(IIDDcuCleanUp(DMB_CORE, argc, argv));
}
