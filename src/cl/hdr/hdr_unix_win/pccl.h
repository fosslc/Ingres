/*
 *	Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PC.h
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		defines global symbols for PC module
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
**	11-may-89 (daveb)
**		Remove 2.0 sccsid.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Comment out string following #endif;
**		Change PID to be a i4  (int) which is POSIX.
**	24-may-1993 (bryanp)
**	    Add prototype for PCforce_exit() and PCis_alive().
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	7-Jul-1993 (daveb)
**	    Put in extern for getpid, which we use in a macro. 
**	14-Nov-96 (gordy)
**	    Added PCtid() macro.
**	12-dec-96 (mcgem01)
**	    Add PCtid() macro for NT.
**	12-dec-1996 (donc)
**	    Add OK_TERMINATE definition for OpenROAD
**	18-feb-1997 (canor01)
**	    For NT, include <process.h> instead of <unistd.h> to get
**	    declaration for getpid().
**      09-May-1997 (merja01)
**          Change pthread_self to __pthread_self for Digital Unix 4.0.
**	15-may-1997 (muhpa01)
**	    Don't have PCtid() return thread id when POSIX_DRAFT_4
**	03-Dec-1998 (muhpa01)
**	    Removed code for POSIX_DRAFT_4 on HP - obsolete
**	08-Feb1997 (peeje0101)
**	    Change return type of getpid() from int to pid_t
**	09-nov-1999 (somsa01)
**	    Changed PCfork() on NT, and added PCwait().
**      08-Dec-1999 (podni01)  
**          Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**          with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**          this code for some reason removed all over the place.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Aug-2002 (hanch04)
**	    Added PCdoexeclp64().
**	23-Aug-2002 (yeema01) Sir # 108552
**	    For OpenROAD Mainwin 4.02 port,added define xCL_069_UNISTD_H_EXISTS
**	    here in order to pick up <unistd.h> which will have correct
**	    declaration for getpid. This is required because OpenROAD
**	    codes do not include clconfig.h which contain this cl define.
**	13-Feb-2003 (hanje04)
**	    BUG 109555
**	    Add PCmpid() to specifically get the master process ID for a multi-
**	    threaded program on Linux using getpgrp(). On all other platforms
**	    this will just be defined at PCpid().
**	19-Feb-2003 (hanje04)
**	    Replace _lnx's with generic LNX
**	17-Jun-2003 (hanje04)
**	    Define PCpid to be getpgrp() for all Linux platforms and remove
**	    references to PCmpid().
**	31-Mar-2005 (srisu02)
**          Added prototype for PCdoexeclp32 to resolve compilation errors 
**          in AIX 
**	25-Jul-2007 (drivi01)
**	    Define a return code for PCexit function.  Return code will 
**	    correspond to a Windows OS error code 740 which is 
**	    ERROR_ELEVATION_REQUIRED when user attempts to execute
**	    a program that can't run under stripped token.  PC version
**	    of the error will be PC_ELEVATION_REQUIRED.
**      26-Aug-2009 (coomi01) b122528
**          On Linux platforms, PCpid gets the group pid. This causes
**          problems where a parent process knows the child pid, and at
**          times wants to read a unique error file with details of child
**          failure, commonly err.$pid, So define yet another macro where
**          get the pid means get THE pid. We also define the same macro
**          in a sensible fashion for all other platforms so that we will
**          not fall foul of build errors.
**      15-nov-2010 (stephenb)
**          Add proto for PCspawn.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**	2-Dec-2010 (kschendel) SIR 124685
**	    Include unistd.h for getpgrp() on linux.  unistd.h probably
**	    should be included in compat, todo for later.
*/

# ifndef	PID

# define	PIPE		i4

#define		OK_TERMINATE	0xFF

# ifdef xde
# define xCL_069_UNISTD_H_EXISTS
# endif

/*
	defines macro PCpid to set passed pid to current process id
*/

# ifdef NT_GENERIC
# define	PID		DWORD
# define	PCpid(pid)	*pid = GetCurrentProcessId()
# define	PCpidOwn(pid)	*pid = GetCurrentProcessId()
typedef enum
{
    HIDE = 0,
    NORMAL,
    MINIMIZED,
    MAXIMIZED,
    NOACTIVATE,
    SHOW,
    MINIMIZE,
    MINNOACTIVE,
    SHOWNA,
    RESTORE,
    SHOWDEFAULT
} WINDPROP;

# else
# define	PID		i4
# ifdef LNX
/* FIXME unistd.h probably should be included in compat.h -- do here for now. */
#include <unistd.h>
# define        PCpid(pid)     *pid = getpgrp()
                /* For Uniqueness on Linux, we may need to avoid the abiguity of the above macro 
                */ 
# define        PCpidOwn(pid)  *pid = getpid()
# define        PCsetpgrp()     setpgrp();
# else
# define	PCpid(pid)	*pid = getpid()
# define        PCpidOwn(pid)   *pid = getpid()
# endif
# endif

/*
** Defines macro to return current thread ID.
** For none threaded systems, return fixed value.
*/

# if defined(OS_THREADS_USED) && !defined(DCE_THREADS)

# ifdef POSIX_THREADS

# ifdef axp_osf

# define	PCtid()		(i4)__pthread_self()

# else /*axp_osf*/

# define	PCtid()		(i4)pthread_self()

# endif /*axp_osf */

# endif /* POSIX_THREADS */

# ifdef SYS_V_THREADS

# define	PCtid()		(i4)thr_self()

# endif

# ifdef NT_GENERIC

# define	PCtid()		(i4)GetCurrentThreadId()

# endif

# else /* OS_THREADS_USED */

# define	PCtid()		((i4)0)

# endif /* OS_THREADS_USED */

/*
	returned by UNIX wait() when no children are alive
*/

# define	SYS_NO_CHILDREN		 -1

# define	PC_WAIT			  1
# define	PC_NO_WAIT		  0
# define	PC_ELEVATION_REQUIRED	  740

/* symbols for starting up debuggers in the backend or other subprocess */
# define	PC_ADB		"adb"
# define	PC_ADBI30	"adbi30"
# define	PC_SDB		"sdb"
# define	PC_DBX		"dbx"
# define	PC_DBXI30	"dbxi30"
# define	PC_STAND	"stand"

/* externs of system things we use directly through macros */

# if defined(__STDC__) && defined(xCL_069_UNISTD_H_EXISTS)
#	include <systypes.h>
#	include <unistd.h>
# else
# ifdef NT_GENERIC
#	include <process.h>
# else
	extern pid_t getpid();
# endif /* NT_GENERIC */
# endif /* __STDC__ && xCL_069_UNISTD_H_EXISTS */

# endif /* PID */

#ifndef NT_GENERIC
FUNC_EXTERN i4  PCfork( 
	STATUS *status
#else
FUNC_EXTERN PID PCfork(
	LPTHREAD_START_ROUTINE	thread_func,
	LPVOID			func_arg,
	STATUS			*status
#endif /* NT_GENERIC */
);

FUNC_EXTERN STATUS PCwait(
	PID	proc
);

FUNC_EXTERN STATUS PCdocmdline( 
		LOCATION *      interpreter, 
		char *          cmdline,
		i4             wait,
		i4             append,
		LOCATION *      err_log,
		CL_ERR_DESC *   err_code);

FUNC_EXTERN STATUS PCdowindowcmdline(
		LOCATION *      interpreter,
		char *          cmdline,
		i4             wait,
		i4             append,
		i4             property,
		LOCATION *      err_log,
		CL_ERR_DESC *   err_code);

FUNC_EXTERN STATUS  PCdoexeclp64(
            	i4		argc,
            	char		**argv,
	    	bool		exitonerror
);

FUNC_EXTERN STATUS  PCdoexeclp32(
            	i4		argc,
            	char		**argv,
	    	bool		exitonerror
);
FUNC_EXTERN STATUS PCspawn(
	i4 		argc,
	char		**argv,
	i1		wait,
	LOCATION	*in_name,
	LOCATION	*out_name,
	PID		*pid
);
i4	PCreap_withThisPid(int targetPid);

