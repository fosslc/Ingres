/*
**  Name: iicvtgcn -- Convert RACC (relative) GCN files to optimized
**      RACC files
**
**  Description:
**
**      Iicvtgcn converts Name Server vnode and login files in standard RACC 
**      format to optimized RACC format.  If the source file is found to be
**      in optimized RACC format, iicvtgcn exits without performing
**      any conversions.
**    
**  Entry points:
**	This file contains no externally visible functions.
**
** History:
**     06-Dec-2005 (loera01)
**         Created.
**     11-Feb-2008 (Ralph Loen) Bug 116097 
**         Formalized for Ingres release.
**	24-Aug-09 (gordy)
**	    Use more appropriate size for host name buffer length.
**	    Check new record format for alternate record size.  The
**	    conditions under which a new record length would actually
**	    be used are highly unlikely to happen, so simply abort
**	    processing if non-standard record length is found.
**     14-Jan-2010 (horda03) Bug 123153
**         Allow optinal parameter of node name (for use in setting
**         up cluster installations.
*/

#include    <compat.h>
#include    <cm.h>
#include    <lo.h>
#include    <nm.h>
#include    <si.h>
#include    <st.h>
#include    <pc.h>
#include    <gc.h>
#include    <iicommon.h>
#include    <erclf.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>

main(int argc, char **argv)
{
    STATUS status = OK;
    GCN_DB_RECORD buf;
    FILE *srcFile = NULL;
    FILE *dstFile = NULL;
    LOCATION loc;
    i4 count;
    char ingFilenameFmt[2][20] = { "IILOGIN_%s", "IINODE_%s" };
    char filename[2][MAX_LOC+1];
    char hostname[ GC_HOSTNAME_MAX + 1 ];
    i2 i;
    bool srcOpened=FALSE;
    bool dstOpened=FALSE;

    if (argc == 2)
    {
       STcopy( argv [1], hostname);
    }
    else
       GChostname( hostname, sizeof( hostname ) );

    for ( i = 0; i < 2; i++ )
    {  
        STprintf( filename[i], ingFilenameFmt[i],hostname);
     
        if ( ( status = NMloc( FILES, PATH & FILENAME, 
            (char *)NULL, &loc ) ) != OK )
        {
            SIprintf("iicvtgcn: Could not find II_SYSTEM:[ingres.files]\n");
            goto cvt_exit;
        }
     
        LOfaddpath( &loc, "name", &loc );
        LOfstfile( filename[i], &loc );
      
        /*
        ** Silently exit if the source file is already an optimized 
        ** RACC file.
        */
        status = SIfopen( &loc, "r", 
            (i4)SI_RACC, sizeof(GCN_DB_RECORD), &srcFile );
        if ( status == OK )
        {
            srcOpened = TRUE;
            goto cvt_exit;
        }
        /*
        ** A new installation may not have any login or node files
        ** configured.  Silently exit if this is the case.
        */
        if ( LOexist( &loc ) != OK )
            goto cvt_exit;

        /*
        ** Open source file with special GCN_RACC_FILE flag, which allows
        ** for backward compatibility with non-optimized RACC files.
        */
        status = SIfopen( &loc, "r", (i4)GCN_RACC_FILE, 
            sizeof( GCN_DB_RECORD ), &srcFile );
        if (status != OK )
        {
            SIprintf( "iicvtgcn: Error opening %s, status is %x\n",
                filename[i], status );
            goto cvt_exit;
        }

        srcOpened = TRUE;

        /*
        ** Open a new version of the file as optimized RACC.
        */
        status = SIfopen( &loc, "w", (i4)SI_RACC, sizeof( GCN_DB_RECORD ), 
            &dstFile );
        if ( status != OK )
        {
            SIprintf( "iicvtgcn: Error opening %s, status is %x\n",filename[i],
               status );
            goto cvt_exit;
        }

        dstOpened = TRUE;

        while ( status == OK )
        {
            /*
            ** Read the source data and write each record to the
            ** new version.
            */
            status = SIread( srcFile, sizeof( GCN_DB_RECORD ), 
                &count, (char *)&buf );
            if ( status == ENDFILE )
                break;

            if ( status != OK )
            {
                SIprintf("iicvtgcn: Error reading %s, status is %x\n",
                   filename[i], status);
                goto cvt_exit;
            }

	    if ( 
	         buf.rec1.gcn_rec_type != GCN_DBREC_EOF  &&
		 (buf.rec1.gcn_rec_type & GCN_DBREC_V0_MASK) == 0  &&
		 buf.rec1.gcn_rec_type != GCN_DBREC_V0  &&
	    	 buf.rec1.gcn_rec_len != sizeof( GCN_DB_RECORD )
	       )
	    {
		SIprintf("iicvtgcn: record length %d in %s not supported\n",
			buf.rec1.gcn_rec_len, filename[i] );
	    	break;
	    }

            status = SIwrite(sizeof( GCN_DB_RECORD ), (char *)&buf, 
                &count, dstFile );
            if ( status != OK)
            {
                SIprintf( "iicvtgcn: Failed to write file %s, status is %x\n",
                    filename[i], status );
                goto cvt_exit;
            }

	    /*
	    ** Done when logical EOF has been written.
	    */
	    if ( buf.rec1.gcn_rec_type == GCN_DBREC_EOF )
	    	break;
        } /* while ( status == OK ) */
   
        SIclose( srcFile );
        srcOpened = FALSE;
        SIclose( dstFile );
        dstOpened = FALSE;

    } /* for (i = 0; i < 2; i++ ) */

cvt_exit:
    if (srcOpened)
        SIclose(srcFile);
    if (dstOpened)
        SIclose(dstFile);

    PCexit( OK );
}
