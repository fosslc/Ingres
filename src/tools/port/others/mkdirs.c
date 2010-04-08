/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** $Header: /cmlib1/ingres63p.lib/unix/tools/port/others/mkdirs.c,v 1.1 90/03/09 09:18:07 source Exp $
**
** Make directory and all subdirectories as needed.
**
** $Log:	mkdirs.c,v $
 * Revision 1.1  90/03/09  09:18:07  source
 * Initialized as new code.
 * 
 * Revision 1.1  89/08/01  12:36:27  source
 * sc
 * 
 * Revision 1.1  87/07/30  18:48:51  roger
 * This is the new stuff.  We go dancing in.
 * 
**		Revision 30.2  86/11/07  10:07:32  source
**		sc
**		
**		Revision 30.1  86/02/14  16:11:39  daveb
**		*** empty log message ***
**		
** Created 2/11/86 (daveb) for UTS; should be OK everywhere.
*/

# define MAXARG	5120	/* maximum argument list size */

char	cmd[ MAXARG ] = "/bin/mkdir ";

# if defined(BSD) || defined(BSD42)
# define strchr	index
# endif

main( argc, argv )
	int	argc;
	char **argv;
{
	register char	*arg;
	register char	*ap;			/* ptr in arg */
	register char	*ip;			/* ptr inside current *argv */
	register char	*slash;			/* location of next slash in *argv */
	int				status = 0;

	char	*strchr();				/* SV; -Dstrchr=index on BSD */

	arg = cmd + strlen( cmd );

	while( --argc )
	{
		ip = *++argv;
		ap = arg;

		/* append all subdirectories in creation order */

		for( slash = ip ; slash = strchr( slash, '/' ) ; slash++ )
		{				
			/* skip to second if full path */
			
			if( slash == ip )
				continue;

			/* append this subdirectory as an argument */

			for( ip = *argv; ip < slash ; )
				*ap++ = *ip++;

			*ap++ = ' ';									
		}

		/* append the complete directory */

		for( ip = *argv; *ip ; )
			*ap++ = *ip++;

		*ap = '\0';

		status |= system( cmd );
	}

	exit( status );
}

