/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comcache.cpp: implementation for the CmtCache class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The main entry of the Muti-thread cache
**
** History:
**
** 21-Nov-2000 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "comcache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CmtCache::CmtCache() :CmtItemData(), CmtProtect()
{
}


BOOL CmtCache::Initialize()
{
	try
	{
		ASSERT (FALSE);
		/*
		if (!m_VirtualNodeAccess.GetStatus())
			return FALSE;
		if (!m_VirtualNodeAccess.InitNodeList())
			return FALSE;
		*/
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
	return TRUE;
}


