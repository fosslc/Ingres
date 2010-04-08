/*
 * alarm_handler.c
 *
 * alarm_handler() is called when the interval timer goes off. It checks
 * each of the test types to see if any are due to receive interrupt or
 * kill signals. If needed, it selects test instances to signal.
 *
 *
 *  History:
 *         7-19-93 (Jeffr)
 *            Added new code to support (-a option) - running for set period
 *            of time
 *	   8-7-93 (Jeffr)
 *             Fixed file close logic bug - compares to stdout now
 *	   10-20-93 (Jeffr)
 *		Changed active_children to GLOBALREF from int
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
*/

#include "achilles.h"
GLOBALREF HI_RES_TIME achilles_start;
GLOBALREF i4	    time_to_run;
#include <stat.h>
GLOBALREF i4  active_children;
extern i4       time_set;
alarm_handler ()
{
	register i4  i,
		     j,
		     target,
		     num_to_kill,
		     int_target,
		     kill_target;
	HI_RES_TIME  now;
	struct stat    st;
	char fbuf[MBOX_BUFSIZE];
	char          *fname;
	ACTIVETEST    *curtest;
	ACblock(CHILD | STATRPT | ABORT);

	ACgetTime(&now, (LO_RES_TIME *) NULL);

#define USE 1
#ifdef USE
	for (i = 0 ; i < numtests ; i++)
	{
	    int_target = -1;
/** see if we want to exit achilles first - (we exceeded time limit) **/

            if ((time_set) && 
                (ACcompareTime(&now,&achilles_start)/3600 >= time_to_run) )
            {

                for (j = 0 ; j < tests[i]->t_nkids ; j++)
                {
		    if (!*tests[i]->t_outfile)
                    {
                                LOtos(&tests[i]->t_children[j].a_outfloc,
                                  &fname);
                                STprintf(fbuf, "%s.LOG", fname);
                                stat(fbuf, &st);
                                if (st.st_size == 0)
                                        delete(fbuf);
         	    }
	    /** kill all children in the group **/
		    ACkillChild(&tests[i]->t_children[j]);
		    active_children--;
                 curtest = &(tests[i]->t_children[j]);
    /** keep running total of child times for each instance **/
         tests[i]->t_ctime+= (float)ACcompareTime(&now,&curtest->a_start)/3600;

	        /* Log the dead child */
		    log_entry(&now, i, j, curtest->a_iters, tests[i]->t_children[j].a_pid, C_EXIT,
			    curtest->a_status[0], &curtest->a_start);

              }
		
              curtest->a_start = now.time;

	if (active_children == 0)
        {
		/** generate report **/
		ACreport();
                puts("time to run expired - exiting.");
                if (logfile != stdout)
                        SIclose(logfile);

                PCexit(OK);
        }

    }
/** not time to die yet **/
    else {
	    if ( (ACintEnabled(i) )
	      && (ACcompareTime(&tests[i]->t_inttime, &now) <= 0) )
	    {
		/* Randomly pick the starting point of a cluster-sized
		   group of child processes. Send each a signal, unless
		   it's already completed all its iterations.
		*/
		int_target = ACrandom() % tests[i]->t_nkids;
		for (j = 0 ; j < tests[i]->t_intgrp ; j++)
		{
		    target = (int_target + j) % tests[i]->t_nkids;
		    if ( (tests[i]->t_niters > 0)
		      && 
(tests[i]->t_children[target].a_iters > tests[i]->t_niters) )			
log_entry(&now, i, target, 0, 0, C_NO_INT, 0, 0);
		    else
		    {
			/* Issuing an Interrupt on VMS does not work like it
			does on UNIX.  A program never returns from an Inter-
			rupt like it does on Unix.  Instead, it just aborts
			as if it had received a kill -9.  Since this is messy
			on VMS we do a kill -9 for interrupts so that the 
			program terminates with the appropriate error message */

/*	printf("killing child %d \n", tests[i]->t_children[target].a_pid); */
			ACkillChild(&tests[i]->t_children[target]);
/*			ACintChild(&tests[i]->t_children[target]);	*/
			/*log_entry(&now, i, target, 
			  tests[i]->t_children[target].a_iters,
			  tests[i]->t_children[target].a_pid, C_INT, 0, 0);
               */
		    }
		}
		/* Update time to next interrupt. */
		ACaddTime(&tests[i]->t_inttime, &now, &tests[i]->t_intint);
	    }
	    if ( (ACkillEnabled(i) )
	      && (ACcompareTime(&tests[i]->t_killtime, &now) <= 0)
	      && ( (int_target < 0) 
	        || (tests[i]->t_intgrp < tests[i]->t_nkids) ) )
	    {
		/* Do the same thing for kill signals, if needed, but
		   don't kill any process which just received an
		   interrupt.
		*/
		if (int_target >= 0)
		{
		    num_to_kill = min(tests[i]->t_killgrp, 
		      tests[i]->t_nkids - tests[i]->t_intgrp);
		    if (num_to_kill < 0)
			num_to_kill = 0;
		    kill_target = ACrandom() 
		      % (tests[i]->t_nkids - tests[i]->t_intgrp);
		    if (kill_target >= int_target)
			kill_target += tests[i]->t_intgrp;
		}
		else
		{
		    kill_target = ACrandom() % tests[i]->t_nkids;
		    num_to_kill = tests[i]->t_killgrp;
		}
		for (j = 0 ; j < num_to_kill ; j++)
		{
		    target = (kill_target + j) % tests[i]->t_nkids;
		    if ( (target >= int_target) 
		      && (target < (int_target + tests[i]->t_intgrp) ) )
			target = (target + tests[i]->t_intgrp)
			  % tests[i]->t_nkids;
		    if ((tests[i]->t_niters > 0)
		      &&
(tests[i]->t_children[target].a_iters > tests[i]->t_niters))
	log_entry(&now, i, target, 0, 0, C_NO_KILL, 0, 0);
		    else
		    {
			/*printf("killing child %d \n",
			     tests[i]->t_children[target].a_pid);
			*/
			ACkillChild(&tests[i]->t_children[target]);
	log_entry(&now, i, target,	 tests[i]->t_children[target].a_iters,
		  tests[i]->t_children[target].a_pid, C_KILL, 0, 0);
		    }
		}
		ACaddTime(&tests[i]->t_killtime, &now, &tests[i]->t_killint);
	    }
	}
#endif
}
	ACresetAlarm();
	ACunblock();

/*printf("exited alarmhandler \n");*/
	return;
} /* alarm_handler */
