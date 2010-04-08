# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/salign.c,v 1.1 90/03/09 09:17:37 source Exp $";
# endif

# include	<stdlib.h>
# include	<stdio.h>

/*
**	salign.c - Copyright (c) 2004 Ingres Corporation 
**
**	Show structure alignment restrictions.
**
**	Usage:  salign [char][double][float][int][long][ptr][short][-v]
**
**		char	structure alignment for chars
**		double	structure alignment for doubles
**		float	structure alignment for floats
**		int	structure alignment for integers
**		long	structure alignment for longs
**		ptr	structure alignment for pointers
**		short	structure alignment for shorts
**
**		-v	verbose output
**
**	Non-verbose output is the type required for alignment of
**	the selected type, in the form:
**
**		`type': sizeof_struct `struct_size', pad `pad_amount'
**
**	where:	`type'		is a type selected on the command line.
**		`struct_size'	is the size of the structure
**		`pad_amount'	is the amount of padding in the structure.
**
**	By default, the structure alignment for everything is printed.
**      
**	Mar 6 00 wansh01 included generic.h.
**                       added #if for rmx_us5 for typedef ss_char. 
**                       s_char is defined in the sys/types.h        
**	May 19  wansh01  redo Mar 6's changes to make it generic 
*/


typedef struct
{
        char    a;
	char	b;
} ii_s_char;

typedef struct
{
	char	a;
	short	b;
} s_short;

typedef struct
{
	char	a;
	int	b;
} s_int;

typedef struct
{
	char	a;
	long	b;
} s_long;

typedef struct
{
	char	a;
	float	b;
} s_float;

typedef struct
{
	char	a;
	double	b;
} s_double;

typedef struct
{
	char	a;
	char *	b;
} s_ptr;


static	int	vflag;
static	char	* myname;
static	char	* indent = "";

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
		printf("%schar: sizeof_struct %d, pad %d\n", indent,
			sizeof(ii_s_char),
			sizeof(ii_s_char) - sizeof(char) - sizeof(char));

	if(sflag)
		printf("%sshort: sizeof_struct %d, pad %d\n", indent,
			sizeof(s_short),
			sizeof(s_short) - sizeof(char) - sizeof(short));

	if(iflag)
		printf("%sint: sizeof_struct %d, pad %d\n", indent,
			sizeof(s_int),
			sizeof(s_int) - sizeof(char) - sizeof(int));

	if(lflag)
		printf("%slong: sizeof_struct %d, pad %d\n", indent,
			sizeof(s_long),
			sizeof(s_long) - sizeof(char) - sizeof(long));

	if(fflag)
		printf("%sfloat: sizeof_struct %d, pad %d\n", indent,
			sizeof(s_float),
			sizeof(s_float) - sizeof(char) - sizeof(float));

	if(dflag)
		printf("%sdouble: sizeof_struct %d, pad %d\n", indent,
			sizeof(s_double),
			sizeof(s_double) - sizeof(char) - sizeof(double));

	if(pflag)
		printf("%sptr: sizeof_struct %d, pad %d\n", indent,
			sizeof(s_ptr),
			sizeof(s_ptr) - sizeof(char) - sizeof(char *));

	exit(0);
}
