/******************** print_status() ***********************************
**
** print_status is a minimal reporting factility that will be activated
** to print out status of processes that are running - this can be accomplished
** by hitting the default status key
**
**
**	History:
**
**	29-apr-92 (purusho)
**		Added amd_us5 specific changes 
**
**      19-jul-93 (jeffr)
**	    Added useful reporting stuff
**      20-jul-93 (jeffr)
**          fixed signal handler problems by blocking 	STATRPT
**	17_jan-96 (bocpa01)
**	    added support for NT_GENERIC 
**	10-may-1999 (walro03)
**	    Remove obsolete version string amd_us5.
**
**
***************************************************************************/

#include <achilles.h>
#include <string.h>
extern HI_RES_TIME achilles_start; 
extern PID     achpid;
#ifdef UNIX
#include <signal.h>
#endif

print_status ()
{
	register int i, j, tests_up;
	char ct[TIME_STR_SIZE];
	HI_RES_TIME now, xtime;
	ACTIVETEST *curtest;
	int val=0;

        ACblock(ALARM | CHILD | ABORT | STATRPT);

	if (ACrightChar() )
	{
	    ACgetTime(&now, (LO_RES_TIME *) NULL);
	    ACtimeStr(&now, ct);
	    SIprintf("\n ***** ACHILLES REPORT - STATUS as of %s:****\n", ct);
	    SIprintf("---------------------------------------------------------\n\n");

	    for (i = 0 ; i < numtests ; i++)
	    {
		SIprintf("Test#\t\tTest_Name\tPID#\tChild#\tIters\tStatus\tTime Running\n");
	 SIprintf("-----\t\t--------\t----\t-----\t-----\t------\t------------ \n");
		val=i+1;
		tests_up = 0;
		for (j = 0 ; j < tests[i]->t_nkids ; j++)
		{
		    curtest = &(tests[i]->t_children[j]);
		    ACloToHi(&curtest->a_start, &xtime);
		    ACtimeStr(&xtime, ct);
		    if ( (tests[i]->t_niters > 0) 
		      && (curtest->a_iters > tests[i]->t_niters) )
		    	SIprintf("   Child #%d completed %d iterations on %s\n",
		    	  j+1, tests[i]->t_niters, ct);
		    else
		    {
		   SIprintf("%d\t\t%s\t\t%d\t%d\t%d\t%d\t%8.2f hour(s)\n",val, 
#ifdef NT_GENERIC
  (strrchr(curtest->a_argv[0],'/') + 1),curtest->a_processInfo.dwProcessId,j+1,curtest->a_iters,curtest->a_status, (float)ACcompareTime(&now,&achilles_start)/3600,'.'); 	
#else
  (strrchr(curtest->a_argv[0],'/') + 1),curtest->a_pid,j+1,curtest->a_iters,curtest->a_status, (float)ACcompareTime(&now,&achilles_start)/3600,'.');
#endif /* END #ifdef NT_GENERIC */
		    	tests_up++;
		    }
		}
		SIprintf("\n   --> %d%% of test #%d's children are still running.\n\n",
		  tests_up * 100 / tests[i]->t_nkids, i+1);
	    }
	    SIputc('\n', stdout);
	    SIflush(stdout);
	SIprintf("\n  Hit <CTRL %c> again for Run-Time Report Status\n",
'@' +stat_char);
	SIprintf("\n Do a \"kill -TERM %d\" for Final Summary Report\n",achpid);
	}
	ACresetPrint();
#ifdef NT_GENERIC
	return 0;
#else
	ACunblock();  
#endif /* END #ifdef NT_GENERIC */
}
