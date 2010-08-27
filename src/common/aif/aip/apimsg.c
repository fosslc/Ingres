/*
** Copyright (c) 2010 Ingres Corporation
*/


# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <cv.h>
# include <gc.h>
# include <me.h>
# include <pc.h>
# include <st.h>
# include <tm.h>
# include <tmtz.h>

# include <iicommon.h>
# include <gca.h>
# include <adf.h>
# include <adp.h>
# include <copy.h>
# include <generr.h>
# include <sqlstate.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apimsg.h>
# include <apidatav.h>
# include <apimisc.h>
# include <apitrace.h>
# include <apige2ss.h>


/*
** Name apimsg.c
**
** Description:
**	This file contains functions which create GCA messages
**	in raw packed byte stream format.
**
**	IIapi_readMsgPrompt	Read GCA Prompt message.
**	IIapi_readMsgQCID	Read GCA Query/cursor Id.
**	IIapi_readMsgDescr	Read GCA Tuple Descriptor message.
**	IIapi_readMsgCopyMap	Read GCA Copy Map message.
**	IIapi_readMsgEvent	Read GCA DB Event message.
**	IIapi_readMsgError	Read GCA Error message.
**	IIapi_readMsgRetProc	Read GCA Return Procedure message.
**	IIapi_readMsgResponse	Read GCA Response message.
**
**	IIapi_createMsgMDAssoc	Create GCA Modify Association message.
**	IIapi_createMsgPRReply	Create GCA Prompt Reply message.
**	IIapi_createMsgSecure	Create GCA Secure message.
**	IIapi_createMsgXA	Create GCA XA message
**	IIapi_createMsgQuery	Create GCA Query message.
**	IIapi_createMsgAck	Create GCA Acknowledge message.
**	IIapi_createMsgAttn	Create GCA Attention message.
**	IIapi_createMsgFetch	Create GCA Fetch message.
**	IIapi_createMsgScroll	Create GCA Fetch with scroll message.
**	IIapi_createMsgClose	Create GCA Close message.
**	IIapi_createMsgResponse	Create GCA Response message.
**	IIapi_createMsgRelease	Create GCA Release message.
**
**	IIapi_beginMessage	Initialize message buffer.
**	IIapi_extendMessage	Queue message buffer & allocate new buffer.
**	IIapi_endMessage	Move current message buffer to queue.
**
**	IIapi_createMsgTuple	Create GCA Tuple message.
**	IIapi_initMsgQuery	Initialize query messages.
**	IIapi_addMsgParams	Append query message parameters.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	06-Apr-10 (gordy)
**	    Include missing header files.  Drop unused local variables.
**	    Cast numeric values to eliminate warning messages.
**	 7-Apr-10 (gordy)
**	    Use intermediate storage when copying GCA four byte integers
**	    into API two byte values.  Fix protocol level for database
**	    procedure output parameters.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	29-Jul-10 (gordy)
**	    Use the actual message buffer size when a buffer is
**	    retrieved from the free queue.
*/


/*
** Query text fragments used to build certain query types.
*/
static char	*api_qt_repeat = "define query ~Q is ";
static char	*api_qt_open = "open ~Q cursor for ";
static char	*api_qt_scroll = "open ~Q scroll cursor for ";
static char	*api_qt_where = " where current of ~Q ";
static char	*api_qt_using = " using ~V ";
static char	*api_qt_marker = ", ~V ";


/*
** Query text control block used to manage the
** building of query text to send to the server.
*/
typedef struct
{
    u_i1	*gdv_ptr;
    i4		seg_max;
    i4		seg_len;

} QTXT_CB;


/*
** Macros for reading fixed sized integers
** which may be split across message buffers.
*/
#define READI4( connHndl, msgBuff, i4ptr )	\
		readBytes( connHndl, msgBuff, sizeof( i4 ), (u_i1 *)i4ptr )

#define	READI2( connHndl, msgBuff, i2ptr )	\
		readBytes( connHndl, msgBuff, sizeof( i2 ), (u_i1 *)i2ptr )



/*
** Local routines.
*/
static	IIAPI_STATUS	readGCAName( IIAPI_CONNHNDL *, 
				     IIAPI_MSG_BUFF **, char ** );
static	IIAPI_STATUS	readGCADesc( IIAPI_CONNHNDL *, 
				     IIAPI_MSG_BUFF **, IIAPI_DESCRIPTOR * );
static	IIAPI_STATUS	readGCADataValue( IIAPI_CONNHNDL *, IIAPI_MSG_BUFF **, 
					  IIAPI_DESCRIPTOR *, IIAPI_DATAVALUE *,
					  i4, u_i1 * );
static	bool		readBytes( IIAPI_CONNHNDL *, 
				   IIAPI_MSG_BUFF **, i4, u_i1 * );
static	II_VOID		flushRecvQueue( IIAPI_CONNHNDL * );
static	II_VOID		formatMDAssocI4( IIAPI_MSG_BUFF *, i4, i4 );
static	II_VOID		formatMDAssocStr( IIAPI_MSG_BUFF *, i4, char * );
static	IIAPI_STATUS	createQuery( IIAPI_STMTHNDL *, bool );
static	IIAPI_STATUS	createExecStmt( IIAPI_STMTHNDL *, bool );
static	IIAPI_STATUS	createExecProc( IIAPI_STMTHNDL *, bool );
static	IIAPI_STATUS	createCursorOpen( IIAPI_STMTHNDL * );
static	IIAPI_STATUS	createCursorUpd( IIAPI_STMTHNDL * );
static	IIAPI_STATUS	createDefRepeat( IIAPI_STMTHNDL * );
static	IIAPI_STATUS	createExecRepeat( IIAPI_STMTHNDL * );
static	IIAPI_STATUS	initProcExec( IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM * );
static	IIAPI_STATUS	initCursorOpen(IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM *);
static	IIAPI_STATUS	initCursorDel( IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM * );
static	IIAPI_STATUS	initCursorUpd( IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM * );
static	IIAPI_STATUS	initRepeatDef( IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM * );
static	IIAPI_STATUS	initRepeatExec(IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM *);
static	IIAPI_STATUS	addQueryParams( IIAPI_STMTHNDL *, 
					IIAPI_PUTPARMPARM *, bool );
static	IIAPI_STATUS	formatGCAQuery( IIAPI_STMTHNDL *, bool );
static	IIAPI_STATUS	formatUsingClause( IIAPI_STMTHNDL *, QTXT_CB * );
static	IIAPI_STATUS	beginQueryText( IIAPI_STMTHNDL *, QTXT_CB * );
static	IIAPI_STATUS	formatQueryText( IIAPI_STMTHNDL *, QTXT_CB *, char * );
static	IIAPI_STATUS	endQueryText( IIAPI_STMTHNDL *, QTXT_CB * );
static	IIAPI_STATUS	formatQueryId( IIAPI_STMTHNDL *, i4, i4, i4, char * );
static	II_VOID		formatGCAId( IIAPI_MSG_BUFF *, GCA_ID * );
static	II_VOID		formatGDV( IIAPI_MSG_BUFF *, GCA_DATA_VALUE * );
static	IIAPI_STATUS	formatNullValue( IIAPI_STMTHNDL *, 
					 IIAPI_DESCRIPTOR *, bool );
static	IIAPI_STATUS	formatDataValue( IIAPI_STMTHNDL *, IIAPI_DESCRIPTOR *, 
					 IIAPI_DATAVALUE *, bool );
static	IIAPI_STATUS	formatLOB( IIAPI_STMTHNDL *, IIAPI_DESCRIPTOR *, 
				   IIAPI_DATAVALUE *, bool );
static	IIAPI_STATUS	formatLOBhdr( IIAPI_STMTHNDL *, i4 );
static	IIAPI_STATUS	formatLOBseg( IIAPI_STMTHNDL *, 
				      IIAPI_DESCRIPTOR *, IIAPI_DATAVALUE * );
static	IIAPI_STATUS	formatLOBend( IIAPI_STMTHNDL * );
static	IIAPI_STATUS	copyData( IIAPI_STMTHNDL *, i4, u_i1 * );
static	IIAPI_STATUS	copyUCS2( IIAPI_STMTHNDL *, i4, u_i1 * );
static	IIAPI_STATUS	fillData( IIAPI_STMTHNDL *, i4, char );
static	IIAPI_STATUS	fillUCS2( IIAPI_STMTHNDL *, i4, UCS2 );
static	IIAPI_STATUS	reserve( IIAPI_STMTHNDL *, i4 );
static	i4		get_tz_offset( char * );
static	bool		dynamic_open( char * );


/*
** Name: IIapi_allocMsgBuffer
**
** Description:
**	Allocates a GCA message buffer for a connection.
**
** Input:
**	handle			Handle associated with connection.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	GCA Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	29-Jul-10 (gordy)
**	    Use the actual message buffer size when a buffer is
**	    retrieved from the free queue.
*/

IIAPI_MSG_BUFF *
IIapi_allocMsgBuffer( IIAPI_HNDL *handle )
{
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl( handle );
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*ptr = NULL;
    i4			size = DB_MAXTUP;

    if ( connHndl )
    {
	if ( (ptr = (u_i1 *)QUremove( connHndl->ch_msgQueue.q_next )) )
	    size = ((IIAPI_MSG_BUFF *)ptr)->size;
	else
	    size = max( size, connHndl->ch_sizeAdvise );
    }

    /*
    ** Allocate the message buffer.  The data buffer is
    ** allocated at the same time and follows the message
    ** buffer.
    */
    if ( ! ptr )
    {
	if ( ! (ptr = (u_i1 *)MEreqmem( 0, sizeof( IIAPI_MSG_BUFF ) + size, 
					FALSE, NULL )) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_allocMsgBuffer: can't allocate buffer size %d\n",
		  size );
	    return( NULL );
	}

	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "IIapi_allocMsgBuffer: allocated buffer %p size %d\n",
	      ptr, size );
    }

    msgBuff = (IIAPI_MSG_BUFF *)ptr;
    QUinit( &msgBuff->queue );
    msgBuff->connHndl = connHndl;
    msgBuff->useCount = 0;
    msgBuff->msgType = 0;
    msgBuff->flags = 0;
    msgBuff->size = size;
    msgBuff->buffer = ptr + sizeof( IIAPI_MSG_BUFF );
    msgBuff->length = 0;
    msgBuff->data = msgBuff->buffer;

    return( msgBuff );
}


/*
** Name: IIapi_freeMsgBuffer
**
** Description:
**	Free GCA message buffer.
**
** Input:
**	msgBuff		GCA Message buffer.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_VOID
IIapi_freeMsgBuffer( IIAPI_MSG_BUFF *msgBuff )
{
    if ( msgBuff->connHndl )
	QUinsert( &msgBuff->queue, msgBuff->connHndl->ch_msgQueue.q_next );
    else
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "IIapi_freeMsgBuffer: free buffer %p\n", msgBuff );
	MEfree( (PTR)msgBuff );
    }

    return;
}


/*
** Name: IIapi_reserveMsgBuffer
**
** Description:
**	Increments the usage counter of a message buffer so 
**	that IIapi_releaseMsgBuffer will not free the buffer 
**	until all reservers have released the buffer.
**
** Input:
**	msgBuff		Message buffer to reserve.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_reserveMsgBuffer( IIAPI_MSG_BUFF *msgBuff )
{
    msgBuff->useCount++;
    return;
}


/*
** Name: IIapi_releaseMsgBuffer
**
** Description:
**	Decrements the usage counter of a message buffer structure.
**	Buffer is freed When it is no longer used The buffer may no 
**	longer be valid once this function returns.
**
** Input:
**	msgBuff		Message buffer to be released.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_releaseMsgBuffer( IIAPI_MSG_BUFF *msgBuff )
{
    if ( --msgBuff->useCount <= 0 )  IIapi_freeMsgBuffer( msgBuff );
    return;
}


/*
** Name: IIapi_readMsgPrompt
**
** Description:
**	Read a GCA_PROMPT message data object.  The GCA_DATA_VALUE
**	array in the message is not returned directly.  If there
**	is a CHAR data value in the message then a reference to the
**	string in the buffer will be returned.
**
** Input:
**	msgBuff		GCA Message buffer.
**
** Output:
**	promptData	GCA Prompt data.
**	length		Length of prompt text.
**	text		Prompt text.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgPrompt( IIAPI_MSG_BUFF *msgBuff, GCA_PROMPT_DATA *promptData,
		     i4 *length, char **text )
{
    i4	i, size = sizeof( i4 ) * 4;

    if ( msgBuff->length < size )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgPrompt: data too short (%d of %d bytes)\n",
	      msgBuff->length, size );

	return( IIAPI_ST_FAILURE );
    }

    *length = 0;
    *text = NULL;

    I4ASSIGN_MACRO( *msgBuff->data, promptData->gca_prflags );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    I4ASSIGN_MACRO( *msgBuff->data, promptData->gca_prtimeout );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    I4ASSIGN_MACRO( *msgBuff->data, promptData->gca_prmaxlen );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    I4ASSIGN_MACRO( *msgBuff->data, promptData->gca_l_prmesg );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    for( i = 0; i < promptData->gca_l_prmesg; i++ )
    {
	GCA_DATA_VALUE gdv;

	size = sizeof( i4 ) * 3;

	/*
	** If data is not available at this point, simply
	** treat it as no prompt string available.
	*/
	if ( msgBuff->length < size )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgPrompt: data too short (%d of %d bytes)\n",
		  msgBuff->length, size );

	    msgBuff->length = 0;	/* discard unused data */
	    break;
	}

	I4ASSIGN_MACRO( *msgBuff->data, gdv.gca_type );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	I4ASSIGN_MACRO( *msgBuff->data, gdv.gca_precision );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	I4ASSIGN_MACRO( *msgBuff->data, gdv.gca_l_value );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	if ( msgBuff->length < gdv.gca_l_value )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgPrompt: data too short (%d of %d bytes)\n",
		  msgBuff->length, size );

	    msgBuff->length = 0;	/* discard unused data */
	    break;
	}

	if ( gdv.gca_type == DB_CHA_TYPE )
	{
	    *length = gdv.gca_l_value;
	    *text = msgBuff->data;
	}

        msgBuff->data += gdv.gca_l_value;
	msgBuff->length -= gdv.gca_l_value;
    }

    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ("IIapi_readMsgPrompt: extra message data (%d bytes)\n", length);

	msgBuff->length = 0;	/* discard unused data */
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: IIapi_readMsgQCID
**
** Description:
**	Read a GCA_QCID message data object.
**
** Input:
**	msgBuff		GCA Message buffer.
**
** Output:
**	gcaId		GCA Query/cursor Id.
**
** Returns:
**	IIAPI_STATUS	Success or failure.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgQCID( IIAPI_MSG_BUFF *msgBuff, GCA_ID *gcaId )
{
    i4 size = (sizeof( i4 ) * 2) + sizeof( gcaId->gca_name );

    if ( msgBuff->length < size )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgQCID: data too short (%d of %d bytes)\n",
	      msgBuff->length, size );

	return( IIAPI_ST_FAILURE );
    }

    I4ASSIGN_MACRO( *msgBuff->data, gcaId->gca_index[0] );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    I4ASSIGN_MACRO( *msgBuff->data, gcaId->gca_index[1] );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    MEcopy( msgBuff->data, sizeof( gcaId->gca_name ), gcaId->gca_name );
    msgBuff->data += sizeof( gcaId->gca_name );
    msgBuff->length -= sizeof( gcaId->gca_name );

    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_readMsgQCID: extra message data (%d bytes)\n",
	      msgBuff->length );

	msgBuff->length = 0;	/* discard unused data */
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** IIapi_readMsgDescr
**
** Description:
**	Read a GCA_TDESCR message data object.  The object 
**	is read from the connection receive queue associated
**	with a statement handle.  The statement handle is
**	updated with the descriptor information.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgDescr( IIAPI_STMTHNDL *stmtHndl )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_MSG_BUFF	*msgBuff;
    i4			val, flags, count;
    IIAPI_STATUS	status = IIAPI_ST_SUCCESS;

    /*
    ** Load initial (likely only) message buffer from the 
    ** connection receive queue.
    **
    ** The following tests should not fail, so simply treat 
    ** these conditions as if no error message exists.
    */
    if ( ! (connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl ))  ||
	 ! (msgBuff = (IIAPI_MSG_BUFF *)QUremove( 
					connHndl->ch_rcvQueue.q_next )) )
	return( IIAPI_ST_FAILURE );

    /*
    ** If the fixed data is all available in the current
    ** message buffer, then it can be extracted more
    ** efficiently than having to worry about data split
    ** across buffers.
    */
    if ( msgBuff->length >= (sizeof( i4 ) * 4) )
    {
	/* gca_tsize */
	I4ASSIGN_MACRO( *msgBuff->data, val );		/* ignored */
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_result_modifier */
	I4ASSIGN_MACRO( *msgBuff->data, flags );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_id_tdscr */
	I4ASSIGN_MACRO( *msgBuff->data, val );		/* ignored */
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_l_col_desc */
	I4ASSIGN_MACRO( *msgBuff->data, count );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );
    }
    else  if ( 
	       ! READI4( connHndl, &msgBuff, &val )  ||	/* ignored */
	       ! READI4( connHndl, &msgBuff, &flags )  ||
	       ! READI4( connHndl, &msgBuff, &val )  ||	/* ignored */
	       ! READI4( connHndl, &msgBuff, &count )
	     )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgDescr: message truncated [1]\n" );

	count = flags = 0;
	status = IIAPI_ST_FAILURE;
    }

    if ( flags & GCA_COMP_MASK )  stmtHndl->sh_flags |= IIAPI_SH_COMP_VARCHAR;

    /*
    ** Update cursor attributes
    */
    if ( stmtHndl->sh_flags & IIAPI_SH_CURSOR )
    {
	stmtHndl->sh_responseData.gq_mask |= IIAPI_GQ_CURSOR;
	stmtHndl->sh_responseData.gq_cursorType = 0;

	if ( flags & GCA_READONLY_MASK )
	    stmtHndl->sh_responseData.gq_readonly = TRUE;
	else
	{
	    stmtHndl->sh_responseData.gq_readonly = FALSE;
	    stmtHndl->sh_responseData.gq_cursorType |= IIAPI_CURSOR_UPDATE;
	}

	if ( flags & GCA_SCROLL_MASK )
	    stmtHndl->sh_responseData.gq_cursorType |= IIAPI_CURSOR_SCROLL;
    }

    if ( count > 0 )
    {
	stmtHndl->sh_colDescriptor = (IIAPI_DESCRIPTOR *)
	    MEreqmem( 0, count * sizeof( IIAPI_DESCRIPTOR ), FALSE, NULL );

	if ( ! stmtHndl->sh_colDescriptor )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgDescr: can't allocate descriptor\n" );

	    count = 0;
	    status = IIAPI_ST_OUT_OF_MEMORY;
	}
    }

    for( 
         stmtHndl->sh_colCount = 0; 
	 stmtHndl->sh_colCount < count; 
	 stmtHndl->sh_colCount++ 
       )
    {
        status = readGCADesc( connHndl, &msgBuff,
			&stmtHndl->sh_colDescriptor[ stmtHndl->sh_colCount] );
	if ( status != IIAPI_ST_SUCCESS )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgDescr: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY)
					      ? "can't allocate name" 
					      : " message truncated [2]" );
	    break;
	}
    }

    /*
    ** Release current message buffer and any remaining
    ** buffers which are queued for processing.
    */
    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_readMsgDescr: extra message data (%d bytes)\n",
	      msgBuff->length );
    }

    IIapi_releaseMsgBuffer( msgBuff );
    flushRecvQueue( connHndl );
    return( status );
}


/*
** Name: IIapi_readMsgCopyMap
**
** Description:
**	Read a GCA copy map data object.  The object is read 
**	from the connection receive queue associated with a 
**	statement handle.  The statement handle is updated 
**	with the descriptor information.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	06-Apr-10 (gordy)
**	    Add cast to quiet compiler warning.
**	 7-Apr-10 (gordy)
**	    Integer sizes which are different in GCA messages and API
**	    structures require intermediate values to ensure the proper
**	    bits of the values are copied.  Doesn't cause a problem on
**	    little-endian platforms as the values are copied in order
**	    so that the extra bytes don't overwrite copied data.  On
**	    big-endian platforms, the low order bytes are lost.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgCopyMap( IIAPI_STMTHNDL *stmtHndl )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_MSG_BUFF	*msgBuff;
    IIAPI_COPYMAP	*map = &stmtHndl->sh_copyMap;
    i4			i, dir, tsize, flags, tid, count;
    bool		ext;
    IIAPI_STATUS	status = IIAPI_ST_SUCCESS;

    map->cp_fileName = NULL;
    map->cp_logName = NULL;
    map->cp_dbmsDescr = NULL;
    map->cp_dbmsCount = 0; 
    map->cp_fileDescr = NULL;
    map->cp_fileCount = 0; 
    stmtHndl->sh_cpyDescriptor = NULL;
    stmtHndl->sh_colDescriptor = NULL;
    stmtHndl->sh_colCount = 0;

    /*
    ** Load initial (likely only) message buffer from the 
    ** connection receive queue.
    **
    ** The following tests should not fail, so simply treat 
    ** these conditions as if no copy map exists.
    */
    if ( ! (connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl ))  ||
	 ! (msgBuff = (IIAPI_MSG_BUFF *)QUremove( 
					connHndl->ch_rcvQueue.q_next )) )
	return( IIAPI_ST_FAILURE );

    ext = (msgBuff->msgType != GCA_C_FROM  &&  msgBuff->msgType != GCA_C_INTO);

    /*
    ** If the fixed data is all available in the current
    ** message buffer, then it can be extracted more
    ** efficiently than having to worry about data split
    ** across buffers.
    */
    if ( msgBuff->length >= (sizeof( i4 ) * 3) )
    {
	/* gca_status_cp */
	I4ASSIGN_MACRO( *msgBuff->data, map->cp_flags );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_direction_cp */
	I4ASSIGN_MACRO( *msgBuff->data, dir );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_maxerrs_cp */
	I4ASSIGN_MACRO( *msgBuff->data, map->cp_errorCount );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );
    }
    else  if ( 
	       ! READI4( connHndl, &msgBuff, &map->cp_flags )  ||	
	       ! READI4( connHndl, &msgBuff, &dir )  ||
	       ! READI4( connHndl, &msgBuff, &map->cp_errorCount )
	     )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgCopyMap: message truncated [1]\n" );

	status = IIAPI_ST_FAILURE;
	goto done;
    }

    map->cp_copyFrom = dir ? TRUE : FALSE;

    status = readGCAName( connHndl, &msgBuff, &map->cp_fileName );
    if ( status != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgCopyMap: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY) 
					    ? "can't allocate file name" 
					    : " message truncated [2]" );
	goto done;
    }

    status = readGCAName( connHndl, &msgBuff, &map->cp_logName );
    if ( status != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgCopyMap: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY) 
					    ? "can't allocate log name" 
					    : " message truncated [3]" );
	goto done;
    }

    if ( msgBuff->length >= (sizeof( i4 ) * 4) )
    {
	/* gca_tsize */
	I4ASSIGN_MACRO( *msgBuff->data, tsize );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_result_modifier */
	I4ASSIGN_MACRO( *msgBuff->data, flags );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_id_tdscr */
	I4ASSIGN_MACRO( *msgBuff->data, tid );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );

	/* gca_l_col_desc */
	I4ASSIGN_MACRO( *msgBuff->data, count );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );
    }
    else  if ( 
	       ! READI4( connHndl, &msgBuff, &tsize )  ||
	       ! READI4( connHndl, &msgBuff, &flags )  ||
	       ! READI4( connHndl, &msgBuff, &tid )  ||
	       ! READI4( connHndl, &msgBuff, &count )
	     )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgCopyMap: message truncated [4]\n" );

	status = IIAPI_ST_FAILURE;
	goto done;
    }

    if ( flags & GCA_COMP_MASK )  stmtHndl->sh_flags |= IIAPI_SH_COMP_VARCHAR;

    if ( count > 0 )
    {
	map->cp_dbmsDescr = (IIAPI_DESCRIPTOR *)
	    MEreqmem( 0, count * sizeof( IIAPI_DESCRIPTOR ), FALSE, NULL );

	if ( ! map->cp_dbmsDescr )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgCopyMap: can't allocate dbms descriptor\n" );

	    status = IIAPI_ST_OUT_OF_MEMORY;
	    goto done;
	}
    }

    stmtHndl->sh_colDescriptor = map->cp_dbmsDescr;

    for( 
         stmtHndl->sh_colCount = map->cp_dbmsCount = 0; 
	 stmtHndl->sh_colCount < count; 
	 stmtHndl->sh_colCount++, map->cp_dbmsCount++
       )
    {
	status = readGCADesc( connHndl, &msgBuff,
			&stmtHndl->sh_colDescriptor[ stmtHndl->sh_colCount ] );

	if ( status != IIAPI_ST_SUCCESS )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL ) ( "IIapi_readMsgCopyMap: %s\n", 
		(status == IIAPI_ST_OUT_OF_MEMORY) ? "can't allocate name" 
						   : " message truncated [5]" );
	    goto done;
	}
    }

    /*
    ** Het-net connections require a descriptor to be
    ** provided with each GCA_TUPLE send request.
    */
    if ( connHndl->ch_flags & IIAPI_CH_DESCR_REQ )
    {
	u_i1	*ptr;
	i4	attlen = (sizeof(i4) * 3) + (sizeof(i2) * 2);
	i4	desclen = (sizeof(i4) * 4) + (attlen * stmtHndl->sh_colCount);
	i4	zero = 0;

	stmtHndl->sh_cpyDescriptor = MEreqmem( 0, desclen, FALSE, NULL );
	if ( ! stmtHndl->sh_cpyDescriptor )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgCopyMap: can't allocate descriptor\n" );

	    status = IIAPI_ST_OUT_OF_MEMORY;
	    goto done;
	}

	ptr = (u_i1 *)stmtHndl->sh_cpyDescriptor;
	I4ASSIGN_MACRO( tsize, *ptr );	/* gca_tsize */
	ptr += sizeof( i4 );
	I4ASSIGN_MACRO( flags, *ptr );	/* gca_result_modifier */
	ptr += sizeof( i4 );
	I4ASSIGN_MACRO( tid, *ptr );	/* gca_id_tdscr */
	ptr += sizeof( i4 );
	I4ASSIGN_MACRO( count, *ptr );	/* gca_l_col_desc */
	ptr += sizeof( i4 );

	for( i = 0; i < count; i++ )
	{
	    GCA_DATA_VALUE	gdv;
	    i2			i2val;

	    IIapi_cnvtDescr2GDV( &stmtHndl->sh_colDescriptor[ i ], &gdv );

	    I4ASSIGN_MACRO( zero, *ptr );		/* db_data */
	    ptr += sizeof( i4 );
	    I4ASSIGN_MACRO( gdv.gca_l_value, *ptr );	/* db_length */
	    ptr += sizeof( i4 );
	    i2val = (i2)gdv.gca_type;			/* db_datatype */
	    I2ASSIGN_MACRO( i2val, *ptr );
	    ptr += sizeof( i2 );
	    i2val = (i2)gdv.gca_precision;		/* db_prec */
	    I2ASSIGN_MACRO( i2val, *ptr );
	    ptr += sizeof( i2 );
	    I4ASSIGN_MACRO( zero, *ptr );		/* gca_l_attname */
	    ptr += sizeof( i4 );
	}
    }

    if ( msgBuff->length >= sizeof( i4 ) )
    {
	/* gca_l_row_desc_cp */
	I4ASSIGN_MACRO( *msgBuff->data, count );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );
    }
    else  if ( ! READI4( connHndl, &msgBuff, &count ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgCopyMap: message truncated [6]\n" );

	status = IIAPI_ST_FAILURE;
	goto done;
    }

    if ( count > 0 )
    {
	map->cp_fileDescr = (IIAPI_FDATADESCR *)
	    MEreqmem( 0, count * sizeof( IIAPI_FDATADESCR ), FALSE, NULL );

	if ( ! map->cp_fileDescr )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgCopyMap: can't allocate file descriptor\n" );

	    status = IIAPI_ST_OUT_OF_MEMORY;
	    goto done;
	}
    }

    for( 
         map->cp_fileCount = 0; 
	 map->cp_fileCount < count; 
	 map->cp_fileCount++
       )
    {
	IIAPI_FDATADESCR	*fd = &map->cp_fileDescr[ map->cp_fileCount ];
	char			name[ GCA_MAXNAME + 1 ];
	i4			type, prec, length, dellen, nullen;
	i4			delim, nullable;
	char			delims[ 2 ];

	fd->fd_name = NULL;
	fd->fd_delimValue = NULL;
	fd->fd_nullValue.dv_value = NULL;

	if ( msgBuff->length >= GCA_MAXNAME )
	{
	    MEcopy( msgBuff->data, GCA_MAXNAME, name );
	    msgBuff->data += GCA_MAXNAME;
	    msgBuff->length -= GCA_MAXNAME;
	}
	else  if ( ! readBytes( connHndl, &msgBuff, GCA_MAXNAME, name ) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgCopyMap: message truncated [7]\n" );

	    status = IIAPI_ST_FAILURE;
	    goto done;
	}

	name[ GCA_MAXNAME ] = EOS;
	STtrmwhite( name );

	if ( ! (fd->fd_name = STalloc( name )) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgCopyMap: can't allocate column name\n" );

	    status = IIAPI_ST_OUT_OF_MEMORY;
	    goto done;
	}

	prec = 0;
	fd->fd_cvPrec = 0;

	if ( msgBuff->length >= (i2)((sizeof( i4 ) * (ext ? 11 : 9)) + 
				     sizeof( delims )) )
	{
	    /* gca_type_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, type );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    if ( ext )
	    {
		/* gca_precision_cp */
		I4ASSIGN_MACRO( *msgBuff->data, prec );
		msgBuff->data += sizeof( i4 );
		msgBuff->length -= sizeof( i4 );
	    }

	    /* gca_length_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, length );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_user_delim_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, delim );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_l_delim_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, dellen );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_delim_cp */
	    MEcopy( msgBuff->data, sizeof( delims ), delims );
	    msgBuff->data += sizeof( delims );
	    msgBuff->length -= sizeof( delims );

	    /* gca_tupmap_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, fd->fd_column );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_cvid_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, fd->fd_funcID );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    if ( ext )
	    {
		/* gca_cvprec_cp */
		I4ASSIGN_MACRO( *msgBuff->data, fd->fd_cvPrec );
		msgBuff->data += sizeof( i4 );
		msgBuff->length -= sizeof( i4 );
	    }

	    /* gca_cvlen_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, fd->fd_cvLen );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_withnull_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, nullable );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_nulllen_cp */
	    I4ASSIGN_MACRO( *msgBuff->data, nullen );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );
	}
	else  if ( 
		   ! READI4( connHndl, &msgBuff, &type )  ||
		   (ext  &&  ! READI4( connHndl, &msgBuff, &prec ))  ||
		   ! READI4( connHndl, &msgBuff, &length )  ||
		   ! READI4( connHndl, &msgBuff, &delim )  ||
		   ! READI4( connHndl, &msgBuff, &dellen )  ||
		   ! readBytes( connHndl, &msgBuff, sizeof(delims), delims ) ||
		   ! READI4( connHndl, &msgBuff, &fd->fd_column )  ||
		   ! READI4( connHndl, &msgBuff, &fd->fd_funcID )  ||
		   (ext  &&  ! READI4( connHndl, &msgBuff, &fd->fd_cvPrec )) ||
		   ! READI4( connHndl, &msgBuff, &fd->fd_cvLen )  ||
		   ! READI4( connHndl, &msgBuff, &nullable )  ||
		   ! READI4( connHndl, &msgBuff, &nullen )
		 )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgCopyMap: message truncated [8]\n" );

	    status = IIAPI_ST_FAILURE;
	    goto done;
	}

	fd->fd_type = (II_INT2)type;
	fd->fd_length = (II_INT2)length;
	fd->fd_prec = (II_INT2)prec;
	fd->fd_delimiter = delim ? TRUE : FALSE;
	fd->fd_delimLength = (II_INT2)dellen;
	fd->fd_nullable = nullable ? TRUE : FALSE;
	fd->fd_nullInfo = (nullable  &&  nullen > 0) ? TRUE : FALSE;

	if ( fd->fd_nullInfo )
	    if ( ! ext )
	    {
		IIapi_cnvtGDV2Descr( connHndl->ch_envHndl, IIAPI_CHR_TYPE, 
				     nullen, 0, &fd->fd_nullDescr );

		if ( ! (fd->fd_nullValue.dv_value = (char *)
		    MEreqmem( 0, fd->fd_nullDescr.ds_length, FALSE, NULL )) )
		{
		    IIAPI_TRACE( IIAPI_TR_FATAL )
			( "IIapi_readMsgCopyMap: can't allocate null value\n" );

		    status = IIAPI_ST_OUT_OF_MEMORY;
		    goto done;
		}

		if ( msgBuff->length >= nullen )
		{
		    IIapi_cnvtGDV2DataValue( &fd->fd_nullDescr, 
					     msgBuff->data, &fd->fd_nullValue);
		    msgBuff->data += nullen;
		    msgBuff->length -= nullen;
		}
		else
		{
		    /*
		    ** The data value must be collected into a single
		    ** buffer for processing.  Use a local storage if
		    ** value will fit.  Otherwise, use dynamic storage.
		    */
		    u_i1 temp[ 256 ];
		    u_i1 *ptr = temp;

		    if ( nullen > sizeof( temp )  &&
			 ! (ptr = MEreqmem( 0, nullen, FALSE, NULL )) )
		    {
			IIAPI_TRACE( IIAPI_TR_FATAL )
			    ("IIapi_readMsgCopyMap: can't allocate storage\n");

			status = IIAPI_ST_OUT_OF_MEMORY;
			goto done;
		    }

		    if ( ! readBytes( connHndl, &msgBuff, nullen, ptr ) )
		    {
			IIAPI_TRACE( IIAPI_TR_FATAL )
			    ( "IIapi_readMsgCopyMap: message truncated [9]\n" );

			if ( ptr != temp )  MEfree( ptr );
			status = IIAPI_ST_FAILURE;
			goto done;
		    }

		    IIapi_cnvtGDV2DataValue( &fd->fd_nullDescr, 
					     ptr, &fd->fd_nullValue);
		    if ( ptr != temp )  MEfree( ptr );
		}
	    }
	    else
	    {
		status = readGCADataValue( connHndl, &msgBuff,
					   &fd->fd_nullDescr, 
					   &fd->fd_nullValue, 0, NULL );
		if ( status != IIAPI_ST_SUCCESS )
		{
		    IIAPI_TRACE( IIAPI_TR_FATAL ) 
			( "IIapi_readMsgCopyMap: %s\n", 
				(status == IIAPI_ST_OUT_OF_MEMORY) 
				? "can't allocate null value" 
				: " message truncated [10]" );
		    goto done;
		}

		while( --nullen )
		{
		    /*
		    ** This should not happen since there should
		    ** only be one null value.  Read and discard
		    ** the additional values.
		    */
		    IIAPI_DESCRIPTOR	desc;
		    IIAPI_DATAVALUE	dv;
		    u_i1		value[ 256 ];

		    status = readGCADataValue( connHndl, &msgBuff, &desc, 
					       &dv, sizeof( value ), value );
		    if ( status != IIAPI_ST_SUCCESS )
		    {
			IIAPI_TRACE( IIAPI_TR_FATAL ) 
			    ( "IIapi_readMsgCopyMap: %s\n", 
				    (status == IIAPI_ST_OUT_OF_MEMORY) 
				    ? "can't allocate null value" 
				    : " message truncated [11]" );
			goto done;
		    }

		    if ( dv.dv_value != value )  MEfree( dv.dv_value );
		}
	    }

	if ( fd->fd_delimLength > 0 )
	{
	    if ( ! (fd->fd_delimValue = (char *)
			MEreqmem( 0, fd->fd_delimLength, FALSE, NULL )) )
	    {
		IIAPI_TRACE( IIAPI_TR_FATAL )
		    ( "IIapi_readMsgCopyMap: can't allocate delimiters\n" );

		status = IIAPI_ST_OUT_OF_MEMORY;
		goto done;
	    }

	    MEcopy( delims, fd->fd_delimLength, fd->fd_delimValue );
	}
    }

    /*
    ** For some reason, the column descriptor never contains
    ** the column names.  Column names are available in the
    ** file descriptors, but only for the subset of columns
    ** being directed to the file (the column descriptor
    ** contains all columns in the table).  Still, this is
    ** better than nothing.  Attempt to coordinate the file
    ** and column descriptors and copy the column names
    ** where possible.
    */
    for( i = 0; i < map->cp_fileCount; i++ )
    {
	IIAPI_FDATADESCR *fd = &map->cp_fileDescr[ i ];

	/*
	** Delimiter entries have a column index of 0 even though
	** they are not related to column 0, so check for the
	** special copy dummy data types.
	*/
	if (
	     fd->fd_type != CPY_DUMMY_TYPE  &&
	     fd->fd_type != CPY_TEXT_TYPE  &&
	     fd->fd_column >= 0  &&
	     fd->fd_column < stmtHndl->sh_colCount
	   )
	{
	    IIAPI_DESCRIPTOR *descriptor = 
			     &stmtHndl->sh_colDescriptor[ fd->fd_column ];

	    /*
	    ** Free the column descriptor name if empty string.
	    */
	    if ( 
	         descriptor->ds_columnName != NULL  &&
	         descriptor->ds_columnName[ 0 ] == EOS
	       )
	    {
	        MEfree( descriptor->ds_columnName );
		descriptor->ds_columnName = NULL;
	    }

	    /*
	    ** If column descriptor name does not exist, 
	    ** copy the file descriptor name.
	    */
	    if (
	         ! descriptor->ds_columnName  &&
	         ! (descriptor->ds_columnName = STalloc( fd->fd_name ))
	       )
	    {
		IIAPI_TRACE( IIAPI_TR_FATAL )
		    ( "IIapi_readMsgCopyMap: can't allocate column name\n" );

		status = IIAPI_ST_OUT_OF_MEMORY;
		goto done;
	    }
	}
    }

  done:

    if ( status != IIAPI_ST_SUCCESS )
    {
	if ( map->cp_fileName )
	{
	    MEfree( map->cp_fileName );
	    map->cp_fileName = NULL;
	}

	if ( map->cp_logName )
	{
	    MEfree( map->cp_logName );
	    map->cp_logName = NULL;
	}

	if ( map->cp_dbmsDescr )
	{
	    for( i = 0; i < map->cp_dbmsCount; i++ )
		if ( map->cp_dbmsDescr[i].ds_columnName )
		    MEfree( map->cp_dbmsDescr[i].ds_columnName );

	    MEfree( map->cp_dbmsDescr );
	    map->cp_dbmsDescr = NULL;
	}

	map->cp_dbmsCount = 0; 
	stmtHndl->sh_colCount = 0;
	stmtHndl->sh_colDescriptor = NULL;

	if ( map->cp_fileDescr )
	{
	    for( i = 0; i < map->cp_fileCount; i++ )
	    {
		if ( map->cp_fileDescr[i].fd_name )  
		    MEfree( map->cp_fileDescr[i].fd_name );

		if ( map->cp_fileDescr[i].fd_delimValue )
		    MEfree( map->cp_fileDescr[i].fd_delimValue );

		if ( map->cp_fileDescr[i].fd_nullValue.dv_value )
		    MEfree( map->cp_fileDescr[i].fd_nullValue.dv_value );
	    }

	    MEfree( map->cp_fileDescr );
	    map->cp_fileDescr = NULL;
	}

	map->cp_fileCount = 0; 

	if ( stmtHndl->sh_cpyDescriptor )  
	{
	    MEfree(stmtHndl->sh_cpyDescriptor);
	    stmtHndl->sh_cpyDescriptor = NULL;
	}
    }

    /*
    ** Release current message buffer and any remaining
    ** buffers which are queued for processing.
    */
    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_readMsgCopyMap: extra message data (%d bytes)\n",
	      msgBuff->length );
    }

    IIapi_releaseMsgBuffer( msgBuff );
    flushRecvQueue( connHndl );
    return( status );
}


/*
** Name: IIapi_readMsgEvent
**
** Description:
**	Read a GCA_EVENT message data object.  The object 
**	is read from the connection receive queue.  The 
**	DB event control block is updated with the event
**	information.
**
** Input:
**	connHndl	Connection handle.
**
** Output:
**	dbevCB		DB Event info.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgEvent( IIAPI_CONNHNDL *connHndl, IIAPI_DBEVCB *dbevCB )
{
    IIAPI_MSG_BUFF	*msgBuff;
    IIAPI_DESCRIPTOR	descriptor;
    i4			count;
    IIAPI_STATUS	status;

    /*
    ** Load initial (likely only) message buffer from the 
    ** connection receive queue.
    **
    ** The following test should not fail, so simply treat 
    ** these conditions as if no error message exists.
    */
    if ( ! (msgBuff = (IIAPI_MSG_BUFF *)QUremove( 
					connHndl->ch_rcvQueue.q_next )) )
	return( IIAPI_ST_FAILURE );

    status = readGCAName( connHndl, &msgBuff, &dbevCB->ev_name );
    if ( status != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgEvent: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY) 
					  ? "can't allocate event name" 
					  : " message truncated [1]" );
	goto done;
    }

    status = readGCAName( connHndl, &msgBuff, &dbevCB->ev_owner );
    if ( status != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgEvent: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY) 
					  ? "can't allocate event owner" 
					  : " message truncated [2]" );
	goto done;
    }

    status = readGCAName( connHndl, &msgBuff, &dbevCB->ev_database );
    if ( status != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgEvent: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY) 
					  ? "can't allocate event database" 
					  : " message truncated [3]" );
	goto done;
    }

    status = readGCADataValue( connHndl, &msgBuff, 
			       &descriptor, &dbevCB->ev_time, 0, NULL );
    if ( status != IIAPI_ST_SUCCESS )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgEvent: %s\n", (status == IIAPI_ST_OUT_OF_MEMORY) 
					  ? "can't allocate event time value" 
					  : " message truncated [4]" );
	goto done;
    }

    if ( descriptor.ds_dataType != IIAPI_DTE_TYPE )
    {
	dbevCB->ev_time.dv_null = TRUE;
	dbevCB->ev_time.dv_length = 0;

	if ( dbevCB->ev_time.dv_value )
	{
	    MEfree( dbevCB->ev_time.dv_value );
	    dbevCB->ev_time.dv_value = NULL;
	}
    }

    if ( msgBuff->length >= sizeof( i4 ) )
    {
	/* gca_l_evvalue */
	I4ASSIGN_MACRO( *msgBuff->data, count );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );
    }
    else  if ( ! READI4( connHndl, &msgBuff, &count ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgEvent: message truncated [5]\n" );

	count = 0;
	status = IIAPI_ST_FAILURE;
    }

    if ( count > 0 )
    {
	if ( ! (dbevCB->ev_descriptor = (IIAPI_DESCRIPTOR *)
		MEreqmem(0, sizeof(IIAPI_DESCRIPTOR) * count, FALSE, NULL))  ||
	     ! (dbevCB->ev_data = (IIAPI_DATAVALUE *)
		MEreqmem(0, sizeof(IIAPI_DATAVALUE) * count, FALSE, NULL)) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgEvent: can't allocate event data desc\n" );

	    count = 0;
	    status = IIAPI_ST_OUT_OF_MEMORY;
	}
    }

    for( dbevCB->ev_count = 0; dbevCB->ev_count < count; dbevCB->ev_count++ )
    {
	status = readGCADataValue( connHndl, &msgBuff, 
				&dbevCB->ev_descriptor[ dbevCB->ev_count ],
				&dbevCB->ev_data[ dbevCB->ev_count ], 0, NULL );

	if ( status != IIAPI_ST_SUCCESS )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL ) ( "IIapi_readMsgEvent: %s\n", 
					    (status == IIAPI_ST_OUT_OF_MEMORY) 
					    ? "can't allocate event data value" 
					    : " message truncated [6]" );
	    break;
        }
    }

  done:

    /*
    ** Release current message buffer and any remaining
    ** buffers which are queued for processing.
    */
    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_readMsgEvent: extra message data (%d bytes)\n",
	      msgBuff->length );
    }

    IIapi_releaseMsgBuffer( msgBuff );
    flushRecvQueue( connHndl );
    return( status );
}


/*
** IIapi_readMsgError
**
** Description:
**	Read a GCA_ERROR message data object.  The object is
**	read from the connection receive queue associated
**	with an API handle.  An error handle is created and
**	saved on the provided handle.
**
** Input:
**	handle		Handle associated with connection and error.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgError( IIAPI_HNDL *handle )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_MSG_BUFF	*msgBuff;
    i4			errCount;
    IIAPI_STATUS	status = IIAPI_ST_SUCCESS;

    /*
    ** Each error element is processed individually.  
    ** The most common case is a single error with
    ** one parameter (error text) which all fits in
    ** a single message buffer.  This case is handled
    ** via local storage.  Any variance requires
    ** dynamic memory to be temporarily allocated.
    */
    IIAPI_SVR_ERRINFO	errInfo;	/* An error */
    IIAPI_DESCRIPTOR	descriptor;	/* Parameter descriptor */
    IIAPI_DATAVALUE	dataValue;	/* Parameter data-value */
    u_i1		value[ 512 ];	/* Parameter value */

    /*
    ** Load initial (likely only) message buffer from the 
    ** connection receive queue.
    **
    ** The following tests should not fail, so simply treat 
    ** these conditions as if no error message exists.
    */
    if ( ! (connHndl = IIapi_getConnHndl( handle ))  ||
	 ! (msgBuff = (IIAPI_MSG_BUFF *)QUremove( 
					connHndl->ch_rcvQueue.q_next )) )
	return( IIAPI_ST_SUCCESS );

    if ( msgBuff->length >= sizeof( i4 ) )
    {
	/* gca_l_e_element */
	I4ASSIGN_MACRO( *msgBuff->data, errCount );
	msgBuff->data += sizeof( i4 );
	msgBuff->length -= sizeof( i4 );
    }
    else  if ( ! READI4( connHndl, &msgBuff, &errCount ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgError: message truncated [1]\n" );

	errCount = 0;
	status = IIAPI_ST_FAILURE;
    }

    while( errCount-- )
    {
	i4 idx, count;

	errInfo.svr_parmCount = 0;

	/*
	** If the fixed data is all available in the current
	** message buffer, then it can be extracted more
	** efficiently than having to worry about data split
	** across buffers.
	*/
	if ( msgBuff->length >= (sizeof( i4 ) * 6) )
	{
	    /* gca_id_error */
	    I4ASSIGN_MACRO( *msgBuff->data, errInfo.svr_id_error );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_id_server */
	    I4ASSIGN_MACRO( *msgBuff->data, errInfo.svr_id_server );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_server_type */
	    I4ASSIGN_MACRO( *msgBuff->data, errInfo.svr_server_type );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_severity */
	    I4ASSIGN_MACRO( *msgBuff->data, errInfo.svr_severity );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_local_error */
	    I4ASSIGN_MACRO( *msgBuff->data, errInfo.svr_local_error );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );

	    /* gca_l_error_parm */
	    I4ASSIGN_MACRO( *msgBuff->data, count );
	    msgBuff->data += sizeof( i4 );
	    msgBuff->length -= sizeof( i4 );
	}
	else  if ( 
		   ! READI4( connHndl, &msgBuff, &errInfo.svr_id_error )  ||
		   ! READI4( connHndl, &msgBuff, &errInfo.svr_id_server )  ||
		   ! READI4( connHndl, &msgBuff, &errInfo.svr_server_type )  ||
		   ! READI4( connHndl, &msgBuff, &errInfo.svr_severity )  ||
		   ! READI4( connHndl, &msgBuff, &errInfo.svr_local_error )  ||
		   ! READI4( connHndl, &msgBuff, &count )
		 )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_readMsgError: message truncated [2]\n" );

	    status = IIAPI_ST_FAILURE;
	    break;
	}

	if ( connHndl->ch_type == IIAPI_SMT_NS )
	{
	    /*
	    ** Name server replaces generic error code
	    ** with the local error code and does not
	    ** support SQLSTATE.
	    */
	    errInfo.svr_local_error = errInfo.svr_id_error;

	    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_60 )
		errInfo.svr_id_error = E_GE80E8_LOGICAL_ERROR;
	    else
		errInfo.svr_id_error = ss_encode(
		    IIapi_gen2IntSQLState( E_GE80E8_LOGICAL_ERROR,
					   errInfo.svr_local_error ) );
	}

	/*
	** Use local storage if at most one parameter value
	** (as expected) is provided.  Otherwise, allocate
	** parameter descriptors and data-values.
	*/
	if ( count <= 1 )
	{
	    errInfo.svr_parmDescr = &descriptor;
	    errInfo.svr_parmValue = &dataValue;
	}
	else
	{
	    errInfo.svr_parmDescr = (IIAPI_DESCRIPTOR *)
		MEreqmem( 0, sizeof( IIAPI_DESCRIPTOR ) * count, FALSE, NULL );

	    if ( ! errInfo.svr_parmDescr )
	    {
		IIAPI_TRACE( IIAPI_TR_FATAL )
		    ( "IIapi_readMsgError: can't allocate descriptor\n" );

		status = IIAPI_ST_OUT_OF_MEMORY;
		break;
	    }

	    errInfo.svr_parmValue = (IIAPI_DATAVALUE *)
		MEreqmem( 0, sizeof( IIAPI_DATAVALUE ) * count, FALSE, NULL );

	    if ( ! errInfo.svr_parmValue )
	    {
		IIAPI_TRACE( IIAPI_TR_FATAL )
		    ( "IIapi_readMsgError: can't allocate data-value\n" );

		MEfree( errInfo.svr_parmDescr );
		status = IIAPI_ST_OUT_OF_MEMORY;
		break;
	    }
	}

	for( ; errInfo.svr_parmCount < count; errInfo.svr_parmCount++ )
	{
	    idx = errInfo.svr_parmCount;

	    /*
	    ** Local storage can be used to hold the parameter
	    ** value if there is only one parameter.
	    */
	    if ( count == 1 )
		status = readGCADataValue( connHndl, &msgBuff,
					   &errInfo.svr_parmDescr[idx],
					   &errInfo.svr_parmValue[idx], 
					   sizeof( value ), value );
	    else
		status = readGCADataValue( connHndl, &msgBuff,
					   &errInfo.svr_parmDescr[idx],
					   &errInfo.svr_parmValue[idx], 
					   0, NULL );

	    if ( status != IIAPI_ST_SUCCESS )  break;
        }

	if ( status == IIAPI_ST_SUCCESS )
	    status = IIapi_serverError( handle, &errInfo );

	/*
	** Free any temporary storage.
	*/
	for( idx = 0; idx < errInfo.svr_parmCount; idx++ )
	    if ( errInfo.svr_parmValue[idx].dv_value != value )
		MEfree( errInfo.svr_parmValue[idx].dv_value );

	if ( errInfo.svr_parmValue != &dataValue )
	    MEfree( errInfo.svr_parmValue );

	if ( errInfo.svr_parmDescr != &descriptor )
	    MEfree( errInfo.svr_parmDescr );

        if ( status != IIAPI_ST_SUCCESS )  break;
    }

    /*
    ** Release current message buffer and any remaining
    ** buffers which are queued for processing.
    */
    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_readMsgError: extra message data (%d bytes)\n",
	      msgBuff->length );
    }

    IIapi_releaseMsgBuffer( msgBuff );
    flushRecvQueue( connHndl );
    return( status );
}


/*
** Name: IIapi_readRetProc
**
** Description:
**	Read a GCA_RETPROC message data object.
**
** Input:
**	msgBuff		GCA Message buffer.
**
** Output:
**	retProc		GCA Return Procedure data.
**
** Returns:
**	IIAPI_STATUS	Success or failure.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgRetProc( IIAPI_MSG_BUFF *msgBuff, GCA_RP_DATA *retProc )
{
    i4 size = (sizeof( i4 ) * 3) + sizeof( retProc->gca_id_proc.gca_name );

    if ( msgBuff->length < size )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgRetProc: data too short (%d of %d bytes)\n",
	      msgBuff->length, size );
	return( IIAPI_ST_FAILURE );
    }

    I4ASSIGN_MACRO( *msgBuff->data, retProc->gca_id_proc.gca_index[0] );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    I4ASSIGN_MACRO( *msgBuff->data, retProc->gca_id_proc.gca_index[1] );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    MEcopy( msgBuff->data, sizeof( retProc->gca_id_proc.gca_name ), 
	    retProc->gca_id_proc.gca_name );
    msgBuff->data += sizeof( retProc->gca_id_proc.gca_name );
    msgBuff->length -= sizeof( retProc->gca_id_proc.gca_name );

    I4ASSIGN_MACRO( *msgBuff->data, retProc->gca_retstat );
    msgBuff->data += sizeof( i4 );
    msgBuff->length -= sizeof( i4 );

    if ( msgBuff->length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_readMsgRetProc: extra message data (%d bytes)\n",
	      msgBuff->length );
	msgBuff->length = 0;	/* discard unused data */
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: IIapi_readMsgResponse
**
** Description:
**	Read a GCA_RESPONSE message data object.  The message data
**	can be optionally read without being consumed.  If the consume
**	flag is FALSE, the message buffer will not be updated and the
**	data can be read again with a subsequent call.
**
** Input:
**	msgBuff		GCA Message buffer.
**	consume		TRUE if data should be consumed.
**
** Output:
**	respData	GCA Response data.
**
** Returns:
**	IIAPI_STATUS	Success or failure.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_readMsgResponse( IIAPI_MSG_BUFF *msgBuff, 
		       GCA_RE_DATA *respData, bool consume )
{
    u_i1	*data = msgBuff->data;
    i4		length = msgBuff->length;
    i4		size = (sizeof( i4 ) * 10) + sizeof( respData->gca_logkey );

    if ( length < size )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_readMsgResponse: data too short (%d of %d bytes)\n",
	      length, size );
	return( IIAPI_ST_FAILURE );
    }

    I4ASSIGN_MACRO( *data, respData->gca_rid );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_rqstatus );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_rowcount );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_rhistamp );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_rlostamp );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_cost );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_errd0 );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_errd1 );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_errd4 );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    I4ASSIGN_MACRO( *data, respData->gca_errd5 );
    data += sizeof( i4 );
    length -= sizeof( i4 );

    MEcopy( data, sizeof(respData->gca_logkey), respData->gca_logkey );
    data += sizeof( respData->gca_logkey );
    length -= sizeof( respData->gca_logkey );

    if ( length )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ("IIapi_readMsgResponse: extra message data (%d bytes)\n", length);
	length = 0;	/* discard unused data */
    }

    if ( consume )
    {
	msgBuff->data = data;
	msgBuff->length = length;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: readGCAName
**
** Description:
**	Reads a GCA_NAME from the message buffer provided.
**	If there isn't sufficient data available in the buffer, 
**	the buffer will be released and replaced by the next 
**	buffer on the receive queue.
**
**	A GCA_NAME consists of a four byte length followed 
**	by a character string.  Storage for the anme is
**	allocated.
**
** Input:
**	connHndl	Connection handle.
**	msgBuff		Current message buffer.
**
** Output:
**	msgBuff		Current message buffer.
**	ptr		Name read.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_OUT_OF_MEMORY if memory allocation fails.
**			IIAPI_ST_FAILURE if insufficient message data.
**			IIAPI_ST_SUCCESS otherwise.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
readGCAName( IIAPI_CONNHNDL *connHndl, IIAPI_MSG_BUFF **msgBuff, char **ptr )
{
    i4 length;

    if ( (*msgBuff)->length >= sizeof( i4 ) )
    {
	/* gca_l_name */
	I4ASSIGN_MACRO( *(*msgBuff)->data, length );
	(*msgBuff)->data += sizeof( i4 );
	(*msgBuff)->length -= sizeof( i4 );
    }
    else  if ( ! READI4( connHndl, msgBuff, &length ) )
    {
	*ptr = NULL;
	return( IIAPI_ST_FAILURE );
    }

    if ( ! (*ptr = (char *)MEreqmem( 0, length + 1, FALSE, NULL )) )
	return( IIAPI_ST_OUT_OF_MEMORY );

    if ( (*msgBuff)->length >= length )
    {
	MEcopy( (*msgBuff)->data, length, *ptr );
	(*msgBuff)->data += length;
	(*msgBuff)->length -= length;
    }
    else  if ( ! readBytes( connHndl, msgBuff, length, *ptr ) )
    {
	MEfree( *ptr );
	*ptr = NULL;
	return( IIAPI_ST_FAILURE );
    }

    (*ptr)[ length ] = EOS;
    return( IIAPI_ST_SUCCESS );
}


/*
** Name: readGCADesc
**
** Description:
**	Reads a GCA tuple descriptor entry from the message buffer 
**	provided and returns a descriptor and the data value.  If 
**	there isn't sufficient data available in the buffer, the 
**	buffer will be released and replaced by the next buffer on 
**	the receive queue.
**
** Input:
**	connHndl	Connection handle.
**	msgBuff		Current message buffer.
**
** Output:
**	msgBuff		Current message buffer.
**	descriptor	Descriptor.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_OUT_OF_MEMORY if memory allocation fails.
**			IIAPI_ST_FAILURE if insufficient message data.
**			IIAPI_ST_SUCCESS otherwise.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
readGCADesc( IIAPI_CONNHNDL *connHndl, 
	     IIAPI_MSG_BUFF **msgBuff, IIAPI_DESCRIPTOR *descriptor )
{
    i4 length, val;
    i2 type, prec;

    /*
    ** If the fixed data is all available in the current
    ** message buffer, then it can be extracted more
    ** efficiently than having to worry about data split
    ** across buffers.
    */
    if ( (*msgBuff)->length >= ((sizeof( i4 ) * 2) + (sizeof( i2 ) * 2)) )
    {
	/* db_data */
	I4ASSIGN_MACRO( *(*msgBuff)->data, val );		/* ignored */
	(*msgBuff)->data += sizeof( i4 );
	(*msgBuff)->length -= sizeof( i4 );

	/* db_length */
	I4ASSIGN_MACRO( *(*msgBuff)->data, length );
	(*msgBuff)->data += sizeof( i4 );
	(*msgBuff)->length -= sizeof( i4 );

	/* db_datatype */
	I2ASSIGN_MACRO( *(*msgBuff)->data, type );
	(*msgBuff)->data += sizeof( i2 );
	(*msgBuff)->length -= sizeof( i2 );

	/* db_prec */
	I2ASSIGN_MACRO( *(*msgBuff)->data, prec );
	(*msgBuff)->data += sizeof( i2 );
	(*msgBuff)->length -= sizeof( i2 );
    }
    else  if ( 
	       ! READI4( connHndl, msgBuff, &val )  ||		/* ignored */
	       ! READI4( connHndl, msgBuff, &length )  ||
	       ! READI2( connHndl, msgBuff, &type )  ||
	       ! READI2( connHndl, msgBuff, &prec )
	     )
    {
	return( IIAPI_ST_FAILURE );
    }

    IIapi_cnvtGDV2Descr( connHndl->ch_envHndl, 
			 (IIAPI_DT_ID)type, length, prec, descriptor );

    return( readGCAName( connHndl, msgBuff, &descriptor->ds_columnName ) );
}


/*
** Name: readGCADataValue
**
** Description:
**	Reads a GCA_DATA_VALUE from the message buffer provided
**	and returns a descriptor and the data value.  If there 
**	isn't sufficient data available in the buffer, the buffer 
**	will be released and replaced by the next buffer on the 
**	receive queue.
**
** Input:
**	connHndl	Connection handle.
**	msgBuff		Current message buffer.
**	size		Length of value.
**	value		Storage for data value.
**
** Output:
**	msgBuff		Current message buffer.
**	descriptor	Data value description.
**	dataValue	Data value.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_OUT_OF_MEMORY if memory allocation fails.
**			IIAPI_ST_FAILURE if insufficient message data.
**			IIAPI_ST_SUCCESS otherwise.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	06-Apr-10 (gordy)
**	    Removed unused variable.
*/

static IIAPI_STATUS
readGCADataValue
( 
    IIAPI_CONNHNDL	*connHndl, 
    IIAPI_MSG_BUFF	**msgBuff, 
    IIAPI_DESCRIPTOR	*descriptor, 
    IIAPI_DATAVALUE	*dataValue, 
    i4			size, 
    u_i1		*value 
)
{
    i4 type, prec, length;

    /*
    ** If the fixed data is all available in the current
    ** message buffer, then it can be extracted more
    ** efficiently than having to worry about data split
    ** across buffers.
    */
    if ( (*msgBuff)->length >= (sizeof( i4 ) * 3) )
    {
	/* gca_type */
	I4ASSIGN_MACRO( *(*msgBuff)->data, type );
	(*msgBuff)->data += sizeof( i4 );
	(*msgBuff)->length -= sizeof( i4 );

	/* gca_precision */
	I4ASSIGN_MACRO( *(*msgBuff)->data, prec );
	(*msgBuff)->data += sizeof( i4 );
	(*msgBuff)->length -= sizeof( i4 );

	/* gca_l_value */
	I4ASSIGN_MACRO( *(*msgBuff)->data, length );
	(*msgBuff)->data += sizeof( i4 );
	(*msgBuff)->length -= sizeof( i4 );
    }
    else  if ( 
	       ! READI4( connHndl, msgBuff, &type )  ||
	       ! READI4( connHndl, msgBuff, &prec )  ||
	       ! READI4( connHndl, msgBuff, &length )
	     )
    {
	return( IIAPI_ST_FAILURE );
    }

    IIapi_cnvtGDV2Descr( connHndl->ch_envHndl, 
			 (IIAPI_DT_ID)type, length, prec, descriptor );

    /*
    ** Use the provided data storage if it will fit.
    ** Otherwise, use dynamic storage.
    */
    if ( value  &&  descriptor->ds_length <= size  )
	dataValue->dv_value = value;
    else
    {
	dataValue->dv_value = MEreqmem(0, descriptor->ds_length, FALSE, NULL);
	if ( ! dataValue->dv_value )  return( IIAPI_ST_OUT_OF_MEMORY );
    }

    if ( (*msgBuff)->length >= length )
    {
	IIapi_cnvtGDV2DataValue( descriptor, (*msgBuff)->data, dataValue );
	(*msgBuff)->data += length;
	(*msgBuff)->length -= length;
    }
    else
    {
	/*
	** The data value must be collected into a single
	** buffer for processing.  Use a local storage if
	** value will fit.  Otherwise, use dynamic storage.
	*/
	u_i1	temp[ 512 ];
	u_i1	*ptr = temp;

	if ( length > sizeof( temp )  &&
	     ! (ptr = (u_i1 *)MEreqmem( 0, length, FALSE, NULL )) )
	{
	    if ( dataValue->dv_value != value )  MEfree( dataValue->dv_value );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}

	if ( ! readBytes( connHndl, msgBuff, length, ptr ) )
	{
	    if ( ptr != temp )  MEfree( ptr );
	    if ( dataValue->dv_value != value )  MEfree( dataValue->dv_value );
	    return( IIAPI_ST_FAILURE );
	}

	/*
	** Process the data value and free dynamic storage.
	*/
	IIapi_cnvtGDV2DataValue( descriptor, ptr, dataValue );
	if ( ptr != temp )  MEfree( ptr );
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: readBytes
**
** Description:
**	Reads the requested number of bytes from the message
**	buffer provided.  If there isn't sufficient data
**	available in the buffer, the buffer will be released
**	and replaced by the next buffer on the receive queue.
**
** Input:
**	connHndl	Connection handle.
**	msgBuff		Current message buffer.
**	length		Number of bytes to read.
**
** Output:
**	msgBuff		Current message buffer.
**	ptr		Bytes read.
**
** Returns:
**	bool		TRUE if all bytes read, FALSE if message truncated.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static bool
readBytes( IIAPI_CONNHNDL *connHndl, 
	   IIAPI_MSG_BUFF **msgBuff, i4 length, u_i1 *ptr )
{
    while( length > 0 )
    {
	i4 len = min( length, (*msgBuff)->length );

	if ( len < 0 )  len = 0;

	switch( len )
	{
	case 4 : *(ptr++) = *((*msgBuff)->data++);
	case 3 : *(ptr++) = *((*msgBuff)->data++);
	case 2 : *(ptr++) = *((*msgBuff)->data++);
	case 1 : *(ptr++) = *((*msgBuff)->data++);
	    (*msgBuff)->length -= len;
	    length -= len;
	    break;

	case 0 :
	    /*
	    ** Current message buffer is empty.  Release the buffer
	    ** and load next queued buffer.  Fail when all buffers
	    ** processed.
	    */
	    IIapi_releaseMsgBuffer( *msgBuff );
	    *msgBuff = (IIAPI_MSG_BUFF *)
				QUremove( connHndl->ch_rcvQueue.q_next );
	    if ( ! *msgBuff )  return( FALSE );
	    break;

	default :
	    MEcopy( (*msgBuff)->data, len, ptr );
	    ptr += len;
	    (*msgBuff)->data += len;
	    (*msgBuff)->length -= len;
	    length -= len;
	    break;
	}
    }

    return( TRUE );
}


/*
** Name: flushRecvQueue
**
** Description:
**	Remove and release all message buffers on the receive queue.
**
** Input:
**	connHndl	Connection handle.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static II_VOID
flushRecvQueue( IIAPI_CONNHNDL *connHndl )
{
    IIAPI_MSG_BUFF *msgBuff;

    while( (msgBuff = (IIAPI_MSG_BUFF *)QUremove( 
					connHndl->ch_rcvQueue.q_next )) )
    {
	if ( msgBuff->length )
	{
	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "Flushing extra message data after processing (%d bytes)\n",
		  msgBuff->length );
	}

	IIapi_releaseMsgBuffer( msgBuff );
    }

    return;
}


/*
** Name: IIapi_createMsgMDAssoc
**
** Description:
**	Create a GCA Modify Association message.  Allocates and 
**	returns a GCA message buffer.
**
** Input:
**	connHndl		Connection handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**      30-sep-93 (ctham)
**          creation
**	 11-apr-94 (edg)
**	    Target name in MD_ASSOC message should have any vnode info
**	    stripped out.
**      11-apr-94 (ctham)
**          all string parameters in GCA_MD_ASSOC should have the
**          data types of IIAPI_CHA_TYPE instead of IIAPI_CHR_TYPE.
**      18-apr-94 (ctham)
**          GCA_RUPASS is not accepted in GCA_MD_ASSOC when
**          the protocol level is less than GCA_PROTOCOL_LEVEL_60,
**          corrected in IIapi_createGCAMdassoc().
**      26-apr-94 (ctham)
**          remove GCA_RUPASS completely from API.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     use hd_memTag for memory allocation.
**	14-Nov-94 (gordy)
**	    Cleanup GCA_MD_ASSOC processing.
**	17-Jan-95 (gordy)
**	    Conditionalized code for older GCA interface.
**	19-Jan-95 (gordy)
**	    Conditionalized code for older timezone interface.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	10-May-95 (gordy)
**	    Make sure the parameter count is correct when some
**	    parms are not sent due to protocol restrictions.
**	14-Jun-95 (gordy)
**	    Transaction IDs now sent as formatted strings.
**	28-Jul-95 (gordy)
**	    Fixed tracing.
**	19-Jan-96 (gordy)
**	    Added global data structure.
**	 8-Mar-96 (gordy)
**	    GCD_SESSION_PARMS is now a variable length data structure
**	    rather than containing a pointer to a variable length array.
**	15-May-96 (gordy)
**	    Use a static location to hold timezone offset when connecting
**	    at a GCA protocol level which doesn't support timezone names.
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
**	 9-Sep-96 (gordy)
**	    Added support for GCA_RUPASS, GCA_YEAR_CUTOFF.
**	16-Dec-99 (gordy)
**	    Make sure param count handles all conditional parameters.
**	23-Mar-01 (gordy)
**	    Need to use timezone selected by application when sending
**	    timezone offset to 6.4 DBMS.
**      28-May-03 (loera01) Bug 110318
**          Do not send GCA_DECIMAL unless the decimal value is a comma.
**	15-Mar-04 (gordy)
**	    Added connection parameter for I8 width.
**	 8-Aug-07 (gordy)
**	    Added connection parameter for date alias.
**	25-Mar-10 (gordy)
**	    Converted to use unformatted GCA message buffers.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgMDAssoc( IIAPI_CONNHNDL	*connHndl )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*lptr;
    i4			parmCnt, i;
    II_LONG		idx, mask;
    IIAPI_SESSIONPARM	*sessionParm = connHndl->ch_sessionParm;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createMsgMDAssoc: creating Modify Assoc message\n" );
    
    /* This is an internal error and should not occur. */
    if ( ! sessionParm )  return( NULL );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	return( NULL );

    msgBuff->msgType = GCA_MD_ASSOC;

    /*
    ** Save reference to array element count.  It will
    ** be updated once the parameter count is known.
    */
    lptr = msgBuff->data + msgBuff->length;
    msgBuff->length += sizeof( i4 );
    parmCnt = 0;
    
    for( 
	 mask = sessionParm->sp_mask1, idx = 0x00000001;
	 mask  &&  idx;
	 mask &= ~idx, idx <<= 1
       )
	switch( mask & idx )
	{
	case IIAPI_SP_EXCLUSIVE :
	    formatMDAssocI4(msgBuff, GCA_EXCLUSIVE, sessionParm->sp_exclusive);
	    parmCnt++;
	    break;

	case IIAPI_SP_VERSION :
	    formatMDAssocI4( msgBuff, GCA_VERSION, sessionParm->sp_version );
	    parmCnt++;
	    break;

	case IIAPI_SP_APPLICATION :
	    formatMDAssocI4( msgBuff, 
			     GCA_APPLICATION, sessionParm->sp_application );
	    parmCnt++;
	    break;

	case IIAPI_SP_QLANGUAGE :
	    formatMDAssocI4(msgBuff, GCA_QLANGUAGE, sessionParm->sp_qlanguage);
	    parmCnt++;
	    break;

	case IIAPI_SP_CWIDTH :
	    formatMDAssocI4( msgBuff, GCA_CWIDTH, sessionParm->sp_cwidth );
	    parmCnt++;
	    break;

	case IIAPI_SP_TWIDTH :
	    formatMDAssocI4( msgBuff, GCA_TWIDTH, sessionParm->sp_twidth );
	    parmCnt++;
	    break;

	case IIAPI_SP_I1WIDTH :
	    formatMDAssocI4( msgBuff, GCA_I1WIDTH, sessionParm->sp_i1width );
	    parmCnt++;
	    break;

	case IIAPI_SP_I2WIDTH :
	    formatMDAssocI4( msgBuff, GCA_I2WIDTH, sessionParm->sp_i2width );
	    parmCnt++;
	    break;

	case IIAPI_SP_I4WIDTH :
	    formatMDAssocI4( msgBuff, GCA_I4WIDTH, sessionParm->sp_i4width );
	    parmCnt++;
	    break;

	case IIAPI_SP_F4WIDTH :
	    formatMDAssocI4( msgBuff, GCA_F4WIDTH, sessionParm->sp_f4width );
	    parmCnt++;
	    break;

	case IIAPI_SP_F4PRECISION :
	    formatMDAssocI4( msgBuff, 
			     GCA_F4PRECISION, sessionParm->sp_f4precision );
	    parmCnt++;
	    break;

	case IIAPI_SP_F8WIDTH :
	    formatMDAssocI4( msgBuff, GCA_F8WIDTH, sessionParm->sp_f8width );
	    parmCnt++;
	    break;

	case IIAPI_SP_F8PRECISION :
	    formatMDAssocI4( msgBuff, 
			     GCA_F8PRECISION, sessionParm->sp_f8precision );
	    parmCnt++;
	    break;

	case IIAPI_SP_SVTYPE :
	    formatMDAssocI4( msgBuff, GCA_SVTYPE, sessionParm->sp_svtype );
	    parmCnt++;
	    break;

	case IIAPI_SP_NLANGUAGE :
	    formatMDAssocI4(msgBuff, GCA_NLANGUAGE, sessionParm->sp_nlanguage);
	    parmCnt++;
	    break;

	case IIAPI_SP_MPREC :
	    formatMDAssocI4( msgBuff, GCA_MPREC, sessionParm->sp_mprec );
	    parmCnt++;
	    break;

	case IIAPI_SP_MLORT :
	    formatMDAssocI4( msgBuff, GCA_MLORT, sessionParm->sp_mlort );
	    parmCnt++;
	    break;

	case IIAPI_SP_DATE_FORMAT :
	    formatMDAssocI4( msgBuff, 
			     GCA_DATE_FORMAT, sessionParm->sp_date_format );
	    parmCnt++;
	    break;

	case IIAPI_SP_TIMEZONE :
	    /*
	    ** Don't send this if at wrong protocol level.
	    */
	    if ( connHndl->ch_partnerProtocol > GCA_PROTOCOL_LEVEL_3  &&
		 connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_60 )
	    {
		formatMDAssocI4( msgBuff, 
				 GCA_TIMEZONE, sessionParm->sp_timezone );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_TIMEZONE_NAME :
	    /*
	    ** Only send this parameter at the correct GCA protocol
	    ** level.  If at lower protocol level and IIAPI_SP_TIMEZONE
	    ** is NOT set, then convert to older protocol.
	    */
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60 )
	    {
		formatMDAssocStr( msgBuff, GCA_TIMEZONE_NAME, 
				  sessionParm->sp_timezone_name );
		parmCnt++;
	    }
	    else  if ( ! ( sessionParm->sp_mask1 & IIAPI_SP_TIMEZONE )  &&
		       connHndl->ch_partnerProtocol > GCA_PROTOCOL_LEVEL_3 )
	    {
		/*
		** We need a static location to hold the
		** timezone info.  Since we checked that
		** IIAPI_SP_TIMEZONE is not currently set,
		** we can use the associated session parm.
		*/
		sessionParm->sp_timezone = 
		    get_tz_offset( sessionParm->sp_timezone_name );

		formatMDAssocI4( msgBuff, 
				 GCA_TIMEZONE, sessionParm->sp_timezone );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_DBNAME :
	    formatMDAssocStr( msgBuff, GCA_DBNAME, sessionParm->sp_dbname );
	    parmCnt++;
	    break;

	case IIAPI_SP_IDX_STRUCT :
	    formatMDAssocStr( msgBuff, 
			      GCA_IDX_STRUCT, sessionParm->sp_idx_struct );
	    parmCnt++;
	    break;

	case IIAPI_SP_RES_STRUCT :
	    formatMDAssocStr( msgBuff, 
			      GCA_RES_STRUCT, sessionParm->sp_res_struct );
	    parmCnt++;
	    break;

	case IIAPI_SP_EUSER :
	    formatMDAssocStr( msgBuff, GCA_EUSER, sessionParm->sp_euser );
	    parmCnt++;
	    break;

	case IIAPI_SP_MATHEX :
	    formatMDAssocStr( msgBuff, GCA_MATHEX, sessionParm->sp_mathex );
	    parmCnt++;
	    break;

	case IIAPI_SP_F4STYLE :
	    formatMDAssocStr( msgBuff, GCA_F4STYLE, sessionParm->sp_f4style );
	    parmCnt++;
	    break;

	case IIAPI_SP_F8STYLE :
	    formatMDAssocStr( msgBuff, GCA_F8STYLE, sessionParm->sp_f8style );
	    parmCnt++;
	    break;

	case IIAPI_SP_MSIGN :
	    formatMDAssocStr( msgBuff, GCA_MSIGN, sessionParm->sp_msign );
	    parmCnt++;
	    break;

	case IIAPI_SP_DECIMAL :
	    if ( ! STbcompare( sessionParm->sp_decimal, 0, ",\0", 0, TRUE ) )
	    {
		formatMDAssocStr( msgBuff, 
				  GCA_DECIMAL, sessionParm->sp_decimal );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_APLID :
	    formatMDAssocStr( msgBuff, GCA_APLID, sessionParm->sp_aplid );
	    parmCnt++;
	    break;

	case IIAPI_SP_GRPID :
	    formatMDAssocStr( msgBuff, GCA_GRPID, sessionParm->sp_grpid );
	    parmCnt++;
	    break;

	case IIAPI_SP_DECFLOAT :
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60 )
	    {
		formatMDAssocStr( msgBuff, 
				  GCA_DECFLOAT, sessionParm->sp_decfloat );
		parmCnt++;
	    }
	    break;
	}

    for( 
	 mask = sessionParm->sp_mask2, idx = 0x00000001;
	 mask  &&  idx;
	 mask &= ~idx, idx <<= 1
       )
	switch( mask & idx )
	{
	case IIAPI_SP_RUPASS :
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_62 )
	    {
		formatMDAssocStr(msgBuff, GCA_RUPASS, sessionParm->sp_rupass);
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_MISC_PARM :
	    for( i = 0; i < sessionParm->sp_misc_parm_count; i++ )
	    {
		formatMDAssocStr(msgBuff, 
				 GCA_MISC_PARM, sessionParm->sp_misc_parm[i]);
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_STRTRUNC :
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60 )
	    {
		formatMDAssocStr( msgBuff, 
				  GCA_STRTRUNC, sessionParm->sp_strtrunc );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_XUPSYS :
	    formatMDAssocI4( msgBuff, GCA_XUPSYS, sessionParm->sp_xupsys );
	    parmCnt++;
	    break;

	case IIAPI_SP_SUPSYS :
	    formatMDAssocI4( msgBuff, GCA_SUPSYS, sessionParm->sp_supsys );
	    parmCnt++;
	    break;

	case IIAPI_SP_WAIT_LOCK :
	    formatMDAssocI4(msgBuff, GCA_WAIT_LOCK, sessionParm->sp_wait_lock);
	    parmCnt++;
	    break;

	case IIAPI_SP_GW_PARM :
	    for( i = 0; i < sessionParm->sp_gw_parm_count; i++ )
	    {
		formatMDAssocStr( msgBuff, 
				  GCA_GW_PARM, sessionParm->sp_gw_parm[i] );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_RESTART :
	    formatMDAssocStr( msgBuff, GCA_RESTART, sessionParm->sp_restart );
	    parmCnt++;
	    break;

	case IIAPI_SP_SL_TYPE :
	    if( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60)
	    {
		formatMDAssocI4(msgBuff, GCA_SL_TYPE, sessionParm->sp_sl_type);
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_CAN_PROMPT :
	    if( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60)
	    {
		formatMDAssocI4( msgBuff, 
				 GCA_CAN_PROMPT, sessionParm->sp_can_prompt );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_XA_RESTART :
	    formatMDAssocStr( msgBuff, 
			      GCA_XA_RESTART, sessionParm->sp_xa_restart );
	    parmCnt++;
	    break;

	case IIAPI_SP_YEAR_CUTOFF :
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_62 )
	    {
		formatMDAssocI4( msgBuff, 
				 GCA_YEAR_CUTOFF, sessionParm->sp_year_cutoff );
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_I8WIDTH :
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_65 )
	    {
		formatMDAssocI4(msgBuff, GCA_I8WIDTH, sessionParm->sp_i8width);
		parmCnt++;
	    }
	    break;

	case IIAPI_SP_DATE_ALIAS :
	    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_67 )
	    {
		formatMDAssocStr( msgBuff, 
				  GCA_DATE_ALIAS, sessionParm->sp_date_alias );
		parmCnt++;
	    }
	    break;
	}

    /*
    ** Update array element count now that 
    ** parameter cound has been determined.
    */
    I4ASSIGN_MACRO( parmCnt, *lptr );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: formatMDAssocI4
**
** Description:
**	Format a Modify Association (GCA_USER_DATA) integer parameter.
**
** Input:
**	msgBuff		GCA message buffer.
**	parmIndex	Parameter index.
**	parmVal		Parameter value.
**
** Output:
**	msgBuff		Appended GCA_USER_DATA.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static II_VOID
formatMDAssocI4( IIAPI_MSG_BUFF *msgBuff, i4 parmIndex, i4 parmVal )
{
    GCA_DATA_VALUE gdv;

    I4ASSIGN_MACRO( parmIndex, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( i4 );

    gdv.gca_type = DB_INT_TYPE;
    gdv.gca_precision = 0;
    gdv.gca_l_value = sizeof( i4 );
    formatGDV( msgBuff, &gdv );

    I4ASSIGN_MACRO( parmVal, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( i4 );
    return;
}


/*
** Name: formatMDAssocStr
**
** Description:
**	Format a Modify Association (gCA_USER_DATA) string parameter.
**
** Input:
**	msgBuff		GCA message buffer.
**	parmIndex	Parameter index.
**	parmVal		Parameter value.
**
** Output:
**	msgBuff		Appended GCA_USER_DATA.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static II_VOID
formatMDAssocStr( IIAPI_MSG_BUFF *msgBuff, i4 parmIndex, char *parmVal )
{
    GCA_DATA_VALUE	gdv;
    i4			len = STlength( parmVal );

    I4ASSIGN_MACRO( parmIndex, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( i4 );

    if ( ! len )
    {
	/*
	** Ingres does not like zero length CHAR strings,
	** so send a zero length VARCHAR string which is
	** simply a two byte length indicator with value 0.
	*/
	i2 zero = 0;

	gdv.gca_type = DB_VCH_TYPE;
	gdv.gca_precision = 0;
	gdv.gca_l_value = sizeof( i2 );
	formatGDV( msgBuff, &gdv );

	I2ASSIGN_MACRO( zero, *(msgBuff->data + msgBuff->length) );
	msgBuff->length += sizeof( i2 );
    }
    else
    {
	gdv.gca_type = DB_CHA_TYPE;
	gdv.gca_precision = 0;
	gdv.gca_l_value = len;
	formatGDV( msgBuff, &gdv );

    	MEcopy( parmVal, len, msgBuff->data + msgBuff->length );
	msgBuff->length += len;
    }

    return;
}


/*
** Name: IIapi_createMsgPRReply
**
** Description:
**	Create a GCA Prompt Reply message.  Allocates and returns a
**	GCA message buffer.
**
** Input:
**	connHndl		Connection handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgPRReply( IIAPI_CONNHNDL *connHndl )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;
    IIAPI_PROMPTPARM	*promptParm = 
    				(IIAPI_PROMPTPARM *)connHndl->ch_prmptParm;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgPRReply: creating Prompt Reply message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	return( NULL );

    msgBuff->msgType = GCA_PRREPLY;
    msg = msgBuff->data + msgBuff->length;

    val = 0;					/* gca_prflags */
    if ( promptParm->pd_flags & IIAPI_PR_NOECHO )  val |= GCA_PR_NOECHO;
    if ( promptParm->pd_rep_flags & IIAPI_REPLY_TIMEOUT ) val |= GCA_PR_TIMEOUT;

    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = promptParm->pd_rep_len ? 1 : 0;	/* gca_l_prvalue */
    I4ASSIGN_MACRO( val, *msg );
    msgBuff->length += sizeof( i4 );

    if ( promptParm->pd_rep_len )		/* gca_prvalue */
    {
	GCA_DATA_VALUE	gdv;

	gdv.gca_type = DB_CHA_TYPE;
	gdv.gca_precision = 0;
	gdv.gca_l_value = promptParm->pd_rep_len;

	formatGDV( msgBuff, &gdv );
	MEcopy( promptParm->pd_reply, promptParm->pd_rep_len,
	        msgBuff->data + msgBuff->length );
	msgBuff->length += promptParm->pd_rep_len;
    }

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgSecure
**
** Description:
**	Create a GCA Secure message.  Allocates and returns a GCA 
**	message buffer.
**
** Input:
**	tranHndl		Transaction handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgSecure( IIAPI_TRANHNDL *tranHndl )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgSecure: creating Secure message\n" );

    if ( ! tranHndl->th_tranName )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createMsgSecure: no distributed xact name handle\n" );
	return( NULL );
    }

    if ( tranHndl->th_tranName->tn_tranID.ti_type == IIAPI_TI_XAXID )
    {
	if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)tranHndl )) )
	    return( NULL );

	msgBuff->msgType = GCA_XA_SECURE;
	msg = msgBuff->data + msgBuff->length;

	val = 0;				/* gca_xa_type */
	I4ASSIGN_MACRO( val, *msg );
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );
        
	val = 0;				/* gca_xa_precision */
	I4ASSIGN_MACRO( val, *msg );
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );
        
	val = sizeof( IIAPI_XA_DIS_TRAN_ID );	/* gca_xa_l_value */
	I4ASSIGN_MACRO( val, *msg );
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );
        
	MEcopy( &tranHndl->th_tranName->tn_tranID.ti_value.xaXID, 
	        sizeof( IIAPI_XA_DIS_TRAN_ID ), msg );
	msgBuff->length += sizeof( IIAPI_XA_DIS_TRAN_ID );
    }
    else
    {
	IIAPI_II_DIS_TRAN_ID *iiTranID = 
			&tranHndl->th_tranName->tn_tranID.ti_value.iiXID;

	if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)tranHndl )) )
	    return( NULL );

	msgBuff->msgType = GCA_SECURE;
	msg = msgBuff->data + msgBuff->length;
        
	/* gca_tx_id.gca_index[0] */
	I4ASSIGN_MACRO( iiTranID->ii_tranID.it_highTran, *msg );
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );
        
	/* gca_tx_id.ga_index[1] */
	I4ASSIGN_MACRO( iiTranID->ii_tranID.it_lowTran, *msg );
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );
        
	/*
	** Server ignores gca_name value.  Pass same value as LIBQ.
	*/
    	MEfill( GCA_MAXNAME, 0, msg );		/* gca_tx_id.gca_name */
	*msg = (u_i1)('?');
	msg += GCA_MAXNAME;
	msgBuff->length += GCA_MAXNAME;

	val = GCA_DTX;				/* gca_tx_type */
	I4ASSIGN_MACRO( val, *msg );
	msgBuff->length += sizeof( i4 );
    }

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgXA
**
** Description:
**	Create a GCA XA message.  Allocates and returns a GCA 
**	message buffer.
**
** Input:
**	handle			Handle associated with connection.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgXA( IIAPI_HNDL *handle, i4 msgType,
		   IIAPI_XA_DIS_TRAN_ID *xid, u_i4 xaFlags )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val, len;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgXA: creating XA message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( handle )) )  return( NULL );
    msgBuff->msgType = msgType;
    msg = msgBuff->data + msgBuff->length;

    val = xaFlags;				/* gca_xa_flags */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );
        
    val = xid->xa_tranID.xt_formatID;		/* gca_xa_xid.formatID */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );
        
    val = xid->xa_tranID.xt_gtridLength;	/* gca_xa_xid.gtrid_length */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );
        
    val = xid->xa_tranID.xt_bqualLength;	/* gca_xa_xid.bqual_length */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );
        
						/* gca_xa_xid.data */
    len = xid->xa_tranID.xt_gtridLength + xid->xa_tranID.xt_bqualLength;
    len = min( len, GCA_XA_XID_MAX );

    MEcopy( xid->xa_tranID.xt_data, len, msg );
    if ( len < GCA_XA_XID_MAX )  MEfill( GCA_XA_XID_MAX - len, 0, msg + len );
    msg += GCA_XA_XID_MAX;
    msgBuff->length += GCA_XA_XID_MAX;

    val = xid->xa_branchSeqnum;			/* gca_xa_xid.branch_seqnum */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );
        
    val = xid->xa_branchFlag;			/* gca_xa_xid.branch_flag */
    I4ASSIGN_MACRO( val, *msg );
    msgBuff->length += sizeof( i4 );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgQuery
**
** Description:
**	Create a GCA Query message.  This routine may be used to
**	execute internal API statements but should not be used
**	for queries or external statements.  Allocates and returns
**	a GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**	queryText		Query Text.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgQuery( IIAPI_HNDL *handle, char *queryText )
{
    GCA_DATA_VALUE	gdv;
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val, len = STlength( queryText ) + 1;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgQuery: creating Query message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( handle )) )  return( NULL );
    msgBuff->msgType = GCA_QUERY;
    msg = msgBuff->data + msgBuff->length;

    val = DB_SQL;				/* gca_language_id */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = 0;					/* gca_query_modifier */
    I4ASSIGN_MACRO( val, *msg );
    msgBuff->length += sizeof( i4 );

    gdv.gca_type = DB_QTXT_TYPE;		/* Query Text */
    gdv.gca_precision = 0;
    gdv.gca_l_value = len;
    formatGDV( msgBuff, &gdv );

    MEcopy( queryText, len, msgBuff->data + msgBuff->length );
    msgBuff->length += len;

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgAck
**
** Description:
**	Create a GCA Acknowledge message.  Allocates and returns a
**	GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgAck( IIAPI_HNDL *handle )
{
    IIAPI_MSG_BUFF	*msgBuff;
    i4			val;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgAck: creating Acknowledge message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( handle )) )  return( NULL );
    msgBuff->msgType = GCA_ACK;

    val = 0;					/* gca_ak_data */
    I4ASSIGN_MACRO( val, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( i4 );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgAttn
**
** Description:
**	Create a GCA Attention message.  Allocates and returns a
**	GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgAttn( IIAPI_HNDL *handle )
{
    IIAPI_MSG_BUFF	*msgBuff;
    i4			val;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgAttn: creating Attention message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( handle )) )  return( NULL );
    msgBuff->msgType = GCA_ATTENTION;

    val = GCA_INTALL;				/* gca_itype */
    I4ASSIGN_MACRO( val, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( i4 );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgFetch
**
** Description:
**	Create a GCA Fetch message.  Allocates and returns a 
**	GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**	gca_id			Cursor ID
**	rowCount		Number of rows
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgFetch( IIAPI_HNDL *handle, GCA_ID *gca_id, i2 rowCount )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl( handle );

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgFetch: creating Fetch message\n" );

    if ( ! connHndl )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createMsgFetch: can't retrieve connection handle\n" );
	return( NULL );
    }

    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_62  &&
         rowCount > 1 )
    {
	if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	    return( NULL );

	msgBuff->msgType = GCA1_FETCH;
	formatGCAId( msgBuff, gca_id );

	val = (i4)rowCount;				/* gca_rowcount */
	I4ASSIGN_MACRO( val, *(msgBuff->data + msgBuff->length) );
	msgBuff->length += sizeof( i4 );
    }
    else
    {
	if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	    return( NULL );

	msgBuff->msgType = GCA_FETCH;
	msg = msgBuff->data + msgBuff->length;

	if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_2  ||
	     rowCount <= 1 )
	    formatGCAId( msgBuff, gca_id );
	else
	{
	    GCA_ID id;

	    id.gca_index[0] = gca_id->gca_index[0];
	    id.gca_index[1] = gca_id->gca_index[1];
	    MEcopy( gca_id->gca_name, GCA_MAXNAME, id.gca_name );
	    CVna( (i4)rowCount, &id.gca_name[ DB_GW1_MAXNAME_32 ] );

	    formatGCAId( msgBuff, &id );
	}
    }

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgScroll
**
** Description:
**	Create a GCA Fetch message with scrolling.  Allocates and 
**	returns a GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**	gca_id			Cursor ID
**	reference		Reference point
**	offset			Row offset
**	rowCount		Number of rows
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgScroll( IIAPI_HNDL *handle, GCA_ID *gca_id,
		       u_i2 reference, i4 offset, i2 rowCount )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl( handle );

    if ( ! connHndl )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createMsgScroll: can't retrieve  connection handle\n" );
	return( NULL );
    }

    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_67 )
	return( IIapi_createMsgFetch( handle, gca_id, rowCount ) );

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgScroll: creating Fetch with scroll message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	return( NULL );

    msgBuff->msgType = GCA2_FETCH;
    formatGCAId( msgBuff, gca_id );
    msg = msgBuff->data + msgBuff->length;

    val = (i4)rowCount;				/* gca_rowcount */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = (i4)reference;			/* gca_anchor */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    I4ASSIGN_MACRO( offset, *msg );		/* gca_offset */
    msgBuff->length += sizeof( i4 );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgClose
**
** Description:
**	Create a GCA Close message.  Allocates and returns a
**	GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**	gca_id			Cursor ID.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	06-Apr-10 (gordy)
**	    Removed unused variables.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgClose( IIAPI_HNDL *handle, GCA_ID *gca_id )
{
    IIAPI_MSG_BUFF	*msgBuff;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgClose: creating Close message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( handle )) )  return( NULL );
    msgBuff->msgType = GCA_CLOSE;
    formatGCAId( msgBuff, gca_id );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgResponse
**
** Description:
**	Create a GCA Response message.  Allocates and returns a
**	GCA message buffer.
**
** Input:
**	handle			Handle associated with connection.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgResponse( IIAPI_HNDL *handle )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val, len;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgResponse: creating Response message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( handle )) )  return( NULL );
    msgBuff->msgType = GCA_RESPONSE;
    msg = msgBuff->data + msgBuff->length;

    val = 0;					/* gca_rid */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = GCA_OK_MASK;				/* gca_rqstatus */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = GCA_NO_ROW_COUNT;			/* gca_rowcount */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    /*
    ** Fill remainder of message with zeros
    */
    len = (sizeof( i4 ) * 7) + 16;
    MEfill( len, 0, msg );
    msgBuff->length += len;

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_createMsgRelease
**
** Description:
**	Create a GCA Release message.  Allocates and returns a
**	GCA message buffer.
**
** Input:
**	connHndl		Connection handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	Message buffer, NULL if error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createMsgRelease( IIAPI_CONNHNDL *connHndl )
{
    IIAPI_MSG_BUFF	*msgBuff;
    i4			val;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
    	( "IIapi_createMsgRelease: creating Release message\n" );

    if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	return( NULL );

    msgBuff->msgType = GCA_RELEASE;

    /*
    ** GCA_RELEASE uses the GCA_ERROR message object.
    ** No error message associated with the release
    ** is sent to the server, so simply set the element
    ** length to zero.
    */
    val = 0;
    I4ASSIGN_MACRO( val, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( i4 );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_beginMessage
**
** Description:
**	Allocates a GCA message buffer with a size appropriate
**	for the associated connection.  If there is an existing
**	message buffer, it will be marked EOD and moved to the
**	message queue.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	New message buffer.
**	    sh_sndQueue	Appended with existing message buffer.
**
** Returns:
**	IIAPI_STATUS	API Status, IIAPI_ST_SUCCESS if no error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_beginMessage( IIAPI_STMTHNDL *stmtHndl, i4 msgType )
{
    IIapi_endMessage( stmtHndl, 0 );

    stmtHndl->sh_sndBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)stmtHndl );
    if ( ! stmtHndl->sh_sndBuff )  return( IIAPI_ST_OUT_OF_MEMORY );

    stmtHndl->sh_sndBuff->msgType = msgType;
    return( IIAPI_ST_SUCCESS );
}


/*
** Name: IIapi_extendMessage
**
** Description:
**	Extend current GCA message by saving current message buffer
**	on the message queue and allocating a new message buffer.
**	Message type and flags are propogated to new buffer.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	New message buffer.
**	    sh_sndQueue	Appended with existing message buffer.
**
** Returns:
**	IIAPI_STATUS	API Status, IIAPI_ST_SUCCESS if no error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_extendMessage( IIAPI_STMTHNDL *stmtHndl )
{
    IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;

    /*
    ** Turn off end of message flags (since message is geing extended)
    ** and move current message buffer to message queue.
    */
    msgBuff->flags &= ~(IIAPI_MSG_EOD | IIAPI_MSG_EOG | IIAPI_MSG_NEOG);
    QUinsert( &msgBuff->queue, stmtHndl->sh_sndQueue.q_prev );

    /*
    ** Create new message buffer with same query type.
    */
    stmtHndl->sh_sndBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)stmtHndl );
    if ( ! stmtHndl->sh_sndBuff )  return( IIAPI_ST_OUT_OF_MEMORY );

    stmtHndl->sh_sndBuff->msgType = msgBuff->msgType;
    stmtHndl->sh_sndBuff->flags = msgBuff->flags;
    return( IIAPI_ST_SUCCESS );
}


/*
** IIapi_endMessage
**
** Description:
**	Marks the current message buffer as EOD and includes any
**	caller provided flags.  Moves buffer to the message queue.  
**
**	This routine is a NO-OP if there isn't a current message buffer.
**
** Input:
**	stmtHndl	Statement handle.
**	flags		Additional message flags.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	New message buffer.
**	    sh_sndQueue	Appended with existing message buffer.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_endMessage( IIAPI_STMTHNDL *stmtHndl, u_i2 flags )
{
    if ( stmtHndl->sh_sndBuff )
    {
	stmtHndl->sh_sndBuff->flags |= IIAPI_MSG_EOD | flags;

	QUinsert( &stmtHndl->sh_sndBuff->queue,
		  stmtHndl->sh_sndQueue.q_prev );

	stmtHndl->sh_sndBuff = NULL;
    }

    return;
}


/*
** Name: IIapi_createMsgTuple
**
** Description:
**	Create a GCA Tuple message.  The only tuples sent to the
**	server are during a copy operation, so the message type
**	is actually GCA_CDATA.
**
**	The message must be finalized by calling IIapi_endMessage().
**
** Input:
**      stmtHndl	Statement handle
**	putColParm	Parameter block for IIapi_putColumns()
**
** Output
**	stmtHndl
**	    sh_sndBuff	GCA_CDATA message.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_createMsgTuple( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTCOLPARM *putColParm )
{
    IIAPI_DESCRIPTOR	*descriptor;
    IIAPI_DATAVALUE	*dataValue;
    bool		compVCH;
    IIAPI_STATUS	status;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createMsgTuple: creating Tuple message \n" );
    
    /*
    ** Complete the current message at the start of each tuple.
    **
    ** Note that if the first column is a LOB, the MORE_SEGMENTS 
    ** flag will be set after the first segment is processed and 
    ** indicates that initial processing has already been done.
    **
    */
    if ( ! (stmtHndl->sh_colIndex > 0  ||  
            stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS) )
	IIapi_endMessage( stmtHndl, 0 );

    /*
    ** Start a message if there isn't a current message buffer.
    */
    if ( ! stmtHndl->sh_sndBuff )
    {
	status = IIapi_beginMessage( stmtHndl, GCA_CDATA );
	if ( status != IIAPI_ST_SUCCESS )  return( status );
    }

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	("IIapi_createMsgTuple: %d columns starting at %d (%d total)\n",
	 (i4)stmtHndl->sh_colFetch, (i4)stmtHndl->sh_colIndex, 
	 (i4)stmtHndl->sh_colCount );
    
    /*
    ** The descriptor in the statement handle is complete
    ** and the current element is referenced by colIndex.
    ** The provided data array may be partial and the
    ** current element must be calculated from the total
    ** number of elements and the number remaining to be
    ** sent.
    */
    descriptor = &stmtHndl->sh_colDescriptor[ stmtHndl->sh_colIndex ];
    dataValue = &putColParm->pc_columnData[ putColParm->pc_columnCount - 
					    stmtHndl->sh_colFetch ];
    compVCH = ((stmtHndl->sh_flags & IIAPI_SH_COMP_VARCHAR) != 0);

    while( stmtHndl->sh_colFetch > 0 )
    {
	bool moreSegments;
	u_i1 null_indicator = 0;

	/*
	** The more-segments indicator only applies to
	** the last parameter in the current request.
	*/
	stmtHndl->sh_colFetch--;
	moreSegments = (stmtHndl->sh_colFetch <= 0  &&
			putColParm->pc_moreSegments);

	if ( descriptor->ds_nullable  &&  dataValue->dv_null )
	{
	    status = formatNullValue( stmtHndl, descriptor, compVCH );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    null_indicator = GCA_NULL_BIT;
	}
	else  if ( IIapi_isBLOB( descriptor->ds_dataType ) )
	{
	    status = formatLOB( stmtHndl, descriptor, dataValue, moreSegments );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );

	    /*
	    ** If there are more LOB segments to be send,
	    ** interrupt processing until the next request
	    ** is made.
	    */
	    if ( moreSegments )  break;
	}
	else
	{
	    status = formatDataValue(stmtHndl, descriptor, dataValue, compVCH);
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}

	/*
	** Nullable values are terminated with a NULL indicator byte.
	*/
	if ( descriptor->ds_nullable )
	{
	    IIAPI_MSG_BUFF	*msgBuff;

	    status = reserve( stmtHndl, 1 );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );

	    msgBuff = stmtHndl->sh_sndBuff;
	    msgBuff->data[ msgBuff->length++ ] = null_indicator;
	}

	/*
	** Prepare to process next column.
	*/
	stmtHndl->sh_colIndex++;
	descriptor++;
	dataValue++;
    }
    
    /*
    ** Check to see if we have completed the current tuple.
    */
    if ( stmtHndl->sh_colIndex >= stmtHndl->sh_colCount )
	stmtHndl->sh_colIndex = 0;              /* Reset for next tuple */

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: IIapi_initMsgQuery
**
** Description:
**	Initialize the appropriate GCA message for the current 
**	statement type.  
**
**	Parameter values associated with the statement are 
**	formatted by IIapi_addMsgParams() which must be called 
**	at some point following this routine, even if there are
**	no query parameters provided, because some query types
**	have optional service parameters for which defaults
**	must be generated.
**
**	The message must be finalized by calling IIapi_endMessage().
**
**	During batch processing, a given statement may be executed
**	repeatedly.  This operation can be optimized by setting
**	the repeat flag for the second and all subsequent executions
**	in the sequence.  Repeated execution is only applicable to
**	general statements, prepared statements and procedures.  
**	Note that this is distinctly different from the execution 
**	of a server defined repeat query.
**
** Input:
**	stmtHndl	Statement handle.
**	repeat		Previous statement is being repeated.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API Status, IIAPI_ST_SUCCESS if no error.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_initMsgQuery( IIAPI_STMTHNDL *stmtHndl, bool repeat )
{
    IIAPI_STATUS status;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_initMsgQuery: creating query message\n" );

    /*
    ** Initialize a new GCA message buffer.  The default
    ** message type is GCA_QUERY which may be updated by
    ** subsequent processing.
    */
    status = IIapi_beginMessage( stmtHndl, GCA_QUERY );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    switch( stmtHndl->sh_queryType )
    {
    case IIAPI_QT_EXEC :
    	status = createExecStmt( stmtHndl, repeat );
	break;

    case IIAPI_QT_OPEN :
	status = createCursorOpen( stmtHndl );
	break;

    case IIAPI_QT_CURSOR_DELETE :
    case IIAPI_QT_CURSOR_UPDATE :
	status = createCursorUpd( stmtHndl );
	break;

    case IIAPI_QT_DEF_REPEAT_QUERY :
	status = createDefRepeat( stmtHndl );
	break;

    case IIAPI_QT_EXEC_REPEAT_QUERY :
	status = createExecRepeat( stmtHndl );
	break;

    case IIAPI_QT_EXEC_PROCEDURE :
	status = createExecProc( stmtHndl, repeat );
	break;

    default :
	status = createQuery( stmtHndl, repeat );
	break;
    }

    return( status );
}


/*
** Name: createQuery
**
** Description:
**	Creates a generic GCA_QUERY message and formats the 
**	query text in the message.
**
** Input:
**	stmtHndl	Statement Handle.
**	repeat		Previous statement is being repeated.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createQuery( IIAPI_STMTHNDL *stmtHndl, bool repeat )
{
    QTXT_CB		qtxt;
    IIAPI_STATUS	status;

    /*
    ** Build the GCA_Q_DATA message object.
    */
    status = formatGCAQuery( stmtHndl, repeat );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    if ( ! repeat )
    {
	/*
	** Format the query text as GCA_DATA_VALUEs.
	*/
	status = beginQueryText( stmtHndl, &qtxt );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	status = formatQueryText( stmtHndl, &qtxt, stmtHndl->sh_queryText );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	status = endQueryText( stmtHndl, &qtxt );
    }

    return( status );
}


/*
** Name: createExecStmt
**
** Description:
**	Creates a GCA_QUERY message for executing a prepared
**	statement and formats the query text in the message.
**
** Input:
**	stmtHndl	Statement Handle.
**	repeat		Previous statement is being repeated.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createExecStmt( IIAPI_STMTHNDL *stmtHndl, bool repeat )
{
    QTXT_CB		qtxt;
    IIAPI_STATUS	status;

    /*
    ** Build the GCA_Q_DATA message object.
    */
    status = formatGCAQuery( stmtHndl, repeat );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    if ( ! repeat )
    {
	/*
	** Format the query text as GCA_DATA_VALUEs.
	*/
	status = beginQueryText( stmtHndl, &qtxt );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	status = formatQueryText( stmtHndl, &qtxt, stmtHndl->sh_queryText );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	/*
	** Append USING clause for dynamic parameters.
	*/
	status = formatUsingClause( stmtHndl, &qtxt );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	status = endQueryText( stmtHndl, &qtxt );
    }

    return( status );
}


/*
** Name: createExecProc
**
** Description:
**	Creates a GCA{1,2}_INVPROC message for executing a procedure.
**
** Input:
**	stmtHndl	Statement Handle.
**	repeat		Previous statement is being repeated.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Message type is assigned.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createExecProc( IIAPI_STMTHNDL *stmtHndl, bool repeat )
{
    IIAPI_CONNHNDL *connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl );

    /*
    ** Determine which GCA{1,2}_INVPROC message will be used.
    */
    if ( ! connHndl )		/* Should not happen! */
	stmtHndl->sh_sndBuff->msgType = GCA_INVPROC;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_68  &&
	       stmtHndl->sh_parmDescriptor[0].ds_dataType != IIAPI_HNDL_TYPE )
	stmtHndl->sh_sndBuff->msgType = GCA2_INVPROC;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60 )
	stmtHndl->sh_sndBuff->msgType = GCA1_INVPROC;
    else
	stmtHndl->sh_sndBuff->msgType = GCA_INVPROC;

    /*
    ** Nothing else can be done until the service
    ** parameter values are available.
    */
    return( IIAPI_ST_SUCCESS );
}


/*
** Name: createCursorOpen
**
** Description:
**	Creates a GCA_QUERY message for opening a cursor and 
**	formats the query text in the message.
**
** Input:
**	stmtHndl	Statement Handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createCursorOpen( IIAPI_STMTHNDL *stmtHndl )
{
    QTXT_CB		qtxt;
    IIAPI_STATUS	status;

    /*
    ** Build the GCA_Q_DATA message object.
    */
    status = formatGCAQuery( stmtHndl, FALSE );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    /*
    ** Format the query text as GCA_DATA_VALUEs.
    */
    status = beginQueryText( stmtHndl, &qtxt );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    /*
    ** Query text is prefixed with OPEN CURSOR.
    */
    status = formatQueryText( stmtHndl, &qtxt, 
	(stmtHndl->sh_flags & IIAPI_QF_SCROLL) ? api_qt_scroll : api_qt_open );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = formatQueryText( stmtHndl, &qtxt, stmtHndl->sh_queryText );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    /*
    ** Dynamic cursor requires USING clause to be appended.
    */
    if ( dynamic_open( stmtHndl->sh_queryText ) )
    {
	status = formatUsingClause( stmtHndl, &qtxt );
	if ( status != IIAPI_ST_SUCCESS )  return( status );
    }

    status = endQueryText( stmtHndl, &qtxt );
    return( status );
}


/*
** Name: createCursorUpd
**
** Description:
**	Creates a GCA_QUERY message for cursor relative update 
**	or delete and formats the query text in the message.
**
** Input:
**	stmtHndl	Statement Handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createCursorUpd( IIAPI_STMTHNDL *stmtHndl )
{
    QTXT_CB		qtxt;
    IIAPI_STATUS	status;

    /*
    ** Build the GCA_Q_DATA message object.
    */
    status = formatGCAQuery( stmtHndl, FALSE );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    /*
    ** Format the query text as GCA_DATA_VALUEs.
    */
    status = beginQueryText( stmtHndl, &qtxt );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = formatQueryText( stmtHndl, &qtxt, stmtHndl->sh_queryText );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = formatQueryText( stmtHndl, &qtxt, api_qt_where );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = endQueryText( stmtHndl, &qtxt );
    return( status );
}


/*
** Name: createDefRepeat
**
** Description:
**	Creates a GCA_QUERY message for defining a repeat
**	statement and formats the query text in the message.
**
** Input:
**	stmtHndl	Statement Handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createDefRepeat( IIAPI_STMTHNDL *stmtHndl )
{
    QTXT_CB		qtxt;
    IIAPI_STATUS	status;

    /*
    ** Build the GCA_Q_DATA message object.
    */
    status = formatGCAQuery( stmtHndl, FALSE );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    /*
    ** Format the query text as GCA_DATA_VALUEs.
    */
    status = beginQueryText( stmtHndl, &qtxt );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = formatQueryText( stmtHndl, &qtxt, api_qt_repeat );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = formatQueryText( stmtHndl, &qtxt, stmtHndl->sh_queryText );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    status = endQueryText( stmtHndl, &qtxt );
    return( status );
}


/*
** Name: createExecRepeat
**
** Description:
**	Creates a GCA_INVOKE message for executing a repeat query.
**
** Input:
**	stmtHndl	Statement Handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Message type is assigned.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
createExecRepeat( IIAPI_STMTHNDL *stmtHndl )
{
    /*
    ** Nothing to do until parameter values are
    ** available other than to set the correct
    ** message type.
    */
    stmtHndl->sh_sndBuff->msgType = GCA_INVOKE;
    return( IIAPI_ST_SUCCESS );
}


/*
** Name: IIapi_addMsgParams
**
** Description:
**	Formats parameter values for the statement started by
**	IIapi_initMsgQuery().  
**
**	This routine must be called even if there are no provided 
**	query parameters since some query types have optional 
**	parameters for which defaults will be provided.  The
**	IIapi_putParms() parameter block should be set to NULL
**	when there are no query parameters provided.
**	
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block, may be NULL.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_addMsgParams( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_STATUS status;

    /*
    ** Perform any statement specific pre-processing when the first 
    ** set of parameters are processed.
    **
    ** Note that if the first parameter is a LOB, the MORE_SEGMENTS 
    ** flag will be set after the first segment is processed and 
    ** indicates that initial processing has already been done.
    */
    if ( ! (stmtHndl->sh_parmIndex  ||  
            stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS) )
	switch( stmtHndl->sh_queryType )
	{
	case IIAPI_QT_OPEN :
	    status = initCursorOpen( stmtHndl, putParmParm );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;

	case IIAPI_QT_CURSOR_DELETE :
	    status = initCursorDel( stmtHndl, putParmParm );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;

	case IIAPI_QT_CURSOR_UPDATE :
	    status = initCursorUpd( stmtHndl, putParmParm );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;

	case IIAPI_QT_DEF_REPEAT_QUERY :
	    status = initRepeatDef( stmtHndl, putParmParm );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;

	case IIAPI_QT_EXEC_REPEAT_QUERY :
	    status = initRepeatExec( stmtHndl, putParmParm );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;

	case IIAPI_QT_EXEC_PROCEDURE :
	    status = initProcExec( stmtHndl, putParmParm );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;
	}

    /*
    ** Process query parameters.
    */
    if ( putParmParm )
	switch( stmtHndl->sh_queryType )
	{
	case IIAPI_QT_EXEC_PROCEDURE :
	    status = addQueryParams( stmtHndl, putParmParm, TRUE );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;

	default : 
	    status = addQueryParams( stmtHndl, putParmParm, FALSE );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;
	}

    /*
    ** Perform any statement specific post-processing when the last
    ** parameters have been processed.
    */
    if ( stmtHndl->sh_parmIndex >= stmtHndl->sh_parmCount )
	switch( stmtHndl->sh_queryType )
	{
	case IIAPI_QT_CURSOR_UPDATE :
	{
	    /*
	    ** The cursor ID was saved at the start of parameter
	    ** processing, but must be sent after all other query
	    ** parameters.
	    */
	    GCA_ID *gca_id = &stmtHndl->sh_cursorHndl->id_gca_id;
	    stmtHndl->sh_cursorHndl = NULL;

	    status = formatQueryId( stmtHndl, 
				    gca_id->gca_index[0], gca_id->gca_index[1], 
				    sizeof( gca_id->gca_name ), 
				    gca_id->gca_name );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    break;
	}
	}

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: initProcExec
**
** Description:
**	Formats service parameter values for the EXECUTE PROCEDURE 
**	statement.  
**
**	One service parameter is required: the procedure name or handle.  
**	One optional service parameter is allowed: the procedure owner.
**	
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Parameters appended.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
initProcExec( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_DESCRIPTOR	*descriptor;
    IIAPI_DATAVALUE	*dataValue;
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;
    bool		owner;
    IIAPI_STATUS	status;

    /*
    ** The descriptor in the statement handle is complete
    ** and the current element is referenced by parmIndex.
    ** The provided data array may be partial and the
    ** current element must be calculated from the total
    ** number of elements and the number remaining to be
    ** sent.
    */
    descriptor = &stmtHndl->sh_parmDescriptor[ stmtHndl->sh_parmIndex ];
    dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount -
    					   stmtHndl->sh_parmSend ];
    owner = (stmtHndl->sh_parmCount > 1  &&
	     descriptor[1].ds_columnType == IIAPI_COL_SVCPARM);

    /*
    ** Build the appropriate message object for the
    ** message type determined in createProcExec().
    */
    switch( stmtHndl->sh_sndBuff->msgType )
    {
    case GCA_INVPROC :
	/*
	** Reserve space for gca_id_proc (two i4's and GCA_MAXNAME)
	** and gca_proc_mask (an i4).
	*/
	status = reserve( stmtHndl, (sizeof( i4 ) * 3) + GCA_MAXNAME );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	if ( descriptor[0].ds_dataType == IIAPI_HNDL_TYPE )
	{
	    IIAPI_IDHNDL *idHndl;

	    MEcopy( dataValue[0].dv_value, sizeof( II_PTR ), &idHndl );
	    formatGCAId( stmtHndl->sh_sndBuff, &idHndl->id_gca_id );

	    stmtHndl->sh_parmIndex++;
	    stmtHndl->sh_parmSend--;
	}
	else
	{
	    GCA_ID	gca_id;
	    char	*ptr;
	    i4		len, size;

	    /*
	    ** Build GCA_ID
	    */
	    gca_id.gca_index[0] = 0;
	    gca_id.gca_index[1] = 0;
	    ptr = gca_id.gca_name;
	    size = sizeof( gca_id.gca_name );

	    if ( owner )
	    {
		/*
		** Prefix ID with procedure owner.
		*/
		len = min( dataValue[1].dv_length, size );
		MEcopy( dataValue[1].dv_value, len, ptr );
		ptr += len;
		size -= len;

		stmtHndl->sh_parmIndex++;
		stmtHndl->sh_parmSend--;

		/*
		** Add separator between owner and procedure name.
		*/
		if ( size )
		{
		    *(ptr++) = '.';
		    size--;
		}
	    }

	    /*
	    ** Append procedure name to ID.
	    */
	    if ( (len = min( dataValue[0].dv_length, size )) )
	    {
		MEcopy( dataValue[0].dv_value, len, ptr );
		ptr += len;
		size -= len;
	    }

	    stmtHndl->sh_parmIndex++;
	    stmtHndl->sh_parmSend--;

	    /*
	    ** Remainder of ID is space filled.
	    */
	    if ( size )  MEfill( size, ' ', ptr );

	    formatGCAId( stmtHndl->sh_sndBuff, &gca_id );
	}
	break;

    case GCA1_INVPROC :
    {
	i4 own_len = owner ? dataValue[1].dv_length : 0;

	/*
	** Reserve space for gca_id_proc (two i4's and GCA_MAXNAME), 
	** gca_proc_own (an i4 and owner name) and gca_proc_mask (an i4).
	*/
	status = reserve( stmtHndl, (sizeof(i4) * 4) + GCA_MAXNAME + own_len );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	if ( descriptor[0].ds_dataType == IIAPI_HNDL_TYPE )
	{
	    IIAPI_IDHNDL *idHndl;

	    MEcopy( dataValue[0].dv_value, sizeof( II_PTR ), &idHndl );
	    formatGCAId( stmtHndl->sh_sndBuff, &idHndl->id_gca_id );

	    stmtHndl->sh_parmIndex++;
	    stmtHndl->sh_parmSend--;
	}
	else
	{
	    GCA_ID	gca_id;
	    char	*ptr;
	    i4		len, size;

	    /*
	    ** Build GCA_ID
	    */
	    gca_id.gca_index[0] = 0;
	    gca_id.gca_index[1] = 0;
	    ptr = gca_id.gca_name;
	    size = sizeof( gca_id.gca_name );

	    if ( (len = min( dataValue[0].dv_length, size )) )
	    {
		MEcopy( dataValue[0].dv_value, len, ptr );
		ptr += len;
		size -= len;
	    }

	    stmtHndl->sh_parmIndex++;
	    stmtHndl->sh_parmSend--;

	    /*
	    ** Remainder of ID is space filled.
	    */
	    if ( size )  MEfill( size, ' ', ptr );

	    formatGCAId( stmtHndl->sh_sndBuff, &gca_id );
	}

	msgBuff = stmtHndl->sh_sndBuff;
	msg = msgBuff->data + msgBuff->length;

	I4ASSIGN_MACRO( own_len, *msg );	/* gca_proc_own.gca_l_name */
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );

	if ( owner )
	{
	    if ( own_len )
	    {
		MEcopy( dataValue[1].dv_value, own_len, msg );
		msgBuff->length += own_len;
	    }

	    stmtHndl->sh_parmIndex++;
	    stmtHndl->sh_parmSend--;
	}
	break;
    }
    case GCA2_INVPROC :
    {
	i4 name_len = dataValue[0].dv_length;
	i4 own_len = owner ? dataValue[1].dv_length : 0;

	/*
	** Reserve space for gca_proc_name (an i4 and proc name), 
	** gca_proc_own (an i4 and owner name) and gca_proc_mask 
	** (an i4).
	*/
	status = reserve( stmtHndl, (sizeof( i4 ) * 3) + name_len + own_len );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	msgBuff = stmtHndl->sh_sndBuff;
	msg = msgBuff->data + msgBuff->length;

	I4ASSIGN_MACRO( name_len, *msg );	/* gca_proc_name.gca_l_name */
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );

	if ( name_len )
	{
	    MEcopy( dataValue[0].dv_value, name_len, msg );
	    msg += name_len;
	    msgBuff->length += name_len;
	}

	stmtHndl->sh_parmIndex++;
	stmtHndl->sh_parmSend--;

	I4ASSIGN_MACRO( own_len, *msg );	/* gca_proc_own.gca_l_name */
	msg += sizeof( i4 );
	msgBuff->length += sizeof( i4 );

	if ( owner )
	{
	    if ( own_len )
	    {
		MEcopy( dataValue[1].dv_value, own_len, msg );
		msgBuff->length += own_len;
	    }

	    stmtHndl->sh_parmIndex++;
	    stmtHndl->sh_parmSend--;
	}
	break;
    }
    }

    val = GCA_PARAM_NAMES_MASK;			/* gca_proc_mask */

    if ( stmtHndl->sh_flags & IIAPI_QF_LOCATORS )
	val |= GCA_IP_LOCATOR_MASK;

    I4ASSIGN_MACRO( val, *(stmtHndl->sh_sndBuff->data + 
			   stmtHndl->sh_sndBuff->length) );
    stmtHndl->sh_sndBuff->length += sizeof( i4 );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: initCursorOpen
**
** Description:
**	Formats service parameter values for the OPEN CURSOR statement.
**
**	One optional service parameter is allowed: the cursor name.
**
**	This routine must be called even if there are no provided query 
**	parameters since the cursor name is optional and a default will 
**	be provided if necessary.  The IIapi_putParms() parameter block 
**	should be set to NULL when there are no query parameters provided.
**	
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block, may be NULL.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Parameters appended.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
initCursorOpen( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_DESCRIPTOR	*descriptor;
    IIAPI_DATAVALUE	*dataValue;
    i2			parmCount = 0;
    char		buffer[ GCA_MAXNAME + 1 ];
    char		*ptr;
    i4			len;
    IIAPI_STATUS	status;

    /*
    ** Extract parameter info if provided.
    */
    if ( putParmParm )
    {
	/*
	** The descriptor in the statement handle is complete
	** and the current element is referenced by parmIndex.
	** The provided data array may be partial and the
	** current element must be calculated from the total
	** number of elements and the number remaining to be
	** sent.
	*/
	parmCount = stmtHndl->sh_parmSend;
	descriptor = &stmtHndl->sh_parmDescriptor[ stmtHndl->sh_parmIndex ];
	dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount -
					       stmtHndl->sh_parmSend ];
    }

    /*
    ** Use the cursor name if provided as a service parameter.
    ** Otherwise, generate a default cursor name.
    */
    if ( parmCount  &&  descriptor->ds_columnType == IIAPI_COL_SVCPARM )
    {
	len = dataValue[0].dv_length;
	ptr = dataValue[0].dv_value;
	stmtHndl->sh_parmIndex++;
	stmtHndl->sh_parmSend--;
    }
    else
    {
	/*
	** Generate a unique cursor name.  Use the statement
	** handle address if the connection handle cannot
	** be retrieved (should always be accessible).
	*/
	IIAPI_CONNHNDL *connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl );

	STprintf( buffer, "IIAPICURSOR%d", 
		  connHndl ? connHndl->ch_cursorID++ : (i4)(SCALARP)stmtHndl );

	ptr = buffer;
	len = STlength( buffer );
    }

    /*
    ** Format the query ID as three query parameters.
    */
    status = formatQueryId( stmtHndl, 0, 0, len, ptr );
    return( status );
}


/*
** Name: initCursorDel
**
** Description:
**	Formats service parameter values for the CURSOR DELETE statement.
**
**	One service parameter is required: the cursor handle.
**
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Parameters appended.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
initCursorDel( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_DATAVALUE	*dataValue;
    IIAPI_STMTHNDL	*cursorHndl;
    IIAPI_STATUS	status;

    /*
    ** The provided data array may be partial and the
    ** current element must be calculated from the total
    ** number of elements and the number remaining to be
    ** sent.
    */
    dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount -
					   stmtHndl->sh_parmSend ];

    /*
    ** The cursor handle parameter is actually a statement
    ** handle which contains a cursor handle.
    */
    MEcopy( dataValue[0].dv_value, sizeof( II_PTR ), &cursorHndl );

    stmtHndl->sh_parmIndex++;
    stmtHndl->sh_parmSend--;

    status = formatQueryId( stmtHndl, 
			    cursorHndl->sh_cursorHndl->id_gca_id.gca_index[0], 
    			    cursorHndl->sh_cursorHndl->id_gca_id.gca_index[1], 
			    GCA_MAXNAME, 
			    cursorHndl->sh_cursorHndl->id_gca_id.gca_name );

    return( status );
}


/*
** Name: initCursorUpd
**
** Description:
**	Formats service parameter values for the CURSOR UPDATE statement.
**
**	One service parameter is required: the cursor handle.  
**	
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Parameters appended.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
initCursorUpd( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_DATAVALUE	*dataValue;
    IIAPI_STMTHNDL	*cursorHndl;

    /*
    ** The provided data array may be partial and the
    ** current element must be calculated from the total
    ** number of elements and the number remaining to be
    ** sent.
    */
    dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount -
					   stmtHndl->sh_parmSend ];

    /*
    ** The first provided parameter is a statement handle
    ** which contains a cursor handle.  The cursor query
    ** ID is the last parameter to be sent, so the cursor
    ** handle must be saved in the current statement
    ** handle for later processing.  Don't set the cursor
    ** flag since the cursor is actually owned by the
    ** other statement handle.
    */
    MEcopy( dataValue[0].dv_value, sizeof( II_PTR ), &cursorHndl );
    stmtHndl->sh_cursorHndl = cursorHndl->sh_cursorHndl;
    stmtHndl->sh_parmIndex++;
    stmtHndl->sh_parmSend--;

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: initRepeatDef
**
** Description:
**	Formats service parameter values for the OPEN CURSOR statement.
**
**	Three optional service parameter is allowed: the components of
**	a query ID.
**
**	This routine must be called even if there are no provided query 
**	parameters since the query ID is optional and a default will 
**	be provided if necessary.  The IIapi_putParms() parameter block 
**	should be set to NULL when there are no query parameters provided.
**	
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block, may be NULL.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Parameters appended.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
initRepeatDef( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_DESCRIPTOR	*descriptor;
    IIAPI_DATAVALUE	*dataValue;
    i2			parmCount = 0;
    char		buffer[ GC_HOSTNAME_MAX + GCA_MAXNAME + 8 ];
    char		*ptr;
    i4			len, gca_id1, gca_id2;
    IIAPI_STATUS	status;

    /*
    ** Extract parameter info if provided.
    */
    if ( putParmParm )
    {
	/*
	** The descriptor in the statement handle is complete
	** and the current element is referenced by parmIndex.
	** The provided data array may be partial and the
	** current element must be calculated from the total
	** number of elements and the number remaining to be
	** sent.
	*/
	parmCount = stmtHndl->sh_parmSend;
	descriptor = &stmtHndl->sh_parmDescriptor[ stmtHndl->sh_parmIndex ];
	dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount -
					       stmtHndl->sh_parmSend ];
    }

    /*
    ** Use the repeat query ID if provided as a service parameter.
    ** Otherwise, generate a default repeat query ID.
    */
    if ( parmCount  &&  descriptor->ds_columnType == IIAPI_COL_SVCPARM )
    {
	I4ASSIGN_MACRO( *((u_i1 *)dataValue[0].dv_value), gca_id1 );
	I4ASSIGN_MACRO( *((u_i1 *)dataValue[1].dv_value), gca_id2 );
	len = dataValue[2].dv_length;
	ptr = dataValue[2].dv_value;

	stmtHndl->sh_parmIndex += 3;
	stmtHndl->sh_parmSend -= 3;
    }
    else
    {
	/*
	** Generate a unique repeat query ID.  The time from
	** TMnow() should be unique in this process.  Use
	** other system info to ensure uniqueness across
	** processes and hosts.
	*/
	SYSTIME now;
	char	host[ GC_HOSTNAME_MAX ];
	char	process[ GCA_MAXNAME ];

	TMnow( &now );
	GChostname( host, sizeof( host ) );
	PCunique( process );

	gca_id1 = now.TM_secs;
	gca_id2 = now.TM_msecs;

	STpolycat( 4, "IIAPI_", host, "_", process, buffer );

	ptr = buffer;
	len = STlength( buffer );
    }

    /*
    ** Format the query ID as three query parameters.
    */
    status = formatQueryId( stmtHndl, gca_id1, gca_id2, len, ptr );
    return( status );
}


/*
** Name: initRepeatExec
**
** Description:
**	Formats service parameter values for the OPEN CURSOR statement.
**
**	One service parameter is required: the query handle.
**
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameter block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Parameters appended.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
initRepeatExec( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    IIAPI_DATAVALUE	*dataValue;
    IIAPI_IDHNDL	*idHndl;
    IIAPI_STATUS	status;

    /*
    ** The provided data array may be partial and the
    ** current element must be calculated from the total
    ** number of elements and the number remaining to be
    ** sent.
    */
    dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount -
					   stmtHndl->sh_parmSend ];

    MEcopy( dataValue[0].dv_value, sizeof( II_PTR ), &idHndl );

    stmtHndl->sh_parmIndex++;
    stmtHndl->sh_parmSend--;

    /*
    ** Reserve space for gca_qid (two i4's and GCA_MAXNAME).
    */
    status = reserve( stmtHndl, (sizeof( i4 ) * 2) + GCA_MAXNAME );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    formatGCAId( stmtHndl->sh_sndBuff, &idHndl->id_gca_id );

    return( status );
}


/*
** Name: addQueryParams
**
** Description:
**	Formats query parameter values.  Optionally, the parameters
**	may be formatted for procedure execution (GCA{1,2}_INVPROC).
**	
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	Parameter block from IIapi_putParms().
**	execProc	Format parameters for procedure execution.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	GCA_QUERY message.
**
** Returns:
**	IIAPI_STATUS	API Status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	 7-Apr-10 (gordy)
**	    Output parameters supported at protocol level 66.
*/

static IIAPI_STATUS
addQueryParams( IIAPI_STMTHNDL *stmtHndl, 
		IIAPI_PUTPARMPARM *putParmParm, bool execProc )
{
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl((IIAPI_HNDL *)stmtHndl);
    IIAPI_DESCRIPTOR	*descriptor;
    IIAPI_DATAVALUE	*dataValue;
    GCA_DATA_VALUE	gdv;
    IIAPI_STATUS	status;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_addMsgParams: %d params starting with %d, %d total params\n",
	  stmtHndl->sh_parmSend, stmtHndl->sh_parmIndex, 
	  stmtHndl->sh_parmCount );

    /*
    ** The descriptor in the statement handle is complete
    ** and the current element is referenced by parmIndex.
    ** The provided data array may be partial and the
    ** current element must be calculated from the total
    ** number of elements and the number remaining to be
    ** sent.
    */
    descriptor = &stmtHndl->sh_parmDescriptor[ stmtHndl->sh_parmIndex ];
    dataValue = &putParmParm->pp_parmData[ putParmParm->pp_parmCount - 
					   stmtHndl->sh_parmSend ];

    while( stmtHndl->sh_parmSend > 0 )
    {
	bool	moreSegments;
	u_i1	null_indicator = 0;

	/*
	** The more-segments indicator only applies to
	** the last parameter in the current request.
	*/
	stmtHndl->sh_parmSend--;
	moreSegments = (stmtHndl->sh_parmSend <= 0 &&
			putParmParm->pp_moreSegments);

	/*
	** Each parameter value is encapsulated as a 
	** GCA_DATA_VALUE which begins with a fixed
	** header.
	**
	** The IIAPI_SH_MORE_SEGMENTS flag indicates that
	** processing of the current LOB value began in
	** a preceding request and the GDV header has
	** already been formatted.
	*/
	if ( ! (stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS) )
	{
	    /*
	    ** Make sure there is space availabe in message buffer
	    ** to hold the fixed portion of the GCA_DATA_VALUE.
	    */
	    i4 name_len, size = sizeof( i4 ) * 3;

	    if ( execProc )
	    {
		/*
		** Procedure parameters include the parameter
		** name, with an i4 for the length, and an i4
		** mask.
		*/
		name_len = descriptor->ds_columnName
			   ? STlength( descriptor->ds_columnName ) : 0;

		size += (sizeof( i4 ) * 2) + name_len;
	    }

	    status = reserve( stmtHndl, size );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );

	    if ( execProc )
	    {
		i4		mask = 0;
		IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;
		u_i1		*msg = msgBuff->data + msgBuff->length;

		I4ASSIGN_MACRO( name_len, *msg );	/* gca_l_name */
		msg += sizeof( i4 );
		msgBuff->length += sizeof( i4 );

		if ( name_len )
		{
		    MEcopy( descriptor->ds_columnName, name_len, msg );
		    msg += name_len;
		    msgBuff->length += name_len;
		}

		switch( descriptor->ds_columnType )
		{
		case IIAPI_COL_PROCINOUTPARM :	mask |= GCA_IS_BYREF;	break;
		case IIAPI_COL_PROCGTTPARM :	mask |= GCA_IS_GTTPARM;	break;

		case IIAPI_COL_PROCOUTPARM :
		    /*
		    ** Output parameters backward supported as BYREF.
		    */
		    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_66 )
			mask |= GCA_IS_OUTPUT;
		    else
			mask |= GCA_IS_BYREF;
		    break;
		}

		I4ASSIGN_MACRO( mask, *msg );		/* gca_parm_mask */
		msgBuff->length += sizeof( i4 );
	    }

	    IIapi_cnvtDescr2GDV( descriptor, &gdv );
	    formatGDV( stmtHndl->sh_sndBuff, &gdv );
	}

	if ( descriptor->ds_nullable  &&  dataValue->dv_null )
	{
	    status = formatNullValue( stmtHndl, descriptor, FALSE );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    null_indicator = GCA_NULL_BIT;
	}
	else  if ( IIapi_isBLOB( descriptor->ds_dataType ) )
	{
	    status = formatLOB(stmtHndl, descriptor, dataValue, moreSegments);
	    if ( status != IIAPI_ST_SUCCESS )  return( status );

	    /*
	    ** If there are more LOB segments to be sent,
	    ** interrupt processing until the next request
	    ** is made.
	    */
	    if ( moreSegments )  break;
	}
	else
	{
	    status = formatDataValue( stmtHndl, descriptor, dataValue, FALSE );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}

	/*
	** Nullable values are terminated with a NULL indicator byte.
	*/
	if ( descriptor->ds_nullable )
	{
	    IIAPI_MSG_BUFF *msgBuff;

	    status = reserve( stmtHndl, 1 );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );

	    msgBuff = stmtHndl->sh_sndBuff;
	    msgBuff->data[ msgBuff->length++ ] = null_indicator;
	}

	/*
	** Prepare to process next parameter.
	*/
	stmtHndl->sh_parmIndex++;
    	descriptor++;
	dataValue++;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatGCAQuery
**
** Description:
**	Format the fixed portion of a GCA_QUERY message.  Space for
**	two I4 values must be available (reserved) in the buffer.
**
** Input:
**	stmtHndl	Statement handle.
**	repeat		Previous statement is being repeated.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended GCA_Q_DATA.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatGCAQuery( IIAPI_STMTHNDL *stmtHndl, bool repeat )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;
    IIAPI_STATUS	status;

    /*
    ** Make sure there is space availabe in message buffer
    ** to hold the fixed portion of the GCA_Q_DATA.
    */
    status = reserve( stmtHndl, sizeof( i4 ) * 2 );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    msgBuff = stmtHndl->sh_sndBuff;
    msg = msgBuff->data + msgBuff->length;

    /*
    ** Build the GCA_Q_DATA message object.
    */
    val = DB_SQL;				/* gca_language_id */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = GCA_NAMES_MASK | GCA_COMP_MASK;	/* gca_query_modifier */

    if ( stmtHndl->sh_queryType == IIAPI_QT_SELECT_SINGLETON )
	val |= GCA_SINGLE_MASK;

    if ( stmtHndl->sh_flags & IIAPI_QF_LOCATORS )
	val |= GCA_LOCATOR_MASK;

    if ( (connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl ))  &&
	 connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60 )
	val |= GCA_Q_NO_XACT_STMT;

    if ( repeat )  val |= GCA_REPEAT_MASK;

    I4ASSIGN_MACRO( val, *msg );
    msgBuff->length += sizeof( i4 );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatUsingClause
**
** Description:
**	Determines the number of query parameters, if any,
**	and appends an appropriate USING clause.
**
** Input:
**	stmtHndl	Statement handle.
**	qtxt		Query text control block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	New buffer if needed.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatUsingClause( IIAPI_STMTHNDL *stmtHndl, QTXT_CB *qtxt )
{
    i4			i, parmCount;
    IIAPI_STATUS	status;

    /*
    ** Determine the number of query parameters so that the
    ** using clause can be built.  The parameter set may
    ** include service params which need to be excluded.
    ** Service params, if present, are grouped at the start.
    */
    for( 
         i = 0, parmCount = stmtHndl->sh_parmCount;
         i < stmtHndl->sh_parmCount  &&
	 stmtHndl->sh_parmDescriptor[i].ds_columnType == IIAPI_COL_SVCPARM;
	 i++, parmCount--
       );

    if ( parmCount > 0 )
    {
	/*
	** Append ' USING ~V {, ~V }' to query text.
	*/
	status = formatQueryText( stmtHndl, qtxt, api_qt_using );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	while( --parmCount )
	{
	    status = formatQueryText( stmtHndl, qtxt, api_qt_marker );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: beginQueryText
**
** Description:
**	Initializes the message buffer and query text control
**	block so that query text can be built up in pieces.
**	Must be called prior to calling formatQueryText().
**
** Input:
**	stmtHndl	Statement handle.
**	qtxt		Query text control block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended QTXT GCA_DATA_VALUE.
**	qtxt
**	    gdv_ptr	Current GDV length reference.
**	    seg_max	Maximum length of current segment.
**	    seg_len	0.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
beginQueryText( IIAPI_STMTHNDL *stmtHndl, QTXT_CB *qtxt )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			val;
    IIAPI_STATUS	status;

    /*
    ** Query text is broken up into GCA_DATA_VALUE segments.
    **
    ** Make sure there is space availabe in message buffer
    ** to hold the fixed portion of the GCA_DATA_VALUE and
    ** at least one byte of query text.
    */
    status = reserve( stmtHndl, sizeof( i4 ) * 3 + 1 );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    msgBuff = stmtHndl->sh_sndBuff;
    msg = msgBuff->data + msgBuff->length;

    val = DB_QTXT_TYPE;			/* gca_type */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = 0;				/* gca_precision */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    val = 0;				/* gca_l_value */
    I4ASSIGN_MACRO( val, *msg );
    msgBuff->length += sizeof( i4 );

    /*
    ** The GDV length is currently just a place holder.
    ** It will be updated when the segment is complete.
    */
    qtxt->gdv_ptr = msg;
    qtxt->seg_len = 0;

    /*
    ** The maximum length of each segment depends on the 
    ** space available in the message buffer and standard 
    ** string length supported by servers.
    */
    qtxt->seg_max = IIAPI_MSG_UNUSED( msgBuff );
    qtxt->seg_max = min( qtxt->seg_max, DB_MAXTUP );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatQueryText
**
** Description:
**	Format query text into message buffer.  The query
**	text may be built up in pieces by making multiple
**	calls to this function.  The message buffer must
**	be initialized to receive query text by calling
**	beginQueryText() prior to the first call to this
**	function.  EndQueryText() must be called once the
**	query text is complete.
**
** Input:
**	stmtHndl	Statement handle.
**	qtxt		Query text control block.
**	queryText	Query text.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended QTXT GCA_DATA_VALUE.
**	qtxt
**	    seg_len	Current segment length.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatQueryText( IIAPI_STMTHNDL *stmtHndl, QTXT_CB *qtxt, char *queryText )
{
    i4			queryLen = STlength( queryText );
    IIAPI_STATUS	status;

    while( queryLen )
    {
	i4 seglen;

	/*
	** When the current segment is filled, update the
	** GDV length of the segment and initialize a new
	** segment.
	*/
	if ( qtxt->seg_len >= qtxt->seg_max )
	{
	    I4ASSIGN_MACRO( qtxt->seg_len, *qtxt->gdv_ptr );
	    status = beginQueryText( stmtHndl, qtxt );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}

	seglen = qtxt->seg_max - qtxt->seg_len;
	seglen = min( queryLen, seglen );

        status = copyData( stmtHndl, seglen, queryText );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	qtxt->seg_len += seglen;
	queryText += seglen;
	queryLen -= seglen;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: endQueryText
**
** Description:
**	Finalizes the formatted query text.  A trailing terminator
**	is appended and the current segment GDV length is updated.
**
** Input:
**	stmtHndl	Statement handle.
**	qtxt		Query text control block.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended QTXT GCA_DATA_VALUE.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
endQueryText( IIAPI_STMTHNDL *stmtHndl, QTXT_CB *qtxt )
{
    IIAPI_MSG_BUFF	*msgBuff;
    IIAPI_STATUS	status;

    /*
    ** Unfortunately, the current segment could be full.
    ** If so, a new segment must be initialized jus for
    ** the terminator.
    */
    if ( qtxt->seg_len >= qtxt->seg_max )
    {
	I4ASSIGN_MACRO( qtxt->seg_len, *qtxt->gdv_ptr );
	status = beginQueryText( stmtHndl, qtxt );
	if ( status != IIAPI_ST_SUCCESS )  return( status );
    }

    /*
    ** Segments are sized so as to not span message buffers.
    ** Since there is now room in the current segment for
    ** the terminator simply append the terminator and update
    ** the segment length.
    */
    msgBuff = stmtHndl->sh_sndBuff;
    msgBuff->data[ msgBuff->length++ ] = 0;
    qtxt->seg_len++;
    I4ASSIGN_MACRO( qtxt->seg_len, *qtxt->gdv_ptr );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatQueryId
**
** Description:
**	Formats a query ID as two I4 parameters and a CHAR parameter.
**	The character component will be truncated to GCA_MAXNAME.
**
** Input:
**	stmtHndl	Statement handle.
**	id1		First query ID value.
**	id2		Second query ID value.
**	length		Length of id3.
**	id3		Third query ID value.
**
** Output:
**
** Returns;
**	IIAPI_STATUS
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatQueryId( IIAPI_STMTHNDL *stmtHndl, i4 id1, i4 id2, i4 length, char *id3 )
{
    GCA_DATA_VALUE	gdv;
    IIAPI_STATUS	status;

    /*
    ** Make sure there is space availabe in message buffer
    ** to hold entire GCA_DATA_VALUE.
    */
    status = reserve( stmtHndl, sizeof( i4 ) * 4 );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    gdv.gca_type = DB_INT_TYPE;	
    gdv.gca_precision = 0;
    gdv.gca_l_value = sizeof( i4 );
    formatGDV( stmtHndl->sh_sndBuff, &gdv );

    I4ASSIGN_MACRO( id1, *(stmtHndl->sh_sndBuff->data + 
			   stmtHndl->sh_sndBuff->length) );
    stmtHndl->sh_sndBuff->length += sizeof( i4 );

    status = reserve( stmtHndl, sizeof( i4 ) * 4 );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    formatGDV( stmtHndl->sh_sndBuff, &gdv );	/* Same as preceeding type */

    I4ASSIGN_MACRO( id2, *(stmtHndl->sh_sndBuff->data + 
			   stmtHndl->sh_sndBuff->length) );
    stmtHndl->sh_sndBuff->length += sizeof( i4 );

    status = reserve( stmtHndl, (sizeof( i4 ) * 3) + GCA_MAXNAME );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    gdv.gca_type = DB_CHA_TYPE;
    gdv.gca_precision = 0;
    gdv.gca_l_value = GCA_MAXNAME;
    formatGDV( stmtHndl->sh_sndBuff, &gdv );

    length = min( length, GCA_MAXNAME );

    if ( length )
    {
	MEcopy( id3, length, stmtHndl->sh_sndBuff->data + 
			     stmtHndl->sh_sndBuff->length );
	stmtHndl->sh_sndBuff->length += length;
    }

    if ( length < GCA_MAXNAME )  
    {
	length = GCA_MAXNAME - length;
	MEfill( length, ' ', stmtHndl->sh_sndBuff->data + 
			     stmtHndl->sh_sndBuff->length );
	stmtHndl->sh_sndBuff->length += length;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatGCAId
**
** Description:
**	Formats a GCA_ID structure.  Space for GCA_MAXNAME and
**	two I4 values must be available (reserved) in the buffer.
**
** Input:
**	msgBuff		GCA message buffer.
**	gca_id		GCA ID.
**
** Output:
**	msgBuff		Appended GCA_ID.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static II_VOID
formatGCAId( IIAPI_MSG_BUFF *msgBuff, GCA_ID *gca_id )
{
    u_i1 *msg = msgBuff->data + msgBuff->length;

    I4ASSIGN_MACRO( gca_id->gca_index[0], *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    I4ASSIGN_MACRO( gca_id->gca_index[1], *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    MEcopy( gca_id->gca_name, GCA_MAXNAME, msg );
    msgBuff->length += GCA_MAXNAME;

    return;
}


/*
** Name: formatGDV
**
** Description:
**	Format the fixed portion of a GCA_DATA_VALUE.  Space for
**	three I4 values must be available (reserved) in the buffer.
**	The actual data value must be formatted separately after 
**	calling this routine.
**
** Input:
**	msgBuff		Message buffer.
**	gdv		GCA_DATA_VALUE.
**
** Output:
**	msgBuff		Appended GCA_DATA_VALUE.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static II_VOID
formatGDV( IIAPI_MSG_BUFF *msgBuff, GCA_DATA_VALUE *gdv )
{
    u_i1 *msg = msgBuff->data + msgBuff->length;

    I4ASSIGN_MACRO( gdv->gca_type, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    I4ASSIGN_MACRO( gdv->gca_precision, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    I4ASSIGN_MACRO( gdv->gca_l_value, *msg );
    msgBuff->length += sizeof( i4 );

    return;
}


/*
** Name: formatNullValue
**
** Description:
**	Append a NULL data value to the current message buffer.
**	The output does not include the NULL indicator byte.
**
** Input:
**	stmtHndl	Statement handle.
**	descriptor	API descriptor.
**	compVCH		Compress varchar strings.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended NULL data value.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatNullValue( IIAPI_STMTHNDL *stmtHndl, 
		 IIAPI_DESCRIPTOR *descriptor, bool compVCH )
{
    i4			length;
    IIAPI_STATUS	status;

    if ( compVCH  &&  IIapi_isVAR( descriptor->ds_dataType ) )
    {
	/*
	** Compressed NULL variable length strings are
	** formatted as a single 2-byte length indicator.
	*/
	length = sizeof( i2 );
    }
    else
    {
	/*
	** The GCA length includes the NULL indicator byte.
	*/
	length = IIapi_getGCALength( descriptor ) - 1;
    }

    /*
    ** Zero fill the NULL value.
    */
    status = fillData( stmtHndl, length, 0 );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatDataValue
**
** Description:
**	Append a non-NULL data value to the current message buffer.
**	LOB values are formatted as a single segment.  The output
**	does not include the NULL indicator byte even if the value
**	is nullable.
**
** Input:
**	stmtHndl	Statement handle.
**	descriptor	API descriptor.
**	compVCH		Compress varchar strings.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended data value.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	31-Mar-2010 (kschendel)
**	    Make count a u_i2.
**	06-Apr-10 (gordy)
**	    Add cast to quiet compiler warning.
*/

static IIAPI_STATUS
formatDataValue( IIAPI_STMTHNDL *stmtHndl, IIAPI_DESCRIPTOR *descriptor, 
		 IIAPI_DATAVALUE *dataValue, bool compVCH )
{
    IIAPI_MSG_BUFF	*msgBuff;
    i4			size, length;
    u_i2		count;
    IIAPI_STATUS	status;

    switch( descriptor->ds_dataType )
    {
    case IIAPI_LVCH_TYPE :
    case IIAPI_LBYTE_TYPE :
    case IIAPI_LNVCH_TYPE :
	/*
	** LOB values are treated as a single data segment
	** (no more segments to follow).
	*/
	status = formatLOB( stmtHndl, descriptor, dataValue, FALSE );
	if ( status != IIAPI_ST_SUCCESS )  return( status );
	break;

    case IIAPI_LCLOC_TYPE :
    case IIAPI_LBLOC_TYPE :
    case IIAPI_LNLOC_TYPE :
	/*
	** Locators are formatted as an ADF peripheral header
	** followed by the 4-byte integer locator value.
	*/
	status = formatLOBhdr( stmtHndl, ADP_P_LOC_L_UNK );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	status = reserve( stmtHndl, sizeof( i4 ) );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	msgBuff = stmtHndl->sh_sndBuff;
	I4ASSIGN_MACRO( *(u_i1 *)dataValue->dv_value, 
			*(msgBuff->data + msgBuff->length) );
	msgBuff->length += sizeof( i4 );
	break;

    case IIAPI_CHA_TYPE :
    case IIAPI_CHR_TYPE :
	/*
	** Fixed length character types are space padded.
	*/
	length = min( descriptor->ds_length, dataValue->dv_length );
	status = copyData( stmtHndl, length, dataValue->dv_value );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	if ( length < descriptor->ds_length )
	{
	    status = fillData( stmtHndl, descriptor->ds_length - length, ' ' );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}
	break;

    case IIAPI_NCHA_TYPE :
	/*
	** Fixed length UCS2 type is padded with UCS2 spaces.
	*/
	size = descriptor->ds_length / sizeof( UCS2 );
	length = dataValue->dv_length / sizeof( UCS2 );
	length = min( length, size );

	status = copyUCS2( stmtHndl, length, dataValue->dv_value );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	if ( length < size )
	{
	    status = fillUCS2( stmtHndl, size - length, 0x0020 );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}
	break;

    case IIAPI_NVCH_TYPE :
	/*
	** Variable length UCS2 type is padded with UCS2 zeros.
	** The embedded 2-byte length indicator is copied as an
	** additional UCS2 element.
	*/
	I2ASSIGN_MACRO( *(u_i1 *)dataValue->dv_value, count );
	count++;	/* Include length indicator */

	/*
	** Varchar compression decreases the size of the
	** column to just the data present.
	*/
	size = descriptor->ds_length / sizeof( UCS2 );
	if ( compVCH )  size = min( size, count );

	count = (u_i2)min( count, size );
	status = copyUCS2( stmtHndl, count, dataValue->dv_value );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	if ( count < size )
	{
	    status = fillUCS2( stmtHndl, size - count, 0x0000 );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}
	break;

    default :
	/*
	** All other types are copied as binary data and zero padded.
	*/
	size = descriptor->ds_length;

	if ( compVCH  &&  IIapi_isVAR( descriptor->ds_dataType ) )
	{
	    /*
	    ** Varchar compression decreases the size of the
	    ** column to just the data present.
	    */
	    I2ASSIGN_MACRO( *(u_i1 *)dataValue->dv_value, count );
	    count += sizeof( u_i2 );	/* Include length indicator */
	    size = min( size, count );
	}

	length = min( size, dataValue->dv_length );
	status = copyData( stmtHndl, length, dataValue->dv_value );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	if ( length < size )
	{
	    status = fillData( stmtHndl, size - length, '\0' );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	}
	break;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatLOB
**
** Description:
**	Format a LOB value.  
**
**	The IIAPI_SH_MORE_SEGMENTS flag is set when the first segment is 
**	processed and an ADP_PERIPHERAL header is added at the start of 
**	the LOB value.  
**
**	Subsequent calls to this routine will generate additional segments 
**	until the end of the LOB is indicated (moreSegments set to FALSE).  
**
**	The LOB vlaue is terminated with an end-of-segments indicator and 
**	the IIAPI_SH_MORE_SEGMENTS flag is cleared.
**
** Input:
**	stmtHndl	Statement handle.
**	descriptor	API data type descriptor for LOB.
**	dataValue	API data value for segment.
**	moreSegments	Are there more segments to follow?
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended GCA_DATA_VALUE.
**	    sh_flags	IIAPI_SH_MORE_SEGMENTS is set/cleared.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatLOB( IIAPI_STMTHNDL *stmtHndl, IIAPI_DESCRIPTOR *descriptor, 
	   IIAPI_DATAVALUE *dataValue, bool moreSegments )
{
    IIAPI_STATUS	status;

    /*
    ** The IIAPI_SH_MORE_SEGMENTS flag is set when  LOB value
    ** is started and remains set until end-of-segments is
    ** reached.  Only format the LOB header when at the start
    ** of a LOB value.
    */
    if ( ! (stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS) )
    {
	status = formatLOBhdr( stmtHndl, ADP_P_GCA_L_UNK );
	if ( status != IIAPI_ST_SUCCESS )  return( status );
	stmtHndl->sh_flags |= IIAPI_SH_MORE_SEGMENTS;
    }

    status = formatLOBseg( stmtHndl, descriptor, dataValue );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    /*
    ** Terminate LOB value with end-of-segments indicator
    ** if there are no more segments.
    */
    if ( ! moreSegments )
    {
	stmtHndl->sh_flags &= ~IIAPI_SH_MORE_SEGMENTS;
	status = formatLOBend( stmtHndl );
	if ( status != IIAPI_ST_SUCCESS )  return( status );
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatLOBhdr
**
** Description:
**	Format the ADF peripheral header.
**
** Input:
**	stmtHndl	Statement handle.
**	tag		ADP_PERIPHERAL per_tag value.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended GCA_DATA_VALUE.
**	    sh_flags	IIAPI_SH_MORE_SEGMENTS is set.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
formatLOBhdr( IIAPI_STMTHNDL *stmtHndl, i4 tag )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    i4			size0 = 0;
    i4			size1 = 0;
    IIAPI_STATUS	status;

    /*
    ** Make sure there is space availabe in message buffer
    ** to hold the ADF peripheral header.
    */
    status = reserve( stmtHndl, sizeof( i4 ) * 3 );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    msgBuff = stmtHndl->sh_sndBuff;
    msg = msgBuff->data + msgBuff->length;

    I4ASSIGN_MACRO( tag, *msg );
    msg += sizeof( tag );
    msgBuff->length += sizeof( tag );

    I4ASSIGN_MACRO( size0, *msg );
    msg += sizeof( size0 );
    msgBuff->length += sizeof( size0 );

    I4ASSIGN_MACRO( size1, *msg );
    msgBuff->length += sizeof( size1 );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatLOBseg
**
** Description:
**	Format LOB data segment.  No output is generated if the
**	segment length is zero.
**
** Input:
**	stmtHndl	Statement handle.
**	descriptor	API data type descriptor for LOB.
**	dataValue	API data value for segment.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended GCA_DATA_VALUE.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	31-Mar-2010 (kschendel)
**	    blobstor loops because it's feeding API 32K segments.
**	    Use u_i2 here to fix the looping.
**	06-Apr-10 (gordy)
**	    Add cast to quiet compiler warning.
*/

static IIAPI_STATUS
formatLOBseg( IIAPI_STMTHNDL *stmtHndl, 
	      IIAPI_DESCRIPTOR *descriptor, IIAPI_DATAVALUE *dataValue )
{
    u_i1		*data;
    u_i2		count, size;
    IIAPI_STATUS	status;

    I2ASSIGN_MACRO( *(u_i1 *)dataValue->dv_value, count );
    data = (u_i1 *)dataValue->dv_value + sizeof( count );
    size = (descriptor->ds_dataType == IIAPI_LNVCH_TYPE) ? sizeof( UCS2 ) 
    							 : sizeof( char );
    while( count )
    {
	IIAPI_MSG_BUFF	*msgBuff;
	u_i1		*msg;
	i4		indicator = 1;
	u_i2		seglen; 

	/*
	** Make sure there is space availabe in message buffer
	** for the segment indicator, length and at least one 
	** element.
	*/
	status = reserve( stmtHndl, sizeof( i4 ) + sizeof( i2 ) + size );
	if ( status != IIAPI_ST_SUCCESS )  return( status );

	msgBuff = stmtHndl->sh_sndBuff;
	msg = msgBuff->data + msgBuff->length;

	/*
	** Segment length is limited by the amount of availabe
	** data, the space remaining in the message buffer and
	** a standard limit on string lengths supported by
	** servers.
	*/
	seglen = (u_i2)IIAPI_MSG_UNUSED( msgBuff ) - 
		 sizeof( i4 ) - sizeof( i2 );
	seglen = min( seglen, DB_MAXTUP ) / size;
	seglen = min( seglen, count );

	I4ASSIGN_MACRO( indicator, *msg );
	msg += sizeof( indicator );
	msgBuff->length += sizeof( indicator );

	I2ASSIGN_MACRO( seglen, *msg );
	msgBuff->length += sizeof( seglen );

	status = (descriptor->ds_dataType == IIAPI_LNVCH_TYPE)
		 ? copyUCS2( stmtHndl, seglen, data )
		 : copyData( stmtHndl, seglen, data );

	if ( status != IIAPI_ST_SUCCESS )  return( status );
	data += seglen * size;
    	count -= seglen;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: formatLOBend
**
** Description:
**	Format LOB end-of-segments indicator.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended GCA_DATA_VALUE.
**	    sh_flags	IIAPI_SH_MORE_SEGMENTS is cleared.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
**	06-Apr-10 (gordy)
**	    Removed unused variable.
*/

static IIAPI_STATUS
formatLOBend( IIAPI_STMTHNDL *stmtHndl )
{
    IIAPI_MSG_BUFF	*msgBuff;
    i4			indicator = 0;
    IIAPI_STATUS	status;

    /*
    ** Make sure there is space availabe in message buffer
    ** for the end-of-segment indicator.
    */
    status = reserve( stmtHndl, sizeof( i4 ) );
    if ( status != IIAPI_ST_SUCCESS )  return( status );

    msgBuff = stmtHndl->sh_sndBuff;
    I4ASSIGN_MACRO( indicator, *(msgBuff->data + msgBuff->length) );
    msgBuff->length += sizeof( indicator );

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: copyData
**
** Description:
**	Copy data into message buffer.  Message will be extended
**	if data is larger than space available in current buffer.
**
** Input:
**	stmtHndl	Statement handle.
**	length		Data length.
**	data		Data.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended data.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
copyData( IIAPI_STMTHNDL *stmtHndl, i4 length, u_i1 *data )
{
    IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;
    IIAPI_STATUS	status;

    while( length )
    {
	/*
	** Data may span message buffers.  Copy remainder 
	** of data or fill remainder of current buffer.
	*/
	i4 seglen = IIAPI_MSG_UNUSED( msgBuff );
	seglen = min( length, seglen );

	if ( seglen <= 0 )
	{
	    status = IIapi_extendMessage( stmtHndl );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    msgBuff = stmtHndl->sh_sndBuff;
	    continue;
	}

	MEcopy( data, seglen, msgBuff->data + msgBuff->length );
	msgBuff->length += seglen;
	data += seglen;
	length -= seglen;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: copyUCS2
**
** Description:
**	Copy UCS2 data into message buffer without splitting UCS2
**	elements.  Message will be extended if data is larger than 
**	space available in current buffer.
**
** Input:
**	stmtHndl	Statement handle.
**	length		Number of UCS2 elements.
**	data		UCS2 data.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended data.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
copyUCS2( IIAPI_STMTHNDL *stmtHndl, i4 length, u_i1 *data )
{
    IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;
    IIAPI_STATUS	status;

    while( length )
    {
	/*
	** Data may span message buffers.  Copy remainder 
	** of data or fill remainder of current buffer.
	*/
	i4 seglen = IIAPI_MSG_UNUSED( msgBuff ) / sizeof( UCS2 );
	seglen = min( length, seglen );

	if ( seglen <= 0 )
	{
	    status = IIapi_extendMessage( stmtHndl );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    msgBuff = stmtHndl->sh_sndBuff;
	    continue;
	}

	length -= seglen;
	seglen *= sizeof( UCS2 );

	MEcopy( data, seglen, msgBuff->data + msgBuff->length );
	msgBuff->length += seglen;
	data += seglen;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: fillData
**
** Description:
**	Fills portion of message buffer with fixed data value.
**	Message will be extended if data is larger than space 
**	available in current buffer.
**
** Input:
**	stmtHndl	Statement handle.
**	length		Data length.
**	data		Data.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended data.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
fillData( IIAPI_STMTHNDL *stmtHndl, i4 length, char data )
{
    IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;
    IIAPI_STATUS	status;

    while( length )
    {
	/*
	** Data may span message buffers.  Fill remainder 
	** of data or fill remainder of current buffer.
	*/
	i4 seglen = IIAPI_MSG_UNUSED( msgBuff );
	seglen = min( length, seglen );

	if ( seglen <= 0 )
	{
	    status = IIapi_extendMessage( stmtHndl );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    msgBuff = stmtHndl->sh_sndBuff;
	    continue;
	}

	MEfill( seglen, data, msgBuff->data + msgBuff->length );
	msgBuff->length += seglen;
	length -= seglen;
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: fillUCS2
**
** Description:
**	Fills portion of message buffer with fixed UCS2 value.
**	Message will be extended if data is larger than space 
**	available in current buffer.
**
** Input:
**	stmtHndl	Statement handle.
**	length		Number of UCS2 elements.
**	data		UCS2 data.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	Appended data.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
fillUCS2( IIAPI_STMTHNDL *stmtHndl, i4 length, UCS2 data )
{
    IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;
    IIAPI_STATUS	status;

    while( length )
    {
	/*
	** Data may span message buffers.  Fill remainder 
	** of data or fill remainder of current buffer.
	*/
	i4 seglen = IIAPI_MSG_UNUSED( msgBuff ) / sizeof( UCS2 );
	seglen = min( length, seglen );

	if ( seglen <= 0 )
	{
	    status = IIapi_extendMessage( stmtHndl );
	    if ( status != IIAPI_ST_SUCCESS )  return( status );
	    msgBuff = stmtHndl->sh_sndBuff;
	    continue;
	}

	length -= seglen;

	while( seglen-- )
	{
	    I2ASSIGN_MACRO( data, *(msgBuff->data + msgBuff->length) );
	    msgBuff->length += sizeof( data );
	}
    }

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: reserve
**
** Description:
**	Ensure that space is available in current message buffer.
**	Message will be extended if there space available is less
**	than requested.
**
** Input:
**	stmtHndl	Statement handle.
**	length		Space to reserve.
**
** Output:
**	stmtHndl
**	    sh_sndBuff	New buffer if needed.
**
** Returns:
**	IIAPI_STATUS	API status.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static IIAPI_STATUS
reserve( IIAPI_STMTHNDL *stmtHndl, i4 length )
{
    IIAPI_MSG_BUFF	*msgBuff = stmtHndl->sh_sndBuff;
    IIAPI_STATUS	status = IIAPI_ST_SUCCESS;

    if ( IIAPI_MSG_UNUSED( msgBuff ) < length )
	status = IIapi_extendMessage( stmtHndl );

    return( status );
}


/*
** Name: get_tz_offset
**
** Description:
**	Given a timezone name, returns the timezone offset
**	from GMT in minutes.  The offset is in the format
**	used by the GCA_MD_ASSOC parameter GCA_TIMEZONE.
**
** Input:
**	tz_name		Timezone name.
**
** Output:
**	None.
**
** Returns:
**	i4		Timezone offset.
**
** History:
**	23-Mar-01 (gordy)
**	    Created.
*/

static i4
get_tz_offset( char *tz_name )
{
    i4		tmsecs;
    PTR		tz_cb;
    i4		offset;
    STATUS	status;

    /*
    ** TMtz_search requires a GMT or time local to the 
    ** timezone.  We don't have either of these, but we 
    ** can get GMT time from our local time by adjusting 
    ** for our gmt offset.
    */
    if ( (tmsecs = TMsecs()) > MAXI4 )  tmsecs = MAXI4;
    offset = TMtz_search( IIapi_static->api_tz_cb, TM_TIMETYPE_LOCAL, tmsecs );
    tmsecs -= offset;

    /*
    ** Get the timezone control block for the requested timezone.
    */
    status = TMtz_lookup( tz_name, &tz_cb );
    if ( status == TM_TZLKUP_FAIL )
    {
	MUp_semaphore( &IIapi_static->api_semaphore );
	status = TMtz_load( tz_name, &tz_cb );
	MUv_semaphore( &IIapi_static->api_semaphore );
    }

    if ( status != OK )  
    {
	/*
	** We have already gotten our local timezone
	** offset, so go ahead and use that instead.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "get_tz_offset: invalid timezone '%s'\n", tz_name );
    }
    else
    {
	/*
	** Now, get the timezone offset of the requested timezone,
	** using the current GMT time.
	*/
        offset = TMtz_search( tz_cb, TM_TIMETYPE_GMT, tmsecs );
    }

    /*
    ** Adjust to format expected by GCA_MD_ASSOC.
    */
    return( -offset / 60 );
}


/*
** Name: dynamic_open
**
** Description:
**	Determines if query text is a select statement or a
**	prepared statement name.
**
** Input:
**	qtxt	Query text.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if query text is prepared statement name (dynamic).
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static bool
dynamic_open( char *qtxt )
{
    bool dynamic = TRUE;

    /*
    ** Skip leading white space.
    */
    while( *qtxt  &&  CMwhite( qtxt ) )  CMnext( qtxt );

    /*
    ** A select statement may be nested in parenthesis 
    ** or will start with the keyword SELECT.
    */
    if ( *qtxt == '('  ||  
         (STlength( qtxt ) > 7  &&  STncasecmp( qtxt, "select ", 7 ) == 0 ) )
	dynamic = FALSE;

    return( dynamic );
}


