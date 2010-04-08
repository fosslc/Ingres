/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: iipmhost.c -- writes string returned PMhost() to standard
**	output or a specified file. 
**
**  Usage:
**	iipmhost [[ -rad <radid> | -node <nodename> ] [ -local ]] [ outfile ] 
**
**  Description:
**	Writes local host name filtered by PMhost() to standard output or
**	a specified output file.  This executable is invoked as an external
**	procedure from the configuration rules system and is called by
**	package setup scripts to instantiate the host name component of
**	various configuration resources.
**
**	On a Clustered system, you may optionally pass a node name
**	(a not particularly useful identity operation ;-)), or a node alias,
**	with the '-node' parameter and the corresponding node name is echoed.
**
**	On a NUMA clustered host, you can pass a rad id with the -rad
**	parameter and the corresponding virtual node name is echoed.
**
**	For either of these options, if "-local" is passed, then valid
**	values for the '-rad' & '-node' arguments are restricted to
**	nodes defined on the local host machine.
**
**	If you do pass one of these three cluster options the status code
**	for this program will be set to FAIL on exit, and no hostname
**	is echoed if the values passed are not part of the cluster, or
**	if the local host is configured for NUMA clusters, and RAD context
**	was not set.  We take advantage of this in 'ipcclean' to make
**	sure that 'ipcclean' has a valid RAD context set before going
**	off and blitzing shared memory segments.
**
**
**  Exit Status:
**	OK	Succeeded.
**	FAIL	Couldn't open output file.
**
**  History:
**	16-dec-93 (tyler)
**		Created.
**      21-apr-1999 (hanch04)
**          Added st.h
**	21-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
PROGRAM =	(PROG1PRFX)pmhost
**
NEEDLIBS =	COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

# include	<compat.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<er.h>
# include	<pm.h>
# include	<cs.h>
# include	<pc.h>
# include	<cx.h>

main( i4  argc, char **argv )
{
	char host[ CX_MAX_NODE_NAME_LEN + 1];
	char locBuf[ MAX_LOC + 1 ];
	LOCATION loc;
	FILE *fp;
	i4   flags, arg, lastarg;
	char **argh;

	MEadvise( ME_INGRES_ALLOC );

	flags = CX_NSC_CONSUME | CX_NSC_NODE_GLOBAL | CX_NSC_RAD_OPTIONAL;

	for ( argh = argv + 1, arg = 1, lastarg = argc; arg < lastarg; arg++ )
	{
		if ( 0 == STcompare( ERx( "-local" ), argv[arg] ) )
		{
			*argh = NULL;
			argc--;
			flags = CX_NSC_CONSUME;
		}
		else
		{
			*argh++ = argv[arg];
		}
	}
	*argh = NULL;

	if ( OK != CXget_context( &argc, argv, flags,
				  host, CX_MAX_NODE_NAME_LEN ) )
	{
		PCexit(FAIL);
	}

	if( argc < 2 )
	{
		/* send result to standard output */
		SIprintf( ERx( "%s\n" ), host );
		PCexit( OK );
	}
	/* otherwise, send to temporary file passed in argv[ 2 ] */
	STlcopy( argv[ 1 ], locBuf, sizeof( locBuf ) );
	LOfroms( PATH & FILENAME, locBuf, &loc );
	if( SIfopen( &loc , ERx( "w" ), SI_TXT, SI_MAX_TXT_REC, &fp )
		!= OK )
	{
		PCexit( FAIL );	
	}
	SIputrec( host, fp );
	SIclose( fp );
	PCexit( OK );
}
