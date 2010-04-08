/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<termios.h>
# include 	<te.h>

/*
**
**	Name: TETM.C - Terminal settings for terminal monitor when using history
**	
**	Description:
**		The functions are used by SIhistgetrec(), which is called by terminal
**	monitor. 
**	This file defines the following functions:
**
**	TEsettmattr() - Set the terminal attributes required by the SIhistgetrec
**	function.
**	TEresettmattr() - Reset the terminal attributes as they were before calling
**	TEsettmattr function.
**
**	These functions are now implemented using termios. They can also be
**	implemented using ioctl or related functions. 
**
**	History
**	31-mar-2004 (kodse01)
**		SIR 112068
**		Written.
**		NOTE: platform related issues to be taken care.
*/

struct termios org_tty_termios;

/* Set the terminal attributes required by the SIhistgetrec function */
i4 
TEsettmattr()
{
	struct termios new_tty_termios;
	if (tcgetattr(STDINFD, &org_tty_termios) < 0)
        return -1;
	new_tty_termios = org_tty_termios; /* working with a copy */

	/* local modes - 
		echoing off, canonical off, no signal chars (^Z,^C) */ 
	new_tty_termios.c_lflag &= ~(ECHO | ICANON | ISIG);

	/* set the new tty values */ 
	if (tcsetattr(STDINFD, TCSANOW, &new_tty_termios) < 0) 
		return -1;
	
	return 0;
}
	
/* Reset the terminal attributes as they were before calling TEsettmattr 
	function */
i4 
TEresettmattr()
{
	if (tcsetattr(STDINFD, TCSAFLUSH, &org_tty_termios) < 0)
		return -1;
	return 0;
}
