/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apilower.h
**
** Description:
**	This file defines the GCA interface routines.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	26-Oct-94 (gordy)
**	    Pass generic handle as input parameter.
**	20-Jan-95 (gordy)
**	    Flag GCA activity.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	13-Jun-95 (gordy)
**	    Added IIAPI_GCA_BUFF to encapsulate GCA parms and 
**	    provide message buffering.  Moved IIAPI_CLOSURE
**	    to apilower.c.  
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	19-Jan-96 (gordy)
**	    Added global data structure.  Convert short functions to macros.
**	15-Jul-96 (gordy)
**	    Added timeout parameter to IIapi_rcvNormalGCA().
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA().
**	18-Feb-97 (gordy)
**	    Moved PARMNMEM to apimisc.h.  Added IIapi_abortGCA() and
**	    IIapi_allocGCABuffer().  Simplified IIapi_connGCA() parms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Jul-02 (gordy)
**	    Added IIapi_expandGCABuffer() to handle long column values.
**	25-Mar-10 (gordy)
**	    Replaced GCA buffer with combined send/recv message buffer.
*/

# ifndef __APILOWER_H__
# define __APILOWER_H__

# include <apichndl.h>
# include <apimsg.h>


/*
** Name: IIapi_gcaActive
**
** Description:
**	Returns indication if GCA has active requests for the
**	current thread.  GCA receives on the expedited channel
**	are not included because the API never explicitly waits
**	for just these events.
**
** Input:
**	thread		Thread's control data
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if GCA request active, FALSE otherwise.
**
** History:
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
*/


# define IIapi_gcaActive( thread )	\
	(((IIAPI_THREAD *)(thread))->api_gca_active > 0)

/*
** Global Functions.
*/

II_EXTERN II_BOOL	IIapi_initGCA( II_LONG );
II_EXTERN II_VOID	IIapi_termGCA( II_VOID );
II_EXTERN IIAPI_STATUS	IIapi_connGCA( IIAPI_HNDL *, II_LONG );
II_EXTERN IIAPI_STATUS	IIapi_disconnGCA( IIAPI_CONNHNDL * );
II_EXTERN IIAPI_STATUS	IIapi_abortGCA( IIAPI_CONNHNDL * );
II_EXTERN IIAPI_STATUS	IIapi_sndGCA( IIAPI_HNDL *, IIAPI_MSG_BUFF *, II_PTR );
II_EXTERN IIAPI_STATUS	IIapi_rcvNormalGCA( IIAPI_HNDL *, IIAPI_MSG_BUFF *,
					    II_LONG );
II_EXTERN II_VOID	IIapi_rcvExpediteGCA( IIAPI_HNDL *, IIAPI_MSG_BUFF * );
II_EXTERN IIAPI_STATUS	IIapi_resumeGCA( IIAPI_HNDL *, II_PTR );

# endif /* __APILOWER_H__ */
