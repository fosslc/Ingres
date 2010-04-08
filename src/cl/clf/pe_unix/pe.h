
/*
** PE.h -- Header file for the Permission Compatibility library for UNIX.
**
**	History:
**	
 * Revision 1.1  90/03/09  09:16:20  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:45:06  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:59:23  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:19:12  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:13:14  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:44:11  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  14:58:33  mikem
**      initial 6.0 rpath checkin
**      
**      Revision 1.1  87/11/09  13:10:55  mikem
**      Initial revision
**      
 * Revision 1.1  87/07/30  18:09:21  roger
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.2  86/04/25  10:03:17  daveb
 * remove comment cruft.
 * 
*/


# include	<compat.h>		/* include the compatiblity header */
# include	<LO.h>			/* LOCATION library header file */
# include	<sys/stat.h>		/* header for info on a file */
# include	"PEerr.h"		/* PE error header file */

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
