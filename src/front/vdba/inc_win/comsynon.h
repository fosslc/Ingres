/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comsynon.h: interface for the CmtSynonym, CmtFolderSynonym class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Synonym object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMSYNONYM_HEADER)
#define COMSYNONYM_HEADER
#include "combase.h"
#include "dmlsynon.h"
#include "comaccnt.h"

class CmtFolderSynonym;
class CmtDatabase;

//
// Object: Synonym 
// *****************
class CmtSynonym: public CmtItemData, public CmtProtect
{
public:
	CmtSynonym(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtSynonym(CaSynonym* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtSynonym(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderSynonym* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderSynonym* GetBackParent(){return m_pBackParent;}
	CaSynonym& GetSynonym(){return m_object;}
	void GetSynonym(CaSynonym*& pObject){pObject = new CaSynonym(m_object);}

protected:
	CaSynonym m_object;
	CmtFolderSynonym* m_pBackParent;

};

//
// Object: Folder Synonym 
// *************************
class CmtFolderSynonym: public CmtItemData, public CmtProtect
{
public:
	CmtFolderSynonym():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderSynonym();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtDatabase* GetBackParent(){return m_pBackParent;}
	CmtSynonym* SearchObject (CaLLQueryInfo* pInfo, CaSynonym* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtSynonym* > m_listObject;
	CmtDatabase* m_pBackParent;
};

#endif // COMSYNONYM_HEADER