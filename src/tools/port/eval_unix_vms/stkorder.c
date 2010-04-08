#ifndef lint
static char rcsid[] = 
	"$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/stkorder.c,v 1.1 90/03/09 09:17:37 source Exp $";
#endif

# include	<stdlib.h>
# include	<stdio.h>
# include       <generic.h>

/*
**	stackorder.c - Copyright (c) 2004 Ingres Corporation 
**
**	What direction does the stack grow?
**
**	Usage:	stackorder [-v]
**
**	prints either "high to low" or "low to high"
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
static char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	char	buff[BUFSIZ];
	
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

	stackorder(buff);
	exit(0);
}

stackorder(callerbuf)
	char	*callerbuf;
{
	char	buff[BUFSIZ];
	
	if(vflag)
	{
# ifdef PRINTF_PERCENT_P
		printf("\tCaller's buffer\t@ 0x%p\n", callerbuf);
		printf("\tMy buffer\t@ 0x%p\n", buff);
# else
		printf("\tCaller's buffer\t@ 0x%x\n", callerbuf);
		printf("\tMy buffer\t@ 0x%x\n", buff);
# endif /* PRINTF_PERCENT_P */
	}
	if(buff > callerbuf)
		printf("%slow to high\n", indent);
	else
		printf("%shigh to low\n", indent);
}
