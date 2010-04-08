/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<si.h>
# include	<lo.h>
# include	<st.h>
# include	<nm.h>
# include	"nmlocal.h"
# ifndef DESKTOP
# include	<pwd.h>
# endif

/**
** Name: NMPATHING.C - Get Ingres home directory path.
**
** Description:
**
**	Return the path of the root of the Ingres installation:
**		II_SYSTEM!ingres
**	II_SYSTEM must be defined on UNIX in the user's environment.
**	A Null is returned if the name is undefined.
**
**	This file contains the following external routine:
**      	NMpathing() - Get root of Ingres installation
**
** History: Log:	nmpathing.c,v
**      
**      Revision 1.6  88/01/07  16:17:49  mikem
**      Home directory of the installation is now given by II_SYSTEM only.
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>
**	19-jan-1990 (greg)
**	    zap obsolete junk.
**	    someone should get rid of static buffer.
**	25-jan-1989 (greg)
**          zap obsolete comments
**	    use getenv for II_SYSTEM, don't bother with NMgtAt
**	14-oct-1994 (nanpr01)
**          removed #include "nmerr.h". Bug # 63157. 
**	10-may-1995 (emmag)
**	    Desktop porting changes.
**	03-jun-1996 (canor01)
**	    Removed semaphore protection of static buffer.  Function not
**	    used in threaded server.
**	18-oct-96 (mcgem01)
**	    Remove hard-coded ingres references.
**	29-sep-2000 (devjo01)
**	    Add check for excessively large value for II_SYSTEM.  Use
**	    MAX_LOC for t_Pathname declaration.
**/

/* # defines */
/* typedefs */
/* forward references */

/* externs */

/* statics */

static	char		t_Pathname[MAX_LOC+1] = "";


/*{
** Name: NMpathIng - Get Ingres home directory path.
**
** Description:
**    Return the value of an attribute which may be systemwide
**    or per-user.  The per-user attributes are searched first.
**    A Null is returned if the name is undefined.
**
** Re-entrancy:
**	Not.
**
** Inputs:
**	none.
**
** Output:
**	name				    path to ingres home directory.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          NM_INGUSR                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	26-jan-1989 (greg)
**	    Use getenv.  Requirement is that II_SYSTEM be defined in
**	    the users environment, why mess around?
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	29-sep-2000 (devjo01)
**	    Add check for excessively large value for II_SYSTEM.
*/
STATUS
NMpathIng(name)
char	**name;
{
	char	*t_PathPtr;
	STATUS	status = OK;


	if (t_Pathname[0] == '\0')
	{
	    if (t_PathPtr = getenv( SystemLocationVariable ))
	    {
		if ( STlength(t_PathPtr) + 1 +
		      STlength(SystemLocationSubdirectory) >
		     MAX_LOC )
		{
		    status = NM_TOOLONG;
		}
		else
		{
# ifdef DESKTOP
	            STpolycat( 3, t_PathPtr, "\\", 
			   SystemLocationSubdirectory, t_Pathname);
# else /* DESKTOP */
	            STpolycat( 3, t_PathPtr, "/",
			   SystemLocationSubdirectory, t_Pathname);
# endif /* DESKTOP */
		}
	    }
	    else
	    {
	        status = NM_INGUSR;
	    }
	}

	*name = t_Pathname;

	return(status);
}
