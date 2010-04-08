/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<iisqlca.h>
# include	<iilibq.h>

/**
+*  Name: iiseterr.c - Set the error function pointer.
**
**  Description:
**	Allows a language which can pass a procedure as a parameter
**	to set the name of a routine to call when an INGRES error
**	is encountered.
**
**  Defines:
**	IIseterr
**
**  Notes:
**	See the IIerror notes.
-*
**  History:
**	13-oct-1986	- Modified for 6.0 [global-free] (mrw)
**	19-may-1989	- Modified names of globals for multiple connect. (bjb)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIseterr - desc
**
**  Description:
**	If this routine returns the error number passed as its single
**	argument, the INGRES error message is printed. If the routine
**	returns zero, the EQUEL package is silent.
**
**  Inputs:
**	proc	- Address of procedure to process errors.
**
**  Outputs:
**	Returns:
**	    i4	(*old)()	- The previous error function pointer.
**	Errors:
**	    None.
-*
**  Side Effects:
**	None.
**  History:
**	15-jul-1985 - return address of previous error routine (gwb)
**	13-oct-1986 - Modified for 6.0 w/o globals (mrw)
**	19-may-1989 - Modified names of global vars for multiple connect. (bjb)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
*/

i4 (*IIseterr(proc))()
i4	(*proc)();
{
    i4	(*old)();

    old = IIglbcb->ii_gl_seterr;
    IIglbcb->ii_gl_seterr = proc;

    return old;
}
