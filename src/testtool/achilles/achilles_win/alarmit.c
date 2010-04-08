/*
 * alarm_handler.c
 *
 * alarm_handler() is called when the interval timer goes off. It checks
 * each of the test types to see if any are due to receive interrupt or
 * kill signals. If needed, it selects test instances to signal.
*/

/*
** History:
**	12-jan-96 (bocpa01)
**		Created to support NT.
**	17-dec-1997 (canor01)
**		Pass a pointer to a structure, not the structure.
**      22-sep-2000 (mcgem01)
**          More nat and longnat to i4 changes.
**	07-Aug-2009 (drivi01)
**	    Add pragma to disable deprecated POSIX functions warning
**          4996.
**/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

#include <achilles.h>
#include <sys/stat.h>

extern i4 active_children;
extern VOID ACreport();

HI_RES_TIME achilles_start;
extern i4   time_to_run;
extern VOID ACchildHandler ();
extern i4  	time_set;

tag_child();		

int	noMoreChildren = 0;

alarm_handler ()
{
	register i4  i,
		     j,
		     target,
		     sigcount,
		     count,
		     avail = 0,
		     Using_sigs,
			 rc = 0;
	HI_RES_TIME  now;
	STATUS	    status;
	ACTIVETEST  *testptr;
	ACTIVETEST  **availtest = NULL, 	  /* will be an array of pointers to tests */
				**availtestNew = NULL;    
	extern PID     achpid;
	char          *fname;

    ACTIVETEST    *curtest;
    struct stat    st;
    char           timebuf [80];

	if (noMoreChildren)
		return 0;

    ACblock(CHILD | STATRPT | ABORT | ALARM );

	/** we always want to be Using_sigs for Alarm Handler **/
	Using_sigs = 1;
	ACgetTime(&now, (LO_RES_TIME *) NULL);
	for (i = 0 ; i < numtests ; i++) {	//1

		/** see if we want to exit achilles first - (we exceeded time limit) **/

        if ((time_set) &&
                (ACcompareTime(&now,&achilles_start)/3600 >= time_to_run) ) {	//2
			for (j = 0 ; j < tests[i]->t_nkids ; j++) {	//3
				curtest=&tests[i]->t_children[j];
				LOtos(&tests[i]->t_children[j].a_outfloc, &fname);
				if ( (!*tests[i]->t_outfile)
                          && (!stat(fname, &st) )
                          && (st.st_size == 0) )
					unlink(fname);
				
/*				SIfprintf(stderr,"crashes in alarmit\n"); */
				/*	Record the end time of the last process in a_start,
					for use by print_status.
				*/
				/** kill all children in the group **/
                ACkillChild(&tests[i]->t_children[j]);
		    
                active_children--;
				/** keep running total of child times for each instance **/
       			tests[i]->t_ctime+= ACcompareTime(&now,&curtest->a_start)/3600.0F;
                        curtest->a_start = now.time;
				curtest = &(tests[i]->t_children[j]);
                /* Log the dead process. */
				log_entry(&now, i, j, curtest->a_iters,

				tests[i]->t_children[j].a_processInfo.dwProcessId, 
                       C_EXIT, &testptr->a_status, &curtest->a_start); 
/**				fprintf(stderr,"active children = %d\n",active_children); **/
			}	//3

			if (active_children == 0) {	//3
/*				fprintf(stderr,"\n active children = %d Achilles now Exiting !! \n",active_children); */
				/* time and date stamp the conclusion of the achilles test run
				11/16/89, bls */
				/** generate report **/
				ACreport();

				ACgetTime (&now, (LO_RES_TIME *) NULL);
				ACtimeStr (&now, timebuf);
				SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n",
                       timebuf, "ACHILLES END", achpid, "END");
				SIflush(logfile);
				if (logfile != stdout) 
					fclose(logfile);
				noMoreChildren = 1;
			}	//3
  
        } else {	//2
/*			if( ACintEnabled(i) || ACkillEnabled(i) )
			Using_sigs++;
*/

			if ( (ACintEnabled(i) )
				&& (ACcompareTime(&tests[i]->t_inttime, &now) <= 0) ) {	//3

				/*	Randomly pick the starting point of a cluster-sized
					group of child processes. Send each a signal, unless
					it's already completed all its iterations.
				*/

				/*	allocate memory for the array of test instance pointers.
					Each element in the array points to a test instance available
					to receive interrupt or kill signals.
					11-3-89, bls
				*/

				availtest = (ACTIVETEST **) MEreqmem(0, tests[i]->t_nkids * 
                         sizeof (ACTIVETEST *), TRUE, &status);

#ifdef DEBUG

				fprintf (stderr, "availtest = %X (hex), *availtest = %X (hex)\n",
                     availtest, *availtest);

#endif
				if (status != OK) {	//4
		    		SIprintf("Can't MEreqmem space for available test array\n");
		    		PCexit(FAIL);
				}	//4

				/*	determine the number of test instances available to 
					receive interrupts.  A test instance is available if
					it has not completed all of its iterations.
					11-3-89, bls
				*/

				for (j = 0, avail = 0; j < tests[i]->t_nkids; j++) { //4
					/*	for each available test instance, store a pointer to 
						it in the availtest array and increment the count
						11-3-89, bls
					*/

#ifdef DEBUG

					fprintf (stderr, "i = %d j = %d iters = %d tot iters = %d\n",
                         i, j, tests[i]->t_children[j].a_iters,
                         tests[i]->t_niters);
#endif

					if (tests[i]->t_children[j].a_iters <= tests[i]->t_niters) {	//5
						availtest[avail] = &(tests[i]->t_children[j]);
						avail++;
					}	//5
				}	//4

				/* determine how many signals to send.  The number of
				   signals to send is the lesser of the available test
				   instances or the interrupt cluster size.
				   11-3-89, bls
				*/

				sigcount = avail < tests[i]->t_intgrp ? avail : tests[i]->t_intgrp;

#ifdef DEBUG

				fprintf (stderr, "avail = %d intgrp = %d sigcount = %d\n", 
                     avail, tests[i]->t_intgrp, sigcount);

#endif
				/* if the interrupt cluster size is larger than the number
				   of available test instances, all of the available 
				   instances get sent a signal.
				   11-3-89, bls
				*/

				if (avail <= tests[i]->t_intgrp) {	//4
					for (j = 0 ; j < sigcount; j++) {	//5
						testptr = availtest[j];     

#ifdef DEBUG

						fprintf (stderr, "j = %d testptr = %X (hex)\n", j, testptr);

#endif
						tag_child (testptr, i, C_INT);
					}	//5
				} else {	//4
				
				/* the number of test instances available exceeds the interrupt
				   cluster size.  Choose randomly among all available instances.
				   Mark each test as unavailable after it is selected.
				   11-3-89, bls
				*/

					count = 0;
					while (count < sigcount) {	//5
						target = ACrandom() % avail;
						if (availtest[target] != NULL) {	//6
							testptr = availtest[target];
#ifdef DEBUG

							fprintf (stderr, "target = %d testptr = %X (hex)\n", 
                                  target, testptr);
#endif
							tag_child (testptr, i, C_INT);
							count++;
							availtest[target] = NULL;
						}	//6
					}	//5
				}	//4
				/* Update time to next interrupt.   */

				ACaddTime(&tests[i]->t_inttime, &now, &tests[i]->t_intint);
			}	//3

			/* 
				now do the same for kill signals, but don't kill any process 
				which just received an interrupt. Processes on NT are killed,
				not interrupted.
				1-23-96 bocpa01
            */

			if ( (ACkillEnabled(i) ) 
				&& (ACcompareTime(&tests[i]->t_killtime, &now) <= 0)) {	//3


				/*	At this point, if (ACintEnabled(i) was TRUE, avail is equal 
					to the initial number of available tests. Interrupted (killed) 
					processes have availtest[i] = NULL. Compact availtest[] by 
					eliminating those emty slots and recalculate avail.
					If, however, (ACintEnabled(i) was FALSE, calculate avail and
					allocate availtest[] for killable children.
					1-24-96 bocpa01
				*/

				if (avail) {	//4

					i4  k;

					availtestNew = (ACTIVETEST **) MEreqmem(0, avail * 
                         sizeof (ACTIVETEST *), TRUE, &status);

#ifdef DEBUG

					fprintf (stderr, "availtestNew = %X (hex), *availtestNew = %X (hex)\n",
                     availtestNew, *availtestNew);

#endif
					if (status != OK) {	//5
		    			SIprintf("Can't MEreqmem space for available test array\n");
		    			PCexit(FAIL);
					}	//5


					for (k = 0, j = 0; k < avail; k++) {	//5
						if (availtest[k]) 
							availtestNew[j++] = availtest[k];
					}	//5
					MEfree (availtest);
					avail = j;
					availtest = availtestNew;
				} else {	//4

					availtest = (ACTIVETEST **) MEreqmem(0, tests[i]->t_nkids * 
                         sizeof (ACTIVETEST *), TRUE, &status);
#ifdef DEBUG

					fprintf (stderr, "availtest = %X (hex), *availtest = %X (hex)\n",
						availtest, *availtest);

#endif
					if (status != OK) {	//5
		    			SIprintf("Can't MEreqmem space for available test array\n");
		    			PCexit(FAIL);
					}	//5

					for (j = 0, avail = 0; j < tests[i]->t_nkids; j++) { //5

						/*	for each available test instance, store a pointer to 
							it in the availtest array and increment the count
							11-3-89, bls
						*/

#ifdef DEBUG

						fprintf (stderr, "i = %d j = %d iters = %d tot iters = %d\n",
							 i, j, tests[i]->t_children[j].a_iters,
							tests[i]->t_niters);
#endif

						if (tests[i]->t_children[j].a_iters <= tests[i]->t_niters) {	//6
							availtest[avail] = &(tests[i]->t_children[j]);
							avail++;
						}	//6
					}	//5
				}	//4

				/*	determine how many signals to send.  The number of
					signals to send is the lesser of the available test
					instances or the kill cluster size.
					11-3-89, bls
				*/

				sigcount = avail < tests[i]->t_killgrp ? 
                       avail : tests[i]->t_killgrp;

#ifdef DEBUG

				fprintf (stderr, "avail = %d killgrp = %d sigcount = %d\n", 
                     avail, tests[i]->t_killgrp, sigcount);

#endif
				/*	if the kill cluster size is larger than the number
					of available test instances, all of the available 
					instances get sent a signal.
					11-3-89, bls
				*/

				if (avail <= tests[i]->t_killgrp) {	//4
					for (j = 0 ; j < sigcount; j++) {	//5
						testptr = availtest[j];  
#ifdef DEBUG

						fprintf (stderr, 
                            "kill j = %d testptr = %X (hex)\n", j, testptr);
#endif
						tag_child(testptr, i, C_KILL);
					}	//5
				} else {	//4

					/*	the number of test instances available exceeds the kill
						cluster size.  Choose randomly among all available instances.
						Mark each test as unavailable after it is selected.
						11-3-89, bls
					*/

					count = 0;
					while (count < sigcount){	//5
						target = ACrandom() % avail;
						if (availtest[target] != NULL) { //6
							testptr = availtest[target];
#ifdef DEBUG

							fprintf (stderr, 
                                 "kill target = %d testptr = %X (hex)\n", 
                                  target, testptr);
#endif
							tag_child (testptr, i, C_KILL);
							count++;
							availtest[target] = NULL;
						}	//6
					}	//5
				}	//4
				/* Update time to next kill signal. */

				ACaddTime(&tests[i]->t_killtime, &now, &tests[i]->t_killint);

				/*	Free the memory used for the 
					available test array        
					11-3-89, bls               
				*/ 
				MEfree (availtest);
			}	//3
		}	//2

	}	//1
	return rc;
} /* alarm_handler */

tag_child(testptr, i, tag)
ACTIVETEST  *testptr;
int	i;
int	tag;
{
	int	status;
	HI_RES_TIME  now;

	ACgetTime(&now, (LO_RES_TIME *) NULL);
	if( tag == C_INT )
	{
		if (ACkillChild (testptr) < 0)
			tag = C_NO_INT;
	}
	else if ( tag = C_KILL )
	{
		if (ACkillChild (testptr) < 0)
			tag = C_NO_KILL;
	}
	else
		status = C_NOT_FOUND;
		
	log_entry (&now, i, atoi(testptr->a_childnum),
	   testptr->a_iters, testptr->a_processInfo.dwProcessId, tag, 0, 0);
	return 0;
}

ACkillChild(ACTIVETEST *testptr) {

	HI_RES_TIME now;
	int			i, j;
	i4 			action;

	for (i = 0 ; i < numtests ; i++) {
		for (j = 0 ; j < tests[i]->t_nkids &&
			 &(tests[i]->t_children[j]) != testptr; j++);
	}
	if (i >= numtests) 
		i = j = -1;
	ACgetTime(&now, (LO_RES_TIME *) NULL);
	if (!TerminateProcess(testptr->a_processInfo.hProcess, CHILDKILLEDRC)) {
		action = C_NO_KILL;		
		GetExitCodeProcess(testptr->a_processInfo.hProcess, &testptr->a_status);
		CloseHandle(testptr->a_processInfo.hProcess);
		testptr->a_processInfo.hProcess = 0;
	} else
		action = C_KILL;		
	log_entry(&now, i, j, testptr->a_iters, 
				testptr->a_processInfo.dwProcessId, action,
				&testptr->a_status, &testptr->a_start);
	if (action == C_NO_KILL)
		return -1;
	return 0;
}
