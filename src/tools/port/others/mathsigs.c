/*
** Copyright (c) 1989, 2004 Ingres Corporation
**
*/
/*
** mathsigs.c - write clsecret.h defines for math signal behavior.
**
** Description:
**
**	Write the correct defines for the machine from the set:
**			xCL_046_IDIV_HARD
**			xCL_047_IDIV_SIGILL
**			xCL_IDIV_SIGTRAP
**			xCL_048_IDIV_UNDETECTED
**			xCL_049_FDIV_HARD
**			xCL_050_FDIV_SIGILL
**			xCL_FDIV_SIGTRAP
**			xCL_051_FDIV_UNDETECTED
**			xCL_052_IOVF_HARD
**			xCL_053_IOVF_SIGILL
**			xCL_IOVF_SIGTRAP
**			xCL_054_IOVF_UNDETECTED
**			xCL_055_FOVF_HARD
**			xCL_056_FOVF_SIGILL
**			xCL_FOVF_SIGTRAP
**			xCL_057_FOVF_UNDETECTED
**			xCL_058_FUNF_HARD
**			xCL_059_FUNF_SIGILL
**			xCL_FUNF_SIGTRAP
**			xCL_060_FUNF_UNDETECTED
**
** Usage:	mathsigs >> clsecret.h
**
**              mathsigs -p [IDIV FDIV IOVF FOVF FUNF]
**
**                  FLAGS
**
**                  -p     Prints out the results of the exception opperation
**                         in a comment.
**
**		    XXXX   Options [IDIV FDIV IOVF FOVF FUNF] perform only
**                         the specified opperation(s).
**
** History:
**	31-may-89 (daveb)
**		Created.
**	6-june-89 (harry)
**		Fix incorrectly terminated comment string for
**		"Looping SIGFPE".
**	08-jul-91 (johnr)
**		Remove unneeded escape characters. "\/" doesn't require it.
**	12-jan-93 (swm)
**		On some platforms bad arithmetic operations can cause SIGTRAP
**		signals (as well as SIGFPE or SIGILL). Added automatic
**		detection of these and appropriate generation of new CL
**		configuration strings: xCL_IDIV_SIGTRAP, xCL_FDIV_SIGTRAP,
**		xCL_IOVF_SIGTRAP, xCL_FOVF_SIGTRAP, and xCL_FUNF_SIGTRAP.
**      01-feb-93 (smc)
**          	Tidied up signal handler functions to be TYPESIG (*)().
**      27-mar-93 (smc)
**              Added xCL_FDIV_SIGTRAP that was missed in the previous
**              change to include SIGTRAP. Made the tests for a looping
**              FPE seperate from the 'type of signal' test so that
**              xCL_0xx_xxxx_HARD is generated even if the platform
**              does not loop. Split the tests up into seperate routines
**              and allowed these to be specifically requested from the
**              command line so that opperations with 'chaotic' behavour
**              could be studied seperately. Added print flag to display
**              results of undetected opperations to show if they are
**              simply IEEE complient.
**	19-mar-1998 (fucch01)
**		Added ifdef SIGTRAP's around code that checks for SIGTRAP.
**		We don't need to check for it if it doesn't exist, which
**		it doesn't on sgi_us5 w/ POSIX os threads supported.
**	26-mar-1998 (muhpa01)
**		For hpb_us5, include <generic.h> and add definition for HUGE
**	02-Nov-1998 (kosma01)
**		Many changes for IRIX 6.4. Enable fpe(s) by system function
**		set_fpc_csr. Define HUGE in a double way. Adjust PC register
**		in fpe_loop to avoid looping. Logically remove "for loop" way
**		of testing for looping, (it always will indicat loop).
**		no optim for sgi_us5 because the optimizer removes the code
**		that would have caused an exception and just initializes the 
**		floating point variable to <inf>.
**	11-dec-2001 (somsa01)
**		Added support for hp2_us5.
**	21-May-2003 (bonro01)
**		Add support for HP Itanium (i64_hpu)
**	02-nov-2004 (abbjo03)
**	    Add axm_vms to list defining HUGE_VAL.
**	18-Jan-2005 (hanje04)
**	    Remove definition of TYPESIG as it's defined in default.h
**  27-Jun-20006 (raddo01)
**		bug 116311: Must use POSIX signals on Linux or we core dump.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**       7-Nov-2006 (hanal04) SIR 117044
**          Correct broken line ifdef from integration of change 484289.
**      22-dec-2008 (stegr01)
**         Itanium VMS port.
*/

/* NO_OPTIM = sgi_us5 */

#ifdef _SGI_SOURCE
# include <pthread.h>
#endif
# include <stdio.h>
# include <setjmp.h>
#ifdef _SGI_SOURCE 
# include <math.h>
# include <float.h>
# undef HUGE
# define HUGE DBL_MAX
# include <sys/fpu.h>
# include <errno.h>
#else
# include <math.h>
#endif
# include <signal.h>
# include <generic.h>

# if defined(hpb_us5) || defined(hp2_us5) || defined (i64_hpu) || defined(axm_vms) || defined(i64_vms)
#define HUGE	HUGE_VAL
# endif /* hpb_us5 hp2_us5 */

#ifdef _SGI_SOURCE
int sgi_enable_fpe(void);
#endif

/*
**	Linux uses the POSIX signal model & so must we to avoid core dumps.
*/
#if defined(a64_lnx) || defined(int_lnx) || defined(int_rpl) || defined(i64_lnx) || defined(i64_lnx)
sigjmp_buf  jb = {0};
int         sm = 1;			/* Non-zero to save current signal mask. */
# undef     longjmp
# define    longjmp	siglongjmp
# undef     setjmp
# define    setjmp(b)	sigsetjmp((b), sm)
#else
jmp_buf jb;
#endif	/* *_lnx */
int count, iz = 0, print = 0;
double dz = 0.0;
float fz = 0.0;

#ifdef _SGI_SOURCE
TYPESIG fpe_loop( int sig, int code, struct sigcontext *sc )
#else
TYPESIG fpe_loop( sig )
int sig;
#endif
{
	(void)signal( sig, fpe_loop );
#ifdef sgi_us5
	sc->sc_pc += 4;    /* four bytes is the length of any fp instruction */
#endif
	if( ++count > 5  )
	{
		printf("/* Looping SIGFPE */\n" );
		longjmp( jb, sig );
	}
}

TYPESIG fpe_handler( sig )
int sig;
{
	printf("/* Floating point exception */\n" );
	(void)signal( sig, fpe_handler );
	longjmp( jb, sig );
}

TYPESIG ill_handler( sig )
int sig;
{
	printf("/* Illegal instruction */\n" );
	(void) signal( sig, ill_handler );
	longjmp( jb, sig );
}

TYPESIG trap_handler( sig )
int sig;
{
	printf("/* Trace trap */\n" );
	(void) signal( sig, trap_handler );
	longjmp( jb, sig );
}

main(argc, argv)
int argc;
char *argv[];
{
	int useage = 0;
	char *p;

#ifdef _SGI_SOURCE
	sgi_enable_fpe();
#endif
	/* set up handlers that remain constant */
	(void)signal( SIGILL, ill_handler );
# ifdef SIGTRAP
	(void)signal( SIGTRAP, trap_handler );
# endif

	/* check for print flag */
	if (argc > 1 && !strcmp(argv[1], "-p")) 
	{
	        print = 1;
		argc--;
		argv++;
	}

	/* do the required tests */
	if (argc == 1) 
	{
		idiv_test();
		fdiv_test();
		iovf_test();
		fovf_test();
		funf_test();
	}
	else 
	{
		while (--argc > 0)
		{
			++argv;

			if (!strcmp(*argv, "IDIV"))
				idiv_test();
			else if (!strcmp(*argv, "FDIV"))
				fdiv_test();
			else if (!strcmp(*argv, "IOVF"))
				iovf_test();
			else if (!strcmp(*argv, "FOVF"))
				fovf_test();
			else if (!strcmp(*argv, "FUNF"))
				funf_test();
			else
		       	{
			        printf("illegal option -- %s\n", *argv);
				useage = 1;
			}

			if (useage)
				printf("useage: mathsigs -p [IDIV FDIV IOVF FOVF FUNF]\n");
		}
	}

	exit(0);
}

/*
 * integer divide by zero
 */

idiv_test()
{
	int i, j, l;

	/* test for looping FPE first of all */

	(void)signal( SIGFPE, fpe_loop );

	if (!setjmp( jb ))
	{
	       count = 0;
	       i = 1 / iz;
	       j = 10 / iz;
	}

	/* now test for type of signal */

	(void)signal( SIGFPE, fpe_handler );

	if( (l = setjmp( jb )) == SIGFPE )
		printf( "# define xCL_046_IDIV_HARD\n");
	else if ( l == SIGILL )
		printf( "# define xCL_047_IDIV_SIGILL\n");
# ifdef SIGTRAP
	else if ( l == SIGTRAP )
		printf( "# define xCL_IDIV_SIGTRAP\n");
# endif
	else
	{
		i = 1 / iz;
		j = 10 / iz;
		printf("# define xCL_048_IDIV_UNDETECTED");
		if (print)
			printf("\t\t/* IDIV gives <%d> <%d> */", i, j);
		printf("\n");
	}
}

/*
 * floating point divide by zero
 */

fdiv_test()
{
        int l;
        double d;
        float f;

	/* reset handler to test for looping FPE */

	(void)signal( SIGFPE, fpe_loop );

	if (!setjmp( jb ))
	{
	       count = 0;

	       d = 1.0 / dz;
	       f = 1.0 / fz;
	}

	/* back to testing for type of signal */

	(void)signal( SIGFPE, fpe_handler );

	if( (l = setjmp( jb ) ) == SIGFPE )
		printf( "# define xCL_049_FDIV_HARD\n");
	else if ( l == SIGILL )
		printf( "# define xCL_050_FDIV_SIGILL\n");
# ifdef SIGTRAP
	else if ( l == SIGTRAP )
		printf( "# define xCL_FDIV_SIGTRAP\n");
# endif
	else
	{
		d = 1.0 / dz;
		f = 1.0 / fz;
		printf("# define xCL_051_FDIV_UNDETECTED");
		if (print)
			printf("\t\t/* FDIV gives <%f> <%f> */", d, f);
		printf("\n");
	}
}		

/*
 * integer overflow
 */

iovf_test()
{
        int i, l;

	/* reset handler to test for looping FPE */

	(void)signal( SIGFPE, fpe_loop );

	if (!setjmp( jb ))
	{
	       count = 0;

	       /* should cause overflow sometime...? */
	       for( i = 1.0; i > 0 ; i *= 17 )
			;
	}

	/* back to testing for type of signal */

	(void)signal( SIGFPE, fpe_handler );

	if( (l = setjmp( jb ) ) == SIGFPE )
		printf( "# define xCL_052_IOVF_HARD\n");
	else if ( l == SIGILL )
		printf( "# define xCL_053_IOVF_SIGILL\n");
# ifdef SIGTRAP
	else if ( l == SIGTRAP )
		printf( "# define xCL_IOVF_SIGTRAP\n");
# endif
	else
	{
		/* should cause overflow sometime...? */
		for( i = 1; i > 0 ; i *= 17 )
			;

		printf("# define xCL_054_IOVF_UNDETECTED");
		if (print)
			printf("\t\t/* IOVF gives <%d> */", i);
		printf("\n");
	}
}		

/*
 * floating point overflow
 */

fovf_test()
{
        double d, x;
        float f;
        int i, l;

	/* test for looping FPE first of all */

	(void)signal( SIGFPE, fpe_loop );

	if (!setjmp( jb ))
	{
	        x = HUGE / 10;
		count = 0;

#ifndef _SGI_SOURCE
		/* try to overflow a double */
		for( i = 0, d = HUGE - x; i < 1000 ; i++ )
			d *= HUGE;
#endif 
	
		/* if loop above didn't overflow, this conversion should. */
		f = d;
		
		/* if above left a nan, try something else */
		d = HUGE;
		d *= 2;
		f = d;
	}

	/* back to testing for type of signal */

	(void)signal( SIGFPE, fpe_handler );

	if( (l = setjmp( jb ) ) == SIGFPE )
		printf( "# define xCL_055_FOVF_HARD\n");
	else if ( l == SIGILL )
		printf( "# define xCL_056_FOVF_SIGILL\n");
# ifdef SIGTRAP
	else if ( l == SIGTRAP )
		printf( "# define xCL_FOVF_SIGTRAP\n");
# endif
	else
	{
		x = HUGE / 10;

		/* try to overflow a double */
		for( i = 0, d = HUGE - x; i < 1000 ; i++ )
			d *= HUGE;
	
		/* if loop above didn't overflow, this conversion should. */
		f = d;
		
		/* if above left a nan, try something else */
		d = HUGE;
		d *= 2;
		f = d;
		
		printf("# define xCL_057_FOVF_UNDETECTED");
		if (print)
			printf("\t\t/* FOVF gives <%f> */", f);
		printf("\n");
	}
}

/*
 * floating point underflow
 */

funf_test()
{
        float f;
	double d, x;
        int l, i;

	/* test for looping FPE first of all */

	(void)signal( SIGFPE, fpe_loop );

	if (!setjmp( jb ))
	{
		x = HUGE / 10;
	        count = 0;

#ifndef _SGI_SOURCE
		/* try to underflow a double */
		for( i = 0, d = 1/x; i < 1000 ; i++ )
			d /= HUGE;
#endif
	
		/* if loop above didn't underflow, this conversion should. */
		f = d;

		/* if above left a nan, try something else */
		d = 1 / HUGE;
		d /= 2;
		f = d;
	}

	/* back to testing for type of signal */

	(void)signal( SIGFPE, fpe_handler );

	if( (l = setjmp( jb ) ) == SIGFPE )
		printf( "# define xCL_058_FUNF_HARD\n");
	else if ( l == SIGILL )
		printf( "# define xCL_059_FUNF_SIGILL\n");
# ifdef SIGTRAP
	else if ( l == SIGTRAP )
		printf( "# define xCL_FUNF_SIGTRAP\n");
# endif
	else
	{
		x = HUGE / 10;

		/* try to underflow a double */
		for( i = 0, d = 1/x; i < 1000 ; i++ )
			d /= HUGE;
	
		/* if loop above didn't underflow, this conversion should. */
		f = d;

		/* if above left a nan, try something else */
		d = 1 / HUGE;
		d /= 2;
		f = d;

		printf("# define xCL_060_FUNF_UNDETECTED");
		if (print)
			printf("\t\t/* FUNF gives <%f> */", f);
		printf("\n");
	}
}

#ifdef _SGI_SOURCE
int
sgi_enable_fpe(void)
{
	union fpc_csr flp;
	flp.fc_struct.en_invalid = 0;
	flp.fc_struct.en_underflow = 1;
	flp.fc_struct.en_overflow = 1;
	flp.fc_struct.en_divide0 = 1;
	flp.fc_struct.en_inexact = 0;
	flp.fc_struct.se_invalid = 0;
	flp.fc_struct.se_divide0 = 0;
	flp.fc_struct.se_overflow = 0;
	flp.fc_struct.se_underflow = 0;
	flp.fc_struct.se_inexact = 0;
	return(set_fpc_csr(flp.fc_word));
}
#endif
