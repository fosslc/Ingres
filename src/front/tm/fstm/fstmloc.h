/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<termdr.h>

/*
**	FSloc.h - Declarations for Full Screen Terminal Monitor
*/

extern	WINDOW	*dispscr;	/* Display Area */
extern	WINDOW	*statscr;	/* Status Area */
extern	WINDOW	*bordscr;	/* Border Area */

# define	fsopHOME	1
# define	fsopBOTT	2
# define	fsopLSCR	3
# define	fsopRSCR	4
