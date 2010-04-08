/*
** Copyright (c) 1987, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <cv.h>
#include <gc.h>
#include <me.h>
#include <mu.h>
#include <qu.h>
#include <si.h>
#include <sl.h>
#include <st.h>
#include <tm.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcu.h>
#include <gcm.h>
#include <gcmint.h>
#include <gcn.h>
#include <gcnint.h>
#include <gccer.h>

/**
**
**  Name: gcngca.c
**
**  Description:
**	Contains the routines which provide the interface to call GCA functions.
**	The following routines are defined.
**
**	    gcn_init		Initialize GCA
**	    gcn_term		Terminate GCA
**	    gcn_request		Connect to local/remote Name Server.
**	    gcn_send		Send GCA message.
**	    gcn_receive		Receive GCA message.
**	    gcn_release		Release a session.
**	    gcn_fastselect	Server Request (used for shutdown).
**	    gcn_testaddr	Test target address.
**	    gcn_bedcheck	Check if Name Server running.
**
**  History:    $Log-for RCS$
**	    
**      08-Sep-87   (lin)
**          Initial creation.
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.  Message formats between
**	    IIGCN and its clients remain the same, but the messages are
**	    constructed and extracted via properly aligning code.
**	    Variable length structures eliminated.  Strings are now
**	    null-terminated (they had to before, but no one admitted so).
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	7/16/90 (jorge/jimG)
**	    changed sprintf() to STprintf() in gcn_testaddr().
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	10-Sep-91 (seiwald) bug #38073
**	    New GCN_RCV_TIMEOUT waiting for connection to and responses
**	    from the Name Server.  This prevents iinamu from hanging when
**	    connecting to a bogus Name Server.
**	21-Nov-91 (seiwald)
**	    Mucked with gcn_testaddr().
**	2-June-92 (seiwald)
**	    Added gcn_host to support connecting to a remote Name Server.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	04-Oct-93 (joplin)
**	    Released assocations with back levels of gca_peer_protocol in
**	    gcn_request(). Added username parameter to gcn_testaddr so 
**	    NETUTIL could test connections on behelf of other people.
**	11-Nov_94 (Leati)
**	    Modified In_l_comm_buf init from 1024 to 992 - fixes bug 62668.
**	    This prevents a VNODE row from spanning 2 consecutive buffers
**	    as returned by two consecutive GCA_RECEIVE requests. Code in
**	    NETU and NETUTIL d/n know how to handle a fragmented row.
**	17-Nov-94 (sweeney)
**	    The last change to this file introduced a massive spurious
**	    diff against earlier revisions. Reintegrate Leati's change
**	    such that p rcompare behaves sanely against older versions.
**	21-Mar-95 (reijo01)
**	    Backed off previous two changes because the changes did not
**	    correct the problem.
**	 4-Dec-95 (gordy)
**	    Added prototypes.
**	 8-Dec-95 (gordy)
**	    Moved gcn_bedcheck() here from gcnoper.c.
**	 3-Apr-96 (gordy)
**	    Fixed parameters to IIGCa_call().
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.  Added gcn_init() and
**	    gcn_term().  Moved communication buffers to gcninit.c.
**	10-Dec-96 (gordy)
**	    gca_call() time-outs now in milli-seconds.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	19-Mar-97 (gordy)
**	    Removed global communications buffers.  Use API LEVEL 5.
**	    Name Server protocol violations fixed at level 63.  Added
**	    support for formatted interface.
**	27-May-97 (thoda04)
**	    Included cv.h, gcmint.h for function prototypes.  WIN16 fixes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-2004 (somsa01)
**	    In gcn_bedcheck(), corrected initialization of gca_partner_name.
**	20-Aug-09 (gordy)
**	    Remove string length restrictions.
**/

# define GCN_RCV_TIMEOUT	(120 * (i4)1000)	/* that's two minutes */

/*
** We use our own GCA control block so as
** to not interfere with anyone else.
*/
static PTR	gcn_gca_cb = NULL;


/*
** name: gcn_init - Initialize GCA
**
** Description:
**	Initialize GCA and format a communications buffer.
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
**	 3-Sep-96 (gordy)
**	    Created.
**	19-Mar-97 (gordy)
**	    Removed communication buffers.  Use API LEVEL 5.
*/

STATUS
gcn_init( VOID )
{
    GCA_IN_PARMS	init;
    STATUS		status;

    MEfill( sizeof( init ), '\0', (PTR)&init );
    init.gca_modifiers = GCA_API_VERSION_SPECD;
    init.gca_api_version = GCA_API_LEVEL_5;
    init.gca_local_protocol = GCA_PROTOCOL_LEVEL_63;

    gca_call( &gcn_gca_cb, GCA_INITIATE, (GCA_PARMLIST *)&init, 
	      GCA_SYNC_FLAG, NULL, -1, &status );

    gcn_checkerr( "GCA_INITIATE",
		  &status, init.gca_status, &init.gca_os_status );

    return( status );
}


/*
** Name: gcn_term - Terminate GCA
**
** Description:
**	Shutdown GCA.
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
**	 3-Sep-96 (gordy)
**	    Created.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

STATUS
gcn_term( VOID )
{
    GCA_TE_PARMS	term;
    STATUS		status;

    /*
    ** Make GCA_TERMINATE call.
    */
    MEfill( sizeof( term ), 0, (PTR)&term );

    gca_call( &gcn_gca_cb, GCA_TERMINATE, (GCA_PARMLIST *)&term, 
	      GCA_SYNC_FLAG, NULL, -1, &status );

    gcn_checkerr( "GCA_TERMINATE",
		  &status, term.gca_status, &term.gca_os_status);

    return( status );
}


/*{
** Name: gcn_request - request connection to the name server.
**
** Description:
**	This routine is called by the Name Server Management Utility to set
**	connection with the Name Server. The external name specified in the
**	GCA_REQUEST is "/IINMSVR", which is the Name Server's service class.
**
** Inputs:
**	gcn_host	Host machine (vnode) for Name Server
**
** Outputs:
**	assoc_id	Association identifier.
**			
** Returns:
**	STATUS
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      04-Oct-93 (joplin)
**          Released assocations with back levels of gca_peer_protocol. 
**          These could be left open, which hung the MVS name server.
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
**	19-Mar-97 (gordy)
**	    Use protocol level 63 which fixes Name Server GCA violations.
**	20-Aug-09 (gordy)
**	    Use more appropriate size for default buffer.  Use dynamic
**	    storage if actual length exceeds default size.
*/

STATUS
gcn_request( char *gcn_host, i4  *assoc_no, i4 *protocol )
{
    GCA_RQ_PARMS	request;
    STATUS		status = OK;
    char		tbuff[ GC_HOSTNAME_MAX + 12 ];
    char		*target;
    i4			len;

    /* 
    ** Prepare for GCA_REQUEST service call 
    */
    len = (gcn_host ? STlength( gcn_host ) + 2 : 0) + 10;
    target = (len <= sizeof( tbuff )) 
    	     ? tbuff : (char *)MEreqmem( 0, len, FALSE, NULL );

    if ( ! target )  return( E_GC0013_ASSFL_MEM );

    STprintf( target, "%s%s/IINMSVR", 
	      gcn_host ? gcn_host : "", gcn_host ? "::" : "" );

    MEfill( sizeof( request ), '\0', (PTR)&request );
    request.gca_peer_protocol = GCA_PROTOCOL_LEVEL_63;
    request.gca_partner_name = target;

    /* 
    ** Make GCA_REQUEST service call 
    */
    gca_call( &gcn_gca_cb, GCA_REQUEST, (GCA_PARMLIST *)&request,
	      GCA_SYNC_FLAG, NULL, GCN_RCV_TIMEOUT, &status );

    gcn_checkerr( "GCA_REQUEST", 
		  &status, request.gca_status, &request.gca_os_status );

    if ( status != OK )  goto done;

    if ( gcn_host  &&  request.gca_peer_protocol < GCA_PROTOCOL_LEVEL_50 )
    {
        gcn_release( request.gca_assoc_id );
	CL_CLEAR_ERR( &request.gca_os_status );
	gcn_checkerr( "GCA_REQUEST", 
		      &status, E_GC000A_INT_PROT_LVL, &request.gca_os_status );
	goto done;
    }

    *assoc_no = request.gca_assoc_id;
    *protocol = request.gca_peer_protocol;

  done :

    if ( target != tbuff )  MEfree( (PTR)target );
    return( status );
}


/*{
** Name: gcn_recv - Issue GCA_RECEIVE and GCA_INTERPRET
**
** Description:
**	Receive a GCA message from the Name Server.  
**
** Inputs:
**	assoc_id		Association id.
**	buff			Buffer to receive message.
**	buff_len		Length of buff.
**	formatted		TRUE if GCD formatted interface desired.
**
** Outputs:
**	msg_type		Message type.
**	msg_size		Message length.    
**	eod			End-of-data flag.
**	eog			End-of-group flag.
**
** Returns:
**	STATUS
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Overhauled.  Added eod flag.
**	18-Jun-91 (alan)
**	    Boost GCA_RECEIVE tmo (for netu authorization updates).
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
**	19-Mar-97 (gordy)
**	    No GCA_INTERPRET at API LEVEL 5.  Added support for
**	    GCD formatted interface.
*/

STATUS
gcn_recv( i4 assoc_id, char *buff, i4 buff_len, bool formatted,
          i4  *msg_type, i4 *msg_size, bool *eod, bool *eog )
{
    GCA_RV_PARMS	recv;
    STATUS		status;

    MEfill( sizeof( recv ), 0, (PTR)&recv );
    recv.gca_association_id = assoc_id;
    recv.gca_flow_type_indicator = GCA_NORMAL;
    recv.gca_modifiers = formatted ? GCA_FORMATTED : 0;
    recv.gca_buffer = buff;
    recv.gca_b_length = buff_len;

    gca_call( &gcn_gca_cb, GCA_RECEIVE, (GCA_PARMLIST *)&recv, 
	      GCA_SYNC_FLAG, NULL, GCN_RCV_TIMEOUT, &status );

    gcn_checkerr("GCA_RECEIVE", &status, recv.gca_status, &recv.gca_os_status);
    if ( status != OK )  return( status );

    *msg_type = recv.gca_message_type;
    *msg_size = recv.gca_d_length;
    *eod = recv.gca_end_of_data;
    *eog = recv.gca_end_of_group;

    return( OK );
}



/*{
** Name: gcn_send - Issue GCA_SEND
**
** Description:
**	Send a GCA message to the Name Server.
**
** Inputs:
**	assoc_id		Association id.
**	msg_type		Message type.
**	msg_buf			Message buffer.
**	msg_size		Message size.
**	eod			End of data flag.
**
** Outputs:
**	None
**
** Returns:
**	STATUS
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Overhauled.  Added eod flag.  Always synchronous for now.
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
**	19-Mar-97 (gordy)
**	    Messages are now formatted.
*/

STATUS
gcn_send( i4 assoc_id, 
	  i4  msg_type, char *msg_buf, i4 msg_size, bool eod )
{
    GCA_SD_PARMS	send;
    STATUS		status;

    MEfill( sizeof( send ), '\0', (PTR)&send );
    send.gca_association_id = assoc_id;
    send.gca_message_type = msg_type;
    send.gca_modifiers = GCA_FORMATTED;
    send.gca_buffer = msg_buf;
    send.gca_msg_length = msg_size;
    send.gca_end_of_data = eod;

    gca_call( &gcn_gca_cb, GCA_SEND, (GCA_PARMLIST *)&send,
	      GCA_SYNC_FLAG, NULL, -1, &status );

    gcn_checkerr( "GCA_SEND", &status, send.gca_status, &send.gca_os_status );

    return( status );
}

/*{
** Name: gcn_release - terminate GCA session.
**
** Description:
**	Release GCA association.
**
** Inputs:
**	assoc_id		Association id.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Overhauled.  Added send_flag.
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
*/

STATUS
gcn_release( i4 assoc_no )
{
    GCA_DA_PARMS	disc;
    STATUS		status;

    MEfill( sizeof( disc ), '\0', (PTR)&disc );
    disc.gca_association_id = assoc_no;

    gca_call( &gcn_gca_cb, GCA_DISASSOC, (GCA_PARMLIST *)&disc,
	      GCA_SYNC_FLAG, NULL, -1, &status );

    gcn_checkerr("GCA_DISASSOC", &status, disc.gca_status, &disc.gca_os_status);

    return( status );
}


/*{
** Name: gcn_fastselect - make special request to a server
**
** Description:
**	This routine is called by IINAMU and NETU to make special
**	connections to the Comm Server or Name Server.  These special
**	connections consist of a GCA_REQUEST with modifiers set to
**	one of the "fast select" operations: GCA_CS_SHUTDOWN or 
**	GCA_CS_QUIESCE.  The server acknowledges the operation by
**	refusing the connection with a CS_OK status.
**
** Input:
**	oper_code:	operation (usually GCA_CS_SHUTDOWN|GCA_NO_XLATE)
**	target:		listen address of target server
**
** Output:
**	None.
**
** Returns:
**	STATUS		E_GC0040_CS_OK -- operation successful
**
** History:
**      30-Jan-89 (jorge)
**          Initial function creation.
**	09-Sep-89 (seiwald)
**	    Changed from gcc_request to gcc_doop.
**	27-Nov-89 (seiwald)
**	    Changed from gcc_doop gcn_fastselect.
**	13-May-91 (brucek)
**	    Added call to gcn_checkerr on bad status.
**	05-Jan-93 (brucek)
**	    Don't treat E_GC0152_GCN_SHUTDOWN as error.
**	29-Jan-93 (brucek)
**	    E_GC0040_CS_OK is now the "shutdown ok" status.
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
*/

STATUS
gcn_fastselect( u_i4 oper_code, char *target )
{
    GCA_RQ_PARMS	request;
    i4	    	assoc_no;
    STATUS		status;
    STATUS		tmp_stat;

    MEfill( sizeof( request ), '\0', (PTR)&request );
    request.gca_partner_name = target;
    request.gca_modifiers = oper_code;

    gca_call( &gcn_gca_cb, GCA_REQUEST, (GCA_PARMLIST *)&request, 
	      GCA_SYNC_FLAG, NULL, (i4)(-1), &status );
    if ( status == OK )  status = request.gca_status;

    if ( status != E_GC0040_CS_OK )
    {
	gcn_checkerr( "GCA_REQUEST", &status, 
		      request.gca_status, &request.gca_os_status);
    }

    gcn_release( request.gca_assoc_id );

    return( status );
}


/*{
** Name: gcn_testaddr - Check if server is running.
**
** Description:
**	Connects and disconnects to a server
**
** Input:
**	target		Server address
**	direct:		TRUE if address is resolved (GCA_NO_XLATE).
**	username:	username of user being tested or NULL
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	11-Apr-90 (seiwald)
**	    Written.
**      04-Oct-93 (joplin)
**	    Added username parameter.
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
**	19-Mar-97 (gordy)
**	    Removed global communications buffers.
*/

STATUS
gcn_testaddr( char *target, i4  direct, char *username )
{
    GCA_RQ_PARMS	request;
    i4		count = 0;
    STATUS		status;
    STATUS		req_status;

    MEfill( sizeof( request ), '\0', (PTR)&request );
    request.gca_partner_name = target;
    request.gca_modifiers = direct ? GCA_NO_XLATE : 0;
    request.gca_user_name = username;

    gca_call( &gcn_gca_cb, GCA_REQUEST, (GCA_PARMLIST *)&request,
	      GCA_SYNC_FLAG, NULL, -1, &status );

    gcn_checkerr( "GCA_REQUEST", &status, 
	          request.gca_status, &request.gca_os_status );

    /*
    ** If we connected successfully, release the connection.
    */
    if ( status == OK )
	gcn_send( request.gca_assoc_id, GCA_RELEASE, 
		  (char *)&count, sizeof( count ), TRUE );

    /*
    ** Even failed GCA_REQUEST calls need to call GCA_DISASSOC.
    */
    gcn_release( request.gca_assoc_id );

    return( status );
}


/*
** Name: gcn_bedcheck - check if Name Server is running
**
** Description:
**	Perform a GCA_FASTSELECT to the local name server.  
**	If user has MONITOR priviledges, verifies that the
**	server is the Name Server.
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
**	04/14/92 (brucek) Created.
**	01/24/94 (brucek) Convert to use GCA_FASTSELECT for bedcheck.
**	 3-Apr-96 (gordy)
**	    Fixed parameters to IIGCa_call().
**	 3-Sep-96 (gordy)
**	    Use new GCA control block interface.
**	19-Mar-97 (gordy)
**	    Set our own GCA protocol level, don't uses Name Server's.
**	    Removed global communications buffers.
**	30-mar-2004 (somsa01)
**	    Corrected initialization of gca_partner_name.
**	20-Aug-09 (gordy)
**	    Allow space for string terminator in GCN address.
*/

STATUS
gcn_bedcheck( VOID )
{
    GCA_FS_PARMS	fast;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    char		gcn_addr[ GCA_SERVER_ID_LNG + 1 ];
    static char		msg_buff[ GCA_FS_MAX_DATA ];
    char		obj_buffer [ 32 ]; 
    char		*p, *var;
    i4			len, server_flags;
    char		gca_partner_name[] = "/IINMSVR";
 
    /*
    ** We can bail out early if Name Server
    ** ID has not been globally registered.
    */
    status = GCnsid( GC_FIND_NSID, gcn_addr, sizeof( gcn_addr ), &sys_err );
    if ( status == GC_NM_SRVR_ID_ERR )  return( E_GC0126_GCN_NO_GCN );

    /* 
    ** Roll up GCM_GET message.
    */
    p = msg_buff;		/* point to start of message data */
    p += sizeof( i4 );          /* bump past error_status */
    p += sizeof( i4 );          /* bump past error_index */
    p += sizeof( i4 );          /* bump past future[0] */
    p += sizeof( i4 );          /* bump past future[1] */
    p += gcm_put_int( p, -1 );  /* set client_perms */
    p += gcm_put_int( p, 1 );   /* set row_count */
    p += gcm_put_int( p, 1 );   /* set element_count */
    p += gcm_put_str( p, GCA_MIB_CLIENT_FLAGS );	/* classid */
    p += gcm_put_str( p, "" );		/* instance */
    p += gcm_put_str( p, "" );		/* value */
    p += gcm_put_int( p, 0 );		/* perms */

    /* 
    ** Issue GCA_FASTSELECT.
    */
    MEfill( sizeof( fast ), '\0', (PTR)&fast );
    fast.gca_partner_name = gca_partner_name;
    fast.gca_modifiers = GCA_RQ_GCM;
    fast.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    fast.gca_buffer = msg_buff;
    fast.gca_b_length = sizeof( msg_buff );
    fast.gca_message_type = GCM_GETNEXT;
    fast.gca_msg_length = p - msg_buff;

    gca_call( &gcn_gca_cb, GCA_FASTSELECT, (GCA_PARMLIST *)&fast, 
	      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( status == OK  &&  fast.gca_status != OK )  status = fast.gca_status;

    /* 
    ** If we can't probe it, we'll just 
    ** have to assume it is a GCN. 
    */
    if ( status == E_GC003F_PM_NOPERM )  
	status = OK;
    else  if ( status == OK )
    {
	/* 
	** Unroll GCM msg.
	*/
        p = msg_buff;
        p += gcm_get_int( p, &status );

        /* 
        ** if GCM GET succeeded, test server flags.
        */
        if ( status == OK )
        {
	    p += sizeof( i4 );		/* bump past element_count */
	    p += sizeof( i4 );		/* bump past error_index */
	    p += sizeof( i4 );		/* bump past future[0] */
	    p += sizeof( i4 );		/* bump past future[1] */
	    p += sizeof( i4 );		/* bump past client_perms */
	    p += sizeof( i4 );		/* bump past row_count */
            p += gcm_get_str( p, &var, &len );  /* skip classid */
            p += gcm_get_str( p, &var, &len );  /* skip instance */
            p += gcm_get_str( p, &var, &len );  /* get value */
            MEcopy( var, len, obj_buffer );
            obj_buffer[ len ] = '\0';
            CVan( obj_buffer,  &server_flags );

	    if ( ! (server_flags & GCA_RG_IINMSVR) )
    	        status = E_GC0126_GCN_NO_GCN;
        }
    }
 
    return( status );
}
