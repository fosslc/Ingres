/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<er.h>
# include	<uigdata.h>

/*
**  FEfile.c
**
**	Contains routine that deals with input, output testing
**	files, recovery file and frame dump files in a
**	multi-processes environment.
**
**  HISTORY:
**	2/85 (ac) -- created.
**	7/85 (ac/bab) -- use STcopy()/STpolycat() instead of
**		STprintf() since SIgetfd() uses an internal static.
**	7/85 (bab) -- construct a third 'number' to add on to the
**		returned string indicating if IIUIgdata->testing should be
**		set in the sub-process.
**	19-jun-87 (bab)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	28-aug-1990 (Joe)
**	    Change IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
**	27-dec-1994 (canor01)
**	    added prototype for "obsolete" function SIgetfd()
**      08-Feb-96 (fanra01)
**          Changed externs to GLOBALDEF as not defined anywhere.
**	05-mar-96 (albany)
**	    Changed previous change to GLOBALREF after receiving 
**	    'multiply defined symbol' warnings; fdrcvfile is 
**	    GLOBALDEFed in !frontcl!ft ftsvinpt.c, and fddmpfile 
**	    in !frame!frame frframe.c.
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
*/

GLOBALREF	FILE	*fdrcvfile;
GLOBALREF	FILE	*fddmpfile;

char *
FEfile()
{
	static	char	buf[6];
	char	testing[2];
        static char     s[10];

	if (fdrcvfile != NULL)
		SIflush(fdrcvfile);
	if (fddmpfile != NULL)
		SIflush(fddmpfile);
	if (IIUIfedata()->testing)
		testing[0] = '1';
	else
		testing[0] = '0';
	testing[1] = '\0';

#ifndef FT3270
	SIflush(stdout);
#endif
        if (fdrcvfile == NULL)
        {
                s[0] = '0';
		s[1] = '\0';
        }
        else
                CVla( SIfileno( fdrcvfile), s);

	STcopy(s, buf);
	STpolycat(5, buf, ERx(","), s, ERx(","), testing, buf);
	return (buf);
}
