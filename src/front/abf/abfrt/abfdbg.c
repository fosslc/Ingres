/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<si.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abrterr.h>

/**
** Name:	abfdbg.c -	ABF Run-Time FRS Debugging Setup/Close Module.
**
** Description:
**	Contains all routines that set-up and close down the FRS testing.
**	Defines:
**
**	IIARidbgInit()
**	IIARcdbgClose()
**
**	IIarTest	testing files (-Z)
**	IIarDebug	debug output file (-D)
**	IIarDump	keystroke output file (-I)
**
** History:
**	Revision  6.0  88/01  wong
**	Moved global test variables, here.
**
**	Revision 2.0  82/09  joe
**	Initial revision.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
*/

/*
** Name:	ABFRT Debugging Files
**
** Description:
**	FRS debugging files to be passed through 'UTexe()' (called from
**	'abexeprog()'.
*/
GLOBALREF char	*IIarTest ;	/* Names for testing files (-Z) */
GLOBALREF char	*IIarDebug ;	/* Name of debug output file (-D) */
GLOBALREF char	*IIarDump ;	/* Name of keystroke output file (-I) */

static FILE	*abDbgfile = NULL;

/*{
** Name:	IIARidbgInit() -	ABF Run-Time FRS Debugging Initialization.
**
** Description:
**	Initialize the debugging routines for the Forms system.
**	Opens the test files if needed.
**
** Input:
**	name	{char *}  The name of the application.
**	dmp	{char *}  Name of the keystroke file for output.
**	dbg	{char *}  Name of dump file for output.
**	test	{char *}  Name of testing files.
**
** Called by:
**	'abf()', 'IIARmain()'.
**
** Side Effects:
**	Sets 'IIarDump', 'IIarDebug' and 'IIarTest' (for use by 'abexeprog()'.)
**
** Error Messages:
**	DBGFILE
**
** History:
**	Written 9/16/82 (jrc)
**	12/16/85 (joe)
**		Added 3 arguments to initialize IIarDump, IIarDebug and IIarTest
**		This facilitates shared libraries.
*/
VOID
IIARidbgInit(name, dmp, dbg, test)
char	*name;
char	*dmp;
char	*dbg;
char	*test;
{
	LOCATION	file;
	char		buf[MAX_LOC+1];

	IIarDump = dmp;
	IIarDebug = dbg;
	IIarTest = test;

	if (IIarDump != NULL)
	{ /* keystroke output */
		FDrcvalloc(IIarDump);
	}
	if (IIarDebug != NULL)
	{ /* debug output file */
		STcopy(IIarDebug, buf);
		LOfroms(FILENAME, buf, &file);
		if (SIopen(&file, ERx("w"), &abDbgfile))
			abproerr(ERx("abdbginit"), DBGFILE,NULL);
		FDdmpinit(abDbgfile);
	}
	if (IIarTest != NULL)
		FEtest(IIarTest);
}

/*{
** Name:	abdbgnterp	-	Set up test constants for interp.
**
** Description:
**	This is called by the interpreter when it is being run from ABF.
**	It sets the ABF global variables for testing, but doesn't
**	open the test files.
**
** Inputs:
**	name		The name of the application.
**
**	dmp		The name of the dump file. Given with
**			the -I flag.
**
**	dbg		The name of the debug file. Given with
**			the -D flag.
**
**	test		The test flags. Given with the -Z flag.
**
** History
**	3-dec-1986 (Joe)
**		First Written
*/
VOID
abdbgnterp (name, dmp, dbg, test)
char	*name;
char	*dmp;
char	*dbg;
char	*test;
{
	IIarDump = dmp;
	IIarDebug = dbg;
	IIarTest = test;
}

/*{
** Name:	IIARcdbgClose() -	ABF Run-Time FRS Debugging Close.
**
** Description:
**	Close any files associated with the FRS testing or debugging.
**
** Called by:
**	'IIabf()', 'IIARmain()'.
**
** History:
**		Written 9/16/82 (jrc)
*/
VOID
IIARcdbgClose()
{
	if (IIarDump != NULL)
		FDrcvclose(FALSE);
	if (IIarDebug != NULL)
		SIclose(abDbgfile);
}
