/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <pc.h>
#include    <si.h>
#include    <ck.h>
#include    <cm.h>
#include    <errno.h>
#include    <si.h>
#include    <st.h>

/**
**
**  Name: CKSPAWN.C - Run a CK command of the host operating system
**
**  Description:
**	Run a CK command on the host operating system
**
**	    CK_spawn()		- spawn a CK command and wait
**	    CK_spawnnow()	- spawn a CK command and do not wait
**	    CK_is_alive()	- is a CK command alive?
**          CK_execute_cmd()	- spawn a non-echo CK command and wait
**
**  History:
**      27-oct-1988 (anton)
**	    Created.
**	03-may-1989 (fredv)
** 	    For some reasons, IBM RT's system() call always gets an interrupt
**	 	signal and returns fail. Use our PCcmdline() works a lot 
**		better. So added a special case for IBM RT.
**	08-may-89 (daveb)
**		If system() doesn't work somewhere, but PCcmdline does,
**		just use PCcmdline everywhere.  Removed the ifdef for
**		rtp_us5 and made if the default.
**
**		It would have been better, in my opinion, to have
**		put this relatively general interface into PC rather
**		than burying it here in CK.  (You would want this
**		style call, because it has a CL_ERR_DESC, unlike
**		PCcmdline.)
**	08-jan-90 (sylviap)
**		Added PCcmdline parameters.
**      08-mar-90 (sylviap)
**              Added a parameter to PCcmdline call to pass an CL_ERR_DESC.
**	26-apr-1993 (bryanp)
**	    Prototyping CK for 6.5.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-apr-95 (canor01)
**	    added <errno.h>
**      15-May-1996 (hanch04)
**	    added CK_spawnnow
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings - the first parameter passed
**	    to PCcmdline is a LOCATION.
**	06-oct-1998 (somsa01)
**	    In CK_spawn() and CK_spawnnow(), if errno is zero and PCcmdline()
**	    does not return OK, then the spawned command returned an error.
**	    Use CK_COMMAND_ERROR as the return code to reflect this.
**	    (SIR #93752)
**	02-nov-1998 (schte01)
**       Added include for st.h which is needed when using STlength.  
**       Without it, an undefined will be caused for those platforms that
**       #define STlength as strlen.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	09-nov-1999 (somsa01)
**	    On NT, we cannot fork processes. Therefore, implemented
**	    CK_spawnnow() so that on NT we would create a thread which
**	    would spawn the checkpoint command and wait. The callers of
**	    CK_spawnnow() on NT would actually get back a thread id
**	    instead of a pid.
**	16-nov-1999 (somsa01)
**	    Forgot to #ifdef the declaration of CK_spawnnow_thread().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-may-2002 (somsa01)
**	    In CK_spawn() and CK_spawnnow(), due to localization issues,
**	    if this is an "echo" command just use SIprintf() to print out
**	    the string.
**      22-jul-2004 (rigka01) bug 111666, problem INGSRV2426
**	    Also bug 110741, problem INGSRV2476
**          The ckpdb and rollforwarddb messages from cktmpl.def
**          files do not print out in their entirety
**          Add local function, CK_execute_cmd().
**/

STATUS CK_execute_cmd(char *command, CL_ERR_DESC *err_code);

#ifdef NT_GENERIC
static i4	CK_spawnnow_thread(LPVOID  thread_arg);
#endif


/*{
** Name: CK_spawn - spawn a CK command and wait
**
** Description:
**	Run the suppled string as a command as ingres which hopefully
**	does a CK operation.
**
** Inputs:
**	command			command to run
**
** Outputs:
**	err_code		machine dependent error return
**
**	Returns:
**	    E_DB_OK
**	    CK_SPAWN		Command would not run or exited failure
**
**	Exceptions:
**	    if enabled
**	    EXCHLDDIED		when command finishs (SIGCHLD)
**
** Side Effects:
**	    none
**
** History:
**	27-oct-1988 (anton)
**	    Created.
**	03-may-1989 (fredv)
** 	    For some reasons, IBM RT's system() call always gets an interrupt
**	 	signal and returns fail. Use our PCcmdline() works a lot 
**		better. So added a special case for IBM RT.
**	08-may-89 (daveb)
**		If system() doesn't work somewhere, but PCcmdline does,
**		just use PCcmdline everywhere.  Removed the ifdef for
**		rtp_us5 and made if the default.
**	08-jan-90 (sylviap)
**		Added PCcmdline parameters.
**      08-mar-90 (sylviap)
**              Added a parameter to PCcmdline call to pass an CL_ERR_DESC.
**	06-oct-1998 (somsa01)
**	    If errno is zero and PCcmdline() does not return OK, then the
**	    spawned command returned an error. Use CK_COMMAND_ERROR as the
**	    return code to reflect this.  (SIR #93752)
**	15-may-2002 (somsa01)
**	    Due to localization issues, if this is an "echo" command
**	    just use SIprintf() to print out the string.
**      22-jul-2004 (rigka01) bug 111666, problem INGSRV2426
**	    Also bug 110741, problem INGSRV2476
**          The ckpdb and rollforwarddb messages from cktmpl.def
**          files do not print out in their entirety
**          Add local function, CK_execute_cmd().
*/

STATUS
CK_spawn(
char		*command,
CL_ERR_DESC	*err_code)
{
    STATUS	ret_val = OK;
#ifdef NT_GENERIC
    {
	char *FC=NULL,    /* ptr to First Character of (remaining) string */ 
	     *LC=NULL,    /* ptr to Last Character of (remaining) string  */ 
	     *ODQ=NULL,   /* ptr to Open Double Quote in (remaining) string */
	     *CDQ=NULL,   /* ptr to Close Double Quote in (remaining) string */
	     *FA=NULL,    /* ptr to First And (&) in (remaining) string */
	     *FE=NULL,    /* ptr to start of First Echo string */
	     *LC_NEXT=NULL; /* ptr to null character following LC */
	/* 	
	** cktmpl.def file entry may have multiple commands.
	** Commands are separated by "&".
	** "&" may appear within an echo command within double quotes. 
	** CreateProcess only processes one command at a time.
	** Commands need to be parsed out and sent through one
	** at a time. 
	** Echo commands will be sent out through SIprintf.
	** All other commands will be executed through PCcmdline
	** (via CK_execute_cmd) which executes CreateProcess.
	** Single quotes can appear in the string and do not require
	** special processing. 
	*/ 
	/*
	** Find the last character of the string (LC). 
	** The string ends in a null character.
	*/
            LC = &command[STlength(command)-1]; 
            LC_NEXT = &command[STlength(command)]; 

	    FC=command;
	/*
	** Parse the cktmpl.def entry
	*/
	while ( FC < LC_NEXT )
	{
	   /*
	   ** Find the first character of the string (FC).
	   ** Skip over blanks as well as special characters such as ",&
	   */
           while ( (FC != LC_NEXT) &&
                   !CMalpha(FC) && !CMdigit(FC) )
           {
               CMnext(FC);
           }
	   if ( FC == LC_NEXT ) /* nothing to execute */ 
	   {
		break;
	   }
	   /*
	   ** If FC=LC, string consists of a single character command
	   ** Send off FC to be executed and return.
	   */
	   if ( (FC == LC) ) 
	   {
	 	ret_val=CK_execute_cmd(FC, err_code);
		return (ret_val);
	   }
  	   /*	
	   ** Find first opening double quote if any (ODQ); if none, ODQ=LC+1
	   */
           if ((ODQ = STindex(FC, "\"", 0)) == NULL)
	   {
		ODQ = LC_NEXT;   /* no next quote */
	        CDQ = LC_NEXT; 
	   }
	   else
	   /*
  	   ** Find the closing double quote if any (CDQ); if none, CDQ=LC+1
	   */
	   {
 	     CDQ=ODQ;
	     CMnext(CDQ); 
             if ((CDQ = STindex(CDQ, "\"", 0)) == NULL)
	       CDQ = LC_NEXT;  /* no closing quote */
	   } 
	   /*
	   ** Is the command an echo command? 
	   */
           if ((FE = STstrindex(FC, "echo ", sizeof("echo "), TRUE)) == NULL)
		FE = LC_NEXT;
	   /*
	   ** Find the first valid "&" if any (FA); if none, FA=LC+1
	   **   find first FA starting from FC
	   **   while ODQ<FA<CDQ (& not a cmd connector as its within quotes) 
	   **        find next FA (ignore this &)
	   */
           if ((FA = STindex(FC, "&", 0)) != NULL)
	   {   /* there is an & somewhere in the string */
	     /*
	     ** check position of & in relation to quotes 
	     */  
	     while (FA != LC_NEXT) 
	     {
	       if ( (ODQ < FA) && (FA < CDQ) ) /*& between quotes*/ 
	       {
                 if ((FA =  STindex(CDQ,  "&", 0)) == NULL) /* get next & */
		 {
		   FA = LC_NEXT; /* no more & between quotes */
		   break;  
		 }
	       }
	       else
	       {
	         if (FA > CDQ) /* & comes after current set of quotes    */ 
	         {   /* setup to check position of & with next set of quotes */
	           ODQ=CDQ;
	           CMnext(ODQ);	
                   if ((ODQ = STindex(ODQ,  "\"", 0)) == NULL) 
		   {
		     ODQ = LC_NEXT;   /* no next quote */
		     CDQ = LC_NEXT;
		   }
		   else /* look for closing quote */ 
	           {
 	             CDQ=ODQ;
	             CMnext(CDQ); 
                     if ((CDQ = STindex(CDQ, "\"", 0)) == NULL)
		       CDQ = LC_NEXT; /* no closing quote */
	           } 
	         }
	         else 
		   break; /* & before current set of quotes */	
	       }
	     }
	     *FA = 0; 
	     if (FA < LC)
	       CMnext(FA); 
	   }
	   else  /* there is no & somewhere in the (remaining) string */
	   {
 	     FA = LC_NEXT;
	   }
	   if (FE != FC) /* not an echo cmd */
	   {
	 	if ((ret_val=CK_execute_cmd(FC, err_code)) != OK)
		    return (ret_val);
	   }
	   else /* cmd is echo cmd */
	   {
		CMnext(FC); /* skip past "e" */
		CMnext(FC); /* skip past "c" */
		CMnext(FC); /* skip past "h" */
		CMnext(FC); /* skip past "o" */
		CMnext(FC); /* skip past " " */
		SIprintf("%s\n", FC);
		SIflush(stdout);
	   }
	   FC=FA; /* remaining string starts after & */ 
	} /* END WHILE */
    } 
	 
#else 
    ret_val=CK_execute_cmd(command, err_code);
#endif

    return(ret_val);
}

/*{
** Name: CK_execute_cmd - spawn a non-echo CK command and wait
**
** Description:
**	Run the suppled string as a command as ingres which hopefully
**	does a CK operation.
**
** Inputs:
**	command			command to run
**
** Outputs:
**	err_code		machine dependent error return
**
**	Returns:
**	    E_DB_OK
**	    CK_SPAWN		Command would not run or exited failure
**
**	Exceptions:
**	    if enabled
**	    EXCHLDDIED		when command finishs (SIGCHLD)
**
** Side Effects:
**	    none
**
** History:
**	03-aug-2004 (rigka01)
**	    Created as copy of previous version of CK_spawn()
*/
STATUS
CK_execute_cmd(
char		*command,
CL_ERR_DESC	*err_code)
{
    STATUS	ret_val = OK;
    i4		os_ret;

    if ((os_ret=PCcmdline((LOCATION *) NULL, command, PC_WAIT, NULL, err_code)))
    {
	SETCLERR(err_code, 0, ER_system);
	if (err_code->errnum)
	    ret_val = CK_SPAWN;
	else
	{
	    err_code->moreinfo[0].size = sizeof(os_ret);
	    err_code->moreinfo[0].data._i4 = os_ret;

	    /*
	    ** Let's make sure that we do not overflow what we have left
	    ** to work with in the moreinfo structure. Thus, since we have
	    ** (CLE_INFO_ITEMS-1) unions left to use, the max would be
	    ** CLE_INFO_MAX*(CLE_INFO_ITEMS-1).
	    */
	    err_code->moreinfo[1].size = min(STlength(command),
		CLE_INFO_MAX*(CLE_INFO_ITEMS-1));
	    STncpy(err_code->moreinfo[1].data.string, command,
		    err_code->moreinfo[1].size);
	    err_code->moreinfo[1].data.string[ err_code->moreinfo[1].size ] = EOS;
	    ret_val = CK_COMMAND_ERROR;
	}
    }

    return(ret_val);
}

/*{
** Name: CK_spawnnow - spawn a CK command and do not wait
**
** Description:
**      Run the suppled string as a command as ingres which hopefully
**      does a CK operation.
**
** Inputs:
**      command                 command to run
**
** Outputs:
**      err_code                machine dependent error return
**
**      Returns:
**          E_DB_OK
**          CK_SPAWN            Command would not run or exited failure
**
**      Exceptions:
**          if enabled
**          EXCHLDDIED          when command finishs (SIGCHLD)
**
** Side Effects:
**          none
**
** History:
**	15-May-1996 (hanch04)
**          Created from CK_spawn.
**	06-oct-1998 (somsa01)
**	    If errno is zero and PCcmdline() does not return OK, then the
**	    spawned command returned an error. Use CK_COMMAND_ERROR as the
**	    return code to reflect this.  (SIR #93752)
**	09-nov-1999 (somsa01)
**	    On NT, we cannot fork processes. Therefore, implemented
**	    CK_spawnnow() so that on NT we would create a thread which
**	    would spawn the checkpoint command and wait. The callers of
**	    CK_spawnnow() on NT would actually get back a thread id
**	    instead of a pid.
**	16-nov-1999 (somsa01)
**	    err_code is a pointer.
**	15-may-2002 (somsa01)
**	    Due to localization issues, if this is an "echo" command
**	    just use SIprintf() to print out the string.
*/
 
STATUS
CK_spawnnow(
char            *command,
PID		*pid,
CL_ERR_DESC     *err_code)
{
    STATUS		ret_val = OK;
# ifndef NT_GENERIC
    i4		os_ret;
# else
    char		*thread_arg;
# endif /* NT_GENERIC */
 
# ifdef NT_GENERIC
    {
	char	*strptr;

	/*
	** Due to localization issues, if this is an "echo" command,
	** just use SIprintf() to print out the string.
	*/
	if ((strptr = STrstrindex(command, "echo ", 0, TRUE)))
	{
	    strptr = strptr + 5;
	    while (strptr != &command[STlength(command)-1] &&
		   !CMalpha(strptr) && !CMdigit(strptr))
	    {
		strptr++;
	    }

	    SIprintf("%s\n", strptr);
	    SIflush(stdout);
	    return(ret_val);
	}
    }
# endif  /* NT_GENERIC */

# ifndef NT_GENERIC

    if ( (*pid = (PID)PCfork( &ret_val )) > 0 )   /* parent */
    {
        /*
        ** (v)fork returns control to parent after exec.
        */
	return(ret_val);
    }
    else if ( *pid == 0 )                               /* child */
    {
	if ((os_ret=PCcmdline((LOCATION *) NULL, command, PC_WAIT, NULL,
				err_code)))
	{
	    SETCLERR(err_code, 0, ER_system);
	    if (err_code->errnum)
		ret_val = CK_SPAWN;
	    else
		ret_val = CK_COMMAND_ERROR;

	    /*
	    ** b122528
	    ** We need to get our own, unique, pid.
	    ** - That which was returned by the fork above.
	    ** So we can communicate errors properly by the ensuing error call. 
	    */
	    PCpidOwn(pid);
	    CK_write_error(pid, ret_val, err_code->errnum, os_ret, command);
	}

	exit( ret_val );
    }
    else if ( *pid < 0 )                               /* child failed */
    {
	SETCLERR(err_code, 0, ER_system);
	ret_val = CK_SPAWN;
    }

# else

    thread_arg = STalloc(command);

    *pid = PCfork((LPTHREAD_START_ROUTINE)CK_spawnnow_thread,
		  (LPVOID)thread_arg, &ret_val);
    if (pid < 0)
    {
	SETCLERR(err_code, ret_val, ER_system);
	ret_val = CK_SPAWN;
    }

# endif /* NT_GENERIC */

    return(ret_val);
}

# ifdef NT_GENERIC
/*{
** Name: CK_spawnnow_thread - thread code for CK_spawnnow (NT Only)
**
** Description:
**	This thread basically executes CK_spawn().
**
** Inputs:
**      thread_arg              address of thread argument structure
**
** Outputs:
**      thread_arg              filled in with machine dependent error return
**
**      Returns:
**          E_DB_OK
**          CK_SPAWN            Command would not run or exited failure
**
**      Exceptions:
**          if enabled
**          EXCHLDDIED          when command finishs (SIGCHLD)
**
** Side Effects:
**          none
**
** History:
**	09-nov-1999 (somsa01)
**	    Created.
*/

static i4
CK_spawnnow_thread(
LPVOID	thread_arg)
{
    STATUS	ret_val = OK;
    i4	os_ret;
    PID		pid;
    char	*command = (char *)thread_arg;
    CL_ERR_DESC	err_code;

    if ((os_ret=PCcmdline((LOCATION *)NULL, command, PC_WAIT, NULL,
			  &err_code)))
    {
	SETCLERR(&err_code, 0, ER_system);
	if (err_code.errnum)
	    ret_val = CK_SPAWN;
	else
	    ret_val = CK_COMMAND_ERROR;

	pid = GetCurrentThreadId();
	CK_write_error(&pid, ret_val, err_code.errnum, os_ret, command);
    }

    return(ret_val);
}
# endif /* NT_GENERIC */

/*{
** Name: CK_is_alive - is a CK command alive?
**
** Description:
**	This function basically executes PCis_alive() on non-NT platforms
**	and PCisthread_alive() on NT.
**
** Inputs:
**	pid			The PID / Thread ID to check.
**
** Outputs:
**
**      Returns:
**	    Return code from PCis_alive() of PCisthread_alive().
**
**      Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	10-nov-1999 (somsa01)
**	    Created.
*/

bool
CK_is_alive(
PID	pid)
{
#ifndef NT_GENERIC
    return(PCis_alive(pid));
#else
    return(PCisthread_alive(pid));
#endif /* NT_GENERIC */
}

