/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<ex.h>
# include	<te.h>
# include	<si.h>
# include	"telocal.h"

/*{
** Name:	TEopen	- Initialize the terminal.
**
** Description:
**	Open up the terminal associated with this user's process.  Get the
**	speed of the terminal and use this to return a delay constant.
**	Initialize the buffering structures for terminal I/O.  TEopen()
**	saves the state of the user's terminal for possible later use
**	by TErestore(TE_NORMAL).  Additionally, return the number of
**	rows and columns on the terminal.  For all environments but DG,
**	these sizes will presently be 0, indicating that the values
**	obtained from the termcap file are to be used.  Returning a
**	negative row or column value also indicates use of the values
**	from the termcap file, but additionally causes use of the
**	returned size's absolute value as the number of rows (or columns)
**	to be made unavailable to the forms runtime system.  Both the
**	delay_constant and the number of rows and columns are members
**	of a TEENV_INFO structure.
**
** Inputs:
**	None.
**
** Outputs:
**	envinfo		Must point to a TEENV_INFO structure.
**		.delay_constant	Constant value used to determine delays on
**				terminal updates, based on transmission speed.
**		.rows		Indicate number of rows on the terminal screen;
**				or no information if 0; or number of rows to
**				reserve from the forms system if negative.
**		.cols		Indicate number of cols on the terminal screen;
**				or no information if 0; or number of cols to
**				reserve from the forms system if negative.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	10/83 - (mmm)
**		written
**	8/85  - (joe)
**		Changed from using stdin to using IIte_fpin
**	85/10/02  17:08:36  daveb
**		Don't set GT here; should be done by termdr.
**	85/12/12  15:54:29  shelby
**		Fixed delay constant so that it defaults to DE_B9600.
**		This fixes Gould's 19.2 baud rate problem.
**	86/03/26  22:59:45  bruceb
**		enable the EXTA baud rate--usable provided it is >=9600 baud.
**	86/06/30  15:51:15  boba
**		Change WECO to TERMIO.
**	14-oct-87 (bab)
**		Changed parameter to TEopen to be a pointer to a TEENV_INFO
**		struct rather than a i4  *.  Code changed to match, including
**		setting the row, column members of the struct to 0.
**
**		29-Oct-86 nancy
**		enable the EXTB baud rate
**
**	14-apr-1989 (fredv)
**		Replaced TERMIO by xCL_018_TERMIO_EXISTS.
**		Removed inclusion of <os.h>.
**	12/26/89 (dkh) - Changed to check stdout instead of stderr on
**			 startup.  Also added getting window size to
**			 better support terminal emulators.
**	11-mar-91 (rudyw)
**		Add odt_us5 #ifdef section to #include additional header files
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	22-Feb-1995 (canor01)
**	    added support for HP non-standard baud rates
**	10-may-1999 (walro03)
**	    Remove obsolete version string odt_us5.
**	01-jul-1999 (toumi01)
**		Move IIte_fpin = stdin init to run time, avoiding compile error
**		under Linux glibc 2.1: "initializer element is not constant".
**		The GNU glibc 2.1 FAQ claims that "a strict reading of ISO C"
**		does not allow constructs like "static FILE *foo = stdin" for
**		stdin/stdout/stderr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Apr-2005 (hanje04)
**	    B900 not defined on Mac OS X.
**	06-May-2005 (hanje04)
**	    As part of Mac OSX port, add support for termios tcgetatt() etc
**	    as much of termio()/ioctl() stuff has been depricated.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
*/


static bool	TExtabs();
static bool	TEupcase();
extern bool	TEisa();
static STATUS	TEdelay();
VOID	TEwinsize();
GLOBALDEF FILE	*IIte_fpin = NULL;


STATUS
TEopen(envinfo)
TEENV_INFO	*envinfo;
{
	STATUS	ret_val;

	/* For the BT M6000, flush the terminal... */
#ifdef btf_us5
	ioctl(stdout, TCFLSH, 1);
#endif

	/* disable interrupts to make TEopen() and TEclose()
	** atomic actions.
	*/
	EXinterrupt(EX_OFF);

	UPCSTERM = TEupcase();

	envinfo->rows = 0;
	envinfo->cols = 0;

	for (;;)
	{
		/* this loop is only executed once */

		if (!TEisa(stdout))
		{
			ret_val = OK;
			envinfo->delay_constant = DE_B9600;
			break;
		}

		if (ret_val = TEsave())
		{
			break;
		}

		if (ret_val = TEdelay(&(envinfo->delay_constant)))
		{
			break;
		}

		if (ret_val = TErestore(TE_FORMS))
		{
			break;
		}

		TEwinsize(envinfo);

		ret_val = OK;

		break;
	}

	/* allow interrupts */
	EXinterrupt(EX_ON);

	return(ret_val);
}


/*
** Tests whether I/O stream 'stream' is a tty or not.
** We first find the file descriptor of the stream
** and use this value to check if 'stream' is a tty or not.
** Will return TRUE if 'stream' is a tty, otherwise
** FALSE will be returned.
*/

bool
TEisa(stream)
FILE	*stream;
{
	if (isatty(fileno(stream)) == 1)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

# if defined(xCL_018_TERMIO_EXISTS) || defined(xCL_TERMIOS_EXISTS)
/* u3b5 Terminal I/O compatibility routines */

/*
** Terminal I/O Compatibility library for u3b5 .
**
** Created 2-15-83 (dkh)
**
** Note:
**	The file descriptor for 'stdout' is 1 on 4.2 Berkeley UNIX.
**	This may not be true on other systems.  To change the
**	file descriptor for stdout simply put in a new value
**	for STDOUTFD in "TElib.h".
*/

# include	<st.h>



/*
** TEdelay
**	Places into "delay" what delay it thinks the terminal needs for
**	which speed it thinks the terminal is running at.
**	It is assumed that input and output speeds are the
**	same.  If this routine can not get the speed information
**	or the speed is not what it expects then
**	it will return FAIL and set delay to TE_BNULL.
**
**
** Returns:
**	OK - if things were successful.
**	FAIL - if something went wrong.
**
** Note:
**	We will have more speeds defined than what UNIX normally
**	supports because VMS supports more speeds.
**
**	EXTA is being supported as a speed >= 9600 baud.  If this is
**	false on a given machine, users of RTI products can not set
**	their baud rate to EXTA and still expect everything to work.
**
**	HP supports some non-standard baud rates that it defines with
**	a prepended underscore (e.g. _B900, _B3600, _B460800).
**
**	Some machines use EXTA and EXTB for B19200 and B38400
**	respectively, but others do not.
*/

static STATUS
TEdelay(delay)
i4	*delay;
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
		*delay = TE_BNULL;
		return(FAIL);
	}
/* Check termial speed */
# ifdef CBAUD
	switch(_tty.c_cflag & CBAUD)
# else
	switch(cfgetispeed( &_tty ))
# endif
	{
		case B50:
			*delay = DE_B50;
			break;
		case B75:
			*delay = DE_B75;
			break;
		case B110:
			*delay = DE_B110;
			break;
		case B134:
			*delay = DE_B134;
			break;
		case B150:
			*delay = DE_B150;
			break;
		case B200:
			*delay = DE_B200;
			break;
		case B300:
			*delay = DE_B300;
			break;
		case B600:
#ifdef _B900
		case _B900:
#endif /* _B900 */
			*delay = DE_B600;
			break;
		case B1200:
			*delay = DE_B1200;
			break;
		case B1800:
			*delay = DE_B1800;
			break;
		case B2400:
#ifdef _B3600
		case _B3600:
#endif /* _B3600 */
			*delay = DE_B2400;
			break;
		case B4800:
#ifdef _B7200
		case _B7200:
#endif /* _B7200 */
			*delay = DE_B4800;
			break;
		case B9600:
#if defined(B19200) && (B19200 != EXTA)
		case B19200:
#endif /* B19200 && (B19200 != EXTA) */
#if defined(B38400) && (B38400 != EXTB)
		case B38400:
#endif /* B38400 && (B38400 != EXTB) */
#ifdef _B57600
		case _B57600:
#endif /* _B57600 */
#ifdef _B115200
		case _B115200:
#endif /* _B115200 */
#ifdef _B230400
		case _B230400:
#endif /* _B230400 */
#ifdef _B460800
		case _B460800:
#endif /* _B460800 */
		case EXTA:
		case EXTB:
			*delay = DE_B9600;
			break;
		default:
			*delay = DE_ILLEGAL;
			return(FAIL);
	}
	return(OK);
}

/*
** TEupcase
**	Returns "True" if the user terminal is an upper
**	case only terminal.  "False" otherwise.
**
*/

static bool
TEupcase()
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
		return(FALSE);
	}
# if defined(IUCLC) && defined(OLCUC)
	if (_tty.c_iflag & IUCLC || _tty.c_oflag & OLCUC)
	{
		return(TRUE);
	}
	else
# endif
	{
		return(FALSE);
	}
}

/*
** TExtabs
**	Returns "FALSE" if the user terminal causes TABS to be expanded
**	by the appropriate amount of spaces on output.
**	"TRUE" otherwise.
**	Should return FALSE if on VMS.
**
*/

static bool
TExtabs()
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
		return(FALSE);
	}
# ifdef TAB3
	if (_tty.c_oflag & TAB3)
	{
		return(FALSE);
	}
	else
# endif
	{
		return(TRUE);
	}
}



/*{
** Name:	TEwinsize - Get window size of controlling tty on stdout.
**
** Description:
**	This routine finds out the window size of the controlling tty
**	pointed to by stdout (by using TIOCGWINSZ) and sets the
**	appropriate members of the passed in TEENV_INFO structure.
**
**	We assume that we get a zero if stdout is not a tty or that
**	TIOCGWINSZ is not really supported, even though it is defined.
**
**	Nothing is updated if TIOCGWINSZ is not supported or defined
**	in the system headers.
**
** Inputs:
**	None.
**
** Outputs:
**	envinfo
**		.rows		The number of rows in controlling terminal.
**		.columns	The number of columns in controlling terminal.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/26/89 (dkh) - Initial version.
*/
VOID
TEwinsize(envinfo)
TEENV_INFO	*envinfo;
{
# ifdef TIOCGWINSZ
	struct winsize	win_size;
	
	if (ioctl(STDOUTFD, TIOCGWINSZ, &win_size) == 0)
	{
		envinfo->rows = win_size.ws_row;
		envinfo->cols = win_size.ws_col;
	}
# endif /* TIOCGWINSZ */
}

# else /* !xCL_018_TERMIO_EXISTS || !xCL_97_TERMIOS_EXISTS */


/*
** TEdelay
**	Places into "delay" what delay it thinks the terminal needs for
**	which speed it thinks the terminal is running at.
**	It is assumed that input and output speeds are the
**	same.  If this routine can not get the speed information
**	or the speed is not what it expects then
**	it will return FAIL and set delay to TE_BNULL.
**
**
** Returns:
**	OK - if things were successful.
**	FAIL - if something went wrong.
**
** Note:
**	We will have more speeds defined than what UNIX normally
**	supports because VMS supports more speeds.
**
**	EXTA is being supported as a speed >= 9600 baud.  If this is
**	false on a given machine, users of RTI products can not set
**	their baud rate to EXTA and still expect everything to work.
**
**	HP supports some non-standard baud rates that it defines with
**	a prepended underscore (e.g. _B900, _B3600, _B460800).
**
**	Some machines use EXTA and EXTB for B19200 and B38400
**	respectively, but others do not.
*/


static STATUS
TEdelay(delay)
i4	*delay;
{
	struct sgttyb	_tty;
	if (gtty(STDOUTFD,&_tty) != 0)
	{
		*delay = DE_ILLEGAL;
		return(FAIL);
	}
	switch(_tty.sg_ispeed)
	{
		case B50:
			*delay = DE_B50;
			break;
		case B75:
			*delay = DE_B75;
			break;
		case B110:
			*delay = DE_B110;
			break;
		case B134:
			*delay = DE_B134;
			break;
		case B150:
			*delay = DE_B150;
			break;
		case B200:
			*delay = DE_B200;
			break;
		case B300:
			*delay = DE_B300;
			break;
		case B600:
# ifdef B900
		case B900:
# endif
			*delay = DE_B600;
			break;
		case B1200:
			*delay = DE_B1200;
			break;
		case B1800:
			*delay = DE_B1800;
			break;
		case B2400:
#ifdef _B3600
		case _B3600:
#endif /* _B3600 */
			*delay = DE_B2400;
			break;
		case B4800:
#ifdef _B7200
		case _B7200:
#endif /* _B7200 */
			*delay = DE_B4800;
			break;
		case B9600:
#if defined(B19200) && (B19200 != EXTA)
		case B19200:
#endif /* B19200 && (B19200 != EXTA) */
#if defined(B38400) && (B38400 != EXTB)
		case B38400:
#endif /* B38400 && (B38400 != EXTB) */
#ifdef _B57600
		case _B57600:
#endif /* _B57600 */
#ifdef _B115200
		case _B115200:
#endif /* _B115200 */
#ifdef _B230400
		case _B230400:
#endif /* _B230400 */
#ifdef _B460800
		case _B460800:
#endif /* _B460800 */
		case EXTA:
		case EXTB:
			*delay = DE_B9600;
			break;
		default:
			*delay = DE_ILLEGAL;
			return(FAIL);
	}
	return(OK);
}

/*
** TEupcase
**	Returns "True" if the user terminal is an upper
**	case only terminal.  "False" otherwise.
**
*/

static bool
TEupcase()
{
	struct	sgttyb	_tty;
	if (gtty(STDOUTFD,&_tty) != 0)
	{
		return(FALSE);
	}
	if (_tty.sg_flags & LCASE)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}


/*
** TExtabs
**	Returns "FALSE" if the user terminal causes TABS to be expanded
**	by the appropriate amount of spaces on output.
**	"TRUE" otherwise.
**	Should return FALSE if on VMS.
**
*/

static bool
TExtabs()
{
	struct	sgttyb	_tty;
	if (gtty(STDOUTFD,&_tty) != 0)
	{
		return(FALSE);
	}
	if (_tty.sg_flags & XTABS)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}


/*{
** Name:	TEwinsize - Get window size of controlling tty on stdout.
**
** Description:
**	This routine finds out the window size of the controlling tty
**	pointed to by stdout (by using TIOCGWINSZ) and sets the
**	appropriate members of the passed in TEENV_INFO structure.
**
**	We assume that we get a zero if stdout is not a tty or that
**	TIOCGWINSZ is not really supported, even though it is defined.
**
**	Nothing is updated if TIOCGWINSZ is not supported or defined
**	in the system headers.
**
**	FOR NOW, THIS VERSION OF THE ROUTINE DOES NOTHING SINCE I DON'T
**	HAVE ANY DOCUMENTATION FOR NON-TERMIO SYSTEMS AT THE MOMENT.
**
** Inputs:
**	None.
**
** Outputs:
**	envinfo
**		.rows		The number of rows in controlling terminal.
**		.columns	The number of columns in controlling terminal.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/26/89 (dkh) - Initial version.
*/
VOID
TEwinsize(envinfo)
TEENV_INFO	*envinfo;
{
}
# endif	/* if xCL_018_TERMIO_EXISTS else endif */
