/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
**		This is the internal version that is used in building OpenROAD
**		executables.  The customer version is in "w4gl!w4gldemo!extev_ms".
**		The customer version is installed into "ingres\files" directory.
*/


/*
** History:
**	08-may-95 (chan) b68415
**		Increase MAX_RECIPIENTS, MAX_MESSAGES, and MAX_EVENTS
**	14-may-95 (chan)
**		Modify above change; Turns out that was too excessive for w32s.
**	16-may-95 (chan)
**		Add definition for STATUS, and changed return types of
**		prototypes to STATUS, to make 16-bit medium-model 3gl
**		programs work.
**	30-jun-95 (puree)
**		Use constants and function prototypes from customer version
**		of the include file to make sure that they are always in sync.
**      18-feb-98 (maggie)
**              Define xde for OR40 Unix Port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifdef xde
# include "../../../w4gl/w4gldemo/extev_ms/w4glext.h"
#else
# include "..\..\..\w4gl\w4gldemo\extev_ms\w4glext.h"
#endif


#ifndef	i4
#define	i4	long
#endif

#ifndef	i4
#define	i4	long
#endif

#ifndef	i2
#define	i2	short
#endif

#ifndef	STATUS
#define	STATUS	i4
#endif


#define MAX_RECIPIENTS				    (256)
#define MAX_MESSAGES				    (2048)
#define MAX_EVENTS				        (256)
#define MAX_EVENT_CHARS				    (64)


/*
**		Recipient Information.
*/
typedef struct	_RECIPIENT	RECIPIENT;
typedef struct	_RECIPIENT
{
	i4		id;
	i4			ILcode;
};

/*
**		An external user event.
*/
typedef struct	_W4GL_XEVENT	W4GL_XEVENT;
typedef struct	_W4GL_XEVENT
{
	i4			W4GLId;		                /* W4GL event id */
	i4			WindowsId;	                /* Windows message type */
	char			EventName[MAX_EVENT_CHARS];
	i4			RecipientCount;
	RECIPIENT	Recipients[MAX_RECIPIENTS]; /* window handles of recipients */
};


/*
**		Message for an external user event.
*/
typedef struct	_W4GL_XMESSAGE	W4GL_XMESSAGE;
typedef struct	_W4GL_XMESSAGE
{
	i4         W4GLId;           /* W4GL event id */
	i4			RecipientId;		/* message id */
	i4				FocusBehavior;
	i4				MsgValue;
};

#ifndef xde
 STATUS IIW4GL_Close(char *errmsg,char *token); 
 STATUS IIW4GL_Open(char *errmsg,char *hostname,char * *token); 
 STATUS IIW4GL_SendUserEvent(char *ErrMsg,i2 *SentCount, 
                          char *Token,char *EventName,i4 Fb,i4 Value);
#endif
 

typedef struct	_SNDUEVPARMS	SNDUEVPARMS;
typedef struct	_SNDUEVPARMS
{
	char *	ErrMsg;
	i2 * 	SentCount;
	char *	Token;
	char *	EventName;
	i4      Fb;
	i4      Value;
};
