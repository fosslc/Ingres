/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apidspth.h
**
** Description:
**	Definition of API event dispatching interface.  The API
**	has two dispatching layers.  The upper level dispatcher
**	is used to process API events resulting from API function
**	invocations.  The lower level dispatcher is used to
**	process GCA request results from callback/completion
**	routines.
**
**	The upper level dispatcher processes handles top-down
**	so as to verify parent handle states prior to event
**	specific handling.  The lower level dispatcher processes
**	handles bottom-up so as to complete detailed processing
**	prior to general processing such as connection failure
**	or transaction aborts.
**	
**	In addition, the lower level dispatcher supports deletion
**	of the parameter block when dispatching completes by way
**	of a deletion function.  This feature is not supported in
**	the upper level interface since API function parameter
**	blocks are managed by the application.
**
**	Dispatching is serialized to ensure all related handle
**	processing has completed prior to dispatching subsequent
**	events.  In addition, callers of IIapi_serviceOpQue()
**	must also ensure serialization by checking for active
**	dispatch processing with IIapi_setDispatchFlag().
**
** History:
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	14-Nov-96 (gordy)
**	    Moved synchronization data structures to thread-local-storage.
**	 3-Jul-97 (gordy)
**          State machine interface now made through initialization
**          function which fill in the dispatch table entries.
**          Added IIapi_sm_init().
*/

# ifndef __APIDSPTH_H__
# define __APIDSPTH_H__

/*
** Function prototype for parameter block deletion function.
*/

typedef II_VOID  (*IIAPI_DELPARMFUNC)( II_PTR parmBlock );


/*
** Global Functions.
*/

II_EXTERN II_BOOL 	IIapi_setDispatchFlag( IIAPI_THREAD * );

II_EXTERN II_VOID 	IIapi_clearDispatchFlag( IIAPI_THREAD * );

II_EXTERN II_BOOL	IIapi_serviceOpQue( IIAPI_THREAD *thread );

II_EXTERN IIAPI_STATUS	IIapi_sm_init( II_UINT2 );

II_EXTERN II_VOID	IIapi_uiDispatch( IIAPI_EVENT, 
					  IIAPI_HNDL *, IIAPI_GENPARM * );

II_EXTERN II_VOID	IIapi_liDispatch( IIAPI_EVENT, IIAPI_HNDL *, 
					  II_PTR, IIAPI_DELPARMFUNC );

# endif /* __APIDSPTH_H__ */

