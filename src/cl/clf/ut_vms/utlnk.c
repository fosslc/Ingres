/*
**    Copyright (c) 1983, 2008 Ingres Corporation
*/

# include	<compat.h> 
# include	<gl.h>
# include	<lo.h> 
# include	<si.h>
# include	<st.h>
# include	<pc.h> 
# include	<nm.h>
# include	<ut.h>


FUNC_EXTERN VOID UTopentrace();
FUNC_EXTERN VOID UTlogtrace();
FUNC_EXTERN STATUS PCcmdintr();


/**
** Name:	utlnk.c -	Compatability Library Utility Module
**				Link Objects and Libraries into Executable.
** History:
**	Revision 6.4  90/04/10  mikes
**	Add errfile, pristine, and clerror arguments
**	90/03/14 (sylviap) Changed interface call to PCcmdintr to be consistent 
**		w/PCcmdline.
**	4-feb-92 (mgw)
**		added trace file handling to print the command and link
**		options file to the trace file if requested.
**
**	Revision 6.3  90/06  wong
**	Removed obsolete ",-" continuation on lines written to link options
**	file.  (Options files with continuation on lines are now limited to
**	32K in size.)  Bug #30507.
**
**	Revision 6.2  89/04  wong
**	Enlarged 'command' buffer, which was overwriting 'lnkcombuf'.  JupBug
**		#5365.
**
**	Revision 2.0  83/10/18  daved
**	10/18/83 (dd) 	VMS CL
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**/

STATUS
UTlink ( objs, libs, exe, errfile, pristine, clerror )
LOCATION	*objs[];
char		*libs[];
LOCATION	*exe;
LOCATION 	*errfile;
i4 		*pristine;
CL_ERR_DESC	*clerror;
{
	FILE		*fptr;
	int		i;
	char		*stptr;
	char		*cp;
	char		*dcp;
	STATUS		status;
	char		command[256];
	LOCATION	loc;
	LOCATION	lnkcom;
	char		lnkcombuf[MAX_LOC+1];
	FILE		*Trace = NULL;

	CL_CLEAR_ERR(clerror);
        *pristine = TRUE;	/* We can redirect all output on VMS */

	if ( *objs == NULL || *libs == NULL )
		return FAIL;
	if ( NMt_open("w", "options", "opt", &loc, &fptr) != OK )
		return FAIL;
	LOcopy(&loc, lnkcombuf, &lnkcom);
	if ( (status = NMloc(LIB, FILENAME, "frontmain.obj", &loc)) != OK )
		return status;
	LOtos(&loc, &cp);
	SIfprintf(fptr, cp);
	i = 0;
	while ( objs[i] != NULL )
	{
		LOtos(objs[i++], &cp);
		SIfprintf(fptr, "\n%s", cp);
	}
	i = 0;
	while ( libs[i] != NULL )
	{
		SIfprintf(fptr, "\n%s", libs[i++]);
	}
	SIfprintf(fptr, "\n");
	SIclose(fptr);
	LOtos(exe, &cp);
	LOtos(&lnkcom, &stptr);
	NMgtAt("II_UT_LNK_DEBUG", &dcp);
	if (dcp != NULL && *dcp != EOS )
	{
		_VOID_ STprintf( command, "link%s/exe=%s %s/options",
						dcp, cp, stptr
		);
	}
	else
	{
		/*
		** BUG 3590 -- Don't leave .map files around
		*/
		_VOID_ STprintf( command, "link/nomap/exe=%s %s/options",
						cp, stptr
		);
	}
	if ( STlength(command) >= sizeof(command) )
		return UTEBIGCOM;

	UTopentrace(&Trace);	/* open trace file if II_UT_TRACE set */

	if (Trace)	/* log command first in case of major failure */
	{
		SIfprintf(Trace, "%s\n", command);
		_VOID_ SIcat(&lnkcom, Trace);
		_VOID_ SIclose(Trace);
	}
	status = PCcmdintr(NULL, command, FALSE, errfile, FALSE, clerror);
	LOdelete(&lnkcom);
	UTlogtrace(errfile, status, clerror);	 /* save error output to file */
	return status;
}
