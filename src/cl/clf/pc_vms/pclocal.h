/*
 *	Copyright (c) 1983 Ingres Corporation
 *
 *	Name:
 *		PC.h
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		defines symbols for PC module routines
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *
 *
 *      02-oct-96 (rosga02)
 *          Add CLI$M_TRUSTED (%X40) for bug 74980
 *
 */

/*
	stores current STATUS of PC routines
*/

globalref	STATUS		PCstatus;
globalref	i4		PCeventflg;

# define	MAXSTRINGBUF	80
# define	PCEVENTFLGP	&PCeventflg
# define        CLI$M_TRUSTED	0x40L
