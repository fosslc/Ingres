/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comgroup.h: interface for the CmtGroup, CmtFolderGroup class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Group object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMGROUP_HEADER)
#define COMGROUP_HEADER
#include "combase.h"
#include "dmlgroup.h"
#include "comaccnt.h"

class CmtFolderGroup;
//
// Object: Group 
// *************
class CmtGroup: public CmtItemData, public CmtProtect
{
public:
	CmtGroup(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtGroup(CaGroup* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtGroup(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderGroup* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderGroup* GetBackParent(){return m_pBackParent;}
	CaGroup& GetGroup(){return m_object;}
	void GetGroup(CaGroup*& pObject){pObject = new CaGroup(m_object);}

protected:
	CaGroup m_object;
	CmtFolderGroup* m_pBackParent;

};

//
// Object: Folder Group 
// ********************
class CmtFolderGroup: public CmtItemData, public CmtProtect
{
public:
	CmtFolderGroup():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderGroup();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtGroup* SearchObject (CaLLQueryInfo* pInfo, CaGroup* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtGroup* > m_listObject;
	CmtNode* m_pBackParent;
};

#endif // COMGROUP_HEADER