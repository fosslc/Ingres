/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cv.h>
#include    <qu.h>
#include    <si.h>
#include    <lo.h>
#include    <nm.h>
#include    <st.h>
#include    <me.h>
#include    <mu.h>
#include    <tm.h>
#include    <gc.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcu.h>

/**
**
**  Name: gcnsoper.c
**
**  Description:
**      Contains the following functions:
**
**          gcn_oper_ns - Execute an user specified operation on Name
**			    Database.
**
**  History:    $Log-for RCS$
**	    
**      08-Sep-87   (lin)
**          Initial creation.
**	09-Nov-88   (jorge)
**	    Updated to perform correct check of priviledged operations.
**	    Also updated the parameter interface to gcn_usr_validate.
**	12-Dec-88   (jorge)
**	    Changed calls to gcn_cmp_str to STbcompare. removed gcn_cmp_str.
**	    fixed gcn_do_op to enforce uniqueness of entries.
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.  Message formats between
**	    IIGCN and its clients remain the same, but the messages are
**	    constructed and extracted via properly aligning code.
**	    Variable length structures eliminated.  Strings are now
**	    null-terminated (they had to before, but no one admitted so).
**	    IIGCN database files no longer shadow the in-core data
**	    structures.  After the structures change, IIGCN writes
**	    them out whole again.
**	23-Apr-89 (jorge)
**	    Made gcn_uid parameter passed down from the 
**	    original GCA_LISTEN call. This guaranties that the UID/PASSWD
**	    has been validated. Note that the info is redundantly provided
**	    in the gcn request message, but it may not be correct there.
**	23-Apr-89 (jorge)
**	   Added checking of SAR ans RDB servers in the case of GCN_RET message.
**	26-Apr-89 (seiwald)
**	   Fixed gcn_tup_compare() to handle wild card component of IILOGIN and
**	   IINODE tuples.
**	22-May-89 (seiwald)
**	   Use MEreqmem().
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	14-jun-89 (seiwald)
**	    For adding names, incoming message format is {header}
**	    follow by a number of {class,uid,obj,val} tuples.  They were
**	    being treated as {header,class} followed by any number of
**	    {uid,obj,val} tuples, skewing the second tuple and beyond.
**	    gcn_oper_ns and gcn_do_op now cooperate to treat server class
**	    as part of the tuple, not header.
**	17-Jul-89 (seiwald)
**	    Simplified interface to gcn_error().
**	25-Nov-89 (seiwald)
**	    Reworked for async GCN.
**	17-Jan-90 (seiwald)
**	    Avoid using memory after it's freed.
**	03-Feb-90 (seiwald)
**	    For object (vnode) uniqueness when adding to IILOGIN table.
**	06-Feb-90 (seiwald)
**	    Do server cleanup when adding (or deleting) a server.
**	    Call gcn_cleanup with the name of the queue to clean.
**	19-Nov-90 (seiwald)
**	    Allow wildcards for obj and val when deleting entries.
**	4-Dec-90 (seiwald)
**	    Support for GCN_SOL_FLAG: the uid for transient entries
**	    (server registries) nows contains the gcn_flag word from
**	    GCN_D_OPER structure of the GCN_ADD operation.  It is 
**	    formatted as a hex string and used by GCN_RESOLVE.  (The
**	    uid was previously '*' - as there are no private server
**	    registries.)
**	02-Jan-91 (brucek)
**	    Support for GCN_MRG_FLAG: when this flag is set on an ADD
**	    operation, we do not delete previously existing entries 
**	    with the same value.
**	07-Jan-91 (brucek)
**	    Wildcard character "*" can now be escaped by using a backslash
**	    ("\*") so that user can delete a server entry supporting all
**	    objects without deleting all entries for that server.
**	24-Apr-91 (brucek)
**	    Added include for tm.h
**	07-May-91 (brucek)
**	    Modified default behavior for adding node entries: now old
**	    entries for same vnode are deleted unless merge flag is set.
**	28-Jun-91 (seiwald)
**	    Internal name queue access restructured.  See gcnsfile.c
**	    for new description.
**	23-Mar-92 (seiwald)
**	    Defined GCN_GLOBAL_USER as "*" to help distinguish the owner
**	    of global records from other uses of "*".
**	23-Mar-92 (seiwald)
**	    Strip passwords from the login entries on RET.
**	29-Sep-92 (brucek)
**	    New argument to gcn_nq_add.
**	29-Oct-92 (brucek)
**	    Converted to call gca_chk_priv instead of gcn_usr_validate.
**	04-Nov-92 (brucek)
**	    Use #define'd privilege names instead of char strings.
**	05-Nov-92 (brucek)
**	    Support for dynamic server classes.
**	12-Jan-93 (brucek)
**	    Allow trusted client to bypass privilege checks.
**	18-Feb-93 (edg)
**	    gcn_cleanup() no longer used.
**	24-mar-93 (smc)
**	    Added forward declarations of gcn_do_op() and gcn_result().
**	06-Apr-93 (brucek)
**	    Fixed privilege check on global operations so that trusted
**	    user can delete as well as add.
**	13-Apr-93 (seiwald)
**	    Flagbuf should be 9 bytes long, to handle the '\0' at the end
**	    of a converted hex string.
**	18-May-93 (brucek)
**	    Allow privileged user to add/delete private node/login
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	    entries on behalf of another user.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	23-March-95 (reijo01)
**	    Correct bug 67521. netutil was not displaying all the vnodes that
**	    are present in the names database. The change makes the data
**	    used length equal to the data formatted length in the gcf parms 
** 	    if the buffer has been filled with tuples and there is still
**	    tuples to be sent back. 
**	 4-Dec-95 (gordy)
**	    Moved global references to gcnint.h.  Added prototypes.
**	11-Feb-97 (gordy)
**	    Access to TICKET info requires NET_ADMIN privilege.
**	    Use case insensitive compare when determining access type.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	20-Mar-97 (gordy)
**	    Fixes to support Name Server access across the net.
**	27-May-97 (thoda04)
**	    Included gcaint.h and cv.h for the function prototypes.
**	29-May-97 (gordy)
**	    Added GCS server key parameter.  Maintain registered server
**	    info (including server key) for ADD/DEL of transient queue.
**	19-Feb-98 (gordy)
**	    Use gcn_nq_scan() for better performance when retrieving
**	    name queue tuples.
**	31-Mar-98 (gordy)
**	    Allow anyone to query the registered server queue since we
**	    no longer return the server keys.  Only trusted users may
**	    update the registered server queue.
**	 4-Jun-98 (gordy)
**	    Added gcn_oper_check() to bedcheck servers for request.
**	10-Jun-98 (gordy)
**	    Added better message for gcn_nq_create() failure.
**	 7-Jul-98 (gordy)
**	    Produce 0 result rows rather than error for unknown server
**	    classes now that all server classes are dynamically created.
**	 7-Oct-98 (gordy)
**	    If SERVERS entry is being added manually, make sure the
**	    server key (uid) is cleared.  Only add SERVERS entry for
**	    registering servers if server key is provided.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Sep-02 (hanal04) Bug 110641 INGSRV2436
**           Make server class case insensitive again.
**	15-Jul-04 (gordy)
**	    Enhanced encryption of stored passwords.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS queue.  SHOW SERVERS command now handled
**	    by scanning each transient queue.  Individual NS_OPER
**	    operations broken out into their own functions.
**	 3-Aug-09 (gordy)
**	    Remove string length restrictions.
**	27-Aug-10 (gordy)
**	    Use configured password encoding version for login entries.
*/

/*
** Operation flags.
*/

#define	GCN_OPFLAG_PUB		0x0001		/* Global */
#define	GCN_OPFLAG_UID		0x0002		/* Set UID */
#define	GCN_OPFLAG_MRG		0x0004		/* Merge */
#define	GCN_OPFLAG_TRUST	0x0008		/* Trusted */

#define	GCN_OPFLAG_NODE		0x0010		/* VNODE connection */
#define	GCN_OPFLAG_LOGIN	0x0020		/* VNODE login */
#define	GCN_OPFLAG_ATTR		0x0040		/* VNODE attribute */
#define	GCN_OPFLAG_TICK		0x0080		/* IP local & remote tickets */

#define	GCN_OPFLAG_SRV		0x0100		/* Specific server class */
#define	GCN_OPFLAG_SRVS		0x0200		/* All registered servers */

#define	GCN_OPFLAG_VNODE	(GCN_OPFLAG_NODE | \
				 GCN_OPFLAG_LOGIN | GCN_OPFLAG_ATTR)


/*
**  Forward and/or External function references.
*/

static STATUS	gcn_op_get( GCN_MBUF *, char *, GCN_TUP *, i4 );
static STATUS	gcn_op_servers( GCN_MBUF *, GCN_TUP *, i4 );
static STATUS	gcn_op_add( char *, GCN_MBUF *, 
			    char *, char *, GCN_TUP *, i4, i4, i4 );
static STATUS	gcn_op_del( GCN_MBUF *, char *, char *, GCN_TUP *, i4 );
static STATUS	gcn_result( GCN_MBUF *mbout, i4 opcode, i4 count );
static STATUS	transform_login( char *ibuff, char *obuff );


/*
** Name: gcn_oper_check
**
** Description:
**	Checks a GCA_NS_OPER request to see if bedchecking is
**	required.  Servers which need bedchecking are placed
**	on the bedcheck queue and the number of servers on the
**	queue is returned.
**
** Input:
**	mbin		Message buffer.
**	bdchk_q		Bedcheck queue.
**
** Output:
**	None.
**
** Returns:
**	i4		Number of entries added to bedcheck queue.
**
** History:
**	 4-Jun-98 (gordy)
**	    Created.
*/

i4
gcn_oper_check( GCN_MBUF *mbin, QUEUE *bchk_q )
{
    GCN_TUP	tup;
    char	*ptr;
    char	*msg_in = mbin->data;
    i4		ival;
    i4		count = 0;

    msg_in += gcu_get_int( msg_in, &ival );		/* Flags */
    msg_in += gcu_get_int( msg_in, &ival );		/* OP code */

    /*
    ** Bedchecking is only needed for data retrieval.
    */
    if ( ival == GCN_RET )
    {
	msg_in += gcu_get_str( msg_in, &ptr );		/* Installation */
	msg_in += gcu_get_int( msg_in, &ival );		/* Tuple count */

	while( ival-- > 0 )
	{
	    msg_in += gcu_get_str( msg_in, &ptr );	/* Server class */
	    msg_in += gcn_get_tup( msg_in, &tup );	/* Tup mask */

	    /*
	    ** Add servers of requested class to queue.
	    */
	    count += gcn_bchk_class( ptr, bchk_q );
	}
    }

    return( count );
}


/*{
** Name: gcn_oper_ns - This routine is called by the Name Server to process the
**		request of GCA_NS_OPER message type.
**
** Description:
**	This routine takes the message from the caller, picks up the arguments
**	and calls the correspondent routines to process each operation.
**	The requested service class is picked up from the input message and
**	used to identify the array element of the control information. The
**	control information contains the service class, the queue header of
**	cached entries, and the flag indicating whether the name entries are
**	cached. If the name entries are cached, the queue is searched; 
**	otherwise, the file is searched. 
**	The current valid operations are ADD, DELETE, RETRIEVE, and SHUTDOWN.
**	For ADD operation, the existing name entries are searched. The entry
**	is only inserted when it's not existing. The name entry is inserted
**	into the file, and into the queue if chached. The number of inserted
**	entries is returned to the user.
**	For DELETE operation, the entries are deleted from the file, and from
**	the queue if cached.
**	For RETRIEVE, the specified field is searched for matching. The provite
**	entries are returned if found, otherwise the global entries are
**	returned if found. 
**	The SHUTDOWN operation closes all name files and shut down the Name
**	Server.
**
** Inputs:
**	mbin	Input message buffer.
**	mbout	Output message buffer queue.
**	user	Client user ID.
**	trusted	Client is trusted user.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** Side Effects:
**	    The Name Database may be updated according to the operation code
**	    specified in the massage buffer.
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Revamped.
**	19-Apr-89 (jorge)
**	    Made class point to a local copy of the string if gcn_getsym()
**	    is used.
**	23-Apr-89 (jorge)
**	    Made gcn_uid parameter passed down from the 
**	    original GCA_LISTEN call.
**	14-jun-89 (seiwald)
**	    Only peek at the gcn_type in the incoming message, so that
**	    gcn_do_op can see it.
**	17-may-90 (ham)
**	    Call GCnsid to clear the name server listen logical.
**	18-jun-90 (seiwald)
**	    Move that GCnsid call into the shutdown code, not the user
**	    validation code.
**	02-Jan-91 (brucek)
**	    Support for GCN_MRG_FLAG (see comments at top)
**	01-Oct-92 (brucek)
**	    Remove merge flag before registering new server.
**	11-Feb-97 (gordy)
**	    Access to TICKET info requires NET_ADMIN privilege.
**	    Use case insensitive compare when determining access type.
**	29-May-97 (gordy)
**	    Added GCS server key parameter.  Maintain registered server
**	    info (including server key) for ADD/DEL of transient queue.
**	15-Aug-97 (rajus01)
**	    Added GCN_ATTR name queue to specify security mechanisms.
**	31-Mar-98 (gordy)
**	    Allow anyone to query the registered server queue since we
**	    no longer return the server keys.  Only trusted users may
**	    update the registered server queue.
**	 7-Jul-98 (gordy)
**	    Produce 0 result rows rather than error for unknown server
**	    classes now that all server classes are dynamically created.
**	 7-Oct-98 (gordy)
**	    If SERVERS entry is being added manually, make sure the
**	    server key (uid) is cleared.  Only add SERVERS entry for
**	    registering servers if server key is provided.
**      10-Sep-02 (hanal04) Bug 110641 INGSRV2436
**          Convert gcn_type to upper case. Server classes should
**          be case insensitive as they were in releases prior to
**          AI 2.6.
**	 1-Mar-06 (gordy)
**	    Server key storage moved so server key parameter dropped.
**	    Moved common functionality to this routine and broke-up
**	    monolithic processing routine into separate routines for
**	    each action.  Implemented 'show servers' action since
**	    the SERVERS queue which used to provide that function
**	    is being dropped.
*/

STATUS
gcn_oper_ns
( 
    GCN_MBUF	*mbin, 
    GCN_MBUF	*mbout, 
    char	*user, 
    bool	trusted 
)
{
    char	*msg_in = mbin->data;
    char	*tup_list;
    char	*gcn_install;
    char	*gcn_type;
    GCN_TUP	tup;
    i4		opcode; 
    i4		opflags = 0;
    i4		flags;
    i4		num_tuple;
    STATUS	status = OK;
    CL_SYS_ERR	sys_err;

    /*
    ** Retrieve GCN_NS_OPER message contents.
    */
    msg_in += gcu_get_int( msg_in, &flags );

    if ( trusted )		opflags |= GCN_OPFLAG_TRUST;	/* Trusted */
    if ( flags & GCN_PUB_FLAG )	opflags |= GCN_OPFLAG_PUB;	/* Global */
    if ( flags & GCN_UID_FLAG )	opflags |= GCN_OPFLAG_UID;	/* Set UID */
    if ( flags & GCN_MRG_FLAG )	opflags |= GCN_OPFLAG_MRG;	/* Merge */
    flags &= ~(GCN_MRG_FLAG | GCN_UID_FLAG);	/* Remove internal flags */

    msg_in += gcu_get_int( msg_in, &opcode );
    msg_in += gcu_get_str( msg_in, &gcn_install );
    msg_in += gcu_get_int( msg_in, &num_tuple );

    if ( num_tuple > 0 )
    {
	tup_list = msg_in;		/* Tuple list needed for GCN_ADD */
	msg_in += gcu_get_str( msg_in, &gcn_type );
	msg_in += gcn_get_tup( msg_in, &tup );

	/* Check for default server class */
	if ( ! gcn_type[0] )  gcn_type = IIGCn_static.svr_type;
	CVupper( gcn_type );

	if ( ! STcompare( gcn_type, GCN_NODE ) )
	    opflags |= GCN_OPFLAG_NODE;		/* VNODE connection info */
	else  if ( ! STcompare( gcn_type, GCN_LOGIN ) )
	    opflags |= GCN_OPFLAG_LOGIN;	/* VNODE login info */
	else  if ( ! STcompare( gcn_type, GCN_ATTR ) )
	    opflags |= GCN_OPFLAG_ATTR;		/* VNODE attribute info */
	else  if ( ! STcompare( gcn_type, GCN_LTICKET )  ||
		   ! STcompare( gcn_type, GCN_RTICKET ) )
	    opflags |= GCN_OPFLAG_TICK;		/* IP local/remote tickets */
	else  if ( ! STcompare( gcn_type, "SERVERS" ) )
	    opflags |= GCN_OPFLAG_SRVS;		/* All registered servers */
	else
	    opflags |= GCN_OPFLAG_SRV;		/* Anything else is a server */

	/* 
	** Set user ID.  IP tickets and public entries are user
	** independent.  In general, the client user ID is used
	** (overwrites requested user ID) unless the Set UID flag 
	** has been specified (and client has NET_ADMIN privilege).
	*/
	if ( opflags & GCN_OPFLAG_TICK )
	    tup.uid = "";
	else  if ( opflags & GCN_OPFLAG_PUB )
	    tup.uid = GCN_GLOBAL_USER;
	else  if ( ! (opflags & GCN_OPFLAG_UID) )
	    tup.uid = user;
    }

    /*
    ** Check that user has required privileges for operation requested.
    ** (These checks are bypassed if client is trusted.)
    */
    if ( ! (opflags & GCN_OPFLAG_TRUST) )
    {
	/*
	** NET_ADMIN is required to access IP local/remote tickets.
	*/
	if ( opflags & GCN_OPFLAG_TICK )
	    status = gca_chk_priv( user, GCA_PRIV_NET_ADMIN );

	/*
	** NET_ADMIN is required to access another users's VNODE entry.
	*/
	if ( opflags & GCN_OPFLAG_VNODE  &&  opflags & GCN_OPFLAG_UID )
	    status = gca_chk_priv( user, GCA_PRIV_NET_ADMIN );

	/*
	** SERVER_CONTROL is required to shutdown Name Server.
	*/
	if ( opcode == GCN_SHUTDOWN )
	     status = gca_chk_priv( user, GCA_PRIV_SERVER_CONTROL );

	if ( status != OK )
	{
	    gcn_error( status, NULL, user, mbout );
	    return( status );
	}
    }
	
    switch( opcode )
    {
	case GCN_DEL :
	    if ( num_tuple != 1  ||  opflags & GCN_OPFLAG_SRVS )
		status = gcn_result( mbout, opcode, 0 ); /* Nothing to do... */
	    else
		status = gcn_op_del( mbout, user, gcn_type, &tup, opflags );
	    break;

	case GCN_ADD :
	    if ( opflags & GCN_OPFLAG_SRVS )
		status = gcn_result( mbout, opcode, 0 ); /* Nothing to do... */
	    else
		status = gcn_op_add( tup_list, mbout, user, gcn_type, 
				     &tup, num_tuple, opflags, flags );
	    break;

	case GCN_RET :
	    if ( num_tuple != 1 )
		status = gcn_result( mbout, opcode, 0 ); /* Nothing to do... */
	    else  if ( opflags & GCN_OPFLAG_SRVS )
	    	status = gcn_op_servers( mbout, &tup, opflags );
	    else
		status = gcn_op_get( mbout, gcn_type, &tup, opflags );
	    break;

	case GCN_SHUTDOWN:
	    /*
	    **  Deassign the nameserver listen logical
	    */
	    GCnsid( GC_CLEAR_NSID, NULL, 0, &sys_err );
	    gcn_result( mbout, GCN_SHUTDOWN, 0 );
	    status = E_GC0152_GCN_SHUTDOWN;
	    break;
    }

    return( status );
}


/*
** Name: gcn_op_get
**	Searches name queue identified by gcn_type and returns
**	entries matching masktup as GCN_RET messages in buffers
**	added to callers message buffer queue.
**
** Description:
**
** Inputs:
**	mbout		GCN_MBUF queue.
**	gcn_type	GCN Class (queue name).
**	masktup		Deletion tuple mask.
**	opflags		Operation flags.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Revamped.
**	23-Mar-92 (seiwald)
**	    Strip passwords from the login entries on RET.
**	23-March-95 (reijo01)
**	    Correct bug 67521. netutil was not displaying all the vnodes that
**	    are present in the names database. The change makes the data
**	    used length equal to the data formatted length in the gcf parms 
** 	    if the buffer has been filled with tuples and there is still
**	    tuples to be sent back. 
**	20-Mar-97 (gordy)
**	    Send the correct message length.  Garbage in unsed section
**	    was messing up GCC.
**	19-Feb-98 (gordy)
**	    Use gcn_nq_scan() for better performance when retrieving
**	    name queue tuples.
**	31-Mar-98 (gordy)
**	    Allow anyone to query the registered server queue since we
**	    no longer return the server keys.
**	 7-Jul-98 (gordy)
**	    Produce 0 result rows rather than error for unknown server
**	    classes now that all server classes are dynamically created.
**	 1-Mar-06 (gordy)
**	    Separated into routines for specific operations.
**	 3-Aug-09 (gordy)
**	    Declare default sized temp buffer.  Use dynamic storage
**	    if actual length exceeds default size.
*/

static STATUS
gcn_op_get
( 
    GCN_MBUF	*mbout, 
    char	*gcn_type,
    GCN_TUP	*masktup,
    i4		opflags
)
{
    GCN_QUE_NAME	*nq;
    GCN_MBUF		*mbuf;
    char		*msg_out = NULL;
    char		*msg_post = NULL;
    char		*row_ptr = NULL;
    i4			row_count = 0;
    STATUS		status = OK;

    /*
    ** Find desired class
    */
    if ( ! (nq = gcn_nq_find( gcn_type )) )
	if ( opflags & GCN_OPFLAG_SRV )
	    return( gcn_result( mbout, GCN_RET, 0 ) );	/* No servers */
	else
	{
	    /* Can't find class.  */
	    gcn_error( E_GC0100_GCN_SVRCLASS_NOTFOUND, NULL, gcn_type, mbout );
	    return( E_GC0100_GCN_SVRCLASS_NOTFOUND );
	}

    /*
    ** User ID is not applicable for registered servers.
    */
    if ( nq->gcn_transient )  masktup->uid = "";

    if ( gcn_nq_lock( nq, FALSE ) == OK )
    {
	GCN_TUP	*tupvec[ GCN_SVR_MAX ];
	PTR	scan_cb = NULL;
	i4	i, tupcount;

	/* 
	** Call gcn_nq_scan in a loop to get a batch at a time. 
	*/
	do
	{
	    /* Load up the next entries */
	    tupcount = gcn_nq_scan( nq, masktup, &scan_cb, 
				    GCN_SVR_MAX, tupvec, NULL );

	    /* Loop through server entries */
	    for( i = 0; i < tupcount; i++ )
	    {
		GCN_TUP	*tup = tupvec[ i ];
		GCN_TUP	newtup;
		char 	buf[ 1024 ];
		char	*p = buf;
		char	tmpbuf[ 128 ];
		char	*tmp = tmpbuf;

		p += gcu_put_str( p, nq->gcn_type );
		newtup.uid = tup->uid;
		newtup.obj = tup->obj;
		newtup.val = tup->val;

		/*
		** VNODE login passwords are not exposed.
		*/
		if ( opflags & GCN_OPFLAG_LOGIN )
		{
		    char	*pv[2];
		    i4		len = STlength( tup->val ) + 1;

		    if ( len > sizeof( tmpbuf )  &&
		    	 ! (tmp = (char *)MEreqmem(0, len, FALSE, NULL)) )
		    {
			gcn_nq_unlock( nq );
			return( E_GC0121_GCN_NOMEM );
		    }

		    (void)gcu_words( tup->val, tmp, pv, ',', 2 );
		    newtup.val = pv[0];
		}

		p += gcn_put_tup( p, &newtup );
		if ( tmp != tmpbuf )  MEfree( (PTR)tmp );

		if ( msg_out  &&  (p - buf) + msg_out > msg_post )
		{
		    /*
		    ** Update the row count for this message.
		    ** We send individual messages rather than
		    ** continued messages hoping (in vain) that
		    ** the client won't need to piece message
		    ** segments back together.
		    */
		    gcu_put_int( row_ptr, row_count );
		    mbuf->used = msg_out - mbuf->data;
		    row_count = 0;
		    msg_out = NULL;
		}

		if ( ! msg_out )
		{
		    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
		    if ( status != OK )
		    {
			gcn_nq_unlock( nq );
			return( status );
		    }

		    mbuf->type = GCN_RESULT;
		    msg_out = mbuf->data;
		    msg_post = mbuf->data + mbuf->len;

		    msg_out += gcu_put_int( msg_out, GCN_RET );
		    row_ptr = msg_out;
		    msg_out += gcu_put_int( msg_out, 0 );
		}
			    
		MEcopy( buf, p - buf, msg_out );
		msg_out += p - buf;
		row_count++;
	    }
	} while( tupcount == GCN_SVR_MAX );

	if ( ! msg_out )
	    status = gcn_result( mbout, GCN_RET, 0 );
	else
	{
	    (void)gcu_put_int( row_ptr, row_count );
	    mbuf->used = msg_out - mbuf->data;
	} 

	gcn_nq_unlock( nq );
    }
    else
    {
	status = E_GC0134_GCN_INT_NQ_ERROR;
	gcn_error( status, NULL, NULL, mbout );
    }

    return( status );
}


/*
** Name: gcn_op_servers
**
** Description:
**	Implement the 'show SERVERS' functionality which was
**	originally implemented using the SERVERS name queu.
**
**	The list of name queues is scanned for registered
**	servers (transient).  Entries matching masktup in 
**	the queues found are formatted as GCN_RET tuples.  
**	Result messages and buffers are added to the callers 
**	message buffer queue.
**
** Inputs:
**	mbout		GCN_MBUF queue.
**	masktup		Search tuple mask.
**	opflags		Operation flags.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	 1-Mar-06 (gordy)
**	    Created.
**	22-May-06 (gordy)
**	    Don't return info from the registry queue (NMSVR) since
**	    these aren't installation registered servers.  Also, the
**	    output is now identical to the other show commands, so
**	    the tuple object no longer requires special processing.
*/

static STATUS
gcn_op_servers
( 
    GCN_MBUF	*mbout, 
    GCN_TUP	*masktup,
    i4		opflags
)
{
    QUEUE		*q;
    GCN_MBUF		*mbuf;
    char		*msg_out = NULL;
    char		*msg_post = NULL;
    char		*row_ptr = NULL;
    i4			row_count = 0;
    STATUS		status = OK;

    /*
    ** User ID is not applicable for registered servers.
    */
    masktup->uid = "";

    for(
	 q = IIGCn_static.name_queues.q_next;
	 q != &IIGCn_static.name_queues;
	 q = q->q_next
       )
    {
	GCN_QUE_NAME	*nq = (GCN_QUE_NAME *)q;
	GCN_TUP		*tupvec[ GCN_SVR_MAX ];
	PTR		scan_cb = NULL;
	i4		i, tupcount;

	/*
	** Skip non-server and registry queues.
	** Also, ignore errors on individual queues.
	*/
        if ( ! nq->gcn_transient )  continue;
	if ( ! STcompare( GCN_NS_REG, nq->gcn_type ) )  continue;
	if ( gcn_nq_lock( nq, FALSE ) != OK )  continue;

	/* 
	** Call gcn_nq_scan in a loop to get a batch at a time. 
	*/
	do
	{
	    /* Load up the next entries */
	    tupcount = gcn_nq_scan( nq, masktup, &scan_cb, 
				    GCN_SVR_MAX, tupvec, NULL );

	    /* Loop through server entries */
	    for( i = 0; i < tupcount; i++ )
	    {
		GCN_TUP	*tup = tupvec[ i ];
		GCN_TUP	newtup;
		char 	buf[ 1024 ];
		char	*p = buf;

		newtup.uid = tup->uid;
		newtup.obj = tup->obj;
		newtup.val = tup->val;
		p += gcu_put_str( p, nq->gcn_type );
		p += gcn_put_tup( p, &newtup );

		if ( msg_out  &&  (p - buf) + msg_out > msg_post )
		{
		    /*
		    ** Update the row count for this message.
		    ** We send individual messages rather than
		    ** continued messages hoping (in vain) that
		    ** the client won't need to piece message
		    ** segments back together.
		    */
		    gcu_put_int( row_ptr, row_count );
		    mbuf->used = msg_out - mbuf->data;
		    row_count = 0;
		    msg_out = NULL;
		}

		if ( ! msg_out )
		{
		    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
		    if ( status != OK )  return( status );

		    mbuf->type = GCN_RESULT;
		    msg_out = mbuf->data;
		    msg_post = mbuf->data + mbuf->len;

		    msg_out += gcu_put_int( msg_out, GCN_RET );
		    row_ptr = msg_out;
		    msg_out += gcu_put_int( msg_out, 0 );
		}
			    
		MEcopy( buf, p - buf, msg_out );
		msg_out += p - buf;
		row_count++;
	    }
	} while( tupcount == GCN_SVR_MAX );

	gcn_nq_unlock( nq );
    }

    if ( ! msg_out )
	status = gcn_result( mbout, GCN_RET, 0 );
    else
    {
	(void)gcu_put_int( row_ptr, row_count );
	mbuf->used = msg_out - mbuf->data;
    } 

    return( status );
}


/*
** Name: gcn_op_add
**
** Description:
**	Read input message tuples from msg_in and add entries 
**	to the name queue specified by gcn_type.  Duplicate 
**	entries (those which match masktup) are first deleted 
**	from the queue.  Result message/buffer is added to 
**	callers message buffer queue.
**
** Inputs:
**	msg_in		Input tuples.
**	mbout		GCN_MBUF queue.
**	user		Client user ID.
**	gcn_type	GCN Class (queue name).
**	masktup		Deletion tuple mask.
**	num_tuple	Number of input tuples.
**	opflags		Operation flags.
**	srv_flags	Server flags.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Revamped.
**      23-Apr-89 (jorge)
**          Made gcn_uid parameter passed down from the
**          original GCA_LISTEN call.
**      14-jun-89 (seiwald)
**          Incoming message buffer pointer p now points to the
**          class of the first {class,uid,obj,value} tuple.
**      26-Jul-89 (seiwald)
**          Insert tuple in proper alphabetic position when adding.
**	02-Jan-91 (brucek)
**	    Support for GCN_MRG_FLAG (see comments at top)
**	06-Apr-91 (seiwald)
**	    Set modified flag to TRUE on deletes.  It was leaving it as
**	    FALSE and bypassing dumping the queue to the file.
**	07-May-91 (brucek)
**	    Assume no merge on ADDs to iinode (see comments at top).
**	18-Jun-91 (alan)
**	    Mark added tuples for incremental file update.
**	29-May-97 (gordy)
**	    Added GCS server key parameter.  Maintain registered server
**	    info (including server key) for ADD/DEL of transient queue.
**	15-Aug-97 (rajus01)
**	    Added GCN_ATTR name queue to specify security mechanisms.
**	31-Mar-98 (gordy)
**	    Only trusted users may update the registered server queue.
**	 7-Oct-98 (gordy)
**	    If SERVERS entry is being added manually, make sure the
**	    server key (uid) is cleared.  Only add SERVERS entry for
**	    registering servers if server key is provided.
**      10-Sep-02 (hanal04) Bug 110641 INGSRV2436
**          Convert gcn_type to upper case. Server classes should
**          be case insensitive as they were in releases prior to
**          AI 2.6.
**	15-Jul-04 (gordy)
**	    Encryption of stored passwords now different than what is
**	    used by utility programs: transform new LOGIN passwords.
**	 1-Mar-06 (gordy)
**	    Removed SERVERS class and moved server key storage.
**	    Separated into routines for specific operations.
**	 3-Aug-09 (gordy)
**	    Declare default sized value buffer.  Use dynamic storage
**	    if actual length exceeds default size.
*/

static STATUS
gcn_op_add
( 
    char	*msg_in,
    GCN_MBUF	*mbout, 
    char	*user, 
    char	*gcn_type,
    GCN_TUP	*masktup,
    i4		num_tuple,
    i4		opflags,
    i4		srv_flags
)
{
    GCN_QUE_NAME	*nq = NULL;
    i4			row_count, tup_count;
    STATUS		status = OK;
    char		flagbuf[ 9 ];
    char		valbuf[ 65 ];
    char		*newval = valbuf;

    /*
    ** Check that user has required privileges for operation requested.
    ** (These checks are bypassed if client is trusted.)
    */
    if ( ! (opflags & GCN_OPFLAG_TRUST) )
    {
	/*
	** SERVER_CONTROL is required to add a server entry.
	*/
	if ( opflags & GCN_OPFLAG_SRV )
	    status = gca_chk_priv( user, GCA_PRIV_SERVER_CONTROL );

	/*
	** NET_ADMIN is required to add a global vnode entry.
	*/
	if ( opflags & GCN_OPFLAG_VNODE  &&  opflags & GCN_OPFLAG_PUB )
	    status = gca_chk_priv( user, GCA_PRIV_NET_ADMIN );

	if ( status != OK )
	{
	    gcn_error( status, NULL, user, mbout );
	    return( status );
	}
    }

    /* Scan name list for given service class. */
    if ( ! (nq = gcn_nq_find( gcn_type )) )
	if ( ! (opflags & GCN_OPFLAG_SRV) )
	{
	    /* Can't find class.  */
	    gcn_error( E_GC0100_GCN_SVRCLASS_NOTFOUND, NULL, gcn_type, mbout );
	    return( E_GC0100_GCN_SVRCLASS_NOTFOUND );
	}
	else  if ( ! (nq = gcn_nq_create( gcn_type )) )
	{
	    gcn_error( E_GC0121_GCN_NOMEM, NULL, gcn_type, mbout );
	    return( E_GC0121_GCN_NOMEM );
	}

    /*
    ** Matching entries are deleted prior to adding new entries.
    ** When merging, only exact duplicates are deleted.  Otherwise,
    ** matches depend on various tuple fields for different classes.
    */
    if ( nq->gcn_transient )
    {
	/*
	** Registered servers match on listen address (value).
	** UID is overloaded with formatted server flags.
	*/
	if ( ! (opflags & GCN_OPFLAG_MRG) )  masktup->obj = "";
	masktup->uid = "";
	CVlx( (long)srv_flags, flagbuf );
    }
    else if ( opflags & GCN_OPFLAG_NODE )
    {
	/*
	** VNODE connection entries match on UID and VNODE (object).
	*/
	if ( ! (opflags & GCN_OPFLAG_MRG) )  masktup->val = "";
    }
    else if ( opflags & GCN_OPFLAG_LOGIN )
    {
	/*
	** VNODE login entries match on UID and VNODE (object).
	** Multiple values (merged entries) are not permitted.
	*/
	masktup->val = "";
    }
    else if ( opflags & GCN_OPFLAG_ATTR )
    {
	/*
	** VNODE attribute entries match on UID, VNODE (object)
	** and attribute name.
	*/
	if ( ! (opflags & GCN_OPFLAG_MRG) )
	{
	    /*
	    ** Tuple value is attribute name and its value.
	    ** Build match value to only have attribute name.
	    */
	    char	*pv[2];
	    i4  	str_len = STlength( masktup->val ) + 1;

	    if ( str_len > sizeof( valbuf ) )
	    {
	    	newval = (char *)MEreqmem( 0, str_len, FALSE, NULL );
		if ( ! newval )  return( E_GC0121_GCN_NOMEM );
	    }

	    (void)gcu_words( masktup->val, newval, pv, ',', 2 );
	    str_len = STlength( pv[0] );
	    pv[0][str_len] = ',';
	    pv[0][str_len + 1] = '\0';
	    masktup->val = pv[0];
	}
    }

    if ( gcn_nq_lock( nq, TRUE ) == OK )
    {
	/*
	** First delete any overlapping tuples and 
	** report that to the user.
	*/
	status = (row_count = gcn_nq_del( nq, masktup ))
	    	 ?  gcn_result( mbout, GCN_DEL, row_count ) : OK ;

	/*
	** Insert the tuples into the name queue.
	*/
	for( row_count = tup_count = 0; 
	     status == OK  &&  tup_count < num_tuple; tup_count++ )
	{
	    GCN_TUP	tup;
	    char	buff[ 256 ];
	    char	*login = buff;

	    /* 
	    ** Get next tuple from user request. 
	    */
	    msg_in += gcu_get_str( msg_in, &gcn_type );
	    msg_in += gcn_get_tup( msg_in, &tup );

	    /*
	    ** All tuples must have the same class.
	    */
	    if ( ! gcn_type[0] )  gcn_type = IIGCn_static.svr_type;
	    CVupper( gcn_type );
	    if ( STcompare( nq->gcn_type, gcn_type ) )  continue;

	    /* 
	    ** Set user ID appropriately.
	    */
	    if ( nq->gcn_transient )
		tup.uid = flagbuf;
	    else  if ( opflags & GCN_OPFLAG_PUB )
		tup.uid = GCN_GLOBAL_USER;
	    else  if ( ! (opflags & GCN_OPFLAG_UID) )
		tup.uid = user;

	    /*
	    ** Login passwords are encrypted by the client
	    ** and must be transformed into storage format.
	    */
	    if ( opflags & GCN_OPFLAG_LOGIN )  
	    {
		/*
		** The encryption algorithm for storage produces 
		** a slightly longer string than the communication 
		** encryption.  Ensure temp buffer is large enough.  
		** 50% larger is more than sufficient for current 
		** algorithm.
		*/
		i4 len = (STlength( tup.val ) * 3) / 2;

		if ( len >= sizeof( buff ) )
		    login = (char *)MEreqmem( 0, len + 1, FALSE, NULL );

		if ( ! login )
		    status = E_GC0121_GCN_NOMEM;
		else
		{
		    transform_login( tup.val, login );
		    tup.val = login;
	        }
	    }

	    if ( status == OK ) 
	    {
		row_count += gcn_nq_add( nq, &tup );
		if ( login != buff )  MEfree( (PTR)login );
	    }
	}

	if ( status == OK )
	    status = gcn_result( mbout, GCN_ADD, row_count );
	else
	    gcn_error( status, NULL, NULL, mbout );

	gcn_nq_unlock( nq );
    }
    else
    {
	status = E_GC0134_GCN_INT_NQ_ERROR;
	gcn_error( status, NULL, NULL, mbout );
    }

    if ( newval != valbuf )  MEfree( (PTR)newval );
    return( status );
}



/*
** Name: gcn_op_del
**
** Description:
**	Search the name queue specified by gcn_type and delete
**	tuples which match masktup.  Result message/buffer is
**	added to callers message buffer queue.
**
** Inputs:
**	mbout		GCN_MBUF queue.
**	user		Client user ID.
**	gcn_type	GCN Class (queue name).
**	masktup		Deletion tuple mask.
**	opflags		Operation flags.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Revamped.
**	31-Mar-98 (gordy)
**	    Only trusted users may update the registered server queue.
**	 7-Jul-98 (gordy)
**	    Produce 0 result rows rather than error for unknown server
**	    classes now that all server classes are dynamically created.
**	 1-Mar-06 (gordy)
**	    Separated into routines for specific operations.
*/

static STATUS
gcn_op_del
( 
    GCN_MBUF	*mbout, 
    char	*user, 
    char	*gcn_type,
    GCN_TUP	*masktup,
    i4		opflags
)
{
    GCN_QUE_NAME	*nq = NULL;
    i4			row_count = 0;
    STATUS		status = OK;

    /*
    ** Check that user has required privileges for operation requested.
    ** (These checks are bypassed if client is trusted.)
    */
    if ( ! (opflags & GCN_OPFLAG_TRUST) )
    {
	/*
	** Deleting a registered server requires SERVER_CONTROL privilege.
	*/
	if ( opflags & GCN_OPFLAG_SRV )
	    status = gca_chk_priv( user, GCA_PRIV_SERVER_CONTROL );

	/*
	** NET_ADMIN privilege is required to delete a global vnode entry.
	*/
	if ( opflags & GCN_OPFLAG_VNODE  &&  opflags & GCN_OPFLAG_PUB )
	    status = gca_chk_priv( user, GCA_PRIV_NET_ADMIN );

	if ( status != OK )
	{
	    gcn_error( status, NULL, user, mbout );
	    return( status );
	}
    }

    /* 
    ** Scan name list for given service class. 
    */
    if ( ! (nq = gcn_nq_find( gcn_type )) )
	if ( opflags & GCN_OPFLAG_SRV )
	    return( gcn_result( mbout, GCN_RET, 0 ) );	/* No servers */
	else
	{
	    /* Can't find class.  */
	    gcn_error( E_GC0100_GCN_SVRCLASS_NOTFOUND, NULL, gcn_type, mbout );
	    return( E_GC0100_GCN_SVRCLASS_NOTFOUND );
	}

    /*
    ** User ID is not applicable for registered servers.
    */
    if ( nq->gcn_transient )  masktup->uid = "";

    /* 
    ** When deleting, allow wild cards in obj and val.
    */
    if ( ! STcompare( masktup->obj, "*" ) )  
    	masktup->obj = "";
    else  if ( ! STcompare( masktup->obj, "\\*" ) )  
    	masktup->obj = "*";

    if ( ! STcompare( masktup->val, "*" ) )  
    	masktup->val = "";
    else  if ( ! STcompare( masktup->val, "\\*" ) )  
    	masktup->val = "*";

    /*
    ** Delete selected tuples.
    */
    if ( gcn_nq_lock( nq, TRUE ) == OK )
    {
	row_count = gcn_nq_del( nq, masktup );
	status = gcn_result( mbout, GCN_DEL, row_count );
	gcn_nq_unlock( nq );
    }
    else
    {
	status = E_GC0134_GCN_INT_NQ_ERROR;
	gcn_error( status, NULL, NULL, mbout );
    }

    return( status );
}



/*
** Name: gcn_result
**
** Description:
**	Generate a GCN message buffer containing a GCN_RESULT
**	message.  Message buffer is added to caller provided
**	message queue.
**
** Input:
**	mbout	GCN_MBUF queue.
**	opcode	GCN Operation.
**	count	Number of rows.
**
** Output:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
*/

static STATUS
gcn_result( GCN_MBUF *mbout, i4 opcode, i4 count )
{
    char	*msg_out;
    GCN_MBUF	*mbuf;
    STATUS	status = OK;

    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK ) return( status );

    mbuf->type = GCN_RESULT;
    msg_out = mbuf->data;
    msg_out += gcu_put_int( msg_out, opcode );
    msg_out += gcu_put_int( msg_out, count );
    mbuf->used = msg_out - mbuf->data;

    return( OK );
}


/*
** Name: transform_login
**
** Description:
**	Converts login password from format passed by client utility
**	into that used for storage.
**
**	Output may be slightly larger than the input.  A 50% larger
**	buffer is sufficient for the output.
**
** Input:
**	ibuff	Client login: user ID and encrypted password.
**
** Output:
**	obuff	Transformed login value.
**
** Returns:
**	STATUS	OK or FAIL
**
** History:
**	15-Jul-04 (gordy)
**	    Created.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold {encrypted} password.
**	 3-Aug-09 (gordy)
**	    Declare default sized temp buffers.  Use dynamic storage
**	    if actual lengths exceed defaults.
**	27-Aug-10 (gordy)
**	    Use configured password encoding version for login entries.
*/

static STATUS
transform_login( char *ibuff, char *obuff )
{
    char	*pv[ 2 ];
    char	pbuff[ 128 ], tbuff[ 128 ];
    char	*pwd, *tmp;
    i4		len;
    STATUS	status;

    /*
    ** Isolate user ID and encrypted password.
    ** Ensure output buffer is large enough.
    ** Decrypt the password.
    */
    if ( gcu_words( ibuff, NULL, pv, ',', 2 ) < 2 )  pv[1] = "";

    len = STlength( pv[1] );
    pwd = (len < sizeof( pbuff )) 
          ? pbuff : (char *)MEreqmem( 0, len + 1, FALSE, NULL );

    if ( ! pwd )
    	status = E_GC0121_GCN_NOMEM;
    else
	status = gcn_login( GCN_VLP_CLIENT, 0, FALSE, pv[0], pv[1], pwd );

    /*
    ** Re-encrypt password for storage
    */
    if ( status == OK )
    {
	/*
	** Ensure temp buffer is large enough.
	*/
	len = (STlength( pwd ) + 8) * 2;

	tmp = (len < sizeof( tbuff )) 
	      ? tbuff : (char *)MEreqmem( 0, len + 1, FALSE, NULL );

	if ( ! tmp )
	    status = E_GC0121_GCN_NOMEM;
	else
	    status = gcn_login( GCN_VLP_LOGIN, 
	    			IIGCn_static.pwd_enc_vers, 
				TRUE, pv[0], pwd, tmp );
    }

    /*
    ** Rebuild the login tuple value.
    */
    if ( status == OK )
    {
	char vers[ 32 ];

	STprintf( vers, "V%d", IIGCn_static.pwd_enc_vers );
	STpolycat( 5, pv[0], ",", tmp, ",", vers, obuff );
    }

    if ( pwd != pbuff )  MEfree( (PTR)pwd );
    if ( tmp != tbuff )  MEfree( (PTR)tmp );
    return( status );
}

