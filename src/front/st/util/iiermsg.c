/*
**  Copyright (c) 2004 Ingres Corporation
**
**  Name: iiermsg.c -- display an INGRES message given its hex id and
**	optional arguments.
**
**  Usage:
**	iiermsg hex_msg_id [ er_args  ]
**
**  Description:
**	Checks to make sure that the message id is in hex, assembles
**	an ER_ARGUMENT array, calls ERlookup(), and finally displays
**	the message on standard output with SIprintf().
**
**	In order to be displayed with iiermsg, a message which has two
**	arguments (for example) needs to be declared as follows: 
**
**		S_XX123_sample_message		"This %0c is %1c.\n"
**
**	Assuming that the script in which iiermsg is called is preprocessed
**	(in order to replace message symbols with hex ids) the above message
**	could be displayed by the following "source" line: 
**
**		iiermsg S_XX123_sample_message example silly
**
**	which would produce the output:
**
**		This example is silly. (followed by newline)
**
**  Exit Status:
**	OK	message displayed successfully.
**	FAIL	something went wrong.	
**
**  History:
**	06-aug-93 (tyler)
**		Created.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**    09-apr-1997 (hanch04)
**            Delete space at the end of ming hint
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2009 (frima01) 122490
**	    Add include of pc.h to avoid gcc 4.3 warnings.
*/

/*
PROGRAM = 	iiermsg
**
NEEDLIBS = 	COMPATLIB
**
UNDEFS = 	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <er.h>
# include <si.h>
# include <st.h>
# include <simacro.h>
# include <cv.h>
# include <cm.h>
# include <pc.h>

# define MAX_ER_ARG	50

main( argc, argv )
int argc;
char **argv;
{
	i4 language, i, length, nargs;
	u_i4 n;
	char buf[ 1024 ], *p;
	CL_ERR_DESC cl_err;
	ER_ARGUMENT er_args[ MAX_ER_ARG ];

	if( argc == 1 )
		ERROR( "Usage: iiermsg hex_msg_id [ er_args  ]" );

	if( ERlangcode( NULL, &language ) != OK )
		ERROR( "iiermsg: ERlangcode() failed." );

	if( STbcompare( argv[ 1 ], 2, ERx( "0x" ), 2, TRUE ) != 0 )
		F_ERROR( "iiermsg: message id \"%s\" is apparently not hex.",
			argv[ 1 ] );

	/* skip leading 0x */
	p = argv[ 1 ];
	CMnext( p );
	CMnext( p );

	if( CVahxl( p, &n ) != OK )
		F_ERROR( "iiermsg: message id \"%s\" is apparently not hex.",
			argv[ 1 ] );

	for( i = 2, nargs = 0; i < argc && i - 2 < MAX_ER_ARG; i++, nargs++ )
	{
		er_args[ i - 2 ].er_value = (PTR) argv[ i ];
		er_args[ i - 2 ].er_size = ER_PTR_ARGUMENT; 
	}

	if( ERlookup( (i4) n, NULL, ER_TEXTONLY, NULL, buf,
		sizeof( buf ), language, &length, &cl_err, nargs,
		(nargs > 0) ? er_args : (ER_ARGUMENT *) NULL )
		!= OK )
	{
		F_ERROR( "iiermsg: message %s not found.", argv[ 1 ] );
	} 

	F_PRINT( ERx( "%s" ), buf );

	PCexit( OK );
}
