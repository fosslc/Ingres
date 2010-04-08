			/*
** accompat.c
**
** History:
**      17-Jan-96 (bocpa01)
**              created
**      22-sep-2000 (mcgem01)
**          More nat and longnat to i4 changes.
**	1-Sep-2009 (bonro01)
**	    Compile problem caused by change 499857 for SIR 121804
**	    Remove local prototype for ACreport() which was added
**	    to achilles.h
**/

#include <achilles.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>

#define ACH_TIMEOUT 0xFFFFFFFE

GLOBALDEF char stat_char = '.';

ACgetPathList();

extern i4 achilles_start;
extern alarm_handler();

i4 active_children = 0;

VOID ACblock ( flags )
i4 flags;
{
} /* ACblock */


PHANDLE					lpHandles = NULL;	// child processes handle array
GLOBALDEF HI_RES_TIME	gInterruptTime;		// last timer interrupt time
GLOBALDEF				i4 gWaitTime;		// next time-out interval
GLOBALDEF int			gWaitInterval;		// time-out interval 

BOOL FoundKilledProcess(WAIT_STATUS *pStatus, int *piTest, int *piChild) { 

	int i, j;

	for (i = 0 ; i < numtests ; i++) {
		for (j = 0 ; j < tests[i]->t_nkids ; j++) {
			if (tests[i]->t_children[j].a_status == CHILDKILLEDRC) {
				*pStatus = CHILDKILLEDRC;
				*piTest = i;
				*piChild = j;
				return TRUE;
			}
		}
	}
	return FALSE;
}

DWORD WaitForProcesses(DWORD nCount, DWORD dwTimeOut) {

	// MAXIMUM_WAIT_OBJECTS limits the number of objects to be waited for in NT.
	// This routine circumwents this by dividing array of handles into slots of
	// MAXIMUM_WAIT_OBJECTS and waiting for one slot after another. 
	// Correspondingly, the time-out interval is divided into MAXIMUM_WAIT_OBJECTS.

	DWORD 	nCurrentCount, dwHandleIndex = WAIT_TIMEOUT, dwHandleOffset,
			dwCurrentTimeOut = (1000 * dwTimeOut) / (nCount / MAXIMUM_WAIT_OBJECTS + 1);
	PHANDLE	lpCurrentHandle;

	for (lpCurrentHandle = lpHandles,
		 nCurrentCount = nCount > MAXIMUM_WAIT_OBJECTS ? MAXIMUM_WAIT_OBJECTS : nCount; 
		 nCurrentCount && (dwHandleIndex == WAIT_TIMEOUT);
		 lpCurrentHandle +=  nCurrentCount,   
		 nCurrentCount = nCount - (lpCurrentHandle - lpHandles) > MAXIMUM_WAIT_OBJECTS ? 
		 	MAXIMUM_WAIT_OBJECTS : nCount - (lpCurrentHandle - lpHandles)) {

		dwHandleIndex = WaitForMultipleObjectsEx(nCurrentCount, lpCurrentHandle, 
					FALSE, dwCurrentTimeOut, FALSE);
		dwHandleOffset = lpCurrentHandle - lpHandles;
	}
	if (dwHandleIndex == WAIT_TIMEOUT)
		dwHandleIndex = ACH_TIMEOUT;		
	else if (dwHandleIndex != 0xFFFFFFFF) 
		dwHandleIndex += dwHandleOffset;
	return dwHandleIndex;
}

BOOL Wait(WAIT_STATUS *pStatus, int *piTest, int *piChild) { 

	DWORD	dwHandleIndex = ACH_TIMEOUT,	// hArray index to terminetd child 
			dwExitCode,						// child process exit code
			nCount,							// number of handles to wait for
			dwTimeOut,						// time-out interval
			dwElapsedTime;					// time since the last timer int
	int		i, j;
	HI_RES_TIME now;
	STATUS	 status;

	/* allocate handles array when first time
	*/

	if (lpHandles == NULL) {

		/*	Go through tests and calculate number of chaild processes
		*/

		nCount = 0;
		for (i = 0 ; i < numtests ; i++)
			nCount += tests[i]->t_nkids;
		if (nCount == 0)
			return FALSE;		// no children 


		lpHandles = (PHANDLE) MEreqmem(0, nCount * sizeof (HANDLE), TRUE, &status);
	    if (status != OK) {
	    	SIprintf("Can't MEreqmem space for handles array\n");
	    	PCexit(FAIL);
	    }
	}

	/* Look for killed child process
	*/

	if (FoundKilledProcess(pStatus, piTest, piChild))
		return TRUE;

	/*	Loop here untill next child is done
	*/

	while (dwHandleIndex == ACH_TIMEOUT) {

		/*	Go through tests and collect all eligible handles 
			into handles array.
		*/

		nCount = 0;
		for (i = 0 ; i < numtests ; i++) {
			for (j = 0 ; j < tests[i]->t_nkids ; j++) {
				if (tests[i]->t_children[j].a_processInfo.hProcess) 
					lpHandles[nCount++] = 
						tests[i]->t_children[j].a_processInfo.hProcess;
			}
		}
		if (nCount == 0)
			return FALSE;		// all children 

		/*	Calculate next time-out interval = wait interval - 
				time elapsed since last interrupt
		*/

		ACgetTime(&now, (LO_RES_TIME *) NULL);
		dwElapsedTime = ACcompareTime(&now, &gInterruptTime);
		if (gWaitTime <= (i4)dwElapsedTime) {	
			gWaitTime = gWaitInterval;		// in case it took too long
			gInterruptTime = now;
			alarm_handler();
			if (FoundKilledProcess(pStatus, piTest, piChild))
				return TRUE;
			ACgetTime(&now, (LO_RES_TIME *) NULL);
			dwElapsedTime = ACcompareTime(&now, &gInterruptTime);
			dwTimeOut = gWaitTime + (((dwElapsedTime - gWaitTime) / gWaitInterval) + 1) *
					gWaitInterval - dwElapsedTime;
		} else 
			dwTimeOut = gWaitTime - dwElapsedTime;

		/*	wait untill something happens
		*/

/*		if ((dwHandleIndex = 
				WaitForMultipleObjectsEx(nCount, lpHandles, 
					FALSE, dwTimeOut * 1000, FALSE)) == 0xFFFFFFFF) { */
		if ((dwHandleIndex = 
				WaitForProcesses(nCount, dwTimeOut)) == 0xFFFFFFFF) {
			*pStatus = 0;
			SIprintf("WaitForMultipleObjectsEx faild. Error code = %d\n", GetLastError());
			return FALSE;
		}
		if (dwHandleIndex == ACH_TIMEOUT) {
			ACgetTime(&gInterruptTime, (LO_RES_TIME *) NULL);
			gWaitTime = gWaitInterval;
			alarm_handler();
			if (FoundKilledProcess(pStatus, piTest, piChild))
				return TRUE;
		}
	}

	GetExitCodeProcess(lpHandles[dwHandleIndex - WAIT_OBJECT_0], &dwExitCode); 
	*pStatus = dwExitCode;
	CloseHandle(lpHandles[dwHandleIndex - WAIT_OBJECT_0]);

	/* find the test which just has finished 
	*/

	for (i = 0 ; i < numtests ; i++) {
		for (j = 0 ; j < tests[i]->t_nkids ; j++) {
			if (tests[i]->t_children[j].a_processInfo.hProcess ==
				lpHandles[dwHandleIndex - WAIT_OBJECT_0]) {
				tests[i]->t_children[j].a_processInfo.hProcess = 0;
				*piTest = i;
				*piChild = j;
				return TRUE;
			}
		}
	}
	return FALSE;
}

VOID ACchildHandler ()
{
	register i4    i,
		       j,
		       pid;
	char	      *fname;
	char	       timebuf [80];
	extern PID     achpid;
	HI_RES_TIME    now;
	ACTIVETEST    *curtest;
	struct stat    st;
	int		termsig =0;
	int		retcode=0;

	ACblock(ALARM | STATRPT | ABORT);
	while ( Wait(&retcode, &i, &j))
	{	//1
		ACgetTime(&now, (LO_RES_TIME *) NULL);
		curtest = &(tests[i]->t_children[j]);
		/** keep running total of child times for each instance **/
		tests[i]->t_ctime+= ACcompareTime(&now,&curtest->a_start)/3600.0F;
		/*SIfprintf(logfile,"tests[%d]->t_ctime %f\n",i,tests[i]->t_ctime,'.'); */ 

		/* Log the dead process. */
		/** verify the state first to see if it didnt die normally **/
		 if (retcode) 
			 tests[i]->t_cbad++;

		log_entry(&now, i, j, curtest->a_iters, pid, C_EXIT,
			  retcode, &curtest->a_start);
		curtest->a_iters++;
		/* If more iteration if this instance should be run, start a
		   new one.
		*/
		if ( (curtest->a_iters <= tests[i]->t_niters)
		  || (tests[i]->t_niters <= 0) ) 
        {     //2
		    active_children--;
		    start_child(i, j, curtest->a_iters);
			ResumeThread(tests[i]->t_children[j].a_processInfo.hThread);
		/** add to total children running - vers 2 - jeffr **/
        	tests[i]->t_ctotal++;
		 }	//2
		else
		{	//2
			/* If we don't need to start any more tests, clean up
			   the output file: If no output file was specified in
			   the config file, and the "temporary" output file
			   remained empty (ie no unexpected error messages),
			   remove it. If it shouldn't be removed, make it owned
			   by the user who's running this program. (It's owned
			   by root for now.)
			*/
			LOtos(&curtest->a_outfloc, &fname);
			if ( (!*tests[i]->t_outfile)
			  && (!stat(fname, &st) )
			  && (st.st_size == 0) )
				unlink(fname);
			/* Record the end time of the last process in a_start,
			   for use by print_status.
			*/
			curtest->a_start = now.time;
			active_children--;

			/*	make sure it is not included into wait handles array
				next time
			*/

			curtest->a_processInfo.hProcess = 0;	

		    ACgetTime (&now, (LO_RES_TIME *) NULL);
		    ACtimeStr (&now, timebuf);
		    SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n", 
			           timebuf, "ACHILLES WAIT", achpid, "SUSPENDED");
		    SIflush(logfile);
		}	//2
	/** add to total children run for this test - includes all tagged children **/
	}	//1
	/* If there are no more outstanding child processes, we're done */

    /* time and date stamp the conclusion of the achilles test run
       11/16/89, bls
    */
	MEfree(lpHandles);
    ACgetTime (&now, (LO_RES_TIME *) NULL);
    ACtimeStr (&now, timebuf);
    SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n", 
               timebuf, "ACHILLES END", achpid, "END");
    SIflush(logfile);
	/** generate report **/
    ACreport();

	if (logfile != stdout) fclose(logfile);
	PCexit(OK);
	return;
} /* ACchildHandler */

STATUS ACfindExec (tempdata, gooddata, data_end, argv, cur_argv)
char  *tempdata,
     **gooddata,
     **data_end,
      *argv[];
i4   *cur_argv;
{
	char	      exname[128], path[128];
	i4	      i;
	static char **pathlist = NULL;


	/* Make sure it's .exe */

	STcopy(tempdata, path);
	if (STindex(path, ".exe", 0) == NULL) 
		STcat(path, ".exe");
	/* If user specified a location, look there first. */

	if (testpath)
	{	//1
		STprintf(exname, "%s\\%s", testpath, path);
		if(!access(exname, 0))
			*gooddata = exname;
	}	//1

	/* Next, search the directories in the PATH environment variable. */

	if (!*gooddata)
	{	//1
		/* If the PATH variable hasn't already been translated into an
		   argv-like array of strings, do the conversion now.
		*/

		if (!pathlist)
		{	//2
			if(ACgetPathList(&pathlist) < 0)
				return(FAIL);
		}	//2
		for (i =0 ; pathlist[i] ; i++)
		{	//2
			STprintf(exname, "%s\\%s", pathlist[i], path);
			if(!access(exname, 0)) 
			{	//3
				*gooddata = exname;
				break;
			}
		}	//2
	}	//1

	/* Finally, look in the current directory. If we find it here, we don't
	   have to prepend a directory name.
	*/
#ifdef USE_LOCAL
	if ( !*gooddata)
	{	//1
		STcopy(tempdata, exname);
		if(!access(exname, 0))
			*gooddata = tempdata;
	}	//1
#endif

	if(!*gooddata)
		fprintf(stderr, "Can't find %s\n", tempdata);
	return(*gooddata ? OK : !OK);
} /* ACfindExec */

VOID ACforkChild (testnum, curtest, iternum)
i4	    testnum;
ACTIVETEST *curtest;
i4 iternum;
{
	char		*fname, *cmdLine;
    extern FILE *filename;        /* logfile name               */
    extern int   silentflg;       /* discard front-end stdout ? */
                                  /* 10-27-89, bls              */
	int			fdIn, fdOut;
	int			f_mode;
	int			i, cmdLength;
	STARTUPINFO	startupInfo;
	STATUS		status;

	if (*tests[testnum]->t_infile) {
		if ((fdIn = open(tests[testnum]->t_infile, O_RDONLY)) < 0)
			{
				curtest->a_processInfo.dwProcessId = 0;
				perror(tests[testnum]->t_infile);
				return;
			}
		if (fdIn != 0)
			dup2(fdIn, 0);
	}

                                     /* if "silent mode" has been specified */
                                     /* throw away front-end stdout, and    */
                                     /* re-direct stderr to achilles logfile*/
                                     /* 10-27-89, bls                       */

	if (silentflg)
		fname = "nul";
	else
		LOtos(&curtest->a_outfloc, &fname);
	f_mode = (O_WRONLY | O_CREAT | O_TRUNC);
	
	if ((fdOut = _open(fname, f_mode, 0666)) < 0)
		{
		    perror(fname);
		    fdOut = fileno(stdout);
        }

	if (fdOut != 1) {
		/*dup2(fd, 1); */
		dup2(fileno (logfile), 1); 
		/** stdout should point to logfile now -  
		child now writes to where stdout is pointing to,
		before it wrote to where stderr was pointing to
		- jeffr **/
	}
	if (fdOut != 2)
	{
		dup2(fileno (logfile), 2);
		/*** dup2(fd, 2); ***/
	}
	
	/** allocate command line **/

	for (i = 0; curtest->a_argv[i]; i++);
	cmdLength = curtest->a_argv[i - 1] + 
		STlength(curtest->a_argv[i - 1]) + 1 - curtest->a_argv[0];
	cmdLine = (char*)MEreqmem(0, cmdLength, TRUE, &status);
	if( status != OK ) {
		SIprintf("Can't MEreqmem cmdLine\n");
		curtest->a_processInfo.dwProcessId = 0;
		return;
	}

	/** create argument string **/

	for (i = 0; i < cmdLength - 1; i++) {
		if (*(*curtest->a_argv + i))			
			*(cmdLine + i) = *(*curtest->a_argv + i);
		else
			*(cmdLine + i) = ' ';
	}
	*(cmdLine + i) = '\0';

	/** create the child process **/				

	startupInfo.cb = sizeof(STARTUPINFO);
   	startupInfo.lpReserved = NULL;
   	startupInfo.lpReserved2 = NULL;
   	startupInfo.cbReserved2 = 0;
   	startupInfo.lpDesktop = NULL;
	startupInfo.lpTitle = NULL;
   	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
   	startupInfo.wShowWindow = SW_HIDE;
	startupInfo.hStdInput = 0;
	startupInfo.hStdOutput = 0;
	startupInfo.hStdError = 0;
   	if (!CreateProcess (NULL, cmdLine, NULL, NULL, TRUE, 
    					CREATE_SUSPENDED, NULL, NULL, &startupInfo, 
						&curtest->a_processInfo)) {
		curtest->a_processInfo.dwProcessId = 0;
		MEfree(cmdLine);
		return;
	}
	MEfree(cmdLine);

/** add one to global active children - jeffr **/
	
	active_children++;

} /* ACforkChild */

ACgetPathList (plist)
char ***plist;
{
	register char *p;
	register int i;
	char *tmppath, *path;
	int word_cnt = 0;
	STATUS	status;

	if ( (tmppath = getenv("PATH") ) == NULL)
	{
		*plist = (char **) MEreqmem(0, sizeof(char *), TRUE, &status);
	  	if( status != OK )
		{
			SIprintf("Can't MEreqmem *plist\n");
			return(-1);
		}
	}
	else
	{
		path = MEreqmem(0, STlength(tmppath)+1, TRUE, &status);
		if( status != OK )
		{
			SIprintf("Can't MEreqmem tmppath\n");
			return(-1);
		}
		STcopy(tmppath, path);
		for (p = path ; *p ; p++) {
			if (*p == ';')
				word_cnt++;
		}
		*plist = (char **) MEreqmem(0, (word_cnt+2)*sizeof(char *),
		  			TRUE, &status);
		if(status != OK )
		{
			SIprintf("Can't MEreqmem word_cnt\n");
			return(-1);
		}
		(*plist)[0] = path;
		i = 1;
		for (p = path ; *p ; p++) {
			if (*p == ';')
			{
				*p = '\0';
				(*plist)[i++] = p+1;
			}
		}
		(*plist)[i] = NULL;
	}
	return(0);
} /* ACgetPathList */

VOID ACinitAlarm (waittime)
i4 waittime;
{
	ACgetTime(&gInterruptTime, (LO_RES_TIME *) NULL);
	if( waittime >= 0 )  {

		char *p;

		gWaitTime = waittime;
		gWaitInterval = 1;
		if(p=getenv("II_ACH_TIMER"))
			gWaitInterval = atoi(p);
	}
} /* ACinitAlarm */

VOID ACioini()
{
}

VOID ACsetTerm ()
{
} /* ACsetTerm */


extern VOID ACchildHandler(), print_status(), abort_handler();

VOID ACsetupHandlers ()
{
//	set_terminate(abort_handler);
} /* ACsetupHandlers */

GLOBALDEF int optind = 1;
GLOBALDEF char *optarg = "";

i4 ACgetopt (argc, argv, valids)
i4 argc;
char **argv, *valids;
{
	char *p, *strchr();

	if ( (optind >= argc) || (*argv[optind] != '-') )
		return(-1);
	if ( (p = strchr(valids, argv[optind][1]) ) != NULL)
		if (*(p+1) == ':')
			if (argv[optind][2] == '\0')
				if (++optind < argc)
					optarg = argv[optind];
				else
					return('?');
			else
				optarg = argv[optind] + 2;
	optind++;
	return(p ? *p : '?');
} /* ACgetopt */

VOID ACgetEvents() {

	ACchildHandler();
}

VOID ACreleaseChildren() {

	int	i,j;

	for (i = 0 ; i < numtests ; i++) {
		for (j = 0 ; j < tests[i]->t_nkids ; j++) {
			if (tests[i]->t_children[j].a_processInfo.hThread) 
				ResumeThread(tests[i]->t_children[j].a_processInfo.hThread);
		}
	}
}



