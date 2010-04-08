#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/align.c,v 1.2 90/07/25 18:19:57 source Exp $";
#endif

# include	<stdio.h>
# include 	<stdlib.h>
# include	<signal.h>
# include	<setjmp.h>
# include	<generic.h>

/*
**	align.c - Copyright (c) 2004 Ingres Corporation 
**
**	Check hardware alignment for faults.
**
**	Usage:  align [double][float][int][long][ptr][short][-v]
**
**		double	alignment type for doubles
**		float	alignment type for floats
**		int	alignment type for integers
**		long	alignment type for longs
**		ptr	alignment type for pointers
**		short	alignment type for shorts
**
**		v	verbose output
**
**	Non-verbose output is the type required for alignment of
**	the selected type, in the form:
**
**		`type' align type: `datatype'
**
**	where:	`type' is the type selected by -type
**		`datatype' is the minimally restrictive type for alignment.
**
**	By default, the alignment type for everything is printed.
**
** History:
**	25-jul-90 (kirke)
**		Added missing t in \t for tab.
**	16-Nov-1993 (tad)
**		Bug #56449
**		Added #ifdef PRINTF_PERCENT_P cabability to distinguish
**		platforms that support printf %p format option and added
**		#include <generic.h> to pick this flag, if defined.
**		Replace %x with %p for pointer values where necessary.
*/

typedef	char * ptr;

static char	buff[BUFSIZ];
static char	* testbuf;
static char	* type = "no type";

static int 	i;
static int	errs;
static char	* myname;
static char	* indent = "";
static int	vflag;

static jmp_buf	env;

main(argc, argv)
	int	argc;
	char	**argv;
{
	char	*alignequiv();
	char	*errtype = "";
	int	toterrs;

	int	dflag = 0;
	int	fflag = 0;
	int	iflag = 0;
	int	lflag = 0;
	int	pflag = 0;
	int	sflag = 0;

	void	buserr();
	void	segerr();
	void	illerr();
	void	emterr();

	int	j;

	myname = argv[0];
	while(--argc)
	{
		if(!strcmp(*++argv, "double"))
			dflag = 1;
		else if(!strcmp(*argv, "float"))
			fflag = 1;
 		else if(!strcmp(*argv, "int"))
			iflag = 1;
 		else if(!strcmp(*argv, "long"))
			lflag = 1;
		else if(!strcmp(*argv, "ptr"))
			pflag = 1;
		else if(!strcmp(*argv, "short"))
			sflag = 1;
		else if(!strcmp(*argv, "-v"))
			vflag = 1;
		else
		{
			fprintf(stderr, "bad argument `%s'\n", *argv);
			fprintf(stderr, "Usage:  %s [double][float][int][long][ptr][short][v]\n",
				myname);
			exit(1);
		}
	}
	if(!dflag && !fflag && !iflag && !lflag && !pflag && !sflag)
		dflag = fflag = iflag = lflag = pflag = sflag = 1;

	if(vflag)
	{
		indent = "\t";
		printf("%s:\n", myname);
	}

	/*
	**	Set up environment
	*/
	toterrs = 0;
	(void) signal(SIGBUS, buserr);
	(void) signal(SIGSEGV, segerr);
	(void) signal(SIGILL, illerr);
# ifdef SIGDANGER
	/* AIX uses SIGDANGER instead */
	(void) signal(SIGDANGER, emterr);
# endif
# ifdef SIGEMT
	(void) signal(SIGEMT, emterr);
# endif

	/*
	**	Check chars
	*/
	errs = 0;
	type = "char";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(char);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(char *)testbuf = (char)0;

			for (j=0;j<sizeof(char);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	toterrs += errs;

	/*
	**	Check shorts
	*/
	errtype = "char";
	errs = 0;
	type = "short";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(short);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(short *)testbuf = (short)0;

			for (j=0;j<sizeof(short);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(errs)
		errtype = alignequiv(sizeof(short));
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	toterrs += errs;
	if(sflag)
		printf("%s%s align type: %s\n", indent, type, errtype);

	/*
	**	Check ints
	*/
	errs = 0;
	type = "int";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(int);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(int *)testbuf = (int)0;

			for (j=0;j<sizeof(int);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(errs)
		errtype = alignequiv(sizeof(int));
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	toterrs += errs;
	if(iflag)
		printf("%s%s align type: %s\n", indent, type, errtype);

	/*
	**	Check longs
	*/
	errtype = "char";
	errs = 0;
	type = "long";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(long);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(long *)testbuf = (long)0;

			for (j=0;j<sizeof(long);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(errs)
		errtype = alignequiv(sizeof(long));
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	toterrs += errs;
	if(lflag)
		printf("%s%s align type: %s\n", indent, type, errtype);

	/*
	**	Check floats
	*/
	errtype = "char";
	errs = 0;
	type = "float";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(float);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(float *)testbuf = (float)0;

			for (j=0;j<sizeof(float);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(errs)
		errtype = alignequiv(sizeof(float));
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	toterrs += errs;
	if(fflag)
		printf("%s%s align type: %s\n", indent, type, errtype);

	/*
	**	Check doubles
	*/
	errs = 0;
	type = "double";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(double);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(double *)testbuf = (double)0;

			for (j=0;j<sizeof(double);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(errs < sizeof(double) - 1)
		errtype = alignequiv(sizeof(float));
	else
		errtype = alignequiv(sizeof(double));
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	toterrs += errs;
	if(dflag)
		printf("%s%s align type: %s\n", indent, type, errtype);


	/*
	**	Check pointers
	*/
	errtype = "char";
	errs = 0;
	type = "ptr";
	if(vflag)
		printf("\tAlignment for type %s:\n", type);
	for(i = 0; i < sizeof(double); i++)
	{
		if(setjmp(env) == 0)
		{
			for (j=0;j<sizeof(char *);j++)
				buff[i+j] = 1;

			testbuf = &buff[i];
			*(char **)testbuf = (char *)0;

			for (j=0;j<sizeof(char *);j++)
				if (buff[i+j] != 0) {
					errs++;
					break;
				}
		}
	}
	if(errs)
		errtype = alignequiv(sizeof(type));
	if(vflag)
		printf("\t%d alignment errors for type %s\n\n",
			errs, type);
	if(pflag)
		printf("%s%s align type: %s\n", indent, type, errtype);

	/*
	**	Report totals
	*/
	if(vflag)
		printf("\t%d TOTAL alignment errors for all types\n",
			toterrs);

	exit(0);
}

void
buserr(sig)
{
	(void) signal(sig, buserr);
	errs++;
	if(vflag)
# ifdef PRINTF_PERCENT_P
	    printf("\t\tBus error: %s aligned %d (0x%p)\n", type, i, testbuf);
# else
	    printf("\t\tBus error: %s aligned %d (0x%x)\n", type, i, testbuf);
# endif /* PRINTF_PERCENT_P */
	longjmp(env, 1);
}


void
segerr(sig)
{
	(void) signal(sig, segerr);
	errs++;
	if(vflag)
# ifdef PRINTF_PERCENT_P
		printf("\t\tSegmentation violation: %s aligned %d (0x%p)\n",
			type, i, testbuf);
# else
		printf("\t\tSegmentation violation: %s aligned %d (0x%x)\n",
			type, i, testbuf);
# endif /* PRINTF_PERCENT_P */
	longjmp(env, 1);
}

void
illerr(sig)
{
	(void) signal(sig, illerr);
	errs++;
	if(vflag)
# ifdef PRINTF_PERCENT_P
		printf("\t\tIllegal instruction: %s aligned %d (0x%p)\n",
			type, i, testbuf);
# else
		printf("\t\tIllegal instruction: %s aligned %d (0x%x)\n",
			type, i, testbuf);
# endif /* PRINTF_PERCENT_P */
	longjmp(env, 1);
}

void
emterr(sig)
{
	(void) signal(sig, emterr);
	errs++;
	if(vflag)
# ifdef PRINTF_PERCENT_P
	    printf("\t\tEMT error: %s aligned %d (0x%p)\n", type, i, testbuf);
# else
	    printf("\t\tEMT error: %s aligned %d (0x%x)\n", type, i, testbuf);
# endif /* PRINTF_PERCENT_P */
	longjmp(env, 1);
}

char *
alignequiv(size)
{
	if(size == sizeof(char))
		return("char");

	else if(size == sizeof(short))
		return("short");

	else if(size == sizeof(int))
		return("int");

	else if(size == sizeof(long))
		return("long");

	else if(size == sizeof(float))
		return("float");

	else if(size == sizeof(double))
		return("double");

	else
		return("bad size in alignequiv");

}
