/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ex.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	"ilargs.h"

/**
** Name:	ilexhdlr.c -	Exception handlers within interpreter
**
** Description:
**	Contains 2 handlers for the run-time interpreter:
**		IIOmhMhandler()		Math exception handler.
**		IIOehExitHandler()	Handler called before exiting.
**
** History:
**	Revision 5.1  86/09/05  arthur
**	Initial revision.
**	86/11/18  arthur  IIOehExitHandler() added.
**
**	Revision 6.5
**	01-feb-93 (davel)
**		Added exceptions for decimal (divide by zero and overflow)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF ILARGS	IIITARGS;

/*{
** Name:	IIOmhMhandler() - Handler for math exceptions within interpreter
**
** Description:
**	A routine that can be called by the interrupt handler whenever a math
**	exception is raised while an expression is being evaluated by
**	the run-time interpreter.  The math exceptions handled here are
**	integer and float divide by 0, integer and float overflow, and
**	float underflow.
**
**	This routine prints out an error message and ends the evaluation
**	of the current expression by returning EXDECLARE.
**
**	If the exception raised is not one of those known by this routine,
**	IIOmhMhandler() will return EXRESIGNAL, and the exception will be
**	caught by some outer handler.
**
**	This routine could later be generalized for use by other front-ends.
**
** Inputs:
**	ex	The exception structure.
**
** Outputs:
**	Returns:
**		EXDECLARE if the exception was one handled by this routine,
**		else EXRESIGNAL.
**
** History:
**	5-sep-1986 (agh)
**		Written
*/

EX
IIOmhMhandler (ex)
EX_ARGS		*ex;
{
	i4	exarg_n;	/* the exception number */

	exarg_n = ex->exarg_num;

	switch (exarg_n)
	{
	  case EXFLTDIV:
	  case EXINTDIV:
	  case EXDECDIV:
		IIOerError(ILE_STMT, ILE_DIVZERO, (char *) NULL);
		return (EXDECLARE);

	  case EXFLTOVF:
	  case EXFLTUND:
	  case EXINTOVF:
	  case EXDECOVF:
		IIOerError(ILE_STMT, ILE_NUMOVF, (char *) NULL);
		return (EXDECLARE);

#ifdef EXFLOAT
	  case EXFLOAT:
		IIOerError(ILE_STMT, ILE_FLTBAD, (char *) NULL);
		return (EXDECLARE);
#endif /* EXFLOAT */

	  default:
		return (EXRESIGNAL);
	}

	/* NOTREACHED */
}

/*{
** Name:	IIOehExitHandler()	-	Handler called before exiting
**
** Description:
**	A routine called by the interrupt handler whenever there is
**	about to be an exit from the interpreter caused by an exception.
**	IIOehExitHandler() calls the ILRF clean-up routine, and then passes
**	control on to the next outer handler in line, presumably FEhandler().
**
** Inputs:
**	ex	The exception structure.
**
** Outputs:
**	Returns:
**		EXRESIGNAL.
**
** History:
**	18-nov-1986 (agh)
**		Written.
*/

EX
IIOehExitHandler (ex)
EX_ARGS		*ex;
{
	EX exret;
	FUNC_EXTERN EX	FEhandler();
    
	exret = IIOmhMhandler(ex); 
	if (exret == EXDECLARE)
		return(EXDECLARE);

	IIORscSessClose(&il_irblk);
	if (IIOdebug)
	{
	    if (IIITARGS.ildbgfile != stdout)
		SIclose(IIITARGS.ildbgfile);
	}
	return (FEhandler(ex));
}
