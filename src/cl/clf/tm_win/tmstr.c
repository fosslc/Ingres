/*
** Copyright (c) 1987, 2003 Ingres Corporation
*/

#if !defined(NT_IA64) && !defined(NT_AMD64)
#ifndef _USE_32BIT_TIME_T
#define _USE_32BIT_TIME_T
#endif /*_USE_32BIT_TIME_T */
#endif /* #if !defined(NT_IA64) && !defined(NT_AMD64)*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include	<st.h>			
# include       <tmtz.h>
/*
** Name: TMSTR.C - Time to string
**
** Description:
**      This file contains the following tm routines:
**    
**      TMstr()        -  Time to string
**
** History:
** Revision 1.2  88/10/17  15:29:29  jeff
** no newline at the end
** 
** Revision 1.1  88/08/05  13:46:56  roger
** UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
** 
**      Revision 1.2  87/11/10  16:04:19  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	15-feb-2001 (somsa01)
**	    Changed time_input from long to time_t for NT_IA64.
**	02-oct-2003 (somsa01)
**	    Added NT_AMD64 to last change.
*/


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
**	04-May-2009 (drivi01)
**	    In effort to port to Visual Studio 2008 compiler,
**	    updating TMstr to use "time_t time_input" for all
**	    platforms.
**	    In a new compiler gmtime evaluates to gmtime64 and
**	    time_t evaluates to time64_t to force it to evaluate to 
**	    gmtime32 and time32_t we have to define _USE_32BIT_TIME_T.
*/
VOID
TMstr(timeval,timestr)
SYSTIME	*timeval;
char	*timestr;
{
    struct tm *gmtime();
    char      *asctime();
    i4	      time_check;
    time_t    time_input;
    PTR       tm_cb;
    STATUS    status;


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
    
    STlcopy(asctime(gmtime( &time_input)), timestr, 24);
}

