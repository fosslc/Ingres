/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

/*
** Name: apivalid.h
**
** Description:
**	This file defines parameter validation functions.
**
** History:
**      02-nov-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	11-Jan-95 (gordy)
**	    Added IIapi_validDescriptor().
**	22-Oct-04 (gordy)
**	    Return error code from IIapi_validDescriptor().
**	 5-Mar-09 (gordy)
**	    Added IIapi_validDataValue().
*/

# ifndef __APIVALID_H__
# define __APIVALID_H__

# include <apishndl.h>


/*
** Global Functions.
*/

II_EXTERN II_BOOL	IIapi_validParmCount( IIAPI_STMTHNDL *stmtHndl,
					      IIAPI_PUTPARMPARM *putParmParm );

II_EXTERN II_BOOL	IIapi_validPColCount( IIAPI_STMTHNDL *stmtHndl,
					      IIAPI_PUTCOLPARM *putColParm );

II_EXTERN II_BOOL	IIapi_validGColCount( IIAPI_STMTHNDL *stmtHndl,
					      IIAPI_GETCOLPARM *getColParm );

II_EXTERN II_ULONG	IIapi_validDescriptor( IIAPI_STMTHNDL *stmthndl,
					       II_LONG count,
					       IIAPI_DESCRIPTOR *descriptor );

II_EXTERN II_ULONG	IIapi_validDataValue( II_LONG count,
					      IIAPI_DESCRIPTOR *descriptor,
					      IIAPI_DATAVALUE *datavalue );

# endif				/* __APIVALID_H__ */
