/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** te.h
**
** Header file for Terminal I/O Compatibility library for
** Berkeley UNIX.
**
** Created 2-15-83 (dkh)
** Modified 8-9-85 (joe)
**	Changed input from stdin to IIte_fpin to prepare for TEswitch.
** 14-oct-87 (bruceb)
**	Added typedef of TEENV_INFO.
** 10-mar-89 (bruceb)
**	Added define of TE_TIMEDOUT, for support of timeout.
** 25-may-89 (mca)
**	Added constants used by the "new" TEtest to distinguish SEP-based
**	TCFILEs from regular FILEs.
** 23-may-90 (blaise)
**	Integrated changes from 61 and ingresug:
**	    Include systypes.h so u_char, u_short, etc. don't get redefined.
**	11/29/90 (dkh) - Added declaration of TEget().
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	09-Jan-2001 (hanje04)
**	    Added Ingres defn TE_VMIN TE_VTIME for VMIN and VTIME because of
**	    bug 103708 on axp.lnx (Alpha Linux). For ALL other platforms these 
**	    will be defined as system defines VMIN and VTIME.
**	17-aug-2001 (somsa01 for fanra01)
**	    SIR 104700
**	    Add TE_IO_MODE for direct stream I/O.
**	24-Feb-2002 (hanch04)
**	    Moved TE_ECHOON and TE_ECHOOFF from te.h to tecl.h
*/

# include	<compat.h>		/* include compatiblity header file */
# include	<systypes.h>
# include	<si.h>			/* to get stdio.h stuff */

# define	STDINFD		0	/* file descriptor for stdin on UNIX */
# define	STDOUTFD	1	/* file descriptor for stdout on UNIX*/
# define	STDERRFD	2	/* file descriptor for stderr on UNIX */

/*
** Terminal characteristics you can set
*/

# define	TECRMOD		1

/*
** Constants used by TErestore
*/

# define	TE_NORMAL	1
# define	TE_FORMS	2
# define	TE_CMDMODE	3
# define        TE_ECHOON       4       /* standard input echo on */
# define        TE_ECHOOFF      5       /* standard input echo off */
# define	TE_IO_MODE	6

/*
** Constants used by TEgbf
*/

# define 	TE_FILE		1
# define 	TE_ISA		2
# define 	TEGBFOPEN	3
# define 	TEGBFCLOSE	4

/*
** Constants used by TEtest
*/
# define	TE_NO_FILE	1
# define	TE_SI_FILE	2
# define	TE_TC_FILE	3

/*
** The following are defines for the speeds that a terminal can run at.
** The list is longer than what UNIX supports due to VMS.
*/

# define	TE_BNULL	-1
# define	TE_B50		0
# define	TE_B75		1
# define	TE_B110		2
# define	TE_B134		3
# define	TE_B150		4
# define	TE_B200		5
# define	TE_B300		6
# define	TE_B600		7
# define	TE_B1200	8
# define	TE_B1800	9
# define	TE_B2000	10
# define	TE_B2400	11
# define	TE_B3600	12
# define	TE_B4800	13
# define	TE_B9600	14
# define	TE_B19200	15

/*
	Used by qrymod (protects) and journaling
*/

# define	TTYIDSIZE	 8



extern	bool	GT,	UPCSTERM;

/*
** Type used by TEgbf
*/

typedef	i2	TEGBFACT;

/*}
** Name:	TEENV_INFO	- Terminal environment information.
**
** Description:
**	Terminal environment information.  Used by TEopen to return
**	information to the calling routine.  Currently contains
**	a delay constant and the number of rows and columns on
**	the terminal screen.  If rows (or cols) is zero (0), use
**	the value obtained by looking up TERM_INGRES in the termcap
**	file.  If rows (or cols) is greater than zero, then use this
**	value instead of the one obtained from termcap.  If rows
**	(or cols) is negative, then the absolute value is used to
**	indicate the number of rows (or cols) made unavailable to
**	the forms runtime system (i.e., subtract from termcap value).
**
**	Caveat:  This structure is not intended to be used for dynamically
**	re-sizing the forms system window size at runtime.
**	
** History:
**	25-sep-87 (bab)
**		Created for use in TEopen.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
typedef struct
{
	i4	delay_constant;	/*
				**  A multiplier for determining the number
				**  of pad characters sent during terminal
				**  output.  Value based on terminal baud rate.
				*/
	i4	rows;		/*
				**  If positive, this is the number of rows
				**  to use for the terminal.  If zero, use
				**  value in termcap.  If negative, use
				**  absolute value to indicate number of
				**  rows made unavailable to the forms
				**  runtime system (subtract from value
				**  obtained from termcap).
				*/
	i4	cols;		/*
				**  If positive, this is the number of
				**  columns to use for the terminal.  If zero,
				**  use value in termcap.  If negative, use
				**  absolute value to indicate number of
				**  columns made unavailable to the forms
				**  runtime system (subtract from value
				**  obtained from termcap).
				*/
} TEENV_INFO;

/*TEflush
**
**	Flush buffers to terminal
**
**	Flush the write buffer to the terminal.  Normally called when
**	the buffer is full or when the application decides that the
**	screen should be updated.
**
*/
# define	TEflush()	fflush(stdout)

extern FILE	*IIte_fpin;	/* Defined in TEopen */

/*
**
**
**		Utility routines that deals with input, 
**		output testing files, recovery file and frame dump 
**		files in a multi-processes environment.
**
**
**	history:
**		2/85 (ac) -- created.
*/


/* given a file descriptor, getfp returns the corresponding spot of
** the file pointer array _iob.  This doesn't overwrite the existing
** file pointer, since getfp is called right after the process is
** created.
*/

# define	TEgetfp(fd)	(fd == 0 ? NULL : &_iob[fd])

/* The file descriptor of recovery file and frame dump file can't
** be 0 for sure.
*/

# define	TEgetfd(fp)	(fp == NULL ? 0 : fileno( fp ) )

/* make fp unbuffered */

# define	TEunbuf(fp)	setbuf( fp, NULL )


# define	TE_TIMEDOUT	((i4) -2)

/* In te and gc VMIN and VTIME have been replace with TE_VMIN & TE_VTIME.
** This is because on Alpha Linux the value for these are incorrect for the
** termio structure (they correspond to the termios structure). For Alpha 
** Linux they will be hard coded to 4 & 5 respectively, for all other platforms
** they will just be set equivalent to VMIN and VTIME.
*/

#ifdef axp_lnx

#define TE_VMIN 4
#define TE_VTIME 5

#else

#define TE_VMIN VMIN
#define TE_VTIME VTIME

#endif /* axp_lnx */
