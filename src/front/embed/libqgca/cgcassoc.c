/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<id.h>
# include	<er.h>
# include	<st.h>
# include	<me.h>
# include	<mh.h>
# include	<mu.h>
# include	<nm.h>
# include	<cm.h>
# include	<cv.h>
# include	<lo.h>
# include	<pc.h>
#ifndef NT_GENERIC
#include	<unistd.h>
#endif
# include	<si.h>
# include	<tm.h>
# include	<tmtz.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<adf.h>
# include	<generr.h>
# include	<erlc.h>
# include	<erlq.h>

/**
+*  Name: cgcassoc.c - This file contains routines to associate with GCA.
**
**  Description:
**	The routines in this file set up (or reset) the association between
**	the client (as seen by LIBQ) and GCA.
**
**  Defines:
**
**	IIcgc_connect - 	Connect with server (GCA_INITIATE, GCA_REQUEST).
**	IIcgc_tail_connect - 	Tail end of server connection for gateways.
**	IIcgc_disconnect -	End session (GCA_RELEASE,_DISASSOC,_TERMINATE).
**	IIcgc_alloc_cgc	-	Allocate IICGC_DESC.
**	IIcgc_save - 		Save GCA and user state (GCA_SAVE).
**	IIcc1_connect_params - 	Parse and send modifiers (GCA_MD_ASSOC).
**	IIcc2_connect_restore -	Restore parent GCA and user state (GCA_RESTORE).
**	IIcc3_discon_service - 	Invoke the disconnect services. 
-*
**  History:
**	28-jul-1987	- Written (ncg)
**	08-feb-1988	- Added IIcgc_tail_connect (ncg)
**	23-may-1989	- Added support for multiple database connections. (bjb)
**	03-aug-1989	- Bug 6651: GCA_ATTRIBS service now after tail connect.
**				(barbara)
**	23-feb-1990	(barbara)
**	    Integrated daveb's connection retry code into the 63 path.
**	01-mar-1990	(barbara)
**	    Fixed bug with setup for GCA_REQUEST call.
**	14-mar-1990	(barbara)
**	    Fixed bug regarding use of null char pointer dervied from
**	    NMgtAt of II_%s_PATH logical.
**	20-apr-1990	(barbara)
**	    Added memory allocation error E_LC0014_ASSOC_ALLOC.
**	15-jun-1990	(barbara)
**	    Fixed to bug 21832 allows program to continue after association
**	    failures.  Changed disconnect code to allow a specialized
**	    disconnection if the connection has already gone away.
**	    Also changed minimum buffer size in keeping with GordownW's
**	    performance changes.
**	05-jul-1990	(barbara)
**	    Fixed bug 31278.  When GCA_REQUEST service fails, save assoc_id
**	    immediately for use in IIcc3_discon_service.  Otherwise, we
**	    always disconnect the sessions whose internal id is 0.
**	05-sep-1990     (barbara)
**	    Send down timezone information as another session parameter.
**	21-sep-1990	(barbara)
**	    Timezone info now depends on peer protocol level and II_EMBED_SET.
**	    Added new routine IIcgc_alloc_cgc as part of change to read
**	    II_EMBED_SET logicals before connection.
**	03-oct-1990	(barbara)
**	    Backed out time zone change until further notice from dbms group.
**      09-nov-1990     (teresal)
**          Moved the parsing of ADF type format flags to IIdbconnect where
**          the ADF control block is now updated with ADF format
**          flags information . A benefit of setting the ADF control
**          block is that this fixes a COPY problem so an embedded
**          program can copy a large float number without loss of precision
**          if the user specifies an appropriate `-f` flag at connect time.
**          Bug 33675.
**	14-nov-1990	(barbara)
**	    Dbms group says it's time to go with timezone fix.
**	08-mar-1991 (barbara)
**	    Raised protocol level to 40.
**	01-aug-1991 (teresal)
**	    Raised protocol level to 50 for new copy map to fix
**	    bug 37564.  Also, reset protocol level for GCA trace 
**	    mechanism after estabishing a peer protocol level
**	    at connect time.
**	15-aug-1991 (kathryn) Bug 39172
**	    Changed IIcc1_connect_params() to ensure that startup flags which
**	    have zero-length character string arguments are sent as varchar 
**	    type (DB_VCH_TYPE) rather than fixed length type (DB_CHR_TYPE). 
**	    This occurs when startup flags which expect a string value are 
**	    given no argument.
**	    EX: "sql -G dbname" rather than the expected "sql -Ggroupid dbname".
**	24-mar-1992 (leighb) DeskTop Porting Change:
**	    Added conditional function pointer prototypes.
**	12-nov-1992 (teresal)
**	    Decimal changes. Set the new ADF protocol level field which
**	    is used by ADF to determine if decimal is a valid type
**	    Note: Decimal is supported as of protocol level 60.
**      31-mar-1993 (smc)
**          Fixed up prototype of cgc_handle in IIcgc_alloc_cgc.
**	22-jul-1993 (seiwald)
**	    New functionality at API_LEVEL 4: passing remote system user
**	    name and password on the connect line. If IIcgc_connect()
**	    is passed in remote_username and remote_passwd, turn on
**	    the GCA_RQ_REMPW flag in gca_modifiers and set the fields
**	    in GCA_RQ_PARMS.
**	12-oct-93 (donc) Bug 55431
**	    Add support for -string_truncation = {ignore|fail} and
**	    numeric_overflow = {ignore|warn|fail}
**	20-oct-93 (donc)
**	    set adf_proto_level for GCA_PROTOCOL_LEVEL = 60 to have bits for
**	    Decimal, Byte, and Long datatypes turned on, as this is when these 
**	    datatypes became supported.  
**	11/22/93 (dkh) - Fixed 56062.  Saved the timezone name so that
**			 subsequent calls to NMgtAt does not cause the
**			 information to be lost.
**	14-nov-95 (nick)
**	    Add support for II_DATE_CENTURY_BOUNDARY.
**	25-Jul-96 (gordy)
**	    Added support for local username and password.
**	27-jul-1996 (canor01)
**	    When in a connect retry loop, only call IIcc3_discon_service
**	    on last retry, since it releases all GCA resources.
**	 1-Aug-96 (gordy)
**	    When attempting to change local user, set GCA_RQ_SERVER flag
**	    to indicate we are acting as a server on behalf of a client.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	09-jan-98 (mcgem01)
**	    Move the GLOBALDEF of IIgca_cb to the newly created cgcdata file.
**      28-Jul-1998 (carsu07)
**          Allow users to run setuid applications as the process owner and
**          not their own userid. (Bug #77917)
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	29-Apr-1999 (andbr08)
**	    Check if GCA_PROTOCOL_LEVEL_60 before passing the
**	    II_NUMERIC_LITERAL flag.
**	15-Jul-1999(rigka01)
**	    Bug# 97950 - ingmenu fails with illegal instruction in
**	    IIcgc_connect() on some machines
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-May-01 (gordy)
**	    Bumped GCA protocol level to 64 for national character sets.
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	15-Mar-07 (gordy)
**	    Bumped protocol level for scrollable cursors.
**	 1-Oct-09 (gordy)
**	    Bumped protocol level for GCA2_INVPROC message.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**      12-mar-2010 (joea)
**          Set AD_BOOLEAN_PROTO in cgc_connect if at GCA_PROTOCOL_LEVEL_68.
**/

/* Default message buffer size */
# define IICC__MINBUFSIZE	1024

static FILE	*logfp = NULL;
static PID	mypid;

GLOBALREF PTR	IIgca_cb;



/*
** Name: IIcgc_init
**
** Description:
**
** Input:
**	cgc_handle	- Caller function handler.
**
** Output:
**	hdr_len		GCA message buffer header length.
**
** Returns:
**
** History:
**	18-May-01 (gordy)
**	    Bumped GCA protocol level to 64 for national character sets.
**	4-Feb-2005 (schka24)
**	    Bumped protocol level (somewhat tardily) for i8 support.
**	6-feb-2007 (dougi)
**	    Bumped protocol level (equally tardily) for date/time support.
**	15-Mar-07 (gordy)
**	    Bumped protocol level for scrollable cursors.
**	 1-Oct-09 (gordy)
**	    Bumped protocol level for GCA2_INVPROC message.
*/

STATUS
IIcgc_init( i4  (*cgc_handle)( i4, IICGC_HARG * ), i4  *hdr_len )
{
    GCA_PARMLIST	gca_parm;
    GCA_IN_PARMS	*gca_int = &gca_parm.gca_in_parms;
    IICGC_DESC		cgc;
    STATUS		gca_stat;
    LOCATION		loc;
    char		locbuf[ MAX_LOC + 1 ];
    char		*ptr;
    TM_STAMP		time_stamp;
    char		time_str[32];

    cgc.cgc_handle = cgc_handle;

    MEfill( sizeof( gca_parm ), EOS, (PTR)&gca_parm );
    gca_int->gca_local_protocol	= GCA_PROTOCOL_LEVEL_68;
    gca_int->gca_modifiers = GCA_SYN_SVC;

    IIGCa_cb_call( &IIgca_cb, GCA_INITIATE, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if ( gca_int->gca_status != E_GC0000_OK  &&  gca_stat == E_GC0000_OK )  
	gca_stat = gca_int->gca_status;

    if ( gca_stat != E_GC0000_OK )
    {
	char	stat_buf[IICGC_ELEN + 1];
	IIcc3_util_status( gca_stat, &gca_int->gca_os_status, stat_buf );
	IIcc1_util_error( &cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
			  ERx("GCA_INITIATE"), stat_buf, NULL, NULL );
	return( FAIL );
    }

    *hdr_len = gca_int->gca_header_length;	/* Save header length */

    MHsrand( TMsecs() );
    NMgtAt( ERx("II_CONNECT_LOG"), &ptr );

    if ( ptr  &&  *ptr )
    {
	PCpid( &mypid );
	STlcopy( ptr, locbuf, sizeof(locbuf)-1 );
	LOfroms( PATH | FILENAME, locbuf, &loc );

	if ( SIfopen( &loc, ERx("a"), SI_TXT, 80, &logfp ) != OK )
	    logfp = NULL;
	else
	{
	    TMget_stamp( &time_stamp );
	    TMstamp_str( &time_stamp, time_str );
	    SIfprintf( logfp, ERx("Process %d (0x%x) at %s\n"),
		       mypid, mypid, time_str );
	    SIflush( logfp );
	}
    }
    
    return( OK );
}


/*
** Name: IIcgc_term
**
** Description:
**
** Input:
**
** Output:
** 
** Returns:
**
** History:
*/

VOID
IIcgc_term()
{
    GCA_PARMLIST	gca_parm;
    STATUS		gca_stat;

    MEfill( sizeof( gca_parm ), EOS, (PTR)&gca_parm );

    IIGCa_cb_call( &IIgca_cb, GCA_TERMINATE, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if ( logfp )
    {
	SIclose( logfp );
	logfp = NULL;
    }

    return;
}

 
/*{
**  Name: IIcgc_connect - Start up GCA session and connect to server.
**
**  Description:
**	Allocate internal buffers, set up the GCA association (GCA_INITIATE
**	service), and request a connection with the server (GCA_REQUEST
**	service).  If the connection is successful then:
**	(1) Format the "write" message buffer (GCA_FORMAT service) for the
**	    rest of the session,
**	(2) Pass any command-line parameters to the server (using the
**	    GCA_MD_ASSOC message).
**
**	If this is a spawned process (-X flag) then do not re-request a
**	connection with the server - instead restore the parent's saved state
**	(GCA_RESTORE service).
**
**	Only invoke the GCA_INITIATE service if this is the first
**	connection.   Subsequent database connections in a multiple-connection
**	application will share the same GCA session.
**
**	The last part of the connection is done by IIcgc_tail_connect.
**	This is so that gateway parameters may be sent after command-line 
**	parameters are sent and then the connection can be completed.  The
**	CONNECT syntax includes an OPTIONS list (command-line parameters)
**	and a WITH clause (gateway parameters).
**
**	If errors occur before the connection to the database has completed
**	the GCA_TERMINATE service is invoked only if this is the last
**	database connection.  See comments in IIcc3_discon_service().
**
**  Inputs:
**	argc		- Argument count of following array.
**	argv		- Array of user's command line arguments - null 
**			  terminated and blank trimmed.
**	query_lang	- Query language of session startup.
**	hdrlen		- Length of header for message buffer allocation.
**	gateway		- If set then this routine does NOT complete the
**			  startup protocol, but leaves it to later, when
**			  IIcgc_tail_connect will be called.
**	sendzone	- II_EMBED_SET was not set to "notimezone".  Okay to
**			  send time zone information.
**	dbname		- User specified dbname without all internal syntax.
**			  This may be modified upon return to remove the 
**			  operators for nodes, etc.
**	adf_cb		- ADF control block to pass to presentation layer.
**	cgc_handle	- Caller function handler.
**	lcl_username	- Local username for connection, may be NULL.
**	lcl_passwd	- Local password for local username, may be NULL.
**	rem_username	- Remote username for connection, may be NULL.
**	rem_passwd	- Remote password for remote username, may be NULL.
**
**  Outputs:
**	dbname		- Buffer for database name set by this routine.  May
**			  be filled with up to MAX_LOC characters.
**	cgc		- Pointer to client general communications descriptor
**			  allocated and set by this routine.
**
**	Returns:
**		STATUS	- Returned from the CL during startup phase.
**	Errors:
**		Memory allocation CL errors.
**		E_LC0001_SERVICE_FAIL - GCA INITIATE/REQUEST/FORMAT failed.
**		E_LC0010_ASSOC_REJECT - Association request rejected.
**		E_LC0013_ASSOC_NO_NAME - DMBS session name not valid.
**              E_LC0035_BAD_RETRY_VALUE - II_CONNECT_RETRIES is bogus value.
**              E_LC0036_BAD_RETRY_SYNTAX - bogus syntax in II_CONNECT_RETRIES
**
**  Side Effects:
**	None
**	
**  History:
**	29-aug-1986	- Written (ncg)
**	27-jul-1987	- Rewritten for GCA (ncg)
**	03-feb-1988	- Modified to issue one GCA_FORMAT here instead of
**			  multiple ones in IIcgc_init_msg. (ncg)
**	05-feb-1988	- Extracted out last part of startup to allow
**			  gateway parameters. (ncg)
**	22-jun-1988	- Added a some new parameters for INGRES/NET and
**			  modified some of the GCA service startup calls. (ncg)
**	10-aug-1988	- Allowed II_DBMS_SERVER in the new protocol as well
**			  to override the database name. (ncg)
**	18-apr-1989	- Record into caller's communications descriptor 
**			  (after ATTRIBS) whether assoc is heterogenous. (bjb)
**	23-may-1989	- Added support for multiple connections. (bjb)
**	03-aug-1989	- Bug #6651 don't invoke ATTRIBS until after connection
**			  is completed.  
**	26-jan-1990	- Clean up status checking for GCA_ATTRIBS. (bjb)
**	23-feb-1990	(barbara)
**	    Integrated daveb's connection retry code into the 63 path.
**	01-mar-1990	(barbara)
**	    Fixed bug with setup for GCA_REQUEST call.  GCA_NO_XLATE flag was
**	    always getting passed.
**	14-mar-1990	(barbara)
**	    Fixed bug regarding use of null char pointer dervied from
**	    NMgtAt of II_%s_PATH logical.
**	20-apr-1990	(barbara)
**	    Memory allocation errors now have their own LIBQGCA error.
**	    Previously they went through LIBQ where they were printed
**	    to the screen.
**	05-jul-1990	(barbara)
**	    Fixed bug 31278.  When GCA_REQUEST service fails, save assoc_id
**	    immediately for use in IIcc3_discon_service.  Otherwise, we
**	    always disconnect the sessions whose internal id is 0.
**	08-Jun-90 (GordonW)
**	    Added a SIflush after writting to the II_CONNECT_LOG
**	    so when we are tracing we get up-to-date entries.
**	    Also added a process id stamp to each log entry, as
**	    well as time stamps.
**	21-sep-1990	(barbara)
**	    IICGC_DESC is no longer allocated in this routine.  IIdbconnect
**	    calls a separate routine to preallocate it.
**	03-oct-1990
**	    Backed out time zone change until further notice from dbms group.
**	14-nov-1990
**	    DBMS says it's time to go with timezone fix.
**	17-jun-92 (leighb) DeskTop Porting Change:
**	    Changed "try" to "tryctr" to avoid name conflict in WIN/NT.
**	01-jul-92 (leighb) DeskTop Porting Change:
**	    Set partner_name = NULL so SIfprintf won't blow up.
**      10-nov-94 (tutto01) Bug #64692 Incorrect logging information fixed.
**          Log file defined by II_CONNECT_LOG was reporting successful
**          connections to non-existent databases.
**	7-apr-93 (robf)
**	    Secure 2.0: Default to protocol level 61.
**	14-nov-95 (nick)
**	    Default to protocol level 62.
**	25-Jul-96 (gordy)
**	    Added support for local username and password.
**	27-jul-1996 (canor01)
**	    When in a connect retry loop, only call IIcc3_discon_service
**	    on last retry, since it releases all GCA resources.
**	 1-Aug-96 (gordy)
**	    When attempting to change local user, set GCA_RQ_SERVER flag
**	    to indicate we are acting as a server on behalf of a client.
**     28-Jul-1998 (carsu07)
**          Allow users to run setuid applications as the process owner and
**          not their own userid.  Removed the userid when making the
**          connection. (Cross integrate fix for Bug #77917)
**	15-Jul-1999(rigka01)
**	    Bug# 97950 - ingmenu fails with illegal instruction in
**          IIcgc_connect() on some machines.  Use variable storage for	
**	    the initial setting of partner_name instead of initializing
**	    it to a literal string.
**	18-May-01 (gordy)
**	    Bumped GCA protocol level to 64 for national character sets.
**      18-Apr-2002 (hanal04) Bug 81521 INGSRV 1580
**          II_DBMS_SERVER must not over-ride connections to the
**          STAR server.
**	23-Jul-2007 (smeke01) b118798.
**	    Added adf_proto_level flag value AD_I8_PROTO. adf_proto_level 
**	    has AD_I8_PROTO set iff the client session can cope with i8s, 
**	    which is the case if the client session is at GCA_PROTOCOL_LEVEL_65 
**	    or greater. 
**      26-Nov-2009 (hanal04) Bug 122938
**          Different GCA protocol levels can handle different levels
**          of decimal precision.
**      30-Nov-2010 (hanal04) Bug 124758
**          Different GCA protocol levels expect different object name
**          lengths for example table and owner names in an SQL call to
**          resolve_table().
*/

STATUS
IIcgc_connect(argc, argv, query_lang, hdrlen, gateway,
		sendzone, dbname, adf_cb, cgc_handle, cgc,
	        lcl_username, lcl_passwd, rem_username, rem_passwd)
i4		argc;
char		**argv;
i4		query_lang;
i4		hdrlen;
i4		gateway;
i4		sendzone;
char		*dbname;
ADF_CB		*adf_cb;
i4		(*cgc_handle)();
IICGC_DESC	*cgc;
char		*lcl_username;
char		*lcl_passwd;
char		*rem_username;
char		*rem_passwd;
{
    GCA_PARMLIST	*gca_parm;	/* General GCA parameter */
    GCA_RQ_PARMS	*gca_req;	/* GCA_REQUEST parameter */
    GCA_FO_PARMS	*gca_fmt;	/* GCA_FORMAT parameter */
    GCA_AT_PARMS	*gca_att;	/* GCA_ATTRIBS parameter */
    STATUS		gca_stat;	/* Status for IIGCa_call+ME routines */
    i4		buf_size;	/* Advised size for message buffer */
    char		*alloc_buf;	/* Temp alloc for message buffers */
    char		*save_name;	/* Saved name for -X flags */
    i4			save_ind;	/* Index to find -X flag */
    char		stat_buf[IICGC_ELEN + 1];	/* Status buffer */
    char		pn[MAX_LOC+1], *partner_name;
    u_i4		sleep_time;

    /* stuff added for retries and vnode rollover (daveb) */
# define MAX_VNODES     16
# define MAX_NODE_LEN   64      /* same value used in netu */
# define DEF_TRIES      1

    i4  i;              /* inner loop counter */
    i4  j;              /* inner loop bound */
    i4  last;           /* is this the last pass through the loop? */
    char *p;            /* handy ptr */
    char *q;            /* handy ptr */
    char *d;            /* ptr to database in dbname */
    char *e;            /* ptr to vnode list */

    bool connected;     /* are we connected yet? */
    i4  tryctr;         /* outer loop counter */	 
    i4  max_tries;      /* max number of tries */
    i4  nv;             /* number of vnodes to try */
    bool retry_delay;   /* should we delay retries? */

    /* vnodes to try in inner loop */
    char vnodes[ MAX_VNODES ][ MAX_NODE_LEN ];

    char sym[ 256 ];               /* II_bnode_PATH symbol name */
    char bnode[ MAX_NODE_LEN ];    /* base node in name */
    char nodename[ MAX_NODE_LEN ]; /* modified node name for printing */
    char mydbname[ 256 ];          /* actual vnode::dbname to try */
    char cntbuf[ 20 ];             /* retry count as characters */

    TM_STAMP    time_stamp;
    char        time_str[32];

    char inheritedconnection[] = "inherited connection"; 

    /* end rollover vars */
    alloc_buf = NULL;
    partner_name = inheritedconnection;  

    /* Save global state */
    cgc->cgc_query_lang = query_lang;
    cgc->cgc_handle = cgc_handle;	/* Assign function and debug handler */

    /*
    ** Set compile time protocol level.  This may be modified at GCA_REQUEST-
    ** time by our partner.
    */
    cgc->cgc_version = GCA_PROTOCOL_LEVEL_64;

    gca_parm = &cgc->cgc_write_msg.cgc_parm;

    /*
    ** Check if this is an association with a child process.  If so then
    ** restore the environment and do NOT re-request a connection.
    */
    for (save_ind = 0, save_name = (char *)0; save_ind < argc; save_ind++)
    {
	if (argv[save_ind][0] == '-' && argv[save_ind][1] == 'X')
	    save_name = argv[save_ind];
    }

    /* extract potential base vnode in dbname to bnode */
    for( p = dbname, q = bnode; *p && (p[0] != ':' || p[1] != ':') ; )
    {
	CMcpychar( p, q );
	CMnext( p );
	CMnext( q );
    }
    *q = 0;

    /* if unadorned database, bnode should be empty, and reset p to dbname */
    if( *p != ':' )
    {
	p = dbname;
	bnode[0] = 0;
    }

    /* find the database name, and leave pointer in d */
    while( *p == ':' )
    	CMnext( p );
    d = p;

    /*
    ** Interpret the II_CONNECT_RETRIES variable, whose syntax must be:
    **
    **          number[d|D]
    **
    ** Otherwise an error results.
    */

    max_tries = DEF_TRIES;
    NMgtAt( ERx("II_CONNECT_RETRIES"), &e );
    p = e;
    if( e && *e )
    {
	/* copy digits into buffer for CVan, which doesn't like
	   the optional trailing 'd' */
	for( q = cntbuf ; p && *p && CMdigit(p) ;  )
	{
	    CMcpychar( p, q );
	    CMnext(p);
	    CMnext(q);
	}
	*q = 0;

	max_tries = 0;
	if( p == e || OK != CVan( cntbuf, &max_tries ) || max_tries <= 0 )
	{
	    if(logfp)
	    {
		SIfprintf(logfp,
		ERx("%d: II_CONNECT_RETRIES value '%s' isn't a good number\n"),
		    mypid, e);
		SIflush(logfp);
	    }
	    /* error about var syntax, value in e. */
	    IIcc1_util_error(cgc, TRUE, E_LC0035_BAD_RETRY_VALUE, 1,
			     e, (char *)0,(char *)0, (char *)0);
	    return FAIL;
	}
    }

    /* if anything left, it must be 'd' or 'D', else it's an error */
    retry_delay = FALSE;
    if( p && *p )
    {
	if( p[1] != EOS || (*p != 'd' && *p != 'D') )
	{
	    if(logfp)
	    {
		SIfprintf(logfp,
	      ERx("%d: II_CONNECT_RETRIES value '%s' doesn't end with [d|D]\n"),
		      mypid, e );
	    	IIcc1_util_error(cgc, TRUE, E_LC0036_BAD_RETRY_SYNTAX, 1,
			     e, (char *)0, (char *)0, (char *)0);
		SIflush(logfp);
	    }
	    return FAIL;
	}
	retry_delay = TRUE;
    }
    /*
    ** Look for a symbol of the type II_{NODE}_PATH, with vnodes
    ** to try for the node separated with "," commas.  For instance,
    **
    **  given dbname "svr::dbname" and II_SVR_PATH "0,1,2,3,4,5",
    **
    **  The code will try svr0, svr2, svr3, svr4, and svr5 in semi-random
    **  order.
    **
    **  We cheat and reuse space in vnodes[0] here
    */
    for( p = bnode, q = vnodes[0]; *p ; )
    {
	CMtoupper(p, q);
	CMnext( q );
	CMnext( p );
    }

    *q = 0;
    STprintf( sym, ERx("II_%s_PATH"), vnodes[0] );
    *vnodes[0] = 0;             /* clear this back out */
    NMgtAt( sym, &e );
 
    if(logfp)
    {
        STcopy(bnode,nodename);
        if (!STcompare(nodename,ERx("\0")))  {
           STcopy(ERx("local vnode"),nodename);
           SIfprintf(logfp,
             ERx("%d: Want to connect to local vnode, database:%s\n"), mypid,d);
        }
        else  {
        SIfprintf(logfp,
	    ERx("%d: _Want to connect to vnode:%s, database:%s\n"),
                mypid,nodename,d);
        }
	SIfprintf(logfp,
		ERx("%d: symbol '%s'='%s', max_tries %d, retry_delay %s\n"),
		mypid, sym, e, max_tries, retry_delay ? ERx("ON"):ERx("OFF") );
	SIflush(logfp);
    }

    /*
    ** Copy comma (",") separated vnode suffixes in e[]
    ** to the vnodes[] array.
    */
    for( nv = 0, p = e; p && *p && nv <= MAX_VNODES ; nv++)
    {
	/* extract the next vnode */
	for( q = vnodes[nv]; *p && *p != ',' ; )
	{
	    CMcpychar( p, q );
	    CMnext( p );
	    CMnext( q );
	}
	*q = 0;

	if( *p == ',' )
	    CMnext( p );
    }
    if(!nv)
    nv = 1;

    /* once for all retries */
    if ( ! lcl_username  ||  ! lcl_passwd )
    {
	lcl_username = NULL;
	lcl_passwd = NULL;
    }

    /*   
    ** Loop over each possible vnode max_tries times, unless some error
    ** causes you to leave (via return) earlier.  The variable "connected"
    ** will tell whether the connection succeeded.
    */
    connected = FALSE;
    for( tryctr = 0; !connected && tryctr < max_tries; tryctr++ )		 
    {
	/* Pick a random start in the vnode[] array.
	** MHrand is assumed to be in the range 0..1.  Unfortunately,
	** the CL spec doesn't say if that is correct.  If it is 0..MAXI?,
	** then (innat)(MHrand() % nv) is right, and you can't have it both
	** ways. 
	*/
	i = (i4)(MHrand() * nv);
	if( logfp && nv > 1 )
	{
	    SIfprintf(logfp,
		ERx("%d: Try %d, hunt starting at %s element %d: '%s'\n"),
			mypid, tryctr, sym, i, vnodes[i] );			 
	    SIflush(logfp);
	}

	/* if we want to delay, have it tend to increase with tries,
	** but make it slightly random as well.  More complicated schemes
	** are certainly possible. */
	if( retry_delay && tryctr > 1 )						 
	{
	    sleep_time = (u_i4) (tryctr + 1) * 1000;				 
	    if(logfp)
	    {
		SIfprintf(logfp,
		ERx("%d: We'll go into a %d msec sleep now...\n"),
			mypid, sleep_time);
		SIflush(logfp);
	    }
	    PCsleep( sleep_time );
	}

	for( j = nv; !connected && j-- ; i = ++i >= nv ? 0 : i )
	{
	    /* is this the last iteration? */
	    last = (tryctr + 1 == max_tries) && !j;				 
    	    if (save_name)
    	    {
		save_name += 2;			/* Skip the -X */
		if (IIcc2_connect_restore(cgc, save_name, hdrlen, dbname)
		    != OK)
		{
		    /* if this fails, don't retry */
		    return FAIL;
		}
		buf_size = cgc->cgc_write_msg.cgc_b_length;
		cgc->cgc_g_state |= IICGC_CHILD_MASK;
		connected = TRUE;
	    }
	    else  /* Is not a child process - no -X flag */
	    {
		/* any state we have is garbage, so clear out */
		cgc->cgc_m_state = 0;
		cgc->cgc_g_state = 0;

		NMgtAt(ERx("II_DBMS_SERVER"), &partner_name);
		if ((partner_name != (char *)0 && *partner_name != '\0') &&
                   ((STrstrindex(dbname, "/star", 0, TRUE)) == NULL) )
		{
		    /* Save partner name and later mark as not to translate */
		    STlcopy(partner_name, pn, MAX_LOC);
		    partner_name = pn;
		}
		else			/* II_DBMS_SERVER is not defined */
		{
		    /* use the next available vnode */
		    if( *vnodes[i] )
			STprintf( mydbname, ERx("%s%s::%s"),
				bnode, vnodes[i], d );
		    else
			STcopy( dbname, mydbname );
		    partner_name = mydbname;
		} /* No logical match */

		if( logfp )
		{
		    TMget_stamp(&time_stamp);
		    TMstamp_str(&time_stamp, time_str);
		    SIfprintf(logfp, ERx("%d: Trying %s at %s ...\n"),
			mypid, nodename, time_str  );
		    SIflush(logfp);
		}

		gca_req = &gca_parm->gca_rq_parms;
		gca_req->gca_partner_name	= partner_name;	/* In */
		gca_req->gca_user_name    	= lcl_username;	/* In */
		gca_req->gca_password     	= lcl_passwd;	/* In */
		gca_req->gca_account_name 	= (char *)0;	/* In */

		gca_req->gca_modifiers    	= 0;		/* In */
		if ( partner_name != mydbname )
		    gca_req->gca_modifiers |= GCA_NO_XLATE;
		if ( lcl_passwd )  
		    gca_req->gca_modifiers |= GCA_RQ_SERVER;

		gca_req->gca_assoc_id     	= 0;		/* Out */
		gca_req->gca_size_advise  	= 0;		/* Out */
		gca_req->gca_peer_protocol 	= GCA_PROTOCOL_LEVEL_61; /* Out */
		gca_req->gca_flags        	= 0;		/* Out */
		gca_req->gca_status       	= E_GC0000_OK;	/* Out */

		if (rem_username && rem_passwd)
		{
		       gca_req->gca_rem_username = rem_username; /* In */
		       gca_req->gca_rem_password = rem_passwd;	 /* In */
		       gca_req->gca_modifiers  |= GCA_RQ_REMPW ;
		}

		IIGCa_cb_call( &IIgca_cb, GCA_REQUEST, gca_parm, 
			       GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

		/* Save association id */
		cgc->cgc_assoc_id = gca_req->gca_assoc_id;
		if (gca_req->gca_status != E_GC0000_OK)	/* Check for success */
		{
		    if (gca_stat == E_GC0000_OK)
			gca_stat = gca_req->gca_status;
		}
		if (gca_stat != E_GC0000_OK)
		{
		    /* only show errors on the last iteration */
		    if (last)
		    {
			IIcc3_util_status(gca_stat, &gca_req->gca_os_status, stat_buf);
			IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
				 ERx("GCA_REQUEST"), stat_buf, (char *)0, (char *)0);
			/* if we're giving up, release the resources */
		        IIcc3_discon_service(cgc);
		    }
		    if( logfp )
		    {
			SIfprintf(logfp, ERx("%d: Failed GCA_REQUEST\n"),
				mypid);
			SIflush(logfp);
		    }
		    continue;
		}

		/* Used for allocation */
		buf_size = gca_req->gca_size_advise;
		if (buf_size == 0 || buf_size < hdrlen + IICC__MINBUFSIZE)
		    buf_size = hdrlen + IICC__MINBUFSIZE;
		/*
		** Reset protocol level.  In future versions we should
		** detect older protocols to work with.
		*/
		cgc->cgc_version = gca_req->gca_peer_protocol;

		/*
		** Reset protocol level for GCA tracing.
		** GCA tracing is set up before the connection
		** happens therefore, the trace protocol level 
		** must be reset here.
		*/
		if ( cgc->cgc_trace.gca_emit_exit )
		    IIcgct1_set_trace( cgc, cgc->cgc_trace.gca_line_length,
				       cgc->cgc_trace.gca_trace_sem, 
				       cgc->cgc_trace.gca_emit_exit );

		/*
		** Set the ADF datatype support level - used for
		** checking that Decimal, Byte, and Long's are valid types.
		** Decimal,Byte, and LONG are supported as of protocol level 60.
		*/
                if ( cgc->cgc_version >= GCA_PROTOCOL_LEVEL_60 ) {
		    adf_cb->adf_proto_level |= AD_DECIMAL_PROTO;
		    adf_cb->adf_proto_level |= AD_BYTE_PROTO;
		    adf_cb->adf_proto_level |= AD_LARGE_PROTO;
                }

	    	if ( cgc->cgc_version >= GCA_PROTOCOL_LEVEL_65 )
		{
		    adf_cb->adf_proto_level |= AD_I8_PROTO; 
		}

                if ( cgc->cgc_version >= GCA_PROTOCOL_LEVEL_67 )
                {
                    adf_cb->adf_max_decprec = CL_MAX_DECPREC_39;
                }
                else
                {
                    adf_cb->adf_max_decprec = CL_MAX_DECPREC_31;
                }

                if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_68)
                {
                    adf_cb->adf_proto_level |= AD_BOOLEAN_PROTO;
                    adf_cb->adf_max_namelen = DB_GW1_MAXNAME;
                }
                else
                {
                    adf_cb->adf_max_namelen = DB_OLDMAXNAME_32;
                }

	    } /* If we are a child process - saved name */

	    /*
	    ** You'd sort of like to not to allocate this buffer and do
	    ** the GCA_FORMAT on  each retry, but the buffer size might
	    ** change, and there is connection info needed by GCA_FORMAT,
	    ** so you have to do this all over and over again.  That's
	    ** among the reasons this all ought to be in GCF when possible.
	    */
	    if ( cgc->cgc_write_msg.cgc_buffer )
	    {
		MEfree( cgc->cgc_write_msg.cgc_buffer );
	        cgc->cgc_write_msg.cgc_buffer = NULL;
	    }

	    /* Alloc 2 message buffers (buf_size) + local query buffer
	    ** (default size)
	    */
	    alloc_buf = (char *)MEreqmem((u_i4)0,
				 (u_i4)(2*buf_size + IICC__MINBUFSIZE),
				 TRUE, &gca_stat);
	    if (alloc_buf == (char *)0)
	    {
		CVna( (i4)gca_stat, stat_buf );
		IIcc1_util_error(cgc, TRUE, E_LC0014_ASSOC_ALLOC, 1,
			     stat_buf, (char *)0,(char *)0, (char *)0);
		IIcc3_discon_service(cgc);
		if(logfp)
		{
		    SIfprintf(logfp, ERx("%d: MEreqmem failed\n"), mypid);
		    SIflush(logfp);
		}
		return FAIL;
	    } /* If could not allocate message buffers */
 
	    /*
	    ** The "Write" message buffer - allocation starts here (need
	    ** only free from here)
	    */
	    cgc->cgc_write_msg.cgc_b_length = buf_size;
	    cgc->cgc_write_msg.cgc_buffer   = alloc_buf;

	    /* The "Result" message buffer */
	    cgc->cgc_result_msg.cgc_b_length = buf_size;
	    cgc->cgc_result_msg.cgc_buffer   = alloc_buf + buf_size;

	    /*
	    ** The "Query" work buffer.   Note that the recorded size of the
	    ** query buffer is one less than the actual size allocated.  This
	    ** is so that in IIcgc_end_msg we can always add a null
	    ** terminating byte to the query data stored without having to
	    ** check whether it will fit or not.  The null byte later acts as
	    ** a sentinel in the server's parser.
	    */
	    cgc->cgc_qry_buf.cgc_q_max = IICC__MINBUFSIZE - 1;
	    cgc->cgc_qry_buf.cgc_q_buf = alloc_buf + 2 * buf_size;

	    /*
	    ** Prime the "Write" message buffer once - GCA_FORMAT.  If there
	    ** is an error return to caller.
	    */
	    gca_parm = &cgc->cgc_write_msg.cgc_parm;
	    gca_fmt = &gca_parm->gca_fo_parms;
	    gca_fmt->gca_buffer    = cgc->cgc_write_msg.cgc_buffer;   /* In */
	    gca_fmt->gca_b_length  = cgc->cgc_write_msg.cgc_b_length; /* In */
	    gca_fmt->gca_data_area = (char *)0;			      /* Out */
	    gca_fmt->gca_d_length  = 0;				      /* Out */
	    gca_fmt->gca_status    = E_GC0000_OK;		      /* Out */

	    IIGCa_cb_call( &IIgca_cb, GCA_FORMAT, gca_parm, 
			   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

	    if (gca_fmt->gca_status != E_GC0000_OK)	/* Check for success */
	    {
		if (gca_stat == E_GC0000_OK)
		    gca_stat = gca_fmt->gca_status;
	    }
	    if (gca_stat == E_GC0000_OK)
	    {
		cgc->cgc_write_msg.cgc_d_length = gca_fmt->gca_d_length;
		cgc->cgc_write_msg.cgc_d_used   = 0;
		cgc->cgc_write_msg.cgc_data     = gca_fmt->gca_data_area;
	    }
	    else 	/* status not ok */
	    {
		IIcc3_util_status(gca_stat, &gca_fmt->gca_os_status, stat_buf);
		IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
				 ERx("GCA_FORMAT"), stat_buf,
				 (char *)0, (char *)0);
		IIcc3_discon_service(cgc);
		if(logfp)
		{
		    SIfprintf(logfp, ERx("%d: GCA FORMAT failed\n"), mypid);
		    SIflush(logfp);
		}
		return FAIL;
	    }		/* if status ok */

	    /*
	    ** If this is not "child" process then:
	    ** Turn around and read the "acceptance" or "rejection" of the
	    ** server connection request.
	    */
	    if ((cgc->cgc_g_state & IICGC_CHILD_MASK) == 0)
	    {
		/*
		** Was the request accepted?
		**
		** Set "startup so no quit" flag so that if we read a
		** GCA_RELEASE message we do not just exit via the
		** quit handler.  Mask is cleared
		** on next message.
		*/
		cgc->cgc_m_state |= IICGC_START_MASK;
		while (IIcgc_read_results(cgc, TRUE, GCA_ACCEPT))
			;
		if (cgc->cgc_result_msg.cgc_message_type != GCA_ACCEPT)
		{
		    char stat_buf[30];
		    IIcc1_util_error(cgc, FALSE, E_LC0010_ASSOC_REJECT, 1,
				     IIcc2_util_type_name(
					cgc->cgc_result_msg.cgc_message_type,
					stat_buf),
				     (char *)0, (char *)0, (char *)0);
		    if( logfp )
		    {
			SIfprintf(logfp, ERx("%d: Failed read_results\n"),
				mypid);
			SIflush(logfp);
		    }
		    continue;
		}

		connected = TRUE;
	 
	    } /* If was not child */

	} /* loop on vnodes */

    } /* retry */

    /* if all retries failed, exit now */
    if( !connected )
    {
	if(logfp)
	{
	    SIfprintf(logfp,
	      ERx("%d: Connection failed after %d attempt(s), max tries %d.\n"),
			mypid, tryctr, max_tries );				 
	    SIflush(logfp);
	}

	return FAIL;
    }

    /*
    ** If this is not "child" process then:
    ** Send the "modify association parameters" message to
    ** negotiate startup.
    */

    if ((cgc->cgc_g_state & IICGC_CHILD_MASK) == 0)
    {
	/*
	** Now process association parameters - send GCA_MD_ASSOC message with
	** the user parameters (gotten from argument list).
	*/
	IIcgc_init_msg(cgc, GCA_MD_ASSOC, query_lang, 0);
	if (IIcc1_connect_params(cgc, argc, argv, gateway, sendzone,
		adf_cb, dbname) != OK)
	{
	    IIcc3_discon_service(cgc);
	    if(logfp)
	    {
		SIfprintf(logfp,
			ERx("%d: Failed GCA_MD_ASSOC, returning FAIL\n"),
				mypid);
		SIflush(logfp);
	    }
	    return FAIL;
	}

	/*
	** If there is no gateway WITH clause, then complete connection.
	** If completion fails, data structures are deallocated.
	*/
	if (!gateway && IIcgc_tail_connect(cgc) != OK)
	{
	    if(logfp)
	    {
	        SIfprintf(logfp,
		    ERx("%d: Failed tail_connect, returning FAIL.\n"),
				mypid);
		SIflush(logfp);
	    }
	    return FAIL;
	}
    } /* If was not child */

    if (!save_name)
    {
	/*
	** Find out if the association is heterogeneous.  FE's need
	** to know so that encoded information can be transformed.
	*/
	gca_att = &gca_parm->gca_at_parms;
	gca_att->gca_association_id = cgc->cgc_assoc_id;	/* Input */
	gca_att->gca_flags = 0;					/* Output */
	gca_att->gca_status = E_GC0000_OK;			/* Output */

	IIGCa_cb_call( &IIgca_cb, GCA_ATTRIBS, gca_parm, 
		       GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );
	{
	    if (gca_stat == E_GC0000_OK)
		gca_stat = gca_att->gca_status;
	}
	if (gca_stat != E_GC0000_OK)
	{
	    IIcc3_util_status(gca_stat, &gca_att->gca_os_status, stat_buf);
	    IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
			 ERx("GCA_ATTRIBS"), stat_buf, (char *)0, (char *)0);
	    IIcc3_discon_service(cgc);
	    if(logfp)
	    {
		SIfprintf(logfp, ERx("%d: GCA_ATTRIBS failed\n"), mypid);
		SIflush(logfp);
	    }
	    return FAIL;
	}
	if (gca_att->gca_flags & GCA_HET)
	    cgc->cgc_g_state |= IICGC_HET_MASK;    /* Save heterog arch info */
	else
	    cgc->cgc_g_state &= ~IICGC_HET_MASK;
    }

    if(logfp)
    {
    	TMget_stamp(&time_stamp);
	TMstamp_str(&time_stamp, time_str);
	SIfprintf(logfp, ERx("%d: Connected to %s at %s.\n"),
		mypid,nodename,time_str);
	SIflush(logfp) ;
    }

    return OK;
} /* IIcgc_connect */

/*{
**  Name: IIcgc_tail_connect - Complete GCA session connection.
**
**  Description:
**	Complete session connection to the server.  This routine allows 
**	both command-line parameters and general purpose WITH clauses
**	to be sent in a single message by the client.
**
**	Upon entry, the routine assumes that a GCA_MD_ASSOC message has been
**	filled.  The message is flushed to the server and the appropriate
**	response is read.  Since this is startup, whenever an error is
**	detected the input parameter is deallocated, as though we
**	were called from IIcgc_connec.  IIcc3_discon_service is called
**	to clean up GCA resources (GCA_DISASSOC) and to terminate if
**	this is the last session.
**
**  Inputs:
**	cgc		- Pointer to client general communications descriptor.  **			  If this routine fails then this data structure is
**			  deallocated.
**
**  Outputs:
**
**	Returns:
**		STATUS	- Returned during startup phase.
**	Errors:
**		None
**
**  Side Effects:
**	1. Deallocation of communications descriptor.
**	
**  History:
**	08-feb-1988	- Written (ncg)
**	26-may-1989	- Call IIcc3_discon_service to disconnect. (bjb)
**	05-jul-1990 (barbara)
**	    If IIcgc_read_results has read a GCA_RELEASE (which it will have
**	    done if, for instance, the database didn't exist), the FASSOC
**	    flag will be set.  This tells this routine to disconnect
**	    session.
*/

STATUS
IIcgc_tail_connect(cgc)
IICGC_DESC	*cgc;
{

    IIcgc_end_msg(cgc);

    /*
    ** Set "startup so no quit" flag so that if we read a GCA_RELEASE 
    ** message we do not just exit via the quit handler.  Mask is cleared
    ** on next message.
    */
    cgc->cgc_m_state |= IICGC_START_MASK;
    while (IIcgc_read_results(cgc, TRUE, GCA_ACCEPT))
	    ;

    /* If something fatal went wrong on startup then return the status */
    if (cgc->cgc_g_state & IICGC_FASSOC_MASK)
    {
	IIcc3_discon_service(cgc); 
	return FAIL;
    }
    return OK;
} /* IIcgc_tail_connect */

/*{
+*  Name: IIcgc_disconnect - End session with server.
**
**  Description:
**	End the session with the server if not a child.  Call
**	IIcc3_discon_service to clean up any GCA resources that were
**	allocated for this database connection and to terminate if this
**	is the last session.  Deallocate anything that was allocated for
**	the client GCA descriptor.  Note that if the session was already
**	ended then do not do so again.
**
**	This routine may be called in 3 ways:
**	1. The client wants to exit from the server (normal exit).  Here we
**	   set the magic number of the clients cgc descriptor to prevent being
**	   called again.
**	2. The server returned a GCA_RELEASE message (this will probably
**	   have an error attached), and we are called internally.  Here we
**	   set the internal QUIT mask, so that when the client exits (and thinks
**	   they have to disconnect) we know that we have already done this.
**	3. GCA told us that an association with the server failed (this implies
**	   communication problems), and we are called internally.  Same logic as
**	   case 2.
**	Cases 2 and 3 are flagged by the caller passing the "internal quit"
**	flag.
**
**	When disconnecting and we are not a "child" process:
**	1. Send the GCA_RELEASE message to the server if not called internally.
**	2. Call the GCA_DISASSOC service to tell GCA that we've just sent the
**	   GCA_RELEASE message.
**	3. The application may have multiple connections outstanding.
**	   Call the GCA_TERMINATE service to shut down GCA if this is
**	   the final disconnect in an application.
**	If the "child" mask is set (this is not the original process) then
**	just deallocate the GCA descriptor.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	internal_quit	- Implicit exit (from within GCA) or explicit (via
**			  out real user).
**	
**  Outputs:
**	Returns:
**	    None
**		
**	Errors:
**	    None
**
**  Side Effects:
**	1. Because cgc is deallocated, no members of it should be referred
**	   to after this routine.  In fact the caller should assign NULL to
**	   their cgc pointer.  This only happens if internal_quit is false.
-*	
**  History:
**	17-sep-1986	- Written (ncg)
**	23-jul-1987	- Rewritten for GCA (ncg)
**	19-sep-1988	- Trace completion and deallocation (ncg)
**	23-may-1989	- Added support for multiple connections. (bjb)
**	15-jun-1990	(barbara)
**	    Check for association failure before sending RELEASE.
*/

VOID
IIcgc_disconnect(cgc, internal_quit)
IICGC_DESC	*cgc;
bool		internal_quit;
{
    i4		gca_l_e_element; /* No errors to copy */

    if ( cgc )
    {
	/*
	** If we've already quit or we are not original process then
	** don't send the GCA_RELEASE message or the DISASSOC/TERMINATE
	** messages.
	*/
	if ((cgc->cgc_g_state & (IICGC_QUIT_MASK|IICGC_CHILD_MASK)) == 0)
	{
	    /*
	    ** If we're called because the association has gone bad,
	    ** don't send the RELEASE.  Note that we will want to DISASSOC.
	    */
	    if ( ! internal_quit  &&  ! (cgc->cgc_g_state & IICGC_FASSOC_MASK) )
	    {
		gca_l_e_element = 0;
		IIcgc_init_msg(cgc, GCA_RELEASE, DB_NOLANG, 0);
		IIcc3_put_bytes(cgc, TRUE, sizeof(gca_l_e_element), 
				(char *)&gca_l_e_element);
		IIcgc_end_msg(cgc);
	    }

	    IIcc3_discon_service(cgc);
	}

	/* We're done */
	cgc->cgc_g_state |= IICGC_QUIT_MASK;
    }

    return;
} /* IIcgc_disconnect */

/*{
+*  Name: IIcgc_alloc_cgc - Allocated an IICGC_DESC
**
**  Description:
**	Allocate a general communications descriptor and assign its address
*-	to incoming pointer.
**  Inputs:	
**	cgc_handle	- Caller function handler.
**
**  Outputs:
**	cgc_buf		- Pointer to client general communications descriptor
**			  allocated and set by this routine.
**  Returns:
**	STATUS 		- SUCCESS or FAIL.
**  Errors:
**	GE_NO_RESOURCE/E_LC0014_ASSOC_ALLOC.  Error is handled by (*cgc_handle).
**  Side Effects:
**	None.
*/

STATUS
IIcgc_alloc_cgc(cgc_buf, cgc_handle)
IICGC_DESC	**cgc_buf;
# ifdef	CL_HAS_PROTOS					 
i4		(*cgc_handle)(int, IICGC_HARG *);  
# else							 
i4		(*cgc_handle)();
# endif							 
{
    STATUS	gca_stat;
    char	stat_buf[IICGC_ELEN +1];/* Status buffer */
    IICGC_HARG	handle_arg;		/* Handle argument for errors */

    *cgc_buf = (IICGC_DESC *)MEreqmem((u_i4)0, (u_i4)sizeof(IICGC_DESC),
				 TRUE, &gca_stat);
    if (*cgc_buf == (IICGC_DESC *)0)
    {
	CVna( (i4)gca_stat, stat_buf );
	handle_arg.h_errorno = GE_NO_RESOURCE;
	handle_arg.h_local = E_LC0014_ASSOC_ALLOC;
	handle_arg.h_numargs = 1;
	handle_arg.h_data[0] = stat_buf;
	_VOID_ (*cgc_handle)(IICGC_HUSERROR, &handle_arg);
	return FAIL;
    } /* Cgc descriptor not allocated */
    return OK;
}


/*
** IIcgc_free_cgc
**
** Description:
**
** Input:
**
** Output:
**
** Returns:
**
** History:
*/

VOID
IIcgc_free_cgc( IICGC_DESC *cgc )
{
    if ( cgc->cgc_write_msg.cgc_buffer )
	MEfree( (PTR)cgc->cgc_write_msg.cgc_buffer );

    if ( cgc->cgc_savestate )
	MEfree( (PTR)cgc->cgc_savestate );

    if ( cgc->cgc_trace.gca_continued_buf )
	MEfree( (PTR)cgc->cgc_trace.gca_continued_buf );

    MEfree( (PTR)cgc );

    return;
}


/*{
+*  Name: IIcc1_connect_params - Send off any association parameters to server.
**
**  Description:
**	This routine is internal to the IIcgc routines.
**
**	This routine first sets up "global" parameters, such as "version" and
**	"query language".  Then send all the user defined parameters, such
**	as "database name", "-u user name", etc.  All the user defined 
**	parameters are defined in the common header file "gca.h".  That
**	file shows the user string flag, the corresponding flag index and data
**	value.
**
**	All the parameters are sent in a single GCA_MD_ASSOC message.
**	Each association parameter is sent via "user data" which consists
**	of the flag integer index and a GCA data value for the flag value.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	argc		- Number of user arguments.
**	argv		- Array of null terminated, blank trimmed string
**			  user arguments.
**	adf_cb		- ADF control block.  This is checked to see if we
**			  need to send international parameters as well.
**			  This may be set to null.
**	gateway		- Are there any gateway parameters to follow?
**
**  Outputs:
**	dbname		- When finding the database name in the list this
**			  will be filled with it.
**	Returns:
**	    STATUS - Fail if cannot send flag.
**
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	27-jul-1987	- Written (ncg)
**	08-feb-1988	- Added 'gateway' flag to indicate more data (ncg)
**	31-aug-1988	- Added the sending of international parameters (ncg)
**	28-nov-1988	- Added the checking of natural language (ncg)
**	03-feb-1989	- Added the sending of group/application id flags (bjb)
**	28-mar-1989	- Trimmed dbtype as well as node off dbname (bjb)
**	21-apr-1989	- Changed names of grp/applid flags to -y/-z (bjb)
**	21-sep-1989	- Updated for -k flag and II_NUMERIC_LITERAL log (bjb)
**	11-oct-1989	- Changed names of grp/roleid flags to -G/-R (bjb)
**	20-nov-1989	- Allow unlimited number of node names on dbname (bjb)
**	02-jan-1990	- Added support for -Ppassword flag. (bjb)
**	21-sep-1990     (barbara)
**	    Send down timezone information as another session parameter.  For
**	    inter-operability, don't send if dbms is running at a lower
**	    protocol level than 31 or if sendzone is false.
**	03-oct-1990	(barbara)
**	    Backed out time zone fix until dbms is ready.
**	14-nov-1990
**	    DBMS says it's time to go with timezone fix.
**	08-mar-1991	(teresal)
**	    Changed behavior so unrecognized parameters will no longer
**	    generate a LIBQ error but will be sent to the
**	    server as GCA_MISC_PARM.  It is now up to the server to
**	    recognize these parameters or generate an error.
**	15-aug-1991 (kathryn) Bug 39172
**          For GCA_SESSION_PARMS - If character type (DB_CHR_TYPE) has length
**	    of zero, then change type to varchar type (DB_VCH_TYPE). Het Net
**	    issues GCA error message for zero length DB_CHR_TYPE.
**	14-jul-1992 (mgw)
**	    Added New Timezone Table support for 6.5 keyed off of the new
**	    II_TIMEZONE_NAME logical.
**	11-nov-93 (robf)
**          Tell DBMS if we can handle dbprompts.
**	21-apr-1994 (connie) Bug #58188
**	    Removed CVlower around group name(-G) and user(-u) to work with
**	    FIPS iidbdb.
**	14-nov-1995 (nick)
**	    Pass GCA_YEAR_CUTOFF at protocol level 62 onwards ...
**	06-mar-1998 (nanpr01)
**	    GCA_YEAR_CUTOFF is causing certain GCA parameters not passed.
**	    Star Trak #6048953. Bug # 89429.
**	01-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE for Sir 92541.  
**	    Note - don't expect money symbol if DB_NONE_MONY.
**      30-Nov-2005 (hanal04) Bug 100715 INGSRV3376
**          For the purposes of the COPY command (and possibly others)
**          the front end's AD_CB needs to record any changes to the
**          string_truncation option.
**	07-Aug-2007 (gupsh01)
**	    Added support for GCA_DATE_ALIAS.
**	17-Aug-2007 (gupsh01)
**	    GCA_GW_PARM is not being send when date_alias
**	    is ingresdate.
*/

STATUS
IIcc1_connect_params(cgc, argc, argv, gateway, sendzone, adf_cb, dbname)
IICGC_DESC	*cgc;
i4		argc;
char		**argv;
i4		gateway;
i4		sendzone;
ADF_CB		*adf_cb;
char		*dbname;
{
    i4		gca_num_args;	/* Number of arguments to put */
    i4		flag_index;	/* Index of what the flag is */
    DB_DATA_VALUE	index_dbv;
    DB_DATA_VALUE	flag_dbv;	/* The actual flag value */
    i4		flag_intval;	/* Some flags are int values */
    char		*flag_chval;	/*    and some are chars */
    char		*flag;		/* Pointer to flag string */
    bool		unknown_flag;	/* Unknown flag */
    char 		err_buf[MAX_LOC+3];
    char		*cv_ptr;	/* Pointer for CVal */
    char		decimal_char;	/* Decimal character */
    char		*cp;		/* Temp pointer */
    i4			ind;		/* Temp counter */
    bool		flagk_seen = FALSE;	/* -k flag seen */
    bool		declog_seen = FALSE;	/* II_NUMERIC_LITERAL seen */
    char		dl[MAX_LOC +1],*decimal_log;
    i4		tmzone = 0L;	/* 6.4 and before timezone info */
    char		*tm_zone_name;	/* 6.5 and beyond timezone info */
    char		tm_loc[MAX_LOC + 1];  /* to hold tm_zone_name */
    i4		year_cutoff = -1L;    /* 6.5 date_century boundary */
    char		*date_century_boundary;	/* to hold date cutoff */
    char		dst[MAX_LOC + 1];     /* to futz with long flags */
    bool		can_prompt = FALSE;   /* Frontend allows dbms prompts */
    char		date_type_alias[DB_MAXNAME]; /* store ingresdate or ansidate */

    static DB_TEXT_STRING zero_vch = {0,""};

    /*
    ** fail early if II_TIMEZONE_NAME is not set. Should be set at install
    ** time.
    */
    NMgtAt("II_TIMEZONE_NAME", &tm_zone_name);
    if (tm_zone_name == NULL || *tm_zone_name == EOS)
    {
	/* fail - II_TIMEZONE_NAME must be set to the name of your timezone. */
	IIcc1_util_error(cgc, TRUE, E_LC0037_BAD_TIMEZONE_NAME, 0, (char *)0,
			 (char *)0, (char *)0, (char *)0);
	return FAIL;
    }

    STlcopy(tm_zone_name, tm_loc, MAX_LOC);

    dbname[0] = '\0';

    if ((cgc->cgc_version >= GCA_PROTOCOL_LEVEL_67) &&
        (adf_cb->adf_date_type_alias == AD_DATE_TYPE_ALIAS_ANSI))
    {
      STlcopy("ansidate", date_type_alias, 8);
      date_type_alias[8] = EOS;
    }
    else
    {
      STlcopy("ingresdate", date_type_alias, 10);
      date_type_alias[10] = EOS;
    }

    /*
    ** First determine the number of arguments:
    **
    ** 		# of user arguments
    **	 +	2 implied arguments
    **			(  version number
    **			 + query language)
    **   +	11 ADF output format information
    **	[-	1 if the caller gave us the -X flag]
    **  [+	1 if II_NUMERIC_LITERAL log defined  and -k flag not defined ]
    **	[+ 	1 if 'gateway' is set]
    **	[+	1 if ADF natural language is not default (english = 0 or 1)]
    **	[+	1 if ADF date format is not US]
    **	[+	1 if ADF money precision is not 2]
    **	[+	1 if ADF money sign is not leading]
    **	[+	1 if ADF money sign is not '$']
    **	[+	1 if ADF decimal point is not '.']
    **	[+	1 if peer protcl > 3 && sendzone]
    **  [+      1 if peer protcl > 4 && secure  for SL type ]
    ** 
    ** Use gca_num_args for the i4 gca_l_user_data.
    */
    gca_num_args = argc + 2;
    if (cgc->cgc_g_state & IICGC_CHILD_MASK)
	gca_num_args--;

    /* Check out if we have a '-k' argument */
    for (ind = 0; ind < argc; ind++)
    {
	if (argv[ind][0] == '-')
	{
	    if (argv[ind][1] == 'k')
	    {
		flagk_seen = TRUE;
	    }
	}
    } /* for all args */
    if (gateway)
	gca_num_args++;

    /* Should we handle prompts ? */
    if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_60)
    {
	char  *dbprompt=NULL;

	/*
	** Could send, see if overridden
	*/
	NMgtAt(ERx("II_DBPROMPT"), &dbprompt);
	if (dbprompt != (char *)0 && dbprompt[0] != '\0')
	{
	    /*
	    ** If set to NO or OFF then don't handle dbprompts.
	    */
	    if (STbcompare(dbprompt, 0, ERx("off"), 0, TRUE) == 0 ||
	        STbcompare(dbprompt, 0, ERx("no"), 0, TRUE) == 0 )
	    {
			/*
			** Don't handle prompts
			*/
			can_prompt=FALSE;
	    }
	    else
	    {
			can_prompt=TRUE;
	    }
	}
	else  if(SIterminal(stdin))
	{
		/*
		** Its a terminal, so prompt
		*/
		can_prompt=TRUE;
	}
	else
		can_prompt=FALSE;
    }
    else
	can_prompt=FALSE;

    if(can_prompt)
	gca_num_args++;

    if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_62)
    {
        NMgtAt(ERx("II_DATE_CENTURY_BOUNDARY"), &date_century_boundary);
        if (date_century_boundary && *date_century_boundary)
        {
	    i4  date_temp;

	    if (CVan(date_century_boundary, &date_temp) == OK)
	    {
	    	year_cutoff = date_temp;
	    }
        }
    }
    if (year_cutoff != -1)
	gca_num_args++;

    /* Should we send style of literal: floating or decimal */
    if (flagk_seen == FALSE)
    {
	decimal_log = (char *)0;
	NMgtAt(ERx("II_NUMERIC_LITERAL"), &decimal_log);
	if (decimal_log != (char *)0 && decimal_log[0] != '\0' && 
		cgc->cgc_version >= GCA_PROTOCOL_LEVEL_60)
	{
	    if (   STbcompare(decimal_log, 0, ERx("decimal"), 7, TRUE) == 0
		|| STbcompare(decimal_log, 0, ERx("float"), 5, TRUE) == 0)
	    {		
		/* Save decimal literal logical */
		dl[0] = *decimal_log;
		dl[1] = '\0';
		CVlower(&dl[0]);
		decimal_log = dl;
		declog_seen = TRUE;
		gca_num_args++;
	    }
	}
    }

    /* Check ADF formats - if present they are sent after all arguments */
    if (adf_cb != (ADF_CB *)0)
    {
	/* Check natural language (0 - undef, 1 - english) */
	if (adf_cb->adf_slang != 0 && adf_cb->adf_slang != 1)
	    gca_num_args++;
	if (adf_cb->adf_dfmt != DB_US_DFMT)
	    gca_num_args++;
	if (adf_cb->adf_mfmt.db_mny_prec != 2)
	    gca_num_args++;
	if (adf_cb->adf_mfmt.db_mny_lort != DB_LEAD_MONY)
	    gca_num_args++;
	if ((adf_cb->adf_mfmt.db_mny_sym[0] != '$') &&
	    (adf_cb->adf_mfmt.db_mny_sym[0] != EOS))
	    gca_num_args++;
	if (adf_cb->adf_decimal.db_decimal != '.')
	    gca_num_args++;
	/*
        ** Add arguments for the following 11 ADF formating flags
        ** that are sent down for every connection:
        **      ad_c0width
        **      ad_t0width
        **      ad_i1width
        **      ad_i2width
        **      ad_i4width
        **      ad_f4width
        **      ad_f8width
        **      ad_f4prec
        **      ad_f8prec
        **      ad_f4style
        **      ad_f8style
        */
 
        gca_num_args += 11;	
    } /* If ADF check */

    /* timezone is counted if protocol > 3 and user hasn't set "notimezone" */
    if (cgc->cgc_version > GCA_PROTOCOL_LEVEL_3 && sendzone) 
        gca_num_args++;

   /* Same with date type alias */
    if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_67)
        gca_num_args++;

    /* sl type is counted is protocol > 4 and user 

    /* Put out the # of arguments */
    flag_dbv.db_datatype = DB_INT_TYPE;
    flag_dbv.db_prec     = 0;
    flag_dbv.db_length 	 = sizeof(gca_num_args);
    flag_dbv.db_data     = (PTR)&gca_num_args;
    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &flag_dbv);

    /* Fixed DB data value for index (for IIcgc_put_msg) */
    index_dbv.db_datatype = DB_INT_TYPE;
    index_dbv.db_prec     = 0;
    index_dbv.db_length   = sizeof(flag_index);
    index_dbv.db_data     = (PTR)&flag_index;

    /*
    ** Send version number - this version number is here for historical
    ** reasons, and can be either removed, or made to match the version
    ** returned from GCA_REQUEST (stored in cgc_version).
    */
    flag_index = GCA_VERSION;
    flag_intval = GCA_V_60;
    flag_dbv.db_length   = sizeof(flag_intval);
    flag_dbv.db_data     = (PTR)&flag_intval;
    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);

    /* Send query language */
    flag_index = GCA_QLANGUAGE;
    flag_intval = cgc->cgc_query_lang;
    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);

    for (; argc && *argv; argc--, argv++)
    {
	flag = *argv;
	flag_index = 0;			/* Reset all to default values */
	flag_intval = 0;
	flag_chval = (char *)0;
	unknown_flag = FALSE;
	cv_ptr = (char *)0;

	if (flag[0] == '-')
	{
	    switch (flag[1])
	    {
	      case 'A':				/* -Addd */
		flag_index = GCA_APPLICATION;
		cv_ptr = &flag[2];
		CVal(&flag[2], &flag_intval);
		break;

	      case 'k':				/* -k[f|d] */
		flag_index = GCA_DECFLOAT;
		flag_chval = &flag[2];
		if (*flag_chval != 'd' && *flag_chval != 'f')
		    unknown_flag = TRUE;
		break;

	      case 'r':				/* -rccc */
		flag_index = GCA_RES_STRUCT;
		flag_chval = &flag[2];
		CVlower(flag_chval);
		break;

	      case 'n':				/* -nccc */
		unknown_flag = TRUE;
                if ( !STbcompare( &flag[1], 16, 
			          ERx("numeric_overflow"), 16, FALSE ) )
                {
	            /* -numeric_overflow={fail|ignore|warn} */
		    (VOID) STzapblank( &flag[1], dst );
                    if ( !STbcompare( &dst[17], 4, ERx("fail"), 4, TRUE ) 
                      || !STbcompare( &dst[17], 6, ERx("ignore"), 6, TRUE ) 
                      || !STbcompare( &dst[17], 4, ERx("warn"), 4, TRUE ) )
		    {
		        flag_index = GCA_MATHEX;
		        flag_chval =  &dst[17];
		        CVlower(flag_chval);
		        unknown_flag = FALSE;
                    }
                }
                else 
                {
                   /* -menccc */
		   flag_index = GCA_IDX_STRUCT;
		   flag_chval = &flag[2];
		   CVlower(flag_chval);
		   unknown_flag = FALSE;
                }
		break;

	      case 'u':				/* -uccc */
		flag_index = GCA_EUSER;
		flag_chval = &flag[2];
		break;
	    
	      case 'x':				/* -x{f|w} */
		flag_index = GCA_MATHEX;
		flag_chval = &flag[2];
		if (*flag_chval != 'f' && *flag_chval != 'w')
		    unknown_flag = TRUE;
		break;

	      case 's':	   	  /* -string_truncation={fail|ignore} */
		unknown_flag = TRUE;
                if ( !STbcompare( &flag[1], 17, 
			          ERx("string_truncation"), 17, FALSE ) )
                {
	            /* -string_truncation={fail|ignore} */
		    (VOID) STzapblank( &flag[1], dst );
                    if ( !STbcompare( &dst[18], 4, ERx("fail"), 4, TRUE ) 
                      || !STbcompare( &dst[18], 6, ERx("ignore"), 6, TRUE ) ) 
		    {
			if ( !STbcompare( &dst[18], 4, ERx("fail"), 4, TRUE ))
			{
			    adf_cb->adf_strtrunc_opt = ADF_ERR_STRTRUNC;
			}
			else
			{
			    adf_cb->adf_strtrunc_opt = ADF_IGN_STRTRUNC;
			}
		        flag_index = GCA_STRTRUNC;
		        flag_chval =  &dst[18];
		        CVlower(flag_chval);
		        unknown_flag = FALSE;
                    }
                }
		break;

	      case 'l':				/* -l */
		flag_index = GCA_EXCLUSIVE;
		flag_intval = 0;
		break;

	      case 'w':				/* -w */
		flag_index = GCA_WAIT_LOCK;
		flag_intval = GCA_OFF;
		break;

	      case '^':			/* Enables upper-case on U or Y */
		if (flag[2] == 'u' || flag[2] == 'U')
		    flag_index = GCA_XUPSYS;
		else if (flag[2] == 'y' || flag[2] == 'Y')
		    flag_index = GCA_SUPSYS;
		else
		    unknown_flag = TRUE;
		flag_intval = GCA_OFF;
		break;

	      case 'G':				/* -Ggroupid */
		flag_index = GCA_GRPID;
		flag_chval = &flag[2];
		break;

	      case 'P':				/* -Ppassword */
		flag_index = GCA_RUPASS;
		flag_chval = &flag[2];
		break;

	      case 'R':				/* -Rroleid[/rolepass] */
		flag_index = GCA_APLID;
		flag_chval = &flag[2];
		break;

	      case 'U':				/* -U */
		flag_index = GCA_XUPSYS;
		flag_intval = GCA_OFF;
		break;

	      case 'Y':				/* -Y */
		flag_index = GCA_SUPSYS;
		flag_intval = GCA_OFF;
		break;

	      case 'X':			/* Ignore -X flags (used by GCA) */
		continue;

	      default:
		unknown_flag = TRUE;
		break;
	    } /* Switch flag[1] */
	}
	else if (flag[0] == '+')
	{
	    flag_intval = GCA_ON;
	    switch (flag[1])
	    {
	      case 'w':				/* +w */
		flag_index = GCA_WAIT_LOCK;
		break;

	      case '^':			/* Enables upper-case on U or Y */
		if (flag[2] == 'u' || flag[2] == 'U')
		    flag_index = GCA_XUPSYS;
		else if (flag[2] == 'y' || flag[2] == 'Y')
		    flag_index = GCA_SUPSYS;
		else
		    unknown_flag = TRUE;
		break;

	      case 'U':				/* +U */
		flag_index = GCA_XUPSYS;
		break;

	      case 'Y':				/* +Y */
		flag_index = GCA_SUPSYS;
		break;

	      default:
		unknown_flag = TRUE;
		break;
	    } /* Switch flag[1] */
	} 
	else if (!CMwhite(flag) && dbname[0] == '\0')
	{
	    flag_index = GCA_DBNAME;
	    flag_chval = &flag[0];

	    /* Strip out any node names - [node::{node::}]dbname */
	    for (cp = flag_chval; *cp; CMnext(cp))
	    {
		if (*cp == ':' && *(cp+1) == ':')
		{
		    flag_chval = cp+2;
		    cp++;
		}
	    }
	    /* ??? Currently nothing to do with node name? */
	    /* Remove dbtype: [node::]dbname[/dbtype] */
	    if (cp = STindex( flag_chval, ERx("/"), 0 ))
		*cp = '\0';
#ifdef	VMS
	    CVlower(flag_chval);
#endif
	    STlcopy(flag_chval, dbname, MAX_LOC);	/* Save for caller */

	}
	else
	{
	    unknown_flag = TRUE;
	} /* If flag is -, + or starts name */

	/* If there is char data to convert to integer */
	if (cv_ptr && !unknown_flag)
	{
	    if (CVal(cv_ptr, &flag_intval) != OK)
		unknown_flag = TRUE;
	}
	/*
	** Send all flags we don't recognize to the server
	** with a GCA_MISC_PARM index (a catch-all category
	** for unknown flags).  If the server doesn't recognize
	** this flag, it will give the appropriate error
	** message. We used to give an LIBQGCA error
	** here, but this prohibited the server from using/testing
	** a new dbms flag if running with an old FE (i.e., if LIBQGCA
	** didn't know about this flag).
	*/ 
	if (unknown_flag)
	{
	    /* Pass entire flag including any initial '-' and '+' */
	    flag_index = GCA_MISC_PARM;
	    flag_chval = &flag[0];
	}
	/* Check if character or integer type */
	if (flag_chval)
	{
	    flag_dbv.db_length   = STlength(flag_chval);
	    if (flag_dbv.db_length == 0)
	    {
		flag_dbv.db_datatype = DB_VCH_TYPE;
		flag_dbv.db_data     = (PTR)&zero_vch;
		flag_dbv.db_length   = sizeof(zero_vch);
	    }
	    else
	    {
	        flag_dbv.db_datatype = DB_CHR_TYPE;
	        flag_dbv.db_data     = (PTR)flag_chval;
	    }
	}
	else  /* Used flag_intval */
	{
	    flag_dbv.db_datatype = DB_INT_TYPE;
	    flag_dbv.db_length   = sizeof(flag_intval);
	    flag_dbv.db_data     = (PTR)&flag_intval;
	}
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);

    } /* For all arguments */

    /* If -k flag not specified, send II_NUMERIC_LITERAL setting */
    if (flagk_seen == FALSE && declog_seen == TRUE)
    {
	flag_index = GCA_DECFLOAT;
	flag_chval = decimal_log;
	flag_dbv.db_datatype = DB_CHR_TYPE;
	flag_dbv.db_length =   STlength(flag_chval);
	flag_dbv.db_data     = (PTR)flag_chval;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    }

    /* Now put the ADF formats */
    if (adf_cb != (ADF_CB *)0)
    {
	flag_dbv.db_datatype = DB_INT_TYPE;			/* Integers */
	flag_dbv.db_length   = sizeof(flag_intval);
	flag_dbv.db_data     = (PTR)&flag_intval;
	if ((flag_intval = adf_cb->adf_slang) != 0 && flag_intval != 1)
	{
	    flag_index = GCA_NLANGUAGE;
	    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
	}
	if ((flag_intval = adf_cb->adf_dfmt) != DB_US_DFMT)
	{
	    flag_index = GCA_DATE_FORMAT;
	    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
	}
	if ((flag_intval = adf_cb->adf_mfmt.db_mny_prec) != 2)
	{
	    flag_index = GCA_MPREC;
	    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
	}
	if ((flag_intval = adf_cb->adf_mfmt.db_mny_lort) != DB_LEAD_MONY)
	{
	    flag_index = GCA_MLORT;
	    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
	}
	/* Put out all ADF output format widths */
 
        flag_index = GCA_CWIDTH;                /* C or CHAR */
        flag_intval = adf_cb->adf_outarg.ad_c0width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_TWIDTH;                /* TEXT or VARCHAR */
        flag_intval = adf_cb->adf_outarg.ad_t0width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_I1WIDTH;               /* INTEGER1 */
        flag_intval = adf_cb->adf_outarg.ad_i1width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_I2WIDTH;               /* INTEGER2 */
        flag_intval = adf_cb->adf_outarg.ad_i2width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_I4WIDTH;               /* INTEGER4 */
        flag_intval = adf_cb->adf_outarg.ad_i4width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_F4WIDTH;               /* FLOAT4 */
        flag_intval = adf_cb->adf_outarg.ad_f4width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);

	flag_index = GCA_F8WIDTH;               /* FLOAT8 */
        flag_intval = adf_cb->adf_outarg.ad_f8width;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_F4PRECISION;           /* FLOAT4  precision */
        flag_intval = adf_cb->adf_outarg.ad_f4prec;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
 
        flag_index = GCA_F8PRECISION;           /* FLOAT8  precision */
        flag_intval = adf_cb->adf_outarg.ad_f8prec;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);

	flag_dbv.db_datatype = DB_CHR_TYPE;			/* Strings */
	if ((adf_cb->adf_mfmt.db_mny_sym[0] != '$') &&
	    (adf_cb->adf_mfmt.db_mny_sym[0] != EOS))
	{
	    flag_index 	   = GCA_MSIGN;
	    flag_chval 	   = adf_cb->adf_mfmt.db_mny_sym;
	    flag_dbv.db_length = STlength(flag_chval);
	    flag_dbv.db_data   = (PTR)flag_chval;
	    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
	}
	if ((decimal_char = adf_cb->adf_decimal.db_decimal) != '.')
	{
	    flag_index 	   = GCA_DECIMAL;
	    flag_dbv.db_length = 1;
	    flag_dbv.db_data   = (PTR)&decimal_char;
	    IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	    IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
	}
	flag_index = GCA_F4STYLE;
        flag_chval       = &adf_cb->adf_outarg.ad_f4style;
        flag_dbv.db_length   = 1;
        flag_dbv.db_data     = (PTR)flag_chval;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);

	flag_index = GCA_F8STYLE;
        flag_chval       = &adf_cb->adf_outarg.ad_f8style;
        flag_dbv.db_length   = 1;
        flag_dbv.db_data     = (PTR)flag_chval;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    } /* If ADF check */

    /* 
    ** Send time zone in old (6.4 or before) format if protocol allows
    ** it and if user hasn't set NOTIMEZONE.
    */
    if (cgc->cgc_version > GCA_PROTOCOL_LEVEL_3 
	&& cgc->cgc_version < GCA_PROTOCOL_LEVEL_60 && sendzone)
    {
	flag_index = GCA_TIMEZONE;

	/* was TMzone(&tmzone); */
	tmzone = TMsecs();
	if (tmzone > MAXI4)
		tmzone = MAXI4;
	tmzone = (i4) - TMtz_search(adf_cb->adf_tzcb, TM_TIMETYPE_LOCAL,
					 (i4) tmzone) / 60;
	
	flag_dbv.db_datatype = DB_INT_TYPE;
	flag_dbv.db_length   = sizeof(tmzone);
	flag_dbv.db_data     = (PTR)&tmzone;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    }

    /* 
    ** Send time zone in new (6.5 and beyond) format if protocol allows
    ** it and if user hasn't set NOTIMEZONE.
    */
    if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_60 && sendzone)
    {
	/* flag_index = GCA_TIMEZONE;              */
	/* flag_dbv.db_datatype = DB_INT_TYPE;     */
	/* flag_dbv.db_length = sizeof(tmzone);    */
	/* flag_dbv.db_data = (PTR) &tmzone;       */

	flag_index = GCA_TIMEZONE_NAME;
	flag_dbv.db_datatype = DB_CHR_TYPE;
	flag_dbv.db_length = STlength(tm_loc);
	flag_dbv.db_data = (PTR) tm_loc;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    }
    if (year_cutoff != -1)
    {
	flag_index = GCA_YEAR_CUTOFF;
	flag_dbv.db_datatype = DB_INT_TYPE;
	flag_dbv.db_length = sizeof(year_cutoff);
	flag_dbv.db_data = (PTR)&year_cutoff;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    }
    /*
    ** Tell DBMS we can prompt, if applicable
    */
    if (can_prompt)
    {
        flag_intval = GCA_ON;

	flag_index = GCA_CAN_PROMPT;
	flag_dbv.db_datatype = DB_INT_TYPE;
	flag_dbv.db_length = sizeof(flag_intval);
	flag_dbv.db_data = (PTR) &flag_intval;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    }

    if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_67)
    {
        flag_index = GCA_DATE_ALIAS;
        flag_dbv.db_datatype = DB_CHR_TYPE;
        flag_dbv.db_length = STlength(date_type_alias);
        flag_dbv.db_data = (PTR) date_type_alias;
        IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &index_dbv);
        IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &flag_dbv);
    }

    return OK;
} /* IIcc1_connect_params */ 

/*{
+*  Name: IIcgc_save - Save the state for the current process.
**
**  Description:
**	This routine is internal to the IIcgc routines.
**
**	This routine is an interface to GCA_SAVE.  GCA saves most of the
**	stuff, while this routine just saves a couple of local items.  The
**	format of the local saved data must be (** NO SPACES **):
**
**		name=value,{name=value,} [client=(text),]
**	where:
**		value - is an integer or a unquoted string constant
**		text - parenthesized to allow client to include private formats
**
**	This syntax implies that the operators "=,()" are reserved,
**	and even the last object is followed by a comma.  The null terminator
**	is also saved.
**
**	Note that the object ordering is not strict, and the restorer should
**	just pick off the objects it understands.  Client state data may have
**	been setup by the caller into cgc_savestate.
**
**	Currently we save:
**		association = %ld	-- Association id
**		database    = %s	-- Database name
**		version     = %ld	-- Version number
**		bufsize     = %ld	-- Buffer size
**		flags	    = %ld	-- Extra flags
**		client	    = (%s)	-- Saved client state
**		terminating null byte
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	dbname		- Database name.
**	
**  Outputs:
**	save_name	- Buffer to put in saved data name (must be
**			  GCA_SV_NM_LNG +1 in length) and should be passed
**			  via the -X flag.
**	Returns:
**	    STATUS	- Was save successful ?
**
**	Errors:
**	    E_LC0001_SERVICE_FAIL - GCA SAVE service failed.
-*
**  Side Effects:
**	
**  History:
**	27-jul-1987	- Written (ncg)
**	01-sep-1988	- Added inclusion of client state data (ncg) 
**	13-jun-1989	- Pass association id for multiple sessions (ncg)
**	27-sep-1989	- Pass flag info, e.g. HET_MASK. (bjb)
*/

STATUS
IIcgc_save(cgc, dbname, save_name)
IICGC_DESC	*cgc;
char		*dbname;
char		*save_name;
{
    GCA_PARMLIST	*gca_parm;	/* General GCA parameter */
    GCA_SV_PARMS	*gca_sav;	/* GCA_SAVE parameter */
    STATUS		gca_stat;	/* Status for IIGCa_call */
    char		save_dat[200+IICGC_SV_LEN+1];	/* Saved user data */
    char		stat_buf[IICGC_ELEN + 1];	/* Status buffer */
#define CGC_SAVE_MASKS	IICGC_HET_MASK	/* | with all masks we want to save */

    STprintf(save_dat,
	ERx("association=%d,database=%s,version=%d,bufsize=%d,flags=%d,"), 
	    cgc->cgc_assoc_id, dbname, cgc->cgc_version,
	    cgc->cgc_write_msg.cgc_b_length,
	    (cgc->cgc_g_state & CGC_SAVE_MASKS));
    /* Check if saving client state data */
    if (cgc->cgc_savestate != (char *)0)
    {
	STprintf(save_dat + STlength(save_dat),
		ERx("client=(%s),"), cgc->cgc_savestate);
    }

    gca_parm = &cgc->cgc_write_msg.cgc_parm;
    gca_sav = &gca_parm->gca_sv_parms;
    gca_sav->gca_association_id	  = cgc->cgc_assoc_id;		/* Input */
    gca_sav->gca_ptr_user_data    = (PTR)save_dat;		/* Input */
    gca_sav->gca_length_user_data = STlength(save_dat)+1;	/* Input */
    gca_sav->gca_save_name        = save_name;			/* Output */
    gca_sav->gca_status		  = E_GC0000_OK; 		/* Output */

    IIGCa_cb_call( &IIgca_cb, GCA_SAVE, gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (gca_sav->gca_status != E_GC0000_OK)	/* Check for success */
    {
	if (gca_stat == E_GC0000_OK)
	    gca_stat = gca_sav->gca_status;
    }
    if (gca_stat != E_GC0000_OK)
    {
	IIcc3_util_status(gca_stat, &gca_sav->gca_os_status, stat_buf);
	IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
			 ERx("GCA_SAVE"), stat_buf, (char *)0, (char *)0);
	return FAIL;
    } /* Saving was not ok */
    return OK;
} /* IIcgc_save */

/*{
+*  Name: IIcc2_connect_restore - Restore the state from parent process.
**
**  Description:
**	This routine is internal to the IIcgc routines.
**
**	This routine is an interface to GCA_RESTORE.  GCA restores most of the
**	stuff, while this routine just restores a couple of local items that
**	have been saved as user data in the parent.  The format of the local
**	saved data is described in IIcgc_save.  The objects we currently
**	understand are also described in IIcgc_save.
**
**	If an object name is not understood then it is ignored when read (this
**	allows newer versions to spawn older children).  Also note that
**	order is not dependent.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	save_name	- Name saved when using IIcgc_save.
**	header_length	- Default length of buffer header.
**	
**  Outputs:
**	dbname		- Buffer for database name of parent process.
**	Returns:
**	    STATUS
**
**	Errors:
**	    E_LC0001_SERVICE_FAIL - GCA RESTORE service failed.
**	    E_LC0012_ASSOC_RESTORE_DATA - GCA RESTORE data corrupted.
-*
**  Side Effects:
**	1. Sets cgc_version, cgc_assoc_id and cgc_write_msg.cgc_b_length with
**	   saved data.
**	2. If we have saved private client state data then we allocate and save
**	   it into cgc_savestate;
**	
**  History:
**	27-jul-1987	- Written (ncg)
**	01-sep-1988	- Added restoration of client state data (ncg) 
**	27-sep-1989	- Restore flag info, e.g. for HET_MASK (barbara)
**	18-May-01 (gordy)
**	    Bumped GCA protocol level to 64 for national character sets.
*/

STATUS
IIcc2_connect_restore(cgc, save_name, header_length, dbname)
IICGC_DESC	*cgc;
char		*save_name;
i4		header_length;		
char		*dbname;
{
    GCA_PARMLIST	*gca_parm;	/* General GCA parameter */
    GCA_RS_PARMS	*gca_res;	/* GCA_RESTORE parameter */
    STATUS		gca_stat;	/* Status for IIGCa_call */
    char		stat_buf[IICGC_ELEN + 1];	/* Status buffer */
    char		*cp1, *cp2;	/* Parses saved data string */
    /*
    ** Data saved by parent.  The following is a table that looks up the
    ** parent object that was saved and tries to interpret it.  If a new object
    ** is added then you must add the flag definition, the entry into the table,
    ** and the case to assign the saved value into the local descriptor.
    */
#   define PAR_ASSOC_ID	1
#   define PAR_DATABASE	2
#   define PAR_VERSION	3
#   define PAR_BUFSIZE	4
#   define PAR_FLAGS	5
#   define PAR_CLIENT	6
    static struct parent_data {
	char	*par_name;		/* Object name */
	i4	par_type;		/* Object type */
	i4	par_dattype;		/* Object data type */
    } parent_args[] = {
			    {ERx("association"), PAR_ASSOC_ID,	DB_INT_TYPE},
			    {ERx("database"),    PAR_DATABASE,	DB_CHR_TYPE},
			    {ERx("version"),     PAR_VERSION,	DB_INT_TYPE},
			    {ERx("bufsize"),     PAR_BUFSIZE,	DB_INT_TYPE},
			    {ERx("flags"),       PAR_FLAGS,	DB_INT_TYPE},
			    {ERx("client"),      PAR_CLIENT,	DB_CHR_TYPE},
			    {(char *)0,     	 0,		0}
		      };
    struct parent_data	*par;		/* Pointer to look up data */

    gca_parm = &cgc->cgc_write_msg.cgc_parm;
    gca_res = &gca_parm->gca_rs_parms;
    gca_res->gca_save_name        = save_name;		/* Input */
    gca_res->gca_ptr_user_data    = (PTR)0;		/* Output */
    gca_res->gca_length_user_data = 0;			/* Output */
    gca_res->gca_status		  = E_GC0000_OK; 	/* Output */

    IIGCa_cb_call( &IIgca_cb, GCA_RESTORE, gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (gca_res->gca_status != E_GC0000_OK)	/* Check for success */
    {
	if (gca_stat == E_GC0000_OK)
	    gca_stat = gca_res->gca_status;
    }
    if (gca_stat != E_GC0000_OK)
    {
	IIcc3_util_status(gca_stat, &gca_res->gca_os_status, stat_buf);
	IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
			 ERx("GCA_RESTORE"), stat_buf, (char *)0, (char *)0);
	return FAIL;
    } /* Restoring was not ok */

    /* Defaults */
    cgc->cgc_version = GCA_PROTOCOL_LEVEL_64;
    cgc->cgc_write_msg.cgc_b_length = header_length + IICC__MINBUFSIZE;
    cgc->cgc_savestate = (char *)0;

    /* Parse off objects saved by parent */
    for (cp1 = gca_res->gca_ptr_user_data;
	 (cp1 < gca_res->gca_ptr_user_data + gca_res->gca_length_user_data) &&
	 *cp1;
	 cp1++)
    {
	i4		tmpint;

	/*
	** Strip off objects - ignore ones you don't know about. 
	** For each object find its data type and flag, and translate
	** the value into our descriptor:
	**	name=value,{name=value,}
	** where value is:
	**	integer | string | (string)
	**
	** Cp1 points at start of an object name.
	*/
	cp2 = STindex(cp1, ERx("="), 0);
	if (cp2 == (char *)0)
	{
	    IIcc1_util_error(cgc, FALSE, E_LC0012_ASSOC_RESTORE_DATA, 1,
			     ERx("="), (char *)0, (char *)0, (char *)0);
	    return FAIL;
	}
	*cp2 = '\0';			/* Place holder for '=' */
	/* Compare against known objects */
	for (par = &parent_args[0];
	     par->par_name && STcompare(cp1, par->par_name) != 0;
	     par++)
	     ;
	cp1 = ++cp2;		/* Cp1 -> beginning of value */
	if (*cp1 == '(')	/* Cp2 -> find trailing byte after value */
	{
	    *cp1++ = '\0';	/* Strip and skip first paren */
	    if ((cp2 = STrchr(cp1, ')')) != (char *)0)
		*cp2++ = '\0';	/* Strip and skip last paren */
	}
	else
	{
	    cp2 = STindex(cp1, ERx(","), 0);
	}
	if (cp2 == (char *)0)
	{
	    IIcc1_util_error(cgc, FALSE, E_LC0012_ASSOC_RESTORE_DATA, 1,
			     ERx(",)"), (char *)0, (char *)0, (char *)0);
	    return FAIL;
	}
	*cp2 = '\0';

	/* If integer then get value - cp1 -> at beginning of number */
	if (par->par_dattype == DB_INT_TYPE)
	    CVal(cp1, &tmpint);

	/*
	** If value is an integer then in tmpint, else if value is a char
	** cp1 points at null-terminated string value.
	*/
	switch (par->par_type)
	{
	  case PAR_ASSOC_ID:
	    cgc->cgc_assoc_id = tmpint;
	    break;
	  case PAR_VERSION:
	    cgc->cgc_version = (i4)tmpint;
	    break;
	  case PAR_BUFSIZE:
	    cgc->cgc_write_msg.cgc_b_length = tmpint;
	    break;
	  case PAR_DATABASE:
	    STlcopy(cp1, dbname, MAX_LOC);
	    break;
	  case PAR_FLAGS:
	    cgc->cgc_g_state |= tmpint;		/* Restore saved flags */
	    break;
	  case PAR_CLIENT:
	    if ((cgc->cgc_savestate = STalloc(cp1)) == (char *)0)
	    {
		IIcc1_util_error(cgc, FALSE, E_LC0012_ASSOC_RESTORE_DATA, 1,
			     ERx("STalloc"), (char *)0, (char *)0, (char *)0);
		return FAIL;
	    }
	    break;
	  default:
	    break;		/* Ignore */
	} /* Switch on parent saved type */
	cp1 = cp2; 	/* Cp1 -> trailing byte (was ',' and now '\0') */
    } /* For parsing off more objects */
    MEfree((PTR)gca_res->gca_ptr_user_data);		/* Free user data */
    return OK;
} /* IIcc2_connect_restore */


/*{
+*  Name: IIcc3_discon_service - Invoke disconnection services
**
**  Description:
**	This routine is internal to the IIcgc routines.
**
**	This routine invokes the GCA_DISASSOC service to clean up
**	GCA resources allocated for an inidivudal database connection.
**	If this is the last database connection, the GCA_TERMINATE
**	service is called to end the application's connection to GCA.
**	The routine does not end the actual database connection (does not
**	send a GCA_RELEASE message).
**
**	This routine will be called in the following situations:
**	1.  The application requested to disconnect from the
**	    database.  In this case, the GCA_RELEASE message
**	    has already been sent unless the application is a child
**	    process which is sharing the parent's database connection.
**	    In the latter case the database connection will remain
**	    open, but the GCA resources between the child process
**	    and GCA will be cleaned up (DISASSOC).  Termination of GCA
**	    will only be done if this is the last connection (in the parent
**	    or the child).
**	2.  An existing association with the database failed.  
**	    Just clean up GCA resources (DISASSOC) and terminate if
**	    this is the last connection.
**	3.  A requested database association failed to complete.
**	    In this situation a RELEASE will not have been sent (no
**	    need since the connection hasn't been made).  Again,
**	    GCA resources need to be cleaned up (DISASSOC) and the
**	    GCA connection terminated if this is the last session.
**	    (Last connection here actually means the very first
**	    requested connection.)
**
** Inputs:
**	cgc	- Client general communications descriptor.
**
** Outputs:
**	Returns:
**	    None.
**
**	Errors:
**	    None.
**
** Side Effects:
**	None.
-*
** History:
**	26-may-1989	- Written (bjb)
*/

VOID
IIcc3_discon_service(cgc)
IICGC_DESC	*cgc;
{

    GCA_PARMLIST	*gca_parm;	/* General GCA parameter */
    STATUS		gca_stat;	/* Status for IIGCa_call - unused */


    /*
    ** Send the disassociation to GCA. 
    */
    gca_parm = &cgc->cgc_write_msg.cgc_parm;
    gca_parm->gca_da_parms.gca_association_id = cgc->cgc_assoc_id; /* Input  */
    gca_parm->gca_da_parms.gca_status = E_GC0000_OK;		   /* Output */

    IIGCa_cb_call( &IIgca_cb, GCA_DISASSOC, gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat);

}
