/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<fdesc.h>
#include	<abfrts.h>
# include	<ex.h>

/**
** Name:	abfimain.c - ABF Interpreter Entry Point Routine.
**
** Description:
**	Entry point for an ABF interpreter executable.
**
** History:
**	Revision 6.2  89/02  bobm
**	Initial revision.
**
**      Revision 6.3/03/00 90/10   stevet
**      Added IIUGinit call to initialize character set attribute table.
**
**	30-aug-91 (leighb) Revived ImageBld: load default args.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	17-Aug-95 (fanra01)
**	    Added redirection to ii_user_main.
**      26-Sep-95 (fanra01)
**          Removed redirection to ii_user_main.  No longer required with
**          console based tools.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF ABRTSOBJ	IIOhl_proc[];

/**/ main (argc, argv)
i4	argc;
char	**argv;
{
	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Note:  By default we use the user's memory allocator as we should.
	**
	** MEadvise( ME_USER_ALLOC );
	*/

	IIARload_defargs(&argc, &argv);

        if ( IIUGinit() != OK)
	{
	    PCexit(FAIL);
	}

	IIOtop(argc, argv, IIOhl_proc);
}
