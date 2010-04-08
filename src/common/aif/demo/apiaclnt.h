/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiaclnt.h
**
** Description:
**	Global definitions for asynchronous OpenAPI demo client module.
*/


/*
** Global functions
*/

extern	IIAPI_STATUS	IIdemo_init_client( II_PTR, char *, II_BOOL );
extern	void		IIdemo_run_client( int, II_BOOL, II_BOOL );
extern	void		IIdemo_term_client( II_BOOL, II_BOOL );

