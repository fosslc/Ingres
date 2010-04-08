/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apiehndl.h
**
** Description:
**	This file contains the definition of the database event handle.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	 8-Mar-96 (gordy)
**	    Fixed processing of messages spanning buffers.
**	 2-Oct-96 (gordy)
**	    Renamed things to distinguish between Database
**	    events and API events.
**	 3-Sep-98 (gordy)
**	    Added function to abort handle.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Updated function parameter accordingly.
*/

# ifndef __APIEHNDL_H__
# define __APIEHNDL_H__

# include <apichndl.h>


/*
** Name: IIAPI_DBEVCB
** 
** Description:
**	Information associated with a Database Event.
**
**	When database event handles are matched to this 
**	database event, they are placed on the database 
**	event handle list and ev_notify is incremented.  
**	When the callback is made for a database event 
**	handle, ev_notify is decremented.  Database 
**	events which occur after the current database
**	event may not be dispatched until all the call-
**	backs have been made for the current database 
**	event (ev_notify == 0).  This permits the app-
**	lication a chance to restart database event 
**	retrievals which may match the subsequent 
**	database events.
**
** History:
**	25-Apr-95 (gordy)
**	    Created.
*/

typedef struct _IIAPI_DBEVCB
{

    IIAPI_HNDLID	ev_id;

    IIAPI_CONNHNDL	*ev_connHndl;		/* Parent */
    QUEUE		ev_dbevHndlList;	/* Children */
    II_LONG		ev_notify;		/* # outstanding callbacks */

    /*
    ** Database Event Info
    */
    II_CHAR		*ev_name;
    II_CHAR		*ev_owner;
    II_CHAR		*ev_database;
    IIAPI_DATAVALUE	ev_time;
    II_LONG		ev_count;
    IIAPI_DESCRIPTOR	*ev_descriptor;
    IIAPI_DATAVALUE	*ev_data;

} IIAPI_DBEVCB;




/*
** Name: IIAPI_DBEVHNDL
**
** Description:
**      This data type defines the database event handle.
**
**	Active database event handles are placed on a queue 
**	of the connection handle.  When a database event is
**	matched to a database event handle, the database
**	event handle is moved to a queue of the database
**	event control block.  The parent data structure 
**	(connection handle or database event control block) 
**	is accessible through eh_parent.
**
**	Cancelled queries are left on the same queue 
**	as active queries until they are closed.  The
**	eh_cancelled flag is used to distinguish the
**	two cases so that matching of database events
**	with cancelled queries can be avoided.
**
**	The eh_done flag is used to indicate that database
**	event data has been retrieved (IIapi_getColumns() 
**	has been called) and that subsequent attepts to get
**	the data should return NO_DATA status.
**
**	NOTE: the functionality provided by eh_done could
**	also be achieved utilizing the state tables and
**	would provide a more extensible solution.  However,
**	it would require two new API events and at least
**	one additional event state and is beyond the scope
**	of what is currently required.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	 2-Oct-96 (gordy)
**	    Simplified function callbacks.
*/

typedef struct _IIAPI_DBEVHNDL
{
    IIAPI_HNDL     	eh_header;
    
    /*
    ** Database Event specific parameters.
    */
    II_PTR		eh_parent;	/* Connection handle or DBEvent CB */
    char		*eh_dbevOwner;
    char		*eh_dbevName;
    II_BOOL		eh_cancelled;	/* Database Event cancelled */
    II_BOOL		eh_done;	/* Database Event data retrieved */
    
    /*
    ** API Function call info.
    */
    II_BOOL		eh_callback;
    IIAPI_GENPARM	*eh_parm;

} IIAPI_DBEVHNDL;



/*
** Global functions.
*/

II_EXTERN II_VOID		IIapi_createDbevCB( IIAPI_CONNHNDL * );
II_EXTERN II_VOID		IIapi_processDbevCB( IIAPI_CONNHNDL * );
II_EXTERN IIAPI_DBEVHNDL	*IIapi_createDbevHndl(IIAPI_CATCHEVENTPARM *);
II_EXTERN II_VOID		IIapi_deleteDbevHndl( IIAPI_DBEVHNDL * );
II_EXTERN II_VOID		IIapi_abortDbevHndl( IIAPI_DBEVHNDL *, II_LONG,
						     char *, IIAPI_STATUS );
II_EXTERN II_BOOL		IIapi_isDbevHndl( IIAPI_DBEVHNDL *);
II_EXTERN IIAPI_DBEVHNDL 	*IIapi_getDbevHndl( IIAPI_HNDL *);


# endif	/* __APIEHNDL_H__ */
