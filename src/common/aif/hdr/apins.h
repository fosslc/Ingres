/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apins.h
**
** Description:
**	Name Server State Machine interfaces.
**
** History:
**	12-Feb-97 (gordy)
**	    Created.
**	25-Mar-97 (gordy)
**	    Added macros for handling changes in Name Server protocol.
**	 3-Jul-97 (gordy)
**	    State machine interface is now through initialization
**	    function which sets the dispatch table entries.
**	22-Oct-04 (gordy)
**	    Return error code from IIapi_validNSDescriptor().
**	 1-Sep-09 (gordy)
**	    Increase maximum GCN string lengths to 256.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

# ifndef __APINS_H__
# define __APINS_H__

# include <apishndl.h>
# include <apimsg.h>


/*
** Maximum length of a GCN object.
*/
#define API_NS_MAX_LEN		256


/*
** Macros for handling Name Server protocol changes.
**
** In the original Name Server protocol end-of-group
** was indicated by the end-of-data flag and there was
** no way to indicate message continuation (end-of-data
** always assumed to be TRUE.
**
** In the new Name Server protocol end-of-group and
** end-of-data are handled according to GCA standards.
*/
#define NS_EOG( ch, gb )        \
		      ( ((ch)->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_63) \
			? ((gb)->flags & IIAPI_MSG_EOD) != 0 \
			: ((gb)->flags & IIAPI_MSG_EOD) != 0  && \
			  ((gb)->flags & IIAPI_MSG_EOG) != 0 )

#define NS_EOD( ch, gb )        \
		      ( ((ch)->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_63) \
			? (TRUE) \
			: ((gb)->flags & IIAPI_MSG_EOD) != 0 )


/*
** GCN result tuple.
**
** Provide information from GCN_REQ_TUPLE message object.
*/

typedef struct
{
    i4		gcn_l_item;
    char	*gcn_value;

} IIAPI_GCN_VAL;

typedef struct
{
    IIAPI_GCN_VAL	gcn_type;
    IIAPI_GCN_VAL	gcn_uid;
    IIAPI_GCN_VAL	gcn_obj;
    IIAPI_GCN_VAL	gcn_val;

} IIAPI_GCN_REQ_TUPLE;


/*
** Global functions.
*/

II_EXTERN IIAPI_STATUS		IIapi_ns_init( VOID );
II_EXTERN IIAPI_STATUS		IIapi_ns_cinit( VOID );
II_EXTERN IIAPI_STATUS		IIapi_ns_tinit( VOID );
II_EXTERN IIAPI_STATUS		IIapi_ns_sinit( VOID );

II_EXTERN IIAPI_MSG_BUFF	*IIapi_createGCNOper( IIAPI_STMTHNDL * );

II_EXTERN IIAPI_STATUS		IIapi_parseNSQuery(IIAPI_STMTHNDL *, STATUS *);

II_EXTERN II_ULONG		IIapi_validNSDescriptor( IIAPI_STMTHNDL *,
							 II_LONG,
							 IIAPI_DESCRIPTOR * );

II_EXTERN II_BOOL		IIapi_validNSParams( IIAPI_STMTHNDL *,
						     IIAPI_PUTPARMPARM * );

II_EXTERN II_VOID		IIapi_saveNSParams( IIAPI_STMTHNDL *,
						    IIAPI_PUTPARMPARM * );

II_EXTERN IIAPI_STATUS		IIapi_getNSDescriptor( IIAPI_STMTHNDL * );

II_EXTERN II_VOID		IIapi_loadNSColumns( IIAPI_STMTHNDL *,
						     IIAPI_GETCOLPARM *,
						     IIAPI_GCN_REQ_TUPLE * );

# endif /* __APINS_H__ */
