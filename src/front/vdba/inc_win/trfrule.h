/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlrule.h: interface for the CtrfRule, CtrfFolderRule class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Rule object and Folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFRULE_HEADER)
#define TRFRULE_HEADER
#include "dmlrule.h"
#include "trctldta.h"

//
// Object: RULE
// ************
class CtrfRule: public CtrfItemData
{
	DECLARE_SERIAL(CtrfRule)
public:
	CtrfRule(): CtrfItemData(O_RULE){}
	CtrfRule(CaRule* pObj): CtrfItemData(O_RULE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfRule(){}
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetRuleImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaRule& GetObject(){return m_object;}
	void GetObject(CaRule*& pObject){pObject = new CaRule(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaRule m_object;
};



//
// Folder of Rule:
// ***************
class CtrfFolderRule: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderRule)
public:
	CtrfFolderRule();
	virtual ~CtrfFolderRule();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_INDEX);}

	CtrfItemData* SearchObject(CaRule* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;
};


#endif // TRFRULE_HEADER