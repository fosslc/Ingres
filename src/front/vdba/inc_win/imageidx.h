/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : imageidx.h: common include file for 0-based index of image list
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : index of image list. bitmap, icon, ...
**
** History:
**
** 12-Dec-2000 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (IMAGExINDEX_HEADER)
#define IMAGExINDEX_HEADER

//
// Zero-base index of image list IDB_INGRESOBJECT16x16
// in rctools.rc
enum
{
	IM_FOLDER_CLOSED = 0,
	IM_FOLDER_OPENED,
	IM_FOLDER_NODE_LOCAL,
	IM_FOLDER_NODE_NORMAL,

	IM_FOLDER_DATABASE,
	IM_FOLDER_DATABASE_STAR,
	IM_FOLDER_DATABASE_STAR_LINK,
	IM_FOLDER_TABLE,
	IM_FOLDER_TABLE_STAR,
	IM_FOLDER_EMPTY
};
#define IM_FOLDER_NODE_SERVER IM_FOLDER_NODE_NORMAL


//
// Zero-base index of image list IDB_INGRESOBJECT2_16x16
// in rctools.rc
enum
{
	IM_DATABASE,
	IM_TABLE,
	IM_VIEW,
	IM_PROCEDURE,
	IM_DBEVENT,
	IM_SYNONYM,
	IM_INDEX,
	IM_RULE,
	IM_INTEGRITY,
	IM_COLUMN,
	IM_ALARM
};

//
// Zero-base index of image list IDB_INGRESOBJECT3_16x16
// in rctools.rc
enum
{
	IM_INSTALLATION,
	IM_USER,
	IM_GROUP,
	IM_ROLE,
	IM_PROFILE,
	IM_LOCATION
};

#endif // IMAGExINDEX_HEADER