/*
** Copyright (c) 2004, 2010 Ingres Corporation
**
NO_OPTIM = rmx_us5  rux_us5 sqs_ptx ris_u64 rs4_us5 i64_aix
*/

# include    <compat.h>
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
**	24-jun-1997 (hweho01)
**		Add sleep support for Unixware (usl_us5) with sleep() from libc.
**	29-jul-1997 (walro03)
**		Unoptimize for Tandem NonStop (ts2_us5).  csspintest with
**		more than one forked process loops unless this program is
**		not optimized.
**	17-may-1997 (mosjo01)
**		Remove usleep from dgi_us5 processing, it hangs.
**	32-feb-1998 (toumi01)
**		For Linux (lnx_us5) delay the CS_des_installation().
**		This way we keep the sysseg.mem around until after
**		spintest.mem is allocated.  If this is NOT done we get into
**		trouble: spintest.mem ends up using the same inode as
**		sysseg.mem in $II_SYSTEM/ingres/files/memory, which
**		causes ftok to hash the same IPC key.  This leads to an
**		"already exists" error when we try to allocate spintest.mem,
**		because sysseg.mem is still allocated (the destory marks
**		the shared segment as such, but since an shmdt is not done
**		the nattch is still 1, and the segment is still there).
**	24-jun-1998 (muhpa01)
**		Add hp8_us5 to list using usleep().
**	25-jun-1998 (walro03)
**		After fixes applied on rs4_us5 (AIX 4.1), no need to run shmdt
**		any more.
**      09-aug-1998 (hweho01)
**              Changed to use sleep() for dg8_us5; the process
**              hangs if usleep() is used on DG/88K OS/R.4.11MU04.
**	10-may-1999 (walro03)
**		Remove obsolete version string pyr_u42.
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      7-Sep-1999 (bonro01)
**          Modified result variable in spin_on_lock() routine to be VOLATILE
**          to eliminate infinite loop caused by optimization errors.
**          I found that a while() loop without a statement to execute will
**          cause the optimizer to load the test condition variable into a
**          register from memory before the loop, then the test in the loop
**          will only compare the register value and not the value in memory.
**          This kind of optimization is fine for a single-process
**          single-threaded program, but not for a shared memory multi-process
**          application.  Using VOLATILE on the shared memory structure will
**          tell the optimizer that this storage may be modified from outside
**          of the current process.
**          This should correctly fix all the other platforms that had looping
**          problems by letting the compiler know that it needs to refresh the
**          CPU registers with the memory value each time through the while
**          loop.  I've removed NO_OPTIM and sleep() calls for other platforms.
**          Some platforms used NO_OPTIM to get around this problem, and some
**          used sleep() within the while() loop to get around this problem.
**          The sleep fixed the problem because a function call within the loop
**          forced the optimizer to reload the while condition variables from
**          memory.
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**      08-Sep-2000 (linke01)
**              Added NO_OPTIM for dgi_us5 as part 5 of cltest relating to
**              csspintest was looping forever.
**       8-dec-1999(mosjo01 SIR 97550)
**         Need NO_OPTIM for sqs_ptx to perform better but not 100%.
**      19-May-2000 (hweho01)
**         Added support for AIX 64-bit platform (ris_u64).
**	19-May-2000 (ahaal01)
**	    Added NO_OPTIM for rs4_us5 to prevent csspintest from hanging.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-jun-2000 (hanje04)
**	   Delay the CS_des_installation() For Linux for OS/390 (ibm_lnx) as
**	   for int_lnx. See 32-feb-1999(?) toumi01 change for details 
**	07-Sep-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) to int_lnx delay of 
**	    CS_des_installation
**	01-Dec-2000 (bonro01)
**	    This fixes a bug that has been in csspintest a long time.
**	    Code in spin_on_lock() was calling CS_SPININIT() before
**	    incrementing the result->start field.  This had the result
**	    of allowing two processes to grab the same lock because the
**	    process did not lock the shared memory when it updated the
**	    the result->start process count.  Changing the CS_SPININIT() to
**	    a CS_TAS locked the shared memory for access and fixed the bug.
**	    On a fast multiprocessing system, the last child process would
**	    increment the process count and allow the other children to 
**	    continue executing and grab the lock.  The last child would
**	    then execute the CS_ACLR() instruction and reset the
**	    lock bit when another child had it locked.
**	    Also discovered that NO_OPTIMization is still required for AIX.
**	    With optimization on, csspintest hangs.  I examined the assembly
**	    code and determined that csspintest was hung on a single
**	    assembly instruction that kept testing a register value
**	    and looping on the same instruction until the register changed.
**	    This was the code generated for the statement
**	    while(result->start != num_child);  in spin_on_lock()
**	    The optimizer did not correctly interpret the VOLATILE type
**	    on the result variable and the code was generated wrong,
**	    so NO_OPTIM is required for AIX.
**	25-jul-2001 (toumi01)
**	    NO_OPTIM i64_aix else csspintest fails
**	03-Dec-2001 (hanje04)
**	    Added IA64 Linux to list of other Linuxes delaying 
**	    CS_des_installation.
**	07-oct-2004 (thaju02)
**	    Change alloc_pages to SIZE_TYPE.
**	12-Jan-2004 (hanje04)
**	    Add support for AMD64 Linux
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**      23-Mar-2007
**          SIR 117985
**          Add support for 64bit PowerPC Linux (ppc_lnx) by
**	    replacing all *_lnx references with LNX.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	15-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/

/* Testing constants */

/* Number of spins for which to waste time in critical section */
# define CS_SPINS	50

/*
PROGRAM = csspintest

OWNER =	INGUSER

MODE = SETUID

NEEDLIBS = COMPATLIB MALLOCLIB
*/

VOID perror();

typedef struct tally {
	i4     start;
        CS_SPIN spinbit;
	unsigned int counter;	
	i4  	slot[100];
	i4 	set;
	} TALLY;

static void spin_on_lock(i4 revolutions, i4 lockid,
	VOLATILE TALLY * result,
	i4 use_tas, i4 num_child);

static void getargs(i4 argc, char **argv,
	i4 *revolutions, i4 *num_child, i4 *use_tas);

/*ARGSUSED*/
main(argc, argv)
i4	argc;
char	**argv;
{
    STATUS     status = OK; 
    PTR	       address;
    LOCATION   shmid;
    CL_SYS_ERR err_code;
    volatile TALLY      *result;
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
#if !defined(LNX)
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

    for (x = 0; x < num_child; x++)
    {
        if (fork() == 0)		/* In child */
    	{
#if defined(ris_us5)
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
    while(wait(0) > 0);			/* wait for everyone to finish */
    SIprintf("Finished spinning...\n");
    for (x = 0; x < num_child; x++)
	 completed += result->slot[x];
    SIprintf("%d out of %d children successfully acquired locks\n",completed,
				num_child);
    if (!use_tas)
        SIprintf("Number of collisions (cssp_collide)=%d\n",
				result->spinbit.cssp_collide);
    MEsmdestroy("spintst.mem", &err_code);     /* cleanup when done */
#if defined(LNX)
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
static void
spin_on_lock(i4 revolutions, i4 lockid, volatile TALLY *result,
	i4 use_tas, i4 num_child)
{
    i4		x;
    i4		y;

    /* Increment child count, shared data must be locked to update */
    CS_TAS(&result->spinbit.cssp_bit); 
    result->start++;	/* This child is ready to go... */
    CS_ACLR(&result->spinbit.cssp_bit);

    /* Spin until all processes have been forked and all shmem attached */
    while(result->start != num_child);

    for (x = 0; x < revolutions ; x++)
    {
	if (use_tas)
    	    while (!CS_TAS(&result->spinbit.cssp_bit)); 
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
        	CS_ACLR(&result->spinbit.cssp_bit);
	else
		CS_relspin(&result->spinbit);
    }
        result->slot[lockid] = 1;
}

static void
getargs(i4 argc, char **argv, i4 *revolutions, i4 *num_child, i4 *use_tas)
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
