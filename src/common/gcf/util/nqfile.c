/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cv.h>
#include    <er.h>
#include    <gc.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>

#include    <iicommon.h>
#include    <erglf.h>
#include    <erclf.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>

/*
** Name: nqfile.c
**
** Description:
**	Display information on Name Server Name Queue disk file.
**
**	usage: nqfile [-v] <queue_name>
**
**	    -v		Verbose mode (includes individual record info).
**	    <queue>	Name of Name Server Queue (INGRES, NODE, etc).
**
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	 3-Aug-09 (gordy)
**	    New variable length parameterized field record format.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
*/

/*
** MING Hints:
**
NEEDLIBS =	GCFLIB COMPATLIB 
OWNER =		INGUSER
PROGRAM =	nqfile
*/

/*
** Forward and/or External function references.
*/

static	VOID	usage( char *name );
static	VOID	analyze( char *name );
static	STATUS	gcn_open( char *name, char *mode, FILE **file, i4 *reclen );
static	VOID	gcn_nq_filename( char *type, char *host, char *filename );
static	STATUS	gcn_nq_fopen( char *name, char *mode, FILE **file );
static	STATUS	gcn_read_rec( FILE *file, u_i1 *record, i4 rec_len );
static	VOID	gcn_read_rec0( i4 rec_no, GCN_DB_REC0 *record, i4 tuple_id );
static	VOID	gcn_read_rec1( i4 rec_no, GCN_DB_REC1 *record );

static	bool	verbose = FALSE;


/*
** Name: main
**
** Description:
**	Initialize and process command line.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

i4
main( i4  argc, char **argv )
{
    i4  arg = 1;

    MEadvise(ME_INGRES_ALLOC);
    SIeqinit();
    PMinit();

    if ( argc > 1  &&  argv[1][0] == '-' )
    {
	arg++;

	switch( argv[1][1] )
	{
	    case 'v' : 
		verbose = TRUE;
		break;

	    default  :
		usage( argc >= 1 ? argv[0] : NULL );
		PCexit(FAIL);
	}
    }

    if ( arg >= argc )
    {
	usage( argc >= 1 ? argv[0] : NULL );
	PCexit(FAIL);
    }
    else
    {
	analyze( argv[ arg ] );
	PCexit(OK);
    }

    return(OK);
}


/*
** Name: usage
**
** Description:
**	Display command line usage info.
**
** Input:
**	name		Name used to execute this utility.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
*/

static VOID
usage( char *name )
{
    SIprintf( "usage: %s [-v] <name>\n", (name && *name) ? name : "nqfile" );
    return;
}


/*
** Name: analyze
**
** Description:
**	Opens disk file associated with a Name Queue.  Reads records
**	in disk file, compiles and displays statistics.
**
** Input:
**	name		Name of Queue.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
**	 3-Aug-09 (gordy)
**	    New record format.  Declare record format for default 
**	    record length.  Use dynamic storage if actual record
**	    length is larger than default.  Use more appropriate
**	    length for hostname.
*/

static VOID 
analyze( char *name )
{
    GCN_DB_RECORD	dflt_rec;
    GCN_DB_RECORD	*record = &dflt_rec;
    STATUS		status;
    FILE		*file;
    char		host[ GC_HOSTNAME_MAX + 1 ];
    char		filename[MAX_LOC];
    i4			rec_len = sizeof( GCN_DB_RECORD );
    i4			records = 0;
    i4			deleted = 0;

    GChostname( host, sizeof( host ) );
    gcn_nq_filename( name, host, filename );
    status = gcn_nq_fopen( filename, "r", &file );

    if ( status != OK )
    {
        SIprintf( "could not open %s: 0x%x\n", filename, status );
	return;
    }

    if ( rec_len != sizeof( GCN_DB_RECORD ) )
    {
        record = (GCN_DB_RECORD *)MEreqmem( 0, rec_len, FALSE, NULL );
	
	if ( ! record )
	{
	    SIprintf("could not allocate record buffer - size %d\n", rec_len);
	    return;
	}

        if ( verbose )  SIprintf("File expanded record length: %d\n", rec_len);
    }

    while( gcn_read_rec( file, (u_i1 *)record, rec_len ) == OK )
    {
	i4 tuple_id = 0;

	if ( record->rec1.gcn_rec_type == GCN_DBREC_EOF )
	{
	    if ( verbose )  SIprintf( "record %d: LEOF\n", records );
	    break;
	}

	if ( record->rec1.gcn_rec_type & GCN_DBREC_V0_MASK )
	{
	    tuple_id = record->rec0.gcn_tup_id;
	    record->rec1.gcn_rec_type = GCN_DBREC_V0;
	}

	switch( record->rec1.gcn_rec_type )
	{
	case GCN_DBREC_V0 :
	    if ( record->rec0.gcn_invalid )  
	    {
		deleted++;

		if ( verbose )  
		    SIprintf( "record %d: deleted (v0)\n", records );
	    }
	    else  if ( verbose )
	    {
		gcn_read_rec0( records, &record->rec0, tuple_id );
	    }
	    break;

	case GCN_DBREC_V1 :
	    if ( record->rec1.gcn_rec_len != rec_len )
		SIprintf( "Record length mismatch: record %d, length %d\n",
			  records, record->rec1.gcn_rec_len );

	    if ( verbose )  gcn_read_rec1( records, &record->rec1 );
	    break;

	case GCN_DBREC_DEL :
	    if ( record->rec1.gcn_rec_len != rec_len )
		SIprintf( "Record length mismatch: record %d, length %d\n",
			  records, record->rec1.gcn_rec_len );

	    deleted++;
	    if ( verbose )  SIprintf( "record %d: deleted\n", records );
	    break;

	default :
	    SIprintf( "record %d: invalid record version %d\n",
	    	      records, record->rec1.gcn_rec_type );
	    break;
	}

        records++;
    }

    SIclose( file );

    SIprintf( "\nSummary of file %s:\n", filename );
    SIprintf( "Records: %d\n", records );
    SIprintf( "Active : %d\n", records - deleted );
    SIprintf( "Deleted: %d\n", deleted );
    SIprintf( "\n" );

    if ( record != &dflt_rec )  MEfree( (PTR)record );
    return;
}


/*
** Name: gcn_open
**
** Description:
**	Open a NS Queue file and determine the record length.
**
** Input:
**	name	File name.
**	mode	Read/write mode.
**	reclen	Default record length.
**
** Output:
*8	file	File descriptor.
**	reclen	Actual record length.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

static STATUS 
gcn_open( char *name, char *mode, FILE **file, i4 *reclen )
{
    GCN_DB_RECORD	record;
    STATUS		status;

    if ( (status = gcn_nq_fopen( name, mode, file )) != OK )
	return( status );

    if ( gcn_read_rec( *file, (u_i1 *)&record, sizeof( record ) ) == OK )
	switch( record.rec1.gcn_rec_type )
	{
	case GCN_DBREC_V1 :
	case GCN_DBREC_DEL :
	    if ( record.rec1.gcn_rec_len != *reclen )
	    {
		SIclose( *file );
		*file = NULL;

		*reclen = record.rec1.gcn_rec_len;
	        status = gcn_nq_fopen( name, mode, file );
		if ( status != OK )  return( status );
	    }
	    break;

	default :
	    if ( *reclen != sizeof( GCN_DB_RECORD ) )
	    {
		SIprintf( "Warning: mismatch in record length: \n" );
	        SIprintf( "         using %d instead of %d\n",
			  *reclen, sizeof( GCN_DB_RECORD ) );
	    }
	}

    SIfseek( *file, (i4) 0, SI_P_START );

    return( OK );
}


/*
** Name: gcn_nq_filename
**
** Description:
**	Form a disk filename for a name queue.  The host name should
**	be provided for non-global queues.
**
** Input:
**	type		Name of queue.
**	host		Name of host (NULL if global queue).
**
** Output:
**	filename	Disk filename.
**
** Returns:
**	VOID
**
** History:
**	26-Feb-96 (gordy)
**	    Created.
*/

static VOID
gcn_nq_filename( char *type, char *host, char *filename )
{
    i4		len, plen, slen;
    char        *onOff = NULL;
    bool        clustered = FALSE;
    STATUS      status;

    /*
    ** Filename template: II<type>[_<host>]
    */
    plen = 2;
    len = STlength( type );
    slen = (host && *host) ? STlength( host ) + 1 : 0;

    /*
    ** Adjust filename for platform limitations:
    **   drop prefix if type and suffix OK; 
    **   drop suffix if prefix and type OK;
    **   otherwise, drop both prefix and suffix.
    */
    if ( plen + len + slen > LO_NM_LEN )
	if ( len + slen <= LO_NM_LEN )
	    plen = 0;
	else  if ( plen + len <= LO_NM_LEN )
	    slen = 0;
	else
	    plen = slen = 0;

    /*
    ** See if this is a clustered installation.  If it is, 
    ** the node, login, and attribute files have no file extension.
    */
    if ( PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL ) != OK )
    {
        SIprintf("Error reading config.dat\n");
        return;
    }

    PMsetDefault( 0, "ii" );
    PMsetDefault( 1, host );
    PMsetDefault( 2, ERx("gcn") );

    status = PMget( ERx("!.cluster_mode"), &onOff);

    if (onOff && *onOff)
        clustered = !STncasecmp(onOff, "ON", STlength(onOff));

    if (clustered && (!STncasecmp("LOGIN", type, STlength(type)) ||
        !STncasecmp("NODE", type, STlength(type)) ||
        !STncasecmp("ATTR", type, STlength(type))))
        slen = 0;

    CVupper(type);

    STprintf( filename, "%s%s%s%s", 
        plen ? "II" : "", type, slen ? "_" : "", slen ? host : "" );

    /*
    ** Finally, truncate the filename if it is too long
    ** (hopefully this will never happen).
    */
    if ( STlength( filename ) > LO_NM_LEN )
	filename[ LO_NM_LEN ] = '\0';
    
    return;
}


/*
** Name: gcn_nq_fopen 	- Open a named file.
**
** Description:
**	This routine opens a named file which contains the type of database 
**	identified by its name. The file descriptor is returned.
**
** Inputs:
**	name	- The file name.
**	mode  - read/write mode.
**
** Outputs:
**	file
**
** Returns:
**	OK  - succeed	
**	status - Error code
**
** Exceptions:
**	    none
**
** History:
**
**      23-Mar-88 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Revamped.  Added reclen.
**	29-Jun-92 (fredb)
**	    Integrate bryanp's 1989 changes for MPE (hp9_mpe):
**      	17-Sep-89 (bryanp)
**          		Fixed a bug in the hp9_mpe filename generation code 
**			introduced during 61b3ug integration.
**      	18-Sep-89 (bryanp)
**          		Fixed another bug in the hp9_mpe filename generation 
**			code which was introduced during 61b3ug integration.
**     16-Nov-2010 (Ralph Loen) Bug 122895
**           Open global files without a hostname extension if 
**           ii.[hostname].config.cluster_mode is ON.  Remove reclen
**           argument.
**
*/

static STATUS 
gcn_nq_fopen( char *name, char *mode, FILE **file )
{
    LOCATION	loc;
    STATUS	status;

#ifndef hp9_mpe
    if ( ( status = NMloc( ADMIN, PATH, (char *)NULL, &loc ) ) != OK )
	goto error;

    LOfaddpath( &loc, "name", &loc );
    LOfstfile( name, &loc );
#else
    if ( (status = NMloc( FILES, FILENAME, name, &loc )) != OK )
	goto error;
#endif

    if ( ! STcompare( mode, "r" )  &&  (status = LOexist( &loc )) != OK )
	goto error;

#ifdef VMS
    status = SIfopen( &loc, mode, SI_RACC, sizeof(GCN_DB_RECORD), file );

    if (status == E_CL1904_SI_CANT_OPEN && ( LOexist( &loc ) == OK ) )
	status = SIfopen( &loc, mode, GCN_RACC_FILE, sizeof (GCN_DB_RECORD), 
            file );
#else
    status = SIfopen( &loc, mode, SI_RACC, sizeof (GCN_DB_RECORD), file );
#endif

error:

    if ( status != OK )  *file = NULL;
    return( status );
}


/*
** Name: gcn_read_rec
**
** Description:
**	Read a record from a Name Queue disk file.
**
** Input:
**	file		Open file handle.
**	rec_len		Record length.
**
** Output:
**	record		Contents of next record in file.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
**	 3-Aug-09 (gordy)
**	    Pass in record length for new varying length records.
*/

static STATUS
gcn_read_rec( FILE *file, u_i1 *record, i4 rec_len )
{
    STATUS	status = OK;
    char	*rec_ptr = (char *)record;
    i4		bytes_read = 0;

    while( rec_len )
    {
	status = SIread( file, rec_len, &bytes_read, rec_ptr );
	if ( status != OK )  break;

	rec_ptr += bytes_read;
	rec_len -= bytes_read;
    }

    return( status );
}


/*
** Name: gcn_read_rec0
**
** Description:
**	Display components of a V0 DB record.
**
** Input:
**	rec_no	Record number.
**	record	V1 DB record buffer.
**	tup_id	Tuple ID.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** Side-Effects:
**	Input buffer may be corrupted.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

static VOID
gcn_read_rec0( i4 rec_no, GCN_DB_REC0 *record, i4 tup_id )
{
    i4	uidlen = min( record->gcn_l_uid, sizeof( record->gcn_uid ) );
    i4	objlen = min( record->gcn_l_obj, sizeof( record->gcn_obj ) );
    i4	vallen = min( record->gcn_l_val, sizeof( record->gcn_val ) );

    record->gcn_uid[ uidlen ] = EOS;
    record->gcn_obj[ objlen ] = EOS;
    record->gcn_val[ vallen ] = EOS;

    SIprintf( "record %d: v0 0x%x '%s', '%s', '%s'\n", rec_no, 
	      tup_id, record->gcn_uid, record->gcn_obj, record->gcn_val );
    return;
}


/*
** Name: gcn_read_rec1
**
** Description:
**	Display components of a V1 DB record.
**
** Input:
**	rec_no	Record number.
**	record	V1 DB record buffer.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** Side-Effects:
**	Input buffer may be corrupted.
**
** History:
**	 3-Aug-09 (gordy)
**	    Created.
*/

static VOID
gcn_read_rec1( i4 rec_no, GCN_DB_REC1 *record )
{
    static char *empty = "";

    u_i1	*ptr = (u_i1 *)record + sizeof( GCN_DB_REC1 );
    u_i1	*end = (u_i1 *)record + record->gcn_rec_len;
    char	*uid = empty;
    char	*obj = empty;
    char	*val = empty;
    i4		uidlen = 0;
    i4		objlen = 0;
    i4		vallen = 0;
    i4		tuple_id = 0;

    while( (ptr + sizeof( GCN_DB_PARAM )) <= end )
    {
    	GCN_DB_PARAM	param;

	MEcopy( (PTR)ptr, sizeof( GCN_DB_PARAM ), (PTR)&param );
	if ( ! param.gcn_p_type )  break;
	ptr += sizeof( GCN_DB_PARAM );

	if ( (ptr + param.gcn_p_len) > end )
	{
	    SIprintf( "record %d: value length exceeds record length: %d\n",
	    	      rec_no, param.gcn_p_len );
	    return;
	}

	switch( param.gcn_p_type )
	{
	case GCN_P_UID :
	    uid = (char *)ptr;
	    uidlen = param.gcn_p_len;
	    break;

	case GCN_P_OBJ :
	    obj = (char *)ptr;
	    objlen = param.gcn_p_len;
	    break;

	case GCN_P_VAL :
	    val = (char *)ptr;
	    vallen = param.gcn_p_len;
	    break;

	case GCN_P_TID :
	    if ( param.gcn_p_len != sizeof( i4 ) )
	    {
		SIprintf( "record %d: invalid TID size %d\n",
			  rec_no, param.gcn_p_len );
		return;
	    }

	    MEcopy( (PTR)ptr, param.gcn_p_len, (PTR)&tuple_id );
	    break;
    
	default :
	    SIprintf( "record %d: invalid value ID %d\n",
	    	      rec_no, param.gcn_p_type );
	    return;
	}

	ptr += param.gcn_p_len;
    }

    uid[ uidlen ] = EOS;
    obj[ objlen ] = EOS;
    val[ vallen ] = EOS;

    SIprintf( "record %d: v%d 0x%x '%s', '%s', '%s'\n", 
    	      rec_no, record->gcn_rec_type, tuple_id, uid, obj, val );
    return;
}

