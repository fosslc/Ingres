/**
 ** actools.c
 **
 ** Miscellaneous routines that are shared by VMS and Unix to  handle mostly
 ** time issues
 **
 ** History:
 **     5-Aug-93  (jeffr)
 **       Added ACreport to do final report generation for Achilles Version 2
 **     20-Aug-93 (jeffr)
 **      Fixed formatting problems in ACreport (i.e. extended field length 
 **	 and make case to handle "/bin/sh")
 **     28-oct-93 (dianeh)
 **       Added sys/ to #include of stat.h so yam can find it properly.
 **     28-oct-93 (dianeh)
 **       Added machine/ to #include of a.out.h so it can be found properly.
 **     11-nov-93 (dianeh)
 **	  Back out previous change -- it wasn't right, but as it turns
 **	  out, a.out.h isn't needed, so take it out altogether.
 **    10-jan-94 (ricka)
 **       it's <stat.h>, not <sys/stat.h> on VMS, and only need one achilles.h
 **	20-Dec-94 (GordonW)
 **	  Solaris....
 **	27-Dec-94 (GordonW)
 **	   added cast of (long) to ACtime.
 ** 12-Jan-96 (bocpa01)
 **		added support for NT (NT_GENERIC)
 **	13-Mar-97 (toumi01)
 **	   Moved # for #ifdef to column 1 as required by many compilers.
 **	01-Aug-2000 (bonro01)
 **	   Include <string.h> to include prototypes for strcat, strstr,
 **	   and strrchr.  ACreport() abended on axp_osf without the 
 **	   prototypes, because the function parms were passed incorrectly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Sep-2005 (hanje04)
**	    Quiet compiler warnings on Mac.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
 **/

#include <achilles.h>

#ifdef VMS
#include <descrip.h>
#include <iodef.h>
#include <lnmdef.h>
#include <ssdef.h>
#include <rmsdef>
#include <stat.h>
#endif
#ifdef UNIX
#include <pwd.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#if defined(su4_us5)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif /* su4_us5 */
#endif
#ifdef NT_GENERIC
#include <time.h>
#endif /* #ifdef NT_GENERIC */

VOID ACaddTime (dest, part1, part2)
HI_RES_TIME *dest, *part1;
LO_RES_TIME *part2;
{
        dest->time = part1->time + *part2;
        dest->millitm = part1->millitm;
} /* ACaddTime */

i4  ACcompareTime (time1, time2)
HI_RES_TIME *time1, *time2;
{
        if (time1->time != time2->time)
                return(time1->time - time2->time);
        else
                return( (i4) (time1->millitm - time2->millitm) );
} /* ACcompareTime */

char *ACgetRuntime (tv, starttime)
HI_RES_TIME *tv;
LO_RES_TIME *starttime;
{
        static char result[8];

        STprintf(result, "%3d:%02d", (tv->time - *starttime) / 60,
          (tv->time - *starttime) % 60);
        return(result);
} /* AcgetRuntime */

VOID ACgetTime (hires, lores)
HI_RES_TIME *hires;
LO_RES_TIME *lores;
{
	long junk;

        if (hires)
#if defined(su4_us5) || defined(OSX)
                ACftime(&hires->time);
#else
		ACftime(hires);
#endif
	junk = (long) ACtime(0);
        if (lores)
                *lores = (hires ? hires->time : junk );
} /* ACgetTime */

VOID ACinitTimes (testnum)
i4  testnum;
{
        HI_RES_TIME now;
 
        ACgetTime(&now, (LO_RES_TIME *) NULL);
        if (tests[testnum]->t_intint > 0)
                tests[testnum]->t_inttime.time =
                  now.time + tests[testnum]->t_intint;
        else
                tests[testnum]->t_inttime.time = -1;
        tests[testnum]->t_inttime.millitm = now.millitm;
        if (tests[testnum]->t_killint > 0)
                tests[testnum]->t_killtime.time =
                  now.time + tests[testnum]->t_killint;
        else
                tests[testnum]->t_killtime.time = -1;
        tests[testnum]->t_killtime.millitm = now.millitm;
} /* ACinitTimes */

VOID ACtimeStr (tv, ct)
HI_RES_TIME *tv;
char        *ct;
{
        STprintf(ct, "%.15s.%02d", ctime(&tv->time)+4, tv->millitm/10);
        return;
} /* ACtimeStr */

VOID ACwriteLogEntry (tv, testnum, childnum, iternum, pid, action, code,
                      starttime)
HI_RES_TIME *tv;
i4           testnum,
             childnum,
             iternum,
             pid,
             action,
             code;
LO_RES_TIME *starttime;
{
        LOGBLOCK log;
        i4 history;
 
        log.l_sec = tv->time;
        log.l_usec = (char) ( (tv->millitm / 10) & 255);
        log.l_test = (u_i1) ( (testnum + 1) & 255);
        log.l_child = (u_i2) ( (childnum + 1) & 65535);
        log.l_iter = (u_i2) (iternum & 65535);
        log.l_pid = (u_i2) (pid & 65535);
        log.l_action = (u_i1) (action & 255);
        log.l_code = (u_i1) (code & 255);
        log.l_runtime = (u_i2) (starttime ? tv->time - *starttime : 0);        
        history = 1;
        SIwrite(sizeof(log), (char *) &log, &history, logfile);
} /* ACwriteLogEntry */


/********************************** ACreport() ******************************
**
** description - prints out report for user at the end of the run
** 
** 23-July-93 (Jeffr)
**             Initial Creation for Achilles Version 2 
** 
** 10-Jan-94 (jeffr)
**		added Total Children feature for Report Enhancement
**
**
*******************************************************************************/
GLOBALREF char tembuf[256];
GLOBALREF char *outdir;
GLOBALREF char logfspec[MAX_LOC];
GLOBALREF int achpid; 
GLOBALREF HI_RES_TIME achilles_start; 
GLOBALREF int interflg;
GLOBALREF int tot_children;

VOID ACreport()

{
register int i
#ifndef NT_GENERIC
		, j, tests_up
#endif /* END #ifndef NT_GENERIC */
		;
        char ct[TIME_STR_SIZE];
        HI_RES_TIME now
#ifndef NT_GENERIC
					, xtime
#endif /* END #ifndef NT_GENERIC */
					;
#ifndef NT_GENERIC
        ACTIVETEST *curtest;
#endif /* END #ifndef NT_GENERIC */
        int val=0;
	i4 tot_tests=0;
	i4 tot_bad=0;
/** changed it from 0 to 0.0F to avoid VCPP compiler warning - bocpa01 **/
	float tot_time=0.0F;
        char	rlogfname[MAX_LOC];
	FILE      *rlogfile  = NULL;        /* Optional log file */
        LOCATION        rlogloc;
	char              rlogfspec[MAX_LOC];
#ifndef NT_GENERIC
	char		 *rptr;
#endif /* END #ifndef NT_GENERIC */
/** open a .RPT file based on PID **/
 
/** strip off .log **/
if ( *tests[0]->t_outfile )
        {
            STcopy( tests[0]->t_outfile, rlogfspec );
            strcat( rlogfspec, ".rpt" );
        }
        else
/** make our own name using pid **/
            STprintf(rlogfspec,"%d.rpt",achpid );


 STcopy(outdir, rlogfname);

 LOfroms(PATH, rlogfname, &rlogloc);
 
 LOfstfile( rlogfspec , &rlogloc );

/*SIprintf("writing logfile %s\n",rlogfspec); */

if (interflg) /** tell user where it is - he might have forgotten **/
   SIprintf("REPORT FILE is  located at: %s\n",rlogfname);

 if (SIopen(&rlogloc, "w", &rlogfile) != OK)
        {
            SIprintf("Unable to open log file.\n");
            rlogfile = 0;
        }
#ifndef NT_GENERIC 
        else
        {
            ACchown(&rlogloc, uid);
        }
#endif /* #ifndef NT_GENERIC */
	    ACgetTime(&now, (LO_RES_TIME *) NULL);
	    ACtimeStr(&now, ct);

	    SIfprintf(rlogfile,"\n ***** ACHILLES SUMMARY REPORT - STATUS as of %s ****\n", ct);
       SIfprintf(rlogfile,"\n\t\t **** FOR TEST RUN OF %d CHILDREN **** \n", 
tot_children);
	    SIfprintf(rlogfile,"-------------------------------------------------------------------------\n\n");

	    for (i = 0 ; i < numtests; i++)
	    {
		SIfprintf(rlogfile,"Test_Name\t\tTotal ChldRun\tTotal ChdBad\tAvgTime  Chld Ran\n");
	 SIfprintf(rlogfile,"---------\t\t------------\t------------\t----------------------\n");
#if defined(VMS) || defined(NT_GENERIC)
	SIfprintf(rlogfile,"%-16s\t%6d\t\t%6d\t\t%8.2f hr(s)\n",
                 tests[i]->t_argv[0],
           tests[i]->t_ctotal,tests[i]->t_cbad,
          (tests[i]->t_ctotal < 1 ) ? 0 :
		 tests[i]->t_ctime/tests[i]->t_ctotal,'.');

#else
        SIfprintf(rlogfile,"%-16s\t%6d\t\t%6d\t\t%8.4f hr(s)\n",
            	(strstr("/bin/sh",tests[i]->t_argv[0]) ? 
            		strrchr(tests[i]->t_argv[2],'/') + 1 : 
            		strrchr(tests[i]->t_argv[0],'/') + 1), 
            	tests[i]->t_ctotal,tests[i]->t_cbad, 
            	(tests[i]->t_ctotal < 1 )  ? 0.00 : 
            		tests[i]->t_ctime/tests[i]->t_ctotal,'.');
/*        SIfprintf(rlogfile,"time is %8.4f\n",tests[i]->t_ctime,'.');
        SIfprintf(rlogfile,"tot  %d\n",tests[i]->t_ctotal);
*/
#endif
	    tot_bad+=tests[i]->t_cbad; 
	    tot_tests+=tests[i]->t_ctotal;
	    tot_time+= (tests[i]->t_ctotal < 1 ) ? 0 : 
              tests[i]->t_ctime/tests[i]->t_ctotal ; 
          }
	 SIfprintf(rlogfile,"--------------------------------------------------------------------------------\n");
       SIfprintf(rlogfile,"Cumulative Totals\t%6d\t\t%6d\t\t%8.2f \n",tot_tests,tot_bad,tot_time,'.');

/* 
SIfprintf(rlogfile,"\n nows %d achilles_startt %d \n", now.millitm ,
achilles_start.millitm,'.');
SIfprintf(rlogfile,"\n now %d achilles_start %d \n", now.time,
achilles_start.time,'.');
*/

 SIfprintf(rlogfile,"\nDURATION:\t\t%d\thour(s)\n",
             ACcompareTime(&now,&achilles_start)/3600 < 1 ? 0:
              ACcompareTime(&now,&achilles_start)/3600,'.' );

	 (logfile == stdout) ? SIfprintf(rlogfile,"LOG FILE is STDOUT\n") : SIfprintf(rlogfile,"\nLOG FILE located at: %s\n",tembuf);
	 SIfprintf(rlogfile,"REPORT FILE is  located at: %s\n",rlogfname);     
 
	    SIputc('\n', stdout);
	    SIflush(stdout);

}
