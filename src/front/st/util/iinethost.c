/*
** Copyright (c) 2007 Ingres Corporation
**
**  Name: iinethost.c -- writes fully qualified host name to standard
**	output.
**
**  Usage:
**	iinethost [ -local ] [ outfile ]
**
**  Description:
**	Writes fully qualified DNS host name to the standard output or
**      a specified output file.
**
**      Iinethost is guaranteed to write the a hostname, even if the
**      local machine is not configured for DNS or TCP/IP.  In the 
**      latter case, the unqualified host name is displayed.
**       
**      If option "-local" is specified, the local hostname is displayed. 
**
**  Exit Status:
**	OK	Succeeded.
**
**  History:
**	01-Aug-2007 (Ralph Loen) SIR 118898
**        Created.
**	26-Oct-2009 (hweho01)  Bug 122277
**        Added option '-local' to write local hostname. 
*/

# include	<compat.h>
# include       <cv.h>
# include       <er.h>
# include	<gc.h>
# include     	<gcccl.h>
# include	<lo.h>
# include	<pc.h>
# include	<si.h>
# include	<st.h>

main(int argc, char **argv)
{
    char  host[GCC_L_NODE + 1];
    char locBuf[ MAX_LOC + 1 ];
    LOCATION loc;
    FILE *fp;
    bool local = FALSE;  
    i4   index = 1; 

    if( ( argc > 1 ) && ( STcompare( argv[1], "-local" ) == 0 ) )
    { 
      local = TRUE; 
      index++;
    }
      
    if( !local )
    {
      if (GCdns_hostname(host, sizeof(host)) != OK)
      {
        GChostname(host, sizeof(host));
        CVlower(host);
      }
    }
    else
    {
      GChostname(host, sizeof(host));
      CVlower(host);
    }

    if( ( argc < 2 ) || ( local && argc == 2 ) )
    {
        /* 
        ** Send the result to standard output.
        */
        SIprintf( ERx( "%s\n" ), host );
        PCexit( OK );
    }
    /* 
    ** Otherwise, send to the temporary file passed in the argument.
    */
    STlcopy( argv[ index ], locBuf, sizeof( locBuf ) - 1 );
    LOfroms( PATH & FILENAME, locBuf, &loc );
    if( SIfopen( &loc , ERx( "w" ), SI_TXT, SI_MAX_TXT_REC, &fp ) != OK )
        PCexit( FAIL );	

    SIputrec( host, fp );
    SIclose( fp );
    PCexit( OK );
}
