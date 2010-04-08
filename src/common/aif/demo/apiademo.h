/*
** Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: apiademo.h
**
** Description:
**	Global definitions for asynchronous OpenAPI demo control module.
*/


/*
** Constants
*/

#define	IIdemo_eventName	"api_event"	/* Name of database event */


/*
** Global functions
*/

extern	void	IIdemo_start_async();		/* Start async OpenAPI call */
extern	void	IIdemo_complete_async();	/* End async OpenAPI call */
extern	void	IIdemo_set_abort();		/* Abort operation */

