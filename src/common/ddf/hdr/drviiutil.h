/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: drviiutil.h - Util method for the Data Dictionnary Driver for Ingres
**
** Description:
**  This file contains structure and functions to interface OpenAPI.
**
** History: 
**      02-mar-98 (marol01)
**          Created
**      06-aug-1999
**          Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#ifndef OIUTIL_INCLUDED
#define OIUTIL_INCLUDED

#include <ddfcom.h>
#include <iiapi.h>

#define OI_SELECT_QUERY		1
#define OI_EXECUTE_QUERY	2
#define OI_SYSTEM_QUERY		3
#define OI_PROCEDURE		4
#define BLOB_BUFFER_SIZE	32768
typedef struct __OI_Param 
{
	II_INT2				type;
	II_INT2				size;
	PTR					value;
	II_INT2				columnType;
	struct __OI_Param	*next;
} OI_Param, *POI_PARAM;

GSTATUS 
OI_Initialize(i4 timeout);

GSTATUS 
OI_Query_Type (
		char *stmt, 
		u_i4 *type);

GSTATUS 
OI_GetError( 
		II_PTR handle, 
		GSTATUS error );

GSTATUS 
OI_GetResult( 
		IIAPI_GENPARM 	*genParm,
		i4 timeout);

GSTATUS 
OI_GetQInfo( 
		II_PTR	stmtHandle,
		u_i4		*count,
		i4	timeout);

GSTATUS 
OI_GetDescriptor( 
		IIAPI_GETDESCRPARM	*getDescrParm, 
		II_PTR stmtHandle,
		i4	timeout);

GSTATUS 
OI_PutParam( 
		IIAPI_QUERYPARM	*queryParm, 
		POI_PARAM				params, 
		II_INT2					VarCounter,
		i4					timeout);

GSTATUS 
OI_Close( 
		II_PTR	stmtHandle,
		i4	timeout);

#endif
