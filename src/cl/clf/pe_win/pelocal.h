/*
** Copyright (c) 1987, Ingres Corporation
*/

/**
** Name: PElocal.H - Local definitions for the PE compatibility library.
**
** Description:
**      The file contains the local types used by PE.
**      These are used for permission handling.
**
** History: 
 * Revision 1.1  90/03/09  09:16:21  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:45:10  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:59:37  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:19:20  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:13:14  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:44:13  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 1.1  87/11/10  14:57:00  mikem
 * Initial revision
 * 
**      04-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**	16-sep-1986 (thurston)
**	    Combined the PEerr.h and local PE.h files into this file, now
**	    named PElocal.h.
**	21-jul-1987 (mmm)
**	    Updated UNIX CL to jupiter coding standards.
**/


/* 
** Defined Constants
*/

# define	READ		04	/* access mode for reading */
# define	WRITE		02	/* access mode for writing */
# define	EXEC		01	/* access mode for executing */

# define	AREAD		0444	/* all read access */
# define	AWRITE		0222	/* all write access */
# define	AEXEC		0111	/* all execute access */


# define	OREAD		0400	/* owner read access */
# define	OWRITE		0200	/* owner write access */
# define	OEXEC		0100	/* owner execute access */

/*
**	needed as UNIX doesn't check world status
**		if user is in same group as owner???
*/

# define	WREAD		0044	/* group and world read access */
# define	WWRITE		0022	/* group and world write access */
# define	WEXEC		0011	/* group and world execute access */

# define	UNKNOWN		0	/* unknown permission status */
# define	GIVE		1	/* give permission status */
# define	TAKE		2	/* take away permission status */

# define	UMASKLENGTH	6	/* length of the umask */
# define	PEBUFSZ		256	/* string buffer size in PE.c */

# define	NONE		0	/* resets the permission bits */


/* PE return status codes. */

# define	PE_BAD_PATTERN	2800	/* :Permission pattern passed to a permission routine had syntax errors */
# define	PE_NULL_LOCATION	2805	/* :Location passed to a permission routine was null */
