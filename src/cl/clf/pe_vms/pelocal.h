/*
** Copyright (c) 1985, Ingres Corporation
*/

/**
** Name: PElocal.H - Local definitions for the PE compatibility library.
**
** Description:
**      The file contains the local types used by PE.
**      These are used for permission handling.
**
** History:
**      04-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**	16-sep-1986 (thurston)
**	    Combined the PEerr.h and local PE.h files into this file, now
**	    named PElocal.h.
**/


/* 
** Defined Constants
*/

# define	READ		04	/* access mode for reading */
# define	WRITE		02	/* access mode for writing */
# define	EXEC		01	/* access mode for executing */

# define	OREAD		0x0010	/* owner read access */
# define	OWRITE		0x00a0	/* owner write and delete access */
# define	OEXEC		0x0040	/* owner execute access */

# define	WREAD		0x1100	/* group and world read access */
# define	WWRITE		0xaa00	/* group and world write and delete access */
# define	WEXEC		0x4400	/* group and world execute access */
# define	SYSRWED		0x000f	/* Rd, Wt, Ex, Del perm for system */

# define	UNKNOWN		0	/* unknown permission status */
# define	GIVE		1	/* give permission status */
# define	TAKE		2	/* take away permission status */

# define	UMASKLENGTH	6	/* length of the umask */
# define	PEBUFSZ		256	/* string buffer size in PE.c */

# define	NONE		0	/* resets the permission bits */


/* PE return status codes. */

# define	PE_BAD_PATTERN	        (E_CL_MASK + E_PE_MASK + 0x01)
				/* Permission pattern passed to a
                                ** permission routine had syntax errors
                                */

# define	PE_NULL_LOCATION	(E_CL_MASK + E_PE_MASK + 0x02)
				/* Location passed to a permission
                                ** routine was null
                                */
