#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/addrfmt.c,v 1.1 90/03/09 09:17:29 source Exp $";
#endif

# include	<stdio.h>
# include 	<stdlib.h>
# include       <generic.h>

/*
**	addrfmt.c -Copyright (c) 2004 Ingres Corporation 
**
**	Show raw pointer formats.
**
**	No -v option.  You need to stare at the results.
**
**	History
**
**	16-Nov-1993 (tad)
**		Bug #56449
**		Added #ifdef PRINTF_PERCENT_P cabability to distinguish
**		platforms that support printf %p format option and added
**		#include <generic.h> to pick this flag, if defined.
**		Replace %x with %p for pointer values where necessary.
**	28-feb-94 (vijay)
**		Remove extern defn.
*/

static int	vflag;
static char	* myname;

main(argc, argv)
	int	argc;
	char	**argv;
{
	
	char	* indent = "";
	char	c;
	short	s;
	int	i;
	long	l;
	float	f;
	double	d;

	char	* charp		= &c;
	short	* shortp	= &s;
	int	* intp		= &i;
	long	* longp		= &l;
	float	* floatp	= &f;
	double	* doublep	= &d;

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

# ifdef PRINTF_PERCENT_P
	printf("%schar *\t\t= %p\t0%o\n", indent, charp, charp);
	charp++;
	printf("%s++char *\t= %p\t0%o\n\n", indent, charp, charp);

	printf("%sshort *\t\t= %p\t0%o\n", indent, shortp, shortp);
	shortp++;
	printf("%s++short *\t= %p\t0%o\n\n", indent, shortp, shortp);

	printf("%sint *\t\t= %p\t0%o\n", indent, intp, intp);
	intp++;
	printf("%s++int *\t\t= %p\t0%o\n\n", indent, intp, intp);

	printf("%slong *\t\t= %p\t0%o\n", indent, longp, longp);
	longp++;
	printf("%s++long *\t= %p\t0%o\n\n", indent, longp, longp);

	printf("%sfloat *\t\t= %p\t0%o\n", indent, floatp, floatp);
	floatp++;
	printf("%s++float *\t= %p\t0%o\n\n", indent, floatp, floatp);

	printf("%sdouble *\t= %p\t0%o\n", indent, doublep, doublep);
	doublep++;
	printf("%s++double *\t= %p\t0%o\n", indent, doublep, doublep);
# else
	printf("%schar *\t\t= %x\t0%o\n", indent, charp, charp);
	charp++;
	printf("%s++char *\t= %x\t0%o\n\n", indent, charp, charp);

	printf("%sshort *\t\t= %x\t0%o\n", indent, shortp, shortp);
	shortp++;
	printf("%s++short *\t= %x\t0%o\n\n", indent, shortp, shortp);

	printf("%sint *\t\t= %x\t0%o\n", indent, intp, intp);
	intp++;
	printf("%s++int *\t\t= %x\t0%o\n\n", indent, intp, intp);

	printf("%slong *\t\t= %x\t0%o\n", indent, longp, longp);
	longp++;
	printf("%s++long *\t= %x\t0%o\n\n", indent, longp, longp);

	printf("%sfloat *\t\t= %x\t0%o\n", indent, floatp, floatp);
	floatp++;
	printf("%s++float *\t= %x\t0%o\n\n", indent, floatp, floatp);

	printf("%sdouble *\t= %x\t0%o\n", indent, doublep, doublep);
	doublep++;
	printf("%s++double *\t= %x\t0%o\n", indent, doublep, doublep);
# endif /* PRINTF_PERCENT_P */

	exit(0);
}
