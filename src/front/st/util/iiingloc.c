/*
** Copyright (c) 2010 Ingres Corporation
**
**  Name: iiingloc.c -- utility which returns location of
**			BIN, FILES, LIB, LIB32, and LIB64 directories. 
**
**  Description:
**
**	Usage: iiingloc dirname [outfile]
**
**  History:
**	1-Jun-98  (rajus01)
**	26-apr-2000 (somsa01)
**	    Adjusted MING hint for program name to reflect product.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Sep-2010 (rajus01) SD issue 146492, Bug 124381
**	    Add LIB32 and LIB64.
**      30-sep-2010 (joea)
**          Wrap above change with an #ifndef VMS.
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <lo.h>
# include <si.h>
# include <st.h>
# include <pc.h>
# include <er.h>
# include <cm.h>
# include <cv.h>
# include <pm.h>
# include <util.h>
# include <sl.h>
# include <nm.h>
/*
PROGRAM = 	(PROG4PRFX)(PROG2PRFX)loc
**
NEEDLIBS = 	UTILLIB COMPATLIB MALLOCLIB 
**
UNDEFS =	II_copyright
**
DEST   =	utility
*/

static STATUS get_nm_symbol( char *dirname, i4  *symbol );

main( argc, argv )
int argc;
char *argv[];
{
    STATUS	status = OK;
    char	*dirname = NULL;
    LOCATION	dirloc, fileloc;
    char	dir_locbuf[ MAX_LOC+ 1 ];
    char	*full_path;
    FILE	*fp = NULL;
    char	locbuf[ MAX_LOC + 1 ];        
    i4		symbol;

    MEadvise( ME_INGRES_ALLOC );

    if( ( argc < 2 ) )
    {
	SIprintf("usage: iiingloc dirname [outfile]\n");
	PCexit(FAIL);
    }

    dirname = argv[ 1 ];

    if( (  get_nm_symbol( dirname, &symbol ) ) != OK )
    {
	SIprintf("%s directory is not supported\n", dirname );
	PCexit( FAIL );
    }

    NMloc( symbol, PATH, NULL, &dirloc );
    LOcopy( &dirloc, dir_locbuf, &dirloc );
    LOtos( &dirloc, &full_path );

    if( argc < 3 )
    {
	/* send result to standard output */
        SIprintf( ERx( "%s\n" ), full_path );
        PCexit( OK );
    }
    /* 
    ** otherwise, send to temporary file passed in argv[ 2 ]
    */
    STlcopy( argv[ 2 ], locbuf, sizeof( locbuf ) );
    LOfroms( PATH & FILENAME, locbuf, &fileloc );

    if( SIfopen( &fileloc, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC, &fp ) != OK )
    {
	SIfprintf(stderr, "Unable to open file %s\n", argv[ 2 ]);
	PCexit(FAIL);
    }
    SIfprintf(fp, ERx( "%s\n"), full_path);
    SIclose( fp );
    PCexit( OK );
}

static STATUS get_nm_symbol( char *dirname, i4  *symbol )
{
    if ( ! dirname ||  ! *dirname )
        return( FAIL );
    else  if ( ! STbcompare( dirname, 0, "bin", 0, TRUE ) )
        *symbol = BIN;
    else  if ( ! STbcompare( dirname, 0, "lib", 0, TRUE ) )
        *symbol = LIB;
#ifndef VMS
    else  if ( ! STbcompare( dirname, 0, "lib32", 0, TRUE ) )
        *symbol = LIB32;
    else  if ( ! STbcompare( dirname, 0, "lib64", 0, TRUE ) )
        *symbol = LIB64;
#endif
    else  if ( ! STbcompare( dirname, 0, "files", 0, TRUE ) )
        *symbol = FILES;
    else
        return( FAIL );

    return( OK );
}
