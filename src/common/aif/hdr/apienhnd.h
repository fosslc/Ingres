/*
** Copyright (c) 2004, 2007 Ingres Corporation
*/

/*
** Name: apienhnd.h
**
** Description:
**	This file contains the definition of the environment handle.
**	Environment handles permit independent modules to use the
**	API without interference.  Thus, a module will be able to
**	call IIapi_initialize() without worrying if it has already
**	been done my some other module.
**
**	The initial implementation of environment handles is totally
**	internal to the API.  There is one default environment handle
**	used by all API users.  In version 2 of the API, environment
**	handles will be made visible through the API interface.  The
**	environment handles will be used in the following routines:
**
**	IIapi_initialize()	Creates an environment handle.
**	IIapi_terminate()	Deletes an environment handle.
**	IIapi_setConnectParam()	Environment for the connection.
**	IIapi_connect()		Environment for the connection.
**	IIapi_registerXID()	Environment for transaction ID.
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
**	24-May-96 (gordy)
**	    Removed en_memTag.
**	15-Aug-98 (rajus01)
**	    Added IIAPI_USRPARM structure. Added IIAPI_USRPARM field 
**	    in IIAPI_ENVHNDL structure.
**	    Added IIapi_setEnvironParm(), IIapi_clearEnvironParm() global
**	    functions prototypes.
**	06-Oct-98 (rajus01)
**	    Added support for GCA_PROMPT.
**      03-Feb-99 (rajus01)
**	    Added IIAPI_EP_EVENT_FUNC. Renamed IIAPI_EP_CAN_PROMPT to
**	    IIAPI_EP_PROMPT_FUNC. Renamed IIAPI_UP_CAN_PROMPT to 
**	    IIAPI_UP_PROMPT_FUNC. Renamed up_can_prompt() to up_prompt_func().
**	    Added up_event_func().
**	15-Mar-04 (gordy)
**	    Added IIAPI_EP_INT8_WIDTH and IIAPI_UP_I8WIDTH for 8-byte ints.
**	 8-Aug-07 (gordy)
**	    Added IIAPI_UP_DATE_ALIAS.
*/

# ifndef __APIENHND_H__
# define __APIENHND_H__


typedef struct _IIAPI_USRPARM
{
    II_ULONG    up_mask1;

# define IIAPI_UP_CWIDTH                0x00000001
# define IIAPI_UP_TWIDTH                0x00000002
# define IIAPI_UP_I1WIDTH               0x00000004
# define IIAPI_UP_I2WIDTH               0x00000008
# define IIAPI_UP_I4WIDTH               0x00000010
# define IIAPI_UP_I8WIDTH		0x04000000
# define IIAPI_UP_F4WIDTH               0x00000020
# define IIAPI_UP_F4PRECISION           0x00000040
# define IIAPI_UP_F8WIDTH               0x00000080
# define IIAPI_UP_F8PRECISION           0x00000100
# define IIAPI_UP_MPREC                 0x00000200
# define IIAPI_UP_F4STYLE               0x00000400
# define IIAPI_UP_F8STYLE               0x00000800
# define IIAPI_UP_DECFLOAT              0x00001000
# define IIAPI_UP_MSIGN                 0x00002000
# define IIAPI_UP_MLORT                 0x00004000
# define IIAPI_UP_DECIMAL               0x00008000
# define IIAPI_UP_MATHEX                0x00010000
# define IIAPI_UP_STRTRUNC              0x00020000
# define IIAPI_UP_DATE_FORMAT           0x00040000
# define IIAPI_UP_TIMEZONE              0x00080000
# define IIAPI_UP_NLANGUAGE             0x00100000
# define IIAPI_UP_CENTURY_BOUNDARY      0x00200000
# define IIAPI_UP_MAX_SEGMENT_LEN       0x00400000
# define IIAPI_UP_TRACE_FUNC	        0x00800000
# define IIAPI_UP_PROMPT_FUNC	        0x01000000
# define IIAPI_UP_EVENT_FUNC	        0x02000000
/*	 IIAPI_UP_I8WIDTH		0x04000000  See above. */
# define IIAPI_UP_DATE_ALIAS		0x08000000

    II_CHAR     *up_decfloat;
    II_CHAR     *up_mathex;
    II_CHAR     *up_timezone;
    II_CHAR     *up_strtrunc;
    II_CHAR	*up_date_alias;
    II_LONG	up_max_seglen;
    VOID	(*up_trace_func)();
    VOID	(*up_prompt_func)();
    VOID	(*up_event_func)();

}IIAPI_USRPARM;

/*
** Name: IIAPI_ENVHNDL
**
** Description:
**      This data type defines the environment handle.
**
** History:
**	17-Jan-96 (gordy)
**	    Created.
**	24-May-96 (gordy)
**	    Removed en_memTag.
*/

typedef struct IIAPI_ENVHNDL
{
    IIAPI_HNDL		en_header;
    
    /*
    ** Environment specific parameters.
    */
    II_LONG		en_initCount;
    II_LONG		en_version;
    PTR			en_adf_cb;
    IIAPI_USRPARM   	en_usrParm;
        
    /*
    ** Handle Queues.
    */
    MU_SEMAPHORE	en_semaphore;
    QUEUE		en_connHndlList;
    QUEUE		en_tranNameList;

} IIAPI_ENVHNDL;

/*
** Global functions.
*/

II_EXTERN IIAPI_ENVHNDL		*IIapi_createEnvHndl( II_LONG version );

II_EXTERN II_VOID		IIapi_deleteEnvHndl( IIAPI_ENVHNDL *envHndl );

II_EXTERN II_BOOL		IIapi_isEnvHndl( IIAPI_ENVHNDL *envHndl );

II_EXTERN IIAPI_ENVHNDL 	*IIapi_getEnvHndl( IIAPI_HNDL *handle );

II_EXTERN IIAPI_STATUS		IIapi_setEnvironParm(
					IIAPI_SETENVPRMPARM *envParm );

II_EXTERN II_VOID		IIapi_clearEnvironParm(
					IIAPI_ENVHNDL *envHndl );

# endif	/* __APIENHND_H__ */
