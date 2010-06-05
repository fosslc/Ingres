/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
#include	<cm.h>
#include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<st.h>
#include	<erug.h>
#include	"eric.h"
#include	"global.h"
#include 	<cv.h>

/**
** Name:    util.c -	AccessDB Utilities Module.
**
** History:
**	Revision 40.4  86/01/20  14:42:48  annie
**	bug fix # 6436.
** 
**	Revision 40.3  85/10/30  16:14:22  peterk
**	Y-line integration into rplus.
**	add RCS comments, createdb comments
** 
**	Revision 30.5  86/03/14  12:30:30  garyb
**	IBM porting changes.
**
**	Revision 30.4  85/10/08	 16:32:14  jas
**	ibs integration
**
**	Revision 30.3  85/09/18	 14:48:32  cgd
**	Use PEumask() to override user's umask to insure ingres can
**	access database.
**
**	15-nov-95 (pchang)
**	    Added routine chk_loc_area
**	24-jun-97 (i4jo01)
**	    Allow non-ingres user to add locations.
** 	18-jun-1997 (wonst02)
** 	    Do not fail if the directory is not writable...OK for readonly db.
**	02-jul-1999 (hanch04)
**	    Added create_loc_area for extenddb 
**      18-Jan-2000 (wanya01)
**          Bug 87619. Take out previous change 430844(made on 24-jun-97) 
**          which fixed bug 81151. Bug 81151 is actually a documentation bug.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-May-2001 (jenjo02)
**	    Replaced chk_loc_area() functionality with iiQEF_check_loc_area
**	    internal database procedure to resolve longstanding
**	    permissions problems with non-Ingres users.
**	15-Oct-2001 (jenjo02)
**	    Removed create_loc_area(). Functionality is now performed
**	    within the Server rather than locally.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

/*{
** Name:    chkdbname() -	Check Name for DBMS Validity.
**
**	The name of a database is checked for validity.  A valid
**	database name is not more than DB_MAXNAME characters long,
**	begins with an alphabetic character, and contains only alpha-
**	numerics.  Underscore is not allowed.
**
**	THIS ROUTINE CONVERTS THE NAME TO LOWER CASE
**
** Input:
**	name	{char *}  the string to check.
**
** Returns:
**	{STATUS}	OK or FAIL.
**
** History:
**	12/87 (jhw) - replaced with copy of 'FEchkname()'.
**	4/87 (peterk) - brought into ingcntrl/util.c,
**		adapted from back/iutil/checkdbn.c
*/

STATUS
chkdbname (name)
register char	*name;
{
    register char	*cp = name;

    if (CMnmstart(cp))
    {
	do {
	    CMnext(cp);
	} while (*cp != EOS && CMnmchar(cp));
    }

    if (*cp == EOS && cp - name <= DB_MAXNAME)
    {
	CVlower(name);
	return OK;
    }
    else
	return FAIL;
}

/*
** Name:        puborpriv() -- tells if database is public or private.
**
** Description:
**      translate whether database has global access or not into the strings
**      "public" or "private".
*/
char *
puborpriv (b)
i4      b;
{
    return ( b ? ERget(F_IC000C_public)
	       : ERget(F_IC000D_private));
}
