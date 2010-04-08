/*
** Copyright (c) 1986, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <me.h>
#include    <st.h>
#include    <rms.h>
#include    <starlet.h>
#include    "pelocal.h"		    /* header file for PE stuff */

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
**
**  History:    $Log-for RCS$
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes amd
**	   external function references
**/

FUNC_EXTERN	VOID LOdir_to_file();


/*
**  Forward and/or External typedef/struct references.
*/

typedef struct NAM	_NAM;
typedef struct FAB	_FAB;
typedef struct XABPRO	_XAB;


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
*/

STATUS
PEumask(pattern)
char		*pattern;
{
	u_i2	permbits	= NONE;


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
	/* give system all permissions */
	permbits |= SYSRWED;

	permbits = ~permbits;
	sys$setdfprot(&permbits, 0);
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
*/

STATUS
PEworld(pattern,loc)
char		*pattern;
LOCATION	*loc;
{
	i4	pstatus = UNKNOWN;
   	_FAB	fab;
	_XAB	xab;
	char	dirfile[MAX_LOC];
	char	*strptr;
	STATUS	PEstatus = OK;
	u_i2	oldprot;

	LOtos(loc, &strptr);
	if (*strptr == '\0')
	{
		return(PE_NULL_LOCATION);
	}
	if (*pattern == '\0')
	{
		return(PE_BAD_PATTERN);
	}


	/* 
	** Conert directory format to file format, if need be.
	*/
	if((loc->desc | FILENAME) != FILENAME)
	{
		LOdir_to_file(strptr, dirfile);
		strptr = dirfile;
	}		

	/*	Initialize the RMS fab		*/

	MEfill(sizeof(fab), NULL, (PTR)&fab);
	fab.fab$b_bid = FAB$C_BID;
	fab.fab$b_bln = FAB$C_BLN;
	fab.fab$b_fac = FAB$M_PUT;
	fab.fab$l_fop = 0;
	fab.fab$b_fns = STlength(fab.fab$l_fna = strptr);
	fab.fab$l_xab = &xab;

	/* 	Initialize the RMS xab		*/
	
	MEfill(sizeof(xab), NULL, (PTR)&xab);
	xab.xab$b_cod = XAB$C_PRO;
	xab.xab$b_bln = XAB$C_PROLEN;


	/*	See if we can open it			*/

	if ((sys$open(&fab) & 1) == 0)
		return (FAIL);
	

	oldprot = xab.xab$w_pro = ~xab.xab$w_pro;
	while ((*pattern != '\0') && (PEstatus == OK))
	{
		switch (*pattern)
		{
			case '+':
				if (pstatus != UNKNOWN)
				{
					PEstatus = PE_BAD_PATTERN;
				}
				pstatus = GIVE;
				break;
			case '-':
				if (pstatus != UNKNOWN)
				{
					PEstatus = PE_BAD_PATTERN;
				}
				pstatus = TAKE;
				break;
			case 'r':
				if (pstatus == GIVE)
					xab.xab$w_pro |= WREAD;
				else
					xab.xab$w_pro &= ~WREAD;
				pstatus = UNKNOWN;
				break;
			case 'w':
				if (pstatus == GIVE)
					xab.xab$w_pro |= WWRITE;
				else
					xab.xab$w_pro &= ~WWRITE;
				pstatus = UNKNOWN;
				break;
			case 'x':
				if (pstatus == GIVE)
					xab.xab$w_pro |= WEXEC;
				else
					xab.xab$w_pro &= ~WEXEC;
				pstatus = UNKNOWN;
				break;
			default:
				PEstatus = PE_BAD_PATTERN;
				break;
		}
		pattern++;
	}
	if (pstatus != UNKNOWN)
	{
		PEstatus = PE_BAD_PATTERN;
	}
	xab.xab$w_pro = (PEstatus == OK) ? ~xab.xab$w_pro : ~oldprot;
	if((sys$close(&fab) & 1) == 0)
		return(FAIL);
	return(PEstatus);
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
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.  Also made this a VOID
**	    function.
*/

static u_i2	prot = 0;

VOID
PEsave()
{
	sys$setdfprot(0,&prot);
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
**	    Static variable "prot" is needed for the VMS version of this code.
**
** History:
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 standards.  Also made this a VOID
**	    function.
*/

VOID
PEreset()
{
	sys$setdfprot(&prot, 0);
	return;
}
