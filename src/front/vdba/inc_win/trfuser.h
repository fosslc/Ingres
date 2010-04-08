/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmluser.h: interface for the CtrfUser, CtrfFolderUser class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : User object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (TRFUSER_HEADER)
#define TRFUSER_HEADER
#include "trctldta.h"
#include "dmluser.h"

//
// Object: User 
// ************
class CtrfUser: public CtrfItemData
{
	DECLARE_SERIAL(CtrfUser)
public:
	CtrfUser(): CtrfItemData(O_USER){}
	CtrfUser(CaUser* pObj): CtrfItemData(O_USER)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfUser(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetUserImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	CaUser& GetObject(){return m_object;}
	void GetObject(CaUser*& pObject){pObject = new CaUser(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaUser m_object;
};


//
// Object: Folder of User 
// **********************
class CtrfFolderUser: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderUser)
public:
	CtrfFolderUser();
	virtual ~CtrfFolderUser();

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_PROFILE);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
//	virtual void Display (CTreeCtrl* pTree,  HTREEITEM hParent = NULL);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
//	HRESULT GetListObject(CTypedPtrList< CObList, CtrfUser* >*&listObj);
	CtrfItemData* SearchObject (CaUser* pObj);

protected:
	CtrfItemData m_EmptyNode;

};


#endif // TRFUSER_HEADER
