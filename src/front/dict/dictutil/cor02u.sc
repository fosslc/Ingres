/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<uigdata.h>

/**
** Name:	main.c -	Main Routine and Entry Point for module upgrade
**
** Description:
**	main		The main program.
**
** History:
**	27-sep-1990 (pete) written.
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
** Name:	main() -	Main Entry Point for CORE v. 1 upgrade.
**
** Description:
**	Upgrade module CORE from version 1 to 2.
**	This program is a NO-OP and DOES AN IMMEDIATE EXIT -- does
**	not even connect to database. Required because of our rule
**	that every module upgrade requires an executable.
**
** Side Effects:
**
** History:
**	27-sep-1990 (pete) written.
**	20-may-1991 (pete) Change module Description above.
*/

/*
**	MKMFIN Hints
PROGRAM =	cor02uin

NEEDLIBS =	DICTUTILLIB SHQLIB COMPATLIB SHEMBEDLIB 

UNDEFS =	II_copyright
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{
	PCexit(OK);
}
