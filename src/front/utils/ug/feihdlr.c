/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>
#include	<ex.h>
#include	<si.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:    feihdlr.c -	Front-End Interrupt Handler Module.
**
** Description:
**	Contains the front-end interrupt handler routines.
**
**	FEhandler()	front-end interrupt handler.
**	FEjmp_handler()	front-end non-local goto handler.
**
** History:
**	Revision 5.1  86/10/17  16:13:01  wong
**	Added Front-End memory and bug exceptions.  10/86 wong.
**	Added front-end out of memory exception.  08/86 wong.
**
**	Revision 4.0  86/04/23  15:28:16  wong
**	Initial revision (abstracted from 'FDhandler()'.)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:    FEhandler() -	Front-End Interrupt Handler.
**
** Description:
**	A routine that can be called by the interrupt handler whenever a signal
**	is generated for the process.  Usually this is when the user types a
**	interrupt character (e.g., control-C), but this may also happen if a
**	coding error generates a hardware exception.
**
**	This routine tries to exit gracefully by executing any module clean-up
**	routines through 'FEexits()'.  (Only Front-End modules that absolutely
**	require clean-up should set an exit clean-up routine upon module
**	start-up.  At present, this is only the database and forms system
**	interfaces.)
**
**	To use this routine, the following call should be put in the main line
**	of the program that is to use it, just before INGRES or the Forms system
**	is started (see "EX" for details):
**
**		if (EXdeclare(FEhandler, &context) != OK)
**			return FAIL;
**
**	Since this routine handles all interrupts and never returns, it should
**	be declared before any other interrupt handling routines that a program
**	is to use.  It will then handle only the interrupts not handled by other
**	declared interrupt handlers.  Hardware/system exceptions are resignaled
**	so that EX can handle them in a system-dependent manner.
**
** Inputs:
**	ex	{EX_ARGS *}  The exception structure.
**
** Returns:
**	{STATUS}  EXRESIGNAL on hardware/system exception, otherwise never.
**
** History:
**	01/86 (jhw) -- Modified from 'FDhandler()' in "frame/intrhdlr.c".
**	29-08-1988 (elein)
**		Changed exception output from stderr to stdout
**	24 feb 91 (seg)
**	    Fixed to correspond to spec: return STATUS
**	    Use less stack space in case that is cause of exception
**	    Don't bother with informative messages on recursive exceptions,
**		we should never see them in the first place, and we may
**		cause endless recursion by trying to be too kind.
*/

static u_i4	called = 0;

STATUS
FEhandler (ex)
EX_ARGS	*ex;
{
    char    buf[1024];	/* Buffer for some messages */
    i4      exarg_n;	/* the exception number */
    char    *cp;
    bool    system = FALSE;

    VOID    FEexits();

    if (ex->exarg_num == EX_UNWIND)
	return EXRESIGNAL;	/* do nothing on unwind from EXRESIGNAL below */

    if (called++ > 0) /* should never happen according to spec.... */
	return EXRESIGNAL;

    exarg_n = ex->exarg_num;
    FEexits(ERget(S_UG0030_Exiting));
    STcopy(ERget(S_UG0031_Because), cp = buf);
    if (exarg_n == EXINTR)
	cp = STcat(cp, ERget(S_UG0032_Interrupt));
    else if (exarg_n == EXKILL)
	cp = STcat(cp, ERget(S_UG0033_Termination));
    else if (exarg_n == EXQUIT)
	cp = STcat(cp, ERget(S_UG0034_Quit));
    else if (exarg_n == EXTEEOF)
	cp = STcat(cp, ERget(S_UG0035_EOF));
    else
    {
	if (exarg_n == EXFEMEM)
	{ /* front-end out of memory */
	    cp = STcat(cp, ERget(S_UG0036_NoMemory));
	    if (ex->exarg_num == 1)
	    {
		cp = STprintf(buf + STlength(buf), ERget(S_UG003B_Routine),
				'\n', ex->exarg_array[0]
		);
	    }
	}
	else if (exarg_n == EXFEBUG)
	{ /* front-end bug */
	    cp = STcat(cp, ERget(S_UG0037_BugCheck));
	    if (ex->exarg_count == 1)
	    {
		cp = STprintf(buf + STlength(buf), ERget(S_UG003C_BugMessage),
				'\n', ex->exarg_array[0]
		);
	    }
	}
	else if (!(system = EXsys_report(ex, (cp = buf))))
	    cp = STprintf(buf, ERget(S_UG0038_UnknownException), exarg_n);

	cp = STcat(buf, ERget(S_UG003F_Contact));
    }

    SIfprintf(stdout, cp);
    SIputc('\n', stdout);
    SIflush(stdout);
    if (!system)
	PCexit(exarg_n);

    return EXRESIGNAL;	/* let EX handle hardware/system exceptions */
}

/*{
** Name:	FEjmp_handler() -	Front-End Non-Local Goto Handler.
**
** Description:
**	Interrupt handler used to implement non-local, scoped goto's by
**	routine in the front-ends.  This routine should be set up by the
**	'FEsetjmp()' macro in "FEjmp.h".
**
** Inputs:
**	ex -- The exception structure.
**
** Returns:
**	EXDECLARE	if long jump exception (EXVALJMP).
**	EXRESIGNAL	otherwise.
**
** History:
**	02/86 (jhw) -- Written.
**	24 feb 91 (seg)
**	    Fixed to correspond to spec: return STATUS
*/

STATUS
FEjmp_handler (ex)
EX_ARGS		*ex;
{
    return ex->exarg_num == EXVALJMP ? EXDECLARE : EXRESIGNAL;
}
