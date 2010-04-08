/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmldetai.h: interface for the Get Detail of Object
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Get detail of object
**
** History:
**
** 25-Set-2000 (uk$so01)
**    Created
**/

#if !defined(DMLDETAIL_OBJECT_HEADER)
#define DMLDETAIL_OBJECT_HEADER
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dmlbase.h"
#include "qryinfo.h"


BOOL INGRESII_llQueryDetail (CaLLQueryInfo* pQueryInfo, CaDBObject* pBaseObject);



#endif // !defined(DMLDETAIL_OBJECT_HEADER)
