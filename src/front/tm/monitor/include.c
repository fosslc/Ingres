# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"monitor.h"
# include	<st.h>
# include	<er.h>
# include	"ermo.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  INCLUDE FILE
**
**	A file name, which must follow the \i, is read and inserted
**	into the text stream at this location.	It may include all of
**	the standard control functions.	 Includes may be nested.
**
**	If the parameter is 0, the file name is taken from the input;
**	otherwise it is taken directly from the parameter.  In this
**	mode, errors are not printed.
**
**	Prompts are turned off during the include.
**
**	Parameters:
**		fileloc		- LOCATION of the file to include (0 if the filename should come from stdin)
**		errmes		- error message fragment telling what is being attempted (to be included
**				in general-purpose error messages.  This fragment should be of the form:
**				"\nwhile trying to include INIT_INGRES".  Note the leading newline and lack of
**				a period.  This parameter should be an empty string if no fragment is needed
**				(NOT a null pointer).
**	History:
**		11/06/84 (lichtman)	-- added string parameter so that error message can say
**					what was being attempted when the error occurred.
**		85.11.04 (bml)		-- bug #6709.  \branch macros were access violating because
**					the fileloc location was not being copied into the Ifile_loc array
**					for future reference.
**		85.11.21 (bml)		-- bug #6382.  \quit in an include or startup file was not processed.
**					A very hack solution in response to a field emergency.	Catch a return
**					status of -1 from monitor and pass it up to tmmain() to quit.
**	06/27/88 (dkh) - Changed monitor() to IItmmonitor().
**	03/15/89 (kathryn) - Bug 4533 - Initialize errbuf to EOS, so if
**			calls to ERreport fail garbage will not be printed.
**	01/25/90 (teresal) - Changed LOfroms buffer size to MAX_LOC+1.
**	03/21/91 (kathryn) - Integrate Desktop Porting changes:
**		Increase Ifile_str_buf from 6 to 7.
**      25-sep-96 (rodjo04)
**              BUG 77034: Fix given so that terminal monitor can use the
**              environment variable II_TM_ON_ERROR to either terminate or
**              continue on error. Added a boolean parameter to IItmmonitor().
**              This new parameter is necessary so that we can tell the
**              difference between stdin, redirected input, and the use
**              of an include file.
**	29-Sep-2009 (frima01) 122490
**		Add return type for include() to avoid gcc 4.3 warnings.
*/

static char	Ifile_str_buf[7][MAX_LOC+1] ZERO_FILL;


i4
include(fileloc, errmes)
LOCATION	*fileloc;
char		*errmes;
{
	i2			savendf;
	FILE			*saveinp;
	char			*filename;
	FILE			*b;
	FUNC_EXTERN char	*getfilenm();
	register i4		status;
	char			errbuf[ER_MAX_LEN];
	register char		*specfile;
	i4			i;

	errbuf[0] = EOS;		/* Bug 4533 */

	if (fileloc != NULL)
	{
		/* get filename as a string */
		LOtos(fileloc, &filename);

		/*(bml) -- bug fix #6709 -- must copy file location for future reference */
		LOcopy(fileloc, Ifile_str_buf[Idepth + 1], &Ifile_loc[Idepth + 1]);
	}

	if (fileloc  == NULL)
	{
		specfile = getfilenm();
		STcopy(specfile, Ifile_str_buf[Idepth + 1]);

		if (status = LOfroms(PATH & FILENAME, Ifile_str_buf[Idepth + 1] , &Ifile_loc[Idepth + 1]))
		{
			ERreport(status, errbuf);
			putprintf(ERget(F_MO001C_Error_in_include_file), specfile, errmes, errbuf);
			return;
		}

		filename  = specfile;
		fileloc	  = &Ifile_loc[Idepth + 1];
	}

	/*
	** this may not work on all systems but I don't see
	** another way to do it
	*/
	if (!STcompare((char *)filename, ERx("-")))
	{
		/* read keyboard */
		b = stdin;
	}
	else if (*((char *)filename) == '\0') /* this may not work on all systems but I don't see another way to do it */
	{
		/* back up one level (EOF on next read) */
		return;
	}
	else
	{
		/* read file */
		if (status = SIopen(fileloc, ERx("r"), &b))
		{
			ERreport(status, errbuf);
			putprintf(ERget(F_MO001D_Error_opening_include), filename, errmes, errbuf);
			return;
		}
	}

	/* check for too deep */

	if (Idepth >= 5)
	{
		putprintf(ERget(F_MO001E_Include_nest_too_deep));

		if (b && b != stdin)
			SIclose(b);

		return;
	}

	Idepth++;
	Ifiled[Idepth] = b;

	/* get input from alternate file */

	savendf = Nodayfile;

	if (b != stdin)
		Nodayfile = -1;
	else
	{
		Nodayfile = Userdflag;
		prompt(ERget(F_MO001F_input));
	}

	saveinp = Input;
	Input	= b;

	/*(bml) bug fix #6382 -- catch \quit in an included file and pass special status back to tmmain() */
	if ((i = IItmmonitor(TRUE)) == -1)
		return(-1);

	/* done -- restore old file */

	Input		= saveinp;
	Nodayfile	= savendf;
	Ifiled[Idepth]	= NULL;
	Idepth--;
	return(OK);
}
