/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<nm.h>
#include	<lo.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:	ughlpnm.c -	Front-End Help File Name Utility.
**
** Description:
**	Contains the interface for the front-end help filename utility,
**	which returns the full pathname of the installed help file.  Defines:
**
**	IIUGhlpname()	gets the help file name.
**	IIUGbfd_buildFilesDirname()	builds
**
** History:
**	Revision 6.2  88/12  wong
**	Renamed from 'FEhlpnam()' and moved here from "runtime!fehelp.qsc".
**
**	Revision 6.0  87/08  peter
**	08/16/87 (peter) - Added support for II_HELPDIR, and added
**			support for multiple language versions
**			of help files.  Also, add FEhlpnam
**	3/22/90  (pete)	Created IIUGbfl_buildFilesLoc() to be called from
**			IIUGhlpname().
**	21-Nov-92 (fredb)
**		Added a parameter to IIUGbfl_buildFilesLoc to solve LO
**		usage problem.  Changes to IIUGhlpname & IIUGbfl_... to
**		use the new parameter and complete the LO usage fix.
**	21-Nov-92 (fredb)
**		MPE/iX (hp9_mpe) specific fix to IIUGhlpname().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS IIUGbfl_buildFilesLoc();

/*{
** Name:	IIUGhlpname() -	Get The Help File Name.
**
** Description:
**	Returns the full path of the help file name for an RT defined help
**	file, using the conventions of the standard installations.  This
**	firsts checks if the II_HELPDIR name is defined, and if so, uses
**	that directory plus the file name to generate the help file name.  If
**	it is not defined, it will use the II_CONFIG/II_LANGUAGE directory.
**
** Inputs:
**	filename	{char *}  name of help file.
**
** Outputs:
**	fullpath	{char [MAX_LOC]}  full path name of file.
**			  This will be filled in by the routine.  It
**			  should be a buffer at least MAX_LOC in length.
** Returns:
**	{STATUS}  OK or FAIL if error occurs.  This will print errors.
**
** History:
**	17-aug-1987 (peter)
**		Written.
**	21-Nov-1992 (fredb)
**		Changes to support new calling sequence for IIUGbfl_...
**		New variable, helpbuf.
**	21-Nov-92 (fredb)
**		MPE/iX (hp9_mpe) specific fix: Due to limited name space,
**		we don't support file names and directories in the normal
**		way.  We're getting a clash between the incoming filename
**		and the PATH to the help files here.  To solve the problem,
**		the extension must be removed from the filename.
*/

STATUS
IIUGhlpname ( filename, fullpath )
char	*filename;
char	*fullpath;
{
	STATUS		clerror;
	LOCATION	helploc;
	char		helpbuf[ MAX_LOC + 1];
	char		*hfile;

	clerror = IIUGbfl_buildFilesLoc(ERx("II_HELPDIR"), &helploc, helpbuf);

	if ( clerror == OK )
	{ /* Now add the help file name */
#ifdef hp9_mpe
		/*
		** Remove the extension from the filename by terminating
		** the string sooner.  "Borrow" hfile var for a few lines ...
		*/
		if ((hfile = STindex(filename, ".", 0)) != (char *) NULL)
		   *hfile = EOS;
#endif
		clerror = LOfstfile(filename, &helploc);
	}

	if ( clerror != OK )
	{
		IIUGerr(clerror, 0, 0, 0);
		return clerror;
	}

	LOtos(&helploc, &hfile);
	STcopy(hfile, fullpath);

	return OK;
}

/*{
** Name:	IIUGbfl_buildFilesLoc - Build LOCATION for FILES/Language dir.
**
** Description:
**	Build a LOCATION structure for the Language subdirectory of the
**	FILES directory. The name of an environment variable is also passed;
**	if that environment var is set, then set LOCATION structure to its
**	translation rather than using FILES/Language.
**	
** Inputs:
**	char	 *env_var	Name of an environment variable.
**	LOCATION *loc		Location structure to write to.
**	char	 *buf		Buffer associated with location structure.
**
** Outputs:
**	LOCATION *loc and char *buf are filled in.
**
**	Returns:
**		OK if all went well, else != OK.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	3/22/90	(pete)	Initial Version (extracted code from IIUGhlpname).
**	21-Nov-92 (fredb)
**		"LO" claims another victim!  The location structure does
**		not contain the actual buffer, just pointers to it.  LOfroms
**		actually associates the string with the location structure.
**		The bug was that the location structure came from the caller
**		and the buffer (string) was local here.  When we leave this
**		function, the memory for the buffer goes away so the caller
**		is stuck with a location associated with whatever gets left
**		on the stack.  Not good to return a pointer to a local
**		variable.
**
**		I added another parm to the calling sequence for the buffer
**		now supplied by the caller and fixed the code to use it.
**	17-Jun-2004 (schka24)
**	    Safer env variable handling.
*/
STATUS
IIUGbfl_buildFilesLoc (env_var, loc, buf)
char	 *env_var;
LOCATION *loc;
char	 *buf;
{
	STATUS		clerror = OK;
	char		*dirname;
	char		langstr[ER_MAX_LANGSTR + 1];

	/* If "env_var" defined, use it */

	NMgtAt(env_var, &dirname);
	if ( dirname != NULL && *dirname != EOS )
	{
		/* the environment variable is defined */

		STlcopy(dirname, buf, MAX_LOC-1);
		clerror = LOfroms(PATH, buf, loc);
	}
	else
	{
		/*
		** Environment variable not defined, so use FILES.
		** First get language name to add to FILES directory.
		*/
		i4	langcode;

		clerror = ERlangcode(NULL, &langcode);
		if (clerror == OK)
		{   /* If ERlangcode works, ERlangstr will too */
			_VOID_ ERlangstr(langcode, langstr);
			clerror = NMloc(FILES, PATH, langstr, loc);
			/*
			** NMloc associates the location with a static
			** buffer.  For safety, we'll use the caller's
			** buffer instead.
			*/
			if (clerror == OK)
			   LOcopy(loc, buf, loc);
		}
	}

	return clerror;
}
