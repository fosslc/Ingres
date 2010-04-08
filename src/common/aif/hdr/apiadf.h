/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apiadf.h
**
** Description:
**	Header file for API ADF Interface.
**
** History:
**	 9-Feb-95 (gordy)
**	    Created.
**	 8-Mar-95 (gordy)
**	    Replaced globals with access functions.
**	19-Jan-96 (gordy)
**	    Added global data structure.  Provide separate 
**	    initialization at session level.
*/

#ifndef _APIADF_INCLUDED
#define _APIADF_INCLUDED

# include <apienhnd.h>


/*
** External Functions
*/

II_EXTERN II_BOOL	IIapi_initADF( II_VOID );
II_EXTERN II_VOID	IIapi_termADF( II_VOID );
II_EXTERN II_BOOL	IIapi_initADFSession( IIAPI_ENVHNDL * );
II_EXTERN II_VOID	IIapi_termADFSession( IIAPI_ENVHNDL * );

#endif	/* _APIADF_INCLUDED */
