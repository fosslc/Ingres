/******************************************************************************
**
** Copyright (c) 1985, 2003 Ingres Corporation
**
******************************************************************************/

# include   <stdarg.h>
# include   <stdio.h>
# include   <math.h>
# include   <compat.h>
# include   <clconfig.h>
# include   <ex.h>
# include   <cs.h>
# include   <me.h>
# include   <tr.h>
# include   <windef.h>
# include   <pc.h>
# include   <st.h>
# include   <er.h>
# include   <exinternal.h>

/******************************************************************************
**
** Name:    EX.C -  Exception handling module
**
** Description:
**  This file contains functions to manage exceptions.
**
** History:
**    20-May-95 (fanra01)
**          Added check in all calls with NT critical sections that the
**          structures have been initialised.  Initialisation is mutexed
**          to protect against reentrancy.
**          Branched for Windows NT.
**    23-Jun-95 (emmag)
**	    Fixed up the previous fix which doesn't release the MUTEX
**	    after it grabs it.  This could cause us to hang on a wait
**	    for the mutex since we wait forever.
**    15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**    24-jul-95 (emmag)
**	    Changed the keyboard routine so that the ctrl-c sequence
**	    in the terminal monitor now works as expected.
**    19-sep-95 (emmag)
**	    Changed iEXdeliver so that it now exits 
**	    when an EXEXIT is passed down, as happens on UNIX.
**    12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**    06-Feb-96 (fanra01)
**          Bug # 74515.
**          Replaced __declspec(thread) Ex_handler_stack with thread local
**          storage.  All access to Ex_handler_stack no incurs an extra level
**          of indirection.
**    03-mar-1996 (canor01)
**	    Changed iEXdeliver so that it now exits 
**	    when an EXEXIT is passed down only if there is no handler
**	    installed.
**    07-mar-1996 (canor01)
**	    Take default action of EXEXIT when all handlers have been
**	    taken and we are not trying to unwind.
**    17-may-1996 (canor01)
**	    Removed matherr function.
**    17-dec-1996 (donc)
**	    Pro-forma bullet-proofing don't look "into" a NULL Ex_handler_stack 
**    27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**    25-jun-1997 (canor01)
**          Add better exception reporting and stack dumping if available.
**    16-jan-1998 (canor01)
**	    Remove unnecessary MEfill of ii_context in EXsetup.
**    23-jan-1998 (canor01)
**	    Dump of Intel registers should be only for NT_INTEL.
**    04-mar-1998 (canor01)
**	    Since the Ex_handler_stack is thread-local, we don't need to
**	    protect it with a critical section in EXsetup and EXdelete.
**    06-aug-1999 (mcgem01)
**        Replace nat and longnat with i4.
**    15-nov-1999 (somsa01)
**	    Changed EXCONTINUE to EXCONTINUES.
**    08-feb-2001 (somsa01)
**	    Cleaned up compiler warnings.
**    23-may-2002 (somsa01)
**	    In EXsys_report(), after filling the message buffer, send it
**	    to the log via ERsend() (as we do on UNIX). Also, properly set
**	    exrp and ctxtp.
**    14-mar-2003 (somsa01)
**	    Modified EXexit() into EX_TLSCleanup(), used to clean up EX
**	    TLS allocations.
**    10-sep-2003 (horda03) Bug 110889/INGSRV 2521
**          Clear System Interrupt Handlers prior to setting up Ingres'
**          Interrupt handler. This allows CTRL-C interrupt to be processed
**          by the Interrupt handler setup here.
**	16-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
******************************************************************************/

/*
**
**  Forward declarations
**
*/

static STATUS ex_print_error(PTR arg1, i4 msg_length, char * msg_buffer);

STATUS       iEXdeliver(EX_ARGS * argp);
VOID         EXsignal(EX ex, i4 arg_count, i4 arg1,...);
BOOL WINAPI  keyboard(DWORD);

bool    EXtlReady (void);
STATUS  EXtlinit (void);
DWORD   EXtlid (void);
STATUS  EXtlput (DWORD tlsid, PTR tlsitem, u_i4 size);
STATUS  EXtlget (DWORD tlsid, PTR * tlsitem);
STATUS  EXtlrelease (DWORD tlsid, PTR tlsitem);

/*
**
**  Global Data Structures
**
*/

#define   MAX_EX_THREADS 4096

/*
**
** The stack (LIFO) of exception handlers
**
*/

static DWORD    Extlsidentifier = 0xFFFFFFFF;
# define EXTLID Extlsidentifier

/*
**
** Internal Variables and Data Structures
**
*/

static CRITICAL_SECTION Ex_critical_section;
GLOBALREF CRITICAL_SECTION SemaphoreInitCriticalSection;
GLOBALREF CRITICAL_SECTION ConditionCriticalSection;
HANDLE hLocalMutex = NULL;

/*
**
** The list (FIFO) of pending exceptions
**
*/

static EX_ARGS *Ex_xcpt_list = NULL;

/*
**
** The count of 'disable user interrupt' levels.
**
*/

static i4       EX_disable_level = 0;

/*
**
** The count of pending user interrupts.
**
*/

static i4       EXsigints = 0;

/*
**
** The count of pending user quit signals.
**
*/

static i4       EXsigquits = 0;

/*
**
** The next two variables are used to help avoid recursion.
**
*/

static i4       InInit = 0;
static i4       InQuit = 0;

/*
**
** DEBUG
**
*/

static i4       stack_count[MAX_EX_THREADS] ZERO_FILL;

/*
**
** Keep track of the fact we've done any exception system initialization.
**
*/

static i4       initialized = 0;
static i4 	startupdone = 0;

/******************************************************************************
**
** Name:    EXsetup() - Install an exception handler in EX stack
**
** Description:
**      Append the given handler and context to the EX stack which is a
**      singly linked list.
**
**      This function is called out of the EXdeclare() macro defined in the
**      ex.h as the following:
**
**      # define EXdeclare(x,y) (EXsetup(x,y), setjmp((y)->jmpbuf))
**
** Input:
**      handler         the exception handler function to install
**      context          the exception context
**
** Output:
**      none
**
** Return:
**      OK, EX_BADCONTEXT
**
** History:
**	16-jan-1998 (canor01)
**	    Remove unnecessary MEfill of ii_context.
**      10-sep-2003 (horda03) Bug 110889/INGSRV 2521
**          During initialisation of the exception handling ensure
**          that any OS defined handlers are removed prior to
**          installing the exception handler to be used.
******************************************************************************/
i4 
EXsetup(STATUS(*handler) (EX_ARGS *), EX_CONTEXT *ii_context)
{
	i4              tid;
	EX_CONTEXT     *hold;
        EX_CONTEXT     * * Ex_handler_stack = NULL;
	int             status;

	if(!startupdone)
            EXstartup();

        if ( ((status = EXtlget(EXTLID, (PTR *) &Ex_handler_stack)) == OK) &&
             (Ex_handler_stack == (EX_CONTEXT * *)0) )
        {
            Ex_handler_stack = (EX_CONTEXT **) MEreqmem(0,sizeof(PTR),TRUE,&status);
            if (Ex_handler_stack != NULL)
            {
                *Ex_handler_stack = NULL;
                if ((status = EXtlput(EXTLID, (PTR) Ex_handler_stack,sizeof(PTR))) != OK)
                {
                    TRdisplay("ERROR: EXsetup - EXtlput error %x",status);
                }
            }
        }
        else
        {
            if (status != OK)
                TRdisplay("ERROR: EXsetup - EXtlget error %x",status);
        }
	/* Get the thread id. */

	tid = GetCurrentThreadId();

	/* insert the handler into EX_CONTEXT structure */

	if (handler == NULL) {
		/* error, eh? */
	}

	/* add the context to the LIFO list */

	ii_context->marker_1   = EX_MARKER_1;
	ii_context->marker_2   = EX_MARKER_2 + tid;
	ii_context->handler    = handler;
	ii_context->next       = *Ex_handler_stack;

	*Ex_handler_stack    = ii_context;

	hold = *Ex_handler_stack;

	/* If it hasn't been done, set up the ^C/^break handler. */

	if (initialized == 0) {
                /* Remove any existing handler prior to adding
                ** ours. Thus allowing CTRL-C to work.
                */
                SetConsoleCtrlHandler( NULL, FALSE );

		status = SetConsoleCtrlHandler(keyboard, TRUE);
		initialized = 1;
	}

	return (OK);
}

/******************************************************************************
**
**  keyboard() exists in the WIN32 NT environment to trap conrol-c and
**  control-break user interrupts, and deliver them to the Ingres exception
**  handling system.
**
******************************************************************************/
BOOL WINAPI
keyboard(DWORD type)
{
	i4              ex;

	switch (type) {
	case CTRL_C_EVENT:
		ex = EXINTR;
		if (EX_disable_level) 
		{
			EXsigints++;
			return (TRUE);
		}
		EXsigints++;
		break;

	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		ex = EXQUIT;
		if (EX_disable_level) 
		{
			EXsigquits++;
			return (TRUE);
		}
		EXsigquits++;
		break;

	default:
		printf("keyboard() in EX: unknown event x%X\n", type);
		return (TRUE);
	}
	return (TRUE);
}

/******************************************************************************
**
** Name:    EXdelete - delete an active exception handler
**
** Description:
**      Delete the exception handler declared in the current call frame.
**      EXdelete must be called before returning from a routine in which
**      a handler is EXdeclare'd.  EXdelete must not be called from a
**      routine which does not have an active handler.
**
** Inputs:
**     None
**
** Outputs:
**     None
**
** Returns:
**     None
**
******************************************************************************/
STATUS
EXdelete(void)
{
	i4              tid;
	EX_CONTEXT     *hold;
	EX_CONTEXT     * *Ex_handler_stack;
        STATUS          status = OK;

        if(!startupdone)
            EXstartup();

        if ((status = EXtlget(EXTLID, (PTR *) &Ex_handler_stack)) != OK)
        {
            TRdisplay("ERROR: EXdelete - EXtlget error %x",status);
        }
	/* Get the thread id. */

	tid = GetCurrentThreadId();

	/* Remove the context from EX LIFO list */

	hold = * Ex_handler_stack;

	if (hold->marker_1 != EX_MARKER_1 ||
	    hold->marker_2 != (EX_MARKER_2 + tid)) {
		LeaveCriticalSection(&Ex_critical_section);
		PCexit(-1);
	}

	* Ex_handler_stack = hold->next;
	hold = (hold == 0 ? 0 : hold->next);

	return(status);
}

/******************************************************************************
**
** Name:    EXinterrupt - alter delivery of user generated exceptions
**
** Description:
**      Control delivery of normal user originated exceptions.  This
**      includes EXINTR.  These come at completely random times as
**      generated by a user at the keyboard.  The value of cmd must be
**      one of EX_OFF, EX_ON, EX_RESET, or EX_DELIVER.  By default,
**      exceptions are delivered immediately.
**
**  1)  EX_OFF means you don't want to receive these exceptions
**      when they are triggered.  It bumps a disable level and
**      has EX queue up the exceptions as pending.
**
**  2)  EX_ON decrements the disable level set by EX_OFF, and if
**      it becomes 0, delivers any pending exceptions.
**
**  3)  EX_RESET forces the disable level to 0 and delivers any
**      pending exceptions.
**
**  4)  EX_DELIVER causes delivery of any pending exceptions
**      without adjusting the disable level.  This lets you poll
**      for pending keyboard exceptions without opening a window
**      of vulnerability to their asynchronous delivery.
**
** Inputs:
**      cmd      One of EX_OFF, EX_ON, EX_RESET, or EX_DELIVER.
**
** Outputs:
**      None
**
** Returns:
**      None
**
******************************************************************************/
VOID
EXinterrupt(i4  cmd)
{
	i4              deliver;

        if(!startupdone)
            EXstartup();

	EnterCriticalSection(&Ex_critical_section);

	switch (cmd) {
	case EX_OFF:
		EX_disable_level++;
		deliver = FALSE;
		break;

	case EX_ON:
		if (--EX_disable_level > 0)
			break;

	case EX_RESET:
		EX_disable_level = 0;

	case EX_DELIVER:
		if (EX_disable_level > 0)
			break;

		if (EXsigints > 0) {
			EXsigints = 0;
			LeaveCriticalSection(&Ex_critical_section);
			EXsignal(EXINTR, 0, 0);
			EnterCriticalSection(&Ex_critical_section);
		}
		if (EXsigquits > 0) {
			EXsigquits = 0;
			LeaveCriticalSection(&Ex_critical_section);
			EXsignal(EXQUIT, 0, 0);
			EnterCriticalSection(&Ex_critical_section);
		}
		break;
	}


	LeaveCriticalSection(&Ex_critical_section);
	return;
}

/******************************************************************************
**
** Name:    EXmath - Enable/Disable some math exceptions
**
** Description:
**      Attempt to enable (or disable) the generation of math exceptions,
**      such as EXDECOVF, EXFLTUND, EXINTOVF, cancelling effect of any
**      previous EXmath() calls.
**
**      The effect of this routine will vary from machine to machine.  It
**      provides a hint about the behaviour the client would prefer.  Some
**      machines may never generated exceptions for exceptional math
**      conditions, and on other machines it may be impossible to turn them
**      off.
**
**      If the "hard" math exceptions may be raised on a system, from which
**      continuation is illegal, the compile time flag EX_HARD_MATH will
**      be defined.  This allows placement of additional handlers that may
**      be needed.
**
** Inputs:
**      cmd      One of EX_OFF or EX_ON
**
** Outputs:
**      None
**
** Returns:
**      None
**
******************************************************************************/
VOID
EXmath(i4  cmd)
{
	cmd = 0;
}

/******************************************************************************
**
** Name:    EXmatch - match exception values
**
** Description:
**
**      Given an exception, ex, it searches for it as a member of
**      its argument list.  It returns the index of the matching
**      exception number, or it returns zero to signify that none
**      matched.  Switch on the returned index to handle specific
**      exceptions.
**
**      This routine is necessary because the EX exception type is
**      an illegal case label in a switch in some environments.
**
** Inputs:
**      ex      The exception in question.
**      arg_count The number of following arguments
**      ex_match1 ...  Values to compare against the input exception.
**
** Outputs:
**      None
**
** Returns:
**      Index into the list of the matched exception (starting
**      at 1), or 0 if no match was found in the list.
**
******************************************************************************/
i4 
EXmatch(EX ex, i4  arg_count,...)
{
	i4              index;
	va_list         optional_args;

	va_start(optional_args, arg_count);
	for (index = 0; index < arg_count; index++) {
		if (ex == va_arg(optional_args, EX))
			return index + 1;
	}
	return 0;
}

/******************************************************************************
**
** Name:    EXsignal - raise exception
**
** Description:
**      Signal exception ex with handler arguments arg1, arg2, ....  It
**      is critical that arg_count is correct.
**
**      The maximum number of optional arguments passed to EXsignal is
**      EX_MAX_ARGS.  EX_MAX_ARGS is guaranteed to at least 4. Generic
**      code may not assume that EX_MAX_ARGS is greater than 4.
**      The calling routine must behave responsibly if the exception is
**      blindly continued with no action by the handler.  It may not loop
**      on an error assuming that the handler will clear it making
**      continuation possible.
**
** Inputs:
**          ex              the exception to raise.
**          arg_count       the number of following arguments, must be less
**                          than or equal to EX_MAX_ARGS.
**          arg1,...        argument to be passed to the handler function.
**
** Outputs:
**     None
**
** Returns:
**     None
**
******************************************************************************/
VOID
EXsignal(EX ex, i4  arg_count, i4 arg1,...)
{
	EX_ARGS         exarg;
	SIZE_TYPE      *arrayp = exarg.exarg_array;
	va_list         argp;

        if(!startupdone)
            EXstartup();            /* iEXdeliver contains critical section. */

	if (arg_count > EXMAXARGS)
		arg_count = EXMAXARGS;

	/* build the EX_ARGS */

	exarg.exarg_num = ex;
	exarg.exarg_count = arg_count;

	va_start(argp, arg1);
	for (*arrayp++ = arg1; arg_count > 1; arg_count--) {
		*arrayp++ = va_arg(argp, SIZE_TYPE);
	}
	va_end(argp);

	(void) iEXdeliver(&exarg);
	return;

}

/******************************************************************************
**
** Name:    EXsys_report - Report system/hardware/CL exceptions
**
** Description:
**      Reports if an exception was generated by hardware, the operating
**      system, or internally by the CL.  When this is the case an
**      appropriate message is copied into the buffer anf TRUE is returned.
**      Otherwise, FALSE is returned.
**
**      An exception for which EXsys_report retyrns TURE may not be
**      continued, unless documented otherwise Excoptions for which EXsys_
**      report returns FALSE may be continued.
**
**      Buffer may be NULL, in which case no diagnostic messahe output will
**      be available.
**
**      The buffer must be at least EX_MAX_SYS_REP in length.
**
** Inputs:
**      exarg       the exception argument structure to decode
**
** Outputs:
**      buffer      the address of the buffer that can contain the message
**                  for the hardware/system exception.
**
** Returns:
**      FALSE       if the exception is not raised within the CL or by the
**                  hardware or by the operating system.
**
** History:
**	25-jun-1997 (canor01)
**	    Add better exception reporting and stack dump.
**	23-may-2002 (somsa01)
**	    After filling the message buffer, send it to the log via
**	    ERsend() (as we do on UNIX). Also, properly set exrp and
**	    ctxtp.
**
******************************************************************************/
bool
EXsys_report(EX_ARGS * exargs, char *buffer)
{
    char             *cp;
    char             msg[ 80 ];
    bool	     print_stack = FALSE;
    EXCEPTION_RECORD *exrp;
    CONTEXT          *ctxtp;
    CL_ERR_DESC      error;

    /*
    ** This should report all hardware or system signals mapped to our
    ** internal Ingres exceptions.
    **
    ** For system delivered signals the argument count will be 3:
    ** The system-specific portion of EX_ARGS includes the elements:
    **    i4 signal     the signal number (or 0 if not raised by signal)
    **    i4 code       the sigcontext code or garbage.
    **    PTR scp       either a sigcontext structure or some other non-zero
    **                  value.
    **
    ** For software raised exceptions (EXsignal), the argument count will
    ** be 1, and exarg_array[] will contain nothing.
    **
    */
 
    if ( exargs->exarg_count != 3 )
        return FALSE;           /* definately not a system raised exception */
 
    switch (exargs->exarg_num) 
    {
	case EXBNDVIO:
		cp = "Memory Bounds Violation";
		print_stack = TRUE;
		break;

	case EXOPCOD:
		cp = "Illegal Opcode";
		print_stack = TRUE;
		break;
	case EXNONDP:
		cp = "Processor extension not available";
		break;
	case EXNDPERR:
		cp = "Processor extension segment overrun";
		break;
	case EXSEGVIO:
		cp = "General Protection Exception";
		print_stack = TRUE;
		break;
	case EXBUSERR:
		cp = "Data Misalignment";
		print_stack = TRUE;
		break;
	case EXINTR:
		cp = "Control-Break";
		break;
	case EXINTDIV:
		cp = "Integer divide error";
		break;
	case EXINTOVF:
		cp = "Integer overflow";
		break;
	case EXFLTOVF:
		cp = "Floating point overflow";
		break;
	case EXFLTUND:
		cp = "Floating point underflow";
		break;
	case EXFLTDIV:
		cp = "Floating point divide error";
		break;
	case EXFLOAT:
		cp = "Floating point error";
		break;
	case EXUNKNOWN:
	        STprintf( msg, "Unknown exception %x", exargs->exarg_array[0] );
		cp = msg;
		print_stack = TRUE;
		break;
	default:
		return (FALSE);
    }

    exrp = (EXCEPTION_RECORD *) exargs->exarg_array[1];
    ctxtp = (CONTEXT *) exargs->exarg_array[2];

    if (buffer != NULL)
    {
# ifdef NT_INTEL
        /* Intel-specific registers */
	STprintf( buffer, "%s @%x SP:%x BP:%x AX:%x CX:%x DX:%x BX:%x SI:%x DI:%x", 
		cp, exrp->ExceptionAddress, ctxtp->Esp, ctxtp->Ebp,
		ctxtp->Eax, ctxtp->Ecx, ctxtp->Edx, ctxtp->Ebx, ctxtp->Esi,
		ctxtp->Edi );

	ERsend(ER_ERROR_MSG, buffer, STlength(buffer), &error);
# endif /* NT_INTEL */
    }

    if ( print_stack && Ex_print_stack )
    {
        Ex_print_stack(0, ctxtp, NULL, ex_print_error, TRUE);
    }

    return (TRUE);
}

/******************************************************************************
**
** Name:    iEXdeliver - deliver exception to handlers.
**
** Description:
**      Deliver the given exception to the handlers on the EX LIFO.
**
**      This function is internal to EX.
** Inputs:
**          argp              the exception argument to deliver.
**
** Outputs:
**     None
**
** Returns:
**     None
**
******************************************************************************/
STATUS
iEXdeliver(EX_ARGS *argp)
{
	EX_CONTEXT     *context, *inner;
	STATUS          rc;
	STATUS(*handler) (EX_ARGS *);
	EX_CONTEXT     * *Ex_handler_stack;


	EnterCriticalSection(&Ex_critical_section);

	/* Guard against handling multiple user interrupts. */
        if ((rc = EXtlget(EXTLID, (PTR *) &Ex_handler_stack)) != OK)
        {
            TRdisplay("ERROR: EXdelete - EXtlget error %x",rc);
        }

	switch (argp->exarg_num) {
	case EXINTR:
		if (InInit++ > 0) {
			LeaveCriticalSection(&Ex_critical_section);
			return (EXCONTINUES);
		}
	case EXQUIT:
		if (InQuit++ > 0) {
			LeaveCriticalSection(&Ex_critical_section);
			return (EXCONTINUES);
		}
	default:
		break;
	}


	/* Check for an absolutely blown EX stack... */

	if ( !Ex_handler_stack || *Ex_handler_stack == NULL) {
		LeaveCriticalSection(&Ex_critical_section);
		if (argp->exarg_num == EXEXIT)
			exit(0);
		/* In the present form, the following will cause WIN32 NT */
		/* to look for another handler, which will result in */
		/* the default, program termination. */
		return (EXRESIGNAL);
	}

	/*
	 * Call handlers on the EX stack.  Take action based on the return
	 * code from each handler
	 */

	for (context = *Ex_handler_stack;
	     context;
	     context = context->next) {

		if (context == NULL) {
	    		LeaveCriticalSection(&Ex_critical_section);
			if (argp->exarg_num == EXEXIT)
				exit(0);
			return (EXRESIGNAL);
		}

		handler = context->handler;

		if (handler == NULL) {
			continue;
		}

		LeaveCriticalSection(&Ex_critical_section);

		rc = (*handler) (argp);

		EnterCriticalSection(&Ex_critical_section);

		switch (rc) {

		case EXRESIGNAL:
			continue;

		case EXCONTINUES:
			InInit = InQuit = 0;
			LeaveCriticalSection(&Ex_critical_section);
			return (EXCONTINUES);
			break;

		case EXDECLARE:
			break;

		default:
			LeaveCriticalSection(&Ex_critical_section);
			PCexit(-1);

		}
		break;
	}

	/* if all handlers have been handled, bail out */
	if ( context == NULL )
	{
		if ( argp->exarg_num == EXEXIT )
			exit( 0 );
		EXsignal( EXEXIT, 0, 0 );
	}

	/*
	** Unwind the EX stack from the top of stack to
	** the current handler; then longjump to that handler.
	*/

	argp->exarg_num = EX_UNWIND;
	argp->exarg_count = 0;

	for (inner = *Ex_handler_stack;
	     inner != context;
	     inner = inner->next) {

		if (*Ex_handler_stack == NULL) {
			return (EXRESIGNAL);
		}

		handler = inner->handler;

		if (handler == NULL) {
			continue;
		}
		LeaveCriticalSection(&Ex_critical_section);

		rc = (*handler) (argp);

		EnterCriticalSection(&Ex_critical_section);
		*Ex_handler_stack = inner->next;
	}

	InInit = InQuit = 0;
	LeaveCriticalSection(&Ex_critical_section);

	longjmp(context->jmpbuf, EXDECLARE);

	/* Should never get here, but compiler will report error otherwise */

	return (rc);
}

/******************************************************************************
**
** EXstartup must be called when a process starts up
**
** History:
**     15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**      24-Jan-96 (fanra01)
**          Added check and initialisation of thread local storage.
**
******************************************************************************/
VOID
EXstartup()
{
    SECURITY_ATTRIBUTES sa;

    iimksec (&sa);

    hLocalMutex = CreateMutex(&sa, FALSE, NULL);
    if ( hLocalMutex != NULL )
    {
        if (WaitForSingleObject(hLocalMutex, INFINITE) != WAIT_FAILED )
	{
            if (!startupdone)
            {
                InitializeCriticalSection(&Ex_critical_section);
                InitializeCriticalSection(&SemaphoreInitCriticalSection);
                InitializeCriticalSection(&ConditionCriticalSection);
                if (!EXtlReady())
                    EXtlinit();
                startupdone = 1;
            }
            ReleaseMutex(hLocalMutex);
	}
    }
    return;
}

/*{
**  Name: EX_TLSCleanup
**
**  Description:
**	Cleans up thread-local storage for EX module.
**
**  Inputs:
**	none
**
**	Returns:
**	    VOID
**
**  History:
**	14-mar-2003 (somsa01)
**	    Modified from original EXexit() function, which was never used.
*/
VOID
EX_TLSCleanup()
{
    EX_CONTEXT	*Ex_handler_stack;
    STATUS	status;

    if ((status = EXtlget(EXTLID, (PTR *)&Ex_handler_stack)) == OK)
    {
	if (Ex_handler_stack)
	    MEfree((PTR)Ex_handler_stack);
    }
    else
    {
	TRdisplay("ERROR: EXdelete - EXtlget error %x",status);
    }

    EXtlput(EXTLID, 0, sizeof(PTR));
    return;
}

/******************************************************************************
**
**  Name:  EXfilter - exception filter for WIN32 NT's Structured
**                                       Exception Handler.
**
**  Description: Determine the exception that got us here, translate
**               this into something Ingres understands, and pass on
**               the exception to the EX context stack.
**               NOTE: this is *NOT* IAW the WIN32 spec; we are
**               supposed to simply determine if we want to handle
**               the exception - not actually do it!  However, until
**               that golden day when Ingres decides to do things in
**               a structured manner, ...
**
**  Inputs:  None.
**
**  Returns: EXCEPTION_EXECUTE_HANDLER: in this case, exit the system.
**           EXCEPTION_CONTINUE_SEARCH: Punt.  The practical effect in
**                                      our case is to exit.
**           EXCEPTION_CONTINUE_EXECUTION: continue execution at the point
**                                      the exception was raised.
**
******************************************************************************/
DWORD
EXfilter(EX ex)
{
	DWORD           rc;
	EX_ARGS         exarg;


	switch (ex) {

	case STATUS_DATATYPE_MISALIGNMENT:
		ex = EXBUSERR;
		break;

	case STATUS_CONTROL_C_EXIT:
		ex = EXINTR;
		break;

	case STATUS_ACCESS_VIOLATION:
	case STATUS_NONCONTINUABLE_EXCEPTION:
	case STATUS_STACK_OVERFLOW:
	case STATUS_ARRAY_BOUNDS_EXCEEDED:
	case STATUS_PRIVILEGED_INSTRUCTION:
		ex = EXSEGVIO;
		break;

	case STATUS_TIMEOUT:
		ex = EXTIMEOUT;
		break;

	case STATUS_ILLEGAL_INSTRUCTION:
		ex = EXOPCOD;
		break;

	case STATUS_FLOAT_DIVIDE_BY_ZERO:
		ex = EXFLTDIV;
		break;

	case STATUS_FLOAT_OVERFLOW:
		ex = EXFLTOVF;
		break;

	case STATUS_FLOAT_UNDERFLOW:
		ex = EXFLTUND;
		break;

	case STATUS_INTEGER_DIVIDE_BY_ZERO:
		ex = EXINTDIV;
		break;

	case STATUS_INTEGER_OVERFLOW:
		ex = EXINTOVF;
		break;

	default:
		return (EXCEPTION_CONTINUE_SEARCH);
		break;
	}

	/* build the EX_ARGS */

	exarg.exarg_count = 0;
	exarg.exarg_num   = ex;

	switch (iEXdeliver(&exarg)) {
	case EXRESIGNAL:

		/* Under our current incarnation, getting here means we're */
		/* giving up, and the OS is probably gonna terminate us. */

	case EXDECLARE:
		rc = EXCEPTION_CONTINUE_SEARCH;
		break;

	case EXCONTINUES:
		rc = EXCEPTION_CONTINUE_EXECUTION;
		break;

	default:
		rc = EXCEPTION_EXECUTE_HANDLER;
		break;
	}
	return (rc);
}

/******************************************************************************
**
**  Name:  EXtlReady - Checks that thread local storage has been initialised.
**
**
**  Description: Determine that the thread local storage system has been
**               initialised.
**
**  Inputs:  None.
**
**  Returns: TRUE       thread local storage is ready
**           FALSE      thread local storage needs to be initialised
**
******************************************************************************/

bool
EXtlReady ()
{
    return( ((Extlsidentifier != 0xFFFFFFFF) ? TRUE : FALSE) );
}

/******************************************************************************
**
**  Name:  EXtlinit - Initialise the thread local storage system.
**
**  Description: Initialise the thread local storage system and get a thread
**               storage identifier.
**
**  Inputs:  None.
**
**  Returns: OK        thread local storage is ready
**           !OK       thread local storage failed to initialise
**
******************************************************************************/

STATUS
EXtlinit ()
{

STATUS status = OK;

    if(Extlsidentifier == 0xFFFFFFFF)
    {
        Extlsidentifier = TlsAlloc();
        if(Extlsidentifier == 0xFFFFFFFF)
        {
            status = GetLastError();
        }
    }
    return(status);
}

/******************************************************************************
**
**  Name:  EXtlid - returns the value of Extlsidentifier
**
**  Description:
**
**  Inputs:
**
**  Returns:    value of Extlsidentifier
**
******************************************************************************/

DWORD
EXtlid ()
{
    return(Extlsidentifier);
}

/******************************************************************************
**
**  Name:  EXtlPut - Puts a pointer into the thread local storage directory
**
**  Description: Check that the thread local storage directory entry is not
**               set and assign the tlsitem into it.
**
**  Inputs:
**           tlsid      value returned by EXtlid.
**           tlitem     pointer to the item to be stored.
**           size       size of item to be stored.        (for future)
**
**  Returns: OK         Item is stored
**           FAIL       Item could not be stored.
**
******************************************************************************/

STATUS
EXtlput (DWORD tlsid, PTR tlsitem, u_i4 size)
{

STATUS status = FAIL;

    if(tlsid != 0xFFFFFFFF)
    {   /* if the tls identifier is valid */
        if (TlsGetValue(tlsid) == NULL)
        {   /* if no value stored already */
            if ((status = GetLastError()) == NO_ERROR)
            { /* is it truely empty */
                if (TlsSetValue(tlsid, (LPVOID) tlsitem) != FALSE)
                {
                    status = OK;
                }
            }
        }
    }
    return(status);
}

/******************************************************************************
**
**  Name:  EXtlget - gets a pointer into the thread local storage directory
**
**  Description: Check that the thread local storage directory entry is not
**               set and assign the tlsitem into it.
**
**  Inputs:
**           tlsid      value returned by EXtlid.
**           tlitem     pointer to the item to be retrieved.
**
**  Returns: OK         Item is retrieved
**           FAIL       Item could not be retrieved.
**
******************************************************************************/

STATUS
EXtlget (DWORD tlsid, PTR * tlsitem)
{

STATUS status = FAIL;

    if(tlsid != 0xFFFFFFFF)
    {   /* if the tls identifier is valid */
        if ((* tlsitem = TlsGetValue(tlsid)) == NULL)
        {
            if ((status = GetLastError()) == NO_ERROR)
            { /* is it truely empty */
                status = OK;
            }
        }
        else
        {
            status = OK;
        }
    }
    return(status);
}

/******************************************************************************
**
**  Name:  EXtlrelease - returns the thread local storage directory entry to
**                       system
**
**  Description:
**
**  Inputs:
**           tlsid      value returned by EXtlid.
**           tlitem     pointer to the item to be released. (for future)
**
**  Returns: OK         Item is released
**           FAIL       Item could not be released.
**
******************************************************************************/

STATUS
EXtlrelease (DWORD tlsid, PTR tlsitem)
{
    return( (TlsFree(tlsid) == TRUE) ? OK : FAIL);
}

/*
**
** Name:    ex_print_error() -  print ex message to errlog.log
**
** Description:
**      Function passed to CS_dump_stack to print message to errlog.log.
**      Called through TRformat hence first argument is a dummy for us.
**
** Inputs:
**      arg1                    ignored.
**      msg_length              length of message.
**      msg_buffer              bufer for message.
**
** Returns:
**      void
**
** History:
**      13-dec-1993 (andys)
**              Created.
**      31-dec-1993 (andys)
**              Add ER_ERROR_MSG parameter to ERsend.
*/
static STATUS
ex_print_error(PTR arg1, i4 msg_length, char * msg_buffer)
{
    CL_ERR_DESC err_code;

    ERsend(ER_ERROR_MSG, msg_buffer, msg_length, &err_code);
    return (OK);
}
