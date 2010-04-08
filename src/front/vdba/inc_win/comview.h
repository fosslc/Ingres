/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comview.h: interface for the CmtView, CmtFolderView class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : View object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMVIEW_HEADER)
#define COMVIEW_HEADER
#include "combase.h"
#include "dmlview.h"
#include "comaccnt.h"
#include "comcolum.h"

class CmtFolderView;
class CmtDatabase;

//
// Object: View 
// *************
class CmtView: public CmtItemData, public CmtProtect
{
	DECLARE_SERIAL(CmtView)
public:
	CmtView(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtView(CaView* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtView(){}
	virtual CmtSessionManager* GetSessionManager();
	void Initialize()
	{
		m_folderColumn.SetFolderType(CmtFolderColumn::F_COLUMN_VIEW);
		m_folderColumn.SetBackParent(this);
	}

	void SetBackParent(CmtFolderView* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderView* GetBackParent(){return m_pBackParent;}
	CaView& GetView(){return m_object;}
	void GetView(CaView*& pObject){pObject = new CaView(m_object);}

	CmtFolderColumn& GetFolderColumn(){return m_folderColumn;}

protected:
	CaView m_object;
	CmtFolderView* m_pBackParent;

	CmtFolderColumn m_folderColumn;
};

//
// Object: Folder View 
// ********************
class CmtFolderView: public CmtItemData, public CmtProtect
{
public:
	CmtFolderView():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderView();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtDatabase* GetBackParent(){return m_pBackParent;}
	CmtView* SearchObject (CaLLQueryInfo* pInfo, CaView* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtView* > m_listObject;
	CmtDatabase* m_pBackParent;
};

#endif // COMVIEW_HEADER