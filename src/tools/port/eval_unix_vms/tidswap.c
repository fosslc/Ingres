# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/tidswap.c,v 1.1 90/03/09 09:17:38 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>

/*
**	tidswap.c - Copyright (c) 2004 Ingres Corporation 
**
**	Are bit fields swapped or not?
**
**	Usage:	tidswap [-v]
**
**	Prints either "TID_SWAPPED" or " NOT TID_SWAPPED"
**
**	17-Jan-88  -- (pholman)  Ensure that the TID structure
**		reflects clhdr_unix/compat.h.
*/

static	int	vflag;
static	char	* myname;
static	char	* indent = "";

typedef unsigned int u_i4;
typedef int	       i4;

# define BITFLD u_i4

typedef struct
{
BITFLD line:9;
BITFLD page:23;
} TID;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	TID	tid;
	i4	*tidptr;
	int	swapped = 1;
	int	dummymax = 2147483392;


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

	/*
	**	Real stuff starts here
	**
	**	Initialize the i4 pointer we're using
	**	to set our bit fields
	*/

	tidptr = (i4 *) &tid;

	/*
	**	Test One
	*/

	*tidptr = 0;

	if (vflag)
		printf("%spage = %d\tline = %d\n", indent, tid.page, tid.line);

	/*
	**	Test Two
	*/

	*tidptr = 1;

	if (vflag)
		printf("%spage = %d\tline = %d\n", indent, tid.page, tid.line);

	if (tid.line)
		swapped = 0;

	/*
	**	Test Three
	*/

	*tidptr = 511;

	if (vflag)
		printf("%spage = %d\tline = %d\n", indent, tid.page, tid.line);

	if (tid.line)
		swapped = 0;

	/*
	**	Test Four
	*/

	*tidptr = 512;

	if (vflag)
		printf("%spage = %d\tline = %d\n", indent, tid.page, tid.line);

	if (tid.page == 1)
		swapped = 0;

	/*
	**	Test Five
	*/

	*tidptr = 2 * dummymax;

	if (vflag)
		printf("%spage = %d\tline = %d\n", indent, tid.page, tid.line);

	if (tid.page == 8388607)
		swapped = 0;

	if (swapped)
	{
		printf("\nTID_SWAPPED\n");
		exit(1);
	}

	printf("\nNOT TID_SWAPPED\n");
	exit(0);
}
