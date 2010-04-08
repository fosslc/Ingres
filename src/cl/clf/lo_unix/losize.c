
#include	<compat.h>
#include	<gl.h>
#include        <clconfig.h>
#include	<systypes.h>
#include	<lo.h>
#include	"lolocal.h"
#include	<errno.h>
#include	<sys/stat.h>

/*
** Copyright (c) 1985, 2000 Ingres Corporation
**
** LOsize -- Get location size.
**
**	Get LOCATION size in bytes.
**
**	History:
**	
 * Revision 1.2  90/05/23  15:57:28  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:33  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:07  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:53:13  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:14:00  source
 * sc
 * 
 * Revision 1.2  89/03/06  14:30:09  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:37  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:40:52  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  09:57:16  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:57:02  joe
 * *** empty log message ***
 * 
**		Revision 1.2  86/04/24  17:03:01  daveb
**		take cleaner rewrite from 3.0 porting.
**		
**		Revision 30.2  86/02/07  16:37:55  seiwald
**		Cleaned up by BSS (Brower/Seiwalds/Shanklin).
**		
**		03/09/83 -- (mmm)
**			written
**		02/23/89 -- (daveb, seng)
**			rename LO.h to lolocal.h
**		28-Feb-1989 (fredv)
**			Include <systypes.h>.
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	22-mar-96 (nick)
**	    Only return size for regular files.
**	28-feb-2000 (somsa01)
**	    Added support for LARGEFILE64.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	    Declare errno correctly (i.e. via errno.h).
*/

STATUS
LOsize( loc, loc_size )
register LOCATION	*loc;
register OFFSET_TYPE	*loc_size;
{
	STATUS		LOerrno();
#ifdef LARGEFILE64
	struct stat64	statinfo;
#else /* LARGEFILE64 */
	struct stat	statinfo;
#endif /* LARGEFILE64 */

#ifdef LARGEFILE64
	if( stat64( loc->string, &statinfo ) < 0 )
#else /* LARGEFILE64 */
	if( stat( loc->string, &statinfo ) < 0 )
#endif /* LARGEFILE64 */
	{
		*loc_size = -1;
		return ( LOerrno( errno ) ); 
	}

	if ((statinfo.st_mode & S_IFMT) == S_IFREG)
	    *loc_size = (OFFSET_TYPE) statinfo.st_size;
	else
	    *loc_size = 0;

	return ( OK ); 
}
