/*
**    Copyright (c) 1991, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<lo.h>
# include	<nm.h>
# include	<st.h>
# include	<ut.h>
# include	<er.h>
# include	<cv.h>

/**
** Name:	utdeffil.c - Find UTexe definition file
**
** Description:
**	This file defines:
**
**	UTdeffile	Find UTexe definition file
**
** History:
**	4/91 (Mike S) Initial version
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**/

/* # define's */
# define MAXPROGNAME	8	/* Chop program name to 8 characters */
# define FILEEXT	ERx("def")
# define DEF_UT_FILE	ERx("utexe.def")
# define DEFFILES	ERx("deffiles")

/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	UTdeffile       Find UTexe definition file
**
** Description:
**	Find the UTexe definition file for the given program.  This will be
**	II_CONFIG/deffiles/program.def if it exists; II_CONFIG/files/utexe.def
**	otherwise.
**
** Inputs:
**	progname	char *		Program name
**	buf		char *		Location buffer
**
** Outputs:
**	ploc		LOCATION *	Filled-in location
**	clerror		CL_ERR_DESC *	Error desciption
**
**	Returns:
**		OK
**		NM or LO error		Unexpected internal error
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	4/91 (Mike S) Initial version
*/
STATUS
UTdeffile(progname, buf, ploc, clerror)
char	*progname;
char	*buf;
LOCATION *ploc;
CL_ERR_DESC *clerror;
{
	STATUS status;

	/* Files directory location */
	LOCATION filedir;	
	char	 fdbuffer[MAX_LOC];

	/* program-specific UTexe definition file location */
	LOCATION psfile;
	char	psbuffer[MAX_LOC];
	char	psfilename[LO_FPREFIX_MAX+1];

	/* program-specific file name pieces */
	char filedev[LO_DEVNAME_MAX+1];
	char filepath[LO_PATH_MAX+1];
	char dum_filename[LO_FPREFIX_MAX+1];
	char dum_filetype[LO_FSUFFIX_MAX+1];
	char dum_filevers[LO_FVERSION_MAX+1];

	/* LO information stuff */
	LOINFORMATION locinfo;
	i4 flags = 0;

	/* Clear the CL_ERR_DESC */
	CL_CLEAR_ERR(clerror);

	/* First, get the files directory */
	if ((status = NMloc(FILES, PATH, (char *)NULL, &filedir)) != OK)
		return status;
	LOcopy(&filedir, fdbuffer, &filedir);

	/* Construct the prefix part of the program-specific file name */
	STlcopy(progname, psfilename, MAXPROGNAME);
	CVlower(psfilename);

	/* 
	** Construct a LOCATION for the program-specific file, and check for
	** its existence.
	*/
	LOcopy(&filedir, psbuffer, &psfile);
	if ((status = LOfaddpath(&psfile, DEFFILES, &psfile)) == OK &&
	    (status = LOdetail(&psfile, filedev, filepath, dum_filename,
			       dum_filetype, dum_filevers)) == OK &&
	    (status = LOcompose(filedev, filepath, psfilename, FILEEXT,
				(char *)NULL, &psfile)) == OK &&
	    (status = LOinfo(&psfile, &flags, &locinfo)) == OK )
	{
		/* The program-specific file exists */
		LOcopy(&psfile, buf, ploc);
	}
	else
	{
		/* 
		** Something went wrong with the program-specific one;
		** return the default one.
		*/
		if ((status = LOfstfile(DEF_UT_FILE, &filedir)) == OK)
			LOcopy(&filedir, buf, ploc);
	}
	return status;
}
