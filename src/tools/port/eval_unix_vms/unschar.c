# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/unschar.c,v 1.1 90/03/09 09:17:39 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>

/*
**	unschar.c - Copyright (c) 2004 Ingres Corporation
**
**	Are chars unsigned?
**
**	Usage:	unschar [-v]
**
**	Prints either "signed" or "unsigned"
*/

static	int	vflag;
static	char	* myname;
static	char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	char	c;
	int	i;

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

	c = (char)~0;
	i = c;
	if(vflag)
	{
		printf("\tchar, all ones, printed as %%d == %d\n", c);
		printf("\tint = c == %d\n", i);
	}

	printf("%s%s\n", indent, c < 0 ? "signed" : "unsigned");	
	exit(0);
}
