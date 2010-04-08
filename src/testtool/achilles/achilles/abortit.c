/*
 * abort_handler.c
 *
 * Contains abort_handler(), which is called when achilles is aborted via a
 * SIGINT, or SIGTERM signal. Terminates all current child processes,
 * disables further alarm signals, and sets all child data structures to not
 * restart when notification of a child's death comes in. NOTE - the program
 * does not exit here. It keep waiting on SIGCHLD signals until all children
 * are accounted for.
 *
 *      29-apr-92 (purusho)
 *              Added amd_us5 specific changes
 *	11-aug-93 (jeffr)
 *		removed blocked Child since we really want to do a wait()
 *		to count the total children time
 *	12-jan-96 (bocpa01)
 *		added NT support (NT_GENERIC)
 *	10-may-1999 (walro03)
 *		Remove obsolete version string amd_us5.
 *
*/

#include <achilles.h>

abort_handler()
{
	i4	    i,
		    j;
	HI_RES_TIME now;
	ACTIVETEST *curtest;
	PID	    pid;

	ACdisableAlarm();
        ACblock(STATRPT );

	ACgetTime(&now, (LO_RES_TIME *) NULL);
	PCpid(&pid);
	log_entry(&now, -1, -1, 0, pid, C_ABORT, 0, 0);
	for (i = 0 ; i < numtests ; i++) 
		for (j = 0 ; j < tests[i]->t_nkids ; j++)
		{
			curtest = &(tests[i]->t_children[j]);
			if ( (tests[i]->t_niters > 0) 
			  && (curtest->a_iters > tests[i]->t_niters) )
				log_entry(&now, i, j, 0, 0, C_NO_KILL, 0, 0);
			else
			{
				log_entry(&now, i, j, curtest->a_iters,
#ifdef NT_GENERIC
				  curtest->a_processInfo.dwProcessId, C_KILL, 0, 0);
#else
				  curtest->a_pid, C_KILL, 0, 0);
#endif /* END #ifndef NT_GENERIC */
				curtest->a_iters = tests[i]->t_niters;
				ACkillChild(curtest);
			}
		}
#ifdef NT_GENERIC
	return 0;
#else
	ACunblock();
#endif /* END #ifndef NT_GENERIC */
} /* abort_handler */
