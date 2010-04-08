/*
 * start_child.c
 *
 * Contains the start_child() routine, which is called by main() at startup
 * and by ACchildHandler() whenever a dead child needs rejuvenating. Forks
 * a child, rearranges the child's stdin, stdout, and stderr as specified
 * by the config file, changes ownership of the process as specified by
 * the config file, and execs the frontend.
 *
 **		17_jan-96 (bocpa01)
 **			added support for NT_GENERIC 
 */

#include <achilles.h>

start_child (testnum, childnum, iternum)
{
	ACTIVETEST *curtest;
	HI_RES_TIME nowh;
	LO_RES_TIME nowl;

	curtest = &(tests[testnum]->t_children[childnum]);
	ACforkChild(testnum, curtest, iternum);
	ACgetTime(&nowh, &nowl);
	curtest->a_start = nowl;
#ifdef NT_GENERIC
	if (curtest->a_processInfo.hProcess == 0)
	{
		curtest->a_iters = tests[testnum]->t_niters + 1;
		log_entry(&nowh, testnum, childnum, iternum, curtest->a_processInfo.dwProcessId,
		  C_NO_START, 0, 0);
	}
	else
		log_entry(&nowh, testnum, childnum, iternum, curtest->a_processInfo.dwProcessId,
		  C_START, 0, 0);
	return 0;
#else
	if (curtest->a_pid < 0)
	{
		curtest->a_iters = tests[testnum]->t_niters + 1;
		log_entry(&nowh, testnum, childnum, iternum, curtest->a_pid,
		  C_NO_START, 0, 0);
	}
	else
		log_entry(&nowh, testnum, childnum, iternum, curtest->a_pid,
		  C_START, 0, 0);
	return;
#endif /* END #ifdef NT_GENERIC */
} /* start_child */
