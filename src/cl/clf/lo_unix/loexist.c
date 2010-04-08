
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include 	<systypes.h>
# include	<sys/stat.h>
# include	<lo.h>
# include	"lolocal.h"

/*
**Copyright (c) 2004 Ingres Corporation
**
** LOexist -- Does location exist?
**
**		NOTE:
**		  	access() runs as the real UID rather than the
**			effective UID.  This screws up ingres when it
**			tries to see if ~ingres/data/default exists.
**			data is owned by ingres with permission 700.
**			to get around this a call to stat is made instead.
**
**		returns: OK, FAIL
**
**	History:
**	
 * Revision 1.2  90/05/23  15:56:55  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:29  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:42:59  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:52:33  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:13:20  source
 * sc
 * 
 * Revision 1.2  89/03/06  14:29:38  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:32  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:40:43  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  09:53:06  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:52:51  joe
 * *** empty log message ***
 * 
**Revision 30.1  85/08/14  19:12:45  source
**llama code freeze 08/14/85
**
**		Revision 3.0  85/05/12  00:28:32  wong
**		Removed include of "<sys/types.h>" (already included
**		by "compat.h").
**
**	02/23/89 (daveb, seng)
**		rename LO.h to lolocal.h
**	28-Feb-1989 (fredv)
**		include <systypes.h>.
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**		
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	28-feb-2000 (somsa01)
**	    Use stat64() when LARGEFILE64 is defined.
**	11-May-2001 (jenjo02)
**	    Return appropriate error if stat() fails rather than just
**	    "FAIL" so that LO users can distinguish between "does not exist"
**	    and problems with permissions.
*/

STATUS
LOexist (loc)
register LOCATION	*loc;	/* location of file or directory */
{
#ifdef LARGEFILE64
	struct	stat64	dummy_stat;
#else /* LARGEFILE64 */
	struct	stat	dummy_stat;
#endif /* LARGEFILE64 */

	/* stat returns 0 if a status is available (this will be translated
	** as "yes, the file exists.  Stat returns -1 if the file cannot be
	** found, or if there are problems with permissions.
	**
	** Throw away information filled into dummy_stat.
	*/

#ifdef LARGEFILE64
	if ( stat64(loc->string, &dummy_stat) )
#else /* LARGEFILE64 */
	if ( stat(loc->string, &dummy_stat) )
#endif /* LARGEFILE64 */
	    return ( LOerrno((i4)errno) );
	return(OK);
}
