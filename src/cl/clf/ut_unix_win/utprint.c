
# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include       <clconfig.h>
# include	<er.h>
# include	<lo.h>
# include	<nm.h>
# include	<pc.h>
# include	<si.h>
# include	"UTi.h"
# include	"UTerr.h"

# ifdef NT_GENERIC
# define    UT_PRINT_BUF_SIZE   136
# define    MAX_COPIES          100
# endif /* NT_GENERIC */

/*
**Copyright (c) 2004 Ingres Corporation
**
** UTprint.c -- Print file.
**
**	Function:
**		UTprint
**
**	Arguments:
**		char		*printer_cmd;	* printer command to use *
**		LOCATION	*filename;	* name of file to print  *
**		bool		delete;		* TRUE if file to be deleted *
**		i4		copies;		* number of copies to print *
**		char		*title;		* job name/title of output *
**		char		*destination;	* destination printer *
**
**	Result:
**		Print 'n copies' of the file 'filename' with 
**			'printer_cmd'.  If 'printer_cmd' is NULL, use the 
**			default one.  Send the output to 'destination' if set.
**			Use 'title' as the jobname if set.  If 'delete' is TRUE,
**			delete the file after spooling it.
**
**	        Assumes the calling program has check if ING_PRINT is set by
**			using NMgtAt.  If set, then pass to UTprint as the
**			first parameter, printer_cmd.
**
**	Side Effects:
**		'filename' will be deleted if 'delete' is TRUE.
**
**	History:
**Revision 30.2  85/08/28  17:58:28  daveb
**UTS chokes on static forward func refs.
**
**Revision 30.1  85/08/14  19:27:44  source
**llama code freeze 08/14/85
**
**		Revision 3.0  85/05/07  16:55:35  wong
**		Final 3.0 Integration.  Integrated CFE version with UNIX version;
**		deleted extraneous function declarations.  Added WECO default
**		printer path.
**		
**		03/83 -- (gb) written
**		09/24/84 -- (agh) added delete parameter
**		12/84 (jhw) -- 3.0 Integration; added `delete' flag.
**
**		06/01/87 (daveb& lin) PCwait now take a pid by value.
**			  It is fixed so the old bug requiring a PCwait
**			  sometimes after a failed PCspawn is no longer
**			  required.
**		08-aug-89 (sylviap)
**			Changed the parameters, and added new functionality to
**			  print number of copies and to a destination.
**			Took out check for ING_PRINT.  Calling routines now have
**			  to check if ING_PRINT is set.
**		08-jan-90 (sylviap)
**			Added PCcmdline parameters.
**      	08-mar-90 (sylviap)
**              	Added a parameter to PCcmdline call to pass a 
**			CL_ERR_DESC.
**      	04-mar-91 (sylviap)
**			Redirects errors so won't mess up forms display. #32101
**              22-mar-91 (seng, kimman)  Adding clconfig.h because it
**                        includes clsecret.h which defines PRINTER_CMD.
**                        Using the defined PRINTER_CMD instead of
**                        a hardcoded printer binary.  Integrated from 6.3/02p.
**		25-Apr-1991 (mguthrie)
**			Bull lp command requires global read, so do a chmod
**                      first.
**		16-Feb-1993 (smc)
**			Added forward declarations.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	20-apr-95 (emmag)
**	    NT porting changes.
**	20-jul-95 (emmag)
**	    Modified printing for NT so that it can actually print.
**	    Add capability to print multiple copies of the report.
**	20-oct-1995 (canor01)
**	    Redirected output from the print command to null device.
**	    Otherwise, print.exe gets access violation trying to
**	    display its status message.
**	13-Jun-1996 (mckba01)
**	    Added if defined statements so that correct lp parameters
**	    are used when printing on HP machines, hp8_us5, hp3_us5.
**	12-May-1997 (kitch01)
**	    Bug 77697 - Allow -oprinter to be used correctly by setting 
**	    /D:printer for print.exe on WindowsNT.
**	    Bug 82041 - print.exe is not supplied with Windows95 so use 
**	    copy instead.
**	03-sep-97 (mcgem01)
**	    Use copy, to copy to the relevant port on both NT and '95.
**	    This makes the code more straightforward and avoids a problem
**	    should Microsoft cease shipping print with NT.
**	10-may-1999 (walro03)
**	    Remove obsolete version string bu2_us5, bu3_us5.
**      02-Aug-1999 (linke01)
**          same as hp8_us5, in STprint, SGI (sgi_us5) uses -n option for 
**          copies and -t option for title.   
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Jan-2001 (shust01)
**	    XIP of 6.4 change 275049.   05-jul-95 (abowler)
**	    Fixed 60056 - added space before '>' when creating a
**	    print command to avoid misinterpretation by the shell.
**	15-Jun-2004 (schka24)
**	    Watch out for buffer overrun (Windows).
**	17-Jun-2004 (schka24)
**	    More of the same, printer-cmd param is untrusted.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

/* forward declarations */
static  STATUS  invoke_shell();   	/* called to invoke a shell that
					** invokes the printer
					*/

static  char    def_printer[]   =       PRINTER_CMD;

STATUS
UTprint(printer_cmd, filename, delete, copies, title, destination)
char		*printer_cmd;
LOCATION	*filename;
bool		delete;
i4		copies;
char		*title;
char		*destination;
{
# ifndef NT_GENERIC
	char	user_print[MAX_LOC];	  /* stores name of printer to spawn */
	char	*fname;			  /* string to contain filename */
	PID	pid;			  /* pid of printer that was spawned */

	CL_ERR_DESC err_code;


	UTstatus = OK;
	
	/*
	** it makes no sense to print a file with a NULL name.
	*/
	if ((filename == (LOCATION *)NULL) ||
			(LOtos(filename, &fname), fname == (char *)NULL))
		UTstatus = UT_PR_CALL;
	else
	{
		if ((printer_cmd != (char *)NULL) && (*printer_cmd != '\0'))
			STlcopy(printer_cmd, user_print, sizeof(user_print)-1);
		else
			STlcopy(def_printer, user_print, sizeof(user_print)-1);

		if (UTstatus == OK)
		{
#			define	MAXLEN	256	/* max. len of command */
			char	cmd [MAXLEN + 1];
			char	buf[ER_MAX_LEN+1];
			char	buf2[ER_MAX_LEN+1];
			char	buf3[ER_MAX_LEN+1];
			/*
			** set up arguments to PCspawn
			*/

			/* 
			** Set up the number of copies.  Create string
			** "-#n", where n = number of copies
			*/
#if defined(hp3_us5) || defined(any_hpux) || defined(sgi_us5)
                        STprintf (buf, " -n%d",copies);
#else
			STprintf (buf, " -#%d",copies);
#endif

			/* Set up job name.  Default is filename. */
			if ((title != (char *)NULL) && (*title != '\0'))
#if defined(hp3_us5) || defined(any_hpux) || defined(sgi_us5)
				STprintf (buf2, " -t%s",title);
#else
				STprintf (buf2, " -J%s",title);
#endif
			else
				STprintf (buf2, "");

			/* Set up destination printer.  */
			if ((destination != (char *)NULL) && 
				(*destination != '\0'))
#if defined(hp3_us5) || defined(any_hpux)
				STprintf (buf3, " -d%s",destination);
#else
				STprintf (buf3, " -P%s",destination);
#endif
			else
				STprintf (buf3, "");

			STlpolycat(7, sizeof(cmd)-1, user_print, buf, buf2, buf3, " ", 
				fname, " > /dev/null 2>&1", cmd);

			if ((UTstatus = PCcmdline((LOCATION *)NULL, cmd, 
				PC_WAIT, NULL, &err_code)) == OK)
			{

				/* Delete file if printed successfully */

				if (delete && UTstatus == OK)
					LOdelete(filename);
			}
		}

	}

	return UTstatus;

#else  /* NT_GENERIC */
    int     ncopies;
    char *  outfile;
    char *  infile;
    char    printbuf [255];

    if ((filename == (LOCATION *)NULL) ||
			(LOtos(filename, &infile), infile == (char *)NULL))
        return (UT_PR_CALL);

    NMgtAt(ING_PRINT, &outfile);
    if (outfile == (char *)NULL || *outfile == '\0')
	{
		/* print.exe is not provided by Win95 so use COPY instead.
		** Also if a printer has been specified use it instead of the
		** default.
		*/
		outfile = "COPY";
		if (destination == (char *)NULL)
			sprintf (printbuf, "%s %s lpt1 >NUL", outfile, infile);
		else
			sprintf (printbuf, "%s %s %s >NUL", outfile, infile, 
					destination);
	}
	else
	{
		/* ING_PRINT is set so use it to direct output */
		STlpolycat(4, sizeof(printbuf)-1, outfile, " ",
			infile, " >NUL", &printbuf[0]);
	}

    ncopies = (copies <= MAX_COPIES) ? copies : MAX_COPIES;
    while (--ncopies >= 0)
    {
        system (printbuf);
    }

    if (delete)
	LOdelete(filename);

    return(OK);

#endif /* NT_GENERIC */
}




/*
**	calls PCcmdline to invoke the default command line interpreter (shell)
**		to print the specified file with the default printer.
*/

static STATUS
invoke_shell(filename, err_code)
char	    *filename;
CL_ERR_DESC *err_code;
{
	char	command[(5*MAX_LOC)/4];		/* stores the name of the 
						** printer and file to be 
						** printed */


	STpolycat(3, def_printer, " ", filename, command);

	return (PCcmdline((LOCATION *) NULL, command, PC_WAIT, NULL, err_code));
}
