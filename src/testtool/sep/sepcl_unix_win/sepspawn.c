#include <compat.h>
#include <clconfig.h>
#include <systypes.h>
#include <clnfile.h>
#include <lo.h>
#include <pc.h>
#include <er.h>
#include <ep.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#ifdef NT_GENERIC
#include <me.h>
#endif


# define sepspawn_c

#include <sepdefs.h>
#include <fndefs.h>

#ifdef  xCL_023_VFORK_EXISTS
#define SEPfork vfork
#else
#ifndef NT_GENERIC
#define SEPfork fork
#endif
#endif

#include <errno.h>

/*
** History:
**
**	##-may-1989 (mca)
**		Created.
**	15-sep-1989 (mca)
**		Added a formal parameter so that SEP and Listexec can both
**		use this function.
**	25-sep-1989 (mca)
**		Allow output from OS/TCL commands to stderr to be caught on
**		Unix.
**	6-12-1989 (eduardo)
**		Added closure of descriptors for child process using
**		CL routines
**	08-jul-1991 (johnr)
**		hp3_us5 change for systypes
**	08-jul-1991 (johnr)
**		Made systype change generic.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	28-jan-1992 (jefff)
**	    Changed batchMode GLOBALREF to be bool, consistent with
**	    the GLOBALDEF
**	24-feb-1992 (donj)
**	    Add in tracing code to trace dialog between SEP and SEPSON.
**      12-Nov-92 (GordonW)
**          Added:
**              1) SEPfork symbol.
**              2) error checking in spawning
**	29-jan-1993 (donj)
**	    Took out code that removed the "ferun" and "run" commands from
**	    the spawned command line before spawning.
**    26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	11-Jan-1994 (fredv)
**	    Moved <clconfig.h>, <clnfile.h>, <systypes.h> up so that getpid()
**		will declare properly on hp8.
**	 3-may-1994 (donj)
**	    SEPspawn() was stripping the '"''s off of argv's. Removed the code.
**	    It was screwing up delimited ID command line switches.
**	11-may-1994 (donj)
**	    Removed trace_nr GLOBALREF. Not Used.
**	22-mar-1995 (holla02)
**	    Reinstated code removed in change 413097 (3-may-1994 above) because
**	    UNIX needs '"''s stripped from "-Rroleid", "-Ggroupid", etc.  Added
**	    code to make stripping conditional on BOTH 1st char of argv == '"'
**	    (as before) AND 2nd char == '-', so that delimited ID handling
**	    remains consistent.  Updated to use CMnext, CMcmpcase, etc.
**	03-may-1995 (holla02)
**	    Added to the above change to strip double quotes from "+N" cmdline
**	    options as well as "-N".
**
**	??-sep-1995 (somsa01)
**	    Ported to NT_GENERIC platforms. For this platform, the child is
**	    now another executible (sepchild.c). Interprocess communication
**	    between parent and child is accomplished by using shared memory.
**	27-Feb-1996 (somsa01)
**	    Added support for passing delimited names to INGRES on the command
**	    line for NT_GENERIC. Because of the need of the use of the
**	    backslash character, this case needed to be specially handled.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	09-jun-2000 (kitch01) Bug 102022
**	   Added extra error checking for Windows to prevent hangs caused by
**	   shared memory segment not being updated if the process cannot be
**	   spawned.
**	   Also hangs on NT 4 SP6 have been resolved by sleeping for a minute
**	   amount of time, just enough to allow the child to update shared
**	   memory and for the shared memory to be flushed.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-sep-2000 (mcgem01)
**	    More nat and longnat to i4 changes.
**	29-jul-2002 (somsa01)
**	    Corrected the delimiting of delimiters, as an argument can
**	    have multiple delimited phrases.
**  17-Oct-2007 (drivi01)
**	    sep doesn't check for return code when IIMEget_pages is called
**	    which results in a SEGV if the function fails.  IIMEget_pages
**      may fail b/c the previously created shared segment wasn't cleaned
**      up yet.  Added routines to retry the allocation and if all fails
**      error gracefully without a SEGV.
**	12-May-2009 (drivi01)
**	     Force elevation on Vista in this module.
**	02-Oct-2009 (bonro01)
**	    Increase sleep to eliminate 100% cpu use
**	07-Apr-2010 (drivi01)
**	    update allocated_pages datatype to be SIZE_TYPE to account for
**	    x64 port.  Add better error checking.
*/

GLOBALREF bool batchMode;

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

#ifdef NT_GENERIC
GLOBALREF    char          SEPpidstr [6] ;

struct shm_struct {
  bool batchMode;
  i4  stderr_action;
  char prog_name[64];
  char in_name[256];
  char out_name[256];
  bool child_spawn;
};
#endif

STATUS
SEPspawn(i4 argc,char **argv,bool wait_for_child,LOCATION *in_loc,LOCATION *out_loc,PID *pid,i4 *exitstat,i4 stderr_action)
{
    STATUS		ret_val = OK;
    char		*in_name = NULL;
    char		*out_name = NULL;
    char		prog_name[64];
    char		**nargv = NULL;
    PID			dummypid;
    char		**p = NULL;
    char		*q = NULL;
    register i4		i;
#ifdef NT_GENERIC
    char		full_prog_name[256];
    char		shm_name[20];
    char		*delimptr, *delimptr2;
    CL_ERR_DESC		err_code;
    SIZE_TYPE		allocated_pages;
    STATUS		mem_status;
	int			try_count=100;
    struct shm_struct	*childenv_ptr ZERO_FILL;
    STARTUPINFO		startinfo;
    PROCESS_INFORMATION	procinfo;
    char		arg_line[512], temparg[256];
    int			j, k, retcode;
    bool		delim_found;

    startinfo.cb = sizeof (STARTUPINFO);
    startinfo.lpReserved = 0;
    startinfo.lpDesktop = NULL;
    startinfo.lpTitle = NULL;
    startinfo.dwX = 0;
    startinfo.dwY = 0;
    startinfo.dwXSize = 0;
    startinfo.dwYSize = 0;
    startinfo.dwXCountChars = 0;
    startinfo.dwYCountChars = 0;
    startinfo.dwFillAttribute = 0;
    startinfo.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    startinfo.wShowWindow = SW_HIDE;
    startinfo.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    startinfo.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
    startinfo.hStdError=GetStdHandle(STD_ERROR_HANDLE);
    startinfo.cbReserved2 = 0;
    startinfo.lpReserved2 = 0;

    strcpy (shm_name, SEPpidstr);
    strcat (shm_name, "sep.shm");

    ELEVATION_REQUIRED();

    mem_status = IIMEget_pages(
                     ME_MZERO_MASK|ME_IO_MASK|ME_SSHARED_MASK|ME_CREATE_MASK,
                     1,shm_name,&childenv_ptr,&allocated_pages,&err_code);

	/* 
	** Check error code returned from MEget_pages, do not
	** assume that MEget_pages will always be successful
	*/
	while ((mem_status == ME_ALREADY_EXISTS || err_code.errnum == ME_ALREADY_EXISTS ) && try_count-->0)
	{
		/*
		** If memory segment already exists, it's possible the segment hasn't
		** been cleaned up yet from last transaction.
		** Sleep for a fraction of a second and try again.
		** sep will give it 100 tries to attempt to allocate shared segment,
		** it's probably too big of a number b/c it could mean a wait of up to 
		** 10 seconds, but on slower machines this maybe needed.
		*/
		PCsleep(100);
		mem_status = IIMEget_pages(
						ME_MZERO_MASK|ME_IO_MASK|ME_SSHARED_MASK|ME_CREATE_MASK,
						1, shm_name, &childenv_ptr, &allocated_pages, &err_code);
	}
	if ( ((mem_status != 0) && (mem_status != ME_ALREADY_EXISTS || err_code.errnum != ME_ALREADY_EXISTS)) || (childenv_ptr == NULL) )
	{
 		char	buf[MAX_PATH+64];
		int	errnum;

		errnum = GetLastError();
		STprintf(buf, "Couldn't allocate shared memory, errno %d and mem_status %d, err_code.errnum %d", errnum, mem_status, err_code.errnum);
		SIfprintf(stderr, "%s\n", buf);
		SIflush(stderr);
		ret_val = FAIL;
	}

#endif

    if (in_loc != NULL)
	LOtos(in_loc, &in_name);
    if (out_loc != NULL)
	LOtos(out_loc, &out_name);

    nargv = argv;

    for (p = nargv ; *p ; p++)
    {
       q = *p + STlength(*p)-1;
       if ((CMcmpcase(*p,DBL_QUO) == 0)&& (CMcmpcase(q,DBL_QUO) == 0))
           if ((CMcmpcase(*p+1,HYPHEN) == 0) || (CMcmpcase(*p+1,PLUS) == 0))
           {
               CMnext (*p);
               *q = EOS;
           }
    }

    if (tracing&TRACE_DIALOG)
    {
	SIfprintf(traceptr,ERx("SEPspawn argc = %d\n"),argc);
	for (i=0; i<argc; i++)
	    SIfprintf(traceptr,ERx("SEPspawn argv[%d]> %s\n"),i,argv[i]);
    }

    STcopy(nargv[0], prog_name);
    if (exitstat)
	*exitstat = 0;
    if (!pid)
	pid = &dummypid;
#ifndef NT_GENERIC
    if ( (*pid = SEPfork() ) == 0)
    {

	/*
	**	close all descriptors except for stdin, stdout & stderr
	*/

	i4	fd;

	for (fd = 3; fd < CL_NFILE(); fd++)
	{
		close(fd);
	}

	if (batchMode || in_name)
	{
	    char *fin = NULL ;
	    fin = ( in_name ? in_name : ERx("/dev/null"));
	    freopen(fin, ERx("r"), stdin);
	}

	if (batchMode || out_name)
	{
	    char *fout = NULL ;

	    fout = ( out_name ? out_name : ERx("/dev/null"));
	    freopen(fout, ERx("w"), stdout);
	}

	switch (stderr_action)
	{
	    case 1:
		freopen(ERx("/dev/null"), ERx("a"), stderr);
		break;
	    case 2:
		dup2(1, 2);
		break;
	}
	execvp(prog_name, nargv);

        /* Handle error condition */
        {
                char    buf[64];
                int     errnum = errno;

                STprintf(buf, "Can't execvp %s, errno %d", prog_name, errnum);
                SIfprintf(stderr, "%s\n", buf);
                SIflush(stderr);
        }
	_exit(509);
    }
    else if( *pid > 0 )
    {
	if (wait_for_child)
	{
	    int status, stat;

	    errno = 0;
	    while ( ((stat=wait(&status)) != *pid) ||
                    (stat < 0 && errno == EINTR) )
		errno = 0;

	    if ( ( (status >> 8) & 511) == 509)
		ret_val = FAIL;
	    else
	    {
		if (exitstat)
		    *exitstat = (i4) (status >> 8);
	    }
	}
    }
    else
    {
	/* Handle error condition */
	{
		char	buf[64];
		int	errnum = errno;

		STprintf(buf, "Can't fork %s, errno %d", prog_name, errnum);
		SIfprintf(stderr, "%s\n", buf);
		SIflush(stderr);
	}
	ret_val = FAIL;
    }
#else
	if (ret_val != FAIL)
	{
    childenv_ptr->batchMode = batchMode;
    childenv_ptr->stderr_action = stderr_action;
    STcopy(prog_name, childenv_ptr->prog_name);
    if (in_loc != NULL)
      STcopy(in_name, childenv_ptr->in_name);
    else
      STcopy(ERx(""), childenv_ptr->in_name);
    if (out_loc != NULL)
      STcopy(out_name, childenv_ptr->out_name);
    else
      STcopy(ERx(""), childenv_ptr->out_name);
    childenv_ptr->child_spawn = FALSE;
	}
    nargv[argc]=shm_name;

    /*
    ** CreateProcess() drops delimiters. Since we will need to pass any
    ** delimited strings "as is" to the program being executed, we need to
    ** delim the delimiters themselves.
    */
    strcpy (arg_line, "");
    for (j=0; j<=argc; j++)
    {
	delim_found = FALSE;
	delimptr = nargv[j];
	k = 0;
	memset(temparg, '\0', sizeof(temparg));
	if (j != 0)
	    temparg[k++] = ' ';
	while (*delimptr)
	{
	    if (*delimptr == '"' && *(delimptr+1) == '\\' &&
		*(delimptr+2) == '"')
	    {
		/*
		** We found the beginning of a delimiter.
		*/
		delimptr += 2;
		temparg[k++] = '"';
		temparg[k++] = '\\';
		temparg[k++] = '"';
		temparg[k++] = '\\';
		temparg[k++] = '\\';
		temparg[k++] = '\\';
		temparg[k++] = '"';
		delim_found = TRUE;
	    }
	    else if (delim_found && *delimptr == '\\' &&
		     *(delimptr+1) == '"' && *(delimptr+2) == '"')
	    {
		/*
		** We found the end of a delimiter.
		*/
		delimptr += 2;
		temparg[k++] = '\\';
		temparg[k++] = '\\';
		temparg[k++] = '\\';
		temparg[k++] = '"';
		temparg[k++] = '\\';
		temparg[k++] = '"';
		temparg[k++] = '"';
		delim_found = FALSE;
	    }
	    else
		temparg[k++] = *delimptr;

	    CMnext(delimptr);
	}

	strcat(arg_line, temparg);
    }

	/* Get return code so we don't hang if no sepchild.exe */
    retcode = SearchPath (NULL, "sepchild", ".exe", sizeof(full_prog_name), 
							full_prog_name, NULL);
	if (retcode == 0)
	{
 		char	buf[64];
		int	errnum;

		errnum = GetLastError();
		STprintf(buf, "Can't find sepchild.exe, errno %d", errnum);
		SIfprintf(stderr, "%s\n", buf);
		SIflush(stderr);
		ret_val = FAIL;
	}
	else
	{
		if (CreateProcess (full_prog_name,arg_line,NULL,NULL,TRUE,
                           CREATE_NEW_PROCESS_GROUP|NORMAL_PRIORITY_CLASS,NULL,
                           NULL,&startinfo,&procinfo) == TRUE)
		{  /* Child was created successfully */
			if (wait_for_child)
			{
				int status;

			    WaitForSingleObject(procinfo.hProcess,INFINITE);

				GetExitCodeProcess(procinfo.hProcess,&status);

				if (status == 509)
				ret_val = FAIL;
				else
				{
				if (exitstat)
					*exitstat = (i4) (status);
				}
			}
		}
		else
		{
			/* Handle error condition */
			char	buf[64];
			int	errnum;

			errnum = GetLastError();
			STprintf(buf, "Can't fork %s, errno %d", prog_name, errnum);
			SIfprintf(stderr, "%s\n", buf);
			SIflush(stderr);
			ret_val = FAIL;
		}
	}

	/* There is no point checking the shared memory if there has been a
	** previous failure since the child process cannot update the
	** shared memory 
	*/
	if (ret_val != FAIL)
	{
		/* Wait for shared memory to be updated by child 
		** sleep for a minute amount of time which is sufficient
		** to prevent hangs on NT 4.0 SP6 caused by timing faults
		*/
		while (childenv_ptr->child_spawn == FALSE)
			SleepEx(10,TRUE);
	}

    *pid = procinfo.dwProcessId;
    mem_status = IIMEfree_pages(childenv_ptr,1,&err_code);
    mem_status = IIMEsmdestroy(shm_name,&err_code);
#endif
    return(ret_val);
} /* SEPspawn */
