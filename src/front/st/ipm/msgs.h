/*
** (c)1989, Relational Technology
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/*
** msgs.h - include file for error or status message routines and callers
**
** History
**	8/24/89		tomt	broken out from dba.qh to make it easier
**				to call message routines from .c files
*/

/*
** Forward Reference and External Functions
*/
VOID	dispmsg();		/* displays messages for user to see */

/*
**	for dispmsg() routine
*/
#define MSG_POPUP 1	/* fancy boxed popup message */
#define MSG_LINE  2	/* plain ingres single line message */

#define ERBUFSZ 1024    /* also defined in servwork.c */

