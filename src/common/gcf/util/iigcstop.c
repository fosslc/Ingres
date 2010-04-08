/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>

# include	<iicommon.h>
# include	<gca.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>

/*
** Name: iigcstop.c
**
** Description:
**	Stop an Ingres GCA based server which accepts 
**	GCA_CS_SHUTDOWN requests.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
**      mkming hints
NEEDLIBS =      GCFLIB COMPATLIB 
OWNER =         INGUSER
PROGRAM =       (PROG1PRFX)gcstop
*/

int
main( int argc, char **argv )
{
    GCA_PARMLIST	parms;
    STATUS		iigca_status = OK;
    STATUS		r_stat = OK;
    i4			i, assoc_no;

    MEfill( sizeof( parms ), 0, (PTR)&parms );
    parms.gca_in_parms.gca_modifiers = GCA_SYN_SVC | GCA_API_VERSION_SPECD;
    parms.gca_in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_63;
    parms.gca_in_parms.gca_api_version = GCA_API_LEVEL_5;

    IIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0, -1, &iigca_status );

    r_stat = ( iigca_status != E_GC0000_OK )
	     ? iigca_status : parms.gca_in_parms.gca_status;

    if ( r_stat == OK )
    {
        u_i4		flags = GCA_CS_SHUTDOWN; 

	for( i = 1; i < argc; i++ )
	    if( argv[i][0] == '-' )
	    {
		switch( argv[i][1] )
		{
		case 'q' : flags = GCA_CS_QUIESCE;	break;
		case 's' : flags = GCA_CS_SHUTDOWN;	break;
		default :
		    SIfprintf( stderr, "Invalid flag: %s\n\n", argv[i]);
		    SIflush(stderr);
		    return(FAIL);
		}
	    }
	    else
	    {

		MEfill( sizeof( parms ), 0, (PTR)&parms );
		parms.gca_rq_parms.gca_partner_name = argv[ i ];
		parms.gca_rq_parms.gca_modifiers = GCA_NO_XLATE | flags;

		IIGCa_call(GCA_REQUEST,&parms,GCA_SYNC_FLAG,0,-1,&iigca_status);

		if ( iigca_status == E_GC0000_OK )
		    iigca_status = parms.gca_rq_parms.gca_status;
		if ( iigca_status != E_GC0000_OK )  r_stat = iigca_status;

		assoc_no = parms.gca_rq_parms.gca_assoc_id;
		MEfill( sizeof( parms ), 0, (PTR)&parms );
		parms.gca_da_parms.gca_association_id = assoc_no;

		IIGCa_call(GCA_DISASSOC,&parms,GCA_SYNC_FLAG,0,-1,&iigca_status);
	    }

        MEfill( sizeof( parms ), 0, (PTR)&parms );
	IIGCa_call(GCA_TERMINATE,&parms,GCA_SYNC_FLAG,0,-1,&iigca_status);
    }

    PCexit( 0 );
    return( r_stat );
}
