/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<te.h>
#include 	<tc.h>

/**
** Name:	TEswitch.c	- Contains TEswitch, of course.
**
** Description
**	Contains the routine to toggle TE's input from a file/terminal
**	to the terminal/file.
**
** History	
 * Revision 1.1  90/03/09  09:16:40  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:45:29  source
 * sc
 * 
 * Revision 1.2  89/06/12  11:37:16  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:21:45  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:13:36  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:46:23  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  11:13:30  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  11:13:03  joe
 * *** empty log message ***
 * 
 * Revision 1.1  85/10/10  17:22:52  joe
 * Initial revision
 * 
*/

static FILE	*TEalternate = NULL;

extern int	IIte_use_tc;
extern TCFILE 	*IItc_in;

/*{
** Name:	TEswitch	- Switch TE's input.
**
** Description:
**	Toggles TE's input between the file open by TEtest and
**	the terminal.  TEtest must have been called first.
**
** Outputs
**	Returns:
**		Status. FAIL is returned if TEtest has not been
**		called!
**
** Side Effects
**	Changes the value of IIte_fpin between stdin and a file.
**	Stores the alternate file pointer in a static.
**
** History
**	9-aug-1985 (joe)
**		First Written.
**	23-may-89  (mca)
**		Added support for switching to/from a TCFILE, if we're in
**		SEP test mode.
**	12-jun-89  (mca)
**		Changed bitwise NOT to logical NOT when toggling TCFILE mode.
**	01-jul-1999 (toumi01)
**		Move IIte_fpin = stdin init to run time, avoiding compile error
**		under Linux glibc 2.1: "initializer element is not constant".
**		The GNU glibc 2.1 FAQ claims that "a strict reading of ISO C"
**		does not allow constructs like "static FILE *foo = stdin" for
**		stdin/stdout/stderr.
*/
STATUS
TEswitch()
{
	FILE	*tmp;

	if (IItc_in != NULL)
	{
		IIte_use_tc = !IIte_use_tc;
		return OK;
	}
	/*
	** First time.  Make sure
	*/
	if (IIte_fpin == NULL)
		IIte_fpin = stdin;
	if (TEalternate == NULL)
	{
		/*
		** If IIte_fpin is equal to stdin then
		** TEtest was not called!
		*/
		if (IIte_fpin == stdin)
			return FAIL;
		TEalternate = stdin;
	}
	tmp = IIte_fpin;
	IIte_fpin = TEalternate;
	TEalternate = tmp;
	return OK;
}
