# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/maxvals.c,v 1.2 90/07/20 13:48:41 source Exp $";
# endif

/*
** History:
**	20-jul-1990 (chsieh)
**		add # include <sys/types.h> for system V. 
**		Otherwise, fcntl.h may complain.
**
**		add # ifndef SYS5 to get rid of SYS5 multiple definition warning
*/

# include	<stdlib.h>
# include	<stdio.h>
# include	<sys/types.h>
# include	<fcntl.h>

# ifndef 	SYS5

# ifdef		FNDELAY
# define	V7
# else
# define	SYS5
# endif

# endif		/* SYS5 */
/*
**	maxvals.c - Copyright (c) 2004 Ingres Corporation 
**
**	show limits of integral types
**
**	Usage:  maxvals [char][int][long][short][-v]
**
**		char	min/max values for chars
**		int	min/max values for integers
**		long	min/max values for longs
**		short	min/max values for shorts
**
**		-v	verbose output
**
**	Non-verbose output produces the minimum and maximum values
**	that can be assigned to the selected type, both signed and unsigned.
**
**		type:  max_unsigned_value, min_value, max_value
**
**	where:	`type' 			is one of: {char|int|long|short}.
**		`max_unsigned_value'	is the highest value `unsigned type'
**					may hold.
**		`min_value'		is the smallest value `type' may hold.
**		`max_value'		is the biggest value `type' may hold.
**
x**	By default, ouput is non-verbose for all types.
*/

static char	* indent = "";
static char	* myname;
static int	vflag;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	bits;

	int	cflag = 0;
	int	iflag = 0;
	int	lflag = 0;
	int	sflag = 0;

	int	uflag;		/* unsigned chars? */

	char	c, maxc;
	short	s, maxs;
	unsigned short us;
	int	i, maxi;
	long	l, maxl;

	myname = argv[0];
	while(--argc)
	{
		if(!strcmp(*++argv, "char"))
			cflag = 1;
 		else if(!strcmp(*argv, "int"))
			iflag = 1;
 		else if(!strcmp(*argv, "long"))
			lflag = 1;
		else if(!strcmp(*argv, "short"))
			sflag = 1;
		else if(!strcmp(*argv, "-v"))
			vflag = 1;
		else
		{
			fprintf(stderr, "bad argument `%s'\n", *argv);
			fprintf(stderr, "Usage:  %s [char][int][long][short][-v]\n",
				myname);
			exit(1);
		}
	}
	if(!cflag && !iflag && !lflag && !sflag)
		cflag = iflag = lflag = sflag = 1;

	if(vflag)
	{
		indent = "\t";
		printf("%s:\n", myname);
	}
	
	/*
	**	chars can be signed, unsigned or both
	*/
	if(cflag)
	{
		int	x3b_bug;	/* 3b compiler bug */

		/* find number of bits; left shift to avoid sign extension */
		bits = 0;
		c = (char)~0;
		x3b_bug = c;
		uflag = x3b_bug >= 0;
		for(bits = 0; c; bits++ )
			c <<= 1;
		maxc = (char)~(1 << bits - 1);
		if(vflag)
			printf("\tchar has %d bits\n", bits);
		printf("%schar:", indent);
		printf(" %u,", (unsigned char)~0);
		if(uflag)
		{
			printf(" %d,", 0);
			c = ~0;
			printf(" %d", (unsigned char)c);
		}
		else
		{
			c = (1 << bits - 1);
			printf(" %d,", c);
			printf(" %d", maxc);
		}
		printf("\n");
	}	

	/*
	**	Shorts
	*/
	if(sflag)
	{
		bits = 0;
		for(s = (short)~0; s; s <<= 1)
			bits++;
		maxs = (short)~(1 << bits - 1);
		if(vflag)
			printf("\tshort has %d bits\n", bits);
		printf("%sshort:", indent);
		us = ~0;
		printf(" %u,", us);
		s = (1 << bits -1);		
		printf(" %d,", s);
		printf(" %d", maxs);
		printf("\n");
	}

	/*
	**	ints
	*/
	if(iflag)
	{
		bits = 0;
		for(i = (int)~0; i; i <<= 1)
			bits++;
		maxi = (int)~(1 << bits - 1);
		if(vflag)
			printf("\tint has %d bits\n", bits);
		printf("%sint:", indent);
		printf(" %u,", (unsigned int)~0);
		i = 1 << bits -1;
		printf(" %d,", i);
		printf(" %d", maxi);
		printf("\n");
	}

	/*
	**	Longs
	*/
	if(lflag)
	{
		bits = 0;
		for(l = (long)~0L; l; l <<= 1)
			bits++;
		maxl = (long)~(1L << bits - 1);
		if(vflag)
			printf("\tlong has %d bits\n", bits);
		printf("%slong:", indent);
		l = 1L << bits - 1;
# ifdef SYS5
		/* System V printf */
		printf(" %lu,", (unsigned long)~0);
		printf(" %ld,", l);
		printf(" %ld", maxl);
# endif
# ifdef V7
		/* includes BSD */
		printf(" %U,", (unsigned long)~0);
		printf(" %D,", l);
		printf(" %D", maxl);
# endif
		printf("\n");
	}

	exit(0);
}
