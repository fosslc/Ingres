/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiasvr.h
**
** Description:
**	Global definitions for asynchronous OpenAPI demo server module.
*/

/*
** Global functions
*/

extern	IIAPI_STATUS	IIdemo_init_server( II_PTR, char *, II_BOOL );
extern	void		IIdemo_run_server( II_BOOL, II_BOOL  );
extern	void		IIdemo_term_server( II_BOOL, II_BOOL );

