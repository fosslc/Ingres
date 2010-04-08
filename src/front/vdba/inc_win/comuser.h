/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmluser.h: interface for the CmtUser, CmtFolderUser class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : User object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/

#if !defined (COMUSER_HEADER)
#define COMUSER_HEADER
#include "combase.h"
#include "dmluser.h"
#include "comaccnt.h"

class CmtFolderUser;
//
// Object: User 
// *************
class CmtUser: public CmtItemData, public CmtProtect
{
public:
	CmtUser(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtUser(CaUser* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT (pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtUser(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderUser* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderUser* GetBackParent(){return m_pBackParent;}
	CaUser& GetUser(){return m_object;}
	void GetUser(CaUser*& pObject){pObject = new CaUser(m_object);}

protected:
	CaUser m_object;
	CmtFolderUser* m_pBackParent;
};


//
// Object: Folder User 
// ********************
class CmtFolderUser: public CmtItemData, public CmtProtect
{
public:
	CmtFolderUser():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderUser();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtUser* SearchObject (CaLLQueryInfo* pInfo, CaUser* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtUser* > m_listObject;
	CmtNode* m_pBackParent;

};


#endif // COMUSER_HEADER
