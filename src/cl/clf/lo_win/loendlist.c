
# include	<compat.h>
# include	<gl.h>
# include       <clconfig.h>
# include 	<systypes.h>
# include	<lo.h>
# include	"lolocal.h"
# include	<diracc.h>

/*
** Copyright (c) 1985 Ingres Corporation
**
**	Name:	LOendlist
**
**	Function:
**		LOendlist is called in conjunction with LOlist.
**		LOendlist is called when one wishes to end a directory
**		search before LOlist reaches end of file.
**
**	Arguments:
**		loc : a pointer to the location whose directory LOlist is
**		      is searching.
**
**	Result:
**		The file pointer opened in order to do the list is closed.
**
**	Side Effects:
**
**	History:
**	
 * Revision 1.1  90/03/09  09:15:29  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:42:55  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:52:28  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:13:15  source
 * sc
 * 
 * Revision 1.2  89/03/06  14:29:32  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:31  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:40:41  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  09:52:10  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:51:54  joe
 * *** empty log message ***
 * 
**		Revision 1.2  86/04/24  17:23:17  daveb
**		Take clean 3.0 porting version using the
**		opendir emulation package.
**		
**		4/04/83 -- (mmm) written
**		7/20/83 -- (fhc) changed with ifdefs to handle
**			directory closing on unix 41c
**		02/23/89 -- (daveb, seng)
**			rename LO.h to lolocal.h
**			moved dir.h to hdr/diracc.h
**                      so we can share directory routines with di.
**		28-Feb-1989 (fredv)
**			include <systypes.h>.
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

STATUS
LOendlist(loc)
register LOCATION	*loc;
{
	/*
	** We cast this to (FILE *) when we set it in LOlist().
	** We now corece it back to it's real value
	** before using it 
	*/
	closedir( (DIR *)loc->fp );

	loc->fp = NULL;

	return(OK);
}
