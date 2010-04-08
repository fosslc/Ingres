/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: MACHCONF.H - Defines for cl internal users of the ii_sysconf() routine
**
** Description:
**	Necessary info to call ii_sysconf() to get machine configuration
**	information.  Initial use will be to determine the number of processors
**	on the machine, later more system configuration info may be available.
**
** History:
**      01-nov-90 (mikem)
**          Created.
**	04-mar-92 (mikem)
**	    SIR #43445
**	    Added 2 new system configuration entries:
**	    IISYSCONF_MAX_SEM_LOOPS, IISYSCONF_DESCHED_USEC_SLEEP.  
**	    See ii_sysconf() for descriptions of their use.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25th-Dec-2004 (shaha03)
**	    SIR #113754 Added macro "GETNUMSPUS" definition for i64_hpu, as 
**	    HP-UX itanium system headers don't have it.	
**/


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN i4 ii_sysconf();    /* return system config info */


/*
**  Defines of other constants.
*/

/* Defines for available system configuration info */

#define		IISYSCONF_PROCESSORS  		0x01
#define		IISYSCONF_MAX_SEM_LOOPS		0x02
#define		IISYSCONF_DESCHED_USEC_SLEEP	0x03

/* Defaults for non-ifdef'd machine specific configuration information */

/* default number of processors - ie. single processor */
#define         DEF_PROCESSORS                  1
/* default number of loops to spin on a MP before going into sleep */
#define         DEF_MAX_SEM_LOOPS               200
/* number of micro-seconds to sleep once the loop count has been exceeded */
#define         DEF_DESCHED_USEC_SLEEP          1

/*Number of CPUs in the machine */
#if defined(i64_hpu)
#define GETNUMSPUS()    \
	syscall(SYS_MPCTL, MPC_GETNUMSPUS_SYS)
#endif
