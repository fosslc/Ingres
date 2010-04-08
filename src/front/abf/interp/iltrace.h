/*
**Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	ILTRACE.h	-	Header file for interpreter tracing
**
** Description:
**	Header file for interpreter tracing.
**
*/


/*
** Trace size
*/
# define	IIOTRSIZE	3

/*
** Flag to ifdef out ALL trace calls, and compilation of trace routines.
*/
# define	IIOtxDebug	y
/*
** Flags to ifdef out specific types of trace calls
*/
# define	IIOtxRoutent
# define	IIOtxStk	y

/*
** Flags for use with the trace routines.
*/
# define	IIOTRROUTENT	0	/* Info at beginning of routines */
# define	IIOTRSTK	1	/* Dump contents of stack frame */
