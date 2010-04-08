/*
**	static	char	Sccsid[] = "@(#)trace.h	1.2	2/7/85";
*/

/*
**  11-11-84:
**
**  NOTE:  This file is not currently used. (dkh)
*/

/*
** A multitude of defined constants for use with the trace flags
** facility.
**
** Only interested in three things in vifred, and all are to do
** with timing.
**
** DB - any database operations
** FRM - any thing to do with the frames of vifred.  Those frames
**	  vifred uses to get information.
** STRUCTS - any internal structures, but mostly line table.
**
** Copyright (c) 2004 Ingres Corporation
*/

 
/*
** TRACE SIZE
*/

# define	vfTSIZE		4

/*
** flags which ifdef out tTf calls
*/
/*
# define	txDB
# define	txFRM
# define	txSTRUCT
# define	txMOVE
*/

/*
** Trace flags for the main objects
*/

# define	vfTDB		0
# define	vfTFRM		1
# define	vfTSTRUCT	2
# define	vfTMOVE		3

/*
** bits within the flags
*/

# define	vfTINIT		0		/* any inits */
# define	vfTWRITE	1		/* any writes */
# define	vfTALLOC	2

/*
**	Inline expansion of tTf
*/

extern i2	*tT;

# ifndef	tTf
# define	tTf(a,b) (( b < 0) ? tT[a] : (tT[a] & (1 << b)))
# endif /* tTf */
