#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/argloc.c,v 1.1 90/03/09 09:17:30 source Exp $";
#endif

# include 	<stdlib.h>
# include	<stdio.h>
# include       <generic.h>

/*
**	argloc.c - Copyright (c) 2004 Ingres Corporation  
**
**	Where are arguments passed?
**
**	Usage:	argloc [-v]
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
**	18-jan-96 (morayf)
**		Turn off optimization for SNI RMx00 (rmx_us5). Was getting an
**		internal code (8091) from uopt in v1.1A of SNI C-DS compiler.
**	23-feb-96 (morayf)
**		Turn off optimization for Pyramid (pym_us5). Was getting an
**		internal code (8091) from uopt in v4.0 of Pyramid C compiler.
**	28-jul-1997 (walro03)
**		internal code (8091) from C compiler.
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	20-Aug-2002 (hanje04)
**	     Declare malloc() correctly as void not char to stop build errors
**	     under GLIBC 2.2
*/

/*
NO_OPTIM = rmx_us5 pym_us5 ts2_us5 rux_us5
*/

typedef	struct
{
	char	b_buff[BUFSIZ];
} BUFFER;

typedef struct
{
	unsigned int field1:4;
	unsigned int field2:4;
} BITFIELDS;

typedef struct
{
	int	integer;
} INTEGER;

typedef union
{
	char	charpart;
	short	shortpart;
	int	intpart;
	long	longpart;
	char	* ptrpart;
} ANYTYPE;

static int	vflag;
static char	* myname;
static char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int		i = 1;
	char *		s = NULL;
	BUFFER		buff;
	BITFIELDS	bits;
	INTEGER		integer;
	ANYTYPE		any;

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

	argloc(i, s, buff, &buff, bits, &bits, integer, &integer, any, &any);

	exit(0);
}

argloc(i, s, buff, buffptr, bits, bitsptr, integer, intptr, any, anyptr)
	int		i;
	char		*s;
	BUFFER		buff;
	BUFFER		*buffptr;
	BITFIELDS	bits;
	BITFIELDS	*bitsptr;
	INTEGER		integer;
	INTEGER		*intptr;
	ANYTYPE		any;
	ANYTYPE		*anyptr;
{
	void		* malloc();
	BUFFER		* newbuf;
	BITFIELDS	* newbits;
	INTEGER		* newint;
	ANYTYPE		* newany;

	printf("%sinteger arg:\n", indent);
# ifdef PRINTF_PERCENT_P
	printf("%s\t&i\t\t= 0x%p\n\n", indent, &i);

	printf("%schar * arg:\n", indent);
	printf("%s\t&s\t\t= 0x%p\ts\t= 0x%x\n\n", indent, &s, s);

	printf("%sstructure with char array:\n", indent);
	printf("%s\t&buff\t\t= 0x%p\n", indent, &buff);
	printf("%s\t&buffptr\t= 0x%p\tbuffptr\t= 0x%p\n",
		indent, &buffptr, buffptr);
	newbuf = (BUFFER *) malloc(sizeof(BUFFER));
	printf("%s\t&newbuf\t\t= 0x%p\tnewbuf\t= 0x%p\n\n",
		indent, &newbuf, newbuf);

	printf("%sstructure with small bitfields:\n", indent);
	printf("%s\t&bits\t\t= 0x%p\n", indent, &bits);
	printf("%s\t&bitsptr\t= 0x%p\tbitsptr\t= 0x%p\n",
		indent, &bitsptr, bitsptr);
	newbits = (BITFIELDS *)malloc(sizeof(BITFIELDS));
	printf("%s\t&newbits\t= 0x%p\tnewbits\t= 0x%p\n\n",
		indent, &newbits, newbits);

	printf("%sstructure with one int:\n", indent);
	printf("%s\t&integer\t= 0x%p\n", indent, &integer);
	printf("%s\t&intptr\t\t= 0x%p\tintptr\t= 0x%p\n",
		indent, &intptr, intptr);
	newint = (INTEGER *)malloc(sizeof(INTEGER));
	printf("%s\t&newint\t\t= 0x%p\tnewint\t= 0x%p\n\n",
		indent, &newint, newint);

	printf("%sany type union:\n", indent);
	printf("%s\t&any\t\t= 0x%p\n", indent, &any);
	printf("%s\t&anyptr\t\t= 0x%p\tanyptr\t= 0x%p\n",
		indent, &anyptr, anyptr);
	newany = (ANYTYPE *)malloc(sizeof(ANYTYPE));
	printf("%s\t&newany\t\t= 0x%p\tnewany\t= 0x%p\n\n",
		indent, &newany, newany);
# else
	printf("%s\t&i\t\t= 0x%x\n\n", indent, &i);

	printf("%schar * arg:\n", indent);
	printf("%s\t&s\t\t= 0x%x\ts\t= 0x%x\n\n", indent, &s, s);

	printf("%sstructure with char array:\n", indent);
	printf("%s\t&buff\t\t= 0x%x\n", indent, &buff);
	printf("%s\t&buffptr\t= 0x%x\tbuffptr\t= 0x%x\n",
		indent, &buffptr, buffptr);
	newbuf = (BUFFER *) malloc(sizeof(BUFFER));
	printf("%s\t&newbuf\t\t= 0x%x\tnewbuf\t= 0x%x\n\n",
		indent, &newbuf, newbuf);

	printf("%sstructure with small bitfields:\n", indent);
	printf("%s\t&bits\t\t= 0x%x\n", indent, &bits);
	printf("%s\t&bitsptr\t= 0x%x\tbitsptr\t= 0x%x\n",
		indent, &bitsptr, bitsptr);
	newbits = (BITFIELDS *)malloc(sizeof(BITFIELDS));
	printf("%s\t&newbits\t= 0x%x\tnewbits\t= 0x%x\n\n",
		indent, &newbits, newbits);

	printf("%sstructure with one int:\n", indent);
	printf("%s\t&integer\t= 0x%x\n", indent, &integer);
	printf("%s\t&intptr\t\t= 0x%x\tintptr\t= 0x%x\n",
		indent, &intptr, intptr);
	newint = (INTEGER *)malloc(sizeof(INTEGER));
	printf("%s\t&newint\t\t= 0x%x\tnewint\t= 0x%x\n\n",
		indent, &newint, newint);

	printf("%sany type union:\n", indent);
	printf("%s\t&any\t\t= 0x%x\n", indent, &any);
	printf("%s\t&anyptr\t\t= 0x%x\tanyptr\t= 0x%x\n",
		indent, &anyptr, anyptr);
	newany = (ANYTYPE *)malloc(sizeof(ANYTYPE));
	printf("%s\t&newany\t\t= 0x%x\tnewany\t= 0x%x\n\n",
		indent, &newany, newany);
# endif /* PRINTF_PERCENT_P */

	free(newbuf);
	free(newbits);
	free(newint);
	free(newany);
}
