/*
**Copyright (c) 2004 Ingres Corporation
*/
# include <stdio.h>
# include <ctype.h>

/*
 * eatjunk.c -- cleanup SV lint output
 *
 * Usage:  eatjunk pattern1 pattern2
 *
 * Eatjunk gets rid of 'uninteresting' errors from SV lint output
 * that you don't want.  Given input of:
 *
 *     warning: possible pointer alignment problem
 *       (1) (2) (3)
 *       (4) (5) (6)
 *     warning: brain damaged indirection
 *       (1) (2) (3)
 *
 * The command eatjunk 'possible pointer' will get rid of the error message
 * and all subsequent indented lines.
 *
 * Any number of patterns may be specified on the command line, eg:
 * 
 * eatjunk 'name declared but never used or defined' \
 *         'name defined but never used' \
 *         'name multiply declared' 
 */

main(argc, argv)
int argc;
char **argv;
{
    char buf[ BUFSIZ ];
    register int n = strlen( argv[1] );
    register char *p;
    register char *s;
    register int match;
    register int i;
    
    if( argc < 2 )
    {
	fprintf(stderr, "Usage %s pattern [ pattern ... ]\n", argv[0] );
	exit( 0 );
    }
    
    while( ( p = fgets( buf, sizeof buf, stdin ) ) )
    {
	/* here to examine a new line against all patterns */
again:
	/* whack off trailing \n */
	p[ strlen( p ) - 1 ] = 0;
	
	/*
	 * Try to match each pattern in argv
	 */
	match = 0;
	for( i = 1; i < argc ; i++ )
	{
	    s = argv[ i ];
	    /*
	     * If match somewhere, chew lines till one with
	     * non-blank start, then try next pattern
	     */
	    for( match = 0, p = buf; *p && !match; p++ )
	    {
		if( *s == *p && !strncmp( s, p, n ) )
		{
		    match = 1;
		    while( ( p = fgets( buf, sizeof buf, stdin ) )
			  && isspace( *p ) )
			;
		    
		    goto again;
		}
	    }
	}
	if( !match )
	    puts( buf );
    }
    
    exit( 0 );
    /*NOTREACHED*/
}




