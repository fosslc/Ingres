/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apimisc.h
**
** Description:
**	This file contains miscellanous function definitions.
**
** History:
**      02-nov-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Feb-95 (gordy)
**	    Added ADF interface.
**	10-Feb-95 (gordy)
**	    Moved IIapi_appCallback().
**	 8-Mar-95 (gordy)
**	    Added IIapi_initialized() and IIapi_apiVersion().
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	17-Jan-96 (gordy)
**	    Added environment handles.  Implemented short functions as macros.
**	15-Mar-96 (gordy)
**	    Added IIapi_isVAR().
**	24-May-96 (gordy)
**	    Added IIapi_newMemTag() and IIapi_freeMemTag() for
**	    tagged memory support.
**	11-Nov-96 (gordy)
**	    Converted IIapi_newMemTag() and IIapi_freeMemTag() to
**	    macros (its amazing what you'll find in the GL/CL).
**	14-Nov-96 (gordy)
**	    Added IIapi_thread() for multi-thread support.
**	18-Feb-97 (gordy)
**	    Moved PARMNMEM stuff here from various other files.
**	05-Oct-2000 (lunbr01)  Bug 102766
**	    Added auto variable to IIapi_freeMemTag macro to prevent
**          access violation on call to free the tag when the variable
**          passed in is contained the memory being freed.
**	 1-Mar-01 (gordy)
**	    Added IIapi_isUCS2() for Unicode support.
**	12-Jul-02 (gordy)
**	    Changed name of variable in IIapi_freeMemTag macro.  It was
**	    conflicting with the actual name of the macro parameter.
**	25-Oct-06 (gordy)
**	    Added IIapi_isLocator().
**	25-Mar-10 (gordy)
**	    Enhanced GCA parameter block to support packed byte stream
**	    buffer without tagged memory.
*/

# ifndef __APIMISC_H__
# define __APIMISC_H__

# include <apienhnd.h>

/*
** The following functions are implemented as macros.
*/

# define IIapi_isQueEmpty( q )	(((QUEUE *)(q))->q_next == ((QUEUE *)(q)))

# define IIapi_initialized()	(IIapi_static != NULL)

# define IIapi_defaultEnvHndl()	\
	(IIapi_static ? (IIAPI_ENVHNDL *)IIapi_static->api_env_default : NULL)


/*
** Global Functions.
*/

II_EXTERN IIAPI_ENVHNDL	*	IIapi_initAPI( II_LONG, II_LONG );
II_EXTERN II_BOOL		IIapi_termAPI( IIAPI_ENVHNDL * );
II_EXTERN II_VOID		IIapi_appCallback( IIAPI_GENPARM *, 
						   IIAPI_HNDL *, IIAPI_STATUS );
II_EXTERN IIAPI_THREAD *	IIapi_thread( II_VOID );
II_EXTERN II_BOOL		IIapi_isVAR( IIAPI_DT_ID );
II_EXTERN II_BOOL		IIapi_isBLOB( IIAPI_DT_ID );
II_EXTERN II_BOOL		IIapi_isUCS2( IIAPI_DT_ID );
II_EXTERN II_BOOL		IIapi_isLocator( IIAPI_DT_ID );

# endif				/* __APIMISC_H__ */
