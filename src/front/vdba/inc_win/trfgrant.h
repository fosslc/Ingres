/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : trfgrant.h: interface for the CtrfGrantee, CtrfFolderGrantee class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Grantee object and folder
**
** History:
**
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    Created
**/


#if !defined (TRFGRANT_HEADER)
#define TRFGRANT_HEADER
#include "trctldta.h"
#include "dmlgrant.h"

//
// Object: CtrfGrantPrivilege
// ************************************************************************************************
class CtrfGrantPrivilege: public CtrfItemData
{
	DECLARE_SERIAL (CtrfGrantPrivilege)
public:
	CtrfGrantPrivilege(): CtrfItemData(O_PRIVILEGE){}
	CtrfGrantPrivilege(CaPrivilegeItem* pObj): CtrfItemData(O_PRIVILEGE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfGrantPrivilege(){}
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetGroupImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaPrivilegeItem& GetObject(){return m_object;}
	void GetObject(CaPrivilegeItem*& pObject){pObject = new CaPrivilegeItem(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaPrivilegeItem m_object;
};

//
// Object: CtrfGrantee
// ************************************************************************************************
class CtrfGrantee: public CtrfItemData
{
	DECLARE_SERIAL (CtrfGrantee)
public:
	CtrfGrantee(): CtrfItemData(O_GRANTEE){}
	CtrfGrantee(CaGrantee* pObj): CtrfItemData(O_GRANTEE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfGrantee(){}
	virtual void Initialize();
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetGroupImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaGrantee& GetObject(){return m_object;}
	void GetObject(CaGrantee*& pObject){pObject = new CaGrantee(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaGrantee m_object;
};


//
// Object: Folder of Grantee
// ************************************************************************************************
class CtrfFolderGrantee: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderGrantee)
	CtrfFolderGrantee();
public:
	CtrfFolderGrantee(int nObjectParentType);
	virtual ~CtrfFolderGrantee();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_GROUP);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaGrantee* pObj);

	void SetDisplayGranteePrivileges(BOOL bSet){m_bDisplayGranteePrivilege = bSet;}
protected:
	CtrfItemData m_EmptyNode;
	int  m_nObjectParentType;
	BOOL m_bDisplayGranteePrivilege;

};

#endif // TRFGRANT_HEADER
