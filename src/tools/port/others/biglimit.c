/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**	biglimit -- set-user-id program to create shell w/ sufficient
**		    file size limit to do some work on System V.
**
**		executable must be owned by "root".
**
**	$Log:	biglimit.c,v $
 * Revision 1.1  90/03/09  09:18:04  source
 * Initialized as new code.
 * 
 * Revision 1.1  89/08/01  12:35:37  source
 * sc
 * 
 * Revision 1.2  87/12/15  16:27:47  source
 * sc
 * 
 * Revision 1.1  87/07/30  18:48:46  roger
 * This is the new stuff.  We go dancing in.
 * 
**		Revision 1.3  86/11/18  09:29:23  source
**		sc
**		
**		Revision 1.2  86/11/07  10:07:03  source
**		sc
**		
**		Revision 1.1  86/01/22  15:54:59  roger
**		Original: Paul Sherman, ATTIS. Reworked by Roger.
**		
*/

# include	<stdio.h>
# include	<errno.h>

# ifdef SYS5

main(argc, argv)
int	argc;
char	*argv[];
{
	extern long	ulimit();
	extern long	atol();
	extern char	*getenv();

	char		*shell;
	extern char	**environ;

	if ((ulimit(2, (argc == 2) ? atol(argv[1]) : (long)32000)) < 0)
	{
		printf("%s: can't increase file size limit\n", argv[0]);
	}
	else
	{
		setuid(getuid());

		if ((shell = getenv("SHELL")) == NULL)
			shell = "/bin/sh";

		execle(shell, shell, 0, environ);
	}
	exit(errno);
}

# else

main()
{
	printf("biglimit does nothing on BSD systems\n");
}

# endif
