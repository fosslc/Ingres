/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/

/**
** Name: TE.H - Global definitions for the TE compatibility library.
**
** Description:
**      The file contains the type used by TE and the definition of the
**      TE functions.  These are used for terminal support.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**      14-oct-87 (bab)
**          Added typedef of TEENV_INFO.
**	12/01/88 (dkh) - Added definition of TE_TIMEDOUT.
**	05/22/89 (Eduardo) - Added new constants for TEtest
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	17-aug-2001 (somsa01 for fanra01)
**	    SIR 104700
**	    Add TE_IO_MODE for direct stream I/O.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN STATUS  TEgbf();
FUNC_EXTERN bool    TEgtty();
FUNC_EXTERN STATUS  TEopendev();
FUNC_EXTERN VOID    TEread();

/* 
** Defined Constants
*/

/* TEtest constants */

#define		TE_NO_FILE	0

#define		TE_SI_FILE	1

#define		TE_TC_FILE	2

/* TE return status codes. */

#define                 TE_OK           0

# define	TE_TIMEDOUT	(i4) -2

# define	TE_NORMAL	1
# define	TE_FORMS	2
# define	TE_CMDMODE	3
# define	TE_IO_MODE	4

/*
**     ****** Should delete this, nested includes
**     ****** not allowed in coding standard.
*/

# ifndef TTYIDSIZE
# define	TTYIDSIZE	 8
# endif

# define	TE_FILE		1
# define	TE_ISA		2

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
