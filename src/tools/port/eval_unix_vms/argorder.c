#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/argorder.c,v 1.1 90/03/09 09:17:31 source Exp $";
#endif

# include 	<stdlib.h>
# include	<stdio.h>
# include       <generic.h>

/*
**	argorder.c - Copyright (c) 2004 Ingres Corporation 
**
**	What order are args put on the stack?
**
**	Usage:	argorder [-v]
**
**	prints one of the strings: "left above right" or "right above left"
**	
**	WARNING:
**		This version assumes you can take the address of arguments:
**		Not true if they've been put in registers by the compiler.
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
static char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	
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

	argorder(1, 2);
	exit(0);
}

argorder(left, right)
	int	left, right;
{
	if(vflag)
	{
# ifdef PRINTF_PERCENT_P
		printf("\t&left = 0x%p\n", &left);
		printf("\t&right = 0x%p\n", &right);
# else
		printf("\t&left = 0x%x\n", &left);
		printf("\t&right = 0x%x\n", &right);
# endif /* PRINTF_PERCENT_P */
		printf("\t(Consult the stack order to interpret this.)\n");
	}
	printf("%s%s\n", indent,
		&left > &right ? "left above right" : "right above left");
}
