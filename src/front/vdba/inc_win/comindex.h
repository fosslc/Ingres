/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comindex.h: interface for the CmtIndex, CmtFolderIndex class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Index object and folder
**
** History:
**
** 24-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMINDEX_HEADER)
#define COMINDEX_HEADER
#include "combase.h"
#include "dmlindex.h"
#include "comaccnt.h"

class CmtFolderIndex;
class CmtTable;

//
// Object: Index 
// *****************
class CmtIndex: public CmtItemData, public CmtProtect
{
public:
	CmtIndex(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtIndex(CaIndex* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtIndex(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderIndex* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderIndex* GetBackParent(){return m_pBackParent;}
	CaIndex& GetIndex(){return m_object;}
	void GetIndex(CaIndex*& pObject){pObject = new CaIndex(m_object);}

protected:
	CaIndex m_object;
	CmtFolderIndex* m_pBackParent;

};

//
// Object: Folder Index 
// *************************
class CmtFolderIndex: public CmtItemData, public CmtProtect
{
public:
	CmtFolderIndex():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderIndex();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtTable* pBackParent){m_pBackParent = pBackParent;}
	CmtTable* GetBackParent(){return m_pBackParent;}
	CmtIndex* SearchObject (CaLLQueryInfo* pInfo, CaIndex* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtIndex* > m_listObject;
	CmtTable* m_pBackParent;
};

#endif // COMINDEX_HEADER