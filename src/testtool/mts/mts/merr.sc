/*
** merr.sc: Error handling procedure for MTS.
**
** 	If tracing, print error message of all errors encountered 
** 	This will probably cause diffs but we don't care if we're tracing
** 	Otherwise, conceal expected errors and only print unexpected ones
**	This will cause diffs, but they are usually terminating errors
**
**
** HISTORY:
**
** 04/20/92 ec	Added ERR_QUERYABORT  -4708
**
** 02/04/93 (erniec)
**	Add ERR_CATDEADLK, ERR_RQF_TIMEOUT to error list and if statement
**	Add ERR_LOGFULL to if statement
**	Remove ERR_SELECT which was suppressing real error messages
**	Modify comments
**
*/

/*
**	Move this OUTSIDE the function to get it into correct scope.
*/

exec sql include sqlca;


# include	<compat.h>
# include	<si.h>

exec sql begin declare section;
# define	ERR_DEADLOCK	-4700	
# define	ERR_LOGFULL 	-4706	
# define	ERR_QUERYABORT	-4708
# define 	ERR_CATDEADLK	-196674
# define 	ERR_RQF_TIMEOUT	-394566
exec sql end declare section;

GLOBALREF       i4 traceflag;      /*flag to set trace on/off */

merr()
{
	exec sql begin declare section;
		char	errmsg[256];
	exec sql end declare section;

/*      Use DBMS specific and NOT generic error messages */
        exec sql set_ingres (ERRORTYPE = 'dbmserror');

	if (traceflag)		
	{
		SIprintf("MERR Trace: Returned errno = %d\n",sqlca.sqlcode);

		exec sql inquire_ingres (:errmsg = ERRORTEXT);
		SIprintf("%s\n", errmsg);
		SIflush(stdout);
	}
	else if ((sqlca.sqlcode != ERR_DEADLOCK) && 
		 (sqlca.sqlcode != ERR_LOGFULL) && 
		 (sqlca.sqlcode != ERR_CATDEADLK) && 
		 (sqlca.sqlcode != ERR_RQF_TIMEOUT) && 
		 (sqlca.sqlcode != ERR_QUERYABORT))
	{
		exec sql inquire_ingres (:errmsg = ERRORTEXT);
		SIprintf("%s\n", errmsg);
		SIflush(stdout);
	}
	return;
}
