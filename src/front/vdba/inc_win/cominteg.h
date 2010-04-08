/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cominteg.h: interface for the CmtIntegrity, CmtFolderIntegrity class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Integrity object and folder
**
** History:
**
** 24-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMINTEG_HEADER)
#define COMINTEG_HEADER
#include "combase.h"
#include "dmlinteg.h"
#include "comaccnt.h"

class CmtFolderIntegrity;
class CmtTable;

//
// Object: Integrity 
// *****************
class CmtIntegrity: public CmtItemData, public CmtProtect
{
public:
	CmtIntegrity(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtIntegrity(CaIntegrity* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtIntegrity(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderIntegrity* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderIntegrity* GetBackParent(){return m_pBackParent;}
	CaIntegrity& GetIntegrity(){return m_object;}
	void GetIntegrity(CaIntegrity*& pObject){pObject = new CaIntegrity(m_object);}

protected:
	CaIntegrity m_object;
	CmtFolderIntegrity* m_pBackParent;

};

//
// Object: Folder Integrity 
// *************************
class CmtFolderIntegrity: public CmtItemData, public CmtProtect
{
public:
	CmtFolderIntegrity():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderIntegrity();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtTable* pBackParent){m_pBackParent = pBackParent;}
	CmtTable* GetBackParent(){return m_pBackParent;}
	CmtIntegrity* SearchObject (CaLLQueryInfo* pInfo, CaIntegrity* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtIntegrity* > m_listObject;
	CmtTable* m_pBackParent;
};

#endif // COMINTEG_HEADER