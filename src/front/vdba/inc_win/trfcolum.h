/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlcolum.h: interface for the CtrfColumn, CtrfFolderColumn class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Column object and Folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFCOLUMNS_HEADER)
#define TRFCOLUMNS_HEADER
#include "trctldta.h"
#include "dmlcolum.h"

//
// Object: Column 
// **************
class CtrfColumn: public CtrfItemData
{
	DECLARE_SERIAL(CtrfColumn)
public:
	CtrfColumn(): CtrfItemData(O_COLUMN){}
	CtrfColumn(CaColumn* pObj): CtrfItemData(O_COLUMN)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfColumn(){}

	CaColumn& GetObject(){return m_object;}
	void GetObject(CaColumn*& pObject){pObject = new CaColumn(m_object);}
	CaDBObject* GetDBObject(){return &m_object;}
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetColumnImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

protected:
	CaColumn m_object;
};


//
// Object: Folder of Column 
// ************************
class CtrfFolderColumn: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderColumn)
public:
	CtrfFolderColumn();
	virtual ~CtrfFolderColumn();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_COLUMN);}


	CtrfItemData* SearchObject(CaColumn* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

//	CtrfColumn* SearchColumn (LPCTSTR lpszName, BOOL b2QueryIfNotDone = TRUE);
	void SetTableParent(BOOL bSet){m_bParentIsTable = bSet;}

protected:
	CtrfItemData m_EmptyNode;
	BOOL m_bParentIsTable;

};


#endif // TRFCOLUMNS_HEADER