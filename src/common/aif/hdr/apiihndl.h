/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apiihndl.h
**
** Description:
**	This file contains the definition of the GCA identifier handle
**	management functions.
**
** History:
**	20-Dec-94 (gordy)
**	    Created.
**	16-Feb-95 (gordy)
**	    Added unique IDs.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

# ifndef __APIIHNDL_H__
# define __APIIHNDL_H__


/*
** Name: IIAPI_IDHNDL
**
** Description:
**	Data structure representing GCA identifiers.
**
**	id_id
**	    Common handle information.
**	id_type
**	    Type of ID: cursor, procedure, or repeat query.
**	id_gca_id
**	    GCA ID information.
**
** History:
**	20-Dec-94 (gordy)
**	    Created.
*/

typedef struct _IIAPI_IDHNDL
{

    IIAPI_HNDLID	id_id;
    II_LONG		id_type;

#define	IIAPI_ID_CURSOR		0x01
#define	IIAPI_ID_PROCEDURE	0x02
#define	IIAPI_ID_REPEAT_QUERY	0x03

    GCA_ID		id_gca_id;

} IIAPI_IDHNDL;



/*
** Function Prototypes
*/

II_EXTERN IIAPI_IDHNDL	*IIapi_createIdHndl( II_LONG type, GCA_ID *gcaID );
II_EXTERN II_VOID	IIapi_deleteIdHndl( IIAPI_IDHNDL *idHndl );
II_EXTERN II_BOOL	IIapi_isIdHndl( IIAPI_IDHNDL *idHndl, II_LONG type );


# endif				/* __APIIHNDL_H__ */
