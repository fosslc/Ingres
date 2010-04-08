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
**      10/90 (stevet) Added IIUGinit call to initialize character set 
**                     attribute table.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	IIDDcuCleanUp -
**
** Description:
**	Perform cleanup.  This is the "real" main entry point, invoked by
**	several covers.
**
** Side Effects:
**	which 		which module to cleanup
**	arg,argv	command line parameters
**
** History:
**	4/90 (bobm) written.
**      10/90 (stevet) Added IIUGinit call to initialize character set 
**                     attribute table.
*/

STATUS
IIDDcuCleanUp(which,argc, argv)
u_i4	which;
i4	argc;
char	**argv;
{
	i4 junk;
	STATUS stat;

	/* Call IIUGinit to initialize character set attribute table */
	if ( IIUGinit() != OK)
	{
	        PCexit(FAIL);
	}

	if ((stat = IIDDdbcDataBaseConnect(argc,argv)) != OK)
		return stat;

	/*
	** if IIUIdgDeleteGarbage fails, we do IIUInslNoSessionLocks()
	** anyway.  Return a failure status if anything failed.
	** IIUIdgDeleteGarbage does this with its own internal queries,
	** also.  The general idea is that we try all of the parts of
	** cleanup, whether or not some of the independent pieces
	** failed.
	*/
	stat = IIUIdgDeleteGarbage(which,FALSE,&junk);

	if ((which & (DMB_APPDEV1|DMB_APPDEV3)) != 0)
	{
		if (stat == OK)
			stat = IIUInslNoSessionLocks();
		else
			_VOID_ IIUInslNoSessionLocks();
	}

	FEing_exit();

	return stat;
}
