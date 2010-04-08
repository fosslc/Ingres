/*
 * achilles.h
 *
 * This is the only header file which must be included by the source files
 * for Achilles. It contains all needed system include files, special data
 * structures, non-integer external function declarations, and global
 * variable declarations.
 *
 * History:
 *    16-jan-89  (mca) - Written.
 *    26-Sept-89 (GordonW)
 *	added maxkids
 *    5-Aug-93 (jeffr)
 *	added three new fields  in TESTDESC for  enhanced Report Generation
 *	in Achilles Version 2
 *   20-Dec-94 (GordonW)
 * 	cleanup.
 *	 10-Jan-96 (bocpa01)
 *		Added NT support (NT_GENERIC)
 *   21-May-1997 (allmi01)
 *	Added support for sos_us5 (x-integ from 1.2/01).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    08-oct-2008 (stegr01)
**       insert includes for <starlet> and <astjacket> for VMS Itanium
**	21-Aug-2009 (kschendel) 121804
**	    Add function declarations for gcc 4.3.
**	10-Sep-2009 (kschendel) 121804
**	    Above broke VMS because it declares ACunblock as a macro.  Fix.
*/

#ifndef _ACHILLES_H
#define _ACHIlLES_H

#include <compat.h>

#ifdef sos_us5
#define _SVID3
#define __SCO_WAIT3__
#endif

#include <gl.h>

#include <cm.h>
#include <cv.h>
#include <lo.h>
#include <me.h>
#include <pc.h>
#include <pe.h>
#include <si.h>
#include <st.h>

#include "accompat.h"

/*
 * Each instance of a test type is represented by an ACTIVETEST structure.
 */

#if defined(VMS) && defined(OS_THREADS_USED)
#include <starlet.h>
#include <astjacket.h>
#endif

#ifdef VMS
typedef struct {
	i4	    a_pid;		/* Process ID */
	unsigned short
		    a_status[4];	/* Process iosb */
	i4	    a_iters;		/* Iteration number for this instance */
	LO_RES_TIME a_start;		/* Start time of this fe process */
	char	    a_outfstr[MAX_LOC],	/* Output filename - will be stdout of
					   child */
		   *a_argv[32],		/* Argv used to invoke process */
		    a_childnum[8];	/* String version of instance number -
					   used by a_argv if <CHILDNUM> has
					   been expanded */
	LOCATION    a_outfloc;
} ACTIVETEST;

#else
typedef struct {
#ifdef NT_GENERIC
	PROCESS_INFORMATION	
			a_processInfo; /* child process info */
	i4	
#else
	i4	    a_pid,		/* Process ID */
#endif /* END #ifdef NT_GENERIC */
		    a_status,
		    a_iters;		/* Iteration number for this instance */
	LO_RES_TIME a_start;		/* Start time of this fe process */
	char	    a_outfstr[MAX_LOC],	/* Output filename - will be stdout of
					   child */
		   *a_argv[MAX_LOC],	/* Argv used to invoke process */
		    a_childnum[8];	/* String version of instance number -
					   used by a_argv if <CHILDNUM> has
					   been expanded */
	LOCATION    a_outfloc;
} ACTIVETEST;
#endif /* VMS */

/*
 * Each test type is represented by a TESTDESC structure.
 */

#ifdef VMS
typedef struct _t1 {
	char	   *t_argv[32],		/* Generic argv for test type */
		    t_argvdata[128];	/* Holds data that argv points at */
	i4	    t_nkids,		/* Number of instances to run */
		    t_niters,		/* # times to run each instance */
		    t_intgrp,		/* # processes to interrupt at each
					   interrupt interval */
		    t_killgrp,		/* Number of processes to kill at each
					   kill interval */
		    t_intint,		/* Interrupt interval, in seconds */
		    t_killint;		/* Kill interval, in seconds */
	UID	    t_user;		/* User ID to run this test under */
	char	    t_uname[32];	/* User name of detached processes */
	char	    t_infile[32],	/* Input file name */
		    t_outfile[32];	/* Output file name */
	HI_RES_TIME t_inttime,		/* Time next interrupt is due */
		    t_killtime;		/* Time next kill is due */
	ACTIVETEST *t_children;		/* Instances of this test type */
	struct _t1 *t_next;		/* Link pointer - used only when
					   reading config file; array of test
					   ptrs built after that. */
	i4	    t_ctotal;		/* total chld for this test instance */
        float       t_ctime;		/* running total of chld for test instance*/ 
	i4	    t_cbad;		/* total chld bad for this test instance*/ 	

	} TESTDESC;

#else
typedef struct _t1 {
	char	   *t_argv[128],	/* Generic argv for test type */
#ifdef NT_GENERIC
		   *t_cmdLine,			/* child command line */	   
#endif /* END #ifdef NT_GENERIC */
		    t_argvdata[MAX_LOC];/* Holds data that argv points at */
	i4	    t_nkids,		/* Number of instances to run */
		    t_niters,		/* # times to run each instance */
		    t_intgrp,		/* # processes to interrupt at each
					   interrupt interval */
		    t_killgrp,		/* Number of processes to kill at each
					   kill interval */
		    t_intint,		/* Interrupt interval, in seconds */
		    t_killint;		/* Kill interval, in seconds */
	UID	    t_user;		/* User ID to run this test under */
	char	    t_infile[128],	/* Input file name */
		    t_outfile[128];	/* Output file name */
	HI_RES_TIME t_inttime,		/* Time next interrupt is due */
		    t_killtime;		/* Time next kill is due */
	ACTIVETEST *t_children;		/* Instances of this test type */
	struct _t1 *t_next;		/* Link pointer - used only when
					   reading config file; array of test
					   ptrs built after that. */
	i4         t_ctotal;           /* total chld for this test instance */
        float       t_ctime;            /* running total of chld for test instan
ce*/
	i4         t_cbad;		/* total chld bad for this test instance
*/
} TESTDESC;
#endif /* VMS */

/*
 * When the user specifies the -l option, LOGBLOCK records are written to a
 * log file, rather than the default of human-readable information written
 * to stdout. We use this structure rather than ASCII text because this takes
 * about 1/5 as much space. When tests run for 24 hours there will be LOTS of
 * log records, and this space saving will be needed. Note that most of these
 * fields are represented as nats throughout the achilles program - they're
 * explicitly converted to shorts and chars here to save space.
 *   The readlog program is used to convert the log file back into human-
 * readable form.
 */

typedef struct {
	u_i4 l_sec;		/* Seconds component of current time */
	u_i1 l_usec,		/* usec component of cur. time / 10000 */
	     l_test;		/* Test number */
	u_i2 l_child,		/* Child number */
	     l_iter;		/* Iteration number */
	u_i2 l_pid;		/* Process ID */
	u_i1 l_action,		/* Action code (ie STARTED, KILLED, etc.) */
	     l_code;		/* Return code from EXIT or signal from TERM */
	u_i2 l_runtime;		/* Number of seconds process ran (optional) */
} LOGBLOCK;

/*
 * Action codes, for LOGBLOCK.l_action above and log_actions[] below.
 */
#define C_START     0
#define C_NO_START  1
#define C_INT       2
#define C_NO_INT    3
#define C_KILL      4
#define C_NO_KILL   5
#define C_EXIT      6
#define C_TERM      7
#define C_ABORT     8
#define C_NOT_FOUND 9

#define CHILD	1
#define STATRPT 2
#define ALARM	4
#define ABORT	8

/*
 * Globals
 */
GLOBALREF i4	     numtests;	/* Number of test types */
GLOBALREF TESTDESC **tests;	/* Array of test types */
GLOBALREF char	    *testpath, 	/* User-specified location of frontends */
		    *dbname;	/* User-specified database name */
#ifndef NT_GENERIC 
GLOBALREF UID	     uid;	/* Real user ID of achilles master */
#endif /* #ifndef NT_GENERIC */
GLOBALREF char	     stat_char;
GLOBALREF FILE 	    *logfile;	/* File pointer to log file - 0 if logging
				   to stdout */

#ifdef VMS
GLOBALREF FILE	    *envfile;	/* File pointer to environment file - 0 if
				   none specified */
#else
GLOBALREF char      *maxkids;	/* User-specified children count */
#endif	/* VMS */

/* NOTE - The array width of log_actions declared here must match the
   width defines in log_actions.c.
*/
#ifdef NT_GENERIC
extern char log_actions[][16];	/* Names for log_entry actions. */
#else
GLOBALREF char log_actions[][16];	/* Names for log_entry actions. */
#endif /* END #ifdef NT_GENERIC */

FUNC_EXTERN void ACreport(void);

#ifndef VMS			/* Defined as a macro on VMS */
FUNC_EXTERN void ACunblock(void);
#endif

#endif /* _ACHILLES_H */
