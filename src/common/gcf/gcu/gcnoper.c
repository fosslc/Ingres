/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cv.h>
#include    <gc.h>
#include    <id.h>
#include    <me.h>
#include    <mu.h>
#include    <pc.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcm.h>
#include    <gcd.h>
#include    <gcu.h>
#include    <gcaimsg.h>


/**
**
**  Name: gcnoper.c
**
**  Description:
**      Contains the following functions:
**
**	IIgcn_oper		Execute an operation on Name Database.
**	IIgcn_stop		Stop the Name Server.
**	IIgcn_check		Check for Name Server or embedded interface.
**
**  History:    $Log-for RCS$
**	    
**      03-Sep-87   (lin)
**          Initial creation.
**	01-Mar-89 (seiwald)
**	    Name Server (IIGCN) revamped.  Message formats between
**	    IIGCN and its clients remain the same, but the messages are
**	    constructed and extracted via properly aligning code.
**	    Variable length structures eliminated.  Strings are now
**	    null-terminated (they had to before, but no one admitted so).
**	9-May-89 (jorge)
**	    Made gcn_respnse return an op code that is used to
**	    enable  IIgcn_nm_oper the opportunity  of gracefully terminating
**	    the connection.
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	29-Jun-1989 (jorge)
**	    fix formating output problems so that columns are aligned.
**	17-Jul-89 (seiwald)
**	    Fix jorge's fix for output formatting.
**	    IINAMU now sends GCA_RELEASE after receiving the GCN_SHUTDOWN
**	    response from the name server.  IINAMU then exits after 
**	    disassociating.
**	26-Nov-89 (seiwald)
**	    Changed hardwired references to GCN_NODE and GCN_LOGIN.
**	    Fix up gcn_erhndl to truncate error message strings.
**	19-Nov-90 (seiwald)
**	    Added support for , separated object lists during iinamu
**	    or netu adds.  This allows users manually to add servers
**	    serving more than one database (or vnode).
**	03-Jan-91 (brucek)
**	    Added sole server indication to result displays
**	31-Jan-91 (brucek)
**	    Fixed sole server support (flag was uninitialized)
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	24-Apr-91 (brucek)
**	    Added include for tm.h
**	20-Nov-91 (seiwald)
**	    Cast Out_data_area to char * before doing pointer arithmetic.
**	2-June-92 (seiwald)
**	    Added gcn_host to GCN_CALL_PARMS to support connecting to a remote
**	    Name Server.
**	2-July-92 (jonb)
**	    If gcn_response_func is non-NULL in GCN_CALL_PARMS, call it
**	    for every line of output that would otherwise go to the screen.
**      23-Mar-93 (smc)
**          Addef forward declarations of gcn_respond & gcn_respsh.
**	13-May-93 (brucek)
**	    Changed E_GC0145_GCN_NO_GCN to E_GC0126_GCN_NO_GCN.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	24-Jan-94 (brucek)
**	    Modified gcn_bedcheck() to issue GCM query to determine
**	    whether process listening at Name Server address is really GCN.
**	16-May-94 (seiwald)
**	    Fixed above change: pull the status from the GCA_FS_PARMS so
**	    that failures are honestly reported.  Unfortunately, one of
**	    the legal failures from a working Name Server is that the 
**	    caller doesn't have permission to make a GCM query.
**	29-Nov-95 (gordy)
**	    Renamed to IIgcn_oper().
**	 4-Dec-95 (gordy)
**	    Added prototypes.
**	 8-Dec-95 (gordy)
**	    Renamed old IIgcn_oper() to gcn_oper_nmsvr() and replaced
**	    with routine to test for embedded access.  Added IIgcn_stop()
**	    and IIgcn_check().  Moved gcn_bedcheck() to gcngca.c.
**	    Added gcn_oper_embed() and gcn_result() to support embedded
**	    access to the Name Database.
**	24-Apr-96 (chech02) windows 3.1 port.
**	    If request is GCN_RET in gcn_oper_embed() and the row_count is
**	    0, gcn_result() still needs to be called to set the flag, 
**	    lastOpStat, in netutil!nuaccess.c.    
**	 3-Sep-96 (gordy)
**	    Moved all GCA access to gcngca.c to support using the
**	    new GCA control block interface.
**	11-Feb-97 (gordy)
**	    Access to TICKET info requires NET_ADMIN privilege.
**	    Use case insensitive compare for determining access type.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	20-Mar-97 (gordy)
**	    Rewritten to support PROTOCOL_LEVEL_63 (Name Server protocol
**	    now uses end-of-group for turn-aroune) and formatted messages.
**	27-May-97 (thoda04)
**	    Included cv.h, id.h headers for function prototypes.
**	 1-Jun-98 (gordy)
**	    Use same buffer size as Name Server.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 3-Aug-09 (gordy)
**	    Remove string length restrictions.
**/

/*
**  Forward and/or External function references.
*/
static STATUS	gcn_oper_nmsvr( i4,  GCN_CALL_PARMS * );
static STATUS	gcn_oper_embed( i4,  GCN_CALL_PARMS * );
static VOID	gcn_result( i4, GCN_CALL_PARMS *, i4, char *, GCN_TUP * );
static VOID	gcn_res_raw( i4  (*)(), char *, i4, bool );
static VOID	gcn_res_fmt( char *, i4 );
static VOID	gcn_res_out( char *, i4, bool );
static VOID	gcn_row_out( GCD_GCN_REQ_TUPLE * );
static VOID	gcn_err_raw( i4  (*)(), char *, i4, bool );
static VOID	gcn_err_fmt( char *, i4 );
static VOID	gcn_err_out( char *, i4, bool );

/*
** Determine interface being used.
*/

#define	GCN_ACCESS_UNKNOWN	0
#define	GCN_ACCESS_NMSVR	1
#define	GCN_ACCESS_EMBEDDED	2
#define	GCN_ACCESS_CLOSED	3

/*
** Local data.
*/
static i4	gcn_access = GCN_ACCESS_UNKNOWN;
static char	*gcn_msg_buf = NULL;
static i4	gcn_msg_len = GCN_MSG_BUFF_LEN;



/*
** Name: IIgcn_check
**
** Description:
**	Cover for gcn_bedcheck() which also checks to see if the
**	embedded Name Server interface can be used if no Name
**	Server is available.
**
**	GCA uses compile time symbols, environment logicals, and
**	config file entries to determine if the embedded interface
**	should be used.  We just check to see if GCA has enabled
**	the embedded interface.
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
**	 8-Dec-95 (gordy)
**	    Created.
**	 3-Sep-96 (gordy)
**	    GCA global data restructured.
*/

STATUS
IIgcn_check( VOID )
{
    STATUS	status;
    CL_ERR_DESC	cl_err;
    
    status = gcn_bedcheck();

    if ( status == OK )
	gcn_access = GCN_ACCESS_NMSVR;
#ifdef GCF_EMBEDDED_GCN
    else  if ( IIGCa_global.gca_embedded_gcn )
    {
	gcn_access = GCN_ACCESS_EMBEDDED;
	status = OK;
    }
#endif
    else
    {
	gcn_access = GCN_ACCESS_CLOSED;
	CL_CLEAR_ERR( &cl_err );
        gcn_checkerr("GCA_FASTSELECT", &status, E_GC0026_NM_SRVR_ERR, &cl_err);
    }
    
    return( status );
}


/*
** Name: IIgcn_stop
**
** Description:
**	Shutdown the Name Server process.  
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
**	 8-Dec-95 (gordy)
**	    Created.
**	20-Aug-09 (gordy)
**	    Use more appropriate length for address buffer.
*/

STATUS
IIgcn_stop( VOID )
{
    char	gcnaddr[ GCA_SERVER_ID_LNG + 1 ];
    STATUS	status = OK;
    CL_ERR_DESC cl_err;
    
    if (gcn_access == GCN_ACCESS_CLOSED || gcn_access == GCN_ACCESS_EMBEDDED)
    {
	CL_CLEAR_ERR( &cl_err );
        gcn_checkerr( "IIgcn_stop", &status, E_GC0126_GCN_NO_GCN, &cl_err );
    }
    else
    {
	status = GCnsid( GC_FIND_NSID, gcnaddr, sizeof(gcnaddr), &cl_err );

        if ( status != GC_NM_SRVR_ID_ERR )
	    status = gcn_fastselect( GCA_CS_SHUTDOWN | GCA_NO_XLATE, gcnaddr );
	else
	{
	    gcn_access = GCN_ACCESS_CLOSED;
	    status = E_GC0126_GCN_NO_GCN;
	    gcn_checkerr("IIgcn_stop", &status, E_GC0026_NM_SRVR_ERR, &cl_err);
	}

    }
    
    return( status );
}


/*
** Name: IIgcn_oper
**
** Description:
**	Route Name Database request to proper interface, embedded or
**	Name Server via GCA.  The following criteria determine routine:
**
**	*   The Embedded interface access must have been enabled 
**	    previously.  In general, the embedded interface is only 
**	    used if there is no Name Server available and GCA 
**	    determined that the embedded interface could be used.
**
**	*   If a host name is specified, the request is assumed to be
**	    to a remote host and GCA will be used.  Note that this may
**	    still use the embedded Name Server interface within GCA and
**	    possibly the embedded Comm Server interface for remote
**	    communications.  
**
** Inputs:
**	request		GCN operation code.
**	params		GCN parameter list.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	 8-Dec-95 (gordy)
**	    Renamed old IIgcn_oper() to gcn_oper_nmsvr() and converted
**	    this routine to a router for embedded vs Name Server.
*/

STATUS
IIgcn_oper( i4  request, GCN_CALL_PARMS *params )
{
    STATUS	status;

#ifdef GCF_EMBEDDED_GCN
    if ( gcn_access == GCN_ACCESS_EMBEDDED  &&  ! params->gcn_host )
	status = gcn_oper_embed( request, params );
    else
#endif
	status = gcn_oper_nmsvr( request, params );
    
    return( status );
}


/*{
** Name: gcn_oper_nmsvr
**
** Description:
**	This routine takes the parameters specified by the user, formats
**	them in the message block and sends it to the Name Server. It also
**	receives the returned message from the Name Server and takes action
**	based on the returned message type. If the requested operation is
**	add or delete, the number of rows is returned in the returned message.
**	This number is printed on the ternimal. If the requested operation is
**	retrieve, the result of the retrieved data is printed on the terminal.
**
** Inputs:
**      service_code	Identifier of the requested GCA service.
**      param_list	Pointer to the data structure containing the
**			parameters required by the requested service.
**
** Outputs:
**
**
**      status                          Result of the service request execution.
**                                      The following values may be returned:
**
**		E_GCN000_OK		Request accepted for execution.
**		E_GCN003_INV_SVC_CODE	Invalid service code
**		E_GCN004_INV_PLIST_PTR	Invalid parameter list pointer
**
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**      03-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Overhauled.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	29-Nov-95 (gordy)
**	    Renamed to IIgcn_oper().
**	 8-Dec-95 (gordy)
**	    Renamed to gcn_oper_nmsvr() as companion to gcn_oper_embed().
**	 3-Sep-96 (gordy)
**	    Moved all GCA access to gcngca.c to support using the
**	    new GCA control block interface.
**	20-Mar-97 (gordy)
**	    Rewritten to support PROTOCOL_LEVEL_63 (Name Server protocol
**	    now uses end-of-group for turn-aroune) and formatted messages.
**	 3-Aug-09 (gordy)
**	    Provide standard sized default buffer.  Use dynamic storage
**	    if actual length exceeds default size.
*/

static STATUS
gcn_oper_nmsvr( i4  service_code,  GCN_CALL_PARMS *param_list )
{
    GCA_PARMLIST	parms;
    GCD_GCN_D_OPER	*oper;
    STATUS		status = E_DB_OK;
    CL_ERR_DESC		cl_err;
    char		buffer[ 256 ];
    char		*objbuf;
    char		*objvec[ 128 ];
    bool		eod, eog;
    bool		continued = FALSE;
    bool		formatted = FALSE;
    i4			protocol, msg_len;
    i4			assoc_no, msg_type;
    i4			objcnt, objlen;
    i4			i, op = 0;
    
    CL_CLEAR_ERR( &cl_err );

    /*
    ** Don't bother trying if the request is local and
    ** there is no local Name Server.  Note that the
    ** embedded interface is selected only if there is
    ** no local Name Server.
    */
    if ( 
	 ! param_list->gcn_host  &&
	 (gcn_access == GCN_ACCESS_CLOSED || gcn_access == GCN_ACCESS_EMBEDDED) 
       )
    {
        gcn_checkerr( "IIgcn_oper", &status, E_GC0126_GCN_NO_GCN, &cl_err );
	return( status );
    }
    
    if ( ! gcn_msg_buf )
    {
	gcn_msg_buf = (char *)MEreqmem( 0, gcn_msg_len, FALSE, &status );

	if ( ! gcn_msg_buf )
	{
	    status = E_GC0121_GCN_NOMEM;
	    gcn_checkerr( "MEreqmem", &status, FAIL, &cl_err );
	    return( status );
	}
    }

    /* 
    ** Build the association. 
    */
    status = gcn_request( param_list->gcn_host, &assoc_no, &protocol );
    if ( status != OK )  return( status );
    if ( protocol >= GCA_PROTOCOL_LEVEL_63 )  formatted = TRUE;

    /* 
    ** Split apart object at ,'s.
    */
    objlen = STlength( param_list->gcn_obj );
    objbuf = (objlen < sizeof( buffer )) 
    	     ? buffer : (char *)MEreqmem( 0, objlen + 1, FALSE, NULL );
    if ( ! objbuf )  return( E_GC0121_GCN_NOMEM );

    objcnt = gcu_words( param_list->gcn_obj, objbuf, objvec, ',', 128 );

    if ( ! objcnt )
    {
	objcnt = 1;
	objvec[ 0 ] = param_list->gcn_obj;
    }

    /*
    ** We really only expect to get one object,
    ** so if there are too many for the send
    ** buffer, just throw the rest away.
    */
    objcnt = min( objcnt, ((gcn_msg_len - sizeof( GCD_GCN_D_OPER )) 
			  / sizeof( GCD_GCN_REQ_TUPLE )) + 1 );

    /*
    ** Bundle up request.
    */
    oper = (GCD_GCN_D_OPER *)gcn_msg_buf;
    oper->gcn_flag = param_list->gcn_flag;
    oper->gcn_opcode = service_code;
    oper->gcn_install.gcn_l_item = STlength( param_list->gcn_install ) + 1;
    oper->gcn_install.gcn_value = param_list->gcn_install;
    oper->gcn_tup_cnt = objcnt;

    for( i = 0; i < objcnt; i++ )
    {
	oper->gcn_tuple[i].gcn_type.gcn_value = param_list->gcn_type;
	oper->gcn_tuple[i].gcn_type.gcn_l_item = 
					STlength( param_list->gcn_type ) + 1;
	oper->gcn_tuple[i].gcn_uid.gcn_value = param_list->gcn_uid;
	oper->gcn_tuple[i].gcn_uid.gcn_l_item = 
					STlength( param_list->gcn_uid ) + 1;
	oper->gcn_tuple[i].gcn_obj.gcn_value = objvec[i];
	oper->gcn_tuple[i].gcn_obj.gcn_l_item = 
					STlength( objvec[i] ) + 1;
	oper->gcn_tuple[i].gcn_val.gcn_value = param_list->gcn_value;
	oper->gcn_tuple[i].gcn_val.gcn_l_item = 
					STlength( param_list->gcn_value ) + 1;
    }

    status = gcn_send( assoc_no, GCN_NS_OPER, gcn_msg_buf, 
		       sizeof( GCD_GCN_D_OPER ) + 
		       (objcnt - 1) * sizeof( GCD_GCN_REQ_TUPLE ), TRUE );
    if ( status != OK )  goto done;

    /* 
    ** Receive response from name server. 
    */
    for(;;)
    {
	status = gcn_recv( assoc_no, gcn_msg_buf, gcn_msg_len, formatted,
			   &msg_type, &msg_len, &eod, &eog );

	if ( status != OK )  goto done;

	switch( msg_type )
	{
	    case GCN_RESULT:
		if ( ! param_list->gcn_response_func )
		    if ( formatted )
			gcn_res_out( gcn_msg_buf, msg_len, continued );
		    else
			gcn_res_fmt( gcn_msg_buf, msg_len );
		else  if ( formatted )
		    gcn_res_raw( param_list->gcn_response_func,
				 gcn_msg_buf, msg_len, continued );
		else
		    (*param_list->gcn_response_func)( msg_type, 
						      gcn_msg_buf, msg_len );
		break;

	    case GCA_ERROR:
		if ( ! param_list->gcn_response_func )
		    if ( formatted )
			gcn_err_out( gcn_msg_buf, msg_len, continued );
		    else
			gcn_err_fmt( gcn_msg_buf, msg_len );
		else  if ( formatted )
		    gcn_err_raw( param_list->gcn_response_func,
				 gcn_msg_buf, msg_len, continued );
		else
		    (*param_list->gcn_response_func)( msg_type, 
						      gcn_msg_buf, msg_len );
		break;

	    case GCA_RELEASE:
		status = gcn_release( assoc_no );
		goto done;

	    default:
		break;
	}

	continued = FALSE;

	if ( protocol < GCA_PROTOCOL_LEVEL_63 )
	    { if ( eod )  break; }
	else  if ( eod )
	    { if ( eog )  break; }
	else  
	    continued = TRUE;
    }

    objcnt = 0;
    status = gcn_send( assoc_no, GCA_RELEASE, 
		       (char *)&objcnt, sizeof( objcnt ), TRUE );
    if ( status != OK )  goto done;

    status = gcn_release( assoc_no );

  done:

    if ( objbuf != buffer )  MEfree( (PTR)objbuf );
    return( status );
}


#ifdef GCF_EMBEDDED_GCN
/*
** Name: gcn_oper_embed
**
** Description:
**	Use the embedded Name Server interface to satisfy the request.
**	This is a combination of the routines gcn_oper_nmsvr() and 
**	gcn_oper_ns().
**
** Input:
**	request		GCN operation code.
**	params		GCN parameter list.
**
** Output:
**	None.
** Returns:
**	STATUS
**
** History:
**	 8-Dec-95 (gordy)
**	    Created.
**	11-Feb-97 (gordy)
**	    Access to TICKET info requires NET_ADMIN privilege.
**	    Use case insensitive compare for determining access type.
**	 3-Aug-09 (gordy)
**	    Provide standard sized default buffer.  Use dynamic storage
**	    if actual length exceeds default size.  Use appropriate size 
**	    for username buffer.
*/

static STATUS
gcn_oper_embed( i4  request, GCN_CALL_PARMS *params )
{
    STATUS		status = OK;
    CL_ERR_DESC		cl_err;
    GCN_TUP		tup;
    GCN_QUE_NAME	*nq;
    char		buffer[ 256 ];
    char		*objbuf;
    char		*objvec[ 128 ];
    char		flagbuf[ 9 ];
    char		uid[ GC_USERNAME_MAX + 1 ];
    char		*puid = uid;
    char		*gcn_type = params->gcn_type;
    i4			flag = params->gcn_flag;
    i4			row_count, objcnt, objlen;
    bool		global, merge, setuid;
    bool		isnode, islogin, isserver;
    bool		islticket, isrticket;
    
    /*
    ** Initialize local control variables.
    */
    CL_CLEAR_ERR( &cl_err );
    IDname( &puid );
    STzapblank( uid, uid );

    if ( ! gcn_msg_buf )
    {
	gcn_msg_buf = (char *)MEreqmem( 0, gcn_msg_len, FALSE, &status );

	if ( ! gcn_msg_buf )
	{
	    status = E_GC0121_GCN_NOMEM;
	    gcn_checkerr( "MEreqmem", &status, FAIL, &cl_err );
	    return( status );
	}
    }

    global = (flag & GCN_PUB_FLAG) ? TRUE : FALSE;
    setuid = (flag & GCN_UID_FLAG) ? TRUE : FALSE;
    merge  = (flag & GCN_MRG_FLAG) ? TRUE : FALSE;
    flag &= ~(GCN_MRG_FLAG | GCN_UID_FLAG);

    if ( ! gcn_type  ||  ! gcn_type[ 0 ] )  
	gcn_type = IIGCn_static.svr_type;

    isnode =    ! STcasecmp( gcn_type, GCN_NODE );
    islogin =   ! STcasecmp( gcn_type, GCN_LOGIN );
    islticket = ! STcasecmp( gcn_type, GCN_LTICKET );
    isrticket = ! STcasecmp( gcn_type, GCN_RTICKET );
    isserver =  ! isnode  &&  ! islogin  &&  ! islticket  &&  ! isrticket;
    
    /*
    ** Check that user has required privileges for operation requested.
    **
    ** If adding or deleting a server registry entry, the 
    ** SERVER_CONTROL privilege is required.
    **
    ** If adding or deleting a global vnode or remote auth entry, the
    ** NET_ADMIN privilege is required.
    **
    ** If accessing another user's vnode or remote auth entry, the
    ** NET_ADMIN privilege is required.
    */
    if ( isserver  &&  (request == GCN_ADD  ||  request == GCN_DEL) )
    {
	status = gca_chk_priv( uid, GCA_PRIV_SERVER_CONTROL );

	if ( status != OK )
	{
	    gcn_checkerr( "gca_chk_priv", 
			  &status, E_GC003F_PM_NOPERM, &cl_err );
	    return( status );
	}
    }
    else  if ( islticket  ||  isrticket  ||
	       ( (isnode  ||  islogin)  && 
	         (setuid  ||  (global  &&  ( request == GCN_ADD  ||  
					     request == GCN_DEL ))) ) ) 
    {
	status = gca_chk_priv( uid, GCA_PRIV_NET_ADMIN );

	if ( status != OK )
	{
	    gcn_checkerr( "gca_chk_priv", 
			  &status, E_GC003F_PM_NOPERM, &cl_err );
	    return( status );
	}
    }

    /*
    ** Make sure we can access the Name Queue
    ** for the requested type.
    */
    nq = gcn_nq_find( gcn_type );

    if ( ! nq  &&  isserver  &&  request == GCN_ADD )
	nq = gcn_nq_create( gcn_type );

    if ( ! nq )
    {
	gcn_checkerr( "gcn_nq_find", 
		      &status, E_GC0100_GCN_SVRCLASS_NOTFOUND , &cl_err );
	return( status );
    }
    
    /* 
    ** Get user ID mask.  Note that this overrides the user ID
    ** parameter with the current user ID unless the setuid
    ** flag was set (requires NET_ADMIN privilege).
    */
    if ( nq->gcn_transient )
	tup.uid = "";
    else  if ( global )
	tup.uid = GCN_GLOBAL_USER;
    else  if ( setuid )
        tup.uid = params->gcn_uid ? params->gcn_uid : "";
    else
	tup.uid = uid;

    /* 
    ** Split apart object at ,'s.
    */
    objlen = STlength( params->gcn_obj );
    objbuf = (objlen < sizeof( buffer ))
    	     ? buffer : (char *)MEreqmem( 0, objlen + 1, FALSE, NULL );
    if ( ! objbuf )  return( E_GC0121_GCN_NOMEM );

    objcnt = gcu_words( params->gcn_obj, objbuf, objvec, ',', 128 );

    if ( ! objcnt )
    {
	objcnt = 1;
	objvec[ 0 ] = params->gcn_obj ? params->gcn_obj : "";
    }

    tup.obj = objvec[ 0 ];
    tup.val = params->gcn_value ? params->gcn_value : "";

    /* 
    ** When adding servers, delete other server entries with the same
    ** tup.val (the listen_id).  Format the flag word for the uid field.
    ** If the merge flag is set, only delete an identical server entry
    ** (so as not to produce duplicates when the add takes place).
    **
    ** When adding login info, delete any other login info for that user.
    **
    ** When deleting, allow wild cards in obj and val.
    */
    if ( request == GCN_ADD )
    {
        if ( nq->gcn_transient )
        {
	    if ( ! merge )  tup.obj = "";
	    CVlx( (long)flag, flagbuf );
	}
	else  if ( islogin )
	    tup.val = "";
	else  if ( isnode  &&  ! merge )  
	    tup.val = "";
    }
    else  if ( request == GCN_DEL )
    {
	if ( ! STcompare( tup.obj, "*" ) )  tup.obj = "";
	if ( ! STcompare( tup.val, "*" ) )  tup.val = "";
	if ( ! STcompare( tup.obj, "\\*" ) )  tup.obj = "*";
	if ( ! STcompare( tup.val, "\\*" ) )  tup.val = "*";
    }

    /*
    ** Finally, access the Name Queue and process the request.
    */
    if ( gcn_nq_lock( nq, request == GCN_ADD || request == GCN_DEL ) != OK )
    {
	gcn_checkerr( "gcn_nq_lock", 
		      &status, E_GC0134_GCN_INT_NQ_ERROR, &cl_err );
	goto done;
    }

    switch( request )
    {
	case GCN_RET:
	    {
		GCN_TUP	*info, tupvec[ GCN_SVR_MAX ];
		char	buff[ 128 ];
		char	*tmp;
		char	*pv[2];
		i4	i, tuptotal = 0;

		/* 
		** Call gcn_nq_ret in a loop to get a batch at a time. 
		*/
		do
		{
		    /* 
		    ** Load up the next entries.
		    */
		    row_count = gcn_nq_ret( nq, &tup, 
					    tuptotal, GCN_SVR_MAX, tupvec );

             
		    if (row_count == 0)
		    {
			gcn_result( GCN_RET, params, 0, NULL, NULL);
		    	break;
		    }

		    /* 
		    ** Loop through server entries.
		    */
		    for( i = 0; i < row_count; tuptotal++, i++ )
		    {
			info = &tupvec[ i ];
			tmp = buff;

			/* 
			** Strip password from login entry. 
			*/
			if ( islogin )
			{
			    i4 len = STlength( info->val ) + 1;

			    if ( len > sizeof( buff ) )
			    {
			    	tmp = (char *)MEreqmem( 0, len, FALSE, NULL );
			    	if ( ! tmp )  return( E_GC0013_ASSFL_MEM );
			    }

			    gcu_words( info->val, tmp, pv, ',', 2 );
			    info->val = pv[0];
			}

			/*
			** Unlike the Name Server, we hand off 1 entry
			** per message just because its easier.
			*/
			gcn_result( GCN_RET, params, 1, nq->gcn_type, info );
		    	if ( tmp != buff )  MEfree( (PTR)tmp );
		    }

		} while( row_count == GCN_SVR_MAX );
	    }
	    break;

	case GCN_ADD:
	    /*
	    ** First delete any overlapping tuples and report that to the
	    ** user.
	    */
	    if ( (row_count = gcn_nq_del( nq, &tup )) )
		gcn_result( GCN_DEL, params, row_count, NULL, NULL );

	    /*
	    ** Insert the tuples into the name queue.  Fix
	    ** tuple values which were modified for the
	    ** delete above.
	    */
	    if ( nq->gcn_transient )  tup.uid = flagbuf;
	    tup.val = params->gcn_value ? params->gcn_value : "";

	    for( row_count = 0; row_count < objcnt; row_count++ )
	    {
		tup.obj = objvec[ row_count ];
		(void)gcn_nq_add( nq, &tup, GCN_REC_ADDED, (i4)0, (i4)0 );
	    }

	    gcn_result( GCN_ADD, params, row_count, NULL, NULL );
	    break;
	
	case GCN_DEL:
	    /*
	    ** Delete selected tuples.
	    */
	    gcn_result( GCN_DEL, params, gcn_nq_del( nq, &tup ), NULL, NULL );
	    break;

	case GCN_SECURITY:
	    break;
    }

    gcn_nq_unlock( nq );

  done:

    if ( objbuf != buffer )  MEfree( (PTR)objbuf );
    return( status );
}
#endif /* GCF_EMBEDDED_GCN */


/*
** Name: gcn_result
**
** Description:
**	Build a GCN_RESULT message buffer and pass to the
**	appropriate output function for processing.  A GCN_RET
**	result message can be built by providing the parameters
**	type and tup.  For other result messages, type and tup
**	should be NULL.
**
** Input:
**	opcode		GCN request type.
**	params		GCN request parameters.
**	count		Number of entries modified.
**	type		GCN tuple type, may be NULL.
**	tup		GCN tuple info, may be NULL.
**
** Ouput:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 8-Dec-95 (gordy)
**	    Created.
*/

static VOID
gcn_result( i4  opcode, GCN_CALL_PARMS *params, 
	    i4  count, char *type, GCN_TUP *tup )
{
    char	*p;

    p = gcn_msg_buf;

    p += gcu_put_int( p, opcode );
    p += gcu_put_int( p, count );
    if ( type )  p += gcu_put_str( p, type );
    if ( tup ) p += gcn_put_tup( p, tup );

    if ( params->gcn_response_func )
	(*params->gcn_response_func)( GCN_RESULT, 
				      gcn_msg_buf, p - gcn_msg_buf );
    else
	gcn_res_fmt( gcn_msg_buf, p - gcn_msg_buf );

    return;
}


/*
** Name: gcn_res_raw
**
** Description:
**	Converts a GCD formatted GCN_RESULT message into a raw
**	byte stream and calls a callback function for processing.
**
** Input:
**	func		Callback function.
**	buf		GCA message buffer containing GCD formatted message.
**	len		Length of GCD message.
**	continued	FALSE if buffer contains start of new message.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	20-Mar-97 (gordy)
**	    Created.
*/

static VOID
gcn_res_raw( i4  (*func)(), char *buf, i4 len, bool continued )
{
    GCD_GCN_REQ_TUPLE	*tuple;
    i4			op, count;
    char		buffer[ 512 ];
    char		*end = buf + len;

    if ( continued )
    {
	/*
	** This must be a GCN_RET result since this
	** is the only type which returns tuples.
	*/
	op = GCN_RET;
	count = 1;
	tuple = (GCD_GCN_REQ_TUPLE *)buf;
    }
    else
    {
	GCD_GCN_OPER_DATA	*oper = (GCD_GCN_OPER_DATA *)buf;

	op = oper->gcn_op;
	count = oper->gcn_tup_cnt;
	tuple = &oper->gcn_tuple[0];
    }

    switch( op )
    {
	case GCN_RET:
	    if ( count )
	    {
		/*
		** We return one tuple at a time
		** to the callback function.
		*/
		for( count = 1; (char *)tuple < end; tuple++ )
		{
		    buf = buffer;
		    buf += gcu_put_int( buf, op );
		    buf += gcu_put_int( buf, count );

		    if ( tuple->gcn_type.gcn_l_item )
			buf += gcu_put_str( buf, tuple->gcn_type.gcn_value );
		    else
			buf += gcu_put_str( buf, "" );

		    if ( tuple->gcn_uid.gcn_l_item )
			buf += gcu_put_str( buf, tuple->gcn_uid.gcn_value );
		    else
			buf += gcu_put_str( buf, "" );

		    if ( tuple->gcn_obj.gcn_l_item )
			buf += gcu_put_str( buf, tuple->gcn_obj.gcn_value );
		    else
			buf += gcu_put_str( buf, "" );

		    if ( tuple->gcn_val.gcn_l_item )
			buf += gcu_put_str( buf, tuple->gcn_val.gcn_value );
		    else
			buf += gcu_put_str( buf, "" );

		    (*func)( GCN_RESULT, buffer, buf - buffer );
		}

		/*
		** All done.
		*/
		break;
	    }

	    /*
	    ** Fall through to handle no tuples case.
	    */

	case GCN_ADD:
	case GCN_DEL:
	    buf = buffer;
	    buf += gcu_put_int( buf, op );
	    buf += gcu_put_int( buf, count );
	    (*func)( GCN_RESULT, buffer, buf - buffer );
	    break;
    }

    return;
}


/*
** Name: gcn_res_fmt
**
** Description:
**	Converts a raw byte stream GCN_RESULT message into a formatted
**	GCD structure and calls gcn_res_out() to display message.
**
** Input:
**	buf		GCA message buffer.
**	len		Length of GCA message.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	20-Mar-97 (gordy)
**	    Created.
*/

static VOID
gcn_res_fmt( char *buf, i4 len )
{
    GCD_GCN_OPER_DATA	oper;
    GCD_GCN_REQ_TUPLE	tuple;
    char		*end = buf + len;

    buf += gcu_get_int( buf, &oper.gcn_op );
    buf += gcu_get_int( buf, &oper.gcn_tup_cnt );
    if ( buf > end )  return;

    switch( oper.gcn_op )
    {
	case GCN_ADD:
	case GCN_DEL:
	    gcn_res_out( (char *)&oper, sizeof( oper ) - 
					sizeof( GCD_GCN_REQ_TUPLE ), FALSE );
	    break;

	case GCN_RET:
	    while( oper.gcn_tup_cnt--  &&  buf < end )
	    {
		gcu_get_int( buf, &tuple.gcn_type.gcn_l_item );
		buf += gcu_get_str( buf, &tuple.gcn_type.gcn_value );
		gcu_get_int( buf, &tuple.gcn_uid.gcn_l_item );
		buf += gcu_get_str( buf, &tuple.gcn_uid.gcn_value );
		gcu_get_int( buf, &tuple.gcn_obj.gcn_l_item );
		buf += gcu_get_str( buf, &tuple.gcn_obj.gcn_value );
		gcu_get_int( buf, &tuple.gcn_val.gcn_l_item );
		buf += gcu_get_str( buf, &tuple.gcn_val.gcn_value );

		if ( buf > end )  break;

		/*
		** We can hand off one tuple at a time and
		** pretend that this is a continued message.
		*/
		gcn_res_out( (char *)&tuple, sizeof(GCD_GCN_REQ_TUPLE), TRUE );
	    }
	    break;
    }

    return;
}


/*
** Name: gcn_res_out
**
** Description:
**	Display GCN_RESULT info.
**
** Input:
**	buf		GCA message buffer containing GCD formatted message.
**	len		Length of GCD message.
**	continued	FALSE if buffer contains start of new message.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	20-Mar-97 (gordy)
**	    Converted to formatted messages.
*/

static VOID
gcn_res_out( char *buf, i4 len, bool continued )
{
    GCD_GCN_OPER_DATA	*oper = (GCD_GCN_OPER_DATA *)buf;
    GCD_GCN_REQ_TUPLE	*tuple;
    i4			op;
    char		*end = buf + len;

    if ( continued )
    {
	/*
	** This must be a GCN_RET result since this
	** is the only type which returns tuples.
	*/
	op = GCN_RET;
	tuple = (GCD_GCN_REQ_TUPLE *)buf;
    }
    else
    {
	op = oper->gcn_op;
	tuple = &oper->gcn_tuple[0];
    }

    switch( op )
    {
	case GCN_ADD:
	    SIprintf( "%d row(s) added to the Server Registry\n",
		      oper->gcn_tup_cnt );
	    break;

	case GCN_DEL:
	    SIprintf( "%d row(s) deleted from the Server Registry\n",
		      oper->gcn_tup_cnt );
	    break;

	case GCN_RET:
	    for( ; (char *)tuple < end; tuple++ )   gcn_row_out( tuple );
	    break;
    }

    return;
}


/*
** Name: gcn_row_out
**
** Description:
**	Display a single GCN result tuple.
**
** Input:
**	tuple		GCD formatted GCN result tuple
** 
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	17-Feb-93 (mrelac) MPE/iX
**	    MPE/iX user names can be up to 17 characters long.  The last two
**	    characters were being truncated.
**	20-Mar-96 (gordy)
**	    Converted to formatted messages.
**	15-Aug-97 (rajus01)
**	    Added GCN_ATTR name queue for security mechanisms. 
*/

static VOID
gcn_row_out( GCD_GCN_REQ_TUPLE *tuple )
{
    char	*pv[5];

    if ( ! STcasecmp( tuple->gcn_type.gcn_value, GCN_NODE ) )
    {
	SIprintf( "%-8s  %-15s", "", tuple->gcn_obj.gcn_value );

	switch( gcu_words( tuple->gcn_val.gcn_value, NULL, pv, ',', 3 ) )
	{
	    case 1:
		SIprintf( " %-15s %-15s\n", "", pv[0] );
		break;

	    case 2:
		SIprintf( " %-15s %-15s\n", pv[1], pv[0] );
		break;

	    case 3:
		SIprintf( " %-15s %-15s %-15s\n", pv[1], pv[0], pv[2] );
		break;
	}
    } 
    else  if ( ! STcasecmp(tuple->gcn_type.gcn_value, GCN_LOGIN ) )
    {
	gcu_words( tuple->gcn_val.gcn_value, NULL, pv, ',', 3 );

#ifdef hp9_mpe
	SIprintf( "%-8s  %-15s %-17s\n", "", tuple->gcn_obj.gcn_value, pv[0] );
#else
	SIprintf( "%-8s  %-15s %-15s\n", "", tuple->gcn_obj.gcn_value, pv[0] );
#endif
    }
    else if (! STcasecmp( tuple->gcn_type.gcn_value, GCN_ATTR ) )
    {
	SIprintf( "%-8s  %-15s", "", tuple->gcn_obj.gcn_value );

	switch( gcu_words( tuple->gcn_val.gcn_value, NULL, pv, ',', 3 ) )
	{
	    case 1:
		SIprintf( " %-20s %-15s\n", "", pv[0] );
		break;

	    case 2:
		SIprintf( " %-20s %-15s\n", pv[0], pv[1] );
		break;
	}
    }
    else
    {
	i4	flag = 0;
	char	*c;

	CVahxl( tuple->gcn_uid.gcn_value, &flag );
	c = (flag & GCN_SOL_FLAG) ? "(sole server)" : "";
	SIprintf( "%-8s  %-15s %s\t%s\n", tuple->gcn_type.gcn_value, 
		  tuple->gcn_obj.gcn_value, tuple->gcn_val.gcn_value, c );
    }

    return;
}


/*
** Name: gcn_err_raw
**
** Description:
**	Converts a GCD formatted GCA_ERROR message into a raw
**	byte stream and calls a callback function for processing.
**
** Input:
**	func		Callback function.
**	buf		GCA message buffer containing GCD formatted message.
**	len		Length of GCD message.
**	continued	FALSE if buffer contains start of new message.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	20-Mar-97 (gordy)
**	    Created.
*/

static VOID
gcn_err_raw( i4  (*func)(), char *buf, i4 len, bool continued )
{
    GCD_E_ELEMENT	*ele;
    char		buffer[ 512 ];
    char		*end = buf + len;
    i4			val;

    /*
    ** Continued message are just arrays of elements.
    */
    ele = continued ? (GCD_E_ELEMENT *)buf 
		    : &((GCD_ER_DATA *)buf)->gca_e_element[0];

    /*
    ** For formatted messages we ignore the element 
    ** count and just read each available element.
    */
    for( ; (char *)ele < end; ele++ )
    {
	/*
	** We build a raw GCA_ERROR message for each
	** element. 
	*/
	buf = buffer;
	val = 1;
	buf += gcu_put_int( buf, val );

	val = ele->gca_id_error;
        buf += gcu_put_int( buf, val );
	val = ele->gca_id_server;
        buf += gcu_put_int( buf, val );
	val = ele->gca_server_type;
        buf += gcu_put_int( buf, val );
	val = ele->gca_severity;
        buf += gcu_put_int( buf, val );
	val = ele->gca_local_error;
        buf += gcu_put_int( buf, val );

	/*
	** We pass at most one error parameter.
	*/
	val = min( 1, ele->gca_l_error_parm );
        buf += gcu_put_int( buf, val );

	if ( val )
	{
	    buf += gcu_put_int( buf, ele->gca_error_parm[0].gca_type );
	    buf += gcu_put_int( buf, ele->gca_error_parm[0].gca_precision );
	    val = min( ele->gca_error_parm[0].gca_l_value, 
		       sizeof( buffer ) - (buf - buffer) - sizeof( val ) );
	    buf += gcu_put_int( buf, val );
	    MEcopy((PTR)ele->gca_error_parm[0].gca_value, val, (PTR)buf);
	    buf += val;
	}

	/*
	** Now make the callback for one error.
	*/
	(*func)( GCA_ERROR, buffer, buf - buffer );
    }

    return;
}


/*
** Name: gcn_err_fmt
**
** Description:
**	Converts a raw byte stream GCA_ERROR message into a formatted
**	GCD structure and calls gcn_err_out() to display message.
**
** Input:
**	buf	Unformatted GCA message buffer.
**	len	Length of GCA message.
**
** Output:
**	None.
**
** Returns
**	VOID
**
** History:
**	20-Mar-97 (gordy)
**	    Created.
*/

static VOID
gcn_err_fmt( char *buf, i4 len )
{
    GCD_ER_DATA		err;
    GCD_DATA_VALUE	dv;
    char		*end = buf + len;
    char		buffer[ 512 ];
    i4			ele_cnt, parm_cnt, val;

    buf += gcu_get_int( buf, &ele_cnt );
    if ( buf > end )  return;

    while( ele_cnt-- )
    {
        buf += gcu_get_int( buf, &val );
	err.gca_e_element[0].gca_id_error = val;
        buf += gcu_get_int( buf, &val );
	err.gca_e_element[0].gca_id_server = val;
        buf += gcu_get_int( buf, &val );
	err.gca_e_element[0].gca_server_type = val;
        buf += gcu_get_int( buf, &val );
	err.gca_e_element[0].gca_severity = val;
        buf += gcu_get_int( buf, &val );
	err.gca_e_element[0].gca_local_error = val;

	if ( buf >= end )  break;
	err.gca_e_element[ 0 ].gca_l_error_parm = 0;
        buf += gcu_get_int( buf, &parm_cnt );

	if ( parm_cnt-- )
	{
	    buf += gcu_get_int( buf, &dv.gca_type );
	    buf += gcu_get_int( buf, &dv.gca_precision );
	    buf += gcu_get_int( buf, &val );
	    dv.gca_l_value = min( val, sizeof( buffer ) );
	    dv.gca_value = buffer;
	    MEcopy( (PTR)buf, dv.gca_l_value, (PTR)buffer );
	    buf += val;

	    if ( buf <= end )
	    {
		/*
		** Ignore additional parameters.
		*/
		err.gca_e_element[ 0 ].gca_l_error_parm = 1;

		while( parm_cnt--  &&  buf < end )
		{
		    buf += gcu_get_int( buf, &val );
		    buf += gcu_get_int( buf, &val );
		    buf += gcu_get_int( buf, &val );
		    buf += val;
		}
	    }
	}

	err.gca_l_e_element = 1;
	err.gca_e_element[ 0 ].gca_error_parm = &dv;

	gcn_err_out( (char *)&err, sizeof( err ), FALSE );
    }

    return;
}


/*
** Name: gcn_err_out
**
** Description:
**	Displays GCA_ERROR messages.
**
** Input:
**	buf		GCA message buffer containing GCD formatted message.
**	len		Length of GCD message.
**	continued	FALSE if buffer contains start of new message.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	20-Mar-97 (gordy)
**	    Converted to formatted messages.
*/

static VOID
gcn_err_out( char *buf, i4 len, bool continued )
{
    GCD_E_ELEMENT	*ele;
    char		*end = buf + len;

    /*
    ** Continued message are just arrays of elements.
    */
    ele = continued ? (GCD_E_ELEMENT *)buf 
		    : &((GCD_ER_DATA *)buf)->gca_e_element[0];

    /*
    ** For formatted messages we ignore the element 
    ** count and just read each available element.
    */
    for( ; (char *)ele < end; ele++ )
	if ( ele->gca_l_error_parm )
	    SIprintf( "%.*s", ele->gca_error_parm[0].gca_l_value, 
		              ele->gca_error_parm[0].gca_value );

    return;
}

