# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/maxstack.c,v 1.1 90/03/09 09:17:34 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>
# include	<setjmp.h>

/*
**	maxstack.c - Copyright (c) 2004 Ingres Corporation 
**
**	How big can the stack get?
**
**	Usage:	maxstack [-v][-x]
**
**		-v	`verbose'
**		-x	`extra-verbose'
**
**	Prints either "1024kB" or some smaller limited value.
*/

/* step size */
# define	INCREMENT	1024

/* 100k should be plenty deep enough */
# define	MAXTEST		100 * INCREMENT

static	long	size;
static	jmp_buf	env;

static	char	* myname;
static	int	vflag = 0;
static	int	xflag = 0;
static	char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	void	buserr();
	void	segerr();
	void	illerr();
#	ifdef SIGSTACK
	void	stackerr();
#	endif

	myname = argv[0];
	while(--argc)
	{
		if(!strcmp(*++argv, "-v"))
			vflag = 1;
		else if(!strcmp(*argv, "-x"))
			xflag = vflag = 1;
		else
		{
			fprintf(stderr, "bad argument `%s'\n", *argv);
			fprintf(stderr, "Usage:  %s [-v][-x]\n", myname);
			exit(1);
		}
	}
	if(vflag)
	{
		indent = "\t";
		printf("%s:\n", myname);
	}

	(void) signal(SIGBUS, buserr);
	(void) signal(SIGSEGV, segerr);
	(void) signal(SIGILL, illerr);
#	ifdef SIGSTACK
	(void) signal(SIGSTACK, stackerr);
#	endif

	if(setjmp(env) == 0)
	{
		stackdepth(MAXTEST/INCREMENT);
	}
	printf("%s%dkB\n", indent, size/1024);
	exit(0);
}

stackdepth(levels)
	int	levels;
{
	char	space[INCREMENT];
	
	if(xflag != 0)
		printf("\tsize %dkB\n", size/1024);

	/* bump global size count */
	size +=	INCREMENT;

	/* recurse to add depth to the stack */
	if(--levels > 0)
		stackdepth(levels);
}


void
buserr(sig)
	int sig;
{
	if(vflag)
		printf("\tSIGBUS, signal %d\n", sig);
	longjmp(env, 1);
}


void
segerr(sig)
	int sig;
{
	if(vflag)
		printf("\tSIGSEGV, signal %d\n", sig);
	longjmp(env, 1);
}


void
illerr(sig)
	int sig;
{
	if(vflag)
		printf("\tSIGILL, signal %d\n", sig);
	longjmp(env, 1);
}



# ifdef SIGSTACK
void
stackerr(sig)
	int sig;
{
	if(vflag)
		printf("\tSIGSTACK, signal %d\n", sig);
	longjmp(env, 1);
}
# endif
