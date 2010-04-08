/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<te.h>
# include	"telocal.h"

GLOBALDEF bool	TEcmdEnabled = FALSE;
GLOBALDEF bool	TEstreamEnabled = FALSE;

static STATUS	TErare(),
		TEnoecho(),
                TEechoon(),
                TEechooff(),
		TEset(),
		TEcmdmode();

# if defined( xCL_018_TERMIO_EXISTS ) || defined( xCL_TERMIOS_EXISTS )

/*
** TErestore
**	Restore terminal characteristics.
**	
**	Restore the terminal to either forms or normal
**	state depending on the value of "which" (TE_NORMAL
**	or TE_FORMS).
**	This will be a no-op for VMS.
**
** Arguments:
**	i1	which;	
** Returns:
**	OK - if the terminal state was restored.
**	FAIL - if we could not restored the terminal state or
**		if there was no previous call to TEsave or
**		if a call to TEsave was unsuccessful.
**
**	History:
**		14-apr-1989 (fredv)
**			Replaced TERMIO by xCL_018_TERMIO_EXISTS.
**		12/26/89 (dkh) - Changed to probe stdout instead of stderr.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**		18-may-1995 (harpa06)
**			Cross-Integration of bug 49935 by prida01 - 
**			 Add echoon and echo off for stdin 49935
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2001 (hanje04)
**	    Replaced VMIN and VTIME with TE_VMIN & TE_VTIME to impliment
**	    fix for bug 103708
**	17-aug-2001 (somsa01 for fanra01)
**	    SIR 104700
**	    Add handling of new TE_IO_MODE for direct streams access.
**      24-Feb-2002 (hanch04)
**	    Removed TEcmdmode call for TE_IO_MODE
**	06-May-2005 (hanje04)
**	    As part of Mac OSX port, add support for termios tcgetatt() etc
**	    as much of termio()/ioctl() stuff has been depricated.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
*/

/*
** The following buffer is for saving the terminal state
** information so we can restore a user's terminal after
** our front-ends exit.  It is declared here so only
** the routines TEsave and TErestore will be able to
** use it.  Boolean TEsaveok is a flag indicating that
** a called to TEsave was successful.
*/
#ifdef xCL_TERMIOS_EXISTS
static	struct termios	TE_ttybuf;
# else
static	struct termio	TE_ttybuf;
# endif
static	bool		TEsaveok = FALSE;


STATUS
TErestore(which)
i4	which;
{
	STATUS		ret_val = OK;


	if (which == TE_NORMAL)
	{
		if (!TEsaveok)
		{
			/* can not restore a terminal that has not
			** been saved yet.  This save happens in
			** TEopen()
			*/
			ret_val = FAIL;
		}
# ifdef TCSETAW
		else if (ioctl(STDOUTFD,TCSETAW,&TE_ttybuf) != 0)
# else
		else if ( tcsetattr(STDOUTFD, TCSADRAIN, &TE_ttybuf) != 0)
# endif
		{
			ret_val = FAIL;	
		}
		TEcmdEnabled = FALSE;
		TEstreamEnabled = FALSE;
	}
	else if (which == TE_FORMS)
	{
		/* set the terminal to forms state */

		if ((TEset(~TECRMOD))	||	/* set ~CRMOD */
		    (TErare())		||
		    (TEnoecho()))
		{
			ret_val = FAIL;
		}
		TEcmdEnabled = FALSE;
		TEstreamEnabled = FALSE;
	}
        else if (which == TE_ECHOOFF)
        {
                if (TEechooff())
                {
                        ret_val = FAIL;
                }
        }
        /* added for bug 49935 if stdout is redirected to a file */
        else if (which == TE_ECHOON)
        {
                if (TEechoon())
                {
                         ret_val = FAIL;
                }
        }
	else if (which == TE_IO_MODE)
	{
		TEcmdEnabled = TRUE;
		TEstreamEnabled = TRUE;
	}
	else if (which == TE_CMDMODE)
	{
		if (TEcmdmode())
		{
			ret_val = FAIL;
		}
		TEcmdEnabled = TRUE;
	}
	else
	{
		/* Bad "which" passed in */
		ret_val = FAIL;
	}

	return(ret_val);
}





/*
** TEsave
**	Saves the terminal state so we can restore it
**	when the user exits from one of the front-ends.
**	This will be a no-op for VMS.
**
**	This routine is internal to the TE routines and
**	is not to be used outside the compatibility library.
**
** Returns:
**	OK - if the terminal state was saved.
**	FAIL - if we could not save the terminal state.
*/

STATUS
TEsave()
{
# ifdef TCGETA
	if (ioctl(STDOUTFD,TCGETA,&TE_ttybuf) != 0)
# else
	if (tcgetattr(STDOUTFD,&TE_ttybuf) != 0)
# endif
	{
		return(FAIL);
	}
	else
	{
		TEsaveok = TRUE;
		return(OK);
	}
}





/*
** Set Terminal (i.e., stdin) to rare state.
** Rare state allows most characters to pass through unchecked, but still
** allows job control, Control-Q/S, and a few other things.
** Will return OK when it has been successfully accomplished.
** Fail will be returned in all other cases.
*/


static STATUS
TErare()
{


# ifdef TCGETA
        struct  termio  _tty;
        if (ioctl(STDOUTFD,TCGETA,&_tty) != 0)
# else
        /* tcgetattr takes termios not termio */
        struct  termios _tty;
        if (tcgetattr(STDOUTFD,&_tty) != 0)
# endif
	{
		return(FAIL);
	}

	_tty.c_lflag &= ~ICANON;
	_tty.c_iflag &= ~INLCR;
	_tty.c_iflag &= ~ICRNL;
	_tty.c_cc[TE_VMIN] = 1;
	_tty.c_cc[TE_VTIME] = 1;

# ifdef TCSETAW
	if (ioctl(STDOUTFD,TCSETAW,&_tty) != 0)
# else
	if ( tcsetattr(STDOUTFD, TCSADRAIN, &_tty) != 0)
# endif
	{
		return(FAIL);
	}

	return(OK);
}



/*
** Set terminal (i.e., stdout) to noecho state.
** Don't echo any characters the user types in.
** Will return OK when it has been successfully accomplished. 
** Fail wiil be returned in all other cases.
*/

static STATUS
TEnoecho()
{
# ifdef TCGETA
        struct  termio  _tty;
        if (ioctl(STDOUTFD,TCGETA,&_tty) != 0)
# else
        /* tcgetattr takes termios not termio */
        struct  termios _tty;
        if (tcgetattr(STDOUTFD,&_tty) != 0)
# endif
	{
		return(FAIL);
	}
	_tty.c_lflag &= ~ECHO;
# ifdef TCSETAW
	if (ioctl(STDOUTFD,TCSETAW,&_tty) != 0)
# else
	if ( tcsetattr(STDOUTFD, TCSADRAIN, &_tty) != 0)
# endif
	{
		return(FAIL);
	}
	return(OK);
}

/*
** We need this 'echooff', because when stdout is redirected to a
** file the TCGETA fails. For passwords this causes the password to be
** echoed. We turn echo off stdin as well to cover this situation
*/
static STATUS
TEechooff()
{
# ifdef TCGETA
        struct  termio  _tty;
        if (ioctl(STDINFD,TCGETA,&_tty) != 0)
# else
        /* tcgetattr takes termios not termio */
        struct  termios _tty;
        if (tcgetattr(STDINFD,&_tty) != 0)
# endif
        {
                return(FAIL);
        }
        _tty.c_lflag &= ~ECHO;
# ifdef TCSETAW
	if (ioctl(STDINFD,TCSETAW,&_tty) != 0)
# else
	if ( tcsetattr(STDINFD, TCSADRAIN, &_tty) != 0)
# endif
        {
                return(FAIL);
        }
        return(OK);
}
 
/*
** We need this extra 'echoon', because when stdout is redirected to a
** file the TCGETA fails. For passwords this causes the password to be
** echoed. We turn echo off stdin as well to cover this situation
*/
static STATUS
TEechoon()
{
# ifdef TCGETA
        struct  termio  _tty;
        if (ioctl(STDINFD,TCGETA,&_tty) != 0)
# else
        /* tcgetattr takes termios not termio */
        struct  termios _tty;
        if (tcgetattr(STDINFD,&_tty) != 0)
# endif
        {
                return(FAIL);
        }
        _tty.c_lflag |= ECHO;
# ifdef TCSETAW
	if (ioctl(STDINFD,TCSETAW,&_tty) != 0)
# else
	if ( tcsetattr(STDINFD, TCSADRAIN, &_tty) != 0)
# endif
        {
                return(FAIL);
        }
        return(OK);
}
 
 
/*
** TEset
**	Set some sort of terminal characteristic
**	Right now the only characteristic that can be set is
**	CRMOD.  If routine does not know what you
**	want to set it will simply return FAIL.
**
** Arg:
**	mode - characteristic that you want to set
**
*/

static STATUS
TEset(mode)
i4	mode;
{
# ifdef TCGETA
        struct  termio  _tty;
        if (ioctl(STDOUTFD,TCGETA,&_tty) != 0)
# else
        /* tcgetattr takes termios not termio */
        struct  termios _tty;
        if (tcgetattr(STDOUTFD,&_tty) != 0)
# endif
	{
		return(FAIL);
	}
	switch (mode)
	{
		case TECRMOD:
			_tty.c_oflag |= ONLCR;
			break;
		case ~TECRMOD:
			_tty.c_oflag &= ~ONLCR;
			break;
		default:
			return(FAIL);
	}
# ifdef TCSETAW
	if (ioctl(STDOUTFD,TCSETAW,&_tty) != 0)
# else
	if ( tcsetattr(STDOUTFD, TCSADRAIN, &_tty) != 0)
# endif
	{
		return(FAIL);
	}
	return(OK);
}



/*
** Set terminal (i.e., stdout) for command mode
** to communicate with another computer.
*/

static STATUS
TEcmdmode()
{
# ifdef TCGETA
        struct  termio  _tty;
        if (ioctl(STDOUTFD,TCGETA,&_tty) != 0)
# else
        /* tcgetattr takes termios not termio */
        struct  termios _tty;
        if (tcgetattr(STDOUTFD,&_tty) != 0)
# endif
	{
		return(FAIL);
	}
	_tty.c_iflag = IXON|IXOFF;
	_tty.c_oflag &= ~OPOST;
	_tty.c_cflag |= CS8|CREAD|CLOCAL;
	_tty.c_cflag &= ~PARENB;
	_tty.c_lflag = ISIG;
# ifdef TCGETA
	if (ioctl(STDOUTFD,TCSETA,&_tty) != 0)
# else
        /* tcsetattr takes termios not termio */
	if (tcsetattr(STDOUTFD, TCSANOW, &_tty) != 0)
# endif
	{
		return(FAIL);
	}
	return(OK);
}



# else		/* Right now we have WECO and everyone else */


/*
** TErestore
**	Restore terminal characteristics.
**	
**	Restore the terminal to either forms or normal
**	state depending on the value of "which" (TE_NORMAL
**	or TE_FORMS).
**	This will be a no-op for VMS.
**
** Arguments:
**	i1	which;	
** Returns:
**	OK - if the terminal state was restored.
**	FAIL - if we could not restored the terminal state or
**		if there was no previous call to TEsave or
**		if a call to TEsave was unsuccessful.
*/

/*
** The following buffer is for saving the terminal state
** information so we can restore a user's terminal after
** our front-ends exit.  It is declared here so only
** the routines TEsave and TErestore will be able to
** use it.  Boolean TEsaveok is a flag indicating that
** a called to TEsave was successful.
*/

static	struct sgttyb	TE_ttybuf;
static	bool	TEsaveok = FALSE;


STATUS
TErestore(which)
i1	which;
{
	STATUS		ret_val = OK;
	STATUS	TErare();
	STATUS	TEnoecho();
	STATUS	TEset();

	if (which == TE_NORMAL)
	{
		if (!TEsaveok)
		{
			/* can not restore a terminal that has not
			** been saved yet.  This save happens in
			** TEopen()
			*/
			ret_val = FAIL;
		}
		else if (stty(STDOUTFD,&TE_ttybuf) != 0)
		{
			ret_val = FAIL;
		}

	}
	else if (which == TE_FORMS)
	{
		/* set the terminal to forms state */

		if ((TEset(~TECRMOD))	||	/* set ~CRMOD */
		    (TErare())		||
		    (TEnoecho()))
		{
			ret_val = FAIL;
		}

	}
	else
	{
		/* Bad "which" passed in */
		ret_val = FAIL;
	}

	return(ret_val);
}




/*
** TEsave
**	Saves the terminal state so we can restore it
**	when the user exits from one of the front-ends.
**	This will be a no-op for VMS.
**
**	This routine is internal to the TE routines and
**	is not to be used outside the compatibility library.
**
** Returns:
**	OK - if the terminal state was saved.
**	FAIL - if we could not save the terminal state.
*/

STATUS
TEsave()
{
	if (gtty(STDOUTFD,&TE_ttybuf) != 0)
	{
		return(FAIL);
	}
	else
	{
		TEsaveok = TRUE;
		return(OK);
	}
}



/*
**
** Set Terminal (i.e., stdin) to rare state.
** Rare state allows most characters to pass through unchecked, but still
** allows job control, Control-Q/S, and a few other things.
** Will return OK when it has been successfully accomplished.
** Fail will be returned in all other cases.
*/

static STATUS
TErare()
{
	struct	sgttyb	_tty;


	if (gtty(STDOUTFD,&_tty) != 0)
	{
		return(FAIL);
	}

	_tty.sg_flags |= CBREAK;

	if (stty(STDOUTFD,&_tty) != 0)
	{
		return(FAIL);
	}

	return(OK);
}



/*
** Set terminal (i.e., stdout) to noecho state.
** Don't echo any characters the user types in.
** Will return OK when it has been successfully accomplished. 
** Fail wiil be returned in all other cases.
*/

static STATUS
TEnoecho()
{
	struct	sgttyb	_tty;


	if (gtty(STDOUTFD,&_tty) != 0)
	{
		return(FAIL);
	}

	_tty.sg_flags &= ~ECHO;

	if (stty(STDOUTFD,&_tty) != 0)
	{
		return(FAIL);
	}

	return(OK);
}




/*
** TEset
**	Set some sort of terminal characteristic
**	Right now the only characteristic that can be set is
**	CRMOD.  If routine does not know what you
**	want to set it will simply return FAIL.
**
** Arg:
**	mode - characteristic that you want to set
**
*/
static STATUS
TEset(mode)
i4	mode;
{
	struct	sgttyb	_tty;
	if (gtty(STDOUTFD,&_tty) != 0)
	{
		return(FAIL);
	}
	switch (mode)
	{
		case TECRMOD:
			_tty.sg_flags |= CRMOD;
			break;
		case ~TECRMOD:
			_tty.sg_flags &= ~CRMOD;
			break;
		default:
			return(FAIL);
	}
	if (stty(STDOUTFD,&_tty) != 0)
	{
		return(FAIL);
	}
	return(OK);
}

# endif
