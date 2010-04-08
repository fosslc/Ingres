# include	<compat.h>
# include	<gl.h>
# include	<pc.h> 
# include	<er.h> 
# include	<lo.h>
# include	<st.h>
# include	<array.h> 
# include	<ut.h>



FUNC_EXTERN  STATUS PCcmdnoout();


/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
/*
**
**	Name:
**		UTprint.c
**
**	Function:
**		UTprint
**
**	Arguments:
**              char            *printer_cmd;   * printer command to use *
**              LOCATION        *filename;      * name of file to print  *
**              bool            delete;         * TRUE if file to be deleted *
**              i4             copies;         * number of copies to print *
**              char            *title;         * job name/title of output *
**              char            *destination;   * destination printer *
**
**      Result:
**              Print 'n copies' of the file 'filename' with
**                      'printer_cmd'.  If 'printer_cmd' is NULL, use the
**                      default one.  Send the output to 'destination' if set.
**                      Use 'title' as the jobname if set.  If 'delete' is TRUE,**                      delete the file after spooling it.
**
**              Assumes the calling program has check if ING_PRINT is set by
**                      using NMgtAt.  If set, then pass to UTprint as the
**                      first parameter, printer_cmd.
**
**      Side Effects:
**              'filename' will be deleted if 'delete' is TRUE.
**
**
**	HISTORY
**		Modified 8/3/84 (jrc)   Added the delete parameter.
**              08-aug-89 (sylviap)
**             		Changed the parameters, and added new functionality to
**                        print number of copies and to a destination.
**                      Took out check for ING_PRINT.  Calling routines now 
**			  have to check if ING_PRINT is set.
**		13-feb-90 (sylviap)
**			Added parameter to PCcmdline.
**		14-mar-90 (sylviap)
**			Passing a CL_ERR_DESC to PCcmdline.
**		26-apr-90 (jim) on behalf of sylviap
**			Added /noidentify to the print command to the msg
**			will not cause frame to scroll up (#21137)
**		04-mar-91 (sylviap)
**			Changed UTprint so no errors are sent to the screen -
**			   messing up the forms system. #32101
**		15-mar-91 (steveh) fixed bug 35781 by appending a trailing
**			`.' when a print file has no suffix.  Also added
**			required include of st.h.
**		6/91 (Mike S) Use PCcmdnoout to accomplish 04-mar-91 change
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/


STATUS
UTprint(printer_cmd, filename, delete, copies, title, destination)
char		*printer_cmd;
LOCATION	*filename;
bool		delete;
i4             copies;
char            *title;
char            *destination;
{
	char	*fname;			/* pointer to filename */
	char	dev[MAX_LOC+1]; 	/* file device */
	char	path[MAX_LOC+1];	/* file name path */
	char	fprefix[MAX_LOC+1];	/* prefix portion of file name */
	char	fsuffix[MAX_LOC+1];	/* suffix portion of file name */
	char	version[MAX_LOC+1];	/* version of file */
	char	command[256];		/* complete print command */
	char	buf1[256] = "";		/* buffer for printer command */
	char	buf2[256] = "";		/* buffer for delete and copies flag */
	char	*_null = ERx("");

	/*	it makes no sense to print a file with a NULL name.	*/
	if ((filename == NULL))
		return(UT_PR_CALL);

        LOtos(filename, &fname) ;
        if  (fname == NULL)
		return(UT_PR_CALL);

	/* If the file name has no suffix then append a period 
	   (`.') to the end of the file name. This will fix bug
	   35781 in which a file with no suffix is created but
	   the VMS print command looks for a file ending in .LIS */
	if(fname[STlength(fname)-1] != '.')
	{
		/* break filename into its component parts              */
		LOdetail(filename, dev, path, fprefix, fsuffix, version);

		/* check for a missing suffix */
		if (fsuffix[0] == EOS)
		{
			STcat(fname, ".");
		}
	}

	/* get printer command if set */
	if((printer_cmd != NULL) && (*printer_cmd != NULL))
		STcopy (printer_cmd, buf1);
	else
		STcopy (ERx("print"), buf1);

	/* 
	** Should file be deleted after printing ? 
	** How many copies should be printed?
	*/
	if (delete)
		STprintf (buf2,"/noidentify/delete/copies=%d", copies);
	else
		STprintf (buf2,"/noidentify/copies=%d", copies);

	STpolycat(8, 
		  buf1, buf2,
		  ((destination != NULL) && (*destination != NULL)) ?
			"/queue=": _null,  
		  ((destination != NULL) && (*destination != NULL)) ?
			destination: _null,
		  ((title != NULL) && (*title != NULL)) ?
			"/name=": _null,
		  ((title != NULL) && (*title != NULL)) ?
 			title: _null,
		   " ", fname,
		   command);

	/* spawn a CLI subprocess that will print the file */

	/* 
	** Use PCsubproc, rather than PCcmdline. Set the mode so no errors
	** will be sent to the screen. #32101
	*/

	return PCcmdnoout( (char *)NULL, command);
}
