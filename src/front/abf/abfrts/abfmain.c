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
#include	<adf.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abfcnsts.h>

/**
** Name:    abfmain.c - ABF Application Image Entry Point Routine.
**
** Description:
**	Entry point for an ABF image.  Depends on the values of certain
**	run-time variables.  Defines:
**
**	main()	ABF application image entry point.
**
** History:
**	Revision  6.0  88/01  wong
**	Extracted main-line into "abfrt/abrtmain.c" leaving entry point, here.
**
**      Revision  6.3/03/00  90/10  stevet
**      Add IIUGinit call to initialize character set attribute table.
**
**	30-aug-91 (leighb) Revived ImageBld: load default args.
**      25-Jul-95 (fanra01) Added define of ii_user_main for NT systems
**      26-Sep-95 (fanra01)
**          Removed redirection to ii_user_main.  No longer required with
**          console based tools.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	18-dec-96 (hanch04)
**	    Moved global back for bug 79658
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** 2.0 OSL Compatibility:  C code produced by the 2.0 OSL compiler
** had direct references to 'Abrret'.  These are defined here and
** passed to the run-time system, which may be a shared library.
*/
GLOBALDEF i4	Abrret ;
GLOBALDEF i4	cAbrret ;	/* 2.0 WSC compatibility */

/**/ main (argc, argv)
i4	argc;
char	**argv;
{
	extern ABRTSOBJ	ABEXTCNAME;

	IIARload_defargs(&argc, &argv);

	/* Call IIUGinit to initial character set attribute table */
	if ( IIUGinit() != OK)
	{  
	  PCexit(FAIL);
	}

	IIARmain(argc, argv, &ABEXTCNAME, &Abrret, &cAbrret);
}
