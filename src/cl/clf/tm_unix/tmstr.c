/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<st.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include       <tmtz.h>
/**
** Name: TMSTR.C - Time to string
**
** Description:
**      This file contains the following tm routines:
**    
**      TMstr()        -  Time to string
**
** History:
 * Revision 1.2  88/10/17  15:29:29  jeff
 * no newline at the end
 * 
 * Revision 1.1  88/08/05  13:46:56  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  16:04:19  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
**/


/*{
** Name: TMstr	- Time to string
**
** Description:
**	This routine sets "timestr" to point to a character string
**	giving the 1) Day of the week; 2) Month;
**	3) Date; 4) Hour, minutes, seconds and 5) year of "timeval"
**	in an ASCII format.
**
**	Note:
**	    Every time this routine is called the previous data
**	    is lost since the buffer is overwritten.
**
**	    I blindly assume that timeval is non-negative and
**	    that timestr is a good pointer.
**
** Inputs:
**      timeval                         time value to convert to string
**
** Outputs:
**      timestr                         character representation of timeval
**	Returns:
**	    STATUS of operation
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	06-jul-87 (mmm)
**	    initial jupiter unix cl.
**      22-sep-92 (stevet)
**          Remove the use of ctime and call TMtz routines to
**          calculate timezone offset, which is consistence with
**          other timezone offset calculations in other part of
**          Ingres product.  
**	23-may-95 (tutto01)
**	    Added gmtime_r and asctime_r calls for reentrancy.
**	03-jun-1996 (canor01)
**	    Changed dependency for MT-safe gmtime_r() and actime_r()
**	    functions to generic operating-system threads.
**  30-Apr-1997 (merja01)
**      Removed definition of gmtime_r and asctime_r on axp_osf to prevent
**      compile errors from incomplete and unnecessary prototype.
**	02-sep-1997 (muhpa01)
**	    Add hp8_us5 to platforms removing prototypes for gmtime_r and
**	    asctime_r
**	23-Sep-1997 (kosma01)
**	    Use xCL_TIME_CNVRT_TIME_AND_BUFFER to select the 
**	    correct set of function arguments. asctime_r on AIX 4
**	    accepts only to arguments, a time ptr and a buf ptr.
**	    No buffer length argument is passed.
**  10-Feb-1998 (bonro01)
**      The re-entrant function asctime_r causes a SEGV under DGUX so
**      use the non-reentrant version with semaphores.
**	06-Mar-1997 (kosma01)
**	    Remove xCL_TIME_CNVRT_TIME_AND_BUFFER. Moved change to
**	    correct # of arguments passed to AIX's asctime_r to
**	    the transparent wrappers defined in tmcl.h.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      19-jul-1999 (hweho01)
**          Fixed the error result from gmtime_r(), changed time_input 
**          from long to time_t (4-byte) for ris_u64. 
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**          Correct rux_us5 changes that were applied using the rmx_us5
**          label.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	15-Oct-2002 (bonro01) SGI 64-bit build requires a 4 byte field for
**	    the time value.  This should be the same for all 64-bit platforms.
**		
*/
VOID
TMstr(timeval,timestr)
SYSTIME	*timeval;
char	*timestr;
{
    struct tm *gmtime();
    char      *asctime();
    i4	      time_check;
#if defined(LP64)
    time_t    time_input;
#else
    long      time_input;
#endif 
    PTR       tm_cb;
    STATUS    status;

# ifdef OS_THREADS_USED       
    struct tm gmtime_res;
    char      asctime_res[26];
#if !defined(axp_osf) && !defined(any_hpux)
	struct tm *gmtime_r();
	char *asctime_r();
#endif /* axp_osf && hpux */
# endif /* OS_THREADS_USED */



    time_input = (long)timeval->TM_secs;
    if( time_input > MAXI4)
	time_check = MAXI4;
    else if( time_input < MINI4)
	time_check = MINI4;
    else
	time_check = (i4) time_input;

    status = TMtz_init(&tm_cb);
    if( status == OK)
	time_input += TMtz_search(tm_cb, TM_TIMETYPE_GMT, time_check);
# ifdef OS_THREADS_USED
    TM_copy_gmtime( &time_input, &gmtime_res );
    TM_copy_asctime( &gmtime_res, asctime_res, 26 );
    STncpy( timestr, asctime_res, 24); 
    timestr[ 24 ] = EOS;
# else
    STncpy( timestr, asctime(gmtime( &time_input)), 24);
    timestr[ 24 ] = EOS;
# endif /* OS_THREADS_USED */
}

