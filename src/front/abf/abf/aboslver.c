/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ci.h>


/**
** Name:	aboslver.c	-  Figure out what OSL version is running.
**
** Description:
**	This is only used on VMS where we support old OSL and new OSL.
**	On all other systems we only support new OSL.
**
**	aboslver	Initialize OSL version.
**
** History:
**	19-jun-1987 (Joe)
**		Moved from abf.c
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**/

GLOBALDEF i2	osNewVersion;

/*{
** Name:	aboslver	-  Initialize the OSL version.
**
** Description:
**	Initialize the OSL version.  On most OS this is a noop.	 On VMS
**	check to capability settings.  If either OSL/QUEL or OSL/SQL are
**	enabled, then it is new OSL.
**
** History:
**	19-jun-1987 (Joe)
**		Initial Version
*/
aboslver()
{
# ifdef VMS
	char	*cp;

	osNewVersion = 1;
	/*
	** Some VMS customers may have old version of OSL.
	** Want a way to test, so allow a logical name to set.
	*/
	NMgtAt(ERx("II_OSL_VERSION"), &cp);
	if (cp != NULL)
	{
		CVlower(cp);
		osNewVersion = !(*cp == 'o');
	}
# else
	osNewVersion = 1;
# endif /* VMS */
}
