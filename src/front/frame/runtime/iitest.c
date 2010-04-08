/*
**  Copyright (c) 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<nm.h>
# include	<cm.h>
# include	<si.h>
# include	<lo.h>
# include	<st.h>
# include	<er.h>
# include	<uigdata.h>
# include	"erfr.h"

#ifdef SEP

# include	<tc.h>

GLOBALREF	TCFILE	*IIFDcommfile;

FUNC_EXTERN	STATUS	FTtestsep();	/* routine to redirect output to COMM-file */

#endif /* SEP */


/*
**  IITEST - Contains IItestfrs() routine to manage test programs.  This is
**	     called at ## forms and at ## endforms.
**
**  Uses:
**	 System logical attribute II_FRSFLAGS that can define the test flags
**	 used for the current program.
**
**  Flags:
**	 -I	-	Save user form input to a file.
**	 -D	-	Dump all form screens to a file.
**	 -Z	-	Given an input file, dump form output to a file
**			or stdin.
**  Example Usage:
**	 VMS:	$ define ii_frsflags "-If1.i -Df1.d"
**		$ run prog
**
**  History:
**		30-may-1984	- Written (ncg)
**		2/85	(ac)	- Tested whether files are already
**				  opened in case of a multi-process
**				  environment.
**		8/85	(dpr)	- Enables the Keystroke File Editor if
**				  the proper flags and symbols are set.
**		2/26/86 (garyb) - ifdef away references to the Keystroke
**				  File Editor for non-3270 devices.
**		10/20/86 (KY)  -- Changed CH.h to CM.h.**
**		11/18/86 (KY)  -- changed STindex,STrindex for Double
**				  Bytes characters.
**		10/14/87 (dkh) - Removed definition of FEtesting, now in ug.
**		11/30/87 (dkh) - Added definition of KFE.
**		12/18/87 (dkh) - Fixed jup bug 1648.
**		03/27/89 (emerson) reinstate references to the Keystroke
**				  File Editor for 3270 devices.
**		may-89	 (Eduardo)- added support for SEP tool
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**	01-oct-96 (mcgem01)
**		extern changed to GLOBALREF for global data project.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Update prototype for gcc 4.3.
*/


# ifndef	PCINGRES
FUNC_EXTERN	void	PCsleep();


# define	MAX_COMMAND_LINE	400

static	i4	test_open();		/* Startup QA tests */
static	i4	test_close();		/* Close QA tests */
static	char	*get_name();		/* Parse off object name */
static	i4	loc_err();		/* Error routine */

static	FILE	*Dfile = NULL;		/* -D file */
static	i4	Isaved = FALSE;		/* -I flag used */

GLOBALREF	bool	multp;

# endif /* PCINGRES */

void
IItestfrs(i4 begintest)
{
# ifndef	PCINGRES
	if (begintest)
	{
		test_open();
	}
	else
	{
		test_close();
	}
# endif		/* PCINGRES */
}

# ifndef	PCINGRES
/*
** TEST_OPEN - Begin any tests specified in the system attribute II_FRSFLAGS.
**
** History:
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
*/
static	i4
test_open()
{
	char		*frsflags;
	char		flagbuf[MAX_COMMAND_LINE +1];	/* II_FRSFLAGS value */
	register char	*flagcp;			/* Current flag */
	register char	*ch;
	LOCATION	floc;
	char		filebuf[MAX_LOC + 1];		/* File/object names */
	char		kfbufsym[MAX_COMMAND_LINE +1];	/*II_TT_KFE value */
	char		*kfesymbol = kfbufsym;

	if (multp)
		return;


	NMgtAt(ERx("II_FRSFLAGS"), &frsflags);
	if (frsflags == NULL)
		return;

	STcopy(frsflags, flagbuf);

	flagcp = flagbuf;

	for (;;)
	{
		if (*flagcp == '\0')
			return;

		if ((flagcp = STindex(flagcp, ERx("-"), 0)) == NULL)
			return;

		CMnext(flagcp);
		ch = flagcp;
		flagcp = get_name(flagcp, filebuf);
		if (*filebuf != '\0')
		{
			u_char	tmp_char[3];
			CMtolower(ch, tmp_char);
			switch (*tmp_char)
			{
			  case	'i':


				/* Save user input ( -I ) */
				Isaved = TRUE;
				FDrcvalloc(filebuf);
				break;

			  case	'd':
				/* Initialize output saving file and routine ( -D )*/
				LOfroms(FILENAME & PATH, filebuf, &floc);
				if (SIopen(&floc, ERx("w"), &Dfile) != OK)
				{
					loc_err(ERget(E_FR0001_IIFlag_D),
						filebuf);
					return;
				}
				FDdmpinit(Dfile);
				break;

			  case	'z':


				/* Initialize input/output testing routine ( -Z )*/
				if (FEtest(filebuf) == FAIL)
				{
					loc_err(ERget(E_FR0002_IIFlag_Z),
						filebuf);
					return;
				}
				break;

#ifdef SEP
			  case '*':
				if (FTtestsep(filebuf,&IIFDcommfile) == OK)
				{
				    IIUIfedata()->testing = TRUE;
				}
				break;
#endif /* SEP */
			  default:
				loc_err(ERget(E_FR0003_Illegal_II_FRSFLAG),
					*flagcp);
				return;
			}
		}
	}
}

/*
** TEST_CLOSED - Close any files opened by testing procedures.
*/
static	i4
test_close()
{

	if (Isaved)
	{
		FDrcvclose( (bool) FALSE );
		Isaved = FALSE;
	}

#ifdef SEP

	if (IIFDcommfile != NULL)
		TCclose(IIFDcommfile);

#endif /* SEP */

	if (Dfile != NULL)
	{
		SIclose(Dfile);
		Dfile = NULL;
	}

}

/*
** GET_NAME -	Each flag comes in as "-< flag >< object >{ <space> | NULL }".
**		This routine eats up the object name until the first following
** space or till the end of the string.
**
** Parameters:
**		cp   - points at the first (and only) character in the flag.
**		buf  - a buffer for the result object name.
*/
static	char *
get_name(cp, buf)
register char	*cp;
register char	*buf;
{
	CMnext(cp);
	while (!CMwhite(cp) && *cp != '\0')
		CMcpyinc(cp, buf);
	*buf = '\0';
	return (cp);
}

/* VARARGS1 */
static	i4
loc_err(err, arg)
char	*err;
i4	arg;
{
	SIprintf(err, arg);
	SIflush(stdout);
	PCsleep((u_i4) 2000);
}
# endif		/* PCINGRES */

/*
** Dummy calls from older versions.
*/
IItstbeg()
{
}

IItstend()
{
}

