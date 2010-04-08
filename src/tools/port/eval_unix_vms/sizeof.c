# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/sizeof.c,v 1.1 90/03/09 09:17:37 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>

/*
**	sizeof.c - Copyright (c) 2004 Ingres Corporation 
**
**	Usage:  sizeof [char][double][float][int][long][ptr][short][-v]
**
**		char	show sizeof a char
**		double	show sizeof a double
**		float	show sizeof a float
**		int	show sizeof a integer
**		long	show sizeof a long
**		ptr	show sizeof a pointer
**		short	show sizeof a short
**
**		-v	verbose output
**
**	Non-verbose output is the sizeof the specified type in the form:
**
**		sizeof `type' `size'
**
**	where:	`type'		is a type selected on the command line
**		`size'		is the size as returned by sizeof(type)
**
**	By default, the sizeof everything is printed.
*/

static	int	vflag;
static	char	* myname;
static	char	*indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	int	cflag = 0;
	int	dflag = 0;
	int	fflag = 0;
	int	iflag = 0;
	int	lflag = 0;
	int	pflag = 0;
	int	sflag = 0;

	myname = argv[0];
	while(--argc)
	{
		++argv;
		if(!strcmp(*argv, "char"))
			cflag = 1;
		else if(!strcmp(*argv, "double"))
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
			fprintf(stderr, "Usage:  %s [char][double][float][int][long][ptr][short][-v]\n",
				myname);
			exit(1);
		}
	}
	if(!cflag && !dflag && !fflag && !iflag && !lflag && !pflag && !sflag)
		cflag = dflag = fflag = iflag = lflag = pflag = sflag = 1;

	if(vflag)
	{
		indent = "\t";
		printf("%s:\n", myname);
	}

	if(cflag)
		printf("%ssizeof char %d\n", indent, sizeof(char));

	if(sflag)
		printf("%ssizeof short %d\n", indent, sizeof(short));

	if(iflag)
		printf("%ssizeof int %d\n", indent, sizeof(int));

	if(lflag)
		printf("%ssizeof long %d\n", indent, sizeof(long));

	if(fflag)
		printf("%ssizeof float %d\n", indent, sizeof(float));

	if(dflag)
		printf("%ssizeof double %d\n", indent, sizeof(double));

	if(pflag)
		printf("%ssizeof ptr %d\n", indent, sizeof(char *));
		
	exit(0);
}
