/*
** Copyright (c) 2004, 2010 Ingres Corporation
**
NO_OPTIM = sui_us5 rmx_us5 rux_us5 i64_aix
*/

# include    <compat.h>
# ifdef OS_THREADS_USED
# include    <gl.h>
# include    <clconfig.h>
# include    <systypes.h>
# include    <cs.h>
# include    <fdset.h>
# include    <csev.h>
# include    <cssminfo.h>
# include    <errno.h>
# include    <lo.h>
# include    <me.h>
# include    <nm.h>
# include    <si.h>
# include    <tr.h>

/* don't want to go through LGKrundown PCatexit glorp */
# define PCexit(s)      exit(s)

#if defined(VMS)
static i4 fork(void) {return (0);}
#endif

#ifdef ds3_ulx
#include <sys/time.h>
struct timeval   st_delay;
#define usleep(x)       { st_delay.tv_usec = (x); st_delay.tv_sec = 0; \
                          select(32, NULL, NULL, NULL, &st_delay); }
#endif

/**
**
**  Name: CSSPINTEST.C - A simple test of CS locking and shared memory
**
**  Description:
** 	This program tests the CS locking routines CS_getspin, CS_tas
**	plus MEget_pages, MEget_shared and MEsmdelete. 
**
**    usuage:  csspintest [-t ] number_of_locks number_of_children
**      	where -t sets locking call to CS_tas instead of CS_getspin
**
**   Note:  If this program aborts prematurely, make sure you delete
**   outstanding shared memory segments manually.
**
**          main() - locking  test
**
**
**  History:
**	22-Mar-89 (GordonW)
**	    Correct certain bugs.
**      03-mar-89 (markd)
**          First version
**	12-jun-89 (rogerk)
**	    Changed MEget_pages, MEsmdestroy calls to take character string
**	    segment name rather than a LOCATION pointer.  Added allocated_pages
**	    argument to MEget_pages.
**	15-may-90 (kimman)
**	    Adding ds3_ulx specific fix so that csspintest will work.
**		1)  cannot attach to same shmid in same process space more
**			than once
**		2)  slow down while loop to let other processes fork
**	5-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Set NO_OPTIM for pyr_u42;
**		Add include <clconfig.h> to pick up correct ifdefs in <csev.h>;
**		Clean up release lock to use CS_ACLR or CS_relspin based on
**		use_tas.
**	09-apr-91 (seng)
**	    Add RS/6000 to machines with systems that can't attach to same shmid
**	    in the same process space more than once.
**	3-jul-1992 (bryanp)
**	    Pass new num_users arg to CS_create_sys_segment().
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      30-Nov-94 (nive/walro03)
**              Integrated change from FCS1 port by kevinm
**              Added dg8_us5 to ds3_ulx specific change that slowed down the
**              loop to let other processes to fork.
**      22-jun-95 (allmi01)
**              Added dgi_us5 to dg8_us5 changes above. DG-UX is DG-UX 
**              whether or not it is on a M68k M88k or Intel based box.
**      25-jul-95 (morayf)
**              Added NO_OPTIM for sos_us5 as optimisation causes process to
**              hang.
**      30-jul-1995 (pursch)
**              Add changes made by erickson from 6405.
**              Slow down loop to allow processes to fork for pym_us5.
**              Turn off optimization for pym_us5. With optim turned on,
**              using the/usr/opt/cc3.11/bin/cc (Optional 3.11 C compiler),
**              process hangs.
**	15-nov-1995 (murf)
**		Added NO_OPTIM for sui_us5 as part 5 of cltest relating to
**		csspintest was looping forever. OS is solaris 2.4 and the
**		compiler is 3.0.1
**	18-dec-95 (morayf)
**		Added NO_OPTIM for rmx_us5 as optimisation causes process to
**		hang for version 1.1A of SINIX 5.42 C compiler.
**	20-feb-1996 (toumi01)
**		Add axp_osf to list of platforms that need sleep().
**	03-jun-1996 (canor01)
**		Use different structure member for CS_TAS testing, for
**		compatibility with operating system threads.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmtspintest.c and moved all calls to CSMT...
**      07-mar-1997 (canor01)
**          Make all functions dummies unless OS_THREADS_USED is defined.
**	25-jun-1998 (walro03)
**		After fixes applied on rs4_us5 (AIX 4.1), no need to run shmdt
**		any more.
**      09-aug-1998 (hweho01)
**          Changed to use sleep() for dg8_us5; the process
**          hangs if usleep() is used on DG/88K OS/R.4.11MU04.
**	10-may-1999 (walro03)
**	    Remove obsolete version string pyr_u42.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	08-dec-1999 (mosjo01 SIR 97550)
**	    Print a message indicating when test not applicable.
**      13-dec-1999 (hweho01)
**          Added support for AIX 64-bit platform (ris_u64). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	22-aug-2000 (toumi01)
**	   Modified to properly work on Linux platforms.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-jul-2001 (toumi01)
**	    NO_OPTIM i64_aix else csspintest fails
**	03-Dec-2001 (hanje04)
**	   Added IA64 Linux to Linux specific changes.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	6-Jul-2005 (schka24)
**	    Fix bugs on x86-64: clear startup counter initially, and use
**	    adjust-counter to increment it.  Sleep instead of CPU-spinning
**	    while waiting for startup and shutdown.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**      20-mar-2009 (stegr01)
**          exclude calls to fork() for VMS
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**/

/* Testing constants */

/* Number of spins for which to waste time in critical section */
# define CS_SPINS	50

/*
PROGRAM = csmtspintest

NO_OPTIM = sos_us5

OWNER =	INGUSER

MODE = SETUID

NEEDLIBS = COMPATLIB MALLOCLIB
*/

VOID perror();

typedef struct tally {
	i4     start;
        CS_SPIN spinbit;
	CS_ASET abit;
	unsigned int counter;	
	i4  	slot[100];
	i4 	set;
	} TALLY;

extern	int	errno;

/*ARGSUSED*/
main(argc, argv)
i4	argc;
char	**argv;
{
    STATUS     status = OK; 
    PTR	       address;
    LOCATION   shmid;
    CL_SYS_ERR err_code;
    TALLY      *result;
    i4     x; 
    i4     revolutions = 1000; 
    i4     num_child = 1;
    i4     completed  = 0; 
    i4     use_tas = FALSE;
    SIZE_TYPE	alloc_pages;
   
    MEadvise(ME_INGRES_ALLOC);

    TRset_file(TR_F_OPEN, "stdio", 5, &err_code);

    /*
    ** check if system segment is there, if so abort
    */
    if( CS_create_sys_segment(2, 50, &err_code))
    {
         perror("Can't run now. There is a shared memory segment mapped already");
	 PCexit(1);
    }
#ifndef LNX
    CS_des_installation();
#endif

    NMloc(ADMIN,FILENAME,"spintstseg",&shmid);
    getargs(argc, argv, &revolutions, &num_child,&use_tas); 
    SIprintf("\nLock structure is %d byte(s).\n",sizeof(CS_ASET));
    if (use_tas)
        SIprintf("%s: %d child processes making %d lock calls each using CS_tas.\n",argv[0],num_child,revolutions);
    else
        SIprintf("%s: %d child processes making %d lock calls each using CS_getspin.\n",argv[0],num_child,revolutions);

    if (status = MEget_pages(ME_MSHARED_MASK|ME_MZERO_MASK|ME_CREATE_MASK, 1,
                             "spintst.mem", &address, &alloc_pages, &err_code))
    {
         perror("Parent could not create shared segment");
	 PCexit(1);
    }
    result = (TALLY *) address;
    /* once, for everybody */
    CS_SPININIT(&result->spinbit);
    CS_ACLR(&result->abit);
    result->start = 0;

    for (x = 0; x < num_child; x++)
    {
        if (fork() == 0)		/* In child */
    	{
#if defined(ds3_ulx) || defined(ris_us5)
	    shmdt(address);
#endif
    	    if (status = MEget_pages(ME_MSHARED_MASK,1,"spintst.mem", &address, 
				     &alloc_pages, &err_code))
    	   {
                 perror("Child could not create shared segment");
		 PCexit(1);
    	    }
           result = (TALLY *) address;
    	   spin_on_lock(revolutions, x, result, use_tas, num_child);
    	   PCexit(0);
    	}
    }
    /* Parent continues */
    while(wait(0) > 0) sleep(1);			/* wait for everyone to finish */
    SIprintf("Finished spinning...\n");
    for (x = 0; x < num_child; x++)
	 completed += result->slot[x];
    SIprintf("%d out of %d children successfully acquired locks\n",completed,
				num_child);
# ifndef OS_THREADS_USED
    if (!use_tas)
        SIprintf("Number of collisions (cssp_collide)=%d\n",
				result->spinbit.cssp_collide);
# endif /* OS_THREADS_USED */
    MEsmdestroy("spintst.mem", &err_code);     /* cleanup when done */
#ifdef LNX
    CS_des_installation();
#endif
    if (completed == num_child)
        PCexit(0); 
    else
	PCexit(1);
}

/*
**
**  spin_on_lock
**
*/
spin_on_lock(revolutions, lockid, result, use_tas, num_child)
i4	revolutions, lockid, use_tas, num_child;
struct tally * result;
{
    i4		x;
    i4		y;

    CSadjust_counter(&result->start,1);	/* This child is ready to go... */
    /* Spin until all processes have been forked and all shmem attached */
    while(result->start != num_child)
    {
#if defined(ds3_ulx) || defined(any_aix) || \
    defined(pym_us5) || defined(dgi_us5) || defined(axp_osf) || \
    defined(int_lnx) || defined(int_rpl)
        usleep(10L);
#endif

#if defined(dg8_us5)
        sleep(1);
#endif    /* if defined dg8_us5  */
    }

    for (x = 0; x < revolutions ; x++)
    {
	if (use_tas)
    	    while (!CS_TAS(&result->abit)); 
	else
            CS_getspin(&result->spinbit); 
        if (result->set == 1)
        {			
	    if (use_tas)
    	    	SIprintf("ERROR: CS_TAS found \"set\" already locked in child %d on lock call %d.\n",lockid,x);
            else
    	    	SIprintf("ERROR: CS_getspin found \"set\" already locked in child %d on lock call %d.\n",lockid,x);
       	    PCexit(1);
        }
        result->set = 1;
        for (y = 0; y < CS_SPINS; y++);  /* Waste some time in crit. sec. */
	result->set = 0;

	if (use_tas)
        	CS_ACLR(&result->abit);
	else
		CS_relspin(&result->spinbit);
    }
        result->slot[lockid] = 1;
}

getargs(argc, argv, revolutions, num_child, use_tas)
i4	argc;
char	**argv;
i4	*revolutions, *num_child, *use_tas;
{
	i4 	argvpointer; 
 	argvpointer = argc - 1; 
	if(argc < 2)
		return;

	if ( (argv[1]) [0] == '-')
	{
 		if ( (argv[1])[1] == 't') 
		{
			*use_tas = TRUE;
			argc--;
		}
		else
		{
			SIfprintf(stderr,"%s: bad flag\n",argv[0]);
			SIfprintf(stderr,"Usage: %s [-t] revolutions children\n",argv[0]);
			PCexit(1);
		}
	}
	if (argc > 3)
	{
		SIfprintf(stderr,"%s: too many arguements\n",argv[0]);
		SIfprintf(stderr,"Usage: %s [-t] revolutions children\n",argv[0]);
		PCexit(1);
	}
	if (argc == 3)
	{
		if (( ((*num_child) = atoi(argv[argvpointer])) <= 100) &&
			(*num_child > 0))
		{
			argc--;
			argvpointer--;
		}
		else
		{
			SIfprintf(stderr,"ERROR: limitation of between 1 to 100 processes\n");
			PCexit(1);
		}
	}
	if (argc == 2)
		*revolutions = atoi(argv[argvpointer]);
}
# else /* OS_THREADS_USED */
main( int argc, char **argv )
{
    SIprintf("csmtspintest not performed because OS_THREADS_USED does not \
apply to this platform.\n");
    return (1);
}
# endif /* OS_THREADS_USED */
