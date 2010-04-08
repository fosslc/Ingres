/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlrole.h: interface for the CtrfRole, CtrfFolderRole class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Role object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFROLE_HEADER)
#define TRFROLE_HEADER
#include "trctldta.h"
#include "dmlrole.h"

//
// Object: Role
// ************
class CtrfRole: public CtrfItemData
{
	DECLARE_SERIAL (CtrfRole)
public:
	CtrfRole(): CtrfItemData(O_ROLE){}
	CtrfRole(CaRole* pObj): CtrfItemData(O_ROLE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfRole(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetRoleImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaRole& GetObject(){return m_object;}
	void GetObject(CaRole*& pObject){pObject = new CaRole(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaRole m_object;
};


//
// Object: Role
// ************
class CtrfFolderRole: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderRole)
public:
	CtrfFolderRole();
	virtual ~CtrfFolderRole();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_ROLE);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaRole* pObj);

protected:
	CtrfItemData m_EmptyNode;

};


#endif // TRFROLE_HEADER