/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cv.h>
# include <er.h>
# include <st.h>
# include <sp.h>
# include <mo.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apiihndl.h>
# include <apitname.h>
# include <apierhnd.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apichndl.c
**
** Description:
**	This file defines the connection handle management functions.
**
**	IIapi_createConnHndl	Create connection handle
**	IIapi_deleteConnHndl	Delete connection handle
**	IIapi_isConnHndl	Is connection handle valid
**	IIapi_getConnHndl	Get connection handle
**	IIapi_setConnParm	Set a connection parameter
**	IIapi_clearConnParm	Cleanup connection parameters
**	IIapi_getConnIdHndl	Get a connection object ID handle.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          terminate API when the connection queue is empty.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	26-Oct-94 (gordy)
**	    General cleanup and minor fixes.
**	14-Nov-94 (gordy)
**	    Cleaning up handling of GCA_MD_ASSOC.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	10-Jan-95 (gordy)
**	    Added IIapi_getConnIdHndl().
**	17-Jan-95 (gordy)
**	    Use our own symbol for default GCA protocol.  
**	    Conditionalized code to support older GCA interface.
**	19-Jan-95 (gordy)
**	    Added support for older TIMEZONE format.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	10-May-95 (gordy)
**	    Fixed up native languague connection parm.  Changed
**	    return type of IIapi_setConnParm().
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Transaction IDs are formatted into strings rather than
**	    sending the binary data structure.
**	13-Jul-95 (gordy)
**	    Fix formatting of transaction ID for 16-bit systems.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	14-mar-96 (lawst01)
**	   Windows 3.1 port changes - added include <cv.h>
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
**	 9-Sep-96 (gordy)
**	    Added support for IIAPI_SP_RUPASS and IIAPI_SP_YEAR_CUTOFF.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	13-Mar-97 (gordy)
**	    Make sure all connection handle resources are freed.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	20-Apr-01 (gordy)
**	    Formatting of XA transaction IDs for parameter 
**	    GCA_XA_RESTART should be byte order insensitive.
**      28-jun-2002 (loera01) Bug 108147
**          Apply mutex protection to connection handle.
**      27-Sep-02 (wansh01)
**          Added support for IIAPI_CP_LOGIN_LOCAL login local.
**	 6-Jun-03 (gordy)
**	    Connection handle is now a MIB object.
**	15-Mar-04 (gordy)
**	    Added IIAPI_CP_INT8_WIDTH.
**	22-Oct-04 (gordy)
**	    Environment handle must now be provided as input to
**	    IIapi_createConnHndl().
**	12-Jan-04 (wansh01)
** 	    Freeing connHndl->ch_target_display if it exists.		
**	 8-Aug-07 (gordy)
**	    Added IIAPI_CP_DATE_ALIAS.
**	25-Mar-10 (gordy)
**	    Added message buffer queues to connection handle.
*/




/*
** Name: IIapi_createConnHndl
**
** Description:
**	This function creates a connection handle.
**
** Input:
**	envHndl		Environment Handle.
**
** Output:
**	None.
**
** Returns:
**      connHndl	Connection handle created, NULL if error.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	26-Oct-94 (gordy)
**	    Only free optional values when actually allocated.
**	14-Nov-94 (gordy)
**	    Cleaning up handling of GCA_MD_ASSOC.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	17-Jan-95 (gordy)
**	    Use our own symbol for default GCA protocol.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
**	 6-Jun-03 (gordy)
**	    Attach connection handle MIB object.
**	22-Oct-04 (gordy)
**	    Environment handle must now be provided.
**	25-Mar-10 (gordy)
**	    Added message buffer queues.
*/

II_EXTERN IIAPI_CONNHNDL*
IIapi_createConnHndl( IIAPI_ENVHNDL *envHndl )
{
    IIAPI_CONNHNDL	*connHndl;
    char		buff[16];
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createConnHndl: create a connection handle, envHndl = %p\n",
	  envHndl );
    
    if ( ! envHndl )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_createConnHndl: no environment handle available\n" );
	return( NULL );
    }

    if ( ! (connHndl = (IIAPI_CONNHNDL *)
		       MEreqmem(0, sizeof (IIAPI_CONNHNDL), TRUE, &status)) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createConnHndl: can't alloc connection handle\n" );
	return( NULL );
    }
    
    /*
    ** Initialize connection handle parameters, provide default values.
    */
    connHndl->ch_type = IIAPI_SMT_SQL;		/* Default to SQL */
    connHndl->ch_header.hd_id.hi_hndlID = IIAPI_HI_CONN_HNDL;
    connHndl->ch_header.hd_delete = FALSE;
    connHndl->ch_header.hd_smi.smi_state = IIAPI_IDLE;
    connHndl->ch_header.hd_smi.smi_sm = 
			IIapi_sm[ connHndl->ch_type ][ IIAPI_SMH_CONN ];

    if ( MUi_semaphore( &connHndl->ch_header.hd_sem ) != OK )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createConnHndl: can't create semaphore\n" );
        MEfree( (PTR)connHndl );
	return( NULL );
    }
    connHndl->ch_envHndl= envHndl;
    connHndl->ch_connID = -1;
    connHndl->ch_sizeAdvise = DB_MAXTUP;
    connHndl->ch_partnerProtocol = 0;
    connHndl->ch_callback = FALSE;
    
    QUinit( &connHndl->ch_idHndlList );
    QUinit( &connHndl->ch_tranHndlList );
    QUinit( &connHndl->ch_dbevHndlList );
    QUinit( &connHndl->ch_dbevCBList );
    QUinit( &connHndl->ch_msgQueue );
    QUinit( &connHndl->ch_rcvQueue );
    QUinit( &connHndl->ch_header.hd_id.hi_queue );
    QUinit( &connHndl->ch_header.hd_errorList );
    connHndl->ch_header.hd_errorQue = &connHndl->ch_header.hd_errorList;

    MUp_semaphore( &envHndl->en_semaphore );

    QUinsert( (QUEUE *)connHndl, &envHndl->en_connHndlList );

    MUv_semaphore( &envHndl->en_semaphore );
    
    MOptrout( 0, (PTR)connHndl, sizeof( buff ), buff );
    MOattach( MO_INSTANCE_VAR, IIAPI_MIB_CONN, buff, (PTR)connHndl );

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createConnHndl: connHndl %p created\n", connHndl );
    
    return( connHndl );
}




/*
** Name: IIapi_deleteConnHndl
**
** Description:
**	This function deletes a connection handle.
**
** Input:
**	connHndl  connection handle to be deleted
**
** Output:
**	None.
**
** Returns:
**	II_VOID
**
** History:
**      20-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          terminate API when the connection queue is empty.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	26-Oct-94 (gordy)
**	    Always reset the handle ID to flag deleted handles.
**	    Only free optional values when actually allocated.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	13-Mar-97 (gordy)
**	    Clear the transaction and DB event queues.  Make
**	    sure all resources are freed.
**	14-Oct-98 (rajus01)
**	    Free prompt parameter block.
**	 9-Aug-99 (rajus01)
**	    Check hd_delete flag before removing the handle from
**	    connHndl queue. ( Bug #98303 )
**	 6-Jun-03 (gordy)
**	    Detach connection handle MIB object.
**	25-Mar-10 (gordy)
**	    Added message buffer queues.
*/

II_EXTERN II_VOID
IIapi_deleteConnHndl( IIAPI_CONNHNDL *connHndl )
{
    IIAPI_ENVHNDL	*envHndl;
    char		buff[16];
    QUEUE		*q;

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	("IIapi_deleteConnHndl: delete connection handle %p\n", connHndl);
    
    MOptrout( 0, (PTR)connHndl, sizeof( buff ), buff );
    MOdetach( IIAPI_MIB_CONN, buff );

    /*
    ** This should not fail (in a perfect world).
    */
    if ( (envHndl = IIapi_getEnvHndl( (IIAPI_HNDL *)connHndl )) )
    {
	MUp_semaphore( &envHndl->en_semaphore );
	if( ! connHndl->ch_header.hd_delete )
	    QUremove( (QUEUE *)connHndl );
	MUv_semaphore( &envHndl->en_semaphore );
    }

    for( 
	 q = connHndl->ch_idHndlList.q_next;
	 q != &connHndl->ch_idHndlList;
	 q = connHndl->ch_idHndlList.q_next 
       )
    {
	IIAPI_IDHNDL	*idHndl = (IIAPI_IDHNDL *)q;
	QUremove( &idHndl->id_id.hi_queue );
	IIapi_deleteIdHndl( idHndl );
    }

    for( 
	 q = connHndl->ch_tranHndlList.q_next;
	 q != &connHndl->ch_tranHndlList;
	 q =  connHndl->ch_tranHndlList.q_next 
       )
	IIapi_deleteTranHndl( (IIAPI_TRANHNDL *)q );

    for( 
	 q = connHndl->ch_dbevHndlList.q_next;
	 q != &connHndl->ch_dbevHndlList;
	 q =  connHndl->ch_dbevHndlList.q_next 
       )
	IIapi_deleteDbevHndl( (IIAPI_DBEVHNDL *)q );

    /*
    ** DBEV control blocks are deleted in IIapi_processDbevCB()
    ** when all associated DBEV handles have been dispatched
    ** and deleted.  Control blocks are processed from oldest
    ** to newest, so we must scan this queue in the same order.
    */
    for( 
	 q = connHndl->ch_dbevCBList.q_prev;
	 q != &connHndl->ch_dbevCBList;
	 q =  connHndl->ch_dbevCBList.q_prev 
       )
    {
	IIAPI_DBEVCB	*dbevCB = (IIAPI_DBEVCB *)q;

	/*
	** This is a bit tricky.  If the DBEV handle 
	** queue in the control block is empty, we 
	** process the control block which should do 
	** the delete for us.  Otherwise, we delete 
	** the first handle in the DBEV handle queue.  
	** If this is also the last handle in the 
	** queue, the control block will be deleted 
	** along with the DBEV handle.  if this is 
	** not the last handle, we will pick up the 
	** same control block in the outer loop and 
	** continue with the next handle in the DBEV 
	** handle queue.
	*/
	dbevCB->ev_notify = 0;		/* Cancel dispatching */

	if ( IIapi_isQueEmpty( &dbevCB->ev_dbevHndlList ) )
	    IIapi_processDbevCB( dbevCB->ev_connHndl );
	else
	    IIapi_deleteDbevHndl( (IIAPI_DBEVHNDL *)
				  dbevCB->ev_dbevHndlList.q_next );
    }

    while( (q = QUremove( connHndl->ch_rcvQueue.q_next )) )
	MEfree( (PTR)q );

    while( (q = QUremove( connHndl->ch_msgQueue.q_next )) )
	MEfree( (PTR)q );

    if ( connHndl->ch_target )  MEfree( (PTR)connHndl->ch_target );
    if ( connHndl->ch_username )  MEfree( (PTR)connHndl->ch_username );
    if ( connHndl->ch_password )  MEfree( (PTR)connHndl->ch_password );
    if ( connHndl->ch_sessionParm )  IIapi_clearConnParm( connHndl );
    if ( connHndl->ch_target_display )  
	MEfree( (PTR)connHndl->ch_target_display );

    if ( connHndl->ch_prmptParm )
    {
	IIAPI_PROMPTPARM  *prmptParm = connHndl->ch_prmptParm;

	if( prmptParm->pd_message ) MEfree( (PTR)prmptParm->pd_message );
	if( prmptParm->pd_reply )   MEfree( (PTR)prmptParm->pd_reply );
	MEfree( (PTR)prmptParm );
    }

    IIapi_cleanErrorHndl( &connHndl->ch_header );

    MUr_semaphore( &connHndl->ch_header.hd_sem );
    connHndl->ch_header.hd_id.hi_hndlID = ~IIAPI_HI_CONN_HNDL;
    MEfree( (II_PTR)connHndl );

    return;
}




/*
** Name: IIapi_isConnHndl
**
** Description:
**     This function returns TRUE if the input is a valid connection
**     handle.
**
**     connHndl  connection handle to be validated
**
** Return value:
**     status    TRUE if the input is a valid connection handle. 
**               Otherwise, FALSE.
**
** Re-entrancy:
**     This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	26-Oct-94 (gordy)
**	    Removed tracing: callers can decide if trace needed.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN II_BOOL
IIapi_isConnHndl( IIAPI_CONNHNDL *connHndl )
{
    return( connHndl  &&  
	    connHndl->ch_header.hd_id.hi_hndlID == IIAPI_HI_CONN_HNDL );
}




/*
** Name: IIapi_getConnHndl
**
** Description:
**	This function returns the connection handle in the generic handle
**      parameter.
**
** Return value:
**      connHndl = connection handle
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	26-Oct-94 (gordy)
**	    Allow for NULL handles.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
*/

II_EXTERN IIAPI_CONNHNDL *
IIapi_getConnHndl( IIAPI_HNDL *handle )
{
    IIAPI_DBEVHNDL *dbevHndl;

    if ( handle )
    {
	switch( handle->hd_id.hi_hndlID )
	{
	    case IIAPI_HI_CONN_HNDL: 
		break;

	    case IIAPI_HI_TRAN_HNDL:
		handle = (IIAPI_HNDL *)
			 ((IIAPI_TRANHNDL *)handle)->th_connHndl;
		break;

	    case IIAPI_HI_STMT_HNDL:
		handle = (IIAPI_HNDL *)
			 ((IIAPI_STMTHNDL *)handle)->sh_tranHndl->th_connHndl;
		break;

	    case IIAPI_HI_DBEV_HNDL:
		dbevHndl = (IIAPI_DBEVHNDL *)handle;

		if ( IIapi_isConnHndl((IIAPI_CONNHNDL *)dbevHndl->eh_parent) )
		    handle = (IIAPI_HNDL *)dbevHndl->eh_parent;
		else
		{
		    IIAPI_DBEVCB *dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;
		    handle = (IIAPI_HNDL *)dbevCB->ev_connHndl;
		}
		break;

	    default:
		handle = NULL;
		break;
	}
    }

    return( (IIAPI_CONNHNDL *)handle );
}




/*
** Name: IIapi_setConnParm
**
** Description:
**	Internal routine for IIapi_setConnectParam() to actually
**	save the connection parameter in the connection handle.
**
**	connHndl
**	parmID		Connection Parameter ID
**	parmValue	Pointer to parameter value, type specific to parmID
**	sQLState	Pointer to the SQLSTATE buffer for error output
**	
** Returns:
**
**	IIAPI_STATUS	IIAPI_ST_SUCCESS
**			IIAPI_ST_OUT_OF_MEMORY
**			IIAPI_ST_FAILURE - bad parameter
**
** History:
**	14-Nov-94 (gordy)
**	    Created.
**	18-Nov-94 (gordy)
**	    Made IIAPI_CP_RESTART a local parameter.
**	17-Jan-95 (gordy)
**	    Conditionalized code to support older GCA interface.
**	19-Jan-95 (gordy)
**	    Added support for older TIMEZONE format.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	10-May-95 (gordy)
**	    Fixed up native languague connection parm.  Changed
**	    return type of IIapi_setConnParm().
**	14-Jun-95 (gordy)
**	    Transaction IDs are formatted into strings rather than
**	    sending the binary data structure.
**	13-Jul-95 (gordy)
**	    Fix formatting of transaction ID for 16-bit systems.
**	 9-Sep-96 (gordy)
**	    Added support for IIAPI_SP_RUPASS and IIAPI_SP_YEAR_CUTOFF.
**	18-jun-1997 (kosma01)
**	    From the comments, the conversion should go from a integer to a 
**	    string, for xaXID->xa_branchSeqnum and xaXID->xa_branchFlag.
**	14-Oct-98 (rajus01)
**	    Added support for IIAPI_SP_CAN_PROMPT, IIAPI_SP_SL_TYPE.
**	20-Apr-01 (gordy)
**	    Formatting of XA transaction IDs for parameter GCA_XA_RESTART
**	    should be byte order insensitive (see libqxa!iicxxa.sc).
**      27-Sep-02 (wansh01)
**          set connection handle ch_flag to IIAPI_CH_LT_LOCAL if login
**          type is local. 
**      13-feb-03 (chash01) x-integrate change#461908
**          alpha vms compiler compalins about the complexity of the
**          statement involving variable "data", and claim the meaning 
**          of the statement is undefined.  Change to equivalent statement
**          without using post increment (data++).
**	15-Mar-04 (gordy)
**	    Added IIAPI_CP_INT8_WIDTH.
**	 8-Aug-07 (gordy)
**	    Added IIAPI_CP_DATE_ALIAS.
**	11-Aug-08 (gordy)
**	    Fixed string termination.
*/

II_EXTERN IIAPI_STATUS
IIapi_setConnParm( IIAPI_CONNHNDL *connHndl, II_LONG parmID, II_PTR parmValue )
{
    IIAPI_SESSIONPARM	*sessionParm;
    IIAPI_TRANNAME	*tranName;
    STATUS		status;
    char 		*dbname, *cp, **cpa;
    i4			i;

    if ( ! connHndl->ch_sessionParm  &&
	 ! ( connHndl->ch_sessionParm = (IIAPI_SESSIONPARM *)
		 MEreqmem( 0, sizeof( IIAPI_SESSIONPARM ), TRUE, &status ) ) )
    {
	goto allocFail;
    }

    sessionParm = connHndl->ch_sessionParm;

    switch( parmID )
    {
	case IIAPI_CP_RESTART :
	    tranName = (IIAPI_TRANNAME *)parmValue;
	
	    if ( tranName->tn_tranID.ti_type == IIAPI_TI_IIXID )
	    {
		char	buffer[ 33 ];

		/*
		** The 8 byte Ingres transaction ID is sent
		** in string format as <high>:<low>.
		*/
		STprintf( buffer, "%d:%d",
		    tranName->tn_tranID.ti_value.iiXID.ii_tranID.it_highTran,
		    tranName->tn_tranID.ti_value.iiXID.ii_tranID.it_lowTran );

		if ( sessionParm->sp_restart )
		    MEfree( (PTR)sessionParm->sp_restart );
		if ( ! ( sessionParm->sp_restart = STalloc( buffer ) ) )
		    goto allocFail;
		sessionParm->sp_mask2 |= IIAPI_SP_RESTART;
	    }
	    else
	    {
		IIAPI_XA_DIS_TRAN_ID	*xaXID;
		char			*ptr, buffer[ 512 ];
		u_i4			cnt, val;
		u_i1			*data;

		/*
		** The members of the XA transaction ID are
		** formatted into a character string (some
		** in hex) with ':' separators.  The char
		** data array is represented by a 4 byte
		** integer (in hex) for each group of 4
		** entries.  There is also a field just
		** before the newer branch members which
		** is ignored by the server.
		*/
		xaXID = &tranName->tn_tranID.ti_value.xaXID;
		ptr = buffer;

		CVlx( xaXID->xa_tranID.xt_formatID, ptr );
		ptr += STlength( ptr );

		*(ptr++) = ':';
		CVla( xaXID->xa_tranID.xt_gtridLength, ptr );
		ptr += STlength( ptr );

		*(ptr++) = ':';
		CVla( xaXID->xa_tranID.xt_bqualLength, ptr );
		ptr += STlength( ptr );

		data = (u_i1 *)&xaXID->xa_tranID.xt_data; 
		cnt = (xaXID->xa_tranID.xt_gtridLength +
		       xaXID->xa_tranID.xt_bqualLength + sizeof(i4) - 1)
		      / sizeof(i4);

		while( cnt-- )
		{
		    val = (*(data) << 24) | (*(data+1) << 16) |
			  (*(data+2) << 8)  | *(data+3);
                    data +=4;
		    *(ptr++) = ':';
		    CVlx( val, ptr );
		    ptr += STlength( ptr );
		}

		STcopy( ":XA", ptr );
		ptr += 3;

		*(ptr++) = ':';
		CVna( xaXID->xa_branchSeqnum, ptr );
		ptr += STlength( ptr );

		*(ptr++) = ':';
		CVna( xaXID->xa_branchFlag, ptr );
		ptr += STlength( ptr );

		STcopy( ":EX", ptr );

		if ( sessionParm->sp_xa_restart )
		    MEfree( (PTR)sessionParm->sp_xa_restart );
		if ( ! ( sessionParm->sp_xa_restart = STalloc( buffer ) ) )
		    goto allocFail;
		sessionParm->sp_mask2 |= IIAPI_SP_XA_RESTART;
	    }
	    break;

	case IIAPI_CP_EFFECTIVE_USER :
	    if ( sessionParm->sp_euser )
		MEfree( (PTR)sessionParm->sp_euser );
	    if ( ! ( sessionParm->sp_euser = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_EUSER;
	    break;

	case IIAPI_CP_QUERY_LANGUAGE :
	    sessionParm->sp_qlanguage = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_QLANGUAGE;
	    break;

	case IIAPI_CP_SL_TYPE :
	    sessionParm->sp_sl_type = *(II_LONG *)parmValue;
	    sessionParm->sp_mask2 |= IIAPI_SP_SL_TYPE;
	    break;

	case IIAPI_CP_CAN_PROMPT :
	    sessionParm->sp_can_prompt = *(II_LONG *)parmValue;
	    sessionParm->sp_mask2 |= IIAPI_SP_CAN_PROMPT;
	    break;

	case IIAPI_CP_DATABASE :
	    /*
	    ** Skip vnode before saving database name.
	    */
	    dbname = (char *)parmValue;
	    while( ( cp = STchr( dbname, ':') )  &&  *(cp + 1) == ':' )
		dbname = cp + 2;
		
	    if ( sessionParm->sp_dbname )
		MEfree( (PTR)sessionParm->sp_dbname );
	    if ( ! ( sessionParm->sp_dbname = STalloc( dbname ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_DBNAME;

	    /*
	    ** Now drop the server class.
	    */
	    if ( ( cp = STchr(sessionParm->sp_dbname, '/') ) )
		*cp = EOS;
	    break;

	case IIAPI_CP_CHAR_WIDTH :
	    sessionParm->sp_cwidth = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_CWIDTH;
	    break;

	case IIAPI_CP_TXT_WIDTH :
	    sessionParm->sp_twidth = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_TWIDTH;
	    break;

	case IIAPI_CP_INT1_WIDTH :
	    sessionParm->sp_i1width = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_I1WIDTH;
	    break;

	case IIAPI_CP_INT2_WIDTH :
	    sessionParm->sp_i2width = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_I2WIDTH;
	    break;

	case IIAPI_CP_INT4_WIDTH :
	    sessionParm->sp_i4width = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_I4WIDTH;
	    break;

	case IIAPI_CP_INT8_WIDTH :
	    sessionParm->sp_i8width = *(II_LONG *)parmValue;
	    sessionParm->sp_mask2 |= IIAPI_SP_I8WIDTH;
	    break;

	case IIAPI_CP_FLOAT4_WIDTH :
	    sessionParm->sp_f4width = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_F4WIDTH;
	    break;

	case IIAPI_CP_FLOAT8_WIDTH :
	    sessionParm->sp_f8width = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_F8WIDTH;
	    break;

	case IIAPI_CP_FLOAT4_PRECISION :
	    sessionParm->sp_f4precision = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_F4PRECISION;
	    break;

	case IIAPI_CP_FLOAT8_PRECISION :
	    sessionParm->sp_f8precision = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_F8PRECISION;
	    break;

	case IIAPI_CP_MONEY_PRECISION :
	    sessionParm->sp_mprec = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_MPREC;
	    break;

	case IIAPI_CP_FLOAT4_STYLE :
	    if ( sessionParm->sp_f4style )
		MEfree( (PTR)sessionParm->sp_f4style );
	    if ( ! ( sessionParm->sp_f4style = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_F4STYLE;
	    break;

	case IIAPI_CP_FLOAT8_STYLE :
	    if ( sessionParm->sp_f8style )
		MEfree( (PTR)sessionParm->sp_f8style );
	    if ( ! ( sessionParm->sp_f8style = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_F8STYLE;
	    break;

	case IIAPI_CP_NUMERIC_TREATMENT :
	    if ( sessionParm->sp_decfloat )
		MEfree( (PTR)sessionParm->sp_decfloat );
	    if ( ! ( sessionParm->sp_decfloat = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_DECFLOAT;
	    break;

	case IIAPI_CP_MONEY_SIGN :
	    if ( sessionParm->sp_msign )
		MEfree( (PTR)sessionParm->sp_msign );
	    if ( ! ( sessionParm->sp_msign = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_MSIGN;
	    break;

	case IIAPI_CP_MONEY_LORT :
	    sessionParm->sp_mlort = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_MLORT;
	    break;

	case IIAPI_CP_DECIMAL_CHAR :
	    if ( sessionParm->sp_decimal )
		MEfree( (PTR)sessionParm->sp_decimal );
	    if ( ! ( sessionParm->sp_decimal = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_DECIMAL;
	    break;

	case IIAPI_CP_MATH_EXCP :
	    if ( sessionParm->sp_mathex )
		MEfree( (PTR)sessionParm->sp_mathex );
	    if ( ! ( sessionParm->sp_mathex = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_MATHEX;
	    break;

	case IIAPI_CP_STRING_TRUNC :
	    if ( sessionParm->sp_strtrunc )
		MEfree( (PTR)sessionParm->sp_strtrunc );
	    if ( ! ( sessionParm->sp_strtrunc = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask2 |= IIAPI_SP_STRTRUNC;
	    break;

	case IIAPI_CP_DATE_FORMAT :
	    sessionParm->sp_date_format = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_DATE_FORMAT;
	    break;

	case IIAPI_CP_TIMEZONE_OFFSET :
	    sessionParm->sp_timezone = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_TIMEZONE;
	    break;

	case IIAPI_CP_TIMEZONE :
	    /*
	    ** We are not assured of knowing the GCA protocol level
	    ** at this point since parameters can be set prior to
	    ** a connection being established.  We will save the
	    ** timezone name and adjust for the protocol level
	    ** when the GCA_MD_ASSOC message is built.
	    */
	    if ( sessionParm->sp_timezone_name )
		MEfree( (PTR)sessionParm->sp_timezone_name );
	    if (!(sessionParm->sp_timezone_name = STalloc((char *)parmValue)))
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_TIMEZONE_NAME;
	    break;

	case IIAPI_CP_SECONDARY_INX :
	    if ( sessionParm->sp_idx_struct )
		MEfree( (PTR)sessionParm->sp_idx_struct );
	    if ( ! ( sessionParm->sp_idx_struct = STalloc((char *)parmValue) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_IDX_STRUCT;
	    break;

	case IIAPI_CP_RESULT_TBL :
	    if ( sessionParm->sp_res_struct )
		MEfree( (PTR)sessionParm->sp_res_struct );
	    if ( ! ( sessionParm->sp_res_struct = STalloc((char *)parmValue) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_RES_STRUCT;
	    break;

	case IIAPI_CP_SERVER_TYPE :
	    sessionParm->sp_svtype = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_SVTYPE;
	    break;

	case IIAPI_CP_NATIVE_LANG :
	    if ( ERlangcode( parmValue, &i ) != OK )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_setConnParm: unknown language\n" );
		return( IIAPI_ST_FAILURE );
	    }
	    else
	    {
		sessionParm->sp_nlanguage = i;
		sessionParm->sp_mask1 |= IIAPI_SP_NLANGUAGE;
	    }
	    break;

	case IIAPI_CP_NATIVE_LANG_CODE :
	    sessionParm->sp_nlanguage = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_NLANGUAGE;
	    break;

	case IIAPI_CP_APPLICATION :
	    sessionParm->sp_application = *(II_LONG *)parmValue;
	    sessionParm->sp_mask1 |= IIAPI_SP_APPLICATION;
	    break;

	case IIAPI_CP_APP_ID :
	    if ( sessionParm->sp_aplid )
		MEfree( (PTR)sessionParm->sp_aplid );
	    if ( ! ( sessionParm->sp_aplid = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_APLID;
	    break;

	case IIAPI_CP_GROUP_ID :
	    if ( sessionParm->sp_grpid )
		MEfree( (PTR)sessionParm->sp_grpid );
	    if ( ! ( sessionParm->sp_grpid = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask1 |= IIAPI_SP_GRPID;
	    break;

	case IIAPI_CP_EXCLUSIVE_SYS_UPDATE :
	    /*
	    ** Convert Boolean user value to GCA_ON or GCA_OFF.
	    */
	    if ( *(II_BOOL *)parmValue )
		sessionParm->sp_xupsys = GCA_ON;
	    else
		sessionParm->sp_xupsys = GCA_OFF;
	    sessionParm->sp_mask2 |= IIAPI_SP_XUPSYS;
	    break;

	case IIAPI_CP_SHARED_SYS_UPDATE :
	    /*
	    ** Convert Boolean user value to GCA_ON or GCA_OFF.
	    */
	    if ( *(II_BOOL *)parmValue )
		sessionParm->sp_supsys = GCA_ON;
	    else
		sessionParm->sp_supsys = GCA_OFF;
	    sessionParm->sp_mask2 |= IIAPI_SP_SUPSYS;
	    break;

	case IIAPI_CP_EXCLUSIVE_LOCK :
	    /*
	    ** Convert Boolean user value to GCA form.
	    ** For GCA, send the parameter for exclusive
	    ** lock (value ignored), don't send otherwise.
	    */
	    if ( *(II_BOOL *)parmValue )
	    {
		sessionParm->sp_exclusive = 0;
		sessionParm->sp_mask1 |= IIAPI_SP_EXCLUSIVE;
	    }
	    else
	    {
		sessionParm->sp_mask1 &= ~IIAPI_SP_EXCLUSIVE;
	    }
	    break;

	case IIAPI_CP_WAIT_LOCK :
	    /*
	    ** Convert Boolean user value to GCA_ON or GCA_OFF.
	    */
	    if ( *(II_BOOL *)parmValue )
		sessionParm->sp_wait_lock = GCA_ON;
	    else
		sessionParm->sp_wait_lock = GCA_OFF;
	    sessionParm->sp_mask2 |= IIAPI_SP_WAIT_LOCK;
	    break;

	case IIAPI_CP_MISC_PARM :
	    /*
	    ** Expand size of pointer array and save parameter.
	    */
	    if ( ! ( cpa = (char **)
		     MEreqmem( 0, sizeof(char *) * 
				  (sessionParm->sp_misc_parm_count + 1), 
			       FALSE, &status ) ) )
	       goto allocFail;

	    if ( ! ( cpa[ sessionParm->sp_misc_parm_count ] = 
		     STalloc( (char *)parmValue ) ) )
	    {
		MEfree( (PTR)cpa );
		goto allocFail;
	    }

	    for( i = 0; i < sessionParm->sp_misc_parm_count; i++ )
		cpa[ i ] = sessionParm->sp_misc_parm[ i ];

	    if ( sessionParm->sp_misc_parm )
		MEfree( (PTR)sessionParm->sp_misc_parm );

	    sessionParm->sp_misc_parm = cpa;
	    sessionParm->sp_misc_parm_count++;
	    sessionParm->sp_mask2 |= IIAPI_SP_MISC_PARM;
	    break;

	case IIAPI_CP_GATEWAY_PARM :
	    /*
	    ** Expand size of pointer array and save parameter.
	    */
	    if ( ! ( cpa = (char **)
		     MEreqmem( 0, sizeof(char *) * 
				  (sessionParm->sp_gw_parm_count + 1), 
			       FALSE, &status ) ) )
	       goto allocFail;

	    if ( ! ( cpa[ sessionParm->sp_gw_parm_count ] = 
		     STalloc( (char *)parmValue ) ) )
	    {
		MEfree( (PTR)cpa );
		goto allocFail;
	    }

	    for( i = 0; i < sessionParm->sp_gw_parm_count; i++ )
		cpa[ i ] = sessionParm->sp_gw_parm[ i ];

	    if ( sessionParm->sp_gw_parm )
		MEfree( (PTR)sessionParm->sp_gw_parm );

	    sessionParm->sp_gw_parm = cpa;
	    sessionParm->sp_gw_parm_count++;
	    sessionParm->sp_mask2 |= IIAPI_SP_GW_PARM;
	    break;

	case IIAPI_CP_DBMS_PASSWORD :
	    if ( sessionParm->sp_rupass )
		MEfree( (PTR)sessionParm->sp_rupass );
	    if ( ! ( sessionParm->sp_rupass = STalloc( (char *)parmValue ) ) )
		goto allocFail;
	    sessionParm->sp_mask2 |= IIAPI_SP_RUPASS;
	    break;

	case IIAPI_CP_CENTURY_BOUNDARY :
	    sessionParm->sp_year_cutoff = *(II_LONG *)parmValue;
	    sessionParm->sp_mask2 |= IIAPI_SP_YEAR_CUTOFF;
	    break;

	case IIAPI_CP_LOGIN_LOCAL  :
	/*    test login type local   */
	     if ( *(II_BOOL *)parmValue )
	    	connHndl->ch_flags  |= IIAPI_CH_LT_LOCAL;
             else	      
	    	connHndl->ch_flags  &= ~IIAPI_CH_LT_LOCAL;
	    break;
	     
	case IIAPI_CP_DATE_ALIAS :
	    if ( sessionParm->sp_date_alias)
		MEfree( (PTR)sessionParm->sp_date_alias );
	    if (!(sessionParm->sp_date_alias = STalloc((char *)parmValue)))
		goto allocFail;
	    sessionParm->sp_mask2 |= IIAPI_SP_DATE_ALIAS;
	    break;

	default :
	    /*
	    ** This should not happen as the application input
	    ** should be verified in IIapi_setConnectParam(),
	    ** and all other calls are API internal.  
	    */
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_setConnParm: API ERROR - invalid conn parm ID\n" );
	    return( IIAPI_ST_FAILURE );
	    break;
    }

    return( IIAPI_ST_SUCCESS );

  allocFail :

    IIAPI_TRACE( IIAPI_TR_FATAL )
	( "IIapi_setConnParm: memory allocation failed\n" );

    return( IIAPI_ST_OUT_OF_MEMORY );
}





/*
** Name: IIapi_clearConnParm
**
** Description:
**	Free memory allocated for connection parameters and
**	reset the parameter flags.
**
** History:
**	14-Nov-94 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_clearConnParm( IIAPI_CONNHNDL *connHndl )
{
    IIAPI_SESSIONPARM	*sessionParm = connHndl->ch_sessionParm;
    i4			i;

    if ( ! sessionParm )  return;

    if ( sessionParm->sp_restart )
    {
	MEfree( (PTR)sessionParm->sp_restart );
	sessionParm->sp_restart = NULL;
    }

    if ( sessionParm->sp_restart )
    {
	MEfree( (PTR)sessionParm->sp_xa_restart );
	sessionParm->sp_xa_restart = NULL;
    }

    if ( sessionParm->sp_euser )
    {
	MEfree( (PTR)sessionParm->sp_euser );
	sessionParm->sp_euser = NULL;
    }

    if ( sessionParm->sp_dbname )
    {
	MEfree( (PTR)sessionParm->sp_dbname );
	sessionParm->sp_dbname = NULL;
    }

    if ( sessionParm->sp_f4style )
    {
	MEfree( (PTR)sessionParm->sp_f4style );
	sessionParm->sp_f4style = NULL;
    }

    if ( sessionParm->sp_f8style )
    {
	MEfree( (PTR)sessionParm->sp_f8style );
	sessionParm->sp_f8style = NULL;
    }

    if ( sessionParm->sp_decfloat )
    {
	MEfree( (PTR)sessionParm->sp_decfloat );
	sessionParm->sp_decfloat = NULL;
    }

    if ( sessionParm->sp_msign )
    {
	MEfree( (PTR)sessionParm->sp_msign );
	sessionParm->sp_msign = NULL;
    }

    if ( sessionParm->sp_decimal )
    {
	MEfree( (PTR)sessionParm->sp_decimal );
	sessionParm->sp_decimal = NULL;
    }

    if ( sessionParm->sp_mathex )
    {
	MEfree( (PTR)sessionParm->sp_mathex );
	sessionParm->sp_mathex = NULL;
    }

    if ( sessionParm->sp_strtrunc )
    {
	MEfree( (PTR)sessionParm->sp_strtrunc );
	sessionParm->sp_strtrunc = NULL;
    }

    if ( sessionParm->sp_timezone_name )
    {
	MEfree( (PTR)sessionParm->sp_timezone_name );
	sessionParm->sp_timezone_name = NULL;
    }

    if ( sessionParm->sp_idx_struct )
    {
	MEfree( (PTR)sessionParm->sp_idx_struct );
	sessionParm->sp_idx_struct = NULL;
    }

    if ( sessionParm->sp_res_struct )
    {
	MEfree( (PTR)sessionParm->sp_res_struct );
	sessionParm->sp_res_struct = NULL;
    }

    if ( sessionParm->sp_aplid )
    {
	MEfree( (PTR)sessionParm->sp_aplid );
	sessionParm->sp_aplid = NULL;
    }

    if ( sessionParm->sp_grpid )
    {
	MEfree( (PTR)sessionParm->sp_grpid );
	sessionParm->sp_grpid = NULL;
    }

    if ( sessionParm->sp_misc_parm )
    {
	for( i = 0; i < sessionParm->sp_misc_parm_count; i++ )
	    MEfree( (PTR)sessionParm->sp_misc_parm[ i ] );
	MEfree( (PTR)sessionParm->sp_misc_parm );
	sessionParm->sp_misc_parm = NULL;
    }

    if ( sessionParm->sp_gw_parm )
    {
	for( i = 0; i < sessionParm->sp_gw_parm_count; i++ )
	    MEfree( (PTR)sessionParm->sp_gw_parm[ i ] );
	MEfree( (PTR)sessionParm->sp_gw_parm );
	sessionParm->sp_gw_parm = NULL;
    }

    MEfree( (PTR)sessionParm );
    connHndl->ch_sessionParm = NULL;

    return;
}


/*
** Name: IIapi_getConnIdHndl
**
** Description:
**	Obtain an ID Handle for a GCA_ID associated with the
**	connection.  Creates a new ID Handle if the GCA_ID
**	has not yet been defined.  Returns NULL if unable to
**	create a new ID Handle.
**
** History:
**	10-Jan-95 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

II_EXTERN IIAPI_IDHNDL *
IIapi_getConnIdHndl(IIAPI_CONNHNDL *connHndl, II_LONG type,  GCA_ID *gcaID)
{
    IIAPI_IDHNDL	*idHndl;

    for( idHndl = (IIAPI_IDHNDL *)connHndl->ch_idHndlList.q_next;
	 idHndl != (IIAPI_IDHNDL *)&connHndl->ch_idHndlList;
	 idHndl = (IIAPI_IDHNDL *)idHndl->id_id.hi_queue.q_next )
    {
	if ( IIapi_isIdHndl( idHndl, type )  &&
	     ! MEcmp( (II_PTR)&idHndl->id_gca_id, 
		      (II_PTR)gcaID, sizeof( GCA_ID ) ) )
	    return( idHndl );
    }

    if ( ( idHndl = IIapi_createIdHndl( type, gcaID ) ) )
	QUinsert( &idHndl->id_id.hi_queue, &connHndl->ch_idHndlList );

    return( idHndl );
}


