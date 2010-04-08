/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/

# include 	<compat.h>
# include 	<gl.h>
# include 	<systypes.h>
# include	<lo.h>
# include	<sys/stat.h>

/*
**	Name:
**		LOisdir
**
**	Function:
**		LOisdir makes a system call to determine if a location is a
**		directory.  Return fail if something else.
**
**	Arguments:
**		loc -- a pointer to the location.
**
**	Result:
**		Returns OK if location is a directory.
**
**	Side Effects:
**
**	History:
**	
 * Revision 1.1  90/03/09  09:15:31  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:03  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:52:53  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:13:40  source
 * sc
 * 
 * Revision 1.2  89/03/06  11:58:51  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:34  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:40:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  09:55:05  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:54:49  joe
 * *** empty log message ***
 * 
**		Revision 1.2  86/04/24  17:22:11  daveb
**		Take cleaner rewrite from 3.0 porting.
**		
**		Revision 30.2  86/02/07  16:31:17  seiwald
**		Cleaned by BSS (brower, seiwald and shanklin).
**		
**		4/4/83 -- (mmm) written
**		28-Feb-1989 (fredv)
**			Include <systypes.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	06-may-1996 (canor01)
**	    Clean up compiler warnings (NULL != EOS).
**	31-jul-2001 (somsa01)
**	    Use _stati64() if LARGEFILE64 is defined.
*/

STATUS
LOisdir( loc, flag )
register LOCATION 	*loc;
register i2		*flag;
{

#ifdef LARGEFILE64
	struct _stati64	statinfo;

	if( 0 == _stati64( loc->string, &statinfo ) )
#else
	struct stat	statinfo;
	
	if( 0 == stat( loc->string, &statinfo ) )
#endif  /* LARGEFILE64 */
	{
		*flag = statinfo.st_mode & S_IFDIR ? ISDIR : ISFILE;
		return( OK );
	}
	*flag = EOS;
	return( FAIL );
}
