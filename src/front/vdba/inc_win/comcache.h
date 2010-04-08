/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comcache.h: interface for the CmtCache class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The main entry of the Muti-thread cache
**
** History:
**
** 21-Nov-2000 (uk$so01)
**    Created
**/

#if !defined (COMMUTITHREAD_CACHE_HEADER)
#define COMMUTITHREAD_CACHE_HEADER
#include "comaccnt.h" // m_folderAccount
#include "qryinfo.h"  // CaLLQueryInfo

// ------------------------------------------------------------------
// The main entry of the cache is Node:
// The cache is a tree hierachy.

class CmtCache: public CmtItemData, public CmtProtect
{
public:
	CmtCache();
	virtual ~CmtCache(){}
	BOOL Initialize();


	CmtFolderAccount& GetFolderAccount(){return m_folderAccount;}

protected:
	//
	// The main entry of INGCHSVR COM Server cache is an ACCOUNT:
	// SO, the cache is a list of accounts:
	CmtFolderAccount m_folderAccount;

private:
//	CaNodeDataAccess m_VirtualNodeAccess;
};



#endif // !defined (COMMUTITHREAD_CACHE_HEADER)
