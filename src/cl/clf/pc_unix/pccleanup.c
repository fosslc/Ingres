# include 	<compat.h>	/* compatability library header */
# include	<systypes.h>
# include	<pc.h>
# include	<qu.h>

# include	<clconfig.h>

# include	<errno.h>

# include	<clsigs.h>
# include	<ex.h>

# include	<PCerr.h>
# include	"pclocal.h"
# include	"pcpidq.h"

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		pccleanup.c
**
**	Function:
**		PCcleanup -- search for subprocesses, terminate them 
**
**	Arguments:
**		none.
**
**	Result:
**		subprocesses are terminated by sending SIGKILL
**		to all processes in current process group.
**		For safety, use only from front-ends.
**		Use of this function is dependent on
**		EX_CLEANUP being defined in cl!hdr!hdr!ex.h 
**	Returns:
**		none
**
**	Side Effects:
**		none
**
**	History:
**	5-dec-1996 (angusm) Copied partly from pcreap.c.
**			    See also "Advanced Programming in the
**			    UNIX environment" by WR Stevens, sections
**			    8.3, 8.5, 9.4, 9.5, 10.9
**	29-aug-1997 (canor01)
**	    Change "ifdef 0" to "if 0", since former will cause
**	    compile errors on some platforms.
*/



void
PCcleanup(except)
EX	except;
{


    extern QUEUE	Pidq;
    register PIDQUE	*sqp;		/* search queue pointer */
    register STATUS	rval;
    EX			sendex;

    TYPESIG (*inthandler)();
    TYPESIG (*quithandler)();

    /* 
    ** If the Pidq doesn't exist, no point waiting for the child 
    ** since we don't care anyway.
    */
    if(Pidq_init == FALSE)
	return;

    /*
    ** Disable the SIGINT and SIGQUIT, and save their old actions.
    ** EXinterrupt(EX_OFF) now latches incoming keyboard interrupts,
    ** rather than discarding them; we need to actually turn the
    ** signal handling off.
    */
    inthandler = signal(SIGINT, SIG_IGN);
    quithandler = signal(SIGQUIT, SIG_IGN);

    /* Search for the PID in the pid queue 
    */
    sendex = SIGKILL;

    for (sqp = (PIDQUE *) Pidq.q_next; 
		sqp != (PIDQUE *) &Pidq;
		sqp = (PIDQUE *) sqp->pidq.q_next )
    {
	rval = PCsendex(sendex, 0); 
    }
    
    /* reset interrupt and quit signals */

    (void) signal(SIGINT, inthandler);
    (void) signal(SIGQUIT, quithandler);


}
#if 0
STATUS
PCsetpcgrp()
{
	STATUS rval, PCstatus;
	PID	tpid = PCpid();

	rval = setpgid(tpid, tpid);

	PCstatus = OK;
	if (rval == -1)
		PCstatus = FAIL;

return(PCstatus);

}
#endif
