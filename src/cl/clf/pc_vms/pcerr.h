/*
** Copyright (c) 1985, Ingres Corporation
*/

# include	<ssdef.h>

/**
** Name: PC.H - Global definitions for the PC compatibility library.
**
** Description:
**      The file contains the local types used by PC.
**      These are used for process control.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	11/23/92 (dkh) - Updated to use more conventional C include syntax
**			 for alpha port.
**/

/*
**  Forward and/or External function references.
*/

/* 
** Defined Constants
*/

/* PC return status codes. */

# define	 PC_CM_CALL	(E_CL_MASK + E_PC_MASK + 0x01)
/*  PCcmdline: arguments incorrect  */
# define	 PC_CM_BAD	(E_CL_MASK + E_PC_MASK + 0x0A)
/*  PC*: unreadable parameters, bad or inacessible date, etc.  */
# define	 PC_NOT_MBX	(E_CL_MASK + E_PC_MASK + 0x24)
/* PCendpipe: channel not assigned to mailbox */
# define	 PC_INTERLOCK	(E_CL_MASK + E_PC_MASK + 0x25)
/* PCendpipe: mailbox locked by another process */
# define	 PC_BADCHAN	(E_CL_MASK + E_PC_MASK + 0x26)
/* PCendpipe: not a legal channel number */
# define	 PC_NOPRIV	(E_CL_MASK + E_PC_MASK + 0x27)
/* PCendpipe: mailbox not assigned or insufficient privilege */
# define	 PC_SP_CALL	(E_CL_MASK + E_PC_MASK + 0x0D)
/*  PCspawn: arguments incorrect  */
# define	 PC_WT_BAD	(E_CL_MASK + E_PC_MASK + 0x20)
/*  PCwait: illegal event flag number */
