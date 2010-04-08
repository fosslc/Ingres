
# include 	<stdlib.h>
# include	<stdio.h>
# include	<signal.h>
# include	<setjmp.h>

/*
**	maxfiles.c - Copyright (c) 2004 Ingres Corporation 
**
**	How many files can we open?
**
**	Usage:	maxfiles [-v]
**
**		-v	`verbose'
**
**	Prints the number of fd's we can open.
*/

static	char	* myname;
static	int	vflag = 0;
static	char	* indent = "";

main(argc, argv)
	int	argc;
	char	**argv;
{
	int fd;
	int i;

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

	for( i = 3; (fd = open( "/etc/passwd", 0 ) ) >= 0 ; i++ )
		if(vflag)
			printf("opened fd %d, n %d\n", fd, i + 1 );

	if(vflag)
	{
		fprintf(stderr, "fd %d: ", fd);
		perror("");
	}
		
	printf("%d\n", i );
	exit(0);
}

