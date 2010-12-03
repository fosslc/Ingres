
/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* include the compatiblity header */
# include	<gl.h>
# include	<cs.h>
# include	<st.h>
# include	<systypes.h>
# include	<lo.h>			/* LOCATION library header file */
# include	<pe.h>
# include	<sys/stat.h>		/* header for info on a file */
# include	"pelocal.h"		/* PE local header file */

/**
**
**  Name: PE.C - Contains file permissions routines.
**
**  Description:
**      This file contains all of the PE module routines as defined in the 
**      CL spec.
**
**	IMPORTANT NOTE:
**	==============
**	The routines in this file are not meant to be used in a server
**	environment.
**
**          PEumask() - Set file creation mode mask.
**          PEworld() - Set world permissions.
**          PEsave() - Save current default permissions.
**          PEreset() - Reset default permissions to those last saved by PEsave.
**
**	Note(s):
**		The routines in this file use 'LOtos' and expect this
**		routine to always work.  Even if a null string is
**		returned.
**
**		The routines in here only work for locations on a single
**		machine.  When we go to network locations, the routines
**		here will have to be updated to handle network (remote)
**		locations.
**
**  History:    
 * Revision 1.1  90/03/09  09:16:20  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:45:02  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:59:18  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:19:10  source
 * sc
 * 
 * Revision 1.2  89/03/06  11:59:08  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:13:13  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:44:10  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  14:56:42  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 standards.
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	06-jun-1995 (canor01)
**	    semaphore protect static variable
**	03-jun-1996 (canor01)
**	    removed above semaphore--code must be protected by caller
**	    if used in a multi-threaded server
**	15-nov-2010 (stephenb)
**	    Include pe.h for prototypes.
**/


/*
**  Forward and/or External typedef/struct references.
*/

/* static */

static int	prot;		/* used to save default permissions */


/*{
** Name: PEumask() - Set file creation mode mask.
**
** Description:
**      Set file creation mode mask for the current process.  The pattern must
**      be exactly six (6) characters long and must be in a specific order.  The
**      first three (3) characters define the read (r), write (w), and execute
**      (x) permissions for the owner, in that order.  The last three (3)
**      characters are the permissions for the world.  Any of the permissions
**      may be given as a '-' meaning that the permission in the corresponding
**      location is not to be set.  Group permissions, if they exist, are set
**      the same as world.  System permissions, if they exist, are all set on
**      (i.e. s:rwed). 
**
**	    (Editor's comment:  What does the following note mean?)
**	Note:  UNIX & VMS demand that a set bit means no permission. Therefore,
**      the the complement of the permissions mask must be passed to the system.
**
** Inputs:
**      pattern                         The permissions pattern to use.
**
** Outputs:
**      none
**
**	Returns:
**	    OK
**	    PE_BAD_PATTERN
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 standards.
*/
STATUS
PEumask(pattern)
char		*pattern;
{
	i4	permbits	= NONE;


	if (*pattern == '\0' || STlength(pattern) != UMASKLENGTH)
	{
		return(PE_BAD_PATTERN);
	}
	if (pattern[0] == 'r')
	{
		permbits |= OREAD;
	}
	else if (pattern[0] != '-')
	{
		return(PE_BAD_PATTERN);
	}
	if (pattern[1] == 'w')
	{
		permbits |= OWRITE;
	}
	else if (pattern[1] != '-')
	{
		return(PE_BAD_PATTERN);
	}
	if (pattern[2] == 'x')
	{
		permbits |= OEXEC;
	}
	else if (pattern[2] != '-')
	{
		return(PE_BAD_PATTERN);
	}
	if (pattern[3] == 'r')
	{
		permbits |= WREAD;
	}
	else if (pattern[3] != '-')
	{
		return(PE_BAD_PATTERN);
	}
	if (pattern[4] == 'w')
	{
		permbits |= WWRITE;
	}
	else if (pattern[4] != '-')
	{
		return(PE_BAD_PATTERN);
	}
	if (pattern[5] == 'x')
	{
		permbits |= WEXEC;
	}
	else if (pattern[5] != '-')
	{
		return(PE_BAD_PATTERN);
	}
	umask(~permbits);
	return(OK);
}

/*{
** Name: PEworld() - Set world permissions.
**
** Description:
**      Set world (access by every body) permissions on location "loc" to some
**	combination of read (r), write (w), and execute (x), with a "pattern"
**	such as "+w", "+r-w", "-x-w", etc.  The absence of a letter implies that
**      the permission is not affected.  A pattern such as "-r+r" is allowed and
**      means a no-op. 
**
**	Note:  This algorithm allows the user to specify a permission in the
**      pattern as many times as he wants.  As long as the pattern obeys the
**      correct syntax. 
**
**      Note:  On UNIX, the group permissions must be set in tandem with the
**      world permissions for things to work properly.
**
** Inputs:
**      pattern                         The permissions pattern to use.
**      loc                             The location to set permissions on.
**
** Outputs:
**      none
**
**	Returns:
**	    OK
**	    FAIL
**	    PE_NULL_LOCATION
**	    PE_BAD_PATTERN
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 standards.
*/
STATUS
PEworld(pattern,loc)
char		*pattern;
LOCATION	*loc;
{
	char		*str;
	i4		pstatus = UNKNOWN;
	i4		permbits;
	struct stat	statbuf;


	LOtos(loc, &str);

	if (*str == '\0')
	{
		return(PE_NULL_LOCATION);
	}

	if (*pattern == '\0')
	{
		return(PE_BAD_PATTERN);
	}

	if (stat(str, &statbuf) != 0)
	{
		return(FAIL);
	}

	permbits = statbuf.st_mode;

	while (*pattern != '\0')
	{
		switch (*pattern)
		{
			case '+':
				if (pstatus != UNKNOWN)
				{
					return(PE_BAD_PATTERN);
				}
				pstatus = GIVE;
				break;
			case '-':
				if (pstatus != UNKNOWN)
				{
					return(PE_BAD_PATTERN);
				}
				pstatus = TAKE;
				break;
			case 'r':
				if (pstatus == TAKE)
				{
					if (permbits & READ)
					{
						permbits &= ~READ;
					}
				}
				else
				{
					permbits |= AREAD;
				}
				pstatus = UNKNOWN;
				break;
			case 'w':
				if (pstatus == TAKE)
				{
					if (permbits & WRITE)
					{
						permbits &= ~WRITE;
					}
				}
				else
				{
					permbits |= AWRITE;
				}
				pstatus = UNKNOWN;
				break;
			case 'x':
				if (pstatus == TAKE)
				{
					if (permbits & EXEC)
					{
						permbits &= ~EXEC;
					}
				}
				else
				{
					permbits |= AEXEC;
				}
				pstatus = UNKNOWN;
				break;
			default:
				return(PE_BAD_PATTERN);
				break;
		}
		pattern++;
	}

	if (pstatus != UNKNOWN)
	{
		return(PE_BAD_PATTERN);
	}

	if (permbits == statbuf.st_mode)
	{
		return(OK);
	}

	if (chmod(str, permbits) == 0)
	{
		return(OK);
	}
	else
	{
		return(FAIL);
	}
}

/*{
** Name: PEsave() - Save current default permissions.
**
** Description:
**      Saves the current default permissions for this process.  This is a NOOP
**      on systems which do not support the concept of default permissions. 
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Static variable "prot" is needed for the VMS version of this code.
**
** History:
**	01/85 (jhw) -- Implemented for UNIX.
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.  Also made this a VOID
**	    function.
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 standards.
*/
VOID
PEsave()
{
# ifdef VMS
	sys$setdfprot(0,&prot)
# else		/* UNIX */
	prot = umask(prot);	/* get current mask */
	(VOID) umask(prot);	/* reset */
# endif
	return;
}

/*{
** Name: PEreset() - Reset default permissions to those last saved by PEsave.
**
** Description:
**      Restores the default permissions saved by PEsave.  This is a NOOP
**      on systems which do not support the concept of default permissions. 
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Static variable "prot" is needed for the UNIX and VMS version of 
**		this code.
**
** History:
**	01/85 (jhw) -- Implemented for UNIX.
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.  Also made this a VOID
**	    function.
**      21-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 standards.
*/

VOID
PEreset(void)
{
# ifdef VMS
	sys$setdfprot(&prot);
# else		/* UNIX */
	(VOID) umask(prot);	/* reset saved protection mask */
# endif
	return;
}
