/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<te.h>
# include	<st.h>
# include	<uigdata.h>

/*
**	FEtfile.c
**
**		Contains routine that deals with input, output testing
**		files, recovery file and frame dump files in a
**		multi-processes environment.
**
**
**	history:
**		2/85 (ac) -- created.
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
**	21-apr-92 (seg)
**	    STindex takes a pointer as second parameter.  Old calls never
**	    worked, I think.
**	10-sep-92 (leighb) DeskTop Porting Change:
**	    Make sure correct correct FRONTCL.DLL is loaded because the
**	    data 'fdrecover' and 'fdrcvfile' are across facility boundaries.
**      10-Dec-95   (fanra01)
**          Changed externs to GLOBALREFs
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
*/

GLOBALREF    FILE    *fdrcvfile;
GLOBALREF    bool    fdrecover;
GLOBALREF    FILE    *fddmpfile;

GLOBALREF bool  multp;

FEtfile(tfiles)
char	*tfiles;
{

	char	*comma;
	char	*strp;
	char	*testing;
	i4	result;

	multp = TRUE;

	comma = STindex(tfiles, ",", 0);
	strp = comma;
	strp++;
	*comma = '\0';

	comma = STindex(strp, ",", 0);
	testing = comma;
	testing++;
	*comma = '\0';

	CVal(testing, &result);
	IIUIfedata()->testing = (bool)result;

	return(fdtofp(tfiles, strp));
}


fdtofp(rcvfd, dmpfd)
char	*rcvfd;
char	*dmpfd;
{

#ifdef	PMFEWIN3
# ifndef WIN16
	IIload_frontcl(); 
# endif
#endif

	i4	fd;

	if (rcvfd == NULL)
	{
		fdrcvfile = NULL;
		fdrecover = FALSE;
	}
	else
	{
		fdrecover = TRUE;
		CVal(rcvfd, &fd);
		fdrcvfile = SIfdopen( fd, "w" );
	}
	if (dmpfd == NULL)
		fddmpfile = NULL;
	else
	{
		CVal(dmpfd, &fd);
		fddmpfile = SIfdopen( fd, "w" );
	}

	/* make stdin nonbuffered, so that in a multi-process
	** environment, parent and child processes can read the
	** same file interchangely without missing any character.
	*/

	SIunbuf(stdin);

	return(OK);
}
