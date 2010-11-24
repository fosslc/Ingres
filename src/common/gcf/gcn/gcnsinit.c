/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cm.h>
#include    <cv.h>
#include    <er.h>
#include    <gc.h>
#include    <lo.h>
#include    <me.h>
#include    <mh.h>
#include    <mu.h>
#include    <pm.h>
#include    <qu.h>
#include    <si.h>
#include    <sl.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcs.h>
#include    <gcu.h>

/*
**
** Name: gcnsinit.c
**
** Description:
**	Functions to initialize the Name Database interface used
**	by the Name Server (IIGCN) and embedded Name Server.
**
**	gcn_srv_init()	Initialize Name Database interface.
**	gcn_srv_term()	Terminate Name Database interface.
**	gcn_srv_load()	Load server/name queues.
**	
**  History:    $Log-for RCS$
**	    
**      09-Sep-87   (lin)
**          Initial creation.
**	09-Nov-87  (jorge)
**	    Updated GCN_NS_INIT to use the GCNSID(GCN_RESV_NSID)
**	01-Mar-89 (seiwald)
**	    Revamped.
**	    IIGCN database files no longer shadow the in-core data
**	    structures.  After the structures change, IIGCN writes
**	    them out whole again.
**	25-Nov-89 (seiwald)
**	    Reworked for async GCN.
**	28-Nov-89 (seiwald)
**	    Intialize gcn_incached flag in name queue head.
**	05-Feb-90 (seiwald)
**	    Use GC0101 instead of GC0125 on file open error.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	24-Apr-91 (brucek)
**	    Added include for tm.h
**	28-Jun-91 (seiwald)
**	    Internal name queue access restructured.  See gcnsfile.c
**	    for new description.
**	24-Jul-91 (alan)
**	    Support for incremental name file update.
**	23-Mar-92 (seiwald)
**	    Defined GCN_GLOBAL_USER as "*" to help distinguish the owner
**	    of global records from other uses of "*".
**	1-Sep-92 (seiwald)
**	    gca_erlog() renamed to gcu_erlog() and now takes ER_ARGUMENT's.
**	28-Sep-92 (brucek)
**	    Added MO calls to define GCN MIB objects;
**	    new argument to gcn_nq_add;
**	    GCN now marks its own queue as transient and registers with
**	    MIB support flag set.
**	01-Oct-92 (brucek)
**	    Moved MIB class definitions to gcnsfile.c.
**	07-Oct-92 (brucek)
**	    Set GCA_RG_TRAP flag to indicate GCM trap support.
**	14-Oct-92 (brucek)
**	    Set MIB flags in queue entry from IIGCa_static flags.
**      15-oct-92 (rkumar)
**          changed variable name 'shared' to 'common'. 'shared'
**          is a reserved word for Sequent's compiler.
**	07-Jan-93 (edg)
**	    Removed #include for gcagref.h.
**	19-Jan-93 (edg)
**	    Removed restart argument -- does nothing.
**	24-may-94 (wolf) 
**	    Resolve undefined symbol by including mu.h
**	 4-Dec-95 (gordy)
**	    Renamed gcn_initnames() to gcn_load().  Created gcn_init()
**	    with code extracted from main() in iigcn.c.  Added gcn_term().
**	15-Jan-96 (gordy)
**	    Clear IIGCn_static in gcn_term() in case we are re-initialized.
**	26-Feb-96 (gordy)
**	    Extracted disk filename formation to function to handle
**	    platform dependencies.
**	 3-Sep-96 (gordy)
**	    Renamed functions to ensure distinction with similar client
**	    functions.  Shouldn't be accessing global GCA data.
**	27-sep-1996 (canor01)
**	    Moved global data definitions to gcndata.c.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	27-May-97 (thoda04)
**	    Included cv.h.
**	29-May-97 (gordy)
**	    Delete registered server info matching name server address.
**	17-Feb-98 (gordy)
**	    Added addition GCM instrumentation support.
**	21-May-98 (gordy)
**	    Check for default remote authentication mechanism.
**	 4-Jun-98 (gordy)
**	    Added GCA_LISTEN timeout.
**	 5-Jun-98 (gordy)
**	    Made transient name queue processing dynamic.
**	10-Jun-98 (gordy)
**	    Added configuration symbol for file compression ratio.
**	16-Jun-98 (gordy)
**	    Added hashed access to Name Queues.
**	28-Jul-98 (gordy)
**	    Added configuration for server bedchecks.
**	14-Sep-98 (gordy)
**	    Initialize installation registry info and register self.
**	15-Sep-98 (gordy)
**	    Make hash case insensitive to match Name Server semantics.
**	 7-Oct-98 (gordy)
**	    Generate and save server key for ourself.
**      13-Jan-1999 (fanra01)
**          Add initialisation of registered name server uid.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
** 	11-Aug-2003 (wonst02)
** 	    In gcn_load_file(), a loop was modifying the loop control variable,
** 	    which was causing some of the keywords to be skipped.
**	24-May-04 (wansh01) 
**	    Convert timeout to milli-seconds for gca_call(); 
**	26-Jun-04 (gordy)
**	    Initialize random number generator.
**	15-Jul-04 (gordy)
**	    Added gcn_init_mask() for password encryption masks.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS queue, own server key no longer saved.
**	 3-Aug-09 (gordy)
**	    Remove string length restrictions.
**	27-Aug-10 (gordy)
**	    Added config param for login password encoding version.
**      16-Nov-2010 (Ralph Loen) Bug 122895
**          Made gcn_encode(), gcn_decode() and gcn_decrypt() public so that
**          iicvtgcn can use these functions.
*/

/*
** Forward and/or External function references.
*/

static	VOID		gcn_get_pmsym( char *, char *, char *, i4  );
static	GCN_HASH	ticket_hash( GCN_TUP * );
static	GCN_HASH	obj_uid_hash( GCN_TUP * );


/*
** Name Queue hashing information.
**
** LTICKETs are hashed on the ticket portion of the
** tuple value.  A standard hash function is also
** provided which hashes the tuple object and user
** ID.
*/

typedef struct
{
    char	*type;
    HASH_FUNC	func;
    i4		size;
} HASH_INFO;

static HASH_INFO hash_info[] =
{
    { GCN_NODE,    obj_uid_hash, GCN_HASH_SMALL },
    { GCN_LOGIN,   obj_uid_hash, GCN_HASH_SMALL },
    { GCN_ATTR,    obj_uid_hash, GCN_HASH_SMALL },
    { GCN_RTICKET, obj_uid_hash, GCN_HASH_LARGE },
    { GCN_LTICKET, ticket_hash,  GCN_HASH_LARGE }
};

#define	HASH_INFO_SIZE	(sizeof( hash_info ) / sizeof( hash_info[0] ))



/*
** Name: gcn_srv_init
**
** Description:
**	Initialize IIGCn_static values.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	30-Nov-95 (gordy)
**	    Extracted from iigcn.c
**	17-Feb-98 (gordy)
**	    Added addition GCM instrumentation support.
**	21-May-98 (gordy)
**	    Check for default remote authentication mechanism.
**	27-May-98 (gordy)
**	    Converted remote auth mech from pointer to char array.
**	 4-Jun-98 (gordy)
**	    Added GCA_LISTEN timeout.
**	28-Jul-98 (gordy)
**	    Added configuration for server bedchecks.
**	14-Sep-98 (gordy)
**	    Initialize installation registry.
**	24-May-04 (wansh01) 
**	    Convert timeout to milli-seconds for gca_call(); 
**      01-Oct-2004 (rajus01) Startrak Prob 148; Bug# 113165
**         Initialize timeout value for bedcheck messages. 
**	26-Jun-04 (gordy)
**	    Initialize random number generator.
**	 3-Aug-09 (gordy)
**	    Provide max username buffer.  Dynamically allocate storage
**	    for username, server type, local vnode, remote vnode and
**	    remote mechanism.
**	27-Aug-10 (gordy)
**	    Added config param for login password encoding version.
**	    Log warnings for invalid values.
*/

STATUS
gcn_srv_init( VOID  )
{
    CL_ERR_DESC	system_status;
    char	username[ GC_USERNAME_MAX + 1 ];
    char	*ptr, tmp[ 256 ];

    static char	*empty = "";

    /*
    ** Check to see if we have already been called.
    */
    if ( IIGCn_static.max_user_sessions )  return( OK );

    /*
    ** Initialize random number generator.
    */
    MHsrand( TMsecs() );

    /*
    ** Initialize IIGCn_static values.
    */
    IIGCn_static.max_user_sessions = GCN_MAX_SESSIONS;
    IIGCn_static.timeout = GCN_TIMEOUT;
    IIGCn_static.pwd_enc_vers = GCN_VLP_V1;
    IIGCn_static.compress_point = GCN_COMPRESS_POINT;
    IIGCn_static.registry_type = GCN_REG_NONE;
    IIGCn_static.bedcheck_interval = GCN_TIME_CLEANUP;
    IIGCn_static.ticket_exp_interval = GCN_TIME_CLEANUP;
    IIGCn_static.ticket_exp_time = GCN_TICKET_EXPIRE;
    IIGCn_static.ticket_cache_size = GCN_DFLT_L_TICKVEC;
    IIGCn_static.rmt_mech = empty;
    IIGCn_static.language = 1;			/* assume english for now */
    IIGCn_static.flags = GCN_BCHK_CONN;
    IIGCn_static.bchk_msg_timeout = GCN_FSEL_TIMEOUT;

    GCusername( username, sizeof( username ) );
    IIGCn_static.username = STalloc( username );

    TMnow( &IIGCn_static.now );
    STRUCT_ASSIGN_MACRO( IIGCn_static.now, IIGCn_static.cache_modified );
    STRUCT_ASSIGN_MACRO( IIGCn_static.now, IIGCn_static.registry_check );
    QUinit( &IIGCn_static.name_queues );

    gcn_get_pmsym( "!.default_server_class", GCN_INGRES, tmp, sizeof( tmp ) );
    IIGCn_static.svr_type = STalloc( tmp );

    gcn_get_pmsym( "!.local_vnode", IIGCn_static.hostname, tmp, sizeof(tmp) );
    IIGCn_static.lcl_vnode = STalloc( tmp );

    gcn_get_pmsym( "!.remote_vnode", "", tmp, sizeof( tmp ) );
    IIGCn_static.rmt_vnode = STalloc( tmp );

    gcn_get_pmsym( "!.remote_mechanism", "", tmp, sizeof( tmp ) );
    if ( *tmp )  IIGCn_static.rmt_mech = STalloc( tmp );

    gcn_get_pmsym( "!.session_limit", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVan( tmp, &IIGCn_static.max_user_sessions );

    gcn_get_pmsym( "!.timeout", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVan( tmp, &IIGCn_static.timeout );

    /* convert timeout to milli-seconds  */
    if (IIGCn_static.timeout > 0 ) 
	IIGCn_static.timeout = IIGCn_static.timeout * 1000;  

    gcn_get_pmsym( "!.check_timeout", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVan( tmp, &IIGCn_static.bchk_msg_timeout );

    gcn_get_pmsym( "!.cluster_mode", ERx("OFF"), tmp, sizeof( tmp ) );
    if ( ! STcasecmp( tmp, ERx("ON") ) )
    {
        /*
        ** GCN passwords in a cluster records are always encoded as GCN_VLP_V0.
        */
	IIGCn_static.flags |= GCN_CLUSTERED;
        IIGCn_static.pwd_enc_vers = GCN_VLP_V0;
    }

    if (!IIGCn_static.flags & GCN_CLUSTERED)
    {
        gcn_get_pmsym( "!.pwd_encode_version", "-1", tmp, sizeof( tmp ) );

        if ( *tmp )
        {
	    STATUS	status;
	    i4	version;

	    if ( (status = CVal( tmp, &version )) == OK )
            {
	        switch( version )
	        {
	        case GCN_VLP_V0 : IIGCn_static.pwd_enc_vers = GCN_VLP_V0; break;
	        case GCN_VLP_V1 : IIGCn_static.pwd_enc_vers = GCN_VLP_V1; break;
	        case -1         : /* Default */				  break;
	        default         : status = FAIL;			  break;
	        }

	        if ( status != OK )
	        {
	            ER_ARGUMENT earg[2];

	            earg[0].er_value = ERx("pwd_encode_version");
	            earg[0].er_size = STlength( earg[0].er_value );
	            earg[1].er_value = tmp;
	            earg[1].er_size = STlength( earg[1].er_value );

	            gcu_erlog( 0, 1, E_GC0106_GCN_CONFIG_VALUE, NULL, 2, 
                        (PTR)earg );
    	        }
            }
        }
    }

    gcn_get_pmsym( "!.compress_point", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVal( tmp, &IIGCn_static.compress_point );

    gcn_get_pmsym( "!.registry_type", "none", tmp, sizeof( tmp ) );
    if ( ! STcasecmp( tmp, ERx("slave") ) )
	IIGCn_static.registry_type = GCN_REG_SLAVE;
    else  if ( ! STcasecmp( tmp, ERx("peer") ) )
	IIGCn_static.registry_type = GCN_REG_PEER;
    else  if ( ! STcasecmp( tmp, ERx("master") ) )
	IIGCn_static.registry_type = GCN_REG_MASTER;
    else  if ( STcasecmp( tmp, ERx("none") ) )
    {
	ER_ARGUMENT earg[2];

	earg[0].er_value = ERx("registry_type");
	earg[0].er_size = STlength( earg[0].er_value );
	earg[1].er_value = tmp;
	earg[1].er_size = STlength( earg[1].er_value );

	gcu_erlog( 0, 1, E_GC0106_GCN_CONFIG_VALUE, NULL, 2, (PTR)earg );
    }

    gcn_get_pmsym( "!.check_type", "", tmp, sizeof( tmp ) );
    if ( *tmp )
    {
	char	*word[ 10 ];
	char	nbuff[ sizeof( tmp ) ];
	i4	i, cnt;

        cnt = gcu_words( tmp, nbuff, word, ',', 10 );

	for( i = 0; i < cnt; i++ )
	{
	    if ( ! STcasecmp( word[i], ERx("none") ) )
		IIGCn_static.flags &= ~GCN_BCHK_CONN;
	    else  if ( ! STcasecmp( word[i], ERx("connect") ) )
		IIGCn_static.flags |= GCN_BCHK_PERM;
	    else  if ( ! STcasecmp( word[i], ERx("install") ) )
		IIGCn_static.flags |= GCN_BCHK_INST;
	    else  if ( ! STcasecmp( word[i], ERx("class") ) )
		IIGCn_static.flags |= GCN_BCHK_CLASS;
	    else
	    {
		ER_ARGUMENT earg[2];

		earg[0].er_value = ERx("check_type");
		earg[0].er_size = STlength( earg[0].er_value );
		earg[1].er_value = word[i];
		earg[1].er_size = STlength( earg[1].er_value );

		gcu_erlog(0, 1, E_GC0106_GCN_CONFIG_VALUE, NULL, 2, (PTR)earg);
	    }
	}
    }

    gcn_get_pmsym( "!.check_interval", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVal( tmp, &IIGCn_static.bedcheck_interval );

    gcn_get_pmsym( "!.expire_interval", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVal( tmp, &IIGCn_static.ticket_exp_interval );

    gcn_get_pmsym( "!.ticket_expire", "", tmp, sizeof( tmp ) );
    if ( *tmp )  CVal( tmp, &IIGCn_static.ticket_exp_time );

    gcn_get_pmsym( "!.ticket_cache_size", "", tmp, sizeof( tmp ) );
    if ( *tmp )  
    {
	CVal( tmp, &IIGCn_static.ticket_cache_size );
	if ( IIGCn_static.ticket_cache_size > GCN_DFLT_L_TICKVEC )
	    IIGCn_static.ticket_cache_size = GCN_DFLT_L_TICKVEC;
    }

    return( OK );
}


/*
** Name: gcn_srv_term
**
** Description:
**	Free resources associated with name queues.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 4-Dec-95 (gordy)
**	    Created.
**	15-Jan-96 (gordy)
**	    Clear IIGCn_static in case we are re-initialized.
**	 7-Oct-98 (gordy)
**	    Drop server key.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS queue, own server key no longer saved.
*/

VOID
gcn_srv_term( VOID )
{
    /*
    ** Free all name queues.
    */
    while( IIGCn_static.name_queues.q_next != &IIGCn_static.name_queues )
	gcn_nq_drop( (GCN_QUE_NAME *)IIGCn_static.name_queues.q_next );

    MEfill( sizeof( GCN_STATIC ), (u_char)0, (PTR)&IIGCn_static );

    return;
}


/*{
** Name: gcn_load_file
**
** Description:
**	Read a name queue definition file and initialize name queues.
**
** Input:
**	filename	Name of definition file.
**	create		TRUE if file should be created if does not exist.
**	host		Host node name.
**	myaddr		Name Server listen address, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	 5-Jun-98 (gordy)
**	    Extracted from gcn_svr_load().
** 	11-Aug-2003 (wonst02)
** 	    Add variable to a loop that was modifying the loop control variable,
** 	    which was causing some of the keywords to be skipped.
**	 3-Aug-09 (gordy)
**	    Remove invalid symbol reference for line buffer size.
**	    128 is sufficient line length for Name Server files.
*/	

static STATUS
gcn_load_file( char *filename, bool create, char *host, char *myaddr )
{
    FILE		*fptr;
    char		buf[ 128 ];
    STATUS		status = OK;

    /*
    ** Open file for reading.  If it does not exist and
    ** creation has been requested, open file for writing
    ** and initialize.
    */
    if ( gcn_fopen( filename, "r", &fptr, 0 ) != OK )
	if ( ! create  ||  gcn_fopen( filename, "w", &fptr, 0 ) != OK )
	{
	    ER_ARGUMENT	earg[1];

	    earg->er_value = filename;
	    earg->er_size = STlength( filename );
	    gcu_erlog( 0, IIGCn_static.language, 
		       E_GC0124_GCN_NO_IINAME, NULL, 1, (PTR)earg );
	    return( FAIL );
	}
	else
	{
	    /*
	    ** Initialize file with no entries.
	    */
	    SIfprintf( fptr, "#\n" );
	    SIfprintf( fptr, "# Name: %s\n", filename );
	    SIfprintf( fptr, "#\n" );
	    SIfprintf( fptr, "# Description:\n" );
	    SIfprintf( fptr, "#\tInternal Name Server file: do not modify.\n" );
	    SIfprintf( fptr, "#\tSee iiname.all for further information.\n" );
	    SIfprintf( fptr, "#\n" );
	    SIclose( fptr );
	    return( OK );	/* We be done */
	}

    while( SIgetrec( buf, sizeof( buf ), fptr ) != ENDFILE )
    {
	GCN_QUE_NAME	*nq;
	bool		global = FALSE;
	bool		transient = FALSE;
	bool		temporary = FALSE;
	bool		common;
	char		fname[MAX_LOC];
	char		*p, *words[16];
	i4		wcount = 15;
	i4		i, subfields = 1;

	/*
	** Parse line from definition file.
	*/
	if ( p = STchr( buf, '#' ) )  *p = '\0';
	if ( (p = buf + STlength(buf)) != buf && p[-1] == '\n' )  p[-1] = '\0';

	STgetwords( buf, &wcount, words );
	if ( ! wcount )  continue;

	for( i = 1; i < wcount; i++ )
	    if ( ! STcasecmp( ERx("global"), words[i] ) )
		global = TRUE;
	    else  if ( ! STcasecmp( ERx("transient"), words[i] ) )
		transient = TRUE;
	    else  if ( ! STcasecmp( ERx("temporary"), words[i] ) )
		temporary = TRUE;
	    else  if ( CMdigit( words[i] ) )
	    {
	    	i4	num;
		if ( CVan( words[i], &num ) == OK )  subfields = num;
	    }

	common = (IIGCn_static.flags & GCN_CLUSTERED  &&  global);

	/*
	** Initialize name queue for this type.
	*/
	if ( ! temporary )  
	    gcn_nq_file( words[0], common ? NULL : host, fname );

	if ( ! (nq = gcn_nq_init( words[0], temporary ? NULL : fname, 
				  common, transient, subfields )) )
	{
	    status = FAIL;
	    break;
	}

	/* 
	** Delete any servers listening at name server address
	*/
	if ( nq->gcn_transient  &&  myaddr )
	{
	    GCN_TUP	masktup;

	    masktup.uid = masktup.obj = "";
	    masktup.val = myaddr;

	    if ( gcn_nq_lock( nq, TRUE ) != OK )  
	    {
		status = FAIL;
		break;
	    }

	    gcn_nq_del( nq, &masktup );
	    gcn_nq_unlock( nq );
	}
    }

    SIclose( fptr );

    return( status );
}


/*
** Name: gcn_srv_load
**
** Description:
**	Reads the name queue definition files (iiname.all, svrclass.nam)
**	and initializes the name queues.
**
** Inputs:
**	host		Host node name.
**	myaddr		Name Server listen address, may be NULL.
**			  
** Side effects:
**	Global name queues built.
**
** History:
**	21-Mar-89 (seiwald)
**	    Written.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	29-Nov-95 (gordy)
**	    Allow address to be NULL to support embedded Name Server.
**	    Removed cluster parameter, use IIGCn_static.clustered.
**	26-Feb-96 (gordy)
**	    Extracted disk filename formation to function to handle
**	    platform dependencies.
**	29-May-97 (gordy)
**	    Delete registered server info matching name server address.
**	15-Aug-97 (rajus01)
**	    Extended vnode database to specify a distibuted
**	    security mechanism for a vnode. The GCN_ATTR name queue is
**	    added.
**	 5-Jun-98 (gordy)
**	    Separated static and transient queue definitions to make
**	    transient queue definitions dynamic.
**	16-Jun-98 (gordy)
**	    Added hashed access to Name Queues.
**	14-Sep-98 (gordy)
**	    Register self in installation registry.
**	 7-Oct-98 (gordy)
**	    Create own server key for registration.
**      13-Jan-1999 (fanra01)
**          Set uid flag for registered name server.
**	22-Mar-02 (gordy)
**	    Use NS classes rather than deprecated GCA classes.
**	 1-Mar-06 (gordy)
**	    Local Server Key no longer saved, only registered servers.
*/	

STATUS
gcn_srv_load( char *host, char *myaddr )
{
    GCN_QUE_NAME	*nq;
    STATUS		status;
    i4			i;

    /*
    ** If the name queue is not empty then we have
    ** already been called and there is nothing to do.
    */
    if ( IIGCn_static.name_queues.q_next != &IIGCn_static.name_queues )  
	return( OK );

    /* 
    ** Read static queue types in iiname.all and 
    ** initialize name queue for each type.
    */
    if ( (status = gcn_load_file( GCN_QUE_FNAME, FALSE, host, myaddr )) != OK )
	return( status );

    /* 
    ** Read server types in svrclass.nam and 
    ** initialize name queue for each type.
    */
    if ( (status = gcn_load_file( GCN_SVR_FNAME, TRUE, host, myaddr )) != OK )
	return( status );

    /*
    ** Initialize hashed access to various queues.
    */
    for( i = 0; i < HASH_INFO_SIZE; i++ )
	if ( (nq = gcn_nq_find( hash_info[i].type ))  &&
	     (status = gcn_nq_hash( nq, hash_info[i].func, 
				        hash_info[i].size )) != OK )
	    return( status );

    /*
    ** Register self in Name Server queue.
    */
    if ( myaddr )
    {
	GCN_TUP		nstup;
	STATUS		status;

	/*
	** Register as the Name Server in installation.
	*/
	if ( (nq = gcn_nq_find(GCN_NMSVR))  &&  gcn_nq_lock(nq, TRUE) == OK )
	{
	    char	flag_buff[9];

	    CVlx( GCN_PUB_FLAG | GCA_RG_IINMSVR | GCA_RG_TRAP, flag_buff );
	    nstup.uid = flag_buff;
	    nstup.obj = ERx("*");
	    nstup.val = myaddr;

	    gcn_nq_add( nq, &nstup );
	    gcn_nq_unlock( nq );
	}

	/*
	** Register as registry Name Server for installation.
	*/
	if ( (nq = gcn_nq_find(GCN_NS_REG))  &&  gcn_nq_lock(nq, TRUE) == OK )
	{
	    char	flag_buff[9];

	    CVlx( GCN_PUB_FLAG | GCA_RG_NMSVR, flag_buff );
	    nstup.uid = flag_buff;
	    nstup.obj = IIGCn_static.install_id;
	    nstup.val = myaddr;

	    gcn_nq_add( nq, &nstup );
	    gcn_nq_unlock( nq );
	}
    }

    return( OK );
}


/*
** Name: gcn_init_mask
**
** Description:
**	Initialize a pseudo-random byte mask based on a key.
**
** Input:
**	key	Key string.
**	len	Number of bytes in mask.
**
** Output:
**	mask	Pseudo-random bytes.
**
** Returns:
**	void
**
** History:
**	15-Jul-04 (gordy)
**	    Created.
*/

#define	MASK_HASH( hash, chr )	( (((u_i4)(hash) << 4) ^ ((chr) & 0xFF)) \
				  ^ ((u_i4)(hash) >> 28) )
#define	MASK_MOD( val )		((val) % 714025L)
#define MASK_SEED( seed )	MASK_MOD(seed)
#define	MASK_RAND( seed )	MASK_MOD((seed) * 4096L + 150889L)

void
gcn_init_mask( char *key, i4 len, u_i1 *mask )
{
    u_i4	rand, carry, i;
    u_i4	seed = 0;

    for( i = 0; key[i]; i++ )  seed = MASK_HASH( seed, key[i] );
    rand = MASK_SEED( seed );

    for( i = 0; i < len; i++ )
    {
	carry = rand;
	rand = MASK_RAND( rand );
	mask[i] = (u_i1)((rand >> 8) ^ carry);
    }

    return;
}


/*
** Name: gcn_get_pmsym() - get PM resources from the resource file
**
** Description:
**	gcn_get_pmsym() makes PM calls to get the resource name
**
**	gcn_get_pmsym() copies the symbol's value into the result buffer.  If
**	the symbol is not set, the default value is copied.
**
** Inputs:
**	sym	- symbol to look up
**	dflt	- default result
**	len	- max length of result
**
** Outputs:
**	result	- value of symbol placed here
**
** History:
**	04-Dec-92 (gautam)
**	    Written
*/	

static VOID
gcn_get_pmsym( char *sym, char *dflt, char *result, i4  len )
{
    char *buf, *val;

    if ( PMget( sym,  &buf) != OK )
	STncpy( result, dflt, len - 1);
    else
	STncpy( result, buf, len - 1 );
    result[ len - 1 ] = EOS;

    return;
}


/*
** Name: ticket_hash
**
** Description:
**	Produces an 8 bit hash value using the string encoded value
**	of a ticket.  
**
**	Binary values encoded as strings using CItotext() do not 
**	vary much in the upper four bits.  To provide better hash 
**	distribution, the hash value is first primed in the upper 
**	and lower nibbles using the first character of the ticket.  
**	The remaining characters in the ticket are then combined 
**	with the hash value, rotating (not just shifting) the value
**	one bit for each character.
**
** Input:
**	tup		Name Queue tuple.
**
** Output:
**	None.
**
** Returns:
**	GCN_HASH	8 bit hash value.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
**	15-Sep-98 (gordy)
**	    Make hash case insensitive to match Name Server semantics.
*/

static GCN_HASH
ticket_hash( GCN_TUP *tup )
{
    GCN_HASH	hval;
    char	*p, cval[2];

    /*
    ** Ticket is first element of tuple value.  Find the end.
    */
    if ( ! (p = STchr( tup->val, ',' )) )
	p = tup->val + STlength( tup->val );

    /*
    ** Prime the hash value using first character.
    */
    CMtoupper( tup->val, cval );
    hval = *cval ^ (*cval << 4);

    /*
    ** Now combine rest of ticket into hash value.
    */
    while( --p > tup->val )
    {
	CMtoupper( p, cval );

	if ( hval & 0x80 )
	    hval = ((hval << 1) | 0x01) ^ *cval;
	else
	    hval = (hval << 1) ^ *cval;
    }

    return( hval );
}


/*
** Name: obj_uid_hash
**
** Description:
**	Produces an 8 bit hash value using the uid and obj
**	elements of a tuple.
**
** Input:
**	tup		Name Queue tuple.
**
** Output:
**	None.
**
** Returns:
**	GCN_HASH	8 bit hash value.
**
** History:
**	16-Jun-98 (gordy)
**	    Created.
**	15-Sep-98 (gordy)
**	    Make hash case insensitive to match Name Server semantics.
*/

static GCN_HASH
obj_uid_hash( GCN_TUP *tup )
{
    GCN_HASH	hval;
    char	*p, cval[2];

    /*
    ** Prime the hash value using first character of uid.
    */
    CMtoupper( tup->uid, cval );
    hval = *cval << 3;

    /*
    ** Combine the uid and obj into the hash value.
    */
    for( p = tup->uid; *p; p++ )
    {
	CMtoupper( p, cval );

	if ( hval & 0x80 )
	    hval = ((hval << 1) | 0x01) ^ *cval;
	else
	    hval = (hval << 1) ^ *cval;
    }

    for( p = tup->obj; *p; p++ )
    {
	CMtoupper( p, cval );

	if ( hval & 0x80 )
	    hval = ((hval << 1) | 0x01) ^ *cval;
	else
	    hval = (hval << 1) ^ *cval;
    }

    return( hval );
}
