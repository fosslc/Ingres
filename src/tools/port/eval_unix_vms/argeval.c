#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/argeval.c,v 1.1 90/03/09 09:17:30 source Exp $";
#endif

# include 	<stdlib.h>
# include	<stdio.h>

/*
**	argeval.c -Copyright (c) 2004 Ingres Corporation 
**
**	In what order are args evaluated?
**
**	Usage:	argeval [-v]
**
**	prints the words "left to right" or "right to left".
**
**	-v "verbose" option prints some internal information.
**  History:
**	09-Mar-99 (schte01)
**	    Changed argorder call so there is no dependency on order of evaluation.
**	17-Mar-99 (schte01)
**	    Backed out previous change since function depends on ambiguity.
*/

static int	vflag;
static char	* myname;
static char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	int	i = 0;

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

	argorder(i++, i++);
	exit(0);
}

argorder(a, b)
	int	a, b;
{
	if(vflag)
		printf("\ta = %d, b = %d\n", a, b);

	if(a < b)
		printf("%sleft to right\n", indent);
	else
		printf("%sright to left\n", indent);
}
