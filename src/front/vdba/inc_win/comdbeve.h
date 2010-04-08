/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comdbeve.h: interface for the CmtDBEvent, CmtFolderDBEvent class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : DBEvent object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMDBEVENT_HEADER)
#define COMDBEVENT_HEADER
#include "combase.h"
#include "dmldbeve.h"
#include "comaccnt.h"

class CmtFolderDBEvent;
class CmtDatabase;

//
// Object: DBEvent 
// *****************
class CmtDBEvent: public CmtItemData, public CmtProtect
{
public:
	CmtDBEvent(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtDBEvent(CaDBEvent* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtDBEvent(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderDBEvent* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderDBEvent* GetBackParent(){return m_pBackParent;}
	CaDBEvent& GetDBEvent(){return m_object;}
	void GetDBEvent(CaDBEvent*& pObject){pObject = new CaDBEvent(m_object);}

protected:
	CaDBEvent m_object;
	CmtFolderDBEvent* m_pBackParent;

};

//
// Object: Folder DBEvent 
// *************************
class CmtFolderDBEvent: public CmtItemData, public CmtProtect
{
public:
	CmtFolderDBEvent():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderDBEvent();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtDatabase* GetBackParent(){return m_pBackParent;}
	CmtDBEvent* SearchObject (CaLLQueryInfo* pInfo, CaDBEvent* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtDBEvent* > m_listObject;
	CmtDatabase* m_pBackParent;
};

#endif // COMDBEVENT_HEADER