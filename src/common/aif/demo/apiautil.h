/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiautil.h
**
** Description:
**	Global definitions for asynchronous OpenAPI demo utility module.
*/


/*
** Global functions
*/

extern	IIAPI_STATUS	IIdemo_conn( II_PTR, char *, II_PTR *, II_BOOL );
extern	IIAPI_STATUS	IIdemo_disconn( II_PTR *, II_BOOL );
extern	IIAPI_STATUS	IIdemo_abort( II_PTR  *, II_BOOL );
extern	IIAPI_STATUS	IIdemo_autocommit( II_PTR  *, II_PTR *, II_BOOL );
extern	IIAPI_STATUS	IIdemo_query( II_PTR *, II_PTR *, char *, II_BOOL );
extern	void		IIdemo_checkError( char *, IIAPI_GENPARM * );

