/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlindex.h: interface for the CtrfIndex, CtrfFolderIndex class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Index object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFINDEX_HEADER)
#define TRFINDEX_HEADER
#include "trctldta.h"
#include "dmlindex.h"
#include "comsessi.h"



//
// Object: Index 
// *************
class CtrfIndex: public CtrfItemData
{
	DECLARE_SERIAL(CtrfIndex)
public:
	CtrfIndex(): CtrfItemData(O_INDEX){}
	CtrfIndex(CaIndex* pObj): CtrfItemData(O_INDEX)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfIndex(){}
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetIndexImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	CaIndex& GetObject(){return m_object;}
	void GetObject(CaIndex*& pObject){pObject = new CaIndex(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaIndex m_object;
};



//
// Object: Folder of Index 
// ***********************
class CtrfFolderIndex: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderIndex)
public:
	CtrfFolderIndex();
	virtual ~CtrfFolderIndex();


	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_INDEX);}


	CtrfItemData* SearchObject(CaIndex* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;

};



#endif // TRFINDEX_HEADER
