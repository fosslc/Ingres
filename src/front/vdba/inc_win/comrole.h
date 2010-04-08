/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comrole.h: interface for the CmtRole, CmtFolderRole class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Role object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMROLE_HEADER)
#define COMROLE_HEADER
#include "combase.h"
#include "dmlrole.h"
#include "comaccnt.h"

class CmtFolderRole;
//
// Object: Role 
// *************
class CmtRole: public CmtItemData, public CmtProtect
{
public:
	CmtRole(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtRole(CaRole* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtRole(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderRole* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderRole* GetBackParent(){return m_pBackParent;}
	CaRole& GetRole(){return m_object;}
	void GetRole(CaRole*& pObject){pObject = new CaRole(m_object);}

protected:
	CaRole m_object;
	CmtFolderRole* m_pBackParent;

};

//
// Object: Folder Role 
// ********************
class CmtFolderRole: public CmtItemData, public CmtProtect
{
public:
	CmtFolderRole():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderRole();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtRole* SearchObject (CaLLQueryInfo* pInfo, CaRole* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject (CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtRole* > m_listObject;
	CmtNode* m_pBackParent;
};

#endif // COMROLE_HEADER