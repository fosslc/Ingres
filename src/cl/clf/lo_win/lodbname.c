
# include	 <compat.h>
# include	 <gl.h>
# include	 <lo.h>
# include	 <st.h>

/*
** Copyright (c) 1985 Ingres Corporation
**
** LOdbname - get the database name from the LOCATION of the database directory
**
**	Parameters:
**		dir_loc	- LOCATION of the directory to get the dbname from
**		buf	- place to put the dbname
**
**	Returns:
**		OK	- buf has been filled with the dbname
**		FAIL	- dir_loc can't be a database directory location
**
**	History:
**	
 * Revision 1.1  90/03/09  09:15:28  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:42:52  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:52:20  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:13:07  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:30  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:40:39  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 60.2  86/12/02  09:51:02  joe
 * Initial60 edit of files
 * 
 * Revision 60.1  86/12/02  09:50:47  joe
 * *** empty log message ***
 * 
**Revision 30.1  85/08/14  19:12:33  source
**llama code freeze 08/14/85
**
**		Revision 3.0  85/05/12  01:00:20  wong
**		'STcopy()' no longer returns length, so test '*buf' directly
**		to determine that length is at least one.
**		
**		03/28/84 (lichtman)	- written
**		08/17/84 (lichtman)	- check for ".DIR" extension was
**						inverted; corrected it
**		12/12/84 (boba)		- convert for Unix.
**		4/9/85	(dreyfus)	- Make sure that the location is a
**						directory (path only)
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
*/

STATUS
LOdbname(dir_loc, buf)
LOCATION	*dir_loc;
char		*buf;
{
	if (dir_loc->desc == PATH)
	{
		STcopy(dir_loc->string, buf);	/* copy name into buffer */

		if (*buf != '\0')	/* ensure at least length 1 */
			return OK;
	}

	return FAIL;
}
