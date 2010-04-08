/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmltable.h: interface for the CtrfTable, CtrfFolderTable class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Table object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (TRFTABLE_HEADER)
#define TRFTABLE_HEADER
#include "trctldta.h"
#include "constdef.h"
#include "dmltable.h" // Table
#include "trfcolum.h" // columns
#include "trfindex.h" // index
#include "trfrule.h"  // rule
#include "trfinteg.h" // integrity
#include "trfgrant.h" // grantee
#include "trfalarm.h" // security alarm

//
// Object: Table 
// ************************************************************************************************
class CtrfTable: public CtrfItemData
{
	DECLARE_SERIAL(CtrfTable)
public:
	CtrfTable(): CtrfItemData(O_TABLE)
	{
		m_pFolderColumn    = NULL;
		m_pFolderIndex     = NULL;
		m_pFolderRule      = NULL;
		m_pFolderIntegrity = NULL;
		m_pFolderGrantee   = NULL;
		m_pFolderAlarm     = NULL;
	}
	CtrfTable(CaTable* pObj): CtrfItemData(O_TABLE)
	{
		m_pFolderColumn    = NULL;
		m_pFolderIndex     = NULL;
		m_pFolderRule      = NULL;
		m_pFolderIntegrity = NULL;
		m_pFolderGrantee   = NULL;
		m_pFolderAlarm     = NULL;
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	void Initialize();
	virtual ~CtrfTable();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual CString GetDisplayedItem(int nMode = 0);

	CtrfFolderColumn*     GetFolderColumn(){return m_pFolderColumn;}
	CtrfFolderIndex*      GetFolderIndex() {return m_pFolderIndex;}
	CtrfFolderRule*       GetFolderRule() {return m_pFolderRule;}
	CtrfFolderIntegrity*  GetFolderIntegrity() {return m_pFolderIntegrity;}
	CtrfFolderGrantee*    GetFolderGrantee() {return m_pFolderGrantee;}
	CtrfFolderAlarm*      GetFolderAlarm() {return m_pFolderAlarm;}

	CaTable& GetObject(){return m_object;}
	void GetObject(CaTable*& pObject){pObject = new CaTable(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaTable m_object;

	CtrfFolderColumn*    m_pFolderColumn;
	CtrfFolderIndex*     m_pFolderIndex;
	CtrfFolderRule*      m_pFolderRule;
	CtrfFolderIntegrity* m_pFolderIntegrity;
	CtrfFolderGrantee*   m_pFolderGrantee;
	CtrfFolderAlarm*     m_pFolderAlarm;

};


//
// Object: Folder of Table 
// ************************************************************************************************
class CtrfFolderTable: public CtrfItemData
{
	DECLARE_SERIAL (CtrfFolderTable)
public:
	CtrfFolderTable();
	virtual ~CtrfFolderTable();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual UINT GetFolderFlag(){return FOLDER_TABLE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_TABLE);}

	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	CtrfItemData* SearchObject(CaTable* pObj, BOOL bMatchOwner = TRUE);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;


};


#endif // TRFTABLE_HEADER
