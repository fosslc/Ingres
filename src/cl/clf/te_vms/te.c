/*
** Copyright (c) 1983, 2008 Ingres Corporation
**
*/
# include	<compat.h>
# include	<gl.h>
# include	<dcdef.h>
# include	<dvidef.h>
# include	<efndef.h>
# include	<iledef.h>
# include	<iosbdef.h>
# include	<jpidef.h>
# include	<iodef.h>
# include	<ssdef.h>
# include	<descrip.h>
# include	<lib$routines.h>
# include	<starlet.h>
# include	<ttdef.h>
# include	<te.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<tc.h>
# include	<nm.h>
# include	<cv.h>
# include       <astjacket.h>
/*
**
**	TE.C	- Terminal I/O Compatiblity Library for VMS
**
**  History:
**	10/14/87 (dkh) - Added new entry point "TEinflush".
**	15-oct-87 (bab)
**		Addition of TElock.  Modification of TEopen to meed
**		new spec; it, and calls on it (see TEgbf) now pass
**		a pointer to a TEENV_INFO struct instead of a i4 *.
**	02/24/88 (dkh) - Fixed jup bug 1995 by integrating fix to bug
**			 12007 from 5.0.
**	12/01/88 (dkh) - Added support for timeout.
**	05/12/89 (Eduardo) - Added support for SEP tool
**	05/22/89 (Eduardo) - Modified TEtest
**	12/26/89 (dkh) - Put in changes to not split escape sequences.
**			 This is done by expanding the output buffer to
**			 1024 and having termdr not write anything bigger
**			 than this when playing with escape sequences.
**			 Using a buffer size of 1024 is based on the
**			 fact that the "MAXBUF" SYSGEN parameter on VMS
**			 has a low limit value that is greater than 1024.
**			 This ensures that a qio call will be somewhat
**			 atomic.  Note that we still use a size of 255
**			 when writing output to a file (i.e., in test mode).
**	07/23/90 (dkh) - Put in fix to make SEP work again.  Previous changes
**			 to allow TEopen and TEtest to be called in any order
**			 broke SEP.
**	11/04/90 (dkh) - Changed datatype of te_in_ptr, te_out_buffer and
**			 te_in_buffer from char to u_char to support 8 bit
**			 input.
**	11/20/90 (dkh) - Changed return type for TEget() from char to i4.
**			 This not only matches the CL spec but also handles
**			 sign extension correctly for 8 bit input.
**	10/03/92 (dkh) - Rearranged code to get around compiler bug that
**			 can't handle the ? and : operators.
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	    Replace homegrown item lists by ILE3.
**      20-mar-2009 (stegr01)
**          Add include for astjacket.h for posiz threads on VMS Itanium
*/


# define	TE_TOUTSIZE	1024	/* Output buffer size to a terminal */
# define	TE_FOUTSIZE	255	/* Ouptut buffer size to a file */

/* Save area for terminal settings */
struct termio {
	char	class;
	char	type;
	short	buf_size;
	long	basic;		/* uppermost byte contains page length */
	long	extended;
};


static	struct termio	termio = {0};
static	bool		TEsaveok = FALSE;
static	bool		TEcmdEnabled = FALSE;
static	i4		cur_term_mode = TE_FORMS;
static	bool		te_fromgbf = FALSE;
static 	short		te_in_channel = 0;
static	short		te_out_channel = 0;
static	int		te_in_count;
static  int		te_out_count;
static  int		te_in_type = 0;
static  int		te_out_type = 0;
static	u_char		*te_in_ptr;
static	u_char		te_out_buffer[TE_TOUTSIZE];
static	u_char		te_in_buffer[255];
static  FILE		*te_in_test;
static  FILE		*te_out_test;
static  TCFILE		*te_in_comm;
static  TCFILE		*te_out_comm;
static  STATUS		(*te_in_func)() = NULL;
static  STATUS		(*te_out_func)() = NULL;
readonly static int	delay_table[] =
	{ 0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 50, 41, 20, 10, 7, 5, 2 };
static	i4		timeoutsecs = 0;
static	i4		tereadmod = IO$_READVBLK|IO$M_NOECHO|IO$M_NOFILTR;
static	i4		teoutsize = 0;

static  STATUS		TEtestin();
static  STATUS		TEtestout();
static  STATUS		TEin();
static  STATUS		TEout();
static  STATUS          TEsave();
static  STATUS          TEcmdmode();
static	STATUS		TEchkdev();
static	STATUS		TEassdev();



/*
**	TEopen	- initialize the terminal handling routines
**
**	Description:
**		Open up the terminal associated with this user's process.
**		Get the speed of the terminal and use this to return a delay
**		constant.  Initialize the buffering structures for terminal
**		I/O.  TEopen() saves the state of the user's terminal for
**		possible later use by TErestore(TE_NORMAL).  Additionally,
**		return the number of rows and columns on the terminal.  For
**		all environments but DG, these sizes will presently be 0,
**		indicating that the values obtained from the termcap file are
**		to be used.  Returning a negative row or column value also
**		indicates use of the values from the termcap file, but
**		additionally causes use of the returned size's absolute value
**		as the number of rows (or columns) to be made unavailable to
**		the forms runtime system.  Both the delay_constant and the
**		number of rows and columns are members of a TEENV_INFO
**		structure.
**		If the logical names II_TT_INPUT and II_TT_OUTPUT
**		are defined then they are used as input and output.
**
**	Outputs:
**		envinfo		Must point to a TEENV_INFO structure.
**			.delay_constant	Constant value used to determine
**					delays on terminal updates, based on
**					transmission speed.
**			.rows		Indicate number of rows on the
**					terminal screen; or no information if
**					0; or number of rows to reserve from
**					the forms system if negative.
**			.cols		Indicate number of cols on the
**					terminal screen; or no information if
**					0; or number of cols to reserve from
**					the forms system if negative.
*/
STATUS
TEopen(envinfo)
TEENV_INFO	*envinfo;
{
	char				*cp;
	char				in[MAX_LOC];
	char				out[MAX_LOC];

	in[0] = '\0';
	out[0] = '\0';
	NMgtAt("II_TT_INPUT", &cp);
	if (cp != NULL && *cp != '\0')
		STcopy(cp, in);
	NMgtAt("II_TT_OUTPUT", &cp);
	if (cp != NULL && *cp != '\0')
		STcopy(cp, out);
	if (in[0] != '\0' || *out != '\0')
		TEtest(in, TE_SI_FILE, out, TE_SI_FILE);
	envinfo->rows = 0;
	envinfo->cols = 0;
	if (TEopendev(&(envinfo->delay_constant)) == FAIL)
		return(FAIL);
	return(TEsave());
}

STATUS
TEopendev(delay_constant)
i4			*delay_constant;
{
	struct	dsc$descriptor_s	terminal;
	short				terminal_name = 'TT';
	int	astval;
	int	retval;
	int	class;
	int	status;
	int	channel;
	char	buffer[8];
	bool	opening_in = FALSE;
#pragma member_alignment save
#pragma nomember_alignment
	struct	{
		short	status;
		char	txmt_speed;
		char	rcvr_speed;
		char	cr_fill;
		char	lf_fill;
		char	parity;
		char	zero;
	}	tt_iosb;
#pragma member_alignment restore

	terminal.dsc$w_length = 2;
	terminal.dsc$a_pointer = (PTR)&terminal_name;

	/*	assume failure, setup structure used to device type information from VMS	*/

	retval = FAIL;

	/*	disable interrupts to make TEopen() and TEclose() atomic actions	*/

	astval = sys$setast(0);
	for (;;)
	{
		int	status;

		/*	If we already have a terminal open, fail	*/

		if (te_in_func != NULL)
			break;

		te_in_func = (te_in_test == NULL ? TEin : TEtestin);
		te_out_func = (te_out_test == NULL ? TEout : TEtestout);
		teoutsize = (te_out_test == NULL ? TE_TOUTSIZE : TE_FOUTSIZE);
		if (te_in_test && te_out_test)
		{
			*delay_constant = 2;
			retval = OK;
			break;
		}
		/*	get information about device, if can't get it or not terminal, fail	*/

		if (te_in_channel == NULL && te_in_test == NULL)
		{
			opening_in = TRUE;
			if ((te_fromgbf == FALSE) && TEchkdev(&terminal) != OK)
			{
				break;
			}
			else
			{
				if ((retval = TEassdev(&terminal, &te_in_channel)) != OK)
				{
					break;
				}
			}
		}
		te_in_ptr = te_in_buffer;
		te_in_count = 0;
		te_out_count = 0;

		/*	get the baud rate, so we can calculate the delay constant	*/
		if (te_out_channel == NULL && te_out_test == NULL)
		{
			if (opening_in)
			{
				te_out_channel = te_in_channel;
			}
			else
			{
				if (TEchkdev(&terminal) != OK)
				{
					break;
				}
				else
				{
					if ((retval = TEassdev(&terminal, &te_out_channel)) != OK)
					{
						break;
					}
				}
			}
		}

		channel = te_out_channel != NULL ? te_out_channel : te_in_channel;
		status = sys$qiow(EFN$C_ENF, channel, IO$_SENSEMODE, &tt_iosb,
				  0, 0, buffer, 8, 0, 0, 0, 0);
		if (status & 1)
		    status = tt_iosb.status;
		if ((status & 1) == 0)
		{
			sys$dassgn(channel);
			te_in_channel = te_out_channel = 0;
			break;
		}
		if (tt_iosb.txmt_speed == 0 || tt_iosb.txmt_speed > sizeof(delay_table)/sizeof(int))
			*delay_constant = 2;
		else
			*delay_constant = delay_table[tt_iosb.txmt_speed];
		retval = OK;
	}
	if (astval == SS$_WASSET)
		sys$setast(1);
	return (retval);
}

/*
**	TEclose - close the terminal
**
**	Description:
**		Flush the contents of the write buffers, and close the terminal.
*/

STATUS
TEclose()
{
	int	astval;
	int	status;
	int	retval = OK;

	if (te_in_func == NULL)
		return (FAIL);
	astval = sys$setast(0);
	if (te_out_count)
		TEflush();
	status = sys$dassgn(te_in_channel);
	status = sys$dassgn(te_out_channel);
	te_in_channel = 0;
	te_out_channel = 0;
	te_in_func = NULL;
	te_in_test = NULL;
	te_out_test = NULL;
	if ((status & 1) == 0)
		retval = FAIL;
	if (astval == SS$_WASSET)
		sys$setast(1);
	return (retval);
}

/*
**	TEflush	- perform buffer writes to the terminal at this point.
**
**	Description:
**		Flush the write buffer to the terminal.  This is normally called
**		when the buffer is full or when the application decides that the
**		screen should be updated.
*/

STATUS
TEflush()
{
	int	status;

	if (te_out_count)
	{
		status = (*te_out_func)();
		if (status)
			return status;
		te_out_count = 0;
	}
	return (OK);
}

static STATUS
TEout()
{
	STATUS	status;
	IOSB	iosb;

	status = sys$qiow(EFN$C_ENF, te_out_channel,
			  IO$_WRITEVBLK | IO$M_NOFORMAT, &iosb, 0, 0,
			  te_out_buffer, te_out_count, 0, 0, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if ((status & 1) == 0)
		return (FAIL);
	return OK;
}


/*
**	TEread	- to be filled in at a later time
**
**	Description:
**		This is a place holder to when we decide to use the more advanced features
**		that some operating systems support.  (I.E. functions keys parsing, line editing,
**		out-of-band terminators ..., these are features from VMS).
*/

void
TEread()
{
}

/*
**	TEwrite	- write a buffer to the terminal
**
**	Description:
**		When the application knows that it is writing more than
**		one character it should call this routine.  It efficently
**		moves many characters to the buffer and then to the
**		terminal. The buffer is discarded without being written
**		to the terminal if command mode is enabled.
**	History
**		10/83 derek  VMS CL
**		11/8/83 (dd)  allow correct operation if 'length' is large.
*/

void
TEwrite(buffer, length)
char	*buffer;
int	length;
{
	int	l = length;
	char	*b = buffer;
	int	move_amount;

	if (TEcmdEnabled)
		return;
	while (l)
	{
		move_amount =  teoutsize - te_out_count;

		if (move_amount > l)
			move_amount = l;
		MEcopy(b, move_amount, (PTR)&te_out_buffer[te_out_count]);
		if ((te_out_count += move_amount) == teoutsize)
			TEflush();
		l -= move_amount;
		b += move_amount;
	}
}

/*
**	TEcmdwrite - write a buffer to the terminal
**
**	Description:
**		TEcmdwrite is used to output characters in the command
**		mode. It efficiently moves many characters to the
**		buffer and then to the terminal. It makes sure that all
**		the characters are sent to the terminal so there is no
**		need to call TEflush subsequently. The buffer is
**		discarded without being written to the terminal if
**		command mode is not enabled.
*/

void
TEcmdwrite(buffer, length)
char	*buffer;
int	length;
{
	int	l = length;
	char	*b = buffer;
	int	move_amount;

	if (! TEcmdEnabled)
		return;
	while (l)
	{
		move_amount =  teoutsize - te_out_count;

		if (move_amount > l)
			move_amount = l;
		MEcopy(b, move_amount, (PTR)&te_out_buffer[te_out_count]);
		if ((te_out_count += move_amount) == teoutsize)
			TEflush();
		l -= move_amount;
		b += move_amount;
	}
	TEflush();
}

/*
**	TEget	- get a character
**
**	Description:
**		Return the next character in the lookahead buffer, or go ask VMS for the
**		next character.
**
**	History:
**		02/24/88 (dkh) - Fixed jup bug 1995 by integrating fix to
**		bug 12007 from 5.0.
*/

i4
TEget(seconds)
i4	seconds;
{
	int	status;

	if (te_in_count)
	{
		te_in_count--;
		return (*te_in_ptr++);
	}
	status = (*te_in_func)(seconds);
	if (status)
	{
		if (status == SS$_PARTESCAPE)
		{
			return(te_in_buffer[0]);
		}
		else if (status == SS$_TIMEOUT)
		{
			return(TE_TIMEDOUT);
		}
		if (status == EOF)
			return EOF;
		lib$signal(status);
	}
	return (te_in_buffer[0]);
}

static
TEin(seconds)
i4	seconds;
{
	int	status;
	IOSB	iosb;
	i4	readmod;

	readmod = IO$_READVBLK|IO$M_NOECHO|IO$M_NOFILTR;

	/*
	**  If timeout value greater than zero, then change
	**  read modifier to have timeout on input.
	**  We are assuming that we are not passed a negative value.
	*/
	if (seconds > 0)
	{
		readmod |= IO$M_TIMED;
	}

	for (;;)
	{
		status = sys$qiow(EFN$C_ENF, te_in_channel, readmod, &iosb, 0, 0,
			te_in_buffer, 1, seconds, 0, 0, 0);
	    /*
		readmod = IO$_READVBLK|IO$M_NOECHO|IO$M_NOFILTR;
	    */
		readmod &= ~IO$M_PURGE;
		if ((status & 1) == 0)
			return (status);
		if ((iosb.iosb$w_status & 1) == 0)
		{
			if (iosb.iosb$w_status == SS$_DATAOVERUN)
			{
				readmod |= IO$M_PURGE;
			    /*
				readmod = IO$_READVBLK|IO$M_NOECHO|IO$M_NOFILTR|IO$M_PURGE;
			    */
				continue;
			}
			return (iosb.iosb$w_status);
		}
		else if (iosb.iosb$w_status == SS$_CONTROLC)
			continue;
		break;
	}
	return OK;
}


/*
**	TEput	- put a character
**
**	Description:
**		Put a character to the terminal.  This is actually
**		buffered to the terminal, so to force the change
**		call TEflush().
**		The character is discarded if command mode is
**		enabled.
*/

void
TEput(
char	c)
{
	if (TEcmdEnabled)
		return;
	if (te_out_count == teoutsize)
		TEflush();
	te_out_buffer[te_out_count] = c;
	te_out_count++;
}

/*
**	TErestore - Restore previous terminal characteristics
**
**	Description:
**		This is the entry point to reset the terminal state.
**		On VMS, the terminal state is modified only when command
**		mode is enabled.
**		This function also sets the mode for output placement.
*/

STATUS
TErestore(which)
i4	which;
{
    STATUS	ret_val = OK;
    int	sys_stat;
    short	channel;

    if ((which == TE_NORMAL) || (which == TE_FORMS))
    {
	if (cur_term_mode != TE_CMDMODE)
	    return(OK);

	if (!TEsaveok)
	{
	    /*
	    ** can not restore a terminal that has not
	    ** been saved yet.  This save happens in
	    ** TEopen()
	    */
	    return(FAIL);
	}
	if (te_in_channel != 0)
	    channel = te_in_channel;
	else if (te_out_channel != 0)
	    channel = te_out_channel;
	else
	    channel = 0;
	
	if (channel != 0)
	{
	    IOSB iosb;

	    sys_stat = sys$qiow(EFN$C_ENF, channel, IO$_SETMODE, &iosb,
				0, 0, &termio, sizeof(termio),
				0, 0, 0, 0);
	    if (sys_stat & 1)
		sys_stat = iosb.iosb$w_status;
	    if (! (sys_stat & SS$_NORMAL))
	    {
		ret_val = FAIL;	
	    }
	}
	if (ret_val == OK)
	{
	    cur_term_mode = which;
	    TEcmdEnabled = FALSE;
	}
    }
    else if (which == TE_CMDMODE)
    {
    	if (cur_term_mode == TE_CMDMODE)
	    return(OK);

	if (TEcmdmode())
	{
	    ret_val = FAIL;
	}
	else
	{
	    TEcmdEnabled = TRUE;
	    cur_term_mode = which;
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
**	TEname - return the name of the terminal
**
**	Description:
**		Used by the protection code to see if protections apply
**		on a terminal by terminal basis.
**              
**              VMS returns either a four character terminal name in upper
**              case followed by a colon with a length of 5 (e.g TXD4:)
**              or a null terminal name which is a colon with a length of 1.
**              Since Ingres expects a four character lower case name or
**              a null string if no name exists the colon is replaced with
**              a null character (\0) and the name is converted to lower
**              case.
**
**      History:
**
**         jml (23-JUl-84) convert name to lower case.
**
*/

STATUS
TEname(terminal)
char	*terminal;
{
	int		status;
	int		parent_pid;
	char		term_name[8];
	IOSB		iosb;
	u_i2	terminal_length;	
	ILE3		jpi_list[] =
	    {
		{ 8, JPI$_TERMINAL, terminal, &terminal_length },
		{ 4, JPI$_OWNER, (PTR)&parent_pid, 0 },
		{ 0 }
	    };

	/*	see if we are a login process	*/

	status = sys$getjpiw(EFN$C_ENF, 0, 0, jpi_list, &iosb, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if ((status & 1) == 0)
		return (FAIL);

	/*	search backwards thru are ancestry and see if they belong to a terminal	*/

	while (terminal_length == 0 && parent_pid)
	{
	    status = sys$getjpiw(EFN$C_ENF, &parent_pid, 0, jpi_list, &iosb,
                                 0, 0);
	    if (status & 1)
		status = iosb.iosb$w_status;
	    if ((status & 1) == 0)
		return (FAIL);
	}

	/*	if there is no terminal, return a null string	*/

	if (terminal_length)
		terminal[terminal_length - 1] = '\0';

        /*       convert uppercase to lowercase      */

        CVlower(terminal);

	return (OK);
}

/*
**	TEvalid - check to see if the name is a terminal device
**
**	Description:
**		Used by the protection code to determine if a define
**		permit is referencing a legal terminal.
**
**	NOTE:
**		On VMS devices of the form:
**			rt*
**			tt*
**			tx*
**			op*
**		are legal terminal names.
*/

bool
TEvalid(terminal)
char	*terminal;
{
	if (terminal[0] == 't')
		if (terminal[1] == 't' || terminal[1] == 'x')
			return (TRUE);
		else
			return (FALSE);
	if (terminal[0] == 'r' && terminal[1] == 't')
		return (TRUE);
	if (terminal[0] == 'o' && terminal[1] == 'p')
		return (TRUE);
	return (FALSE);
}

static STATUS
TEchkdev(term)
struct	dsc$descriptor_s	*term;
{
	int	status;
	int	retval;
	int	class;
	int	astval;
	IOSB	iosb;
	ILE3 dvi_list[] =
	    {
	       { sizeof(class), DVI$_DEVCLASS, (PTR)&class, 0 }, { 0 }
	    };

	retval = FAIL;

	/* 
	** Note:  If GETDVI is called on a non-device (ie., a file),
	** GETDVI will NOT be queued and a status of SS$_IVDEVNAM will
	** be returned in status.  The code below works fine but is
	** technically incorrect.  (rlk)
	*/

	status = sys$getdvi(EFN$C_ENF, 0, term, dvi_list, &iosb, 0, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if ((status & 1) == 0)
	{
		return(retval);
	}
	if (class != DC$_TERM)
	{
		return(retval);
	}
	return(OK);
}


static STATUS
TEassdev(term, channel)
struct	dsc$descriptor_s	*term;
short				*channel;
{
	int	status;

	status = sys$assign(term, channel, 0, 0);
	if ((status & 1) == 0)
	{
		return(FAIL);
	}
	return(OK);
}


/*
** TEtest is used to test forms programs.
** in and out are the names of files to open for reading and
** writing.
*/
TEtest(in, in_type, out, out_type)
char	*in;
i4	in_type;
char	*out;
i4	out_type;
{
	i4		rval;
	LOCATION	loc;
	char		buf[MAX_LOC];
	int		status;
	struct	dsc$descriptor_s	terminal;

	terminal.dsc$b_class = DSC$K_CLASS_S;
	terminal.dsc$b_dtype = DSC$K_DTYPE_T;

	if (in && *in != '\0' && in_type != TE_NO_FILE)
	{
		/*
		 * Check to see if input channel has been assigned.
		 * If it has been and is different from the outut
		 * channel, then deassign it.
		 */
		if (te_in_channel != 0)
		{
			if (te_in_channel != te_out_channel)
			{
				status = sys$dassgn(te_in_channel);
				if (! (status & SS$_NORMAL))
					return(FAIL);
			}
			te_in_channel = 0;
		}

		/* 
		 * If output was assigned with a call to TEtest, then
		 * close the output file.
		 */
		if (te_in_test != NULL)
		{
			if (te_in_type == TE_SI_FILE)
				SIclose(te_in_test);
			else if (te_in_type == TE_TC_FILE)
				TCclose(te_in_comm);
		}

		terminal.dsc$w_length = STlength(in);
		terminal.dsc$a_pointer = in;
		if (TEchkdev(&terminal) == OK)
		{
			if ((rval = TEassdev(&terminal, &te_in_channel)) != OK)
			{
				return(rval);
			}
		}
		else
		{
		    switch (in_type)
		    {
			case TE_SI_FILE:
			    te_in_type = TE_SI_FILE;
			    STcopy(in, buf);
			    LOfroms(FILENAME, buf, &loc);
			    if (rval = SIopen(&loc, "r", &te_in_test))
			    {
				return rval;
			    }
			    break;
			case TE_TC_FILE:
			    te_in_type = TE_TC_FILE;
			    te_in_test = 1;
			    STcopy(in, buf);
			    LOfroms(FILENAME, buf, &loc);
			    if (rval = TCopen(&loc, "r", &te_in_comm))
			    {
                                return rval;
			    }
			    break;
			default:
			    return(FAIL);
		    }

		    /*
		    **  If te_in_func is already set, then we must
		    **  have called TEopen() already.  In this case,
		    **  we need to set te_in_func here (since no one
		    **  else will do so.  If te_in_func is still NULL,
		    **  then MUST leave it alone because TEopen() has
		    **  not been called yet.  Setting te_in_func before
		    **  TEopen is called will cause TEopen() to fail and
		    **  FAIL to the caller, not a good thing.  TEopen() is
		    **  set up to handle TEtest being called first and will
		    ** set te_in_func correctly for this case.
		    */
		    if (te_in_func != NULL)
		    {
			te_in_func = TEtestin;
		    }
		}
	}

	if (out && *out != '\0' && out_type != TE_NO_FILE)
	{
		/*
		 * Check to see if output channel has been assigned.
		 * If it has been and is different from the input
		 * channel, then deassign it.
		 */
		if (te_out_channel != 0)
		{
			if (te_out_channel != te_in_channel)
			{
				status = sys$dassgn(te_out_channel);
				if (! (status & SS$_NORMAL))
					return(FAIL);
			}
			te_out_channel = 0;
		}

		/* 
		 * If output was assigned with a call to TEtest,
		 * then close the output file.
		 */
		if (te_out_test != NULL)
		{
			if (te_out_type == TE_SI_FILE)
				SIclose(te_out_test);
			else if (te_out_type == TE_TC_FILE)
				TCclose(te_out_comm);
		}

		terminal.dsc$w_length = STlength(out);
		terminal.dsc$a_pointer = out;
		if (TEchkdev(&terminal) == OK)
		{
			if ((rval = TEassdev(&terminal, &te_out_channel)) != OK)
			{
				return(rval);
			}
		}
		else
		{
		    switch (out_type)
		    {
			case TE_SI_FILE:
			    te_out_type = TE_SI_FILE;
			    STcopy(out, buf);
			    LOfroms(FILENAME, buf, &loc);
			    if (rval = SIfopen(&loc, "w", SI_VAR, 0,
			    			&te_out_test))
			    {
				return rval;
			    }
			    break;
			case TE_TC_FILE:
			    te_out_type = TE_TC_FILE;
			    te_out_test = 1;
			    STcopy(out, buf);
			    LOfroms(FILENAME, buf, &loc);
			    if (rval = TCopen(&loc, "w", &te_out_comm))
			    {
                                return rval;
			    }
			    break;
			default:
			    return(FAIL);
		    }
		    /*
		    **  Don't set te_out_func if it is still NULL.
		    **  See comment for te_in_func above.
		    */
		    if (te_out_func != NULL)
		    {
			te_out_func = TEtestout;
		    }
		}
	}
	return OK;
}


/*
    routine was modified so it can read from COMM-files
*/

static
TEtestin()
{

	switch(te_in_type)
	{
	    case TE_SI_FILE:
		if ((te_in_buffer[0] = SIgetc(te_in_test)) == EOF)
		{
		    return(EOF);
		}
		break;
	    case TE_TC_FILE:
		if ((te_in_buffer[0] = TCgetc(te_in_comm,0)) == TC_EOF)
		{
		    return(EOF);
		}
		break;
	    default:
		break;
	}
        return OK;

}

static 
TEtestout()
{
	i4	count;

	switch(te_out_type)
	{
	    case TE_SI_FILE:
		SIwrite(te_out_count, (PTR)te_out_buffer, &count, te_out_test);
		SIflush(te_out_test);
		break;
	    case TE_TC_FILE:
		for (count = 0; count < te_out_count; count++)
		{
		    TCputc(te_out_buffer[count], te_out_comm);
		}
		TCflush(te_out_comm);
		break;
	    default:
		break;
	}
}

/*{
** Name:	TEswitch	- Switch input.
**
** Description:
**	Toggles input between a test file and the terminal.
**	TEtest must have been called first.  TEoutput must
**	must to the terminal.
**
** Inputs:
*/
TEswitch()
{
	if (te_in_channel == NULL)
	{
		struct dsc$descriptor_s	terminal;
		short			terminal_name = 'TT';

		terminal.dsc$w_length = 2;
		terminal.dsc$a_pointer = (PTR)&terminal_name;
		if (te_in_func != TEtestin)
			return FAIL;
		if (TEchkdev(&terminal) != OK)
			return FAIL;
		if (TEassdev(&terminal, &te_in_channel) != OK)
			return FAIL;
		if (te_out_channel == NULL)
			return FAIL;
		te_in_ptr = te_in_buffer;
		te_in_count = 0;
	}

	if (te_in_func == TEin)
	{
		te_in_func = TEtestin;
	}
	else
	{
		te_in_func = TEin;
	}

	/*
	**  The statement below confused the C compiler on alpha
	**  so it has been rewritten to be the simple if else
	**  statement above.

	te_in_func = te_in_func == TEin ? TEtestin : TEin;
	*/

	return OK;
}

/*
** TEgbf
**	Special open call for GBF to open a device the user inputs.
**	If the device is a terminal, then the normal TE mechanism will
**	do IO to the device.  If is not a terminal, it is assumed to
**	be a file which is created and the test mechanism is used to write
**	to the file.
*/
STATUS
TEgbf(devname, type)
char	*devname;
i4	*type;
{
	i4			delay;
	STATUS			retval;
	struct dsc$descriptor_s sysout;
	struct dsc$descriptor_s	dev;
	TEENV_INFO		open_dummy;

	/*	Show that TEgbf has been called */

	te_fromgbf = TRUE;

	dev.dsc$b_class = DSC$K_CLASS_S;
	dev.dsc$b_dtype = DSC$K_DTYPE_T;
	/*
	** If the string is NULL, then GBF output is going to the
	** terminal so open TT (which is NLA0: when GBF is run from a
	** batch job.)
	*/

	/*
	** All plotter device drivers will write to sys$output
	** which defined here to be the right thing.
	*/
	sysout.dsc$a_pointer = "SYS$OUTPUT";
	sysout.dsc$w_length = 10;
	sysout.dsc$b_class = DSC$K_CLASS_S;
	sysout.dsc$b_dtype = DSC$K_DTYPE_T;
	sys$crelog(2, &sysout, &dev, 0);

	if (devname == NULL || *devname == '\0')
	{
		dev.dsc$a_pointer = "TT:";
		dev.dsc$w_length = 3; 
	}
	else
	{
		dev.dsc$a_pointer = devname;
		dev.dsc$w_length = STlength(dev.dsc$a_pointer);
	}
	/*
	** get information about device, if can't get it
	** assume not a terminal
	*/

	if ((retval = TEchkdev(&dev)) != OK)
	{
		*type = TE_FILE;
		if (retval = TEtest(0,TE_NO_FILE,dev.dsc$a_pointer,TE_SI_FILE))
			return retval;
		return TEopen(&open_dummy);
	}
	else
	{
		*type = TE_ISA;
		TEassdev(&dev, &te_out_channel);
		return(TEopendev(&delay));
	}
}


/*{
**  Name:	TEinflush - Flush any input in typeahead buffers.
**
**  Description:
**	Flush any type ahead input from the terminal.  The
**	assumption here is that this routine will NOT be
**	called if a forms based front end is in test mode
**	(i.e., input coming from a file).  This will
**	eliminate the possibility of doing a flush on
**	input that comes from a file.
**
**  Inputs:
**	None.
**
**  Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
**  Side Effects:
**	None.
**
**  History:
**	05/29/87 (dkh) - Initial version.
*/
VOID
TEinflush()
{
	int	status;
	IOSB	iosb;
	i4	readmod;
	char	buffer[10];

	readmod = IO$_READVBLK|IO$M_NOECHO|IO$M_NOFILTR|IO$M_PURGE;
	status = sys$qiow(EFN$C_ENF, te_in_channel, readmod, &iosb, 0, 0,
		buffer, 0, 0, 0, 0, 0);
}


/*{
** Name:	TElock	- Allow or disallow screen modification by a
**			  cooperating process.
**
** Description:
**	This entry point is a stub for all environments but Data General.
**	It is used to bracket sections of code within which the terminal
**	screen may not be modified by another cooperating process.  For
**	DG, the process being locked out is that used to update the
**	Comprehensive Electronic Office (CEO) status line.
**	
**	Screen modification by the cooperating process is disallowed until
**	this routine is first called to unlock the terminal.  At that time,
**	the current cursor location is also specified.  Multiple attempts
**	to lock the terminal will be accepted, but the first call to unlock
**	the screen will succeed.
**	
**	Coordinate system is zero based with the origin at the upper
**	left corner of the screen with the X coordinate increasing to
**	the right and the Y coordinate increasing towards the bottom.
**	
**	Locking is called only when the forms system is about to update
**	the screen.  Thus the forms system will set the lock when it is
**	about to update the terminal screen and unlock when it is finished.
**	It is assumed that if an INGRES application exits, the cooperating
**	process also exits.  There is also no danger of locking the terminal
**	physically since the locking mechanism described here is only for a
**	parent and cooperating process.  No other processes are affected.
**	
**	Usage is TElock(TRUE, (nat) 0, (nat) 0) to lock the screen,
**	TElock(FALSE, row, column) to re-enable screen updates.
**	
** Inputs:
**	lock	Specify whether the program is to lock out screen updates.
**	row	Specify the cursor's y coordinate; ignored if screen
**		is locked.  Possible values range from 0 to the
**		screen length - 1.
**	col	Specify the cursor's x coordinate; ignored if screen is
**		locked.  Possible values range from 0 to the screen
**		width - 1.
**
** Outputs:
**	None.
**
**	Returns:
**		VOID.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	This is a stub for all non-DG systems.
**
** History:
**	08-sep-87 (bab)	Creation of stub.
*/
VOID
TElock(lock, row, col)
bool	lock;
i4	row;
i4	col;
{
}


/*
** TEsave
**	Saves the terminal state so we can restore it
**	when the user exits from one of the front-ends.
**
**	This routine is internal to the TE routines and
**	is not to be used outside the compatibility library.
**
** Returns:
**	OK - if the terminal state was saved.
**	FAIL - if we could not save the terminal state.
*/
static STATUS
TEsave()
{
        int     sys_stat;
	short	channel;
	IOSB	iosb;

	if (te_in_channel != 0)
		channel = te_in_channel;
	else if (te_out_channel != 0)
		channel = te_out_channel;
	else
		return(OK);
	sys_stat = sys$qiow(EFN$C_ENF, channel, IO$_SENSEMODE, &iosb, 0, 0,
			    &termio, sizeof(termio), 0, 0, 0, 0);
	if (sys_stat & 1)
	    sys_stat = iosb.iosb$w_status;
	if (! (sys_stat & SS$_NORMAL))
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
** Set terminal (i.e., stdout) for command mode
** to communicate with another computer.
*/
static STATUS
TEcmdmode()
{
	struct	termio	_tty;
	int		sys_stat;
	short		channel;
	IOSB		iosb;

	if (te_in_channel != 0)
		channel = te_in_channel;
	else if (te_out_channel != 0)
		channel = te_out_channel;
	else
		return(OK);

	sys_stat = sys$qiow(EFN$C_ENF, channel, IO$_SENSEMODE, &iosb, 0, 0,
			    &_tty, sizeof(_tty), 0, 0, 0, 0);
	if (sys_stat & 1)
	    sys_stat = iosb.iosb$w_status;
	if (! (sys_stat & SS$_NORMAL))
	{
		return(FAIL);
	}

	_tty.basic |= TT$M_EIGHTBIT|TT$M_HOSTSYNC|TT$M_LOWER|TT$M_TTSYNC;
	sys_stat = sys$qiow(EFN$C_ENF, channel, IO$_SETMODE, &iosb, 0, 0,
			    &_tty, sizeof(_tty), 0, 0, 0, 0);
	if (sys_stat & 1)
	    sys_stat = iosb.iosb$w_status;
	if (! (sys_stat & SS$_NORMAL))
	{
		return(FAIL);
	}

	return(OK);
}


