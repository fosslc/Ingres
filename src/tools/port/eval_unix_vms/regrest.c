#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/regrest.c,v 1.1 90/03/09 09:17:36 source Exp $";
#endif

# include	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>
# include	<setjmp.h>

/*
**	regrest.c - Copyright (c) 2004 Ingres Corporation 
**
**	Check register restoration on return from longjmp().
**
**	Usage:  regrest [-v]
**
**		v	verbose output
**
*/

# define	VALUE1		10
# define	VALUE2		20

char	*myname;
char	*indent = "";
int	vflag = 0;

char	*addr1;
char	*addr2;

jmp_buf		env;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	address = 0;
	int	data = 0;
	int	exit_val = 0;
	void	handler();

	register char	*reg1;
	register int	reg2;
	register char	*reg3;
	register int	reg4;

	myname = argv[0];
	if (argc > 1)
	{
		if(!strcmp(argv[1], "-v"))
			vflag = 1;
		else
		{
			fprintf(stderr, "bad argument `%s'\n", argv[1]);
			fprintf(stderr, "Usage:  %s [-v]", myname);
			exit(1);
		}
	}

	if(vflag)
	{
		indent = "\t";
		printf("%s:\n", myname);
	}

	/*
	**	Set up environment
	*/

	(void) signal(SIGBUS, handler);
	reg1 = (char *)&addr1;
	reg2 = VALUE1;
	reg3 = (char *)&addr2;
	reg4 = VALUE2;

	/*
	**	Print initial register values
	*/

	if (vflag)
	{
		printf("%sinitial register values\n", indent);
		printf("%sreg1 = %d (0x%x)\n", indent, reg1, reg1);
		printf("%sreg2 = %d (0x%x)\n", indent, reg2, reg2);
		printf("%sreg3 = %d (0x%x)\n", indent, reg3, reg3);
		printf("%sreg4 = %d (0x%x)\n\n", indent, reg4, reg4);
	}

	if (setjmp(env))
	{
		if (vflag)
		{
			printf("%sAfter returning from longjmp()\n", indent);
			printf("%sreg1 = %d (0x%x)\n", indent, reg1, reg1);
			printf("%sreg2 = %d (0x%x)\n", indent, reg2, reg2);
			printf("%sreg3 = %d (0x%x)\n", indent, reg3, reg3);
			printf("%sreg4 = %d (0x%x)\n\n", indent, reg4, reg4);
		}
		
		if ((reg1 != (char *)&addr1) || (reg3 != (char *)&addr2))
		{
			printf("%saddress registers NOT restored\n", indent);
			address = 1;
			exit_val = 1;
		}
		if ((reg2 != VALUE1) || (reg4 != VALUE2))
		{
			printf("%sdata registers NOT restored\n", indent);
			data = 1;
			exit_val = 1;
		}
			
		if (!address && !data)
			printf("%sAll registers restored\n", indent);	

		exit(exit_val);
	}

	/*
	**	New values for data registers.
	*/

	reg2 = VALUE1 * 10;
	reg4 = VALUE2 * 10;

	/*
	**	New values for address registers.
	*/

	reg1 = (char *)0;
	reg3 = (char *)0;

	/*
	**	Send signal to do longjmp.
	*/

	kill(getpid(), SIGBUS);

	/*
	**	Error if got here.
	*/

	fprintf(stderr, "%sSignal missed!\n", indent);
	exit(1);
}


void
handler(sig)
{
	(void) signal(sig, handler);
	if(vflag)
		printf("\t\tSignal: %d\n\n", sig);
	longjmp(env, 1);
}
