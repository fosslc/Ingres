/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:    feexits.c -	Front-End Exits Module.
**
** Description:
**	Contains routines used to set, clear and execute exit clean-up routines
**	for the Front-Ends.  Exit clean-up routines are set by modules that need
**	to do clean-up on exit from an interrupt, e.g., forms.  They will then
**	be executed in LIFO order of their being set upon exit from interrupt.
**
**	This module is usually used by the Front-End Interrupt Handler module
**	and defines:
**
**	FEexits()	execute exit clean-up routines.
**	FEset_exit()	set an exit clean-up routine.
**	FEclr_exit()	clear an exit clean-up routine.
**
**	NOTE:  THIS MODULE CANNOT REPORT BUG CHECKS USING EXCEPTIONS SINCE
**	THAT WOULD CAUSE A RECURSIVE LOOP.  USE 'IIUGerr()' INSTEAD.
**
** History:
**	Revision 6.0  86/12/16  12:22:47  wong
**	Copied real version from "graphics/graf".
**
**	Revision 40.0  86/03/03  15:51:56  wong
**	Initial revision.  (Implemented from a suggestion made by
**	Neil Goodman.)
**
**	01-dec-1999 (rigka01) bug#99200
**	  Attempting to use an ABF application after cancelling out of
**	  killing it results in an access violation.  Check for 0 value
**	  for '*routine' in FEexits() 
**	26-may-2000 (rigka01) bug#101673
**	  compilation of feexits.c fails on UNIX with:
**	  "feexits.c", line 96: controlling expressions must have
**	  scalar type
**	  type cast (*routine) from change for bug #99200 to (i4)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*
** The exit clean-up routines are stored in a LIFO stack defined by the
** following data elements.  The LIFO structure enables the exit clean-up
** routines to be called in LIFO order of their being set.
*/

#define MAX_EXITS	8

static i4	(*exits[MAX_EXITS])();
static i4	(**exit_top)() = exits;

/*{
** Name:    FEexits() -	Front-End Execute Exit Clean-Up Routines Utility.
**
** Description:
**	Outputs the input string using 'FEmsg()' and then executes each modules
**	exit clean-up routine in LIFO order of their having been set.
**
** Inputs:
**	str	{char *}  A informational string to be output before executing
**			  any exit clean-up routines.
**
** History:
**	03/86 (jhw) -- Written.
**	01-dec-1999 (rigka01) bug#99200
**	  Attempting to use an ABF application after cancelling out of
**	  killing it results in an access violation.  Check for 0 value
**	  for '*routine' in FEexits() 
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

VOID
FEexits (char *str)
{
    register i4	i;

    if (str != NULL)
	FEmsg(str, FALSE);

    for (i = exit_top - exits - 1 ; i >= 0 ; --i)
    {
	i4	(*routine)();

	/* Remove routine from Exits stack before calling it
	** to avoid possible recursion on error when it is called.
	*/
	routine = exits[i];
	exits[i] = NULL;

	if (routine != NULL)
	  (*routine)();	/* do it */
	--exit_top;
    }
}

/*{
** Name:    FEset_exit() -	Front-End Set Exit Clean-Up Routine Utility.
**
** Description:
**	Pushes a module's exit clean-up routine on the 'exits[]' stack.
**
** Input:
**	routine		{nat (*)()}  The address of the module's exit clean-up
**					routine.
**
** History:
**	03/86 (jhw) -- Written.
*/

VOID
FEset_exit (routine)
i4	(*routine)();
{
    if (exit_top > &exits[MAX_EXITS])
	IIUGerr(E_UG000B_ExitsLimit, UG_ERR_FATAL, 0);

    *exit_top++ = routine;
}

/*
** Name:    FEclr_exit() - Front-End Clear Exit Clean-Up Routine Utility.
**
** Description:
**	Although exit clean-up routines are executed in LIFO order of their
**	being set, a module need not end in LIFO order of its beginning with
**	respect to other modules' beginnings.  This routine clears the module's
**	exit clean-up routine from the 'exits[]' stack and collapses the stack
**	to fill in the newly empty entry.
**
** Input:
**	routine		{nat (*)()}  The address of the module's exit clean-up
**					routine.
**
** History:
**	03/86 (jhw) -- Written.
*/

VOID
FEclr_exit (routine)
i4	(*routine)();
{
    register i4	(**exit_p)();

    /* Remove entry */
    for (exit_p = exits ; exit_p <= exit_top ; ++exit_p)
	if (*exit_p == routine)
	{
	    *exit_p = NULL;
	    break;
	}

    if (exit_p > exit_top)
	IIUGerr(E_UG000C_ExitsMissing, UG_ERR_FATAL, 0);

    /* Collapse stack */
    for ( ; exit_p < exit_top ; ++exit_p)
	*exit_p = *(exit_p + 1);

    --exit_top;
}
