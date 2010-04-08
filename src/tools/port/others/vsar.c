/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**
** vsar.c - visual sar for system status on System V
**
** This corresponds vaguely to the 'mon' program for 4.2
** It launches a sar and displays the output on the screen.
** Most display wierdness is attributable to sar, not vsar.
**
** Edit with 4 column tabs.
**
** Author:  David Brower
**          Relational Technology, Alameda, CA
**
**          {decvax, ucbvax}!mtxinu!rtech!daveb
**                                  rtech!gonzo!{daveb, root}
**
** This program is released to the public domain.
**
NEEDLIBS	=	CURSESLIB
**
** Compile:
**		cc -O vsar.c -lcurses -o vsar
** History:
**	20-jul-1990 (chsieh)
**		add # include <sys/types.h> for system V. 
**		Otherwise, fcntl.h may complain.
*/

# ifdef BSD42

main()
{
	exit( system( "mon" ) );
}

# else

# include	<sys/types.h>
# include	<curses.h>
# include	<setjmp.h>
# include	<signal.h>
# include	<fcntl.h>

typedef struct
{
	short 	x, y;		/* x,y of message */
	short	skip;		/* leading amount to skip */
	short	doclear;	/* do clear to eol after? */
} MSG;

/* describe destiny of each header line from sar */
MSG hdrlines[] =
{
	/* blank */
	-1,	-1, 0,	0,

	/* node */			/* blank */			/* %usr */
	0,	0,	0,	0,		-1,	-1,	0,	0,		0,	43,	12,	1,

	/* igets */			/* msgs */			/* runq */
	3,	0,	8,	0,		3,	26,	8,	0,		3,	48,	8,	1,

	/* text */
	6,	0,	8,	1,

	/* bread */
	9,	0,	8,	1,

	/* scall */
	12,	0,	8,	1,

	/* swpin */
	15,	0,	8,	1,
	
	/* disks */
	18,	0,	8,	1,

	/* blank */
	-1,	-1,	0,	0
} ;


MSG datalines[] =
{
											/* %usr */
											1,	31,	0,	1,

	/* igets */			/* msgs */			/* runq */
	4,	0,	8,	1,		4,	26,	8,	1,		4,	48,	8,	1,

	/* text */
	7,	0,	8,	1,

	/* bread */
	10,	0,	8,	1,

	/* scall */
	13,	0,	8,	1,

	/* swpin */
	16,	0,	8,	0,
	
	/* disk(s) */
	19,	0,	8,	1,
	20,	0,	8,	1,
	21,	0,	8,	1

} ;

/* cleanup terminal */
die()
{
	void	block();

	move( 20, 0 );
	refresh();
	endwin();
/*	resetty();	 */
	block();
	exit( 0 );
}


main( argc, argv )
int	argc;
char **argv;
{
	register MSG	* mp;
	register FILE	* sarfp;

	int		i;
	int		interval;

	char	buf[100];

	FILE	* popen();
	int		gotinput();
	void	noblock();

	/* process args */
	if( argc > 2 )
	{
		fprintf( stderr, "Usage: %s [ interval ]\n", *argv );
		exit ( 1 );
	}
	
	interval = 5;
	if ( 2 == argc )
		(void) sscanf ( *++argv, "%d", &interval );

	(void) sprintf( buf, "/usr/bin/sar -uamqvbcwd %d 9999", interval );
	(void) puts( buf );

	/* open the piped from sar */
	if ( NULL == (sarfp = popen( buf, "r" )) )
	{
		perror("starting sar");
		exit ( 1 );
	}

	/* setup screen */
/*	savetty();	*/
	initscr();
	nonl();
	cbreak();
	noblock();

	/* make sure you cleanup */
	(void) signal( SIGINT, die );
	(void) signal( SIGQUIT, die );
	(void) signal( SIGTERM, die );
	(void) signal( SIGHUP, die );
	
	/* process header lines */
	for( i = 0; i < sizeof hdrlines / sizeof( MSG ); i++ )
	{
		if ( NULL == fgets( buf, sizeof buf, sarfp ))
			die();

		domsg( buf, &hdrlines[i] );
	}

	/* process data lines forever */
	for( mp = datalines; ; mp++ )
	{
		if ( gotinput() )
			clearok( curscr, TRUE );			

		refresh();

		if ( NULL == fgets( buf, sizeof buf, sarfp ))
			break;

		/* got a date?  Back to the top... */
		if( ' ' != buf[0] )
			mp = datalines;

		domsg( buf, mp );
	}

	die();
}


/* display a message */
domsg( buf, mp )
char * buf;
register MSG * mp;
{
	/* ignore this line */
	if ( -1 == mp->x )
		return;

	/* display in specified place */
	buf[ strlen( buf ) - 1 ] = '\0';
	move( mp->x, mp->y );
	addstr( &buf[ mp->skip ] );
	if ( mp->doclear )
		clrtoeol();

	/* always leave cursor here */
	move( 20, 0 );
}

/* put input in non-blocking state */
void
noblock()
{
	int	flags;

	flags = fcntl( 0, F_GETFL, 0 );
	(void) fcntl( 0, F_SETFL, flags | O_NDELAY );
}

/* put input in blocking state */
void
block()
{
	int	flags;

	flags = fcntl( 0, F_GETFL, 0 );
	(void) fcntl( 0, F_SETFL, flags & ~O_NDELAY );
}


/* any input pending? */
int
gotinput()
{
	char	garbage[ BUFSIZ ];

	/* return 0 if no input */
	return ( read( 0, garbage, sizeof garbage ) );
}


/* end of vsar.c */


# endif
