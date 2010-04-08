/*
** Copyright (c) 2004, 2010 Ingres Corporation
**
**  Name: iiaudloc.c -- utility for generating default audit locations
**
**  Description:
**
**	Usage: iiaudloc filename
**
**  History:
**	7-apr-94  (robf)
**           Created
**	8-apr-94 (robf)
**           Change UTILITY to utility so it goes in utility. 
**	13-apr-94 (robf/ajc)
**	     Make iiaudloc more forgiving incase iigenres adds 
**	     more parameters on the command line.
**      13-apr-94 (ajc)
**           Updated to write result to temporary file for iisues.
**      02-sept-09 (frima01) 121804
**           Remove extra function for usage.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
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
PROGRAM = 	iiaudloc
**
NEEDLIBS = 	UTILLIB COMPATLIB MALLOCLIB 
**
UNDEFS =	II_copyright
**
DEST   =	utility
*/

main( argc, argv )
int argc;
char *argv[];
{
	STATUS status=OK;
	char	    *pmvalue;
	char	    *pmname;
	CL_ERR_DESC errdesc;
	char	     *auditfile;
	LOCATION     audit_loc, fileloc;
	char	     audit_locbuf[ MAX_LOC+ 1 ];
	char	     *full_path;
        FILE         *result_log_fp = NULL;
        char locbuf[ MAX_LOC + 1 ];        

	MEadvise( ME_INGRES_ALLOC );

	PMinit();

	PMlowerOn();

	status = PMload( (LOCATION *) NULL, PMerror );

	switch( status )
	{
		case OK:
			break;

		default:
                        
			PCexit( FAIL );
	}
	PMsetDefault( 1, PMhost() );

	if(argc<2)
	{
		SIprintf("usage: iiaudloc auditfile\n");
		PCexit(1);
	}
	auditfile=argv[1];
	/*
	** Build a default location name, under CONFIG
	*/
	/* prepare audit file LOCATION */
	NMloc( ADMIN, FILENAME, auditfile, &audit_loc );
	LOcopy( &audit_loc, audit_locbuf, &audit_loc ); 
	LOtos( &audit_loc, &full_path );
        
        if((argc > 2) && (argc < 5))
        {
            
                /* 
                 * write result to a temporary file
                 */

                STlcopy( argv[ 2 ], locbuf, sizeof( locbuf ) );
                LOfroms( PATH & FILENAME, locbuf, &fileloc );

                if( SIfopen( &fileloc, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC,
                        &result_log_fp ) != OK )
                {
                        SIfprintf(stderr, "Unable to open file %s\n",argv[2]);
                        PCexit(FAIL);
                }

                SIfprintf(result_log_fp, ERx( "%s\n"), full_path);
                SIclose(result_log_fp);
        }
        else
                SIprintf("%s\n",full_path);
        
	PCexit(0);
}
