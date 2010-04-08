/*
 * log_entry.c
 *
 * Contains log_entry(), which (as one might expect) logs interesting
 * events, either in readable text to stdout, or (if -l is specified) in
 * a more compact form to a log file.
 *
 * History:
 *      05-July-91 (DonJ)
 *              Change quotes to brackets on include of achilles.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
 */

#include <achilles.h>

log_entry (tv, testnum, childnum, iternum, pid, action, code, starttime)
HI_RES_TIME *tv;
i4	     testnum,
	     childnum,
	     iternum,
	     pid,
	     action,
	     code;
LO_RES_TIME *starttime;
{
	extern   FILE	 *logfile;             /* logfile pointer          */
	extern   int 	 silentflg;            /* run in silent mode ?     */
	char	 ct[TIME_STR_SIZE];

        /* data compression removed -- 10-29-89, bls                       */

        /* write the log entry to the appropriate output file, either the
           specified logfile, or stdout if we're not using a logfile ...   
           10-29-89, bls
        */

	ACtimeStr(tv, ct);
	if ( (action == C_TERM) || (action == C_EXIT) )
    {
        /* if the test is ending, record the run time ...
           10-29-89, bls
        */

	    SIfprintf(logfile, AClogTermFormat, ct, testnum+1, childnum+1, 
	      iternum, ACpidFormat(pid), log_actions[action], code,
	      ACgetRuntime(tv, starttime) );
    }

	else
	{
	    SIfprintf(logfile, AClogEntryFormat, ct, testnum+1, childnum+1, 
	              iternum, ACpidFormat(pid), log_actions[action]);
	}

	SIflush(logfile);

} /* log_entry */
