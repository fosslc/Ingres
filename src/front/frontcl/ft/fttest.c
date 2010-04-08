# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	"ftframe.h"
# include	<er.h>
# include	<te.h>
# include	<erft.h>

#ifdef SEP

# include	<st.h>
# include	<lo.h>
# include	<tc.h>

GLOBALREF   TCFILE  *IIFTcommfile;	/* pointer to output COMM-file */

#endif /* SEP */


/*
**  FTtest
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  History:
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	08/28/87 (dkh) - Changes for 8000 series error numbers.
**	09/11/87 (dkh) - Use IIUGerr instead of FTgeterr().
**	05/22/89 (Eduardo) - Changed call to TEtest to comply with new
**			     subroutine format
**	07/03/90 (dufour) - Removed underscore character for SEP files
**			    when used with MPE XL OS.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
FTtest(testin, testout)
char	*testin;
char	*testout;
{
	i4 in_type,out_type;

	in_type = (testin && *testin != '\0') ? TE_SI_FILE : TE_NO_FILE;
	out_type = (testout && *testout != '\0') ? TE_SI_FILE : TE_NO_FILE;
	if (TEtest(testin, in_type, testout, out_type) != OK)
	{
		IIUGerr(E_FT003A_IONOREDIR, 0, NULL);
		return(FAIL);
	}
	return(OK);
}


#ifdef SEP

/*
**
**  FTtestsep
**
**  routine to initializa pointer to output COMM-file
**  while working in SEP mode
**  It's called by iitest (in runtime library)
**
**  History:
**  25-Apr-89	Eduardo created it
**
*/

STATUS
FTtestsep(flaginfo,infile)
char	*flaginfo;
TCFILE	**infile;
{
    char	*comma;
    LOCATION	floc;
    char	sepinfile[MAX_LOC + 1],sepoutfile[MAX_LOC + 1];
    bool	noout = TRUE;
    i4		out_type;

    /* check if working in batch mode */

    if ((comma = STindex(flaginfo,ERx(","),0)) != NULL)
	noout = FALSE;
    if (!noout && *(comma + 1) != 'B' && *(comma + 1) != 'b')
	noout = TRUE;
    if (comma)
	*comma = '\0';

    /* name input COMM-file */

# ifdef hp9_mpe
    STprintf(sepinfile,ERx("in%s.sep"),flaginfo);
# else
    STprintf(sepinfile,ERx("in_%s.sep"),flaginfo);
# endif
 
    /* name output RES file if working in batch mode */

    if (!noout)
    {
# ifdef hp9_mpe
	STprintf(sepoutfile,ERx("b%s.sep"), flaginfo);
# else
	STprintf(sepoutfile,ERx("b_%s.sep"), flaginfo);
# endif
	out_type = TE_SI_FILE;
    }
    else
    {
	*sepoutfile = '\0';
	out_type = TE_NO_FILE;
    }

    /* redirect input to COMM-file */

    if (TEtest(sepinfile, TE_TC_FILE, sepoutfile, out_type) == FAIL)
	return(FAIL);

    /* open output COMM file */

# ifdef hp9_mpe
    STprintf(sepoutfile,ERx("out%s.sep"),flaginfo);
# else
    STprintf(sepoutfile,ERx("out_%s.sep"),flaginfo);
# endif

    LOfroms(FILENAME & PATH,sepoutfile, &floc);
    if (TCopen(&floc, ERx("w"), &IIFTcommfile) != OK)
    	return(FAIL);

    /* send beginning of session message */

    TCputc(TC_BOS,IIFTcommfile);
    TCflush(IIFTcommfile);

    /* initialize FRAME/RUNTIME pointer */

    *infile = IIFTcommfile;

    return(OK);

}

#endif /* SEP */

