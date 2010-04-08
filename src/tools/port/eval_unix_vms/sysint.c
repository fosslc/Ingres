# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/sysint.c,v 1.1 90/03/09 09:17:38 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>
# include	<errno.h>
# include	<setjmp.h>
# include	<generic.h>

/*
**	sysint.c - Copyright (c) 2004 Ingres Corporation 
**
**	Find out if system calls are interruptable.
**
**	Usage:	sysint [-v]
**	
**	Should print "OK".  It it doesn't, you're in trouble!
**
**	Author: Dave Brower
**
**	Note that under BSD, you must longjmp out of the signal
**	handler, otherwise you go back into the kernal and hang.
**	You will NEVER get EINTR on BSD directly from read or ioctl.
**
**	History:
**
**      01-feb-93 (smc)
**      	Tidied up signal handler functions to be TYPESIG (*)().
**	28-feb-94 (vijay)
**		Remove extern defn.
*/

# define	TIMEOUT		5

static	jmp_buf	env;
static	int	vflag;
static	char	* myname;
static	char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	extern int	errno;
	
	char	buff[BUFSIZ];
	int	pipes[2];
	int	stat;	
	int	jumped;

	TYPESIG	timeout();
	int	intr();
		
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
		(void) fflush(stdout);
	}

	if((stat = pipe(pipes)) == -1)
	{
		perror("opening pipes");
		exit(1);
	}

	/* 
	 * Set alarm in 5 seconds, hang in
	 * read on empty open pipe.  Alarm should
	 * bounce you out, with errno == EINTR.
	 */
	if(vflag)
	{
		printf("\thanging, timing out in %d seconds\n", TIMEOUT);
		(void) fflush(stdout);
	}

	(void) signal(SIGALRM, timeout);
	alarm(TIMEOUT);

	stat = -1;
	if((jumped = setjmp(env) == 0))
	{
		stat = read(pipes[0], buff, 1);
		(void) signal(SIGALRM, SIG_DFL);
	}

	if(vflag)
	{
		printf("\ttimeleft == %d\n", alarm(0));
		(void) fflush(stdout);
	}

	if(jumped || stat != 1)
	{
		printf("%sOK\n", indent);
	}
	else
	{
		printf("read returned - shouldn't have happened!!\n");
		printf("errno == %d\n", errno);
		(void) fflush(stdout);
		exit(1);
	}

	exit(0);
}

TYPESIG timeout(sig)
	int	sig;
{
	if(vflag)
	{
		printf("\tSIGALRM handler\n");
		(void) fflush(stdout);
	}
	errno = EINTR;
	longjmp(env, sig);
}
