/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apisql.h
**
** Description:
**	SQL State Machine interfaces.
**
** History:
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	 3-Jul-97 (gordy)
**	    State machine interface is now through initialization
**	    function which sets the dispatch table entries.
*/

# ifndef __APISQL_H__
# define __APISQL_H__

/*
** Global functions.
*/

II_EXTERN IIAPI_STATUS		IIapi_sql_init( VOID );
II_EXTERN IIAPI_STATUS		IIapi_jas_init( VOID );
II_EXTERN IIAPI_STATUS		IIapi_sql_cinit( VOID );
II_EXTERN IIAPI_STATUS		IIapi_sql_tinit( VOID );
II_EXTERN IIAPI_STATUS		IIapi_sql_sinit( VOID );
II_EXTERN IIAPI_STATUS		IIapi_sql_dinit( VOID );

# endif /* __APISQL_H__ */
