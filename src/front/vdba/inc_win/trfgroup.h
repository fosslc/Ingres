/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : trfgroup.h: interface for the CtrfGroup, CtrfFolderGroup class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Group object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFGROUP_HEADER)
#define TRFGROUP_HEADER
#include "trctldta.h"
#include "dmlgroup.h"

//
// Object: Group
// *************
class CtrfGroup: public CtrfItemData
{
	DECLARE_SERIAL (CtrfGroup)
public:
	CtrfGroup(): CtrfItemData(O_GROUP){}
	CtrfGroup(CaGroup* pObj): CtrfItemData(O_GROUP)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfGroup(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetGroupImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaGroup& GetObject(){return m_object;}
	void GetObject(CaGroup*& pObject){pObject = new CaGroup(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaGroup m_object;
};


//
// Object: Folder of Group
// ***********************
class CtrfFolderGroup: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderGroup)
public:
	CtrfFolderGroup();
	virtual ~CtrfFolderGroup();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_GROUP);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaGroup* pObj);

protected:
	CtrfItemData m_EmptyNode;

};

#endif // TRFGROUP_HEADER
