/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlview.h: interface for the CtrfView, CtrfFolderView class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : View object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (TRFVIEW_HEADER)
#define TRFVIEW_HEADER
#include "trctldta.h"
#include "trfcolum.h"
#include "dmlview.h"
#include "comsessi.h"

//
// Object: View 
// ************************************************************************************************
class CtrfView: public CtrfItemData
{
	DECLARE_SERIAL(CtrfView)
public:
	CtrfView(): CtrfItemData(O_VIEW)
	{
		m_pFolderColumn = NULL;
	}
	CtrfView(CaView* pObj): CtrfItemData(O_VIEW)
	{
		m_pFolderColumn = NULL;
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}

	void Initialize();
	virtual ~CtrfView(){};
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetViewImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CtrfFolderColumn* GetFolderColumn(){return m_pFolderColumn;}

	CaView& GetObject(){return m_object;}
	void GetObject(CaView*& pObject){pObject = new CaView(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaView m_object;
	CtrfFolderColumn* m_pFolderColumn;

};


//
// Object: Folder of View 
// ************************************************************************************************
class CtrfFolderView: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderView)
public:
	CtrfFolderView();
	virtual ~CtrfFolderView();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_VIEW);}


	CtrfItemData* SearchObject(CaView* pObj, BOOL bMatchOwner = TRUE);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;

};

#endif // TRFVIEW_HEADER