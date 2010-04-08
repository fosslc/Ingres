# ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/endian.c,v 1.1 90/03/09 09:17:32 source Exp $";
# endif

# include 	<stdlib.h>
# include	<stdio.h>

/*
**	endian.c - Copyright 2004, Ingres Corporation
**
*/

static int		hexval = (int)0x01020304;
static unsigned char	lit_end[] = { 0x04, 0x03, 0x02, 0x01 };
static unsigned char	big_end[] = { 0x01, 0x02, 0x03, 0x04 };

main(argc, argv)
    int	argc;
    char	**argv;
{
    char	*chrp = (char *)&hexval;
    char	*myname = argv[0];
    char	*indent = "";
    int		vflag = 0;

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

    if(vflag)
    {
	printf("\tlittle:\t%d%d%d%d\n", 
	    lit_end[0], lit_end[1], lit_end[2], lit_end[3]);
	printf("\tbig:\t%d%d%d%d\n", 
	    big_end[0], big_end[1], big_end[2], big_end[3]);
	printf("\tlocal:\t%d%d%d%d\n", 
	    chrp[0], chrp[1], chrp[2], chrp[3]);
    }

    if ( chrp[0] == lit_end[0] && chrp[1] == lit_end[1] &&
	 chrp[2] == lit_end[2] && chrp[3] == lit_end[3] )
	printf("%slittle\n", indent);
    else  if ( chrp[0] == big_end[0] && chrp[1] == big_end[1] &&
	       chrp[2] == big_end[2] && chrp[3] == big_end[3] )
	printf("%sbig\n", indent);
    else
	printf("%sother\n", indent);

    exit(0);
}
