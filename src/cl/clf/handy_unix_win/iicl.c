/*
** Copyright (c) 1995, 2004 Ingres Corporation
*/
#include   <compat.h>
#include   <gl.h>
#include   <me.h>
#include   <st.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <clsocket.h>
#if defined(LNX) 
#include   <stdlib.h>
extern char *__dg_fcvt_r( double, int, int *, int *, char * );
extern char *__dg_ecvt_r( double, int, int *, int *, char * );
#endif
# ifdef UNIX
#include   <pwd.h>
#if defined(sparc_sol) || defined(a64_sol)
#include   <floatingpoint.h>
#endif
# endif /* UNIX */
#include   <diracc.h>
#include   <handy.h>
#if defined(sgi_us5) || defined(OSX)
#include   <cs.h>
GLOBALREF       CS_SEMAPHORE     CL_misc_sem;
#endif

#if defined(usl_us5)
#include   <cs.h>
GLOBALDEF       CS_SYNCH	fcvt_mutex ZERO_FILL;
#endif

# if defined(thr_hpux) || defined(sparc_sol)
# include   <netdb.h>
# endif
# if defined(thr_hpux)
	extern int h_errno;
# endif

#ifndef NT_GENERIC
/*
** iireaddir.c -- MT-safe call to readdir.  
**
** Description:
**	This wrapper function allows the caller to access the readdir
**	function in a reentrant fashion
**
** Input:
**	dirp		Pointer to DIR structure.
**	readdir_buf	Pointer to a user-supplied buffer.
**
** Output:
**	dirp		Pointer to filled DIR structure, in the reentrant
**			case, this will be the user supplied buffer.
**
** History:
**	23-may-95 (tutto01)
**	    Created.
**	01-may-1997 (toumi01)
**	    Provide appropriate readdir_r function based on platform
**	    for su4_us5, rs4_us5, and axp_osf.
**	17-jun-1997 (muhpa01)
**	    Added reentrant versions of calls to readdir_r(), getpwnam_r(),
**	    getpwuid_r(), gethostbyname_r(), gethostbyaddr_r(), fcvt_r(),
**	    and ecvt_r() for hp8_us5.
**	24-jun-1997 (hayke02)
**	    Use readdir_r() rather than readdir() in iiCLreaddir() for
**	    Tandem Nonstop (ts2_us5).
**	15-dec-1997 (hweho01)
**          Function readdir_r() does not return the correct data  
**          on DG/UX ( up to OS release R4.11MU03 ), so use the 
**          non-reentrant function with the semaphore calls to   
**          serialize the execution.
**	03-apr-1998 (muhpa01)
**	    Removed definitions for hp8_us5 (obsolete - no OS_THREADS) and
**	    added definitions for hpb_us5
**	19-May-1998 (fucch01)
**	    Use reentrant readdir_r() for sgi_us5...
**      18-Jan-1999 (fanra01)
**          Add iiCLttyname for thread safe call to ttyname.
**	10-feb-1999 (fanra01)
**	    Remove unneeded netdb.h which was required as part of the
**	    UNIX debugging exercise.
**	12-feb-1999 (mcgem01)
**	    The function iiCLttyname is not required on NT.
**	19-Feb-1999 (loera01) Bug 95183
**	    For rs4_us5, evaluated the status return code of gethostbyname_r()
**	    before filling hostp with hostent.  For AIX, hostent will be 
**	    filled with the local host's address, if gethostbyname_r() fails.
**	    In this case, hostent must be nullified to agree with generic
**	    treatment by iiCLgethostbyname().
**	    readdir_r.
**	08-aug-1999 (mcgem01)
**	    Changed nat to i4.
**	27-Apr-1999 (ahaal01)
**	    Adding a cast of (char *) to ttyname_r() resolves a compile
**	    error on AIX (rs4_us5).
**	17-May-1999 (hweho01)
**	    Added reentrant versions of calls to readdir_r(), getpwnam_r(),
**	    getpwuid_r(), gethostbyname_r(), gethostbyaddr_r(), fcvt_r(),
**	    ecvt_r() and ttyname_r() for AIX 64-bit (ris_u64) platform.
**	09-Jun-2000 (ahaal01)
**	    Added reentrant versions of calls to getpwnam_r() and getpwuid_r()
**	    for AIX (rs4_us5) platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	30-nov-98 (stephenb)
**	    64-bit solaris builds use the final posix draft version of
**	    readdir_r.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	08-apr-2002 (somsa01)
**	    We have readdir_r for su9_us5 as well.
**	23-Sep-2002 (hweho01)
**	    Defined appropriate iiCLttyname() routine for rs4_us5 platform.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	20-May-2003 (hanje04)
**	    Linux builds should use AIX (POSIX) version of ttyname_r()
**      15-Mar-2005 (bonro01)
**          Added support for Solaris AMD64 a64_sol.
**	01-Apr-2005 (bonro01)
**	    Additional changes for Solaris AMD64 a64_sol.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	07-Feb-2008 (lunbr01)  Bug 119802
**	    On AIX, DNS reverse lookup (gethostbyaddr_r) is checking wrong
**	    IP address due to wrong level of indirection.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	15-Sep-2009 (hanje04)
**	    Return NULL on failure for axp_osf, OSX etc as failure to do
**	    so causes problems on OSX.
**	1-Dec-2010 (kschendel)
**	    Drop obsolete ports.
**	3-Dec-2010 (kschendel)
**	    Turns out DESKTOP was on for windows, fix.
*/
 
struct dirent
*iiCLreaddir(DIR *dirp, struct dirent *readdir_buf)
{

#if (defined(OS_THREADS_USED) && !defined(NT_GENERIC))

# if defined(sparc_sol) || defined(a64_sol)
# define have_readdir_r
#if (!defined(LP64))
    return(readdir_r(dirp, readdir_buf));
#else /* LP64 */
    int status;
    struct dirent *return_dirent;
    status = readdir_r(dirp, readdir_buf, &return_dirent);
    if (!status)
       return(return_dirent);
    else
       return(NULL);
#endif /* LP64 */
# endif /* solaris */

# if defined(any_aix)
# define have_readdir_r
    int status;
    struct dirent *return_dirent;
    status = readdir_r(dirp, readdir_buf, &return_dirent);
    if (!status)
       return(return_dirent);
    else
       return(NULL);
# endif /* aix */

# if defined(axp_osf) || defined(thr_hpux) || defined(OSX)
# define have_readdir_r
    int status;
    struct dirent *return_dirent = NULL;
    status = readdir_r(dirp, readdir_buf, &return_dirent);
    if (status)
    {
	TRdisplay("iicl: readdir_r returned errno=%d status=%d\n", errno, status);
	return(NULL);
    }
    return(return_dirent);
# endif /* axp_osf */

# if defined(sgi_us5)
# define have_readdir_r
    struct dirent *return_dirent;
    return( readdir_r(dirp, readdir_buf, &return_dirent) == 0 ? return_dirent : NULL );
# endif /* sgi_us5 */

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# ifndef have_readdir_r

    return(readdir(dirp));

# endif /* ! have_readdir_r */

}



/*
** iigetpwnam -- MT-safe call to getpwnam.  
**
** Description:
**	This wrapper function allows the caller to access the getpwnam
**	function in a reentrant fashion.
**
** Input:
**
** Output:
**
** History:
**	23-may-95 (tutto01)
**	    Created.
**	04-Dec-1995 (walro03/mosjo01)
**	    "const" causes compile errors on DG/UX (dg8_us5, dgi_us5).
**	    Replaced with READONLY.
**	03-jun-1996 (canor01)
**	    password structure must be passed in by caller.
**	01-may-1997 (toumi01)
**	    Provide appropriate getpwnam_r function based on platform
**	    for su4_us5, rs4_us5, and axp_osf.
**	10-sep-1997/28-apr-1997 (hweho01)
**	    For Unixware (usl_us5), the reentrant and non-reentrant 
**	    versions of getpwnam have the same name.
**      02-oct-1997 (hweho01)
**          Provide appropriate getpwnam_r function for DG/UX.
**	19-May-1998 (fucch01)
**	    Use reentrant getpwnam_r() for sgi_us5...
**      08-Nov-2002 (hweho01)
**          Set return_pwd to NULL, if there is bad status code   
**          from getpwnam_r(). 
**	16-dec-2002 (somsa01)
**	    On HP-UX, getpwnam_r() does not return a pointer to a password
**	    entry location in the case of INTERNAL threads. Thus, use
**	    getpwnam() instead in this case.
**	19-mar-2004 (somsa01)
**	    Itanium Linux uses getpwnam_r().
*/
 
struct passwd
*iiCLgetpwnam(READONLY char *name, struct passwd *pwd, char *pwnam_buf, int size)
{

# if defined(OS_THREADS_USED) && !defined(NT_GENERIC)

# if defined(sparc_sol) || defined(a64_sol)
# define have_getpwnam_r
    return(getpwnam_r(name, pwd, pwnam_buf, size));
# endif /* solaris */

# if defined(axp_osf) || defined(thr_hpux) || defined(any_aix) || \
     defined(OSX)
# define have_getpwnam_r
    static bool cs_mt = TRUE;
    static bool checked = FALSE;
    int status;
    struct passwd *return_pwd;

# if defined(thr_hpux)
    if (!checked)
    {
	char *str = NULL;

	NMgtAt("II_THREAD_TYPE", &str);
	if ((str) && (*str))
	{
	    if (STbcompare(str, 0, "INTERNAL", 0, TRUE) == 0)
		cs_mt = FALSE;
	}

	checked = TRUE;
    }
# endif  /* HP */

    if (cs_mt)
    {
	status = getpwnam_r(name, pwd, pwnam_buf, size, &return_pwd);
	if (status)
	{
	    return_pwd = NULL; 
	    TRdisplay("iicl: getpwnam_r returned error %d\n", status);
	}
	return(return_pwd);
    }
    else
	return(getpwnam(name));
# endif /* axp_osf */

# if defined(usl_us5)
# define have_getpwnam_r
    return(getpwnam(name));  
# endif  /* usl_us5 */

# if defined(sgi_us5) || defined(i64_lnx)
# define have_getpwnam_r    
    struct passwd *return_pwd;
    return( getpwnam_r(name, pwd, pwnam_buf, size, &return_pwd) == 0 ? return_pwd : NULL );
# endif /* sgi_us5 */

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# ifndef have_getpwnam_r

    return(getpwnam(name));

# endif /* ! have_getpwnam_r */

}



/*
** iiCLgetpwuid -- MT-safe call to getpwuid.
**
** Description:
**      This wrapper function allows the caller to access the getpwuid
**      function in a reentrant fashion.
**
** Input:
**
** Output:
**
** History:
**      23-may-95 (tutto01)
**          Created.
**	03-jun-1996 (canor01)
**	    password structure must be passed in by caller.
**	01-may-1997 (toumi01)
**	    Provide appropriate getpwuid_r function based on platform
**	    for su4_us5, rs4_us5, and axp_osf.
**      10-sep-1997/28-apr-1997 (hweho01)
**          For Unixware (usl_us5), the reentrant and non-reentrant
**          versions of getpwuid have the same name.
**      02-oct-1997 (hweho01)
**          Provide appropriate getpwuid_r function for DG/UX.
**	19-May-1998 (fucch01)
**	    Use reentrant getpwuid_r() for sgi_us5...
**	25-MAR-2002 (xu$we01)
**	    Since dgi_us5 dose not use "__d6_getpwuid_r", remove
**	    "if defined(dgi_us5)" to resolve the compilation error.
**      08-Nov-2002 (hweho01)
**          Set return_pwd to NULL, if there is bad status code   
**          from getpwuid_r(). 
**	19-mar-2004 (somsa01)
**	    Itanium Linux uses getpwuid_r().
*/
 
struct passwd
*iiCLgetpwuid(uid_t uid, struct passwd *pwd, char *pwuid_buf, int size)
{

# if defined(OS_THREADS_USED) && !defined(NT_GENERIC)

# if defined(sparc_sol) || defined(a64_sol)
# define have_getpwuid_r
    return(getpwuid_r(uid, pwd, pwuid_buf, size));
# endif /* solaris */

# if defined(axp_osf) || defined(thr_hpux) || defined(any_aix) || \
     defined(i64_lnx) || defined(OSX)
# define have_getpwuid_r
    int status;
    struct passwd *return_pwd;
    status = getpwuid_r(uid, pwd, pwuid_buf, size, &return_pwd);
    if (status)
      {
        return_pwd = NULL;
	TRdisplay("iicl: getpwuid_r returned error %d\n", status);
      }
    return(return_pwd);
# endif /* axp_osf */

# if defined(usl_us5)
# define have_getpwuid_r
    return(getpwuid(uid));
# endif  /* usl_us5 */

# if defined(sgi_us5)
# define have_getpwuid_r    
    struct passwd *return_pwd;
    return( getpwuid_r(uid, pwd, pwuid_buf, size, &return_pwd) == 0 ? return_pwd : NULL );
# endif /* sgi_us5 */

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# ifndef have_getpwuid_r
    return(getpwuid(uid));

# endif /* ! have_getpwuid_r */
 
}
#endif /* !NT_GENERIC */


/*
** iiCLgethostbyname -- MT-safe call to gethostbyname.
**
** Description:
**      This wrapper function allows the caller to access the gethostbyname
**      function in a reentrant fashion.
**
** Input:
**
** Output:
**
** History:
**      03-jun-1996 (canor01)
**          Created.
**	01-may-1997 (toumi01)
**	    Provide appropriate gethostbyname_r function based on platform
**	    for su4_us5 and rs4_us5 (function is obsolete on axp_osf).
**      10-sep-1997/28-apr-1997 (popri01/hweho01)
**          For Unixware (usl_us5), the reentrant and non-reentrant
**          versions of gethostbyname have the same name.
**          Also, in order to handle the difference  
**          in returning buffers between su4_us5 and usl_us5,  
**          extended the fix that Bob Bonchik has written for    
**          DG_UX to UnixWare to reconstruct the returning data. 
**      26-Sep-1997 (bonro01)
**          Added check for dg8_us5 and dgi_us5 systems
**          which are compiled with OS_THREADS_USED but are missing
**          certain reentrant system routines.  This code puts a
**          wrapper around the function to serialize the execution
**          of the non-reentrant code and copy the return data to
**          thread local storage to prevent a race condition.
**          The return data is only partially reconstructed to
**          eliminate processing time for data that is not used.
**	19-May-1998 (fucch01)
**	    Add sgi_us5 to dg8_us5 and dgi_us5 platforms.  See above
**	    comment for details... 
**	19-Feb-1999 (loera01) Bug 95183
**	    For rs4_us5, evaluated the status return code of gethostbyname_r()
**	    before filling hostp with hostent.  For AIX, hostent will be 
**	    filled with the local host's address, if gethostbyname_r() fails.
**	    In this case, hostent must be nullified to agree with generic
**	    treatment by iiCLgethostbyname().
**	04-Dec-2001 (legru01)
**	    Added su9_us5 to use system function gethostbyname_r().
**	19-mar-2004 (somsa01)
**	    Itanium Linux has gethostbyname_r().
*/

struct hostent *
iiCLgethostbyname( char *name,
		   struct hostent *hostent,
		   char *buffer,
		   int size,
		   int *h_errnop )
		
{
    struct hostent *hostp;

# if defined(OS_THREADS_USED) && !defined(NT_GENERIC)

# if defined(sparc_sol) || defined(a64_sol)
# define have_gethostbyname_r
    MEfill (size, 0, buffer);
    MEfill (sizeof(struct hostent), 0, hostent);
    *h_errnop=0;
    hostp = gethostbyname_r( name, hostent, buffer, size, h_errnop );
# endif /* solaris */

# if defined(any_aix)
# define have_gethostbyname_r
    *h_errnop = gethostbyname_r( name, hostent, (struct hostent_data *)buffer );
    if (!*h_errnop)
    	hostp = hostent;
    else
	hostp = NULL;
# endif /* aix */

# if defined(i64_lnx)
    int status;
    status = gethostbyname_r(name, hostent, buffer, size, &hostp, h_errnop);
    if (status)
    {
        hostp = NULL;
	TRdisplay("iicl: gethostbyname_r returned error %d\n", status);
    }
# endif /* i64_lnx */

# if defined(usl_us5) || defined(sgi_us5) || defined(OSX)
{ 
# define have_gethostbyname_r
    struct s_hostent_data {
       struct hostent r_hostent;
       char *r_addr_list[2];
       char r_addr_entry[5];
    } *hostent_data;

#if defined(sgi_us5) || defined(OSX)

    gen_Psem(&CL_misc_sem);
#endif

    hostent_data = (struct s_hostent_data *)buffer;
    hostp = gethostbyname( name );

#if defined(sgi_us5) || defined(OSX)

    if ( h_errnop )
    *h_errnop = h_errno;
#endif

    /* Reconstruct the hostent data in the return buffer */
    if( hostp )
    {
	hostent_data->r_hostent = *hostp;
	hostent_data->r_hostent.h_name = NULL;
	hostent_data->r_hostent.h_aliases = NULL;
	MEcopy(hostp->h_addr, 5, hostent_data->r_addr_entry);
	hostent_data->r_addr_list[0] = &hostent_data->r_addr_entry[0];
	hostent_data->r_addr_list[1] = NULL;
	hostent_data->r_hostent.h_addr_list = &hostent_data->r_addr_list[0];
    }
#if defined(sgi_us5) || defined(OSX)

    gen_Vsem(&CL_misc_sem);
#endif

} 
# endif   /* usl_us5 */

# if defined(thr_hpux)
# define have_gethostbyname_r
    hostp = gethostbyname( name );
    if ( h_errnop )
	*h_errnop = h_errno;
# endif /* hpux */

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# ifndef have_gethostbyname_r

    hostp = gethostbyname( name );
    if ( h_errnop )
	*h_errnop = errno;

# endif /* ! have_gethostbyname_r */

    return ( hostp );
}


/*
** iiCLgethostbyaddr -- MT-safe call to gethostbyaddr.
**
** Description:
**      This wrapper function allows the caller to access the gethostbyaddr
**      function in a reentrant fashion.
**
** Input:
**
** Output:
**
** History:
**      03-jun-1996 (canor01)
**          Created.
**	01-may-1997 (toumi01)
**	    Provide appropriate gethostbyaddr_r function based on platform
**	    for su4_us5 and rs4_us5 (function is obsolete on axp_osf).
**      10-sep-1997/28-apr-1997 (popri01/hweho01)
**          For Unixware (usl_us5), the reentrant and non-reentrant
**          versions of gethostbyaddr have the same name.
**          Also, in order to handle the difference
**          in returning buffers between su4_us5 and usl_us5,
**          extended the fix that Bob Bonchik has written for 
**          DG_UX to UnixWare to reconstruct the returning data.
**      26-Sep-1997 (bonro01)
**          Added check for dg8_us5 and dgi_us5 systems
**          which are compiled with OS_THREADS_USED but are missing
**          certain reentrant system routines.  This code puts a
**          wrapper around the function to serialize the execution
**          of the non-reentrant code and copy the return data to
**          thread local storage to prevent a race condition.
**          The return data is only partially reconstructed to
**          eliminate processing time for data that is not used.
**	19-May-1998 (fucch01)
**	    Add sgi_us5 to dg8_us5 and dgi_us5 platforms.  See above
**	    comment for details... 
**	04-Dec-2001 (legru01)
**	    Added su9_us5 to define system function gethostbyaddr_r().
**      26-Aug-2002 (hweho01)
**          Added return code check for the routine used by AIX.  
**	 5-Mar-2002 (wanfr01)
**	    INGNET 95, Bug 107258
**	    On AIX, return a null hostname if gethostbyaddr_r returns
**	    unsuccessfully.   
**	19-mar-2004 (somsa01)
**	    Itanium Linux has gethostbyaddr_r().
*/

struct hostent *
iiCLgethostbyaddr( char *addr,
		   int len,
		   int type,
		   struct hostent *hostent,
		   char *buffer,
		   int size,
		   int *h_errnop )
		
{
    struct hostent *hostp;

# if defined(OS_THREADS_USED) && !defined(NT_GENERIC)

# if defined(sparc_sol) || defined(a64_sol)
# define have_gethostbyaddr_r
    hostp = gethostbyaddr_r( addr, len, type, hostent, buffer, size, h_errnop );
# endif /* solaris */

# if defined(any_aix)
# define have_gethostbyaddr_r
    struct in_addr *sock_addr_p;
    struct in_addr sock_addr;
    int real_len = sizeof(sock_addr);
    sock_addr.s_addr = *(in_addr_t *)addr;
    sock_addr_p = &sock_addr;
    *h_errnop = gethostbyaddr_r( (char *)sock_addr_p, real_len, type, hostent,
	(struct hostent_data*)buffer);
    if (*h_errnop) 
       hostp = NULL;
    else
       hostp = hostent;
# endif /* aix */

# if defined(i64_lnx)
    int status;
    status = gethostbyaddr_r(addr, len, type, hostent, buffer, size, &hostp,
			     h_errnop);
    if (status)
    {
        hostp = NULL;
	TRdisplay("iicl: gethostbyaddr_r returned error %d\n", status);
    }
# endif /* i64_lnx */

# if defined(usl_us5) || defined(sgi_us5) || defined(OSX)
{ 
# define have_gethostbyaddr_r
    struct s_hostent_data {
       struct hostent r_hostent;
       char *r_addr_list[2];
       char r_addr_entry[5];
    } *hostent_data;

#if defined(sgi_us5) || defined(OSX)
    gen_Psem(&CL_misc_sem);
#endif

    hostent_data = (struct s_hostent_data *)buffer;
    hostp = gethostbyaddr( addr, len, type );

#if defined(sgi_us5) || defined(OSX)
    if ( h_errnop )
    *h_errnop = h_errno;
#endif

    /* Reconstruct the hostent data in the return buffer */
    if ( hostp )
    {
	hostent_data->r_hostent = *hostp;
	hostent_data->r_hostent.h_name = NULL;
	hostent_data->r_hostent.h_aliases = NULL;
	MEcopy(hostp->h_addr, 5, hostent_data->r_addr_entry);
	hostent_data->r_addr_list[0] = &hostent_data->r_addr_entry[0];
	hostent_data->r_addr_list[1] = NULL;
	hostent_data->r_hostent.h_addr_list = &hostent_data->r_addr_list[0];
    }

#if defined(sgi_us5) || defined(OSX)
    gen_Vsem(&CL_misc_sem);
#endif

} 
# endif     /* usl_us5  */

# if defined(thr_hpux)
# define have_gethostbyaddr_r
    hostp = gethostbyaddr( addr, len, type );
    if ( h_errnop )
	*h_errnop = h_errno;
# endif /* hpux */

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# ifndef have_gethostbyaddr_r

    hostp = gethostbyaddr( addr, len, type );
    if ( h_errnop )
	*h_errnop = errno;

# endif /* ! have_gethostbyaddr_r */

    return ( hostp );
}



/*
** iiCLfcvt -- MT-safe call to fcvt.
**
** Description:
**      This wrapper function allows the caller to access the fcvt
**      function in a reentrant fashion.
**
** Input:
**
** Output:
**
** History:
**      25-may-95 (tutto01)
**          Created.
**	01-may-1997 (toumi01)
**	    Provide appropriate fcvt reentrant function based on platform
**	    for su4_us5, rs4_us5, and axp_osf.
**	24-jun-1997 (hweho01)
**	    Provide appropriate fcvt reentrant function for
**	    UnixWare (usl_us5).
**      26-Sep-1997 (bonro01)
**          Added check for dg8_us5 and dgi_us5 systems
**          which are compiled with OS_THREADS_USED but are missing
**          certain reentrant system routines.  This code puts a
**          wrapper around the function to serialize the execution
**          of the non-reentrant code and copy the return data to
**          thread local storage to prevent a race condition.
**          The return data is only partially reconstructed to
**          eliminate processing time for data that is not used.
**	19-May-1998 (fucch01)
**	    Use reentrant fcvt_r() for sgi_us5...
**	10-Sep-1999 (toumi01)
**	    For Linux (lnx_us5) include stdlib.h to provide fcvt prototype.
**	    Also, to avoid buggy fcvt, use fcvt_r instead.  This avoids
**	    segvs caused by malloc errors on corrupted heap with glibc 2.1.1;
**	    the bug is value-dependent, and one value that fails to convert
**	    is 723456.78901229997, which is generated by sep test abf51.
**	06-Oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	07-Dec-1999 (devjo01)
**	    Use reentrant __dg_fcvt_r() for dgi (b99713).
**	14-jun-2000 (hanje04)
**	    For OS/390 Linux (ibm_lnx) include stdlib.h to provide fcvt 
**	    prototype. Also, to avoid buggy fcvt, use fcvt_r instead.  This
**	    avoids segvs caused by malloc errors on corrupted heap with
**	    glibc 2.1.1; the bug is value-dependent, and one value that fails
**	    to convert is 723456.78901229997, which is generated by sep test
**	    abf51.
**      25-Jul-2000 (hanal04) Bug 101275 INGSRV 1155
**          Mutex protect the call to fcvt() for usl as it is not thread safe.
**	07-Sep-2000 (hanje04)
**	    Include stdlib for axp_lnx (Alpha Linux) to  use fcvt_r
**	07-Sep-2000 (hanje04)
**	    Include stdlib for i64_lnx (IA64 Linux) to  use fcvt_r
**	02-Nov-2007 (hanje04)
**	    Bug 114907
**	    To avoid a bug in fcvt() when ndigit = 0 on OSX, recode this case
**	    using snprintf(). Then manipulate the resulting string so the 
**	    value returned value is in the format expected by the caller.
*/
 
char
*iiCLfcvt(double value, int ndigit, int *decpt, int *sign, char *buf, int len)
{
 
# if defined(OS_THREADS_USED) && !defined(NT_GENERIC)

# if defined(sparc_sol) || defined(a64_sol)
# define have_fcvt_r
    return(fconvert(value, ndigit, decpt, sign, buf));
# endif /* solaris */

# if defined(any_aix) || defined(axp_osf)
# define have_fcvt_r
    int status;
    status = fcvt_r(value, ndigit, decpt, sign, buf, len);
    if (status)
	TRdisplay("iicl: fcvt_r conversion failed\n");
    return(buf);
#endif /* aix || axp_osf */

# if defined(thr_hpux)
# define have_fcvt_r
    return(fcvt(value, ndigit, decpt, sign));
#endif /* hpux */

# if defined(usl_us5)
# define have_fcvt_r
    {
    char *fcvtp;

    CS_synch_lock(&fcvt_mutex);
    fcvtp = fcvt(value, ndigit, decpt, sign);
    STcopy(fcvtp, buf);
    CS_synch_unlock(&fcvt_mutex);
    return (buf);
    }
# endif /* usl_us5  */

# if defined(sgi_us5)
# define have_fcvt_r
    return((char *)fcvt_r(value, ndigit, decpt, sign, buf));
# endif /* sgi_us5 */

# ifdef LNX
# define have_fcvt_r
    fcvt_r(value, ndigit, decpt, sign, buf, len);
    return(buf);
# endif /* Linux */

# ifdef OSX
# define have_fcvt_r
    {
    char *fcvtp;

    /*
    ** fcvt() bug in tiger means we don't get a value if ndigit = 0
    ** so use snprintf() for that case
    */
    if ( ndigit == 0 )
    {
	char	tmpbuf[50]; /* should be plenty */
	char	*tmpbufp = tmpbuf;

        gen_Psem(&CL_misc_sem);
	*decpt = snprintf( tmpbuf, len, "%.0f", value );
	/*
	** caller is expecting fcvtp() type returns, even if the value we've got
	** is -ve, mark it as not so we don't get two - signs added
	*/
	if ( *tmpbufp == '-' )
	{
	    tmpbufp++;
	    (*decpt)--;
	    *sign = 1;
	}
	else
	    *sign = 0;

	STcopy( tmpbufp, buf );
	gen_Vsem(&CL_misc_sem);
    }
    else
    {
        gen_Psem(&CL_misc_sem);
	fcvtp = (char *)fcvt(value, ndigit, decpt, sign);
	STcopy( fcvtp, buf );
	gen_Vsem(&CL_misc_sem);
    }

    return(buf);
    }
# endif

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# ifdef LNX
# define have_fcvt_r
    fcvt_r(value, ndigit, decpt, sign, buf, len);
    return(buf);
# endif /* Linux */

# ifndef have_fcvt_r
 
# if defined(sgi_us5)
    return((char *)(fcvt(value, ndigit, decpt, sign)));
# else
    return(fcvt(value, ndigit, decpt, sign));
# endif
 
# endif /* ! have_fcvt_r */
 
}



 
/*
** iiCLecvt -- MT-safe call to ecvt.
**
** Description:
**      This wrapper function allows the caller to access the ecvt
**      function in a reentrant fashion.
**
** Input:
**
** Output:
**
** History:
**      25-may-95 (tutto01)
**          Created.
**	01-may-1997 (toumi01)
**	    Provide appropriate ecvt reentrant function based on platform
**	    for su4_us5, rs4_us5, and axp_osf.
**      24-jun-1997 (hweho01)
**          Provide appropriate ecvt reentrant function for
**          UnixWare(usl_us5).
**      26-Sep-1997 (bonro01)
**          Added check for dg8_us5 and dgi_us5 systems
**          which are compiled with OS_THREADS_USED but are missing
**          certain reentrant system routines.  This code puts a
**          wrapper around the function to serialize the execution
**          of the non-reentrant code and copy the return data to
**          thread local storage to prevent a race condition.
**          The return data is only partially reconstructed to
**          eliminate processing time for data that is not used.
**	19-May-1998 (fucch01)
**	    Use reentrant ecvt_r() for sgi_us5...
**	10-Sep-1999 (toumi01)
**	    For Linux (lnx_us5) include stdlib.h to provide ecvt prototype.
**	    Also, to avoid buggy ecvt, use ecvt_r instead.  This avoids
**	    segvs caused by malloc errors on corrupted heap with glibc 2.1.1;
**	    the bug is value-dependent, and one value that fails to convert
**	    is 723456.78901229997, which is generated by sep test abf51.
**	06-Oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	07-Dec-1999 (devjo01)
**	    Use reentrant __dg_ecvt_r() for dgi (b99713).
**	14-jun-2000 (hanje04)
**	    For OS/390 Linux (ibm_lnx) include stdlib.h to provide fcvt 
**	    prototype. Also, to avoid buggy fcvt, use fcvt_r instead.  This
**	    avoids segvs caused by malloc errors on corrupted heap with
**	    glibc 2.1.1; the bug is value-dependent, and one value that fails
**	    to convert is 723456.78901229997, which is generated by sep test
**	    abf51.
**      18-Jan-2002 (bonro01)
**	    Another fix for Bug 101275. fcvt() & ecvt() are both used to
**	    generate floating point number output.
**          Mutex protect the call to ecvt() for usl as it is not thread safe.
**	19-mar-2004 (somsa01)
**	    Itanium Linux uses ecvt_r().
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/
 
char
*iiCLecvt(double value, int ndigit, int *decpt, int *sign, char *buf, int len)
{
 
# if defined(OS_THREADS_USED) && !defined(NT_GENERIC)

# if defined(sparc_sol) || defined(a64_sol)
# define have_ecvt_r
    return(econvert(value, ndigit, decpt, sign, buf));
# endif /* solaris */

# if defined(any_aix) || defined(axp_osf)
# define have_ecvt_r
    int status;
    status = ecvt_r(value, ndigit, decpt, sign, buf, len);
    if (status)
	TRdisplay("iicl: ecvt_r conversion failed\n");
    return(buf);
#endif /* osf, aix */

# if defined(thr_hpux)
# define have_ecvt_r
   return(ecvt(value, ndigit, decpt, sign));
# endif /* hpux  */  

# if defined(usl_us5)
# define have_ecvt_r
    {
    char *ecvtp;

    CS_synch_lock(&fcvt_mutex);
    ecvtp = ecvt(value, ndigit, decpt, sign);
    STcopy(ecvtp, buf);
    CS_synch_unlock(&fcvt_mutex);
    return (buf);
    }
# endif /* usl_us5  */

# if defined(sgi_us5)
# define have_ecvt_r
    return((char *)ecvt_r(value, ndigit, decpt, sign, buf));
# endif /* sgi_us5 */

# if defined(int_lnx) || defined(int_rpl) || defined(ibm_lnx) || \
     defined(axp_lnx) || defined(i64_lnx)
# define have_ecvt_r
    ecvt_r(value, ndigit, decpt, sign, buf, len);
    return(buf);
# endif /* int_lnx || ibm_lnx */

# ifdef OSX
# define have_ecvt_r
    {

    gen_Psem(&CL_misc_sem);
    buf = (char *)ecvt(value, ndigit, decpt, sign);
    gen_Vsem(&CL_misc_sem);

    return(buf);
    }
# endif

# endif /* OS_THREADS_USED && ! NT_GENERIC */

# if defined(int_lnx) || defined(int_rpl) || defined(ibm_lnx) | defined(axp_lnx)
# define have_ecvt_r
    ecvt_r(value, ndigit, decpt, sign, buf, len);
    return(buf);
# endif /* int_lnx || ibm_lnx  */

# ifndef have_ecvt_r
 
#if defined(sgi_us5)
    return((char *)(ecvt(value, ndigit, decpt, sign)));
# else
    return(ecvt(value, ndigit, decpt, sign));
# endif
 
# endif /* ! have_ecvt_r */
 
}

#ifndef NT_GENERIC
STATUS
iiCLttyname (i4 filedes, char* buf, i4 size)
{
    STATUS status = FAIL;
    char* retbuf = NULL;
# if defined(OS_THREADS_USED) && !defined(rux_us5)
# if defined(any_aix)|| defined(LNX) || defined(OSX)
     if ( ttyname_r (filedes, buf, size) == 0 )
#  else
    if ((retbuf = (char*)ttyname_r (filedes, buf, size)) != NULL) 
#  endif
    {
        status = OK;
    }
# else
    if ((retbuf = ttyname(filedes)) != NULL)
    {
        STlcopy (retbuf, buf, size);
        status = OK;
    }
# endif /* OS_THREADS_USED */
    return (status);
}
# endif /* NT_GENERIC */
