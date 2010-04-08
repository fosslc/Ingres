# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/nullderef.c,v 1.1 90/03/09 09:17:35 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>
# include	<setjmp.h>

/*
**	nullderef.c - Copyright (c) 2004 Ingres Corporation 
**
**	What happens when you dereference through a null pointer?
**
**	Usage:	nullderef [-v]
**
**	Prints either "OK" or "Illegal"
**
*/

static	jmp_buf	env;
static	int	vflag;
static	char	* myname;
static	char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	char	*s = NULL;
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

	if(setjmp(env) == 0)
	{
		if(vflag)
			printf("\tchar @NULL = %d\n", *s);
		printf("%sOK\n", indent);
	}
	else
	{
		printf("%sIllegal\n", indent);
	}

	exit(0);
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


