/*
 *	Copyright (c) 1983, 2000 Ingres Corporation
 *
 *	Name:
 *		PCexit.c
 *
 *	Function:
 *		PCexit
 *
 *	Arguments:
 *		i4	status;
 *
 *	Result:
 *		Program exit.
 *		Status is value returned to shell or command interpreter.
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *		09/83 (wood) -- adapted to VMS environment.
**		10-apr-84 (fhc) -- adapted to transalate "normal" unix numbers
**				to normal vms numbers
 *		19-feb-88 -- (marge)  
 *			added PCatexit function to support ATEX recovery feature
 **		4/91 (Mike S) Change SI_iob to SI_iobptr
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
 *
 *
 */


# include	<ssdef.h>
# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<ex.h>
# include	<starlet.h>

# define 	MAXFUNCS	32

static VOID	(*PCexitfuncs[ MAXFUNCS ])();
static VOID	(**PClastfunc)() = PCexitfuncs;

PCexit(status)
i4	status;
{
	FILE	*fp = SI_iobptr;
	register VOID (**f)()=PClastfunc;

	int	i;

	for (i = 0; i < _NFILE; i++)
		SIclose(fp++);
	while (f-- != PCexitfuncs)	/* Execute any cleanup funcs pushed */
		(**f)();

	/*
	** we reset low order bit on control-c / control-y
	** so that control-c'ed processes exit abnormally - desired
	** so that things like ABF don't proceed on the assumption
	** that other executables they call worked if aborted by the user
	*/
	if (status == 0)
		status = SS$_NORMAL;
	else if (status == -1)
		status = SS$_ABORT;
	else if (status == FAIL)
		status = (0x10000000|SS$_ABORT);
	else if (status == SS$_CONTROLC || status == SS$_CONTROLY)
		status &= ~1;
	sys$exit(status);
}


/*{
**  Name:	PCatexit	- Push exit functions on func stack 
**
**  Description:
**	Puts the entry to an cleanup function on the exitfunction stack
**	This stack (array) is read by PCexit and each function
**	on the stack is executed in LIFO order.
**
**  Inputs:
**	func	- Entry to cleanup function to be called at exit
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  History:
**	02-jan-1988  (daveb) -- Written for 5.0
**	19-feb-1988  (marge) -- Moved to 6.0
**      16-jul-93 (ed)
**	    added gl.h
*/

VOID
PCatexit( func )
VOID (*func)();
{
    	if( PClastfunc < &PCexitfuncs[ MAXFUNCS ] )
        	*PClastfunc++ = func;
}

