# include	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>
# include	<setjmp.h>
# include	<unistd.h>

/*
**	maxproc.c - Copyright (c) 2004 Ingres Corporation 
**
**	How big can a process get?
**
**	Usage:	maxproc [-v]
**
**		-v	`verbose'
**
**	Prints either "2000kB" or some smaller limited value.
*/

/* step size */
# define	INCREMENT	2048

/* 100k should be plenty deep enough */
# define	MAXTEST		1000 * INCREMENT

static	long	size;
static	jmp_buf	env;

static	char	*myname;
static	int	vflag = 0;
static	char	*indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	i;
	int	strcmp();
	void	buserr();
	void	segerr();
	void	illerr();

	myname = argv[0];
	while(--argc)
	{
		if(!strcmp(*++argv, "-v"))
			vflag = 1;
		else
		{
			fprintf(stderr, "bad argument `%s'\n", *argv);
			fprintf(stderr, "Usage:  %s [-v]\n", myname);
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

	if(setjmp(env))
	{
		exit(1);
	}

	growproc(MAXTEST/INCREMENT);
	printf("%s%dkB\n", indent, size/1024);

	exit(0);
}

growproc(maxpages)
	int	maxpages;
{
	char	*space;
	int	i;

	if ((space = sbrk(MAXTEST)) == (char *)-1)
	{
		if (vflag)
			printf("%sFailed allocating %d bytes\n",
				indent, MAXTEST);
	}
	else
	{
		size = MAXTEST;
		return;
	}

	for (i = 0; i < maxpages; i++)
		if ((space = sbrk(INCREMENT)) == (char *)-1)
		{
			if (vflag)
				printf("%sFailed after allocating %d bytes\n",
					indent, (INCREMENT * (i - 1)));
			break;

		}

	size = i * INCREMENT;
	return;
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
