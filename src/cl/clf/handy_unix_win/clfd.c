/*
**Copyright (c) 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <si.h>
#include    <clnfile.h>
#include    <errno.h>

#if defined(any_aix) || defined(sparc_sol) || \
    defined(a64_sol) || defined(LNX)
# define    CL_FD_SETSIZE  8192  
  static i4 max_count = 0;
#endif /* aix, solaris, LNX */
#if defined(OSX)
# define    CL_FD_SETSIZE  FD_SETSIZE
  static i4 max_count = 0;
#endif /* OSX */

/**
**
**  Name: CLFD.C - return number of file descriptors.
**
**  Description:
**	Return maximum number of file descriptors for the current process.
**
**	Moved some ifdef logic from header clnfile.h to an actual function.
**
**          iiCL_get_fd_table_size() - return max number of file descriptors.
** 	    iiCL_increase_fd_table_size() - increase max # of file descriptors.
**
**  History:
**      05-Nov-92 (mikem)
**         Created, during su4_us5 port.
**	30-nov-1992 (rmuth)
**	   Fixed problem when trying to increase the the number of
**	   file descriptors.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	03-aug-1998 (canor01)
**	    remove extraneous definition of errno.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Jan-2003 (hweho01)
**          The number of file handler can be set to unlimited (32767)
**          on AIX, but the maximum is defined to 8192 only in clpoll.c.       
**          To avoid error "FATAL: fd surpasses max fd" in iiCLfdreg(),    
**          need to limit the number of handler to be equal or less than 
**          8192. Star issue 12282800.
**	28-aug-2003 (hayke02)
**	    Extend the above fix for bug 109536 to su4_us5 and su9_us5 -
**	    Solaris 9 allows 65536 file handles so the limit of 8192 can
**	    be exceeded.
**	13-sep-04 (toumi01)
**	    For LNX limit open files to (FD_SETSIZE - connect_limit) so that
**	    we don't fail in clpoll with "iiCLfdreg FATAL". TODO: determine
**	    whether we can raise poll fd limit and so support a higher hard
**	    "open files" limit.
**	24-sep-04 (toumi01)
**	    For LNX don't dedicate connect_limit FDs because we're only
**	    dealing with a pool of 1024 at this point.
**	27-sep-04 (toumi01)
**	    Fix comment delimit typo in previous connect_limit fix.
**      13-Jan-2005 (hanje04)
**          Back out toumi01's 3 previous changes limiting open files
**          on Linux to FD_SETSIZE. xCL_RESERVE_STREAM_FDS is now defined
**          in clsecret.h which causes the lowest 255 fd's to be reserved
**          for stream activity.
**	25-Feb-2005 (hanje04)
**	    Re-instigate toumi01's change to limit FD_SETSIZE to 1024 on
**	    Linux. Implementing xCL_RESERVE_STREAM_FDS has caused a number
** 	    of problems. Until these can be resolved it cannot be used.
**	    See issues 13941751 & 13935786 for more info
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**      05-Nov-2007 (hweho01)
**          For a64_sol, use the same number of file handler as it is    
**          defined in clpoll. 
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	01-apr-2008 (toumi01)
**	    Activate 8192 fd limit for Linux change.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/

/*{
** Name: iiCL_get_fd_table_size() - Return max # of file descriptors available.
**
** Description:
**	If available use system calls to determine maximum number of file
**	descriptors available to a process.  If possible this call should 
**	return the actual number for FD's allocated to the current process 
**	rather than some #define from a system header.  
**
**	Many operating system now provide
**	ways for processes to change the number of file descriptors available
**	to a process.  Methods for determining the number of file descriptors
**	also vary from OS to OS.  The following hierarchy of calls will be
**	used to determine which method to use:
**
**	#if xCL_034_GETRLIMIT_EXISTS 
**	    ** SVR4 and BSD **
**	    use getrlimit()
**	#else if xCL_004_GETDTABLESIZE_EXISTS
**	    use getdtablesize()
**	#else if machine specific stuff
**	    use some machine specific stuff - hopefully can be generalized in
**	    future based on new xCL capabilities.
**	#else if 
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    maximum number of file descriptors for current process.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Nov-92 (mikem)
**         Created, during su4_us5 port.
**	9-Aug-07 (kibro01) b118007
**	   Honour the RLIM_INFINITY (which is usually -1) which causes
**	   the recovery server to fail to start.
**	25-Oct-2007 (hanje04)
**	    BUG 114907
**	    We actually want the hard limit for file descriptors rather
**	    than the current limit as that is the error the caller reports
**	    should the returned value be to low.
**      20-Mar-2009 (Ralph Loen) Bug 120471
**          Put back the soft limit instead of the hard limit specified in
**          the fix for bug 114907.  The error message was actually
**          incorrect, not the behavior in iiCL_get_fd_table_size().
*/
i4
iiCL_get_fd_table_size()
{
    i4	max_fds = 0;

#if defined(xCL_034_GETRLIMIT_EXISTS) && defined(RLIMIT_NOFILE)
    /* getrlimit() comes with SVR4 and BSD - should be the "normal" case */
    {
	struct rlimit	rlim;

	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
	{
#if defined(RLIM_INFINITY)
	    if (rlim.rlim_cur == RLIM_INFINITY)
		rlim.rlim_cur = MAXI4;
#endif
	    max_fds = rlim.rlim_cur;
	}
    }
#else
#   if defined(xCL_004_GETDTABLESIZE_EXISTS)
    {
	max_fds = getdtablesize();
    }
#   else
    {
	/* see clnfile.h for twisty set of ifdef's */
	max_fds = CL_NFILE();
    }
#   endif /* defined(xCL_004_GETDTABLESIZE_EXISTS) */
#endif /* defined(xCL_034_GETRLIMIT_EXISTS) && defined(RLIMIT_NOFILE) */

#if defined(any_aix) || defined(sparc_sol) || \
    defined(LNX) || defined(OSX) || defined(a64_sol)
/* Limit the number of file handler to be not greater than the number of      
** bits in fd_set (FD_SETSIZE, also sel->count in iiCLfdreg). 
** It needs to include the number of connect_limit in the calculation.
*/   
 if ( max_count == 0 )                         /* Is the first pass ? */ 
   { 
     struct rlimit user_limit; 
     char *limit_ptr;
     i4  num, status ; 
/* For Linux we are currently using the 1024 glibc FD_SETSIZE limit, so
** don't decrement the connect_limit, which is a theoretical high water
** for connections; rather, make all 1024 available as needed for whatever
** purpose and hope that our LRU handling will keep the open FD count down
*/
#if defined(LNX) || defined(OSX)
     num = 0;
#else
     if ( PMget( "!.connect_limit", &limit_ptr ) != OK )
         num = 20;  
     else
         num = 20 + atoi ( limit_ptr );
#endif /* defined(LNX) */
     max_count = CL_FD_SETSIZE - num; 
     if ( max_fds > max_count )     
      {
        max_fds = max_count ;     /* need to reset the limit */ 
        user_limit.rlim_max = user_limit.rlim_cur = max_count ; 
        status = setrlimit( RLIMIT_NOFILE, &user_limit );
        if (status != 0 )
           TRdisplay("clfd : setrlimit returned errno=%d status=%d\n",
                             errno, status );
       }
   }
#endif /* solaris, aix, LNX || OSX */

    return(max_fds);
}

/*{
** Name: iiCL_increase_fd_table_size() - increase max # of file desc available.
**
** Description:
**	If available use system calls to increase maximum number of file
**	descriptors available to a process.  If possible this call should 
**	return the actual number for FD's allocated to the current process 
**	rather than some #define from a system header.  
**
**	Currently the 2 known ways to increase the number of file descriptors
**	available to a process is with setrlimit() or setdtablesize().
**
**	The following hierarchy of calls will be used to determine which 
**	method to use:
**
**	#if xCL_034_GETRLIMIT_EXISTS 
**	    ** SVR4 and BSD **
**	    use setrlimit()
**	#else if xCL_004_GETDTABLESIZE_EXISTS
**	    use setdtablesize()
**	#else if machine specific stuff
**	    nothing currently.
**	#else if 
**	
**	The function returns the current number of file descriptors allocated
**	to the process.
**
** Inputs:
**	maximum			if TRUE then set fd table to maximum possible.
**	increase_fds		if maximum is FALSE then set fd table to
**				    "increase_fds" # of fd's if it is not that
**				     big already.
**
** Outputs:
**	none.
**
**	Returns:
**	    maximum number of file descriptors for current process.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Nov-92 (mikem)
**         Created, during su4_us5 port.
**      30-nov-1992 (rmuth)
**	   when make call to getrlimit need to set both the rlim_cur and
**	   rlim_max fields to new value.
**	9-Aug-07 (kibro01) b118007
**	   Honour the RLIM_INFINITY (which is usually -1) which causes
**	   the recovery server to fail to start.
*/
i4
iiCL_increase_fd_table_size(maximum, increase_fds)
i4 maximum;
i4	increase_fds;
{
    i4 ret_max_fds = 0;

    ret_max_fds = iiCL_get_fd_table_size();

#if defined(xCL_034_GETRLIMIT_EXISTS) && defined(RLIMIT_NOFILE)
    /* getrlimit() comes with SVR4 and BSD - should be the "normal" case */
    {
	struct rlimit	rlim;

	if (maximum)
	{
	    getrlimit(RLIMIT_NOFILE, &rlim);
#if defined(RLIM_INFINITY)
	    if (rlim.rlim_cur == RLIM_INFINITY)
		rlim.rlim_cur = MAXI4;
#endif
	    increase_fds = rlim.rlim_max;
	}

	if (increase_fds > ret_max_fds)
	{

	    rlim.rlim_max = rlim.rlim_cur = increase_fds;

	    _VOID_ setrlimit(RLIMIT_NOFILE, &rlim);
	}
    }
#else
#   if defined(xCL_062_SETDTABLESIZE)
    {
	i4	table_size;
	i4 setdt_ret;

	if (maximum)
	{
	    /* loop adding one at a time */
	    increase_fds = MAXI4;
	    table_size = iiCL_get_fd_table_size();;
	    while (table_size < increase_fds)
	    {
		setdt_ret = setdtablesize(table_size + 1);
		if ((setdt_ret < 0) || (setdt_ret <= table_size))
		{
		    /* break when error or no increase in table size */
		    break;
		}
		else
		{
		    table_size = setdt_ret;
		}
	    }
	}
	else
	{
	    if (increase_fds > ret_max_fds)
		ret_max_fds = setdtablesize(increase_fds);
	}
    }
#   else
    {
	/* no way to increase file descriptors */
    }
#   endif /* defined(xCL_062_SETDTABLESIZE) */

#endif /* defined(xCL_034_GETRLIMIT_EXISTS) && defined(RLIMIT_NOFILE) */

    ret_max_fds = iiCL_get_fd_table_size();

    return(ret_max_fds);
}
