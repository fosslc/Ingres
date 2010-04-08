/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<lo.h>
# include	<nm.h>
# include	<ds.h>
# include	<ut.h>
# include	<er.h>
# include	<errno.h>
# include	<cv.h>
# include	<st.h>

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
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-may-97 (mcgem01)
**	    Clean up compiler warnings by adding includes for st.h and cv.h
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
**	19-apr-95 (canor01)
**	    added <errno.h>
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

	/* 
	** Clear the CL_ERR_DESC (it would be nice if this were 
	** system-independent).
	*/
# ifdef VMS
	clerror->error = 0;
# else
# ifdef UNIX
	CL_CLEAR_ERR( clerror );
# endif
# endif

	/* First, get the files directory */
	if ((status = NMloc(FILES, PATH, (char *)NULL, &filedir)) != OK)
		return status;
	LOcopy(&filedir, fdbuffer, &filedir);

	/* Construct the prefix part of the program-specific file name */
	STncpy( psfilename, progname, MAXPROGNAME);
	psfilename[ MAXPROGNAME ] = EOS;
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
