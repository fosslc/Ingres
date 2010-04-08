/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# if defined(ris_us5) || defined(ts2_us5)
# include       <clconfig.h>
# include       <rusage.h>
# endif
# if !(defined(dr6_us5) || defined(sqs_ptx))
# include	<systypes.h>
# endif  /* dr6_us5) */
# if defined(ris_us5) || defined(sos_us5)
# include	<time.h>
# endif /* ris_us5 sos_us5 */
# include	<st.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include       <tmtz.h>
# if defined(dr6_us5) || defined( dgi_us5 ) || defined(dg8_us5) || \
     defined(sqs_ptx) || defined(int_lnx) || defined(ibm_lnx) || \
     defined(int_rpl)
# include       <time.h>
# endif  /* dr6_us5 */

/**
** Name: TMTZSTR.C - Timezone Local Time to string
**
** Description:
**      This file contains the following tm routines:
**    
**      TMtz_str()        -  Time to string
**
** History:
**	15-aug-1997 (canor01)
**	    Created from tmstr.c.
**	27-aug-1997 (canor01)
**	    Added st.h.
**	06-nov-1997 (walro03)
**	    For ICL (dr6_us5), added time.h following tmtz.h, and removed
**	    systypes.h.
**	30-sep-1997 (walro03/mosjo01)
**	    Added time.h because of compile errors on ris_us5 and sos_us5.
**	2-dec-1997 (popri01)
**	    Remove function prototype for asctime_r for Unixware (usl_us5).
**	    (note: function prototype was undef'ed for all platforms in tmstr.c)
**      26-Feb-1998 (bonro01)
**          Added time.h for dg8_us5 and dgi_us5.
**	05-mar-1998 (musro02)
**	    Added sqs_ptx to platforms needing time.h following tmtz.h
**	    and not needing systypes.h
**	10-jul-1998 (ocoro02)
**	    Addition of clconfig.h and rusage.h on sos_us5 broke build.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	29-feb-2000 (toumi01)
**	    Added int_lnx to platforms needing time.h.
**	15-aug-2000 (somsa01)
**	    Added ibm_lnx to platforms needing time.h.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**/


/*{
** Name: TMtz_str	- Time to string
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
**	tz_cb				pointer to TM_TZ_CB
**
** Outputs:
**      timestr                         character representation of timeval
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-1997 (canor01)
**	    Created from tmstr.c.
**	01-oct-1997 (mosjo01)
**	    Added clconfig.h and rusage.h because of compile errors on 
**	    ris_us5, sos_us5 and ts2_us5.
**	2-dec-1997 (popri01)
**	    Remove function prototype for asctime_r for Unixware (usl_us5).
*/

VOID
TMtz_str(SYSTIME *timeval, PTR tz_cb, char *timestr)
{
    i4	      time_check;
    long      time_input;
    PTR       tm_cb;
    STATUS    status;
    struct tm gmtime_res;
    char      asctime_res[26];

#if defined(usl_us5)
	char* asctime_r();
#endif

    time_input = (long)timeval->TM_secs;
    if( time_input > MAXI4)
	time_check = MAXI4;
    else if( time_input < MINI4)
	time_check = MINI4;
    else
	time_check = (i4) time_input;

    status = TMtz_init(&tm_cb);
    if ( tz_cb != NULL )
	tm_cb = tz_cb;
    if( status == OK)
	time_input += TMtz_search(tm_cb, TM_TIMETYPE_GMT, time_check);
    TM_copy_gmtime(&time_input, &gmtime_res );
    TM_copy_asctime(&gmtime_res, asctime_res, 26);
    STncpy(timestr, asctime_res, 24); 
    timestr[24]='\0';
}

