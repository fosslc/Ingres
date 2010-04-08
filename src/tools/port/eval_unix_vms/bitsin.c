# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/bitsin.c,v 1.1 90/03/09 09:17:31 source Exp $";
# endif

# include 	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>

/* if SIGUSR1 exists, we're on a System V, otherwise assume BSD */

/*
**	bitsin.c - Copyright (c) 2004 Ingres Corporation 
**
**	show limits of integral types
**
**	Usage:  bitsin [char][int][long][longlong][ptr][short][-v]
**
**		char		number of bits in a char
**		int		number of bits in a integer
**		long		number of bits in a long
**		longlong	number of bits in a long long
**		ptr		number of bits in a ptr
**		short		number of bits in a short
**
**		-v	verbose output
**
**	Non-verbose output produces the minimum and maximum values
**	that can be assigned to the selected type, both signed and unsigned.
**
**		type: nbits
**
**	where:	`type' 		is one of: {char|int|long|ptr|short}.
**		`nbits'		is the number of bits in the type
**
x**	By default, ouput is non-verbose for all types.
**
**  History:
**	03-mar-1999 (walro03)
**	    Add support for longlong, which tells the number of bits in a
**	    long long.
**      15-Jan-2002 (hanal04) Bug 106820.
**          Removed above change for 2.5/0011 (sos.us5/00) as the
**          compiler (SCO UNIX Development System  Release 5.1.2A 27Jul00) 
**          does not recognise the type 'long long'.
**	24-May-2002 (hanch04)
**	    Back out sco.us5 change. mkbzarch.sh needs bitsin to run,
**	    so bitsin cannot include compat.h which includes bzarch.h,
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
	int	llflag = 0;
	int	pflag = 0;
	int	sflag = 0;

	char		c;
	short		s;
	int		i;
	long		l;
	long long	ll;
	char		* p;

	myname = argv[0];
	while(--argc)
	{
		if(!strcmp(*++argv, "char"))
			cflag = 1;
 		else if(!strcmp(*argv, "int"))
			iflag = 1;
 		else if(!strcmp(*argv, "longlong"))
			llflag = 1;
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
			fprintf(stderr, "Usage: %s [char][int][long][longlong][ptr][short][-v]\n",
				myname);
			exit(1);
		}
	}
	if(!cflag && !iflag && !lflag 
          && !llflag 
          && !pflag && !sflag)
        {
            cflag = iflag = lflag = pflag = sflag = 1;
            llflag = 1;
        }

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
		/* find number of bits; left shift to avoid sign extension */
		bits = 0;
		for(c = (char)~0; c; c <<= 1)
			bits++;
		printf("%schar: %d\n", indent, bits);
	}	

	/*
	**	Shorts
	*/
	if(sflag)
	{
		bits = 0;
		for(s = (short)~0; s; s <<= 1)
			bits++;
		printf("%sshort: %d\n", indent, bits);
	}

	/*
	**	ints
	*/
	if(iflag)
	{
		bits = 0;
		for(i = (int)~0; i; i <<= 1)
			bits++;
		printf("%sint: %d\n", indent, bits);
	}

	/*
	**	Longs
	*/
	if(lflag)
	{
		bits = 0;
		for(l = (long)~0L; l; l <<= 1)
			bits++;
		printf("%slong: %d\n", indent, bits);
	}


	/*
	**	Longlongs
	*/
	if(llflag)
	{
		bits = 0;
		for(ll = (long long)~0LL; ll; ll <<= 1)
			bits++;
		printf("%slong long: %d\n", indent, bits);
	}


	/*
	**	Pointers
	**
	**	Makes some ugly assumtions about ptr/long
	**	conversions.
	*/
	if(pflag)
	{
		l = (long)~0L;
		for(bits = 0; l && bits < 256; bits++)
		{
			l <<= 1;
			p = (char *)l;
			l = (long)p;
		}		

		printf("%sptr: %d\n", indent, bits);
	}


	exit(0);
}
