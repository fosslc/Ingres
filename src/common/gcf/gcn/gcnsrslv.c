/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <me.h>
#include <cv.h>
#include <er.h>
#include <gc.h>
#include <mh.h>
#include <mu.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <tr.h>
#include <tm.h>

#include <sl.h>
#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcu.h>
#include <gcs.h>
#include <gcaint.h>
#include <gcaimsg.h>
#include <gcnint.h>

/**
**  Name: gcnsrslv.c - process GCN_NS_RESOLVE message
**
**  Description:
**	This module takes a GCN_NS_RESOLVE message, containing a target
**	database string (all components are optional):
**
**		vnode::dbname/class
**
**	and produces a GCN_RESOLVED message containing the information
**	necessary for GCA_REQUEST to establish a local or remote
**	connection.
**
**	If vnode:: is not present and II_GCNxx_RMT_VNODE is defined,
**	that value is used as a default (except if class is IINMSVR).
**
**	If vnode:: is present but it matches the value II_GCNxx_LCL_VNODE
**	it is stripped.
**
**	If /class is not present, it defaults to "/ingres".
**
**	For remote connections (with vnode::), the COMSVR server class
**	is used implicity and the /class is forwarded.
**
**	Class may take the form @addr to connect directly to the
**	server with listen address addr.
**
**	Vnode may take the form @host,proto,port to specify the network 
**	connection information directly.  @host is a special variation 
**	which uses the installation registry protocols and known ports 
**	for the connection.  Use @::/iinmsvr to make a local connection 
**	to the installation registry master Name Server.  Any other 
**	usage of @:: is handled as a local installation connection.
**
**	Any vnode spec may contain a username and password pair in the 
**	format vnode[user,pwd]::.  The username and password are used 
**	instead of the vnode login information and the NS_2_RESOLVE 
**	username and password.  This format may be used to specify 
**	the username and password for local connections using 
**	[user,pwd]::db/class.
**
**    External routines
**	gcn_rslv_init		Unwrap the GCN_NS_RESOLVE message.
**	gcn_rslv_parse		Parse targent into components.
**	gcn_rslv_login		Lookup login info for vnode.
**	gcn_rslv_node		Lookup connection info for vnode.
**	gcn_rslv_attr		Lookup attribute info for vnode.
**	gcn_rslv_server		Build local server list.
**	gcn_rslv_err		Propagate error status.
**	gcn_rslv_done		Build final GCN_RESOLVED message.
**	gcn_rslv_cleanup	Cleanup after resolve completed.
**	gcn_unparse		Format vnode::dname/class.
**
**    Internal routines
**	gcn_rslv_rand		Randomize a bunch of tuples.
**	gcn_rslv_sort		Sort a bunch of tuples based on no_assocs.
**	gcn_parse		Parse vnode::dname/class.
**	
**  History:
**      28-Sep-87   (lin)
**          Initial creation.
**	20-Jan-89 (jorge)
**	    Changed II_DEF_VNODE logical to II_GCNxx_RMT_VNODE,
**	    Changed II_INS_VNODE logical to II_GCNxx_LCL_VNODE,
**	    Changed II_DEF_CLASS logical to II_GCNxx_SVR_TYPE
**	    where 'xx' is the II_INSTALLATION id.
**	    Forced GCN_NMSVR (ie. "IINMSVR") server requests to always
**	    skip local II_GCNxx_RMT_VNODE definitions and be considered local. 
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.  Message formats between
**	    IIGCN and its clients remain the same, but the messages are
**	    constructed and extracted via properly aligning code.
**	    Variable length structures eliminated.  Strings are now
**	    null-terminated (they had to before, but no one admitted so).
**	05-Apr-89 (jorge)
**	    Upraded the UID info lookup to search for : ext_vnode, lcl_vnode,
**	    and finally whatever the clinet had directly provided as the local
**	    UID info, in that order.
**      18-Apr-89 (jorge)
**	    put in quick kludge to force lookups to use the GCA_REQUEST
**	    username rather than the GCACM.C provide client process uid.
**   	    This is required for STAR. A permanent fix should be done to 
**	    use GCN's GCA_LISTEN username directly.
**      18-Apr-89 (jorge)
**	    Put in fix to generate remote partner_id as concatenated 
** 	    dbname/svr_class. Introduced new GCN_PARTNER function.
**      18-Apr-89 (jorge)
**  	    upgraded so that the returned SVR_CLASS is the true server class.
**	23-Apr-89 (jorge)
**	    Made gcn_uid parameter passed down from the 
**	    original GCA_LISTEN call. This guaranties that the UID/PASSWD
**	    has been validated. Note that the info is redundantly provided
**	    in the gcn request message, but it may not be correct there.
**	25-Apr-89 (jorge)
**	    Moved GCN_PAARSE() error handling code into gcn_resolve().
**	    Made the code contional on GCACM_HANDLE ERR ifdefs.
**	27-Apr-89 (jorge)
**	    Structred code so that the NS_RESOLVE messgae always has the 
**	    correct  entries. This is important to STAR since the AUX data
**	    type must be set based on the conetnts of vnode (see GCACM.C).
**	    Old code resulted in NS_RESPONSE messgae being miss-alligned for
**	    local IPC connection.
**	3-May-89 (jorge)
**	    Fixed bug 5513; bad svr_type on remote node. Use default only
**	    after the NS_RESPONSE is packed. 
**	3-May-89 (jorge)
**	    Use GCA_REQUEST password as default (after encryption).	
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	08-Nov-89 (seiwald)
**	    Include mh.h so as to declare MHrand() properly as an f8, and
**	    then use MHrand() properly as an f8.  Previously, the f8 bit
**	    pattern returned from MHrand() was being used as an i4.
**	16-Nov-89 (seiwald)
**	    Rework calls to gcn_error for their potential use.
**	25-Nov-89 (seiwald)
**	    Reworked for async GCN.
**	04-Jan-90 (seiwald)
**	    GCA_REQUEST's below GCA_PROTOCOL_LEVEL_4 can only handle
**	    GCN_RESOLVED messages.  Below that level, if a resolve fails
**	    we'll send an empty GCN_RESOLVED message instead of a 
**	    GCA_ERROR message.
**	03-Feb-90 (seiwald)
**	    Remote connection rollover: at GCA_PROTOCOL_LEVEL_4 the
**	    GCN_RESOLVED message contains a count and array of
**	    node,protocol,port tuples for a remote vnode, rather than
**	    just allowing one tuple.  The tuples are prioritised
**	    (private then global) and randomised, just as are local
**	    server addresses.  Through netu, a user may add multiple
**	    node connection entries for a single vnode.
**	02-Oct-90 (jorge)
**	    Added call to gcn_cleanup() so that stale transient
**	    entries can be removed if enough time (5 min def) has passed.
**	08-Oct-90 (jorge)
**	    Due to introduction of GCA_PROTOCOL_LEVEL_31 (TIME_ZONE patch),
**	    Remote connection "fail-over" will be suported at 
**	    GCA_PROTOCOL_LEVEL_40 instead of GCA_PROTOCOL_LEVEL_4.
**	4-Dec-90 (seiwald)
**	    Support for GCN_SOL_FLAG: the uid for transient entries
**	    (server registries) nows contains the gcn_flag word from
**	    GCN_D_OPER structure of the GCN_ADD operation.  It is 
**	    formatted as a hex string and used by GCN_RESOLVE.  (The
**	    uid was previously '*' - as there are no private server
**	    registries.)
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	24-Apr-91 (brucek)
**	    Added include for tm.h
**	28-Jun-91 (seiwald)
**	    Internal name queue access restructured.  See gcnsfile.c
**	    for new description.
**	31-Jan-92 (seiwald)
**	    Support for installation passwords: gcn_resolve operation
**	    split into separate functions for each step, so that the
**	    state machine in gcn_server() can orchestrate the resolve.
**	23-Mar-92 (seiwald)
**	    Defined GCN_GLOBAL_USER as "*" to help distinguish the owner
**	    of global records from other uses of "*".
**	27-Mar-92 (seiwald)
**	    Was treating the val of a server entry as the flagword -
**	    the uid of a server entry is its flagword.
**	28-Mar-92 (seiwald)
**	    Function headers added.
**	1-Apr-92 (seiwald)
**	    Documented.
**	1-Apr-92 (seiwald)
**	    Some functions consolidated.
**	1-Apr-92 (seiwald)
**	    The catalogs [catl, catr] in the GCN_RESOLVE_CB are now
**	    string buffers rather than name queue tuple pointers.
**	31-Aug-92 (brucek)
**	    Support for pre-resolved target strings: if server class
**	    begins with a @, the remainder of string is treated as raw 
**	    GCA address of target server.
**	18-Feb-92 (edg)
**	    gcn_cleanup() no longer used.
**	16-Mar-93 (edg)
**	    gcn_rslv_server() uses gcn_rslv_sort() to get back a list of
**	    local connections sorted by no_assocs (low to high).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	27-nov-95 (nick)
**	    In gcn_rslv_server(), return sole_server for both named objects
**	    and global objects, rather than the first we find. #72834
**	28-Nov-95 (gordy)
**	    When configured for an embedded Comm Server, a registered
**	    Comm Server is not required to support out-bound connections.
**	    If there is no GCC for a remote connection, return an address
**	    of 'unknown' which will be ignored by the embedded Comm Server,
**	    satisfy those who require an address, and fail if it gets
**	    passed to the wrong functions (GCA CL).
**	 4-Dec-95 (gordy)
**	    Created gcn_rslv_parse() from part of gcn_rslv_init().
**	    Added prototypes.
**	 4-Aug-96 (gordy)
**	    Use the local user ID to access node/login info.  Remote
**	    username and password should not be used locally.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	27-May-97 (thoda04)
**	    Included cv.h for the function prototypes.
**	29-May-97 (gordy)
**	    Redefined user/password conventions in resolve control block:
**	    user_in/password_in are strictly from NS_2_RESOLVE, grcb->user
**	    is from listen.  Use remote username and password from 
**	    NS_2_RESOLVE messages for local connections too (password must 
**	    be validated locally).  Otherwise, local connections always use 
**	    user from GCA listen (special STAR provisions replaced by GCS 
**	    server auths).  New RESOLVED message at PROTOCOL_LEVEL_63 to 
**	    support GCS server auths.
**	08-Jul-97 (sarjo01)
**	    Test for "IIPROMPTxxx" strings and move password insertion
**	    point to allow for longer userids. Bug 83658
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	    Added defines for known attributes.
**	 5-May-98 (gordy)
**	    Password auth now takes user alias.  Server auth takes client ID.
**	15-May-98 (gordy)
**	    Added support for remote authentication.
**	21-May-98 (gordy)
**	    No login info is permitted to allow for remote authentication.
**	    Use default remote mechanism if not specified in vnode.
**	27-May-98 (gordy)
**	    Make sure there is some type of auth for remote connections.
**	10-Jun-98 (gordy)
**	    Adjusted trace levels.
**	16-Jun-98 (gordy)
**	    Use hashed access scan for vnodes info for better performance.
**	27-Aug-98 (gordy)
**	    Generate remote auth if delegated auth token available.
**	 1-Sep-98 (gordy)
**	    Since all transient classes are now dynamic, return
**	    NO GCC error message if can't find COMSVR queue for
**	    remote requests.
**	30-Sep-98 (gordy)
**	    Handle pre-resolved VNODE @<host>.
**	 2-Oct-98 (gordy)
**	    Fix handling of quotes in target specifications.  Enhance
**	    vnode format to allow full specification of vnode connection 
**	    and login information: @host,proto,port[user,pwd].
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**      13-Jul-2001 (wansh01)
**          Enhanced vnode format to allow attributes specification
**          as (name1=value1;name2=value2;....) 
**          Changed grcb->target to a ptr. use gcu_alloc_str() to
**          extract the target.
**          Changed gcn_parse() to 2 do while loops
**	27-aug-2001 (somsa01)
**	    In gcn_parse(), before starting the second 'do...while' loop,
**	    reinitialize 'in' to 'dbname' if we didn't find a vnode.
**	28-Jan-04 (gordy)
**	    Do not allow installation passwords to be used explicitly.
**	15-Jul-04 (gordy)
**	    Resolved password is now flagged if encrypted.  Enhanced
**	    encryption of stored passwords and passwords passed to gcc.
**	 5-Aug-04 (gordy)
**	    Extend vnode string to allow simply host and port and use
**	    the 'default' protocol.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**	 8-Jun-07 (kibro01) b118226
**	    Disallow chained vnodes (vnode1::vnode2::db) for now since
**	    they are not yet truly supported.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Remove special Windows IIPROMPT code which is no longer
**	    needed (and no longer serves its original purpose).
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
**/

/*
**  Forward and/or External function references.
*/

static i4	gcn_rslv_rand( GCN_QUE_NAME *, GCN_TUP *, i4, GCN_TUP ** );
static i4	gcn_rslv_sort( GCN_QUE_NAME *, GCN_TUP *, i4, GCN_TUP * );
static STATUS	gcn_parse( char *, char **, char **, char ** );
static STATUS	gcn_pwd_override( GCN_RESOLVE_CB *, char * );
static VOID	gcn_attr_override( GCN_RESOLVE_CB *, char * );
static VOID     gcn_set_attr( GCN_RESOLVE_CB *, char ** );
static STATUS	gcn_use_login( GCN_RESOLVE_CB *grcb );
static STATUS	gcn_get_login( GCN_RESOLVE_CB *grcb );


/*{
** Name: gcn_rslv_init() - unwrap GCN_NS_RESOLVE/GCN_NS_2_RESOLVE messages
**
** Description:
**	Extracts the GCN_NS_RESOLVE or GCN_NS_2_RESOLVE message hanging 
**	off of mbin and initializes the GCN_RESOLVE_CB.
**
** Inputs:
**	grcb		Control block for resolve operation
**	mbin		GCN_NS_RESOLVE or GCN_NS_2_RESOLVE message
**	account		User ID from GCA_LISTEN.
**	user		User alias from GCA_LISTEN.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	18-Nov-92 (gautam)
**	    Added in support for unwrapping GCN_NS_2_RESOLVE  message.
**	 4-Dec-95 (gordy)
**	    Remove actions not directly associated with NS[_2]_RESOLVE.
**	29-May-97 (gordy)
**	    Removed password parameter (no longer available from listen).
**	    Redefined user/password conventions in resolve control block:
**	    user_in/password_in are strictly from NS_2_RESOLVE, grcb->user
**	    is from listen.
**	17-Oct-97 (rajus01)
**	    Initialized emode and emech.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 5-May-98 (gordy)
**	    Server auth takes client ID.
**	15-May-98 (gordy)
**	    Added support for remote authentication.
**	27-Aug-98 (gordy)
**	    Use default remote mechanism if password not provided.
**	15-Jul-04 (gordy)
**	    Resolved password may be encrypted or left as plain text.
**      04-Apr-08 (rajus01) SD issue 126704, Bug 120136
**	    JDBC applications report E_GC1010 error when either password
**	    or username specified in the JDBC URL is longer but less than the
**	    supported limit (GC_L_PASSWORD, GC_L_USER_ID). Also when user
**	    and/or password length exceed the supported lengths
**	    GCN crashes and E_CLFE07 is returned to the application.
**	    Check for the lenghth of the username/password strings and copy
**	    only strings of length GC_L_USER_ID, GC_L_PASSWORD respectivly to
**	    user_in, passwd_in fields  of the GCN_RESOLVE_CB to avoid the 
**	    corruption of the 'grcb' buffer. This memory corruption  causes
**	    GCN to crash.
**	21-Jul-09 (gordy)
**	    Reference account and client user ID rather than copying.  
**	    Combined usr/pwd buffer Replaced with separate default sized 
**	    buffers.  Installation password is dynamically allocated.
**	    Added buffer sizes to gcn_copy_str().  Use dynamic storage
**	    if user alias or password exceed default sizes.
*/

STATUS
gcn_rslv_init(GCN_RESOLVE_CB *grcb, GCN_MBUF *mbin, char *account, char *user)
{
    char	*msg_in = mbin->data;
    char	*msg_end = msg_in + mbin->len;
    char	*tmp;

    grcb->status = OK;
    grcb->flags = 0;
    grcb->target = NULL;
    grcb->vnode = grcb->dbname = grcb->class = "";
    grcb->account = account;
    grcb->username = user;
    grcb->usrptr = grcb->usrbuf;
    grcb->pwdptr = grcb->pwdbuf;
    *grcb->usrptr = *grcb->pwdptr = EOS;
    grcb->usr = grcb->pwd = "";
    grcb->ip = NULL;
    grcb->auth = NULL;
    grcb->auth_len = 0;
    grcb->emode[0] = grcb->emech[0] = grcb->rmech[0] = EOS;
    grcb->catl.tupc = grcb->catr.tupc = 0;

    if ( grcb->gca_message_type != GCN_NS_2_RESOLVE  &&
	 STcasecmp( IIGCn_static.rmt_mech, ERx("none") ) )
	STcopy( IIGCn_static.rmt_mech, grcb->rmech );

    /*
    ** Un-wrap the incoming message.  Use copy_str() to copy 
    ** and null-terminate the strings.  Old versions of GCA 
    ** sent over unterminated strings.  Ignore the uid in the 
    ** message.  We trust the one from GCA_LISTEN.
    */
    msg_in += gcu_get_str( msg_in, &tmp ); /* skip install */
    msg_in += gcu_get_str( msg_in, &tmp ); /* skip uid */
    msg_in += gcu_alloc_str( msg_in, &grcb->target );

    if ( ! grcb->target ) 
    {
	if ( IIGCn_static.trace >= 4 )
	    TRdisplay( "gcn_rslv_init: gcu_alloc_str error\n");
	return( E_GC0121_GCN_NOMEM );
    }
	
    msg_in += gcu_get_str( msg_in, &tmp ); /* skip gcn_user */
    msg_in += gcu_get_str( msg_in, &tmp ); /* skip gcn_passwd */

    if ( grcb->gca_message_type == GCN_NS_2_RESOLVE )
    {
 	i4 len;
 
 	gcu_get_int( msg_in, &len );	/* don't increment msg_in yet. */
	len = min( len, msg_end - msg_in ) + 1;

	grcb->usrptr = (len <= sizeof( grcb->usrbuf )) ? grcb->usrbuf 
			: (char *)MEreqmem( 0, len, FALSE, NULL );

 	if ( ! grcb->usrptr )  return( E_GC0121_GCN_NOMEM );
	msg_in += gcn_copy_str( msg_in, msg_end - msg_in, grcb->usrptr, len );

 	gcu_get_int( msg_in, &len );	/* don't increment msg_in yet. */
	len = min( len, msg_end - msg_in ) + 1;

	grcb->pwdptr = (len <= sizeof( grcb->pwdbuf )) ? grcb->pwdbuf 
		 	: (char *)MEreqmem( 0, len, FALSE, NULL );

 	if ( ! grcb->pwdptr )  return( E_GC0121_GCN_NOMEM );
	msg_in += gcn_copy_str( msg_in, msg_end - msg_in, grcb->pwdptr, len );
    }

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_resolve: partner name '%s'\n", grcb->target );

    return( OK );
}


/*
** Name: gcn_rslv_parse
**
** Description:
**	Parse the requested target into components.  The following
**	conventions are enforced.
**
**	If /class is the Star synonym, replace with actual Star class.
**
**	If vnode:: is not present and II_GCNxx_RMT_VNODE is defined,
**	that value is used as a default (except if class is IINMSVR).
**
**	If vnode:: is present but it matches the value II_GCNxx_LCL_VNODE
**	it is stripped.
**
** Input:
**	grcb		Control Block for resolve operation.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	 4-Dec-95 (gordy)
**	    Extracted from gcn_rslv_init().
**	 2-Oct-98 (gordy)
**	    The parser allows for quoted components, so be sure
**	    to remove the quoting before additional processing.
**	    The quoting is removed here rather than in the parser
**	    because we need to know if dbname is quoted.
**	 8-Jun-07 (kibro01) b118226
**	    Disallow chained vnodes (vnode1::vnode2::db) for now since
**	    they are not yet truly supported.
**	21-Jul-09 (gordy)
**	    Check for errors in gcn_pwd_override().
*/

STATUS
gcn_rslv_parse( GCN_RESOLVE_CB *grcb )
{
    char	*srch;
    STATUS	status;

    /* 
    ** Parse the external name and return the corresponding value 
    */
    if ( gcn_parse( grcb->target, 
		    &grcb->vnode, &grcb->dbname, &grcb->class ) != OK )
	return( E_GC0130_GCN_BAD_SYNTAX );

    /*
    ** Unquote vnode and class for processing.
    ** Unquote dbname for display and comparison.
    ** DBname will be passed on in same state as
    ** it arrived.  Class will be re-quoted if it
    ** contains any delimiter characters.  
    */
    gcn_unquote( &grcb->vnode );
    gcn_unquote( &grcb->class );
    if ( gcn_unquote( &grcb->dbname ) )  grcb->flags |= GCN_RSLV_QUOTE;

    /*
    ** See if vnode contains a username & password for the connection.
    */
    if ( (status = gcn_pwd_override( grcb, grcb->vnode )) != OK )
        return( status );

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_resolve: '%s'::'%s'/'%s' for user %s\n", 
		    grcb->vnode, grcb->dbname, grcb->class, grcb->username );

    /*
    ** See if vnode contains attrs (name=value) for the connection.
    */
    gcn_attr_override( grcb, grcb->vnode );

    /* 
    ** The following is a KLUDGE to maintain reverse compatibility
    ** with Rel 5.0 STAR, where server type is "d", not "star".
    */
    if ( ! STcasecmp( grcb->class, GCN_STSYN ) )  grcb->class = GCN_STAR;

    /*
    ** If no virtual node specified, check whether there is a default
    ** remote defined. NOTE: this is not done if the local server
    ** type is Name Server. 
    */
    if ( ! *grcb->vnode  &&  STcasecmp( grcb->class, GCN_NMSVR ) )
	grcb->vnode = IIGCn_static.rmt_vnode;

    /*
    ** If the virtual node name is the defined local one, treat it as local.
    */
    if ( ! STbcompare( grcb->vnode, 0, IIGCn_static.lcl_vnode, 0, TRUE ) )
	grcb->vnode = "";

    /* Check for DBNAME containing second (chained) vnode */
    for( srch = grcb->dbname; *srch; srch++ )
    {
	if ( srch[0] == ':'  &&  srch[1] == ':' )
	    return( E_GC0164_GCN_CHAINED_VNODE );
    }

    return( OK );
}


/*{
** Name: gcn_rslv_node() - lookup connection info for vnode
**
** Description:
**	For remote connections, looks up the connection information in
**	the GCN_NODE queue.  Looks first for a private entry, then
**	a global entry.
**
**	Users update the GCN_NODE queue with NETU.
**
** Inputs:
**	grcb	control block for resolve operation
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	29-May-97 (gordy)
**	    Do nothing if for local connections (Special STAR 
**	    provisions replaced by GCS server auths).
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	30-Sep-98 (gordy)
**	    Handle pre-resolved VNODE @<host>.
**	 2-Oct-98 (gordy)
**	    Enhance vnode format to allow full specification of vnode 
**	    connection and login information: @host,proto,port[user,pwd].
**	 5-Aug-04 (gordy)
**	    Allow specification of just host and port and use default protocol.
**	21-Jul-09 (gordy)
**	    A single default sized remote info buffer is available.
**	    Use dynamic storage if length exceeds default size for
**	    first entry and dynamic storage for all other entries.
*/

STATUS
gcn_rslv_node( GCN_RESOLVE_CB *grcb )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		tupmask;
    GCN_TUP		*tupv[ GCN_SVR_MAX ];
    i4			tupc;

    /*
    ** Nothing to do for local requests.
    */
    if ( ! *grcb->vnode )  return( OK );

    /*
    ** Check for pre-resolved vnode: @<host>
    */
    if ( *grcb->vnode == '@' )
    {
	char	*pv[ 4 ];
	i4	len, count;

	/*
	** Connection info provided directly in vnode
	** string.  The three components host, protocol,
	** and port are separated by commas.  Various
	** combinations of components are permitted:
	** the valid combinations and how the missing
	** are replaced is as follows:
	**
	**	 #	Components		Replacements
	**	---	----------		------------
	**	 1	<host>			Registry protocol & port
	**	 2	<host>,<port>		Default protocol
	**	 3	<host>,<proto>,<port>
	*/
	len = STlength( &grcb->vnode[1] ) + 1;

	grcb->catr.rmtptr[0] = (len <= sizeof( grcb->catr.rmtbuf ))
		? grcb->catr.rmtbuf : (char *)MEreqmem( 0, len, FALSE, NULL );

	if ( ! grcb->catr.rmtptr )  return( E_GC0121_GCN_NOMEM );
	count = gcu_words( &grcb->vnode[1], grcb->catr.rmtptr[0], pv, ',', 4 );

	switch( count )
	{
	case 1 :	/* Host provided, use registry protocols & ports */
	    gcn_ir_proto( grcb, pv[0] );
	    break;

	case 2 :	/* Host & port provided, use default protocol */
	    grcb->catr.node[0] = pv[0];
	    grcb->catr.port[0] = pv[1];
	    grcb->catr.protocol[0] = ERx("default");
	    grcb->catr.tupc = 1;
	    break;

	case 3 :	/* Host, protocol and port provided */
	    grcb->catr.node[0] = pv[0];
	    grcb->catr.protocol[0] = pv[1];
	    grcb->catr.port[0] = pv[2];
	    grcb->catr.tupc = 1;
	    break;
	}

	return( grcb->catr.tupc ? OK : E_GC0132_GCN_VNODE_UNKNOWN );
    }

    /* 
    ** For remote requests we have to resolve the node name
    ** and password information.  Translate the logical node 
    ** name into network address, protocol, and port id. 
    ** These three elements are concatenated by commas.
    */
    if ( ! (nq = gcn_nq_find( GCN_NODE ))  ||  gcn_nq_lock( nq, FALSE ) != OK )
	return( E_GC0134_GCN_INT_NQ_ERROR );

    /*
    ** Get entries private to the user 
    */
    tupmask.uid = grcb->username;
    tupmask.obj = grcb->vnode;
    tupmask.val = "";
    tupc = gcn_rslv_rand( nq, &tupmask, GCN_SVR_MAX, tupv );

    /* 
    ** Get global entries 
    */
    tupmask.uid = GCN_GLOBAL_USER;
    tupc += gcn_rslv_rand( nq, &tupmask, GCN_SVR_MAX - tupc, tupv + tupc );

    /* 
    ** Copy results 
    */
    for( grcb->catr.tupc = 0; grcb->catr.tupc < tupc; grcb->catr.tupc++ )
    {
	char	*pv[3];
	i4	len = STlength( tupv[ grcb->catr.tupc ]->val ) + 1;

	pv[0] = pv[1] = pv[2] = "";
	grcb->catr.rmtptr[ grcb->catr.tupc ] = 
		(grcb->catr.tupc == 0  &&  len <= sizeof( grcb->catr.rmtbuf ))
		? grcb->catr.rmtbuf : (char *)MEreqmem( 0, len, FALSE, NULL );

	if ( ! grcb->catr.rmtptr[ grcb->catr.tupc ] )
	    return( E_GC0121_GCN_NOMEM );

	gcu_words( tupv[ grcb->catr.tupc ]->val, 
		   grcb->catr.rmtptr[ grcb->catr.tupc ], pv, ',', 3 );

	grcb->catr.node[ grcb->catr.tupc ] = pv[0];
	grcb->catr.protocol[ grcb->catr.tupc ] = pv[1];
	grcb->catr.port[ grcb->catr.tupc ] = pv[2];
    }

    gcn_nq_unlock( nq );

    if ( ! grcb->catr.tupc )
	if ( grcb->vnode == IIGCn_static.rmt_vnode )
	    return( E_GC0131_GCN_BAD_RMT_VNODE );
	else
	    return( E_GC0132_GCN_VNODE_UNKNOWN );

    return( OK );
}


/*{
** Name: gcn_rslv_login() - lookup password for vnode
**
** Description:
**	For remote connections, looks up the user's name and password
**	in the GCN_LOGIN queue.  Looks first for a private entry, then
**	a global entry.
**
**	Users update the GCN_LOGIN queue with NETU.
**
** Inputs:
**	grcb	control block for resolve operation
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	29-May-97 (gordy)
**	    Use remote username and password from NS_2_RESOLVE messages
**	    for local connections too (password must be validated locally).
**	    Otherwise, local connections always use user from GCA listen
**	    (special STAR provisions replaced by GCS server auths).
**      08-Jul-97 (sarjo01)
**          Test for "IIPROMPTxxx" strings and move password insertion
**          point to allow for longer userids. Bug 83658
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 5-May-98 (gordy)
**	    Password auth now takes user alias.
**	21-May-98 (gordy)
**	    No login info is permitted to allow for remote authentication.
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
**	30-Sep-98 (gordy)
**	    Handle pre-resolved VNODE @<host>.
**	22-Oct-98 (gordy)
**	    Don't allow the reserved user ID in local NS_2_RESOLVE requests.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Remove special Windows IIPROMPT code which is no longer
**	    needed (and no longer serves its original purpose).
*/

STATUS
gcn_rslv_login( GCN_RESOLVE_CB *grcb )
{
    STATUS		status = OK;

    /*
    ** By default, use the user ID of client process.
    */
    grcb->usr = grcb->username;

    /* 
    ** Login information may be provided as part of the resovle
    ** request (GCN_NS_2_RESOLVE), as part of a pre-resolved
    ** vnode (simulates GCN_NS_2_RESOLVE), or from an actual
    ** vnode.
    */ 
    if ( grcb->gca_message_type == GCN_NS_2_RESOLVE )
	status = gcn_use_login( grcb );
    else  if ( *grcb->vnode  &&  *grcb->vnode != '@' )
	status = gcn_get_login( grcb );

    return( status );
}


/*
** Name: gcn_use_login
**
** Description:
**	Use the login information provided in a GCN_NS_2_RESOLVE
**	request.  Note that login information contained in a pre-
**	resolved vnode ('@[usr,pwd]') has already been parsed and
**	used to simulate a GCN_NS_2_RESOLVE request.
**
**	The login info is validated for local connections.
**
** Input:
**	grcb	Resolve control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	28-Jan-04 (gordy)
**	    Extracted from gcn_rslv_login().  Don't permit
**	    explicit use of installation passwords.
**	15-Jul-04 (gordy)
**	    Resolved password may be encrypted or left as plain text.
*/

static STATUS
gcn_use_login( GCN_RESOLVE_CB *grcb )
{
    STATUS status = OK;

    if ( IIGCn_static.trace >= 4 )
       TRdisplay( "gcn_rslv_login: resolving GCN_NS_2_RESOLVE message\n" );

    /*
    ** Installation passwords may not be provided explicitly.
    ** The reserved internal login is not allowed for local
    ** connections, but is allowed for remote browsing.
    */
    if ( ! STcompare( GCN_NOUSER, grcb->usrptr ) )
	if ( STcompare( GCN_NOPASS, grcb->pwdptr ) )
	    return( E_GC0145_GCN_INPW_NOT_ALLOWED );
	else  if ( ! *grcb->vnode )
	    return( E_GC000B_RMT_LOGIN_FAIL );

    grcb->usr = grcb->usrptr;

    if ( *grcb->vnode )
    {
	grcb->pwd = grcb->pwdptr;
	grcb->flags &= ~GCN_RSLV_PWD_ENC;
    }
    else
    {
	/*
	** For local connections, we must validate the
	** password provided in the GCN_NS_2_RESOLVE.
	** First generate a password authentication, 
	** then validate it.
	*/
	GCS_PWD_PARM	pwd;
	char		auth[ GCN_AUTH_MAX_LEN ];

	pwd.user = grcb->usrptr;
	pwd.password = grcb->pwdptr;
	pwd.length = sizeof( auth );
	pwd.buffer = (PTR)auth;

	status = IIgcs_call( GCS_OP_PWD_AUTH, GCS_NO_MECH, (PTR)&pwd );

	if ( status == OK )
	{
	    GCS_VALID_PARM	valid;

	    valid.user = grcb->usrptr;
	    valid.alias = grcb->usrptr;
	    valid.length = pwd.length;
	    valid.auth = pwd.buffer;
	    valid.size = 0;

	    status = IIgcs_call( GCS_OP_VALIDATE, GCS_NO_MECH, (PTR)&valid );
	}
    }

    return( status );
}


/*
** Name: gcn_get_login
**
** Description:
**	Get login information associated with a vnode.
**
** Input:
**	grcb	Resolve control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	28-Jan-04 (gordy)
**	    Extracted from gcn_rslv_login().
**	15-Jul-04 (gordy)
**	    Enhanced encryption of stored passwords.
**	21-Jul-09 (gordy)
**	    Use same buffers as explicit usr/pwd to hold results.
**	    Use dynamic storage if username or password exceed
**	    default sized buffers.
*/

static STATUS
gcn_get_login( GCN_RESOLVE_CB *grcb )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		*tup, tupmask;
    PTR			scan_cb;
    i4			tupcount;

    if ( ! (nq = gcn_nq_find( GCN_LOGIN ))  || 
	 gcn_nq_lock( nq, FALSE ) != OK )
	return( E_GC0134_GCN_INT_NQ_ERROR );

    /* 
    ** Look-up login information for vnode.
    ** Get entry private to the user 
    */
    tupmask.uid = grcb->username;
    tupmask.obj = grcb->vnode;
    tupmask.val = "";
    scan_cb = NULL;
    tupcount = gcn_nq_hscan( nq, &tupmask, &scan_cb, 1, &tup, NULL );

    if ( ! tupcount )
    {
	/* 
	** No private entries, try global entry.
	*/
	tupmask.uid = GCN_GLOBAL_USER;
	scan_cb = NULL;
	tupcount = gcn_nq_hscan( nq, &tupmask, &scan_cb, 1, &tup, NULL );
    }

    gcn_nq_unlock( nq );

    /*
    ** No error is generated if login information does not
    ** exist since remote authentication may be possible.
    */
    if ( tupcount )
    {
	char 	*pv[3], *ptr;
	i4	pl[3];
	i4	count;
	i4	version = 0;

	/*
	** Extract user ID, encrypted password and possibly
	** the encryption version from comma separated list.
	*/
	for( ptr = pv[0] = tup->val, count = 0; ; ptr++ )
	    if ( ! *ptr )
	    {
		pl[count] = ptr - pv[count];
	    	count++;
	    	break;
	    }
	    else  if ( *ptr == ',' )
	    {
	    	pl[count] = ptr - pv[count];
		count++;
		if ( count >= 3 )  break;
		pv[count] = ptr + 1;
	    }

	/*
	** There should always be a password, but if not then
	** force the length to zero to ensure proper handling.
	*/
	if ( count < 2 )  pl[1] = 0;

	/*
	** Provide buffer space for username and password.
	*/
	if ( grcb->usrptr  &&  grcb->usrptr != grcb->usrbuf )
	    MEfree( (PTR)grcb->usrptr );

	grcb->usrptr = (pl[0] < sizeof( grcb->usrbuf)) ? grcb->usrbuf
			: (char *)MEreqmem( 0, pl[0] + 1, FALSE, NULL );

	if ( grcb->pwdptr  &&  grcb->pwdptr != grcb->pwdbuf )
	    MEfree( (PTR)grcb->pwdptr );

	grcb->pwdptr = (pl[1] < sizeof( grcb->pwdbuf)) ? grcb->pwdbuf
			: (char *)MEreqmem( 0, pl[1] + 1, FALSE, NULL );

	if ( ! grcb->usrptr || ! grcb->pwdptr )  return( E_GC0121_GCN_NOMEM );

	/*
	** Save username and password.
	*/
	if ( pl[0] )  MEcopy( (PTR)pv[0], pl[0], grcb->usrptr );
	if ( pl[1] )  MEcopy( (PTR)pv[1], pl[1], grcb->pwdptr );
	grcb->usrptr[ pl[0] ] = EOS;
	grcb->usr = grcb->usrptr;
	grcb->pwdptr[ pl[1] ] = EOS;
	grcb->pwd = grcb->pwdptr;

	/*
	** Check encryption version.
	*/
	if ( count > 2  &&  pv[2][0] == 'V'  &&  
	     pv[2][1] >= '0'  &&  pv[2][1] <= '9'  &&  pv[2][2] == EOS )
	    version = (pv[2][1] - '0');

	/*
	** Password is stored encrypted, decrypt it now.
	** Decrypted password will fit in space used by
	** the encrypted password.
	*/
	gcn_login( GCN_VLP_LOGIN, version, FALSE, 
		   grcb->usr, grcb->pwd, grcb->pwd );
	grcb->flags &= ~GCN_RSLV_PWD_ENC;

    }

    return( OK );
}


/*
** Name: gcn_rslv_attr() - lookup attribute info for vnode
**
** Description:
**	For remote connections, looks up the attribute information in
**	the GCN_ATTR queue. Get private entries first, look for matching
**	emode, emech tuples. If either of them not found or none of them 
**	found, get global entries, look for matching emode, emech tuples.
**
**	Users update the GCN_ATTR queue with netu or netutil.
**
** Inputs:
**	grcb	control block for resolve operation
**
** Returns:
**	STATUS
**
** History:
**	09-Oct-97 (rajus01)
**	    Created.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	31-Mar-98 (gordy)
**	    Added defines for known attributes.
**	15-May-98 (gordy)
**	    Added support for remote authentication.
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
**	27-Aug-98 (gordy)
**	    Save remote mechanism only if password not provided.
**	30-Sep-98 (gordy)
**	    Handle pre-resolved VNODE @<host>.
**	21-Jul-09 (gordy)
**	    Declare default sized temp buffers.  Use dynamic storage
**	    if lengths exceed default size.  Return memory allocation
**	    failure status.
*/

STATUS
gcn_rslv_attr( GCN_RESOLVE_CB *grcb )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		tupmask;
    GCN_TUP		*tupv[ GCN_SVR_MAX ];
    PTR			scan_cb;
    i4			tupc, i;
    STATUS		status = OK;

    /*
    ** Nothing to do for local requests or pre-resolved vnodes.
    */
    if ( ! *grcb->vnode  ||  *grcb->vnode == '@' )  return( OK );

    /* 
    ** remote request.
    */
    if ( ! (nq = gcn_nq_find( GCN_ATTR ))  ||  gcn_nq_lock( nq, FALSE ) != OK )
	return( E_GC0134_GCN_INT_NQ_ERROR );

    /*
    ** Get private entry
    */
    tupmask.uid = grcb->username;
    tupmask.obj = grcb->vnode;
    tupmask.val = "";
    scan_cb = NULL;

    tupc = gcn_nq_hscan( nq, &tupmask, &scan_cb, GCN_SVR_MAX, tupv, NULL );

    /*
    ** Copy results.
    */
    for( i = 0; i < tupc; i++ )
    {
	char	*pv[2];
	char	*tmp, tmpbuf[ 65 ];
	i4	len = STlength( tupv[ i ]->val ) + 1;

	tmp = (len <= sizeof( tmpbuf ))
	      ? tmpbuf : (char *)MEreqmem( 0, len, FALSE, NULL );

	if ( ! tmp )
	{
	    status = E_GC0121_GCN_NOMEM;
	    break;
	}

	pv[0] = pv[1] = "";
	gcu_words( tupv[ i ]->val, tmp, pv, ',', 2 );
        gcn_set_attr( grcb, pv );   
    	if ( tmp != tmpbuf )  MEfree( (PTR)tmp );
    }

    /* 
    ** get global entry
    */
    if ( status == OK )
    {
	tupmask.uid = GCN_GLOBAL_USER;
	scan_cb = NULL;
	tupc = gcn_nq_hscan( nq, &tupmask, &scan_cb, GCN_SVR_MAX, tupv, NULL );

	/*
	** Copy results
	*/
	for( i = 0; i < tupc; i++ )
	{
	    char	*pv[2];
	    char	*tmp, tmpbuf[ 65 ];
	    i4		len = STlength( tupv[ i ]->val ) + 1;

	    tmp = (len <= sizeof( tmpbuf ))
		  ? tmpbuf : (char *)MEreqmem( 0, len, FALSE, NULL );

	    if ( ! tmp )
	    {
		status = E_GC0121_GCN_NOMEM;
		break;
	    }

	    pv[0] = pv[1] = "";
	    gcu_words( tupv[ i ]->val, tmp, pv, ',', 2 );
	    gcn_set_attr( grcb, pv ); 
	    if ( tmp != tmpbuf )  MEfree( (PTR)tmp );
	}
    }

    gcn_nq_unlock( nq );
    return( status );
}

/*
** Name: gcn_set_attr
**
** Description:
**	assign attr value in the resolve control block if not already set
**
** Input
**	grcb		Resolve control block.
**      pv              parsed attr name and value 
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	13-jul-2001(wansh01) 
**	    Created.
*/

static VOID
gcn_set_attr( GCN_RESOLVE_CB *grcb, char *pv[2] )
{
   
    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_set_attr: attr name =%s, attr value=%s\n", 
		     pv[0], pv[1] ); 

    if (   (grcb->gca_message_type != GCN_NS_2_RESOLVE ) && 
           ( ! (grcb->flags & GCN_RSLV_RMECH) && 
           ! ( STcasecmp( pv[ 0 ], GCN_ATTR_AUTH_MECH ) ) ) )
	{
	if ( ! STcasecmp( pv[ 1 ], ERx("none") ) ) 
	    grcb->rmech[0] = EOS;
	else 
    	    STcopy( pv[ 1 ], grcb->rmech  );
	grcb->flags |= GCN_RSLV_RMECH;
	}


    if ( ! (grcb->flags & GCN_RSLV_EMODE) && 
	 ! ( STcasecmp( pv[ 0 ], GCN_ATTR_ENC_MODE ) ) )
	{
    	STcopy( pv[ 1 ], grcb->emode  );
	grcb->flags |= GCN_RSLV_EMODE; 
	}

    if ( ! (grcb->flags & GCN_RSLV_EMECH) && 
	 ! ( STcasecmp( pv[ 0 ], GCN_ATTR_ENC_MECH ) ) )
	{
    	STcopy( pv[ 1 ], grcb->emech );
	grcb->flags |= GCN_RSLV_EMECH;
	}

    if ( ! (grcb->flags & GCN_RSLV_CTYPE) && 
         ! STcasecmp( pv[ 0 ], GCN_ATTR_CONN_TYPE )  &&
	 ! STcasecmp( pv[ 1 ], GCN_ATTR_VAL_DIRECT ) )
	{
	grcb->flags |= GCN_RSLV_DIRECT;
	grcb->flags |= GCN_RSLV_CTYPE;
	}

    return;
}


/*{
** Name: gcn_rslv_server() - build local server list
**
** Description:
**	Using the server class given as part of the target name,
**	the the COMSVR server class for remote connections, builds
**	a randomised list of local servers to try to connect to.
**
** Inputs:
**	grcb	control block for resolve operation
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	15-Mar-93 (edg)
**	    Use gcn_rslv_sort() for local servers.
**	16-Mar-93 (edg)
**	    Increment no_assocs in cache of the first listen address returned
**	    in otupv.  This will address the case where one server may get
**	    overloaded by lots of connect requests occurring between
**	    bedchecks.  
**	27-Nov-95 (nick)
**	    Change the reduction to sole_server to be on category rather than
**	    the entire vector. #72834
**	28-Nov-95 (gordy)
**	    When configured for an embedded Comm Server, a registered
**	    Comm Server is not required to support out-bound connections.
**	    If there is no GCC for a remote connection, return an address
**	    of 'unknown' which will be ignored by the embedded Comm Server,
**	    satisfy those who require an address, and fail if it gets
**	    passed to the wrong functions (GCA CL).
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	 1-Sep-98 (gordy)
**	    Since all transient classes are now dynamic, return
**	    NO GCC error message if can't find COMSVR queue for
**	    remote requests.
**	30-Sep-98 (gordy)
**	    Handle pre-resolved VNODE @<host>.
**	21-Jul-09 (gordy)
**	    Reference registry address directly rather than copying.
**	    A single default sized local info buffer is provided.  
**	    Use dynamic storage if exceeds default size.  Reference
**	    existing storage where possible.
*/

STATUS
gcn_rslv_server( GCN_RESOLVE_CB *grcb )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		tupmask;
    bool		remote = (*grcb->vnode != EOS);
    GCN_TUP		tupv[ GCN_SVR_MAX ];
    char		*class;
    i4			i, tupc, tupgc;

    /*
    ** Check for pre-resolved vnode with no host specified.
    */
    if ( remote  &&  ! STcompare( grcb->vnode, ERx("@") ) )
    {
	/*
	** Force local connection.
	*/
	*grcb->vnode = EOS;
	remote = FALSE;

	/*
	** A target of @::/iinmsvr is interpreted as a local
	** connection request to the master Name Server.
	*/
	if ( ! STcasecmp( grcb->class, GCN_NMSVR ) )
	{
	    grcb->catl.tupc = 1;
	    grcb->catl.host[0] = "";
	    grcb->catl.addr[0] = IIGCn_static.registry_addr;
	    grcb->catl.lclptr[0] = NULL;
	    grcb->catl.auth[0] = NULL;
	    grcb->catl.auth_len[0] = 0;
	    return( OK );
	}
    }

    /*
    ** Pick the server class: for remote connections it is the COMSVR;
    ** if specified in the target name, use that; otherwise, use the
    ** default.
    */
    class = remote ? GCN_COMSVR 
		   : (*grcb->class ? grcb->class : IIGCn_static.svr_type);

    /*
    ** If this is pre-resolved target, just 
    ** return input string as server address.
    */
    if ( class[0] == '@' )
    {
	i4 len = STlength( &class[1] ) + 1;

	grcb->catl.lclptr[0] = (len <= sizeof( grcb->catl.lclbuf )) 
				? grcb->catl.lclbuf 
				: (char *)MEreqmem( 0, len, FALSE, NULL );
	STcopy( &class[1], grcb->catl.lclptr[0] );

	grcb->catl.tupc = 1;
	grcb->catl.host[0] = "";
	grcb->catl.addr[0] = grcb->catl.lclptr[0];
	grcb->catl.auth[0] = NULL;
	grcb->catl.auth_len[0] = 0;
	return( OK );
    }

    /* 
    ** Search for Object Server types and generate the vector
    ** (list) of GCA address.
    **
    ** If this is local request, search for all server id for this type.
    ** Two categories of id will be returned. The first category contains
    ** those ids which can specifically serve the named databases, the 
    ** second one contains those servers which can serve all databases. 
    ** The first category comes before the second in the return vector.
    **
    ** The server ids for each category are returned in ascending existing 
    ** connection count order.
    **
    ** If a sole server exists in either category, then that category set
    ** will be reduced to just this.
    */
    if ( ! (nq = gcn_nq_find( class )) )
	if ( remote )
	    if( ! (grcb->flags & GCN_RSLV_EMBEDGCC ) ) 
	        return( E_GC0137_GCN_NO_GCC );
	    else
	    {
	    	/*
	    	** A registered Comm Server is not required
	    	** when configured for embedded Comm Server.
	    	** Need to pass back an address to make every-
	    	** one happy, but make it an invalid one just
	    	** in case it gets passed to the GCA CL.  The
	    	** embedded Comm Server ignores the address.
	    	*/
	    	STcopy( ERx( "Unknown" ), grcb->catl.lclbuf );
		grcb->catl.lclptr[0] = grcb->catl.lclbuf;

	    	grcb->catl.tupc = 1;
	    	grcb->catl.host[0] = "";
	    	grcb->catl.addr[0] = grcb->catl.lclptr[0];
	    	grcb->catl.auth[0] = NULL;
	    	grcb->catl.auth_len[0] = 0;
	    	return( OK );
	    }
	else  if ( class == IIGCn_static.svr_type )
	    return( E_GC0135_GCN_BAD_SVR_TYPE );
	else
	    return( E_GC0136_GCN_SVRCLASS_UNKNOWN );

    if ( gcn_nq_lock( nq, FALSE ) != OK ) 
	return E_GC0134_GCN_INT_NQ_ERROR;

    /* 
    ** Get servers registered for vnode/database 
    ** and reduce to sole_server.
    */
    tupmask.uid = tupmask.val = "";
    tupmask.obj = remote ? grcb->vnode : grcb->dbname;
    tupc = gcn_rslv_sort( nq, &tupmask, GCN_SVR_MAX, tupv );

    for( i = 0; i < tupc; i++ )
    {
	u_i4	flag = 0;

	CVahxl( tupv[i].uid, &flag );

	if ( flag & GCN_SOL_FLAG )
	{
	    tupc = 1;
	    tupv[0].val = tupv[i].val;
	    break;
	}
    }

    /* 
    ** Get servers registered for any object 
    ** and reduce to sole_server.
    */
    tupmask.obj = "*";
    tupgc = gcn_rslv_sort( nq, &tupmask, GCN_SVR_MAX - tupc, tupv + tupc );

    for( i = tupc; i < (tupgc + tupc); i++ )
    {
	u_i4	flag = 0;

	CVahxl( tupv[i].uid, &flag );

	if ( flag & GCN_SOL_FLAG )
	{
	    tupgc = 1;
	    tupv[tupc].val = tupv[i].val;
	    break;
	}
    }

    /* 
    ** Copy results 
    */
    tupc += tupgc;
    grcb->catl.tupc = tupc;

    while( tupc-- )  
    {
	grcb->catl.host[ tupc ] = "";
	grcb->catl.addr[ tupc ] = tupv[ tupc ].val;
	grcb->catl.lclptr[ tupc ] = NULL;
	grcb->catl.auth[ tupc ] = NULL;
	grcb->catl.auth_len[ tupc ] = 0;
    }

    gcn_nq_unlock( nq );

    if ( ! grcb->catl.tupc )
	if ( remote )
	    if( ! (grcb->flags & GCN_RSLV_EMBEDGCC ) ) 
	        return( E_GC0137_GCN_NO_GCC );
	    else
	    {
	    	/*
	    	** A registered Comm Server is not required
	    	** when configured for embedded Comm Server.
	    	** Need to pass back an address to make every-
	    	** one happy, but make it an invalid one just
	    	** in case it gets passed to the GCA CL.  The
	    	** embedded Comm Server ignores the address.
	    	*/
	    	STcopy( ERx( "Unknown" ), grcb->catl.lclbuf );
		grcb->catl.lclptr[0] = grcb->catl.lclbuf;

	    	grcb->catl.tupc = 1;
	    	grcb->catl.host[0] = "";
	    	grcb->catl.addr[0] = grcb->catl.lclptr[0];
	    	grcb->catl.auth[0] = NULL;
	    	grcb->catl.auth_len[0] = 0;
	    	return( OK );
	    }
	else  if ( class == IIGCn_static.svr_type )
	    return( E_GC0139_GCN_NO_DBMS );
	else
	    return( E_GC0138_GCN_NO_SERVER );

    /*
    ** Update no_assocs of first tuple in list.
    */
    gcn_nq_assocs_upd( nq, tupv[0].val, tupv[0].no_assocs + 1 );

    return( OK );
}


/*{
** Name: gcn_rslv_err() - propagate error status
**
** Description:
**	Stashes an error status for gcn_rslv_done() to find.
**
** Inputs:
**	grcb	control block for resolve operation
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	 4-Dec-97 (rajus01)
**	   Free the memory allocated for remote authentication. 
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
*/

VOID
gcn_rslv_err( GCN_RESOLVE_CB *grcb, STATUS status )
{
    grcb->status = status;
    gcn_rslv_cleanup( grcb );

    return;
}


/*{
** Name: gcn_rslv_done() - build final GCN_RESOLVED message
**
** Description:
**	Uses information found in the resolve control block to
**	build the GCN_RESOLVE message.  Handles old protocol
**	clients by generating different messages/formats.
**
** Inputs:
**	grcb		control block for resolve operation
**	protocol	GCA_PROTOCOL of client
**	deleg_len	Length of delegated authentication (0 for none).
**	deleg		Delegated authentication (NULL if deleg_len is 0).
**
** Outputs:
**	mbout	GCN_RESOLVED message
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	29-May-97 (gordy)
**	    New RESOLVED message at PROTOCOL_LEVEL_63 to support
**	    GCS server auths.
**	17-Oct-97 (rajus01)
**	    The new resolved message now carries remote auth, emode,
**	    emech.
**	 4-Dec-97 (rajus01)
**	    Send remote auth, if length of remote auth is greater
**	    than 0 and free the memory allocated for remote auth.
**	12-Jan-98 (gordy)
**	    Added support for direct GCA network connections.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	 5-May-98 (gordy)
**	    Server auth takes client ID.
**	15-May-98 (gordy)
**	    Added support for remote authentication.
**	21-May-98 (gordy)
**	    Use default remote mechanism if not specified in vnode.
**	27-May-98 (gordy)
**	    Make sure there is some type of auth for remote connections.
**	27-Aug-98 (gordy)
**	    Generate remote auth if delegated auth token available.
**	 2-Oct-98 (gordy)
**	    Restore quoting of database name before passing to server.
**	15-Jul-04 (gordy)
**	    Resolved password may be encrypted or left as plain text.
**	    Encrypt for COMSVR (now enhanced) if needed.
**	24-Oct-05 (gordy)
**	    Password may expand to twice the allowed length during
**	    encryption.  Ensure output buffer is large enough.
**      04-Apr-08 (rajus01) SD issue 126704, Bug 120136
**          JDBC applications report E_GC1010 error when either password
**          or username specified in the JDBC URL is longer but less than the
**          supported limit (GC_L_PASSWORD, GC_L_USER_ID). Also when user
**          and/or password length exceed the supported lengths
**          GCN crashes and E_CLFE07 is returned to the application.
**          Provide bigger buffer to hold the password authentication token.
**      08-Apr-2008 (rajus01) SD issue 126704 Bug 120317
**	    The fix to resolve the SEGV error in JDBC server revealed another
**	    buffer overrun in GCN from gcn_unparse() resulting in SEGV.
**	21-Jul-09 (gordy)
**	    Remove password length restrictions.  Removed delgated
**	    authentication from grcb and added as parameters.  Declare
**	    default sized buffers.  Use dynamic storage if length
**	    exceeds default size.
**	27-Aug-10 (gordy)
**	    Added symbols for encoding versions.
*/

STATUS
gcn_rslv_done
( 
    GCN_RESOLVE_CB	*grcb, 
    GCN_MBUF		*mbout, 
    i4			protocol,
    i4			deleg_len,
    PTR			deleg
)
{
    GCN_MBUF 	*mbuf;
    char	*msg_out, *cnt_ptr;
    char	pbuff[ 64 ];
    char	*partner;
    char	pwdbuf[ 128 ];
    char	*pwd;
    char	*empty = "";
    STATUS	status;
    i4		i, len, count;

    /*
    ** Check to make sure we have some type
    ** of remote authentication for the client.
    */
    if ( grcb->status == OK  &&  grcb->catr.tupc )
    {
	if ( ! *grcb->pwd  &&  
	     ( protocol < GCA_PROTOCOL_LEVEL_63  ||
	       (grcb->auth_len <= 0  &&  ! grcb->rmech[0]) ) )
	    if ( grcb->vnode == IIGCn_static.rmt_vnode )
		grcb->status = E_GC0131_GCN_BAD_RMT_VNODE;
	    else
		grcb->status = E_GC0133_GCN_LOGIN_UNKNOWN;
    }

    if ( IIGCn_static.trace >= 3  ||  ! grcb->catl.tupc )
    {
	TRdisplay( "gcn_resolve: status %x local=%d remote=%d prot %d\n",
		    grcb->status, grcb->catl.tupc, grcb->catr.tupc, 
		    protocol );
    }

    /*
    ** Handle error.
    */
    if ( grcb->status != OK )
    {
	gcn_rslv_cleanup( grcb );

	/*
	** Send back GCA_ERROR if client can take it.
	*/
	if ( protocol >= GCA_PROTOCOL_LEVEL_40 )
	{
	    gcn_error( grcb->status, (CL_ERR_DESC *)0, (char *)0, mbout );
	    return( OK );
	}
    }

    /*
    ** Set the partner_id that is to be used for this connection. 
    ** Note that this is used in preparing the AUX_DATA partner_id 
    ** component.  The partner_id is the concatenated string 
    ** "dbname/svr_class".
    */
    if ( grcb->flags & GCN_RSLV_QUOTE )  gcn_quote( &grcb->dbname );

    /* 
    ** Don't call gcn_unparse() if the buffer provided to build partner_id
    ** is not enough and return E_GC0172 error.
    */
    len = STlength( grcb->dbname ) + STlength( grcb->class ) + 10;
    partner = (len <= sizeof( pbuff ))
    	      ? pbuff : (char *)MEreqmem( 0, len, FALSE, NULL );
    if ( ! partner )  return( E_GC0121_GCN_NOMEM );

    gcn_unparse( "", grcb->dbname, grcb->class, partner );

    /*
    ** Produce the GCN_RESOLVED message.
    */
    mbuf = gcn_add_mbuf( mbout, TRUE, &status );
    if ( status != OK )  
    {
	if ( partner != pbuff )  MEfree( (PTR)partner );
	gcn_rslv_cleanup( grcb );
	return( status );
    }

    msg_out = mbuf->data;

    /*
    ** Password must be encrypted for COMSVR.
    */
    if ( ! *grcb->usr  ||  ! *grcb->pwd )
	pwd = NULL;
    else  if ( grcb->flags & GCN_RSLV_PWD_ENC )
	pwd = grcb->pwd;
    else
    {
	i4 len = (STlength( grcb->pwd ) + 8) * 2;

	pwd = (len < sizeof( pwdbuf )) 
	      ? pwdbuf : (char *)MEreqmem( 0, len + 1, FALSE, NULL );

	if ( ! pwd )  
	{
	    if ( partner != pbuff )  MEfree( (PTR)partner );
	    return( E_GC0121_GCN_NOMEM );
	}

	gcn_login(GCN_VLP_COMSVR, GCN_VLP_V1, TRUE, grcb->usr, grcb->pwd, pwd);
    }

    if ( protocol < GCA_PROTOCOL_LEVEL_63 )
    {
	i4 tupc;

	/* 
	** Remote address catalogs.  Create one empty
	** address when there are no remote addresses.
	*/
	if ( protocol < GCA_PROTOCOL_LEVEL_40 )
	    tupc = min( grcb->catr.tupc, 1 );
	else
	{
	    tupc = max( grcb->catr.tupc, 1 );
	    msg_out += gcu_put_int( msg_out, tupc );
	}

	for( i = 0; i < max( tupc, 1 ); i++ )
	{
	    if ( i < tupc )
	    {
		msg_out += gcu_put_str( msg_out, grcb->catr.node[i] );
		msg_out += gcu_put_str( msg_out, grcb->catr.protocol[i] );
		msg_out += gcu_put_str( msg_out, grcb->catr.port[i] );
	    }
	    else
	    {
		msg_out += gcu_put_str( msg_out, empty );	/* phys node */
		msg_out += gcu_put_str( msg_out, empty );	/* protocol */
		msg_out += gcu_put_str( msg_out, empty );	/* port */
	    }
	}

	/*
	** User info.
	*/
	msg_out += gcu_put_str( msg_out, grcb->usr );
	msg_out += gcu_put_str( msg_out, pwd ? pwd : empty );
	msg_out += gcu_put_str( msg_out, partner ); 
	msg_out += gcu_put_str( msg_out, grcb->class );

	/*
	** Local address catalogs.
	*/
	msg_out += gcu_put_int( msg_out, grcb->catl.tupc );

	for( i = 0; i < grcb->catl.tupc; i++ )
	    msg_out += gcu_put_str( msg_out, grcb->catl.addr[ i ] );

	mbuf->used = msg_out - mbuf->data;
	mbuf->type = GCN_RESOLVED;
    }
    else
    {
	char *user;

	/*
	** Local user info.  When connecting remotely
	** use the local user from the listen.  When 
	** connecting locally, use the resolved user ID.
	*/
	user =  grcb->catr.tupc ? grcb->username : grcb->usr;
	msg_out += gcu_put_str( msg_out, user );

	/*
	** Local address catalogs.
	*/
	msg_out += gcu_put_int( msg_out, grcb->catl.tupc );

	for( i = 0; i < grcb->catl.tupc; i++ )
	{
	    msg_out += gcu_put_str( msg_out, grcb->catl.host[ i ] );
	    msg_out += gcu_put_str( msg_out, grcb->catl.addr[ i ] );

	    if ( grcb->catl.auth_len[ i ] )
	    {
		/*
		** Use authentication already provided.
		*/
		msg_out += gcu_put_int( msg_out, grcb->catl.auth_len[ i ] );
		MEcopy( grcb->catl.auth[ i ], 
			grcb->catl.auth_len[ i ], (PTR)msg_out );
		msg_out += grcb->catl.auth_len[ i ];
	    }
	    else
	    {
		/*
		** Generate server authentication.
		*/
		char	*ptr = msg_out;
		i4	length;

		msg_out += gcu_put_int( msg_out, 0 );
		length = gcn_server_auth( grcb->catl.addr[ i ], 
					  grcb->account, user,
					  mbuf->size - (msg_out - mbuf->data), 
					  msg_out );
		gcu_put_int( ptr, length );
		msg_out += length;
	    }
	}

	/*
	** Remote info.
	*/
	if ( ! grcb->catr.tupc )
	{
	    msg_out += gcu_put_int( msg_out, 0 );	/* remote addr count */
	    msg_out += gcu_put_int( msg_out, 0 );	/* remote data count */
	}
	else
	{
	    /* 
	    ** Remote address catalogs.
	    */
	    msg_out += gcu_put_int( msg_out, grcb->catr.tupc );
		
	    for( i = 0; i < grcb->catr.tupc; i++ )
	    {
		msg_out += gcu_put_str( msg_out, grcb->catr.node[i] );
		msg_out += gcu_put_str( msg_out, grcb->catr.protocol[i] );
		msg_out += gcu_put_str( msg_out, grcb->catr.port[i] );
	    }

	    cnt_ptr = msg_out;
	    count = 0;
	    msg_out += gcu_put_int( msg_out, count );	/* place holder */

	    if ( *partner )
	    {
		msg_out += gcu_put_int( msg_out, GCN_RMT_DB );
		msg_out += gcu_put_str( msg_out, partner ); 
		count++;
	    }

	    /*
	    ** Provide authentication info with the following precedence:
	    **
	    ** 1. Existing remote authentication (installation password).
	    ** 2. Generate remote auth (delegated auth) for client.
	    ** 3. Remote auth mechanism and/or password.
	    */
	    if ( grcb->auth_len > 0 )
	    {
		msg_out += gcu_put_int( msg_out, GCN_RMT_AUTH );
		msg_out += gcu_put_int( msg_out, grcb->auth_len );
		MEcopy( grcb->auth, grcb->auth_len, msg_out );
		msg_out += grcb->auth_len;
		count++;
	    }
	    else  if ( grcb->rmech[0]  &&  deleg_len > 0 )
	    {
		char	*ptr = msg_out;
		char	*len_ptr;
		i4	len;

		/* Build header (temporary until remote auth succeeds)  */
		ptr += gcu_put_int( ptr, GCN_RMT_AUTH );
		len_ptr = ptr;
		ptr += gcu_put_int( ptr, 0 );
		len = mbuf->size - (ptr - mbuf->data);

		if ( gcn_rem_auth( grcb->rmech, grcb->catr.node[0], 
				   deleg_len, deleg, &len, ptr ) == OK )
		{
		    gcu_put_int( len_ptr, len );
		    msg_out = ptr + len;
		    grcb->usr = grcb->username;	  /* Authenticated client */
		    count++;
		}
		else
		{
		    msg_out += gcu_put_int( msg_out, GCN_RMT_MECH );
		    msg_out += gcu_put_str( msg_out, grcb->rmech );
		    count++;

		    if ( pwd  &&  *pwd )
		    {
			msg_out += gcu_put_int( msg_out, GCN_RMT_PWD );
			msg_out += gcu_put_str( msg_out, pwd );
			count++;
		    }
		}
	    }
	    else  
	    {
		if ( grcb->rmech[0] )
		{
		    msg_out += gcu_put_int( msg_out, GCN_RMT_MECH );
		    msg_out += gcu_put_str( msg_out, grcb->rmech );
		    count++;
		}

		if ( pwd  &&  *pwd )
		{
		    msg_out += gcu_put_int( msg_out, GCN_RMT_PWD );
		    msg_out += gcu_put_str( msg_out, pwd );
		    count++;
		}
	    }

	    if ( *grcb->usr )
	    {
		msg_out += gcu_put_int( msg_out, GCN_RMT_USR );
		msg_out += gcu_put_str( msg_out, grcb->usr );
		count++;
	    }

	    if ( *grcb->emech )
	    {
		msg_out += gcu_put_int( msg_out, GCN_RMT_EMECH );
		msg_out += gcu_put_str( msg_out, grcb->emech );
		count++;
	    }

	    if ( *grcb->emode )
	    {
		msg_out += gcu_put_int( msg_out, GCN_RMT_EMODE );
		msg_out += gcu_put_str( msg_out, grcb->emode );
		count++;
	    }

	    if ( count )  gcu_put_int( cnt_ptr, count );
	}

	mbuf->used = msg_out - mbuf->data;
	mbuf->type = GCN2_RESOLVED;
    }

    if ( partner != pbuff )  MEfree( (PTR)partner );
    if ( pwd  &&  pwd != grcb->pwd  &&  pwd != pwdbuf )  MEfree( (PTR)pwd );
    gcn_rslv_cleanup( grcb );
    return( OK );
}


/*
** Name: gcn_rslv_cleanup
**
** Description:
**	Release dynamically allocated resources in the
**	resolve control block.
**
** Input:
**	grcb		Resolve control block.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	12-Jan-98 (gordy)
**	    Created.
**      19-Jul-2001 (wansh01) 
**          Cleanup grcb->target. 
**	21-Jul-09 (gordy)
**	    Free additional dynamic resources.
*/

VOID
gcn_rslv_cleanup( GCN_RESOLVE_CB *grcb )
{
    i4  i;

    grcb->flags = 0;

    grcb->vnode = "";
    grcb->dbname = "";
    grcb->class = "";

    if ( grcb->target )
    {
	MEfree( (PTR)grcb->target );
	grcb->target = NULL;
    }

    grcb->usr = "";
    grcb->pwd = "";

    if ( grcb->usrptr  &&  grcb->usrptr != grcb->usrbuf )
    {
        MEfree( (PTR)grcb->usrptr );
    	grcb->usrptr = NULL;
    }

    if ( grcb->pwdptr  &&  grcb->pwdptr != grcb->pwdbuf )
    {
        MEfree( (PTR)grcb->pwdptr );
    	grcb->pwdptr = NULL;
    }

    if ( grcb->ip )
    {
    	MEfree( (PTR)grcb->ip );
	grcb->ip = NULL;
    }

    if ( grcb->auth_len )
    {
	MEfree( grcb->auth );
	grcb->auth = NULL;
	grcb->auth_len = 0;
    }

    for( i = 0; i < grcb->catl.tupc; i++ )
    {
	grcb->catl.host[ i ] = "";
	grcb->catl.addr[ i ] = "";

	if ( grcb->catl.lclptr[ i ]  &&
	     grcb->catl.lclptr[ i ] != grcb->catl.lclbuf )
	{
	    MEfree( (PTR)grcb->catl.lclptr[ i ] );
	    grcb->catl.lclptr[ i ] = NULL;
	}

	if ( grcb->catl.auth_len[ i ] )
	{
	    MEfree( grcb->catl.auth[ i ] );
	    grcb->catl.auth[ i ] = NULL;
	    grcb->catl.auth_len[ i ] = 0;
	}
    }

    for( i = 0; i < grcb->catr.tupc; i++ )
    {
	grcb->catr.protocol[ i ] = "";
	grcb->catr.node[ i ] = "";
	grcb->catr.port[ i ] = "";

    	if ( grcb->catr.rmtptr[ i ]  &&
	     grcb->catr.rmtptr[ i ] != grcb->catr.rmtbuf )
	{
	    MEfree( (PTR)grcb->catr.rmtptr[ i ] );
	    grcb->catr.rmtptr[ i ] = NULL;
	}
    }

    grcb->catl.tupc = 0;
    grcb->catr.tupc = 0;

    return;
}


/*{
** Name: gcn_rslv_rand() - randomize a bunch of tuples
**
** Description:
**	Retrieves tuples from a queue and randomizes them.
**
** Inputs:
**	nq	name queue for the tuples
**	tupmask	pattern for selecting tuples
**	tupmax	max tuples to return
**
** Outputs:
**	otupv	resulting tuples
**
** Returns:
**	tuples in otupv
**
** History:
**	29-Mar-92 (seiwald)
**	    Split from old gcn_resolve().
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
*/

static i4
gcn_rslv_rand(GCN_QUE_NAME *nq, GCN_TUP *tupmask, i4  tupmax, GCN_TUP **otupv)
{
    GCN_TUP	*tupv[ GCN_SVR_MAX ];
    PTR		scan_cb = NULL;
    i4		tupc;
    i4		i;

    i = tupc = gcn_nq_hscan( nq, tupmask, &scan_cb, tupmax, tupv, NULL );

    while( i-- )
    {
	i4 n = (i4)( MHrand() * (f8)( i + 1 ) );

	*(otupv++) = tupv[ n ];
	tupv[ n ] = tupv[ i ];
    }

    return( tupc );
}


/*{
** Name: gcn_rslv_sort() - sort a bunch of tuples
**
** Description:
**	Retrieves tuples from a queue and returns them sorted based on
**	no_assocs from lowest to highest
**
** Inputs:
**	nq	name queue for the tuples
**	tupmask	pattern for selecting tuples
**	tupmax	max tuples to return
**
** Outputs:
**	otupv	resulting tuples
**
** Returns:
**	tuples in otupv
**
** History:
**	15-Mar-93 (edg)
**	    Written.
*/

static i4
gcn_rslv_sort( GCN_QUE_NAME *nq, GCN_TUP *tupmask, i4  tupmax, GCN_TUP *otupv )
{
    GCN_TUP	tupv[ GCN_SVR_MAX ];
    i4		tupc;
    i4		i;
    i4		last_used;

    tupc = gcn_nq_ret( nq, tupmask, 0, tupmax, tupv );

    while( tupc )
    {
	last_used = -1;
	otupv->no_assocs = -1;

	/*
	** Put the last lowest tuple (no_assocs) from tupv to otupv.
	** unless otupv->no_assocs == -1, in which case prime it with
	** the first unused tupv tuple.
	*/
	for( i = 0; i < tupc; i++ )
	{
	    if ( ( tupv[i].no_assocs <= otupv->no_assocs 
		 && tupv[i].no_assocs >= 0 ) ||
		 ( tupv[i].no_assocs >= 0 && otupv->no_assocs == -1 ) )
	    {
		STRUCT_ASSIGN_MACRO( tupv[ i ], *otupv );
		last_used = i;
	    }
	}

	if ( last_used >= 0 )
	{
	    /*
	    ** Mark the last tup assigned to otupv with -1 so that we
	    ** don't reuse it.  Bump otupv pointer and continue.
	    */
	    tupv[last_used].no_assocs = -1;
	    otupv++;
	    continue;
	}

	/*
	** If we're here we've assigned all addresses to otupv.
	*/
	break;
    }

    return( tupc );
}

/*
** Name: gcn_attr_override
**
** Description:
**	Looks to see if a vnode component contains 
**	attr;attrs in the form
**
**		[name=value]
**
**	If present, the attrs are removed 
**	and the resolve control block is updated with the
**	name and value as if the RESOLVE request had
**	contained the information (info from original RESOLVE
**	message is overwritten).
**
** Input
**	grcb		Resolve control block.
**	vnode		Virtual node definition.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	11-jul-2001(wansh01) 
**	    Created.
*/

static VOID
gcn_attr_override( GCN_RESOLVE_CB *grcb, char *vnode )
{
    char	*pv[ 2 ],*pv2[5];
    char        *ptr;
    i4		i, count;

    /*
    ** more than one attr can be specified and attrs are separated by ';'. 
    ** for each attribute, it must contain a single '=' to separate
    ** the name and value. 
    */
    /* looking for attr. if not there then return     
    */
    if  (ptr = STchr(vnode, ';'))  
       *ptr = EOS;
    else 
       return; 


    count = gcu_words( ptr +1, NULL, pv2, ';', 5 );
    for (i = 0; i < count; i++)    
    {
	 pv[0] = pv[1] = "";
	 gcu_words( pv2[i], NULL, pv, '=', 2 );
	 gcn_set_attr(grcb, pv);
    }


    return;
}


/*
** Name: gcn_pwd_override
**
** Description:
**	Looks to see if a vnode component contains an explicit
**	username/password pair in the form
**
**		[user,pwd]
**
**	If present, the pair is removed (including brackets)
**	and the resolve control block is updated with the
**	username and password as if the RESOLVE request had
**	contained the information (info from original RESOLVE
**	message is overwritten).
**
** Input
**	grcb		Resolve control block.
**	vnode		Virtual node definition.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 2-Oct-98 (gordy)
**	    Created.
**      16-Jul-2001 (wansh01) 
**          set grcb->rmech to EOS if the message type is changed to
**          GCN_NS_2_RESOLVE
**	21-Jul-09 (gordy)
**	    Return status.  Default sized username and password
**	    buffers are declared in grcb.  Use dynamic storage if
**	    actual lengths exceed default size.
*/

static STATUS
gcn_pwd_override( GCN_RESOLVE_CB *grcb, char *vnode )
{
    char	*bptr, *eptr;
    char	*pv[ 3 ];
    char	buff[ 256 ];
    char	*ptr = buff;
    i4		len, count;
    STATUS	status = OK;

    /*
    ** Only the final pair of brackets is tested
    ** and it must contain a single ',' separated
    ** pair of words.
    */
    if ( (bptr = STrchr( vnode, '[' ))  && 
	 (eptr = STchr( bptr, ']' ))  &&  *(eptr + 1) == EOS )
    {
	*eptr = EOS;
	len = STlength( bptr + 1 ) + 1;

	if ( len > sizeof( buff )  &&
	     ! (ptr = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	    return( E_GC0121_GCN_NOMEM );

	count = gcu_words( bptr + 1, ptr, pv, ',', 3 );

	if ( count != 2 )
	    *eptr = ']';	/* Restore original string */
	else
	{
	    /*
	    ** Clear out any prior GCN_NS_2_RESOLVE message state
	    ** and force control block back to GCN_NS_RESOLVE state.
	    */
	    if ( grcb->usrptr  &&  grcb->usrptr != grcb->usrbuf )
	        MEfree( (PTR)grcb->usrptr );

	    if ( grcb->pwdptr  &&  grcb->pwdptr != grcb->pwdbuf )
	        MEfree( (PTR)grcb->pwdptr );

	    grcb->usrptr = grcb->pwdptr = NULL;
	    grcb->gca_message_type = GCN_NS_RESOLVE;

	    /*
	    ** Extract user ID and password from vnode string.
	    ** If both provided, fake GCN_NS_2_RESOLVE state.
	    */
	    *bptr = EOS;
	    len = STlength( pv[0] ) + 1;
	    grcb->usrptr = (len <= sizeof( grcb->usrbuf )) ? grcb->usrbuf
	    		   : (char *)MEreqmem( 0, len, FALSE, NULL );

	    if ( ! grcb->usrptr )
	    {
	        status = E_GC0121_GCN_NOMEM;
		goto done;
	    }

	    STcopy( pv[0], grcb->usrptr );

	    len = STlength( pv[1] ) + 1;
	    grcb->pwdptr = (len <= sizeof( grcb->pwdbuf )) ? grcb->pwdbuf
	    		   : (char *)MEreqmem( 0, len, FALSE, NULL );

	    if ( ! grcb->pwdptr )  
	    {
	        status = E_GC0121_GCN_NOMEM;
	    	goto done;
	    }

	    STcopy( pv[1], grcb->pwdptr );

	    if ( *grcb->usrptr  &&  *grcb->pwdptr )
	    {
		grcb->gca_message_type = GCN_NS_2_RESOLVE;
		grcb->rmech[ 0 ] = EOS; 
            }	
	}
    }

  done :

    if ( ptr != buff )  MEfree( (PTR)ptr );
    return( status );
}


/*{
** Name: gcn_parse() - parse vnode::dname/class
**
** Description:
**	Parses the target name the user typed on the command line.
**
** Inputs:
**	in	the target name
**
** Outputs:
**	vnode	pointer to vnode component, or NULL
**	dbname	pointer to dbname component
**	class	pointer to class component, or NULL
**
** Side effects:
**	Puts null bytes into the 'in' buffer.
**
** Returns:
**	STATUS
**
** History:
**	29-Mar-92 (seiwald)
**	    Documented.
**	 2-Oct-98 (gordy)
**	    Allow single quoted strings.
**      19-Jul-2001 (wansh01) 
**          Separate the loop to two do while loops. 
**	27-aug-2001 (somsa01)
**	    Before starting the second 'do...while' loop, reinitialize
**	    'in' to 'dbname' if we didn't find a vnode.
*/

static STATUS
gcn_parse( char *in, char **vnode, char **dbname, char **class )
{
    char	c, *quote;
    static char null[] = "";

    *vnode = *class = null;
    *dbname = in;

    do 
    {
	switch( c = *in++ )
	{
	case '\'':
	case '"':
	    /* 
	    ** Copy literally + inclusively the quoted string. 
	    */
	    quote = in;
	    do { c = *in++; } while( c  &&  c != *quote );
	    if ( ! c ) return( FAIL );
	    break;

	case ':':
	    /*
	    ** Terminate vnode and start dbname with '::'. 
	    */
	    if ( *in++ == ':' )  
	    {
		in[-2] = EOS;
		*vnode = *dbname;
		*dbname = in;
	    }
	    break;
	}
    } while( c  &&  *vnode == null );

    if ( *vnode == null )  in = *dbname;

    do 
    {
	switch( c = *in++ )
	{
	case '\'':
	case '"':
	    /* 
	    ** Copy literally + inclusively the quoted string. 
	    */
	    quote = in;
	    do { c = *in++; } while( c  &&  c != *quote );
	    if ( ! c ) return( FAIL );
	    break;

	case '/':
	    /*
	    ** Terminate dbname and start class.  Only one / allowed.
	    */
	    in[ -1 ] = EOS;
	    *class = in;
	    break;
	}
    } while(  c  &&  *class == null );

    return( OK );
}


/*{
** Name: gcn_unparse() - format vnode::dname/class
**
** Description:
**	Formats vnode, dbname, and class into a target string
**	Vnode and class are quoted if they contain delimiters.
**	DBname must be handled separately since its semantics
**	are not controlled by the Name Server.
**
** Inputs:
**	vnode		Vnode component or zero-length string.
**	dbname		DBname component or zero-length string.
**	class		Class component or zero-length string.
**
** Outputs:
**	out	the target name
**
** History:
**	29-Mar-92 (seiwald)
**	    Documented.
**	 2-Oct-98 (gordy)
**	    Quote vnode or class if they contain delimiters.
*/

VOID
gcn_unparse( char *vnode, char *dbname, char *class, char *out )
{
    char	*ptr;
    bool	quote;

    /* 
    ** Set the partner_id that is to be used for this connection.
    ** The partner_id is the concatenated string vnode::dbname/class.
    */
    *out = EOS;
    
    if ( *vnode )
    {
	for( ptr = vnode, quote = FALSE; *ptr; ptr++ )
	    if ( *ptr == ':'  ||  *ptr == '/' )
	    {
		quote = TRUE;
		break;
	    }

	if ( quote )  STcat( out, "\"" );
	STcat( out, vnode );
	if ( quote )  STcat( out, "\"" );
	STcat( out, "::" );
    }

    STcat( out, dbname );

    if ( *class )
    {
	for( ptr = class, quote = FALSE; *ptr; ptr++ )
	    if ( *ptr == ':'  ||  *ptr == '/' )
	    {
		quote = TRUE;
		break;
	    }

	STcat( out, "/" );
	if ( quote )  STcat( out, "\"" );
	STcat( out, class );
	if ( quote )  STcat( out, "\"" );
    }

    return;
}


/*
** Name: gcn_unquote
**
** Description:
**	Removes quotes surrounding a string (if present).  
**	The string is modified in place: the pointer is 
**	bumped forward past starting quote and the ending 
**	quote is replaced by EOS.  String may be restored 
**	using gcn_quote() on the original buffer.
**
** Input:
**	str		String.
**
** Output:
**	str		Unquoted string.
**
** Returns:
**	bool		TRUE if string was quoted.
**
** History:
**	 2-Oct-98 (gordy)
**	    Created.
*/

bool
gcn_unquote( char **str )
{
    char	*ptr = *str;
    i4		len = STlength( ptr );
    bool	quoted = FALSE;

    /*
    ** Make sure we have a quoted string.
    */
    if ( (ptr[ 0 ] == '"'  ||  ptr[ 0 ] == '\'')  &&  
	 len > 1  &&  ptr[ len - 1 ] == ptr[ 0 ] )
    {
	ptr[ len - 1 ] = EOS;		/* Remove ending quote */
	(*str)++;			/* Bump past starting quote */
	quoted = TRUE;
    }

    return( quoted );
}

/*
** Name: gcn_quote
**
** Description:
**	Restores a string which was unquoted by gcn_unquote().
**	Original buffer passed to gcn_unquote must be maintained.
**	String pointer is decremented back to starting quote and
**	matching quote is placed at end of string.
**
** Input:
**	str		String
**
** Output:
**	str		Quoted string.
**
** Returns:
**	VOID.
**
** History:
**	 2-Oct-98 (gordy)
**	    Created.
*/

VOID
gcn_quote( char **str )
{
    char	*ptr = *str - 1;
    i4		len = STlength( ptr );

    if ( ptr[ 0 ] == '"'  ||  ptr[ 0 ] == '\''  )
    {
	ptr[ len ] = ptr[ 0 ];		/* Restore ending quote */
	ptr[ len + 1 ] = EOS;
	(*str)--;			/* Reset to starting quote */
    }

    return;
}

