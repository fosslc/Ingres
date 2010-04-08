/*
 * alarm_handler.c
 *
 * alarm_handler() is called when the interval timer goes off. It checks
 * each of the test types to see if any are due to receive interrupt or
 * kill signals. If needed, it selects test instances to signal.
*/

/*
** History:
**      05-July-91 (DonJ)
**              Change quotes to brackets on include of achilles.h.
**      29-apr-92 (purusho)
**              Added amd_us5 specific changes
**	18-Aug-92 (GordonW)
**		Add tag_child() which will test if the KILL/INT actually
**		worked or not and log the approperiate msg.
**	11-Nov-92 (GordonW)
**		Disable alarm timer if not using INT/KILL functions.
**       6-19-93 (Jeffr)
** 		Added fix to disable Alarm so were not interrrupted SVR 3 based
**		systems must block Alarm as they will be killed if it goes off
**		and it hasnt been blocked
**	 7-19-93 (Jeffr)
**		Added new code to support (-a option) - running for set period
**		of time
**	 8-7-93 (Jeffr)
**		Fixed file close logic bug - compares to stdout now
**	 8-13-93 (jeffr)
**	       removed total count of children from this file	
**	 28-oct-93 (dianeh)
**	       Added sys/ to #include of stat.h so yam can find it properly.
**	 28-oct-93 (dianeh)
**	       Added machine/ to #include of a.out.h so it can be found properly
**	 11-nov-93 (dianeh)
**		Back out previous change -- it wasn't right, but as it turns
**		out, a.out.h isn't needed, so take it out altogether.
**
**       10-jan-94 (jeffr)
**		always enable interupt handler so that -a flag works in
**		acreport
**	20-Dec-94 (GordonW)
**		Adding Solaris.
**	13-Feb-1995 (canor01)
**		Adding rs4_us5 (AIX 4.1).
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Adding axp_osf.
**	27-feb-96 (hopgi01)
**		Added support for sui_us5 following su4_us5
**  23-Jun-97 (merja01)
**      Fix incompatable function prototype error on axp for LOtos
**      10-sep-97/09-apr-97 (hweho01) 
**              Cross-integrated UnixWare 2.1 fix from OI 1.2/01: 
**              Added support for usl_us5.
**	23-jun-1997 (walro03)
**		Added support for ICL DRS 6000 (dr6_us5).
**	29-jul-1997 (walro03)
**		Added pym_us5 (pyramid) and rmx_us5 (siemens).
**	12-aug-1997 (fucch01)
**		Added support for Tandem Nonstop (ts2_us5).
**	29-aug-1997 (mosjo01)
**		Added support for sos_us5.
**      29-aug-97 (musro02)
**              For sqs_ptx, don't define sigmask
**	23-Sep-1997 (kosma01)
**	        Following CL_PROTOTYPED becoming a default define
**	        in cl/hdr/hdr/compat.h, the AIX compiler cracked 
**	        down on using a LOCATION type instead of a LOCATION *
**	        type for a call to LOtos.
**	02-jul-1998 (kosma01 for fucch01)
**		Added support for Silicon Graphics (sgi_us5).
**	29-sep-1998 (matbe01)
**		Removed include for signal.h for NCR (nc4_us5).
**	03-may-1999 (toumi01)
**		Added support for Linux (lnx_us5).
**	10-may-1999 (walro03)
**	    Remove obsolete version string amd_us5, sqs_us5.
**      03-jul-1999 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**      22-Nov-1999 (hweho01)
**              Added support for ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	15-Jun-2000 (hanje04)
**		Added support for OS/390 Linux (ibm_lnx).
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Sep-2000 (hanje04)
**              Added support for axp_lnx (Alpha Linux).
**	15-May-2002 (hanje04)
**	    Added support for Itanium Linux (i64_lnx)
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	15-Mar-2005 (bonro01)
**	    Add support for Solaris AMD64 a64_sol
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	11-Apr-2007 (hanje04)
**	    SIR 117985
**	    Fix compile problems against GLIBC 2.5 for PowerPC Linux port.
**	30-Apr-2007 (hanje04)
**	    SIR 117985
**	    Use xCL_NEED_RPCSVC_REX_HDR to conditionally include rcpsrv/rex.h
**	    instead of GLIBC version as it's not consistent across Linuxes.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	28-Jun-2009 (kschendel) SIR 122138
**	    Use sparc-sol, any-hpux, any-aix symbols where needed.
**/


#include <achilles.h>

#ifdef	UNIX
#include <clconfig.h>
#endif
#include <pwd.h>

#ifdef dr6_us5
#include <sys/ttold.h>
#include <sys/termio.h>
#endif /* dr6_us5 */

#ifndef	any_hpux
#include <sys/file.h>
#else
#include <sys/fcntl.h>
#endif

#if defined(dg8_us5) || defined(dgi_us5)
#define _BSD_TTY_FLAVOR
#include <ioctl.h>
#endif

#ifdef any_hpux
#include <sgtty.h> 
#include  <termio.h>
#include "missing.h"
#include "a800.h"
static struct termio tbuf;
#endif

#ifdef sqs_ptx
#include <sys/wait.h>
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include <sys/signal.h>
#include "missing.h"
#include "wait3.h"
#endif

#if defined(usl_us5)
#include <sys/ttold.h>
#include <sys/termios.h>
#endif

#ifdef hp3_us5
#define _INCLUDE_POSIX_SOURCE 
#include <sgtty.h> 
#include  <termio.h>
#include "missing.h"
static struct termio tbuf;
#endif

#if defined(pym_us5) || defined(rmx_us5) || defined(rux_us5)
#include "/usr/ucbinclude/sys/wait.h"
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include "/usr/ucbinclude/sys/signal.h"
#endif

#ifdef sun_u42      /* plus sun3 */
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#endif

#ifdef su4_u42      /* plus sun4 */
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#endif

#if defined(sparc_sol) || defined(sui_us5)	/* plus solaris */
#include <sgtty.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <sys/ttold.h>
#endif

#ifdef ds3_ulx      /* plus ultrix  */
#include <sys/ioctl.h>
#endif

#if defined(any_aix)
#include <xcoff.h>
#include <sys/ioctl.h>
#endif

# ifdef nc4_us5 
#include "/usr/ucbinclude/sys/wait.h"
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include <unistd.h>
#endif

#include <sys/wait.h>
#if defined(any_aix)
#include <sys/m_wait.h>
#endif


#include <sys/time.h>

#ifdef pyr_u42
#include <sys/sgtty.h>
#include <sys/ioctl.h>
#endif

#ifdef ts2_us5
#include <sgtty.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include "missing.h"
#endif

#ifdef axp_osf
#include <sys/ioctl.h>
#endif

#ifdef sos_us5
#include <sys/ttold.h>
#include <sys/termios.h>
#include "missing.h"
#endif

#ifdef sgi_us5
#include <sys/ttold.h>
#include <sgtty.h>
#endif

#ifdef LNX
#include <termio.h>
#  ifdef xCL_NEED_RPCSVC_REX_HDR
#    include <rpcsvc/rex.h>
#  endif
static struct termio tbuf;
#endif

#ifdef a64_sol
#include <sys/ttold.h>
#include <sgtty.h>
#endif

#ifdef OSX      /* Mac OS X */
#include <sys/types.h>
#include <termios.h>
#include <sys/tty.h>
#include <sgtty.h> 
static struct termios tbuf;
#endif

extern i4  active_children;

GLOBALREF HI_RES_TIME achilles_start;
extern i4        time_to_run;
extern VOID ACchildHandler ();
extern i4  	time_set;
#include <sys/stat.h>

alarm_handler ()
{
	register i4  i,
		     j,
		     target,
		     sigcount,
		     count,
		     avail,
		     int_target,
		     Using_sigs,
		     kill_target;
	HI_RES_TIME  now;
	STATUS	    status;
	ACTIVETEST  *testptr;
	ACTIVETEST  **availtest;     /* will be an array of pointers to tests */
	extern PID     achpid;
	char          *fname;
	struct tchars  tc;
        struct sgttyb  sg;

        ACTIVETEST    *curtest;
        struct stat    st;
        char           timebuf [80];

        ACblock(CHILD | STATRPT | ABORT | ALARM );

/** we always want to be Using_sigs for Alarm Handler **/
	Using_sigs = 1;
	ACgetTime(&now, (LO_RES_TIME *) NULL);
	for (i = 0 ; i < numtests ; i++)
	{
	    int_target = -1;

/** see if we want to exit achilles first - (we exceeded time limit) **/

            if ((time_set) &&
                (ACcompareTime(&now,&achilles_start)/3600 >= time_to_run) )
            {
                        for (j = 0 ; j < tests[i]->t_nkids ; j++)
			{
			  curtest=&tests[i]->t_children[j];
			  LOtos(&tests[i]->t_children[j].a_outfloc, &fname);
			  if ( (!*tests[i]->t_outfile)
                          && (!stat(fname, &st) )
                          && (st.st_size == 0) )
                                unlink(fname);
                        else
                                ACchown(&curtest->a_outfloc, uid);
				
/*		SIfprintf(stderr,"crashes in alarmit\n"); */
			/* Record the end time of the last process in a_start,
                           for use by print_status.
                        */
			/** kill all children in the group **/
                    ACkillChild(&tests[i]->t_children[j]);
		    
                    active_children--;
		/** keep running total of child times for each instance **/
       	     tests[i]->t_ctime+= ACcompareTime(&now,&curtest->a_start)/3600.0;
                        curtest->a_start = now.time;
			curtest = &(tests[i]->t_children[j]);
                /* Log the dead process. */
			log_entry(&now, i, j, curtest->a_iters,

           tests[i]->t_children[j].a_pid, 
                       C_EXIT,curtest->a_status, &curtest->a_start); 
/**		fprintf(stderr,"active children = %d\n",active_children); **/
			 }

		   if (active_children == 0) 
		    {
/*fprintf(stderr,"\n active children = %d Achilles now Exiting !! \n",active_children); */
/* time and date stamp the conclusion of the achilles test run
               11/16/89, bls */
/** generate report **/
            ACreport();

            ACgetTime (&now, (LO_RES_TIME *) NULL);
            ACtimeStr (&now, timebuf);
            SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n",
                       timebuf, "ACHILLES END", achpid, "END");
            SIflush(logfile);
	       if (logfile != stdout) fclose(logfile);

#if defined(hp3_us5) || defined(any_hpux) || defined(LNX)
		if (ioctl(fileno(stdout), TCGETA, &tbuf) != -1)
                {
                        tbuf.c_cc[1] = orig_quit; 
                        ioctl(fileno(stdout), TCSETA, &tbuf);
                }
	
		if (ioctl(fileno(stdout), TCGETA, &tbuf) != -1)
                {
                        tbuf.c_lflag |= ECHO;
                        ioctl(fileno(stdout), TCSETAF, &tbuf);
                }
#else
#if defined(sqs_ptx) 
		if (_get_term(fileno(stdout), &tc) != -1)
		{
			tc.t_quitc = orig_quit;
			_set_term(fileno(stdout), &tc, 0);
		}
		termop(fileno(stdout), _ECHO | _DRAIN);
#else

/*		fprintf(stderr,"doing sun term reset in alarm\n");  */
		if (ioctl(fileno(stdout), TIOCGETC, &tc) != -1)
		{
			tc.t_quitc = orig_quit;
			ioctl(fileno(stdout), TIOCSETC, &tc);
		}

		if (ioctl(fileno(stdout), TIOCGETP, &sg) != -1)
		{
			sg.sg_flags |= ECHO;
			ioctl(fileno(stdout), TIOCSETN, &sg);
		}
#endif /* sqs_ptx */
#endif
    		PCexit(OK);
                signal ( SIGCHLD,ACchildHandler );
                /* Reinitialize handler */

	  }
  
        }

	else {
/*	    if( ACintEnabled(i) || ACkillEnabled(i) )
		Using_sigs++;
*/

	    if ( (ACintEnabled(i) )
	      && (ACcompareTime(&tests[i]->t_inttime, &now) <= 0) )
	    {

		/* Randomly pick the starting point of a cluster-sized
		   group of child processes. Send each a signal, unless
		   it's already completed all its iterations.
		*/

		/* allocate memory for the array of test instance pointers.
           Each element in the array points to a test instance available
           to receive interrupt or kill signals.
           11-3-89, bls
        */

		    availtest = (ACTIVETEST **) MEreqmem(0, tests[i]->t_nkids * 
                         sizeof (ACTIVETEST *), TRUE, &status);

#ifdef DEBUG

			fprintf (stderr, "availtest = %X (hex), *availtest = %X (hex)\n",
                     availtest, *availtest);

#endif
		    if (status != OK)
		    {
		    	SIprintf("Can't MEreqmem space for available test array\n");
		    	PCexit(FAIL);
		    }

		/* determine the number of test instances available to 
           receive interrupts.  A test instance is available if
           it has not completed all of its iterations.
           11-3-89, bls
        */

            for (j = 0, avail = 0; j < tests[i]->t_nkids; j++)
			{
				/* for each available test instance, store a pointer to 
				   it in the availtest array and increment the count
				   11-3-89, bls
				*/

#ifdef DEBUG

				fprintf (stderr, "i = %d j = %d iters = %d tot iters = %d\n",
                         i, j, tests[i]->t_children[j].a_iters,
                         tests[i]->t_niters);
#endif

				if (tests[i]->t_children[j].a_iters <= tests[i]->t_niters)
				{
					 availtest[avail] = &(tests[i]->t_children[j]);
					 avail++;
				}
			}

				/* determine how many signals to send.  The number of
				   signals to send is the lesser of the available test
				   instances or the interrupt cluster size.
				   11-3-89, bls
				*/

			sigcount = avail < tests[i]->t_intgrp ? avail : tests[i]->t_intgrp;

#ifdef DEBUG

		    fprintf (stderr, "avail = %d intgrp = %d sigcount = %d\n", 
                     avail, tests[i]->t_intgrp, sigcount);

#endif
				/* if the interrupt cluster size is larger than the number
				   of available test instances, all of the available 
				   instances get sent a signal.
				   11-3-89, bls
				*/

			if (avail <= tests[i]->t_intgrp)
			{
				for (j = 0 ; j < sigcount; j++)
				{
					testptr = availtest[j];     

#ifdef DEBUG

				    fprintf (stderr, "j = %d testptr = %X (hex)\n", j, testptr);

#endif
					tag_child (testptr, i, C_INT);
				}
			}

				/* the number of test instances available exceeds the interrupt
				   cluster size.  Choose randomly among all available instances.
				   Mark each test as unavailable after it is selected.
				   11-3-89, bls
				*/

			else
			{
				count = 0;
				while (count < sigcount)
				{
					target = ACrandom() % avail;
					if (availtest[target] != NULL)
					{
						 testptr = availtest[target];
#ifdef DEBUG

				         fprintf (stderr, "target = %d testptr = %X (hex)\n", 
                                  target, testptr);
#endif
						 tag_child (testptr, i, C_INT);
						 count++;
						 availtest[target] = NULL;
					}
				}
			}
		                                 /* Update time to next interrupt.   */

		    ACaddTime(&tests[i]->t_inttime, &now, &tests[i]->t_intint);

		                                 /* Free the memory used for the 
		                                    available test array        
		                                    11-3-89, bls               
		                                 */ 
            MEfree (availtest);
		}

				 /* 
				    now do the same for kill signals -- NOTE:  test instances 
                    that just received interrupts are eligible; this is a 
                    change from previous logic which made them ineligible 
                    11-3-89, bls
                 */

	    if ( (ACkillEnabled(i) )
	      && (ACcompareTime(&tests[i]->t_killtime, &now) <= 0))
	    {

			/* allocate memory for the array of test instance pointers.
			   Each element in the array points to a test instance available
			   to receive interrupt or kill signals.
			   11-3-89, bls
			*/

		    availtest = (ACTIVETEST **) MEreqmem(0, tests[i]->t_nkids * 
                         sizeof (ACTIVETEST *), TRUE, &status);

		    if (status != OK)
		    {
		    	SIprintf("Can't MEreqmem space for available test array\n");
		    	PCexit(FAIL);
		    }

			/* determine the number of test instances available to 
			   receive kill signals.  A test instance is available if
			   it has not completed all of its iterations.
			   11-3-89, bls
			*/

            for (j = 0, avail = 0; j < tests[i]->t_nkids; j++)
			{
				/* for each available test instance, store a pointer to 
				   it in the availtest array and increment the count
				   11-3-89, bls
				*/

				if (tests[i]->t_children[j].a_iters <= tests[i]->t_niters)
				{
					 availtest[avail] = &(tests[i]->t_children[j]);
					 avail++;
				}
			}

				/* determine how many signals to send.  The number of
				   signals to send is the lesser of the available test
				   instances or the kill cluster size.
				   11-3-89, bls
				*/

			sigcount = avail < tests[i]->t_killgrp ? 
                       avail : tests[i]->t_killgrp;

#ifdef DEBUG

		    fprintf (stderr, "avail = %d killgrp = %d sigcount = %d\n", 
                     avail, tests[i]->t_killgrp, sigcount);

#endif
				/* if the kill cluster size is larger than the number
				   of available test instances, all of the available 
				   instances get sent a signal.
				   11-3-89, bls
				*/

			if (avail <= tests[i]->t_killgrp)
			{
				for (j = 0 ; j < sigcount; j++)
				{
					testptr = availtest[j];  
#ifdef DEBUG

				    fprintf (stderr, 
                            "kill j = %d testptr = %X (hex)\n", j, testptr);
#endif
					tag_child(testptr, i, C_KILL);
				}
			}

				/* the number of test instances available exceeds the kill
				   cluster size.  Choose randomly among all available instances.
				   Mark each test as unavailable after it is selected.
				   11-3-89, bls
				*/

			else
			{
				count = 0;
				while (count < sigcount)
				{
					target = ACrandom() % avail;
					if (availtest[target] != NULL)
					{
						 testptr = availtest[target];
#ifdef DEBUG

				         fprintf (stderr, 
                                 "kill target = %d testptr = %X (hex)\n", 
                                  target, testptr);
#endif
						 tag_child (testptr, i, C_KILL);
						 count++;
						 availtest[target] = NULL;
					}
				}
			}
		                                 /* Update time to next kill signal. */

		    ACaddTime(&tests[i]->t_killtime, &now, &tests[i]->t_killint);

		                                 /* Free the memory used for the 
		                                    available test array        
		                                    11-3-89, bls               
		                                 */ 
            MEfree (availtest);
		}
	}

    }
	ACresetAlarm();
	if( !Using_sigs )
		ACinitAlarm(0);		/* Disables Alarms */
	else
		ACinitAlarm(-1);	/* Resets Alarms */
	ACunblock();
	return;

} /* alarm_handler */

tag_child(testptr, i, tag)
ACTIVETEST  *testptr;
int	i;
int	tag;
{
	int	status;
	HI_RES_TIME  now;

	ACgetTime(&now, (LO_RES_TIME *) NULL);
	if( tag == C_INT )
	{
		if (ACintChild (testptr) < 0)
			tag = C_NO_INT;
	}
	else if ( tag = C_KILL )
	{
		if (ACkillChild (testptr) < 0)
			tag = C_NO_KILL;
	}
	else
		status = C_NOT_FOUND;
		
	log_entry (&now, i, atoi(testptr->a_childnum),
	   testptr->a_iters, testptr->a_pid, tag, 0, 0);
}
