/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apierhnd.h
**
** Description:
**	This file contains functions which manages error handles within
**      connection , transaction and statement handles.
**
** History:
**      02-nov-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 8-Mar-95 (gordy)
**	    Added missing prototypes.
**	 9-Jun-95 (gordy)
**	    Added status to IIapi_localError().
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	11-Oct-95 (gordy)
**	    Removed IIapi_initError() and IIapi_termError().  We do not
**	    use individual message files any longer.  The only other
**	    reason to call ERinit() is for semaphore routines.  Client
**	    side API does not need this and if we are in a server, then
**	    server code should be setting up ER for us.
**	 8-Mar-96 (gordy)
**	    Fixed processing of messages spanning buffers.
**	 2-Oct-96 (gordy)
**	    Added IIapi_saveErrors() and moved IIAPI_WORST_STATUS
**	    here from api.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Jul-06 (gordy)
**	    Added IIapi_xaError().
*/

# ifndef __APIERHND_H__
# define __APIERHND_H__


/*
** The following depends on the status values
** in iiapi.h being ordered with SUCCESS being
** the smallest value and working upward through
** the more critical status values.
*/

# define IIAPI_WORST_STATUS( s1, s2 )	( ((s1) > (s2)) ? (s1) : (s2) )


/*
** Name: IIAPI_ERRORHNDL
**
** Description:
**	Defines an error handle.
**
**	er_id		API handle header information.
**
**	er_type		Handle type (ERROR, WARNING, MESSAGE).
**
**	er_SQLSTATE	SQLSTATE value.
**
**	er_errorCode	Host DBMS error code.
**
**	er_message	Message text.
**
**	er_serverInfoAvail	TRUE if next member set.
**
**	er_serverInfo	Server error information block.
*/

typedef struct IIAPI_ERRORHNDL
{
    
    IIAPI_HNDLID	er_id;
    II_LONG		er_type;
    II_CHAR		er_SQLSTATE[ 6 ];
    II_LONG		er_errorCode;
    II_CHAR		*er_message;
    II_BOOL		er_serverInfoAvail;
    IIAPI_SVR_ERRINFO	*er_serverInfo;
    
} IIAPI_ERRORHNDL;


/*
** Global Functions.
*/

II_EXTERN IIAPI_ERRORHNDL *	IIapi_createErrorHndl( IIAPI_HNDL *handle );

II_EXTERN IIAPI_STATUS		IIapi_serverError( IIAPI_HNDL *,
						   IIAPI_SVR_ERRINFO * );

II_EXTERN II_BOOL		IIapi_localError( IIAPI_HNDL *handle,
						  II_LONG errorCode, 
						  char *SQLState, 
						  IIAPI_STATUS status );

II_EXTERN II_BOOL		IIapi_xaError( IIAPI_HNDL *handle, 
					       II_LONG xaError );

II_EXTERN IIAPI_ERRORHNDL *	IIapi_getErrorHndl( IIAPI_HNDL *handle );

II_EXTERN II_VOID		IIapi_cleanErrorHndl( IIAPI_HNDL *handle);

II_EXTERN II_VOID		IIapi_clearAllErrors( IIAPI_HNDL *handle );

II_EXTERN IIAPI_STATUS		IIapi_errorStatus( IIAPI_HNDL *handle );

II_EXTERN IIAPI_HNDL *		IIapi_saveErrors( IIAPI_HNDL *handle );

# endif /* __APIERHND_H__ */
