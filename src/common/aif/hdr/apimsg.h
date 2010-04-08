/*
** Copyright (c) 2010 Ingres Corporation
*/

/*
** Name: apimsg.h
**
** Description:
**	GCA packed bytes stream message functions.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

# ifndef __APIMSG_H__
# define __APIMSG_H__

typedef	struct _IIAPI_MSG_BUFF	IIAPI_MSG_BUFF;

# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>


/*
** Message buffer.
*/

struct _IIAPI_MSG_BUFF
{
    
    QUEUE		queue;
    IIAPI_CONNHNDL	*connHndl;	/* Associated connection */
    i4			useCount;	/* Reserve count */
    i4			msgType;	/* GCA message type */
    u_i2		flags;		/* Message status flags */

#define	IIAPI_MSG_EOD	0x01		/* End of data */
#define	IIAPI_MSG_EOG	0x02		/* End of group */
#define	IIAPI_MSG_NEOG	0x04		/* Explicitly not end of group */

    i4			size;		/* Size of buffer */
    u_i1		*buffer;	/* GCA message buffer */
    i4			length;		/* Length of message data */
    u_i1		*data;		/* Start of message data */

#define	IIAPI_MSG_UNUSED( mb )	((mb)->size - (mb)->length)

};


/*
** Global Functions.
*/

II_EXTERN IIAPI_MSG_BUFF *	IIapi_allocMsgBuffer( IIAPI_HNDL * );
II_EXTERN II_VOID		IIapi_freeMsgBuffer( IIAPI_MSG_BUFF * );
II_EXTERN II_VOID		IIapi_reserveMsgBuffer( IIAPI_MSG_BUFF * );
II_EXTERN II_VOID		IIapi_releaseMsgBuffer( IIAPI_MSG_BUFF * );

II_EXTERN IIAPI_STATUS		IIapi_readMsgPrompt( IIAPI_MSG_BUFF *, 
						     GCA_PROMPT_DATA *, 
						     i4 *, char ** );
II_EXTERN IIAPI_STATUS		IIapi_readMsgQCID(IIAPI_MSG_BUFF *, GCA_ID *);
II_EXTERN IIAPI_STATUS		IIapi_readMsgDescr( IIAPI_STMTHNDL * );
II_EXTERN IIAPI_STATUS		IIapi_readMsgCopyMap( IIAPI_STMTHNDL * );
II_EXTERN IIAPI_STATUS		IIapi_readMsgEvent( IIAPI_CONNHNDL *, 
						    IIAPI_DBEVCB * );
II_EXTERN IIAPI_STATUS		IIapi_readMsgError( IIAPI_HNDL * );
II_EXTERN IIAPI_STATUS		IIapi_readMsgRetProc( IIAPI_MSG_BUFF *, 
						      GCA_RP_DATA *);
II_EXTERN IIAPI_STATUS		IIapi_readMsgResponse( IIAPI_MSG_BUFF *, 
						       GCA_RE_DATA *, bool );

II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgMDAssoc( IIAPI_CONNHNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgPRReply( IIAPI_CONNHNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgRelease( IIAPI_CONNHNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgQuery( IIAPI_HNDL *, char * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgAck( IIAPI_HNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgAttn( IIAPI_HNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgFetch( IIAPI_HNDL *, 
						       GCA_ID *, i2 );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgScroll( IIAPI_HNDL *, GCA_ID *,
							u_i2 , i4, i2 );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgClose(IIAPI_HNDL *, GCA_ID *);
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgResponse( IIAPI_HNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgSecure( IIAPI_TRANHNDL * );
II_EXTERN IIAPI_MSG_BUFF	*IIapi_createMsgXA( IIAPI_HNDL *, i4,
						    IIAPI_XA_DIS_TRAN_ID *,
						    u_i4 );

II_EXTERN IIAPI_STATUS	IIapi_beginMessage( IIAPI_STMTHNDL *, i4 );
II_EXTERN IIAPI_STATUS	IIapi_extendMessage( IIAPI_STMTHNDL * );
II_EXTERN II_VOID	IIapi_endMessage( IIAPI_STMTHNDL *, u_i2 );
II_EXTERN IIAPI_STATUS	IIapi_initMsgQuery( IIAPI_STMTHNDL *, bool );
II_EXTERN IIAPI_STATUS	IIapi_addMsgParams( IIAPI_STMTHNDL *, 
					    IIAPI_PUTPARMPARM * );

# endif /* __APIMSG_H__ */
