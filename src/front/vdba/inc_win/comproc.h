/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comproc.h: interface for the CmtProcedure, CmtFolderProcedure class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Procedure object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMPROCEDURE_HEADER)
#define COMPROCEDURE_HEADER
#include "combase.h"
#include "dmlproc.h"
#include "comaccnt.h"

class CmtFolderProcedure;
class CmtDatabase;

//
// Object: Procedure 
// *****************
class CmtProcedure: public CmtItemData, public CmtProtect
{
public:
	CmtProcedure(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtProcedure(CaProcedure* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtProcedure(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderProcedure* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderProcedure* GetBackParent(){return m_pBackParent;}
	CaProcedure& GetProcedure(){return m_object;}
	void GetProcedure(CaProcedure*& pObject){pObject = new CaProcedure(m_object);}

protected:
	CaProcedure m_object;
	CmtFolderProcedure* m_pBackParent;

};

//
// Object: Folder Procedure 
// *************************
class CmtFolderProcedure: public CmtItemData, public CmtProtect
{
public:
	CmtFolderProcedure():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderProcedure();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtDatabase* GetBackParent(){return m_pBackParent;}
	CmtProcedure* SearchObject (CaLLQueryInfo* pInfo, CaProcedure* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtProcedure* > m_listObject;
	CmtDatabase* m_pBackParent;
};

#endif // COMPROCEDURE_HEADER