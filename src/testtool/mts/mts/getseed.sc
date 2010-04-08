/*
**	Program to add a "seed" value to the table ``seeds''.
**	This "seed" is used by MTS to determine which operations
**	to perform.
**
** 	bh	06/25/92	added libraries for ming - NEEDLIBS
*/


/* ming hints 
**
NEEDLIBS = MTSLIB LIBINGRES
**
*/

main (argc,argv)
int  	argc;
char    *argv[];
{
	exec sql include sqlca;
	exec sql whenever sqlerror call sqlprint;
	exec sql whenever sqlwarning call sqlprint;

	exec sql begin declare section;
		int	seed;
		char	*dbname = argv[1];
	exec sql end declare section;

	if (argc != 3)
	{
		printf ("\n\tUsage: getseed <dbname> <seed>\n");
		PCexit();
	}

	exec sql whenever sqlerror stop;

	exec sql connect :dbname;

	exec sql whenever sqlerror call sqlprint;

	seed = atoi(argv[2]);
	if (seed == 0)
	{
		seed = (int) time(0);
	}
	printf ("\n\tSeed used: %d\n", seed);

	exec sql modify seeds to truncated;

	exec sql insert into seeds values (:seed);

	exec sql commit;

	exec sql disconnect;
}
