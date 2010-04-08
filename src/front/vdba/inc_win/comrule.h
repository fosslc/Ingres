/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comrule.h: interface for the CmtRule, CmtFolderRule class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Rule object and folder
**
** History:
**
** 24-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMRULE_HEADER)
#define COMRULE_HEADER
#include "combase.h"
#include "dmlrule.h"
#include "comaccnt.h"

class CmtFolderRule;
class CmtTable;

//
// Object: Rule 
// *****************
class CmtRule: public CmtItemData, public CmtProtect
{
public:
	CmtRule(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtRule(CaRule* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtRule(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderRule* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderRule* GetBackParent(){return m_pBackParent;}
	CaRule& GetRule(){return m_object;}
	void GetRule(CaRule*& pObject){pObject = new CaRule(m_object);}

protected:
	CaRule m_object;
	CmtFolderRule* m_pBackParent;

};

//
// Object: Folder Rule 
// *************************
class CmtFolderRule: public CmtItemData, public CmtProtect
{
public:
	CmtFolderRule():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderRule();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtTable* pBackParent){m_pBackParent = pBackParent;}
	CmtTable* GetBackParent(){return m_pBackParent;}
	CmtRule* SearchObject (CaLLQueryInfo* pInfo, CaRule* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject (CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtRule* > m_listObject;
	CmtTable* m_pBackParent;
};

#endif // COMRULE_HEADER