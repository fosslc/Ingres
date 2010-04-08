/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   IIREP - EQUEL interface to the report formatter.  This acts the
**	same as the r_main routine for the standalone program.
**	This calls MAKEWORD to set up the argument string.
**
**	Parameters:
**		string	string containing the rest of the command
**			line from EQUEL.
**
**	Returns:
**		0	normal successful completion.
**		-1	an error has occurred.
**
**	Called by:
**		EQUEL.
**
**	Trace Flags:
**		1.0, 1.1.
**
**	History:
**		10/15/81 (ps)	written.
**		16-oct-1992 (rdrane)
**	          Use new constants in r_reset() invocations.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



IIrep(comline) 
char	*comline;		/* addresses of parameter strings */
{
	/* internal declarations */

	static	i4	argc;		/* number of parameters returned
					** from CMDLIN */
	static	char	**argv;		/* addresses of parameter strings */
	char		**argv1;	/* temp holder of argv */

	/* start of MAIN routine */

	if (St_called!=TRUE)
	{	/* not yet called */
		r_reset(RP_RESET_PGM,RP_RESET_LIST_END);
	}

	En_program = PRG_REPORT;	/* REPORT */
	St_called = TRUE;		/* program called report */

	argv = (char **) makeword(comline);	/* crack the command line into
						** parameters */
	for(argc=0, argv1=argv; *argv1!=0; argv1++)
	{
		argc++;
	}

	return(r_report(argc,argv));	/* runs the report and exits */
}
